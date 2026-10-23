#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mbstring.h>
#include <new>
#include "Engine/Source/Services/Profile.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Storm/H/storm.h"

enum PROFILE_PARSE_STATE : int32_t {
    STATE_NEWLINE = 0,
    STATE_COMMENT = 1,
    STATE_SECTION = 2,
    STATE_STRIP_TRAILING = 3,
    STATE_KEY = 4,
    STATE_VALUE = 5,
};

// VERIFY: c051543c auto
// OFFSET: 0x004BBCB0
static void StrTrimTrailingSpace(char* str) {
    char* ptr = str + SStrLen(str) - 1;

    if (ptr >= str) {
        while (_ismbcspace(*ptr)) {
            ptr--;
        }

        ptr[1] = '\0';
    }
}

// VERIFY: 74491526 manual
// OFFSET: 0x004BBD00
int32_t ProfileInternal::PrfStrToInt(const char* str) {
    int32_t result = 0;

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
    PROFILE_PARSE_STATE state = STATE_NEWLINE;

    for (char* ptr = text; *ptr; ptr++) {
        switch (state) {
            case STATE_NEWLINE:
                if (SStrChr("\r\n", *ptr)) {
                    break;
                }

                if (!SStrCmp(ptr, "//", 2)) {
                    state = STATE_COMMENT;
                } else if (*ptr == '[') {
                    section = ptr + 1;
                    state = STATE_SECTION;
                } else {
                    key = ptr;
                    state = STATE_KEY;
                }

                break;

            case STATE_COMMENT:
            case STATE_STRIP_TRAILING:
                if (SStrChr("\r\n", *ptr)) {
                    state = STATE_NEWLINE;
                }

                break;

            case STATE_SECTION:
                if (SStrChr("\r\n", *ptr)) {
                    section = lastSection;
                    state = STATE_NEWLINE;
                } else if (*ptr == ']') {
                    *ptr = '\0';
                    lastSection = section;
                    state = STATE_STRIP_TRAILING;
                }

                break;

            case STATE_KEY:
                if (SStrChr("\r\n", *ptr)) {
                    state = STATE_NEWLINE;
                    key = nullptr;
                } else if (*ptr == '=') {
                    *ptr = '\0';
                    value = ptr + 1;
                    state = STATE_VALUE;
                }

                break;

            case STATE_VALUE:
                if (SStrChr("\r\n", *ptr)) {
                    *ptr = '\0';
                    TokenizeStringValues(profile, section, key, value);
                    state = STATE_NEWLINE;
                }

                break;
        }
    }

    if (state == STATE_VALUE) {
        TokenizeStringValues(profile, section, key, value);
    }

    return 1;
}

// VERIFY: 1b2c8e3e auto
// OFFSET: 0x004BD7C0
int32_t ProfileInternal::IReadFile(PROFILE* profile, const char* path) {
    char name[260];
    SStrCopy(name, path, sizeof(name));
    StrTrimTrailingSpace(name);

    uint32_t bytes = 0;
    void* buffer = nullptr;

    if (!SFile::LoadFile(name, &buffer, &bytes, 1, nullptr)) {
        return 0;
    }

    int32_t result = IReadBuffer(profile, buffer, bytes);
    SFile::Unload(buffer);

    return result;
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
