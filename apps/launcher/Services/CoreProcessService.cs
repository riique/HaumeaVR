using System.Diagnostics;
using System.Text;
using System.Text.Json.Nodes;
using UniversalXRBridge.Launcher.Services.Ipc;

namespace UniversalXRBridge.Launcher.Services;

public sealed record CoreDiagnosticResult(int ExitCode, string Output, string ExecutablePath)
{
    public bool Successful => ExitCode == 0;
}

public sealed class CoreControlSession : IAsyncDisposable
{
    private readonly Process process;
    private readonly UxrbIpcClient client;
    private readonly StringBuilder output;
    private readonly object outputGate;
    private bool stopped;

    internal CoreControlSession(
        Process process,
        UxrbIpcClient client,
        string executablePath,
        string sessionId,
        StringBuilder output,
        object outputGate)
    {
        this.process = process;
        this.client = client;
        this.output = output;
        this.outputGate = outputGate;
        ExecutablePath = executablePath;
        SessionId = sessionId;
        UnityTransportName = $"Local\\UXRB-Unity-{sessionId}";
    }

    public string ExecutablePath { get; }
    public string SessionId { get; }
    public string UnityTransportName { get; }
    public int ProcessId => process.Id;
    public bool HasExited => process.HasExited;

    public string Output
    {
        get
        {
            lock (outputGate) return output.ToString().Trim();
        }
    }

    public Task<UxrbMessage> RequestAsync(
        string type,
        JsonObject? data = null,
        CancellationToken cancellationToken = default) => client.RequestAsync(type, data, cancellationToken);

    public async Task StopAsync(CancellationToken cancellationToken = default)
    {
        if (stopped) return;
        stopped = true;
        if (!process.HasExited)
        {
            try
            {
                await client.RequestAsync("stop-session", cancellationToken: cancellationToken).ConfigureAwait(false);
                using var timeout = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
                timeout.CancelAfter(TimeSpan.FromSeconds(5));
                await process.WaitForExitAsync(timeout.Token).ConfigureAwait(false);
            }
            catch when (!cancellationToken.IsCancellationRequested)
            {
                if (!process.HasExited) process.Kill(entireProcessTree: true);
                await process.WaitForExitAsync(CancellationToken.None).ConfigureAwait(false);
            }
        }
    }

    public async ValueTask DisposeAsync()
    {
        try { await StopAsync().ConfigureAwait(false); }
        finally
        {
            await client.DisposeAsync().ConfigureAwait(false);
            process.Dispose();
        }
    }
}

public sealed class CoreProcessService
{
    public string? FindCoreExecutable()
    {
        var configured = Environment.GetEnvironmentVariable("UXRB_CORE_PATH");
        if (!string.IsNullOrWhiteSpace(configured) && File.Exists(configured)) return Path.GetFullPath(configured);

        var packagedCandidates = new[]
        {
            Path.Combine(AppContext.BaseDirectory, "uxrb_core.exe"),
            Path.Combine(Environment.CurrentDirectory, "uxrb_core.exe"),
        };
        var packaged = packagedCandidates.FirstOrDefault(File.Exists);
        if (packaged is not null) return Path.GetFullPath(packaged);

        var buildCandidates = new List<string>();
        var starts = new[] { AppContext.BaseDirectory, Environment.CurrentDirectory };
        foreach (var start in starts)
        {
            var directory = new DirectoryInfo(start);
            for (var depth = 0; directory is not null && depth < 8; depth++, directory = directory.Parent)
            {
                buildCandidates.Add(Path.Combine(directory.FullName, "build", "windows-msvc-release", "bin", "uxrb_core.exe"));
                buildCandidates.Add(Path.Combine(directory.FullName, "build", "windows-msvc-debug", "bin", "uxrb_core.exe"));
            }
        }
        return buildCandidates
            .Where(File.Exists)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .OrderByDescending(File.GetLastWriteTimeUtc)
            .FirstOrDefault();
    }

    public async Task<CoreDiagnosticResult> DiagnoseAsync(CancellationToken cancellationToken = default)
    {
        var executable = FindCoreExecutable()
            ?? throw new FileNotFoundException("uxrb_core.exe não foi encontrado. Compile o núcleo nativo primeiro.");
        var startInfo = new ProcessStartInfo(executable, "--diagnose")
        {
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
        };
        using var process = new Process { StartInfo = startInfo };
        if (!process.Start()) throw new InvalidOperationException("Não foi possível iniciar o diagnóstico nativo.");

        var outputTask = process.StandardOutput.ReadToEndAsync(cancellationToken);
        var errorTask = process.StandardError.ReadToEndAsync(cancellationToken);
        try
        {
            await process.WaitForExitAsync(cancellationToken).ConfigureAwait(false);
        }
        catch (OperationCanceledException)
        {
            if (!process.HasExited) process.Kill(entireProcessTree: true);
            throw;
        }
        var output = (await outputTask.ConfigureAwait(false)) + (await errorTask.ConfigureAwait(false));
        return new CoreDiagnosticResult(process.ExitCode, output.Trim(), executable);
    }

    public async Task<CoreControlSession> StartControlSessionAsync(
        string? openXrRuntimeManifest = null,
        CancellationToken cancellationToken = default)
    {
        var executable = FindCoreExecutable()
            ?? throw new FileNotFoundException("uxrb_core.exe não foi encontrado. Compile o núcleo nativo primeiro.");
        var sessionId = UxrbIpcProtocol.GenerateSessionId();
        var token = UxrbIpcProtocol.GenerateSessionToken();
        var startInfo = new ProcessStartInfo(executable)
        {
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
        };
        startInfo.ArgumentList.Add("--ipc-server");
        startInfo.ArgumentList.Add(sessionId);
        startInfo.Environment["UXRB_IPC_TOKEN"] = token;
        if (!string.IsNullOrWhiteSpace(openXrRuntimeManifest))
        {
            var manifest = Path.GetFullPath(openXrRuntimeManifest);
            if (!File.Exists(manifest)) throw new FileNotFoundException("O manifest do runtime OpenXR selecionado não existe.", manifest);
            startInfo.Environment["XR_RUNTIME_JSON"] = manifest;
        }

        var output = new StringBuilder();
        var outputGate = new object();
        var process = new Process { StartInfo = startInfo, EnableRaisingEvents = true };
        process.OutputDataReceived += (_, eventArgs) => AppendOutput(output, outputGate, eventArgs.Data);
        process.ErrorDataReceived += (_, eventArgs) => AppendOutput(output, outputGate, eventArgs.Data);
        if (!process.Start())
        {
            process.Dispose();
            throw new InvalidOperationException("Não foi possível iniciar o endpoint IPC nativo.");
        }
        process.BeginOutputReadLine();
        process.BeginErrorReadLine();

        try
        {
            var client = await UxrbIpcClient.ConnectAsync(
                sessionId,
                token,
                TimeSpan.FromSeconds(8),
                cancellationToken).ConfigureAwait(false);
            return new CoreControlSession(process, client, executable, sessionId, output, outputGate);
        }
        catch (Exception exception)
        {
            if (!process.HasExited) process.Kill(entireProcessTree: true);
            await process.WaitForExitAsync(CancellationToken.None).ConfigureAwait(false);
            var captured = ReadOutput(output, outputGate);
            process.Dispose();
            throw new InvalidOperationException(
                string.IsNullOrWhiteSpace(captured)
                    ? "O núcleo não abriu o endpoint IPC autenticado."
                    : $"O núcleo não abriu o endpoint IPC autenticado.{Environment.NewLine}{captured}",
                exception);
        }
    }

    private static void AppendOutput(StringBuilder output, object gate, string? line)
    {
        if (line is null) return;
        lock (gate) output.AppendLine(line);
    }

    private static string ReadOutput(StringBuilder output, object gate)
    {
        lock (gate) return output.ToString().Trim();
    }
}
