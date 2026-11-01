#include "shape_palette.h"

#include "shapes.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {

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

bool Contains(const Rect& rect, const Point point) {
    return point.x >= rect.left && point.x < rect.right &&
           point.y >= rect.top && point.y < rect.bottom;
}

void DrawPointer(Renderer& renderer, const RectF& box, const Color colour) {
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

void ShapePalette::SetExtent(const Size extent, const double dpiScale) {
    extent_ = extent;
    dpiScale_ = dpiScale;
}

void ShapePalette::Invalidate() {
    if (onInvalidate) onInvalidate();
}

void ShapePalette::SetActive(const Tool tool) {
    if (active_ == tool) return;
    active_ = tool;
    Invalidate();
}

int ShapePalette::Columns() const {
    const int cell = ui::Scale(dpiScale_, kCell);
    const int usable = extent_.cx - ui::Scale(dpiScale_, kPadding) * 2;
    return (std::max)(1, usable / (std::max)(1, cell));
}

Rect ShapePalette::CellBounds(const int index) const {
    const int cell = ui::Scale(dpiScale_, kCell);
    const int padding = ui::Scale(dpiScale_, kPadding);
    const int columns = Columns();

    const int x = padding + (index % columns) * cell;
    const int y = padding + ui::Scale(dpiScale_, kHeaderHeight) + (index / columns) * cell;
    return Rect{x, y, x + cell, y + cell};
}

int ShapePalette::HitTest(const Point point) const {
    for (int i = 0; i < kEntryCount; ++i)
        if (Contains(CellBounds(i), point)) return i;
    return -1;
}

void ShapePalette::Render(Renderer& renderer, Canvas& canvas) {
    const Rect client{0, 0, extent_.cx, extent_.cy};
    renderer.FillRect(RectF::FromRect(client), ui::Argb(ui::kChrome));

    const int padding = ui::Scale(dpiScale_, kPadding);
    Rect header{padding, padding / 2, extent_.cx - padding,
                padding / 2 + ui::Scale(dpiScale_, kHeaderHeight)};

    const int described = hot_ >= 0 ? hot_ : -1;
    const std::wstring caption =
        described >= 0 ? Entries()[static_cast<std::size_t>(described)].label : L"Shapes";
    ui::Text(canvas, header, caption, described >= 0 ? ui::kText : ui::kTextDim,
             ui::TextAlign::left, ui::Font(dpiScale_, true));

    const float radius = ui::ScaleF(dpiScale_, 6.0f);

    for (int i = 0; i < kEntryCount; ++i) {
        const Rect cell = CellBounds(i);
        if (cell.top > extent_.cy) break;

        const Entry& entry = Entries()[static_cast<std::size_t>(i)];
        const bool active = entry.tool == active_;
        const RectF bounds = RectF::FromRect(cell).Inflated(-ui::ScaleF(dpiScale_, 3.0f));

        if (active)
            ui::FillRoundRect(renderer, bounds, radius, ui::kAccentSoft);
        else if (i == hot_)
            ui::FillRoundRect(renderer, bounds, radius, ui::kHover);

        if (active) ui::FrameRoundRect(renderer, bounds.Inflated(-0.5f), radius, ui::kAccent, 1.4f);

        const RectF box = bounds.Inflated(-ui::ScaleF(dpiScale_, 9.0f));
        const Color ink = active ? ui::kAccent : ui::kText;

        if (entry.tool.kind == Tool::Kind::Select) {
            DrawPointer(renderer, box, ink);
            continue;
        }

        if (entry.tool.kind == Tool::Kind::Text) {
            ui::Text(canvas, box.ToRect(), L"A", ink, ui::TextAlign::center,
                     ui::Font(dpiScale_, true));
            continue;
        }

        Path path;
        BuildShapePath(entry.tool.shape, box, path);
        if (!IsLinear(entry.tool.shape))
            renderer.Fill(path, SolidPaint(ui::Argb(active ? Rgb(0xD5, 0xE6, 0xF8)
                                                           : Rgb(0xE9, 0xEC, 0xF2))));
        renderer.Stroke(path, SolidPaint(ui::Argb(ink)),
                        StrokeStyle{ui::ScaleF(dpiScale_, 1.4f), true});

        if (entry.tool.shape == ShapeKind::Arrow) {
            Path head;
            BuildArrowHead({box.right, box.top}, {box.left, box.bottom},
                           ui::ScaleF(dpiScale_, 9.0f), head);
            renderer.Fill(head, SolidPaint(ui::Argb(ink)));
        }
    }

    ui::Line(renderer, {static_cast<float>(extent_.cx) - 0.5f, 0.0f},
             {static_cast<float>(extent_.cx) - 0.5f, static_cast<float>(extent_.cy)},
             ui::kBorder, 1.0f);
}

void ShapePalette::OnMouseMove(const Point point) {
    if (pressed_ >= 0 && !dragging_) {
        const int threshold = ui::Scale(dpiScale_, kDragThreshold);
        if (std::abs(point.x - dragStart_.x) > threshold ||
            std::abs(point.y - dragStart_.y) > threshold) {
            dragging_ = Entries()[static_cast<std::size_t>(pressed_)].tool.kind == Tool::Kind::Shape;
        }
    }

    if (dragging_) {
        if (onDragCursor) onDragCursor();
        return;
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

void ShapePalette::OnLeftDown(const Point point) {
    pressed_ = HitTest(point);
    dragStart_ = point;
    dragging_ = false;
    if (pressed_ >= 0 && onCapture) onCapture(true);
}

void ShapePalette::OnLeftUp(const Point point) {
    const int pressed = pressed_;
    const bool dragged = dragging_;
    pressed_ = -1;
    dragging_ = false;

    if (onCapture) onCapture(false);
    if (pressed < 0) return;

    const Entry& entry = Entries()[static_cast<std::size_t>(pressed)];
    if (dragged) {
        if (onDrop) onDrop(entry.tool.shape, point);
        return;
    }

    const Rect cell = CellBounds(pressed);
    if (Contains(cell, point) && onSelect) onSelect(entry.tool);
}
