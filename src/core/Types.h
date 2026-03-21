#pragma once
//=============================================================================
// OneClickRGB-Universal - Core Type Definitions
//=============================================================================

#include <cstdint>
#include <string>
#include <vector>

namespace OCRGB {

//-----------------------------------------------------------------------------
// Version
//-----------------------------------------------------------------------------
constexpr const char* APP_NAME = "OneClickRGB-Universal";
constexpr const char* APP_VERSION = "1.0.0";
constexpr const wchar_t* APP_NAME_W = L"OneClickRGB-Universal";
constexpr const wchar_t* APP_VERSION_W = L"1.0.0";

//-----------------------------------------------------------------------------
// Color Types
//-----------------------------------------------------------------------------
struct RGB {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    RGB() = default;
    RGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}

    bool operator==(const RGB& other) const {
        return r == other.r && g == other.g && b == other.b;
    }

    static RGB FromHex(uint32_t hex) {
        return RGB((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
    }

    uint32_t ToHex() const {
        return (r << 16) | (g << 8) | b;
    }
};

struct RGBW {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t w = 0;
};

//-----------------------------------------------------------------------------
// Device Modes
//-----------------------------------------------------------------------------
enum class DeviceMode : uint8_t {
    Off = 0,
    Static,
    Breathing,
    Wave,
    Spectrum,      // Rainbow cycle
    Reactive,      // React to input
    ColorCycle,
    Gradient,
    Custom         // User-defined
};

inline const char* DeviceModeToString(DeviceMode mode) {
    switch (mode) {
        case DeviceMode::Off: return "Off";
        case DeviceMode::Static: return "Static";
        case DeviceMode::Breathing: return "Breathing";
        case DeviceMode::Wave: return "Wave";
        case DeviceMode::Spectrum: return "Spectrum";
        case DeviceMode::Reactive: return "Reactive";
        case DeviceMode::ColorCycle: return "Color Cycle";
        case DeviceMode::Gradient: return "Gradient";
        case DeviceMode::Custom: return "Custom";
        default: return "Unknown";
    }
}

//-----------------------------------------------------------------------------
// Device Types
//-----------------------------------------------------------------------------
enum class DeviceType : uint8_t {
    Unknown = 0,
    Mainboard,
    GPU,
    RAM,
    Keyboard,
    Mouse,
    Mousepad,
    Headset,
    Controller,
    LEDStrip,
    Fan,
    AIO,           // All-in-one cooler
    Case,
    Other
};

inline const char* DeviceTypeToString(DeviceType type) {
    switch (type) {
        case DeviceType::Mainboard: return "Mainboard";
        case DeviceType::GPU: return "GPU";
        case DeviceType::RAM: return "RAM";
        case DeviceType::Keyboard: return "Keyboard";
        case DeviceType::Mouse: return "Mouse";
        case DeviceType::Mousepad: return "Mousepad";
        case DeviceType::Headset: return "Headset";
        case DeviceType::Controller: return "Controller";
        case DeviceType::LEDStrip: return "LED Strip";
        case DeviceType::Fan: return "Fan";
        case DeviceType::AIO: return "AIO Cooler";
        case DeviceType::Case: return "Case";
        case DeviceType::Other: return "Other";
        default: return "Unknown";
    }
}

//-----------------------------------------------------------------------------
// Protocol Types
//-----------------------------------------------------------------------------
enum class ProtocolType : uint8_t {
    Unknown = 0,
    HID,           // USB HID (most common)
    SMBus,         // System Management Bus (RAM, some mainboards)
    I2C,           // I2C bus
    USB_Bulk,      // Raw USB bulk transfer
    USB_Interrupt, // USB interrupt transfer
    Serial,        // Serial/COM port
    Network        // Network-based (Philips Hue, etc.)
};

//-----------------------------------------------------------------------------
// Device Address
//-----------------------------------------------------------------------------
struct DeviceAddress {
    ProtocolType protocol = ProtocolType::Unknown;

    // HID/USB identifiers
    uint16_t vendorId = 0;
    uint16_t productId = 0;
    uint16_t usagePage = 0;
    uint16_t usage = 0;
    std::wstring path;          // HID device path

    // SMBus/I2C identifiers
    uint8_t busNumber = 0;
    uint8_t deviceAddress = 0;  // 7-bit I2C address

    // Serial
    std::string comPort;
    uint32_t baudRate = 0;

    // Network
    std::string ipAddress;
    uint16_t port = 0;

    std::string ToString() const {
        char buf[256];
        switch (protocol) {
            case ProtocolType::HID:
                snprintf(buf, sizeof(buf), "HID:%04X:%04X", vendorId, productId);
                break;
            case ProtocolType::SMBus:
            case ProtocolType::I2C:
                snprintf(buf, sizeof(buf), "SMBus:%d:0x%02X", busNumber, deviceAddress);
                break;
            case ProtocolType::Serial:
                snprintf(buf, sizeof(buf), "Serial:%s@%d", comPort.c_str(), baudRate);
                break;
            case ProtocolType::Network:
                snprintf(buf, sizeof(buf), "Net:%s:%d", ipAddress.c_str(), port);
                break;
            default:
                return "Unknown";
        }
        return buf;
    }
};

//-----------------------------------------------------------------------------
// Device Capabilities
//-----------------------------------------------------------------------------
struct Capabilities {
    bool supportsColor = true;
    bool supportsBrightness = true;
    bool supportsSpeed = true;
    bool supportsDirection = false;
    bool supportsPerLedColor = false;     // Addressable LEDs

    std::vector<DeviceMode> supportedModes;

    int zoneCount = 1;
    int ledsPerZone = 1;
    int totalLeds = 1;

    // Color format
    bool usesRGBW = false;
    bool usesGRB = false;               // Some WS2812 strips use GRB order

    // Limits
    uint8_t minBrightness = 0;
    uint8_t maxBrightness = 100;
    uint8_t minSpeed = 0;
    uint8_t maxSpeed = 100;
};

//-----------------------------------------------------------------------------
// Device Info
//-----------------------------------------------------------------------------
struct DeviceInfo {
    std::string id;                      // Unique identifier
    std::string name;                    // Display name
    std::string vendor;                  // Manufacturer
    std::string model;                   // Model name
    std::string firmwareVersion;
    std::string serialNumber;

    DeviceType type = DeviceType::Unknown;
    DeviceAddress address;
    Capabilities capabilities;
};

//-----------------------------------------------------------------------------
// Device State
//-----------------------------------------------------------------------------
struct DeviceState {
    bool connected = false;
    bool initialized = false;
    bool enabled = true;

    RGB currentColor;
    DeviceMode currentMode = DeviceMode::Static;
    uint8_t brightness = 100;
    uint8_t speed = 50;

    // Per-zone colors (for addressable devices)
    std::vector<RGB> zoneColors;
};

//-----------------------------------------------------------------------------
// Result Types
//-----------------------------------------------------------------------------
enum class ResultCode : uint8_t {
    Success = 0,
    Error,
    NotConnected,
    NotSupported,
    InvalidParameter,
    Timeout,
    PermissionDenied,
    DeviceBusy,
    CommunicationError
};

struct Result {
    ResultCode code = ResultCode::Success;
    std::string message;

    bool IsSuccess() const { return code == ResultCode::Success; }
    bool IsError() const { return code != ResultCode::Success; }

    static Result Success() { return {ResultCode::Success, ""}; }
    static Result Error(const std::string& msg) { return {ResultCode::Error, msg}; }
    static Result NotConnected() { return {ResultCode::NotConnected, "Device not connected"}; }
    static Result NotSupported() { return {ResultCode::NotSupported, "Operation not supported"}; }
};

} // namespace OCRGB
