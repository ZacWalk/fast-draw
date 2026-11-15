#include "test_framework.h"

#include "libdesktop/ui/test_support.h"
#include "style_panel.h"

#include <string>

namespace
{

	class Panel
	{
	public:
		Panel()
		{
			panel_.SetExtent({236, 600}, 1.0);
			panel_.onChange = [this](const Style&)
			{
				++changes;
			};
		}

		void Down(const Point point) { panel_.OnLeftDown(point); }
		void Up(const Point point) { panel_.OnLeftUp(point); }

		bool FindButton(Point& found)
		{
			for (int y = 0; y < 600; y += 2)
			{
				const int before = changes;
				Down({142, y});
				Up({142, y});
				if (changes != before)
				{
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

} // namespace

TEST(AStyleButtonActsOnlyOnAClickThatStartedOnIt)
{
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

TEST(TheStylePanelOffersDownAndRightGradientsAndNoRemovedControls)
{
	StylePanel panel;
	panel.SetExtent({236, 600}, 1.0);
	Style published;
	int changes = 0;
	panel.onChange = [&](const Style& style)
	{
		published = style;
		++changes;
	};

	ui::test::fake_draw_context dc({0, 0, 236, 600});
	panel.Render(dc);

	const std::string down = "\xE2\x86\x93";
	const std::string right = "\xE2\x86\x92";
	const ui::test::fake_draw_context::text_draw* rightChip = nullptr;
	for (const auto& drawn : dc.texts)
	{
		if (drawn.text == right) rightChip = &drawn;
		CHECK(drawn.text != "Round");
		CHECK(drawn.text != "B");
		CHECK(drawn.text != "I");
		CHECK(drawn.text != "\xE2\x86\x98");
		CHECK(drawn.text != "\xE2\x86\x97");
	}
	CHECK(dc.drawn_text().find(down) != std::string::npos);
	CHECK(rightChip != nullptr);
	if (!rightChip) return;

	const Point centre{(rightChip->clip.left + rightChip->clip.right) / 2,
	                   (rightChip->clip.top + rightChip->clip.bottom) / 2};
	panel.OnLeftDown(centre);
	panel.OnLeftUp(centre);
	CHECK_EQ(changes, 1);
	CHECK(published.gradient);
	CHECK(published.gradientDirection == GradientDirection::Right);
}
