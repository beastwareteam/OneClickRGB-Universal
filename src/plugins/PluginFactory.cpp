#include "PluginFactory.h"

#include "asus/AsusAuraController.h"
#include "steelseries/SteelSeriesRival.h"
#include "evision/EVisionKeyboard.h"
#include "gskill/GSkillTridentZ5.h"

// GPIO plugins (Raspberry Pi)
#ifdef OCRGB_PLATFORM_LINUX
#include "gpio/WS2812Controller.h"
#endif

#include <cstring>
#include <memory>

namespace OCRGB {

DevicePtr PluginFactory::Create(const HardwareDB::DeviceDefinition& def, const DeviceAddress* address) {
    if (!def.id) {
        return nullptr;
    }

    if (std::strcmp(def.id, "asus_aura_mainboard") == 0) {
        return std::make_shared<Plugins::AsusAuraController>();
    }

    if (std::strcmp(def.id, "steelseries_rival_600") == 0) {
        return std::make_shared<Plugins::SteelSeriesRival>();
    }

    if (std::strcmp(def.id, "evision_keyboard") == 0) {
        return std::make_shared<Plugins::EVisionKeyboard>();
    }

    if (std::strcmp(def.id, "gskill_trident_z5") == 0) {
        uint8_t smbusAddress = Plugins::GSkill::ADDR_SLOT_A1;
        if (address && address->protocol == ProtocolType::SMBus && address->deviceAddress != 0) {
            smbusAddress = address->deviceAddress;
        }

        return std::make_shared<Plugins::GSkillTridentZ5>(smbusAddress);
    }

#ifdef OCRGB_PLATFORM_LINUX
    // GPIO devices (Raspberry Pi)
    if (std::strcmp(def.id, "rpi_ws2812b") == 0) {
        int ledCount = 60;  // Default
        int gpioPin = 18;   // Default PWM pin

        // TODO: Parse from config
        return std::make_shared<Plugins::WS2812Controller>(ledCount, gpioPin);
    }

    if (std::strcmp(def.id, "rpi_apa102") == 0) {
        int ledCount = 30;
        return std::make_shared<Plugins::APA102Controller>(ledCount);
    }

    if (std::strcmp(def.id, "rpi_pwm_rgb") == 0) {
        return std::make_shared<Plugins::PWMRGBController>();
    }
#endif

    return nullptr;
}

} // namespace OCRGB
