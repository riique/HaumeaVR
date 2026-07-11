#include "uxrb/logger.hpp"
#include "uxrb/desktop_capture.hpp"
#include "uxrb/d3d11_renderer.hpp"
#include "uxrb/ipc_server.hpp"
#include "uxrb/openxr_app.hpp"
#include "uxrb/projection.hpp"

#include <Windows.h>

#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

void printUsage() {
    std::cout
        << "HaumeaVR - Milestone 1\n\n"
        << "Usage:\n"
        << "  uxrb_core.exe               Run the OpenXR stereo test scene\n"
        << "  uxrb_core.exe --diagnose    Inspect the active runtime and HMD\n"
        << "  uxrb_core.exe --self-test   Run tests that need no runtime/headset\n"
        << "  uxrb_core.exe --capture-self-test\n"
        << "                                Validate Desktop Duplication on the primary display\n"
        << "  uxrb_core.exe --virtual-screen --window-hwnd <handle> [options]\n"
        << "                                Capture a window into a tracked OpenXR panel\n"
        << "    --screen-width <meters>    0.5 to 10.0 (default 3.2)\n"
        << "    --screen-distance <meters> 0.5 to 10.0 (default 2.2)\n"
        << "    --screen-curvature <0..1>  Flat to curved (default 0.35)\n"
        << "  uxrb_core.exe --unity-stereo --unity-transport <name> [--unity-pid <pid>]\n"
        << "                                Receive opt-in Unity per-eye shared textures\n"
        << "  uxrb_core.exe --ipc-server <session-id>\n"
        << "                                Run the authenticated launcher control endpoint\n"
        << "  uxrb_core.exe --help        Show this help\n";
}

} // namespace

int wmain(const int argumentCount, wchar_t** arguments) {
    SetConsoleOutputCP(CP_UTF8);
    uxrb::Logger::instance().initialize();

    bool diagnose = false;
    bool selfTest = false;
    bool captureSelfTest = false;
    uxrb::OpenXrRunOptions runOptions;
    std::optional<std::wstring> ipcSession;
    for (int index = 1; index < argumentCount; ++index) {
        const std::wstring_view argument(arguments[index]);
        if (argument == L"--diagnose") {
            diagnose = true;
        } else if (argument == L"--self-test") {
            selfTest = true;
        } else if (argument == L"--capture-self-test") {
            captureSelfTest = true;
        } else if (argument == L"--virtual-screen") {
            runOptions.sceneMode = uxrb::OpenXrSceneMode::virtualScreen;
        } else if (argument == L"--unity-stereo") {
            runOptions.sceneMode = uxrb::OpenXrSceneMode::unityStereo;
        } else if (argument == L"--unity-transport") {
            if (index + 1 >= argumentCount) {
                std::wcerr << L"--unity-transport requires a Local\\UXRB-Unity- name.\n";
                return 2;
            }
            runOptions.unityTransportName = arguments[++index];
        } else if (argument == L"--unity-pid") {
            if (index + 1 >= argumentCount) {
                std::wcerr << L"--unity-pid requires a positive process identifier.\n";
                return 2;
            }
            wchar_t* end = nullptr;
            errno = 0;
            const unsigned long value = std::wcstoul(arguments[++index], &end, 10);
            if (errno == ERANGE || end == arguments[index] || *end != L'\0' || value == 0) {
                std::wcerr << L"--unity-pid is invalid.\n";
                return 2;
            }
            runOptions.unityProcessId = static_cast<std::uint32_t>(value);
        } else if (argument == L"--window-hwnd") {
            if (index + 1 >= argumentCount) {
                std::wcerr << L"--window-hwnd requires a decimal or 0x-prefixed handle.\n";
                return 2;
            }
            wchar_t* end = nullptr;
            errno = 0;
            const unsigned long long value = std::wcstoull(arguments[++index], &end, 0);
            if (errno == ERANGE || end == arguments[index] || *end != L'\0' || value == 0) {
                std::wcerr << L"--window-hwnd is invalid.\n";
                return 2;
            }
            runOptions.captureWindow = reinterpret_cast<HWND>(static_cast<std::uintptr_t>(value));
        } else if (argument == L"--screen-width"
            || argument == L"--screen-distance"
            || argument == L"--screen-curvature") {
            if (index + 1 >= argumentCount) {
                std::wcerr << argument << L" requires a numeric value.\n";
                return 2;
            }
            wchar_t* end = nullptr;
            errno = 0;
            const float value = std::wcstof(arguments[++index], &end);
            if (errno == ERANGE || end == arguments[index] || *end != L'\0' || !std::isfinite(value)) {
                std::wcerr << argument << L" is invalid.\n";
                return 2;
            }
            if (argument == L"--screen-width") runOptions.virtualScreen.widthMeters = value;
            if (argument == L"--screen-distance") runOptions.virtualScreen.distanceMeters = value;
            if (argument == L"--screen-curvature") runOptions.virtualScreen.curvature = value;
        } else if (argument == L"--ipc-server") {
            if (index + 1 >= argumentCount) {
                std::wcerr << L"--ipc-server requires a session id.\n";
                return 2;
            }
            ipcSession = arguments[++index];
        } else if (argument == L"--help" || argument == L"-h") {
            printUsage();
            return 0;
        } else {
            std::wcerr << L"Unknown argument: " << argument << L"\n";
            printUsage();
            return 2;
        }
    }

    const int selectedModeCount = static_cast<int>(diagnose)
        + static_cast<int>(selfTest)
        + static_cast<int>(captureSelfTest)
        + static_cast<int>(runOptions.sceneMode == uxrb::OpenXrSceneMode::virtualScreen)
        + static_cast<int>(runOptions.sceneMode == uxrb::OpenXrSceneMode::unityStereo)
        + static_cast<int>(ipcSession.has_value());
    if (selectedModeCount > 1) {
        std::cerr << "Select only one execution mode.\n";
        return 2;
    }

    try {
        if (ipcSession.has_value()) {
            return uxrb::runIpcServer(*ipcSession);
        }
        if (selfTest) {
            std::string failureReason;
            uxrb::Logger::instance().debug("Self-test: validating OpenXR projection math.");
            if (!uxrb::runProjectionSelfTest(failureReason)) {
                uxrb::Logger::instance().error("Self-test failed: " + failureReason);
                return 1;
            }
            uxrb::Logger::instance().debug("Self-test: validating D3D11 render pipelines.");
            if (!uxrb::runD3D11SelfTest(failureReason)) {
                uxrb::Logger::instance().error("D3D11 self-test failed: " + failureReason);
                return 1;
            }
            uxrb::Logger::instance().info("Runtime-independent self-test passed.");
            return 0;
        }
        if (captureSelfTest) {
            std::string failureReason;
            if (!uxrb::runDesktopCaptureSelfTest(failureReason)) {
                uxrb::Logger::instance().error("Desktop capture self-test failed: " + failureReason);
                return 1;
            }
            uxrb::Logger::instance().info("Desktop capture self-test passed with a GPU-resident texture.");
            return 0;
        }

        uxrb::OpenXrApp application;
        return diagnose ? application.diagnose() : application.run(runOptions);
    } catch (const std::exception& exception) {
        const std::string errorMessage = exception.what();
        uxrb::Logger::instance().error(errorMessage);
        if (diagnose) {
            if (errorMessage.find("XR_ERROR_FORM_FACTOR_UNAVAILABLE") != std::string::npos) {
                uxrb::Logger::instance().error(
                    "The runtime loaded, but no HMD is available. Connect/wake the headset and start its PC VR session.");
            } else if (errorMessage.find("XR_ERROR_RUNTIME_UNAVAILABLE") != std::string::npos) {
                uxrb::Logger::instance().error(
                    "Select an OpenXR runtime in its vendor application, then run diagnostics again.");
            } else {
                uxrb::Logger::instance().error(
                    "The runtime or headset did not complete diagnostics; inspect the preceding error and log file.");
            }
        } else {
            uxrb::Logger::instance().error(
                "Run --diagnose for runtime registration details. HaumeaVR did not change system settings.");
        }
        return 1;
    }
}
