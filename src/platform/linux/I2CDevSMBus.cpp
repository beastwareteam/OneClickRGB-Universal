//=============================================================================
// OneClickRGB-Universal - I2C-Dev SMBus Implementation
//=============================================================================

#ifdef OCRGB_PLATFORM_LINUX

#include "I2CDevSMBus.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <cstring>

// SMBus function definitions (if not in header)
#ifndef I2C_SMBUS_READ
#define I2C_SMBUS_READ  1
#define I2C_SMBUS_WRITE 0
#define I2C_SMBUS_BYTE_DATA     2
#define I2C_SMBUS_WORD_DATA     3
#define I2C_SMBUS_BLOCK_DATA    5
#endif

namespace OCRGB {
namespace Platform {

//=============================================================================
// Helper: i2c_smbus_access (if not available in system headers)
//=============================================================================

static inline int i2c_smbus_access(int fd, char read_write, uint8_t command,
                                    int size, union i2c_smbus_data* data) {
    struct i2c_smbus_ioctl_data args;
    args.read_write = read_write;
    args.command = command;
    args.size = size;
    args.data = data;
    return ioctl(fd, I2C_SMBUS, &args);
}

static inline int i2c_smbus_write_byte_data(int fd, uint8_t command, uint8_t value) {
    union i2c_smbus_data data;
    data.byte = value;
    return i2c_smbus_access(fd, I2C_SMBUS_WRITE, command, I2C_SMBUS_BYTE_DATA, &data);
}

static inline int i2c_smbus_read_byte_data(int fd, uint8_t command) {
    union i2c_smbus_data data;
    if (i2c_smbus_access(fd, I2C_SMBUS_READ, command, I2C_SMBUS_BYTE_DATA, &data) < 0) {
        return -1;
    }
    return data.byte;
}

static inline int i2c_smbus_write_word_data(int fd, uint8_t command, uint16_t value) {
    union i2c_smbus_data data;
    data.word = value;
    return i2c_smbus_access(fd, I2C_SMBUS_WRITE, command, I2C_SMBUS_WORD_DATA, &data);
}

static inline int i2c_smbus_read_word_data(int fd, uint8_t command) {
    union i2c_smbus_data data;
    if (i2c_smbus_access(fd, I2C_SMBUS_READ, command, I2C_SMBUS_WORD_DATA, &data) < 0) {
        return -1;
    }
    return data.word;
}

//=============================================================================
// Implementation
//=============================================================================

I2CDevSMBus::I2CDevSMBus() {
    // Check if any i2c devices exist
    auto buses = GetAvailableBuses();
    m_available = !buses.empty();
}

I2CDevSMBus::~I2CDevSMBus() {
    Shutdown();
}

bool I2CDevSMBus::IsAvailable() const {
    return m_available;
}

bool I2CDevSMBus::RequiresElevation() const {
    // Check if we can access /dev/i2c-*
    auto buses = GetAvailableBuses();
    if (buses.empty()) {
        return true;
    }

    std::string path = "/dev/i2c-" + std::to_string(buses[0]);
    return access(path.c_str(), R_OK | W_OK) != 0;
}

std::string I2CDevSMBus::GetDriverInfo() const {
    if (!m_available) {
        return "No I2C devices found";
    }

    auto buses = GetAvailableBuses();
    std::ostringstream ss;
    ss << "i2c-dev, buses: ";
    for (size_t i = 0; i < buses.size() && i < 5; i++) {
        if (i > 0) ss << ", ";
        ss << buses[i];
    }
    if (buses.size() > 5) {
        ss << " (+" << (buses.size() - 5) << " more)";
    }

    return ss.str();
}

std::vector<int> I2CDevSMBus::GetAvailableBuses() const {
    std::vector<int> buses;

    DIR* dir = opendir("/dev");
    if (!dir) {
        return buses;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "i2c-", 4) == 0) {
            int busNum = atoi(entry->d_name + 4);
            buses.push_back(busNum);
        }
    }

    closedir(dir);
    return buses;
}

int I2CDevSMBus::FindSMBusBus() const {
    // Look for common SMBus adapters
    auto buses = GetAvailableBuses();

    for (int bus : buses) {
        std::string path = "/sys/class/i2c-adapter/i2c-" + std::to_string(bus) + "/name";
        std::ifstream file(path);
        if (file.is_open()) {
            std::string name;
            std::getline(file, name);

            // Look for Intel SMBus controllers
            if (name.find("SMBus") != std::string::npos ||
                name.find("I801") != std::string::npos ||
                name.find("PIIX4") != std::string::npos) {
                return bus;
            }
        }
    }

    // Fallback to first available bus
    return buses.empty() ? -1 : buses[0];
}

bool I2CDevSMBus::Initialize() {
    if (m_initialized) {
        return true;
    }

    int busNum = m_busNumber;
    if (busNum < 0) {
        busNum = FindSMBusBus();
    }

    if (busNum < 0) {
        SetError("No SMBus adapter found");
        return false;
    }

    if (!OpenBus(busNum)) {
        return false;
    }

    m_initialized = true;
    ClearError();
    return true;
}

void I2CDevSMBus::Shutdown() {
    CloseBus();
    m_initialized = false;
}

bool I2CDevSMBus::OpenBus(int busNum) {
    CloseBus();

    m_devicePath = "/dev/i2c-" + std::to_string(busNum);
    m_fd = open(m_devicePath.c_str(), O_RDWR);

    if (m_fd < 0) {
        SetError("Failed to open " + m_devicePath + ": " + strerror(errno));
        return false;
    }

    m_busNumber = busNum;
    return true;
}

void I2CDevSMBus::CloseBus() {
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
}

bool I2CDevSMBus::SetSlaveAddress(uint8_t addr) {
    if (m_fd < 0) {
        SetError("Bus not open");
        return false;
    }

    if (ioctl(m_fd, I2C_SLAVE, addr) < 0) {
        SetError("Failed to set slave address: " + std::string(strerror(errno)));
        return false;
    }

    return true;
}

void I2CDevSMBus::SetError(const std::string& error) {
    m_lastError = error;
}

void I2CDevSMBus::SetBusNumber(int busNum) {
    if (m_initialized) {
        Shutdown();
    }
    m_busNumber = busNum;
}

//=============================================================================
// Write Operations
//=============================================================================

bool I2CDevSMBus::WriteByte(uint8_t deviceAddr, uint8_t reg, uint8_t value) {
    if (!m_initialized) {
        SetError("SMBus not initialized");
        return false;
    }

    if (!SetSlaveAddress(deviceAddr)) {
        return false;
    }

    if (i2c_smbus_write_byte_data(m_fd, reg, value) < 0) {
        SetError("SMBus write byte failed: " + std::string(strerror(errno)));
        return false;
    }

    return true;
}

bool I2CDevSMBus::WriteWord(uint8_t deviceAddr, uint8_t reg, uint16_t value) {
    if (!m_initialized) {
        SetError("SMBus not initialized");
        return false;
    }

    if (!SetSlaveAddress(deviceAddr)) {
        return false;
    }

    if (i2c_smbus_write_word_data(m_fd, reg, value) < 0) {
        SetError("SMBus write word failed: " + std::string(strerror(errno)));
        return false;
    }

    return true;
}

bool I2CDevSMBus::WriteBlock(uint8_t deviceAddr, uint8_t reg,
                              const uint8_t* data, size_t length) {
    if (!m_initialized) {
        SetError("SMBus not initialized");
        return false;
    }

    if (!SetSlaveAddress(deviceAddr)) {
        return false;
    }

    // Write bytes individually (block write may not be supported)
    for (size_t i = 0; i < length; i++) {
        if (i2c_smbus_write_byte_data(m_fd, reg + i, data[i]) < 0) {
            SetError("SMBus write block failed at offset " + std::to_string(i));
            return false;
        }
    }

    return true;
}

//=============================================================================
// Read Operations
//=============================================================================

bool I2CDevSMBus::ReadByte(uint8_t deviceAddr, uint8_t reg, uint8_t& value) {
    if (!m_initialized) {
        SetError("SMBus not initialized");
        return false;
    }

    if (!SetSlaveAddress(deviceAddr)) {
        return false;
    }

    int result = i2c_smbus_read_byte_data(m_fd, reg);
    if (result < 0) {
        SetError("SMBus read byte failed: " + std::string(strerror(errno)));
        return false;
    }

    value = static_cast<uint8_t>(result);
    return true;
}

bool I2CDevSMBus::ReadWord(uint8_t deviceAddr, uint8_t reg, uint16_t& value) {
    if (!m_initialized) {
        SetError("SMBus not initialized");
        return false;
    }

    if (!SetSlaveAddress(deviceAddr)) {
        return false;
    }

    int result = i2c_smbus_read_word_data(m_fd, reg);
    if (result < 0) {
        SetError("SMBus read word failed: " + std::string(strerror(errno)));
        return false;
    }

    value = static_cast<uint16_t>(result);
    return true;
}

bool I2CDevSMBus::ReadBlock(uint8_t deviceAddr, uint8_t reg,
                             uint8_t* buffer, size_t length) {
    if (!m_initialized) {
        SetError("SMBus not initialized");
        return false;
    }

    if (!SetSlaveAddress(deviceAddr)) {
        return false;
    }

    // Read bytes individually
    for (size_t i = 0; i < length; i++) {
        int result = i2c_smbus_read_byte_data(m_fd, reg + i);
        if (result < 0) {
            SetError("SMBus read block failed at offset " + std::to_string(i));
            return false;
        }
        buffer[i] = static_cast<uint8_t>(result);
    }

    return true;
}

//=============================================================================
// Bus Scanning
//=============================================================================

std::vector<uint8_t> I2CDevSMBus::ScanBus() {
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

bool I2CDevSMBus::ProbeAddress(uint8_t deviceAddr) {
    if (!m_initialized || m_fd < 0) {
        return false;
    }

    if (!SetSlaveAddress(deviceAddr)) {
        return false;
    }

    // Try a quick read to probe
    int result = i2c_smbus_read_byte_data(m_fd, 0x00);
    return result >= 0;
}

} // namespace Platform
} // namespace OCRGB

#endif // OCRGB_PLATFORM_LINUX
