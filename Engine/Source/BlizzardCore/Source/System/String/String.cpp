#include <cstdarg>
#include <cstdint>
#include <cstring>
#if !defined(_WIN32)
#include <strings.h>
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#endif
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: 7b459c65 manual
// OFFSET: 0x00428670
uint32_t Blizzard::String::Copy(char* dst, const char* src, uint32_t maxchars) {
    if (!maxchars || !dst) {
        return 0;
    }

    if (!src) {
        *dst = '\0';
        return 0;
    }

    char* end = dst + maxchars - 1;
    char* p = dst;

    while (p < end) {
        char c = *src++;

        if (!c) {
            break;
        }

        *p++ = c;
    }

    *p = '\0';

    return static_cast<uint32_t>(p - dst);
}

// VERIFY: a0f7b087 manual
// OFFSET: 0x004286C0
uint32_t Blizzard::String::Append(char* dst, const char* src, uint32_t maxchars) {
    if (!maxchars || !dst) {
        return 0;
    }

    char* end = dst + maxchars - 1;
    char* p = dst;

    if (p <= end) {
        while (*p) {
            if (++p > end) {
                return static_cast<uint32_t>(p - dst);
            }
        }

        if (p <= end && src) {
            while (p < end) {
                char c = *src++;

                if (!c) {
                    break;
                }

                *p++ = c;
            }

            *p = '\0';
        }
    }

    return static_cast<uint32_t>(p - dst);
}

// VERIFY: 109c466e auto
// OFFSET: 0x00428830
const char* Blizzard::String::FindFirstOf(const char* str, const char* chars, int32_t maxchars) {
    if (!str || !chars || !*chars || maxchars < 1 || !*str) {
        return nullptr;
    }

    const char* end = str + maxchars;

    for (const char* p = str; p < end && *p; p++) {
        for (const char* c = chars; *c; c++) {
            if (*p == *c) {
                return p;
            }
        }
    }

    return nullptr;
}

// VERIFY: a95efa1a auto
// OFFSET: 0x004288A0
int32_t Blizzard::String::VFormat(char* dst, uint32_t maxchars, const char* format, va_list args) {
    if (!format) {
        if (maxchars) {
            *dst = '\0';
        }

        return 0;
    }

    int32_t result = vsnoprintf(dst, maxchars, format, args);
    dst[maxchars - 1] = '\0';

    return result;
}

// VERIFY: 4ac59ac3 auto
// OFFSET: 0x004288E0
void Blizzard::String::MemCopy(void* dst, const void* src, int32_t bytes) {
    if (dst >= static_cast<const uint8_t*>(src) + bytes || static_cast<uint8_t*>(dst) + bytes <= src) {
        memcpy(dst, src, bytes);
    } else {
        memmove(dst, src, bytes);
    }
}

// VERIFY: 4a8b72b2 auto
// OFFSET: 0x00428920
void Blizzard::String::MemFill(void* dst, uint32_t bytes, uint8_t value) {
    memset(dst, value, bytes);
}

// VERIFY: d6ad756e manual
// OFFSET: 0x0042AA50
const char* System_String::Find(const char* str, const char* substr, uint32_t maxchars) {
    uint32_t sublen = substr ? static_cast<uint32_t>(strlen(substr)) : 0;
    uint32_t len = str ? static_cast<uint32_t>(strlen(str)) : 0;
    len = len < maxchars ? len : maxchars;
    const char* end = str + len - sublen + 1;
    const char* p = str;

    if (p >= end) {
        return nullptr;
    }

    while (!p || !substr || strncmp(p, substr, sublen)) {
        if (++p == end) {
            return nullptr;
        }
    }

    return p;
}

// VERIFY: 265edc75 manual
// OFFSET: 0x0042AAE0
const char* System_String::FindReverse(const char* str, char c, uint32_t maxchars) {
    const char* last = nullptr;

    for (const char* p = str; p != str + maxchars && *p; p++) {
        if (c == *p) {
            last = p;
        }
    }

    return last;
}

// VERIFY: 75e07458 manual
// OFFSET: 0x0042AB10
const char* System_String::FindI(const char* str, const char* substr, uint32_t maxchars) {
    uint32_t sublen = substr ? static_cast<uint32_t>(strlen(substr)) : 0;
    uint32_t len = str ? static_cast<uint32_t>(strlen(str)) : 0;
    len = len < maxchars ? len : maxchars;
    const char* end = str + len - sublen + 1;
    const char* p = str;

    if (p >= end) {
        return nullptr;
    }

    while (!p || !substr || _strnicmp(p, substr, sublen)) {
        if (++p == end) {
            return nullptr;
        }
    }

    return p;
}

// VERIFY: 2c8889ef auto
// OFFSET: 0x0042E530
char* System_String::Duplicate(const char* str, const char* fileName, int32_t lineNumber) {
    if (!str) {
        return nullptr;
    }

    uint32_t bytes = static_cast<uint32_t>(strlen(str)) + 1;
    char* dst = static_cast<char*>(SMemAlloc(bytes, fileName, lineNumber, 0));
    Blizzard::String::MemCopy(dst, str, bytes);

    return dst;
}

// VERIFY: 7764fd67 manual
// OFFSET: 0x0042E5A0
const char* Blizzard::String::Find(const char* str, const char* substr, uint32_t maxchars) {
    if (str && substr) {
        return System_String::Find(str, substr, maxchars);
    }

    return nullptr;
}

// VERIFY: bec97fe1 manual
// OFFSET: 0x0042E5D0
const char* Blizzard::String::FindReverse(const char* str, char c, uint32_t maxchars) {
    if (str && c) {
        return System_String::FindReverse(str, c, maxchars);
    }

    return nullptr;
}

// VERIFY: 8e199746 manual
// OFFSET: 0x0042E600
const char* Blizzard::String::FindI(const char* str, const char* substr, uint32_t maxchars) {
    if (str && substr) {
        return System_String::FindI(str, substr, maxchars);
    }

    return nullptr;
}

// VERIFY: 01512b35 auto
// OFFSET: 0x0042E670
int32_t Blizzard::String::Format(char* dst, uint32_t maxchars, const char* format, ...) {
    if (!dst || !maxchars) {
        return 0;
    }

    if (!format) {
        *dst = '\0';
        return 0;
    }

    va_list args;
    va_start(args, format);
    int32_t result = VFormat(dst, maxchars, format, args);
    va_end(args);

    return result;
}

// VERIFY: 86316efd manual
// OFFSET: 0x00433570
char* Blizzard::String::Duplicate(const char* str) {
    return System_String::Duplicate(str, __FILE__, __LINE__);
}

// VERIFY: 3f0dab9f manual
// OFFSET: 0x00428730
bool Blizzard::String::Equal(const char* str1, const char* str2) {
    return str1 && str2 && !strcmp(str1, str2);
}

// VERIFY: a6e9afd5 manual
// OFFSET: 0x00428780
bool Blizzard::String::EqualI(const char* str1, const char* str2) {
    return str1 && str2 && !_stricmp(str1, str2);
}

// VERIFY: ea6a1355 auto
// OFFSET: 0x004287B0
bool Blizzard::String::EqualI(const char* str1, const char* str2, uint32_t count) {
    return str1 && str2 && !_strnicmp(str1, str2, count);
}

// VERIFY: 87af17bb manual
// OFFSET: 0x004287E0
uint32_t Blizzard::String::Length(const char* str) {
    if (str) {
        return static_cast<uint32_t>(strlen(str));
    }

    return 0;
}
