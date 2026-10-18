// Original model transforms and layered materials, expressed in effect terms.
constant uint modelPermutation [[function_constant(5)]];
constant bool modelShaderAlphaTest [[function_constant(6)]];
constant bool modelTextureAlpha [[function_constant(7)]];
// 0: material diffuse, 1: Color_T1, 2: CDiffuse_T1.
constant uint modelColorMode [[function_constant(8)]];
constant bool modelEnvironment [[function_constant(9)]];
constant uint modelLayerOperation [[function_constant(10)]];
constant bool modelDualUV [[function_constant(11)]];
constant bool modelProjected [[function_constant(20)]];
constant bool modelReflectionOnly [[function_constant(33)]];
enum ModelLayerOperation { LayerNone, LayerAdd, LayerMultiply, LayerBlendDoubleModulate, LayerDoubleModulate, LayerAddAlpha, LayerAlphaWeightedAdd, LayerBlendAlphaWeightedAdd, LayerDoubleModulateRGB };
struct ModelVertexParameters {
    float4x4 projection;
    float4 uv[4];
    float4 sunDiffuse, sunAmbient, sunDirection;
    float4 diffuse, emissive, fog;
    float4 lightColor[4], lightPosition[4];
    float4 attenuationConstant, attenuationLinear, attenuationQuadratic;
};
struct ModelFragmentParameters { float4 fogColorAlphaRef, decodeScale[2]; float stateAlphaRef; float padding[3]; };
struct ModelVertex {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 uv [[attribute(2)]];
    float2 uv2 [[attribute(6)]];
};
struct ModelSkinVertex {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 uv [[attribute(2)]];
    float2 uv2 [[attribute(6)]];
    float4 weights [[attribute(3)]];
    uint4 bones [[attribute(4)]];
};
struct ModelRaster { float4 position [[position]]; float4 color; float2 uv, secondaryUV; float fog; float pointSize [[point_size]]; };
inline ModelRaster modelTransform(float3 position, float3 normal, float2 uv, float2 uv2, float4 vertexColor,
    ModelAffine viewFromModel, constant ModelVertexParameters& p) {
    const float4 point = float4(position, 1);
    const float3 view = float3(dot(viewFromModel.x, point), dot(viewFromModel.y, point), dot(viewFromModel.z, point));
    float3 direction = 0;
    if (modelEnvironment || (modelColorMode != 1 && (modelPermutation & 1))) {
        direction = normalize(float3(dot(viewFromModel.x.xyz, normal),
            dot(viewFromModel.y.xyz, normal), dot(viewFromModel.z.xyz, normal)));
    }
    float4 color;
    if (modelColorMode == 1) {
        color = (modelPermutation & 1) ? float4(1) : vertexColor;
    } else if (modelPermutation & 1) {
        float3 lighting = fma(saturate(dot(-p.sunDirection.xyz, direction)), p.sunDiffuse.xyz, p.sunAmbient.xyz);
        const uint count = (modelPermutation / 2) % 5;
        for (uint i = 0; i < count; ++i) {
            const float3 toLight = p.lightPosition[i].xyz - view;
            const float distanceSquared = dot(toLight, toLight);
            const float inverseDistance = rsqrt(distanceSquared);
            const float attenuation = 1 / (fma(distanceSquared, p.attenuationQuadratic[i],
                distanceSquared * inverseDistance * p.attenuationLinear[i]) + p.attenuationConstant[i]);
            const float diffuse = max(dot(toLight, direction) * inverseDistance, 0.0f);
            lighting = fma(p.lightColor[i].xyz, attenuation * diffuse, lighting);
        }
        color = saturate(fma(modelColorMode == 2 ? vertexColor : p.diffuse, float4(saturate(lighting), 1), p.emissive));
    } else { color = modelColorMode == 2 ? vertexColor : saturate(p.diffuse + p.emissive); }
    const float fog = min(pow(max(fma(view.z, p.fog.x, p.fog.y), 0.0f), p.fog.z), 1.0f);
    const float3 texturePoint = float3(uv, 1);
    float2 secondaryUV = 0;
    if (modelProjected) {
        secondaryUV = float2(dot(p.uv[2], float4(view, 1)), dot(p.uv[3], float4(view, 1)));
    } else if (modelDualUV) {
        const float3 secondPoint = float3(uv2, 1);
        secondaryUV = float2(dot(p.uv[2].xyw, secondPoint), dot(p.uv[3].xyw, secondPoint));
    } else if (modelEnvironment) {
        // Original view-space sphere reflection, including its positive Y
        // mapping. The authored second texture matrix is not applied here.
        const float3 incident = -normalize(view);
        const float projection = dot(incident, direction);
        const float3 reflected = fma(float3(projection + projection), -direction, incident);
        const float3 sphere = float3(reflected.xy, reflected.z + 1);
        secondaryUV = fma(reflected.xy * rsqrt(dot(sphere, sphere)), float2(0.5f), float2(0.5f));
    }
    return {p.projection * float4(view, 1), color,
        modelReflectionOnly ? secondaryUV : modelProjected ? float2(dot(p.uv[0], float4(view, 1)), dot(p.uv[1], float4(view, 1))) :
            float2(dot(p.uv[0].xyw, texturePoint), dot(p.uv[1].xyw, texturePoint)), secondaryUV, fog, 1};
}
vertex ModelRaster modelRigidVertex(ModelVertex v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    return modelTransform(v.position, v.normal, v.uv, v.uv2, float4(1), palette[0], p);
}
vertex ModelRaster modelSkinVertex(ModelSkinVertex v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    const auto transform = skinTransform(v.weights, v.bones, palette, modelPermutation >= 20);
    return modelTransform(v.position, v.normal, v.uv, v.uv2, float4(1), transform, p);
}
struct ModelColorVertex {
    float3 position [[attribute(0)]];
    float2 uv [[attribute(2)]];
    float4 color [[attribute(5)]];
};
struct ModelColorLitVertex {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 uv [[attribute(2)]];
    float4 color [[attribute(5)]];
};
struct ModelWhiteVertex { float3 position [[attribute(0)]]; float2 uv [[attribute(2)]]; };
vertex ModelRaster modelColorVertex(ModelColorVertex v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    return modelTransform(v.position, float3(0, 0, 1), v.uv, float2(0), v.color, palette[0], p);
}
vertex ModelRaster modelColorLitVertex(ModelColorLitVertex v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    return modelTransform(v.position, v.normal, v.uv, float2(0), v.color, palette[0], p);
}
vertex ModelRaster modelWhiteVertex(ModelWhiteVertex v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    return modelTransform(v.position, float3(0, 0, 1), v.uv, float2(0), float4(1), palette[0], p);
}
// Projected and reflection-only geometry have no authored UV stream. These entry points match the
// original PN/PC/P world batches and the existing packed skinned M2 layout.
struct ModelProjectedPosition { float3 position [[attribute(0)]]; };
struct ModelProjectedNormal { float3 position [[attribute(0)]]; float3 normal [[attribute(1)]]; };
struct ModelProjectedColor { float3 position [[attribute(0)]]; float4 color [[attribute(5)]]; };
struct ModelProjectedSkin {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float4 weights [[attribute(3)]];
    uint4 bones [[attribute(4)]];
};
vertex ModelRaster modelProjectedPositionVertex(ModelProjectedPosition v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    return modelTransform(v.position, float3(0,0,1), float2(0), float2(0), float4(1), palette[0], p);
}
vertex ModelRaster modelProjectedNormalVertex(ModelProjectedNormal v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    return modelTransform(v.position, v.normal, float2(0), float2(0), float4(1), palette[0], p);
}
vertex ModelRaster modelProjectedColorVertex(ModelProjectedColor v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    return modelTransform(v.position, float3(0,0,1), float2(0), float2(0), v.color, palette[0], p);
}
vertex ModelRaster modelProjectedSkinVertex(ModelProjectedSkin v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    return modelTransform(v.position, v.normal, float2(0), float2(0), float4(1), skinTransform(v.weights, v.bones, palette, modelPermutation >= 20), p);
}
fragment float4 modelDiffuseFragment(ModelRaster v [[stage_in]], constant ModelFragmentParameters& p [[buffer(0)]],
    texture2d<float> image [[texture(0)]], sampler filtering [[sampler(0)]],
    texture2d<float> layerTexture [[texture(1)]], sampler layerFiltering [[sampler(1)]]) {
    const float4 texel = decoded(image.sample(filtering, v.uv), p.decodeScale[0]);
    float4 layer = 1;
    if (modelLayerOperation != LayerNone)
        layer = decoded(layerTexture.sample(layerFiltering, v.secondaryUV), p.decodeScale[1]);
    // Preserve the original MUL then MAD alpha-test order for Mod_Mod.
    const float baseAlpha = modelTextureAlpha ? texel.a * v.color.a : v.color.a;
    // Original DP2ADD sums two alpha products before the scalar threshold.
    const float doubledAlpha = dot(float2(baseAlpha), float2(layer.a));
    // Opaque_Add ignores texture 0 alpha; Mod_Add uses the original MAD.
    const float additiveAlpha = modelTextureAlpha ? fma(v.color.a, texel.a, layer.a) : layer.a + v.color.a;
    const float alpha = modelLayerOperation == LayerAddAlpha ? additiveAlpha : modelLayerOperation == LayerDoubleModulate ? doubledAlpha : modelLayerOperation == LayerMultiply ? baseAlpha * layer.a : baseAlpha;
    const float shaderAlpha = modelLayerOperation == LayerAddAlpha ? additiveAlpha - p.fogColorAlphaRef.a : modelLayerOperation == LayerDoubleModulate ? doubledAlpha - p.fogColorAlphaRef.a :
        modelLayerOperation == LayerMultiply ? fma(baseAlpha, layer.a, -p.fogColorAlphaRef.a) :
        modelTextureAlpha ? fma(texel.a, v.color.a, -p.fogColorAlphaRef.a) : v.color.a - p.fogColorAlphaRef.a;
    if (modelShaderAlphaTest && shaderAlpha < 0) discard_fragment();
    if (alphaTest && alpha < p.stateAlphaRef) discard_fragment();
    float3 shaded;
    if (modelLayerOperation == LayerAdd || modelLayerOperation == LayerAddAlpha) {
        // AddNA contributes RGB independently of either texture's alpha.
        shaded = fma(texel.rgb, v.color.rgb, layer.rgb) - p.fogColorAlphaRef.rgb;
    } else if (modelLayerOperation == LayerAlphaWeightedAdd) {
        shaded = fma(texel.rgb, v.color.rgb, layer.rgb * layer.a) - p.fogColorAlphaRef.rgb;
    } else if (modelLayerOperation == LayerBlendAlphaWeightedAdd) {
        // The combined pass applies lighting after the authored base-alpha LRP.
        const float3 combined = fma(layer.rgb, float3(layer.a), texel.rgb);
        const float3 base = fma(float3(texel.a), texel.rgb - combined, combined);
        shaded = fma(base, v.color.rgb, -p.fogColorAlphaRef.rgb);
    } else if (modelLayerOperation == LayerMultiply || modelLayerOperation == LayerDoubleModulate || modelLayerOperation == LayerDoubleModulateRGB) {
        const float3 product = texel.rgb * layer.rgb;
        shaded = fma(modelLayerOperation != LayerMultiply ? product + product : product, v.color.rgb, -p.fogColorAlphaRef.rgb);
    } else if (modelLayerOperation == LayerBlendDoubleModulate) {
        // The original combined opaque/reflection/alpha passes: base alpha
        // interpolates from double modulation back to the unmodulated base.
        const float3 product = texel.rgb * layer.rgb;
        const float3 base = fma(float3(texel.a), fma(product, float3(-2), texel.rgb), product + product);
        shaded = fma(base, v.color.rgb, -p.fogColorAlphaRef.rgb);
    } else { shaded = fma(texel.rgb, v.color.rgb, -p.fogColorAlphaRef.rgb); }
    return float4(fma(float3(v.fog), shaded, p.fogColorAlphaRef.rgb), alpha);
}
