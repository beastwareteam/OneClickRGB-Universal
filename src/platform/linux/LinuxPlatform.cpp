//=============================================================================
// OneClickRGB-Universal - Linux Platform Implementation
//=============================================================================

#ifdef OCRGB_PLATFORM_LINUX

#include "LinuxPlatform.h"
#include "SysfsSystemInfo.h"
#include "LinuxDeviceEnumerator.h"
#include "I2CDevSMBus.h"

#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <fstream>
#include <sstream>

namespace OCRGB {
namespace Platform {

//=============================================================================
// Constructor / Destructor
//=============================================================================

LinuxPlatform::LinuxPlatform() = default;

LinuxPlatform::~LinuxPlatform() {
    Shutdown();
}

//=============================================================================
// Lifecycle
//=============================================================================

bool LinuxPlatform::Initialize() {
    if (m_initialized) {
        return true;
    }

    // Create subsystems
    m_systemInfo = std::make_unique<SysfsSystemInfo>();
    m_deviceEnumerator = std::make_unique<LinuxDeviceEnumerator>();
    m_smbusProvider = std::make_unique<I2CDevSMBus>();

    // Initialize subsystems
    m_systemInfo->Initialize();
    m_deviceEnumerator->Initialize();

    // SMBus is optional
    if (m_smbusProvider->IsAvailable()) {
        m_smbusProvider->Initialize();
    }

    m_initialized = true;
    return true;
}

void LinuxPlatform::Shutdown() {
    if (!m_initialized) {
        return;
    }

    if (m_smbusProvider) {
        m_smbusProvider->Shutdown();
        m_smbusProvider.reset();
    }

    m_deviceEnumerator.reset();
    m_systemInfo.reset();

    m_initialized = false;
}

bool LinuxPlatform::IsInitialized() const {
    return m_initialized;
}

//=============================================================================
// Subsystems
//=============================================================================

ISystemInfo* LinuxPlatform::GetSystemInfo() {
    return m_systemInfo.get();
}

IDeviceEnumerator* LinuxPlatform::GetDeviceEnumerator() {
    return m_deviceEnumerator.get();
}

ISMBusProvider* LinuxPlatform::GetSMBusProvider() {
    return m_smbusProvider.get();
}

//=============================================================================
// Capabilities
//=============================================================================

bool LinuxPlatform::HasSMBusSupport() const {
    return m_smbusProvider && m_smbusProvider->IsAvailable();
}

bool LinuxPlatform::RequiresElevation() const {
    // SMBus requires root or i2c group membership
    return !CheckI2CGroup();
}

bool LinuxPlatform::IsElevated() const {
    return CheckRoot() || CheckI2CGroup();
}

bool LinuxPlatform::CheckRoot() const {
    return geteuid() == 0;
}

bool LinuxPlatform::CheckI2CGroup() const {
    // Check if user is in i2c group
    gid_t groups[64];
    int ngroups = 64;

    if (getgroups(ngroups, groups) == -1) {
        return false;
    }

    struct group* grp = getgrnam("i2c");
    if (!grp) {
        return false;
    }

    for (int i = 0; i < ngroups; i++) {
        if (groups[i] == grp->gr_gid) {
            return true;
        }
    }

    return false;
}

//=============================================================================
// Diagnostics
//=============================================================================

std::string LinuxPlatform::GetDiagnostics() const {
    std::ostringstream ss;

    ss << "Platform: Linux\n";
    ss << "Initialized: " << (m_initialized ? "yes" : "no") << "\n";
    ss << "Running as root: " << (CheckRoot() ? "yes" : "no") << "\n";
    ss << "In i2c group: " << (CheckI2CGroup() ? "yes" : "no") << "\n";

    if (m_smbusProvider) {
        ss << "SMBus Available: " << (m_smbusProvider->IsAvailable() ? "yes" : "no") << "\n";
        ss << "SMBus Driver: " << m_smbusProvider->GetDriverInfo() << "\n";
    }

    // Kernel version
    std::ifstream version("/proc/version");
    if (version.is_open()) {
        std::string line;
        std::getline(version, line);
        ss << "Kernel: " << line.substr(0, 60) << "...\n";
    }

    return ss.str();
}

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_LINUX
