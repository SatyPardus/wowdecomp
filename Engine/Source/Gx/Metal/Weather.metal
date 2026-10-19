// Original SM2 weather vertices feed the fixed texture/fog stages. Particle
// simulation, packet timing and collision remain in the original World code.
constant bool weatherPointSprite [[function_constant(41)]];
constant uint weatherKind [[function_constant(42)]];
constant bool weatherVelocity = weatherKind != 4;
constant bool weatherCorner = weatherKind >= 3;
struct WeatherParameters {
    float4x4 clipFromCamera;
    float4 camera, color, sizeFalloff, fog;
    float4 corners[3], uvCorners[3], widths[3], lengths[3], blur[3];
    float age, tailTime, minimumSize, maximumSize;
    float2 pixelToClip, padding;
};
struct WeatherInput {
    float3 position [[attribute(0)]];
    float4 corner [[attribute(1), function_constant(weatherCorner)]];
    float2 lifetime [[attribute(2)]];
    float3 velocity [[attribute(4), function_constant(weatherVelocity)]];
};
vertex FixedUnlitRaster weatherVertex(WeatherInput v [[stage_in]], uint vertexID [[vertex_id]], constant WeatherParameters& p [[buffer(0)]]) {
    FixedUnlitRaster out = {};
    float3 position;
    float alpha, size = 1;
    float2 uv = 0;
    if (weatherKind == 1) {
        const float movingAge = min(v.lifetime.y, p.age) - v.lifetime.x;
        position = fma(v.velocity, movingAge, v.position) - p.camera.xyz;
        size = max(saturate(fma(length(position), p.sizeFalloff.x, p.sizeFalloff.y)) * p.sizeFalloff.z, 1.0f);
        const float fade = saturate(1 - 4 * max(p.age - v.lifetime.y, 0.0f));
        // Before landing the original shader uses age, not a separate ramp.
        alpha = fade < 1 ? fade : movingAge;
    } else if (weatherKind == 2) {
        const float duration = v.lifetime.y - v.lifetime.x;
        const float movingAge = min(max(p.age - v.lifetime.x, 0.0f), duration);
        position = fma(v.velocity, movingAge, v.position) - p.camera.xyz;
        size = saturate(fma(length(position), p.sizeFalloff.x, p.sizeFalloff.y)) * p.sizeFalloff.z;
        const float inverseTail = 1 / (duration - (duration - 0.2f));
        alpha = (1 - saturate(1 - 5 * movingAge)) * saturate(fma(duration, inverseTail, -movingAge * inverseTail)) * p.color.a;
    } else {
        // Packet vertices encode the triangle corner in the alpha byte.
        const uint corner = min(uint(rint(v.corner.a * 255.001953125f)), 2u);
        if (weatherKind == 3) {
            const float end = v.lifetime.y + p.tailTime / -v.velocity.z;
            const float t = min(corner >= 2 ? end : v.lifetime.y, p.age) - v.lifetime.x;
            position = fma(v.velocity, t, v.position);
            const float3 toCamera = normalize((p.camera.xyz - position).zxy);
            const float3 direction = normalize(-v.velocity);
            const float3 side = normalize(toCamera.zxy * direction.zxy - toCamera * direction.yzx);
            const float3 offset = fma(side, p.widths[corner].xyz, direction * p.lengths[corner].xyz);
            position += fma(offset.z, p.blur[2].xyz, fma(offset.x, p.blur[0].xyz, offset.y * p.blur[1].xyz));
            position -= p.camera.xyz;
            uv = p.uvCorners[corner].xy;
            alpha = float(v.lifetime.x < p.age && p.age < end);
        } else {
            position = v.position + p.corners[corner].xyz - p.camera.xyz;
            const float facing = max(-position.z * rsqrt(dot(position, position)), 0.0f);
            position = fma(-0.5f * facing, p.corners[1].xyz, position);
            position.z = fma(facing, 1.0f / 12.0f, position.z);
            // The BLS constructs both atlas axes with truncation toward zero.
            uv = float2(trunc((p.age - v.lifetime.x) * 15.96f), trunc((1 - facing) * 3.99f)) * 0.25f + p.uvCorners[corner].xy;
            alpha = float(v.lifetime.x < p.age && p.age < v.lifetime.y);
        }
    }
    out.position = p.clipFromCamera * float4(position, 1);
    // D3D9 clamps vertex diffuse outputs before the fixed fragment stages.
    out.color = saturate(float4(p.color.rgb, alpha));
    out.fog = weatherKind >= 3 ? min(pow(max(fma(position.z, p.fog.x, p.fog.y), 0.0f), p.fog.z), 1.0f) : 1;
    out.pointSize = 1;
    if (weatherKind <= 2) {
        // Metal clips native points by their centers. D3D9 clips the expanded
        // square instead, so a partly visible flake must survive a center that
        // is outside the viewport. Instance the original packet as GPU quads.
        const float2 corners[6] = {float2(0,0),float2(1,0),float2(0,1),float2(0,1),float2(1,0),float2(1,1)};
        const float2 corner = corners[vertexID];
        const float pointSize = clamp(size, p.minimumSize, p.maximumSize);
        out.position.xy += (corner - 0.5f) * pointSize * p.pixelToClip * out.position.w;
        if (weatherPointSprite) uv = corner;
    }
    out.uv0 = out.uv1 = out.uv2 = out.uv3 = float3(uv, 1);
    return out;
}
