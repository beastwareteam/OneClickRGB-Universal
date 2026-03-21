//=============================================================================
// OneClickRGB-Universal - HID Device Implementation
//=============================================================================

#include "HIDDevice.h"
#include <cstring>

namespace OCRGB {

HIDDevice::HIDDevice()
    : m_bridge(std::make_unique<HIDBridge>()) {
    m_info.address.protocol = ProtocolType::HID;
}

HIDDevice::~HIDDevice() {
    Shutdown();
}

void HIDDevice::SetIdentifiers(uint16_t vendorId, uint16_t productId,
                                uint16_t usagePage, uint16_t usage) {
    m_info.address.vendorId = vendorId;
    m_info.address.productId = productId;
    m_info.address.usagePage = usagePage;
    m_info.address.usage = usage;
}

void HIDDevice::SetDeviceInfo(const std::string& name, const std::string& vendor,
                              DeviceType type) {
    m_info.name = name;
    m_info.vendor = vendor;
    m_info.type = type;

    // Generate unique ID
    char idBuf[64];
    snprintf(idBuf, sizeof(idBuf), "%s_%04X_%04X",
             vendor.c_str(), m_info.address.vendorId, m_info.address.productId);
    m_info.id = idBuf;
}

void HIDDevice::SetCapabilities(const Capabilities& caps) {
    m_info.capabilities = caps;
}

Result HIDDevice::Initialize() {
    if (m_state.initialized) {
        return Result::Success();
    }

    // Open HID device
    if (!m_bridge->Open(m_info.address)) {
        return Result::Error("Failed to open HID device");
    }

    m_state.connected = true;

    // Call device-specific initialization
    Result result = OnConnected();
    if (result.IsError()) {
        m_bridge->Close();
        m_state.connected = false;
        return result;
    }

    m_state.initialized = true;
    NotifyConnected();

    return Result::Success();
}

void HIDDevice::Shutdown() {
    if (!m_state.initialized) return;

    OnDisconnecting();

    if (m_bridge) {
        m_bridge->Close();
    }

    m_state.connected = false;
    m_state.initialized = false;

    NotifyDisconnected();
}

bool HIDDevice::IsConnected() const {
    return m_state.connected && m_bridge && m_bridge->IsOpen();
}

bool HIDDevice::IsReady() const {
    return m_state.initialized && IsConnected();
}

Result HIDDevice::SendRawPacket(const uint8_t* data, size_t length) {
    if (!IsConnected()) {
        return Result::NotConnected();
    }

    if (!m_bridge->Write(data, length)) {
        NotifyError("Failed to send packet");
        return Result::Error("Write failed");
    }

    return Result::Success();
}

Result HIDDevice::ReadRawResponse(uint8_t* buffer, size_t length,
                                   size_t& bytesRead, int timeoutMs) {
    if (!IsConnected()) {
        return Result::NotConnected();
    }

    int read = m_bridge->Read(buffer, length, timeoutMs);
    if (read < 0) {
        return Result::Error("Read failed");
    }

    bytesRead = (size_t)read;
    return Result::Success();
}

Result HIDDevice::SendFeatureReport(const uint8_t* data, size_t length) {
    if (!IsConnected()) {
        return Result::NotConnected();
    }

    if (!m_bridge->SendFeatureReport(data, length)) {
        return Result::Error("Failed to send feature report");
    }

    return Result::Success();
}

Result HIDDevice::GetFeatureReport(uint8_t* buffer, size_t length) {
    if (!IsConnected()) {
        return Result::NotConnected();
    }

    if (!m_bridge->GetFeatureReport(buffer, length)) {
        return Result::Error("Failed to get feature report");
    }

    return Result::Success();
}

Result HIDDevice::SendPacket(uint8_t command, const uint8_t* payload, size_t payloadLen) {
    size_t packetSize = GetPacketSize();
    std::vector<uint8_t> buffer(packetSize, 0);

    buffer[0] = GetReportId();
    buffer[1] = command;

    if (payload && payloadLen > 0) {
        size_t copyLen = std::min(payloadLen, packetSize - 2);
        memcpy(&buffer[2], payload, copyLen);
    }

    return SendRawPacket(buffer.data(), buffer.size());
}

void HIDDevice::BuildPacket(uint8_t* buffer, size_t bufferSize,
                            uint8_t reportId, const uint8_t* header, size_t headerLen,
                            const uint8_t* payload, size_t payloadLen) {
    memset(buffer, 0, bufferSize);

    size_t offset = 0;

    // Report ID
    buffer[offset++] = reportId;

    // Header
    if (header && headerLen > 0) {
        size_t copyLen = std::min(headerLen, bufferSize - offset);
        memcpy(&buffer[offset], header, copyLen);
        offset += copyLen;
    }

    // Payload
    if (payload && payloadLen > 0) {
        size_t copyLen = std::min(payloadLen, bufferSize - offset);
        memcpy(&buffer[offset], payload, copyLen);
    }
}

} // namespace OCRGB
