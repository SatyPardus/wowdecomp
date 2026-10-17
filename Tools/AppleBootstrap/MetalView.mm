#include "Tools/AppleBootstrap/MetalView.h"
#include "Engine/Source/Gx/Metal/TextureDevice.h"
#include <memory>
#include <stdexcept>

@implementation WowTextureRenderer {
    std::unique_ptr<GxMetal::TextureDevice> _gpu;
    id<MTLTexture> _texture;
    GxMetal::GeometryDraw _quad;
    __weak MTKView* _view;
    dispatch_semaphore_t _frames;
    BOOL _active;
}
- (instancetype)initWithView:(MTKView*)view image:(const BLP::Image&)image {
    if ((self = [super init])) {
        _gpu = std::make_unique<GxMetal::TextureDevice>(MTLCreateSystemDefaultDevice());
        _texture = _gpu->Upload(image);
        // Immutable Gx PCT vertices and uint16 indices are shared by all frames.
        struct Vertex { float x, y, z; uint32_t color; float u, v; };
        static_assert(sizeof(Vertex) == 24, "Gx PCT layout");
        const Vertex vertices[] = {{-1, 1, 0, 0xFFFFFFFF, 0, 0}, {-1, -1, 0, 0xFFFFFFFF, 0, 1},
            {1, 1, 0, 0xFFFFFFFF, 1, 0}, {1, -1, 0, 0xFFFFFFFF, 1, 1}};
        const uint16_t indices[] = {0, 1, 2, 2, 1, 3};
        _quad.vertices = [_gpu->device newBufferWithBytes:vertices length:sizeof(vertices) options:MTLResourceStorageModeShared];
        _quad.indices = [_gpu->device newBufferWithBytes:indices length:sizeof(indices) options:MTLResourceStorageModeShared];
        if (!_quad.vertices || !_quad.indices) { throw std::runtime_error("Cannot allocate native preview geometry"); }
        _quad.batch = {GxPrim_Triangles, 0, 6, 0, 3};
        _view = view;
        _frames = dispatch_semaphore_create(3);
        view.device = _gpu->device;
        view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        view.clearColor = MTLClearColorMake(0.12, 0.14, 0.18, 1);
        view.paused = YES;
        view.enableSetNeedsDisplay = YES;
        view.delegate = self;
    }
    return self;
}
- (void)resume {
    _active = YES;
    [_view draw];
}
- (void)suspend {
    _active = NO;
    // Drain already submitted GPU work before the app completes backgrounding.
    // The on-demand preview submits nothing while inactive.
    id<MTLCommandBuffer> fence = [_gpu->queue commandBuffer];
    [fence commit];
    [fence waitUntilCompleted];
}
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    if (_active) { [view draw]; }
}
- (void)drawInMTKView:(MTKView*)view {
    if (!_active || !view.drawableSize.width || !view.drawableSize.height) { return; }
    if (dispatch_semaphore_wait(_frames, DISPATCH_TIME_NOW)) { return; }
    MTLRenderPassDescriptor* pass = view.currentRenderPassDescriptor;
    id<CAMetalDrawable> drawable = view.currentDrawable;
    if (!pass || !drawable) { dispatch_semaphore_signal(_frames); return; }
    try {
        id<MTLCommandBuffer> command = [_gpu->queue commandBuffer];
        command.label = @"WoW BLP preview";
        GxMetal::QuadParameters parameters;
        const double imageAspect = double(_texture.width) / _texture.height;
        const double viewAspect = view.drawableSize.width / view.drawableSize.height;
        if (imageAspect > viewAspect) { parameters.scale.y = viewAspect / imageAspect; }
        else { parameters.scale.x = imageAspect / viewAspect; }
        _quad.clipFromModel.Identity();
        _quad.clipFromModel.Scale(C3Vector(parameters.scale.x, parameters.scale.y, 1));
        GxMetal::TextureDrawState state;
        state.blend = GxBlend_Alpha;
        state.alphaRef = 1;
        _gpu->DrawUnlit(command, pass, _texture, _quad, parameters, state);
        dispatch_semaphore_t frames = _frames;
        [command addCompletedHandler:^(id<MTLCommandBuffer> completed) {
            if (completed.error) { NSLog(@"Metal preview failed: %@", completed.error); }
            dispatch_semaphore_signal(frames);
        }];
        [command presentDrawable:drawable];
        [command commit];
    } catch (const std::exception& error) {
        dispatch_semaphore_signal(_frames);
        _active = NO;
        NSLog(@"Metal preview stopped: %s", error.what());
    }
}
@end
