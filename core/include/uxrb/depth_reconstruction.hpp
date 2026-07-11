#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <cstdint>
#include <string>

namespace uxrb {

struct DepthReconstructionSettings final {
    float maximumDisparityPixels{24.0F};
    float convergenceDepth{0.55F};
    float depthGamma{1.0F};
    bool reversedDepth{false};
};

class DepthReconstructor final {
public:
    void initialize(ID3D11Device* device);

    void reconstruct(
        ID3D11DeviceContext* context,
        ID3D11ShaderResourceView* color,
        ID3D11ShaderResourceView* normalizedLinearDepth,
        ID3D11UnorderedAccessView* leftOutput,
        ID3D11UnorderedAccessView* rightOutput,
        std::uint32_t width,
        std::uint32_t height,
        const DepthReconstructionSettings& settings);

    void visualizeDepth(
        ID3D11DeviceContext* context,
        ID3D11ShaderResourceView* normalizedLinearDepth,
        ID3D11UnorderedAccessView* output,
        std::uint32_t width,
        std::uint32_t height,
        bool reversedDepth);

private:
    struct alignas(16) Constants final {
        std::uint32_t width{};
        std::uint32_t height{};
        float maximumDisparityPixels{};
        float convergenceDepth{};
        float depthGamma{};
        std::uint32_t reversedDepth{};
        float padding[2]{};
    };

    void updateConstants(
        ID3D11DeviceContext* context,
        std::uint32_t width,
        std::uint32_t height,
        const DepthReconstructionSettings& settings);
    static void clearBindings(ID3D11DeviceContext* context);

    Microsoft::WRL::ComPtr<ID3D11ComputeShader> reconstructShader_;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> visualizeShader_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constants_;
};

[[nodiscard]] bool runDepthReconstructionSelfTest(std::string& failureReason);

}  // namespace uxrb
