#include "NativeDisplayProbe.h"
#include "NativeGuiProbe.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Metal/NativeHost.h"
#include "Engine/Source/Os/Apple/Input.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Os/Input.h"
#include "WoW/Source/ConsoleDevice/ConsoleDevice.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include <TargetConditionals.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <thread>

namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
}
struct NativeDisplayProbe::Impl {
    CGxDeviceMetal& device;
    MTKView* view;
    CGxFormat original;
    BOOL automatic;
    CBoundingBox viewport;
    unsigned phase = 0;
    C2iVector sizes[6];
#if TARGET_OS_OSX
    NSRect windowedFrame;
    NSWindowStyleMask windowedStyle;
    NSApplicationPresentationOptions windowedPresentation;
    unsigned fullscreenPhase = 0;
    CFAbsoluteTime fullscreenDeadline = 0;
    CGxGammaRamp originalGamma;
    bool FullScreenStep();
#endif
    Impl(CGxDeviceMetal& d, MTKView* v) : device(d), view(v), original(d.m_format), automatic(v.autoResizeDrawable), viewport(d.m_viewport) {
        original.m_size = {int32_t(view.drawableSize.width), int32_t(view.drawableSize.height)};
        sizes[0] = {320, 180}; sizes[1] = {641, 359}; sizes[2] = original.m_size;
#if TARGET_OS_OSX
        originalGamma = d.m_gammaRamp;
#endif
    }
    ~Impl() {
        if (device.FrameCommand()) device.ScenePresent();
#if TARGET_OS_OSX
        device.DeviceSetGamma(originalGamma);
#endif
        id<MTKViewDelegate> delegate = view.delegate; view.delegate = nil;
        device.DeviceSetFormat(original); view.autoResizeDrawable = automatic;
        view.delegate = delegate;
        device.XformSetViewport(viewport.m_x.m_l, viewport.m_x.m_h, viewport.m_y.m_l, viewport.m_y.m_h, viewport.m_z.m_l, viewport.m_z.m_h);
    }
    void Request(C2iVector& size) {
        CGxFormat format = original; format.m_size = size;
        if (ConsoleDeviceExists()) {
            SetGxCVars(format);
            Require(CCGxRestart(nullptr, nullptr), "Original gxRestart failed");
        } else {
            Require(device.DeviceSetFormat(format), "Native windowed format request failed");
        }
        Accepted(size);
    }
    void Accepted(C2iVector& size) {
        const auto accepted = device.m_format.m_size;
#if TARGET_OS_OSX
        const CGFloat scale = view.window.backingScaleFactor;
        Require(std::abs(accepted.x - size.x) < scale && std::abs(accepted.y - size.y) < scale,
            "Window size adjustment exceeded native point rounding");
#else
        Require(accepted.x == size.x && accepted.y == size.y, "UIKit changed the explicit render resolution");
#endif
        size = accepted;
        if (ConsoleDeviceExists()) {
            Require(s_requestedFormat.m_size.x == size.x && s_requestedFormat.m_size.y == size.y &&
                s_lastGoodFormat.m_size.x == size.x && s_lastGoodFormat.m_size.y == size.y,
                "Original restart published a different size from the native window");
            char value[64]; std::snprintf(value, sizeof(value), "%dx%d", size.x, size.y);
            Require(!std::strcmp(CVar::Lookup("gxResolution")->m_stringValue.m_str, value), "Restart CVar retained the unadjusted size");
        }
    }
    bool Step();
};
NativeDisplayProbe::NativeDisplayProbe(CGxDeviceMetal& device, MTKView* view) : m(std::make_unique<Impl>(device, view)) {}
NativeDisplayProbe::~NativeDisplayProbe() = default;
bool NativeDisplayProbe::Step() { return m->Step(); }
bool NativeDisplayProbe::Impl::Step() {
    Require(!device.FrameCommand(), "Native format check requires an idle host");
#if TARGET_OS_OSX
    if (phase >= 7) return FullScreenStep();
#endif
    if (!GxMetal::PrepareHostDrawable((__bridge void*)view)) return false;
    device.XformSetViewport(0, 1, 0, 1, 0, 1);
    if (!phase) {
        CheckNativeGui((__bridge void*)view); Request(sizes[phase++]);
        if (ConsoleDeviceExists()) {
            s_requestedFormat.m_depthFormat = CGxFormat::Fmt_Ds24X;
            CCGxRestart(nullptr, nullptr);
            Require(device.m_format.m_depthFormat == original.m_depthFormat &&
                s_requestedFormat.m_depthFormat == original.m_depthFormat, "Restart failed to restore last good format after rejecting D24");
        }
        return false;
    }
    {
        const C2iVector size = sizes[phase - 1];
        CGxFormat requested = original; requested.m_size = size;
#if TARGET_OS_OSX
        requested.m_maximize = phase == 5;
        if (phase == 5) {
            OsInputSetWindowResizeLock(0);
            OsInputSetWindowResizeLock(1); OsInputSetWindowResizeLock(0);
            Require(NSEqualRects(view.window.frame, view.window.screen.frame) && view.window.styleMask == NSWindowStyleMaskBorderless &&
                view.window.canBecomeKeyWindow && view.window.isKeyWindow, "Borderless game window lost screen coverage or input focus");
            Require((NSApp.presentationOptions & (NSApplicationPresentationAutoHideDock | NSApplicationPresentationAutoHideMenuBar)) ==
                (NSApplicationPresentationAutoHideDock | NSApplicationPresentationAutoHideMenuBar), "Borderless window did not hide desktop chrome");
            NSWindow* other = [[NSWindow alloc] initWithContentRect:NSMakeRect(40, 40, 180, 80)
                styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];
            other.releasedWhenClosed = NO;
            [other makeKeyAndOrderFront:nil];
            Require(other.isKeyWindow, "Display check could not transfer native focus");
            Require(NSApp.presentationOptions == windowedPresentation, "Borderless focus loss did not restore desktop presentation");
            [view.window makeKeyAndOrderFront:nil]; [other close];
            Require(view.window.isKeyWindow, "Borderless window could not regain keyboard focus");
        } else if (phase == 6) {
            Require(NSEqualRects(view.window.frame, windowedFrame) && view.window.styleMask == windowedStyle &&
                NSApp.presentationOptions == windowedPresentation, "Leaving borderless mode did not restore the window and desktop");
        }
#endif
        Require(view.drawableSize.width == size.x && view.drawableSize.height == size.y,
            "DeviceSetFormat left the drawable at its old resolution");
#if TARGET_OS_OSX
        NSRect backing = [view convertRectToBacking:view.bounds];
        Require(backing.size.width == size.x && backing.size.height == size.y, "Window points did not map to requested drawable pixels");
#endif
        tagRECT bounds{}; OsGetDefaultWindowRect(&bounds);
        Require(bounds.right == size.x && bounds.bottom == size.y, "Native input retained the previous pixel bounds");
        auto accepted = device.m_format;
        auto reject = [&](const CGxFormat& invalid) {
            Require(!device.DeviceSetFormat(invalid), "Unsupported native presentation was accepted");
            Require(device.m_format.m_size.x == accepted.m_size.x && device.m_format.m_size.y == accepted.m_size.y &&
                view.drawableSize.width == size.x && view.drawableSize.height == size.y, "Rejected native format changed drawable or active size");
        };
        CGxFormat invalid = accepted; invalid.m_size.x = 16385; reject(invalid);
        invalid = accepted; invalid.m_window = false; invalid.m_size = {16384,16384}; reject(invalid);
        invalid = accepted; invalid.m_size.x = 16385;
        Require(!device.DeviceCreate((__bridge void*)view, invalid) && device.DeviceWindow() == (__bridge void*)view &&
            OsGuiGetWindow(0) == (__bridge void*)view && view.drawableSize.width == size.x,
            "Failed host creation lost the existing native binding");
        MTKView* detached = [[MTKView alloc] initWithFrame:CGRectMake(0, 0, 17, 13) device:view.device];
        Require(!device.DeviceCreate((__bridge void*)detached, invalid) && device.DeviceWindow() == (__bridge void*)view,
            "Failed replacement published the new host");
        invalid = accepted;
#if TARGET_OS_OSX
        invalid.m_maximize = 2;
#else
        invalid.m_maximize = 1;
#endif
        reject(invalid);
        invalid = accepted;
#if TARGET_OS_OSX
        invalid.m_window = 2;
#else
        invalid.m_window = false;
#endif
        reject(invalid);
        invalid = accepted; invalid.m_refreshRate = 777; reject(invalid);
        invalid = accepted; invalid.m_depthFormat = CGxFormat::Fmt_Ds24X; reject(invalid);
#if !TARGET_OS_OSX
        invalid = accepted; invalid.m_vsync = 0; reject(invalid);
#endif
        int32_t workerResult = 1;
        std::thread worker([&] { workerResult = device.DeviceSetFormat(accepted); }); worker.join();
        Require(!workerResult, "Background thread mutated the native window");
        auto pass = view.currentRenderPassDescriptor; auto drawable = view.currentDrawable;
        Require(pass && drawable && pass.colorAttachments[0].texture.width == size.x && pass.colorAttachments[0].texture.height == size.y &&
            pass.depthAttachment.texture.width == size.x && pass.depthAttachment.texture.height == size.y,
            "Native drawable/depth attachments did not rebuild at the new size");
        device.BeginFrame(pass, drawable);
        // Interactive resizing publishes its actual format with the new frame.
        accepted = device.m_format;
        invalid = accepted; invalid.m_size = {123, 77}; reject(invalid);
        device.SceneClear(3, CImVector(31, 73, 149, 255));
        CiRect rect(0, 0, size.y, size.x); TSGrowableArray<CImVector> pixels;
        device.DeviceReadPixels(rect, pixels);
        Require(pixels.Count() == uint32_t(size.x * size.y), "Resized framebuffer readback size changed");
        for (uint32_t i = 0; i < pixels.Count(); ++i)
            Require(pixels[i].b == 31 && pixels[i].g == 73 && pixels[i].r == 149 && pixels[i].a == 255, "Resized framebuffer lost clear coverage");
        CiRect corner(size.y - 1, size.x - 1, size.y, size.x); TSGrowableArray<float> depth;
        device.DeviceReadDepths(corner, depth);
        Require(depth.Count() == 1 && depth[0] == 1.0f, "Resized depth attachment did not clear its final pixel");
        const bool deferred = ConsoleDeviceExists() && phase < 3;
        if (deferred) {
            CGxFormat next = original; next.m_size = {127, 79};
            SetGxCVars(next); CCGxRestart(nullptr, nullptr);
            next.m_size = sizes[phase]; SetGxCVars(next); CCGxRestart(nullptr, nullptr);
            Require(device.m_format.m_size.x == size.x && device.m_format.m_size.y == size.y &&
                view.drawableSize.width == size.x && view.drawableSize.height == size.y,
                "Original restart changed attachments during an active frame");
        }
        device.ScenePresent(); device.Drain();
        if (deferred) Accepted(sizes[phase]);
        [view releaseDrawables];
#if TARGET_OS_OSX
        if (!deferred) {
            requested.m_vsync = 0;
            Require(device.DeviceSetFormat(requested) && !((CAMetalLayer*)view.layer).displaySyncEnabled, "Native vsync disable did not reach the layer");
            requested.m_vsync = 1;
            Require(device.DeviceSetFormat(requested) && ((CAMetalLayer*)view.layer).displaySyncEnabled, "Native vsync restore did not reach the layer");
        }
#endif
    }
    if (phase < 3) { if (!ConsoleDeviceExists()) Request(sizes[phase]); ++phase; return false; }
#if TARGET_OS_OSX
    if (phase == 3) {
        // Exercise a native window resize across the event loop, rather than
        // only checking the explicit DeviceSetFormat value immediately.
        [view.window setContentSize:NSMakeSize(367, 209)];
        const auto backing = [view convertRectToBacking:view.bounds];
        sizes[3] = {int32_t(backing.size.width), int32_t(backing.size.height)};
        ++phase; return false;
    }
    if (phase == 4 || phase == 5) {
        CGxFormat format = device.m_format;
        format.m_maximize = phase == 4;
        if (phase == 4) {
            windowedFrame = view.window.frame; windowedStyle = view.window.styleMask;
            windowedPresentation = NSApp.presentationOptions;
            OsInputSetWindowResizeLock(1);
        }
        if (ConsoleDeviceExists()) { SetGxCVars(format); Require(CCGxRestart(nullptr, nullptr), "Borderless gxRestart failed"); }
        else Require(device.DeviceSetFormat(format), "Native borderless format request failed");
        sizes[phase] = device.m_format.m_size;
        Require(device.m_format.m_maximize == format.m_maximize, "Native window did not apply the requested presentation");
        if (ConsoleDeviceExists()) {
            Require(CVar::Lookup("gxMaximize")->m_intValue == format.m_maximize && s_lastGoodFormat.m_maximize == format.m_maximize &&
                s_requestedFormat.m_maximize == format.m_maximize, "Original video options lost the applied borderless mode");
            Accepted(sizes[phase]);
        }
        ++phase; return false;
    }
    std::fputs("Native display: borderless screen coverage, full GPU readback, focus/presentation restoration and original window-size restoration passed\n", stderr);
    phase = 7; return false;
#endif
    std::fprintf(stderr, "Native display: real drawable/depth resize, odd pixel dimensions, full-surface GPU readback, input bounds, presentation rejection and active-frame/thread safety passed; original ConsoleDevice restart %s\n", ConsoleDeviceExists() ? "passed (deferred latest request and last-good rollback)" : "not active");
    return true;
}

#if TARGET_OS_OSX
bool NativeDisplayProbe::Impl::FullScreenStep() {
    auto* window = (GxMetalWindow*)view.window;
    Require([window isKindOfClass:GxMetalWindow.class], "Fullscreen check needs the native game window");
    auto request = [&](bool full, C2iVector size) {
        auto format = device.m_format; format.m_window = !full; format.m_maximize = 0; format.m_size = size;
        if (ConsoleDeviceExists()) { SetGxCVars(format); Require(CCGxRestart(nullptr,nullptr), "Fullscreen gxRestart failed"); }
        else Require(device.DeviceSetFormat(format), "Native fullscreen request failed");
        Require(bool(device.m_format.m_window) == !full, "Fullscreen request did not reach the device");
        if (ConsoleDeviceExists()) Require(bool(CVar::Lookup("gxWindow")->m_intValue) == !full &&
            bool(s_lastGoodFormat.m_window) == !full, "Original video state lost the fullscreen request");
        fullscreenDeadline = CFAbsoluteTimeGetCurrent()+15;
    };
    if (!fullscreenPhase) {
        windowedFrame = window.frame; windowedStyle = window.styleMask; windowedPresentation = NSApp.presentationOptions;
        OsInputSetWindowResizeLock(1);
        device.DeviceSetGamma(.5f); request(true,{641,359});
        ++fullscreenPhase; return false;
    }
    Require(CFAbsoluteTimeGetCurrent() < fullscreenDeadline, "Native fullscreen transition did not complete");
    if (!GxMetal::PrepareHostDrawable((__bridge void*)view)) return false;
    const bool full = (window.styleMask & NSWindowStyleMaskFullScreen) != 0;
    if (window.gameFullScreenTransition || (fullscreenPhase <= 2 && !full) || (fullscreenPhase >= 3 && full)) return false;
    auto checkImage = [&] {
        const auto size = device.m_format.m_size;
        Require(view.drawableSize.width == size.x && view.drawableSize.height == size.y, "Fullscreen render resolution drifted to native backing size");
        tagRECT bounds{}; OsGetDefaultWindowRect(&bounds);
        Require(bounds.right == size.x && bounds.bottom == size.y, "Fullscreen input pixel dimensions changed");
        auto pass = view.currentRenderPassDescriptor; auto drawable = view.currentDrawable;
        Require(pass && drawable, "Fullscreen has no drawable");
        device.BeginFrame(pass,drawable); device.XformSetViewport(0,1,0,1,0,1);
        device.SceneClear(3,CImVector(31,73,149,255));
        CiRect edge(size.y-1,size.x-1,size.y,size.x); TSGrowableArray<CImVector> pixels;
        device.DeviceReadPixels(edge,pixels);
        Require(pixels.Count()==1 && pixels[0] == CImVector(31,73,149,255), "Fullscreen game backbuffer changed before gamma");
        const auto before = device.EncodingStats().gammaPresentations;
        device.ScenePresent(); device.Drain();
        Require(device.EncodingStats().gammaPresentations == before + (fullscreenPhase == 1), "Fullscreen gamma did not follow video mode");
        auto buffer = [device.GPU().device newBufferWithLength:256 options:MTLResourceStorageModeShared];
        auto command = [device.GPU().queue commandBuffer]; auto blit = [command blitCommandEncoder];
        [blit copyFromTexture:drawable.texture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(size.x-1,size.y-1,0)
            sourceSize:MTLSizeMake(1,1,1) toBuffer:buffer destinationOffset:0 destinationBytesPerRow:256 destinationBytesPerImage:256];
        [blit endEncoding]; [command commit]; [command waitUntilCompleted];
        Require(!command.error, "Fullscreen presentation readback failed");
        const auto pixel = *static_cast<const CImVector*>(buffer.contents);
        auto channel = [&](uint8_t code) { return fullscreenPhase == 1 ? uint8_t(std::lround(double(device.m_gammaRamp.m_red[code])*255/65535)) : code; };
        Require(pixel.r == channel(149) && pixel.g == channel(73) && pixel.b == channel(31), "Native fullscreen presentation has incorrect gamma or coverage");
        [view releaseDrawables];
    };
    checkImage();
    if (fullscreenPhase == 1) {
        Require(device.m_format.m_size.x == 641 && device.m_format.m_size.y == 359 && window.isKeyWindow,
            "Fullscreen lost the selected odd resolution or keyboard focus");
        auto* cursor = GxMetal::CursorForHostView((__bridge void*)view);
        Require(std::abs(cursor.image.size.width*641/view.bounds.size.width-32) < .001 &&
            std::abs(cursor.image.size.height*359/view.bounds.size.height-32) < .001, "Fullscreen hardware cursor did not follow game pixel size");
        device.DeviceSetGamma(1); request(true,{1279,719});
        ++fullscreenPhase; return false;
    }
    if (fullscreenPhase == 2) {
        Require(device.m_format.m_size.x==1279 && device.m_format.m_size.y==719, "Fullscreen resolution update was lost");
        [window toggleFullScreen:nil]; // Native action must use the original restart/CVars.
        Require(device.m_format.m_window, "Native fullscreen action bypassed original video settings");
        fullscreenDeadline = CFAbsoluteTimeGetCurrent()+15;
        ++fullscreenPhase; return false;
    }
    Require(NSEqualRects(window.frame,windowedFrame), "Fullscreen exit did not restore the window frame");
    if (fullscreenPhase == 3) {
        request(true,{641,359}); GxMetal::PrepareHostDrawable((__bridge void*)view);
        Require(window.gameFullScreenTransition, "Fullscreen reversal check did not start a transition");
        request(false,device.m_format.m_size); // Reverse while AppKit is animating.
        ++fullscreenPhase; return false;
    }
    OsInputSetWindowResizeLock(0);
    Require(window.styleMask==windowedStyle && NSApp.presentationOptions==windowedPresentation,
        "Native fullscreen did not restore resize style or system presentation");
    device.DeviceSetGamma(originalGamma);
    std::fputs("Native fullscreen: real AppKit enter/exit and reversed transition, original CVars/native action, fixed odd render resolutions, input/cursor scaling, gamma/readback, frame/style restoration passed\n",stderr);
    return true;
}
#endif
