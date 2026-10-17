#if defined(__APPLE__)

#include <cstdlib>
#include <cstring>
#include <SDL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"

static int CGxMonitorModeSort(const void* a, const void* b) {
    const CGxMonitorMode* modeA = static_cast<const CGxMonitorMode*>(a);
    const CGxMonitorMode* modeB = static_cast<const CGxMonitorMode*>(b);

    return modeA->m_size.x * modeA->m_size.y - modeB->m_size.x * modeB->m_size.y;
}

static void AddFormat(TSGrowableArray<CGxFormat>& formats, CGxFormat::Format color, CGxFormat::Format depth, uint32_t samples) {
    CGxFormat format;
    memset(&format, 0, sizeof(format));
    format.m_colorFormat = color;
    format.m_depthFormat = depth;
    format.m_multisampleCount = samples;

    if (!CGxDevice::FormatExists(formats, format)) {
        *formats.New() = format;
    }
}

static void SetGlAttributes(const CGxFormat& format) {
    int red = 8;
    int green = 8;
    int blue = 8;
    int alpha = 0;

    if (format.m_colorFormat == CGxFormat::Fmt_Rgb565) {
        red = 5;
        green = 6;
        blue = 5;
    } else if (format.m_colorFormat == CGxFormat::Fmt_Argb8888) {
        alpha = 8;
    }

    int depth = 24;
    int stencil = 0;

    if (format.m_depthFormat == CGxFormat::Fmt_Ds160) {
        depth = 16;
    } else if (format.m_depthFormat == CGxFormat::Fmt_Ds248) {
        depth = 24;
        stencil = 8;
    } else if (format.m_depthFormat == CGxFormat::Fmt_Ds320) {
        depth = 32;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, red);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, green);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, blue);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, alpha);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depth);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencil);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, format.m_multisampleCount > 1 ? 1 : 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, format.m_multisampleCount > 1 ? format.m_multisampleCount : 0);
}

static bool VerifyFormat(const CGxFormat& format) {
    SDL_Window* currentWindow = SDL_GL_GetCurrentWindow();
    SDL_GLContext currentContext = SDL_GL_GetCurrentContext();

    SetGlAttributes(format);
    SDL_Window* window = SDL_CreateWindow("adfmt", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);

    if (!window) {
        return false;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    bool result = context != nullptr;

    if (context) {
        SDL_GL_DeleteContext(context);
    }

    SDL_DestroyWindow(window);

    if (currentWindow && currentContext) {
        SDL_GL_MakeCurrent(currentWindow, currentContext);
    }

    return result;
}

int32_t CGxDevice::AdapterFormats(EGxApi api, TSGrowableArray<CGxFormat>& formats) {
    formats.SetCount(0);
    formats.m_chunk = 64;
    formats.Reserve(256, 0);

    if (api == GxApi_OpenGl) {
        CGxDevice::OpenGlAdapterFormats(formats);
    }

    return formats.Count() != 0;
}

int32_t CGxDevice::DeviceAdapterID(uint16_t& vendorID, uint16_t& deviceID, uint32_t& driverVersionHi, uint32_t& driverVersionLo) {
    vendorID = 0xFFFF;
    deviceID = 0xFFFF;
    driverVersionHi = 0;
    driverVersionLo = 0;
    CGxDevice::Log("CGxDevice::DeviceAdapterID(): RET: %d, VID: %x, DID: %x, DVER: %x.%x", 0, vendorID, deviceID, driverVersionHi, driverVersionLo);
    return 0;
}

int32_t CGxDevice::DeviceAdapterInfer(uint16_t& deviceID) {
    int32_t result = 0;
    deviceID = 0;

    // Windows classifies the adapter from the Direct3D 9 HAL caps (hardware T&L, more than two texture stages,
    // pixel shader 2.0 / 1.1). The OpenGL equivalents are ARB_vertex_program, GL_MAX_TEXTURE_UNITS_ARB and
    // ARB_fragment_program / ATI_fragment_shader / NV_texture_shader, queried on a throwaway context.
    SDL_GL_ResetAttributes();
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    SDL_Window* window = SDL_CreateWindow("adinfer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 16, 16, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);

    if (window) {
        SDL_GLContext context = SDL_GL_CreateContext(window);

        if (context) {
            const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
            GLint textureUnits = 0;
            glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &textureUnits);

            auto hasExtension = [extensions](const char* name) {
                return extensions && strstr(extensions, name) != nullptr;
            };

            bool hwTransform = hasExtension("GL_ARB_vertex_program");
            bool ps20 = hasExtension("GL_ARB_fragment_program");
            bool ps11 = ps20 || hasExtension("GL_ATI_fragment_shader") || hasExtension("GL_NV_texture_shader");

            if (hwTransform && textureUnits > 2 && ps20) {
                deviceID = 3;
            } else if (hwTransform && textureUnits > 2 && ps11) {
                deviceID = 2;
            } else if (hwTransform && textureUnits >= 2) {
                deviceID = 1;
            } else {
                deviceID = 0;
            }

            result = 1;

            SDL_GL_MakeCurrent(window, nullptr);
            SDL_GL_DeleteContext(context);
        }

        SDL_DestroyWindow(window);
    }

    SDL_GL_ResetAttributes();
    CGxDevice::Log("CGxDevice::DeviceAdapterInfer(): RET: %d, DID: %x", result, deviceID);

    return result;
}

int32_t CGxDevice::AdapterDesktopMode(CGxMonitorMode& mode) {
    SDL_DisplayMode displayMode;

    if (SDL_GetDesktopDisplayMode(0, &displayMode)) {
        return 0;
    }

    mode.m_size.x = displayMode.w;
    mode.m_size.y = displayMode.h;
    mode.m_refreshRate = displayMode.refresh_rate;
    mode.m_bpp = SDL_BITSPERPIXEL(displayMode.format);

    return 1;
}

int32_t CGxDevice::OpenGlAdapterFormats(TSGrowableArray<CGxFormat>& formats) {
    CGxFormat::Format colorFormats[] = { CGxFormat::Fmt_Rgb565, CGxFormat::Fmt_ArgbX888, CGxFormat::Fmt_Argb8888 };
    CGxFormat::Format depthFormats[] = { CGxFormat::Fmt_Ds160, CGxFormat::Fmt_Ds24X, CGxFormat::Fmt_Ds248, CGxFormat::Fmt_Ds320 };
    uint32_t sampleCounts[] = { 1, 2, 4, 8 };

    for (uint32_t color = 0; color < sizeof(colorFormats) / sizeof(colorFormats[0]); color++) {
        for (uint32_t depth = 0; depth < sizeof(depthFormats) / sizeof(depthFormats[0]); depth++) {
            for (uint32_t samples = 0; samples < sizeof(sampleCounts) / sizeof(sampleCounts[0]); samples++) {
                CGxFormat format;
                memset(&format, 0, sizeof(format));
                format.m_colorFormat = colorFormats[color];
                format.m_depthFormat = depthFormats[depth];
                format.m_multisampleCount = sampleCounts[samples];

                if (VerifyFormat(format)) {
                    AddFormat(formats, format.m_colorFormat, format.m_depthFormat, format.m_multisampleCount);
                }
            }
        }
    }

    return formats.Count() != 0;
}

int32_t CGxDevice::AdapterMonitorModes(TSGrowableArray<CGxMonitorMode>& modes) {
    modes.SetCount(0);

    int count = SDL_GetNumDisplayModes(0);

    for (int i = 0; i < count; i++) {
        SDL_DisplayMode displayMode;

        if (SDL_GetDisplayMode(0, i, &displayMode)) {
            continue;
        }

        int bpp = SDL_BITSPERPIXEL(displayMode.format);

        if (displayMode.w >= 640 && displayMode.h >= 480 && bpp >= 16) {
            CGxMonitorMode* mode = modes.New();
            mode->m_size.x = displayMode.w;
            mode->m_size.y = displayMode.h;
            mode->m_bpp = bpp;
            mode->m_refreshRate = displayMode.refresh_rate;
        }
    }

    qsort(modes.Ptr(), modes.Count(), sizeof(CGxMonitorMode), CGxMonitorModeSort);

    return modes.Count() != 0;
}

#endif
