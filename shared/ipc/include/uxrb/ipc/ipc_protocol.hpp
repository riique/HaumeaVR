#pragma once

#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace uxrb::ipc {

inline constexpr std::uint16_t protocolVersion = 1;
inline constexpr std::size_t frameHeaderSize = 24;
inline constexpr std::uint32_t maximumPayloadSize = 1024U * 1024U;

enum class MessageKind : std::uint16_t {
    handshake = 1,
    authResult = 2,
    command = 3,
    event = 4,
    response = 5,
    error = 6,
    shutdown = 7,
};

struct FrameHeader {
    MessageKind kind{MessageKind::error};
    std::uint32_t payloadSize{0};
    std::uint32_t flags{0};
    std::uint64_t requestId{0};
};

struct Message {
    MessageKind kind{MessageKind::error};
    std::uint64_t requestId{0};
    nlohmann::json payload = nlohmann::json::object();
};

[[nodiscard]] std::array<std::byte, frameHeaderSize> encodeHeader(const FrameHeader& header);
[[nodiscard]] FrameHeader decodeHeader(std::span<const std::byte> bytes);

[[nodiscard]] std::vector<std::byte> encodeFrame(const Message& message);
[[nodiscard]] Message decodeFrame(std::span<const std::byte> bytes);

[[nodiscard]] bool constantTimeEquals(std::string_view left, std::string_view right) noexcept;
[[nodiscard]] std::string generateSessionToken();

} // namespace uxrb::ipc

