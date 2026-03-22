//=============================================================================
// OneClickRGB-Universal - PawnIO SMBus Implementation
//=============================================================================

#ifdef OCRGB_PLATFORM_WINDOWS

#include "PawnIOSMBus.h"
#include <Windows.h>

namespace OCRGB {
namespace Platform {

//=============================================================================
// PawnIO Function Types
//=============================================================================

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

//=============================================================================
// Implementation
//=============================================================================

PawnIOSMBus::PawnIOSMBus() {
    // Get driver path
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);

    char* lastSlash = strrchr(path, '\\');
    if (lastSlash) {
        *lastSlash = '\0';
    }

    m_driverPath = path;
    m_driverPath += "\\SmbusI801.bin";

    // Check if library exists
    HMODULE hLib = LoadLibraryA("PawnIOLib.dll");
    if (hLib) {
        FreeLibrary(hLib);
        m_available = true;
    }
}

PawnIOSMBus::~PawnIOSMBus() {
    Shutdown();
}

bool PawnIOSMBus::IsAvailable() const {
    return m_available;
}

std::string PawnIOSMBus::GetDriverInfo() const {
    if (!m_available) {
        return "PawnIOLib.dll not found";
    }

    if (m_initialized) {
        return "PawnIO initialized, driver: " + m_driverPath;
    }

    return "PawnIO available, not initialized";
}

bool PawnIOSMBus::LoadLibrary() {
    if (m_hPawnLib) {
        return true;
    }

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

    if (!pawnio_init || !pawnio_shutdown) {
        SetError("PawnIO library missing required functions");
        UnloadLibrary();
        return false;
    }

    return true;
}

void PawnIOSMBus::UnloadLibrary() {
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

bool PawnIOSMBus::Initialize() {
    if (m_initialized) {
        return true;
    }

    if (!LoadLibrary()) {
        return false;
    }

    int result = pawnio_init(m_driverPath.c_str());
    if (result != 0) {
        SetError("Failed to initialize PawnIO driver");
        return false;
    }

    m_initialized = true;
    ClearError();
    return true;
}

void PawnIOSMBus::Shutdown() {
    if (!m_initialized) {
        return;
    }

    if (pawnio_shutdown) {
        pawnio_shutdown();
    }

    UnloadLibrary();
    m_initialized = false;
}

void PawnIOSMBus::SetError(const std::string& error) {
    m_lastError = error;
}

//=============================================================================
// Write Operations
//=============================================================================

bool PawnIOSMBus::WriteByte(uint8_t deviceAddr, uint8_t reg, uint8_t value) {
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

bool PawnIOSMBus::WriteWord(uint8_t deviceAddr, uint8_t reg, uint16_t value) {
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

bool PawnIOSMBus::WriteBlock(uint8_t deviceAddr, uint8_t reg,
                              const uint8_t* data, size_t length) {
    if (!m_initialized) {
        SetError("SMBus not initialized");
        return false;
    }

    if (pawnio_write_block) {
        int result = pawnio_write_block(deviceAddr, reg, data, length);
        return result == 0;
    }

    // Fallback to byte-by-byte
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

//=============================================================================
// Read Operations
//=============================================================================

bool PawnIOSMBus::ReadByte(uint8_t deviceAddr, uint8_t reg, uint8_t& value) {
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

bool PawnIOSMBus::ReadWord(uint8_t deviceAddr, uint8_t reg, uint16_t& value) {
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

bool PawnIOSMBus::ReadBlock(uint8_t deviceAddr, uint8_t reg,
                             uint8_t* buffer, size_t length) {
    if (!m_initialized) {
        SetError("SMBus not initialized");
        return false;
    }

    if (pawnio_read_block) {
        int result = pawnio_read_block(deviceAddr, reg, buffer, length);
        return result == 0;
    }

    // Fallback to byte-by-byte
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

//=============================================================================
// Bus Scanning
//=============================================================================

std::vector<uint8_t> PawnIOSMBus::ScanBus() {
    std::vector<uint8_t> addresses;

    if (!m_initialized) {
        return addresses;
    }

    // Scan typical SMBus range (0x08 - 0x77)
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        if (ProbeAddress(addr)) {
            addresses.push_back(addr);
        }
    }

    return addresses;
}

bool PawnIOSMBus::ProbeAddress(uint8_t deviceAddr) {
    if (!m_initialized || !pawnio_read_byte) {
        return false;
    }

    uint8_t dummy;
    return pawnio_read_byte(deviceAddr, 0x00, &dummy) == 0;
}

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_WINDOWS
