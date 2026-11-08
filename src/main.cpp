#include "main_window.h"

#include "libdesktop/core.h"
#include "libdesktop/platform.h"

#include <memory>
#include <vector>

int main(int argc, char* argv[]) {
    core::set_app_name("fast-draw");

    std::vector<core::file_path> paths;
    for (int i = 1; i < argc; ++i)
        if (argv[i] && *argv[i]) paths.emplace_back(argv[i]);

    auto app = std::make_shared<MainWindow>(std::move(paths));
    return platform::run(app, argc, argv);
}
