// The drawing surface for one document: scrolling, zooming, selection, the
// drawing tools and in-place label editing.  The view is an app-owned model
// and renderer.  Its containing window is libdesktop's ui::frame.

#pragma once

#include "document.h"
#include "libdesktop/ui.h"
#include "shapes.h"
#include "style.h"
#include "ui.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

struct Tool {
    enum class Kind { Select, Shape, Text };

    Kind kind = Kind::Select;
    ShapeKind shape = ShapeKind::Rectangle;

    friend bool operator==(const Tool& a, const Tool& b) {
        return a.kind == b.kind && a.shape == b.shape;
    }
};

class View {
public:
    explicit View(std::unique_ptr<Document> document,
                  ui::frame_ptr frame = {});
    ~View();

    View(const View&) = delete;
    View& operator=(const View&) = delete;

    // The local surface is laid out by MainWindow.  dpiScale is the monitor
    // scale reported by libdesktop; no native window handle is retained by the application.
    void SetViewport(Size extent, double dpiScale);
    Size Viewport() const { return viewport_; }

    Document& Doc() const { return *document_; }

    // Document units (0.01 inch, y up, origin at the page centre) to pixels.
    Transform GetTransform() const;

    const std::vector<ObjectId>& Selection() const { return selection_; }
    void SelectAll();
    void ClearSelection();

    Tool CurrentTool() const { return tool_; }
    void SetTool(Tool tool);

    // The style shown in the sidebar: the first selected object's, or the
    // style that new objects will be created with.
    Style EffectiveStyle() const;
    void ApplyStyle(const Style& style);

    // Drops a ready-made object centred on a local client point (palette drag).
    void CreateAt(ShapeKind kind, Point clientPoint);

    // Commits an in-progress label. Call before asking whether the document
    // needs saving, or the last keystrokes are not accounted for yet.
    void FinishEditing();

    bool OnCommand(int id);
    void QueryCommandState(int id, bool& enabled, bool& checked) const;

    void Render(Renderer& renderer, Canvas& canvas) const;
    bool ExportPng(const std::filesystem::path& path) const;

    // Input delivered by MainWindow's ui::frame_host.
    void OnLeftDown(Point point, bool shift, bool control, bool alt);
    void OnLeftUp(Point point, bool shift, bool control, bool alt);
    void OnMouseMove(Point point, bool shift, bool control, bool alt);
    void OnDoubleClick(Point point, bool shift, bool control, bool alt);
    void OnMouseWheel(Point point, int delta, bool control, bool shift);
    bool OnKeyDown(char32_t key, bool shift);
    void OnChar(char32_t character);
    void OnTimer(unsigned int id);
    bool IsEditing() const { return editing_ != ObjectId::None; }
    void OnContextMenu(Point point);

    // Raised when the selection, the style or the modified flag changes.
    std::function<void()> onChanged;
    std::function<void()> onInvalidate;
    std::function<void(bool)> onCapture;
    std::function<void(platform::cursor)> onCursor;
    std::function<std::optional<Color>(Color)> onPickColour;
    std::function<void()> onContextMenuRequested;

private:
    enum class Drag { None, Move, Resize, Band };

    struct ZoomBadge {
        Rect bounds{};
        Rect minus{};
        Rect label{};
        Rect plus{};
    };

    Point ToDoc(Point clientPoint) const;
    long HitTolerance() const;
    Point Snap(Point docPoint) const;
    int HandleAt(const DrawObj& object, Point docPoint) const;

    // Borrowed for the duration of one operation only.
    DrawObj* Find(ObjectId id) const;
    DrawObj* Primary() const;

    void DrawPage(Renderer& renderer, const Transform& transform) const;
    void DrawSelection(Renderer& renderer, const Transform& transform) const;
    void DrawCaret(Renderer& renderer, Canvas& canvas, const Transform& transform) const;
    void DrawZoomBadge(Renderer& renderer, Canvas& canvas) const;
    ZoomBadge BadgeLayout() const;

    void UpdateScrollBars();
    void ScrollTo(Point scroll);
    void SetZoom(float zoom, Point anchor);
    void ZoomBy(float factor);
    void ZoomToFit();

    void Select(ObjectId id, bool add);
    void AddToSelection(ObjectId id);
    void RemoveFromSelection(ObjectId id);
    bool IsSelected(ObjectId id) const;
    void DeleteSelection();
    void DuplicateSelection();
    void NudgeSelection(long dx, long dy);
    void AlignSelection(int id);
    void Copy();
    void Paste();
    bool ClipboardHasObjects() const;

    void BeginEditing(ObjectId id);
    void EndEditing();
    std::size_t CaretIndexAt(Point clientPoint) const;
    void EndDragChange();

    void Invalidate() const;
    void Changed();
    void SetCursor(platform::cursor cursor) const;

    ui::frame_ptr frame_;
    std::unique_ptr<Document> document_;

    Size viewport_{};
    double dpiScale_ = 1.0;
    std::vector<ObjectId> selection_;
    Tool tool_;
    Style style_;
    bool showGrid_ = true;
    bool snap_ = true;

    Point scroll_{};
    Size extent_{};
    float zoom_ = 1.0f;

    Drag drag_ = Drag::None;
    int dragHandle_ = 0;
    Point dragOrigin_{};
    Rect dragStart_{};
    bool dragChange_ = false;
    Rect band_{};
    bool bandActive_ = false;
    bool captured_ = false;
    bool altHeld_ = false;
    bool shiftHeld_ = false;

    ObjectId editing_ = ObjectId::None;
    std::size_t caret_ = 0;
    bool caretVisible_ = true;
    bool textChange_ = false;
};

// Renders a whole page at `dpi` into `canvas`. Shared by PNG export, printing
// and the tests.
bool RenderDocumentToCanvas(const Document& document, Canvas& canvas, int dpi);
