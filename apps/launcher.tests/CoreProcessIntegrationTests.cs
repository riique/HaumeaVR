using UniversalXRBridge.Launcher.Services;
using UniversalXRBridge.Launcher.Services.Ipc;
using System.Text.Json.Nodes;

namespace UniversalXRBridge.Launcher.Tests;

[TestClass]
public sealed class CoreProcessIntegrationTests
{
    [TestMethod]
    [TestCategory("Integration")]
    public async Task StartsAuthenticatesQueriesAndStopsNativeCore()
    {
        var service = new CoreProcessService();
        if (service.FindCoreExecutable() is null)
        {
            Assert.Inconclusive("Compile uxrb_core.exe before running native integration tests.");
        }

        await using var session = await service.StartControlSessionAsync();
        var status = await session.RequestAsync("get-status");

        Assert.AreEqual(UxrbMessageKind.Response, status.Kind);
        Assert.AreEqual("status", status.Payload["type"]?.GetValue<string>());
        Assert.AreEqual("idle", status.Payload["data"]?["state"]?.GetValue<string>());
        Assert.AreEqual(UxrbIpcProtocol.Version, status.Payload["data"]?["protocolVersion"]?.GetValue<int>());
        Assert.IsTrue(status.Payload["data"]?["capabilities"]?["virtualScreen"]?.GetValue<bool>());
        Assert.IsFalse(status.Payload["data"]?["safety"]?["kernelDriver"]?.GetValue<bool>());
        Assert.IsFalse(session.HasExited);
        StringAssert.StartsWith(session.UnityTransportName, "Local\\UXRB-Unity-");

        var invalidStart = await session.RequestAsync(
            "start-virtual-screen",
            new JsonObject { ["windowHandle"] = 0 });
        Assert.AreEqual(UxrbMessageKind.Error, invalidStart.Kind);

        var invalidUnityStart = await session.RequestAsync(
            "start-unity-stereo",
            new JsonObject
            {
                ["transportName"] = "Global\\untrusted",
                ["processId"] = 0,
            });
        Assert.AreEqual(UxrbMessageKind.Error, invalidUnityStart.Kind);

        var inactiveRecenter = await session.RequestAsync("recenter");
        Assert.AreEqual(UxrbMessageKind.Error, inactiveRecenter.Kind);

        await session.StopAsync();
        Assert.IsTrue(session.HasExited);
    }
}
