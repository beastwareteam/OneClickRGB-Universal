# Neue Geräte hinzufügen

Anleitung zum Hinzufügen neuer RGB-Geräte zu OneClickRGB-Universal.

---

## Übersicht

1. Gerät zur Hardware-Datenbank hinzufügen
2. Plugin-Klasse erstellen
3. In PluginFactory registrieren
4. Testen

---

## 1. Hardware-Datenbank (`config/hardware_db.json`)

```json
{
  "devices": [
    {
      "id": "vendor_device_name",
      "name": "Vendor Device Name",
      "vendor": "Vendor",
      "type": "Keyboard|Mouse|Mainboard|RAM|LEDStrip",
      "protocol": "HID|SMBus|GPIO",
      "vid": "0x1234",
      "pid": "0x5678",
      "usagePage": "0xFF00",
      "usage": "0x01"
    }
  ]
}
```

Danach Config neu generieren:
```bash
cd tools
python generate_config.py
```

---

## 2. Plugin-Klasse erstellen

### HID-Gerät

Verzeichnis: `src/plugins/<vendor>/`

**Header (`MyDevice.h`):**

```cpp
#pragma once
#include "../../devices/HIDDevice.h"

namespace OCRGB {
namespace Plugins {

class MyDevice : public HIDDevice {
public:
    MyDevice();
    ~MyDevice() override;

    Result Initialize() override;
    Result SetColor(const RGB& color) override;
    Result SetMode(DeviceMode mode) override;
    Result Apply() override;

    DeviceInfo GetInfo() const override;
    Capabilities GetCapabilities() const override;

private:
    void BuildPacket(uint8_t* buffer, const RGB& color);
};

} // namespace Plugins
} // namespace OCRGB
```

**Implementation (`MyDevice.cpp`):**

```cpp
#include "MyDevice.h"

namespace OCRGB {
namespace Plugins {

MyDevice::MyDevice()
    : HIDDevice(0x1234, 0x5678)  // VID, PID
{
}

Result MyDevice::Initialize() {
    Result result = HIDDevice::Initialize();
    if (!result.IsSuccess()) {
        return result;
    }

    // Gerätespezifische Initialisierung
    return Result::Success();
}

Result MyDevice::SetColor(const RGB& color) {
    m_pendingColor = color;
    return Result::Success();
}

Result MyDevice::Apply() {
    uint8_t packet[64] = {0};
    BuildPacket(packet, m_pendingColor);

    return SendReport(packet, sizeof(packet));
}

void MyDevice::BuildPacket(uint8_t* buffer, const RGB& color) {
    // Gerätespezifisches Protokoll
    buffer[0] = 0xEC;           // Report ID oder Command
    buffer[1] = 0x01;           // Subcommand
    buffer[2] = color.r;
    buffer[3] = color.g;
    buffer[4] = color.b;
}

DeviceInfo MyDevice::GetInfo() const {
    DeviceInfo info;
    info.id = "vendor_device_name";
    info.name = "Vendor Device Name";
    info.vendor = "Vendor";
    info.type = "Keyboard";
    return info;
}

Capabilities MyDevice::GetCapabilities() const {
    Capabilities caps;
    caps.supportsColor = true;
    caps.supportsBrightness = true;
    caps.supportedModes = {DeviceMode::Static, DeviceMode::Breathing};
    caps.zoneCount = 1;
    return caps;
}

} // namespace Plugins
} // namespace OCRGB
```

### SMBus-Gerät

```cpp
#include "../../devices/SMBusDevice.h"

class MySMBusDevice : public SMBusDevice {
public:
    MySMBusDevice(uint8_t address)
        : SMBusDevice(address)
    {}

    Result SetColor(const RGB& color) override {
        // SMBus-Register schreiben
        WriteRegister(0x10, color.r);
        WriteRegister(0x11, color.g);
        WriteRegister(0x12, color.b);
        return Result::Success();
    }
};
```

### GPIO-Gerät (Raspberry Pi)

```cpp
#include "../../devices/GPIODevice.h"

class MyGPIODevice : public GPIODevice {
public:
    MyGPIODevice()
        : GPIODevice("my-gpio-led", "My LED Strip",
            GPIOConfig{
                GPIOLedType::WS2812B,
                ColorOrder::GRB,
                18,     // GPIO pin
                "",     // SPI (not used)
                0,
                0, 0, 0, -1,
                800,
                60,     // LED count
                255,
                10
            })
    {}
};
```

---

## 3. In PluginFactory registrieren

**`src/plugins/PluginFactory.cpp`:**

```cpp
#include "vendor/MyDevice.h"

DevicePtr PluginFactory::Create(const HardwareDB::DeviceDefinition& def, ...) {
    // ... existing code ...

    if (std::strcmp(def.id, "vendor_device_name") == 0) {
        return std::make_shared<Plugins::MyDevice>();
    }

    return nullptr;
}
```

---

## 4. CMakeLists.txt aktualisieren

```cmake
set(CORE_SOURCES
    # ... existing sources ...
    src/plugins/vendor/MyDevice.cpp
)
```

---

## 5. Testen

```bash
# Build
mkdir build && cd build
cmake ..
make

# Test
./oneclickrgb --dry-run
./oneclickrgb set 255 0 0
```

---

## Protokoll-Analyse

### USB-HID analysieren

**Windows:**
- Wireshark + USBPcap
- HIDSharp
- USBlyzer

**Linux:**
```bash
# HID Report Descriptor
sudo cat /sys/kernel/debug/hid/<device>/rdesc

# USB Traffic
sudo modprobe usbmon
sudo wireshark
```

**Wichtige Informationen:**
- Report ID
- Feature vs Output Reports
- Paketstruktur (Header, Payload)
- Timing-Anforderungen

### SMBus analysieren

```bash
# I2C-Scan
sudo i2cdetect -y 1

# Register lesen
sudo i2cdump -y 1 0x50
```

---

## Beispiele

| Gerät | Protokoll | Referenz-Code |
|-------|-----------|---------------|
| ASUS Aura | HID | `src/plugins/asus/AsusAuraController.cpp` |
| SteelSeries | HID | `src/plugins/steelseries/SteelSeriesRival.cpp` |
| G.Skill RAM | SMBus | `src/plugins/gskill/GSkillTridentZ5.cpp` |
| WS2812B | GPIO | `src/plugins/gpio/WS2812Controller.cpp` |

---

## Checkliste

- [ ] `hardware_db.json` aktualisiert
- [ ] `generate_config.py` ausgeführt
- [ ] Plugin-Header und -Implementation erstellt
- [ ] In `PluginFactory.cpp` registriert
- [ ] `CMakeLists.txt` aktualisiert
- [ ] Kompiliert ohne Fehler
- [ ] `--dry-run` Test erfolgreich
- [ ] Hardware-Test erfolgreich
