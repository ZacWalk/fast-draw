// A drawable object: one shape, one style and an optional text label.
//
// Geometry is stored in document coordinates: 0.01 inch units with the y axis
// pointing up, so `position.top` is normally greater than `position.bottom`.

#pragma once

#include "geometry.h"
#include "render.h"
#include "shapes.h"
#include "style.h"

#include <windows.h>

#include <cstdint>
#include <string>

// A stable handle to an object. Unlike an address it survives undo replacing
// the whole object list, and unlike an index it survives reordering.
enum class ObjectId : std::uint64_t { None = 0 };

class DrawObj {
public:
    DrawObj() = default;
    DrawObj(ShapeKind shape, const Rect& bounds, const Style& objectStyle)
        : kind(shape), position(bounds), style(objectStyle) {}

    Json ToJson() const;
    static DrawObj FromJson(const Json& json);

    // The outline in device pixels. Linear shapes stay open.
    void BuildPath(const Transform& transform, Path& out) const;

    void Draw(Renderer& renderer, const Transform& transform) const;
    // Text still goes through GDI; call this after the shapes have been filled.
    // `fit` trims an overlong label with an ellipsis. A label that is being typed
    // is drawn in full instead, so the caret always sits beside visible text.
    void DrawLabel(HDC dc, const Transform& transform, bool fit = true) const;

    // Document bounds including the stroke width and any arrow heads.
    Rect Bounds() const;

    bool HitTest(Point docPoint, long tolerance) const;
    bool IsWithin(const Rect& docRect) const;

    int HandleCount() const;
    Point Handle(int index) const;
    void MoveHandle(int index, Point docPoint);
    HCURSOR HandleCursor(int index) const;

    void Offset(Size delta);

    ShapeKind kind = ShapeKind::Rectangle;
    Rect position;
    Style style;
    std::wstring text;

    // Assigned by the document; not serialized, so a pasted copy is a new object.
    ObjectId id = ObjectId::None;

    friend bool operator==(const DrawObj&, const DrawObj&) = default;
};

// The label font at a given document-to-device scale. The caller owns it.
HFONT CreateStyleFont(const Style& style, float scale);

// The rectangle the text label is laid out in, in device pixels.
RECT LabelRect(const DrawObj& object, const Transform& transform);

// Where a Shift-dragged corner handle goes to keep the shape square: onto the
// diagonal through the corner opposite `handle` in `start`, the position at the
// start of the drag. An edge handle cannot keep a square, so its target is
// returned unchanged.
Point SquareCornerTarget(const Rect& start, int handle, Point target);
