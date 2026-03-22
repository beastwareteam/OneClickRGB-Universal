#include "DevicePipeline.h"

namespace OCRGB {
namespace App {

Result DevicePipeline::Fingerprint(PipelineContext& context) const {
    if (context.detectedDevice.id.empty()) {
        return Result{ResultCode::InvalidParameter, "Detected device is empty"};
    }

    context.config.SetupFromHardwareMatch(context.detectedDevice);
    return Result::Success();
}

Result DevicePipeline::ResolveProfile(PipelineContext& context) const {
    if (context.config.GetProfileId().empty()) {
        ResolvedProfile fallback;
        fallback.id = "default-auto";
        fallback.priority = 1;
        context.config.SetResolvedProfile(fallback);
    }

    return Result::Success();
}

Result DevicePipeline::BuildDevicePlan(PipelineContext& context) const {
    if (context.config.GetDeviceKey().empty()) {
        return Result{ResultCode::Error, "Device key is missing after fingerprint"};
    }

    return Result::Success();
}

Result DevicePipeline::ApplyDevicePlan(PipelineContext& context, IDevice& device) const {
    Result colorResult = device.SetColor(context.config.GetDefaultColor());
    if (colorResult.IsError()) {
        return colorResult;
    }

    Result modeResult = device.SetMode(context.config.GetMode());
    if (modeResult.IsError()) {
        return modeResult;
    }

    Result brightnessResult = device.SetBrightness(context.config.GetBrightness());
    if (brightnessResult.IsError()) {
        return brightnessResult;
    }

    Result speedResult = device.SetSpeed(context.config.GetSpeed());
    if (speedResult.IsError()) {
        return speedResult;
    }

    Result applyResult = device.Apply();
    if (applyResult.IsError()) {
        return applyResult;
    }

    context.applied = true;
    return Result::Success();
}

Result DevicePipeline::VerifyAndPersist(PipelineContext& context, const IDevice& device) const {
    if (!context.applied) {
        return Result{ResultCode::Error, "Configuration was not applied"};
    }

    if (!device.IsReady()) {
        return Result{ResultCode::NotConnected, "Device is not ready after apply"};
    }

    context.verified = true;
    return Result::Success();
}

Result DevicePipeline::Run(PipelineContext& context, IDevice& device) const {
    Result fingerprintResult = Fingerprint(context);
    if (fingerprintResult.IsError()) return fingerprintResult;

    Result resolveResult = ResolveProfile(context);
    if (resolveResult.IsError()) return resolveResult;

    Result planResult = BuildDevicePlan(context);
    if (planResult.IsError()) return planResult;

    Result applyResult = ApplyDevicePlan(context, device);
    if (applyResult.IsError()) return applyResult;

    return VerifyAndPersist(context, device);
}

} // namespace App
} // namespace OCRGB
