//=============================================================================
// OneClickRGB-Universal - SMBus Bridge Implementation
//=============================================================================
// Uses PawnIO for SMBus access. PawnIO is a kernel driver that provides
// direct hardware access for RGB RAM control.
//=============================================================================

#include "SMBusBridge.h"
#include "../core/DryRunMode.h"
#include <Windows.h>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace OCRGB {

// Helper for SMBus logging
static std::string FormatSMBusOp(uint8_t addr, uint8_t reg, const char* extra = nullptr) {
    std::ostringstream ss;
    ss << "addr=0x" << std::hex << std::setw(2) << std::setfill('0') << (int)addr;
    ss << " reg=0x" << std::setw(2) << (int)reg;
    if (extra) ss << " " << extra;
    return ss.str();
}

//-----------------------------------------------------------------------------
// PawnIO Function Types
//-----------------------------------------------------------------------------
typedef int (*PawnIO_Init_t)(const char* driverPath);
typedef void (*PawnIO_Shutdown_t)(void);
typedef int (*PawnIO_SMBusWriteByte_t)(uint8_t addr, uint8_t reg, uint8_t value);
typedef int (*PawnIO_SMBusWriteWord_t)(uint8_t addr, uint8_t reg, uint16_t value);
typedef int (*PawnIO_SMBusWriteBlock_t)(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len);
typedef int (*PawnIO_SMBusReadByte_t)(uint8_t addr, uint8_t reg, uint8_t* value);
typedef int (*PawnIO_SMBusReadWord_t)(uint8_t addr, uint8_t reg, uint16_t* value);
typedef int (*PawnIO_SMBusReadBlock_t)(uint8_t addr, uint8_t reg, uint8_t* buffer, size_t len);

// Global function pointers
static PawnIO_Init_t pawnio_init = nullptr;
static PawnIO_Shutdown_t pawnio_shutdown = nullptr;
static PawnIO_SMBusWriteByte_t pawnio_write_byte = nullptr;
static PawnIO_SMBusWriteWord_t pawnio_write_word = nullptr;
static PawnIO_SMBusWriteBlock_t pawnio_write_block = nullptr;
static PawnIO_SMBusReadByte_t pawnio_read_byte = nullptr;
static PawnIO_SMBusReadWord_t pawnio_read_word = nullptr;
static PawnIO_SMBusReadBlock_t pawnio_read_block = nullptr;

//-----------------------------------------------------------------------------
// SMBusBridge Implementation
//-----------------------------------------------------------------------------

SMBusBridge::SMBusBridge() = default;

SMBusBridge::~SMBusBridge() {
    Shutdown();
    UnloadPawnIO();
}

bool SMBusBridge::IsPawnIOAvailable() {
    // Check if PawnIOLib.dll exists
    HMODULE hLib = LoadLibraryA("PawnIOLib.dll");
    if (hLib) {
        FreeLibrary(hLib);
        return true;
    }
    return false;
}

std::string SMBusBridge::GetPawnIOPath() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);

    // Get directory
    char* lastSlash = strrchr(path, '\\');
    if (lastSlash) {
        *lastSlash = '\0';
    }

    strcat_s(path, "\\SmbusI801.bin");
    return path;
}

bool SMBusBridge::LoadPawnIO() {
    if (m_hPawnLib) return true;

    m_hPawnLib = LoadLibraryA("PawnIOLib.dll");
    if (!m_hPawnLib) {
        SetError("Failed to load PawnIOLib.dll");
        return false;
    }

    // Load function pointers
    pawnio_init = (PawnIO_Init_t)GetProcAddress((HMODULE)m_hPawnLib, "PawnIO_Init");
    pawnio_shutdown = (PawnIO_Shutdown_t)GetProcAddress((HMODULE)m_hPawnLib, "PawnIO_Shutdown");
    pawnio_write_byte = (PawnIO_SMBusWriteByte_t)GetProcAddress((HMODULE)m_hPawnLib, "PawnIO_SMBusWriteByte");
    pawnio_write_word = (PawnIO_SMBusWriteWord_t)GetProcAddress((HMODULE)m_hPawnLib, "PawnIO_SMBusWriteWord");
    pawnio_write_block = (PawnIO_SMBusWriteBlock_t)GetProcAddress((HMODULE)m_hPawnLib, "PawnIO_SMBusWriteBlock");
    pawnio_read_byte = (PawnIO_SMBusReadByte_t)GetProcAddress((HMODULE)m_hPawnLib, "PawnIO_SMBusReadByte");
    pawnio_read_word = (PawnIO_SMBusReadWord_t)GetProcAddress((HMODULE)m_hPawnLib, "PawnIO_SMBusReadWord");
    pawnio_read_block = (PawnIO_SMBusReadBlock_t)GetProcAddress((HMODULE)m_hPawnLib, "PawnIO_SMBusReadBlock");

    // Check required functions
    if (!pawnio_init || !pawnio_shutdown) {
        SetError("PawnIO library missing required functions");
        UnloadPawnIO();
        return false;
    }

    return true;
}

void SMBusBridge::UnloadPawnIO() {
    if (m_hPawnLib) {
        FreeLibrary((HMODULE)m_hPawnLib);
        m_hPawnLib = nullptr;
    }

    pawnio_init = nullptr;
    pawnio_shutdown = nullptr;
    pawnio_write_byte = nullptr;
    pawnio_write_word = nullptr;
    pawnio_write_block = nullptr;
    pawnio_read_byte = nullptr;
    pawnio_read_word = nullptr;
    pawnio_read_block = nullptr;
}

bool SMBusBridge::Initialize() {
    if (m_initialized) return true;

    // Dry-run mode: simulate successful init
    if (DryRun::IsEnabled()) {
        DryRun::Log("SMBusBridge", "Initialize", "simulated");
        m_initialized = true;
        m_dryRunMode = true;
        ClearError();
        return true;
    }

    if (!LoadPawnIO()) {
        return false;
    }

    // Initialize PawnIO with driver path
    std::string driverPath = GetPawnIOPath();
    int result = pawnio_init(driverPath.c_str());
    if (result != 0) {
        SetError("Failed to initialize PawnIO driver");
        return false;
    }

    m_initialized = true;
    ClearError();
    return true;
}

void SMBusBridge::Shutdown() {
    if (!m_initialized) return;

    if (m_dryRunMode) {
        DryRun::Log("SMBusBridge", "Shutdown", "");
        m_initialized = false;
        m_dryRunMode = false;
        return;
    }

    if (pawnio_shutdown) {
        pawnio_shutdown();
    }

    m_initialized = false;
}

bool SMBusBridge::Open(const DeviceAddress& address) {
    if (address.protocol != ProtocolType::SMBus && address.protocol != ProtocolType::I2C) {
        SetError("Invalid protocol type for SMBus bridge");
        return false;
    }

    if (!Initialize()) {
        return false;
    }

    m_currentDeviceAddr = address.deviceAddress;
    return true;
}

void SMBusBridge::Close() {
    m_currentDeviceAddr = 0;
}

bool SMBusBridge::IsOpen() const {
    return m_initialized && m_currentDeviceAddr != 0;
}

bool SMBusBridge::IsReady() const {
    return m_initialized;
}

bool SMBusBridge::Write(const uint8_t* data, size_t length) {
    if (!IsOpen() || length < 2) {
        return false;
    }

    // First byte is register, rest is data
    return WriteBlock(m_currentDeviceAddr, data[0], &data[1], length - 1);
}

int SMBusBridge::Read(uint8_t* buffer, size_t length, int /* timeoutMs */) {
    if (!IsOpen()) {
        return -1;
    }

    if (ReadBlock(m_currentDeviceAddr, 0x00, buffer, length)) {
        return (int)length;
    }

    return -1;
}

bool SMBusBridge::WriteByte(uint8_t deviceAddr, uint8_t reg, uint8_t value) {
    // Dry-run mode
    if (m_dryRunMode) {
        std::ostringstream ss;
        ss << "value=0x" << std::hex << std::setw(2) << std::setfill('0') << (int)value;
        DryRun::Log("SMBusBridge", "WriteByte", FormatSMBusOp(deviceAddr, reg, ss.str().c_str()));
        return true;
    }

    if (!m_initialized || !pawnio_write_byte) {
        SetError("SMBus not initialized");
        return false;
    }

    int result = pawnio_write_byte(deviceAddr, reg, value);
    if (result != 0) {
        SetError("SMBus write byte failed");
        return false;
    }

    return true;
}

bool SMBusBridge::WriteWord(uint8_t deviceAddr, uint8_t reg, uint16_t value) {
    if (!m_initialized || !pawnio_write_word) {
        SetError("SMBus not initialized");
        return false;
    }

    int result = pawnio_write_word(deviceAddr, reg, value);
    if (result != 0) {
        SetError("SMBus write word failed");
        return false;
    }

    return true;
}

bool SMBusBridge::WriteBlock(uint8_t deviceAddr, uint8_t reg, const uint8_t* data, size_t length) {
    // Dry-run mode
    if (m_dryRunMode) {
        std::ostringstream ss;
        ss << "len=" << length;
        DryRun::Log("SMBusBridge", "WriteBlock", FormatSMBusOp(deviceAddr, reg, ss.str().c_str()));
        return true;
    }

    if (!m_initialized) {
        SetError("SMBus not initialized");
        return false;
    }

    // If block write is available, use it
    if (pawnio_write_block) {
        int result = pawnio_write_block(deviceAddr, reg, data, length);
        return result == 0;
    }

    // Fallback: write bytes individually
    if (!pawnio_write_byte) {
        SetError("SMBus write not available");
        return false;
    }

    for (size_t i = 0; i < length; i++) {
        if (pawnio_write_byte(deviceAddr, reg + (uint8_t)i, data[i]) != 0) {
            SetError("SMBus write byte failed");
            return false;
        }
    }

    return true;
}

bool SMBusBridge::QuickCommand(uint8_t /* deviceAddr */, bool /* read */) {
    // Not implemented in PawnIO
    SetError("Quick command not supported");
    return false;
}

bool SMBusBridge::ReadByte(uint8_t deviceAddr, uint8_t reg, uint8_t& value) {
    // Dry-run mode: return dummy value
    if (m_dryRunMode) {
        DryRun::Log("SMBusBridge", "ReadByte", FormatSMBusOp(deviceAddr, reg));
        value = 0x00;
        return true;
    }

    if (!m_initialized || !pawnio_read_byte) {
        SetError("SMBus not initialized");
        return false;
    }

    int result = pawnio_read_byte(deviceAddr, reg, &value);
    if (result != 0) {
        SetError("SMBus read byte failed");
        return false;
    }

    return true;
}

bool SMBusBridge::ReadWord(uint8_t deviceAddr, uint8_t reg, uint16_t& value) {
    if (!m_initialized || !pawnio_read_word) {
        SetError("SMBus not initialized");
        return false;
    }

    int result = pawnio_read_word(deviceAddr, reg, &value);
    if (result != 0) {
        SetError("SMBus read word failed");
        return false;
    }

    return true;
}

bool SMBusBridge::ReadBlock(uint8_t deviceAddr, uint8_t reg, uint8_t* buffer, size_t length) {
    if (!m_initialized) {
        SetError("SMBus not initialized");
        return false;
    }

    // If block read is available, use it
    if (pawnio_read_block) {
        int result = pawnio_read_block(deviceAddr, reg, buffer, length);
        return result == 0;
    }

    // Fallback: read bytes individually
    if (!pawnio_read_byte) {
        SetError("SMBus read not available");
        return false;
    }

    for (size_t i = 0; i < length; i++) {
        if (pawnio_read_byte(deviceAddr, reg + (uint8_t)i, &buffer[i]) != 0) {
            SetError("SMBus read byte failed");
            return false;
        }
    }

    return true;
}

int SMBusBridge::ScanBus(std::vector<uint8_t>& addresses) {
    addresses.clear();

    if (!m_initialized) {
        return 0;
    }

    // Dry-run mode: return simulated G.Skill RAM addresses
    if (m_dryRunMode) {
        DryRun::Log("SMBusBridge", "ScanBus", "simulated scan 0x08-0x77");
        addresses.push_back(0x58);  // Simulated G.Skill slot A1
        addresses.push_back(0x59);  // Simulated G.Skill slot A2
        return (int)addresses.size();
    }

    // Scan typical SMBus address range (0x08 - 0x77)
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        if (ProbeAddress(addr)) {
            addresses.push_back(addr);
        }
    }

    return (int)addresses.size();
}

bool SMBusBridge::ProbeAddress(uint8_t deviceAddr) {
    if (!m_initialized || !pawnio_read_byte) {
        return false;
    }

    uint8_t dummy;
    return pawnio_read_byte(deviceAddr, 0x00, &dummy) == 0;
}

} // namespace OCRGB
