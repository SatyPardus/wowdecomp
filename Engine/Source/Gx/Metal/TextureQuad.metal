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
struct QuadVertex { float4 position [[position]]; float2 uv; };

vertex QuadVertex textureQuadVertex(uint index [[vertex_id]], constant QuadParameters& p [[buffer(0)]]) {
    const float2 uv = float2(index >> 1, index & 1);
    // Native Metal clip coordinates, top-left texture origin, no D3D9 half-pixel offset.
    return {float4((uv * float2(2, -2) + float2(-1, 1)) * p.scale, 0, 1), uv * p.uvScale + p.uvOffset};
}

fragment float4 textureQuadFragment(QuadVertex v [[stage_in]], texture2d<float> colorTexture [[texture(0)]],
                                   sampler filtering [[sampler(0)]], constant QuadParameters& p [[buffer(0)]]) {
    float4 texel = p.explicitMip ? colorTexture.sample(filtering, v.uv, level(p.mipLevel)) : colorTexture.sample(filtering, v.uv);
    float4 color = texel * p.tint;
    // D3D9 uses GREATEREQUAL. Keep alpha testing independent of blend factors.
    if (alphaTest && color.a < p.alphaRef) { discard_fragment(); }
    return color;
}
