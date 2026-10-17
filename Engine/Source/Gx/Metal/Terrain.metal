// Original Terrain/Terrain1 surface semantics. Projected shadows and cube
// reflection have separate permutations and are not substituted by this path.
constant uint terrainVertexPermutation [[function_constant(12)]];
constant uint terrainPixelPermutation [[function_constant(13)]];
struct TerrainPointLight { float4 position, color, attenuation; };
struct TerrainVertexParameters {
    float4x4 worldView, projection;
    float4 fog, texOffset[5], texScale[5];
    float4 position, sunDirection, ambient, diffuse, specular;
    TerrainPointLight lights[3];
};
struct TerrainFragmentParameters {
    float4 fogColor, layerLighting, decodeScale[5];
    float stateAlphaRef; float padding[3];
};
struct TerrainVertex { float3 position [[attribute(0)]]; float3 normal [[attribute(1)]]; };
struct TerrainColorVertex {
    float3 position [[attribute(0)]]; float3 normal [[attribute(1)]]; float4 color [[attribute(2)]];
};
struct TerrainRaster {
    float4 position [[position]];
    float3 diffuse, specular;
    float2 uv0, uv1, uv2, uv3, uv4;
    float fog; float pointSize [[point_size]];
};
inline float3 terrainRotate(float3 point, float4x4 matrix) {
    return fma(point.z, matrix[2].xyz, fma(point.x, matrix[0].xyz, point.y * matrix[1].xyz));
}
inline TerrainRaster terrainTransform(float3 position, float3 normal, float3 color, constant TerrainVertexParameters& p) {
    const float4 view = fma(position.z, p.worldView[2], fma(position.x, p.worldView[0], position.y * p.worldView[1])) + p.worldView[3];
    const float4 clip = fma(view.w, p.projection[3], fma(view.z, p.projection[2], fma(view.x, p.projection[0], view.y * p.projection[1])));
    // Terrain's original vertex shader deliberately leaves this normal unnormalized.
    const float3 direction = terrainRotate(normal, p.worldView);
    float3 lighting = fma(p.diffuse.rgb, saturate(dot(p.sunDirection.xyz, direction)), p.ambient.rgb);
    if (terrainVertexPermutation & 64) {
        for (uint i = 0; i < 3; ++i) {
            const float3 delta = terrainRotate(p.lights[i].position.xyz, p.worldView) - view.xyz;
            const float inverseDistance = rsqrt(dot(delta, delta));
            const float distance = 1 / inverseDistance;
            const float diffuse = saturate(dot(delta * inverseDistance, direction));
            const float attenuation = fma(distance * distance, p.lights[i].attenuation.z,
                fma(distance, p.lights[i].attenuation.y, p.lights[i].attenuation.x));
            lighting = fma(p.lights[i].color.rgb * diffuse, 1 / attenuation, lighting);
        }
    }
    float3 specular = 0;
    if (terrainVertexPermutation & 8) {
        const float3 halfway = normalize(fma(-view.xyz, rsqrt(dot(view.xyz, view.xyz)), p.sunDirection.xyz));
        specular = pow(max(dot(halfway, direction), 0.0f), p.specular.w) * p.specular.rgb;
    }
    const float2 relative = position.yx - p.position.yx;
    float2 uv[5];
    for (uint i = 0; i < 5; ++i) uv[i] = fma(relative, p.texScale[i].yx, p.texOffset[i].yx);
    const float fog = min(pow(max(fma(view.z, p.fog.x, p.fog.y), 0.0f), p.fog.z), 1.0f);
    return {clip, min(lighting, 1.0f) * color, specular, uv[0], uv[1], uv[2], uv[3], uv[4], fog, 1};
}
vertex TerrainRaster terrainVertex(TerrainVertex v [[stage_in]], constant TerrainVertexParameters& p [[buffer(0)]]) {
    return terrainTransform(v.position, v.normal, float3(0.5f), p);
}
vertex TerrainRaster terrainColorVertex(TerrainColorVertex v [[stage_in]], constant TerrainVertexParameters& p [[buffer(0)]]) {
    return terrainTransform(v.position, v.normal, v.color.rgb, p);
}
fragment float4 terrainFragment(TerrainRaster v [[stage_in]], constant TerrainFragmentParameters& p [[buffer(0)]],
    texture2d<float> image0 [[texture(0)]], sampler sampler0 [[sampler(0)]],
    texture2d<float> image1 [[texture(1)]], sampler sampler1 [[sampler(1)]],
    texture2d<float> image2 [[texture(2)]], sampler sampler2 [[sampler(2)]],
    texture2d<float> image3 [[texture(3)]], sampler sampler3 [[sampler(3)]],
    texture2d<float> image4 [[texture(4)]], sampler sampler4 [[sampler(4)]]) {
    const uint layers = 1 + ((terrainPixelPermutation >> 1) & 3);
    float4 texels[5];
    texels[0] = decoded(image0.sample(sampler0, v.uv0), p.decodeScale[0]);
    texels[1] = decoded(image1.sample(sampler1, v.uv1), p.decodeScale[1]);
    if (layers >= 2) texels[2] = decoded(image2.sample(sampler2, v.uv2), p.decodeScale[2]);
    if (layers >= 3) texels[3] = decoded(image3.sample(sampler3, v.uv3), p.decodeScale[3]);
    if (layers >= 4) texels[4] = decoded(image4.sample(sampler4, v.uv4), p.decodeScale[4]);
    const float4 blend = texels[layers];
    if (terrainPixelPermutation & 16) {
        for (uint i = 0; i < layers; ++i)
            if (p.layerLighting[i] != 0) texels[i].rgb *= v.diffuse + v.diffuse;
    }
    float4 mixed;
    if (terrainPixelPermutation & 8) {
        const float baseWeight = 1 - saturate(dot(blend.rgb, float3(1)));
        mixed = layers == 1 ? texels[0] * baseWeight : fma(texels[0], baseWeight, texels[1] * blend.r);
        if (layers >= 3) mixed = fma(texels[2], blend.g, mixed);
        if (layers >= 4) mixed = fma(texels[3], blend.b, mixed);
    } else {
        mixed = texels[0];
        for (uint i = 1; i < layers; ++i) mixed = mix(mixed, texels[i], blend[i - 1]);
    }
    const float shadow = fma(blend.a, 0.3f, 0.7f);
    float3 lit;
    if (terrainPixelPermutation & 16) lit = fma(mixed.rgb, shadow, (mixed.a * v.specular) * blend.a);
    else {
        const float3 diffuse = (mixed.rgb * shadow) * v.diffuse;
        lit = fma(mixed.a * v.specular, blend.a, diffuse + diffuse);
    }
    if (alphaTest && 1.0f < p.stateAlphaRef) discard_fragment();
    return float4(fma(float3(v.fog), lit - p.fogColor.rgb, p.fogColor.rgb), 1);
}
