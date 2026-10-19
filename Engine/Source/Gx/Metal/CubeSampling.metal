// D3D9 cube-face addressing and derivatives; filter within the selected face.
struct CubeFaceCoordinate {
    float2 uv, dx, dy;
    uint face;
};
inline CubeFaceCoordinate cubeFaceCoordinate(float3 direction) {
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

