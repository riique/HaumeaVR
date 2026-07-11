#pragma once

#include <Windows.h>

#include <cstdint>
#include <string_view>

namespace uxrb {

class MirrorWindow final {
public:
    MirrorWindow() = default;
    ~MirrorWindow();

    MirrorWindow(const MirrorWindow&) = delete;
    MirrorWindow& operator=(const MirrorWindow&) = delete;

    void create(std::uint32_t clientWidth, std::uint32_t clientHeight);
    void pumpMessages();
    void setTitle(std::wstring_view title) const;

    [[nodiscard]] HWND handle() const noexcept;
    [[nodiscard]] bool closed() const noexcept;

private:
    static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    HWND handle_{nullptr};
    bool closed_{false};
};

} // namespace uxrb

