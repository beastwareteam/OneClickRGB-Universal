//=============================================================================
// OneClickRGB-Universal - GPIO Device Implementation
//=============================================================================

#include "GPIODevice.h"
#include <algorithm>
#include <cstring>

namespace OCRGB {

//=============================================================================
// Constructor / Destructor
//=============================================================================

GPIODevice::GPIODevice(const std::string& id, const std::string& name, const GPIOConfig& config)
    : m_id(id)
    , m_name(name)
    , m_config(config)
{
    m_ledBuffer.resize(config.ledCount, RGB{0, 0, 0});
}

GPIODevice::~GPIODevice() {
    Shutdown();
}

//=============================================================================
// IDevice Interface
//=============================================================================

Result GPIODevice::Initialize() {
    if (m_initialized) {
        return Result::Success();
    }

    Result result = InitializeHardware();
    if (!result.IsSuccess()) {
        return result;
    }

    m_initialized = true;
    m_connected = true;

    return Result::Success();
}

Result GPIODevice::Shutdown() {
    if (!m_initialized) {
        return Result::Success();
    }

    // Turn off all LEDs
    std::fill(m_ledBuffer.begin(), m_ledBuffer.end(), RGB{0, 0, 0});
    WriteToHardware();

    ShutdownHardware();

    m_initialized = false;
    m_connected = false;

    return Result::Success();
}

bool GPIODevice::IsConnected() const {
    return m_connected;
}

bool GPIODevice::IsReady() const {
    return m_initialized && m_connected;
}

Result GPIODevice::SetColor(const RGB& color) {
    std::fill(m_ledBuffer.begin(), m_ledBuffer.end(), color);
    return Result::Success();
}

Result GPIODevice::SetMode(DeviceMode mode) {
    m_currentMode = mode;

    if (mode == DeviceMode::Off) {
        std::fill(m_ledBuffer.begin(), m_ledBuffer.end(), RGB{0, 0, 0});
    }

    return Result::Success();
}

Result GPIODevice::SetBrightness(uint8_t brightness) {
    m_brightness = brightness;
    m_config.globalBrightness = static_cast<uint8_t>((brightness * 255) / 100);
    return Result::Success();
}

Result GPIODevice::Apply() {
    if (!IsReady()) {
        return Result::Error(ResultCode::NotConnected, "GPIO device not ready");
    }

    return WriteToHardware();
}

DeviceInfo GPIODevice::GetInfo() const {
    DeviceInfo info;
    info.id = m_id;
    info.name = m_name;
    info.vendor = "GPIO";

    switch (m_config.ledType) {
        case GPIOLedType::WS2812B:
            info.type = "WS2812B";
            break;
        case GPIOLedType::WS2811:
            info.type = "WS2811";
            break;
        case GPIOLedType::SK6812:
        case GPIOLedType::SK6812_RGBW:
            info.type = "SK6812";
            break;
        case GPIOLedType::APA102:
            info.type = "APA102";
            break;
        case GPIOLedType::PWM_RGB:
        case GPIOLedType::PWM_RGBW:
            info.type = "PWM";
            break;
    }

    info.firmwareVersion = "N/A";
    info.protocol = "GPIO";

    return info;
}

Capabilities GPIODevice::GetCapabilities() const {
    Capabilities caps;

    caps.supportsColor = true;
    caps.supportsBrightness = true;
    caps.supportsSpeed = true;
    caps.supportsDirection = true;

    // Addressable LEDs support per-LED control
    caps.supportsPerLedColor = (m_config.ledType != GPIOLedType::PWM_RGB &&
                                 m_config.ledType != GPIOLedType::PWM_RGBW);

    caps.supportedModes = {
        DeviceMode::Off,
        DeviceMode::Static,
        DeviceMode::Breathing,
        DeviceMode::Wave,
        DeviceMode::Spectrum,
        DeviceMode::ColorCycle
    };

    caps.zoneCount = 1;
    caps.ledsPerZone = m_config.ledCount;
    caps.totalLeds = m_config.ledCount;

    caps.usesRGBW = (m_config.ledType == GPIOLedType::SK6812_RGBW ||
                     m_config.ledType == GPIOLedType::PWM_RGBW);
    caps.usesGRB = (m_config.colorOrder == ColorOrder::GRB);

    caps.minBrightness = 0;
    caps.maxBrightness = 100;
    caps.minSpeed = 0;
    caps.maxSpeed = 100;

    return caps;
}

DeviceState GPIODevice::GetState() const {
    DeviceState state;
    state.currentMode = m_currentMode;
    state.brightness = m_brightness;
    state.speed = 50;

    // Return first LED color as representative
    if (!m_ledBuffer.empty()) {
        state.currentColor = m_ledBuffer[0];
    }

    return state;
}

//=============================================================================
// GPIO-specific Methods
//=============================================================================

Result GPIODevice::SetLedColor(int index, const RGB& color) {
    if (index < 0 || index >= static_cast<int>(m_ledBuffer.size())) {
        return Result::Error(ResultCode::InvalidParameter, "LED index out of range");
    }

    m_ledBuffer[index] = color;
    return Result::Success();
}

Result GPIODevice::SetGradient(const RGB& start, const RGB& end) {
    int count = static_cast<int>(m_ledBuffer.size());

    for (int i = 0; i < count; ++i) {
        float t = (count > 1) ? static_cast<float>(i) / (count - 1) : 0.0f;

        m_ledBuffer[i].r = static_cast<uint8_t>(start.r + t * (end.r - start.r));
        m_ledBuffer[i].g = static_cast<uint8_t>(start.g + t * (end.g - start.g));
        m_ledBuffer[i].b = static_cast<uint8_t>(start.b + t * (end.b - start.b));
    }

    return Result::Success();
}

Result GPIODevice::SetRange(int startIndex, int count, const RGB& color) {
    int endIndex = std::min(startIndex + count, static_cast<int>(m_ledBuffer.size()));

    for (int i = startIndex; i < endIndex; ++i) {
        m_ledBuffer[i] = color;
    }

    return Result::Success();
}

//=============================================================================
// Color Order Conversion
//=============================================================================

void GPIODevice::ApplyColorOrder(const RGB& in, uint8_t* out) const {
    switch (m_config.colorOrder) {
        case ColorOrder::RGB:
            out[0] = in.r; out[1] = in.g; out[2] = in.b;
            break;
        case ColorOrder::RBG:
            out[0] = in.r; out[1] = in.b; out[2] = in.g;
            break;
        case ColorOrder::GRB:
            out[0] = in.g; out[1] = in.r; out[2] = in.b;
            break;
        case ColorOrder::GBR:
            out[0] = in.g; out[1] = in.b; out[2] = in.r;
            break;
        case ColorOrder::BRG:
            out[0] = in.b; out[1] = in.r; out[2] = in.g;
            break;
        case ColorOrder::BGR:
            out[0] = in.b; out[1] = in.g; out[2] = in.r;
            break;
    }

    // Apply brightness
    float brightness = m_config.globalBrightness / 255.0f;
    out[0] = static_cast<uint8_t>(out[0] * brightness);
    out[1] = static_cast<uint8_t>(out[1] * brightness);
    out[2] = static_cast<uint8_t>(out[2] * brightness);
}

//=============================================================================
// Platform-specific Implementation (Stubs - Override in platform code)
//=============================================================================

Result GPIODevice::InitializeHardware() {
    // Platform-specific initialization
    // Implemented in platform/linux/GPIOBridge.cpp for Raspberry Pi
    return Result::Success();
}

Result GPIODevice::ShutdownHardware() {
    return Result::Success();
}

Result GPIODevice::WriteToHardware() {
    // Platform-specific write
    // Implemented in platform/linux/GPIOBridge.cpp for Raspberry Pi
    return Result::Success();
}

} // namespace OCRGB
