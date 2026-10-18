#include "NativeView.h"
#include "Engine/Source/Os/Apple/Input.h"
#include "Engine/Source/Os/OsIME.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Event/EvtInput.h"
#include "Engine/Source/Base/Prop.h"
#import <Carbon/Carbon.h>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <chrono>
#include <thread>

namespace {
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::vector<EVENT_DATA_KEY> downs, ups;
std::vector<int32_t> characters;
std::vector<EVENT_DATA_MOUSE> mouse;
struct Composition { int32_t action, flags; std::string text; uint32_t start, end; };
std::vector<Composition> compositions;
int32_t Initialize(const void*, void*) {
    EventRegister(EVENT_ID_KEYDOWN, [](const void* v, void*) -> int32_t { downs.push_back(*static_cast<const EVENT_DATA_KEY*>(v)); return 1; });
    EventRegister(EVENT_ID_KEYDOWN_REPEATING, [](const void* v, void*) -> int32_t { downs.push_back(*static_cast<const EVENT_DATA_KEY*>(v)); return 1; });
    EventRegister(EVENT_ID_KEYUP, [](const void* v, void*) -> int32_t { ups.push_back(*static_cast<const EVENT_DATA_KEY*>(v)); return 1; });
    EventRegister(EVENT_ID_CHAR, [](const void* v, void*) -> int32_t { characters.push_back(static_cast<const EVENT_DATA_CHAR*>(v)->m_ch); return 1; });
    for (auto id : {EVENT_ID_MOUSEDOWN, EVENT_ID_MOUSEUP, EVENT_ID_MOUSEMOVE, EVENT_ID_MOUSEMOVE_RELATIVE, EVENT_ID_MOUSEWHEEL})
        EventRegister(id, [](const void* v, void*) -> int32_t { mouse.push_back(*static_cast<const EVENT_DATA_MOUSE*>(v)); return 1; });
    EventRegister(EVENT_ID_IME, [](const void* v, void*) -> int32_t {
        const auto& event = *static_cast<const EVENT_DATA_IME*>(v);
        char text[32]; uint32_t start, end, cursor;
        OsIMEGetCompositionString(text, sizeof(text)); OsIMEGetClauseInfo(start, end, cursor);
        Require(event.m_codePage == 65001 && cursor == start, "Native composition encoding/cursor changed");
        compositions.push_back({event.m_action, event.m_param, text, start, end}); return 1;
    });
    EventRegister(EVENT_ID_0, [](const void*, void*) -> int32_t { EventSetMouseMode(MOUSE_MODE_RELATIVE, 0); return 1; });
    return 1;
}
NSEvent* Key(NSEventType type, unsigned short code, NSString* text, NSEventModifierFlags flags = 0, BOOL repeat = NO) {
    return [NSEvent keyEventWithType:type location:NSZeroPoint modifierFlags:flags timestamp:0 windowNumber:0 context:nil
        characters:text charactersIgnoringModifiers:text isARepeat:repeat keyCode:code];
}
void Step() {
    // The original scheduler enforces a minimum 1 ms context interval.
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    Require(EventProcessPending(), "Native input unexpectedly stopped the Event loop");
}
}
int main() {
    @autoreleasepool {
        try {
            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
            PropInitialize(); EventInitialize(1, 0); OsIMEInitialize();
            struct Cleanup { ~Cleanup() { EventDestroy(); PropDestroy(); } } cleanup;
            const auto context = EventCreateContextEx(1, Initialize, nullptr, 0, 0); Step();
            NSWindow* window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 400, 200)
                styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];
            WowNativeView* view = [[WowNativeView alloc] initWithFrame:NSMakeRect(0, 0, 400, 200) device:nil];
            view.paused = YES; window.contentView = view; view.autoResizeDrawable = NO; view.drawableSize = CGSizeMake(800, 400);
            OsInputSetDrawableSize(800, 400); [window makeFirstResponder:view]; view.gameInputActive = YES; Step();
            [view keyDown:Key(NSEventTypeKeyDown, kVK_ANSI_A, @"a")];
            [view keyDown:Key(NSEventTypeKeyDown, kVK_ANSI_A, @"a", 0, YES)];
            [view keyUp:Key(NSEventTypeKeyUp, kVK_ANSI_A, @"a")];
            [view flagsChanged:Key(NSEventTypeFlagsChanged, kVK_Shift, @"", NSEventModifierFlagShift | NX_DEVICELSHIFTKEYMASK)];
            [view flagsChanged:Key(NSEventTypeFlagsChanged, kVK_RightShift, @"", NSEventModifierFlagShift | NX_DEVICELSHIFTKEYMASK | NX_DEVICERSHIFTKEYMASK)];
            [view flagsChanged:Key(NSEventTypeFlagsChanged, kVK_Shift, @"", NSEventModifierFlagShift | NX_DEVICERSHIFTKEYMASK)];
            [view flagsChanged:Key(NSEventTypeFlagsChanged, kVK_RightShift, @"", 0)]; Step();
            Require(downs.size() == 4 && ups.size() == 3 && downs[0].m_key == KEY_A && downs[1].m_repeat == 2 &&
                ups[1].m_metaKeyState == (1u << KEY_RSHIFT) && ups[2].m_metaKeyState == 0, "Native repeat or independent modifiers failed");
            Require(characters == std::vector<int32_t>({'a','a'}), "Key input did not produce exactly one character per press/repeat");
            for (int i = 0; i < 80; ++i) {
                [view keyDown:Key(NSEventTypeKeyDown, kVK_ANSI_B, @"b")]; [view keyUp:Key(NSEventTypeKeyUp, kVK_ANSI_B, @"b")];
            }
            Step(); Require(ups.size() == 83 && !OsInputIsKeyDown(KEY_B), "Native event burst lost key releases");

            NSEvent* click = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:NSMakePoint(100, 150) modifierFlags:0 timestamp:0
                windowNumber:window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:1];
            [view mouseDown:click]; [view mouseUp:click]; Step();
            Require(mouse.size() == 2 && mouse[0].m_button == MOUSE_BUTTON_LEFT && mouse[0].m_x == .25f && mouse[0].m_y == .75f &&
                mouse[1].m_buttonState == 0, "Native point/backing conversion or button release failed");
            CGEventRef wheel = CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitLine, 1, 1);
            [view scrollWheel:[NSEvent eventWithCGEvent:wheel]]; CFRelease(wheel); Step();
            Require(mouse.back().m_wheelDistance == 120, "Native wheel is not in original 120-unit steps");
            EventQueuePost(context, EVENT_ID_0, nullptr, 0); Step();
            CGEventRef move = CGEventCreateMouseEvent(nullptr, kCGEventMouseMoved, CGPointZero, kCGMouseButtonLeft);
            CGEventSetIntegerValueField(move, kCGMouseEventDeltaX, 3); CGEventSetIntegerValueField(move, kCGMouseEventDeltaY, -2);
            [view mouseMoved:[NSEvent eventWithCGEvent:move]]; CFRelease(move); Step();
            Require(mouse.back().m_mode == MOUSE_MODE_RELATIVE && mouse.back().m_x == 6 && mouse.back().m_y == -4,
                "Native relative motion lost drawable scaling or Y direction");

            characters.clear(); compositions.clear(); OsIMEEnable(1);
            [view setMarkedText:@"ni" selectedRange:NSMakeRange(1, 1) replacementRange:NSMakeRange(NSNotFound, 0)];
            [view setMarkedText:@"\u4f60" selectedRange:NSMakeRange(0, 1) replacementRange:NSMakeRange(NSNotFound, 0)];
            [view insertText:@"\u4f60\u597d" replacementRange:NSMakeRange(NSNotFound, 0)]; Step();
            Require(compositions.size() == 5 && compositions[0].action == 1 && compositions[1].text == "ni" && compositions[1].start == 1 &&
                compositions[2].text == u8"\u4f60" && compositions[3].flags == 4 && compositions[3].text == u8"\u4f60\u597d" &&
                compositions[4].action == 6 && characters.empty(), "Queued IME snapshots or commit ordering failed");
            [view insertText:@"e\u0301\U0001f600" replacementRange:NSMakeRange(NSNotFound, 0)]; Step();
            Require(characters == std::vector<int32_t>({'e',0x301,0x1F600}), "Committed Unicode scalars were split or normalized");
            uint16_t malformed[] = {0xD800, 'X', 0xDC00}; OsInputQueueText(malformed, 3); Step();
            Require(characters[3] == 0xFFFD && characters[4] == 'X' && characters[5] == 0xFFFD, "Malformed UTF-16 was not replaced safely");
            [view setMarkedText:@"\u4f60\u597d" selectedRange:NSMakeRange(0, 2) replacementRange:NSMakeRange(NSNotFound, 0)]; Step();
            char tiny[5]; OsIMEGetCompositionString(tiny, sizeof(tiny)); Require(std::string(tiny) == u8"\u4f60", "Bounded IME UTF-8 split a character");
            OsInputEnableSecureMode(1); Step();
            Require(!view.hasMarkedText && IsSecureEventInputEnabled() && !OsInputAllowsComposition(), "Secure input did not cancel composition");
            OsQueuePut(OS_INPUT_KEY_DOWN, KEY_W, 1, 0, 0);
            view.gameInputActive = NO; OsQueuePut(OS_INPUT_FOCUS, 0, 0, 0, 0); Step();
            Require(!OsInputIsKeyDown(KEY_W) && !IsSecureEventInputEnabled() && OsInputGetMouseMode() == OS_MOUSE_MODE_NORMAL &&
                Input::s_mouseMode == MOUSE_MODE_NORMAL, "Focus loss retained keys, secure input, or relative capture");
            view.gameInputActive = YES; Require(IsSecureEventInputEnabled(), "Secure input did not resume");
            OsInputEnableSecureMode(0); view.gameInputActive = NO;
            [window orderOut:nil];
            std::puts("Native AppKit -> original Event: key repeat/bursts, independent modifiers, drawable coordinates, buttons/wheel/relative motion, Unicode, IME snapshots/cancellation, secure input and focus passed");
            return 0;
        } catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
    }
}
