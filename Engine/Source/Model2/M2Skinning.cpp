#include "Engine/Source/Model2/M2Skinning.h"
#include "Engine/Source/Model2/M2Vector.h"

// VERIFY: 3101ba14 manual
// OFFSET: 0x0082A210
void M2TransformVertices_cpp(const C44Matrix* bones, const M2Vertex* vertex, uint32_t count, uint8_t* vertices, int32_t texCoord) {
    C44Matrix matrix;
    bool matrixReady = false;
    uint32_t weights = 0;
    uint32_t indices = 0;
    const M2Vertex* end = vertex + count;
    float* out = reinterpret_cast<float*>(vertices);

    for (; vertex < end; vertex++, out += 8) {
        if (!matrixReady || static_cast<uint32_t>(vertex->m_weights) != weights || static_cast<uint32_t>(vertex->m_indices) != indices) {
            matrixReady = true;
            weights = static_cast<uint32_t>(vertex->m_weights);
            indices = static_cast<uint32_t>(vertex->m_indices);

            const C44Matrix& bone = bones[vertex->m_indices.b[0]];
            float weight = vertex->m_weights.b[0] * 0.0039215689f;
            matrix.a0 = bone.a0 * weight;
            matrix.a1 = bone.a1 * weight;
            matrix.a2 = bone.a2 * weight;
            matrix.b0 = bone.b0 * weight;
            matrix.b1 = bone.b1 * weight;
            matrix.b2 = bone.b2 * weight;
            matrix.c0 = bone.c0 * weight;
            matrix.c1 = bone.c1 * weight;
            matrix.c2 = bone.c2 * weight;
            matrix.d0 = bone.d0 * weight;
            matrix.d1 = bone.d1 * weight;
            matrix.d2 = bone.d2 * weight;

            for (uint32_t k = 1; k < 4 && vertex->m_weights.b[k]; k++) {
                const C44Matrix& next = bones[vertex->m_indices.b[k]];
                weight = vertex->m_weights.b[k] * 0.0039215689f;
                matrix.a0 += next.a0 * weight;
                matrix.a1 += next.a1 * weight;
                matrix.a2 += next.a2 * weight;
                matrix.b0 += next.b0 * weight;
                matrix.b1 += next.b1 * weight;
                matrix.b2 += next.b2 * weight;
                matrix.c0 += next.c0 * weight;
                matrix.c1 += next.c1 * weight;
                matrix.c2 += next.c2 * weight;
                matrix.d0 += next.d0 * weight;
                matrix.d1 += next.d1 * weight;
                matrix.d2 += next.d2 * weight;
            }
        }

        C3Vector position = vertex->m_position * matrix;
        out[0] = position.x;
        out[1] = position.y;
        out[2] = position.z;
        out[3] = vertex->m_normal.y * matrix.b0 + matrix.c0 * vertex->m_normal.z + matrix.a0 * vertex->m_normal.x;
        out[4] = vertex->m_normal.y * matrix.b1 + matrix.c1 * vertex->m_normal.z + matrix.a1 * vertex->m_normal.x;
        out[5] = vertex->m_normal.y * matrix.b2 + matrix.c2 * vertex->m_normal.z + matrix.a2 * vertex->m_normal.x;
        out[6] = vertex->m_texcoord[texCoord].x;
        out[7] = vertex->m_texcoord[texCoord].y;
    }
}

// VERIFY: 3dfd45aa manual
// OFFSET: 0x0082A4E0
void M2TransformVerticesOneBone_cpp(const C44Matrix* bones, const M2Vertex* vertex, uint32_t count, uint8_t* vertices, int32_t texCoord) {
    const M2Vertex* end = vertex + count;
    float* out = reinterpret_cast<float*>(vertices);

    for (; vertex < end; vertex++, out += 8) {
        const C44Matrix& bone = bones[vertex->m_indices.b[0]];
        out[0] = bone.c0 * vertex->m_position.z + vertex->m_position.y * bone.b0 + vertex->m_position.x * bone.a0 + bone.d0;
        out[1] = vertex->m_position.x * bone.a1 + vertex->m_position.y * bone.b1 + bone.c1 * vertex->m_position.z + bone.d1;
        out[2] = vertex->m_position.x * bone.a2 + vertex->m_position.y * bone.b2 + bone.c2 * vertex->m_position.z + bone.d2;
        out[3] = vertex->m_normal.x * bone.a0 + bone.c0 * vertex->m_normal.z + bone.b0 * vertex->m_normal.y;
        out[4] = bone.c1 * vertex->m_normal.z + bone.b1 * vertex->m_normal.y + bone.a1 * vertex->m_normal.x;
        out[5] = bone.c2 * vertex->m_normal.z + bone.b2 * vertex->m_normal.y + bone.a2 * vertex->m_normal.x;
        out[6] = vertex->m_texcoord[texCoord].x;
        out[7] = vertex->m_texcoord[texCoord].y;
    }
}

// VERIFY: edef765a manual
// OFFSET: 0x0082A600
void M2TransformVertices_simd(const C44Matrix* bones, const M2Vertex* vertex, uint32_t count, uint8_t* vertices, int32_t texCoord) {
    if (reinterpret_cast<uintptr_t>(vertices) & 0xF) {
        M2TransformVertices_cpp(bones, vertex, count, vertices, texCoord);
        return;
    }

    bool matrixReady = false;
    uint32_t weights = 0;
    uint32_t indices = 0;
    const M2Vertex* end = vertex + count;
    float* out = reinterpret_cast<float*>(vertices);
    M2Vector::Float4 row0;
    M2Vector::Float4 row1;
    M2Vector::Float4 row2;
    M2Vector::Float4 row3;

    while (vertex != end) {
        if (!matrixReady || static_cast<uint32_t>(vertex->m_weights) != weights || static_cast<uint32_t>(vertex->m_indices) != indices) {
            matrixReady = true;
            weights = static_cast<uint32_t>(vertex->m_weights);
            indices = static_cast<uint32_t>(vertex->m_indices);

            M2Vector::Float4 weight = M2Vector::Splat(vertex->m_weights[0] * 0.0039215689f);
            const float* bone = reinterpret_cast<const float*>(&bones[vertex->m_indices[0]]);
            row0 = M2Vector::Multiply(M2Vector::Load(&bone[0]), weight);
            row1 = M2Vector::Multiply(M2Vector::Load(&bone[4]), weight);
            row2 = M2Vector::Multiply(M2Vector::Load(&bone[8]), weight);
            row3 = M2Vector::Multiply(M2Vector::Load(&bone[12]), weight);

            for (uint32_t i = 1; i < 4 && vertex->m_weights[i]; i++) {
                weight = M2Vector::Splat(vertex->m_weights[i] * 0.0039215689f);
                bone = reinterpret_cast<const float*>(&bones[vertex->m_indices[i]]);
                row0 = M2Vector::Add(row0, M2Vector::Multiply(M2Vector::Load(&bone[0]), weight));
                row1 = M2Vector::Add(row1, M2Vector::Multiply(M2Vector::Load(&bone[4]), weight));
                row2 = M2Vector::Add(row2, M2Vector::Multiply(M2Vector::Load(&bone[8]), weight));
                row3 = M2Vector::Add(row3, M2Vector::Multiply(M2Vector::Load(&bone[12]), weight));
            }
        }

        M2Vector::Float4 normal = M2Vector::Add(M2Vector::Add(M2Vector::Multiply(M2Vector::Splat(vertex->m_normal.x), row0), M2Vector::Multiply(M2Vector::Splat(vertex->m_normal.y), row1)), M2Vector::Multiply(M2Vector::Splat(vertex->m_normal.z), row2));
        M2Vector::Float4 position = M2Vector::Add(M2Vector::Add(M2Vector::Add(M2Vector::Multiply(M2Vector::Splat(vertex->m_position.x), row0), M2Vector::Multiply(M2Vector::Splat(vertex->m_position.y), row1)), M2Vector::Multiply(M2Vector::Splat(vertex->m_position.z), row2)), row3);
        M2Vector::Store(&out[0], position);
        M2Vector::Store(&out[3], normal);
        out[6] = vertex->m_texcoord[texCoord].x;
        out[7] = vertex->m_texcoord[texCoord].y;
        vertex++;
        out += 8;
    }
}

// VERIFY: 9d870aa4 manual
// OFFSET: 0x0082AC10
void M2TransformVerticesOneBone_simd(const C44Matrix* bones, const M2Vertex* vertex, uint32_t count, uint8_t* vertices, int32_t texCoord) {
    if (reinterpret_cast<uintptr_t>(vertices) & 0xF) {
        M2TransformVerticesOneBone_cpp(bones, vertex, count, vertices, texCoord);
        return;
    }

    const M2Vertex* end = vertex + count;
    float* out = reinterpret_cast<float*>(vertices);

    for (; vertex < end; vertex++, out += 8) {
        const float* bone = reinterpret_cast<const float*>(&bones[vertex->m_indices.b[0]]);
        M2Vector::Float4 row0 = M2Vector::Load(&bone[0]);
        M2Vector::Float4 row1 = M2Vector::Load(&bone[4]);
        M2Vector::Float4 row2 = M2Vector::Load(&bone[8]);
        M2Vector::Float4 position = M2Vector::Add(M2Vector::Add(M2Vector::Multiply(row2, M2Vector::Splat(vertex->m_position.z)), M2Vector::Add(M2Vector::Multiply(row1, M2Vector::Splat(vertex->m_position.y)), M2Vector::Multiply(row0, M2Vector::Splat(vertex->m_position.x)))), M2Vector::Load(&bone[12]));
        M2Vector::Float4 normal = M2Vector::Add(M2Vector::Add(M2Vector::Multiply(row0, M2Vector::Splat(vertex->m_normal.x)), M2Vector::Multiply(row1, M2Vector::Splat(vertex->m_normal.y))), M2Vector::Multiply(M2Vector::Splat(vertex->m_normal.z), row2));
        M2Vector::Store(&out[0], position);
        M2Vector::Store(&out[3], normal);
        out[6] = vertex->m_texcoord[texCoord].x;
        out[7] = vertex->m_texcoord[texCoord].y;
    }
}

