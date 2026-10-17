#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Gx/Gx.h"
#include <cstring>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
void Require(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
struct Pixels {
    uint32_t values[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    uint32_t locks = 0, unlocks = 0;
    bool partial = false;
};
void TextureCallback(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t, uint32_t level,
    void* arg, uint32_t& stride, const void*& data) {
    auto& pixels = *static_cast<Pixels*>(arg);
    if (command == GxTex_Lock) { ++pixels.locks; }
    if (command == GxTex_Unlock) { ++pixels.unlocks; }
    if (command == GxTex_Latch) {
        Require(width == 2 && height == 2 && level == 0, "Gx texture callback dimensions changed");
        stride = pixels.partial ? 4 : 8; data = pixels.values;
    }
}
struct Vertex { float x, y, z; uint32_t color; float u, v; };
void Vertices(Vertex* output, uint32_t color) {
    const Vertex vertices[] = {{-1, 1, 0, color, 0, 0}, {-1, -1, 0, color, 0, 1},
        {1, 1, 0, color, 1, 0}, {1, -1, 0, color, 1, 1}};
    std::memcpy(output, vertices, sizeof(vertices));
}
}

void CheckGxDevice() {
    Require(!g_theGxDevicePtr, "Gx device probe must run before the game host creates its device");
    SFile::Initialize(); SFile::EnableDirectAccess(0);
    C2iVector size{16, 16};
    CGxFormat format(true, size, CGxFormat::Fmt_Argb8888, CGxFormat::Fmt_Ds320, 0, 1, true, false, false, false, 0);
    std::unique_ptr<CGxDevice> owner(CreateGxDevice(GxDefaultApi(), static_cast<GxWindowProc>(nullptr), format));
    Require(owner && GxApiSupported(GxApi_Metal) && !GxApiSupported(GxApi_D3d9), "Apple Gx factory failed");
    auto& device = *static_cast<CGxDeviceMetal*>(owner.get());
    auto& gpu = device.GPU();
    CGxShader* vertex[2]{}; CGxShader* pixel = nullptr;
    device.ShaderCreate(vertex, GxSh_Vertex, "Shaders\\Vertex", "UI", 2);
    device.ShaderCreate(&pixel, GxSh_Pixel, "Shaders\\Pixel", "UI", 1);
    Require(vertex[0]->Valid() && vertex[1]->Valid() && pixel->Valid(), "Game UI shader validation failed");
    CGxShader* shared = nullptr;
    device.ShaderCreate(&shared, GxSh_Pixel, "Shaders\\Pixel", "UI", 1);
    Require(shared == pixel && pixel->m_refCount == 2, "Gx shader cache/refcount failed"); device.ShaderDestroy(shared);
    GxRsSet(GxRs_VertexShader, vertex[0]); GxRsSet(GxRs_PixelShader, pixel);
    GxRsSet(GxRs_Fog, 0); GxRsSet(GxRs_DepthTest, 0); GxRsSet(GxRs_DepthWrite, 0); GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_BlendingMode, int32_t(GxBlend_Opaque));
    // Exercise the formerly overflowing signed difference in IRsSync.
    device.IRsSync(1);
    Pixels pixels;
    CGxTex* texture = nullptr;
    CGxTexFlags flags(GxTex_Nearest);
    device.TexCreate(GxTex_2d, 2, 2, 0, GxTex_Argb8888, GxTex_Argb8888, flags, &pixels, TextureCallback, "Gx device probe", texture);
    GxRsSet(GxRs_Texture0, texture);
    device.PoolSizeSet(device.m_vertexPool, 4 * sizeof(Vertex));
    auto* indicesPool = device.PoolCreate(GxPoolTarget_Index, GxPoolUsage_Static, 16, GxPoolHintBit_Unk0, "probe indices");
    auto* indices = device.BufCreate(indicesPool, 2, 6, 2);
    const uint16_t indexData[] = {0, 1, 2, 2, 1, 3};
    GxBufData(indices, indexData, sizeof(indexData), 0); device.PrimIndexPtr(indices);
    auto targetDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:16 mipmapped:NO];
    targetDesc.storageMode = MTLStorageModePrivate; targetDesc.usage = MTLTextureUsageRenderTarget;
    id<MTLTexture> target = [gpu.device newTextureWithDescriptor:targetDesc];
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = target; pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
    auto draw = [&](float scale, float translation, uint32_t color) {
        CGxBuf* vertices = GxBufStream(GxPoolTarget_Vertex, sizeof(Vertex), 4);
        Vertices(reinterpret_cast<Vertex*>(device.BufLock(vertices)), color);
        GxBufUnlock(vertices, 4 * sizeof(Vertex)); GxPrimVertexPtr(vertices, GxVBF_PCT);
        C44Matrix matrix(1); matrix.Translate(C3Vector(translation, 0, 0)); matrix.Scale(C3Vector(scale, 1, 1));
        const auto registers = matrix.Transpose(); GxShaderConstantsSet(GxSh_Vertex, 0, &registers.a0, 4);
        CGxBatch batch{GxPrim_Triangles, 0, 6, 0, 3}; GxDraw(&batch, 1);
    };
    auto read = [&]() {
        id<MTLBuffer> buffer = [gpu.device newBufferWithLength:16 * 256 options:MTLResourceStorageModeShared];
        id<MTLCommandBuffer> command = device.FrameCommand();
        id<MTLBlitCommandEncoder> encoder = [command blitCommandEncoder];
        [encoder copyFromTexture:target sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(16, 16, 1)
            toBuffer:buffer destinationOffset:0 destinationBytesPerRow:256 destinationBytesPerImage:16 * 256];
        [encoder endEncoding]; GxScenePresent(); [command waitUntilCompleted];
        Require(!command.error, "Gx frame GPU execution failed");
        std::vector<uint32_t> result(16 * 16);
        for (size_t y = 0; y < 16; ++y) { std::memcpy(result.data() + y * 16, static_cast<uint8_t*>(buffer.contents) + y * 256, 64); }
        return result;
    };
    device.BeginFrame(pass); draw(0.5f, -0.5f, 0xFFFF0000); draw(0.5f, 0.5f, 0xFF00FF00);
    auto result = read();
    for (size_t y = 0; y < 16; ++y) { for (size_t x = 0; x < 16; ++x) {
        if (result[y * 16 + x] != (x < 8 ? 0xFFFF0000u : 0xFF00FF00u)) {
            char message[128]; std::snprintf(message, sizeof(message), "Gx streaming draw mismatch at %zu,%zu: %08x", x, y, result[y * 16 + x]);
            throw std::runtime_error(message);
        }
    } }
    device.BeginFrame(pass);
    for (auto& value : pixels.values) { value = 0xFFFF0000; }
    CiRect full(0, 0, 2, 2); GxTexUpdate(texture, full, 0); draw(0.5f, -0.5f, 0xFFFFFFFF);
    for (auto& value : pixels.values) { value = 0xFF00FF00; }
    GxTexUpdate(texture, full, 0); draw(0.5f, 0.5f, 0xFFFFFFFF); result = read();
    for (size_t y = 0; y < 16; ++y) { for (size_t x = 0; x < 16; ++x) {
        Require(result[y * 16 + x] == (x < 8 ? 0xFFFF0000 : 0xFF00FF00), "Gx texture update changed an earlier draw");
    } }
    device.BeginFrame(pass);
    pixels.partial = true; pixels.values[0] = 0xFF0000FF; flags.m_bit15 = 1; GxTexSetFlags(texture, flags);
    CiRect dirty(1, 1, 2, 2); GxTexUpdate(texture, dirty, 1); draw(1, 0, 0xFFFFFFFF); result = read();
    for (size_t y = 0; y < 16; ++y) { for (size_t x = 0; x < 16; ++x) {
        Require(result[y * 16 + x] == (x >= 8 && y >= 8 ? 0xFF0000FF : 0xFF00FF00), "Gx partial-origin texture update mismatch");
    } }
    Require(pixels.locks == pixels.unlocks && device.TexIsReadyNow(texture), "Gx texture callback lifecycle mismatch");
    bool rejected = false;
    try { device.ShaderCreate(&shared, GxSh_Pixel, "Shaders\\Pixel", "NotImplemented", 1); } catch (const std::runtime_error&) { rejected = true; }
    Require(rejected, "Unknown game shader silently accepted");
    GxTexDestroy(texture); device.BufDestroy(indices); device.PoolDestroy(indicesPool);
    device.ShaderDestroy(vertex[0]); device.ShaderDestroy(vertex[1]); device.ShaderDestroy(pixel);
    device.DeviceDestroy();
}
