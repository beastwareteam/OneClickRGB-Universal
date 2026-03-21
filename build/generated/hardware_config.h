#pragma once
//=============================================================================
// OneClickRGB-Universal - Generated Hardware Configuration
//=============================================================================
// AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
// Generated: 2026-03-21T21:36:39.257541
// Database Version: 1.0.0
//=============================================================================

#include "../src/core/Types.h"
#include <vector>
#include <string>

namespace OCRGB {
namespace HardwareDB {

//-----------------------------------------------------------------------------
// Device Definition Structure
//-----------------------------------------------------------------------------
struct DeviceDefinition {
    const char* id;
    const char* name;
    const char* vendor;
    DeviceType type;
    ProtocolType protocol;
    uint16_t vendorId;
    uint16_t productId;
    uint16_t usagePage;
    uint8_t reportId;
    uint8_t packetSize;
    uint8_t zones;
    uint8_t ledsPerZone;
    std::vector<DeviceMode> supportedModes;
};

//-----------------------------------------------------------------------------
// Known Devices Database
//-----------------------------------------------------------------------------
inline const std::vector<DeviceDefinition>& GetKnownDevices() {
    static const std::vector<DeviceDefinition> devices = {
    {
        "asus_aura_mainboard",                          // id
        "ASUS Aura Mainboard",                               // name
        "ASUS",                        // vendor
        DeviceType::Mainboard,                          // type
        ProtocolType::HID,                             // protocol
        0x0B05,                            // vendorId
        0x19AF,                            // productId
        0xFF72,                     // usagePage
        0xB0,                      // reportId
        65,                          // packetSize
        8,                                // zones
        60,                        // ledsPerZone
        { DeviceMode::Static, DeviceMode::Breathing, DeviceMode::Wave, DeviceMode::Spectrum, DeviceMode::Reactive }  // supportedModes
    },
    {
        "steelseries_rival_600",                          // id
        "SteelSeries Rival 600",                               // name
        "SteelSeries",                        // vendor
        DeviceType::Mouse,                          // type
        ProtocolType::HID,                             // protocol
        0x1038,                            // vendorId
        0x1724,                            // productId
        0x0000,                     // usagePage
        0x00,                      // reportId
        65,                          // packetSize
        2,                                // zones
        1,                        // ledsPerZone
        { DeviceMode::Static, DeviceMode::Breathing, DeviceMode::Spectrum }  // supportedModes
    },
    {
        "evision_keyboard",                          // id
        "EVision RGB Keyboard",                               // name
        "EVision",                        // vendor
        DeviceType::Keyboard,                          // type
        ProtocolType::HID,                             // protocol
        0x3299,                            // vendorId
        0x4E9F,                            // productId
        0xFF1C,                     // usagePage
        0x04,                      // reportId
        64,                          // packetSize
        1,                                // zones
        1,                        // ledsPerZone
        { DeviceMode::Static, DeviceMode::Breathing, DeviceMode::Wave, DeviceMode::Reactive, DeviceMode::Spectrum }  // supportedModes
    },
    {
        "gskill_trident_z5",                          // id
        "G.Skill Trident Z5 RGB",                               // name
        "G.Skill",                        // vendor
        DeviceType::RAM,                          // type
        ProtocolType::SMBus,                             // protocol
        0x0000,                            // vendorId
        0x0000,                            // productId
        0x0000,                     // usagePage
        0x00,                      // reportId
        65,                          // packetSize
        1,                                // zones
        8,                        // ledsPerZone
        { DeviceMode::Static }  // supportedModes
    }
    };
    return devices;
}

//-----------------------------------------------------------------------------
// Device Lookup Functions
//-----------------------------------------------------------------------------
inline const DeviceDefinition* FindDeviceById(const std::string& id) {
    for (const auto& dev : GetKnownDevices()) {
        if (dev.id == id) return &dev;
    }
    return nullptr;
}

inline const DeviceDefinition* FindDeviceByVidPid(uint16_t vid, uint16_t pid) {
    for (const auto& dev : GetKnownDevices()) {
        if (dev.vendorId == vid && dev.productId == pid) return &dev;
    }
    return nullptr;
}

inline const DeviceDefinition* FindDeviceByVidPidUsage(uint16_t vid, uint16_t pid, uint16_t usagePage) {
    for (const auto& dev : GetKnownDevices()) {
        if (dev.vendorId == vid && dev.productId == pid) {
            if (usagePage == 0 || dev.usagePage == 0 || dev.usagePage == usagePage) {
                return &dev;
            }
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
// Statistics
//-----------------------------------------------------------------------------
inline size_t GetKnownDeviceCount() {
    return GetKnownDevices().size();
}

} // namespace HardwareDB
} // namespace OCRGB
