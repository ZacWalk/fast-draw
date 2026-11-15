// Shared drawing for fast-draw's chrome and documents: colours, the UI font,
// and paths, shapes and text drawn through libdesktop's draw_context. Window
// creation, input and presentation belong to libdesktop's ui and platform layers.

#pragma once

#include "geometry.h"
#include "libdesktop/render.h"
#include "path.h"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace ui
{

	inline constexpr Color kChrome = Rgb(0xF6, 0xF6, 0xF8);
	inline constexpr Color kChromeDeep = Rgb(0xE3, 0xE4, 0xE9);
	inline constexpr Color kPanel = Rgb(0xFF, 0xFF, 0xFF);
	inline constexpr Color kHover = Rgb(0xE8, 0xEF, 0xF9);
	inline constexpr Color kPressed = Rgb(0xD2, 0xE1, 0xF4);
	inline constexpr Color kBorder = Rgb(0xD2, 0xD3, 0xDA);
	inline constexpr Color kBorderStrong = Rgb(0xA6, 0xA7, 0xB2);
	inline constexpr Color kText = Rgb(0x1B, 0x1B, 0x1F);
	inline constexpr Color kTextDim = Rgb(0x5F, 0x60, 0x6C);
	inline constexpr Color kAccent = Rgb(0x0F, 0x6C, 0xBD);
	inline constexpr Color kAccentSoft = Rgb(0xD8, 0xE7, 0xF8);
	inline constexpr Color kDanger = Rgb(0xC4, 0x2B, 0x1C);
	inline constexpr Color kWorkspace = Rgb(0x7A, 0x7C, 0x86);

	int Scale(double dpiScale, int value);
	float ScaleF(double dpiScale, float value);

	// A process-wide descriptor cache keeps the controls and labels on one font face.
	render::font_face Font(double dpiScale, bool bold = false);

	inline core::color ToCoreColor(const Color colour, const std::uint8_t alpha = 255)
	{
		return core::color(core::rgba((colour >> 16) & 0xffu, (colour >> 8) & 0xffu, colour & 0xffu, alpha));
	}

	std::vector<core::pointd> Points(std::span<const Vec2> points);

	// Each closed contour of `path` as a polygon drawn in `style`, and each open one
	// as a line in the style's border colour and width.
	void DrawPath(render::draw_context& dc, const Path& path, const render::polygon_style& style);

	void FillRoundRect(render::draw_context& dc, const RectF& bounds, float radius, Color colour,
	                   std::uint8_t alpha = 255);
	void FrameRoundRect(render::draw_context& dc, const RectF& bounds, float radius, Color colour, float width);
	void Line(render::draw_context& dc, Vec2 from, Vec2 to, Color colour, float width);

	enum class TextAlign
	{
		left,
		center,
		right
	};

	void Text(render::draw_context& dc, const Rect& bounds, std::wstring_view text, Color colour, TextAlign align,
	          render::font_face font);

	// A checkerboard-free colour swatch with a border, used all over the sidebar.
	void Swatch(render::draw_context& dc, const RectF& bounds, Color colour, bool empty, bool hot, float radius);

} // namespace ui
