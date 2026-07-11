#include "uxrb/unity_transport.hpp"
#include "uxrb/unity_shared_texture_source.hpp"

#include <Windows.h>

#include <cstdlib>
#include <cwchar>
#include <iostream>
#include <string>

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Unity transport test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    const std::wstring name = L"Local\\UXRB-Unity-test-" + std::to_wstring(GetCurrentProcessId());
    std::string error;
    uxrb::unity_transport::Mapping owner;
    uxrb::unity_transport::Mapping client;
    if (!owner.createOwner(name, error)) require(false, error.c_str());
    uxrb::unity_transport::Mapping duplicateOwner;
    std::string duplicateError;
    require(
        !duplicateOwner.createOwner(name, duplicateError),
        "a second owner was allowed to attach to an existing mapping");
    require(
        duplicateError.find("already exists") != std::string::npos,
        "duplicate owner refusal did not explain the mapping collision");
    if (!client.openClient(name, error)) require(false, error.c_str());

    uxrb::unity_transport::StereoFrame sentFrame{};
    sentFrame.frameId = 99;
    sentFrame.predictedDisplayTimeNanoseconds = 123456789;
    sentFrame.recommendedWidth = 1512;
    sentFrame.recommendedHeight = 1680;
    sentFrame.eyes[0].position[0] = -0.032F;
    sentFrame.eyes[1].position[0] = 0.032F;
    require(owner.publishFrame(sentFrame), "owner did not publish a frame");

    uxrb::unity_transport::StereoFrame receivedFrame{};
    require(client.readFrame(receivedFrame), "client did not read a stable frame");
    require(receivedFrame.frameId == 99, "frame identity changed");
    require(receivedFrame.recommendedWidth == 1512, "frame dimensions changed");

    uxrb::unity_transport::PluginState sentPlugin{};
    sentPlugin.processId = GetCurrentProcessId();
    sentPlugin.readyEyeMask = 3;
    sentPlugin.copiedFrameId = 99;
    sentPlugin.eyes[0].generation = 7;
    wcscpy_s(sentPlugin.eyes[0].sharedName, L"Local\\UXRB-Unity-texture-test");
    require(client.publishPluginState(sentPlugin), "client did not publish plug-in state");

    uxrb::unity_transport::PluginState receivedPlugin{};
    require(owner.readPluginState(receivedPlugin), "owner did not read stable plug-in state");
    require(receivedPlugin.readyEyeMask == 3, "ready eye mask changed");
    require(receivedPlugin.eyes[0].generation == 7, "texture generation changed");
    require(
        std::wstring(receivedPlugin.eyes[0].sharedName) == L"Local\\UXRB-Unity-texture-test",
        "texture name changed");

    owner.requestShutdown();
    require(client.shutdownRequested(), "shutdown signal was not shared");
    client.close();
    owner.close();
    std::string sharedTextureFailure;
    if (!uxrb::runUnitySharedTextureSourceSelfTest(sharedTextureFailure)) {
        require(false, sharedTextureFailure.c_str());
    }
    std::cout << "Unity cross-process seqlock transport test passed.\n";
    return EXIT_SUCCESS;
}
