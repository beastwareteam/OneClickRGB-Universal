//=============================================================================
// OneClickRGB-Universal - macOS Platform Implementation
//=============================================================================

#ifdef OCRGB_PLATFORM_MACOS

#include "MacOSPlatform.h"
#include "IOKitSystemInfo.h"
#include "MacOSDeviceEnumerator.h"

#include <unistd.h>
#include <sys/sysctl.h>
#include <sstream>

namespace OCRGB {
namespace Platform {

//=============================================================================
// Constructor / Destructor
//=============================================================================

MacOSPlatform::MacOSPlatform() = default;

MacOSPlatform::~MacOSPlatform() {
    Shutdown();
}

//=============================================================================
// Lifecycle
//=============================================================================

bool MacOSPlatform::Initialize() {
    if (m_initialized) {
        return true;
    }

    // Create subsystems
    m_systemInfo = std::make_unique<IOKitSystemInfo>();
    m_deviceEnumerator = std::make_unique<MacOSDeviceEnumerator>();

    // Initialize subsystems
    m_systemInfo->Initialize();
    m_deviceEnumerator->Initialize();

    m_initialized = true;
    return true;
}

void MacOSPlatform::Shutdown() {
    if (!m_initialized) {
        return;
    }

    m_deviceEnumerator.reset();
    m_systemInfo.reset();

    m_initialized = false;
}

bool MacOSPlatform::IsInitialized() const {
    return m_initialized;
}

//=============================================================================
// Subsystems
//=============================================================================

ISystemInfo* MacOSPlatform::GetSystemInfo() {
    return m_systemInfo.get();
}

IDeviceEnumerator* MacOSPlatform::GetDeviceEnumerator() {
    return m_deviceEnumerator.get();
}

ISMBusProvider* MacOSPlatform::GetSMBusProvider() {
    // SMBus is not supported on macOS
    return nullptr;
}

//=============================================================================
// Capabilities
//=============================================================================

bool MacOSPlatform::HasSMBusSupport() const {
    // macOS does not provide direct SMBus access
    return false;
}

bool MacOSPlatform::RequiresElevation() const {
    // HID access doesn't require root on macOS
    return false;
}

bool MacOSPlatform::IsElevated() const {
    return CheckRoot();
}

bool MacOSPlatform::CheckRoot() const {
    return geteuid() == 0;
}

//=============================================================================
// Diagnostics
//=============================================================================

std::string MacOSPlatform::GetDiagnostics() const {
    std::ostringstream ss;

    ss << "Platform: macOS\n";
    ss << "Initialized: " << (m_initialized ? "yes" : "no") << "\n";
    ss << "Running as root: " << (CheckRoot() ? "yes" : "no") << "\n";
    ss << "SMBus Support: no (not available on macOS)\n";
    ss << "HID Support: yes\n";

    // macOS version
    char version[64];
    size_t size = sizeof(version);
    if (sysctlbyname("kern.osproductversion", version, &size, nullptr, 0) == 0) {
        ss << "macOS Version: " << version << "\n";
    }

    return ss.str();
}

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_MACOS
