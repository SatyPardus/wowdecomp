#include <cmath>
#include <cctype>
#if !defined(_WIN32)
#include <strings.h>
#define _strnicmp strncasecmp
#endif
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "SOption.h"
#include "Engine/Source/Storm/H/storm.h"

#define mix(a, b, c) \
    { \
        a -= b; a -= c; a ^= (c >> 13); \
        b -= c; b -= a; b ^= (a << 8); \
        c -= a; c -= b; c ^= (b >> 13); \
        a -= b; a -= c; a ^= (c >> 12); \
        b -= c; b -= a; b ^= (a << 16); \
        c -= a; c -= b; c ^= (b >> 5); \
        a -= b; a -= c; a ^= (c >> 3); \
        b -= c; b -= a; b ^= (a << 10); \
        c -= a; c -= b; c ^= (b >> 15); \
    }

// STATIC: 0x00A39538
static const uint32_t s_hashtable[16] = { 0x486E26EE, 0xDCAA16B3, 0xE1918EEF, 0x202DAFDB, 0x341C7DC7, 0x1C365303, 0x40EF2D37, 0x65FD5E49, 0xD6057177, 0x904ECE93, 0x1C38024F, 0x98FD323B, 0xE3061AE7, 0xA39B0FA1, 0x9797F25F, 0xE4444563 };

// STATIC: 0x00A395F8
static const uint32_t offsetsFromUTF8[7] = { 0x00000000, 0x00000000, 0x00003080, 0x000E2080, 0x03C82080, 0xFA082080, 0x82082080 };

// STATIC: 0x00A39618
static const uint8_t bytesFromUTF8[256] = {
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6
};

// STATIC: 0x00CAC16C
static int32_t s_initialized;

// STATIC: 0x00CAC170
static double s_realDigit[20][10];

// VERIFY: 6c594f7b auto
// OFFSET: 0x0076E6E0
char* SStrChr(const char* string, char search) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    for (; *string; string++) {
        if (*string == search) {
            return const_cast<char*>(string);
        }
    }

    return nullptr;
}

// VERIFY: c2203f65 auto
// OFFSET: 0x0076E720
char* SStrChrR(const char* string, char search) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    const char* result = nullptr;

    for (; *string; string++) {
        if (*string == search) {
            result = string;
        }
    }

    return const_cast<char*>(result);
}

// VERIFY: 8e9003dc auto
// OFFSET: 0x0076E760
int32_t STORMAPI SStrCmp(const char* string1, const char* string2, size_t maxchars) {
    return strncmp(string1, string2, maxchars);
}

// VERIFY: 0278ea41 auto
// OFFSET: 0x0076E780
int32_t STORMAPI SStrCmpI(const char* string1, const char* string2, size_t maxchars) {
    return _strnicmp(string1, string2, maxchars);
}

// VERIFY: 806bfb4a manual
// OFFSET: 0x0076E7A0
static int32_t GetNextTextUpper(const char** string, uint32_t* code, uint32_t* upper) {
    int32_t bytes = bytesFromUTF8[static_cast<uint8_t>(**string)];
    *code = 0;
    *upper = 0;

    switch (bytes) {
    case 6:
        *code += static_cast<uint8_t>(*(*string)++);
        if (!**string) {
            return 0;
        }
        *code <<= 6;
    case 5:
        *code += static_cast<uint8_t>(*(*string)++);
        if (!**string) {
            return 0;
        }
        *code <<= 6;
    case 4:
        *code += static_cast<uint8_t>(*(*string)++);
        if (!**string) {
            return 0;
        }
        *code <<= 6;
    case 3:
        *code += static_cast<uint8_t>(*(*string)++);
        if (!**string) {
            return 0;
        }
        *code <<= 6;
    case 2:
        *code += static_cast<uint8_t>(*(*string)++);
        if (!**string) {
            return 0;
        }
        *code <<= 6;
    case 1:
        *code += static_cast<uint8_t>(*(*string)++);
    }

    *code -= offsetsFromUTF8[bytes];

    if (*code > 0xFFFF) {
        *code = 0xFFFD;
    }

    uint32_t ch = *code;

    if (bytes == 1) {
        if (ch >= 'a' && ch <= 'z') {
            ch -= 32;
        }
    } else if (bytes == 2) {
        if (ch >= 0xE0 && ch <= 0xFE) {
            ch -= 32;
        } else if (ch == 0x153) {
            *upper = 0x152;
            return bytes;
        } else if (ch == 0x451) {
            *upper = 0x401;
            return bytes;
        } else if (ch >= 0x430 && ch <= 0x44F) {
            ch -= 32;
        }
    }

    *upper = ch;
    return bytes;
}

// VERIFY: 6bf80111 manual
// OFFSET: 0x0076E8D0
static int32_t SStrCmpGetNextTextUpper(const char** string, uint32_t* code, uint32_t* upper) {
    int32_t bytes = bytesFromUTF8[static_cast<uint8_t>(**string)];
    *code = 0;
    *upper = 0;

    switch (bytes) {
    case 6:
        *code += static_cast<uint8_t>(*(*string)++);
        if (!**string) {
            return 0;
        }
        *code <<= 6;
    case 5:
        *code += static_cast<uint8_t>(*(*string)++);
        if (!**string) {
            return 0;
        }
        *code <<= 6;
    case 4:
        *code += static_cast<uint8_t>(*(*string)++);
        if (!**string) {
            return 0;
        }
        *code <<= 6;
    case 3:
        *code += static_cast<uint8_t>(*(*string)++);
        if (!**string) {
            return 0;
        }
        *code <<= 6;
    case 2:
        *code += static_cast<uint8_t>(*(*string)++);
        if (!**string) {
            return 0;
        }
        *code <<= 6;
    case 1:
        *code += static_cast<uint8_t>(*(*string)++);
    }

    *code -= offsetsFromUTF8[bytes];

    if (*code > 0xFFFF) {
        *code = 0xFFFD;
    }

    uint32_t ch = *code;

    if (bytes == 1) {
        if (ch >= 'a' && ch <= 'z') {
            ch -= 32;
        }
    } else if (bytes == 2) {
        if (ch >= 0xE0 && ch <= 0xFE) {
            ch -= 32;
        } else if (ch == 0x153) {
            *upper = 0x152;
            return bytes;
        } else if (ch == 0x401 || ch == 0x451) {
            *upper = 0x415;
            return bytes;
        } else if (ch >= 0x410 && ch <= 0x415) {
            *upper = ch - 1;
            return bytes;
        } else if (ch >= 0x430 && ch <= 0x44F) {
            *upper = ch - (ch <= 0x435) - 32;
            return bytes;
        }
    }

    *upper = ch;
    return bytes;
}

// VERIFY: 6124f42a manual
// OFFSET: 0x0076EA40
int32_t STORMAPI SStrCmpUTF8I(const char* string1, const char* string2, size_t maxchars) {
    while ((*string1 || *string2) && maxchars--) {
        uint32_t code1;
        uint32_t upper1;
        SStrCmpGetNextTextUpper(&string1, &code1, &upper1);

        uint32_t code2;
        uint32_t upper2;
        SStrCmpGetNextTextUpper(&string2, &code2, &upper2);

        if (upper1 != upper2) {
            return upper1 - upper2;
        }
    }

    return 0;
}

// VERIFY: 6b766a9b auto
// OFFSET: 0x0076EAB0
static uint32_t GetSortChar(uint32_t ch) {
    if (ch >= 0xC0 && ch <= 0xDF) {
        if (ch <= 0xC6) {
            return 'A';
        }
        if (ch == 0xC7) {
            return 'C';
        }
        if (ch >= 0xC8 && ch <= 0xCB) {
            return 'E';
        }
        if (ch >= 0xCC && ch <= 0xCF) {
            return 'I';
        }
        if (ch == 0xD1) {
            return 'N';
        }
        if (ch >= 0xD2 && ch <= 0xD6) {
            return 'O';
        }
        if (ch >= 0xD9 && ch <= 0xDC) {
            return 'U';
        }
        if (ch == 0xDF) {
            return 'S';
        }
        return ch;
    }

    if (ch == 0x152) {
        return 'O';
    }

    if (ch == 0x401) {
        return 0x415;
    }

    return ch;
}

// VERIFY: f7b4479a auto
// OFFSET: 0x0076EB50
static uint32_t GetNextSortChar(const char* string, int32_t* bytes) {
    uint32_t code;
    uint32_t upper;
    *bytes = GetNextTextUpper(&string, &code, &upper);
    return GetSortChar(upper);
}

// VERIFY: 09b7377b manual
// OFFSET: 0x0076EB80
static bool CompareSortChars(uint32_t upper1, uint32_t upper2, const char** string1, const char** string2) {
    if (GetSortChar(upper1) != GetSortChar(upper2)) {
        return false;
    }

    if (upper1 == upper2) {
        return true;
    }

    int32_t bytes;

    if (upper1 == 0xC6) {
        if (GetNextSortChar(*string2, &bytes) != 'E') {
            return false;
        }

        *string2 += bytes;
        return true;
    }

    if (upper2 == 0xC6) {
        if (GetNextSortChar(*string1, &bytes) != 'E') {
            return false;
        }

        *string1 += bytes;
        return true;
    }

    if (upper1 == 0xDF) {
        if (GetNextSortChar(*string2, &bytes) != 'S') {
            return false;
        }

        *string2 += bytes;
        return true;
    }

    if (upper2 == 0xDF) {
        if (GetNextSortChar(*string1, &bytes) != 'S') {
            return false;
        }

        *string1 += bytes;
        return true;
    }

    if (upper1 == 0x152) {
        if (GetNextSortChar(*string2, &bytes) != 'E') {
            return false;
        }

        *string2 += bytes;
        return true;
    }

    if (upper2 == 0x152) {
        if (GetNextSortChar(*string1, &bytes) != 'E') {
            return false;
        }

        *string1 += bytes;
        return true;
    }

    return true;
}

// VERIFY: bbed3513 manual
// OFFSET: 0x0076EC80
int32_t STORMAPI SStrSortUTF8I(const char* string1, const char* string2, size_t maxchars) {
    int32_t result = 0;

    while ((*string1 || *string2) && maxchars--) {
        uint32_t code1;
        uint32_t upper1;
        GetNextTextUpper(&string1, &code1, &upper1);

        uint32_t code2;
        uint32_t upper2;
        GetNextTextUpper(&string2, &code2, &upper2);

        if (!CompareSortChars(upper1, upper2, &string1, &string2)) {
            return GetSortChar(upper1) - GetSortChar(upper2);
        }

        if (!result) {
            result = code1 - code2;
        }
    }

    return result;
}

// VERIFY: b3c3de02 manual
// OFFSET: 0x0076ED20
size_t STORMAPI SStrCopy(char* dest, const char* source, size_t destsize) {
    if (!dest || !source) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    char* curr = dest;

    if (destsize == STORM_MAX_STR) {
        while (*source) {
            *curr++ = *source++;
        }
    } else {
        char* end = dest + destsize - 1;

        while (*source && curr < end) {
            *curr++ = *source++;
        }
    }

    *curr = 0;
    return curr - dest;
}

// VERIFY: 4cf8cd50 manual
// OFFSET: 0x0076EDA0
size_t STORMAPI SStrCopyUTF8(char* dest, const char* source, size_t destsize, size_t maxchars) {
    if (!dest || !source) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    char* end = dest + destsize - 1;
    char* curr = dest;
    char* last = dest;

    while (*source && curr < end && maxchars) {
        *curr = *source;

        if ((*source & 0xC0) != 0x80) {
            maxchars--;
            last = curr + 1;
        }

        curr++;
        source++;
    }

    if (curr < end) {
        *curr = 0;
    } else {
        *last = 0;
    }

    return curr - dest;
}

// VERIFY: c8b00c92 auto
// OFFSET: 0x0076EE30
size_t STORMAPI SStrLen(const char* string) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    const char* curr = string;

    while (*curr) {
        curr++;
    }

    return curr - string;
}

// VERIFY: 32365d2c auto
// OFFSET: 0x0076EE60
size_t STORMAPI SStrLen(const uint16_t* string) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    const uint16_t* curr = string;

    while (*curr) {
        curr++;
    }

    return curr - string;
}

// VERIFY: 2dc41369 auto
// OFFSET: 0x0076EEA0
size_t STORMAPI SStrLenUTF8(const char* string) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    size_t length = 0;

    for (; *string; string++) {
        if ((*string & 0xC0) != 0x80) {
            length++;
        }
    }

    return length;
}

// VERIFY: 4492ca41 manual
// OFFSET: 0x0076EEE0
size_t STORMAPI SStrLenUTF8(const char* string, size_t maxchars) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    size_t length = 0;

    for (; *string && maxchars--; string++) {
        if ((*string & 0xC0) != 0x80) {
            length++;
        }
    }

    return length;
}

// VERIFY: 4d394932 manual
// OFFSET: 0x0076EF30
const char* STORMAPI SStrAdvanceCharsUTF8(const char* string, size_t chars) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    for (; *string && chars; string++) {
        if ((*string & 0xC0) != 0x80) {
            chars--;
        }
    }

    return string;
}

// VERIFY: 2678bf77 manual
// OFFSET: 0x0076EF70
size_t STORMAPI SStrPack(char* dest, const char* source, size_t destsize) {
    if (!dest || !source) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!destsize) {
        return 0;
    }

    char* curr = dest;

    while (*curr) {
        curr++;
    }

    if (destsize == STORM_MAX_STR) {
        while (*source) {
            *curr++ = *source++;
        }
    } else {
        char* end = dest + destsize - 1;

        while (*source && curr < end) {
            *curr++ = *source++;
        }
    }

    *curr = 0;
    return curr - dest;
}

// VERIFY: 98987d37 manual
// OFFSET: 0x0076F010
static size_t ISStrVPrintf(char* dest, size_t maxchars, const char* format, va_list arglist) {
    if (!maxchars) {
        return 0;
    }

    if (maxchars == STORM_MAX_STR) {
        if (g_opt.orderedprintfenabled) {
            return vsoprintf(dest, format, arglist);
        }

        return vsprintf(dest, format, arglist);
    }

    size_t result;

    if (g_opt.orderedprintfenabled) {
        result = vsnoprintf(dest, maxchars, format, arglist);
    } else {
#if defined(_WIN32)
        result = _vsnprintf(dest, maxchars, format, arglist);
#else
        result = vsnprintf(dest, maxchars, format, arglist);
#endif
    }

    if (result >= maxchars) {
        result = maxchars - 1;
        dest[maxchars - 1] = 0;
    }

    return result;
}

// VERIFY: c3018cdb auto
// OFFSET: 0x0076F070
size_t SStrPrintf(char* dest, size_t maxchars, const char* format, ...) {
    if (!dest || !format) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    va_list arglist;
    va_start(arglist, format);
    return ISStrVPrintf(dest, maxchars, format, arglist);
}

// VERIFY: 190c39be manual
// OFFSET: 0x0076F0A0
size_t SStrVPrintf(char* dest, size_t maxchars, const char* format, va_list arglist) {
    if (!dest || !format) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    return ISStrVPrintf(dest, maxchars, format, arglist);
}

// VERIFY: 79f69ad9 manual
// OFFSET: 0x0076F0D0
int32_t STORMAPI SStrToInt(const char* string) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    const char* curr = string;

    if (*string == '-') {
        curr++;
    }

    uint32_t result = 0;
    uint32_t digit;

    while ((digit = *curr - '0') < 10) {
        result = 10 * result + digit;
        curr++;
    }

    if (*string == '-') {
        return -static_cast<int32_t>(result);
    }

    return result;
}

// VERIFY: c5c1fa9e auto
// OFFSET: 0x0076F140
uint32_t STORMAPI SStrToUnsigned(const char* string) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    uint32_t result = 0;
    uint32_t digit;

    while ((digit = *string - '0') < 10) {
        result = 10 * result + digit;
        string++;
    }

    return result;
}

// VERIFY: 0b70bbc1 manual
// OFFSET: 0x0076F190
uint32_t STORMAPI SStrToUnsigned(const char** string) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    uint32_t result = 0;

    while (**string >= '0' && **string <= '9') {
        result = 10 * result + **string - '0';
        (*string)++;
    }

    return result;
}

// VERIFY: cb6da48d manual
// OFFSET: 0x0076F1E0
void STORMAPI SStrTokenize(const char** string, char* buffer, size_t bufferchars, const char* whitespace, int32_t* quoted) {
    int32_t inquotes = 0;

    if (!string || !*string || (!buffer && bufferchars) || !whitespace) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    const char* curr = *string;
    int32_t checkquotes = SStrChr(whitespace, '"') != nullptr;
    int32_t usedquotes = 0;

    for (; *curr && SStrChr(whitespace, *curr); curr++) {
        if (checkquotes && *curr == '"') {
            usedquotes = 1;
            inquotes = 1;
            curr++;
            break;
        }
    }

    size_t destchars = 0;

    for (; *curr; curr++) {
        if (checkquotes && *curr == '"') {
            if (destchars && !inquotes) {
                break;
            }

            usedquotes = 1;
            inquotes = !inquotes;

            if (!inquotes) {
                curr++;
                break;
            }
        } else {
            if (!inquotes && SStrChr(whitespace, *curr)) {
                curr++;
                break;
            }

            if (destchars + 1 < bufferchars) {
                buffer[destchars++] = *curr;
            }
        }
    }

    if (destchars < bufferchars) {
        buffer[destchars] = 0;
    }

    *string = curr;

    if (quoted) {
        *quoted = usedquotes;
    }
}

// VERIFY: a6e9692a auto
// OFFSET: 0x0076F340
uint32_t STORMAPI SStrHash(const char* string, uint32_t flags, uint32_t seed) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    uint32_t result = seed ? seed : 0x7FED7FED;
    uint32_t adjust = 0xEEEEEEEE;

    if (flags & SSTR_HASH_CASESENSITIVE) {
        for (; *string; string++) {
            uint32_t ch = static_cast<uint8_t>(*string);
            result = (s_hashtable[ch >> 4] - s_hashtable[ch & 0xF]) ^ (adjust + result);
            adjust = ch + result + 33 * adjust + 3;
        }
    } else {
        for (; *string; string++) {
            uint32_t ch = static_cast<uint8_t>(*string);

            if (ch >= 'a' && ch <= 'z') {
                ch -= 32;
            }

            if (ch == '/') {
                ch = '\\';
            }

            result = (s_hashtable[ch >> 4] - s_hashtable[ch & 0xF]) ^ (adjust + result);
            adjust = ch + result + 33 * adjust + 3;
        }
    }

    return result ? result : 1;
}

// VERIFY: 9e769fd0 manual
// OFFSET: 0x0076F420
static uint32_t bjhash(const uint8_t* k, uint32_t length, uint32_t initval) {
    uint32_t a = 0x9E3779B9;
    uint32_t b = 0x9E3779B9;
    uint32_t c = initval;
    uint32_t len = length;

    while (len >= 12) {
        a += k[0] + (static_cast<uint32_t>(k[1]) << 8) + (static_cast<uint32_t>(k[2]) << 16) + (static_cast<uint32_t>(k[3]) << 24);
        b += k[4] + (static_cast<uint32_t>(k[5]) << 8) + (static_cast<uint32_t>(k[6]) << 16) + (static_cast<uint32_t>(k[7]) << 24);
        c += k[8] + (static_cast<uint32_t>(k[9]) << 8) + (static_cast<uint32_t>(k[10]) << 16) + (static_cast<uint32_t>(k[11]) << 24);
        mix(a, b, c);
        k += 12;
        len -= 12;
    }

    c += length;

    switch (len) {
    case 11:
        c += static_cast<uint32_t>(k[10]) << 24;
    case 10:
        c += static_cast<uint32_t>(k[9]) << 16;
    case 9:
        c += static_cast<uint32_t>(k[8]) << 8;
    case 8:
        b += static_cast<uint32_t>(k[7]) << 24;
    case 7:
        b += static_cast<uint32_t>(k[6]) << 16;
    case 6:
        b += static_cast<uint32_t>(k[5]) << 8;
    case 5:
        b += k[4];
    case 4:
        a += static_cast<uint32_t>(k[3]) << 24;
    case 3:
        a += static_cast<uint32_t>(k[2]) << 16;
    case 2:
        a += static_cast<uint32_t>(k[1]) << 8;
    case 1:
        a += k[0];
    }

    mix(a, b, c);
    return c;
}

// VERIFY: de28bb00 manual
// OFFSET: 0x0076F640
uint32_t STORMAPI SStrHashHT(const char* string) {
    uint8_t buffer[1024];
    uint8_t* dest = buffer;
    uint32_t length = 0;

    while (*string && length < 1020) {
        uint32_t code;
        uint32_t upper;
        GetNextTextUpper(&string, &code, &upper);

        if (upper == '/') {
            upper = '\\';
        }

        do {
            *dest++ = static_cast<uint8_t>(upper);
            length++;
            upper >>= 8;
        } while (upper);
    }

    *dest = 0;
    return bjhash(buffer, length, 0);
}

// VERIFY: 95c28b4a auto
// OFFSET: 0x0076F6C0
void STORMAPI SStrUpper(char* string) {
#if defined(_WIN32)
    _strupr(string);
#else
    for (char* p = string; *p; ++p) { *p = std::toupper(static_cast<unsigned char>(*p)); }
#endif
}

// VERIFY: 760ebea0 auto
// OFFSET: 0x0076F6E0
void STORMAPI SStrLower(char* string) {
#if defined(_WIN32)
    _strlwr(string);
#else
    for (char* p = string; *p; ++p) { *p = std::tolower(static_cast<unsigned char>(*p)); }
#endif
}

// VERIFY: d9e4e017 auto
// OFFSET: 0x0076F700
char* SStrStr(const char* string, const char* search) {
    if (!string || !search) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    size_t length = SStrLen(search);

    for (; *string; string++) {
        if (!SStrCmp(string, search, length)) {
            return const_cast<char*>(string);
        }
    }

    return nullptr;
}

// VERIFY: 648bbc09 auto
// OFFSET: 0x0076F770
char* SStrStrI(const char* string, const char* search) {
    if (!string || !search) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    size_t length = SStrLen(search);

    for (; *string; string++) {
        if (!SStrCmpI(string, search, length)) {
            return const_cast<char*>(string);
        }
    }

    return nullptr;
}

// VERIFY: 3228cf7b manual
// OFFSET: 0x0076F7E0
const char* SStrStrUTF8I(const char* string, const char* search) {
    if (!string || !search) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    size_t length = SStrLenUTF8(search);

    while (*string) {
        if (!SStrCmpUTF8I(string, search, length)) {
            return string;
        }

        int32_t bytes = bytesFromUTF8[static_cast<uint8_t>(*string)];

        while (bytes-- && *string) {
            string++;
        }
    }

    return nullptr;
}

// VERIFY: 01bc1256 manual
// OFFSET: 0x0076F860
char* Int64ToString(int64_t num, char* buf, size_t destsize) {
    char nbuf[32];

    if (destsize > sizeof(nbuf)) {
        destsize = sizeof(nbuf);
    }

    char* curr = nbuf;
    int32_t thou = 0;

    if (!num) {
        *curr++ = '0';
    } else {
        while (num) {
            if (curr >= &nbuf[destsize - 1]) {
                memset(buf, '*', destsize - 1);
                buf[destsize - 1] = 0;
                return buf;
            }

            *curr++ = static_cast<char>(num % 10 + '0');
            num /= 10;

            if (++thou == 3) {
                if (!num) {
                    break;
                }

                *curr++ = ',';
                thou = 0;
            }
        }
    }

    char* dest = buf;

    while (curr >= nbuf) {
        *dest++ = *--curr;
    }

    *dest = 0;
    return buf;
}

// VERIFY: dc3f250d manual
// OFFSET: 0x0076F970
static void InitializeFloatDigits() {
    for (int32_t exponent = -1; exponent > -21; exponent--) {
        for (int32_t digit = 0; digit < 10; digit++) {
            s_realDigit[-1 - exponent][digit] = pow(10.0, exponent) * digit;
        }
    }
}

// VERIFY: 6dbcfaba auto
// OFFSET: 0x0076F9E0
char* STORMAPI SStrDupA(const char* string, const char* filename, int32_t linenumber) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    size_t length = SStrLen(string) + 1;
    char* result = static_cast<char*>(SMemAlloc(length, filename, linenumber, 0));
    memcpy(result, string, length);
    return result;
}

// VERIFY: f5475b96 auto
// OFFSET: 0x0076FA40
void STORMAPI SStrInitialize() {
    InitializeFloatDigits();
    s_initialized = 1;
}

void STORMAPI SStrDestroy() {
}

// VERIFY: 0bb00853 manual
// OFFSET: 0x0076FA50
int64_t STORMAPI SStrToInt64(const char* string) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    int32_t negative = *string == '-';

    if (negative) {
        string++;
    }

    int64_t result = 0;
    uint32_t value = 0;
    const char* start = string;
    uint32_t digit;

    while ((digit = *string - '0') < 10) {
        value = 10 * value + digit;
        string++;

        if (value >= 0x19999999) {
            result = value + static_cast<int64_t>(pow(10.0, string - start) + 0.5) * result;
            value = 0;
            start = string;
        }
    }

    if (result) {
        result = value + static_cast<int64_t>(pow(10.0, string - start) + 0.5) * result;
    } else {
        result = value;
    }

    return negative ? -result : result;
}

// INLINED: [0x0076FB9E->0x0076FBB6]
static void CheckInitialized() {
    if (!s_initialized) {
        SStrInitialize();
    }
}

// VERIFY: 5ad58df0 manual
// OFFSET: 0x0076FB80
float STORMAPI SStrToFloat(const char* string) {
    if (!string) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0.0f;
    }

    CheckInitialized();

    int32_t negative = *string == '-';

    if (negative) {
        string++;
    }

    double result = 0.0;
    uint32_t value = 0;
    const char* start = string;
    uint32_t digit;

    while ((digit = *string - '0') < 10) {
        value = 10 * value + digit;
        string++;

        if (value >= 0x19999999) {
            result = pow(10.0, string - start) * result + value;
            value = 0;
            start = string;
        }
    }

    if (result == 0.0) {
        result = value;
    } else {
        result = pow(10.0, string - start) * result + value;
    }

    if (*string == '.') {
        string++;

        for (int32_t i = 0; (digit = *string - '0') < 10; i++) {
            string++;

            if (i < 20) {
                result += s_realDigit[i][digit];
            } else {
                result += pow(10.0, -1 - i) * digit;
            }
        }
    }

    if (*string == 'e' || *string == 'E') {
        string++;

        if (*string == '+') {
            string++;
        }

        result = pow(10.0, SStrToInt(string)) * result;
    }

    return static_cast<float>(negative ? -result : result);
}
