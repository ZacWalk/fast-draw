// Geometry in device pixels: the document-to-device transform and paths of
// flattened contours. Curves become points here, before anything is drawn, so
// the outline that is drawn is the outline that is hit tested.

#pragma once

#include "geometry.h"

#include <span>
#include <vector>

struct Vec2
{
	float x = 0.0f;
	float y = 0.0f;
};

struct RectF
{
	float left = 0.0f;
	float top = 0.0f;
	float right = 0.0f;
	float bottom = 0.0f;

	float Width() const { return right - left; }
	float Height() const { return bottom - top; }
	bool IsEmpty() const { return right <= left || bottom <= top; }
	Vec2 Centre() const { return {(left + right) * 0.5f, (top + bottom) * 0.5f}; }

	RectF Inflated(float amount) const { return {left - amount, top - amount, right + amount, bottom + amount}; }

	static RectF Around(Vec2 a, Vec2 b);
	static RectF FromRect(const Rect& rect);
	Rect ToRect() const;
};

// Maps document coordinates (0.01 inch units, y axis up, origin at the centre
// of the page) to device pixels.
struct Transform
{
	float scale = 1.0f;
	float offsetX = 0.0f;
	float offsetY = 0.0f;

	Vec2 ToDevice(Point point) const
	{
		return {static_cast<float>(point.x) * scale + offsetX, offsetY - static_cast<float>(point.y) * scale};
	}
	Point ToDoc(Vec2 point) const
	{
		return Point(static_cast<long>((point.x - offsetX) / scale), static_cast<long>((offsetY - point.y) / scale));
	}
	float ToDevice(long length) const { return static_cast<float>(length) * scale; }

	RectF ToDevice(const Rect& rect) const
	{
		return RectF::Around(ToDevice(rect.TopLeft()), ToDevice(rect.BottomRight()));
	}
};

// A set of contours of already-flattened points, in device pixels.
class Path
{
public:
	struct Contour
	{
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
