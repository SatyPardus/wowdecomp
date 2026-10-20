// The four original vsLiquidProcWater programs are byte-identical. Wave
// samples perturb the tangent basis, not the mesh; local lights are unused.
struct ProceduralWaterVertexParameters {
    float4x4 projection, worldView;
    LiquidUVTransform textureTransform[6];
    float4 fog, radialPhase, planarPhase;
    float4 radialWaves[3], radialFalloff, planarFalloff;
    float4 planarWaves[3], planarFrequency, planarAmplitude;
};
struct ProceduralWaterFragmentParameters {
    float4 fogColor, cameraPosition, sunDirection, specular;
    float4 optical, fresnel, decodeScale[6];
    float stateAlphaRef;
    float padding[3];
};
struct ProceduralWaterVertex {
    float3 position [[attribute(0)]];
    float2 uv [[attribute(3)]], depthUV [[attribute(4)]];
};
struct ProceduralWaterRaster {
    float4 position [[position]];
    float4 nearNormalUV, farNormalUV;
    float2 depthUV, highlightUV;
    float3 localPosition, tangent, bitangent, normal;
    float fog;
    float pointSize [[point_size]];
};
inline float waterWaveSine(float phase) {
    // Keep the original wrapped phase before SINCOS, including large phases.
    return sin(fma(fract(fma(phase, 0.15915493667125702f, 0.5f)), 6.2831854820251465f, -3.1415927410125732f));
}
vertex ProceduralWaterRaster proceduralWaterVertex(ProceduralWaterVertex v [[stage_in]],
    constant ProceduralWaterVertexParameters& p [[buffer(0)]]) {
    // Height at the vertex and the two half-unit offsets used by the BLS.
    float3 heights = 0;
    for (uint i = 0; i < 3; ++i) {
        const float4 wave = p.radialWaves[i];
        const float2 delta = v.position.xy - wave.xy;
        const float3 distances(length(delta), length(delta + float2(0.5f, 0)), length(delta + float2(0, 0.5f)));
        const float3 amplitudes = saturate(fma(distances, -p.radialFalloff[i], 1.0f)) * wave.w;
        for (uint sample = 0; sample < 3; ++sample) {
            // Base radial phase uses MUL then SUB; the offset samples use MAD.
            const float phase = sample ? fma(distances[sample], wave.z, -p.radialPhase[i]) :
                distances[sample] * wave.z - p.radialPhase[i];
            heights[sample] = fma(waterWaveSine(phase), amplitudes[sample], heights[sample]);
        }
    }
    for (uint i = 0; i < 3; ++i) {
        const float4 wave = p.planarWaves[i];
        const float2 delta = v.position.xy - wave.xy;
        const float3 distances(length(delta), length(delta + float2(0.5f, 0)), length(delta + float2(0, 0.5f)));
        const float3 amplitudes = saturate(fma(distances, -p.planarFalloff[i], 1.0f)) * p.planarAmplitude[i];
        const float2 scaled = v.position.xy * p.planarFrequency[i];
        // Authored offsets are added AFTER the frequency scale.
        const float3 phases(dot(scaled, wave.zw), dot(scaled + float2(0.5f, 0), wave.zw),
                            dot(scaled + float2(0, 0.5f), wave.zw));
        for (uint sample = 0; sample < 3; ++sample)
            heights[sample] = fma(waterWaveSine(phases[sample] - p.planarPhase[i]), amplitudes[sample], heights[sample]);
    }
    ProceduralWaterRaster out;
    out.tangent = normalize(float3(1, 0, heights.y - heights.x));
    out.bitangent = normalize(float3(0, 1, heights.z - heights.x));
    out.normal = cross(out.tangent, out.bitangent);
    const float4 view = p.worldView * float4(v.position, 1);
    out.position = p.projection * view;
    out.localPosition = v.position;
    out.fog = min(pow(max(fma(view.z, p.fog.x, p.fog.y), 0.0f), p.fog.z), 1.0f);
    out.nearNormalUV = float4(liquidUV(v.uv, p.textureTransform[0]), liquidUV(v.uv, p.textureTransform[1]));
    out.farNormalUV = float4(liquidUV(v.uv, p.textureTransform[2]), liquidUV(v.uv, p.textureTransform[3]));
    out.depthUV = liquidUV(v.depthUV, p.textureTransform[4]);
    out.highlightUV = liquidUV(v.uv, p.textureTransform[5]);
    out.pointSize = 1;
    return out;
}
inline float3 waterNormal(float2 encoded) {
    const float2 xy = fma(encoded, 2.0f, -1.0f);
    // D3D RSQ takes abs; preserve out-of-disk encoded normals as well.
    // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/rsq---vs
    return float3(xy, sqrt(abs(1.0f - dot(xy, xy))));
}
fragment float4 proceduralWaterFragment(ProceduralWaterRaster v [[stage_in]],
    constant ProceduralWaterFragmentParameters& p [[buffer(0)]],
    texture2d_array<float> refraction [[texture(0)]], sampler refractionSampler [[sampler(0)]],
    texture2d_array<float> reflection [[texture(1)]], sampler reflectionSampler [[sampler(1)]],
    texture2d<float> depthColor [[texture(2)]], sampler depthSampler [[sampler(2)]],
    texture2d<float> highlight [[texture(3)]], sampler highlightSampler [[sampler(3)]],
    texture2d<float> nearNormals [[texture(4)]], sampler nearSampler [[sampler(4)]],
    texture2d<float> farNormals [[texture(5)]], sampler farSampler [[sampler(5)]]) {
    const float4 near0 = decoded(nearNormals.sample(nearSampler, v.nearNormalUV.xy), p.decodeScale[4]);
    const float4 near1 = decoded(nearNormals.sample(nearSampler, v.nearNormalUV.zw), p.decodeScale[4]);
    const float4 far0 = decoded(farNormals.sample(farSampler, v.farNormalUV.xy), p.decodeScale[5]);
    const float4 far1 = decoded(farNormals.sample(farSampler, v.farNormalUV.zw), p.decodeScale[5]);
    // Each normal texture stores two XY pairs in RG and AB (in that order).
    const float3 nearNormal = normalize(waterNormal(near0.xy) + waterNormal(near1.wz));
    const float3 farNormal = normalize(waterNormal(far0.xy) + waterNormal(far1.wz));
    const float3 delta = v.localPosition - p.cameraPosition.xyz;
    const float distance = length(delta.xy);
    const float blend = saturate(fma(distance, p.optical.x, 0.5f));
    const float3 tangentNormal = fma(blend, nearNormal - farNormal, farNormal);
    // Preserve the original dot-product basis and horizontal eye-distance
    // normalization. Substituting a conventional TBN/normalize changes it.
    const float3 normal(dot(v.tangent, tangentNormal), dot(v.bitangent, tangentNormal), dot(v.normal, tangentNormal));
    const float3 eye = delta / distance;
    const float3 halfway = normalize(eye + p.sunDirection.xyz);
    const float3 specular = pow(saturate(dot(halfway, normal)), p.specular.w) * p.specular.xyz;
    const float facing = 1.0f - saturate(dot(-eye, normal));
    const float fresnel = facing * facing;
    const float4 depth = decoded(depthColor.sample(depthSampler, v.depthUV), p.decodeScale[2]);
    const float alpha = fma(p.fresnel.x, fresnel, depth.a);
    const float eyeNormal = dot(eye, normal);
    const float discriminant = fma(p.optical.y * p.optical.y, -fma(-eyeNormal, eyeNormal, 1.0f), 1.0f);
    const float transmitted = discriminant >= 0.0f ? 1.0f : 0.0f;
    const float eta = transmitted * p.optical.y;
    const float bend = fma(eta, eyeNormal, sqrt(discriminant * transmitted));
    const float3 reflected = fma(normal.xzy, -(eyeNormal + eyeNormal), eye.xzy);
    const float3 refracted = fma(eta, eye.xzy, -normal.xzy * bend);
    const auto refractedUV = cubeFaceCoordinate(refracted), reflectedUV = cubeFaceCoordinate(reflected);
    const float3 below = decoded(refraction.sample(refractionSampler, refractedUV.uv, refractedUV.face,
        gradient2d(refractedUV.dx, refractedUV.dy)), p.decodeScale[0]).rgb + depth.rgb;
    const float3 above = decoded(reflection.sample(reflectionSampler, reflectedUV.uv, reflectedUV.face,
        gradient2d(reflectedUV.dx, reflectedUV.dy)), p.decodeScale[1]).rgb;
    const float reflectionWeight = fma(p.optical.w, fresnel, p.optical.z);
    const float3 environment = mix(below, above, reflectionWeight);
    const float highlightAlpha = decoded(highlight.sample(highlightSampler, v.highlightUV), p.decodeScale[3]).a;
    const float3 color = fma((specular * highlightAlpha) * 4.0f, fma(p.fresnel.z, fresnel, p.fresnel.y), environment);
    // Test after derivative-dependent cube sampling, as in the D3D alpha stage.
    if (alphaTest && alpha < p.stateAlphaRef) discard_fragment();
    return float4(fma(v.fog, color - p.fogColor.rgb, p.fogColor.rgb), alpha);
}
