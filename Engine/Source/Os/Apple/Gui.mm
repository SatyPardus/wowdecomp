#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Storm/H/storm.h"
#include <cerrno>
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif

namespace {
// Gx owns its view. Queries must not keep a destroyed client window alive.
__weak id gameView;
bool OnMainThread() {
    if (NSThread.isMainThread) return true;
    SErrSetLastError(EWOULDBLOCK);
    return false;
}
#if TARGET_OS_OSX
NSWindow* Window(id handle) {
    if ([handle isKindOfClass:NSWindow.class]) return handle;
    if ([handle isKindOfClass:NSView.class]) return [handle window];
    return nil;
}
id ClientViewHandle(NSWindow* window) {
    if (!window) return nil;
    id view = gameView;
    return view && Window(view) == window ? view : window.contentView;
}
#else
UIWindow* Window(id handle) {
    if ([handle isKindOfClass:UIWindow.class]) return handle;
    if ([handle isKindOfClass:UIView.class]) return [handle window];
    return nil;
}
UIWindow* ActiveWindow() {
    UIWindow* selected = Window(gameView);
    if (selected.isKeyWindow && (!selected.windowScene || selected.windowScene.activationState == UISceneActivationStateForegroundActive)) return selected;
    for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
        if (![scene isKindOfClass:UIWindowScene.class] || scene.activationState != UISceneActivationStateForegroundActive) continue;
        for (UIWindow* window in ((UIWindowScene*)scene).windows) if (window.isKeyWindow) return window;
    }
    return nil;
}
id ClientViewHandle(UIWindow* window) {
    if (!window) return nil;
    id view = gameView;
    return view && Window(view) == window ? view : window.rootViewController.view;
}
#endif
}

void OsGuiSetGxWindow(void* window) {
    if (!OnMainThread()) return;
    id handle = (__bridge id)window;
#if TARGET_OS_OSX
    if (handle && ![handle isKindOfClass:NSView.class] && ![handle isKindOfClass:NSWindow.class]) {
#else
    if (handle && ![handle isKindOfClass:UIView.class]) {
#endif
        SErrSetLastError(EINVAL); return;
    }
    gameView = handle;
}

void* OsGuiGetWindow(int32_t type) {
    if (!OnMainThread()) return nullptr;
    id result = nil;
    if (type == 0) result = gameView;
#if TARGET_OS_OSX
    else if (type == 1) result = ClientViewHandle(NSApp.keyWindow);
    // Another application's foreground window is not a client-owned handle.
    else if (type == 2 && NSApp.isActive) result = ClientViewHandle(NSApp.keyWindow ?: NSApp.mainWindow);
#else
    else if (type == 1 || (type == 2 && UIApplication.sharedApplication.applicationState == UIApplicationStateActive)) result = ClientViewHandle(ActiveWindow());
#endif
    return (__bridge void*)result;
}

void OsGuiSetWindowTitle(void* window, const char* title) {
    if (!OnMainThread()) return;
    auto native = Window((__bridge id)window);
    if (!native) { SErrSetLastError(EINVAL); return; }
    NSString* text = [NSString stringWithUTF8String:title ? title : ""];
    if (!text) { SErrSetLastError(EILSEQ); return; }
#if TARGET_OS_OSX
    native.title = text;
#else
    if (native.windowScene) native.windowScene.title = text;
    else native.rootViewController.title = text;
#endif
}

int32_t OsGuiMessageBox(void* parentWindow, int32_t style, const char* message, const char* title) {
    if (!OnMainThread()) return 2;
    NSString* body = message ? [NSString stringWithUTF8String:message] : nil;
    NSString* caption = [NSString stringWithUTF8String:title ? title : ""];
    if (!body || !caption || (parentWindow && !Window((__bridge id)parentWindow))) {
        SErrSetLastError(EINVAL); return 2;
    }
#if TARGET_OS_OSX
    // AppKit runs its own modal event loop. Do not recursively enter another
    // game prompt from a callback while the first question is unanswered.
    static bool presenting = false;
    if (presenting) { SErrSetLastError(EBUSY); return 2; }
    struct Scope { bool& flag; Scope(bool& f) : flag(f) { flag = true; } ~Scope() { flag = false; } } scope(presenting);
    NSAlert* alert = [NSAlert new];
    alert.messageText = caption;
    alert.informativeText = body;
    alert.alertStyle = NSAlertStyleInformational;
    [alert addButtonWithTitle:(style == 2 || style == 3) ? @"Yes" : @"OK"];
    if (style == 2 || style == 3) [alert addButtonWithTitle:@"No"];
    if (style == 1 || style == 3) [alert addButtonWithTitle:@"Cancel"].keyEquivalent = @"\033";
    const auto result = [alert runModal];
    if (result == NSAlertFirstButtonReturn) return 0;
    if (result == NSAlertSecondButtonReturn && (style == 2 || style == 3)) return 1;
    return 2;
#else
    // The original API is synchronous. Its callers need an asynchronous
    // startup continuation on UIKit; blocking or silently accepting a prompt
    // would corrupt that contract. Preserve the original cancel/failure code.
    NSLog(@"Client synchronous prompt requires UIKit continuation: %@: %@", caption, body);
    SErrSetLastError(ENOTSUP);
    return 2;
#endif
}
