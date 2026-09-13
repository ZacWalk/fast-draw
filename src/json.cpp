#include "json.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

// Guards against stack exhaustion from deeply nested (or hostile) input.
constexpr int kMaxDepth = 100;

const Json kNullValue;

void AppendCodepoint(std::string& out, unsigned codepoint) {
    if (codepoint <= 0x7F) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}

void EscapeString(std::string& out, std::string_view text) {
    out.push_back('"');
    for (const char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buffer[8];
                    std::snprintf(buffer, sizeof(buffer), "\\u%04x",
                                  static_cast<unsigned>(static_cast<unsigned char>(c)));
                    out += buffer;
                } else {
                    out.push_back(c);
                }
                break;
        }
    }
    out.push_back('"');
}

void AppendNumber(std::string& out, double value) {
    if (!std::isfinite(value)) {
        out += "0";
        return;
    }

    // Whole numbers are written without a decimal point; everything else gets
    // enough digits to round-trip.
    double integral = 0.0;
    if (std::modf(value, &integral) == 0.0 && std::fabs(value) < 1e15) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%lld", static_cast<long long>(integral));
        out += buffer;
        return;
    }

    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%.17g", value);
    out += buffer;
}

class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    Json ParseDocument() {
        SkipWhitespace();
        Json value = ParseValue(0);
        SkipWhitespace();
        if (position_ != text_.size()) Fail("trailing characters after the JSON value");
        return value;
    }

private:
    [[noreturn]] void Fail(std::string_view message) const {
        throw JsonError(std::string(message) + " at offset " + std::to_string(position_));
    }

    bool AtEnd() const { return position_ >= text_.size(); }
    char Peek() const { return AtEnd() ? '\0' : text_[position_]; }

    char Take() {
        if (AtEnd()) Fail("unexpected end of input");
        return text_[position_++];
    }

    void Expect(char expected) {
        if (Peek() != expected) Fail(std::string("expected '") + expected + "'");
        ++position_;
    }

    void SkipWhitespace() {
        while (!AtEnd()) {
            const char c = text_[position_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                ++position_;
            else
                break;
        }
    }

    void ExpectLiteral(std::string_view literal) {
        if (text_.substr(position_, literal.size()) != literal)
            Fail(std::string("invalid literal, expected '") + std::string(literal) + "'");
        position_ += literal.size();
    }

    unsigned ParseHex4() {
        if (position_ + 4 > text_.size()) Fail("truncated \\u escape");
        unsigned result = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = text_[position_++];
            result <<= 4;
            if (c >= '0' && c <= '9')
                result |= static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f')
                result |= static_cast<unsigned>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F')
                result |= static_cast<unsigned>(c - 'A' + 10);
            else
                Fail("invalid hex digit in \\u escape");
        }
        return result;
    }

    std::string ParseString() {
        Expect('"');
        std::string result;

        for (;;) {
            const char c = Take();
            if (c == '"') break;

            if (static_cast<unsigned char>(c) < 0x20) Fail("unescaped control character in string");

            if (c != '\\') {
                result.push_back(c);
                continue;
            }

            switch (Take()) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                case 'u': {
                    unsigned codepoint = ParseHex4();
                    // Combine a surrogate pair into a single code point.
                    if (codepoint >= 0xD800 && codepoint <= 0xDBFF && position_ + 1 < text_.size() &&
                        text_[position_] == '\\' && text_[position_ + 1] == 'u') {
                        const std::size_t mark = position_;
                        position_ += 2;
                        const unsigned low = ParseHex4();
                        if (low >= 0xDC00 && low <= 0xDFFF)
                            codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                        else
                            position_ = mark;
                    }
                    AppendCodepoint(result, codepoint);
                    break;
                }
                default: Fail("invalid escape sequence");
            }
        }

        return result;
    }

    double ParseNumber() {
        const std::size_t start = position_;

        if (Peek() == '-') ++position_;
        while (!AtEnd() && text_[position_] >= '0' && text_[position_] <= '9') ++position_;
        if (!AtEnd() && text_[position_] == '.') {
            ++position_;
            while (!AtEnd() && text_[position_] >= '0' && text_[position_] <= '9') ++position_;
        }
        if (!AtEnd() && (text_[position_] == 'e' || text_[position_] == 'E')) {
            ++position_;
            if (!AtEnd() && (text_[position_] == '+' || text_[position_] == '-')) ++position_;
            while (!AtEnd() && text_[position_] >= '0' && text_[position_] <= '9') ++position_;
        }

        if (position_ == start) Fail("expected a number");

        const std::string digits(text_.substr(start, position_ - start));
        char* end = nullptr;
        const double result = std::strtod(digits.c_str(), &end);
        if (end != digits.c_str() + digits.size()) Fail("malformed number");
        return result;
    }

    Json ParseValue(int depth) {
        if (depth > kMaxDepth) Fail("nesting is too deep");

        SkipWhitespace();
        switch (Peek()) {
            case 'n': ExpectLiteral("null"); return Json();
            case 't': ExpectLiteral("true"); return Json(true);
            case 'f': ExpectLiteral("false"); return Json(false);
            case '"': return Json(ParseString());
            case '[': return ParseArray(depth);
            case '{': return ParseObject(depth);
            default: return Json(ParseNumber());
        }
    }

    Json ParseArray(int depth) {
        Expect('[');
        Json::Array items;

        SkipWhitespace();
        if (Peek() == ']') {
            ++position_;
            return Json(std::move(items));
        }

        for (;;) {
            items.push_back(ParseValue(depth + 1));
            SkipWhitespace();
            const char c = Take();
            if (c == ']') break;
            if (c != ',') Fail("expected ',' or ']' in array");
        }

        return Json(std::move(items));
    }

    Json ParseObject(int depth) {
        Expect('{');
        Json::Object members;

        SkipWhitespace();
        if (Peek() == '}') {
            ++position_;
            return Json(std::move(members));
        }

        for (;;) {
            SkipWhitespace();
            std::string key = ParseString();
            SkipWhitespace();
            Expect(':');
            members.emplace_back(std::move(key), ParseValue(depth + 1));

            SkipWhitespace();
            const char c = Take();
            if (c == '}') break;
            if (c != ',') Fail("expected ',' or '}' in object");
        }

        return Json(std::move(members));
    }

    std::string_view text_;
    std::size_t position_ = 0;
};

}  // namespace

bool Json::AsBool(bool fallback) const {
    if (const auto* value = std::get_if<bool>(&value_)) return *value;
    return fallback;
}

double Json::AsNumber(double fallback) const {
    if (const auto* value = std::get_if<double>(&value_)) return *value;
    return fallback;
}

long Json::AsInt(long fallback) const {
    if (const auto* value = std::get_if<double>(&value_)) return static_cast<long>(std::lround(*value));
    return fallback;
}

std::string Json::AsString(std::string_view fallback) const {
    if (const auto* value = std::get_if<std::string>(&value_)) return *value;
    return std::string(fallback);
}

const Json::Array& Json::AsArray() const {
    static const Array empty;
    if (const auto* value = std::get_if<Array>(&value_)) return *value;
    return empty;
}

const Json::Object& Json::AsObject() const {
    static const Object empty;
    if (const auto* value = std::get_if<Object>(&value_)) return *value;
    return empty;
}

const Json& Json::operator[](std::string_view key) const {
    if (const auto* object = std::get_if<Object>(&value_)) {
        for (const auto& member : *object) {
            if (member.first == key) return member.second;
        }
    }
    return kNullValue;
}

void Json::Set(std::string key, Json value) {
    if (!IsObject()) value_ = Object{};

    auto& object = std::get<Object>(value_);
    for (auto& member : object) {
        if (member.first == key) {
            member.second = std::move(value);
            return;
        }
    }
    object.emplace_back(std::move(key), std::move(value));
}

void Json::Push(Json value) {
    if (!IsArray()) value_ = Array{};
    std::get<Array>(value_).push_back(std::move(value));
}

void Json::Write(std::string& out, int indent, int depth) const {
    const bool pretty = indent > 0;
    const std::string childPad = pretty ? std::string(static_cast<std::size_t>(indent) * (depth + 1), ' ') : std::string();
    const std::string closePad = pretty ? std::string(static_cast<std::size_t>(indent) * depth, ' ') : std::string();

    if (IsNull()) {
        out += "null";
    } else if (const auto* boolean = std::get_if<bool>(&value_)) {
        out += *boolean ? "true" : "false";
    } else if (const auto* number = std::get_if<double>(&value_)) {
        AppendNumber(out, *number);
    } else if (const auto* text = std::get_if<std::string>(&value_)) {
        EscapeString(out, *text);
    } else if (const auto* array = std::get_if<Array>(&value_)) {
        if (array->empty()) {
            out += "[]";
            return;
        }
        out += '[';
        for (std::size_t i = 0; i < array->size(); ++i) {
            if (i != 0) out += ',';
            if (pretty) {
                out += '\n';
                out += childPad;
            }
            (*array)[i].Write(out, indent, depth + 1);
        }
        if (pretty) {
            out += '\n';
            out += closePad;
        }
        out += ']';
    } else if (const auto* object = std::get_if<Object>(&value_)) {
        if (object->empty()) {
            out += "{}";
            return;
        }
        out += '{';
        for (std::size_t i = 0; i < object->size(); ++i) {
            if (i != 0) out += ',';
            if (pretty) {
                out += '\n';
                out += childPad;
            }
            EscapeString(out, (*object)[i].first);
            out += pretty ? ": " : ":";
            (*object)[i].second.Write(out, indent, depth + 1);
        }
        if (pretty) {
            out += '\n';
            out += closePad;
        }
        out += '}';
    }
}

std::string Json::Dump(int indent) const {
    std::string out;
    Write(out, indent, 0);
    if (indent > 0) out += '\n';
    return out;
}

Json Json::Parse(std::string_view text) {
    // Skip a UTF-8 byte order mark if one is present.
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB && static_cast<unsigned char>(text[2]) == 0xBF)
        text.remove_prefix(3);

    return Parser(text).ParseDocument();
}
