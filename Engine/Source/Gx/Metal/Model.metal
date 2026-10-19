// Original model transforms and layered materials, expressed in effect terms.
constant uint modelPermutation [[function_constant(5)]];
constant bool modelShaderAlphaTest [[function_constant(6)]];
constant bool modelTextureAlpha [[function_constant(7)]];
// 0: material diffuse, 1: Color_T1, 2: CDiffuse_T1.
constant uint modelColorMode [[function_constant(8)]];
constant uint modelPrimaryUV [[function_constant(9)]];
constant uint modelLayerOperation [[function_constant(10)]];
constant uint modelBaseOperation [[function_constant(43)]];
enum ModelBaseOperation { BaseMultiply, BaseDecal, BaseFade, BaseAdd, BaseDoubleMultiply };
constant uint modelSecondaryUV [[function_constant(11)]];
enum ModelUVSource { UVNone, UVAuthoredFirst, UVAuthoredSecond, UVReflection, UVProjectedFirst, UVProjectedSecond };
constant uint modelShadowMode [[function_constant(35)]];
constant bool modelClipping [[function_constant(38)]];
constant bool modelDepthOffset [[function_constant(39)]];
struct ModelClipParameters { float4 planes[6]; uint mask; uint padding[3]; };
enum ModelLayerOperation { LayerNone, LayerAdd, LayerMultiply, LayerBlendDoubleModulate, LayerDoubleModulate, LayerAddAlpha, LayerAlphaWeightedAdd, LayerBlendAlphaWeightedAdd, LayerDoubleModulateRGB, LayerMultiplyRGB, LayerQuadrupleModulate };
struct ModelVertexParameters {
    float4x4 projection;
    float4 uv[4];
    float4 sunDiffuse, sunAmbient, sunDirection;
    float4 diffuse, emissive, fog;
    float4 lightColor[4], lightPosition[4];
    float4 attenuationConstant, attenuationLinear, attenuationQuadratic;
};
struct ModelFragmentParameters { float4 fogColorAlphaRef, decodeScale[2]; float stateAlphaRef, depthOffset; float padding[2]; };
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
// Clip distances are rasterizer outputs, not fragment inputs. Keep the
// fragment signature limited to the interpolated material data.
struct ModelVaryings {
    float4 position [[position]]; float4 color; float2 uv, secondaryUV; float fog; float pointSize [[point_size]];
    float3 viewPosition, normal, shadowCoordinate, nearShadow, middleShadow, farShadow;
};
struct ModelRaster {
    float4 position [[position]]; float4 color; float2 uv, secondaryUV; float fog; float pointSize [[point_size]];
    float3 viewPosition, normal, shadowCoordinate, nearShadow, middleShadow, farShadow;
    // Metal 2 optional return fields omit all clipping outputs when disabled.
    // https://developer.apple.com/metal/Metal-Shading-Language-Specification.pdf
    float clipDistance [[clip_distance, function_constant(modelClipping)]] [6];
};
inline float2 modelCoordinates(uint source, float2 uv, float2 uv2, float3 view, float2 reflection, constant ModelVertexParameters& p) {
    if (source == UVReflection) return reflection;
    if (source == UVAuthoredFirst) return float2(dot(p.uv[0].xyw, float3(uv, 1)), dot(p.uv[1].xyw, float3(uv, 1)));
    if (source == UVAuthoredSecond) return float2(dot(p.uv[2].xyw, float3(uv2, 1)), dot(p.uv[3].xyw, float3(uv2, 1)));
    if (source == UVProjectedFirst) return float2(dot(p.uv[0], float4(view, 1)), dot(p.uv[1], float4(view, 1)));
    if (source == UVProjectedSecond) return float2(dot(p.uv[2], float4(view, 1)), dot(p.uv[3], float4(view, 1)));
    return float2(0);
}
inline ModelRaster modelTransform(float3 position, float3 normal, float2 uv, float2 uv2, float4 vertexColor,
    ModelAffine viewFromModel, constant ModelVertexParameters& p, constant ShadowReceiverParameters& shadow, constant ModelClipParameters& clip) {
    const float4 point = float4(position, 1);
    const float3 view = float3(dot(viewFromModel.x, point), dot(viewFromModel.y, point), dot(viewFromModel.z, point));
    float3 direction = 0;
    // Color_T1 and Color_Proj_Proj output a fixed view-space +Z receiver normal,
    // including their white variants. CDiffuse_T1 uses its authored normal even
    // when unlit; neither rule follows from whether vertex lighting is enabled.
    if (modelShadowMode && modelColorMode == 1) {
        direction = float3(0, 0, 1);
    } else if (modelShadowMode || modelPrimaryUV == UVReflection || modelSecondaryUV == UVReflection || (modelColorMode != 1 && (modelPermutation & 1))) {
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
    float2 reflectionUV = 0;
    if (modelPrimaryUV == UVReflection || modelSecondaryUV == UVReflection) {
        // Original view-space sphere reflection, including its positive Y
        // mapping. The authored second texture matrix is not applied here.
        const float3 incident = -normalize(view);
        const float projection = dot(incident, direction);
        const float3 reflected = fma(float3(projection + projection), -direction, incident);
        const float3 sphere = float3(reflected.xy, reflected.z + 1);
        reflectionUV = fma(reflected.xy * rsqrt(dot(sphere, sphere)), float2(0.5f), float2(0.5f));
    }
    ModelRaster result{p.projection * float4(view, 1), color,
        modelCoordinates(modelPrimaryUV, uv, uv2, view, reflectionUV, p),
        modelCoordinates(modelSecondaryUV, uv, uv2, view, reflectionUV, p), fog, 1};
    if (modelShadowMode) {
        result.viewPosition = view; result.normal = direction;
        result.shadowCoordinate = float3(dot(shadow.projection[0].x, float4(view,1)),
            dot(shadow.projection[0].y, float4(view,1)), dot(shadow.projection[0].z, float4(view,1)));
        if (modelShadowMode >= 2) {
            result.nearShadow = float3(dot(shadow.projection[1].x, float4(view,1)),
                dot(shadow.projection[1].y, float4(view,1)), dot(shadow.projection[1].z, float4(view,1)));
            result.middleShadow = float3(dot(shadow.projection[2].x, float4(view,1)),
                dot(shadow.projection[2].y, float4(view,1)), dot(shadow.projection[2].z, float4(view,1)));
            result.farShadow = float3(dot(shadow.projection[3].x, float4(view,1)),
                dot(shadow.projection[3].y, float4(view,1)), dot(shadow.projection[3].z, float4(view,1)));
        }
    }
    if (modelClipping) for (uint i = 0; i < 6; ++i)
        result.clipDistance[i] = (clip.mask & (1u << i)) ? dot(clip.planes[i], result.position) : 1.0f;
    return result;
}
vertex ModelRaster modelRigidVertex(ModelVertex v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]], constant ShadowReceiverParameters& shadow [[buffer(3)]], constant ModelClipParameters& clip [[buffer(4)]]) {
    return modelTransform(v.position, v.normal, v.uv, v.uv2, float4(1), palette[0], p, shadow, clip);
}
vertex ModelRaster modelSkinVertex(ModelSkinVertex v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]], constant ShadowReceiverParameters& shadow [[buffer(3)]], constant ModelClipParameters& clip [[buffer(4)]]) {
    const auto transform = skinTransform(v.weights, v.bones, palette, modelPermutation >= 20);
    return modelTransform(v.position, v.normal, v.uv, v.uv2, float4(1), transform, p, shadow, clip);
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
    constant ModelAffine* palette [[buffer(2)]], constant ShadowReceiverParameters& shadow [[buffer(3)]], constant ModelClipParameters& clip [[buffer(4)]]) {
    return modelTransform(v.position, float3(0, 0, 1), v.uv, float2(0), v.color, palette[0], p, shadow, clip);
}
vertex ModelRaster modelColorLitVertex(ModelColorLitVertex v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]], constant ShadowReceiverParameters& shadow [[buffer(3)]], constant ModelClipParameters& clip [[buffer(4)]]) {
    return modelTransform(v.position, v.normal, v.uv, float2(0), v.color, palette[0], p, shadow, clip);
}
vertex ModelRaster modelWhiteVertex(ModelWhiteVertex v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]], constant ShadowReceiverParameters& shadow [[buffer(3)]], constant ModelClipParameters& clip [[buffer(4)]]) {
    return modelTransform(v.position, float3(0, 0, 1), v.uv, float2(0), float4(1), palette[0], p, shadow, clip);
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
    constant ModelAffine* palette [[buffer(2)]], constant ShadowReceiverParameters& shadow [[buffer(3)]], constant ModelClipParameters& clip [[buffer(4)]]) {
    return modelTransform(v.position, float3(0,0,1), float2(0), float2(0), float4(1), palette[0], p, shadow, clip);
}
vertex ModelRaster modelProjectedNormalVertex(ModelProjectedNormal v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]], constant ShadowReceiverParameters& shadow [[buffer(3)]], constant ModelClipParameters& clip [[buffer(4)]]) {
    return modelTransform(v.position, v.normal, float2(0), float2(0), float4(1), palette[0], p, shadow, clip);
}
vertex ModelRaster modelProjectedColorVertex(ModelProjectedColor v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]], constant ShadowReceiverParameters& shadow [[buffer(3)]], constant ModelClipParameters& clip [[buffer(4)]]) {
    return modelTransform(v.position, float3(0,0,1), float2(0), float2(0), v.color, palette[0], p, shadow, clip);
}
vertex ModelRaster modelProjectedSkinVertex(ModelProjectedSkin v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]], constant ShadowReceiverParameters& shadow [[buffer(3)]], constant ModelClipParameters& clip [[buffer(4)]]) {
    return modelTransform(v.position, v.normal, float2(0), float2(0), float4(1), skinTransform(v.weights, v.bones, palette, modelPermutation >= 20), p, shadow, clip);
}
// Dynamic distance fade precedes the minimum with distant bands. Normal fade
// then applies to that combined visibility, before RGB-only modulation.
template <typename ShadowTexture>
inline float modelShadowVisibility(ModelVaryings v, constant ShadowReceiverParameters& p,
    ShadowTexture image, sampler filtering, ShadowTexture nearImage, sampler nearSampler,
    ShadowTexture middleImage, sampler middleSampler, ShadowTexture farImage, sampler farSampler) {
    float visibility = receiverDynamicVisibility(v.shadowCoordinate,v.viewPosition,p,image,filtering,modelShadowMode);
    if (modelShadowMode >= 2)
        visibility = min(visibility,receiverBandedVisibility(v.nearShadow,v.middleShadow,v.farShadow,p,nearImage,nearSampler,middleImage,middleSampler,farImage,farSampler));
    return receiverShadowModulation(visibility,v.normal,p.lightDirection.xyz);
}
struct ModelFragmentOutput {
    float4 color [[color(0)]];
    float depth [[depth(any), function_constant(modelDepthOffset)]];
};
fragment ModelFragmentOutput modelDiffuseFragment(ModelVaryings v [[stage_in]], constant ModelFragmentParameters& p [[buffer(0)]],
    uint sample [[sample_id, function_constant(sampleDepthOffset)]],
    texture2d<float> image [[texture(0)]], sampler filtering [[sampler(0)]],
    texture2d<float> layerTexture [[texture(1)]], sampler layerFiltering [[sampler(1)]],
    constant ShadowReceiverParameters& shadow [[buffer(3)]],
    texture2d<float> shadowImage [[texture(4), function_constant(shadowFloatSampling)]], sampler shadowFiltering [[sampler(4)]],
    texture2d<float> nearImage [[texture(5), function_constant(shadowFloatSampling)]], sampler nearSampler [[sampler(5)]],
    texture2d<float> middleImage [[texture(6), function_constant(shadowFloatSampling)]], sampler middleSampler [[sampler(6)]],
    texture2d<float> farImage [[texture(7), function_constant(shadowFloatSampling)]], sampler farSampler [[sampler(7)]],
    depth2d<float> shadowDepth [[texture(4), function_constant(shadowComparison)]],
    depth2d<float> nearDepth [[texture(5), function_constant(shadowComparison)]],
    depth2d<float> middleDepth [[texture(6), function_constant(shadowComparison)]],
    depth2d<float> farDepth [[texture(7), function_constant(shadowComparison)]]) {
    if (modelShadowMode) {
        if(shadowComparison) v.color.rgb *= modelShadowVisibility(v,shadow,shadowDepth,shadowFiltering,nearDepth,nearSampler,middleDepth,middleSampler,farDepth,farSampler);
        else v.color.rgb *= modelShadowVisibility(v,shadow,shadowImage,shadowFiltering,nearImage,nearSampler,middleImage,middleSampler,farImage,farSampler);
    }
    const float4 texel = decoded(image.sample(filtering, v.uv), p.decodeScale[0]);
    float4 layer = 1;
    if (modelLayerOperation != LayerNone)
        layer = decoded(layerTexture.sample(layerFiltering, v.secondaryUV), p.decodeScale[1]);
    // Preserve the original MUL then MAD alpha-test order for Mod_Mod.
    const float baseAlpha = modelBaseOperation == BaseAdd ? texel.a + v.color.a :
        modelBaseOperation == BaseDoubleMultiply ? dot(float2(texel.a), float2(v.color.a)) :
        modelTextureAlpha ? texel.a * v.color.a : v.color.a;
    // Original DP2ADD sums two alpha products before the scalar threshold.
    const float doubledAlpha = dot(float2(baseAlpha), float2(layer.a));
    // Opaque_Add ignores texture 0 alpha; Mod_Add uses the original MAD.
    const float additiveAlpha = modelTextureAlpha ? fma(v.color.a, texel.a, layer.a) : layer.a + v.color.a;
    const float alpha = modelLayerOperation == LayerQuadrupleModulate ? (baseAlpha * layer.a) * 4 :
        modelLayerOperation == LayerAddAlpha ? additiveAlpha : modelLayerOperation == LayerDoubleModulate ? doubledAlpha : modelLayerOperation == LayerMultiply ? baseAlpha * layer.a : baseAlpha;
    const float shaderAlpha = modelLayerOperation == LayerQuadrupleModulate ? fma(baseAlpha * layer.a, 4.0f, -p.fogColorAlphaRef.a) :
        modelLayerOperation == LayerAddAlpha ? additiveAlpha - p.fogColorAlphaRef.a : modelLayerOperation == LayerDoubleModulate ? doubledAlpha - p.fogColorAlphaRef.a :
        modelLayerOperation == LayerMultiply ? fma(baseAlpha, layer.a, -p.fogColorAlphaRef.a) :
        (modelBaseOperation == BaseAdd || modelBaseOperation == BaseDoubleMultiply) ? baseAlpha - p.fogColorAlphaRef.a :
        modelTextureAlpha ? fma(texel.a, v.color.a, -p.fogColorAlphaRef.a) : v.color.a - p.fogColorAlphaRef.a;
    if (modelShaderAlphaTest && shaderAlpha < 0) discard_fragment();
    if (alphaTest && alpha < p.stateAlphaRef) discard_fragment();
    float3 shaded;
    if (modelBaseOperation == BaseAdd) {
        // Add_Mod preserves MUL(texture0, texture1), then MAD(vertex, texture1).
        const float3 combined = modelLayerOperation == LayerMultiply ?
            fma(layer.rgb, v.color.rgb, texel.rgb * layer.rgb) : texel.rgb + v.color.rgb;
        shaded = combined - p.fogColorAlphaRef.rgb;
    } else if (modelBaseOperation == BaseDoubleMultiply) {
        shaded = fma(texel.rgb + texel.rgb, v.color.rgb, -p.fogColorAlphaRef.rgb);
    } else if (modelLayerOperation == LayerQuadrupleModulate) {
        const float3 product = texel.rgb * layer.rgb;
        // Original shadowed variants scale the texture product before lighting;
        // unshadowed variants multiply lighting first, then MAD by four.
        shaded = modelShadowMode ? fma(product * 4, v.color.rgb, -p.fogColorAlphaRef.rgb) :
            fma(product * v.color.rgb, float3(4), -p.fogColorAlphaRef.rgb);
    } else if (modelBaseOperation == BaseDecal) {
        // Original LRP alpha, vertex RGB, texture RGB. Only vertex RGB receives
        // the shadow modulation above; texture alpha does not enter the result.
        shaded = fma(float3(v.color.a), v.color.rgb - texel.rgb, texel.rgb) - p.fogColorAlphaRef.rgb;
    } else if (modelBaseOperation == BaseFade) {
        // The opposite LRP direction is a distinct authored material.
        shaded = fma(float3(v.color.a), texel.rgb - v.color.rgb, v.color.rgb) - p.fogColorAlphaRef.rgb;
    } else if (modelLayerOperation == LayerAdd || modelLayerOperation == LayerAddAlpha) {
        // AddNA contributes RGB independently of either texture's alpha.
        shaded = fma(texel.rgb, v.color.rgb, layer.rgb) - p.fogColorAlphaRef.rgb;
    } else if (modelLayerOperation == LayerAlphaWeightedAdd) {
        shaded = fma(texel.rgb, v.color.rgb, layer.rgb * layer.a) - p.fogColorAlphaRef.rgb;
    } else if (modelLayerOperation == LayerBlendAlphaWeightedAdd) {
        // The combined pass applies lighting after the authored base-alpha LRP.
        const float3 combined = fma(layer.rgb, float3(layer.a), texel.rgb);
        const float3 base = fma(float3(texel.a), texel.rgb - combined, combined);
        shaded = fma(base, v.color.rgb, -p.fogColorAlphaRef.rgb);
    } else if (modelLayerOperation == LayerMultiply || modelLayerOperation == LayerMultiplyRGB || modelLayerOperation == LayerDoubleModulate || modelLayerOperation == LayerDoubleModulateRGB) {
        // Mod_Opaque multiplies both RGB layers while retaining texture 0's
        // alpha; LayerMultiplyRGB deliberately leaves the base-alpha path above.
        const float3 product = texel.rgb * layer.rgb;
        shaded = fma((modelLayerOperation == LayerDoubleModulate || modelLayerOperation == LayerDoubleModulateRGB) ? product + product : product, v.color.rgb, -p.fogColorAlphaRef.rgb);
    } else if (modelLayerOperation == LayerBlendDoubleModulate) {
        // The original combined opaque/reflection/alpha passes: base alpha
        // interpolates from double modulation back to the unmodulated base.
        const float3 product = texel.rgb * layer.rgb;
        const float3 base = fma(float3(texel.a), fma(product, float3(-2), texel.rgb), product + product);
        shaded = fma(base, v.color.rgb, -p.fogColorAlphaRef.rgb);
    } else { shaded = fma(texel.rgb, v.color.rgb, -p.fogColorAlphaRef.rgb); }
    ModelFragmentOutput result{float4(fma(float3(v.fog), shaded, p.fogColorAlphaRef.rgb), alpha)};
    // Preserve the normalized D3D9 offset after clipping and viewport mapping.
    // The disabled specialization has no fragment-depth output.
    if (modelDepthOffset) result.depth = biasedSampleDepth(v.position.z, p.depthOffset, sampleDepthOffset ? sample : 0);
    return result;
}
