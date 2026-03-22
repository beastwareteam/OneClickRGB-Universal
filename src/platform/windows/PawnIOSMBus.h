#pragma once
//=============================================================================
// OneClickRGB-Universal - PawnIO SMBus Provider (Windows)
//=============================================================================

#ifdef OCRGB_PLATFORM_WINDOWS

#include "../IPlatform.h"
#include <string>

namespace OCRGB {
namespace Platform {

class PawnIOSMBus : public ISMBusProvider {
public:
    PawnIOSMBus();
    ~PawnIOSMBus() override;

    //=========================================================================
    // Availability
    //=========================================================================
    bool IsAvailable() const override;
    bool RequiresElevation() const override { return true; }
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

private:
    void* m_hPawnLib = nullptr;
    bool m_initialized = false;
    bool m_available = false;
    std::string m_lastError;
    std::string m_driverPath;

    bool LoadLibrary();
    void UnloadLibrary();
    void SetError(const std::string& error);
};

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_WINDOWS
