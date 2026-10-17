#include "Tools/AppleBootstrap/MetalView.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/Apple/Input.h"
#include <cstring>
#include <memory>
#include <stdexcept>

namespace {
class GameEvents {
    static GameEvents* s_current;
    void (*m_paint)(void*);
    void* m_owner;
    std::string m_error;
    static int32_t Paint(const void*, void* arg) {
        auto& self = *static_cast<GameEvents*>(arg);
        self.m_paint(self.m_owner);
        return 1;
    }
    static int32_t Destroy(const void*, void*) { AsyncFileReadDestroy(); return 1; }
    static int32_t Initialize(const void*, void*) {
        try { AsyncFileReadInitialize(0, 20); }
        catch (const std::exception& error) { s_current->m_error = error.what(); }
        EventRegisterEx(EVENT_ID_PAINT, Paint, s_current, 0);
        return 1;
    }
public:
    void Check() { if (!m_error.empty()) { throw std::runtime_error(m_error); } }
    GameEvents(void (*paint)(void*), void* owner) : m_paint(paint), m_owner(owner) {
        if (s_current) { throw std::logic_error("A native game event host already exists"); }
        s_current = this;
        PropInitialize();
        try {
            EventInitialize(1, 0);
            if (!EventCreateContextEx(1, Initialize, Destroy, 1, 0)) { throw std::runtime_error("Cannot create native game Event context"); }
        } catch (...) {
            EventDestroy(); PropDestroy(); s_current = nullptr; throw;
        }
    }
    ~GameEvents() {
        EventDestroy();
        PropDestroy();
        s_current = nullptr;
    }
};
GameEvents* GameEvents::s_current;

// The visible resource follows the original Services worker and Event poll
// completion path. Its bounded BLP decoder retains the original mip payloads.
class AsyncImage {
    CAsyncObject* m_object = nullptr;
    std::vector<uint8_t> m_bytes;
    std::unique_ptr<BLP::Image> m_image;
    std::string m_error;
    bool m_started = false;
    void Finish() {
        AsyncFileReadDestroyObject(m_object); m_object = nullptr;
        std::vector<uint8_t>().swap(m_bytes);
    }
    static void Success(void* arg) {
        auto& self = *static_cast<AsyncImage*>(arg);
        try { self.m_image = std::make_unique<BLP::Image>(BLP::Image::Load(self.m_bytes.data(), self.m_bytes.size(), 256 * 1024 * 1024)); }
        catch (const std::exception& error) { self.m_error = error.what(); }
        self.Finish();
    }
    static void Failure(void* arg) {
        auto& self = *static_cast<AsyncImage*>(arg);
        self.m_error = "The native preview's asynchronous resource read failed";
        self.Finish();
    }
public:
    explicit AsyncImage(const char* name) {
        SFileImpl* file = nullptr;
        if (!SFile::OpenEx(nullptr, name, 0, &file)) { throw std::runtime_error("Cannot open the native preview texture"); }
        try {
            uint32_t high = 0;
            const uint32_t size = SFileGetFileSize(file, &high);
            if (high || !size || size > 64 * 1024 * 1024) { throw std::runtime_error("Native preview texture exceeds the file budget"); }
            m_bytes.resize(size);
            m_object = AsyncFileReadAllocObject();
            m_object->m_file = file; m_object->m_buffer = m_bytes.data(); m_object->m_size = size;
            m_object->m_userArg = this;
            m_object->m_userPostloadCallback = reinterpret_cast<void*>(Success);
            m_object->m_userPostUnloadCallback = reinterpret_cast<void*>(Failure);
        } catch (...) { SFileCloseFile(file); throw; }
    }
    ~AsyncImage() { if (m_object) { AsyncFileReadDestroyObject(m_object); } }
    const BLP::Image* Get() {
        if (!m_started) { AsyncFileReadObject(m_object, 0); m_started = true; }
        if (!m_error.empty()) { throw std::runtime_error(m_error); }
        return m_image.get();
    }
};

class GameText {
    CGxFont* m_font = nullptr;
    CGxString* m_string = nullptr;
    CGxStringBatch* m_batch = nullptr;
public:
    GameText() { GxuFontInitialize(); }
    ~GameText() {
        if (m_batch) { GxuFontDestroyBatch(m_batch); }
        if (m_string) { GxuFontDestroyString(m_string); }
        if (m_font) { GxuFontDestroyFont(m_font); }
        GxuFontShutdown();
        TextureGxCacheFlush();
    }
    void Draw(uint32_t width, uint32_t height) {
        CoordinateSetAspectRatio(float(width) / height);
        GxuFontWindowSizeChanged();
        if (!m_font && !GxuFontCreateFont("Fonts\\ARIALN.TTF", 0.065f, m_font, 0)) {
            throw std::runtime_error("Cannot open the game font for the native preview");
        }
        if (!m_batch) { m_batch = GxuFontCreateBatch(false, false); }
        if (!m_string) {
            if (!GxuFontCreateString(m_font, "World of Warcraft\n|cffffcc00Wrath of the Lich King|r", 0.065f,
                C3Vector(0.05f, 0.01f, 0), 0.9f, 0.16f, 0, m_string, GxVJ_Top, GxHJ_Center, 0,
                CImVector(255, 255, 255, 255), 0, 1)) {
                throw std::runtime_error("Cannot lay out the native preview's game text");
            }
            GxuFontAddToBatch(m_batch, m_string);
        }
        GxuFontAddShadow(m_string, CImVector(0, 0, 0, 255), C2Vector(1.5f / width, -1.5f / height));
        GxuFontRenderBatch(m_batch);
    }
};

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

@interface WowTextureRenderer ()
- (void)renderGameFrame;
@end

@implementation WowTextureRenderer {
    std::unique_ptr<CGxDeviceMetal> _device;
    // Declared after the device so the original font system and its texture
    // cache are destroyed while their Gx device is still alive.
    std::unique_ptr<GameText> _text;
    std::unique_ptr<GameEvents> _events;
    std::unique_ptr<AsyncImage> _image;
    CGxTex* _texture;
    CGxBuf* _vertices;
    CGxBuf* _indices;
    CGxShader* _vertexShaders[2];
    CGxShader* _pixelShader;
    __weak MTKView* _view;
    BOOL _active;
    uint32_t _presentedFrames;
}
- (instancetype)initWithView:(MTKView*)view textureName:(const char*)textureName {
    if ((self = [super init])) {
        if (g_theGxDevicePtr) { throw std::runtime_error("A game Gx device already owns the native host"); }
        C2iVector size{std::max(1, int32_t(view.drawableSize.width)), std::max(1, int32_t(view.drawableSize.height))};
        CGxFormat format(true, size, CGxFormat::Fmt_Argb8888, CGxFormat::Fmt_Ds320, 0, 1, true, false, false, false, 0);
        _device.reset(static_cast<CGxDeviceMetal*>(CreateGxDevice(GxDefaultApi(), static_cast<GxWindowProc>(nullptr), format)));
        if (!_device || !_device->DeviceCreate((__bridge void*)view, format)) { throw std::runtime_error("Cannot create the native game Gx device"); }
        _image = std::make_unique<AsyncImage>(textureName);
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
        view.paused = YES; view.enableSetNeedsDisplay = NO; view.delegate = self;
        _events = std::make_unique<GameEvents>([](void* owner) {
            [(__bridge WowTextureRenderer*)owner renderGameFrame];
        }, (__bridge void*)self);
        OsInputSetDrawableSize(uint32_t(size.x), uint32_t(size.y));
    }
    return self;
}
- (void)resume {
    _active = YES;
    EventResetElapsedTime();
    OsQueuePut(OS_INPUT_FOCUS, 1, 0, 0, 0);
    _view.paused = NO;
    [_view draw];
}
- (void)suspend {
    _active = NO; _view.paused = YES;
    OsQueuePut(OS_INPUT_FOCUS, 0, 0, 0, 0);
    EventProcessPending();
    _device->Drain();
}
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    OsInputSetDrawableSize(uint32_t(size.width), uint32_t(size.height));
    OsQueuePut(OS_INPUT_SIZE, int32_t(size.width), int32_t(size.height), 0, 0);
    if (_active) { [view draw]; }
}
- (void)drawInMTKView:(MTKView*)view {
    if (!_active) { return; }
    if (!EventProcessPending()) { [self suspend]; }
}
- (void)renderGameFrame {
    MTKView* view = _view;
    if (!_active || !view.drawableSize.width || !view.drawableSize.height) { return; }
    MTLRenderPassDescriptor* pass = view.currentRenderPassDescriptor;
    id<CAMetalDrawable> drawable = view.currentDrawable;
    if (!pass || !drawable) { return; }
    try {
        _events->Check();
        const auto* image = _image->Get();
        _device->BeginFrame(pass, drawable);
        if (image) {
            if (!_texture) {
                const EGxTexFormat formats[] = {GxTex_Argb8888, GxTex_Dxt1, GxTex_Dxt3, GxTex_Dxt5};
                const auto textureFormat = formats[unsigned(image->format)];
                _device->TexCreate(GxTex_2d, image->mips[0].width, image->mips[0].height, 0, textureFormat, textureFormat,
                    CGxTexFlags(GxTex_LinearMipLinear), const_cast<BLP::Image*>(image), TextureData, "Async MPQ preview", _texture);
            }
            GxRsSet(GxRs_VertexShader, _vertexShaders[0]); GxRsSet(GxRs_PixelShader, _pixelShader);
            GxRsSet(GxRs_Texture0, _texture);
            GxRsSet(GxRs_Fog, 0); GxRsSet(GxRs_Culling, 0); GxRsSet(GxRs_DepthTest, 0); GxRsSet(GxRs_DepthWrite, 0);
            GxRsSet(GxRs_BlendingMode, int32_t(GxBlend_Alpha)); GxRsSet(GxRs_AlphaRef, 1);
            const double imageAspect = double(image->mips[0].width) / image->mips[0].height;
            const double viewAspect = view.drawableSize.width / (view.drawableSize.height * 0.72);
            C44Matrix matrix(1);
            matrix.Scale(C3Vector(imageAspect > viewAspect ? 1 : imageAspect / viewAspect, 0.72 * (imageAspect > viewAspect ? viewAspect / imageAspect : 1), 1));
            matrix.Translate(C3Vector(0, 0.24f, 0));
            const auto registers = matrix.Transpose(); GxShaderConstantsSet(GxSh_Vertex, 0, &registers.a0, 4);
            GxPrimVertexPtr(_vertices, GxVBF_PCT); _device->PrimIndexPtr(_indices);
            CGxBatch batch{GxPrim_Triangles, 0, 6, 0, 3}; GxDraw(&batch, 1);
        }
        if (!_text) { _text = std::make_unique<GameText>(); }
        _text->Draw(uint32_t(view.drawableSize.width), uint32_t(view.drawableSize.height));
        GxScenePresent();
        if (image && ++_presentedFrames == 120) { NSLog(@"Original Event native frame loop: 120 frames presented; Services async texture loaded"); }
    } catch (const std::exception& error) {
        if (_device->FrameCommand()) { _device->ScenePresent(); }
        _active = NO;
        view.paused = YES;
        NSLog(@"Metal Gx preview stopped: %s", error.what());
    }
}
@end
