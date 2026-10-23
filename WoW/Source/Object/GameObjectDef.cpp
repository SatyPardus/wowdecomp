#include "GameObjectDef.h"

// STATIC: 0x00ADAD30
static const int32_t s_props0[8] = { 1, 4, 3, 57, 89, 90, 117, 85 };
// STATIC: 0x00ADAD70
static const int32_t s_props1[10] = { 1, 4, 3, 30, 57, 48, 89, 90, 55, 85 };
// STATIC: 0x00ADADC0
static const int32_t s_props2[11] = { 4, 5, 17, 38, 22, 57, 89, 55, 91, 48, 85 };
// STATIC: 0x00ADAE18
static const int32_t s_props3[18] = { 4, 6, 7, 8, 25, 26, 13, 30, 20, 31, 55, 56, 62, 66, 89, 92, 19, 85 };
// STATIC: 0x00ADB06C
static const int32_t s_props5[7] = { 19, 18, 37, 48, 63, 20, 85 };
// STATIC: 0x00ADAEE0
static const int32_t s_props6[16] = { 4, 31, 24, 10, 9, 23, 3, 32, 37, 65, 48, 58, 89, 90, 123, 85 };
// STATIC: 0x00ADAEA8
static const int32_t s_props7[5] = { 11, 12, 94, 13, 85 };
// STATIC: 0x00ADAFA0
static const int32_t s_props8[9] = { 14, 24, 30, 37, 20, 48, 19, 63, 85 };
// STATIC: 0x00ADAFE8
static const int32_t s_props9[5] = { 15, 16, 17, 91, 85 };
// STATIC: 0x00ADB010
static const int32_t s_props10[23] = { 4, 20, 21, 3, 22, 8, 23, 15, 16, 17, 10, 57, 30, 48, 89, 90, 55, 91, 19, 38, 127, 63, 85 };
// STATIC: 0x00ADB170
static const int32_t s_props11[6] = { 93, 1, 3, 125, 126, 87 };
// STATIC: 0x00ADAF60
static const int32_t s_props12[8] = { 4, 24, 27, 28, 29, 3, 89, 90 };
// STATIC: 0x00ADB100
static const int32_t s_props13[5] = { 4, 33, 21, 89, 85 };
// STATIC: 0x00ADB128
static const int32_t s_props15[9] = { 35, 36, 43, 44, 45, 99, 87, 67, 128 };
// STATIC: 0x00ADB1A0
static const int32_t s_props18[9] = { 34, 10, 39, 40, 49, 50, 54, 46, 85 };
// STATIC: 0x00ADB1E8
static const int32_t s_props19[1] = { 85 };
// STATIC: 0x00ADB1F0
static const int32_t s_props21[2] = { 42, 9 };
// STATIC: 0x00ADB204
static const int32_t s_props22[6] = { 10, 9, 47, 91, 48, 85 };
// STATIC: 0x00ADB234
static const int32_t s_props23[3] = { 51, 52, 53 };
// STATIC: 0x00ADB24C
static const int32_t s_props24[9] = { 4, 59, 24, 60, 61, 57, 89, 55, 85 };
// STATIC: 0x00ADB2BC
static const int32_t s_props25[5] = { 24, 6, 25, 26, 4 };
// STATIC: 0x00ADB294
static const int32_t s_props26[5] = { 4, 21, 59, 57, 89 };
// STATIC: 0x00ADB2E4
static const int32_t s_props27[1] = { 64 };
// STATIC: 0x00ADB308
static const int32_t s_props29[22] = { 24, 10, 67, 68, 70, 71, 72, 73, 74, 75, 76, 77, 78, 69, 79, 80, 81, 82, 48, 18, 118, 119 };
// STATIC: 0x00ADB2EC
static const int32_t s_props30[7] = { 1, 24, 83, 85, 84, 86, 37 };
// STATIC: 0x00ADB3D4
static const int32_t s_props31[2] = { 87, 88 };
// STATIC: 0x00ADAED0
static const int32_t s_props32[2] = { 12, 121 };
// STATIC: 0x00ADB3E8
static const int32_t s_props33[24] = { 95, 97, 103, 109, 103, 96, 103, 103, 103, 110, 103, 103, 103, 103, 111, 103, 112, 103, 124, 114, 103, 103, 120, 103 };
// STATIC: 0x00ADB3E4
static const int32_t s_props34[1] = { 85 };
// STATIC: 0x00ADB4AC
static const int32_t s_props35[3] = { 93, 1, 3 };

// STATIC: 0x00A36F30
const CGameObjectDef::TypeInfo CGameObjectDef::s_typeInfo[NUM_GAMEOBJECT_TYPE] = {
    { "door", 8, s_props0, nullptr },
    { "button", 10, s_props1, nullptr },
    { "questgiver", 11, s_props2, nullptr },
    { "chest", 18, s_props3, nullptr },
    { "binder", 0, nullptr, nullptr },
    { "generic", 7, s_props5, nullptr },
    { "trap", 16, s_props6, nullptr },
    { "chair", 5, s_props7, nullptr },
    { "spellFocus", 9, s_props8, nullptr },
    { "text", 5, s_props9, nullptr },
    { "goober", 23, s_props10, nullptr },
    { "transport", 6, s_props11, nullptr },
    { "areaDamage", 8, s_props12, nullptr },
    { "camera", 5, s_props13, nullptr },
    { "mapobject", 0, nullptr, nullptr },
    { "moTransport", 9, s_props15, nullptr },
    { "duelFlag", 0, nullptr, nullptr },
    { "fishingNode", 0, nullptr, nullptr },
    { "ritual", 9, s_props18, nullptr },
    { "mailbox", 1, s_props19, nullptr },
    { "DONOTUSE", 0, nullptr, nullptr },
    { "guardPost", 2, s_props21, nullptr },
    { "spellCaster", 6, s_props22, nullptr },
    { "meetingStone", 3, s_props23, nullptr },
    { "flagStand", 9, s_props24, nullptr },
    { "fishingHole", 5, s_props25, nullptr },
    { "flagDrop", 5, s_props26, nullptr },
    { "minigame", 1, s_props27, nullptr },
    { "DONOTUSE", 0, nullptr, nullptr },
    { "capturePoint", 22, s_props29, nullptr },
    { "auraGenerator", 7, s_props30, nullptr },
    { "dungeonDifficulty", 2, s_props31, nullptr },
    { "barberChair", 2, s_props32, nullptr },
    { "destructibleBuilding", 24, s_props33, nullptr },
    { "guild bank", 1, s_props34, nullptr },
    { "trapdoor", 3, s_props35, nullptr },
};

// VERIFY: 65bb1e58 auto
// OFFSET: 0x00746190
int32_t CGameObjectDef::GetPropNum(int32_t typeId, int32_t prop) {
    if (typeId >= NUM_GAMEOBJECT_TYPE || prop >= 129) {
        return -1;
    }

    int32_t count = CGameObjectDef::s_typeInfo[typeId].m_numProps;

    for (int32_t i = 0; i < count; i++) {
        if (CGameObjectDef::s_typeInfo[typeId].m_props[i] == prop) {
            return i;
        }
    }

    return -1;
}
