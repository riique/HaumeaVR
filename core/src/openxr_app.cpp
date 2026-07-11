#include "uxrb/openxr_app.hpp"

#include "uxrb/logger.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <initializer_list>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace uxrb {
namespace {

constexpr XrViewConfigurationType primaryViewConfiguration = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

std::string resultName(const XrInstance instance, const XrResult result) {
    if (instance != XR_NULL_HANDLE) {
        char buffer[XR_MAX_RESULT_STRING_SIZE]{};
        if (XR_SUCCEEDED(xrResultToString(instance, result, buffer))) {
            return buffer;
        }
    }

    switch (result) {
    case XR_ERROR_RUNTIME_UNAVAILABLE:
        return "XR_ERROR_RUNTIME_UNAVAILABLE";
    case XR_ERROR_FORM_FACTOR_UNAVAILABLE:
        return "XR_ERROR_FORM_FACTOR_UNAVAILABLE";
    case XR_ERROR_API_VERSION_UNSUPPORTED:
        return "XR_ERROR_API_VERSION_UNSUPPORTED";
    case XR_ERROR_EXTENSION_NOT_PRESENT:
        return "XR_ERROR_EXTENSION_NOT_PRESENT";
    case XR_ERROR_INSTANCE_LOST:
        return "XR_ERROR_INSTANCE_LOST";
    case XR_ERROR_INITIALIZATION_FAILED:
        return "XR_ERROR_INITIALIZATION_FAILED";
    case XR_ERROR_VALIDATION_FAILURE:
        return "XR_ERROR_VALIDATION_FAILURE";
    default:
        break;
    }
    return std::to_string(static_cast<std::int32_t>(result));
}

void checkXr(
    const XrInstance instance,
    const XrResult result,
    const std::string_view operation) {
    if (XR_FAILED(result)) {
        throw std::runtime_error(
            std::string(operation) + " failed: " + resultName(instance, result));
    }
}

std::string wideToUtf8(const std::wstring_view text) {
    if (text.empty()) {
        return {};
    }
    const int required = WideCharToMultiByte(
        CP_UTF8,
        0,
        text.data(),
        static_cast<int>(text.size()),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (required <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        text.data(),
        static_cast<int>(text.size()),
        result.data(),
        required,
        nullptr,
        nullptr);
    return result;
}

std::optional<std::wstring> registryString(
    const HKEY root,
    const wchar_t* keyPath,
    const wchar_t* valueName) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(root, keyPath, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        return std::nullopt;
    }

    DWORD type = 0;
    DWORD byteCount = 0;
    LONG result = RegQueryValueExW(key, valueName, nullptr, &type, nullptr, &byteCount);
    if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        RegCloseKey(key);
        return std::nullopt;
    }

    std::vector<wchar_t> buffer(byteCount / sizeof(wchar_t) + 1, L'\0');
    result = RegQueryValueExW(
        key,
        valueName,
        nullptr,
        &type,
        reinterpret_cast<BYTE*>(buffer.data()),
        &byteCount);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    }
    return std::wstring(buffer.data());
}

void logAvailableRuntimes(const HKEY root, const std::string_view hiveName) {
    constexpr wchar_t keyPath[] = L"SOFTWARE\\Khronos\\OpenXR\\1\\AvailableRuntimes";
    HKEY key = nullptr;
    if (RegOpenKeyExW(root, keyPath, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        return;
    }

    for (DWORD index = 0;; ++index) {
        std::array<wchar_t, 32768> name{};
        DWORD nameLength = static_cast<DWORD>(name.size());
        DWORD type = 0;
        DWORD value = 1;
        DWORD valueSize = sizeof(value);
        const LONG result = RegEnumValueW(
            key,
            index,
            name.data(),
            &nameLength,
            nullptr,
            &type,
            reinterpret_cast<BYTE*>(&value),
            &valueSize);
        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }
        if (result == ERROR_SUCCESS && type == REG_DWORD) {
            Logger::instance().info(
                std::string(hiveName) + " registered runtime ("
                + (value == 0 ? "enabled" : "disabled") + "): "
                + wideToUtf8(std::wstring_view(name.data(), nameLength)));
        }
    }

    RegCloseKey(key);
}

const char* sessionStateName(const XrSessionState state) noexcept {
    switch (state) {
    case XR_SESSION_STATE_UNKNOWN:
        return "UNKNOWN";
    case XR_SESSION_STATE_IDLE:
        return "IDLE";
    case XR_SESSION_STATE_READY:
        return "READY";
    case XR_SESSION_STATE_SYNCHRONIZED:
        return "SYNCHRONIZED";
    case XR_SESSION_STATE_VISIBLE:
        return "VISIBLE";
    case XR_SESSION_STATE_FOCUSED:
        return "FOCUSED";
    case XR_SESSION_STATE_STOPPING:
        return "STOPPING";
    case XR_SESSION_STATE_LOSS_PENDING:
        return "LOSS_PENDING";
    case XR_SESSION_STATE_EXITING:
        return "EXITING";
    default:
        return "UNRECOGNIZED";
    }
}

std::string luidString(const LUID& luid) {
    std::ostringstream stream;
    stream << std::hex << std::uppercase
           << static_cast<std::uint32_t>(luid.HighPart) << ':'
           << static_cast<std::uint32_t>(luid.LowPart);
    return stream.str();
}

} // namespace

OpenXrApp::~OpenXrApp() {
    cleanup();
}

int OpenXrApp::run(const OpenXrRunOptions& options) {
    if (options.sceneMode == OpenXrSceneMode::virtualScreen) {
        if (options.captureWindow == nullptr || !IsWindow(options.captureWindow)) {
            throw std::invalid_argument("Virtual Screen requires a valid --window-hwnd target.");
        }
        const auto& screen = options.virtualScreen;
        if (!std::isfinite(screen.widthMeters) || screen.widthMeters < 0.5F || screen.widthMeters > 10.0F
            || !std::isfinite(screen.distanceMeters) || screen.distanceMeters < 0.5F || screen.distanceMeters > 10.0F
            || !std::isfinite(screen.curvature) || screen.curvature < 0.0F || screen.curvature > 1.0F) {
            throw std::invalid_argument("Virtual Screen width, distance, or curvature is outside its supported range.");
        }
    }
    if (options.sceneMode == OpenXrSceneMode::unityStereo) {
        if (!unity_transport::Mapping::validName(options.unityTransportName)) {
            throw std::invalid_argument(
                "Unity Stereo requires --unity-transport Local\\\\UXRB-Unity-<random-id>.");
        }
        std::string transportError;
        if (!unityTransport_.createOwner(options.unityTransportName, transportError)) {
            throw std::runtime_error("Unity transport creation failed: " + transportError);
        }
    }
    runOptions_ = options;
    recenterRequested_.store(options.sceneMode == OpenXrSceneMode::virtualScreen);
    virtualScreenAnchorValid_ = false;
    recenterKeyWasDown_ = false;
    unityFallbackWarningEmitted_ = false;
    unityFrameId_ = 0;

    logRuntimeRegistration();
    createInstance();
    selectSystem();
    const XrGraphicsRequirementsD3D11KHR requirements = graphicsRequirements();
    logViewConfiguration();

    mirrorWindow_.create(960, 540);
    renderer_.initializeDevice(requirements.adapterLuid, requirements.minFeatureLevel);
    renderer_.createPipeline();
    if (runOptions_.sceneMode == OpenXrSceneMode::virtualScreen) {
        desktopCapture_.initialize(renderer_.device(), renderer_.context(), runOptions_.captureWindow);
        Logger::instance().info(
            "Virtual Screen mode enabled; press F8 to recenter the panel in front of the headset.");
    } else if (runOptions_.sceneMode == OpenXrSceneMode::unityStereo) {
        unityTextureSource_.initialize(renderer_.device(), &unityTransport_, runOptions_.unityProcessId);
        Logger::instance().info(
            "Unity Stereo transport ready; waiting for the opt-in Unity D3D11 bridge producer.");
    }
    createSession(requirements);
    createActions();
    suggestActionBindings();
    attachActionSet();
    createReferenceSpace();
    chooseEnvironmentBlendMode();
    createSwapchains();

    renderStart_ = std::chrono::steady_clock::now();
    Logger::instance().info("OpenXR frame loop initialized; waiting for the session to become READY.");

    while (!exitRequested_ && !instanceLossPending_) {
        mirrorWindow_.pumpMessages();
        pollEvents();

        if (runOptions_.sceneMode == OpenXrSceneMode::virtualScreen) {
            const bool recenterKeyDown = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
            if (recenterKeyDown && !recenterKeyWasDown_) requestRecenter();
            recenterKeyWasDown_ = recenterKeyDown;
        }

        if (externalExitRequested_.exchange(false)) {
            if (sessionRunning_) {
                const XrResult requestResult = xrRequestExitSession(session_);
                if (XR_FAILED(requestResult)) {
                    Logger::instance().warning(
                        "xrRequestExitSession failed for an external stop request: "
                        + resultName(instance_, requestResult));
                }
            }
            // An IPC stop is authoritative. Do not make SessionHost::stop wait
            // indefinitely for a runtime that never emits EXITING after the
            // polite request above; normal teardown destroys the session.
            exitRequested_ = true;
        }

        if (mirrorWindow_.closed()) {
            renderer_.shutdownMirror();
            mirrorEnabled_ = false;
            if (sessionRunning_) {
                const XrResult requestResult = xrRequestExitSession(session_);
                if (XR_FAILED(requestResult)) {
                    Logger::instance().warning(
                        "xrRequestExitSession failed while closing: "
                        + resultName(instance_, requestResult));
                }
            }
            exitRequested_ = true;
            continue;
        }

        if (sessionRunning_) {
            syncActions();
            renderFrame();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    Logger::instance().info("OpenXR frame loop stopped cleanly.");
    return 0;
}

int OpenXrApp::diagnose() {
    logRuntimeRegistration();
    createInstance();
    selectSystem();
    static_cast<void>(graphicsRequirements());
    logViewConfiguration();
    chooseEnvironmentBlendMode();
    Logger::instance().info("OpenXR diagnostics completed successfully.");
    return 0;
}

void OpenXrApp::requestRecenter() noexcept {
    recenterRequested_.store(true);
}

void OpenXrApp::requestExit() noexcept {
    externalExitRequested_.store(true);
}

void OpenXrApp::logRuntimeRegistration() const {
    constexpr wchar_t keyPath[] = L"SOFTWARE\\Khronos\\OpenXR\\1";
    std::array<wchar_t, 32768> processOverride{};
    const DWORD overrideLength = GetEnvironmentVariableW(
        L"XR_RUNTIME_JSON",
        processOverride.data(),
        static_cast<DWORD>(processOverride.size()));
    if (overrideLength > 0 && overrideLength < processOverride.size()) {
        Logger::instance().info(
            "Process-local XR_RUNTIME_JSON override: "
            + wideToUtf8(std::wstring_view(processOverride.data(), overrideLength)));
    }

    const auto machineRuntime = registryString(HKEY_LOCAL_MACHINE, keyPath, L"ActiveRuntime");
    const auto userRuntime = registryString(HKEY_CURRENT_USER, keyPath, L"ActiveRuntime");

    if (userRuntime.has_value()) {
        Logger::instance().info("HKCU active OpenXR runtime: " + wideToUtf8(*userRuntime));
    } else if (machineRuntime.has_value()) {
        Logger::instance().info("HKLM active OpenXR runtime: " + wideToUtf8(*machineRuntime));
    } else {
        Logger::instance().warning(
            "No ActiveRuntime value is registered for the 64-bit OpenXR loader.");
    }

    logAvailableRuntimes(HKEY_CURRENT_USER, "HKCU");
    logAvailableRuntimes(HKEY_LOCAL_MACHINE, "HKLM");
}

void OpenXrApp::createInstance() {
    std::uint32_t extensionCount = 0;
    checkXr(
        XR_NULL_HANDLE,
        xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr),
        "xrEnumerateInstanceExtensionProperties(count)");

    std::vector<XrExtensionProperties> extensions(extensionCount);
    for (auto& extension : extensions) {
        extension.type = XR_TYPE_EXTENSION_PROPERTIES;
        extension.next = nullptr;
    }
    checkXr(
        XR_NULL_HANDLE,
        xrEnumerateInstanceExtensionProperties(
            nullptr,
            extensionCount,
            &extensionCount,
            extensions.data()),
        "xrEnumerateInstanceExtensionProperties(values)");

    const bool hasD3D11 = std::ranges::any_of(extensions, [](const XrExtensionProperties& extension) {
        return std::strcmp(extension.extensionName, XR_KHR_D3D11_ENABLE_EXTENSION_NAME) == 0;
    });
    if (!hasD3D11) {
        throw std::runtime_error("The active OpenXR runtime does not expose XR_KHR_D3D11_enable.");
    }

    constexpr std::array enabledExtensions{XR_KHR_D3D11_ENABLE_EXTENSION_NAME};
    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    strncpy_s(
        createInfo.applicationInfo.applicationName,
        "HaumeaVR",
        _TRUNCATE);
    createInfo.applicationInfo.applicationVersion = 1;
    strncpy_s(
        createInfo.applicationInfo.engineName,
        "HaumeaVR Native",
        _TRUNCATE);
    createInfo.applicationInfo.engineVersion = 1;
    // Milestone 0 only consumes OpenXR 1.0 core functionality. Advertising
    // 1.0 keeps the bridge usable with installed runtimes that have not yet
    // moved to OpenXR 1.1 while retaining the newer SDK headers and loader.
    createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
    createInfo.enabledExtensionCount = static_cast<std::uint32_t>(enabledExtensions.size());
    createInfo.enabledExtensionNames = enabledExtensions.data();

    checkXr(XR_NULL_HANDLE, xrCreateInstance(&createInfo, &instance_), "xrCreateInstance");

    XrInstanceProperties properties{XR_TYPE_INSTANCE_PROPERTIES};
    checkXr(instance_, xrGetInstanceProperties(instance_, &properties), "xrGetInstanceProperties");
    std::ostringstream message;
    message << "OpenXR runtime: " << properties.runtimeName
            << " " << XR_VERSION_MAJOR(properties.runtimeVersion)
            << '.' << XR_VERSION_MINOR(properties.runtimeVersion)
            << '.' << XR_VERSION_PATCH(properties.runtimeVersion);
    Logger::instance().info(message.str());
}

void OpenXrApp::selectSystem() {
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    checkXr(instance_, xrGetSystem(instance_, &getInfo, &systemId_), "xrGetSystem(HMD)");

    XrSystemProperties properties{XR_TYPE_SYSTEM_PROPERTIES};
    checkXr(instance_, xrGetSystemProperties(instance_, systemId_, &properties), "xrGetSystemProperties");

    std::ostringstream message;
    message << "OpenXR system: " << properties.systemName
            << ", vendor " << properties.vendorId
            << ", max layer count " << properties.graphicsProperties.maxLayerCount
            << ", orientation tracking "
            << (properties.trackingProperties.orientationTracking == XR_TRUE ? "yes" : "no")
            << ", position tracking "
            << (properties.trackingProperties.positionTracking == XR_TRUE ? "yes" : "no");
    Logger::instance().info(message.str());
}

XrGraphicsRequirementsD3D11KHR OpenXrApp::graphicsRequirements() const {
    PFN_xrVoidFunction rawFunction = nullptr;
    checkXr(
        instance_,
        xrGetInstanceProcAddr(instance_, "xrGetD3D11GraphicsRequirementsKHR", &rawFunction),
        "xrGetInstanceProcAddr(xrGetD3D11GraphicsRequirementsKHR)");
    if (rawFunction == nullptr) {
        throw std::runtime_error("The OpenXR loader returned a null D3D11 requirements function.");
    }

    const auto function = reinterpret_cast<PFN_xrGetD3D11GraphicsRequirementsKHR>(rawFunction);
    XrGraphicsRequirementsD3D11KHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
    checkXr(
        instance_,
        function(instance_, systemId_, &requirements),
        "xrGetD3D11GraphicsRequirementsKHR");

    std::ostringstream message;
    message << "OpenXR D3D11 requirement: adapter LUID " << luidString(requirements.adapterLuid)
            << ", minimum feature level 0x" << std::hex << std::uppercase
            << static_cast<unsigned int>(requirements.minFeatureLevel);
    Logger::instance().info(message.str());
    return requirements;
}

void OpenXrApp::logViewConfiguration() const {
    std::uint32_t viewCount = 0;
    checkXr(
        instance_,
        xrEnumerateViewConfigurationViews(
            instance_,
            systemId_,
            primaryViewConfiguration,
            0,
            &viewCount,
            nullptr),
        "xrEnumerateViewConfigurationViews(count)");

    std::vector<XrViewConfigurationView> views(viewCount);
    for (auto& view : views) {
        view.type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
        view.next = nullptr;
    }
    checkXr(
        instance_,
        xrEnumerateViewConfigurationViews(
            instance_,
            systemId_,
            primaryViewConfiguration,
            viewCount,
            &viewCount,
            views.data()),
        "xrEnumerateViewConfigurationViews(values)");

    for (std::uint32_t index = 0; index < viewCount; ++index) {
        const auto& view = views[index];
        std::ostringstream message;
        message << "View " << index << ": recommended "
                << view.recommendedImageRectWidth << 'x' << view.recommendedImageRectHeight
                << ", max " << view.maxImageRectWidth << 'x' << view.maxImageRectHeight
                << ", recommended samples " << view.recommendedSwapchainSampleCount;
        Logger::instance().info(message.str());
    }
}

void OpenXrApp::createSession(const XrGraphicsRequirementsD3D11KHR& requirements) {
    if (renderer_.featureLevel() < requirements.minFeatureLevel) {
        throw std::runtime_error("The created D3D11 device is below the runtime's minimum feature level.");
    }

    XrGraphicsBindingD3D11KHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
    graphicsBinding.device = renderer_.device();

    XrSessionCreateInfo createInfo{XR_TYPE_SESSION_CREATE_INFO};
    createInfo.next = &graphicsBinding;
    createInfo.systemId = systemId_;
    checkXr(instance_, xrCreateSession(instance_, &createInfo, &session_), "xrCreateSession");
}

void OpenXrApp::createActions() {
    if (runOptions_.sceneMode != OpenXrSceneMode::virtualScreen) return;

    XrActionSetCreateInfo setInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
    strncpy_s(setInfo.actionSetName, "uxrb_controls", _TRUNCATE);
    strncpy_s(setInfo.localizedActionSetName, "HaumeaVR controls", _TRUNCATE);
    setInfo.priority = 0;
    checkXr(instance_, xrCreateActionSet(instance_, &setInfo, &actionSet_), "xrCreateActionSet");

    const auto createAction = [this](
                                  const char* name,
                                  const char* localizedName,
                                  const XrActionType type,
                                  XrAction& action) {
        XrActionCreateInfo info{XR_TYPE_ACTION_CREATE_INFO};
        strncpy_s(info.actionName, name, _TRUNCATE);
        strncpy_s(info.localizedActionName, localizedName, _TRUNCATE);
        info.actionType = type;
        checkXr(instance_, xrCreateAction(actionSet_, &info, &action), "xrCreateAction");
    };
    createAction("recenter", "Recenter screen", XR_ACTION_TYPE_BOOLEAN_INPUT, recenterAction_);
    createAction("adjust_screen", "Adjust screen", XR_ACTION_TYPE_VECTOR2F_INPUT, adjustScreenAction_);
    createAction("toggle_curve", "Toggle flat or curved screen", XR_ACTION_TYPE_BOOLEAN_INPUT, toggleCurveAction_);
}

void OpenXrApp::suggestActionBindings() {
    if (actionSet_ == XR_NULL_HANDLE) return;

    const auto path = [this](const char* text) {
        XrPath result = XR_NULL_PATH;
        checkXr(instance_, xrStringToPath(instance_, text, &result), "xrStringToPath(action binding)");
        return result;
    };
    const auto suggest = [this, &path](
                             const char* profile,
                             const std::initializer_list<std::pair<XrAction, const char*>> bindings) {
        std::vector<XrActionSuggestedBinding> suggested;
        suggested.reserve(bindings.size());
        for (const auto& [action, binding] : bindings) {
            suggested.push_back(XrActionSuggestedBinding{action, path(binding)});
        }
        XrInteractionProfileSuggestedBinding info{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        info.interactionProfile = path(profile);
        info.suggestedBindings = suggested.data();
        info.countSuggestedBindings = static_cast<std::uint32_t>(suggested.size());
        const XrResult result = xrSuggestInteractionProfileBindings(instance_, &info);
        if (XR_FAILED(result)) {
            Logger::instance().debug(
                std::string("Interaction profile binding was not accepted for ") + profile
                + ": " + resultName(instance_, result));
        }
    };

    suggest(
        "/interaction_profiles/oculus/touch_controller",
        {
            {recenterAction_, "/user/hand/right/input/thumbstick/click"},
            {adjustScreenAction_, "/user/hand/left/input/thumbstick"},
            {toggleCurveAction_, "/user/hand/left/input/thumbstick/click"},
        });
    suggest(
        "/interaction_profiles/valve/index_controller",
        {
            {recenterAction_, "/user/hand/right/input/thumbstick/click"},
            {adjustScreenAction_, "/user/hand/left/input/thumbstick"},
            {toggleCurveAction_, "/user/hand/left/input/thumbstick/click"},
        });
    suggest(
        "/interaction_profiles/microsoft/motion_controller",
        {
            {recenterAction_, "/user/hand/right/input/thumbstick/click"},
            {adjustScreenAction_, "/user/hand/left/input/thumbstick"},
            {toggleCurveAction_, "/user/hand/left/input/thumbstick/click"},
        });
    suggest(
        "/interaction_profiles/khr/simple_controller",
        {
            {recenterAction_, "/user/hand/right/input/select/click"},
            {toggleCurveAction_, "/user/hand/left/input/menu/click"},
        });
}

void OpenXrApp::attachActionSet() {
    if (actionSet_ == XR_NULL_HANDLE) return;
    XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &actionSet_;
    checkXr(instance_, xrAttachSessionActionSets(session_, &attachInfo), "xrAttachSessionActionSets");
    lastActionSync_ = std::chrono::steady_clock::now();
}

void OpenXrApp::syncActions() {
    if (actionSet_ == XR_NULL_HANDLE) return;
    const XrActiveActionSet activeSet{actionSet_, XR_NULL_PATH};
    XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
    syncInfo.countActiveActionSets = 1;
    syncInfo.activeActionSets = &activeSet;
    const XrResult syncResult = xrSyncActions(session_, &syncInfo);
    if (syncResult == XR_SESSION_NOT_FOCUSED) return;
    checkXr(instance_, syncResult, "xrSyncActions");

    XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
    getInfo.action = recenterAction_;
    XrActionStateBoolean recenterState{XR_TYPE_ACTION_STATE_BOOLEAN};
    checkXr(instance_, xrGetActionStateBoolean(session_, &getInfo, &recenterState), "xrGetActionStateBoolean(recenter)");
    if (recenterState.isActive == XR_TRUE
        && recenterState.changedSinceLastSync == XR_TRUE
        && recenterState.currentState == XR_TRUE) {
        requestRecenter();
    }

    getInfo.action = toggleCurveAction_;
    XrActionStateBoolean curveState{XR_TYPE_ACTION_STATE_BOOLEAN};
    checkXr(instance_, xrGetActionStateBoolean(session_, &getInfo, &curveState), "xrGetActionStateBoolean(toggle curve)");
    if (curveState.isActive == XR_TRUE
        && curveState.changedSinceLastSync == XR_TRUE
        && curveState.currentState == XR_TRUE) {
        runOptions_.virtualScreen.curvature =
            runOptions_.virtualScreen.curvature > 0.01F ? 0.0F : 0.35F;
        Logger::instance().info(
            runOptions_.virtualScreen.curvature == 0.0F
                ? "Virtual Screen changed to flat."
                : "Virtual Screen changed to curved.");
    }

    getInfo.action = adjustScreenAction_;
    XrActionStateVector2f adjustState{XR_TYPE_ACTION_STATE_VECTOR2F};
    checkXr(instance_, xrGetActionStateVector2f(session_, &getInfo, &adjustState), "xrGetActionStateVector2f(adjust screen)");
    const auto now = std::chrono::steady_clock::now();
    const float deltaSeconds = std::clamp(
        std::chrono::duration<float>(now - lastActionSync_).count(),
        0.0F,
        0.1F);
    lastActionSync_ = now;
    if (adjustState.isActive == XR_TRUE) {
        const float horizontal = std::abs(adjustState.currentState.x) >= 0.15F
            ? adjustState.currentState.x
            : 0.0F;
        const float vertical = std::abs(adjustState.currentState.y) >= 0.15F
            ? adjustState.currentState.y
            : 0.0F;
        runOptions_.virtualScreen.widthMeters = std::clamp(
            runOptions_.virtualScreen.widthMeters + horizontal * deltaSeconds * 1.25F,
            0.5F,
            10.0F);
        runOptions_.virtualScreen.distanceMeters = std::clamp(
            runOptions_.virtualScreen.distanceMeters - vertical * deltaSeconds * 1.0F,
            0.5F,
            10.0F);
    }
}

void OpenXrApp::createReferenceSpace() {
    std::uint32_t spaceCount = 0;
    checkXr(
        instance_,
        xrEnumerateReferenceSpaces(session_, 0, &spaceCount, nullptr),
        "xrEnumerateReferenceSpaces(count)");
    std::vector<XrReferenceSpaceType> spaces(spaceCount);
    checkXr(
        instance_,
        xrEnumerateReferenceSpaces(session_, spaceCount, &spaceCount, spaces.data()),
        "xrEnumerateReferenceSpaces(values)");

    XrReferenceSpaceType selected = XR_REFERENCE_SPACE_TYPE_LOCAL;
    const bool localSpace = runOptions_.sceneMode != OpenXrSceneMode::diagnosticCube;
    if (!localSpace && std::ranges::find(spaces, XR_REFERENCE_SPACE_TYPE_STAGE) != spaces.end()) {
        selected = XR_REFERENCE_SPACE_TYPE_STAGE;
    } else if (std::ranges::find(spaces, XR_REFERENCE_SPACE_TYPE_LOCAL) == spaces.end()) {
        throw std::runtime_error("The runtime exposes neither STAGE nor LOCAL reference space.");
    }

    XrReferenceSpaceCreateInfo createInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    createInfo.referenceSpaceType = selected;
    createInfo.poseInReferenceSpace.orientation.w = 1.0F;
    checkXr(instance_, xrCreateReferenceSpace(session_, &createInfo, &appSpace_), "xrCreateReferenceSpace");
    Logger::instance().info(
        selected == XR_REFERENCE_SPACE_TYPE_STAGE
            ? "Using STAGE reference space."
            : "Using LOCAL reference space.");
}

void OpenXrApp::chooseEnvironmentBlendMode() {
    std::uint32_t modeCount = 0;
    checkXr(
        instance_,
        xrEnumerateEnvironmentBlendModes(
            instance_,
            systemId_,
            primaryViewConfiguration,
            0,
            &modeCount,
            nullptr),
        "xrEnumerateEnvironmentBlendModes(count)");
    std::vector<XrEnvironmentBlendMode> modes(modeCount);
    checkXr(
        instance_,
        xrEnumerateEnvironmentBlendModes(
            instance_,
            systemId_,
            primaryViewConfiguration,
            modeCount,
            &modeCount,
            modes.data()),
        "xrEnumerateEnvironmentBlendModes(values)");
    if (modes.empty()) {
        throw std::runtime_error("The runtime returned no environment blend mode.");
    }

    const auto opaque = std::ranges::find(modes, XR_ENVIRONMENT_BLEND_MODE_OPAQUE);
    environmentBlendMode_ = opaque != modes.end() ? *opaque : modes.front();
    Logger::instance().info(
        "Environment blend mode: " + std::to_string(static_cast<int>(environmentBlendMode_)));
}

void OpenXrApp::createSwapchains() {
    std::uint32_t viewCount = 0;
    checkXr(
        instance_,
        xrEnumerateViewConfigurationViews(
            instance_, systemId_, primaryViewConfiguration, 0, &viewCount, nullptr),
        "xrEnumerateViewConfigurationViews(count)");
    if (viewCount != 2) {
        throw std::runtime_error(
            "Milestone 0 requires a two-view PRIMARY_STEREO configuration; runtime returned "
            + std::to_string(viewCount) + " views.");
    }

    viewConfiguration_.resize(viewCount);
    for (auto& view : viewConfiguration_) {
        view.type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
        view.next = nullptr;
    }
    checkXr(
        instance_,
        xrEnumerateViewConfigurationViews(
            instance_,
            systemId_,
            primaryViewConfiguration,
            viewCount,
            &viewCount,
            viewConfiguration_.data()),
        "xrEnumerateViewConfigurationViews(values)");

    std::uint32_t formatCount = 0;
    checkXr(
        instance_,
        xrEnumerateSwapchainFormats(session_, 0, &formatCount, nullptr),
        "xrEnumerateSwapchainFormats(count)");
    std::vector<std::int64_t> formats(formatCount);
    checkXr(
        instance_,
        xrEnumerateSwapchainFormats(session_, formatCount, &formatCount, formats.data()),
        "xrEnumerateSwapchainFormats(values)");
    if (formats.empty()) {
        throw std::runtime_error("The OpenXR runtime returned no D3D11 swapchain format.");
    }

    constexpr std::array<std::int64_t, 4> defaultPreferredFormats{
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM,
    };
    constexpr std::array<std::int64_t, 4> unityPreferredFormats{
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
    };
    const auto& preferredFormats = runOptions_.sceneMode == OpenXrSceneMode::unityStereo
        ? unityPreferredFormats
        : defaultPreferredFormats;
    swapchainFormat_ = formats.front();
    for (const std::int64_t preferred : preferredFormats) {
        if (std::ranges::find(formats, preferred) != formats.end()) {
            swapchainFormat_ = preferred;
            break;
        }
    }
    Logger::instance().info("Selected DXGI swapchain format: " + std::to_string(swapchainFormat_));

    swapchains_.resize(viewCount);
    locatedViews_.resize(viewCount);
    projectionViews_.resize(viewCount);
    for (auto& view : locatedViews_) {
        view.type = XR_TYPE_VIEW;
        view.next = nullptr;
    }
    for (auto& view : projectionViews_) {
        view.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        view.next = nullptr;
    }

    std::uint32_t unityWidth = 0;
    std::uint32_t unityHeight = 0;
    if (runOptions_.sceneMode == OpenXrSceneMode::unityStereo) {
        for (const auto& configuration : viewConfiguration_) {
            unityWidth = std::max(unityWidth, configuration.recommendedImageRectWidth);
            unityHeight = std::max(unityHeight, configuration.recommendedImageRectHeight);
        }
    }

    bool transferSourceSupported = true;
    for (std::uint32_t index = 0; index < viewCount; ++index) {
        auto& swapchain = swapchains_[index];
        const auto& configuration = viewConfiguration_[index];
        swapchain.width = unityWidth != 0 ? unityWidth : configuration.recommendedImageRectWidth;
        swapchain.height = unityHeight != 0 ? unityHeight : configuration.recommendedImageRectHeight;

        XrSwapchainCreateInfo createInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT
            | XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT;
        createInfo.format = swapchainFormat_;
        createInfo.sampleCount = 1;
        createInfo.width = swapchain.width;
        createInfo.height = swapchain.height;
        createInfo.faceCount = 1;
        createInfo.arraySize = 1;
        createInfo.mipCount = 1;

        XrResult result = xrCreateSwapchain(session_, &createInfo, &swapchain.handle);
        if (XR_FAILED(result)) {
            Logger::instance().warning(
                "Swapchain creation with TRANSFER_SRC failed for view " + std::to_string(index)
                + "; retrying without desktop mirroring for this runtime.");
            transferSourceSupported = false;
            createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
            checkXr(
                instance_,
                xrCreateSwapchain(session_, &createInfo, &swapchain.handle),
                "xrCreateSwapchain(color-only)");
        }

        std::uint32_t imageCount = 0;
        checkXr(
            instance_,
            xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr),
            "xrEnumerateSwapchainImages(count)");
        swapchain.images.resize(imageCount);
        for (auto& image : swapchain.images) {
            image.type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
            image.next = nullptr;
            image.texture = nullptr;
        }
        checkXr(
            instance_,
            xrEnumerateSwapchainImages(
                swapchain.handle,
                imageCount,
                &imageCount,
                reinterpret_cast<XrSwapchainImageBaseHeader*>(swapchain.images.data())),
            "xrEnumerateSwapchainImages(values)");

        swapchain.renderTargets.reserve(imageCount);
        for (const auto& image : swapchain.images) {
            swapchain.renderTargets.push_back(renderer_.createEyeRenderTarget(
                image.texture,
                static_cast<DXGI_FORMAT>(swapchainFormat_)));
        }
    }

    mirrorEnabled_ = transferSourceSupported;
    if (mirrorEnabled_) {
        try {
            renderer_.createMirrorSwapchain(
                mirrorWindow_.handle(),
                swapchains_.front().width,
                swapchains_.front().height,
                static_cast<DXGI_FORMAT>(swapchainFormat_));
        } catch (const std::exception& exception) {
            mirrorEnabled_ = false;
            Logger::instance().warning(
                std::string("Desktop mirror was disabled: ") + exception.what());
        }
    }
}

void OpenXrApp::pollEvents() {
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    for (;;) {
        const XrResult result = xrPollEvent(instance_, &event);
        if (result == XR_EVENT_UNAVAILABLE) {
            return;
        }
        checkXr(instance_, result, "xrPollEvent");

        switch (event.type) {
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
            handleSessionStateChanged(
                *reinterpret_cast<const XrEventDataSessionStateChanged*>(&event));
            break;
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            Logger::instance().warning("OpenXR runtime reported instance loss pending.");
            instanceLossPending_ = true;
            break;
        case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
            const auto& lost = *reinterpret_cast<const XrEventDataEventsLost*>(&event);
            Logger::instance().warning(
                "OpenXR reported " + std::to_string(lost.lostEventCount) + " lost events.");
            break;
        }
        default:
            Logger::instance().debug(
                "Ignoring OpenXR event type " + std::to_string(static_cast<int>(event.type)) + '.');
            break;
        }

        event = XrEventDataBuffer{XR_TYPE_EVENT_DATA_BUFFER};
    }
}

void OpenXrApp::handleSessionStateChanged(const XrEventDataSessionStateChanged& event) {
    sessionState_ = event.state;
    Logger::instance().info(std::string("OpenXR session state: ") + sessionStateName(sessionState_));

    switch (sessionState_) {
    case XR_SESSION_STATE_READY: {
        XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
        beginInfo.primaryViewConfigurationType = primaryViewConfiguration;
        checkXr(instance_, xrBeginSession(session_, &beginInfo), "xrBeginSession");
        sessionRunning_ = true;
        break;
    }
    case XR_SESSION_STATE_STOPPING:
        if (sessionRunning_) {
            checkXr(instance_, xrEndSession(session_), "xrEndSession");
            sessionRunning_ = false;
        }
        break;
    case XR_SESSION_STATE_EXITING:
        exitRequested_ = true;
        break;
    case XR_SESSION_STATE_LOSS_PENDING:
        instanceLossPending_ = true;
        break;
    default:
        break;
    }
}

void OpenXrApp::renderFrame() {
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    checkXr(instance_, xrWaitFrame(session_, &waitInfo, &frameState), "xrWaitFrame");

    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    checkXr(instance_, xrBeginFrame(session_, &beginInfo), "xrBeginFrame");
    bool frameOpen = true;

    try {
        std::uint32_t locatedCount = 0;
        XrViewState viewState{XR_TYPE_VIEW_STATE};
        XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};
        locateInfo.viewConfigurationType = primaryViewConfiguration;
        locateInfo.displayTime = frameState.predictedDisplayTime;
        locateInfo.space = appSpace_;
        checkXr(
            instance_,
            xrLocateViews(
                session_,
                &locateInfo,
                &viewState,
                static_cast<std::uint32_t>(locatedViews_.size()),
                &locatedCount,
                locatedViews_.data()),
            "xrLocateViews");

        const XrViewStateFlags requiredTracking = XR_VIEW_STATE_ORIENTATION_VALID_BIT
            | XR_VIEW_STATE_POSITION_VALID_BIT;
        const bool trackingValid = (viewState.viewStateFlags & requiredTracking) == requiredTracking;
        const bool renderLayer = frameState.shouldRender == XR_TRUE
            && trackingValid
            && locatedCount == locatedViews_.size();

        if (renderLayer) {
            const float elapsedSeconds = std::chrono::duration<float>(
                std::chrono::steady_clock::now() - renderStart_).count();

            if (runOptions_.sceneMode == OpenXrSceneMode::virtualScreen) {
                static_cast<void>(desktopCapture_.captureFrame(0));
                if (!virtualScreenAnchorValid_ || recenterRequested_.exchange(false)) {
                    virtualScreenAnchor_.orientation = locatedViews_.front().pose.orientation;
                    XrVector3f center{};
                    for (const auto& view : locatedViews_) {
                        center.x += view.pose.position.x;
                        center.y += view.pose.position.y;
                        center.z += view.pose.position.z;
                    }
                    const float inverseCount = 1.0F / static_cast<float>(locatedViews_.size());
                    virtualScreenAnchor_.position = XrVector3f{
                        center.x * inverseCount,
                        center.y * inverseCount,
                        center.z * inverseCount,
                    };
                    virtualScreenAnchorValid_ = true;
                    Logger::instance().info("Virtual Screen recentered from the current predicted headset pose.");
                }
            } else if (runOptions_.sceneMode == OpenXrSceneMode::unityStereo) {
                unity_transport::StereoFrame transportFrame{};
                transportFrame.frameId = ++unityFrameId_;
                transportFrame.predictedDisplayTimeNanoseconds = frameState.predictedDisplayTime;
                transportFrame.recommendedWidth = swapchains_.front().width;
                transportFrame.recommendedHeight = swapchains_.front().height;
                for (std::size_t eye = 0; eye < locatedViews_.size(); ++eye) {
                    const auto& source = locatedViews_[eye];
                    auto& destination = transportFrame.eyes[eye];
                    destination.position[0] = source.pose.position.x;
                    destination.position[1] = source.pose.position.y;
                    destination.position[2] = source.pose.position.z;
                    destination.orientation[0] = source.pose.orientation.x;
                    destination.orientation[1] = source.pose.orientation.y;
                    destination.orientation[2] = source.pose.orientation.z;
                    destination.orientation[3] = source.pose.orientation.w;
                    destination.fov[0] = source.fov.angleLeft;
                    destination.fov[1] = source.fov.angleRight;
                    destination.fov[2] = source.fov.angleUp;
                    destination.fov[3] = source.fov.angleDown;
                }
                static_cast<void>(unityTransport_.publishFrame(transportFrame));
                static_cast<void>(unityTextureSource_.refresh());
            }

            for (std::uint32_t index = 0; index < locatedCount; ++index) {
                auto& swapchain = swapchains_[index];
                std::uint32_t imageIndex = 0;
                XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
                checkXr(
                    instance_,
                    xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &imageIndex),
                    "xrAcquireSwapchainImage");
                bool imageAcquired = true;

                try {
                    XrSwapchainImageWaitInfo imageWaitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
                    imageWaitInfo.timeout = XR_INFINITE_DURATION;
                    checkXr(
                        instance_,
                        xrWaitSwapchainImage(swapchain.handle, &imageWaitInfo),
                        "xrWaitSwapchainImage");

                    if (runOptions_.sceneMode == OpenXrSceneMode::virtualScreen) {
                        const auto& captureInfo = desktopCapture_.info();
                        const float aspectRatio = static_cast<float>(captureInfo.width)
                            / static_cast<float>(captureInfo.height);
                        renderer_.renderVirtualScreenEye(
                            swapchain.renderTargets.at(imageIndex),
                            locatedViews_[index].pose,
                            locatedViews_[index].fov,
                            virtualScreenAnchor_,
                            desktopCapture_.shaderResourceView(),
                            aspectRatio,
                            runOptions_.virtualScreen);
                    } else if (runOptions_.sceneMode == OpenXrSceneMode::unityStereo) {
                        if (!unityTextureSource_.copyEye(
                                renderer_.context(),
                                swapchain.renderTargets.at(imageIndex),
                                index)) {
                            if (!unityFallbackWarningEmitted_) {
                                Logger::instance().warning(
                                    "Unity eye textures are not ready; rendering the diagnostic cube until the producer catches up.");
                                unityFallbackWarningEmitted_ = true;
                            }
                            renderer_.renderEye(
                                swapchain.renderTargets.at(imageIndex),
                                locatedViews_[index].pose,
                                locatedViews_[index].fov,
                                elapsedSeconds,
                                index);
                        } else if (unityFallbackWarningEmitted_) {
                            Logger::instance().info("Unity Stereo eye textures are now flowing into OpenXR.");
                            unityFallbackWarningEmitted_ = false;
                        }
                    } else {
                        renderer_.renderEye(
                            swapchain.renderTargets.at(imageIndex),
                            locatedViews_[index].pose,
                            locatedViews_[index].fov,
                            elapsedSeconds,
                            index);
                    }

                    auto& projectionView = projectionViews_[index];
                    projectionView.pose = locatedViews_[index].pose;
                    projectionView.fov = locatedViews_[index].fov;
                    projectionView.subImage.swapchain = swapchain.handle;
                    projectionView.subImage.imageRect.offset = XrOffset2Di{0, 0};
                    projectionView.subImage.imageRect.extent = XrExtent2Di{
                        static_cast<std::int32_t>(swapchain.width),
                        static_cast<std::int32_t>(swapchain.height)};
                    projectionView.subImage.imageArrayIndex = 0;

                    if (index == 0 && mirrorEnabled_) {
                        renderer_.presentMirror(swapchain.images[imageIndex].texture);
                    }

                    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                    checkXr(
                        instance_,
                        xrReleaseSwapchainImage(swapchain.handle, &releaseInfo),
                        "xrReleaseSwapchainImage");
                    imageAcquired = false;
                } catch (...) {
                    if (imageAcquired) {
                        XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                        static_cast<void>(xrReleaseSwapchainImage(swapchain.handle, &releaseInfo));
                    }
                    throw;
                }
            }
        }

        XrCompositionLayerProjection projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
        projectionLayer.space = appSpace_;
        projectionLayer.viewCount = static_cast<std::uint32_t>(projectionViews_.size());
        projectionLayer.views = projectionViews_.data();

        const XrCompositionLayerBaseHeader* layers[] = {
            reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer),
        };

        XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
        endInfo.displayTime = frameState.predictedDisplayTime;
        endInfo.environmentBlendMode = environmentBlendMode_;
        endInfo.layerCount = renderLayer ? 1U : 0U;
        endInfo.layers = renderLayer ? layers : nullptr;
        const XrResult endResult = xrEndFrame(session_, &endInfo);
        frameOpen = false;
        checkXr(instance_, endResult, "xrEndFrame");
    } catch (...) {
        if (frameOpen) {
            XrFrameEndInfo recoveryEndInfo{XR_TYPE_FRAME_END_INFO};
            recoveryEndInfo.displayTime = frameState.predictedDisplayTime;
            recoveryEndInfo.environmentBlendMode = environmentBlendMode_;
            recoveryEndInfo.layerCount = 0;
            recoveryEndInfo.layers = nullptr;
            static_cast<void>(xrEndFrame(session_, &recoveryEndInfo));
        }
        throw;
    }
}

void OpenXrApp::cleanup() noexcept {
    if (unityTransport_.owner()) unityTransport_.requestShutdown();
    unityTextureSource_.reset();
    unityTransport_.close();
    desktopCapture_.reset();
    virtualScreenAnchorValid_ = false;
    renderer_.shutdownMirror();
    mirrorEnabled_ = false;

    for (auto& swapchain : swapchains_) {
        swapchain.renderTargets.clear();
        swapchain.images.clear();
        if (swapchain.handle != XR_NULL_HANDLE) {
            static_cast<void>(xrDestroySwapchain(swapchain.handle));
            swapchain.handle = XR_NULL_HANDLE;
        }
    }
    swapchains_.clear();

    if (appSpace_ != XR_NULL_HANDLE) {
        static_cast<void>(xrDestroySpace(appSpace_));
        appSpace_ = XR_NULL_HANDLE;
    }
    if (session_ != XR_NULL_HANDLE) {
        static_cast<void>(xrDestroySession(session_));
        session_ = XR_NULL_HANDLE;
    }
    sessionRunning_ = false;
    if (actionSet_ != XR_NULL_HANDLE) {
        static_cast<void>(xrDestroyActionSet(actionSet_));
        actionSet_ = XR_NULL_HANDLE;
    }
    recenterAction_ = XR_NULL_HANDLE;
    adjustScreenAction_ = XR_NULL_HANDLE;
    toggleCurveAction_ = XR_NULL_HANDLE;
    if (instance_ != XR_NULL_HANDLE) {
        static_cast<void>(xrDestroyInstance(instance_));
        instance_ = XR_NULL_HANDLE;
    }
    systemId_ = XR_NULL_SYSTEM_ID;
}

} // namespace uxrb
