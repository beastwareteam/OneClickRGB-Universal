#pragma once
//=============================================================================
// OneClickRGB-Universal - Device Interface
//=============================================================================
// Abstract base class for all RGB devices. Implementations must derive from
// this class and implement all pure virtual methods.
//=============================================================================

#include "../core/Types.h"
#include <memory>
#include <functional>

namespace OCRGB {

//-----------------------------------------------------------------------------
// Device Event Callbacks
//-----------------------------------------------------------------------------
using DeviceConnectedCallback = std::function<void(const DeviceInfo&)>;
using DeviceDisconnectedCallback = std::function<void(const std::string& deviceId)>;
using DeviceErrorCallback = std::function<void(const std::string& deviceId, const std::string& error)>;

//-----------------------------------------------------------------------------
// IDevice Interface
//-----------------------------------------------------------------------------
class IDevice {
public:
    virtual ~IDevice() = default;

    //=========================================================================
    // Lifecycle
    //=========================================================================

    /// Initialize the device connection
    /// @return Result indicating success or failure
    virtual Result Initialize() = 0;

    /// Shutdown the device connection gracefully
    virtual void Shutdown() = 0;

    /// Check if device is currently connected
    virtual bool IsConnected() const = 0;

    /// Check if device is initialized and ready
    virtual bool IsReady() const = 0;

    /// Attempt to reconnect to the device
    virtual Result Reconnect() {
        Shutdown();
        return Initialize();
    }

    //=========================================================================
    // Device Information
    //=========================================================================

    /// Get device information
    virtual const DeviceInfo& GetInfo() const = 0;

    /// Get device capabilities
    virtual const Capabilities& GetCapabilities() const = 0;

    /// Get current device state
    virtual const DeviceState& GetState() const = 0;

    /// Get unique device identifier
    virtual std::string GetId() const { return GetInfo().id; }

    /// Get display name
    virtual std::string GetName() const { return GetInfo().name; }

    //=========================================================================
    // Color Control
    //=========================================================================

    /// Set the primary color for all zones
    /// @param color RGB color to set
    /// @return Result indicating success or failure
    virtual Result SetColor(const RGB& color) = 0;

    /// Set color using individual RGB values
    Result SetColor(uint8_t r, uint8_t g, uint8_t b) {
        return SetColor(RGB(r, g, b));
    }

    /// Set color for a specific zone
    /// @param zone Zone index (0-based)
    /// @param color RGB color to set
    virtual Result SetZoneColor(int zone, const RGB& color) {
        // Default implementation: set all zones to same color
        return SetColor(color);
    }

    /// Set colors for all zones at once
    /// @param colors Vector of colors, one per zone
    virtual Result SetAllZoneColors(const std::vector<RGB>& colors) {
        Result result = Result::Success();
        for (size_t i = 0; i < colors.size() && i < (size_t)GetCapabilities().zoneCount; i++) {
            result = SetZoneColor((int)i, colors[i]);
            if (result.IsError()) break;
        }
        return result;
    }

    /// Turn off all LEDs
    virtual Result TurnOff() {
        return SetColor(RGB(0, 0, 0));
    }

    //=========================================================================
    // Mode Control
    //=========================================================================

    /// Set the lighting mode/effect
    /// @param mode The mode to set
    /// @return Result indicating success or failure
    virtual Result SetMode(DeviceMode mode) = 0;

    /// Get supported modes for this device
    virtual std::vector<DeviceMode> GetSupportedModes() const {
        return GetCapabilities().supportedModes;
    }

    /// Check if a mode is supported
    virtual bool IsModeSupported(DeviceMode mode) const {
        const auto& modes = GetSupportedModes();
        return std::find(modes.begin(), modes.end(), mode) != modes.end();
    }

    //=========================================================================
    // Brightness & Speed
    //=========================================================================

    /// Set brightness level (0-100)
    /// @param brightness Brightness percentage
    virtual Result SetBrightness(uint8_t brightness) = 0;

    /// Set animation speed (0-100)
    /// @param speed Speed percentage
    virtual Result SetSpeed(uint8_t speed) = 0;

    //=========================================================================
    // Direct Hardware Access
    //=========================================================================

    /// Send a raw packet to the device
    /// @param data Pointer to data buffer
    /// @param length Length of data
    /// @return Result indicating success or failure
    virtual Result SendRawPacket(const uint8_t* data, size_t length) = 0;

    /// Read a raw response from the device
    /// @param buffer Buffer to store response
    /// @param length Maximum bytes to read
    /// @param bytesRead Actual bytes read
    /// @param timeoutMs Timeout in milliseconds
    virtual Result ReadRawResponse(uint8_t* buffer, size_t length,
                                    size_t& bytesRead, int timeoutMs = 1000) = 0;

    /// Send feature report (HID devices)
    virtual Result SendFeatureReport(const uint8_t* data, size_t length) {
        return Result::NotSupported();
    }

    /// Get feature report (HID devices)
    virtual Result GetFeatureReport(uint8_t* buffer, size_t length) {
        return Result::NotSupported();
    }

    //=========================================================================
    // Apply / Commit
    //=========================================================================

    /// Apply all pending changes to the device
    /// Some devices require explicit commit after setting colors
    virtual Result Apply() {
        return Result::Success();  // Default: no explicit apply needed
    }

    /// Save current settings to device memory (if supported)
    virtual Result SaveToDevice() {
        return Result::NotSupported();
    }

    //=========================================================================
    // Event Callbacks
    //=========================================================================

    void SetConnectedCallback(DeviceConnectedCallback cb) { m_onConnected = cb; }
    void SetDisconnectedCallback(DeviceDisconnectedCallback cb) { m_onDisconnected = cb; }
    void SetErrorCallback(DeviceErrorCallback cb) { m_onError = cb; }

protected:
    DeviceConnectedCallback m_onConnected;
    DeviceDisconnectedCallback m_onDisconnected;
    DeviceErrorCallback m_onError;

    void NotifyConnected() {
        if (m_onConnected) m_onConnected(GetInfo());
    }

    void NotifyDisconnected() {
        if (m_onDisconnected) m_onDisconnected(GetId());
    }

    void NotifyError(const std::string& error) {
        if (m_onError) m_onError(GetId(), error);
    }
};

//-----------------------------------------------------------------------------
// Device Smart Pointer
//-----------------------------------------------------------------------------
using DevicePtr = std::shared_ptr<IDevice>;
using DeviceWeakPtr = std::weak_ptr<IDevice>;

} // namespace OCRGB
