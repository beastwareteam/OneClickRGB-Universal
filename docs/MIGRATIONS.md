# Migrationsanleitungen

Dokumentation für Breaking Changes zwischen Major-Versionen.

---

## v1.x → v2.x (geplant)

*Noch keine Breaking Changes*

---

## v0.x → v1.0

### Änderungen

| Bereich | v0.x | v1.0 |
|---------|------|------|
| Namespace | `OCRGB` | `OCRGB` (unverändert) |
| Result-Typ | `bool` | `Result` mit `ResultCode` |
| Device-Interface | Direkte Methoden | `IDevice` Interface |
| Plugin-System | Hardcoded | `PluginFactory` |
| Konfiguration | Keine | `hardware_db.json` |

### Result-Migration

**Vorher (v0.x):**
```cpp
bool success = device->SetColor(255, 0, 0);
if (!success) {
    // Fehlerbehandlung
}
```

**Nachher (v1.0):**
```cpp
Result result = device->SetColor(RGB{255, 0, 0});
if (!result.IsSuccess()) {
    std::cerr << "Error: " << result.message << std::endl;
    // Spezifische Fehlerbehandlung basierend auf result.code
}
```

### Device-Creation-Migration

**Vorher (v0.x):**
```cpp
AsusAuraController* device = new AsusAuraController();
device->Initialize();
```

**Nachher (v1.0):**
```cpp
// Option 1: Einfaches API
OneClickRGB rgb;
rgb.Start();  // Auto-Detection

// Option 2: Manuelle Erstellung via Factory
auto device = PluginFactory::Create(deviceDef);
device->Initialize();
```

### Capabilities-Migration

**Vorher (v0.x):**
```cpp
if (device->SupportsRainbow()) {
    device->SetModeRainbow();
}
```

**Nachher (v1.0):**
```cpp
Capabilities caps = device->GetCapabilities();
if (caps.IsModeSupported(DeviceMode::Spectrum)) {
    device->SetMode(DeviceMode::Spectrum);
}
```

---

## Plugin-API-Versionen

| API-Version | Core-Version | Änderungen |
|-------------|--------------|------------|
| 1 | 1.0.0 | Initial |

### Kompatibilitätsmatrix

Plugins mit `PluginApiVersion = 1` funktionieren mit Core `1.x.x`.

Bei Major-Updates (z.B. v2.0):
- Alte Plugins werden isoliert deaktiviert
- Core läuft weiter
- Warnung im Log

---

## ConfigBundle-Versionen

| Bundle-Version | Änderungen |
|----------------|------------|
| 1.0.0 | Initial: profiles, actions, policies |

### Bundle-Migration

Bundles sind rückwärtskompatibel innerhalb derselben Major-Version.

Bei Breaking Changes:
1. Neue `bundleVersion` deklarieren
2. Migrationsskript bereitstellen
3. Alte Version für Übergangszeit unterstützen

---

## Datenbank-Schema-Versionen

| Schema-Version | Änderungen |
|----------------|------------|
| 1.0.0 | Initial: devices, protocols |

### Schema-Migration

`tools/migrate_hardware_db.py` (geplant) für automatische Migration.

---

## Deprecation-Policy

1. **Deprecation-Warnung** in Minor-Version (z.B. v1.1)
2. **Entfernung** in nächster Major-Version (z.B. v2.0)
3. **Migration-Guide** in dieser Datei

### Aktuell deprecated

*Keine*

---

## Rollback-Anleitung

Falls ein Update Probleme verursacht:

```bash
# Git-basierter Rollback
git checkout v1.0.0
cd build && cmake .. && make

# Provisioning-Rollback
./oneclickrgb provision --rollback
```

---

*Letzte Aktualisierung: März 2026*
