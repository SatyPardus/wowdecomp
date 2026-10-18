// Original ShadowMap / ShadowMapSL. Store linear light-view depth in RG;
// projection depth remains independent for visibility/depth-buffer testing.
constant uint shadowSkinning [[function_constant(21)]];
constant bool shadowCutout [[function_constant(22)]];
struct ShadowMapVertexParameters {
    float4x4 projection;
    ModelAffine lightFromView;
    float4 uv[2];
};
struct ShadowMapFragmentParameters {
    float depthScale, alphaRef, stateAlphaRef, padding;
    float4 decodeScale;
};
struct ShadowMapVertex { float3 position [[attribute(0)]]; float2 uv [[attribute(2), function_constant(shadowCutout)]]; };
struct ShadowMapSkinVertex {
    float3 position [[attribute(0)]];
    float2 uv [[attribute(2), function_constant(shadowCutout)]];
    float4 weights [[attribute(3)]];
    uint4 bones [[attribute(4)]];
};
struct ShadowMapRaster { float4 position [[position]]; float2 uv; float lightDepth; float pointSize [[point_size]]; };
inline ShadowMapRaster shadowMapTransform(float3 position, float2 uv, ModelAffine transform, constant ShadowMapVertexParameters& p) {
    const float4 point = float4(position, 1);
    float3 light = float3(dot(transform.x, point), dot(transform.y, point), dot(transform.z, point));
    // Rigid WMO batches already compose their world/view matrix. Skinned M2
    // palettes are in the scene view and need the separate light-view matrix.
    if (shadowSkinning != 0) {
        const float4 view = float4(light, 1);
        light = float3(dot(p.lightFromView.x, view), dot(p.lightFromView.y, view), dot(p.lightFromView.z, view));
    }
    const float4 texturePoint = float4(uv, 0, 1);
    return {p.projection * float4(light, 1), float2(dot(p.uv[0], texturePoint), dot(p.uv[1], texturePoint)), light.z, 1};
}
vertex ShadowMapRaster shadowMapRigidVertex(ShadowMapVertex v [[stage_in]], constant ShadowMapVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    return shadowMapTransform(v.position, shadowCutout ? v.uv : float2(0), palette[0], p);
}
vertex ShadowMapRaster shadowMapSkinVertex(ShadowMapSkinVertex v [[stage_in]], constant ShadowMapVertexParameters& p [[buffer(0)]],
    constant ModelAffine* palette [[buffer(2)]]) {
    return shadowMapTransform(v.position, shadowCutout ? v.uv : float2(0), skinTransform(v.weights, v.bones, palette, shadowSkinning == 2), p);
}
fragment float4 shadowMapFragment(ShadowMapRaster v [[stage_in]], constant ShadowMapFragmentParameters& p [[buffer(0)]],
    texture2d<float> image [[texture(0), function_constant(shadowCutout)]], sampler filtering [[sampler(0), function_constant(shadowCutout)]]) {
    if (shadowCutout) {
        const float alpha = decoded(image.sample(filtering, v.uv), p.decodeScale).a;
        if (alpha - p.alphaRef < 0) discard_fragment();
    }
    // Original shader output alpha is one; Gx state alpha testing is separate.
    if (alphaTest && 1.0f < p.stateAlphaRef) discard_fragment();
    const float depth = max(v.lightDepth * p.depthScale, 0.0f);
    return float4(depth, depth, 0, 1);
}
