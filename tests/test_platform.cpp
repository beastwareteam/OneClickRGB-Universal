//=============================================================================
// OneClickRGB-Universal - Platform Abstraction Tests
//=============================================================================

#include "TestFramework.h"
#include "platform/IPlatform.h"
#include "platform/PlatformFactory.cpp"

using namespace OCRGB::Platform;

//=============================================================================
// Platform Factory Tests
//=============================================================================

TEST_CASE("Platform::Create returns valid platform") {
    auto platform = IPlatform::Create();
    CHECK(platform != nullptr);
    PASS();
}

TEST_CASE("Platform::Initialize succeeds") {
    auto platform = IPlatform::Create();
    CHECK(platform != nullptr);

    bool initialized = platform->Initialize();
    CHECK_TRUE(initialized);
    CHECK_TRUE(platform->IsInitialized());

    platform->Shutdown();
    CHECK_FALSE(platform->IsInitialized());

    PASS();
}

TEST_CASE("Platform::Initialize is idempotent") {
    auto platform = IPlatform::Create();
    CHECK(platform != nullptr);

    CHECK_TRUE(platform->Initialize());
    CHECK_TRUE(platform->Initialize());  // Second call should also succeed
    CHECK_TRUE(platform->IsInitialized());

    platform->Shutdown();
    PASS();
}

//=============================================================================
// System Info Tests
//=============================================================================

TEST_CASE("Platform::GetSystemInfo returns valid object") {
    auto platform = IPlatform::Create();
    platform->Initialize();

    ISystemInfo* sysInfo = platform->GetSystemInfo();
    CHECK(sysInfo != nullptr);

    platform->Shutdown();
    PASS();
}

TEST_CASE("SystemInfo::GetCpuName returns non-empty string") {
    auto platform = IPlatform::Create();
    platform->Initialize();

    ISystemInfo* sysInfo = platform->GetSystemInfo();
    CHECK(sysInfo != nullptr);

    std::string cpuName = sysInfo->GetCpuName();
    CHECK_FALSE(cpuName.empty());

    platform->Shutdown();
    PASS();
}

TEST_CASE("SystemInfo::GetCpuCores returns positive value") {
    auto platform = IPlatform::Create();
    platform->Initialize();

    ISystemInfo* sysInfo = platform->GetSystemInfo();
    CHECK(sysInfo != nullptr);

    int cores = sysInfo->GetCpuCores();
    CHECK(cores > 0);

    platform->Shutdown();
    PASS();
}

TEST_CASE("SystemInfo::GetTotalRamBytes returns positive value") {
    auto platform = IPlatform::Create();
    platform->Initialize();

    ISystemInfo* sysInfo = platform->GetSystemInfo();
    CHECK(sysInfo != nullptr);

    uint64_t ram = sysInfo->GetTotalRamBytes();
    CHECK(ram > 0);

    platform->Shutdown();
    PASS();
}

TEST_CASE("SystemInfo::GetOsName returns non-empty string") {
    auto platform = IPlatform::Create();
    platform->Initialize();

    ISystemInfo* sysInfo = platform->GetSystemInfo();
    CHECK(sysInfo != nullptr);

    std::string osName = sysInfo->GetOsName();
    CHECK_FALSE(osName.empty());

#ifdef OCRGB_PLATFORM_WINDOWS
    CHECK_EQ(osName, "Windows");
#elif defined(OCRGB_PLATFORM_LINUX)
    CHECK_EQ(osName, "Linux");
#elif defined(OCRGB_PLATFORM_MACOS)
    CHECK_EQ(osName, "macOS");
#endif

    platform->Shutdown();
    PASS();
}

//=============================================================================
// Device Enumerator Tests
//=============================================================================

TEST_CASE("Platform::GetDeviceEnumerator returns valid object") {
    auto platform = IPlatform::Create();
    platform->Initialize();

    IDeviceEnumerator* deviceEnum = platform->GetDeviceEnumerator();
    CHECK(deviceEnum != nullptr);

    platform->Shutdown();
    PASS();
}

TEST_CASE("DeviceEnumerator::EnumerateUSBDevices runs without crash") {
    auto platform = IPlatform::Create();
    platform->Initialize();

    IDeviceEnumerator* deviceEnum = platform->GetDeviceEnumerator();
    CHECK(deviceEnum != nullptr);

    // Should not throw, even if no devices found
    auto devices = deviceEnum->EnumerateUSBDevices();
    // Result count depends on system - just verify no crash

    platform->Shutdown();
    PASS();
}

TEST_CASE("DeviceEnumerator::EnumerateHIDDevices runs without crash") {
    auto platform = IPlatform::Create();
    platform->Initialize();

    IDeviceEnumerator* deviceEnum = platform->GetDeviceEnumerator();
    CHECK(deviceEnum != nullptr);

    // Should not throw, even if no devices found
    auto devices = deviceEnum->EnumerateHIDDevices();
    // Result count depends on system - just verify no crash

    platform->Shutdown();
    PASS();
}

//=============================================================================
// SMBus Provider Tests
//=============================================================================

TEST_CASE("Platform::HasSMBusSupport returns expected value") {
    auto platform = IPlatform::Create();
    platform->Initialize();

#ifdef OCRGB_PLATFORM_MACOS
    // macOS does not support SMBus
    CHECK_FALSE(platform->HasSMBusSupport());
    CHECK(platform->GetSMBusProvider() == nullptr);
#else
    // Windows/Linux may or may not have SMBus depending on system
    // Just verify no crash
    bool hasSMBus = platform->HasSMBusSupport();
    (void)hasSMBus;  // Suppress unused warning
#endif

    platform->Shutdown();
    PASS();
}

//=============================================================================
// Capabilities Tests
//=============================================================================

TEST_CASE("Platform::RequiresElevation returns boolean") {
    auto platform = IPlatform::Create();
    platform->Initialize();

    // Just verify it doesn't crash
    bool requires = platform->RequiresElevation();
    (void)requires;

    platform->Shutdown();
    PASS();
}

TEST_CASE("Platform::IsElevated returns boolean") {
    auto platform = IPlatform::Create();
    platform->Initialize();

    // Just verify it doesn't crash
    bool elevated = platform->IsElevated();
    (void)elevated;

    platform->Shutdown();
    PASS();
}

TEST_CASE("Platform::GetDiagnostics returns non-empty string") {
    auto platform = IPlatform::Create();
    platform->Initialize();

    std::string diag = platform->GetDiagnostics();
    CHECK_FALSE(diag.empty());

    platform->Shutdown();
    PASS();
}
