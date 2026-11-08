// The hand-drawn shape palette.  It is an app-owned control model; the
// libdesktop UI frame supplies the containing window and input dispatch.

#pragma once

#include "ui.h"
#include "view.h"

#include <functional>

class ShapePalette {
public:
    ShapePalette() = default;

    void SetExtent(Size extent, double dpiScale);
    void SetActive(Tool tool);

    void Render(Renderer& renderer, Canvas& canvas);
    void OnMouseMove(Point point);
    void OnMouseLeave();
    void OnLeftDown(Point point);
    void OnLeftUp(Point point);

    std::function<void(Tool)> onSelect;
    // The point is local to the palette surface.
    std::function<void(ShapeKind, Point)> onDrop;
    std::function<void()> onInvalidate;
    std::function<void(bool)> onCapture;
    std::function<void()> onDragCursor;

private:
    Rect CellBounds(int index) const;
    int HitTest(Point point) const;
    int Columns() const;
    void Invalidate();

    Size extent_{};
    double dpiScale_ = 1.0;
    Tool active_;
    int hot_ = -1;
    int pressed_ = -1;
    bool dragging_ = false;
    Point dragStart_{};
    bool tracking_ = false;
};
