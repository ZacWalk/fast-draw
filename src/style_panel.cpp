#include "style_panel.h"

#include "color_popup.h"

#include <windowsx.h>

#include <algorithm>
#include <string>

namespace {

constexpr wchar_t kClassName[] = L"FastDrawStylePanel";

constexpr int kPadding = 10;
constexpr int kHeader = 20;
constexpr int kRow = 24;
constexpr int kGap = 6;
constexpr int kSwatchWidth = 34;

const wchar_t* const kDirectionGlyphs[] = {L"\u2193", L"\u2192", L"\u2198", L"\u2197"};

void Chip(Renderer& renderer, HDC dc, HWND window, const RECT& bounds, std::wstring_view label,
          bool active, bool hot) {
    const RectF box = RectF::FromRect(bounds);
    const float radius = ui::ScaleF(window, 5.0f);

    ui::FillRoundRect(renderer, box, radius, active ? ui::kAccent : (hot ? ui::kHover : ui::kPanel));
    ui::FrameRoundRect(renderer, box.Inflated(-0.5f), radius,
                       active ? ui::kAccent : (hot ? ui::kAccent : ui::kBorder), 1.0f);

    if (!label.empty())
        ui::Text(dc, bounds, label, active ? RGB(255, 255, 255) : ui::kText,
                 DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX, ui::Font(window));
}

}  // namespace

void StylePanel::RegisterWindowClass(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &StylePanel::WndProc;
    wc.hInstance = instance;
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;
    ::RegisterClassExW(&wc);
}

HWND StylePanel::Create(HWND parent, int controlId) {
    return ::CreateWindowExW(0, kClassName, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0,
                             0, 0, parent,
                             reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId)),
                             ::GetModuleHandleW(nullptr), nullptr);
}

StylePanel* StylePanel::FromHandle(HWND window) {
    if (!window) return nullptr;
    return reinterpret_cast<StylePanel*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
}

void StylePanel::SetStyle(const Style& style, bool hasSelection) {
    if (style == style_ && hasSelection == hasSelection_) return;
    style_ = style;
    hasSelection_ = hasSelection;
    Invalidate();
}

int StylePanel::PreferredHeight() const {
    return ui::Scale(hwnd_, kPadding * 2 + kHeader * 4 + kRow * 7 + kGap * 8);
}

void StylePanel::Layout() {
    RECT client{};
    ::GetClientRect(hwnd_, &client);

    const int padding = ui::Scale(hwnd_, kPadding);
    const int header = ui::Scale(hwnd_, kHeader);
    const int row = ui::Scale(hwnd_, kRow);
    const int gap = ui::Scale(hwnd_, kGap);
    const int left = padding;
    const int right = (std::max<int>)(static_cast<int>(client.right) - padding,
                                      left + ui::Scale(hwnd_, 120));
    const int swatch = ui::Scale(hwnd_, kSwatchWidth);

    int y = padding;

    auto placeHeader = [&](int index) {
        headers_[static_cast<std::size_t>(index)] = RECT{left, y, right, y + header};
        y += header;
    };
    auto quarter = [&](int index, int count) {
        const int width = (right - left - gap * (count - 1)) / count;
        return RECT{left + index * (width + gap), y, left + index * (width + gap) + width,
                    y + row};
    };

    placeHeader(0);
    Bounds(Item::FillColour) = RECT{left, y, left + swatch, y + row};
    Bounds(Item::FillToggle) = RECT{left + swatch + gap, y, right, y + row};
    y += row + gap;

    Bounds(Item::GradientToggle) = RECT{left, y, right - swatch - gap, y + row};
    Bounds(Item::GradientColour) = RECT{right - swatch, y, right, y + row};
    y += row + gap;

    for (int i = 0; i < 4; ++i)
        Bounds(static_cast<Item>(static_cast<int>(Item::Direction0) + i)) = quarter(i, 4);
    y += row + gap;

    placeHeader(1);
    Bounds(Item::LineColour) = RECT{left, y, left + swatch, y + row};
    Bounds(Item::LineToggle) = RECT{left + swatch + gap, y, right, y + row};
    y += row + gap;

    const int stepper = ui::Scale(hwnd_, 26);
    Bounds(Item::WidthDown) = RECT{right - stepper * 2 - gap, y, right - stepper - gap, y + row};
    Bounds(Item::WidthUp) = RECT{right - stepper, y, right, y + row};
    widthValue_ = RECT{left, y, right - stepper * 2 - gap * 2, y + row};
    y += row + gap;

    for (int i = 0; i < 4; ++i)
        Bounds(static_cast<Item>(static_cast<int>(Item::Dash0) + i)) = quarter(i, 4);
    y += row + gap;

    Bounds(Item::RoundJoins) = quarter(0, 3);
    Bounds(Item::ArrowStart) = quarter(1, 3);
    Bounds(Item::ArrowEnd) = quarter(2, 3);
    y += row + gap;

    placeHeader(2);
    Bounds(Item::TextColour) = RECT{left, y, left + swatch, y + row};
    Bounds(Item::SizeDown) = RECT{left + swatch + gap, y, left + swatch + gap + stepper, y + row};
    sizeValue_ = RECT{left + swatch + gap * 2 + stepper, y,
                      left + swatch + gap * 2 + stepper * 2, y + row};
    Bounds(Item::SizeUp) = RECT{left + swatch + gap * 3 + stepper * 2, y,
                                left + swatch + gap * 3 + stepper * 3, y + row};
    Bounds(Item::Bold) = RECT{right - stepper * 2 - gap, y, right - stepper - gap, y + row};
    Bounds(Item::Italic) = RECT{right - stepper, y, right, y + row};
    y += row;

    contentHeight_ = y + padding;
}

void StylePanel::Render(Renderer& renderer, HDC dc) {
    Layout();

    RECT client{};
    ::GetClientRect(hwnd_, &client);
    renderer.FillRect(RectF::FromRect(client), ui::Argb(ui::kChrome));

    HFONT headerFont = ui::Font(hwnd_, true);
    const wchar_t* const titles[] = {L"Fill", L"Line", L"Text"};
    for (std::size_t i = 0; i < headers_.size(); ++i) {
        ui::Text(dc, headers_[i], titles[i], ui::kTextDim,
                 DT_SINGLELINE | DT_BOTTOM | DT_LEFT | DT_NOPREFIX, headerFont);
    }

    const auto hot = [this](Item item) { return hot_ == static_cast<int>(item); };
    const float radius = ui::ScaleF(hwnd_, 4.0f);

    ui::Swatch(renderer, RectF::FromRect(Bounds(Item::FillColour)), style_.fill, !style_.hasFill,
               hot(Item::FillColour), radius);
    Chip(renderer, dc, hwnd_, Bounds(Item::FillToggle), style_.hasFill ? L"Filled" : L"No fill",
         style_.hasFill, hot(Item::FillToggle));

    Chip(renderer, dc, hwnd_, Bounds(Item::GradientToggle), L"Gradient", style_.gradient,
         hot(Item::GradientToggle));
    ui::Swatch(renderer, RectF::FromRect(Bounds(Item::GradientColour)), style_.fillEnd, false,
               hot(Item::GradientColour), radius);

    for (int i = 0; i < 4; ++i) {
        const auto item = static_cast<Item>(static_cast<int>(Item::Direction0) + i);
        const bool active =
            style_.gradient && static_cast<int>(style_.gradientDirection) == i;
        Chip(renderer, dc, hwnd_, Bounds(item), kDirectionGlyphs[i], active, hot(item));
    }

    ui::Swatch(renderer, RectF::FromRect(Bounds(Item::LineColour)), style_.line, !style_.hasLine,
               hot(Item::LineColour), radius);
    Chip(renderer, dc, hwnd_, Bounds(Item::LineToggle), style_.hasLine ? L"Stroked" : L"No line",
         style_.hasLine, hot(Item::LineToggle));

    ui::Text(dc, widthValue_, L"Width " + std::to_wstring(style_.lineWidth), ui::kText,
             DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX, ui::Font(hwnd_));
    Chip(renderer, dc, hwnd_, Bounds(Item::WidthDown), L"\u2212", false, hot(Item::WidthDown));
    Chip(renderer, dc, hwnd_, Bounds(Item::WidthUp), L"+", false, hot(Item::WidthUp));

    for (int i = 0; i < 4; ++i) {
        const auto item = static_cast<Item>(static_cast<int>(Item::Dash0) + i);
        const bool active = static_cast<int>(style_.dash) == i;
        Chip(renderer, dc, hwnd_, Bounds(item), {}, active, hot(item));

        const RECT& box = Bounds(item);
        const float midY = static_cast<float>(box.top + box.bottom) * 0.5f;
        const float inset = ui::ScaleF(hwnd_, 6.0f);

        Path line;
        const Vec2 points[2] = {{box.left + inset, midY}, {box.right - inset, midY}};
        line.AddPolyline(std::span<const Vec2>(points, 2));

        std::array<float, 4> storage{};
        const float width = ui::ScaleF(hwnd_, 1.6f);
        renderer.Stroke(line, SolidPaint(ui::Argb(active ? RGB(255, 255, 255) : ui::kText)),
                        StrokeStyle{width, true,
                                    DashPattern(static_cast<DashStyle>(i), width * 1.6f, storage)});
    }

    Chip(renderer, dc, hwnd_, Bounds(Item::RoundJoins), L"Round", style_.roundJoins,
         hot(Item::RoundJoins));
    Chip(renderer, dc, hwnd_, Bounds(Item::ArrowStart), L"\u25c0\u2014", style_.startArrow,
         hot(Item::ArrowStart));
    Chip(renderer, dc, hwnd_, Bounds(Item::ArrowEnd), L"\u2014\u25b6", style_.endArrow,
         hot(Item::ArrowEnd));

    ui::Swatch(renderer, RectF::FromRect(Bounds(Item::TextColour)), style_.textColour, false,
               hot(Item::TextColour), radius);
    Chip(renderer, dc, hwnd_, Bounds(Item::SizeDown), L"\u2212", false, hot(Item::SizeDown));
    ui::Text(dc, sizeValue_, std::to_wstring(style_.fontSize), ui::kText,
             DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX, ui::Font(hwnd_));
    Chip(renderer, dc, hwnd_, Bounds(Item::SizeUp), L"+", false, hot(Item::SizeUp));
    Chip(renderer, dc, hwnd_, Bounds(Item::Bold), L"B", style_.bold, hot(Item::Bold));
    Chip(renderer, dc, hwnd_, Bounds(Item::Italic), L"I", style_.italic, hot(Item::Italic));

    if (!hasSelection_) {
        RECT hint{ui::Scale(hwnd_, kPadding), contentHeight_ - ui::Scale(hwnd_, kPadding),
                  client.right - ui::Scale(hwnd_, kPadding),
                  contentHeight_ + ui::Scale(hwnd_, kRow)};
        ui::Text(dc, hint, L"Defaults for new shapes", ui::kTextDim,
                 DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX | DT_END_ELLIPSIS,
                 ui::Font(hwnd_));
    }

    ui::Line(renderer, {0.0f, static_cast<float>(client.bottom) - 0.5f},
             {static_cast<float>(client.right), static_cast<float>(client.bottom) - 0.5f},
             ui::kBorder, 1.0f);
    ui::Line(renderer, {static_cast<float>(client.right) - 0.5f, 0.0f},
             {static_cast<float>(client.right) - 0.5f, static_cast<float>(client.bottom)},
             ui::kBorder, 1.0f);
}

void StylePanel::OnPaint() {
    PAINTSTRUCT ps{};
    if (surface_.Begin(hwnd_, ps)) Render(surface_.GetRenderer(), surface_.Dc());
    surface_.End(hwnd_, ps);
}

int StylePanel::HitTest(POINT point) const {
    for (std::size_t i = 0; i < bounds_.size(); ++i) {
        RECT box = bounds_[i];
        if (::PtInRect(&box, point)) return static_cast<int>(i);
    }
    return -1;
}

void StylePanel::OnMouseMove(POINT point) {
    if (!tracking_) {
        TRACKMOUSEEVENT track{};
        track.cbSize = sizeof(track);
        track.dwFlags = TME_LEAVE;
        track.hwndTrack = hwnd_;
        tracking_ = ::TrackMouseEvent(&track) != FALSE;
    }

    const int hot = HitTest(point);
    if (hot == hot_) return;
    hot_ = hot;
    Invalidate();
}

void StylePanel::OnMouseLeave() {
    tracking_ = false;
    if (hot_ == -1) return;
    hot_ = -1;
    Invalidate();
}

void StylePanel::OnLeftDown(POINT point) {
    pressed_ = HitTest(point);
    if (pressed_ >= 0) ::SetCapture(hwnd_);
}

void StylePanel::OnLeftUp(POINT point) {
    // Only a click that started on the same item counts. The release of a click
    // that began elsewhere - the one that dismissed a colour popup - would
    // otherwise toggle whatever happens to lie under the pointer.
    const int pressed = pressed_;
    pressed_ = -1;
    if (::GetCapture() == hwnd_) ::ReleaseCapture();

    const int index = HitTest(point);
    if (index >= 0 && index == pressed) Activate(static_cast<Item>(index));
}

void StylePanel::Activate(Item item) {
    auto pick = [this](Item target, COLORREF& colour) {
        RECT anchor = Bounds(target);
        ::MapWindowPoints(hwnd_, nullptr, reinterpret_cast<POINT*>(&anchor), 2);
        if (auto chosen = PickColour(hwnd_, anchor, colour)) {
            colour = *chosen;
            return true;
        }
        return false;
    };

    switch (item) {
        case Item::FillColour:
            if (pick(item, style_.fill)) style_.hasFill = true;
            break;
        case Item::FillToggle: style_.hasFill = !style_.hasFill; break;

        case Item::GradientToggle: style_.gradient = !style_.gradient; break;
        case Item::GradientColour:
            if (pick(item, style_.fillEnd)) style_.gradient = true;
            break;

        case Item::Direction0:
        case Item::Direction1:
        case Item::Direction2:
        case Item::Direction3:
            style_.gradientDirection = static_cast<GradientDirection>(
                static_cast<int>(item) - static_cast<int>(Item::Direction0));
            style_.gradient = true;
            break;

        case Item::LineColour:
            if (pick(item, style_.line)) style_.hasLine = true;
            break;
        case Item::LineToggle: style_.hasLine = !style_.hasLine; break;

        case Item::WidthDown: style_.lineWidth = (std::max)(1, style_.lineWidth - 1); break;
        case Item::WidthUp: style_.lineWidth = (std::min)(40, style_.lineWidth + 1); break;

        case Item::Dash0:
        case Item::Dash1:
        case Item::Dash2:
        case Item::Dash3:
            style_.dash =
                static_cast<DashStyle>(static_cast<int>(item) - static_cast<int>(Item::Dash0));
            break;

        case Item::RoundJoins: style_.roundJoins = !style_.roundJoins; break;
        case Item::ArrowStart: style_.startArrow = !style_.startArrow; break;
        case Item::ArrowEnd: style_.endArrow = !style_.endArrow; break;

        case Item::TextColour: pick(item, style_.textColour); break;
        case Item::SizeDown: style_.fontSize = (std::max)(6, style_.fontSize - 1); break;
        case Item::SizeUp: style_.fontSize = (std::min)(96, style_.fontSize + 1); break;
        case Item::Bold: style_.bold = !style_.bold; break;
        case Item::Italic: style_.italic = !style_.italic; break;

        case Item::Count: return;
    }

    Publish();
}

void StylePanel::Publish() {
    Invalidate();
    if (onChange) onChange(style_);
}

LRESULT CALLBACK StylePanel::WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    StylePanel* self = FromHandle(window);

    switch (message) {
        case WM_CREATE: {
            auto* panel = new StylePanel(window);
            ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(panel));
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
            if (self) {
                self->Layout();
                self->Invalidate();
            }
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

        case WM_CAPTURECHANGED:
            if (self) self->pressed_ = -1;
            return 0;

        default: break;
    }

    return ::DefWindowProcW(window, message, wParam, lParam);
}
