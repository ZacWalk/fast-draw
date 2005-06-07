#include "shape_palette.h"

#include "shapes.h"

#include <windowsx.h>

#include <algorithm>
#include <array>

namespace {

constexpr wchar_t kClassName[] = L"FastDrawShapePalette";

constexpr int kHeaderHeight = 24;
constexpr int kCell = 44;
constexpr int kPadding = 8;
constexpr int kDragThreshold = 5;

struct Entry {
    Tool tool;
    const wchar_t* label;
};

std::array<Entry, kShapeKindCount + 2> BuildEntries() {
    std::array<Entry, kShapeKindCount + 2> entries{};
    entries[0] = {Tool{Tool::Kind::Select, ShapeKind::Rectangle}, L"Select"};
    entries[1] = {Tool{Tool::Kind::Text, ShapeKind::Rectangle}, L"Text"};
    for (int i = 0; i < kShapeKindCount; ++i) {
        const auto kind = static_cast<ShapeKind>(i);
        entries[static_cast<std::size_t>(i) + 2] = {Tool{Tool::Kind::Shape, kind},
                                                    ShapeKindLabel(kind)};
    }
    return entries;
}

const std::array<Entry, kShapeKindCount + 2>& Entries() {
    static const auto entries = BuildEntries();
    return entries;
}

constexpr int kEntryCount = kShapeKindCount + 2;

// The pointer and the letter A are drawn by hand; every other cell reuses the
// real shape geometry so the preview always matches what gets created.
void DrawPointer(Renderer& renderer, const RectF& box, COLORREF colour) {
    const float w = box.Width();
    const float h = box.Height();
    const Vec2 points[] = {{box.left + w * 0.28f, box.top + h * 0.12f},
                           {box.left + w * 0.28f, box.top + h * 0.86f},
                           {box.left + w * 0.46f, box.top + h * 0.66f},
                           {box.left + w * 0.58f, box.top + h * 0.92f},
                           {box.left + w * 0.70f, box.top + h * 0.84f},
                           {box.left + w * 0.58f, box.top + h * 0.60f},
                           {box.left + w * 0.78f, box.top + h * 0.56f}};

    Path path;
    path.AddPolygon(std::span<const Vec2>(points, std::size(points)));
    renderer.Fill(path, SolidPaint(ui::Argb(colour)));
}

}  // namespace

void ShapePalette::RegisterWindowClass(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &ShapePalette::WndProc;
    wc.hInstance = instance;
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;
    ::RegisterClassExW(&wc);
}

HWND ShapePalette::Create(HWND parent, int controlId) {
    return ::CreateWindowExW(0, kClassName, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0,
                             0, 0, parent,
                             reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId)),
                             ::GetModuleHandleW(nullptr), nullptr);
}

ShapePalette* ShapePalette::FromHandle(HWND window) {
    if (!window) return nullptr;
    return reinterpret_cast<ShapePalette*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
}

void ShapePalette::SetActive(Tool tool) {
    if (active_ == tool) return;
    active_ = tool;
    Invalidate();
}

int ShapePalette::Columns() const {
    RECT client{};
    ::GetClientRect(hwnd_, &client);
    const int cell = ui::Scale(hwnd_, kCell);
    const int usable = client.right - ui::Scale(hwnd_, kPadding) * 2;
    return (std::max)(1, usable / (std::max)(1, cell));
}

RECT ShapePalette::CellBounds(int index) const {
    const int cell = ui::Scale(hwnd_, kCell);
    const int padding = ui::Scale(hwnd_, kPadding);
    const int columns = Columns();

    const int x = padding + (index % columns) * cell;
    const int y = padding + ui::Scale(hwnd_, kHeaderHeight) + (index / columns) * cell;
    return RECT{x, y, x + cell, y + cell};
}

int ShapePalette::HitTest(POINT point) const {
    for (int i = 0; i < kEntryCount; ++i) {
        RECT bounds = CellBounds(i);
        if (::PtInRect(&bounds, point)) return i;
    }
    return -1;
}

void ShapePalette::Render(Renderer& renderer, HDC dc) {
    RECT client{};
    ::GetClientRect(hwnd_, &client);

    renderer.FillRect(RectF::FromRect(client), ui::Argb(ui::kChrome));

    const int padding = ui::Scale(hwnd_, kPadding);
    RECT header{padding, padding / 2, client.right - padding,
                padding / 2 + ui::Scale(hwnd_, kHeaderHeight)};

    const int described = hot_ >= 0 ? hot_ : -1;
    const std::wstring caption =
        described >= 0 ? Entries()[static_cast<std::size_t>(described)].label : L"Shapes";
    ui::Text(dc, header, caption, described >= 0 ? ui::kText : ui::kTextDim,
             DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX | DT_END_ELLIPSIS,
             ui::Font(hwnd_, true));

    const float radius = ui::ScaleF(hwnd_, 6.0f);

    for (int i = 0; i < kEntryCount; ++i) {
        const RECT cell = CellBounds(i);
        if (cell.top > client.bottom) break;

        const Entry& entry = Entries()[static_cast<std::size_t>(i)];
        const bool active = entry.tool == active_;
        const RectF bounds = RectF::FromRect(cell).Inflated(-ui::ScaleF(hwnd_, 3.0f));

        if (active)
            ui::FillRoundRect(renderer, bounds, radius, ui::kAccentSoft);
        else if (i == hot_)
            ui::FillRoundRect(renderer, bounds, radius, ui::kHover);

        if (active) ui::FrameRoundRect(renderer, bounds.Inflated(-0.5f), radius, ui::kAccent, 1.4f);

        const RectF box = bounds.Inflated(-ui::ScaleF(hwnd_, 9.0f));
        const COLORREF ink = active ? ui::kAccent : ui::kText;

        if (entry.tool.kind == Tool::Kind::Select) {
            DrawPointer(renderer, box, ink);
            continue;
        }

        if (entry.tool.kind == Tool::Kind::Text) {
            RECT glyph = box.ToRect();
            ui::Text(dc, glyph, L"A", ink,
                     DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX, ui::Font(hwnd_, true));
            continue;
        }

        Path path;
        BuildShapePath(entry.tool.shape, box, path);
        if (!IsLinear(entry.tool.shape))
            renderer.Fill(path, SolidPaint(ui::Argb(active ? RGB(0xD5, 0xE6, 0xF8)
                                                           : RGB(0xE9, 0xEC, 0xF2))));
        renderer.Stroke(path, SolidPaint(ui::Argb(ink)),
                        StrokeStyle{ui::ScaleF(hwnd_, 1.4f), true});

        if (entry.tool.shape == ShapeKind::Arrow) {
            Path head;
            BuildArrowHead({box.right, box.top}, {box.left, box.bottom},
                           ui::ScaleF(hwnd_, 9.0f), head);
            renderer.Fill(head, SolidPaint(ui::Argb(ink)));
        }
    }

    ui::Line(renderer, {static_cast<float>(client.right) - 0.5f, 0.0f},
             {static_cast<float>(client.right) - 0.5f, static_cast<float>(client.bottom)},
             ui::kBorder, 1.0f);
}

void ShapePalette::OnPaint() {
    PAINTSTRUCT ps{};
    if (surface_.Begin(hwnd_, ps)) Render(surface_.GetRenderer(), surface_.Dc());
    surface_.End(hwnd_, ps);
}

void ShapePalette::OnMouseMove(POINT point) {
    if (pressed_ >= 0 && !dragging_) {
        const int threshold = ui::Scale(hwnd_, kDragThreshold);
        if (std::abs(point.x - dragStart_.x) > threshold ||
            std::abs(point.y - dragStart_.y) > threshold) {
            dragging_ = Entries()[static_cast<std::size_t>(pressed_)].tool.kind ==
                        Tool::Kind::Shape;
        }
    }

    if (dragging_) {
        ::SetCursor(::LoadCursorW(nullptr, IDC_CROSS));
        return;
    }

    if (!tracking_) {
        TRACKMOUSEEVENT track{};
        track.cbSize = sizeof(track);
        track.dwFlags = TME_LEAVE;
        track.hwndTrack = hwnd_;
        tracking_ = ::TrackMouseEvent(&track) != FALSE;
    }

    const int hot = HitTest(point);
    if (hot != hot_) {
        hot_ = hot;
        Invalidate();
    }
}

void ShapePalette::OnMouseLeave() {
    tracking_ = false;
    if (hot_ == -1) return;
    hot_ = -1;
    Invalidate();
}

void ShapePalette::OnLeftDown(POINT point) {
    pressed_ = HitTest(point);
    dragStart_ = point;
    dragging_ = false;
    if (pressed_ >= 0) ::SetCapture(hwnd_);
}

void ShapePalette::OnLeftUp(POINT point) {
    const int pressed = pressed_;
    const bool dragged = dragging_;
    pressed_ = -1;
    dragging_ = false;

    if (::GetCapture() == hwnd_) ::ReleaseCapture();
    if (pressed < 0) return;

    const Entry& entry = Entries()[static_cast<std::size_t>(pressed)];

    if (dragged) {
        POINT screenPoint = point;
        ::ClientToScreen(hwnd_, &screenPoint);
        if (onDrop) onDrop(entry.tool.shape, screenPoint);
        return;
    }

    RECT cell = CellBounds(pressed);
    if (::PtInRect(&cell, point) && onSelect) onSelect(entry.tool);
}

LRESULT CALLBACK ShapePalette::WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    ShapePalette* self = FromHandle(window);

    switch (message) {
        case WM_CREATE: {
            auto* palette = new ShapePalette(window);
            ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(palette));
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

        case WM_SETCURSOR:
            if (self && self->dragging_) return TRUE;
            break;

        case WM_CAPTURECHANGED:
            if (self) {
                self->pressed_ = -1;
                self->dragging_ = false;
            }
            return 0;

        default: break;
    }

    return ::DefWindowProcW(window, message, wParam, lParam);
}
