//=============================================================================
// OneClickRGB-Universal - ConfigBundle Parser Implementation
//=============================================================================
// Simple JSON parser without external dependencies.
// Handles the specific structure of config_bundle.schema.json.
//=============================================================================

#include "ConfigBundleParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace OCRGB {
namespace App {

//-----------------------------------------------------------------------------
// Loading
//-----------------------------------------------------------------------------

bool ConfigBundleParser::LoadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        m_lastError = "Failed to open file: " + filePath;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return LoadFromString(buffer.str());
}

bool ConfigBundleParser::LoadFromString(const std::string& jsonContent) {
    m_rawContent = jsonContent;
    m_bundle = ConfigBundle();
    m_lastError.clear();

    if (jsonContent.empty()) {
        m_lastError = "Empty JSON content";
        return false;
    }

    return ParseJson(jsonContent);
}

//-----------------------------------------------------------------------------
// Profile Matching
//-----------------------------------------------------------------------------

const BundleProfile* ConfigBundleParser::FindMatchingProfile(const DeviceInfo& info) const {
    return FindMatchingProfile(info.vendor, info.type);
}

const BundleProfile* ConfigBundleParser::FindMatchingProfile(
    const std::string& vendor, DeviceType deviceType) const {

    const BundleProfile* bestMatch = nullptr;
    int bestPriority = -1;

    std::string deviceTypeStr = DeviceTypeToString(deviceType);

    for (const auto& profile : m_bundle.profiles) {
        bool vendorMatch = profile.match.vendors.empty();
        bool typeMatch = profile.match.deviceTypes.empty();

        // Check vendor match
        for (const auto& v : profile.match.vendors) {
            if (v == vendor) {
                vendorMatch = true;
                break;
            }
        }

        // Check device type match
        for (const auto& t : profile.match.deviceTypes) {
            if (t == deviceTypeStr) {
                typeMatch = true;
                break;
            }
        }

        // If both match and higher priority, use this profile
        if (vendorMatch && typeMatch && profile.priority > bestPriority) {
            bestMatch = &profile;
            bestPriority = profile.priority;
        }
    }

    return bestMatch;
}

ResolvedProfile ConfigBundleParser::ToResolvedProfile(const BundleProfile& profile) const {
    ResolvedProfile resolved;
    resolved.id = profile.id;
    resolved.priority = profile.priority;

    // Apply actions to build resolved profile
    for (const auto& action : profile.actions) {
        if (action.op == "set-mode") {
            resolved.mode = ParseDeviceMode(action.value);
        }
        else if (action.op == "set-color") {
            resolved.defaultColor = ParseHexColor(action.value);
        }
        else if (action.op == "set-brightness") {
            resolved.brightness = static_cast<uint8_t>(std::stoi(action.value));
        }
        else if (action.op == "set-speed") {
            resolved.speed = static_cast<uint8_t>(std::stoi(action.value));
        }
        // "apply" is handled at execution time, not in resolved profile
    }

    return resolved;
}

//-----------------------------------------------------------------------------
// Integrity Verification
//-----------------------------------------------------------------------------

bool ConfigBundleParser::VerifyIntegrity() const {
    // TODO: Implement SHA-256 hash verification
    // For now, just check that integrity fields are present
    return !m_bundle.integrity.hash.empty() &&
           !m_bundle.integrity.signature.empty() &&
           !m_bundle.integrity.keyId.empty();
}

//-----------------------------------------------------------------------------
// JSON Parsing (minimal implementation)
//-----------------------------------------------------------------------------

bool ConfigBundleParser::ParseJson(const std::string& json) {
    // Extract top-level fields
    m_bundle.bundleVersion = ExtractString(json, "bundleVersion");
    m_bundle.createdAt = ExtractString(json, "createdAt");

    if (m_bundle.bundleVersion.empty()) {
        m_lastError = "Missing required field: bundleVersion";
        return false;
    }

    // Extract target
    size_t targetPos = json.find("\"target\"");
    if (targetPos != std::string::npos) {
        size_t targetStart = json.find('{', targetPos);
        size_t targetEnd = json.find('}', targetStart);
        if (targetStart != std::string::npos && targetEnd != std::string::npos) {
            std::string targetJson = json.substr(targetStart, targetEnd - targetStart + 1);
            m_bundle.targetOs = ExtractString(targetJson, "os");
            m_bundle.targetArch = ExtractString(targetJson, "arch");
        }
    }

    // Extract policies
    size_t policiesPos = json.find("\"policies\"");
    if (policiesPos != std::string::npos) {
        size_t policiesStart = json.find('{', policiesPos);
        size_t policiesEnd = json.find('}', policiesStart);
        if (policiesStart != std::string::npos && policiesEnd != std::string::npos) {
            std::string policiesJson = json.substr(policiesStart, policiesEnd - policiesStart + 1);
            m_bundle.policies.allowRawPackets = ExtractBool(policiesJson, "allowRawPackets", false);
            m_bundle.policies.maxRetry = ExtractInt(policiesJson, "maxRetry", 3);
            m_bundle.policies.rollbackOnError = ExtractBool(policiesJson, "rollbackOnError", true);
        }
    }

    // Extract integrity
    size_t integrityPos = json.find("\"integrity\"");
    if (integrityPos != std::string::npos) {
        size_t integrityStart = json.find('{', integrityPos);
        size_t integrityEnd = json.find('}', integrityStart);
        if (integrityStart != std::string::npos && integrityEnd != std::string::npos) {
            std::string integrityJson = json.substr(integrityStart, integrityEnd - integrityStart + 1);
            m_bundle.integrity.hash = ExtractString(integrityJson, "hash");
            m_bundle.integrity.signature = ExtractString(integrityJson, "signature");
            m_bundle.integrity.keyId = ExtractString(integrityJson, "keyId");
        }
    }

    // Extract profiles array
    size_t profilesPos = json.find("\"profiles\"");
    if (profilesPos != std::string::npos) {
        size_t arrayStart = json.find('[', profilesPos);
        if (arrayStart != std::string::npos) {
            // Find matching ]
            int depth = 1;
            size_t arrayEnd = arrayStart + 1;
            while (arrayEnd < json.size() && depth > 0) {
                if (json[arrayEnd] == '[') depth++;
                else if (json[arrayEnd] == ']') depth--;
                arrayEnd++;
            }

            std::string profilesArray = json.substr(arrayStart, arrayEnd - arrayStart);

            // Parse individual profiles
            size_t pos = 0;
            while ((pos = profilesArray.find('{', pos)) != std::string::npos) {
                // Find matching }
                int braceDepth = 1;
                size_t endPos = pos + 1;
                while (endPos < profilesArray.size() && braceDepth > 0) {
                    if (profilesArray[endPos] == '{') braceDepth++;
                    else if (profilesArray[endPos] == '}') braceDepth--;
                    endPos++;
                }

                std::string profileJson = profilesArray.substr(pos, endPos - pos);

                BundleProfile profile;
                profile.id = ExtractString(profileJson, "id");
                profile.priority = ExtractInt(profileJson, "priority", 0);

                // Parse match criteria
                size_t matchPos = profileJson.find("\"match\"");
                if (matchPos != std::string::npos) {
                    size_t matchStart = profileJson.find('{', matchPos);
                    size_t matchEnd = profileJson.find('}', matchStart);
                    if (matchStart != std::string::npos && matchEnd != std::string::npos) {
                        std::string matchJson = profileJson.substr(matchStart, matchEnd - matchStart + 1);
                        profile.match.machineId = ExtractString(matchJson, "machineId");
                        profile.match.vendors = ExtractStringArray(matchJson, "vendors");
                        profile.match.deviceTypes = ExtractStringArray(matchJson, "deviceTypes");
                        profile.match.tags = ExtractStringArray(matchJson, "tags");
                    }
                }

                // Parse actions
                size_t actionsPos = profileJson.find("\"actions\"");
                if (actionsPos != std::string::npos) {
                    size_t actionsStart = profileJson.find('[', actionsPos);
                    size_t actionsEnd = profileJson.find(']', actionsStart);
                    if (actionsStart != std::string::npos && actionsEnd != std::string::npos) {
                        std::string actionsJson = profileJson.substr(actionsStart, actionsEnd - actionsStart + 1);

                        size_t actionPos = 0;
                        while ((actionPos = actionsJson.find('{', actionPos)) != std::string::npos) {
                            size_t actionEnd = actionsJson.find('}', actionPos);
                            if (actionEnd != std::string::npos) {
                                std::string actionJson = actionsJson.substr(actionPos, actionEnd - actionPos + 1);

                                BundleAction action;
                                action.op = ExtractString(actionJson, "op");
                                action.value = ExtractString(actionJson, "value");

                                if (!action.op.empty()) {
                                    profile.actions.push_back(action);
                                }

                                actionPos = actionEnd + 1;
                            } else {
                                break;
                            }
                        }
                    }
                }

                if (!profile.id.empty()) {
                    m_bundle.profiles.push_back(profile);
                }

                pos = endPos;
            }
        }
    }

    if (m_bundle.profiles.empty()) {
        m_lastError = "No profiles found in bundle";
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
// String Extraction Helpers
//-----------------------------------------------------------------------------

std::string ConfigBundleParser::ExtractString(const std::string& json, const std::string& key) const {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";

    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) return "";

    size_t valueStart = json.find('"', colonPos);
    if (valueStart == std::string::npos) return "";

    size_t valueEnd = json.find('"', valueStart + 1);
    if (valueEnd == std::string::npos) return "";

    return json.substr(valueStart + 1, valueEnd - valueStart - 1);
}

int ConfigBundleParser::ExtractInt(const std::string& json, const std::string& key, int defaultVal) const {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return defaultVal;

    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) return defaultVal;

    // Skip whitespace
    size_t valueStart = colonPos + 1;
    while (valueStart < json.size() && std::isspace(json[valueStart])) {
        valueStart++;
    }

    // Read digits
    std::string numStr;
    while (valueStart < json.size() && (std::isdigit(json[valueStart]) || json[valueStart] == '-')) {
        numStr += json[valueStart];
        valueStart++;
    }

    if (numStr.empty()) return defaultVal;
    return std::stoi(numStr);
}

bool ConfigBundleParser::ExtractBool(const std::string& json, const std::string& key, bool defaultVal) const {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return defaultVal;

    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) return defaultVal;

    // Look for true/false
    size_t truePos = json.find("true", colonPos);
    size_t falsePos = json.find("false", colonPos);

    if (truePos != std::string::npos && (falsePos == std::string::npos || truePos < falsePos)) {
        return true;
    }
    if (falsePos != std::string::npos) {
        return false;
    }

    return defaultVal;
}

std::string ConfigBundleParser::Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> ConfigBundleParser::ExtractStringArray(const std::string& json, const std::string& key) {
    std::vector<std::string> result;

    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return result;

    size_t arrayStart = json.find('[', keyPos);
    if (arrayStart == std::string::npos) return result;

    size_t arrayEnd = json.find(']', arrayStart);
    if (arrayEnd == std::string::npos) return result;

    std::string arrayContent = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);

    size_t pos = 0;
    while ((pos = arrayContent.find('"', pos)) != std::string::npos) {
        size_t endPos = arrayContent.find('"', pos + 1);
        if (endPos != std::string::npos) {
            result.push_back(arrayContent.substr(pos + 1, endPos - pos - 1));
            pos = endPos + 1;
        } else {
            break;
        }
    }

    return result;
}

DeviceMode ConfigBundleParser::ParseDeviceMode(const std::string& modeName) {
    std::string lower = modeName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "off") return DeviceMode::Off;
    if (lower == "static") return DeviceMode::Static;
    if (lower == "breathing") return DeviceMode::Breathing;
    if (lower == "wave") return DeviceMode::Wave;
    if (lower == "spectrum" || lower == "rainbow") return DeviceMode::Spectrum;
    if (lower == "reactive") return DeviceMode::Reactive;
    if (lower == "colorcycle") return DeviceMode::ColorCycle;
    if (lower == "gradient") return DeviceMode::Gradient;
    if (lower == "custom") return DeviceMode::Custom;

    return DeviceMode::Static;  // Default
}

RGB ConfigBundleParser::ParseHexColor(const std::string& hexColor) {
    std::string hex = hexColor;

    // Remove # prefix
    if (!hex.empty() && hex[0] == '#') {
        hex = hex.substr(1);
    }

    // Parse as hex
    uint32_t value = 0;
    try {
        value = static_cast<uint32_t>(std::stoul(hex, nullptr, 16));
    } catch (...) {
        return RGB(0, 170, 255);  // Default blue
    }

    return RGB::FromHex(value);
}

} // namespace App
} // namespace OCRGB
