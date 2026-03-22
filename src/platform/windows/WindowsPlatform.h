#pragma once
//=============================================================================
// OneClickRGB-Universal - Windows Platform Implementation
//=============================================================================

#ifdef OCRGB_PLATFORM_WINDOWS

#include "../IPlatform.h"
#include <memory>

namespace OCRGB {
namespace Platform {

// Forward declarations
class WmiSystemInfo;
class WindowsDeviceEnumerator;
class PawnIOSMBus;

//=============================================================================
// Windows Platform
//=============================================================================

class WindowsPlatform : public IPlatform {
public:
    WindowsPlatform();
    ~WindowsPlatform() override;

    // Prevent copying
    WindowsPlatform(const WindowsPlatform&) = delete;
    WindowsPlatform& operator=(const WindowsPlatform&) = delete;

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
    bool m_comInitialized = false;

    std::unique_ptr<WmiSystemInfo> m_systemInfo;
    std::unique_ptr<WindowsDeviceEnumerator> m_deviceEnumerator;
    std::unique_ptr<PawnIOSMBus> m_smbusProvider;

    bool InitializeCOM();
    void ShutdownCOM();
    bool CheckElevation() const;
};

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_WINDOWS
