//=============================================================================
// OneClickRGB-Universal - Test Runner
//=============================================================================
// Compiles and runs all tests.
//
// Build: cl /EHsc /I.. tests/test_main.cpp tests/test_*.cpp src/app/...
// Run:   test_runner.exe
//
//=============================================================================

#include "TestFramework.h"

// Include all test files
#include "test_types.cpp"
#include "test_registry.cpp"
#include "test_effects.cpp"
#include "test_config.cpp"
#include "test_bundle_parser.cpp"
#include "test_dryrun.cpp"
#include "test_platform.cpp"

int main() {
    std::cout << "========================================\n";
    std::cout << "OneClickRGB-Universal Test Suite\n";
    std::cout << "========================================\n\n";

    return OCRGB::Test::TestRunner::Run();
}
