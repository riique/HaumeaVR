#pragma once

#include "uxrb/ipc/ipc_protocol.hpp"

#include <Windows.h>

#include <cstdint>
#include <string>
#include <string_view>

namespace uxrb::ipc {

[[nodiscard]] std::wstring makePipePath(std::wstring_view sessionId);

class NamedPipeConnection final {
public:
    NamedPipeConnection() = default;
    explicit NamedPipeConnection(HANDLE handle) noexcept;
    ~NamedPipeConnection();

    NamedPipeConnection(const NamedPipeConnection&) = delete;
    NamedPipeConnection& operator=(const NamedPipeConnection&) = delete;
    NamedPipeConnection(NamedPipeConnection&& other) noexcept;
    NamedPipeConnection& operator=(NamedPipeConnection&& other) noexcept;

    void send(const Message& message) const;
    [[nodiscard]] Message receive() const;
    void close() noexcept;

    [[nodiscard]] bool valid() const noexcept;

private:
    void readExact(std::span<std::byte> bytes) const;
    void writeExact(std::span<const std::byte> bytes) const;

    HANDLE handle_{INVALID_HANDLE_VALUE};
};

class NamedPipeServer final {
public:
    explicit NamedPipeServer(std::wstring sessionId);

    [[nodiscard]] NamedPipeConnection accept() const;
    [[nodiscard]] const std::wstring& path() const noexcept;

private:
    std::wstring path_;
};

class NamedPipeClient final {
public:
    [[nodiscard]] static NamedPipeConnection connect(
        std::wstring_view sessionId,
        std::uint32_t timeoutMilliseconds = 5000);
};

[[nodiscard]] bool performServerHandshake(
    NamedPipeConnection& connection,
    std::string_view expectedToken,
    std::string_view serverName = "uxrb_core");

void performClientHandshake(
    NamedPipeConnection& connection,
    std::string_view token,
    std::string_view clientName = "launcher");

} // namespace uxrb::ipc

