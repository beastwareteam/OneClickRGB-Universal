#pragma once
//=============================================================================
// OneClickRGB-Universal - Linux Platform Implementation
//=============================================================================

#ifdef OCRGB_PLATFORM_LINUX

#include "../IPlatform.h"
#include <memory>

namespace OCRGB {
namespace Platform {

// Forward declarations
class SysfsSystemInfo;
class LinuxDeviceEnumerator;
class I2CDevSMBus;

//=============================================================================
// Linux Platform
//=============================================================================

class LinuxPlatform : public IPlatform {
public:
    LinuxPlatform();
    ~LinuxPlatform() override;

    // Prevent copying
    LinuxPlatform(const LinuxPlatform&) = delete;
    LinuxPlatform& operator=(const LinuxPlatform&) = delete;

    //=========================================================================
    // Subsystems
    //=========================================================================
    ISystemInfo* GetSystemInfo() override;
    IDeviceEnumerator* GetDeviceEnumerator() override;
    ISMBusProvider* GetSMBusProvider() override;

    //=========================================================================
    // Capabilities
    //=========================================================================
    bool HasSMBusSupport() const override;
    bool RequiresElevation() const override;
    bool IsElevated() const override;

    //=========================================================================
    // Lifecycle
    //=========================================================================
    bool Initialize() override;
    void Shutdown() override;
    bool IsInitialized() const override;

    //=========================================================================
    // Diagnostics
    //=========================================================================
    std::string GetDiagnostics() const override;

private:
    bool m_initialized = false;

    std::unique_ptr<SysfsSystemInfo> m_systemInfo;
    std::unique_ptr<LinuxDeviceEnumerator> m_deviceEnumerator;
    std::unique_ptr<I2CDevSMBus> m_smbusProvider;

    bool CheckRoot() const;
    bool CheckI2CGroup() const;
};

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_LINUX
