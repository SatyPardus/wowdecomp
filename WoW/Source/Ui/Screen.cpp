#include "Screen.h"
#include <cstdint>
#include <cstdio>
#include <cmath>
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"

// VERIFY: a937de73 auto
// OFFSET: 0x0050F7C0
float Screen::GetAspectRatio() {
    CVar* widescreen = CVar::Lookup("widescreen");
    CVar* resolution = CVar::Lookup("gxResolution");
    int32_t width = 4;
    int32_t height = 3;
    char separator;

    if (widescreen && widescreen->m_intValue) {
        if (g_theGxDevicePtr) {
            // The display window can differ from gxResolution after a resize.
            // Use the default window rather than an offscreen render target.
            const CRect& window = g_theGxDevicePtr->DeviceDefWindow();
            float displayWidth = window.maxX - window.minX;
            float displayHeight = window.maxY - window.minY;
            if (std::isfinite(displayWidth) && std::isfinite(displayHeight) && displayWidth > 0.0f && displayHeight > 0.0f) {
                return displayWidth / displayHeight;
            }
        }

        if (resolution && sscanf(resolution->m_stringValue.m_str, "%d%c%d", &width, &separator, &height) == 3 && width > 0 && height > 0) {
            return static_cast<float>(width) / height;
        }
    }

    return 4.0f / 3.0f;
}
