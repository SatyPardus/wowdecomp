// Native fixed-function draws: sky, horizon and texture-stage materials.
// Keep RGB/alpha operations independent and saturate after each stage.
constant uint fixedStages [[function_constant(26)]];
constant uint fixedColorOps [[function_constant(27)]];
constant uint fixedAlphaOps [[function_constant(28)]];
constant bool fixedFog [[function_constant(29)]];
constant bool fixedHasColor [[function_constant(30)]];
constant bool fixedHasUV0 [[function_constant(31)]];
constant bool fixedHasUV1 [[function_constant(32)]];
constant uint fixedPositionCoords [[function_constant(34)]];
constant bool fixedLighting [[function_constant(45)]];
constant bool fixedNormalizeNormals [[function_constant(46)]];
constant bool fixedSpecular [[function_constant(47)]];
constant uint fixedLightMask [[function_constant(48)]];
constant bool fixedTexture0 = fixedStages > 0;
constant bool fixedTexture1 = fixedStages > 1;
constant bool fixedTexture2 = fixedStages > 2;
constant bool fixedTexture3 = fixedStages > 3;
struct FixedUnlitVertexParameters {
    float4x4 clipFromModel, viewFromModel;
    float4 viewDepth, materialColor, fog;
    float4 textureRows[4][3];
    uint4 uvSet, projected;
};
struct FixedUnlitFragmentParameters {
    float4 fogColor, decodeScale[4];
    float alphaRef, depthOffset;
    float padding[2];
};
struct FixedLight {
    float4 positionOrDirection, ambient, diffuse, specular, attenuation;
};
struct FixedLightingParameters {
    float3x3 normalFromModel;
    float4 emissive, specularPower;
    uint4 materialSource;
    FixedLight lights[4];
};
struct FixedUnlitVertex {
    float3 position [[attribute(0)]];
    float4 color [[attribute(1), function_constant(fixedHasColor)]];
    float2 uv0 [[attribute(2), function_constant(fixedHasUV0)]];
    float2 uv1 [[attribute(3), function_constant(fixedHasUV1)]];
    float3 normal [[attribute(4), function_constant(fixedLighting)]];
};
struct FixedUnlitRaster {
    float4 position [[position]];
    float4 color;
    float3 specular;
    float3 uv0, uv1, uv2, uv3;
    float fog;
    float pointSize [[point_size]];
};
inline float3 fixedUnit(float3 value) {
    float magnitude = length(value);
    return magnitude > 0 ? value / magnitude : float3(0);
}
vertex FixedUnlitRaster fixedUnlitVertex(FixedUnlitVertex v [[stage_in]], constant FixedUnlitVertexParameters& p [[buffer(0)]],
    constant FixedLightingParameters& lighting [[buffer(2), function_constant(fixedLighting)]]) {
    FixedUnlitRaster out;
    float4 position = float4(v.position, 1);
    out.position = p.clipFromModel * position;
    out.color = fixedHasColor ? v.color : p.materialColor;
    out.specular = 0;
    out.fog = fixedFog ? saturate((p.fog.y - abs(dot(p.viewDepth, position))) * p.fog.z) : 1;
    out.pointSize = 1;
    float4 viewPosition = fixedPositionCoords || fixedLighting ? p.viewFromModel * position : float4(0);
    if (fixedLighting) {
        float3 normal = lighting.normalFromModel * v.normal;
        if (fixedNormalizeNormals) normal = fixedUnit(normal);
        float4 material = fixedHasColor && lighting.materialSource.x == 0 ? v.color : p.materialColor;
        float3 diffuse = fixedHasColor && lighting.materialSource.x == 2 ? v.color.rgb : lighting.emissive.rgb;
        float3 specular = 0;
        // D3D9's local-viewer half vector and inverse-transpose normal transform.
        // See Microsoft's camera-space-transformations and specular-lighting docs.
        for (uint i = 0; i < 4; ++i) {
            if (!(fixedLightMask & (1u << i))) continue;
            constant FixedLight& light = lighting.lights[i];
            float3 direction = light.positionOrDirection.xyz;
            float attenuation = 1;
            if (light.positionOrDirection.w == 1) {
                direction -= viewPosition.xyz;
                float distance = length(direction);
                if (distance > light.attenuation.w) continue;
                direction = fixedUnit(direction);
                attenuation = 1 / dot(light.attenuation.xyz, float3(1, distance, distance * distance));
            }
            diffuse += material.rgb * (light.ambient.rgb + light.diffuse.rgb * max(dot(normal, direction), 0.0f)) * attenuation;
            if (fixedSpecular) {
                float3 halfway = fixedUnit(fixedUnit(-viewPosition.xyz) + direction);
                specular += light.specular.rgb * pow(max(dot(normal, halfway), 0.0f), lighting.specularPower.w) * attenuation;
            }
        }
        out.color = float4(saturate(diffuse), material.a);
        if (fixedSpecular) {
            float3 materialSpecular = fixedHasColor && lighting.materialSource.x == 1 ? v.color.rgb : lighting.specularPower.rgb;
            out.specular = saturate(materialSpecular * specular);
        }
    }
    float3 coords[4];
    for (uint stage = 0; stage < 4; ++stage) {
        float2 uv = float2(0);
        if (p.uvSet[stage] == 0 && fixedHasUV0) uv = v.uv0;
        if (p.uvSet[stage] == 1 && fixedHasUV1) uv = v.uv1;
        float4 input = (fixedPositionCoords & (1u << stage)) ? viewPosition : float4(uv, 1, 0);
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
inline float4 fixedUnlitColor(FixedUnlitRaster v, constant FixedUnlitFragmentParameters& p,
    texture2d<float> image0, sampler sampling0,
    texture2d<float> image1, sampler sampling1,
    texture2d<float> image2, sampler sampling2,
    texture2d<float> image3, sampler sampling3) {
    float4 color = v.color;
    if (fixedTexture0) color = fixedStage(0, decoded(image0.sample(sampling0, v.uv0.xy / v.uv0.z), p.decodeScale[0]), color, v.color.a);
    if (fixedTexture1) color = fixedStage(1, decoded(image1.sample(sampling1, v.uv1.xy / v.uv1.z), p.decodeScale[1]), color, v.color.a);
    if (fixedTexture2) color = fixedStage(2, decoded(image2.sample(sampling2, v.uv2.xy / v.uv2.z), p.decodeScale[2]), color, v.color.a);
    if (fixedTexture3) color = fixedStage(3, decoded(image3.sample(sampling3, v.uv3.xy / v.uv3.z), p.decodeScale[3]), color, v.color.a);
    if (alphaTest && color.a < p.alphaRef) discard_fragment();
    if (fixedSpecular) color.rgb = saturate(color.rgb + v.specular);
    if (fixedFog) color.rgb = mix(p.fogColor.rgb, color.rgb, v.fog);
    return color;
}

fragment float4 fixedUnlitFragment(FixedUnlitRaster v [[stage_in]], constant FixedUnlitFragmentParameters& p [[buffer(0)]],
    texture2d<float> image0 [[texture(0), function_constant(fixedTexture0)]], sampler sampling0 [[sampler(0), function_constant(fixedTexture0)]],
    texture2d<float> image1 [[texture(1), function_constant(fixedTexture1)]], sampler sampling1 [[sampler(1), function_constant(fixedTexture1)]],
    texture2d<float> image2 [[texture(2), function_constant(fixedTexture2)]], sampler sampling2 [[sampler(2), function_constant(fixedTexture2)]],
    texture2d<float> image3 [[texture(3), function_constant(fixedTexture3)]], sampler sampling3 [[sampler(3), function_constant(fixedTexture3)]]) {
    return fixedUnlitColor(v, p, image0, sampling0, image1, sampling1, image2, sampling2, image3, sampling3);
}

struct BiasedFixedUnlit {
    float4 color [[color(0)]];
    float depth [[depth(any)]];
};
fragment BiasedFixedUnlit fixedUnlitBiasedFragment(FixedUnlitRaster v [[stage_in]], constant FixedUnlitFragmentParameters& p [[buffer(0)]],
    uint sample [[sample_id, function_constant(sampleDepthOffset)]],
    texture2d<float> image0 [[texture(0), function_constant(fixedTexture0)]], sampler sampling0 [[sampler(0), function_constant(fixedTexture0)]],
    texture2d<float> image1 [[texture(1), function_constant(fixedTexture1)]], sampler sampling1 [[sampler(1), function_constant(fixedTexture1)]],
    texture2d<float> image2 [[texture(2), function_constant(fixedTexture2)]], sampler sampling2 [[sampler(2), function_constant(fixedTexture2)]],
    texture2d<float> image3 [[texture(3), function_constant(fixedTexture3)]], sampler sampling3 [[sampler(3), function_constant(fixedTexture3)]]) {
    // Keep the D3D9 normalized offset after clipping and viewport mapping.
    // Metal's raster bias depends on the depth format and primitive exponent.
    return {fixedUnlitColor(v, p, image0, sampling0, image1, sampling1, image2, sampling2, image3, sampling3),
        biasedSampleDepth(v.position.z, p.depthOffset, sampleDepthOffset ? sample : 0)};
}
