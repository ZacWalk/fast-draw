// The application frame. libdesktop owns the native window, drawn menu and event loop;
// this class owns fast-draw's document models and explicit software-raster presentation.

#pragma once

#include "canvas.h"
#include "libdesktop/ui.h"
#include "libdesktop/ui/elements.h"
#include "libdesktop/ui/menu_bar.h"
#include "shape_palette.h"
#include "style_panel.h"
#include "tab_bar.h"
#include "view.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

class MainWindow final : public ui::app,
                         public ui::frame_host,
                         public std::enable_shared_from_this<MainWindow> {
public:
    explicit MainWindow(std::vector<core::file_path> startupPaths = {});
    ~MainWindow() override;

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

    View* ActiveView() const;
    View* NewDocument();
    void RenderForPaint();
    core::const_surface_ptr PresentedSurface() const { return surface_.Surface(); }
    bool IsDirty() const { return dirty_; }

    // ui::app lifecycle and keyboard/text routing.
    bool wants_gpu() const override { return false; }
    bool key_down(char32_t key, platform::key_state keys) override;
    bool text_input(std::string_view text) override;
    ui::focus_mode focus_mode() const override;
    void track_menu(const ui::frame_ptr& parent, core::recti bounds,
                    const std::vector<ui::command_ptr>& commands) override;

    bool parse_command_line(std::string_view) override { return false; }
    bool load_settings(const platform::setting_file_ptr& store) override;
    bool init(std::string_view command_line) override;
    void idle() override {}
    void frame_tick() override;
    bool can_exit() override;
    void exit() override {}
    void final_exit() override;

    // ui::frame_host paints the menu's sibling content and receives the app-owned canvas input.
    void on_window_layout(render::measure_context& mc, core::sizei extent,
                          bool is_minimized) override;
    void on_window_paint(render::draw_context& dc) override;
    void visit_elements(const std::function<void(const ui::element_ptr&, core::pointi)>& visit) override;
    void on_window_destroy() override;
    void on_mouse_move(core::pointi loc, bool is_tracking) override;
    void on_mouse_leave(core::pointi loc) override;
    void on_mouse_left_button_down(core::pointi loc, platform::key_state keys) override;
    void on_mouse_left_button_up(core::pointi loc, platform::key_state keys) override;
    void on_mouse_middle_button_up(core::pointi loc, platform::key_state keys) override;
    void on_mouse_left_button_double_click(core::pointi loc,
                                           platform::key_state keys) override;
    void on_mouse_wheel(core::pointi loc, int delta, platform::key_state keys,
                        bool& was_handled) override;
    bool key_down(int key, platform::key_state keys) override;
    std::vector<ui::command_ptr> menu(core::pointi loc) override;
    void on_mouse_other_button_up(const platform::other_mouse_button& button,
                                  core::pointi loc, platform::key_state keys) override;

private:
    enum class CaptureTarget { none, view, tabs, palette, style };

    void Invalidate();
    void Layout(core::sizei extent, double dpiScale);
    void Render();
    void CopySurface(const Canvas& source, Canvas& destination, Point origin);
    void ConfigureControls();
    void InstallMenu();
    void RefreshCommandState(const std::vector<ui::command_ptr>& commands);
    void RefreshMenuState();
    void RefreshChrome();
    void ActivateTab(int index);
    void CloseTab(int index);
    void StepTab(int delta);
    void OpenDocument();
    void OnCommand(int id);
    bool OnClose();
    void ShowContextMenu(Point point);
    std::vector<ui::command_ptr> BuildContextMenu() const;
    std::optional<Color> PickColour(Color initial);
    void ReportDocumentError(std::wstring_view action, const std::filesystem::path& path,
                             std::string_view detail) const;
    void SetCursor(platform::cursor cursor);
    void SetCapture(bool capture);
    CaptureTarget Route(core::pointi point) const;
    Point LocalPoint(CaptureTarget target, core::pointi point) const;
    void RenderIfDirty();

    ui::frame_ptr frame_;
    platform::setting_file_ptr settings_;
    std::vector<core::file_path> startupPaths_;
    std::vector<std::unique_ptr<View>> views_;
    int activeIndex_ = -1;
    bool sidebarVisible_ = true;
    bool dirty_ = true;
    bool captured_ = false;
    core::sizei extent_{};
    double dpiScale_ = 1.0;
    int menuHeight_ = 0;

    core::recti tabBounds_{};
    core::recti paletteBounds_{};
    core::recti styleBounds_{};
    core::recti viewBounds_{};

    Canvas surface_;
    Canvas tabSurface_;
    Canvas paletteSurface_;
    Canvas styleSurface_;
    TabBar tabBar_;
    ShapePalette palette_;
    StylePanel stylePanel_;
    CaptureTarget captureTarget_ = CaptureTarget::none;

    std::vector<ui::command_ptr> menus_;
    ui::control_ptr menuBar_;
    ui::element_ptr surfaceElement_;
};

// Document actions are kept as small app-level policies around platform dialogs.
bool SaveDocument(View& view, bool forceSaveAs, const ui::frame_ptr& frame);
bool PromptToSave(View& view, const ui::frame_ptr& frame);
