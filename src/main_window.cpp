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

namespace {

constexpr int kSidebarWidth = 236;
constexpr int kPaletteFloor = 230;

bool Contains(const Rect& rect, const Point point) {
    return point.x >= rect.left && point.x < rect.right &&
           point.y >= rect.top && point.y < rect.bottom;
}

std::wstring ErrorMessage(std::wstring_view action, const std::filesystem::path& path,
                          std::string_view detail) {
    std::wstring result = L"Could not " + std::wstring(action) + L" " +
                          path.filename().wstring() + L".\n\n";
    result += core::utf8_to_utf16(detail);
    return result;
}

std::filesystem::path NativePath(const core::file_path& path) {
    return std::filesystem::path(core::utf8_to_utf16(path.str()));
}

std::vector<Color> PaletteColours() {
    return {
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
    };
}

class SurfaceElement final : public ui::element {
    MainWindow& owner_;

public:
    explicit SurfaceElement(MainWindow& owner) : owner_(owner) {
        padding = {};
        margin = {};
    }

    void render(render::draw_context& dc, const core::pointi element_offset) const override {
        owner_.RenderForPaint();
        if (const auto surface = owner_.PresentedSurface())
            render::draw_surface(dc, surface, bounds.offset(element_offset), 1.0f,
                                 render::texture_sampler::point);
    }

    core::sizei measure(render::measure_context&, const int) const override {
        return bounds.extent();
    }
};

}  // namespace

bool SaveDocument(View& view, const bool forceSaveAs, const ui::frame_ptr& frame) {
    view.FinishEditing();
    Document& document = view.Doc();

    const std::wstring existing = document.Path().wstring();
    core::file_path selected(existing.empty() ? "" : core::utf16_to_utf8(existing));
    if (forceSaveAs || selected.is_empty()) {
        const std::vector<platform::file_dialog_filter> filters = {
            {"fast-draw drawings (*.fdr)", "*.fdr"}, {"All files (*.*)", "*.*"}};
        if (!platform::prompt_for_save_path(selected, filters)) return false;
    }

    const std::filesystem::path path = NativePath(selected);
    const Document::ErrorHandler report =
        [frame](std::wstring_view action, const std::filesystem::path& failed,
                std::string_view detail) {
            const std::wstring message = ErrorMessage(action, failed, detail);
            platform::message_box(nullptr, core::utf16_to_utf8(message), "fast-draw",
                                  {platform::message_box_buttons::ok,
                                   platform::message_box_icon::warning});
        };
    return document.Save(path, report);
}

bool PromptToSave(View& view, const ui::frame_ptr& frame) {
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

MainWindow::MainWindow(std::vector<core::file_path> startupPaths)
    : startupPaths_(std::move(startupPaths)) {
    InstallMenu();
    surfaceElement_ = std::make_shared<SurfaceElement>(*this);
    ConfigureControls();
}

MainWindow::~MainWindow() = default;

View* MainWindow::ActiveView() const {
    if (activeIndex_ < 0 || activeIndex_ >= static_cast<int>(views_.size())) return nullptr;
    return views_[static_cast<std::size_t>(activeIndex_)].get();
}

void MainWindow::Invalidate() {
    dirty_ = true;
    if (frame_) frame_->invalidate();
}

void MainWindow::SetCursor(const platform::cursor cursor) {
    if (frame_) frame_->set_cursor(cursor);
}

void MainWindow::ConfigureControls() {
    tabBar_.onInvalidate = [this] { Invalidate(); };
    tabBar_.onCapture = [this](const bool capture) {
        captured_ = capture;
        captureTarget_ = capture ? CaptureTarget::tabs : CaptureTarget::none;
        if (capture)
            SetCapture(true);
        else
            SetCapture(false);
    };
    tabBar_.onSelect = [this](const int index) { ActivateTab(index); };
    tabBar_.onClose = [this](const int index) { CloseTab(index); };
    tabBar_.onNewTab = [this] { NewDocument(); };

    stylePanel_.onInvalidate = [this] { Invalidate(); };
    stylePanel_.onCapture = [this](const bool capture) {
        captured_ = capture;
        captureTarget_ = capture ? CaptureTarget::style : CaptureTarget::none;
        if (capture)
            SetCapture(true);
        else
            SetCapture(false);
    };
    stylePanel_.onChange = [this](const Style& style) {
        if (View* view = ActiveView()) view->ApplyStyle(style);
    };
    stylePanel_.onPickColour = [this](const Color initial) { return PickColour(initial); };

    palette_.onInvalidate = [this] { Invalidate(); };
    palette_.onCapture = [this](const bool capture) {
        captured_ = capture;
        captureTarget_ = capture ? CaptureTarget::palette : CaptureTarget::none;
        if (capture)
            SetCapture(true);
        else
            SetCapture(false);
    };
    palette_.onDragCursor = [this] { SetCursor(platform::cursor::link); };
    palette_.onSelect = [this](const Tool tool) {
        if (View* view = ActiveView()) view->SetTool(tool);
        palette_.SetActive(tool);
        RefreshChrome();
        Invalidate();
    };
    palette_.onDrop = [this](const ShapeKind kind, const Point point) {
        if (View* view = ActiveView()) {
            const Point root{paletteBounds_.left + point.x, paletteBounds_.top + point.y};
            view->CreateAt(kind, {root.x - viewBounds_.left, root.y - viewBounds_.top});
        }
    };
}

View* MainWindow::NewDocument() {
    auto view = std::make_unique<View>(std::make_unique<Document>(), frame_);
    view->onChanged = [this] {
        RefreshChrome();
        Invalidate();
    };
    view->onInvalidate = [this] { Invalidate(); };
    view->onCapture = [this](const bool capture) {
        captured_ = capture;
        captureTarget_ = capture ? CaptureTarget::view : CaptureTarget::none;
        if (capture)
            SetCapture(true);
        else
            SetCapture(false);
    };
    view->onCursor = [this](const platform::cursor cursor) { SetCursor(cursor); };
    view->onPickColour = [this](const Color initial) { return PickColour(initial); };
    view->onContextMenuRequested = [] {};

    View* result = view.get();
    views_.push_back(std::move(view));
    ActivateTab(static_cast<int>(views_.size()) - 1);
    return result;
}

void MainWindow::ActivateTab(const int index) {
    if (index < 0 || index >= static_cast<int>(views_.size())) return;
    activeIndex_ = index;
    if (frame_) frame_->focus();
    RefreshChrome();
    Invalidate();
}

void MainWindow::CloseTab(const int index) {
    if (index < 0 || index >= static_cast<int>(views_.size())) return;
    View* view = views_[static_cast<std::size_t>(index)].get();
    if (!PromptToSave(*view, frame_)) return;

    const int active = activeIndex_;
    views_.erase(views_.begin() + index);
    if (views_.empty()) {
        activeIndex_ = -1;
        NewDocument();
        return;
    }
    if (index != active && active >= 0) {
        ActivateTab(active > index ? active - 1 : active);
    } else {
        ActivateTab((std::min)(index, static_cast<int>(views_.size()) - 1));
    }
}

void MainWindow::StepTab(const int delta) {
    if (views_.size() < 2) return;
    const int count = static_cast<int>(views_.size());
    ActivateTab(((activeIndex_ + delta) % count + count) % count);
}

void MainWindow::RefreshChrome() {
    RefreshMenuState();
    std::vector<std::wstring> titles;
    titles.reserve(views_.size());
    for (const auto& view : views_) {
        std::wstring title = view->Doc().Title();
        if (view->Doc().IsModified()) title += L" \u2022";
        titles.push_back(std::move(title));
    }
    tabBar_.SetTabs(std::move(titles), activeIndex_);

    if (View* view = ActiveView()) {
        stylePanel_.SetStyle(view->EffectiveStyle(), !view->Selection().empty());
        palette_.SetActive(view->CurrentTool());
        if (frame_) {
            const std::wstring wideTitle = view->Doc().Title();
            frame_->window_text(core::utf16_to_utf8(wideTitle) + " - fast-draw");
        }
    } else if (frame_) {
        frame_->window_text("fast-draw");
    }
}

void MainWindow::Layout(const core::sizei extent, const double dpiScale) {
    extent_ = extent;
    dpiScale_ = dpiScale;

    const int sidebar = sidebarVisible_ ? ui::Scale(dpiScale_, kSidebarWidth) : 0;
    const int tabHeight = ui::Scale(dpiScale_, 34);
    const int width = (std::max)(0, extent.cx);
    const int height = (std::max)(0, extent.cy);

    tabBounds_ = {sidebar, 0, width, tabHeight};
    viewBounds_ = {sidebar, tabHeight, width, height};

    const int styleHeight = sidebarVisible_
                                ? (std::min)(stylePanel_.PreferredHeight(),
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

void MainWindow::CopySurface(const Canvas& source, Canvas& destination, const Point origin) {
    if (!source.IsValid() || !destination.IsValid()) return;
    const int width = (std::min)(source.Width(), destination.Width() - static_cast<int>(origin.x));
    const int height = (std::min)(source.Height(), destination.Height() - static_cast<int>(origin.y));
    if (width <= 0 || height <= 0 || origin.x < 0 || origin.y < 0) return;
    for (int y = 0; y < height; ++y) {
        std::copy_n(source.Row(y), width, destination.Row(origin.y + y) + origin.x);
    }
}

void MainWindow::Render() {
    if (extent_.cx <= 0 || extent_.cy <= 0) return;
    if (!surface_.Resize(extent_.cx, extent_.cy)) return;
    surface_.Clear(ui::Argb(ui::kChrome));

    auto renderComponent = [](Canvas& canvas, const Size size,
                               const auto& draw) {
        if (size.cx <= 0 || size.cy <= 0 || !canvas.Resize(size.cx, size.cy)) return;
        Renderer renderer(canvas);
        draw(renderer, canvas);
    };

    const Size tabSize{tabBounds_.right - tabBounds_.left, tabBounds_.bottom - tabBounds_.top};
    renderComponent(tabSurface_, tabSize,
                    [this](Renderer& renderer, Canvas& canvas) {
                        tabBar_.Render(renderer, canvas);
                    });
    CopySurface(tabSurface_, surface_, {tabBounds_.left, tabBounds_.top});

    if (sidebarVisible_) {
        const Size paletteSize{paletteBounds_.right - paletteBounds_.left,
                               paletteBounds_.bottom - paletteBounds_.top};
        renderComponent(paletteSurface_, paletteSize,
                        [this](Renderer& renderer, Canvas& canvas) {
                            palette_.Render(renderer, canvas);
                        });
        CopySurface(paletteSurface_, surface_, {paletteBounds_.left, paletteBounds_.top});

        const Size styleSize{styleBounds_.right - styleBounds_.left,
                             styleBounds_.bottom - styleBounds_.top};
        renderComponent(styleSurface_, styleSize,
                        [this](Renderer& renderer, Canvas& canvas) {
                            stylePanel_.Render(renderer, canvas);
                        });
        CopySurface(styleSurface_, surface_, {styleBounds_.left, styleBounds_.top});
    }

    if (View* view = ActiveView()) {
        const Size viewSize{viewBounds_.right - viewBounds_.left,
                            viewBounds_.bottom - viewBounds_.top};
        if (viewSize.cx > 0 && viewSize.cy > 0 && surface_.IsValid()) {
            Canvas viewSurface;
            if (viewSurface.Resize(viewSize.cx, viewSize.cy)) {
                Renderer renderer(viewSurface);
                view->Render(renderer, viewSurface);
                CopySurface(viewSurface, surface_, {viewBounds_.left, viewBounds_.top});
            }
        }
    }
    surface_.SetOpaque();
}

void MainWindow::RenderForPaint() {
    RenderIfDirty();
}

void MainWindow::RenderIfDirty() {
    if (!dirty_ && surface_.IsValid()) return;
    Render();
    dirty_ = false;
}

void MainWindow::OpenDocument() {
    std::vector<core::file_path> selected;
    const std::vector<platform::file_dialog_filter> filters = {
        {"fast-draw drawings (*.fdr)", "*.fdr"}, {"All files (*.*)", "*.*"}};
    if (!platform::prompt_for_open_paths(selected, filters, true)) return;

    for (const core::file_path& file : selected) {
        View* view = NewDocument();
        const std::filesystem::path path = NativePath(file);
        const Document::ErrorHandler report =
            [this](std::wstring_view action, const std::filesystem::path& failed,
                   std::string_view detail) { ReportDocumentError(action, failed, detail); };
        if (!view->Doc().Load(path, report)) {
            views_.pop_back();
            activeIndex_ = views_.empty() ? -1 : static_cast<int>(views_.size()) - 1;
            if (views_.empty()) NewDocument();
            RefreshChrome();
            Invalidate();
        }
    }
}

void MainWindow::OnCommand(const int id) {
    View* view = ActiveView();
    switch (id) {
        case ID_FILE_NEW: NewDocument(); return;
        case ID_FILE_OPEN: OpenDocument(); return;
        case ID_FILE_CLOSE: CloseTab(activeIndex_); return;
        case ID_FILE_SAVE:
            if (view) SaveDocument(*view, false, frame_);
            return;
        case ID_FILE_SAVE_AS:
            if (view) SaveDocument(*view, true, frame_);
            return;
        case ID_FILE_PRINT:
            if (view) PrintDocument(*view);
            return;
        case ID_FILE_EXPORT_PNG: {
            if (!view) return;
            const std::wstring wideTitle = view->Doc().Title();
            core::file_path selected(core::utf16_to_utf8(wideTitle));
            const std::vector<platform::file_dialog_filter> filters = {
                {"PNG images (*.png)", "*.png"}, {"All files (*.*)", "*.*"}};
            if (!platform::prompt_for_save_path(selected, filters)) return;
            if (!view->ExportPng(NativePath(selected)))
                platform::message_box(nullptr, "Could not write the image.", "fast-draw",
                                      {platform::message_box_buttons::ok,
                                       platform::message_box_icon::warning});
            return;
        }
        case ID_APP_EXIT:
            if (frame_) frame_->close();
            return;
        case ID_APP_ABOUT:
            platform::message_box(nullptr,
                                  "fast-draw 2.0\n\nA small vector drawing program with its own renderer.",
                                  "About fast-draw",
                                  {platform::message_box_buttons::ok,
                                   platform::message_box_icon::info});
            return;
        case ID_WINDOW_NEXT_TAB: StepTab(1); return;
        case ID_WINDOW_PREV_TAB: StepTab(-1); return;
        case ID_WINDOW_CLOSE_TAB: CloseTab(activeIndex_); return;
        case ID_VIEW_SIDEBAR:
            sidebarVisible_ = !sidebarVisible_;
            Layout(extent_, dpiScale_);
            return;
        default: break;
    }
    if (view && view->OnCommand(id)) {
        RefreshChrome();
        Invalidate();
    }
}

void MainWindow::InstallMenu() {
    const auto accelerator = [](const char32_t key, const uint32_t state) {
        return std::vector<platform::keyboard_accelerator_t>{{key, state}};
    };
    const auto leaf = [this](std::string text, const int id,
                              std::vector<platform::keyboard_accelerator_t> kba = {}) {
        auto command = std::make_shared<ui::command>();
        command->text = std::move(text);
        command->opaque = id;
        command->kba = std::move(kba);
        if (!command->kba.empty()) {
            command->keyboard_accelerator_text =
                platform::format_keyboard_accelerator(command->kba);
        }
        command->invoke = [this, id] { OnCommand(id); };
        return command;
    };
    // The drawn menu draws a null entry as a separator.
    const auto separator = [] { return ui::command_ptr{}; };
    const auto submenu = [this](std::string text, std::vector<ui::command_ptr> children) {
        auto command = std::make_shared<ui::command>();
        command->text = std::move(text);
        command->menu = [this, children = std::move(children)] {
            RefreshCommandState(children);
            return children;
        };
        return command;
    };

    const uint32_t ctrl = platform::keyboard_accelerator_t::control;
    const uint32_t shift = platform::keyboard_accelerator_t::shift;
    const auto ctrlKey = [&](const char32_t key) { return accelerator(key, ctrl); };
    const auto ctrlShiftKey = [&](const char32_t key) { return accelerator(key, ctrl | shift); };

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
    edit[1]->keyboard_accelerator_text =
        platform::format_keyboard_accelerator(edit[1]->kba);

    std::vector<ui::command_ptr> pages{
        leaf("&Letter Landscape", ID_PAGE_LETTER_LANDSCAPE),
        leaf("Letter &Portrait", ID_PAGE_LETTER_PORTRAIT),
        leaf("&A4 Landscape", ID_PAGE_A4_LANDSCAPE),
        leaf("A&4 Portrait", ID_PAGE_A4_PORTRAIT),
        leaf("A&3 Landscape", ID_PAGE_A3_LANDSCAPE),
    };
    std::vector<ui::command_ptr> view{
        leaf("Zoom &In", ID_VIEW_ZOOM_IN, {
            {platform::keys::OEM_PLUS, ctrl}, {platform::keys::ADD, ctrl}}),
        leaf("Zoom &Out", ID_VIEW_ZOOM_OUT, {
            {platform::keys::OEM_MINUS, ctrl}, {platform::keys::SUBTRACT, ctrl}}),
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

    menus_ = {
        submenu("&File", std::move(file)), submenu("&Edit", std::move(edit)),
        submenu("&View", std::move(view)), submenu("&Arrange", std::move(arrange)),
        submenu("&Window", std::move(window)), submenu("&Help", std::move(help)),
    };
    menuBar_ = ui::make_menu_bar(menus_);
    RefreshMenuState();
}

void MainWindow::RefreshCommandState(const std::vector<ui::command_ptr>& commands) {
    const auto needs_view = [](const int id) {
        switch (id) {
            case ID_FILE_NEW:
            case ID_APP_EXIT:
            case ID_APP_ABOUT:
            case ID_VIEW_SIDEBAR:
            case ID_WINDOW_NEXT_TAB:
            case ID_WINDOW_PREV_TAB: return false;
            default: return true;
        }
    };

    for (const auto& command : commands) {
        if (!command) continue;
        if (command->opaque.has_value()) {
            const int id = std::any_cast<int>(command->opaque);
            bool enabled = !needs_view(id);
            bool checked = false;
            if (View* view = ActiveView()) {
                view->QueryCommandState(id, enabled, checked);
            }
            if (id == ID_VIEW_SIDEBAR) checked = sidebarVisible_;
            command->enable = enabled;
            command->checked = checked;
        }
        if (command->menu) RefreshCommandState(command->menu());
    }
}

void MainWindow::RefreshMenuState() { RefreshCommandState(menus_); }

std::optional<Color> MainWindow::PickColour(const Color initial) {
    std::optional<Color> selected;
    std::vector<ui::command_ptr> colours;
    int index = 0;
    for (const Color colour : PaletteColours()) {
        auto command = std::make_shared<ui::command>();
        command->text = "Colour " + std::to_string(index++);
        command->checked = colour == initial;
        command->invoke = [&selected, colour] { selected = colour; };
        colours.push_back(std::move(command));
    }

    const core::pointi cursor = platform::cursor_position();
    frame_->track_menu(core::recti(cursor, core::sizei{}), colours);
    return selected;
}

std::vector<ui::command_ptr> MainWindow::BuildContextMenu() const {
    std::vector<ui::command_ptr> items;
    const auto add = [this, &items](std::string text, const int id) {
        auto command = std::make_shared<ui::command>();
        command->text = std::move(text);
        command->opaque = id;
        command->invoke = [this, id] { const_cast<MainWindow*>(this)->OnCommand(id); };
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

void MainWindow::ShowContextMenu(const Point point) {
    if (!frame_) return;
    const core::recti anchor{point.x, point.y, point.x + 1, point.y + 1};
    frame_->track_menu(anchor, BuildContextMenu());
}

bool MainWindow::OnClose() {
    for (const auto& view : views_)
        if (!PromptToSave(*view, frame_)) return false;
    return true;
}

void MainWindow::ReportDocumentError(const std::wstring_view action,
                                     const std::filesystem::path& path,
                                     const std::string_view detail) const {
    const std::wstring message = ErrorMessage(action, path, detail);
    platform::message_box(nullptr, core::utf16_to_utf8(message), "fast-draw",
                          {platform::message_box_buttons::ok,
                           platform::message_box_icon::warning});
}

namespace {

#ifdef _WIN32
platform::window* NativePlatformWindow(const ui::frame_ptr& frame) {
    if (!frame) return nullptr;
    const std::any handle = frame->handle();
    const auto hwnd = std::any_cast<HWND>(&handle);
    return hwnd ? ::platform_window_from_native(*hwnd) : nullptr;
}
#endif

}  // namespace

bool MainWindow::load_settings(const platform::setting_file_ptr& store) {
    settings_ = store;
    return true;
}

bool MainWindow::init(const std::string_view) {
    if (!settings_) settings_ = platform::settings();

    ui::frame_style style;
    style.drawn_caption = false;
    style.hardware_accelerated = false;
    style.resizable = true;

    frame_ = ui::create_app_frame(shared_from_this(), settings_, shared_from_this(), style);
    if (!frame_) return false;
    frame_->window_text("fast-draw");

    if (views_.empty()) {
        if (startupPaths_.empty()) {
            NewDocument();
        } else {
            const auto paths = std::move(startupPaths_);
            for (const core::file_path& file : paths) {
                View* view = NewDocument();
                const std::filesystem::path path = NativePath(file);
                const Document::ErrorHandler report =
                    [this](std::wstring_view action, const std::filesystem::path& failed,
                           std::string_view detail) { ReportDocumentError(action, failed, detail); };
                view->Doc().Load(path, report);
            }
        }
    }

    platform::set_frame_delay(500);
    RefreshChrome();
    frame_->focus();
    return true;
}

void MainWindow::frame_tick() {
    if (View* view = ActiveView()) view->OnTimer(1);
}

bool MainWindow::can_exit() { return OnClose(); }

void MainWindow::final_exit() {
    platform::set_frame_delay(0);
    frame_.reset();
}

void MainWindow::on_window_layout(render::measure_context& mc, const core::sizei extent,
                                  const bool is_minimized) {
    if (is_minimized || !menuBar_ || !surfaceElement_) return;
    dpiScale_ = mc.scale_factor;
    const int measured = menuBar_->measure(mc, extent.cx).cy;
    menuHeight_ = (std::max)(1, measured);
    menuBar_->layout(mc, {0, 0, extent.cx, menuHeight_});
    const core::sizei body{extent.cx, (std::max)(0, extent.cy - menuHeight_)};
    surfaceElement_->layout(mc, {0, menuHeight_, extent.cx, extent.cy});
    Layout(body, dpiScale_);
}

void MainWindow::on_window_paint(render::draw_context& dc) {
    // The frame host owns the paint order: menu first, then the app-owned surface.
    if (menuBar_) menuBar_->render(dc, {});
    if (surfaceElement_) surfaceElement_->render(dc, {});
}

void MainWindow::visit_elements(
    const std::function<void(const ui::element_ptr&, core::pointi)>& visit) {
    if (menuBar_) visit(menuBar_, {});
    if (surfaceElement_) visit(surfaceElement_, {});
}

void MainWindow::on_window_destroy() {
    captured_ = false;
    captureTarget_ = CaptureTarget::none;
}

void MainWindow::SetCapture(const bool capture) {
#ifdef _WIN32
    if (auto* const window = NativePlatformWindow(frame_)) window->capture(capture);
#else
    (void)capture;
#endif
}

MainWindow::CaptureTarget MainWindow::Route(const core::pointi point) const {
    if (captured_) return captureTarget_;
    if (point.y < menuHeight_) return CaptureTarget::none;
    const Point local{point.x, point.y - menuHeight_};
    const Rect tab{tabBounds_.left, tabBounds_.top, tabBounds_.right, tabBounds_.bottom};
    const Rect palette{paletteBounds_.left, paletteBounds_.top,
                       paletteBounds_.right, paletteBounds_.bottom};
    const Rect style{styleBounds_.left, styleBounds_.top, styleBounds_.right, styleBounds_.bottom};
    const Rect view{viewBounds_.left, viewBounds_.top, viewBounds_.right, viewBounds_.bottom};
    if (Contains(view, local)) return CaptureTarget::view;
    if (Contains(tab, local)) return CaptureTarget::tabs;
    if (Contains(palette, local)) return CaptureTarget::palette;
    if (Contains(style, local)) return CaptureTarget::style;
    return CaptureTarget::none;
}

Point MainWindow::LocalPoint(const CaptureTarget target, const core::pointi point) const {
    const Point local{point.x, point.y - menuHeight_};
    switch (target) {
        case CaptureTarget::view: return {local.x - viewBounds_.left, local.y - viewBounds_.top};
        case CaptureTarget::tabs: return {local.x - tabBounds_.left, local.y - tabBounds_.top};
        case CaptureTarget::palette: return {local.x - paletteBounds_.left, local.y - paletteBounds_.top};
        case CaptureTarget::style: return {local.x - styleBounds_.left, local.y - styleBounds_.top};
        case CaptureTarget::none: return local;
    }
    return local;
}

void MainWindow::on_mouse_move(const core::pointi loc, const bool) {
    const auto route = Route(loc);
    const auto keys = platform::current_key_state();
    const bool alt = keys.alt;
    if (route == CaptureTarget::view) {
        if (View* view = ActiveView()) view->OnMouseMove(LocalPoint(route, loc), keys.shift, keys.control, alt);
    } else if (route == CaptureTarget::tabs) {
        tabBar_.OnMouseMove(LocalPoint(route, loc));
    } else if (route == CaptureTarget::palette) {
        palette_.OnMouseMove(LocalPoint(route, loc));
    } else if (route == CaptureTarget::style) {
        stylePanel_.OnMouseMove(LocalPoint(route, loc));
    }
}

void MainWindow::on_mouse_leave(const core::pointi) {
    tabBar_.OnMouseLeave();
    palette_.OnMouseLeave();
    stylePanel_.OnMouseLeave();
}

void MainWindow::on_mouse_left_button_down(const core::pointi loc,
                                           const platform::key_state keys) {
    const auto route = Route(loc);
    if (route == CaptureTarget::view) {
        if (View* view = ActiveView()) view->OnLeftDown(LocalPoint(route, loc), keys.shift, keys.control, keys.alt);
    } else if (route == CaptureTarget::tabs) {
        tabBar_.OnLeftDown(LocalPoint(route, loc));
    } else if (route == CaptureTarget::palette) {
        palette_.OnLeftDown(LocalPoint(route, loc));
    } else if (route == CaptureTarget::style) {
        stylePanel_.OnLeftDown(LocalPoint(route, loc));
    }
}

void MainWindow::on_mouse_left_button_up(const core::pointi loc,
                                         const platform::key_state keys) {
    const auto route = captured_ ? captureTarget_ : Route(loc);
    if (route == CaptureTarget::view) {
        if (View* view = ActiveView()) view->OnLeftUp(LocalPoint(route, loc), keys.shift, keys.control, keys.alt);
    } else if (route == CaptureTarget::tabs) {
        tabBar_.OnLeftUp(LocalPoint(route, loc));
    } else if (route == CaptureTarget::palette) {
        palette_.OnLeftUp(LocalPoint(route, loc));
    } else if (route == CaptureTarget::style) {
        stylePanel_.OnLeftUp(LocalPoint(route, loc));
    }
}

void MainWindow::on_mouse_middle_button_up(const core::pointi loc,
                                           const platform::key_state) {
    if (Route(loc) == CaptureTarget::tabs) tabBar_.OnMiddleUp(LocalPoint(CaptureTarget::tabs, loc));
}

void MainWindow::on_mouse_left_button_double_click(const core::pointi loc,
                                                   const platform::key_state keys) {
    if (Route(loc) == CaptureTarget::view)
        if (View* view = ActiveView())
            view->OnDoubleClick(LocalPoint(CaptureTarget::view, loc), keys.shift, keys.control, keys.alt);
}

void MainWindow::on_mouse_wheel(const core::pointi loc, const int delta,
                                const platform::key_state keys, bool& was_handled) {
    if (Route(loc) == CaptureTarget::view)
        if (View* view = ActiveView()) {
            view->OnMouseWheel(LocalPoint(CaptureTarget::view, loc), delta, keys.control, keys.shift);
            was_handled = true;
        }
}

bool MainWindow::key_down(const char32_t key, const platform::key_state keys) {
    // Alt and system keys belong to Windows so close and system-menu commands survive.
    if (keys.alt || key == platform::keys::F10) return false;
    RefreshMenuState();
    if (ui::dispatch_accelerator(menus_, key, keys)) return true;
    if (View* view = ActiveView()) return view->OnKeyDown(key, keys.shift);
    return false;
}

bool MainWindow::text_input(const std::string_view text) {
    View* view = ActiveView();
    if (!view || !view->IsEditing()) return false;
    auto it = text.begin();
    while (it != text.end()) view->OnChar(static_cast<char32_t>(core::pop_utf8_char(it, text.end())));
    return true;
}

ui::focus_mode MainWindow::focus_mode() const {
    if (const View* view = ActiveView())
        return view->IsEditing() ? ui::focus_mode::text_edit : ui::focus_mode::view;
    return ui::focus_mode::none;
}

void MainWindow::track_menu(const ui::frame_ptr& parent, const core::recti bounds,
                            const std::vector<ui::command_ptr>& commands) {
    if (parent) parent->track_menu(bounds, commands);
}

bool MainWindow::key_down(const int, const platform::key_state) {
    return false;
}

std::vector<ui::command_ptr> MainWindow::menu(const core::pointi screen) {
#ifdef _WIN32
    if (auto* const window = NativePlatformWindow(frame_)) {
        const auto client = window->screen_to_client(screen);
        const auto route = Route(client);
        if (route != CaptureTarget::view) return {};
        if (View* view = ActiveView()) {
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

void MainWindow::on_mouse_other_button_up(const platform::other_mouse_button&, const core::pointi,
                                          const platform::key_state) {
}

