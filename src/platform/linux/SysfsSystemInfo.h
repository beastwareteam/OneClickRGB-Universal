#pragma once
//=============================================================================
// OneClickRGB-Universal - Sysfs System Information (Linux)
//=============================================================================

#ifdef OCRGB_PLATFORM_LINUX

#include "../IPlatform.h"
#include <string>

namespace OCRGB {
namespace Platform {

class SysfsSystemInfo : public ISystemInfo {
public:
    SysfsSystemInfo() = default;
    ~SysfsSystemInfo() override = default;

    bool Initialize();

    //=========================================================================
    // Mainboard (DMI)
    //=========================================================================
    std::string GetMainboardManufacturer() override;
    std::string GetMainboardProduct() override;
    std::string GetMainboardSerial() override;
    std::string GetMainboardVersion() override;

    //=========================================================================
    // BIOS (DMI)
    //=========================================================================
    std::string GetBiosVendor() override;
    std::string GetBiosVersion() override;
    std::string GetBiosDate() override;

    //=========================================================================
    // CPU (/proc/cpuinfo)
    //=========================================================================
    std::string GetCpuName() override;
    std::string GetCpuVendor() override;
    int GetCpuCores() override;
    int GetCpuThreads() override;
    uint32_t GetCpuFrequencyMHz() override;

    //=========================================================================
    // GPU (DRM/sysfs)
    //=========================================================================
    std::vector<GpuInfo> GetGpus() override;

    //=========================================================================
    // RAM (/proc/meminfo, dmidecode)
    //=========================================================================
    std::vector<RamModuleInfo> GetRamModules() override;
    uint64_t GetTotalRamBytes() override;

    //=========================================================================
    // OS
    //=========================================================================
    std::string GetOsName() override;
    std::string GetOsVersion() override;
    std::string GetKernelVersion() override;
    std::string GetArchitecture() override;
    std::string GetHostname() override;

private:
    std::string ReadDmiFile(const std::string& name);
    std::string ReadFile(const std::string& path);
    std::string TrimWhitespace(const std::string& str);
    std::string GetCpuInfoField(const std::string& field);
};

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_LINUX
