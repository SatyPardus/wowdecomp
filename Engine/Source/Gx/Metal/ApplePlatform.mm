#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Metal/NativeHost.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/Apple/Input.h"
#import <MetalKit/MetalKit.h>
#include <TargetConditionals.h>
#include <stdexcept>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <vector>
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif

#if TARGET_OS_OSX
@interface GxMetalWindow ()
@property(nonatomic) BOOL borderlessGame, ownsPresentation;
@property(nonatomic) NSRect restoredFrame;
@property(nonatomic) NSWindowStyleMask restoredStyle;
@property(nonatomic) NSApplicationPresentationOptions restoredPresentation, appliedPresentation;
@property(nonatomic) BOOL fullScreenRequested, gameFullScreenTransition, fullScreenStyleSaved, closingGame;
@property(nonatomic) BOOL windowedUpdatePending, targetMaximized, restoreWindowAfterFullScreen;
@property(nonatomic) NSWindowStyleMask fullScreenRestoredStyle;
@property(nonatomic) NSRect fullScreenRestoredFrame;
@property(nonatomic) NSSize renderSize, fullScreenRenderSize, lastCursorBounds;
@property(nonatomic) NSUInteger transitionGeneration;
@property(nonatomic, copy) void (^gameFullScreenRequest)(const CGxFormat&);
- (void)setBorderlessGamePresentation:(BOOL)enabled;
- (void)driveGameFullScreen;
@end
@implementation GxMetalWindow
- (instancetype)initWithContentRect:(NSRect)rect styleMask:(NSWindowStyleMask)style backing:(NSBackingStoreType)backing defer:(BOOL)defer {
    if ((self = [super initWithContentRect:rect styleMask:style backing:backing defer:defer])) {
        self.collectionBehavior |= NSWindowCollectionBehaviorFullScreenPrimary;
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(gameFullScreenFinished:)
            name:NSWindowDidEnterFullScreenNotification object:self];
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(gameFullScreenFinished:)
            name:NSWindowDidExitFullScreenNotification object:self];
    }
    return self;
}
- (void)dealloc { [NSNotificationCenter.defaultCenter removeObserver:self]; }
- (void)toggleFullScreen:(id)sender {
    [self requestGameFullScreen:!self.fullScreenRequested restoring:NO];
}
- (void)requestGameFullScreen:(BOOL)enabled restoring:(BOOL)restoring {
    if (!g_theGxDevicePtr || g_theGxDevicePtr->DeviceWindow() != (__bridge void*)self.contentView) return;
    auto format = g_theGxDevicePtr->m_format; format.m_window = !enabled;
    if (enabled && restoring) format.m_size = {int32_t(self.fullScreenRenderSize.width),int32_t(self.fullScreenRenderSize.height)};
    if (self.gameFullScreenRequest) self.gameFullScreenRequest(format);
    else g_theGxDevicePtr->DeviceSetFormat(format);
}
- (void)gameFullScreenFinished:(NSNotification*)notification {
    self.gameFullScreenTransition = NO; ++self.transitionGeneration;
    CGxDevice::Log("Native fullscreen %s: render %.0fx%.0f, view %.0fx%.0f points",
        (self.styleMask & NSWindowStyleMaskFullScreen) ? "entered" : "exited",
        self.renderSize.width,self.renderSize.height,self.contentView.bounds.size.width,self.contentView.bounds.size.height);
    // Reconcile on the next host frame, after AppKit has completed all of its
    // resize/focus notifications. Reversed requests retain their latest target.
}
- (void)gameFullScreenTransitionFailed {
    if (!self.gameFullScreenTransition) return;
    self.gameFullScreenTransition = NO; ++self.transitionGeneration;
    self.fullScreenRequested = (self.styleMask & NSWindowStyleMaskFullScreen) != 0;
    CGxDevice::Log("AppKit fullscreen transition failed; restoring the actual native presentation");
    [self requestGameFullScreen:self.fullScreenRequested restoring:YES];
}
- (void)driveGameFullScreen {
    if (self.closingGame || self.gameFullScreenTransition) return;
    const BOOL actual = (self.styleMask & NSWindowStyleMaskFullScreen) != 0;
    if (actual != self.fullScreenRequested) {
        if (!self.visible) return; // Startup requests wait until activation.
        if (self.fullScreenRequested && !self.fullScreenStyleSaved) {
            [self setBorderlessGamePresentation:NO];
            self.fullScreenRestoredFrame = self.frame; self.fullScreenRestoredStyle = self.styleMask;
            self.fullScreenStyleSaved = YES;
            // Native fullscreen is allowed even when the game locks manual
            // window resizing; retain the locked style for the eventual exit.
            self.styleMask |= NSWindowStyleMaskResizable;
        }
        self.gameFullScreenTransition = YES;
        const NSUInteger generation = ++self.transitionGeneration;
        [super toggleFullScreen:nil];
        __weak GxMetalWindow* weak = self;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 10*NSEC_PER_SEC), dispatch_get_main_queue(), ^{
            GxMetalWindow* window = weak;
            if (window && window.transitionGeneration == generation && window.gameFullScreenTransition)
                [window gameFullScreenTransitionFailed];
        });
        return;
    }
    if (!actual && self.fullScreenStyleSaved) {
        self.styleMask = self.fullScreenRestoredStyle;
        self.fullScreenStyleSaved = NO;
    }
    if (!actual && self.windowedUpdatePending) {
        self.windowedUpdatePending = NO;
        [self setBorderlessGamePresentation:self.targetMaximized];
        if (self.targetMaximized) [self setFrame:self.screen.frame display:NO];
        else if (self.restoreWindowAfterFullScreen) [self setFrame:self.fullScreenRestoredFrame display:NO];
        else [self setContentSize:NSMakeSize(self.renderSize.width/self.backingScaleFactor, self.renderSize.height/self.backingScaleFactor)];
    }
}
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }
- (void)restoreGamePresentation {
    if (!self.ownsPresentation) return;
    if (NSApp.presentationOptions == self.appliedPresentation) NSApp.presentationOptions = self.restoredPresentation;
    self.ownsPresentation = NO;
}
- (void)updateGamePresentation {
    if (!self.borderlessGame || !self.isKeyWindow) { [self restoreGamePresentation]; return; }
    if (self.ownsPresentation) return;
    self.restoredPresentation = NSApp.presentationOptions;
    self.appliedPresentation = (self.restoredPresentation & ~(NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar)) |
        NSApplicationPresentationAutoHideDock | NSApplicationPresentationAutoHideMenuBar;
    NSApp.presentationOptions = self.appliedPresentation;
    self.ownsPresentation = YES;
}
- (void)becomeKeyWindow { [super becomeKeyWindow]; [self updateGamePresentation]; }
- (void)resignKeyWindow { [self restoreGamePresentation]; [super resignKeyWindow]; }
- (void)close {
    self.closingGame = YES; ++self.transitionGeneration; self.gameFullScreenRequest = nil;
    [self restoreGamePresentation]; [super close];
}
- (void)setBorderlessGamePresentation:(BOOL)enabled {
    if (enabled == self.borderlessGame) return;
    if (enabled) {
        self.restoredFrame = self.frame; self.restoredStyle = self.styleMask;
        self.borderlessGame = YES; self.styleMask = NSWindowStyleMaskBorderless;
    } else {
        self.borderlessGame = NO; [self restoreGamePresentation];
        self.styleMask = self.restoredStyle;
        [self setFrame:self.restoredFrame display:NO];
    }
    [self updateGamePresentation];
}
@end
#endif

void GxMetal::RefreshHostCursor(void* nativeView) {
#if TARGET_OS_OSX
    if (!nativeView) return;
    if (!NSThread.isMainThread) throw std::runtime_error("Native cursor updates require the host thread");
    NSView* view = (__bridge NSView*)nativeView;
    if ([view conformsToProtocol:@protocol(GxMetalCursorHost)]) [(id<GxMetalCursorHost>)view updateGameCursor];
#else
    (void)nativeView;
#endif
}

#if TARGET_OS_OSX
NSCursor* GxMetal::CursorForHostView(void* nativeView) {
    NSView* view = (__bridge NSView*)nativeView;
    auto* device = g_theGxDevicePtr;
    if (!device || device->DeviceApi() != GxApi_Metal || device->DeviceWindow() != nativeView || !device->m_context)
        return NSCursor.arrowCursor;
    const CGFloat scale = view.window.backingScaleFactor;
    if (!(scale > 0)) return NSCursor.arrowCursor;
    const auto* metalView = [view isKindOfClass:MTKView.class] ? (MTKView*)view : nil;
    const CGFloat pixelsX = metalView.drawableSize.width > 0 && view.bounds.size.width > 0 ? metalView.drawableSize.width/view.bounds.size.width : scale;
    const CGFloat pixelsY = metalView.drawableSize.height > 0 && view.bounds.size.height > 0 ? metalView.drawableSize.height/view.bounds.size.height : scale;
    // Original cursor pixels are straight-alpha BGRA, 32 physical pixels.
    // Giving NSImage its backing-scaled point size preserves that footprint.
    NSBitmapImageRep* bitmap = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nullptr pixelsWide:32 pixelsHigh:32
        bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
        bitmapFormat:NSBitmapFormatAlphaNonpremultiplied bytesPerRow:128 bitsPerPixel:32];
    if (!bitmap) throw std::runtime_error("Cannot allocate native cursor image");
    for (size_t i = 0; i < 1024; ++i) {
        const uint32_t pixel = device->m_hwCursor && device->m_cursorVisible ? device->m_cursor[i] : 0;
        auto* rgba = bitmap.bitmapData + i * 4;
        rgba[0] = (pixel >> 16) & 255; rgba[1] = (pixel >> 8) & 255; rgba[2] = pixel & 255; rgba[3] = pixel >> 24;
    }
    NSImage* image = [[NSImage alloc] initWithSize:NSMakeSize(32 / pixelsX, 32 / pixelsY)];
    [image addRepresentation:bitmap];
    return [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(device->m_cursorHotspotX / pixelsX, device->m_cursorHotspotY / pixelsY)];
}
#endif

bool GxMetal::GetHostFormat(CGxFormat& format) {
    if (!NSThread.isMainThread) return false;
    MTKView* view = (__bridge MTKView*)OsGuiGetWindow(0);
    if (![view isKindOfClass:MTKView.class] || !view.window ||
        view.drawableSize.width <= 0 || view.drawableSize.height <= 0 ||
        view.drawableSize.width > 16384 || view.drawableSize.height > 16384) return false;
#if TARGET_OS_OSX
    const uint32_t refresh = 0;
#else
    const uint32_t refresh = uint32_t(view.window.screen.maximumFramesPerSecond);
#endif
    format = CGxFormat(true, {int32_t(view.drawableSize.width), int32_t(view.drawableSize.height)},
        CGxFormat::Fmt_Argb8888, CGxFormat::Fmt_Ds320, refresh, 1, true, false, false, true, 0);
#if TARGET_OS_OSX
    if ([view.window isKindOfClass:GxMetalWindow.class]) {
        auto* window = (GxMetalWindow*)view.window;
        format.m_maximize = window.targetMaximized; format.m_window = !window.fullScreenRequested;
    }
#endif
    return true;
}

bool GxMetal::PrepareHostDrawable(void* nativeView) {
#if TARGET_OS_OSX
    if (!NSThread.isMainThread) throw std::runtime_error("Native drawable sizing requires the host thread");
    MTKView* view = (__bridge MTKView*)nativeView;
    if (![view isKindOfClass:MTKView.class] || !view.window) return false;
    GxMetalWindow* window = [view.window isKindOfClass:GxMetalWindow.class] ? (GxMetalWindow*)view.window : nil;
    [window driveGameFullScreen];
    const NSRect backing = [view convertRectToBacking:view.bounds];
    if (backing.size.width <= 0 || backing.size.height <= 0) return false;
    // MTKView automatic resizing can replace a Retina drawable with the point
    // size on the next event-loop pass, even though preferredDrawableSize and
    // the layer's contentsScale report native pixels. Own this conversion at
    // the frame boundary, before either color or depth attachments are acquired.
    view.autoResizeDrawable = NO;
    // Output/UI always uses backing pixels, including native fullscreen.
    // Scene resolution is controlled independently by gxRenderScale.
    const NSSize size = backing.size;
    if (size.width > 0 && size.height > 0 && !CGSizeEqualToSize(view.drawableSize, size)) {
        [view releaseDrawables];
        view.drawableSize = size;
    }
    if (window && !NSEqualSizes(window.lastCursorBounds, view.bounds.size)) {
        window.lastCursorBounds = view.bounds.size; RefreshHostCursor(nativeView);
    }
    // MTKView may already have cached a drawable during window activation.
    // Setting the size does not invalidate that one on every macOS version.
    auto drawable = view.currentDrawable;
    if (drawable && (drawable.texture.width != size_t(view.drawableSize.width) ||
                     drawable.texture.height != size_t(view.drawableSize.height))) {
        [view releaseDrawables]; return false;
    }
#else
    (void)nativeView;
#endif
    return true;
}

void GxMetal::SetHostFullScreenRequest(void (*request)(const CGxFormat&)) {
#if TARGET_OS_OSX
    if (!NSThread.isMainThread) throw std::runtime_error("Fullscreen handler requires the host thread");
    NSView* view = (__bridge NSView*)OsGuiGetWindow(0);
    if (![view.window isKindOfClass:GxMetalWindow.class]) return;
    GxMetalWindow* window = (GxMetalWindow*)view.window;
    if (request) window.gameFullScreenRequest = ^(const CGxFormat& format) { request(format); };
    else window.gameFullScreenRequest = nil;
#else
    (void)request;
#endif
}

bool CGxDeviceMetal::ApplyHostFormat(CGxFormat& format) {
    // AppKit/UIKit mutations and drawable acquisition share the host thread.
    if (!NSThread.isMainThread) return false;
    MTKView* view = (__bridge MTKView*)DeviceWindow();
    if (![view isKindOfClass:MTKView.class] || format.m_window < 0 || format.m_window > 1 || format.m_vsync > 1) return false;
#if TARGET_OS_OSX
    // This path changes windowed rendering, not the physical display mode.
    if (format.m_refreshRate || !view.window || view.window.contentView != view || view.inFullScreenMode) return false;
    if (format.m_maximize < 0 || format.m_maximize > 1 || (format.m_maximize && ![view.window isKindOfClass:GxMetalWindow.class])) return false;
    GxMetalWindow* gameWindow = [view.window isKindOfClass:GxMetalWindow.class] ? (GxMetalWindow*)view.window : nil;
    if (!format.m_window && !gameWindow) return false;
    const bool nativeFullscreen = gameWindow && (gameWindow.fullScreenRequested || gameWindow.gameFullScreenTransition || gameWindow.fullScreenStyleSaved ||
        (gameWindow.styleMask & NSWindowStyleMaskFullScreen));
    NSScreen* screen = view.window.screen;
    const CGFloat scale = view.window.backingScaleFactor;
    if (!(scale > 0)) return false;
    if (!format.m_window || format.m_maximize) {
        if (!screen) return false;
        const NSSize size = [screen convertRectToBacking:screen.frame].size;
        // Validate the effective screen size before changing either window or
        // attachments; the caller has only validated the requested size.
        if (size.width <= 0 || size.height <= 0 || size.width > m_caps.m_texMaxSize[GxTex_2d] || size.height > m_caps.m_texMaxSize[GxTex_2d] ||
            (format.m_multisampleCount > 1 && size.width * size.height > (256 * 1024 * 1024) / 8 / format.m_multisampleCount)) return false;
        if (!format.m_window) format.m_size = {int32_t(size.width), int32_t(size.height)};
    }
    const bool restoreWindowSize = gameWindow.borderlessGame && !format.m_maximize &&
        format.m_size.x == m_format.m_size.x && format.m_size.y == m_format.m_size.y;
    const BOOL wasKey = view.window.isKeyWindow;
#else
    // UIKit owns scene size and presentation synchronization. A fixed render
    // resolution may scale into that scene, but cannot disable its vsync.
    if (!format.m_window || format.m_maximize || !format.m_vsync || (format.m_refreshRate && format.m_refreshRate != view.window.screen.maximumFramesPerSecond)) return false;
#endif
    Drain();
    // Changing drawableSize can synchronously call the host's size delegate.
    // Prevent reentrant rendering until the format and attachments agree.
    id<MTKViewDelegate> delegate = view.delegate;
    view.delegate = nil;
    [view releaseDrawables];
    view.autoResizeDrawable = NO;
#if TARGET_OS_OSX
    gameWindow.fullScreenRequested = !format.m_window;
    gameWindow.targetMaximized = format.m_maximize;
    if (format.m_window && nativeFullscreen) {
        gameWindow.windowedUpdatePending = YES;
        gameWindow.restoreWindowAfterFullScreen = !format.m_maximize && gameWindow.fullScreenStyleSaved &&
            format.m_size.x == m_format.m_size.x && format.m_size.y == m_format.m_size.y;
        if (format.m_maximize) {
            const NSSize size = [screen convertRectToBacking:screen.frame].size;
            format.m_size = {int32_t(size.width),int32_t(size.height)};
        } else if (gameWindow.restoreWindowAfterFullScreen) {
            const auto content = [NSWindow contentRectForFrameRect:gameWindow.fullScreenRestoredFrame styleMask:gameWindow.fullScreenRestoredStyle];
            format.m_size = {int32_t(content.size.width*scale),int32_t(content.size.height*scale)};
        } else {
            format.m_size = {int32_t(std::round(format.m_size.x/scale)*scale),int32_t(std::round(format.m_size.y/scale)*scale)};
        }
    } else if (format.m_window) {
        [gameWindow setBorderlessGamePresentation:format.m_maximize != 0];
        if (format.m_maximize) [view.window setFrame:screen.frame display:NO];
        else if (!restoreWindowSize) [view.window setContentSize:NSMakeSize(format.m_size.x / scale, format.m_size.y / scale)];
    }
    if (wasKey) [view.window makeKeyWindow];
    // AppKit rounds window points (e.g. 641 pixels at 2x becomes 642).
    // Publish the actual native size, as D3D's adjusted window creation does;
    // do not scale an odd-sized drawable into a differently sized window.
    if (format.m_window && !nativeFullscreen) {
        const NSRect backing = [view convertRectToBacking:view.bounds];
        format.m_size = {int32_t(backing.size.width), int32_t(backing.size.height)};
    }
    gameWindow.renderSize = NSMakeSize(format.m_size.x,format.m_size.y);
    if (!format.m_window) gameWindow.fullScreenRenderSize = gameWindow.renderSize;
    ((CAMetalLayer*)view.layer).displaySyncEnabled = format.m_vsync != 0;
    ((CAMetalLayer*)view.layer).opaque = YES;
#endif
    view.drawableSize = CGSizeMake(format.m_size.x, format.m_size.y);
    // macOS hosts apply interactive resizing at the next frame boundary.
    view.delegate = delegate;
    OsInputSetDrawableSize(format.m_size.x, format.m_size.y);
    OsQueuePut(OS_INPUT_SIZE, format.m_size.x, format.m_size.y, 0, 0);
    return true;
}

int32_t CGxDevice::DeviceAdapterID(uint16_t& vendor, uint16_t& device, uint32_t& driverHi, uint32_t& driverLo) {
    // Apple GPU does not expose a D3D PCI device/driver version tuple.
    vendor = 0; device = 0; driverHi = driverLo = 0;
    return 0;
}
int32_t CGxDevice::DeviceAdapterInfer(uint16_t& device) {
    device = 0;
    if (!MTLCreateSystemDefaultDevice()) return 0;
    // The original generic tier 3 means hardware vertex processing, >2
    // textures and SM2-class programmable fragments. All native Metal devices
    // at our deployment targets meet it. This is a DBC capability category,
    // not a PCI ID, nor a claim that every game effect is already implemented.
    device = 3;
    return 1;
}
int32_t CGxDevice::AdapterDesktopMode(CGxMonitorMode& mode) {
    if (!NSThread.isMainThread) {
        __block int32_t result;
        dispatch_sync(dispatch_get_main_queue(), ^{ result = AdapterDesktopMode(mode); });
        return result;
    }
#if TARGET_OS_OSX
    NSView* view = (__bridge NSView*)OsGuiGetWindow(0);
    NSScreen* screen = view.window.screen ?: NSScreen.mainScreen;
    if (!screen) { return 0; }
    const NSRect pixels = [screen convertRectToBacking:screen.frame];
    mode.m_size.x = int32_t(pixels.size.width); mode.m_size.y = int32_t(pixels.size.height);
    mode.m_refreshRate = 0; // Variable refresh is not a fixed desktop mode rate.
#else
    UIScreen* screen = UIScreen.mainScreen;
    mode.m_size.x = int32_t(screen.nativeBounds.size.width); mode.m_size.y = int32_t(screen.nativeBounds.size.height);
    mode.m_refreshRate = uint32_t(screen.maximumFramesPerSecond);
#endif
    mode.m_bpp = 32;
    return 1;
}
int32_t CGxDevice::AdapterMonitorModes(TSGrowableArray<CGxMonitorMode>& modes) {
    if (!NSThread.isMainThread) {
        __block int32_t result;
        dispatch_sync(dispatch_get_main_queue(), ^{ result = AdapterMonitorModes(modes); });
        return result;
    }
    modes.SetCount(0);
    CGxMonitorMode current;
    if (!AdapterDesktopMode(current)) { return 0; }
#if TARGET_OS_OSX
    // These are window content sizes, not physical display modes.
    // Fullscreen output always uses the desktop backing size.
    CGxFormat active;
    const bool hasHost = GxMetal::GetHostFormat(active);
    if (hasHost && !active.m_window) {
        modes.New(current);
        return 1;
    }
    std::vector<C2iVector> sizes;
    for (const C2iVector size : {C2iVector{800,600}, {1024,768}, {1280,720}, {1280,800},
            {1280,1024}, {1366,768}, {1440,900}, {1600,900}, {1600,1200}, {1680,1050},
            {1920,1080}, {1920,1200}, {2048,1152}, {2560,1440}, {2560,1600},
            {2880,1800}, {3200,1800}, {3840,2160}}) {
        if (size.x <= current.m_size.x && size.y <= current.m_size.y) sizes.push_back(size);
    }
    sizes.push_back(current.m_size);
    if (hasHost) sizes.push_back(active.m_size);
    std::sort(sizes.begin(), sizes.end(), [](const auto& a, const auto& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    });
    C2iVector last{0,0};
    for (const auto& size : sizes) {
        if (size.x == last.x && size.y == last.y) continue;
        CGxMonitorMode mode = current; mode.m_size = size;
        modes.New(mode); last = size;
    }
#else
    // UIKit owns its scene dimensions, including portrait orientations.
    modes.New(current);
#endif
    return 1;
}
int32_t CGxDevice::AdapterFormats(EGxApi api, TSGrowableArray<CGxFormat>& formats) {
    formats.SetCount(0);
    CGxMonitorMode mode;
    if (api != GxApi_Metal || !AdapterDesktopMode(mode)) { return 0; }
    id<MTLDevice> gpu = MTLCreateSystemDefaultDevice();
    for (uint32_t samples : {1u, 2u, 4u, 8u, 16u}) {
        if (![gpu supportsTextureSampleCount:samples] || (samples > 1 && !gpu.areProgrammableSamplePositionsSupported)) continue;
        CGxFormat format(true, mode.m_size, CGxFormat::Fmt_Argb8888, CGxFormat::Fmt_Ds320, mode.m_refreshRate, 1, true, false, false, true, 0);
        format.m_multisampleCount = samples; formats.New(format);
    }
    return 1;
}
void OsInputGetMousePosition(int32_t* x, int32_t* y) {
#if TARGET_OS_OSX
    NSView* view = g_theGxDevicePtr ? (__bridge NSView*)g_theGxDevicePtr->DeviceWindow() : nil;
    if (!view.window) { OsInputGetQueuedPointer(x, y); return; }
    NSPoint point = [view convertPoint:[view.window convertPointFromScreen:NSEvent.mouseLocation] fromView:nil];
    point.x -= view.bounds.origin.x; point.y -= view.bounds.origin.y;
    if (!view.isFlipped) { point.y = view.bounds.size.height - point.y; }
    tagRECT bounds; OsGetDefaultWindowRect(&bounds);
    const NSPoint pixels = NSMakePoint(point.x * bounds.right / view.bounds.size.width, point.y * bounds.bottom / view.bounds.size.height);
    if (x) { *x = int32_t(pixels.x); }
    if (y) { *y = int32_t(pixels.y); }
#else
    // UI layout can run before an iOS host has delivered any pointer events.
    // This supplies its initial position; it does not synthesize touch input.
    OsInputGetQueuedPointer(x, y);
#endif
}

void CGxDeviceMetal::CapsWindowSizeInScreenCoords(CRect& rect) {
    // UI pixel rounding uses the host drawable, independent of an offscreen
    // render target. Headless GPU clients have a virtual screen at the origin.
    rect = m_defWindowRect;
#if TARGET_OS_OSX
    NSView* view = (__bridge NSView*)DeviceWindow();
    if (!view.window) return;
    const NSRect screenRect = [view.window convertRectToScreen:[view convertRect:view.bounds toView:nil]];
    const CGFloat scale = view.window.backingScaleFactor;
    rect.minX = NSMinX(screenRect) * scale;
    rect.minY = (NSScreen.screens.firstObject.frame.size.height - NSMaxY(screenRect)) * scale;
#else
    UIView* view = (__bridge UIView*)DeviceWindow();
    if (!view.window) return;
    const CGRect screenRect = [view convertRect:view.bounds toCoordinateSpace:view.window.screen.coordinateSpace];
    rect.minX = screenRect.origin.x * view.contentScaleFactor;
    rect.minY = screenRect.origin.y * view.contentScaleFactor;
#endif
    rect.maxX = rect.minX + m_defWindowRect.maxX - m_defWindowRect.minX;
    rect.maxY = rect.minY + m_defWindowRect.maxY - m_defWindowRect.minY;
}

static OS_MOUSE_MODE current = OS_MOUSE_MODE_NORMAL;
OS_MOUSE_MODE OsInputGetMouseMode() { return current; }
void OsInputSetMouseMode(OS_MOUSE_MODE mode) {
    if (mode == current) { return; }
    if (mode >= OS_MOUSE_MODES) { throw std::invalid_argument("Invalid native mouse mode"); }
#if TARGET_OS_OSX
    const auto result = CGAssociateMouseAndMouseCursorPosition(mode == OS_MOUSE_MODE_NORMAL);
    if (result != kCGErrorSuccess) { throw std::runtime_error("Cannot change native relative mouse mode"); }
    if (mode == OS_MOUSE_MODE_RELATIVE) { [NSCursor hide]; }
    else { [NSCursor unhide]; }
    current = mode;
#else
    throw std::runtime_error("iOS relative pointer input is not connected");
#endif
}
void OsInputSetMousePosition(int32_t x, int32_t y) {
#if TARGET_OS_OSX
    NSView* view = g_theGxDevicePtr ? (__bridge NSView*)g_theGxDevicePtr->DeviceWindow() : nil;
    if (!view.window) { throw std::runtime_error("Mouse positioning requires a native host view"); }
    tagRECT bounds; OsGetDefaultWindowRect(&bounds);
    NSPoint point = NSMakePoint(x * view.bounds.size.width / bounds.right, y * view.bounds.size.height / bounds.bottom);
    if (!view.isFlipped) { point.y = view.bounds.size.height - point.y; }
    point.x += view.bounds.origin.x; point.y += view.bounds.origin.y;
    point = [view.window convertPointToScreen:[view convertPoint:point toView:nil]];
    const CGFloat top = NSScreen.screens.firstObject.frame.size.height;
    if (CGWarpMouseCursorPosition(CGPointMake(point.x, top - point.y)) != kCGErrorSuccess) {
        throw std::runtime_error("Cannot position the native mouse cursor");
    }
#else
    throw std::runtime_error("iOS pointer positioning is not connected");
#endif
}

namespace {
std::atomic<bool> resizeLocked{false};
#if TARGET_OS_OSX
__weak NSWindow* lockedWindow;
bool restoreResizable, applyingResizeLock;
NSWindowStyleMask WindowedStyle(NSWindow* window) {
    if ([window isKindOfClass:GxMetalWindow.class] && ((GxMetalWindow*)window).fullScreenStyleSaved)
        return ((GxMetalWindow*)window).fullScreenRestoredStyle;
    if ([window isKindOfClass:GxMetalWindow.class] && ((GxMetalWindow*)window).borderlessGame)
        return ((GxMetalWindow*)window).restoredStyle;
    return window.styleMask;
}
void SetWindowedStyle(NSWindow* window, NSWindowStyleMask style) {
    if ([window isKindOfClass:GxMetalWindow.class] && ((GxMetalWindow*)window).fullScreenStyleSaved)
        ((GxMetalWindow*)window).fullScreenRestoredStyle = style;
    else if ([window isKindOfClass:GxMetalWindow.class] && ((GxMetalWindow*)window).borderlessGame)
        ((GxMetalWindow*)window).restoredStyle = style;
    else window.styleMask = style;
}
#endif
}
void OsInputApplyWindowResizeLock(void* nativeView) {
#if TARGET_OS_OSX
    auto apply = ^{
        if (applyingResizeLock) return; // AppKit style changes may reattach the view.
        applyingResizeLock = true;
        NSWindow* window = nativeView ? ((__bridge NSView*)nativeView).window : nil;
        if (lockedWindow && (lockedWindow != window || !resizeLocked.load())) {
            if (restoreResizable) SetWindowedStyle(lockedWindow, WindowedStyle(lockedWindow) | NSWindowStyleMaskResizable);
            lockedWindow = nil;
        }
        if (window && resizeLocked.load() && lockedWindow != window) {
            restoreResizable = (WindowedStyle(window) & NSWindowStyleMaskResizable) != 0;
            lockedWindow = window;
            SetWindowedStyle(window, WindowedStyle(window) & ~NSWindowStyleMaskResizable);
        }
        applyingResizeLock = false;
    };
    if (NSThread.isMainThread) apply();
    else dispatch_sync(dispatch_get_main_queue(), apply);
#else
    // UIKit owns scene/window sizing. This setting only restricts interactive
    // window resizing on macOS; it must not block iOS orientation/scene changes.
    (void)nativeView;
#endif
}
void OsInputSetWindowResizeLock(int32_t lock) {
    resizeLocked.store(lock != 0);
    OsInputApplyWindowResizeLock(g_theGxDevicePtr ? g_theGxDevicePtr->DeviceWindow() : nullptr);
}
