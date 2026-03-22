//=============================================================================
// OneClickRGB-Universal - WMI System Information Implementation
//=============================================================================

#ifdef OCRGB_PLATFORM_WINDOWS

#include "WmiSystemInfo.h"
#include <comdef.h>
#include <algorithm>
#include <sstream>

#pragma comment(lib, "wbemuuid.lib")

namespace OCRGB {
namespace Platform {

//=============================================================================
// Constructor / Destructor
//=============================================================================

WmiSystemInfo::WmiSystemInfo() = default;

WmiSystemInfo::~WmiSystemInfo() {
    Shutdown();
}

//=============================================================================
// Lifecycle
//=============================================================================

bool WmiSystemInfo::Initialize() {
    if (m_initialized) {
        return true;
    }

    HRESULT hr;

    // Create WMI locator
    hr = CoCreateInstance(
        CLSID_WbemLocator,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        reinterpret_cast<void**>(&m_pLocator)
    );

    if (FAILED(hr)) {
        return false;
    }

    // Connect to WMI
    hr = m_pLocator->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        nullptr,
        nullptr,
        nullptr,
        0,
        nullptr,
        nullptr,
        &m_pServices
    );

    if (FAILED(hr)) {
        m_pLocator->Release();
        m_pLocator = nullptr;
        return false;
    }

    // Set security levels
    hr = CoSetProxyBlanket(
        m_pServices,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        nullptr,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr,
        EOAC_NONE
    );

    if (FAILED(hr)) {
        m_pServices->Release();
        m_pLocator->Release();
        m_pServices = nullptr;
        m_pLocator = nullptr;
        return false;
    }

    m_initialized = true;
    return true;
}

void WmiSystemInfo::Shutdown() {
    if (m_pServices) {
        m_pServices->Release();
        m_pServices = nullptr;
    }

    if (m_pLocator) {
        m_pLocator->Release();
        m_pLocator = nullptr;
    }

    m_initialized = false;
    m_cachePopulated = false;
}

//=============================================================================
// WMI Helpers
//=============================================================================

std::string WmiSystemInfo::QueryWmiString(const wchar_t* query, const wchar_t* property) {
    if (!m_initialized || !m_pServices) {
        return "";
    }

    IEnumWbemClassObject* pEnumerator = nullptr;

    HRESULT hr = m_pServices->ExecQuery(
        bstr_t(L"WQL"),
        bstr_t(query),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnumerator
    );

    if (FAILED(hr)) {
        return "";
    }

    std::string result;
    IWbemClassObject* pObject = nullptr;
    ULONG returned = 0;

    if (pEnumerator->Next(WBEM_INFINITE, 1, &pObject, &returned) == S_OK) {
        VARIANT vtProp;
        VariantInit(&vtProp);

        hr = pObject->Get(property, 0, &vtProp, nullptr, nullptr);

        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
            result = BstrToString(vtProp.bstrVal);
        }

        VariantClear(&vtProp);
        pObject->Release();
    }

    pEnumerator->Release();
    return TrimWhitespace(result);
}

int WmiSystemInfo::QueryWmiInt(const wchar_t* query, const wchar_t* property) {
    if (!m_initialized || !m_pServices) {
        return 0;
    }

    IEnumWbemClassObject* pEnumerator = nullptr;

    HRESULT hr = m_pServices->ExecQuery(
        bstr_t(L"WQL"),
        bstr_t(query),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnumerator
    );

    if (FAILED(hr)) {
        return 0;
    }

    int result = 0;
    IWbemClassObject* pObject = nullptr;
    ULONG returned = 0;

    if (pEnumerator->Next(WBEM_INFINITE, 1, &pObject, &returned) == S_OK) {
        VARIANT vtProp;
        VariantInit(&vtProp);

        hr = pObject->Get(property, 0, &vtProp, nullptr, nullptr);

        if (SUCCEEDED(hr)) {
            if (vtProp.vt == VT_I4) {
                result = vtProp.intVal;
            } else if (vtProp.vt == VT_UI4) {
                result = static_cast<int>(vtProp.uintVal);
            }
        }

        VariantClear(&vtProp);
        pObject->Release();
    }

    pEnumerator->Release();
    return result;
}

uint64_t WmiSystemInfo::QueryWmiUInt64(const wchar_t* query, const wchar_t* property) {
    if (!m_initialized || !m_pServices) {
        return 0;
    }

    IEnumWbemClassObject* pEnumerator = nullptr;

    HRESULT hr = m_pServices->ExecQuery(
        bstr_t(L"WQL"),
        bstr_t(query),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnumerator
    );

    if (FAILED(hr)) {
        return 0;
    }

    uint64_t result = 0;
    IWbemClassObject* pObject = nullptr;
    ULONG returned = 0;

    if (pEnumerator->Next(WBEM_INFINITE, 1, &pObject, &returned) == S_OK) {
        VARIANT vtProp;
        VariantInit(&vtProp);

        hr = pObject->Get(property, 0, &vtProp, nullptr, nullptr);

        if (SUCCEEDED(hr)) {
            if (vtProp.vt == VT_BSTR) {
                result = _wcstoui64(vtProp.bstrVal, nullptr, 10);
            } else if (vtProp.vt == VT_I4) {
                result = static_cast<uint64_t>(vtProp.intVal);
            } else if (vtProp.vt == VT_UI4) {
                result = vtProp.uintVal;
            }
        }

        VariantClear(&vtProp);
        pObject->Release();
    }

    pEnumerator->Release();
    return result;
}

std::string WmiSystemInfo::WideToUtf8(const wchar_t* wide) {
    if (!wide) return "";

    int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";

    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, &result[0], size, nullptr, nullptr);
    return result;
}

std::string WmiSystemInfo::BstrToString(BSTR bstr) {
    if (!bstr) return "";
    return WideToUtf8(bstr);
}

std::string WmiSystemInfo::TrimWhitespace(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

//=============================================================================
// Mainboard
//=============================================================================

std::string WmiSystemInfo::GetMainboardManufacturer() {
    return QueryWmiString(
        L"SELECT Manufacturer FROM Win32_BaseBoard",
        L"Manufacturer"
    );
}

std::string WmiSystemInfo::GetMainboardProduct() {
    return QueryWmiString(
        L"SELECT Product FROM Win32_BaseBoard",
        L"Product"
    );
}

std::string WmiSystemInfo::GetMainboardSerial() {
    return QueryWmiString(
        L"SELECT SerialNumber FROM Win32_BaseBoard",
        L"SerialNumber"
    );
}

std::string WmiSystemInfo::GetMainboardVersion() {
    return QueryWmiString(
        L"SELECT Version FROM Win32_BaseBoard",
        L"Version"
    );
}

//=============================================================================
// BIOS
//=============================================================================

std::string WmiSystemInfo::GetBiosVendor() {
    return QueryWmiString(
        L"SELECT Manufacturer FROM Win32_BIOS",
        L"Manufacturer"
    );
}

std::string WmiSystemInfo::GetBiosVersion() {
    return QueryWmiString(
        L"SELECT SMBIOSBIOSVersion FROM Win32_BIOS",
        L"SMBIOSBIOSVersion"
    );
}

std::string WmiSystemInfo::GetBiosDate() {
    return QueryWmiString(
        L"SELECT ReleaseDate FROM Win32_BIOS",
        L"ReleaseDate"
    );
}

//=============================================================================
// CPU
//=============================================================================

std::string WmiSystemInfo::GetCpuName() {
    return QueryWmiString(
        L"SELECT Name FROM Win32_Processor",
        L"Name"
    );
}

std::string WmiSystemInfo::GetCpuVendor() {
    return QueryWmiString(
        L"SELECT Manufacturer FROM Win32_Processor",
        L"Manufacturer"
    );
}

int WmiSystemInfo::GetCpuCores() {
    return QueryWmiInt(
        L"SELECT NumberOfCores FROM Win32_Processor",
        L"NumberOfCores"
    );
}

int WmiSystemInfo::GetCpuThreads() {
    return QueryWmiInt(
        L"SELECT NumberOfLogicalProcessors FROM Win32_Processor",
        L"NumberOfLogicalProcessors"
    );
}

uint32_t WmiSystemInfo::GetCpuFrequencyMHz() {
    return static_cast<uint32_t>(QueryWmiInt(
        L"SELECT MaxClockSpeed FROM Win32_Processor",
        L"MaxClockSpeed"
    ));
}

//=============================================================================
// GPU
//=============================================================================

std::vector<GpuInfo> WmiSystemInfo::GetGpus() {
    std::vector<GpuInfo> gpus;

    if (!m_initialized || !m_pServices) {
        return gpus;
    }

    IEnumWbemClassObject* pEnumerator = nullptr;

    HRESULT hr = m_pServices->ExecQuery(
        bstr_t(L"WQL"),
        bstr_t(L"SELECT Name, AdapterCompatibility, DriverVersion, AdapterRAM FROM Win32_VideoController"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnumerator
    );

    if (FAILED(hr)) {
        return gpus;
    }

    IWbemClassObject* pObject = nullptr;
    ULONG returned = 0;

    while (pEnumerator->Next(WBEM_INFINITE, 1, &pObject, &returned) == S_OK) {
        GpuInfo gpu;
        VARIANT vtProp;

        VariantInit(&vtProp);
        if (SUCCEEDED(pObject->Get(L"Name", 0, &vtProp, nullptr, nullptr)) && vtProp.vt == VT_BSTR) {
            gpu.name = BstrToString(vtProp.bstrVal);
        }
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        if (SUCCEEDED(pObject->Get(L"AdapterCompatibility", 0, &vtProp, nullptr, nullptr)) && vtProp.vt == VT_BSTR) {
            gpu.vendor = BstrToString(vtProp.bstrVal);
        }
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        if (SUCCEEDED(pObject->Get(L"DriverVersion", 0, &vtProp, nullptr, nullptr)) && vtProp.vt == VT_BSTR) {
            gpu.driverVersion = BstrToString(vtProp.bstrVal);
        }
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        if (SUCCEEDED(pObject->Get(L"AdapterRAM", 0, &vtProp, nullptr, nullptr)) && vtProp.vt == VT_UI4) {
            gpu.memoryBytes = vtProp.uintVal;
        }
        VariantClear(&vtProp);

        if (!gpu.name.empty()) {
            gpus.push_back(gpu);
        }

        pObject->Release();
    }

    pEnumerator->Release();
    return gpus;
}

//=============================================================================
// RAM
//=============================================================================

std::vector<RamModuleInfo> WmiSystemInfo::GetRamModules() {
    std::vector<RamModuleInfo> modules;

    if (!m_initialized || !m_pServices) {
        return modules;
    }

    IEnumWbemClassObject* pEnumerator = nullptr;

    HRESULT hr = m_pServices->ExecQuery(
        bstr_t(L"WQL"),
        bstr_t(L"SELECT Manufacturer, PartNumber, SerialNumber, Capacity, Speed, DeviceLocator, MemoryType FROM Win32_PhysicalMemory"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnumerator
    );

    if (FAILED(hr)) {
        return modules;
    }

    IWbemClassObject* pObject = nullptr;
    ULONG returned = 0;

    while (pEnumerator->Next(WBEM_INFINITE, 1, &pObject, &returned) == S_OK) {
        RamModuleInfo ram;
        VARIANT vtProp;

        VariantInit(&vtProp);
        if (SUCCEEDED(pObject->Get(L"Manufacturer", 0, &vtProp, nullptr, nullptr)) && vtProp.vt == VT_BSTR) {
            ram.manufacturer = TrimWhitespace(BstrToString(vtProp.bstrVal));
        }
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        if (SUCCEEDED(pObject->Get(L"PartNumber", 0, &vtProp, nullptr, nullptr)) && vtProp.vt == VT_BSTR) {
            ram.partNumber = TrimWhitespace(BstrToString(vtProp.bstrVal));
        }
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        if (SUCCEEDED(pObject->Get(L"SerialNumber", 0, &vtProp, nullptr, nullptr)) && vtProp.vt == VT_BSTR) {
            ram.serialNumber = TrimWhitespace(BstrToString(vtProp.bstrVal));
        }
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        if (SUCCEEDED(pObject->Get(L"Capacity", 0, &vtProp, nullptr, nullptr)) && vtProp.vt == VT_BSTR) {
            ram.capacityBytes = _wcstoui64(vtProp.bstrVal, nullptr, 10);
        }
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        if (SUCCEEDED(pObject->Get(L"Speed", 0, &vtProp, nullptr, nullptr)) && vtProp.vt == VT_UI4) {
            ram.speedMHz = vtProp.uintVal;
        }
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        if (SUCCEEDED(pObject->Get(L"DeviceLocator", 0, &vtProp, nullptr, nullptr)) && vtProp.vt == VT_BSTR) {
            ram.slot = BstrToString(vtProp.bstrVal);
        }
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        if (SUCCEEDED(pObject->Get(L"MemoryType", 0, &vtProp, nullptr, nullptr)) && vtProp.vt == VT_UI2) {
            // Memory type enum: 20=DDR, 21=DDR2, 22=DDR3, 26=DDR4, 34=DDR5
            switch (vtProp.uiVal) {
                case 20: ram.type = "DDR"; break;
                case 21: ram.type = "DDR2"; break;
                case 22: ram.type = "DDR3"; break;
                case 26: ram.type = "DDR4"; break;
                case 34: ram.type = "DDR5"; break;
                default: ram.type = "Unknown"; break;
            }
        }
        VariantClear(&vtProp);

        modules.push_back(ram);
        pObject->Release();
    }

    pEnumerator->Release();
    return modules;
}

uint64_t WmiSystemInfo::GetTotalRamBytes() {
    return QueryWmiUInt64(
        L"SELECT TotalPhysicalMemory FROM Win32_ComputerSystem",
        L"TotalPhysicalMemory"
    );
}

//=============================================================================
// OS
//=============================================================================

std::string WmiSystemInfo::GetOsName() {
    return QueryWmiString(
        L"SELECT Caption FROM Win32_OperatingSystem",
        L"Caption"
    );
}

std::string WmiSystemInfo::GetOsVersion() {
    return QueryWmiString(
        L"SELECT Version FROM Win32_OperatingSystem",
        L"Version"
    );
}

std::string WmiSystemInfo::GetKernelVersion() {
    return QueryWmiString(
        L"SELECT BuildNumber FROM Win32_OperatingSystem",
        L"BuildNumber"
    );
}

std::string WmiSystemInfo::GetArchitecture() {
    return QueryWmiString(
        L"SELECT OSArchitecture FROM Win32_OperatingSystem",
        L"OSArchitecture"
    );
}

std::string WmiSystemInfo::GetHostname() {
    char hostname[256];
    DWORD size = sizeof(hostname);

    if (GetComputerNameA(hostname, &size)) {
        return hostname;
    }

    return "";
}

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_WINDOWS
