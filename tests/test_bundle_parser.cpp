//=============================================================================
// Tests for app/config/ConfigBundleParser
//=============================================================================

#include "TestFramework.h"
#include "../src/app/config/ConfigBundleParser.h"

using namespace OCRGB;
using namespace OCRGB::App;

//-----------------------------------------------------------------------------
// Sample Bundle JSON
//-----------------------------------------------------------------------------

static const char* VALID_BUNDLE = R"({
    "bundleVersion": "1.0.0",
    "createdAt": "2026-03-22T12:00:00Z",
    "target": {
        "os": "windows",
        "arch": "x64"
    },
    "profiles": [
        {
            "id": "default-gaming",
            "priority": 100,
            "match": {
                "vendors": ["ASUS", "G.Skill"],
                "deviceTypes": ["Mainboard", "RAM"]
            },
            "actions": [
                { "op": "set-mode", "value": "Static" },
                { "op": "set-color", "value": "#00AAFF" },
                { "op": "set-brightness", "value": "80" },
                { "op": "apply" }
            ]
        },
        {
            "id": "low-priority",
            "priority": 10,
            "match": {
                "deviceTypes": ["Keyboard"]
            },
            "actions": [
                { "op": "set-color", "value": "#FF0000" }
            ]
        }
    ],
    "policies": {
        "allowRawPackets": false,
        "maxRetry": 3,
        "rollbackOnError": true
    },
    "integrity": {
        "hash": "sha256:abc123",
        "signature": "base64sig",
        "keyId": "ocrgb-prod-1"
    }
})";

static const char* MINIMAL_BUNDLE = R"({
    "bundleVersion": "1.0.0",
    "profiles": [
        {
            "id": "minimal",
            "priority": 1,
            "match": {},
            "actions": [
                { "op": "set-color", "value": "#FFFFFF" }
            ]
        }
    ],
    "policies": {
        "allowRawPackets": false,
        "maxRetry": 0,
        "rollbackOnError": false
    },
    "integrity": {
        "hash": "sha256:xxx",
        "signature": "yyy",
        "keyId": "test"
    }
})";

//-----------------------------------------------------------------------------
// Loading Tests
//-----------------------------------------------------------------------------

TEST_CASE("ConfigBundleParser: load valid bundle") {
    ConfigBundleParser parser;
    bool result = parser.LoadFromString(VALID_BUNDLE);

    CHECK(result);
    CHECK(parser.IsLoaded());
    CHECK(parser.GetLastError().empty());
    PASS();
}

TEST_CASE("ConfigBundleParser: load empty string fails") {
    ConfigBundleParser parser;
    bool result = parser.LoadFromString("");

    CHECK(!result);
    CHECK(!parser.IsLoaded());
    CHECK(!parser.GetLastError().empty());
    PASS();
}

TEST_CASE("ConfigBundleParser: load invalid JSON fails") {
    ConfigBundleParser parser;
    bool result = parser.LoadFromString("{ not valid json }}}");

    CHECK(!result);
    PASS();
}

TEST_CASE("ConfigBundleParser: missing bundleVersion fails") {
    ConfigBundleParser parser;
    bool result = parser.LoadFromString(R"({
        "profiles": [{"id": "x", "actions": []}]
    })");

    CHECK(!result);
    PASS();
}

//-----------------------------------------------------------------------------
// Bundle Field Tests
//-----------------------------------------------------------------------------

TEST_CASE("ConfigBundleParser: bundle version") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    CHECK_EQ(parser.GetBundle().bundleVersion, "1.0.0");
    PASS();
}

TEST_CASE("ConfigBundleParser: target fields") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    CHECK_EQ(parser.GetBundle().targetOs, "windows");
    CHECK_EQ(parser.GetBundle().targetArch, "x64");
    PASS();
}

TEST_CASE("ConfigBundleParser: policies") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    const auto& policies = parser.GetPolicies();
    CHECK_EQ(policies.allowRawPackets, false);
    CHECK_EQ(policies.maxRetry, 3);
    CHECK_EQ(policies.rollbackOnError, true);
    PASS();
}

TEST_CASE("ConfigBundleParser: integrity") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    const auto& integrity = parser.GetBundle().integrity;
    CHECK_EQ(integrity.hash, "sha256:abc123");
    CHECK_EQ(integrity.signature, "base64sig");
    CHECK_EQ(integrity.keyId, "ocrgb-prod-1");
    PASS();
}

//-----------------------------------------------------------------------------
// Profile Tests
//-----------------------------------------------------------------------------

TEST_CASE("ConfigBundleParser: profile count") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    CHECK_EQ(parser.GetProfiles().size(), 2u);
    PASS();
}

TEST_CASE("ConfigBundleParser: profile fields") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    const auto& profiles = parser.GetProfiles();
    CHECK_EQ(profiles[0].id, "default-gaming");
    CHECK_EQ(profiles[0].priority, 100);
    PASS();
}

TEST_CASE("ConfigBundleParser: profile match vendors") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    const auto& match = parser.GetProfiles()[0].match;
    CHECK_EQ(match.vendors.size(), 2u);
    CHECK_EQ(match.vendors[0], "ASUS");
    CHECK_EQ(match.vendors[1], "G.Skill");
    PASS();
}

TEST_CASE("ConfigBundleParser: profile match deviceTypes") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    const auto& match = parser.GetProfiles()[0].match;
    CHECK_EQ(match.deviceTypes.size(), 2u);
    CHECK_EQ(match.deviceTypes[0], "Mainboard");
    CHECK_EQ(match.deviceTypes[1], "RAM");
    PASS();
}

TEST_CASE("ConfigBundleParser: profile actions") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    const auto& actions = parser.GetProfiles()[0].actions;
    CHECK_EQ(actions.size(), 4u);
    CHECK_EQ(actions[0].op, "set-mode");
    CHECK_EQ(actions[0].value, "Static");
    CHECK_EQ(actions[1].op, "set-color");
    CHECK_EQ(actions[1].value, "#00AAFF");
    PASS();
}

//-----------------------------------------------------------------------------
// Profile Matching Tests
//-----------------------------------------------------------------------------

TEST_CASE("ConfigBundleParser: find matching profile by vendor") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    const auto* profile = parser.FindMatchingProfile("ASUS", DeviceType::Mainboard);
    CHECK(profile != nullptr);
    CHECK_EQ(profile->id, "default-gaming");
    PASS();
}

TEST_CASE("ConfigBundleParser: find matching profile by device type only") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    const auto* profile = parser.FindMatchingProfile("Unknown", DeviceType::Keyboard);
    CHECK(profile != nullptr);
    CHECK_EQ(profile->id, "low-priority");
    PASS();
}

TEST_CASE("ConfigBundleParser: no matching profile") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    const auto* profile = parser.FindMatchingProfile("Unknown", DeviceType::Headset);
    CHECK(profile == nullptr);
    PASS();
}

TEST_CASE("ConfigBundleParser: priority selection") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    // ASUS Mainboard should match high-priority profile
    const auto* profile = parser.FindMatchingProfile("ASUS", DeviceType::Mainboard);
    CHECK(profile != nullptr);
    CHECK_EQ(profile->priority, 100);
    PASS();
}

//-----------------------------------------------------------------------------
// ToResolvedProfile Tests
//-----------------------------------------------------------------------------

TEST_CASE("ConfigBundleParser: ToResolvedProfile") {
    ConfigBundleParser parser;
    parser.LoadFromString(VALID_BUNDLE);

    const auto* profile = parser.FindMatchingProfile("ASUS", DeviceType::Mainboard);
    CHECK(profile != nullptr);

    ResolvedProfile resolved = parser.ToResolvedProfile(*profile);

    CHECK_EQ(resolved.id, "default-gaming");
    CHECK_EQ(resolved.priority, 100);
    CHECK_EQ(resolved.mode, DeviceMode::Static);
    CHECK_EQ(resolved.brightness, 80);
    // Color #00AAFF = RGB(0, 170, 255)
    CHECK_EQ(resolved.defaultColor.r, 0);
    CHECK_EQ(resolved.defaultColor.g, 170);
    CHECK_EQ(resolved.defaultColor.b, 255);
    PASS();
}

//-----------------------------------------------------------------------------
// Minimal Bundle Tests
//-----------------------------------------------------------------------------

TEST_CASE("ConfigBundleParser: minimal bundle") {
    ConfigBundleParser parser;
    bool result = parser.LoadFromString(MINIMAL_BUNDLE);

    CHECK(result);
    CHECK_EQ(parser.GetProfiles().size(), 1u);
    CHECK_EQ(parser.GetProfiles()[0].id, "minimal");
    PASS();
}
