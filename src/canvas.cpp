#include "canvas.h"

#include <algorithm>

void Canvas::Clear(const std::uint32_t colour) {
    if (!IsValid()) return;
    for (int y = 0; y < Height(); ++y) std::fill_n(Row(y), Width(), colour);
}

void Canvas::SetOpaque() {
    if (!IsValid()) return;
    for (int y = 0; y < Height(); ++y)
        for (int x = 0; x < Width(); ++x) Row(y)[x] |= 0xff000000u;
}

bool Canvas::Resize(const int width, const int height) {
    if (width <= 0 || height <= 0) {
        surface_.reset();
        return false;
    }

    if (surface_ && surface_->width() == static_cast<uint32_t>(width) &&
        surface_->height() == static_cast<uint32_t>(height))
        return true;

    surface_ = std::make_shared<core::surface>();
    surface_->alloc(width, height, core::texture_format::ARGB, core::orientation::top_left);
    return true;
}
