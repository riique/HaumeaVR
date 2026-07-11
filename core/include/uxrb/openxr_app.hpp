#pragma once

#include "uxrb/desktop_capture.hpp"
#include "uxrb/d3d11_renderer.hpp"
#include "uxrb/mirror_window.hpp"
#include "uxrb/openxr_platform.hpp"
#include "uxrb/unity_shared_texture_source.hpp"
#include "uxrb/unity_transport.hpp"

#include <chrono>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace uxrb {

enum class OpenXrSceneMode {
    diagnosticCube,
    virtualScreen,
    unityStereo,
};

struct OpenXrRunOptions {
    OpenXrSceneMode sceneMode{OpenXrSceneMode::diagnosticCube};
    HWND captureWindow{nullptr};
    VirtualScreenRenderSettings virtualScreen{};
    std::wstring unityTransportName;
    std::uint32_t unityProcessId{};
};

class OpenXrApp final {
public:
    OpenXrApp() = default;
    ~OpenXrApp();

    OpenXrApp(const OpenXrApp&) = delete;
    OpenXrApp& operator=(const OpenXrApp&) = delete;

    int run(const OpenXrRunOptions& options = {});
    int diagnose();
    void requestRecenter() noexcept;
    void requestExit() noexcept;

private:
    struct ViewSwapchain {
        XrSwapchain handle{XR_NULL_HANDLE};
        std::uint32_t width{0};
        std::uint32_t height{0};
        std::vector<XrSwapchainImageD3D11KHR> images;
        std::vector<EyeRenderTarget> renderTargets;
    };

    void logRuntimeRegistration() const;
    void createInstance();
    void selectSystem();
    [[nodiscard]] XrGraphicsRequirementsD3D11KHR graphicsRequirements() const;
    void logViewConfiguration() const;
    void createSession(const XrGraphicsRequirementsD3D11KHR& requirements);
    void createActions();
    void suggestActionBindings();
    void attachActionSet();
    void syncActions();
    void createReferenceSpace();
    void chooseEnvironmentBlendMode();
    void createSwapchains();
    void pollEvents();
    void handleSessionStateChanged(const XrEventDataSessionStateChanged& event);
    void renderFrame();
    void cleanup() noexcept;

    XrInstance instance_{XR_NULL_HANDLE};
    XrSystemId systemId_{XR_NULL_SYSTEM_ID};
    XrSession session_{XR_NULL_HANDLE};
    XrSpace appSpace_{XR_NULL_HANDLE};
    XrActionSet actionSet_{XR_NULL_HANDLE};
    XrAction recenterAction_{XR_NULL_HANDLE};
    XrAction adjustScreenAction_{XR_NULL_HANDLE};
    XrAction toggleCurveAction_{XR_NULL_HANDLE};
    XrSessionState sessionState_{XR_SESSION_STATE_UNKNOWN};
    XrEnvironmentBlendMode environmentBlendMode_{XR_ENVIRONMENT_BLEND_MODE_OPAQUE};

    std::vector<XrViewConfigurationView> viewConfiguration_;
    std::vector<XrView> locatedViews_;
    std::vector<XrCompositionLayerProjectionView> projectionViews_;
    std::vector<ViewSwapchain> swapchains_;

    D3D11Renderer renderer_;
    DesktopCapture desktopCapture_;
    unity_transport::Mapping unityTransport_;
    UnitySharedTextureSource unityTextureSource_;
    MirrorWindow mirrorWindow_;
    std::chrono::steady_clock::time_point renderStart_{};
    std::chrono::steady_clock::time_point lastActionSync_{};

    bool sessionRunning_{false};
    bool exitRequested_{false};
    bool instanceLossPending_{false};
    bool mirrorEnabled_{false};
    bool virtualScreenAnchorValid_{false};
    bool recenterKeyWasDown_{false};
    bool unityFallbackWarningEmitted_{false};
    std::int64_t swapchainFormat_{0};
    XrPosef virtualScreenAnchor_{
        XrQuaternionf{0.0F, 0.0F, 0.0F, 1.0F},
        XrVector3f{0.0F, 0.0F, 0.0F}};
    OpenXrRunOptions runOptions_{};
    std::atomic_bool recenterRequested_{false};
    std::atomic_bool externalExitRequested_{false};
    std::uint64_t unityFrameId_{};
};

} // namespace uxrb
