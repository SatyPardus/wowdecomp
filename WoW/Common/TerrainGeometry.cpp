#include "TerrainGeometry.h"

// Original CMapChunk world-space geometry: 0x007C3F30.
void Terrain::CreateVerticesWorld(CGxVertexPN* v, const GeometryView& geometry) {
    float cols[9];
    float rows[9];

    float left = 17066.666f - geometry.chunkOffset.x * 33.333332f;
    float top = 17066.666f - geometry.chunkOffset.y * 33.333332f;
    rows[0] = top;
    cols[0] = left;
    rows[1] = top - 4.1666665f;
    rows[2] = top - 8.333333f;
    rows[3] = top - 12.5f;
    rows[4] = top - 16.666666f;
    rows[5] = top - 20.833332f;
    rows[6] = top - 25.0f;
    rows[7] = top - 29.166666f;
    cols[1] = left - 4.1666665f;
    cols[2] = left - 8.333333f;
    cols[3] = left - 12.5f;
    cols[4] = left - 16.666666f;
    cols[5] = left - 20.833332f;
    cols[6] = left - 25.0f;
    cols[7] = left - 29.166666f;
    rows[8] = 17066.666f - 33.333332f * (geometry.chunkOffset.y + 1);
    cols[8] = 17066.666f - (geometry.chunkOffset.x + 1) * 33.333332f;

    const float* height = geometry.heights;
    const int8_t* normals = geometry.normals;

    for (int32_t row = 0; row < 9; row++) {
        for (int32_t col = 0; col < 9; col++) {
            v->m_position.x = rows[row];
            v->m_position.y = cols[col];
            v->m_position.z = geometry.baseHeight + height[col];
            v->m_normal.x = normals[3 * col + 0] * 0.0078740157f;
            v->m_normal.y = normals[3 * col + 1] * 0.0078740157f;
            v->m_normal.z = normals[3 * col + 2] * 0.0078740157f;
            v++;
        }

        height += 9;
        normals += 27;

        if (row < 8) {
            for (int32_t col = 0; col < 8; col++) {
                v->m_position.x = rows[row] - 2.0833333f;
                v->m_position.y = cols[col] - 2.0833333f;
                v->m_position.z = geometry.baseHeight + height[col];
                v->m_normal.x = normals[3 * col + 0] * 0.0078740157f;
                v->m_normal.y = normals[3 * col + 1] * 0.0078740157f;
                v->m_normal.z = normals[3 * col + 2] * 0.0078740157f;
                v++;
            }

            height += 8;
            normals += 24;
        }
    }
}

// Original CMapChunk world-space geometry: 0x007C4620.
void Terrain::CreateVerticesWorld(CGxVertexPNC* v, const GeometryView& geometry) {
    float cols[9];
    float rows[9];

    float left = 17066.666f - geometry.chunkOffset.x * 33.333332f;
    float top = 17066.666f - geometry.chunkOffset.y * 33.333332f;
    rows[0] = top;
    cols[0] = left;
    rows[1] = top - 4.1666665f;
    rows[2] = top - 8.333333f;
    rows[3] = top - 12.5f;
    rows[4] = top - 16.666666f;
    rows[5] = top - 20.833332f;
    rows[6] = top - 25.0f;
    rows[7] = top - 29.166666f;
    cols[1] = left - 4.1666665f;
    cols[2] = left - 8.333333f;
    cols[3] = left - 12.5f;
    cols[4] = left - 16.666666f;
    cols[5] = left - 20.833332f;
    cols[6] = left - 25.0f;
    cols[7] = left - 29.166666f;
    rows[8] = 17066.666f - 33.333332f * (geometry.chunkOffset.y + 1);
    cols[8] = 17066.666f - (geometry.chunkOffset.x + 1) * 33.333332f;

    const int8_t* normals = geometry.normals;
    const uint32_t* shading = geometry.colors;
    const float* height = geometry.heights;

    for (int32_t row = 0; row < 9; row++) {
        for (int32_t col = 0; col < 9; col++) {
            v->m_position.x = rows[row];
            v->m_position.y = cols[col];
            v->m_position.z = geometry.baseHeight + *height;
            v->m_normal.x = normals[0] * 0.0078740157f;
            v->m_normal.y = normals[1] * 0.0078740157f;
            v->m_normal.z = normals[2] * 0.0078740157f;

            uint32_t color;

            if (!geometry.colors) {
                color = 0xFF7F7F7F;
            } else if (geometry.swapRedBlue) {
                const uint8_t* src = reinterpret_cast<const uint8_t*>(shading);
                color = src[2] | (src[1] << 8) | (src[0] << 16) | (static_cast<uint32_t>(src[3]) << 24);
            } else {
                color = *shading;
            }

            height++;
            *reinterpret_cast<uint32_t*>(&v->m_color) = color;
            v++;
            normals += 3;
            if (shading) ++shading;
        }

        if (row < 8) {
            for (int32_t col = 0; col < 8; col++) {
                v->m_position.x = rows[row] - 2.0833333f;
                v->m_position.y = cols[col] - 2.0833333f;
                v->m_position.z = geometry.baseHeight + *height;
                v->m_normal.x = normals[0] * 0.0078740157f;
                v->m_normal.y = normals[1] * 0.0078740157f;
                v->m_normal.z = normals[2] * 0.0078740157f;

                uint32_t color;

                if (!geometry.colors) {
                    color = 0xFF7F7F7F;
                } else if (geometry.swapRedBlue) {
                    const uint8_t* src = reinterpret_cast<const uint8_t*>(shading);
                    color = src[2] | (src[1] << 8) | (src[0] << 16) | (static_cast<uint32_t>(src[3]) << 24);
                } else {
                    color = *shading;
                }

                height++;
                *reinterpret_cast<uint32_t*>(&v->m_color) = color;
                v++;
                normals += 3;
                if (shading) ++shading;
            }
        }
    }
}

// Original CMapChunk triangle ordering and 4x4 hole mask: 0x007C3B60.
uint16_t Terrain::CreateIndices(uint16_t* indexDst, uint16_t holes, int32_t baseVertex) {
    uint16_t totalIndices = 0;

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            int holeMaskIndex = (row / 2) * 4 + (col / 2);
            if (holes & (1u << holeMaskIndex))
                continue;

            uint16_t TL = baseVertex + col;
            uint16_t TR = baseVertex + col + 1;
            uint16_t C = baseVertex + col + 9;
            uint16_t BL = baseVertex + col + 17;
            uint16_t BR = baseVertex + col + 18;

            indexDst[0] = C;
            indexDst[1] = TL;
            indexDst[2] = BL;
            indexDst[3] = C;
            indexDst[4] = TR;
            indexDst[5] = TL;
            indexDst[6] = C;
            indexDst[7] = BR;
            indexDst[8] = TR;
            indexDst[9] = C;
            indexDst[10] = BL;
            indexDst[11] = BR;
            indexDst += 12;
            totalIndices += 12;
        }

        baseVertex += 17;
    }

    return totalIndices;
}
