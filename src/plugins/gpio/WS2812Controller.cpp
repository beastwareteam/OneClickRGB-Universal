//=============================================================================
// OneClickRGB-Universal - WS2812B LED Strip Controller Implementation
//=============================================================================

#include "WS2812Controller.h"

namespace OCRGB {
namespace Plugins {

//=============================================================================
// WS2812 Controller
//=============================================================================

WS2812Controller::WS2812Controller(int ledCount, int gpioPin)
    : GPIODevice(
        "ws2812-gpio" + std::to_string(gpioPin),
        "WS2812B LED Strip (GPIO" + std::to_string(gpioPin) + ")",
        GPIOConfig{
            GPIOLedType::WS2812B,
            ColorOrder::GRB,
            gpioPin,        // dataPin
            "",             // spiDevice (not used)
            0,              // spiSpeedHz (not used)
            0, 0, 0, -1,    // PWM pins (not used)
            800,            // pwmFrequency (not used)
            ledCount,       // ledCount
            255,            // globalBrightness
            10              // dmaChannel
        })
{
}

WS2812Controller::~WS2812Controller() {
    Shutdown();
}

std::unique_ptr<IDevice> WS2812Controller::Create(int ledCount, int gpioPin) {
    return std::make_unique<WS2812Controller>(ledCount, gpioPin);
}

Result WS2812Controller::InitializeHardware() {
    m_bridge = GPIOBridgeFactory::Create(GPIOLedType::WS2812B);

    if (!m_bridge) {
        return Result::Error(ResultCode::NotSupported, "WS2812B not supported on this platform");
    }

    if (!m_bridge->Initialize(m_config)) {
        return Result::Error(ResultCode::Error, m_bridge->GetLastError());
    }

    return Result::Success();
}

Result WS2812Controller::ShutdownHardware() {
    if (m_bridge) {
        m_bridge->Shutdown();
        m_bridge.reset();
    }
    return Result::Success();
}

Result WS2812Controller::WriteToHardware() {
    if (!m_bridge) {
        return Result::Error(ResultCode::NotConnected, "Bridge not initialized");
    }

    if (!m_bridge->WriteLeds(m_ledBuffer, m_config)) {
        return Result::Error(ResultCode::CommunicationError, m_bridge->GetLastError());
    }

    return Result::Success();
}

//=============================================================================
// APA102 Controller
//=============================================================================

APA102Controller::APA102Controller(int ledCount, const std::string& spiDevice)
    : GPIODevice(
        "apa102-spi",
        "APA102 LED Strip (SPI)",
        GPIOConfig{
            GPIOLedType::APA102,
            ColorOrder::BGR,    // APA102 uses BGR
            0,                  // dataPin (not used)
            spiDevice,          // spiDevice
            8000000,            // spiSpeedHz (8 MHz)
            0, 0, 0, -1,        // PWM pins (not used)
            800,                // pwmFrequency (not used)
            ledCount,           // ledCount
            255,                // globalBrightness
            0                   // dmaChannel (not used)
        })
{
}

APA102Controller::~APA102Controller() {
    Shutdown();
}

std::unique_ptr<IDevice> APA102Controller::Create(int ledCount, const std::string& spiDevice) {
    return std::make_unique<APA102Controller>(ledCount, spiDevice);
}

Result APA102Controller::InitializeHardware() {
    m_bridge = GPIOBridgeFactory::Create(GPIOLedType::APA102);

    if (!m_bridge) {
        return Result::Error(ResultCode::NotSupported, "APA102 not supported on this platform");
    }

    if (!m_bridge->Initialize(m_config)) {
        return Result::Error(ResultCode::Error, m_bridge->GetLastError());
    }

    return Result::Success();
}

Result APA102Controller::ShutdownHardware() {
    if (m_bridge) {
        m_bridge->Shutdown();
        m_bridge.reset();
    }
    return Result::Success();
}

Result APA102Controller::WriteToHardware() {
    if (!m_bridge) {
        return Result::Error(ResultCode::NotConnected, "Bridge not initialized");
    }

    if (!m_bridge->WriteLeds(m_ledBuffer, m_config)) {
        return Result::Error(ResultCode::CommunicationError, m_bridge->GetLastError());
    }

    return Result::Success();
}

//=============================================================================
// PWM RGB Controller
//=============================================================================

PWMRGBController::PWMRGBController(int redPin, int greenPin, int bluePin)
    : GPIODevice(
        "pwm-rgb",
        "PWM RGB LED",
        GPIOConfig{
            GPIOLedType::PWM_RGB,
            ColorOrder::RGB,
            0,                  // dataPin (not used)
            "",                 // spiDevice (not used)
            0,                  // spiSpeedHz (not used)
            redPin,             // redPin
            greenPin,           // greenPin
            bluePin,            // bluePin
            -1,                 // whitePin (not used)
            800,                // pwmFrequency
            1,                  // ledCount (single LED)
            255,                // globalBrightness
            0                   // dmaChannel (not used)
        })
{
}

PWMRGBController::~PWMRGBController() {
    Shutdown();
}

std::unique_ptr<IDevice> PWMRGBController::Create(int redPin, int greenPin, int bluePin) {
    return std::make_unique<PWMRGBController>(redPin, greenPin, bluePin);
}

Result PWMRGBController::InitializeHardware() {
    m_bridge = GPIOBridgeFactory::Create(GPIOLedType::PWM_RGB);

    if (!m_bridge) {
        return Result::Error(ResultCode::NotSupported, "PWM RGB not supported on this platform");
    }

    if (!m_bridge->Initialize(m_config)) {
        return Result::Error(ResultCode::Error, m_bridge->GetLastError());
    }

    return Result::Success();
}

Result PWMRGBController::ShutdownHardware() {
    if (m_bridge) {
        m_bridge->Shutdown();
        m_bridge.reset();
    }
    return Result::Success();
}

Result PWMRGBController::WriteToHardware() {
    if (!m_bridge) {
        return Result::Error(ResultCode::NotConnected, "Bridge not initialized");
    }

    if (!m_bridge->WriteLeds(m_ledBuffer, m_config)) {
        return Result::Error(ResultCode::CommunicationError, m_bridge->GetLastError());
    }

    return Result::Success();
}

} // namespace Plugins
} // namespace OCRGB
