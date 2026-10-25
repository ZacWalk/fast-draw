#pragma once

#include <cstddef>
#include <string>
#include <string_view>

// The document model uses UTF-16 (std::wstring); the JSON and SVG files are
// UTF-8, so conversions are needed at those boundaries.
std::string Utf8Encode(std::wstring_view text);
std::wstring Utf8Decode(std::string_view text);

// Caret positions in UTF-16 text step over whole code points, so editing never
// splits a surrogate pair (an emoji, say) into two unpaired halves.
std::size_t PreviousCaretStop(std::wstring_view text, std::size_t index);
std::size_t NextCaretStop(std::wstring_view text, std::size_t index);
bool IsCaretStop(std::wstring_view text, std::size_t index);
