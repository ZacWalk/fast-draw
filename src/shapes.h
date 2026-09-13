// The shape vocabulary: one geometry definition per shape, shared by the
// canvas, the hit tester and the palette previews.

#pragma once

#include "render.h"

#include <string_view>

enum class ShapeKind : int {
    Rectangle,
    RoundRect,
    Ellipse,
    Diamond,
    Triangle,
    Hexagon,
    Parallelogram,
    Chevron,
    Cylinder,
    Component,
    Document,
    Note,
    Cloud,
    Actor,
    Line,
    Arrow,
};

inline constexpr int kShapeKindCount = 16;

// Line and Arrow are open, two-point shapes; everything else is a closed area.
constexpr bool IsLinear(ShapeKind kind) {
    return kind == ShapeKind::Line || kind == ShapeKind::Arrow;
}

std::string_view ShapeKindName(ShapeKind kind);
ShapeKind ShapeKindFromName(std::string_view name, ShapeKind fallback);
const wchar_t* ShapeKindLabel(ShapeKind kind);

// Appends the outline of `kind` inscribed in `bounds`. Linear shapes become the
// diagonal of the rectangle, which is what the palette previews want; objects
// use BuildLinePath so the drag direction is preserved.
void BuildShapePath(ShapeKind kind, const RectF& bounds, Path& out);

void BuildLinePath(Vec2 start, Vec2 end, Path& out);

// A filled triangular head pointing at `tip`, aimed away from `from`.
void BuildArrowHead(Vec2 tip, Vec2 from, float size, Path& out);
