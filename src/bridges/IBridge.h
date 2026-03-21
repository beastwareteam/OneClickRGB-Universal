#pragma once
//=============================================================================
// OneClickRGB-Universal - Bridge Interface
//=============================================================================
// Abstract interface for hardware communication bridges.
// Bridges handle the low-level protocol (HID, SMBus, USB, etc.)
//=============================================================================

#include "../core/Types.h"
#include <cstdint>
#include <cstddef>

namespace OCRGB {

//-----------------------------------------------------------------------------
// Bridge Interface
//-----------------------------------------------------------------------------
class IBridge {
public:
    virtual ~IBridge() = default;

    //=========================================================================
    // Lifecycle
    //=========================================================================

    /// Open connection to the device
    /// @param address Device address information
    /// @return true if successful
    virtual bool Open(const DeviceAddress& address) = 0;

    /// Close the connection
    virtual void Close() = 0;

    /// Check if connection is open
    virtual bool IsOpen() const = 0;

    //=========================================================================
    // Communication
    //=========================================================================

    /// Write data to the device
    /// @param data Pointer to data buffer
    /// @param length Number of bytes to write
    /// @return true if successful
    virtual bool Write(const uint8_t* data, size_t length) = 0;

    /// Read data from the device
    /// @param buffer Buffer to store read data
    /// @param length Maximum bytes to read
    /// @param timeoutMs Timeout in milliseconds
    /// @return Number of bytes read, or -1 on error
    virtual int Read(uint8_t* buffer, size_t length, int timeoutMs = 1000) = 0;

    //=========================================================================
    // Protocol-Specific Methods
    //=========================================================================

    /// Get the protocol type this bridge handles
    virtual ProtocolType GetProtocolType() const = 0;

    /// Get last error message
    virtual std::string GetLastError() const { return m_lastError; }

protected:
    std::string m_lastError;

    void SetError(const std::string& error) { m_lastError = error; }
    void ClearError() { m_lastError.clear(); }
};

} // namespace OCRGB
