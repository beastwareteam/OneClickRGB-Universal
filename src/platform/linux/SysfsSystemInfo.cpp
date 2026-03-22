//=============================================================================
// OneClickRGB-Universal - Sysfs System Information Implementation
//=============================================================================

#ifdef OCRGB_PLATFORM_LINUX

#include "SysfsSystemInfo.h"
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <algorithm>

namespace OCRGB {
namespace Platform {

//=============================================================================
// Helpers
//=============================================================================

bool SysfsSystemInfo::Initialize() {
    return true;
}

std::string SysfsSystemInfo::ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }

    std::string content;
    std::getline(file, content);
    return TrimWhitespace(content);
}

std::string SysfsSystemInfo::ReadDmiFile(const std::string& name) {
    return ReadFile("/sys/class/dmi/id/" + name);
}

std::string SysfsSystemInfo::TrimWhitespace(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string SysfsSystemInfo::GetCpuInfoField(const std::string& field) {
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        return "";
    }

    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find(field) == 0) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                return TrimWhitespace(line.substr(colonPos + 1));
            }
        }
    }

    return "";
}

//=============================================================================
// Mainboard (DMI)
//=============================================================================

std::string SysfsSystemInfo::GetMainboardManufacturer() {
    return ReadDmiFile("board_vendor");
}

std::string SysfsSystemInfo::GetMainboardProduct() {
    return ReadDmiFile("board_name");
}

std::string SysfsSystemInfo::GetMainboardSerial() {
    return ReadDmiFile("board_serial");
}

std::string SysfsSystemInfo::GetMainboardVersion() {
    return ReadDmiFile("board_version");
}

//=============================================================================
// BIOS (DMI)
//=============================================================================

std::string SysfsSystemInfo::GetBiosVendor() {
    return ReadDmiFile("bios_vendor");
}

std::string SysfsSystemInfo::GetBiosVersion() {
    return ReadDmiFile("bios_version");
}

std::string SysfsSystemInfo::GetBiosDate() {
    return ReadDmiFile("bios_date");
}

//=============================================================================
// CPU
//=============================================================================

std::string SysfsSystemInfo::GetCpuName() {
    return GetCpuInfoField("model name");
}

std::string SysfsSystemInfo::GetCpuVendor() {
    return GetCpuInfoField("vendor_id");
}

int SysfsSystemInfo::GetCpuCores() {
    // Count physical cores from /sys
    std::string content = ReadFile("/sys/devices/system/cpu/cpu0/topology/core_cpus_list");
    if (content.empty()) {
        // Fallback: count unique core IDs
        int maxCore = 0;
        for (int i = 0; i < 256; i++) {
            std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/topology/core_id";
            std::string coreId = ReadFile(path);
            if (coreId.empty()) break;
            int id = std::atoi(coreId.c_str());
            if (id > maxCore) maxCore = id;
        }
        return maxCore + 1;
    }

    // Parse from cpu_siblings
    std::string siblings = GetCpuInfoField("cpu cores");
    if (!siblings.empty()) {
        return std::atoi(siblings.c_str());
    }

    return 1;
}

int SysfsSystemInfo::GetCpuThreads() {
    // Count online CPUs
    int count = 0;
    for (int i = 0; i < 256; i++) {
        std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(i);
        if (access(path.c_str(), F_OK) == 0) {
            count++;
        } else {
            break;
        }
    }
    return count > 0 ? count : 1;
}

uint32_t SysfsSystemInfo::GetCpuFrequencyMHz() {
    // Read max frequency from cpufreq
    std::string freq = ReadFile("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    if (!freq.empty()) {
        return static_cast<uint32_t>(std::stoull(freq) / 1000);  // kHz to MHz
    }

    // Fallback to cpuinfo
    std::string mhz = GetCpuInfoField("cpu MHz");
    if (!mhz.empty()) {
        return static_cast<uint32_t>(std::stof(mhz));
    }

    return 0;
}

//=============================================================================
// GPU (DRM/sysfs)
//=============================================================================

std::vector<GpuInfo> SysfsSystemInfo::GetGpus() {
    std::vector<GpuInfo> gpus;

    // Enumerate DRM cards
    DIR* dir = opendir("/sys/class/drm");
    if (!dir) {
        return gpus;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;

        // Look for cardN entries
        if (name.find("card") != 0 || name.find("-") != std::string::npos) {
            continue;
        }

        GpuInfo gpu;
        std::string basePath = "/sys/class/drm/" + name + "/device";

        // Get vendor from PCI
        std::string vendorId = ReadFile(basePath + "/vendor");
        if (!vendorId.empty()) {
            if (vendorId == "0x10de") gpu.vendor = "NVIDIA";
            else if (vendorId == "0x1002") gpu.vendor = "AMD";
            else if (vendorId == "0x8086") gpu.vendor = "Intel";
            else gpu.vendor = vendorId;
        }

        // Get device name (requires PCI database lookup, simplified here)
        std::string deviceId = ReadFile(basePath + "/device");
        gpu.name = gpu.vendor + " GPU " + deviceId;

        // Driver version
        std::string driver = ReadFile(basePath + "/driver/module/version");
        if (!driver.empty()) {
            gpu.driverVersion = driver;
        }

        if (!gpu.vendor.empty()) {
            gpus.push_back(gpu);
        }
    }

    closedir(dir);
    return gpus;
}

//=============================================================================
// RAM
//=============================================================================

std::vector<RamModuleInfo> SysfsSystemInfo::GetRamModules() {
    std::vector<RamModuleInfo> modules;

    // Note: Getting detailed RAM info requires dmidecode with root
    // For now, return basic info from /proc/meminfo
    RamModuleInfo info;
    info.manufacturer = "Unknown";
    info.capacityBytes = GetTotalRamBytes();
    info.slot = "System";
    modules.push_back(info);

    return modules;
}

uint64_t SysfsSystemInfo::GetTotalRamBytes() {
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) {
        return 0;
    }

    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            // Format: MemTotal:       16384000 kB
            size_t start = line.find_first_of("0123456789");
            if (start != std::string::npos) {
                uint64_t kb = std::stoull(line.substr(start));
                return kb * 1024;  // Convert kB to bytes
            }
        }
    }

    return 0;
}

//=============================================================================
// OS
//=============================================================================

std::string SysfsSystemInfo::GetOsName() {
    // Try /etc/os-release
    std::ifstream osRelease("/etc/os-release");
    if (osRelease.is_open()) {
        std::string line;
        while (std::getline(osRelease, line)) {
            if (line.find("PRETTY_NAME=") == 0) {
                std::string name = line.substr(12);
                // Remove quotes
                if (name.front() == '"') name = name.substr(1);
                if (name.back() == '"') name.pop_back();
                return name;
            }
        }
    }

    return "Linux";
}

std::string SysfsSystemInfo::GetOsVersion() {
    std::ifstream osRelease("/etc/os-release");
    if (osRelease.is_open()) {
        std::string line;
        while (std::getline(osRelease, line)) {
            if (line.find("VERSION_ID=") == 0) {
                std::string ver = line.substr(11);
                if (ver.front() == '"') ver = ver.substr(1);
                if (ver.back() == '"') ver.pop_back();
                return ver;
            }
        }
    }

    return "";
}

std::string SysfsSystemInfo::GetKernelVersion() {
    struct utsname buf;
    if (uname(&buf) == 0) {
        return buf.release;
    }
    return "";
}

std::string SysfsSystemInfo::GetArchitecture() {
    struct utsname buf;
    if (uname(&buf) == 0) {
        return buf.machine;
    }
    return "";
}

std::string SysfsSystemInfo::GetHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return hostname;
    }
    return "";
}

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_LINUX
