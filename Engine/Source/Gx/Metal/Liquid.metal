// Original texture-animated water and magma. Water's two independent layers
// have different UV sources; its normals and light sum are not normalized or
// saturated beyond the explicit per-light dot products in the mounted shader.
constant uint liquidMaterial [[function_constant(18)]];
constant uint liquidLightCount [[function_constant(19)]];
struct LiquidUVTransform { float4 x, y, translation; };
struct LiquidPointLight { float4 position, color, attenuation; };
struct LiquidVertexParameters {
    float4x4 projection, worldView;
    LiquidUVTransform textureTransform[2];
    float4 fog, sunDirection, ambient, diffuse, specular;
    LiquidPointLight lights[3];
};
struct LiquidFragmentParameters {
    float4 fogColor, decodeScale[2];
    float stateAlphaRef;
    float padding[3];
};
struct LiquidRaster {
    float4 position [[position]];
    float4 color;
    float3 specular;
    float2 uv, layerUV;
    float fog;
    float pointSize [[point_size]];
};
inline float2 liquidUV(float2 uv, constant LiquidUVTransform& p) {
    return fma(uv.x, p.x.xy, uv.y * p.y.xy) + p.translation.xy;
}
inline LiquidRaster liquidTransform(float3 position, float3 normal, float4 color, float2 uv, float2 layerUV,
    constant LiquidVertexParameters& p) {
    const float4 view = p.worldView * float4(position, 1);
    const float fog = min(pow(max(fma(view.z, p.fog.x, p.fog.y), 0.0f), p.fog.z), 1.0f);
    float3 specular = 0;
    const float2 animatedUV = liquidUV(uv, p.textureTransform[0]);
    float2 baseUV = animatedUV;
    if (liquidMaterial != 2) {
        baseUV = liquidUV(layerUV, p.textureTransform[1]);
        const float3 direction = (p.worldView * float4(normal, 0)).xyz;
        float3 lighting = fma(saturate(dot(-p.sunDirection.xyz, direction)), p.diffuse.rgb, p.ambient.rgb);
        for (uint i = 0; i < liquidLightCount; ++i) {
            const float3 delta = p.lights[i].position.xyz - view.xyz;
            const float inverseDistance = rsqrt(dot(delta, delta)), distance = 1 / inverseDistance;
            const float3 attenuation = p.lights[i].attenuation.xyz;
            const float divisor = fma(distance * distance, attenuation.z, fma(attenuation.y, distance, attenuation.x));
            lighting = fma(saturate(dot(delta * inverseDistance, direction)) * p.lights[i].color.rgb, float3(1 / divisor), lighting);
        }
        color.rgb *= lighting;
        if (liquidMaterial == 0) {
            const float3 halfway = normalize(normalize(view.xyz) + p.sunDirection.xyz);
            specular = pow(max(dot(-halfway, direction), 0.0f), p.specular.a) * p.specular.rgb;
        }
    }
    return {p.projection * view, color, specular, baseUV, animatedUV, fog, 1};
}
struct LiquidWaterVertex {
    float3 position [[attribute(0)]], normal [[attribute(1)]];
    float4 color [[attribute(2)]];
    float2 uv [[attribute(3)]], layerUV [[attribute(4)]];
};
struct LiquidMagmaVertex { float3 position [[attribute(0)]]; float4 color [[attribute(2)]]; float2 uv [[attribute(3)]]; };
vertex LiquidRaster liquidWaterVertex(LiquidWaterVertex v [[stage_in]], constant LiquidVertexParameters& p [[buffer(0)]]) {
    return liquidTransform(v.position, v.normal, v.color, v.uv, v.layerUV, p);
}
vertex LiquidRaster liquidMagmaVertex(LiquidMagmaVertex v [[stage_in]], constant LiquidVertexParameters& p [[buffer(0)]]) {
    return liquidTransform(v.position, float3(0), v.color, v.uv, float2(0), p);
}
fragment float4 liquidFragment(LiquidRaster v [[stage_in]], constant LiquidFragmentParameters& p [[buffer(0)]],
    texture2d<float> image [[texture(0)]], sampler filtering [[sampler(0)]],
    texture2d<float> layerTexture [[texture(1)]], sampler layerFiltering [[sampler(1)]]) {
    const float4 base = decoded(image.sample(filtering, v.uv), p.decodeScale[0]);
    const float alpha = liquidMaterial == 2 ? 1 : base.a * v.color.a;
    if (alphaTest && alpha < p.stateAlphaRef) discard_fragment();
    float3 shaded;
    if (liquidMaterial == 2) shaded = fma(v.color.rgb, base.rgb, -p.fogColor.rgb);
    else {
        const float4 layer = decoded(layerTexture.sample(layerFiltering, v.layerUV), p.decodeScale[1]);
        shaded = fma(v.color.rgb, base.rgb, layer.rgb);
        if (liquidMaterial == 0) shaded += (v.specular + 0.25f) * layer.a;
        shaded -= p.fogColor.rgb;
    }
    return float4(fma(float3(v.fog), shaded, p.fogColor.rgb), alpha);
}
