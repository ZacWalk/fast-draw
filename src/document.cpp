#include "document.h"

#include "file_io.h"

#include <algorithm>

namespace {

constexpr char kFormatName[] = "fast-draw";
constexpr long kFormatVersion = 2;

// Roughly how many edits can be walked back.
constexpr std::size_t kMaxHistory = 128;

// A page side read from a file is capped at 100": several times A0, yet small
// enough that the grid, the scroll range and an export stay tractable. A
// damaged file asking for kilometres of paper would otherwise hang the view.
constexpr long kMaxPageSide = 10000;

int NextUntitledIndex() {
    static int next = 1;
    return next++;
}

void ReportFailure(HWND owner, std::wstring_view action, const std::filesystem::path& path,
                   const std::exception& error) {
    const std::string what = error.what();
    const std::wstring message = L"Could not " + std::wstring(action) + L" " +
                                 path.filename().wstring() + L".\n\n" +
                                 std::wstring(what.begin(), what.end());
    ::MessageBoxW(owner, message.c_str(), L"fast-draw", MB_OK | MB_ICONERROR);
}

}  // namespace

Json ObjectsToJson(const Document::ObjectList& objects) {
    Json list = Json::MakeArray();
    for (const DrawObj& object : objects) list.Push(object.ToJson());
    return list;
}

Document::ObjectList ObjectsFromJson(const Json& json) {
    if (!json.IsArray()) throw JsonError("expected an array of objects");

    Document::ObjectList objects;
    objects.reserve(json.AsArray().size());
    for (const auto& item : json.AsArray()) objects.push_back(DrawObj::FromJson(item));
    return objects;
}

Document::Document() : untitledIndex_(NextUntitledIndex()) {}

std::wstring Document::Title() const {
    if (!path_.empty()) return path_.filename().wstring();
    return L"Untitled " + std::to_wstring(untitledIndex_);
}

ObjectId Document::Add(DrawObj object) {
    object.id = static_cast<ObjectId>(++nextId_);
    objects_.push_back(std::move(object));
    modified_ = true;
    return objects_.back().id;
}

Document::ObjectList::iterator Document::Locate(ObjectId id) {
    if (id == ObjectId::None) return objects_.end();
    return std::find_if(objects_.begin(), objects_.end(),
                        [id](const DrawObj& candidate) { return candidate.id == id; });
}

DrawObj* Document::Find(ObjectId id) {
    auto it = Locate(id);
    return it == objects_.end() ? nullptr : &*it;
}

const DrawObj* Document::Find(ObjectId id) const {
    return const_cast<Document*>(this)->Find(id);
}

void Document::Remove(ObjectId id) {
    auto it = Locate(id);
    if (it == objects_.end()) return;
    objects_.erase(it);
    modified_ = true;
}

ObjectId Document::ObjectAt(Point docPoint, long tolerance) const {
    // Search top down so the frontmost object wins.
    for (auto it = objects_.rbegin(); it != objects_.rend(); ++it) {
        if (it->HitTest(docPoint, tolerance)) return it->id;
    }
    return ObjectId::None;
}

void Document::MoveToFront(ObjectId id) {
    auto it = Locate(id);
    if (it == objects_.end() || it + 1 == objects_.end()) return;
    std::rotate(it, it + 1, objects_.end());
    modified_ = true;
}

void Document::MoveToBack(ObjectId id) {
    auto it = Locate(id);
    if (it == objects_.end() || it == objects_.begin()) return;
    std::rotate(objects_.begin(), it, it + 1);
    modified_ = true;
}

void Document::MoveForward(ObjectId id) {
    auto it = Locate(id);
    if (it == objects_.end() || it + 1 == objects_.end()) return;
    std::iter_swap(it, it + 1);
    modified_ = true;
}

void Document::MoveBackward(ObjectId id) {
    auto it = Locate(id);
    if (it == objects_.end() || it == objects_.begin()) return;
    std::iter_swap(it, it - 1);
    modified_ = true;
}

void Document::Draw(Renderer& renderer, HDC textDc, const Transform& transform,
                    ObjectId editing) const {
    for (const DrawObj& object : objects_) {
        object.Draw(renderer, transform);
        if (textDc)
            object.DrawLabel(textDc, transform, editing == ObjectId::None || object.id != editing);
    }
}

void Document::SetPaperColour(COLORREF colour) {
    paperColour_ = colour;
    modified_ = true;
}

void Document::SetPageSize(Size size) {
    if (size.cx <= 0 || size.cy <= 0 || size == pageSize_) return;
    pageSize_ = size;
    modified_ = true;
}

// ------------------------------------------------------------------ history

Document::Snapshot Document::Capture() const {
    return Snapshot{objects_, paperColour_, pageSize_};
}

void Document::Restore(Snapshot snapshot) {
    objects_ = std::move(snapshot.objects);
    paperColour_ = snapshot.paperColour;
    pageSize_ = snapshot.pageSize;
    modified_ = true;
}

void Document::BeginChange() {
    if (changeDepth_++ > 0) return;

    undo_.push_back(Capture());
    if (undo_.size() > kMaxHistory) undo_.erase(undo_.begin());
    redo_.clear();
}

void Document::CommitChange() {
    if (changeDepth_ == 0 || --changeDepth_ > 0) return;
    if (undo_.empty()) return;

    // Dragging, restyling and labelling mutate objects directly, so this is the
    // only place that can tell whether anything really happened.
    if (undo_.back() == Capture())
        undo_.pop_back();
    else
        modified_ = true;
}

bool Document::Undo() {
    // A drag in progress owns the document; replacing the object list under it
    // would leave the view working from a half-applied edit.
    if (changeDepth_ > 0 || undo_.empty()) return false;

    redo_.push_back(Capture());
    Restore(std::move(undo_.back()));
    undo_.pop_back();
    return true;
}

bool Document::Redo() {
    if (changeDepth_ > 0 || redo_.empty()) return false;

    undo_.push_back(Capture());
    Restore(std::move(redo_.back()));
    redo_.pop_back();
    return true;
}

bool Document::Load(HWND owner, const std::filesystem::path& path) {
    try {
        const Json json = Json::Parse(ReadFileText(path));

        if (json["format"].AsString() != kFormatName)
            throw JsonError("this is not a fast-draw document");
        if (json["version"].AsInt() > kFormatVersion)
            throw JsonError("the document was written by a newer version");

        paperColour_ = ColourFromHex(json["paperColour"].AsString(), paperColour_);

        const Json& page = json["page"];
        const Size size(page["width"].AsInt(pageSize_.cx), page["height"].AsInt(pageSize_.cy));
        if (size.cx > 0 && size.cy > 0)
            pageSize_ = Size((std::min)(size.cx, kMaxPageSide), (std::min)(size.cy, kMaxPageSide));

        objects_ = ObjectsFromJson(json["objects"]);
        for (DrawObj& object : objects_) object.id = static_cast<ObjectId>(++nextId_);

        undo_.clear();
        redo_.clear();
        changeDepth_ = 0;
        path_ = path;
        modified_ = false;
        return true;
    } catch (const std::exception& error) {
        ReportFailure(owner, L"open", path, error);
        return false;
    }
}

bool Document::Save(HWND owner, const std::filesystem::path& path) {
    try {
        Json page = Json::MakeObject();
        page.Set("width", Json(pageSize_.cx));
        page.Set("height", Json(pageSize_.cy));

        Json json = Json::MakeObject();
        json.Set("format", Json(std::string(kFormatName)));
        json.Set("version", Json(kFormatVersion));
        json.Set("units", Json(std::string("0.01in, origin at page centre, y up")));
        json.Set("page", std::move(page));
        json.Set("paperColour", Json(ColourToHex(paperColour_)));
        json.Set("objects", ObjectsToJson(objects_));

        WriteFileText(path, json.Dump());

        path_ = path;
        modified_ = false;
        return true;
    } catch (const std::exception& error) {
        ReportFailure(owner, L"save", path, error);
        return false;
    }
}
