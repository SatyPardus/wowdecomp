#pragma once
#import <MetalKit/MetalKit.h>
#import <TargetConditionals.h>
#if TARGET_OS_OSX
// The native window boundary. All gameplay/UI input goes through Os/Event.
@interface WowNativeView : MTKView <NSTextInputClient>
@property(nonatomic) BOOL gameInputActive;
// Focused game caret in normalized coordinates, with the origin at bottom left.
// An empty rectangle means that the UI currently has no text insertion point.
@property(nonatomic, copy) NSRect (^textInputRectProvider)(void);
@end
#endif
