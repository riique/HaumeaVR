#pragma once

#include <cstdint>

#if defined(_WIN32)
#if defined(UXRB_UNITY_BRIDGE_EXPORTS)
#define UXRB_UNITY_API __declspec(dllexport)
#else
#define UXRB_UNITY_API __declspec(dllimport)
#endif
#define UXRB_UNITY_CALL __stdcall
#else
#define UXRB_UNITY_API
#define UXRB_UNITY_CALL
#endif

namespace uxrb::unity {

inline constexpr std::uint32_t kApiVersion = 1;
inline constexpr std::int32_t kCopyStereoFrameEvent = 0x55585242;
inline constexpr std::uint32_t kEyeCount = 2;

enum class BridgeError : std::uint32_t {
    none = 0,
    unityInterfacesUnavailable = 1,
    unsupportedRenderer = 2,
    d3d11InterfaceUnavailable = 3,
    invalidEye = 4,
    invalidTexture = 5,
    unsupportedTexture = 6,
    sharedTextureCreationFailed = 7,
    synchronizationFailed = 8,
    copyFailed = 9,
};

struct EyePose final {
    float position[3]{};
    float orientation[4]{0.0F, 0.0F, 0.0F, 1.0F};
    // OpenXR-style asymmetric FOV angles: left, right, up, down (radians).
    float fov[4]{};
};

struct StereoFrame final {
    std::uint32_t structSize{sizeof(StereoFrame)};
    std::uint32_t apiVersion{kApiVersion};
    std::uint64_t frameId{};
    std::int64_t predictedDisplayTimeNanoseconds{};
    std::uint32_t recommendedWidth{};
    std::uint32_t recommendedHeight{};
    std::uint32_t flags{};
    std::uint32_t reserved{};
    EyePose eyes[kEyeCount]{};
};

struct BridgeStatus final {
    std::uint32_t structSize{sizeof(BridgeStatus)};
    std::uint32_t apiVersion{kApiVersion};
    std::uint32_t unityRenderer{};
    std::uint32_t readyEyeMask{};
    std::uint64_t submittedFrameId{};
    std::uint64_t copiedFrameId{};
    std::uint32_t width[kEyeCount]{};
    std::uint32_t height[kEyeCount]{};
    std::uint32_t dxgiFormat[kEyeCount]{};
    std::uint32_t generation[kEyeCount]{};
    BridgeError lastError{BridgeError::none};
    std::uint32_t droppedFrames{};
};

using UnityRenderingEvent = void(UXRB_UNITY_CALL*)(int eventId);

}  // namespace uxrb::unity

extern "C" {

UXRB_UNITY_API std::uint32_t UXRB_UNITY_CALL UxrbUnity_GetApiVersion();
UXRB_UNITY_API std::int32_t UXRB_UNITY_CALL UxrbUnity_GetCopyStereoFrameEventId();
UXRB_UNITY_API uxrb::unity::UnityRenderingEvent UXRB_UNITY_CALL UxrbUnity_GetRenderEventFunc();
UXRB_UNITY_API std::int32_t UXRB_UNITY_CALL UxrbUnity_RegisterEyeTexture(
    std::int32_t eye,
    void* nativeTexture,
    std::uint32_t expectedWidth,
    std::uint32_t expectedHeight);
UXRB_UNITY_API void UXRB_UNITY_CALL UxrbUnity_SubmitFrame(std::uint64_t frameId);
UXRB_UNITY_API std::int32_t UXRB_UNITY_CALL UxrbUnity_SetStereoFrame(
    const uxrb::unity::StereoFrame* frame);
UXRB_UNITY_API std::int32_t UXRB_UNITY_CALL UxrbUnity_TryGetStereoFrame(
    uxrb::unity::StereoFrame* frame);
UXRB_UNITY_API std::int32_t UXRB_UNITY_CALL UxrbUnity_GetStatus(
    uxrb::unity::BridgeStatus* status);
UXRB_UNITY_API std::int32_t UXRB_UNITY_CALL UxrbUnity_CopySharedTextureName(
    std::int32_t eye,
    wchar_t* destination,
    std::int32_t destinationCharacterCount);
UXRB_UNITY_API void UXRB_UNITY_CALL UxrbUnity_Reset();

}
