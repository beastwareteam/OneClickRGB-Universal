# OneClickRGB-Universal Architektur (Stand: März 2026)

## 1) Zielbild

OneClickRGB-Universal wird als **modulare RGB-Plattform** weiterentwickelt:

- **Zero-Config für Endbenutzer** — funktioniert out-of-the-box ohne Konfiguration
- **stabiler Core** für Geräte-Lifecycle, Registry, Profile, Scheduling
- **austauschbare Protokoll-Bridges** (HID, SMBus, später USB/I2C/Serial/Network)
- **plugin-basierte Gerätecontroller** für Vendor-/Geräte-spezifische Implementierung
- **mehrere Interfaces** (CLI zuerst, danach GUI und optional API)
- **datengetriebene Erkennung** über `hardware_db.json` + generierte Konfiguration

### Design-Prinzip: Einfachheit zuerst

```
Benutzer-Sicht:          Entwickler-Sicht:
┌─────────────┐          ┌─────────────────────────────────────┐
│ OneClickRGB │ ───────► │ DeviceService → Pipeline → Registry │
│ (3 Methoden)│          │ Scanner → PluginFactory → Bridges   │
└─────────────┘          └─────────────────────────────────────┘
```

Die meisten Benutzer brauchen nur:
- `Start()` — Geräte automatisch erkennen
- `SetColor()` — Farbe setzen
- `Stop()` — Aufräumen

Komplexität (Profile, Provisioning, per-Device-Control) ist optional und versteckt.

---

## 2) Ist-Zustand (realer Code im Repository)

*Letzte Verifizierung: 2026-03-22*

### Vorhanden und funktionsfähig

- `IDevice`, `HIDDevice`, `SMBusDevice` mit `Result`-basiertem API
- `IBridge`, `HIDBridge`, `SMBusBridge`
- `DeviceRegistry` mit Bulk-Operationen (`SetColorAll`, `ApplyAll`, ...)
- `HardwareScanner` für HID/SMBus und Matching gegen `build/generated/hardware_config.h`
- `HardwareScanner::CreateDevice()` implementiert via `PluginFactory::Create()`
- `PluginFactory` als zentrale Mapping-Stelle für Device-Erzeugung
- Plugin-Verzeichnisse: `asus`, `steelseries`, `evision`, `gskill`
- Build-Konfigurationspfad über `tools/generate_config.py` und `build/generated/`
- `src/app/` mit Application-Layer-Komponenten:
  - `pipeline/DevicePipeline` — Orchestrierung von Fingerprint → Apply → Verify
  - `config/DeviceConfiguration` — Getter/Setup für Profile
  - `effects/EffectFactory` — 2D-Effektsequenzen (Frame × Zone)
- `docs/` mit Basisdokumenten:
  - `LEGACY_FEATURE_EXTRACTION.md` — Feature-Mapping aus Vorgänger
  - `CONFIG_STRUCTURE.md` — Konfigurationsstruktur und Pipeline-Stages

### Noch offen / unvollständig

- `src/ui/` ist derzeit leer (GUI Phase 2)
- `tests/` ist derzeit leer (Testpyramide Phase 4)
- Keine klar getrennte Interface-Schicht (`IInterface`) im Code
- Kein Runtime-Plugin-Loader (derzeit statisch/kompilierzeitnah)
- `ProvisioningService`, `MachineFingerprint`, `ProfileResolver` nicht implementiert
- ConfigBundle-Parser fehlt (Schema vorhanden, kein Loader)
- CLI nicht implementiert

Diese Lücken sind die primären Architektur- und Delivery-Risiken für Phase 1/1.5.

---

## 3) Zielarchitektur 2026 (Referenz)

```text
┌──────────────────────────────────────────────────────────────────────┐
│                         Interface Layer                              │
│   CLI (Phase 1)   GUI (Phase 2)   API/WebSocket (Phase 3, optional) │
└───────────────────────────────┬──────────────────────────────────────┘
                                │
┌───────────────────────────────▼──────────────────────────────────────┐
│                       Application Layer                              │
│ Command Orchestrator | Profile Service | Effect Scheduler | Events   │
└───────────────────────────────┬──────────────────────────────────────┘
                                │
┌───────────────────────────────▼──────────────────────────────────────┐
│                            Core Layer                                │
│ DeviceRegistry | DeviceLifecycle | Capability Resolver | Error Model │
└───────────────────────────────┬──────────────────────────────────────┘
                                │
┌───────────────────────────────▼──────────────────────────────────────┐
│                      Device Plugin Layer                             │
│ AsusAura | SteelSeries | EVision | GSkill | Future Vendors          │
│ (Implementieren IDevice + optional vendor-spezifische Services)      │
└───────────────────────────────┬──────────────────────────────────────┘
                                │
┌───────────────────────────────▼──────────────────────────────────────┐
│                     Bridge / Protocol Layer                          │
│ HIDBridge | SMBusBridge | (optional USB/I2C/Serial/Network)         │
└───────────────────────────────┬──────────────────────────────────────┘
                                │
┌───────────────────────────────▼──────────────────────────────────────┐
│               Detection & Data Layer                                 │
│ hardware_db.json | protocol templates | generated hardware_config.h   │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 4) Architekturprinzipien

1. **Core bleibt vendor-agnostisch**
   - Keine gerätespezifische Protokolllogik in `core/`.
2. **Capabilities vor Special-Casing**
   - Verhalten über `Capabilities` steuern, nicht über harte Geräte-`if`-Ketten.
3. **Deterministischer Geräte-Lifecycle**
   - `Discovered -> Initialized -> Ready -> Degraded/Disconnected`.
4. **Einheitliches Fehlerbild**
   - Alle Ebenen arbeiten mit `Result` + standardisierten Fehlercodes.
5. **Interface-agnostische Use-Cases**
   - CLI/GUI/API nutzen denselben Application-Layer.
6. **Sichere Fallbacks**
   - Bei Teilfehlern keine globale Blockade (`best effort` für Bulk-Operationen).

---

## 5) Optionen für den Ausbau (Best/Most Options, technisch konkret)

## Option A: MVP-Plus (schnellster Wert)

- Fokus: robuste CLI + stabile Geräteansteuerung
- Kein Runtime-Plugin-Loader, Plugins statisch registriert
- GUI/API zurückgestellt
- Konfigurationsverteilung über signierte JSON-Profile + CLI-Import

**Wann wählen:** kleines Team, schneller Release-Zyklus, geringe Wartungslast.

## Option B: Balanced (empfohlen)

- CLI + grundlegende GUI
- Plugin-Factory mit sauberer Registrierungs-API
- Event-Bus intern, Profile + Presets produktionsreif
- Testpyramide: Unit + Integration für Scanner/Registry/Bridges
- **Auto-Provisioning Agent** (Windows Service) für vollautomatische Konfiguration nach Erstinstallation

**Wann wählen:** bestes Verhältnis aus Time-to-Market, Wartbarkeit und Erweiterbarkeit.

## Option C: Enterprise-Ready

- Alles aus Option B plus:
- Runtime Plugin Loading (DLL-Schnittstellen + Versionsprüfung)
- Lokale API (REST/WebSocket) inkl. AuthN/AuthZ
- Telemetrie, Healthchecks, erweiterte Observability
- Striktes Rechte-/Sicherheitsmodell (Admin/Non-Admin Pfade)
- Flottenbetrieb mit zentralem Profil-Repository und Rollout-Ringen

**Wann wählen:** Multi-Client-Ökosystem, Integrationen, große Community/Partner.

---

## 6) Konkreter Umsetzungsplan (Roadmap)

## Phase 0 – Stabilisieren (1–2 Wochen)

**Ziele**
- Scanner-zu-Device-Weg schließen
- reproduzierbarer Build
- Architektur-Baseline dokumentiert

**Tasks**
- `HardwareScanner::CreateDevice(...)` implementieren
- Plugin-Factory als zentrale Mapping-Stelle aufbauen
- Fehlermeldungen/`Result` in Scanner/Registry vereinheitlichen
- `docs/` mit Basisdokumenten initialisieren

**Abnahmekriterien**
- `DiscoverDevices()` registriert mindestens bekannte Gerätepfade zuverlässig
- Einfache End-to-End-Demo: Scan -> Init -> SetColor -> Apply

## Phase 1 – Application Layer + CLI First (2–4 Wochen)

**Ziele**
- klare Use-Cases statt direkter Core-Aufrufe
- produktionsfähiges CLI

**Tasks**
- `src/app/` aufbauen (`DeviceService`, `ProfileService`, `CommandOrchestrator`)
- CLI-Kommandos: `scan`, `list`, `set-color`, `set-mode`, `brightness`, `apply`, `profile`
- Exit-Codes und machine-readable Output (JSON optional)

**Abnahmekriterien**
- Alle Kernfunktionen skriptbar
- Keine UI-Abhängigkeit für Standardbetrieb

## Phase 1.5 – Vollautomatische Erstkonfiguration (2–3 Wochen)

**Ziele**
- neue Maschinen nach Erstinstallation ohne manuelle Eingriffe konfigurieren
- zwei Betriebsarten bereitstellen: hardware-nah und autonomes Modul

**Tasks**
- `ProvisioningService` als Windows Service (Autostart) definieren
- `MachineFingerprint` implementieren (Mainboard/BIOS/CPU/GPU/RAM/HID über WMI + SetupAPI)
- `ProfileResolver` bauen (Regelwerk: Gerätetyp, Vendor, Modell, Standort, Tag)
- `ConfigBundle`-Format einführen (`manifest.json`, Profile, Policies, Signatur)
- Fallback-Mechanik: `HardwarePath` zuerst, sonst `AutonomousModule`
- Drift-Erkennung + Self-Heal (`hash`-basiert, periodischer Reconcile)

**Abnahmekriterien**
- Erststart auf neuer Maschine wählt automatisch ein gültiges Profil
- bei Gerätetausch erfolgt automatische Re-Match-Konfiguration ohne Nutzeraktion
- bei Fehlern greift ein sicherer Fallback auf Basiskonfiguration

## Phase 2 – GUI & UX Layer (3–6 Wochen)

**Ziele**
- nutzbare Oberfläche mit identischer Funktionalität zur CLI

**Tasks**
- `src/ui/` mit modularen Panels (Devices, Colors, Modes, Profiles)
- UI kommuniziert ausschließlich mit Application-Layer
- Persistenz für zuletzt genutzte Profile/Settings

**Abnahmekriterien**
- GUI und CLI liefern konsistente Ergebnisse
- Geräte-/Profilzustände bleiben über Restart stabil

## Phase 3 – Erweiterte Protokolle & API (optional, 4–8 Wochen)

**Ziele**
- externe Integrationen und größere Hardwareabdeckung

**Tasks**
- optionale Bridges: USB Bulk, I2C, Serial, Network
- lokale API (REST/WebSocket) auf Basis bestehender Use-Cases
- Sicherheits- und Rechtekonzept (insb. SMBus/Low-level Zugriff)
- optionaler zentraler Config-Endpoint für autonome Re-Provisionierung

**Abnahmekriterien**
- API kann bestehende CLI/GUI-Funktionen 1:1 auslösen
- klar dokumentierte Sicherheitsgrenzen

## Phase 4 – Qualität & Skalierung (laufend)

**Ziele**
- Regressionen minimieren, Beitrag neuer Vendoren beschleunigen

**Tasks**
- Unit-Tests für Registry, Matching, Result-Pfade
- Integrations-Tests für HID/SMBus mit Mock/Fake-Bridges
- Plugin-Onboarding-Template + Contribution-Guide

**Abnahmekriterien**
- stabile Testbasis für Releases
- neue Geräteplugins können ohne Core-Änderungen integriert werden

---

## 7) Empfohlene Zielentscheidung

Für dieses Repository wird **Option B (Balanced)** empfohlen, erweitert um **Phase 1.5 Auto-Provisioning**.

Warum:
- passt zum aktuellen technischen Reifegrad
- liefert kurzfristig nutzbare Ergebnisse (CLI + erste GUI)
- hält den Weg zu API/Runtime-Plugins offen, ohne sofortige Überkomplexität
- erfüllt dein Ziel einer vollautomatischen Konfiguration auf neuen Maschinen

---

## 8) Technische Leitplanken für Implementierung

- `core/` darf keine Vendor-spezifischen Includes bekommen
- Plugin-Registrierung nur an einer Stelle (Factory/Registry Adapter)
- jede Bridge kapselt ihre Fehler intern und liefert konsistente `Result`-Meldungen
- Scanner-Matching strikt datengetrieben (`hardware_db.json` + Generated Config)
- keine neue Feature-Implementierung ohne mindestens einen Testfall im betroffenen Bereich

---

## 9) Nächste 5 konkrete Arbeitspakete

*Stand: 2026-03-22*

### Erledigt (Phase 0 + Phase 1 Basis)

- [x] `CreateDevice()` + statische Plugin-Factory implementieren
- [x] `src/app/` mit `DevicePipeline` und `DeviceConfiguration` anlegen
- [x] `EffectFactory` für 2D-Effektsequenzen implementieren
- [x] `docs/` mit Basisdokumenten initialisieren
- [x] `DeviceService` als Orchestrierungs-Layer
- [x] Modulgrenzen-Verletzung behoben (`DeviceRegistry` entkoppelt)
- [x] Einfaches Interface (`OneClickRGB.h`) — Zero-Config für Endbenutzer
- [x] CLI implementiert: `set`, `off`, `mode`, `brightness`
- [x] Dry-Run-Modus für Tests ohne Hardware (DryRunMode, Bridge-Integration, CLI-Flags)

### Phase 1 - Abgeschlossen

- [x] ConfigBundle-Parser für `config_bundle.schema.json` implementieren
- [x] Unit-Tests für Types, Registry, Effects, Config, BundleParser, DryRun erstellen
- [x] Build-System (CMake/compile.bat) aktualisieren für neue Dateien
- [x] Exit-Codes und JSON-Output für CLI (Skript-Integration)

### Phase 1.5 - Abgeschlossen

- [x] `MachineFingerprint` — Hardware-Erkennung via WMI/SetupAPI
- [x] `ProfileResolver` — Regelwerk für automatische Profilzuordnung
- [x] `ProvisioningService` — Auto-Config-Kern mit State Machine
- [x] CLI-Kommandos `provision --auto`, `--check`, `--self-heal`, `--rollback`, `--fingerprint`, `--status`
- [x] Drift-Erkennung und Rollback-Mechanik
- [x] Persistenter Machine-State mit JSON-Serialisierung

### Nächste Priorität (Phase 2 - GUI)

5. `src/ui/` mit modularen Panels (Devices, Colors, Modes, Profiles)
6. UI kommuniziert ausschließlich mit Application-Layer
7. Persistenz für zuletzt genutzte Profile/Settings

---

## 10) Vollautomatische Konfiguration auf neuen Maschinen (Design)

### Betriebsart A – HardwarePath (hardware-nah)

1. Installer legt Service + Basisprofil ab.
2. Service sammelt Hardware-Fingerprint (WMI, SetupAPI, HID-Enumeration, SMBus-Scan).
3. `ProfileResolver` matched gegen Regeln in `hardware_db.json` + lokalen Policies.
4. Konfiguration wird angewendet (`scan -> register -> set -> apply`).
5. Ergebnis wird als Machine-State persistiert (inkl. Hash für Drift-Detection).

**Vorteile**
- maximale Genauigkeit bei direkter Hardware-Erkennung
- keine externe Infrastruktur zwingend nötig

**Risiken**
- mehr Rechtebedarf auf manchen Bussen (SMBus)

### Betriebsart B – AutonomousModule (vollautonom)

1. Lokales Modul arbeitet eventgesteuert (Device arrival/removal, timerbasiert, startup).
2. Modul lädt signierte `ConfigBundle`-Versionen aus lokalem Cache oder optionalem Endpoint.
3. Rules Engine evaluiert Gerätetopologie + Policies und reconciled Soll/Ist.
4. Bei Abweichungen: automatische Korrektur (Self-Heal), danach Health-Check.
5. Bei Fehlern: atomarer Rollback auf letzte stabile Bundle-Version.

**Vorteile**
- vollständig autonom nach Erstinstallation
- ideal für wiederholbare Rollouts auf vielen Maschinen

**Risiken**
- höherer initialer Entwicklungsaufwand

### Minimaler Technologie-Stack (bereits gut verfügbar)

- Discovery: vorhandene `HardwareScanner`-Pfade + Windows WMI + SetupAPI
- Orchestrierung: Windows Service + Scheduled Task (Fallback)
- Datenformat: signiertes JSON `ConfigBundle`
- Integrität: SHA-256 Hash + Signaturprüfung
- Logging: rotierende lokale Logs + optional EventLog

### Sicherheitsregeln (Pflicht)

- Provisioning läuft standardmäßig im Least-Privilege-Modus
- SMBus/Low-Level-Zugriffe nur in explizitem Elevated-Pfad
- Signaturprüfung vor jeder Bundle-Anwendung
- niemals direkte Raw-Packets aus untrusted Quellen anwenden

---

## 11) Anti-Sackgasse: harte Go/No-Go-Kriterien

Ein Feature wird nur integriert, wenn alle Punkte erfüllt sind:

1. **Austauschbarkeit**: mindestens eine klare Interface-Grenze (Core kennt keine Vendor-Details).
2. **Rückbau**: Feature kann ohne Datenverlust deaktiviert oder entfernt werden.
3. **Fallback**: bei Fehlern existiert ein deterministischer Safe-Mode.
4. **Testbarkeit**: Unit-Test + mindestens ein Integrationsszenario möglich.
5. **Versionierbarkeit**: Config/Plugin/API besitzen explizite Version.

Wenn ein Punkt fehlt: **kein Merge**, stattdessen ADR mit Alternativpfad.

---

## 12) Modulgrenzen und erlaubte Abhängigkeiten

**Erlaubte Richtung (einseitig):**

`ui -> app -> core -> devices/plugins -> bridges -> os/windows`

**Verboten:**

- `core -> plugins`
- `core -> ui`
- `bridges -> app`
- Querimports zwischen Vendor-Plugins

**Regel:** Nur `app` orchestriert Use-Cases; `ui` und `api` dürfen keine Bridge-Calls direkt ausführen.

---

## 13) Vertragsdefinitionen (Contracts)

### 13.1 Device Contract v1

Pflichtmethoden für produktionsreife Plugins:

- `Initialize`, `Shutdown`, `IsConnected`, `IsReady`
- `SetColor`, `SetMode`, `SetBrightness`, `Apply`
- `GetInfo`, `GetCapabilities`, `GetState`

Pflichtverhalten:

- `Initialize` ist **idempotent**.
- `Shutdown` darf nie crashen, auch bei Teilinitialisierung.
- `Apply` darf nur gemeldete Capability-Pfade nutzen.

### 13.2 Result/Error Contract

- Jeder Fehlerpfad liefert `ResultCode` + technische Kurzmeldung.
- Keine stillen Fehler (`bool false` ohne Kontext ist unzulässig).

**Definierte ResultCodes** (siehe `src/core/Types.h`):

| Code | Wert | Bedeutung | Reaktion |
|------|------|-----------|----------|
| `Success` | 0 | Operation erfolgreich | — |
| `Error` | 1 | Allgemeiner Fehler | Log + User-Feedback |
| `NotConnected` | 2 | Gerät nicht verbunden | Reconnect-Flow |
| `NotSupported` | 3 | Operation nicht unterstützt | Capability-Fallback |
| `InvalidParameter` | 4 | Ungültiger Parameter | Validierung prüfen |
| `Timeout` | 5 | Zeitüberschreitung | Retry mit Backoff |
| `PermissionDenied` | 6 | Keine Berechtigung | Elevated-Pfad oder Abbruch |
| `DeviceBusy` | 7 | Gerät beschäftigt | Warten + Retry |
| `CommunicationError` | 8 | Kommunikationsfehler | Retry mit Backoff |

**Mapping auf Betriebsaktion:**
- `NotConnected` → Reconnect-Flow
- `NotSupported` → Capability-Fallback
- `CommunicationError` / `Timeout` → Retry mit exponentiellem Backoff
- `PermissionDenied` → Elevated-Pfad anfordern oder User informieren
- `DeviceBusy` → kurze Wartezeit, dann Retry

### 13.3 Provisioning Contract

- Eingang: `MachineFingerprint + ConfigBundle + PolicySet`
- Ausgang: `AppliedProfile + DriftState + HealthStatus`
- Lauf ist atomar: Erfolg oder Rollback auf vorherigen stabilen Zustand.

---

## 14) Plugin-System: statisch jetzt, dynamisch vorbereiten

### Phase-jetzt (statisch)

- `PluginFactory` mappt `deviceDefinition.id -> CreatorFn`.
- Registrierung zentral in genau einer Datei (`src/plugins/PluginFactory.cpp`).
- `CreateDevice()` ruft nur Factory auf, keine `if/else`-Ketten über Vendors.

### Phase-später (runtime)

- ABI-Grenze über `PluginApiVersion`.
- Handshake beim Laden: `name`, `vendor`, `apiVersion`, `minCoreVersion`, `capabilities`.
- Bei Version-Mismatch: Plugin isoliert deaktivieren, Core läuft weiter.

---

## 14b) Capabilities-System (verbindlich)

### Capabilities-Struct

Jedes Gerät deklariert seine Fähigkeiten über `Capabilities` (siehe `src/core/Types.h`):

```cpp
struct Capabilities {
    bool supportsColor = true;          // RGB-Farbsteuerung
    bool supportsBrightness = true;     // Helligkeitsregelung
    bool supportsSpeed = true;          // Animationsgeschwindigkeit
    bool supportsDirection = false;     // Richtungssteuerung (Wave, etc.)
    bool supportsPerLedColor = false;   // Addressable LEDs

    std::vector<DeviceMode> supportedModes;

    int zoneCount = 1;                  // Anzahl unabhängiger Zonen
    int ledsPerZone = 1;                // LEDs pro Zone
    int totalLeds = 1;                  // Gesamt-LED-Anzahl

    bool usesRGBW = false;              // 4-Kanal-Farbe
    bool usesGRB = false;               // GRB statt RGB (WS2812)

    uint8_t minBrightness = 0;
    uint8_t maxBrightness = 100;
    uint8_t minSpeed = 0;
    uint8_t maxSpeed = 100;
};
```

### DeviceMode-Enum

```cpp
enum class DeviceMode : uint8_t {
    Off = 0,        // Aus
    Static,         // Statische Farbe
    Breathing,      // Pulsierend
    Wave,           // Wellenbewegung
    Spectrum,       // Regenbogenzyklus
    Reactive,       // Reaktiv auf Input
    ColorCycle,     // Farbwechsel (reserviert)
    Gradient,       // Farbverlauf (reserviert)
    Custom          // Benutzerdefiniert (reserviert)
};
```

### Capability-basierte Steuerung

- Vor jeder Operation prüfen: `if (device->GetCapabilities().supportsX)`
- `IsModeSupported(mode)` vor `SetMode()` aufrufen
- Keine harten Geräte-`if`-Ketten im Core (siehe §4 Prinzip 2)

---

## 14c) Effekt-System

### Architektur

Das Effekt-System arbeitet mit 2D-Sequenzen (Zeit × Zone):

```cpp
using EffectFrame = std::vector<RGB>;  // Eine Farbe pro Zone

struct EffectSequence2D {
    std::vector<EffectFrame> frameZoneMatrix;  // [frameIndex][zoneIndex] = RGB
    uint32_t frequencyHz = 30;                  // Abspielgeschwindigkeit
    bool loop = true;                           // Endlosschleife
};
```

### EffectFactory-Methoden

| Methode | Beschreibung |
|---------|--------------|
| `CreateStatic(color, zoneCount)` | 1-Frame-Sequenz mit konstanter Farbe |
| `CreateFlowGradient(zones, frames, start, end, hz)` | Fließender Farbverlauf über Zeit und Zonen |
| `SelectFrame(sequence, tickIndex)` | Wählt aktuellen Frame basierend auf Tick |
| `Interpolate(start, end, t)` | Lineare RGB-Interpolation |

### Erweiterungspunkte

Neue Effekte werden durch zusätzliche Factory-Methoden hinzugefügt:
- `CreateRainbow(zoneCount, frameCount, hz)`
- `CreateBreathing(color, zoneCount, frameCount, hz)`
- `CreateWave(colors, zoneCount, frameCount, direction, hz)`

Effekt-Playback erfolgt über externen Timer, der `SelectFrame()` mit inkrementierendem `tickIndex` aufruft.

---

## 14d) CLI-Referenz

### Basis-Kommandos

```bash
oneclickrgb                        # Erkannte Geräte anzeigen
oneclickrgb set 255 0 128          # Farbe setzen (RGB)
oneclickrgb set #FF0080            # Farbe setzen (Hex)
oneclickrgb off                    # Alle LEDs ausschalten
oneclickrgb mode static            # Modus setzen (static/breathing/rainbow/wave)
oneclickrgb brightness 50          # Helligkeit setzen (0-100)
oneclickrgb status                 # Gerätestatus anzeigen
oneclickrgb help                   # Hilfe anzeigen
```

### Provisioning-Kommandos

```bash
oneclickrgb provision --auto       # Auto-Provisioning aus Fingerprint
oneclickrgb provision --check      # Drift-Erkennung
oneclickrgb provision --self-heal  # Automatische Reparatur
oneclickrgb provision --rollback   # Zurück zur vorherigen Konfiguration
oneclickrgb provision --fingerprint # Hardware-Fingerprint anzeigen
oneclickrgb provision --status     # Provisioning-Status
```

### Globale Optionen

```bash
--dry-run                          # Simulation ohne Hardware-Zugriff
--verbose                          # Detaillierte Log-Ausgabe
--json                             # JSON-Output für Skript-Integration
```

### Exit-Codes

| Code | Bedeutung |
|------|-----------|
| 0 | Erfolg |
| 1 | Ungültige Argumente |
| 2 | Keine Geräte gefunden |
| 3 | Gerätekommunikationsfehler |
| 4 | Konfigurationsfehler |
| 5 | Berechtigung verweigert |

### JSON-Output Beispiel

```bash
oneclickrgb --json set 255 0 0
```

```json
{
  "success": true,
  "command": "set",
  "message": "Color set to RGB(255, 0, 0).",
  "r": 255,
  "g": 0,
  "b": 0
}
```

---

## 14e) MachineFingerprint-System

### Datenquellen (Windows)

| Quelle | Informationen |
|--------|---------------|
| WMI Win32_BaseBoard | Mainboard Hersteller, Produkt, Seriennummer |
| WMI Win32_BIOS | BIOS Vendor, Version, Datum |
| WMI Win32_Processor | CPU Name, Vendor, Cores, Threads |
| WMI Win32_VideoController | GPU Name, Vendor, Treiberversion |
| WMI Win32_PhysicalMemory | RAM Hersteller, Kapazität, Geschwindigkeit |
| SetupAPI | USB-Geräte mit VID/PID |
| HID-Enumeration | RGB-Geräte (HID) |
| SMBus-Scan | RGB-RAM (SMBus-Adressen) |

### Fingerprint-Struktur

```cpp
struct MachineFingerprint {
    MainboardInfo mainboard;        // Hersteller, Produkt, Serial
    CpuInfo cpu;                    // Name, Cores, Threads
    std::vector<GpuInfo> gpus;      // Grafikkarten
    std::vector<RamInfo> ram;       // RAM-Module
    std::vector<RgbDeviceInfo> rgbDevices;  // Erkannte RGB-Geräte

    std::string GetHash() const;    // Eindeutiger Config-Hash
    std::string GetMachineId() const; // Stabile Maschinen-ID
};
```

### Hash-Berechnung

Der Fingerprint-Hash basiert auf:
- Mainboard (Hersteller + Produkt + Serial)
- CPU
- GPU-Konfiguration
- RAM-Konfiguration (Anzahl + Gesamtkapazität)
- RGB-Geräte (VID:PID)

---

## 14f) ProvisioningService State Machine

### Zustände

```
Uninitialized → Bootstrap → Fingerprinting → Resolving → Applying → Verifying → Healthy
                                                           ↓           ↓
                                                       Failed ← Degraded
                                                           ↓
                                                     RollingBack
```

### Zustandsbeschreibungen

| Zustand | Beschreibung |
|---------|--------------|
| `Uninitialized` | Service nicht gestartet |
| `Bootstrap` | Initialisierung, Laden von Bundles |
| `Fingerprinting` | Hardware-Erkennung läuft |
| `Resolving` | Profil-Matching gegen Regeln |
| `Applying` | Profil wird angewendet |
| `Verifying` | Konfiguration wird überprüft |
| `Healthy` | Erfolgreich, System korrekt konfiguriert |
| `Degraded` | Teilweise erfolgreich, mit Einschränkungen |
| `RollingBack` | Rollback zur vorherigen Konfiguration |
| `Failed` | Provisioning fehlgeschlagen |

### MachineState-Persistenz

```json
{
  "machineId": "abc123...",
  "profileId": "gaming-profile",
  "configHash": "def456...",
  "fingerprintHash": "ghi789...",
  "provisionedAt": "2026-03-22T14:30:00",
  "bundleVersion": "1.0.0",
  "previousProfileId": "default",
  "provisionCount": 3,
  "rollbackCount": 0,
  "selfHealCount": 1
}
```

---

## 14g) ProfileResolver Regelwerk

### Prioritätsreihenfolge (höchste zuerst)

1. **MachineId** (100) — Explizite Maschinenzuweisung
2. **Tag** (50) — Standort/Umgebungs-Tag
3. **Mainboard** (30) — Mainboard-Hersteller/Produkt
4. **GPU** (25) — GPU-Vendor/Modell
5. **RAM** (25) — RAM-Hersteller
6. **Vendor** (20) — RGB-Geräte-Vendor
7. **DeviceType** (15) — RGB-Geräte-Typ (HID/SMBus)
8. **Default** (0) — Fallback-Profil

### Regel-Beispiele

```cpp
// Explizite Maschinenzuweisung
resolver.AssignToMachine("abc123...", "workstation-profile");

// Tag-basierte Regel
resolver.AddTagRule("office", "office-profile", 50);

// Mainboard-Regel
resolver.AddMainboardRule("ASUS", "ROG STRIX", "gaming-profile", 30);

// Vendor-Regel
resolver.AddVendorRule({"ASUS", "G.Skill"}, "rgb-sync-profile", 20);

// Default-Fallback
resolver.SetDefaultProfile("default");
```

---

## 15) ConfigBundle-Format (verbindlich)

```json
{
   "bundleVersion": "1.0.0",
   "createdAt": "2026-03-22T12:00:00Z",
   "target": {
      "os": "windows",
      "arch": "x64"
   },
   "profiles": [
      {
         "id": "default-gaming",
         "priority": 100,
         "match": {
            "vendors": ["ASUS", "G.Skill"],
            "deviceTypes": ["Mainboard", "RAM"]
         },
         "actions": [
            { "op": "set-mode", "value": "Static" },
            { "op": "set-color", "value": "#00AAFF" },
            { "op": "apply" }
         ]
      }
   ],
   "policies": {
      "allowRawPackets": false,
      "maxRetry": 3,
      "rollbackOnError": true
   },
   "integrity": {
      "hash": "sha256:...",
      "signature": "base64:...",
      "keyId": "ocrgb-prod-1"
   }
}
```

---

## 16) Rules Engine und Prioritätsmodell

Reihenfolge (höchste gewinnt):

1. Explizite Maschinenzuweisung (`machineId`)
2. Standort/Tag-Regel
3. Hardware-Fingerprint-Regel
4. Vendor/DeviceType-Regel
5. globales Default-Profil

Bei Gleichstand entscheidet:

- höhere `priority`
- danach neuere `bundleVersion`
- danach deterministische Sortierung nach `profile.id`

---

## 17) Zustandsmaschinen (verbindlich)

### 17.1 Device Lifecycle

`Discovered -> Initialized -> Ready -> Degraded -> Recovered | Removed`

- `Degraded` erlaubt reduzierte Features, blockiert aber nicht alle Geräte.

### 17.2 Provisioning Lifecycle

`Bootstrap -> Fingerprint -> Resolve -> Apply -> Verify -> Healthy`

Fehlerpfad:

`Apply/Verify Error -> Rollback -> SafeProfile -> RetryWindow`

---

## 18) Performance- und Stabilitätsbudgets

- QuickScan Zielzeit: <= 2 Sekunden bei Standard-HID-Umgebung
- Full Scan Zielzeit: <= 10 Sekunden inkl. SMBus
- Provisioning nach Erstinstallation: <= 30 Sekunden bis `Healthy`
- Maximaler RAM-Footprint Core+Service (idle): <= 150 MB
- Crash-Freiheit Ziel: >= 99.5 % Sitzungen pro Release-Zyklus

---

## 19) Kompatibilität und Versionierung

- `core` SemVer: `MAJOR.MINOR.PATCH`
- Plugin-Kompatibilität gegen `PluginApiVersion` statt nur App-Version
- ConfigBundle separat versioniert (`bundleVersion`)
- Migrationsregeln je MAJOR in `docs/MIGRATIONS.md` pflegen

---

## 20) Security- und Trust-Modell

- Nur signierte Bundles werden automatisch angewendet.
- Schlüsselrotation mit `keyId` + Vertrauenskette.
- Lokaler Trust Store versioniert und auditierbar.
- Elevated-Aktionen strikt gekapselt (separater Pfad/Prozess).

---

## 21) Delivery-Gates pro Phase

### Gate A (Ende Phase 0)

- `CreateDevice()` produktiv
- mindestens 4 bekannte Gerätepfade initiiert
- keine Blocker-Fehler in Scan/Apply-Basispfad

### Gate B (Ende Phase 1.5)

- Auto-Provisioning auf frischer Maschine ohne UI-Eingriff
- Rollback nach absichtlich fehlerhaftem Bundle nachweisbar
- Self-Heal korrigiert Drift im definierten Zeitfenster

### Gate C (Ende Phase 2)

- GUI und CLI liefern identisches Ergebnis für dieselbe Aktion
- Regressionssuite für Registry/Scanner/Provisioning grün

---

## 22) Offene Architekturentscheidungen (ADR-Backlog)

1. Runtime-Plugin-Laden im selben Prozess oder in isoliertem Host-Prozess
2. API-Protokoll zuerst REST oder direkt gRPC/WebSocket-first
3. Persistenz für Machine-State: JSON-Datei vs. SQLite
4. Event-Bus intern: leichtgewichtig (Observer) vs. Message-Queue-Ansatz
5. Verteilung von Bundles: rein lokal, SMB-Share, oder zentraler Endpoint

Jede Entscheidung wird als ADR dokumentiert: Kontext, Optionen, Entscheidung, Konsequenzen, Rückfallstrategie.

---

## 23) Technische Ablageorte (verbindlich)

Damit keine Sackgasse durch unklare Ownership entsteht, gelten folgende Source-of-Truth-Dateien:

### Einfaches Interface (Benutzer-facing)

- Hauptheader (einziger Include für die meisten Benutzer): `src/OneClickRGB.h`
- Implementierung: `src/OneClickRGB.cpp`
- CLI-Einstiegspunkt: `src/main.cpp`

### Core-Definitionen

- Basisdatentypen (RGB, DeviceMode, Capabilities, Result): `src/core/Types.h`
- Dry-Run-Modus (Test ohne Hardware): `src/core/DryRunMode.h`
- Device-Interface-Contract: `src/devices/IDevice.h`
- HID-Basisklasse: `src/devices/HIDDevice.h` und `src/devices/HIDDevice.cpp`
- SMBus-Basisklasse: `src/devices/SMBusDevice.h` und `src/devices/SMBusDevice.cpp`
- Device-Registry: `src/core/DeviceRegistry.h` und `src/core/DeviceRegistry.cpp`

### Bridge-Layer

- Bridge-Interface: `src/bridges/IBridge.h`
- HID-Bridge: `src/bridges/HIDBridge.h` und `src/bridges/HIDBridge.cpp`
- SMBus-Bridge: `src/bridges/SMBusBridge.h` und `src/bridges/SMBusBridge.cpp`

### Scanner und Plugin-System

- Hardware-Scanner: `src/scanner/HardwareScanner.h` und `src/scanner/HardwareScanner.cpp`
- Plugin-Factory (zentrale Device-Erzeugung): `src/plugins/PluginFactory.h` und `src/plugins/PluginFactory.cpp`

### Application-Layer

- Device-Service (Discovery + Orchestrierung): `src/app/services/DeviceService.h` und `src/app/services/DeviceService.cpp`
- Provisioning-Service (Auto-Config): `src/app/services/ProvisioningService.h` und `src/app/services/ProvisioningService.cpp`
- Profile-Resolver (Regelwerk): `src/app/services/ProfileResolver.h` und `src/app/services/ProfileResolver.cpp`
- Machine-Fingerprint (Hardware-ID): `src/app/fingerprint/MachineFingerprint.h` und `src/app/fingerprint/MachineFingerprint.cpp`
- Getter/Setup-Struktur: `src/app/config/DeviceConfiguration.h` und `src/app/config/DeviceConfiguration.cpp`
- ConfigBundle-Parser: `src/app/config/ConfigBundleParser.h` und `src/app/config/ConfigBundleParser.cpp`
- Interne Provisioning-Pipeline: `src/app/pipeline/DevicePipeline.h` und `src/app/pipeline/DevicePipeline.cpp`
- Effektfactory (2D-Array, chronologisch + frequenzbasiert): `src/app/effects/EffectFactory.h` und `src/app/effects/EffectFactory.cpp`

### Konfiguration und Daten

- Gerätedatenbank (Source): `config/hardware_db.json`
- Gerätedatenbank-Schema: `config/hardware_db.schema.json`
- ConfigBundle-Schema: `config/config_bundle.schema.json`
- Generierte Gerätedefinitionen: `build/generated/hardware_config.h`

### Tests

- Test-Framework: `tests/TestFramework.h`
- Test-Runner: `tests/test_main.cpp`
- Unit-Tests: `tests/test_types.cpp`, `tests/test_registry.cpp`, `tests/test_effects.cpp`, `tests/test_config.cpp`, `tests/test_bundle_parser.cpp`, `tests/test_dryrun.cpp`

### Dokumentation

- Architektur und Roadmap: `ARCHITECTURE.md`
- Legacy-Feature-Extraktion: `docs/LEGACY_FEATURE_EXTRACTION.md`
- Detaildokument zur Konfigurationsstruktur: `docs/CONFIG_STRUCTURE.md`

### Für den aktuellen Scope explizit ausgeschlossen

- Rechteverwaltung
- Online-APIs
- Übersetzungen (i18n)
- Barrierefreiheit
