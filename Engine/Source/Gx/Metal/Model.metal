// Original Diffuse_T1 and opaque/modulated materials, expressed in model terms.
constant uint modelPermutation [[function_constant(5)]];
constant bool modelShaderAlphaTest [[function_constant(6)]];
constant bool modelTextureAlpha [[function_constant(7)]];
// 0: material diffuse, 1: Color_T1, 2: CDiffuse_T1.
constant uint modelColorMode [[function_constant(8)]];
constant bool modelEnvironment [[function_constant(9)]];
constant bool modelEnvironmentAdd [[function_constant(10)]];
struct ModelAffine { float4 x, y, z; };
struct ModelVertexParameters {
    float4x4 projection;
    float4 uv[2];
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
};
struct ModelSkinVertex {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 uv [[attribute(2)]];
    float4 weights [[attribute(3)]];
    uint4 bones [[attribute(4)]];
};
struct ModelRaster { float4 position [[position]]; float4 color; float2 uv, environmentUV; float fog; float pointSize [[point_size]]; };
inline ModelRaster modelTransform(float3 position, float3 normal, float2 uv, float4 vertexColor,
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
    float2 environmentUV = 0;
    if (modelEnvironment) {
        // Original view-space sphere reflection, including its positive Y
        // mapping. The authored second texture matrix is not applied here.
        const float3 incident = -normalize(view);
        const float projection = dot(incident, direction);
        const float3 reflected = fma(float3(projection + projection), -direction, incident);
        const float3 sphere = float3(reflected.xy, reflected.z + 1);
        environmentUV = fma(reflected.xy * rsqrt(dot(sphere, sphere)), float2(0.5f), float2(0.5f));
    }
    return {p.projection * float4(view, 1), color,
        float2(dot(p.uv[0].xyw, texturePoint), dot(p.uv[1].xyw, texturePoint)), environmentUV, fog, 1};
}
vertex ModelRaster modelRigidVertex(ModelVertex v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    return modelTransform(v.position, v.normal, v.uv, float4(1), palette[0], p);
}
vertex ModelRaster modelSkinVertex(ModelSkinVertex v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    ModelAffine transform = palette[v.bones.x];
    if (modelPermutation >= 20) {
        // Match the original's y,x,z,w weighted-matrix association; do not
        // normalize weights or blend already-normalized vertex normals.
        transform.x = fma(palette[v.bones.x].x, v.weights.x, palette[v.bones.y].x * v.weights.y);
        transform.y = fma(palette[v.bones.x].y, v.weights.x, palette[v.bones.y].y * v.weights.y);
        transform.z = fma(palette[v.bones.x].z, v.weights.x, palette[v.bones.y].z * v.weights.y);
        transform.x = fma(palette[v.bones.z].x, v.weights.z, transform.x);
        transform.y = fma(palette[v.bones.z].y, v.weights.z, transform.y);
        transform.z = fma(palette[v.bones.z].z, v.weights.z, transform.z);
        transform.x = fma(palette[v.bones.w].x, v.weights.w, transform.x);
        transform.y = fma(palette[v.bones.w].y, v.weights.w, transform.y);
        transform.z = fma(palette[v.bones.w].z, v.weights.w, transform.z);
    }
    return modelTransform(v.position, v.normal, v.uv, float4(1), transform, p);
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
    return modelTransform(v.position, float3(0, 0, 1), v.uv, v.color, palette[0], p);
}
vertex ModelRaster modelColorLitVertex(ModelColorLitVertex v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    return modelTransform(v.position, v.normal, v.uv, v.color, palette[0], p);
}
vertex ModelRaster modelWhiteVertex(ModelWhiteVertex v [[stage_in]], constant ModelVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    return modelTransform(v.position, float3(0, 0, 1), v.uv, float4(1), palette[0], p);
}
fragment float4 modelDiffuseFragment(ModelRaster v [[stage_in]], constant ModelFragmentParameters& p [[buffer(0)]],
    texture2d<float> image [[texture(0)]], sampler filtering [[sampler(0)]],
    texture2d<float> environment [[texture(1)]], sampler environmentFiltering [[sampler(1)]]) {
    const float4 texel = decoded(image.sample(filtering, v.uv), p.decodeScale[0]);
    // Opaque ignores texture alpha; Mod multiplies it by material/vertex alpha.
    // The original Mod shader tests a MAD result before writing MUL alpha.
    const float alpha = modelTextureAlpha ? texel.a * v.color.a : v.color.a;
    const float shaderAlpha = modelTextureAlpha ? fma(texel.a, v.color.a, -p.fogColorAlphaRef.a) : v.color.a - p.fogColorAlphaRef.a;
    if (modelShaderAlphaTest && shaderAlpha < 0) discard_fragment();
    if (alphaTest && alpha < p.stateAlphaRef) discard_fragment();
    float3 shaded;
    if (modelEnvironmentAdd) {
        const float3 reflection = decoded(environment.sample(environmentFiltering, v.environmentUV), p.decodeScale[1]).rgb;
        // AddNA contributes RGB independently of either texture's alpha and
        // remains unsaturated until the final attachment write/blend.
        shaded = fma(texel.rgb, v.color.rgb, reflection) - p.fogColorAlphaRef.rgb;
    } else { shaded = fma(texel.rgb, v.color.rgb, -p.fogColorAlphaRef.rgb); }
    return float4(fma(float3(v.fog), shaded, p.fogColorAlphaRef.rgb), alpha);
}
