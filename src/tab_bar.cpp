#include "tab_bar.h"

#include <algorithm>

namespace {

constexpr int kBarHeight = 34;
constexpr int kTabMaxWidth = 240;
constexpr int kTabPadding = 12;
constexpr int kCloseSize = 18;
constexpr int kNewTabWidth = 34;

bool Contains(const Rect& rect, const Point point) {
    return point.x >= rect.left && point.x < rect.right &&
           point.y >= rect.top && point.y < rect.bottom;
}

bool Empty(const Rect& rect) { return rect.right <= rect.left || rect.bottom <= rect.top; }

}  // namespace

void TabBar::SetExtent(const Size extent, const double dpiScale) {
    extent_ = extent;
    dpiScale_ = dpiScale;
}

int TabBar::PreferredHeight() const { return ui::Scale(dpiScale_, kBarHeight); }

void TabBar::Invalidate() {
    if (onInvalidate) onInvalidate();
}

void TabBar::SetTabs(std::vector<std::wstring> titles, const int activeIndex) {
    if (titles == titles_ && activeIndex == active_) return;

    titles_ = std::move(titles);
    active_ = activeIndex;
    if (hotTab_ >= static_cast<int>(titles_.size())) hotTab_ = -1;
    if (hotClose_ >= static_cast<int>(titles_.size())) hotClose_ = -1;
    Invalidate();
}

std::vector<TabBar::TabLayout> TabBar::LayoutTabs() const {
    std::vector<TabLayout> layouts;
    if (titles_.empty()) return layouts;

    const int newTabWidth = ui::Scale(dpiScale_, kNewTabWidth);
    const int available = (std::max)(0, static_cast<int>(extent_.cx) - newTabWidth);
    const int maxWidth = ui::Scale(dpiScale_, kTabMaxWidth);

    const int closeSize = ui::Scale(dpiScale_, kCloseSize);
    const int padding = ui::Scale(dpiScale_, kTabPadding);
    const int closeTop = (static_cast<int>(extent_.cy) - closeSize) / 2;

    const int width = (std::max)(
        1, (std::min)(maxWidth, available / static_cast<int>(titles_.size())));
    const bool hasClose = width >= closeSize + padding * 2;

    int x = 0;
    for (std::size_t i = 0; i < titles_.size(); ++i) {
        TabLayout layout;
        layout.bounds = Rect{x, 0, x + width, static_cast<int>(extent_.cy)};
        if (hasClose)
            layout.close = Rect{layout.bounds.right - padding - closeSize + padding / 3, closeTop,
                                layout.bounds.right - padding + padding / 3, closeTop + closeSize};
        layouts.push_back(layout);
        x += width;
    }

    return layouts;
}

Rect TabBar::NewTabButton() const {
    const auto layouts = LayoutTabs();
    const int left = layouts.empty() ? 0 : layouts.back().bounds.right;
    const int width = ui::Scale(dpiScale_, kNewTabWidth);
    return Rect{left, 0, (std::min)(left + width, static_cast<int>(extent_.cx)),
                static_cast<int>(extent_.cy)};
}

void TabBar::Render(Renderer& renderer, Canvas& canvas) {
    const Rect client{0, 0, extent_.cx, extent_.cy};
    renderer.FillRect(RectF::FromRect(client), ui::Argb(ui::kChromeDeep));

    const auto layouts = LayoutTabs();
    const float radius = ui::ScaleF(dpiScale_, 7.0f);
    const int padding = ui::Scale(dpiScale_, kTabPadding);

    for (std::size_t i = 0; i < layouts.size(); ++i) {
        const auto index = static_cast<int>(i);
        const Rect& bounds = layouts[i].bounds;
        const bool isActive = index == active_;

        RectF tab = RectF::FromRect(bounds);
        tab.left += 1.0f;
        tab.right -= 1.0f;
        tab.top += ui::ScaleF(dpiScale_, 4.0f);
        tab.bottom += radius;

        if (tab.IsEmpty()) continue;

        if (isActive)
            ui::FillRoundRect(renderer, tab, radius, ui::kPanel);
        else if (index == hotTab_)
            ui::FillRoundRect(renderer, tab, radius, ui::kHover);

        if (isActive) {
            const RectF accent{tab.left + radius * 0.5f, tab.top,
                               tab.right - radius * 0.5f, tab.top + ui::ScaleF(dpiScale_, 2.5f)};
            ui::FillRoundRect(renderer, accent, 1.0f, ui::kAccent);
        }

        const Rect& close = layouts[i].close;
        const bool hasClose = !Empty(close);

        Rect textRect = bounds;
        textRect.left += padding;
        textRect.right = (hasClose ? close.left : bounds.right) - padding / 2;
        ui::Text(canvas, textRect, titles_[i], isActive ? ui::kText : ui::kTextDim,
                 ui::TextAlign::left, ui::Font(dpiScale_, isActive));

        if (!hasClose) continue;

        const RectF closeBox = RectF::FromRect(close);
        if (index == hotClose_)
            ui::FillRoundRect(renderer, closeBox, ui::ScaleF(dpiScale_, 4.0f), ui::kPressed);

        const float inset = ui::ScaleF(dpiScale_, 5.5f);
        const Color glyph = index == hotClose_ ? ui::kDanger
                               : isActive         ? ui::kText
                                                  : ui::kTextDim;
        ui::Line(renderer, {closeBox.left + inset, closeBox.top + inset},
                 {closeBox.right - inset, closeBox.bottom - inset}, glyph,
                 ui::ScaleF(dpiScale_, 1.3f));
        ui::Line(renderer, {closeBox.right - inset, closeBox.top + inset},
                 {closeBox.left + inset, closeBox.bottom - inset}, glyph,
                 ui::ScaleF(dpiScale_, 1.3f));
    }

    const Rect newTab = NewTabButton();
    const RectF newTabBox = RectF::FromRect(newTab).Inflated(-ui::ScaleF(dpiScale_, 6.0f));
    if (hotNewTab_) ui::FillRoundRect(renderer, newTabBox, ui::ScaleF(dpiScale_, 5.0f), ui::kHover);

    const Vec2 centre = newTabBox.Centre();
    const float arm = ui::ScaleF(dpiScale_, 5.0f);
    ui::Line(renderer, {centre.x - arm, centre.y}, {centre.x + arm, centre.y}, ui::kTextDim,
             ui::ScaleF(dpiScale_, 1.4f));
    ui::Line(renderer, {centre.x, centre.y - arm}, {centre.x, centre.y + arm}, ui::kTextDim,
             ui::ScaleF(dpiScale_, 1.4f));
}

void TabBar::OnMouseMove(const Point point) {
    const auto layouts = LayoutTabs();
    int hotTab = -1;
    int hotClose = -1;

    for (std::size_t i = 0; i < layouts.size(); ++i) {
        if (!Contains(layouts[i].bounds, point)) continue;
        hotTab = static_cast<int>(i);
        if (Contains(layouts[i].close, point)) hotClose = hotTab;
        break;
    }

    const bool hotNewTab = Contains(NewTabButton(), point);
    if (hotTab == hotTab_ && hotClose == hotClose_ && hotNewTab == hotNewTab_) return;

    hotTab_ = hotTab;
    hotClose_ = hotClose;
    hotNewTab_ = hotNewTab;
    Invalidate();
}

void TabBar::OnMouseLeave() {
    tracking_ = false;
    if (hotTab_ == -1 && hotClose_ == -1 && !hotNewTab_) return;
    hotTab_ = -1;
    hotClose_ = -1;
    hotNewTab_ = false;
    Invalidate();
}

void TabBar::OnLeftDown(const Point point) {
    pressedClose_ = -1;
    pressedNewTab_ = false;

    const auto layouts = LayoutTabs();
    for (std::size_t i = 0; i < layouts.size(); ++i) {
        if (!Contains(layouts[i].bounds, point)) continue;

        if (Contains(layouts[i].close, point)) {
            pressedClose_ = static_cast<int>(i);
            if (onCapture) onCapture(true);
            return;
        }

        if (onSelect) onSelect(static_cast<int>(i));
        return;
    }

    if (Contains(NewTabButton(), point)) {
        pressedNewTab_ = true;
        if (onCapture) onCapture(true);
    }
}

void TabBar::OnLeftUp(const Point point) {
    const int pressedClose = pressedClose_;
    const bool pressedNewTab = pressedNewTab_;
    pressedClose_ = -1;
    pressedNewTab_ = false;

    if (onCapture) onCapture(false);

    if (pressedClose >= 0) {
        const auto layouts = LayoutTabs();
        if (pressedClose < static_cast<int>(layouts.size()) &&
            Contains(layouts[static_cast<std::size_t>(pressedClose)].close, point) && onClose)
            onClose(pressedClose);
        return;
    }

    if (pressedNewTab && Contains(NewTabButton(), point) && onNewTab) onNewTab();
}

void TabBar::OnMiddleUp(const Point point) {
    const auto layouts = LayoutTabs();
    for (std::size_t i = 0; i < layouts.size(); ++i) {
        if (Contains(layouts[i].bounds, point) && onClose) {
            onClose(static_cast<int>(i));
            break;
        }
    }
}
