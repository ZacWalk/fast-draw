#include "view.h"

#include "file_io.h"
#include "png_export.h"
#include "resource.h"
#include "utf8.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {

// Gap between the workspace edge and the paper, in device pixels at 96 dpi.
constexpr int kPageMargin = 28;

// Size of an object created by a click rather than a drag, in document units.
// Both are even multiples of the grid so a click lands on grid lines.
constexpr long kDefaultWidth = 150;
constexpr long kDefaultHeight = 100;

constexpr long kMinorGrid = 25;   // 0.25"
constexpr long kMajorGrid = 100;  // 1"

constexpr float kMinZoom = 0.1f;
constexpr float kMaxZoom = 8.0f;
constexpr float kZoomStep = 1.25f;

constexpr int kExportDpi = 150;

constexpr unsigned int kCaretTimer = 1;

// Page sizes in document units, indexed by ID_PAGE_FIRST..ID_PAGE_LAST.
constexpr Size kPageSizes[] = {
    {1100, 850},   // Letter landscape
    {850, 1100},   // Letter portrait
    {1169, 827},   // A4 landscape
    {827, 1169},   // A4 portrait
    {1654, 1169},  // A3 landscape
};

static_assert(std::size(kPageSizes) == ID_PAGE_LAST - ID_PAGE_FIRST + 1);

Rect CentredRect(Point centre, long width, long height) {
    return Rect(centre.x - width / 2, centre.y + height / 2, centre.x + width / 2,
                centre.y - height / 2);
}

long RoundTo(long value, long step) {
    const long half = value >= 0 ? step / 2 : -step / 2;
    return (value + half) / step * step;
}

// Axis-aligned extents of an object in document space, y increasing upwards.
struct Extent {
    long minX, maxX, minY, maxY;
};

Extent ExtentOf(const Rect& rect) {
    return Extent{(std::min)(rect.left, rect.right), (std::max)(rect.left, rect.right),
                  (std::min)(rect.top, rect.bottom), (std::max)(rect.top, rect.bottom)};
}

}  // namespace

bool RenderDocumentToCanvas(const Document& document, Canvas& canvas, int dpi) {
    const Size page = document.PageSize();
    if (!canvas.Resize(static_cast<int>((static_cast<long long>(page.cx) * dpi) / 100),
                      static_cast<int>((static_cast<long long>(page.cy) * dpi) / 100)))
        return false;

    Renderer renderer(canvas);
    renderer.Clear(ArgbFromColor(document.PaperColour()));

    Transform transform;
    transform.scale = static_cast<float>(dpi) / 100.0f;
    transform.offsetX = page.cx * 0.5f * transform.scale;
    transform.offsetY = page.cy * 0.5f * transform.scale;

    document.Draw(renderer, canvas, transform);
    canvas.SetOpaque();
    return true;
}

// ------------------------------------------------------ creation / layout

View::View(std::unique_ptr<Document> document, pf::window_frame_ptr frame)
    : frame_(std::move(frame)), document_(std::move(document)) {}

View::~View() = default;

void View::SetViewport(const Size extent, const double dpiScale) {
    viewport_ = extent;
    dpiScale_ = dpiScale;
    UpdateScrollBars();
}

// ------------------------------------------------------------- coordinates

Transform View::GetTransform() const {
    const Size page = document_->PageSize();
    const float margin = ui::ScaleF(dpiScale_, static_cast<float>(kPageMargin));

    Transform transform;
    transform.scale = static_cast<float>(dpiScale_ * 96.0 / 100.0) * zoom_;
    transform.offsetX = margin - scroll_.x + page.cx * 0.5f * transform.scale;
    transform.offsetY = margin - scroll_.y + page.cy * 0.5f * transform.scale;
    return transform;
}

Point View::ToDoc(const Point clientPoint) const {
    return GetTransform().ToDoc(
        Vec2{static_cast<float>(clientPoint.x), static_cast<float>(clientPoint.y)});
}

long View::HitTolerance() const {
    return (std::max)(3L, static_cast<long>(ui::ScaleF(dpiScale_, 5.0f) / GetTransform().scale));
}

Point View::Snap(Point docPoint) const {
    if (!snap_ || altHeld_) return docPoint;
    return Point(RoundTo(docPoint.x, kMinorGrid), RoundTo(docPoint.y, kMinorGrid));
}

int View::HandleAt(const DrawObj& object, Point docPoint) const {
    const long reach = static_cast<long>(ui::ScaleF(dpiScale_, 6.0f) / GetTransform().scale);
    for (int i = 0; i < object.HandleCount(); ++i) {
        const Point handle = object.Handle(i);
        if (std::labs(handle.x - docPoint.x) <= reach && std::labs(handle.y - docPoint.y) <= reach)
            return i;
    }
    return -1;
}

// --------------------------------------------------------------- selection

DrawObj* View::Find(ObjectId id) const { return document_->Find(id); }

DrawObj* View::Primary() const { return selection_.empty() ? nullptr : Find(selection_.front()); }

bool View::IsSelected(ObjectId id) const {
    return std::find(selection_.begin(), selection_.end(), id) != selection_.end();
}

void View::AddToSelection(ObjectId id) {
    if (id != ObjectId::None && !IsSelected(id)) selection_.push_back(id);
}

void View::Select(ObjectId id, bool add) {
    if (!add) selection_.clear();
    AddToSelection(id);
    Invalidate();
    Changed();
}

void View::RemoveFromSelection(ObjectId id) {
    std::erase(selection_, id);
}

void View::SelectAll() {
    selection_.clear();
    for (const DrawObj& object : document_->Objects()) selection_.push_back(object.id);
    Invalidate();
    Changed();
}

void View::ClearSelection() {
    if (selection_.empty()) return;
    Select(ObjectId::None, false);
}

void View::DeleteSelection() {
    if (selection_.empty()) return;
    EndEditing();

    document_->BeginChange();
    const std::vector<ObjectId> doomed = selection_;
    selection_.clear();
    for (const ObjectId id : doomed) document_->Remove(id);
    document_->CommitChange();

    Invalidate();
    Changed();
}

void View::DuplicateSelection() {
    if (selection_.empty()) return;

    document_->BeginChange();
    std::vector<ObjectId> copies;
    copies.reserve(selection_.size());
    for (const ObjectId id : selection_) {
        const DrawObj* source = Find(id);
        if (!source) continue;

        DrawObj copy = *source;
        copy.Offset(Size(kMinorGrid, -kMinorGrid));
        copies.push_back(document_->Add(std::move(copy)));
    }
    document_->CommitChange();

    selection_ = std::move(copies);
    Invalidate();
    Changed();
}

void View::NudgeSelection(long dx, long dy) {
    if (selection_.empty()) return;

    document_->BeginChange();
    for (const ObjectId id : selection_) {
        if (DrawObj* object = Find(id)) object->Offset(Size(dx, dy));
    }
    document_->CommitChange();

    Invalidate();
    Changed();
}

void View::AlignSelection(int command) {
    if (selection_.size() < 2) return;

    bool any = false;
    Extent group{};
    for (const ObjectId id : selection_) {
        const DrawObj* object = Find(id);
        if (!object) continue;

        const Extent each = ExtentOf(object->position);
        if (!any) {
            group = each;
            any = true;
            continue;
        }
        group.minX = (std::min)(group.minX, each.minX);
        group.maxX = (std::max)(group.maxX, each.maxX);
        group.minY = (std::min)(group.minY, each.minY);
        group.maxY = (std::max)(group.maxY, each.maxY);
    }
    if (!any) return;

    document_->BeginChange();
    for (const ObjectId id : selection_) {
        DrawObj* object = Find(id);
        if (!object) continue;

        const Extent each = ExtentOf(object->position);
        long dx = 0;
        long dy = 0;

        switch (command) {
            case ID_OBJECT_ALIGN_LEFT: dx = group.minX - each.minX; break;
            case ID_OBJECT_ALIGN_RIGHT: dx = group.maxX - each.maxX; break;
            case ID_OBJECT_ALIGN_CENTRE:
                dx = (group.minX + group.maxX) / 2 - (each.minX + each.maxX) / 2;
                break;
            case ID_OBJECT_ALIGN_TOP: dy = group.maxY - each.maxY; break;
            case ID_OBJECT_ALIGN_BOTTOM: dy = group.minY - each.minY; break;
            case ID_OBJECT_ALIGN_MIDDLE:
                dy = (group.minY + group.maxY) / 2 - (each.minY + each.maxY) / 2;
                break;
            default: break;
        }

        object->Offset(Size(dx, dy));
    }
    document_->CommitChange();

    Invalidate();
    Changed();
}

Style View::EffectiveStyle() const {
    if (const DrawObj* object = Primary()) return object->style;
    return style_;
}

void View::ApplyStyle(const Style& style) {
    // Only the fields the sidebar actually changed are pushed onto the
    // selection, so a mixed selection keeps the rest of its own styling. The
    // defaults for new objects take the same change and nothing else: the
    // panel is showing a selected object's style, and copying all of it would
    // hand, say, a text label's missing fill and outline to the next shape.
    const Style previous = EffectiveStyle();
    ApplyStyleDelta(previous, style, style_);

    if (!selection_.empty()) {
        document_->BeginChange();
        for (const ObjectId id : selection_) {
            if (DrawObj* object = Find(id)) ApplyStyleDelta(previous, style, object->style);
        }
        document_->CommitChange();
    }

    Invalidate();
    Changed();
}

void View::SetTool(Tool tool) {
    if (tool_ == tool) return;
    tool_ = tool;
    if (tool.kind != Tool::Kind::Select) EndEditing();
}

// ----------------------------------------------------------------- drawing

void View::DrawPage(Renderer& renderer, const Transform& transform) const {
    const Size page = document_->PageSize();
    const RectF paper =
        transform.ToDevice(Rect(-page.cx / 2, page.cy / 2, page.cx / 2, -page.cy / 2));

    const float shadow = ui::ScaleF(dpiScale_, 3.0f);
    ui::FillRoundRect(renderer,
                      RectF{paper.left + shadow, paper.top + shadow, paper.right + shadow,
                            paper.bottom + shadow},
                      shadow, RGB(0, 0, 0), 40);
    renderer.FillRect(paper, ui::Argb(document_->PaperColour()));

    // Grid lines are gathered into two paths so the rasterizer runs twice
    // rather than once per line.
    if (showGrid_ && transform.scale > 0.25f) {
        Path minor;
        Path major;

        for (long x = -page.cx / 2; x <= page.cx / 2; x += kMinorGrid) {
            const float device = std::floor(transform.ToDevice(Point(x, 0)).x);
            (x % kMajorGrid == 0 ? major : minor)
                .AddRect({device, paper.top, device + 1.0f, paper.bottom});
        }
        for (long y = -page.cy / 2; y <= page.cy / 2; y += kMinorGrid) {
            const float device = std::floor(transform.ToDevice(Point(0, y)).y);
            (y % kMajorGrid == 0 ? major : minor)
                .AddRect({paper.left, device, paper.right, device + 1.0f});
        }

        renderer.Fill(minor, SolidPaint(ui::Argb(RGB(0xE8, 0xEB, 0xF0))));
        renderer.Fill(major, SolidPaint(ui::Argb(RGB(0xD3, 0xD9, 0xE2))));
    }

    ui::FrameRoundRect(renderer, paper, 0.0f, ui::kBorderStrong, 1.0f);
}

void View::DrawSelection(Renderer& renderer, const Transform& transform) const {
    const float handle = ui::ScaleF(dpiScale_, 4.0f);

    for (const ObjectId id : selection_) {
        const DrawObj* object = Find(id);
        if (!object) continue;

        const RectF bounds = transform.ToDevice(object->Bounds());
        ui::FrameRoundRect(renderer, bounds, 2.0f, ui::kAccent, 1.0f);

        if (selection_.size() > 1) continue;

        for (int i = 0; i < object->HandleCount(); ++i) {
            const Vec2 point = transform.ToDevice(object->Handle(i));
            const RectF box{point.x - handle, point.y - handle, point.x + handle,
                            point.y + handle};
            ui::FillRoundRect(renderer, box, 2.0f, ui::kPanel);
            ui::FrameRoundRect(renderer, box.Inflated(-0.5f), 2.0f, ui::kAccent, 1.4f);
        }
    }
}

void View::DrawCaret(Renderer& renderer, Canvas& canvas,
                     const Transform& transform) const {
    const DrawObj* object = Find(editing_);
    if (!object) return;

    const pf::font_handle font = CreateStyleFont(object->style, transform.scale);
    if (!font) return;

    auto context = pf::create_pixel_surface_context(canvas.Surface());
    if (!context) {
        pf::delete_font_handle(font);
        return;
    }
    const auto full = context->measure_text_h(pf::utf16_to_utf8(object->text), font);
    const auto prefix = context->measure_text_h(
        pf::utf16_to_utf8(object->text.substr(0, caret_)), font);
    const int lineHeight = pf::line_height_for_font(font);
    pf::delete_font_handle(font);

    const Rect label = LabelRect(*object, transform);
    const float centreX = (label.left + label.right) * 0.5f;
    const float centreY = (label.top + label.bottom) * 0.5f;
    const float x = centreX - full.cx * 0.5f + prefix.cx;
    const float half = lineHeight * 0.5f;

    renderer.FillRect({x, centreY - half, x + ui::ScaleF(dpiScale_, 1.5f), centreY + half},
                      ui::Argb(object->style.textColour));
}

View::ZoomBadge View::BadgeLayout() const {
    const Rect client{0, 0, viewport_.cx, viewport_.cy};

    const int margin = ui::Scale(dpiScale_, 10);
    const int height = ui::Scale(dpiScale_, 24);
    const int button = ui::Scale(dpiScale_, 26);
    const int label = ui::Scale(dpiScale_, 52);

    ZoomBadge badge;
    badge.bounds = Rect{margin, client.bottom - margin - height, margin + button * 2 + label,
                        client.bottom - margin};
    badge.minus = Rect{badge.bounds.left, badge.bounds.top, badge.bounds.left + button,
                       badge.bounds.bottom};
    badge.label = Rect{badge.minus.right, badge.bounds.top, badge.minus.right + label,
                       badge.bounds.bottom};
    badge.plus = Rect{badge.label.right, badge.bounds.top, badge.bounds.right,
                      badge.bounds.bottom};
    return badge;
}

void View::DrawZoomBadge(Renderer& renderer, Canvas& canvas) const {
    const ZoomBadge badge = BadgeLayout();
    const RectF bounds = RectF::FromRect(badge.bounds);
    const float radius = ui::ScaleF(dpiScale_, 5.0f);

    ui::FillRoundRect(renderer, bounds, radius, ui::kPanel, 235);
    ui::FrameRoundRect(renderer, bounds.Inflated(-0.5f), radius, ui::kBorder, 1.0f);

    const float arm = ui::ScaleF(dpiScale_, 4.5f);
    const Vec2 minus = RectF::FromRect(badge.minus).Centre();
    ui::Line(renderer, {minus.x - arm, minus.y}, {minus.x + arm, minus.y}, ui::kText, 1.4f);

    const Vec2 plus = RectF::FromRect(badge.plus).Centre();
    ui::Line(renderer, {plus.x - arm, plus.y}, {plus.x + arm, plus.y}, ui::kText, 1.4f);
    ui::Line(renderer, {plus.x, plus.y - arm}, {plus.x, plus.y + arm}, ui::kText, 1.4f);

    const auto percent = static_cast<int>(std::lround(zoom_ * 100.0f));
    ui::Text(canvas, badge.label, std::to_wstring(percent) + L"%", ui::kText,
             ui::TextAlign::center, ui::Font(dpiScale_));
}

void View::Render(Renderer& renderer, Canvas& canvas) const {
    renderer.Clear(ui::Argb(ui::kWorkspace));

    const Transform transform = GetTransform();
    DrawPage(renderer, transform);

    document_->Draw(renderer, canvas, transform, editing_);

    if (document_->Objects().empty()) {
        // Centred on what the user can actually see, not on the page.
        const Rect hint{0, 0, viewport_.cx, viewport_.cy};
        ui::Text(canvas, hint, L"Pick a shape on the left, then click or drag here", ui::kTextDim,
                 ui::TextAlign::center, ui::Font(dpiScale_));
    }

    DrawSelection(renderer, transform);

    if (bandActive_) {
        const RectF band = RectF::FromRect(band_.Normalized());
        renderer.FillRect(band, ui::Argb(ui::kAccent, 40));
        ui::FrameRoundRect(renderer, band, 0.0f, ui::kAccent, 1.0f);
    }

    if (editing_ != ObjectId::None && caretVisible_) DrawCaret(renderer, canvas, transform);

    DrawZoomBadge(renderer, canvas);
}

void View::Invalidate() const {
    if (onInvalidate) onInvalidate();
}

void View::Changed() {
    if (onChanged) onChanged();
}

// ------------------------------------------------------- scrolling / zoom

void View::UpdateScrollBars() {
    const Transform transform = GetTransform();
    const Size page = document_->PageSize();
    const auto margin = ui::Scale(dpiScale_, kPageMargin);

    extent_ = Size(static_cast<std::int32_t>(page.cx * transform.scale) + margin * 2,
                   static_cast<std::int32_t>(page.cy * transform.scale) + margin * 2);

    scroll_.x = (std::clamp)(scroll_.x, 0, (std::max)(0, extent_.cx - viewport_.cx));
    scroll_.y = (std::clamp)(scroll_.y, 0, (std::max)(0, extent_.cy - viewport_.cy));
}

void View::ScrollTo(Point scroll) {
    scroll.x = (std::clamp)(scroll.x, 0, (std::max)(0, extent_.cx - viewport_.cx));
    scroll.y = (std::clamp)(scroll.y, 0, (std::max)(0, extent_.cy - viewport_.cy));
    if (scroll == scroll_) return;

    scroll_ = scroll;
    UpdateScrollBars();
    Invalidate();
}

void View::SetZoom(float zoom, Point anchor) {
    zoom = (std::clamp)(zoom, kMinZoom, kMaxZoom);
    if (std::fabs(zoom - zoom_) < 1e-4f) return;

    // Keep whatever sits under `anchor` pinned there.
    const Point pinned = ToDoc(anchor);
    zoom_ = zoom;
    UpdateScrollBars();

    const Vec2 moved = GetTransform().ToDevice(pinned);
    ScrollTo(Point(scroll_.x + static_cast<long>(moved.x) - anchor.x,
                   scroll_.y + static_cast<long>(moved.y) - anchor.y));

    UpdateScrollBars();
    Invalidate();
    Changed();
}

void View::ZoomBy(float factor) {
    const Rect client{0, 0, viewport_.cx, viewport_.cy};
    SetZoom(zoom_ * factor, Point{client.right / 2, client.bottom / 2});
}

void View::ZoomToFit() {
    const Rect client{0, 0, viewport_.cx, viewport_.cy};

    const Size page = document_->PageSize();
    const float base = static_cast<float>(dpiScale_ * 96.0 / 100.0);
    const float margin = ui::ScaleF(dpiScale_, static_cast<float>(kPageMargin)) * 2.0f;

    const float available = (std::max)(1.0f, static_cast<float>(client.right) - margin);
    const float availableY = (std::max)(1.0f, static_cast<float>(client.bottom) - margin);

    SetZoom((std::min)(available / (page.cx * base), availableY / (page.cy * base)),
            Point{client.right / 2, client.bottom / 2});
}

void View::OnMouseWheel(const Point point, const int delta, const bool control, const bool shift) {
    if (control) {
        SetZoom(zoom_ * std::pow(kZoomStep, static_cast<float>(delta) / 120.0f), point);
        return;
    }

    Point target = scroll_;
    const long step = delta * ui::Scale(dpiScale_, 60) / 120;
    if (shift)
        target.x -= step;
    else
        target.y -= step;
    ScrollTo(target);
}

// ---------------------------------------------------------------- clipboard

void View::Copy() {
    if (selection_.empty()) return;

    Document::ObjectList clones;
    clones.reserve(selection_.size());
    for (const ObjectId id : selection_) {
        if (const DrawObj* object = Find(id)) clones.push_back(*object);
    }

    const std::string text = ObjectsToJson(clones).Dump();
    if (frame_) frame_->text_to_clipboard(text);
}

bool View::ClipboardHasObjects() const {
    if (!frame_) return false;
    const std::string text = frame_->text_from_clipboard();
    if (text.empty()) return false;
    try {
        return Json::Parse(text).IsArray();
    } catch (const FileError&) {
        return false;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

void View::Paste() {
    if (!frame_) return;
    const std::string text = frame_->text_from_clipboard();
    if (text.empty()) return;

    Document::ObjectList pasted;
    try {
        pasted = ObjectsFromJson(Json::Parse(text));
    } catch (const std::exception&) {
        return;
    }

    document_->BeginChange();
    selection_.clear();
    for (DrawObj& object : pasted) {
        object.Offset(Size(kMinorGrid, -kMinorGrid));
        selection_.push_back(document_->Add(std::move(object)));
    }
    document_->CommitChange();

    Invalidate();
    Changed();
}

// -------------------------------------------------------------- interaction

void View::CreateAt(ShapeKind kind, Point clientPoint) {
    Style style = style_;
    if (IsLinear(kind)) {
        style.hasFill = false;
        style.endArrow = kind == ShapeKind::Arrow;
    }

    const Point centre = Snap(ToDoc(clientPoint));

    document_->BeginChange();
    const ObjectId id =
        document_->Add(DrawObj(kind, CentredRect(centre, kDefaultWidth, kDefaultHeight), style));
    document_->CommitChange();

    Select(id, false);
    Invalidate();
}

void View::OnLeftDown(const Point point, const bool shift, const bool control, const bool alt) {
    shiftHeld_ = shift;
    altHeld_ = alt;

    const ZoomBadge badge = BadgeLayout();
    if (point.x >= badge.minus.left && point.x < badge.minus.right &&
        point.y >= badge.minus.top && point.y < badge.minus.bottom) {
        ZoomBy(1.0f / kZoomStep);
        return;
    }
    if (point.x >= badge.plus.left && point.x < badge.plus.right &&
        point.y >= badge.plus.top && point.y < badge.plus.bottom) {
        ZoomBy(kZoomStep);
        return;
    }
    if (point.x >= badge.label.left && point.x < badge.label.right &&
        point.y >= badge.label.top && point.y < badge.label.bottom) {
        SetZoom(1.0f, Point{badge.label.left, badge.label.top});
        return;
    }

    const Point local = ToDoc(point);
    if (editing_ != ObjectId::None) {
        // Clicking inside the label being typed just moves the caret.
        const DrawObj* object = Find(editing_);
        if (object && object->HitTest(local, HitTolerance())) {
            caret_ = CaretIndexAt(point);
            caretVisible_ = true;
            Invalidate();
            return;
        }
        EndEditing();
    }

    dragOrigin_ = local;
    drag_ = Drag::None;
    dragHandle_ = 0;

    if (tool_.kind != Tool::Kind::Select) {
        Style style = style_;
        const bool linear = IsLinear(tool_.shape);
        if (tool_.kind == Tool::Kind::Text) {
            style.hasFill = false;
            style.hasLine = false;
        } else if (linear) {
            style.hasFill = false;
            style.endArrow = tool_.shape == ShapeKind::Arrow;
        }

        const ShapeKind kind =
            tool_.kind == Tool::Kind::Text ? ShapeKind::Rectangle : tool_.shape;

        document_->BeginChange();
        dragChange_ = true;

        const Point start = Snap(local);
        const Rect bounds(start, Size(0, 0));

        selection_.assign(1, document_->Add(DrawObj(kind, bounds, style)));
        drag_ = Drag::Resize;
        dragHandle_ = linear ? 1 : 4;
        dragStart_ = bounds;
        dragOrigin_ = start;
        captured_ = true;
        if (onCapture) onCapture(true);
        Invalidate();
        Changed();
        return;
    }

    // Resize handles win over everything else, for a single selection.
    if (selection_.size() == 1) {
        const DrawObj* object = Primary();
        const int handle = object ? HandleAt(*object, local) : -1;
        if (handle >= 0) {
            document_->BeginChange();
            dragChange_ = true;
            drag_ = Drag::Resize;
            dragHandle_ = handle;
            dragStart_ = object->position;
            captured_ = true;
            if (onCapture) onCapture(true);
            return;
        }
    }

    if (const ObjectId hit = document_->ObjectAt(local, HitTolerance()); hit != ObjectId::None) {
        const bool add = shift;
        const bool selected = IsSelected(hit);

        if (add && selected)
            RemoveFromSelection(hit);
        else if (!selected)
            Select(hit, add);

        if (!selection_.empty()) {
            document_->BeginChange();
            dragChange_ = true;
            if (control) DuplicateSelection();
            drag_ = Drag::Move;
            if (const DrawObj* primary = Primary()) dragStart_ = primary->position;
            captured_ = true;
            if (onCapture) onCapture(true);
        }
        Invalidate();
        Changed();
        return;
    }

    if (!shift) ClearSelection();
    drag_ = Drag::Band;
    bandActive_ = true;
    band_ = Rect(point.x, point.y, point.x, point.y);
    captured_ = true;
    if (onCapture) onCapture(true);
}

void View::SetCursor(const pf::cursor_shape cursor) const {
    if (onCursor) onCursor(cursor);
}

void View::OnMouseMove(const Point point, const bool shift, const bool control, const bool alt) {
    (void)control;
    shiftHeld_ = shift;
    altHeld_ = alt;
    const Point local = ToDoc(point);

    if (drag_ == Drag::None) {
        pf::cursor_shape cursor = pf::cursor_shape::arrow;
        if (tool_.kind == Tool::Kind::Text) {
            cursor = pf::cursor_shape::ibeam;
        } else if (tool_.kind == Tool::Kind::Shape) {
            cursor = pf::cursor_shape::hand;
        } else {
            const DrawObj* primary = selection_.size() == 1 ? Primary() : nullptr;
            const int handle = primary ? HandleAt(*primary, local) : -1;
            if (handle >= 0) {
                cursor = (handle == 1 || handle == 5)
                              ? pf::cursor_shape::size_ns
                              : (handle == 3 || handle == 7)
                                    ? pf::cursor_shape::size_we
                                    : pf::cursor_shape::hand;
            } else if (IsSelected(document_->ObjectAt(local, HitTolerance()))) {
                cursor = pf::cursor_shape::hand;
            }
        }

        const ZoomBadge badge = BadgeLayout();
        if (point.x >= badge.bounds.left && point.x < badge.bounds.right &&
            point.y >= badge.bounds.top && point.y < badge.bounds.bottom)
            cursor = pf::cursor_shape::hand;

        SetCursor(cursor);
        return;
    }

    switch (drag_) {
        case Drag::Band:
            band_.right = point.x;
            band_.bottom = point.y;
            Invalidate();
            break;

        case Drag::Move: {
            SetCursor(pf::cursor_shape::hand);

            const DrawObj* primary = Primary();
            if (!primary) break;

            // Snap the object the drag started on; everything else follows it.
            const Size raw = local - dragOrigin_;
            const Point wanted = Snap(dragStart_.TopLeft() + raw);
            const Size delta = wanted - primary->position.TopLeft();
            if (delta.cx == 0 && delta.cy == 0) break;

            for (const ObjectId id : selection_) {
                if (DrawObj* object = Find(id)) object->Offset(delta);
            }
            Invalidate();
            break;
        }

        case Drag::Resize: {
            DrawObj* primary = Primary();
            if (!primary) break;

            SetCursor((dragHandle_ == 1 || dragHandle_ == 5)
                          ? pf::cursor_shape::size_ns
                          : (dragHandle_ == 3 || dragHandle_ == 7)
                                ? pf::cursor_shape::size_we
                                : pf::cursor_shape::hand);

            Point target = Snap(local);
            // Shift keeps a shape square, pivoting on the opposite corner.
            if (shift && !IsLinear(primary->kind))
                target = SquareCornerTarget(dragStart_, dragHandle_, target);
            primary->MoveHandle(dragHandle_, target);
            Invalidate();
            break;
        }

        case Drag::None: break;
    }
}

std::size_t View::CaretIndexAt(const Point clientPoint) const {
    const DrawObj* object = Find(editing_);
    if (!object || object->text.empty()) return 0;

    const Transform transform = GetTransform();
    const pf::font_handle font = CreateStyleFont(object->style, transform.scale);
    if (!font) return object->text.size();
    const auto full = pf::measure_text_with_font(font, pf::utf16_to_utf8(object->text));

    const Rect label = LabelRect(*object, transform);
    const int origin = (label.left + label.right) / 2 - full.cx / 2;

    std::size_t best = 0;
    int bestDistance = std::abs(clientPoint.x - origin);
    for (std::size_t i = 1; i <= object->text.size(); ++i) {
        if (!IsCaretStop(object->text, i)) continue;
        const auto prefix = pf::measure_text_with_font(
            font, pf::utf16_to_utf8(object->text.substr(0, i)));
        const int distance = std::abs(clientPoint.x - (origin + prefix.cx));
        if (distance < bestDistance) {
            bestDistance = distance;
            best = i;
        }
    }

    pf::delete_font_handle(font);
    return best;
}

void View::EndDragChange() {
    if (!dragChange_) return;
    dragChange_ = false;
    document_->CommitChange();
}

void View::OnLeftUp(const Point point, const bool shift, const bool control, const bool alt) {
    shiftHeld_ = shift;
    altHeld_ = alt;
    const Drag drag = drag_;
    drag_ = Drag::None;
    if (captured_) {
        captured_ = false;
        if (onCapture) onCapture(false);
    }

    if (drag == Drag::Band) {
        bandActive_ = false;
        const Rect device = band_.Normalized();

        const Transform transform = GetTransform();
        const Point corner = transform.ToDoc(
            Vec2{static_cast<float>(device.left), static_cast<float>(device.top)});
        const Point opposite = transform.ToDoc(
            Vec2{static_cast<float>(device.right), static_cast<float>(device.bottom)});
        const Rect area(corner.x, corner.y, opposite.x, opposite.y);

        for (const DrawObj& object : document_->Objects()) {
            if (object.IsWithin(area)) AddToSelection(object.id);
        }

        Invalidate();
        Changed();
        return;
    }

    if (drag == Drag::Resize && tool_.kind != Tool::Kind::Select && !selection_.empty()) {
        const ObjectId id = selection_.front();

        // A click without a drag places a default-sized object instead.
        if (DrawObj* object = Find(id)) {
            const Rect fixed = object->position.Normalized();
            if (fixed.Width() < 8 && fixed.Height() < 8)
                object->position = CentredRect(dragOrigin_, kDefaultWidth, kDefaultHeight);
        }

        const bool wasText = tool_.kind == Tool::Kind::Text;
        tool_ = Tool{};
        EndDragChange();
        if (wasText) BeginEditing(id);

        Invalidate();
        Changed();
        return;
    }

    EndDragChange();
    if (drag != Drag::None) Changed();

    OnMouseMove(point, shift, control, alt);
}

void View::OnDoubleClick(const Point point, const bool shift, const bool control, const bool alt) {
    // The zoom badge holds buttons: a quick second click on one is another
    // step, not a double-click on whatever lies underneath.
    const ZoomBadge badge = BadgeLayout();
    if (point.x >= badge.bounds.left && point.x < badge.bounds.right &&
        point.y >= badge.bounds.top && point.y < badge.bounds.bottom) {
        OnLeftDown(point, shift, control, alt);
        return;
    }

    const ObjectId id = document_->ObjectAt(ToDoc(point), HitTolerance());
    if (id == ObjectId::None) return;

    Select(id, false);
    BeginEditing(id);
}

void View::BeginEditing(ObjectId id) {
    if (!document_->Contains(id)) return;

    EndEditing();
    document_->BeginChange();
    textChange_ = true;

    editing_ = id;
    caret_ = Find(id)->text.size();
    caretVisible_ = true;
    if (frame_) frame_->set_timer(kCaretTimer, 500);
    Invalidate();
}

void View::EndEditing() {
    if (editing_ == ObjectId::None) return;

    if (frame_) frame_->kill_timer(kCaretTimer);
    editing_ = ObjectId::None;

    if (textChange_) {
        textChange_ = false;
        document_->CommitChange();
    }

    Invalidate();
    Changed();
}

void View::FinishEditing() { EndEditing(); }

void View::OnChar(const char32_t character) {
    DrawObj* object = Find(editing_);
    if (!object) return;

    if (character == L'\b') {
        if (caret_ == 0) return;
        const std::size_t start = PreviousCaretStop(object->text, caret_);
        object->text.erase(start, caret_ - start);
        caret_ = start;
    } else if (character == L'\r' || character == L'\n' || character == L'\t') {
        EndEditing();
        return;
    } else if (character >= U' ') {
        std::wstring inserted;
        if (character <= 0xffff) {
            inserted.push_back(static_cast<wchar_t>(character));
        } else {
            inserted.push_back(static_cast<wchar_t>((character >> 10) + 0xd7c0));
            inserted.push_back(static_cast<wchar_t>((character & 0x3ff) + 0xdc00));
        }
        object->text.insert(caret_, inserted);
        caret_ += inserted.size();
    } else {
        return;
    }

    caretVisible_ = true;
    Invalidate();
}

void View::OnKeyDown(const unsigned int key, const bool shift) {
    if (DrawObj* object = Find(editing_)) {
        switch (key) {
            case pf::platform_key::Escape: EndEditing(); return;
            case pf::platform_key::Left: caret_ = PreviousCaretStop(object->text, caret_); break;
            case pf::platform_key::Right: caret_ = NextCaretStop(object->text, caret_); break;
            case pf::platform_key::Home: caret_ = 0; break;
            case pf::platform_key::End: caret_ = object->text.size(); break;
            case pf::platform_key::Delete:
                if (caret_ < object->text.size())
                    object->text.erase(caret_, NextCaretStop(object->text, caret_) - caret_);
                break;
            default: return;
        }
        caretVisible_ = true;
        Invalidate();
        return;
    }

    const long step = shift ? 1 : kMinorGrid;
    switch (key) {
        case pf::platform_key::Escape: OnCommand(ID_CANCEL); break;
        case pf::platform_key::Left: NudgeSelection(-step, 0); break;
        case pf::platform_key::Right: NudgeSelection(step, 0); break;
        case pf::platform_key::Up: NudgeSelection(0, step); break;
        case pf::platform_key::Down: NudgeSelection(0, -step); break;
        case pf::platform_key::F2:
            if (selection_.size() == 1) BeginEditing(selection_.front());
            break;
        default: break;
    }
}

void View::OnContextMenu(const Point point) {
    if (point.x >= 0 && point.y >= 0) {
        const ObjectId hit = document_->ObjectAt(ToDoc(point), HitTolerance());
        if (hit != ObjectId::None && !IsSelected(hit)) Select(hit, false);
    }
    if (onContextMenuRequested) onContextMenuRequested();
}

void View::OnTimer(const unsigned int id) {
    if (id != kCaretTimer || editing_ == ObjectId::None) return;
    caretVisible_ = !caretVisible_;
    Invalidate();
}

// ---------------------------------------------------------------- commands

bool View::OnCommand(int id) {
    // While a label is being typed the edit commands belong to the text.
    if (editing_ != ObjectId::None) {
        switch (id) {
            case ID_EDIT_CLEAR: OnKeyDown(pf::platform_key::Delete, false); return true;
            case ID_EDIT_CUT:
            case ID_EDIT_PASTE:
            case ID_EDIT_DUPLICATE:
            case ID_EDIT_SELECT_ALL: return true;
            default: break;
        }
    }

    switch (id) {
        case ID_EDIT_UNDO:
        case ID_EDIT_REDO: {
            EndEditing();
            const bool changed = id == ID_EDIT_UNDO ? document_->Undo() : document_->Redo();
            if (changed) {
                // Ids outlive the snapshot swap, so the selection survives.
                std::erase_if(selection_,
                              [this](ObjectId each) { return !document_->Contains(each); });
                Invalidate();
                Changed();
            }
            return true;
        }

        case ID_EDIT_SELECT_ALL: SelectAll(); return true;
        case ID_EDIT_CLEAR: DeleteSelection(); return true;
        case ID_EDIT_DUPLICATE: DuplicateSelection(); return true;
        case ID_EDIT_COPY: Copy(); return true;
        case ID_EDIT_CUT:
            Copy();
            DeleteSelection();
            return true;
        case ID_EDIT_PASTE: Paste(); return true;

        case ID_EDIT_TEXT:
            if (selection_.size() == 1) BeginEditing(selection_.front());
            return true;

        case ID_VIEW_GRID:
            showGrid_ = !showGrid_;
            Invalidate();
            return true;

        case ID_VIEW_SNAP:
            snap_ = !snap_;
            return true;

        case ID_VIEW_ZOOM_IN: ZoomBy(kZoomStep); return true;
        case ID_VIEW_ZOOM_OUT: ZoomBy(1.0f / kZoomStep); return true;
        case ID_VIEW_ZOOM_FIT: ZoomToFit(); return true;
        case ID_VIEW_ZOOM_ACTUAL: {
            const Rect client{0, 0, viewport_.cx, viewport_.cy};
            SetZoom(1.0f, Point{client.right / 2, client.bottom / 2});
            return true;
        }

        case ID_VIEW_PAPER_COLOUR: {
            if (onPickColour) {
                if (auto colour = onPickColour(document_->PaperColour())) {
                    document_->BeginChange();
                    document_->SetPaperColour(*colour);
                    document_->CommitChange();
                    Invalidate();
                    Changed();
                }
            }
            return true;
        }

        case ID_OBJECT_ALIGN_LEFT:
        case ID_OBJECT_ALIGN_CENTRE:
        case ID_OBJECT_ALIGN_RIGHT:
        case ID_OBJECT_ALIGN_TOP:
        case ID_OBJECT_ALIGN_MIDDLE:
        case ID_OBJECT_ALIGN_BOTTOM: AlignSelection(id); return true;

        case ID_OBJECT_FRONT:
        case ID_OBJECT_BACK:
        case ID_OBJECT_FORWARD:
        case ID_OBJECT_BACKWARD: {
            if (selection_.size() != 1) return true;
            const ObjectId target = selection_.front();

            document_->BeginChange();
            if (id == ID_OBJECT_FRONT)
                document_->MoveToFront(target);
            else if (id == ID_OBJECT_BACK)
                document_->MoveToBack(target);
            else if (id == ID_OBJECT_FORWARD)
                document_->MoveForward(target);
            else
                document_->MoveBackward(target);
            document_->CommitChange();

            Invalidate();
            Changed();
            return true;
        }

        case ID_CANCEL:
            EndEditing();
            if (captured_) {
                captured_ = false;
                if (onCapture) onCapture(false);
            }
            drag_ = Drag::None;
            EndDragChange();
            bandActive_ = false;
            tool_ = Tool{};
            Invalidate();
            Changed();
            return true;

        default: break;
    }

    if (id >= ID_PAGE_FIRST && id <= ID_PAGE_LAST) {
        document_->BeginChange();
        document_->SetPageSize(kPageSizes[static_cast<std::size_t>(id - ID_PAGE_FIRST)]);
        document_->CommitChange();
        UpdateScrollBars();
        Invalidate();
        Changed();
        return true;
    }

    return false;
}

void View::QueryCommandState(int id, bool& enabled, bool& checked) const {
    enabled = true;
    checked = false;

    if (id >= ID_PAGE_FIRST && id <= ID_PAGE_LAST) {
        checked = document_->PageSize() ==
                  kPageSizes[static_cast<std::size_t>(id - ID_PAGE_FIRST)];
        return;
    }

    const bool editing = editing_ != ObjectId::None;

    switch (id) {
        // Invoking either ends the label first, so an open edit still counts.
        case ID_EDIT_UNDO: enabled = document_->CanUndo() || editing; break;
        case ID_EDIT_REDO: enabled = document_->CanRedo() || editing; break;

        case ID_EDIT_CLEAR:
        case ID_EDIT_COPY:
        case ID_EDIT_CUT:
        case ID_EDIT_DUPLICATE: enabled = !selection_.empty(); break;

        case ID_EDIT_TEXT:
        case ID_OBJECT_FRONT:
        case ID_OBJECT_BACK:
        case ID_OBJECT_FORWARD:
        case ID_OBJECT_BACKWARD: enabled = selection_.size() == 1; break;

        case ID_OBJECT_ALIGN_LEFT:
        case ID_OBJECT_ALIGN_CENTRE:
        case ID_OBJECT_ALIGN_RIGHT:
        case ID_OBJECT_ALIGN_TOP:
        case ID_OBJECT_ALIGN_MIDDLE:
        case ID_OBJECT_ALIGN_BOTTOM: enabled = selection_.size() > 1; break;

        case ID_EDIT_PASTE: enabled = ClipboardHasObjects(); break;
        case ID_EDIT_SELECT_ALL: enabled = !document_->Objects().empty(); break;
        case ID_VIEW_GRID: checked = showGrid_; break;
        case ID_VIEW_SNAP: checked = snap_; break;
        default: break;
    }
}

// ------------------------------------------------------------ export/print

bool View::ExportPng(const std::filesystem::path& path) const {
    Canvas canvas;
    if (!RenderDocumentToCanvas(*document_, canvas, kExportDpi)) return false;

    try {
        WritePng(canvas, path);
    } catch (const std::exception&) {
        return false;
    }
    return true;
}
