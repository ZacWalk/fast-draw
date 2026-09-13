#include "ui.h"

#include <algorithm>
#include <vector>

namespace ui {
namespace {

struct CachedFont {
    int dpi;
    bool bold;
    HFONT font;
};

std::vector<CachedFont>& FontCache() {
    static std::vector<CachedFont> cache;
    return cache;
}

}  // namespace

int Dpi(HWND window) {
    const UINT dpi = window ? ::GetDpiForWindow(window) : 96;
    return dpi == 0 ? 96 : static_cast<int>(dpi);
}

int Scale(HWND window, int value) { return ::MulDiv(value, Dpi(window), 96); }

float ScaleF(HWND window, float value) { return value * static_cast<float>(Dpi(window)) / 96.0f; }

HFONT Font(HWND window, bool bold) {
    const int dpi = Dpi(window);
    for (const CachedFont& entry : FontCache()) {
        if (entry.dpi == dpi && entry.bold == bold) return entry.font;
    }

    LOGFONTW logFont{};
    logFont.lfHeight = -::MulDiv(9, dpi, 72);
    logFont.lfWeight = bold ? FW_SEMIBOLD : FW_NORMAL;
    logFont.lfQuality = CLEARTYPE_QUALITY;
    logFont.lfCharSet = DEFAULT_CHARSET;
    ::wcscpy_s(logFont.lfFaceName, L"Segoe UI");

    HFONT font = ::CreateFontIndirectW(&logFont);
    FontCache().push_back({dpi, bold, font});
    return font;
}

bool Surface::Begin(HWND window, PAINTSTRUCT& ps) {
    ::BeginPaint(window, &ps);

    RECT client{};
    ::GetClientRect(window, &client);
    if (!canvas_.Resize(client.right, client.bottom)) return false;

    renderer_.emplace(canvas_);
    renderer_->SetClip(ps.rcPaint);
    return true;
}

void Surface::End(HWND window, PAINTSTRUCT& ps) {
    if (renderer_) {
        ::GdiFlush();
        const RECT& dirty = ps.rcPaint;
        ::BitBlt(ps.hdc, dirty.left, dirty.top, dirty.right - dirty.left,
                 dirty.bottom - dirty.top, canvas_.Dc(), dirty.left, dirty.top, SRCCOPY);
        renderer_.reset();
    }
    ::EndPaint(window, &ps);
}

void FillRoundRect(Renderer& renderer, const RectF& bounds, float radius, COLORREF colour,
                   std::uint8_t alpha) {
    Path path;
    if (radius <= 0.5f)
        path.AddRect(bounds);
    else
        path.AddRoundRect(bounds, radius);
    renderer.Fill(path, SolidPaint(Argb(colour, alpha)));
}

void FrameRoundRect(Renderer& renderer, const RectF& bounds, float radius, COLORREF colour,
                    float width) {
    Path path;
    if (radius <= 0.5f)
        path.AddRect(bounds);
    else
        path.AddRoundRect(bounds, radius);
    renderer.Stroke(path, SolidPaint(Argb(colour)), StrokeStyle{width, true});
}

void Line(Renderer& renderer, Vec2 from, Vec2 to, COLORREF colour, float width) {
    Path path;
    const Vec2 points[2] = {from, to};
    path.AddPolyline(std::span<const Vec2>(points, 2));
    renderer.Stroke(path, SolidPaint(Argb(colour)), StrokeStyle{width, true});
}

void Text(HDC dc, const RECT& bounds, std::wstring_view text, COLORREF colour, UINT format,
          HFONT font) {
    if (text.empty()) return;

    HGDIOBJ previousFont = ::SelectObject(dc, font);
    const COLORREF previousColour = ::SetTextColor(dc, colour);
    const int previousMode = ::SetBkMode(dc, TRANSPARENT);

    RECT box = bounds;
    ::DrawTextW(dc, text.data(), static_cast<int>(text.size()), &box, format);

    ::SetBkMode(dc, previousMode);
    ::SetTextColor(dc, previousColour);
    ::SelectObject(dc, previousFont);
    ::GdiFlush();
}

void Swatch(Renderer& renderer, const RectF& bounds, COLORREF colour, bool empty, bool hot,
            float radius) {
    if (empty) {
        FillRoundRect(renderer, bounds, radius, kPanel);
        Line(renderer, {bounds.left + 2.0f, bounds.bottom - 2.0f},
             {bounds.right - 2.0f, bounds.top + 2.0f}, kDanger, 1.5f);
    } else {
        FillRoundRect(renderer, bounds, radius, colour);
    }

    FrameRoundRect(renderer, bounds.Inflated(-0.5f), radius, hot ? kAccent : kBorderStrong,
                   hot ? 1.8f : 1.0f);
}

}  // namespace ui
