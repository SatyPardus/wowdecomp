#ifndef ENGINE_SOURCE_BLPFILE_BLP_H
#define ENGINE_SOURCE_BLPFILE_BLP_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/BLPFile/Types.h"
#include "Engine/Source/Services/Types.h"

class CBLPFile {
    public:
    static uint8_t s_eightBitAlphaLookup[16];
    static uint8_t s_oneBitAlphaLookup[2];

    /* 0000 */ MipBits* m_images;
    /* 0004 */ BLPHeader m_header;
    /* 0498 */ void* m_inMemoryImage;
    /* 049C */ int32_t m_inMemoryNeedsFree;
    /* 04A0 */ uint32_t m_numLevels;
    /* 04A4 */ uint32_t m_quality;
    /* 04A8 */ uint32_t m_colorMapping;
    /* 04AC */ MipMapAlgorithm m_mipMapAlgorithm;
    /* 04B0 */ uint8_t* m_lockDecompMem;

    static int32_t ValidateHeader(const BLPHeader& header);

    CBLPFile();
    ~CBLPFile();

    void Close();
    int32_t GetFormatSize(PIXEL_FORMAT format, uint32_t mipLevel, uint32_t* size, uint32_t* stride) const;
    uint32_t HasMips() const;
    uint32_t Height(uint32_t mipLevel) const;
    int32_t IsValidMip(uint32_t mipLevel) const;
    int32_t Lock(PIXEL_FORMAT format, uint32_t mipLevel, uint8_t*& data, uint32_t& stride);
    int32_t Lock2(const char* fileName, PIXEL_FORMAT format, uint32_t mipLevel, uint8_t* data, uint32_t& stride);
    int32_t LockChain(PIXEL_FORMAT format, MipBits*& images, uint32_t mipLevel);
    int32_t LockChain2(const char* fileName, PIXEL_FORMAT format, MipBits*& images, uint32_t mipLevel, int32_t inPlace);
    int32_t Open(const char* fileName, int32_t flags);
    uint32_t Pixels(uint32_t mipLevel) const;
    int32_t Source(void* data, size_t size);
    int32_t Unlock(uint32_t mipLevel);
    uint32_t Width(uint32_t mipLevel) const;

    protected:
    void DecompPalARGB1555DitherFloydSteinberg(uint8_t* out, const void* in, uint32_t width, uint32_t height);
    void DecompPalARGB2565DitherFloydSteinberg(uint8_t* out, const void* in, uint32_t width, uint32_t height);
    void DecompPalARGB4444DitherFloydSteinberg(uint8_t* out, const void* in, uint32_t width, uint32_t height);
    void DecompPalARGB565DitherFloydSteinberg(uint8_t* out, const void* in, uint32_t width, uint32_t height);
    void DecompPalARGB8888(uint8_t* out, const void* in, uint32_t count);
    int32_t DecompPalDither(PIXEL_FORMAT format, uint32_t mipLevel, uint8_t* out, const void* in);
    void DecompPalFastPath(uint8_t* out, const void* in, uint32_t count);
};

#endif
