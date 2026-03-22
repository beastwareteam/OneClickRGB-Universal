#pragma once

#include "../../core/Types.h"
#include <string>
#include <vector>

namespace OCRGB {
namespace App {

struct ResolvedProfile {
    std::string id;
    int priority = 0;
    DeviceMode mode = DeviceMode::Static;
    RGB defaultColor = RGB(0, 170, 255);
    uint8_t brightness = 100;
    uint8_t speed = 50;
};

class DeviceConfiguration {
public:
    DeviceConfiguration() = default;

    void SetupFromBundle(const std::string& bundleVersion, const std::string& profileId);
    void SetupFromHardwareMatch(const DeviceInfo& info);
    void SetResolvedProfile(const ResolvedProfile& profile);

    const std::string& GetBundleVersion() const;
    const std::string& GetProfileId() const;
    const std::string& GetDeviceKey() const;
    DeviceMode GetMode() const;
    RGB GetDefaultColor() const;
    uint8_t GetBrightness() const;
    uint8_t GetSpeed() const;

private:
    std::string m_bundleVersion;
    std::string m_profileId;
    std::string m_deviceKey;
    ResolvedProfile m_profile;
};

} // namespace App
} // namespace OCRGB
