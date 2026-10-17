#include "WoW/Source/WorldClient/MapRenderChunk.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "Engine/Source/Storm/H/storm.h"
#include <cstring>

// Original terrain alpha/shadow unpacking, shared by WorldClient and native hosts.
// STATIC: 0x00A3FFF4
static const uint32_t s_nibbleMask[2] = { 0x0F, 0xF0 };
// STATIC: 0x00A3FFFC
static const uint32_t s_nibbleShift[2] = { 4, 0 };
// STATIC: 0x00A40004
static const uint32_t s_nibbleUnshift[2] = { 0, 4 };
// STATIC: 0x00A4000C
static const uint32_t s_bitMask[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
// STATIC: 0x00A4002C
static const uint32_t s_bitShift[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
// STATIC: 0x00A4004C
static const uint8_t s_shadowAlpha[2] = { 0xFF, 0x00 };
// STATIC: 0x00AEE214
static uint16_t s_shadowTexel[2] = { 0x0000, 0xFFFF };
// STATIC: 0x00D1CED8
static uint8_t s_alphaRowBuf[4][64];
// STATIC: 0x00D1CFD8
static uint8_t s_defaultShadowRow[64];
// STATIC: 0x00D1D018
static uint8_t s_defaultAlphaRow[64];

void CMapRenderChunk::ResetAlphaRows() {
    std::memset(s_defaultAlphaRow, 0, sizeof(s_defaultAlphaRow));
    std::memset(s_defaultShadowRow, 0, sizeof(s_defaultShadowRow));
}

// VERIFY: d15abdb3 auto
// OFFSET: 0x007B7420
uint8_t* CMapRenderChunk::DecodeAlphaRLE(uint8_t* src, uint8_t* dst, int32_t count) {
    int32_t srcIndex = 0;
    int32_t dstIndex = 0;

    while (dstIndex < count) {
        int8_t header = static_cast<int8_t>(src[srcIndex]);
        srcIndex++;

        if (header < 0) {
            uint32_t runLength = header & 0x7F;
            uint8_t value = src[srcIndex];
            srcIndex++;

            if (runLength) {
                memset(&dst[dstIndex], value, runLength);
                dstIndex += runLength;
            }
        } else {
            for (uint32_t i = static_cast<uint8_t>(header); i; i--) {
                dst[dstIndex] = src[srcIndex];
                srcIndex++;
                dstIndex++;
            }
        }
    }

    return &src[srcIndex];
}

// VERIFY: 425713a6 auto
// OFFSET: 0x007B74A0
void CMapRenderChunk::SetupAlphaRows(uint8_t** shadowCursor, uint8_t** shadowRow, SLayerAlpha* layers, uint32_t size, uint8_t** alphaRows) {
    if (*shadowCursor) {
        *shadowRow = *shadowCursor;
        *shadowCursor += size >> 3;
    } else {
        *shadowRow = s_defaultShadowRow;
    }

    for (int32_t i = 0; i < 4; i++) {
        if (!layers[i].m_alphaData) {
            alphaRows[i] = s_defaultAlphaRow;
        } else if (layers[i].m_flags & 0x200) {
            alphaRows[i] = s_alphaRowBuf[i];
            layers[i].m_alphaData = CMapRenderChunk::DecodeAlphaRLE(layers[i].m_alphaData, s_alphaRowBuf[i], size);
        } else {
            alphaRows[i] = layers[i].m_alphaData;
            layers[i].m_alphaData += size;
        }
    }
}

// VERIFY: 900e0dee auto
// OFFSET: 0x007B7530
void CMapRenderChunk::SetupAlphaRows4444(uint32_t size, uint8_t** shadowRow, uint8_t** alphaRows, SLayerAlpha* layers, uint8_t** shadowCursor) {
    if (*shadowCursor) {
        *shadowRow = *shadowCursor;
        *shadowCursor += size >> 3;
    } else {
        *shadowRow = s_defaultShadowRow;
    }

    for (int32_t i = 0; i < 4; i++) {
        if (layers[i].m_alphaData) {
            alphaRows[i] = layers[i].m_alphaData;
            layers[i].m_alphaData += size >> 1;
        } else {
            alphaRows[i] = s_defaultAlphaRow;
        }
    }
}

// VERIFY: 25ebba41 manual
// OFFSET: 0x007B75B0
void __stdcall CMapRenderChunk::UnpackAlphaBitsFixed4444Mip0(uint16_t* dst, uint32_t size, SLayerAlpha* layer) {
    uint8_t* alphaData = layer->m_alphaData;
    uint32_t i = 0;

    for (uint32_t y = 0; y < size; y++) {
        for (uint32_t x = 0; x < size; x++) {
            *dst = static_cast<uint16_t>((((alphaData[i >> 1] & s_nibbleMask[i & 1]) >> s_nibbleUnshift[i & 1]) << 12) | 0xFFF);
            dst++;
            i++;
        }
    }
}

// VERIFY: ede6e802 manual
// OFFSET: 0x007B7620
void __stdcall CMapRenderChunk::UnpackAlphaBitsFixed4444Mip1(uint16_t* dst, uint32_t size, SLayerAlpha* layer) {
    uint8_t* alphaData = layer->m_alphaData;
    uint32_t i = 0;

    for (uint32_t y = 0; y < size; y += 2) {
        for (uint32_t x = 0; x < size - 2; x += 2) {
            *dst = static_cast<uint16_t>((((alphaData[i >> 1] & s_nibbleMask[i & 1]) >> s_nibbleUnshift[i & 1]) << 12) | 0xFFF);
            dst++;
            i += 2;
        }

        i++;
        *dst = static_cast<uint16_t>((((alphaData[i >> 1] & s_nibbleMask[i & 1]) >> s_nibbleUnshift[i & 1]) << 12) | 0xFFF);
        dst++;
        i++;

        if (y == size - 4) {
            i += 64;
        }

        i += size;
    }
}

// VERIFY: 152a7dd5 manual
// OFFSET: 0x007B76F0
void __stdcall CMapRenderChunk::UnpackAlphaBitsUnfixed4444Mip0(uint16_t* dst, uint32_t size, SLayerAlpha* layer) {
    uint8_t* alphaData = layer->m_alphaData;
    uint32_t i = 0;
    uint16_t texel = 0xFFF;

    for (uint32_t y = 0; y < size - 1; y++) {
        for (uint32_t x = 0; x < size - 1; x++) {
            texel = static_cast<uint16_t>((((alphaData[i >> 1] & s_nibbleMask[i & 1]) >> s_nibbleUnshift[i & 1]) << 12) | 0xFFF);
            i++;
            *dst = texel;
            dst++;
        }

        *dst = texel;
        dst++;
        i++;
    }

    i -= size;

    for (uint32_t x = 0; x < size - 1; x++) {
        texel = static_cast<uint16_t>((((alphaData[i >> 1] & s_nibbleMask[i & 1]) >> s_nibbleUnshift[i & 1]) << 12) | 0xFFF);
        i++;
        *dst = texel;
        dst++;
    }

    *dst = texel;
}

// VERIFY: 8998a684 manual
// OFFSET: 0x007B77D0
void __stdcall CMapRenderChunk::UnpackAlphaBitsUnfixed4444Mip1(uint16_t* dst, uint32_t size, SLayerAlpha* layer) {
    uint8_t* alphaData = layer->m_alphaData;
    uint32_t i = 0;

    for (uint32_t y = 0; y < size - 1; y += 2) {
        for (uint32_t x = 0; x < size - 1; x += 2) {
            *dst = static_cast<uint16_t>((((alphaData[i >> 1] & s_nibbleMask[i & 1]) >> s_nibbleUnshift[i & 1]) << 12) | 0xFFF);
            dst++;
            i += 2;
        }

        i += size;
    }
}

// VERIFY: 3a09a0ac manual
// OFFSET: 0x007B7860
void CMapRenderChunk::CollectLayerAlphaPtrs(CMapChunk* chunk, SLayerAlpha* layers) {
    for (uint32_t i = 0; i < 4; i++) {
        layers[i].m_alphaData = nullptr;

        if (this->m_layersCount > i) {
            SMLayer* layer = &chunk->m_layers[i];

            if (layer->m_flags & 0x100) {
                layers[i].m_alphaData = &chunk->m_additionalShadowmap[layer->m_offsetInMCAL];
            }

            layers[i].m_flags = layer->m_flags;
        }
    }
}

// VERIFY: c03f753e manual
// OFFSET: 0x007B7930
void __stdcall CMapRenderChunk::ExpandShadowBitsToTexels(uint16_t* dst, uint32_t size, uint8_t* shadowMap) {
    uint32_t i = 0;

    for (uint32_t y = 0; y < size; y++) {
        for (uint32_t x = 0; x < size; x++) {
            *dst = s_shadowTexel[(shadowMap[i >> 3] & s_bitMask[i & 7]) >> s_bitShift[i & 7]];
            dst++;
            i++;
        }
    }
}

// VERIFY: e6e2d775 manual
// OFFSET: 0x007B79A0
void __stdcall CMapRenderChunk::ExpandShadowBitsToTexelsMip(uint16_t* dst, uint32_t size, uint8_t* shadowMap) {
    uint32_t i = 0;

    for (uint32_t y = 0; y < size; y += 2) {
        for (uint32_t x = 0; x < size - 2; x += 2) {
            *dst = s_shadowTexel[(shadowMap[i >> 3] & s_bitMask[i & 7]) >> s_bitShift[i & 7]];
            dst++;
            i += 2;
        }

        i++;
        *dst = s_shadowTexel[(shadowMap[i >> 3] & s_bitMask[i & 7]) >> s_bitShift[i & 7]];
        dst++;
        i++;

        if (y == size - 4) {
            i += size;
        }

        i += size;
    }
}

// VERIFY: 3cd77f91 manual
// OFFSET: 0x007B7C60
void __stdcall CMapRenderChunk::UnpackAlphaShadowBitsFixed8888Mip0(CImVector* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, int32_t alphaSlot, uint8_t* shadowMap) {
    CImVector* dst = dstBase + dstOffset;

    for (uint32_t y = 0; y < size; y++) {
        uint8_t* shadowRow;
        uint8_t* alphaRows[4];
        CMapRenderChunk::SetupAlphaRows(&shadowMap, &shadowRow, cursors, size, alphaRows);

        for (uint32_t x = 0; x < size; x++) {
            CImVector color;

            if (alphaSlot) {
                int32_t alpha[4];
                alpha[1] = alphaRows[1][x];
                alpha[2] = alphaRows[2][x];
                alpha[3] = alphaRows[3][x];
                alpha[alphaSlot] = 255 - alpha[3] - alpha[2] - alpha[1] - alphaRows[0][x];
                color.b = alpha[3];
                color.g = alpha[2];
                color.r = alpha[1];
            } else {
                color.b = alphaRows[3][x];
                color.g = alphaRows[2][x];
                color.r = alphaRows[1][x];
            }

            color.a = s_shadowAlpha[(shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]];
            dst[x] = color;
        }

        dst += dstPitch;
    }
}

// VERIFY: 57dc0019 manual
// OFFSET: 0x007B7DC0
void __stdcall CMapRenderChunk::UnpackAlphaShadowBitsFixed8888Mip1(CImVector* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, int32_t alphaSlot, uint8_t* shadowMap) {
    CImVector* dst = dstBase + dstOffset;

    for (uint32_t y = 0; y < size; y += 2) {
        uint8_t* shadowRow;
        uint8_t* alphaRows[4];
        CMapRenderChunk::SetupAlphaRows(&shadowMap, &shadowRow, cursors, size, alphaRows);

        uint32_t x = 0;
        uint32_t n = 0;

        for (; x < size - 2; x += 2) {
            CImVector color;

            if (alphaSlot) {
                int32_t alpha[4];
                alpha[1] = alphaRows[1][x];
                alpha[2] = alphaRows[2][x];
                alpha[3] = alphaRows[3][x];
                alpha[alphaSlot] = 255 - alpha[3] - alpha[2] - alpha[1] - alphaRows[0][x];
                color.b = alpha[3];
                color.g = alpha[2];
                color.r = alpha[1];
            } else {
                color.b = alphaRows[3][x];
                color.g = alphaRows[2][x];
                color.r = alphaRows[1][x];
            }

            color.a = s_shadowAlpha[(shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]];
            dst[n] = color;
            n++;
        }

        x++;

        CImVector color;

        if (alphaSlot) {
            int32_t alpha[4];
            alpha[1] = alphaRows[1][x];
            alpha[2] = alphaRows[2][x];
            alpha[3] = alphaRows[3][x];
            alpha[alphaSlot] = 255 - alpha[3] - alpha[2] - alpha[1] - alphaRows[0][x];
            color.b = alpha[3];
            color.g = alpha[2];
            color.r = alpha[1];
        } else {
            color.b = alphaRows[3][x];
            color.g = alphaRows[2][x];
            color.r = alphaRows[1][x];
        }

        color.a = s_shadowAlpha[(shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]];
        dst[n] = color;

        if (y < size - 2) {
            CMapRenderChunk::SetupAlphaRows(&shadowMap, &shadowRow, cursors, size, alphaRows);
        }

        if (y == size - 4) {
            CMapRenderChunk::SetupAlphaRows(&shadowMap, &shadowRow, cursors, size, alphaRows);
        }

        dst += dstPitch;
    }
}

// VERIFY: ddfa67aa manual
// OFFSET: 0x007B8070
void __stdcall CMapRenderChunk::UnpackAlphaShadowBitsFixed4444Mip0(uint16_t* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, uint8_t* shadowMap) {
    uint16_t* dst = dstBase + dstOffset;

    for (uint32_t y = 0; y < size; y++) {
        uint8_t* shadowRow;
        uint8_t* alphaRows[4];
        CMapRenderChunk::SetupAlphaRows4444(size, &shadowRow, alphaRows, cursors, &shadowMap);

        for (uint32_t x = 0; x < size; x++) {
            uint32_t mask = s_nibbleMask[x & 1];
            uint32_t shift = s_nibbleShift[x & 1];
            uint8_t alpha1 = static_cast<uint8_t>((alphaRows[1][x >> 1] & mask) << shift);
            uint8_t alpha2 = static_cast<uint8_t>((alphaRows[2][x >> 1] & mask) << shift);
            uint8_t alpha3 = static_cast<uint8_t>((alphaRows[3][x >> 1] & mask) << shift);
            uint8_t shadow = s_shadowAlpha[(shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]];
            dst[x] = static_cast<uint16_t>(((shadow & 0xF0) << 8) | ((alpha1 & 0xF0) << 4) | (alpha2 & 0xF0) | (alpha3 >> 4));
        }

        dst += dstPitch;
    }
}

// VERIFY: 17932350 manual
// OFFSET: 0x007B8190
void __stdcall CMapRenderChunk::UnpackAlphaShadowBitsFixed4444Mip1(uint16_t* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, uint8_t* shadowMap) {
    uint16_t* dst = dstBase + dstOffset;

    for (uint32_t y = 0; y < size; y += 2) {
        uint8_t* shadowRow;
        uint8_t* alphaRows[4];
        CMapRenderChunk::SetupAlphaRows4444(size, &shadowRow, alphaRows, cursors, &shadowMap);

        uint32_t x = 0;
        uint32_t n = 0;

        for (; x < size - 2; x += 2) {
            uint32_t mask = s_nibbleMask[x & 1];
            uint32_t shift = s_nibbleShift[x & 1];
            uint8_t alpha1 = static_cast<uint8_t>((alphaRows[1][x >> 1] & mask) << shift);
            uint8_t alpha2 = static_cast<uint8_t>((alphaRows[2][x >> 1] & mask) << shift);
            uint8_t alpha3 = static_cast<uint8_t>((alphaRows[3][x >> 1] & mask) << shift);
            uint8_t shadow = s_shadowAlpha[(shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]];
            dst[n] = static_cast<uint16_t>(((shadow & 0xF0) << 8) | ((alpha1 & 0xF0) << 4) | (alpha2 & 0xF0) | (alpha3 >> 4));
            n++;
        }

        x++;

        uint32_t mask = s_nibbleMask[x & 1];
        uint32_t shift = s_nibbleShift[x & 1];
        uint8_t alpha1 = static_cast<uint8_t>((alphaRows[1][x >> 1] & mask) << shift);
        uint8_t alpha2 = static_cast<uint8_t>((alphaRows[2][x >> 1] & mask) << shift);
        uint8_t alpha3 = static_cast<uint8_t>((alphaRows[3][x >> 1] & mask) << shift);
        uint8_t shadow = s_shadowAlpha[(shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]];
        dst[n] = static_cast<uint16_t>(((shadow & 0xF0) << 8) | ((alpha1 & 0xF0) << 4) | (alpha2 & 0xF0) | (alpha3 >> 4));

        // The final output row already consumed the last source row. Avoid
        // advancing source pointers past their arrays after all texels are done.
        if (y < size - 2) {
            CMapRenderChunk::SetupAlphaRows4444(size, &shadowRow, alphaRows, cursors, &shadowMap);
        }

        if (y == size - 4) {
            CMapRenderChunk::SetupAlphaRows4444(size, &shadowRow, alphaRows, cursors, &shadowMap);
        }

        dst += dstPitch;
    }
}

// VERIFY: 3b181805 manual
// OFFSET: 0x007B84A0
void CMapRenderChunk::PackAlphaShadow4444(uint16_t* dst, uint8_t** alphaRows, uint8_t* shadowRow, uint32_t size) {
    uint16_t texel = 0;
    uint32_t x = 0;

    for (; x < size - 1; x++) {
        uint32_t mask = s_nibbleMask[x & 1];
        uint32_t shift = s_nibbleShift[x & 1];
        uint8_t shadow = s_shadowAlpha[(shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]];
        uint8_t alpha1 = static_cast<uint8_t>((alphaRows[1][x >> 1] & mask) << shift);
        uint8_t alpha2 = static_cast<uint8_t>((alphaRows[2][x >> 1] & mask) << shift);
        uint8_t alpha3 = static_cast<uint8_t>((alphaRows[3][x >> 1] & mask) << shift);
        texel = static_cast<uint16_t>(((shadow & 0xF0) << 8) | ((alpha1 & 0xF0) << 4) | (alpha2 & 0xF0) | (alpha3 >> 4));
        dst[x] = texel;
    }

    dst[x] = texel;
}

// VERIFY: d74fae7b manual
// OFFSET: 0x007B85A0
void __stdcall CMapRenderChunk::UnpackAlphaShadowBitsUnfixed4444Mip0(uint16_t* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, uint8_t* shadowMap) {
    uint16_t* dst = dstBase + dstOffset;
    uint8_t* shadowRow = nullptr;
    uint8_t* alphaRows[4];

    for (uint32_t y = 0; y < size - 1; y++) {
        CMapRenderChunk::SetupAlphaRows4444(size, &shadowRow, alphaRows, cursors, &shadowMap);
        CMapRenderChunk::PackAlphaShadow4444(dst, alphaRows, shadowRow, size);
        dst += dstPitch;
    }

    CMapRenderChunk::PackAlphaShadow4444(dst, alphaRows, shadowRow, size);
}

// VERIFY: 81a839dc manual
// OFFSET: 0x007B8620
void __stdcall CMapRenderChunk::UnpackAlphaShadowBitsUnfixed4444Mip1(uint16_t* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, uint8_t* shadowMap) {
    uint16_t* dst = dstBase + dstOffset;

    for (uint32_t y = 0; y < size - 1; y += 2) {
        uint8_t* shadowRow;
        uint8_t* alphaRows[4];
        CMapRenderChunk::SetupAlphaRows4444(size, &shadowRow, alphaRows, cursors, &shadowMap);

        uint16_t* texel = dst;

        for (uint32_t x = 0; x < size - 1; x += 2) {
            uint32_t mask = s_nibbleMask[x & 1];
            uint32_t shift = s_nibbleShift[x & 1];
            uint8_t alpha1 = static_cast<uint8_t>((alphaRows[1][x >> 1] & mask) << shift);
            uint8_t alpha2 = static_cast<uint8_t>((alphaRows[2][x >> 1] & mask) << shift);
            uint8_t alpha3 = static_cast<uint8_t>((alphaRows[3][x >> 1] & mask) << shift);
            uint8_t shadow = s_shadowAlpha[(shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]];
            *texel = static_cast<uint16_t>(((shadow & 0xF0) << 8) | ((alpha1 & 0xF0) << 4) | (alpha2 & 0xF0) | (alpha3 >> 4));
            texel++;
        }

        CMapRenderChunk::SetupAlphaRows4444(size, &shadowRow, alphaRows, cursors, &shadowMap);

        dst += dstPitch;
    }
}

// VERIFY: 0b452c5b manual
// OFFSET: 0x007B87F0
void CMapRenderChunk::UnpackAlphaShadowBits(void* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, int32_t alphaSlot, uint8_t* shadowMap, int32_t genFormat, int32_t doNotFixAlphaMap) {
    if (doNotFixAlphaMap) {
        if (genFormat == 3) {
            if (this->m_renderFlags & 0x8) {
                CMapRenderChunk::UnpackAlphaShadowBitsFixed4444Mip1(static_cast<uint16_t*>(dstBase), dstOffset, dstPitch, size, cursors, shadowMap);
            } else {
                CMapRenderChunk::UnpackAlphaShadowBitsFixed4444Mip0(static_cast<uint16_t*>(dstBase), dstOffset, dstPitch, size, cursors, shadowMap);
            }
        } else {
            if (this->m_renderFlags & 0x8) {
                CMapRenderChunk::UnpackAlphaShadowBitsFixed8888Mip1(static_cast<CImVector*>(dstBase), dstOffset, dstPitch, size, cursors, alphaSlot, shadowMap);
            } else {
                CMapRenderChunk::UnpackAlphaShadowBitsFixed8888Mip0(static_cast<CImVector*>(dstBase), dstOffset, dstPitch, size, cursors, alphaSlot, shadowMap);
            }
        }

        return;
    }

    if (genFormat != 3) {
        SErrDisplayAppFatal("CMapChunk::UnpackAlphaShadowBits(): Bad genformat.");
    }

    if (this->m_renderFlags & 0x8) {
        CMapRenderChunk::UnpackAlphaShadowBitsUnfixed4444Mip1(static_cast<uint16_t*>(dstBase), dstOffset, dstPitch, size, cursors, shadowMap);
    } else {
        CMapRenderChunk::UnpackAlphaShadowBitsUnfixed4444Mip0(static_cast<uint16_t*>(dstBase), dstOffset, dstPitch, size, cursors, shadowMap);
    }
}

// VERIFY: b610665e manual
// OFFSET: 0x007B88D0
void __stdcall CMapRenderChunk::RecreateAlphaBitsFixed8888Mip0(CImVector* dst, uint32_t size, SLayerAlpha* layer, uint8_t* shadowMap) {
    for (uint32_t y = 0; y < size; y++) {
        uint8_t* shadowRow;

        if (shadowMap) {
            shadowRow = shadowMap;
            shadowMap += size >> 3;
        } else {
            shadowRow = s_defaultShadowRow;
        }

        uint8_t* alphaRow;

        if (layer->m_flags & 0x200) {
            alphaRow = s_alphaRowBuf[0];
            layer->m_alphaData = CMapRenderChunk::DecodeAlphaRLE(layer->m_alphaData, s_alphaRowBuf[0], size);
        } else {
            alphaRow = layer->m_alphaData;
            layer->m_alphaData += size;
        }

        for (uint32_t x = 0; x < size; x++) {
            uint32_t alpha = alphaRow[x];

            if ((shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]) {
                alpha = (178 * alpha) >> 8;
            }

            CImVector color;
            color.b = alpha;
            color.g = alpha;
            color.r = alpha;
            color.a = alpha;
            *dst = color;
            dst++;
        }
    }
}

// VERIFY: 7b569ca8 manual
// OFFSET: 0x007B89C0
void __stdcall CMapRenderChunk::RecreateAlphaBitsFixed8888Mip1(CImVector* dst, uint32_t size, SLayerAlpha* layer, uint8_t* shadowMap) {
    for (uint32_t y = 0; y < size; y += 2) {
        uint8_t* shadowRow;

        if (shadowMap) {
            shadowRow = shadowMap;
            shadowMap += size >> 3;
        } else {
            shadowRow = s_defaultShadowRow;
        }

        uint8_t* alphaRow;

        if (layer->m_flags & 0x200) {
            alphaRow = s_alphaRowBuf[0];
            layer->m_alphaData = CMapRenderChunk::DecodeAlphaRLE(layer->m_alphaData, s_alphaRowBuf[0], size);
        } else {
            alphaRow = layer->m_alphaData;
            layer->m_alphaData += size;
        }

        uint32_t x = 0;

        for (; x < size - 2; x += 2) {
            uint32_t alpha = alphaRow[x];

            if ((shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]) {
                alpha = (178 * alpha) >> 8;
            }

            CImVector color;
            color.b = alpha;
            color.g = alpha;
            color.r = alpha;
            color.a = alpha;
            *dst = color;
            dst++;
        }

        x++;

        uint32_t alpha = alphaRow[x];

        if ((shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]) {
            alpha = (178 * alpha) >> 8;
        }

        CImVector color;
        color.b = alpha;
        color.g = alpha;
        color.r = alpha;
        color.a = alpha;
        *dst = color;
        dst++;

        if (y < size - 2) {
            if (shadowMap) {
                shadowMap += size >> 3;
            }

            if (layer->m_flags & 0x200) {
                layer->m_alphaData = CMapRenderChunk::DecodeAlphaRLE(layer->m_alphaData, s_alphaRowBuf[0], size);
            } else {
                layer->m_alphaData += size;
            }
        }

        if (y == size - 4) {
            if (shadowMap) {
                shadowMap += size >> 3;
            }

            if (layer->m_flags & 0x200) {
                layer->m_alphaData = CMapRenderChunk::DecodeAlphaRLE(layer->m_alphaData, s_alphaRowBuf[0], size);
            } else {
                layer->m_alphaData += size;
            }
        }
    }
}

// VERIFY: 24f3ebfc manual
// OFFSET: 0x007B8B80
void CMapRenderChunk::UnpackAlphaBitsFixed8888Mip0(CImVector* dst, uint32_t size, uint8_t* shadowMap) {
    SLayerAlpha layers[4];
    this->CollectLayerAlphaPtrs(this->m_mapChunks[0], layers);

    for (uint32_t y = 0; y < size; y++) {
        uint8_t* shadowRow;
        uint8_t* alphaRows[4];
        CMapRenderChunk::SetupAlphaRows(&shadowMap, &shadowRow, layers, size, alphaRows);

        for (uint32_t x = 0; x < size; x++) {
            uint32_t alpha = 255 - alphaRows[1][x] - alphaRows[3][x] - alphaRows[2][x];

            if ((shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]) {
                alpha = (178 * alpha) >> 8;
            }

            CImVector color;
            color.b = alpha;
            color.g = alpha;
            color.r = alpha;
            color.a = alpha;
            *dst = color;
            dst++;
        }
    }
}

// VERIFY: d75a2a5a manual
// OFFSET: 0x007B8C70
void CMapRenderChunk::UnpackAlphaBitsFixed8888Mip1(CImVector* dst, uint32_t size, uint8_t* shadowMap) {
    SLayerAlpha layers[4];
    this->CollectLayerAlphaPtrs(this->m_mapChunks[0], layers);

    for (uint32_t y = 0; y < size; y += 2) {
        uint8_t* shadowRow;
        uint8_t* alphaRows[4];
        CMapRenderChunk::SetupAlphaRows(&shadowMap, &shadowRow, layers, size, alphaRows);

        uint32_t x = 0;

        for (; x < size - 2; x += 2) {
            uint32_t alpha = 255 - alphaRows[1][x] - alphaRows[3][x] - alphaRows[2][x];

            if ((shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]) {
                alpha = (178 * alpha) >> 8;
            }

            CImVector color;
            color.b = alpha;
            color.g = alpha;
            color.r = alpha;
            color.a = alpha;
            *dst = color;
            dst++;
        }

        x++;

        uint32_t alpha = 255 - alphaRows[3][x] - alphaRows[2][x] - alphaRows[1][x];

        if ((shadowRow[x >> 3] & s_bitMask[x & 7]) >> s_bitShift[x & 7]) {
            alpha = (178 * alpha) >> 8;
        }

        CImVector color;
        color.b = alpha;
        color.g = alpha;
        color.r = alpha;
        color.a = alpha;
        *dst = color;
        dst++;

        if (y < size - 2) {
            CMapRenderChunk::SetupAlphaRows(&shadowMap, &shadowRow, layers, size, alphaRows);
        }

        if (y == size - 4) {
            CMapRenderChunk::SetupAlphaRows(&shadowMap, &shadowRow, layers, size, alphaRows);
        }
    }
}

// VERIFY: 4cab62df manual
// OFFSET: 0x007B8E20
void CMapRenderChunk::UnpackAlphaBits(void* dst, uint32_t size, SLayerAlpha* layerInfo, uint8_t* shadowMap, int32_t layerMode, int32_t bigAlpha) {
    if (bigAlpha) {
        if (layerMode == 3) {
            if (this->m_renderFlags & 0x8) {
                CMapRenderChunk::UnpackAlphaBitsFixed4444Mip1(static_cast<uint16_t*>(dst), size, layerInfo);
            } else {
                CMapRenderChunk::UnpackAlphaBitsFixed4444Mip0(static_cast<uint16_t*>(dst), size, layerInfo);
            }

            return;
        }

        if (layerMode == 2) {
            if (layerInfo->m_alphaData) {
                if (this->m_renderFlags & 0x8) {
                    CMapRenderChunk::RecreateAlphaBitsFixed8888Mip1(static_cast<CImVector*>(dst), size, layerInfo, shadowMap);
                } else {
                    CMapRenderChunk::RecreateAlphaBitsFixed8888Mip0(static_cast<CImVector*>(dst), size, layerInfo, shadowMap);
                }
            } else {
                if (this->m_renderFlags & 0x8) {
                    this->UnpackAlphaBitsFixed8888Mip1(static_cast<CImVector*>(dst), size, shadowMap);
                } else {
                    this->UnpackAlphaBitsFixed8888Mip0(static_cast<CImVector*>(dst), size, shadowMap);
                }
            }

            return;
        }
    } else if (layerMode == 3) {
        if (this->m_renderFlags & 0x8) {
            CMapRenderChunk::UnpackAlphaBitsUnfixed4444Mip1(static_cast<uint16_t*>(dst), size, layerInfo);
        } else {
            CMapRenderChunk::UnpackAlphaBitsUnfixed4444Mip0(static_cast<uint16_t*>(dst), size, layerInfo);
        }

        return;
    }

    SErrDisplayAppFatal("CMapChunk::UnpackAlphaBits(): Bad genformat.");
}

