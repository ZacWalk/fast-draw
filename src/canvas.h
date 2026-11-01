// A 32-bit BGRA8 pixel surface owned by the shared desktop layer.

#pragma once

#include "geometry.h"

#include <cstdint>

class Canvas {
public:
    Canvas() = default;
    ~Canvas() = default;

    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    Canvas(Canvas&&) noexcept = default;
    Canvas& operator=(Canvas&&) noexcept = default;

    bool Resize(int width, int height);

    bool IsValid() const { return surface_.width > 0 && surface_.height > 0; }
    int Width() const { return surface_.width; }
    int Height() const { return surface_.height; }
    Size GetSize() const { return Size(surface_.width, surface_.height); }

    std::uint32_t* Pixels() { return surface_.pixels.data(); }
    const std::uint32_t* Pixels() const { return surface_.pixels.data(); }
    std::uint32_t* Row(int y) { return surface_.pixels.data() + static_cast<std::ptrdiff_t>(y) * Width(); }
    const std::uint32_t* Row(int y) const {
        return surface_.pixels.data() + static_cast<std::ptrdiff_t>(y) * Width();
    }

    pf::pixel_surface& Surface() { return surface_; }
    const pf::pixel_surface& Surface() const { return surface_; }

    // `colour` is 0xAARRGGBB; the surface bytes are BGRA8.
    void Clear(std::uint32_t colour);
    void SetOpaque();

private:
    pf::pixel_surface surface_;
};

inline std::uint32_t ArgbFromColor(Color colour, std::uint8_t alpha = 255) {
    return (static_cast<std::uint32_t>(alpha) << 24) | (colour & 0x00ffffffu);
}
