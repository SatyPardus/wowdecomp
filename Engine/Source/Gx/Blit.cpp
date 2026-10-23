#include <cstdint>
#include <cstring>
#include "Engine/Source/Gx/Blit.h"
#include "Engine/Source/Tempest/C2iVector.h"
#include "Engine/Source/Tempest/CImVector.h"

// STATIC: 0x00C60930
static BLIT_FUNCTION s_blits[BlitFormats_Last][BlitFormats_Last][BlitAlphas_Last];

// STATIC: 0x00C613D0
DxtColorBlock::Tables DxtColorBlock::s_tables;

// INLINED
CRgb565::CRgb565() {
}

// INLINED
CArgb1555::CArgb1555() {
}

// INLINED
CArgb4444::CArgb4444() {
}

// VERIFY: 6e17f8a6 auto
// OFFSET: 0x006ABC10
uint8_t Dxt3A4(uint32_t alpha) {
    return alpha;
}

// VERIFY: ad402a34 auto
// OFFSET: 0x006ABAB0
uint8_t Dxt3A8(uint32_t alpha) {
    return alpha << 4;
}

// VERIFY: 6659ca19 manual
// OFFSET: 0x006ABAC0
void Dxt5AlphaTable(uint8_t* alphas) {
    int32_t a0 = alphas[0];
    int32_t a1 = alphas[1];

    if (alphas[0] > alphas[1]) {
        alphas[2] = (6 * a0 + a1 + 3) / 7;
        alphas[3] = (5 * a0 + 2 * a1 + 3) / 7;
        alphas[4] = (4 * a0 + 3 * a1 + 3) / 7;
        alphas[5] = (3 * a0 + 4 * a1 + 3) / 7;
        alphas[6] = (2 * a0 + 5 * a1 + 3) / 7;
        alphas[7] = (a0 + 6 * a1 + 3) / 7;
    } else {
        alphas[2] = (4 * a0 + a1 + 2) / 5;
        alphas[3] = (3 * a0 + 2 * a1 + 2) / 5;
        alphas[4] = (2 * a0 + 3 * a1 + 2) / 5;
        alphas[5] = (a0 + 4 * a1 + 2) / 5;
        alphas[6] = 0;
        alphas[7] = 0xFF;
    }
}

// VERIFY: 99da9511 auto
// OFFSET: 0x006ABC00
uint8_t Dxt5A4(uint32_t alpha) {
    return alpha >> 4;
}

// VERIFY: 0f3dc5ac auto
// OFFSET: 0x006ABC10
uint8_t Dxt5A8(uint32_t alpha) {
    return alpha;
}

// VERIFY: d2f6b1f4 manual
// OFFSET: 0x006ABC20
static void ConvertTexels3216(const uint8_t* in, void* out, const C2iVector& size, uint32_t inStride, uint32_t outStride, uint8_t aShr, uint8_t rShr, uint8_t gShr, uint8_t bShr, uint8_t aShl, uint8_t rShl, uint8_t gShl, uint8_t bShl) {
    if (size.x < 2) {
        uint16_t* out16 = static_cast<uint16_t*>(out);

        for (int32_t y = size.y; y; y--) {
            for (int32_t x = 0; x < size.x; x++) {
                const uint8_t* texel = &in[x * 4];
                out16[x] = ((texel[0] >> bShr) << bShl) | ((texel[1] >> gShr) << gShl) | ((texel[3] >> aShr) << aShl) | ((texel[2] >> rShr) << rShl);
            }

            in += inStride;
            out16 = reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(out16) + outStride);
        }
    } else {
        uint32_t* out32 = static_cast<uint32_t*>(out);

        for (int32_t y = size.y; y; y--) {
            for (int32_t x = 0; x < size.x; x += 2) {
                const uint8_t* texel = &in[x * 4];
                out32[x / 2] = ((texel[0] >> bShr) << bShl) | ((texel[1] >> gShr) << gShl) | ((texel[2] >> rShr) << rShl) | ((texel[3] >> aShr) << aShl) | ((texel[4] >> bShr) << (bShl + 16)) | ((texel[5] >> gShr) << (gShl + 16)) | ((texel[7] >> aShr) << (aShl + 16)) | ((texel[6] >> rShr) << (rShl + 16));
            }

            in += inStride;
            out32 = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(out32) + outStride);
        }
    }
}

// VERIFY: 63fb8f6e auto
// OFFSET: 0x006ABE00
static void Blit_Argb8888_Argb4444(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    ConvertTexels3216(static_cast<const uint8_t*>(in), out, size, inStride, outStride, 4, 4, 4, 4, 12, 8, 4, 0);
}

// VERIFY: efe01b4f auto
// OFFSET: 0x006ABE30
static void Blit_Argb8888_Argb1555(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    ConvertTexels3216(static_cast<const uint8_t*>(in), out, size, inStride, outStride, 7, 3, 3, 3, CArgb1555::eAlphaS, CArgb1555::eRedS, CArgb1555::eGreenS, CArgb1555::eBlueS);
}

// VERIFY: 34d329d7 auto
// OFFSET: 0x006ABE60
static void Blit_Argb8888_Rgb565(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    ConvertTexels3216(static_cast<const uint8_t*>(in), out, size, inStride, outStride, 8, 3, 2, 3, 0, 11, 5, 0);
}

// VERIFY: 09a5ad9f manual
// OFFSET: 0x006ABE90
static void Blit_Argb4444_Abgr8888(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    const uint8_t* src = static_cast<const uint8_t*>(in);
    uint8_t* dst = static_cast<uint8_t*>(out);

    for (int32_t y = size.y; y; y--) {
        for (int32_t x = 0; x < size.x; x++) {
            uint32_t texel = reinterpret_cast<const uint16_t*>(src)[x];
            uint8_t a = texel >> 12;
            uint8_t r = (texel >> 8) & 0xF;
            uint8_t g = (texel >> 4) & 0xF;
            uint8_t b = texel & 0xF;
            dst[x * 4 + 3] = (a << 4) | a;
            dst[x * 4 + 1] = (g << 4) | g;
            dst[x * 4 + 2] = (b << 4) | b;
            dst[x * 4 + 0] = (r << 4) | r;
        }

        src += inStride;
        dst += outStride;
    }
}

// VERIFY: 1e40a650 manual
// OFFSET: 0x006ABF40
static void Blit_Argb8888_Abgr8888(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    const uint8_t* src = static_cast<const uint8_t*>(in);
    uint8_t* dst = static_cast<uint8_t*>(out);

    for (int32_t y = size.y; y; y--) {
        for (int32_t x = 0; x < size.x; x++) {
            dst[x * 4 + 3] = src[x * 4 + 3];
            dst[x * 4 + 2] = src[x * 4 + 0];
            dst[x * 4 + 1] = src[x * 4 + 1];
            dst[x * 4 + 0] = src[x * 4 + 2];
        }

        src += inStride;
        dst += outStride;
    }
}

// VERIFY: a0aa5e63 auto
// OFFSET: 0x006ABFC0
static void Blit_Argb8888_Argb8888_A1(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    const uint8_t* src = static_cast<const uint8_t*>(in);
    uint8_t* dst = static_cast<uint8_t*>(out);

    for (int32_t y = size.y; y; y--) {
        const uint32_t* srcRow = reinterpret_cast<const uint32_t*>(src);
        uint32_t* dstRow = reinterpret_cast<uint32_t*>(dst);

        for (int32_t x = 0; x < size.x; x++) {
            if (srcRow[x] & 0xFF000000) {
                dstRow[x] ^= (dstRow[x] ^ srcRow[x]) & 0x00FFFFFF;
            }
        }

        src += inStride;
        dst += outStride;
    }
}

// VERIFY: 66b1bee5 auto
// OFFSET: 0x006AC030
static void Blit_Dxt1_Dxt1(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    int32_t width = size.x < 4 ? 4 : size.x;
    int32_t blockRows = size.y >> 2;
    if (blockRows < 1) {
        blockRows = 1;
    }

    uint32_t rowBytes = width * 2;

    if (rowBytes == inStride && rowBytes == outStride) {
        memcpy(out, in, blockRows * width * 2);
        return;
    }

    const uint8_t* src = static_cast<const uint8_t*>(in);
    uint8_t* dst = static_cast<uint8_t*>(out);

    for (; blockRows; blockRows--) {
        memcpy(dst, src, rowBytes);
        src += inStride;
        dst += outStride;
    }
}

// VERIFY: 17c6637b auto
// OFFSET: 0x006AC0B0
static void Blit_Dxt35_Dxt35(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    int32_t width = size.x < 4 ? 4 : size.x;
    int32_t blockRows = size.y >> 2;
    if (blockRows < 1) {
        blockRows = 1;
    }

    uint32_t rowBytes = width * 4;

    if (rowBytes == inStride && rowBytes == outStride) {
        memcpy(out, in, blockRows * width * 4);
        return;
    }

    const uint8_t* src = static_cast<const uint8_t*>(in);
    uint8_t* dst = static_cast<uint8_t*>(out);

    for (; blockRows; blockRows--) {
        memcpy(dst, src, rowBytes);
        src += inStride;
        dst += outStride;
    }
}

// VERIFY: 3b76b950 auto
// OFFSET: 0x006AC130
uint32_t CalcRowStride(BlitFormat format, uint32_t width, uint32_t height) {
    // STATIC: 0x00AD9038
    static const uint32_t BYTES_PER_BLOCK[BlitFormats_Last] = { 0, 4, 4, 2, 2, 2, 8, 16, 16, 2, 4, 4, 4 };

    // STATIC: 0x00AD906C
    static const uint32_t PIXELS_PER_BLOCK_SHIFT[BlitFormats_Last] = { 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0 };

    if (format >= BlitFormat_Dxt1 && format <= BlitFormat_Dxt5) {
        if (width == 6 * height) {
            width = 6 * (height > 4 ? height : 4);
        } else if (width <= 4) {
            width = 4;
        }
    }

    return BYTES_PER_BLOCK[format] * (width >> PIXELS_PER_BLOCK_SHIFT[format]);
}

// VERIFY: 5c2ff0f7 manual
// OFFSET: 0x006AC190
static void CopyGeneric32(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    if (size.x * 4 == inStride && size.x * 4 == outStride) {
        memcpy(out, in, size.y * size.x * 4);
        return;
    }

    const uint8_t* src = static_cast<const uint8_t*>(in);
    uint8_t* dst = static_cast<uint8_t*>(out);

    for (int32_t y = size.y; y; y--) {
        memcpy(dst, src, size.x * 4);
        src += inStride;
        dst += outStride;
    }
}

// VERIFY: 5837c6d7 auto
// OFFSET: 0x006AC200
static void CopyGeneric16(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    if (size.x * 2 == inStride && size.x * 2 == outStride) {
        memcpy(out, in, size.y * size.x * 2);
        return;
    }

    const uint8_t* src = static_cast<const uint8_t*>(in);
    uint8_t* dst = static_cast<uint8_t*>(out);

    for (int32_t y = size.y; y; y--) {
        memcpy(dst, src, size.x * 2);
        src += inStride;
        dst += outStride;
    }
}

// VERIFY: b4573c84 auto
// OFFSET: 0x006AC270
template <>
void DxtMakeTable<CImVector>(const DxtColorBlock* block, CImVector* colors) {
    const DxtColorBlock::Tables& t = DxtColorBlock::s_tables;
    uint32_t* out = reinterpret_cast<uint32_t*>(colors);

    out[0] = 0xFF000000 | (static_cast<uint8_t>(block->m_color0 >> 11) << 19) | (((block->m_color0 >> 5) & 0x3F) << 10) | ((block->m_color0 & 0x1F) << 3);
    out[1] = 0xFF000000 | (static_cast<uint8_t>(block->m_color1 >> 11) << 19) | (((block->m_color1 >> 5) & 0x3F) << 10) | ((block->m_color1 & 0x1F) << 3);

    uint32_t c0 = block->m_color0;
    uint32_t c1 = block->m_color1;
    uint8_t r = (t.m_oneThird5[c1 >> 11] + t.m_twoThirds5[c0 >> 11]) >> 8;
    uint8_t g = (t.m_oneThird6[(c1 >> 5) & 0x3F] + t.m_twoThirds6[(c0 >> 5) & 0x3F]) >> 8;
    uint8_t b = (t.m_oneThird5[c1 & 0x1F] + t.m_twoThirds5[c0 & 0x1F]) >> 8;
    out[2] = 0xFF000000 | (r << 19) | (g << 10) | (b << 3);

    c0 = block->m_color0;
    c1 = block->m_color1;
    r = (t.m_oneThird5[c0 >> 11] + t.m_twoThirds5[c1 >> 11]) >> 8;
    g = (t.m_oneThird6[(c0 >> 5) & 0x3F] + t.m_twoThirds6[(c1 >> 5) & 0x3F]) >> 8;
    b = (t.m_oneThird5[c0 & 0x1F] + t.m_twoThirds5[c1 & 0x1F]) >> 8;
    out[3] = 0xFF000000 | (r << 19) | (g << 10) | (b << 3);
}

// VERIFY: 534a270c auto
// OFFSET: 0x006AC3F0
template <>
void DxtMakeTable<CArgb4444>(const DxtColorBlock* block, CArgb4444* colors) {
    const DxtColorBlock::Tables& t = DxtColorBlock::s_tables;

    colors[0].m_value = 0xF000 | (((block->m_color0 >> 11) & 0x1E) << 7) | (((block->m_color0 >> 5) & 0x3C) << 2) | ((block->m_color0 >> 1) & 0xF);
    colors[1].m_value = 0xF000 | (((block->m_color1 >> 11) & 0x1E) << 7) | (((block->m_color1 >> 5) & 0x3C) << 2) | ((block->m_color1 >> 1) & 0xF);

    uint32_t c0 = block->m_color0;
    uint32_t c1 = block->m_color1;
    uint32_t r = (t.m_oneThird5[c1 >> 11] + t.m_twoThirds5[c0 >> 11]) >> 8;
    uint32_t g = (t.m_oneThird6[(c1 >> 5) & 0x3F] + t.m_twoThirds6[(c0 >> 5) & 0x3F]) >> 8;
    uint32_t b = (t.m_oneThird5[c1 & 0x1F] + t.m_twoThirds5[c0 & 0x1F]) >> 8;
    colors[2].m_value = 0xF000 | ((r & 0xFE) << 7) | ((g & 0xFC) << 2) | (b >> 1);

    c0 = block->m_color0;
    c1 = block->m_color1;
    r = (t.m_oneThird5[c0 >> 11] + t.m_twoThirds5[c1 >> 11]) >> 8;
    g = (t.m_oneThird6[(c0 >> 5) & 0x3F] + t.m_twoThirds6[(c1 >> 5) & 0x3F]) >> 8;
    b = (t.m_oneThird5[c0 & 0x1F] + t.m_twoThirds5[c1 & 0x1F]) >> 8;
    colors[3].m_value = 0xF000 | ((r & 0xFE) << 7) | ((g & 0xFC) << 2) | (b >> 1);
}

// VERIFY: 5c65b7ef manual
// OFFSET: 0x006AC5D0
template <>
void DxtMakeTableAlpha<CRgb565>(const DxtColorBlock* block, CRgb565* colors) {
    const DxtColorBlock::Tables& t = DxtColorBlock::s_tables;

    if (block->m_color0 <= block->m_color1) {
        colors[0].m_value = block->m_color0;
        colors[1].m_value = block->m_color1;

        int32_t r = ((block->m_color0 >> 11) + (block->m_color1 >> 11)) / 2;
        int32_t g = (((block->m_color0 >> 5) & 0x3F) + ((block->m_color1 >> 5) & 0x3F)) / 2;
        int32_t b = ((block->m_color0 & 0x1F) + (block->m_color1 & 0x1F)) / 2;
        colors[2].m_value = (r << 11) | (g << 5) | b;
        colors[3].m_value = 0;
    } else {
        colors[0].m_value = block->m_color0;
        colors[1].m_value = block->m_color1;

        uint32_t c0 = block->m_color0;
        uint32_t c1 = block->m_color1;
        uint32_t r = (t.m_oneThird5[c1 >> 11] + t.m_twoThirds5[c0 >> 11]) >> 8;
        uint32_t g = (t.m_oneThird6[(c1 >> 5) & 0x3F] + t.m_twoThirds6[(c0 >> 5) & 0x3F]) >> 8;
        uint32_t b = (t.m_oneThird5[c1 & 0x1F] + t.m_twoThirds5[c0 & 0x1F]) >> 8;
        colors[2].m_value = (r << 11) | (g << 5) | b;

        c0 = block->m_color0;
        c1 = block->m_color1;
        r = (t.m_oneThird5[c0 >> 11] + t.m_twoThirds5[c1 >> 11]) >> 8;
        g = (t.m_oneThird6[(c0 >> 5) & 0x3F] + t.m_twoThirds6[(c1 >> 5) & 0x3F]) >> 8;
        b = (t.m_oneThird5[c0 & 0x1F] + t.m_twoThirds5[c1 & 0x1F]) >> 8;
        colors[3].m_value = (r << 11) | (g << 5) | b;
    }
}

// VERIFY: afda2bfb manual
// OFFSET: 0x006AC780
template <>
void DxtMakeTableAlpha<CArgb1555>(const DxtColorBlock* block, CArgb1555* colors) {
    const DxtColorBlock::Tables& t = DxtColorBlock::s_tables;

    if (block->m_color0 <= block->m_color1) {
        colors[0].m_value = CArgb1555::eAlphaMask | (((block->m_color0 >> 11) & 0x1F) << CArgb1555::eRedS) | (((block->m_color0 >> 5) & 0x3E) << 4) | (block->m_color0 & 0x1F);
        colors[1].m_value = CArgb1555::eAlphaMask | (((block->m_color1 >> 11) & 0x1F) << CArgb1555::eRedS) | (((block->m_color1 >> 5) & 0x3E) << 4) | (block->m_color1 & 0x1F);

        int32_t r = ((block->m_color0 >> 11) + (block->m_color1 >> 11)) / 2;
        int32_t g = (((block->m_color0 >> 5) & 0x3F) + ((block->m_color1 >> 5) & 0x3F)) / 2;
        int32_t b = ((block->m_color0 & 0x1F) + (block->m_color1 & 0x1F)) / 2;
        colors[2].m_value = CArgb1555::eAlphaMask | (r << CArgb1555::eRedS) | ((g & 0xFE) << 4) | b;
        colors[3].m_value = 0;
    } else {
        colors[0].m_value = CArgb1555::eAlphaMask | (((block->m_color0 >> 11) & 0x1F) << CArgb1555::eRedS) | (((block->m_color0 >> 5) & 0x3E) << 4) | (block->m_color0 & 0x1F);
        colors[1].m_value = CArgb1555::eAlphaMask | (((block->m_color1 >> 11) & 0x1F) << CArgb1555::eRedS) | (((block->m_color1 >> 5) & 0x3E) << 4) | (block->m_color1 & 0x1F);

        uint32_t c0 = block->m_color0;
        uint32_t c1 = block->m_color1;
        uint32_t r = (t.m_oneThird5[c1 >> 11] + t.m_twoThirds5[c0 >> 11]) >> 8;
        uint32_t g = (t.m_oneThird6[(c1 >> 5) & 0x3F] + t.m_twoThirds6[(c0 >> 5) & 0x3F]) >> 8;
        uint32_t b = (t.m_oneThird5[c1 & 0x1F] + t.m_twoThirds5[c0 & 0x1F]) >> 8;
        colors[2].m_value = CArgb1555::eAlphaMask | (r << CArgb1555::eRedS) | ((g & 0xFE) << 4) | b;

        c0 = block->m_color0;
        c1 = block->m_color1;
        r = (t.m_oneThird5[c0 >> 11] + t.m_twoThirds5[c1 >> 11]) >> 8;
        g = (t.m_oneThird6[(c0 >> 5) & 0x3F] + t.m_twoThirds6[(c1 >> 5) & 0x3F]) >> 8;
        b = (t.m_oneThird5[c0 & 0x1F] + t.m_twoThirds5[c1 & 0x1F]) >> 8;
        colors[3].m_value = CArgb1555::eAlphaMask | (r << CArgb1555::eRedS) | ((g & 0xFE) << 4) | b;
    }
}

// VERIFY: 07710d1a manual
// OFFSET: 0x006ACA10
template <>
void DxtMakeTableAlpha<CImVector>(const DxtColorBlock* block, CImVector* colors) {
    const DxtColorBlock::Tables& t = DxtColorBlock::s_tables;
    uint32_t* out = reinterpret_cast<uint32_t*>(colors);

    if (block->m_color0 <= block->m_color1) {
        out[0] = 0xFF000000 | ((block->m_color0 >> 11) << 19) | (((block->m_color0 >> 5) & 0x3F) << 10) | ((block->m_color0 & 0x1F) << 3);
        out[1] = 0xFF000000 | ((block->m_color1 >> 11) << 19) | (((block->m_color1 >> 5) & 0x3F) << 10) | ((block->m_color1 & 0x1F) << 3);

        int32_t r = ((block->m_color0 >> 11) + (block->m_color1 >> 11)) / 2;
        int32_t g = (((block->m_color0 >> 5) & 0x3F) + ((block->m_color1 >> 5) & 0x3F)) / 2;
        int32_t b = ((block->m_color0 & 0x1F) + (block->m_color1 & 0x1F)) / 2;
        out[2] = 0xFF000000 | (r << 19) | (g << 10) | (b << 3);
        out[3] = 0;
    } else {
        out[0] = 0xFF000000 | ((block->m_color0 >> 11) << 19) | (((block->m_color0 >> 5) & 0x3F) << 10) | ((block->m_color0 & 0x1F) << 3);
        out[1] = 0xFF000000 | ((block->m_color1 >> 11) << 19) | (((block->m_color1 >> 5) & 0x3F) << 10) | ((block->m_color1 & 0x1F) << 3);

        uint32_t c0 = block->m_color0;
        uint32_t c1 = block->m_color1;
        uint8_t r = (t.m_oneThird5[c1 >> 11] + t.m_twoThirds5[c0 >> 11]) >> 8;
        uint8_t g = (t.m_oneThird6[(c1 >> 5) & 0x3F] + t.m_twoThirds6[(c0 >> 5) & 0x3F]) >> 8;
        uint8_t b = (t.m_oneThird5[c1 & 0x1F] + t.m_twoThirds5[c0 & 0x1F]) >> 8;
        out[2] = 0xFF000000 | (r << 19) | (g << 10) | (b << 3);

        c0 = block->m_color0;
        c1 = block->m_color1;
        r = (t.m_oneThird5[c0 >> 11] + t.m_twoThirds5[c1 >> 11]) >> 8;
        g = (t.m_oneThird6[(c0 >> 5) & 0x3F] + t.m_twoThirds6[(c1 >> 5) & 0x3F]) >> 8;
        b = (t.m_oneThird5[c0 & 0x1F] + t.m_twoThirds5[c1 & 0x1F]) >> 8;
        out[3] = 0xFF000000 | (r << 19) | (g << 10) | (b << 3);
    }
}

// VERIFY: 0ba5f832 auto
// OFFSET: 0x006ACCE0
static void Blit_generic32_generic32(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    CopyGeneric32(size, in, inStride, out, outStride);
}

// VERIFY: 8572017d manual
// OFFSET: 0x006ACCF0
static void Blit_Argb8888_Argb8888_A8(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    const uint8_t* src = static_cast<const uint8_t*>(in);
    uint8_t* dst = static_cast<uint8_t*>(out);

    for (int32_t y = size.y; y; y--) {
        const CImVector* srcRow = reinterpret_cast<const CImVector*>(src);
        CImVector* dstRow = reinterpret_cast<CImVector*>(dst);

        for (int32_t x = 0; x < size.x; x++) {
            if ((*reinterpret_cast<const uint32_t*>(&srcRow[x]) & 0xFF000000) && srcRow[x].a) {
                dstRow[x].InterpColor(srcRow[x].a, &srcRow[x]);
            }
        }

        src += inStride;
        dst += outStride;
    }
}

// VERIFY: de295cdd auto
// OFFSET: 0x006ACD70
static void Blit_generic16_generic16(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    CopyGeneric16(size, in, inStride, out, outStride);
}

// VERIFY: 1d7e740e manual
// OFFSET: 0x006ACD80
void DxtDecodeBlock(const Dxt3Block* block, CImVector** rows, const DxtRect& rect, uint8_t (*alpha)(uint32_t)) {
    // STATIC: 0x00C6082C
    static CImVector colors[4];

    DxtMakeTable(&block->m_color, colors);

    for (uint32_t y = rect.top; y <= rect.bottom; y++) {
        uint32_t colorBits = block->m_color.m_indices[y] >> (DxtColorBlock::BPP * rect.left);
        uint32_t alphaBits = block->m_alpha[y] >> (4 * rect.left);

        for (uint32_t x = rect.left; x <= rect.right; x++) {
            uint8_t a = alpha(alphaBits & 0xF);
            alphaBits >>= 4;

            CImVector& pixel = rows[y][x];
            pixel.a = a;
            pixel.r = colors[colorBits & DxtColorBlock::PIXEL_LSB_MASK].r;
            pixel.g = colors[colorBits & DxtColorBlock::PIXEL_LSB_MASK].g;
            pixel.b = colors[colorBits & DxtColorBlock::PIXEL_LSB_MASK].b;
            colorBits >>= DxtColorBlock::BPP;
        }

        rows[y] += rect.width;
    }
}

// VERIFY: 72a5c086 manual
// OFFSET: 0x006ACEA0
void DxtDecodeBlock(const Dxt3Block* block, CArgb4444** rows, const DxtRect& rect, uint8_t (*alpha)(uint32_t)) {
    // STATIC: 0x00C60840
    static CArgb4444 colors[4];

    DxtMakeTable(&block->m_color, colors);

    for (uint32_t y = rect.top; y <= rect.bottom; y++) {
        uint32_t colorBits = block->m_color.m_indices[y] >> (DxtColorBlock::BPP * rect.left);
        uint32_t alphaBits = block->m_alpha[y] >> (4 * rect.left);

        for (uint32_t x = rect.left; x <= rect.right; x++) {
            rows[y][x].m_value = (alpha(alphaBits & 0xF) << 12) | (colors[colorBits & DxtColorBlock::PIXEL_LSB_MASK].m_value & 0xFFF);
            colorBits >>= DxtColorBlock::BPP;
            alphaBits >>= 4;
        }

        rows[y] += rect.width;
    }
}

// VERIFY: d8b4df81 manual
// OFFSET: 0x006ACF90
void DxtDecodeBlock(const Dxt5Block* block, CImVector** rows, const DxtRect& rect, uint8_t (*alpha)(uint32_t)) {
    // STATIC: 0x00C6084C
    static CImVector colors[4];

    DxtMakeTable(&block->m_color, colors);

    uint8_t alphas[8];
    alphas[0] = block->m_alpha[0];
    alphas[1] = block->m_alpha[1];
    Dxt5AlphaTable(alphas);

    uint32_t rowBit = 3 * (rect.left + 4 * rect.top);

    for (uint32_t y = rect.top; y <= rect.bottom; y++) {
        uint32_t bit = rowBit;
        uint32_t colorBits = block->m_color.m_indices[y] >> (DxtColorBlock::BPP * rect.left);

        for (uint32_t x = rect.left; x <= rect.right; x++) {
            const CImVector& color = colors[colorBits & DxtColorBlock::PIXEL_LSB_MASK];
            uint32_t index = ((block->m_alphaIndices[bit >> 3] >> (bit & 7)) | (block->m_alphaIndices[(bit >> 3) + 1] << (8 - (bit & 7)))) & 7;
            uint8_t a = alpha(alphas[index]);
            colorBits >>= DxtColorBlock::BPP;

            CImVector& pixel = rows[y][x];
            pixel.a = a;
            pixel.r = color.r;
            pixel.g = color.g;
            pixel.b = color.b;
            bit += 3;
        }

        rowBit += 12;
        rows[y] += rect.width;
    }
}

// VERIFY: bd20ea45 auto
// OFFSET: 0x006AD0E0
void DxtDecodeBlock(const Dxt5Block* block, CArgb4444** rows, const DxtRect& rect, uint8_t (*alpha)(uint32_t)) {
    // STATIC: 0x00C60860
    static CArgb4444 colors[4];

    DxtMakeTable(&block->m_color, colors);

    uint8_t alphas[8];
    alphas[0] = block->m_alpha[0];
    alphas[1] = block->m_alpha[1];
    Dxt5AlphaTable(alphas);

    uint32_t rowBit = 3 * (rect.left + 4 * rect.top);

    for (uint32_t y = rect.top; y <= rect.bottom; y++) {
        uint32_t bit = rowBit;
        uint32_t colorBits = block->m_color.m_indices[y] >> (DxtColorBlock::BPP * rect.left);

        for (uint32_t x = rect.left; x <= rect.right; x++) {
            uint32_t index = ((block->m_alphaIndices[bit >> 3] >> (bit & 7)) | (block->m_alphaIndices[(bit >> 3) + 1] << (8 - (bit & 7)))) & 7;
            rows[y][x].m_value = (alpha(alphas[index]) << 12) | (colors[colorBits & DxtColorBlock::PIXEL_LSB_MASK].m_value & 0xFFF);
            colorBits >>= DxtColorBlock::BPP;
            bit += 3;
        }

        rowBit += 12;
        rows[y] += rect.width;
    }
}

// VERIFY: 3ccf618a manual
// OFFSET: 0x006AD220
void DxtDecodeBlock(const Dxt1Block* block, CRgb565** rows, const DxtRect& rect) {
    // STATIC: 0x00C6086C
    static CRgb565 colors[4];

    DxtMakeTableAlpha(&block->m_color, colors);

    for (uint32_t y = rect.top; y <= rect.bottom; y++) {
        uint32_t bits = block->m_color.m_indices[y] >> (DxtColorBlock::BPP * rect.left);

        for (uint32_t x = rect.left; x <= rect.right; x++) {
            rows[y][x] = colors[bits & DxtColorBlock::PIXEL_LSB_MASK];
            bits >>= DxtColorBlock::BPP;
        }

        rows[y] += rect.width;
    }
}

// VERIFY: 67147bef manual
// OFFSET: 0x006AD2D0
void DxtDecodeBlock(const Dxt1Block* block, CArgb1555** rows, const DxtRect& rect) {
    // STATIC: 0x00C60878
    static CArgb1555 colors[4];

    DxtMakeTableAlpha(&block->m_color, colors);

    for (uint32_t y = rect.top; y <= rect.bottom; y++) {
        uint32_t bits = block->m_color.m_indices[y] >> (DxtColorBlock::BPP * rect.left);

        for (uint32_t x = rect.left; x <= rect.right; x++) {
            rows[y][x] = colors[bits & DxtColorBlock::PIXEL_LSB_MASK];
            bits >>= DxtColorBlock::BPP;
        }

        rows[y] += rect.width;
    }
}

// VERIFY: 70602d29 manual
// OFFSET: 0x006AD380
void DxtDecodeBlock(const Dxt1Block* block, CImVector** rows, const DxtRect& rect) {
    // STATIC: 0x00C60884
    static CImVector colors[4];

    DxtMakeTableAlpha(&block->m_color, colors);

    for (uint32_t y = rect.top; y <= rect.bottom; y++) {
        uint32_t bits = block->m_color.m_indices[y] >> (DxtColorBlock::BPP * rect.left);

        for (uint32_t x = rect.left; x <= rect.right; x++) {
            rows[y][x] = colors[bits & DxtColorBlock::PIXEL_LSB_MASK];
            bits >>= DxtColorBlock::BPP;
        }

        rows[y] += rect.width;
    }
}

// VERIFY: 32f80391 manual
// OFFSET: [0x006AD440, 0x006AD660, 0x006AD880, 0x006ADAE0, 0x006ADD20, 0x006ADF90, 0x006AE1D0]
template <class Block, class Pixel>
void Blit_DxtUnaligned(const C2iVector& size, const Block* in, uint32_t inStride, Pixel* out, uint32_t outStride) {
    // STATIC: [0x00C608A8, 0x00C608D8, 0x00C60908]
    static Pixel colors[4];

    int32_t height = size.y;
    bool cube = size.x == 6 * size.y;
    int32_t faces = cube ? 6 : 1;
    int32_t width = cube ? size.x / 6 : size.x;

    for (int32_t face = 0; face < faces; face++) {
        for (int32_t y = 0; y < height; y += 4) {
            const Block* block = reinterpret_cast<const Block*>(reinterpret_cast<const uint8_t*>(in) + inStride * (y >> 2));
            Pixel* rows[4];
            rows[0] = out;
            rows[1] = reinterpret_cast<Pixel*>(reinterpret_cast<uint8_t*>(out) + outStride);
            rows[2] = reinterpret_cast<Pixel*>(reinterpret_cast<uint8_t*>(out) + 2 * outStride);
            rows[3] = reinterpret_cast<Pixel*>(reinterpret_cast<uint8_t*>(out) + 3 * outStride);

            for (int32_t x = 0; x < width; x += 4) {
                int32_t right = width - 1 - x;
                int32_t bottom = height - 1 - y;

                DxtRect rect;
                rect.left = 0;
                rect.top = 0;
                rect.right = right < 3 ? right : 3;
                rect.bottom = bottom < 3 ? bottom : 3;
                rect.width = rect.right + 1;
                rect.height = rect.bottom + 1;

                DxtDecodeBlock(block, rows, rect);
                block++;
            }
        }

        in += width;
        out += width;
    }
}

// VERIFY: 4f0b7f9c manual
// OFFSET: [0x006AD5B0, 0x006AD7D0, 0x006ADA10, 0x006ADC60, 0x006ADEB0, 0x006AE110, 0x006AE360]
template <class Block, class Pixel>
void Blit_Dxt(const C2iVector& size, const Block* in, uint32_t inStride, Pixel* out, uint32_t outStride) {
    // STATIC: [0x00C608BC, 0x00C608EC, 0x00C6091C]
    static Pixel colors[4];

    for (int32_t y = 0; y < size.y; y += 4) {
        Pixel* rows[4];
        rows[0] = out;
        rows[1] = reinterpret_cast<Pixel*>(reinterpret_cast<uint8_t*>(out) + outStride);
        rows[2] = reinterpret_cast<Pixel*>(reinterpret_cast<uint8_t*>(out) + 2 * outStride);
        rows[3] = reinterpret_cast<Pixel*>(reinterpret_cast<uint8_t*>(out) + 3 * outStride);
        out = reinterpret_cast<Pixel*>(reinterpret_cast<uint8_t*>(out) + 4 * outStride);

        const Block* block = in;

        for (int32_t x = 0; x < size.x; x += 4) {
            DxtRect rect;
            rect.left = 0;
            rect.top = 0;
            rect.right = 3;
            rect.bottom = 3;
            rect.width = 4;
            rect.height = 4;

            DxtDecodeBlock(block, rows, rect);
            block++;
        }

        in = reinterpret_cast<const Block*>(reinterpret_cast<const uint8_t*>(in) + inStride);
    }
}

// VERIFY: f5c84b9d manual
// OFFSET: 0x006AE440
static void Blit_Dxt1_Rgb565(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
        Blit_DxtUnaligned(size, static_cast<const Dxt1Block*>(in), inStride, static_cast<CRgb565*>(out), outStride);
    } else {
        Blit_Dxt(size, static_cast<const Dxt1Block*>(in), inStride, static_cast<CRgb565*>(out), outStride);
    }
}

// VERIFY: 9212be0b manual
// OFFSET: 0x006AE4A0
static void Blit_Dxt1_Argb1555(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
        Blit_DxtUnaligned(size, static_cast<const Dxt1Block*>(in), inStride, static_cast<CArgb1555*>(out), outStride);
    } else {
        Blit_Dxt(size, static_cast<const Dxt1Block*>(in), inStride, static_cast<CArgb1555*>(out), outStride);
    }
}

// VERIFY: ad77c98f manual
// OFFSET: 0x006AE500
static void Blit_Dxt1_Argb8888(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
        Blit_DxtUnaligned(size, static_cast<const Dxt1Block*>(in), inStride, static_cast<CImVector*>(out), outStride);
    } else {
        Blit_Dxt(size, static_cast<const Dxt1Block*>(in), inStride, static_cast<CImVector*>(out), outStride);
    }
}

// VERIFY: 170b801d manual
// OFFSET: 0x006AE560
static void Blit_Dxt3_Argb4444(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
        Blit_DxtUnaligned(size, static_cast<const Dxt3Block*>(in), inStride, static_cast<CArgb4444*>(out), outStride);
    } else {
        Blit_Dxt(size, static_cast<const Dxt3Block*>(in), inStride, static_cast<CArgb4444*>(out), outStride);
    }
}

// VERIFY: 123b8062 manual
// OFFSET: 0x006AE5C0
static void Blit_Dxt3_Argb8888(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
        Blit_DxtUnaligned(size, static_cast<const Dxt3Block*>(in), inStride, static_cast<CImVector*>(out), outStride);
    } else {
        Blit_Dxt(size, static_cast<const Dxt3Block*>(in), inStride, static_cast<CImVector*>(out), outStride);
    }
}

// VERIFY: 709f51ad manual
// OFFSET: 0x006AE620
static void Blit_Dxt5_Argb4444(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
        Blit_DxtUnaligned(size, static_cast<const Dxt5Block*>(in), inStride, static_cast<CArgb4444*>(out), outStride);
    } else {
        Blit_Dxt(size, static_cast<const Dxt5Block*>(in), inStride, static_cast<CArgb4444*>(out), outStride);
    }
}

// VERIFY: a2fd280c manual
// OFFSET: 0x006AE680
static void Blit_Dxt5_Argb8888(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride) {
    if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
        Blit_DxtUnaligned(size, static_cast<const Dxt5Block*>(in), inStride, static_cast<CImVector*>(out), outStride);
    } else {
        Blit_Dxt(size, static_cast<const Dxt5Block*>(in), inStride, static_cast<CImVector*>(out), outStride);
    }
}

// VERIFY: 93653188 auto
// OFFSET: 0x006AE6E0
static void InitBlit() {
    s_blits[BlitFormat_Argb8888][BlitFormat_Abgr8888][BlitAlpha_0] = Blit_Argb8888_Abgr8888;
    s_blits[BlitFormat_Argb8888][BlitFormat_Argb8888][BlitAlpha_0] = Blit_generic32_generic32;
    s_blits[BlitFormat_Argb8888][BlitFormat_Argb8888][BlitAlpha_1] = Blit_Argb8888_Argb8888_A1;
    s_blits[BlitFormat_Argb8888][BlitFormat_Argb8888][BlitAlpha_8] = Blit_Argb8888_Argb8888_A8;
    s_blits[BlitFormat_Argb8888][BlitFormat_Argb4444][BlitAlpha_0] = Blit_Argb8888_Argb4444;
    s_blits[BlitFormat_Argb8888][BlitFormat_Argb1555][BlitAlpha_0] = Blit_Argb8888_Argb1555;
    s_blits[BlitFormat_Argb8888][BlitFormat_Rgb565][BlitAlpha_0] = Blit_Argb8888_Rgb565;
    s_blits[BlitFormat_Rgb565][BlitFormat_Rgb565][BlitAlpha_0] = Blit_generic16_generic16;
    s_blits[BlitFormat_Argb4444][BlitFormat_Abgr8888][BlitAlpha_0] = Blit_Argb4444_Abgr8888;
    s_blits[BlitFormat_Argb4444][BlitFormat_Argb4444][BlitAlpha_0] = Blit_generic16_generic16;
    s_blits[BlitFormat_Argb1555][BlitFormat_Argb1555][BlitAlpha_0] = Blit_generic16_generic16;
    s_blits[BlitFormat_Dxt1][BlitFormat_Dxt1][BlitAlpha_0] = Blit_Dxt1_Dxt1;
    s_blits[BlitFormat_Dxt3][BlitFormat_Dxt3][BlitAlpha_0] = Blit_Dxt35_Dxt35;
    s_blits[BlitFormat_Dxt5][BlitFormat_Dxt5][BlitAlpha_0] = Blit_Dxt35_Dxt35;
    s_blits[BlitFormat_Dxt1][BlitFormat_Rgb565][BlitAlpha_0] = Blit_Dxt1_Rgb565;
    s_blits[BlitFormat_Dxt1][BlitFormat_Argb1555][BlitAlpha_0] = Blit_Dxt1_Argb1555;
    s_blits[BlitFormat_Dxt1][BlitFormat_Argb8888][BlitAlpha_0] = Blit_Dxt1_Argb8888;
    s_blits[BlitFormat_Dxt3][BlitFormat_Argb4444][BlitAlpha_0] = Blit_Dxt3_Argb4444;
    s_blits[BlitFormat_Dxt3][BlitFormat_Argb8888][BlitAlpha_0] = Blit_Dxt3_Argb8888;
    s_blits[BlitFormat_Dxt5][BlitFormat_Argb4444][BlitAlpha_0] = Blit_Dxt5_Argb4444;
    s_blits[BlitFormat_Dxt5][BlitFormat_Argb8888][BlitAlpha_0] = Blit_Dxt5_Argb8888;
    s_blits[BlitFormat_Uv88][BlitFormat_Uv88][BlitAlpha_0] = Blit_generic16_generic16;
    s_blits[BlitFormat_Gr1616F][BlitFormat_Gr1616F][BlitAlpha_0] = Blit_generic32_generic32;
    s_blits[BlitFormat_R32F][BlitFormat_R32F][BlitAlpha_0] = Blit_generic32_generic32;
    s_blits[BlitFormat_D24X8][BlitFormat_D24X8][BlitAlpha_0] = Blit_generic32_generic32;
}

// VERIFY: f1f74b3d auto
// OFFSET: 0x006AE7C0
int32_t Blit(const C2iVector& size, BlitAlpha alpha, const void* in, uint32_t inStride, BlitFormat inFormat, void* out, uint32_t outStride, BlitFormat outFormat) {
    // STATIC: 0x00C613C4
    static int32_t initBlit;

    if (!initBlit) {
        InitBlit();
        initBlit = 1;
    }

    BLIT_FUNCTION blit = s_blits[inFormat][outFormat][alpha];
    if (!blit) {
        return 0;
    }

    blit(size, in, inStride, out, outStride);

    return 1;
}

// VERIFY: dbdfa352 manual
// OFFSET: 0x006AE820
DxtColorBlock::Tables::Tables() {
    for (uint32_t i = 0; i < 32; i++) {
        this->m_oneThird5[i] = (i << 8) / 3 + 1;
        this->m_twoThirds5[i] = (i << 9) / 3 + 1;
    }

    for (uint32_t i = 0; i < 64; i++) {
        this->m_oneThird6[i] = (i << 8) / 3 + 1;
        this->m_twoThirds6[i] = (i << 9) / 3 + 1;
    }
}
