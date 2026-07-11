using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Win32.SafeHandles;
using UniversalXRBridge.Launcher.Models;

namespace UniversalXRBridge.Launcher.Services;

public sealed record GameWindow(nint Handle, uint ProcessId, string Title, int ClientArea);

public sealed class GameWindowService
{
    public GameWindow? FindBestWindow(string executablePath, uint? preferredProcessId = null)
    {
        var expectedPath = Path.GetFullPath(executablePath);
        var candidates = EnumerateWindows()
            .Where(window => preferredProcessId == window.ProcessId
                || string.Equals(TryGetProcessPath(window.ProcessId), expectedPath, StringComparison.OrdinalIgnoreCase))
            .OrderByDescending(window => preferredProcessId == window.ProcessId)
            .ThenByDescending(window => window.ClientArea)
            .ToArray();
        return candidates.FirstOrDefault();
    }

    public async Task<GameWindow> FindOrLaunchAsync(
        GameProfile profile,
        TimeSpan timeout,
        CancellationToken cancellationToken = default)
    {
        var existing = FindBestWindow(profile.Executable.Path);
        if (existing is not null) return existing;

        return await LaunchAndFindWindowAsync(profile, timeout, null, null, cancellationToken)
            .ConfigureAwait(false);
    }

    public Task<GameWindow> LaunchUnityBridgeAsync(
        GameProfile profile,
        string transportName,
        TimeSpan timeout,
        CancellationToken cancellationToken = default)
    {
        if (profile.Detection.AntiCheat == AntiCheatStatus.Detected)
            throw new InvalidOperationException("Unity Stereo é bloqueado para jogos com anti-cheat detectado.");
        const string prefix = "Local\\UXRB-Unity-";
        var suffix = transportName?.StartsWith(prefix, StringComparison.Ordinal) == true
            ? transportName[prefix.Length..]
            : string.Empty;
        if (string.IsNullOrEmpty(suffix)
            || transportName!.Length > 120
            || suffix.Any(character => !char.IsAsciiLetterOrDigit(character) && character != '-'))
            throw new ArgumentException("Nome de transporte Unity inválido.", nameof(transportName));

        var environment = new Dictionary<string, string>(StringComparer.Ordinal)
        {
            ["UXRB_UNITY_TRANSPORT"] = transportName,
            ["UXRB_UNITY_ENABLE"] = "1",
        };
        return LaunchAndFindWindowAsync(
            profile,
            timeout,
            ["-force-d3d11", "-uxrb-enable"],
            environment,
            cancellationToken);
    }

    private async Task<GameWindow> LaunchAndFindWindowAsync(
        GameProfile profile,
        TimeSpan timeout,
        IReadOnlyList<string>? additionalArguments,
        IReadOnlyDictionary<string, string>? environment,
        CancellationToken cancellationToken)
    {

        var startInfo = new ProcessStartInfo(profile.Executable.Path)
        {
            UseShellExecute = false,
            WorkingDirectory = Path.GetDirectoryName(profile.Executable.Path) ?? Environment.CurrentDirectory,
        };
        foreach (var argument in profile.Executable.LaunchArguments) startInfo.ArgumentList.Add(argument);
        if (additionalArguments is not null)
        {
            foreach (var argument in additionalArguments)
            {
                if (!profile.Executable.LaunchArguments.Contains(argument, StringComparer.OrdinalIgnoreCase))
                    startInfo.ArgumentList.Add(argument);
            }
        }
        if (environment is not null)
        {
            foreach (var variable in environment) startInfo.Environment[variable.Key] = variable.Value;
        }
        using var process = Process.Start(startInfo)
            ?? throw new InvalidOperationException("O Windows não iniciou o executável selecionado.");

        var deadline = DateTimeOffset.UtcNow + timeout;
        while (DateTimeOffset.UtcNow < deadline)
        {
            cancellationToken.ThrowIfCancellationRequested();
            var window = FindBestWindow(profile.Executable.Path, checked((uint)process.Id));
            if (window is not null) return window;
            await Task.Delay(250, cancellationToken).ConfigureAwait(false);
        }
        throw new TimeoutException(
            "O jogo foi iniciado, mas nenhuma janela renderizável apareceu em 30 segundos. "
            + "Abra a janela principal do jogo e tente novamente.");
    }

    private static IReadOnlyList<GameWindow> EnumerateWindows()
    {
        var windows = new List<GameWindow>();
        NativeMethods.EnumWindows((window, _) =>
        {
            if (!NativeMethods.IsWindowVisible(window) || NativeMethods.IsIconic(window)) return true;
            var titleLength = NativeMethods.GetWindowTextLength(window);
            if (titleLength <= 0) return true;
            var title = new StringBuilder(titleLength + 1);
            if (NativeMethods.GetWindowText(window, title, title.Capacity) <= 0) return true;
            if (!NativeMethods.GetClientRect(window, out var rectangle)) return true;
            var width = Math.Max(0, rectangle.Right - rectangle.Left);
            var height = Math.Max(0, rectangle.Bottom - rectangle.Top);
            var area = width * height;
            if (area < 160 * 90) return true;
            NativeMethods.GetWindowThreadProcessId(window, out var processId);
            if (processId != 0) windows.Add(new GameWindow(window, processId, title.ToString(), area));
            return true;
        }, 0);
        return windows;
    }

    private static string? TryGetProcessPath(uint processId)
    {
        using var process = NativeMethods.OpenProcess(NativeMethods.ProcessQueryLimitedInformation, false, processId);
        if (process.IsInvalid) return null;
        var capacity = 32768;
        var buffer = new StringBuilder(capacity);
        return NativeMethods.QueryFullProcessImageName(process, 0, buffer, ref capacity)
            ? buffer.ToString()
            : null;
    }

    private static class NativeMethods
    {
        internal const uint ProcessQueryLimitedInformation = 0x1000;
        internal delegate bool EnumWindowsCallback(nint window, nint parameter);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool EnumWindows(EnumWindowsCallback callback, nint parameter);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool IsWindowVisible(nint window);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool IsIconic(nint window);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        internal static extern int GetWindowText(nint window, StringBuilder text, int maximumCount);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        internal static extern int GetWindowTextLength(nint window);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool GetClientRect(nint window, out NativeRectangle rectangle);

        [DllImport("user32.dll")]
        internal static extern uint GetWindowThreadProcessId(nint window, out uint processId);

        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern SafeProcessHandle OpenProcess(uint desiredAccess, [MarshalAs(UnmanagedType.Bool)] bool inheritHandle, uint processId);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool QueryFullProcessImageName(
            SafeProcessHandle process,
            uint flags,
            StringBuilder executableName,
            ref int size);
    }

    [StructLayout(LayoutKind.Sequential)]
    private readonly struct NativeRectangle
    {
        public readonly int Left;
        public readonly int Top;
        public readonly int Right;
        public readonly int Bottom;
    }
}
