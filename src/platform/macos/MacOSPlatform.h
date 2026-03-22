#pragma once
//=============================================================================
// OneClickRGB-Universal - macOS Platform Implementation
//=============================================================================
// Note: SMBus access is limited on macOS. Only HID devices are fully supported.
//=============================================================================

#ifdef OCRGB_PLATFORM_MACOS

#include "../IPlatform.h"
#include <memory>

namespace OCRGB {
namespace Platform {

// Forward declarations
class IOKitSystemInfo;
class MacOSDeviceEnumerator;

//=============================================================================
// macOS Platform
//=============================================================================

class MacOSPlatform : public IPlatform {
public:
    MacOSPlatform();
    ~MacOSPlatform() override;

    // Prevent copying
    MacOSPlatform(const MacOSPlatform&) = delete;
    MacOSPlatform& operator=(const MacOSPlatform&) = delete;

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

    std::unique_ptr<IOKitSystemInfo> m_systemInfo;
    std::unique_ptr<MacOSDeviceEnumerator> m_deviceEnumerator;
    // Note: No SMBus provider for macOS

    bool CheckRoot() const;
};

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_MACOS
