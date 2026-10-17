#include "Engine/Source/Gx/Metal/TextureDevice.h"
#include "MetalShaderSource.h"
#include <algorithm>
#include <cstring>
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
    for (bool blend : {false, true}) {
        MTLRenderPipelineDescriptor* descriptor = [MTLRenderPipelineDescriptor new];
        descriptor.label = blend ? @"WoW straight alpha texture" : @"WoW opaque texture";
        descriptor.vertexFunction = [library newFunctionWithName:@"textureQuadVertex"];
        descriptor.fragmentFunction = [library newFunctionWithName:@"textureQuadFragment"];
        auto attachment = descriptor.colorAttachments[0];
        attachment.pixelFormat = MTLPixelFormatBGRA8Unorm;
        attachment.blendingEnabled = blend;
        // Same factors for color and alpha as GxBlend_Alpha / D3D9 without
        // separate-alpha blending. Texture bytes remain straight alpha.
        attachment.sourceRGBBlendFactor = attachment.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        attachment.destinationRGBBlendFactor = attachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        id<MTLRenderPipelineState> pipeline = [device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        if (!pipeline) { throw std::runtime_error(error.localizedDescription.UTF8String ?: "Metal pipeline creation failed"); }
        if (blend) { m_alphaBlend = pipeline; } else { m_opaque = pipeline; }
    }
    for (bool linear : {false, true}) {
        MTLSamplerDescriptor* sampler = [MTLSamplerDescriptor new];
        sampler.minFilter = sampler.magFilter = linear ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
        sampler.mipFilter = linear ? MTLSamplerMipFilterLinear : MTLSamplerMipFilterNearest;
        sampler.sAddressMode = sampler.tAddressMode = MTLSamplerAddressModeClampToEdge;
        id<MTLSamplerState> state = [device newSamplerStateWithDescriptor:sampler];
        Require(state != nil, "Cannot create Metal sampler");
        if (linear) { m_linear = state; } else { m_nearest = state; }
    }
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
    const QuadParameters& parameters, bool blend, bool linear) {
    id<MTLRenderCommandEncoder> encoder = [command renderCommandEncoderWithDescriptor:pass];
    Require(encoder != nil, "Cannot create Metal render encoder");
    [encoder setRenderPipelineState:blend ? m_alphaBlend : m_opaque];
    [encoder setCullMode:MTLCullModeNone];
    [encoder setVertexBytes:&parameters length:sizeof(parameters) atIndex:0];
    [encoder setFragmentBytes:&parameters length:sizeof(parameters) atIndex:0];
    [encoder setFragmentTexture:texture atIndex:0];
    [encoder setFragmentSamplerState:linear ? m_linear : m_nearest atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    [encoder endEncoding];
}
