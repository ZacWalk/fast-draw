// Small value types over the Win32 POINT / SIZE / RECT structs.
//
// Logical (document) coordinates are 0.01 inch with the y axis pointing *up*,
// so a logical Rect normally has top > bottom. Normalize() produces the
// left<=right / top<=bottom ordering that the GDI region APIs expect.

#pragma once

#include <windows.h>

#include <algorithm>

struct Size : SIZE {
    constexpr Size() : SIZE{0, 0} {}
    constexpr Size(long cx_, long cy_) : SIZE{cx_, cy_} {}
    constexpr Size(SIZE s) : SIZE{s} {}

    friend constexpr bool operator==(const Size& a, const Size& b) {
        return a.cx == b.cx && a.cy == b.cy;
    }
};

struct Point : POINT {
    constexpr Point() : POINT{0, 0} {}
    constexpr Point(long x_, long y_) : POINT{x_, y_} {}
    constexpr Point(POINT p) : POINT{p} {}

    friend constexpr bool operator==(const Point& a, const Point& b) {
        return a.x == b.x && a.y == b.y;
    }

    constexpr Point operator+(const Size& s) const { return {x + s.cx, y + s.cy}; }
    constexpr Point operator-(const Size& s) const { return {x - s.cx, y - s.cy}; }
    constexpr Size operator-(const Point& p) const { return {x - p.x, y - p.y}; }

    constexpr Point& operator+=(const Size& s) {
        x += s.cx;
        y += s.cy;
        return *this;
    }
    constexpr Point& operator-=(const Size& s) {
        x -= s.cx;
        y -= s.cy;
        return *this;
    }
};

struct Rect : RECT {
    constexpr Rect() : RECT{0, 0, 0, 0} {}
    constexpr Rect(long l, long t, long r, long b) : RECT{l, t, r, b} {}
    constexpr Rect(RECT r) : RECT{r} {}
    constexpr Rect(const Point& topLeft, const Size& size)
        : RECT{topLeft.x, topLeft.y, topLeft.x + size.cx, topLeft.y + size.cy} {}

    constexpr long Width() const { return right - left; }
    constexpr long Height() const { return bottom - top; }
    constexpr Point TopLeft() const { return {left, top}; }
    constexpr Point BottomRight() const { return {right, bottom}; }
    constexpr Size GetSize() const { return {Width(), Height()}; }

    constexpr bool IsEmpty() const { return left >= right || top >= bottom; }

    constexpr void Normalize() {
        if (left > right) std::swap(left, right);
        if (top > bottom) std::swap(top, bottom);
    }
    constexpr Rect Normalized() const {
        Rect r = *this;
        r.Normalize();
        return r;
    }

    constexpr void Inflate(long dx, long dy) {
        left -= dx;
        top -= dy;
        right += dx;
        bottom += dy;
    }
    constexpr void Offset(long dx, long dy) {
        left += dx;
        right += dx;
        top += dy;
        bottom += dy;
    }

    // Both operands must be normalized for these to be meaningful.
    Rect Intersect(const Rect& other) const {
        Rect out;
        ::IntersectRect(&out, this, &other);
        return out;
    }
    Rect Union(const Rect& other) const {
        Rect out;
        ::UnionRect(&out, this, &other);
        return out;
    }

    constexpr Rect& operator+=(const Size& delta) {
        Offset(delta.cx, delta.cy);
        return *this;
    }

    friend constexpr bool operator==(const Rect& a, const Rect& b) {
        return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
    }
};
