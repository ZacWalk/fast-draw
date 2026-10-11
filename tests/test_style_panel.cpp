#include "test_framework.h"

#include "style_panel.h"

#include <windows.h>

namespace {

// A style panel in a hidden parent, driven through its window procedure the way
// the mouse drives it.
class Panel {
public:
    Panel() {
        HINSTANCE instance = ::GetModuleHandleW(nullptr);
        StylePanel::RegisterWindowClass(instance);
        parent_ = ::CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 236, 600, nullptr,
                                    nullptr, instance, nullptr);
        window_ = StylePanel::Create(parent_, 1);
        ::MoveWindow(window_, 0, 0, 236, 600, FALSE);

        panel_ = StylePanel::FromHandle(window_);
        if (panel_) panel_->onChange = [this](const Style&) { ++changes; };
    }

    ~Panel() {
        if (parent_) ::DestroyWindow(parent_);
    }

    Panel(const Panel&) = delete;
    Panel& operator=(const Panel&) = delete;

    bool Valid() const { return panel_ != nullptr; }

    void Down(POINT point) const { Send(WM_LBUTTONDOWN, MK_LBUTTON, point); }
    void Up(POINT point) const { Send(WM_LBUTTONUP, 0, point); }

    // The first control straight down the middle of the panel: the Filled
    // toggle, a plain button. The colour swatches sit at the panel's edges and
    // would open a modal picker.
    bool FindButton(POINT& found) {
        RECT client{};
        ::GetClientRect(window_, &client);
        const LONG x = client.right * 6 / 10;
        for (LONG y = 0; y < client.bottom; y += 2) {
            const int before = changes;
            Down({x, y});
            Up({x, y});
            if (changes != before) {
                found = {x, y};
                return true;
            }
        }
        return false;
    }

    int changes = 0;

private:
    void Send(UINT message, WPARAM keys, POINT point) const {
        ::SendMessageW(window_, message, keys, MAKELPARAM(point.x, point.y));
    }

    HWND parent_ = nullptr;
    HWND window_ = nullptr;
    StylePanel* panel_ = nullptr;
};

}  // namespace

TEST(AStyleButtonActsOnlyOnAClickThatStartedOnIt) {
    Panel panel;
    CHECK(panel.Valid());
    if (!panel.Valid()) return;

    POINT button{};
    CHECK(panel.FindButton(button));
    panel.changes = 0;

    // The release of a click that began elsewhere - the one that dismissed a
    // colour popup, say - must not toggle whatever lies under the pointer.
    panel.Up(button);
    CHECK_EQ(panel.changes, 0);

    // Pressing on the button and letting go off it cancels, as with any button.
    panel.Down(button);
    panel.Up({button.x, 0});
    CHECK_EQ(panel.changes, 0);

    panel.Down(button);
    panel.Up(button);
    CHECK_EQ(panel.changes, 1);
}
