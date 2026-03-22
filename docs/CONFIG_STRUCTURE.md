# Konfigurationsstruktur (verbindlich)

*Letzte Aktualisierung: 2026-03-22*

Dieses Dokument hält fest, **wo** die Konfigurationsstruktur, Getter/Setup-Logik und interne Pipelines definiert sind.

## 1) Source of Truth

- Bundle-Schema: `config/config_bundle.schema.json`
- Hardware-DB-Schema: `config/hardware_db.schema.json`
- Hardwaredaten: `config/hardware_db.json`
- Generierte Gerätedefinitionen: `build/generated/hardware_config.h`
- Architekturregeln und Gates: `ARCHITECTURE.md`

## 2) Setup- und Getter-Struktur

- Zentrale Modellierung: `src/app/config/DeviceConfiguration.h`
- Implementierung der Getter/Setup-Flows: `src/app/config/DeviceConfiguration.cpp`

Verbindliche Reihenfolge:
1. `SetupFromBundle(...)`
2. `SetupFromHardwareMatch(...)`
3. `SetResolvedProfile(...)`
4. Getter-Aufrufe (`GetDeviceKey`, `GetMode`, `GetDefaultColor`, ...)

## 3) Device Service (Orchestrierung)

- Service-Interface: `src/app/services/DeviceService.h`
- Service-Implementierung: `src/app/services/DeviceService.cpp`

Der DeviceService koordiniert Scanner, Pipeline und Registry:

```
DiscoverAndRegister()
    └── QuickScan() → CreateDevice() → Initialize() → ProvisionDevice() → RegisterDevice()
```

**Wichtig:** Die DeviceRegistry (`src/core/`) darf nicht direkt auf den app-Layer zugreifen. Für Discovery mit Pipeline-Ausführung muss `App::DeviceService` verwendet werden.

## 4) Interne Pipeline

- Pipeline-Contracts: `src/app/pipeline/DevicePipeline.h`
- Standardpipeline: `src/app/pipeline/DevicePipeline.cpp`

Verbindliche Stages:

| Stage | Funktion | Status |
|-------|----------|--------|
| `Fingerprint` | Setzt DeviceKey aus `vendor:model:id` | Implementiert |
| `ResolveProfile` | Wählt Profil oder Fallback `default-auto` | Implementiert |
| `BuildDevicePlan` | Validiert DeviceKey | Minimal (TODO: Plan-Erstellung) |
| `ApplyDevicePlan` | SetColor → SetMode → SetBrightness → SetSpeed → Apply | Implementiert |
| `VerifyAndPersist` | Prüft `IsReady()` | Minimal (TODO: Persistierung)

## 5) Ausschlüsse (explizit)

Für dieses Projektziel sind **nicht** Bestandteil der Pipeline:
- Rechteverwaltung
- Online-APIs
- Übersetzungen/i18n
- Barrierefreiheit

## 6) Effektlogik

- Factory + Sequenzmodell: `src/app/effects/EffectFactory.h`
- Grundalgorithmen: `src/app/effects/EffectFactory.cpp`

Der Sequenzträger ist ein 2D-Modell:
- Achse 1: Zeit/Frame (chronologisch)
- Achse 2: technische Ausgabegruppe (z. B. Zone/Channel)

Damit sind fließende Effekte über Zeit und Frequenz steuerbar.
