#ifndef WOW_SOURCE_OBJECT_OBJECT_H
#define WOW_SOURCE_OBJECT_OBJECT_H

#include <cstddef>
#include <cstdint>
#include <windows.h>
#include "WoW/Source/Object/ObjectClient/Types.h"
#include "WoW/Source/Object/Types.h"
#include "WoW/Source/Object/WGUID.h"

class CGObjectData;

class CGObject {
    public:
    static const uint32_t s_savedFields[3];

    static int32_t GetSavedIndexFromDataIndex(uint32_t index);
    static int32_t TotalFieldsSaved();

    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ CGObjectData* m_obj;
    /* 000C */ void* m_objMirror;
    /* 0010 */ uint32_t m_heapIndex;
    /* 0014 */ OBJECT_TYPE_ID m_typeId;

    virtual ~CGObject();
};

class CGObjectData {
    public:
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ OBJECT_TYPE m_type;
    /* 000C */ uint32_t m_entryID;
    /* 0010 */ float m_scale;
    /* 0014 */ uint32_t m_pad;
};

#endif
