#pragma once
//=============================================================================
// OneClickRGB-Universal - HID Device Base Class
//=============================================================================
// Base implementation for USB HID devices. Most RGB devices use HID protocol.
// Specific devices should extend this class and implement the protocol details.
//=============================================================================

#include "IDevice.h"
#include "../bridges/HIDBridge.h"
#include <memory>

namespace OCRGB {

//-----------------------------------------------------------------------------
// HID Device Base Class
//-----------------------------------------------------------------------------
class HIDDevice : public IDevice {
public:
    HIDDevice();
    virtual ~HIDDevice();

    //=========================================================================
    // IDevice Implementation
    //=========================================================================

    Result Initialize() override;
    void Shutdown() override;
    bool IsConnected() const override;
    bool IsReady() const override;

    const DeviceInfo& GetInfo() const override { return m_info; }
    const Capabilities& GetCapabilities() const override { return m_info.capabilities; }
    const DeviceState& GetState() const override { return m_state; }

    Result SendRawPacket(const uint8_t* data, size_t length) override;
    Result ReadRawResponse(uint8_t* buffer, size_t length,
                          size_t& bytesRead, int timeoutMs = 1000) override;
    Result SendFeatureReport(const uint8_t* data, size_t length) override;
    Result GetFeatureReport(uint8_t* buffer, size_t length) override;

protected:
    //=========================================================================
    // Methods for Derived Classes
    //=========================================================================

    /// Set device identifiers - must be called before Initialize()
    void SetIdentifiers(uint16_t vendorId, uint16_t productId,
                       uint16_t usagePage = 0, uint16_t usage = 0);

    /// Set device info - must be called before Initialize()
    void SetDeviceInfo(const std::string& name, const std::string& vendor,
                      DeviceType type);

    /// Set capabilities - must be called before Initialize()
    void SetCapabilities(const Capabilities& caps);

    /// Get the HID bridge for direct access
    HIDBridge* GetBridge() { return m_bridge.get(); }

    /// Build and send a packet using the device's packet format
    /// Derived classes should implement this based on their protocol
    virtual Result SendPacket(uint8_t command, const uint8_t* payload, size_t payloadLen);

    /// Helper to create a standard packet with header
    void BuildPacket(uint8_t* buffer, size_t bufferSize,
                    uint8_t reportId, const uint8_t* header, size_t headerLen,
                    const uint8_t* payload, size_t payloadLen);

    //=========================================================================
    // Protocol Hooks - Override in Derived Classes
    //=========================================================================

    /// Called after successful connection - for device-specific init
    virtual Result OnConnected() { return Result::Success(); }

    /// Called before disconnection - for cleanup
    virtual void OnDisconnecting() {}

    /// Get the expected packet size for this device
    virtual size_t GetPacketSize() const { return 65; }  // Standard HID report + ID

    /// Get the report ID for output reports
    virtual uint8_t GetReportId() const { return 0; }

protected:
    DeviceInfo m_info;
    DeviceState m_state;
    std::unique_ptr<HIDBridge> m_bridge;
};

} // namespace OCRGB
