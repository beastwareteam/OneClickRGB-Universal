//=============================================================================
// OneClickRGB-Universal - Linux Device Enumerator Implementation
//=============================================================================

#ifdef OCRGB_PLATFORM_LINUX

#include "LinuxDeviceEnumerator.h"
#include <hidapi.h>
#include <fstream>
#include <dirent.h>
#include <cstring>
#include <cwchar>

namespace OCRGB {
namespace Platform {

//=============================================================================
// LinuxHidDevice
//=============================================================================

LinuxHidDevice::LinuxHidDevice(void* handle)
    : m_handle(handle) {
}

LinuxHidDevice::~LinuxHidDevice() {
    Close();
}

bool LinuxHidDevice::IsOpen() const {
    return m_handle != nullptr;
}

void LinuxHidDevice::Close() {
    if (m_handle) {
        hid_close(static_cast<hid_device*>(m_handle));
        m_handle = nullptr;
    }
}

int LinuxHidDevice::Write(const uint8_t* data, size_t length) {
    if (!m_handle) return -1;
    return hid_write(static_cast<hid_device*>(m_handle), data, length);
}

int LinuxHidDevice::Read(uint8_t* buffer, size_t length, int timeoutMs) {
    if (!m_handle) return -1;
    return hid_read_timeout(static_cast<hid_device*>(m_handle), buffer, length, timeoutMs);
}

int LinuxHidDevice::SendFeatureReport(const uint8_t* data, size_t length) {
    if (!m_handle) return -1;
    return hid_send_feature_report(static_cast<hid_device*>(m_handle), data, length);
}

int LinuxHidDevice::GetFeatureReport(uint8_t* buffer, size_t length) {
    if (!m_handle) return -1;
    return hid_get_feature_report(static_cast<hid_device*>(m_handle), buffer, length);
}

std::string LinuxHidDevice::GetManufacturer() const {
    if (!m_handle) return "";

    wchar_t buf[256];
    if (hid_get_manufacturer_string(static_cast<hid_device*>(m_handle), buf, sizeof(buf) / sizeof(wchar_t)) == 0) {
        return LinuxDeviceEnumerator::WcharToUtf8(buf);
    }
    return "";
}

std::string LinuxHidDevice::GetProduct() const {
    if (!m_handle) return "";

    wchar_t buf[256];
    if (hid_get_product_string(static_cast<hid_device*>(m_handle), buf, sizeof(buf) / sizeof(wchar_t)) == 0) {
        return LinuxDeviceEnumerator::WcharToUtf8(buf);
    }
    return "";
}

std::string LinuxHidDevice::GetSerialNumber() const {
    if (!m_handle) return "";

    wchar_t buf[256];
    if (hid_get_serial_number_string(static_cast<hid_device*>(m_handle), buf, sizeof(buf) / sizeof(wchar_t)) == 0) {
        return LinuxDeviceEnumerator::WcharToUtf8(buf);
    }
    return "";
}

//=============================================================================
// LinuxDeviceEnumerator
//=============================================================================

LinuxDeviceEnumerator::LinuxDeviceEnumerator() = default;

LinuxDeviceEnumerator::~LinuxDeviceEnumerator() {
    if (m_hidInitialized) {
        hid_exit();
    }
}

bool LinuxDeviceEnumerator::Initialize() {
    if (m_hidInitialized) {
        return true;
    }

    if (hid_init() == 0) {
        m_hidInitialized = true;
        return true;
    }

    return false;
}

std::string LinuxDeviceEnumerator::ReadSysfsFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }

    std::string content;
    std::getline(file, content);

    // Trim whitespace
    size_t start = content.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = content.find_last_not_of(" \t\r\n");
    return content.substr(start, end - start + 1);
}

std::string LinuxDeviceEnumerator::WcharToUtf8(const wchar_t* wstr) {
    if (!wstr) return "";

    std::string result;
    while (*wstr) {
        wchar_t wc = *wstr++;
        if (wc < 0x80) {
            result += static_cast<char>(wc);
        } else if (wc < 0x800) {
            result += static_cast<char>(0xC0 | (wc >> 6));
            result += static_cast<char>(0x80 | (wc & 0x3F));
        } else {
            result += static_cast<char>(0xE0 | (wc >> 12));
            result += static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (wc & 0x3F));
        }
    }
    return result;
}

//=============================================================================
// USB Enumeration
//=============================================================================

std::vector<UsbDeviceInfo> LinuxDeviceEnumerator::EnumerateUSB() {
    std::vector<UsbDeviceInfo> devices;

    DIR* dir = opendir("/sys/bus/usb/devices");
    if (!dir) {
        return devices;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;

        // Skip . and .. and hub entries
        if (name[0] == '.' || name.find(':') != std::string::npos) {
            continue;
        }

        std::string basePath = "/sys/bus/usb/devices/" + name;

        // Check if it's a USB device (has idVendor)
        std::string vidStr = ReadSysfsFile(basePath + "/idVendor");
        if (vidStr.empty()) {
            continue;
        }

        UsbDeviceInfo usb;
        usb.vendorId = static_cast<uint16_t>(std::strtoul(vidStr.c_str(), nullptr, 16));

        std::string pidStr = ReadSysfsFile(basePath + "/idProduct");
        usb.productId = static_cast<uint16_t>(std::strtoul(pidStr.c_str(), nullptr, 16));

        usb.manufacturer = ReadSysfsFile(basePath + "/manufacturer");
        usb.description = ReadSysfsFile(basePath + "/product");
        usb.serialNumber = ReadSysfsFile(basePath + "/serial");
        usb.path = basePath;

        devices.push_back(usb);
    }

    closedir(dir);
    return devices;
}

std::vector<UsbDeviceInfo> LinuxDeviceEnumerator::EnumerateUSB(uint16_t vendorId) {
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

std::vector<HidDeviceInfo> LinuxDeviceEnumerator::EnumerateHID() {
    return EnumerateHID(0, 0);
}

std::vector<HidDeviceInfo> LinuxDeviceEnumerator::EnumerateHID(uint16_t vendorId, uint16_t productId) {
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
            info.manufacturer = WcharToUtf8(cur->manufacturer_string);
        }
        if (cur->product_string) {
            info.product = WcharToUtf8(cur->product_string);
        }
        if (cur->serial_number) {
            info.serialNumber = WcharToUtf8(cur->serial_number);
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

std::unique_ptr<IHidDevice> LinuxDeviceEnumerator::OpenHID(const std::string& path) {
    if (!m_hidInitialized) {
        Initialize();
    }

    hid_device* handle = hid_open_path(path.c_str());
    if (!handle) {
        return nullptr;
    }

    return std::make_unique<LinuxHidDevice>(handle);
}

std::unique_ptr<IHidDevice> LinuxDeviceEnumerator::OpenHID(
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

    hid_device* handle = hid_open(vendorId, productId, nullptr);
    if (!handle) {
        return nullptr;
    }

    return std::make_unique<LinuxHidDevice>(handle);
}

//=============================================================================
// Hotplug
//=============================================================================

bool LinuxDeviceEnumerator::SupportsHotplug() const {
    // Could be implemented with libudev
    return false;
}

void LinuxDeviceEnumerator::RegisterHotplugCallback(HotplugCallback callback) {
    m_hotplugCallback = callback;
    // TODO: Implement with libudev monitor
}

void LinuxDeviceEnumerator::UnregisterHotplugCallback() {
    m_hotplugCallback = nullptr;
}

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_LINUX
