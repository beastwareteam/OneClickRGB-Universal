# OneClickRGB-Universal

**Cross-Platform, Plugin-Based RGB Controller**

A complete rewrite of OneClickRGB with a universal device abstraction layer, enabling support for any RGB hardware through plugins.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Windows](https://img.shields.io/badge/Platform-Windows-blue.svg)]()
[![Linux](https://img.shields.io/badge/Platform-Linux-green.svg)]()
[![macOS](https://img.shields.io/badge/Platform-macOS-lightgrey.svg)]()

---

## Features

- **Cross-Platform** - Windows, Linux, macOS support
- **Plugin Architecture** - Add new devices without modifying core code
- **Universal Device Interface** - Common API for all RGB hardware
- **Protocol Bridges** - HID, SMBus support (platform-dependent)
- **Hardware Database** - JSON-based device definitions
- **Auto-Detection** - Scan and identify connected RGB devices
- **Auto-Provisioning** - Zero-config setup on new machines
- **Build-Time Config** - Generate optimized code from JSON database

---

## Quick Start

```bash
# Scan and show detected devices
oneclickrgb

# Set color (all devices)
oneclickrgb set 255 0 128

# Turn off
oneclickrgb off

# Auto-provision based on hardware
oneclickrgb provision --auto
```

---

## Supported Devices

| Device | Type | Protocol | Status |
|--------|------|----------|--------|
| ASUS Aura Mainboard | Mainboard | HID | Supported |
| SteelSeries Rival 600 | Mouse | HID | Supported |
| EVision Keyboard | Keyboard | HID | Supported |
| G.Skill Trident Z5 | RAM | SMBus | Supported (Win/Linux) |

---

## Platform Support

| Feature | Windows | Linux | macOS |
|---------|---------|-------|-------|
| HID Devices | Yes | Yes | Yes |
| SMBus (RAM) | Yes (PawnIO) | Yes (i2c-dev) | No |
| Auto-Provisioning | Yes | Yes | Yes (HID only) |
| Hardware Fingerprint | Yes | Yes | Yes |

---

## Architecture

```
                    +-----------------------+
                    |    Simple Interface   |
                    |  OneClickRGB (3 APIs) |
                    +-----------+-----------+
                                |
                    +-----------v-----------+
                    |   Application Layer   |
                    | DeviceService, Provis.|
                    | ProfileResolver, etc. |
                    +-----------+-----------+
                                |
                    +-----------v-----------+
                    |   Platform Abstraction|
                    |  IPlatform Interface  |
                    +---+-------+-------+---+
                        |       |       |
              +---------+   +---+---+   +---------+
              | Windows |   | Linux |   |  macOS  |
              |   WMI   |   | sysfs |   |  IOKit  |
              | PawnIO  |   |i2c-dev|   |   HID   |
              +---------+   +-------+   +---------+
                                |
                    +-----------v-----------+
                    |      Core Layer       |
                    | Registry, Types, etc. |
                    +-----------+-----------+
                                |
                    +-----------v-----------+
                    |    Device Plugins     |
                    | ASUS, SteelSeries,... |
                    +-----------+-----------+
                                |
                    +-----------v-----------+
                    |    Protocol Bridges   |
                    |   HIDBridge, SMBus    |
                    +-----------------------+
```

Full architecture documentation: [ARCHITECTURE.md](ARCHITECTURE.md)

---

## Building

### Prerequisites

**Windows:**
- Visual Studio 2019+ Build Tools
- Python 3.8+ (for config generation)

**Linux:**
```bash
sudo apt install build-essential cmake libhidapi-dev libudev-dev
sudo modprobe i2c-dev  # For SMBus support
```

**macOS:**
```bash
brew install cmake hidapi
```

### Build Steps

```bash
# 1. Generate hardware config from JSON database
cd tools
python generate_config.py

# 2. Build with CMake
mkdir build && cd build
cmake ..
cmake --build .
```

**Windows (Visual Studio):**
```batch
cd build
compile.bat
```

---

## Directory Structure

```
OneClickRGB-Universal/
├── src/
│   ├── OneClickRGB.h/cpp         # Simple API (just include this!)
│   ├── main.cpp                  # CLI entry point
│   │
│   ├── app/                      # Application Layer
│   │   ├── config/               # Configuration & Bundle Parser
│   │   ├── effects/              # Effect Factory (Static, Breathing, etc.)
│   │   ├── fingerprint/          # Machine Fingerprint
│   │   ├── pipeline/             # Device Pipeline
│   │   └── services/             # DeviceService, Provisioning, ProfileResolver
│   │
│   ├── core/                     # Core Layer
│   │   ├── Types.h               # RGB, DeviceMode, Capabilities, Result
│   │   ├── DeviceRegistry.h/cpp  # Central device management
│   │   └── DryRunMode.h          # Test mode without hardware
│   │
│   ├── devices/                  # Device Abstraction
│   │   ├── IDevice.h             # Device interface contract
│   │   ├── HIDDevice.h/cpp       # HID device base class
│   │   └── SMBusDevice.h/cpp     # SMBus device base class
│   │
│   ├── plugins/                  # Device Plugins
│   │   ├── PluginFactory.h/cpp   # Central plugin registration
│   │   ├── asus/                 # ASUS Aura Controller
│   │   ├── steelseries/          # SteelSeries Devices
│   │   ├── evision/              # EVision Keyboards
│   │   └── gskill/               # G.Skill RAM
│   │
│   ├── bridges/                  # Protocol Bridges
│   │   ├── IBridge.h             # Bridge interface
│   │   ├── HIDBridge.h/cpp       # HID protocol (HIDAPI)
│   │   └── SMBusBridge.h/cpp     # SMBus protocol
│   │
│   ├── scanner/                  # Hardware Detection
│   │   └── HardwareScanner.h/cpp
│   │
│   └── platform/                 # Platform Abstraction Layer
│       ├── IPlatform.h           # Platform interface
│       ├── PlatformFactory.cpp   # Platform factory
│       ├── windows/              # Windows implementation (WMI, PawnIO)
│       ├── linux/                # Linux implementation (sysfs, i2c-dev)
│       └── macos/                # macOS implementation (IOKit)
│
├── config/
│   ├── hardware_db.json          # Device database
│   ├── hardware_db.schema.json   # Database schema
│   └── config_bundle.schema.json # ConfigBundle schema
│
├── tests/
│   ├── TestFramework.h           # Minimal test framework
│   ├── test_main.cpp             # Test runner
│   └── test_*.cpp                # Unit tests
│
├── docs/
│   ├── CONFIG_STRUCTURE.md       # Configuration documentation
│   ├── CROSS_PLATFORM_ARCHITECTURE.md
│   └── LEGACY_FEATURE_EXTRACTION.md
│
├── build/
│   ├── generated/                # Generated headers
│   ├── compile.bat               # Windows build script
│   └── build.bat
│
├── tools/
│   └── generate_config.py        # Config generator
│
├── dependencies/
│   └── hidapi/                   # HIDAPI headers
│
├── ARCHITECTURE.md               # Full architecture documentation
├── CMakeLists.txt                # CMake build configuration
└── README.md                     # This file
```

---

## Usage

### Command Line Interface

```bash
# Basic Commands
oneclickrgb                        # Show detected devices
oneclickrgb set 255 0 128          # Set color (RGB)
oneclickrgb set #FF0080            # Set color (Hex)
oneclickrgb off                    # Turn off all LEDs
oneclickrgb mode static            # Set mode
oneclickrgb brightness 50          # Set brightness (0-100)
oneclickrgb status                 # Show device status

# Provisioning Commands
oneclickrgb provision --auto       # Auto-provision from fingerprint
oneclickrgb provision --check      # Check for drift
oneclickrgb provision --self-heal  # Auto-repair drift
oneclickrgb provision --rollback   # Rollback to previous config
oneclickrgb provision --fingerprint # Show hardware fingerprint
oneclickrgb provision --status     # Show provisioning status

# Options
--dry-run                          # Simulate without hardware
--verbose                          # Detailed output
--json                             # JSON output for scripting
```

### Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Invalid arguments |
| 2 | No devices found |
| 3 | Device communication error |
| 4 | Configuration error |
| 5 | Permission denied |

### C++ Simple API

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

### C++ Global Functions

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

### Advanced Usage

```cpp
#include "app/services/DeviceService.h"
#include "app/services/ProvisioningService.h"
#include "platform/IPlatform.h"

int main() {
    // Initialize platform
    auto platform = OCRGB::Platform::IPlatform::Create();
    platform->Initialize();

    // Auto-provision
    OCRGB::App::ProvisioningService provisioning;
    provisioning.AutoProvision();

    // Per-device control
    auto devices = OCRGB::DeviceRegistry::Instance().GetAllDevices();
    for (auto& device : devices) {
        device->SetColor(RGB(255, 0, 0));
        device->Apply();
    }

    platform->Shutdown();
    return 0;
}
```

---

## Documentation

| Document | Description |
|----------|-------------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Full architecture, roadmap, all systems |
| [docs/CONFIG_STRUCTURE.md](docs/CONFIG_STRUCTURE.md) | Configuration and pipeline documentation |
| [docs/CROSS_PLATFORM_ARCHITECTURE.md](docs/CROSS_PLATFORM_ARCHITECTURE.md) | Platform abstraction details |
| [docs/LEGACY_FEATURE_EXTRACTION.md](docs/LEGACY_FEATURE_EXTRACTION.md) | Migration from original OneClickRGB |

---

## Tests

```bash
# Build and run tests
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .
./test_runner
```

Test coverage:
- Types and data structures
- DeviceRegistry operations
- Effect generation
- Configuration parsing
- ConfigBundle validation
- Dry-run mode
- Platform abstraction

---

## Contributing

1. Fork the repository
2. Add your device in `src/plugins/<vendor>/`
3. Register in `src/plugins/PluginFactory.cpp`
4. Add device to `config/hardware_db.json`
5. Write tests in `tests/`
6. Submit a pull request

See [ARCHITECTURE.md](ARCHITECTURE.md) for technical guidelines.

---

## License

MIT License - See [LICENSE](LICENSE) for details.
