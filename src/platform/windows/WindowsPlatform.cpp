//=============================================================================
// OneClickRGB-Universal - Windows Platform Implementation
//=============================================================================

#ifdef OCRGB_PLATFORM_WINDOWS

#include "WindowsPlatform.h"
#include "WmiSystemInfo.h"
#include "WindowsDeviceEnumerator.h"
#include "PawnIOSMBus.h"

#include <Windows.h>
#include <objbase.h>
#include <sstream>

namespace OCRGB {
namespace Platform {

//=============================================================================
// Constructor / Destructor
//=============================================================================

WindowsPlatform::WindowsPlatform() = default;

WindowsPlatform::~WindowsPlatform() {
    Shutdown();
}

//=============================================================================
// Lifecycle
//=============================================================================

bool WindowsPlatform::Initialize() {
    if (m_initialized) {
        return true;
    }

    // Initialize COM (required for WMI)
    if (!InitializeCOM()) {
        return false;
    }

    // Create subsystems
    m_systemInfo = std::make_unique<WmiSystemInfo>();
    m_deviceEnumerator = std::make_unique<WindowsDeviceEnumerator>();
    m_smbusProvider = std::make_unique<PawnIOSMBus>();

    // Initialize subsystems
    if (!m_systemInfo->Initialize()) {
        // Non-fatal: continue without system info
    }

    if (!m_deviceEnumerator->Initialize()) {
        // Non-fatal: continue without device enumeration
    }

    // SMBus is optional - don't fail if unavailable
    if (m_smbusProvider->IsAvailable()) {
        m_smbusProvider->Initialize();
    }

    m_initialized = true;
    return true;
}

void WindowsPlatform::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // Shutdown subsystems in reverse order
    if (m_smbusProvider) {
        m_smbusProvider->Shutdown();
        m_smbusProvider.reset();
    }

    if (m_deviceEnumerator) {
        m_deviceEnumerator.reset();
    }

    if (m_systemInfo) {
        m_systemInfo->Shutdown();
        m_systemInfo.reset();
    }

    ShutdownCOM();
    m_initialized = false;
}

bool WindowsPlatform::IsInitialized() const {
    return m_initialized;
}

//=============================================================================
// Subsystems
//=============================================================================

ISystemInfo* WindowsPlatform::GetSystemInfo() {
    return m_systemInfo.get();
}

IDeviceEnumerator* WindowsPlatform::GetDeviceEnumerator() {
    return m_deviceEnumerator.get();
}

ISMBusProvider* WindowsPlatform::GetSMBusProvider() {
    return m_smbusProvider.get();
}

//=============================================================================
// Capabilities
//=============================================================================

bool WindowsPlatform::HasSMBusSupport() const {
    return m_smbusProvider && m_smbusProvider->IsAvailable();
}

bool WindowsPlatform::RequiresElevation() const {
    // SMBus access requires admin on Windows
    return true;
}

bool WindowsPlatform::IsElevated() const {
    return CheckElevation();
}

//=============================================================================
// COM Initialization
//=============================================================================

bool WindowsPlatform::InitializeCOM() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        return false;
    }

    m_comInitialized = true;

    // Set COM security
    hr = CoInitializeSecurity(
        nullptr,
        -1,
        nullptr,
        nullptr,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr,
        EOAC_NONE,
        nullptr
    );

    // Ignore error if already initialized
    if (FAILED(hr) && hr != RPC_E_TOO_LATE) {
        // Non-fatal, continue
    }

    return true;
}

void WindowsPlatform::ShutdownCOM() {
    if (m_comInitialized) {
        CoUninitialize();
        m_comInitialized = false;
    }
}

//=============================================================================
// Elevation Check
//=============================================================================

bool WindowsPlatform::CheckElevation() const {
    BOOL isElevated = FALSE;
    HANDLE hToken = nullptr;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);

        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isElevated = elevation.TokenIsElevated;
        }

        CloseHandle(hToken);
    }

    return isElevated != FALSE;
}

//=============================================================================
// Diagnostics
//=============================================================================

std::string WindowsPlatform::GetDiagnostics() const {
    std::ostringstream ss;

    ss << "Platform: Windows\n";
    ss << "Initialized: " << (m_initialized ? "yes" : "no") << "\n";
    ss << "Elevated: " << (IsElevated() ? "yes" : "no") << "\n";
    ss << "COM Initialized: " << (m_comInitialized ? "yes" : "no") << "\n";

    if (m_smbusProvider) {
        ss << "SMBus Available: " << (m_smbusProvider->IsAvailable() ? "yes" : "no") << "\n";
        ss << "SMBus Driver: " << m_smbusProvider->GetDriverInfo() << "\n";
    }

    // Windows version
    OSVERSIONINFOEXW osvi = { sizeof(osvi) };
    using RtlGetVersionPtr = NTSTATUS(WINAPI*)(PRTL_OSVERSIONINFOW);
    auto RtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(
        GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));

    if (RtlGetVersion && RtlGetVersion(reinterpret_cast<PRTL_OSVERSIONINFOW>(&osvi)) == 0) {
        ss << "Windows Version: " << osvi.dwMajorVersion << "."
           << osvi.dwMinorVersion << "." << osvi.dwBuildNumber << "\n";
    }

    return ss.str();
}

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_WINDOWS
