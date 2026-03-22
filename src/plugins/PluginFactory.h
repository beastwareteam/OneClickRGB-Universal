#pragma once

#include "../../build/generated/hardware_config.h"
#include "../devices/IDevice.h"

namespace OCRGB {

class PluginFactory {
public:
    static DevicePtr Create(const HardwareDB::DeviceDefinition& def, const DeviceAddress* address = nullptr);
};

} // namespace OCRGB
