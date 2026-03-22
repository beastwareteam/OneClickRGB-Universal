#pragma once
//=============================================================================
// OneClickRGB-Universal - Profile Resolver
//=============================================================================
// Resolves the best matching profile based on machine fingerprint and rules.
//
// Priority Order (highest wins):
//   1. Explicit machine ID assignment
//   2. Location/Tag rules
//   3. Hardware fingerprint rules (mainboard, GPU, etc.)
//   4. Vendor/DeviceType rules
//   5. Global default profile
//
// Usage:
//   ProfileResolver resolver;
//   resolver.LoadRules("config/profiles.json");
//
//   auto result = resolver.Resolve(fingerprint);
//   if (result.matched) {
//       ApplyProfile(result.profile);
//   }
//
//=============================================================================

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "../fingerprint/MachineFingerprint.h"
#include "../config/ConfigBundleParser.h"

namespace OCRGB {
namespace App {

//=============================================================================
// Rule Types
//=============================================================================

enum class RuleType {
    MachineId,      // Explicit machine assignment
    Tag,            // Location/environment tag
    Mainboard,      // Mainboard manufacturer/product
    Gpu,            // GPU vendor/model
    Ram,            // RAM manufacturer
    DeviceType,     // RGB device type (HID, SMBus)
    Vendor,         // RGB device vendor
    Default         // Fallback
};

struct ProfileRule {
    RuleType type = RuleType::Default;
    int priority = 0;               // Higher = more important
    std::string profileId;          // Profile to apply
    std::string matchValue;         // Value to match against
    std::string matchPattern;       // Optional regex pattern

    // For compound rules
    std::vector<std::string> matchVendors;
    std::vector<std::string> matchDeviceTypes;
};

//=============================================================================
// Resolution Result
//=============================================================================

struct ResolveResult {
    bool matched = false;
    std::string profileId;
    std::string matchReason;        // Why this profile was selected
    int priority = 0;
    RuleType ruleType = RuleType::Default;

    // All candidate profiles considered (for debugging)
    std::vector<std::pair<std::string, int>> candidates;
};

//=============================================================================
// Profile Resolver
//=============================================================================

class ProfileResolver {
public:
    ProfileResolver() = default;

    //=========================================================================
    // Rule Management
    //=========================================================================

    /// Add a rule
    void AddRule(const ProfileRule& rule);

    /// Clear all rules
    void ClearRules();

    /// Load rules from JSON file
    bool LoadRules(const std::string& path);

    /// Load rules from ConfigBundle
    void LoadFromBundle(const ConfigBundle& bundle);

    //=========================================================================
    // Resolution
    //=========================================================================

    /// Resolve best matching profile for a fingerprint
    ResolveResult Resolve(const MachineFingerprint& fingerprint) const;

    /// Resolve with additional context (tags, environment)
    ResolveResult Resolve(const MachineFingerprint& fingerprint,
                          const std::vector<std::string>& tags) const;

    //=========================================================================
    // Convenience Methods
    //=========================================================================

    /// Set default profile (lowest priority fallback)
    void SetDefaultProfile(const std::string& profileId);

    /// Add machine-specific assignment (highest priority)
    void AssignToMachine(const std::string& machineId, const std::string& profileId);

    /// Add tag-based rule
    void AddTagRule(const std::string& tag, const std::string& profileId, int priority = 50);

    /// Add mainboard rule
    void AddMainboardRule(const std::string& manufacturer, const std::string& product,
                          const std::string& profileId, int priority = 30);

    /// Add vendor rule
    void AddVendorRule(const std::vector<std::string>& vendors,
                       const std::string& profileId, int priority = 20);

    //=========================================================================
    // Query
    //=========================================================================

    /// Get all rules
    const std::vector<ProfileRule>& GetRules() const { return m_rules; }

    /// Get rule count
    size_t GetRuleCount() const { return m_rules.size(); }

private:
    std::vector<ProfileRule> m_rules;
    std::string m_defaultProfileId;

    // Matching helpers
    bool MatchesRule(const ProfileRule& rule, const MachineFingerprint& fp,
                     const std::vector<std::string>& tags) const;
    int CalculatePriority(const ProfileRule& rule) const;
};

} // namespace App
} // namespace OCRGB
