#include "uxrb/d3d11_renderer.hpp"

#include "uxrb/logger.hpp"
#include "uxrb/projection.hpp"

#include <d3dcompiler.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace uxrb {
namespace {

using Microsoft::WRL::ComPtr;

struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 color;
};

struct ScreenVertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 textureCoordinate;
};

constexpr std::uint32_t screenSegmentCount = 64;

std::string hresultMessage(const HRESULT result, const std::string_view operation) {
    std::ostringstream message;
    message << operation << " failed with HRESULT 0x"
            << std::hex << std::uppercase << static_cast<unsigned long>(result);

    char* systemMessage = nullptr;
    const DWORD length = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        static_cast<DWORD>(result),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<char*>(&systemMessage),
        0,
        nullptr);
    if (length != 0 && systemMessage != nullptr) {
        message << ": " << systemMessage;
        LocalFree(systemMessage);
    }
    return message.str();
}

void throwIfFailed(const HRESULT result, const std::string_view operation) {
    if (FAILED(result)) {
        throw std::runtime_error(hresultMessage(result, operation));
    }
}

std::string wideToUtf8(const wchar_t* text) {
    if (text == nullptr || *text == L'\0') {
        return {};
    }
    const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) {
        return {};
    }
    std::string result(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), required, nullptr, nullptr);
    result.pop_back();
    return result;
}

DXGI_FORMAT linearFormat(const DXGI_FORMAT format) noexcept {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    default:
        return format;
    }
}

std::string featureLevelName(const D3D_FEATURE_LEVEL featureLevel) {
    switch (featureLevel) {
    case D3D_FEATURE_LEVEL_12_1:
        return "12_1";
    case D3D_FEATURE_LEVEL_12_0:
        return "12_0";
    case D3D_FEATURE_LEVEL_11_1:
        return "11_1";
    case D3D_FEATURE_LEVEL_11_0:
        return "11_0";
    case D3D_FEATURE_LEVEL_10_1:
        return "10_1";
    case D3D_FEATURE_LEVEL_10_0:
        return "10_0";
    default:
        return "unknown";
    }
}

} // namespace

void D3D11Renderer::initializeDevice(
    const LUID& adapterLuid,
    const D3D_FEATURE_LEVEL minimumFeatureLevel) {
    findAdapter(adapterLuid);

    constexpr std::array requestedLevels{
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    std::vector<D3D_FEATURE_LEVEL> usableLevels;
    for (const D3D_FEATURE_LEVEL level : requestedLevels) {
        if (level >= minimumFeatureLevel) {
            usableLevels.push_back(level);
        }
    }
    if (usableLevels.empty()) {
        throw std::runtime_error("The runtime requested an unsupported D3D feature level.");
    }

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifndef NDEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT result = D3D11CreateDevice(
        adapter_.Get(),
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        flags,
        usableLevels.data(),
        static_cast<UINT>(usableLevels.size()),
        D3D11_SDK_VERSION,
        &device_,
        &featureLevel_,
        &context_);

#ifndef NDEBUG
    if (FAILED(result)) {
        Logger::instance().warning("D3D11 debug device creation failed; retrying without the debug layer.");
        flags &= ~D3D11_CREATE_DEVICE_DEBUG;
        result = D3D11CreateDevice(
            adapter_.Get(),
            D3D_DRIVER_TYPE_UNKNOWN,
            nullptr,
            flags,
            usableLevels.data(),
            static_cast<UINT>(usableLevels.size()),
            D3D11_SDK_VERSION,
            &device_,
            &featureLevel_,
            &context_);
    }
#endif

    throwIfFailed(result, "D3D11CreateDevice");
    Logger::instance().info(
        "D3D11 device: " + adapterName_ + ", feature level " + featureLevelName(featureLevel_));
}

void D3D11Renderer::createPipeline() {
    if (device_ == nullptr) {
        throw std::logic_error("The D3D11 device must be initialized before the pipeline.");
    }
    compileShaders();
    createGeometry();
}

EyeRenderTarget D3D11Renderer::createEyeRenderTarget(
    ID3D11Texture2D* colorTexture,
    const DXGI_FORMAT colorFormat) const {
    if (colorTexture == nullptr) {
        throw std::invalid_argument("OpenXR returned a null D3D11 swapchain image.");
    }

    EyeRenderTarget target;
    target.colorTexture = colorTexture;

    D3D11_TEXTURE2D_DESC colorDescription{};
    colorTexture->GetDesc(&colorDescription);
    target.width = colorDescription.Width;
    target.height = colorDescription.Height;

    D3D11_RENDER_TARGET_VIEW_DESC colorViewDescription{};
    colorViewDescription.Format = colorFormat;
    colorViewDescription.ViewDimension = colorDescription.SampleDesc.Count > 1
        ? D3D11_RTV_DIMENSION_TEXTURE2DMS
        : D3D11_RTV_DIMENSION_TEXTURE2D;
    colorViewDescription.Texture2D.MipSlice = 0;
    throwIfFailed(
        device_->CreateRenderTargetView(colorTexture, &colorViewDescription, &target.colorView),
        "ID3D11Device::CreateRenderTargetView");

    D3D11_TEXTURE2D_DESC depthDescription{};
    depthDescription.Width = colorDescription.Width;
    depthDescription.Height = colorDescription.Height;
    depthDescription.MipLevels = 1;
    depthDescription.ArraySize = 1;
    depthDescription.Format = DXGI_FORMAT_D32_FLOAT;
    depthDescription.SampleDesc = colorDescription.SampleDesc;
    depthDescription.Usage = D3D11_USAGE_DEFAULT;
    depthDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    throwIfFailed(
        device_->CreateTexture2D(&depthDescription, nullptr, &target.depthTexture),
        "ID3D11Device::CreateTexture2D(depth)");
    throwIfFailed(
        device_->CreateDepthStencilView(target.depthTexture.Get(), nullptr, &target.depthView),
        "ID3D11Device::CreateDepthStencilView");

    return target;
}

void D3D11Renderer::createMirrorSwapchain(
    const HWND window,
    const std::uint32_t width,
    const std::uint32_t height,
    const DXGI_FORMAT sourceFormat) {
    ComPtr<IDXGIDevice> dxgiDevice;
    throwIfFailed(device_.As(&dxgiDevice), "Query IDXGIDevice");

    ComPtr<IDXGIAdapter> dxgiAdapter;
    throwIfFailed(dxgiDevice->GetAdapter(&dxgiAdapter), "IDXGIDevice::GetAdapter");

    ComPtr<IDXGIFactory1> factory;
    throwIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(&factory)), "IDXGIAdapter::GetParent");

    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferDesc.Width = width;
    description.BufferDesc.Height = height;
    description.BufferDesc.Format = linearFormat(sourceFormat);
    description.SampleDesc.Count = 1;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 2;
    description.OutputWindow = window;
    description.Windowed = TRUE;
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    throwIfFailed(
        factory->CreateSwapChain(device_.Get(), &description, &mirrorSwapchain_),
        "IDXGIFactory::CreateSwapChain(mirror)");
    factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
    mirrorWarningEmitted_ = false;
}

void D3D11Renderer::renderEye(
    const EyeRenderTarget& target,
    const XrPosef& pose,
    const XrFovf& fov,
    const float elapsedSeconds,
    const std::uint32_t eyeIndex) {
    const std::array leftClear{0.018F, 0.030F, 0.060F, 1.0F};
    const std::array rightClear{0.035F, 0.018F, 0.060F, 1.0F};
    const auto& clear = eyeIndex == 0 ? leftClear : rightClear;

    context_->ClearRenderTargetView(target.colorView.Get(), clear.data());
    context_->ClearDepthStencilView(target.depthView.Get(), D3D11_CLEAR_DEPTH, 1.0F, 0);

    ID3D11RenderTargetView* renderTarget = target.colorView.Get();
    context_->OMSetRenderTargets(1, &renderTarget, target.depthView.Get());

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(target.width);
    viewport.Height = static_cast<float>(target.height);
    viewport.MinDepth = 0.0F;
    viewport.MaxDepth = 1.0F;
    context_->RSSetViewports(1, &viewport);
    context_->RSSetState(rasterizerState_.Get());

    const UINT stride = sizeof(Vertex);
    constexpr UINT offset = 0;
    ID3D11Buffer* vertexBuffer = vertexBuffer_.Get();
    context_->IASetInputLayout(inputLayout_.Get());
    context_->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context_->IASetIndexBuffer(indexBuffer_.Get(), DXGI_FORMAT_R16_UINT, 0);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    using namespace DirectX;
    const XMMATRIX world = XMMatrixRotationY(elapsedSeconds * 0.65F)
        * XMMatrixRotationX(elapsedSeconds * 0.23F)
        * XMMatrixTranslation(0.0F, 0.0F, -2.0F);
    const XMMATRIX view = createViewMatrix(pose);
    const XMMATRIX projection = createProjectionMatrix(fov, 0.05F, 100.0F);

    XMFLOAT4X4 worldViewProjection{};
    XMStoreFloat4x4(&worldViewProjection, world * view * projection);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    throwIfFailed(
        context_->Map(constantBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped),
        "ID3D11DeviceContext::Map(scene constants)");
    std::memcpy(mapped.pData, &worldViewProjection, sizeof(worldViewProjection));
    context_->Unmap(constantBuffer_.Get(), 0);

    ID3D11Buffer* constantBuffer = constantBuffer_.Get();
    context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
    context_->VSSetConstantBuffers(0, 1, &constantBuffer);
    context_->PSSetShader(pixelShader_.Get(), nullptr, 0);
    context_->DrawIndexed(indexCount_, 0, 0);

    context_->OMSetRenderTargets(0, nullptr, nullptr);
}

void D3D11Renderer::renderVirtualScreenEye(
    const EyeRenderTarget& target,
    const XrPosef& eyePose,
    const XrFovf& fov,
    const XrPosef& anchorPose,
    ID3D11ShaderResourceView* captureView,
    const float captureAspectRatio,
    const VirtualScreenRenderSettings& settings) {
    if (captureView == nullptr) throw std::invalid_argument("Virtual Screen requires a captured texture.");
    updateVirtualScreenGeometry(captureAspectRatio, settings);

    constexpr std::array clear{0.004F, 0.006F, 0.012F, 1.0F};
    context_->ClearRenderTargetView(target.colorView.Get(), clear.data());
    context_->ClearDepthStencilView(target.depthView.Get(), D3D11_CLEAR_DEPTH, 1.0F, 0);

    ID3D11RenderTargetView* renderTarget = target.colorView.Get();
    context_->OMSetRenderTargets(1, &renderTarget, target.depthView.Get());
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(target.width);
    viewport.Height = static_cast<float>(target.height);
    viewport.MinDepth = 0.0F;
    viewport.MaxDepth = 1.0F;
    context_->RSSetViewports(1, &viewport);
    context_->RSSetState(rasterizerState_.Get());

    const UINT stride = sizeof(ScreenVertex);
    constexpr UINT offset = 0;
    ID3D11Buffer* vertexBuffer = screenVertexBuffer_.Get();
    context_->IASetInputLayout(screenInputLayout_.Get());
    context_->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context_->IASetIndexBuffer(screenIndexBuffer_.Get(), DXGI_FORMAT_R16_UINT, 0);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    using namespace DirectX;
    XMVECTOR anchorOrientation = XMVectorSet(
        anchorPose.orientation.x,
        anchorPose.orientation.y,
        anchorPose.orientation.z,
        anchorPose.orientation.w);
    anchorOrientation = XMQuaternionNormalize(anchorOrientation);
    const XMMATRIX world = XMMatrixRotationQuaternion(anchorOrientation)
        * XMMatrixTranslation(anchorPose.position.x, anchorPose.position.y, anchorPose.position.z);
    const XMMATRIX view = createViewMatrix(eyePose);
    const XMMATRIX projection = createProjectionMatrix(fov, 0.05F, 100.0F);

    XMFLOAT4X4 worldViewProjection{};
    XMStoreFloat4x4(&worldViewProjection, world * view * projection);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    throwIfFailed(
        context_->Map(constantBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped),
        "ID3D11DeviceContext::Map(Virtual Screen constants)");
    std::memcpy(mapped.pData, &worldViewProjection, sizeof(worldViewProjection));
    context_->Unmap(constantBuffer_.Get(), 0);

    ID3D11Buffer* constantBuffer = constantBuffer_.Get();
    ID3D11SamplerState* sampler = screenSampler_.Get();
    context_->VSSetShader(screenVertexShader_.Get(), nullptr, 0);
    context_->VSSetConstantBuffers(0, 1, &constantBuffer);
    context_->PSSetShader(screenPixelShader_.Get(), nullptr, 0);
    context_->PSSetShaderResources(0, 1, &captureView);
    context_->PSSetSamplers(0, 1, &sampler);
    context_->DrawIndexed(screenIndexCount_, 0, 0);

    ID3D11ShaderResourceView* noResource = nullptr;
    context_->PSSetShaderResources(0, 1, &noResource);
    context_->OMSetRenderTargets(0, nullptr, nullptr);
}

void D3D11Renderer::presentMirror(ID3D11Texture2D* sourceTexture) noexcept {
    if (mirrorSwapchain_ == nullptr || sourceTexture == nullptr) {
        return;
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT result = mirrorSwapchain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(result)) {
        if (!mirrorWarningEmitted_) {
            Logger::instance().warning(hresultMessage(result, "Mirror GetBuffer"));
            mirrorWarningEmitted_ = true;
        }
        return;
    }

    D3D11_TEXTURE2D_DESC sourceDescription{};
    D3D11_TEXTURE2D_DESC destinationDescription{};
    sourceTexture->GetDesc(&sourceDescription);
    backBuffer->GetDesc(&destinationDescription);

    const bool compatible = sourceDescription.Width == destinationDescription.Width
        && sourceDescription.Height == destinationDescription.Height
        && sourceDescription.SampleDesc.Count == destinationDescription.SampleDesc.Count
        && linearFormat(sourceDescription.Format) == linearFormat(destinationDescription.Format);
    if (!compatible) {
        if (!mirrorWarningEmitted_) {
            Logger::instance().warning("Mirror copy skipped because the OpenXR and DXGI textures are incompatible.");
            mirrorWarningEmitted_ = true;
        }
        return;
    }

    context_->CopyResource(backBuffer.Get(), sourceTexture);
    result = mirrorSwapchain_->Present(0, 0);
    if (FAILED(result) && !mirrorWarningEmitted_) {
        Logger::instance().warning(hresultMessage(result, "Mirror Present"));
        mirrorWarningEmitted_ = true;
    }
}

void D3D11Renderer::shutdownMirror() noexcept {
    mirrorSwapchain_.Reset();
}

ID3D11Device* D3D11Renderer::device() const noexcept {
    return device_.Get();
}

ID3D11DeviceContext* D3D11Renderer::context() const noexcept {
    return context_.Get();
}

D3D_FEATURE_LEVEL D3D11Renderer::featureLevel() const noexcept {
    return featureLevel_;
}

const std::string& D3D11Renderer::adapterName() const noexcept {
    return adapterName_;
}

void D3D11Renderer::findAdapter(const LUID& adapterLuid) {
    ComPtr<IDXGIFactory1> factory;
    throwIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "CreateDXGIFactory1");

    for (UINT index = 0;; ++index) {
        ComPtr<IDXGIAdapter1> candidate;
        const HRESULT result = factory->EnumAdapters1(index, &candidate);
        if (result == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        throwIfFailed(result, "IDXGIFactory1::EnumAdapters1");

        DXGI_ADAPTER_DESC1 description{};
        throwIfFailed(candidate->GetDesc1(&description), "IDXGIAdapter1::GetDesc1");
        if (description.AdapterLuid.HighPart == adapterLuid.HighPart
            && description.AdapterLuid.LowPart == adapterLuid.LowPart) {
            adapter_ = candidate;
            adapterName_ = wideToUtf8(description.Description);
            return;
        }
    }

    throw std::runtime_error("No DXGI adapter matched the LUID required by the OpenXR runtime.");
}

void D3D11Renderer::compileShaders() {
    constexpr std::string_view shaderSource = R"(
cbuffer SceneConstants : register(b0)
{
    row_major float4x4 worldViewProjection;
};

struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR0;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(float4(input.position, 1.0), worldViewProjection);
    output.color = input.color;
    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    return float4(input.color, 1.0);
}

Texture2D screenTexture : register(t0);
SamplerState screenSampler : register(s0);

struct ScreenVertexInput
{
    float3 position : POSITION;
    float2 textureCoordinate : TEXCOORD0;
};

struct ScreenVertexOutput
{
    float4 position : SV_POSITION;
    float2 textureCoordinate : TEXCOORD0;
};

ScreenVertexOutput ScreenVS(ScreenVertexInput input)
{
    ScreenVertexOutput output;
    output.position = mul(float4(input.position, 1.0), worldViewProjection);
    output.textureCoordinate = input.textureCoordinate;
    return output;
}

float4 ScreenPS(ScreenVertexOutput input) : SV_TARGET
{
    return screenTexture.Sample(screenSampler, input.textureCoordinate);
}
)";

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifndef NDEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> vertexBytecode;
    ComPtr<ID3DBlob> pixelBytecode;
    ComPtr<ID3DBlob> screenVertexBytecode;
    ComPtr<ID3DBlob> screenPixelBytecode;
    ComPtr<ID3DBlob> errors;

    HRESULT result = D3DCompile(
        shaderSource.data(),
        shaderSource.size(),
        "uxrb_test_scene.hlsl",
        nullptr,
        nullptr,
        "VSMain",
        "vs_5_0",
        flags,
        0,
        &vertexBytecode,
        &errors);
    if (FAILED(result)) {
        const std::string details = errors != nullptr
            ? std::string(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize())
            : hresultMessage(result, "D3DCompile(VSMain)");
        throw std::runtime_error("Vertex shader compilation failed: " + details);
    }

    errors.Reset();
    result = D3DCompile(
        shaderSource.data(),
        shaderSource.size(),
        "uxrb_test_scene.hlsl",
        nullptr,
        nullptr,
        "PSMain",
        "ps_5_0",
        flags,
        0,
        &pixelBytecode,
        &errors);
    if (FAILED(result)) {
        const std::string details = errors != nullptr
            ? std::string(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize())
            : hresultMessage(result, "D3DCompile(PSMain)");
        throw std::runtime_error("Pixel shader compilation failed: " + details);
    }

    errors.Reset();
    result = D3DCompile(
        shaderSource.data(),
        shaderSource.size(),
        "uxrb_virtual_screen.hlsl",
        nullptr,
        nullptr,
        "ScreenVS",
        "vs_5_0",
        flags,
        0,
        &screenVertexBytecode,
        &errors);
    if (FAILED(result)) {
        const std::string details = errors != nullptr
            ? std::string(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize())
            : hresultMessage(result, "D3DCompile(ScreenVS)");
        throw std::runtime_error("Virtual Screen vertex shader compilation failed: " + details);
    }

    errors.Reset();
    result = D3DCompile(
        shaderSource.data(),
        shaderSource.size(),
        "uxrb_virtual_screen.hlsl",
        nullptr,
        nullptr,
        "ScreenPS",
        "ps_5_0",
        flags,
        0,
        &screenPixelBytecode,
        &errors);
    if (FAILED(result)) {
        const std::string details = errors != nullptr
            ? std::string(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize())
            : hresultMessage(result, "D3DCompile(ScreenPS)");
        throw std::runtime_error("Virtual Screen pixel shader compilation failed: " + details);
    }

    throwIfFailed(
        device_->CreateVertexShader(
            vertexBytecode->GetBufferPointer(),
            vertexBytecode->GetBufferSize(),
            nullptr,
            &vertexShader_),
        "ID3D11Device::CreateVertexShader");
    throwIfFailed(
        device_->CreatePixelShader(
            pixelBytecode->GetBufferPointer(),
            pixelBytecode->GetBufferSize(),
            nullptr,
            &pixelShader_),
        "ID3D11Device::CreatePixelShader");
    throwIfFailed(
        device_->CreateVertexShader(
            screenVertexBytecode->GetBufferPointer(),
            screenVertexBytecode->GetBufferSize(),
            nullptr,
            &screenVertexShader_),
        "ID3D11Device::CreateVertexShader(Virtual Screen)");
    throwIfFailed(
        device_->CreatePixelShader(
            screenPixelBytecode->GetBufferPointer(),
            screenPixelBytecode->GetBufferSize(),
            nullptr,
            &screenPixelShader_),
        "ID3D11Device::CreatePixelShader(Virtual Screen)");

    constexpr std::array inputElements{
        D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                                 D3D11_INPUT_PER_VERTEX_DATA, 0},
        D3D11_INPUT_ELEMENT_DESC{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
                                 D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    throwIfFailed(
        device_->CreateInputLayout(
            inputElements.data(),
            static_cast<UINT>(inputElements.size()),
            vertexBytecode->GetBufferPointer(),
            vertexBytecode->GetBufferSize(),
            &inputLayout_),
        "ID3D11Device::CreateInputLayout");

    constexpr std::array screenInputElements{
        D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                                 D3D11_INPUT_PER_VERTEX_DATA, 0},
        D3D11_INPUT_ELEMENT_DESC{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
                                 D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    throwIfFailed(
        device_->CreateInputLayout(
            screenInputElements.data(),
            static_cast<UINT>(screenInputElements.size()),
            screenVertexBytecode->GetBufferPointer(),
            screenVertexBytecode->GetBufferSize(),
            &screenInputLayout_),
        "ID3D11Device::CreateInputLayout(Virtual Screen)");
}

void D3D11Renderer::createGeometry() {
    constexpr std::array vertices{
        Vertex{{-0.35F, -0.35F, -0.35F}, {0.15F, 0.65F, 1.00F}},
        Vertex{{-0.35F,  0.35F, -0.35F}, {0.20F, 1.00F, 0.75F}},
        Vertex{{ 0.35F,  0.35F, -0.35F}, {0.90F, 0.35F, 1.00F}},
        Vertex{{ 0.35F, -0.35F, -0.35F}, {1.00F, 0.55F, 0.20F}},
        Vertex{{-0.35F, -0.35F,  0.35F}, {0.20F, 0.45F, 1.00F}},
        Vertex{{-0.35F,  0.35F,  0.35F}, {0.30F, 1.00F, 0.40F}},
        Vertex{{ 0.35F,  0.35F,  0.35F}, {1.00F, 0.25F, 0.70F}},
        Vertex{{ 0.35F, -0.35F,  0.35F}, {1.00F, 0.85F, 0.15F}},
    };

    constexpr std::array<std::uint16_t, 36> indices{
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7,
    };
    indexCount_ = static_cast<std::uint32_t>(indices.size());

    D3D11_BUFFER_DESC vertexDescription{};
    vertexDescription.ByteWidth = static_cast<UINT>(sizeof(vertices));
    vertexDescription.Usage = D3D11_USAGE_IMMUTABLE;
    vertexDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    const D3D11_SUBRESOURCE_DATA vertexData{vertices.data(), 0, 0};
    throwIfFailed(
        device_->CreateBuffer(&vertexDescription, &vertexData, &vertexBuffer_),
        "ID3D11Device::CreateBuffer(vertices)");

    D3D11_BUFFER_DESC indexDescription{};
    indexDescription.ByteWidth = static_cast<UINT>(sizeof(indices));
    indexDescription.Usage = D3D11_USAGE_IMMUTABLE;
    indexDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
    const D3D11_SUBRESOURCE_DATA indexData{indices.data(), 0, 0};
    throwIfFailed(
        device_->CreateBuffer(&indexDescription, &indexData, &indexBuffer_),
        "ID3D11Device::CreateBuffer(indices)");

    D3D11_BUFFER_DESC constantDescription{};
    constantDescription.ByteWidth = sizeof(DirectX::XMFLOAT4X4);
    constantDescription.Usage = D3D11_USAGE_DYNAMIC;
    constantDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    throwIfFailed(
        device_->CreateBuffer(&constantDescription, nullptr, &constantBuffer_),
        "ID3D11Device::CreateBuffer(constants)");

    D3D11_RASTERIZER_DESC rasterizerDescription{};
    rasterizerDescription.FillMode = D3D11_FILL_SOLID;
    rasterizerDescription.CullMode = D3D11_CULL_NONE;
    rasterizerDescription.DepthClipEnable = TRUE;
    throwIfFailed(
        device_->CreateRasterizerState(&rasterizerDescription, &rasterizerState_),
        "ID3D11Device::CreateRasterizerState");

    D3D11_BUFFER_DESC screenVertexDescription{};
    screenVertexDescription.ByteWidth = static_cast<UINT>(
        sizeof(ScreenVertex) * (screenSegmentCount + 1U) * 2U);
    screenVertexDescription.Usage = D3D11_USAGE_DYNAMIC;
    screenVertexDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    screenVertexDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    throwIfFailed(
        device_->CreateBuffer(&screenVertexDescription, nullptr, &screenVertexBuffer_),
        "ID3D11Device::CreateBuffer(Virtual Screen vertices)");

    std::array<std::uint16_t, screenSegmentCount * 6U> screenIndices{};
    for (std::uint32_t segment = 0; segment < screenSegmentCount; ++segment) {
        const auto currentBottom = static_cast<std::uint16_t>(segment * 2U);
        const auto currentTop = static_cast<std::uint16_t>(currentBottom + 1U);
        const auto nextBottom = static_cast<std::uint16_t>(currentBottom + 2U);
        const auto nextTop = static_cast<std::uint16_t>(currentBottom + 3U);
        const std::size_t offset = static_cast<std::size_t>(segment) * 6U;
        screenIndices[offset + 0U] = currentBottom;
        screenIndices[offset + 1U] = currentTop;
        screenIndices[offset + 2U] = nextTop;
        screenIndices[offset + 3U] = currentBottom;
        screenIndices[offset + 4U] = nextTop;
        screenIndices[offset + 5U] = nextBottom;
    }
    screenIndexCount_ = static_cast<std::uint32_t>(screenIndices.size());
    D3D11_BUFFER_DESC screenIndexDescription{};
    screenIndexDescription.ByteWidth = static_cast<UINT>(sizeof(screenIndices));
    screenIndexDescription.Usage = D3D11_USAGE_IMMUTABLE;
    screenIndexDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
    const D3D11_SUBRESOURCE_DATA screenIndexData{screenIndices.data(), 0, 0};
    throwIfFailed(
        device_->CreateBuffer(&screenIndexDescription, &screenIndexData, &screenIndexBuffer_),
        "ID3D11Device::CreateBuffer(Virtual Screen indices)");

    D3D11_SAMPLER_DESC samplerDescription{};
    samplerDescription.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDescription.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDescription.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;
    throwIfFailed(
        device_->CreateSamplerState(&samplerDescription, &screenSampler_),
        "ID3D11Device::CreateSamplerState(Virtual Screen)");
}

void D3D11Renderer::updateVirtualScreenGeometry(
    const float aspectRatio,
    const VirtualScreenRenderSettings& settings) {
    const bool valid = std::isfinite(aspectRatio) && aspectRatio > 0.0F
        && std::isfinite(settings.widthMeters) && settings.widthMeters >= 0.5F && settings.widthMeters <= 10.0F
        && std::isfinite(settings.distanceMeters) && settings.distanceMeters >= 0.5F && settings.distanceMeters <= 10.0F
        && std::isfinite(settings.curvature) && settings.curvature >= 0.0F && settings.curvature <= 1.0F;
    if (!valid) throw std::invalid_argument("Virtual Screen geometry settings are outside their supported range.");

    constexpr float tolerance = 0.0001F;
    if (screenGeometryValid_
        && std::abs(screenAspectRatio_ - aspectRatio) < tolerance
        && std::abs(screenSettings_.widthMeters - settings.widthMeters) < tolerance
        && std::abs(screenSettings_.distanceMeters - settings.distanceMeters) < tolerance
        && std::abs(screenSettings_.curvature - settings.curvature) < tolerance) {
        return;
    }

    const float height = settings.widthMeters / aspectRatio;
    const float arc = settings.curvature * 1.6F;
    std::array<ScreenVertex, (screenSegmentCount + 1U) * 2U> vertices{};
    for (std::uint32_t segment = 0; segment <= screenSegmentCount; ++segment) {
        const float u = static_cast<float>(segment) / static_cast<float>(screenSegmentCount);
        const float normalizedX = u - 0.5F;
        float x = normalizedX * settings.widthMeters;
        float z = -settings.distanceMeters;
        if (arc > tolerance) {
            const float radius = settings.widthMeters / arc;
            const float angle = normalizedX * arc;
            x = std::sin(angle) * radius;
            z += radius * (1.0F - std::cos(angle));
        }
        vertices[segment * 2U] = ScreenVertex{{x, -height * 0.5F, z}, {u, 1.0F}};
        vertices[segment * 2U + 1U] = ScreenVertex{{x, height * 0.5F, z}, {u, 0.0F}};
    }

    D3D11_MAPPED_SUBRESOURCE mapped{};
    throwIfFailed(
        context_->Map(screenVertexBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped),
        "ID3D11DeviceContext::Map(Virtual Screen vertices)");
    std::memcpy(mapped.pData, vertices.data(), sizeof(vertices));
    context_->Unmap(screenVertexBuffer_.Get(), 0);

    screenAspectRatio_ = aspectRatio;
    screenSettings_ = settings;
    screenGeometryValid_ = true;
}

bool runD3D11SelfTest(std::string& failureReason) {
    try {
        ComPtr<IDXGIFactory1> factory;
        throwIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "CreateDXGIFactory1(self-test)");

        DXGI_ADAPTER_DESC1 selectedDescription{};
        bool foundAdapter = false;
        for (UINT index = 0;; ++index) {
            ComPtr<IDXGIAdapter1> adapter;
            const HRESULT result = factory->EnumAdapters1(index, &adapter);
            if (result == DXGI_ERROR_NOT_FOUND) {
                break;
            }
            throwIfFailed(result, "IDXGIFactory1::EnumAdapters1(self-test)");

            DXGI_ADAPTER_DESC1 description{};
            throwIfFailed(adapter->GetDesc1(&description), "IDXGIAdapter1::GetDesc1(self-test)");
            if ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) {
                selectedDescription = description;
                foundAdapter = true;
                break;
            }
        }
        if (!foundAdapter) {
            throw std::runtime_error("No hardware DXGI adapter is available for the D3D11 self-test.");
        }

        Logger::instance().debug("D3D11 self-test: creating renderer device.");
        D3D11Renderer renderer;
        renderer.initializeDevice(selectedDescription.AdapterLuid, D3D_FEATURE_LEVEL_11_0);
        Logger::instance().debug("D3D11 self-test: compiling both render pipelines.");
        renderer.createPipeline();

        D3D11_TEXTURE2D_DESC colorDescription{};
        colorDescription.Width = 64;
        colorDescription.Height = 64;
        colorDescription.MipLevels = 1;
        colorDescription.ArraySize = 1;
        colorDescription.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
        colorDescription.SampleDesc.Count = 1;
        colorDescription.Usage = D3D11_USAGE_DEFAULT;
        colorDescription.BindFlags = D3D11_BIND_RENDER_TARGET;

        ComPtr<ID3D11Texture2D> colorTexture;
        throwIfFailed(
            renderer.device()->CreateTexture2D(&colorDescription, nullptr, &colorTexture),
            "ID3D11Device::CreateTexture2D(self-test color)");
        const EyeRenderTarget target = renderer.createEyeRenderTarget(
            colorTexture.Get(),
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

        const XrPosef pose{
            XrQuaternionf{0.0F, 0.0F, 0.0F, 1.0F},
            XrVector3f{0.0F, 0.0F, 0.0F},
        };
        const XrFovf fov{-0.75F, 0.75F, 0.75F, -0.75F};
        Logger::instance().debug("D3D11 self-test: rendering the diagnostic cube.");
        renderer.renderEye(target, pose, fov, 0.0F, 0);

        constexpr std::array<std::uint32_t, 4> pixels{
            0xFF002FA7U, 0xFFF7F7F8U,
            0xFFF7F7F8U, 0xFF002FA7U,
        };
        D3D11_TEXTURE2D_DESC captureDescription{};
        captureDescription.Width = 2;
        captureDescription.Height = 2;
        captureDescription.MipLevels = 1;
        captureDescription.ArraySize = 1;
        captureDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        captureDescription.SampleDesc.Count = 1;
        captureDescription.Usage = D3D11_USAGE_IMMUTABLE;
        captureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        const D3D11_SUBRESOURCE_DATA captureData{pixels.data(), 2U * sizeof(std::uint32_t), 0};
        ComPtr<ID3D11Texture2D> captureTexture;
        throwIfFailed(
            renderer.device()->CreateTexture2D(&captureDescription, &captureData, &captureTexture),
            "ID3D11Device::CreateTexture2D(self-test capture)");
        ComPtr<ID3D11ShaderResourceView> captureView;
        throwIfFailed(
            renderer.device()->CreateShaderResourceView(captureTexture.Get(), nullptr, &captureView),
            "ID3D11Device::CreateShaderResourceView(self-test capture)");
        Logger::instance().debug("D3D11 self-test: rendering the textured Virtual Screen mesh.");
        renderer.renderVirtualScreenEye(
            target,
            pose,
            fov,
            pose,
            captureView.Get(),
            16.0F / 9.0F,
            VirtualScreenRenderSettings{});
        renderer.context()->Flush();
        Logger::instance().debug("D3D11 self-test: both render pipelines completed.");

        failureReason.clear();
        return true;
    } catch (const std::exception& exception) {
        failureReason = exception.what();
        return false;
    }
}

} // namespace uxrb
