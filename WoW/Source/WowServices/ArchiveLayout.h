#ifndef WOW_SOURCE_WOWSERVICES_ARCHIVELAYOUT_H
#define WOW_SOURCE_WOWSERVICES_ARCHIVELAYOUT_H

#include <array>
#include <cstdint>

// Shared 3.3.5a archive layout. Array positions are also used by Client.cpp
// for expansion detection and interface archive enumeration.
struct ARCHIVEINFO {
    /* 0000 */ const char* m_name;
    /* 0004 */ int32_t m_unk04;
    /* 0008 */ int32_t m_unk08;
    /* 000C */ bool m_opened;
};

inline constexpr std::array<ARCHIVEINFO, 28> ClientArchiveLayout = {{
    { "alternate.MPQ", 3, 3, false },
    { "interface.MPQ", 1, 1, false },
    { "misc.MPQ", 1, 1, false },
    { "model.MPQ", 1, 1, false },
    { "texture.MPQ", 1, 1, false },
    { "terrain.MPQ", 1, 1, false },
    { "wmo.MPQ", 1, 1, false },
    { "sound.MPQ", 1, 1, false },
    { "fonts.MPQ", 1, 1, false },
    { "dbc.MPQ", 1, 1, false },
    { "speech.MPQ", 1, 1, false },
    { "expansionloc.MPQ", 2, 1, false },
    { "lichkingloc.MPQ", 2, 1, false },
    { "expansionspeech.MPQ", 2, 1, false },
    { "lichkingspeech.MPQ", 2, 1, false },
    { "expansion.MPQ", 2, 3, false },
    { "lichking.MPQ", 2, 3, false },
    { "common.MPQ", 1, 2, false },
    { "common-2.MPQ", 2, 2, false },
    { "****\\locale-****.MPQ", 1, 2, false },
    { "****\\speech-****.MPQ", 1, 2, false },
    { "****\\expansion-locale-****.MPQ", 2, 2, false },
    { "****\\lichking-locale-****.MPQ", 2, 2, false },
    { "****\\expansion-speech-****.MPQ", 2, 2, false },
    { "****\\lichking-speech-****.MPQ", 2, 2, false },
    { "development.MPQ", 2, 1, false },
    { "streaming.MPQ", 4, 1, false },
    { "streamingloc.MPQ", 4, 1, false },
}};

struct PATCHARCHIVESPEC {
    /* 0000 */ int32_t m_pass;
    /* 0004 */ int32_t m_flag;
    /* 0008 */ bool m_useInstallPath;
    /* 000C */ const char* m_directory;
    /* 0010 */ const char* m_pattern;
};

inline constexpr std::array<PATCHARCHIVESPEC, 12> ClientPatchArchiveSpecs = {{
    { 0, 0, false, "Data\\", "patch-?.MPQ" },
    { 0, 0, false, "Data\\%s\\", "patch-%s-?.MPQ" },
    { 1, 0, false, "Data\\", "patch.MPQ" },
    { 1, 0, false, "Data\\%s\\", "patch-%s.MPQ" },
    { 1, 1, false, "Data\\", "patch-4.MPQ" },
    { 1, 1, false, "Data\\%s\\", "patch-%s-4.MPQ" },
    { 1, 1, false, "Data\\", "patch-3.MPQ" },
    { 1, 1, false, "Data\\%s\\", "patch-%s-3.MPQ" },
    { 1, 1, false, "..\\Data\\", "patch-2.MPQ" },
    { 1, 1, false, "..\\Data\\%s\\", "patch-%s-2.MPQ" },
    { 1, 1, false, "..\\Data\\", "patch.MPQ" },
    { 1, 1, false, "..\\Data\\%s\\", "patch-%s.MPQ" },
}};

inline constexpr std::array<const char*, 12> ClientLocaleNames = {
    "deDE", "enGB", "enUS", "esES", "frFR", "koKR", "zhCN", "zhTW", "enCN", "enTW", "esMX", "ruRU"
};

#endif
