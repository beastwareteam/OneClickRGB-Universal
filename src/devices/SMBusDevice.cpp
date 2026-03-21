//=============================================================================
// OneClickRGB-Universal - SMBus Device Implementation
//=============================================================================

#include "SMBusDevice.h"
#include <cstring>

namespace OCRGB {

SMBusDevice::SMBusDevice()
    : m_bridge(std::make_unique<SMBusBridge>()) {
    m_info.address.protocol = ProtocolType::SMBus;
}

SMBusDevice::~SMBusDevice() {
    Shutdown();
}

void SMBusDevice::SetSMBusAddress(uint8_t busNumber, uint8_t deviceAddress) {
    m_info.address.busNumber = busNumber;
    m_info.address.deviceAddress = deviceAddress;
}

void SMBusDevice::SetDeviceInfo(const std::string& name, const std::string& vendor,
                                DeviceType type) {
    m_info.name = name;
    m_info.vendor = vendor;
    m_info.type = type;

    // Generate unique ID
    char idBuf[64];
    snprintf(idBuf, sizeof(idBuf), "%s_SMBus_%d_0x%02X",
             vendor.c_str(), m_info.address.busNumber, m_info.address.deviceAddress);
    m_info.id = idBuf;
}

void SMBusDevice::SetCapabilities(const Capabilities& caps) {
    m_info.capabilities = caps;
}

Result SMBusDevice::Initialize() {
    if (m_state.initialized) {
        return Result::Success();
    }

    // Initialize SMBus bridge (PawnIO)
    if (!m_bridge->Initialize()) {
        return Result::Error("Failed to initialize SMBus (PawnIO)");
    }

    // Probe for device
    if (!ProbeDevice()) {
        m_bridge->Shutdown();
        return Result::Error("Device not found on SMBus");
    }

    m_state.connected = true;

    // Call device-specific initialization
    Result result = OnConnected();
    if (result.IsError()) {
        m_bridge->Shutdown();
        m_state.connected = false;
        return result;
    }

    m_state.initialized = true;
    NotifyConnected();

    return Result::Success();
}

void SMBusDevice::Shutdown() {
    if (!m_state.initialized) return;

    OnDisconnecting();

    if (m_bridge) {
        m_bridge->Shutdown();
    }

    m_state.connected = false;
    m_state.initialized = false;

    NotifyDisconnected();
}

bool SMBusDevice::IsConnected() const {
    return m_state.connected && m_bridge && m_bridge->IsReady();
}

bool SMBusDevice::IsReady() const {
    return m_state.initialized && IsConnected();
}

bool SMBusDevice::ProbeDevice() {
    // Try to read a byte from the device
    // Most SMBus devices respond to register 0x00
    uint8_t dummy;
    return ReadByte(0x00, dummy).IsSuccess();
}

Result SMBusDevice::SendRawPacket(const uint8_t* data, size_t length) {
    if (!IsConnected()) {
        return Result::NotConnected();
    }

    if (length < 2) {
        return Result{ResultCode::InvalidParameter, "Packet too short"};
    }

    // First byte is register, rest is data
    return WriteBlock(data[0], &data[1], length - 1);
}

Result SMBusDevice::ReadRawResponse(uint8_t* buffer, size_t length,
                                     size_t& bytesRead, int timeoutMs) {
    if (!IsConnected()) {
        return Result::NotConnected();
    }

    // SMBus doesn't have async reads - just read from register 0
    Result result = ReadBlock(0x00, buffer, length);
    if (result.IsSuccess()) {
        bytesRead = length;
    }
    return result;
}

Result SMBusDevice::WriteByte(uint8_t reg, uint8_t value) {
    if (!IsConnected()) {
        return Result::NotConnected();
    }

    if (!m_bridge->WriteByte(m_info.address.deviceAddress, reg, value)) {
        NotifyError("SMBus write byte failed");
        return Result::Error("SMBus write failed");
    }

    return Result::Success();
}

Result SMBusDevice::WriteWord(uint8_t reg, uint16_t value) {
    if (!IsConnected()) {
        return Result::NotConnected();
    }

    if (!m_bridge->WriteWord(m_info.address.deviceAddress, reg, value)) {
        NotifyError("SMBus write word failed");
        return Result::Error("SMBus write failed");
    }

    return Result::Success();
}

Result SMBusDevice::WriteBlock(uint8_t reg, const uint8_t* data, size_t length) {
    if (!IsConnected()) {
        return Result::NotConnected();
    }

    if (!m_bridge->WriteBlock(m_info.address.deviceAddress, reg, data, length)) {
        NotifyError("SMBus write block failed");
        return Result::Error("SMBus write failed");
    }

    return Result::Success();
}

Result SMBusDevice::ReadByte(uint8_t reg, uint8_t& value) {
    if (!IsConnected()) {
        return Result::NotConnected();
    }

    if (!m_bridge->ReadByte(m_info.address.deviceAddress, reg, value)) {
        return Result::Error("SMBus read failed");
    }

    return Result::Success();
}

Result SMBusDevice::ReadWord(uint8_t reg, uint16_t& value) {
    if (!IsConnected()) {
        return Result::NotConnected();
    }

    if (!m_bridge->ReadWord(m_info.address.deviceAddress, reg, value)) {
        return Result::Error("SMBus read failed");
    }

    return Result::Success();
}

Result SMBusDevice::ReadBlock(uint8_t reg, uint8_t* buffer, size_t length) {
    if (!IsConnected()) {
        return Result::NotConnected();
    }

    if (!m_bridge->ReadBlock(m_info.address.deviceAddress, reg, buffer, length)) {
        return Result::Error("SMBus read failed");
    }

    return Result::Success();
}

} // namespace OCRGB
