#pragma once
//=============================================================================
// OneClickRGB-Universal - Windows Device Enumerator
//=============================================================================

#ifdef OCRGB_PLATFORM_WINDOWS

#include "../IPlatform.h"

namespace OCRGB {
namespace Platform {

//=============================================================================
// HID Device Wrapper
//=============================================================================

class WindowsHidDevice : public IHidDevice {
public:
    WindowsHidDevice(void* handle);
    ~WindowsHidDevice() override;

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
// Windows Device Enumerator
//=============================================================================

class WindowsDeviceEnumerator : public IDeviceEnumerator {
public:
    WindowsDeviceEnumerator();
    ~WindowsDeviceEnumerator() override;

    bool Initialize();

    //=========================================================================
    // USB Enumeration
    //=========================================================================
    std::vector<UsbDeviceInfo> EnumerateUSB() override;
    std::vector<UsbDeviceInfo> EnumerateUSB(uint16_t vendorId) override;

    //=========================================================================
    // HID Enumeration
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
    // Hotplug
    //=========================================================================
    bool SupportsHotplug() const override { return false; }

private:
    bool m_hidInitialized = false;

    static std::string WideToUtf8(const wchar_t* wide);
};

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_WINDOWS
