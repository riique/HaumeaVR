#pragma once

#include "uxrb/d3d11_renderer.hpp"
#include "uxrb/unity_transport.hpp"

#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <string>

namespace uxrb {

class UnitySharedTextureSource final {
public:
    void initialize(
        ID3D11Device* device,
        unity_transport::Mapping* transport,
        std::uint32_t expectedProcessId = 0);
    void reset() noexcept;

    [[nodiscard]] bool refresh() noexcept;
    [[nodiscard]] bool copyEye(
        ID3D11DeviceContext* context,
        const EyeRenderTarget& target,
        std::uint32_t eyeIndex) noexcept;
    [[nodiscard]] bool ready(std::uint32_t eyeIndex) const noexcept;
    [[nodiscard]] std::uint32_t producerProcessId() const noexcept;
    [[nodiscard]] const std::string& lastError() const noexcept;

private:
    struct Eye final {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Microsoft::WRL::ComPtr<IDXGIKeyedMutex> keyedMutex;
        std::uint32_t generation{};
        std::uint32_t width{};
        std::uint32_t height{};
        DXGI_FORMAT format{DXGI_FORMAT_UNKNOWN};
        std::wstring name;
    };

    [[nodiscard]] bool openEye(
        std::uint32_t eyeIndex,
        const unity_transport::EyeTexture& state,
        std::uint32_t processId) noexcept;
    static void clearEye(Eye& eye) noexcept;
    static bool compatibleFormats(DXGI_FORMAT source, DXGI_FORMAT destination) noexcept;

    Microsoft::WRL::ComPtr<ID3D11Device1> device_;
    unity_transport::Mapping* transport_{};
    std::array<Eye, unity_transport::eyeCount> eyes_{};
    std::uint32_t expectedProcessId_{};
    std::uint32_t producerProcessId_{};
    std::string lastError_;
};

[[nodiscard]] bool runUnitySharedTextureSourceSelfTest(std::string& failureReason);

}  // namespace uxrb
