//=============================================================================
// OneClickRGB-Universal - EVision Keyboard Implementation
//=============================================================================

#include "EVisionKeyboard.h"
#include <cstring>

namespace OCRGB {
namespace Plugins {

EVisionKeyboard::EVisionKeyboard() {
    // Set device identifiers
    SetIdentifiers(EVision::VENDOR_ID, EVision::PRODUCT_ID, EVision::USAGE_PAGE);

    // Set device info
    SetDeviceInfo("EVision RGB Keyboard", "EVision", DeviceType::Keyboard);

    // Set capabilities
    Capabilities caps;
    caps.supportsColor = true;
    caps.supportsBrightness = true;
    caps.supportsSpeed = true;
    caps.zoneCount = 1;
    caps.ledsPerZone = 1;  // Keyboard is one zone
    caps.supportedModes = {
        DeviceMode::Static,
        DeviceMode::Breathing,
        DeviceMode::Wave,
        DeviceMode::Spectrum,
        DeviceMode::Reactive
    };
    SetCapabilities(caps);
}

Result EVisionKeyboard::OnConnected() {
    // No special initialization needed
    return Result::Success();
}

Result EVisionKeyboard::SendCommand(uint8_t cmd, uint8_t value) {
    uint8_t packet[EVision::PACKET_SIZE] = {0};

    packet[0] = EVision::REPORT_ID;
    packet[1] = EVision::CMD_PREFIX;
    packet[2] = cmd;
    packet[3] = value;

    return SendRawPacket(packet, sizeof(packet));
}

Result EVisionKeyboard::SendCommand(uint8_t cmd, uint8_t v1, uint8_t v2, uint8_t v3) {
    uint8_t packet[EVision::PACKET_SIZE] = {0};

    packet[0] = EVision::REPORT_ID;
    packet[1] = EVision::CMD_PREFIX;
    packet[2] = cmd;
    packet[3] = v1;
    packet[4] = v2;
    packet[5] = v3;

    return SendRawPacket(packet, sizeof(packet));
}

Result EVisionKeyboard::SetColor(const RGB& color) {
    m_state.currentColor = color;

    // Send color command
    return SendCommand(EVision::CMD_SET_COLOR, color.r, color.g, color.b);
}

Result EVisionKeyboard::SetMode(DeviceMode mode) {
    uint8_t modeByte = ModeToEVisionByte(mode);

    Result result = SendCommand(EVision::CMD_SET_MODE, modeByte);
    if (result.IsSuccess()) {
        m_currentMode = modeByte;
        m_state.currentMode = mode;
    }

    return result;
}

Result EVisionKeyboard::SetBrightness(uint8_t brightness) {
    m_state.brightness = brightness;

    // EVision uses 0-50 scale for brightness
    uint8_t scaled = (brightness * 50) / 100;
    return SendCommand(EVision::CMD_SET_BRIGHTNESS, scaled);
}

Result EVisionKeyboard::SetSpeed(uint8_t speed) {
    m_state.speed = speed;

    // EVision uses 0-10 scale for speed
    uint8_t scaled = (speed * 10) / 100;
    return SendCommand(EVision::CMD_SET_SPEED, scaled);
}

Result EVisionKeyboard::Apply() {
    // EVision applies changes immediately, no explicit apply needed
    return Result::Success();
}

uint8_t EVisionKeyboard::ModeToEVisionByte(DeviceMode mode) {
    switch (mode) {
        case DeviceMode::Static:    return EVision::MODE_STATIC;
        case DeviceMode::Breathing: return EVision::MODE_BREATHING;
        case DeviceMode::Wave:      return EVision::MODE_WAVE_SHORT;
        case DeviceMode::Spectrum:  return EVision::MODE_SPECTRUM;
        case DeviceMode::Reactive:  return EVision::MODE_REACTIVE;
        default:                    return EVision::MODE_STATIC;
    }
}

} // namespace Plugins
} // namespace OCRGB
