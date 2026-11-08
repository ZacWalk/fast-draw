// A 32-bit BGRA8 pixel surface owned by libdesktop's core layer.

#pragma once

#include "geometry.h"
#include "libdesktop/core.h"

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

    bool IsValid() const { return surface_ && !surface_->empty(); }
    int Width() const { return surface_ ? static_cast<int>(surface_->width()) : 0; }
    int Height() const { return surface_ ? static_cast<int>(surface_->height()) : 0; }
    Size GetSize() const { return Size(Width(), Height()); }

    std::uint32_t* Pixels() {
        return surface_ ? reinterpret_cast<std::uint32_t*>(surface_->pixels()) : nullptr;
    }
    const std::uint32_t* Pixels() const {
        return surface_ ? reinterpret_cast<const std::uint32_t*>(surface_->pixels()) : nullptr;
    }
    std::uint32_t* Row(int y) {
        return surface_ ? reinterpret_cast<std::uint32_t*>(surface_->pixels_line(y)) : nullptr;
    }
    const std::uint32_t* Row(int y) const {
        return surface_ ? reinterpret_cast<const std::uint32_t*>(surface_->pixels_line(y)) : nullptr;
    }

    core::surface_ptr Surface() { return surface_; }
    core::const_surface_ptr Surface() const { return surface_; }

    // `colour` is 0xAARRGGBB; the surface bytes are BGRA8.
    void Clear(std::uint32_t colour);
    void SetOpaque();

private:
    core::surface_ptr surface_;
};

inline std::uint32_t ArgbFromColor(Color colour, std::uint8_t alpha = 255) {
    return (static_cast<std::uint32_t>(alpha) << 24) | (colour & 0x00ffffffu);
}
