#include "NativeGuiProbe.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Storm/H/storm.h"
#include <TargetConditionals.h>
#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <thread>
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif

namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
#if TARGET_OS_OSX
NSButton* FindButton(NSView* view, NSString* title) {
    if ([view isKindOfClass:NSButton.class] && [((NSButton*)view).title isEqualToString:title]) return (NSButton*)view;
    for (NSView* child in view.subviews) if (auto* button = FindButton(child, title)) return button;
    return nil;
}
void CheckDialogs(void* view) {
    struct Case { int style; NSString* button; int result; };
    const Case cases[] = {{0, @"OK", 0}, {1, @"OK", 0}, {1, @"Cancel", 2}, {2, @"Yes", 0},
        {2, @"No", 1}, {3, @"Yes", 0}, {3, @"No", 1}, {3, @"Cancel", 2}, {99, @"OK", 0}};
    for (const auto& test : cases) {
        __block const char* error = nullptr;
        __block bool clicked = false;
        const auto deadline = [NSDate dateWithTimeIntervalSinceNow:5];
        NSTimer* timer = [NSTimer timerWithTimeInterval:0.05 repeats:YES block:^(NSTimer* timer) {
            NSWindow* modal = NSApp.modalWindow;
            if (!modal) return;
            if (auto* button = FindButton(modal.contentView, test.button)) {
                if (OsGuiMessageBox(view, 0, "nested", "nested") != 2 || SErrGetLastError() != EBUSY)
                    error = "Native modal prompt accepted recursive entry";
                const bool hasNo = FindButton(modal.contentView, @"No") != nil;
                const bool hasCancel = FindButton(modal.contentView, @"Cancel") != nil;
                if (hasNo != (test.style == 2 || test.style == 3) || hasCancel != (test.style == 1 || test.style == 3))
                    error = "Native prompt changed the original available decisions";
                clicked = true; [timer invalidate]; [button performClick:nil];
            } else if (deadline.timeIntervalSinceNow < 0) {
                error = "Native prompt did not expose the expected button";
                [timer invalidate]; [NSApp abortModal];
            }
        }];
        [NSRunLoop.mainRunLoop addTimer:timer forMode:NSModalPanelRunLoopMode];
        const int result = OsGuiMessageBox(view, test.style, "Literal 100%: %s %n", "WoW \xe7\xaa\x97\xe5\x8f\xa3");
        [timer invalidate];
        Require(!error, error ? error : "");
        Require(clicked && result == test.result, "Native dialog button changed the original return code");
    }
    std::fputs("Native GUI: nine real AppKit prompt decisions and nested-prompt rejection passed\n", stderr);
}
#endif
}

void CheckNativeGui(void* handle, bool dialogs) {
    Require(OsGuiGetWindow(0) == handle, "Native Gx window was not registered");
#if TARGET_OS_OSX
    NSView* view = (__bridge NSView*)handle;
    NSString* savedTitle = view.window.title;
    auto title = [&] { return view.window.title; };
#else
    UIView* view = (__bridge UIView*)handle;
    auto title = [&] { return view.window.windowScene ? view.window.windowScene.title : view.window.rootViewController.title; };
    NSString* savedTitle = title();
#endif
    Require(view.window, "Native GUI check needs a hosted view");
    struct Restore { void* handle; NSString* title; ~Restore() { OsGuiSetGxWindow(handle); OsGuiSetWindowTitle(handle, title.UTF8String); } } restore{handle, savedTitle};
    OsGuiSetWindowTitle(handle, "WoW \xe7\xaa\x97\xe5\x8f\xa3 100% %s");
    Require([title() isEqualToString:@"WoW \u7a97\u53e3 100% %s"], "Native window title lost UTF-8 or literal percent characters");
    OsGuiSetWindowTitle(handle, "\xff");
    Require(SErrGetLastError() == EILSEQ && [title() isEqualToString:@"WoW \u7a97\u53e3 100% %s"], "Invalid UTF-8 changed the window title");
    OsGuiSetWindowTitle(handle, nullptr);
    Require(!title().length, "Null title did not clear the native title");
    Require(!OsGuiGetWindow(99), "Unknown native window query returned a window");
    @autoreleasepool {
#if TARGET_OS_OSX
        NSView* temporary = [NSView new];
#else
        UIView* temporary = [UIView new];
#endif
        OsGuiSetGxWindow((__bridge void*)temporary);
        Require(OsGuiGetWindow(0) == (__bridge void*)temporary, "Native window binding did not change");
        temporary = nil;
    }
    Require(!OsGuiGetWindow(0), "Native GUI kept a released view alive");
    OsGuiSetGxWindow(handle);
    NSObject* invalid = [NSObject new];
    OsGuiSetGxWindow((__bridge void*)invalid);
    Require(SErrGetLastError() == EINVAL && OsGuiGetWindow(0) == handle, "Invalid GUI handle replaced the live view");
    bool workerOK = false;
    std::thread worker([&] {
        @autoreleasepool {
            const bool query = !OsGuiGetWindow(0) && SErrGetLastError() == EWOULDBLOCK;
            OsGuiSetGxWindow(nullptr); OsGuiSetWindowTitle(handle, "wrong thread");
            workerOK = query && SErrGetLastError() == EWOULDBLOCK && OsGuiMessageBox(handle, 0, "worker", "worker") == 2;
        }
    }); worker.join();
    Require(workerOK && OsGuiGetWindow(0) == handle && !title().length, "Worker thread mutated native GUI state");
    Require(OsGuiMessageBox(handle, 0, nullptr, "invalid") == 2, "Invalid prompt returned success");
#if TARGET_OS_OSX
    if (NSApp.keyWindow == view.window) Require(OsGuiGetWindow(1) == handle, "Active native window lost the Gx view handle");
    if (dialogs) CheckDialogs(handle);
#else
    Require(OsGuiMessageBox(handle, 2, "Unsupported synchronous decision", "GUI check") == 2 && SErrGetLastError() == ENOTSUP,
        "UIKit synchronous prompt silently accepted a decision");
#endif
    std::fputs("Native GUI: hosted view, UTF-8 title, weak lifetime, invalid inputs and worker-thread rejection passed\n", stderr);
}
