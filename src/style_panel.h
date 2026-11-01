// The hand-drawn style inspector.  It is rendered into an app-owned surface;
// libdesktop delivers the containing window's input and owns its lifetime.

#pragma once

#include "style.h"
#include "ui.h"

#include <array>
#include <functional>
#include <optional>

class StylePanel {
public:
    StylePanel() = default;

    void SetExtent(Size extent, double dpiScale);
    void SetStyle(const Style& style, bool hasSelection);
    int PreferredHeight() const;

    void Render(Renderer& renderer, Canvas& canvas);
    void OnMouseMove(Point point);
    void OnMouseLeave();
    void OnLeftDown(Point point);
    void OnLeftUp(Point point);

    std::function<void(const Style&)> onChange;
    std::function<std::optional<Color>(Color)> onPickColour;
    std::function<void()> onInvalidate;
    std::function<void(bool)> onCapture;

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

    Rect& Bounds(Item item) { return bounds_[static_cast<std::size_t>(item)]; }
    const Rect& Bounds(Item item) const { return bounds_[static_cast<std::size_t>(item)]; }

    void Layout();
    int HitTest(Point point) const;
    void Activate(Item item);
    void Publish();
    void Invalidate();

    Size extent_{};
    double dpiScale_ = 1.0;
    Style style_;
    bool hasSelection_ = false;
    int hot_ = -1;
    int pressed_ = -1;
    bool tracking_ = false;

    std::array<Rect, static_cast<std::size_t>(Item::Count)> bounds_{};
    std::array<Rect, 3> headers_{};
    Rect widthValue_{};
    Rect sizeValue_{};
    int contentHeight_ = 0;
};
