// A hand-drawn tab strip: one tab per open document, each with a close button.

#pragma once

#include "ui.h"

#include <functional>
#include <string>
#include <vector>

class TabBar {
public:
    static void RegisterWindowClass(HINSTANCE instance);
    static HWND Create(HWND parent, int controlId);
    static TabBar* FromHandle(HWND window);

    // Only repaints when something actually changed, so an idle-time refresh
    // costs nothing and never flickers.
    void SetTabs(std::vector<std::wstring> titles, int activeIndex);
    int PreferredHeight() const;

    std::function<void(int)> onSelect;
    std::function<void(int)> onClose;
    std::function<void()> onNewTab;

private:
    struct TabLayout {
        RECT bounds{};
        RECT close{};
    };

    explicit TabBar(HWND window) : hwnd_(window) {}

    static LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    std::vector<TabLayout> LayoutTabs() const;
    RECT NewTabButton() const;

    void Render(Renderer& renderer, HDC dc);
    void OnPaint();
    void OnMouseMove(POINT point);
    void OnMouseLeave();
    void OnLeftDown(POINT point);
    void OnLeftUp(POINT point);
    void Invalidate() { ::InvalidateRect(hwnd_, nullptr, FALSE); }

    HWND hwnd_ = nullptr;
    ui::Surface surface_;
    std::vector<std::wstring> titles_;
    int active_ = -1;
    int hotTab_ = -1;
    int hotClose_ = -1;
    bool hotNewTab_ = false;
    int pressedClose_ = -1;
    bool pressedNewTab_ = false;
    bool tracking_ = false;
};
