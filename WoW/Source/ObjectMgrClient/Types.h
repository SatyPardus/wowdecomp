#ifndef WOW_SOURCE_OBJECTMGRCLIENT_TYPES_H
#define WOW_SOURCE_OBJECTMGRCLIENT_TYPES_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"

class CGGameObject_C;
class CGObject_C;
class CGPlayer_C;
class CGUnit_C;
class ObjectMgr;

union CGObjectUnion {
    /* 0000 */ CGObject_C* m_obj;
    /* 0000 */ CGUnit_C* m_unit;
    /* 0000 */ CGPlayer_C* m_player;
    /* 0000 */ CGGameObject_C* m_gameObject;
};

enum TypeID {
    TYPEID_OBJECT = 0,
    TYPEID_ITEM = 1,
    TYPEID_CONTAINER = 2,
    TYPEID_UNIT = 3,
    TYPEID_PLAYER = 4,
    TYPEID_GAMEOBJECT = 5,
    TYPEID_DYNAMICOBJECT = 6,
    TYPEID_CORPSE = 7,
    NUM_TYPEIDS = 8,
};

struct WowTlsBlock {
    /* 0000 */ uint32_t m_tlsStart;
    /* 0004 */ uint32_t m_unused04;
    /* 0008 */ ObjectMgr* m_pObjMgr;
    /* 000C */ uint32_t unused_000C;
    /* 0010 */ WOWGUID m_loginCharacterGuid;
};

#endif
