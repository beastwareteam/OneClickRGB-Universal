#pragma once
//=============================================================================
// OneClickRGB-Universal - Simple Interface
//=============================================================================
// This is the ONLY header most users need to include.
// Provides a dead-simple API for RGB control with sensible defaults.
//
// Usage:
//   #include "OneClickRGB.h"
//
//   OneClickRGB rgb;
//   rgb.Start();                    // Auto-detect and initialize all devices
//   rgb.SetColor(0, 100, 255);      // Set color on all devices
//   rgb.Stop();                     // Cleanup
//
// Dry-Run Mode (for testing without hardware):
//   OneClickRGB rgb;
//   rgb.EnableDryRun();             // Enable before Start()
//   rgb.Start();                    // Simulates device discovery
//   rgb.SetColor(0, 100, 255);      // Logged but not executed
//   auto log = rgb.GetDryRunLog();  // Get operations log
//   rgb.Stop();
//
//=============================================================================

#include "core/Types.h"
#include <memory>
#include <vector>
#include <string>

namespace OCRGB {

// Forward declarations
class IDevice;
namespace App { class DeviceService; }

//=============================================================================
// OneClickRGB - Main Interface
//=============================================================================
class OneClickRGB {
public:
    OneClickRGB();
    ~OneClickRGB();

    //=========================================================================
    // Lifecycle (call Start once, Stop when done)
    //=========================================================================

    /// Start RGB control - auto-detects and initializes all devices
    /// @return Number of devices found
    int Start();

    /// Stop RGB control and release all devices
    void Stop();

    /// Check if RGB control is active
    bool IsRunning() const { return m_running; }

    //=========================================================================
    // Simple Color Control
    //=========================================================================

    /// Set color on all devices (RGB values 0-255)
    void SetColor(uint8_t r, uint8_t g, uint8_t b);

    /// Set color on all devices (hex value, e.g. 0x00AAFF)
    void SetColor(uint32_t hexColor);

    /// Set color on all devices (RGB struct)
    void SetColor(const RGB& color);

    /// Turn off all LEDs
    void TurnOff();

    //=========================================================================
    // Simple Mode Control
    //=========================================================================

    /// Set static color mode (default)
    void SetModeStatic();

    /// Set breathing/pulsing mode
    void SetModeBreathing();

    /// Set rainbow/spectrum cycle mode
    void SetModeRainbow();

    /// Set wave effect mode
    void SetModeWave();

    //=========================================================================
    // Brightness Control
    //=========================================================================

    /// Set brightness (0-100%)
    void SetBrightness(int percent);

    //=========================================================================
    // Device Info (optional - most users don't need this)
    //=========================================================================

    /// Get number of detected devices
    int GetDeviceCount() const;

    /// Get device names (for display purposes)
    std::vector<std::string> GetDeviceNames() const;

    //=========================================================================
    // Dry-Run Mode (for testing without hardware)
    //=========================================================================

    /// Enable dry-run mode (call before Start)
    /// In dry-run mode, no hardware is accessed - all operations are logged
    void EnableDryRun();

    /// Disable dry-run mode
    void DisableDryRun();

    /// Check if dry-run mode is enabled
    bool IsDryRunEnabled() const;

    /// Get dry-run operation log
    std::vector<std::string> GetDryRunLog() const;

    /// Clear dry-run log
    void ClearDryRunLog();

private:
    bool m_running = false;
    std::unique_ptr<App::DeviceService> m_service;

    void ApplyToAll();
};

} // namespace OCRGB

//=============================================================================
// Global Convenience Functions (for absolute simplicity)
//=============================================================================
// These allow one-liner usage without creating an object:
//
//   OCRGB_Start();
//   OCRGB_SetColor(0, 100, 255);
//   OCRGB_Stop();
//
//=============================================================================

/// Start RGB control (call once at program start)
int OCRGB_Start();

/// Stop RGB control (call once at program end)
void OCRGB_Stop();

/// Set color on all devices (RGB 0-255)
void OCRGB_SetColor(uint8_t r, uint8_t g, uint8_t b);

/// Set color on all devices (hex, e.g. 0x00AAFF)
void OCRGB_SetColorHex(uint32_t hexColor);

/// Turn off all LEDs
void OCRGB_TurnOff();

/// Set brightness (0-100%)
void OCRGB_SetBrightness(int percent);

/// Set mode: "static", "breathing", "rainbow", "wave"
void OCRGB_SetMode(const char* modeName);

/// Enable dry-run mode (call before OCRGB_Start)
void OCRGB_EnableDryRun();

/// Disable dry-run mode
void OCRGB_DisableDryRun();

/// Check if dry-run mode is enabled
bool OCRGB_IsDryRunEnabled();
