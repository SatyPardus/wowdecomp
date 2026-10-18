// The ripple texture supplies RGB; vertex color only changes alpha. The game
// applies additive source-alpha blending separately in the render state.
struct WaterRippleParameters {
    float4 decodeScale;
    float alphaRef, depthOffset;
    float padding[2];
};
vertex QuadVertex waterRippleVertex(VertexPCT v [[stage_in]], constant float4x4& clipFromWorld [[buffer(0)]]) {
    const float4 clip =
        fma(v.position.z, clipFromWorld[2], fma(v.position.x, clipFromWorld[0], v.position.y * clipFromWorld[1])) + clipFromWorld[3];
    return {clip, v.uv, v.color, 1};
}
inline float4 waterRippleColor(QuadVertex v, constant WaterRippleParameters& p, texture2d<float> image, sampler filtering) {
    const float4 texel = decoded(image.sample(filtering, v.uv), p.decodeScale);
    const float alpha = texel.a * v.color.a;
    if (alphaTest && alpha < p.alphaRef)
        discard_fragment();
    return float4(texel.rgb, alpha);
}
fragment float4 waterRippleFragment(QuadVertex v [[stage_in]], constant WaterRippleParameters& p [[buffer(0)]],
                                    texture2d<float> image [[texture(0)]], sampler filtering [[sampler(0)]]) {
    return waterRippleColor(v, p, image, filtering);
}
struct BiasedRipple {
    float4 color [[color(0)]];
    float depth [[depth(any)]];
};
fragment BiasedRipple waterRippleBiasedFragment(QuadVertex v [[stage_in]], constant WaterRippleParameters& p [[buffer(0)]],
                                                texture2d<float> image [[texture(0)]], sampler filtering [[sampler(0)]]) {
    // D3D9 uses a normalized, constant depth offset. Metal's raster bias is
    // depth-format/primitive dependent. Apply the original offset after clipping
    // and viewport mapping; shifting vertices would also move the clip planes.
    return {waterRippleColor(v, p, image, filtering), saturate(v.position.z + p.depthOffset)};
}
