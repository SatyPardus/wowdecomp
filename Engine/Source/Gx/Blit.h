#ifndef ENGINE_SOURCE_GX_BLIT_H
#define ENGINE_SOURCE_GX_BLIT_H

#include <cstdint>
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Tempest/C2iVector.h"
#include "Engine/Source/Tempest/CImVector.h"

enum BlitAlpha {
    BlitAlpha_0 = 0,
    BlitAlpha_1 = 1,
    BlitAlpha_8 = 2,
    BlitAlpha_3 = 3,
    BlitAlphas_Last = 4,
};

typedef void (*BLIT_FUNCTION)(const C2iVector& size, const void* in, uint32_t inStride, void* out, uint32_t outStride);

class CRgb565 {
    public:
    /* 0000 */ uint16_t m_value;

    CRgb565();
};

class CArgb1555 {
    public:
    enum : uint32_t {
        eAlphaMask = 0x8000,
        eNotAlphaMask = 0xFFFF7FFF,
        eRedMask = 0x7C00,
        eNotRedMask = 0xFFFF83FF,
        eGreenMask = 0x3E0,
        eNotGreenMask = 0xFFFFFC1F,
        eBlueMask = 0x1F,
        eNotBlueMask = 0xFFFFFFE0,
    };

    enum : uint32_t {
        eAlphaS = 15,
        eRedS = 10,
        eGreenS = 5,
        eBlueS = 0,
    };

    /* 0000 */ uint16_t m_value;

    CArgb1555();
};

class CArgb4444 {
    public:
    /* 0000 */ uint16_t m_value;

    CArgb4444();
};

class DxtColorBlock {
    public:
    enum : uint32_t {
        BPP = 2,
        PIXEL_LSB_MASK = 3,
    };

    class Tables {
        public:
        /* 0000 */ uint16_t m_oneThird5[32];
        /* 0040 */ uint16_t m_twoThirds5[32];
        /* 0080 */ uint16_t m_oneThird6[64];
        /* 0100 */ uint16_t m_twoThirds6[64];

        Tables();
    };

    static Tables s_tables;

    /* 0000 */ uint16_t m_color0;
    /* 0002 */ uint16_t m_color1;
    /* 0004 */ uint8_t m_indices[4];
};

class Dxt1Block {
    public:
    /* 0000 */ DxtColorBlock m_color;
};

class Dxt3Block {
    public:
    /* 0000 */ uint16_t m_alpha[4];
    /* 0008 */ DxtColorBlock m_color;
};

class Dxt5Block {
    public:
    /* 0000 */ uint8_t m_alpha[2];
    /* 0002 */ uint8_t m_alphaIndices[6];
    /* 0008 */ DxtColorBlock m_color;
};

struct DxtRect {
    /* 0000 */ uint32_t left;
    /* 0004 */ uint32_t top;
    /* 0008 */ uint32_t right;
    /* 000C */ uint32_t bottom;
    /* 0010 */ uint32_t width;
    /* 0014 */ uint32_t height;
};

uint8_t Dxt3A4(uint32_t alpha);
uint8_t Dxt3A8(uint32_t alpha);
uint8_t Dxt5A4(uint32_t alpha);
uint8_t Dxt5A8(uint32_t alpha);
void Dxt5AlphaTable(uint8_t* alphas);

template <class T>
void DxtMakeTable(const DxtColorBlock* block, T* colors);

template <>
void DxtMakeTable<CImVector>(const DxtColorBlock* block, CImVector* colors);

template <>
void DxtMakeTable<CArgb4444>(const DxtColorBlock* block, CArgb4444* colors);

template <class T>
void DxtMakeTableAlpha(const DxtColorBlock* block, T* colors);

template <>
void DxtMakeTableAlpha<CRgb565>(const DxtColorBlock* block, CRgb565* colors);

template <>
void DxtMakeTableAlpha<CArgb1555>(const DxtColorBlock* block, CArgb1555* colors);

template <>
void DxtMakeTableAlpha<CImVector>(const DxtColorBlock* block, CImVector* colors);

void DxtDecodeBlock(const Dxt3Block* block, CImVector** rows, const DxtRect& rect, uint8_t (*alpha)(uint32_t) = Dxt3A8);
void DxtDecodeBlock(const Dxt3Block* block, CArgb4444** rows, const DxtRect& rect, uint8_t (*alpha)(uint32_t) = Dxt3A4);
void DxtDecodeBlock(const Dxt5Block* block, CImVector** rows, const DxtRect& rect, uint8_t (*alpha)(uint32_t) = Dxt5A8);
void DxtDecodeBlock(const Dxt5Block* block, CArgb4444** rows, const DxtRect& rect, uint8_t (*alpha)(uint32_t) = Dxt5A4);
void DxtDecodeBlock(const Dxt1Block* block, CRgb565** rows, const DxtRect& rect);
void DxtDecodeBlock(const Dxt1Block* block, CArgb1555** rows, const DxtRect& rect);
void DxtDecodeBlock(const Dxt1Block* block, CImVector** rows, const DxtRect& rect);

template <class Block, class Pixel>
void Blit_DxtUnaligned(const C2iVector& size, const Block* in, uint32_t inStride, Pixel* out, uint32_t outStride);

template <class Block, class Pixel>
void Blit_Dxt(const C2iVector& size, const Block* in, uint32_t inStride, Pixel* out, uint32_t outStride);

uint32_t CalcRowStride(BlitFormat format, uint32_t width, uint32_t height);
int32_t Blit(const C2iVector& size, BlitAlpha alpha, const void* in, uint32_t inStride, BlitFormat inFormat, void* out, uint32_t outStride, BlitFormat outFormat);

#endif
