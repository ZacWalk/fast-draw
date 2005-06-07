#include "shapes.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {

// Control point offset for a quarter arc.
constexpr float kArc = 0.5523f;

struct ShapeInfo {
    std::string_view name;
    const wchar_t* label;
};

// Indexed by ShapeKind.
constexpr ShapeInfo kShapes[] = {
    {"rectangle", L"Rectangle"},   {"roundRect", L"Rounded"},
    {"ellipse", L"Ellipse"},       {"diamond", L"Decision"},
    {"triangle", L"Triangle"},     {"hexagon", L"Service"},
    {"parallelogram", L"Data"},    {"chevron", L"Stage"},
    {"cylinder", L"Database"},     {"component", L"Component"},
    {"document", L"Document"},     {"note", L"Note"},
    {"cloud", L"Cloud"},           {"actor", L"Actor"},
    {"line", L"Line"},             {"arrow", L"Arrow"},
};

static_assert(std::size(kShapes) == kShapeKindCount);

void AddPolygon(Path& out, std::initializer_list<Vec2> points) {
    out.AddPolygon(std::span<const Vec2>(points.begin(), points.size()));
}

void AddLine(Path& out, Vec2 a, Vec2 b) {
    const Vec2 points[2] = {a, b};
    out.AddPolyline(std::span<const Vec2>(points, 2));
}

void AddCylinder(Path& out, const RectF& r) {
    const float rx = r.Width() * 0.5f;
    const float ry = (std::min)(r.Height() * 0.28f, r.Width() * 0.5f) * 0.5f;
    const float cx = r.Centre().x;
    const float topY = r.top + ry;
    const float bottomY = r.bottom - ry;

    out.MoveTo({r.left, topY});
    out.CubicTo({r.left, topY - ry * kArc}, {cx - rx * kArc, r.top}, {cx, r.top});
    out.CubicTo({cx + rx * kArc, r.top}, {r.right, topY - ry * kArc}, {r.right, topY});
    out.LineTo({r.right, bottomY});
    out.CubicTo({r.right, bottomY + ry * kArc}, {cx + rx * kArc, r.bottom}, {cx, r.bottom});
    out.CubicTo({cx - rx * kArc, r.bottom}, {r.left, bottomY + ry * kArc}, {r.left, bottomY});
    out.Close();

    out.AddEllipse({r.left, r.top, r.right, r.top + ry * 2.0f});
}

void AddDocument(Path& out, const RectF& r) {
    const float wave = r.Height() * 0.16f;
    const float w = r.Width();

    out.MoveTo({r.left, r.top});
    out.LineTo({r.right, r.top});
    out.LineTo({r.right, r.bottom - wave});
    out.CubicTo({r.right - w * 0.28f, r.bottom - wave * 2.4f},
                {r.left + w * 0.28f, r.bottom + wave * 0.9f}, {r.left, r.bottom - wave});
    out.Close();
}

void AddNote(Path& out, const RectF& r) {
    const float fold = (std::min)(r.Width(), r.Height()) * 0.3f;

    AddPolygon(out, {{r.left, r.top},
                     {r.right - fold, r.top},
                     {r.right, r.top + fold},
                     {r.right, r.bottom},
                     {r.left, r.bottom}});
    AddPolygon(out, {{r.right - fold, r.top},
                     {r.right, r.top + fold},
                     {r.right - fold, r.top + fold}});
}

void AddComponent(Path& out, const RectF& r) {
    const float tabWidth = (std::min)(r.Width() * 0.3f, r.Height() * 0.45f);
    const float tabHeight = (std::min)(r.Height() * 0.18f, tabWidth * 0.7f);

    out.AddRect({r.left + tabWidth * 0.5f, r.top, r.right, r.bottom});
    out.AddRect({r.left, r.top + r.Height() * 0.2f, r.left + tabWidth,
                 r.top + r.Height() * 0.2f + tabHeight});
    out.AddRect({r.left, r.bottom - r.Height() * 0.2f - tabHeight, r.left + tabWidth,
                 r.bottom - r.Height() * 0.2f});
}

void AddCloud(Path& out, const RectF& r) {
    constexpr int kBumps = 9;
    const Vec2 centre = r.Centre();
    const float rx = r.Width() * 0.36f;
    const float ry = r.Height() * 0.32f;
    const float bulge = (std::min)(r.Width(), r.Height()) * 0.30f;

    std::array<Vec2, kBumps> ring{};
    for (int i = 0; i < kBumps; ++i) {
        const float angle = 6.2831853f * i / kBumps - 1.5707963f;
        ring[static_cast<std::size_t>(i)] = {centre.x + std::cos(angle) * rx,
                                             centre.y + std::sin(angle) * ry};
    }

    out.MoveTo(ring[0]);
    for (int i = 0; i < kBumps; ++i) {
        const Vec2 a = ring[static_cast<std::size_t>(i)];
        const Vec2 b = ring[static_cast<std::size_t>((i + 1) % kBumps)];
        const Vec2 mid{(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f};

        Vec2 outward{mid.x - centre.x, mid.y - centre.y};
        const float length = std::sqrt(outward.x * outward.x + outward.y * outward.y);
        if (length > 1e-4f) {
            outward.x = outward.x / length * bulge;
            outward.y = outward.y / length * bulge;
        }

        out.CubicTo({a.x + outward.x, a.y + outward.y}, {b.x + outward.x, b.y + outward.y}, b);
    }
    out.Close();
}

void AddActor(Path& out, const RectF& r) {
    const float headRadius = (std::min)(r.Width() * 0.3f, r.Height() * 0.22f) * 0.5f;
    const float cx = r.Centre().x;
    const float neck = r.top + headRadius * 2.0f;
    const float hip = r.top + r.Height() * 0.62f;
    const float reach = (std::min)(r.Width() * 0.5f, r.Height() * 0.45f);

    out.AddEllipse({cx - headRadius, r.top, cx + headRadius, neck});
    AddLine(out, {cx, neck}, {cx, hip});
    AddLine(out, {cx - reach, neck + (hip - neck) * 0.35f},
            {cx + reach, neck + (hip - neck) * 0.35f});
    AddLine(out, {cx, hip}, {cx - reach * 0.8f, r.bottom});
    AddLine(out, {cx, hip}, {cx + reach * 0.8f, r.bottom});
}

}  // namespace

std::string_view ShapeKindName(ShapeKind kind) {
    const auto index = static_cast<std::size_t>(kind);
    return index < std::size(kShapes) ? kShapes[index].name : kShapes[0].name;
}

ShapeKind ShapeKindFromName(std::string_view name, ShapeKind fallback) {
    for (std::size_t i = 0; i < std::size(kShapes); ++i) {
        if (kShapes[i].name == name) return static_cast<ShapeKind>(i);
    }
    return fallback;
}

const wchar_t* ShapeKindLabel(ShapeKind kind) {
    const auto index = static_cast<std::size_t>(kind);
    return index < std::size(kShapes) ? kShapes[index].label : kShapes[0].label;
}

void BuildLinePath(Vec2 start, Vec2 end, Path& out) {
    const Vec2 points[2] = {start, end};
    out.AddPolyline(std::span<const Vec2>(points, 2));
}

void BuildArrowHead(Vec2 tip, Vec2 from, float size, Path& out) {
    Vec2 direction{tip.x - from.x, tip.y - from.y};
    const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length < 1e-3f || size <= 0.0f) return;

    direction.x /= length;
    direction.y /= length;

    const Vec2 base{tip.x - direction.x * size, tip.y - direction.y * size};
    const float half = size * 0.42f;
    const Vec2 normal{-direction.y * half, direction.x * half};

    AddPolygon(out, {tip, {base.x + normal.x, base.y + normal.y},
                     {base.x - normal.x, base.y - normal.y}});
}

void BuildShapePath(ShapeKind kind, const RectF& bounds, Path& out) {
    RectF r = bounds;
    if (r.Width() < 1.0f) r.right = r.left + 1.0f;
    if (r.Height() < 1.0f) r.bottom = r.top + 1.0f;

    const float w = r.Width();
    const float h = r.Height();
    const Vec2 centre = r.Centre();

    switch (kind) {
        case ShapeKind::Rectangle: out.AddRect(r); break;

        case ShapeKind::RoundRect: out.AddRoundRect(r, (std::min)(w, h) * 0.2f); break;

        case ShapeKind::Ellipse: out.AddEllipse(r); break;

        case ShapeKind::Diamond:
            AddPolygon(out, {{centre.x, r.top},
                             {r.right, centre.y},
                             {centre.x, r.bottom},
                             {r.left, centre.y}});
            break;

        case ShapeKind::Triangle:
            AddPolygon(out, {{centre.x, r.top}, {r.right, r.bottom}, {r.left, r.bottom}});
            break;

        case ShapeKind::Hexagon: {
            const float inset = w * 0.22f;
            AddPolygon(out, {{r.left + inset, r.top},
                             {r.right - inset, r.top},
                             {r.right, centre.y},
                             {r.right - inset, r.bottom},
                             {r.left + inset, r.bottom},
                             {r.left, centre.y}});
            break;
        }

        case ShapeKind::Parallelogram: {
            const float inset = w * 0.22f;
            AddPolygon(out, {{r.left + inset, r.top},
                             {r.right, r.top},
                             {r.right - inset, r.bottom},
                             {r.left, r.bottom}});
            break;
        }

        case ShapeKind::Chevron: {
            const float inset = (std::min)(w * 0.25f, h * 0.5f);
            AddPolygon(out, {{r.left, r.top},
                             {r.right - inset, r.top},
                             {r.right, centre.y},
                             {r.right - inset, r.bottom},
                             {r.left, r.bottom},
                             {r.left + inset, centre.y}});
            break;
        }

        case ShapeKind::Cylinder: AddCylinder(out, r); break;
        case ShapeKind::Component: AddComponent(out, r); break;
        case ShapeKind::Document: AddDocument(out, r); break;
        case ShapeKind::Note: AddNote(out, r); break;
        case ShapeKind::Cloud: AddCloud(out, r); break;
        case ShapeKind::Actor: AddActor(out, r); break;

        case ShapeKind::Line:
        case ShapeKind::Arrow: BuildLinePath({r.left, r.bottom}, {r.right, r.top}, out); break;
    }
}
