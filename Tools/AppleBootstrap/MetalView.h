#pragma once
#import <MetalKit/MetalKit.h>
#include "Engine/Source/BLPFile/BlpImage.h"
#include "Engine/Source/Gx/Metal/UIShader.h"

@interface WowTextureRenderer : NSObject <MTKViewDelegate>
- (instancetype)initWithView:(MTKView*)view image:(const BLP::Image&)image uiShader:(const GxMetal::UIShader&)ui;
- (void)resume;
- (void)suspend;
@end
