//=============================================================================
// Tests for core/DeviceRegistry
//=============================================================================

#include "TestFramework.h"
#include "../src/core/DeviceRegistry.h"
#include "../src/core/Types.h"
#include <memory>

using namespace OCRGB;

//-----------------------------------------------------------------------------
// Mock Device for Testing
//-----------------------------------------------------------------------------

class MockDevice : public IDevice {
public:
    MockDevice(const std::string& id, const std::string& name, DeviceType type)
        : m_connected(true), m_ready(true) {
        m_info.id = id;
        m_info.name = name;
        m_info.type = type;
        m_state.connected = true;
        m_state.initialized = true;
    }

    Result Initialize() override {
        m_state.initialized = true;
        m_ready = true;
        return Result::Success();
    }

    void Shutdown() override {
        m_state.initialized = false;
        m_ready = false;
    }

    bool IsConnected() const override { return m_connected; }
    bool IsReady() const override { return m_ready; }

    const DeviceInfo& GetInfo() const override { return m_info; }
    const Capabilities& GetCapabilities() const override { return m_info.capabilities; }
    const DeviceState& GetState() const override { return m_state; }

    Result SetColor(const RGB& color) override {
        m_state.currentColor = color;
        m_lastColor = color;
        return Result::Success();
    }

    Result SetMode(DeviceMode mode) override {
        m_state.currentMode = mode;
        return Result::Success();
    }

    Result SetBrightness(uint8_t brightness) override {
        m_state.brightness = brightness;
        return Result::Success();
    }

    Result SetSpeed(uint8_t speed) override {
        m_state.speed = speed;
        return Result::Success();
    }

    Result SendRawPacket(const uint8_t*, size_t) override {
        return Result::NotSupported();
    }

    Result ReadRawResponse(uint8_t*, size_t, size_t&, int) override {
        return Result::NotSupported();
    }

    Result Apply() override {
        m_applyCount++;
        return Result::Success();
    }

    // Test helpers
    RGB GetLastColor() const { return m_lastColor; }
    int GetApplyCount() const { return m_applyCount; }
    void SetConnected(bool c) { m_connected = c; m_state.connected = c; }

private:
    DeviceInfo m_info;
    DeviceState m_state;
    bool m_connected;
    bool m_ready;
    RGB m_lastColor;
    int m_applyCount = 0;
};

//-----------------------------------------------------------------------------
// Registry Tests
//-----------------------------------------------------------------------------

TEST_CASE("Registry: starts empty") {
    DeviceRegistry::Instance().Clear();
    CHECK_EQ(DeviceRegistry::Instance().GetDeviceCount(), 0u);
    PASS();
}

TEST_CASE("Registry: register and get device") {
    DeviceRegistry::Instance().Clear();

    auto device = std::make_shared<MockDevice>("test-001", "Test Device", DeviceType::Keyboard);
    DeviceRegistry::Instance().RegisterDevice(device);

    CHECK_EQ(DeviceRegistry::Instance().GetDeviceCount(), 1u);

    auto retrieved = DeviceRegistry::Instance().GetDevice("test-001");
    CHECK(retrieved != nullptr);
    CHECK_EQ(retrieved->GetId(), "test-001");

    DeviceRegistry::Instance().Clear();
    PASS();
}

TEST_CASE("Registry: get non-existent device returns nullptr") {
    DeviceRegistry::Instance().Clear();

    auto device = DeviceRegistry::Instance().GetDevice("does-not-exist");
    CHECK(device == nullptr);

    PASS();
}

TEST_CASE("Registry: unregister device") {
    DeviceRegistry::Instance().Clear();

    auto device = std::make_shared<MockDevice>("test-002", "Test Device 2", DeviceType::Mouse);
    DeviceRegistry::Instance().RegisterDevice(device);
    CHECK_EQ(DeviceRegistry::Instance().GetDeviceCount(), 1u);

    DeviceRegistry::Instance().UnregisterDevice("test-002");
    CHECK_EQ(DeviceRegistry::Instance().GetDeviceCount(), 0u);

    PASS();
}

TEST_CASE("Registry: GetAllDevices") {
    DeviceRegistry::Instance().Clear();

    auto dev1 = std::make_shared<MockDevice>("dev-a", "Device A", DeviceType::Keyboard);
    auto dev2 = std::make_shared<MockDevice>("dev-b", "Device B", DeviceType::Mouse);
    auto dev3 = std::make_shared<MockDevice>("dev-c", "Device C", DeviceType::RAM);

    DeviceRegistry::Instance().RegisterDevice(dev1);
    DeviceRegistry::Instance().RegisterDevice(dev2);
    DeviceRegistry::Instance().RegisterDevice(dev3);

    auto all = DeviceRegistry::Instance().GetAllDevices();
    CHECK_EQ(all.size(), 3u);

    DeviceRegistry::Instance().Clear();
    PASS();
}

TEST_CASE("Registry: GetDevicesByType") {
    DeviceRegistry::Instance().Clear();

    auto kb1 = std::make_shared<MockDevice>("kb-1", "Keyboard 1", DeviceType::Keyboard);
    auto kb2 = std::make_shared<MockDevice>("kb-2", "Keyboard 2", DeviceType::Keyboard);
    auto mouse = std::make_shared<MockDevice>("mouse-1", "Mouse 1", DeviceType::Mouse);

    DeviceRegistry::Instance().RegisterDevice(kb1);
    DeviceRegistry::Instance().RegisterDevice(kb2);
    DeviceRegistry::Instance().RegisterDevice(mouse);

    auto keyboards = DeviceRegistry::Instance().GetDevicesByType(DeviceType::Keyboard);
    CHECK_EQ(keyboards.size(), 2u);

    auto mice = DeviceRegistry::Instance().GetDevicesByType(DeviceType::Mouse);
    CHECK_EQ(mice.size(), 1u);

    auto rams = DeviceRegistry::Instance().GetDevicesByType(DeviceType::RAM);
    CHECK_EQ(rams.size(), 0u);

    DeviceRegistry::Instance().Clear();
    PASS();
}

TEST_CASE("Registry: SetColorAll") {
    DeviceRegistry::Instance().Clear();

    auto dev1 = std::make_shared<MockDevice>("col-1", "Device 1", DeviceType::Keyboard);
    auto dev2 = std::make_shared<MockDevice>("col-2", "Device 2", DeviceType::Mouse);

    DeviceRegistry::Instance().RegisterDevice(dev1);
    DeviceRegistry::Instance().RegisterDevice(dev2);

    RGB testColor(255, 128, 64);
    DeviceRegistry::Instance().SetColorAll(testColor);

    CHECK(dev1->GetLastColor() == testColor);
    CHECK(dev2->GetLastColor() == testColor);

    DeviceRegistry::Instance().Clear();
    PASS();
}

TEST_CASE("Registry: ApplyAll") {
    DeviceRegistry::Instance().Clear();

    auto dev1 = std::make_shared<MockDevice>("apply-1", "Device 1", DeviceType::Keyboard);
    auto dev2 = std::make_shared<MockDevice>("apply-2", "Device 2", DeviceType::Mouse);

    DeviceRegistry::Instance().RegisterDevice(dev1);
    DeviceRegistry::Instance().RegisterDevice(dev2);

    DeviceRegistry::Instance().ApplyAll();

    CHECK_EQ(dev1->GetApplyCount(), 1);
    CHECK_EQ(dev2->GetApplyCount(), 1);

    DeviceRegistry::Instance().ApplyAll();

    CHECK_EQ(dev1->GetApplyCount(), 2);
    CHECK_EQ(dev2->GetApplyCount(), 2);

    DeviceRegistry::Instance().Clear();
    PASS();
}

TEST_CASE("Registry: Clear shuts down devices") {
    DeviceRegistry::Instance().Clear();

    auto device = std::make_shared<MockDevice>("clear-1", "Device", DeviceType::Keyboard);
    DeviceRegistry::Instance().RegisterDevice(device);

    CHECK(device->IsReady());

    DeviceRegistry::Instance().Clear();

    CHECK(!device->IsReady());
    CHECK_EQ(DeviceRegistry::Instance().GetDeviceCount(), 0u);

    PASS();
}

TEST_CASE("Registry: null device ignored") {
    DeviceRegistry::Instance().Clear();

    DeviceRegistry::Instance().RegisterDevice(nullptr);
    CHECK_EQ(DeviceRegistry::Instance().GetDeviceCount(), 0u);

    PASS();
}
