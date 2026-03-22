//=============================================================================
// OneClickRGB-Universal - Profile Resolver Implementation
//=============================================================================

#include "ProfileResolver.h"
#include "../../core/DryRunMode.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace OCRGB {
namespace App {

//=============================================================================
// Rule Management
//=============================================================================

void ProfileResolver::AddRule(const ProfileRule& rule) {
    m_rules.push_back(rule);

    // Keep sorted by priority (descending)
    std::sort(m_rules.begin(), m_rules.end(),
        [](const ProfileRule& a, const ProfileRule& b) {
            return a.priority > b.priority;
        });
}

void ProfileResolver::ClearRules() {
    m_rules.clear();
    m_defaultProfileId.clear();
}

bool ProfileResolver::LoadRules(const std::string& path) {
    // Simple JSON parsing for rules file
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // TODO: Proper JSON parsing
    // For now, this is a placeholder
    // The real implementation would use the JSON parser from ConfigBundleParser

    return true;
}

void ProfileResolver::LoadFromBundle(const ConfigBundle& bundle) {
    for (const auto& profile : bundle.profiles) {
        ProfileRule rule;
        rule.profileId = profile.id;
        rule.priority = profile.priority;
        rule.matchVendors = profile.matchVendors;
        rule.matchDeviceTypes = profile.matchDeviceTypes;

        if (!profile.matchVendors.empty() || !profile.matchDeviceTypes.empty()) {
            rule.type = RuleType::Vendor;
        }

        AddRule(rule);
    }
}

//=============================================================================
// Convenience Methods
//=============================================================================

void ProfileResolver::SetDefaultProfile(const std::string& profileId) {
    m_defaultProfileId = profileId;

    ProfileRule rule;
    rule.type = RuleType::Default;
    rule.priority = 0;
    rule.profileId = profileId;
    AddRule(rule);
}

void ProfileResolver::AssignToMachine(const std::string& machineId, const std::string& profileId) {
    ProfileRule rule;
    rule.type = RuleType::MachineId;
    rule.priority = 100;  // Highest priority
    rule.profileId = profileId;
    rule.matchValue = machineId;
    AddRule(rule);
}

void ProfileResolver::AddTagRule(const std::string& tag, const std::string& profileId, int priority) {
    ProfileRule rule;
    rule.type = RuleType::Tag;
    rule.priority = priority;
    rule.profileId = profileId;
    rule.matchValue = tag;
    AddRule(rule);
}

void ProfileResolver::AddMainboardRule(const std::string& manufacturer, const std::string& product,
                                        const std::string& profileId, int priority) {
    ProfileRule rule;
    rule.type = RuleType::Mainboard;
    rule.priority = priority;
    rule.profileId = profileId;
    rule.matchValue = manufacturer + "/" + product;
    AddRule(rule);
}

void ProfileResolver::AddVendorRule(const std::vector<std::string>& vendors,
                                     const std::string& profileId, int priority) {
    ProfileRule rule;
    rule.type = RuleType::Vendor;
    rule.priority = priority;
    rule.profileId = profileId;
    rule.matchVendors = vendors;
    AddRule(rule);
}

//=============================================================================
// Resolution
//=============================================================================

ResolveResult ProfileResolver::Resolve(const MachineFingerprint& fingerprint) const {
    return Resolve(fingerprint, {});
}

ResolveResult ProfileResolver::Resolve(const MachineFingerprint& fingerprint,
                                        const std::vector<std::string>& tags) const {
    ResolveResult result;

    if (DryRun::IsEnabled()) {
        DryRun::Log("ProfileResolver", "Resolve",
            "machineId=" + fingerprint.GetMachineId() + " rules=" + std::to_string(m_rules.size()));
    }

    // Collect all matching rules
    std::vector<const ProfileRule*> matches;

    for (const auto& rule : m_rules) {
        if (MatchesRule(rule, fingerprint, tags)) {
            matches.push_back(&rule);
            result.candidates.push_back({rule.profileId, rule.priority});
        }
    }

    // Rules are already sorted by priority, so first match is best
    if (!matches.empty()) {
        const ProfileRule* best = matches[0];
        result.matched = true;
        result.profileId = best->profileId;
        result.priority = best->priority;
        result.ruleType = best->type;

        // Generate match reason
        switch (best->type) {
            case RuleType::MachineId:
                result.matchReason = "Machine ID assignment";
                break;
            case RuleType::Tag:
                result.matchReason = "Tag rule: " + best->matchValue;
                break;
            case RuleType::Mainboard:
                result.matchReason = "Mainboard match: " + best->matchValue;
                break;
            case RuleType::Gpu:
                result.matchReason = "GPU match";
                break;
            case RuleType::Ram:
                result.matchReason = "RAM match";
                break;
            case RuleType::Vendor:
                result.matchReason = "Vendor match";
                break;
            case RuleType::DeviceType:
                result.matchReason = "Device type match";
                break;
            case RuleType::Default:
                result.matchReason = "Default profile";
                break;
        }

        if (DryRun::IsEnabled()) {
            DryRun::Log("ProfileResolver", "Matched",
                "profile=" + result.profileId + " reason=" + result.matchReason);
        }
    }

    return result;
}

bool ProfileResolver::MatchesRule(const ProfileRule& rule, const MachineFingerprint& fp,
                                   const std::vector<std::string>& tags) const {
    switch (rule.type) {
        case RuleType::MachineId:
            return fp.GetMachineId() == rule.matchValue;

        case RuleType::Tag:
            for (const auto& tag : tags) {
                if (tag == rule.matchValue) {
                    return true;
                }
            }
            return false;

        case RuleType::Mainboard: {
            std::string mainboardKey = fp.GetMainboard().manufacturer + "/" +
                                        fp.GetMainboard().product;
            return mainboardKey.find(rule.matchValue) != std::string::npos;
        }

        case RuleType::Gpu:
            for (const auto& gpu : fp.GetGpus()) {
                if (gpu.name.find(rule.matchValue) != std::string::npos ||
                    gpu.vendor.find(rule.matchValue) != std::string::npos) {
                    return true;
                }
            }
            return false;

        case RuleType::Ram:
            for (const auto& ram : fp.GetRam()) {
                if (ram.manufacturer.find(rule.matchValue) != std::string::npos) {
                    return true;
                }
            }
            return false;

        case RuleType::Vendor:
            if (!rule.matchVendors.empty()) {
                for (const auto& rgb : fp.GetRgbDevices()) {
                    for (const auto& vendor : rule.matchVendors) {
                        if (rgb.vendor == vendor) {
                            return true;
                        }
                    }
                }
            }
            // Also check device types if specified
            if (!rule.matchDeviceTypes.empty()) {
                for (const auto& rgb : fp.GetRgbDevices()) {
                    for (const auto& dtype : rule.matchDeviceTypes) {
                        if (rgb.type == dtype) {
                            return true;
                        }
                    }
                }
            }
            return rule.matchVendors.empty() && rule.matchDeviceTypes.empty();

        case RuleType::DeviceType:
            for (const auto& rgb : fp.GetRgbDevices()) {
                if (rgb.type == rule.matchValue) {
                    return true;
                }
            }
            return false;

        case RuleType::Default:
            return true;  // Default always matches (lowest priority)
    }

    return false;
}

int ProfileResolver::CalculatePriority(const ProfileRule& rule) const {
    // Base priority from rule type
    int basePriority = 0;

    switch (rule.type) {
        case RuleType::MachineId:   basePriority = 100; break;
        case RuleType::Tag:         basePriority = 50;  break;
        case RuleType::Mainboard:   basePriority = 30;  break;
        case RuleType::Gpu:         basePriority = 25;  break;
        case RuleType::Ram:         basePriority = 25;  break;
        case RuleType::Vendor:      basePriority = 20;  break;
        case RuleType::DeviceType:  basePriority = 15;  break;
        case RuleType::Default:     basePriority = 0;   break;
    }

    // Add rule-specific priority offset
    return basePriority + rule.priority;
}

} // namespace App
} // namespace OCRGB
