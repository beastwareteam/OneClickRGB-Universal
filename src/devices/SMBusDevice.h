#pragma once
//=============================================================================
// OneClickRGB-Universal - SMBus Device Base Class
//=============================================================================
// Base implementation for SMBus devices (RAM, some motherboards).
// Uses PawnIO for SMBus access on Windows.
//=============================================================================

#include "IDevice.h"
#include "../bridges/SMBusBridge.h"
#include <memory>

namespace OCRGB {

//-----------------------------------------------------------------------------
// SMBus Device Base Class
//-----------------------------------------------------------------------------
class SMBusDevice : public IDevice {
public:
    SMBusDevice();
    virtual ~SMBusDevice();

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

protected:
    //=========================================================================
    // Methods for Derived Classes
    //=========================================================================

    /// Set SMBus identifiers - must be called before Initialize()
    void SetSMBusAddress(uint8_t busNumber, uint8_t deviceAddress);

    /// Set device info - must be called before Initialize()
    void SetDeviceInfo(const std::string& name, const std::string& vendor,
                      DeviceType type);

    /// Set capabilities - must be called before Initialize()
    void SetCapabilities(const Capabilities& caps);

    /// Get the SMBus bridge for direct access
    SMBusBridge* GetBridge() { return m_bridge.get(); }

    //=========================================================================
    // SMBus Operations
    //=========================================================================

    /// Write a byte to a register
    Result WriteByte(uint8_t reg, uint8_t value);

    /// Write a word (2 bytes) to a register
    Result WriteWord(uint8_t reg, uint16_t value);

    /// Write a block of data
    Result WriteBlock(uint8_t reg, const uint8_t* data, size_t length);

    /// Read a byte from a register
    Result ReadByte(uint8_t reg, uint8_t& value);

    /// Read a word from a register
    Result ReadWord(uint8_t reg, uint16_t& value);

    /// Read a block of data
    Result ReadBlock(uint8_t reg, uint8_t* buffer, size_t length);

    //=========================================================================
    // Protocol Hooks - Override in Derived Classes
    //=========================================================================

    /// Called after successful connection
    virtual Result OnConnected() { return Result::Success(); }

    /// Called before disconnection
    virtual void OnDisconnecting() {}

    /// Probe for device presence
    virtual bool ProbeDevice();

protected:
    DeviceInfo m_info;
    DeviceState m_state;
    std::unique_ptr<SMBusBridge> m_bridge;
};

} // namespace OCRGB
