#include "test_framework.h"

#include "tab_bar.h"

#include <windows.h>

#include <string>
#include <vector>

namespace {

// A tab strip in a hidden parent, driven through its window procedure the way
// the mouse drives it.
class TabStrip {
public:
    TabStrip(int width, int count, int active) {
        HINSTANCE instance = ::GetModuleHandleW(nullptr);
        TabBar::RegisterWindowClass(instance);
        parent_ = ::CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, width, 100, nullptr,
                                    nullptr, instance, nullptr);
        bar_ = TabBar::Create(parent_, 1);
        ::MoveWindow(bar_, 0, 0, width, 34, FALSE);

        tabs_ = TabBar::FromHandle(bar_);
        if (!tabs_) return;
        tabs_->SetTabs(std::vector<std::wstring>(static_cast<std::size_t>(count), L"Untitled"),
                       active);
        tabs_->onSelect = [this](int index) { selected = index; };
        tabs_->onClose = [this](int index) { closed = index; };
        tabs_->onNewTab = [this] { ++added; };
    }

    ~TabStrip() {
        if (parent_) ::DestroyWindow(parent_);
    }

    TabStrip(const TabStrip&) = delete;
    TabStrip& operator=(const TabStrip&) = delete;

    bool Valid() const { return tabs_ != nullptr; }

    void Click(int x, int y) {
        ::SendMessageW(bar_, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
        ::SendMessageW(bar_, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
    }

    int selected = -1;
    int closed = -1;
    int added = 0;

private:
    HWND parent_ = nullptr;
    HWND bar_ = nullptr;
    TabBar* tabs_ = nullptr;
};

// The test process is not DPI aware, so the strip lays out at 96 dpi: a 34 px
// new-tab button at the end of the tabs, 18 px close buttons, 12 px padding.
constexpr int kNewTab = 34;

}  // namespace

TEST(ManyTabsShareTheStripInsteadOfRunningOffItsEnd) {
    // 40 tabs in 800 px: each is (800 - 34) / 40 = 19 px wide, so the last tab
    // ends at 760 and the new-tab button follows it.
    TabStrip strip(800, 40, 39);
    CHECK(strip.Valid());

    strip.Click(755, 17);
    CHECK_EQ(strip.selected, 39);
    CHECK_EQ(strip.closed, -1);

    strip.Click(760 + kNewTab / 2, 17);
    CHECK_EQ(strip.added, 1);

    // Too narrow for a close button: a click anywhere on a tab selects it.
    strip.Click(19 * 5 + 17, 17);
    CHECK_EQ(strip.selected, 5);
    CHECK_EQ(strip.closed, -1);
}

TEST(TabsWithRoomKeepTheirCloseButtons) {
    // Three tabs in 800 px are 240 px each; tab 1's close button spans
    // x 454..472, y 8..26.
    TabStrip strip(800, 3, 0);
    CHECK(strip.Valid());

    strip.Click(463, 17);
    CHECK_EQ(strip.closed, 1);
    CHECK_EQ(strip.selected, -1);

    strip.Click(300, 17);
    CHECK_EQ(strip.selected, 1);

    strip.Click(3 * 240 + kNewTab / 2, 17);
    CHECK_EQ(strip.added, 1);
}
