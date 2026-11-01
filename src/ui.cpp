#include "ui.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>

namespace ui {
namespace {

std::map<std::tuple<int, bool>, pf::font_handle>& FontCache() {
    static std::map<std::tuple<int, bool>, pf::font_handle> cache;
    return cache;
}

pf::color_t ToPfColor(const Color color) {
    return {static_cast<std::uint8_t>(color >> 16 & 0xffu),
            static_cast<std::uint8_t>(color >> 8 & 0xffu),
            static_cast<std::uint8_t>(color & 0xffu)};
}

}  // namespace

int Scale(const double dpiScale, const int value) {
    return static_cast<int>(std::lround(static_cast<double>(value) * dpiScale));
}

float ScaleF(const double dpiScale, const float value) {
    return value * static_cast<float>(dpiScale);
}

pf::font_handle Font(const double dpiScale, const bool bold) {
    const int dpi = (std::max)(1, static_cast<int>(std::lround(dpiScale * 96.0)));
    const auto key = std::make_tuple(dpi, bold);
    if (const auto found = FontCache().find(key); found != FontCache().end())
        return found->second;

    pf::font_def definition;
    definition.face = "Segoe UI";
    definition.size = 9;
    definition.weight = bold ? 600 : 400;
    const auto handle = pf::create_font_handle(definition);
    FontCache().emplace(key, handle);
    return handle;
}

void FillRoundRect(Renderer& renderer, const RectF& bounds, const float radius,
                   const Color colour, const std::uint8_t alpha) {
    Path path;
    if (radius <= 0.5f)
        path.AddRect(bounds);
    else
        path.AddRoundRect(bounds, radius);
    renderer.Fill(path, SolidPaint(Argb(colour, alpha)));
}

void FrameRoundRect(Renderer& renderer, const RectF& bounds, const float radius,
                    const Color colour, const float width) {
    Path path;
    if (radius <= 0.5f)
        path.AddRect(bounds);
    else
        path.AddRoundRect(bounds, radius);
    renderer.Stroke(path, SolidPaint(Argb(colour)), StrokeStyle{width, true});
}

void Line(Renderer& renderer, const Vec2 from, const Vec2 to, const Color colour,
          const float width) {
    Path path;
    const Vec2 points[2] = {from, to};
    path.AddPolyline(std::span<const Vec2>(points, 2));
    renderer.Stroke(path, SolidPaint(Argb(colour)), StrokeStyle{width, true});
}

void Text(Canvas& canvas, const Rect& bounds, const std::wstring_view text,
          const Color colour, const TextAlign align, const pf::font_handle font) {
    if (text.empty() || !font) return;

    auto draw = pf::create_pixel_surface_context(canvas.Surface());
    if (!draw) return;
    const std::string utf8 = pf::utf16_to_utf8(text);
    const pf::isize measured = draw->measure_text_h(utf8, font);
    int x = bounds.left;
    if (align == TextAlign::center)
        x += (bounds.Width() - measured.cx) / 2;
    else if (align == TextAlign::right)
        x = bounds.right - measured.cx;

    const int y = bounds.top + (bounds.Height() - measured.cy) / 2;
    draw->set_clip_rect({bounds.left, bounds.top, bounds.right, bounds.bottom});
    draw->draw_text_h(x, y, utf8, font, ToPfColor(colour));
    draw->clear_clip_rect();
}

void Swatch(Renderer& renderer, const RectF& bounds, const Color colour, const bool empty,
            const bool hot, const float radius) {
    if (empty) {
        FillRoundRect(renderer, bounds, radius, kPanel);
        Line(renderer, {bounds.left + 2.0f, bounds.bottom - 2.0f},
             {bounds.right - 2.0f, bounds.top + 2.0f}, kDanger, 1.5f);
    } else {
        FillRoundRect(renderer, bounds, radius, colour);
    }

    FrameRoundRect(renderer, bounds.Inflated(-0.5f), radius, hot ? kAccent : kBorderStrong,
                   hot ? 1.8f : 1.0f);
}

}  // namespace ui
