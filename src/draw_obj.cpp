#include "draw_obj.h"

#include "ui.h"
#include "utf8.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>

namespace {

// Text is inset from the shape outline by this many document units.
constexpr long kLabelInset = 6;

Json RectToJson(const Rect& rect) {
    Json json = Json::MakeObject();
    json.Set("left", Json(rect.left));
    json.Set("top", Json(rect.top));
    json.Set("right", Json(rect.right));
    json.Set("bottom", Json(rect.bottom));
    return json;
}

// A document is untrusted input; keep coordinates far away from the range where
// inflating a rectangle would overflow.
constexpr long kCoordinateLimit = 1'000'000;

long CoordinateFromJson(const Json& value) {
    return (std::clamp)(static_cast<long>(value.AsInt()), -kCoordinateLimit, kCoordinateLimit);
}

Rect RectFromJson(const Json& json) {
    return Rect(CoordinateFromJson(json["left"]), CoordinateFromJson(json["top"]),
                CoordinateFromJson(json["right"]), CoordinateFromJson(json["bottom"]));
}

// Arrow heads scale with the line so they stay in proportion.
long ArrowSize(const Style& style) { return 8 + style.lineWidth * 3; }

// The transform used for geometry expressed in document units.
constexpr Transform kDocTransform{1.0f, 0.0f, 0.0f};

}  // namespace

Json DrawObj::ToJson() const {
    Json json = Json::MakeObject();
    json.Set("shape", Json(ShapeKindName(kind)));
    json.Set("position", RectToJson(position));
    json.Set("style", StyleToJson(style));
    if (!text.empty()) json.Set("text", Json(Utf8Encode(text)));
    return json;
}

DrawObj DrawObj::FromJson(const Json& json) {
    DrawObj object;
    object.kind = ShapeKindFromName(json["shape"].AsString(), ShapeKind::Rectangle);
    object.position = RectFromJson(json["position"]);
    object.style = StyleFromJson(json["style"]);
    object.text = Utf8Decode(json["text"].AsString());
    return object;
}

void DrawObj::BuildPath(const Transform& transform, Path& out) const {
    if (IsLinear(kind)) {
        BuildLinePath(transform.ToDevice(position.TopLeft()),
                      transform.ToDevice(position.BottomRight()), out);
        return;
    }
    BuildShapePath(kind, transform.ToDevice(position), out);
}

void DrawObj::Draw(Renderer& renderer, const Transform& transform) const {
    Path path;
    BuildPath(transform, path);
    if (path.Empty()) return;

    if (style.hasFill && !IsLinear(kind))
        renderer.Fill(path, FillPaint(style, transform.ToDevice(position)));

    const float width = (std::max)(1.0f, transform.ToDevice(style.lineWidth));
    if (style.hasLine && style.lineWidth > 0) {
        std::array<float, 4> dashes{};
        StrokeStyle stroke{width, style.roundJoins, DashPattern(style.dash, width, dashes)};
        renderer.Stroke(path, LinePaint(style), stroke);
    }

    if (!IsLinear(kind) || (!style.startArrow && !style.endArrow)) return;

    const Vec2 start = transform.ToDevice(position.TopLeft());
    const Vec2 end = transform.ToDevice(position.BottomRight());
    const float size = (std::max)(5.0f, transform.ToDevice(ArrowSize(style)));

    Path heads;
    if (style.endArrow) BuildArrowHead(end, start, size, heads);
    if (style.startArrow) BuildArrowHead(start, end, size, heads);
    renderer.Fill(heads, LinePaint(style));
}

pf::font_handle CreateStyleFont(const Style& style, const float scale) {
    pf::font_def definition;
    definition.face = "Segoe UI";
    definition.size = (std::max)(1, static_cast<int>(std::lround(style.fontSize * scale / 0.96f)));
    definition.weight = style.bold ? 600 : 400;
    definition.italic = style.italic;
    return pf::create_font_handle(definition);
}

Rect LabelRect(const DrawObj& object, const Transform& transform) {
    Rect box = object.position.Normalized();

    if (IsLinear(object.kind)) {
        // Sit the label just above the middle of the line.
        const auto cx = (box.left + box.right) / 2;
        const auto cy = (box.top + box.bottom) / 2;
        const auto halfWidth = (std::max)(std::int32_t{120}, box.Width() / 2);
        box = Rect(cx - halfWidth, cy + 58, cx + halfWidth, cy + 8);
    } else {
        box.Inflate(-kLabelInset, -kLabelInset);
    }

    const RectF device =
        RectF::Around(transform.ToDevice(Point(box.left, box.top)),
                      transform.ToDevice(Point(box.right, box.bottom)));
    return device.ToRect();
}

void DrawObj::DrawLabel(Canvas& canvas, const Transform& transform, const bool fit) const {
    (void)fit;
    if (text.empty()) return;

    const pf::font_handle font = CreateStyleFont(style, transform.scale);
    if (!font) return;

    const Rect box = LabelRect(*this, transform);
    std::wstring display = text;
    if (fit) {
        const auto fits = [&] {
            auto context = pf::create_pixel_surface_context(canvas.Surface());
            return context &&
                   context->measure_text_h(pf::utf16_to_utf8(display), font).cx <= box.Width();
        };
        while (!display.empty() && !fits()) {
            const std::size_t stop = PreviousCaretStop(display, display.size());
            display.resize(stop);
            if (!display.empty()) display += L"\u2026";
        }
    }
    ui::Text(canvas, box, display, style.textColour, ui::TextAlign::center, font);
    pf::delete_font_handle(font);
}

Rect DrawObj::Bounds() const {
    Rect bounds = position.Normalized();
    long margin = style.lineWidth + 2;
    if (IsLinear(kind) && (style.startArrow || style.endArrow)) margin += ArrowSize(style);
    bounds.Inflate(margin, margin);
    return bounds;
}

bool DrawObj::HitTest(Point docPoint, long tolerance) const {
    const auto reach = (std::max<long>)(tolerance, style.lineWidth / 2 + 2);

    // Building the outline is not free and the view hit-tests on every mouse
    // move, so reject the common miss on the bounding box first.
    Rect box = Bounds();
    box.Inflate(reach, reach);
    if (docPoint.x < box.left || docPoint.x > box.right || docPoint.y < box.top ||
        docPoint.y > box.bottom)
        return false;

    Path path;
    BuildPath(kDocTransform, path);
    if (path.Empty()) return false;

    const Vec2 point = kDocTransform.ToDevice(docPoint);
    if (PathContains(path, point)) return true;

    return PathNearEdge(path, point, static_cast<float>(reach));
}

bool DrawObj::IsWithin(const Rect& docRect) const {
    const Rect fixed = position.Normalized();
    const Rect target = docRect.Normalized();
    // Compare the edges directly: ::UnionRect ignores an empty rectangle, which
    // would put every horizontal or vertical line inside any selection band.
    return fixed.left >= target.left && fixed.right <= target.right &&
           fixed.top >= target.top && fixed.bottom <= target.bottom;
}

int DrawObj::HandleCount() const { return IsLinear(kind) ? 2 : 8; }

Point DrawObj::Handle(int index) const {
    if (IsLinear(kind)) return index == 0 ? position.TopLeft() : position.BottomRight();

    const long midX = (position.left + position.right) / 2;
    const long midY = (position.top + position.bottom) / 2;

    switch (index) {
        case 0: return {position.left, position.top};
        case 1: return {midX, position.top};
        case 2: return {position.right, position.top};
        case 3: return {position.right, midY};
        case 4: return {position.right, position.bottom};
        case 5: return {midX, position.bottom};
        case 6: return {position.left, position.bottom};
        case 7: return {position.left, midY};
        default: return {position.left, position.top};
    }
}

void DrawObj::MoveHandle(int index, Point docPoint) {
    if (IsLinear(kind)) {
        if (index == 0) {
            position.left = docPoint.x;
            position.top = docPoint.y;
        } else {
            position.right = docPoint.x;
            position.bottom = docPoint.y;
        }
        return;
    }

    switch (index) {
        case 0:
            position.left = docPoint.x;
            position.top = docPoint.y;
            break;
        case 1: position.top = docPoint.y; break;
        case 2:
            position.right = docPoint.x;
            position.top = docPoint.y;
            break;
        case 3: position.right = docPoint.x; break;
        case 4:
            position.right = docPoint.x;
            position.bottom = docPoint.y;
            break;
        case 5: position.bottom = docPoint.y; break;
        case 6:
            position.left = docPoint.x;
            position.bottom = docPoint.y;
            break;
        case 7: position.left = docPoint.x; break;
        default: break;
    }
}

void DrawObj::Offset(Size delta) { position.Offset(delta.cx, delta.cy); }

Point SquareCornerTarget(const Rect& start, int handle, Point target) {
    // Handles run clockwise from the top left, so a corner's opposite is four on.
    Point anchor;
    switch (handle) {
        case 0: anchor = Point(start.right, start.bottom); break;
        case 2: anchor = Point(start.left, start.bottom); break;
        case 4: anchor = Point(start.left, start.top); break;
        case 6: anchor = Point(start.right, start.top); break;
        default: return target;
    }

    const long size = (std::max)(std::labs(target.x - anchor.x), std::labs(target.y - anchor.y));
    return Point(anchor.x + (target.x < anchor.x ? -size : size),
                 anchor.y + (target.y < anchor.y ? -size : size));
}
