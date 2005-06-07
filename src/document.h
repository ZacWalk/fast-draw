// The document: an ordered list of objects plus the page it lives on.
//
// Painting and invalidation belong to the view; this type is pure data and
// persistence so the tests can exercise it without a window.
//
// Objects are stored by value and referred to from outside by ObjectId. A
// DrawObj* handed out by Find() is a borrowed view of one element and is only
// valid until the next mutation, so nothing keeps one across an edit.

#pragma once

#include "draw_obj.h"
#include "geometry.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class Document {
public:
    using ObjectList = std::vector<DrawObj>;

    Document();

    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;

    // Painted back to front, so the tail of the list is on top.
    ObjectList& Objects() { return objects_; }
    const ObjectList& Objects() const { return objects_; }

    ObjectId Add(DrawObj object);
    void Remove(ObjectId id);

    // Borrowed; invalidated by the next mutation. Null when the id is stale.
    DrawObj* Find(ObjectId id);
    const DrawObj* Find(ObjectId id) const;
    bool Contains(ObjectId id) const { return Find(id) != nullptr; }

    ObjectId ObjectAt(Point docPoint, long tolerance) const;

    void MoveToFront(ObjectId id);
    void MoveToBack(ObjectId id);
    void MoveForward(ObjectId id);
    void MoveBackward(ObjectId id);

    void Draw(Renderer& renderer, HDC textDc, const Transform& transform) const;

    // Bracket every mutation so it can be undone. Nested calls collapse into
    // one history entry, and an edit that changed nothing leaves no entry.
    void BeginChange();
    void CommitChange();

    bool CanUndo() const { return changeDepth_ == 0 && !undo_.empty(); }
    bool CanRedo() const { return changeDepth_ == 0 && !redo_.empty(); }
    bool Undo();
    bool Redo();

    Size PageSize() const { return pageSize_; }
    void SetPageSize(Size size);

    COLORREF PaperColour() const { return paperColour_; }
    void SetPaperColour(COLORREF colour);

    bool IsModified() const { return modified_; }
    void SetModified(bool modified = true) { modified_ = modified; }

    const std::filesystem::path& Path() const { return path_; }
    bool HasPath() const { return !path_.empty(); }
    std::wstring Title() const;

    // Both report failure by returning false after showing a message box.
    bool Load(HWND owner, const std::filesystem::path& path);
    bool Save(HWND owner, const std::filesystem::path& path);

private:
    struct Snapshot {
        ObjectList objects;
        COLORREF paperColour = 0;
        Size pageSize;

        friend bool operator==(const Snapshot&, const Snapshot&) = default;
    };

    ObjectList::iterator Locate(ObjectId id);
    Snapshot Capture() const;
    void Restore(Snapshot snapshot);

    ObjectList objects_;
    std::vector<Snapshot> undo_;
    std::vector<Snapshot> redo_;
    int changeDepth_ = 0;
    // Never rewound by undo, so a restored object can never collide with a new one.
    std::uint64_t nextId_ = 0;
    Size pageSize_{1100, 850};  // 11" x 8.5", landscape suits diagrams
    COLORREF paperColour_ = RGB(255, 255, 255);
    bool modified_ = false;
    std::filesystem::path path_;
    int untitledIndex_ = 0;
};

// Used for the document body and for the private clipboard format. Reading
// leaves the ids unset; the document assigns them as the objects are added.
Json ObjectsToJson(const Document::ObjectList& objects);
Document::ObjectList ObjectsFromJson(const Json& json);
