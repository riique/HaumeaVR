using System.Text.Json.Nodes;
using UniversalXRBridge.Launcher.Services.Ipc;

namespace UniversalXRBridge.Launcher.Tests;

[TestClass]
public sealed class IpcProtocolTests
{
    [TestMethod]
    public void EncodeDecode_RoundTripsTheNativeWireFormat()
    {
        var message = new UxrbMessage(
            UxrbMessageKind.Command,
            73,
            new JsonObject
            {
                ["type"] = "get-status",
                ["data"] = new JsonObject { ["message"] = "Conexão pronta" },
            });

        var frame = UxrbIpcProtocol.Encode(message);
        var decoded = UxrbIpcProtocol.Decode(frame);

        Assert.AreEqual(message.Kind, decoded.Kind);
        Assert.AreEqual(message.RequestId, decoded.RequestId);
        Assert.AreEqual("get-status", decoded.Payload["type"]?.GetValue<string>());
        Assert.AreEqual("Conexão pronta", decoded.Payload["data"]?["message"]?.GetValue<string>());
    }

    [TestMethod]
    public void Decode_RejectsInvalidMagic()
    {
        var frame = UxrbIpcProtocol.Encode(new UxrbMessage(UxrbMessageKind.Event, 0, new JsonObject { ["type"] = "status" }));
        frame[0] = (byte)'N';
        Assert.Throws<InvalidDataException>(() => UxrbIpcProtocol.Decode(frame));
    }

    [TestMethod]
    public void GenerateSessionToken_ReturnsIndependent256BitTokens()
    {
        var first = UxrbIpcProtocol.GenerateSessionToken();
        var second = UxrbIpcProtocol.GenerateSessionToken();
        Assert.AreEqual(64, first.Length);
        Assert.AreNotEqual(first, second);
    }
}
