// Every visual property of an object lives here, so the sidebar can edit one
// struct and objects, previews and defaults all share it.

#pragma once

#include "geometry.h"
#include "json.h"

#include <string>
#include <string_view>

// Which way a gradient fill runs from its first colour to its second.
enum class GradientDirection : int
{
	Down,
	Right,
};

inline constexpr int kGradientDirectionCount = 2;

struct Style
{
	bool hasFill = true;
	Color fill = Rgb(0xDC, 0xE9, 0xFB);
	bool gradient = false;
	Color fillEnd = Rgb(0x8F, 0xB8, 0xE8);
	GradientDirection gradientDirection = GradientDirection::Down;

	bool hasLine = true;
	Color line = Rgb(0x1F, 0x4B, 0x7A);
	int lineWidth = 2; // 0.01 inch units, same as document coordinates

	bool startArrow = false;
	bool endArrow = false;

	Color textColour = Rgb(0x1B, 0x1B, 0x1F);
	int fontSize = 11; // points

	friend bool operator==(const Style&, const Style&) = default;
};

// Colours are stored in documents as "#rrggbb".
std::string ColourToHex(Color colour);
Color ColourFromHex(std::string_view text, Color fallback);

// Reading ignores keys this version does not know - older documents also hold
// dash styles, round joins, bold and italic - and a gradient direction other
// than down or right reads as the fallback's.
Json StyleToJson(const Style& style);
Style StyleFromJson(const Json& json, const Style& fallback = {});

// Copies across only the fields that differ between `before` and `after`, so
// changing one sidebar control leaves the rest of a mixed selection alone.
void ApplyStyleDelta(const Style& before, const Style& after, Style& target);
