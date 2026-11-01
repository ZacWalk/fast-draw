// The hand-drawn document tab strip.  It is an app-owned control model and
// renderer; libdesktop owns the containing window and delivers the input.

#pragma once

#include "ui.h"

#include <functional>
#include <string>
#include <vector>

class TabBar {
public:
    TabBar() = default;

    // The model is laid out in its own local surface.
    void SetExtent(Size extent, double dpiScale);

    // Only repaints when something actually changed, so an idle-time refresh
    // costs nothing and never flickers.
    void SetTabs(std::vector<std::wstring> titles, int activeIndex);
    int PreferredHeight() const;

    void Render(Renderer& renderer, Canvas& canvas);
    void OnMouseMove(Point point);
    void OnMouseLeave();
    void OnLeftDown(Point point);
    void OnLeftUp(Point point);
    void OnMiddleUp(Point point);

    std::function<void(int)> onSelect;
    std::function<void(int)> onClose;
    std::function<void()> onNewTab;
    std::function<void()> onInvalidate;
    std::function<void(bool)> onCapture;

private:
    struct TabLayout {
        Rect bounds{};
        Rect close{};
    };

    std::vector<TabLayout> LayoutTabs() const;
    Rect NewTabButton() const;
    void Invalidate();

    Size extent_{};
    double dpiScale_ = 1.0;
    std::vector<std::wstring> titles_;
    int active_ = -1;
    int hotTab_ = -1;
    int hotClose_ = -1;
    bool hotNewTab_ = false;
    int pressedClose_ = -1;
    bool pressedNewTab_ = false;
    bool tracking_ = false;
};
