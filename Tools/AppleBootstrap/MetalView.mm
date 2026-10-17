#include "Tools/AppleBootstrap/MetalView.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include <cstring>
#include <memory>
#include <stdexcept>

namespace {
void TextureData(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t, uint32_t level,
    void* arg, uint32_t& stride, const void*& data) {
    if (command != GxTex_Latch) { return; }
    const auto& image = *static_cast<BLP::Image*>(arg);
    if (level >= image.mips.size() || image.mips[level].width != width || image.mips[level].height != height) {
        throw std::runtime_error("Game Gx texture callback requested an unavailable authored mip");
    }
    stride = image.mips[level].rowBytes; data = image.mips[level].bytes.data();
}
}

@implementation WowTextureRenderer {
    std::unique_ptr<CGxDeviceMetal> _device;
    std::unique_ptr<BLP::Image> _image;
    CGxTex* _texture;
    CGxBuf* _vertices;
    CGxBuf* _indices;
    CGxShader* _vertexShaders[2];
    CGxShader* _pixelShader;
    __weak MTKView* _view;
    BOOL _active;
}
- (instancetype)initWithView:(MTKView*)view image:(const BLP::Image&)image {
    if ((self = [super init])) {
        if (g_theGxDevicePtr) { throw std::runtime_error("A game Gx device already owns the native host"); }
        C2iVector size{std::max(1, int32_t(view.drawableSize.width)), std::max(1, int32_t(view.drawableSize.height))};
        CGxFormat format(true, size, CGxFormat::Fmt_Argb8888, CGxFormat::Fmt_Ds320, 0, 1, true, false, false, false, 0);
        _device.reset(static_cast<CGxDeviceMetal*>(CreateGxDevice(GxDefaultApi(), static_cast<GxWindowProc>(nullptr), format)));
        if (!_device || !_device->DeviceCreate((__bridge void*)view, format)) { throw std::runtime_error("Cannot create the native game Gx device"); }
        _image = std::make_unique<BLP::Image>(image);
        const EGxTexFormat formats[] = {GxTex_Argb8888, GxTex_Dxt1, GxTex_Dxt3, GxTex_Dxt5};
        const auto textureFormat = formats[unsigned(image.format)];
        CGxTexFlags flags(GxTex_LinearMipLinear);
        _device->TexCreate(GxTex_2d, image.mips[0].width, image.mips[0].height, 0, textureFormat, textureFormat,
            flags, _image.get(), TextureData, "MPQ preview", _texture);
        _device->ShaderCreate(_vertexShaders, GxSh_Vertex, "Shaders\\Vertex", "UI", 2);
        _device->ShaderCreate(&_pixelShader, GxSh_Pixel, "Shaders\\Pixel", "UI", 1);
        struct Vertex { float x, y, z; uint32_t color; float u, v; };
        const Vertex vertices[] = {{-1, 1, 0, 0xFFFFFFFF, 0, 0}, {-1, -1, 0, 0xFFFFFFFF, 0, 1},
            {1, 1, 0, 0xFFFFFFFF, 1, 0}, {1, -1, 0, 0xFFFFFFFF, 1, 1}};
        const uint16_t indices[] = {0, 1, 2, 2, 1, 3};
        auto* vp = _device->PoolCreate(GxPoolTarget_Vertex, GxPoolUsage_Static, sizeof(vertices), GxPoolHintBit_Unk0, "preview vertices");
        auto* ip = _device->PoolCreate(GxPoolTarget_Index, GxPoolUsage_Static, sizeof(indices), GxPoolHintBit_Unk0, "preview indices");
        _vertices = _device->BufCreate(vp, sizeof(Vertex), 4, 0);
        _indices = _device->BufCreate(ip, sizeof(uint16_t), 6, 0);
        GxBufData(_vertices, vertices, sizeof(vertices), 0); GxBufData(_indices, indices, sizeof(indices), 0);
        _view = view;
        view.device = _device->GPU().device;
        view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        view.framebufferOnly = NO; // Gx screenshot / framebuffer copy source.
        view.clearColor = MTLClearColorMake(0.12, 0.14, 0.18, 1);
        view.paused = YES; view.enableSetNeedsDisplay = YES; view.delegate = self;
    }
    return self;
}
- (void)resume { _active = YES; [_view draw]; }
- (void)suspend { _active = NO; _device->Drain(); }
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size { if (_active) { [view draw]; } }
- (void)drawInMTKView:(MTKView*)view {
    if (!_active || !view.drawableSize.width || !view.drawableSize.height) { return; }
    MTLRenderPassDescriptor* pass = view.currentRenderPassDescriptor;
    id<CAMetalDrawable> drawable = view.currentDrawable;
    if (!pass || !drawable) { return; }
    try {
        _device->BeginFrame(pass, drawable);
        GxRsSet(GxRs_VertexShader, _vertexShaders[0]); GxRsSet(GxRs_PixelShader, _pixelShader);
        GxRsSet(GxRs_Texture0, _texture);
        GxRsSet(GxRs_Fog, 0); GxRsSet(GxRs_Culling, 0); GxRsSet(GxRs_DepthTest, 0); GxRsSet(GxRs_DepthWrite, 0);
        GxRsSet(GxRs_BlendingMode, int32_t(GxBlend_Alpha)); GxRsSet(GxRs_AlphaRef, 1);
        const double imageAspect = double(_image->mips[0].width) / _image->mips[0].height;
        const double viewAspect = view.drawableSize.width / view.drawableSize.height;
        C44Matrix matrix(1);
        matrix.Scale(C3Vector(imageAspect > viewAspect ? 1 : imageAspect / viewAspect, imageAspect > viewAspect ? viewAspect / imageAspect : 1, 1));
        const auto registers = matrix.Transpose(); GxShaderConstantsSet(GxSh_Vertex, 0, &registers.a0, 4);
        GxPrimVertexPtr(_vertices, GxVBF_PCT); _device->PrimIndexPtr(_indices);
        CGxBatch batch{GxPrim_Triangles, 0, 6, 0, 3}; GxDraw(&batch, 1); GxScenePresent();
    } catch (const std::exception& error) {
        if (_device->FrameCommand()) { _device->ScenePresent(); }
        _active = NO;
        NSLog(@"Metal Gx preview stopped: %s", error.what());
    }
}
@end
