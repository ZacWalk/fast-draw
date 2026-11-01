// Portable value types used by the document and software renderer.
//
// The platform boundary uses the same coordinate convention (`pf::ipoint`,
// `pf::isize`, `pf::irect`), but these app-owned types deliberately do not
// expose a Windows SDK structure in public headers.

#pragma once

#include "platform.h"

#include <algorithm>
#include <cstdint>

using Color = std::uint32_t;  // 0x00RRGGBB, alpha is added at render time.

constexpr Color Rgb(const std::uint32_t r, const std::uint32_t g, const std::uint32_t b) {
    return (r << 16) | (g << 8) | b;
}

// Kept as a source-compatible color constructor while the document format
// migration settles; unlike the SDK macro this is a portable RGB value.
constexpr Color RGB(const std::uint32_t r, const std::uint32_t g, const std::uint32_t b) {
    return Rgb(r, g, b);
}

struct Size {
    std::int32_t cx = 0;
    std::int32_t cy = 0;

    constexpr Size() = default;
    constexpr Size(const std::int32_t width, const std::int32_t height) : cx(width), cy(height) {}
    constexpr Size(const pf::isize value) : cx(value.cx), cy(value.cy) {}

    constexpr operator pf::isize() const { return {cx, cy}; }
    friend constexpr bool operator==(const Size&, const Size&) = default;
};

struct Point {
    std::int32_t x = 0;
    std::int32_t y = 0;

    constexpr Point() = default;
    constexpr Point(const std::int32_t x_, const std::int32_t y_) : x(x_), y(y_) {}
    constexpr Point(const pf::ipoint value) : x(value.x), y(value.y) {}

    constexpr operator pf::ipoint() const { return {x, y}; }
    friend constexpr bool operator==(const Point&, const Point&) = default;

    constexpr Point operator+(const Size& size) const { return {x + size.cx, y + size.cy}; }
    constexpr Point operator-(const Size& size) const { return {x - size.cx, y - size.cy}; }
    constexpr Size operator-(const Point& point) const { return {x - point.x, y - point.y}; }

    constexpr Point& operator+=(const Size& size) {
        x += size.cx;
        y += size.cy;
        return *this;
    }
    constexpr Point& operator-=(const Size& size) {
        x -= size.cx;
        y -= size.cy;
        return *this;
    }
};

struct Rect {
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t right = 0;
    std::int32_t bottom = 0;

    constexpr Rect() = default;
    constexpr Rect(const std::int32_t l, const std::int32_t t, const std::int32_t r,
                   const std::int32_t b)
        : left(l), top(t), right(r), bottom(b) {}
    constexpr Rect(const Point& topLeft, const Size& size)
        : left(topLeft.x), top(topLeft.y), right(topLeft.x + size.cx), bottom(topLeft.y + size.cy) {}

    constexpr std::int32_t Width() const { return right - left; }
    constexpr std::int32_t Height() const { return bottom - top; }
    constexpr Point TopLeft() const { return {left, top}; }
    constexpr Point BottomRight() const { return {right, bottom}; }
    constexpr Size GetSize() const { return {Width(), Height()}; }
    constexpr bool IsEmpty() const { return left >= right || top >= bottom; }

    constexpr void Normalize() {
        if (left > right) std::swap(left, right);
        if (top > bottom) std::swap(top, bottom);
    }

    constexpr Rect Normalized() const {
        Rect result = *this;
        result.Normalize();
        return result;
    }

    constexpr void Inflate(const std::int32_t dx, const std::int32_t dy) {
        left -= dx;
        top -= dy;
        right += dx;
        bottom += dy;
    }

    constexpr void Offset(const std::int32_t dx, const std::int32_t dy) {
        left += dx;
        right += dx;
        top += dy;
        bottom += dy;
    }

    Rect Intersect(const Rect& other) const {
        return {std::max(left, other.left), std::max(top, other.top),
                std::min(right, other.right), std::min(bottom, other.bottom)};
    }

    Rect Union(const Rect& other) const {
        if (IsEmpty()) return other;
        if (other.IsEmpty()) return *this;
        return {std::min(left, other.left), std::min(top, other.top),
                std::max(right, other.right), std::max(bottom, other.bottom)};
    }

    constexpr Rect& operator+=(const Size& delta) {
        Offset(delta.cx, delta.cy);
        return *this;
    }

    friend constexpr bool operator==(const Rect&, const Rect&) = default;
};
