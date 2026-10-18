#include <cstdint>
#include "ConsoleDevice.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxFormat.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Services/Texture.h"
#include "WoW/Source/Ui/GameUI.h"
#if defined(__APPLE__)
#include "Engine/Source/Gx/Metal/NativeHost.h"
#endif

// VERIFY: 6674b551 auto
// OFFSET: 0x00769FF0
int32_t CCGxRestart(const char* command, const char* arguments) {
#if defined(__APPLE__)
    if (GxMetal::DeferDisplayRestart([] { CCGxRestart(nullptr, nullptr); })) return 1;
#endif
    OnGxRestartClearWorld();
    ValidateFormatMonitor(s_requestedFormat);

    if (g_theGxDevicePtr->DeviceSetFormat(s_requestedFormat)) {
#if defined(__APPLE__)
        s_requestedFormat = g_theGxDevicePtr->m_format;
#endif
        s_lastGoodFormat = s_requestedFormat;
        UpdateGxCVars();
    } else {
        ConsoleWrite("unable to set requested display mode", DEFAULT_COLOR);
        s_requestedFormat = s_lastGoodFormat;

        if (!g_theGxDevicePtr->DeviceSetFormat(s_requestedFormat)) {
            ConsoleWrite("unable to set last good mode", DEFAULT_COLOR);
            s_requestedFormat = s_desktopFormat;

            if (!g_theGxDevicePtr->DeviceSetFormat(s_requestedFormat)) {
                ConsoleWrite("unable to set default format", DEFAULT_COLOR);
                s_requestedFormat = s_fallbackFormat;

                if (!g_theGxDevicePtr->DeviceSetFormat(s_requestedFormat)) {
                    goto done;
                }
            }

            s_lastGoodFormat = s_requestedFormat;
        }

        SetGxCVars(s_requestedFormat);
    }

done:
#if defined(__APPLE__)
    // AppKit may round requested pixels to native window points. CVars and
    // rollback state must describe the accepted size, including fallback.
    s_requestedFormat = s_lastGoodFormat = g_theGxDevicePtr->m_format;
    SetGxCVars(s_requestedFormat);
#endif
    OsGuiSetGxWindow(g_theGxDevicePtr->DeviceWindow());
    OsGuiSetWindowTitle(g_theGxDevicePtr->DeviceWindow(), s_windowTitle);
    TextureNotifyGxRestart();

    return 1;
}
