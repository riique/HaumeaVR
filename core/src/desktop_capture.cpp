#include "uxrb/desktop_capture.hpp"

#include "uxrb/logger.hpp"

#include <Windows.h>

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace uxrb {
namespace {

using Microsoft::WRL::ComPtr;

std::string hresultMessage(const HRESULT result, const std::string_view operation) {
    std::ostringstream message;
    message << operation << " failed with HRESULT 0x"
            << std::hex << std::uppercase << static_cast<unsigned long>(result);
    return message.str();
}

void throwIfFailed(const HRESULT result, const std::string_view operation) {
    if (FAILED(result)) throw std::runtime_error(hresultMessage(result, operation));
}

std::string wideToUtf8(const wchar_t* text) {
    if (text == nullptr || *text == L'\0') return {};
    const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) return {};
    std::string result(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), required, nullptr, nullptr);
    result.pop_back();
    return result;
}

std::int64_t intersectionArea(const RECT& left, const RECT& right) noexcept {
    const LONG width = std::max<LONG>(0, std::min(left.right, right.right) - std::max(left.left, right.left));
    const LONG height = std::max<LONG>(0, std::min(left.bottom, right.bottom) - std::max(left.top, right.top));
    return static_cast<std::int64_t>(width) * static_cast<std::int64_t>(height);
}

RECT clientRectangleOnDesktop(const HWND window) {
    if (!IsWindow(window)) throw std::invalid_argument("The requested capture HWND is not a valid window.");
    RECT client{};
    if (!GetClientRect(window, &client)) throw std::runtime_error("GetClientRect failed for the capture window.");
    POINT topLeft{client.left, client.top};
    POINT bottomRight{client.right, client.bottom};
    if (!ClientToScreen(window, &topLeft) || !ClientToScreen(window, &bottomRight)) {
        throw std::runtime_error("ClientToScreen failed for the capture window.");
    }
    return RECT{topLeft.x, topLeft.y, bottomRight.x, bottomRight.y};
}

} // namespace

DesktopCapture::~DesktopCapture() {
    reset();
}

void DesktopCapture::initialize(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    const HWND targetWindow) {
    if (device == nullptr || context == nullptr) {
        throw std::invalid_argument("Desktop capture requires a D3D11 device and immediate context.");
    }
    reset();
    device_ = device;
    context_ = context;
    targetWindow_ = targetWindow;
    selectOutput();
    createDuplication();
    if (!captureFrame(1000)) {
        throw std::runtime_error("Desktop Duplication did not deliver an initial frame within one second.");
    }
    Logger::instance().info(
        "Desktop capture ready: " + info_.outputName + ", "
        + std::to_string(info_.width) + "x" + std::to_string(info_.height)
        + (targetWindow_ == nullptr ? ", full output." : ", window client area."));
}

bool DesktopCapture::captureFrame(const std::uint32_t timeoutMilliseconds) {
    if (duplication_ == nullptr) throw std::logic_error("Desktop capture has not been initialized.");
    if (targetWindow_ != nullptr && IsIconic(targetWindow_)) return false;

    if (!updateSourceRectangle()) {
        duplication_.Reset();
        output_.Reset();
        selectOutput();
        createDuplication();
        if (!updateSourceRectangle()) {
            throw std::runtime_error("The capture window is outside every output on the OpenXR adapter.");
        }
    }

    DXGI_OUTDUPL_FRAME_INFO frameInfo{};
    ComPtr<IDXGIResource> resource;
    HRESULT result = duplication_->AcquireNextFrame(timeoutMilliseconds, &frameInfo, &resource);
    if (result == DXGI_ERROR_WAIT_TIMEOUT) return false;
    if (result == DXGI_ERROR_ACCESS_LOST) {
        createDuplication();
        return false;
    }
    throwIfFailed(result, "IDXGIOutputDuplication::AcquireNextFrame");

    bool frameAcquired = true;
    try {
        ComPtr<ID3D11Texture2D> desktopTexture;
        throwIfFailed(resource.As(&desktopTexture), "Query duplicated desktop texture");
        D3D11_TEXTURE2D_DESC desktopDescription{};
        desktopTexture->GetDesc(&desktopDescription);

        const std::uint32_t width = static_cast<std::uint32_t>(sourceRectangle_.right - sourceRectangle_.left);
        const std::uint32_t height = static_cast<std::uint32_t>(sourceRectangle_.bottom - sourceRectangle_.top);
        ensureCaptureTexture(desktopDescription.Format, width, height);

        const LONG outputLeft = outputDescription_.DesktopCoordinates.left;
        const LONG outputTop = outputDescription_.DesktopCoordinates.top;
        const D3D11_BOX sourceBox{
            static_cast<UINT>(sourceRectangle_.left - outputLeft),
            static_cast<UINT>(sourceRectangle_.top - outputTop),
            0,
            static_cast<UINT>(sourceRectangle_.right - outputLeft),
            static_cast<UINT>(sourceRectangle_.bottom - outputTop),
            1,
        };
        context_->CopySubresourceRegion(captureTexture_.Get(), 0, 0, 0, 0, desktopTexture.Get(), 0, &sourceBox);
        throwIfFailed(duplication_->ReleaseFrame(), "IDXGIOutputDuplication::ReleaseFrame");
        frameAcquired = false;
        hasFrame_ = true;
        return true;
    } catch (...) {
        if (frameAcquired) static_cast<void>(duplication_->ReleaseFrame());
        throw;
    }
}

void DesktopCapture::reset() noexcept {
    hasFrame_ = false;
    captureView_.Reset();
    captureTexture_.Reset();
    duplication_.Reset();
    output_.Reset();
    context_.Reset();
    device_.Reset();
    outputDescription_ = {};
    sourceRectangle_ = {};
    targetWindow_ = nullptr;
    info_ = {};
}

ID3D11ShaderResourceView* DesktopCapture::shaderResourceView() const noexcept {
    return captureView_.Get();
}

bool DesktopCapture::hasFrame() const noexcept {
    return hasFrame_;
}

const DesktopCaptureInfo& DesktopCapture::info() const noexcept {
    return info_;
}

void DesktopCapture::selectOutput() {
    ComPtr<IDXGIDevice> dxgiDevice;
    throwIfFailed(device_.As(&dxgiDevice), "Query IDXGIDevice for desktop capture");
    ComPtr<IDXGIAdapter> adapter;
    throwIfFailed(dxgiDevice->GetAdapter(&adapter), "IDXGIDevice::GetAdapter for desktop capture");

    RECT target{};
    if (targetWindow_ != nullptr) target = clientRectangleOnDesktop(targetWindow_);

    std::int64_t bestArea = std::numeric_limits<std::int64_t>::min();
    for (UINT index = 0;; ++index) {
        ComPtr<IDXGIOutput> candidate;
        const HRESULT result = adapter->EnumOutputs(index, &candidate);
        if (result == DXGI_ERROR_NOT_FOUND) break;
        throwIfFailed(result, "IDXGIAdapter::EnumOutputs");

        DXGI_OUTPUT_DESC description{};
        throwIfFailed(candidate->GetDesc(&description), "IDXGIOutput::GetDesc");
        if (description.AttachedToDesktop == FALSE) continue;

        std::int64_t area = 0;
        if (targetWindow_ != nullptr) {
            area = intersectionArea(target, description.DesktopCoordinates);
        } else {
            const RECT primaryPoint{0, 0, 1, 1};
            area = intersectionArea(primaryPoint, description.DesktopCoordinates) > 0 ? 1 : 0;
        }
        if (output_ != nullptr && area <= bestArea) continue;

        ComPtr<IDXGIOutput1> candidateOutput;
        throwIfFailed(candidate.As(&candidateOutput), "Query IDXGIOutput1");
        output_ = candidateOutput;
        outputDescription_ = description;
        bestArea = area;
    }

    if (output_ == nullptr || (targetWindow_ != nullptr && bestArea <= 0)) {
        throw std::runtime_error("No attached output on the OpenXR GPU contains the requested capture region.");
    }
    if (outputDescription_.Rotation != DXGI_MODE_ROTATION_IDENTITY
        && outputDescription_.Rotation != DXGI_MODE_ROTATION_UNSPECIFIED) {
        throw std::runtime_error("Rotated outputs are not yet supported by the Desktop Duplication crop path.");
    }
    info_.outputName = wideToUtf8(outputDescription_.DeviceName);
    info_.windowCapture = targetWindow_ != nullptr;
}

void DesktopCapture::createDuplication() {
    duplication_.Reset();
    throwIfFailed(output_->DuplicateOutput(device_.Get(), &duplication_), "IDXGIOutput1::DuplicateOutput");
}

bool DesktopCapture::updateSourceRectangle() {
    RECT desired = outputDescription_.DesktopCoordinates;
    if (targetWindow_ != nullptr) {
        if (!IsWindow(targetWindow_)) throw std::runtime_error("The capture window was closed.");
        desired = clientRectangleOnDesktop(targetWindow_);
    }

    RECT clipped{};
    if (!IntersectRect(&clipped, &desired, &outputDescription_.DesktopCoordinates)) return false;
    if (clipped.right <= clipped.left || clipped.bottom <= clipped.top) return false;
    sourceRectangle_ = clipped;
    info_.width = static_cast<std::uint32_t>(clipped.right - clipped.left);
    info_.height = static_cast<std::uint32_t>(clipped.bottom - clipped.top);
    info_.desktopLeft = clipped.left;
    info_.desktopTop = clipped.top;
    return true;
}

void DesktopCapture::ensureCaptureTexture(
    const DXGI_FORMAT format,
    const std::uint32_t width,
    const std::uint32_t height) {
    if (captureTexture_ != nullptr) {
        D3D11_TEXTURE2D_DESC current{};
        captureTexture_->GetDesc(&current);
        if (current.Width == width && current.Height == height && current.Format == format) return;
    }

    captureView_.Reset();
    captureTexture_.Reset();
    D3D11_TEXTURE2D_DESC description{};
    description.Width = width;
    description.Height = height;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = format;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    throwIfFailed(device_->CreateTexture2D(&description, nullptr, &captureTexture_), "Create desktop capture texture");
    throwIfFailed(device_->CreateShaderResourceView(captureTexture_.Get(), nullptr, &captureView_), "Create desktop capture SRV");
}

bool runDesktopCaptureSelfTest(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    std::string& failureReason) {
    try {
        DesktopCapture capture;
        capture.initialize(device, context);
        if (!capture.hasFrame() || capture.shaderResourceView() == nullptr
            || capture.info().width == 0 || capture.info().height == 0) {
            throw std::runtime_error("Desktop Duplication produced no usable GPU texture.");
        }
        failureReason.clear();
        return true;
    } catch (const std::exception& exception) {
        failureReason = exception.what();
        return false;
    }
}

bool runDesktopCaptureSelfTest(std::string& failureReason) {
    try {
        ComPtr<IDXGIFactory1> factory;
        throwIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "CreateDXGIFactory1(capture self-test)");

        ComPtr<IDXGIAdapter1> selectedAdapter;
        for (UINT adapterIndex = 0; selectedAdapter == nullptr; ++adapterIndex) {
            ComPtr<IDXGIAdapter1> adapter;
            const HRESULT adapterResult = factory->EnumAdapters1(adapterIndex, &adapter);
            if (adapterResult == DXGI_ERROR_NOT_FOUND) break;
            throwIfFailed(adapterResult, "IDXGIFactory1::EnumAdapters1(capture self-test)");

            DXGI_ADAPTER_DESC1 adapterDescription{};
            throwIfFailed(adapter->GetDesc1(&adapterDescription), "IDXGIAdapter1::GetDesc1(capture self-test)");
            if ((adapterDescription.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) continue;

            for (UINT outputIndex = 0;; ++outputIndex) {
                ComPtr<IDXGIOutput> output;
                const HRESULT outputResult = adapter->EnumOutputs(outputIndex, &output);
                if (outputResult == DXGI_ERROR_NOT_FOUND) break;
                throwIfFailed(outputResult, "IDXGIAdapter1::EnumOutputs(capture self-test)");
                DXGI_OUTPUT_DESC outputDescription{};
                throwIfFailed(output->GetDesc(&outputDescription), "IDXGIOutput::GetDesc(capture self-test)");
                const RECT primaryPoint{0, 0, 1, 1};
                if (outputDescription.AttachedToDesktop != FALSE
                    && intersectionArea(primaryPoint, outputDescription.DesktopCoordinates) > 0) {
                    selectedAdapter = adapter;
                    break;
                }
            }
        }
        if (selectedAdapter == nullptr) {
            throw std::runtime_error("No hardware adapter owns the primary desktop output.");
        }

        constexpr D3D_FEATURE_LEVEL levels[]{
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        D3D_FEATURE_LEVEL createdLevel{};
        throwIfFailed(
            D3D11CreateDevice(
                selectedAdapter.Get(),
                D3D_DRIVER_TYPE_UNKNOWN,
                nullptr,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                levels,
                static_cast<UINT>(std::size(levels)),
                D3D11_SDK_VERSION,
                &device,
                &createdLevel,
                &context),
            "D3D11CreateDevice(capture self-test)");
        static_cast<void>(createdLevel);
        return runDesktopCaptureSelfTest(device.Get(), context.Get(), failureReason);
    } catch (const std::exception& exception) {
        failureReason = exception.what();
        return false;
    }
}

} // namespace uxrb
