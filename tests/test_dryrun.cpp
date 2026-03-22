//=============================================================================
// Tests for core/DryRunMode
//=============================================================================

#include "TestFramework.h"
#include "../src/core/DryRunMode.h"

using namespace OCRGB;

//-----------------------------------------------------------------------------
// Basic Enable/Disable Tests
//-----------------------------------------------------------------------------

TEST_CASE("DryRun: disabled by default") {
    DryRun::Disable();  // Reset state
    CHECK(!DryRun::IsEnabled());
    PASS();
}

TEST_CASE("DryRun: enable and disable") {
    DryRun::Disable();

    DryRun::Enable();
    CHECK(DryRun::IsEnabled());

    DryRun::Disable();
    CHECK(!DryRun::IsEnabled());
    PASS();
}

//-----------------------------------------------------------------------------
// Logging Tests
//-----------------------------------------------------------------------------

TEST_CASE("DryRun: log when enabled") {
    DryRun::Enable();
    DryRun::ClearLog();

    DryRun::Log("TestComponent", "TestOp", "test details");

    auto entries = DryRun::GetLogEntries();
    CHECK_EQ(entries.size(), 1u);
    CHECK_EQ(entries[0].component, "TestComponent");
    CHECK_EQ(entries[0].operation, "TestOp");
    CHECK_EQ(entries[0].details, "test details");

    DryRun::Disable();
    PASS();
}

TEST_CASE("DryRun: no log when disabled") {
    DryRun::Disable();
    DryRun::ClearLog();

    DryRun::Log("TestComponent", "TestOp", "should not appear");

    CHECK_EQ(DryRun::GetLogCount(), 0u);
    PASS();
}

TEST_CASE("DryRun: clear log") {
    DryRun::Enable();

    DryRun::Log("A", "B", "C");
    DryRun::Log("D", "E", "F");
    CHECK_EQ(DryRun::GetLogCount(), 2u);

    DryRun::ClearLog();
    CHECK_EQ(DryRun::GetLogCount(), 0u);

    DryRun::Disable();
    PASS();
}

TEST_CASE("DryRun: log has timestamp") {
    DryRun::Enable();
    DryRun::ClearLog();

    DryRun::Log("Test", "Op", "");

    auto entries = DryRun::GetLogEntries();
    CHECK_EQ(entries.size(), 1u);
    CHECK(!entries[0].timestamp.empty());
    // Timestamp should contain colons (HH:MM:SS format)
    CHECK(entries[0].timestamp.find(':') != std::string::npos);

    DryRun::Disable();
    PASS();
}

//-----------------------------------------------------------------------------
// Scoped Dry Run Tests
//-----------------------------------------------------------------------------

TEST_CASE("DryRun::Scope: enables within scope") {
    DryRun::Disable();
    CHECK(!DryRun::IsEnabled());

    {
        DryRun::Scope scope;
        CHECK(DryRun::IsEnabled());
    }

    CHECK(!DryRun::IsEnabled());
    PASS();
}

TEST_CASE("DryRun::Scope: preserves previous enabled state") {
    DryRun::Enable();
    CHECK(DryRun::IsEnabled());

    {
        DryRun::Scope scope;
        CHECK(DryRun::IsEnabled());
    }

    // Should still be enabled because it was enabled before
    CHECK(DryRun::IsEnabled());

    DryRun::Disable();
    PASS();
}

TEST_CASE("DryRun::Scope: nested scopes") {
    DryRun::Disable();

    {
        DryRun::Scope outer;
        CHECK(DryRun::IsEnabled());

        {
            DryRun::Scope inner;
            CHECK(DryRun::IsEnabled());
        }

        // Still enabled from outer scope
        CHECK(DryRun::IsEnabled());
    }

    CHECK(!DryRun::IsEnabled());
    PASS();
}

//-----------------------------------------------------------------------------
// Macro Tests
//-----------------------------------------------------------------------------

TEST_CASE("DRYRUN_LOG macro") {
    DryRun::Enable();
    DryRun::ClearLog();

    DRYRUN_LOG("MacroTest", "Operation", "details here");

    CHECK_EQ(DryRun::GetLogCount(), 1u);
    auto entries = DryRun::GetLogEntries();
    CHECK_EQ(entries[0].component, "MacroTest");

    DryRun::Disable();
    PASS();
}

//-----------------------------------------------------------------------------
// Callback Tests
//-----------------------------------------------------------------------------

TEST_CASE("DryRun: callback invoked") {
    DryRun::Enable();
    DryRun::ClearLog();

    int callbackCount = 0;
    std::string lastComponent;

    DryRun::SetLogCallback([&](const DryRunLogEntry& entry) {
        callbackCount++;
        lastComponent = entry.component;
    });

    DryRun::Log("CallbackTest", "Op1", "");
    DryRun::Log("CallbackTest2", "Op2", "");

    CHECK_EQ(callbackCount, 2);
    CHECK_EQ(lastComponent, "CallbackTest2");

    // Clear callback
    DryRun::SetLogCallback(nullptr);
    DryRun::Disable();
    PASS();
}

//-----------------------------------------------------------------------------
// Integration Tests
//-----------------------------------------------------------------------------

TEST_CASE("DryRun: enable clears previous log") {
    DryRun::Enable();
    DryRun::Log("Before", "Op", "");
    CHECK(DryRun::GetLogCount() > 0);

    // Re-enable should clear log
    DryRun::Enable();
    CHECK_EQ(DryRun::GetLogCount(), 0u);

    DryRun::Disable();
    PASS();
}
