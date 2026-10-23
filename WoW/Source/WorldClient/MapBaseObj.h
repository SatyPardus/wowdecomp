#ifndef WOW_SOURCE_WORLDCLIENT_MAPBASEOBJ_H
#define WOW_SOURCE_WORLDCLIENT_MAPBASEOBJ_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/WorldClient/CMapBaseObjLink.h"
#include "WoW/Source/WorldClient/Types.h"

class CM2Lighting;
class CMapBaseObj;

class CMapBaseObj {
    public:
    enum : uint16_t {
        Type_BaseObj = 1,
        Type_Chunk = 0x4,
        Type_MapObjDef = 0x8,
        Type_MapObjDefGroup = 0x10,
        Type_Entity = 0x20,
        Type_DoodadDef = 0x40,
        Type_Light = 0x80,
        Type_UserCreated = 0x200,
        Type_StaticEntityEx = 0x300,
    };

    // Caller-created entities survive map unload; StaticEntityEx is a combined factory tag.
    // Historical Type_Area = 0x2 is not set by the current area factory.
    /* 0004 */ int32_t m_objectIndex;
    /* 0008 */ uint16_t m_type;
    /* 000A */ uint16_t m_refCount;
    /* 000C */ MapObjFlag m_flags;
    /* 0010 */ TSLink<CMapBaseObj> m_lameAssLink;
    /* 0018 */ TSExplicitList<CMapBaseObjLink, offsetof(CMapBaseObjLink, m_ownerLink)> m_parentLinkList;

    virtual ~CMapBaseObj();
    virtual void SelectLights(CM2Lighting* lighting);
    virtual void SelectUnderwater(CM2Lighting* lighting);

    CMapBaseObj();
};

#endif
