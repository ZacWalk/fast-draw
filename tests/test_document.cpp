#include "test_framework.h"

#include "document.h"
#include "draw_obj.h"
#include "file_io.h"
#include "json.h"
#include "style.h"

#include <filesystem>

namespace {

std::filesystem::path TempFile(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

DrawObj RoundTrip(const DrawObj& object) {
    return DrawObj::FromJson(Json::Parse(object.ToJson().Dump()));
}

}  // namespace

TEST(ColourHexRoundTrip) {
    CHECK_EQ(ColourToHex(RGB(0, 0, 0)), std::string("#000000"));
    CHECK_EQ(ColourToHex(RGB(255, 255, 255)), std::string("#ffffff"));
    CHECK_EQ(ColourToHex(RGB(0x12, 0x34, 0x56)), std::string("#123456"));

    CHECK_EQ(ColourFromHex("#123456", 0), RGB(0x12, 0x34, 0x56));
    CHECK_EQ(ColourFromHex("#ABCDEF", 0), RGB(0xAB, 0xCD, 0xEF));

    const COLORREF original = RGB(12, 200, 7);
    CHECK_EQ(ColourFromHex(ColourToHex(original), 0), original);
}

TEST(ColourHexRejectsBadInput) {
    const COLORREF fallback = RGB(1, 2, 3);
    CHECK_EQ(ColourFromHex("", fallback), fallback);
    CHECK_EQ(ColourFromHex("123456", fallback), fallback);
    CHECK_EQ(ColourFromHex("#12345", fallback), fallback);
    CHECK_EQ(ColourFromHex("#1234567", fallback), fallback);
    CHECK_EQ(ColourFromHex("#12345g", fallback), fallback);
}

TEST(EveryShapeKindHasAStableName) {
    for (int i = 0; i < kShapeKindCount; ++i) {
        const auto kind = static_cast<ShapeKind>(i);
        CHECK(ShapeKindFromName(ShapeKindName(kind), ShapeKind::Ellipse) == kind);
    }
    CHECK(ShapeKindFromName("banana", ShapeKind::Cloud) == ShapeKind::Cloud);
}

TEST(ObjectRoundTrip) {
    Style style;
    style.hasFill = false;
    style.fill = RGB(200, 100, 50);
    style.gradient = true;
    style.fillEnd = RGB(10, 20, 30);
    style.gradientDirection = GradientDirection::UpRight;
    style.line = RGB(1, 2, 3);
    style.lineWidth = 7;
    style.dash = DashStyle::DashDot;
    style.roundJoins = false;
    style.startArrow = true;
    style.textColour = RGB(9, 8, 7);
    style.fontSize = 21;
    style.bold = true;

    DrawObj original(ShapeKind::Cylinder, Rect(-100, 50, 100, -50), style);
    original.text = L"caf\u00e9 \u4f60\u597d";

    const DrawObj restored = RoundTrip(original);

    CHECK(restored.kind == ShapeKind::Cylinder);
    CHECK(restored.position == original.position);
    CHECK_EQ(restored.text, original.text);
    CHECK_EQ(restored.style.hasFill, false);
    CHECK_EQ(restored.style.fill, style.fill);
    CHECK_EQ(restored.style.gradient, true);
    CHECK_EQ(restored.style.fillEnd, style.fillEnd);
    CHECK(restored.style.gradientDirection == GradientDirection::UpRight);
    CHECK_EQ(restored.style.lineWidth, 7);
    CHECK(restored.style.dash == DashStyle::DashDot);
    CHECK_EQ(restored.style.roundJoins, false);
    CHECK_EQ(restored.style.startArrow, true);
    CHECK_EQ(restored.style.endArrow, false);
    CHECK_EQ(restored.style.fontSize, 21);
    CHECK_EQ(restored.style.bold, true);
}

TEST(UnknownShapeFallsBackToARectangle) {
    const DrawObj restored =
        DrawObj::FromJson(Json::Parse(R"({"shape":"banana","position":{}})"));
    CHECK(restored.kind == ShapeKind::Rectangle);
}

TEST(WildCoordinatesInAFileAreClamped) {
    // Bounds() inflates the rectangle, so unclamped extremes would overflow.
    const DrawObj restored = DrawObj::FromJson(Json::Parse(
        R"({"shape":"rectangle","position":{"left":-1e30,"top":1e30,"right":1e30,"bottom":-1e30}})"));

    CHECK(restored.position.left >= -1000000L);
    CHECK(restored.position.right <= 1000000L);
    CHECK(restored.position.top <= 1000000L);
    CHECK(restored.position.bottom >= -1000000L);

    const Rect bounds = restored.Bounds();
    CHECK(bounds.left < bounds.right);
    CHECK(bounds.top < bounds.bottom);
}

TEST(ObjectListRoundTrip) {
    Document::ObjectList objects;
    objects.emplace_back(ShapeKind::Ellipse, Rect(0, 10, 10, 0), Style{});
    objects.emplace_back(ShapeKind::Arrow, Rect(0, 0, 40, -40), Style{});

    const Json json = ObjectsToJson(objects);
    CHECK_EQ(json.AsArray().size(), std::size_t(2));

    const auto restored = ObjectsFromJson(Json::Parse(json.Dump()));
    CHECK_EQ(restored.size(), std::size_t(2));
    if (restored.size() == 2) {
        CHECK(restored[0].kind == ShapeKind::Ellipse);
        CHECK(restored[1].kind == ShapeKind::Arrow);
        // Ids belong to a document, not to the serialized form.
        CHECK(restored[0].id == ObjectId::None);
    }
}

TEST(ObjectListRejectsNonArrays) {
    CHECK_THROWS(ObjectsFromJson(Json::Parse("{}")), JsonError);
}

TEST(HitTestingUsesTheRealOutline) {
    // A diamond misses its own corners but catches the middle.
    const DrawObj diamond(ShapeKind::Diamond, Rect(-100, 100, 100, -100), Style{});
    CHECK(diamond.HitTest(Point(0, 0), 2));
    CHECK(!diamond.HitTest(Point(-95, 95), 2));

    // A line only hits near the stroke.
    Style style;
    style.lineWidth = 2;
    const DrawObj line(ShapeKind::Line, Rect(0, 0, 100, -100), style);
    CHECK(line.HitTest(Point(50, -50), 4));
    CHECK(!line.HitTest(Point(90, -10), 4));
}

TEST(HitTestingHonoursTheToleranceAtTheEdge) {
    Style style;
    style.lineWidth = 1;
    const DrawObj box(ShapeKind::Rectangle, Rect(0, 100, 100, 0), style);

    // Just outside the outline but within the requested reach.
    CHECK(box.HitTest(Point(-8, 50), 10));
    CHECK(!box.HitTest(Point(-40, 50), 10));
    CHECK(!box.HitTest(Point(50, 400), 10));
}

TEST(SelectionRectangleContainment) {
    const DrawObj object(ShapeKind::Rectangle, Rect(0, 100, 100, 0), Style{});
    CHECK(object.IsWithin(Rect(-10, 110, 110, -10)));
    CHECK(!object.IsWithin(Rect(10, 90, 90, 10)));
}

TEST(SelectionRectangleContainmentForStraightLines) {
    // A horizontal or vertical line has an empty bounding box, which ::UnionRect
    // ignores; it must still be judged by where it actually is.
    const DrawObj horizontal(ShapeKind::Line, Rect(500, 200, 700, 200), Style{});
    const DrawObj vertical(ShapeKind::Arrow, Rect(500, 200, 500, -200), Style{});
    const Rect band(-10, 110, 110, -10);

    CHECK(!horizontal.IsWithin(band));
    CHECK(!vertical.IsWithin(band));

    CHECK(horizontal.IsWithin(Rect(490, 210, 710, 190)));
    CHECK(vertical.IsWithin(Rect(490, 210, 510, -210)));
    // Overlapping the band is not enough; the whole line has to be inside it.
    CHECK(!horizontal.IsWithin(Rect(490, 210, 600, 190)));
}

TEST(ShiftDraggingACornerKeepsASquarePinnedOnTheOppositeCorner) {
    // Document space: y up, so top > bottom.
    const Rect start(0, 100, 200, 0);

    for (int handle : {0, 2, 4, 6}) {
        DrawObj box(ShapeKind::Rectangle, start, Style{});
        const int opposite = (handle + 4) % 8;
        const Point pinned = box.Handle(opposite);

        // Drag the corner well away from the shape, further in x than in y.
        const Point grabbed = box.Handle(handle);
        const Point target(grabbed.x + (grabbed.x == 0 ? -150 : 150),
                           grabbed.y + (grabbed.y == 0 ? -20 : 20));
        box.MoveHandle(handle, SquareCornerTarget(start, handle, target));

        const Rect result = box.position.Normalized();
        CHECK_EQ(result.Width(), result.Height());
        CHECK_EQ(result.Width(), 350L);
        CHECK(box.Handle(opposite) == pinned);
    }

    // Edge handles cannot keep a square; their target passes through untouched.
    CHECK(SquareCornerTarget(start, 3, Point(260, 37)) == Point(260, 37));

    // Creating a shape drags corner 4 out of an empty rectangle at the click.
    const Rect click(Point(10, 10), Size(0, 0));
    CHECK(SquareCornerTarget(click, 4, Point(60, -100)) == Point(120, -100));
}

TEST(BoundsGrowWithTheStrokeAndArrows) {
    Style style;
    style.lineWidth = 4;
    const DrawObj plain(ShapeKind::Rectangle, Rect(0, 100, 100, 0), style);
    CHECK_EQ(plain.Bounds().left, -6L);

    style.endArrow = true;
    const DrawObj arrow(ShapeKind::Arrow, Rect(0, 100, 100, 0), style);
    CHECK(arrow.Bounds().left < plain.Bounds().left);
}

TEST(HandleCounts) {
    const DrawObj box(ShapeKind::Rectangle, Rect(0, 100, 100, 0), Style{});
    CHECK_EQ(box.HandleCount(), 8);
    CHECK(box.Handle(0) == Point(0, 100));
    CHECK(box.Handle(4) == Point(100, 0));

    const DrawObj line(ShapeKind::Arrow, Rect(0, 100, 100, 0), Style{});
    CHECK_EQ(line.HandleCount(), 2);
    CHECK(line.Handle(1) == Point(100, 0));
}

TEST(DocumentZOrder) {
    Document document;
    const ObjectId first = document.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 10, 10, 0), Style{}));
    const ObjectId second = document.Add(DrawObj(ShapeKind::Ellipse, Rect(0, 10, 10, 0), Style{}));

    CHECK(document.Objects().front().id == first);
    document.MoveToFront(first);
    CHECK(document.Objects().back().id == first);
    document.MoveBackward(first);
    CHECK(document.Objects().front().id == first);
    CHECK(document.Objects().back().id == second);
}

TEST(EveryObjectGetsItsOwnId) {
    Document document;
    const ObjectId first = document.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 10, 10, 0), Style{}));
    const ObjectId second = document.Add(DrawObj(ShapeKind::Ellipse, Rect(0, 10, 10, 0), Style{}));

    CHECK(first != ObjectId::None);
    CHECK(first != second);
    CHECK(document.Find(first) != nullptr);
    CHECK(document.Contains(second));

    document.Remove(first);
    CHECK(document.Find(first) == nullptr);
    CHECK(document.Contains(second));
    CHECK(document.Find(ObjectId::None) == nullptr);
}

TEST(IdsSurviveUndoSoASelectionCanBeRestored) {
    Document document;
    const ObjectId kept = document.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 100, 100, 0), Style{}));

    document.BeginChange();
    document.Find(kept)->Offset(Size(50, 0));
    const ObjectId added = document.Add(DrawObj(ShapeKind::Ellipse, Rect(0, 10, 10, 0), Style{}));
    document.CommitChange();

    CHECK(document.Undo());
    // The object that existed before the edit keeps its identity...
    CHECK(document.Contains(kept));
    CHECK(document.Find(kept)->position == Rect(0, 100, 100, 0));
    // ...and the one the edit created is gone rather than merely moved.
    CHECK(!document.Contains(added));

    CHECK(document.Redo());
    CHECK(document.Contains(kept));
    CHECK(document.Contains(added));
}

TEST(AnIdIsNeverReusedAfterUndo) {
    Document document;

    document.BeginChange();
    const ObjectId first = document.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 10, 10, 0), Style{}));
    document.CommitChange();

    CHECK(document.Undo());

    document.BeginChange();
    const ObjectId second = document.Add(DrawObj(ShapeKind::Ellipse, Rect(0, 10, 10, 0), Style{}));
    document.CommitChange();

    // A stale reference to the undone object must not resolve to the new one.
    CHECK(first != second);
    CHECK(!document.Contains(first));
}

TEST(DocumentTopmostObjectWins) {
    Document document;
    document.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 100, 100, 0), Style{}));
    const ObjectId top = document.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 100, 100, 0), Style{}));

    CHECK(document.ObjectAt(Point(50, 50), 2) == top);
    CHECK(document.ObjectAt(Point(500, 500), 2) == ObjectId::None);
}

TEST(UndoAndRedoRestoreTheObjectList) {
    Document document;
    document.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 100, 100, 0), Style{}));
    CHECK(!document.CanUndo());

    document.BeginChange();
    document.Add(DrawObj(ShapeKind::Ellipse, Rect(0, 50, 50, 0), Style{}));
    document.CommitChange();

    CHECK_EQ(document.Objects().size(), std::size_t(2));
    CHECK(document.CanUndo());
    CHECK(!document.CanRedo());

    CHECK(document.Undo());
    CHECK_EQ(document.Objects().size(), std::size_t(1));
    CHECK(document.Objects().front().kind == ShapeKind::Rectangle);
    CHECK(document.CanRedo());

    CHECK(document.Redo());
    CHECK_EQ(document.Objects().size(), std::size_t(2));
    CHECK(document.Objects().back().kind == ShapeKind::Ellipse);

    CHECK(document.Undo());
    CHECK(!document.Undo());
}

TEST(UndoTracksEditsToExistingObjects) {
    Document document;
    const ObjectId id = document.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 100, 100, 0), Style{}));

    document.BeginChange();
    document.Find(id)->Offset(Size(40, -40));
    document.Find(id)->text = L"moved";
    document.CommitChange();

    CHECK(document.Undo());
    CHECK(document.Find(id)->position == Rect(0, 100, 100, 0));
    CHECK(document.Find(id)->text.empty());
}

TEST(AnEditThatChangesNothingLeavesNoHistory) {
    Document document;
    document.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 100, 100, 0), Style{}));

    document.BeginChange();
    document.CommitChange();
    CHECK(!document.CanUndo());

    // Clicking an object without moving it must not become an undo step.
    document.BeginChange();
    document.Objects().front().Offset(Size(0, 0));
    document.CommitChange();
    CHECK(!document.CanUndo());
}

TEST(NestedChangesCollapseIntoOneStep) {
    Document document;

    document.BeginChange();
    document.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 10, 10, 0), Style{}));
    document.BeginChange();
    document.Add(DrawObj(ShapeKind::Ellipse, Rect(0, 10, 10, 0), Style{}));
    document.CommitChange();
    document.CommitChange();

    CHECK_EQ(document.Objects().size(), std::size_t(2));
    CHECK(document.Undo());
    CHECK(document.Objects().empty());
}

TEST(EditingAnObjectInPlaceMarksTheDocumentModified) {
    Document document;
    const ObjectId id = document.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 100, 100, 0), Style{}));
    document.SetModified(false);

    // Dragging, restyling and labelling all mutate the object directly rather
    // than going through the document, so the change bracket has to notice.
    document.BeginChange();
    document.Find(id)->Offset(Size(50, 0));
    document.CommitChange();
    CHECK(document.IsModified());

    document.SetModified(false);
    document.BeginChange();
    document.Find(id)->style.lineWidth = 9;
    document.CommitChange();
    CHECK(document.IsModified());

    document.SetModified(false);
    document.BeginChange();
    document.Find(id)->text = L"label";
    document.CommitChange();
    CHECK(document.IsModified());
}

TEST(AChangeThatDoesNothingLeavesTheDocumentUnmodified) {
    Document document;
    document.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 100, 100, 0), Style{}));
    document.SetModified(false);

    document.BeginChange();
    document.CommitChange();

    CHECK(!document.IsModified());
    CHECK(!document.CanUndo());
}

TEST(UndoIsRefusedWhileAChangeIsInProgress) {
    Document document;
    document.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 100, 100, 0), Style{}));

    document.BeginChange();
    document.Add(DrawObj(ShapeKind::Ellipse, Rect(0, 10, 10, 0), Style{}));
    document.CommitChange();
    CHECK(document.CanUndo());

    // A drag in progress owns the document; undoing underneath it would leave
    // the view holding pointers into a replaced object list.
    document.BeginChange();
    CHECK(!document.Undo());
    CHECK(!document.Redo());
    document.CommitChange();

    CHECK(document.Undo());
    CHECK_EQ(document.Objects().size(), std::size_t(1));
}

TEST(LoadingClearsTheHistory) {
    const auto path = TempFile("fast-draw-test-history.fdr");

    Document saved;
    saved.Add(DrawObj(ShapeKind::Rectangle, Rect(0, 10, 10, 0), Style{}));
    CHECK(saved.Save(nullptr, path));

    Document document;
    document.BeginChange();
    document.Add(DrawObj(ShapeKind::Ellipse, Rect(0, 10, 10, 0), Style{}));
    document.CommitChange();
    CHECK(document.CanUndo());

    CHECK(document.Load(nullptr, path));
    CHECK(!document.CanUndo());
    CHECK(!document.CanRedo());

    std::filesystem::remove(path);
}

TEST(LoadingCapsAnAbsurdPageSize) {
    // A damaged file asking for a page 300 km wide used to hang the first paint
    // drawing its grid.
    const auto path = TempFile("fast-draw-test-huge-page.fdr");
    WriteFileText(path, R"({"format":"fast-draw","version":2,)"
                        R"("page":{"width":2000000000,"height":2000000000},"objects":[]})");

    Document document;
    CHECK(document.Load(nullptr, path));
    CHECK(document.PageSize().cx > 0 && document.PageSize().cx <= 10000);
    CHECK(document.PageSize().cy > 0 && document.PageSize().cy <= 10000);

    // A real paper size comes through untouched.
    WriteFileText(path, R"({"format":"fast-draw","version":2,)"
                        R"("page":{"width":1654,"height":1169},"objects":[]})");
    Document a3;
    CHECK(a3.Load(nullptr, path));
    CHECK(a3.PageSize() == Size(1654, 1169));

    std::filesystem::remove(path);
}

TEST(StyleDeltaOnlyCopiesWhatChanged) {
    Style before;
    Style after = before;
    after.lineWidth = 9;

    Style target;
    target.fill = RGB(1, 2, 3);
    target.line = RGB(4, 5, 6);
    target.lineWidth = 2;

    ApplyStyleDelta(before, after, target);

    CHECK_EQ(target.lineWidth, 9);
    // The rest of the target's own styling survives.
    CHECK_EQ(target.fill, RGB(1, 2, 3));
    CHECK_EQ(target.line, RGB(4, 5, 6));
}

TEST(StyleDeltaCopiesEveryChangedField) {
    Style before;
    Style after = before;
    after.hasFill = !before.hasFill;
    after.gradient = !before.gradient;
    after.gradientDirection = GradientDirection::UpRight;
    after.dash = DashStyle::Dot;
    after.endArrow = !before.endArrow;
    after.fontSize = before.fontSize + 5;
    after.italic = !before.italic;

    Style target;
    ApplyStyleDelta(before, after, target);

    CHECK_EQ(target.hasFill, after.hasFill);
    CHECK_EQ(target.gradient, after.gradient);
    CHECK(target.gradientDirection == GradientDirection::UpRight);
    CHECK(target.dash == DashStyle::Dot);
    CHECK_EQ(target.endArrow, after.endArrow);
    CHECK_EQ(target.fontSize, after.fontSize);
    CHECK_EQ(target.italic, after.italic);
}

TEST(FileTextRoundTrip) {
    const auto path = TempFile("fast-draw-test-file.json");
    const std::string content = R"({"format":"fast-draw","text":"caf\u00e9"})";

    WriteFileText(path, content);
    CHECK_EQ(ReadFileText(path), content);

    WriteFileText(path, "{}");
    CHECK_EQ(ReadFileText(path), std::string("{}"));

    std::filesystem::remove(path);
}

TEST(ReadingAMissingFileThrows) {
    CHECK_THROWS(ReadFileText(TempFile("fast-draw-does-not-exist.json")), FileError);
}

TEST(DocumentSaveAndLoad) {
    const auto path = TempFile("fast-draw-test-doc.fdr");

    Document saved;
    saved.SetPaperColour(RGB(0xF0, 0xF0, 0xE0));
    DrawObj object(ShapeKind::Hexagon, Rect(-50, 25, 50, -25), Style{});
    object.text = L"Gateway";
    saved.Add(std::move(object));
    CHECK(saved.Save(nullptr, path));
    CHECK(!saved.IsModified());

    Document loaded;
    CHECK(loaded.Load(nullptr, path));
    CHECK_EQ(loaded.Objects().size(), std::size_t(1));
    CHECK_EQ(loaded.PaperColour(), RGB(0xF0, 0xF0, 0xE0));
    if (!loaded.Objects().empty()) {
        CHECK(loaded.Objects().front().kind == ShapeKind::Hexagon);
        CHECK_EQ(loaded.Objects().front().text, std::wstring(L"Gateway"));
        // Loading assigns fresh ids so nothing refers to the previous document.
        CHECK(loaded.Objects().front().id != ObjectId::None);
    }

    std::filesystem::remove(path);
}
