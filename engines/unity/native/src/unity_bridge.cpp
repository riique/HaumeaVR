#include "uxrb/unity_bridge.h"
#include "uxrb/unity_transport.hpp"

#include <Windows.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityGraphicsD3D11.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cwchar>
#include <mutex>
#include <string>

using Microsoft::WRL::ComPtr;

namespace {

struct EyeResources final {
    ComPtr<ID3D11Texture2D> source;
    ComPtr<ID3D11Texture2D> shared;
    ComPtr<IDXGIKeyedMutex> keyedMutex;
    HANDLE sharedHandle{};
    std::wstring sharedName;
    std::uint32_t expectedWidth{};
    std::uint32_t expectedHeight{};
    std::uint32_t generation{};
};

class Bridge final {
public:
    static Bridge& instance() {
        static Bridge value;
        return value;
    }

    void onUnityLoad(IUnityInterfaces* interfaces) {
        std::scoped_lock lock(mutex_);
        unityInterfaces_ = interfaces;
        unityGraphics_ = interfaces == nullptr ? nullptr : interfaces->Get<IUnityGraphics>();
        if (unityGraphics_ == nullptr) {
            status_.lastError = uxrb::unity::BridgeError::unityInterfacesUnavailable;
            return;
        }

        unityGraphics_->RegisterDeviceEventCallback(onGraphicsDeviceEvent);
        initializeDeviceLocked();
    }

    void onUnityUnload() {
        std::scoped_lock lock(mutex_);
        publishTransportStateLocked(true);
        if (unityGraphics_ != nullptr) {
            unityGraphics_->UnregisterDeviceEventCallback(onGraphicsDeviceEvent);
        }
        clearDeviceLocked();
        unityGraphics_ = nullptr;
        unityInterfaces_ = nullptr;
        transport_.close();
    }

    void onDeviceEvent(const UnityGfxDeviceEventType eventType) {
        std::scoped_lock lock(mutex_);
        switch (eventType) {
            case kUnityGfxDeviceEventInitialize:
            case kUnityGfxDeviceEventAfterReset:
                initializeDeviceLocked();
                break;
            case kUnityGfxDeviceEventShutdown:
            case kUnityGfxDeviceEventBeforeReset:
                clearDeviceLocked();
                break;
        }
    }

    int registerEyeTexture(
        const int eye,
        void* nativeTexture,
        const std::uint32_t expectedWidth,
        const std::uint32_t expectedHeight) {
        if (eye < 0 || eye >= static_cast<int>(uxrb::unity::kEyeCount)) {
            setError(uxrb::unity::BridgeError::invalidEye);
            return 0;
        }

        std::scoped_lock lock(mutex_);
        auto& resources = eyes_[static_cast<std::size_t>(eye)];
        resources.source.Reset();
        resources.expectedWidth = expectedWidth;
        resources.expectedHeight = expectedHeight;

        if (nativeTexture == nullptr) {
            clearEyeLocked(static_cast<std::size_t>(eye));
            return 1;
        }

        auto* resource = static_cast<ID3D11Resource*>(nativeTexture);
        if (FAILED(resource->QueryInterface(IID_PPV_ARGS(&resources.source)))) {
            status_.lastError = uxrb::unity::BridgeError::invalidTexture;
            return 0;
        }
        return 1;
    }

    void submitFrame(const std::uint64_t frameId) {
        std::scoped_lock lock(mutex_);
        status_.submittedFrameId = frameId;
    }

    int setStereoFrame(const uxrb::unity::StereoFrame* frame) {
        if (frame == nullptr || frame->structSize != sizeof(uxrb::unity::StereoFrame) ||
            frame->apiVersion != uxrb::unity::kApiVersion) {
            return 0;
        }
        std::scoped_lock lock(mutex_);
        stereoFrame_ = *frame;
        hasStereoFrame_ = true;
        return 1;
    }

    int tryGetStereoFrame(uxrb::unity::StereoFrame* frame) {
        if (frame == nullptr || frame->structSize != sizeof(uxrb::unity::StereoFrame)) {
            return 0;
        }
        std::scoped_lock lock(mutex_);
        ensureTransportLocked();
        if (transport_.valid()) {
            if (transport_.shutdownRequested()) return 0;
            uxrb::unity_transport::StereoFrame transportFrame{};
            if (transport_.readFrame(transportFrame) && transportFrame.frameId != 0) {
                frame->structSize = sizeof(uxrb::unity::StereoFrame);
                frame->apiVersion = uxrb::unity::kApiVersion;
                frame->frameId = transportFrame.frameId;
                frame->predictedDisplayTimeNanoseconds = transportFrame.predictedDisplayTimeNanoseconds;
                frame->recommendedWidth = transportFrame.recommendedWidth;
                frame->recommendedHeight = transportFrame.recommendedHeight;
                frame->flags = transportFrame.flags;
                frame->reserved = 0;
                for (std::size_t eye = 0; eye < uxrb::unity::kEyeCount; ++eye) {
                    std::copy_n(transportFrame.eyes[eye].position, 3, frame->eyes[eye].position);
                    std::copy_n(transportFrame.eyes[eye].orientation, 4, frame->eyes[eye].orientation);
                    std::copy_n(transportFrame.eyes[eye].fov, 4, frame->eyes[eye].fov);
                }
                return 1;
            }
        }
        if (!hasStereoFrame_) {
            return 0;
        }
        *frame = stereoFrame_;
        return 1;
    }

    int getStatus(uxrb::unity::BridgeStatus* status) {
        if (status == nullptr || status->structSize != sizeof(uxrb::unity::BridgeStatus)) {
            return 0;
        }
        std::scoped_lock lock(mutex_);
        *status = status_;
        return 1;
    }

    int copySharedName(const int eye, wchar_t* destination, const int capacity) {
        if (eye < 0 || eye >= static_cast<int>(uxrb::unity::kEyeCount) || destination == nullptr || capacity <= 0) {
            return 0;
        }
        std::scoped_lock lock(mutex_);
        const auto& name = eyes_[static_cast<std::size_t>(eye)].sharedName;
        if (name.empty() || name.size() + 1U > static_cast<std::size_t>(capacity)) {
            destination[0] = L'\0';
            return 0;
        }
        std::wmemcpy(destination, name.c_str(), name.size() + 1U);
        return static_cast<int>(name.size());
    }

    void render(const int eventId) {
        if (eventId != uxrb::unity::kCopyStereoFrameEvent) {
            return;
        }

        std::scoped_lock lock(mutex_);
        if (device_ == nullptr || context_ == nullptr) {
            ++status_.droppedFrames;
            return;
        }

        bool copiedAny = false;
        for (std::size_t eye = 0; eye < eyes_.size(); ++eye) {
            copiedAny |= copyEyeLocked(eye);
        }
        if (copiedAny) {
            status_.copiedFrameId = status_.submittedFrameId;
        } else {
            ++status_.droppedFrames;
        }
        publishTransportStateLocked(false);
    }

    void reset() {
        std::scoped_lock lock(mutex_);
        for (std::size_t eye = 0; eye < eyes_.size(); ++eye) {
            clearEyeLocked(eye);
        }
        status_.submittedFrameId = 0;
        status_.copiedFrameId = 0;
        status_.lastError = uxrb::unity::BridgeError::none;
        status_.droppedFrames = 0;
        stereoFrame_ = {};
        hasStereoFrame_ = false;
        publishTransportStateLocked(true);
    }

private:
    static void UNITY_INTERFACE_API onGraphicsDeviceEvent(const UnityGfxDeviceEventType eventType) {
        instance().onDeviceEvent(eventType);
    }

    void initializeDeviceLocked() {
        clearDeviceLocked();
        if (unityGraphics_ == nullptr || unityInterfaces_ == nullptr) {
            status_.lastError = uxrb::unity::BridgeError::unityInterfacesUnavailable;
            return;
        }

        const auto renderer = unityGraphics_->GetRenderer();
        status_.unityRenderer = static_cast<std::uint32_t>(renderer);
        if (renderer != kUnityGfxRendererD3D11) {
            status_.lastError = uxrb::unity::BridgeError::unsupportedRenderer;
            return;
        }

        const auto* graphicsD3d11 = unityInterfaces_->Get<IUnityGraphicsD3D11>();
        if (graphicsD3d11 == nullptr || graphicsD3d11->GetDevice == nullptr) {
            status_.lastError = uxrb::unity::BridgeError::d3d11InterfaceUnavailable;
            return;
        }

        device_ = graphicsD3d11->GetDevice();
        if (device_ == nullptr) {
            status_.lastError = uxrb::unity::BridgeError::d3d11InterfaceUnavailable;
            return;
        }
        device_->GetImmediateContext(&context_);
        status_.lastError = uxrb::unity::BridgeError::none;
    }

    void clearDeviceLocked() {
        for (std::size_t eye = 0; eye < eyes_.size(); ++eye) {
            clearSharedEyeLocked(eye);
        }
        context_.Reset();
        device_.Reset();
    }

    void clearEyeLocked(const std::size_t eye) {
        eyes_[eye].source.Reset();
        eyes_[eye].expectedWidth = 0;
        eyes_[eye].expectedHeight = 0;
        clearSharedEyeLocked(eye);
    }

    void clearSharedEyeLocked(const std::size_t eye) {
        if (eyes_[eye].sharedHandle != nullptr) {
            CloseHandle(eyes_[eye].sharedHandle);
            eyes_[eye].sharedHandle = nullptr;
        }
        eyes_[eye].keyedMutex.Reset();
        eyes_[eye].shared.Reset();
        eyes_[eye].sharedName.clear();
        status_.readyEyeMask &= ~(1U << eye);
        status_.width[eye] = 0;
        status_.height[eye] = 0;
        status_.dxgiFormat[eye] = 0;
    }

    bool copyEyeLocked(const std::size_t eye) {
        auto& resources = eyes_[eye];
        if (resources.source == nullptr) {
            return false;
        }

        D3D11_TEXTURE2D_DESC sourceDesc{};
        resources.source->GetDesc(&sourceDesc);
        if (sourceDesc.ArraySize != 1 || sourceDesc.MipLevels == 0 || sourceDesc.SampleDesc.Count != 1 ||
            (resources.expectedWidth != 0 && resources.expectedWidth != sourceDesc.Width) ||
            (resources.expectedHeight != 0 && resources.expectedHeight != sourceDesc.Height)) {
            status_.lastError = uxrb::unity::BridgeError::unsupportedTexture;
            return false;
        }

        if (!ensureSharedEyeLocked(eye, sourceDesc)) {
            return false;
        }

        const HRESULT acquireResult = resources.keyedMutex->AcquireSync(0, 0);
        if (FAILED(acquireResult)) {
            status_.lastError = uxrb::unity::BridgeError::synchronizationFailed;
            return false;
        }

        context_->CopyResource(resources.shared.Get(), resources.source.Get());
        // Latest-frame transport: producer and consumer both acquire/release key
        // zero. This serializes access without stalling forever when either side
        // starts late or disconnects; frame identity is carried separately.
        const HRESULT releaseResult = resources.keyedMutex->ReleaseSync(0);
        if (FAILED(releaseResult)) {
            status_.lastError = uxrb::unity::BridgeError::synchronizationFailed;
            return false;
        }

        status_.lastError = uxrb::unity::BridgeError::none;
        return true;
    }

    bool ensureSharedEyeLocked(const std::size_t eye, const D3D11_TEXTURE2D_DESC& sourceDesc) {
        auto& resources = eyes_[eye];
        if (resources.shared != nullptr && status_.width[eye] == sourceDesc.Width &&
            status_.height[eye] == sourceDesc.Height &&
            status_.dxgiFormat[eye] == static_cast<std::uint32_t>(sourceDesc.Format)) {
            return true;
        }

        clearSharedEyeLocked(eye);
        D3D11_TEXTURE2D_DESC sharedDesc = sourceDesc;
        sharedDesc.MipLevels = 1;
        sharedDesc.ArraySize = 1;
        sharedDesc.SampleDesc = {1, 0};
        sharedDesc.Usage = D3D11_USAGE_DEFAULT;
        sharedDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        sharedDesc.CPUAccessFlags = 0;
        sharedDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

        if (FAILED(device_->CreateTexture2D(&sharedDesc, nullptr, &resources.shared))) {
            status_.lastError = uxrb::unity::BridgeError::sharedTextureCreationFailed;
            return false;
        }
        if (FAILED(resources.shared.As(&resources.keyedMutex))) {
            clearSharedEyeLocked(eye);
            status_.lastError = uxrb::unity::BridgeError::sharedTextureCreationFailed;
            return false;
        }

        ComPtr<IDXGIResource1> dxgiResource;
        if (FAILED(resources.shared.As(&dxgiResource))) {
            clearSharedEyeLocked(eye);
            status_.lastError = uxrb::unity::BridgeError::sharedTextureCreationFailed;
            return false;
        }

        ++resources.generation;
        resources.sharedName = L"Local\\UXRB-Unity-" + std::to_wstring(GetCurrentProcessId()) + L"-eye" +
            std::to_wstring(eye) + L"-g" + std::to_wstring(resources.generation);
        const HRESULT handleResult = dxgiResource->CreateSharedHandle(
            nullptr,
            DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
            resources.sharedName.c_str(),
            &resources.sharedHandle);
        if (FAILED(handleResult)) {
            clearSharedEyeLocked(eye);
            status_.lastError = uxrb::unity::BridgeError::sharedTextureCreationFailed;
            return false;
        }
        status_.readyEyeMask |= 1U << eye;
        status_.width[eye] = sourceDesc.Width;
        status_.height[eye] = sourceDesc.Height;
        status_.dxgiFormat[eye] = static_cast<std::uint32_t>(sourceDesc.Format);
        status_.generation[eye] = resources.generation;
        return true;
    }

    void ensureTransportLocked() {
        if (transport_.valid()) return;
        const auto now = std::chrono::steady_clock::now();
        if (now < nextTransportAttempt_) return;
        nextTransportAttempt_ = now + std::chrono::milliseconds(500);
        const std::wstring name = uxrb::unity_transport::Mapping::environmentName();
        if (name.empty()) return;
        std::string error;
        static_cast<void>(transport_.openClient(name, error));
    }

    void publishTransportStateLocked(const bool clearing) {
        ensureTransportLocked();
        if (!transport_.valid()) return;
        uxrb::unity_transport::PluginState plugin{};
        plugin.processId = GetCurrentProcessId();
        plugin.readyEyeMask = clearing ? 0U : status_.readyEyeMask;
        plugin.submittedFrameId = status_.submittedFrameId;
        plugin.copiedFrameId = status_.copiedFrameId;
        plugin.lastError = static_cast<std::uint32_t>(status_.lastError);
        plugin.droppedFrames = status_.droppedFrames;
        for (std::size_t eye = 0; eye < eyes_.size(); ++eye) {
            plugin.eyes[eye].width = status_.width[eye];
            plugin.eyes[eye].height = status_.height[eye];
            plugin.eyes[eye].dxgiFormat = status_.dxgiFormat[eye];
            plugin.eyes[eye].generation = status_.generation[eye];
            if (!clearing && !eyes_[eye].sharedName.empty()) {
                wcsncpy_s(
                    plugin.eyes[eye].sharedName,
                    eyes_[eye].sharedName.c_str(),
                    uxrb::unity_transport::textureNameCharacters - 1U);
            }
        }
        static_cast<void>(transport_.publishPluginState(plugin));
    }

    void setError(const uxrb::unity::BridgeError error) {
        std::scoped_lock lock(mutex_);
        status_.lastError = error;
    }

    std::mutex mutex_;
    IUnityInterfaces* unityInterfaces_{};
    IUnityGraphics* unityGraphics_{};
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    std::array<EyeResources, uxrb::unity::kEyeCount> eyes_{};
    uxrb::unity::StereoFrame stereoFrame_{};
    bool hasStereoFrame_{};
    uxrb::unity::BridgeStatus status_{};
    uxrb::unity_transport::Mapping transport_;
    std::chrono::steady_clock::time_point nextTransportAttempt_{};
};

void UNITY_INTERFACE_API onRenderEvent(const int eventId) {
    Bridge::instance().render(eventId);
}

}  // namespace

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces) {
    Bridge::instance().onUnityLoad(unityInterfaces);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload() {
    Bridge::instance().onUnityUnload();
}

extern "C" std::uint32_t UXRB_UNITY_API UXRB_UNITY_CALL UxrbUnity_GetApiVersion() {
    return uxrb::unity::kApiVersion;
}

extern "C" std::int32_t UXRB_UNITY_API UXRB_UNITY_CALL UxrbUnity_GetCopyStereoFrameEventId() {
    return uxrb::unity::kCopyStereoFrameEvent;
}

extern "C" uxrb::unity::UnityRenderingEvent UXRB_UNITY_API UXRB_UNITY_CALL UxrbUnity_GetRenderEventFunc() {
    return onRenderEvent;
}

extern "C" std::int32_t UXRB_UNITY_API UXRB_UNITY_CALL UxrbUnity_RegisterEyeTexture(
    const std::int32_t eye,
    void* nativeTexture,
    const std::uint32_t expectedWidth,
    const std::uint32_t expectedHeight) {
    return Bridge::instance().registerEyeTexture(eye, nativeTexture, expectedWidth, expectedHeight);
}

extern "C" void UXRB_UNITY_API UXRB_UNITY_CALL UxrbUnity_SubmitFrame(const std::uint64_t frameId) {
    Bridge::instance().submitFrame(frameId);
}

extern "C" std::int32_t UXRB_UNITY_API UXRB_UNITY_CALL UxrbUnity_SetStereoFrame(
    const uxrb::unity::StereoFrame* frame) {
    return Bridge::instance().setStereoFrame(frame);
}

extern "C" std::int32_t UXRB_UNITY_API UXRB_UNITY_CALL UxrbUnity_TryGetStereoFrame(
    uxrb::unity::StereoFrame* frame) {
    return Bridge::instance().tryGetStereoFrame(frame);
}

extern "C" std::int32_t UXRB_UNITY_API UXRB_UNITY_CALL UxrbUnity_GetStatus(
    uxrb::unity::BridgeStatus* status) {
    return Bridge::instance().getStatus(status);
}

extern "C" std::int32_t UXRB_UNITY_API UXRB_UNITY_CALL UxrbUnity_CopySharedTextureName(
    const std::int32_t eye,
    wchar_t* destination,
    const std::int32_t destinationCharacterCount) {
    return Bridge::instance().copySharedName(eye, destination, destinationCharacterCount);
}

extern "C" void UXRB_UNITY_API UXRB_UNITY_CALL UxrbUnity_Reset() {
    Bridge::instance().reset();
}
