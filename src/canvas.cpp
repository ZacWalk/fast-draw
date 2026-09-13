#include "canvas.h"

#include <immintrin.h>

#include <algorithm>

void Canvas::Destroy() {
    if (dc_) {
        if (previousBitmap_) ::SelectObject(dc_, previousBitmap_);
        ::DeleteDC(dc_);
    }
    if (bitmap_) ::DeleteObject(bitmap_);

    dc_ = nullptr;
    bitmap_ = nullptr;
    previousBitmap_ = nullptr;
    pixels_ = nullptr;
    width_ = 0;
    height_ = 0;
}

void Canvas::MoveFrom(Canvas& other) {
    dc_ = other.dc_;
    bitmap_ = other.bitmap_;
    previousBitmap_ = other.previousBitmap_;
    pixels_ = other.pixels_;
    width_ = other.width_;
    height_ = other.height_;

    other.dc_ = nullptr;
    other.bitmap_ = nullptr;
    other.previousBitmap_ = nullptr;
    other.pixels_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
}

bool Canvas::Resize(int width, int height) {
    if (width <= 0 || height <= 0) {
        Destroy();
        return false;
    }
    if (width == width_ && height == height_ && pixels_) return true;

    Destroy();

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;  // negative: top-down rows
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    bitmap_ = ::CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap_ || !bits) {
        Destroy();
        return false;
    }

    dc_ = ::CreateCompatibleDC(nullptr);
    if (!dc_) {
        Destroy();
        return false;
    }

    previousBitmap_ = ::SelectObject(dc_, bitmap_);
    pixels_ = static_cast<std::uint32_t*>(bits);
    width_ = width;
    height_ = height;
    return true;
}

void Canvas::Clear(std::uint32_t colour) {
    if (!pixels_) return;

    const std::size_t count = static_cast<std::size_t>(width_) * height_;
    std::size_t i = 0;

    const __m256i wide = _mm256_set1_epi32(static_cast<int>(colour));
    for (; i + 8 <= count; i += 8)
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pixels_ + i), wide);

    for (; i < count; ++i) pixels_[i] = colour;
}

void Canvas::SetOpaque() {
    if (!pixels_) return;

    const std::size_t count = static_cast<std::size_t>(width_) * height_;
    std::size_t i = 0;

    const __m256i alpha = _mm256_set1_epi32(static_cast<int>(0xFF000000u));
    for (; i + 8 <= count; i += 8) {
        auto* target = reinterpret_cast<__m256i*>(pixels_ + i);
        _mm256_storeu_si256(target, _mm256_or_si256(_mm256_loadu_si256(target), alpha));
    }

    for (; i < count; ++i) pixels_[i] |= 0xFF000000u;
}
