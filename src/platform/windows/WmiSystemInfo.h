#pragma once
//=============================================================================
// OneClickRGB-Universal - WMI System Information (Windows)
//=============================================================================

#ifdef OCRGB_PLATFORM_WINDOWS

#include "../IPlatform.h"
#include <Windows.h>
#include <Wbemidl.h>
#include <string>

namespace OCRGB {
namespace Platform {

class WmiSystemInfo : public ISystemInfo {
public:
    WmiSystemInfo();
    ~WmiSystemInfo() override;

    // Lifecycle
    bool Initialize();
    void Shutdown();

    //=========================================================================
    // Mainboard
    //=========================================================================
    std::string GetMainboardManufacturer() override;
    std::string GetMainboardProduct() override;
    std::string GetMainboardSerial() override;
    std::string GetMainboardVersion() override;

    //=========================================================================
    // BIOS
    //=========================================================================
    std::string GetBiosVendor() override;
    std::string GetBiosVersion() override;
    std::string GetBiosDate() override;

    //=========================================================================
    // CPU
    //=========================================================================
    std::string GetCpuName() override;
    std::string GetCpuVendor() override;
    int GetCpuCores() override;
    int GetCpuThreads() override;
    uint32_t GetCpuFrequencyMHz() override;

    //=========================================================================
    // GPU
    //=========================================================================
    std::vector<GpuInfo> GetGpus() override;

    //=========================================================================
    // RAM
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
    IWbemLocator* m_pLocator = nullptr;
    IWbemServices* m_pServices = nullptr;
    bool m_initialized = false;

    // Cached values
    mutable std::string m_cachedMainboardManufacturer;
    mutable std::string m_cachedMainboardProduct;
    mutable std::string m_cachedCpuName;
    mutable bool m_cachePopulated = false;

    // WMI helpers
    std::string QueryWmiString(const wchar_t* query, const wchar_t* property);
    int QueryWmiInt(const wchar_t* query, const wchar_t* property);
    uint64_t QueryWmiUInt64(const wchar_t* query, const wchar_t* property);

    template<typename T, typename Callback>
    void QueryWmiMultiple(const wchar_t* query, Callback callback);

    static std::string WideToUtf8(const wchar_t* wide);
    static std::string BstrToString(BSTR bstr);
    static std::string TrimWhitespace(const std::string& str);
};

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_WINDOWS
