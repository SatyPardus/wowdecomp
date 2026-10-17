#if defined(__APPLE__)

#include <cstdint>
#include <cstring>
#include "Engine/Source/BlizzardCore/Source/System/Unicode/Unicode.h"

// macOS file system and C library APIs take UTF-8, so the "native" encoding is the UTF-8 input itself
// (Windows converts to the ANSI code page here).
Blizzard::StringConversion_CodepageToUTF8::ToNative::ToNative(const char* str) : Blizzard::Memory::TempBuffer<char, 300>((str ? static_cast<uint32_t>(strlen(str)) : 0) + 1) {
    if (str) {
        memcpy(this->m_data, str, this->m_count);
    } else {
        this->m_data[0] = '\0';
    }
}

#endif
