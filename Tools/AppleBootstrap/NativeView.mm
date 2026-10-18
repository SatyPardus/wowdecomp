#include "NativeView.h"
#if TARGET_OS_OSX
#include "Engine/Source/Os/Apple/Input.h"
#include "Engine/Source/Os/OsIME.h"
#import <Carbon/Carbon.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace {
KEY GameKey(unsigned short code) {
    // Physical keys remain available while the input method consumes text.
    // Layout-dependent characters are delivered separately by insertText.
    switch (code) {
#define K(native, game) case kVK_##native: return KEY_##game
        K(ANSI_A,A); K(ANSI_B,B); K(ANSI_C,C); K(ANSI_D,D); K(ANSI_E,E); K(ANSI_F,F);
        K(ANSI_G,G); K(ANSI_H,H); K(ANSI_I,I); K(ANSI_J,J); K(ANSI_K,K); K(ANSI_L,L);
        K(ANSI_M,M); K(ANSI_N,N); K(ANSI_O,O); K(ANSI_P,P); K(ANSI_Q,Q); K(ANSI_R,R);
        K(ANSI_S,S); K(ANSI_T,T); K(ANSI_U,U); K(ANSI_V,V); K(ANSI_W,W); K(ANSI_X,X); K(ANSI_Y,Y); K(ANSI_Z,Z);
        K(ANSI_0,0); K(ANSI_1,1); K(ANSI_2,2); K(ANSI_3,3); K(ANSI_4,4);
        K(ANSI_5,5); K(ANSI_6,6); K(ANSI_7,7); K(ANSI_8,8); K(ANSI_9,9);
        K(ANSI_Equal,PLUS); K(ANSI_Minus,MINUS); K(ANSI_LeftBracket,BRACKET_OPEN); K(ANSI_RightBracket,BRACKET_CLOSE);
        K(ANSI_Semicolon,SEMICOLON); K(ANSI_Quote,APOSTROPHE); K(ANSI_Comma,COMMA); K(ANSI_Period,PERIOD);
        K(ANSI_Slash,SLASH); K(ANSI_Backslash,BACKSLASH); K(ANSI_Grave,TILDE); K(ISO_Section,TILDE);
        K(Space,SPACE); K(Return,ENTER); K(Tab,TAB); K(Delete,BACKSPACE); K(Escape,ESCAPE);
        K(LeftArrow,LEFT); K(RightArrow,RIGHT); K(UpArrow,UP); K(DownArrow,DOWN);
        K(Home,HOME); K(End,END); K(PageUp,PAGEUP); K(PageDown,PAGEDOWN); K(ForwardDelete,DELETE); K(Help,INSERT);
        K(F1,F1); K(F2,F2); K(F3,F3); K(F4,F4); K(F5,F5); K(F6,F6); K(F7,F7); K(F8,F8);
        K(F9,F9); K(F10,F10); K(F11,F11); K(F12,F12); K(F13,F13); K(F14,F14); K(F15,F15);
        K(F16,F16); K(F17,F17); K(F18,F18); K(F19,F19);
        K(ANSI_Keypad0,NUMPAD0); K(ANSI_Keypad1,NUMPAD1); K(ANSI_Keypad2,NUMPAD2); K(ANSI_Keypad3,NUMPAD3);
        K(ANSI_Keypad4,NUMPAD4); K(ANSI_Keypad5,NUMPAD5); K(ANSI_Keypad6,NUMPAD6); K(ANSI_Keypad7,NUMPAD7);
        K(ANSI_Keypad8,NUMPAD8); K(ANSI_Keypad9,NUMPAD9); K(ANSI_KeypadPlus,NUMPAD_PLUS);
        K(ANSI_KeypadMinus,NUMPAD_MINUS); K(ANSI_KeypadMultiply,NUMPAD_MULTIPLY); K(ANSI_KeypadDivide,NUMPAD_DIVIDE);
        K(ANSI_KeypadDecimal,NUMPAD_DECIMAL); K(ANSI_KeypadEquals,NUMPAD_EQUALS); K(ANSI_KeypadEnter,ENTER); K(ANSI_KeypadClear,NUMLOCK);
        K(Shift,LSHIFT); K(RightShift,RSHIFT); K(Control,LCONTROL); K(RightControl,RCONTROL);
        K(Option,LALT); K(RightOption,RALT); K(CapsLock,CAPSLOCK);
#undef K
        default: return KEY_NONE;
    }
}
uint32_t Scalars(NSString* value, NSUInteger end) {
    uint32_t count = 0;
    for (NSUInteger i = 0; i < std::min(end, value.length); ++i, ++count) {
        const unichar ch = [value characterAtIndex:i];
        if (ch >= 0xD800 && ch <= 0xDBFF && i + 1 < end) {
            const unichar next = [value characterAtIndex:i + 1];
            if (next >= 0xDC00 && next <= 0xDFFF) ++i;
        }
    }
    return count;
}
NSString* PlainText(id value) { return [value isKindOfClass:NSAttributedString.class] ? [value string] : value; }
int32_t Integral(double& remainder, double delta) {
    remainder += delta;
    const auto value = int32_t(std::clamp(std::trunc(remainder), double(INT32_MIN), double(INT32_MAX)));
    remainder -= value;
    return value;
}
}

@interface WowNativeView ()
- (void)textStateChanged;
@end
@implementation WowNativeView {
    NSString* _marked;
    NSRange _selection;
    BOOL _secureInput;
    NSTrackingArea* _tracking;
    double _relativeX, _relativeY, _wheel;
    OS_MOUSE_MODE _lastMouseMode;
}
- (instancetype)initWithFrame:(NSRect)frame device:(id<MTLDevice>)device {
    if ((self = [super initWithFrame:frame device:device])) {
        _marked = @"";
        OsInputSetTextStateCallback([](void* view) { [(__bridge WowNativeView*)view textStateChanged]; }, (__bridge void*)self);
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(languageChanged:)
            name:NSTextInputContextKeyboardSelectionDidChangeNotification object:nil];
    }
    return self;
}
- (void)dealloc {
    OsInputSetTextStateCallback(nullptr, nullptr);
    [NSNotificationCenter.defaultCenter removeObserver:self];
    if (_secureInput) DisableSecureEventInput();
    if (_gameInputActive) OsInputSetMouseMode(OS_MOUSE_MODE_NORMAL);
}
- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    OsInputApplyWindowResizeLock((__bridge void*)self);
}
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent*)event { return YES; }
- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    if (_tracking) [self removeTrackingArea:_tracking];
    _tracking = [[NSTrackingArea alloc] initWithRect:NSZeroRect
        options:NSTrackingMouseMoved | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect owner:self userInfo:nil];
    [self addTrackingArea:_tracking];
}
- (void)setGameInputActive:(BOOL)active {
    if (_gameInputActive == active) return;
    _gameInputActive = active;
    _relativeX = _relativeY = _wheel = 0;
    if (!active) {
        [self cancelComposition];
        OsInputSetMouseMode(OS_MOUSE_MODE_NORMAL);
    }
    if (active) [self languageChanged:nil];
    [self textStateChanged];
}
- (void)languageChanged:(NSNotification*)notification {
    TISInputSourceRef source = TISCopyCurrentKeyboardInputSource();
    NSArray* languages = source ? (__bridge NSArray*)TISGetInputSourceProperty(source, kTISPropertyInputSourceLanguages) : nil;
    NSString* language = languages.firstObject;
    OsInputSetLanguage([language hasPrefix:@"ja"] ? 1 : [language hasPrefix:@"ko"] ? 2 : [language hasPrefix:@"zh"] ? 3 : 0);
    if (source) CFRelease(source);
}
- (void)textStateChanged {
    if (!OsInputAllowsComposition()) [self cancelComposition];
    const BOOL secure = _gameInputActive && OsInputIsSecure();
    if (secure != _secureInput) {
        const OSStatus status = secure ? EnableSecureEventInput() : DisableSecureEventInput();
        if (status != noErr) throw std::runtime_error("Cannot change native secure text input");
        _secureInput = secure;
    }
}
- (void)keyDown:(NSEvent*)event {
    if (!_gameInputActive) return;
    if (event.modifierFlags & NSEventModifierFlagCommand) { [super keyDown:event]; return; }
    const KEY key = GameKey(event.keyCode);
    // While composing, Return/Escape/arrows belong to the input method and
    // must not submit/cancel/navigate the original edit box underneath it.
    if (!_marked.length && key != KEY_NONE) OsQueuePut(OS_INPUT_KEY_DOWN, int32_t(key), event.isARepeat ? 2 : 1, 0, 0);
    if (OsInputAllowsComposition()) [self interpretKeyEvents:@[event]];
    else if (!(event.modifierFlags & NSEventModifierFlagControl)) {
        // Control characters are key events, not printable edit-box content.
        NSString* value = event.characters;
        if (value.length && [value characterAtIndex:0] >= 32 && [value characterAtIndex:0] != 127 &&
            !([value characterAtIndex:0] >= 0xF700 && [value characterAtIndex:0] <= 0xF8FF))
            [self insertText:value replacementRange:NSMakeRange(NSNotFound, 0)];
    }
}
- (void)keyUp:(NSEvent*)event {
    const KEY key = GameKey(event.keyCode);
    if (_gameInputActive && key != KEY_NONE && OsInputIsKeyDown(key)) OsQueuePut(OS_INPUT_KEY_UP, int32_t(key), 1, 0, 0);
}
- (void)flagsChanged:(NSEvent*)event {
    if (!_gameInputActive) return;
    const KEY key = GameKey(event.keyCode);
    if (key == KEY_NONE) return;
    // Device-specific masks distinguish release of one side while the other
    // Shift/Control/Option remains pressed. Aggregate AppKit masks cannot.
    uint64_t mask = 0;
    switch (key) {
        case KEY_LSHIFT: mask = NX_DEVICELSHIFTKEYMASK; break; case KEY_RSHIFT: mask = NX_DEVICERSHIFTKEYMASK; break;
        case KEY_LCONTROL: mask = NX_DEVICELCTLKEYMASK; break; case KEY_RCONTROL: mask = NX_DEVICERCTLKEYMASK; break;
        case KEY_LALT: mask = NX_DEVICELALTKEYMASK; break; case KEY_RALT: mask = NX_DEVICERALTKEYMASK; break;
        case KEY_CAPSLOCK: mask = NSEventModifierFlagCapsLock; break; default: return;
    }
    OsQueuePut((event.modifierFlags & mask) ? OS_INPUT_KEY_DOWN : OS_INPUT_KEY_UP, int32_t(key), 1, 0, 0);
}
- (NSPoint)gamePoint:(NSEvent*)event {
    NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    point.x -= self.bounds.origin.x; point.y -= self.bounds.origin.y;
    if (!self.isFlipped) point.y = self.bounds.size.height - point.y;
    // drawableSize can differ from the backing size (dynamic resolution).
    return NSMakePoint(point.x * self.drawableSize.width / std::max(CGFloat(1), self.bounds.size.width),
                       point.y * self.drawableSize.height / std::max(CGFloat(1), self.bounds.size.height));
}
- (void)button:(NSEvent*)event down:(BOOL)down {
    if (!_gameInputActive) return;
    if (down) [self.window makeFirstResponder:self];
    const NSInteger index = event.buttonNumber;
    if (index < 0 || index > 14) return;
    const uint32_t button = index == 0 ? MOUSE_BUTTON_LEFT : index == 1 ? MOUSE_BUTTON_RIGHT : index == 2 ? MOUSE_BUTTON_MIDDLE : (1u << index);
    const NSPoint point = [self gamePoint:event];
    OsQueuePut(down ? OS_INPUT_MOUSE_DOWN : OS_INPUT_MOUSE_UP, button, int32_t(point.x), int32_t(point.y), 0);
}
- (void)mouseDown:(NSEvent*)event { [self button:event down:YES]; }
- (void)mouseUp:(NSEvent*)event { [self button:event down:NO]; }
- (void)rightMouseDown:(NSEvent*)event { [self button:event down:YES]; }
- (void)rightMouseUp:(NSEvent*)event { [self button:event down:NO]; }
- (void)otherMouseDown:(NSEvent*)event { [self button:event down:YES]; }
- (void)otherMouseUp:(NSEvent*)event { [self button:event down:NO]; }
- (void)mouseMoved:(NSEvent*)event {
    if (!_gameInputActive) return;
    if (_lastMouseMode != OsInputGetMouseMode()) { _relativeX = _relativeY = 0; _lastMouseMode = OsInputGetMouseMode(); }
    if (OsInputGetMouseMode() == OS_MOUSE_MODE_RELATIVE) {
        const double sx = self.drawableSize.width / std::max(CGFloat(1), self.bounds.size.width);
        const double sy = self.drawableSize.height / std::max(CGFloat(1), self.bounds.size.height);
        const int32_t x = Integral(_relativeX, event.deltaX * sx), y = Integral(_relativeY, event.deltaY * sy);
        if (x || y) OsQueuePut(OS_INPUT_MOUSE_MOVE_RELATIVE, 0, x, y, 0);
    } else {
        _relativeX = _relativeY = 0;
        const NSPoint point = [self gamePoint:event];
        OsQueuePut(OS_INPUT_MOUSE_MOVE, 0, int32_t(point.x), int32_t(point.y), 0);
    }
}
- (void)mouseDragged:(NSEvent*)event { [self mouseMoved:event]; }
- (void)rightMouseDragged:(NSEvent*)event { [self mouseMoved:event]; }
- (void)otherMouseDragged:(NSEvent*)event { [self mouseMoved:event]; }
- (void)scrollWheel:(NSEvent*)event {
    if (!_gameInputActive) return;
    // NSEvent deltaY is in wheel units even on precise devices; use that
    // normalized value, not scrollingDeltaY's trackpad pixel distance.
    const int32_t distance = Integral(_wheel, event.deltaY * 120.0);
    const NSPoint point = [self gamePoint:event];
    if (distance) OsQueuePut(OS_INPUT_MOUSE_WHEEL, distance, int32_t(point.x), int32_t(point.y), 0);
}
- (void)insertText:(id)value replacementRange:(NSRange)range {
    if (!_gameInputActive) return;
    NSString* text = PlainText(value);
    if (_marked.length) {
        OsInputQueueComposition(2, {text.UTF8String ?: "", 0, 0}, true);
        _marked = @""; _selection = NSMakeRange(0, 0);
        OsInputQueueComposition(6, {});
    } else {
        std::vector<uint16_t> units(text.length);
        [text getCharacters:units.data() range:NSMakeRange(0, text.length)];
        OsInputQueueText(units.data(), units.size());
    }
}
- (void)setMarkedText:(id)value selectedRange:(NSRange)selection replacementRange:(NSRange)range {
    if (!_gameInputActive || !OsInputAllowsComposition()) return;
    if (!_marked.length) OsInputQueueComposition(1, {});
    NSString* text = PlainText(value);
    if (range.location != NSNotFound && range.location <= _marked.length && range.length <= _marked.length - range.location)
        _marked = [_marked stringByReplacingCharactersInRange:range withString:text];
    else _marked = [text copy];
    selection.location = std::min(selection.location, _marked.length);
    selection.length = std::min(selection.length, _marked.length - selection.location);
    _selection = selection;
    OsInputQueueComposition(2, {_marked.UTF8String ?: "", Scalars(_marked, selection.location), Scalars(_marked, NSMaxRange(selection))});
}
- (void)unmarkText {
    if (_marked.length) [self insertText:_marked replacementRange:NSMakeRange(NSNotFound, 0)];
}
- (void)cancelComposition {
    if (_marked.length) {
        // Remove the transient original edit-box highlight before ending IME.
        OsInputQueueComposition(2, {});
        OsInputQueueComposition(6, {});
        _marked = @""; _selection = NSMakeRange(0, 0);
        [self.inputContext discardMarkedText];
    }
}
- (BOOL)hasMarkedText { return _marked.length != 0; }
- (NSRange)markedRange { return _marked.length ? NSMakeRange(0, _marked.length) : NSMakeRange(NSNotFound, 0); }
- (NSRange)selectedRange { return _selection; }
- (NSArray*)validAttributesForMarkedText { return @[]; }
- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actual {
    if (range.location == NSNotFound || range.location >= _marked.length) { if (actual) *actual = NSMakeRange(NSNotFound, 0); return nil; }
    range.length = std::min(range.length, _marked.length - range.location);
    if (actual) *actual = range;
    return [[NSAttributedString alloc] initWithString:[_marked substringWithRange:range]];
}
- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actual {
    if (actual) *actual = [self markedRange];
    NSRect caret = self.textInputRectProvider ? self.textInputRectProvider() : NSZeroRect;
    if (!NSIsEmptyRect(caret)) {
        if (self.isFlipped) caret.origin.y = 1 - NSMaxY(caret);
        caret = NSMakeRect(self.bounds.origin.x + caret.origin.x * self.bounds.size.width,
            self.bounds.origin.y + caret.origin.y * self.bounds.size.height,
            caret.size.width * self.bounds.size.width, caret.size.height * self.bounds.size.height);
    } else caret = NSMakeRect(8, self.isFlipped ? self.bounds.size.height - 24 : 8, 1, 16);
    return [self.window convertRectToScreen:[self convertRect:caret toView:nil]];
}
- (NSUInteger)characterIndexForPoint:(NSPoint)point { return NSNotFound; }
- (void)doCommandBySelector:(SEL)selector { /* Editing/navigation is already in the original key event queue. */ }
@end
#endif
