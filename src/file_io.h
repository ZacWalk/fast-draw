#pragma once

#include <filesystem>
#include <stdexcept>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

class FileError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Reads and atomic writes delegate to libdesktop's shared file handles. Both
// throw FileError on failure. Content is treated as raw bytes; the document
// writers produce UTF-8.
std::string ReadFileText(const std::filesystem::path& path);
void WriteFileText(const std::filesystem::path& path, std::string_view text);
void WriteFileBytes(const std::filesystem::path& path, std::span<const std::uint8_t> bytes);
