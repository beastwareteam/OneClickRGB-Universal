//=============================================================================
// OneClickRGB-Universal - macOS Device Enumerator Implementation
//=============================================================================

#ifdef OCRGB_PLATFORM_MACOS

#include "MacOSDeviceEnumerator.h"

// IOKit headers
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/hid/IOHIDLib.h>
#include <CoreFoundation/CoreFoundation.h>

// HIDAPI for cross-platform HID access
#include <hidapi/hidapi.h>

namespace OCRGB {
namespace Platform {

//=============================================================================
// Helpers
//=============================================================================

static std::string CFStringToStdString(CFStringRef cfStr) {
    if (!cfStr) return "";

    CFIndex length = CFStringGetLength(cfStr);
    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

    std::string result(maxSize, '\0');
    if (CFStringGetCString(cfStr, &result[0], maxSize, kCFStringEncodingUTF8)) {
        result.resize(strlen(result.c_str()));
        return result;
    }

    return "";
}

static int GetIntProperty(io_service_t service, CFStringRef key) {
    CFTypeRef value = IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0);
    if (!value) return 0;

    int result = 0;
    if (CFGetTypeID(value) == CFNumberGetTypeID()) {
        CFNumberGetValue((CFNumberRef)value, kCFNumberIntType, &result);
    }
    CFRelease(value);
    return result;
}

static std::string GetStringProperty(io_service_t service, CFStringRef key) {
    CFTypeRef value = IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0);
    if (!value) return "";

    std::string result;
    if (CFGetTypeID(value) == CFStringGetTypeID()) {
        result = CFStringToStdString((CFStringRef)value);
    }
    CFRelease(value);
    return result;
}

//=============================================================================
// Lifecycle
//=============================================================================

bool MacOSDeviceEnumerator::Initialize() {
    if (m_initialized) {
        return true;
    }

    // Initialize HIDAPI
    if (hid_init() != 0) {
        return false;
    }

    m_initialized = true;
    return true;
}

void MacOSDeviceEnumerator::Shutdown() {
    if (!m_initialized) {
        return;
    }

    hid_exit();
    m_initialized = false;
}

//=============================================================================
// USB Device Enumeration (IOKit)
//=============================================================================

std::vector<UsbDeviceInfo> MacOSDeviceEnumerator::EnumerateUSBDevices() {
    return EnumerateIOKitUSB();
}

std::vector<UsbDeviceInfo> MacOSDeviceEnumerator::EnumerateIOKitUSB() {
    std::vector<UsbDeviceInfo> devices;

    CFMutableDictionaryRef matching = IOServiceMatching(kIOUSBDeviceClassName);
    if (!matching) {
        return devices;
    }

    io_iterator_t iterator;
    if (IOServiceGetMatchingServices(kIOMasterPortDefault, matching, &iterator) != KERN_SUCCESS) {
        return devices;
    }

    io_service_t service;
    while ((service = IOIteratorNext(iterator)) != MACH_PORT_NULL) {
        UsbDeviceInfo device;

        // Get VID/PID
        device.vendorId = GetIntProperty(service, CFSTR(kUSBVendorID));
        device.productId = GetIntProperty(service, CFSTR(kUSBProductID));

        // Get strings
        device.manufacturer = GetStringProperty(service, CFSTR(kUSBVendorString));
        device.product = GetStringProperty(service, CFSTR(kUSBProductString));
        device.serial = GetStringProperty(service, CFSTR(kUSBSerialNumberString));

        // Get location (for path)
        int locationId = GetIntProperty(service, CFSTR(kUSBDevicePropertyLocationID));
        if (locationId > 0) {
            char pathBuf[64];
            snprintf(pathBuf, sizeof(pathBuf), "usb:%08x", locationId);
            device.path = pathBuf;
        }

        // Only add devices with valid VID/PID
        if (device.vendorId > 0 && device.productId > 0) {
            devices.push_back(device);
        }

        IOObjectRelease(service);
    }

    IOObjectRelease(iterator);
    return devices;
}

//=============================================================================
// HID Device Enumeration (HIDAPI)
//=============================================================================

std::vector<HidDeviceInfo> MacOSDeviceEnumerator::EnumerateHIDDevices() {
    return EnumerateHIDAPI();
}

std::vector<HidDeviceInfo> MacOSDeviceEnumerator::EnumerateHIDAPI() {
    std::vector<HidDeviceInfo> devices;

    if (!m_initialized) {
        return devices;
    }

    // Enumerate all HID devices
    struct hid_device_info* devs = hid_enumerate(0, 0);
    struct hid_device_info* cur = devs;

    while (cur) {
        HidDeviceInfo device;

        device.vendorId = cur->vendor_id;
        device.productId = cur->product_id;
        device.usagePage = cur->usage_page;
        device.usage = cur->usage;

        if (cur->path) {
            device.path = cur->path;
        }

        if (cur->serial_number) {
            // Convert wide string to UTF-8
            std::wstring ws(cur->serial_number);
            device.serial = std::string(ws.begin(), ws.end());
        }

        if (cur->manufacturer_string) {
            std::wstring ws(cur->manufacturer_string);
            device.manufacturer = std::string(ws.begin(), ws.end());
        }

        if (cur->product_string) {
            std::wstring ws(cur->product_string);
            device.product = std::string(ws.begin(), ws.end());
        }

        device.interfaceNumber = cur->interface_number;

        devices.push_back(device);
        cur = cur->next;
    }

    hid_free_enumeration(devs);
    return devices;
}

//=============================================================================
// I2C Device Enumeration
//=============================================================================

std::vector<I2CDeviceInfo> MacOSDeviceEnumerator::EnumerateI2CDevices() {
    // macOS doesn't provide direct I2C/SMBus access
    // Return empty list - I2C devices are not supported on this platform
    return std::vector<I2CDeviceInfo>();
}

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_MACOS
