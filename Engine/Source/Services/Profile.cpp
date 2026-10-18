#include <cstddef>
#include <cstdint>
#include <cstring>
#if defined(__APPLE__)
#include <cerrno>
#include <vector>
#else
#include <mbstring.h>
#endif
#include <new>
#include "Engine/Source/Services/Profile.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: c051543c auto
// OFFSET: 0x004BBCB0
static void StrTrimTrailingSpace(char* str) {
#if defined(__APPLE__)
    // Native paths are UTF-8. Trim the original ASCII whitespace without
    // interpreting continuation bytes as a Windows multibyte code page.
    size_t length = SStrLen(str);
    while (length && std::strchr(" \t\r\n\v\f", str[length - 1])) --length;
    str[length] = '\0';
#else
    char* ptr = str + SStrLen(str) - 1;

    if (ptr >= str) {
        while (_ismbcspace(*ptr)) {
            ptr--;
        }

        ptr[1] = '\0';
    }
#endif
}

// VERIFY: 74491526 manual
// OFFSET: 0x004BBD00
int32_t ProfileInternal::PrfStrToInt(const char* str) {
#if defined(__APPLE__)
    uint32_t result = 0;
#else
    int32_t result = 0;
#endif

    if (*str != '\'') {
        return SStrToInt(str);
    }

    const char* ptr = str + 1;

    for (int32_t i = 4; i; i--) {
        char c = *ptr;

        if (!c || c == '\'') {
            break;
        }

        result = c | (result << 8);
        ptr++;
    }

    return result;
}

// VERIFY: 7f0bd168 auto
// OFFSET: 0x004BBD40
void ProfileClose(void* profile) {
    if (profile) {
        delete static_cast<ProfileInternal::PROFILE*>(profile);
    }
}

// INLINED
char* ProfileInternal::STRINGBLOCK::Data() {
    return reinterpret_cast<char*>(this + 1);
}

// VERIFY: d2c1e4f1 manual
// OFFSET: 0x004BBEA0
ProfileInternal::STRINGBLOCK* ProfileInternal::STRINGBLOCK::AllocBlock(uint32_t size) {
    uint32_t bytes = size <= 4 ? 4 : size;
    STRINGBLOCK* block = new (SMemAlloc(bytes + sizeof(STRINGBLOCK), __FILE__, __LINE__, 0)) STRINGBLOCK;
    block->m_size = size;
    block->m_refs = 0;
    block->m_used = 0;

    return block;
}

// VERIFY: 34843167 manual
// OFFSET: 0x004BC1C0
char* ProfileInternal::STRINGBLOCK::AllocString(TSList<STRINGBLOCK, TSGetLink<STRINGBLOCK> >& list, const char* str, int32_t reuse) {
    if (!str) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    if (reuse) {
        for (STRINGBLOCK* block = list.Head(); block; block = list.Next(block)) {
            if (str >= block->Data() && str < block->Data() + block->m_size) {
                block->m_refs++;
                return const_cast<char*>(str);
            }
        }
    }

    uint32_t length = SStrLen(str) + 1;
    STRINGBLOCK* block = list.Head();

    if (!block || length > block->m_size - block->m_used) {
        block = AllocBlock(length <= 0x1000 ? 0x1000 : length);
        list.LinkToHead(block);
    }

    char* dest = block->Data() + block->m_used;
    memcpy(dest, str, length);
    block->m_used += length;
    block->m_refs++;

    return dest;
}

// VERIFY: e84487f4 manual
// OFFSET: 0x004BC290
void ProfileInternal::STRINGBLOCK::FreeString(TSList<STRINGBLOCK, TSGetLink<STRINGBLOCK> >& list, char* str) {
    if (!str) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    STRINGBLOCK* found = nullptr;

    for (STRINGBLOCK* block = list.Head(); block; block = list.Next(block)) {
        if (str >= block->Data() && str < block->Data() + block->m_size) {
            found = block;
            break;
        }
    }

    if (found->m_refs-- == 1) {
        found->m_used = 0;
        list.LinkToHead(found);
    }
}

// VERIFY: 2d27ed1d manual
// OFFSET: [0x004BCF60, 0x004BCFE0]
ProfileInternal::PROFILE::~PROFILE() {
    this->m_sections.Clear();
    this->m_strings.Clear();
}

// VERIFY: e55d3f97 manual
// OFFSET: 0x004BD150
void* ProfileCreate() {
    return new (SMemAlloc(sizeof(ProfileInternal::PROFILE), __FILE__, __LINE__, 0)) ProfileInternal::PROFILE;
}

// VERIFY: 4d0d24db manual
// OFFSET: 0x004BD320
int32_t ProfileGetValue(void* profile, const char* section, const char* key, int32_t* value, uint32_t index) {
#if defined(__APPLE__)
    if (!profile) { SErrSetLastError(STORM_ERROR_INVALID_PARAMETER); return 0; }
#endif
    if (!section) {
        DebugValidate("section");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!key) {
        DebugValidate("key");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!value) {
        DebugValidate("value");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    *value = 0;

    ProfileInternal::SECTION* sect = static_cast<ProfileInternal::PROFILE*>(profile)->m_sections.Ptr(section);

    if (!sect) {
        return 0;
    }

    ProfileInternal::KEYVALUE* keyValue = sect->m_keys.Ptr(key);

    if (!keyValue || index >= keyValue->m_values.Count()) {
        return 0;
    }

    char* str = keyValue->m_values[index];

    if (!str) {
        return 0;
    }

    *value = ProfileInternal::PrfStrToInt(str);

    return 1;
}

// VERIFY: 477a2fb3 manual
// OFFSET: 0x004BD3B0
int32_t ProfileGetValue(void* profile, const char* section, const char* key, char* buffer, uint32_t bufferSize, uint32_t index) {
#if defined(__APPLE__)
    if (!profile || !bufferSize) { SErrSetLastError(STORM_ERROR_INVALID_PARAMETER); return 0; }
#endif
    if (!section) {
        DebugValidate("section");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!key) {
        DebugValidate("key");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!buffer) {
        DebugValidate("value");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    *buffer = '\0';

    ProfileInternal::SECTION* sect = static_cast<ProfileInternal::PROFILE*>(profile)->m_sections.Ptr(section);

    if (!sect) {
        return 0;
    }

    ProfileInternal::KEYVALUE* keyValue = sect->m_keys.Ptr(key);

    if (!keyValue || index >= keyValue->m_values.Count()) {
        return 0;
    }

    char* str = keyValue->m_values[index];

    if (!str) {
        return 0;
    }

    SStrCopy(buffer, str, bufferSize);

    return 1;
}

// VERIFY: 9c09dbf0 manual
// OFFSET: 0x004BD440
char* ProfileInternal::AddValue(PROFILE* profile, const char* section, const char* key, const char* value, int32_t replace, int32_t reuse) {
    SECTION* sect = profile->m_sections.Ptr(section);

    if (!sect) {
        char* name = STRINGBLOCK::AllocString(profile->m_strings, section, reuse);
        sect = profile->m_sections.New(SStrHashHT(name), name, 0, 0);
    }

    KEYVALUE* keyValue = sect->m_keys.Ptr(key);

    if (!keyValue) {
        char* name = STRINGBLOCK::AllocString(profile->m_strings, key, reuse);
        keyValue = sect->m_keys.New(SStrHashHT(name), name, 0, 0);
    }

    if (replace) {
        for (uint32_t i = keyValue->m_values.Count(); i; i--) {
            STRINGBLOCK::FreeString(profile->m_strings, keyValue->m_values[i - 1]);
        }

        keyValue->m_values.Clear();
    }

    char** slot = keyValue->m_values.New();
    *slot = STRINGBLOCK::AllocString(profile->m_strings, value, reuse);

    return *slot;
}

// VERIFY: 6aeb4c03 manual
// OFFSET: 0x004BD580
void ProfileInternal::TokenizeStringValues(PROFILE* profile, const char* section, const char* key, char* values) {
    if (!section || !key || !values || !*values) {
        return;
    }

    char* ptr = values;

    do {
        int32_t quoted = 0;

        if (*ptr == '"') {
            quoted = 1;
            ptr++;
        }

        char* start = ptr;

        if (*ptr) {
            while (true) {
                if (quoted) {
                    if (*ptr == '"') {
                        *ptr++ = '\0';

                        if (*ptr == ',') {
                            ptr++;
                        }

                        break;
                    }
                } else if (*ptr == ',') {
                    *ptr++ = '\0';
                    break;
                }

                if (!*++ptr) {
                    break;
                }
            }
        }

        AddValue(profile, section, key, start, 0, 1);
    } while (*ptr);
}

// VERIFY: f8f127cf manual
// OFFSET: 0x004BD600
int32_t ProfileInternal::IReadBuffer(PROFILE* profile, const void* data, uint32_t size) {
#if defined(__APPLE__)
    if (!profile || (!data && size) || size > 4 * 1024 * 1024) {
        errno = size > 4 * 1024 * 1024 ? EFBIG : EINVAL;
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER); return 0;
    }
#endif
    STRINGBLOCK* block = STRINGBLOCK::AllocBlock(size + 1);
    char* text = block->Data();
    memcpy(text, data, size);
    text[size] = '\0';
    block->m_used = block->m_size;
    profile->m_strings.LinkToTail(block);

    const char* section = nullptr;
    const char* lastSection = nullptr;
    char* key = nullptr;
    char* value = nullptr;
    int32_t state = 0;

    for (char* ptr = text; *ptr; ptr++) {
        switch (state) {
            case 0:
                if (SStrChr("\r\n", *ptr)) {
                    break;
                }

                if (!SStrCmp(ptr, "//", 2)) {
                    state = 1;
                } else if (*ptr == '[') {
                    section = ptr + 1;
                    state = 2;
                } else {
                    key = ptr;
                    state = 4;
                }

                break;

            case 1:
            case 3:
                if (SStrChr("\r\n", *ptr)) {
                    state = 0;
                }

                break;

            case 2:
                if (SStrChr("\r\n", *ptr)) {
                    section = lastSection;
                    state = 0;
                } else if (*ptr == ']') {
                    *ptr = '\0';
                    lastSection = section;
                    state = 3;
                }

                break;

            case 4:
                if (SStrChr("\r\n", *ptr)) {
                    state = 0;
                    key = nullptr;
                } else if (*ptr == '=') {
                    *ptr = '\0';
                    value = ptr + 1;
                    state = 5;
                }

                break;

            case 5:
                if (SStrChr("\r\n", *ptr)) {
                    *ptr = '\0';
                    TokenizeStringValues(profile, section, key, value);
                    state = 0;
                }

                break;
        }
    }

    if (state == 5) {
        TokenizeStringValues(profile, section, key, value);
    }

    return 1;
}

// VERIFY: 1b2c8e3e auto
// OFFSET: 0x004BD7C0
int32_t ProfileInternal::IReadFile(PROFILE* profile, const char* path) {
#if defined(__APPLE__)
    if (!profile || !path || SStrLen(path) >= 260) { errno = EINVAL; return 0; }
#endif
    char name[260];
    SStrCopy(name, path, sizeof(name));
    StrTrimTrailingSpace(name);

#if defined(__APPLE__)
    if (!*name) { errno = EINVAL; return 0; }
    SFileImpl* file = nullptr;
    if (!SFile::OpenEx(nullptr, name, 0, &file)) return 0;
    struct Close { SFileImpl* file; ~Close() { SFileCloseFile(file); } } close{file};
    uint32_t high = 0;
    const uint32_t bytes = SFileGetFileSize(file, &high);
    // Bound allocation before reading; preserve SFile's archive/loose override
    // order and parse the original bytes using the shared profile parser.
    if (high || bytes > 4 * 1024 * 1024) { errno = EFBIG; return 0; }
    std::vector<char> buffer(size_t(bytes) + 1);
    uint32_t read = 0;
    if (bytes && (!SFileReadFile(file, buffer.data(), bytes, &read, nullptr, 0) || read != bytes)) return 0;
    return IReadBuffer(profile, buffer.data(), bytes);
#else
    uint32_t bytes = 0;
    void* buffer = nullptr;

    if (!SFile::LoadFile(name, &buffer, &bytes, 1, nullptr)) {
        return 0;
    }

    int32_t result = IReadBuffer(profile, buffer, bytes);
    SFile::Unload(buffer);

    return result;
#endif
}

// VERIFY: 23301000 auto
// OFFSET: 0x004BD840
int32_t ProfileReadFile(void* profile, const char* path) {
    if (!path) {
        DebugValidate("path");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    return ProfileInternal::IReadFile(static_cast<ProfileInternal::PROFILE*>(profile), path);
}
