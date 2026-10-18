#include <cstdint>
#include <cstdio>
#include <cstring>
#if defined(__APPLE__)
#include <charconv>
#endif
#include "ConsoleDevice.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxDevice/Types.h"
#include "Engine/Source/Gx/CGxFormat.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/Hardware.h"

// STATIC: 0x00CABA98
static CVar* s_cvGxStereoEnabled;

// STATIC: 0x00CABA9C
static CVar* s_cvGxRefresh;

// STATIC: 0x00CABAA0
CVar* s_cvGxMaximize;

// STATIC: 0x00CABAA4
static CVar* s_cvGxMultisample;

// STATIC: 0x00CABAA8
static CVar* s_cvGxCursor;

// STATIC: 0x00CABAAC
static CVar* s_cvGxStereoSeparation;

// STATIC: 0x00CABAB0
static CVar* s_cvGxMultisampleQuality;

// STATIC: 0x00CABAB4
static CVar* s_cvGxResolution;

// STATIC: 0x00CABAB8
CVar* s_cvHwDetect;

// STATIC: 0x00CABABC
static CVar* s_cvGxOverride;

// STATIC: 0x00CABAC0
CVar* s_cvGxFixLag;

// STATIC: 0x00CABAC4
static CVar* s_cvMaxFPS;

// STATIC: 0x00CABAEC
static CVar* s_cvGxVSync;

// STATIC: 0x00CABB30
CVar* s_cvVideoOptionsVersion;

// STATIC: 0x00CABB34
static CVar* s_cvGxStereoConvergence;

// STATIC: 0x00CABB6C
static CVar* s_cvMaxFPSBk;

// STATIC: 0x00CABB70
static CVar* s_cvGxTripleBuffer;

// STATIC: 0x00CABB74
static CVar* s_cvGxDepthBits;

// STATIC: 0x00CABB78
static CVar* s_cvGxColorBits;

// STATIC: 0x00CABBA0
static CVar* s_cvGxApi;

// STATIC: 0x00CABBA4
static CVar* s_cvGxAspect;

// STATIC: 0x00CABBA8
CVar* s_cvFixedFunction;

// STATIC: 0x00CABBAC
static CVar* s_cvWidescreen;

// STATIC: 0x00CABBB0
CVar* s_cvGxWindow;

// STATIC: 0x00CABBB4
static CVar* s_cvWindowResizeLock;

// VERIFY: b2a180e4 auto
// OFFSET: 0x00769240
static bool CVGxColorBitsCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    switch (SStrToInt(value)) {
        case 16:
            s_requestedFormat.m_colorFormat = CGxFormat::Fmt_Rgb565;
            ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
            return true;

        case 24:
#if defined(__APPLE__)
            // gxColorBits counts RGB bits, including for Fmt_Argb8888 in the
            // original s_formatToInt table. Native drawables retain alpha.
            s_requestedFormat.m_colorFormat = CGxFormat::Fmt_Argb8888;
#else
            s_requestedFormat.m_colorFormat = CGxFormat::Fmt_ArgbX888;
#endif
            ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
            return true;

        case 30:
            s_requestedFormat.m_colorFormat = CGxFormat::Fmt_Argb2101010;
            ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
            return true;

        default:
            ConsoleWrite("Color bits must be 16, 24, or 30", DEFAULT_COLOR);
            return false;
    }
}

// VERIFY: 1b589d62 auto
// OFFSET: 0x007692D0
static bool CVGxDepthBitsCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    switch (SStrToInt(value)) {
        case 16:
            s_requestedFormat.m_depthFormat = CGxFormat::Fmt_Ds160;
            ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
            return true;

        case 24:
            s_requestedFormat.m_depthFormat = CGxFormat::Fmt_Ds24X;
            ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
            return true;

        case 32:
            s_requestedFormat.m_depthFormat = CGxFormat::Fmt_Ds320;
            ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
            return true;

        default:
            ConsoleWrite("Depth bits must be 16, 24, or 32", DEFAULT_COLOR);
            return false;
    }
}

// VERIFY: fd54fc0b manual
// OFFSET: 0x00769360
static bool CVGxTripleBufferCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    uint32_t tripleBuffer = SStrToInt(value);

    if (tripleBuffer >= 2) {
        ConsoleWrite("TripleBuffer must be 0 or 1", DEFAULT_COLOR);
        return false;
    }

    s_requestedFormat.m_backBufferCount = (tripleBuffer != 0) + 1;
    ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);

    return true;
}

// VERIFY: 1b35a664 auto
// OFFSET: 0x007693B0
static bool CVGxApiCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    for (int32_t api = GxApi_OpenGl; api < GxApis_Last; api++) {
        if (GxApiSupported(static_cast<EGxApi>(api)) && !SStrCmpI(value, g_gxApiNames[api], STORM_MAX_STR)) {
            ConsoleWrite("GxApi set pending gxRestart", DEFAULT_COLOR);
            return true;
        }
    }

    char message[1024] = "unsupported api, must be one of ";
    bool first = true;

    for (int32_t api = GxApi_OpenGl; api < GxApis_Last; api++) {
        if (!GxApiSupported(static_cast<EGxApi>(api))) {
            continue;
        }

        if (!first) {
            strcat(message, ", ");
        }

        strcat(message, "'");
        strcat(message, g_gxApiNames[api]);
        strcat(message, "'");
        first = false;
    }

    ConsoleWrite(message, DEFAULT_COLOR);

    return false;
}

// VERIFY: 05847e9e auto
// OFFSET: 0x00769520
static bool CVGxVSyncCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    s_requestedFormat.m_vsync = SStrToInt(value);
    ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
    return true;
}

// VERIFY: b82579a5 auto
// OFFSET: 0x00769550
static bool CVGxWindowCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    s_requestedFormat.m_window = SStrToInt(value) != 0;
    ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
    return true;
}

// VERIFY: 88a23b6c auto
// OFFSET: 0x00769580
static bool CVGxAspectCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    s_requestedFormat.m_aspect = SStrToInt(value) != 0;
    ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
    return true;
}

// VERIFY: f09230be auto
// OFFSET: 0x007695B0
static bool CVGxMaximizeCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    s_requestedFormat.m_maximize = SStrToInt(value) != 0;
    ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
    return true;
}

// VERIFY: 5986dd1d auto
// OFFSET: 0x007695E0
static bool CVGxCursorCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    s_requestedFormat.m_hwCursor = SStrToInt(value) != 0;
    ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
    return true;
}

// VERIFY: 93f25d19 manual
// OFFSET: 0x00769610
static bool CVGxMultisampleCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    int32_t multisample = SStrToInt(value);

    if (multisample <= 1) {
        multisample = 1;
    } else if (multisample >= 16) {
        multisample = 16;
    }

    s_requestedFormat.m_multisampleCount = multisample;
    ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);

    return true;
}

// VERIFY: 1bcaf62f auto
// OFFSET: 0x00769650
static bool CVGxMultisampleQualityCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    float quality = SStrToFloat(value);

    if (quality < 0.0f) {
        quality = 0.0f;
    } else if (quality >= 1.0f) {
        quality = 1.0f;
    }

    s_requestedFormat.m_multisampleQuality = quality;
    ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);

    return true;
}

// VERIFY: c1f4a85d auto
// OFFSET: 0x007696A0
static bool CVGxFixLagCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    s_requestedFormat.m_fixLag = SStrToInt(value) != 0;
    ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
    return true;
}

// VERIFY: 34058bc1 auto
// OFFSET: 0x00769810
static bool CVGxOverrideCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    ConsoleGxOverride(value);
    return true;
}

// VERIFY: b867ad47 auto
// OFFSET: 0x00769830
static bool CVGxMaxFPSCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    uint32_t maxFPS = SStrToInt(value);

    if (maxFPS - 1 <= 6) {
        maxFPS = 8;
    }

    GxSetMaxFPS(maxFPS);

    return true;
}

// VERIFY: f691ee8b auto
// OFFSET: 0x00769860
static bool CVGxMaxFPSBkCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    uint32_t maxFPSBk = SStrToInt(value);

    if (maxFPSBk - 1 <= 6) {
        maxFPSBk = 8;
    }

    GxSetMaxFPSBk(maxFPSBk);

    return true;
}

// VERIFY: 7d022997 auto
// OFFSET: 0x00769890
static bool CVGxWindowResizeLockCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    OsInputSetWindowResizeLock(SStrToInt(value));
    return true;
}

// VERIFY: 1bd7dbb1 auto
// OFFSET: 0x007698B0
bool UpdateGxCVars() {
    s_cvGxColorBits->Update();
    s_cvGxDepthBits->Update();
    s_cvGxWindow->Update();
    s_cvGxResolution->Update();
    s_cvGxRefresh->Update();
    s_cvGxTripleBuffer->Update();
    s_cvGxApi->Update();
    s_cvGxVSync->Update();
    s_cvGxAspect->Update();
    s_cvGxMaximize->Update();
    s_cvGxCursor->Update();
    s_cvGxMultisample->Update();
    s_cvGxMultisampleQuality->Update();
    return s_cvGxFixLag->Update();
}

// VERIFY: 716d764e auto
// OFFSET: 0x00769950
bool SetGxCVars(const CGxFormat& format) {
    char value[1024];

    s_cvGxColorBits->Set(s_formatToInt[format.m_colorFormat], true, false, false, true);
    s_cvGxDepthBits->Set(s_formatToInt[format.m_depthFormat], true, false, false, true);

    SStrPrintf(value, sizeof(value), "%d", format.m_window);
    s_cvGxWindow->Set(value, true, false, false, true);

    SStrPrintf(value, sizeof(value), "%dx%d", format.m_size.x, format.m_size.y);
    s_cvGxResolution->Set(value, true, false, false, true);

    SStrPrintf(value, sizeof(value), "%d", format.m_refreshRate);
    s_cvGxRefresh->Set(value, true, false, false, true);

    s_cvGxTripleBuffer->Set(format.m_backBufferCount > 1 ? "1" : "0", true, false, false, true);

    SStrPrintf(value, sizeof(value), "%d", format.m_vsync);
    s_cvGxVSync->Set(value, true, false, false, true);

    SStrPrintf(value, sizeof(value), "%d", format.m_aspect);
    s_cvGxAspect->Set(value, true, false, false, true);

    SStrPrintf(value, sizeof(value), "%d", format.m_maximize);
    s_cvGxMaximize->Set(value, true, false, false, true);

    SStrPrintf(value, sizeof(value), "%d", format.m_hwCursor);
    s_cvGxCursor->Set(value, true, false, false, true);

    SStrPrintf(value, sizeof(value), "%d", format.m_multisampleCount);
    s_cvGxMultisample->Set(value, true, false, false, true);

    SStrPrintf(value, sizeof(value), "%f", format.m_multisampleQuality);
    s_cvGxMultisampleQuality->Set(value, true, false, false, true);

    SStrPrintf(value, sizeof(value), "%d", format.m_fixLag);
    s_cvGxFixLag->Set(value, true, false, false, true);

    return UpdateGxCVars();
}

// VERIFY: a95866c0 auto
// OFFSET: 0x00769C00
static bool CVGxStereoEnabledCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    s_requestedFormat.m_stereoEnabled = SStrToInt(value) == 1;
    ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
    return true;
}

// VERIFY: 30f5328f auto
// OFFSET: 0x00769C30
static bool CVGxStereoConvergenceCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    g_theGxDevicePtr->StereoSetConvergence(SStrToFloat(value));
    return true;
}

// VERIFY: 75b13ba9 auto
// OFFSET: 0x00769C60
static bool CVGxStereoSeparationCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    g_theGxDevicePtr->StereoSetSeparation(SStrToFloat(value));
    return true;
}

// VERIFY: dc6233ea auto
// OFFSET: 0x0076A220
static bool CVGxResolutionCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    int32_t width = -1;
    int32_t height = -1;
#if defined(__APPLE__)
    // sscanf's overflowing %d conversion is undefined; never publish invalid
    // drawable dimensions from a saved/user-provided native configuration.
    const char* end = value + std::strlen(value);
    const auto x = std::from_chars(value, end, width);
    if (x.ec != std::errc() || x.ptr == end || (*x.ptr != 'x' && *x.ptr != 'X')) return false;
    const auto y = std::from_chars(x.ptr + 1, end, height);
    if (y.ec != std::errc() || y.ptr != end || width <= 0 || height <= 0) return false;
#else
    char separator;
    sscanf(value, "%d%c%d", &width, &separator, &height);
#endif

    if (s_requestedFormat.m_window) {
        s_requestedFormat.m_size.x = width;
        s_requestedFormat.m_size.y = height;
        ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
        return true;
    }

    TSGrowableArray<C2iVector> resolutions;
    C2iVector resolution;

    if (s_cvWidescreen->m_intValue) {
        TSGrowableArray<CGxMonitorMode> modes;
        GxAdapterMonitorModes(modes);

        C2iVector last = { 0, 0 };

        for (uint32_t i = 0; i < modes.Count(); i++) {
            const C2iVector& size = modes[i].m_size;

            if (static_cast<float>(size.x) / size.y >= 1.248f && size.x >= 640 && size.y >= 480 && (size.x != last.x || size.y != last.y)) {
                last = size;
                resolutions.New(last);
            }
        }
    }

    if (!resolutions.Count()) {
        resolution.x = 640;
        resolution.y = 480;
        resolutions.New(resolution);
        resolution.x = 800;
        resolution.y = 600;
        resolutions.New(resolution);
        resolution.x = 1024;
        resolution.y = 768;
        resolutions.New(resolution);
        resolution.x = 1152;
        resolution.y = 864;
        resolutions.New(resolution);
        resolution.x = 1280;
        resolution.y = 960;
        resolutions.New(resolution);
        resolution.x = 1280;
        resolution.y = 1024;
        resolutions.New(resolution);
        resolution.x = 1600;
        resolution.y = 1200;
        resolutions.New(resolution);
    }

    uint32_t i;

    for (i = 0; i < resolutions.Count(); i++) {
        if (width == resolutions[i].x && height == resolutions[i].y) {
            break;
        }
    }

    if (i == resolutions.Count()) {
        char message[256] = "invalid resolution, must be one of ";

        for (uint32_t j = 0; j < resolutions.Count(); j++) {
            if (j) {
                strcat(message, ", ");
            }

            if (SStrLen(message) > 100) {
                ConsoleWrite(message, DEFAULT_COLOR);
                message[0] = '\0';
            }

            char entry[16];
            SStrPrintf(entry, sizeof(entry), "%dx%d", resolutions[j].x, resolutions[j].y);
            strcat(message, entry);
        }

        ConsoleWrite(message, DEFAULT_COLOR);

        return false;
    }

    s_requestedFormat.m_size.x = width;
    s_requestedFormat.m_size.y = height;
    ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);

    return true;
}

// VERIFY: ec771f1a manual
// OFFSET: 0x0076A580
static bool CVGxRefreshCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    uint32_t refreshRate = SStrToUnsigned(value);

    TSGrowableArray<CGxMonitorMode> modes;
    GxAdapterMonitorModes(modes);

    uint32_t i;

    for (i = 0; i < modes.Count(); i++) {
        if (refreshRate == modes[i].m_refreshRate) {
            break;
        }
    }

    if (i == modes.Count()) {
        ConsoleWrite("Unsupported refresh rate", DEFAULT_COLOR);
        return false;
    }

    s_requestedFormat.m_refreshRate = refreshRate;
    ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);

    return true;
}

// VERIFY: 05a0817f auto
// OFFSET: 0x008A1420
static bool CVVideoOptionsVersionCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    return true;
}

// VERIFY: 62bcef94 auto
// OFFSET: 0x00769C90
void RegisterStereoCVars() {
    s_cvGxStereoConvergence = CVar::Register("gxStereoConvergence", "Set stereoscopic rendering convergence depth", 1, "1", CVGxStereoConvergenceCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvGxStereoSeparation = CVar::Register("gxStereoSeparation", "Set stereoscopic rendering separation percentage", 1, "25", CVGxStereoSeparationCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    g_theGxDevicePtr->AddStereoChangedCallback(StereoChangedCallback);
}

// VERIFY: 8dcc523c auto
// OFFSET: 0x0076A630
void RegisterGxCVars() {
    const CGxFormat& format = *s_defaults.m_format;
#if defined(__APPLE__)
    const bool window = format.m_window != 0;
#else
    bool window = s_locale == 4;
#endif
    char value[260];

    s_cvWidescreen = CVar::Register("widescreen", "Allow widescreen support", 0, "1", nullptr, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvGxWindow = CVar::Register("gxWindow", "toggle fullscreen/window", 3, window ? "1" : "0", CVGxWindowCallback, CATEGORY_GRAPHICS, false, nullptr, false);
#if defined(__APPLE__)
    const bool maximize = format.m_maximize != 0;
#else
    const bool maximize = window;
#endif
    s_cvGxMaximize = CVar::Register("gxMaximize", "maximize game window", 3, maximize ? "1" : "0", CVGxMaximizeCallback, CATEGORY_GRAPHICS, false, nullptr, false);

    SStrPrintf(value, sizeof(value), "%s", s_formatToInt[format.m_colorFormat]);
    s_cvGxColorBits = CVar::Register("gxColorBits", "color bits", 3, value, CVGxColorBitsCallback, CATEGORY_GRAPHICS, false, nullptr, false);

    SStrPrintf(value, sizeof(value), "%s", s_formatToInt[format.m_depthFormat]);
    s_cvGxDepthBits = CVar::Register("gxDepthBits", "depth bits", 3, value, CVGxDepthBitsCallback, CATEGORY_GRAPHICS, false, nullptr, false);

    SStrPrintf(value, sizeof(value), "%dx%d", format.m_size.x, format.m_size.y);
    s_cvGxResolution = CVar::Register("gxResolution", "resolution", 3, value, CVGxResolutionCallback, CATEGORY_GRAPHICS, false, nullptr, false);

#if defined(__APPLE__)
    SStrPrintf(value, sizeof(value), "%u", format.m_refreshRate);
    const char* refresh = value;
#else
    const char* refresh = "75";
#endif
    s_cvGxRefresh = CVar::Register("gxRefresh", "refresh rate", 3, refresh, CVGxRefreshCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvGxTripleBuffer = CVar::Register("gxTripleBuffer", "triple buffer", 3, "0", CVGxTripleBufferCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvGxApi = CVar::Register("gxApi", "graphics api", 3, g_gxApiNames[GxDefaultApi()], CVGxApiCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvGxVSync = CVar::Register("gxVSync", "vsync on or off", 3, "1", CVGxVSyncCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvGxAspect = CVar::Register("gxAspect", "constrain window aspect", 3, "1", CVGxAspectCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvGxCursor = CVar::Register("gxCursor", "toggle hardware cursor", 3, "1", CVGxCursorCallback, CATEGORY_GRAPHICS, false, nullptr, false);

    SStrPrintf(value, sizeof(value), "%d", s_hardware.m_videoHw->m_multisample);
    s_cvGxMultisample = CVar::Register("gxMultisample", "multisample", 3, value, CVGxMultisampleCallback, CATEGORY_GRAPHICS, false, nullptr, false);

    s_cvGxMultisampleQuality = CVar::Register("gxMultisampleQuality", "multisample quality", 3, "0.0", CVGxMultisampleQualityCallback, CATEGORY_GRAPHICS, false, nullptr, false);

    SStrPrintf(value, sizeof(value), "%d", s_hardware.m_videoHw->m_fixLag);
    s_cvGxFixLag = CVar::Register("gxFixLag", "prevent cursor lag", 3, value, CVGxFixLagCallback, CATEGORY_GRAPHICS, false, nullptr, false);

    s_cvGxStereoEnabled = CVar::Register("gxStereoEnabled", "Enable stereoscopic rendering", 1, "0", CVGxStereoEnabledCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvGxOverride = CVar::Register("gxOverride", "gx overrides", 1, "", CVGxOverrideCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvMaxFPS = CVar::Register("maxFPS", "Set FPS limit", 1, "200", CVGxMaxFPSCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvMaxFPSBk = CVar::Register("maxFPSBk", "Set background FPS limit", 1, "30", CVGxMaxFPSBkCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvVideoOptionsVersion = CVar::Register("videoOptionsVersion", "Video options version", 3, "0", CVVideoOptionsVersionCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvWindowResizeLock = CVar::Register("windowResizeLock", "prevent resizing in windowed mode", 1, "0", CVGxWindowResizeLockCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvFixedFunction = CVar::Register("fixedFunction", "Force fixed function rendering", 3, "0", nullptr, CATEGORY_GRAPHICS, false, nullptr, false);
}
