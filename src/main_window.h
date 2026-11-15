// The application window: a ui::shell whose content, below the menu bar, is the
// tab strip, the sidebar and the active document. libdesktop provides the native
// frame, input, drawn menus, dialogs, DPI, placement and presentation; the
// document model and the views stay app-owned in src/ and draw with the
// window's draw context.

#pragma once

#include "libdesktop/ui.h"
#include "libdesktop/ui/elements.h"
#include "libdesktop/ui/menu_bar.h"
#include "libdesktop/ui/shell.h"
#include "shape_palette.h"
#include "style_panel.h"
#include "tab_bar.h"
#include "view.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

class MainWindow final : public ui::shell
{
public:
	explicit MainWindow(std::vector<core::file_path> startupPaths = {});
	~MainWindow() override;

	MainWindow(const MainWindow&) = delete;
	MainWindow& operator=(const MainWindow&) = delete;

	View* ActiveView() const;
	View* NewDocument();

	// ui::shell: the menus, the content below them, the documents opened once
	// the window is made, and the keys its accelerators leave.
	std::vector<ui::command_ptr> make_menus() override;
	ui::element_ptr make_content() override;
	bool start(std::string_view command_line) override;
	bool key(char32_t key, platform::key_state keys) override;

	// ui::app lifecycle and text routing.
	bool text_input(std::string_view text) override;
	ui::focus_mode focus_mode() const override;
	void frame_tick() override;
	bool can_exit() override;
	void final_exit() override;

	// ui::frame_host input that the content's controller does not receive.
	void on_window_destroy() override;
	void on_mouse_leave(core::pointi loc) override;
	void on_mouse_wheel(core::pointi loc, int delta, platform::key_state keys, bool& was_handled) override;
	std::vector<ui::command_ptr> menu(core::pointi loc) override;

private:
	enum class CaptureTarget
	{
		none,
		view,
		tabs,
		palette,
		style
	};

	// The element below the menu bar, and the controller that hands its pointer
	// input to whichever part lies under it.
	class Content;
	class Pointer;

	void Invalidate(core::recti part = {});
	void LayoutContent(core::recti bounds, double dpiScale);
	void LayoutParts();
	void PaintContent(render::draw_context& dc, core::recti area);
	void ConfigureControls();
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
	void Capture(CaptureTarget target, bool capture);
	CaptureTarget Route(core::pointi point) const;
	Point LocalPoint(CaptureTarget target, core::pointi point) const;

	void OnMouseMove(core::pointi loc);
	void OnLeftDown(core::pointi loc, platform::key_state keys);
	void OnLeftUp(core::pointi loc, platform::key_state keys);
	void OnMiddleUp(core::pointi loc);
	void OnDoubleClick(core::pointi loc, platform::key_state keys);

	std::vector<core::file_path> startupPaths_;
	std::vector<std::unique_ptr<View>> views_;
	int activeIndex_ = -1;
	bool sidebarVisible_ = true;
	bool captured_ = false;
	double dpiScale_ = 1.0;
	platform::cursor cursor_ = platform::cursor::normal;

	// The content in the window's client area, and its parts within it.
	core::recti contentBounds_{};
	core::recti tabBounds_{};
	core::recti paletteBounds_{};
	core::recti styleBounds_{};
	core::recti viewBounds_{};

	TabBar tabBar_;
	ShapePalette palette_;
	StylePanel stylePanel_;
	CaptureTarget captureTarget_ = CaptureTarget::none;
};

// Document actions are kept as small app-level policies around platform dialogs.
bool SaveDocument(View& view, bool forceSaveAs, const ui::frame_ptr& frame);
bool PromptToSave(View& view, const ui::frame_ptr& frame);
