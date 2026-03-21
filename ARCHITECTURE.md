# OneClickRGB-Universal Architecture

## Overview

OneClickRGB-Universal is a modular, plugin-based RGB controller that supports any hardware through a unified abstraction layer.

```
┌─────────────────────────────────────────────────────────────────┐
│                        APPLICATION LAYER                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │   Main UI   │  │  Settings   │  │   Device Manager UI     │  │
│  └──────┬──────┘  └──────┬──────┘  └────────────┬────────────┘  │
└─────────┼────────────────┼──────────────────────┼───────────────┘
          │                │                      │
┌─────────▼────────────────▼──────────────────────▼───────────────┐
│                        CORE LAYER                                │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │  App State  │  │   Profiles  │  │    Device Registry      │  │
│  └─────────────┘  └─────────────┘  └────────────┬────────────┘  │
└─────────────────────────────────────────────────┼───────────────┘
                                                  │
┌─────────────────────────────────────────────────▼───────────────┐
│                    DEVICE ABSTRACTION LAYER                      │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    IDevice Interface                         ││
│  │  - Initialize() / Shutdown()                                 ││
│  │  - SetColor(r, g, b) / SetMode(mode) / SetBrightness(val)   ││
│  │  - GetInfo() / GetCapabilities()                             ││
│  └─────────────────────────────────────────────────────────────┘│
│                              │                                   │
│     ┌────────────────────────┼────────────────────────┐         │
│     ▼                        ▼                        ▼         │
│  ┌──────────┐          ┌──────────┐            ┌──────────┐     │
│  │HIDDevice │          │SMBusDevice│           │USBDevice │     │
│  └────┬─────┘          └────┬─────┘            └────┬─────┘     │
└───────┼─────────────────────┼───────────────────────┼───────────┘
        │                     │                       │
┌───────▼─────────────────────▼───────────────────────▼───────────┐
│                      PROTOCOL LAYER                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │ HID Bridge  │  │SMBus Bridge │  │     USB Bridge          │  │
│  │  (hidapi)   │  │  (PawnIO)   │  │   (libusb/WinUSB)       │  │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Directory Structure

```
OneClickRGB-Universal/
├── src/
│   ├── main.cpp                    # Entry point
│   ├── app/
│   │   ├── Application.h/.cpp      # Main application class
│   │   ├── Settings.h/.cpp         # App settings management
│   │   └── Profiles.h/.cpp         # Profile management
│   │
│   ├── core/
│   │   ├── DeviceRegistry.h/.cpp   # Device discovery & management
│   │   ├── DeviceInfo.h            # Device information structs
│   │   └── Capabilities.h          # Device capability flags
│   │
│   ├── devices/
│   │   ├── IDevice.h               # Abstract device interface
│   │   ├── HIDDevice.h/.cpp        # HID device base class
│   │   ├── SMBusDevice.h/.cpp      # SMBus device base class
│   │   └── USBDevice.h/.cpp        # USB device base class
│   │
│   ├── plugins/                    # Device implementations
│   │   ├── asus/
│   │   │   ├── AsusAuraController.h/.cpp
│   │   │   └── device.json         # Device definition
│   │   ├── steelseries/
│   │   │   ├── SteelSeriesRival.h/.cpp
│   │   │   └── device.json
│   │   ├── evision/
│   │   │   ├── EVisionKeyboard.h/.cpp
│   │   │   └── device.json
│   │   └── gskill/
│   │       ├── GSkillTridentZ5.h/.cpp
│   │       └── device.json
│   │
│   ├── bridges/
│   │   ├── IBridge.h               # Bridge interface
│   │   ├── HIDBridge.h/.cpp        # HID communication
│   │   ├── SMBusBridge.h/.cpp      # SMBus via PawnIO
│   │   └── USBBridge.h/.cpp        # Raw USB
│   │
│   ├── scanner/
│   │   ├── HardwareScanner.h/.cpp  # Scan for devices
│   │   ├── DeviceMatcher.h/.cpp    # Match against database
│   │   └── ProtocolAnalyzer.h/.cpp # Analyze unknown devices
│   │
│   └── ui/
│       ├── MainWindow.h/.cpp       # Main application window
│       ├── DevicePanel.h/.cpp      # Device list/status
│       ├── ColorPicker.h/.cpp      # Color selection
│       ├── SettingsDialog.h/.cpp   # Settings UI
│       └── DeviceWizard.h/.cpp     # New device setup wizard
│
├── config/
│   ├── hardware_db.json            # Known device database
│   ├── protocols/
│   │   ├── hid_templates.json      # HID protocol templates
│   │   ├── smbus_templates.json    # SMBus templates
│   │   └── usb_templates.json      # USB templates
│   └── user_devices.json           # User-defined devices
│
├── tools/
│   ├── generate_config.py          # Build-time config generator
│   ├── device_analyzer.py          # Device analysis tool
│   └── protocol_tester.py          # Protocol testing utility
│
├── dependencies/
│   ├── hidapi/                     # HID library
│   └── pawnio/                     # SMBus access
│
├── build/
│   ├── compile.bat                 # Build script
│   └── generated/
│       └── hardware_config.h       # Auto-generated device config
│
├── docs/
│   ├── ARCHITECTURE.md             # This file
│   ├── ADDING_DEVICES.md           # How to add new devices
│   ├── PROTOCOL_GUIDE.md           # Protocol documentation
│   └── API_REFERENCE.md            # API documentation
│
├── tests/
│   ├── test_devices.cpp            # Device tests
│   ├── test_protocols.cpp          # Protocol tests
│   └── mock_hardware.h             # Hardware mocking
│
└── README.md
```

## Core Interfaces

### IDevice Interface

```cpp
class IDevice {
public:
    virtual ~IDevice() = default;

    // Lifecycle
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool IsConnected() const = 0;

    // Device Info
    virtual DeviceInfo GetInfo() const = 0;
    virtual Capabilities GetCapabilities() const = 0;

    // Control
    virtual bool SetColor(uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual bool SetMode(DeviceMode mode) = 0;
    virtual bool SetBrightness(uint8_t brightness) = 0;
    virtual bool SetSpeed(uint8_t speed) = 0;

    // Multi-zone support
    virtual int GetZoneCount() const { return 1; }
    virtual bool SetZoneColor(int zone, uint8_t r, uint8_t g, uint8_t b) {
        return SetColor(r, g, b);
    }

    // Direct control (for advanced users)
    virtual bool SendRawPacket(const uint8_t* data, size_t len) = 0;
    virtual bool ReadResponse(uint8_t* buffer, size_t len) = 0;
};
```

### IBridge Interface

```cpp
class IBridge {
public:
    virtual ~IBridge() = default;

    virtual bool Open(const DeviceAddress& addr) = 0;
    virtual void Close() = 0;
    virtual bool IsOpen() const = 0;

    virtual bool Write(const uint8_t* data, size_t len) = 0;
    virtual bool Read(uint8_t* buffer, size_t len, int timeout_ms = 1000) = 0;

    // Feature reports (HID specific)
    virtual bool SendFeatureReport(const uint8_t* data, size_t len) { return false; }
    virtual bool GetFeatureReport(uint8_t* buffer, size_t len) { return false; }
};
```

## Hardware Database Format

### device.json Example

```json
{
    "name": "ASUS Aura Mainboard",
    "vendor": "ASUS",
    "type": "mainboard",
    "protocol": "hid",

    "identifiers": {
        "vendorId": "0x0B05",
        "productId": "0x19AF",
        "usagePage": "0xFF72"
    },

    "capabilities": {
        "color": true,
        "brightness": true,
        "modes": ["static", "breathing", "wave", "rainbow"],
        "zones": 8,
        "addressable": true,
        "ledsPerZone": 60
    },

    "protocol": {
        "reportId": 0xB0,
        "packetSize": 65,
        "colorPacket": {
            "header": [0xB0, 0x35],
            "colorOffset": 4,
            "format": "RGB"
        },
        "modePacket": {
            "header": [0xB0, 0x35],
            "modeOffset": 3,
            "modes": {
                "static": 0,
                "breathing": 1,
                "wave": 2,
                "rainbow": 4
            }
        },
        "commitPacket": [0xB0, 0x35, 0x00]
    }
}
```

## Build Process

1. **Config Generation**: `generate_config.py` reads all `device.json` files and generates `hardware_config.h`
2. **Compilation**: Standard C++ build with generated config
3. **Optional**: User can add custom devices without recompiling (JSON-based runtime loading)

## Adding New Devices

See [ADDING_DEVICES.md](ADDING_DEVICES.md) for detailed instructions.

Quick overview:
1. Create folder in `src/plugins/<vendor>/`
2. Create `device.json` with device definition
3. Implement device class extending `HIDDevice`, `SMBusDevice`, or `USBDevice`
4. Register in `DeviceRegistry`
5. Rebuild

## Migration from OneClickRGB

The original OneClickRGB monolithic code is preserved in the `legacy/` branch. All four original devices are migrated as plugins:

- ASUS Aura → `src/plugins/asus/`
- SteelSeries Rival → `src/plugins/steelseries/`
- EVision Keyboard → `src/plugins/evision/`
- G.Skill Trident Z5 → `src/plugins/gskill/`
