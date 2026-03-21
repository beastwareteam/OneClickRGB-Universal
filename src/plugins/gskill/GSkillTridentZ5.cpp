//=============================================================================
// OneClickRGB-Universal - G.Skill Trident Z5 Implementation
//=============================================================================

#include "GSkillTridentZ5.h"
#include "../../bridges/SMBusBridge.h"
#include <cstring>

namespace OCRGB {
namespace Plugins {

GSkillTridentZ5::GSkillTridentZ5(uint8_t smbusAddress)
    : m_smbusAddress(smbusAddress) {

    // Set SMBus address
    SetSMBusAddress(0, smbusAddress);

    // Set device info
    char name[64];
    snprintf(name, sizeof(name), "G.Skill Trident Z5 (0x%02X)", smbusAddress);
    SetDeviceInfo(name, "G.Skill", DeviceType::RAM);

    // Set capabilities
    Capabilities caps;
    caps.supportsColor = true;
    caps.supportsBrightness = false;  // Z5 doesn't have brightness control
    caps.supportsSpeed = false;
    caps.supportsPerLedColor = true;
    caps.zoneCount = GSkill::LEDS_PER_MODULE;
    caps.ledsPerZone = 1;
    caps.totalLeds = GSkill::LEDS_PER_MODULE;
    caps.supportedModes = {
        DeviceMode::Static
    };
    SetCapabilities(caps);

    // Initialize LEDs to off
    memset(m_ledColors, 0, sizeof(m_ledColors));
}

Result GSkillTridentZ5::OnConnected() {
    // Nothing special needed
    return Result::Success();
}

bool GSkillTridentZ5::ProbeDevice() {
    uint8_t value;
    if (ReadByte(GSkill::REG_DETECT, value).IsSuccess()) {
        // Check if value matches expected G.Skill signature
        return (value == GSkill::VAL_DETECT || value != 0);
    }
    return false;
}

Result GSkillTridentZ5::SetColor(const RGB& color) {
    // Set all LEDs to the same color
    for (int led = 0; led < GSkill::LEDS_PER_MODULE; led++) {
        m_ledColors[led] = color;
        Result result = SetLEDColor(led, color);
        if (result.IsError()) return result;
    }

    m_state.currentColor = color;
    return Result::Success();
}

Result GSkillTridentZ5::SetZoneColor(int zone, const RGB& color) {
    if (zone < 0 || zone >= GSkill::LEDS_PER_MODULE) {
        return Result{ResultCode::InvalidParameter, "Invalid LED index"};
    }

    m_ledColors[zone] = color;
    return SetLEDColor(zone, color);
}

Result GSkillTridentZ5::SetLEDColor(int led, const RGB& color) {
    // Each LED has 3 registers: R, G, B
    // LED 0: 0x20, 0x21, 0x22
    // LED 1: 0x23, 0x24, 0x25
    // etc.

    uint8_t baseReg = GSkill::LED_BASE_REG + (led * 3);

    Result result = WriteByte(baseReg + 0, color.r);
    if (result.IsError()) return result;

    result = WriteByte(baseReg + 1, color.g);
    if (result.IsError()) return result;

    result = WriteByte(baseReg + 2, color.b);
    return result;
}

Result GSkillTridentZ5::SetMode(DeviceMode mode) {
    m_state.currentMode = mode;

    // G.Skill Z5 only supports static mode via direct control
    if (mode != DeviceMode::Static) {
        return Result::NotSupported();
    }

    return Result::Success();
}

Result GSkillTridentZ5::SetBrightness(uint8_t brightness) {
    m_state.brightness = brightness;
    // Not supported
    return Result::NotSupported();
}

Result GSkillTridentZ5::SetSpeed(uint8_t speed) {
    m_state.speed = speed;
    // Not supported
    return Result::NotSupported();
}

Result GSkillTridentZ5::Apply() {
    // Send apply command
    return WriteByte(GSkill::REG_APPLY, GSkill::VAL_APPLY);
}

std::vector<uint8_t> GSkillTridentZ5::DetectModules() {
    std::vector<uint8_t> modules;

    SMBusBridge bridge;
    if (!bridge.Initialize()) {
        return modules;
    }

    // Check each possible RAM slot address
    uint8_t addresses[] = {
        GSkill::ADDR_SLOT_A1,
        GSkill::ADDR_SLOT_A2,
        GSkill::ADDR_SLOT_B1,
        GSkill::ADDR_SLOT_B2
    };

    for (uint8_t addr : addresses) {
        if (bridge.ProbeAddress(addr)) {
            // Try to read detection register
            uint8_t value;
            if (bridge.ReadByte(addr, GSkill::REG_DETECT, value)) {
                modules.push_back(addr);
            }
        }
    }

    bridge.Shutdown();
    return modules;
}

} // namespace Plugins
} // namespace OCRGB
