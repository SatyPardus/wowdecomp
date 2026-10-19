#pragma once
#import <MetalKit/MetalKit.h>
#import <TargetConditionals.h>
#if TARGET_OS_OSX
// The native window boundary. All gameplay/UI input goes through Os/Event.
@interface WowNativeView : MTKView <NSTextInputClient, NSUserInterfaceValidations>
@property(nonatomic) BOOL gameInputActive;
// Focused game caret in normalized coordinates, with the origin at bottom left.
// An empty rectangle means that the UI currently has no text insertion point.
@property(nonatomic, copy) NSRect (^textInputRectProvider)(void);
// Explicit game edit-box focus, including numeric and secure fields which
// disable IME. Native editing shortcuts must never become gameplay bindings.
@property(nonatomic, copy) BOOL (^textEditingActiveProvider)(void);
- (void)copy:(id)sender;
- (void)cut:(id)sender;
- (void)paste:(id)sender;
- (void)selectAll:(id)sender;
@end
#endif
