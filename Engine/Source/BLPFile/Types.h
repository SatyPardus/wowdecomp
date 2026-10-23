#ifndef ENGINE_SOURCE_BLPFILE_TYPES_H
#define ENGINE_SOURCE_BLPFILE_TYPES_H

#include <cstddef>
#include <cstdint>

enum BLP_FILE_MAGIC : uint32_t {
    BLP_MAGIC_BLP2 = 0x32504C42,
};

enum BLP_HEADER_CONSTANTS : uint32_t {
    // Physical header storage, independent of the computed mip count.
    BLP_MIP_SLOTS = 16,
    BLP_PALETTE_ENTRIES = 256,
};

enum COLOR_FILE_FORMAT : uint8_t {
    COLOR_JPEG = 0, // Legacy encoding; JPEG decoding is disabled in this client.
    COLOR_PAL = 1,
    COLOR_DXT = 2,
    COLOR_RAW = 3,
};

enum BLP_ALPHA_BITS : uint8_t {
    ALPHA_0 = 0,
    ALPHA_1 = 1,
    ALPHA_4 = 4,
    ALPHA_8 = 8,
};

struct C4Pixel {
    /* 0000 */ uint8_t m_b;
    /* 0001 */ uint8_t m_g;
    /* 0002 */ uint8_t m_r;
    /* 0003 */ uint8_t m_a;
};

struct BlpPalPixel {
    /* 0000 */ uint8_t m_b;
    /* 0001 */ uint8_t m_g;
    /* 0002 */ uint8_t m_r;
    /* 0003 */ uint8_t m_pad;
};

struct BLPJPEGHeader {
    /* 0000 */ uint32_t m_headerSize;
    /* 0004 */ uint8_t m_headerData[1020];
};

struct BLPHeader {
    /* 0000 */ uint32_t m_magic;
    /* 0004 */ uint32_t m_formatVersion;
    /* 0008 */ uint8_t m_colorEncoding;
    /* 0009 */ uint8_t m_alphaSize;
    /* 000A */ uint8_t m_preferredFormat;
    /* 000B */ uint8_t m_hasMips;
    /* 000C */ uint32_t m_width;
    /* 0010 */ uint32_t m_height;
    /* 0014 */ uint32_t m_mipOffsets[BLP_MIP_SLOTS];
    /* 0054 */ uint32_t m_mipSizes[BLP_MIP_SLOTS];
    /* 0094 */ union {
        BlpPalPixel m_palette[BLP_PALETTE_ENTRIES];
        BLPJPEGHeader m_jpeg;
    } m_extended;
};

struct MipBits {
    /* 0000 */ C4Pixel* m_mip [1];
};

enum MipMapAlgorithm {
    MMA_BOX = 0,
    MMA_CUBIC = 1,
    MMA_FULLDFT = 2,
    MMA_KAISER = 3,
    MMA_LINEARLIGHTKAISER = 4,
};

#endif
