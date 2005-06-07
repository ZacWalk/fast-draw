// A 32-bit BGRA surface backed by a DIB section.
//
// Using a DIB section means the software renderer and GDI can write to the same
// pixels: shapes are rasterized directly into `pixels_`, while text still goes
// through GDI via `dc_`.

#pragma once

#include "geometry.h"

#include <windows.h>

#include <cstdint>

class Canvas {
public:
    Canvas() = default;
    ~Canvas() { Destroy(); }

    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    Canvas(Canvas&& other) noexcept { MoveFrom(other); }
    Canvas& operator=(Canvas&& other) noexcept {
        if (this != &other) {
            Destroy();
            MoveFrom(other);
        }
        return *this;
    }

    bool Resize(int width, int height);

    bool IsValid() const { return pixels_ != nullptr; }
    int Width() const { return width_; }
    int Height() const { return height_; }
    Size GetSize() const { return Size(width_, height_); }

    // Rows are contiguous and top-down, so the stride equals the width.
    std::uint32_t* Pixels() { return pixels_; }
    const std::uint32_t* Pixels() const { return pixels_; }
    std::uint32_t* Row(int y) { return pixels_ + static_cast<std::ptrdiff_t>(y) * width_; }
    const std::uint32_t* Row(int y) const {
        return pixels_ + static_cast<std::ptrdiff_t>(y) * width_;
    }

    HDC Dc() const { return dc_; }

    // `colour` is 0xAARRGGBB.
    void Clear(std::uint32_t colour);

    // GDI text output leaves the alpha byte at zero; call this before saving.
    void SetOpaque();

private:
    void Destroy();
    void MoveFrom(Canvas& other);

    HDC dc_ = nullptr;
    HBITMAP bitmap_ = nullptr;
    HGDIOBJ previousBitmap_ = nullptr;
    std::uint32_t* pixels_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

// 0xAARRGGBB from a Win32 COLORREF (0x00BBGGRR).
inline std::uint32_t ArgbFromColorRef(COLORREF colour, std::uint8_t alpha = 255) {
    return (static_cast<std::uint32_t>(alpha) << 24) |
           (static_cast<std::uint32_t>(GetRValue(colour)) << 16) |
           (static_cast<std::uint32_t>(GetGValue(colour)) << 8) |
           static_cast<std::uint32_t>(GetBValue(colour));
}
