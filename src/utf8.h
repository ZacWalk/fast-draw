#pragma once

#include <string>
#include <string_view>

// The document model uses UTF-16 (std::wstring); the JSON and SVG files are
// UTF-8, so conversions are needed at those boundaries.
std::string Utf8Encode(std::wstring_view text);
std::wstring Utf8Decode(std::string_view text);
