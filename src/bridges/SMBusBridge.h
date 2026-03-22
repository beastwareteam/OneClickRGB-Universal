#pragma once
//=============================================================================
// OneClickRGB-Universal - SMBus Bridge
//=============================================================================
// SMBus communication bridge using PawnIO library for Windows.
// PawnIO provides kernel-level access to SMBus for RGB RAM control.
//=============================================================================

#include "IBridge.h"

namespace OCRGB {

//-----------------------------------------------------------------------------
// PawnIO Library Interface (dynamically loaded)
//-----------------------------------------------------------------------------
typedef void* PawnIO_t;

//-----------------------------------------------------------------------------
// SMBus Bridge
//-----------------------------------------------------------------------------
class SMBusBridge : public IBridge {
public:
    SMBusBridge();
    ~SMBusBridge() override;

    // Prevent copying
    SMBusBridge(const SMBusBridge&) = delete;
    SMBusBridge& operator=(const SMBusBridge&) = delete;

    //=========================================================================
    // IBridge Implementation
    //=========================================================================

    bool Open(const DeviceAddress& address) override;
    void Close() override;
    bool IsOpen() const override;

    bool Write(const uint8_t* data, size_t length) override;
    int Read(uint8_t* buffer, size_t length, int timeoutMs = 1000) override;

    ProtocolType GetProtocolType() const override { return ProtocolType::SMBus; }

    //=========================================================================
    // SMBus Lifecycle
    //=========================================================================

    /// Initialize PawnIO and SMBus access
    bool Initialize();

    /// Shutdown PawnIO
    void Shutdown();

    /// Check if bridge is ready
    bool IsReady() const;

    //=========================================================================
    // SMBus Operations
    //=========================================================================

    /// Write a single byte to a register
    bool WriteByte(uint8_t deviceAddr, uint8_t reg, uint8_t value);

    /// Write a word (2 bytes) to a register
    bool WriteWord(uint8_t deviceAddr, uint8_t reg, uint16_t value);

    /// Write a block of bytes
    bool WriteBlock(uint8_t deviceAddr, uint8_t reg, const uint8_t* data, size_t length);

    /// Quick command (address only, no data)
    bool QuickCommand(uint8_t deviceAddr, bool read);

    /// Read a byte from a register
    bool ReadByte(uint8_t deviceAddr, uint8_t reg, uint8_t& value);

    /// Read a word from a register
    bool ReadWord(uint8_t deviceAddr, uint8_t reg, uint16_t& value);

    /// Read a block of bytes
    bool ReadBlock(uint8_t deviceAddr, uint8_t reg, uint8_t* buffer, size_t length);

    //=========================================================================
    // Device Detection
    //=========================================================================

    /// Scan for devices on SMBus
    /// @param addresses Output vector of found device addresses
    /// @return Number of devices found
    int ScanBus(std::vector<uint8_t>& addresses);

    /// Check if a device exists at address
    bool ProbeAddress(uint8_t deviceAddr);

    //=========================================================================
    // Static Methods
    //=========================================================================

    /// Check if PawnIO is available
    static bool IsPawnIOAvailable();

    /// Get PawnIO driver path
    static std::string GetPawnIOPath();

private:
    PawnIO_t m_pawnio = nullptr;
    bool m_initialized = false;
    bool m_dryRunMode = false;  // True when initialized in dry-run mode
    uint8_t m_currentDeviceAddr = 0;

    // PawnIO function pointers (loaded dynamically)
    void* m_hPawnLib = nullptr;

    bool LoadPawnIO();
    void UnloadPawnIO();
};

} // namespace OCRGB
