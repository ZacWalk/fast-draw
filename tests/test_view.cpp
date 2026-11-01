#include "test_framework.h"

#include "view.h"

namespace {

class HostedView {
public:
    HostedView() : view_(std::make_unique<Document>()) {
        view_.SetViewport({800, 600}, 1.0);
    }

    View* operator->() { return &view_; }
    const View& get() const { return view_; }

private:
    View view_;
};

const DrawObj* Newest(const View& view) {
    const auto& objects = view.Doc().Objects();
    return objects.empty() ? nullptr : &objects.back();
}

}  // namespace

TEST(RestylingASelectionOnlyChangesThatFieldInTheDefaults) {
    HostedView view;

    const Style defaults = view->EffectiveStyle();
    CHECK(defaults.hasFill);

    view->CreateAt(ShapeKind::Line, Point{200, 200});
    CHECK_EQ(view->Selection().size(), std::size_t(1));
    CHECK(!view->EffectiveStyle().hasFill);

    Style wider = view->EffectiveStyle();
    wider.lineWidth = defaults.lineWidth + 3;
    view->ApplyStyle(wider);

    const DrawObj* line = Newest(view.get());
    CHECK(line && line->style.lineWidth == defaults.lineWidth + 3);
    CHECK(line && !line->style.hasFill);

    view->ClearSelection();
    CHECK(view->EffectiveStyle().hasFill);
    CHECK_EQ(view->EffectiveStyle().lineWidth, defaults.lineWidth + 3);

    view->CreateAt(ShapeKind::Rectangle, Point{400, 300});
    const DrawObj* box = Newest(view.get());
    CHECK(box && box->kind == ShapeKind::Rectangle);
    CHECK(box && box->style.hasFill);
    CHECK(box && box->style.hasLine);
    CHECK(box && box->style.lineWidth == defaults.lineWidth + 3);
}

TEST(RestylingWithNothingSelectedSetsTheDefaults) {
    HostedView view;

    Style bold = view->EffectiveStyle();
    bold.bold = true;
    bold.hasLine = false;
    view->ApplyStyle(bold);

    CHECK(view->EffectiveStyle() == bold);
    CHECK(view->Doc().Objects().empty());
}
