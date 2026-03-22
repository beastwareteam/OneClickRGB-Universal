//=============================================================================
// Tests for core/Types.h
//=============================================================================

#include "TestFramework.h"
#include "../src/core/Types.h"

using namespace OCRGB;

//-----------------------------------------------------------------------------
// RGB Tests
//-----------------------------------------------------------------------------

TEST_CASE("RGB: default constructor") {
    RGB color;
    CHECK_EQ(color.r, 0);
    CHECK_EQ(color.g, 0);
    CHECK_EQ(color.b, 0);
    PASS();
}

TEST_CASE("RGB: parameterized constructor") {
    RGB color(255, 128, 64);
    CHECK_EQ(color.r, 255);
    CHECK_EQ(color.g, 128);
    CHECK_EQ(color.b, 64);
    PASS();
}

TEST_CASE("RGB: FromHex") {
    RGB color = RGB::FromHex(0xFF8040);
    CHECK_EQ(color.r, 255);
    CHECK_EQ(color.g, 128);
    CHECK_EQ(color.b, 64);
    PASS();
}

TEST_CASE("RGB: ToHex") {
    RGB color(255, 128, 64);
    CHECK_EQ(color.ToHex(), 0xFF8040u);
    PASS();
}

TEST_CASE("RGB: FromHex and ToHex roundtrip") {
    uint32_t original = 0x00AAFF;
    RGB color = RGB::FromHex(original);
    CHECK_EQ(color.ToHex(), original);
    PASS();
}

TEST_CASE("RGB: equality operator") {
    RGB a(100, 150, 200);
    RGB b(100, 150, 200);
    RGB c(100, 150, 201);

    CHECK(a == b);
    CHECK(!(a == c));
    PASS();
}

//-----------------------------------------------------------------------------
// Result Tests
//-----------------------------------------------------------------------------

TEST_CASE("Result: Success") {
    Result r = Result::Success();
    CHECK(r.IsSuccess());
    CHECK(!r.IsError());
    CHECK_EQ(r.code, ResultCode::Success);
    PASS();
}

TEST_CASE("Result: Error with message") {
    Result r = Result::Error("Something went wrong");
    CHECK(!r.IsSuccess());
    CHECK(r.IsError());
    CHECK_EQ(r.code, ResultCode::Error);
    CHECK_EQ(r.message, "Something went wrong");
    PASS();
}

TEST_CASE("Result: NotConnected") {
    Result r = Result::NotConnected();
    CHECK(r.IsError());
    CHECK_EQ(r.code, ResultCode::NotConnected);
    PASS();
}

TEST_CASE("Result: NotSupported") {
    Result r = Result::NotSupported();
    CHECK(r.IsError());
    CHECK_EQ(r.code, ResultCode::NotSupported);
    PASS();
}

//-----------------------------------------------------------------------------
// DeviceMode Tests
//-----------------------------------------------------------------------------

TEST_CASE("DeviceMode: ToString") {
    CHECK_EQ(std::string(DeviceModeToString(DeviceMode::Off)), "Off");
    CHECK_EQ(std::string(DeviceModeToString(DeviceMode::Static)), "Static");
    CHECK_EQ(std::string(DeviceModeToString(DeviceMode::Breathing)), "Breathing");
    CHECK_EQ(std::string(DeviceModeToString(DeviceMode::Wave)), "Wave");
    CHECK_EQ(std::string(DeviceModeToString(DeviceMode::Spectrum)), "Spectrum");
    PASS();
}

//-----------------------------------------------------------------------------
// DeviceType Tests
//-----------------------------------------------------------------------------

TEST_CASE("DeviceType: ToString") {
    CHECK_EQ(std::string(DeviceTypeToString(DeviceType::Mainboard)), "Mainboard");
    CHECK_EQ(std::string(DeviceTypeToString(DeviceType::RAM)), "RAM");
    CHECK_EQ(std::string(DeviceTypeToString(DeviceType::Keyboard)), "Keyboard");
    CHECK_EQ(std::string(DeviceTypeToString(DeviceType::Mouse)), "Mouse");
    PASS();
}

//-----------------------------------------------------------------------------
// DeviceAddress Tests
//-----------------------------------------------------------------------------

TEST_CASE("DeviceAddress: HID ToString") {
    DeviceAddress addr;
    addr.protocol = ProtocolType::HID;
    addr.vendorId = 0x0B05;
    addr.productId = 0x19AF;

    std::string str = addr.ToString();
    CHECK(str.find("HID") != std::string::npos);
    CHECK(str.find("0B05") != std::string::npos);
    CHECK(str.find("19AF") != std::string::npos);
    PASS();
}

TEST_CASE("DeviceAddress: SMBus ToString") {
    DeviceAddress addr;
    addr.protocol = ProtocolType::SMBus;
    addr.busNumber = 0;
    addr.deviceAddress = 0x58;

    std::string str = addr.ToString();
    CHECK(str.find("SMBus") != std::string::npos);
    CHECK(str.find("0x58") != std::string::npos);
    PASS();
}

//-----------------------------------------------------------------------------
// Capabilities Tests
//-----------------------------------------------------------------------------

TEST_CASE("Capabilities: defaults") {
    Capabilities caps;
    CHECK(caps.supportsColor);
    CHECK(caps.supportsBrightness);
    CHECK(caps.supportsSpeed);
    CHECK(!caps.supportsDirection);
    CHECK(!caps.supportsPerLedColor);
    CHECK_EQ(caps.zoneCount, 1);
    CHECK_EQ(caps.minBrightness, 0);
    CHECK_EQ(caps.maxBrightness, 100);
    PASS();
}
