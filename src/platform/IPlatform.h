#pragma once
//=============================================================================
// OneClickRGB-Universal - Platform Abstraction Layer
//=============================================================================
// Cross-platform interface for OS-specific functionality.
//
// Supported Platforms:
//   - Windows (WMI, SetupAPI, PawnIO)
//   - Linux (sysfs, udev, i2c-dev)
//   - macOS (IOKit, sysctl)
//
// Usage:
//   auto platform = IPlatform::Create();
//   platform->Initialize();
//
//   auto sysInfo = platform->GetSystemInfo();
//   auto mainboard = sysInfo->GetMainboardProduct();
//
//   auto smbus = platform->GetSMBusProvider();
//   if (smbus->IsAvailable()) {
//       smbus->WriteByte(0x58, 0x00, 0xFF);
//   }
//
//=============================================================================

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

namespace OCRGB {
namespace Platform {

//=============================================================================
// Platform Type
//=============================================================================

enum class PlatformType {
    Windows,
    Linux,
    macOS,
    Unknown
};

inline const char* PlatformTypeToString(PlatformType type) {
    switch (type) {
        case PlatformType::Windows: return "Windows";
        case PlatformType::Linux:   return "Linux";
        case PlatformType::macOS:   return "macOS";
        default:                    return "Unknown";
    }
}

//=============================================================================
// Info Structures
//=============================================================================

struct GpuInfo {
    std::string name;
    std::string vendor;
    std::string driverVersion;
    uint64_t memoryBytes = 0;
};

struct RamModuleInfo {
    std::string manufacturer;
    std::string partNumber;
    std::string serialNumber;
    uint64_t capacityBytes = 0;
    uint32_t speedMHz = 0;
    std::string slot;
    std::string type;  // DDR4, DDR5, etc.
};

struct UsbDeviceInfo {
    uint16_t vendorId = 0;
    uint16_t productId = 0;
    std::string path;
    std::string description;
    std::string manufacturer;
    std::string serialNumber;
};

struct HidDeviceInfo {
    uint16_t vendorId = 0;
    uint16_t productId = 0;
    uint16_t usagePage = 0;
    uint16_t usage = 0;
    std::string path;
    std::string manufacturer;
    std::string product;
    std::string serialNumber;
    int interfaceNumber = -1;
};

//=============================================================================
// ISystemInfo - Hardware Information Interface
//=============================================================================

class ISystemInfo {
public:
    virtual ~ISystemInfo() = default;

    //=========================================================================
    // Mainboard
    //=========================================================================
    virtual std::string GetMainboardManufacturer() = 0;
    virtual std::string GetMainboardProduct() = 0;
    virtual std::string GetMainboardSerial() = 0;
    virtual std::string GetMainboardVersion() = 0;

    //=========================================================================
    // BIOS/UEFI
    //=========================================================================
    virtual std::string GetBiosVendor() = 0;
    virtual std::string GetBiosVersion() = 0;
    virtual std::string GetBiosDate() = 0;

    //=========================================================================
    // CPU
    //=========================================================================
    virtual std::string GetCpuName() = 0;
    virtual std::string GetCpuVendor() = 0;
    virtual int GetCpuCores() = 0;
    virtual int GetCpuThreads() = 0;
    virtual uint32_t GetCpuFrequencyMHz() = 0;

    //=========================================================================
    // GPU
    //=========================================================================
    virtual std::vector<GpuInfo> GetGpus() = 0;

    //=========================================================================
    // RAM
    //=========================================================================
    virtual std::vector<RamModuleInfo> GetRamModules() = 0;
    virtual uint64_t GetTotalRamBytes() = 0;

    //=========================================================================
    // OS
    //=========================================================================
    virtual std::string GetOsName() = 0;
    virtual std::string GetOsVersion() = 0;
    virtual std::string GetKernelVersion() = 0;
    virtual std::string GetArchitecture() = 0;
    virtual std::string GetHostname() = 0;
};

//=============================================================================
// IDeviceEnumerator - USB/HID Device Enumeration
//=============================================================================

class IHidDevice {
public:
    virtual ~IHidDevice() = default;

    virtual bool IsOpen() const = 0;
    virtual void Close() = 0;

    virtual int Write(const uint8_t* data, size_t length) = 0;
    virtual int Read(uint8_t* buffer, size_t length, int timeoutMs = 1000) = 0;
    virtual int SendFeatureReport(const uint8_t* data, size_t length) = 0;
    virtual int GetFeatureReport(uint8_t* buffer, size_t length) = 0;

    virtual std::string GetManufacturer() const = 0;
    virtual std::string GetProduct() const = 0;
    virtual std::string GetSerialNumber() const = 0;
};

class IDeviceEnumerator {
public:
    virtual ~IDeviceEnumerator() = default;

    //=========================================================================
    // USB Enumeration
    //=========================================================================
    virtual std::vector<UsbDeviceInfo> EnumerateUSB() = 0;
    virtual std::vector<UsbDeviceInfo> EnumerateUSB(uint16_t vendorId) = 0;

    //=========================================================================
    // HID Enumeration
    //=========================================================================
    virtual std::vector<HidDeviceInfo> EnumerateHID() = 0;
    virtual std::vector<HidDeviceInfo> EnumerateHID(uint16_t vendorId, uint16_t productId) = 0;

    //=========================================================================
    // Device Access
    //=========================================================================
    virtual std::unique_ptr<IHidDevice> OpenHID(const std::string& path) = 0;
    virtual std::unique_ptr<IHidDevice> OpenHID(uint16_t vendorId, uint16_t productId,
                                                 uint16_t usagePage = 0, uint16_t usage = 0) = 0;

    //=========================================================================
    // Hotplug (Optional)
    //=========================================================================
    using HotplugCallback = std::function<void(const UsbDeviceInfo& device, bool connected)>;

    virtual bool SupportsHotplug() const { return false; }
    virtual void RegisterHotplugCallback(HotplugCallback callback) { (void)callback; }
    virtual void UnregisterHotplugCallback() {}
};

//=============================================================================
// ISMBusProvider - System Management Bus Access
//=============================================================================

class ISMBusProvider {
public:
    virtual ~ISMBusProvider() = default;

    //=========================================================================
    // Availability
    //=========================================================================
    virtual bool IsAvailable() const = 0;
    virtual bool RequiresElevation() const = 0;
    virtual std::string GetDriverInfo() const = 0;

    //=========================================================================
    // Lifecycle
    //=========================================================================
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool IsInitialized() const = 0;

    //=========================================================================
    // Write Operations
    //=========================================================================
    virtual bool WriteByte(uint8_t deviceAddr, uint8_t reg, uint8_t value) = 0;
    virtual bool WriteWord(uint8_t deviceAddr, uint8_t reg, uint16_t value) = 0;
    virtual bool WriteBlock(uint8_t deviceAddr, uint8_t reg,
                            const uint8_t* data, size_t length) = 0;

    //=========================================================================
    // Read Operations
    //=========================================================================
    virtual bool ReadByte(uint8_t deviceAddr, uint8_t reg, uint8_t& value) = 0;
    virtual bool ReadWord(uint8_t deviceAddr, uint8_t reg, uint16_t& value) = 0;
    virtual bool ReadBlock(uint8_t deviceAddr, uint8_t reg,
                           uint8_t* buffer, size_t length) = 0;

    //=========================================================================
    // Bus Scanning
    //=========================================================================
    virtual std::vector<uint8_t> ScanBus() = 0;
    virtual bool ProbeAddress(uint8_t deviceAddr) = 0;

    //=========================================================================
    // Error Handling
    //=========================================================================
    virtual std::string GetLastError() const = 0;
    virtual void ClearError() = 0;
};

//=============================================================================
// IPlatform - Main Platform Interface
//=============================================================================

class IPlatform {
public:
    virtual ~IPlatform() = default;

    //=========================================================================
    // Factory
    //=========================================================================

    /// Create platform-specific implementation
    static std::unique_ptr<IPlatform> Create();

    /// Get current platform type
    static PlatformType GetCurrentPlatform();

    /// Get platform name as string
    static const char* GetPlatformName();

    //=========================================================================
    // Subsystems
    //=========================================================================

    /// Get system information interface
    virtual ISystemInfo* GetSystemInfo() = 0;

    /// Get device enumerator interface
    virtual IDeviceEnumerator* GetDeviceEnumerator() = 0;

    /// Get SMBus provider interface (may return nullptr if unsupported)
    virtual ISMBusProvider* GetSMBusProvider() = 0;

    //=========================================================================
    // Capabilities
    //=========================================================================

    /// Check if SMBus is supported on this platform
    virtual bool HasSMBusSupport() const = 0;

    /// Check if HID is supported (always true)
    virtual bool HasHIDSupport() const { return true; }

    /// Check if elevated privileges are required for full functionality
    virtual bool RequiresElevation() const = 0;

    /// Check if currently running with elevated privileges
    virtual bool IsElevated() const = 0;

    //=========================================================================
    // Lifecycle
    //=========================================================================

    /// Initialize platform subsystems
    virtual bool Initialize() = 0;

    /// Shutdown and cleanup
    virtual void Shutdown() = 0;

    /// Check if initialized
    virtual bool IsInitialized() const = 0;

    //=========================================================================
    // Diagnostics
    //=========================================================================

    /// Get platform diagnostic information
    virtual std::string GetDiagnostics() const = 0;
};

//=============================================================================
// Compile-Time Platform Detection
//=============================================================================

#if defined(_WIN32) || defined(_WIN64)
    #define OCRGB_PLATFORM_WINDOWS 1
    #define OCRGB_PLATFORM_NAME "Windows"
#elif defined(__APPLE__) && defined(__MACH__)
    #define OCRGB_PLATFORM_MACOS 1
    #define OCRGB_PLATFORM_NAME "macOS"
#elif defined(__linux__)
    #define OCRGB_PLATFORM_LINUX 1
    #define OCRGB_PLATFORM_NAME "Linux"
#else
    #define OCRGB_PLATFORM_UNKNOWN 1
    #define OCRGB_PLATFORM_NAME "Unknown"
#endif

} // namespace Platform
} // namespace OCRGB
