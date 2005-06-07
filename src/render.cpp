#include "render.h"

#include "canvas.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

// Vertical samples per pixel row. Horizontal coverage is exact.
constexpr int kSubSamples = 4;
constexpr float kSubWeight = 1.0f / kSubSamples;

// Circles and round joins are approximated with this many segments.
constexpr int kCircleSegments = 24;

Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
Vec2 operator*(Vec2 a, float f) { return {a.x * f, a.y * f}; }

float Length(Vec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }

Vec2 Normalized(Vec2 v) {
    const float length = Length(v);
    if (length < 1e-6f) return {0.0f, 0.0f};
    return {v.x / length, v.y / length};
}

std::uint8_t Lerp(std::uint8_t a, std::uint8_t b, float t) {
    return static_cast<std::uint8_t>(a + (b - a) * t + 0.5f);
}

std::uint32_t LerpColor(std::uint32_t a, std::uint32_t b, float t) {
    t = (std::clamp)(t, 0.0f, 1.0f);
    return (static_cast<std::uint32_t>(Lerp(static_cast<std::uint8_t>(a >> 24),
                                            static_cast<std::uint8_t>(b >> 24), t))
            << 24) |
           (static_cast<std::uint32_t>(Lerp(static_cast<std::uint8_t>(a >> 16),
                                            static_cast<std::uint8_t>(b >> 16), t))
            << 16) |
           (static_cast<std::uint32_t>(Lerp(static_cast<std::uint8_t>(a >> 8),
                                            static_cast<std::uint8_t>(b >> 8), t))
            << 8) |
           static_cast<std::uint32_t>(
               Lerp(static_cast<std::uint8_t>(a), static_cast<std::uint8_t>(b), t));
}

// Source-over blend. `alpha` is the already-combined source alpha and coverage.
std::uint32_t BlendPixel(std::uint32_t dst, std::uint32_t src, unsigned alpha) {
    if (alpha == 0) return dst;
    if (alpha >= 255) return src | 0xFF000000u;

    const unsigned inverse = 255 - alpha;
    const unsigned r = ((src >> 16 & 0xFF) * alpha + (dst >> 16 & 0xFF) * inverse + 127) / 255;
    const unsigned g = ((src >> 8 & 0xFF) * alpha + (dst >> 8 & 0xFF) * inverse + 127) / 255;
    const unsigned b = ((src & 0xFF) * alpha + (dst & 0xFF) * inverse + 127) / 255;
    const unsigned a = (std::max)(dst >> 24, alpha);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

// Distance from `p` to the segment ab.
float DistanceToSegment(Vec2 p, Vec2 a, Vec2 b) {
    const Vec2 ab = b - a;
    const float lengthSquared = ab.x * ab.x + ab.y * ab.y;
    if (lengthSquared < 1e-9f) return Length(p - a);

    float t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lengthSquared;
    t = (std::clamp)(t, 0.0f, 1.0f);
    return Length(p - (a + ab * t));
}

}  // namespace

RectF RectF::Around(Vec2 a, Vec2 b) {
    return {(std::min)(a.x, b.x), (std::min)(a.y, b.y), (std::max)(a.x, b.x),
            (std::max)(a.y, b.y)};
}

RectF RectF::FromRect(const RECT& rect) {
    return {static_cast<float>(rect.left), static_cast<float>(rect.top),
            static_cast<float>(rect.right), static_cast<float>(rect.bottom)};
}

RECT RectF::ToRect() const {
    return RECT{static_cast<long>(std::floor(left)), static_cast<long>(std::floor(top)),
                static_cast<long>(std::ceil(right)), static_cast<long>(std::ceil(bottom))};
}

// ------------------------------------------------------------------- Path

Path::Contour& Path::Current() {
    if (contours_.empty() || contours_.back().closed) contours_.emplace_back();
    return contours_.back();
}

void Path::MoveTo(Vec2 point) {
    contours_.emplace_back();
    contours_.back().points.push_back(point);
}

void Path::LineTo(Vec2 point) {
    Contour& contour = Current();
    if (!contour.points.empty()) {
        const Vec2 last = contour.points.back();
        if (std::fabs(last.x - point.x) < 1e-5f && std::fabs(last.y - point.y) < 1e-5f) return;
    }
    contour.points.push_back(point);
}

void Path::CubicTo(Vec2 control1, Vec2 control2, Vec2 end) {
    Contour& contour = Current();
    if (contour.points.empty()) contour.points.push_back(control1);

    const Vec2 start = contour.points.back();
    // Segment count from the control polygon length; plenty for screen work.
    const float rough =
        Length(control1 - start) + Length(control2 - control1) + Length(end - control2);
    const int steps = (std::clamp)(static_cast<int>(rough / 3.0f), 4, 48);

    for (int i = 1; i <= steps; ++i) {
        const float t = static_cast<float>(i) / steps;
        const float u = 1.0f - t;
        const Vec2 point = start * (u * u * u) + control1 * (3 * u * u * t) +
                           control2 * (3 * u * t * t) + end * (t * t * t);
        contour.points.push_back(point);
    }
}

void Path::Close() {
    if (contours_.empty()) return;
    contours_.back().closed = true;
}

void Path::AddRect(const RectF& rect) {
    MoveTo({rect.left, rect.top});
    LineTo({rect.right, rect.top});
    LineTo({rect.right, rect.bottom});
    LineTo({rect.left, rect.bottom});
    Close();
}

void Path::AddRoundRect(const RectF& rect, float radius) {
    radius = (std::min)(radius, (std::min)(rect.Width(), rect.Height()) * 0.5f);
    if (radius <= 0.5f) {
        AddRect(rect);
        return;
    }

    // Control point offset for a quarter circle.
    const float k = radius * 0.5523f;

    MoveTo({rect.left + radius, rect.top});
    LineTo({rect.right - radius, rect.top});
    CubicTo({rect.right - radius + k, rect.top}, {rect.right, rect.top + radius - k},
            {rect.right, rect.top + radius});
    LineTo({rect.right, rect.bottom - radius});
    CubicTo({rect.right, rect.bottom - radius + k}, {rect.right - radius + k, rect.bottom},
            {rect.right - radius, rect.bottom});
    LineTo({rect.left + radius, rect.bottom});
    CubicTo({rect.left + radius - k, rect.bottom}, {rect.left, rect.bottom - radius + k},
            {rect.left, rect.bottom - radius});
    LineTo({rect.left, rect.top + radius});
    CubicTo({rect.left, rect.top + radius - k}, {rect.left + radius - k, rect.top},
            {rect.left + radius, rect.top});
    Close();
}

void Path::AddEllipse(const RectF& rect) {
    const Vec2 centre = rect.Centre();
    const float rx = rect.Width() * 0.5f;
    const float ry = rect.Height() * 0.5f;
    const float kx = rx * 0.5523f;
    const float ky = ry * 0.5523f;

    MoveTo({centre.x, centre.y - ry});
    CubicTo({centre.x + kx, centre.y - ry}, {centre.x + rx, centre.y - ky},
            {centre.x + rx, centre.y});
    CubicTo({centre.x + rx, centre.y + ky}, {centre.x + kx, centre.y + ry},
            {centre.x, centre.y + ry});
    CubicTo({centre.x - kx, centre.y + ry}, {centre.x - rx, centre.y + ky},
            {centre.x - rx, centre.y});
    CubicTo({centre.x - rx, centre.y - ky}, {centre.x - kx, centre.y - ry},
            {centre.x, centre.y - ry});
    Close();
}

void Path::AddPolygon(std::span<const Vec2> points) {
    if (points.empty()) return;
    MoveTo(points.front());
    for (std::size_t i = 1; i < points.size(); ++i) LineTo(points[i]);
    Close();
}

void Path::AddPolyline(std::span<const Vec2> points) {
    if (points.empty()) return;
    MoveTo(points.front());
    for (std::size_t i = 1; i < points.size(); ++i) LineTo(points[i]);
}

bool Path::Empty() const {
    for (const auto& contour : contours_) {
        if (contour.points.size() >= 2) return false;
    }
    return true;
}

RectF Path::Bounds() const {
    RectF bounds{1e30f, 1e30f, -1e30f, -1e30f};
    bool any = false;
    for (const auto& contour : contours_) {
        for (const Vec2& point : contour.points) {
            bounds.left = (std::min)(bounds.left, point.x);
            bounds.top = (std::min)(bounds.top, point.y);
            bounds.right = (std::max)(bounds.right, point.x);
            bounds.bottom = (std::max)(bounds.bottom, point.y);
            any = true;
        }
    }
    return any ? bounds : RectF{};
}

// ------------------------------------------------------------- hit testing

bool PathContains(const Path& path, Vec2 point) {
    int winding = 0;
    for (const auto& contour : path.Contours()) {
        const auto& points = contour.points;
        if (!contour.closed || points.size() < 3) continue;

        for (std::size_t i = 0; i < points.size(); ++i) {
            const Vec2 a = points[i];
            const Vec2 b = points[(i + 1) % points.size()];
            if (a.y == b.y) continue;

            if ((a.y <= point.y) == (b.y <= point.y)) continue;
            const float x = a.x + (point.y - a.y) * (b.x - a.x) / (b.y - a.y);
            if (x > point.x) winding += (b.y > a.y) ? 1 : -1;
        }
    }
    return winding != 0;
}

bool PathNearEdge(const Path& path, Vec2 point, float tolerance) {
    for (const auto& contour : path.Contours()) {
        const auto& points = contour.points;
        if (points.size() < 2) continue;

        const std::size_t count = contour.closed ? points.size() : points.size() - 1;
        for (std::size_t i = 0; i < count; ++i) {
            const Vec2 a = points[i];
            const Vec2 b = points[(i + 1) % points.size()];
            if (DistanceToSegment(point, a, b) <= tolerance) return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------- stroking

namespace {

// Emits a convex polygon with a consistent winding direction so that
// overlapping pieces union under the nonzero rule.
void EmitConvex(Path& out, std::span<Vec2> points) {
    if (points.size() < 3) return;

    float area = 0.0f;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const Vec2& a = points[i];
        const Vec2& b = points[(i + 1) % points.size()];
        area += a.x * b.y - b.x * a.y;
    }
    if (area < 0.0f) std::reverse(points.begin(), points.end());

    out.AddPolygon(points);
}

void EmitDisc(Path& out, Vec2 centre, float radius) {
    if (radius <= 0.05f) return;

    Vec2 points[kCircleSegments];
    for (int i = 0; i < kCircleSegments; ++i) {
        const float angle = 6.2831853f * i / kCircleSegments;
        points[i] = {centre.x + std::cos(angle) * radius, centre.y + std::sin(angle) * radius};
    }
    EmitConvex(out, std::span<Vec2>(points, kCircleSegments));
}

void EmitSegment(Path& out, Vec2 a, Vec2 b, float half) {
    const Vec2 direction = Normalized(b - a);
    if (direction.x == 0.0f && direction.y == 0.0f) return;

    const Vec2 normal{-direction.y * half, direction.x * half};
    Vec2 quad[4] = {a + normal, b + normal, b - normal, a - normal};
    EmitConvex(out, std::span<Vec2>(quad, 4));
}

// Joins two segments meeting at `corner`, mitring when the angle allows.
void EmitJoin(Path& out, Vec2 previous, Vec2 corner, Vec2 next, float half, bool round) {
    if (round) {
        EmitDisc(out, corner, half);
        return;
    }

    const Vec2 in = Normalized(corner - previous);
    const Vec2 outward = Normalized(next - corner);
    const float cross = in.x * outward.y - in.y * outward.x;
    if (std::fabs(cross) < 1e-4f) return;

    // Outer side of the turn.
    const float sign = cross > 0.0f ? -1.0f : 1.0f;
    const Vec2 a{corner.x - in.y * half * sign, corner.y + in.x * half * sign};
    const Vec2 b{corner.x - outward.y * half * sign, corner.y + outward.x * half * sign};

    const Vec2 bisector = Normalized(Normalized(a - corner) + Normalized(b - corner));
    const float cosHalf = bisector.x * (a.x - corner.x) / half + bisector.y * (a.y - corner.y) / half;

    if (cosHalf > 0.25f) {  // miter length within about 4x the stroke width
        const Vec2 tip = corner + bisector * (half / cosHalf);
        Vec2 points[4] = {corner, a, tip, b};
        EmitConvex(out, std::span<Vec2>(points, 4));
    } else {
        Vec2 points[3] = {corner, a, b};
        EmitConvex(out, std::span<Vec2>(points, 3));
    }
}

void StrokePolyline(Path& out, std::span<const Vec2> points, bool closed, float half, bool round,
                    bool capEnds) {
    if (points.size() < 2) {
        if (points.size() == 1 && round) EmitDisc(out, points[0], half);
        return;
    }

    const std::size_t segments = closed ? points.size() : points.size() - 1;
    for (std::size_t i = 0; i < segments; ++i)
        EmitSegment(out, points[i], points[(i + 1) % points.size()], half);

    const std::size_t joins = closed ? points.size() : points.size() - 1;
    for (std::size_t i = closed ? 0 : 1; i < joins; ++i) {
        const Vec2 previous = points[(i + points.size() - 1) % points.size()];
        const Vec2 next = points[(i + 1) % points.size()];
        EmitJoin(out, previous, points[i], next, half, round);
    }

    if (!closed && capEnds) {
        EmitDisc(out, points.front(), half);
        EmitDisc(out, points.back(), half);
    }
}

// Splits a polyline into the "on" runs of the dash pattern.
void ApplyDashes(std::span<const Vec2> points, bool closed, std::span<const float> pattern,
                 std::vector<std::vector<Vec2>>& out) {
    std::size_t index = 0;
    float remaining = (std::max)(pattern[0], 0.01f);
    bool on = true;

    std::vector<Vec2> run;
    if (on) run.push_back(points.front());

    const std::size_t segments = closed ? points.size() : points.size() - 1;
    for (std::size_t i = 0; i < segments; ++i) {
        Vec2 a = points[i];
        const Vec2 b = points[(i + 1) % points.size()];
        float length = Length(b - a);
        const Vec2 direction = Normalized(b - a);

        while (length > remaining) {
            a = a + direction * remaining;
            length -= remaining;

            if (on) {
                run.push_back(a);
                out.push_back(std::move(run));
                run.clear();
            } else {
                run.push_back(a);
            }
            on = !on;
            index = (index + 1) % pattern.size();
            remaining = (std::max)(pattern[index], 0.01f);
        }

        remaining -= length;
        if (on) run.push_back(b);
    }

    if (run.size() >= 2) out.push_back(std::move(run));
}

}  // namespace

void BuildStrokeOutline(const Path& path, const StrokeStyle& stroke, Path& out) {
    const float half = (std::max)(stroke.width, 0.35f) * 0.5f;

    for (const auto& contour : path.Contours()) {
        if (contour.points.empty()) continue;

        if (stroke.dashes.empty()) {
            StrokePolyline(out, contour.points, contour.closed, half, stroke.round, stroke.round);
            continue;
        }

        std::vector<std::vector<Vec2>> runs;
        ApplyDashes(contour.points, contour.closed, stroke.dashes, runs);
        for (const auto& run : runs)
            StrokePolyline(out, run, false, half, stroke.round, stroke.round);
    }
}

// --------------------------------------------------------------- Renderer

Renderer::Renderer(Canvas& canvas) : canvas_(canvas) {
    clip_ = RECT{0, 0, canvas.Width(), canvas.Height()};
}

void Renderer::SetClip(const RECT& clip) {
    clip_.left = (std::max)(0L, clip.left);
    clip_.top = (std::max)(0L, clip.top);
    clip_.right = (std::min)(static_cast<long>(canvas_.Width()), clip.right);
    clip_.bottom = (std::min)(static_cast<long>(canvas_.Height()), clip.bottom);
}

void Renderer::Clear(std::uint32_t argb) {
    for (long y = clip_.top; y < clip_.bottom; ++y) {
        std::uint32_t* row = canvas_.Row(static_cast<int>(y));
        std::fill(row + clip_.left, row + clip_.right, argb);
    }
}

void Renderer::FillRect(const RectF& rect, std::uint32_t argb) {
    scratch_.Clear();
    scratch_.AddRect(rect);
    Fill(scratch_, SolidPaint(argb));
}


void Renderer::BuildEdges(const Path& path) {
    edges_.clear();
    for (const auto& contour : path.Contours()) {
        const auto& points = contour.points;
        if (!contour.closed || points.size() < 3) continue;

        for (std::size_t i = 0; i < points.size(); ++i) {
            Vec2 a = points[i];
            Vec2 b = points[(i + 1) % points.size()];
            if (a.y == b.y) continue;

            int direction = 1;
            if (a.y > b.y) {
                std::swap(a, b);
                direction = -1;
            }
            edges_.push_back(Edge{a.x, a.y, b.x, b.y, direction});
        }
    }
}

void Renderer::Fill(const Path& path, const Paint& paint) {
    BuildEdges(path);
    Rasterize(paint);
}

void Renderer::Stroke(const Path& path, const Paint& paint, const StrokeStyle& stroke) {
    Path outline;
    BuildStrokeOutline(path, stroke, outline);
    Fill(outline, paint);
}

void Renderer::Rasterize(const Paint& paint) {
    if (edges_.empty() || clip_.right <= clip_.left || clip_.bottom <= clip_.top) return;

    float minY = 1e30f, maxY = -1e30f, minX = 1e30f, maxX = -1e30f;
    for (const Edge& edge : edges_) {
        minY = (std::min)(minY, edge.y0);
        maxY = (std::max)(maxY, edge.y1);
        minX = (std::min)(minX, (std::min)(edge.x0, edge.x1));
        maxX = (std::max)(maxX, (std::max)(edge.x0, edge.x1));
    }

    const long top = (std::max)(clip_.top, static_cast<long>(std::floor(minY)));
    const long bottom = (std::min)(clip_.bottom, static_cast<long>(std::ceil(maxY)) + 1);
    const long left = (std::max)(clip_.left, static_cast<long>(std::floor(minX)));
    const long right = (std::min)(clip_.right, static_cast<long>(std::ceil(maxX)) + 1);
    if (top >= bottom || left >= right) return;

    const auto width = static_cast<std::size_t>(right - left);
    coverage_.assign(width, 0.0f);

    order_.resize(edges_.size());
    for (std::size_t i = 0; i < edges_.size(); ++i) order_[i] = static_cast<int>(i);
    std::sort(order_.begin(), order_.end(),
              [this](int a, int b) { return edges_[a].y0 < edges_[b].y0; });

    active_.clear();
    std::size_t nextEdge = 0;

    // Per-row gradient parameters.
    const RectF& area = paint.gradientBounds;
    const float spanX = area.Width() > 0.0f ? area.Width() : 1.0f;
    const float spanY = area.Height() > 0.0f ? area.Height() : 1.0f;

    for (long y = top; y < bottom; ++y) {
        std::fill(coverage_.begin(), coverage_.end(), 0.0f);

        for (int sample = 0; sample < kSubSamples; ++sample) {
            const float sampleY = static_cast<float>(y) + (sample + 0.5f) * kSubWeight;

            while (nextEdge < order_.size() && edges_[order_[nextEdge]].y0 <= sampleY)
                active_.push_back(order_[nextEdge++]);

            active_.erase(std::remove_if(active_.begin(), active_.end(),
                                         [&](int index) { return edges_[index].y1 <= sampleY; }),
                          active_.end());

            crossings_.clear();
            for (int index : active_) {
                const Edge& edge = edges_[index];
                if (sampleY < edge.y0 || sampleY >= edge.y1) continue;
                const float x =
                    edge.x0 + (sampleY - edge.y0) * (edge.x1 - edge.x0) / (edge.y1 - edge.y0);
                crossings_.emplace_back(x, edge.direction);
            }
            if (crossings_.size() < 2) continue;

            std::sort(crossings_.begin(), crossings_.end(),
                      [](const auto& a, const auto& b) { return a.first < b.first; });

            int winding = 0;
            float spanStart = 0.0f;
            for (const auto& [x, direction] : crossings_) {
                const int previous = winding;
                winding += direction;
                if (previous == 0 && winding != 0) {
                    spanStart = x;
                } else if (previous != 0 && winding == 0) {
                    // Accumulate exact horizontal coverage for [spanStart, x).
                    float x0 = (std::max)(spanStart, static_cast<float>(left));
                    float x1 = (std::min)(x, static_cast<float>(right));
                    if (x1 <= x0) continue;

                    auto first = static_cast<long>(std::floor(x0));
                    auto last = static_cast<long>(std::floor(x1));
                    if (first == last) {
                        coverage_[static_cast<std::size_t>(first - left)] +=
                            (x1 - x0) * kSubWeight;
                        continue;
                    }

                    coverage_[static_cast<std::size_t>(first - left)] +=
                        (static_cast<float>(first + 1) - x0) * kSubWeight;
                    for (long px = first + 1; px < last; ++px)
                        coverage_[static_cast<std::size_t>(px - left)] += kSubWeight;
                    if (last < right)
                        coverage_[static_cast<std::size_t>(last - left)] +=
                            (x1 - static_cast<float>(last)) * kSubWeight;
                }
            }
        }

        std::uint32_t* row = canvas_.Row(static_cast<int>(y));
        const float rowT = (static_cast<float>(y) + 0.5f - area.top) / spanY;

        for (std::size_t i = 0; i < width; ++i) {
            const float value = coverage_[i];
            if (value < 0.002f) continue;

            std::uint32_t source = paint.color;
            if (paint.gradient) {
                const float columnT =
                    (static_cast<float>(left + static_cast<long>(i)) + 0.5f - area.left) / spanX;
                float t = 0.0f;
                switch (paint.direction) {
                    case GradientDirection::Down: t = rowT; break;
                    case GradientDirection::Right: t = columnT; break;
                    case GradientDirection::DownRight: t = (columnT + rowT) * 0.5f; break;
                    case GradientDirection::UpRight: t = (columnT + 1.0f - rowT) * 0.5f; break;
                }
                source = LerpColor(paint.color, paint.endColor, t);
            }

            const float alpha = (std::min)(value, 1.0f) * static_cast<float>(source >> 24) / 255.0f;
            row[left + static_cast<long>(i)] =
                BlendPixel(row[left + static_cast<long>(i)], source,
                           static_cast<unsigned>(alpha * 255.0f + 0.5f));
        }
    }
}
