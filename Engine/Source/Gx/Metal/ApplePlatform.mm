#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Os/Input.h"
#include <TargetConditionals.h>
#include <stdexcept>
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif

int32_t CGxDevice::DeviceAdapterID(uint16_t& vendor, uint16_t& device, uint32_t& driverHi, uint32_t& driverLo) {
    // Apple GPU does not expose a D3D PCI device/driver version tuple.
    vendor = 0; device = 0; driverHi = driverLo = 0;
    return 0;
}
int32_t CGxDevice::DeviceAdapterInfer(uint16_t& device) { device = 0; return 0; }
int32_t CGxDevice::AdapterDesktopMode(CGxMonitorMode& mode) {
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
    if (!view.window) { throw std::runtime_error("Gx mouse position requires a native host view"); }
    NSPoint point = [view convertPoint:[view.window convertPointFromScreen:NSEvent.mouseLocation] fromView:nil];
    if (!view.isFlipped) { point.y = view.bounds.size.height - point.y; }
    const NSPoint pixels = [view convertPointToBacking:point];
    if (x) { *x = int32_t(pixels.x); }
    if (y) { *y = int32_t(pixels.y); }
#else
    throw std::runtime_error("iOS pointer input is not connected to Gx");
#endif
}
