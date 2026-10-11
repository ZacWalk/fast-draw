// The drawing surface for one document: scrolling, zooming, selection, the
// drawing tools and in-place label editing.

#pragma once

#include "document.h"
#include "shapes.h"
#include "style.h"
#include "ui.h"

#include <filesystem>
#include <functional>
#include <memory>
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
    static void RegisterWindowClass(HINSTANCE instance);
    static HWND Create(HWND parent);
    static View* FromHandle(HWND window);

    View(HWND window, std::unique_ptr<Document> document);
    ~View();

    View(const View&) = delete;
    View& operator=(const View&) = delete;

    HWND Handle() const { return hwnd_; }
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

    // Drops a ready-made object centred on a client point (palette drag).
    void CreateAt(ShapeKind kind, POINT clientPoint);

    // Commits an in-progress label. Call before asking whether the document
    // needs saving, or the last keystrokes are not accounted for yet.
    void FinishEditing();

    bool OnCommand(int id);
    void QueryCommandState(int id, bool& enabled, bool& checked) const;

    void Print(HDC printerDc) const;
    bool ExportPng(const std::filesystem::path& path) const;

    // Raised when the selection, the style or the modified flag changes.
    std::function<void()> onChanged;

private:
    enum class Drag { None, Move, Resize, Band };

    // Hit zones of the zoom control in the corner of the view.
    struct ZoomBadge {
        RECT bounds{};
        RECT minus{};
        RECT label{};
        RECT plus{};
    };

    static LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    Point ToDoc(POINT clientPoint) const;
    long HitTolerance() const;
    Point Snap(Point docPoint) const;
    int HandleAt(const DrawObj& object, Point docPoint) const;

    // Borrowed for the duration of one operation only.
    DrawObj* Find(ObjectId id) const;
    DrawObj* Primary() const;

    void OnPaint();
    void Render(Renderer& renderer, HDC dc, const RECT& dirty) const;
    void DrawPage(Renderer& renderer, const Transform& transform) const;
    void DrawSelection(Renderer& renderer, const Transform& transform) const;
    void DrawCaret(Renderer& renderer, HDC dc, const Transform& transform) const;
    void DrawZoomBadge(Renderer& renderer, HDC dc) const;
    ZoomBadge BadgeLayout() const;

    void OnSize();
    void OnScroll(int bar, WPARAM wParam);
    void OnMouseWheel(POINT screenPoint, int delta, UINT flags);
    void UpdateScrollBars();
    void ScrollTo(Point scroll);
    void SetZoom(float zoom, POINT anchor);
    void ZoomBy(float factor);
    void ZoomToFit();

    void OnLeftDown(POINT point, UINT flags);
    void OnLeftUp(POINT point, UINT flags);
    void OnMouseMove(POINT point, UINT flags);
    void OnDoubleClick(POINT point, UINT flags);
    void OnKeyDown(WPARAM key);
    void OnChar(wchar_t character);
    void OnContextMenu(POINT screenPoint);

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
    std::size_t CaretIndexAt(POINT clientPoint) const;
    void EndDragChange();

    void Invalidate() const;
    void Changed();

    HWND hwnd_ = nullptr;
    std::unique_ptr<Document> document_;
    ui::Surface surface_;

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

    ObjectId editing_ = ObjectId::None;
    std::size_t caret_ = 0;
    bool caretVisible_ = true;
    bool textChange_ = false;
};
// The private clipboard format holding a serialized object list.
UINT DrawClipboardFormat();

// Renders a whole page at `dpi` into `canvas`. Shared by PNG export, printing
// and the tests.
bool RenderDocumentToCanvas(const Document& document, Canvas& canvas, int dpi);
