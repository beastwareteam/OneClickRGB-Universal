#pragma once
//=============================================================================
// OneClickRGB-Universal - GPIO Bridge
//=============================================================================
// Protocol bridge for GPIO-based LEDs (Raspberry Pi)
// Supports: WS2812B, APA102, PWM
//=============================================================================

#include "IBridge.h"
#include "../devices/GPIODevice.h"
#include <vector>
#include <memory>

namespace OCRGB {

//=============================================================================
// GPIO Bridge Interface
//=============================================================================

class IGPIOBridge {
public:
    virtual ~IGPIOBridge() = default;

    virtual bool Initialize(const GPIOConfig& config) = 0;
    virtual void Shutdown() = 0;
    virtual bool IsAvailable() const = 0;

    // Write LED data
    virtual bool WriteLeds(const std::vector<RGB>& leds, const GPIOConfig& config) = 0;

    // PWM control (for simple RGB LEDs)
    virtual bool SetPWM(int pin, uint8_t value) = 0;

    // Get error message
    virtual std::string GetLastError() const = 0;
};

//=============================================================================
// WS2812B Bridge (uses rpi_ws281x library)
//=============================================================================

#ifdef OCRGB_PLATFORM_LINUX

class WS2812Bridge : public IGPIOBridge {
public:
    WS2812Bridge();
    ~WS2812Bridge() override;

    bool Initialize(const GPIOConfig& config) override;
    void Shutdown() override;
    bool IsAvailable() const override;

    bool WriteLeds(const std::vector<RGB>& leds, const GPIOConfig& config) override;
    bool SetPWM(int pin, uint8_t value) override { return false; }  // Not applicable

    std::string GetLastError() const override { return m_lastError; }

private:
    bool m_initialized = false;
    std::string m_lastError;
    void* m_ledString = nullptr;  // ws2811_t*
};

//=============================================================================
// APA102 Bridge (uses SPI)
//=============================================================================

class APA102Bridge : public IGPIOBridge {
public:
    APA102Bridge();
    ~APA102Bridge() override;

    bool Initialize(const GPIOConfig& config) override;
    void Shutdown() override;
    bool IsAvailable() const override;

    bool WriteLeds(const std::vector<RGB>& leds, const GPIOConfig& config) override;
    bool SetPWM(int pin, uint8_t value) override { return false; }

    std::string GetLastError() const override { return m_lastError; }

private:
    bool m_initialized = false;
    int m_spiFd = -1;
    std::string m_lastError;
};

//=============================================================================
// PWM Bridge (uses pigpio or sysfs)
//=============================================================================

class PWMBridge : public IGPIOBridge {
public:
    PWMBridge();
    ~PWMBridge() override;

    bool Initialize(const GPIOConfig& config) override;
    void Shutdown() override;
    bool IsAvailable() const override;

    bool WriteLeds(const std::vector<RGB>& leds, const GPIOConfig& config) override;
    bool SetPWM(int pin, uint8_t value) override;

    std::string GetLastError() const override { return m_lastError; }

private:
    bool m_initialized = false;
    bool m_usePigpio = false;
    std::string m_lastError;

    GPIOConfig m_config;

    // sysfs PWM helpers
    bool ExportPWM(int pin);
    bool SetPWMViaSysfs(int pin, uint8_t value);
};

#endif // OCRGB_PLATFORM_LINUX

//=============================================================================
// GPIO Bridge Factory
//=============================================================================

class GPIOBridgeFactory {
public:
    static std::unique_ptr<IGPIOBridge> Create(GPIOLedType type);
};

} // namespace OCRGB
