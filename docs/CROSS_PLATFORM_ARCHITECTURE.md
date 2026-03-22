# Cross-Platform Architektur

## 1) Übersicht

OneClickRGB-Universal wird auf drei Plattformen unterstützt:

| Plattform | Status | HID | SMBus | Fingerprint |
|-----------|--------|-----|-------|-------------|
| Windows | Vollständig | HIDAPI | PawnIO | WMI + SetupAPI |
| Linux | Geplant | HIDAPI | i2c-dev | sysfs + dmidecode |
| macOS | Geplant | HIDAPI | IOKit | IOKit + sysctl |

---

## 2) Architektur-Schichten

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│  (OneClickRGB, ProvisioningService, ProfileResolver)         │
│                    [Plattform-agnostisch]                    │
└──────────────────────────┬──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│                    Platform Abstraction                      │
│                       IPlatform                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │ ISystemInfo │  │ IDeviceEnum │  │  ISMBus     │          │
│  └─────────────┘  └─────────────┘  └─────────────┘          │
└──────────────────────────┬──────────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
┌───────▼───────┐  ┌───────▼───────┐  ┌───────▼───────┐
│    Windows    │  │     Linux     │  │     macOS     │
│  WMI/SetupAPI │  │ sysfs/udev    │  │ IOKit/sysctl  │
│    PawnIO     │  │   i2c-dev     │  │  IOKit SMBus  │
└───────────────┘  └───────────────┘  └───────────────┘
```

---

## 3) Interfaces

### 3.1 IPlatform (Hauptinterface)

```cpp
namespace OCRGB {
namespace Platform {

class IPlatform {
public:
    virtual ~IPlatform() = default;

    // Factory
    static std::unique_ptr<IPlatform> Create();
    static PlatformType GetCurrentPlatform();

    // Subsysteme
    virtual ISystemInfo* GetSystemInfo() = 0;
    virtual IDeviceEnumerator* GetDeviceEnumerator() = 0;
    virtual ISMBusProvider* GetSMBusProvider() = 0;

    // Capabilities
    virtual bool HasSMBusSupport() const = 0;
    virtual bool HasHIDSupport() const = 0;
    virtual bool RequiresElevation() const = 0;

    // Lifecycle
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
};

enum class PlatformType {
    Windows,
    Linux,
    macOS,
    Unknown
};

} // namespace Platform
} // namespace OCRGB
```

### 3.2 ISystemInfo (Hardware-Erkennung)

```cpp
class ISystemInfo {
public:
    virtual ~ISystemInfo() = default;

    // Mainboard
    virtual std::string GetMainboardManufacturer() = 0;
    virtual std::string GetMainboardProduct() = 0;
    virtual std::string GetMainboardSerial() = 0;

    // BIOS
    virtual std::string GetBiosVendor() = 0;
    virtual std::string GetBiosVersion() = 0;
    virtual std::string GetBiosDate() = 0;

    // CPU
    virtual std::string GetCpuName() = 0;
    virtual std::string GetCpuVendor() = 0;
    virtual int GetCpuCores() = 0;
    virtual int GetCpuThreads() = 0;

    // GPU (kann mehrere sein)
    virtual std::vector<GpuInfo> GetGpus() = 0;

    // RAM
    virtual std::vector<RamInfo> GetRamModules() = 0;
    virtual uint64_t GetTotalRam() = 0;

    // OS
    virtual std::string GetOsName() = 0;
    virtual std::string GetOsVersion() = 0;
    virtual std::string GetKernelVersion() = 0;
};
```

### 3.3 IDeviceEnumerator (USB/HID-Geräte)

```cpp
class IDeviceEnumerator {
public:
    virtual ~IDeviceEnumerator() = default;

    // USB-Geräte
    virtual std::vector<UsbDeviceInfo> EnumerateUSB() = 0;
    virtual std::vector<UsbDeviceInfo> EnumerateUSB(uint16_t vendorId) = 0;

    // HID-Geräte (über HIDAPI - plattformübergreifend)
    virtual std::vector<HidDeviceInfo> EnumerateHID() = 0;
    virtual std::vector<HidDeviceInfo> EnumerateHID(uint16_t vendorId, uint16_t productId) = 0;

    // Gerät öffnen
    virtual std::unique_ptr<IHidDevice> OpenHID(const std::string& path) = 0;
    virtual std::unique_ptr<IHidDevice> OpenHID(uint16_t vendorId, uint16_t productId) = 0;

    // Hotplug-Events (optional)
    using DeviceCallback = std::function<void(const UsbDeviceInfo&, bool connected)>;
    virtual void RegisterHotplugCallback(DeviceCallback callback) = 0;
    virtual void UnregisterHotplugCallback() = 0;
};
```

### 3.4 ISMBusProvider (System Management Bus)

```cpp
class ISMBusProvider {
public:
    virtual ~ISMBusProvider() = default;

    // Verfügbarkeit
    virtual bool IsAvailable() const = 0;
    virtual bool RequiresElevation() const = 0;

    // Lifecycle
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;

    // Bus-Operationen
    virtual bool WriteByte(uint8_t addr, uint8_t reg, uint8_t value) = 0;
    virtual bool WriteWord(uint8_t addr, uint8_t reg, uint16_t value) = 0;
    virtual bool WriteBlock(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) = 0;

    virtual bool ReadByte(uint8_t addr, uint8_t reg, uint8_t& value) = 0;
    virtual bool ReadWord(uint8_t addr, uint8_t reg, uint16_t& value) = 0;
    virtual bool ReadBlock(uint8_t addr, uint8_t reg, uint8_t* buffer, size_t len) = 0;

    // Scan
    virtual std::vector<uint8_t> ScanBus() = 0;
    virtual bool ProbeAddress(uint8_t addr) = 0;

    // Fehlerbehandlung
    virtual std::string GetLastError() const = 0;
};
```

---

## 4) Plattform-Implementierungen

### 4.1 Windows

**Dateistruktur:**
```
src/platform/windows/
├── WindowsPlatform.h
├── WindowsPlatform.cpp
├── WmiSystemInfo.h
├── WmiSystemInfo.cpp
├── SetupApiEnumerator.h
├── SetupApiEnumerator.cpp
├── PawnIOSMBus.h
└── PawnIOSMBus.cpp
```

**Abhängigkeiten:**
- `Windows.h`
- `Wbemidl.h` (WMI)
- `SetupAPI.h`
- `PawnIOLib.dll` (dynamisch)
- `hid.lib`, `setupapi.lib`

**SystemInfo via WMI:**
```cpp
// Win32_BaseBoard → Mainboard
// Win32_BIOS → BIOS
// Win32_Processor → CPU
// Win32_VideoController → GPU
// Win32_PhysicalMemory → RAM
```

**SMBus via PawnIO:**
- Kernel-Treiber `SmbusI801.bin`
- Unterstützt Intel ICH/PCH SMBus Controller
- Erfordert Admin-Rechte

### 4.2 Linux

**Dateistruktur:**
```
src/platform/linux/
├── LinuxPlatform.h
├── LinuxPlatform.cpp
├── SysfsSystemInfo.h
├── SysfsSystemInfo.cpp
├── UdevEnumerator.h
├── UdevEnumerator.cpp
├── I2CDevSMBus.h
└── I2CDevSMBus.cpp
```

**Abhängigkeiten:**
- `libudev` (optional, für Hotplug)
- `i2c-dev` Kernel-Modul
- `libhidapi-hidraw` oder `libhidapi-libusb`

**SystemInfo via sysfs/DMI:**
```bash
# Mainboard
/sys/class/dmi/id/board_vendor
/sys/class/dmi/id/board_name
/sys/class/dmi/id/board_serial

# BIOS
/sys/class/dmi/id/bios_vendor
/sys/class/dmi/id/bios_version
/sys/class/dmi/id/bios_date

# CPU
/proc/cpuinfo
/sys/devices/system/cpu/

# RAM
/proc/meminfo
dmidecode --type 17  # Erfordert Root
```

**SMBus via i2c-dev:**
```cpp
// /dev/i2c-* Devices
int fd = open("/dev/i2c-0", O_RDWR);
ioctl(fd, I2C_SLAVE, address);
i2c_smbus_write_byte_data(fd, reg, value);
i2c_smbus_read_byte_data(fd, reg);
```

**Voraussetzungen:**
```bash
# Kernel-Module laden
sudo modprobe i2c-dev
sudo modprobe i2c-i801  # Intel SMBus

# Berechtigungen
sudo usermod -aG i2c $USER
# oder udev-Regel für /dev/i2c-*
```

### 4.3 macOS

**Dateistruktur:**
```
src/platform/macos/
├── MacOSPlatform.h
├── MacOSPlatform.cpp
├── IOKitSystemInfo.h
├── IOKitSystemInfo.cpp
├── IOKitEnumerator.h
├── IOKitEnumerator.cpp
├── IOKitSMBus.h
└── IOKitSMBus.cpp
```

**Abhängigkeiten:**
- `IOKit.framework`
- `CoreFoundation.framework`
- `libhidapi` (via Homebrew)

**SystemInfo via IOKit/sysctl:**
```cpp
// sysctl für CPU/RAM
sysctlbyname("machdep.cpu.brand_string", ...);
sysctlbyname("hw.memsize", ...);

// IOKit für Hardware-Tree
IORegistryEntryCreateCFProperties(...)
// IOPlatformExpertDevice → Mainboard
// AppleSMC → Sensoren
```

**SMBus:**
- Eingeschränkter Zugriff auf macOS
- Kein direkter i2c-dev Zugang
- Möglicher Workaround: AppleSMC für einige Funktionen
- Realistische Einschränkung: SMBus-RAM-RGB nicht unterstützt

---

## 5) HIDAPI (Plattformübergreifend)

HIDAPI ist bereits plattformübergreifend und wird auf allen Plattformen verwendet:

| Plattform | Backend |
|-----------|---------|
| Windows | `hid.dll` (Windows HID API) |
| Linux | `hidraw` oder `libusb` |
| macOS | `IOKit` |

**Bestehende Verwendung:**
- `HIDBridge` verwendet HIDAPI
- Alle HID-Geräte (ASUS Aura, SteelSeries, EVision) funktionieren bereits

**Keine Änderungen erforderlich** außer Build-Konfiguration.

---

## 6) Build-System (CMake)

### 6.1 Plattform-Erkennung

```cmake
# Plattform-Detection
if(WIN32)
    set(PLATFORM_NAME "windows")
    set(PLATFORM_SOURCES
        src/platform/windows/WindowsPlatform.cpp
        src/platform/windows/WmiSystemInfo.cpp
        src/platform/windows/SetupApiEnumerator.cpp
        src/platform/windows/PawnIOSMBus.cpp
    )
    set(PLATFORM_LIBS setupapi hid wbemuuid ole32 oleaut32)

elseif(APPLE)
    set(PLATFORM_NAME "macos")
    set(PLATFORM_SOURCES
        src/platform/macos/MacOSPlatform.cpp
        src/platform/macos/IOKitSystemInfo.cpp
        src/platform/macos/IOKitEnumerator.cpp
        src/platform/macos/IOKitSMBus.cpp
    )
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    set(PLATFORM_LIBS ${IOKIT_FRAMEWORK} ${COREFOUNDATION_FRAMEWORK})

elseif(UNIX)
    set(PLATFORM_NAME "linux")
    set(PLATFORM_SOURCES
        src/platform/linux/LinuxPlatform.cpp
        src/platform/linux/SysfsSystemInfo.cpp
        src/platform/linux/UdevEnumerator.cpp
        src/platform/linux/I2CDevSMBus.cpp
    )
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(UDEV libudev)
    set(PLATFORM_LIBS ${UDEV_LIBRARIES} i2c)
endif()
```

### 6.2 HIDAPI-Integration

```cmake
# HIDAPI finden oder einbetten
find_package(hidapi QUIET)
if(NOT hidapi_FOUND)
    # Verwende eingebettete Version
    add_subdirectory(dependencies/hidapi)
    set(HIDAPI_LIBRARIES hidapi)
else()
    set(HIDAPI_LIBRARIES hidapi::hidapi)
endif()
```

### 6.3 Kompilierungsflags

```cmake
# Plattform-spezifische Definitionen
target_compile_definitions(ocrgb_core PUBLIC
    OCRGB_PLATFORM_${PLATFORM_NAME}
    $<$<BOOL:${WIN32}>:OCRGB_PLATFORM_WINDOWS>
    $<$<BOOL:${APPLE}>:OCRGB_PLATFORM_MACOS>
    $<$<BOOL:${UNIX}>:OCRGB_PLATFORM_LINUX>
)
```

---

## 7) Migrationsplan

### Phase 1: Abstraktion erstellen
1. `src/platform/` Verzeichnisstruktur anlegen
2. `IPlatform`, `ISystemInfo`, `IDeviceEnumerator`, `ISMBusProvider` Interfaces
3. Windows-Implementierung aus bestehendem Code extrahieren

### Phase 2: Code refactoren
1. `MachineFingerprint` auf `ISystemInfo` umstellen
2. `SMBusBridge` auf `ISMBusProvider` umstellen
3. `HardwareScanner` auf `IDeviceEnumerator` umstellen

### Phase 3: Linux-Support
1. Linux Platform-Implementierung
2. sysfs/DMI für SystemInfo
3. i2c-dev für SMBus
4. HIDAPI-Integration (meist vorhanden)

### Phase 4: macOS-Support
1. macOS Platform-Implementierung
2. IOKit für SystemInfo
3. SMBus: eingeschränkt oder deaktiviert

### Phase 5: Testing & CI
1. CI/CD für alle Plattformen (GitHub Actions)
2. Plattform-spezifische Tests
3. Cross-Compilation (optional)

---

## 8) Einschränkungen

### Windows
- PawnIO erfordert Admin-Rechte für SMBus
- Treiber-Signierung erforderlich

### Linux
- i2c-dev erfordert Root oder i2c-Gruppe
- Kernel-Module müssen geladen sein
- Einige Mainboards haben keinen zugänglichen SMBus

### macOS
- Kein direkter SMBus-Zugang
- RAM-RGB (G.Skill etc.) **nicht unterstützt**
- Nur HID-Geräte funktionieren

---

## 9) Verzeichnisstruktur (Ziel)

```
src/
├── platform/
│   ├── IPlatform.h
│   ├── ISystemInfo.h
│   ├── IDeviceEnumerator.h
│   ├── ISMBusProvider.h
│   ├── PlatformFactory.cpp
│   │
│   ├── windows/
│   │   ├── WindowsPlatform.h
│   │   ├── WindowsPlatform.cpp
│   │   ├── WmiSystemInfo.cpp
│   │   ├── SetupApiEnumerator.cpp
│   │   └── PawnIOSMBus.cpp
│   │
│   ├── linux/
│   │   ├── LinuxPlatform.h
│   │   ├── LinuxPlatform.cpp
│   │   ├── SysfsSystemInfo.cpp
│   │   ├── UdevEnumerator.cpp
│   │   └── I2CDevSMBus.cpp
│   │
│   └── macos/
│       ├── MacOSPlatform.h
│       ├── MacOSPlatform.cpp
│       ├── IOKitSystemInfo.cpp
│       ├── IOKitEnumerator.cpp
│       └── IOKitSMBus.cpp  (stub/disabled)
│
├── bridges/
│   ├── IBridge.h          (unverändert)
│   ├── HIDBridge.cpp      (nutzt HIDAPI - plattformübergreifend)
│   └── SMBusBridge.cpp    (nutzt ISMBusProvider)
│
└── ...
```

---

## 10) Zusammenfassung

| Komponente | Änderungsaufwand |
|------------|------------------|
| HIDBridge | Minimal (HIDAPI ist cross-platform) |
| SMBusBridge | Hoch (neues Interface + 3 Implementierungen) |
| MachineFingerprint | Hoch (neues Interface + 3 Implementierungen) |
| HardwareScanner | Mittel (Refactoring auf Interfaces) |
| Plugins (HID) | Keine |
| Plugins (SMBus) | Keine (nutzen abstrakten SMBusBridge) |
| Build-System | Hoch (CMake Erweiterung) |

**Geschätzter Aufwand:**
- Phase 1 (Abstraktion): 1-2 Tage
- Phase 2 (Refactoring): 1 Tag
- Phase 3 (Linux): 2-3 Tage
- Phase 4 (macOS): 1-2 Tage
- Phase 5 (Testing): 1-2 Tage

**Gesamt: ~7-10 Tage**
