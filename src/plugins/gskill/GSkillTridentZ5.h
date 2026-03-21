#pragma once
//=============================================================================
// OneClickRGB-Universal - G.Skill Trident Z5 Controller
//=============================================================================
// Support for G.Skill Trident Z5 RGB RAM via SMBus.
// Uses PawnIO for kernel-level SMBus access.
//=============================================================================

#include "../../devices/SMBusDevice.h"

namespace OCRGB {
namespace Plugins {

//-----------------------------------------------------------------------------
// G.Skill Constants
//-----------------------------------------------------------------------------
namespace GSkill {
    // SMBus addresses for RAM slots
    constexpr uint8_t ADDR_SLOT_A1 = 0x58;
    constexpr uint8_t ADDR_SLOT_A2 = 0x59;
    constexpr uint8_t ADDR_SLOT_B1 = 0x5A;
    constexpr uint8_t ADDR_SLOT_B2 = 0x5B;

    // LED count per module
    constexpr int LEDS_PER_MODULE = 8;

    // Register addresses for each LED (R, G, B)
    constexpr uint8_t LED_BASE_REG = 0x20;

    // Apply command
    constexpr uint8_t REG_APPLY = 0xE0;
    constexpr uint8_t VAL_APPLY = 0x01;

    // Detection
    constexpr uint8_t REG_DETECT = 0x60;
    constexpr uint8_t VAL_DETECT = 0x58;
}

//-----------------------------------------------------------------------------
// G.Skill Trident Z5 Controller
//-----------------------------------------------------------------------------
class GSkillTridentZ5 : public SMBusDevice {
public:
    GSkillTridentZ5(uint8_t smbusAddress);
    ~GSkillTridentZ5() override = default;

    //=========================================================================
    // IDevice Implementation
    //=========================================================================

    Result SetColor(const RGB& color) override;
    Result SetZoneColor(int zone, const RGB& color) override;
    Result SetMode(DeviceMode mode) override;
    Result SetBrightness(uint8_t brightness) override;
    Result SetSpeed(uint8_t speed) override;
    Result Apply() override;

    //=========================================================================
    // Static Methods
    //=========================================================================

    /// Detect G.Skill RAM modules on SMBus
    static std::vector<uint8_t> DetectModules();

protected:
    //=========================================================================
    // SMBusDevice Overrides
    //=========================================================================

    Result OnConnected() override;
    bool ProbeDevice() override;

private:
    /// Set color for a specific LED
    Result SetLEDColor(int led, const RGB& color);

    RGB m_ledColors[GSkill::LEDS_PER_MODULE];
    uint8_t m_smbusAddress;
};

} // namespace Plugins
} // namespace OCRGB
