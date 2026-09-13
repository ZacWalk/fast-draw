#pragma once

#include <filesystem>

class Canvas;

// Writes the canvas as an 8-bit RGBA PNG. Throws FileError on failure.
void WritePng(const Canvas& canvas, const std::filesystem::path& path);
