//=============================================================================
// OneClickRGB-Universal - Device Registry Implementation
//=============================================================================

#include "DeviceRegistry.h"
#include "../scanner/HardwareScanner.h"

namespace OCRGB {

DeviceRegistry& DeviceRegistry::Instance() {
    static DeviceRegistry instance;
    return instance;
}

void DeviceRegistry::RegisterDevice(DevicePtr device) {
    if (!device) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_devices[device->GetId()] = device;
}

void DeviceRegistry::UnregisterDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_devices.find(deviceId);
    if (it != m_devices.end()) {
        it->second->Shutdown();
        m_devices.erase(it);
    }
}

DevicePtr DeviceRegistry::GetDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_devices.find(deviceId);
    return (it != m_devices.end()) ? it->second : nullptr;
}

std::vector<DevicePtr> DeviceRegistry::GetAllDevices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<DevicePtr> result;
    result.reserve(m_devices.size());

    for (const auto& pair : m_devices) {
        result.push_back(pair.second);
    }

    return result;
}

std::vector<DevicePtr> DeviceRegistry::GetDevicesByType(DeviceType type) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<DevicePtr> result;

    for (const auto& pair : m_devices) {
        if (pair.second->GetInfo().type == type) {
            result.push_back(pair.second);
        }
    }

    return result;
}

size_t DeviceRegistry::GetDeviceCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_devices.size();
}

void DeviceRegistry::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& pair : m_devices) {
        pair.second->Shutdown();
    }

    m_devices.clear();
}

void DeviceRegistry::SetColorAll(const RGB& color) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& pair : m_devices) {
        if (pair.second->IsReady()) {
            pair.second->SetColor(color);
        }
    }
}

void DeviceRegistry::SetModeAll(DeviceMode mode) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& pair : m_devices) {
        if (pair.second->IsReady() && pair.second->IsModeSupported(mode)) {
            pair.second->SetMode(mode);
        }
    }
}

void DeviceRegistry::SetBrightnessAll(uint8_t brightness) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& pair : m_devices) {
        if (pair.second->IsReady()) {
            pair.second->SetBrightness(brightness);
        }
    }
}

void DeviceRegistry::ApplyAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& pair : m_devices) {
        if (pair.second->IsReady()) {
            pair.second->Apply();
        }
    }
}

void DeviceRegistry::TurnOffAll() {
    SetColorAll(RGB(0, 0, 0));
    ApplyAll();
}

int DeviceRegistry::DiscoverDevices() {
    HardwareScanner scanner;
    auto results = scanner.QuickScan();

    int count = 0;
    for (const auto& result : results) {
        if (result.isKnown) {
            DevicePtr device = scanner.CreateDevice(result);
            if (device) {
                if (device->Initialize().IsSuccess()) {
                    RegisterDevice(device);
                    count++;
                }
            }
        }
    }

    return count;
}

void DeviceRegistry::RefreshConnections() {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::string> disconnected;

    for (auto& pair : m_devices) {
        if (!pair.second->IsConnected()) {
            // Try to reconnect
            if (pair.second->Reconnect().IsError()) {
                disconnected.push_back(pair.first);
            }
        }
    }

    // Remove devices that couldn't reconnect
    for (const auto& id : disconnected) {
        m_devices.erase(id);
    }
}

} // namespace OCRGB
