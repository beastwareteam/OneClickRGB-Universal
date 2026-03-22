//=============================================================================
// Tests for app/config/DeviceConfiguration
//=============================================================================

#include "TestFramework.h"
#include "../src/app/config/DeviceConfiguration.h"

using namespace OCRGB;
using namespace OCRGB::App;

//-----------------------------------------------------------------------------
// ResolvedProfile Tests
//-----------------------------------------------------------------------------

TEST_CASE("ResolvedProfile: defaults") {
    ResolvedProfile profile;

    CHECK(profile.id.empty());
    CHECK_EQ(profile.priority, 0);
    CHECK_EQ(profile.mode, DeviceMode::Static);
    CHECK_EQ(profile.brightness, 100);
    CHECK_EQ(profile.speed, 50);
    // Default color is RGB(0, 170, 255)
    CHECK_EQ(profile.defaultColor.r, 0);
    CHECK_EQ(profile.defaultColor.g, 170);
    CHECK_EQ(profile.defaultColor.b, 255);
    PASS();
}

//-----------------------------------------------------------------------------
// DeviceConfiguration Tests
//-----------------------------------------------------------------------------

TEST_CASE("DeviceConfiguration: SetupFromBundle") {
    DeviceConfiguration config;

    config.SetupFromBundle("1.0.0", "gaming-profile");

    CHECK_EQ(config.GetBundleVersion(), "1.0.0");
    CHECK_EQ(config.GetProfileId(), "gaming-profile");
    PASS();
}

TEST_CASE("DeviceConfiguration: SetupFromHardwareMatch") {
    DeviceConfiguration config;

    DeviceInfo info;
    info.vendor = "ASUS";
    info.model = "ROG Strix";
    info.id = "asus-001";

    config.SetupFromHardwareMatch(info);

    std::string deviceKey = config.GetDeviceKey();
    CHECK(deviceKey.find("ASUS") != std::string::npos);
    CHECK(deviceKey.find("ROG Strix") != std::string::npos);
    CHECK(deviceKey.find("asus-001") != std::string::npos);
    PASS();
}

TEST_CASE("DeviceConfiguration: SetResolvedProfile") {
    DeviceConfiguration config;

    ResolvedProfile profile;
    profile.id = "custom-profile";
    profile.priority = 100;
    profile.mode = DeviceMode::Breathing;
    profile.defaultColor = RGB(255, 0, 128);
    profile.brightness = 75;
    profile.speed = 30;

    config.SetResolvedProfile(profile);

    CHECK_EQ(config.GetProfileId(), "custom-profile");
    CHECK_EQ(config.GetMode(), DeviceMode::Breathing);
    CHECK(config.GetDefaultColor() == RGB(255, 0, 128));
    CHECK_EQ(config.GetBrightness(), 75);
    CHECK_EQ(config.GetSpeed(), 30);
    PASS();
}

TEST_CASE("DeviceConfiguration: SetResolvedProfile preserves existing profileId") {
    DeviceConfiguration config;

    // First set bundle with profile ID
    config.SetupFromBundle("1.0.0", "original-profile");

    // Then set resolved profile without ID
    ResolvedProfile profile;
    profile.id = "";
    profile.mode = DeviceMode::Wave;

    config.SetResolvedProfile(profile);

    // Original profile ID should be preserved
    CHECK_EQ(config.GetProfileId(), "original-profile");
    CHECK_EQ(config.GetMode(), DeviceMode::Wave);
    PASS();
}

TEST_CASE("DeviceConfiguration: full setup flow") {
    DeviceConfiguration config;

    // Step 1: Setup from bundle
    config.SetupFromBundle("2.0.0", "default-auto");

    // Step 2: Setup from hardware match
    DeviceInfo info;
    info.vendor = "SteelSeries";
    info.model = "Rival 600";
    info.id = "ss-rival-001";
    config.SetupFromHardwareMatch(info);

    // Step 3: Resolve profile
    ResolvedProfile profile;
    profile.id = "default-auto";
    profile.mode = DeviceMode::Spectrum;
    profile.defaultColor = RGB(0, 255, 0);
    profile.brightness = 100;
    profile.speed = 60;
    config.SetResolvedProfile(profile);

    // Verify all values
    CHECK_EQ(config.GetBundleVersion(), "2.0.0");
    CHECK_EQ(config.GetProfileId(), "default-auto");
    CHECK(config.GetDeviceKey().find("SteelSeries") != std::string::npos);
    CHECK_EQ(config.GetMode(), DeviceMode::Spectrum);
    CHECK(config.GetDefaultColor() == RGB(0, 255, 0));
    CHECK_EQ(config.GetBrightness(), 100);
    CHECK_EQ(config.GetSpeed(), 60);
    PASS();
}
