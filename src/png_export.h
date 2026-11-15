#pragma once

#include "libdesktop/core.h"

#include <filesystem>

// Writes a top-down BGRA surface as an 8-bit RGBA PNG. Throws FileError on failure.
void WritePng(const core::const_surface_ptr& surface, const std::filesystem::path& path);
