#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

class FileError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Both throw FileError on failure. Content is treated as raw bytes; the
// document and SVG writers produce UTF-8.
std::string ReadFileText(const std::filesystem::path& path);
void WriteFileText(const std::filesystem::path& path, std::string_view text);
