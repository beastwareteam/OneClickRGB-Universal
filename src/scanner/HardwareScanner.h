#pragma once
//=============================================================================
// OneClickRGB-Universal - Hardware Scanner
//=============================================================================
// Scans for connected RGB devices and matches them against known database.
//=============================================================================

#include "../core/Types.h"
#include "../devices/IDevice.h"
#include "../../build/generated/hardware_config.h"
#include <vector>
#include <functional>

namespace OCRGB {

//-----------------------------------------------------------------------------
// Scan Result
//-----------------------------------------------------------------------------
struct ScanResult {
    DeviceAddress address;
    std::string manufacturer;
    std::string product;
    std::string serialNumber;

    // Matching against known devices
    bool isKnown = false;
    const HardwareDB::DeviceDefinition* knownDevice = nullptr;
};

//-----------------------------------------------------------------------------
// Scanner Callbacks
//-----------------------------------------------------------------------------
using ScanProgressCallback = std::function<void(int current, int total, const std::string& status)>;
using DeviceFoundCallback = std::function<void(const ScanResult& result)>;

//-----------------------------------------------------------------------------
// Hardware Scanner
//-----------------------------------------------------------------------------
class HardwareScanner {
public:
    HardwareScanner();
    ~HardwareScanner();

    //=========================================================================
    // Scanning
    //=========================================================================

    /// Scan for all HID devices
    std::vector<ScanResult> ScanHID();

    /// Scan for all SMBus devices
    std::vector<ScanResult> ScanSMBus();

    /// Scan all buses (HID + SMBus)
    std::vector<ScanResult> ScanAll();

    /// Quick scan - only look for known devices
    std::vector<ScanResult> QuickScan();

    //=========================================================================
    // Device Matching
    //=========================================================================

    /// Check if a device matches any known device
    const HardwareDB::DeviceDefinition* MatchDevice(const DeviceAddress& addr);

    /// Check if a device matches by VID/PID
    const HardwareDB::DeviceDefinition* MatchByVidPid(uint16_t vid, uint16_t pid);

    //=========================================================================
    // Device Creation
    //=========================================================================

    /// Create a device instance from scan result
    DevicePtr CreateDevice(const ScanResult& result);

    /// Create a device instance from known definition
    DevicePtr CreateDevice(const HardwareDB::DeviceDefinition& def);

    //=========================================================================
    // Callbacks
    //=========================================================================

    void SetProgressCallback(ScanProgressCallback cb) { m_progressCallback = cb; }
    void SetDeviceFoundCallback(DeviceFoundCallback cb) { m_deviceFoundCallback = cb; }

private:
    ScanProgressCallback m_progressCallback;
    DeviceFoundCallback m_deviceFoundCallback;

    void ReportProgress(int current, int total, const std::string& status);
    void ReportDeviceFound(const ScanResult& result);

    // HID scanning helpers
    void EnumerateHIDDevices(std::vector<ScanResult>& results);
    void MatchHIDDevice(ScanResult& result);

    // SMBus scanning helpers
    void EnumerateSMBusDevices(std::vector<ScanResult>& results);
    void MatchSMBusDevice(ScanResult& result);
};

} // namespace OCRGB
