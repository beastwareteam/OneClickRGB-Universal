#include "DeviceConfiguration.h"

namespace OCRGB {
namespace App {

void DeviceConfiguration::SetupFromBundle(const std::string& bundleVersion, const std::string& profileId) {
    m_bundleVersion = bundleVersion;
    m_profileId = profileId;
}

void DeviceConfiguration::SetupFromHardwareMatch(const DeviceInfo& info) {
    m_deviceKey = info.vendor + ":" + info.model + ":" + info.id;
}

void DeviceConfiguration::SetResolvedProfile(const ResolvedProfile& profile) {
    m_profile = profile;
    if (m_profileId.empty()) {
        m_profileId = profile.id;
    }
}

const std::string& DeviceConfiguration::GetBundleVersion() const {
    return m_bundleVersion;
}

const std::string& DeviceConfiguration::GetProfileId() const {
    return m_profileId;
}

const std::string& DeviceConfiguration::GetDeviceKey() const {
    return m_deviceKey;
}

DeviceMode DeviceConfiguration::GetMode() const {
    return m_profile.mode;
}

RGB DeviceConfiguration::GetDefaultColor() const {
    return m_profile.defaultColor;
}

uint8_t DeviceConfiguration::GetBrightness() const {
    return m_profile.brightness;
}

uint8_t DeviceConfiguration::GetSpeed() const {
    return m_profile.speed;
}

} // namespace App
} // namespace OCRGB
