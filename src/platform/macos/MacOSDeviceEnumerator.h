#pragma once
//=============================================================================
// OneClickRGB-Universal - macOS Device Enumerator
//=============================================================================

#ifdef OCRGB_PLATFORM_MACOS

#include "../IPlatform.h"
#include <vector>
#include <string>

namespace OCRGB {
namespace Platform {

//=============================================================================
// macOS Device Enumerator (IOKit + HIDAPI)
//=============================================================================

class MacOSDeviceEnumerator : public IDeviceEnumerator {
public:
    MacOSDeviceEnumerator() = default;
    ~MacOSDeviceEnumerator() override = default;

    bool Initialize();
    void Shutdown();

    //=========================================================================
    // IDeviceEnumerator Interface
    //=========================================================================
    std::vector<UsbDeviceInfo> EnumerateUSBDevices() override;
    std::vector<HidDeviceInfo> EnumerateHIDDevices() override;
    std::vector<I2CDeviceInfo> EnumerateI2CDevices() override;

private:
    bool m_initialized = false;

    // IOKit helpers
    std::vector<UsbDeviceInfo> EnumerateIOKitUSB();
    std::vector<HidDeviceInfo> EnumerateHIDAPI();
};

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_MACOS
