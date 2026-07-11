#include "uxrb/ipc_server.hpp"

#include "uxrb/ipc/named_pipe.hpp"
#include "uxrb/logger.hpp"
#include "uxrb/openxr_app.hpp"

#include <Windows.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#ifndef UXRB_VERSION
#define UXRB_VERSION "unknown"
#endif

namespace uxrb {
namespace {

std::string readSessionToken() {
    const DWORD required = GetEnvironmentVariableW(L"UXRB_IPC_TOKEN", nullptr, 0);
    if (required == 0) {
        throw std::runtime_error("UXRB_IPC_TOKEN is missing from the core process environment.");
    }
    std::wstring wide(required, L'\0');
    const DWORD length = GetEnvironmentVariableW(L"UXRB_IPC_TOKEN", wide.data(), required);
    if (length == 0 || length >= required) {
        throw std::runtime_error("UXRB_IPC_TOKEN could not be read from the process environment.");
    }
    wide.resize(length);
    if (wide.size() != 64 || !std::ranges::all_of(wide, [](const wchar_t character) {
            return (character >= L'0' && character <= L'9')
                || (character >= L'a' && character <= L'f')
                || (character >= L'A' && character <= L'F');
        })) {
        throw std::runtime_error("UXRB_IPC_TOKEN is not a 256-bit hexadecimal token.");
    }
    std::string token;
    token.reserve(wide.size());
    std::ranges::transform(wide, std::back_inserter(token), [](const wchar_t character) {
        return static_cast<char>(character);
    });
    return token;
}

ipc::Message response(
    const std::uint64_t requestId,
    const nlohmann::json& payload,
    const ipc::MessageKind kind = ipc::MessageKind::response) {
    return ipc::Message{kind, requestId, payload};
}

ipc::Message errorResponse(
    const std::uint64_t requestId,
    const std::string& message,
    const std::string& command = {}) {
    nlohmann::json data{{"message", message}};
    if (!command.empty()) data["command"] = command;
    return response(
        requestId,
        nlohmann::json{{"type", "error"}, {"data", std::move(data)}},
        ipc::MessageKind::error);
}

class SessionHost final {
public:
    ~SessionHost() {
        stop();
    }

    SessionHost(const SessionHost&) = delete;
    SessionHost& operator=(const SessionHost&) = delete;
    SessionHost() = default;

    [[nodiscard]] bool start(const OpenXrRunOptions& options, std::string& error) {
        {
            std::scoped_lock lock(stateMutex_);
            if (state_ == "starting" || state_ == "running") {
                error = "An OpenXR session is already active.";
                return false;
            }
        }
        if (thread_.joinable()) thread_.join();
        application_.reset();
        application_ = std::make_unique<OpenXrApp>();
        OpenXrApp* application = application_.get();
        {
            std::scoped_lock lock(stateMutex_);
            state_ = "starting";
            lastError_.clear();
        }
        thread_ = std::thread([this, application, options] {
            {
                std::scoped_lock lock(stateMutex_);
                state_ = "running";
            }
            try {
                const int result = application->run(options);
                std::scoped_lock lock(stateMutex_);
                state_ = result == 0 ? "stopped" : "failed";
                if (result != 0) lastError_ = "The OpenXR runtime returned exit code " + std::to_string(result) + '.';
            } catch (const std::exception& exception) {
                Logger::instance().error(std::string("OpenXR session failed: ") + exception.what());
                std::scoped_lock lock(stateMutex_);
                state_ = "failed";
                lastError_ = exception.what();
            }
        });
        error.clear();
        return true;
    }

    [[nodiscard]] bool recenter(std::string& error) {
        std::scoped_lock lock(stateMutex_);
        if (application_ == nullptr || (state_ != "starting" && state_ != "running")) {
            error = "No OpenXR session is active.";
            return false;
        }
        application_->requestRecenter();
        error.clear();
        return true;
    }

    void stop() noexcept {
        if (application_ != nullptr) application_->requestExit();
        if (thread_.joinable()) thread_.join();
        application_.reset();
        std::scoped_lock lock(stateMutex_);
        if (state_ != "failed") state_ = "stopped";
    }

    [[nodiscard]] nlohmann::json status() const {
        std::scoped_lock lock(stateMutex_);
        nlohmann::json result{{"state", state_}};
        if (!lastError_.empty()) result["lastError"] = lastError_;
        return result;
    }

private:
    mutable std::mutex stateMutex_;
    std::unique_ptr<OpenXrApp> application_;
    std::thread thread_;
    std::string state_{"idle"};
    std::string lastError_;
};

std::optional<OpenXrRunOptions> parseVirtualScreenOptions(
    const nlohmann::json& payload,
    std::string& error) {
    const auto dataIterator = payload.find("data");
    if (dataIterator == payload.end() || !dataIterator->is_object()) {
        error = "start-virtual-screen requires a data object.";
        return std::nullopt;
    }
    const auto& data = *dataIterator;
    const std::uint64_t handleValue = data.value("windowHandle", std::uint64_t{0});
    if (handleValue == 0 || handleValue > std::numeric_limits<std::uintptr_t>::max()) {
        error = "windowHandle is invalid.";
        return std::nullopt;
    }
    const HWND window = reinterpret_cast<HWND>(static_cast<std::uintptr_t>(handleValue));
    if (!IsWindow(window)) {
        error = "windowHandle does not identify a live Windows window.";
        return std::nullopt;
    }

    OpenXrRunOptions options;
    options.sceneMode = OpenXrSceneMode::virtualScreen;
    options.captureWindow = window;
    options.virtualScreen.widthMeters = data.value("widthMeters", options.virtualScreen.widthMeters);
    options.virtualScreen.distanceMeters = data.value("distanceMeters", options.virtualScreen.distanceMeters);
    options.virtualScreen.curvature = data.value("curvature", options.virtualScreen.curvature);
    error.clear();
    return options;
}

std::optional<OpenXrRunOptions> parseUnityStereoOptions(
    const nlohmann::json& payload,
    std::string& error) {
    const auto dataIterator = payload.find("data");
    if (dataIterator == payload.end() || !dataIterator->is_object()) {
        error = "start-unity-stereo requires a data object.";
        return std::nullopt;
    }
    const auto& data = *dataIterator;
    const auto nameIterator = data.find("transportName");
    if (nameIterator == data.end() || !nameIterator->is_string()) {
        error = "transportName must be a versioned local Unity transport name.";
        return std::nullopt;
    }
    const std::string transportName = nameIterator->get<std::string>();
    if (!std::ranges::all_of(transportName, [](const unsigned char character) {
            return character >= 0x20U && character <= 0x7EU;
        })) {
        error = "transportName must contain ASCII characters only.";
        return std::nullopt;
    }
    const std::wstring wideName(transportName.begin(), transportName.end());
    if (!unity_transport::Mapping::validName(wideName)) {
        error = "transportName is invalid.";
        return std::nullopt;
    }

    std::uint32_t processId = 0;
    if (const auto processIterator = data.find("processId"); processIterator != data.end()) {
        if (!processIterator->is_number_unsigned()) {
            error = "processId must be an unsigned 32-bit integer.";
            return std::nullopt;
        }
        const std::uint64_t value = processIterator->get<std::uint64_t>();
        if (value > std::numeric_limits<std::uint32_t>::max()) {
            error = "processId exceeds the unsigned 32-bit range.";
            return std::nullopt;
        }
        processId = static_cast<std::uint32_t>(value);
    }

    OpenXrRunOptions options;
    options.sceneMode = OpenXrSceneMode::unityStereo;
    options.unityTransportName = wideName;
    options.unityProcessId = processId;
    error.clear();
    return options;
}

} // namespace

int runIpcServer(const std::wstring& sessionId) {
    const std::string token = readSessionToken();
    ipc::NamedPipeServer server(sessionId);
    Logger::instance().info("IPC server waiting for an authenticated launcher connection.");
    auto connection = server.accept();
    if (!ipc::performServerHandshake(connection, token)) {
        Logger::instance().warning("IPC client authentication was rejected.");
        return 4;
    }
    Logger::instance().info("IPC launcher connection authenticated.");
    SessionHost sessionHost;

    for (;;) {
        ipc::Message message;
        try {
            message = connection.receive();
        } catch (const std::exception& exception) {
            Logger::instance().warning(std::string("IPC connection ended: ") + exception.what());
            return 0;
        }

        if (message.kind == ipc::MessageKind::shutdown) {
            sessionHost.stop();
            connection.send(response(
                message.requestId,
                nlohmann::json{{"type", "status"}, {"data", {{"state", "stopped"}}}}));
            return 0;
        }
        if (message.kind != ipc::MessageKind::command) {
            connection.send(response(
                message.requestId,
                nlohmann::json{{"type", "error"}, {"data", {{"message", "Expected a Command frame."}}}},
                ipc::MessageKind::error));
            continue;
        }

        const std::string type = message.payload.value("type", "");
        if (type == "get-status") {
            nlohmann::json data = sessionHost.status();
            data["version"] = UXRB_VERSION;
            data["protocolVersion"] = ipc::protocolVersion;
            data["capabilities"] = {
                {"unityNative", false},
                {"geometryStereo", false},
                {"depth3D", false},
                {"virtualScreen", true},
            };
            data["validatedFoundations"] = {
                {"unityD3D11SharedTextures", true},
                {"d3d11PresentObservation", true},
                {"depthComputeReconstruction", true},
            };
            data["safety"] = {
                {"kernelDriver", false},
                {"antiCheatBypass", false},
                {"unsignedCommunityCode", false},
            };
            connection.send(response(
                message.requestId,
                nlohmann::json{
                    {"type", "status"},
                    {"data", std::move(data)},
                }));
        } else if (type == "start-virtual-screen") {
            std::string error;
            const auto options = parseVirtualScreenOptions(message.payload, error);
            if (!options.has_value() || !sessionHost.start(*options, error)) {
                connection.send(errorResponse(message.requestId, error, type));
                continue;
            }
            connection.send(response(
                message.requestId,
                nlohmann::json{
                    {"type", "status"},
                    {"data", {{"state", "starting"}, {"mode", "virtual-screen"}}},
                }));
        } else if (type == "start-unity-stereo") {
            std::string error;
            const auto options = parseUnityStereoOptions(message.payload, error);
            if (!options.has_value() || !sessionHost.start(*options, error)) {
                connection.send(errorResponse(message.requestId, error, type));
                continue;
            }
            connection.send(response(
                message.requestId,
                nlohmann::json{
                    {"type", "status"},
                    {"data", {{"state", "starting"}, {"mode", "unity-stereo"}}},
                }));
        } else if (type == "stop-session") {
            sessionHost.stop();
            connection.send(response(
                message.requestId,
                nlohmann::json{{"type", "status"}, {"data", {{"state", "stopped"}}}}));
            return 0;
        } else if (type == "recenter") {
            std::string error;
            if (!sessionHost.recenter(error)) {
                connection.send(errorResponse(message.requestId, error, type));
                continue;
            }
            connection.send(response(
                message.requestId,
                nlohmann::json{{"type", "status"}, {"data", {{"state", "recenter-requested"}}}}));
        } else {
            connection.send(errorResponse(message.requestId, "Unsupported command.", type));
        }
    }
}

} // namespace uxrb
