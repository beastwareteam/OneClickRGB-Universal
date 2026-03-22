#pragma once
//=============================================================================
// OneClickRGB-Universal - I2C-Dev SMBus Provider (Linux)
//=============================================================================
// Uses /dev/i2c-* devices for SMBus access.
// Requires i2c-dev kernel module and appropriate permissions.
//=============================================================================

#ifdef OCRGB_PLATFORM_LINUX

#include "../IPlatform.h"
#include <string>

namespace OCRGB {
namespace Platform {

class I2CDevSMBus : public ISMBusProvider {
public:
    I2CDevSMBus();
    ~I2CDevSMBus() override;

    //=========================================================================
    // Availability
    //=========================================================================
    bool IsAvailable() const override;
    bool RequiresElevation() const override;
    std::string GetDriverInfo() const override;

    //=========================================================================
    // Lifecycle
    //=========================================================================
    bool Initialize() override;
    void Shutdown() override;
    bool IsInitialized() const override { return m_initialized; }

    //=========================================================================
    // Write Operations
    //=========================================================================
    bool WriteByte(uint8_t deviceAddr, uint8_t reg, uint8_t value) override;
    bool WriteWord(uint8_t deviceAddr, uint8_t reg, uint16_t value) override;
    bool WriteBlock(uint8_t deviceAddr, uint8_t reg,
                    const uint8_t* data, size_t length) override;

    //=========================================================================
    // Read Operations
    //=========================================================================
    bool ReadByte(uint8_t deviceAddr, uint8_t reg, uint8_t& value) override;
    bool ReadWord(uint8_t deviceAddr, uint8_t reg, uint16_t& value) override;
    bool ReadBlock(uint8_t deviceAddr, uint8_t reg,
                   uint8_t* buffer, size_t length) override;

    //=========================================================================
    // Bus Scanning
    //=========================================================================
    std::vector<uint8_t> ScanBus() override;
    bool ProbeAddress(uint8_t deviceAddr) override;

    //=========================================================================
    // Error Handling
    //=========================================================================
    std::string GetLastError() const override { return m_lastError; }
    void ClearError() override { m_lastError.clear(); }

    //=========================================================================
    // Bus Selection
    //=========================================================================

    /// Set which I2C bus to use (default: auto-detect)
    void SetBusNumber(int busNum);

    /// Get available I2C buses
    std::vector<int> GetAvailableBuses() const;

private:
    int m_fd = -1;
    int m_busNumber = -1;
    bool m_initialized = false;
    bool m_available = false;
    std::string m_lastError;
    std::string m_devicePath;

    bool OpenBus(int busNum);
    void CloseBus();
    bool SetSlaveAddress(uint8_t addr);
    void SetError(const std::string& error);

    int FindSMBusBus() const;
};

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_LINUX
