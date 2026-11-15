#include "test_framework.h"

#include "draw_obj.h"
#include "libdesktop/ui/test_support.h"
#include "path.h"
#include "shapes.h"
#include "ui.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace
{

	bool Near(int actual, int expected, int tolerance)
	{
		return std::abs(actual - expected) <= tolerance;
	}

	bool Near(core::pointd actual, core::pointd expected)
	{
		return std::abs(actual.X - expected.X) < 0.01 && std::abs(actual.Y - expected.Y) < 0.01;
	}

	RectF Extent(const std::vector<core::pointd>& points)
	{
		RectF extent{1e9f, 1e9f, -1e9f, -1e9f};
		for (const core::pointd point : points)
		{
			extent.left = (std::min)(extent.left, static_cast<float>(point.X));
			extent.top = (std::min)(extent.top, static_cast<float>(point.Y));
			extent.right = (std::max)(extent.right, static_cast<float>(point.X));
			extent.bottom = (std::max)(extent.bottom, static_cast<float>(point.Y));
		}
		return extent;
	}

	// Document units map onto pixels one for one, with the document origin at (200, 100).
	Transform TestTransform()
	{
		return Transform{1.0f, 200.0f, 100.0f};
	}

	struct Drawing
	{
		std::vector<ui::test::fake_draw_context::polygon_draw> polygons;
		std::vector<ui::test::fake_draw_context::polyline_draw> polylines;
	};

	Drawing Drawn(const DrawObj& object)
	{
		ui::test::fake_draw_context dc;
		object.Draw(dc, TestTransform());
		return {dc.polygons, dc.polylines};
	}

	// A 200 x 100 rectangle that lands on device pixels (100, 50) to (300, 150).
	DrawObj Box(const Style& style)
	{
		return DrawObj(ShapeKind::Rectangle, Rect(-100, 50, 100, -50), style);
	}

} // namespace

TEST(TransformMapsDocumentSpaceWithYUp)
{
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

TEST(TransformNormalizesAnInvertedRect)
{
	Transform transform;
	// Document rectangles have top > bottom; the device rect must come out
	// the right way up.
	const RectF device = transform.ToDevice(Rect(-10, 20, 30, -40));
	CHECK(device.left < device.right);
	CHECK(device.top < device.bottom);
}

TEST(PathContainmentUsesTheOutline)
{
	Path rect;
	rect.AddRect({10.0f, 10.0f, 50.0f, 30.0f});
	CHECK(PathContains(rect, {30.0f, 20.0f}));
	CHECK(!PathContains(rect, {5.0f, 20.0f}));

	Path ellipse;
	ellipse.AddEllipse({0.0f, 0.0f, 100.0f, 100.0f});
	CHECK(PathContains(ellipse, {50.0f, 50.0f}));
	CHECK(!PathContains(ellipse, {4.0f, 4.0f})); // corner is outside the circle
}

TEST(OpenContoursAreNotTreatedAsAreas)
{
	const Vec2 points[3] = {{0.0f, 0.0f}, {50.0f, 0.0f}, {50.0f, 50.0f}};
	Path line;
	line.AddPolyline(std::span<const Vec2>(points, 3));

	CHECK(!PathContains(line, {40.0f, 10.0f}));
	CHECK(PathNearEdge(line, {25.0f, 1.0f}, 2.0f));
	CHECK(!PathNearEdge(line, {25.0f, 20.0f}, 2.0f));
}

TEST(AFilledShapeIsOnePolygonWithItsBorder)
{
	const Style style;
	const auto dc = Drawn(Box(style));

	CHECK_EQ(dc.polygons.size(), std::size_t(1));
	CHECK(dc.polylines.empty());
	if (dc.polygons.size() != 1) return;

	const auto& polygon = dc.polygons.front();
	CHECK_EQ(polygon.points.size(), std::size_t(4));
	CHECK(Near(polygon.points[0], {100.0, 50.0}));
	CHECK(Near(polygon.points[2], {300.0, 150.0}));
	CHECK(polygon.style.fill.kind == render::paint_kind::solid);
	CHECK(polygon.style.fill.color == ui::ToCoreColor(style.fill));
	CHECK(polygon.style.border == ui::ToCoreColor(style.line));
	CHECK_EQ(polygon.style.border_width, 2.0);
	CHECK(polygon.style.smooth);
}

TEST(AGradientFillReachesThePolygonAsALinearPaint)
{
	Style style;
	style.gradient = true;
	style.fill = Rgb(0xFF, 0x00, 0x00);
	style.fillEnd = Rgb(0x00, 0x00, 0xFF);

	const auto down = Drawn(Box(style));
	CHECK_EQ(down.polygons.size(), std::size_t(1));
	if (!down.polygons.empty())
	{
		const render::paint& paint = down.polygons.front().style.fill;
		CHECK(paint.kind == render::paint_kind::linear);
		CHECK(paint.color == ui::ToCoreColor(style.fill));
		CHECK(paint.end_color == ui::ToCoreColor(style.fillEnd));
		CHECK(Near(paint.start, {100.0, 50.0}));
		CHECK(Near(paint.end, {100.0, 150.0}));
	}

	style.gradientDirection = GradientDirection::Right;
	const auto right = Drawn(Box(style));
	if (!right.polygons.empty())
	{
		const render::paint& paint = right.polygons.front().style.fill;
		CHECK(Near(paint.start, {100.0, 50.0}));
		CHECK(Near(paint.end, {300.0, 50.0}));
	}
}

TEST(AShapeWithoutFillOrLineLeavesThemOut)
{
	Style outline;
	outline.hasFill = false;
	const auto unfilled = Drawn(Box(outline));
	CHECK_EQ(unfilled.polygons.size(), std::size_t(1));
	if (!unfilled.polygons.empty())
	{
		CHECK(!unfilled.polygons.front().style.fill.is_visible());
		CHECK(unfilled.polygons.front().style.has_border());
	}

	Style flat;
	flat.hasLine = false;
	const auto unstroked = Drawn(Box(flat));
	CHECK_EQ(unstroked.polygons.size(), std::size_t(1));
	if (!unstroked.polygons.empty())
	{
		CHECK(unstroked.polygons.front().style.fill.is_visible());
		CHECK(!unstroked.polygons.front().style.has_border());
	}

	Style nothing;
	nothing.hasFill = false;
	nothing.hasLine = false;
	CHECK(Drawn(Box(nothing)).polygons.empty());
}

TEST(AnArrowIsARoundEndedLineWithAFilledHead)
{
	Style style;
	style.hasFill = false;
	style.endArrow = true;
	const DrawObj arrow(ShapeKind::Arrow, Rect(-100, 0, 100, 0), style);

	// The line runs from (100, 100) to (300, 100), two pixels wide, and its
	// head is drawn over its end.
	const auto dc = Drawn(arrow);
	CHECK(dc.polylines.empty());
	CHECK_EQ(dc.polygons.size(), std::size_t(2));
	if (dc.polygons.size() != 2) return;

	const auto& line = dc.polygons.front();
	const RectF reach = Extent(line.points);
	CHECK(Near(static_cast<int>(std::lround(reach.left)), 99, 0));
	CHECK(Near(static_cast<int>(std::lround(reach.right)), 301, 0));
	CHECK(Near(static_cast<int>(std::lround(reach.top)), 99, 0));
	CHECK(Near(static_cast<int>(std::lround(reach.bottom)), 101, 0));
	CHECK(line.style.fill.color == ui::ToCoreColor(style.line));
	CHECK(!line.style.has_border());

	const auto& head = dc.polygons.back();
	CHECK_EQ(head.points.size(), std::size_t(3));
	CHECK(Near(head.points.front(), {300.0, 100.0}));
	CHECK(head.style.fill.color == ui::ToCoreColor(style.line));
	CHECK(!head.style.has_border());

	style.startArrow = true;
	CHECK_EQ(Drawn(DrawObj(ShapeKind::Arrow, Rect(-100, 0, 100, 0), style)).polygons.size(), std::size_t(3));
}

TEST(CurvesAndCompoundShapesAreDrawnContourByContour)
{
	const Style style;

	// The ellipse is flattened in the app, so every point lies on it.
	const auto ellipse = Drawn(DrawObj(ShapeKind::Ellipse, Rect(-100, 50, 100, -50), style));
	CHECK_EQ(ellipse.polygons.size(), std::size_t(1));
	if (!ellipse.polygons.empty())
	{
		const auto& points = ellipse.polygons.front().points;
		CHECK(points.size() > 16);
		bool onCurve = true;
		for (const core::pointd point : points)
		{
			const double x = (point.X - 200.0) / 100.0;
			const double y = (point.Y - 100.0) / 50.0;
			if (std::abs(x * x + y * y - 1.0) > 0.01) onCurve = false;
		}
		CHECK(onCurve);
	}

	// A cylinder's body and its top; an actor's head and its four strokes.
	CHECK_EQ(Drawn(DrawObj(ShapeKind::Cylinder, Rect(-100, 50, 100, -50), style)).polygons.size(), std::size_t(2));
	const auto actor = Drawn(DrawObj(ShapeKind::Actor, Rect(-100, 50, 100, -50), style));
	CHECK_EQ(actor.polygons.size(), std::size_t(5));
	if (actor.polygons.size() == 5)
	{
		CHECK(actor.polygons.front().style.fill.color == ui::ToCoreColor(style.fill));
		for (std::size_t i = 1; i < actor.polygons.size(); ++i)
			CHECK(actor.polygons[i].style.fill.color == ui::ToCoreColor(style.line));
	}
}

TEST(ALongLabelIsTrimmedToItsShape)
{
	DrawObj box = Box(Style{});
	box.text = L"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";

	// The fake measures eight pixels a character; the label box is 188 wide.
	ui::test::fake_draw_context fitted;
	box.DrawLabel(fitted, TestTransform());
	CHECK_EQ(fitted.texts.size(), std::size_t(1));
	if (!fitted.texts.empty())
	{
		const std::string& text = fitted.texts.front().text;
		CHECK(text.ends_with("\xE2\x80\xA6"));
		CHECK(ui::test::grid_metrics::codepoint_count(text) <= 188 / 8);
		CHECK(fitted.texts.front().color == ui::ToCoreColor(Style{}.textColour));
	}

	ui::test::fake_draw_context typing;
	box.DrawLabel(typing, TestTransform(), false);
	if (!typing.texts.empty()) CHECK_EQ(typing.texts.front().text.size(), box.text.size());
}

TEST(EveryShapeProducesGeometry)
{
	for (int i = 0; i < kShapeKindCount; ++i)
	{
		Path path;
		BuildShapePath(static_cast<ShapeKind>(i), {0.0f, 0.0f, 100.0f, 80.0f}, path);
		CHECK(!path.Empty());

		const RectF bounds = path.Bounds();
		CHECK(bounds.Width() > 0.0f);
		CHECK(bounds.Height() > 0.0f);
	}
}

TEST(ArrowHeadPointsAtTheTip)
{
	Path head;
	BuildArrowHead({100.0f, 50.0f}, {0.0f, 50.0f}, 10.0f, head);
	CHECK(!head.Empty());

	const RectF bounds = head.Bounds();
	CHECK(Near(static_cast<int>(bounds.right), 100, 1));
	CHECK(Near(static_cast<int>(bounds.left), 90, 1));
}
