//=============================================================================
// OneClickRGB-Universal - IOKit System Information Implementation
//=============================================================================

#ifdef OCRGB_PLATFORM_MACOS

#include "IOKitSystemInfo.h"
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <mach/mach.h>

// IOKit headers
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>

namespace OCRGB {
namespace Platform {

//=============================================================================
// Helpers
//=============================================================================

bool IOKitSystemInfo::Initialize() {
    return true;
}

std::string IOKitSystemInfo::GetSysctlString(const char* name) {
    char buffer[256];
    size_t size = sizeof(buffer);

    if (sysctlbyname(name, buffer, &size, nullptr, 0) == 0) {
        return buffer;
    }

    return "";
}

int IOKitSystemInfo::GetSysctlInt(const char* name) {
    int value = 0;
    size_t size = sizeof(value);

    if (sysctlbyname(name, &value, &size, nullptr, 0) == 0) {
        return value;
    }

    return 0;
}

uint64_t IOKitSystemInfo::GetSysctlUInt64(const char* name) {
    uint64_t value = 0;
    size_t size = sizeof(value);

    if (sysctlbyname(name, &value, &size, nullptr, 0) == 0) {
        return value;
    }

    return 0;
}

std::string IOKitSystemInfo::GetIORegistryString(const char* plane, const char* key) {
    std::string result;

    io_registry_entry_t root = IORegistryGetRootEntry(kIOMasterPortDefault);
    if (root == MACH_PORT_NULL) {
        return result;
    }

    CFStringRef keyRef = CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);
    if (!keyRef) {
        IOObjectRelease(root);
        return result;
    }

    io_registry_entry_t entry = root;
    if (plane && strlen(plane) > 0) {
        entry = IORegistryEntryFromPath(kIOMasterPortDefault, plane);
    }

    if (entry != MACH_PORT_NULL) {
        CFTypeRef value = IORegistryEntryCreateCFProperty(entry, keyRef, kCFAllocatorDefault, 0);
        if (value) {
            if (CFGetTypeID(value) == CFStringGetTypeID()) {
                char buffer[256];
                if (CFStringGetCString((CFStringRef)value, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
                    result = buffer;
                }
            } else if (CFGetTypeID(value) == CFDataGetTypeID()) {
                CFDataRef data = (CFDataRef)value;
                result.assign((const char*)CFDataGetBytePtr(data), CFDataGetLength(data));
                // Trim null terminator if present
                while (!result.empty() && result.back() == '\0') {
                    result.pop_back();
                }
            }
            CFRelease(value);
        }

        if (entry != root) {
            IOObjectRelease(entry);
        }
    }

    CFRelease(keyRef);
    IOObjectRelease(root);

    return result;
}

//=============================================================================
// Mainboard (IOKit)
//=============================================================================

std::string IOKitSystemInfo::GetMainboardManufacturer() {
    return GetIORegistryString("IODeviceTree:/", "manufacturer");
}

std::string IOKitSystemInfo::GetMainboardProduct() {
    return GetIORegistryString("IODeviceTree:/", "model");
}

std::string IOKitSystemInfo::GetMainboardSerial() {
    return GetIORegistryString("IODeviceTree:/", "IOPlatformSerialNumber");
}

std::string IOKitSystemInfo::GetMainboardVersion() {
    return GetIORegistryString("IODeviceTree:/", "board-id");
}

//=============================================================================
// BIOS/EFI
//=============================================================================

std::string IOKitSystemInfo::GetBiosVendor() {
    return "Apple";
}

std::string IOKitSystemInfo::GetBiosVersion() {
    return GetIORegistryString("IODeviceTree:/rom", "version");
}

std::string IOKitSystemInfo::GetBiosDate() {
    return GetIORegistryString("IODeviceTree:/rom", "release-date");
}

//=============================================================================
// CPU
//=============================================================================

std::string IOKitSystemInfo::GetCpuName() {
    return GetSysctlString("machdep.cpu.brand_string");
}

std::string IOKitSystemInfo::GetCpuVendor() {
    return GetSysctlString("machdep.cpu.vendor");
}

int IOKitSystemInfo::GetCpuCores() {
    return GetSysctlInt("hw.physicalcpu");
}

int IOKitSystemInfo::GetCpuThreads() {
    return GetSysctlInt("hw.logicalcpu");
}

uint32_t IOKitSystemInfo::GetCpuFrequencyMHz() {
    uint64_t freq = GetSysctlUInt64("hw.cpufrequency");
    return static_cast<uint32_t>(freq / 1000000);  // Hz to MHz
}

//=============================================================================
// GPU
//=============================================================================

std::vector<GpuInfo> IOKitSystemInfo::GetGpus() {
    std::vector<GpuInfo> gpus;

    // Find GPU services
    CFMutableDictionaryRef matching = IOServiceMatching("IOPCIDevice");
    io_iterator_t iterator;

    if (IOServiceGetMatchingServices(kIOMasterPortDefault, matching, &iterator) != KERN_SUCCESS) {
        return gpus;
    }

    io_service_t service;
    while ((service = IOIteratorNext(iterator)) != MACH_PORT_NULL) {
        CFTypeRef classCode = IORegistryEntryCreateCFProperty(service, CFSTR("class-code"), kCFAllocatorDefault, 0);

        if (classCode) {
            // Check if it's a display controller (class 0x03)
            if (CFGetTypeID(classCode) == CFDataGetTypeID()) {
                CFDataRef data = (CFDataRef)classCode;
                if (CFDataGetLength(data) >= 4) {
                    const uint8_t* bytes = CFDataGetBytePtr(data);
                    if (bytes[2] == 0x03) {  // Display controller class
                        GpuInfo gpu;

                        // Get model name
                        CFTypeRef model = IORegistryEntryCreateCFProperty(service, CFSTR("model"), kCFAllocatorDefault, 0);
                        if (model && CFGetTypeID(model) == CFDataGetTypeID()) {
                            CFDataRef modelData = (CFDataRef)model;
                            gpu.name.assign((const char*)CFDataGetBytePtr(modelData), CFDataGetLength(modelData));
                            while (!gpu.name.empty() && gpu.name.back() == '\0') {
                                gpu.name.pop_back();
                            }
                            CFRelease(model);
                        }

                        // Determine vendor from vendor-id
                        CFTypeRef vendorId = IORegistryEntryCreateCFProperty(service, CFSTR("vendor-id"), kCFAllocatorDefault, 0);
                        if (vendorId && CFGetTypeID(vendorId) == CFDataGetTypeID()) {
                            CFDataRef vidData = (CFDataRef)vendorId;
                            if (CFDataGetLength(vidData) >= 2) {
                                const uint8_t* vid = CFDataGetBytePtr(vidData);
                                uint16_t vendor = vid[0] | (vid[1] << 8);
                                if (vendor == 0x10DE) gpu.vendor = "NVIDIA";
                                else if (vendor == 0x1002) gpu.vendor = "AMD";
                                else if (vendor == 0x8086) gpu.vendor = "Intel";
                                else if (vendor == 0x106B) gpu.vendor = "Apple";
                            }
                            CFRelease(vendorId);
                        }

                        if (!gpu.name.empty()) {
                            gpus.push_back(gpu);
                        }
                    }
                }
            }
            CFRelease(classCode);
        }

        IOObjectRelease(service);
    }

    IOObjectRelease(iterator);
    return gpus;
}

//=============================================================================
// RAM
//=============================================================================

std::vector<RamModuleInfo> IOKitSystemInfo::GetRamModules() {
    std::vector<RamModuleInfo> modules;

    // macOS doesn't provide detailed RAM module info without system_profiler
    RamModuleInfo info;
    info.manufacturer = "Unknown";
    info.capacityBytes = GetTotalRamBytes();
    info.slot = "System";
    modules.push_back(info);

    return modules;
}

uint64_t IOKitSystemInfo::GetTotalRamBytes() {
    return GetSysctlUInt64("hw.memsize");
}

//=============================================================================
// OS
//=============================================================================

std::string IOKitSystemInfo::GetOsName() {
    return "macOS";
}

std::string IOKitSystemInfo::GetOsVersion() {
    return GetSysctlString("kern.osproductversion");
}

std::string IOKitSystemInfo::GetKernelVersion() {
    return GetSysctlString("kern.osrelease");
}

std::string IOKitSystemInfo::GetArchitecture() {
    return GetSysctlString("hw.machine");
}

std::string IOKitSystemInfo::GetHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return hostname;
    }
    return "";
}

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_MACOS
