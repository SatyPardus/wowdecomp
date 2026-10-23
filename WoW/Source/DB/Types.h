#ifndef WOW_SOURCE_DB_TYPES_H
#define WOW_SOURCE_DB_TYPES_H

#include <cstdint>

// Indices into the 3.3.5 installed/DB locale table.
enum DB_LOCALE_INDEX : int32_t {
    DB_LOCALE_deDE = 0,
    DB_LOCALE_enGB = 1,
    DB_LOCALE_enUS = 2,
    DB_LOCALE_esES = 3,
    DB_LOCALE_frFR = 4,
    DB_LOCALE_koKR = 5,
    DB_LOCALE_zhCN = 6,
    DB_LOCALE_zhTW = 7,
    DB_LOCALE_enCN = 8,
    DB_LOCALE_enTW = 9,
    DB_LOCALE_esMX = 10,
    DB_LOCALE_ruRU = 11,
    NUM_DB_LOCALES = 12,

    // Unused 0.5.3 WOW_LOCALE names/values; this DB ordering differs.
    // LOCALE_en_US = 0,
    // LOCALE_ko_KR = 1,
    // LOCALE_ja_JP = 2,
    // LOCALE_zh_CN = 3,
    // LOCALE_zh_TW = 4,
    // LOCALE_es_ES = 5,
    // LOCALE_fr_FR = 6,
    // LOCALE_de_DE = 7,
    // NUM_LOCALES = 8,
    // DEFAULT_LANGUAGE = 0,
};

#endif
