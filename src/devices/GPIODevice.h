#pragma once
//=============================================================================
// OneClickRGB-Universal - GPIO Device Base Class
//=============================================================================
// Base class for GPIO-based RGB devices (Raspberry Pi, etc.)
// Supports WS2812B, APA102, PWM LEDs
//=============================================================================

#include "IDevice.h"
#include "../core/Types.h"
#include <vector>
#include <string>

namespace OCRGB {

//=============================================================================
// GPIO LED Types
//=============================================================================

enum class GPIOLedType {
    WS2812B,        // NeoPixel (1-wire, 800kHz)
    WS2811,         // Similar to WS2812B
    SK6812,         // RGBW variant
    SK6812_RGBW,    // Explicit RGBW
    APA102,         // DotStar (SPI, clock + data)
    PWM_RGB,        // Simple PWM (3 pins)
    PWM_RGBW        // PWM with white channel
};

enum class ColorOrder {
    RGB,
    RBG,
    GRB,    // WS2812B default
    GBR,
    BRG,
    BGR
};

//=============================================================================
// GPIO Configuration
//=============================================================================

struct GPIOConfig {
    GPIOLedType ledType = GPIOLedType::WS2812B;
    ColorOrder colorOrder = ColorOrder::GRB;

    // For WS2812B/WS2811/SK6812
    int dataPin = 18;           // GPIO18 = PWM0

    // For APA102 (SPI)
    std::string spiDevice = "/dev/spidev0.0";
    int spiSpeedHz = 8000000;   // 8 MHz

    // For PWM RGB
    int redPin = 12;
    int greenPin = 13;
    int bluePin = 19;
    int whitePin = -1;          // -1 = not used
    int pwmFrequency = 800;

    // LED strip config
    int ledCount = 1;
    uint8_t globalBrightness = 255;

    // DMA channel for WS2812B (Pi specific)
    int dmaChannel = 10;
};

//=============================================================================
// GPIO Device Base Class
//=============================================================================

class GPIODevice : public IDevice {
public:
    GPIODevice(const std::string& id, const std::string& name, const GPIOConfig& config);
    ~GPIODevice() override;

    //=========================================================================
    // IDevice Interface
    //=========================================================================
    Result Initialize() override;
    Result Shutdown() override;
    bool IsConnected() const override;
    bool IsReady() const override;

    Result SetColor(const RGB& color) override;
    Result SetMode(DeviceMode mode) override;
    Result SetBrightness(uint8_t brightness) override;
    Result Apply() override;

    DeviceInfo GetInfo() const override;
    Capabilities GetCapabilities() const override;
    DeviceState GetState() const override;

    //=========================================================================
    // GPIO-specific Methods
    //=========================================================================

    // Set individual LED color (for addressable strips)
    Result SetLedColor(int index, const RGB& color);

    // Set all LEDs to gradient
    Result SetGradient(const RGB& start, const RGB& end);

    // Set LED range
    Result SetRange(int startIndex, int count, const RGB& color);

    // Get LED count
    int GetLedCount() const { return m_config.ledCount; }

    // Get LED type
    GPIOLedType GetLedType() const { return m_config.ledType; }

protected:
    std::string m_id;
    std::string m_name;
    GPIOConfig m_config;

    bool m_initialized = false;
    bool m_connected = false;

    DeviceMode m_currentMode = DeviceMode::Static;
    uint8_t m_brightness = 100;

    // LED buffer (for addressable LEDs)
    std::vector<RGB> m_ledBuffer;

    // Platform-specific implementation (in GPIODevice.cpp)
    virtual Result InitializeHardware();
    virtual Result ShutdownHardware();
    virtual Result WriteToHardware();

    // Color order conversion
    void ApplyColorOrder(const RGB& in, uint8_t* out) const;
};

} // namespace OCRGB
