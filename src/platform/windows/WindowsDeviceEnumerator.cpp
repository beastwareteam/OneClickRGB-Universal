//=============================================================================
// OneClickRGB-Universal - Windows Device Enumerator Implementation
//=============================================================================

#ifdef OCRGB_PLATFORM_WINDOWS

#include "WindowsDeviceEnumerator.h"
#include <hidapi.h>
#include <Windows.h>
#include <SetupAPI.h>
#include <devguid.h>
#include <initguid.h>

#pragma comment(lib, "setupapi.lib")

namespace OCRGB {
namespace Platform {

//=============================================================================
// WindowsHidDevice
//=============================================================================

WindowsHidDevice::WindowsHidDevice(void* handle)
    : m_handle(handle) {
}

WindowsHidDevice::~WindowsHidDevice() {
    Close();
}

bool WindowsHidDevice::IsOpen() const {
    return m_handle != nullptr;
}

void WindowsHidDevice::Close() {
    if (m_handle) {
        hid_close(static_cast<hid_device*>(m_handle));
        m_handle = nullptr;
    }
}

int WindowsHidDevice::Write(const uint8_t* data, size_t length) {
    if (!m_handle) return -1;
    return hid_write(static_cast<hid_device*>(m_handle), data, length);
}

int WindowsHidDevice::Read(uint8_t* buffer, size_t length, int timeoutMs) {
    if (!m_handle) return -1;
    return hid_read_timeout(static_cast<hid_device*>(m_handle), buffer, length, timeoutMs);
}

int WindowsHidDevice::SendFeatureReport(const uint8_t* data, size_t length) {
    if (!m_handle) return -1;
    return hid_send_feature_report(static_cast<hid_device*>(m_handle), data, length);
}

int WindowsHidDevice::GetFeatureReport(uint8_t* buffer, size_t length) {
    if (!m_handle) return -1;
    return hid_get_feature_report(static_cast<hid_device*>(m_handle), buffer, length);
}

std::string WindowsHidDevice::GetManufacturer() const {
    if (!m_handle) return "";

    wchar_t buf[256];
    if (hid_get_manufacturer_string(static_cast<hid_device*>(m_handle), buf, sizeof(buf) / sizeof(wchar_t)) == 0) {
        return WindowsDeviceEnumerator::WideToUtf8(buf);
    }
    return "";
}

std::string WindowsHidDevice::GetProduct() const {
    if (!m_handle) return "";

    wchar_t buf[256];
    if (hid_get_product_string(static_cast<hid_device*>(m_handle), buf, sizeof(buf) / sizeof(wchar_t)) == 0) {
        return WindowsDeviceEnumerator::WideToUtf8(buf);
    }
    return "";
}

std::string WindowsHidDevice::GetSerialNumber() const {
    if (!m_handle) return "";

    wchar_t buf[256];
    if (hid_get_serial_number_string(static_cast<hid_device*>(m_handle), buf, sizeof(buf) / sizeof(wchar_t)) == 0) {
        return WindowsDeviceEnumerator::WideToUtf8(buf);
    }
    return "";
}

//=============================================================================
// WindowsDeviceEnumerator
//=============================================================================

WindowsDeviceEnumerator::WindowsDeviceEnumerator() = default;

WindowsDeviceEnumerator::~WindowsDeviceEnumerator() {
    if (m_hidInitialized) {
        hid_exit();
    }
}

bool WindowsDeviceEnumerator::Initialize() {
    if (m_hidInitialized) {
        return true;
    }

    if (hid_init() == 0) {
        m_hidInitialized = true;
        return true;
    }

    return false;
}

std::string WindowsDeviceEnumerator::WideToUtf8(const wchar_t* wide) {
    if (!wide) return "";

    int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";

    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, &result[0], size, nullptr, nullptr);
    return result;
}

//=============================================================================
// USB Enumeration
//=============================================================================

std::vector<UsbDeviceInfo> WindowsDeviceEnumerator::EnumerateUSB() {
    std::vector<UsbDeviceInfo> devices;

    HDEVINFO hDevInfo = SetupDiGetClassDevs(
        nullptr,
        L"USB",
        nullptr,
        DIGCF_PRESENT | DIGCF_ALLCLASSES
    );

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        return devices;
    }

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) {
        UsbDeviceInfo usb;
        wchar_t buffer[512];

        // Get description
        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData,
            SPDRP_DEVICEDESC, nullptr, (PBYTE)buffer, sizeof(buffer), nullptr)) {
            usb.description = WideToUtf8(buffer);
        }

        // Get hardware ID (VID/PID)
        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData,
            SPDRP_HARDWAREID, nullptr, (PBYTE)buffer, sizeof(buffer), nullptr)) {
            usb.path = WideToUtf8(buffer);

            std::string hwid = usb.path;
            size_t vidPos = hwid.find("VID_");
            size_t pidPos = hwid.find("PID_");

            if (vidPos != std::string::npos) {
                usb.vendorId = static_cast<uint16_t>(
                    std::strtoul(hwid.substr(vidPos + 4, 4).c_str(), nullptr, 16));
            }
            if (pidPos != std::string::npos) {
                usb.productId = static_cast<uint16_t>(
                    std::strtoul(hwid.substr(pidPos + 4, 4).c_str(), nullptr, 16));
            }
        }

        // Get manufacturer
        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData,
            SPDRP_MFG, nullptr, (PBYTE)buffer, sizeof(buffer), nullptr)) {
            usb.manufacturer = WideToUtf8(buffer);
        }

        if (usb.vendorId != 0) {
            devices.push_back(usb);
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return devices;
}

std::vector<UsbDeviceInfo> WindowsDeviceEnumerator::EnumerateUSB(uint16_t vendorId) {
    auto all = EnumerateUSB();
    std::vector<UsbDeviceInfo> filtered;

    for (const auto& dev : all) {
        if (dev.vendorId == vendorId) {
            filtered.push_back(dev);
        }
    }

    return filtered;
}

//=============================================================================
// HID Enumeration
//=============================================================================

std::vector<HidDeviceInfo> WindowsDeviceEnumerator::EnumerateHID() {
    return EnumerateHID(0, 0);
}

std::vector<HidDeviceInfo> WindowsDeviceEnumerator::EnumerateHID(uint16_t vendorId, uint16_t productId) {
    std::vector<HidDeviceInfo> devices;

    if (!m_hidInitialized) {
        Initialize();
    }

    struct hid_device_info* devs = hid_enumerate(vendorId, productId);
    struct hid_device_info* cur = devs;

    while (cur) {
        HidDeviceInfo info;
        info.vendorId = cur->vendor_id;
        info.productId = cur->product_id;
        info.usagePage = cur->usage_page;
        info.usage = cur->usage;
        info.interfaceNumber = cur->interface_number;

        if (cur->path) {
            info.path = cur->path;
        }
        if (cur->manufacturer_string) {
            info.manufacturer = WideToUtf8(cur->manufacturer_string);
        }
        if (cur->product_string) {
            info.product = WideToUtf8(cur->product_string);
        }
        if (cur->serial_number) {
            info.serialNumber = WideToUtf8(cur->serial_number);
        }

        devices.push_back(info);
        cur = cur->next;
    }

    hid_free_enumeration(devs);
    return devices;
}

//=============================================================================
// Device Access
//=============================================================================

std::unique_ptr<IHidDevice> WindowsDeviceEnumerator::OpenHID(const std::string& path) {
    if (!m_hidInitialized) {
        Initialize();
    }

    hid_device* handle = hid_open_path(path.c_str());
    if (!handle) {
        return nullptr;
    }

    return std::make_unique<WindowsHidDevice>(handle);
}

std::unique_ptr<IHidDevice> WindowsDeviceEnumerator::OpenHID(
    uint16_t vendorId, uint16_t productId,
    uint16_t usagePage, uint16_t usage)
{
    if (!m_hidInitialized) {
        Initialize();
    }

    // If usage page/usage specified, enumerate and find matching device
    if (usagePage != 0 || usage != 0) {
        auto devices = EnumerateHID(vendorId, productId);

        for (const auto& dev : devices) {
            if ((usagePage == 0 || dev.usagePage == usagePage) &&
                (usage == 0 || dev.usage == usage)) {
                return OpenHID(dev.path);
            }
        }

        return nullptr;
    }

    // Simple open by VID/PID
    hid_device* handle = hid_open(vendorId, productId, nullptr);
    if (!handle) {
        return nullptr;
    }

    return std::make_unique<WindowsHidDevice>(handle);
}

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_WINDOWS
