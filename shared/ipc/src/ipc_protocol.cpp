#include "uxrb/ipc/ipc_protocol.hpp"

#include <Windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace uxrb::ipc {
namespace {

constexpr std::array<std::byte, 4> magic{
    std::byte{'U'},
    std::byte{'X'},
    std::byte{'R'},
    std::byte{'B'},
};

template <typename Integer>
requires std::is_unsigned_v<Integer>
void writeLittleEndian(
    const std::span<std::byte> output,
    const std::size_t offset,
    const Integer value) {
    if (offset + sizeof(Integer) > output.size()) {
        throw std::out_of_range("IPC header write exceeded the destination buffer.");
    }
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        output[offset + index] = static_cast<std::byte>(
            static_cast<unsigned char>((value >> (index * 8U)) & 0xFFU));
    }
}

template <typename Integer>
requires std::is_unsigned_v<Integer>
[[nodiscard]] Integer readLittleEndian(
    const std::span<const std::byte> input,
    const std::size_t offset) {
    if (offset + sizeof(Integer) > input.size()) {
        throw std::out_of_range("IPC header read exceeded the source buffer.");
    }
    Integer value = 0;
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        value |= static_cast<Integer>(std::to_integer<unsigned char>(input[offset + index]))
            << (index * 8U);
    }
    return value;
}

[[nodiscard]] bool validKind(const MessageKind kind) noexcept {
    const auto value = static_cast<std::uint16_t>(kind);
    return value >= static_cast<std::uint16_t>(MessageKind::handshake)
        && value <= static_cast<std::uint16_t>(MessageKind::shutdown);
}

} // namespace

std::array<std::byte, frameHeaderSize> encodeHeader(const FrameHeader& header) {
    if (!validKind(header.kind)) {
        throw std::invalid_argument("IPC frame contains an unknown message kind.");
    }
    if (header.payloadSize > maximumPayloadSize) {
        throw std::length_error("IPC payload exceeds the 1 MiB protocol limit.");
    }
    if (header.flags != 0) {
        throw std::invalid_argument("IPC frame uses unsupported flags.");
    }

    std::array<std::byte, frameHeaderSize> output{};
    std::ranges::copy(magic, output.begin());
    const std::span<std::byte> bytes(output);
    writeLittleEndian<std::uint16_t>(bytes, 4, protocolVersion);
    writeLittleEndian<std::uint16_t>(bytes, 6, static_cast<std::uint16_t>(header.kind));
    writeLittleEndian<std::uint32_t>(bytes, 8, header.payloadSize);
    writeLittleEndian<std::uint32_t>(bytes, 12, header.flags);
    writeLittleEndian<std::uint64_t>(bytes, 16, header.requestId);
    return output;
}

FrameHeader decodeHeader(const std::span<const std::byte> bytes) {
    if (bytes.size() != frameHeaderSize) {
        throw std::invalid_argument("IPC frame header must contain exactly 24 bytes.");
    }
    if (!std::ranges::equal(magic, bytes.first(magic.size()))) {
        throw std::invalid_argument("IPC frame magic does not match UXRB.");
    }

    const std::uint16_t version = readLittleEndian<std::uint16_t>(bytes, 4);
    if (version != protocolVersion) {
        throw std::invalid_argument("IPC protocol version is unsupported.");
    }

    FrameHeader header;
    header.kind = static_cast<MessageKind>(readLittleEndian<std::uint16_t>(bytes, 6));
    header.payloadSize = readLittleEndian<std::uint32_t>(bytes, 8);
    header.flags = readLittleEndian<std::uint32_t>(bytes, 12);
    header.requestId = readLittleEndian<std::uint64_t>(bytes, 16);

    if (!validKind(header.kind)) {
        throw std::invalid_argument("IPC frame contains an unknown message kind.");
    }
    if (header.payloadSize > maximumPayloadSize) {
        throw std::length_error("IPC payload exceeds the 1 MiB protocol limit.");
    }
    if (header.flags != 0) {
        throw std::invalid_argument("IPC frame uses unsupported flags.");
    }
    return header;
}

std::vector<std::byte> encodeFrame(const Message& message) {
    const std::string payload = message.payload.dump(
        -1,
        ' ',
        false,
        nlohmann::json::error_handler_t::strict);
    if (payload.size() > maximumPayloadSize) {
        throw std::length_error("IPC payload exceeds the 1 MiB protocol limit.");
    }

    const FrameHeader header{
        message.kind,
        static_cast<std::uint32_t>(payload.size()),
        0,
        message.requestId,
    };
    const auto encodedHeader = encodeHeader(header);

    std::vector<std::byte> frame(frameHeaderSize + payload.size());
    std::ranges::copy(encodedHeader, frame.begin());
    std::transform(
        payload.begin(),
        payload.end(),
        frame.begin() + static_cast<std::ptrdiff_t>(frameHeaderSize),
        [](const char value) { return static_cast<std::byte>(static_cast<unsigned char>(value)); });
    return frame;
}

Message decodeFrame(const std::span<const std::byte> bytes) {
    if (bytes.size() < frameHeaderSize) {
        throw std::invalid_argument("IPC frame is shorter than its header.");
    }
    const FrameHeader header = decodeHeader(bytes.first(frameHeaderSize));
    if (bytes.size() != frameHeaderSize + header.payloadSize) {
        throw std::invalid_argument("IPC frame length does not match its payload size.");
    }

    const auto payloadBytes = bytes.subspan(frameHeaderSize, header.payloadSize);
    std::string payload(payloadBytes.size(), '\0');
    std::transform(
        payloadBytes.begin(),
        payloadBytes.end(),
        payload.begin(),
        [](const std::byte value) { return static_cast<char>(std::to_integer<unsigned char>(value)); });

    nlohmann::json parsed;
    try {
        parsed = nlohmann::json::parse(payload);
    } catch (const nlohmann::json::exception& exception) {
        throw std::invalid_argument(std::string("IPC payload is not valid JSON: ") + exception.what());
    }
    if (!parsed.is_object()) {
        throw std::invalid_argument("IPC JSON payload must be an object.");
    }

    return Message{header.kind, header.requestId, std::move(parsed)};
}

bool constantTimeEquals(const std::string_view left, const std::string_view right) noexcept {
    const std::size_t length = std::max(left.size(), right.size());
    std::uint64_t difference = static_cast<std::uint64_t>(left.size() ^ right.size());
    for (std::size_t index = 0; index < length; ++index) {
        const unsigned char leftByte = index < left.size()
            ? static_cast<unsigned char>(left[index])
            : 0U;
        const unsigned char rightByte = index < right.size()
            ? static_cast<unsigned char>(right[index])
            : 0U;
        difference |= static_cast<std::uint64_t>(leftByte ^ rightByte);
    }
    return difference == 0;
}

std::string generateSessionToken() {
    std::array<unsigned char, 32> randomBytes{};
    const NTSTATUS status = BCryptGenRandom(
        nullptr,
        randomBytes.data(),
        static_cast<ULONG>(randomBytes.size()),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (status < 0) {
        throw std::runtime_error("BCryptGenRandom could not generate an IPC session token.");
    }

    constexpr std::string_view hexadecimal = "0123456789abcdef";
    std::string token(randomBytes.size() * 2U, '0');
    for (std::size_t index = 0; index < randomBytes.size(); ++index) {
        token[index * 2U] = hexadecimal[(randomBytes[index] >> 4U) & 0x0FU];
        token[index * 2U + 1U] = hexadecimal[randomBytes[index] & 0x0FU];
    }
    return token;
}

} // namespace uxrb::ipc

