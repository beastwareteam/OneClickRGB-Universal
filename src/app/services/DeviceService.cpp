#include "DeviceService.h"

namespace OCRGB {
namespace App {

int DeviceService::DiscoverAndRegister() {
    return DiscoverAndRegisterWithProfile("default-auto");
}

int DeviceService::DiscoverAndRegisterWithProfile(const std::string& profileId) {
    auto results = m_scanner.QuickScan();

    int count = 0;
    for (const auto& result : results) {
        if (result.isKnown) {
            DevicePtr device = m_scanner.CreateDevice(result);
            if (device) {
                if (device->Initialize().IsSuccess()) {
                    Result provisionResult = ProvisionDevice(*device, profileId);
                    if (provisionResult.IsSuccess()) {
                        DeviceRegistry::Instance().RegisterDevice(device);
                        count++;
                    }
                }
            }
        }
    }

    return count;
}

Result DeviceService::ProvisionDevice(IDevice& device, const std::string& profileId) {
    PipelineContext context;
    context.detectedDevice = device.GetInfo();
    context.config.SetupFromBundle(m_bundleVersion, profileId);

    return m_pipeline.Run(context, device);
}

int DeviceService::ReprovisionAll(const std::string& profileId) {
    auto devices = DeviceRegistry::Instance().GetAllDevices();

    int count = 0;
    for (auto& device : devices) {
        if (device && device->IsReady()) {
            Result result = ProvisionDevice(*device, profileId);
            if (result.IsSuccess()) {
                count++;
            }
        }
    }

    return count;
}

} // namespace App
} // namespace OCRGB
