namespace UniversalXRBridge.Launcher.Services;

public sealed record UnityBridgeDeployment(
    bool Ready,
    string? DataDirectory,
    string? NativePluginPath,
    string? ManagedAssemblyPath,
    string Reason);

public sealed class UnityBridgeDeploymentService
{
    public UnityBridgeDeployment Inspect(string executablePath)
    {
        if (string.IsNullOrWhiteSpace(executablePath) || !File.Exists(executablePath))
        {
            return new(false, null, null, null, "Executável Unity inexistente.");
        }

        var executable = Path.GetFullPath(executablePath);
        var directory = Path.GetDirectoryName(executable)!;
        var dataDirectory = Path.Combine(directory, $"{Path.GetFileNameWithoutExtension(executable)}_Data");
        if (!Directory.Exists(dataDirectory))
        {
            return new(false, dataDirectory, null, null, "Diretório _Data do player Unity não encontrado.");
        }

        var nativeCandidates = new[]
        {
            Path.Combine(dataDirectory, "Plugins", "x86_64", "uxrb_unity_bridge.dll"),
            Path.Combine(dataDirectory, "Plugins", "uxrb_unity_bridge.dll"),
        };
        var nativePlugin = nativeCandidates.FirstOrDefault(File.Exists);
        var managedAssembly = Path.Combine(dataDirectory, "Managed", "Haumea.UniversalXRBridge.Runtime.dll");
        if (nativePlugin is null)
        {
            return new(false, dataDirectory, null, File.Exists(managedAssembly) ? managedAssembly : null,
                "Plug-in nativo UXRB não está implantado neste player.");
        }
        if (!File.Exists(managedAssembly))
        {
            return new(false, dataDirectory, nativePlugin, null,
                "Assembly gerenciado UXRB não está implantado neste player Mono.");
        }

        return new(true, dataDirectory, nativePlugin, managedAssembly,
            "Bridge Unity Mono controlado pronto para D3D11 stereo.");
    }
}
