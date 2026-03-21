#pragma once
//=============================================================================
// OneClickRGB-Universal - HID Bridge
//=============================================================================
// USB HID communication bridge using hidapi library.
//=============================================================================

#include "IBridge.h"
#include <hidapi.h>

namespace OCRGB {

//-----------------------------------------------------------------------------
// HID Bridge
//-----------------------------------------------------------------------------
class HIDBridge : public IBridge {
public:
    HIDBridge();
    ~HIDBridge() override;

    // Prevent copying
    HIDBridge(const HIDBridge&) = delete;
    HIDBridge& operator=(const HIDBridge&) = delete;

    //=========================================================================
    // IBridge Implementation
    //=========================================================================

    bool Open(const DeviceAddress& address) override;
    void Close() override;
    bool IsOpen() const override;

    bool Write(const uint8_t* data, size_t length) override;
    int Read(uint8_t* buffer, size_t length, int timeoutMs = 1000) override;

    ProtocolType GetProtocolType() const override { return ProtocolType::HID; }

    //=========================================================================
    // HID-Specific Methods
    //=========================================================================

    /// Open by VID/PID (convenience method)
    bool Open(uint16_t vendorId, uint16_t productId,
              uint16_t usagePage = 0, uint16_t usage = 0);

    /// Open by device path
    bool OpenPath(const wchar_t* path);

    /// Send a feature report
    bool SendFeatureReport(const uint8_t* data, size_t length);

    /// Get a feature report
    bool GetFeatureReport(uint8_t* buffer, size_t length);

    /// Set non-blocking mode
    bool SetNonBlocking(bool nonblock);

    /// Get device info string
    std::wstring GetManufacturer();
    std::wstring GetProduct();
    std::wstring GetSerialNumber();

    //=========================================================================
    // Static Methods
    //=========================================================================

    /// Initialize HID library (call once at startup)
    static bool InitHID();

    /// Cleanup HID library (call once at shutdown)
    static void ExitHID();

    /// Enumerate all HID devices
    static struct hid_device_info* Enumerate(uint16_t vendorId = 0, uint16_t productId = 0);

    /// Free enumeration list
    static void FreeEnumeration(struct hid_device_info* devs);

private:
    hid_device* m_device = nullptr;
    DeviceAddress m_address;

    hid_device* FindDevice(uint16_t vendorId, uint16_t productId,
                           uint16_t usagePage, uint16_t usage);
};

} // namespace OCRGB
