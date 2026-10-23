#ifndef WOW_SOURCE_GAME_TYPES_H
#define WOW_SOURCE_GAME_TYPES_H

#include <cstddef>
#include <cstdint>
#include <windows.h>

enum CHARSET : int32_t {
    CHARSET_UNKNOWN = -1,
    CHARSET_LATIN1 = 0,
    CHARSET_ENGLISH = 1,
    CHARSET_RUSSIAN = 2,
    CHARSET_KOREAN = 3,
    CHARSET_CHINESE = 4,
    NUM_CHARSETS = 5,
};

enum VALIDATE_NAME_RESULT : int32_t {
    NAME_SUCCESS = 0,
    NAME_FAILURE = 1,
    NAME_NO_NAME = 2,
    NAME_TOO_SHORT = 3,
    NAME_TOO_LONG = 4,
    NAME_INVALID_CHARACTER = 5,
    NAME_MIXED_LANGUAGES = 6,
    NAME_PROFANE = 7,
    NAME_RESERVED = 8,
    NAME_INVALID_APOSTROPHE = 9,
    NAME_MULTIPLE_APOSTROPHES = 10,
    NAME_THREE_CONSECUTIVE = 11,
    NAME_INVALID_SPACE = 12,
    NAME_CONSECUTIVE_SPACES = 13,
    NAME_RUSSIAN_CONSECUTIVE_SILENT_CHARACTERS = 14,
    NAME_RUSSIAN_SILENT_CHARACTER_AT_BEGINNING_OR_END = 15,
    NAME_DECLENSION_DOESNT_MATCH_BASE_NAME = 16,
    NUM_NAME_RESULTS = 17,

    // Unused 0.5.3-only names/values; do not use these as 3.3.5 result codes.
    // NAME_STARTS_WITH_GRAVE = 3,
    // NAME_TWO_GRAVES = 4,
};

struct LoadedWardenModule {
    /* 0000 */ void* m_imageBase;
    /* 0004 */ uint32_t m_imageSize;
    /* 0008 */ uint32_t m_dllsLoaded;
};

#endif
