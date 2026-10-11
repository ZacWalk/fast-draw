#include "test_framework.h"

#include "json.h"
#include "utf8.h"

TEST(ParsesPrimitives) {
    CHECK(Json::Parse("null").IsNull());
    CHECK_EQ(Json::Parse("true").AsBool(), true);
    CHECK_EQ(Json::Parse("false").AsBool(), false);
    CHECK_EQ(Json::Parse("42").AsInt(), 42L);
    CHECK_EQ(Json::Parse("-17").AsInt(), -17L);
    CHECK_EQ(Json::Parse("3.5").AsNumber(), 3.5);
    CHECK_EQ(Json::Parse("1e3").AsNumber(), 1000.0);
    CHECK_EQ(Json::Parse("-2.5e-2").AsNumber(), -0.025);
    CHECK_EQ(Json::Parse("\"hello\"").AsString(), std::string("hello"));
}

TEST(IgnoresSurroundingWhitespaceAndBom) {
    CHECK_EQ(Json::Parse("  \r\n\t 7 \n").AsInt(), 7L);
    CHECK_EQ(Json::Parse("\xEF\xBB\xBF{\"a\":1}")["a"].AsInt(), 1L);
}

TEST(ParsesNestedContainers) {
    const Json json = Json::Parse(R"({"a":[1,2,{"b":"c"}],"d":{"e":[]}})");

    CHECK(json.IsObject());
    CHECK_EQ(json["a"].AsArray().size(), std::size_t(3));
    CHECK_EQ(json["a"].AsArray()[1].AsInt(), 2L);
    CHECK_EQ(json["a"].AsArray()[2]["b"].AsString(), std::string("c"));
    CHECK(json["d"]["e"].IsArray());
    CHECK(json["d"]["e"].AsArray().empty());
}

TEST(ParsesEmptyContainers) {
    CHECK(Json::Parse("[]").IsArray());
    CHECK(Json::Parse("{}").IsObject());
    CHECK(Json::Parse("[ ]").AsArray().empty());
    CHECK(Json::Parse("{ }").AsObject().empty());
}

TEST(ParsesStringEscapes) {
    CHECK_EQ(Json::Parse(R"("a\"b")").AsString(), std::string("a\"b"));
    CHECK_EQ(Json::Parse(R"("a\\b")").AsString(), std::string("a\\b"));
    CHECK_EQ(Json::Parse(R"("a\/b")").AsString(), std::string("a/b"));
    CHECK_EQ(Json::Parse(R"("\b\f\n\r\t")").AsString(), std::string("\b\f\n\r\t"));
    CHECK_EQ(Json::Parse(R"("\u0041")").AsString(), std::string("A"));
    // U+00E9 encodes as two UTF-8 bytes.
    CHECK_EQ(Json::Parse(R"("\u00e9")").AsString(), std::string("\xC3\xA9"));
    // A surrogate pair for U+1F600 encodes as four UTF-8 bytes.
    CHECK_EQ(Json::Parse(R"("\ud83d\ude00")").AsString(), std::string("\xF0\x9F\x98\x80"));
}

TEST(RejectsMalformedInput) {
    CHECK_THROWS(Json::Parse(""), JsonError);
    CHECK_THROWS(Json::Parse("{"), JsonError);
    CHECK_THROWS(Json::Parse("[1,2"), JsonError);
    CHECK_THROWS(Json::Parse("{\"a\" 1}"), JsonError);
    CHECK_THROWS(Json::Parse("nul"), JsonError);
    CHECK_THROWS(Json::Parse(R"("bad \q escape")"), JsonError);
    CHECK_THROWS(Json::Parse("\"unterminated"), JsonError);
    CHECK_THROWS(Json::Parse("1 2"), JsonError);
    CHECK_THROWS(Json::Parse("[1,]"), JsonError);
}

TEST(RejectsExcessiveNesting) {
    // The parser caps recursion depth so hostile input cannot exhaust the stack.
    const std::string deep = std::string(5000, '[') + std::string(5000, ']');
    CHECK_THROWS(Json::Parse(deep), JsonError);
}

TEST(WritesCompactAndPretty) {
    Json json = Json::MakeObject();
    json.Set("b", Json(1));
    json.Set("a", Json(std::string("x")));

    CHECK_EQ(json.Dump(0), std::string(R"({"b":1,"a":"x"})"));
    CHECK_EQ(json.Dump(2), std::string("{\n  \"b\": 1,\n  \"a\": \"x\"\n}\n"));
}

TEST(PreservesKeyInsertionOrder) {
    Json json = Json::MakeObject();
    json.Set("zebra", Json(1));
    json.Set("apple", Json(2));
    json.Set("mango", Json(3));

    const auto& members = json.AsObject();
    CHECK_EQ(members.size(), std::size_t(3));
    CHECK_EQ(members[0].first, std::string("zebra"));
    CHECK_EQ(members[1].first, std::string("apple"));
    CHECK_EQ(members[2].first, std::string("mango"));
}

TEST(SetOverwritesExistingKeyInPlace) {
    Json json = Json::MakeObject();
    json.Set("a", Json(1));
    json.Set("b", Json(2));
    json.Set("a", Json(9));

    CHECK_EQ(json.AsObject().size(), std::size_t(2));
    CHECK_EQ(json["a"].AsInt(), 9L);
    CHECK_EQ(json.AsObject()[0].first, std::string("a"));
}

TEST(EscapesOnWrite) {
    Json json = Json(std::string("quote\" slash\\ newline\n tab\t control\x01"));
    CHECK_EQ(json.Dump(0),
             std::string(R"("quote\" slash\\ newline\n tab\t control\u0001")"));
}

TEST(RoundTripsThroughText) {
    const std::string original =
        R"({"n":-1.5,"s":"\u00e9x","a":[true,false,null],"o":{"deep":[1,2,3]}})";

    const Json first = Json::Parse(original);
    const Json second = Json::Parse(first.Dump(0));

    CHECK_EQ(second.Dump(0), first.Dump(0));
    CHECK_EQ(second["n"].AsNumber(), -1.5);
    CHECK_EQ(second["o"]["deep"].AsArray().size(), std::size_t(3));
}

TEST(MissingKeysYieldNullAndFallbacks) {
    const Json json = Json::Parse(R"({"a":1})");

    CHECK(json["missing"].IsNull());
    CHECK(json["missing"]["nested"].IsNull());
    CHECK_EQ(json["missing"].AsInt(7), 7L);
    CHECK_EQ(json["missing"].AsString("fallback"), std::string("fallback"));
    CHECK_EQ(json["missing"].AsBool(true), true);
    // Wrong-typed values fall back rather than throwing.
    CHECK_EQ(json["a"].AsString("fallback"), std::string("fallback"));
}

TEST(WritesWholeNumbersWithoutDecimalPoint) {
    CHECK_EQ(Json(1000).Dump(0), std::string("1000"));
    CHECK_EQ(Json(-42).Dump(0), std::string("-42"));
    CHECK_EQ(Json(0).Dump(0), std::string("0"));
}

TEST(Utf8RoundTrip) {
    const std::wstring original = L"plain ASCII";
    CHECK_EQ(Utf8Decode(Utf8Encode(original)), original);

    const std::wstring accented = L"caf\u00e9 na\u00efve";
    CHECK_EQ(Utf8Decode(Utf8Encode(accented)), accented);

    // U+1F600, stored as a surrogate pair in UTF-16.
    const std::wstring emoji = L"\xD83D\xDE00";
    CHECK_EQ(Utf8Encode(emoji), std::string("\xF0\x9F\x98\x80"));
    CHECK_EQ(Utf8Decode(Utf8Encode(emoji)), emoji);

    CHECK(Utf8Encode(L"").empty());
    CHECK(Utf8Decode("").empty());
}

TEST(CaretStopsNeverSplitASurrogatePair) {
    // "a", U+1F600 as a surrogate pair, "b": four UTF-16 units, three characters.
    const std::wstring text = L"a\xD83D\xDE00" L"b";

    CHECK(IsCaretStop(text, 0));
    CHECK(IsCaretStop(text, 1));
    CHECK(!IsCaretStop(text, 2));
    CHECK(IsCaretStop(text, 3));
    CHECK(IsCaretStop(text, 4));
    CHECK(!IsCaretStop(text, 5));

    // Backspace and Left from after the emoji land before it, not inside it.
    CHECK_EQ(PreviousCaretStop(text, 3), std::size_t(1));
    CHECK_EQ(PreviousCaretStop(text, 4), std::size_t(3));
    CHECK_EQ(PreviousCaretStop(text, 1), std::size_t(0));
    CHECK_EQ(PreviousCaretStop(text, 0), std::size_t(0));

    // Delete and Right from before it step over both halves.
    CHECK_EQ(NextCaretStop(text, 1), std::size_t(3));
    CHECK_EQ(NextCaretStop(text, 0), std::size_t(1));
    CHECK_EQ(NextCaretStop(text, 3), std::size_t(4));
    CHECK_EQ(NextCaretStop(text, 4), std::size_t(4));

    // An unpaired half is still a character of its own, so it can be removed.
    const std::wstring lone = L"x\xD83D";
    CHECK_EQ(PreviousCaretStop(lone, 2), std::size_t(1));
    CHECK_EQ(NextCaretStop(lone, 1), std::size_t(2));
}

TEST(JsonCarriesNonAsciiText) {
    const std::wstring original = L"caf\u00e9 \u4f60\u597d";

    Json json = Json::MakeObject();
    json.Set("text", Json(Utf8Encode(original)));

    const Json parsed = Json::Parse(json.Dump(0));
    CHECK_EQ(Utf8Decode(parsed["text"].AsString()), original);
}
