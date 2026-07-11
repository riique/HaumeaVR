#include "uxrb/mirror_window.hpp"

#include <stdexcept>
#include <string>

namespace uxrb {
namespace {

constexpr wchar_t windowClassName[] = L"UniversalXRBridgeMirrorWindow";

} // namespace

MirrorWindow::~MirrorWindow() {
    if (handle_ != nullptr && IsWindow(handle_) != FALSE) {
        DestroyWindow(handle_);
    }
}

void MirrorWindow::create(const std::uint32_t clientWidth, const std::uint32_t clientHeight) {
    const HINSTANCE instance = GetModuleHandleW(nullptr);

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &MirrorWindow::windowProcedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    windowClass.lpszClassName = windowClassName;

    if (RegisterClassExW(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        throw std::runtime_error("Could not register the mirror window class.");
    }

    RECT rectangle{0, 0, static_cast<LONG>(clientWidth), static_cast<LONG>(clientHeight)};
    constexpr DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRectEx(&rectangle, style, FALSE, 0);

    handle_ = CreateWindowExW(
        0,
        windowClassName,
        L"HaumeaVR - OpenXR Mirror",
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rectangle.right - rectangle.left,
        rectangle.bottom - rectangle.top,
        nullptr,
        nullptr,
        instance,
        this);

    if (handle_ == nullptr) {
        throw std::runtime_error("Could not create the mirror window.");
    }

    ShowWindow(handle_, SW_SHOWDEFAULT);
    UpdateWindow(handle_);
}

void MirrorWindow::pumpMessages() {
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != FALSE) {
        if (message.message == WM_QUIT) {
            closed_ = true;
            continue;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

void MirrorWindow::setTitle(const std::wstring_view title) const {
    if (handle_ == nullptr) {
        return;
    }
    const std::wstring ownedTitle(title);
    SetWindowTextW(handle_, ownedTitle.c_str());
}

HWND MirrorWindow::handle() const noexcept {
    return handle_;
}

bool MirrorWindow::closed() const noexcept {
    return closed_;
}

LRESULT CALLBACK MirrorWindow::windowProcedure(
    const HWND window,
    const UINT message,
    const WPARAM wParam,
    const LPARAM lParam) {
    auto* self = reinterpret_cast<MirrorWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<MirrorWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }

    switch (message) {
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        if (self != nullptr) {
            self->closed_ = true;
            self->handle_ = nullptr;
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

} // namespace uxrb
