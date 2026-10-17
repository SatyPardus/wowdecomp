#pragma once
#import <MetalKit/MetalKit.h>
#include "Engine/Source/BLPFile/BlpImage.h"

@interface WowTextureRenderer : NSObject <MTKViewDelegate>
- (instancetype)initWithView:(MTKView*)view image:(const BLP::Image&)image;
- (void)resume;
- (void)suspend;
@end
