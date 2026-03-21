#pragma once
//=============================================================================
// OneClickRGB-Universal - EVision Keyboard Controller
//=============================================================================
// Support for EVision RGB keyboards (common in gaming laptops).
// Uses V2 protocol with 64-byte packets.
//=============================================================================

#include "../../devices/HIDDevice.h"

namespace OCRGB {
namespace Plugins {

//-----------------------------------------------------------------------------
// EVision Constants
//-----------------------------------------------------------------------------
namespace EVision {
    constexpr uint16_t VENDOR_ID = 0x3299;
    constexpr uint16_t PRODUCT_ID = 0x4E9F;
    constexpr uint16_t USAGE_PAGE = 0xFF1C;

    constexpr uint8_t REPORT_ID = 0x04;
    constexpr size_t PACKET_SIZE = 64;

    // Commands
    constexpr uint8_t CMD_PREFIX = 0x0E;
    constexpr uint8_t CMD_SET_MODE = 0x01;
    constexpr uint8_t CMD_SET_COLOR = 0x02;
    constexpr uint8_t CMD_SET_BRIGHTNESS = 0x04;
    constexpr uint8_t CMD_SET_SPEED = 0x05;

    // Modes
    constexpr uint8_t MODE_STATIC = 6;
    constexpr uint8_t MODE_BREATHING = 5;
    constexpr uint8_t MODE_SPECTRUM = 4;
    constexpr uint8_t MODE_COLOR_WHEEL = 3;
    constexpr uint8_t MODE_WAVE_LONG = 2;
    constexpr uint8_t MODE_WAVE_SHORT = 1;
    constexpr uint8_t MODE_REACTIVE = 7;
}

//-----------------------------------------------------------------------------
// EVision Keyboard Controller
//-----------------------------------------------------------------------------
class EVisionKeyboard : public HIDDevice {
public:
    EVisionKeyboard();
    ~EVisionKeyboard() override = default;

    //=========================================================================
    // IDevice Implementation
    //=========================================================================

    Result SetColor(const RGB& color) override;
    Result SetMode(DeviceMode mode) override;
    Result SetBrightness(uint8_t brightness) override;
    Result SetSpeed(uint8_t speed) override;
    Result Apply() override;

protected:
    //=========================================================================
    // HIDDevice Overrides
    //=========================================================================

    Result OnConnected() override;
    size_t GetPacketSize() const override { return EVision::PACKET_SIZE; }
    uint8_t GetReportId() const override { return EVision::REPORT_ID; }

private:
    /// Send a V2 protocol command
    Result SendCommand(uint8_t cmd, uint8_t value);
    Result SendCommand(uint8_t cmd, uint8_t v1, uint8_t v2, uint8_t v3);

    /// Convert mode enum to EVision byte
    uint8_t ModeToEVisionByte(DeviceMode mode);

    uint8_t m_currentMode = EVision::MODE_STATIC;
};

} // namespace Plugins
} // namespace OCRGB
