// A small self-contained JSON value, parser and writer.
//
// Objects keep their keys in insertion order so saved documents have a stable,
// readable field order. Lookup is linear, which is fine at this scale.

#pragma once

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

class JsonError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Json {
public:
    using Array = std::vector<Json>;
    using Member = std::pair<std::string, Json>;
    using Object = std::vector<Member>;

    Json() = default;
    Json(std::nullptr_t) {}
    Json(bool value) : value_(value) {}
    Json(double value) : value_(value) {}
    Json(int value) : value_(static_cast<double>(value)) {}
    Json(long value) : value_(static_cast<double>(value)) {}
    Json(unsigned value) : value_(static_cast<double>(value)) {}
    Json(std::string value) : value_(std::move(value)) {}
    Json(std::string_view value) : value_(std::string(value)) {}
    Json(const char* value) : value_(std::string(value)) {}
    Json(Array value) : value_(std::move(value)) {}
    Json(Object value) : value_(std::move(value)) {}

    static Json MakeArray() { return Json(Array{}); }
    static Json MakeObject() { return Json(Object{}); }

    bool IsNull() const { return std::holds_alternative<std::monostate>(value_); }
    bool IsBool() const { return std::holds_alternative<bool>(value_); }
    bool IsNumber() const { return std::holds_alternative<double>(value_); }
    bool IsString() const { return std::holds_alternative<std::string>(value_); }
    bool IsArray() const { return std::holds_alternative<Array>(value_); }
    bool IsObject() const { return std::holds_alternative<Object>(value_); }

    // Lenient readers: they return the fallback when the type does not match,
    // so a hand-edited or truncated document still loads.
    bool AsBool(bool fallback = false) const;
    double AsNumber(double fallback = 0.0) const;
    long AsInt(long fallback = 0) const;
    std::string AsString(std::string_view fallback = {}) const;

    // Empty when the value is not of that type.
    const Array& AsArray() const;
    const Object& AsObject() const;

    // Returns null when absent or when this is not an object.
    const Json& operator[](std::string_view key) const;

    void Set(std::string key, Json value);
    void Push(Json value);

    // `indent` of 0 writes the compact form.
    std::string Dump(int indent = 2) const;

    // Throws JsonError on malformed input or excessive nesting.
    static Json Parse(std::string_view text);

private:
    void Write(std::string& out, int indent, int depth) const;

    std::variant<std::monostate, bool, double, std::string, Array, Object> value_;
};
