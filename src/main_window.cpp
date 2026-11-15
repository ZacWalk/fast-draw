#include "main_window.h"

#include "document.h"
#include "print_backend.h"
#include "resource.h"

#include "libdesktop/core/strings.h"
#include "libdesktop/platform/dialogs.h"
#include "libdesktop/platform/services.h"
#include "libdesktop/platform/win32.h"

#include <algorithm>
#include <array>
#include <string>

namespace
{

	constexpr int kSidebarWidth = 236;
	constexpr int kPaletteFloor = 230;

	bool Contains(const Rect& rect, const Point point)
	{
		return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
	}

	std::wstring ErrorMessage(std::wstring_view action, const std::filesystem::path& path, std::string_view detail)
	{
		std::wstring result = L"Could not " + std::wstring(action) + L" " + path.filename().wstring() + L".\n\n";
		result += core::utf8_to_utf16(detail);
		return result;
	}

	std::filesystem::path NativePath(const core::file_path& path)
	{
		return std::filesystem::path(core::utf8_to_utf16(path.str()));
	}

	std::vector<Color> PaletteColours()
	{
		return {
			// clang-format off
			RGB(0x00, 0x00, 0x00), RGB(0x1F, 0x1F, 0x24), RGB(0x3B, 0x3B, 0x42),
			RGB(0x5C, 0x5C, 0x68), RGB(0x80, 0x80, 0x8C), RGB(0xA6, 0xA7, 0xB2),
			RGB(0xC8, 0xC9, 0xD1), RGB(0xE3, 0xE4, 0xE9), RGB(0xF4, 0xF4, 0xF7),
			RGB(0xFF, 0xFF, 0xFF), RGB(0xB3, 0x1E, 0x24), RGB(0xC2, 0x4E, 0x1B),
			RGB(0xB5, 0x7E, 0x0B), RGB(0x6E, 0x8B, 0x12), RGB(0x1E, 0x7A, 0x46),
			RGB(0x11, 0x7A, 0x77), RGB(0x11, 0x5E, 0xA6), RGB(0x2B, 0x40, 0xA8),
			RGB(0x5B, 0x2C, 0xA0), RGB(0x9B, 0x1E, 0x6E), RGB(0xD9, 0x3B, 0x3B),
			RGB(0xE8, 0x70, 0x2A), RGB(0xE0, 0xA3, 0x0C), RGB(0x8F, 0xB0, 0x1E),
			RGB(0x2E, 0x9E, 0x5B), RGB(0x18, 0x9E, 0x99), RGB(0x1B, 0x7A, 0xD1),
			RGB(0x3A, 0x55, 0xD6), RGB(0x76, 0x3D, 0xCC), RGB(0xC4, 0x2E, 0x8E),
			RGB(0xEE, 0x6C, 0x6C), RGB(0xF3, 0x96, 0x5C), RGB(0xEE, 0xC0, 0x4C),
			RGB(0xB2, 0xCC, 0x56), RGB(0x63, 0xC1, 0x8B), RGB(0x51, 0xC2, 0xBE),
			RGB(0x55, 0xA3, 0xE4), RGB(0x71, 0x86, 0xE7), RGB(0x9F, 0x74, 0xE0),
			RGB(0xDC, 0x6A, 0xB4), RGB(0xF6, 0xA8, 0xA8), RGB(0xF9, 0xC4, 0x9C),
			RGB(0xF6, 0xDD, 0x92), RGB(0xD3, 0xE2, 0x99), RGB(0x9D, 0xDA, 0xB8),
			RGB(0x94, 0xDB, 0xD8), RGB(0x97, 0xC7, 0xF0), RGB(0xA9, 0xB7, 0xF1),
			RGB(0xC6, 0xAB, 0xEC), RGB(0xEC, 0xA5, 0xD3), RGB(0xFB, 0xDC, 0xDC),
			RGB(0xFD, 0xE8, 0xD6), RGB(0xFC, 0xF1, 0xD2), RGB(0xEC, 0xF2, 0xD4),
			RGB(0xD5, 0xEE, 0xE1), RGB(0xD1, 0xEF, 0xEE), RGB(0xD3, 0xE7, 0xF9),
			RGB(0xDC, 0xE3, 0xFA), RGB(0xE8, 0xDD, 0xF7), RGB(0xF8, 0xDC, 0xEE),
			// clang-format on
		};
	}

	ui::shell_options WindowOptions()
	{
		ui::shell_options options;
		options.title = "fast-draw";
		options.client_size = {1280, 800};
		return options;
	}

} // namespace

class MainWindow::Pointer final : public ui::controller
{
	MainWindow& owner_;

public:
	Pointer(ui::element_host_ptr host, const core::recti bounds, MainWindow& owner) :
		controller(std::move(host), bounds), owner_(owner)
	{
	}

	platform::cursor cursor() const override { return owner_.cursor_; }

	void on_mouse_move(const core::pointi loc) override { owner_.OnMouseMove(loc); }

	void on_mouse_left_button_down(const core::pointi loc, const platform::key_state keys) override
	{
		controller::on_mouse_left_button_down(loc, keys);
		owner_.OnLeftDown(loc, keys);
	}

	void on_mouse_left_button_up(const core::pointi loc, const platform::key_state keys) override
	{
		owner_.OnLeftUp(loc, keys);
	}

	void on_mouse_middle_button_up(const core::pointi loc, const platform::key_state) override
	{
		owner_.OnMiddleUp(loc);
	}

	void on_mouse_left_button_double_click(const core::pointi loc, const platform::key_state keys) override
	{
		owner_.OnDoubleClick(loc, keys);
	}
};

class MainWindow::Content final : public ui::element
{
	MainWindow& owner_;

public:
	explicit Content(MainWindow& owner) :
		element(ui::element_options(ui::flex_item::grow | ui::flex_item::stretch)), owner_(owner)
	{
		padding = {};
		margin = {};
	}

	void layout(render::measure_context& mc, const core::recti bounds_in) override
	{
		bounds = bounds_in;
		owner_.LayoutContent(bounds, mc.scale_factor);
	}

	void render(render::draw_context& dc, const core::pointi element_offset) const override
	{
		owner_.PaintContent(dc, bounds.offset(element_offset));
	}

	ui::controller_ptr controller_from_location(const ui::element_host_ptr& host, const core::pointi loc,
	                                            const core::pointi element_offset, ui::hit_test_context&) override
	{
		const core::recti area = bounds.offset(element_offset);
		if (!area.contains(loc)) return nullptr;
		return std::make_shared<Pointer>(host, area, owner_);
	}
};

bool SaveDocument(View& view, const bool forceSaveAs, const ui::frame_ptr& frame)
{
	view.FinishEditing();
	Document& document = view.Doc();

	const std::wstring existing = document.Path().wstring();
	core::file_path selected(existing.empty() ? "" : core::utf16_to_utf8(existing));
	if (forceSaveAs || selected.is_empty())
	{
		const std::vector<platform::file_dialog_filter> filters = {{"fast-draw drawings (*.fdr)", "*.fdr"},
		                                                           {"All files (*.*)", "*.*"}};
		if (!platform::prompt_for_save_path(selected, filters)) return false;
	}

	const std::filesystem::path path = NativePath(selected);
	const Document::ErrorHandler report =
		[frame](std::wstring_view action, const std::filesystem::path& failed, std::string_view detail)
	{
		const std::wstring message = ErrorMessage(action, failed, detail);
		platform::message_box(nullptr, core::utf16_to_utf8(message), "fast-draw",
		                      {platform::message_box_buttons::ok, platform::message_box_icon::warning});
	};
	return document.Save(path, report);
}

bool PromptToSave(View& view, const ui::frame_ptr& frame)
{
	view.FinishEditing();
	if (!view.Doc().IsModified()) return true;

	const std::wstring wideTitle = view.Doc().Title();
	const std::string title = "Save changes to " + core::utf16_to_utf8(wideTitle) + "?";
	const auto answer = platform::message_box(nullptr, title, "fast-draw",
	                                          {platform::message_box_buttons::yes_no_cancel,
	                                           platform::message_box_icon::warning});
	if (answer == platform::message_box_result::cancel) return false;
	if (answer == platform::message_box_result::no) return true;
	return SaveDocument(view, false, frame);
}

MainWindow::MainWindow(std::vector<core::file_path> startupPaths) :
	shell(WindowOptions()), startupPaths_(std::move(startupPaths))
{
	ConfigureControls();
}

MainWindow::~MainWindow() = default;

View* MainWindow::ActiveView() const
{
	if (activeIndex_ < 0 || activeIndex_ >= static_cast<int>(views_.size())) return nullptr;
	return views_[static_cast<std::size_t>(activeIndex_)].get();
}

ui::element_ptr MainWindow::make_content()
{
	return std::make_shared<Content>(*this);
}

void MainWindow::Invalidate(const core::recti part)
{
	frame()->invalidate(part.is_empty() ? contentBounds_ : part.offset(contentBounds_.left, contentBounds_.top));
}

void MainWindow::SetCursor(const platform::cursor cursor)
{
	cursor_ = cursor;
	frame()->set_cursor(cursor);
}

// The frame holds the pointer from a press to its release; this only says which
// part the moves and the release belong to meanwhile.
void MainWindow::Capture(const CaptureTarget target, const bool capture)
{
	captured_ = capture;
	captureTarget_ = capture ? target : CaptureTarget::none;
}

void MainWindow::ConfigureControls()
{
	tabBar_.onInvalidate = [this]
	{
		Invalidate(tabBounds_);
	};
	tabBar_.onCapture = [this](const bool capture)
	{
		Capture(CaptureTarget::tabs, capture);
	};
	tabBar_.onSelect = [this](const int index)
	{
		ActivateTab(index);
	};
	tabBar_.onClose = [this](const int index)
	{
		CloseTab(index);
	};
	tabBar_.onNewTab = [this]
	{
		NewDocument();
	};

	stylePanel_.onInvalidate = [this]
	{
		Invalidate(styleBounds_);
	};
	stylePanel_.onCapture = [this](const bool capture)
	{
		Capture(CaptureTarget::style, capture);
	};
	stylePanel_.onChange = [this](const Style& style)
	{
		if (View* view = ActiveView()) view->ApplyStyle(style);
	};
	stylePanel_.onPickColour = [this](const Color initial)
	{
		return PickColour(initial);
	};

	palette_.onInvalidate = [this]
	{
		Invalidate(paletteBounds_);
	};
	palette_.onCapture = [this](const bool capture)
	{
		Capture(CaptureTarget::palette, capture);
	};
	palette_.onDragCursor = [this]
	{
		SetCursor(platform::cursor::link);
	};
	palette_.onSelect = [this](const Tool tool)
	{
		if (View* view = ActiveView()) view->SetTool(tool);
		palette_.SetActive(tool);
		RefreshChrome();
		Invalidate();
	};
	palette_.onDrop = [this](const ShapeKind kind, const Point point)
	{
		if (View* view = ActiveView())
		{
			const Point root{paletteBounds_.left + point.x, paletteBounds_.top + point.y};
			view->CreateAt(kind, {root.x - viewBounds_.left, root.y - viewBounds_.top});
		}
	};
}

View* MainWindow::NewDocument()
{
	auto view = std::make_unique<View>(std::make_unique<Document>(), frame());
	view->onChanged = [this]
	{
		RefreshChrome();
		Invalidate(viewBounds_);
	};
	view->onInvalidate = [this]
	{
		Invalidate(viewBounds_);
	};
	view->onCapture = [this](const bool capture)
	{
		Capture(CaptureTarget::view, capture);
	};
	view->onCursor = [this](const platform::cursor cursor)
	{
		SetCursor(cursor);
	};
	view->onPickColour = [this](const Color initial)
	{
		return PickColour(initial);
	};
	view->onContextMenuRequested = [] {};
	view->SetViewport({viewBounds_.width(), viewBounds_.height()}, dpiScale_);

	View* result = view.get();
	views_.push_back(std::move(view));
	ActivateTab(static_cast<int>(views_.size()) - 1);
	return result;
}

void MainWindow::ActivateTab(const int index)
{
	if (index < 0 || index >= static_cast<int>(views_.size())) return;
	activeIndex_ = index;
	frame()->focus();
	RefreshChrome();
	Invalidate();
}

void MainWindow::CloseTab(const int index)
{
	if (index < 0 || index >= static_cast<int>(views_.size())) return;
	View* view = views_[static_cast<std::size_t>(index)].get();
	if (!PromptToSave(*view, frame())) return;

	const int active = activeIndex_;
	views_.erase(views_.begin() + index);
	if (views_.empty())
	{
		activeIndex_ = -1;
		NewDocument();
		return;
	}
	if (index != active && active >= 0)
	{
		ActivateTab(active > index ? active - 1 : active);
	}
	else
	{
		ActivateTab((std::min)(index, static_cast<int>(views_.size()) - 1));
	}
}

void MainWindow::StepTab(const int delta)
{
	if (views_.size() < 2) return;
	const int count = static_cast<int>(views_.size());
	ActivateTab(((activeIndex_ + delta) % count + count) % count);
}

void MainWindow::RefreshChrome()
{
	RefreshMenuState();
	std::vector<std::wstring> titles;
	titles.reserve(views_.size());
	for (const auto& view : views_)
	{
		std::wstring title = view->Doc().Title();
		if (view->Doc().IsModified()) title += L" \u2022";
		titles.push_back(std::move(title));
	}
	tabBar_.SetTabs(std::move(titles), activeIndex_);

	if (View* view = ActiveView())
	{
		stylePanel_.SetStyle(view->EffectiveStyle(), !view->Selection().empty());
		palette_.SetActive(view->CurrentTool());
		const std::wstring wideTitle = view->Doc().Title();
		frame()->window_text(core::utf16_to_utf8(wideTitle) + " - fast-draw");
	}
	else
	{
		frame()->window_text("fast-draw");
	}
}

void MainWindow::LayoutContent(const core::recti bounds, const double dpiScale)
{
	contentBounds_ = bounds;
	dpiScale_ = dpiScale;
	LayoutParts();
}

void MainWindow::LayoutParts()
{
	const int sidebar = sidebarVisible_ ? ui::Scale(dpiScale_, kSidebarWidth) : 0;
	const int tabHeight = ui::Scale(dpiScale_, 34);
	const int width = (std::max)(0, contentBounds_.width());
	const int height = (std::max)(0, contentBounds_.height());

	tabBounds_ = {sidebar, 0, width, tabHeight};
	viewBounds_ = {sidebar, tabHeight, width, height};

	const int styleHeight = sidebarVisible_ ? (std::min)(stylePanel_.PreferredHeight(dpiScale_),
	                                                     (std::max)(0, height - ui::Scale(dpiScale_, kPaletteFloor)))
	                                        : 0;
	const int paletteHeight = sidebarVisible_ ? (std::max)(0, height - styleHeight) : 0;
	paletteBounds_ = {0, 0, sidebar, paletteHeight};
	styleBounds_ = {0, paletteHeight, sidebar, height};

	tabBar_.SetExtent({(std::max)(0, tabBounds_.right - tabBounds_.left),
	                   (std::max)(0, tabBounds_.bottom - tabBounds_.top)},
	                  dpiScale_);
	palette_.SetExtent({(std::max)(0, paletteBounds_.right - paletteBounds_.left),
	                    (std::max)(0, paletteBounds_.bottom - paletteBounds_.top)},
	                   dpiScale_);
	stylePanel_.SetExtent({(std::max)(0, styleBounds_.right - styleBounds_.left),
	                       (std::max)(0, styleBounds_.bottom - styleBounds_.top)},
	                      dpiScale_);

	for (const auto& view : views_)
		view->SetViewport({(std::max)(0, viewBounds_.right - viewBounds_.left),
		                   (std::max)(0, viewBounds_.bottom - viewBounds_.top)},
		                  dpiScale_);
	Invalidate();
}

// Each part draws in its own coordinates, moved to where it sits and clipped to it.
void MainWindow::PaintContent(render::draw_context& dc, const core::recti area)
{
	dc.draw_rect(area, ui::ToCoreColor(ui::kChrome));

	const auto paint = [&dc, area](const core::recti part, const auto& draw)
	{
		const core::recti bounds = part.offset(area.left, area.top);
		if (bounds.is_empty()) return;
		render::scoped_clip clip(dc, bounds);
		ui::offset_draw_context local(dc, bounds.top_left());
		draw(local);
	};

	paint(tabBounds_, [this](render::draw_context& local) { tabBar_.Render(local); });
	if (sidebarVisible_)
	{
		paint(paletteBounds_, [this](render::draw_context& local) { palette_.Render(local); });
		paint(styleBounds_, [this](render::draw_context& local) { stylePanel_.Render(local); });
	}
	if (const View* view = ActiveView())
		paint(viewBounds_, [view](render::draw_context& local) { view->Render(local); });
}

void MainWindow::OpenDocument()
{
	std::vector<core::file_path> selected;
	const std::vector<platform::file_dialog_filter> filters = {{"fast-draw drawings (*.fdr)", "*.fdr"},
	                                                           {"All files (*.*)", "*.*"}};
	if (!platform::prompt_for_open_paths(selected, filters, true)) return;

	for (const core::file_path& file : selected)
	{
		View* view = NewDocument();
		const std::filesystem::path path = NativePath(file);
		const Document::ErrorHandler report =
			[this](std::wstring_view action, const std::filesystem::path& failed, std::string_view detail)
		{
			ReportDocumentError(action, failed, detail);
		};
		if (!view->Doc().Load(path, report))
		{
			views_.pop_back();
			activeIndex_ = views_.empty() ? -1 : static_cast<int>(views_.size()) - 1;
			if (views_.empty()) NewDocument();
			RefreshChrome();
			Invalidate();
		}
	}
}

void MainWindow::OnCommand(const int id)
{
	View* view = ActiveView();
	switch (id)
	{
	case ID_FILE_NEW: NewDocument(); return;
	case ID_FILE_OPEN: OpenDocument(); return;
	case ID_FILE_CLOSE: CloseTab(activeIndex_); return;
	case ID_FILE_SAVE:
		if (view) SaveDocument(*view, false, frame());
		return;
	case ID_FILE_SAVE_AS:
		if (view) SaveDocument(*view, true, frame());
		return;
	case ID_FILE_PRINT:
		if (view) PrintDocument(*view);
		return;
	case ID_FILE_EXPORT_PNG:
	{
		if (!view) return;
		const std::wstring wideTitle = view->Doc().Title();
		core::file_path selected(core::utf16_to_utf8(wideTitle));
		const std::vector<platform::file_dialog_filter> filters = {{"PNG images (*.png)", "*.png"},
		                                                           {"All files (*.*)", "*.*"}};
		if (!platform::prompt_for_save_path(selected, filters)) return;
		if (!view->ExportPng(NativePath(selected)))
			platform::message_box(nullptr, "Could not write the image.", "fast-draw",
			                      {platform::message_box_buttons::ok, platform::message_box_icon::warning});
		return;
	}
	case ID_APP_EXIT: frame()->close(); return;
	case ID_APP_ABOUT:
		platform::message_box(nullptr, "fast-draw 2.0\n\nA small vector drawing program.", "About fast-draw",
		                      {platform::message_box_buttons::ok, platform::message_box_icon::info});
		return;
	case ID_WINDOW_NEXT_TAB: StepTab(1); return;
	case ID_WINDOW_PREV_TAB: StepTab(-1); return;
	case ID_WINDOW_CLOSE_TAB: CloseTab(activeIndex_); return;
	case ID_VIEW_SIDEBAR:
		sidebarVisible_ = !sidebarVisible_;
		LayoutParts();
		return;
	default: break;
	}
	if (view && view->OnCommand(id))
	{
		RefreshChrome();
		Invalidate();
	}
}

std::vector<ui::command_ptr> MainWindow::make_menus()
{
	const auto accelerator = [](const char32_t key, const uint32_t state)
	{
		return std::vector<platform::keyboard_accelerator_t>{{key, state}};
	};
	const auto leaf = [this](std::string text, const int id, std::vector<platform::keyboard_accelerator_t> kba = {})
	{
		auto command = std::make_shared<ui::command>();
		command->text = std::move(text);
		command->opaque = id;
		command->kba = std::move(kba);
		if (!command->kba.empty())
		{
			command->keyboard_accelerator_text = platform::format_keyboard_accelerator(command->kba);
		}
		command->invoke = [this, id]
		{
			OnCommand(id);
		};
		return command;
	};
	// The drawn menu draws a null entry as a separator.
	const auto separator = []
	{
		return ui::command_ptr{};
	};
	const auto submenu = [this](std::string text, std::vector<ui::command_ptr> children)
	{
		auto command = std::make_shared<ui::command>();
		command->text = std::move(text);
		command->menu = [this, children = std::move(children)]
		{
			RefreshCommandState(children);
			return children;
		};
		return command;
	};

	const uint32_t ctrl = platform::keyboard_accelerator_t::control;
	const uint32_t shift = platform::keyboard_accelerator_t::shift;
	const auto ctrlKey = [&](const char32_t key)
	{
		return accelerator(key, ctrl);
	};
	const auto ctrlShiftKey = [&](const char32_t key)
	{
		return accelerator(key, ctrl | shift);
	};

	std::vector<ui::command_ptr> file{
		leaf("&New", ID_FILE_NEW, ctrlKey(U'N')),
		leaf("&Open...", ID_FILE_OPEN, ctrlKey(U'O')),
		leaf("&Close", ID_FILE_CLOSE, ctrlKey(U'W')),
		leaf("&Save", ID_FILE_SAVE, ctrlKey(U'S')),
		leaf("Save &As...", ID_FILE_SAVE_AS),
		leaf("E&xport as PNG...", ID_FILE_EXPORT_PNG),
		separator(),
		leaf("&Print...", ID_FILE_PRINT, ctrlKey(U'P')),
		separator(),
		leaf("E&xit", ID_APP_EXIT),
	};

	std::vector<ui::command_ptr> edit{
		leaf("&Undo", ID_EDIT_UNDO, ctrlKey(U'Z')),
		leaf("&Redo", ID_EDIT_REDO, ctrlKey(U'Y')),
		separator(),
		leaf("Cu&t", ID_EDIT_CUT, ctrlKey(U'X')),
		leaf("&Copy", ID_EDIT_COPY, ctrlKey(U'C')),
		leaf("&Paste", ID_EDIT_PASTE, ctrlKey(U'V')),
		leaf("&Delete", ID_EDIT_CLEAR, accelerator(platform::keys::DEL, 0)),
		separator(),
		leaf("D&uplicate", ID_EDIT_DUPLICATE, ctrlKey(U'D')),
		leaf("Select &All", ID_EDIT_SELECT_ALL, ctrlKey(U'A')),
		leaf("Edit &Label", ID_EDIT_TEXT, accelerator(platform::keys::F2, 0)),
	};
	edit[1]->kba.push_back({U'Z', ctrl | shift});
	edit[1]->keyboard_accelerator_text = platform::format_keyboard_accelerator(edit[1]->kba);

	std::vector<ui::command_ptr> pages{
		// clang-format off
		leaf("&Letter Landscape", ID_PAGE_LETTER_LANDSCAPE),
		leaf("Letter &Portrait", ID_PAGE_LETTER_PORTRAIT),
		leaf("&A4 Landscape", ID_PAGE_A4_LANDSCAPE),
		leaf("A&4 Portrait", ID_PAGE_A4_PORTRAIT),
		leaf("A&3 Landscape", ID_PAGE_A3_LANDSCAPE),
		// clang-format on
	};
	std::vector<ui::command_ptr> view{
		leaf("Zoom &In", ID_VIEW_ZOOM_IN, {{platform::keys::OEM_PLUS, ctrl}, {platform::keys::ADD, ctrl}}),
		leaf("Zoom &Out", ID_VIEW_ZOOM_OUT, {{platform::keys::OEM_MINUS, ctrl}, {platform::keys::SUBTRACT, ctrl}}),
		leaf("&Actual Size", ID_VIEW_ZOOM_ACTUAL, ctrlKey(U'0')),
		leaf("&Fit Page", ID_VIEW_ZOOM_FIT, ctrlKey(U'9')),
		separator(),
		leaf("&Grid", ID_VIEW_GRID),
		leaf("S&nap to Grid", ID_VIEW_SNAP),
		leaf("&Paper Colour...", ID_VIEW_PAPER_COLOUR),
		submenu("Page Si&ze", std::move(pages)),
		separator(),
		leaf("&Sidebar", ID_VIEW_SIDEBAR),
	};

	std::vector<ui::command_ptr> arrange{
		leaf("Bring to &Front", ID_OBJECT_FRONT),
		leaf("Send to &Back", ID_OBJECT_BACK),
		leaf("Bring F&orward", ID_OBJECT_FORWARD),
		leaf("Send Back&ward", ID_OBJECT_BACKWARD),
		separator(),
		leaf("Align &Left", ID_OBJECT_ALIGN_LEFT),
		leaf("Align &Centre", ID_OBJECT_ALIGN_CENTRE),
		leaf("Align &Right", ID_OBJECT_ALIGN_RIGHT),
		leaf("Align &Top", ID_OBJECT_ALIGN_TOP),
		leaf("Align &Middle", ID_OBJECT_ALIGN_MIDDLE),
		leaf("Align Botto&m", ID_OBJECT_ALIGN_BOTTOM),
	};
	std::vector<ui::command_ptr> window{
		leaf("&Next Tab", ID_WINDOW_NEXT_TAB, ctrlKey(platform::keys::TAB)),
		leaf("&Previous Tab", ID_WINDOW_PREV_TAB, ctrlShiftKey(platform::keys::TAB)),
		leaf("&Close Tab", ID_WINDOW_CLOSE_TAB, ctrlKey(U'W')),
	};
	std::vector<ui::command_ptr> help{leaf("&About fast-draw...", ID_APP_ABOUT)};

	return {
		submenu("&File", std::move(file)),     submenu("&Edit", std::move(edit)),
		submenu("&View", std::move(view)),     submenu("&Arrange", std::move(arrange)),
		submenu("&Window", std::move(window)), submenu("&Help", std::move(help)),
	};
}

void MainWindow::RefreshCommandState(const std::vector<ui::command_ptr>& commands)
{
	const auto needs_view = [](const int id)
	{
		switch (id)
		{
		case ID_FILE_NEW:
		case ID_APP_EXIT:
		case ID_APP_ABOUT:
		case ID_VIEW_SIDEBAR:
		case ID_WINDOW_NEXT_TAB:
		case ID_WINDOW_PREV_TAB: return false;
		default: return true;
		}
	};

	for (const auto& command : commands)
	{
		if (!command) continue;
		if (command->opaque.has_value())
		{
			const int id = std::any_cast<int>(command->opaque);
			bool enabled = !needs_view(id);
			bool checked = false;
			if (View* view = ActiveView())
			{
				view->QueryCommandState(id, enabled, checked);
			}
			if (id == ID_VIEW_SIDEBAR) checked = sidebarVisible_;
			command->enable = enabled;
			command->checked = checked;
		}
		if (command->menu) RefreshCommandState(command->menu());
	}
}

void MainWindow::RefreshMenuState()
{
	RefreshCommandState(menus());
}

std::optional<Color> MainWindow::PickColour(const Color initial)
{
	std::optional<Color> selected;
	std::vector<ui::command_ptr> colours;
	int index = 0;
	for (const Color colour : PaletteColours())
	{
		auto command = std::make_shared<ui::command>();
		command->text = "Colour " + std::to_string(index++);
		command->checked = colour == initial;
		command->invoke = [&selected, colour]
		{
			selected = colour;
		};
		colours.push_back(std::move(command));
	}

	const core::pointi cursor = platform::cursor_position();
	frame()->track_menu(core::recti(cursor, core::sizei{}), colours);
	return selected;
}

std::vector<ui::command_ptr> MainWindow::BuildContextMenu() const
{
	std::vector<ui::command_ptr> items;
	const auto add = [this, &items](std::string text, const int id)
	{
		auto command = std::make_shared<ui::command>();
		command->text = std::move(text);
		command->opaque = id;
		command->invoke = [this, id]
		{
			const_cast<MainWindow*>(this)->OnCommand(id);
		};
		bool enabled = false;
		bool checked = false;
		if (View* view = ActiveView()) view->QueryCommandState(id, enabled, checked);
		command->enable = enabled;
		command->checked = checked;
		items.push_back(std::move(command));
	};
	add("Cut", ID_EDIT_CUT);
	add("Copy", ID_EDIT_COPY);
	add("Delete", ID_EDIT_CLEAR);
	add("Duplicate", ID_EDIT_DUPLICATE);
	add("Edit Label", ID_EDIT_TEXT);
	add("Bring to Front", ID_OBJECT_FRONT);
	add("Send to Back", ID_OBJECT_BACK);
	return items;
}

void MainWindow::ShowContextMenu(const Point point)
{
	const core::recti anchor{point.x, point.y, point.x + 1, point.y + 1};
	frame()->track_menu(anchor, BuildContextMenu());
}

bool MainWindow::OnClose()
{
	for (const auto& view : views_)
		if (!PromptToSave(*view, frame())) return false;
	return true;
}

void MainWindow::ReportDocumentError(const std::wstring_view action, const std::filesystem::path& path,
                                     const std::string_view detail) const
{
	const std::wstring message = ErrorMessage(action, path, detail);
	platform::message_box(nullptr, core::utf16_to_utf8(message), "fast-draw",
	                      {platform::message_box_buttons::ok, platform::message_box_icon::warning});
}

namespace
{

#ifdef _WIN32
	platform::window* NativePlatformWindow(const ui::frame_ptr& frame)
	{
		if (!frame) return nullptr;
		const std::any handle = frame->handle();
		const auto hwnd = std::any_cast<HWND>(&handle);
		return hwnd ? ::platform_window_from_native(*hwnd) : nullptr;
	}
#endif

} // namespace

bool MainWindow::start(const std::string_view)
{
	if (views_.empty())
	{
		if (startupPaths_.empty())
		{
			NewDocument();
		}
		else
		{
			const auto paths = std::move(startupPaths_);
			for (const core::file_path& file : paths)
			{
				View* view = NewDocument();
				const std::filesystem::path path = NativePath(file);
				const Document::ErrorHandler report =
					[this](std::wstring_view action, const std::filesystem::path& failed, std::string_view detail)
				{
					ReportDocumentError(action, failed, detail);
				};
				view->Doc().Load(path, report);
			}
		}
	}

	platform::set_frame_delay(500);
	RefreshChrome();
	frame()->focus();
	return true;
}

void MainWindow::frame_tick()
{
	if (View* view = ActiveView()) view->OnTimer(1);
}

bool MainWindow::can_exit()
{
	return OnClose() && shell::can_exit();
}

void MainWindow::final_exit()
{
	platform::set_frame_delay(0);
	shell::final_exit();
}

void MainWindow::on_window_destroy()
{
	shell::on_window_destroy();
	captured_ = false;
	captureTarget_ = CaptureTarget::none;
}

MainWindow::CaptureTarget MainWindow::Route(const core::pointi point) const
{
	if (captured_) return captureTarget_;
	const Point local{point.x - contentBounds_.left, point.y - contentBounds_.top};
	if (!Contains(Rect{0, 0, contentBounds_.width(), contentBounds_.height()}, local)) return CaptureTarget::none;
	const Rect tab{tabBounds_.left, tabBounds_.top, tabBounds_.right, tabBounds_.bottom};
	const Rect palette{paletteBounds_.left, paletteBounds_.top, paletteBounds_.right, paletteBounds_.bottom};
	const Rect style{styleBounds_.left, styleBounds_.top, styleBounds_.right, styleBounds_.bottom};
	const Rect view{viewBounds_.left, viewBounds_.top, viewBounds_.right, viewBounds_.bottom};
	if (Contains(view, local)) return CaptureTarget::view;
	if (Contains(tab, local)) return CaptureTarget::tabs;
	if (Contains(palette, local)) return CaptureTarget::palette;
	if (Contains(style, local)) return CaptureTarget::style;
	return CaptureTarget::none;
}

Point MainWindow::LocalPoint(const CaptureTarget target, const core::pointi point) const
{
	const Point local{point.x - contentBounds_.left, point.y - contentBounds_.top};
	switch (target)
	{
	case CaptureTarget::view: return {local.x - viewBounds_.left, local.y - viewBounds_.top};
	case CaptureTarget::tabs: return {local.x - tabBounds_.left, local.y - tabBounds_.top};
	case CaptureTarget::palette: return {local.x - paletteBounds_.left, local.y - paletteBounds_.top};
	case CaptureTarget::style: return {local.x - styleBounds_.left, local.y - styleBounds_.top};
	case CaptureTarget::none: return local;
	}
	return local;
}

void MainWindow::OnMouseMove(const core::pointi loc)
{
	const auto route = Route(loc);
	const auto keys = platform::current_key_state();
	const bool alt = keys.alt;
	// Only the view chooses a pointer; a palette drag sets its own as it moves.
	if (route != CaptureTarget::view) cursor_ = platform::cursor::normal;
	if (route == CaptureTarget::view)
	{
		if (View* view = ActiveView()) view->OnMouseMove(LocalPoint(route, loc), keys.shift, keys.control, alt);
	}
	else if (route == CaptureTarget::tabs)
	{
		tabBar_.OnMouseMove(LocalPoint(route, loc));
	}
	else if (route == CaptureTarget::palette)
	{
		palette_.OnMouseMove(LocalPoint(route, loc));
	}
	else if (route == CaptureTarget::style)
	{
		stylePanel_.OnMouseMove(LocalPoint(route, loc));
	}
}

void MainWindow::on_mouse_leave(const core::pointi loc)
{
	shell::on_mouse_leave(loc);
	tabBar_.OnMouseLeave();
	palette_.OnMouseLeave();
	stylePanel_.OnMouseLeave();
}

void MainWindow::OnLeftDown(const core::pointi loc, const platform::key_state keys)
{
	const auto route = Route(loc);
	if (route == CaptureTarget::view)
	{
		if (View* view = ActiveView()) view->OnLeftDown(LocalPoint(route, loc), keys.shift, keys.control, keys.alt);
	}
	else if (route == CaptureTarget::tabs)
	{
		tabBar_.OnLeftDown(LocalPoint(route, loc));
	}
	else if (route == CaptureTarget::palette)
	{
		palette_.OnLeftDown(LocalPoint(route, loc));
	}
	else if (route == CaptureTarget::style)
	{
		stylePanel_.OnLeftDown(LocalPoint(route, loc));
	}
}

void MainWindow::OnLeftUp(const core::pointi loc, const platform::key_state keys)
{
	const auto route = captured_ ? captureTarget_ : Route(loc);
	if (route == CaptureTarget::view)
	{
		if (View* view = ActiveView()) view->OnLeftUp(LocalPoint(route, loc), keys.shift, keys.control, keys.alt);
	}
	else if (route == CaptureTarget::tabs)
	{
		tabBar_.OnLeftUp(LocalPoint(route, loc));
	}
	else if (route == CaptureTarget::palette)
	{
		palette_.OnLeftUp(LocalPoint(route, loc));
	}
	else if (route == CaptureTarget::style)
	{
		stylePanel_.OnLeftUp(LocalPoint(route, loc));
	}
}

void MainWindow::OnMiddleUp(const core::pointi loc)
{
	if (Route(loc) == CaptureTarget::tabs) tabBar_.OnMiddleUp(LocalPoint(CaptureTarget::tabs, loc));
}

void MainWindow::OnDoubleClick(const core::pointi loc, const platform::key_state keys)
{
	if (Route(loc) == CaptureTarget::view)
		if (View* view = ActiveView())
			view->OnDoubleClick(LocalPoint(CaptureTarget::view, loc), keys.shift, keys.control, keys.alt);
}

void MainWindow::on_mouse_wheel(const core::pointi loc, const int delta, const platform::key_state keys,
                                bool& was_handled)
{
	if (Route(loc) == CaptureTarget::view)
		if (View* view = ActiveView())
		{
			view->OnMouseWheel(LocalPoint(CaptureTarget::view, loc), delta, keys.control, keys.shift);
			was_handled = true;
		}
}

bool MainWindow::key(const char32_t key, const platform::key_state keys)
{
	// Alt and system keys belong to Windows so close and system-menu commands survive.
	if (keys.alt || key == platform::keys::F10) return false;
	if (View* view = ActiveView()) return view->OnKeyDown(key, keys.shift);
	return false;
}

bool MainWindow::text_input(const std::string_view text)
{
	View* view = ActiveView();
	if (!view || !view->IsEditing()) return false;
	auto it = text.begin();
	while (it != text.end()) view->OnChar(static_cast<char32_t>(core::pop_utf8_char(it, text.end())));
	return true;
}

ui::focus_mode MainWindow::focus_mode() const
{
	if (const View* view = ActiveView()) return view->IsEditing() ? ui::focus_mode::text_edit : ui::focus_mode::view;
	return ui::focus_mode::none;
}

std::vector<ui::command_ptr> MainWindow::menu(const core::pointi screen)
{
#ifdef _WIN32
	if (auto* const window = NativePlatformWindow(frame()))
	{
		const auto client = window->screen_to_client(screen);
		const auto route = Route(client);
		if (route != CaptureTarget::view) return {};
		if (View* view = ActiveView())
		{
			view->OnContextMenu(LocalPoint(route, client));
			RefreshMenuState();
			return BuildContextMenu();
		}
	}
#else
	(void)screen;
#endif
	return {};
}
