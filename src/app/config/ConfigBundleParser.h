#pragma once
//=============================================================================
// OneClickRGB-Universal - ConfigBundle Parser
//=============================================================================
// Parses ConfigBundle JSON files according to config_bundle.schema.json.
// Provides a simple API to load and query bundle data.
//
// Usage:
//   ConfigBundleParser parser;
//   if (parser.LoadFromFile("config.json")) {
//       auto profiles = parser.GetProfiles();
//       auto match = parser.FindMatchingProfile(deviceInfo);
//   }
//
//=============================================================================

#include "../../core/Types.h"
#include "DeviceConfiguration.h"
#include <string>
#include <vector>
#include <map>

namespace OCRGB {
namespace App {

//-----------------------------------------------------------------------------
// ConfigBundle Action
//-----------------------------------------------------------------------------
struct BundleAction {
    std::string op;      // "set-mode", "set-color", "set-brightness", "set-speed", "apply"
    std::string value;   // String value (color as hex, mode name, or numeric string)
};

//-----------------------------------------------------------------------------
// ConfigBundle Profile Match Criteria
//-----------------------------------------------------------------------------
struct ProfileMatch {
    std::string machineId;
    std::vector<std::string> vendors;
    std::vector<std::string> deviceTypes;
    std::vector<std::string> tags;
};

//-----------------------------------------------------------------------------
// ConfigBundle Profile
//-----------------------------------------------------------------------------
struct BundleProfile {
    std::string id;
    int priority = 0;
    ProfileMatch match;
    std::vector<BundleAction> actions;
};

//-----------------------------------------------------------------------------
// ConfigBundle Policies
//-----------------------------------------------------------------------------
struct BundlePolicies {
    bool allowRawPackets = false;
    int maxRetry = 3;
    bool rollbackOnError = true;
};

//-----------------------------------------------------------------------------
// ConfigBundle Integrity
//-----------------------------------------------------------------------------
struct BundleIntegrity {
    std::string hash;       // "sha256:..."
    std::string signature;  // Base64 signature
    std::string keyId;      // Key identifier
};

//-----------------------------------------------------------------------------
// ConfigBundle (parsed result)
//-----------------------------------------------------------------------------
struct ConfigBundle {
    std::string bundleVersion;
    std::string createdAt;
    std::string targetOs;
    std::string targetArch;
    std::vector<BundleProfile> profiles;
    BundlePolicies policies;
    BundleIntegrity integrity;

    bool IsValid() const { return !bundleVersion.empty() && !profiles.empty(); }
};

//-----------------------------------------------------------------------------
// ConfigBundle Parser
//-----------------------------------------------------------------------------
class ConfigBundleParser {
public:
    ConfigBundleParser() = default;

    //=========================================================================
    // Loading
    //=========================================================================

    /// Load bundle from JSON file
    /// @param filePath Path to JSON file
    /// @return true if parsing succeeded
    bool LoadFromFile(const std::string& filePath);

    /// Load bundle from JSON string
    /// @param jsonContent JSON content as string
    /// @return true if parsing succeeded
    bool LoadFromString(const std::string& jsonContent);

    /// Get last error message
    const std::string& GetLastError() const { return m_lastError; }

    //=========================================================================
    // Accessors
    //=========================================================================

    /// Get the parsed bundle
    const ConfigBundle& GetBundle() const { return m_bundle; }

    /// Check if a bundle is loaded
    bool IsLoaded() const { return m_bundle.IsValid(); }

    /// Get all profiles
    const std::vector<BundleProfile>& GetProfiles() const { return m_bundle.profiles; }

    /// Get policies
    const BundlePolicies& GetPolicies() const { return m_bundle.policies; }

    //=========================================================================
    // Profile Matching
    //=========================================================================

    /// Find the best matching profile for a device
    /// @param info Device information to match against
    /// @return Pointer to matching profile, or nullptr if none found
    const BundleProfile* FindMatchingProfile(const DeviceInfo& info) const;

    /// Find the best matching profile by vendor and device type
    const BundleProfile* FindMatchingProfile(const std::string& vendor,
                                              DeviceType deviceType) const;

    /// Convert a BundleProfile to a ResolvedProfile
    ResolvedProfile ToResolvedProfile(const BundleProfile& profile) const;

    //=========================================================================
    // Integrity Verification (optional)
    //=========================================================================

    /// Verify bundle integrity (hash check)
    /// @return true if hash matches content
    bool VerifyIntegrity() const;

private:
    ConfigBundle m_bundle;
    std::string m_lastError;
    std::string m_rawContent;  // For integrity verification

    // Parsing helpers
    bool ParseJson(const std::string& json);
    std::string ExtractString(const std::string& json, const std::string& key) const;
    int ExtractInt(const std::string& json, const std::string& key, int defaultVal = 0) const;
    bool ExtractBool(const std::string& json, const std::string& key, bool defaultVal = false) const;

    // String helpers
    static std::string Trim(const std::string& s);
    static std::vector<std::string> ExtractStringArray(const std::string& json, const std::string& key);
    static DeviceMode ParseDeviceMode(const std::string& modeName);
    static RGB ParseHexColor(const std::string& hexColor);
};

} // namespace App
} // namespace OCRGB
