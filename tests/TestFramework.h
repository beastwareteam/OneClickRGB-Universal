#pragma once
//=============================================================================
// OneClickRGB-Universal - Minimal Test Framework
//=============================================================================
// Lightweight test framework without external dependencies.
// Inspired by doctest/catch2 but much simpler.
//
// Usage:
//   TEST_CASE("name") {
//       CHECK(1 + 1 == 2);
//       CHECK_EQ(value, expected);
//       CHECK_NE(value, unexpected);
//   }
//
//   int main() { return TestRunner::Run(); }
//
//=============================================================================

#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <sstream>

namespace OCRGB {
namespace Test {

//-----------------------------------------------------------------------------
// Test Result
//-----------------------------------------------------------------------------
struct TestResult {
    bool passed = true;
    std::string message;
};

//-----------------------------------------------------------------------------
// Test Case
//-----------------------------------------------------------------------------
struct TestCase {
    std::string name;
    std::function<TestResult()> func;
};

//-----------------------------------------------------------------------------
// Test Registry (singleton)
//-----------------------------------------------------------------------------
class TestRegistry {
public:
    static TestRegistry& Instance() {
        static TestRegistry instance;
        return instance;
    }

    void Register(const std::string& name, std::function<TestResult()> func) {
        m_tests.push_back({name, func});
    }

    const std::vector<TestCase>& GetTests() const { return m_tests; }

private:
    std::vector<TestCase> m_tests;
};

//-----------------------------------------------------------------------------
// Test Runner
//-----------------------------------------------------------------------------
class TestRunner {
public:
    static int Run() {
        auto& tests = TestRegistry::Instance().GetTests();

        int passed = 0;
        int failed = 0;

        std::cout << "Running " << tests.size() << " test(s)...\n\n";

        for (const auto& test : tests) {
            std::cout << "[ RUN  ] " << test.name << "\n";

            try {
                TestResult result = test.func();

                if (result.passed) {
                    std::cout << "[ PASS ] " << test.name << "\n";
                    passed++;
                } else {
                    std::cout << "[ FAIL ] " << test.name << "\n";
                    std::cout << "         " << result.message << "\n";
                    failed++;
                }
            } catch (const std::exception& e) {
                std::cout << "[ FAIL ] " << test.name << "\n";
                std::cout << "         Exception: " << e.what() << "\n";
                failed++;
            } catch (...) {
                std::cout << "[ FAIL ] " << test.name << "\n";
                std::cout << "         Unknown exception\n";
                failed++;
            }
        }

        std::cout << "\n========================================\n";
        std::cout << "Passed: " << passed << " / " << (passed + failed) << "\n";

        if (failed > 0) {
            std::cout << "FAILED: " << failed << " test(s)\n";
            return 1;
        }

        std::cout << "All tests passed!\n";
        return 0;
    }
};

//-----------------------------------------------------------------------------
// Auto-registration helper
//-----------------------------------------------------------------------------
struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<TestResult()> func) {
        TestRegistry::Instance().Register(name, func);
    }
};

//-----------------------------------------------------------------------------
// Assertion helpers
//-----------------------------------------------------------------------------
inline TestResult MakePass() {
    return {true, ""};
}

inline TestResult MakeFail(const std::string& msg) {
    return {false, msg};
}

template<typename T>
std::string ToString(const T& value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

} // namespace Test
} // namespace OCRGB

//=============================================================================
// Macros
//=============================================================================

#define TEST_CASE(name) \
    static ::OCRGB::Test::TestResult _test_func_##__LINE__(); \
    static ::OCRGB::Test::TestRegistrar _test_reg_##__LINE__(name, _test_func_##__LINE__); \
    static ::OCRGB::Test::TestResult _test_func_##__LINE__()

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            return ::OCRGB::Test::MakeFail("CHECK failed: " #expr); \
        } \
    } while(0)

#define CHECK_EQ(a, b) \
    do { \
        if (!((a) == (b))) { \
            return ::OCRGB::Test::MakeFail( \
                "CHECK_EQ failed: " #a " == " #b \
                " (got " + ::OCRGB::Test::ToString(a) + " vs " + ::OCRGB::Test::ToString(b) + ")"); \
        } \
    } while(0)

#define CHECK_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            return ::OCRGB::Test::MakeFail("CHECK_NE failed: " #a " != " #b); \
        } \
    } while(0)

#define CHECK_TRUE(expr) CHECK(expr)
#define CHECK_FALSE(expr) CHECK(!(expr))

#define PASS() return ::OCRGB::Test::MakePass()
