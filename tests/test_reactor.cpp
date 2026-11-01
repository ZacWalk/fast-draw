#include "test_framework.h"

#include "main_window.h"

#include <algorithm>
#include <memory>

namespace {

class FakeFrame final : public pf::window_frame {
public:
    void set_reactor(pf::frame_reactor_ptr value) override { reactor = std::move(value); }
    void notify_size() override {}
    pf::irect get_client_rect() const override { return {0, 0, 800, 600}; }
    void invalidate() override { ++invalidations; }
    void invalidate_rect(const pf::irect&) override { ++invalidations; }
    void set_focus() override {}
    bool has_focus() const override { return true; }
    void set_capture() override { captured = true; }
    void release_capture() override { captured = false; }
    uint32_t set_timer(uint32_t, uint32_t) override { return 1; }
    void kill_timer(uint32_t) override {}
    pf::ipoint screen_to_client(pf::ipoint point) const override { return point; }
    void set_cursor_shape(pf::cursor_shape) override {}
    void move_window(const pf::irect&) override {}
    void show(bool) override {}
    bool is_visible() const override { return false; }
    void set_text(std::string_view) override {}
    std::string text_from_clipboard() override { return clipboard; }
    bool text_to_clipboard(std::string_view text) override {
        clipboard = std::string(text);
        return true;
    }
    placement get_placement() const override { return {}; }
    void set_placement(const placement&) override {}
    void track_mouse_leave() override {}
    bool is_key_down(unsigned int) const override { return false; }
    bool is_key_down_async(unsigned int) const override { return false; }
    pf::window_frame_ptr create_child(std::string_view, uint32_t, pf::color_t) const & override {
        return nullptr;
    }
    void close() override { closed = true; }
    int message_box(std::string_view, std::string_view, uint32_t) override { return 6; }
    void present_pixels(const uint32_t*, int, int) override {}
    void set_menu(std::vector<pf::menu_command> value) override {
        menu = std::move(value);
        menu_installed = true;
    }
    std::unique_ptr<pf::measure_context> create_measure_context() const override {
        return nullptr;
    }
    void show_popup_menu(const std::vector<pf::menu_command>&,
                         const pf::ipoint&) override {}
    double get_dpi_scale() const override { return 1.0; }
    void accept_drop_files(bool) override {}
    pf::toolbar_frame_ptr create_address_bar(const pf::address_bar_config&) override {
        return nullptr;
    }

    pf::frame_reactor_ptr reactor;
    std::string clipboard;
    std::vector<pf::menu_command> menu;
    int invalidations = 0;
    bool captured = false;
    bool closed = false;
    bool menu_installed = false;
};

}  // namespace

TEST(MainWindowReactorCreatesAHeadlessDocumentAndMenu) {
    auto frame = std::make_shared<FakeFrame>();
    pf::window_frame_ptr window = frame;
    auto main = std::make_shared<MainWindow>(frame, std::vector<pf::file_path>{});
    frame->set_reactor(main);

    pf::message_params params;
    CHECK_EQ(main->handle_message(frame, pf::message_type::create, params), 0u);
    CHECK(main->ActiveView() != nullptr);
    CHECK(frame->menu_installed);

    pf::pixel_surface presented(800, 600);
    {
        auto draw = pf::create_pixel_surface_context(presented);
        CHECK(draw != nullptr);
        main->handle_paint(window, *draw);
    }
    const auto first_scene = presented.pixels;
    CHECK(std::any_of(first_scene.begin(), first_scene.end(),
                      [](const std::uint32_t pixel) { return pixel != 0; }));

    std::fill(presented.pixels.begin(), presented.pixels.end(), 0);
    {
        auto draw = pf::create_pixel_surface_context(presented);
        CHECK(draw != nullptr);
        main->handle_paint(window, *draw);
    }
    CHECK(std::any_of(presented.pixels.begin(), presented.pixels.end(),
                      [](const std::uint32_t pixel) { return pixel != 0; }));

    pf::mouse_params mouse;
    mouse.point = {400, 300};
    CHECK_EQ(main->handle_mouse(frame, pf::mouse_message_type::left_button_down, mouse), 0u);
    CHECK_EQ(main->handle_mouse(frame, pf::mouse_message_type::left_button_up, mouse), 0u);
    CHECK(frame->invalidations > 0);
}
