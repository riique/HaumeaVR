#pragma once

#include <cstdint>

#if defined(_WIN32)
#if defined(UXRB_D3D11_INTERCEPT_EXPORTS)
#define UXRB_D3D11_HOOK_API __declspec(dllexport)
#else
#define UXRB_D3D11_HOOK_API __declspec(dllimport)
#endif
#define UXRB_D3D11_HOOK_CALL __stdcall
#else
#define UXRB_D3D11_HOOK_API
#define UXRB_D3D11_HOOK_CALL
#endif

namespace uxrb::intercept {

inline constexpr std::uint32_t kD3D11InterceptApiVersion = 1;

enum class InstallResult : std::uint32_t {
    installed = 0,
    alreadyInstalled = 1,
    optInRequired = 2,
    antiCheatDetected = 3,
    graphicsBootstrapFailed = 4,
    detourFailed = 5,
};

struct D3D11InterceptStatus final {
    std::uint32_t structSize{sizeof(D3D11InterceptStatus)};
    std::uint32_t apiVersion{kD3D11InterceptApiVersion};
    std::uint32_t installed{};
    std::uint32_t antiCheatDenied{};
    std::uint64_t presentCount{};
    std::uint32_t backBufferWidth{};
    std::uint32_t backBufferHeight{};
    std::uint32_t backBufferFormat{};
    std::uint32_t depthAvailable{};
    std::uint32_t depthWidth{};
    std::uint32_t depthHeight{};
    std::uint32_t depthFormat{};
    float depthCandidateScore{};
};

}  // namespace uxrb::intercept

extern "C" {

UXRB_D3D11_HOOK_API std::uint32_t UXRB_D3D11_HOOK_CALL UxrbD3D11Hook_GetApiVersion();
UXRB_D3D11_HOOK_API uxrb::intercept::InstallResult UXRB_D3D11_HOOK_CALL UxrbD3D11Hook_InstallForCurrentProcess();
UXRB_D3D11_HOOK_API std::int32_t UXRB_D3D11_HOOK_CALL UxrbD3D11Hook_Uninstall();
UXRB_D3D11_HOOK_API std::int32_t UXRB_D3D11_HOOK_CALL UxrbD3D11Hook_GetStatus(
    uxrb::intercept::D3D11InterceptStatus* status);
UXRB_D3D11_HOOK_API std::int32_t UXRB_D3D11_HOOK_CALL UxrbD3D11Hook_CopyLastError(
    wchar_t* destination,
    std::int32_t destinationCharacterCount);

}
