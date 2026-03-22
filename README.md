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

Aktueller Architekturstand, Zielarchitektur und vollständiger Umsetzungsplan (März 2026):

- [ARCHITECTURE.md](ARCHITECTURE.md)

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
│   ├── OneClickRGB.h           # Simple API (include this!)
│   ├── OneClickRGB.cpp
│   ├── main.cpp                # CLI entry point
│   ├── app/                    # Application layer (advanced)
│   │   ├── config/
│   │   ├── effects/
│   │   ├── pipeline/
│   │   └── services/
│   ├── core/                   # Core types, DeviceRegistry
│   ├── devices/                # IDevice, HIDDevice, SMBusDevice
│   ├── plugins/                # Device implementations
│   │   ├── asus/
│   │   ├── steelseries/
│   │   ├── evision/
│   │   ├── gskill/
│   │   └── PluginFactory.cpp
│   ├── bridges/                # Protocol bridges
│   └── scanner/                # Hardware detection
├── config/
│   ├── hardware_db.json        # Device database
│   └── *.schema.json           # JSON schemas
├── docs/
└── build/
    └── generated/
```

---

## Usage

### Command Line (No Configuration Required)

```batch
# Show detected devices
oneclickrgb

# Set color (RGB values)
oneclickrgb set 255 0 128

# Set color (hex)
oneclickrgb set #FF0080

# Turn off all LEDs
oneclickrgb off

# Change mode
oneclickrgb mode rainbow

# Set brightness
oneclickrgb brightness 50
```

### C++ (Simple API)

```cpp
#include "OneClickRGB.h"

int main() {
    OneClickRGB rgb;
    rgb.Start();              // Auto-detect devices
    rgb.SetColor(0, 100, 255);// Set color
    rgb.SetModeRainbow();     // Change mode
    rgb.Stop();               // Cleanup
    return 0;
}
```

### C++ (Global Functions)

```cpp
#include "OneClickRGB.h"

int main() {
    OCRGB_Start();
    OCRGB_SetColor(255, 0, 128);
    OCRGB_SetMode("breathing");
    OCRGB_Stop();
    return 0;
}
```

### Advanced Usage (Full Control)

For advanced use cases requiring per-device control, profiles, or custom pipelines:

```cpp
#include "app/services/DeviceService.h"
#include "core/DeviceRegistry.h"

int main() {
    App::DeviceService service;
    service.DiscoverAndRegister();

    // Per-device control
    auto devices = DeviceRegistry::Instance().GetAllDevices();
    for (auto& device : devices) {
        device->SetColor(RGB(255, 0, 0));
        device->Apply();
    }

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
