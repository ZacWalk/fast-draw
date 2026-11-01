#include "test_framework.h"

#include "style_panel.h"

namespace {

class Panel {
public:
    Panel() {
        panel_.SetExtent({236, 600}, 1.0);
        panel_.onChange = [this](const Style&) { ++changes; };
    }

    void Down(const Point point) { panel_.OnLeftDown(point); }
    void Up(const Point point) { panel_.OnLeftUp(point); }

    bool FindButton(Point& found) {
        for (int y = 0; y < 600; y += 2) {
            const int before = changes;
            Down({142, y});
            Up({142, y});
            if (changes != before) {
                found = {142, y};
                return true;
            }
        }
        return false;
    }

    int changes = 0;

private:
    StylePanel panel_;
};

}  // namespace

TEST(AStyleButtonActsOnlyOnAClickThatStartedOnIt) {
    Panel panel;

    Point button{};
    CHECK(panel.FindButton(button));
    panel.changes = 0;

    panel.Up(button);
    CHECK_EQ(panel.changes, 0);

    panel.Down(button);
    panel.Up({button.x, 0});
    CHECK_EQ(panel.changes, 0);

    panel.Down(button);
    panel.Up(button);
    CHECK_EQ(panel.changes, 1);
}
