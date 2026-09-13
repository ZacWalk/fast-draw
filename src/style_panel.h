// The style inspector: fill, gradient, line and text settings for the current
// selection, or the defaults new objects are created with.

#pragma once

#include "style.h"
#include "ui.h"

#include <array>
#include <functional>

class StylePanel {
public:
    static void RegisterWindowClass(HINSTANCE instance);
    static HWND Create(HWND parent, int controlId);
    static StylePanel* FromHandle(HWND window);

    void SetStyle(const Style& style, bool hasSelection);
    int PreferredHeight() const;

    std::function<void(const Style&)> onChange;

private:
    enum class Item : int {
        FillColour,
        FillToggle,
        GradientToggle,
        GradientColour,
        Direction0,
        Direction1,
        Direction2,
        Direction3,
        LineColour,
        LineToggle,
        WidthDown,
        WidthUp,
        Dash0,
        Dash1,
        Dash2,
        Dash3,
        RoundJoins,
        ArrowStart,
        ArrowEnd,
        TextColour,
        SizeDown,
        SizeUp,
        Bold,
        Italic,
        Count,
    };

    explicit StylePanel(HWND window) : hwnd_(window) {}

    static LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    RECT& Bounds(Item item) { return bounds_[static_cast<std::size_t>(item)]; }
    const RECT& Bounds(Item item) const { return bounds_[static_cast<std::size_t>(item)]; }

    void Layout();
    void Render(Renderer& renderer, HDC dc);
    void OnPaint();
    void OnMouseMove(POINT point);
    void OnMouseLeave();
    void OnLeftUp(POINT point);
    int HitTest(POINT point) const;
    void Activate(Item item);
    void Publish();
    void Invalidate() { ::InvalidateRect(hwnd_, nullptr, FALSE); }

    HWND hwnd_ = nullptr;
    ui::Surface surface_;
    Style style_;
    bool hasSelection_ = false;
    int hot_ = -1;
    bool tracking_ = false;

    std::array<RECT, static_cast<std::size_t>(Item::Count)> bounds_{};
    std::array<RECT, 3> headers_{};
    RECT widthValue_{};
    RECT sizeValue_{};
    int contentHeight_ = 0;
};
