#include "utf8.h"

#include "platform.h"

#include <algorithm>

std::string Utf8Encode(std::wstring_view text) {
    return pf::utf16_to_utf8(text);
}

std::wstring Utf8Decode(std::string_view text) {
    return pf::utf8_to_utf16(text);
}

namespace {

bool IsHighSurrogate(wchar_t c) { return c >= 0xD800 && c <= 0xDBFF; }
bool IsLowSurrogate(wchar_t c) { return c >= 0xDC00 && c <= 0xDFFF; }

}  // namespace

bool IsCaretStop(std::wstring_view text, std::size_t index) {
    if (index == 0 || index >= text.size()) return index <= text.size();
    return !(IsHighSurrogate(text[index - 1]) && IsLowSurrogate(text[index]));
}

std::size_t PreviousCaretStop(std::wstring_view text, std::size_t index) {
    index = (std::min)(index, text.size());
    if (index == 0) return 0;
    --index;
    return IsCaretStop(text, index) ? index : index - 1;
}

std::size_t NextCaretStop(std::wstring_view text, std::size_t index) {
    if (index >= text.size()) return text.size();
    ++index;
    return IsCaretStop(text, index) ? index : index + 1;
}
