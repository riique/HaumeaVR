#include "uxrb/unity_shared_texture_source.hpp"

#include <Windows.h>
#include <wrl/client.h>

#include <array>
#include <cstring>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace uxrb {
namespace {

void checkHr(const HRESULT result, const char* operation) {
    if (FAILED(result)) {
        throw std::runtime_error(std::string(operation) + " failed with HRESULT " + std::to_string(result) + '.');
    }
}

}  // namespace

void UnitySharedTextureSource::initialize(
    ID3D11Device* device,
    unity_transport::Mapping* transport,
    const std::uint32_t expectedProcessId) {
    reset();
    if (device == nullptr || transport == nullptr || !transport->owner()) {
        throw std::invalid_argument("Unity shared texture source requires a device and owner transport.");
    }
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&device_)))) {
        throw std::runtime_error("The OpenXR D3D11 device does not expose ID3D11Device1.");
    }
    transport_ = transport;
    expectedProcessId_ = expectedProcessId;
}

void UnitySharedTextureSource::reset() noexcept {
    for (auto& eye : eyes_) clearEye(eye);
    device_.Reset();
    transport_ = nullptr;
    expectedProcessId_ = 0;
    producerProcessId_ = 0;
    lastError_.clear();
}

bool UnitySharedTextureSource::refresh() noexcept {
    if (device_ == nullptr || transport_ == nullptr) return false;
    unity_transport::PluginState state{};
    if (!transport_->readPluginState(state) || state.processId == 0) return false;
    if (expectedProcessId_ != 0 && state.processId != expectedProcessId_) {
        lastError_ = "Unity shared texture producer PID does not match the requested process.";
        return false;
    }
    if (producerProcessId_ != 0 && producerProcessId_ != state.processId) {
        lastError_ = "Unity shared texture producer changed during the session.";
        return false;
    }
    producerProcessId_ = state.processId;

    bool anyReady = false;
    for (std::uint32_t eyeIndex = 0; eyeIndex < eyes_.size(); ++eyeIndex) {
        if ((state.readyEyeMask & (1U << eyeIndex)) == 0) {
            clearEye(eyes_[eyeIndex]);
            continue;
        }
        const auto& eyeState = state.eyes[eyeIndex];
        const auto& eye = eyes_[eyeIndex];
        if (eye.texture != nullptr && eye.generation == eyeState.generation &&
            eye.name == eyeState.sharedName) {
            anyReady = true;
            continue;
        }
        anyReady |= openEye(eyeIndex, eyeState, state.processId);
    }
    return anyReady;
}

bool UnitySharedTextureSource::copyEye(
    ID3D11DeviceContext* context,
    const EyeRenderTarget& target,
    const std::uint32_t eyeIndex) noexcept {
    if (context == nullptr || eyeIndex >= eyes_.size()) return false;
    auto& eye = eyes_[eyeIndex];
    if (eye.texture == nullptr || eye.keyedMutex == nullptr || target.colorTexture == nullptr ||
        target.width != eye.width || target.height != eye.height) {
        return false;
    }
    D3D11_TEXTURE2D_DESC targetDescription{};
    target.colorTexture->GetDesc(&targetDescription);
    if (!compatibleFormats(eye.format, targetDescription.Format)) return false;

    if (FAILED(eye.keyedMutex->AcquireSync(0, 0))) return false;
    context->CopyResource(target.colorTexture.Get(), eye.texture.Get());
    const HRESULT releaseResult = eye.keyedMutex->ReleaseSync(0);
    if (FAILED(releaseResult)) {
        lastError_ = "Unity shared texture keyed mutex release failed.";
        return false;
    }
    return true;
}

bool UnitySharedTextureSource::ready(const std::uint32_t eyeIndex) const noexcept {
    return eyeIndex < eyes_.size() && eyes_[eyeIndex].texture != nullptr;
}

std::uint32_t UnitySharedTextureSource::producerProcessId() const noexcept {
    return producerProcessId_;
}

const std::string& UnitySharedTextureSource::lastError() const noexcept {
    return lastError_;
}

bool UnitySharedTextureSource::openEye(
    const std::uint32_t eyeIndex,
    const unity_transport::EyeTexture& state,
    const std::uint32_t processId) noexcept {
    if (eyeIndex >= eyes_.size() || state.generation == 0 || state.width == 0 || state.height == 0 ||
        state.sharedName[0] == L'\0') {
        return false;
    }

    const std::wstring name(state.sharedName);
    const std::wstring expectedPrefix = L"Local\\UXRB-Unity-" + std::to_wstring(processId) + L"-eye" +
        std::to_wstring(eyeIndex) + L"-g";
    if (!name.starts_with(expectedPrefix) || name.size() >= unity_transport::textureNameCharacters) {
        lastError_ = "Unity shared texture name failed producer/eye validation.";
        return false;
    }

    Eye opened;
    const HRESULT openResult = device_->OpenSharedResourceByName(
        name.c_str(),
        DXGI_SHARED_RESOURCE_READ,
        IID_PPV_ARGS(&opened.texture));
    if (FAILED(openResult) || opened.texture == nullptr || FAILED(opened.texture.As(&opened.keyedMutex))) {
        lastError_ = "OpenSharedResourceByName failed for a Unity eye texture.";
        return false;
    }

    D3D11_TEXTURE2D_DESC description{};
    opened.texture->GetDesc(&description);
    if (description.Width != state.width || description.Height != state.height ||
        static_cast<std::uint32_t>(description.Format) != state.dxgiFormat ||
        (description.MiscFlags & D3D11_RESOURCE_MISC_SHARED_NTHANDLE) == 0 ||
        (description.MiscFlags & D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX) == 0) {
        lastError_ = "Unity shared texture description does not match the authenticated transport metadata.";
        return false;
    }

    opened.generation = state.generation;
    opened.width = state.width;
    opened.height = state.height;
    opened.format = description.Format;
    opened.name = name;
    eyes_[eyeIndex] = std::move(opened);
    lastError_.clear();
    return true;
}

void UnitySharedTextureSource::clearEye(Eye& eye) noexcept {
    eye.keyedMutex.Reset();
    eye.texture.Reset();
    eye.generation = 0;
    eye.width = 0;
    eye.height = 0;
    eye.format = DXGI_FORMAT_UNKNOWN;
    eye.name.clear();
}

bool UnitySharedTextureSource::compatibleFormats(
    const DXGI_FORMAT source,
    const DXGI_FORMAT destination) noexcept {
    if (source == destination) return true;
    const bool sourceRgba = source == DXGI_FORMAT_R8G8B8A8_UNORM || source == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    const bool destinationRgba = destination == DXGI_FORMAT_R8G8B8A8_UNORM || destination == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    return sourceRgba && destinationRgba;
}

bool runUnitySharedTextureSourceSelfTest(std::string& failureReason) {
    try {
        constexpr std::uint32_t width = 16;
        constexpr std::uint32_t height = 8;
        const std::wstring mappingName = L"Local\\UXRB-Unity-source-test-" + std::to_wstring(GetCurrentProcessId());
        std::string error;
        unity_transport::Mapping owner;
        unity_transport::Mapping producerMapping;
        if (!owner.createOwner(mappingName, error) || !producerMapping.openClient(mappingName, error)) {
            throw std::runtime_error(error);
        }

        ComPtr<ID3D11Device> producerDevice;
        ComPtr<ID3D11DeviceContext> producerContext;
        ComPtr<ID3D11Device> consumerDevice;
        ComPtr<ID3D11DeviceContext> consumerContext;
        D3D_FEATURE_LEVEL level{};
        checkHr(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
            &producerDevice, &level, &producerContext), "Create producer WARP device");
        checkHr(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
            &consumerDevice, &level, &consumerContext), "Create consumer WARP device");

        D3D11_TEXTURE2D_DESC description{};
        description.Width = width;
        description.Height = height;
        description.MipLevels = 1;
        description.ArraySize = 1;
        description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        description.SampleDesc.Count = 1;
        description.Usage = D3D11_USAGE_DEFAULT;
        description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        description.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
        ComPtr<ID3D11Texture2D> producerTexture;
        checkHr(producerDevice->CreateTexture2D(&description, nullptr, &producerTexture), "Create shared producer texture");
        ComPtr<IDXGIResource1> resource;
        checkHr(producerTexture.As(&resource), "Query IDXGIResource1");
        const std::wstring textureName = L"Local\\UXRB-Unity-" + std::to_wstring(GetCurrentProcessId()) + L"-eye0-g1";
        HANDLE sharedHandle{};
        checkHr(resource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
            textureName.c_str(), &sharedHandle), "Create named shared handle");
        ComPtr<IDXGIKeyedMutex> producerMutex;
        checkHr(producerTexture.As(&producerMutex), "Query producer keyed mutex");
        checkHr(producerMutex->AcquireSync(0, INFINITE), "Acquire producer keyed mutex");
        ComPtr<ID3D11RenderTargetView> producerView;
        checkHr(producerDevice->CreateRenderTargetView(producerTexture.Get(), nullptr, &producerView), "Create producer RTV");
        constexpr float red[]{1.0F, 0.0F, 0.0F, 1.0F};
        producerContext->ClearRenderTargetView(producerView.Get(), red);
        checkHr(producerMutex->ReleaseSync(0), "Release producer keyed mutex");

        unity_transport::PluginState state{};
        state.processId = GetCurrentProcessId();
        state.readyEyeMask = 1;
        state.eyes[0].width = width;
        state.eyes[0].height = height;
        state.eyes[0].dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        state.eyes[0].generation = 1;
        wcscpy_s(state.eyes[0].sharedName, textureName.c_str());
        if (!producerMapping.publishPluginState(state)) throw std::runtime_error("Publish plug-in state failed.");

        UnitySharedTextureSource source;
        source.initialize(consumerDevice.Get(), &owner, GetCurrentProcessId());
        if (!source.refresh()) throw std::runtime_error(source.lastError());
        CloseHandle(sharedHandle);
        sharedHandle = nullptr;

        D3D11_TEXTURE2D_DESC targetDescription = description;
        targetDescription.BindFlags = D3D11_BIND_RENDER_TARGET;
        targetDescription.MiscFlags = 0;
        ComPtr<ID3D11Texture2D> target;
        checkHr(consumerDevice->CreateTexture2D(&targetDescription, nullptr, &target), "Create consumer target");
        EyeRenderTarget targetWrapper;
        targetWrapper.colorTexture = target;
        targetWrapper.width = width;
        targetWrapper.height = height;
        if (!source.copyEye(consumerContext.Get(), targetWrapper, 0)) {
            throw std::runtime_error("Copy shared Unity eye failed: " + source.lastError());
        }

        D3D11_TEXTURE2D_DESC stagingDescription = targetDescription;
        stagingDescription.Usage = D3D11_USAGE_STAGING;
        stagingDescription.BindFlags = 0;
        stagingDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> staging;
        checkHr(consumerDevice->CreateTexture2D(&stagingDescription, nullptr, &staging), "Create Unity staging texture");
        consumerContext->CopyResource(staging.Get(), target.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        checkHr(consumerContext->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped), "Map Unity staging texture");
        const auto* pixel = static_cast<const std::uint8_t*>(mapped.pData);
        const bool redCopied = pixel[0] >= 250 && pixel[1] <= 5 && pixel[2] <= 5 && pixel[3] >= 250;
        consumerContext->Unmap(staging.Get(), 0);
        if (!redCopied) throw std::runtime_error("Named shared texture contents did not reach the consumer.");

        failureReason.clear();
        return true;
    } catch (const std::exception& exception) {
        failureReason = exception.what();
        return false;
    }
}

}  // namespace uxrb
