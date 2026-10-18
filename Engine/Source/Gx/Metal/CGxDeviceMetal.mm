#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Metal/UIShader.h"
#include "Engine/Source/Gx/Metal/ModelShader.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Storm/H/storm.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
void Require(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
[[noreturn]] void Unsupported(const char* operation) { throw std::runtime_error(std::string("Metal Gx operation not implemented: ") + operation); }
constexpr size_t ResourceBudget = 256 * 1024 * 1024;
struct BufferBinding { id<MTLBuffer> buffer; size_t offset; };
// Each frame owns append-only shared pages until its GPU completion. Gx's
// logical streaming pool can wrap independently without overwriting a draw.
struct StreamFrame {
    dispatch_semaphore_t ready = dispatch_semaphore_create(1);
    std::vector<id<MTLBuffer>> pages;
    size_t usedPages = 0, usedBytes = 0, usedCapacity = 0;
    void Reset() { usedPages = usedBytes = usedCapacity = 0; }
    void Trim() { pages.resize(usedPages); }
    BufferBinding Store(id<MTLDevice> device, const void* source, size_t size, uint64_t& allocations) {
        Require(size && size <= ResourceBudget, "Invalid Gx streaming snapshot size");
        const size_t alignedSize = (size + 3) & ~size_t(3);
        if (!usedPages || alignedSize > pages[usedPages - 1].length - usedBytes) {
            const size_t capacity = std::max(size_t(1024 * 1024), (alignedSize + 4095) & ~size_t(4095));
            if (usedPages == pages.size() || pages[usedPages].length < capacity ||
                pages[usedPages].length > ResourceBudget - usedCapacity) {
                Require(capacity <= ResourceBudget - usedCapacity, "Gx frame streaming data exceeds memory budget");
                pages.resize(usedPages); // Drop unused pages left by a larger frame.
                auto page = [device newBufferWithLength:capacity options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined];
                Require(page != nil, "Cannot allocate Gx streaming page");
                page.label = @"Gx immutable frame geometry";
                pages.push_back(page); ++allocations;
            }
            usedCapacity += pages[usedPages++].length; usedBytes = 0;
        }
        BufferBinding result{pages[usedPages - 1], usedBytes};
        std::memcpy(static_cast<uint8_t*>(result.buffer.contents) + result.offset, source, size);
        usedBytes += alignedSize;
        return result;
    }
};
struct Update { id<MTLBuffer> staging; size_t offset; };
struct Pool {
    std::vector<uint8_t> shadow;
    id<MTLBuffer> gpu;
    std::vector<Update> updates;
    bool streaming = false;
    std::unordered_map<uint64_t, BufferBinding> snapshots;
};
struct Texture { BLP::Image image; id<MTLTexture> gpu, sampled; uint32_t base = 0; bool renderTarget = false; };
struct Shader { std::shared_ptr<GxMetal::UIShader> ui; uint32_t permutation; GxMetal::FragmentEffect effect; GxMetal::ModelVertexEffect vertexEffect = GxMetal::ModelVertexEffect::Diffuse; GxMetal::WmoVertexEffect wmoVertex = GxMetal::WmoVertexEffect::Diffuse; };
std::vector<uint8_t> ReadShader(const char* path, bool optional = false) {
    SFileImpl* file = nullptr;
    if (!SFileOpenFile(path, &file) || !file) {
        Require(optional, "Cannot open required shader from mounted Data");
        return {};
    }
    struct Close { SFileImpl* file; ~Close() { SFileCloseFile(file); } } close{file};
    uint32_t high = 0;
    const uint32_t size = SFileGetFileSize(file, &high);
    Require(!high && size && size <= 1024 * 1024, "Invalid shader file size");
    std::vector<uint8_t> data(size);
    uint32_t read = 0;
    Require(SFileReadFile(file, data.data(), size, &read, nullptr, 0) && read == size, "Incomplete shader file");
    return data;
}
std::shared_ptr<GxMetal::UIShader> LoadUI() {
    return std::make_shared<GxMetal::UIShader>(GxMetal::UIShader::FromBls(
        ReadShader("Shaders\\Vertex\\vs_3_0\\UI.bls"), ReadShader("Shaders\\Pixel\\ps_3_0\\UI.bls")));
}
const char* ShaderFamily(GxMetal::FragmentEffect effect, EGxShTarget target = GxSh_Pixel, GxMetal::ModelVertexEffect vertexEffect = GxMetal::ModelVertexEffect::Diffuse, GxMetal::WmoVertexEffect wmoVertex = GxMetal::WmoVertexEffect::Diffuse) {
    if (GxMetal::IsLiquidEffect(effect)) return GxMetal::LiquidShaderFamily(effect, target);
    if (GxMetal::IsWmoEffect(effect)) return target == GxSh_Vertex ? GxMetal::WmoVertexFamily(wmoVertex) : GxMetal::WmoPixelFamily(effect);
    switch (effect) {
        case GxMetal::FragmentEffect::DetailDoodad: return "DetailDoodad";
        case GxMetal::FragmentEffect::WaterRipple: return "WaterRipples";
        case GxMetal::FragmentEffect::ShadowMap: return target == GxSh_Vertex ? "ShadowMap" : "ShadowMapSL";
        case GxMetal::FragmentEffect::Terrain: return target == GxSh_Vertex ? "Terrain" : "Terrain1";
        case GxMetal::FragmentEffect::Box4: return "FFXBox4";
        case GxMetal::FragmentEffect::Gauss4: return "FFXGauss4";
        case GxMetal::FragmentEffect::Glow: return "FFXGlow";
        case GxMetal::FragmentEffect::GlowWave: return "FFXGlowWave";
        case GxMetal::FragmentEffect::ModelOpaque: return target == GxSh_Vertex ? GxMetal::ModelVertexFamily(vertexEffect) : "Combiners_Opaque";
        case GxMetal::FragmentEffect::ModelMod: return target == GxSh_Vertex ? GxMetal::ModelVertexFamily(vertexEffect) : "Combiners_Mod";
        case GxMetal::FragmentEffect::ModelOpaqueAddNA: return "Combiners_Opaque_AddNA";
        case GxMetal::FragmentEffect::ModelModAddNA: return "Combiners_Mod_AddNA";
        case GxMetal::FragmentEffect::ModelModMod: return "Combiners_Mod_Mod";
        case GxMetal::FragmentEffect::ModelModAdd: return "Combiners_Mod_Add";
        case GxMetal::FragmentEffect::ModelOpaqueMod2xNAAlpha: return "Combiners_Opaque_Mod2xNA_Alpha";
        case GxMetal::FragmentEffect::ModelOpaqueMod2x: return "Combiners_Opaque_Mod2x";
        default: return "UI";
    }
}
void LoadPostProcess(GxMetal::FragmentEffect effect) {
    // Match IShaderLoad's asset lookup. A present but unknown higher-profile
    // shader must fail validation, never silently select a different effect.
    for (const char* profile : {"ps_3_0", "ps_2_0"}) {
        const auto path = std::string("Shaders\\Pixel\\") + profile + "\\" + ShaderFamily(effect) + ".bls";
        auto bytes = ReadShader(path.c_str(), true);
        if (bytes.empty()) { continue; }
        GxMetal::PostProcessShader::FromBls(bytes, effect);
        return;
    }
    throw std::runtime_error("Cannot find the original FFX filter shader in mounted Data");
}

void LoadModelShader(EGxShTarget target, GxMetal::FragmentEffect effect, GxMetal::ModelVertexEffect vertexEffect) {
    const auto path = std::string(target == GxSh_Vertex ? "Shaders\\Vertex\\vs_3_0\\" : "Shaders\\Pixel\\ps_3_0\\") +
        ShaderFamily(effect, target, vertexEffect) + ".bls";
    GxMetal::ValidateModelShader(ReadShader(path.c_str()), target, effect, vertexEffect);
}

void LoadWaterRippleShader(EGxShTarget target) {
    const auto path = std::string(target == GxSh_Vertex ? "Shaders\\Vertex\\vs_3_0\\" : "Shaders\\Pixel\\ps_3_0\\") + "WaterRipples.bls";
    GxMetal::ValidateWaterRippleShader(ReadShader(path.c_str()), target);
}

void LoadDetailDoodadShader(EGxShTarget target) {
    const auto path = std::string(target == GxSh_Vertex ? "Shaders\\Vertex\\vs_3_0\\" : "Shaders\\Pixel\\ps_3_0\\") + "DetailDoodad.bls";
    GxMetal::ValidateDetailDoodadShader(ReadShader(path.c_str()), target);
}

void LoadShadowMapShader(EGxShTarget target) {
    const auto path = std::string(target == GxSh_Vertex ? "Shaders\\Vertex\\vs_3_0\\" : "Shaders\\Pixel\\ps_3_0\\") +
        ShaderFamily(GxMetal::FragmentEffect::ShadowMap, target) + ".bls";
    GxMetal::ValidateShadowMapShader(ReadShader(path.c_str()), target);
}

void LoadTerrainShader(EGxShTarget target) {
    const auto path = std::string(target == GxSh_Vertex ? "Shaders\\Vertex\\vs_3_0\\" : "Shaders\\Pixel\\ps_3_0\\") +
        ShaderFamily(GxMetal::FragmentEffect::Terrain, target) + ".bls";
    GxMetal::ValidateTerrainShader(ReadShader(path.c_str()), target);
}

void LoadLiquidShader(EGxShTarget target, GxMetal::FragmentEffect effect) {
    const auto path = std::string(target == GxSh_Vertex ? "Shaders\\Vertex\\vs_3_0\\" : "Shaders\\Pixel\\ps_3_0\\") + GxMetal::LiquidShaderFamily(effect, target) + ".bls";
    GxMetal::ValidateLiquidShader(ReadShader(path.c_str()), effect, target);
}

void LoadWmoShader(EGxShTarget target, GxMetal::WmoVertexEffect vertex, GxMetal::FragmentEffect pixel) {
    const auto path = std::string(target == GxSh_Vertex ? "Shaders\\Vertex\\vs_3_0\\" : "Shaders\\Pixel\\ps_3_0\\") +
        (target == GxSh_Vertex ? GxMetal::WmoVertexFamily(vertex) : GxMetal::WmoPixelFamily(pixel)) + ".bls";
    GxMetal::ValidateWmoShader(ReadShader(path.c_str()), target, vertex, pixel);
}

BLP::Format ImageFormat(EGxTexFormat format) {
    switch (format) {
        case GxTex_Argb8888: return BLP::Format::BGRA8;
        case GxTex_Argb4444: return BLP::Format::ARGB4444;
        case GxTex_Argb1555: return BLP::Format::ARGB1555;
        case GxTex_Rgb565: return BLP::Format::RGB565;
        case GxTex_Dxt1: return BLP::Format::BC1;
        case GxTex_Dxt3: return BLP::Format::BC2;
        case GxTex_Dxt5: return BLP::Format::BC3;
        default: Unsupported("texture format conversion");
    }
}
bool Packed16(EGxTexFormat format) {
    return format == GxTex_Argb4444 || format == GxTex_Argb1555 || format == GxTex_Rgb565;
}
bool SupportedConversion(EGxTexFormat target, EGxTexFormat source) {
    return target == source || (source == GxTex_Argb8888 && Packed16(target));
}
// Same truncation as CGxDeviceD3d::ITexUpload -> Blit_Argb8888_*.
// Quantize only when the game requested a 16-bit target format.
uint16_t Pack16(const uint8_t* bgra, EGxTexFormat target) {
    if (target == GxTex_Argb4444) { return (bgra[3] >> 4) << 12 | (bgra[2] >> 4) << 8 | (bgra[1] >> 4) << 4 | bgra[0] >> 4; }
    if (target == GxTex_Argb1555) { return (bgra[3] >> 7) << 15 | (bgra[2] >> 3) << 10 | (bgra[1] >> 3) << 5 | bgra[0] >> 3; }
    return (bgra[2] >> 3) << 11 | (bgra[1] >> 2) << 5 | bgra[0] >> 3;
}
CiRect SurfaceRect(id<MTLTexture> texture, uint32_t level) {
    return {0, 0, int32_t(std::max(NSUInteger(1), texture.height >> level)), int32_t(std::max(NSUInteger(1), texture.width >> level))};
}
bool RectFits(const CiRect& rect, const CiRect& surface) {
    return rect.minX >= 0 && rect.minY >= 0 && rect.maxX > rect.minX && rect.maxY > rect.minY &&
        rect.maxX <= surface.maxX && rect.maxY <= surface.maxY;
}
}

struct CGxDeviceMetal::Impl {
    explicit Impl(id<MTLDevice> device) : gpu(device) {}
    GxMetal::TextureDevice gpu;
    std::unordered_map<CGxPool*, Pool> pools;
    std::vector<Pool*> preparedPools;
    std::unordered_map<CGxTex*, Texture> textures;
    std::unordered_map<CGxShader*, Shader> shaders;
    id<MTLCommandBuffer> command;
    id<MTLRenderCommandEncoder> renderEncoder;
    EncodingStatistics statistics;
    MTLRenderPassDescriptor* pass;
    MTLRenderPassDescriptor* defaultPass;
    id<MTLTexture> targetDepth;
    id<MTLDrawable> drawable;
    void* window = nullptr;
    bool scissorEnabled = false;
    MTLScissorRect scissor{};
    std::array<StreamFrame, 3> streamFrames;
    size_t frameSlot = 0;
    StreamFrame& CurrentFrame() { return streamFrames[frameSlot]; }
    static void ConsumedLoads(MTLRenderPassDescriptor* descriptor) {
        descriptor.colorAttachments[0].loadAction = MTLLoadActionLoad;
        descriptor.depthAttachment.loadAction = MTLLoadActionLoad;
    }
    void EndRender() {
        if (renderEncoder) { [renderEncoder endEncoding]; renderEncoder = nil; }
    }
    id<MTLRenderCommandEncoder> Render() {
        if (!renderEncoder) {
            renderEncoder = [command renderCommandEncoderWithDescriptor:pass];
            Require(renderEncoder != nil, "Cannot begin Gx render pass");
            renderEncoder.label = @"Gx attachment drawing";
            ++statistics.renderPasses;
            ConsumedLoads(pass);
        }
        return renderEncoder;
    }
    void EnsureLoads(MTLRenderPassDescriptor* descriptor) {
        if (descriptor.colorAttachments[0].loadAction != MTLLoadActionLoad ||
            (descriptor.depthAttachment.texture && descriptor.depthAttachment.loadAction != MTLLoadActionLoad)) {
            EndRender();
            id<MTLRenderCommandEncoder> encoder = [command renderCommandEncoderWithDescriptor:descriptor];
            Require(encoder != nil, "Cannot encode Gx attachment load/clear");
            ++statistics.renderPasses;
            [encoder endEncoding]; ConsumedLoads(descriptor);
        }
    }
    void EnsurePass() { EnsureLoads(pass); }
    bool CopyColor(id<MTLTexture> source, const CiRect& sourceRect, id<MTLTexture> dest, uint32_t destLevel,
        const CiRect& destRect, bool linear) {
        Require(command && source && dest, "Gx surface copy requires an active frame and allocated textures");
        Require((source.pixelFormat == MTLPixelFormatBGRA8Unorm || source.pixelFormat == MTLPixelFormatR32Float) && dest.pixelFormat == source.pixelFormat &&
            source.sampleCount == 1 && dest.sampleCount == 1 && !source.framebufferOnly && !dest.framebufferOnly,
            "Gx surface copy format, multisampling or framebuffer-only attachment is not supported");
        // D3D9 StretchRect rejects a stretch within the same surface. Do not
        // introduce undefined Metal read/write feedback, even for an exact copy.
        if (source == dest || destLevel >= dest.mipmapLevelCount || !RectFits(sourceRect, SurfaceRect(source, 0)) ||
            !RectFits(destRect, SurfaceRect(dest, destLevel))) { return false; }
        const int sw = sourceRect.maxX - sourceRect.minX, sh = sourceRect.maxY - sourceRect.minY;
        const int dw = destRect.maxX - destRect.minX, dh = destRect.maxY - destRect.minY;
        EnsurePass();
        if (source == defaultPass.colorAttachments[0].texture) { EnsureLoads(defaultPass); }
        EndRender();
        if (sw == dw && sh == dh) {
            // Equal-sized rectangles land on the same texel centers even for
            // LINEAR. A blit preserves the UNORM8 channels or R32F bits.
            id<MTLBlitCommandEncoder> encoder = [command blitCommandEncoder];
            Require(encoder != nil, "Cannot encode Gx texture copy");
            [encoder copyFromTexture:source sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(sourceRect.minX, sourceRect.minY, 0)
                sourceSize:MTLSizeMake(sw, sh, 1) toTexture:dest destinationSlice:0 destinationLevel:destLevel
                destinationOrigin:MTLOriginMake(destRect.minX, destRect.minY, 0)];
            [encoder endEncoding];
            return true;
        }
        Require(source.pixelFormat != MTLPixelFormatR32Float || !linear || gpu.device.supports32BitFloatFiltering, "This GPU cannot linearly filter R32F copies");
        Require((dest.usage & MTLTextureUsageRenderTarget) != 0, "Gx stretched copy requires a renderable destination");
        if (!(source.usage & MTLTextureUsageShaderRead)) {
            // Some hosts provide a blit-readable but not sampleable attachment.
            // Keep its snapshot on the GPU and on this frame's command stream.
            Require(source.width <= ResourceBudget / 4 / source.height, "Gx copy snapshot exceeds memory budget");
            auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:source.pixelFormat width:source.width height:source.height mipmapped:NO];
            descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageShaderRead;
            id<MTLTexture> snapshot = [gpu.device newTextureWithDescriptor:descriptor];
            Require(snapshot != nil, "Cannot allocate Gx surface snapshot");
            id<MTLBlitCommandEncoder> encoder = [command blitCommandEncoder];
            Require(encoder != nil, "Cannot encode Gx surface snapshot");
            [encoder copyFromTexture:source sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
                sourceSize:MTLSizeMake(source.width, source.height, 1) toTexture:snapshot destinationSlice:0 destinationLevel:0
                destinationOrigin:MTLOriginMake(0, 0, 0)];
            [encoder endEncoding]; source = snapshot;
        }
        auto copyPass = [MTLRenderPassDescriptor renderPassDescriptor];
        copyPass.colorAttachments[0].texture = dest; copyPass.colorAttachments[0].level = destLevel;
        copyPass.colorAttachments[0].loadAction = MTLLoadActionLoad; copyPass.colorAttachments[0].storeAction = MTLStoreActionStore;
        GxMetal::QuadParameters parameters;
        parameters.explicitMip = 1;
        parameters.uvScale = {float(sw) / source.width, float(sh) / source.height};
        parameters.uvOffset = {float(sourceRect.minX) / source.width, float(sourceRect.minY) / source.height};
        GxMetal::TextureDrawState state;
        state.filter = linear ? GxTex_Linear : GxTex_Nearest;
        const MTLViewport viewport{double(destRect.minX), double(destRect.minY), double(dw), double(dh), 0, 1};
        gpu.Draw(command, copyPass, source, parameters, state, &viewport);
        ++statistics.renderPasses;
        return true;
    }
    void ReadSurface(id<MTLTexture> surface, CiRect& rect, void* output) {
        Require(command && surface && !surface.framebufferOnly && surface.sampleCount == 1,
            "Gx readback requires an active frame and a readable single-sample attachment");
        const size_t width = rect.maxX - rect.minX, height = rect.maxY - rect.minY;
        if (!width || !height) { return; }
        const size_t stride = (width * 4 + 255) & ~size_t(255);
        Require(height <= ResourceBudget / stride, "Gx readback exceeds memory budget");
        id<MTLBuffer> result = [gpu.device newBufferWithLength:stride * height options:MTLResourceStorageModeShared];
        id<MTLCommandBuffer> continuation = [gpu.queue commandBuffer];
        Require(result && continuation, "Cannot allocate Gx readback resources");
        EnsurePass();
        EndRender();
        id<MTLBlitCommandEncoder> encoder = [command blitCommandEncoder];
        Require(encoder != nil, "Cannot encode Gx readback");
        [encoder copyFromTexture:surface sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(rect.minX, rect.minY, 0)
            sourceSize:MTLSizeMake(width, height, 1) toBuffer:result destinationOffset:0
            destinationBytesPerRow:stride destinationBytesPerImage:stride * height];
        [encoder endEncoding];
        id<MTLCommandBuffer> submitted = command;
        command = continuation; command.label = @"Gx frame after synchronous readback";
        // Only a synchronous game readback splits submission and waits. The
        // logical frame keeps its in-flight permit and drawable until Present.
        [submitted commit]; [submitted waitUntilCompleted];
        if (submitted.error) { throw std::runtime_error(submitted.error.localizedDescription.UTF8String); }
        for (size_t row = 0; row < height; ++row) {
            std::memcpy(static_cast<uint8_t*>(output) + row * width * 4,
                static_cast<const uint8_t*>(result.contents) + row * stride, width * 4);
        }
    }
    void Queue(Pool& pool, size_t offset, size_t size) {
        if (!size) { return; }
        if (pool.streaming) { pool.snapshots.clear(); return; }
        const size_t start = offset & ~size_t(3), end = (offset + size + 3) & ~size_t(3);
        id<MTLBuffer> staging = [gpu.device newBufferWithBytes:pool.shadow.data() + start length:end - start options:MTLResourceStorageModeShared];
        Require(staging != nil, "Cannot allocate Gx upload staging buffer");
        pool.updates.push_back({staging, start});
    }
    Pool& Storage(CGxPool* pool) {
        Require(pool && pool->m_size > 0 && size_t(pool->m_size) <= ResourceBudget, "Invalid Gx pool size");
        auto& storage = pools[pool];
        const size_t size = (size_t(pool->m_size) + 3) & ~size_t(3);
        if (storage.shadow.size() != size) {
            storage.shadow.resize(size);
            storage.streaming = pool->m_usage == GxPoolUsage_Stream;
            storage.snapshots.clear();
            if (!storage.streaming) {
                storage.gpu = [gpu.device newBufferWithLength:size options:MTLResourceStorageModePrivate];
                Require(storage.gpu != nil, "Cannot allocate private Gx buffer pool");
                storage.gpu.label = @"Gx buffer pool";
            }
            storage.updates.clear();
            Queue(storage, 0, size);
        }
        pool->m_mem = storage.shadow.data();
        pool->m_apiSpecific = &storage;
        return storage;
    }
    void Upload(Pool &storage, id<MTLBlitCommandEncoder> __strong &encoder) {
        if (storage.streaming || storage.updates.empty())
            return;
        if (!encoder) {
            Require(command != nil, "Gx buffer uploads require an active frame");
            EndRender();
            encoder = [command blitCommandEncoder];
            Require(encoder != nil, "Cannot encode Gx buffer uploads");
            ++statistics.bufferUploadPasses;
        }
        for (const auto &update : storage.updates)
            [encoder copyFromBuffer:update.staging
                       sourceOffset:0
                           toBuffer:storage.gpu
                  destinationOffset:update.offset
                               size:update.staging.length];
        storage.updates.clear();
    }
    BufferBinding Flush(CGxBuf* buffer) {
        Require(buffer && buffer->m_valid && buffer->m_hasData, "Gx draw uses an uninitialized buffer");
        auto& storage = Storage(buffer->m_pool);
        Require(buffer->m_index <= storage.shadow.size() && buffer->m_size <= storage.shadow.size() - buffer->m_index,
            "Gx buffer snapshot exceeds pool");
        if (storage.streaming) {
            const uint64_t key = (uint64_t(buffer->m_index) << 32) | buffer->m_size;
            auto cached = storage.snapshots.find(key);
            if (cached != storage.snapshots.end()) return cached->second;
            auto binding = CurrentFrame().Store(gpu.device, storage.shadow.data() + buffer->m_index,
                buffer->m_size, statistics.streamPageAllocations);
            statistics.streamedBytes += buffer->m_size;
            storage.snapshots.emplace(key, binding);
            return binding;
        }
        id<MTLBlitCommandEncoder> encoder = nil;
        Upload(storage, encoder);
        if (encoder) [encoder endEncoding];
        return {storage.gpu, buffer->m_index};
    }
};

CGxDeviceMetal::CGxDeviceMetal(id<MTLDevice> device) : m(std::make_unique<Impl>(device)) {
    m_api = GxApi_Metal;
    m_caps.m_numTmus = 4;
    m_caps.m_numStreams = 1;
    m_caps.m_streamOffset = 1;
    m_caps.m_colorFormat = GxCF_argb;
    m_caps.m_pixelCenterOnEdge = 1;
    m_caps.m_texelCenterOnEdge = 1;
    m_caps.m_maxIndex = 65535;
    m_caps.m_maxTexAnisotropy = 16;
    m_caps.m_texFilterTrilinear = m_caps.m_texFilterAnisotropic = 1;
    m_caps.m_colorWrite = m_caps.m_blendFactor = m_caps.m_shaderAlphaRef = 1;
    for (auto format : {GxTex_Argb8888, GxTex_Dxt1, GxTex_Dxt3, GxTex_Dxt5}) { m_caps.m_texFmt[format] = 1; }
    m_caps.m_texFmt[GxTex_Argb4444] = 1;
    for (auto format : {GxTex_Argb1555, GxTex_Rgb565}) { m_caps.m_texFmt[format] = 1; }
    m_caps.m_rttFormat[GxTex_Argb8888] = m_caps.m_rttFormat[GxTex_R32F] = 1;
    m_caps.m_texTarget[GxTex_2d] = m_caps.m_texTarget[GxTex_NonPow2] = 1;
    m_caps.m_texMaxSize[GxTex_2d] = m_caps.m_texMaxSize[GxTex_NonPow2] = 16384;
    m_caps.m_texNonPow2 = 1;
    // Advertise the original profile; resolve missing assets in the game order.
    // ShaderCreate still rejects every family lacking a verified native effect.
    m_caps.m_shaderTargets[GxSh_Vertex] = 3;
    m_caps.m_shaderTargets[GxSh_Pixel] = 4;
    m_caps.m_shaderConstants[GxSh_Vertex] = m_caps.m_shaderConstants[GxSh_Pixel] = 256;
}
CGxDeviceMetal::~CGxDeviceMetal() {
    if (m->command) {
        m->EndRender();
        // Preserve already encoded uploads even when a caller unwinds a failed frame.
        dispatch_semaphore_t frames = m->CurrentFrame().ready;
        [m->command addCompletedHandler:^(id<MTLCommandBuffer>) { dispatch_semaphore_signal(frames); }];
        [m->command commit];
        m->command = nil;
    }
    Drain();
    while (auto* texture = m_textures.Head()) { TexDestroy(texture); }
    while (!m->shaders.empty()) {
        CGxShader* shader = m->shaders.begin()->first;
        shader->m_refCount = 1;
        ShaderDestroy(shader);
    }
    BufDestroy(m_streamBufs[0]); BufDestroy(m_streamBufs[1]);
    while (auto* pool = m_poolList.Head()) { PoolDestroy(pool); }
    m_vertexPool = m_indexPool = nullptr;
    if (g_theGxDevicePtr == this) { g_theGxDevicePtr = nullptr; }
}
GxMetal::TextureDevice& CGxDeviceMetal::GPU() { return m->gpu; }
CGxDeviceMetal::EncodingStatistics CGxDeviceMetal::EncodingStats() const { return m->statistics; }
void CGxDeviceMetal::Drain() {
    id<MTLCommandBuffer> fence = [m->gpu.queue commandBuffer];
    [fence commit]; [fence waitUntilCompleted];
}
int32_t CGxDeviceMetal::DeviceCreate(GxWindowProc proc, const CGxFormat& format) {
    m_windowProc = proc;
    return DeviceCreate(static_cast<void*>(nullptr), format);
}
int32_t CGxDeviceMetal::DeviceCreate(void* window, const CGxFormat& format) {
    if (!DeviceSetFormat(format)) { return 0; }
    m->window = window;
    m_context = 1;
    m_windowVisible = m_windowFocus = 1;
    if (!m_vertexPool) { DeviceCreatePools(); DeviceCreateStreamBufs(); }
    RsInit();
    return 1;
}
int32_t CGxDeviceMetal::DeviceSetFormat(const CGxFormat& format) {
    if (format.m_size.x <= 0 || format.m_size.y <= 0 || format.m_multisampleCount != 1 ||
        format.m_colorFormat != CGxFormat::Fmt_Argb8888 || format.m_depthFormat != CGxFormat::Fmt_Ds320 || format.m_stereoEnabled) { return 0; }
    m_format = format;
    m_curWindowRect.minX = m_curWindowRect.minY = 0;
    m_curWindowRect.maxX = format.m_size.x; m_curWindowRect.maxY = format.m_size.y;
    m_defWindowRect = m_curWindowRect;
    m->scissor = {0, 0, size_t(format.m_size.x), size_t(format.m_size.y)};
    return 1;
}
void CGxDeviceMetal::DeviceDestroy() { Require(!m->command, "End the Metal frame before destroying its host"); Drain(); m_context = 0; m->window = nullptr; CGxDevice::DeviceDestroy(); }
void* CGxDeviceMetal::DeviceWindow() { return m->window; }
void CGxDeviceMetal::CapsWindowSize(CRect& rect) { rect = m_curWindowRect; }
void CGxDeviceMetal::CapsWindowSizeInScreenCoords(CRect&) { Unsupported("screen-space host window bounds"); }
int32_t CGxDeviceMetal::DeviceWM(EGxWM, uintptr_t, uintptr_t) { Unsupported("window message dispatch"); }
void CGxDeviceMetal::BeginFrame(MTLRenderPassDescriptor* pass, id<MTLDrawable> drawable) {
    Require(m_context && !m->command && pass.colorAttachments[0].texture, "Invalid Metal frame lifecycle");
    Require(pass.colorAttachments[0].texture.pixelFormat == MTLPixelFormatBGRA8Unorm && pass.colorAttachments[0].texture.sampleCount == 1,
        "Unsupported Metal frame attachment");
    Require(pass.colorAttachments[0].level == 0 && pass.colorAttachments[0].slice == 0 && pass.depthAttachment.level == 0 &&
        pass.depthAttachment.slice == 0, "Metal host frame must use base-level attachments");
    Require(!pass.depthAttachment.texture || pass.depthAttachment.texture.pixelFormat == MTLPixelFormatDepth32Float, "Unsupported Metal frame depth");
    const size_t slot = (m->frameSlot + 1) % m->streamFrames.size();
    auto& frame = m->streamFrames[slot];
    dispatch_semaphore_wait(frame.ready, DISPATCH_TIME_FOREVER);
    m->command = [m->gpu.queue commandBuffer];
    if (!m->command) { dispatch_semaphore_signal(frame.ready); throw std::runtime_error("Cannot allocate Metal frame command"); }
    m->frameSlot = slot; frame.Reset();
    for (auto& entry : m->pools) entry.second.snapshots.clear();
    m->command.label = @"Gx game frame";
    m->defaultPass = [pass copy]; m->drawable = drawable;
    m->defaultPass.colorAttachments[0].storeAction = MTLStoreActionStore;
    m->defaultPass.depthAttachment.storeAction = MTLStoreActionStore;
    const bool resized = m_defWindowRect.maxX != pass.colorAttachments[0].texture.width || m_defWindowRect.maxY != pass.colorAttachments[0].texture.height;
    m_defWindowRect.minX = m_defWindowRect.minY = 0;
    m_defWindowRect.maxX = pass.colorAttachments[0].texture.width;
    m_defWindowRect.maxY = pass.colorAttachments[0].texture.height;
    ApplyRenderTarget();
    if (resized) { m->scissor = {0, 0, size_t(m_curWindowRect.maxX), size_t(m_curWindowRect.maxY)}; m_scissorRectDirty = 1; }
}
id<MTLCommandBuffer> CGxDeviceMetal::FrameCommand() const { return m->command; }
void CGxDeviceMetal::ScenePresent() {
    Require(m->command != nil, "No active Metal frame to present");
    if (m_scrShotClick) { DeviceScreenShot(); }
    m->EnsurePass();
    m->EnsureLoads(m->defaultPass);
    m->EndRender();
    m->CurrentFrame().Trim();
    dispatch_semaphore_t frames = m->CurrentFrame().ready;
    [m->command addCompletedHandler:^(id<MTLCommandBuffer> command) {
        if (command.error) { CGxDevice::Log("Metal frame failed: %s", command.error.localizedDescription.UTF8String); }
        dispatch_semaphore_signal(frames);
    }];
    if (m->drawable) { [m->command presentDrawable:m->drawable]; }
    [m->command commit]; m->command = nil; m->drawable = nil;
    m->pass = m->defaultPass = nil;
    CGxDevice::ScenePresent();
}
void CGxDeviceMetal::SceneClear(uint32_t mask, CImVector color) {
    Require(m->command && (mask & ~3u) == 0, "Invalid Metal Gx clear");
    if (!mask) { return; }
    if (mask & 2) { Require(m->pass.depthAttachment.texture != nil, "No depth surface to clear"); }
    const auto viewport = Viewport();
    size_t left = viewport.originX, top = viewport.originY;
    size_t right = left + viewport.width, bottom = top + viewport.height;
    // D3D9 SceneClear synchronizes the viewport, but uses the last applied
    // scissor state/rectangle. Pending RsSet/SetScissor changes wait for Draw.
    if (m->scissorEnabled) {
        left = std::max(left, m->scissor.x); top = std::max(top, m->scissor.y);
        right = std::min(right, m->scissor.x + m->scissor.width); bottom = std::min(bottom, m->scissor.y + m->scissor.height);
    }
    if (right <= left || bottom <= top) { return; }
    const auto whole = [&](id<MTLTexture> texture) { return left == 0 && top == 0 && right == texture.width && bottom == texture.height; };
    if ((!(mask & 1) || whole(m->pass.colorAttachments[0].texture)) && (!(mask & 2) || whole(m->pass.depthAttachment.texture))) {
        m->EndRender();
        if (mask & 1) { m->pass.colorAttachments[0].loadAction = MTLLoadActionClear;
            m->pass.colorAttachments[0].clearColor = MTLClearColorMake(color.r / 255.0, color.g / 255.0, color.b / 255.0, color.a / 255.0); }
        if (mask & 2) { m->pass.depthAttachment.loadAction = MTLLoadActionClear; m->pass.depthAttachment.clearDepth = 1; }
        // Keep clears pending until a draw or resource/target boundary needs
        // their contents. The next draw can consume them in its own pass.
    } else {
        // A load-action clear would erase pixels outside the viewport, or the
        // unused part of a larger scratch depth attachment. Draw only this area.
        m->gpu.ClearRegion(m->command, m->pass, {left, top, right - left, bottom - top}, mask,
            {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, m->Render());
    }
}
void CGxDeviceMetal::IRsSendToHw(EGxRenderState which) {
    if (which == GxRs_ScissorTest) { m->scissorEnabled = m_appRenderStates[which].m_value.m_data.u[0] != 0; }
    // Other pipeline/state snapshots are bound at Draw.
}

MTLViewport CGxDeviceMetal::Viewport() const {
    const float width = m_curWindowRect.maxX, height = m_curWindowRect.maxY;
    auto rounded = [](float value) { Require(std::isfinite(value) && value >= 0, "Invalid Gx viewport"); return std::floor(value + 0.5f); };
    MTLViewport viewport{};
    viewport.originX = rounded(m_viewport.m_x.m_l * width);
    viewport.originY = rounded((1 - m_viewport.m_y.m_h) * height);
    viewport.width = rounded(m_viewport.m_x.m_h * width - float(viewport.originX));
    viewport.height = rounded((1 - m_viewport.m_y.m_l) * height - float(viewport.originY));
    if (m_textureTarget[GxBuffers_Color].m_texture || m_textureTarget[GxBuffers_Depth].m_texture) {
        const float y = height * m_viewport.m_y.m_l;
        Require(std::isfinite(y) && y >= 0, "Invalid Gx offscreen viewport");
        viewport.originY = std::floor(y); // Original D3D9 offscreen origin truncates.
    }
    viewport.znear = m_viewport.m_z.m_l; viewport.zfar = m_viewport.m_z.m_h;
    Require(viewport.originX + viewport.width <= width && viewport.originY + viewport.height <= height &&
        viewport.znear >= 0 && viewport.zfar <= 1 && viewport.znear <= viewport.zfar, "Gx viewport exceeds its target");
    return viewport;
}
MTLScissorRect CGxDeviceMetal::Scissor() const {
    const float width = m_curWindowRect.maxX, height = m_curWindowRect.maxY;
    auto edge = [](float value, float limit) { Require(std::isfinite(value), "Invalid Gx scissor"); return size_t(std::clamp(value, 0.0f, limit)); };
    const size_t left = edge(m_scissorRect.minX * width + 0.5f, width), right = edge(m_scissorRect.maxX * width + 1, width);
    const bool offscreen = m_textureTarget[GxBuffers_Color].m_texture || m_textureTarget[GxBuffers_Depth].m_texture;
    const size_t top = edge((offscreen ? m_scissorRect.minY : 1 - m_scissorRect.maxY) * height + 0.5f, height);
    const size_t bottom = edge((offscreen ? m_scissorRect.maxY : 1 - m_scissorRect.minY) * height + 1, height);
    return {left, top, right > left ? right - left : 0, bottom > top ? bottom - top : 0};
}

void CGxDeviceMetal::PoolSizeSet(CGxPool* pool, uint32_t size) {
    Require(pool && size && size <= ResourceBudget && !m_bufLocked[pool->m_target], "Invalid Gx pool resize");
    // Pool resize invalidates its logical allocations, as in D3D9.
    pool->m_size = int32_t(size); pool->Discard(); m->Storage(pool);
}
void CGxDeviceMetal::PoolDestroy(CGxPool* pool) {
    if (!pool) { return; }
    Require(m_bufLocked[pool->m_target] == nullptr, "Destroying a locked Gx pool");
    while (auto* buffer = pool->m_bufList.Head()) {
        for (auto& stream : m_streamBufs) { if (stream == buffer) { stream = nullptr; } }
        BufDestroy(buffer);
    }
    m->pools.erase(pool); pool->m_apiSpecific = pool->m_mem = nullptr;
    if (pool == m_vertexPool) { m_vertexPool = nullptr; }
    if (pool == m_indexPool) { m_indexPool = nullptr; }
    CGxDevice::PoolDestroy(pool);
}
char* CGxDeviceMetal::BufLock(CGxBuf* buffer) {
    Require(buffer && buffer->m_pool && buffer->m_itemSize && buffer->m_size, "Invalid Gx buffer lock");
    auto* pool = buffer->m_pool;
    Require(pool->m_target < GxPoolTargets_Last && !m_bufLocked[pool->m_target], "Gx pool already locked");
    auto& storage = m->Storage(pool);
    if (pool->m_usage == GxPoolUsage_Stream) {
        size_t offset = (size_t(pool->m_offset) + buffer->m_itemSize - 1) / buffer->m_itemSize * buffer->m_itemSize;
        if (offset + buffer->m_size > size_t(pool->m_size)) { pool->Discard(); offset = 0; }
        buffer->m_index = uint32_t(offset);
        pool->m_offset = int32_t(offset + buffer->m_size);
    }
    Require(buffer->m_index <= uint32_t(pool->m_size) && buffer->m_size <= uint32_t(pool->m_size) - buffer->m_index,
        "Gx buffer lock exceeds pool");
    CGxDevice::BufLock(buffer);
    return reinterpret_cast<char*>(storage.shadow.data() + buffer->m_index);
}
int32_t CGxDeviceMetal::BufUnlock(CGxBuf* buffer, uint32_t size) {
    Require(buffer && buffer->m_pool && m_bufLocked[buffer->m_pool->m_target] == buffer && size <= buffer->m_size,
        "Invalid Gx buffer unlock");
    auto& storage = m->Storage(buffer->m_pool);
    CGxDevice::BufUnlock(buffer, size);
    buffer->m_hasData = 0;
    // The D3D9 lock covers the whole logical buffer; size is only a legacy
    // hint (font/index callers pass zero). Publish all bytes written by Lock.
    m->Queue(storage, buffer->m_index, buffer->m_size);
    buffer->m_hasData = 1;
    return 1;
}
void CGxDeviceMetal::PrepareBuffers(CGxBuf *const *buffers, uint32_t count) {
    Require(!count || (buffers && m->command), "Gx buffer preparation requires an active frame and buffer list");
    // Validate the full set before opening an encoder. Repeated buffers/pools
    // naturally become no-ops after their queued immutable copies are consumed.
    auto &pools = m->preparedPools;
    pools.clear();
    pools.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        auto *buffer = buffers[i];
        Require(buffer && buffer->m_valid && buffer->m_hasData, "Preparing an uninitialized Gx buffer");
        auto &storage = m->Storage(buffer->m_pool);
        Require(buffer->m_index <= storage.shadow.size() && buffer->m_size <= storage.shadow.size() - buffer->m_index,
                "Gx buffer preparation exceeds pool");
        pools.push_back(&storage);
    }
    id<MTLBlitCommandEncoder> encoder = nil;
    for (auto *pool : pools)
        m->Upload(*pool, encoder);
    if (encoder)
        [encoder endEncoding];
}
void CGxDeviceMetal::BufData(CGxBuf* buffer, const void* data, uint32_t size, uint32_t offset) {
    Require(buffer && buffer->m_pool && data && offset <= buffer->m_size && size <= buffer->m_size - offset, "Invalid Gx buffer data range");
    auto* pool = buffer->m_pool;
    Require(!m_bufLocked[pool->m_target], "Gx buffer is already locked");
    if (pool->m_usage == GxPoolUsage_Stream) {
        // Shared GxPrimIndexPtr writes its streaming index buffer via BufData.
        // Follow the same append/discard allocation as the lock/unlock path.
        char* destination = BufLock(buffer);
        std::memcpy(destination + offset, data, size);
        BufUnlock(buffer, offset + size);
        return;
    }
    auto& storage = m->Storage(pool);
    Require(buffer->m_index <= uint32_t(pool->m_size) && buffer->m_size <= uint32_t(pool->m_size) - buffer->m_index, "Gx buffer data exceeds pool");
    const size_t start = size_t(buffer->m_index) + offset;
    std::memcpy(storage.shadow.data() + start, data, size);
    m->Queue(storage, start, size);
    CGxDevice::BufData(buffer, data, size, offset); buffer->m_hasData = 1;
}
int32_t CGxDeviceMetal::TexCreate(EGxTexTarget target, uint32_t width, uint32_t height, uint32_t depth, EGxTexFormat format,
    EGxTexFormat dataFormat, CGxTexFlags flags, void* arg, GxTexUserFunc callback, const char* name, CGxTex*& texture) {
    texture = nullptr;
    Require((target == GxTex_2d || target == GxTex_NonPow2) && width && height && width <= 16384 && height <= 16384 && depth <= 1,
        "Unsupported Gx texture target or extent");
    Require(!flags.m_generateMipMaps, "Gx automatic mip generation is not implemented");
    if (flags.m_renderTarget) {
        Require(format == GxTex_Argb8888 || format == GxTex_R32F, "Gx render target format is not implemented");
    } else {
        Require(SupportedConversion(format, dataFormat) && callback, "Unsupported Gx texture conversion or missing callback");
        Require(!Packed16(format) || m_caps.m_texFmt[format], "Device cannot preserve packed Gx texture filtering precision");
        ImageFormat(format);
    }
    return CGxDevice::TexCreate(target, width, height, depth, format, dataFormat, flags, arg, callback, name, texture);
}
void CGxDeviceMetal::TexDestroy(CGxTex* texture) {
    if (!texture) { return; }
    if (m_textureTarget[GxBuffers_Color].m_texture == texture) { DeviceSetRenderTarget(GxBuffers_Color, nullptr, 0); }
    for (int state = GxRs_Texture0; state <= GxRs_Texture15; ++state) {
        if (m_appRenderStates[state].m_value.m_data.p == texture) { RsSet(EGxRenderState(state), nullptr); }
    }
    m->textures.erase(texture); texture->m_apiSpecificData = nullptr;
    CGxDevice::TexDestroy(texture);
}
void CGxDeviceMetal::DeviceSetBaseMipLevel(uint32_t level) {
    if (m->command) { m->EnsurePass(); }
    for (auto* texture = m_textures.Head(); texture; texture = m_textures.Next(texture)) {
        auto storage = m->textures.find(texture);
        if (storage == m->textures.end() || !storage->second.gpu) { continue; }
        // Match D3D9's ITexForceRecreation: invalidate resident textures and
        // notify their producers before publishing the new base mip. Encoded
        // commands and the currently bound attachment retain their old images.
        texture->m_apiSpecificData = nullptr;
        texture->m_needsCreation = texture->m_needsFlagUpdate = 1;
        TexMarkForUpdate(texture, CiRect(), 0);
        m->textures.erase(storage);
        if (texture->m_userFunc) {
            uint32_t stride = 0; const void* data = nullptr;
            texture->m_userFunc(GxTex_3, texture->m_width, texture->m_height, 0, 0, texture->m_userArg, stride, data);
        }
    }
    // Shadow and portrait caches listen to this independently of individual
    // texture callbacks. Native hosts need no Win32 window for the notification.
    if (m_context) { NotifyOnTextureRecreation(); }
    CGxDevice::DeviceSetBaseMipLevel(level);
}
void CGxDeviceMetal::ITexMarkAsUpdated(CGxTex* texture) {
    Require(texture != nullptr, "Missing Gx texture");
    uint32_t width, height, base, end;
    ITexWHDStartEnd(texture, width, height, base, end);
    width = std::max(width, 1u); height = std::max(height, 1u);
    auto& storage = m->textures[texture];
    if (texture->m_flags.m_renderTarget) {
        Require((texture->m_format == GxTex_Argb8888 || texture->m_format == GxTex_R32F) && !texture->m_flags.m_generateMipMaps,
            "Unsupported Gx render target format or mip generation");
        const bool recreate = texture->m_needsCreation || !storage.gpu || !storage.renderTarget || storage.base != base ||
            storage.gpu.mipmapLevelCount != end - base;
        if (recreate) {
            size_t total = 0;
            for (uint32_t mip = 0; mip < end - base; ++mip) { total += size_t(std::max(1u, width >> mip)) * std::max(1u, height >> mip) * 4; }
            Require(total <= ResourceBudget, "Gx render target exceeds memory budget");
            auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:texture->m_format == GxTex_R32F ? MTLPixelFormatR32Float : MTLPixelFormatBGRA8Unorm width:width height:height mipmapped:NO];
            descriptor.mipmapLevelCount = end - base;
            descriptor.storageMode = MTLStorageModePrivate;
            descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
            if (texture->m_format == GxTex_R32F) descriptor.usage |= MTLTextureUsagePixelFormatView;
            id<MTLTexture> replacement = [m->gpu.device newTextureWithDescriptor:descriptor];
            Require(replacement != nil, "Cannot allocate Gx color render target");
            replacement.label = @"Gx color render target";
            storage.gpu = replacement; storage.base = base; storage.renderTarget = true; storage.image = {};
            storage.sampled = nil;
            if (texture->m_format == GxTex_R32F) {
                // D3D9 defines absent format channels as one. Keep the render
                // attachment unswizzled and give sampling an immutable view.
                const auto swizzle = MTLTextureSwizzleChannelsMake(MTLTextureSwizzleRed, MTLTextureSwizzleOne, MTLTextureSwizzleOne, MTLTextureSwizzleOne);
                storage.sampled = [replacement newTextureViewWithPixelFormat:MTLPixelFormatR32Float textureType:MTLTextureType2D
                    levels:NSMakeRange(0, end - base) slices:NSMakeRange(0, 1) swizzle:swizzle];
                Require(storage.sampled != nil, "Cannot create R32F sampling view");
            }
        }
        texture->m_apiSpecificData = &storage;
        texture->m_needsCreation = texture->m_needsFlagUpdate = 0;
        CGxDevice::ITexMarkAsUpdated(texture);
        return;
    }
    const bool recreate = texture->m_needsCreation || !storage.gpu || storage.renderTarget || storage.base != base || storage.image.mips.size() != end - base;
    if (!recreate && !texture->m_needsUpdate) { texture->m_needsFlagUpdate = 0; return; }
    Require(texture->m_userFunc && !texture->m_flags.m_generateMipMaps && !texture->m_flags.m_renderTarget && SupportedConversion(texture->m_format, texture->m_dataFormat),
        "Unsupported updated Gx texture flags or conversion");
    BLP::Image image = recreate ? BLP::Image{} : storage.image;
    image.format = ImageFormat(texture->m_format);
    const uint32_t block = BLP::IsBlockCompressed(image.format) ? 4 : 1;
    const uint32_t bytes = block == 4 ? (image.format == BLP::Format::BC1 ? 8 : 16) : Packed16(texture->m_format) ? 2 : 4;
    const uint32_t sourceBytes = Packed16(texture->m_dataFormat) ? 2 : texture->m_dataFormat == GxTex_Argb8888 ? 4 : bytes;
    const bool convert = texture->m_format != texture->m_dataFormat;
    uint32_t stride = 0; const void* pixels = nullptr;
    texture->m_userFunc(GxTex_Lock, texture->m_width, texture->m_height, 0, 0, texture->m_userArg, stride, pixels);
    struct Unlock {
        CGxTex* t;
        ~Unlock() { uint32_t stride = 0; const void* data = nullptr; t->m_userFunc(GxTex_Unlock, t->m_width, t->m_height, 0, 0, t->m_userArg, stride, data); }
    } unlock{texture};
    size_t total = 0;
    for (uint32_t mip = base; mip < end; ++mip) {
        const uint32_t rowBytes = ((width + block - 1) / block) * bytes, rows = (height + block - 1) / block;
        total += size_t(rowBytes) * rows;
        Require(total <= ResourceBudget, "Gx texture upload exceeds memory budget");
        if (recreate) { image.mips.push_back({width, height, rowBytes, std::vector<uint8_t>(size_t(rowBytes) * rows)}); }
        auto& dest = image.mips[mip - base];
        CiRect rect = recreate ? CiRect(0, 0, texture->m_height, texture->m_width) : texture->m_updateRect;
        Require(rect.minX >= 0 && rect.minY >= 0 && rect.maxX <= int32_t(texture->m_width) && rect.maxY <= int32_t(texture->m_height), "Invalid Gx texture dirty rectangle");
        uint32_t left = uint32_t(rect.minX) >> mip, top = uint32_t(rect.minY) >> mip;
        uint32_t right = std::min(width, std::max(left + 1, uint32_t(rect.maxX) >> mip));
        uint32_t bottom = std::min(height, std::max(top + 1, uint32_t(rect.maxY) >> mip));
        left /= block; top /= block; right = (right + block - 1) / block; bottom = (bottom + block - 1) / block;
        pixels = nullptr; stride = 0;
        texture->m_userFunc(GxTex_Latch, width, height, 0, mip, texture->m_userArg, stride, pixels);
        Require(pixels && right > left && bottom > top && stride >= (texture->m_flags.m_bit15 ? right - left : right) * sourceBytes,
            "Missing or invalid Gx texture mip callback data");
        Require(!recreate || !texture->m_flags.m_bit15 || (texture->m_updateRect.minX == 0 && texture->m_updateRect.minY == 0 &&
            texture->m_updateRect.maxX == int32_t(texture->m_width) && texture->m_updateRect.maxY == int32_t(texture->m_height)),
            "Cannot recreate a Gx texture from partial-origin callback data");
        auto* source = static_cast<const uint8_t*>(pixels);
        if (!texture->m_flags.m_bit15) { source += size_t(top) * stride + left * sourceBytes; }
        for (uint32_t row = top; row < bottom; ++row) {
            auto* dst = dest.bytes.data() + size_t(row) * rowBytes + left * bytes;
            const auto* src = source + size_t(row - top) * stride;
            if (convert) {
                for (uint32_t x = 0; x < right - left; ++x) {
                    const uint16_t packed = Pack16(src + x * 4, texture->m_format);
                    std::memcpy(dst + x * 2, &packed, sizeof(packed));
                }
            } else {
                std::memcpy(dst, src, (right - left) * bytes);
            }
        }
        width = std::max(1u, width >> 1); height = std::max(1u, height >> 1);
    }
    // Immutable replacement keeps previously encoded frames' texture contents alive.
    storage.gpu = m->gpu.Upload(image); storage.image = std::move(image); storage.base = base; storage.renderTarget = false;
    texture->m_apiSpecificData = &storage;
    texture->m_needsCreation = texture->m_needsFlagUpdate = 0;
    CGxDevice::ITexMarkAsUpdated(texture);
}

void CGxDeviceMetal::ShaderCreate(CGxShader** shaders, EGxShTarget target, const char* path, const char* name, uint32_t count) {
    Require(shaders && path && name && (target == GxSh_Vertex || target == GxSh_Pixel), "Invalid Metal Gx shader request");
    using Vertex = GxMetal::ModelVertexEffect;
    using Effect = GxMetal::FragmentEffect;
    auto vertexEffect = Vertex::Diffuse;
    auto wmoVertex = GxMetal::WmoVertexEffect::Diffuse;
    auto effect = Effect::Unlit;
    if (target == GxSh_Vertex) {
        for (auto candidate : {Vertex::Diffuse, Vertex::Color, Vertex::ColorDiffuse, Vertex::DiffuseEnvironment, Vertex::DiffuseDualUV, Vertex::DiffuseProjected, Vertex::ColorProjected}) {
            if (SStrCmpI(name, GxMetal::ModelVertexFamily(candidate), STORM_MAX_STR) == 0) {
                vertexEffect = candidate; effect = Effect::ModelOpaque; break;
            }
        }
    } else {
        for (auto candidate : {Effect::Box4, Effect::Gauss4, Effect::Glow, Effect::GlowWave,
            Effect::ModelOpaque, Effect::ModelMod, Effect::ModelOpaqueAddNA, Effect::ModelModAddNA,
            Effect::ModelModMod, Effect::ModelOpaqueMod2xNAAlpha, Effect::ModelOpaqueMod2x, Effect::ModelModAdd}) {
            if (SStrCmpI(name, ShaderFamily(candidate), STORM_MAX_STR) == 0) { effect = candidate; break; }
        }
    }
    for (unsigned i = 0; i < 7; ++i) {
        if (target == GxSh_Vertex && SStrCmpI(name, GxMetal::WmoVertexFamily(GxMetal::WmoVertexEffect(i)), STORM_MAX_STR) == 0) {
            wmoVertex = GxMetal::WmoVertexEffect(i); effect = Effect::WmoDiffuse;
        } else if (target == GxSh_Pixel && SStrCmpI(name, GxMetal::WmoPixelFamily(Effect(unsigned(Effect::WmoDiffuse) + i)), STORM_MAX_STR) == 0) {
            effect = Effect(unsigned(Effect::WmoDiffuse) + i);
        }
    }
    for (auto candidate : {Effect::LiquidWater, Effect::LiquidWaterNoSpec, Effect::LiquidMagma})
        if (SStrCmpI(name, GxMetal::LiquidShaderFamily(candidate, target), STORM_MAX_STR) == 0) effect = candidate;
    if (SStrCmpI(name, ShaderFamily(Effect::Terrain, target), STORM_MAX_STR) == 0) effect = Effect::Terrain;
    if (SStrCmpI(name, ShaderFamily(Effect::ShadowMap, target), STORM_MAX_STR) == 0) effect = Effect::ShadowMap;
    if (SStrCmpI(name, "DetailDoodad", STORM_MAX_STR) == 0) effect = Effect::DetailDoodad;
    if (SStrCmpI(name, "WaterRipples", STORM_MAX_STR) == 0) effect = Effect::WaterRipple;
    const bool ripple = effect == Effect::WaterRipple;
    const bool detail = effect == Effect::DetailDoodad;
    const bool shadow = effect == Effect::ShadowMap;
    const bool wmo = GxMetal::IsWmoEffect(effect);
    const bool liquid = GxMetal::IsLiquidEffect(effect);
    const bool terrain = effect == Effect::Terrain;
    const bool model = GxMetal::IsModelEffect(effect);
    const char* family = ShaderFamily(effect, target, vertexEffect, wmoVertex);
    if (SStrCmpI(name, family, STORM_MAX_STR) != 0) {
        throw std::runtime_error(std::string("Gx shader family has no native Metal implementation: ") + path + "\\" + name);
    }
    Require(SStrCmpI(name, family, STORM_MAX_STR) == 0 &&
        count == (ripple ? 1u : detail ? (target == GxSh_Vertex ? 6u : 8u) : liquid ? GxMetal::LiquidPermutationCount(effect, target) : terrain ? (target == GxSh_Vertex ? 128u : 32u) : (model || wmo || shadow) ? (target == GxSh_Vertex ? 90u : 16u) : effect == GxMetal::FragmentEffect::Unlit ? (target == GxSh_Vertex ? 2u : 1u) : effect == GxMetal::FragmentEffect::GlowWave ? 4u : 2u) &&
        (terrain || model || wmo || liquid || shadow || detail || ripple || effect == GxMetal::FragmentEffect::Unlit || target == GxSh_Pixel) &&
        SStrCmpI(path, target == GxSh_Vertex ? "Shaders\\Vertex" : "Shaders\\Pixel", STORM_MAX_STR) == 0,
        "Gx shader family has no verified native Metal implementation");
    bool cached = true;
    for (uint32_t i = 0; i < count; ++i) {
        const std::string key = count == 1 ? family : std::string(family) + ":" + std::to_string(i);
        cached = cached && m_shaderList[target].Ptr(key.c_str());
    }
    if (cached) {
        for (uint32_t i = 0; i < count; ++i) {
            const std::string key = count == 1 ? family : std::string(family) + ":" + std::to_string(i);
            shaders[i] = m_shaderList[target].Ptr(key.c_str()); ++shaders[i]->m_refCount;
        }
        return;
    }
    // Validate the mounted assets before adding any live shader objects.
    std::shared_ptr<GxMetal::UIShader> ui;
    if (ripple) { LoadWaterRippleShader(target); }
    else if (detail) { LoadDetailDoodadShader(target); }
    else if (shadow) { LoadShadowMapShader(target); }
    else if (liquid) { LoadLiquidShader(target, effect); }
    else if (wmo) { LoadWmoShader(target, wmoVertex, effect); }
    else if (terrain) { LoadTerrainShader(target); }
    else if (model) { LoadModelShader(target, effect, vertexEffect); }
    else if (effect == GxMetal::FragmentEffect::Unlit) { ui = LoadUI(); } else { LoadPostProcess(effect); }
    for (uint32_t i = 0; i < count; ++i) {
        const std::string key = count == 1 ? family : std::string(family) + ":" + std::to_string(i);
        CGxShader* shader = m_shaderList[target].Ptr(key.c_str());
        if (!shader) {
            shader = m_shaderList[target].New(key.c_str(), 0, 0);
            shader->m_target = target;
            auto entry = m->shaders.emplace(shader, Shader{ui, i, effect, vertexEffect, wmoVertex});
            shader->m_apiSpecific = &entry.first->second;
            shader->m_loaded = shader->m_valid = 1;
        }
        ++shader->m_refCount; shaders[i] = shader;
    }
}
void CGxDeviceMetal::ShaderDestroy(CGxShader*& shader) {
    if (!shader) { return; }
    Require(m->shaders.count(shader), "Foreign Metal Gx shader");
    if (shader->m_refCount == 1) {
        for (auto state : {GxRs_VertexShader, GxRs_PixelShader}) {
            if (m_appRenderStates[state].m_value.m_data.p == shader) { RsSet(state, nullptr); }
        }
        m->shaders.erase(shader); shader->m_apiSpecific = nullptr;
    }
    CGxDevice::ShaderDestroy(shader);
}
void CGxDeviceMetal::IShaderCreate(CGxShader* shader) { Require(m->shaders.count(shader), "Unknown native Gx shader"); shader->m_valid = shader->m_loaded = 1; }
void CGxDeviceMetal::IShaderReload(CGxShader* shader, const char* path, const char* name) {
    Require(shader && m->shaders.count(shader) && path && name && SStrCmpI(name, ShaderFamily(m->shaders.at(shader).effect, EGxShTarget(shader->m_target), m->shaders.at(shader).vertexEffect, m->shaders.at(shader).wmoVertex), STORM_MAX_STR) == 0 &&
        SStrCmpI(path, shader->m_target == GxSh_Vertex ? "Shaders\\Vertex" : "Shaders\\Pixel", STORM_MAX_STR) == 0, "Unsupported native shader reload");
    auto& native = m->shaders.at(shader);
    if (native.effect == GxMetal::FragmentEffect::WaterRipple) { LoadWaterRippleShader(EGxShTarget(shader->m_target)); }
    else if (native.effect == GxMetal::FragmentEffect::DetailDoodad) { LoadDetailDoodadShader(EGxShTarget(shader->m_target)); }
    else if (native.effect == GxMetal::FragmentEffect::ShadowMap) { LoadShadowMapShader(EGxShTarget(shader->m_target)); }
    else if (GxMetal::IsLiquidEffect(native.effect)) { LoadLiquidShader(EGxShTarget(shader->m_target), native.effect); }
    else if (GxMetal::IsWmoEffect(native.effect)) { LoadWmoShader(EGxShTarget(shader->m_target), native.wmoVertex, native.effect); }
    else if (native.effect == GxMetal::FragmentEffect::Terrain) { LoadTerrainShader(EGxShTarget(shader->m_target)); }
    else if (GxMetal::IsModelEffect(native.effect)) { LoadModelShader(EGxShTarget(shader->m_target), native.effect, native.vertexEffect); }
    else if (native.effect == GxMetal::FragmentEffect::Unlit) { native.ui = LoadUI(); } else { LoadPostProcess(native.effect); }
    IShaderCreate(shader);
}
void CGxDeviceMetal::ShaderConstantsSet(EGxShTarget target, uint32_t index, const float* values, uint32_t count) {
    Require((target == GxSh_Vertex || target == GxSh_Pixel) && values && index <= 256 && count <= 256 - index, "Invalid Gx shader constant range");
    CGxDevice::ShaderConstantsSet(target, index, values, count);
}

void CGxDeviceMetal::Draw(CGxBatch* batch, int32_t indexed) {
    Require(m->command && batch && batch->m_count, "Gx draw requires an active frame and nonempty batch");
    auto integer = [&](EGxRenderState state) { return m_appRenderStates[state].m_value.m_data.u[0]; };
    auto pointer = [&](EGxRenderState state) { return m_appRenderStates[state].m_value.m_data.p; };
    auto* vertex = static_cast<CGxShader*>(pointer(GxRs_VertexShader));
    auto* pixel = static_cast<CGxShader*>(pointer(GxRs_PixelShader));
    Require(pixel && m->shaders.count(pixel) && pixel->m_target == GxSh_Pixel, "Gx draw requires a verified native pixel shader");
    const auto effect = m->shaders.at(pixel).effect;
    const bool model = GxMetal::IsModelEffect(effect);
    const bool wmo = GxMetal::IsWmoEffect(effect);
    const bool liquid = GxMetal::IsLiquidEffect(effect);
    const bool terrain = effect == GxMetal::FragmentEffect::Terrain;
    const bool shadow = effect == GxMetal::FragmentEffect::ShadowMap;
    const bool ripple = effect == GxMetal::FragmentEffect::WaterRipple;
    const bool detail = effect == GxMetal::FragmentEffect::DetailDoodad;
    const bool postProcess = !ripple && !detail && !shadow && !model && !terrain && !wmo && !liquid && effect != GxMetal::FragmentEffect::Unlit;
    const bool filter = effect == GxMetal::FragmentEffect::Box4 || effect == GxMetal::FragmentEffect::Gauss4;
    const bool wave = effect == GxMetal::FragmentEffect::GlowWave;
    const unsigned inputs = shadow ? unsigned((m->shaders.at(pixel).permutation & 8) != 0) : liquid ? GxMetal::LiquidTextureCount(effect) : wmo ? GxMetal::WmoTextureCount(effect) : terrain ? GxMetal::TerrainLayerCount(GxSh_Pixel, m->shaders.at(pixel).permutation) + 1 : model ? GxMetal::ModelTextureCount(effect) : filter ? 4 : wave ? 3 : postProcess ? 2 : 1;
    Require(postProcess ? !vertex : vertex && m->shaders.count(vertex) && vertex->m_target == GxSh_Vertex &&
        ((liquid || shadow || detail || ripple) ? m->shaders.at(vertex).effect == effect : wmo ? m->shaders.at(vertex).effect == GxMetal::FragmentEffect::WmoDiffuse : terrain ? m->shaders.at(vertex).effect == GxMetal::FragmentEffect::Terrain : model ? m->shaders.at(vertex).effect == GxMetal::FragmentEffect::ModelOpaque : bool(m->shaders.at(vertex).ui)),
        "Unsupported native Gx vertex/pixel shader combination");
    if (model && (!GxMetal::SupportsModelPermutation(GxSh_Vertex, m->shaders.at(vertex).permutation, m->shaders.at(vertex).vertexEffect) ||
        !GxMetal::SupportsModelPermutation(GxSh_Pixel, m->shaders.at(pixel).permutation))) {
        throw std::runtime_error("Metal model vertex/shadow permutation is not implemented (vertex " +
            std::to_string(m->shaders.at(vertex).permutation) + ", pixel " + std::to_string(m->shaders.at(pixel).permutation) + ")");
    }
    if (detail) {
        Require(GxMetal::SupportsDetailDoodadPermutation(GxSh_Vertex, m->shaders.at(vertex).permutation) &&
            GxMetal::SupportsDetailDoodadPermutation(GxSh_Pixel, m->shaders.at(pixel).permutation), "Detail-doodad shadow receivers are not implemented");
    }
    if (wmo) {
        Require(GxMetal::SupportsWmoPermutation(GxSh_Vertex, m->shaders.at(vertex).permutation) &&
            GxMetal::SupportsWmoPermutation(GxSh_Pixel, m->shaders.at(pixel).permutation), "Metal WMO shadow permutation is not implemented");
    }
    if (terrain) {
        Require(GxMetal::SupportsTerrainPermutation(GxSh_Vertex, m->shaders.at(vertex).permutation) &&
            GxMetal::SupportsTerrainPermutation(GxSh_Pixel, m->shaders.at(pixel).permutation),
            "Metal terrain projected shadow or cube reflection permutation is not implemented");
    }
    CGxBuf* vertices = postProcess ? m_primVertexFormatBuf[GxVA_Position] : m_primVertexBuf;
    Require(vertices, "Gx draw requires vertices");
    if (postProcess) {
        const unsigned stride = 16 + inputs * 8;
        Require(m_primVertexFormat == ((filter || wave) ? GxVertexBufferFormats_Last : GxVBF_PCT2) &&
            m_primVertexMask == (0x11u | (((1u << inputs) - 1) << GxVA_TexCoord0)) && vertices->m_itemSize == stride &&
            !integer(GxRs_Lighting), "FFX requires the original unlit vertex stream");
        const CGxVertexAttrib expected[] = {{GxVA_Position, 4, 0, 48}, {GxVA_Color0, 0, 12, 48},
            {GxVA_TexCoord0, 3, 16, 48}, {GxVA_TexCoord1, 3, 24, 48}, {GxVA_TexCoord2, 3, 32, 48}, {GxVA_TexCoord3, 3, 40, 48}};
        for (unsigned i = 0; i < inputs + 2; ++i) {
            const auto& attr = expected[i];
            const auto& actual = m_primVertexFormatAttrib[attr.m_attrib];
            Require(m_primVertexFormatBuf[attr.m_attrib] == vertices && actual.m_type == attr.m_type &&
                actual.m_offset == attr.m_offset && actual.m_bufSize == stride, "Unsupported FFX vertex attribute layout");
        }
        for (unsigned i = 0; i < inputs; ++i) {
            Require(!integer(EGxRenderState(GxRs_TexGen0 + i)) && integer(EGxRenderState(GxRs_TextureCoord0 + i)) == i &&
                (m_xforms[i].m_flags[m_xforms[i].m_level] & CGxMatrixStack::F_Identity), "FFX texture transforms or generated coordinates are not implemented");
        }
    }
    // LiquidSystem retains Fog=1 with its SM3 program, which computes fog in
    // the shader itself. There is no additional fixed-function fog stage.
    Require((liquid || !(MasterEnable(GxMasterEnable_Fog) && integer(GxRs_Fog))) && !integer(GxRs_ClipPlaneMask) &&
        !integer(GxRs_PointSprite) && (ripple || m_appRenderStates[GxRs_PolygonOffset].m_value.m_data.f[0] == 0) && MasterEnable(GxMasterEnable_PolygonFill),
        "Gx fog, clip planes, point sprites, depth bias or wireframe path is not implemented");
    Require(postProcess || (m_primVertexFormat < GxVertexBufferFormats_Last && GxVertexSize(m_primVertexFormat) == vertices->m_itemSize),
        "Custom or split Gx vertex streams are not implemented");
    Require(!m_bufLocked[GxPoolTarget_Vertex] && !m_bufLocked[GxPoolTarget_Index], "Gx draw uses locked buffers");
    Require(indexed ? m_primIndexBuf && m_primIndexBuf->m_itemSize == 2 && batch->m_start <= m_primIndexBuf->m_itemCount &&
        batch->m_count <= m_primIndexBuf->m_itemCount - batch->m_start && batch->m_maxIndex < vertices->m_itemCount :
        batch->m_count <= vertices->m_itemCount, "Gx draw exceeds its logical buffer range");
    GxMetal::GeometryDraw draw;
    std::optional<GxMetal::ModelDrawParameters> modelParameters;
    std::optional<GxMetal::ShadowMapDrawParameters> shadowParameters;
    std::optional<GxMetal::DetailDoodadDrawParameters> detailParameters;
    std::optional<GxMetal::TerrainDrawParameters> terrainParameters;
    std::optional<GxMetal::WmoDrawParameters> wmoParameters;
    std::optional<GxMetal::LiquidDrawParameters> liquidParameters;
    const auto vertexBinding = m->Flush(vertices);
    draw.vertices = vertexBinding.buffer; draw.vertexOffset = vertexBinding.offset;
    if (indexed) {
        const auto indexBinding = m->Flush(m_primIndexBuf);
        draw.indices = indexBinding.buffer; draw.indexOffset = indexBinding.offset;
    }
    draw.format = m_primVertexFormat; draw.batch = *batch; draw.effect = effect;
    if (postProcess) {
        const auto& world = m_xforms[GxXform_World]; const auto& view = m_xforms[GxXform_View];
        draw.clipFromModel = world.m_mtx[world.m_level] * view.m_mtx[view.m_level] * m_projNative;
    } else if (ripple) {
        std::memcpy(&draw.clipFromModel, s_shadowConstants[1].constants, sizeof(C44Matrix));
        const float offset = m_appRenderStates[GxRs_PolygonOffset].m_value.m_data.f[0];
        Require(std::isfinite(offset), "Invalid water-ripple depth offset");
        draw.depthOffset = batch->m_primType >= GxPrim_Triangles ? -offset : 0;
    } else if (detail) {
        auto& native = detailParameters.emplace(); draw.detail = &native;
        native.pointLights = m->shaders.at(vertex).permutation == 3;
        const float* constants = s_shadowConstants[1].constants;
        auto vector = [&](unsigned index) { simd_float4 value; std::memcpy(&value, constants + index * 4, sizeof(value)); return value; };
        std::memcpy(&native.vertex.worldView, constants, sizeof(simd_float4x4));
        std::memcpy(&native.vertex.projection, constants + 16, sizeof(simd_float4x4));
        native.vertex.fog = vector(8); native.vertex.fade = vector(9); native.vertex.sunDirection = vector(10);
        native.vertex.ambient = vector(11); native.vertex.diffuse = vector(12);
        for (unsigned i=0;i<3;++i) native.vertex.lights[i] = {vector(14+i*3), vector(15+i*3), vector(16+i*3)};
        std::memcpy(&native.fragment.fogColor, s_shadowConstants[0].constants + 8, sizeof(simd_float4));
    } else if (shadow) {
        auto& native = shadowParameters.emplace(); draw.shadow = &native;
        native.skinning = GxMetal::ShadowMapSkinning(m->shaders.at(vertex).permutation);
        native.cutout = inputs != 0;
        const float* constants = s_shadowConstants[1].constants;
        auto vector = [&](unsigned index) { simd_float4 value; std::memcpy(&value, constants + index * 4, sizeof(value)); return value; };
        simd_float4x4 rows; std::memcpy(&rows, constants + 8, sizeof(rows)); native.vertex.projection = simd_transpose(rows);
        native.vertex.lightFromView = {vector(14), vector(15), vector(16)};
        native.vertex.uv[0] = vector(6); native.vertex.uv[1] = vector(7);
        native.fragment.depthScale = s_shadowConstants[0].constants[3];
        native.fragment.alphaRef = s_shadowConstants[0].constants[11];
        if (native.skinning) {
            Require(draw.format == GxVBF_PBNT2, "Shadow-map skinning requires the packed bone vertex layout");
            const auto& storage = m->Storage(vertices->m_pool);
            const auto offset = GxVertexAttribOffset(draw.format, GxVA_BlendIndices);
            const size_t first = indexed ? batch->m_minIndex : 0, last = indexed ? batch->m_maxIndex : batch->m_count - 1;
            for (size_t i = first; i <= last; ++i) {
                const uint8_t* indices = storage.shadow.data() + vertices->m_index + i * vertices->m_itemSize + offset;
                for (unsigned j = 0; j < (native.skinning == 2 ? 4u : 1u); ++j) {
                    Require(indices[j] < native.bones.size(), "Shadow-map bone index exceeds the native palette");
                    native.boneCount = std::max(native.boneCount, uint32_t(indices[j]) + 1);
                }
            }
        }
        std::memcpy(native.bones.data(), constants + 31 * 4, native.boneCount * sizeof(GxMetal::ModelAffine));
    } else if (liquid) {
        auto& native = liquidParameters.emplace(); draw.liquid = &native;
        native.lightCount = m->shaders.at(vertex).permutation;
        const float* constants = s_shadowConstants[1].constants;
        auto vector = [&](unsigned index) { simd_float4 value; std::memcpy(&value, constants + index * 4, sizeof(value)); return value; };
        std::memcpy(&native.vertex.projection, constants, sizeof(simd_float4x4));
        std::memcpy(&native.vertex.worldView, constants + 20, sizeof(simd_float4x4));
        for (unsigned i = 0; i < 2; ++i) native.vertex.textureTransform[i] = {vector(9 + i * 4), vector(10 + i * 4), vector(12 + i * 4)};
        native.vertex.fog = vector(4); native.vertex.sunDirection = vector(33); native.vertex.ambient = vector(34);
        native.vertex.diffuse = vector(35); native.vertex.specular = vector(36);
        for (unsigned i = 0; i < 3; ++i) native.vertex.lights[i] = {vector(37 + i * 3), vector(38 + i * 3), vector(39 + i * 3)};
        std::memcpy(&native.fragment.fogColor, s_shadowConstants[0].constants + 16, sizeof(simd_float4));
    } else if (wmo) {
        auto& native = wmoParameters.emplace(); draw.wmo = &native;
        native.permutation = m->shaders.at(vertex).permutation;
        native.vertexEffect = m->shaders.at(vertex).wmoVertex;
        native.shaderAlphaTest = (m->shaders.at(pixel).permutation & 8) != 0;
        const float* constants = s_shadowConstants[1].constants;
        auto vector = [&](unsigned index) { simd_float4 value; std::memcpy(&value, constants + index * 4, sizeof(value)); return value; };
        simd_float4x4 rows; std::memcpy(&rows, constants + 8, sizeof(rows)); native.vertex.projection = simd_transpose(rows);
        native.vertex.worldView = {vector(31), vector(32), vector(33)};
        native.vertex.ambient = vector(10); native.vertex.sunDiffuse = vector(11); native.vertex.sunDirection = vector(12);
        native.vertex.specular = vector(13); native.vertex.diffuse = vector(28); native.vertex.emissive = vector(29); native.vertex.fog = vector(30);
        for (unsigned i = 0; i < 4; ++i) { native.vertex.lightColor[i] = vector(17 + i); native.vertex.lightPosition[i] = vector(21 + i); }
        native.vertex.attenuationConstant = vector(25); native.vertex.attenuationLinear = vector(26); native.vertex.attenuationQuadratic = vector(27);
        std::memcpy(&native.fragment.fogColorAlphaRef, s_shadowConstants[0].constants + 8, sizeof(simd_float4));
    } else if (terrain) {
        auto& native = terrainParameters.emplace(); draw.terrain = &native;
        native.vertexPermutation = m->shaders.at(vertex).permutation;
        native.pixelPermutation = m->shaders.at(pixel).permutation;
        const float* constants = s_shadowConstants[1].constants;
        auto vector = [&](uint32_t index) { simd_float4 value; std::memcpy(&value, constants + index * 4, sizeof(value)); return value; };
        // Terrain uses multiply/add columns, unlike Model2's dot-product rows.
        std::memcpy(&native.vertex.worldView, constants, sizeof(simd_float4x4));
        std::memcpy(&native.vertex.projection, constants + 16, sizeof(simd_float4x4));
        native.vertex.fog = vector(12);
        for (unsigned i = 0; i < 5; ++i) { native.vertex.texOffset[i] = vector(13 + i); native.vertex.texScale[i] = vector(18 + i); }
        native.vertex.position = vector(23); native.vertex.sunDirection = vector(24);
        native.vertex.ambient = vector(25); native.vertex.diffuse = vector(26); native.vertex.specular = vector(27);
        for (unsigned i = 0; i < 3; ++i) native.vertex.lights[i] = {vector(28 + 3*i), vector(29 + 3*i), vector(30 + 3*i)};
        std::memcpy(&native.fragment.layerLighting, s_shadowConstants[0].constants + 4, sizeof(simd_float4));
        std::memcpy(&native.fragment.fogColor, s_shadowConstants[0].constants + 8, sizeof(simd_float4));
    } else if (model) {
        auto& native = modelParameters.emplace();
        draw.model = &native;
        native.permutation = m->shaders.at(vertex).permutation;
        native.vertexEffect = m->shaders.at(vertex).vertexEffect;
        native.shaderAlphaTest = (m->shaders.at(pixel).permutation & 8) != 0;
        const float* constants = s_shadowConstants[1].constants;
        auto vector = [&](uint32_t index) { simd_float4 value; std::memcpy(&value, constants + index * 4, sizeof(value)); return value; };
        // Gx stores dot-product rows. Metal uses column-vector matrices.
        simd_float4x4 rows;
        std::memcpy(&rows, constants + 8, sizeof(rows)); native.vertex.projection = simd_transpose(rows);
        for (unsigned i = 0; i < 4; ++i) native.vertex.uv[i] = vector(6 + i);
        native.vertex.sunDiffuse = vector(10); native.vertex.sunAmbient = vector(11); native.vertex.sunDirection = vector(12);
        for (unsigned i = 0; i < 4; ++i) { native.vertex.lightColor[i] = vector(17 + i); native.vertex.lightPosition[i] = vector(21 + i); }
        native.vertex.attenuationConstant = vector(25); native.vertex.attenuationLinear = vector(26); native.vertex.attenuationQuadratic = vector(27);
        native.vertex.diffuse = vector(28); native.vertex.emissive = vector(29); native.vertex.fog = vector(30);
        std::memcpy(&native.fragment.fogColorAlphaRef, s_shadowConstants[0].constants + 8, sizeof(simd_float4));
        if (native.permutation >= 10) {
            Require(draw.format == GxVBF_PBNT2, "Native model skinning requires the packed bone vertex layout");
            const auto& storage = m->Storage(vertices->m_pool);
            const auto offset = GxVertexAttribOffset(draw.format, GxVA_BlendIndices);
            const size_t first = indexed ? batch->m_minIndex : 0, last = indexed ? batch->m_maxIndex : batch->m_count - 1;
            for (size_t i = first; i <= last; ++i) {
                const uint8_t* indices = storage.shadow.data() + vertices->m_index + i * vertices->m_itemSize + offset;
                for (unsigned j = 0; j < (native.permutation >= 20 ? 4u : 1u); ++j) {
                    Require(indices[j] < native.bones.size(), "Model bone index exceeds the native palette");
                    native.boneCount = std::max(native.boneCount, uint32_t(indices[j]) + 1);
                }
            }
        }
        std::memcpy(native.bones.data(), constants + 31 * 4, native.boneCount * sizeof(GxMetal::ModelAffine));
    } else {
        const auto& shader = m->shaders.at(vertex);
        const auto transform = shader.ui->VertexTransform(s_shadowConstants[1].constants, 256, shader.permutation);
        draw.clipFromModel = transform.clipFromModel; draw.screenAtDepth = transform.screenAtDepth;
    }
    if (wave) {
        const float* constants = s_shadowConstants[0].constants;
        draw.waveSceneScale = {constants[0], constants[3]}; draw.waveBlurScale = {constants[4], constants[7]};
        draw.signedWave = m->shaders.at(pixel).permutation < 2;
    }
    draw.depthTest = MasterEnable(GxMasterEnable_DepthTest) && integer(GxRs_DepthTest);
    draw.depthWrite = MasterEnable(GxMasterEnable_DepthWrite) && integer(GxRs_DepthWrite);
    draw.depthFunction = integer(GxRs_DepthFunc);
    draw.culling = MasterEnable(GxMasterEnable_Culling) ? integer(GxRs_Culling) : 0;
    draw.viewportEnabled = true;
    draw.viewport = Viewport();
    m->scissor = Scissor(); m_scissorRectDirty = 0;
    IRsSync(0);
    if (!draw.viewport.width || !draw.viewport.height) { return; }
    draw.scissorEnabled = integer(GxRs_ScissorTest);
    if (draw.scissorEnabled) {
        draw.scissor = m->scissor;
        if (!draw.scissor.width || !draw.scissor.height) { return; }
    }
    GxMetal::TextureDrawState state;
    state.blend = EGxBlend(integer(GxRs_BlendingMode)); state.alphaRef = integer(GxRs_AlphaRef);
    state.colorWrite = MasterEnable(GxMasterEnable_ColorWrite) ? integer(GxRs_ColorWrite) : 0;
    state.blendFactor = m_appRenderStates[Unk84].m_value.m_data.f[0];
    std::array<id<MTLTexture>, 5> textures{};
    std::array<GxMetal::TextureDrawState, 5> states;
    for (unsigned i = 0; i < inputs; ++i) {
        auto* texture = static_cast<CGxTex*>(pointer(EGxRenderState(GxRs_Texture0 + i)));
        Require(texture && texture != m_textureTarget[GxBuffers_Color].m_texture, "Missing Gx shader texture or active target feedback");
        ITexMarkAsUpdated(texture);
        textures[i] = m->textures.at(texture).sampled ?: m->textures.at(texture).gpu;
        states[i] = state;
        states[i].decodeScale = m->gpu.DecodeScale(m->textures.at(texture).image.format);
        states[i].filter = EGxTexFilter(texture->m_flags.m_filter);
        states[i].wrapU = EGxTexWrapMode(texture->m_flags.m_wrapU); states[i].wrapV = EGxTexWrapMode(texture->m_flags.m_wrapV);
        states[i].anisotropy = texture->m_flags.m_maxAnisotropy;
        Require(texture->m_format != GxTex_R32F || states[i].filter == GxTex_Nearest || states[i].filter == GxTex_NearestMipNearest ||
            m->gpu.device.supports32BitFloatFiltering, "This GPU cannot filter R32F textures with the requested sampler");
    }
    const auto encoder = m->Render();
    if (ripple) { m->gpu.DrawWaterRipple(m->command, m->pass, textures[0], draw, states[0], encoder); }
    else if (detail) { m->gpu.DrawDetailDoodad(m->command, m->pass, textures[0], draw, states[0], encoder); }
    else if (shadow) { m->gpu.DrawShadowMap(m->command, m->pass, textures[0], draw, inputs ? states[0] : state, encoder); }
    else if (model) { m->gpu.DrawModel(m->command, m->pass, {textures[0], textures[1]}, draw, {states[0], states[1]}, encoder); }
    else if (liquid) { m->gpu.DrawLiquid(m->command, m->pass, {textures[0], textures[1]}, draw, {states[0], states[1]}, encoder); }
    else if (wmo) { m->gpu.DrawWmo(m->command, m->pass, {textures[0], textures[1]}, draw, {states[0], states[1]}, encoder); }
    else if (terrain) { m->gpu.DrawTerrain(m->command, m->pass, textures, draw, states, encoder); }
    else if (filter) { m->gpu.DrawFilter4(m->command, m->pass, {textures[0], textures[1], textures[2], textures[3]}, draw, {states[0], states[1], states[2], states[3]}, encoder); }
    else if (wave) { m->gpu.DrawGlowWave(m->command, m->pass, {textures[0], textures[1], textures[2]}, draw, {states[0], states[1], states[2]}, encoder); }
    else if (postProcess) { m->gpu.DrawGlow(m->command, m->pass, {textures[0], textures[1]}, draw, {states[0], states[1]}, encoder); }
    else { m->gpu.DrawUnlit(m->command, m->pass, textures[0], draw, {}, states[0], encoder); }
    ++m->statistics.geometryDraws;
}
void CGxDeviceMetal::XformSetProjection(const C44Matrix& matrix) {
    CGxDevice::XformSetProjection(matrix);
    // Preserve the D3D9 game's projection mapping into the shared [0,w] depth convention.
    C44Matrix native = matrix;
    if (!(std::fabs(native.c3 - 1) < 0.00000023841858f) && !(std::fabs(native.c3) < 0.00000023841858f)) { native *= 1.0f / native.c3; }
    if (native.d3 == 0) {
        const float near = -(native.d2 / (native.c2 + 1)), far = -(native.d2 / (native.c2 - 1));
        native.c2 = far / (far - near); native.d2 = far * near / (near - far);
    } else {
        const float inv = 1 / native.c2, near = (-1 - native.d2) * inv, far = (1 - native.d2) * inv;
        native.c2 = 1 / (far - near); native.d2 = near / (near - far);
    }
    if (!MasterEnable(GxMasterEnable_NormalProjection) && native.d3 != 1) {
        C44Matrix scale; scale.a0 = scale.b1 = scale.c2 = 0.2f; native = native * scale;
    }
    m_projNative = native; m_xforms[GxXform_Projection].m_dirty = 1;
}

// Fail at the actual call site rather than inherit the base class's empty operations.
void CGxDeviceMetal::DeviceReadPixels(CiRect& rect, TSGrowableArray<CImVector>& pixels) {
    Require(m->command && m->pass.colorAttachments[0].texture.pixelFormat == MTLPixelFormatBGRA8Unorm, "Pixel readback requires a BGRA8 Gx frame");
    ClampRectToWindow(rect);
    const size_t width = std::max<int64_t>(0, int64_t(rect.maxX) - rect.minX), height = std::max<int64_t>(0, int64_t(rect.maxY) - rect.minY);
    Require(!height || width <= ResourceBudget / 4 / height, "Pixel readback exceeds memory budget");
    pixels.SetCount(uint32_t(width * height));
    if (!width || !height) { return; }
    // The shared API uses top-left rectangles (also honored by its OpenGL
    // implementation). D3D9's reconstructed readback ignores nonzero origins;
    // current game callers request origin zero. That discrepancy still needs
    // original Windows verification, not an assumed visual-equivalence claim.
    m->ReadSurface(m->pass.colorAttachments[0].texture, rect, pixels.Ptr());
}
void CGxDeviceMetal::ReadFloatColor(CiRect& rect, TSGrowableArray<float>& pixels) {
    Require(m->command && m->pass.colorAttachments[0].texture.pixelFormat == MTLPixelFormatR32Float, "Float color readback requires an R32F attachment");
    ClampRectToWindow(rect);
    const size_t width = std::max<int64_t>(0, int64_t(rect.maxX) - rect.minX), height = std::max<int64_t>(0, int64_t(rect.maxY) - rect.minY);
    Require(!height || width <= ResourceBudget / 4 / height, "Float color readback exceeds memory budget");
    pixels.SetCount(uint32_t(width * height));
    if (width && height) m->ReadSurface(m->pass.colorAttachments[0].texture, rect, pixels.Ptr());
}
void CGxDeviceMetal::DeviceReadDepths(CiRect& rect, TSGrowableArray<float>& depths) {
    Require(m->command && m->pass.depthAttachment.texture, "Depth readback requires a Gx depth attachment");
    ClampRectToWindow(rect);
    const size_t width = std::max<int64_t>(0, int64_t(rect.maxX) - rect.minX), height = std::max<int64_t>(0, int64_t(rect.maxY) - rect.minY);
    Require(!height || width <= ResourceBudget / 4 / height, "Depth readback exceeds memory budget");
    depths.SetCount(uint32_t(width * height));
    if (!width || !height) { return; }
    // Return actual native Depth32Float samples. The reconstructed D3D9 path
    // fills zero and has no game call sites; it is not a depth reference.
    m->ReadSurface(m->pass.depthAttachment.texture, rect, depths.Ptr());
}
void CGxDeviceMetal::DeviceTakeScreenShot() { CGxDevice::DeviceTakeScreenShot(); }
void CGxDeviceMetal::ApplyRenderTarget() {
    auto* texture = m_textureTarget[GxBuffers_Color].m_texture;
    if (!texture) {
        m_curWindowRect = m_defWindowRect;
        if (m->command) { m->pass = m->defaultPass; }
        return;
    }
    ITexMarkAsUpdated(texture);
    id<MTLTexture> color = m->textures.at(texture).gpu;
    // D3D9 reuses a scratch depth surface while it is large enough. Preserve
    // that lifetime, including depth values when switching to a smaller target.
    if (!m->targetDepth || m->targetDepth.width < color.width || m->targetDepth.height < color.height) {
        auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:color.width height:color.height mipmapped:NO];
        descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
        id<MTLTexture> replacement = [m->gpu.device newTextureWithDescriptor:descriptor];
        Require(replacement != nil, "Cannot allocate Gx scratch depth surface");
        m->targetDepth = replacement;
    }
    m_curWindowRect.minX = m_curWindowRect.minY = 0;
    m_curWindowRect.maxX = color.width; m_curWindowRect.maxY = color.height;
    if (m->command) {
        m->pass = [MTLRenderPassDescriptor renderPassDescriptor];
        m->pass.colorAttachments[0].texture = color;
        m->pass.colorAttachments[0].loadAction = MTLLoadActionLoad;
        m->pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        m->pass.depthAttachment.texture = m->targetDepth;
        m->pass.depthAttachment.loadAction = MTLLoadActionLoad;
        m->pass.depthAttachment.storeAction = MTLStoreActionStore;
        m->pass.renderTargetWidth = color.width; m->pass.renderTargetHeight = color.height;
    }
}
void CGxDeviceMetal::DeviceSetRenderTarget(EGxBuffer buffer, CGxTex* texture, uint32_t plane) {
    if (!m_context) { return; }
    Require((buffer == GxBuffers_Color || buffer == GxBuffers_Depth) && plane == 0, "Unsupported Gx target buffer or plane");
    Require(buffer == GxBuffers_Color || !texture, "Gx explicit depth texture targets are not implemented");
    Require(!texture || (texture->m_flags.m_renderTarget && (texture->m_format == GxTex_Argb8888 || texture->m_format == GxTex_R32F)), "Invalid Gx color render target");
    auto& target = m_textureTarget[buffer];
    if (target.m_texture == texture && target.m_plane == plane) { return; }
    if (texture) { ITexMarkAsUpdated(texture); }
    if (m->command) { m->EnsurePass(); m->EndRender(); }
    CGxDevice::DeviceSetRenderTarget(buffer, texture, plane);
    ApplyRenderTarget();
    m->scissor = {0, 0, size_t(m_curWindowRect.maxX), size_t(m_curWindowRect.maxY)};
    m_viewportDirty = m_scissorRectDirty = 1;
}
void CGxDeviceMetal::DeviceSaveDepthSurface() { Unsupported("save depth surface"); }
void CGxDeviceMetal::DeviceRestoreDepthSurface() { Unsupported("restore depth surface"); }
void CGxDeviceMetal::DeviceResolveDepthBuffer(CGxTex*) { Unsupported("resolve depth"); }
void CGxDeviceMetal::DeviceCopyTex(CGxTex* source, uint32_t sourcePlane, CGxTex* dest, uint32_t destPlane) {
    if (!m_context || !source || !dest) { return; }
    Require(sourcePlane == 0 && destPlane == 0 && dest->m_flags.m_renderTarget && (source->m_format == GxTex_Argb8888 || source->m_format == GxTex_R32F) &&
        dest->m_format == source->m_format, "Unsupported Gx texture copy format, target or plane");
    ITexMarkAsUpdated(source); ITexMarkAsUpdated(dest);
    id<MTLTexture> src = m->textures.at(source).gpu, dst = m->textures.at(dest).gpu;
    Require(m->CopyColor(src, SurfaceRect(src, 0), dst, 0, SurfaceRect(dst, 0), false), "Invalid Gx texture copy");
}
int32_t CGxDeviceMetal::TexCopy(CGxTex* dest, const C2iVector& destPos, const C2iVector& sourcePos, const C2iVector& size, uint32_t level, uint32_t plane) {
    if (!m_context) { return 0; }
    Require(m->command && dest && dest->m_flags.m_renderTarget && plane == 0, "Unsupported Gx framebuffer copy target, plane or lifecycle");
    ITexMarkAsUpdated(dest);
    id<MTLTexture> src = m->defaultPass.colorAttachments[0].texture, dst = m->textures.at(dest).gpu;
    if (level >= dst.mipmapLevelCount || size.x <= 0 || size.y <= 0 || sourcePos.x < 0 || sourcePos.y < 0 || destPos.x < 0 || destPos.y < 0) { return 0; }
    const auto extent = SurfaceRect(dst, level);
    if (int64_t(sourcePos.x) + size.x > int64_t(src.width) || int64_t(sourcePos.y) + size.y > int64_t(src.height) ||
        int64_t(destPos.x) + size.x > extent.maxX || int64_t(destPos.y) + size.y > extent.maxY) { return 0; }
    return m->CopyColor(src, {sourcePos.y, sourcePos.x, sourcePos.y + size.y, sourcePos.x + size.x}, dst, level,
        {destPos.y, destPos.x, destPos.y + size.y, destPos.x + size.x}, false);
}
int32_t CGxDeviceMetal::TexStretch(CGxTex* dest, const CiRect* destRect, const CiRect* sourceRect, uint32_t level, uint32_t face) {
    if (!m_context) { return 0; }
    Require(m->command && dest && dest->m_flags.m_renderTarget && face == 0, "Unsupported Gx stretch target, face or lifecycle");
    ITexMarkAsUpdated(dest);
    id<MTLTexture> src = m->defaultPass.colorAttachments[0].texture, dst = m->textures.at(dest).gpu;
    if (level >= dst.mipmapLevelCount) { return 0; }
    const CiRect s = sourceRect ? *sourceRect : SurfaceRect(src, 0), d = destRect ? *destRect : SurfaceRect(dst, level);
    const bool linear = !sourceRect || !destRect || int64_t(s.maxX) - s.minX != int64_t(d.maxX) - d.minX ||
        int64_t(s.maxY) - s.minY != int64_t(d.maxY) - d.minY;
    return m->CopyColor(src, s, dst, level, d, linear);
}
bool CGxDeviceMetal::QueryBegin(CGxQuery*) { Unsupported("query begin"); }
void CGxDeviceMetal::QueryCreate(CGxQuery*& query, EGxQueryType) { query = nullptr; Unsupported("query creation"); }
bool CGxDeviceMetal::QueryEnd(CGxQuery*) { Unsupported("query end"); }
bool CGxDeviceMetal::QueryGetParam(CGxQuery*, EGxQueryParam, uint32_t&) { Unsupported("query parameter"); }
bool CGxDeviceMetal::QueryGetData(CGxQuery*, uint32_t&) { Unsupported("query readback"); }
void CGxDeviceMetal::StereoSetConvergence(float) { Unsupported("stereo convergence"); }
float CGxDeviceMetal::StereoGetConvergence() { Unsupported("stereo convergence"); }
void CGxDeviceMetal::StereoSetSeparation(float) { Unsupported("stereo separation"); }
float CGxDeviceMetal::StereoGetSeparation() { Unsupported("stereo separation"); }
void CGxDeviceMetal::PrimPointSize(float size) { Require(size == 1, "Metal Gx point size is not implemented"); }
void CGxDeviceMetal::PrimLineWidth(float width) { Require(width == 1, "Metal Gx wide lines are not implemented"); }
void CGxDeviceMetal::DeviceSetGamma(float gamma) { Require(gamma == 1, "Metal Gx gamma ramp is not implemented"); CGxDevice::DeviceSetGamma(gamma); }
void CGxDeviceMetal::DeviceSetGamma(const CGxGammaRamp& ramp) {
    CGxGammaRamp identity; identity.Set(1);
    Require(std::memcmp(&identity, &ramp, sizeof(ramp)) == 0, "Metal Gx gamma ramp is not implemented"); CGxDevice::DeviceSetGamma(ramp);
}
CGxDevice* CreateCGxDeviceMetal() { return new CGxDeviceMetal(MTLCreateSystemDefaultDevice()); }
