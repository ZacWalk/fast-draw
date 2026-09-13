#include "test_framework.h"

#include "geometry.h"
#include "style.h"

TEST(RectDimensions) {
    const Rect rect(10, 20, 40, 60);
    CHECK_EQ(rect.Width(), 30L);
    CHECK_EQ(rect.Height(), 40L);
    CHECK(rect.TopLeft() == Point(10, 20));
    CHECK(rect.BottomRight() == Point(40, 60));
    CHECK(rect.GetSize() == Size(30, 40));
}

TEST(RectNormalizeSwapsInvertedEdges) {
    // Document rectangles have y pointing up, so top > bottom is normal.
    Rect rect(40, 60, 10, 20);
    rect.Normalize();
    CHECK_EQ(rect.left, 10L);
    CHECK_EQ(rect.top, 20L);
    CHECK_EQ(rect.right, 40L);
    CHECK_EQ(rect.bottom, 60L);

    const Rect original(40, 60, 10, 20);
    CHECK(original.Normalized() == rect);
    // Normalized() must not modify the source.
    CHECK_EQ(original.left, 40L);
}

TEST(RectEmptiness) {
    CHECK(Rect(0, 0, 0, 0).IsEmpty());
    CHECK(Rect(10, 10, 5, 20).IsEmpty());
    CHECK(!Rect(0, 0, 1, 1).IsEmpty());
}

TEST(RectIntersectAndUnion) {
    const Rect a(0, 0, 10, 10);
    const Rect b(5, 5, 20, 20);

    CHECK(a.Intersect(b) == Rect(5, 5, 10, 10));
    CHECK(a.Union(b) == Rect(0, 0, 20, 20));
    CHECK(a.Intersect(Rect(50, 50, 60, 60)).IsEmpty());
}

TEST(RectInflateAndOffset) {
    Rect rect(10, 10, 20, 20);
    rect.Inflate(5, 2);
    CHECK(rect == Rect(5, 8, 25, 22));

    rect.Offset(-5, -8);
    CHECK(rect == Rect(0, 0, 20, 14));

    rect += Size(1, 2);
    CHECK(rect == Rect(1, 2, 21, 16));
}

TEST(RectFromPointAndSize) {
    const Rect rect(Point(10, 20), Size(30, -40));
    CHECK(rect == Rect(10, 20, 40, -20));
}

TEST(PointArithmetic) {
    const Point a(10, 20);
    const Point b(3, 4);

    CHECK(a + Size(1, 2) == Point(11, 22));
    CHECK(a - Size(1, 2) == Point(9, 18));
    CHECK(a - b == Size(7, 16));

    Point c = a;
    c += Size(5, 5);
    CHECK(c == Point(15, 25));
}

TEST(DashPatternForSolidIsEmpty) {
    std::array<float, 4> storage{};
    CHECK(DashPattern(DashStyle::Solid, 4.0f, storage).empty());
}

TEST(DashPatternScalesWithTheStroke) {
    std::array<float, 4> storage{};

    CHECK_EQ(DashPattern(DashStyle::Dash, 4.0f, storage).size(), std::size_t(2));
    CHECK_EQ(DashPattern(DashStyle::Dot, 4.0f, storage).size(), std::size_t(2));
    CHECK_EQ(DashPattern(DashStyle::DashDot, 4.0f, storage).size(), std::size_t(4));

    const float thin = DashPattern(DashStyle::Dash, 1.0f, storage)[0];
    std::array<float, 4> other{};
    const float thick = DashPattern(DashStyle::Dash, 8.0f, other)[0];
    CHECK(thick > thin);

    // Hairlines still get a visible dash rather than a zero-length one.
    std::array<float, 4> hairline{};
    CHECK(DashPattern(DashStyle::Dash, 0.1f, hairline)[0] > 1.0f);
}

