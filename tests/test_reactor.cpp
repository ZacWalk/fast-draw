#include "test_framework.h"

#include "libdesktop/ui/test_support.h"
#include "main_window.h"

TEST(MainWindowPaintsOffscreenSurfaceAndRoutesMouse) {
    auto main = std::make_shared<MainWindow>();
    CHECK(main->NewDocument() != nullptr);

    ui::test::fake_measure_context measure;
    main->on_window_layout(measure, {800, 600}, false);

    ui::test::fake_draw_context draw({0, 0, 800, 600});
    main->on_window_paint(draw);
    CHECK(!draw.fills.empty());

    const auto surface = main->PresentedSurface();
    CHECK(surface != nullptr);
    CHECK(!surface->empty());
    CHECK(surface->get_pixel(799, surface->height() - 1) != 0);

    draw.reset();
    main->on_window_paint(draw);
    CHECK(!draw.fills.empty());

    main->on_mouse_left_button_down({400, 300}, {});
    main->on_mouse_left_button_up({400, 300}, {});
    CHECK(main->IsDirty());
}
