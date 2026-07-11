using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace UniversalXRBridge.Launcher.Services.Ipc;

public enum UxrbMessageKind : ushort
{
    Handshake = 1,
    AuthResult = 2,
    Command = 3,
    Event = 4,
    Response = 5,
    Error = 6,
    Shutdown = 7,
}

public sealed record UxrbMessage(UxrbMessageKind Kind, ulong RequestId, JsonObject Payload);

public static class UxrbIpcProtocol
{
    public const ushort Version = 1;
    public const int HeaderSize = 24;
    public const int MaximumPayloadSize = 1024 * 1024;
    private static ReadOnlySpan<byte> Magic => "UXRB"u8;

    public static byte[] Encode(UxrbMessage message)
    {
        ValidateKind(message.Kind);
        var payload = JsonSerializer.SerializeToUtf8Bytes(message.Payload);
        if (payload.Length > MaximumPayloadSize) throw new InvalidDataException("A mensagem IPC excede 1 MiB.");

        var frame = new byte[HeaderSize + payload.Length];
        Magic.CopyTo(frame);
        BinaryPrimitives.WriteUInt16LittleEndian(frame.AsSpan(4, 2), Version);
        BinaryPrimitives.WriteUInt16LittleEndian(frame.AsSpan(6, 2), (ushort)message.Kind);
        BinaryPrimitives.WriteUInt32LittleEndian(frame.AsSpan(8, 4), (uint)payload.Length);
        BinaryPrimitives.WriteUInt32LittleEndian(frame.AsSpan(12, 4), 0);
        BinaryPrimitives.WriteUInt64LittleEndian(frame.AsSpan(16, 8), message.RequestId);
        payload.CopyTo(frame.AsSpan(HeaderSize));
        return frame;
    }

    public static UxrbMessage Decode(ReadOnlySpan<byte> frame)
    {
        if (frame.Length < HeaderSize) throw new InvalidDataException("Frame IPC menor que o cabeçalho.");
        if (!frame[..4].SequenceEqual(Magic)) throw new InvalidDataException("Magic IPC inválido.");
        var version = BinaryPrimitives.ReadUInt16LittleEndian(frame.Slice(4, 2));
        if (version != Version) throw new InvalidDataException("Versão de protocolo IPC incompatível.");

        var kind = (UxrbMessageKind)BinaryPrimitives.ReadUInt16LittleEndian(frame.Slice(6, 2));
        ValidateKind(kind);
        var payloadLength = checked((int)BinaryPrimitives.ReadUInt32LittleEndian(frame.Slice(8, 4)));
        if (payloadLength > MaximumPayloadSize) throw new InvalidDataException("A mensagem IPC excede 1 MiB.");
        if (BinaryPrimitives.ReadUInt32LittleEndian(frame.Slice(12, 4)) != 0)
            throw new InvalidDataException("Flags IPC desconhecidas.");
        if (frame.Length != HeaderSize + payloadLength) throw new InvalidDataException("Tamanho do payload IPC divergente.");
        var requestId = BinaryPrimitives.ReadUInt64LittleEndian(frame.Slice(16, 8));

        JsonNode? node;
        try { node = JsonNode.Parse(frame.Slice(HeaderSize, payloadLength)); }
        catch (JsonException exception) { throw new InvalidDataException("Payload IPC não contém JSON válido.", exception); }
        if (node is not JsonObject payload) throw new InvalidDataException("Payload IPC deve ser um objeto JSON.");
        return new UxrbMessage(kind, requestId, payload);
    }

    public static string GenerateSessionToken() => Convert.ToHexString(RandomNumberGenerator.GetBytes(32)).ToLowerInvariant();

    public static string GenerateSessionId() => Convert.ToHexString(RandomNumberGenerator.GetBytes(12)).ToLowerInvariant();

    public static void ValidateSessionId(string sessionId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(sessionId);
        if (sessionId.Length > 128 || sessionId.Any(character => !char.IsAsciiLetterOrDigit(character) && character is not '-' and not '_'))
            throw new ArgumentException("Identificador de sessão IPC inválido.", nameof(sessionId));
    }

    private static void ValidateKind(UxrbMessageKind kind)
    {
        if (kind is < UxrbMessageKind.Handshake or > UxrbMessageKind.Shutdown)
            throw new InvalidDataException("Tipo de mensagem IPC desconhecido.");
    }
}
