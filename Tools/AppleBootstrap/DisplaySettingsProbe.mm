#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/Os/Input.h"
#include "WoW/Source/ConsoleDevice/ConsoleDetect.h"
#include "WoW/Source/ConsoleDevice/ConsoleDevice.h"
#include "WoW/Source/Hardware.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace {
void Require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
void Set(const char* name, const char* value) {
    auto* var = CVar::Lookup(name);
    Require(var != nullptr, "Original display CVar was not registered");
    var->Set(value, true, false, false, true);
}
void Shutdown() {
    CVar::Destroy(); ConsoleCommandDestroy(); CStringManager::DestroyManager(); CVar::s_configFile = nullptr;
}
}

void CheckDisplaySettings(CGxDeviceMetal& device) {
    Require(!CVar::m_initialized && !device.FrameCommand(), "Display settings check requires an idle, unconfigured device");
    std::string pattern = (std::filesystem::temp_directory_path() / "wow-display-XXXXXX").string();
    std::vector<char> directory(pattern.begin(), pattern.end()); directory.push_back(0);
    Require(mkdtemp(directory.data()), "Cannot create native display configuration directory");
    struct Scope {
        CGxDeviceMetal& device;
        std::string directory;
        CGxFormat active = device.m_format, requested = s_requestedFormat;
        DefaultSettings defaults = s_defaults;
        Hardware hardware = s_hardware;
        uint32_t fps = GxGetMaxFPS(), background = GxGetMaxFPSBk();
        ~Scope() {
            OsInputSetWindowResizeLock(0); Shutdown();
            s_requestedFormat = requested; s_defaults = defaults; s_hardware = hardware;
            device.DeviceSetFormat(active); GxSetMaxFPS(fps); GxSetMaxFPSBk(background);
            g_videoHardwareDB.Free(); g_videoHardwareDB.Reset();
            std::error_code error; std::filesystem::remove_all(directory, error);
        }
    } scope{device, directory.data()};
    const std::string config = scope.directory + "/Config.wtf";
    CGxMonitorMode desktop{};
    Require(GxAdapterDesktopMode(desktop), "Native desktop query failed");
    CGxFormat active = device.m_format;
    active.m_refreshRate = desktop.m_refreshRate;
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        // Exercise both original CPU tiers. The selected tier is explicit:
        // native uptime-clock frequency must never be used as processor MHz.
        Hardware slower{}, faster{};
        Require(ConsoleDetectSelectHardware(slower, 0) && ConsoleDetectSelectHardware(faster, 1), "Native adapter did not select original hardware data");
        Require(faster.m_videoDevice.m_vendorID == 0xFFFF && faster.m_videoDevice.m_deviceID == 3 &&
            faster.m_videoHw && faster.m_videoHw == slower.m_videoHw, "Metal selected an invented PCI/DBC record");
        DefaultSettings low{}, high{};
        ConsoleDetectSetDefaults(low, slower); ConsoleDetectSetDefaults(high, faster);
        Require(high.m_farClip >= low.m_farClip && high.m_particleDensity >= low.m_particleDensity &&
            high.m_baseMipLevel == faster.m_videoHw->m_baseMipLevel && high.m_format->m_size.x > 0,
            "Original DBC defaults did not survive native selection");
        const CGxFormat originalFormat = *high.m_format;
        std::fprintf(stderr, "Original hardware: video %u, generic %u, CPU tiers far clip %.0f/%.0f, particles %.2f/%.2f, DBC format %dx%d color %d depth %d\n",
            faster.m_videoID, faster.m_videoDevice.m_deviceID, low.m_farClip, high.m_farClip,
            low.m_particleDensity, high.m_particleDensity, originalFormat.m_size.x, originalFormat.m_size.y,
            originalFormat.m_colorFormat, originalFormat.m_depthFormat);
        s_hardware = faster; s_defaults = high;
        // The host supplies its actual native attachments. Do not rewrite the
        // DBC's D24 choice or pretend that Metal currently implements it.
        s_defaults.m_format = &active;
        s_requestedFormat = CGxFormat();
        CVar::Initialize(config.c_str()); RegisterGxCVars(); UpdateGxCVars();
        Require(s_requestedFormat.m_colorFormat == CGxFormat::Fmt_Argb8888 &&
            s_requestedFormat.m_depthFormat == CGxFormat::Fmt_Ds320 &&
            s_requestedFormat.m_refreshRate == desktop.m_refreshRate &&
            s_requestedFormat.m_window && !s_requestedFormat.m_maximize &&
            !std::strcmp(CVar::Lookup("gxApi")->m_stringValue.m_str, "Metal"), "Native display CVar defaults/round trip failed");
        Require(s_requestedFormat.m_size.x == (cycle ? 29 : active.m_size.x) &&
            s_requestedFormat.m_size.y == (cycle ? 17 : active.m_size.y), "Saved native resolution was overwritten on registration");
        Require(device.m_format.m_size.x == scope.active.m_size.x, "Pending resolution changed the active device");
        Set("gxResolution", "29x17"); UpdateGxCVars();
        for (const char* value : {"", "0x10", "-1x10", "20x0", "20x-1", "20x", "20x10extra", "2147483648x10", "20x999999999999999999"}) {
            Set("gxResolution", value); UpdateGxCVars();
            Require(s_requestedFormat.m_size.x == 29 && s_requestedFormat.m_size.y == 17 &&
                !std::strcmp(CVar::Lookup("gxResolution")->m_stringValue.m_str, "29x17"), "Invalid native resolution was published");
        }
        Set("gxApi", "D3D9"); UpdateGxCVars();
        Require(!std::strcmp(CVar::Lookup("gxApi")->m_stringValue.m_str, "Metal"), "Unavailable API was accepted");
        Set("maxFPS", "2"); Set("maxFPSBk", "0");
        Require(GxGetMaxFPS() == 8 && GxGetMaxFPSBk() == 0, "Original frame-limit CVar rules changed");
        for (const auto& setting : {std::pair{"gxDepthBits", "24"}, {"gxColorBits", "16"}, {"gxMultisample", "4"}, {"gxStereoEnabled", "1"}}) {
            SetGxCVars(active); Set("gxStereoEnabled", "0");
            Set(setting.first, setting.second); UpdateGxCVars();
            Require(!device.DeviceSetFormat(s_requestedFormat), "Unsupported native framebuffer format was silently substituted");
            Require(device.m_format.m_colorFormat == scope.active.m_colorFormat && device.m_format.m_depthFormat == scope.active.m_depthFormat,
                "Rejected format mutated the active device");
        }
        Set("gxStereoEnabled", "0"); SetGxCVars(active);
        Require(device.DeviceSetFormat(s_requestedFormat), "Original display CVars could not round trip the native format");
        Set("gxResolution", "29x17"); UpdateGxCVars();
        Require(CVar::SaveAll(), "Native display configuration save failed");
        Require(high.m_format->m_colorFormat == originalFormat.m_colorFormat && high.m_format->m_depthFormat == originalFormat.m_depthFormat,
            "Native attachment selection modified the original DBC format table");
        Shutdown(); g_videoHardwareDB.Free(); g_videoHardwareDB.Reset();
    }
    std::puts("Original display settings: real generic hardware defaults, native CVar registration/save/reload, latched changes, invalid dimensions and unsupported format rejection passed");
}
