#include "main_window.h"

#include "document.h"
#include "print_backend.h"
#include "resource.h"

#include <algorithm>
#include <array>
#include <string>

namespace {

constexpr int kSidebarWidth = 236;
constexpr int kPaletteFloor = 230;

constexpr char kFdrFilter[] =
    "fast-draw drawings (*.fdr)\0*.fdr\0All files (*.*)\0*.*\0";
constexpr char kPngFilter[] = "PNG images (*.png)\0*.png\0All files (*.*)\0*.*\0";

bool Contains(const Rect& rect, const Point point) {
    return point.x >= rect.left && point.x < rect.right &&
           point.y >= rect.top && point.y < rect.bottom;
}

Point LocalPoint(const Rect& bounds, const Point point) {
    return {point.x - bounds.left, point.y - bounds.top};
}

std::wstring ErrorMessage(std::wstring_view action, const std::filesystem::path& path,
                          std::string_view detail) {
    std::wstring result = L"Could not " + std::wstring(action) + L" " +
                          path.filename().wstring() + L".\n\n";
    result += pf::utf8_to_utf16(detail);
    return result;
}

std::filesystem::path NativePath(const pf::file_path& path) {
    return std::filesystem::path(pf::utf8_to_utf16(path.view()));
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

}  // namespace

bool SaveDocument(View& view, const bool forceSaveAs, const pf::window_frame_ptr& frame) {
    view.FinishEditing();
    Document& document = view.Doc();

    const std::wstring existing = document.Path().wstring();
    pf::file_path selected(existing.empty() ? "" : pf::utf16_to_utf8(existing));
    if (forceSaveAs || selected.empty()) {
        selected = pf::save_file_path("Save fast-draw drawing",
                                      selected,
                                      std::string_view(kFdrFilter, sizeof(kFdrFilter) - 1));
        if (selected.empty()) return false;
    }

    const std::filesystem::path path = NativePath(selected);
    const Document::ErrorHandler report =
        [frame](std::wstring_view action, const std::filesystem::path& failed,
                std::string_view detail) {
            const std::wstring message = ErrorMessage(action, failed, detail);
            frame->message_box(pf::utf16_to_utf8(message), "fast-draw",
                               pf::msg_box_style::ok | pf::msg_box_style::icon_warning);
        };
    return document.Save(path, report);
}

bool PromptToSave(View& view, const pf::window_frame_ptr& frame) {
    view.FinishEditing();
    if (!view.Doc().IsModified()) return true;

    const std::wstring wideTitle = view.Doc().Title();
    const std::string title = "Save changes to " +
                              pf::utf16_to_utf8(wideTitle) + "?";
    const int answer = frame->message_box(
        title, "fast-draw",
        pf::msg_box_style::yes_no_cancel | pf::msg_box_style::icon_warning);
    if (answer == pf::msg_box_result::cancel) return false;
    if (answer == pf::msg_box_result::no) return true;
    return SaveDocument(view, false, frame);
}

MainWindow::MainWindow(pf::window_frame_ptr frame, std::vector<pf::file_path> startupPaths)
    : frame_(std::move(frame)), startupPaths_(std::move(startupPaths)) {}

MainWindow::~MainWindow() = default;

View* MainWindow::ActiveView() const {
    if (activeIndex_ < 0 || activeIndex_ >= static_cast<int>(views_.size())) return nullptr;
    return views_[static_cast<std::size_t>(activeIndex_)].get();
}

void MainWindow::Invalidate() {
    dirty_ = true;
    if (frame_) frame_->invalidate();
}

void MainWindow::SetCursor(const pf::cursor_shape cursor) {
    if (frame_) frame_->set_cursor_shape(cursor);
}

void MainWindow::ConfigureControls() {
    tabBar_.onInvalidate = [this] { Invalidate(); };
    tabBar_.onCapture = [this](const bool capture) {
        captured_ = capture;
        captureTarget_ = capture ? CaptureTarget::tabs : CaptureTarget::none;
        if (capture)
            frame_->set_capture();
        else
            frame_->release_capture();
    };
    tabBar_.onSelect = [this](const int index) { ActivateTab(index); };
    tabBar_.onClose = [this](const int index) { CloseTab(index); };
    tabBar_.onNewTab = [this] { NewDocument(); };

    stylePanel_.onInvalidate = [this] { Invalidate(); };
    stylePanel_.onCapture = [this](const bool capture) {
        captured_ = capture;
        captureTarget_ = capture ? CaptureTarget::style : CaptureTarget::none;
        if (capture)
            frame_->set_capture();
        else
            frame_->release_capture();
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
            frame_->set_capture();
        else
            frame_->release_capture();
    };
    palette_.onDragCursor = [this] { SetCursor(pf::cursor_shape::hand); };
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
            frame_->set_capture();
        else
            frame_->release_capture();
    };
    view->onCursor = [this](const pf::cursor_shape cursor) { SetCursor(cursor); };
    view->onPickColour = [this](const Color initial) { return PickColour(initial); };
    view->onContextMenuRequested = [this] {
        const pf::ipoint cursor = pf::platform_cursor_pos();
        ShowContextMenu({cursor.x, cursor.y});
    };

    View* result = view.get();
    views_.push_back(std::move(view));
    ActivateTab(static_cast<int>(views_.size()) - 1);
    return result;
}

void MainWindow::ActivateTab(const int index) {
    if (index < 0 || index >= static_cast<int>(views_.size())) return;
    activeIndex_ = index;
    if (frame_) frame_->set_focus();
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
        const std::wstring wideTitle = view->Doc().Title();
        frame_->set_text(pf::utf16_to_utf8(wideTitle) + " - fast-draw");
    } else {
        frame_->set_text("fast-draw");
    }
}

void MainWindow::Layout(const pf::isize extent) {
    extent_ = extent;
    dpiScale_ = frame_ ? frame_->get_dpi_scale() : 1.0;

    const int sidebar = sidebarVisible_ ? ui::Scale(dpiScale_, kSidebarWidth) : 0;
    const int tabHeight = ui::Scale(dpiScale_, 34);
    const int width = (std::max)(0, static_cast<int>(extent.cx));
    const int height = (std::max)(0, static_cast<int>(extent.cy));

    tabBounds_ = {sidebar, 0, width, tabHeight};
    viewBounds_ = {sidebar, tabHeight, width, height};

    const int styleHeight = sidebarVisible_
                                ? (std::min)(stylePanel_.PreferredHeight(),
                                             (std::max)(0, height - ui::Scale(dpiScale_, kPaletteFloor)))
                                : 0;
    const int paletteHeight = sidebarVisible_ ? (std::max)(0, height - styleHeight) : 0;
    paletteBounds_ = {0, 0, sidebar, paletteHeight};
    styleBounds_ = {0, paletteHeight, sidebar, height};

    tabBar_.SetExtent({(std::max)(0, static_cast<int>(tabBounds_.right - tabBounds_.left)),
                       (std::max)(0, static_cast<int>(tabBounds_.bottom - tabBounds_.top))},
                      dpiScale_);
    palette_.SetExtent({(std::max)(0, static_cast<int>(paletteBounds_.right - paletteBounds_.left)),
                        (std::max)(0, static_cast<int>(paletteBounds_.bottom - paletteBounds_.top))},
                       dpiScale_);
    stylePanel_.SetExtent({(std::max)(0, static_cast<int>(styleBounds_.right - styleBounds_.left)),
                           (std::max)(0, static_cast<int>(styleBounds_.bottom - styleBounds_.top))},
                          dpiScale_);

    for (const auto& view : views_) {
        view->SetViewport({(std::max)(0, static_cast<int>(viewBounds_.right - viewBounds_.left)),
                           (std::max)(0, static_cast<int>(viewBounds_.bottom - viewBounds_.top))},
                          dpiScale_);
    }
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

void MainWindow::RenderAndPresent() {
    if (frame_) frame_->invalidate();
}

void MainWindow::OnIdle() {}

void MainWindow::OpenDocument() {
    const pf::file_path selected = pf::open_file_path(
        "Open fast-draw drawing", std::string_view(kFdrFilter, sizeof(kFdrFilter) - 1));
    if (selected.empty()) return;

    View* view = NewDocument();
    const std::filesystem::path path = NativePath(selected);
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
            const pf::file_path selected = pf::save_file_path(
                "Export PNG", pf::file_path(pf::utf16_to_utf8(wideTitle)),
                std::string_view(kPngFilter, sizeof(kPngFilter) - 1));
            if (selected.empty()) return;
            if (!view->ExportPng(NativePath(selected)))
                frame_->message_box("Could not write the image.", "fast-draw",
                                    pf::msg_box_style::ok | pf::msg_box_style::icon_warning);
            return;
        }
        case ID_APP_EXIT:
            if (OnClose()) frame_->close();
            return;
        case ID_APP_ABOUT:
            frame_->message_box(
                "fast-draw 2.0\n\nA small vector drawing program with its own renderer.",
                "About fast-draw", pf::msg_box_style::ok);
            return;
        case ID_WINDOW_NEXT_TAB: StepTab(1); return;
        case ID_WINDOW_PREV_TAB: StepTab(-1); return;
        case ID_WINDOW_CLOSE_TAB: CloseTab(activeIndex_); return;
        case ID_VIEW_SIDEBAR:
            sidebarVisible_ = !sidebarVisible_;
            Layout(extent_);
            return;
        default: break;
    }
    if (view && view->OnCommand(id)) {
        RefreshChrome();
        Invalidate();
    }
}

void MainWindow::OnInitMenu() {
    // pf::menu_command evaluates enabled/checked callbacks when it creates
    // the native menu; there is no platform-handle state to synchronize here.
}

void MainWindow::InstallMenu() {
    const auto viewEnabled = [this](const int id) {
        return [this, id] {
            bool enabled = false;
            bool checked = false;
            if (View* view = ActiveView()) view->QueryCommandState(id, enabled, checked);
            return enabled;
        };
    };
    const auto viewChecked = [this](const int id) {
        return [this, id] {
            bool enabled = false;
            bool checked = false;
            if (View* view = ActiveView()) view->QueryCommandState(id, enabled, checked);
            return checked;
        };
    };
    const auto leaf = [this](std::string text, const int id,
                             const pf::key_binding accel = {},
                             std::function<bool()> enabled = nullptr,
                             std::function<bool()> checked = nullptr) {
        pf::menu_command command;
        command.text = std::move(text);
        command.id = id;
        command.action = [this, id] { OnCommand(id); };
        command.accel = accel;
        command.is_enabled = std::move(enabled);
        command.is_checked = std::move(checked);
        return command;
    };
    const auto separator = [] {
        return pf::menu_command{};
    };
    const auto submenu = [](std::string text, std::vector<pf::menu_command> children) {
        pf::menu_command command;
        command.text = std::move(text);
        command.children = std::move(children);
        return command;
    };

    using namespace pf::platform_key;
    using namespace pf::key_mod;
    std::vector<pf::menu_command> file{
        leaf("&New", ID_FILE_NEW, {static_cast<unsigned int>('N'), ctrl}),
        leaf("&Open...", ID_FILE_OPEN, {static_cast<unsigned int>('O'), ctrl}),
        leaf("&Close", ID_FILE_CLOSE, {static_cast<unsigned int>('W'), ctrl},
             [this] { return ActiveView() != nullptr; }),
        leaf("&Save", ID_FILE_SAVE, {static_cast<unsigned int>('S'), ctrl},
             [this] { return ActiveView() != nullptr; }),
        leaf("Save &As...", ID_FILE_SAVE_AS, {}, [this] { return ActiveView() != nullptr; }),
        leaf("E&xport as PNG...", ID_FILE_EXPORT_PNG, {},
             [this] { return ActiveView() != nullptr; }),
        separator(),
        leaf("&Print...", ID_FILE_PRINT, {static_cast<unsigned int>('P'), ctrl},
             [this] { return ActiveView() != nullptr; }),
        separator(),
        leaf("E&xit", ID_APP_EXIT),
    };

    std::vector<pf::menu_command> edit{
        leaf("&Undo", ID_EDIT_UNDO, {static_cast<unsigned int>('Z'), ctrl}, viewEnabled(ID_EDIT_UNDO)),
        leaf("&Redo", ID_EDIT_REDO, {static_cast<unsigned int>('Y'), ctrl}, viewEnabled(ID_EDIT_REDO)),
        separator(),
        leaf("Cu&t", ID_EDIT_CUT, {static_cast<unsigned int>('X'), ctrl}, viewEnabled(ID_EDIT_CUT)),
        leaf("&Copy", ID_EDIT_COPY, {static_cast<unsigned int>('C'), ctrl}, viewEnabled(ID_EDIT_COPY)),
        leaf("&Paste", ID_EDIT_PASTE, {static_cast<unsigned int>('V'), ctrl}, viewEnabled(ID_EDIT_PASTE)),
        leaf("&Delete", ID_EDIT_CLEAR, {pf::platform_key::Delete, none}, viewEnabled(ID_EDIT_CLEAR)),
        separator(),
        leaf("D&uplicate", ID_EDIT_DUPLICATE, {static_cast<unsigned int>('D'), ctrl},
             viewEnabled(ID_EDIT_DUPLICATE)),
        leaf("Select &All", ID_EDIT_SELECT_ALL, {static_cast<unsigned int>('A'), ctrl},
             viewEnabled(ID_EDIT_SELECT_ALL)),
        leaf("Edit &Label", ID_EDIT_TEXT, {pf::platform_key::F2, none}, viewEnabled(ID_EDIT_TEXT)),
    };
    edit[1].accel_alt = {static_cast<unsigned int>('Z'), static_cast<uint8_t>(ctrl | shift)};

    std::vector<pf::menu_command> pages{
        leaf("&Letter Landscape", ID_PAGE_LETTER_LANDSCAPE, {}, viewEnabled(ID_PAGE_LETTER_LANDSCAPE),
             viewChecked(ID_PAGE_LETTER_LANDSCAPE)),
        leaf("Letter &Portrait", ID_PAGE_LETTER_PORTRAIT, {}, viewEnabled(ID_PAGE_LETTER_PORTRAIT),
             viewChecked(ID_PAGE_LETTER_PORTRAIT)),
        leaf("&A4 Landscape", ID_PAGE_A4_LANDSCAPE, {}, viewEnabled(ID_PAGE_A4_LANDSCAPE),
             viewChecked(ID_PAGE_A4_LANDSCAPE)),
        leaf("A&4 Portrait", ID_PAGE_A4_PORTRAIT, {}, viewEnabled(ID_PAGE_A4_PORTRAIT),
             viewChecked(ID_PAGE_A4_PORTRAIT)),
        leaf("A&3 Landscape", ID_PAGE_A3_LANDSCAPE, {}, viewEnabled(ID_PAGE_A3_LANDSCAPE),
             viewChecked(ID_PAGE_A3_LANDSCAPE)),
    };
    std::vector<pf::menu_command> view{
        leaf("Zoom &In", ID_VIEW_ZOOM_IN, {pf::platform_key::Equals, ctrl}, viewEnabled(ID_VIEW_ZOOM_IN)),
        leaf("Zoom &Out", ID_VIEW_ZOOM_OUT, {pf::platform_key::Minus, ctrl}, viewEnabled(ID_VIEW_ZOOM_OUT)),
        leaf("&Actual Size", ID_VIEW_ZOOM_ACTUAL, {static_cast<unsigned int>('0'), ctrl},
             viewEnabled(ID_VIEW_ZOOM_ACTUAL)),
        leaf("&Fit Page", ID_VIEW_ZOOM_FIT, {static_cast<unsigned int>('9'), ctrl},
             viewEnabled(ID_VIEW_ZOOM_FIT)),
        separator(),
        leaf("&Grid", ID_VIEW_GRID, {}, viewEnabled(ID_VIEW_GRID), viewChecked(ID_VIEW_GRID)),
        leaf("S&nap to Grid", ID_VIEW_SNAP, {}, viewEnabled(ID_VIEW_SNAP), viewChecked(ID_VIEW_SNAP)),
        leaf("&Paper Colour...", ID_VIEW_PAPER_COLOUR, {}, viewEnabled(ID_VIEW_PAPER_COLOUR)),
        submenu("Page Si&ze", std::move(pages)),
        separator(),
        leaf("&Sidebar", ID_VIEW_SIDEBAR, {}, nullptr, [this] { return sidebarVisible_; }),
    };
    view[0].accel_alt = {pf::platform_key::Add, ctrl};
    view[1].accel_alt = {pf::platform_key::Subtract, ctrl};

    std::vector<pf::menu_command> arrange{
        leaf("Bring to &Front", ID_OBJECT_FRONT, {}, viewEnabled(ID_OBJECT_FRONT)),
        leaf("Send to &Back", ID_OBJECT_BACK, {}, viewEnabled(ID_OBJECT_BACK)),
        leaf("Bring F&orward", ID_OBJECT_FORWARD, {}, viewEnabled(ID_OBJECT_FORWARD)),
        leaf("Send Back&ward", ID_OBJECT_BACKWARD, {}, viewEnabled(ID_OBJECT_BACKWARD)),
        separator(),
        leaf("Align &Left", ID_OBJECT_ALIGN_LEFT, {}, viewEnabled(ID_OBJECT_ALIGN_LEFT)),
        leaf("Align &Centre", ID_OBJECT_ALIGN_CENTRE, {}, viewEnabled(ID_OBJECT_ALIGN_CENTRE)),
        leaf("Align &Right", ID_OBJECT_ALIGN_RIGHT, {}, viewEnabled(ID_OBJECT_ALIGN_RIGHT)),
        leaf("Align &Top", ID_OBJECT_ALIGN_TOP, {}, viewEnabled(ID_OBJECT_ALIGN_TOP)),
        leaf("Align &Middle", ID_OBJECT_ALIGN_MIDDLE, {}, viewEnabled(ID_OBJECT_ALIGN_MIDDLE)),
        leaf("Align Botto&m", ID_OBJECT_ALIGN_BOTTOM, {}, viewEnabled(ID_OBJECT_ALIGN_BOTTOM)),
    };
    std::vector<pf::menu_command> window{
        leaf("&Next Tab", ID_WINDOW_NEXT_TAB, {pf::platform_key::Tab, ctrl}),
        leaf("&Previous Tab", ID_WINDOW_PREV_TAB, {pf::platform_key::Tab, ctrl | shift}),
        leaf("&Close Tab", ID_WINDOW_CLOSE_TAB, {static_cast<unsigned int>('W'), ctrl},
             [this] { return ActiveView() != nullptr; }),
    };
    std::vector<pf::menu_command> help{leaf("&About fast-draw...", ID_APP_ABOUT)};

    frame_->set_menu({
        submenu("&File", std::move(file)),
        submenu("&Edit", std::move(edit)),
        submenu("&View", std::move(view)),
        submenu("&Arrange", std::move(arrange)),
        submenu("&Window", std::move(window)),
        submenu("&Help", std::move(help)),
    });
}

std::optional<Color> MainWindow::PickColour(const Color initial) {
    std::optional<Color> selected;
    std::vector<pf::menu_command> colours;
    int index = 0;
    for (const Color colour : PaletteColours()) {
        pf::menu_command command;
        command.text = "Colour " + std::to_string(index++);
        command.is_checked = [initial, colour] { return colour == initial; };
        command.action = [&selected, colour] { selected = colour; };
        colours.push_back(std::move(command));
    }
    frame_->show_popup_menu(colours, pf::platform_cursor_pos());
    return selected;
}

void MainWindow::ShowContextMenu(const Point point) {
    if (!frame_) return;
    View* view = ActiveView();
    std::vector<pf::menu_command> items;
    auto add = [&](std::string text, const int id) {
        pf::menu_command command;
        command.text = std::move(text);
        command.id = id;
        command.action = [this, id] { OnCommand(id); };
        command.is_enabled = [view, id] {
            bool enabled = false;
            bool checked = false;
            if (view) view->QueryCommandState(id, enabled, checked);
            return enabled;
        };
        items.push_back(std::move(command));
    };
    add("Cut", ID_EDIT_CUT);
    add("Copy", ID_EDIT_COPY);
    add("Delete", ID_EDIT_CLEAR);
    add("Duplicate", ID_EDIT_DUPLICATE);
    add("Edit Label", ID_EDIT_TEXT);
    add("Bring to Front", ID_OBJECT_FRONT);
    add("Send to Back", ID_OBJECT_BACK);
    frame_->show_popup_menu(items, {point.x, point.y});
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
    frame_->message_box(pf::utf16_to_utf8(message), "fast-draw",
                        pf::msg_box_style::ok | pf::msg_box_style::icon_warning);
}

uint32_t MainWindow::handle_message(pf::window_frame_ptr window,
                                     const pf::message_type message,
                                     const pf::message_params& params) {
    switch (message) {
        case pf::message_type::create: {
            frame_ = window;
            frame_->set_text("fast-draw");
            ConfigureControls();
            InstallMenu();
            if (views_.empty()) {
                if (startupPaths_.empty()) {
                    NewDocument();
                } else {
                    const auto paths = std::move(startupPaths_);
                    for (const auto& path : paths) {
                        View* view = NewDocument();
                        const std::filesystem::path file = NativePath(path);
                        const Document::ErrorHandler report =
                            [this](std::wstring_view action, const std::filesystem::path& failed,
                                   std::string_view detail) {
                                ReportDocumentError(action, failed, detail);
                            };
                        view->Doc().Load(file, report);
                    }
                }
            }
            Layout({static_cast<int>(window->get_client_rect().width()),
                    static_cast<int>(window->get_client_rect().height())});
            return 0;
        }
        case pf::message_type::erase_background: return 1;
        case pf::message_type::close:
            if (OnClose()) window->close();
            return 0;
        case pf::message_type::timer:
            if (View* view = ActiveView()) view->OnTimer(params.timer_id);
            return 0;
        case pf::message_type::dpi_changed:
            dpiScale_ = params.dpi_scale;
            Layout(extent_);
            return 0;
        case pf::message_type::destroy:
            captured_ = false;
            return 0;
        default: return 0;
    }
}

void MainWindow::handle_size(pf::window_frame_ptr&, const pf::isize extent,
                             pf::measure_context&) {
    Layout(extent);
}

uint32_t MainWindow::handle_mouse(pf::window_frame_ptr,
                                   const pf::mouse_message_type message,
                                   const pf::mouse_params& params) {
    const Point point{params.point.x, params.point.y};
    const auto route = captured_ ? captureTarget_
                                 : Contains(viewBounds_, point) ? CaptureTarget::view
                                 : Contains(tabBounds_, point) ? CaptureTarget::tabs
                                 : Contains(paletteBounds_, point) ? CaptureTarget::palette
                                 : Contains(styleBounds_, point) ? CaptureTarget::style
                                                                    : CaptureTarget::none;

    auto view = ActiveView();
    const bool alt = frame_->is_key_down_async(pf::platform_key::Alt);

    switch (message) {
        case pf::mouse_message_type::left_button_down:
            if (route == CaptureTarget::view && view)
                view->OnLeftDown(LocalPoint(viewBounds_, point), params.shift, params.control, alt);
            else if (route == CaptureTarget::tabs)
                tabBar_.OnLeftDown(LocalPoint(tabBounds_, point));
            else if (route == CaptureTarget::palette)
                palette_.OnLeftDown(LocalPoint(paletteBounds_, point));
            else if (route == CaptureTarget::style)
                stylePanel_.OnLeftDown(LocalPoint(styleBounds_, point));
            return 0;
        case pf::mouse_message_type::left_button_up:
            if (captureTarget_ == CaptureTarget::view && view)
                view->OnLeftUp(LocalPoint(viewBounds_, point), params.shift, params.control, alt);
            else if (captureTarget_ == CaptureTarget::tabs)
                tabBar_.OnLeftUp(LocalPoint(tabBounds_, point));
            else if (captureTarget_ == CaptureTarget::palette)
                palette_.OnLeftUp(LocalPoint(paletteBounds_, point));
            else if (captureTarget_ == CaptureTarget::style)
                stylePanel_.OnLeftUp(LocalPoint(styleBounds_, point));
            return 0;
        case pf::mouse_message_type::left_button_dbl_clk:
            if (route == CaptureTarget::view && view)
                view->OnDoubleClick(LocalPoint(viewBounds_, point), params.shift, params.control, alt);
            return 0;
        case pf::mouse_message_type::mouse_move:
            if (route == CaptureTarget::view && view)
                view->OnMouseMove(LocalPoint(viewBounds_, point), params.shift, params.control, alt);
            else if (route == CaptureTarget::tabs)
                tabBar_.OnMouseMove(LocalPoint(tabBounds_, point));
            else if (route == CaptureTarget::palette)
                palette_.OnMouseMove(LocalPoint(paletteBounds_, point));
            else if (route == CaptureTarget::style)
                stylePanel_.OnMouseMove(LocalPoint(styleBounds_, point));
            return 0;
        case pf::mouse_message_type::mouse_leave:
            tabBar_.OnMouseLeave();
            palette_.OnMouseLeave();
            stylePanel_.OnMouseLeave();
            return 0;
        case pf::mouse_message_type::mouse_wheel:
            if (route == CaptureTarget::view && view)
                view->OnMouseWheel(LocalPoint(viewBounds_, point), params.wheel_delta,
                                   params.control, params.shift);
            return 0;
        case pf::mouse_message_type::middle_button_up:
            if (route == CaptureTarget::tabs)
                tabBar_.OnMiddleUp(LocalPoint(tabBounds_, point));
            return 0;
        case pf::mouse_message_type::right_button_down:
            if (route == CaptureTarget::view && view)
                view->OnContextMenu(LocalPoint(viewBounds_, point));
            return 0;
        default: return 0;
    }
}

uint32_t MainWindow::handle_keyboard(pf::window_frame_ptr,
                                      const pf::keyboard_message_type message,
                                      const pf::keyboard_params& params) {
    View* view = ActiveView();
    if (!view) return 0;
    if (message == pf::keyboard_message_type::key_down) {
        const bool repeatable = params.vk == pf::platform_key::Left ||
                                params.vk == pf::platform_key::Right ||
                                params.vk == pf::platform_key::Up ||
                                params.vk == pf::platform_key::Down;
        if (!params.repeat || repeatable)
            view->OnKeyDown(params.vk,
                            frame_->is_key_down_async(pf::platform_key::Shift));
    } else if (message == pf::keyboard_message_type::char_input) {
        view->OnChar(params.ch);
    }
    return 0;
}

void MainWindow::handle_paint(pf::window_frame_ptr&, pf::draw_context& draw) {
    if (dirty_ || !surface_.IsValid()) {
        Render();
        dirty_ = false;
    }
    if (!surface_.IsValid()) return;

    pf::bitmap bitmap(surface_.Width(), surface_.Height(),
                      std::vector<std::uint32_t>(surface_.Pixels(),
                                                 surface_.Pixels() +
                                                     static_cast<std::ptrdiff_t>(
                                                         surface_.Width()) * surface_.Height()));
    draw.draw_bitmap(0, 0, bitmap);
}
