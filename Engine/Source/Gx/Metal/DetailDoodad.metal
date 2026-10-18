// Original ground decoration lighting, terrain tint and distance fading.
constant bool detailPointLights [[function_constant(23)]];
struct DetailDoodadVertexParameters {
    float4x4 worldView, projection;
    float4 fog, fade, sunDirection, ambient, diffuse;
    TerrainPointLight lights[3];
};
struct DetailDoodadFragmentParameters {
    float4 fogColor, decodeScale;
    float stateAlphaRef;
    float padding[3];
};
struct DetailDoodadVertex {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float4 color [[attribute(2)]];
    float2 uv [[attribute(3)]];
};
struct DetailDoodadRaster {
    float4 position [[position]];
    float4 color;
    float2 uv;
    float fade, fog;
    float pointSize [[point_size]];
};
vertex DetailDoodadRaster detailDoodadVertex(DetailDoodadVertex v [[stage_in]], constant DetailDoodadVertexParameters& p [[buffer(0)]]) {
    const float4 view =
        fma(v.position.z, p.worldView[2], fma(v.position.x, p.worldView[0], v.position.y * p.worldView[1])) + p.worldView[3];
    const float4 clip = fma(view.w, p.projection[3], fma(view.z, p.projection[2], fma(view.x, p.projection[0], view.y * p.projection[1])));
    // Ground decorations use the same unnormalized terrain normal and light
    // coordinate convention as the terrain surface, including no translation
    // when transforming each local light position.
    const float3 normal = terrainRotate(v.normal, p.worldView);
    float3 lighting = fma(p.diffuse.rgb, saturate(dot(p.sunDirection.xyz, normal)), p.ambient.rgb);
    if (detailPointLights) {
        for (uint i = 0; i < 3; ++i) {
            const float3 delta = terrainRotate(p.lights[i].position.xyz, p.worldView) - view.xyz;
            const float inverseDistance = rsqrt(dot(delta, delta));
            const float distance = 1 / inverseDistance;
            const float diffuse = saturate(dot(delta * inverseDistance, normal));
            const float attenuation =
                fma(distance * distance, p.lights[i].attenuation.z, fma(distance, p.lights[i].attenuation.y, p.lights[i].attenuation.x));
            lighting = fma(p.lights[i].color.rgb * diffuse, 1 / attenuation, lighting);
        }
    }
    const float fade = saturate(fma(view.z, p.fade.x, p.fade.y));
    const float fog = min(pow(max(fma(view.z, p.fog.x, p.fog.y), 0.0f), p.fog.z), 1.0f);
    return {clip, float4(min(lighting, 1.0f) * v.color.rgb, v.color.a), v.uv, fade, fog, 1};
}
fragment float4 detailDoodadFragment(DetailDoodadRaster v [[stage_in]], constant DetailDoodadFragmentParameters& p [[buffer(0)]],
                                     texture2d<float> image [[texture(0)]], sampler filtering [[sampler(0)]]) {
    const float4 texel = decoded(image.sample(filtering, v.uv), p.decodeScale);
    const float alpha = texel.a * v.fade;
    if (alphaTest && alpha < p.stateAlphaRef)
        discard_fragment();
    const float3 tint = fma(v.color.a, 0.3f, 0.7f) * v.color.rgb;
    return float4(fma(float3(v.fog), fma(texel.rgb, tint, -p.fogColor.rgb), p.fogColor.rgb), alpha);
}
