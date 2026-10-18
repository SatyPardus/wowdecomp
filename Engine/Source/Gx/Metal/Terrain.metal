// Original terrain surface lighting, environment reflection and projected shadows.
constant uint terrainVertexPermutation [[function_constant(12)]];
constant uint terrainPixelPermutation [[function_constant(13)]];
constant uint terrainShadowMode [[function_constant(24)]];
constant uint terrainLayerPass [[function_constant(25)]];
struct TerrainPointLight {
    float4 position, color, attenuation;
};
struct TerrainVertexParameters {
    float4x4 worldView, projection;
    float4 fog, texOffset[5], texScale[5];
    float4 position, sunDirection, ambient, diffuse, specular;
    TerrainPointLight lights[3];
    float4 shadowRows[12], environmentRotation[3];
};
struct TerrainFragmentParameters {
    float4 fogColor, layerLighting, decodeScale[5], pcfOffsets[8], shadowDecodeScale, layerChannel;
    float stateAlphaRef;
    float padding[3];
};
struct TerrainVertex {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
};
struct TerrainColorVertex {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float4 color [[attribute(2)]];
};
struct TerrainRaster {
    float4 position [[position]];
    float3 diffuse, specular;
    float2 uv0, uv1, uv2, uv3, uv4;
    float fog;
    float pointSize [[point_size]];
    float3 dynamicShadow, nearShadow, middleShadow, farShadow;
    float3 environmentDirection;
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
            const float attenuation =
                fma(distance * distance, p.lights[i].attenuation.z, fma(distance, p.lights[i].attenuation.y, p.lights[i].attenuation.x));
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
    for (uint i = 0; i < 5; ++i)
        uv[i] = fma(relative, p.texScale[i].yx, p.texOffset[i].yx);
    const float fog = min(pow(max(fma(view.z, p.fog.x, p.fog.y), 0.0f), p.fog.z), 1.0f);
    TerrainRaster result{clip, min(lighting, 1.0f) * color, specular, uv[0], uv[1], uv[2], uv[3], uv[4], fog, 1};
    if (terrainVertexPermutation & 2) {
        const float3 reflected = fma(direction, -2 * dot(view.xyz, direction), view.xyz);
        result.environmentDirection = fma(reflected.z, p.environmentRotation[2].xzy,
                                          fma(reflected.x, p.environmentRotation[0].xzy, reflected.y * p.environmentRotation[1].xzy));
    }
    if (terrainVertexPermutation & 1) {
        // Each shadow projection is three affine rows, with no perspective
        // division. Keep four named coordinates rather than D3D's packed varyings.
        result.dynamicShadow = float3(dot(view, p.shadowRows[0]), dot(view, p.shadowRows[1]), dot(view, p.shadowRows[2]));
        result.nearShadow = float3(dot(view, p.shadowRows[3]), dot(view, p.shadowRows[4]), dot(view, p.shadowRows[5]));
        result.middleShadow = float3(dot(view, p.shadowRows[6]), dot(view, p.shadowRows[7]), dot(view, p.shadowRows[8]));
        result.farShadow = float3(dot(view, p.shadowRows[9]), dot(view, p.shadowRows[10]), dot(view, p.shadowRows[11]));
    }
    return result;
}
vertex TerrainRaster terrainVertex(TerrainVertex v [[stage_in]], constant TerrainVertexParameters& p [[buffer(0)]]) {
    return terrainTransform(v.position, v.normal, float3(0.5f), p);
}
vertex TerrainRaster terrainColorVertex(TerrainColorVertex v [[stage_in]], constant TerrainVertexParameters& p [[buffer(0)]]) {
    return terrainTransform(v.position, v.normal, v.color.rgb, p);
}
// The authored kernels sample LOD zero, compare after sampling (equality is
// lit), then average. Hardware comparison filtering would change this behavior.
inline float terrainShadowSamples(float3 coordinate, texture2d<float> image, sampler filtering, constant TerrainFragmentParameters& p,
                                  uint band, bool nineTaps) {
    const float2 uv = fma(coordinate.xy, 0.5f, 0.5f);
    const float scale = decodePacked ? p.shadowDecodeScale[band] : 1;
    float visible = image.sample(filtering, uv, level(0)).r * scale >= coordinate.z ? 1.0f : 0.0f;
    for (uint i = 0; i < 8; i += nineTaps ? 1 : 2)
        visible += image.sample(filtering, uv + p.pcfOffsets[i].xy, level(0)).r * scale >= coordinate.z ? 1.0f : 0.0f;
    return visible * (nineTaps ? 1.0f / 9.0f : 0.2f);
}
inline float terrainShadowBorder(float3 coordinate, bool farBand) {
    const float edge = max(abs(coordinate.x), abs(coordinate.y));
    return farBand ? saturate(fma(edge, -11.11111068725586f, 11.0f)) : saturate(fma(edge, -3.4482758045196533f, 3.4137930870056152f));
}
inline float terrainBandedShadow(TerrainRaster v, constant TerrainFragmentParameters& p, texture2d<float> nearImage, sampler nearSampler,
                                 texture2d<float> middleImage, sampler middleSampler, texture2d<float> farImage, sampler farSampler) {
    if (max(abs(v.nearShadow.x), abs(v.nearShadow.y)) < 1)
        return terrainShadowSamples(v.nearShadow, nearImage, nearSampler, p, 1, false);
    if (max(abs(v.middleShadow.x), abs(v.middleShadow.y)) < 1)
        return terrainShadowSamples(v.middleShadow, middleImage, middleSampler, p, 2, false);
    const float farVisibility = terrainShadowSamples(v.farShadow, farImage, farSampler, p, 3, false);
    return fma(terrainShadowBorder(v.farShadow, true), farVisibility - 1, 1.0f);
}
inline float4 terrainShade(TerrainRaster v, constant TerrainFragmentParameters& p, float4 base, texture2d<float> image1, sampler sampler1,
                           texture2d<float> image2, sampler sampler2, texture2d<float> image3, sampler sampler3, texture2d<float> image4,
                           sampler sampler4, texture2d<float> dynamicImage, sampler dynamicSampler, texture2d<float> nearImage,
                           sampler nearSampler, texture2d<float> middleImage, sampler middleSampler, texture2d<float> farImage,
                           sampler farSampler) {
    const uint layers = 1 + ((terrainPixelPermutation >> 1) & 3);
    float4 texels[5];
    texels[0] = decoded(base, p.decodeScale[0]);
    texels[1] = 0;
    if (layers > 1 || terrainShadowMode < 2 || (terrainPixelPermutation & 8))
        texels[1] = decoded(image1.sample(sampler1, v.uv1), p.decodeScale[1]);
    if (layers >= 2)
        texels[2] = decoded(image2.sample(sampler2, v.uv2), p.decodeScale[2]);
    if (layers >= 3)
        texels[3] = decoded(image3.sample(sampler3, v.uv3), p.decodeScale[3]);
    if (layers >= 4)
        texels[4] = decoded(image4.sample(sampler4, v.uv4), p.decodeScale[4]);
    const float4 blend = texels[layers];
    if (terrainPixelPermutation & 16) {
        for (uint i = 0; i < layers; ++i)
            if (p.layerLighting[i] != 0)
                texels[i].rgb *= v.diffuse + v.diffuse;
    }
    float4 mixed;
    if (terrainPixelPermutation & 8) {
        const float baseWeight = 1 - saturate(dot(blend.rgb, float3(1)));
        mixed = layers == 1 ? texels[0] * baseWeight : fma(texels[0], baseWeight, texels[1] * blend.r);
        if (layers >= 3)
            mixed = fma(texels[2], blend.g, mixed);
        if (layers >= 4)
            mixed = fma(texels[3], blend.b, mixed);
    } else {
        mixed = texels[0];
        for (uint i = 1; i < layers; ++i)
            mixed = mix(mixed, texels[i], blend[i - 1]);
    }
    float visibility = blend.a;
    if (terrainShadowMode) {
        const float border = terrainShadowBorder(v.dynamicShadow, false);
        if (terrainShadowMode == 3) {
            // Highest quality selects the nine-tap close map while inside its
            // footprint; unlike the other modes it neither fades nor takes min
            // with the distant bands in this region.
            visibility = border > 0 ? terrainShadowSamples(v.dynamicShadow, dynamicImage, dynamicSampler, p, 0, true)
                                    : terrainBandedShadow(v, p, nearImage, nearSampler, middleImage, middleSampler, farImage, farSampler);
        } else {
            const float dynamic = fma(border, terrainShadowSamples(v.dynamicShadow, dynamicImage, dynamicSampler, p, 0, false) - 1, 1.0f);
            visibility = min(dynamic, terrainShadowMode == 1 ? blend.a
                                                             : terrainBandedShadow(v, p, nearImage, nearSampler, middleImage, middleSampler,
                                                                                   farImage, farSampler));
        }
    }
    const float shadow = fma(visibility, 0.3f, 0.7f);
    float3 lit;
    if (terrainPixelPermutation & 16)
        lit = fma(mixed.rgb, shadow, (mixed.a * v.specular) * visibility);
    else {
        const float3 diffuse = (mixed.rgb * shadow) * v.diffuse;
        lit = fma(mixed.a * v.specular, visibility, diffuse + diffuse);
    }
    float alpha = 1;
    if (terrainLayerPass == 1)
        alpha = dot(blend.rgb, p.layerChannel.rgb);
    // Terrain0 weights the fully lit color before fog. The old-alpha path
    // instead exposes that layer's weight as output alpha for Gx blending.
    if (terrainLayerPass == 2)
        lit *= 1 - saturate(dot(blend.rgb, float3(1)));
    if (terrainLayerPass == 3)
        lit *= dot(blend.rgb, p.layerChannel.rgb);
    if (alphaTest && alpha < p.stateAlphaRef)
        discard_fragment();
    return float4(fma(float3(v.fog), lit - p.fogColor.rgb, p.fogColor.rgb), alpha);
}

struct CubeFaceCoordinate {
    float2 uv, dx, dy;
    uint face;
};
inline CubeFaceCoordinate terrainCubeCoordinate(float3 direction) {
    const float3 dx = dfdx(direction), dy = dfdy(direction), magnitude = abs(direction);
    float3 projected, px, py;
    uint face;
    if (magnitude.x >= magnitude.y && magnitude.x >= magnitude.z) {
        const float sign = direction.x >= 0 ? 1.0f : -1.0f;
        face = direction.x >= 0 ? 0 : 1;
        projected = float3(-sign * direction.z, -direction.y, sign * direction.x);
        px = float3(-sign * dx.z, -dx.y, sign * dx.x);
        py = float3(-sign * dy.z, -dy.y, sign * dy.x);
    } else if (magnitude.y >= magnitude.z) {
        const float sign = direction.y >= 0 ? 1.0f : -1.0f;
        face = direction.y >= 0 ? 2 : 3;
        projected = float3(direction.x, sign * direction.z, sign * direction.y);
        px = float3(dx.x, sign * dx.z, sign * dx.y);
        py = float3(dy.x, sign * dy.z, sign * dy.y);
    } else {
        const float sign = direction.z >= 0 ? 1.0f : -1.0f;
        face = direction.z >= 0 ? 4 : 5;
        projected = float3(sign * direction.x, -direction.y, sign * direction.z);
        px = float3(sign * dx.x, -dx.y, sign * dx.z);
        py = float3(sign * dy.x, -dy.y, sign * dy.z);
    }
    // Direction zero has no defined cube face. Keep coordinates finite without
    // changing the projection of any nonzero direction.
    const float inverseMajor = projected.z > 0 ? 1 / projected.z : 0;
    const float2 ratio = projected.xy * inverseMajor;
    return {ratio * 0.5f + 0.5f, (px.xy - ratio * px.z) * (0.5f * inverseMajor), (py.xy - ratio * py.z) * (0.5f * inverseMajor), face};
}

fragment float4 terrainFragment(TerrainRaster v [[stage_in]], constant TerrainFragmentParameters& p [[buffer(0)]],
                                texture2d<float> image0 [[texture(0)]], sampler sampler0 [[sampler(0)]],
                                texture2d<float> image1 [[texture(1)]], sampler sampler1 [[sampler(1)]],
                                texture2d<float> image2 [[texture(2)]], sampler sampler2 [[sampler(2)]],
                                texture2d<float> image3 [[texture(3)]], sampler sampler3 [[sampler(3)]],
                                texture2d<float> image4 [[texture(4)]], sampler sampler4 [[sampler(4)]],
                                texture2d<float> dynamicImage [[texture(5)]], sampler dynamicSampler [[sampler(5)]],
                                texture2d<float> nearImage [[texture(6)]], sampler nearSampler [[sampler(6)]],
                                texture2d<float> middleImage [[texture(7)]], sampler middleSampler [[sampler(7)]],
                                texture2d<float> farImage [[texture(8)]], sampler farSampler [[sampler(8)]]) {
    const float4 base = image0.sample(sampler0, v.uv0);
    return terrainShade(v, p, base, image1, sampler1, image2, sampler2, image3, sampler3, image4, sampler4, dynamicImage, dynamicSampler,
                        nearImage, nearSampler, middleImage, middleSampler, farImage, farSampler);
}

fragment float4 terrainEnvironmentFragment(TerrainRaster v [[stage_in]], constant TerrainFragmentParameters& p [[buffer(0)]],
                                           texture2d_array<float> image0 [[texture(0)]], sampler sampler0 [[sampler(0)]],
                                           texture2d<float> image1 [[texture(1)]], sampler sampler1 [[sampler(1)]],
                                           texture2d<float> image2 [[texture(2)]], sampler sampler2 [[sampler(2)]],
                                           texture2d<float> image3 [[texture(3)]], sampler sampler3 [[sampler(3)]],
                                           texture2d<float> image4 [[texture(4)]], sampler sampler4 [[sampler(4)]],
                                           texture2d<float> dynamicImage [[texture(5)]], sampler dynamicSampler [[sampler(5)]],
                                           texture2d<float> nearImage [[texture(6)]], sampler nearSampler [[sampler(6)]],
                                           texture2d<float> middleImage [[texture(7)]], sampler middleSampler [[sampler(7)]],
                                           texture2d<float> farImage [[texture(8)]], sampler farSampler [[sampler(8)]]) {
    const auto coord = terrainCubeCoordinate(v.environmentDirection);
    // D3D9 applies the requested address/filter mode within the selected face;
    // Metal's seamless texturecube sampling would blend neighboring faces.
    const float4 base = image0.sample(sampler0, coord.uv, coord.face, gradient2d(coord.dx, coord.dy));
    return terrainShade(v, p, base, image1, sampler1, image2, sampler2, image3, sampler3, image4, sampler4, dynamicImage, dynamicSampler,
                        nearImage, nearSampler, middleImage, middleSampler, farImage, farSampler);
}
