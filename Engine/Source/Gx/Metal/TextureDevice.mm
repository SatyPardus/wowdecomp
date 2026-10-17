#include "Engine/Source/Gx/Metal/TextureDevice.h"
#include "MetalShaderSource.h"
#include "Engine/Source/Gx/Gx.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}
size_t Align(size_t value) { return (value + 255) & ~size_t(255); }
MTLPixelFormat PixelFormat(BLP::Format format) {
    if (format == BLP::Format::BGRA8) { return MTLPixelFormatBGRA8Unorm; }
    if (@available(macOS 11.0, iOS 16.4, *)) {
        switch (format) {
            case BLP::Format::BC1: return MTLPixelFormatBC1_RGBA;
            case BLP::Format::BC2: return MTLPixelFormatBC2_RGBA;
            case BLP::Format::BC3: return MTLPixelFormatBC3_RGBA;
            default: break;
        }
    }
    throw std::runtime_error("Unsupported native texture format");
}
}

GxMetal::TextureDevice::TextureDevice(id<MTLDevice> selected) : device(selected) {
    Require(device != nil, "No Metal device available");
    queue = [device newCommandQueue];
    Require(queue != nil, "Cannot create Metal command queue");
    queue.label = @"WoW texture upload and draw";
    MTLCompileOptions* options = [MTLCompileOptions new];
    options.languageVersion = MTLLanguageVersion2_3;
    options.fastMathEnabled = NO;
    NSError* error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:@(TextureQuadSource) options:options error:&error];
    if (!library) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Metal shader compilation failed"); }
    m_vertex = [library newFunctionWithName:@"textureQuadVertex"];
    m_filterVertex = [library newFunctionWithName:@"filter4Vertex"];
    Require(m_filterVertex != nil, "Missing native FFX vertex function");
    m_clearVertex = [library newFunctionWithName:@"clearVertex"];
    m_clearFragment = [library newFunctionWithName:@"clearFragment"];
    Require(m_clearVertex && m_clearFragment, "Missing native attachment clear functions");
    NSString* geometryFunctions[] = {@"unlitP", @"unlitPC", @"unlitPT", @"unlitPCT"};
    for (bool screenAtDepth : {false, true}) {
        MTLFunctionConstantValues* constants = [MTLFunctionConstantValues new];
        [constants setConstantValue:&screenAtDepth type:MTLDataTypeBool atIndex:1];
        for (unsigned i = 0; i < 4; ++i) {
            m_geometryVertex[screenAtDepth][i] = [library newFunctionWithName:geometryFunctions[i] constantValues:constants error:&error];
            if (!m_geometryVertex[screenAtDepth][i]) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Metal geometry specialization failed"); }
        }
    }
    for (bool alphaTest : {false, true}) {
        MTLFunctionConstantValues* constants = [MTLFunctionConstantValues new];
        [constants setConstantValue:&alphaTest type:MTLDataTypeBool atIndex:0];
        m_fragment[alphaTest] = [library newFunctionWithName:@"textureQuadFragment" constantValues:constants error:&error];
        if (!m_fragment[alphaTest]) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Metal specialization failed"); }
        for (uint32_t filter = 1; filter <= 2; ++filter) {
            [constants setConstantValue:&filter type:MTLDataTypeUInt atIndex:2];
            m_filterFragment[filter - 1][alphaTest] = [library newFunctionWithName:@"filter4Fragment" constantValues:constants error:&error];
            if (!m_filterFragment[filter - 1][alphaTest]) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Metal filter specialization failed"); }
        }
    }
    // Prepare common preview states at initialization. Other masks/states are
    // cached on demand; the full game will prewarm its material permutations.
    TextureDrawState state;
    Pipeline(state); Sampler(state);
    state.blend = GxBlend_Alpha; state.alphaRef = 1;
    Pipeline(state);
    Pipeline(state, GxVBF_PCT);
}

id<MTLRenderPipelineState> GxMetal::TextureDevice::Pipeline(const TextureDrawState& state, EGxVertexBufferFormat format, bool depthAttachment, bool screenAtDepth, FragmentEffect effect) {
    Require(state.blend >= GxBlend_Opaque && state.blend < GxBlends_Last && state.colorWrite <= 15 && state.alphaRef <= 255,
        "Invalid Gx blend, color mask or alpha reference");
    Require(format >= GxVBF_P && format <= GxVertexBufferFormats_Last, "Invalid Gx vertex format");
    const uint32_t key = uint32_t(state.blend) | (uint32_t(state.alphaRef != 0) << 4) | (state.colorWrite << 5) |
        (uint32_t(format) << 9) | (uint32_t(depthAttachment) << 13) | (uint32_t(screenAtDepth) << 14) | (uint32_t(effect) << 15);
    const auto found = m_pipelines.find(key);
    if (found != m_pipelines.end()) { return found->second; }
    // CGxDeviceD3d::s_srcBlend/s_dstBlend, preserving all 12 Gx modes.
    static const MTLBlendFactor source[] = {MTLBlendFactorOne, MTLBlendFactorOne, MTLBlendFactorSourceAlpha,
        MTLBlendFactorSourceAlpha, MTLBlendFactorDestinationColor, MTLBlendFactorDestinationColor,
        MTLBlendFactorDestinationColor, MTLBlendFactorOneMinusSourceAlpha, MTLBlendFactorOneMinusSourceAlpha,
        MTLBlendFactorSourceAlpha, MTLBlendFactorOne, MTLBlendFactorBlendAlpha};
    static const MTLBlendFactor destination[] = {MTLBlendFactorZero, MTLBlendFactorZero, MTLBlendFactorOneMinusSourceAlpha,
        MTLBlendFactorOne, MTLBlendFactorZero, MTLBlendFactorSourceColor, MTLBlendFactorOne, MTLBlendFactorOne,
        MTLBlendFactorZero, MTLBlendFactorZero, MTLBlendFactorOne, MTLBlendFactorOneMinusBlendAlpha};
    static_assert(sizeof(source) / sizeof(source[0]) == GxBlends_Last &&
        sizeof(destination) / sizeof(destination[0]) == GxBlends_Last, "Every Gx blend mode needs a native mapping");
    auto alphaFactor = [](MTLBlendFactor factor) {
        if (factor == MTLBlendFactorSourceColor) { return MTLBlendFactorSourceAlpha; }
        if (factor == MTLBlendFactorDestinationColor) { return MTLBlendFactorDestinationAlpha; }
        return factor;
    };
    MTLRenderPipelineDescriptor* descriptor = [MTLRenderPipelineDescriptor new];
    descriptor.label = [NSString stringWithFormat:@"WoW Gx blend %u mask %u alpha %d", state.blend, state.colorWrite, state.alphaRef != 0];
    descriptor.vertexFunction = m_vertex;
    if (format != GxVertexBufferFormats_Last) {
        const bool color = GxVertexHasAttrib(format, GxVA_Color0), uv = GxVertexHasAttrib(format, GxVA_TexCoord0);
        descriptor.vertexFunction = m_geometryVertex[screenAtDepth][unsigned(color) | (unsigned(uv) << 1)];
        MTLVertexDescriptor* layout = [MTLVertexDescriptor vertexDescriptor];
        auto attribute = [&](unsigned index, EGxVertexAttrib gx, MTLVertexFormat type) {
            layout.attributes[index].bufferIndex = 1;
            layout.attributes[index].offset = GxVertexAttribOffset(format, gx);
            layout.attributes[index].format = type;
        };
        attribute(0, GxVA_Position, MTLVertexFormatFloat3);
        if (color) { attribute(1, GxVA_Color0, MTLVertexFormatUChar4Normalized_BGRA); }
        if (uv) { attribute(2, GxVA_TexCoord0, MTLVertexFormatFloat2); }
        layout.layouts[1].stride = GxVertexSize(format);
        descriptor.vertexDescriptor = layout;
    }
    if (depthAttachment) { descriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float; }
    descriptor.fragmentFunction = m_fragment[state.alphaRef != 0];
    if (effect != FragmentEffect::Unlit) {
        Require((effect == FragmentEffect::Box4 || effect == FragmentEffect::Gauss4) && !screenAtDepth,
            "Invalid native FFX filter pipeline");
        descriptor.vertexFunction = m_filterVertex;
        descriptor.fragmentFunction = m_filterFragment[uint32_t(effect) - 1][state.alphaRef != 0];
        auto layout = [MTLVertexDescriptor vertexDescriptor];
        const MTLVertexFormat formats[] = {MTLVertexFormatFloat3, MTLVertexFormatUChar4Normalized_BGRA,
            MTLVertexFormatFloat2, MTLVertexFormatFloat2, MTLVertexFormatFloat2, MTLVertexFormatFloat2};
        const size_t offsets[] = {0, 12, 16, 24, 32, 40};
        for (unsigned i = 0; i < 6; ++i) {
            layout.attributes[i].bufferIndex = 1; layout.attributes[i].offset = offsets[i]; layout.attributes[i].format = formats[i];
        }
        layout.layouts[1].stride = 48; descriptor.vertexDescriptor = layout;
    }
    auto attachment = descriptor.colorAttachments[0];
    attachment.pixelFormat = MTLPixelFormatBGRA8Unorm;
    attachment.blendingEnabled = state.blend >= GxBlend_Alpha;
    attachment.sourceRGBBlendFactor = source[state.blend];
    attachment.destinationRGBBlendFactor = destination[state.blend];
    attachment.sourceAlphaBlendFactor = alphaFactor(source[state.blend]);
    attachment.destinationAlphaBlendFactor = alphaFactor(destination[state.blend]);
    attachment.writeMask = MTLColorWriteMaskNone;
    if (state.colorWrite & 1) { attachment.writeMask |= MTLColorWriteMaskRed; }
    if (state.colorWrite & 2) { attachment.writeMask |= MTLColorWriteMaskBlue; }
    if (state.colorWrite & 4) { attachment.writeMask |= MTLColorWriteMaskGreen; }
    if (state.colorWrite & 8) { attachment.writeMask |= MTLColorWriteMaskAlpha; }
    NSError* error = nil;
    id<MTLRenderPipelineState> pipeline = [device newRenderPipelineStateWithDescriptor:descriptor error:&error];
    if (!pipeline) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Metal pipeline creation failed"); }
    m_pipelines.emplace(key, pipeline);
    return pipeline;
}

id<MTLSamplerState> GxMetal::TextureDevice::Sampler(const TextureDrawState& state) {
    Require(state.filter >= GxTex_Nearest && state.filter < GxTexFilters_Last && state.wrapU >= GxTex_Clamp && state.wrapU <= GxTex_Wrap &&
        state.wrapV >= GxTex_Clamp && state.wrapV <= GxTex_Wrap && state.anisotropy >= 1 && state.anisotropy <= 16, "Invalid Gx sampler state");
    const uint32_t anisotropy = state.filter == GxTex_Anisotropic ? state.anisotropy : 1;
    const uint32_t key = uint32_t(state.filter) | (uint32_t(state.wrapU) << 3) | (uint32_t(state.wrapV) << 4) | (anisotropy << 5);
    const auto found = m_samplers.find(key);
    if (found != m_samplers.end()) { return found->second; }
    MTLSamplerDescriptor* descriptor = [MTLSamplerDescriptor new];
    const bool nearest = state.filter == GxTex_Nearest || state.filter == GxTex_NearestMipNearest;
    descriptor.minFilter = descriptor.magFilter = nearest ? MTLSamplerMinMagFilterNearest : MTLSamplerMinMagFilterLinear;
    if (state.filter <= GxTex_Linear) { descriptor.mipFilter = MTLSamplerMipFilterNotMipmapped; }
    else if (state.filter <= GxTex_LinearMipNearest) { descriptor.mipFilter = MTLSamplerMipFilterNearest; }
    else { descriptor.mipFilter = MTLSamplerMipFilterLinear; }
    descriptor.sAddressMode = state.wrapU == GxTex_Wrap ? MTLSamplerAddressModeRepeat : MTLSamplerAddressModeClampToEdge;
    descriptor.tAddressMode = state.wrapV == GxTex_Wrap ? MTLSamplerAddressModeRepeat : MTLSamplerAddressModeClampToEdge;
    descriptor.maxAnisotropy = anisotropy;
    id<MTLSamplerState> sampler = [device newSamplerStateWithDescriptor:descriptor];
    Require(sampler != nil, "Cannot create Metal sampler");
    m_samplers.emplace(key, sampler);
    return sampler;
}

bool GxMetal::TextureDevice::SupportsBC() const {
    if (@available(macOS 11.0, iOS 16.4, *)) { return device.supportsBCTextureCompression; }
    return false;
}

id<MTLTexture> GxMetal::TextureDevice::Upload(const BLP::Image& image, bool forceExpansion) {
    Require(!image.mips.empty(), "Cannot upload an empty texture");
    const bool expanded = image.format != BLP::Format::BGRA8 && (forceExpansion || !SupportsBC());
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:
        PixelFormat(expanded ? BLP::Format::BGRA8 : image.format) width:image.mips[0].width height:image.mips[0].height mipmapped:NO];
    descriptor.mipmapLevelCount = image.mips.size();
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage = MTLTextureUsageShaderRead;
    id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
    Require(texture != nil, "Metal texture allocation failed");
    texture.label = @"WoW authored BLP mip chain";
    struct Layout { size_t offset, stride, rows; };
    std::vector<Layout> layouts;
    size_t total = 0;
    for (const auto& mip : image.mips) {
        const size_t stride = Align(expanded ? mip.width * 4 : mip.rowBytes);
        const size_t rows = expanded ? mip.height : mip.bytes.size() / mip.rowBytes;
        layouts.push_back({total, stride, rows});
        total += stride * rows;
    }
    id<MTLBuffer> staging = [device newBufferWithLength:total options:MTLResourceStorageModeShared];
    Require(staging != nil, "Metal texture staging allocation failed");
    for (size_t level = 0; level < image.mips.size(); ++level) {
        BLP::Mip decoded;
        if (expanded) { decoded = image.DecodeBGRA(level); }
        const BLP::Mip& mip = expanded ? decoded : image.mips[level];
        const auto& layout = layouts[level];
        for (size_t row = 0; row < layout.rows; ++row) {
            std::memcpy(static_cast<uint8_t*>(staging.contents) + layout.offset + row * layout.stride,
                mip.bytes.data() + row * mip.rowBytes, mip.rowBytes);
        }
    }
    id<MTLCommandBuffer> command = [queue commandBuffer];
    command.label = @"Upload BLP mip chain";
    id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
    Require(blit != nil, "Cannot create Metal upload encoder");
    for (size_t level = 0; level < image.mips.size(); ++level) {
        const auto& mip = image.mips[level];
        const auto& layout = layouts[level];
        [blit copyFromBuffer:staging sourceOffset:layout.offset sourceBytesPerRow:layout.stride sourceBytesPerImage:layout.stride * layout.rows
            sourceSize:MTLSizeMake(mip.width, mip.height, 1) toTexture:texture destinationSlice:0 destinationLevel:level
            destinationOrigin:MTLOriginMake(0, 0, 0)];
    }
    [blit endEncoding];
    [command addCompletedHandler:^(id<MTLCommandBuffer> completed) {
        if (completed.error) { NSLog(@"BLP upload failed: %@", completed.error); }
    }];
    [command commit];
    return texture;
}

void GxMetal::TextureDevice::Draw(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
    const QuadParameters& parameters, const TextureDrawState& state, const MTLViewport* viewport) {
    // Resolve/validate before opening an encoder so failures cannot leave a pass open.
    id<MTLRenderPipelineState> pipeline = Pipeline(state);
    id<MTLSamplerState> sampler = Sampler(state);
    Require(std::isfinite(state.blendFactor) && state.blendFactor >= 0 && state.blendFactor <= 1, "Invalid Gx blend factor");
    const float factor = uint32_t(state.blendFactor * 255.0f) / 255.0f;
    QuadParameters constants = parameters;
    constants.alphaRef = state.alphaRef / 255.0f;
    id<MTLRenderCommandEncoder> encoder = [command renderCommandEncoderWithDescriptor:pass];
    Require(encoder != nil, "Cannot create Metal render encoder");
    [encoder setRenderPipelineState:pipeline];
    [encoder setBlendColorRed:factor green:factor blue:factor alpha:factor];
    [encoder setCullMode:MTLCullModeNone];
    if (viewport) { [encoder setViewport:*viewport]; }
    [encoder setVertexBytes:&constants length:sizeof(constants) atIndex:0];
    [encoder setFragmentBytes:&constants length:sizeof(constants) atIndex:0];
    [encoder setFragmentTexture:texture atIndex:0];
    [encoder setFragmentSamplerState:sampler atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    [encoder endEncoding];
}

void GxMetal::TextureDevice::ClearRegion(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass,
    const MTLScissorRect& region, uint32_t mask, simd_float4 color) {
    const bool hasDepth = pass.depthAttachment.texture != nil, writeColor = (mask & 1) != 0, writeDepth = (mask & 2) != 0;
    auto target = pass.colorAttachments[0].texture;
    Require(command && target && target.pixelFormat == MTLPixelFormatBGRA8Unorm && target.sampleCount == 1 &&
        (!hasDepth || pass.depthAttachment.texture.pixelFormat == MTLPixelFormatDepth32Float) && (!writeDepth || hasDepth),
        "Unsupported Metal regional clear attachments");
    Require(mask && mask <= 3 && region.width && region.height && region.x < target.width && region.y < target.height &&
        region.width <= target.width - region.x && region.height <= target.height - region.y, "Invalid Metal clear region");
    auto& pipeline = m_clearPipelines[hasDepth][writeColor];
    if (!pipeline) {
        auto descriptor = [MTLRenderPipelineDescriptor new];
        descriptor.label = @"Gx regional attachment clear";
        descriptor.vertexFunction = m_clearVertex; descriptor.fragmentFunction = m_clearFragment;
        descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        descriptor.colorAttachments[0].writeMask = writeColor ? MTLColorWriteMaskAll : MTLColorWriteMaskNone;
        if (hasDepth) { descriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float; }
        NSError* error = nil;
        pipeline = [device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        if (!pipeline) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Cannot create regional clear pipeline"); }
    }
    auto& depth = m_clearDepth[writeDepth];
    if (!depth) {
        auto descriptor = [MTLDepthStencilDescriptor new];
        descriptor.depthCompareFunction = MTLCompareFunctionAlways; descriptor.depthWriteEnabled = writeDepth;
        depth = [device newDepthStencilStateWithDescriptor:descriptor];
        Require(depth != nil, "Cannot create clear depth state");
    }
    auto encoder = [command renderCommandEncoderWithDescriptor:pass];
    Require(encoder != nil, "Cannot encode regional Gx clear");
    [encoder setRenderPipelineState:pipeline]; [encoder setDepthStencilState:depth];
    [encoder setCullMode:MTLCullModeNone];
    [encoder setViewport:MTLViewport{double(region.x), double(region.y), double(region.width), double(region.height), 0, 1}];
    [encoder setScissorRect:region];
    [encoder setFragmentBytes:&color length:sizeof(color) atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    [encoder endEncoding];
}
