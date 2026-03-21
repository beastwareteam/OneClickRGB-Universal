#pragma once
//=============================================================================
// OneClickRGB-Universal - Device Registry
//=============================================================================
// Central registry for managing connected devices.
//=============================================================================

#include "Types.h"
#include "../devices/IDevice.h"
#include <vector>
#include <map>
#include <mutex>

namespace OCRGB {

//-----------------------------------------------------------------------------
// Device Registry
//-----------------------------------------------------------------------------
class DeviceRegistry {
public:
    static DeviceRegistry& Instance();

    // Prevent copying
    DeviceRegistry(const DeviceRegistry&) = delete;
    DeviceRegistry& operator=(const DeviceRegistry&) = delete;

    //=========================================================================
    // Device Management
    //=========================================================================

    /// Register a device
    void RegisterDevice(DevicePtr device);

    /// Unregister a device by ID
    void UnregisterDevice(const std::string& deviceId);

    /// Get a device by ID
    DevicePtr GetDevice(const std::string& deviceId);

    /// Get all registered devices
    std::vector<DevicePtr> GetAllDevices();

    /// Get devices by type
    std::vector<DevicePtr> GetDevicesByType(DeviceType type);

    /// Get device count
    size_t GetDeviceCount() const;

    /// Clear all devices
    void Clear();

    //=========================================================================
    // Bulk Operations
    //=========================================================================

    /// Set color on all devices
    void SetColorAll(const RGB& color);

    /// Set mode on all devices
    void SetModeAll(DeviceMode mode);

    /// Set brightness on all devices
    void SetBrightnessAll(uint8_t brightness);

    /// Apply changes on all devices
    void ApplyAll();

    /// Turn off all devices
    void TurnOffAll();

    //=========================================================================
    // Discovery
    //=========================================================================

    /// Scan and register all connected devices
    int DiscoverDevices();

    /// Refresh device connections
    void RefreshConnections();

private:
    DeviceRegistry() = default;
    ~DeviceRegistry() = default;

    std::map<std::string, DevicePtr> m_devices;
    mutable std::mutex m_mutex;
};

} // namespace OCRGB
