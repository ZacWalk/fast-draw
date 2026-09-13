// A very small assertion harness: no external test framework needed.
#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace testing {

inline int g_checks = 0;
inline int g_failures = 0;
inline const char* g_currentTest = "";

inline void ReportFailure(const char* file, int line, std::string_view message) {
    ++g_failures;
    std::printf("  FAIL %s\n    %s(%d): %.*s\n", g_currentTest, file, line,
                static_cast<int>(message.size()), message.data());
}

inline void Check(bool condition, const char* file, int line, std::string_view expression) {
    ++g_checks;
    if (!condition) ReportFailure(file, line, expression);
}

template <typename A, typename B>
void CheckEqual(const A& actual, const B& expected, const char* file, int line,
                std::string_view expression) {
    ++g_checks;
    if (!(actual == expected)) {
        ReportFailure(file, line, std::string(expression) + " -- values differ");
    }
}

struct TestCase {
    const char* name;
    void (*run)();
};

inline std::vector<TestCase>& Registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const char* name, void (*run)()) { Registry().push_back({name, run}); }
};

}  // namespace testing

#define TEST(name)                                                     \
    static void name();                                                \
    static testing::Registrar registrar_##name(#name, &name);          \
    static void name()

#define CHECK(expr) testing::Check((expr), __FILE__, __LINE__, #expr)
#define CHECK_EQ(actual, expected) \
    testing::CheckEqual((actual), (expected), __FILE__, __LINE__, #actual " == " #expected)
#define CHECK_THROWS(expr, exception_type)                                            \
    do {                                                                              \
        ++testing::g_checks;                                                          \
        bool threw = false;                                                           \
        try {                                                                         \
            (void)(expr);                                                             \
        } catch (const exception_type&) {                                             \
            threw = true;                                                             \
        } catch (...) {                                                               \
        }                                                                             \
        if (!threw)                                                                   \
            testing::ReportFailure(__FILE__, __LINE__, #expr " did not throw " #exception_type); \
    } while (false)
