#include "test_framework.h"

#include "libdesktop/ui/test_support.h"
#include "main_window.h"

#include <string>

TEST(MainWindowDrawsItsPartsDirectlyAndRoutesThePointer)
{
	auto main = std::make_shared<MainWindow>();
	CHECK(main->NewDocument() != nullptr);

	// The content sits below a 20 pixel menu bar.
	const auto content = main->make_content();
	ui::test::fake_measure_context measure;
	content->layout(measure, {0, 20, 800, 600});

	ui::test::fake_draw_context draw({0, 0, 800, 600});
	content->render(draw, {});
	const std::string text = draw.drawn_text();
	CHECK(text.find("Untitled") != std::string::npos);
	CHECK(text.find("Shapes") != std::string::npos);
	CHECK(text.find("Fill") != std::string::npos);
	CHECK(!draw.polygons.empty());

	const ui::element_host_ptr host = main;
	const auto click = [&](const core::pointi loc)
	{
		ui::hit_test_context context{loc, {0, 0, 800, 600}};
		const auto pointer = content->controller_from_location(host, loc, {}, context);
		CHECK(pointer != nullptr);
		if (!pointer) return;
		pointer->on_mouse_left_button_down(loc, {});
		pointer->on_mouse_left_button_up(loc, {});
	};

	// The palette's rectangle, then a click on the page.
	click({118, 74});
	click({500, 320});
	CHECK_EQ(main->ActiveView()->Doc().Objects().size(), std::size_t(1));

	// The new rectangle reaches the window's context as a polygon, moved into the view.
	draw.reset();
	content->render(draw, {});
	const auto fill = ui::ToCoreColor(Style{}.fill);
	bool found = false;
	for (const auto& polygon : draw.polygons)
	{
		if (polygon.style.fill.color != fill || !polygon.style.has_border()) continue;
		found = true;
		for (const core::pointd point : polygon.points)
		{
			CHECK(point.X > 236.0);
			CHECK(point.Y > 54.0);
		}
	}
	CHECK(found);
}

TEST(TheStylePanelFitsTheSidebarAtAHighDisplayScale)
{
	auto main = std::make_shared<MainWindow>();
	CHECK(main->NewDocument() != nullptr);

	// The first layout at 125% has to size the panel for 125%.
	const auto content = main->make_content();
	ui::test::fake_measure_context measure;
	measure.scale_metrics(1.25);
	content->layout(measure, {0, 0, 1000, 1000});

	ui::test::fake_draw_context draw({0, 0, 1000, 1000});
	content->render(draw, {});
	bool found = false;
	for (const auto& text : draw.texts)
	{
		if (text.text != "11") continue; // the text size, on the panel's last row
		found = true;
		CHECK(text.clip.bottom <= 1000);
	}
	CHECK(found);
}
