#pragma once

#include "uxrb/openxr_platform.hpp"

#include <DirectXMath.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <cstdint>
#include <string>

namespace uxrb {

struct EyeRenderTarget {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> colorTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> colorView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthTexture;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthView;
    std::uint32_t width{0};
    std::uint32_t height{0};
};

struct VirtualScreenRenderSettings {
    float widthMeters{3.2F};
    float distanceMeters{2.2F};
    float curvature{0.35F};
};

class D3D11Renderer final {
public:
    void initializeDevice(const LUID& adapterLuid, D3D_FEATURE_LEVEL minimumFeatureLevel);
    void createPipeline();

    [[nodiscard]] EyeRenderTarget createEyeRenderTarget(
        ID3D11Texture2D* colorTexture,
        DXGI_FORMAT colorFormat) const;

    void createMirrorSwapchain(
        HWND window,
        std::uint32_t width,
        std::uint32_t height,
        DXGI_FORMAT sourceFormat);

    void renderEye(
        const EyeRenderTarget& target,
        const XrPosef& pose,
        const XrFovf& fov,
        float elapsedSeconds,
        std::uint32_t eyeIndex);

    void renderVirtualScreenEye(
        const EyeRenderTarget& target,
        const XrPosef& eyePose,
        const XrFovf& fov,
        const XrPosef& anchorPose,
        ID3D11ShaderResourceView* captureView,
        float captureAspectRatio,
        const VirtualScreenRenderSettings& settings);

    void presentMirror(ID3D11Texture2D* sourceTexture) noexcept;
    void shutdownMirror() noexcept;

    [[nodiscard]] ID3D11Device* device() const noexcept;
    [[nodiscard]] ID3D11DeviceContext* context() const noexcept;
    [[nodiscard]] D3D_FEATURE_LEVEL featureLevel() const noexcept;
    [[nodiscard]] const std::string& adapterName() const noexcept;

private:
    void findAdapter(const LUID& adapterLuid);
    void compileShaders();
    void createGeometry();
    void updateVirtualScreenGeometry(float aspectRatio, const VirtualScreenRenderSettings& settings);

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter_;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> mirrorSwapchain_;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState_;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> screenVertexShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> screenPixelShader_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> screenInputLayout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> screenVertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> screenIndexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> screenSampler_;

    D3D_FEATURE_LEVEL featureLevel_{D3D_FEATURE_LEVEL_11_0};
    std::string adapterName_;
    std::uint32_t indexCount_{0};
    std::uint32_t screenIndexCount_{0};
    float screenAspectRatio_{0.0F};
    VirtualScreenRenderSettings screenSettings_{};
    bool screenGeometryValid_{false};
    bool mirrorWarningEmitted_{false};
};

[[nodiscard]] bool runD3D11SelfTest(std::string& failureReason);

} // namespace uxrb
