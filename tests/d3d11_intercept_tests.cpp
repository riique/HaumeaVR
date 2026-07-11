#include "uxrb/d3d11_intercept.h"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <array>
#include <cstdlib>
#include <iostream>

using Microsoft::WRL::ComPtr;

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "D3D11 intercept test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    return DefWindowProcW(window, message, wparam, lparam);
}

}  // namespace

int main() {
    require(UxrbD3D11Hook_GetApiVersion() == uxrb::intercept::kD3D11InterceptApiVersion, "API version mismatch");
    SetEnvironmentVariableW(L"UXRB_D3D11_HOOK_ENABLE", nullptr);
    require(
        UxrbD3D11Hook_InstallForCurrentProcess() == uxrb::intercept::InstallResult::optInRequired,
        "hook installed without explicit opt-in");

    SetEnvironmentVariableW(L"UXRB_D3D11_HOOK_ENABLE", L"1");
    require(
        UxrbD3D11Hook_InstallForCurrentProcess() == uxrb::intercept::InstallResult::installed,
        "opt-in hook installation failed");

    const HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = L"UXRB_D3D11_INTERCEPT_TEST";
    RegisterClassExW(&windowClass);
    const HWND window = CreateWindowExW(
        0, windowClass.lpszClassName, L"test", WS_OVERLAPPED, 0, 0, 64, 64, nullptr, nullptr, instance, nullptr);
    require(window != nullptr, "test window creation failed");

    DXGI_SWAP_CHAIN_DESC swapchainDescription{};
    swapchainDescription.BufferDesc.Width = 64;
    swapchainDescription.BufferDesc.Height = 64;
    swapchainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchainDescription.SampleDesc.Count = 1;
    swapchainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDescription.BufferCount = 1;
    swapchainDescription.OutputWindow = window;
    swapchainDescription.Windowed = TRUE;
    swapchainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ComPtr<IDXGISwapChain> swapchain;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL selectedLevel{};
    constexpr std::array levels{D3D_FEATURE_LEVEL_11_0};
    const HRESULT createResult = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_WARP,
        nullptr,
        0,
        levels.data(),
        static_cast<UINT>(levels.size()),
        D3D11_SDK_VERSION,
        &swapchainDescription,
        &swapchain,
        &device,
        &selectedLevel,
        &context);
    require(SUCCEEDED(createResult), "test D3D11 swapchain creation failed");

    ComPtr<ID3D11Texture2D> color;
    require(SUCCEEDED(swapchain->GetBuffer(0, IID_PPV_ARGS(&color))), "back buffer query failed");
    ComPtr<ID3D11RenderTargetView> colorView;
    require(SUCCEEDED(device->CreateRenderTargetView(color.Get(), nullptr, &colorView)), "color RTV creation failed");

    D3D11_TEXTURE2D_DESC depthDescription{};
    depthDescription.Width = 64;
    depthDescription.Height = 64;
    depthDescription.MipLevels = 1;
    depthDescription.ArraySize = 1;
    depthDescription.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDescription.SampleDesc.Count = 1;
    depthDescription.Usage = D3D11_USAGE_DEFAULT;
    depthDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ComPtr<ID3D11Texture2D> depth;
    require(SUCCEEDED(device->CreateTexture2D(&depthDescription, nullptr, &depth)), "depth texture creation failed");
    ComPtr<ID3D11DepthStencilView> depthView;
    require(SUCCEEDED(device->CreateDepthStencilView(depth.Get(), nullptr, &depthView)), "depth DSV creation failed");

    ID3D11RenderTargetView* rawColorView = colorView.Get();
    context->OMSetRenderTargets(1, &rawColorView, depthView.Get());
    context->ClearDepthStencilView(depthView.Get(), D3D11_CLEAR_DEPTH, 1.0F, 0);
    require(SUCCEEDED(swapchain->Present(0, DXGI_PRESENT_TEST)), "test Present failed");

    uxrb::intercept::D3D11InterceptStatus observed{};
    require(UxrbD3D11Hook_GetStatus(&observed) == 1, "status query failed");
    require(observed.presentCount >= 1, "Present was not observed");
    require(observed.backBufferWidth == 64 && observed.backBufferHeight == 64, "back buffer dimensions were not captured");
    require(observed.depthAvailable == 1, "bound depth buffer was not detected");
    require(observed.depthCandidateScore > 0.9F, "depth candidate score is unexpectedly low");
    require(UxrbD3D11Hook_Uninstall() == 1, "hook uninstall failed");

    DestroyWindow(window);
    UnregisterClassW(windowClass.lpszClassName, instance);
    SetEnvironmentVariableW(L"UXRB_D3D11_HOOK_ENABLE", nullptr);
    std::cout << "D3D11 opt-in Present and depth observation test passed.\n";
    return EXIT_SUCCESS;
}
