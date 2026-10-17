#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Metal/UIShader.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Storm/H/storm.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
void Require(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
[[noreturn]] void Unsupported(const char* operation) { throw std::runtime_error(std::string("Metal Gx operation not implemented: ") + operation); }
constexpr size_t ResourceBudget = 256 * 1024 * 1024;
struct Update { id<MTLBuffer> staging; size_t offset; };
struct Pool {
    std::vector<uint8_t> shadow;
    id<MTLBuffer> gpu;
    std::vector<Update> updates;
};
struct Texture { BLP::Image image; id<MTLTexture> gpu; uint32_t base = 0; };
struct Shader { std::shared_ptr<GxMetal::UIShader> ui; uint32_t permutation; };
std::vector<uint8_t> ReadShader(const char* path) {
    SFileImpl* file = nullptr;
    Require(SFileOpenFile(path, &file) && file, "Cannot open required UI shader from mounted Data");
    struct Close { SFileImpl* file; ~Close() { SFileCloseFile(file); } } close{file};
    uint32_t high = 0;
    const uint32_t size = SFileGetFileSize(file, &high);
    Require(!high && size && size <= 1024 * 1024, "Invalid UI shader file size");
    std::vector<uint8_t> data(size);
    uint32_t read = 0;
    Require(SFileReadFile(file, data.data(), size, &read, nullptr, 0) && read == size, "Incomplete UI shader file");
    return data;
}
std::shared_ptr<GxMetal::UIShader> LoadUI() {
    return std::make_shared<GxMetal::UIShader>(GxMetal::UIShader::FromBls(
        ReadShader("Shaders\\Vertex\\vs_3_0\\UI.bls"), ReadShader("Shaders\\Pixel\\ps_3_0\\UI.bls")));
}
BLP::Format ImageFormat(EGxTexFormat format) {
    switch (format) {
        case GxTex_Argb8888: return BLP::Format::BGRA8;
        case GxTex_Dxt1: return BLP::Format::BC1;
        case GxTex_Dxt3: return BLP::Format::BC2;
        case GxTex_Dxt5: return BLP::Format::BC3;
        default: Unsupported("texture format conversion");
    }
}
}

struct CGxDeviceMetal::Impl {
    explicit Impl(id<MTLDevice> device) : gpu(device) {}
    GxMetal::TextureDevice gpu;
    std::unordered_map<CGxPool*, Pool> pools;
    std::unordered_map<CGxTex*, Texture> textures;
    std::unordered_map<CGxShader*, Shader> shaders;
    id<MTLCommandBuffer> command;
    MTLRenderPassDescriptor* pass;
    id<MTLDrawable> drawable;
    void* window = nullptr;
    bool passUsed = false;
    dispatch_semaphore_t frames = dispatch_semaphore_create(3);
    void EnsurePass() {
        if (!passUsed) {
            id<MTLRenderCommandEncoder> encoder = [command renderCommandEncoderWithDescriptor:pass];
            Require(encoder != nil, "Cannot encode Gx attachment load/clear");
            [encoder endEncoding]; ConsumedPass();
        }
    }
    void ConsumedPass() {
        passUsed = true;
        pass.colorAttachments[0].loadAction = MTLLoadActionLoad;
        pass.depthAttachment.loadAction = MTLLoadActionLoad;
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
            storage.gpu = [gpu.device newBufferWithLength:size options:MTLResourceStorageModePrivate];
            Require(storage.gpu != nil, "Cannot allocate private Gx buffer pool");
            storage.gpu.label = @"Gx buffer pool";
            storage.updates.clear();
            Queue(storage, 0, size);
        }
        pool->m_mem = storage.shadow.data();
        pool->m_apiSpecific = &storage;
        return storage;
    }
    id<MTLBuffer> Flush(CGxBuf* buffer) {
        Require(buffer && buffer->m_valid && buffer->m_hasData, "Gx draw uses an uninitialized buffer");
        auto& storage = Storage(buffer->m_pool);
        if (!storage.updates.empty()) {
            id<MTLBlitCommandEncoder> encoder = [command blitCommandEncoder];
            Require(encoder != nil, "Cannot encode Gx buffer uploads");
            for (const auto& update : storage.updates) {
                [encoder copyFromBuffer:update.staging sourceOffset:0 toBuffer:storage.gpu destinationOffset:update.offset size:update.staging.length];
            }
            [encoder endEncoding];
            storage.updates.clear();
        }
        return storage.gpu;
    }
};

CGxDeviceMetal::CGxDeviceMetal(id<MTLDevice> device) : m(std::make_unique<Impl>(device)) {
    m_api = GxApi_Metal;
    m_caps.m_numTmus = 1;
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
    m_caps.m_texTarget[GxTex_2d] = m_caps.m_texTarget[GxTex_NonPow2] = 1;
    m_caps.m_texMaxSize[GxTex_2d] = m_caps.m_texMaxSize[GxTex_NonPow2] = 16384;
    m_caps.m_texNonPow2 = 1;
    // Select the original 3.0 assets, without falling back to a lower profile.
    // ShaderCreate still rejects every family lacking a verified native effect.
    m_caps.m_shaderTargets[GxSh_Vertex] = 3;
    m_caps.m_shaderTargets[GxSh_Pixel] = 4;
    m_caps.m_shaderConstants[GxSh_Vertex] = m_caps.m_shaderConstants[GxSh_Pixel] = 256;
}
CGxDeviceMetal::~CGxDeviceMetal() {
    if (m->command) {
        // Preserve already encoded uploads even when a caller unwinds a failed frame.
        dispatch_semaphore_t frames = m->frames;
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
    Require(!pass.depthAttachment.texture || pass.depthAttachment.texture.pixelFormat == MTLPixelFormatDepth32Float, "Unsupported Metal frame depth");
    dispatch_semaphore_wait(m->frames, DISPATCH_TIME_FOREVER);
    m->command = [m->gpu.queue commandBuffer];
    if (!m->command) { dispatch_semaphore_signal(m->frames); throw std::runtime_error("Cannot allocate Metal frame command"); }
    m->command.label = @"Gx game frame";
    m->pass = [pass copy]; m->drawable = drawable; m->passUsed = false;
    m->pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    m->pass.depthAttachment.storeAction = MTLStoreActionStore;
    m_curWindowRect.maxX = pass.colorAttachments[0].texture.width;
    m_curWindowRect.maxY = pass.colorAttachments[0].texture.height;
}
id<MTLCommandBuffer> CGxDeviceMetal::FrameCommand() const { return m->command; }
void CGxDeviceMetal::ScenePresent() {
    Require(m->command != nil, "No active Metal frame to present");
    if (m_scrShotClick) { DeviceScreenShot(); }
    m->EnsurePass();
    dispatch_semaphore_t frames = m->frames;
    [m->command addCompletedHandler:^(id<MTLCommandBuffer> command) {
        if (command.error) { CGxDevice::Log("Metal frame failed: %s", command.error.localizedDescription.UTF8String); }
        dispatch_semaphore_signal(frames);
    }];
    if (m->drawable) { [m->command presentDrawable:m->drawable]; }
    [m->command commit]; m->command = nil; m->drawable = nil;
    CGxDevice::ScenePresent();
}
void CGxDeviceMetal::SceneClear(uint32_t mask, CImVector color) {
    Require(m->command && (mask & ~3u) == 0, "Invalid Metal Gx clear");
    Require(m_viewport.m_x.m_l == 0 && m_viewport.m_x.m_h == 1 && m_viewport.m_y.m_l == 0 && m_viewport.m_y.m_h == 1,
        "Partial-viewport Gx clear is not implemented");
    if (mask & 1) { m->pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        m->pass.colorAttachments[0].clearColor = MTLClearColorMake(color.r / 255.0, color.g / 255.0, color.b / 255.0, color.a / 255.0); }
    if (mask & 2) { Require(m->pass.depthAttachment.texture != nil, "No depth surface to clear");
        m->pass.depthAttachment.loadAction = MTLLoadActionClear; m->pass.depthAttachment.clearDepth = 1; }
    [[m->command renderCommandEncoderWithDescriptor:m->pass] endEncoding]; m->ConsumedPass();
}
void CGxDeviceMetal::IRsSendToHw(EGxRenderState) { /* Pipeline/state snapshots are bound at Draw. */ }

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
    Require(format == dataFormat && callback && !flags.m_generateMipMaps && !flags.m_renderTarget, "Unsupported Gx texture conversion, generation or render target");
    ImageFormat(format);
    return CGxDevice::TexCreate(target, width, height, depth, format, dataFormat, flags, arg, callback, name, texture);
}
void CGxDeviceMetal::TexDestroy(CGxTex* texture) {
    if (!texture) { return; }
    for (int state = GxRs_Texture0; state <= GxRs_Texture15; ++state) {
        if (m_appRenderStates[state].m_value.m_data.p == texture) { RsSet(EGxRenderState(state), nullptr); }
    }
    m->textures.erase(texture); texture->m_apiSpecificData = nullptr;
    CGxDevice::TexDestroy(texture);
}
void CGxDeviceMetal::DeviceSetBaseMipLevel(uint32_t level) {
    if (level != m_baseMipLevel) {
        for (auto* texture = m_textures.Head(); texture; texture = m_textures.Next(texture)) { texture->m_needsCreation = 1; }
    }
    CGxDevice::DeviceSetBaseMipLevel(level);
}
void CGxDeviceMetal::ITexMarkAsUpdated(CGxTex* texture) {
    Require(texture != nullptr, "Missing Gx texture");
    uint32_t width, height, base, end;
    ITexWHDStartEnd(texture, width, height, base, end);
    width = std::max(width, 1u); height = std::max(height, 1u);
    auto& storage = m->textures[texture];
    const bool recreate = texture->m_needsCreation || !storage.gpu || storage.base != base || storage.image.mips.size() != end - base;
    if (!recreate && !texture->m_needsUpdate) { texture->m_needsFlagUpdate = 0; return; }
    Require(texture->m_userFunc && !texture->m_flags.m_generateMipMaps && !texture->m_flags.m_renderTarget && texture->m_format == texture->m_dataFormat,
        "Unsupported updated Gx texture flags or conversion");
    BLP::Image image = recreate ? BLP::Image{} : storage.image;
    image.format = ImageFormat(texture->m_dataFormat);
    const uint32_t block = image.format == BLP::Format::BGRA8 ? 1 : 4;
    const uint32_t bytes = block == 1 ? 4 : image.format == BLP::Format::BC1 ? 8 : 16;
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
        Require(pixels && right > left && bottom > top && stride >= (texture->m_flags.m_bit15 ? right - left : right) * bytes,
            "Missing or invalid Gx texture mip callback data");
        Require(!recreate || !texture->m_flags.m_bit15 || (texture->m_updateRect.minX == 0 && texture->m_updateRect.minY == 0 &&
            texture->m_updateRect.maxX == int32_t(texture->m_width) && texture->m_updateRect.maxY == int32_t(texture->m_height)),
            "Cannot recreate a Gx texture from partial-origin callback data");
        auto* source = static_cast<const uint8_t*>(pixels);
        if (!texture->m_flags.m_bit15) { source += size_t(top) * stride + left * bytes; }
        for (uint32_t row = top; row < bottom; ++row) {
            std::memcpy(dest.bytes.data() + size_t(row) * rowBytes + left * bytes, source + size_t(row - top) * stride, (right - left) * bytes);
        }
        width = std::max(1u, width >> 1); height = std::max(1u, height >> 1);
    }
    // Immutable replacement keeps previously encoded frames' texture contents alive.
    storage.gpu = m->gpu.Upload(image); storage.image = std::move(image); storage.base = base;
    texture->m_apiSpecificData = &storage;
    texture->m_needsCreation = texture->m_needsFlagUpdate = 0;
    CGxDevice::ITexMarkAsUpdated(texture);
}

void CGxDeviceMetal::ShaderCreate(CGxShader** shaders, EGxShTarget target, const char* path, const char* name, uint32_t count) {
    Require(shaders && path && name && (target == GxSh_Vertex || target == GxSh_Pixel), "Invalid Metal Gx shader request");
    Require(SStrCmpI(name, "UI", STORM_MAX_STR) == 0 && count == (target == GxSh_Vertex ? 2u : 1u) &&
        SStrCmpI(path, target == GxSh_Vertex ? "Shaders\\Vertex" : "Shaders\\Pixel", STORM_MAX_STR) == 0,
        "Gx shader family has no verified native Metal implementation");
    bool cached = true;
    for (uint32_t i = 0; i < count; ++i) {
        const std::string key = count == 1 ? "UI" : "UI:" + std::to_string(i);
        cached = cached && m_shaderList[target].Ptr(key.c_str());
    }
    if (cached) {
        for (uint32_t i = 0; i < count; ++i) {
            const std::string key = count == 1 ? "UI" : "UI:" + std::to_string(i);
            shaders[i] = m_shaderList[target].Ptr(key.c_str()); ++shaders[i]->m_refCount;
        }
        return;
    }
    // Validate the mounted assets before adding any live shader objects.
    auto ui = LoadUI();
    for (uint32_t i = 0; i < count; ++i) {
        const std::string key = count == 1 ? "UI" : "UI:" + std::to_string(i);
        CGxShader* shader = m_shaderList[target].Ptr(key.c_str());
        if (!shader) {
            shader = m_shaderList[target].New(key.c_str(), 0, 0);
            shader->m_target = target;
            auto entry = m->shaders.emplace(shader, Shader{ui, i});
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
    Require(shader && m->shaders.count(shader) && path && name && SStrCmpI(name, "UI", STORM_MAX_STR) == 0 &&
        SStrCmpI(path, shader->m_target == GxSh_Vertex ? "Shaders\\Vertex" : "Shaders\\Pixel", STORM_MAX_STR) == 0, "Unsupported native shader reload");
    m->shaders.at(shader).ui = LoadUI(); IShaderCreate(shader);
}
void CGxDeviceMetal::ShaderConstantsSet(EGxShTarget target, uint32_t index, const float* values, uint32_t count) {
    Require((target == GxSh_Vertex || target == GxSh_Pixel) && values && index <= 256 && count <= 256 - index, "Invalid Gx shader constant range");
    CGxDevice::ShaderConstantsSet(target, index, values, count);
}

void CGxDeviceMetal::Draw(CGxBatch* batch, int32_t indexed) {
    Require(m->command && batch && m_primVertexBuf, "Gx draw requires an active frame and vertices");
    auto integer = [&](EGxRenderState state) { return m_appRenderStates[state].m_value.m_data.u[0]; };
    auto pointer = [&](EGxRenderState state) { return m_appRenderStates[state].m_value.m_data.p; };
    auto* vertex = static_cast<CGxShader*>(pointer(GxRs_VertexShader));
    auto* pixel = static_cast<CGxShader*>(pointer(GxRs_PixelShader));
    Require(vertex && pixel && m->shaders.count(vertex) && m->shaders.count(pixel) && vertex->m_target == GxSh_Vertex && pixel->m_target == GxSh_Pixel,
        "Gx draw requires verified native UI vertex and pixel shaders; fixed function is pending");
    Require(!(MasterEnable(GxMasterEnable_Fog) && integer(GxRs_Fog)) && !integer(GxRs_ClipPlaneMask) &&
        !integer(GxRs_PointSprite) && m_appRenderStates[GxRs_PolygonOffset].m_value.m_data.f[0] == 0 && MasterEnable(GxMasterEnable_PolygonFill),
        "Gx fog, clip planes, point sprites, depth bias or wireframe path is not implemented");
    Require(m_primVertexFormat < GxVertexBufferFormats_Last && GxVertexSize(m_primVertexFormat) == m_primVertexBuf->m_itemSize,
        "Custom or split Gx vertex streams are not implemented");
    Require(!m_bufLocked[GxPoolTarget_Vertex] && !m_bufLocked[GxPoolTarget_Index], "Gx draw uses locked buffers");
    Require(indexed ? m_primIndexBuf && m_primIndexBuf->m_itemSize == 2 && batch->m_start <= m_primIndexBuf->m_itemCount &&
        batch->m_count <= m_primIndexBuf->m_itemCount - batch->m_start && batch->m_maxIndex < m_primVertexBuf->m_itemCount :
        batch->m_count <= m_primVertexBuf->m_itemCount, "Gx draw exceeds its logical buffer range");
    auto* texture = static_cast<CGxTex*>(pointer(GxRs_Texture0));
    Require(texture != nullptr, "Native UI draw requires texture 0");
    ITexMarkAsUpdated(texture);
    GxMetal::GeometryDraw draw;
    draw.vertices = m->Flush(m_primVertexBuf); draw.vertexOffset = m_primVertexBuf->m_index;
    if (indexed) { draw.indices = m->Flush(m_primIndexBuf); draw.indexOffset = m_primIndexBuf->m_index; }
    draw.format = m_primVertexFormat; draw.batch = *batch;
    const auto& shader = m->shaders.at(vertex);
    const auto transform = shader.ui->VertexTransform(s_shadowConstants[1].constants, 256, shader.permutation);
    draw.clipFromModel = transform.clipFromModel; draw.screenAtDepth = transform.screenAtDepth;
    draw.depthTest = MasterEnable(GxMasterEnable_DepthTest) && integer(GxRs_DepthTest);
    draw.depthWrite = MasterEnable(GxMasterEnable_DepthWrite) && integer(GxRs_DepthWrite);
    draw.depthFunction = integer(GxRs_DepthFunc);
    draw.culling = MasterEnable(GxMasterEnable_Culling) ? integer(GxRs_Culling) : 0;
    const double width = m_curWindowRect.maxX, height = m_curWindowRect.maxY;
    auto rounded = [](double v) { Require(std::isfinite(v) && v >= 0, "Invalid Gx viewport"); return std::floor(v + 0.5); };
    draw.viewportEnabled = true;
    draw.viewport.originX = rounded(m_viewport.m_x.m_l * width);
    draw.viewport.originY = rounded((1 - m_viewport.m_y.m_h) * height);
    draw.viewport.width = rounded(m_viewport.m_x.m_h * width - draw.viewport.originX);
    draw.viewport.height = rounded((1 - m_viewport.m_y.m_l) * height - draw.viewport.originY);
    draw.viewport.znear = m_viewport.m_z.m_l; draw.viewport.zfar = m_viewport.m_z.m_h;
    if (!draw.viewport.width || !draw.viewport.height) { return; }
    draw.scissorEnabled = integer(GxRs_ScissorTest);
    if (draw.scissorEnabled) {
        auto edge = [](double value, double limit) { Require(std::isfinite(value), "Invalid Gx scissor"); return size_t(std::clamp(value, 0.0, limit)); };
        const size_t left = edge(m_scissorRect.minX * width + 0.5, width), right = edge(m_scissorRect.maxX * width + 1, width);
        const size_t top = edge((1 - m_scissorRect.maxY) * height + 0.5, height), bottom = edge((1 - m_scissorRect.minY) * height + 1, height);
        if (right <= left || bottom <= top) { return; }
        draw.scissor = {left, top, right - left, bottom - top};
    }
    GxMetal::TextureDrawState state;
    state.blend = EGxBlend(integer(GxRs_BlendingMode)); state.alphaRef = integer(GxRs_AlphaRef);
    state.colorWrite = MasterEnable(GxMasterEnable_ColorWrite) ? integer(GxRs_ColorWrite) : 0;
    state.blendFactor = m_appRenderStates[Unk84].m_value.m_data.f[0];
    state.filter = EGxTexFilter(texture->m_flags.m_filter);
    state.wrapU = EGxTexWrapMode(texture->m_flags.m_wrapU); state.wrapV = EGxTexWrapMode(texture->m_flags.m_wrapV);
    state.anisotropy = texture->m_flags.m_maxAnisotropy;
    IRsSync(0);
    m->gpu.DrawUnlit(m->command, m->pass, m->textures.at(texture).gpu, draw, {}, state);
    m->ConsumedPass();
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
    Require(m->command != nil, "Pixel readback requires an active Gx frame");
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
void CGxDeviceMetal::DeviceSetRenderTarget(EGxBuffer, CGxTex*, uint32_t) { Unsupported("game render target binding"); }
void CGxDeviceMetal::DeviceSaveDepthSurface() { Unsupported("save depth surface"); }
void CGxDeviceMetal::DeviceRestoreDepthSurface() { Unsupported("restore depth surface"); }
void CGxDeviceMetal::DeviceResolveDepthBuffer(CGxTex*) { Unsupported("resolve depth"); }
void CGxDeviceMetal::DeviceCopyTex(CGxTex*, uint32_t, CGxTex*, uint32_t) { Unsupported("texture copy"); }
int32_t CGxDeviceMetal::TexCopy(CGxTex*, const C2iVector&, const C2iVector&, const C2iVector&, uint32_t, uint32_t) { Unsupported("framebuffer copy"); }
int32_t CGxDeviceMetal::TexStretch(CGxTex*, const CiRect*, const CiRect*, uint32_t, uint32_t) { Unsupported("texture stretch"); }
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
