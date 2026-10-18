struct ShadowReceiverParameters {
    ModelAffine projection[4];
    float4 distanceFade, lightDirection, offsets[8], decodeScale;
};
// Compare after LOD-zero sampling; equality is lit. The model's highest
// quality path still uses five taps, unlike the terrain receiver.
inline float receiverShadowSamples(float3 coordinate, constant ShadowReceiverParameters& p,
    texture2d<float> image, sampler filtering, uint band, bool nine) {
    const float2 uv = fma(coordinate.xy, 0.5f, 0.5f);
    const float scale = decodePacked ? p.decodeScale[band] : 1;
    float samples = image.sample(filtering,uv,level(0)).r * scale >= coordinate.z ? 1.0f : 0.0f;
    for (uint i=0;i<8;i+=nine ? 1 : 2)
        samples += image.sample(filtering,uv+p.offsets[i].xy,level(0)).r * scale >= coordinate.z ? 1.0f : 0.0f;
    return samples * (nine ? 1.0f/9.0f : 0.2f);
}
// Distance fade only affects the dynamic map. Callers combine distant bands
// before the normal-facing fade and the original ambient contribution.
inline float receiverDynamicVisibility(float3 coordinate, float3 viewPosition, constant ShadowReceiverParameters& p,
    texture2d<float> image, sampler filtering, uint mode) {
    const float border = saturate(fma(max(abs(coordinate.x),abs(coordinate.y)), -3.4482758045196533f, 3.4137930870056152f));
    float visibility = 1;
    if (border > (mode == 3 ? 0.0f : 0.01f)) {
        const float samples = receiverShadowSamples(coordinate,p,image,filtering,0,mode != 3 && viewPosition.z >= 10);
        visibility = mode == 3 ? samples : min(fma(border,samples-1,1.0f),1.0f);
    }
    const float distanceFade = saturate(dot(viewPosition,p.distanceFade.xyz)+p.distanceFade.w);
    return mix(visibility,1.0f,distanceFade);
}
inline float receiverShadowModulation(float visibility, float3 normal, float3 lightDirection) {
    const float normalFade = 1.2f-abs(dot(normal,lightDirection));
    const float square = normalFade*normalFade;
    visibility = mix(visibility,1.0f,saturate(square*square));
    return fma(visibility,0.3f,0.7f);
}

inline float receiverBandedVisibility(float3 nearShadow, float3 middleShadow, float3 farShadow, constant ShadowReceiverParameters& p,
    texture2d<float> nearImage, sampler nearSampler, texture2d<float> middleImage, sampler middleSampler,
    texture2d<float> farImage, sampler farSampler) {
    if (max(abs(nearShadow.x),abs(nearShadow.y)) < 1)
        return receiverShadowSamples(nearShadow,p,nearImage,nearSampler,1,false);
    if (max(abs(middleShadow.x),abs(middleShadow.y)) < 1)
        return receiverShadowSamples(middleShadow,p,middleImage,middleSampler,2,false);
    const float visible = receiverShadowSamples(farShadow,p,farImage,farSampler,3,false);
    const float border = saturate(fma(max(abs(farShadow.x),abs(farShadow.y)), -11.11111068725586f, 11.0f));
    return fma(border,visible-1,1.0f);
}
