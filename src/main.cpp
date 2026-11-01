#include "main_window.h"

#include <memory>

namespace {

std::shared_ptr<MainWindow> g_main;

}  // namespace

app_init_result app_init(const pf::window_frame_ptr& main_frame,
                         const std::span<const std::string_view> params) {
    std::vector<pf::file_path> paths;
    for (const std::string_view param : params) {
        if (!param.empty()) paths.emplace_back(param);
    }

    g_main = std::make_shared<MainWindow>(main_frame, std::move(paths));
    main_frame->set_reactor(g_main);

    return {};
}

void app_idle() {
    if (g_main) g_main->OnIdle();
}

void app_destroy() {
    g_main.reset();
}
