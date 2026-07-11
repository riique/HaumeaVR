using UniversalXRBridge.Launcher.Services;

namespace UniversalXRBridge.Launcher.Tests;

[TestClass]
public sealed class UnityBridgeDeploymentServiceTests
{
    [TestMethod]
    public void RequiresBothManagedAndNativeBridgeModules()
    {
        using var directory = new TestDirectory();
        var executable = Path.Combine(directory.Path, "CompatibilityLab.exe");
        File.WriteAllBytes(executable, []);
        var data = Path.Combine(directory.Path, "CompatibilityLab_Data");
        var native = Path.Combine(data, "Plugins", "x86_64", "uxrb_unity_bridge.dll");
        var managed = Path.Combine(data, "Managed", "Haumea.UniversalXRBridge.Runtime.dll");
        Directory.CreateDirectory(Path.GetDirectoryName(native)!);
        Directory.CreateDirectory(Path.GetDirectoryName(managed)!);

        var service = new UnityBridgeDeploymentService();
        Assert.IsFalse(service.Inspect(executable).Ready);

        File.WriteAllBytes(native, [1]);
        Assert.IsFalse(service.Inspect(executable).Ready);

        File.WriteAllBytes(managed, [1]);
        var deployment = service.Inspect(executable);
        Assert.IsTrue(deployment.Ready);
        Assert.AreEqual(Path.GetFullPath(native), deployment.NativePluginPath);
        Assert.AreEqual(Path.GetFullPath(managed), deployment.ManagedAssemblyPath);
    }
}
