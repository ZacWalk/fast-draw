#include "style.h"

#include "canvas.h"

#include <algorithm>
#include <cstdio>

namespace {

constexpr std::string_view kDashNames[] = {"solid", "dash", "dot", "dashDot"};
static_assert(std::size(kDashNames) == kDashStyleCount);

constexpr std::string_view kGradientNames[] = {"down", "right", "downRight", "upRight"};
static_assert(std::size(kGradientNames) == kGradientDirectionCount);

template <typename Enum, std::size_t N>
std::string_view EnumName(Enum value, const std::string_view (&names)[N]) {
    const auto index = static_cast<std::size_t>(value);
    return index < N ? names[index] : names[0];
}

template <typename Enum, std::size_t N>
Enum EnumValue(const Json& json, const std::string_view (&names)[N], Enum fallback) {
    const std::string text = json.AsString();
    for (std::size_t i = 0; i < N; ++i) {
        if (names[i] == text) return static_cast<Enum>(i);
    }
    return fallback;
}

}  // namespace

std::string ColourToHex(COLORREF colour) {
    char buffer[8];
    std::snprintf(buffer, sizeof(buffer), "#%02x%02x%02x", GetRValue(colour), GetGValue(colour),
                  GetBValue(colour));
    return buffer;
}

COLORREF ColourFromHex(std::string_view text, COLORREF fallback) {
    if (text.size() != 7 || text.front() != '#') return fallback;

    unsigned components[3]{};
    for (int i = 0; i < 3; ++i) {
        for (int digit = 0; digit < 2; ++digit) {
            const char c = text[static_cast<std::size_t>(1 + i * 2 + digit)];
            components[i] <<= 4;
            if (c >= '0' && c <= '9')
                components[i] |= static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f')
                components[i] |= static_cast<unsigned>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F')
                components[i] |= static_cast<unsigned>(c - 'A' + 10);
            else
                return fallback;
        }
    }

    return RGB(components[0], components[1], components[2]);
}

Json StyleToJson(const Style& style) {
    Json fill = Json::MakeObject();
    fill.Set("visible", Json(style.hasFill));
    fill.Set("colour", Json(ColourToHex(style.fill)));
    fill.Set("gradient", Json(style.gradient));
    fill.Set("endColour", Json(ColourToHex(style.fillEnd)));
    fill.Set("direction", Json(EnumName(style.gradientDirection, kGradientNames)));

    Json line = Json::MakeObject();
    line.Set("visible", Json(style.hasLine));
    line.Set("colour", Json(ColourToHex(style.line)));
    line.Set("width", Json(style.lineWidth));
    line.Set("dash", Json(EnumName(style.dash, kDashNames)));
    line.Set("roundJoins", Json(style.roundJoins));
    line.Set("startArrow", Json(style.startArrow));
    line.Set("endArrow", Json(style.endArrow));

    Json text = Json::MakeObject();
    text.Set("colour", Json(ColourToHex(style.textColour)));
    text.Set("size", Json(style.fontSize));
    text.Set("bold", Json(style.bold));
    text.Set("italic", Json(style.italic));

    Json json = Json::MakeObject();
    json.Set("fill", std::move(fill));
    json.Set("line", std::move(line));
    json.Set("text", std::move(text));
    return json;
}

Style StyleFromJson(const Json& json, const Style& fallback) {
    Style style = fallback;

    const Json& fill = json["fill"];
    style.hasFill = fill["visible"].AsBool(style.hasFill);
    style.fill = ColourFromHex(fill["colour"].AsString(), style.fill);
    style.gradient = fill["gradient"].AsBool(style.gradient);
    style.fillEnd = ColourFromHex(fill["endColour"].AsString(), style.fillEnd);
    style.gradientDirection =
        EnumValue(fill["direction"], kGradientNames, style.gradientDirection);

    const Json& line = json["line"];
    style.hasLine = line["visible"].AsBool(style.hasLine);
    style.line = ColourFromHex(line["colour"].AsString(), style.line);
    style.lineWidth = (std::clamp)(static_cast<int>(line["width"].AsInt(style.lineWidth)), 0, 48);
    style.dash = EnumValue(line["dash"], kDashNames, style.dash);
    style.roundJoins = line["roundJoins"].AsBool(style.roundJoins);
    style.startArrow = line["startArrow"].AsBool(style.startArrow);
    style.endArrow = line["endArrow"].AsBool(style.endArrow);

    const Json& text = json["text"];
    style.textColour = ColourFromHex(text["colour"].AsString(), style.textColour);
    style.fontSize = (std::clamp)(static_cast<int>(text["size"].AsInt(style.fontSize)), 4, 200);
    style.bold = text["bold"].AsBool(style.bold);
    style.italic = text["italic"].AsBool(style.italic);

    return style;
}

std::span<const float> DashPattern(DashStyle style, float strokeWidth,
                                   std::array<float, 4>& storage) {
    const float unit = (std::max)(strokeWidth, 1.0f);

    switch (style) {
        case DashStyle::Dash:
            storage = {unit * 3.2f, unit * 2.4f, 0.0f, 0.0f};
            return {storage.data(), 2};
        case DashStyle::Dot:
            storage = {unit * 0.9f, unit * 2.0f, 0.0f, 0.0f};
            return {storage.data(), 2};
        case DashStyle::DashDot:
            storage = {unit * 3.2f, unit * 2.0f, unit * 0.9f, unit * 2.0f};
            return {storage.data(), 4};
        case DashStyle::Solid: break;
    }
    return {};
}

Paint FillPaint(const Style& style, const RectF& bounds) {
    Paint paint;
    paint.color = ArgbFromColorRef(style.fill);
    paint.endColor = ArgbFromColorRef(style.fillEnd);
    paint.gradient = style.gradient;
    paint.direction = style.gradientDirection;
    paint.gradientBounds = bounds;
    return paint;
}

Paint LinePaint(const Style& style) { return SolidPaint(ArgbFromColorRef(style.line)); }

void ApplyStyleDelta(const Style& before, const Style& after, Style& target) {
    const auto take = [](const auto& from, const auto& to, auto& field) {
        if (from != to) field = to;
    };

    take(before.hasFill, after.hasFill, target.hasFill);
    take(before.fill, after.fill, target.fill);
    take(before.gradient, after.gradient, target.gradient);
    take(before.fillEnd, after.fillEnd, target.fillEnd);
    take(before.gradientDirection, after.gradientDirection, target.gradientDirection);

    take(before.hasLine, after.hasLine, target.hasLine);
    take(before.line, after.line, target.line);
    take(before.lineWidth, after.lineWidth, target.lineWidth);
    take(before.dash, after.dash, target.dash);
    take(before.roundJoins, after.roundJoins, target.roundJoins);
    take(before.startArrow, after.startArrow, target.startArrow);
    take(before.endArrow, after.endArrow, target.endArrow);

    take(before.textColour, after.textColour, target.textColour);
    take(before.fontSize, after.fontSize, target.fontSize);
    take(before.bold, after.bold, target.bold);
    take(before.italic, after.italic, target.italic);
}
