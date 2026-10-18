#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/Apple/Input.h"
#import <MetalKit/MetalKit.h>
#include <TargetConditionals.h>
#include <stdexcept>
#include <atomic>
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif

bool CGxDeviceMetal::ApplyHostFormat(CGxFormat& format) {
    // AppKit/UIKit mutations and drawable acquisition share the host thread.
    if (!NSThread.isMainThread) return false;
    MTKView* view = (__bridge MTKView*)DeviceWindow();
    if (![view isKindOfClass:MTKView.class] || !format.m_window || format.m_maximize || format.m_vsync > 1) return false;
#if TARGET_OS_OSX
    // This path changes windowed rendering, not the physical display mode.
    if (format.m_refreshRate || !view.window || view.window.contentView != view || view.inFullScreenMode ||
        (view.window.styleMask & NSWindowStyleMaskFullScreen)) return false;
    const CGFloat scale = view.window.backingScaleFactor;
    if (!(scale > 0)) return false;
#else
    // UIKit owns scene size and presentation synchronization. A fixed render
    // resolution may scale into that scene, but cannot disable its vsync.
    if (!format.m_vsync || (format.m_refreshRate && format.m_refreshRate != view.window.screen.maximumFramesPerSecond)) return false;
#endif
    Drain();
    // Changing drawableSize can synchronously call the host's size delegate.
    // Prevent reentrant rendering until the format and attachments agree.
    id<MTKViewDelegate> delegate = view.delegate;
    view.delegate = nil;
    [view releaseDrawables];
    view.autoResizeDrawable = NO;
#if TARGET_OS_OSX
    [view.window setContentSize:NSMakeSize(format.m_size.x / scale, format.m_size.y / scale)];
    // AppKit rounds window points (e.g. 641 pixels at 2x becomes 642).
    // Publish the actual native size, as D3D's adjusted window creation does;
    // do not scale an odd-sized drawable into a differently sized window.
    const NSRect backing = [view convertRectToBacking:view.bounds];
    format.m_size = {int32_t(backing.size.width), int32_t(backing.size.height)};
    ((CAMetalLayer*)view.layer).displaySyncEnabled = format.m_vsync != 0;
#endif
    view.drawableSize = CGSizeMake(format.m_size.x, format.m_size.y);
#if TARGET_OS_OSX
    // Future interactive window resizing follows the screen's backing scale.
    view.autoResizeDrawable = YES;
#endif
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
    NSScreen* screen = NSScreen.mainScreen;
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
    modes.SetCount(0);
    CGxMonitorMode current;
    if (!AdapterDesktopMode(current)) { return 0; }
    // Native hosts use resizable windows / scene dimensions. Display mode
    // switching is not implemented, so do not advertise selectable modes.
    modes.New(current);
    return 1;
}
int32_t CGxDevice::AdapterFormats(EGxApi api, TSGrowableArray<CGxFormat>& formats) {
    formats.SetCount(0);
    CGxMonitorMode mode;
    if (api != GxApi_Metal || !AdapterDesktopMode(mode)) { return 0; }
    formats.New(CGxFormat(true, mode.m_size, CGxFormat::Fmt_Argb8888, CGxFormat::Fmt_Ds320, mode.m_refreshRate, 1, true, false, false, true, 0));
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
#endif
}
void OsInputApplyWindowResizeLock(void* nativeView) {
#if TARGET_OS_OSX
    auto apply = ^{
        if (applyingResizeLock) return; // AppKit style changes may reattach the view.
        applyingResizeLock = true;
        NSWindow* window = nativeView ? ((__bridge NSView*)nativeView).window : nil;
        if (lockedWindow && (lockedWindow != window || !resizeLocked.load())) {
            if (restoreResizable) lockedWindow.styleMask |= NSWindowStyleMaskResizable;
            lockedWindow = nil;
        }
        if (window && resizeLocked.load() && lockedWindow != window) {
            restoreResizable = (window.styleMask & NSWindowStyleMaskResizable) != 0;
            lockedWindow = window;
            window.styleMask &= ~NSWindowStyleMaskResizable;
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
