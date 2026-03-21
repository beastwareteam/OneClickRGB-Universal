//=============================================================================
// OneClickRGB-Universal - ASUS Aura Controller Implementation
//=============================================================================

#include "AsusAuraController.h"
#include <cstring>
#include <thread>
#include <chrono>

namespace OCRGB {
namespace Plugins {

AsusAuraController::AsusAuraController() {
    // Set device identifiers
    SetIdentifiers(AsusAura::VENDOR_ID, AsusAura::PRODUCT_ID, AsusAura::USAGE_PAGE);

    // Set device info
    SetDeviceInfo("ASUS Aura Mainboard", "ASUS", DeviceType::Mainboard);

    // Set capabilities
    Capabilities caps;
    caps.supportsColor = true;
    caps.supportsBrightness = true;
    caps.supportsSpeed = true;
    caps.supportsPerLedColor = true;
    caps.zoneCount = AsusAura::MAX_CHANNELS;
    caps.ledsPerZone = AsusAura::LEDS_PER_CHANNEL;
    caps.totalLeds = AsusAura::MAX_CHANNELS * AsusAura::LEDS_PER_CHANNEL;
    caps.supportedModes = {
        DeviceMode::Static,
        DeviceMode::Breathing,
        DeviceMode::Wave,
        DeviceMode::Spectrum,
        DeviceMode::Reactive
    };
    SetCapabilities(caps);

    // Initialize all LEDs to off
    memset(m_channelColors, 0, sizeof(m_channelColors));
}

Result AsusAuraController::OnConnected() {
    // Put device in direct control mode
    return SetDirectMode();
}

void AsusAuraController::OnDisconnecting() {
    // Nothing special needed
}

Result AsusAuraController::SetDirectMode() {
    uint8_t packet[AsusAura::PACKET_SIZE] = {0};

    packet[0] = AsusAura::REPORT_ID;
    packet[1] = AsusAura::CMD_SET_DIRECT;

    Result result = SendRawPacket(packet, sizeof(packet));
    if (result.IsSuccess()) {
        m_directMode = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    return result;
}

Result AsusAuraController::SetColor(const RGB& color) {
    // Set all LEDs to the same color
    for (int ch = 0; ch < AsusAura::MAX_CHANNELS; ch++) {
        for (int led = 0; led < AsusAura::LEDS_PER_CHANNEL; led++) {
            m_channelColors[ch][led] = color;
        }
    }

    m_state.currentColor = color;
    return SendColorData();
}

Result AsusAuraController::SetZoneColor(int zone, const RGB& color) {
    if (zone < 0 || zone >= AsusAura::MAX_CHANNELS) {
        return Result{ResultCode::InvalidParameter, "Invalid zone"};
    }

    for (int led = 0; led < AsusAura::LEDS_PER_CHANNEL; led++) {
        m_channelColors[zone][led] = color;
    }

    return SendColorData();
}

Result AsusAuraController::SendColorData() {
    if (!m_directMode) {
        Result result = SetDirectMode();
        if (result.IsError()) return result;
    }

    // ASUS Aura expects GRB order for addressable LEDs
    // Send 60 LEDs per packet, one packet per channel

    for (int ch = 0; ch < AsusAura::MAX_CHANNELS; ch++) {
        uint8_t packet[AsusAura::PACKET_SIZE] = {0};

        packet[0] = AsusAura::REPORT_ID;
        packet[1] = AsusAura::CMD_SET_DIRECT;
        packet[2] = (uint8_t)ch;  // Channel number
        packet[3] = 0x00;         // Start LED

        // Pack LED colors (GRB format, 3 bytes per LED)
        // We can fit about 20 LEDs per packet (60 bytes of color data)
        int ledsPerPacket = 20;
        int packetCount = (AsusAura::LEDS_PER_CHANNEL + ledsPerPacket - 1) / ledsPerPacket;

        for (int p = 0; p < packetCount; p++) {
            memset(packet + 4, 0, AsusAura::PACKET_SIZE - 4);

            int startLed = p * ledsPerPacket;
            int endLed = std::min(startLed + ledsPerPacket, AsusAura::LEDS_PER_CHANNEL);

            packet[3] = (uint8_t)startLed;

            int offset = 4;
            for (int led = startLed; led < endLed; led++) {
                const RGB& c = m_channelColors[ch][led];
                // GRB order
                packet[offset++] = c.g;
                packet[offset++] = c.r;
                packet[offset++] = c.b;
            }

            Result result = SendRawPacket(packet, sizeof(packet));
            if (result.IsError()) return result;

            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    return Result::Success();
}

Result AsusAuraController::SetMode(DeviceMode mode) {
    uint8_t modeByte = ModeToAsusByte(mode);

    uint8_t packet[AsusAura::PACKET_SIZE] = {0};
    packet[0] = AsusAura::REPORT_ID;
    packet[1] = AsusAura::CMD_SET_MODE;
    packet[2] = 0x00;
    packet[3] = 0x00;
    packet[4] = modeByte;
    packet[5] = m_speed;
    packet[6] = 0x00;  // Direction
    packet[7] = 0x00;  // Color mode (0 = single color)

    Result result = SendRawPacket(packet, sizeof(packet));
    if (result.IsSuccess()) {
        m_currentMode = modeByte;
        m_state.currentMode = mode;
        m_directMode = false;  // Mode change exits direct mode
    }

    return result;
}

Result AsusAuraController::SetBrightness(uint8_t brightness) {
    m_brightness = brightness;
    m_state.brightness = brightness;

    // Brightness is typically applied through color values
    // Scale current colors by brightness
    return Result::Success();
}

Result AsusAuraController::SetSpeed(uint8_t speed) {
    m_speed = speed;
    m_state.speed = speed;

    // Speed is sent with mode changes
    return Result::Success();
}

Result AsusAuraController::Apply() {
    // Send commit packet
    uint8_t packet[AsusAura::PACKET_SIZE] = {0};
    packet[0] = AsusAura::REPORT_ID;
    packet[1] = AsusAura::CMD_COMMIT;
    packet[2] = 0x00;

    return SendRawPacket(packet, sizeof(packet));
}

uint8_t AsusAuraController::ModeToAsusByte(DeviceMode mode) {
    switch (mode) {
        case DeviceMode::Static:    return AsusAura::MODE_STATIC;
        case DeviceMode::Breathing: return AsusAura::MODE_BREATHING;
        case DeviceMode::Wave:      return AsusAura::MODE_WAVE;
        case DeviceMode::Spectrum:  return AsusAura::MODE_SPECTRUM;
        case DeviceMode::Reactive:  return AsusAura::MODE_REACTIVE;
        default:                    return AsusAura::MODE_STATIC;
    }
}

} // namespace Plugins
} // namespace OCRGB
