#pragma once
//=============================================================================
// OneClickRGB-Universal - WS2812B LED Strip Controller
//=============================================================================
// Plugin for WS2812B/NeoPixel LED strips on Raspberry Pi
//=============================================================================

#include "../../devices/GPIODevice.h"
#include "../../bridges/GPIOBridge.h"
#include <memory>

namespace OCRGB {
namespace Plugins {

class WS2812Controller : public GPIODevice {
public:
    WS2812Controller(int ledCount = 60, int gpioPin = 18);
    ~WS2812Controller() override;

    // Factory method for PluginFactory
    static std::unique_ptr<IDevice> Create(int ledCount, int gpioPin);

protected:
    Result InitializeHardware() override;
    Result ShutdownHardware() override;
    Result WriteToHardware() override;

private:
    std::unique_ptr<IGPIOBridge> m_bridge;
};

//=============================================================================
// APA102 Controller (DotStar)
//=============================================================================

class APA102Controller : public GPIODevice {
public:
    APA102Controller(int ledCount = 30, const std::string& spiDevice = "/dev/spidev0.0");
    ~APA102Controller() override;

    static std::unique_ptr<IDevice> Create(int ledCount, const std::string& spiDevice);

protected:
    Result InitializeHardware() override;
    Result ShutdownHardware() override;
    Result WriteToHardware() override;

private:
    std::unique_ptr<IGPIOBridge> m_bridge;
};

//=============================================================================
// PWM RGB LED Controller
//=============================================================================

class PWMRGBController : public GPIODevice {
public:
    PWMRGBController(int redPin = 12, int greenPin = 13, int bluePin = 19);
    ~PWMRGBController() override;

    static std::unique_ptr<IDevice> Create(int redPin, int greenPin, int bluePin);

protected:
    Result InitializeHardware() override;
    Result ShutdownHardware() override;
    Result WriteToHardware() override;

private:
    std::unique_ptr<IGPIOBridge> m_bridge;
};

} // namespace Plugins
} // namespace OCRGB
