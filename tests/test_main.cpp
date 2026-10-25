#include "test_framework.h"

#include <cstdio>

int main() {
    auto& tests = testing::Registry();
    std::printf("running %zu tests\n", tests.size());

    for (const auto& test : tests) {
        testing::g_currentTest = test.name;
        const int before = testing::g_failures;
        try {
            test.run();
        } catch (const std::exception& error) {
            testing::ReportFailure(__FILE__, __LINE__,
                                   std::string("unexpected exception: ") + error.what());
        }
        if (testing::g_failures == before) std::printf("  ok   %s\n", test.name);
    }

    std::printf("\n%d checks, %d failures\n", testing::g_checks, testing::g_failures);
    return testing::g_failures == 0 ? 0 : 1;
}
