#include "test_framework.h"

#include "canvas.h"
#include "render.h"
#include "shapes.h"

#include <cmath>

namespace {

Canvas MakeCanvas(int width, int height, std::uint32_t background) {
    Canvas canvas;
    canvas.Resize(width, height);
    Renderer(canvas).Clear(background);
    return canvas;
}

int Channel(std::uint32_t colour, int shift) {
    return static_cast<int>((colour >> shift) & 0xFF);
}

bool Near(int actual, int expected, int tolerance) {
    return std::abs(actual - expected) <= tolerance;
}

}  // namespace

TEST(TransformMapsDocumentSpaceWithYUp) {
    Transform transform;
    transform.scale = 2.0f;
    transform.offsetX = 100.0f;
    transform.offsetY = 50.0f;

    const Vec2 device = transform.ToDevice(Point(10, 10));
    CHECK(Near(static_cast<int>(device.x), 120, 0));
    CHECK(Near(static_cast<int>(device.y), 30, 0));

    const Point back = transform.ToDoc(device);
    CHECK(back == Point(10, 10));
}

TEST(TransformNormalizesAnInvertedRect) {
    Transform transform;
    // Document rectangles have top > bottom; the device rect must come out
    // the right way up.
    const RectF device = transform.ToDevice(Rect(-10, 20, 30, -40));
    CHECK(device.left < device.right);
    CHECK(device.top < device.bottom);
}

TEST(PathContainmentUsesTheOutline) {
    Path rect;
    rect.AddRect({10.0f, 10.0f, 50.0f, 30.0f});
    CHECK(PathContains(rect, {30.0f, 20.0f}));
    CHECK(!PathContains(rect, {5.0f, 20.0f}));

    Path ellipse;
    ellipse.AddEllipse({0.0f, 0.0f, 100.0f, 100.0f});
    CHECK(PathContains(ellipse, {50.0f, 50.0f}));
    CHECK(!PathContains(ellipse, {4.0f, 4.0f}));  // corner is outside the circle
}

TEST(OpenContoursAreNotTreatedAsAreas) {
    const Vec2 points[3] = {{0.0f, 0.0f}, {50.0f, 0.0f}, {50.0f, 50.0f}};
    Path line;
    line.AddPolyline(std::span<const Vec2>(points, 3));

    CHECK(!PathContains(line, {40.0f, 10.0f}));
    CHECK(PathNearEdge(line, {25.0f, 1.0f}, 2.0f));
    CHECK(!PathNearEdge(line, {25.0f, 20.0f}, 2.0f));
}

TEST(FillCoversTheInteriorOnly) {
    Canvas canvas = MakeCanvas(40, 40, 0xFF000000);
    Renderer renderer(canvas);

    Path path;
    path.AddRect({10.0f, 10.0f, 30.0f, 30.0f});
    renderer.Fill(path, SolidPaint(0xFFFFFFFF));

    CHECK_EQ(canvas.Row(20)[20], 0xFFFFFFFFu);
    CHECK_EQ(canvas.Row(5)[5], 0xFF000000u);
    CHECK_EQ(canvas.Row(35)[35], 0xFF000000u);
}

TEST(FillIsAntiAliasedAlongADiagonal) {
    Canvas canvas = MakeCanvas(40, 40, 0xFF000000);
    Renderer renderer(canvas);

    const Vec2 triangle[3] = {{0.0f, 0.0f}, {40.0f, 40.0f}, {0.0f, 40.0f}};
    Path path;
    path.AddPolygon(std::span<const Vec2>(triangle, 3));
    renderer.Fill(path, SolidPaint(0xFFFFFFFF));

    // Somewhere along the hypotenuse there must be a partially covered pixel.
    bool partial = false;
    for (int y = 5; y < 35; ++y) {
        const int value = Channel(canvas.Row(y)[y], 0);
        if (value > 20 && value < 235) partial = true;
    }
    CHECK(partial);
}

TEST(ClippingLimitsWhatIsTouched) {
    Canvas canvas = MakeCanvas(40, 40, 0xFF000000);
    Renderer renderer(canvas);
    renderer.SetClip(RECT{0, 0, 20, 40});

    Path path;
    path.AddRect({0.0f, 0.0f, 40.0f, 40.0f});
    renderer.Fill(path, SolidPaint(0xFFFFFFFF));

    CHECK_EQ(canvas.Row(20)[10], 0xFFFFFFFFu);
    CHECK_EQ(canvas.Row(20)[30], 0xFF000000u);
}

TEST(StrokeFollowsTheLine) {
    Canvas canvas = MakeCanvas(60, 60, 0xFF000000);
    Renderer renderer(canvas);

    const Vec2 points[2] = {{5.0f, 30.0f}, {55.0f, 30.0f}};
    Path path;
    path.AddPolyline(std::span<const Vec2>(points, 2));
    renderer.Stroke(path, SolidPaint(0xFFFFFFFF), StrokeStyle{6.0f});

    CHECK(Channel(canvas.Row(30)[30], 0) > 200);
    CHECK(Channel(canvas.Row(10)[30], 0) < 20);
}

TEST(DashedStrokesLeaveGaps) {
    Canvas canvas = MakeCanvas(120, 20, 0xFF000000);
    Renderer renderer(canvas);

    const Vec2 points[2] = {{0.0f, 10.0f}, {120.0f, 10.0f}};
    Path path;
    path.AddPolyline(std::span<const Vec2>(points, 2));

    const float dashes[2] = {6.0f, 6.0f};
    renderer.Stroke(path, SolidPaint(0xFFFFFFFF),
                    StrokeStyle{3.0f, false, std::span<const float>(dashes, 2)});

    int lit = 0;
    for (int x = 0; x < 120; ++x) {
        if (Channel(canvas.Row(10)[x], 0) > 128) ++lit;
    }
    // Roughly half of the run should be painted.
    CHECK(lit > 40 && lit < 80);
}

TEST(LinearGradientRunsFromStartToEnd) {
    Canvas canvas = MakeCanvas(20, 100, 0xFF000000);
    Renderer renderer(canvas);

    Paint paint;
    paint.color = 0xFFFF0000;
    paint.endColor = 0xFF0000FF;
    paint.gradient = true;
    paint.direction = GradientDirection::Down;
    paint.gradientBounds = {0.0f, 0.0f, 20.0f, 100.0f};

    Path path;
    path.AddRect(paint.gradientBounds);
    renderer.Fill(path, paint);

    CHECK(Near(Channel(canvas.Row(1)[10], 16), 255, 12));   // red at the top
    CHECK(Near(Channel(canvas.Row(98)[10], 0), 255, 12));   // blue at the bottom
    CHECK(Near(Channel(canvas.Row(50)[10], 16), 128, 20));  // halfway
}

TEST(EveryShapeProducesGeometry) {
    for (int i = 0; i < kShapeKindCount; ++i) {
        Path path;
        BuildShapePath(static_cast<ShapeKind>(i), {0.0f, 0.0f, 100.0f, 80.0f}, path);
        CHECK(!path.Empty());

        const RectF bounds = path.Bounds();
        CHECK(bounds.Width() > 0.0f);
        CHECK(bounds.Height() > 0.0f);
    }
}

TEST(ArrowHeadPointsAtTheTip) {
    Path head;
    BuildArrowHead({100.0f, 50.0f}, {0.0f, 50.0f}, 10.0f, head);
    CHECK(!head.Empty());

    const RectF bounds = head.Bounds();
    CHECK(Near(static_cast<int>(bounds.right), 100, 1));
    CHECK(Near(static_cast<int>(bounds.left), 90, 1));
}
