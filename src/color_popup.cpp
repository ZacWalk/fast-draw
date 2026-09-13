#include "color_popup.h"

#include "ui.h"

#include <windowsx.h>

#include <algorithm>

namespace {

constexpr wchar_t kClassName[] = L"FastDrawColourPopup";

constexpr int kColumns = 10;
constexpr int kRows = 6;
constexpr int kCell = 20;
constexpr int kGap = 3;
constexpr int kPadding = 10;

// Neutrals on the top row, then five shades of ten hues.
constexpr COLORREF kPalette[kColumns * kRows] = {
    RGB(0x00, 0x00, 0x00), RGB(0x1F, 0x1F, 0x24), RGB(0x3B, 0x3B, 0x42),
    RGB(0x5C, 0x5C, 0x68), RGB(0x80, 0x80, 0x8C), RGB(0xA6, 0xA7, 0xB2),
    RGB(0xC8, 0xC9, 0xD1), RGB(0xE3, 0xE4, 0xE9), RGB(0xF4, 0xF4, 0xF7),
    RGB(0xFF, 0xFF, 0xFF),

    RGB(0xB3, 0x1E, 0x24), RGB(0xC2, 0x4E, 0x1B), RGB(0xB5, 0x7E, 0x0B),
    RGB(0x6E, 0x8B, 0x12), RGB(0x1E, 0x7A, 0x46), RGB(0x11, 0x7A, 0x77),
    RGB(0x11, 0x5E, 0xA6), RGB(0x2B, 0x40, 0xA8), RGB(0x5B, 0x2C, 0xA0),
    RGB(0x9B, 0x1E, 0x6E),

    RGB(0xD9, 0x3B, 0x3B), RGB(0xE8, 0x70, 0x2A), RGB(0xE0, 0xA3, 0x0C),
    RGB(0x8F, 0xB0, 0x1E), RGB(0x2E, 0x9E, 0x5B), RGB(0x18, 0x9E, 0x99),
    RGB(0x1B, 0x7A, 0xD1), RGB(0x3A, 0x55, 0xD6), RGB(0x76, 0x3D, 0xCC),
    RGB(0xC4, 0x2E, 0x8E),

    RGB(0xEE, 0x6C, 0x6C), RGB(0xF3, 0x96, 0x5C), RGB(0xEE, 0xC0, 0x4C),
    RGB(0xB2, 0xCC, 0x56), RGB(0x63, 0xC1, 0x8B), RGB(0x51, 0xC2, 0xBE),
    RGB(0x55, 0xA3, 0xE4), RGB(0x71, 0x86, 0xE7), RGB(0x9F, 0x74, 0xE0),
    RGB(0xDC, 0x6A, 0xB4),

    RGB(0xF6, 0xA8, 0xA8), RGB(0xF9, 0xC4, 0x9C), RGB(0xF6, 0xDD, 0x92),
    RGB(0xD3, 0xE2, 0x99), RGB(0x9D, 0xDA, 0xB8), RGB(0x94, 0xDB, 0xD8),
    RGB(0x97, 0xC7, 0xF0), RGB(0xA9, 0xB7, 0xF1), RGB(0xC6, 0xAB, 0xEC),
    RGB(0xEC, 0xA5, 0xD3),

    RGB(0xFB, 0xDC, 0xDC), RGB(0xFD, 0xE8, 0xD6), RGB(0xFC, 0xF1, 0xD2),
    RGB(0xEC, 0xF2, 0xD4), RGB(0xD5, 0xEE, 0xE1), RGB(0xD1, 0xEF, 0xEE),
    RGB(0xD3, 0xE7, 0xF9), RGB(0xDC, 0xE3, 0xFA), RGB(0xE8, 0xDD, 0xF7),
    RGB(0xF8, 0xDC, 0xEE),
};

struct PopupState {
    COLORREF initial = 0;
    std::optional<COLORREF> result;
    bool done = false;
    int hot = -1;
    int scale = 96;
    ui::Surface surface;
};

int Scaled(const PopupState& state, int value) { return ::MulDiv(value, state.scale, 96); }

RECT CellBounds(const PopupState& state, int index) {
    const int cell = Scaled(state, kCell);
    const int gap = Scaled(state, kGap);
    const int padding = Scaled(state, kPadding);

    const int column = index % kColumns;
    const int row = index / kColumns;
    const int x = padding + column * (cell + gap);
    const int y = padding + row * (cell + gap);
    return RECT{x, y, x + cell, y + cell};
}

int HitTest(const PopupState& state, POINT point) {
    for (int i = 0; i < kColumns * kRows; ++i) {
        RECT bounds = CellBounds(state, i);
        if (::PtInRect(&bounds, point)) return i;
    }
    return -1;
}

void Render(HWND window, PopupState& state, Renderer& renderer) {
    RECT client{};
    ::GetClientRect(window, &client);

    const RectF area = RectF::FromRect(client);
    ui::FillRoundRect(renderer, area, ui::ScaleF(window, 6.0f), ui::kPanel);
    ui::FrameRoundRect(renderer, area.Inflated(-0.5f), ui::ScaleF(window, 6.0f), ui::kBorderStrong,
                       1.0f);

    for (int i = 0; i < kColumns * kRows; ++i) {
        const RectF bounds = RectF::FromRect(CellBounds(state, i));
        const bool selected = kPalette[i] == state.initial;
        ui::Swatch(renderer, bounds, kPalette[i], false, i == state.hot || selected,
                   ui::ScaleF(window, 3.0f));
    }
}

LRESULT CALLBACK PopupProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<PopupState*>(::GetWindowLongPtrW(window, GWLP_USERDATA));

    switch (message) {
        case WM_CREATE: {
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            ::SetWindowLongPtrW(window, GWLP_USERDATA,
                                reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            return 0;
        }

        case WM_ERASEBKGND: return 1;

        case WM_PAINT: {
            if (!state) break;
            PAINTSTRUCT ps{};
            if (state->surface.Begin(window, ps)) Render(window, *state, state->surface.GetRenderer());
            state->surface.End(window, ps);
            return 0;
        }

        case WM_MOUSEMOVE: {
            if (!state) break;
            const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            const int hot = HitTest(*state, point);
            if (hot != state->hot) {
                state->hot = hot;
                ::InvalidateRect(window, nullptr, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            if (!state) break;
            const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            const int index = HitTest(*state, point);
            if (index >= 0) state->result = kPalette[index];
            state->done = true;
            return 0;
        }

        case WM_LBUTTONDOWN: {
            if (!state) break;
            RECT client{};
            ::GetClientRect(window, &client);
            const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if (!::PtInRect(&client, point)) state->done = true;
            return 0;
        }

        case WM_CAPTURECHANGED:
            if (state) state->done = true;
            return 0;

        default: break;
    }

    return ::DefWindowProcW(window, message, wParam, lParam);
}

void EnsureClass(HINSTANCE instance) {
    static bool registered = false;
    if (registered) return;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &PopupProc;
    wc.hInstance = instance;
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;
    ::RegisterClassExW(&wc);
    registered = true;
}

}  // namespace

std::optional<COLORREF> PickColour(HWND owner, const RECT& anchor, COLORREF initial) {
    HINSTANCE instance = ::GetModuleHandleW(nullptr);
    EnsureClass(instance);

    PopupState state;
    state.initial = initial;
    state.scale = ui::Dpi(owner);

    const int width = Scaled(state, kPadding * 2 + kColumns * kCell + (kColumns - 1) * kGap);
    const int height = Scaled(state, kPadding * 2 + kRows * kCell + (kRows - 1) * kGap);

    int x = anchor.left;
    int y = anchor.bottom + Scaled(state, 4);

    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    if (::GetMonitorInfoW(::MonitorFromRect(&anchor, MONITOR_DEFAULTTONEAREST), &monitor)) {
        x = (std::clamp)(x, static_cast<int>(monitor.rcWork.left),
                         static_cast<int>(monitor.rcWork.right) - width);
        if (y + height > monitor.rcWork.bottom) y = anchor.top - height - Scaled(state, 4);
    }

    HWND window = ::CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, kClassName, L"", WS_POPUP, x,
                                    y, width, height, owner, nullptr, instance, &state);
    if (!window) return std::nullopt;

    ::ShowWindow(window, SW_SHOWNA);
    ::SetCapture(window);

    MSG message{};
    while (!state.done) {
        if (!::GetMessageW(&message, nullptr, 0, 0)) {
            ::PostQuitMessage(static_cast<int>(message.wParam));
            break;
        }
        if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE) break;
        ::TranslateMessage(&message);
        ::DispatchMessageW(&message);
    }

    if (::GetCapture() == window) ::ReleaseCapture();
    ::DestroyWindow(window);
    return state.result;
}
