//=============================================================================
// OneClickRGB-Universal - GPIO Bridge Implementation
//=============================================================================

#include "GPIOBridge.h"

#ifdef OCRGB_PLATFORM_LINUX

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <cstring>
#include <fstream>
#include <sstream>

// Optional: rpi_ws281x library
#ifdef HAVE_WS281X
#include <ws2811.h>
#endif

// Optional: pigpio library
#ifdef HAVE_PIGPIO
#include <pigpio.h>
#endif

namespace OCRGB {

//=============================================================================
// WS2812B Bridge Implementation
//=============================================================================

WS2812Bridge::WS2812Bridge() = default;

WS2812Bridge::~WS2812Bridge() {
    Shutdown();
}

bool WS2812Bridge::Initialize(const GPIOConfig& config) {
#ifdef HAVE_WS281X
    if (m_initialized) {
        return true;
    }

    ws2811_t* ledstring = new ws2811_t();
    memset(ledstring, 0, sizeof(ws2811_t));

    ledstring->freq = 800000;  // 800kHz for WS2812B
    ledstring->dmanum = config.dmaChannel;

    ledstring->channel[0].gpionum = config.dataPin;
    ledstring->channel[0].count = config.ledCount;
    ledstring->channel[0].invert = 0;
    ledstring->channel[0].brightness = config.globalBrightness;

    // Set strip type based on LED type
    switch (config.ledType) {
        case GPIOLedType::WS2812B:
            ledstring->channel[0].strip_type = WS2812_STRIP;
            break;
        case GPIOLedType::WS2811:
            ledstring->channel[0].strip_type = WS2811_STRIP_RGB;
            break;
        case GPIOLedType::SK6812:
            ledstring->channel[0].strip_type = SK6812_STRIP;
            break;
        case GPIOLedType::SK6812_RGBW:
            ledstring->channel[0].strip_type = SK6812_STRIP_RGBW;
            break;
        default:
            delete ledstring;
            m_lastError = "Unsupported LED type for WS2812 bridge";
            return false;
    }

    // Second channel unused
    ledstring->channel[1].gpionum = 0;
    ledstring->channel[1].count = 0;

    ws2811_return_t ret = ws2811_init(ledstring);
    if (ret != WS2811_SUCCESS) {
        m_lastError = std::string("ws2811_init failed: ") + ws2811_get_return_t_str(ret);
        delete ledstring;
        return false;
    }

    m_ledString = ledstring;
    m_initialized = true;
    return true;
#else
    m_lastError = "WS281x library not available. Install rpi_ws281x.";
    return false;
#endif
}

void WS2812Bridge::Shutdown() {
#ifdef HAVE_WS281X
    if (m_ledString) {
        ws2811_t* ledstring = static_cast<ws2811_t*>(m_ledString);

        // Turn off all LEDs
        for (int i = 0; i < ledstring->channel[0].count; i++) {
            ledstring->channel[0].leds[i] = 0;
        }
        ws2811_render(ledstring);

        ws2811_fini(ledstring);
        delete ledstring;
        m_ledString = nullptr;
    }
    m_initialized = false;
#endif
}

bool WS2812Bridge::IsAvailable() const {
#ifdef HAVE_WS281X
    // Check if /dev/mem is accessible (requires root or cap_sys_rawio)
    return access("/dev/mem", R_OK | W_OK) == 0;
#else
    return false;
#endif
}

bool WS2812Bridge::WriteLeds(const std::vector<RGB>& leds, const GPIOConfig& config) {
#ifdef HAVE_WS281X
    if (!m_initialized || !m_ledString) {
        return false;
    }

    ws2811_t* ledstring = static_cast<ws2811_t*>(m_ledString);

    int count = std::min(static_cast<int>(leds.size()), ledstring->channel[0].count);

    for (int i = 0; i < count; i++) {
        // Convert RGB to 32-bit color (0x00RRGGBB)
        uint32_t color = (leds[i].r << 16) | (leds[i].g << 8) | leds[i].b;
        ledstring->channel[0].leds[i] = color;
    }

    ws2811_return_t ret = ws2811_render(ledstring);
    if (ret != WS2811_SUCCESS) {
        m_lastError = std::string("ws2811_render failed: ") + ws2811_get_return_t_str(ret);
        return false;
    }

    return true;
#else
    return false;
#endif
}

//=============================================================================
// APA102 Bridge Implementation (SPI)
//=============================================================================

APA102Bridge::APA102Bridge() = default;

APA102Bridge::~APA102Bridge() {
    Shutdown();
}

bool APA102Bridge::Initialize(const GPIOConfig& config) {
    if (m_initialized) {
        return true;
    }

    m_spiFd = open(config.spiDevice.c_str(), O_RDWR);
    if (m_spiFd < 0) {
        m_lastError = "Failed to open SPI device: " + config.spiDevice;
        return false;
    }

    // Configure SPI
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = config.spiSpeedHz;

    if (ioctl(m_spiFd, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(m_spiFd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(m_spiFd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        m_lastError = "Failed to configure SPI";
        close(m_spiFd);
        m_spiFd = -1;
        return false;
    }

    m_initialized = true;
    return true;
}

void APA102Bridge::Shutdown() {
    if (m_spiFd >= 0) {
        close(m_spiFd);
        m_spiFd = -1;
    }
    m_initialized = false;
}

bool APA102Bridge::IsAvailable() const {
    return access("/dev/spidev0.0", R_OK | W_OK) == 0;
}

bool APA102Bridge::WriteLeds(const std::vector<RGB>& leds, const GPIOConfig& config) {
    if (!m_initialized || m_spiFd < 0) {
        return false;
    }

    // APA102 protocol:
    // Start frame: 4 bytes of 0x00
    // LED frames: 0xE0 | brightness (5 bits), Blue, Green, Red
    // End frame: (ledCount / 2 / 8 + 1) bytes of 0xFF

    size_t ledCount = leds.size();
    size_t bufferSize = 4 + (ledCount * 4) + ((ledCount / 16) + 1);
    std::vector<uint8_t> buffer(bufferSize, 0);

    // Start frame
    buffer[0] = buffer[1] = buffer[2] = buffer[3] = 0x00;

    // LED data
    size_t offset = 4;
    uint8_t brightness = (config.globalBrightness >> 3) | 0xE0;  // 5-bit brightness

    for (size_t i = 0; i < ledCount; i++) {
        buffer[offset++] = brightness;
        buffer[offset++] = leds[i].b;
        buffer[offset++] = leds[i].g;
        buffer[offset++] = leds[i].r;
    }

    // End frame
    while (offset < bufferSize) {
        buffer[offset++] = 0xFF;
    }

    // Write via SPI
    struct spi_ioc_transfer tr = {};
    tr.tx_buf = reinterpret_cast<unsigned long>(buffer.data());
    tr.len = buffer.size();
    tr.speed_hz = config.spiSpeedHz;
    tr.bits_per_word = 8;

    if (ioctl(m_spiFd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        m_lastError = "SPI write failed";
        return false;
    }

    return true;
}

//=============================================================================
// PWM Bridge Implementation
//=============================================================================

PWMBridge::PWMBridge() = default;

PWMBridge::~PWMBridge() {
    Shutdown();
}

bool PWMBridge::Initialize(const GPIOConfig& config) {
    if (m_initialized) {
        return true;
    }

    m_config = config;

#ifdef HAVE_PIGPIO
    // Try pigpio first
    if (gpioInitialise() >= 0) {
        m_usePigpio = true;

        // Set pins as output
        gpioSetMode(config.redPin, PI_OUTPUT);
        gpioSetMode(config.greenPin, PI_OUTPUT);
        gpioSetMode(config.bluePin, PI_OUTPUT);

        if (config.whitePin >= 0) {
            gpioSetMode(config.whitePin, PI_OUTPUT);
        }

        // Set PWM frequency
        gpioSetPWMfrequency(config.redPin, config.pwmFrequency);
        gpioSetPWMfrequency(config.greenPin, config.pwmFrequency);
        gpioSetPWMfrequency(config.bluePin, config.pwmFrequency);

        m_initialized = true;
        return true;
    }
#endif

    // Fallback to sysfs PWM
    if (!ExportPWM(config.redPin) ||
        !ExportPWM(config.greenPin) ||
        !ExportPWM(config.bluePin)) {
        m_lastError = "Failed to export PWM pins via sysfs";
        return false;
    }

    m_usePigpio = false;
    m_initialized = true;
    return true;
}

void PWMBridge::Shutdown() {
#ifdef HAVE_PIGPIO
    if (m_usePigpio) {
        // Turn off LEDs
        gpioPWM(m_config.redPin, 0);
        gpioPWM(m_config.greenPin, 0);
        gpioPWM(m_config.bluePin, 0);

        gpioTerminate();
    }
#endif
    m_initialized = false;
}

bool PWMBridge::IsAvailable() const {
    // Check if GPIO is accessible
    return access("/sys/class/gpio/export", W_OK) == 0 ||
           access("/dev/pigpio", R_OK) == 0;
}

bool PWMBridge::WriteLeds(const std::vector<RGB>& leds, const GPIOConfig& config) {
    if (!m_initialized || leds.empty()) {
        return false;
    }

    // For PWM, just use the first LED color
    const RGB& color = leds[0];

    SetPWM(config.redPin, color.r);
    SetPWM(config.greenPin, color.g);
    SetPWM(config.bluePin, color.b);

    return true;
}

bool PWMBridge::SetPWM(int pin, uint8_t value) {
#ifdef HAVE_PIGPIO
    if (m_usePigpio) {
        return gpioPWM(pin, value) == 0;
    }
#endif

    return SetPWMViaSysfs(pin, value);
}

bool PWMBridge::ExportPWM(int pin) {
    std::string exportPath = "/sys/class/gpio/export";

    // Check if already exported
    std::string gpioPath = "/sys/class/gpio/gpio" + std::to_string(pin);
    if (access(gpioPath.c_str(), F_OK) == 0) {
        return true;
    }

    std::ofstream exportFile(exportPath);
    if (!exportFile.is_open()) {
        return false;
    }

    exportFile << pin;
    exportFile.close();

    // Set direction to output
    std::string directionPath = gpioPath + "/direction";
    std::ofstream dirFile(directionPath);
    if (dirFile.is_open()) {
        dirFile << "out";
        dirFile.close();
    }

    return true;
}

bool PWMBridge::SetPWMViaSysfs(int pin, uint8_t value) {
    // Note: Software PWM via sysfs gpio is limited
    // For real PWM, use hardware PWM pins or pigpio

    std::string valuePath = "/sys/class/gpio/gpio" + std::to_string(pin) + "/value";
    std::ofstream valueFile(valuePath);

    if (!valueFile.is_open()) {
        return false;
    }

    // Simple on/off threshold (128)
    valueFile << (value > 127 ? "1" : "0");
    valueFile.close();

    return true;
}

//=============================================================================
// GPIO Bridge Factory
//=============================================================================

std::unique_ptr<IGPIOBridge> GPIOBridgeFactory::Create(GPIOLedType type) {
    switch (type) {
        case GPIOLedType::WS2812B:
        case GPIOLedType::WS2811:
        case GPIOLedType::SK6812:
        case GPIOLedType::SK6812_RGBW:
            return std::make_unique<WS2812Bridge>();

        case GPIOLedType::APA102:
            return std::make_unique<APA102Bridge>();

        case GPIOLedType::PWM_RGB:
        case GPIOLedType::PWM_RGBW:
            return std::make_unique<PWMBridge>();

        default:
            return nullptr;
    }
}

} // namespace OCRGB

#else // !OCRGB_PLATFORM_LINUX

namespace OCRGB {

// Stub implementation for non-Linux platforms
std::unique_ptr<IGPIOBridge> GPIOBridgeFactory::Create(GPIOLedType type) {
    return nullptr;
}

} // namespace OCRGB

#endif // OCRGB_PLATFORM_LINUX
