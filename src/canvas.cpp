#include "canvas.h"

#include <algorithm>

void Canvas::Clear(const std::uint32_t colour) {
    if (!IsValid()) return;
    std::fill(surface_.pixels.begin(), surface_.pixels.end(), colour);
}

void Canvas::SetOpaque() {
    if (!IsValid()) return;
    for (auto& pixel : surface_.pixels) pixel |= 0xff000000u;
}

bool Canvas::Resize(const int width, const int height) {
    if (width <= 0 || height <= 0) {
        surface_ = {};
        return false;
    }

    if (surface_.width == width && surface_.height == height &&
        surface_.pixels.size() == static_cast<std::size_t>(width) * height)
        return true;

    surface_ = pf::pixel_surface(width, height);
    return true;
}
