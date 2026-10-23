#include <cstdint>
#include <cstring>
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/BLPFile/blp.h"
#include "Engine/Source/Gx/Blit.h"
#include "Engine/Source/Images/MippedImg.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2iVector.h"

// STATIC: 0x00AD90B0
uint8_t CBLPFile::s_eightBitAlphaLookup[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };

// STATIC: 0x00AD90C0
uint8_t CBLPFile::s_oneBitAlphaLookup[2] = { 0x00, 0xFF };

// STATIC: 0x00AD90C4
static uint16_t s_oneBitAlpha4444[2] = { 0x0000, 0xF000 };

// STATIC: 0x00AD9188
static uint8_t s_twoBitAlphaMask[4] = { 0x03, 0x0C, 0x30, 0xC0 };

// STATIC: 0x00AD918C
static uint32_t s_twoBitAlphaShift[4] = { 0, 2, 4, 6 };

// STATIC: 0x00AD91A0
static uint32_t s_fourBitAlphaShift[2] = { 2, 6 };

// STATIC: 0x00C79610
static TSGrowableArray<uint8_t> s_blpFileLoadBuffer;

// INLINED
static BlitFormat GetBlitFormat(PIXEL_FORMAT format) {
    // STATIC: 0x00AD91BC
    static BlitFormat blitFmt[NUM_PIXEL_FORMATS] = {
        BlitFormat_Dxt1,
        BlitFormat_Dxt3,
        BlitFormat_Argb8888,
        BlitFormat_Argb1555,
        BlitFormat_Argb4444,
        BlitFormat_Rgb565,
        BlitFormat_Unknown,
        BlitFormat_Dxt5,
        BlitFormat_Unknown,
        BlitFormat_Unknown,
    };

    return blitFmt[format];
}

// VERIFY: 57cc221c manual
// OFFSET: 0x004B58D0
CBLPFile::CBLPFile() {
    this->m_images = nullptr;
    this->m_quality = 100;
    memset(&this->m_header, 0, sizeof(this->m_header));
    this->m_header.m_hasMips &= ~0x10;
    this->m_header.m_magic = BLP_MAGIC_BLP2;
    this->m_header.m_formatVersion = 1;
    this->m_header.m_preferredFormat = PIXEL_ARGB8888;
    this->m_inMemoryImage = nullptr;
    this->m_mipMapAlgorithm = MMA_BOX;
}

// INLINED
CBLPFile::~CBLPFile() {
    this->Close();
}

// INLINED
uint32_t CBLPFile::HasMips() const {
    return this->m_header.m_hasMips & 0xF;
}

// INLINED
int32_t CBLPFile::IsValidMip(uint32_t mipLevel) const {
    return !mipLevel || (this->HasMips() && mipLevel < this->m_numLevels);
}

// VERIFY: e564938b manual
// OFFSET: 0x006AE8B0
void CBLPFile::Close() {
    this->m_inMemoryImage = nullptr;

    if (this->m_images) {
        SMemFree(this->m_images, __FILE__, __LINE__, 0);
    }

    this->m_images = nullptr;
}

// VERIFY: 507be47e auto
// OFFSET: 0x006AE8E0
int32_t CBLPFile::ValidateHeader(const BLPHeader& header) {
    return header.m_magic == BLP_MAGIC_BLP2 && header.m_formatVersion == 1;
}

// VERIFY: ec3a562c manual
// OFFSET: 0x006AE900
int32_t CBLPFile::Source(void* data) {
    this->Close();

    this->m_inMemoryNeedsFree = 0;
    this->m_inMemoryImage = data;
    memcpy(&this->m_header, data, sizeof(this->m_header));

    if (!CBLPFile::ValidateHeader(this->m_header)) {
        return 0;
    }

    if (this->HasMips()) {
        this->m_numLevels = CalcLevelCount(this->m_header.m_width, this->m_header.m_height);
    } else {
        this->m_numLevels = 1;
    }

    return 1;
}

// VERIFY: 3d010c6b manual
// OFFSET: 0x006AE990
void CBLPFile::DecompPalFastPath(uint8_t* out, const void* in, uint32_t count) {
    BlpPalPixel* pixels = reinterpret_cast<BlpPalPixel*>(out);
    const uint8_t* indices = static_cast<const uint8_t*>(in);

    for (uint32_t i = count; i; i--) {
        *pixels = this->m_header.m_extended.m_palette[*indices];
        pixels->m_pad = indices[count];
        pixels++;
        indices++;
    }
}

// VERIFY: 9923d899 manual
// OFFSET: 0x006AE9E0
void CBLPFile::DecompPalARGB8888(uint8_t* out, const void* in, uint32_t count) {
    BlpPalPixel* pixels = reinterpret_cast<BlpPalPixel*>(out);
    const uint8_t* indices = static_cast<const uint8_t*>(in);

    for (uint32_t i = 0; i < count; i++) {
        pixels[i] = this->m_header.m_extended.m_palette[*indices];
        pixels[i].m_pad = 0xFF;
        indices++;
    }

    const uint8_t* alpha = indices;

    switch (this->m_header.m_alphaSize) {
        case ALPHA_1: {
            uint32_t bytes = count >> 3;

            for (uint32_t i = 0; i < bytes; i++) {
                uint8_t bits = alpha[i];

                for (uint32_t bit = 0; bit < 8; bit++) {
                    pixels->m_pad = CBLPFile::s_oneBitAlphaLookup[bits & 1];
                    bits >>= 1;
                    pixels++;
                }
            }

            if (count & 7) {
                uint8_t bits = alpha[bytes];

                for (uint32_t bit = count & 7; bit; bit--) {
                    pixels->m_pad = CBLPFile::s_oneBitAlphaLookup[bits & 1];
                    bits >>= 1;
                    pixels++;
                }
            }

            break;
        }

        case ALPHA_4: {
            for (uint32_t i = count >> 1; i; i--) {
                pixels[0].m_pad = CBLPFile::s_eightBitAlphaLookup[*alpha & 0xF];
                pixels[1].m_pad = CBLPFile::s_eightBitAlphaLookup[*alpha >> 4];
                pixels += 2;
                alpha++;
            }

            if (count & 1) {
                pixels->m_pad = CBLPFile::s_eightBitAlphaLookup[*alpha & 0xF];
            }

            break;
        }

        case ALPHA_8: {
            for (uint32_t i = 0; i < count; i++) {
                pixels[i].m_pad = *alpha;
                alpha++;
            }

            break;
        }
    }
}

// VERIFY: 3802cef8 manual
// OFFSET: 0x006AEBA0
void CBLPFile::DecompPalARGB4444DitherFloydSteinberg(uint8_t* out, const void* in, uint32_t width, uint32_t height) {
    // STATIC: 0x00C61550
    static int32_t s_errors[2][1026][3];

    uint16_t* pixels = reinterpret_cast<uint16_t*>(out);
    const uint8_t* indices = static_cast<const uint8_t*>(in);

    memset(s_errors, 0, (width + 2) * sizeof(s_errors[0][0]));

    for (uint32_t y = 0; y < height; y++) {
        int32_t (*next)[3] = &s_errors[(y - 1) & 1][1];
        int32_t (*current)[3] = &s_errors[y & 1][1];

        next[-1][1] = 0;
        next[-1][2] = 0;
        next[-1][0] = 0;
        next[0][2] = 0;
        next[0][1] = 0;
        next[0][0] = 0;

        for (uint32_t x = 0; x < width; x++) {
            const BlpPalPixel& color = this->m_header.m_extended.m_palette[indices[x]];

            int32_t r = (current[x][0] >> 4) + (color.m_r << 16);
            int32_t g = (current[x][1] >> 4) + (color.m_g << 16);
            int32_t b = (current[x][2] >> 4) + (color.m_b << 16);

            int32_t quantB = (b + 0x80000) & 0xFFF00000;
            int32_t quantG = (g + 0x80000) & 0xFFF00000;
            int32_t quantR = (r + 0x80000) & 0xFFF00000;

            int32_t outR = quantR >> 20;
            int32_t outG = quantG >> 20;
            int32_t outB = quantB >> 20;

            outR = outR < 0 ? 0 : (outR > 15 ? 15 : outR);
            outG = outG < 0 ? 0 : (outG > 15 ? 15 : outG);
            outB = outB < 0 ? 0 : (outB > 15 ? 15 : outB);

            pixels[x] = static_cast<uint16_t>((((outR << 4) | outG) << 4) | outB);

            int32_t errorR = r - quantR;
            int32_t errorG = g - quantG;
            int32_t errorB = b - quantB;

            current[x + 1][0] += 7 * errorR;
            current[x + 1][1] += 7 * errorG;
            current[x + 1][2] += 7 * errorB;
            (next + x)[-1][0] += 5 * errorR;
            (next + x)[-1][1] += 5 * errorG;
            (next + x)[-1][2] += 5 * errorB;
            next[x][0] += 3 * errorR;
            next[x][1] += 3 * errorG;
            next[x][2] += 3 * errorB;
            next[x + 1][1] = errorG;
            next[x + 1][2] = errorB;
            next[x + 1][0] = errorR;
        }

        indices += width;
        pixels += width;
    }

    uint32_t count = width * height;
    uint16_t* pixel = reinterpret_cast<uint16_t*>(out);
    const uint8_t* alpha = indices;

    switch (this->m_header.m_alphaSize) {
        case ALPHA_1: {
            uint32_t mask = 1;

            for (; count; count--) {
                *pixel |= s_oneBitAlpha4444[*alpha & mask];
                mask += mask;
                pixel++;

                if (mask >= 0x100) {
                    mask = 1;
                    alpha++;
                }
            }

            break;
        }

        case ALPHA_4: {
            for (uint32_t i = 0; i < count; i++) {
                if (i & 1) {
                    *pixel |= (*alpha & 0xF0) << 8;
                    alpha++;
                } else {
                    *pixel |= static_cast<uint16_t>(*alpha << 12);
                }

                pixel++;
            }

            break;
        }

        case ALPHA_8: {
            for (; count; count--) {
                *pixel |= (*alpha & 0xF0) << 8;
                pixel++;
                alpha++;
            }

            break;
        }
    }
}

// VERIFY: c92f17c6 manual
// OFFSET: 0x006AEE70
void CBLPFile::DecompPalARGB1555DitherFloydSteinberg(uint8_t* out, const void* in, uint32_t width, uint32_t height) {
    // STATIC: 0x00C67580
    static int32_t s_errors[2][1026][3];

    uint16_t* pixels = reinterpret_cast<uint16_t*>(out);
    const uint8_t* indices = static_cast<const uint8_t*>(in);

    memset(s_errors, 0, (width + 2) * sizeof(s_errors[0][0]));

    for (uint32_t y = 0; y < height; y++) {
        int32_t (*next)[3] = &s_errors[(y - 1) & 1][1];
        int32_t (*current)[3] = &s_errors[y & 1][1];

        next[-1][1] = 0;
        next[-1][2] = 0;
        next[-1][0] = 0;
        next[0][2] = 0;
        next[0][1] = 0;
        next[0][0] = 0;

        for (uint32_t x = 0; x < width; x++) {
            const BlpPalPixel& color = this->m_header.m_extended.m_palette[indices[x]];

            int32_t r = (current[x][0] >> 4) + (color.m_r << 16);
            int32_t g = (current[x][1] >> 4) + (color.m_g << 16);
            int32_t b = (current[x][2] >> 4) + (color.m_b << 16);

            int32_t quantB = (b + 0x40000) & 0xFFF80000;
            int32_t quantG = (g + 0x40000) & 0xFFF80000;
            int32_t quantR = (r + 0x40000) & 0xFFF80000;

            int32_t outR = quantR >> 19;
            int32_t outG = quantG >> 19;
            int32_t outB = quantB >> 19;

            outR = outR < 0 ? 0 : (outR > 31 ? 31 : outR);
            outG = outG < 0 ? 0 : (outG > 31 ? 31 : outG);
            outB = outB < 0 ? 0 : (outB > 31 ? 31 : outB);

            pixels[x] = static_cast<uint16_t>((((outR << 5) | outG) << 5) | outB);

            int32_t errorR = r - quantR;
            int32_t errorG = g - quantG;
            int32_t errorB = b - quantB;

            current[x + 1][0] += 7 * errorR;
            current[x + 1][1] += 7 * errorG;
            current[x + 1][2] += 7 * errorB;
            (next + x)[-1][0] += 5 * errorR;
            (next + x)[-1][1] += 5 * errorG;
            (next + x)[-1][2] += 5 * errorB;
            next[x][0] += 3 * errorR;
            next[x][1] += 3 * errorG;
            next[x][2] += 3 * errorB;
            next[x + 1][1] = errorG;
            next[x + 1][2] = errorB;
            next[x + 1][0] = errorR;
        }

        indices += width;
        pixels += width;
    }

    uint32_t count = width * height;
    uint16_t* pixel = reinterpret_cast<uint16_t*>(out);
    const uint8_t* alpha = indices;
    uint32_t shift = 0;

    switch (this->m_header.m_alphaSize) {
        case ALPHA_1: {
            for (; count; count--) {
                *pixel |= static_cast<uint16_t>((*alpha & (1 << shift)) << (15 - shift));
                shift++;
                pixel++;

                if (shift >= 8) {
                    shift = 0;
                    alpha++;
                }
            }

            break;
        }

        case ALPHA_4: {
            for (; count; count--) {
                if (shift) {
                    shift = 0;
                    *pixel |= (*alpha & 0x80) << 8;
                    alpha++;
                } else {
                    *pixel |= static_cast<uint16_t>((*alpha & 0xFFF8) << 12);
                    shift = this->m_header.m_alphaSize;
                }

                pixel++;
            }

            break;
        }

        case ALPHA_8: {
            for (; count; count--) {
                *pixel |= (*alpha & 0x80) << 8;
                pixel++;
                alpha++;
            }

            break;
        }
    }
}

// VERIFY: 87e7bd46 manual
// OFFSET: 0x006AF140
void CBLPFile::DecompPalARGB565DitherFloydSteinberg(uint8_t* out, const void* in, uint32_t width, uint32_t height) {
    // STATIC: 0x00C6D5B0
    static int32_t s_errors[2][1026][3];

    uint16_t* pixels = reinterpret_cast<uint16_t*>(out);
    const uint8_t* indices = static_cast<const uint8_t*>(in);

    memset(s_errors, 0, (width + 2) * sizeof(s_errors[0][0]));

    for (uint32_t y = 0; y < height; y++) {
        int32_t (*next)[3] = &s_errors[(y - 1) & 1][1];
        int32_t (*current)[3] = &s_errors[y & 1][1];

        next[-1][1] = 0;
        next[-1][2] = 0;
        next[-1][0] = 0;
        next[0][2] = 0;
        next[0][1] = 0;
        next[0][0] = 0;

        for (uint32_t x = 0; x < width; x++) {
            const BlpPalPixel& color = this->m_header.m_extended.m_palette[indices[x]];

            int32_t r = (current[x][0] >> 4) + (color.m_r << 16);
            int32_t g = (current[x][1] >> 4) + (color.m_g << 16);
            int32_t b = (current[x][2] >> 4) + (color.m_b << 16);

            int32_t quantB = (b + 0x40000) & 0xFFF80000;
            int32_t quantG = (g + 0x20000) & 0xFFFC0000;
            int32_t quantR = (r + 0x40000) & 0xFFF80000;

            int32_t outR = quantR >> 19;
            int32_t outG = quantG >> 18;
            int32_t outB = quantB >> 19;

            outR = outR < 0 ? 0 : (outR > 31 ? 31 : outR);
            outG = outG < 0 ? 0 : (outG > 63 ? 63 : outG);
            outB = outB < 0 ? 0 : (outB > 31 ? 31 : outB);

            pixels[x] = static_cast<uint16_t>((((outR << 6) | outG) << 5) | outB);

            int32_t errorR = r - quantR;
            int32_t errorG = g - quantG;
            int32_t errorB = b - quantB;

            current[x + 1][0] += 7 * errorR;
            current[x + 1][1] += 7 * errorG;
            current[x + 1][2] += 7 * errorB;
            (next + x)[-1][0] += 5 * errorR;
            (next + x)[-1][1] += 5 * errorG;
            (next + x)[-1][2] += 5 * errorB;
            next[x][0] += 3 * errorR;
            next[x][1] += 3 * errorG;
            next[x][2] += 3 * errorB;
            next[x + 1][1] = errorG;
            next[x + 1][0] = errorR;
            next[x + 1][2] = errorB;
        }

        indices += width;
        pixels += width;
    }
}

// VERIFY: 696ab781 manual
// OFFSET: 0x006AF340
void CBLPFile::DecompPalARGB2565DitherFloydSteinberg(uint8_t* out, const void* in, uint32_t width, uint32_t height) {
    // STATIC: 0x00C735E0
    static int32_t s_errors[2][1026][3];

    uint16_t* pixels = reinterpret_cast<uint16_t*>(out);
    const uint8_t* indices = static_cast<const uint8_t*>(in);

    memset(s_errors, 0, (width + 2) * sizeof(s_errors[0][0]));

    for (uint32_t y = 0; y < height; y++) {
        int32_t (*next)[3] = &s_errors[(y - 1) & 1][1];
        int32_t (*current)[3] = &s_errors[y & 1][1];

        next[-1][1] = 0;
        next[-1][2] = 0;
        next[-1][0] = 0;
        next[0][2] = 0;
        next[0][1] = 0;
        next[0][0] = 0;

        for (uint32_t x = 0; x < width; x++) {
            const BlpPalPixel& color = this->m_header.m_extended.m_palette[indices[x]];

            int32_t r = (current[x][0] >> 4) + (color.m_r << 16);
            int32_t g = (current[x][1] >> 4) + (color.m_g << 16);
            int32_t b = (current[x][2] >> 4) + (color.m_b << 16);

            int32_t quantB = (b + 0x40000) & 0xFFF80000;
            int32_t quantG = (g + 0x20000) & 0xFFFC0000;
            int32_t quantR = (r + 0x40000) & 0xFFF80000;

            int32_t outR = quantR >> 19;
            int32_t outG = quantG >> 18;
            int32_t outB = quantB >> 19;

            outR = outR < 0 ? 0 : (outR > 31 ? 31 : outR);
            outG = outG < 0 ? 0 : (outG > 63 ? 63 : outG);
            outB = outB < 0 ? 0 : (outB > 31 ? 31 : outB);

            pixels[x] = static_cast<uint16_t>((((outR << 6) | outG) << 5) | outB);

            int32_t errorR = r - quantR;
            int32_t errorG = g - quantG;
            int32_t errorB = b - quantB;

            current[x + 1][0] += 7 * errorR;
            current[x + 1][1] += 7 * errorG;
            current[x + 1][2] += 7 * errorB;
            (next + x)[-1][0] += 5 * errorR;
            (next + x)[-1][1] += 5 * errorG;
            (next + x)[-1][2] += 5 * errorB;
            next[x][0] += 3 * errorR;
            next[x][1] += 3 * errorG;
            next[x][2] += 3 * errorB;
            next[x + 1][1] = errorG;
            next[x + 1][2] = errorB;
            next[x + 1][0] = errorR;
        }

        indices += width;
        pixels += width;
    }

    uint32_t count = width * height;
    uint8_t* alphaOut = reinterpret_cast<uint8_t*>(pixels);
    const uint8_t* alpha = indices;

    switch (this->m_header.m_alphaSize) {
        case ALPHA_1: {
            for (uint32_t i = 0; i < count; i++) {
                uint8_t* dest = alphaOut + (i >> 2);
                uint8_t value = static_cast<uint8_t>(((alpha[i >> 3] >> (i & 7)) * 2) << s_twoBitAlphaShift[i & 3]);
                *dest = (*dest & ~s_twoBitAlphaMask[i & 3]) | value;
            }

            break;
        }

        case ALPHA_4: {
            for (uint32_t i = 0; i < count; i++) {
                uint8_t* dest = alphaOut + (i >> 2);
                uint8_t value = static_cast<uint8_t>((alpha[i >> 1] >> s_fourBitAlphaShift[i & 1]) << s_twoBitAlphaShift[i & 3]);
                *dest = (*dest & ~s_twoBitAlphaMask[i & 3]) | value;
            }

            break;
        }

        case ALPHA_8: {
            for (uint32_t i = 0; i < count; i++) {
                uint8_t* dest = alphaOut + (i >> 2);
                uint8_t value = static_cast<uint8_t>((alpha[i] >> 6) << s_twoBitAlphaShift[i & 3]);
                *dest = (*dest & ~s_twoBitAlphaMask[i & 3]) | value;
            }

            break;
        }
    }
}

// VERIFY: 083826a8 auto
// OFFSET: 0x006AF660
uint32_t CBLPFile::Width(uint32_t mipLevel) const {
    uint32_t width = this->m_header.m_width >> mipLevel;
    return width > 1 ? width : 1;
}

// VERIFY: 2983056f auto
// OFFSET: 0x006AF680
uint32_t CBLPFile::Height(uint32_t mipLevel) const {
    uint32_t height = this->m_header.m_height >> mipLevel;
    return height > 1 ? height : 1;
}

// VERIFY: 6efde90e auto
// OFFSET: 0x006AF6A0
uint32_t CBLPFile::Pixels(uint32_t mipLevel) const {
    return this->Height(mipLevel) * this->Width(mipLevel);
}

// VERIFY: c1020237 manual
// OFFSET: 0x006AF6E0
int32_t CBLPFile::Unlock(uint32_t mipLevel) {
    if (this->m_lockDecompMem) {
        SMemFree(this->m_lockDecompMem, __FILE__, __LINE__, 0);
    }

    return this->IsValidMip(mipLevel);
}

// VERIFY: ebad915e auto
// OFFSET: 0x006AF730
int32_t CBLPFile::GetFormatSize(PIXEL_FORMAT format, uint32_t mipLevel, uint32_t* size, uint32_t* stride) const {
    uint32_t width = this->Width(mipLevel);
    uint32_t height = this->Height(mipLevel);
    uint32_t pixels = width * height;
    uint32_t alphaBytes = pixels >> 2 ? pixels >> 2 : 1;

    switch (format) {
        case PIXEL_ARGB8888:
            *size = 4 * pixels;
            *stride = 4 * width;
            return 1;

        case PIXEL_ARGB1555:
        case PIXEL_ARGB4444:
        case PIXEL_RGB565:
            *size = 2 * pixels;
            *stride = 2 * width;
            return 1;

        case PIXEL_ARGB2565:
            *size = alphaBytes + 2 * pixels;
            *stride = 2 * width;
            return 1;

        default:
            *size = 0;
            *stride = 0;
            return 0;
    }
}

// VERIFY: a7900f2e manual
// OFFSET: 0x006AF810
int32_t CBLPFile::DecompPalDither(PIXEL_FORMAT format, uint32_t mipLevel, uint8_t* out, const void* in) {
    switch (format) {
        case PIXEL_ARGB8888: {
            uint32_t pixels = this->Pixels(mipLevel);

            if (this->m_header.m_alphaSize == ALPHA_8) {
                this->DecompPalFastPath(out, in, pixels);
            } else {
                this->DecompPalARGB8888(out, in, pixels);
            }

            return 1;
        }

        case PIXEL_ARGB1555:
            this->DecompPalARGB1555DitherFloydSteinberg(out, in, this->Width(mipLevel), this->Height(mipLevel));
            return 1;

        case PIXEL_ARGB4444:
            this->DecompPalARGB4444DitherFloydSteinberg(out, in, this->Width(mipLevel), this->Height(mipLevel));
            return 1;

        case PIXEL_RGB565:
            this->DecompPalARGB565DitherFloydSteinberg(out, in, this->Width(mipLevel), this->Height(mipLevel));
            return 1;

        case PIXEL_ARGB2565:
            this->DecompPalARGB2565DitherFloydSteinberg(out, in, this->Width(mipLevel), this->Height(mipLevel));
            return 1;

        default:
            return 0;
    }
}

// VERIFY: 53c2d8d4 manual
// OFFSET: 0x006AF990
int32_t CBLPFile::Lock(PIXEL_FORMAT format, uint32_t mipLevel, uint8_t*& data, uint32_t& stride) {
    if (!this->m_inMemoryImage) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    this->m_lockDecompMem = nullptr;

    if (!this->IsValidMip(mipLevel)) {
        return 0;
    }

    uint8_t* image = static_cast<uint8_t*>(this->m_inMemoryImage) + this->m_header.m_mipOffsets[mipLevel];

    if (!this->m_header.m_mipSizes[mipLevel]) {
        return 0;
    }

    switch (this->m_header.m_colorEncoding) {
        case COLOR_PAL: {
            uint32_t size;
            if (!this->GetFormatSize(format, mipLevel, &size, &stride)) {
                return 0;
            }

            data = static_cast<uint8_t*>(SMemAlloc(size, __FILE__, __LINE__, 0));
            int32_t result = this->DecompPalDither(format, mipLevel, data, image);
            this->m_lockDecompMem = data;

            return result;
        }

        case COLOR_DXT: {
            switch (format) {
                case PIXEL_DXT1:
                case PIXEL_DXT3:
                case PIXEL_DXT5:
                    data = image;
                    return 1;

                case PIXEL_ARGB8888:
                case PIXEL_ARGB1555:
                case PIXEL_ARGB4444:
                case PIXEL_RGB565: {
                    uint32_t size;
                    if (!this->GetFormatSize(format, mipLevel, &size, &stride)) {
                        return 0;
                    }

                    BlitFormat inFormat = GetBlitFormat(static_cast<PIXEL_FORMAT>(this->m_header.m_preferredFormat));
                    BlitFormat outFormat = GetBlitFormat(format);

                    data = static_cast<uint8_t*>(SMemAlloc(size, __FILE__, __LINE__, 0));
                    this->m_lockDecompMem = data;

                    uint32_t width = this->m_header.m_width >> mipLevel;
                    uint32_t height = this->m_header.m_height >> mipLevel;
                    C2iVector blitSize = { static_cast<int32_t>(width), static_cast<int32_t>(height) };

                    if (Blit(blitSize, BlitAlpha_0, image, CalcRowStride(inFormat, width, height), inFormat, data, stride, outFormat)) {
                        return 1;
                    }

                    SMemFree(data, __FILE__, __LINE__, 0);
                    return 0;
                }

                default:
                    return 1;
            }
        }

        case COLOR_RAW:
            data = image;
            return 1;

        default:
            return 0;
    }
}

// VERIFY: 8029449e manual
// OFFSET: 0x006AFB70
int32_t CBLPFile::LockChain(PIXEL_FORMAT format, MipBits*& images, uint32_t mipLevel) {
    if (!this->IsValidMip(mipLevel)) {
        return 0;
    }

    MipBits* allocated = nullptr;

    if (images) {
        MippedImgSet(format, this->Width(mipLevel), this->Height(mipLevel), images);
    } else {
        allocated = MippedImgAllocA(format, this->Width(mipLevel), this->Height(mipLevel), __FILE__, __LINE__);
        images = allocated;

        if (!allocated) {
            return 0;
        }
    }

    for (uint32_t i = 0; mipLevel < this->m_numLevels; i++, mipLevel++) {
        uint8_t* data;
        uint32_t stride;

        if (!this->Lock(format, mipLevel, data, stride)) {
            if (allocated) {
                SMemFree(allocated, __FILE__, __LINE__, 0);
                images = nullptr;
            }

            return 0;
        }

        memcpy(images[i].m_mip[0], data, CalcLevelSize(mipLevel, this->m_header.m_width, this->m_header.m_height, format));
        this->Unlock(mipLevel);
    }

    return 1;
}

// VERIFY: 282eeba2 manual
// OFFSET: 0x006AFCE0
int32_t CBLPFile::Lock2(const char* fileName, PIXEL_FORMAT format, uint32_t mipLevel, uint8_t* data, uint32_t& stride) {
    if (!this->m_inMemoryImage) {
        DebugValidate("m_inMemoryImage");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!this->IsValidMip(mipLevel)) {
        return 0;
    }

    uint8_t* image = static_cast<uint8_t*>(this->m_inMemoryImage) + this->m_header.m_mipOffsets[mipLevel];
    uint32_t imageSize = this->m_header.m_mipSizes[mipLevel];

    if (!imageSize) {
        return 0;
    }

    switch (this->m_header.m_colorEncoding) {
        case COLOR_JPEG:
            DebugLog("%s: JPEG decompresion not enabled", fileName);
            return 0;

        case COLOR_PAL: {
            uint32_t size;
            int32_t result = this->GetFormatSize(format, mipLevel, &size, &stride);

            if (result) {
                result = this->DecompPalDither(format, mipLevel, data, image);
                this->m_lockDecompMem = data;
            }

            return result;
        }

        case COLOR_DXT: {
            switch (format) {
                case PIXEL_DXT1:
                case PIXEL_DXT3:
                case PIXEL_DXT5:
                    memcpy(data, image, imageSize);
                    return 1;

                case PIXEL_ARGB8888:
                case PIXEL_ARGB1555:
                case PIXEL_ARGB4444:
                case PIXEL_RGB565: {
                    uint32_t width = this->Width(mipLevel);
                    uint32_t height = this->Height(mipLevel);
                    BlitFormat inFormat = GetBlitFormat(static_cast<PIXEL_FORMAT>(this->m_header.m_preferredFormat));
                    BlitFormat outFormat = GetBlitFormat(format);
                    C2iVector blitSize = { static_cast<int32_t>(width), static_cast<int32_t>(this->Height(mipLevel)) };

                    return Blit(blitSize, BlitAlpha_0, image, CalcRowStride(inFormat, width, height), inFormat, data, CalcRowStride(outFormat, width, height), outFormat);
                }

                case PIXEL_ARGB2565:
                    DebugLog("%s: component texture saved as DXT compressed", fileName);
                    return 0;

                default:
                    DebugLog("%s: tried to decompress a DXT texture into pixel format %d", fileName, format);
                    return 0;
            }
        }

        case COLOR_RAW:
            memcpy(data, image, imageSize);
            return 1;

        default:
            DebugLog("%s: unsupported image format", fileName);
            return 0;
    }
}

// VERIFY: 8d2086fa manual
// OFFSET: 0x006AFF10
int32_t CBLPFile::Open(const char* fileName, int32_t flags) {
    if (!fileName) {
        DebugValidate("filename");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    this->Close();

    SFileImpl* file;
    if (!SFile::OpenEx(nullptr, fileName, flags != 0, &file)) {
        return 0;
    }

    s_blpFileLoadBuffer.SetCount(SFileGetFileSize(file, nullptr));

    uint32_t bytesRead;
    SFileReadFile(file, s_blpFileLoadBuffer.Ptr(), s_blpFileLoadBuffer.Count(), &bytesRead, nullptr, 0);
    SFileCloseFile(file);

    return this->Source(s_blpFileLoadBuffer.Ptr());
}

// VERIFY: 3d8e87b0 manual
// OFFSET: 0x006AFFD0
int32_t CBLPFile::LockChain2(const char* fileName, PIXEL_FORMAT format, MipBits*& images, uint32_t mipLevel, int32_t inPlace) {
    if (!this->IsValidMip(mipLevel)) {
        return 0;
    }

    if (!images) {
        images = MippedImgAllocA(format, this->Width(mipLevel), this->Height(mipLevel), __FILE__, __LINE__);

        if (!images) {
            return 0;
        }
    } else {
        bool setImages = true;

        if (this->m_header.m_colorEncoding == COLOR_DXT) {
            setImages = format == PIXEL_ARGB4444 || format == PIXEL_RGB565 || format == PIXEL_ARGB1555 || format == PIXEL_ARGB8888 || !inPlace;
        } else if (this->m_header.m_colorEncoding == COLOR_RAW) {
            setImages = !inPlace;
        }

        if (!setImages) {
            for (uint32_t i = 0; this->m_header.m_mipSizes[i]; i++) {
                images[i].m_mip[0] = reinterpret_cast<C4Pixel*>(static_cast<uint8_t*>(this->m_inMemoryImage) + this->m_header.m_mipOffsets[i]);
            }

            this->m_inMemoryImage = nullptr;
            return 1;
        }

        MippedImgSet(format, this->Width(mipLevel), this->Height(mipLevel), images);
    }

    for (uint32_t i = 0; mipLevel < this->m_numLevels; i++, mipLevel++) {
        uint32_t stride;

        if (!this->Lock2(fileName, format, mipLevel, reinterpret_cast<uint8_t*>(images[i].m_mip[0]), stride)) {
            return 0;
        }
    }

    this->m_inMemoryImage = nullptr;

    return 1;
}
