//=============================================================================
// OneClickRGB-Universal - Platform Factory
//=============================================================================
// Creates the appropriate platform implementation based on compile-time
// and runtime detection.
//=============================================================================

#include "IPlatform.h"

// Include platform-specific implementations
#if defined(OCRGB_PLATFORM_WINDOWS)
    #include "windows/WindowsPlatform.h"
#elif defined(OCRGB_PLATFORM_LINUX)
    #include "linux/LinuxPlatform.h"
#elif defined(OCRGB_PLATFORM_MACOS)
    #include "macos/MacOSPlatform.h"
#endif

namespace OCRGB {
namespace Platform {

//=============================================================================
// Static Methods
//=============================================================================

PlatformType IPlatform::GetCurrentPlatform() {
#if defined(OCRGB_PLATFORM_WINDOWS)
    return PlatformType::Windows;
#elif defined(OCRGB_PLATFORM_LINUX)
    return PlatformType::Linux;
#elif defined(OCRGB_PLATFORM_MACOS)
    return PlatformType::macOS;
#else
    return PlatformType::Unknown;
#endif
}

const char* IPlatform::GetPlatformName() {
    return OCRGB_PLATFORM_NAME;
}

std::unique_ptr<IPlatform> IPlatform::Create() {
#if defined(OCRGB_PLATFORM_WINDOWS)
    return std::make_unique<WindowsPlatform>();
#elif defined(OCRGB_PLATFORM_LINUX)
    return std::make_unique<LinuxPlatform>();
#elif defined(OCRGB_PLATFORM_MACOS)
    return std::make_unique<MacOSPlatform>();
#else
    // Return nullptr for unsupported platforms
    return nullptr;
#endif
}

} // namespace Platform
} // namespace OCRGB
