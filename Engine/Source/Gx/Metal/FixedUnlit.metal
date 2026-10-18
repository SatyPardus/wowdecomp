// Native unlit fixed-function draws: sky, horizon and texture-stage materials.
// Keep RGB/alpha operations independent and saturate after each stage.
constant uint fixedStages [[function_constant(26)]];
constant uint fixedColorOps [[function_constant(27)]];
constant uint fixedAlphaOps [[function_constant(28)]];
constant bool fixedFog [[function_constant(29)]];
constant bool fixedHasColor [[function_constant(30)]];
constant bool fixedHasUV0 [[function_constant(31)]];
constant bool fixedHasUV1 [[function_constant(32)]];
constant bool fixedTexture0 = fixedStages > 0;
constant bool fixedTexture1 = fixedStages > 1;
constant bool fixedTexture2 = fixedStages > 2;
constant bool fixedTexture3 = fixedStages > 3;
struct FixedUnlitVertexParameters {
    float4x4 clipFromModel;
    float4 viewDepth, materialColor, fog;
    float4 textureRows[4][3];
    uint4 uvSet, projected;
};
struct FixedUnlitFragmentParameters {
    float4 fogColor, decodeScale[4];
    float alphaRef;
    float padding[3];
};
struct FixedUnlitVertex {
    float3 position [[attribute(0)]];
    float4 color [[attribute(1), function_constant(fixedHasColor)]];
    float2 uv0 [[attribute(2), function_constant(fixedHasUV0)]];
    float2 uv1 [[attribute(3), function_constant(fixedHasUV1)]];
};
struct FixedUnlitRaster {
    float4 position [[position]];
    float4 color;
    float3 uv0, uv1, uv2, uv3;
    float fog;
    float pointSize [[point_size]];
};
vertex FixedUnlitRaster fixedUnlitVertex(FixedUnlitVertex v [[stage_in]], constant FixedUnlitVertexParameters& p [[buffer(0)]]) {
    FixedUnlitRaster out;
    float4 position = float4(v.position, 1);
    out.position = p.clipFromModel * position;
    out.color = fixedHasColor ? v.color : p.materialColor;
    out.fog = fixedFog ? saturate((p.fog.y - abs(dot(p.viewDepth, position))) * p.fog.z) : 1;
    out.pointSize = 1;
    float3 coords[4];
    for (uint stage = 0; stage < 4; ++stage) {
        float2 uv = float2(0);
        if (p.uvSet[stage] == 0 && fixedHasUV0) uv = v.uv0;
        if (p.uvSet[stage] == 1 && fixedHasUV1) uv = v.uv1;
        float4 input = float4(uv, 1, 0);
        coords[stage] = float3(dot(input, p.textureRows[stage][0]), dot(input, p.textureRows[stage][1]),
                              p.projected[stage] ? dot(input, p.textureRows[stage][2]) : 1);
    }
    out.uv0 = coords[0]; out.uv1 = coords[1]; out.uv2 = coords[2]; out.uv3 = coords[3];
    return out;
}
inline float4 fixedOperation(uint op, float4 texel, float4 current, float diffuseAlpha) {
    switch (op) {
        case 0: return texel * current;
        case 1: return 2 * texel * current;
        case 2: return texel + current;
        case 3: return current;
        // The original Gx table reverses arg1/arg2 for BLENDCURRENTALPHA.
        case 4: return current * current.a + texel * (1 - current.a);
        case 5: return texel * diffuseAlpha + current * (1 - diffuseAlpha);
    }
    return current;
}
inline float4 fixedStage(uint stage, float4 texel, float4 current, float diffuseAlpha) {
    return saturate(float4(fixedOperation((fixedColorOps >> (stage * 3)) & 7, texel, current, diffuseAlpha).rgb,
                           fixedOperation((fixedAlphaOps >> (stage * 3)) & 7, texel, current, diffuseAlpha).a));
}
fragment float4 fixedUnlitFragment(FixedUnlitRaster v [[stage_in]], constant FixedUnlitFragmentParameters& p [[buffer(0)]],
    texture2d<float> image0 [[texture(0), function_constant(fixedTexture0)]], sampler sampling0 [[sampler(0), function_constant(fixedTexture0)]],
    texture2d<float> image1 [[texture(1), function_constant(fixedTexture1)]], sampler sampling1 [[sampler(1), function_constant(fixedTexture1)]],
    texture2d<float> image2 [[texture(2), function_constant(fixedTexture2)]], sampler sampling2 [[sampler(2), function_constant(fixedTexture2)]],
    texture2d<float> image3 [[texture(3), function_constant(fixedTexture3)]], sampler sampling3 [[sampler(3), function_constant(fixedTexture3)]]) {
    float4 color = v.color;
    if (fixedTexture0) color = fixedStage(0, decoded(image0.sample(sampling0, v.uv0.xy / v.uv0.z), p.decodeScale[0]), color, v.color.a);
    if (fixedTexture1) color = fixedStage(1, decoded(image1.sample(sampling1, v.uv1.xy / v.uv1.z), p.decodeScale[1]), color, v.color.a);
    if (fixedTexture2) color = fixedStage(2, decoded(image2.sample(sampling2, v.uv2.xy / v.uv2.z), p.decodeScale[2]), color, v.color.a);
    if (fixedTexture3) color = fixedStage(3, decoded(image3.sample(sampling3, v.uv3.xy / v.uv3.z), p.decodeScale[3]), color, v.color.a);
    if (alphaTest && color.a < p.alphaRef) discard_fragment();
    if (fixedFog) color.rgb = mix(p.fogColor.rgb, color.rgb, v.fog);
    return color;
}
