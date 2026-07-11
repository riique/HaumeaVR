#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <cstdint>
#include <string>

namespace uxrb {

struct DesktopCaptureInfo {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::int32_t desktopLeft{0};
    std::int32_t desktopTop{0};
    std::string outputName;
    bool windowCapture{false};
};

class DesktopCapture final {
public:
    DesktopCapture() = default;
    ~DesktopCapture();

    DesktopCapture(const DesktopCapture&) = delete;
    DesktopCapture& operator=(const DesktopCapture&) = delete;

    void initialize(ID3D11Device* device, ID3D11DeviceContext* context, HWND targetWindow = nullptr);
    [[nodiscard]] bool captureFrame(std::uint32_t timeoutMilliseconds = 0);
    void reset() noexcept;

    [[nodiscard]] ID3D11ShaderResourceView* shaderResourceView() const noexcept;
    [[nodiscard]] bool hasFrame() const noexcept;
    [[nodiscard]] const DesktopCaptureInfo& info() const noexcept;

private:
    void selectOutput();
    void createDuplication();
    [[nodiscard]] bool updateSourceRectangle();
    void ensureCaptureTexture(DXGI_FORMAT format, std::uint32_t width, std::uint32_t height);

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGIOutput1> output_;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> duplication_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> captureTexture_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> captureView_;

    DXGI_OUTPUT_DESC outputDescription_{};
    RECT sourceRectangle_{};
    HWND targetWindow_{nullptr};
    DesktopCaptureInfo info_{};
    bool hasFrame_{false};
};

[[nodiscard]] bool runDesktopCaptureSelfTest(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    std::string& failureReason);

[[nodiscard]] bool runDesktopCaptureSelfTest(std::string& failureReason);

} // namespace uxrb
