#include "uxrb/d3d11_intercept.h"

#include <Windows.h>
#include <TlHelp32.h>
#include <d3d11.h>
#include <detours.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

using Microsoft::WRL::ComPtr;

namespace {

using PresentFunction = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);

std::mutex stateMutex;
PresentFunction originalPresent{};
uxrb::intercept::D3D11InterceptStatus status{};
std::wstring lastError;
std::uint32_t lastLoggedDepthWidth{};
std::uint32_t lastLoggedDepthHeight{};
std::uint32_t lastLoggedDepthFormat{};

constexpr std::array<std::wstring_view, 12> blockedModuleFragments{
    L"easyanticheat",
    L"easyanticheat_eos",
    L"battleye",
    L"beservice",
    L"vgc",
    L"vgk",
    L"faceit",
    L"equ8",
    L"mhyprot",
    L"xigncode",
    L"nprotect",
    L"gameguard",
};

std::wstring lower(std::wstring value) {
    std::ranges::transform(value, value.begin(), [](const wchar_t character) {
        return static_cast<wchar_t>(towlower(character));
    });
    return value;
}

bool optInEnabled() {
    std::array<wchar_t, 16> value{};
    const DWORD length = GetEnvironmentVariableW(
        L"UXRB_D3D11_HOOK_ENABLE",
        value.data(),
        static_cast<DWORD>(value.size()));
    if (length == 0 || length >= value.size()) return false;
    return _wcsicmp(value.data(), L"1") == 0 || _wcsicmp(value.data(), L"true") == 0;
}

bool antiCheatModuleLoaded(std::wstring& moduleName) {
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    BOOL hasEntry = Module32FirstW(snapshot, &entry);
    while (hasEntry != FALSE) {
        const std::wstring candidate = lower(entry.szModule);
        for (const auto fragment : blockedModuleFragments) {
            if (candidate.find(fragment) != std::wstring::npos) {
                moduleName = entry.szModule;
                CloseHandle(snapshot);
                return true;
            }
        }
        hasEntry = Module32NextW(snapshot, &entry);
    }
    CloseHandle(snapshot);
    return false;
}

LRESULT CALLBACK bootstrapWindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    return DefWindowProcW(window, message, wparam, lparam);
}

class BootstrapWindow final {
public:
    BootstrapWindow() {
        instance_ = GetModuleHandleW(nullptr);
        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.lpfnWndProc = bootstrapWindowProcedure;
        windowClass.hInstance = instance_;
        windowClass.lpszClassName = className;
        atom_ = RegisterClassExW(&windowClass);
        if (atom_ == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return;
        window_ = CreateWindowExW(
            0,
            className,
            L"UXRB D3D11 bootstrap",
            WS_OVERLAPPED,
            0,
            0,
            8,
            8,
            nullptr,
            nullptr,
            instance_,
            nullptr);
    }

    ~BootstrapWindow() {
        if (window_ != nullptr) DestroyWindow(window_);
        if (atom_ != 0) UnregisterClassW(className, instance_);
    }

    [[nodiscard]] HWND get() const noexcept { return window_; }

private:
    static constexpr wchar_t className[] = L"UXRB_D3D11_BOOTSTRAP_WINDOW";
    HINSTANCE instance_{};
    ATOM atom_{};
    HWND window_{};
};

PresentFunction findPresentFunction() {
    BootstrapWindow window;
    if (window.get() == nullptr) return nullptr;

    DXGI_SWAP_CHAIN_DESC swapchainDescription{};
    swapchainDescription.BufferDesc.Width = 8;
    swapchainDescription.BufferDesc.Height = 8;
    swapchainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchainDescription.SampleDesc.Count = 1;
    swapchainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDescription.BufferCount = 1;
    swapchainDescription.OutputWindow = window.get();
    swapchainDescription.Windowed = TRUE;
    swapchainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ComPtr<IDXGISwapChain> swapchain;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL selectedLevel{};
    constexpr std::array featureLevels{D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        featureLevels.data(),
        static_cast<UINT>(featureLevels.size()),
        D3D11_SDK_VERSION,
        &swapchainDescription,
        &swapchain,
        &device,
        &selectedLevel,
        &context);
    if (FAILED(result)) {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            featureLevels.data(),
            static_cast<UINT>(featureLevels.size()),
            D3D11_SDK_VERSION,
            &swapchainDescription,
            &swapchain,
            &device,
            &selectedLevel,
            &context);
    }
    if (FAILED(result) || swapchain == nullptr) return nullptr;

    auto** vtable = *reinterpret_cast<void***>(swapchain.Get());
    return reinterpret_cast<PresentFunction>(vtable[8]);
}

float scoreDepthCandidate(const D3D11_TEXTURE2D_DESC& color, const D3D11_TEXTURE2D_DESC& depth) {
    float value = 0.0F;
    if (depth.Width == color.Width) value += 0.35F;
    if (depth.Height == color.Height) value += 0.35F;
    if (depth.SampleDesc.Count == color.SampleDesc.Count) value += 0.1F;
    if ((depth.BindFlags & D3D11_BIND_DEPTH_STENCIL) != 0) value += 0.2F;
    return value;
}

void writeObservationLog(const uxrb::intercept::D3D11InterceptStatus& snapshot) {
    std::array<wchar_t, 32768> path{};
    const DWORD length = GetEnvironmentVariableW(
        L"UXRB_D3D11_HOOK_LOG",
        path.data(),
        static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size()) return;

    std::ofstream output(std::filesystem::path(path.data()), std::ios::app);
    if (!output) return;
    output << "{\"event\":\"present-observation\",\"presentCount\":" << snapshot.presentCount
           << ",\"backBuffer\":{\"width\":" << snapshot.backBufferWidth
           << ",\"height\":" << snapshot.backBufferHeight
           << ",\"format\":" << snapshot.backBufferFormat << "}"
           << ",\"depth\":{\"available\":" << snapshot.depthAvailable
           << ",\"width\":" << snapshot.depthWidth
           << ",\"height\":" << snapshot.depthHeight
           << ",\"format\":" << snapshot.depthFormat
           << ",\"score\":" << snapshot.depthCandidateScore << "}}\n";
}

void observePresent(IDXGISwapChain* swapchain) noexcept {
    try {
        ComPtr<ID3D11Texture2D> color;
        if (FAILED(swapchain->GetBuffer(0, IID_PPV_ARGS(&color)))) return;
        D3D11_TEXTURE2D_DESC colorDescription{};
        color->GetDesc(&colorDescription);

        ComPtr<ID3D11Device> device;
        swapchain->GetDevice(IID_PPV_ARGS(&device));
        if (device == nullptr) return;
        ComPtr<ID3D11DeviceContext> context;
        device->GetImmediateContext(&context);

        ComPtr<ID3D11RenderTargetView> renderTarget;
        ComPtr<ID3D11DepthStencilView> depthView;
        ID3D11RenderTargetView* rawRenderTarget{};
        ID3D11DepthStencilView* rawDepthView{};
        context->OMGetRenderTargets(1, &rawRenderTarget, &rawDepthView);
        renderTarget.Attach(rawRenderTarget);
        depthView.Attach(rawDepthView);

        uxrb::intercept::D3D11InterceptStatus snapshot{};
        bool logChanged = false;
        {
            std::scoped_lock lock(stateMutex);
            ++status.presentCount;
            status.backBufferWidth = colorDescription.Width;
            status.backBufferHeight = colorDescription.Height;
            status.backBufferFormat = static_cast<std::uint32_t>(colorDescription.Format);
            status.depthAvailable = 0;
            status.depthWidth = 0;
            status.depthHeight = 0;
            status.depthFormat = 0;
            status.depthCandidateScore = 0.0F;

            if (depthView != nullptr) {
                ComPtr<ID3D11Resource> depthResource;
                depthView->GetResource(&depthResource);
                ComPtr<ID3D11Texture2D> depthTexture;
                if (SUCCEEDED(depthResource.As(&depthTexture))) {
                    D3D11_TEXTURE2D_DESC depthDescription{};
                    depthTexture->GetDesc(&depthDescription);
                    status.depthAvailable = 1;
                    status.depthWidth = depthDescription.Width;
                    status.depthHeight = depthDescription.Height;
                    status.depthFormat = static_cast<std::uint32_t>(depthDescription.Format);
                    status.depthCandidateScore = scoreDepthCandidate(colorDescription, depthDescription);
                }
            }

            logChanged = status.presentCount == 1 || status.depthWidth != lastLoggedDepthWidth ||
                status.depthHeight != lastLoggedDepthHeight || status.depthFormat != lastLoggedDepthFormat;
            if (logChanged) {
                lastLoggedDepthWidth = status.depthWidth;
                lastLoggedDepthHeight = status.depthHeight;
                lastLoggedDepthFormat = status.depthFormat;
            }
            snapshot = status;
        }
        if (logChanged) writeObservationLog(snapshot);
    } catch (...) {
        // A diagnostic hook must never take the host process down.
    }
}

HRESULT STDMETHODCALLTYPE interceptedPresent(IDXGISwapChain* swapchain, const UINT syncInterval, const UINT flags) {
    observePresent(swapchain);
    return originalPresent(swapchain, syncInterval, flags);
}

void setLastError(std::wstring value) {
    std::scoped_lock lock(stateMutex);
    lastError = std::move(value);
}

}  // namespace

extern "C" std::uint32_t UXRB_D3D11_HOOK_API UXRB_D3D11_HOOK_CALL UxrbD3D11Hook_GetApiVersion() {
    return uxrb::intercept::kD3D11InterceptApiVersion;
}

extern "C" uxrb::intercept::InstallResult UXRB_D3D11_HOOK_API UXRB_D3D11_HOOK_CALL
UxrbD3D11Hook_InstallForCurrentProcess() {
    {
        std::scoped_lock lock(stateMutex);
        if (status.installed != 0) return uxrb::intercept::InstallResult::alreadyInstalled;
    }
    if (!optInEnabled()) {
        setLastError(L"UXRB_D3D11_HOOK_ENABLE=1 is required for current-process observation.");
        return uxrb::intercept::InstallResult::optInRequired;
    }

    std::wstring blockedModule;
    if (antiCheatModuleLoaded(blockedModule)) {
        {
            std::scoped_lock lock(stateMutex);
            status.antiCheatDenied = 1;
        }
        setLastError(L"D3D11 observation was hard-blocked because an anti-cheat module is loaded: " + blockedModule);
        return uxrb::intercept::InstallResult::antiCheatDetected;
    }

    originalPresent = findPresentFunction();
    if (originalPresent == nullptr) {
        setLastError(L"A temporary D3D11 swapchain could not be created to locate Present.");
        return uxrb::intercept::InstallResult::graphicsBootstrapFailed;
    }

    LONG result = DetourTransactionBegin();
    if (result == NO_ERROR) result = DetourUpdateThread(GetCurrentThread());
    if (result == NO_ERROR) {
        result = DetourAttach(reinterpret_cast<PVOID*>(&originalPresent), interceptedPresent);
    }
    if (result == NO_ERROR) result = DetourTransactionCommit();
    else DetourTransactionAbort();
    if (result != NO_ERROR) {
        originalPresent = nullptr;
        setLastError(L"Microsoft Detours could not attach the IDXGISwapChain::Present observer.");
        return uxrb::intercept::InstallResult::detourFailed;
    }

    {
        std::scoped_lock lock(stateMutex);
        status.installed = 1;
        lastError.clear();
    }
    return uxrb::intercept::InstallResult::installed;
}

extern "C" std::int32_t UXRB_D3D11_HOOK_API UXRB_D3D11_HOOK_CALL UxrbD3D11Hook_Uninstall() {
    {
        std::scoped_lock lock(stateMutex);
        if (status.installed == 0) return 1;
    }

    LONG result = DetourTransactionBegin();
    if (result == NO_ERROR) result = DetourUpdateThread(GetCurrentThread());
    if (result == NO_ERROR) {
        result = DetourDetach(reinterpret_cast<PVOID*>(&originalPresent), interceptedPresent);
    }
    if (result == NO_ERROR) result = DetourTransactionCommit();
    else DetourTransactionAbort();
    if (result != NO_ERROR) {
        setLastError(L"Microsoft Detours could not detach the IDXGISwapChain::Present observer.");
        return 0;
    }

    std::scoped_lock lock(stateMutex);
    status.installed = 0;
    originalPresent = nullptr;
    return 1;
}

extern "C" std::int32_t UXRB_D3D11_HOOK_API UXRB_D3D11_HOOK_CALL UxrbD3D11Hook_GetStatus(
    uxrb::intercept::D3D11InterceptStatus* destination) {
    if (destination == nullptr || destination->structSize != sizeof(uxrb::intercept::D3D11InterceptStatus)) return 0;
    std::scoped_lock lock(stateMutex);
    *destination = status;
    return 1;
}

extern "C" std::int32_t UXRB_D3D11_HOOK_API UXRB_D3D11_HOOK_CALL UxrbD3D11Hook_CopyLastError(
    wchar_t* destination,
    const std::int32_t destinationCharacterCount) {
    if (destination == nullptr || destinationCharacterCount <= 0) return 0;
    std::scoped_lock lock(stateMutex);
    if (lastError.size() + 1U > static_cast<std::size_t>(destinationCharacterCount)) {
        destination[0] = L'\0';
        return 0;
    }
    std::wmemcpy(destination, lastError.c_str(), lastError.size() + 1U);
    return static_cast<std::int32_t>(lastError.size());
}

BOOL APIENTRY DllMain(HMODULE module, const DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(module);
    return TRUE;
}
