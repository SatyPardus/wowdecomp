#if defined(__APPLE__)

#include <algorithm>
#include <cstring>
#include <SDL.h>
#include "Engine/Source/Gx/CGxDeviceOpenGL/CGxDeviceOpenGl.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Storm/H/storm.h"

static int32_t s_deviceChanging;

static void GlFormatBits(const CGxFormat& format, int& red, int& green, int& blue, int& alpha, int& depth, int& stencil) {
    red = 8;
    green = 8;
    blue = 8;
    alpha = 0;

    switch (format.m_colorFormat) {
        case CGxFormat::Fmt_Rgb565:
            red = 5;
            green = 6;
            blue = 5;
            alpha = 0;
            break;

        case CGxFormat::Fmt_Argb8888:
            alpha = 8;
            break;

        default:
            break;
    }

    depth = 24;
    stencil = 0;

    switch (format.m_depthFormat) {
        case CGxFormat::Fmt_Ds160:
            depth = 16;
            stencil = 0;
            break;

        case CGxFormat::Fmt_Ds248:
            depth = 24;
            stencil = 8;
            break;

        case CGxFormat::Fmt_Ds320:
            depth = 32;
            stencil = 0;
            break;

        default:
            break;
    }
}

static void SetGlAttributes(const CGxFormat& format) {
    int red;
    int green;
    int blue;
    int alpha;
    int depth;
    int stencil;
    GlFormatBits(format, red, green, blue, alpha, depth, stencil);

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

static int SdlGxEventWatch(void* userdata, SDL_Event* event) {
    CGxDeviceOpenGl* device = static_cast<CGxDeviceOpenGl*>(userdata);

    if (!device || !event || event->type != SDL_WINDOWEVENT || event->window.windowID != device->m_sdlWindowID) {
        return 0;
    }

    switch (event->window.event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        case SDL_WINDOWEVENT_RESIZED:
            device->IDevCheckResize();
            break;

        case SDL_WINDOWEVENT_MINIMIZED: {
            CRect rect;
            rect.minY = 0.0f;
            rect.minX = 0.0f;
            rect.maxY = 0.0f;
            rect.maxX = 0.0f;
            device->DeviceWM(GxWM_Size, reinterpret_cast<uintptr_t>(&rect), 1);
            break;
        }

        case SDL_WINDOWEVENT_SHOWN:
        case SDL_WINDOWEVENT_RESTORED:
            device->IDevCheckResize();
            break;

        case SDL_WINDOWEVENT_FOCUS_GAINED:
            device->DeviceWM(GxWM_SetFocus, 0, 0);
            break;

        case SDL_WINDOWEVENT_FOCUS_LOST:
            device->DeviceWM(GxWM_KillFocus, 0, 0);
            break;

        case SDL_WINDOWEVENT_CLOSE:
            device->DeviceWM(GxWM_Destroy, 0, 0);
            break;

        default:
            break;
    }

    return 0;
}

static SDL_Window* CreateGameWindow(CGxFormat& format, void* param) {
    SetGlAttributes(format);

    int width = format.m_size.x > 0 ? format.m_size.x : 800;
    int height = format.m_size.y > 0 ? format.m_size.y : 600;
    uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    int x = format.m_pos.x ? format.m_pos.x : SDL_WINDOWPOS_CENTERED;
    int y = format.m_pos.y ? format.m_pos.y : SDL_WINDOWPOS_CENTERED;

    SDL_Window* window = SDL_CreateWindow("World of Warcraft", x, y, width, height, flags);
    if (!window) {
        CGxDevice::Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return nullptr;
    }

    SDL_SetWindowMinimumSize(window, 320, 240);
    SDL_SetWindowData(window, "CGxDeviceOpenGl", param);

    if (format.m_window && format.m_maximize == 1) {
        SDL_MaximizeWindow(window);
        SDL_GetWindowSize(window, &format.m_size.x, &format.m_size.y);
    }

    if (!format.m_window) {
        SDL_DisplayMode mode;
        SDL_zero(mode);
        mode.w = width;
        mode.h = height;
        mode.refresh_rate = format.m_refreshRate;
        SDL_SetWindowDisplayMode(window, &mode);
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    }

    OsGuiSetGxWindow(window);

    return window;
}

void CGxDeviceOpenGl::DeviceDestroyPbuffer() {
    this->m_pbuffer = nullptr;
    this->m_pbufferDC = nullptr;
    this->m_pbufferRC = nullptr;
}

void CGxDeviceOpenGl::DeviceCreatePbuffer() {
    this->DeviceDestroyPbuffer();

    if (!glExtFramebufferObject) {
        this->m_renderTargetMode = 0;
        return;
    }

    this->m_renderTargetMode = 1;
    this->m_caps.m_rttFormat[GxTex_Argb8888] = 1;
    this->m_caps.m_rttFormat[GxTex_Rgb565] = 1;
    this->m_caps.m_rttFormat[GxTex_D24X8] = 1;
}

void CGxDeviceOpenGl::DeviceQueryPbuffer() {
}

void CGxDeviceOpenGl::IDevSetFocus(int32_t focus, const CGxFormat& format) {
    this->m_windowFocus = focus;

    if (!this->m_hwnd) {
        return;
    }

    if (!format.m_window) {
        if (focus) {
            SDL_SetWindowFullscreen(this->m_hwnd, SDL_WINDOW_FULLSCREEN);
            SDL_RaiseWindow(this->m_hwnd);
        } else {
            SDL_SetWindowFullscreen(this->m_hwnd, 0);
            SDL_MinimizeWindow(this->m_hwnd);
        }
    } else if (focus) {
        SDL_ShowWindow(this->m_hwnd);
        SDL_RaiseWindow(this->m_hwnd);
    }
}

int32_t CGxDeviceOpenGl::SetFormatMode(CGxFormat& format) {
    if (format.m_window) {
        format.m_apiSpecificModeID = 0;
        return 1;
    }

    int wantedBpp = format.m_colorFormat == CGxFormat::Fmt_Rgb565 ? 16 : 32;
    int count = SDL_GetNumDisplayModes(0);

    for (int i = 0; i < count; i++) {
        SDL_DisplayMode mode;

        if (SDL_GetDisplayMode(0, i, &mode)) {
            continue;
        }

        int bpp = SDL_BITSPERPIXEL(mode.format);

        if (mode.w == format.m_size.x && mode.h == format.m_size.y && bpp == wantedBpp && (!format.m_refreshRate || !mode.refresh_rate || static_cast<uint32_t>(mode.refresh_rate) == format.m_refreshRate)) {
            format.m_apiSpecificModeID = i;
            format.m_refreshRate = mode.refresh_rate ? mode.refresh_rate : format.m_refreshRate;
            return 1;
        }
    }

    return 0;
}

void CGxDeviceOpenGl::IDevRemoveGlContext() {
    this->DeviceDestroyPbuffer();

    if (this->m_hglrc) {
        this->ICursorDestroy();
        this->DeviceDestroyPoolAPI();
        this->ITexForceRecreation();
        this->IShaderForceRecreation();
        this->IQueryRelease();
        UnbindGlExtensions();
        SDL_GL_MakeCurrent(this->m_hwnd, nullptr);
        SDL_GL_DeleteContext(this->m_hglrc);
        this->m_hglrc = nullptr;
    }

    if (this->m_hwnd) {
        SDL_SetWindowGammaRamp(this->m_hwnd, this->m_systemGammaRamp.m_red, this->m_systemGammaRamp.m_green, this->m_systemGammaRamp.m_blue);
    }

    this->m_hdc = nullptr;
    this->m_context = 0;
}

void CGxDeviceOpenGl::DeviceDestroy() {
    CGxDevice::DeviceDestroy();
    s_deviceChanging = 1;

    if (this->m_eventWatchAdded) {
        SDL_DelEventWatch(SdlGxEventWatch, this);
        this->m_eventWatchAdded = 0;
    }

    if (this->m_hwnd) {
        this->IDevRemoveGlContext();

        if (this->m_ownhwnd) {
            SDL_DestroyWindow(this->m_hwnd);
        }
    }

    this->m_hwnd = nullptr;
    this->m_sdlWindowID = 0;
    this->m_lastDrawableWidth = 0;
    this->m_lastDrawableHeight = 0;
    OsGuiSetGxWindow(nullptr);
    s_deviceChanging = 0;
}

void CGxDeviceOpenGl::DeviceSetGamma(float gamma) {
    CGxDevice::DeviceSetGamma(gamma);

    if (this->m_hwnd && !this->m_format.m_window) {
        SDL_SetWindowGammaRamp(this->m_hwnd, this->m_gammaRamp.m_red, this->m_gammaRamp.m_green, this->m_gammaRamp.m_blue);
    }
}

void CGxDeviceOpenGl::DeviceSetGamma(const CGxGammaRamp& ramp) {
    CGxDevice::DeviceSetGamma(ramp);

    if (this->m_hwnd && !this->m_format.m_window) {
        SDL_SetWindowGammaRamp(this->m_hwnd, this->m_gammaRamp.m_red, this->m_gammaRamp.m_green, this->m_gammaRamp.m_blue);
    }
}

void* CGxDeviceOpenGl::DeviceWindow() {
    return this->m_hwnd;
}

int32_t CGxDeviceOpenGl::IDevAttachGlContext(const CGxFormat& format) {
    if (!this->m_hwnd) {
        return 0;
    }

    SetGlAttributes(format);
    this->m_hglrc = SDL_GL_CreateContext(this->m_hwnd);

    if (!this->m_hglrc) {
        CGxDevice::Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
        this->IDevRemoveGlContext();
        return 0;
    }

    if (SDL_GL_MakeCurrent(this->m_hwnd, this->m_hglrc)) {
        CGxDevice::Log("SDL_GL_MakeCurrent failed: %s", SDL_GetError());
        this->IDevRemoveGlContext();
        return 0;
    }

    this->m_context = 1;
    BindGlExtensions();
    BindWglExtensions();
    SDL_GL_SetSwapInterval(format.m_vsync != 0);
    this->ISetCaps(format);
    this->DeviceCreatePbuffer();
    this->ICursorCreate(format);
    this->IStateSetContextDefaults();
    this->IRsSync(1);
    this->IShaderConstantsClear();
    this->m_glVendor = glGetString(GL_VENDOR);
    this->m_glRenderer = glGetString(GL_RENDERER);
    this->m_glVersion = glGetString(GL_VERSION);
    this->IDevCheckResize();

    return 1;
}

int32_t CGxDeviceOpenGl::DeviceWM(EGxWM wm, uintptr_t param1, uintptr_t param2) {
    switch (wm) {
        case GxWM_Size:
            if (param2 == 1 || param2 == 2) {
                this->m_windowVisible = 0;
            } else {
                this->m_windowVisible = 1;
                this->DeviceSetDefWindow(*reinterpret_cast<const CRect*>(param1));
                this->m_viewportDirty = 1;
            }
            break;

        case GxWM_DisplayChange:
            if (this->m_windowVisible) {
                this->DeviceSetDefWindow(*reinterpret_cast<const CRect*>(param1));
                this->m_viewportDirty = 1;
            }
            break;

        case GxWM_Destroy:
            this->IDevRemoveGlContext();
            this->m_windowFocus = 0;
            break;

        case GxWM_SetFocus:
            if (!s_deviceChanging) {
                this->IDevSetFocus(1, this->m_format);
            }
            break;

        case GxWM_KillFocus:
            if (!s_deviceChanging) {
                this->m_windowFocus = 0;
            }
            break;

        default:
            break;
    }

    return 0;
}

int32_t CGxDeviceOpenGl::DeviceCreate(void* window, const CGxFormat& format) {
    s_deviceChanging = 1;
    this->m_ownhwnd = 0;
    this->m_hwnd = static_cast<SDL_Window*>(window);

    if (this->m_hwnd) {
        this->m_sdlWindowID = SDL_GetWindowID(this->m_hwnd);
        SDL_SetWindowData(this->m_hwnd, "CGxDeviceOpenGl", this);
        OsGuiSetGxWindow(this->m_hwnd);

        if (!this->m_eventWatchAdded) {
            SDL_AddEventWatch(SdlGxEventWatch, this);
            this->m_eventWatchAdded = 1;
        }

        if (!SDL_GetWindowGammaRamp(this->m_hwnd, this->m_systemGammaRamp.m_red, this->m_systemGammaRamp.m_green, this->m_systemGammaRamp.m_blue)) {
            this->m_gammaRamp = this->m_systemGammaRamp;
        }
    }

    if (this->IDevAttachGlContext(format)) {
        s_deviceChanging = 0;

        if (CGxDevice::DeviceCreate(window, format)) {
            return 1;
        }
    }

    this->DeviceDestroy();
    s_deviceChanging = 0;
    return 0;
}

int32_t CGxDeviceOpenGl::DeviceSetFormat(const CGxFormat& format) {
    CGxDevice::Log("CGxDeviceOpenGl::DeviceSetFormat():");
    this->Log(format);

    s_deviceChanging = 1;
    this->IDevRemoveGlContext();

    if (this->m_hwnd && this->m_ownhwnd) {
        SDL_DestroyWindow(this->m_hwnd);
    }

    this->m_hwnd = nullptr;
    this->m_sdlWindowID = 0;
    this->m_lastDrawableWidth = 0;
    this->m_lastDrawableHeight = 0;

    CGxFormat newFormat = format;

    if (this->SetFormatMode(newFormat) && (this->m_hwnd = CreateGameWindow(newFormat, this)) != nullptr) {
        this->m_sdlWindowID = SDL_GetWindowID(this->m_hwnd);

        if (!this->m_eventWatchAdded) {
            SDL_AddEventWatch(SdlGxEventWatch, this);
            this->m_eventWatchAdded = 1;
        }

        if (!SDL_GetWindowGammaRamp(this->m_hwnd, this->m_systemGammaRamp.m_red, this->m_systemGammaRamp.m_green, this->m_systemGammaRamp.m_blue)) {
            this->m_gammaRamp = this->m_systemGammaRamp;
        }

        this->IDevSetFocus(1, newFormat);

        if (this->IDevAttachGlContext(newFormat)) {
            s_deviceChanging = 0;

            if (CGxDevice::DeviceSetFormat(newFormat)) {
                this->DeviceSetGamma(this->m_gammaRamp);
                return 1;
            }
        }
    }

    s_deviceChanging = 0;
    return 0;
}

void CGxDeviceOpenGl::IDeviceSetRenderTargetCopy(EGxBuffer buffer, CGxTex* texture, uint32_t plane) {
    if (glExtFramebufferObject) {
        this->IDeviceSetRenderTargetFBO(buffer, texture, plane);
    } else {
        CGxDevice::DeviceSetRenderTarget(buffer, texture, plane);
    }
}

void CGxDeviceOpenGl::CapsWindowSizeInScreenCoords(CRect& rect) {
    if (this->IDevIsWindowed() && this->m_hwnd) {
        int x;
        int y;
        int w;
        int h;
        SDL_GetWindowPosition(this->m_hwnd, &x, &y);
        SDL_GetWindowSize(this->m_hwnd, &w, &h);
        rect.minY = static_cast<float>(y);
        rect.minX = static_cast<float>(x);
        rect.maxY = static_cast<float>(y + h);
        rect.maxX = static_cast<float>(x + w);
    } else {
        const CRect& window = this->DeviceCurWindow();
        rect.minY = window.minY;
        rect.minX = window.minX;
        rect.maxY = window.maxY;
        rect.maxX = window.maxX;
    }
}

int32_t CGxDeviceOpenGl::DeviceCreate(GxWindowProc windowProc, const CGxFormat& format) {
    this->m_ownhwnd = 1;

    if (CGxDevice::DeviceCreate(windowProc, format)) {
        return 1;
    }

    this->DeviceDestroy();
    return 0;
}

void CGxDeviceOpenGl::IDevCheckResize() {
    if (!this->m_hwnd) {
        return;
    }

    int width = 0;
    int height = 0;
    SDL_GL_GetDrawableSize(this->m_hwnd, &width, &height);

    if (width <= 0 || height <= 0 || (width == this->m_lastDrawableWidth && height == this->m_lastDrawableHeight)) {
        return;
    }

    this->m_lastDrawableWidth = width;
    this->m_lastDrawableHeight = height;

    CRect rect;
    rect.minY = 0.0f;
    rect.minX = 0.0f;
    rect.maxY = static_cast<float>(height);
    rect.maxX = static_cast<float>(width);
    this->DeviceWM(GxWM_Size, reinterpret_cast<uintptr_t>(&rect), 0);
}

#endif
