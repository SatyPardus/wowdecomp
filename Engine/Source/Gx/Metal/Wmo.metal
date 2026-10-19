// WMO lighting and material semantics. Classic color modulates lighting;
// unified color carries baked lighting and adds scaled dynamic lighting.
constant uint wmoPermutation [[function_constant(14)]];
constant uint wmoVertexEffect [[function_constant(15)]];
constant uint wmoMaterial [[function_constant(16)]];
constant bool wmoShaderAlphaTest [[function_constant(17)]];
constant uint wmoShadowMode [[function_constant(36)]];
struct WmoVertexParameters {
    float4x4 projection;
    ModelAffine worldView;
    float4 ambient, sunDiffuse, sunDirection, specular, diffuse, emissive, fog;
    float4 lightColor[4], lightPosition[4];
    float4 attenuationConstant, attenuationLinear, attenuationQuadratic;
};
struct WmoRaster {
    float4 position [[position]];
    float4 color, secondaryColor;
    float2 uv, secondaryUV;
    float fog;
    float pointSize [[point_size]];
    float3 viewPosition, normal, shadowCoordinate, nearShadow, middleShadow, farShadow;
};
inline WmoRaster wmoTransform(float3 position, float3 normal, float4 color, float4 secondaryColor,
    float2 uv, float2 secondaryUV, constant WmoVertexParameters& p, constant ShadowReceiverParameters& shadow) {
    const bool lit = wmoPermutation & 1;
    const bool environment = wmoVertexEffect == 2 || wmoVertexEffect == 5;
    const bool specular = wmoVertexEffect == 1 || wmoVertexEffect == 4;
    const float4 point = float4(position, 1);
    const float3 view = float3(dot(p.worldView.x, point), dot(p.worldView.y, point), dot(p.worldView.z, point));
    float3 direction = 0;
    if (wmoShadowMode || lit || environment) direction = normalize(float3(dot(p.worldView.x.xyz, normal),
        dot(p.worldView.y.xyz, normal), dot(p.worldView.z.xyz, normal)));
    if (lit) {
        float3 lighting = fma(saturate(dot(-p.sunDirection.xyz, direction)), p.sunDiffuse.rgb, p.ambient.rgb);
        for (uint i = 0; i < (wmoPermutation / 2) % 5; ++i) {
            const float3 delta = p.lightPosition[i].xyz - view;
            const float distanceSquared = dot(delta, delta), inverseDistance = rsqrt(distanceSquared);
            const float attenuation = 1 / (fma(distanceSquared, p.attenuationQuadratic[i],
                distanceSquared * inverseDistance * p.attenuationLinear[i]) + p.attenuationConstant[i]);
            lighting = fma(p.lightColor[i].rgb, attenuation * max(dot(delta, direction) * inverseDistance, 0.0f), lighting);
        }
        lighting = saturate(lighting);
        color.rgb = saturate(wmoVertexEffect >= 3 ? fma(p.diffuse.rgb, lighting, color.rgb) + p.emissive.rgb :
            fma(color.rgb, lighting, p.emissive.rgb));
        color.a = saturate(color.a);
    }
    if (specular) {
        secondaryColor = lit ? float4(p.specular.rgb * pow(saturate(dot(direction,
            -normalize(normalize(view) + p.sunDirection.xyz))), p.specular.a), 1) : float4(0);
    }
    if (environment) {
        // WMO uses the XY of its reflected view vector directly. Model2's
        // sphere-map scale/bias is a different effect and does not apply here.
        const float3 incident = normalize(view);
        const float projection = dot(incident, direction);
        secondaryUV = fma(direction.xy, float2(-(projection + projection)), incident.xy);
    }
    const float fog = min(pow(max(fma(view.z, p.fog.x, p.fog.y), 0.0f), p.fog.z), 1.0f);
    WmoRaster result{p.projection * float4(view, 1), color, secondaryColor, uv, secondaryUV, fog, 1};
    if (wmoShadowMode) {
        result.viewPosition=view; result.normal=direction;
        result.shadowCoordinate=float3(dot(shadow.projection[0].x,float4(view,1)),
            dot(shadow.projection[0].y,float4(view,1)),dot(shadow.projection[0].z,float4(view,1)));
        if (wmoShadowMode >= 2) {
            result.nearShadow=float3(dot(shadow.projection[1].x,float4(view,1)),dot(shadow.projection[1].y,float4(view,1)),dot(shadow.projection[1].z,float4(view,1)));
            result.middleShadow=float3(dot(shadow.projection[2].x,float4(view,1)),dot(shadow.projection[2].y,float4(view,1)),dot(shadow.projection[2].z,float4(view,1)));
            result.farShadow=float3(dot(shadow.projection[3].x,float4(view,1)),dot(shadow.projection[3].y,float4(view,1)),dot(shadow.projection[3].z,float4(view,1)));
        }
    }
    return result;
}
struct WmoVertex { float3 position [[attribute(0)]]; float4 color [[attribute(2)]]; float2 uv [[attribute(3)]]; };
struct WmoLitVertex { float3 position [[attribute(0)]]; float3 normal [[attribute(1)]]; float4 color [[attribute(2)]]; float2 uv [[attribute(3)]]; };
struct WmoCompositeVertex {
    float3 position [[attribute(0)]]; float3 normal [[attribute(1)]];
    float4 color [[attribute(2)]]; float2 uv [[attribute(3)]];
    float4 secondaryColor [[attribute(4)]]; float2 secondaryUV [[attribute(5)]];
};
vertex WmoRaster wmoVertex(WmoVertex v [[stage_in]], constant WmoVertexParameters& p [[buffer(0)]], constant ShadowReceiverParameters& shadow [[buffer(3)]]) {
    return wmoTransform(v.position, float3(0), v.color, float4(0), v.uv, float2(0), p, shadow);
}
vertex WmoRaster wmoLitVertex(WmoLitVertex v [[stage_in]], constant WmoVertexParameters& p [[buffer(0)]], constant ShadowReceiverParameters& shadow [[buffer(3)]]) {
    return wmoTransform(v.position, v.normal, v.color, float4(0), v.uv, float2(0), p, shadow);
}
vertex WmoRaster wmoCompositeVertex(WmoCompositeVertex v [[stage_in]], constant WmoVertexParameters& p [[buffer(0)]], constant ShadowReceiverParameters& shadow [[buffer(3)]]) {
    return wmoTransform(v.position, v.normal, v.color, v.secondaryColor, v.uv, v.secondaryUV, p, shadow);
}
template <typename ShadowTexture>
inline float wmoShadowVisibility(WmoRaster v, constant ShadowReceiverParameters& shadow,
    ShadowTexture shadowImage, sampler shadowFiltering, ShadowTexture nearImage, sampler nearSampler,
    ShadowTexture middleImage, sampler middleSampler, ShadowTexture farImage, sampler farSampler) {
    float visibility=receiverDynamicVisibility(v.shadowCoordinate,v.viewPosition,shadow,shadowImage,shadowFiltering,wmoShadowMode);
    if (wmoShadowMode >= 2)
        visibility=min(visibility,receiverBandedVisibility(v.nearShadow,v.middleShadow,v.farShadow,shadow,nearImage,nearSampler,middleImage,middleSampler,farImage,farSampler));
    return receiverShadowModulation(visibility,v.normal,shadow.lightDirection.xyz);
}
fragment float4 wmoFragment(WmoRaster v [[stage_in]], constant ModelFragmentParameters& p [[buffer(0)]],
    texture2d<float> image [[texture(0)]], sampler filtering [[sampler(0)]],
    texture2d<float> layerTexture [[texture(1)]], sampler layerFiltering [[sampler(1)]],
    constant ShadowReceiverParameters& shadow [[buffer(3)]], texture2d<float> shadowImage [[texture(4), function_constant(shadowFloatSampling)]], sampler shadowFiltering [[sampler(4)]],
    texture2d<float> nearImage [[texture(5), function_constant(shadowFloatSampling)]], sampler nearSampler [[sampler(5)]],
    texture2d<float> middleImage [[texture(6), function_constant(shadowFloatSampling)]], sampler middleSampler [[sampler(6)]],
    texture2d<float> farImage [[texture(7), function_constant(shadowFloatSampling)]], sampler farSampler [[sampler(7)]],
    depth2d<float> shadowDepth [[texture(4), function_constant(shadowComparison)]],
    depth2d<float> nearDepth [[texture(5), function_constant(shadowComparison)]],
    depth2d<float> middleDepth [[texture(6), function_constant(shadowComparison)]],
    depth2d<float> farDepth [[texture(7), function_constant(shadowComparison)]]) {
    if (wmoShadowMode) {
        if(shadowComparison) v.color.rgb*=wmoShadowVisibility(v,shadow,shadowDepth,shadowFiltering,nearDepth,nearSampler,middleDepth,middleSampler,farDepth,farSampler);
        else v.color.rgb*=wmoShadowVisibility(v,shadow,shadowImage,shadowFiltering,nearImage,nearSampler,middleImage,middleSampler,farImage,farSampler);
    }
    float4 texel = decoded(image.sample(filtering, v.uv), p.decodeScale[0]);
    float4 layer = 0;
    if (wmoMaterial >= 4) layer = decoded(layerTexture.sample(layerFiltering, v.secondaryUV), p.decodeScale[1]);
    if (wmoMaterial == 6) texel = v.secondaryColor.a * texel + (1 - v.secondaryColor.a) * layer;
    const bool textureAlpha = wmoMaterial == 0 || wmoMaterial == 6;
    const float alpha = textureAlpha ? texel.a * v.color.a : v.color.a;
    const float shaderAlpha = textureAlpha ? fma(texel.a, v.color.a, -p.fogColorAlphaRef.a) : v.color.a - p.fogColorAlphaRef.a;
    if (wmoShaderAlphaTest && shaderAlpha < 0) discard_fragment();
    if (alphaTest && alpha < p.stateAlphaRef) discard_fragment();
    const float3 product = texel.rgb * v.color.rgb;
    float3 shaded;
    if (wmoMaterial == 4 || wmoMaterial == 5) {
        shaded = fma(wmoMaterial == 5 ? texel.a * texel.rgb : float3(texel.a), layer.rgb, product + product) - p.fogColorAlphaRef.rgb;
    } else {
        // The mounted 3.3.5a Specular and Metal pixel programs share Opaque's
        // color calculation: their vertex specular output is not consumed.
        shaded = fma(product, float3(2), -p.fogColorAlphaRef.rgb);
    }
    return float4(fma(float3(v.fog), shaded, p.fogColorAlphaRef.rgb), alpha);
}
