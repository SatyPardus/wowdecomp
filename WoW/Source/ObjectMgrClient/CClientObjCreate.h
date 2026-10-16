#ifndef WOW_SOURCE_OBJECTMGRCLIENT_CCLIENTOBJCREATE_H
#define WOW_SOURCE_OBJECTMGRCLIENT_CCLIENTOBJCREATE_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/CClientMoveUpdate.h"

class CDataStore;

class CClientObjCreate {
    public:
    /* 0000 */ CClientMoveUpdate m_move;
    /* 02A8 */ uint32_t m_updateFlags;
    /* 02AC */ uint32_t m_lowGuid;
    /* 02B0 */ uint32_t m_highGuid;
    /* 02B4 */ uint32_t unused_02B4;
    /* 02B8 */ WOWGUID m_attackTarget;
    /* 02C0 */ uint32_t m_transportTime;
    /* 02C4 */ uint32_t m_vehicleId;
    /* 02C8 */ float m_vehicleOrientation;
    /* 02CC */ uint32_t unused_02CC;
    /* 02D0 */ uint64_t m_packedRotation;

    static void Skip(CDataStore* msg);

    CClientObjCreate();
    int32_t Get(CDataStore* msg);
};

#endif
