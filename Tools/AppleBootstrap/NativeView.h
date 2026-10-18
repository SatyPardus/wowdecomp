#pragma once
#import <MetalKit/MetalKit.h>
#import <TargetConditionals.h>
#if TARGET_OS_OSX
// The native window boundary. All gameplay/UI input goes through Os/Event.
@interface WowNativeView : MTKView <NSTextInputClient>
@property(nonatomic) BOOL gameInputActive;
@end
#endif
