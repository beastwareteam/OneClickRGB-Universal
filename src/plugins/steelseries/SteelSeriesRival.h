#pragma once
//=============================================================================
// OneClickRGB-Universal - SteelSeries Rival Controller
//=============================================================================
// Support for SteelSeries Rival 600 and similar mice.
//=============================================================================

#include "../../devices/HIDDevice.h"

namespace OCRGB {
namespace Plugins {

//-----------------------------------------------------------------------------
// SteelSeries Constants
//-----------------------------------------------------------------------------
namespace SteelSeries {
    constexpr uint16_t VENDOR_ID = 0x1038;
    constexpr uint16_t RIVAL_600_PID = 0x1724;

    constexpr uint8_t REPORT_ID = 0x00;
    constexpr size_t PACKET_SIZE = 65;

    // Commands
    constexpr uint8_t CMD_SET_COLOR = 0x05;

    // Zones
    constexpr int ZONE_LOGO = 0;
    constexpr int ZONE_WHEEL = 1;
    constexpr int ZONE_COUNT = 2;
}

//-----------------------------------------------------------------------------
// SteelSeries Rival Controller
//-----------------------------------------------------------------------------
class SteelSeriesRival : public HIDDevice {
public:
    SteelSeriesRival();
    ~SteelSeriesRival() override = default;

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

    size_t GetPacketSize() const override { return SteelSeries::PACKET_SIZE; }
    uint8_t GetReportId() const override { return SteelSeries::REPORT_ID; }

private:
    RGB m_zoneColors[SteelSeries::ZONE_COUNT];
};

} // namespace Plugins
} // namespace OCRGB
