// Shared chrome: DPI helpers and the explicit software-raster backend used by
// fast-draw's document and controls.  Window creation, input and presentation
// belong to libdesktop's pf::window_frame; this file only owns pixels and
// drawing primitives.

#pragma once

#include "canvas.h"
#include "platform.h"
#include "render.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ui {

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

// Cached shared-platform font handle; owned by the process.
pf::font_handle Font(double dpiScale, bool bold = false);

inline std::uint32_t Argb(const Color colour, const std::uint8_t alpha = 255) {
    return ArgbFromColor(colour, alpha);
}

void FillRoundRect(Renderer& renderer, const RectF& bounds, float radius, Color colour,
                   std::uint8_t alpha = 255);
void FrameRoundRect(Renderer& renderer, const RectF& bounds, float radius, Color colour,
                    float width);
void Line(Renderer& renderer, Vec2 from, Vec2 to, Color colour, float width);

enum class TextAlign { left, center, right };

// Shared platform text primitive; no app-owned native text/font cache.
void Text(Canvas& canvas, const Rect& bounds, std::wstring_view text, Color colour,
          TextAlign align, pf::font_handle font);

// A checkerboard-free colour swatch with a border, used all over the sidebar.
void Swatch(Renderer& renderer, const RectF& bounds, Color colour, bool empty, bool hot,
            float radius);

}  // namespace ui
