#include "test_framework.h"

#include "tab_bar.h"

#include <string>
#include <vector>

namespace {

class TabStrip {
public:
    TabStrip(const int width, const int count, const int active) {
        tabs_.SetExtent({width, 34}, 1.0);
        tabs_.SetTabs(std::vector<std::wstring>(static_cast<std::size_t>(count), L"Untitled"),
                      active);
        tabs_.onSelect = [this](const int index) { selected = index; };
        tabs_.onClose = [this](const int index) { closed = index; };
        tabs_.onNewTab = [this] { ++added; };
    }

    void Click(const int x, const int y) {
        tabs_.OnLeftDown({x, y});
        tabs_.OnLeftUp({x, y});
    }

    int selected = -1;
    int closed = -1;
    int added = 0;

private:
    TabBar tabs_;
};

constexpr int kNewTab = 34;

}  // namespace

TEST(ManyTabsShareTheStripInsteadOfRunningOffItsEnd) {
    TabStrip strip(800, 40, 39);

    strip.Click(755, 17);
    CHECK_EQ(strip.selected, 39);
    CHECK_EQ(strip.closed, -1);

    strip.Click(760 + kNewTab / 2, 17);
    CHECK_EQ(strip.added, 1);

    strip.Click(19 * 5 + 17, 17);
    CHECK_EQ(strip.selected, 5);
    CHECK_EQ(strip.closed, -1);
}

TEST(TabsWithRoomKeepTheirCloseButtons) {
    TabStrip strip(800, 3, 0);

    strip.Click(463, 17);
    CHECK_EQ(strip.closed, 1);

    strip.Click(300, 17);
    CHECK_EQ(strip.selected, 1);

    strip.Click(3 * 240 + kNewTab / 2, 17);
    CHECK_EQ(strip.added, 1);
}
