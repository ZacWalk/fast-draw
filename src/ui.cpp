#include "ui.h"

#include "libdesktop/core/strings.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>

namespace ui {
namespace {

std::map<std::tuple<int, bool>, render::font_face>& FontCache() {
    static std::map<std::tuple<int, bool>, render::font_face> cache;
    return cache;
}

}  // namespace

int Scale(const double dpiScale, const int value) {
    return static_cast<int>(std::lround(static_cast<double>(value) * dpiScale));
}

float ScaleF(const double dpiScale, const float value) {
    return value * static_cast<float>(dpiScale);
}

render::font_face Font(const double dpiScale, const bool bold) {
    const int dpi = (std::max)(1, static_cast<int>(std::lround(dpiScale * 96.0)));
    const auto key = std::make_tuple(dpi, bold);
    if (const auto found = FontCache().find(key); found != FontCache().end())
        return found->second;

    render::font_desc definition;
    definition.family = "Segoe UI";
    definition.size = (std::max)(1, static_cast<int>(std::lround(9.0 * dpiScale)));
    definition.weight = bold ? 600 : 400;
    const auto handle = render::make_font(definition);
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
          const Color colour, const TextAlign align, const render::font_face font) {
    if (text.empty() || !canvas.IsValid()) return;

    auto draw = render::create_surface_context(canvas.Surface());
    if (!draw) return;
    const std::string utf8 = core::utf16_to_utf8(text);
    const auto style = align == TextAlign::center
                           ? render::text_style::single_line_center
                           : align == TextAlign::right ? render::text_style::single_line_far
                                                       : render::text_style::single_line;
    draw->draw_text(utf8, core::recti{bounds.left, bounds.top, bounds.right, bounds.bottom},
                    font, style, ToCoreColor(colour), {});
    (void)draw->render();
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
