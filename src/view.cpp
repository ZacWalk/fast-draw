#include "view.h"

#include "color_popup.h"
#include "png_export.h"
#include "resource.h"

#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace {

constexpr wchar_t kViewClassName[] = L"FastDrawView";

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
constexpr int kPrintDpi = 200;

constexpr UINT_PTR kCaretTimer = 1;

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

bool AltHeld() { return (::GetKeyState(VK_MENU) & 0x8000) != 0; }

// Axis-aligned extents of an object in document space, y increasing upwards.
struct Extent {
    long minX, maxX, minY, maxY;
};

Extent ExtentOf(const Rect& rect) {
    return Extent{(std::min)(rect.left, rect.right), (std::max)(rect.left, rect.right),
                  (std::min)(rect.top, rect.bottom), (std::max)(rect.top, rect.bottom)};
}

}  // namespace

UINT DrawClipboardFormat() {
    static const UINT format = ::RegisterClipboardFormatW(L"fast-draw objects (JSON)");
    return format;
}

bool RenderDocumentToCanvas(const Document& document, Canvas& canvas, int dpi) {
    const Size page = document.PageSize();
    if (!canvas.Resize(::MulDiv(page.cx, dpi, 100), ::MulDiv(page.cy, dpi, 100))) return false;

    Renderer renderer(canvas);
    renderer.Clear(ArgbFromColorRef(document.PaperColour()));

    Transform transform;
    transform.scale = static_cast<float>(dpi) / 100.0f;
    transform.offsetX = page.cx * 0.5f * transform.scale;
    transform.offsetY = page.cy * 0.5f * transform.scale;

    document.Draw(renderer, canvas.Dc(), transform);
    canvas.SetOpaque();
    return true;
}

// ------------------------------------------------------ creation / teardown

void View::RegisterWindowClass(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = &View::WndProc;
    wc.hInstance = instance;
    wc.hCursor = nullptr;  // the active tool picks the cursor
    wc.lpszClassName = kViewClassName;
    ::RegisterClassExW(&wc);
}

HWND View::Create(HWND parent) {
    return ::CreateWindowExW(0, kViewClassName, L"",
                             WS_CHILD | WS_CLIPSIBLINGS | WS_HSCROLL | WS_VSCROLL, 0, 0, 0, 0,
                             parent, nullptr, ::GetModuleHandleW(nullptr), nullptr);
}

View* View::FromHandle(HWND window) {
    if (!window) return nullptr;
    return reinterpret_cast<View*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
}

View::View(HWND window, std::unique_ptr<Document> document)
    : hwnd_(window), document_(std::move(document)) {}

View::~View() = default;

// ------------------------------------------------------------- coordinates

Transform View::GetTransform() const {
    const Size page = document_->PageSize();
    const float margin = ui::ScaleF(hwnd_, static_cast<float>(kPageMargin));

    Transform transform;
    transform.scale = static_cast<float>(ui::Dpi(hwnd_)) / 100.0f * zoom_;
    transform.offsetX = margin - scroll_.x + page.cx * 0.5f * transform.scale;
    transform.offsetY = margin - scroll_.y + page.cy * 0.5f * transform.scale;
    return transform;
}

Point View::ToDoc(POINT clientPoint) const {
    return GetTransform().ToDoc(
        Vec2{static_cast<float>(clientPoint.x), static_cast<float>(clientPoint.y)});
}

long View::HitTolerance() const {
    return (std::max)(3L, static_cast<long>(ui::ScaleF(hwnd_, 5.0f) / GetTransform().scale));
}

Point View::Snap(Point docPoint) const {
    if (!snap_ || AltHeld()) return docPoint;
    return Point(RoundTo(docPoint.x, kMinorGrid), RoundTo(docPoint.y, kMinorGrid));
}

int View::HandleAt(const DrawObj& object, Point docPoint) const {
    const long reach = static_cast<long>(ui::ScaleF(hwnd_, 6.0f) / GetTransform().scale);
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
    // selection, so a mixed selection keeps the rest of its own styling.
    const Style previous = EffectiveStyle();
    style_ = style;

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

    const float shadow = ui::ScaleF(hwnd_, 3.0f);
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
    const float handle = ui::ScaleF(hwnd_, 4.0f);

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

void View::DrawCaret(Renderer& renderer, HDC dc, const Transform& transform) const {
    const DrawObj* object = Find(editing_);
    if (!object) return;

    HFONT font = CreateStyleFont(object->style, transform.scale);
    if (!font) return;

    HGDIOBJ previous = ::SelectObject(dc, font);

    SIZE full{};
    ::GetTextExtentPoint32W(dc, object->text.c_str(), static_cast<int>(object->text.size()),
                            &full);
    SIZE prefix{};
    ::GetTextExtentPoint32W(dc, object->text.c_str(), static_cast<int>(caret_), &prefix);
    TEXTMETRICW metrics{};
    ::GetTextMetricsW(dc, &metrics);

    ::SelectObject(dc, previous);
    ::DeleteObject(font);

    const RECT label = LabelRect(*object, transform);
    const float centreX = (label.left + label.right) * 0.5f;
    const float centreY = (label.top + label.bottom) * 0.5f;
    const float x = centreX - full.cx * 0.5f + prefix.cx;
    const float half = metrics.tmHeight * 0.5f;

    renderer.FillRect({x, centreY - half, x + ui::ScaleF(hwnd_, 1.5f), centreY + half},
                      ui::Argb(object->style.textColour));
}

View::ZoomBadge View::BadgeLayout() const {
    RECT client{};
    ::GetClientRect(hwnd_, &client);

    const int margin = ui::Scale(hwnd_, 10);
    const int height = ui::Scale(hwnd_, 24);
    const int button = ui::Scale(hwnd_, 26);
    const int label = ui::Scale(hwnd_, 52);

    ZoomBadge badge;
    badge.bounds = RECT{margin, client.bottom - margin - height, margin + button * 2 + label,
                        client.bottom - margin};
    badge.minus = RECT{badge.bounds.left, badge.bounds.top, badge.bounds.left + button,
                       badge.bounds.bottom};
    badge.label = RECT{badge.minus.right, badge.bounds.top, badge.minus.right + label,
                       badge.bounds.bottom};
    badge.plus = RECT{badge.label.right, badge.bounds.top, badge.bounds.right,
                      badge.bounds.bottom};
    return badge;
}

void View::DrawZoomBadge(Renderer& renderer, HDC dc) const {
    const ZoomBadge badge = BadgeLayout();
    const RectF bounds = RectF::FromRect(badge.bounds);
    const float radius = ui::ScaleF(hwnd_, 5.0f);

    ui::FillRoundRect(renderer, bounds, radius, ui::kPanel, 235);
    ui::FrameRoundRect(renderer, bounds.Inflated(-0.5f), radius, ui::kBorder, 1.0f);

    const float arm = ui::ScaleF(hwnd_, 4.5f);
    const Vec2 minus = RectF::FromRect(badge.minus).Centre();
    ui::Line(renderer, {minus.x - arm, minus.y}, {minus.x + arm, minus.y}, ui::kText, 1.4f);

    const Vec2 plus = RectF::FromRect(badge.plus).Centre();
    ui::Line(renderer, {plus.x - arm, plus.y}, {plus.x + arm, plus.y}, ui::kText, 1.4f);
    ui::Line(renderer, {plus.x, plus.y - arm}, {plus.x, plus.y + arm}, ui::kText, 1.4f);

    const auto percent = static_cast<int>(std::lround(zoom_ * 100.0f));
    ui::Text(dc, badge.label, std::to_wstring(percent) + L"%", ui::kText,
             DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX, ui::Font(hwnd_));
}

void View::Render(Renderer& renderer, HDC dc, const RECT& dirty) const {
    renderer.Clear(ui::Argb(ui::kWorkspace));

    const Transform transform = GetTransform();
    DrawPage(renderer, transform);

    const int saved = ::SaveDC(dc);
    ::IntersectClipRect(dc, dirty.left, dirty.top, dirty.right, dirty.bottom);
    document_->Draw(renderer, dc, transform);

    if (document_->Objects().empty()) {
        // Centred on what the user can actually see, not on the page.
        RECT hint{};
        ::GetClientRect(hwnd_, &hint);
        ui::Text(dc, hint, L"Pick a shape on the left, then click or drag here", ui::kTextDim,
                 DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX, ui::Font(hwnd_));
    }

    ::RestoreDC(dc, saved);

    DrawSelection(renderer, transform);

    if (bandActive_) {
        const RectF band = RectF::FromRect(band_.Normalized());
        renderer.FillRect(band, ui::Argb(ui::kAccent, 40));
        ui::FrameRoundRect(renderer, band, 0.0f, ui::kAccent, 1.0f);
    }

    if (editing_ != ObjectId::None && caretVisible_) DrawCaret(renderer, dc, transform);

    DrawZoomBadge(renderer, dc);
}

void View::OnPaint() {
    PAINTSTRUCT ps{};
    if (surface_.Begin(hwnd_, ps)) Render(surface_.GetRenderer(), surface_.Dc(), ps.rcPaint);
    surface_.End(hwnd_, ps);
}

void View::Invalidate() const { ::InvalidateRect(hwnd_, nullptr, FALSE); }

void View::Changed() {
    if (onChanged) onChanged();
}

// ------------------------------------------------------- scrolling / zoom

void View::UpdateScrollBars() {
    const Transform transform = GetTransform();
    const Size page = document_->PageSize();
    const long margin = ui::Scale(hwnd_, kPageMargin);

    extent_ = Size(static_cast<long>(page.cx * transform.scale) + margin * 2,
                   static_cast<long>(page.cy * transform.scale) + margin * 2);

    RECT client{};
    ::GetClientRect(hwnd_, &client);

    scroll_.x = (std::clamp)(scroll_.x, 0L, (std::max)(0L, extent_.cx - client.right));
    scroll_.y = (std::clamp)(scroll_.y, 0L, (std::max)(0L, extent_.cy - client.bottom));

    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;

    info.nMax = (std::max)(0L, extent_.cx - 1);
    info.nPage = static_cast<UINT>((std::max)(0L, client.right));
    info.nPos = scroll_.x;
    ::SetScrollInfo(hwnd_, SB_HORZ, &info, TRUE);

    info.nMax = (std::max)(0L, extent_.cy - 1);
    info.nPage = static_cast<UINT>((std::max)(0L, client.bottom));
    info.nPos = scroll_.y;
    ::SetScrollInfo(hwnd_, SB_VERT, &info, TRUE);
}

void View::ScrollTo(Point scroll) {
    RECT client{};
    ::GetClientRect(hwnd_, &client);

    scroll.x = (std::clamp)(scroll.x, 0L, (std::max)(0L, extent_.cx - client.right));
    scroll.y = (std::clamp)(scroll.y, 0L, (std::max)(0L, extent_.cy - client.bottom));
    if (scroll == scroll_) return;

    scroll_ = scroll;
    UpdateScrollBars();
    Invalidate();
}

void View::SetZoom(float zoom, POINT anchor) {
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
    RECT client{};
    ::GetClientRect(hwnd_, &client);
    SetZoom(zoom_ * factor, POINT{client.right / 2, client.bottom / 2});
}

void View::ZoomToFit() {
    RECT client{};
    ::GetClientRect(hwnd_, &client);

    const Size page = document_->PageSize();
    const float base = static_cast<float>(ui::Dpi(hwnd_)) / 100.0f;
    const float margin = ui::ScaleF(hwnd_, static_cast<float>(kPageMargin)) * 2.0f;

    const float available = (std::max)(1.0f, static_cast<float>(client.right) - margin);
    const float availableY = (std::max)(1.0f, static_cast<float>(client.bottom) - margin);

    SetZoom((std::min)(available / (page.cx * base), availableY / (page.cy * base)),
            POINT{client.right / 2, client.bottom / 2});
}

void View::OnSize() {
    UpdateScrollBars();
    Invalidate();
}

void View::OnScroll(int bar, WPARAM wParam) {
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL;
    ::GetScrollInfo(hwnd_, bar, &info);

    int position = info.nPos;
    const int line = ui::Scale(hwnd_, 24);
    const int page = static_cast<int>(info.nPage);

    switch (LOWORD(wParam)) {
        case SB_LINEUP: position -= line; break;
        case SB_LINEDOWN: position += line; break;
        case SB_PAGEUP: position -= page; break;
        case SB_PAGEDOWN: position += page; break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION: position = info.nTrackPos; break;
        case SB_TOP: position = info.nMin; break;
        case SB_BOTTOM: position = info.nMax; break;
        default: return;
    }

    Point target = scroll_;
    (bar == SB_HORZ ? target.x : target.y) = position;
    ScrollTo(target);
}

void View::OnMouseWheel(POINT screenPoint, int delta, UINT flags) {
    if (flags & MK_CONTROL) {
        POINT client = screenPoint;
        ::ScreenToClient(hwnd_, &client);
        SetZoom(zoom_ * std::pow(kZoomStep, static_cast<float>(delta) / WHEEL_DELTA), client);
        return;
    }

    Point target = scroll_;
    const long step = delta * ui::Scale(hwnd_, 60) / WHEEL_DELTA;
    if (flags & MK_SHIFT)
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

    HGLOBAL memory = ::GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (!memory) return;

    if (void* target = ::GlobalLock(memory)) {
        std::memcpy(target, text.c_str(), text.size() + 1);
        ::GlobalUnlock(memory);
    }

    if (::OpenClipboard(hwnd_)) {
        ::EmptyClipboard();
        if (!::SetClipboardData(DrawClipboardFormat(), memory)) ::GlobalFree(memory);
        ::CloseClipboard();
    } else {
        ::GlobalFree(memory);
    }
}

bool View::ClipboardHasObjects() const {
    return ::IsClipboardFormatAvailable(DrawClipboardFormat()) != FALSE;
}

void View::Paste() {
    if (!::OpenClipboard(hwnd_)) return;

    std::string text;
    if (HANDLE handle = ::GetClipboardData(DrawClipboardFormat())) {
        const SIZE_T size = ::GlobalSize(handle);
        if (const char* source = static_cast<const char*>(::GlobalLock(handle))) {
            text.assign(source, ::strnlen(source, size));
            ::GlobalUnlock(handle);
        }
    }
    ::CloseClipboard();
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

void View::CreateAt(ShapeKind kind, POINT clientPoint) {
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

void View::OnLeftDown(POINT point, UINT flags) {
    ::SetFocus(hwnd_);

    const ZoomBadge badge = BadgeLayout();
    if (::PtInRect(&badge.minus, point)) {
        ZoomBy(1.0f / kZoomStep);
        return;
    }
    if (::PtInRect(&badge.plus, point)) {
        ZoomBy(kZoomStep);
        return;
    }
    if (::PtInRect(&badge.label, point)) {
        SetZoom(1.0f, POINT{badge.label.left, badge.label.top});
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
        ::SetCapture(hwnd_);
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
            ::SetCapture(hwnd_);
            return;
        }
    }

    if (const ObjectId hit = document_->ObjectAt(local, HitTolerance()); hit != ObjectId::None) {
        const bool add = (flags & MK_SHIFT) != 0;
        const bool selected = IsSelected(hit);

        if (add && selected)
            RemoveFromSelection(hit);
        else if (!selected)
            Select(hit, add);

        if (!selection_.empty()) {
            document_->BeginChange();
            dragChange_ = true;
            if (flags & MK_CONTROL) DuplicateSelection();
            drag_ = Drag::Move;
            if (const DrawObj* primary = Primary()) dragStart_ = primary->position;
            ::SetCapture(hwnd_);
        }
        Invalidate();
        Changed();
        return;
    }

    if ((flags & MK_SHIFT) == 0) ClearSelection();
    drag_ = Drag::Band;
    bandActive_ = true;
    band_ = Rect(point.x, point.y, point.x, point.y);
    ::SetCapture(hwnd_);
}

void View::OnMouseMove(POINT point, UINT flags) {
    const Point local = ToDoc(point);

    if (drag_ == Drag::None) {
        HCURSOR cursor = ::LoadCursorW(nullptr, IDC_ARROW);
        if (tool_.kind == Tool::Kind::Text) {
            cursor = ::LoadCursorW(nullptr, IDC_IBEAM);
        } else if (tool_.kind == Tool::Kind::Shape) {
            cursor = ::LoadCursorW(nullptr, IDC_CROSS);
        } else {
            const DrawObj* primary = selection_.size() == 1 ? Primary() : nullptr;
            const int handle = primary ? HandleAt(*primary, local) : -1;
            if (handle >= 0)
                cursor = primary->HandleCursor(handle);
            else if (IsSelected(document_->ObjectAt(local, HitTolerance())))
                cursor = ::LoadCursorW(nullptr, IDC_SIZEALL);
        }

        const ZoomBadge badge = BadgeLayout();
        if (::PtInRect(&badge.bounds, point)) cursor = ::LoadCursorW(nullptr, IDC_HAND);

        ::SetCursor(cursor);
        return;
    }

    switch (drag_) {
        case Drag::Band:
            band_.right = point.x;
            band_.bottom = point.y;
            Invalidate();
            break;

        case Drag::Move: {
            ::SetCursor(::LoadCursorW(nullptr, IDC_SIZEALL));

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

            ::SetCursor(primary->HandleCursor(dragHandle_));

            Point target = Snap(local);
            // Shift constrains a shape to a square.
            if ((flags & MK_SHIFT) && !IsLinear(primary->kind)) {
                const long size = (std::max)(std::labs(target.x - dragStart_.left),
                                             std::labs(target.y - dragStart_.top));
                target.x = dragStart_.left + (target.x < dragStart_.left ? -size : size);
                target.y = dragStart_.top + (target.y < dragStart_.top ? -size : size);
            }
            primary->MoveHandle(dragHandle_, target);
            Invalidate();
            break;
        }

        case Drag::None: break;
    }
}

std::size_t View::CaretIndexAt(POINT clientPoint) const {
    const DrawObj* object = Find(editing_);
    if (!object || object->text.empty()) return 0;

    const Transform transform = GetTransform();
    HDC dc = ::GetDC(hwnd_);
    if (!dc) return object->text.size();

    HFONT font = CreateStyleFont(object->style, transform.scale);
    HGDIOBJ previous = ::SelectObject(dc, font);

    SIZE full{};
    ::GetTextExtentPoint32W(dc, object->text.c_str(), static_cast<int>(object->text.size()),
                            &full);

    const RECT label = LabelRect(*object, transform);
    const int origin = (label.left + label.right) / 2 - full.cx / 2;

    std::size_t best = 0;
    int bestDistance = std::abs(clientPoint.x - origin);
    for (std::size_t i = 1; i <= object->text.size(); ++i) {
        SIZE prefix{};
        ::GetTextExtentPoint32W(dc, object->text.c_str(), static_cast<int>(i), &prefix);
        const int distance = std::abs(clientPoint.x - (origin + prefix.cx));
        if (distance < bestDistance) {
            bestDistance = distance;
            best = i;
        }
    }

    ::SelectObject(dc, previous);
    ::DeleteObject(font);
    ::ReleaseDC(hwnd_, dc);
    return best;
}

void View::EndDragChange() {
    if (!dragChange_) return;
    dragChange_ = false;
    document_->CommitChange();
}

void View::OnLeftUp(POINT point, UINT flags) {
    // Snapshot first: ReleaseCapture delivers WM_CAPTURECHANGED synchronously,
    // which clears the drag state.
    const Drag drag = drag_;
    drag_ = Drag::None;
    if (::GetCapture() == hwnd_) ::ReleaseCapture();

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

    OnMouseMove(point, flags);
}

void View::OnDoubleClick(POINT point) {
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
    ::SetTimer(hwnd_, kCaretTimer, ::GetCaretBlinkTime(), nullptr);
    Invalidate();
}

void View::EndEditing() {
    if (editing_ == ObjectId::None) return;

    ::KillTimer(hwnd_, kCaretTimer);
    editing_ = ObjectId::None;

    if (textChange_) {
        textChange_ = false;
        document_->CommitChange();
    }

    Invalidate();
    Changed();
}

void View::FinishEditing() { EndEditing(); }

void View::OnChar(wchar_t character) {
    DrawObj* object = Find(editing_);
    if (!object) return;

    if (character == L'\b') {
        if (caret_ == 0) return;
        object->text.erase(caret_ - 1, 1);
        --caret_;
    } else if (character == L'\r' || character == L'\n' || character == L'\t') {
        EndEditing();
        return;
    } else if (character >= L' ') {
        object->text.insert(caret_, 1, character);
        ++caret_;
    } else {
        return;
    }

    caretVisible_ = true;
    Invalidate();
}

void View::OnKeyDown(WPARAM key) {
    if (DrawObj* object = Find(editing_)) {
        switch (key) {
            case VK_ESCAPE: EndEditing(); return;
            case VK_LEFT:
                if (caret_ > 0) --caret_;
                break;
            case VK_RIGHT:
                if (caret_ < object->text.size()) ++caret_;
                break;
            case VK_HOME: caret_ = 0; break;
            case VK_END: caret_ = object->text.size(); break;
            case VK_DELETE:
                if (caret_ < object->text.size()) object->text.erase(caret_, 1);
                break;
            default: return;
        }
        caretVisible_ = true;
        Invalidate();
        return;
    }

    const long step = (::GetKeyState(VK_SHIFT) & 0x8000) ? 1 : kMinorGrid;
    switch (key) {
        case VK_ESCAPE: OnCommand(ID_CANCEL); break;
        case VK_LEFT: NudgeSelection(-step, 0); break;
        case VK_RIGHT: NudgeSelection(step, 0); break;
        case VK_UP: NudgeSelection(0, step); break;
        case VK_DOWN: NudgeSelection(0, -step); break;
        case VK_F2:
            if (selection_.size() == 1) BeginEditing(selection_.front());
            break;
        default: break;
    }
}

void View::OnContextMenu(POINT screenPoint) {
    POINT client = screenPoint;
    ::ScreenToClient(hwnd_, &client);

    const ObjectId hit = document_->ObjectAt(ToDoc(client), HitTolerance());
    if (hit != ObjectId::None && !IsSelected(hit)) Select(hit, false);

    HMENU menu = ::LoadMenuW(::GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDR_CONTEXT_MENU));
    if (!menu) return;

    if (HMENU popup = ::GetSubMenu(menu, 0)) {
        ::TrackPopupMenu(popup, TPM_RIGHTBUTTON | TPM_LEFTALIGN, screenPoint.x, screenPoint.y, 0,
                         ::GetParent(hwnd_), nullptr);
    }
    ::DestroyMenu(menu);
}

// ---------------------------------------------------------------- commands

bool View::OnCommand(int id) {
    // While a label is being typed the edit commands belong to the text.
    if (editing_ != ObjectId::None) {
        switch (id) {
            case ID_EDIT_CLEAR: OnKeyDown(VK_DELETE); return true;
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
            RECT client{};
            ::GetClientRect(hwnd_, &client);
            SetZoom(1.0f, POINT{client.right / 2, client.bottom / 2});
            return true;
        }

        case ID_VIEW_PAPER_COLOUR: {
            RECT anchor{};
            ::GetWindowRect(hwnd_, &anchor);
            anchor.bottom = anchor.top + 1;
            if (auto colour = PickColour(hwnd_, anchor, document_->PaperColour())) {
                document_->BeginChange();
                document_->SetPaperColour(*colour);
                document_->CommitChange();
                Invalidate();
                Changed();
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
            if (::GetCapture() == hwnd_) ::ReleaseCapture();
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

void View::Print(HDC printerDc) const {
    Canvas canvas;
    if (!RenderDocumentToCanvas(*document_, canvas, kPrintDpi)) return;

    const int printerWidth = ::GetDeviceCaps(printerDc, HORZRES);
    const int printerHeight = ::GetDeviceCaps(printerDc, VERTRES);

    // Fit the page to the printable area, preserving the aspect ratio.
    const double scale = (std::min)(static_cast<double>(printerWidth) / canvas.Width(),
                                    static_cast<double>(printerHeight) / canvas.Height());
    const int width = static_cast<int>(canvas.Width() * scale);
    const int height = static_cast<int>(canvas.Height() * scale);

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = canvas.Width();
    info.bmiHeader.biHeight = -canvas.Height();  // negative: top-down rows
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    // Printer drivers handle StretchDIBits far more reliably than a blit from
    // a memory DC.
    ::SetStretchBltMode(printerDc, HALFTONE);
    ::StretchDIBits(printerDc, (printerWidth - width) / 2, (printerHeight - height) / 2, width,
                    height, 0, 0, canvas.Width(), canvas.Height(), canvas.Pixels(), &info,
                    DIB_RGB_COLORS, SRCCOPY);
}

// -------------------------------------------------------------- window proc

LRESULT CALLBACK View::WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    View* view = FromHandle(window);

    switch (message) {
        case WM_CREATE: {
            auto* self = new View(window, std::make_unique<Document>());
            ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->UpdateScrollBars();
            return 0;
        }

        case WM_DESTROY:
            ::SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            delete view;
            return 0;

        case WM_ERASEBKGND: return 1;

        case WM_PAINT:
            if (view) view->OnPaint();
            return 0;

        case WM_SIZE:
            if (view) view->OnSize();
            return 0;

        case WM_DPICHANGED_AFTERPARENT:
            if (view) {
                view->UpdateScrollBars();
                view->Invalidate();
            }
            return 0;

        case WM_HSCROLL:
            if (view) view->OnScroll(SB_HORZ, wParam);
            return 0;

        case WM_VSCROLL:
            if (view) view->OnScroll(SB_VERT, wParam);
            return 0;

        case WM_MOUSEWHEEL:
            if (view)
                view->OnMouseWheel({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)},
                                   GET_WHEEL_DELTA_WPARAM(wParam), GET_KEYSTATE_WPARAM(wParam));
            return 0;

        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) return TRUE;
            break;

        case WM_LBUTTONDOWN:
            if (view)
                view->OnLeftDown({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)},
                                 static_cast<UINT>(wParam));
            return 0;

        case WM_LBUTTONUP:
            if (view)
                view->OnLeftUp({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)},
                               static_cast<UINT>(wParam));
            return 0;

        case WM_LBUTTONDBLCLK:
            if (view) view->OnDoubleClick({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
            return 0;

        case WM_MOUSEMOVE:
            if (view)
                view->OnMouseMove({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)},
                                  static_cast<UINT>(wParam));
            return 0;

        case WM_KEYDOWN:
            if (view) view->OnKeyDown(wParam);
            return 0;

        case WM_CHAR:
            if (view) view->OnChar(static_cast<wchar_t>(wParam));
            return 0;

        case WM_TIMER:
            if (view && view->editing_ != ObjectId::None) {
                view->caretVisible_ = !view->caretVisible_;
                view->Invalidate();
            }
            return 0;

        case WM_KILLFOCUS:
            if (view) view->EndEditing();
            return 0;

        case WM_CONTEXTMENU:
            if (view) {
                view->OnContextMenu({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
                return 0;
            }
            break;

        case WM_CAPTURECHANGED:
            if (view && view->drag_ != Drag::None) {
                view->drag_ = Drag::None;
                view->bandActive_ = false;
                view->EndDragChange();
                view->Invalidate();
            }
            return 0;

        default: break;
    }

    return ::DefWindowProcW(window, message, wParam, lParam);
}
