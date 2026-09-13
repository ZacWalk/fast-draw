// A small software renderer.
//
// Paths are flattened to polygons and rasterized with an anti-aliased scanline
// fill straight into a Canvas. Strokes are converted to fill outlines, so one
// rasterizer covers everything. Only text still goes through GDI.

#pragma once

#include "geometry.h"

#include <cstdint>
#include <span>
#include <vector>

class Canvas;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct RectF {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    float Width() const { return right - left; }
    float Height() const { return bottom - top; }
    bool IsEmpty() const { return right <= left || bottom <= top; }
    Vec2 Centre() const { return {(left + right) * 0.5f, (top + bottom) * 0.5f}; }

    RectF Inflated(float amount) const {
        return {left - amount, top - amount, right + amount, bottom + amount};
    }

    static RectF Around(Vec2 a, Vec2 b);
    static RectF FromRect(const RECT& rect);
    RECT ToRect() const;
};

// Maps document coordinates (0.01 inch units, y axis up, origin at the centre
// of the page) to device pixels.
struct Transform {
    float scale = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    Vec2 ToDevice(Point point) const {
        return {static_cast<float>(point.x) * scale + offsetX,
                offsetY - static_cast<float>(point.y) * scale};
    }
    Point ToDoc(Vec2 point) const {
        return Point(static_cast<long>((point.x - offsetX) / scale),
                     static_cast<long>((offsetY - point.y) / scale));
    }
    float ToDevice(long length) const { return static_cast<float>(length) * scale; }

    RectF ToDevice(const Rect& rect) const {
        return RectF::Around(ToDevice(rect.TopLeft()), ToDevice(rect.BottomRight()));
    }
};

enum class GradientDirection : int {
    Down,
    Right,
    DownRight,
    UpRight,
};

inline constexpr int kGradientDirectionCount = 4;

// Colours are 0xAARRGGBB, matching Canvas.
struct Paint {
    std::uint32_t color = 0xFF000000;
    std::uint32_t endColor = 0xFF000000;
    bool gradient = false;
    GradientDirection direction = GradientDirection::Down;
    RectF gradientBounds;
};

inline Paint SolidPaint(std::uint32_t argb) {
    Paint paint;
    paint.color = argb;
    paint.endColor = argb;
    return paint;
}

struct StrokeStyle {
    float width = 1.0f;
    bool round = false;
    // Alternating on/off run lengths in device pixels. Empty means solid.
    std::span<const float> dashes;
};

// A set of contours of already-flattened points, in device pixels.
class Path {
public:
    struct Contour {
        std::vector<Vec2> points;
        bool closed = false;
    };

    void MoveTo(Vec2 point);
    void LineTo(Vec2 point);
    void CubicTo(Vec2 control1, Vec2 control2, Vec2 end);
    void Close();

    void AddRect(const RectF& rect);
    void AddRoundRect(const RectF& rect, float radius);
    void AddEllipse(const RectF& rect);
    void AddPolygon(std::span<const Vec2> points);
    void AddPolyline(std::span<const Vec2> points);

    void Clear() { contours_.clear(); }
    bool Empty() const;
    RectF Bounds() const;

    const std::vector<Contour>& Contours() const { return contours_; }

private:
    Contour& Current();

    std::vector<Contour> contours_;
};

// True when `point` is inside the path under the nonzero winding rule.
bool PathContains(const Path& path, Vec2 point);
// True when `point` is within `tolerance` of any edge. Used to hit lines.
bool PathNearEdge(const Path& path, Vec2 point, float tolerance);

// Expands a stroke into a fillable outline. Exposed for hit-testing.
void BuildStrokeOutline(const Path& path, const StrokeStyle& stroke, Path& out);

class Renderer {
public:
    explicit Renderer(Canvas& canvas);

    // Everything is clipped to this rectangle, in device pixels.
    void SetClip(const RECT& clip);
    const RECT& Clip() const { return clip_; }

    void Clear(std::uint32_t argb);
    void Fill(const Path& path, const Paint& paint);
    void Stroke(const Path& path, const Paint& paint, const StrokeStyle& stroke);

    void FillRect(const RectF& rect, std::uint32_t argb);

private:
    struct Edge {
        float x0, y0, x1, y1;
        int direction;
    };

    void BuildEdges(const Path& path);
    void Rasterize(const Paint& paint);

    Canvas& canvas_;
    RECT clip_{};
    std::vector<Edge> edges_;
    std::vector<int> order_;
    std::vector<int> active_;
    std::vector<std::pair<float, int>> crossings_;
    std::vector<float> coverage_;
    Path scratch_;
};
