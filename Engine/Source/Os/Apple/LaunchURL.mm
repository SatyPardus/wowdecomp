#if defined(__APPLE__)
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Storm/H/storm.h"
#include <cerrno>
#include <cstring>
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif

int32_t OsLaunchURL(const char* text) {
    if (!NSThread.isMainThread) { SErrSetLastError(EWOULDBLOCK); return 0; }
    if (!text || !*text || !OsGuiGetWindow(2)) return 0;
    if (strnlen(text, 1024) == 1024) { SErrSetLastError(STORM_ERROR_INVALID_PARAMETER); return 0; }
    NSString* value = [NSString stringWithUTF8String:text];
    // Pass an encoded URL to the native URL service; decoding its separators
    // before parsing would turn escaped query/path data into URL syntax.
    NSURL* url = value ? [NSURL URLWithString:value] : nil;
    if (!url.scheme.length) { SErrSetLastError(STORM_ERROR_INVALID_PARAMETER); return 0; }
#if TARGET_OS_OSX
    const bool opened = [NSWorkspace.sharedWorkspace openURL:url];
    if (!opened) SErrSetLastError(ENOENT);
    return opened;
#else
    UIApplication* app = UIApplication.sharedApplication;
    if (![app canOpenURL:url]) { SErrSetLastError(ENOENT); return 0; }
    // The legacy result means the OS accepted a launch request, not that a
    // remote page finished loading. UIKit delivers launch failure separately.
    [app openURL:url options:@{} completionHandler:^(BOOL opened) {
        if (!opened) NSLog(@"Native URL launch was rejected by the system");
    }];
    return 1;
#endif
}
#endif
