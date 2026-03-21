//=============================================================================
// OneClickRGB-Universal - SteelSeries Rival Implementation
//=============================================================================

#include "SteelSeriesRival.h"
#include <cstring>

namespace OCRGB {
namespace Plugins {

SteelSeriesRival::SteelSeriesRival() {
    // Set device identifiers
    SetIdentifiers(SteelSeries::VENDOR_ID, SteelSeries::RIVAL_600_PID);

    // Set device info
    SetDeviceInfo("SteelSeries Rival 600", "SteelSeries", DeviceType::Mouse);

    // Set capabilities
    Capabilities caps;
    caps.supportsColor = true;
    caps.supportsBrightness = false;  // Rival 600 doesn't support brightness
    caps.supportsSpeed = false;
    caps.zoneCount = SteelSeries::ZONE_COUNT;
    caps.ledsPerZone = 1;
    caps.supportedModes = {
        DeviceMode::Static,
        DeviceMode::Breathing,
        DeviceMode::Spectrum
    };
    SetCapabilities(caps);

    // Initialize zone colors
    memset(m_zoneColors, 0, sizeof(m_zoneColors));
}

Result SteelSeriesRival::SetColor(const RGB& color) {
    // Set all zones to the same color
    for (int i = 0; i < SteelSeries::ZONE_COUNT; i++) {
        m_zoneColors[i] = color;
    }

    m_state.currentColor = color;

    // Send color command with both zones
    uint8_t packet[SteelSeries::PACKET_SIZE] = {0};

    packet[0] = SteelSeries::CMD_SET_COLOR;
    packet[1] = 0x00;

    // Zone 0 (Logo) - bytes 2-4
    packet[2] = color.r;
    packet[3] = color.g;
    packet[4] = color.b;

    // Zone 1 (Wheel) - bytes 5-7
    packet[5] = color.r;
    packet[6] = color.g;
    packet[7] = color.b;

    return SendRawPacket(packet, sizeof(packet));
}

Result SteelSeriesRival::SetZoneColor(int zone, const RGB& color) {
    if (zone < 0 || zone >= SteelSeries::ZONE_COUNT) {
        return Result{ResultCode::InvalidParameter, "Invalid zone"};
    }

    m_zoneColors[zone] = color;

    // Send color update
    uint8_t packet[SteelSeries::PACKET_SIZE] = {0};

    packet[0] = SteelSeries::CMD_SET_COLOR;
    packet[1] = 0x00;

    // Zone 0 (Logo)
    packet[2] = m_zoneColors[0].r;
    packet[3] = m_zoneColors[0].g;
    packet[4] = m_zoneColors[0].b;

    // Zone 1 (Wheel)
    packet[5] = m_zoneColors[1].r;
    packet[6] = m_zoneColors[1].g;
    packet[7] = m_zoneColors[1].b;

    return SendRawPacket(packet, sizeof(packet));
}

Result SteelSeriesRival::SetMode(DeviceMode mode) {
    m_state.currentMode = mode;

    // SteelSeries mode changes are handled by SteelSeries Engine
    // We only support static color control directly
    return Result::Success();
}

Result SteelSeriesRival::SetBrightness(uint8_t brightness) {
    m_state.brightness = brightness;
    // Not supported on Rival 600
    return Result::NotSupported();
}

Result SteelSeriesRival::SetSpeed(uint8_t speed) {
    m_state.speed = speed;
    // Not supported on Rival 600
    return Result::NotSupported();
}

Result SteelSeriesRival::Apply() {
    // Changes are applied immediately
    return Result::Success();
}

} // namespace Plugins
} // namespace OCRGB
