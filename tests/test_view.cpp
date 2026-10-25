#include "test_framework.h"

#include "view.h"

#include <windows.h>

namespace {

// A drawing view in a hidden parent window, as the frame hosts it.
class HostedView {
public:
    HostedView() {
        HINSTANCE instance = ::GetModuleHandleW(nullptr);
        View::RegisterWindowClass(instance);
        parent_ = ::CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 800, 600, nullptr,
                                    nullptr, instance, nullptr);
        HWND handle = View::Create(parent_);
        ::MoveWindow(handle, 0, 0, 800, 600, FALSE);
        view_ = View::FromHandle(handle);
    }

    ~HostedView() {
        if (parent_) ::DestroyWindow(parent_);
    }

    HostedView(const HostedView&) = delete;
    HostedView& operator=(const HostedView&) = delete;

    View* operator->() const { return view_; }
    const View& get() const { return *view_; }
    bool Valid() const { return view_ != nullptr; }

private:
    HWND parent_ = nullptr;
    View* view_ = nullptr;
};

const DrawObj* Newest(const View& view) {
    const auto& objects = view.Doc().Objects();
    return objects.empty() ? nullptr : &objects.back();
}

}  // namespace

TEST(RestylingASelectionOnlyChangesThatFieldInTheDefaults) {
    HostedView view;
    CHECK(view.Valid());
    if (!view.Valid()) return;

    const Style defaults = view->EffectiveStyle();
    CHECK(defaults.hasFill);

    // A line is created without a fill and is left selected.
    view->CreateAt(ShapeKind::Line, POINT{200, 200});
    CHECK_EQ(view->Selection().size(), std::size_t(1));
    CHECK(!view->EffectiveStyle().hasFill);

    // Thicken it from the sidebar.
    Style wider = view->EffectiveStyle();
    wider.lineWidth = defaults.lineWidth + 3;
    view->ApplyStyle(wider);

    const DrawObj* line = Newest(view.get());
    CHECK(line && line->style.lineWidth == defaults.lineWidth + 3);
    CHECK(line && !line->style.hasFill);

    // The next shape picks up the new width but not the line's missing fill.
    view->ClearSelection();
    CHECK(view->EffectiveStyle().hasFill);
    CHECK_EQ(view->EffectiveStyle().lineWidth, defaults.lineWidth + 3);

    view->CreateAt(ShapeKind::Rectangle, POINT{400, 300});
    const DrawObj* box = Newest(view.get());
    CHECK(box && box->kind == ShapeKind::Rectangle);
    CHECK(box && box->style.hasFill);
    CHECK(box && box->style.hasLine);
    CHECK(box && box->style.lineWidth == defaults.lineWidth + 3);
}

TEST(RestylingWithNothingSelectedSetsTheDefaults) {
    HostedView view;
    CHECK(view.Valid());
    if (!view.Valid()) return;

    Style bold = view->EffectiveStyle();
    bold.bold = true;
    bold.hasLine = false;
    view->ApplyStyle(bold);

    CHECK(view->EffectiveStyle() == bold);
    CHECK(view->Doc().Objects().empty());
}
