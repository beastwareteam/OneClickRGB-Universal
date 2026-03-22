//=============================================================================
// OneClickRGB-Universal - Simple Interface Implementation
//=============================================================================

#include "OneClickRGB.h"
#include "core/DeviceRegistry.h"
#include "core/DryRunMode.h"
#include "app/services/DeviceService.h"
#include <cstring>

namespace OCRGB {

//=============================================================================
// OneClickRGB Class Implementation
//=============================================================================

OneClickRGB::OneClickRGB()
    : m_service(std::make_unique<App::DeviceService>()) {
}

OneClickRGB::~OneClickRGB() {
    Stop();
}

int OneClickRGB::Start() {
    if (m_running) {
        return GetDeviceCount();
    }

    int count = m_service->DiscoverAndRegister();
    m_running = (count > 0);
    return count;
}

void OneClickRGB::Stop() {
    if (!m_running) return;

    TurnOff();
    DeviceRegistry::Instance().Clear();
    m_running = false;
}

void OneClickRGB::SetColor(uint8_t r, uint8_t g, uint8_t b) {
    SetColor(RGB(r, g, b));
}

void OneClickRGB::SetColor(uint32_t hexColor) {
    SetColor(RGB::FromHex(hexColor));
}

void OneClickRGB::SetColor(const RGB& color) {
    if (!m_running) return;
    DeviceRegistry::Instance().SetColorAll(color);
    ApplyToAll();
}

void OneClickRGB::TurnOff() {
    if (!m_running) return;
    DeviceRegistry::Instance().TurnOffAll();
}

void OneClickRGB::SetModeStatic() {
    if (!m_running) return;
    DeviceRegistry::Instance().SetModeAll(DeviceMode::Static);
    ApplyToAll();
}

void OneClickRGB::SetModeBreathing() {
    if (!m_running) return;
    DeviceRegistry::Instance().SetModeAll(DeviceMode::Breathing);
    ApplyToAll();
}

void OneClickRGB::SetModeRainbow() {
    if (!m_running) return;
    DeviceRegistry::Instance().SetModeAll(DeviceMode::Spectrum);
    ApplyToAll();
}

void OneClickRGB::SetModeWave() {
    if (!m_running) return;
    DeviceRegistry::Instance().SetModeAll(DeviceMode::Wave);
    ApplyToAll();
}

void OneClickRGB::SetBrightness(int percent) {
    if (!m_running) return;

    // Clamp to 0-100
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    DeviceRegistry::Instance().SetBrightnessAll(static_cast<uint8_t>(percent));
    ApplyToAll();
}

int OneClickRGB::GetDeviceCount() const {
    return static_cast<int>(DeviceRegistry::Instance().GetDeviceCount());
}

std::vector<std::string> OneClickRGB::GetDeviceNames() const {
    std::vector<std::string> names;
    auto devices = DeviceRegistry::Instance().GetAllDevices();
    names.reserve(devices.size());

    for (const auto& device : devices) {
        names.push_back(device->GetName());
    }

    return names;
}

void OneClickRGB::ApplyToAll() {
    DeviceRegistry::Instance().ApplyAll();
}

void OneClickRGB::EnableDryRun() {
    DryRun::Enable();
}

void OneClickRGB::DisableDryRun() {
    DryRun::Disable();
}

bool OneClickRGB::IsDryRunEnabled() const {
    return DryRun::IsEnabled();
}

std::vector<std::string> OneClickRGB::GetDryRunLog() const {
    std::vector<std::string> result;
    auto entries = DryRun::GetLogEntries();
    result.reserve(entries.size());

    for (const auto& entry : entries) {
        std::string line = "[" + entry.timestamp + "] " +
                          entry.component + "::" + entry.operation;
        if (!entry.details.empty()) {
            line += " - " + entry.details;
        }
        result.push_back(line);
    }

    return result;
}

void OneClickRGB::ClearDryRunLog() {
    DryRun::ClearLog();
}

} // namespace OCRGB

//=============================================================================
// Global Instance for C-style API
//=============================================================================

static OCRGB::OneClickRGB* g_instance = nullptr;

static OCRGB::OneClickRGB& GetGlobalInstance() {
    if (!g_instance) {
        g_instance = new OCRGB::OneClickRGB();
    }
    return *g_instance;
}

//=============================================================================
// Global Convenience Functions
//=============================================================================

int OCRGB_Start() {
    return GetGlobalInstance().Start();
}

void OCRGB_Stop() {
    if (g_instance) {
        g_instance->Stop();
        delete g_instance;
        g_instance = nullptr;
    }
}

void OCRGB_SetColor(uint8_t r, uint8_t g, uint8_t b) {
    GetGlobalInstance().SetColor(r, g, b);
}

void OCRGB_SetColorHex(uint32_t hexColor) {
    GetGlobalInstance().SetColor(hexColor);
}

void OCRGB_TurnOff() {
    GetGlobalInstance().TurnOff();
}

void OCRGB_SetBrightness(int percent) {
    GetGlobalInstance().SetBrightness(percent);
}

void OCRGB_SetMode(const char* modeName) {
    if (!modeName) return;

    auto& rgb = GetGlobalInstance();

    if (std::strcmp(modeName, "static") == 0) {
        rgb.SetModeStatic();
    } else if (std::strcmp(modeName, "breathing") == 0) {
        rgb.SetModeBreathing();
    } else if (std::strcmp(modeName, "rainbow") == 0) {
        rgb.SetModeRainbow();
    } else if (std::strcmp(modeName, "wave") == 0) {
        rgb.SetModeWave();
    }
    // Unknown modes are silently ignored
}

void OCRGB_EnableDryRun() {
    OCRGB::DryRun::Enable();
}

void OCRGB_DisableDryRun() {
    OCRGB::DryRun::Disable();
}

bool OCRGB_IsDryRunEnabled() {
    return OCRGB::DryRun::IsEnabled();
}
