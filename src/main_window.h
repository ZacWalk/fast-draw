// The application frame.  libdesktop owns the native window, reactor and
// message dispatch; this class owns fast-draw's document models and the
// explicit software-raster presentation.

#pragma once

#include "canvas.h"
#include "platform.h"
#include "shape_palette.h"
#include "style_panel.h"
#include "tab_bar.h"
#include "view.h"

#include <memory>
#include <optional>
#include <vector>

class MainWindow final : public pf::frame_reactor,
                         public std::enable_shared_from_this<MainWindow> {
public:
    MainWindow(pf::window_frame_ptr frame, std::vector<pf::file_path> startupPaths);
    ~MainWindow() override;

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

    View* ActiveView() const;
    View* NewDocument();
    void OnIdle();
    bool IsDirty() const { return dirty_; }

    // The main-loop callback calls this after pf::platform_events().
    void RenderAndPresent();

    // pf::frame_reactor
    uint32_t handle_message(pf::window_frame_ptr window, pf::message_type message,
                            const pf::message_params& params) override;
    uint32_t handle_mouse(pf::window_frame_ptr window, pf::mouse_message_type message,
                          const pf::mouse_params& params) override;
    uint32_t handle_keyboard(pf::window_frame_ptr window,
                             pf::keyboard_message_type message,
                             const pf::keyboard_params& params) override;
    void handle_paint(pf::window_frame_ptr& window, pf::draw_context& draw) override;
    void handle_size(pf::window_frame_ptr& window, pf::isize extent,
                     pf::measure_context& measure) override;

private:
    enum class CaptureTarget { none, view, tabs, palette, style };

    void Invalidate();
    void Layout(pf::isize extent);
    void Render();
    void CopySurface(const Canvas& source, Canvas& destination, Point origin);
    void ConfigureControls();
    void InstallMenu();
    void RefreshChrome();
    void ActivateTab(int index);
    void CloseTab(int index);
    void StepTab(int delta);
    void OpenDocument();
    void OnCommand(int id);
    void OnInitMenu();
    bool OnClose();
    void ShowContextMenu(Point point);
    std::optional<Color> PickColour(Color initial);
    void ReportDocumentError(std::wstring_view action, const std::filesystem::path& path,
                             std::string_view detail) const;
    void SetCursor(pf::cursor_shape cursor);

    pf::window_frame_ptr frame_;
    std::vector<pf::file_path> startupPaths_;
    std::vector<std::unique_ptr<View>> views_;
    int activeIndex_ = -1;
    bool sidebarVisible_ = true;
    bool dirty_ = true;
    bool captured_ = false;
    pf::isize extent_{};
    double dpiScale_ = 1.0;

    Rect tabBounds_{};
    Rect paletteBounds_{};
    Rect styleBounds_{};
    Rect viewBounds_{};

    Canvas surface_;
    Canvas tabSurface_;
    Canvas paletteSurface_;
    Canvas styleSurface_;
    TabBar tabBar_;
    ShapePalette palette_;
    StylePanel stylePanel_;
    CaptureTarget captureTarget_ = CaptureTarget::none;
};

// Document actions are kept as small app-level policies around the domain
// model and libdesktop's file dialogs.
bool SaveDocument(View& view, bool forceSaveAs, const pf::window_frame_ptr& frame);
bool PromptToSave(View& view, const pf::window_frame_ptr& frame);
