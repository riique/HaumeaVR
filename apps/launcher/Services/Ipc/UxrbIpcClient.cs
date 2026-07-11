using System.Buffers.Binary;
using System.IO.Pipes;
using System.Text.Json.Nodes;

namespace UniversalXRBridge.Launcher.Services.Ipc;

public sealed class UxrbIpcClient : IAsyncDisposable
{
    private readonly NamedPipeClientStream stream;
    private long nextRequestId;

    private UxrbIpcClient(NamedPipeClientStream stream) => this.stream = stream;

    public static async Task<UxrbIpcClient> ConnectAsync(
        string sessionId,
        string token,
        TimeSpan timeout,
        CancellationToken cancellationToken = default)
    {
        UxrbIpcProtocol.ValidateSessionId(sessionId);
        if (token.Length != 64 || token.Any(character => !Uri.IsHexDigit(character)))
            throw new ArgumentException("Token IPC inválido.", nameof(token));

        var stream = new NamedPipeClientStream(
            ".",
            $"uxrb-{sessionId}",
            PipeDirection.InOut,
            PipeOptions.Asynchronous);
        try
        {
            using var timeoutSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            timeoutSource.CancelAfter(timeout);
            await stream.ConnectAsync(timeoutSource.Token).ConfigureAwait(false);
            var client = new UxrbIpcClient(stream);
            await client.SendAsync(new UxrbMessage(
                UxrbMessageKind.Handshake,
                0,
                new JsonObject
                {
                    ["client"] = "launcher",
                    ["protocolVersion"] = UxrbIpcProtocol.Version,
                    ["token"] = token,
                }), cancellationToken).ConfigureAwait(false);
            var result = await client.ReceiveAsync(cancellationToken).ConfigureAwait(false);
            var accepted = result.Kind == UxrbMessageKind.AuthResult
                && result.Payload["accepted"]?.GetValue<bool>() == true
                && result.Payload["protocolVersion"]?.GetValue<int>() == UxrbIpcProtocol.Version;
            if (!accepted) throw new UnauthorizedAccessException("O núcleo rejeitou a autenticação IPC.");
            return client;
        }
        catch
        {
            await stream.DisposeAsync().ConfigureAwait(false);
            throw;
        }
    }

    public async Task<UxrbMessage> RequestAsync(
        string type,
        JsonObject? data = null,
        CancellationToken cancellationToken = default)
    {
        var requestId = checked((ulong)Interlocked.Increment(ref nextRequestId));
        var payload = new JsonObject { ["type"] = type };
        if (data is not null) payload["data"] = data;
        await SendAsync(new UxrbMessage(UxrbMessageKind.Command, requestId, payload), cancellationToken).ConfigureAwait(false);

        for (; ; )
        {
            var response = await ReceiveAsync(cancellationToken).ConfigureAwait(false);
            if (response.RequestId == requestId && response.Kind is UxrbMessageKind.Response or UxrbMessageKind.Error)
                return response;
        }
    }

    public async Task SendAsync(UxrbMessage message, CancellationToken cancellationToken = default)
    {
        var frame = UxrbIpcProtocol.Encode(message);
        await stream.WriteAsync(frame, cancellationToken).ConfigureAwait(false);
        await stream.FlushAsync(cancellationToken).ConfigureAwait(false);
    }

    public async Task<UxrbMessage> ReceiveAsync(CancellationToken cancellationToken = default)
    {
        var header = new byte[UxrbIpcProtocol.HeaderSize];
        await stream.ReadExactlyAsync(header, cancellationToken).ConfigureAwait(false);
        var payloadLength = checked((int)BinaryPrimitives.ReadUInt32LittleEndian(header.AsSpan(8, 4)));
        if (payloadLength < 0 || payloadLength > UxrbIpcProtocol.MaximumPayloadSize)
            throw new InvalidDataException("Tamanho de payload IPC inválido.");
        var frame = new byte[UxrbIpcProtocol.HeaderSize + payloadLength];
        header.CopyTo(frame, 0);
        if (payloadLength > 0)
            await stream.ReadExactlyAsync(frame.AsMemory(UxrbIpcProtocol.HeaderSize, payloadLength), cancellationToken).ConfigureAwait(false);
        return UxrbIpcProtocol.Decode(frame);
    }

    public ValueTask DisposeAsync() => stream.DisposeAsync();
}
