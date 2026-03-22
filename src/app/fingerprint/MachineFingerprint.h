#pragma once
//=============================================================================
// OneClickRGB-Universal - Machine Fingerprint
//=============================================================================
// Collects hardware information to uniquely identify a machine.
// Used for automatic profile matching and provisioning.
//
// Data Sources (Windows):
//   - WMI: Mainboard, BIOS, CPU, RAM
//   - SetupAPI: GPU, USB devices
//   - HID Enumeration: RGB devices
//   - SMBus Scan: RAM modules
//
// Usage:
//   auto fp = MachineFingerprint::Collect();
//   std::string hash = fp.GetHash();
//   std::string json = fp.ToJson();
//
//=============================================================================

#include <string>
#include <vector>
#include <cstdint>

namespace OCRGB {
namespace App {

//=============================================================================
// Component Info Structures
//=============================================================================

struct MainboardInfo {
    std::string manufacturer;
    std::string product;
    std::string serialNumber;
    std::string biosVendor;
    std::string biosVersion;
    std::string biosDate;
};

struct CpuInfo {
    std::string name;
    std::string vendor;
    int cores = 0;
    int threads = 0;
};

struct GpuInfo {
    std::string name;
    std::string vendor;
    std::string driverVersion;
};

struct RamInfo {
    std::string manufacturer;
    std::string partNumber;
    uint64_t capacityBytes = 0;
    uint32_t speedMHz = 0;
    std::string slot;
};

struct UsbDeviceInfo {
    uint16_t vendorId = 0;
    uint16_t productId = 0;
    std::string description;
    std::string path;
};

struct RgbDeviceInfo {
    std::string name;
    std::string type;          // "HID", "SMBus", etc.
    std::string vendor;
    std::string model;
    uint16_t vendorId = 0;
    uint16_t productId = 0;
    uint8_t smbusAddress = 0;
};

//=============================================================================
// Machine Fingerprint
//=============================================================================

class MachineFingerprint {
public:
    //=========================================================================
    // Factory Method
    //=========================================================================

    /// Collect fingerprint from current machine
    static MachineFingerprint Collect();

    /// Create empty fingerprint
    MachineFingerprint() = default;

    //=========================================================================
    // Accessors
    //=========================================================================

    const MainboardInfo& GetMainboard() const { return m_mainboard; }
    const CpuInfo& GetCpu() const { return m_cpu; }
    const std::vector<GpuInfo>& GetGpus() const { return m_gpus; }
    const std::vector<RamInfo>& GetRam() const { return m_ram; }
    const std::vector<UsbDeviceInfo>& GetUsbDevices() const { return m_usbDevices; }
    const std::vector<RgbDeviceInfo>& GetRgbDevices() const { return m_rgbDevices; }

    /// Get unique hash for this machine configuration
    std::string GetHash() const;

    /// Get stable machine ID (based on mainboard serial)
    std::string GetMachineId() const;

    /// Serialize to JSON
    std::string ToJson() const;

    /// Deserialize from JSON
    static MachineFingerprint FromJson(const std::string& json);

    //=========================================================================
    // Comparison
    //=========================================================================

    /// Check if fingerprints match (same machine)
    bool Matches(const MachineFingerprint& other) const;

    /// Get drift report between fingerprints
    std::vector<std::string> GetDrift(const MachineFingerprint& previous) const;

    //=========================================================================
    // Modifiers (for testing)
    //=========================================================================

    void SetMainboard(const MainboardInfo& info) { m_mainboard = info; }
    void SetCpu(const CpuInfo& info) { m_cpu = info; }
    void AddGpu(const GpuInfo& info) { m_gpus.push_back(info); }
    void AddRam(const RamInfo& info) { m_ram.push_back(info); }
    void AddUsbDevice(const UsbDeviceInfo& info) { m_usbDevices.push_back(info); }
    void AddRgbDevice(const RgbDeviceInfo& info) { m_rgbDevices.push_back(info); }

private:
    MainboardInfo m_mainboard;
    CpuInfo m_cpu;
    std::vector<GpuInfo> m_gpus;
    std::vector<RamInfo> m_ram;
    std::vector<UsbDeviceInfo> m_usbDevices;
    std::vector<RgbDeviceInfo> m_rgbDevices;

    // Collection helpers
    static void CollectMainboard(MachineFingerprint& fp);
    static void CollectCpu(MachineFingerprint& fp);
    static void CollectGpu(MachineFingerprint& fp);
    static void CollectRam(MachineFingerprint& fp);
    static void CollectUsbDevices(MachineFingerprint& fp);
    static void CollectRgbDevices(MachineFingerprint& fp);

    // Hash helper
    static std::string Sha256(const std::string& input);
};

} // namespace App
} // namespace OCRGB
