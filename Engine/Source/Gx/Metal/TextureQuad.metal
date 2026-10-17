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
constant bool screenAtDepth [[function_constant(1)]];
constant uint filter4Kernel [[function_constant(2)]];
struct QuadVertex { float4 position [[position]]; float2 uv; float4 color; };

vertex float4 clearVertex(uint index [[vertex_id]]) {
    const float2 uv = float2(index >> 1, index & 1);
    return float4(uv * float2(2, -2) + float2(-1, 1), 1, 1);
}
fragment float4 clearFragment(constant float4& color [[buffer(0)]]) { return color; }

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
    float4 clip = p.clipFromModel * float4(position, 1);
    if (screenAtDepth) { clip.xy *= clip.w; }
    return {clip, uv * p.texture.uvScale + p.texture.uvOffset, color};
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

struct Filter4Vertex {
    float3 position [[attribute(0)]]; float4 color [[attribute(1)]];
    float2 uv0 [[attribute(2)]]; float2 uv1 [[attribute(3)]];
    float2 uv2 [[attribute(4)]]; float2 uv3 [[attribute(5)]];
};
struct Filter4Raster { float4 position [[position]]; float4 color; float2 uv0, uv1, uv2, uv3; };
vertex Filter4Raster filter4Vertex(Filter4Vertex v [[stage_in]], constant GeometryParameters& p [[buffer(0)]]) {
    return {p.clipFromModel * float4(v.position, 1), v.color, v.uv0, v.uv1, v.uv2, v.uv3};
}
fragment float4 filter4Fragment(Filter4Raster v [[stage_in]], constant QuadParameters& p [[buffer(0)]],
    texture2d<float> image0 [[texture(0)]], texture2d<float> image1 [[texture(1)]],
    texture2d<float> image2 [[texture(2)]], texture2d<float> image3 [[texture(3)]],
    sampler sampling0 [[sampler(0)]], sampler sampling1 [[sampler(1)]], sampler sampling2 [[sampler(2)]], sampler sampling3 [[sampler(3)]]) {
    float4 tap0 = image0.sample(sampling0, v.uv0), tap1 = image1.sample(sampling1, v.uv1);
    float4 tap2 = image2.sample(sampling2, v.uv2), tap3 = image3.sample(sampling3, v.uv3);
    float4 color;
    if (filter4Kernel == 1) {
        // Original Box4: average RGB; alpha comes from the interpolated vertex.
        float3 rgb = tap1.rgb * 0.25f;
        rgb = fma(tap0.rgb, 0.25f, rgb);
        rgb = fma(tap2.rgb, 0.25f, rgb);
        rgb = fma(tap3.rgb, 0.25f, rgb);
        color = float4(rgb, v.color.a);
    } else {
        // Original Gauss4: the inner taps carry 3/8, outer taps 1/8, for RGBA.
        color = tap1 * 0.375f;
        color = fma(tap0, 0.125f, color);
        color = fma(tap2, 0.375f, color);
        color = fma(tap3, 0.125f, color);
    }
    if (alphaTest && color.a < p.alphaRef) { discard_fragment(); }
    return color;
}

struct GlowVertex {
    float3 position [[attribute(0)]]; float4 color [[attribute(1)]];
    float2 sceneUV [[attribute(2)]]; float2 blurUV [[attribute(3)]];
};
struct GlowRaster { float4 position [[position]]; float4 color; float2 sceneUV, blurUV; };
vertex GlowRaster glowVertex(GlowVertex v [[stage_in]], constant GeometryParameters& p [[buffer(0)]]) {
    return {p.clipFromModel * float4(v.position, 1), v.color, v.sceneUV, v.blurUV};
}
fragment float4 glowFragment(GlowRaster v [[stage_in]], constant QuadParameters& p [[buffer(0)]],
    texture2d<float> scene [[texture(0)]], texture2d<float> blurred [[texture(1)]],
    sampler sceneSampling [[sampler(0)]], sampler blurSampling [[sampler(1)]]) {
    const float3 sceneColor = scene.sample(sceneSampling, v.sceneUV).rgb;
    const float3 blurColor = blurred.sample(blurSampling, v.blurUV).rgb;
    // Original FFXGlow: blue controls scene/blur interpolation; alpha controls
    // the squared blur contribution. Texture alpha does not affect the result.
    const float3 base = mix(sceneColor, blurColor, v.color.b);
    const float4 color = float4(fma(blurColor * blurColor, v.color.a, base), 1);
    if (alphaTest && color.a < p.alphaRef) { discard_fragment(); }
    return color;
}
