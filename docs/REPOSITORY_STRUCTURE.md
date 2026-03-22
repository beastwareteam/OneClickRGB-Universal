# Repository-Struktur (Stand: März 2026)

Vollständige Dokumentation aller Dateien und Verzeichnisse im OneClickRGB-Universal Repository.

---

## Verzeichnisübersicht

```
OneClickRGB-Universal/
├── src/                          # Quellcode
│   ├── app/                      # Application Layer
│   ├── core/                     # Core Layer
│   ├── devices/                  # Device Abstraction
│   ├── plugins/                  # Device Plugins
│   ├── bridges/                  # Protocol Bridges
│   ├── scanner/                  # Hardware Detection
│   └── platform/                 # Platform Abstraction
├── config/                       # Konfigurationsdateien
├── tests/                        # Unit Tests
├── docs/                         # Dokumentation
├── build/                        # Build-Artefakte
├── tools/                        # Build-Tools
└── dependencies/                 # Externe Abhängigkeiten
```

---

## 1. Quellcode (`src/`)

### 1.1 Einfaches Interface (Root)

| Datei | Beschreibung |
|-------|--------------|
| `OneClickRGB.h` | Einfaches API für Endbenutzer (nur diese Datei includen!) |
| `OneClickRGB.cpp` | Implementation des einfachen API |
| `main.cpp` | CLI-Einstiegspunkt mit allen Kommandos |

### 1.2 Application Layer (`src/app/`)

Orchestrierung und High-Level-Services.

#### Config (`src/app/config/`)

| Datei | Beschreibung |
|-------|--------------|
| `DeviceConfiguration.h/cpp` | Getter/Setter für Device-Profile |
| `ConfigBundleParser.h/cpp` | Parser für ConfigBundle-JSON |

#### Effects (`src/app/effects/`)

| Datei | Beschreibung |
|-------|--------------|
| `EffectFactory.h/cpp` | 2D-Effektsequenzen (Static, Breathing, Wave, etc.) |

#### Fingerprint (`src/app/fingerprint/`)

| Datei | Beschreibung |
|-------|--------------|
| `MachineFingerprint.h/cpp` | Hardware-Identifikation (Mainboard, CPU, GPU, RAM) |

#### Pipeline (`src/app/pipeline/`)

| Datei | Beschreibung |
|-------|--------------|
| `DevicePipeline.h/cpp` | Orchestrierung: Fingerprint → Apply → Verify |

#### Services (`src/app/services/`)

| Datei | Beschreibung |
|-------|--------------|
| `DeviceService.h/cpp` | Discovery, Registrierung, Orchestrierung |
| `ProvisioningService.h/cpp` | Auto-Config mit State Machine |
| `ProfileResolver.h/cpp` | Regelbasierte Profilzuordnung |

### 1.3 Core Layer (`src/core/`)

Basisdatentypen und zentrale Verwaltung.

| Datei | Beschreibung |
|-------|--------------|
| `Types.h` | RGB, DeviceMode, Capabilities, Result, ResultCode |
| `DeviceRegistry.h/cpp` | Zentrale Device-Verwaltung, Bulk-Operationen |
| `DryRunMode.h` | Testmodus ohne echte Hardware |

### 1.4 Device Abstraction (`src/devices/`)

Device-Interface und Basisklassen.

| Datei | Beschreibung |
|-------|--------------|
| `IDevice.h` | Interface-Contract für alle Devices |
| `HIDDevice.h/cpp` | Basisklasse für HID-Geräte |
| `SMBusDevice.h/cpp` | Basisklasse für SMBus-Geräte |

### 1.5 Device Plugins (`src/plugins/`)

Vendor-spezifische Implementierungen.

| Datei | Beschreibung |
|-------|--------------|
| `PluginFactory.h/cpp` | Zentrale Plugin-Registrierung und -Erzeugung |

#### ASUS (`src/plugins/asus/`)

| Datei | Beschreibung |
|-------|--------------|
| `AsusAuraController.h/cpp` | ASUS Aura Mainboard RGB Controller |

#### SteelSeries (`src/plugins/steelseries/`)

| Datei | Beschreibung |
|-------|--------------|
| `SteelSeriesRival.h/cpp` | SteelSeries Rival 600 Maus |

#### EVision (`src/plugins/evision/`)

| Datei | Beschreibung |
|-------|--------------|
| `EVisionKeyboard.h/cpp` | EVision RGB Keyboard |

#### G.Skill (`src/plugins/gskill/`)

| Datei | Beschreibung |
|-------|--------------|
| `GSkillTridentZ5.h/cpp` | G.Skill Trident Z5 DDR5 RAM |

### 1.6 Protocol Bridges (`src/bridges/`)

Protokoll-Abstraktion.

| Datei | Beschreibung |
|-------|--------------|
| `IBridge.h` | Bridge-Interface |
| `HIDBridge.h/cpp` | HID-Protokoll via HIDAPI |
| `SMBusBridge.h/cpp` | SMBus-Protokoll |

### 1.7 Hardware Scanner (`src/scanner/`)

Automatische Geräteerkennung.

| Datei | Beschreibung |
|-------|--------------|
| `HardwareScanner.h/cpp` | HID/SMBus-Scan, Matching gegen hardware_config.h |

### 1.8 Platform Abstraction (`src/platform/`)

Plattformunabhängige Abstraktion.

| Datei | Beschreibung |
|-------|--------------|
| `IPlatform.h` | Interface: ISystemInfo, IDeviceEnumerator, ISMBusProvider |
| `PlatformFactory.cpp` | Factory für plattformspezifische Implementierung |

#### Windows (`src/platform/windows/`)

| Datei | Beschreibung |
|-------|--------------|
| `WindowsPlatform.h/cpp` | Windows-Hauptklasse, COM-Init |
| `WmiSystemInfo.h/cpp` | WMI-basierte Hardware-Info |
| `WindowsDeviceEnumerator.h/cpp` | SetupAPI + HIDAPI Enumeration |
| `PawnIOSMBus.h/cpp` | PawnIO-Treiber für SMBus |

#### Linux (`src/platform/linux/`)

| Datei | Beschreibung |
|-------|--------------|
| `LinuxPlatform.h/cpp` | Linux-Hauptklasse |
| `SysfsSystemInfo.h/cpp` | sysfs/DMI Hardware-Info |
| `LinuxDeviceEnumerator.h/cpp` | sysfs USB + HIDAPI |
| `I2CDevSMBus.h/cpp` | i2c-dev Kernel-Modul SMBus |

#### macOS (`src/platform/macos/`)

| Datei | Beschreibung |
|-------|--------------|
| `MacOSPlatform.h/cpp` | macOS-Hauptklasse |
| `IOKitSystemInfo.h/cpp` | IOKit + sysctl Hardware-Info |
| `MacOSDeviceEnumerator.h/cpp` | IOKit + HIDAPI |

---

## 2. Konfiguration (`config/`)

| Datei | Beschreibung |
|-------|--------------|
| `hardware_db.json` | Gerätedatenbank (VID/PID, Protokolle, Capabilities) |
| `hardware_db.schema.json` | JSON-Schema für hardware_db.json |
| `config_bundle.schema.json` | JSON-Schema für ConfigBundle (Provisioning) |

---

## 3. Tests (`tests/`)

| Datei | Beschreibung |
|-------|--------------|
| `TestFramework.h` | Minimales Test-Framework (keine externen Abhängigkeiten) |
| `test_main.cpp` | Test-Runner, inkludiert alle Tests |
| `test_types.cpp` | Tests für RGB, DeviceMode, Capabilities |
| `test_registry.cpp` | Tests für DeviceRegistry |
| `test_effects.cpp` | Tests für EffectFactory |
| `test_config.cpp` | Tests für DeviceConfiguration |
| `test_bundle_parser.cpp` | Tests für ConfigBundleParser |
| `test_dryrun.cpp` | Tests für DryRunMode |
| `test_platform.cpp` | Tests für Platform Abstraction |

---

## 4. Dokumentation (`docs/`)

| Datei | Beschreibung |
|-------|--------------|
| `CONFIG_STRUCTURE.md` | Konfigurationsstruktur und Pipeline-Stages |
| `CROSS_PLATFORM_ARCHITECTURE.md` | Detaillierte Cross-Platform-Dokumentation |
| `LEGACY_FEATURE_EXTRACTION.md` | Feature-Mapping aus Original-OneClickRGB |
| `REPOSITORY_STRUCTURE.md` | Diese Datei |

---

## 5. Build (`build/`)

| Datei | Beschreibung |
|-------|--------------|
| `compile.bat` | Windows Build-Script (MSVC) |
| `build.bat` | Alternative Build-Variante |
| `generated/hardware_config.h` | Generierte Device-Definitionen |

---

## 6. Tools (`tools/`)

| Datei | Beschreibung |
|-------|--------------|
| `generate_config.py` | Generiert hardware_config.h aus hardware_db.json |

---

## 7. Dependencies (`dependencies/`)

| Verzeichnis | Beschreibung |
|-------------|--------------|
| `hidapi/` | HIDAPI Header für HID-Kommunikation |

---

## 8. Root-Dateien

| Datei | Beschreibung |
|-------|--------------|
| `README.md` | Projekt-Übersicht und Quick-Start |
| `ARCHITECTURE.md` | Vollständige Architektur-Dokumentation (24 Sektionen) |
| `CMakeLists.txt` | CMake Build-Konfiguration (Cross-Platform) |
| `LICENSE` | MIT-Lizenz |

---

## Schichten-Architektur

```
┌─────────────────────────────────────────────────────────────────┐
│                        Interface Layer                           │
│                   OneClickRGB.h (Simple API)                     │
│                      main.cpp (CLI)                              │
└─────────────────────────────────┬───────────────────────────────┘
                                  │
┌─────────────────────────────────▼───────────────────────────────┐
│                      Application Layer                           │
│   DeviceService | ProvisioningService | ProfileResolver          │
│   DevicePipeline | EffectFactory | ConfigBundleParser            │
│   MachineFingerprint | DeviceConfiguration                       │
└─────────────────────────────────┬───────────────────────────────┘
                                  │
┌─────────────────────────────────▼───────────────────────────────┐
│                    Platform Abstraction                          │
│                     IPlatform Interface                          │
│   ┌─────────────┬─────────────────┬─────────────────────────┐   │
│   │  Windows    │     Linux       │        macOS            │   │
│   │ WMI/PawnIO  │ sysfs/i2c-dev   │    IOKit/sysctl         │   │
│   └─────────────┴─────────────────┴─────────────────────────┘   │
└─────────────────────────────────┬───────────────────────────────┘
                                  │
┌─────────────────────────────────▼───────────────────────────────┐
│                        Core Layer                                │
│         DeviceRegistry | Types | DryRunMode                      │
└─────────────────────────────────┬───────────────────────────────┘
                                  │
┌─────────────────────────────────▼───────────────────────────────┐
│                      Device Plugins                              │
│   AsusAura | SteelSeries | EVision | GSkill | PluginFactory     │
└─────────────────────────────────┬───────────────────────────────┘
                                  │
┌─────────────────────────────────▼───────────────────────────────┐
│                    Device Abstraction                            │
│              IDevice | HIDDevice | SMBusDevice                   │
└─────────────────────────────────┬───────────────────────────────┘
                                  │
┌─────────────────────────────────▼───────────────────────────────┐
│                     Protocol Bridges                             │
│                  HIDBridge | SMBusBridge                         │
└─────────────────────────────────────────────────────────────────┘
```

---

## Abhängigkeitsregeln

**Erlaubt (von oben nach unten):**
```
Interface → Application → Platform → Core → Plugins → Devices → Bridges
```

**Verboten:**
- `core → plugins` (Core kennt keine Vendor-Details)
- `core → app` (Core ist unabhängig von Application)
- `bridges → app` (Bridges sind Low-Level)
- `plugins ↔ plugins` (Keine Querimports zwischen Vendors)

---

## Datei-Statistik

| Kategorie | Anzahl Dateien | LOC (geschätzt) |
|-----------|----------------|-----------------|
| Source (.cpp) | 40 | ~6000 |
| Header (.h) | 32 | ~2500 |
| Tests | 8 | ~800 |
| Config (JSON) | 3 | ~300 |
| Documentation | 5 | ~1500 |
| Build Scripts | 3 | ~100 |
| **Gesamt** | **91** | **~11200** |

---

## Versionierung

- Core: SemVer (`MAJOR.MINOR.PATCH`)
- Plugins: Gegen `PluginApiVersion`
- ConfigBundle: Eigene Version (`bundleVersion`)
- hardware_db.json: Schema-Version

---

*Letzte Aktualisierung: März 2026*
