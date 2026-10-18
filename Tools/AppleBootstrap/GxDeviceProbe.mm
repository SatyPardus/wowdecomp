#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Gx/Gx.h"
#include <cstring>
#include <cmath>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>

void CheckDisplaySettings(CGxDeviceMetal& device);
void CheckGxFFXPasses(CGxDeviceMetal& device);
void CheckGxFFXEffect(CGxDeviceMetal& device);
void CheckGameEvents();
void CheckGameAsyncFiles(bool modelsOnly = false);
void CheckGameWorldLifecycle();
void CheckFixedUnlit(CGxDeviceMetal&);
void CheckModelShaders(CGxDeviceMetal& device);
void CheckShadowMapShaders(CGxDeviceMetal& device);
void CheckDetailDoodadShaders(CGxDeviceMetal& device);
void CheckWaterRippleShaders(CGxDeviceMetal& device);
void CheckTerrainShaders(CGxDeviceMetal& device);
void CheckTerrainShadowGenerator(CGxDeviceMetal& device);
void CheckWmoShaders(CGxDeviceMetal& device);
void CheckLiquidShaders(CGxDeviceMetal& device);
void CheckGxFont(CGxDeviceMetal& device);
void CheckOriginalUI(CGxDeviceMetal& device);
void CheckGxRenderTargets(CGxDeviceMetal& device);
void CheckPackedTextureFormats(CGxDeviceMetal& device);

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
struct FontPixels { uint16_t values[16]; bool partial = false; };
void FontTextureCallback(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t,
    void* arg, uint32_t& stride, const void*& data) {
    if (command != GxTex_Latch) { return; }
    auto& font = *static_cast<FontPixels*>(arg);
    stride = font.partial ? 2 : 32; data = font.values + (font.partial ? 5 : 0);
}
struct Vertex { float x, y, z; uint32_t color; float u, v; };
void Vertices(Vertex* output, uint32_t color, float z) {
    const Vertex vertices[] = {{-1, 1, z, color, 0, 0}, {-1, -1, z, color, 0, 1},
        {1, 1, z, color, 1, 0}, {1, -1, z, color, 1, 1}};
    std::memcpy(output, vertices, sizeof(vertices));
}
}

void CheckGxDevice(const char* focus) {
    Require(!g_theGxDevicePtr, "Gx device probe must run before the game host creates its device");
    SFile::Initialize(); SFile::EnableDirectAccess(0);
    C2iVector size{16, 16};
    CGxFormat format(true, size, CGxFormat::Fmt_Argb8888, CGxFormat::Fmt_Ds320, 0, 1, true, false, false, false, 0);
    std::unique_ptr<CGxDevice> owner(CreateGxDevice(GxDefaultApi(), static_cast<GxWindowProc>(nullptr), format));
    Require(owner && GxApiSupported(GxApi_Metal) && !GxApiSupported(GxApi_D3d9), "Apple Gx factory failed");
    auto& device = *static_cast<CGxDeviceMetal*>(owner.get());
    if (focus && !std::strcmp(focus, "model-shaders")) { CheckModelShaders(device); device.DeviceDestroy(); return; }
    if (focus && !std::strcmp(focus, "ffx")) { CheckGxFFXPasses(device); CheckGxFFXEffect(device); device.DeviceDestroy(); return; }
    if (focus && !std::strcmp(focus, "ui")) { CheckOriginalUI(device); device.DeviceDestroy(); return; }
    CheckDisplaySettings(device);
    if (focus) { CheckGameAsyncFiles(true); device.DeviceDestroy(); return; }
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
    targetDesc.pixelFormat = MTLPixelFormatDepth32Float;
    id<MTLTexture> depth = [gpu.device newTextureWithDescriptor:targetDesc];
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.depthAttachment.texture = depth; pass.depthAttachment.loadAction = MTLLoadActionClear; pass.depthAttachment.clearDepth = 1;
    pass.colorAttachments[0].texture = target; pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
    auto draw = [&](float scale, float translation, uint32_t color, float z = 0) {
        CGxBuf* vertices = GxBufStream(GxPoolTarget_Vertex, sizeof(Vertex), 4);
        Vertices(reinterpret_cast<Vertex*>(device.BufLock(vertices)), color, z);
        GxBufUnlock(vertices, 0); GxPrimVertexPtr(vertices, GxVBF_PCT);
        C44Matrix matrix(1); matrix.Translate(C3Vector(translation, 0, 0)); matrix.Scale(C3Vector(scale, 1, 1));
        const auto registers = matrix.Transpose(); GxShaderConstantsSet(GxSh_Vertex, 0, &registers.a0, 4);
        CGxBatch batch{GxPrim_Triangles, 0, 6, 0, 3}; GxDraw(&batch, 1);
    };
    auto read = [&]() {
        CiRect rect(0, 0, 16, 16); TSGrowableArray<CImVector> pixels;
        GxReadPixels(rect, pixels); GxScenePresent();
        std::vector<uint32_t> result(16 * 16);
        std::memcpy(result.data(), pixels.Ptr(), result.size() * 4);
        return result;
    };
    // Original Desaturate BLS dots the texture RGB before applying vertex alpha;
    // it deliberately does not tint RGB with the vertex color.
    CGxShader* desaturate = nullptr;
    device.ShaderCreate(&desaturate, GxSh_Pixel, "Shaders\\Pixel", "Desaturate", 1);
    GxRsSet(GxRs_PixelShader, desaturate);
    const int32_t oldAlpha = device.m_appRenderStates[GxRs_AlphaRef].m_value.m_data.u[0];
    for (const auto color : {0xFFFF0000u, 0xFF00FF00u, 0xFF0000FFu, 0xFF8143BEu}) {
        for (auto& value : pixels.values) value = color;
        CiRect changed(0, 0, 2, 2); GxTexUpdate(texture, changed, 0);
        const int gray = std::lround(0.299f * ((color >> 16) & 255) + 0.587f * ((color >> 8) & 255) + 0.114f * (color & 255));
        for (int alphaRef : {0, 127, 128}) {
            GxRsSet(GxRs_AlphaRef, alphaRef);
            for (uint32_t tint : {0x7FFFFFFFu, 0x7F000000u, 0x7FFF0000u}) {
                device.BeginFrame(pass); draw(1, 0, tint);
                const auto output = read();
                for (const auto value : output) {
                    if (alphaRef == 128) Require(value == 0xFF000000u, "Desaturate alpha rejection changed");
                    else Require((value >> 24) == 127 && std::abs(int(value & 255) - gray) <= 1 &&
                        std::abs(int((value >> 8) & 255) - gray) <= 1 && std::abs(int((value >> 16) & 255) - gray) <= 1,
                        "Desaturate luminance, vertex tint or alpha boundary changed");
                }
            }
        }
    }
    GxRsSet(GxRs_AlphaRef, oldAlpha);
    GxRsSet(GxRs_PixelShader, pixel); device.ShaderDestroy(desaturate);
    for (auto& value : pixels.values) value = 0xFFFFFFFFu;
    CiRect restored(0, 0, 2, 2); GxTexUpdate(texture, restored, 0);
    std::puts("Desaturate: 36 original shader luminance/tint/alpha checks passed");
    // Batch private vertex/index uploads, including a repeated entry. A later
    // update must remain ordered after the first draw without a CPU/GPU wait.
    auto *preparedPool =
        device.PoolCreate(GxPoolTarget_Vertex, GxPoolUsage_Static, 4 * sizeof(Vertex), GxPoolHintBit_Unk0, "prepared vertices");
    auto *preparedVertices = device.BufCreate(preparedPool, sizeof(Vertex), 4, 0);
    Vertex preparedData[4];
    Vertices(preparedData, 0xFFFF0000, 0);
    device.BufData(preparedVertices, preparedData, sizeof(preparedData), 0);
    CGxBuf *prepared[] = {preparedVertices, indices, preparedVertices};
    device.BeginFrame(pass);
    const auto uploadStart = device.EncodingStats();
    device.PrepareBuffers(prepared, 3);
    Require(device.EncodingStats().bufferUploadPasses - uploadStart.bufferUploadPasses == 1,
            "Private buffer set did not share its upload encoder");
    auto drawPrepared = [&](float translation) {
        GxPrimVertexPtr(preparedVertices, GxVBF_PCT);
        C44Matrix matrix(1);
        matrix.Translate(C3Vector(translation, 0, 0));
        matrix.Scale(C3Vector(0.5f, 1, 1));
        const auto registers = matrix.Transpose();
        GxShaderConstantsSet(GxSh_Vertex, 0, &registers.a0, 4);
        CGxBatch batch{GxPrim_Triangles, 0, 6, 0, 3};
        GxDraw(&batch, 1);
    };
    drawPrepared(-0.5f);
    const auto resident = device.EncodingStats();
    device.PrepareBuffers(prepared, 3);
    Require(device.EncodingStats().bufferUploadPasses == resident.bufferUploadPasses &&
                device.EncodingStats().renderPasses == resident.renderPasses,
            "Resident preparation split a render pass or copied geometry");
    Vertices(preparedData, 0xFF00FF00, 0);
    device.BufData(preparedVertices, preparedData, sizeof(preparedData), 0);
    device.PrepareBuffers(prepared, 3);
    drawPrepared(0.5f);
    auto ordered = read();
    for (size_t y = 0; y < 16; ++y)
        for (size_t x = 0; x < 16; ++x)
            Require(ordered[y * 16 + x] == (x < 8 ? 0xFFFF0000u : 0xFF00FF00u), "Prepared buffer update changed an earlier draw");
    device.BufDestroy(preparedVertices);
    device.PoolDestroy(preparedPool);
    device.BeginFrame(pass); draw(0.5f, -0.5f, 0xFFFF0000);
    CiRect left(0, 0, 16, 8); TSGrowableArray<CImVector> leftPixels;
    GxReadPixels(left, leftPixels);
    for (uint32_t i = 0; i < leftPixels.Count(); ++i) {
        Require(leftPixels[i].r == 255 && leftPixels[i].g == 0, "Gx mid-frame pixel readback failed");
    }
    draw(0.5f, 0.5f, 0xFF00FF00);
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
    device.BeginFrame(pass); draw(1, 0, 0xFFFFFFFF);
    CiRect lowerRight(8, 8, 16, 16); TSGrowableArray<CImVector> region;
    GxReadPixels(lowerRight, region);
    for (uint32_t i = 0; i < region.Count(); ++i) {
        Require(region[i].b == 255 && region[i].r == 0 && region[i].g == 0, "Gx pixel readback origin/row order failed");
    }
    CiRect clipped(-4, -4, 2, 2); GxReadPixels(clipped, region);
    Require(region.Count() == 4 && clipped.minX == 0 && clipped.minY == 0 && region[0].g == 255, "Gx clipped readback failed");
    CiRect empty(20, 20, 24, 24); GxReadPixels(empty, region); Require(!region.Count(), "Gx empty readback failed");
    device.DeviceTakeScreenShot(); GxScenePresent();
    uint32_t shotWidth = 0, shotHeight = 0; const CImVector* shot = nullptr;
    device.DeviceReadScreenShot(shotWidth, shotHeight, shot);
    Require(shotWidth == 16 && shotHeight == 16 && shot && shot[0].g == 255 && shot[255].b == 255, "Gx present-time screenshot failed");
    device.BeginFrame(pass); GxRsSet(GxRs_DepthWrite, 1); draw(1, 0, 0xFFFFFFFF, 0.25f);
    CiRect depthRegion(3, 5, 9, 11); TSGrowableArray<float> samples;
    device.DeviceReadDepths(depthRegion, samples);
    Require(samples.Count() == 36, "Gx depth readback extent mismatch");
    for (uint32_t i = 0; i < samples.Count(); ++i) { Require(samples[i] == 0.25f, "Gx native depth readback mismatch"); }
    device.SceneClear(2, CImVector()); device.DeviceReadDepths(depthRegion, samples);
    for (uint32_t i = 0; i < samples.Count(); ++i) { Require(samples[i] == 1, "Gx depth clear/readback ordering mismatch"); }
    GxScenePresent(); GxRsSet(GxRs_DepthWrite, 0);
    // Exercise the shared immediate geometry entry used by UI/portrait callers,
    // including packing separate attribute arrays and its streaming index path.
    device.BeginFrame(pass);
    const C3Vector positions[] = {{-1, 1, 0}, {-1, -1, 0}, {1, 1, 0}, {1, -1, 0}};
    const C2Vector coordinates[] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    const CImVector colors[] = {{255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};
    const uint16_t strip[] = {0, 1, 2, 3};
    C44Matrix identity(1); GxShaderConstantsSet(GxSh_Vertex, 0, &identity.a0, 4);
    GxPrimLockVertexPtrs(4, positions, sizeof(C3Vector), nullptr, 0, colors, sizeof(CImVector), nullptr, 0,
        coordinates, sizeof(C2Vector), nullptr, 0);
    GxDrawLockedElements(GxPrim_TriangleStrip, 4, strip); GxPrimUnlockVertexPtrs(); result = read();
    Require(result[0] == 0xFF00FF00 && result[255] == 0xFF0000FF, "Shared Gx immediate geometry path failed");
    FontPixels fontPixels{};
    device.PrimIndexPtr(indices);
    for (unsigned i = 0; i < 16; ++i) { fontPixels.values[i] = uint16_t((i << 12) | ((15 - i) << 8) | (((i + 5) & 15) << 4) | ((i + 9) & 15)); }
    CGxTex* fontTexture = nullptr;
    CGxTexFlags fontFlags(GxTex_Nearest);
    device.TexCreate(GxTex_2d, 16, 1, 0, GxTex_Argb4444, GxTex_Argb4444, fontFlags, &fontPixels, FontTextureCallback, "font UNORM4", fontTexture);
    for (bool partial : {false, true}) {
        if (partial) {
            fontPixels.partial = true; fontPixels.values[5] = 0xF123; fontFlags.m_bit15 = 1;
            GxTexSetFlags(fontTexture, fontFlags); CiRect changed(0, 5, 1, 6); GxTexUpdate(fontTexture, changed, 0);
        }
        device.BeginFrame(pass); GxRsSet(GxRs_Texture0, fontTexture); draw(1, 0, 0xFFFFFFFF); result = read();
        for (unsigned y = 0; y < 16; ++y) { for (unsigned x = 0; x < 16; ++x) {
            uint32_t expected = 0;
            for (unsigned c = 0; c < 4; ++c) { expected |= (((fontPixels.values[x] >> (c * 4)) & 15) * 17) << (c * 8); }
            Require(result[y * 16 + x] == expected, "Gx font ARGB4444 channel/alpha/partial update mismatch");
        } }
    }
    GxTexDestroy(fontTexture);
    // Reuse immutable geometry across state changes. A regional clear and
    // disabling scissors must not leak encoder state into later draws.
    const auto beforeEncoding = device.EncodingStats();
    device.BeginFrame(pass); GxRsSet(GxRs_Texture0, texture); draw(1, 0, 0xFFFFFFFF);
    GxXformSetViewport(0.25f, 0.75f, 0.25f, 0.75f, 0, 1);
    device.SceneClear(1, CImVector(255, 0, 0, 255));
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    device.SetScissor(CRect{0, 0, 0.25f, 1}); GxRsSet(GxRs_ScissorTest, 1);
    CGxBatch repeated{GxPrim_Triangles, 0, 6, 0, 3}; GxDraw(&repeated, 1);
    GxRsSet(GxRs_ScissorTest, 0);
    for (unsigned i = 0; i < 128; ++i) GxDraw(&repeated, 1);
    result = read();
    const auto afterEncoding = device.EncodingStats();
    Require(afterEncoding.geometryDraws - beforeEncoding.geometryDraws == 130 &&
        afterEncoding.renderPasses - beforeEncoding.renderPasses == 1, "Compatible Gx draws split the Metal render pass");
    for (size_t y = 0; y < 16; ++y) { for (size_t x = 0; x < 16; ++x) {
        Require(result[y * 16 + x] == (x >= 8 && y >= 8 ? 0xFF0000FF : 0xFF00FF00), "Reused render encoder retained clear/scissor state");
    } }
    std::fprintf(stderr, "Gx pass reuse: 130 geometry draws and a regional clear in one render pass; pixel output preserved\n");
    // Submit beyond the three in-flight slots before reading any output. Each
    // frame wraps the same tiny logical stream pool between its two draws.
    std::vector<id<MTLTexture>> frames;
    const auto beforeStreams = device.EncodingStats();
    uint64_t warmedAllocations = 0;
    for (unsigned frame = 0; frame < 9; ++frame) {
        auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:16 height:16 mipmapped:NO];
        descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
        frames.push_back([gpu.device newTextureWithDescriptor:descriptor]);
        MTLRenderPassDescriptor* framePass = [pass copy]; framePass.colorAttachments[0].texture = frames.back();
        const uint32_t intensity = 16 + frame * 23;
        device.BeginFrame(framePass);
        draw(0.5f, -0.5f, 0xFF000000 | (intensity << 8) | intensity);
        draw(0.5f, 0.5f, 0xFF000000 | ((255 - intensity) << 8) | (255 - intensity));
        device.ScenePresent();
        if (frame == 2) warmedAllocations = device.EncodingStats().streamPageAllocations;
    }
    const auto afterStreams = device.EncodingStats();
    Require(afterStreams.geometryDraws - beforeStreams.geometryDraws == 18 &&
        afterStreams.renderPasses - beforeStreams.renderPasses == 9 &&
        afterStreams.bufferUploadPasses == beforeStreams.bufferUploadPasses &&
        afterStreams.streamPageAllocations == warmedAllocations, "Streaming geometry did not reuse frame pages/render passes");
    device.Drain();
    for (unsigned frame = 0; frame < frames.size(); ++frame) {
        MTLRenderPassDescriptor* framePass = [pass copy]; framePass.colorAttachments[0].texture = frames[frame];
        framePass.colorAttachments[0].loadAction = MTLLoadActionLoad;
        device.BeginFrame(framePass); result = read();
        for (size_t y = 0; y < 16; ++y) { for (size_t x = 0; x < 16; ++x) {
            const uint32_t intensity = x < 8 ? 16 + frame * 23 : 239 - frame * 23;
            const uint32_t expected = 0xFF000000 | (intensity << (x % 8 >= 4 && y >= 8 ? 0 : 8));
            Require(result[y * 16 + x] == expected, "Streaming page reuse overwrote an in-flight draw");
        } }
    }
    std::fprintf(stderr, "Gx streaming: 18 wrapped draws in 9 passes across three in-flight slots, no upload blits or page allocations after warmup; exact pixels preserved\n");
    Require(pixels.locks == pixels.unlocks && device.TexIsReadyNow(texture), "Gx texture callback lifecycle mismatch");
    bool rejected = false;
    try { device.ShaderCreate(&shared, GxSh_Pixel, "Shaders\\Pixel", "NotImplemented", 1); } catch (const std::runtime_error&) { rejected = true; }
    Require(rejected, "Unknown game shader silently accepted");
    CheckPackedTextureFormats(device);
    CheckGxRenderTargets(device);
    GxTexDestroy(texture); device.BufDestroy(indices); device.PoolDestroy(indicesPool);
    device.ShaderDestroy(vertex[0]); device.ShaderDestroy(vertex[1]); device.ShaderDestroy(pixel);
    CheckGxFont(device);
    CheckGxFFXPasses(device);
    CheckGxFFXEffect(device);
    CheckFixedUnlit(device);
    CheckModelShaders(device);
    CheckShadowMapShaders(device);
    CheckDetailDoodadShaders(device);
    CheckWaterRippleShaders(device);
    CheckTerrainShaders(device);
    CheckTerrainShadowGenerator(device);
    CheckWmoShaders(device);
    CheckLiquidShaders(device);
    CheckGameEvents();
    CheckGameAsyncFiles();
    CheckGameWorldLifecycle();
    CheckOriginalUI(device);
    device.DeviceDestroy();
}
