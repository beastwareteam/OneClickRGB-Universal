#pragma once
//=============================================================================
// OneClickRGB-Universal - ASUS Aura Controller
//=============================================================================
// Support for ASUS Aura mainboard RGB control via USB HID.
// Supports addressable headers with up to 8 channels, 60 LEDs each.
//=============================================================================

#include "../../devices/HIDDevice.h"

namespace OCRGB {
namespace Plugins {

//-----------------------------------------------------------------------------
// ASUS Aura Constants
//-----------------------------------------------------------------------------
namespace AsusAura {
    constexpr uint16_t VENDOR_ID = 0x0B05;
    constexpr uint16_t PRODUCT_ID = 0x19AF;
    constexpr uint16_t USAGE_PAGE = 0xFF72;

    constexpr uint8_t REPORT_ID = 0xB0;
    constexpr size_t PACKET_SIZE = 65;

    // Commands
    constexpr uint8_t CMD_SET_DIRECT = 0x40;
    constexpr uint8_t CMD_SET_MODE = 0x35;
    constexpr uint8_t CMD_COMMIT = 0x35;

    // Modes
    constexpr uint8_t MODE_STATIC = 0;
    constexpr uint8_t MODE_BREATHING = 1;
    constexpr uint8_t MODE_WAVE = 2;
    constexpr uint8_t MODE_SPECTRUM = 4;
    constexpr uint8_t MODE_REACTIVE = 7;

    // Addressable LED config
    constexpr int MAX_CHANNELS = 8;
    constexpr int LEDS_PER_CHANNEL = 60;
}

//-----------------------------------------------------------------------------
// ASUS Aura Controller
//-----------------------------------------------------------------------------
class AsusAuraController : public HIDDevice {
public:
    AsusAuraController();
    ~AsusAuraController() override = default;

    //=========================================================================
    // IDevice Implementation
    //=========================================================================

    Result SetColor(const RGB& color) override;
    Result SetZoneColor(int zone, const RGB& color) override;
    Result SetMode(DeviceMode mode) override;
    Result SetBrightness(uint8_t brightness) override;
    Result SetSpeed(uint8_t speed) override;
    Result Apply() override;

protected:
    //=========================================================================
    // HIDDevice Overrides
    //=========================================================================

    Result OnConnected() override;
    void OnDisconnecting() override;
    size_t GetPacketSize() const override { return AsusAura::PACKET_SIZE; }
    uint8_t GetReportId() const override { return AsusAura::REPORT_ID; }

private:
    //=========================================================================
    // Internal Methods
    //=========================================================================

    /// Set device to direct control mode
    Result SetDirectMode();

    /// Send color data for all addressable LEDs
    Result SendColorData();

    /// Convert our mode enum to ASUS mode byte
    uint8_t ModeToAsusByte(DeviceMode mode);

    //=========================================================================
    // State
    //=========================================================================

    RGB m_channelColors[AsusAura::MAX_CHANNELS][AsusAura::LEDS_PER_CHANNEL];
    bool m_directMode = false;
    uint8_t m_currentMode = AsusAura::MODE_STATIC;
    uint8_t m_brightness = 100;
    uint8_t m_speed = 50;
};

} // namespace Plugins
} // namespace OCRGB
