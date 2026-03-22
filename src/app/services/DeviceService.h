#pragma once
//=============================================================================
// OneClickRGB-Universal - Device Service
//=============================================================================
// Application-layer service for device discovery and orchestration.
// This service coordinates between Scanner, Pipeline, and Registry.
//=============================================================================

#include "../../core/DeviceRegistry.h"
#include "../../scanner/HardwareScanner.h"
#include "../pipeline/DevicePipeline.h"
#include "../config/DeviceConfiguration.h"

namespace OCRGB {
namespace App {

//-----------------------------------------------------------------------------
// Device Service
//-----------------------------------------------------------------------------
class DeviceService {
public:
    DeviceService() = default;
    ~DeviceService() = default;

    //=========================================================================
    // Discovery and Registration
    //=========================================================================

    /// Discover devices and register them with the registry
    /// @return Number of devices discovered and registered
    int DiscoverAndRegister();

    /// Discover devices with a specific profile
    /// @param profileId Profile to apply to discovered devices
    /// @return Number of devices discovered and registered
    int DiscoverAndRegisterWithProfile(const std::string& profileId);

    //=========================================================================
    // Pipeline Operations
    //=========================================================================

    /// Run the provisioning pipeline on a device
    /// @param device Device to provision
    /// @param profileId Profile to apply
    /// @return Result of the pipeline execution
    Result ProvisionDevice(IDevice& device, const std::string& profileId);

    /// Re-provision all registered devices
    /// @param profileId Profile to apply
    /// @return Number of devices successfully re-provisioned
    int ReprovisionAll(const std::string& profileId);

    //=========================================================================
    // Configuration
    //=========================================================================

    /// Set the bundle version for new provisions
    void SetBundleVersion(const std::string& version) { m_bundleVersion = version; }

    /// Get the current bundle version
    const std::string& GetBundleVersion() const { return m_bundleVersion; }

private:
    HardwareScanner m_scanner;
    DevicePipeline m_pipeline;
    std::string m_bundleVersion = "1.0.0";
};

} // namespace App
} // namespace OCRGB
