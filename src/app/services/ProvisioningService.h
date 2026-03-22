#pragma once
//=============================================================================
// OneClickRGB-Universal - Provisioning Service
//=============================================================================
// Orchestrates automatic machine configuration and self-healing.
//
// Lifecycle:
//   Bootstrap -> Fingerprint -> Resolve -> Apply -> Verify -> Healthy
//
// Error Path:
//   Apply/Verify Error -> Rollback -> SafeProfile -> RetryWindow
//
// Features:
//   - Auto-provisioning from machine fingerprint
//   - Configuration drift detection
//   - Self-healing with rollback
//   - Persistent state management
//
// Usage:
//   ProvisioningService svc;
//   svc.Initialize();
//
//   auto result = svc.AutoProvision();
//   if (!result.success) {
//       // Handle error
//   }
//
//   // Check for drift
//   auto drift = svc.CheckDrift();
//   if (drift.hasDrift) {
//       svc.SelfHeal();
//   }
//
//=============================================================================

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include "../fingerprint/MachineFingerprint.h"
#include "../config/ConfigBundleParser.h"
#include "ProfileResolver.h"

namespace OCRGB {
namespace App {

//=============================================================================
// State Types
//=============================================================================

enum class ProvisioningState {
    Uninitialized,
    Bootstrap,
    Fingerprinting,
    Resolving,
    Applying,
    Verifying,
    Healthy,
    Degraded,
    RollingBack,
    Failed
};

std::string StateToString(ProvisioningState state);

//=============================================================================
// Result Types
//=============================================================================

struct ProvisionResult {
    bool success = false;
    std::string profileId;
    std::string message;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    ProvisioningState finalState = ProvisioningState::Uninitialized;

    // Timing
    std::chrono::milliseconds duration{0};
};

struct DriftResult {
    bool hasDrift = false;
    std::vector<std::string> changes;
    std::string previousHash;
    std::string currentHash;

    // Recommendation
    bool requiresReprovisioning = false;
    std::string reason;
};

struct HealthStatus {
    bool healthy = false;
    ProvisioningState state = ProvisioningState::Uninitialized;
    std::string profileId;
    std::string machineId;
    std::string lastProvisionTime;
    std::string configHash;

    std::vector<std::string> issues;
};

//=============================================================================
// Persistent State
//=============================================================================

struct MachineState {
    std::string machineId;
    std::string profileId;
    std::string configHash;
    std::string fingerprintHash;
    std::string provisionedAt;
    std::string bundleVersion;

    // For rollback
    std::string previousProfileId;
    std::string previousConfigHash;

    // Metrics
    int provisionCount = 0;
    int rollbackCount = 0;
    int selfHealCount = 0;

    // Serialize/Deserialize
    std::string ToJson() const;
    static MachineState FromJson(const std::string& json);
};

//=============================================================================
// Provisioning Service
//=============================================================================

class ProvisioningService {
public:
    ProvisioningService();
    ~ProvisioningService();

    //=========================================================================
    // Lifecycle
    //=========================================================================

    /// Initialize the service
    bool Initialize();

    /// Shutdown and cleanup
    void Shutdown();

    /// Get current state
    ProvisioningState GetState() const { return m_state; }

    //=========================================================================
    // Configuration
    //=========================================================================

    /// Set bundle search paths
    void SetBundlePaths(const std::vector<std::string>& paths);

    /// Set state persistence path
    void SetStatePath(const std::string& path);

    /// Set callback for state changes
    using StateCallback = std::function<void(ProvisioningState oldState, ProvisioningState newState)>;
    void SetStateCallback(StateCallback callback);

    //=========================================================================
    // Auto-Provisioning
    //=========================================================================

    /// Automatic provisioning from machine fingerprint
    ProvisionResult AutoProvision();

    /// Provision with specific bundle
    ProvisionResult ProvisionWithBundle(const ConfigBundle& bundle);

    /// Provision with specific profile ID
    ProvisionResult ProvisionWithProfile(const std::string& profileId);

    //=========================================================================
    // Drift Detection
    //=========================================================================

    /// Check for configuration drift
    DriftResult CheckDrift();

    /// Get health status
    HealthStatus GetHealthStatus() const;

    //=========================================================================
    // Self-Healing
    //=========================================================================

    /// Attempt to restore healthy state
    ProvisionResult SelfHeal();

    /// Rollback to previous configuration
    ProvisionResult Rollback();

    //=========================================================================
    // State Management
    //=========================================================================

    /// Get current machine state
    const MachineState& GetMachineState() const { return m_machineState; }

    /// Save state to disk
    bool SaveState();

    /// Load state from disk
    bool LoadState();

    /// Clear state (for testing)
    void ClearState();

    //=========================================================================
    // Profile Resolver Access
    //=========================================================================

    ProfileResolver& GetResolver() { return m_resolver; }
    const ProfileResolver& GetResolver() const { return m_resolver; }

private:
    ProvisioningState m_state = ProvisioningState::Uninitialized;
    MachineState m_machineState;
    ProfileResolver m_resolver;
    std::string m_statePath;
    std::vector<std::string> m_bundlePaths;
    StateCallback m_stateCallback;

    MachineFingerprint m_currentFingerprint;
    MachineFingerprint m_savedFingerprint;

    // State transitions
    void TransitionTo(ProvisioningState newState);

    // Internal helpers
    bool LoadBundles();
    bool ApplyProfile(const std::string& profileId, const ConfigBundle* bundle);
    bool VerifyConfiguration();
    std::string GetCurrentConfigHash() const;

    // Profile application
    bool ExecuteProfileActions(const ProfileConfig& profile);
};

} // namespace App
} // namespace OCRGB
