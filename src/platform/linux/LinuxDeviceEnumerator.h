#pragma once
//=============================================================================
// OneClickRGB-Universal - Linux Device Enumerator
//=============================================================================

#ifdef OCRGB_PLATFORM_LINUX

#include "../IPlatform.h"

namespace OCRGB {
namespace Platform {

//=============================================================================
// HID Device Wrapper (HIDAPI)
//=============================================================================

class LinuxHidDevice : public IHidDevice {
public:
    LinuxHidDevice(void* handle);
    ~LinuxHidDevice() override;

    bool IsOpen() const override;
    void Close() override;

    int Write(const uint8_t* data, size_t length) override;
    int Read(uint8_t* buffer, size_t length, int timeoutMs = 1000) override;
    int SendFeatureReport(const uint8_t* data, size_t length) override;
    int GetFeatureReport(uint8_t* buffer, size_t length) override;

    std::string GetManufacturer() const override;
    std::string GetProduct() const override;
    std::string GetSerialNumber() const override;

private:
    void* m_handle = nullptr;
};

//=============================================================================
// Linux Device Enumerator
//=============================================================================

class LinuxDeviceEnumerator : public IDeviceEnumerator {
public:
    LinuxDeviceEnumerator();
    ~LinuxDeviceEnumerator() override;

    bool Initialize();

    //=========================================================================
    // USB Enumeration (via sysfs)
    //=========================================================================
    std::vector<UsbDeviceInfo> EnumerateUSB() override;
    std::vector<UsbDeviceInfo> EnumerateUSB(uint16_t vendorId) override;

    //=========================================================================
    // HID Enumeration (via HIDAPI)
    //=========================================================================
    std::vector<HidDeviceInfo> EnumerateHID() override;
    std::vector<HidDeviceInfo> EnumerateHID(uint16_t vendorId, uint16_t productId) override;

    //=========================================================================
    // Device Access
    //=========================================================================
    std::unique_ptr<IHidDevice> OpenHID(const std::string& path) override;
    std::unique_ptr<IHidDevice> OpenHID(uint16_t vendorId, uint16_t productId,
                                         uint16_t usagePage = 0, uint16_t usage = 0) override;

    //=========================================================================
    // Hotplug (via udev - optional)
    //=========================================================================
    bool SupportsHotplug() const override;
    void RegisterHotplugCallback(HotplugCallback callback) override;
    void UnregisterHotplugCallback() override;

private:
    bool m_hidInitialized = false;
    HotplugCallback m_hotplugCallback;

    static std::string ReadSysfsFile(const std::string& path);
    static std::string WcharToUtf8(const wchar_t* wstr);
};

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_LINUX
