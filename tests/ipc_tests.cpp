#include "uxrb/ipc/ipc_protocol.hpp"
#include "uxrb/ipc/named_pipe.hpp"

#include <Windows.h>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::wstring uniqueSessionId(const wchar_t* suffix) {
    return L"test-" + std::to_wstring(GetCurrentProcessId()) + L"-"
        + std::to_wstring(GetTickCount64()) + L"-" + suffix;
}

void testFrameRoundTrip() {
    const uxrb::ipc::Message input{
        uxrb::ipc::MessageKind::event,
        42,
        nlohmann::json{
            {"type", "status"},
            {"data", {{"runtime", "VirtualDesktopXR"}, {"message", "Conexão pronta"}}},
        },
    };
    const auto bytes = uxrb::ipc::encodeFrame(input);
    const auto output = uxrb::ipc::decodeFrame(bytes);
    require(output.kind == input.kind, "IPC message kind did not survive framing.");
    require(output.requestId == input.requestId, "IPC request id did not survive framing.");
    require(output.payload == input.payload, "IPC JSON payload did not survive framing.");

    auto corrupted = bytes;
    corrupted[0] = std::byte{'N'};
    bool rejected = false;
    try {
        static_cast<void>(uxrb::ipc::decodeFrame(corrupted));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "IPC decoder accepted a frame with invalid magic.");
}

void testAuthenticatedPipe() {
    const std::wstring sessionId = uniqueSessionId(L"valid");
    const std::string token = uxrb::ipc::generateSessionToken();
    require(token.size() == 64, "Generated IPC token is not 256 bits in hexadecimal form.");

    std::exception_ptr serverFailure;
    std::thread server([&] {
        try {
            uxrb::ipc::NamedPipeServer endpoint(sessionId);
            auto connection = endpoint.accept();
            require(
                uxrb::ipc::performServerHandshake(connection, token),
                "Server rejected the correct IPC token.");

            const auto command = connection.receive();
            require(command.kind == uxrb::ipc::MessageKind::command, "Server did not receive a command.");
            connection.send(uxrb::ipc::Message{
                uxrb::ipc::MessageKind::response,
                command.requestId,
                nlohmann::json{{"type", "status"}, {"data", {{"state", "ready"}}}},
            });
        } catch (...) {
            serverFailure = std::current_exception();
        }
    });

    try {
        auto client = uxrb::ipc::NamedPipeClient::connect(sessionId);
        uxrb::ipc::performClientHandshake(client, token);
        client.send(uxrb::ipc::Message{
            uxrb::ipc::MessageKind::command,
            99,
            nlohmann::json{{"type", "get-status"}},
        });
        const auto response = client.receive();
        require(response.kind == uxrb::ipc::MessageKind::response, "Client did not receive a response.");
        require(response.requestId == 99, "IPC response request id does not match its command.");
        require(response.payload.at("data").at("state") == "ready", "IPC response payload is incorrect.");
    } catch (...) {
        server.join();
        throw;
    }

    server.join();
    if (serverFailure) {
        std::rethrow_exception(serverFailure);
    }
}

void testRejectedToken() {
    const std::wstring sessionId = uniqueSessionId(L"invalid");
    const std::string expectedToken = uxrb::ipc::generateSessionToken();
    const std::string wrongToken = uxrb::ipc::generateSessionToken();

    std::exception_ptr serverFailure;
    std::thread server([&] {
        try {
            uxrb::ipc::NamedPipeServer endpoint(sessionId);
            auto connection = endpoint.accept();
            require(
                !uxrb::ipc::performServerHandshake(connection, expectedToken),
                "Server accepted an incorrect IPC token.");
        } catch (...) {
            serverFailure = std::current_exception();
        }
    });

    bool rejected = false;
    try {
        auto client = uxrb::ipc::NamedPipeClient::connect(sessionId);
        uxrb::ipc::performClientHandshake(client, wrongToken);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    server.join();
    if (serverFailure) {
        std::rethrow_exception(serverFailure);
    }
    require(rejected, "Client authentication did not fail for an incorrect token.");
}

} // namespace

int main() {
    try {
        testFrameRoundTrip();
        testAuthenticatedPipe();
        testRejectedToken();
        require(uxrb::ipc::constantTimeEquals("same", "same"), "Equal token comparison failed.");
        require(!uxrb::ipc::constantTimeEquals("same", "different"), "Unequal token comparison passed.");
        std::cout << "All IPC protocol and named-pipe tests passed.\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "IPC test failure: " << exception.what() << '\n';
        return 1;
    }
}

