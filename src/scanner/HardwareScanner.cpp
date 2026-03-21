//=============================================================================
// OneClickRGB-Universal - Hardware Scanner Implementation
//=============================================================================

#include "HardwareScanner.h"
#include "../bridges/HIDBridge.h"
#include "../bridges/SMBusBridge.h"
#include <hidapi.h>

namespace OCRGB {

HardwareScanner::HardwareScanner() = default;
HardwareScanner::~HardwareScanner() = default;

void HardwareScanner::ReportProgress(int current, int total, const std::string& status) {
    if (m_progressCallback) {
        m_progressCallback(current, total, status);
    }
}

void HardwareScanner::ReportDeviceFound(const ScanResult& result) {
    if (m_deviceFoundCallback) {
        m_deviceFoundCallback(result);
    }
}

//-----------------------------------------------------------------------------
// HID Scanning
//-----------------------------------------------------------------------------

std::vector<ScanResult> HardwareScanner::ScanHID() {
    std::vector<ScanResult> results;

    ReportProgress(0, 1, "Scanning HID devices...");
    EnumerateHIDDevices(results);
    ReportProgress(1, 1, "HID scan complete");

    return results;
}

void HardwareScanner::EnumerateHIDDevices(std::vector<ScanResult>& results) {
    struct hid_device_info* devs = hid_enumerate(0, 0);  // All devices
    if (!devs) return;

    int deviceCount = 0;
    for (struct hid_device_info* cur = devs; cur; cur = cur->next) {
        deviceCount++;
    }

    int current = 0;
    for (struct hid_device_info* cur = devs; cur; cur = cur->next) {
        ScanResult result;
        result.address.protocol = ProtocolType::HID;
        result.address.vendorId = cur->vendor_id;
        result.address.productId = cur->product_id;
        result.address.usagePage = cur->usage_page;
        result.address.usage = cur->usage;

        if (cur->path) {
            // Convert path to wstring
            size_t len = strlen(cur->path);
            result.address.path.resize(len);
            for (size_t i = 0; i < len; i++) {
                result.address.path[i] = cur->path[i];
            }
        }

        if (cur->manufacturer_string) {
            // Convert wchar to string
            std::wstring ws(cur->manufacturer_string);
            result.manufacturer = std::string(ws.begin(), ws.end());
        }

        if (cur->product_string) {
            std::wstring ws(cur->product_string);
            result.product = std::string(ws.begin(), ws.end());
        }

        if (cur->serial_number) {
            std::wstring ws(cur->serial_number);
            result.serialNumber = std::string(ws.begin(), ws.end());
        }

        // Match against known devices
        MatchHIDDevice(result);

        results.push_back(result);
        ReportDeviceFound(result);

        current++;
        ReportProgress(current, deviceCount, "Scanning: " + result.product);
    }

    hid_free_enumeration(devs);
}

void HardwareScanner::MatchHIDDevice(ScanResult& result) {
    const auto* def = HardwareDB::FindDeviceByVidPidUsage(
        result.address.vendorId,
        result.address.productId,
        result.address.usagePage
    );

    if (def) {
        result.isKnown = true;
        result.knownDevice = def;
    }
}

//-----------------------------------------------------------------------------
// SMBus Scanning
//-----------------------------------------------------------------------------

std::vector<ScanResult> HardwareScanner::ScanSMBus() {
    std::vector<ScanResult> results;

    ReportProgress(0, 1, "Scanning SMBus devices...");

    SMBusBridge bridge;
    if (!bridge.Initialize()) {
        ReportProgress(1, 1, "SMBus not available (PawnIO required)");
        return results;
    }

    EnumerateSMBusDevices(results);
    bridge.Shutdown();

    ReportProgress(1, 1, "SMBus scan complete");

    return results;
}

void HardwareScanner::EnumerateSMBusDevices(std::vector<ScanResult>& results) {
    SMBusBridge bridge;
    if (!bridge.Initialize()) return;

    std::vector<uint8_t> addresses;
    int count = bridge.ScanBus(addresses);

    for (int i = 0; i < count; i++) {
        ScanResult result;
        result.address.protocol = ProtocolType::SMBus;
        result.address.busNumber = 0;  // Default bus
        result.address.deviceAddress = addresses[i];

        result.manufacturer = "Unknown";
        result.product = "SMBus Device";

        char addrStr[16];
        snprintf(addrStr, sizeof(addrStr), "0x%02X", addresses[i]);
        result.serialNumber = addrStr;

        // Match against known devices
        MatchSMBusDevice(result);

        results.push_back(result);
        ReportDeviceFound(result);

        ReportProgress(i + 1, count, "Scanning SMBus address " + std::string(addrStr));
    }

    bridge.Shutdown();
}

void HardwareScanner::MatchSMBusDevice(ScanResult& result) {
    // Check known SMBus device addresses
    // G.Skill RAM typically at 0x58-0x5B
    uint8_t addr = result.address.deviceAddress;

    for (const auto& dev : HardwareDB::GetKnownDevices()) {
        if (dev.protocol != ProtocolType::SMBus) continue;

        // For now, match G.Skill by address range
        if (dev.id && strstr(dev.id, "gskill")) {
            if (addr >= 0x58 && addr <= 0x5B) {
                result.isKnown = true;
                result.knownDevice = &dev;
                result.manufacturer = dev.vendor;
                result.product = dev.name;
                break;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Combined Scanning
//-----------------------------------------------------------------------------

std::vector<ScanResult> HardwareScanner::ScanAll() {
    std::vector<ScanResult> results;

    auto hidResults = ScanHID();
    results.insert(results.end(), hidResults.begin(), hidResults.end());

    auto smbusResults = ScanSMBus();
    results.insert(results.end(), smbusResults.begin(), smbusResults.end());

    return results;
}

std::vector<ScanResult> HardwareScanner::QuickScan() {
    std::vector<ScanResult> results;

    // Only scan for known devices
    for (const auto& def : HardwareDB::GetKnownDevices()) {
        if (def.protocol == ProtocolType::HID) {
            // Try to open device
            HIDBridge bridge;
            DeviceAddress addr;
            addr.protocol = ProtocolType::HID;
            addr.vendorId = def.vendorId;
            addr.productId = def.productId;
            addr.usagePage = def.usagePage;

            if (bridge.Open(addr)) {
                ScanResult result;
                result.address = addr;
                result.manufacturer = def.vendor;
                result.product = def.name;
                result.isKnown = true;
                result.knownDevice = &def;

                results.push_back(result);
                ReportDeviceFound(result);

                bridge.Close();
            }
        }
    }

    return results;
}

//-----------------------------------------------------------------------------
// Device Matching
//-----------------------------------------------------------------------------

const HardwareDB::DeviceDefinition* HardwareScanner::MatchDevice(const DeviceAddress& addr) {
    if (addr.protocol == ProtocolType::HID) {
        return HardwareDB::FindDeviceByVidPidUsage(
            addr.vendorId, addr.productId, addr.usagePage);
    }

    // For other protocols, iterate and match
    for (const auto& def : HardwareDB::GetKnownDevices()) {
        if (def.protocol == addr.protocol) {
            // Protocol-specific matching
            return &def;
        }
    }

    return nullptr;
}

const HardwareDB::DeviceDefinition* HardwareScanner::MatchByVidPid(uint16_t vid, uint16_t pid) {
    return HardwareDB::FindDeviceByVidPid(vid, pid);
}

//-----------------------------------------------------------------------------
// Device Creation
//-----------------------------------------------------------------------------

DevicePtr HardwareScanner::CreateDevice(const ScanResult& result) {
    if (!result.isKnown || !result.knownDevice) {
        return nullptr;
    }

    return CreateDevice(*result.knownDevice);
}

DevicePtr HardwareScanner::CreateDevice(const HardwareDB::DeviceDefinition& def) {
    // This will be implemented when we add the plugin devices
    // For now, return nullptr
    // TODO: Create appropriate device based on def.id

    return nullptr;
}

} // namespace OCRGB
