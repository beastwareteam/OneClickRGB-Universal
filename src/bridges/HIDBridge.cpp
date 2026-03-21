//=============================================================================
// OneClickRGB-Universal - HID Bridge Implementation
//=============================================================================

#include "HIDBridge.h"
#include <cstring>

namespace OCRGB {

HIDBridge::HIDBridge() = default;

HIDBridge::~HIDBridge() {
    Close();
}

bool HIDBridge::InitHID() {
    return hid_init() == 0;
}

void HIDBridge::ExitHID() {
    hid_exit();
}

struct hid_device_info* HIDBridge::Enumerate(uint16_t vendorId, uint16_t productId) {
    return hid_enumerate(vendorId, productId);
}

void HIDBridge::FreeEnumeration(struct hid_device_info* devs) {
    hid_free_enumeration(devs);
}

bool HIDBridge::Open(const DeviceAddress& address) {
    if (address.protocol != ProtocolType::HID) {
        SetError("Invalid protocol type for HID bridge");
        return false;
    }

    m_address = address;

    if (!address.path.empty()) {
        return OpenPath(address.path.c_str());
    }

    return Open(address.vendorId, address.productId,
                address.usagePage, address.usage);
}

bool HIDBridge::Open(uint16_t vendorId, uint16_t productId,
                     uint16_t usagePage, uint16_t usage) {
    Close();

    m_address.vendorId = vendorId;
    m_address.productId = productId;
    m_address.usagePage = usagePage;
    m_address.usage = usage;

    if (usagePage != 0 || usage != 0) {
        // Need to find specific device with usage page/usage
        m_device = FindDevice(vendorId, productId, usagePage, usage);
    } else {
        // Simple open by VID/PID
        m_device = hid_open(vendorId, productId, nullptr);
    }

    if (!m_device) {
        SetError("Failed to open HID device");
        return false;
    }

    ClearError();
    return true;
}

bool HIDBridge::OpenPath(const wchar_t* path) {
    Close();

    m_device = hid_open_path(path);
    if (!m_device) {
        SetError("Failed to open HID device by path");
        return false;
    }

    m_address.path = path;
    ClearError();
    return true;
}

void HIDBridge::Close() {
    if (m_device) {
        hid_close(m_device);
        m_device = nullptr;
    }
}

bool HIDBridge::IsOpen() const {
    return m_device != nullptr;
}

bool HIDBridge::Write(const uint8_t* data, size_t length) {
    if (!m_device) {
        SetError("Device not open");
        return false;
    }

    int result = hid_write(m_device, data, length);
    if (result < 0) {
        const wchar_t* err = hid_error(m_device);
        if (err) {
            // Convert wchar to string
            char errBuf[256];
            wcstombs(errBuf, err, sizeof(errBuf) - 1);
            SetError(errBuf);
        } else {
            SetError("HID write failed");
        }
        return false;
    }

    return true;
}

int HIDBridge::Read(uint8_t* buffer, size_t length, int timeoutMs) {
    if (!m_device) {
        SetError("Device not open");
        return -1;
    }

    int result = hid_read_timeout(m_device, buffer, length, timeoutMs);
    if (result < 0) {
        SetError("HID read failed");
    }

    return result;
}

bool HIDBridge::SendFeatureReport(const uint8_t* data, size_t length) {
    if (!m_device) {
        SetError("Device not open");
        return false;
    }

    int result = hid_send_feature_report(m_device, data, length);
    if (result < 0) {
        SetError("Failed to send feature report");
        return false;
    }

    return true;
}

bool HIDBridge::GetFeatureReport(uint8_t* buffer, size_t length) {
    if (!m_device) {
        SetError("Device not open");
        return false;
    }

    int result = hid_get_feature_report(m_device, buffer, length);
    if (result < 0) {
        SetError("Failed to get feature report");
        return false;
    }

    return true;
}

bool HIDBridge::SetNonBlocking(bool nonblock) {
    if (!m_device) {
        return false;
    }

    return hid_set_nonblocking(m_device, nonblock ? 1 : 0) == 0;
}

std::wstring HIDBridge::GetManufacturer() {
    if (!m_device) return L"";

    wchar_t buf[256] = {0};
    if (hid_get_manufacturer_string(m_device, buf, sizeof(buf)/sizeof(buf[0])) == 0) {
        return buf;
    }
    return L"";
}

std::wstring HIDBridge::GetProduct() {
    if (!m_device) return L"";

    wchar_t buf[256] = {0};
    if (hid_get_product_string(m_device, buf, sizeof(buf)/sizeof(buf[0])) == 0) {
        return buf;
    }
    return L"";
}

std::wstring HIDBridge::GetSerialNumber() {
    if (!m_device) return L"";

    wchar_t buf[256] = {0};
    if (hid_get_serial_number_string(m_device, buf, sizeof(buf)/sizeof(buf[0])) == 0) {
        return buf;
    }
    return L"";
}

hid_device* HIDBridge::FindDevice(uint16_t vendorId, uint16_t productId,
                                   uint16_t usagePage, uint16_t usage) {
    struct hid_device_info* devs = hid_enumerate(vendorId, productId);
    if (!devs) return nullptr;

    hid_device* result = nullptr;

    for (struct hid_device_info* cur = devs; cur; cur = cur->next) {
        bool match = true;

        if (usagePage != 0 && cur->usage_page != usagePage) {
            match = false;
        }
        if (usage != 0 && cur->usage != usage) {
            match = false;
        }

        if (match) {
            result = hid_open_path(cur->path);
            if (result) {
                m_address.path = cur->path;
                break;
            }
        }
    }

    hid_free_enumeration(devs);
    return result;
}

} // namespace OCRGB
