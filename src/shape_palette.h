// The shape palette: a grid of hand-drawn previews. Clicking arms a tool,
// dragging drops a ready-made object onto the drawing.

#pragma once

#include "ui.h"
#include "view.h"

#include <functional>

class ShapePalette {
public:
    static void RegisterWindowClass(HINSTANCE instance);
    static HWND Create(HWND parent, int controlId);
    static ShapePalette* FromHandle(HWND window);

    void SetActive(Tool tool);

    std::function<void(Tool)> onSelect;
    // Screen coordinates of the drop point.
    std::function<void(ShapeKind, POINT)> onDrop;

private:
    explicit ShapePalette(HWND window) : hwnd_(window) {}

    static LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    RECT CellBounds(int index) const;
    int HitTest(POINT point) const;
    int Columns() const;

    void Render(Renderer& renderer, HDC dc);
    void OnPaint();
    void OnMouseMove(POINT point);
    void OnLeftDown(POINT point);
    void OnLeftUp(POINT point);
    void OnMouseLeave();
    void Invalidate() { ::InvalidateRect(hwnd_, nullptr, FALSE); }

    HWND hwnd_ = nullptr;
    ui::Surface surface_;
    Tool active_;
    int hot_ = -1;
    int pressed_ = -1;
    bool dragging_ = false;
    POINT dragStart_{};
    bool tracking_ = false;
};
