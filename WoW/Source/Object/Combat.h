#ifndef WOW_SOURCE_OBJECT_COMBAT_H
#define WOW_SOURCE_OBJECT_COMBAT_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"

class CClientObjCreate;

// Runtime state, populated field by field from CClientObjCreate. Native arm64
// GUID member calls require their natural alignment inside CGUnit_C as well.
#if !defined(__APPLE__)
#pragma pack(push, 4)
#endif
class CCombat {
    public:
    /* 0000 */ WOWGUID m_attacking;

    void SetAttacking(WOWGUID guid);
    void SetClientInitData(const CClientObjCreate& create);
};

class CCombatClient : public CCombat {
    public:
    /* 0008 */ uint32_t m_attackSent;

    void SetAttackSent(WOWGUID guid);
};
#if !defined(__APPLE__)
#pragma pack(pop)
#else
static_assert(alignof(CCombat) >= alignof(WOWGUID));
static_assert(alignof(CCombatClient) >= alignof(WOWGUID));
#endif

#endif
