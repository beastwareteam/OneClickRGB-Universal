# OneClickRGB-Universal

**Modular, Plugin-Based RGB Controller for Windows**

A complete rewrite of OneClickRGB with a universal device abstraction layer, enabling support for any RGB hardware through plugins.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Windows](https://img.shields.io/badge/Platform-Windows%2010%2F11-blue.svg)]()

---

## Features

- **Plugin Architecture** - Add new devices without modifying core code
- **Universal Device Interface** - Common API for all RGB hardware
- **Protocol Bridges** - HID, SMBus, USB, Serial support
- **Hardware Database** - JSON-based device definitions
- **Auto-Detection** - Scan and identify connected RGB devices
- **Build-Time Config** - Generate optimized code from JSON database

---

## Supported Devices

| Device | Type | Protocol | Status |
|--------|------|----------|--------|
| ASUS Aura Mainboard | Mainboard | HID | Migrated |
| SteelSeries Rival 600 | Mouse | HID | Migrated |
| EVision Keyboard | Keyboard | HID | Migrated |
| G.Skill Trident Z5 | RAM | SMBus | Migrated |

### Adding New Devices

See [docs/ADDING_DEVICES.md](docs/ADDING_DEVICES.md) for instructions on adding support for new hardware.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     APPLICATION LAYER                        │
│              (UI, Settings, Device Manager)                  │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                        CORE LAYER                            │
│           (DeviceRegistry, Profiles, Scanner)                │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                  DEVICE ABSTRACTION LAYER                    │
│        IDevice → HIDDevice / SMBusDevice / USBDevice         │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                      PROTOCOL LAYER                          │
│         HIDBridge (hidapi) / SMBusBridge (PawnIO)           │
└─────────────────────────────────────────────────────────────┘
```

---

## Building

### Prerequisites

- Windows 10/11 (x64)
- Visual Studio 2019+ Build Tools
- Python 3.8+ (for config generation)

### Build Steps

```batch
# 1. Generate hardware config from JSON database
cd tools
python generate_config.py

# 2. Build the application
cd ../build
compile.bat
```

### Dependencies

- **hidapi** - USB HID communication
- **PawnIO** - SMBus access for RAM control (optional)

---

## Directory Structure

```
OneClickRGB-Universal/
├── src/
│   ├── app/           # Application logic
│   ├── core/          # Device registry, types
│   ├── devices/       # Device base classes
│   ├── plugins/       # Device implementations
│   │   ├── asus/
│   │   ├── steelseries/
│   │   ├── evision/
│   │   └── gskill/
│   ├── bridges/       # Protocol bridges
│   └── scanner/       # Hardware detection
├── config/
│   └── hardware_db.json
├── tools/
│   └── generate_config.py
└── build/
    └── generated/
        └── hardware_config.h
```

---

## Usage Example

```cpp
#include "core/DeviceRegistry.h"
#include "scanner/HardwareScanner.h"

int main() {
    // Scan for devices
    HardwareScanner scanner;
    auto results = scanner.QuickScan();

    // Create and register devices
    for (const auto& result : results) {
        if (result.isKnown) {
            auto device = scanner.CreateDevice(result);
            if (device && device->Initialize().IsSuccess()) {
                DeviceRegistry::Instance().RegisterDevice(device);
            }
        }
    }

    // Set color on all devices
    DeviceRegistry::Instance().SetColorAll(RGB(0, 34, 255));
    DeviceRegistry::Instance().ApplyAll();

    return 0;
}
```

---

## Relationship to OneClickRGB

This is a **fork/rewrite** of the original [OneClickRGB](https://github.com/beastwareteam/OneClickRGB) project.

| OneClickRGB | OneClickRGB-Universal |
|-------------|----------------------|
| Monolithic (~4500 LOC) | Modular plugin architecture |
| Hardcoded protocols | JSON-defined protocols |
| 4 fixed devices | Extensible device support |
| Single file | Clean separation of concerns |

The original OneClickRGB remains stable for users who only need the four supported devices. OneClickRGB-Universal is for users who want extensibility.

---

## License

MIT License - See [LICENSE](LICENSE) for details.

---

## Contributing

1. Fork the repository
2. Add your device in `src/plugins/<vendor>/`
3. Create `device.json` with protocol definition
4. Submit a pull request

See [ARCHITECTURE.md](ARCHITECTURE.md) for technical details.
