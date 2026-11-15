#include "path.h"

#include <algorithm>
#include <cmath>

namespace
{

	Vec2 operator+(Vec2 a, Vec2 b)
	{
		return {a.x + b.x, a.y + b.y};
	}
	Vec2 operator-(Vec2 a, Vec2 b)
	{
		return {a.x - b.x, a.y - b.y};
	}
	Vec2 operator*(Vec2 a, float f)
	{
		return {a.x * f, a.y * f};
	}

	float Length(Vec2 v)
	{
		return std::sqrt(v.x * v.x + v.y * v.y);
	}

	// Distance from `p` to the segment ab.
	float DistanceToSegment(Vec2 p, Vec2 a, Vec2 b)
	{
		const Vec2 ab = b - a;
		const float lengthSquared = ab.x * ab.x + ab.y * ab.y;
		if (lengthSquared < 1e-9f) return Length(p - a);

		float t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lengthSquared;
		t = (std::clamp)(t, 0.0f, 1.0f);
		return Length(p - (a + ab * t));
	}

} // namespace

RectF RectF::Around(Vec2 a, Vec2 b)
{
	return {(std::min)(a.x, b.x), (std::min)(a.y, b.y), (std::max)(a.x, b.x), (std::max)(a.y, b.y)};
}

RectF RectF::FromRect(const Rect& rect)
{
	return {static_cast<float>(rect.left), static_cast<float>(rect.top), static_cast<float>(rect.right),
	        static_cast<float>(rect.bottom)};
}

Rect RectF::ToRect() const
{
	return Rect{static_cast<std::int32_t>(std::floor(left)), static_cast<std::int32_t>(std::floor(top)),
	            static_cast<std::int32_t>(std::ceil(right)), static_cast<std::int32_t>(std::ceil(bottom))};
}

// ------------------------------------------------------------------- Path

Path::Contour& Path::Current()
{
	if (contours_.empty() || contours_.back().closed) contours_.emplace_back();
	return contours_.back();
}

void Path::MoveTo(Vec2 point)
{
	contours_.emplace_back();
	contours_.back().points.push_back(point);
}

void Path::LineTo(Vec2 point)
{
	Contour& contour = Current();
	if (!contour.points.empty())
	{
		const Vec2 last = contour.points.back();
		if (std::fabs(last.x - point.x) < 1e-5f && std::fabs(last.y - point.y) < 1e-5f) return;
	}
	contour.points.push_back(point);
}

void Path::CubicTo(Vec2 control1, Vec2 control2, Vec2 end)
{
	Contour& contour = Current();
	if (contour.points.empty()) contour.points.push_back(control1);

	const Vec2 start = contour.points.back();
	// Segment count from the control polygon length; plenty for screen work.
	const float rough = Length(control1 - start) + Length(control2 - control1) + Length(end - control2);
	const int steps = (std::clamp)(static_cast<int>(rough / 3.0f), 4, 48);

	for (int i = 1; i <= steps; ++i)
	{
		const float t = static_cast<float>(i) / steps;
		const float u = 1.0f - t;
		const Vec2 point = start * (u * u * u) + control1 * (3 * u * u * t) + control2 * (3 * u * t * t) +
		                   end * (t * t * t);
		contour.points.push_back(point);
	}
}

void Path::Close()
{
	if (contours_.empty()) return;
	contours_.back().closed = true;
}

void Path::AddRect(const RectF& rect)
{
	MoveTo({rect.left, rect.top});
	LineTo({rect.right, rect.top});
	LineTo({rect.right, rect.bottom});
	LineTo({rect.left, rect.bottom});
	Close();
}

void Path::AddRoundRect(const RectF& rect, float radius)
{
	radius = (std::min)(radius, (std::min)(rect.Width(), rect.Height()) * 0.5f);
	if (radius <= 0.5f)
	{
		AddRect(rect);
		return;
	}

	// Control point offset for a quarter circle.
	const float k = radius * 0.5523f;

	MoveTo({rect.left + radius, rect.top});
	LineTo({rect.right - radius, rect.top});
	CubicTo({rect.right - radius + k, rect.top}, {rect.right, rect.top + radius - k}, {rect.right, rect.top + radius});
	LineTo({rect.right, rect.bottom - radius});
	CubicTo({rect.right, rect.bottom - radius + k}, {rect.right - radius + k, rect.bottom},
	        {rect.right - radius, rect.bottom});
	LineTo({rect.left + radius, rect.bottom});
	CubicTo({rect.left + radius - k, rect.bottom}, {rect.left, rect.bottom - radius + k},
	        {rect.left, rect.bottom - radius});
	LineTo({rect.left, rect.top + radius});
	CubicTo({rect.left, rect.top + radius - k}, {rect.left + radius - k, rect.top}, {rect.left + radius, rect.top});
	Close();
}

void Path::AddEllipse(const RectF& rect)
{
	const Vec2 centre = rect.Centre();
	const float rx = rect.Width() * 0.5f;
	const float ry = rect.Height() * 0.5f;
	const float kx = rx * 0.5523f;
	const float ky = ry * 0.5523f;

	MoveTo({centre.x, centre.y - ry});
	CubicTo({centre.x + kx, centre.y - ry}, {centre.x + rx, centre.y - ky}, {centre.x + rx, centre.y});
	CubicTo({centre.x + rx, centre.y + ky}, {centre.x + kx, centre.y + ry}, {centre.x, centre.y + ry});
	CubicTo({centre.x - kx, centre.y + ry}, {centre.x - rx, centre.y + ky}, {centre.x - rx, centre.y});
	CubicTo({centre.x - rx, centre.y - ky}, {centre.x - kx, centre.y - ry}, {centre.x, centre.y - ry});
	Close();
}

void Path::AddPolygon(std::span<const Vec2> points)
{
	if (points.empty()) return;
	MoveTo(points.front());
	for (std::size_t i = 1; i < points.size(); ++i) LineTo(points[i]);
	Close();
}

void Path::AddPolyline(std::span<const Vec2> points)
{
	if (points.empty()) return;
	MoveTo(points.front());
	for (std::size_t i = 1; i < points.size(); ++i) LineTo(points[i]);
}

bool Path::Empty() const
{
	for (const auto& contour : contours_)
	{
		if (contour.points.size() >= 2) return false;
	}
	return true;
}

RectF Path::Bounds() const
{
	RectF bounds{1e30f, 1e30f, -1e30f, -1e30f};
	bool any = false;
	for (const auto& contour : contours_)
	{
		for (const Vec2& point : contour.points)
		{
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

bool PathContains(const Path& path, Vec2 point)
{
	int winding = 0;
	for (const auto& contour : path.Contours())
	{
		const auto& points = contour.points;
		if (!contour.closed || points.size() < 3) continue;

		for (std::size_t i = 0; i < points.size(); ++i)
		{
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

bool PathNearEdge(const Path& path, Vec2 point, float tolerance)
{
	for (const auto& contour : path.Contours())
	{
		const auto& points = contour.points;
		if (points.size() < 2) continue;

		const std::size_t count = contour.closed ? points.size() : points.size() - 1;
		for (std::size_t i = 0; i < count; ++i)
		{
			const Vec2 a = points[i];
			const Vec2 b = points[(i + 1) % points.size()];
			if (DistanceToSegment(point, a, b) <= tolerance) return true;
		}
	}
	return false;
}
