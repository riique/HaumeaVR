#include "uxrb/depth_reconstruction.hpp"

#include <d3dcompiler.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string_view>

using Microsoft::WRL::ComPtr;

namespace uxrb {
namespace {

constexpr std::string_view shaderSource = R"(
cbuffer ReconstructionConstants : register(b0)
{
    uint outputWidth;
    uint outputHeight;
    float maximumDisparityPixels;
    float convergenceDepth;
    float depthGamma;
    uint reversedDepth;
    float2 padding;
};

Texture2D<float4> sourceColor : register(t0);
Texture2D<float> sourceDepth : register(t1);
RWTexture2D<float4> leftOutput : register(u0);
RWTexture2D<float4> rightOutput : register(u1);

float normalizedDepth(uint2 position)
{
    float value = saturate(sourceDepth.Load(int3(position, 0)));
    if (reversedDepth != 0) value = 1.0 - value;
    return pow(max(value, 0.00001), max(depthGamma, 0.05));
}

float4 sampleReprojected(uint2 target, float eyeSign)
{
    float depth = normalizedDepth(target);
    float disparity = clamp(
        (convergenceDepth - depth) * maximumDisparityPixels,
        -maximumDisparityPixels,
        maximumDisparityPixels);
    int sourceX = clamp(
        int(round(float(target.x) + eyeSign * disparity * 0.5)),
        0,
        int(outputWidth) - 1);

    // A small edge-aware search reduces disocclusion streaks while retaining
    // deterministic, bounded work per pixel.
    uint2 source = uint2(sourceX, target.y);
    float sourceZ = normalizedDepth(source);
    if (abs(sourceZ - depth) > 0.12)
    {
        int direction = eyeSign > 0.0 ? 1 : -1;
        [unroll]
        for (int offset = 1; offset <= 3; ++offset)
        {
            int candidateX = clamp(sourceX + direction * offset, 0, int(outputWidth) - 1);
            uint2 candidate = uint2(candidateX, target.y);
            float candidateZ = normalizedDepth(candidate);
            if (abs(candidateZ - depth) < abs(sourceZ - depth))
            {
                source = candidate;
                sourceZ = candidateZ;
            }
        }
    }
    return sourceColor.Load(int3(source, 0));
}

[numthreads(8, 8, 1)]
void ReconstructCS(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= outputWidth || dispatchThreadId.y >= outputHeight) return;
    uint2 position = dispatchThreadId.xy;
    leftOutput[position] = sampleReprojected(position, 1.0);
    rightOutput[position] = sampleReprojected(position, -1.0);
}

[numthreads(8, 8, 1)]
void VisualizeCS(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= outputWidth || dispatchThreadId.y >= outputHeight) return;
    uint2 position = dispatchThreadId.xy;
    float value = normalizedDepth(position);
    float3 heat = saturate(float3(1.5 - abs(4.0 * value - 3.0),
                                  1.5 - abs(4.0 * value - 2.0),
                                  1.5 - abs(4.0 * value - 1.0)));
    leftOutput[position] = float4(heat, 1.0);
}
)";

void checkHr(const HRESULT result, const char* operation) {
    if (FAILED(result)) {
        throw std::runtime_error(std::string(operation) + " failed with HRESULT " + std::to_string(result) + '.');
    }
}

ComPtr<ID3DBlob> compileComputeShader(const char* entryPoint) {
    ComPtr<ID3DBlob> bytecode;
    ComPtr<ID3DBlob> errors;
    const HRESULT result = D3DCompile(
        shaderSource.data(),
        shaderSource.size(),
        "uxrb_depth_reconstruction.hlsl",
        nullptr,
        nullptr,
        entryPoint,
        "cs_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        &bytecode,
        &errors);
    if (FAILED(result)) {
        const std::string details = errors == nullptr
            ? "no shader compiler diagnostics"
            : std::string(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());
        throw std::runtime_error(std::string(entryPoint) + " compilation failed: " + details);
    }
    return bytecode;
}

}  // namespace

void DepthReconstructor::initialize(ID3D11Device* device) {
    if (device == nullptr) throw std::invalid_argument("Depth reconstruction requires a D3D11 device.");
    const auto reconstructBytecode = compileComputeShader("ReconstructCS");
    const auto visualizeBytecode = compileComputeShader("VisualizeCS");
    checkHr(
        device->CreateComputeShader(
            reconstructBytecode->GetBufferPointer(),
            reconstructBytecode->GetBufferSize(),
            nullptr,
            &reconstructShader_),
        "ID3D11Device::CreateComputeShader(ReconstructCS)");
    checkHr(
        device->CreateComputeShader(
            visualizeBytecode->GetBufferPointer(),
            visualizeBytecode->GetBufferSize(),
            nullptr,
            &visualizeShader_),
        "ID3D11Device::CreateComputeShader(VisualizeCS)");

    D3D11_BUFFER_DESC description{};
    description.ByteWidth = sizeof(Constants);
    description.Usage = D3D11_USAGE_DYNAMIC;
    description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    checkHr(device->CreateBuffer(&description, nullptr, &constants_), "ID3D11Device::CreateBuffer(depth constants)");
}

void DepthReconstructor::reconstruct(
    ID3D11DeviceContext* context,
    ID3D11ShaderResourceView* color,
    ID3D11ShaderResourceView* normalizedLinearDepth,
    ID3D11UnorderedAccessView* leftOutput,
    ID3D11UnorderedAccessView* rightOutput,
    const std::uint32_t width,
    const std::uint32_t height,
    const DepthReconstructionSettings& settings) {
    if (context == nullptr || color == nullptr || normalizedLinearDepth == nullptr ||
        leftOutput == nullptr || rightOutput == nullptr || width == 0 || height == 0) {
        throw std::invalid_argument("Depth reconstruction received a missing resource or zero dimensions.");
    }
    if (!std::isfinite(settings.maximumDisparityPixels) || settings.maximumDisparityPixels < 0.0F ||
        settings.maximumDisparityPixels > 256.0F || !std::isfinite(settings.convergenceDepth) ||
        settings.convergenceDepth < 0.0F || settings.convergenceDepth > 1.0F ||
        !std::isfinite(settings.depthGamma) || settings.depthGamma < 0.05F || settings.depthGamma > 8.0F) {
        throw std::invalid_argument("Depth reconstruction settings are outside their safe range.");
    }
    if (reconstructShader_ == nullptr || constants_ == nullptr) {
        throw std::logic_error("DepthReconstructor::initialize must be called first.");
    }

    updateConstants(context, width, height, settings);
    ID3D11ShaderResourceView* resources[]{color, normalizedLinearDepth};
    ID3D11UnorderedAccessView* outputs[]{leftOutput, rightOutput};
    ID3D11Buffer* constants[]{constants_.Get()};
    context->CSSetShader(reconstructShader_.Get(), nullptr, 0);
    context->CSSetShaderResources(0, 2, resources);
    context->CSSetUnorderedAccessViews(0, 2, outputs, nullptr);
    context->CSSetConstantBuffers(0, 1, constants);
    context->Dispatch((width + 7U) / 8U, (height + 7U) / 8U, 1);
    clearBindings(context);
}

void DepthReconstructor::visualizeDepth(
    ID3D11DeviceContext* context,
    ID3D11ShaderResourceView* normalizedLinearDepth,
    ID3D11UnorderedAccessView* output,
    const std::uint32_t width,
    const std::uint32_t height,
    const bool reversedDepth) {
    if (context == nullptr || normalizedLinearDepth == nullptr || output == nullptr || width == 0 || height == 0) {
        throw std::invalid_argument("Depth visualization received a missing resource or zero dimensions.");
    }
    if (visualizeShader_ == nullptr || constants_ == nullptr) {
        throw std::logic_error("DepthReconstructor::initialize must be called first.");
    }

    DepthReconstructionSettings settings{};
    settings.reversedDepth = reversedDepth;
    updateConstants(context, width, height, settings);
    ID3D11ShaderResourceView* resources[]{nullptr, normalizedLinearDepth};
    ID3D11UnorderedAccessView* outputs[]{output};
    ID3D11Buffer* constants[]{constants_.Get()};
    context->CSSetShader(visualizeShader_.Get(), nullptr, 0);
    context->CSSetShaderResources(0, 2, resources);
    context->CSSetUnorderedAccessViews(0, 1, outputs, nullptr);
    context->CSSetConstantBuffers(0, 1, constants);
    context->Dispatch((width + 7U) / 8U, (height + 7U) / 8U, 1);
    clearBindings(context);
}

void DepthReconstructor::updateConstants(
    ID3D11DeviceContext* context,
    const std::uint32_t width,
    const std::uint32_t height,
    const DepthReconstructionSettings& settings) {
    D3D11_MAPPED_SUBRESOURCE mapped{};
    checkHr(context->Map(constants_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped), "Map(depth constants)");
    const Constants values{
        width,
        height,
        settings.maximumDisparityPixels,
        settings.convergenceDepth,
        settings.depthGamma,
        settings.reversedDepth ? 1U : 0U,
        {},
    };
    std::memcpy(mapped.pData, &values, sizeof(values));
    context->Unmap(constants_.Get(), 0);
}

void DepthReconstructor::clearBindings(ID3D11DeviceContext* context) {
    constexpr std::array<ID3D11ShaderResourceView*, 2> nullResources{};
    constexpr std::array<ID3D11UnorderedAccessView*, 2> nullOutputs{};
    ID3D11Buffer* nullConstant{};
    context->CSSetShaderResources(0, static_cast<UINT>(nullResources.size()), nullResources.data());
    context->CSSetUnorderedAccessViews(0, static_cast<UINT>(nullOutputs.size()), nullOutputs.data(), nullptr);
    context->CSSetConstantBuffers(0, 1, &nullConstant);
    context->CSSetShader(nullptr, nullptr, 0);
}

bool runDepthReconstructionSelfTest(std::string& failureReason) {
    try {
        constexpr std::uint32_t width = 32;
        constexpr std::uint32_t height = 8;
        constexpr std::size_t pixelCount = static_cast<std::size_t>(width) * height;

        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        D3D_FEATURE_LEVEL selectedLevel{};
        checkHr(
            D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_WARP,
                nullptr,
                0,
                nullptr,
                0,
                D3D11_SDK_VERSION,
                &device,
                &selectedLevel,
                &context),
            "D3D11CreateDevice(WARP depth self-test)");

        std::array<std::uint32_t, pixelCount> colorPixels{};
        std::array<float, pixelCount> depthPixels{};
        for (std::uint32_t y = 0; y < height; ++y) {
            for (std::uint32_t x = 0; x < width; ++x) {
                const std::size_t index = static_cast<std::size_t>(y) * width + x;
                colorPixels[index] = 0xFF000000U | (x * 7U) | ((255U - x * 7U) << 8U) | ((x * 3U) << 16U);
                depthPixels[index] = x < width / 2U ? 0.2F : 0.85F;
            }
        }

        D3D11_TEXTURE2D_DESC colorDescription{};
        colorDescription.Width = width;
        colorDescription.Height = height;
        colorDescription.MipLevels = 1;
        colorDescription.ArraySize = 1;
        colorDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        colorDescription.SampleDesc.Count = 1;
        colorDescription.Usage = D3D11_USAGE_DEFAULT;
        colorDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA colorData{colorPixels.data(), width * sizeof(std::uint32_t), 0};
        ComPtr<ID3D11Texture2D> color;
        checkHr(device->CreateTexture2D(&colorDescription, &colorData, &color), "CreateTexture2D(test color)");
        ComPtr<ID3D11ShaderResourceView> colorView;
        checkHr(device->CreateShaderResourceView(color.Get(), nullptr, &colorView), "CreateSRV(test color)");

        D3D11_TEXTURE2D_DESC depthDescription = colorDescription;
        depthDescription.Format = DXGI_FORMAT_R32_FLOAT;
        D3D11_SUBRESOURCE_DATA depthData{depthPixels.data(), width * sizeof(float), 0};
        ComPtr<ID3D11Texture2D> depth;
        checkHr(device->CreateTexture2D(&depthDescription, &depthData, &depth), "CreateTexture2D(test depth)");
        ComPtr<ID3D11ShaderResourceView> depthView;
        checkHr(device->CreateShaderResourceView(depth.Get(), nullptr, &depthView), "CreateSRV(test depth)");

        D3D11_TEXTURE2D_DESC outputDescription = colorDescription;
        outputDescription.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        ComPtr<ID3D11Texture2D> left;
        ComPtr<ID3D11Texture2D> right;
        ComPtr<ID3D11Texture2D> visualized;
        checkHr(device->CreateTexture2D(&outputDescription, nullptr, &left), "CreateTexture2D(left output)");
        checkHr(device->CreateTexture2D(&outputDescription, nullptr, &right), "CreateTexture2D(right output)");
        checkHr(device->CreateTexture2D(&outputDescription, nullptr, &visualized), "CreateTexture2D(depth visualization)");
        ComPtr<ID3D11UnorderedAccessView> leftView;
        ComPtr<ID3D11UnorderedAccessView> rightView;
        ComPtr<ID3D11UnorderedAccessView> visualizedView;
        checkHr(device->CreateUnorderedAccessView(left.Get(), nullptr, &leftView), "CreateUAV(left output)");
        checkHr(device->CreateUnorderedAccessView(right.Get(), nullptr, &rightView), "CreateUAV(right output)");
        checkHr(device->CreateUnorderedAccessView(visualized.Get(), nullptr, &visualizedView), "CreateUAV(depth visualization)");

        DepthReconstructor reconstructor;
        reconstructor.initialize(device.Get());
        DepthReconstructionSettings settings{};
        settings.maximumDisparityPixels = 12.0F;
        reconstructor.reconstruct(
            context.Get(), colorView.Get(), depthView.Get(), leftView.Get(), rightView.Get(), width, height, settings);
        reconstructor.visualizeDepth(context.Get(), depthView.Get(), visualizedView.Get(), width, height, false);

        D3D11_TEXTURE2D_DESC stagingDescription = colorDescription;
        stagingDescription.Usage = D3D11_USAGE_STAGING;
        stagingDescription.BindFlags = 0;
        stagingDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> stagingLeft;
        ComPtr<ID3D11Texture2D> stagingRight;
        ComPtr<ID3D11Texture2D> stagingVisualized;
        checkHr(device->CreateTexture2D(&stagingDescription, nullptr, &stagingLeft), "CreateTexture2D(staging left)");
        checkHr(device->CreateTexture2D(&stagingDescription, nullptr, &stagingRight), "CreateTexture2D(staging right)");
        checkHr(device->CreateTexture2D(&stagingDescription, nullptr, &stagingVisualized), "CreateTexture2D(staging visualization)");
        context->CopyResource(stagingLeft.Get(), left.Get());
        context->CopyResource(stagingRight.Get(), right.Get());
        context->CopyResource(stagingVisualized.Get(), visualized.Get());

        D3D11_MAPPED_SUBRESOURCE mappedLeft{};
        D3D11_MAPPED_SUBRESOURCE mappedRight{};
        D3D11_MAPPED_SUBRESOURCE mappedVisualized{};
        checkHr(context->Map(stagingLeft.Get(), 0, D3D11_MAP_READ, 0, &mappedLeft), "Map(staging left)");
        checkHr(context->Map(stagingRight.Get(), 0, D3D11_MAP_READ, 0, &mappedRight), "Map(staging right)");
        checkHr(context->Map(stagingVisualized.Get(), 0, D3D11_MAP_READ, 0, &mappedVisualized), "Map(staging visualization)");
        const auto* leftRow = static_cast<const std::uint32_t*>(mappedLeft.pData);
        const auto* rightRow = static_cast<const std::uint32_t*>(mappedRight.pData);
        const auto* visualizedRow = static_cast<const std::uint32_t*>(mappedVisualized.pData);
        const bool stereoDiffers = !std::equal(leftRow, leftRow + width, rightRow);
        const bool depthDiffers = visualizedRow[2] != visualizedRow[width - 2U];
        context->Unmap(stagingVisualized.Get(), 0);
        context->Unmap(stagingRight.Get(), 0);
        context->Unmap(stagingLeft.Get(), 0);

        if (!stereoDiffers) throw std::runtime_error("Synthetic depth produced identical left and right rows.");
        if (!depthDiffers) throw std::runtime_error("Depth visualization did not distinguish near and far values.");
        failureReason.clear();
        return true;
    } catch (const std::exception& exception) {
        failureReason = exception.what();
        return false;
    }
}

}  // namespace uxrb
