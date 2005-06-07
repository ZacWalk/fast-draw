// Shared chrome: the palette, DPI helpers and a flicker-free painting surface
// that all of the hand-drawn windows use.

#pragma once

#include "canvas.h"
#include "render.h"

#include <windows.h>

#include <optional>
#include <string_view>

namespace ui {

inline constexpr COLORREF kChrome = RGB(0xF6, 0xF6, 0xF8);
inline constexpr COLORREF kChromeDeep = RGB(0xE3, 0xE4, 0xE9);
inline constexpr COLORREF kPanel = RGB(0xFF, 0xFF, 0xFF);
inline constexpr COLORREF kHover = RGB(0xE8, 0xEF, 0xF9);
inline constexpr COLORREF kPressed = RGB(0xD2, 0xE1, 0xF4);
inline constexpr COLORREF kBorder = RGB(0xD2, 0xD3, 0xDA);
inline constexpr COLORREF kBorderStrong = RGB(0xA6, 0xA7, 0xB2);
inline constexpr COLORREF kText = RGB(0x1B, 0x1B, 0x1F);
inline constexpr COLORREF kTextDim = RGB(0x5F, 0x60, 0x6C);
inline constexpr COLORREF kAccent = RGB(0x0F, 0x6C, 0xBD);
inline constexpr COLORREF kAccentSoft = RGB(0xD8, 0xE7, 0xF8);
inline constexpr COLORREF kDanger = RGB(0xC4, 0x2B, 0x1C);
inline constexpr COLORREF kWorkspace = RGB(0x7A, 0x7C, 0x86);

int Dpi(HWND window);
int Scale(HWND window, int value);
float ScaleF(HWND window, float value);

// Cached; owned by the process.
HFONT Font(HWND window, bool bold = false);

inline std::uint32_t Argb(COLORREF colour, std::uint8_t alpha = 255) {
    return ArgbFromColorRef(colour, alpha);
}

// Draws a whole window into an off-screen canvas and blits it once, so nothing
// ever flickers. Text is still drawn by GDI onto the same surface.
class Surface {
public:
    // Returns false when there is nothing to paint; EndPaint is still required.
    bool Begin(HWND window, PAINTSTRUCT& ps);
    void End(HWND window, PAINTSTRUCT& ps);

    Renderer& GetRenderer() { return *renderer_; }
    HDC Dc() { return canvas_.Dc(); }

private:
    Canvas canvas_;
    std::optional<Renderer> renderer_;
};

void FillRoundRect(Renderer& renderer, const RectF& bounds, float radius, COLORREF colour,
                   std::uint8_t alpha = 255);
void FrameRoundRect(Renderer& renderer, const RectF& bounds, float radius, COLORREF colour,
                    float width);
void Line(Renderer& renderer, Vec2 from, Vec2 to, COLORREF colour, float width);

// Convenience wrappers around DrawTextW; `format` is a DT_* combination.
void Text(HDC dc, const RECT& bounds, std::wstring_view text, COLORREF colour, UINT format,
          HFONT font);

// A checkerboard-free colour swatch with a border, used all over the sidebar.
void Swatch(Renderer& renderer, const RectF& bounds, COLORREF colour, bool empty, bool hot,
            float radius);

}  // namespace ui
