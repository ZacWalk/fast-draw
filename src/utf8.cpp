#include "utf8.h"

#include <windows.h>

#include <algorithm>

std::string Utf8Encode(std::wstring_view text) {
    if (text.empty()) return {};

    const int size = ::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                                           nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};

    std::string result(static_cast<std::size_t>(size), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(),
                          size, nullptr, nullptr);
    return result;
}

std::wstring Utf8Decode(std::string_view text) {
    if (text.empty()) return {};

    const int size = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                                           nullptr, 0);
    if (size <= 0) return {};

    std::wstring result(static_cast<std::size_t>(size), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(),
                          size);
    return result;
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
