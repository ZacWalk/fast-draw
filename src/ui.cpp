#include "ui.h"

#include "libdesktop/core/strings.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>
#include <utility>

namespace ui
{
	namespace
	{

		std::map<std::tuple<int, bool>, render::font_face>& FontCache()
		{
			static std::map<std::tuple<int, bool>, render::font_face> cache;
			return cache;
		}

		void DrawRoundRect(render::draw_context& dc, const RectF& bounds, const float radius,
		                   const render::polygon_style& style)
		{
			Path path;
			if (radius <= 0.5f) path.AddRect(bounds);
			else path.AddRoundRect(bounds, radius);
			DrawPath(dc, path, style);
		}

		// A segment `width` wide with round ends, as one convex polygon. A path's
		// segments cost a polygon each this way, where draw_polyline samples every
		// segment that reaches a row at each pixel it covers - many times more for a
		// long path of many segments.
		std::vector<core::pointd> Capsule(const Vec2 from, const Vec2 to, const float width)
		{
			constexpr double kPi = 3.14159265358979;
			const double half = (std::max)(0.05, width * 0.5);
			const double angle = std::atan2(to.y - from.y, to.x - from.x);
			// Each end keeps within a tenth of a pixel of its arc.
			const double step = 2.0 * std::acos((std::max)(0.0, 1.0 - 0.1 / half));
			const int steps = (std::clamp)(static_cast<int>(std::ceil(kPi / step)), 4, 64);

			std::vector<core::pointd> points;
			points.reserve(static_cast<std::size_t>(steps + 1) * 2);
			const std::pair<Vec2, double> ends[] = {{to, angle - kPi / 2}, {from, angle + kPi / 2}};
			for (const auto& [centre, start] : ends)
			{
				for (int i = 0; i <= steps; ++i)
				{
					const double turn = start + kPi * i / steps;
					points.emplace_back(centre.x + std::cos(turn) * half, centre.y + std::sin(turn) * half);
				}
			}
			return points;
		}

	} // namespace

	int Scale(const double dpiScale, const int value)
	{
		return static_cast<int>(std::lround(static_cast<double>(value) * dpiScale));
	}

	float ScaleF(const double dpiScale, const float value)
	{
		return value * static_cast<float>(dpiScale);
	}

	render::font_face Font(const double dpiScale, const bool bold)
	{
		const int dpi = (std::max)(1, static_cast<int>(std::lround(dpiScale * 96.0)));
		const auto key = std::make_tuple(dpi, bold);
		if (const auto found = FontCache().find(key); found != FontCache().end()) return found->second;

		render::font_desc definition;
		definition.family = "Segoe UI";
		definition.size = (std::max)(1, static_cast<int>(std::lround(9.0 * dpiScale)));
		definition.weight = bold ? 600 : 400;
		const auto handle = render::make_font(definition);
		FontCache().emplace(key, handle);
		return handle;
	}

	std::vector<core::pointd> Points(const std::span<const Vec2> points)
	{
		std::vector<core::pointd> result;
		result.reserve(points.size());
		for (const Vec2 point : points) result.emplace_back(point.x, point.y);
		return result;
	}

	void DrawPath(render::draw_context& dc, const Path& path, const render::polygon_style& style)
	{
		const bool filled = style.fill.is_visible();
		const bool bordered = style.has_border();

		for (const auto& contour : path.Contours())
		{
			const auto& points = contour.points;
			if (points.size() < 2) continue;
			if (contour.closed)
			{
				if (filled || bordered) dc.draw_polygon(Points(points), style);
			}
			else if (bordered)
			{
				const auto width = static_cast<float>(style.border_width);
				for (std::size_t i = 1; i < points.size(); ++i)
					dc.draw_polygon(Capsule(points[i - 1], points[i], width),
					                {.fill = style.border, .smooth = style.smooth});
			}
		}
	}

	void FillRoundRect(render::draw_context& dc, const RectF& bounds, const float radius, const Color colour,
	                   const std::uint8_t alpha)
	{
		DrawRoundRect(dc, bounds, radius, {.fill = ToCoreColor(colour, alpha)});
	}

	void FrameRoundRect(render::draw_context& dc, const RectF& bounds, const float radius, const Color colour,
	                    const float width)
	{
		DrawRoundRect(dc, bounds, radius, {.border = ToCoreColor(colour), .border_width = width});
	}

	void Line(render::draw_context& dc, const Vec2 from, const Vec2 to, const Color colour, const float width)
	{
		dc.draw_line({from.x, from.y}, {to.x, to.y}, ToCoreColor(colour), width);
	}

	void Text(render::draw_context& dc, const Rect& bounds, const std::wstring_view text, const Color colour,
	          const TextAlign align, const render::font_face font)
	{
		if (text.empty() || bounds.IsEmpty()) return;

		const auto style = align == TextAlign::center  ? render::text_style::single_line_center
		                   : align == TextAlign::right ? render::text_style::single_line_far
		                                               : render::text_style::single_line;
		dc.draw_text(core::utf16_to_utf8(text), core::recti{bounds.left, bounds.top, bounds.right, bounds.bottom}, font,
		             style, ToCoreColor(colour), {});
	}

	void Swatch(render::draw_context& dc, const RectF& bounds, const Color colour, const bool empty, const bool hot,
	            const float radius)
	{
		if (empty)
		{
			FillRoundRect(dc, bounds, radius, kPanel);
			Line(dc, {bounds.left + 2.0f, bounds.bottom - 2.0f}, {bounds.right - 2.0f, bounds.top + 2.0f}, kDanger,
			     1.5f);
		}
		else
		{
			FillRoundRect(dc, bounds, radius, colour);
		}

		FrameRoundRect(dc, bounds.Inflated(-0.5f), radius, hot ? kAccent : kBorderStrong, hot ? 1.8f : 1.0f);
	}

} // namespace ui
