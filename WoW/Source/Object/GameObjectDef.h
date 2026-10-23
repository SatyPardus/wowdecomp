#ifndef WOW_SOURCE_OBJECT_GAMEOBJECTDEF_H
#define WOW_SOURCE_OBJECT_GAMEOBJECTDEF_H

#include <cstddef>
#include <cstdint>

enum : int32_t {
    TYPE_DOOR = 0,
    TYPE_BUTTON = 1,
    TYPE_QUESTGIVER = 2,
    TYPE_CHEST = 3,
    TYPE_BINDER = 4,
    TYPE_GENERIC = 5,
    TYPE_TRAP = 6,
    TYPE_CHAIR = 7,
    TYPE_SPELL_FOCUS = 8,
    TYPE_TEXT = 9,
    TYPE_GOOBER = 10,
    TYPE_TRANSPORT = 11,
    TYPE_AREADAMAGE = 12,
    TYPE_CAMERA = 13,
    TYPE_MAP_OBJECT = 14,
    TYPE_MO_TRANSPORT = 15,
    TYPE_DUEL_ARBITER = 16,
    TYPE_FISHINGNODE = 17,
    TYPE_RITUAL = 18,
    TYPE_MAILBOX = 19,
    // Slot 20 is marked DONOTUSE in the native type table.
    TYPE_GUARDPOST = 21, // Metadata kind; no client constructor case.
    TYPE_SPELLCASTER = 22,
    TYPE_MEETINGSTONE = 23,
    TYPE_FLAGSTAND = 24,
    TYPE_FISHINGHOLE = 25,
    TYPE_FLAGDROP = 26,
    TYPE_MINIGAME = 27,
    // Slot 28 is marked DONOTUSE in the native type table.
    TYPE_CAPTUREPOINT = 29,
    TYPE_AURAGENERATOR = 30,
    TYPE_DUNGEON_DIFFICULTY = 31,
    TYPE_BARBER_CHAIR = 32,
    TYPE_DESTRUCTIBLE_BUILDING = 33,
    TYPE_GUILD_BANK = 34,
    TYPE_TRAPDOOR = 35,
    NUM_GAMEOBJECT_TYPE = 36,
};

class CGameObjectDef {
    public:
    struct TypeInfo {
        /* 0000 */ const char* m_name;
        /* 0004 */ int32_t m_numProps;
        /* 0008 */ const int32_t* m_props;
        /* 000C */ const void* m_propInfo;
    };

    static const TypeInfo s_typeInfo[NUM_GAMEOBJECT_TYPE];

    static int32_t GetPropNum(int32_t typeId, int32_t prop);
};

#endif
