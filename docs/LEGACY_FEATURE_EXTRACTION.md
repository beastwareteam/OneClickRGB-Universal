# Legacy-Feature-Extraktion (OneClickRGB -> Universal)

Ziel: vorhandene Funktionen aus dem Vorgänger geordnet als Features führen und in die modulare Struktur überführen.

## 1) Bereits im aktuellen Code eindeutig vorhanden

- Device Lifecycle: Initialize/Shutdown/Reconnect
- Basissteuerung: SetColor, SetMode, SetBrightness, SetSpeed, Apply
- Protokolle: HID, SMBus
- Geräteplugins:
  - ASUS Aura Mainboard
  - SteelSeries Rival
  - EVision Keyboard
  - G.Skill Trident Z5

## 2) Feature-Mapping in modulare Zielstruktur

- Core-Features -> `src/core/`
- Geräte-Features -> `src/plugins/<vendor>/`
- Protokoll-Features -> `src/bridges/`
- Orchestrierung/Setup -> `src/app/`
- Effekt-Features -> `src/app/effects/`

## 3) Extraktions-Backlog (falls zusätzliche Legacy-Dateien eingebracht werden)

1. Legacy-Funktion isolieren (Input/Output, Side Effects)
2. Capability-Mapping auf `DeviceMode` und `Capabilities`
3. Refactor in Plugin oder Core ohne Querverweise
4. Integration in Pipeline (`BuildDevicePlan` / `ApplyDevicePlan`)
5. Testfall für Regression hinzufügen

## 4) Wichtig

Dieses Repository enthält aktuell keine vollständige Legacy-Branch-Historie als Arbeitsbaum.
Für eine 1:1-Extraktion weiterer Vorgängerfeatures müssen die Legacy-Dateien in den Workspace eingebunden werden.
