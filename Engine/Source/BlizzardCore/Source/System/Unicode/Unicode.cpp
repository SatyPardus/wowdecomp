#include <cstdint>
#include <cstring>
#include "Engine/Source/BlizzardCore/Source/System/Unicode/Unicode.h"

// STATIC: 0x009E5360
const uint32_t System_Unicode::s_offsetsFromUTF8[7] = { 0x00000000, 0x00000000, 0x00003080, 0x000E2080, 0x03C82080, 0xFA082080, 0x82082080 };

// STATIC: 0x009E537C
const uint32_t System_Unicode::s_firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

// STATIC: 0x009E5398
const uint8_t System_Unicode::s_bytesFromUTF8[256] = {
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6
};

// VERIFY: 588faabf manual
// OFFSET: 0x00428940
int32_t Blizzard::Unicode::ConvertUTF16to8(char* dst, uint32_t dstmaxchars, const uint16_t* src, int32_t srcmaxchars, uint32_t* dstchars, uint32_t* srcchars) {
    char* dstStart = dst;
    const uint16_t* srcStart = src;
    const uint16_t* srcEnd = srcmaxchars >= 0 ? src + srcmaxchars : reinterpret_cast<const uint16_t*>(-1);
    char* dstEnd = dst + dstmaxchars;
    int32_t result;

    for (;;) {
        if (src >= srcEnd) {
            result = -1;
            break;
        }

        uint32_t ch = *src;
        uint32_t srcUnits = 1;

        if (ch - 0xD800 <= 0x3FF) {
            if (src + 1 >= srcEnd) {
                result = -1;
                break;
            }

            uint32_t ch2 = src[1];

            if (ch2 - 0xDC00 <= 0x3FF) {
                ch = ((ch - 0xD800) << 10) + (ch2 - 0xDC00) + 0x10000;
                srcUnits = 2;
            }
        }

        uint32_t bytesToWrite;

        if (ch < 0x80) {
            bytesToWrite = 1;

            if (!ch) {
                if (dst < dstEnd) {
                    *dst++ = 0;
                    result = 0;
                } else {
                    result = bytesToWrite;
                }

                break;
            }
        } else if (ch < 0x800) {
            bytesToWrite = 2;
        } else if (ch < 0x10000) {
            bytesToWrite = 3;
        } else if (ch < 0x200000) {
            bytesToWrite = 4;
        } else if (ch < 0x4000000) {
            bytesToWrite = 5;
        } else if (ch <= 0x7FFFFFFF) {
            bytesToWrite = 6;
        } else {
            bytesToWrite = 2;
            ch = 0xFFFD;
        }

        if (dst + bytesToWrite > dstEnd) {
            result = bytesToWrite;
            break;
        }

        dst += bytesToWrite;

        switch (bytesToWrite) {
            case 6:
                *--dst = static_cast<char>((ch & 0x3F) | 0x80);
                ch >>= 6;
            case 5:
                *--dst = static_cast<char>((ch & 0x3F) | 0x80);
                ch >>= 6;
            case 4:
                *--dst = static_cast<char>((ch & 0x3F) | 0x80);
                ch >>= 6;
            case 3:
                *--dst = static_cast<char>((ch & 0x3F) | 0x80);
                ch >>= 6;
            case 2:
                *--dst = static_cast<char>((ch & 0x3F) | 0x80);
                ch >>= 6;
            case 1:
                *--dst = static_cast<char>(ch | System_Unicode::s_firstByteMark[bytesToWrite]);
        }

        dst += bytesToWrite;
        src += srcUnits;
    }

    if (srcchars) {
        *srcchars = static_cast<uint32_t>(src - srcStart);
    }

    if (dstchars) {
        *dstchars = static_cast<uint32_t>(dst - dstStart);
    }

    return result;
}

// VERIFY: e28f3041 manual
// OFFSET: 0x00428B00
int32_t Blizzard::Unicode::ConvertUTF8to16(uint16_t* dst, uint32_t dstmaxchars, const char* src, int32_t srcmaxchars, uint32_t* dstchars, uint32_t* srcchars) {
    const uint8_t* source = reinterpret_cast<const uint8_t*>(src);
    uint16_t* dstStart = dst;
    const uint8_t* srcEnd = srcmaxchars >= 0 ? source + srcmaxchars : reinterpret_cast<const uint8_t*>(-1);
    uint16_t* dstEnd = dst + dstmaxchars;
    int32_t result = 0;

    if (source >= srcEnd) {
        result = -1;
    } else {
        for (;;) {
            uint32_t bytes = System_Unicode::s_bytesFromUTF8[*source];

            if (source + bytes > srcEnd) {
                result = -static_cast<int32_t>(bytes);
                break;
            }

            uint32_t read = 0;
            uint32_t ch = 0;

            switch (bytes) {
                case 6:
                    ch += source[read++];
                    ch <<= 6;
                case 5:
                    ch += source[read++];
                    ch <<= 6;
                case 4:
                    ch += source[read++];
                    ch <<= 6;
                case 3:
                    ch += source[read++];
                    ch <<= 6;
                case 2:
                    ch += source[read++];
                    ch <<= 6;
                case 1:
                    ch += source[read++];
                    ch -= System_Unicode::s_offsetsFromUTF8[bytes];
            }

            if (dst >= dstEnd) {
                result = 1;
                break;
            }

            if (ch > 0xFFFF) {
                if (ch > 0x10FFFF) {
                    *dst++ = 0xFFFD;
                } else {
                    if (dst + 1 >= dstEnd) {
                        result = 1;
                        break;
                    }

                    ch -= 0x10000;
                    *dst++ = static_cast<uint16_t>((ch >> 10) + 0xD800);
                    *dst++ = static_cast<uint16_t>((ch & 0x3FF) + 0xDC00);
                }
            } else {
                *dst++ = static_cast<uint16_t>(ch);

                if (!ch) {
                    break;
                }
            }

            source += read;

            if (source >= srcEnd) {
                result = -1;
                break;
            }
        }
    }

    if (srcchars) {
        *srcchars = static_cast<uint32_t>(source - reinterpret_cast<const uint8_t*>(src));
    }

    if (dstchars) {
        *dstchars = static_cast<uint32_t>(dst - dstStart);
    }

    return result;
}
