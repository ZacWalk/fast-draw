#include "main_window.h"
#include "resource.h"
#include "shape_palette.h"
#include "style_panel.h"
#include "tab_bar.h"
#include "view.h"

#include <shellapi.h>
#include <windows.h>

#include <filesystem>
#include <vector>

namespace {

std::vector<std::filesystem::path> CommandLinePaths() {
    std::vector<std::filesystem::path> paths;

    int count = 0;
    LPWSTR* arguments = ::CommandLineToArgvW(::GetCommandLineW(), &count);
    if (!arguments) return paths;

    for (int i = 1; i < count; ++i) paths.emplace_back(arguments[i]);

    ::LocalFree(arguments);
    return paths;
}

int Run(HINSTANCE instance, int showCommand) {
    MainWindow::RegisterWindowClass(instance);
    View::RegisterWindowClass(instance);
    TabBar::RegisterWindowClass(instance);
    StylePanel::RegisterWindowClass(instance);
    ShapePalette::RegisterWindowClass(instance);

    HWND frame = MainWindow::Create(instance, showCommand);
    if (!frame) {
        ::MessageBoxW(nullptr, L"Could not create the main window.", L"fast-draw",
                      MB_OK | MB_ICONERROR);
        return 1;
    }

    MainWindow* mainWindow = MainWindow::FromHandle(frame);
    if (mainWindow) {
        const auto paths = CommandLinePaths();
        for (const auto& path : paths) {
            HWND child = mainWindow->NewDocument();
            if (View* view = View::FromHandle(child)) view->Doc().Load(frame, path);
        }
        if (paths.empty()) mainWindow->NewDocument();
    }

    HACCEL accelerators = ::LoadAcceleratorsW(instance, MAKEINTRESOURCEW(IDR_ACCELERATORS));

    MSG message{};
    bool needsIdle = true;

    for (;;) {
        if (needsIdle && mainWindow && !::PeekMessageW(&message, nullptr, 0, 0, PM_NOREMOVE)) {
            mainWindow->OnIdle();
            needsIdle = false;
        }

        if (!::GetMessageW(&message, nullptr, 0, 0)) break;

        if (!::TranslateAcceleratorW(frame, accelerators, &message)) {
            ::TranslateMessage(&message);
            ::DispatchMessageW(&message);
        }

        needsIdle = true;
    }

    return static_cast<int>(message.wParam);
}

}  // namespace

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE, _In_ LPWSTR,
                      _In_ int showCommand) {
    // The manifest already asks for per-monitor v2; this is the belt to its
    // braces for loaders that ignore it.
    ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    return Run(instance, showCommand);
}
