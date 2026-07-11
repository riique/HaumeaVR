#include "uxrb/unity_bridge.h"

#include <cstdlib>
#include <iostream>

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Unity bridge test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    require(UxrbUnity_GetApiVersion() == uxrb::unity::kApiVersion, "API version handshake mismatch");
    require(
        UxrbUnity_GetCopyStereoFrameEventId() == uxrb::unity::kCopyStereoFrameEvent,
        "render event identifier mismatch");
    require(UxrbUnity_GetRenderEventFunc() != nullptr, "render event function is missing");

    uxrb::unity::BridgeStatus status{};
    require(UxrbUnity_GetStatus(&status) == 1, "status query failed");
    require(status.readyEyeMask == 0, "bridge must start without registered textures");
    require(UxrbUnity_RegisterEyeTexture(-1, nullptr, 0, 0) == 0, "invalid eye was accepted");

    uxrb::unity::StereoFrame source{};
    source.frameId = 42;
    source.recommendedWidth = 1512;
    source.recommendedHeight = 1680;
    source.eyes[0].position[0] = -0.032F;
    source.eyes[1].position[0] = 0.032F;
    require(UxrbUnity_SetStereoFrame(&source) == 1, "stereo frame was rejected");

    uxrb::unity::StereoFrame received{};
    require(UxrbUnity_TryGetStereoFrame(&received) == 1, "stereo frame was not returned");
    require(received.frameId == 42, "stereo frame identity changed");
    require(received.eyes[0].position[0] == -0.032F, "left eye pose changed");
    require(received.eyes[1].position[0] == 0.032F, "right eye pose changed");

    UxrbUnity_Reset();
    received = {};
    require(UxrbUnity_TryGetStereoFrame(&received) == 0, "reset retained a stale stereo frame");

    std::cout << "Unity bridge ABI and lifecycle self-test passed.\n";
    return EXIT_SUCCESS;
}
