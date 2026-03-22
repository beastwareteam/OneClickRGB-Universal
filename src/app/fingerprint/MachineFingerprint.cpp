//=============================================================================
// OneClickRGB-Universal - Machine Fingerprint Implementation
//=============================================================================

#include "MachineFingerprint.h"
#include "../../core/DryRunMode.h"

#ifdef _WIN32
#include <Windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <SetupAPI.h>
#include <devguid.h>
#include <initguid.h>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "setupapi.lib")
#endif

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <functional>

namespace OCRGB {
namespace App {

//=============================================================================
// WMI Helper (Windows only)
//=============================================================================

#ifdef _WIN32

class WmiHelper {
public:
    WmiHelper() {
        HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
        m_comInitialized = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;

        if (!m_comInitialized) return;

        hr = CoInitializeSecurity(
            NULL, -1, NULL, NULL,
            RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL, EOAC_NONE, NULL);

        hr = CoCreateInstance(
            CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID*)&m_pLoc);

        if (FAILED(hr)) return;

        hr = m_pLoc->ConnectServer(
            _bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &m_pSvc);

        if (FAILED(hr)) {
            m_pLoc->Release();
            m_pLoc = nullptr;
            return;
        }

        CoSetProxyBlanket(m_pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
            RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

        m_initialized = true;
    }

    ~WmiHelper() {
        if (m_pSvc) m_pSvc->Release();
        if (m_pLoc) m_pLoc->Release();
        if (m_comInitialized) CoUninitialize();
    }

    bool IsInitialized() const { return m_initialized; }

    std::string QueryString(const wchar_t* wqlQuery, const wchar_t* property) {
        if (!m_initialized) return "";

        IEnumWbemClassObject* pEnumerator = NULL;
        HRESULT hr = m_pSvc->ExecQuery(
            bstr_t("WQL"), bstr_t(wqlQuery),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL, &pEnumerator);

        if (FAILED(hr)) return "";

        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        std::string result;

        if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
            VARIANT vtProp;
            hr = pclsObj->Get(property, 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
                _bstr_t bstr(vtProp.bstrVal);
                result = (const char*)bstr;
            }
            VariantClear(&vtProp);
            pclsObj->Release();
        }

        pEnumerator->Release();
        return result;
    }

    int QueryInt(const wchar_t* wqlQuery, const wchar_t* property) {
        if (!m_initialized) return 0;

        IEnumWbemClassObject* pEnumerator = NULL;
        HRESULT hr = m_pSvc->ExecQuery(
            bstr_t("WQL"), bstr_t(wqlQuery),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL, &pEnumerator);

        if (FAILED(hr)) return 0;

        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        int result = 0;

        if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
            VARIANT vtProp;
            hr = pclsObj->Get(property, 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr) && vtProp.vt == VT_I4) {
                result = vtProp.intVal;
            }
            VariantClear(&vtProp);
            pclsObj->Release();
        }

        pEnumerator->Release();
        return result;
    }

    template<typename Callback>
    void QueryAll(const wchar_t* wqlQuery, Callback cb) {
        if (!m_initialized) return;

        IEnumWbemClassObject* pEnumerator = NULL;
        HRESULT hr = m_pSvc->ExecQuery(
            bstr_t("WQL"), bstr_t(wqlQuery),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL, &pEnumerator);

        if (FAILED(hr)) return;

        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;

        while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
            cb(pclsObj);
            pclsObj->Release();
        }

        pEnumerator->Release();
    }

    static std::string GetProperty(IWbemClassObject* obj, const wchar_t* prop) {
        VARIANT vtProp;
        HRESULT hr = obj->Get(prop, 0, &vtProp, 0, 0);
        std::string result;
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
            _bstr_t bstr(vtProp.bstrVal);
            result = (const char*)bstr;
        }
        VariantClear(&vtProp);
        return result;
    }

    static uint64_t GetPropertyUInt64(IWbemClassObject* obj, const wchar_t* prop) {
        VARIANT vtProp;
        HRESULT hr = obj->Get(prop, 0, &vtProp, 0, 0);
        uint64_t result = 0;
        if (SUCCEEDED(hr)) {
            if (vtProp.vt == VT_BSTR) {
                _bstr_t bstr(vtProp.bstrVal);
                result = _strtoui64((const char*)bstr, nullptr, 10);
            } else if (vtProp.vt == VT_I4) {
                result = vtProp.intVal;
            } else if (vtProp.vt == VT_UI4) {
                result = vtProp.uintVal;
            }
        }
        VariantClear(&vtProp);
        return result;
    }

private:
    bool m_comInitialized = false;
    bool m_initialized = false;
    IWbemLocator* m_pLoc = nullptr;
    IWbemServices* m_pSvc = nullptr;
};

#endif // _WIN32

//=============================================================================
// MachineFingerprint Implementation
//=============================================================================

MachineFingerprint MachineFingerprint::Collect() {
    MachineFingerprint fp;

    // In dry-run mode, return simulated fingerprint
    if (DryRun::IsEnabled()) {
        DryRun::Log("MachineFingerprint", "Collect", "simulated");

        fp.m_mainboard.manufacturer = "ASUS";
        fp.m_mainboard.product = "ROG STRIX Z790-E GAMING WIFI";
        fp.m_mainboard.serialNumber = "SIMULATED-12345";
        fp.m_mainboard.biosVendor = "American Megatrends Inc.";
        fp.m_mainboard.biosVersion = "1.0.0";
        fp.m_mainboard.biosDate = "2024-01-15";

        fp.m_cpu.name = "Intel Core i9-14900K";
        fp.m_cpu.vendor = "GenuineIntel";
        fp.m_cpu.cores = 24;
        fp.m_cpu.threads = 32;

        GpuInfo gpu;
        gpu.name = "NVIDIA GeForce RTX 4090";
        gpu.vendor = "NVIDIA";
        gpu.driverVersion = "551.23";
        fp.m_gpus.push_back(gpu);

        RamInfo ram1, ram2;
        ram1.manufacturer = "G.Skill";
        ram1.partNumber = "F5-6000J3238G32G";
        ram1.capacityBytes = 34359738368ULL; // 32GB
        ram1.speedMHz = 6000;
        ram1.slot = "DIMM_A1";
        fp.m_ram.push_back(ram1);

        ram2 = ram1;
        ram2.slot = "DIMM_A2";
        fp.m_ram.push_back(ram2);

        RgbDeviceInfo rgb1;
        rgb1.name = "ASUS Aura LED Controller";
        rgb1.type = "HID";
        rgb1.vendor = "ASUS";
        rgb1.vendorId = 0x0B05;
        rgb1.productId = 0x19AF;
        fp.m_rgbDevices.push_back(rgb1);

        RgbDeviceInfo rgb2;
        rgb2.name = "G.Skill Trident Z5 RGB";
        rgb2.type = "SMBus";
        rgb2.vendor = "G.Skill";
        rgb2.smbusAddress = 0x58;
        fp.m_rgbDevices.push_back(rgb2);

        return fp;
    }

#ifdef _WIN32
    CollectMainboard(fp);
    CollectCpu(fp);
    CollectGpu(fp);
    CollectRam(fp);
    CollectUsbDevices(fp);
    CollectRgbDevices(fp);
#endif

    return fp;
}

#ifdef _WIN32

void MachineFingerprint::CollectMainboard(MachineFingerprint& fp) {
    WmiHelper wmi;
    if (!wmi.IsInitialized()) return;

    fp.m_mainboard.manufacturer = wmi.QueryString(
        L"SELECT Manufacturer FROM Win32_BaseBoard", L"Manufacturer");
    fp.m_mainboard.product = wmi.QueryString(
        L"SELECT Product FROM Win32_BaseBoard", L"Product");
    fp.m_mainboard.serialNumber = wmi.QueryString(
        L"SELECT SerialNumber FROM Win32_BaseBoard", L"SerialNumber");

    fp.m_mainboard.biosVendor = wmi.QueryString(
        L"SELECT Manufacturer FROM Win32_BIOS", L"Manufacturer");
    fp.m_mainboard.biosVersion = wmi.QueryString(
        L"SELECT SMBIOSBIOSVersion FROM Win32_BIOS", L"SMBIOSBIOSVersion");
    fp.m_mainboard.biosDate = wmi.QueryString(
        L"SELECT ReleaseDate FROM Win32_BIOS", L"ReleaseDate");
}

void MachineFingerprint::CollectCpu(MachineFingerprint& fp) {
    WmiHelper wmi;
    if (!wmi.IsInitialized()) return;

    fp.m_cpu.name = wmi.QueryString(
        L"SELECT Name FROM Win32_Processor", L"Name");
    fp.m_cpu.vendor = wmi.QueryString(
        L"SELECT Manufacturer FROM Win32_Processor", L"Manufacturer");
    fp.m_cpu.cores = wmi.QueryInt(
        L"SELECT NumberOfCores FROM Win32_Processor", L"NumberOfCores");
    fp.m_cpu.threads = wmi.QueryInt(
        L"SELECT NumberOfLogicalProcessors FROM Win32_Processor", L"NumberOfLogicalProcessors");
}

void MachineFingerprint::CollectGpu(MachineFingerprint& fp) {
    WmiHelper wmi;
    if (!wmi.IsInitialized()) return;

    wmi.QueryAll(L"SELECT Name, AdapterCompatibility, DriverVersion FROM Win32_VideoController",
        [&fp](IWbemClassObject* obj) {
            GpuInfo gpu;
            gpu.name = WmiHelper::GetProperty(obj, L"Name");
            gpu.vendor = WmiHelper::GetProperty(obj, L"AdapterCompatibility");
            gpu.driverVersion = WmiHelper::GetProperty(obj, L"DriverVersion");

            if (!gpu.name.empty()) {
                fp.m_gpus.push_back(gpu);
            }
        });
}

void MachineFingerprint::CollectRam(MachineFingerprint& fp) {
    WmiHelper wmi;
    if (!wmi.IsInitialized()) return;

    wmi.QueryAll(L"SELECT Manufacturer, PartNumber, Capacity, Speed, DeviceLocator FROM Win32_PhysicalMemory",
        [&fp](IWbemClassObject* obj) {
            RamInfo ram;
            ram.manufacturer = WmiHelper::GetProperty(obj, L"Manufacturer");
            ram.partNumber = WmiHelper::GetProperty(obj, L"PartNumber");
            ram.capacityBytes = WmiHelper::GetPropertyUInt64(obj, L"Capacity");
            ram.speedMHz = static_cast<uint32_t>(WmiHelper::GetPropertyUInt64(obj, L"Speed"));
            ram.slot = WmiHelper::GetProperty(obj, L"DeviceLocator");

            // Trim whitespace
            while (!ram.manufacturer.empty() && ram.manufacturer.back() == ' ')
                ram.manufacturer.pop_back();
            while (!ram.partNumber.empty() && ram.partNumber.back() == ' ')
                ram.partNumber.pop_back();

            fp.m_ram.push_back(ram);
        });
}

void MachineFingerprint::CollectUsbDevices(MachineFingerprint& fp) {
    // Use SetupAPI to enumerate USB devices
    HDEVINFO hDevInfo = SetupDiGetClassDevs(
        NULL, L"USB", NULL,
        DIGCF_PRESENT | DIGCF_ALLCLASSES);

    if (hDevInfo == INVALID_HANDLE_VALUE) return;

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) {
        wchar_t buffer[512];
        UsbDeviceInfo usb;

        // Get device description
        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData,
            SPDRP_DEVICEDESC, NULL, (PBYTE)buffer, sizeof(buffer), NULL)) {
            char desc[512];
            WideCharToMultiByte(CP_UTF8, 0, buffer, -1, desc, sizeof(desc), NULL, NULL);
            usb.description = desc;
        }

        // Get hardware ID (contains VID/PID)
        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData,
            SPDRP_HARDWAREID, NULL, (PBYTE)buffer, sizeof(buffer), NULL)) {
            char hwid[512];
            WideCharToMultiByte(CP_UTF8, 0, buffer, -1, hwid, sizeof(hwid), NULL, NULL);
            usb.path = hwid;

            // Parse VID/PID from hardware ID
            std::string hwidStr = hwid;
            size_t vidPos = hwidStr.find("VID_");
            size_t pidPos = hwidStr.find("PID_");

            if (vidPos != std::string::npos) {
                usb.vendorId = static_cast<uint16_t>(
                    std::strtoul(hwidStr.substr(vidPos + 4, 4).c_str(), nullptr, 16));
            }
            if (pidPos != std::string::npos) {
                usb.productId = static_cast<uint16_t>(
                    std::strtoul(hwidStr.substr(pidPos + 4, 4).c_str(), nullptr, 16));
            }
        }

        if (usb.vendorId != 0) {
            fp.m_usbDevices.push_back(usb);
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
}

void MachineFingerprint::CollectRgbDevices(MachineFingerprint& fp) {
    // RGB devices are collected from existing device registry
    // This would integrate with the actual device scanner
    // For now, we extract from USB devices with known RGB VIDs

    const std::vector<std::pair<uint16_t, std::string>> knownRgbVendors = {
        {0x0B05, "ASUS"},
        {0x1038, "SteelSeries"},
        {0x0951, "Kingston/HyperX"},
        {0x1532, "Razer"},
        {0x046D, "Logitech"},
        {0x2516, "Cooler Master"},
        {0x1E71, "NZXT"},
        {0x048D, "EVision/Redragon"},
    };

    for (const auto& usb : fp.m_usbDevices) {
        for (const auto& vendor : knownRgbVendors) {
            if (usb.vendorId == vendor.first) {
                RgbDeviceInfo rgb;
                rgb.name = usb.description;
                rgb.type = "HID";
                rgb.vendor = vendor.second;
                rgb.vendorId = usb.vendorId;
                rgb.productId = usb.productId;
                fp.m_rgbDevices.push_back(rgb);
                break;
            }
        }
    }
}

#else // Non-Windows stubs

void MachineFingerprint::CollectMainboard(MachineFingerprint&) {}
void MachineFingerprint::CollectCpu(MachineFingerprint&) {}
void MachineFingerprint::CollectGpu(MachineFingerprint&) {}
void MachineFingerprint::CollectRam(MachineFingerprint&) {}
void MachineFingerprint::CollectUsbDevices(MachineFingerprint&) {}
void MachineFingerprint::CollectRgbDevices(MachineFingerprint&) {}

#endif // _WIN32

//=============================================================================
// Hash and ID
//=============================================================================

std::string MachineFingerprint::Sha256(const std::string& input) {
    // Simple hash implementation (FNV-1a 64-bit for now)
    // In production, use proper SHA-256 from Windows CNG or OpenSSL
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (char c : input) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 0x100000001b3ULL;
    }

    std::ostringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

std::string MachineFingerprint::GetHash() const {
    std::ostringstream ss;

    // Include stable hardware identifiers
    ss << m_mainboard.manufacturer << "|";
    ss << m_mainboard.product << "|";
    ss << m_mainboard.serialNumber << "|";
    ss << m_cpu.name << "|";

    for (const auto& gpu : m_gpus) {
        ss << gpu.name << "|";
    }

    // RAM configuration (count and capacity, not serial)
    ss << m_ram.size() << "|";
    uint64_t totalRam = 0;
    for (const auto& r : m_ram) {
        totalRam += r.capacityBytes;
    }
    ss << totalRam << "|";

    // RGB device fingerprint
    for (const auto& rgb : m_rgbDevices) {
        ss << rgb.vendorId << ":" << rgb.productId << "|";
    }

    return Sha256(ss.str());
}

std::string MachineFingerprint::GetMachineId() const {
    // Machine ID based on mainboard serial (most stable identifier)
    if (!m_mainboard.serialNumber.empty()) {
        return Sha256("OCRGB-" + m_mainboard.serialNumber);
    }

    // Fallback to product + BIOS
    return Sha256("OCRGB-" + m_mainboard.product + "-" + m_mainboard.biosVersion);
}

//=============================================================================
// JSON Serialization
//=============================================================================

std::string MachineFingerprint::ToJson() const {
    std::ostringstream ss;
    ss << "{\n";

    // Mainboard
    ss << "  \"mainboard\": {\n";
    ss << "    \"manufacturer\": \"" << m_mainboard.manufacturer << "\",\n";
    ss << "    \"product\": \"" << m_mainboard.product << "\",\n";
    ss << "    \"serialNumber\": \"" << m_mainboard.serialNumber << "\",\n";
    ss << "    \"biosVendor\": \"" << m_mainboard.biosVendor << "\",\n";
    ss << "    \"biosVersion\": \"" << m_mainboard.biosVersion << "\",\n";
    ss << "    \"biosDate\": \"" << m_mainboard.biosDate << "\"\n";
    ss << "  },\n";

    // CPU
    ss << "  \"cpu\": {\n";
    ss << "    \"name\": \"" << m_cpu.name << "\",\n";
    ss << "    \"vendor\": \"" << m_cpu.vendor << "\",\n";
    ss << "    \"cores\": " << m_cpu.cores << ",\n";
    ss << "    \"threads\": " << m_cpu.threads << "\n";
    ss << "  },\n";

    // GPUs
    ss << "  \"gpus\": [";
    for (size_t i = 0; i < m_gpus.size(); i++) {
        if (i > 0) ss << ",";
        ss << "\n    {\n";
        ss << "      \"name\": \"" << m_gpus[i].name << "\",\n";
        ss << "      \"vendor\": \"" << m_gpus[i].vendor << "\",\n";
        ss << "      \"driverVersion\": \"" << m_gpus[i].driverVersion << "\"\n";
        ss << "    }";
    }
    ss << "\n  ],\n";

    // RAM
    ss << "  \"ram\": [";
    for (size_t i = 0; i < m_ram.size(); i++) {
        if (i > 0) ss << ",";
        ss << "\n    {\n";
        ss << "      \"manufacturer\": \"" << m_ram[i].manufacturer << "\",\n";
        ss << "      \"partNumber\": \"" << m_ram[i].partNumber << "\",\n";
        ss << "      \"capacityBytes\": " << m_ram[i].capacityBytes << ",\n";
        ss << "      \"speedMHz\": " << m_ram[i].speedMHz << ",\n";
        ss << "      \"slot\": \"" << m_ram[i].slot << "\"\n";
        ss << "    }";
    }
    ss << "\n  ],\n";

    // RGB Devices
    ss << "  \"rgbDevices\": [";
    for (size_t i = 0; i < m_rgbDevices.size(); i++) {
        if (i > 0) ss << ",";
        ss << "\n    {\n";
        ss << "      \"name\": \"" << m_rgbDevices[i].name << "\",\n";
        ss << "      \"type\": \"" << m_rgbDevices[i].type << "\",\n";
        ss << "      \"vendor\": \"" << m_rgbDevices[i].vendor << "\",\n";
        ss << "      \"vendorId\": " << m_rgbDevices[i].vendorId << ",\n";
        ss << "      \"productId\": " << m_rgbDevices[i].productId << ",\n";
        ss << "      \"smbusAddress\": " << (int)m_rgbDevices[i].smbusAddress << "\n";
        ss << "    }";
    }
    ss << "\n  ],\n";

    // Hash and ID
    ss << "  \"hash\": \"" << GetHash() << "\",\n";
    ss << "  \"machineId\": \"" << GetMachineId() << "\"\n";

    ss << "}\n";
    return ss.str();
}

MachineFingerprint MachineFingerprint::FromJson(const std::string& /* json */) {
    // TODO: Implement JSON parsing
    // For now, return empty fingerprint
    return MachineFingerprint();
}

//=============================================================================
// Comparison
//=============================================================================

bool MachineFingerprint::Matches(const MachineFingerprint& other) const {
    // Match based on machine ID (mainboard serial)
    return GetMachineId() == other.GetMachineId();
}

std::vector<std::string> MachineFingerprint::GetDrift(const MachineFingerprint& previous) const {
    std::vector<std::string> drift;

    // Check mainboard changes
    if (m_mainboard.product != previous.m_mainboard.product) {
        drift.push_back("Mainboard changed: " + previous.m_mainboard.product +
                        " -> " + m_mainboard.product);
    }

    // Check CPU changes
    if (m_cpu.name != previous.m_cpu.name) {
        drift.push_back("CPU changed: " + previous.m_cpu.name + " -> " + m_cpu.name);
    }

    // Check GPU count
    if (m_gpus.size() != previous.m_gpus.size()) {
        drift.push_back("GPU count changed: " + std::to_string(previous.m_gpus.size()) +
                        " -> " + std::to_string(m_gpus.size()));
    }

    // Check RAM count
    if (m_ram.size() != previous.m_ram.size()) {
        drift.push_back("RAM module count changed: " + std::to_string(previous.m_ram.size()) +
                        " -> " + std::to_string(m_ram.size()));
    }

    // Check RGB device count
    if (m_rgbDevices.size() != previous.m_rgbDevices.size()) {
        drift.push_back("RGB device count changed: " + std::to_string(previous.m_rgbDevices.size()) +
                        " -> " + std::to_string(m_rgbDevices.size()));
    }

    return drift;
}

} // namespace App
} // namespace OCRGB
