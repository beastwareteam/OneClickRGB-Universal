#pragma once
//=============================================================================
// OneClickRGB-Universal - IOKit System Information (macOS)
//=============================================================================

#ifdef OCRGB_PLATFORM_MACOS

#include "../IPlatform.h"
#include <string>

namespace OCRGB {
namespace Platform {

class IOKitSystemInfo : public ISystemInfo {
public:
    IOKitSystemInfo() = default;
    ~IOKitSystemInfo() override = default;

    bool Initialize();

    //=========================================================================
    // Mainboard (IOKit)
    //=========================================================================
    std::string GetMainboardManufacturer() override;
    std::string GetMainboardProduct() override;
    std::string GetMainboardSerial() override;
    std::string GetMainboardVersion() override;

    //=========================================================================
    // BIOS (EFI)
    //=========================================================================
    std::string GetBiosVendor() override;
    std::string GetBiosVersion() override;
    std::string GetBiosDate() override;

    //=========================================================================
    // CPU (sysctl)
    //=========================================================================
    std::string GetCpuName() override;
    std::string GetCpuVendor() override;
    int GetCpuCores() override;
    int GetCpuThreads() override;
    uint32_t GetCpuFrequencyMHz() override;

    //=========================================================================
    // GPU (IOKit)
    //=========================================================================
    std::vector<GpuInfo> GetGpus() override;

    //=========================================================================
    // RAM (sysctl)
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
    std::string GetSysctlString(const char* name);
    int GetSysctlInt(const char* name);
    uint64_t GetSysctlUInt64(const char* name);
    std::string GetIORegistryString(const char* plane, const char* key);
};

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_MACOS
