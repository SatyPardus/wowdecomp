#include <metal_stdlib>
using namespace metal;

struct QuadParameters {
    float2 scale;
    float mipLevel;
    uint explicitMip;
    float4 tint;
    float2 uvScale;
    float2 uvOffset;
    float alphaRef;
    float padding[3];
};
constant bool alphaTest [[function_constant(0)]];
struct QuadVertex { float4 position [[position]]; float2 uv; float4 color; };

vertex QuadVertex textureQuadVertex(uint index [[vertex_id]], constant QuadParameters& p [[buffer(0)]]) {
    const float2 uv = float2(index >> 1, index & 1);
    // Native Metal clip coordinates, top-left texture origin, no D3D9 half-pixel offset.
    return {float4((uv * float2(2, -2) + float2(-1, 1)) * p.scale, 0, 1), uv * p.uvScale + p.uvOffset, float4(1)};
}

struct VertexP { float3 position [[attribute(0)]]; };
struct VertexPC { float3 position [[attribute(0)]]; float4 color [[attribute(1)]]; };
struct VertexPT { float3 position [[attribute(0)]]; float2 uv [[attribute(2)]]; };
struct VertexPCT { float3 position [[attribute(0)]]; float4 color [[attribute(1)]]; float2 uv [[attribute(2)]]; };
struct GeometryParameters { float4x4 clipFromModel; QuadParameters texture; };
inline QuadVertex unlitVertex(float3 position, float4 color, float2 uv, constant GeometryParameters& p) {
    return {p.clipFromModel * float4(position, 1), uv * p.texture.uvScale + p.texture.uvOffset, color};
}
vertex QuadVertex unlitP(VertexP v [[stage_in]], constant GeometryParameters& p [[buffer(0)]]) {
    return unlitVertex(v.position, float4(1), float2(0), p);
}
vertex QuadVertex unlitPC(VertexPC v [[stage_in]], constant GeometryParameters& p [[buffer(0)]]) {
    return unlitVertex(v.position, v.color, float2(0), p);
}
vertex QuadVertex unlitPT(VertexPT v [[stage_in]], constant GeometryParameters& p [[buffer(0)]]) {
    return unlitVertex(v.position, float4(1), v.uv, p);
}
vertex QuadVertex unlitPCT(VertexPCT v [[stage_in]], constant GeometryParameters& p [[buffer(0)]]) {
    return unlitVertex(v.position, v.color, v.uv, p);
}

fragment float4 textureQuadFragment(QuadVertex v [[stage_in]], texture2d<float> colorTexture [[texture(0)]],
                                   sampler filtering [[sampler(0)]], constant QuadParameters& p [[buffer(0)]]) {
    float4 texel = p.explicitMip ? colorTexture.sample(filtering, v.uv, level(p.mipLevel)) : colorTexture.sample(filtering, v.uv);
    float4 color = texel * v.color * p.tint;
    // D3D9 uses GREATEREQUAL. Keep alpha testing independent of blend factors.
    if (alphaTest && color.a < p.alphaRef) { discard_fragment(); }
    return color;
}
