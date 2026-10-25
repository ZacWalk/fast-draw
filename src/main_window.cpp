#include "main_window.h"

#include "document.h"
#include "resource.h"
#include "shape_palette.h"
#include "style_panel.h"
#include "tab_bar.h"
#include "ui.h"
#include "view.h"

#include <commdlg.h>

#include <algorithm>
#include <array>
#include <string>

namespace {

constexpr wchar_t kFrameClassName[] = L"FastDrawFrame";

constexpr wchar_t kFileFilter[] = L"fast-draw drawings (*.fdr)\0*.fdr\0All files (*.*)\0*.*\0";
constexpr wchar_t kPngFilter[] = L"PNG images (*.png)\0*.png\0All files (*.*)\0*.*\0";

constexpr int kSidebarWidth = 236;

std::wstring RunFileDialog(HWND owner, bool save, const wchar_t* filter,
                           const wchar_t* defaultExtension) {
    std::array<wchar_t, MAX_PATH> path{};

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = path.data();
    ofn.nMaxFile = static_cast<DWORD>(path.size());
    ofn.lpstrDefExt = defaultExtension;
    ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR |
                (save ? OFN_OVERWRITEPROMPT : (OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST));

    const BOOL ok = save ? ::GetSaveFileNameW(&ofn) : ::GetOpenFileNameW(&ofn);
    return ok ? std::wstring(path.data()) : std::wstring();
}

void ExportPng(View& view) {
    const std::wstring path = RunFileDialog(view.Handle(), true, kPngFilter, L"png");
    if (path.empty()) return;

    if (!view.ExportPng(path))
        ::MessageBoxW(view.Handle(), L"Could not write the image.", L"fast-draw",
                      MB_OK | MB_ICONERROR);
}

void PrintDocument(View& view) {
    PRINTDLGW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = view.Handle();
    dialog.Flags = PD_RETURNDC | PD_NOSELECTION | PD_NOPAGENUMS;
    dialog.nCopies = 1;

    if (!::PrintDlgW(&dialog) || !dialog.hDC) {
        if (dialog.hDevMode) ::GlobalFree(dialog.hDevMode);
        if (dialog.hDevNames) ::GlobalFree(dialog.hDevNames);
        return;
    }

    DOCINFOW info{};
    info.cbSize = sizeof(info);
    const std::wstring title = view.Doc().Title();
    info.lpszDocName = title.c_str();

    if (::StartDocW(dialog.hDC, &info) > 0) {
        if (::StartPage(dialog.hDC) > 0) {
            view.Print(dialog.hDC);
            ::EndPage(dialog.hDC);
        }
        ::EndDoc(dialog.hDC);
    }

    ::DeleteDC(dialog.hDC);
    if (dialog.hDevMode) ::GlobalFree(dialog.hDevMode);
    if (dialog.hDevNames) ::GlobalFree(dialog.hDevNames);
}

void ShowAbout(HWND owner) {
    ::MessageBoxW(owner,
                  L"fast-draw 2.0\n\n"
                  L"A small vector drawing program with its own renderer.\n\n"
                  L"Originally written as an MFC application by Tao Zhu and Jian Zhu, "
                  L"with drawing code derived from the MFC DrawCli sample.",
                  L"About fast-draw", MB_OK | MB_ICONINFORMATION);
}

}  // namespace

bool SaveDocument(View& view, bool forceSaveAs) {
    view.FinishEditing();
    Document& document = view.Doc();

    std::wstring path = document.HasPath() ? document.Path().wstring() : std::wstring();
    if (forceSaveAs || path.empty()) {
        path = RunFileDialog(view.Handle(), true, kFileFilter, L"fdr");
        if (path.empty()) return false;
    }

    return document.Save(view.Handle(), path);
}

bool PromptToSave(View& view) {
    view.FinishEditing();
    if (!view.Doc().IsModified()) return true;

    const std::wstring message = L"Save changes to " + view.Doc().Title() + L"?";
    const int answer = ::MessageBoxW(view.Handle(), message.c_str(), L"fast-draw",
                                     MB_YESNOCANCEL | MB_ICONWARNING);

    if (answer == IDCANCEL) return false;
    if (answer == IDNO) return true;
    return SaveDocument(view, false);
}

// -------------------------------------------------------------- MainWindow

MainWindow* MainWindow::FromHandle(HWND window) {
    if (!window) return nullptr;
    return reinterpret_cast<MainWindow*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
}

MainWindow::~MainWindow() {
    // Child windows are torn down after the frame's WM_DESTROY, so make sure
    // none of them can still call back into a deleted frame.
    for (HWND handle : views_) {
        if (View* view = View::FromHandle(handle)) view->onChanged = nullptr;
    }
    if (TabBar* tabs = TabBar::FromHandle(tabBar_)) {
        tabs->onSelect = nullptr;
        tabs->onClose = nullptr;
        tabs->onNewTab = nullptr;
    }
    if (StylePanel* panel = StylePanel::FromHandle(stylePanel_)) panel->onChange = nullptr;
    if (ShapePalette* palette = ShapePalette::FromHandle(palette_)) {
        palette->onSelect = nullptr;
        palette->onDrop = nullptr;
    }
}

void MainWindow::RegisterWindowClass(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &MainWindow::WndProc;
    wc.hInstance = instance;
    wc.hIcon = ::LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP));
    wc.hIconSm = wc.hIcon;
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = ::CreateSolidBrush(ui::kChrome);
    wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MAIN_MENU);
    wc.lpszClassName = kFrameClassName;
    ::RegisterClassExW(&wc);
}

HWND MainWindow::Create(HINSTANCE instance, int showCommand) {
    HWND window = ::CreateWindowExW(0, kFrameClassName, L"fast-draw",
                                    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT,
                                    CW_USEDEFAULT, 1240, 820, nullptr, nullptr, instance,
                                    nullptr);
    if (!window) return nullptr;

    ::ShowWindow(window, showCommand);
    ::UpdateWindow(window);
    return window;
}

bool MainWindow::OnCreate() {
    tabBar_ = TabBar::Create(hwnd_, IDC_TABBAR);
    stylePanel_ = StylePanel::Create(hwnd_, IDC_STYLEPANEL);
    palette_ = ShapePalette::Create(hwnd_, IDC_SHAPEPALETTE);
    if (!tabBar_ || !stylePanel_ || !palette_) return false;

    if (TabBar* tabs = TabBar::FromHandle(tabBar_)) {
        tabs->onSelect = [this](int index) { ActivateTab(index); };
        tabs->onClose = [this](int index) { CloseTab(index); };
        tabs->onNewTab = [this] { NewDocument(); };
    }

    if (StylePanel* panel = StylePanel::FromHandle(stylePanel_)) {
        panel->onChange = [this](const Style& style) {
            if (View* view = ActiveView()) view->ApplyStyle(style);
        };
    }

    if (ShapePalette* palette = ShapePalette::FromHandle(palette_)) {
        palette->onSelect = [this](Tool tool) {
            if (View* view = ActiveView()) view->SetTool(tool);
            if (ShapePalette* self = ShapePalette::FromHandle(palette_)) self->SetActive(tool);
        };
        palette->onDrop = [this](ShapeKind kind, POINT screenPoint) {
            View* view = ActiveView();
            if (!view) return;

            POINT client = screenPoint;
            ::ScreenToClient(view->Handle(), &client);

            RECT bounds{};
            ::GetClientRect(view->Handle(), &bounds);
            if (!::PtInRect(&bounds, client)) return;

            view->CreateAt(kind, client);
        };
    }

    return true;
}

void MainWindow::Layout() {
    RECT client{};
    ::GetClientRect(hwnd_, &client);

    const int sidebar = sidebarVisible_ ? ui::Scale(hwnd_, kSidebarWidth) : 0;

    ::ShowWindow(stylePanel_, sidebarVisible_ ? SW_SHOW : SW_HIDE);
    ::ShowWindow(palette_, sidebarVisible_ ? SW_SHOW : SW_HIDE);

    if (sidebarVisible_) {
        const StylePanel* panel = StylePanel::FromHandle(stylePanel_);
        // The shape grid keeps its space; the style panel gives way first.
        const int paletteFloor = ui::Scale(hwnd_, 230);
        const int panelHeight = (std::clamp)(panel ? panel->PreferredHeight() : 0, 0,
                                             (std::max)(0, static_cast<int>(client.bottom) -
                                                               paletteFloor));
        ::MoveWindow(stylePanel_, 0, 0, sidebar, panelHeight, TRUE);
        ::MoveWindow(palette_, 0, panelHeight, sidebar,
                     (std::max)(0, static_cast<int>(client.bottom) - panelHeight), TRUE);
    }

    const TabBar* tabs = TabBar::FromHandle(tabBar_);
    const int tabHeight = tabs ? tabs->PreferredHeight() : 0;
    const int contentWidth = (std::max)(0, static_cast<int>(client.right) - sidebar);

    ::MoveWindow(tabBar_, sidebar, 0, contentWidth, tabHeight, TRUE);

    const int viewHeight = (std::max)(0, static_cast<int>(client.bottom) - tabHeight);
    for (HWND view : views_) ::MoveWindow(view, sidebar, tabHeight, contentWidth, viewHeight, TRUE);
}

View* MainWindow::ActiveView() const {
    if (activeIndex_ < 0 || activeIndex_ >= static_cast<int>(views_.size())) return nullptr;
    return View::FromHandle(views_[static_cast<std::size_t>(activeIndex_)]);
}

void MainWindow::RefreshChrome() {
    View* view = ActiveView();

    if (TabBar* tabs = TabBar::FromHandle(tabBar_)) {
        std::vector<std::wstring> titles;
        titles.reserve(views_.size());
        for (HWND handle : views_) {
            View* each = View::FromHandle(handle);
            std::wstring title = each ? each->Doc().Title() : L"Untitled";
            if (each && each->Doc().IsModified()) title += L" \u2022";
            titles.push_back(std::move(title));
        }
        tabs->SetTabs(std::move(titles), activeIndex_);
    }

    if (StylePanel* panel = StylePanel::FromHandle(stylePanel_)) {
        if (view)
            panel->SetStyle(view->EffectiveStyle(), !view->Selection().empty());
    }

    if (ShapePalette* palette = ShapePalette::FromHandle(palette_)) {
        if (view) palette->SetActive(view->CurrentTool());
    }

    std::wstring title = L"fast-draw";
    if (view) title = view->Doc().Title() + L" - " + title;

    wchar_t current[256]{};
    ::GetWindowTextW(hwnd_, current, static_cast<int>(std::size(current)));
    if (title != current) ::SetWindowTextW(hwnd_, title.c_str());
}

void MainWindow::ActivateTab(int index) {
    if (index < 0 || index >= static_cast<int>(views_.size())) return;

    for (std::size_t i = 0; i < views_.size(); ++i)
        ::ShowWindow(views_[i], i == static_cast<std::size_t>(index) ? SW_SHOW : SW_HIDE);

    activeIndex_ = index;
    ::SetFocus(views_[static_cast<std::size_t>(index)]);

    Layout();
    RefreshChrome();
}

void MainWindow::CloseTab(int index) {
    if (index < 0 || index >= static_cast<int>(views_.size())) return;

    HWND handle = views_[static_cast<std::size_t>(index)];
    if (View* view = View::FromHandle(handle)) {
        if (!PromptToSave(*view)) return;
    }

    const int active = activeIndex_;
    views_.erase(views_.begin() + index);
    ::DestroyWindow(handle);

    // An empty frame is a dead end with nothing to draw on, so keep one
    // document open at all times.
    if (views_.empty()) {
        activeIndex_ = -1;
        NewDocument();
        return;
    }

    // Closing a background tab leaves the document in front where it is.
    if (index != active && active >= 0) {
        ActivateTab(active > index ? active - 1 : active);
        return;
    }

    ActivateTab((std::min)(index, static_cast<int>(views_.size()) - 1));
}

void MainWindow::StepTab(int delta) {
    if (views_.size() < 2) return;
    const int count = static_cast<int>(views_.size());
    ActivateTab(((activeIndex_ + delta) % count + count) % count);
}

HWND MainWindow::NewDocument() {
    HWND handle = View::Create(hwnd_);
    if (!handle) return nullptr;

    if (View* view = View::FromHandle(handle))
        view->onChanged = [this] { RefreshChrome(); };

    views_.push_back(handle);
    ActivateTab(static_cast<int>(views_.size()) - 1);
    return handle;
}

void MainWindow::OpenDocument() {
    const std::wstring path = RunFileDialog(hwnd_, false, kFileFilter, L"fdr");
    if (path.empty()) return;

    const int previous = activeIndex_;
    HWND handle = NewDocument();
    View* view = View::FromHandle(handle);
    if (!view) return;

    if (view->Doc().Load(hwnd_, path)) {
        RefreshChrome();
        return;
    }

    // Drop the tab the failed file would have filled and go back to where the
    // user was.
    CloseTab(activeIndex_);
    if (previous >= 0 && previous < static_cast<int>(views_.size())) ActivateTab(previous);
}

void MainWindow::OnCommand(int id) {
    View* view = ActiveView();

    switch (id) {
        case ID_FILE_NEW: NewDocument(); return;
        case ID_FILE_OPEN: OpenDocument(); return;
        case ID_FILE_CLOSE: CloseTab(activeIndex_); return;

        case ID_FILE_SAVE:
            if (view && SaveDocument(*view, false)) RefreshChrome();
            return;

        case ID_FILE_SAVE_AS:
            if (view && SaveDocument(*view, true)) RefreshChrome();
            return;

        case ID_FILE_PRINT:
            if (view) PrintDocument(*view);
            return;

        case ID_FILE_EXPORT_PNG:
            if (view) ExportPng(*view);
            return;

        case ID_APP_EXIT: ::SendMessageW(hwnd_, WM_CLOSE, 0, 0); return;
        case ID_APP_ABOUT: ShowAbout(hwnd_); return;

        case ID_WINDOW_NEXT_TAB: StepTab(1); return;
        case ID_WINDOW_PREV_TAB: StepTab(-1); return;
        case ID_WINDOW_CLOSE_TAB: CloseTab(activeIndex_); return;

        case ID_VIEW_SIDEBAR:
            sidebarVisible_ = !sidebarVisible_;
            Layout();
            return;

        default: break;
    }

    if (view && view->OnCommand(id)) RefreshChrome();
}

void MainWindow::OnInitMenuPopup(HMENU menu) {
    View* view = ActiveView();

    static const int kViewCommands[] = {
        ID_EDIT_UNDO,          ID_EDIT_REDO,          ID_EDIT_CUT,
        ID_EDIT_COPY,          ID_EDIT_PASTE,         ID_EDIT_CLEAR,
        ID_EDIT_DUPLICATE,     ID_EDIT_SELECT_ALL,    ID_EDIT_TEXT,
        ID_VIEW_GRID,          ID_VIEW_SNAP,          ID_VIEW_PAPER_COLOUR,
        ID_OBJECT_FRONT,       ID_OBJECT_BACK,        ID_OBJECT_FORWARD,
        ID_OBJECT_BACKWARD,    ID_OBJECT_ALIGN_LEFT,  ID_OBJECT_ALIGN_CENTRE,
        ID_OBJECT_ALIGN_RIGHT, ID_OBJECT_ALIGN_TOP,   ID_OBJECT_ALIGN_MIDDLE,
        ID_OBJECT_ALIGN_BOTTOM, ID_PAGE_LETTER_LANDSCAPE, ID_PAGE_LETTER_PORTRAIT,
        ID_PAGE_A4_LANDSCAPE,  ID_PAGE_A4_PORTRAIT,   ID_PAGE_A3_LANDSCAPE,
    };

    for (int id : kViewCommands) {
        if (::GetMenuState(menu, static_cast<UINT>(id), MF_BYCOMMAND) == static_cast<UINT>(-1))
            continue;

        bool enabled = false;
        bool checked = false;
        if (view) view->QueryCommandState(id, enabled, checked);

        ::EnableMenuItem(menu, static_cast<UINT>(id),
                         MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED));
        ::CheckMenuItem(menu, static_cast<UINT>(id),
                        MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED));
    }

    static const int kDocumentCommands[] = {ID_FILE_CLOSE, ID_FILE_SAVE, ID_FILE_SAVE_AS,
                                            ID_FILE_PRINT, ID_FILE_EXPORT_PNG};
    for (int id : kDocumentCommands) {
        if (::GetMenuState(menu, static_cast<UINT>(id), MF_BYCOMMAND) == static_cast<UINT>(-1))
            continue;
        ::EnableMenuItem(menu, static_cast<UINT>(id),
                         MF_BYCOMMAND | (view ? MF_ENABLED : MF_GRAYED));
    }

    if (::GetMenuState(menu, ID_VIEW_SIDEBAR, MF_BYCOMMAND) != static_cast<UINT>(-1))
        ::CheckMenuItem(menu, ID_VIEW_SIDEBAR,
                        MF_BYCOMMAND | (sidebarVisible_ ? MF_CHECKED : MF_UNCHECKED));
}

void MainWindow::OnIdle() { RefreshChrome(); }

bool MainWindow::OnClose() {
    // Nothing is marked clean here: if a later document cancels the close, every
    // earlier one must still hold its unsaved changes.
    for (HWND handle : views_) {
        View* view = View::FromHandle(handle);
        if (!view) continue;
        if (!PromptToSave(*view)) return false;
    }
    return true;
}

LRESULT CALLBACK MainWindow::WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = FromHandle(window);

    switch (message) {
        case WM_CREATE: {
            self = new MainWindow(window);
            ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            if (!self->OnCreate()) return -1;
            return 0;
        }

        case WM_SIZE:
            if (self) {
                self->Layout();
                return 0;
            }
            break;

        case WM_DPICHANGED:
            if (self) {
                const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
                ::SetWindowPos(window, nullptr, suggested->left, suggested->top,
                               suggested->right - suggested->left,
                               suggested->bottom - suggested->top,
                               SWP_NOZORDER | SWP_NOACTIVATE);
                self->Layout();
                return 0;
            }
            break;

        case WM_SETFOCUS:
            if (self) {
                if (View* view = self->ActiveView()) ::SetFocus(view->Handle());
                return 0;
            }
            break;

        case WM_COMMAND:
            if (self) {
                self->OnCommand(LOWORD(wParam));
                return 0;
            }
            break;

        case WM_INITMENUPOPUP:
            if (self && !HIWORD(lParam)) self->OnInitMenuPopup(reinterpret_cast<HMENU>(wParam));
            break;

        case WM_CLOSE:
            if (self && !self->OnClose()) return 0;
            break;

        case WM_DESTROY:
            ::SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            delete self;
            ::PostQuitMessage(0);
            return 0;

        default: break;
    }

    return ::DefWindowProcW(window, message, wParam, lParam);
}
