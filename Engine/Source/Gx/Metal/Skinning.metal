// Affine palette blending shared by surface and shadow passes.
struct ModelAffine { float4 x, y, z; };
inline ModelAffine skinTransform(float4 weights, uint4 bones, constant ModelAffine* palette, bool fourWeights) {
    ModelAffine transform = palette[bones.x];
    if (fourWeights) {
        // Match the original's y,x,z,w weighted-matrix association; do not
        // normalize weights or blend already-normalized vertex normals.
        transform.x = fma(palette[bones.x].x, weights.x, palette[bones.y].x * weights.y);
        transform.y = fma(palette[bones.x].y, weights.x, palette[bones.y].y * weights.y);
        transform.z = fma(palette[bones.x].z, weights.x, palette[bones.y].z * weights.y);
        transform.x = fma(palette[bones.z].x, weights.z, transform.x);
        transform.y = fma(palette[bones.z].y, weights.z, transform.y);
        transform.z = fma(palette[bones.z].z, weights.z, transform.z);
        transform.x = fma(palette[bones.w].x, weights.w, transform.x);
        transform.y = fma(palette[bones.w].y, weights.w, transform.y);
        transform.z = fma(palette[bones.w].z, weights.w, transform.z);
    }
    return transform;
}
