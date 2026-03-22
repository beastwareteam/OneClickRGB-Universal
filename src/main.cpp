//=============================================================================
// OneClickRGB-Universal - Command Line Interface
//=============================================================================
// Simple CLI that works without any configuration.
//
// Usage:
//   oneclickrgb                     # Show detected devices
//   oneclickrgb set 255 0 128       # Set color (RGB)
//   oneclickrgb set #FF0080         # Set color (hex)
//   oneclickrgb off                 # Turn off all LEDs
//   oneclickrgb mode static         # Set mode
//   oneclickrgb brightness 50       # Set brightness (0-100)
//   oneclickrgb status              # Show device status
//   oneclickrgb provision --auto    # Auto-provision from fingerprint
//   oneclickrgb provision --check   # Check for config drift
//
// Options:
//   --dry-run                       Simulate without hardware access
//   --verbose                       Show detailed operation log
//   --json                          Output in JSON format (for scripts)
//
// Exit Codes:
//   0 = Success
//   1 = Invalid arguments / Usage error
//   2 = No devices found
//   3 = Device communication error
//   4 = Configuration error
//   5 = Permission denied
//
//=============================================================================

#include "OneClickRGB.h"
#include "app/services/ProvisioningService.h"
#include "app/fingerprint/MachineFingerprint.h"
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iomanip>

using namespace OCRGB;

//=============================================================================
// Exit Codes
//=============================================================================
enum class ExitCode : int {
    Success = 0,
    InvalidArguments = 1,
    NoDevicesFound = 2,
    DeviceError = 3,
    ConfigError = 4,
    PermissionDenied = 5
};

//=============================================================================
// JSON Output Helper
//=============================================================================
class JsonOutput {
public:
    JsonOutput() = default;

    void SetSuccess(bool success) { m_success = success; }
    void SetMessage(const std::string& msg) { m_message = msg; }
    void SetError(const std::string& err) { m_error = err; }
    void SetDeviceCount(int count) { m_deviceCount = count; m_hasDeviceCount = true; }
    void AddDevice(const std::string& name) { m_devices.push_back(name); }
    void SetCommand(const std::string& cmd) { m_command = cmd; }

    void AddField(const std::string& key, const std::string& value) {
        m_fields.push_back({key, value});
    }

    void AddField(const std::string& key, int value) {
        m_fields.push_back({key, std::to_string(value)});
    }

    std::string Build() const {
        std::ostringstream ss;
        ss << "{\n";
        ss << "  \"success\": " << (m_success ? "true" : "false");

        if (!m_command.empty()) {
            ss << ",\n  \"command\": \"" << Escape(m_command) << "\"";
        }

        if (!m_message.empty()) {
            ss << ",\n  \"message\": \"" << Escape(m_message) << "\"";
        }

        if (!m_error.empty()) {
            ss << ",\n  \"error\": \"" << Escape(m_error) << "\"";
        }

        if (m_hasDeviceCount) {
            ss << ",\n  \"deviceCount\": " << m_deviceCount;
        }

        if (!m_devices.empty()) {
            ss << ",\n  \"devices\": [";
            for (size_t i = 0; i < m_devices.size(); i++) {
                if (i > 0) ss << ", ";
                ss << "\"" << Escape(m_devices[i]) << "\"";
            }
            ss << "]";
        }

        for (const auto& field : m_fields) {
            ss << ",\n  \"" << field.first << "\": ";
            // Check if numeric
            bool isNumeric = !field.second.empty();
            for (char c : field.second) {
                if (!std::isdigit(c) && c != '-' && c != '.') {
                    isNumeric = false;
                    break;
                }
            }
            if (isNumeric) {
                ss << field.second;
            } else {
                ss << "\"" << Escape(field.second) << "\"";
            }
        }

        ss << "\n}\n";
        return ss.str();
    }

private:
    bool m_success = false;
    std::string m_message;
    std::string m_error;
    std::string m_command;
    int m_deviceCount = 0;
    bool m_hasDeviceCount = false;
    std::vector<std::string> m_devices;
    std::vector<std::pair<std::string, std::string>> m_fields;

    static std::string Escape(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }
};

//=============================================================================
// CLI Context
//=============================================================================
struct CliContext {
    bool dryRun = false;
    bool verbose = false;
    bool jsonOutput = false;
    OneClickRGB* rgb = nullptr;
    JsonOutput json;
};

//=============================================================================
// Helpers
//=============================================================================

void PrintUsage() {
    std::cout << "OneClickRGB-Universal v" << APP_VERSION << "\n\n";
    std::cout << "Usage:\n";
    std::cout << "  oneclickrgb                      Show detected devices\n";
    std::cout << "  oneclickrgb set <r> <g> <b>      Set color (0-255)\n";
    std::cout << "  oneclickrgb set #RRGGBB          Set color (hex)\n";
    std::cout << "  oneclickrgb off                  Turn off all LEDs\n";
    std::cout << "  oneclickrgb mode <name>          Set mode (static/breathing/rainbow/wave)\n";
    std::cout << "  oneclickrgb brightness <0-100>   Set brightness\n";
    std::cout << "  oneclickrgb status               Show device status (JSON)\n";
    std::cout << "  oneclickrgb provision --auto     Auto-provision from machine fingerprint\n";
    std::cout << "  oneclickrgb provision --check    Check for configuration drift\n";
    std::cout << "  oneclickrgb help                 Show this help\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "  --dry-run                        Simulate without hardware access\n";
    std::cout << "  --verbose                        Show detailed operation log\n";
    std::cout << "  --json                           Output in JSON format (for scripts)\n";
    std::cout << "\n";
    std::cout << "Exit Codes:\n";
    std::cout << "  0 = Success\n";
    std::cout << "  1 = Invalid arguments\n";
    std::cout << "  2 = No devices found\n";
    std::cout << "  3 = Device error\n";
    std::cout << "  4 = Configuration error\n";
    std::cout << "  5 = Permission denied\n";
}

void PrintDevices(CliContext& ctx) {
    int count = ctx.rgb->GetDeviceCount();
    auto names = ctx.rgb->GetDeviceNames();

    if (ctx.jsonOutput) {
        ctx.json.SetSuccess(true);
        ctx.json.SetCommand("list");
        ctx.json.SetDeviceCount(count);
        for (const auto& name : names) {
            ctx.json.AddDevice(name);
        }
        if (count == 0) {
            ctx.json.SetMessage("No RGB devices detected");
        } else {
            ctx.json.SetMessage("Found " + std::to_string(count) + " device(s)");
        }
        std::cout << ctx.json.Build();
        return;
    }

    if (count == 0) {
        std::cout << "No RGB devices detected.\n";
        return;
    }

    std::cout << "Detected " << count << " RGB device(s):\n";
    for (size_t i = 0; i < names.size(); i++) {
        std::cout << "  [" << (i + 1) << "] " << names[i] << "\n";
    }
}

void PrintDryRunLog(CliContext& ctx) {
    if (!ctx.dryRun || !ctx.verbose) return;

    auto log = ctx.rgb->GetDryRunLog();

    if (ctx.jsonOutput) {
        // JSON already includes the data
        return;
    }

    std::cout << "\n[DRY-RUN LOG]\n";
    for (const auto& line : log) {
        std::cout << "  " << line << "\n";
    }
}

uint32_t ParseHexColor(const char* hex) {
    if (hex[0] == '#') hex++;
    return static_cast<uint32_t>(std::strtoul(hex, nullptr, 16));
}

bool IsHexColor(const char* arg) {
    if (!arg) return false;
    if (arg[0] == '#') arg++;
    if (std::strlen(arg) != 6) return false;
    for (int i = 0; i < 6; i++) {
        char c = arg[i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
            return false;
        }
    }
    return true;
}

int OutputError(CliContext& ctx, ExitCode code, const std::string& message) {
    if (ctx.jsonOutput) {
        ctx.json.SetSuccess(false);
        ctx.json.SetError(message);
        ctx.json.AddField("exitCode", static_cast<int>(code));
        std::cout << ctx.json.Build();
    } else {
        std::cerr << "Error: " << message << "\n";
    }
    return static_cast<int>(code);
}

int OutputSuccess(CliContext& ctx, const std::string& message) {
    PrintDryRunLog(ctx);

    if (ctx.jsonOutput) {
        ctx.json.SetSuccess(true);
        ctx.json.SetMessage(message);
        std::cout << ctx.json.Build();
    } else {
        std::cout << message << "\n";
    }
    return static_cast<int>(ExitCode::Success);
}

//=============================================================================
// Command Handlers
//=============================================================================

int CmdStatus(CliContext& ctx) {
    ctx.json.SetCommand("status");
    ctx.json.SetDeviceCount(ctx.rgb->GetDeviceCount());

    auto names = ctx.rgb->GetDeviceNames();
    for (const auto& name : names) {
        ctx.json.AddDevice(name);
    }

    ctx.json.AddField("dryRunEnabled", ctx.rgb->IsDryRunEnabled() ? "true" : "false");

    if (ctx.jsonOutput) {
        ctx.json.SetSuccess(true);
        ctx.json.SetMessage("Status retrieved");
        std::cout << ctx.json.Build();
    } else {
        std::cout << "Status:\n";
        std::cout << "  Devices: " << ctx.rgb->GetDeviceCount() << "\n";
        std::cout << "  Dry-Run: " << (ctx.rgb->IsDryRunEnabled() ? "enabled" : "disabled") << "\n";
        for (size_t i = 0; i < names.size(); i++) {
            std::cout << "  [" << (i + 1) << "] " << names[i] << "\n";
        }
    }
    return static_cast<int>(ExitCode::Success);
}

int CmdSetColor(CliContext& ctx, int argc, char** argv) {
    ctx.json.SetCommand("set");

    if (argc == 3 && IsHexColor(argv[2])) {
        uint32_t color = ParseHexColor(argv[2]);
        ctx.rgb->SetColor(color);

        ctx.json.AddField("color", argv[2]);
        return OutputSuccess(ctx, "Color set.");
    }
    else if (argc == 5) {
        int r = std::atoi(argv[2]);
        int g = std::atoi(argv[3]);
        int b = std::atoi(argv[4]);

        r = (r < 0) ? 0 : (r > 255) ? 255 : r;
        g = (g < 0) ? 0 : (g > 255) ? 255 : g;
        b = (b < 0) ? 0 : (b > 255) ? 255 : b;

        ctx.rgb->SetColor(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));

        ctx.json.AddField("r", r);
        ctx.json.AddField("g", g);
        ctx.json.AddField("b", b);

        std::ostringstream msg;
        msg << "Color set to RGB(" << r << ", " << g << ", " << b << ").";
        return OutputSuccess(ctx, msg.str());
    }

    return OutputError(ctx, ExitCode::InvalidArguments,
        "Usage: oneclickrgb set <r> <g> <b>  OR  oneclickrgb set #RRGGBB");
}

int CmdOff(CliContext& ctx) {
    ctx.json.SetCommand("off");
    ctx.rgb->TurnOff();
    return OutputSuccess(ctx, "LEDs turned off.");
}

int CmdMode(CliContext& ctx, int argc, char** argv) {
    ctx.json.SetCommand("mode");

    if (argc < 3) {
        return OutputError(ctx, ExitCode::InvalidArguments,
            "Usage: oneclickrgb mode <static|breathing|rainbow|wave>");
    }

    const char* mode = argv[2];
    ctx.json.AddField("mode", mode);

    if (std::strcmp(mode, "static") == 0) {
        ctx.rgb->SetModeStatic();
    } else if (std::strcmp(mode, "breathing") == 0) {
        ctx.rgb->SetModeBreathing();
    } else if (std::strcmp(mode, "rainbow") == 0) {
        ctx.rgb->SetModeRainbow();
    } else if (std::strcmp(mode, "wave") == 0) {
        ctx.rgb->SetModeWave();
    } else {
        return OutputError(ctx, ExitCode::InvalidArguments,
            std::string("Unknown mode: ") + mode + ". Available: static, breathing, rainbow, wave");
    }

    return OutputSuccess(ctx, std::string("Mode set to ") + mode + ".");
}

int CmdBrightness(CliContext& ctx, int argc, char** argv) {
    ctx.json.SetCommand("brightness");

    if (argc < 3) {
        return OutputError(ctx, ExitCode::InvalidArguments,
            "Usage: oneclickrgb brightness <0-100>");
    }

    int brightness = std::atoi(argv[2]);
    ctx.rgb->SetBrightness(brightness);
    ctx.json.AddField("brightness", brightness);

    return OutputSuccess(ctx, "Brightness set to " + std::to_string(brightness) + "%.");
}

int CmdProvision(CliContext& ctx, int argc, char** argv) {
    ctx.json.SetCommand("provision");

    if (argc < 3) {
        return OutputError(ctx, ExitCode::InvalidArguments,
            "Usage: oneclickrgb provision <--auto|--check|--self-heal|--rollback|--fingerprint>");
    }

    const char* subCmd = argv[2];
    App::ProvisioningService svc;
    svc.Initialize();

    if (std::strcmp(subCmd, "--auto") == 0) {
        ctx.json.AddField("action", "auto");

        auto result = svc.AutoProvision();

        if (result.success) {
            ctx.json.AddField("profileId", result.profileId);
            ctx.json.AddField("state", App::StateToString(result.finalState));
            ctx.json.AddField("durationMs", static_cast<int>(result.duration.count()));

            for (const auto& warn : result.warnings) {
                ctx.json.AddField("warning", warn);
            }

            return OutputSuccess(ctx, "Auto-provisioning completed. Profile: " + result.profileId);
        } else {
            std::string errors;
            for (const auto& err : result.errors) {
                if (!errors.empty()) errors += "; ";
                errors += err;
            }
            return OutputError(ctx, ExitCode::ConfigError, "Auto-provisioning failed: " + errors);
        }
    }
    else if (std::strcmp(subCmd, "--check") == 0) {
        ctx.json.AddField("action", "check");

        auto drift = svc.CheckDrift();

        ctx.json.AddField("hasDrift", drift.hasDrift ? "true" : "false");
        ctx.json.AddField("previousHash", drift.previousHash);
        ctx.json.AddField("currentHash", drift.currentHash);

        if (drift.hasDrift) {
            ctx.json.AddField("requiresReprovisioning", drift.requiresReprovisioning ? "true" : "false");

            if (!ctx.jsonOutput) {
                std::cout << "Configuration drift detected:\n";
                for (const auto& change : drift.changes) {
                    std::cout << "  - " << change << "\n";
                }
                if (drift.requiresReprovisioning) {
                    std::cout << "Re-provisioning recommended.\n";
                }
            }

            return OutputSuccess(ctx, "Drift detected: " + std::to_string(drift.changes.size()) + " change(s)");
        } else {
            return OutputSuccess(ctx, "No configuration drift detected.");
        }
    }
    else if (std::strcmp(subCmd, "--self-heal") == 0) {
        ctx.json.AddField("action", "self-heal");

        auto result = svc.SelfHeal();

        if (result.success) {
            ctx.json.AddField("profileId", result.profileId);
            return OutputSuccess(ctx, "Self-heal completed. " + result.message);
        } else {
            return OutputError(ctx, ExitCode::ConfigError, "Self-heal failed: " + result.message);
        }
    }
    else if (std::strcmp(subCmd, "--rollback") == 0) {
        ctx.json.AddField("action", "rollback");

        auto result = svc.Rollback();

        if (result.success) {
            ctx.json.AddField("profileId", result.profileId);
            return OutputSuccess(ctx, "Rollback completed. " + result.message);
        } else {
            return OutputError(ctx, ExitCode::ConfigError, "Rollback failed: " + result.message);
        }
    }
    else if (std::strcmp(subCmd, "--fingerprint") == 0) {
        ctx.json.AddField("action", "fingerprint");

        auto fp = App::MachineFingerprint::Collect();

        if (ctx.jsonOutput) {
            // Output raw fingerprint JSON
            std::cout << fp.ToJson();
            return static_cast<int>(ExitCode::Success);
        } else {
            std::cout << "Machine Fingerprint:\n";
            std::cout << "  Machine ID: " << fp.GetMachineId() << "\n";
            std::cout << "  Hash: " << fp.GetHash() << "\n";
            std::cout << "\n  Mainboard: " << fp.GetMainboard().manufacturer
                      << " " << fp.GetMainboard().product << "\n";
            std::cout << "  CPU: " << fp.GetCpu().name << "\n";
            std::cout << "  GPUs: " << fp.GetGpus().size() << "\n";
            std::cout << "  RAM modules: " << fp.GetRam().size() << "\n";
            std::cout << "  RGB devices: " << fp.GetRgbDevices().size() << "\n";

            return OutputSuccess(ctx, "Fingerprint collected.");
        }
    }
    else if (std::strcmp(subCmd, "--status") == 0) {
        ctx.json.AddField("action", "status");

        auto health = svc.GetHealthStatus();

        ctx.json.AddField("healthy", health.healthy ? "true" : "false");
        ctx.json.AddField("state", App::StateToString(health.state));
        ctx.json.AddField("profileId", health.profileId);
        ctx.json.AddField("machineId", health.machineId);

        if (!ctx.jsonOutput) {
            std::cout << "Provisioning Status:\n";
            std::cout << "  Healthy: " << (health.healthy ? "yes" : "no") << "\n";
            std::cout << "  State: " << App::StateToString(health.state) << "\n";
            std::cout << "  Profile: " << health.profileId << "\n";
            std::cout << "  Machine ID: " << health.machineId << "\n";
            std::cout << "  Last Provision: " << health.lastProvisionTime << "\n";

            if (!health.issues.empty()) {
                std::cout << "  Issues:\n";
                for (const auto& issue : health.issues) {
                    std::cout << "    - " << issue << "\n";
                }
            }
        }

        return OutputSuccess(ctx, health.healthy ? "System healthy" : "System has issues");
    }

    return OutputError(ctx, ExitCode::InvalidArguments,
        "Unknown provision subcommand: " + std::string(subCmd));
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    OneClickRGB rgb;
    CliContext ctx;
    ctx.rgb = &rgb;

    // Parse global flags
    int argOffset = 1;

    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--dry-run") == 0) {
            ctx.dryRun = true;
            argOffset++;
        } else if (std::strcmp(argv[i], "--verbose") == 0) {
            ctx.verbose = true;
            argOffset++;
        } else if (std::strcmp(argv[i], "--json") == 0) {
            ctx.jsonOutput = true;
            argOffset++;
        } else {
            break;
        }
    }

    // Adjust argc/argv to skip flags
    int effectiveArgc = argc - argOffset + 1;
    char** effectiveArgv = argv + argOffset - 1;

    // Enable dry-run mode if requested
    if (ctx.dryRun) {
        rgb.EnableDryRun();
        if (!ctx.jsonOutput) {
            std::cout << "[DRY-RUN MODE] No hardware will be accessed.\n\n";
        }
    }

    // Start and detect devices
    int deviceCount = rgb.Start();

    // Check for help first (before device check)
    if (effectiveArgc > 1) {
        const char* cmd = effectiveArgv[1];
        if (std::strcmp(cmd, "help") == 0 || std::strcmp(cmd, "--help") == 0 || std::strcmp(cmd, "-h") == 0) {
            PrintUsage();
            return static_cast<int>(ExitCode::Success);
        }
    }

    // Handle no devices case
    if (deviceCount == 0 && effectiveArgc > 1) {
        if (!ctx.dryRun) {
            return OutputError(ctx, ExitCode::NoDevicesFound, "No RGB devices detected.");
        }
        if (!ctx.jsonOutput) {
            std::cout << "[DRY-RUN] Simulating with 0 devices.\n";
        }
    }

    // No arguments - show devices
    if (effectiveArgc == 1) {
        PrintDevices(ctx);
        PrintDryRunLog(ctx);
        return static_cast<int>(ExitCode::Success);
    }

    const char* command = effectiveArgv[1];

    // Route commands
    if (std::strcmp(command, "status") == 0) {
        return CmdStatus(ctx);
    }
    if (std::strcmp(command, "set") == 0) {
        return CmdSetColor(ctx, effectiveArgc, effectiveArgv);
    }
    if (std::strcmp(command, "off") == 0) {
        return CmdOff(ctx);
    }
    if (std::strcmp(command, "mode") == 0) {
        return CmdMode(ctx, effectiveArgc, effectiveArgv);
    }
    if (std::strcmp(command, "brightness") == 0 || std::strcmp(command, "bright") == 0) {
        return CmdBrightness(ctx, effectiveArgc, effectiveArgv);
    }
    if (std::strcmp(command, "provision") == 0) {
        return CmdProvision(ctx, effectiveArgc, effectiveArgv);
    }

    // Unknown command
    return OutputError(ctx, ExitCode::InvalidArguments,
        std::string("Unknown command: ") + command);
}
