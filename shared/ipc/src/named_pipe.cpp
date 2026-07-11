#include "uxrb/ipc/named_pipe.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace uxrb::ipc {
namespace {

std::string windowsError(const std::string_view operation, const DWORD error = GetLastError()) {
    return std::string(operation) + " failed with Windows error " + std::to_string(error) + '.';
}

[[nodiscard]] bool allowedSessionCharacter(const wchar_t value) noexcept {
    return (value >= L'a' && value <= L'z')
        || (value >= L'A' && value <= L'Z')
        || (value >= L'0' && value <= L'9')
        || value == L'-'
        || value == L'_';
}

} // namespace

std::wstring makePipePath(const std::wstring_view sessionId) {
    if (sessionId.empty() || sessionId.size() > 128) {
        throw std::invalid_argument("IPC session id must contain between 1 and 128 characters.");
    }
    if (!std::ranges::all_of(sessionId, allowedSessionCharacter)) {
        throw std::invalid_argument("IPC session id contains an unsafe character.");
    }
    return L"\\\\.\\pipe\\uxrb-" + std::wstring(sessionId);
}

NamedPipeConnection::NamedPipeConnection(const HANDLE handle) noexcept
    : handle_(handle) {}

NamedPipeConnection::~NamedPipeConnection() {
    close();
}

NamedPipeConnection::NamedPipeConnection(NamedPipeConnection&& other) noexcept
    : handle_(std::exchange(other.handle_, INVALID_HANDLE_VALUE)) {}

NamedPipeConnection& NamedPipeConnection::operator=(NamedPipeConnection&& other) noexcept {
    if (this != &other) {
        close();
        handle_ = std::exchange(other.handle_, INVALID_HANDLE_VALUE);
    }
    return *this;
}

void NamedPipeConnection::send(const Message& message) const {
    const std::vector<std::byte> frame = encodeFrame(message);
    writeExact(frame);
}

Message NamedPipeConnection::receive() const {
    std::array<std::byte, frameHeaderSize> encodedHeader{};
    readExact(encodedHeader);
    const FrameHeader header = decodeHeader(encodedHeader);

    std::vector<std::byte> frame(frameHeaderSize + header.payloadSize);
    std::ranges::copy(encodedHeader, frame.begin());
    if (header.payloadSize > 0) {
        readExact(std::span<std::byte>(frame).subspan(frameHeaderSize));
    }
    return decodeFrame(frame);
}

void NamedPipeConnection::close() noexcept {
    if (handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
}

bool NamedPipeConnection::valid() const noexcept {
    return handle_ != INVALID_HANDLE_VALUE;
}

void NamedPipeConnection::readExact(const std::span<std::byte> bytes) const {
    if (!valid()) {
        throw std::logic_error("Cannot read from a closed IPC pipe.");
    }

    std::size_t completed = 0;
    while (completed < bytes.size()) {
        const std::size_t remaining = bytes.size() - completed;
        const DWORD requested = static_cast<DWORD>(std::min<std::size_t>(
            remaining,
            static_cast<std::size_t>(std::numeric_limits<DWORD>::max())));
        DWORD received = 0;
        const BOOL success = ReadFile(
            handle_,
            bytes.data() + static_cast<std::ptrdiff_t>(completed),
            requested,
            &received,
            nullptr);
        if (success == FALSE) {
            throw std::runtime_error(windowsError("ReadFile(named pipe)"));
        }
        if (received == 0) {
            throw std::runtime_error("Named pipe closed before a complete IPC frame was received.");
        }
        completed += received;
    }
}

void NamedPipeConnection::writeExact(const std::span<const std::byte> bytes) const {
    if (!valid()) {
        throw std::logic_error("Cannot write to a closed IPC pipe.");
    }

    std::size_t completed = 0;
    while (completed < bytes.size()) {
        const std::size_t remaining = bytes.size() - completed;
        const DWORD requested = static_cast<DWORD>(std::min<std::size_t>(
            remaining,
            static_cast<std::size_t>(std::numeric_limits<DWORD>::max())));
        DWORD written = 0;
        const BOOL success = WriteFile(
            handle_,
            bytes.data() + static_cast<std::ptrdiff_t>(completed),
            requested,
            &written,
            nullptr);
        if (success == FALSE) {
            throw std::runtime_error(windowsError("WriteFile(named pipe)"));
        }
        if (written == 0) {
            throw std::runtime_error("Named pipe closed before a complete IPC frame was sent.");
        }
        completed += written;
    }
}

NamedPipeServer::NamedPipeServer(std::wstring sessionId)
    : path_(makePipePath(sessionId)) {}

NamedPipeConnection NamedPipeServer::accept() const {
    const HANDLE pipe = CreateNamedPipeW(
        path_.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
        1,
        maximumPayloadSize + static_cast<DWORD>(frameHeaderSize),
        maximumPayloadSize + static_cast<DWORD>(frameHeaderSize),
        0,
        nullptr);
    if (pipe == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(windowsError("CreateNamedPipeW"));
    }

    const BOOL connected = ConnectNamedPipe(pipe, nullptr);
    if (connected == FALSE && GetLastError() != ERROR_PIPE_CONNECTED) {
        const DWORD error = GetLastError();
        CloseHandle(pipe);
        throw std::runtime_error(windowsError("ConnectNamedPipe", error));
    }
    return NamedPipeConnection(pipe);
}

const std::wstring& NamedPipeServer::path() const noexcept {
    return path_;
}

NamedPipeConnection NamedPipeClient::connect(
    const std::wstring_view sessionId,
    const std::uint32_t timeoutMilliseconds) {
    const std::wstring path = makePipePath(sessionId);
    const std::uint64_t deadline = GetTickCount64() + timeoutMilliseconds;

    for (;;) {
        const HANDLE pipe = CreateFileW(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr);
        if (pipe != INVALID_HANDLE_VALUE) {
            return NamedPipeConnection(pipe);
        }

        const DWORD error = GetLastError();
        if (error != ERROR_PIPE_BUSY && error != ERROR_FILE_NOT_FOUND) {
            throw std::runtime_error(windowsError("CreateFileW(named pipe)", error));
        }
        if (GetTickCount64() >= deadline) {
            throw std::runtime_error("Timed out while connecting to the UXRB named pipe.");
        }

        if (error == ERROR_PIPE_BUSY) {
            static_cast<void>(WaitNamedPipeW(path.c_str(), 50));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

bool performServerHandshake(
    NamedPipeConnection& connection,
    const std::string_view expectedToken,
    const std::string_view serverName) {
    bool accepted = false;
    try {
        const Message handshake = connection.receive();
        if (handshake.kind == MessageKind::handshake
            && handshake.payload.is_object()
            && handshake.payload.value("protocolVersion", 0) == protocolVersion
            && handshake.payload.contains("token")
            && handshake.payload.at("token").is_string()) {
            accepted = constantTimeEquals(
                handshake.payload.at("token").get_ref<const std::string&>(),
                expectedToken);
        }
    } catch (const nlohmann::json::exception&) {
        accepted = false;
    }

    connection.send(Message{
        MessageKind::authResult,
        0,
        nlohmann::json{
            {"accepted", accepted},
            {"protocolVersion", protocolVersion},
            {"server", serverName},
        },
    });
    return accepted;
}

void performClientHandshake(
    NamedPipeConnection& connection,
    const std::string_view token,
    const std::string_view clientName) {
    connection.send(Message{
        MessageKind::handshake,
        0,
        nlohmann::json{
            {"client", clientName},
            {"protocolVersion", protocolVersion},
            {"token", token},
        },
    });

    const Message result = connection.receive();
    const bool accepted = result.kind == MessageKind::authResult
        && result.payload.is_object()
        && result.payload.value("accepted", false)
        && result.payload.value("protocolVersion", 0) == protocolVersion;
    if (!accepted) {
        throw std::runtime_error("UXRB named-pipe authentication was rejected.");
    }
}

} // namespace uxrb::ipc
