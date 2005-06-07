// Every visual property of an object lives here, so the sidebar can edit one
// struct and objects, previews and defaults all share it.

#pragma once

#include "json.h"
#include "render.h"

#include <windows.h>

#include <array>
#include <span>
#include <string>
#include <string_view>

enum class DashStyle : int {
    Solid,
    Dash,
    Dot,
    DashDot,
};

inline constexpr int kDashStyleCount = 4;

struct Style {
    bool hasFill = true;
    COLORREF fill = RGB(0xDC, 0xE9, 0xFB);
    bool gradient = false;
    COLORREF fillEnd = RGB(0x8F, 0xB8, 0xE8);
    GradientDirection gradientDirection = GradientDirection::Down;

    bool hasLine = true;
    COLORREF line = RGB(0x1F, 0x4B, 0x7A);
    int lineWidth = 2;  // 0.01 inch units, same as document coordinates
    DashStyle dash = DashStyle::Solid;
    bool roundJoins = true;

    bool startArrow = false;
    bool endArrow = false;

    COLORREF textColour = RGB(0x1B, 0x1B, 0x1F);
    int fontSize = 11;  // points
    bool bold = false;
    bool italic = false;

    friend bool operator==(const Style&, const Style&) = default;
};

// Colours are stored in documents as "#rrggbb".
std::string ColourToHex(COLORREF colour);
COLORREF ColourFromHex(std::string_view text, COLORREF fallback);

Json StyleToJson(const Style& style);
Style StyleFromJson(const Json& json, const Style& fallback = {});

// Copies across only the fields that differ between `before` and `after`, so
// changing one sidebar control leaves the rest of a mixed selection alone.
void ApplyStyleDelta(const Style& before, const Style& after, Style& target);

// Fills `storage` with the on/off run lengths for `style` at the given device
// stroke width and returns the span to hand to StrokeStyle. Solid yields empty.
std::span<const float> DashPattern(DashStyle style, float strokeWidth,
                                   std::array<float, 4>& storage);

Paint FillPaint(const Style& style, const RectF& bounds);
Paint LinePaint(const Style& style);
