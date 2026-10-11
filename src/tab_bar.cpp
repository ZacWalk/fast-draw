#include "tab_bar.h"

#include <windowsx.h>

#include <algorithm>

namespace {

constexpr wchar_t kClassName[] = L"FastDrawTabBar";

constexpr int kBarHeight = 34;
constexpr int kTabMaxWidth = 240;
constexpr int kTabPadding = 12;
constexpr int kCloseSize = 18;
constexpr int kNewTabWidth = 34;

bool Contains(const RECT& rect, POINT point) { return ::PtInRect(&rect, point) != FALSE; }

}  // namespace

void TabBar::RegisterWindowClass(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &TabBar::WndProc;
    wc.hInstance = instance;
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;
    ::RegisterClassExW(&wc);
}

HWND TabBar::Create(HWND parent, int controlId) {
    return ::CreateWindowExW(0, kClassName, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0,
                             0, 0, parent,
                             reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId)),
                             ::GetModuleHandleW(nullptr), nullptr);
}

TabBar* TabBar::FromHandle(HWND window) {
    if (!window) return nullptr;
    return reinterpret_cast<TabBar*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
}

int TabBar::PreferredHeight() const { return ui::Scale(hwnd_, kBarHeight); }

void TabBar::SetTabs(std::vector<std::wstring> titles, int activeIndex) {
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

    RECT client{};
    ::GetClientRect(hwnd_, &client);

    const int newTabWidth = ui::Scale(hwnd_, kNewTabWidth);
    const int available = (std::max)(0L, client.right - newTabWidth);
    const int maxWidth = ui::Scale(hwnd_, kTabMaxWidth);

    const int closeSize = ui::Scale(hwnd_, kCloseSize);
    const int padding = ui::Scale(hwnd_, kTabPadding);
    const int closeTop = (static_cast<int>(client.bottom) - closeSize) / 2;

    // Tabs share the strip rather than run off its end, which would hide the
    // active tab and the new-tab button. Titles ellipsize as tabs narrow; a tab
    // too narrow for its close button loses it (middle-click and Ctrl+W still
    // close it), so a click on a sliver of a tab always selects it.
    const int width = (std::max)(
        1, (std::min)(maxWidth, available / static_cast<int>(titles_.size())));
    const bool hasClose = width >= closeSize + padding * 2;

    int x = 0;
    for (std::size_t i = 0; i < titles_.size(); ++i) {
        TabLayout layout;
        layout.bounds = RECT{x, 0, x + width, client.bottom};
        if (hasClose)
            layout.close = RECT{layout.bounds.right - padding - closeSize + padding / 3, closeTop,
                                layout.bounds.right - padding + padding / 3, closeTop + closeSize};
        layouts.push_back(layout);
        x += width;
    }

    return layouts;
}

RECT TabBar::NewTabButton() const {
    RECT client{};
    ::GetClientRect(hwnd_, &client);

    const auto layouts = LayoutTabs();
    const int left = layouts.empty() ? 0 : layouts.back().bounds.right;
    const int width = ui::Scale(hwnd_, kNewTabWidth);
    return RECT{left, 0, (std::min)(static_cast<long>(left + width), client.right), client.bottom};
}

void TabBar::Render(Renderer& renderer, HDC dc) {
    RECT client{};
    ::GetClientRect(hwnd_, &client);

    renderer.FillRect(RectF::FromRect(client), ui::Argb(ui::kChromeDeep));

    const auto layouts = LayoutTabs();
    const float radius = ui::ScaleF(hwnd_, 7.0f);
    const int padding = ui::Scale(hwnd_, kTabPadding);

    for (std::size_t i = 0; i < layouts.size(); ++i) {
        const auto index = static_cast<int>(i);
        const RECT& bounds = layouts[i].bounds;
        const bool isActive = index == active_;

        RectF tab = RectF::FromRect(bounds);
        tab.left += 1.0f;
        tab.right -= 1.0f;
        tab.top += ui::ScaleF(hwnd_, 4.0f);
        // Overshoot the bottom so only the top corners are rounded.
        tab.bottom += radius;

        if (tab.IsEmpty()) continue;

        if (isActive)
            ui::FillRoundRect(renderer, tab, radius, ui::kPanel);
        else if (index == hotTab_)
            ui::FillRoundRect(renderer, tab, radius, ui::kHover);

        if (isActive) {
            const RectF accent{tab.left + radius * 0.5f, tab.top,
                               tab.right - radius * 0.5f, tab.top + ui::ScaleF(hwnd_, 2.5f)};
            ui::FillRoundRect(renderer, accent, 1.0f, ui::kAccent);
        }

        const RECT& close = layouts[i].close;
        const bool hasClose = ::IsRectEmpty(&close) == FALSE;

        RECT textRect = bounds;
        textRect.left += padding;
        textRect.right = (hasClose ? close.left : bounds.right) - padding / 2;
        ui::Text(dc, textRect, titles_[i], isActive ? ui::kText : ui::kTextDim,
                 DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX,
                 ui::Font(hwnd_, isActive));

        if (!hasClose) continue;

        const RectF closeBox = RectF::FromRect(close);
        if (index == hotClose_)
            ui::FillRoundRect(renderer, closeBox, ui::ScaleF(hwnd_, 4.0f), ui::kPressed);

        const float inset = ui::ScaleF(hwnd_, 5.5f);
        const COLORREF glyph = index == hotClose_ ? ui::kDanger
                               : isActive         ? ui::kText
                                                  : ui::kTextDim;
        ui::Line(renderer, {closeBox.left + inset, closeBox.top + inset},
                 {closeBox.right - inset, closeBox.bottom - inset}, glyph,
                 ui::ScaleF(hwnd_, 1.3f));
        ui::Line(renderer, {closeBox.right - inset, closeBox.top + inset},
                 {closeBox.left + inset, closeBox.bottom - inset}, glyph,
                 ui::ScaleF(hwnd_, 1.3f));
    }

    const RECT newTab = NewTabButton();
    const RectF newTabBox = RectF::FromRect(newTab).Inflated(-ui::ScaleF(hwnd_, 6.0f));
    if (hotNewTab_) ui::FillRoundRect(renderer, newTabBox, ui::ScaleF(hwnd_, 5.0f), ui::kHover);

    const Vec2 centre = newTabBox.Centre();
    const float arm = ui::ScaleF(hwnd_, 5.0f);
    ui::Line(renderer, {centre.x - arm, centre.y}, {centre.x + arm, centre.y}, ui::kTextDim,
             ui::ScaleF(hwnd_, 1.4f));
    ui::Line(renderer, {centre.x, centre.y - arm}, {centre.x, centre.y + arm}, ui::kTextDim,
             ui::ScaleF(hwnd_, 1.4f));
}

void TabBar::OnPaint() {
    PAINTSTRUCT ps{};
    if (surface_.Begin(hwnd_, ps)) Render(surface_.GetRenderer(), surface_.Dc());
    surface_.End(hwnd_, ps);
}

void TabBar::OnMouseMove(POINT point) {
    if (!tracking_) {
        TRACKMOUSEEVENT track{};
        track.cbSize = sizeof(track);
        track.dwFlags = TME_LEAVE;
        track.hwndTrack = hwnd_;
        tracking_ = ::TrackMouseEvent(&track) != FALSE;
    }

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

void TabBar::OnLeftDown(POINT point) {
    pressedClose_ = -1;
    pressedNewTab_ = false;

    const auto layouts = LayoutTabs();
    for (std::size_t i = 0; i < layouts.size(); ++i) {
        if (!Contains(layouts[i].bounds, point)) continue;

        if (Contains(layouts[i].close, point)) {
            pressedClose_ = static_cast<int>(i);
            ::SetCapture(hwnd_);
            return;
        }

        if (onSelect) onSelect(static_cast<int>(i));
        return;
    }

    if (Contains(NewTabButton(), point)) {
        pressedNewTab_ = true;
        ::SetCapture(hwnd_);
    }
}

void TabBar::OnLeftUp(POINT point) {
    const int pressedClose = pressedClose_;
    const bool pressedNewTab = pressedNewTab_;
    pressedClose_ = -1;
    pressedNewTab_ = false;

    if (::GetCapture() == hwnd_) ::ReleaseCapture();

    if (pressedClose >= 0) {
        const auto layouts = LayoutTabs();
        if (pressedClose < static_cast<int>(layouts.size()) &&
            Contains(layouts[static_cast<std::size_t>(pressedClose)].close, point) && onClose)
            onClose(pressedClose);
        return;
    }

    if (pressedNewTab && Contains(NewTabButton(), point) && onNewTab) onNewTab();
}

LRESULT CALLBACK TabBar::WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    TabBar* self = FromHandle(window);

    switch (message) {
        case WM_CREATE: {
            auto* bar = new TabBar(window);
            ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(bar));
            return 0;
        }

        case WM_DESTROY:
            ::SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            delete self;
            return 0;

        case WM_ERASEBKGND: return 1;

        case WM_PAINT:
            if (self) self->OnPaint();
            return 0;

        case WM_SIZE:
        case WM_DPICHANGED_AFTERPARENT:
            if (self) self->Invalidate();
            return 0;

        case WM_MOUSEMOVE:
            if (self) self->OnMouseMove({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
            return 0;

        case WM_MOUSELEAVE:
            if (self) self->OnMouseLeave();
            return 0;

        case WM_LBUTTONDOWN:
            if (self) self->OnLeftDown({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
            return 0;

        case WM_LBUTTONUP:
            if (self) self->OnLeftUp({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
            return 0;

        case WM_MBUTTONUP:
            if (self) {
                const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                const auto layouts = self->LayoutTabs();
                for (std::size_t i = 0; i < layouts.size(); ++i) {
                    if (Contains(layouts[i].bounds, point) && self->onClose) {
                        self->onClose(static_cast<int>(i));
                        break;
                    }
                }
            }
            return 0;

        default: break;
    }

    return ::DefWindowProcW(window, message, wParam, lParam);
}
