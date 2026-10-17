#include "Engine/Source/Gx/Metal/TextureDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include <cmath>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) { if (!condition) { throw std::runtime_error(message); } }
struct alignas(16) GeometryParameters {
    C44Matrix clipFromModel;
    GxMetal::QuadParameters texture;
};
static_assert(sizeof(GeometryParameters) == 128 && offsetof(GeometryParameters, texture) == 64, "Metal geometry constant layout");
}

void GxMetal::TextureDevice::DrawUnlit(id<MTLCommandBuffer> command, MTLRenderPassDescriptor* pass, id<MTLTexture> texture,
    const GeometryDraw& geometry, const QuadParameters& parameters, const TextureDrawState& state) {
    Require(command && pass && texture && geometry.vertices, "Missing Metal geometry resource");
    Require(geometry.format >= GxVBF_P && geometry.format < GxVertexBufferFormats_Last, "Invalid Gx vertex format");
    Require(geometry.format != GxVBF_PBNT2, "Skinned Gx vertices require a skinning effect");
    const auto& batch = geometry.batch;
    Require(batch.m_primType >= GxPrim_Points && batch.m_primType < GxPrim_TriangleFan,
        "This Metal geometry path requires points, lines, line strips, triangles or triangle strips");
    const MTLPrimitiveType primitives[] = {MTLPrimitiveTypePoint, MTLPrimitiveTypeLine, MTLPrimitiveTypeLineStrip,
        MTLPrimitiveTypeTriangle, MTLPrimitiveTypeTriangleStrip};
    size_t count = batch.m_count;
    if (batch.m_primType == GxPrim_Lines) { count -= count % 2; }
    if (batch.m_primType == GxPrim_Triangles) { count -= count % 3; }
    Require(count >= (batch.m_primType >= GxPrim_Triangles ? 3 : batch.m_primType >= GxPrim_Lines ? 2 : 1), "Empty Gx geometry batch");
    const size_t stride = GxVertexSize(geometry.format);
    Require(geometry.vertexOffset % 4 == 0 && geometry.vertexOffset <= geometry.vertices.length, "Invalid Gx vertex buffer offset");
    const size_t availableVertices = (geometry.vertices.length - geometry.vertexOffset) / stride;
    size_t indexOffset = geometry.indexOffset;
    if (geometry.indices) {
        Require(batch.m_minIndex <= batch.m_maxIndex && batch.m_maxIndex < availableVertices, "Gx vertex range exceeds buffer");
        Require(indexOffset <= geometry.indices.length && batch.m_start <= (geometry.indices.length - indexOffset) / sizeof(uint16_t),
            "Invalid Gx index buffer offset");
        indexOffset += size_t(batch.m_start) * sizeof(uint16_t);
        Require(indexOffset % 2 == 0 && indexOffset <= geometry.indices.length &&
            count <= (geometry.indices.length - indexOffset) / sizeof(uint16_t), "Gx index range exceeds buffer");
    } else {
        Require(count <= availableVertices, "Gx vertex count exceeds buffer");
    }
    id<MTLTexture> target = pass.colorAttachments[0].texture;
    Require(target && target.pixelFormat == MTLPixelFormatBGRA8Unorm && target.sampleCount == 1, "Unsupported geometry color attachment");
    const bool hasDepth = pass.depthAttachment.texture != nil;
    Require(!hasDepth || pass.depthAttachment.texture.pixelFormat == MTLPixelFormatDepth32Float, "Unsupported geometry depth attachment");
    Require(hasDepth || (!geometry.depthTest && !geometry.depthWrite), "Gx depth state requires a depth attachment");
    Require(geometry.depthFunction < 4 && geometry.culling < 3, "Invalid Gx depth/culling state");
    if (geometry.scissorEnabled) {
        Require(geometry.scissor.x <= target.width && geometry.scissor.y <= target.height &&
            geometry.scissor.width <= target.width - geometry.scissor.x && geometry.scissor.height <= target.height - geometry.scissor.y,
            "Gx scissor exceeds target");
    }
    Require(std::isfinite(state.blendFactor) && state.blendFactor >= 0 && state.blendFactor <= 1, "Invalid Gx blend factor");
    if (geometry.viewportEnabled) {
        const auto& v = geometry.viewport;
        Require(std::isfinite(v.originX) && std::isfinite(v.originY) && std::isfinite(v.width) && std::isfinite(v.height) &&
            v.originX >= 0 && v.originY >= 0 && v.width > 0 && v.height > 0 &&
            v.originX + v.width <= target.width && v.originY + v.height <= target.height &&
            v.znear >= 0 && v.zfar <= 1 && v.znear <= v.zfar, "Invalid Metal geometry viewport");
    }
    id<MTLRenderPipelineState> pipeline = Pipeline(state, geometry.format, hasDepth, geometry.screenAtDepth);
    id<MTLSamplerState> sampler = Sampler(state);
    const uint32_t depthKey = geometry.depthFunction | (unsigned(geometry.depthTest) << 2) | (unsigned(geometry.depthWrite) << 3);
    auto depth = m_depthStates.find(depthKey);
    if (depth == m_depthStates.end()) {
        const MTLCompareFunction comparison[] = {MTLCompareFunctionLessEqual, MTLCompareFunctionEqual, MTLCompareFunctionGreaterEqual, MTLCompareFunctionLess};
        MTLDepthStencilDescriptor* descriptor = [MTLDepthStencilDescriptor new];
        descriptor.depthCompareFunction = geometry.depthTest ? comparison[geometry.depthFunction] : MTLCompareFunctionAlways;
        // Gx leaves D3D9 ZENABLE on: disabling its DepthTest selects ALWAYS,
        // while GxRs_DepthWrite remains independent.
        descriptor.depthWriteEnabled = geometry.depthWrite;
        id<MTLDepthStencilState> created = [device newDepthStencilStateWithDescriptor:descriptor];
        Require(created != nil, "Cannot create Metal depth state");
        depth = m_depthStates.emplace(depthKey, created).first;
    }
    GeometryParameters constants{geometry.clipFromModel, parameters};
    constants.texture.alphaRef = state.alphaRef / 255.0f;
    // Tempest uses row vectors. Its contiguous rows are Metal's columns, so
    // column-vector multiplication reads the same transform without repacking.
    id<MTLRenderCommandEncoder> encoder = [command renderCommandEncoderWithDescriptor:pass];
    Require(encoder != nil, "Cannot create Metal geometry encoder");
    [encoder setRenderPipelineState:pipeline];
    [encoder setDepthStencilState:depth->second];
    [encoder setFrontFacingWinding:MTLWindingClockwise];
    if (geometry.viewportEnabled) { [encoder setViewport:geometry.viewport]; }
    [encoder setCullMode:geometry.culling == 0 ? MTLCullModeNone : geometry.culling == 1 ? MTLCullModeFront : MTLCullModeBack];
    if (geometry.scissorEnabled) { [encoder setScissorRect:geometry.scissor]; }
    const float factor = uint32_t(state.blendFactor * 255.0f) / 255.0f;
    [encoder setBlendColorRed:factor green:factor blue:factor alpha:factor];
    [encoder setVertexBytes:&constants length:sizeof(constants) atIndex:0];
    [encoder setVertexBuffer:geometry.vertices offset:geometry.vertexOffset atIndex:1];
    [encoder setFragmentBytes:&constants.texture length:sizeof(constants.texture) atIndex:0];
    [encoder setFragmentTexture:texture atIndex:0];
    [encoder setFragmentSamplerState:sampler atIndex:0];
    if (geometry.indices) {
        [encoder drawIndexedPrimitives:primitives[batch.m_primType] indexCount:count indexType:MTLIndexTypeUInt16
            indexBuffer:geometry.indices indexBufferOffset:indexOffset];
    } else {
        [encoder drawPrimitives:primitives[batch.m_primType] vertexStart:0 vertexCount:count];
    }
    [encoder endEncoding];
}
