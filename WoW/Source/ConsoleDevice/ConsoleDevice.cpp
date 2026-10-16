#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "ConsoleDevice.h"
#include "ConsoleDetect.h"
#include "Engine/Source/Base/CmdLine.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxDevice/Types.h"
#include "Engine/Source/Gx/CGxFormat.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/Hardware.h"
#include "WoW/Source/Ui/GameUI.h"

// STATIC: 0x00CABAC8
static int32_t s_consoleGxOverrideSet[GxOverrides_Last];

// STATIC: 0x00CABAF0
DefaultSettings s_defaults;

// STATIC: 0x00CABB38
Hardware s_hardware;

// STATIC: 0x00CABB7C
static uint32_t s_consoleGxOverrideVal[GxOverrides_Last];

// STATIC: 0x00CABBB8
char s_windowTitle[256];

// STATIC: 0x00CABCB8
static CGxDevice* s_device;

// STATIC: 0x00CABCBC
static bool s_hardwareDetected;

// STATIC: 0x00CABCBD
static bool s_hwChanged;

// STATIC: 0x00CABCBE
static bool s_hwDetect;

// STATIC: 0x00CABCC0
static int32_t s_defaultsApplied;

// STATIC: 0x00CABCC4
static int32_t s_consoleAccessEnabled;

// STATIC: 0x00CABCC8
static TSGrowableArray<CGxMonitorMode> s_gxMonitorModes;

// STATIC: 0x00CABCD8
CGxFormat s_requestedFormat;

// STATIC: 0x00CABD30
static TSGrowableArray<void (*)(OPTION_PAGE)> s_defaultCallbacks;

// STATIC: 0x00CABD40
CGxFormat s_desktopFormat(true, { 800, 600 }, CGxFormat::Fmt_ArgbX888, CGxFormat::Fmt_Ds24X, 60, 1, true, false, true, true, 0);

// STATIC: 0x00CABDA8
CGxFormat s_fallbackFormat(false, { 640, 480 }, CGxFormat::Fmt_Rgb565, CGxFormat::Fmt_Ds160, 60, 1, true, false, true, true, 0);

// STATIC: 0x00CABE00
CGxFormat s_lastGoodFormat;

// STATIC: 0x00ADBE0C
static const uint32_t s_formatToBpp[] = { 16, 32, 32, 32 };

// VERIFY: 992d9d20 auto
// OFFSET: 0x00769220
int32_t ConsoleDeviceExists() {
    return s_device != nullptr;
}

// VERIFY: 6e7fe291 auto
// OFFSET: 0x00769230
int32_t IsConsoleEnabled() {
    return s_consoleAccessEnabled;
}

// VERIFY: e1f357be auto
// OFFSET: 0x007696D0
void ConsoleGxOverride(const char* list) {
    while (*list) {
        char key[256];
        char value[256];
        SStrTokenize(&list, key, sizeof(key), " ,", nullptr);
        SStrTokenize(&list, value, sizeof(value), " ;", nullptr);

        if (!key[0] || !value[0]) {
            continue;
        }

        uint32_t override = atol(key);
        uint32_t overrideValue = atol(value);

        if (override >= GxOverrides_Last) {
            continue;
        }

        if (override == GxOverride_PixelShader) {
            switch (overrideValue) {
                case 0:
                case 1:
                case 2:
                    overrideValue = 1;
                    break;

                case 3:
                    overrideValue = 2;
                    break;

                case 4:
                    overrideValue = 3;
                    break;

                case 5:
                    overrideValue = 7;
                    break;

                case 6:
                    overrideValue = 8;
                    break;

                case 7:
                    overrideValue = 9;
                    break;

                case 8:
                    overrideValue = 10;
                    break;

                case 10:
                    overrideValue = 12;
                    break;

                case 11:
                    overrideValue = 13;
                    break;

                default:
                    break;
            }
        }

        s_consoleGxOverrideSet[override] = 1;
        s_consoleGxOverrideVal[override] = overrideValue;
    }
}

// VERIFY: 5b8b21e9 manual
// OFFSET: 0x005EEB70
void __fastcall StereoChangedCallback() {
}

// VERIFY: 72262368 auto
// OFFSET: 0x00769D00
Hardware* ConsoleGetHardware() {
    return s_hardwareDetected ? &s_hardware : nullptr;
}

// VERIFY: a072ef73 auto
// OFFSET: 0x00769D10
int32_t ConsoleDeviceHardwareChanged() {
    return s_hwDetect || s_hwChanged;
}

// VERIFY: ce2a0330 auto
// OFFSET: 0x00769D30
DefaultSettings* ConsoleDeviceGetDefaultSettings() {
    return &s_defaults;
}

// VERIFY: d88758a1 auto
// OFFSET: 0x00769D40
void ConsoleDeviceDestroy() {
    g_theGxDevicePtr->RemoveStereoChangedCallback(StereoChangedCallback);
    s_cvVideoOptionsVersion->Update();
    GxDevDestroy(s_device);
    s_device = nullptr;
    GxLogClose();
}

// VERIFY: 9ae0e177 manual
// OFFSET: 0x00769E10
void ValidateFormatMonitor(CGxFormat& format) {
    static C2iVector s_resolutions[] = {
        { 1600, 1200 },
        { 1280, 1024 },
        { 1280, 960 },
        { 1152, 864 },
        { 1024, 768 },
        { 800, 600 },
        { 640, 480 },
    };

    int32_t width = format.m_size.x;
    int32_t height = format.m_size.y;
    int32_t index = -1;
    uint32_t bpp = s_formatToBpp[format.m_colorFormat];
    uint32_t minRefresh = 9999;

    CGxMonitorMode desktop;
    desktop.m_size.x = 0;
    desktop.m_size.y = 0;

    if (!format.m_window || !GxAdapterDesktopMode(desktop) || desktop.m_size.x < format.m_size.x || desktop.m_size.y < format.m_size.y) {
        while (true) {
            uint32_t i;

            for (i = 0; i < s_gxMonitorModes.Count(); i++) {
                if (width == s_gxMonitorModes[i].m_size.x && height == s_gxMonitorModes[i].m_size.y) {
                    break;
                }
            }

            if (i < s_gxMonitorModes.Count()) {
                format.m_size.x = width;
                format.m_size.y = height;
                break;
            }

            do {
                if (++index >= static_cast<int32_t>(sizeof(s_resolutions) / sizeof(s_resolutions[0]))) {
                    goto refresh;
                }
            } while (s_resolutions[index].x >= width || s_resolutions[index].y >= height);

            width = s_resolutions[index].x;
            height = s_resolutions[index].y;
        }
    }

refresh:
    for (uint32_t i = 0; i < s_gxMonitorModes.Count(); i++) {
        CGxMonitorMode& mode = s_gxMonitorModes[i];

        if (format.m_size.x == mode.m_size.x && format.m_size.y == mode.m_size.y && bpp == mode.m_bpp) {
            if (mode.m_refreshRate < minRefresh) {
                minRefresh = mode.m_refreshRate;
            }

            if (format.m_refreshRate == mode.m_refreshRate) {
                return;
            }
        }
    }

    if (minRefresh == 9999) {
        minRefresh = 60;
        GxLog("ValidateFormatMonitor(): unable to find monitor refresh");
    }

    GxLog("ValidateFormatMonitor(): invalid refresh rate %d, set to %d", format.m_refreshRate, minRefresh);
    format.m_refreshRate = minRefresh;
}

// VERIFY: 35de767a auto
// OFFSET: 0x0076A130
void ConsoleDeviceDefaults(OPTION_PAGE page) {
    ConsoleDetectSetDefaults(s_defaults, s_hardware);
    s_cvFixedFunction->Set("0", true, false, false, true);

    if (page == OPTION_PAGE_0) {
        s_requestedFormat = *s_defaults.m_format;
        SetGxCVars(s_requestedFormat);
        CCGxRestart(nullptr, nullptr);
    }

    for (uint32_t i = 0; i < s_defaultCallbacks.Count(); i++) {
        s_defaultCallbacks[i](page);
    }
}

// VERIFY: 653cadbd auto
// OFFSET: 0x0076AAB0
void AddConsoleDeviceDefaultCallback(void (*callback)(OPTION_PAGE page)) {
    s_defaultCallbacks.New(callback);

    if (s_defaultsApplied) {
        for (uint32_t page = OPTION_PAGE_0; page < NUM_OPTION_PAGES; page++) {
            callback(static_cast<OPTION_PAGE>(page));
        }
    }
}

// VERIFY: 8f1edb41 manual
// OFFSET: 0x0076AAF0
void RemoveConsoleDeviceDefaultCallback(void (*callback)(OPTION_PAGE page)) {
    uint32_t count = s_defaultCallbacks.Count();

    for (uint32_t i = 0; i < count; i++) {
        if (s_defaultCallbacks[i] == callback) {
            s_defaultCallbacks[i] = s_defaultCallbacks[--count];
            s_defaultCallbacks.SetCount(count);
        }
    }
}

// VERIFY: 9ac5dfc9 manual
// OFFSET: 0x0076AB80
void ConsoleDeviceInitialize(const char* title, bool unused_a2, bool unused_a3) {
    GxLogOpen();

    s_cvHwDetect = CVar::Register("hwDetect", "do hardware detection", 1, "1", nullptr, CATEGORY_GRAPHICS, false, nullptr, false);
    ConsoleDetectDetectHardware(s_hardware, s_hwChanged);
    s_hardwareDetected = true;

    if (CmdLineGetBool(static_cast<CMDOPT>(CMD_HW_DETECT)) || s_cvHwDetect->m_intValue) {
        s_hwDetect = true;
        s_cvHwDetect->Set("0", true, false, false, true);
    } else {
        s_hwDetect = false;
    }

    s_consoleAccessEnabled = CmdLineGetBool(static_cast<CMDOPT>(CMD_CONSOLE));

    ConsoleDetectSetDefaultsFormat(s_defaults, s_hardware);
    RegisterGxCVars();
    ConsoleCommandRegister("gxRestart", CCGxRestart, CATEGORY_GRAPHICS, nullptr);

    GxAdapterMonitorModes(s_gxMonitorModes);
    ValidateFormatMonitor(s_fallbackFormat);

    CGxMonitorMode desktop;
    desktop.m_size.x = 0;
    desktop.m_size.y = 0;

    if (GxAdapterDesktopMode(desktop)) {
        s_desktopFormat.m_size.x = desktop.m_size.x;
        s_desktopFormat.m_size.y = desktop.m_size.y;
        s_desktopFormat.m_colorFormat = desktop.m_bpp > 16 ? CGxFormat::Fmt_ArgbX888 : CGxFormat::Fmt_Rgb565;
        s_desktopFormat.m_refreshRate = desktop.m_refreshRate;
    }

    GxLog("ConsoleDeviceInitialize(): hwDetect = %d, hwChanged = %d", s_hwDetect, s_hwChanged);

    if (CmdLineGetBool(static_cast<CMDOPT>(CMD_RES_800x600))) {
        s_requestedFormat.m_size.x = 800;
        s_requestedFormat.m_size.y = 600;
    } else if (CmdLineGetBool(static_cast<CMDOPT>(CMD_RES_1024x768))) {
        s_requestedFormat.m_size.x = 1024;
        s_requestedFormat.m_size.y = 768;
    } else if (CmdLineGetBool(static_cast<CMDOPT>(CMD_RES_1280x960))) {
        s_requestedFormat.m_size.x = 1280;
        s_requestedFormat.m_size.y = 960;
    } else if (CmdLineGetBool(static_cast<CMDOPT>(CMD_RES_1280x1024))) {
        s_requestedFormat.m_size.x = 1280;
        s_requestedFormat.m_size.y = 1024;
    } else if (CmdLineGetBool(static_cast<CMDOPT>(CMD_RES_1600x1200))) {
        s_requestedFormat.m_size.x = 1600;
        s_requestedFormat.m_size.y = 1200;
    }

    if (s_cvFixedFunction->m_intValue) {
        s_requestedFormat.m_fixedFunction[4] = 0;
        s_requestedFormat.m_fixedFunction[0] = 0;
    }

    if (s_hwDetect || s_hwChanged) {
        ConsoleDetectSetDefaults(s_defaults, s_hardware);
        s_cvFixedFunction->Set("0", true, false, false, true);
        s_requestedFormat = *s_defaults.m_format;
        s_requestedFormat.m_window = s_cvGxWindow->m_intValue != 0;
        s_requestedFormat.m_maximize = s_cvGxMaximize->m_intValue != 0;
        SetGxCVars(s_requestedFormat);
    }

    EGxApi api = GxDefaultApi();
    CVar* apiVar = CVar::Lookup("gxApi");

    if (apiVar) {
        const char* apiName = apiVar->m_stringValue.m_str;

        for (int32_t i = GxApi_OpenGl; i < GxApis_Last; i++) {
            if (GxApiSupported(static_cast<EGxApi>(i)) && !SStrCmpI(apiName, g_gxApiNames[i], STORM_MAX_STR)) {
                api = static_cast<EGxApi>(i);
                break;
            }
        }
    }

    if (CmdLineGetBool(CMD_OPENGL) && GxApiSupported(GxApi_OpenGl)) {
        api = GxApi_OpenGl;
    }

    if (CmdLineGetBool(CMD_D3D) && GxApiSupported(GxApi_D3d9)) {
        api = GxApi_D3d9;
    }

    if (CmdLineGetBool(CMD_D3D9EX) && GxApiSupported(GxApi_D3d9Ex)) {
        api = GxApi_D3d9Ex;
    }

    s_requestedFormat.m_fixLag = s_cvGxFixLag->m_intValue != 0;
    s_requestedFormat.m_hwTnL = !CmdLineGetBool(CMD_SW_TNL);

    bool window = s_cvGxWindow->m_intValue != 0;

    if (CmdLineGetBool(CMD_FULL_SCREEN)) {
        window = false;
    } else if (CmdLineGetBool(static_cast<CMDOPT>(CMD_WINDOWED))) {
        window = true;
    }

    s_requestedFormat.m_window = window;
    s_desktopFormat.m_window = window;

    bool desktopTried = false;
    CGxFormat format = s_requestedFormat;
    ValidateFormatMonitor(format);

    for (s_device = CreateGxDevice(api, OsWindowProc, format); !s_device; s_device = CreateGxDevice(api, OsWindowProc, format)) {
        if (format.m_multisampleCount > 1) {
            if (format.m_multisampleCount - 2 <= 1) {
                format.m_multisampleCount = 1;
            } else {
                format.m_multisampleCount -= 2;
            }
        } else if (format.m_colorFormat > CGxFormat::Fmt_Rgb565) {
            if (format.m_depthFormat <= CGxFormat::Fmt_Ds160) {
                format.m_depthFormat = format.m_colorFormat == CGxFormat::Fmt_ArgbX888 ? CGxFormat::Fmt_Ds160 : CGxFormat::Fmt_Ds320;
                format.m_colorFormat = static_cast<CGxFormat::Format>(format.m_colorFormat - 1);
            } else {
                format.m_depthFormat = static_cast<CGxFormat::Format>(format.m_depthFormat - 1);
            }
        } else {
            if (desktopTried) {
                GxLog("ConsoleDeviceInitialize(): no output device available!");

                Startup_StringsRec* titleRec = g_startup_StringsDB.GetRecord(1);
                const char* caption = titleRec ? titleRec->m_message : "World of Warcraft";
                const char* message;

                if (api == GxApi_D3d9 || api == GxApi_D3d9Ex) {
                    Startup_StringsRec* rec = g_startup_StringsDB.GetRecord(2);
                    message = rec ? rec->m_message : "World of Warcraft was unable to start up 3D acceleration. Please make sure DirectX 9.0c is installed and your video drivers are up-to-date.";
                } else {
                    Startup_StringsRec* rec = g_startup_StringsDB.GetRecord(3);
                    message = rec ? rec->m_message : "World of Warcraft was unable to start up 3D acceleration.";
                }

                OsGuiMessageBox(nullptr, 0, message, caption);
                GxLogClose();
                exit(0);
            }

            format = s_desktopFormat;
            desktopTried = true;
        }

        ValidateFormatMonitor(format);
    }

    s_requestedFormat = format;
    s_lastGoodFormat = format;
    SetGxCVars(s_requestedFormat);

    if (g_theGxDevicePtr->Caps().m_numTmus < 2) {
        GxDevDestroy(s_device);
        GxLog("ConsoleDeviceInitialize(): output device does not have dual TMUs!");
    } else if (!g_theGxDevicePtr->Caps().m_numStreams) {
        GxDevDestroy(s_device);
        GxLog("ConsoleDeviceInitialize(): output device has 0 streams");
    } else {
        switch (g_theGxDevicePtr->DeviceApi()) {
            case GxApi_OpenGl:
            case GxApi_GLL:
                ConsoleGxOverride(s_hardware.m_videoHw->m_oglOverrides);
                break;

            case GxApi_D3d9:
            case GxApi_D3d9Ex:
                ConsoleGxOverride(s_hardware.m_videoHw->m_d3DOverrides);
                break;

            default:
                break;
        }

        ConsoleGxOverride(CmdLineGetString(static_cast<CMDOPT>(CMD_GX_OVERRIDE)));

        for (uint32_t i = 0; i < GxOverrides_Last; i++) {
            if (s_consoleGxOverrideSet[i]) {
                g_theGxDevicePtr->DeviceOverride(static_cast<EGxOverride>(i), s_consoleGxOverrideVal[i]);
            }
        }

        OsGuiSetGxWindow(g_theGxDevicePtr->DeviceWindow());
        SStrCopy(s_windowTitle, title ? title : "", sizeof(s_windowTitle));

        void* window = g_theGxDevicePtr->DeviceWindow();

        if (window) {
            OsGuiSetWindowTitle(window, s_windowTitle);
        }

        if (s_hwDetect || s_hwChanged) {
            s_defaults.unk_0019 = g_theGxDevicePtr->Caps().m_shaderTargets[4] != 0;
        }

        char version[32];
        SStrPrintf(version, sizeof(version), "%d", 3);
        s_cvVideoOptionsVersion->Set(version, true, false, false, true);

        RegisterStereoCVars();
        OsSetSleepInBackground(1);
        OsSetBackgroundSleepMs(250);

        return;
    }

    Startup_StringsRec* titleRec = g_startup_StringsDB.GetRecord(1);
    const char* caption = titleRec ? titleRec->m_message : "World of Warcraft";
    Startup_StringsRec* rec = g_startup_StringsDB.GetRecord(4);
    const char* message = rec ? rec->m_message : "Your 3D accelerator card is not supported by World of Warcraft. Please install a 3D accelerator card with dual-TMU support.";

    OsGuiMessageBox(nullptr, 0, message, caption);
    GxLogClose();
    exit(0);
}
