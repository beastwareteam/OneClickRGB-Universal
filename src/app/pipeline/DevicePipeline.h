#pragma once

#include "../config/DeviceConfiguration.h"
#include "../../devices/IDevice.h"
#include <vector>

namespace OCRGB {
namespace App {

struct PipelineContext {
    DeviceInfo detectedDevice;
    DeviceConfiguration config;
    bool applied = false;
    bool verified = false;
};

class DevicePipeline {
public:
    Result Fingerprint(PipelineContext& context) const;
    Result ResolveProfile(PipelineContext& context) const;
    Result BuildDevicePlan(PipelineContext& context) const;
    Result ApplyDevicePlan(PipelineContext& context, IDevice& device) const;
    Result VerifyAndPersist(PipelineContext& context, const IDevice& device) const;

    Result Run(PipelineContext& context, IDevice& device) const;
};

} // namespace App
} // namespace OCRGB
