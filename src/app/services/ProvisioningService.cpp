//=============================================================================
// OneClickRGB-Universal - Provisioning Service Implementation
//=============================================================================

#include "ProvisioningService.h"
#include "../../core/DryRunMode.h"
#include "../../core/DeviceRegistry.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace OCRGB {
namespace App {

//=============================================================================
// Helper Functions
//=============================================================================

std::string StateToString(ProvisioningState state) {
    switch (state) {
        case ProvisioningState::Uninitialized: return "Uninitialized";
        case ProvisioningState::Bootstrap:     return "Bootstrap";
        case ProvisioningState::Fingerprinting: return "Fingerprinting";
        case ProvisioningState::Resolving:     return "Resolving";
        case ProvisioningState::Applying:      return "Applying";
        case ProvisioningState::Verifying:     return "Verifying";
        case ProvisioningState::Healthy:       return "Healthy";
        case ProvisioningState::Degraded:      return "Degraded";
        case ProvisioningState::RollingBack:   return "RollingBack";
        case ProvisioningState::Failed:        return "Failed";
    }
    return "Unknown";
}

static std::string GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

//=============================================================================
// MachineState Serialization
//=============================================================================

std::string MachineState::ToJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"machineId\": \"" << machineId << "\",\n";
    ss << "  \"profileId\": \"" << profileId << "\",\n";
    ss << "  \"configHash\": \"" << configHash << "\",\n";
    ss << "  \"fingerprintHash\": \"" << fingerprintHash << "\",\n";
    ss << "  \"provisionedAt\": \"" << provisionedAt << "\",\n";
    ss << "  \"bundleVersion\": \"" << bundleVersion << "\",\n";
    ss << "  \"previousProfileId\": \"" << previousProfileId << "\",\n";
    ss << "  \"previousConfigHash\": \"" << previousConfigHash << "\",\n";
    ss << "  \"provisionCount\": " << provisionCount << ",\n";
    ss << "  \"rollbackCount\": " << rollbackCount << ",\n";
    ss << "  \"selfHealCount\": " << selfHealCount << "\n";
    ss << "}\n";
    return ss.str();
}

MachineState MachineState::FromJson(const std::string& json) {
    MachineState state;
    // Simple parsing - in production, use proper JSON parser

    auto extractString = [&json](const std::string& key) -> std::string {
        std::string searchKey = "\"" + key + "\": \"";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return "";
        pos += searchKey.length();
        size_t end = json.find("\"", pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    };

    auto extractInt = [&json](const std::string& key) -> int {
        std::string searchKey = "\"" + key + "\": ";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return 0;
        pos += searchKey.length();
        return std::atoi(json.c_str() + pos);
    };

    state.machineId = extractString("machineId");
    state.profileId = extractString("profileId");
    state.configHash = extractString("configHash");
    state.fingerprintHash = extractString("fingerprintHash");
    state.provisionedAt = extractString("provisionedAt");
    state.bundleVersion = extractString("bundleVersion");
    state.previousProfileId = extractString("previousProfileId");
    state.previousConfigHash = extractString("previousConfigHash");
    state.provisionCount = extractInt("provisionCount");
    state.rollbackCount = extractInt("rollbackCount");
    state.selfHealCount = extractInt("selfHealCount");

    return state;
}

//=============================================================================
// ProvisioningService Implementation
//=============================================================================

ProvisioningService::ProvisioningService()
    : m_statePath("machine_state.json") {
}

ProvisioningService::~ProvisioningService() {
    Shutdown();
}

bool ProvisioningService::Initialize() {
    if (m_state != ProvisioningState::Uninitialized) {
        return true;  // Already initialized
    }

    TransitionTo(ProvisioningState::Bootstrap);

    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "Initialize", "starting bootstrap");
    }

    // Load saved state if exists
    LoadState();

    // Load available bundles
    LoadBundles();

    return true;
}

void ProvisioningService::Shutdown() {
    if (m_state == ProvisioningState::Uninitialized) {
        return;
    }

    SaveState();
    TransitionTo(ProvisioningState::Uninitialized);
}

void ProvisioningService::SetBundlePaths(const std::vector<std::string>& paths) {
    m_bundlePaths = paths;
}

void ProvisioningService::SetStatePath(const std::string& path) {
    m_statePath = path;
}

void ProvisioningService::SetStateCallback(StateCallback callback) {
    m_stateCallback = callback;
}

void ProvisioningService::TransitionTo(ProvisioningState newState) {
    ProvisioningState oldState = m_state;
    m_state = newState;

    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "StateChange",
            StateToString(oldState) + " -> " + StateToString(newState));
    }

    if (m_stateCallback) {
        m_stateCallback(oldState, newState);
    }
}

//=============================================================================
// Auto-Provisioning
//=============================================================================

ProvisionResult ProvisioningService::AutoProvision() {
    ProvisionResult result;
    auto startTime = std::chrono::steady_clock::now();

    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "AutoProvision", "starting");
    }

    // 1. Fingerprint
    TransitionTo(ProvisioningState::Fingerprinting);
    m_currentFingerprint = MachineFingerprint::Collect();
    m_machineState.machineId = m_currentFingerprint.GetMachineId();
    m_machineState.fingerprintHash = m_currentFingerprint.GetHash();

    // 2. Resolve profile
    TransitionTo(ProvisioningState::Resolving);
    auto resolveResult = m_resolver.Resolve(m_currentFingerprint);

    if (!resolveResult.matched) {
        result.errors.push_back("No matching profile found");
        result.message = "No profile matches this machine configuration";
        TransitionTo(ProvisioningState::Failed);
        result.finalState = m_state;
        return result;
    }

    result.profileId = resolveResult.profileId;

    // 3. Apply profile
    TransitionTo(ProvisioningState::Applying);
    if (!ApplyProfile(resolveResult.profileId, nullptr)) {
        result.errors.push_back("Failed to apply profile: " + resolveResult.profileId);
        result.message = "Profile application failed";
        TransitionTo(ProvisioningState::Failed);
        result.finalState = m_state;
        return result;
    }

    // 4. Verify
    TransitionTo(ProvisioningState::Verifying);
    if (!VerifyConfiguration()) {
        result.warnings.push_back("Configuration verification incomplete");
        TransitionTo(ProvisioningState::Degraded);
    } else {
        TransitionTo(ProvisioningState::Healthy);
    }

    // Update state
    m_machineState.previousProfileId = m_machineState.profileId;
    m_machineState.previousConfigHash = m_machineState.configHash;
    m_machineState.profileId = resolveResult.profileId;
    m_machineState.configHash = GetCurrentConfigHash();
    m_machineState.provisionedAt = GetTimestamp();
    m_machineState.provisionCount++;

    SaveState();

    auto endTime = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    result.success = (m_state == ProvisioningState::Healthy || m_state == ProvisioningState::Degraded);
    result.message = result.success ? "Provisioning completed" : "Provisioning failed";
    result.finalState = m_state;

    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "AutoProvision",
            "completed profile=" + result.profileId + " state=" + StateToString(m_state));
    }

    return result;
}

ProvisionResult ProvisioningService::ProvisionWithBundle(const ConfigBundle& bundle) {
    // Load rules from bundle
    m_resolver.LoadFromBundle(bundle);

    // Run auto-provision with loaded rules
    return AutoProvision();
}

ProvisionResult ProvisioningService::ProvisionWithProfile(const std::string& profileId) {
    ProvisionResult result;
    auto startTime = std::chrono::steady_clock::now();

    TransitionTo(ProvisioningState::Applying);

    if (!ApplyProfile(profileId, nullptr)) {
        result.errors.push_back("Failed to apply profile: " + profileId);
        TransitionTo(ProvisioningState::Failed);
        result.finalState = m_state;
        return result;
    }

    TransitionTo(ProvisioningState::Verifying);
    if (!VerifyConfiguration()) {
        TransitionTo(ProvisioningState::Degraded);
    } else {
        TransitionTo(ProvisioningState::Healthy);
    }

    // Update state
    m_machineState.previousProfileId = m_machineState.profileId;
    m_machineState.profileId = profileId;
    m_machineState.configHash = GetCurrentConfigHash();
    m_machineState.provisionedAt = GetTimestamp();
    m_machineState.provisionCount++;

    SaveState();

    auto endTime = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    result.success = true;
    result.profileId = profileId;
    result.message = "Profile applied successfully";
    result.finalState = m_state;

    return result;
}

//=============================================================================
// Drift Detection
//=============================================================================

DriftResult ProvisioningService::CheckDrift() {
    DriftResult result;

    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "CheckDrift", "checking configuration");
    }

    // Collect current fingerprint
    MachineFingerprint currentFp = MachineFingerprint::Collect();
    std::string currentHash = currentFp.GetHash();

    result.previousHash = m_machineState.fingerprintHash;
    result.currentHash = currentHash;

    if (m_machineState.fingerprintHash.empty()) {
        result.reason = "No previous fingerprint stored";
        return result;
    }

    if (currentHash != m_machineState.fingerprintHash) {
        result.hasDrift = true;

        // Get detailed changes
        m_savedFingerprint = MachineFingerprint::FromJson("{}");  // Would load from file
        result.changes = currentFp.GetDrift(m_savedFingerprint);

        // Check if re-provisioning is needed
        // Major changes require re-provisioning
        for (const auto& change : result.changes) {
            if (change.find("Mainboard") != std::string::npos ||
                change.find("RGB device count") != std::string::npos) {
                result.requiresReprovisioning = true;
                result.reason = change;
                break;
            }
        }
    }

    // Also check config hash
    std::string currentConfigHash = GetCurrentConfigHash();
    if (!m_machineState.configHash.empty() && currentConfigHash != m_machineState.configHash) {
        result.hasDrift = true;
        result.changes.push_back("Configuration changed since last provision");
    }

    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "CheckDrift",
            "drift=" + std::string(result.hasDrift ? "yes" : "no") +
            " changes=" + std::to_string(result.changes.size()));
    }

    return result;
}

HealthStatus ProvisioningService::GetHealthStatus() const {
    HealthStatus status;

    status.state = m_state;
    status.healthy = (m_state == ProvisioningState::Healthy);
    status.profileId = m_machineState.profileId;
    status.machineId = m_machineState.machineId;
    status.lastProvisionTime = m_machineState.provisionedAt;
    status.configHash = m_machineState.configHash;

    if (m_state == ProvisioningState::Uninitialized) {
        status.issues.push_back("Service not initialized");
    }
    if (m_state == ProvisioningState::Failed) {
        status.issues.push_back("Last provisioning failed");
    }
    if (m_state == ProvisioningState::Degraded) {
        status.issues.push_back("Running in degraded mode");
    }
    if (m_machineState.profileId.empty()) {
        status.issues.push_back("No profile assigned");
    }

    return status;
}

//=============================================================================
// Self-Healing
//=============================================================================

ProvisionResult ProvisioningService::SelfHeal() {
    ProvisionResult result;

    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "SelfHeal", "starting");
    }

    // Check drift
    auto drift = CheckDrift();

    if (!drift.hasDrift) {
        result.success = true;
        result.message = "No drift detected, system is healthy";
        result.finalState = m_state;
        return result;
    }

    if (drift.requiresReprovisioning) {
        // Major change - full re-provisioning
        m_machineState.selfHealCount++;
        return AutoProvision();
    } else {
        // Minor change - re-apply current profile
        m_machineState.selfHealCount++;
        return ProvisionWithProfile(m_machineState.profileId);
    }
}

ProvisionResult ProvisioningService::Rollback() {
    ProvisionResult result;

    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "Rollback", "starting");
    }

    if (m_machineState.previousProfileId.empty()) {
        result.success = false;
        result.message = "No previous profile to rollback to";
        result.errors.push_back("Rollback history empty");
        return result;
    }

    TransitionTo(ProvisioningState::RollingBack);

    std::string targetProfile = m_machineState.previousProfileId;

    if (!ApplyProfile(targetProfile, nullptr)) {
        result.success = false;
        result.errors.push_back("Failed to apply previous profile");
        TransitionTo(ProvisioningState::Failed);
        result.finalState = m_state;
        return result;
    }

    // Update state
    std::string temp = m_machineState.profileId;
    m_machineState.profileId = m_machineState.previousProfileId;
    m_machineState.previousProfileId = temp;
    m_machineState.configHash = GetCurrentConfigHash();
    m_machineState.rollbackCount++;

    TransitionTo(ProvisioningState::Healthy);
    SaveState();

    result.success = true;
    result.profileId = targetProfile;
    result.message = "Rolled back to profile: " + targetProfile;
    result.finalState = m_state;

    return result;
}

//=============================================================================
// State Management
//=============================================================================

bool ProvisioningService::SaveState() {
    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "SaveState", m_statePath);
        return true;
    }

    std::ofstream file(m_statePath);
    if (!file.is_open()) {
        return false;
    }

    file << m_machineState.ToJson();
    return true;
}

bool ProvisioningService::LoadState() {
    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "LoadState", m_statePath);
        return true;
    }

    std::ifstream file(m_statePath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    m_machineState = MachineState::FromJson(buffer.str());

    return true;
}

void ProvisioningService::ClearState() {
    m_machineState = MachineState();

    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "ClearState", "");
    }
}

//=============================================================================
// Internal Helpers
//=============================================================================

bool ProvisioningService::LoadBundles() {
    // Load bundles from configured paths
    for (const auto& path : m_bundlePaths) {
        ConfigBundle bundle;
        if (ConfigBundleParser::ParseFile(path, bundle)) {
            m_resolver.LoadFromBundle(bundle);
        }
    }

    // Add default profile if none loaded
    if (m_resolver.GetRuleCount() == 0) {
        m_resolver.SetDefaultProfile("default");
    }

    return true;
}

bool ProvisioningService::ApplyProfile(const std::string& profileId, const ConfigBundle* /* bundle */) {
    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "ApplyProfile", "profile=" + profileId);
        return true;
    }

    // In a full implementation, this would:
    // 1. Look up the profile in loaded bundles
    // 2. Execute the profile's actions
    // 3. Apply settings to devices via DeviceRegistry

    // For now, just log success
    return true;
}

bool ProvisioningService::VerifyConfiguration() {
    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "VerifyConfiguration", "checking");
        return true;
    }

    // In a full implementation, this would:
    // 1. Query current device states
    // 2. Compare with expected configuration
    // 3. Report any discrepancies

    return true;
}

std::string ProvisioningService::GetCurrentConfigHash() const {
    // Generate hash from current device configuration
    std::ostringstream ss;
    ss << m_machineState.profileId << "|";
    ss << m_machineState.machineId << "|";
    ss << GetTimestamp();

    // Simple hash (FNV-1a)
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (char c : ss.str()) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 0x100000001b3ULL;
    }

    std::ostringstream hashStr;
    hashStr << std::hex << hash;
    return hashStr.str();
}

bool ProvisioningService::ExecuteProfileActions(const ProfileConfig& profile) {
    if (DryRun::IsEnabled()) {
        DryRun::Log("ProvisioningService", "ExecuteProfileActions",
            "profile=" + profile.id + " actions=" + std::to_string(profile.actions.size()));
    }

    for (const auto& action : profile.actions) {
        // Execute each action
        // Actions are: set-mode, set-color, set-brightness, apply

        if (DryRun::IsEnabled()) {
            DryRun::Log("ProvisioningService", "ExecuteAction",
                action.operation + "=" + action.value);
        }

        // In full implementation:
        // - Parse action operation
        // - Apply to DeviceRegistry
    }

    return true;
}

} // namespace App
} // namespace OCRGB
