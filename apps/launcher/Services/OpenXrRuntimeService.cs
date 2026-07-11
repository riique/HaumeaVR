using System.Text.Json;
using Microsoft.Win32;
using UniversalXRBridge.Launcher.Models;

namespace UniversalXRBridge.Launcher.Services;

public sealed class OpenXrRuntimeService
{
    private const string RegistryPath = @"SOFTWARE\Khronos\OpenXR\1";

    public Task<OpenXrRuntimeSnapshot> InspectAsync(CancellationToken cancellationToken = default) =>
        Task.Run(() => Inspect(cancellationToken), cancellationToken);

    private static OpenXrRuntimeSnapshot Inspect(CancellationToken cancellationToken)
    {
        var entries = new Dictionary<string, (bool Enabled, bool Active)>(StringComparer.OrdinalIgnoreCase);
        var active = ReadActiveRuntime(RegistryHive.CurrentUser) ?? ReadActiveRuntime(RegistryHive.LocalMachine);

        foreach (var hive in new[] { RegistryHive.CurrentUser, RegistryHive.LocalMachine })
        {
            cancellationToken.ThrowIfCancellationRequested();
            try
            {
                using var baseKey = RegistryKey.OpenBaseKey(hive, RegistryView.Registry64);
                using var available = baseKey.OpenSubKey($@"{RegistryPath}\AvailableRuntimes");
                if (available is null) continue;
                foreach (var name in available.GetValueNames())
                {
                    var enabled = available.GetValue(name) is int state && state == 0;
                    entries[name] = (enabled, string.Equals(name, active, StringComparison.OrdinalIgnoreCase));
                }
            }
            catch (Exception exception) when (exception is UnauthorizedAccessException or System.Security.SecurityException) { }
        }

        if (!string.IsNullOrWhiteSpace(active) && !entries.ContainsKey(active)) entries[active] = (true, true);
        var runtimes = entries.Select(entry => ReadManifest(entry.Key, entry.Value.Enabled, entry.Value.Active)).ToArray();
        return new OpenXrRuntimeSnapshot(active, runtimes, DateTimeOffset.Now);
    }

    private static string? ReadActiveRuntime(RegistryHive hive)
    {
        try
        {
            using var baseKey = RegistryKey.OpenBaseKey(hive, RegistryView.Registry64);
            return baseKey.OpenSubKey(RegistryPath)?.GetValue("ActiveRuntime") as string;
        }
        catch (Exception exception) when (exception is UnauthorizedAccessException or System.Security.SecurityException)
        {
            return null;
        }
    }

    private static RegisteredRuntime ReadManifest(string manifestPath, bool enabled, bool active)
    {
        if (!File.Exists(manifestPath))
        {
            return new RegisteredRuntime(manifestPath, Path.GetFileNameWithoutExtension(manifestPath), null, false, enabled, active);
        }

        try
        {
            using var document = JsonDocument.Parse(File.ReadAllText(manifestPath));
            var root = document.RootElement;
            var runtime = root.TryGetProperty("runtime", out var runtimeElement) ? runtimeElement : root;
            var name = runtime.TryGetProperty("name", out var nameElement) && nameElement.ValueKind == JsonValueKind.String
                ? nameElement.GetString()
                : null;
            var library = runtime.TryGetProperty("library_path", out var libraryElement) && libraryElement.ValueKind == JsonValueKind.String
                ? libraryElement.GetString()
                : null;
            if (!string.IsNullOrWhiteSpace(library) && !Path.IsPathFullyQualified(library))
            {
                library = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(manifestPath)!, library));
            }
            return new RegisteredRuntime(
                manifestPath,
                string.IsNullOrWhiteSpace(name) ? Path.GetFileNameWithoutExtension(manifestPath) : name,
                library,
                true,
                enabled,
                active);
        }
        catch (Exception exception) when (exception is IOException or JsonException)
        {
            return new RegisteredRuntime(manifestPath, Path.GetFileNameWithoutExtension(manifestPath), null, true, enabled, active);
        }
    }
}
