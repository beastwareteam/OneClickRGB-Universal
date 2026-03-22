#pragma once
//=============================================================================
// OneClickRGB-Universal - Dry Run Mode
//=============================================================================
// Global dry-run mode for testing without hardware activation.
//
// When enabled:
//   - No actual hardware communication
//   - All operations logged but not executed
//   - Devices report simulated success
//   - Safe for unit tests and CI/CD
//
// Usage:
//   DryRun::Enable();           // Enable globally
//   DryRun::Disable();          // Disable (default)
//   if (DryRun::IsEnabled()) {} // Check state
//
//   // Or scoped:
//   {
//       DryRun::Scope scope;    // Enables for this scope
//       // ... operations are dry-run ...
//   }                           // Automatically restores previous state
//
//=============================================================================

#include <string>
#include <vector>
#include <mutex>
#include <functional>

namespace OCRGB {

//=============================================================================
// Dry Run Log Entry
//=============================================================================
struct DryRunLogEntry {
    std::string timestamp;
    std::string component;      // "HIDBridge", "SMBusBridge", "Device", etc.
    std::string operation;      // "Write", "SetColor", "Apply", etc.
    std::string details;        // Human-readable details
};

//=============================================================================
// Dry Run Mode Controller
//=============================================================================
class DryRun {
public:
    //=========================================================================
    // Global Enable/Disable
    //=========================================================================

    /// Enable dry-run mode globally
    static void Enable() {
        std::lock_guard<std::mutex> lock(GetMutex());
        GetEnabledFlag() = true;
        ClearLogInternal();
    }

    /// Disable dry-run mode globally
    static void Disable() {
        std::lock_guard<std::mutex> lock(GetMutex());
        GetEnabledFlag() = false;
    }

    /// Check if dry-run mode is enabled
    static bool IsEnabled() {
        std::lock_guard<std::mutex> lock(GetMutex());
        return GetEnabledFlag();
    }

    //=========================================================================
    // Logging (when dry-run is enabled)
    //=========================================================================

    /// Log an operation that would have been executed
    static void Log(const std::string& component,
                    const std::string& operation,
                    const std::string& details = "") {
        if (!IsEnabled()) return;

        std::lock_guard<std::mutex> lock(GetMutex());

        DryRunLogEntry entry;
        entry.timestamp = GetTimestamp();
        entry.component = component;
        entry.operation = operation;
        entry.details = details;

        GetLog().push_back(entry);

        // Call callback if set
        if (GetCallback()) {
            GetCallback()(entry);
        }
    }

    /// Get all logged operations
    static std::vector<DryRunLogEntry> GetLogEntries() {
        std::lock_guard<std::mutex> lock(GetMutex());
        return GetLog();
    }

    /// Clear the log
    static void ClearLog() {
        std::lock_guard<std::mutex> lock(GetMutex());
        ClearLogInternal();
    }

    /// Get log entry count
    static size_t GetLogCount() {
        std::lock_guard<std::mutex> lock(GetMutex());
        return GetLog().size();
    }

    //=========================================================================
    // Callback for Real-Time Logging
    //=========================================================================

    using LogCallback = std::function<void(const DryRunLogEntry&)>;

    /// Set callback for each log entry (e.g., for console output)
    static void SetLogCallback(LogCallback callback) {
        std::lock_guard<std::mutex> lock(GetMutex());
        GetCallback() = callback;
    }

    //=========================================================================
    // Scoped Dry Run (RAII)
    //=========================================================================

    class Scope {
    public:
        Scope() : m_previousState(IsEnabled()) {
            Enable();
        }

        ~Scope() {
            if (!m_previousState) {
                Disable();
            }
        }

        // Non-copyable
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        bool m_previousState;
    };

private:
    // Static storage (Meyer's singleton pattern)
    static bool& GetEnabledFlag() {
        static bool enabled = false;
        return enabled;
    }

    static std::vector<DryRunLogEntry>& GetLog() {
        static std::vector<DryRunLogEntry> log;
        return log;
    }

    static std::mutex& GetMutex() {
        static std::mutex mutex;
        return mutex;
    }

    static LogCallback& GetCallback() {
        static LogCallback callback;
        return callback;
    }

    static void ClearLogInternal() {
        GetLog().clear();
    }

    static std::string GetTimestamp() {
        // Simple timestamp for logging
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        auto time = std::chrono::system_clock::to_time_t(now);

        char buf[32];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&time));

        return std::string(buf) + "." + std::to_string(ms.count());
    }
};

//=============================================================================
// Convenience Macros
//=============================================================================

// Log only if dry-run is enabled
#define DRYRUN_LOG(component, operation, details) \
    do { if (::OCRGB::DryRun::IsEnabled()) ::OCRGB::DryRun::Log(component, operation, details); } while(0)

// Return early with success if dry-run is enabled
#define DRYRUN_RETURN_SUCCESS(component, operation, details) \
    do { \
        if (::OCRGB::DryRun::IsEnabled()) { \
            ::OCRGB::DryRun::Log(component, operation, details); \
            return ::OCRGB::Result::Success(); \
        } \
    } while(0)

// Return early with true if dry-run is enabled
#define DRYRUN_RETURN_TRUE(component, operation, details) \
    do { \
        if (::OCRGB::DryRun::IsEnabled()) { \
            ::OCRGB::DryRun::Log(component, operation, details); \
            return true; \
        } \
    } while(0)

} // namespace OCRGB
