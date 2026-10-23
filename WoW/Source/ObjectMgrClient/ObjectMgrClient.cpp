#include <cstdint>
#include <cstring>
#include <ctime>
#include <malloc.h>
#include <new>
#include "ObjectMgrClient.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Net/NetClient/NetClient.h"
#include "WoW/Source/Object/Container.h"
#include "WoW/Source/Object/Corpse.h"
#include "WoW/Source/Object/DynamicObject.h"
#include "WoW/Source/Object/GameObject.h"
#include "WoW/Source/Object/Item.h"
#include "WoW/Source/Object/ObjectClient/Container_C.h"
#include "WoW/Source/Object/ObjectClient/Corpse_C.h"
#include "WoW/Source/Object/ObjectClient/DynamicObject_C.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/ObjectMgrClient/CClientObjCreate.h"
#include "WoW/Source/Ui/BattlefieldInfo.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/WowServices/RealmConnection.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x00DD2008
static thread_local ObjectMgr* s_curMgr;

// STATIC: 0x00B4B014
static ObjectMgr* s_savedObjMgr;

// STATIC: 0x00B4B020
static TSList<CMirrorHandler, TSGetLink<CMirrorHandler> > s_typeMirrorHandlers[NUM_CLIENT_OBJECT_TYPES][1326];

// STATIC: 0x00B4AFF4
static uint32_t s_objHeapIds[NUM_CLIENT_OBJECT_TYPES];

// STATIC: 0x00B4B018
static int32_t s_heapsAllocated;

// STATIC: 0x00AC3CC4
static const char* s_objNames[NUM_CLIENT_OBJECT_TYPES] = { "CGObject_C", "CGItem_C", "CGContainer_C", "CGUnit_C", "CGPlayer_C", "CGGameObject_C", "CGDynamicObject_C", "CGCorpse_C" };

// STATIC: 0x00AC3CE4
static const uint32_t s_objHeapSize[NUM_CLIENT_OBJECT_TYPES] = { 0, 512, 32, 64, 64, 64, 32, 32 };

// STATIC: 0x00AC3D04
static const uint32_t s_objDwordCount[NUM_CLIENT_OBJECT_TYPES] = { 0x6, 0x40, 0x8A, 0x94, 0x52E, 0x12, 0xC, 0x24 };

// STATIC: 0x00B6A160
uint32_t s_objTotalSize[NUM_CLIENT_OBJECT_TYPES] = {
    sizeof(CGObject_C) + sizeof(CGObjectData) + CGObject::TotalFieldsSaved() * sizeof(uint32_t),
    sizeof(CGItem_C) + sizeof(CGObjectData) + sizeof(CGItemData) + CGItem::TotalFieldsSaved() * sizeof(uint32_t),
    sizeof(CGContainer_C) + sizeof(CGObjectData) + sizeof(CGItemData) + sizeof(CGContainerData) + CGContainer::TotalFieldsSaved() * sizeof(uint32_t),
    sizeof(CGUnit_C) + sizeof(CGObjectData) + sizeof(CGUnitData) + CGUnit::TotalFields() * sizeof(uint32_t),
    sizeof(CGPlayer_C) + sizeof(CGObjectData) + sizeof(CGUnitData) + sizeof(CGPlayerData) + CGPlayer::TotalRemoteFields() * sizeof(uint32_t),
    sizeof(CGGameObject_C) + sizeof(CGObjectData) + sizeof(CGGameObjectData) + CGGameObject::TotalFieldsSaved() * sizeof(uint32_t),
    sizeof(CGDynamicObject_C) + sizeof(CGObjectData) + sizeof(CGDynamicObjectData) + CGDynamicObject::TotalFields() * sizeof(uint32_t),
    sizeof(CGCorpse_C) + sizeof(CGObjectData) + sizeof(CGCorpseData) + CGCorpse::TotalFieldsSaved() * sizeof(uint32_t),
};

// VERIFY: 9aded9c4 manual
// OFFSET: 0x004D5850
static CMirrorHandler* AssignMirrorHandler(TSList<CMirrorHandler, TSGetLink<CMirrorHandler> >* list, uint32_t fieldByteOffset, uint32_t mirrorByteOffset, uint32_t fieldByteSize, MIRRORHANDLERFUNC function, void* param, HANDLER_PRIORITY priority, int32_t alwaysFire) {
    CMirrorHandler* handler = list->NewNode(2, 0, 0);
    handler->m_fieldByteOffset = fieldByteOffset;
    handler->m_mirrorByteOffset = mirrorByteOffset;
    handler->m_fieldByteSize = fieldByteSize;
    handler->m_function = function;
    handler->m_functionParam = param;
    handler->m_linkPositionSelector = priority;
    handler->m_dispatching = 0;
    handler->m_deletePending = 0;
    handler->m_alwaysFire = alwaysFire != 0;
    return handler;
}

// VERIFY: d0d1c29f manual
// OFFSET: 0x004D3BF0
static uint32_t GetObjectTypeFieldByteOffset(OBJECT_TYPE_ID typeId) {
    switch (typeId) {
        case ID_ITEM:
        case ID_UNIT:
        case ID_GAMEOBJECT:
        case ID_DYNAMICOBJECT:
        case ID_CORPSE: {
            return OBJECT_END * sizeof(uint32_t);
        }

        case ID_CONTAINER: {
            return 0x100;
        }

        case ID_PLAYER: {
            return 0x250;
        }

        default: {
            return 0;
        }
    }
}

// VERIFY: 846bf934 auto
// OFFSET: 0x004D44F0
static uint32_t TypeDescriptorToMirrorOffset(OBJECT_TYPE_ID typeId, uint32_t offset, uint32_t size) {
    switch (typeId) {
        case ID_ITEM: {
            return CGItem::DescriptorToMirrorOffset(offset, size, 0, 0);
        }

        case ID_CONTAINER: {
            return CGContainer::DescriptorToMirrorOffset(offset, size, 0, 0);
        }

        case ID_UNIT: {
            return CGUnit::DescriptorToMirrorOffset(offset, size, 0, 0);
        }

        case ID_PLAYER: {
            return CGPlayer::DescriptorToMirrorOffset(offset, size, 0, 0);
        }

        case ID_GAMEOBJECT: {
            return CGGameObject::DescriptorToMirrorOffset(offset, size, 0, 0);
        }

        case ID_DYNAMICOBJECT: {
            return CGDynamicObject::DescriptorToMirrorOffset(offset, size, 0, 0);
        }

        case ID_CORPSE: {
            return CGCorpse::DescriptorToMirrorOffset(offset, size, 0, 0);
        }

        default: {
            return (offset & 3) + 4 * CGObject::GetSavedIndexFromDataIndex(offset >> 2);
        }
    }
}

// VERIFY: fbf971d2 manual
// OFFSET: 0x004D5900
static void UnassignMirrorHandler(TSList<CMirrorHandler, TSGetLink<CMirrorHandler> >* list, MIRRORHANDLERFUNC function, void* param) {
    for (CMirrorHandler* handler = list->Head(); handler; handler = list->Next(handler)) {
        if (handler->m_function == function && handler->m_functionParam == param) {
            if (handler->m_dispatching) {
                handler->m_deletePending = 1;
            } else {
                list->DeleteNode(handler);
            }

            return;
        }
    }
}

// VERIFY: d1ddc062 manual
// OFFSET: 0x004D36C0
void NetClient::PushObjMgr(ClntObjMgr* mgr) {
    if (s_savedObjMgr) {
        return;
    }

    ObjectMgr* current = s_curMgr;

    if (reinterpret_cast<ObjectMgr*>(mgr) != current) {
        s_savedObjMgr = current;
        s_curMgr = reinterpret_cast<ObjectMgr*>(mgr);
    }
}

// VERIFY: a15d9aa7 manual
// OFFSET: 0x004D3700
void NetClient::PopObjMgr() {
    if (s_savedObjMgr) {
        s_curMgr = s_savedObjMgr;
        s_savedObjMgr = nullptr;
    }
}

// VERIFY: f58d5266 manual
// OFFSET: 0x004D3750
int32_t ClntObjMgrIsValid(int32_t a1) {
    ObjectMgr* mgr = s_curMgr;
    return mgr && SMemIsValidPointer(mgr, sizeof(ObjectMgr), a1);
}

// VERIFY: 1c37b7e9 manual
// OFFSET: 0x004D3790
WOWGUID ClntObjMgrGetActivePlayer() {
    ObjectMgr* mgr = s_curMgr;

    if (!mgr) {
        return WOWGUID();
    }

    return mgr->m_playerGuid;
}

// VERIFY: 60136c4f manual
// OFFSET: 0x004D4DB0
CGObject_C* ClntObjMgrObjectPtr(WOWGUID guid, OBJECT_TYPE mask, const char* fileName, int32_t lineNumber) {
    ObjectMgr* mgr = s_curMgr;

    if (!mgr || !guid) {
        return nullptr;
    }

    CGObject_C* object = mgr->m_objects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (object && !(object->m_obj->m_type & mask)) {
        return nullptr;
    }

    return object;
}

// VERIFY: a72ee853 manual
// OFFSET: 0x004D4B30
int32_t ClntObjMgrEnumVisibleObjects(OBJECTENUMFUNC func, void* param) {
    for (CGObject_C* object = s_curMgr->m_visibleObjects.Head(); object; object = s_curMgr->m_visibleObjects.Next(object)) {
        if (!func(object->m_key.m_guid, param)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 96f2f630 manual
// OFFSET: 0x004D3730
ObjectMgr* ClntObjMgrGetCurrent() {
    return s_curMgr;
}

// VERIFY: 4f92f8c6 manual
// OFFSET: 0x004D37E0
uint32_t ClntObjMgrGetMapID() {
    ObjectMgr* mgr = s_curMgr;

    if (!mgr) {
        return 0;
    }

    return mgr->m_mapId;
}

// VERIFY: bc9aab23 manual
// OFFSET: 0x004D3810
CMovementGlobals* ClntObjMgrGetMovementGlobals() {
    ObjectMgr* mgr = s_curMgr;

    if (!mgr) {
        return nullptr;
    }

    return mgr->m_movementGlobal;
}

// VERIFY: 05ff89f3 manual
// OFFSET: 0x004D37C0
uint32_t ClntObjMgrGetPlayerType() {
    return s_curMgr->m_type;
}

// VERIFY: de0acdd8 manual
// OFFSET: 0x004D4CA0
void ClntObjMgrHideObject(WOWGUID guid) {
    ObjectMgr* mgr = s_curMgr;
    CGObject_C* object = mgr->m_objects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (object && mgr->m_visibleObjects.IsLinked(object)) {
        mgr->m_visibleObjects.UnlinkNode(object);
    }
}

// VERIFY: 614fb018 manual
// OFFSET: 0x004D6BC0
void ClntObjMgrLinkInNewObject(CGObject_C* object) {
    WOWGUID guid = object->m_obj->m_guid;
    s_curMgr->m_objects.Insert(object, static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));
}

// VERIFY: be60d36c manual
// OFFSET: 0x004D4D60
void ClntObjMgrLockObject(WOWGUID guid) {
    CGObject_C* object = s_curMgr->m_objects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (object) {
        object->SetObjectLocked(1);
    }
}

// VERIFY: 2f5bfe78 manual
// OFFSET: 0x004D3840
void ClntObjMgrSetMovementGlobals(CMovementGlobals* globals) {
    ObjectMgr* mgr = s_curMgr;

    if (mgr) {
        mgr->m_movementGlobal = globals;
    }
}

// VERIFY: ba2b5330 manual
// OFFSET: 0x004D5BA0
void ClntObjMgrSetTypeMirrorHandler(OBJECT_TYPE_ID typeId, uint32_t fieldOffset, uint32_t fieldSize, MIRRORHANDLERFUNC function, void* param, HANDLER_PRIORITY priority, int32_t alwaysFire) {
    uint32_t mirrorOffset = TypeDescriptorToMirrorOffset(typeId, fieldOffset, fieldSize);
    AssignMirrorHandler(&s_typeMirrorHandlers[typeId][fieldOffset >> 2], fieldOffset + GetObjectTypeFieldByteOffset(typeId), mirrorOffset, fieldSize, function, param, priority, alwaysFire);
}

// VERIFY: df93f0c4 manual
// OFFSET: 0x004D4D00
void ClntObjMgrShowObject(WOWGUID guid) {
    ObjectMgr* mgr = s_curMgr;
    CGObject_C* object = mgr->m_objects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (object && !mgr->m_visibleObjects.IsLinked(object)) {
        mgr->m_visibleObjects.LinkToTail(object);
    }
}

// VERIFY: 4c96b67b auto
// OFFSET: 0x004D5C40
void ClntObjMgrUnsetTypeMirrorHandler(OBJECT_TYPE_ID typeId, uint32_t fieldOffset, MIRRORHANDLERFUNC function, void* param) {
    UnassignMirrorHandler(&s_typeMirrorHandlers[typeId][fieldOffset >> 2], function, param);
}

// VERIFY: f01935d7 manual
// OFFSET: 0x004D3C40
static OBJECT_TYPE_ID IncTypeId(CGObject_C* object, OBJECT_TYPE_ID typeId) {
    switch (object->m_obj->m_type) {
        case HEIR_TYPE_ITEM:
        case HEIR_TYPE_CONTAINER: {
            if (typeId == ID_OBJECT) {
                return ID_ITEM;
            }

            if (typeId == ID_ITEM) {
                return ID_CONTAINER;
            }

            break;
        }

        case HEIR_TYPE_UNIT:
        case HEIR_TYPE_PLAYER: {
            if (typeId == ID_OBJECT) {
                return ID_UNIT;
            }

            if (typeId == ID_UNIT) {
                return ID_PLAYER;
            }

            break;
        }

        case HEIR_TYPE_GAMEOBJECT: {
            if (typeId == ID_OBJECT) {
                return ID_GAMEOBJECT;
            }

            break;
        }

        case HEIR_TYPE_DYNAMICOBJECT: {
            if (typeId == ID_OBJECT) {
                return ID_DYNAMICOBJECT;
            }

            break;
        }

        case HEIR_TYPE_CORPSE: {
            if (typeId == ID_OBJECT) {
                return ID_CORPSE;
            }

            break;
        }
    }

    return NUM_CLIENT_OBJECT_TYPES;
}

// VERIFY: de904275 manual
// OFFSET: 0x004D3D40
static TSList<CMirrorHandler, TSGetLink<CMirrorHandler> >* GetObjectMirrorList(OBJECT_TYPE_ID typeId, CGObject_C* object, uint32_t index) {
    TSList<void>* list;

    switch (typeId) {
        case ID_OBJECT: {
            list = &object->m_objMirrorLists[index];
            break;
        }

        case ID_ITEM: {
            list = &static_cast<CGItem_C*>(object)->m_itemMirrorLists[index];
            break;
        }

        case ID_CONTAINER: {
            list = &static_cast<CGContainer_C*>(object)->m_containerMirrorLists[index];
            break;
        }

        case ID_UNIT: {
            list = &static_cast<CGUnit_C*>(object)->m_unitMirrorLists[index];
            break;
        }

        case ID_PLAYER: {
            CGPlayer_C* player = static_cast<CGPlayer_C*>(object);

            if (player->IsActivePlayer() && index >= 176) {
                list = &player->m_playerLocalMirrorLists[index - 176];
            } else {
                list = &player->m_playerMirrorLists[index];
            }

            break;
        }

        case ID_GAMEOBJECT: {
            list = &static_cast<CGGameObject_C*>(object)->m_gameObjectMirrorLists[index];
            break;
        }

        case ID_DYNAMICOBJECT: {
            list = &static_cast<CGDynamicObject_C*>(object)->m_dynamicObjectMirrorLists[index];
            break;
        }

        case ID_CORPSE: {
            list = &static_cast<CGCorpse_C*>(object)->m_corpseMirrorLists[index];
            break;
        }

        default: {
            return nullptr;
        }
    }

    return reinterpret_cast<TSList<CMirrorHandler, TSGetLink<CMirrorHandler> >*>(list);
}

// VERIFY: 6db3c6e7 auto
// OFFSET: 0x004D3DF0
static uint32_t GetNumDwordBlocks(uint32_t type, WOWGUID guid) {
    switch (type) {
        case HEIR_TYPE_OBJECT: {
            return 6;
        }

        case HEIR_TYPE_ITEM: {
            return 64;
        }

        case HEIR_TYPE_CONTAINER: {
            return 138;
        }

        case HEIR_TYPE_UNIT: {
            return 148;
        }

        case HEIR_TYPE_PLAYER: {
            return guid == ClntObjMgrGetActivePlayer() ? 1326 : 324;
        }

        case HEIR_TYPE_GAMEOBJECT: {
            return 18;
        }

        case HEIR_TYPE_DYNAMICOBJECT: {
            return 12;
        }

        case HEIR_TYPE_CORPSE: {
            return 36;
        }

        default: {
            return 0;
        }
    }
}

// VERIFY: 09b26427 manual
// OFFSET: 0x004D3F10
static int32_t ExtractDirtyMasks(CDataStore* msg, uint8_t* blockCount, uint32_t* masks) {
    uint8_t count;
    msg->Get(count);
    *blockCount = count;

    if (count > 42) {
        return 0;
    }

    int32_t i = 0;

    for (; i < count; i++) {
        msg->Get(masks[i]);
    }

    if (i < 42) {
        memset(&masks[i], 0, sizeof(uint32_t) * (42 - i));
    }

    return 1;
}

// VERIFY: 3918bd3a manual
// OFFSET: 0x004D3F80
static int32_t SkipPartialObjectUpdate(CDataStore* msg) {
    uint32_t masks[42];
    uint8_t blockCount;

    if (!ExtractDirtyMasks(msg, &blockCount, masks)) {
        return 0;
    }

    for (uint32_t i = 0; i < 32u * blockCount; i++) {
        if (masks[i >> 5] & (1 << (i & 31))) {
            uint32_t value;
            msg->Get(value);
        }
    }

    return 1;
}

// VERIFY: 3b1806d2 auto
// OFFSET: 0x004D3FF0
static void InitObject(CGObject_C* object, const CClientObjCreate& create, uint32_t time) {
    switch (object->m_typeId) {
        case ID_ITEM: {
            new (object) CGItem_C(time, create);
            break;
        }

        case ID_CONTAINER: {
            new (object) CGContainer_C(time, create);
            break;
        }

        case ID_UNIT: {
            new (object) CGUnit_C(time, create);
            object->AddWorldObject();
            break;
        }

        case ID_PLAYER: {
            new (object) CGPlayer_C(time, create);
            object->AddWorldObject();
            break;
        }

        case ID_GAMEOBJECT: {
            new (object) CGGameObject_C(time, create);
            object->AddWorldObject();
            break;
        }

        case ID_DYNAMICOBJECT: {
            new (object) CGDynamicObject_C(time, create);
            object->AddWorldObject();
            break;
        }

        case ID_CORPSE: {
            new (object) CGCorpse_C(time, create);
            object->AddWorldObject();
            break;
        }
    }
}

// VERIFY: d78e827e manual
// OFFSET: 0x004D4090
static void DestroyRemovedObject(CGObject_C* object) {
    int32_t activePlayer = object->m_obj->m_guid == ClntObjMgrGetActivePlayer();

    switch (object->m_obj->m_type) {
        case HEIR_TYPE_OBJECT:
        case HEIR_TYPE_ITEM:
        case HEIR_TYPE_CONTAINER:
        case HEIR_TYPE_UNIT:
        case HEIR_TYPE_PLAYER:
        case HEIR_TYPE_GAMEOBJECT:
        case HEIR_TYPE_DYNAMICOBJECT:
        case HEIR_TYPE_CORPSE: {
            object->~CGObject_C();
            break;
        }
    }

    if (activePlayer) {
        SMemFree(object, __FILE__, __LINE__, 0);
    } else {
        ObjectFree(s_objHeapIds[object->m_typeId], object->m_heapIndex);
    }
}

// VERIFY: e9d855a0 auto
// OFFSET: 0x004D41C0
static void SkipSetOfObjects(CDataStore* msg) {
    uint32_t count;
    msg->Get(count);

    for (uint32_t i = 0; i < count; i++) {
        SmartGUID guid;
        *msg >> guid;
    }
}

// VERIFY: 5fd9f216 auto
// OFFSET: 0x004D43F0
static uint32_t ObjDescriptorToMirrorOffset(OBJECT_TYPE_ID typeId, int32_t local, uint32_t offset, uint32_t size) {
    switch (typeId) {
        case ID_CONTAINER: {
            if (offset >= 0x100) {
                return CGContainer::DescriptorToMirrorOffset(offset, size, 0, 0x100);
            }
        }

        case ID_ITEM: {
            if (offset >= 0x18) {
                return CGItem::DescriptorToMirrorOffset(offset, size, 0, 0x18);
            }

            break;
        }

        case ID_PLAYER: {
            if (offset >= 0x250) {
                return CGPlayer::DescriptorToMirrorOffset(offset, size, local, 0x250);
            }
        }

        case ID_UNIT: {
            if (offset >= 0x18) {
                return CGUnit::DescriptorToMirrorOffset(offset, size, local, 0x18);
            }

            break;
        }

        case ID_GAMEOBJECT: {
            if (offset >= 0x18) {
                return CGGameObject::DescriptorToMirrorOffset(offset, size, 0, 0x18);
            }

            break;
        }

        case ID_DYNAMICOBJECT: {
            if (offset >= 0x18) {
                return CGDynamicObject::DescriptorToMirrorOffset(offset, size, 0, 0x18);
            }

            break;
        }

        case ID_CORPSE: {
            if (offset >= 0x18) {
                return CGCorpse::DescriptorToMirrorOffset(offset, size, 0, 0x18);
            }

            break;
        }
    }

    return (offset & 3) + 4 * CGObject::GetSavedIndexFromDataIndex(offset >> 2);
}

// VERIFY: 839b7b55 manual
// OFFSET: 0x004D45B0
static void SetupObjectStorage(CGObject_C* object, OBJECT_TYPE_ID typeId, WOWGUID guid) {
    uint8_t* base = reinterpret_cast<uint8_t*>(object);

    switch (typeId) {
        case ID_OBJECT: {
            uint32_t size = sizeof(CGObjectData);
            uint32_t* storage = reinterpret_cast<uint32_t*>(base + sizeof(CGObject_C));
            object->SetStorage(storage, reinterpret_cast<uint32_t*>(base + sizeof(CGObject_C) + size));
            memset(storage, 0, size);
            break;
        }

        case ID_ITEM: {
            uint32_t size = sizeof(CGObjectData) + sizeof(CGItemData);
            uint32_t* storage = reinterpret_cast<uint32_t*>(base + sizeof(CGItem_C));
            static_cast<CGItem_C*>(object)->SetStorage(storage, reinterpret_cast<uint32_t*>(base + sizeof(CGItem_C) + size));
            memset(storage, 0, size);
            break;
        }

        case ID_CONTAINER: {
            uint32_t size = sizeof(CGObjectData) + sizeof(CGItemData) + sizeof(CGContainerData);
            uint32_t* storage = reinterpret_cast<uint32_t*>(base + sizeof(CGContainer_C));
            static_cast<CGContainer_C*>(object)->SetStorage(storage, reinterpret_cast<uint32_t*>(base + sizeof(CGContainer_C) + size));
            memset(storage, 0, size);
            break;
        }

        case ID_UNIT: {
            uint32_t size = sizeof(CGObjectData) + sizeof(CGUnitData);
            uint32_t* storage = reinterpret_cast<uint32_t*>(base + sizeof(CGUnit_C));
            static_cast<CGUnit_C*>(object)->SetStorage(storage, reinterpret_cast<uint32_t*>(base + sizeof(CGUnit_C) + size));
            memset(storage, 0, size);
            break;
        }

        case ID_PLAYER: {
            CGPlayer_C* player = static_cast<CGPlayer_C*>(object);
            uint32_t size;

            if (guid == ClntObjMgrGetActivePlayer()) {
                size = sizeof(CGObjectData) + sizeof(CGUnitData) + sizeof(CGPlayerLocalData);
                player->m_playerLocalMirrorLists = reinterpret_cast<TSList<void>*>(base + sizeof(CGPlayer_C) + size + CGPlayer::TotalFields() * sizeof(uint32_t));
            } else {
                size = sizeof(CGObjectData) + sizeof(CGUnitData) + sizeof(CGPlayerData);
                player->m_playerLocalMirrorLists = nullptr;
            }

            uint32_t* storage = reinterpret_cast<uint32_t*>(base + sizeof(CGPlayer_C));
            player->SetStorage(storage, reinterpret_cast<uint32_t*>(base + sizeof(CGPlayer_C) + size));
            memset(storage, 0, size);
            break;
        }

        case ID_GAMEOBJECT: {
            uint32_t size = sizeof(CGObjectData) + sizeof(CGGameObjectData);
            uint32_t* storage = reinterpret_cast<uint32_t*>(base + sizeof(CGGameObject_C));
            static_cast<CGGameObject_C*>(object)->SetStorage(storage, reinterpret_cast<uint32_t*>(base + sizeof(CGGameObject_C) + size));
            memset(storage, 0, size);
            break;
        }

        case ID_DYNAMICOBJECT: {
            uint32_t size = sizeof(CGObjectData) + sizeof(CGDynamicObjectData);
            uint32_t* storage = reinterpret_cast<uint32_t*>(base + sizeof(CGDynamicObject_C));
            static_cast<CGDynamicObject_C*>(object)->SetStorage(storage, reinterpret_cast<uint32_t*>(base + sizeof(CGDynamicObject_C) + size));
            memset(storage, 0, size);
            break;
        }

        case ID_CORPSE: {
            uint32_t size = sizeof(CGObjectData) + sizeof(CGCorpseData);
            uint32_t* storage = reinterpret_cast<uint32_t*>(base + sizeof(CGCorpse_C));
            static_cast<CGCorpse_C*>(object)->SetStorage(storage, reinterpret_cast<uint32_t*>(base + sizeof(CGCorpse_C) + size));
            memset(storage, 0, size);
            break;
        }
    }
}

// VERIFY: 550dec9c manual
// OFFSET: 0x004D4850
static void ExpirePendingHandlers(TSExplicitList<CMirrorHandler, offsetof(CMirrorHandler, m_link2)>* pending) {
    CMirrorHandler* handler = pending->Head();

    while (handler) {
        CMirrorHandler* next = pending->Next(handler);

        if (handler->m_count-- == 1) {
            pending->Link(handler)->Unlink();
        }

        handler = next;
    }
}

// VERIFY: addd8a0a manual
// OFFSET: 0x004D48C0
static void ExpireOldObjects(uint32_t expireTime, OBJECT_TYPE_ID typeId) {
    CGObject_C* object = s_curMgr->m_lazyCleanupFifo[typeId - 1].Head();

    if (object && static_cast<int32_t>(OsGetAsyncTimeMs() - object->m_disableTime - expireTime) >= 0) {
        s_curMgr->m_lazyCleanupObjects.Unlink(object);
        object->m_link.Unlink();
        DestroyRemovedObject(object);
    }
}

// VERIFY: 30ceb511 manual
// OFFSET: 0x004D4930
CGObject_C* ClntObjMgrAllocObject(OBJECT_TYPE_ID typeId, WOWGUID guid) {
    if (guid == ClntObjMgrGetActivePlayer()) {
        return static_cast<CGObject_C*>(SMemAlloc(sizeof(CGPlayer_C) + s_objDwordCount[ID_PLAYER] * sizeof(uint32_t) + CGPlayer::TotalFields() * sizeof(uint32_t) + 1002 * sizeof(TSList<void>), __FILE__, __LINE__, 0));
    }

    ExpireOldObjects(10000, typeId);

    uint32_t memHandle;

    if (!ObjectAlloc(s_objHeapIds[typeId], &memHandle, nullptr, false)) {
        return nullptr;
    }

    CGObject_C* object = static_cast<CGObject_C*>(ObjectPtr(s_objHeapIds[typeId], memHandle));
    object->m_heapIndex = memHandle;
    return object;
}

// VERIFY: 9065c202 manual
// OFFSET: 0x004D49E0
static int32_t CCommand_ObjUsage(const char* command, const char* arguments) {
    ObjectMgr* mgr = s_curMgr;
    uint32_t visible = 0;
    uint32_t active = 0;
    uint32_t lazy = 0;

    for (CGObject_C* object = mgr->m_visibleObjects.Head(); object; object = mgr->m_visibleObjects.Next(object)) {
        visible++;
    }

    for (CGObject_C* object = mgr->m_objects.m_fulllist.Head(); object; object = mgr->m_objects.m_fulllist.Next(object)) {
        active++;
    }

    for (CGObject_C* object = mgr->m_lazyCleanupObjects.m_fulllist.Head(); object; object = mgr->m_lazyCleanupObjects.m_fulllist.Next(object)) {
        lazy++;
    }

    ConsoleWrite("Object manager list status:", HIGHLIGHT_COLOR);
    ConsoleWriteA("    Active objects:              %u objects (%u visible)", HIGHLIGHT_COLOR, active, visible);
    ConsoleWriteA("    Objects waiting to be freed: %u objects", HIGHLIGHT_COLOR, lazy);
    return 1;
}

// VERIFY: 8485585f auto
// OFFSET: 0x004D4AC0
void ClntObjMgrInitializeShared() {
    if (!s_heapsAllocated) {
        for (uint32_t i = ID_ITEM; i < NUM_CLIENT_OBJECT_TYPES; i++) {
            s_objHeapIds[i] = ObjectAllocAddHeap(s_objTotalSize[i], s_objHeapSize[i], s_objNames[i], true);
        }

        s_heapsAllocated = 1;
    }

    MirrorInitialize();
    ConsoleCommandRegister("ObjUsage", CCommand_ObjUsage, CATEGORY_GAME, nullptr);
}

// VERIFY: 6d6f17bf manual
// OFFSET: 0x004D5150
static void CallMirrorFunctions(TSExplicitList<CMirrorHandler, offsetof(CMirrorHandler, m_link2)>* pending, WOWGUID guid, CGObject_C* object, OBJECT_TYPE_ID typeId) {
    CMirrorHandler* handler = pending->Head();

    while (handler) {
        handler->m_dispatching = 1;
        handler->m_count = 1;

        const void* mirror = static_cast<uint8_t*>(object->m_objMirror) + handler->m_mirrorByteOffset;

        if (handler->m_alwaysFire || memcmp(reinterpret_cast<uint8_t*>(object->m_obj) + handler->m_fieldByteOffset, mirror, handler->m_fieldByteSize)) {
            handler->m_function(guid, handler->m_fieldByteOffset - GetObjectTypeFieldByteOffset(typeId), handler->m_fieldByteSize, mirror, handler->m_functionParam);
        }

        CMirrorHandler* current = handler;
        handler = pending->Next(handler);

        if (current->m_deletePending) {
            delete current;
        } else {
            current->m_dispatching = 0;
        }
    }
}

// VERIFY: 5369b3f6 manual
// OFFSET: 0x004D52B0
static void CopyMirrorFields(TSList<CMirrorHandler, TSGetLink<CMirrorHandler> >* list, CGObject_C* object) {
    int32_t local = object->m_obj->m_guid == ClntObjMgrGetActivePlayer();

    for (CMirrorHandler* handler = list->Head(); handler; handler = handler->m_link.Next()) {
        uint32_t mirrorOffset = ObjDescriptorToMirrorOffset(object->m_typeId, local, handler->m_fieldByteOffset, handler->m_fieldByteSize);
        memcpy(static_cast<uint8_t*>(object->m_objMirror) + mirrorOffset, reinterpret_cast<uint8_t*>(object->m_obj) + handler->m_fieldByteOffset, handler->m_fieldByteSize);
    }
}

// VERIFY: 199f0f4a manual
// OFFSET: 0x004D5350
static int32_t LinkPendingNodes(TSExplicitList<CMirrorHandler, offsetof(CMirrorHandler, m_link2)>* pending, TSList<CMirrorHandler, TSGetLink<CMirrorHandler> >* list) {
    if (!list || !list->Head()) {
        return 0;
    }

    for (CMirrorHandler* handler = list->Head(); handler; handler = handler->m_link.Next()) {
        uint32_t misalign = handler->m_fieldByteOffset & 3;
        pending->LinkNode(handler, (handler->m_linkPositionSelector != HANDLER_PRIORITY_HIGH) + 1, nullptr);
        handler->m_count = (handler->m_fieldByteSize + misalign + 3) >> 2;
    }

    return 1;
}

// VERIFY: 5cf2342e manual
// OFFSET: 0x004D53C0
static int32_t FillInPartialObjectData(CGObject_C* object, WOWGUID guid, CDataStore* msg, bool skipMirrorUpdate, bool zeroFillAbsent) {
    uint32_t masks[42];
    uint8_t blockCount;

    if (!ExtractDirtyMasks(msg, &blockCount, masks)) {
        return 0;
    }

    OBJECT_TYPE_ID typeId = ID_OBJECT;
    uint32_t typeBase = 0;
    uint32_t numBlocks = GetNumDwordBlocks(object->m_obj->m_type, guid);
    TSExplicitList<CMirrorHandler, offsetof(CMirrorHandler, m_link2)> pending;

    for (uint32_t i = 0; i < numBlocks; i++) {
        if (i >= s_objDwordCount[typeId]) {
            typeBase = s_objDwordCount[typeId];
            typeId = IncTypeId(object, typeId);
        }

        if (!skipMirrorUpdate) {
            ExpirePendingHandlers(&pending);

            TSList<CMirrorHandler, TSGetLink<CMirrorHandler> >* list = &s_typeMirrorHandlers[typeId][i - typeBase];

            if (LinkPendingNodes(&pending, list)) {
                CopyMirrorFields(list, object);
            }

            list = GetObjectMirrorList(typeId, object, i - typeBase);

            if (LinkPendingNodes(&pending, list)) {
                CopyMirrorFields(list, object);
            }
        }

        uint32_t value;

        if (masks[i >> 5] & (1 << (i & 31))) {
            msg->GetArray(reinterpret_cast<uint8_t*>(&value), sizeof(value));
        } else if (zeroFillAbsent) {
            value = 0;
        } else {
            continue;
        }

        object->SetBlock(i, value);
    }

    return 1;
}

// VERIFY: 3cbc372a manual
// OFFSET: 0x004D5550
static int32_t CallMirrorHandlers(CDataStore* msg, bool postInit, WOWGUID guid) {
    if (!postInit) {
        SmartGUID smartGuid;
        *msg >> smartGuid;
        guid = smartGuid;
    }

    CGObject_C* object = s_curMgr->m_objects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (!object) {
        return SkipPartialObjectUpdate(msg);
    }

    uint32_t masks[42];
    uint8_t blockCount;

    if (!ExtractDirtyMasks(msg, &blockCount, masks)) {
        return 0;
    }

    OBJECT_TYPE_ID typeId = ID_OBJECT;
    uint32_t typeBase = 0;
    uint32_t numBlocks = GetNumDwordBlocks(object->m_obj->m_type, guid);
    TSExplicitList<CMirrorHandler, offsetof(CMirrorHandler, m_link2)> pending;

    for (uint32_t i = 0; i < numBlocks; i++) {
        if (i >= s_objDwordCount[typeId]) {
            typeBase = s_objDwordCount[typeId];
            typeId = IncTypeId(object, typeId);
        }

        ExpirePendingHandlers(&pending);
        LinkPendingNodes(&pending, &s_typeMirrorHandlers[typeId][i - typeBase]);
        LinkPendingNodes(&pending, GetObjectMirrorList(typeId, object, i - typeBase));

        if (masks[i >> 5] & (1 << (i & 31))) {
            uint32_t value = 0;
            msg->Get(value);
        } else if (!postInit) {
            continue;
        }

        CallMirrorFunctions(&pending, guid, object, typeId);
    }

    return 1;
}

// VERIFY: 0214d9f3 manual
// OFFSET: 0x004D5720
static void FreeMirrorHandlers(TSList<void>* lists, uint32_t count) {
    TSList<CMirrorHandler, TSGetLink<CMirrorHandler> >* list = reinterpret_cast<TSList<CMirrorHandler, TSGetLink<CMirrorHandler> >*>(lists);

    for (uint32_t i = count; i; i--, list++) {
        for (CMirrorHandler* handler = list->Head(); handler; handler = list->Head()) {
            list->DeleteNode(handler);
        }
    }
}

// VERIFY: 249bc521 manual
// OFFSET: 0x004D5770
static void ClearMirrorLists(CGObject_C* object) {
    switch (object->m_typeId) {
        case ID_CONTAINER: {
            FreeMirrorHandlers(static_cast<CGContainer_C*>(object)->m_containerMirrorLists, 74);
        }

        case ID_ITEM: {
            FreeMirrorHandlers(static_cast<CGItem_C*>(object)->m_itemMirrorLists, 58);
            FreeMirrorHandlers(object->m_objMirrorLists, 6);
            return;
        }

        case ID_PLAYER: {
            CGPlayer_C* player = static_cast<CGPlayer_C*>(object);
            FreeMirrorHandlers(player->m_playerMirrorLists, 176);

            if (player->IsActivePlayer()) {
                FreeMirrorHandlers(player->m_playerLocalMirrorLists, 1002);
            }
        }

        case ID_UNIT: {
            FreeMirrorHandlers(static_cast<CGUnit_C*>(object)->m_unitMirrorLists, 142);
            FreeMirrorHandlers(object->m_objMirrorLists, 6);
            return;
        }

        case ID_GAMEOBJECT: {
            FreeMirrorHandlers(static_cast<CGGameObject_C*>(object)->m_gameObjectMirrorLists, 12);
            break;
        }

        case ID_DYNAMICOBJECT: {
            FreeMirrorHandlers(static_cast<CGDynamicObject_C*>(object)->m_dynamicObjectMirrorLists, 6);
            break;
        }

        case ID_CORPSE: {
            FreeMirrorHandlers(static_cast<CGCorpse_C*>(object)->m_corpseMirrorLists, 30);
            break;
        }
    }

    FreeMirrorHandlers(object->m_objMirrorLists, 6);
}

// VERIFY: 3d049fb8 manual
// OFFSET: 0x004D5950
static void DestroyAllObjects() {
    CGObject_C* object;

    while ((object = s_curMgr->m_objects.m_fulllist.Head()) != nullptr) {
        s_curMgr->m_objects.Unlink(object);

        if (s_curMgr->m_visibleObjects.IsLinked(object)) {
            s_curMgr->m_visibleObjects.UnlinkNode(object);
        }

        ClearMirrorLists(object);
        DestroyRemovedObject(object);
    }

    while ((object = s_curMgr->m_lazyCleanupObjects.m_fulllist.Head()) != nullptr) {
        s_curMgr->m_lazyCleanupObjects.Unlink(object);
        object->m_link.Unlink();
        DestroyRemovedObject(object);
    }

    s_curMgr->m_realmConnection->ResetHandler(SMSG_UPDATE_OBJECT);
    s_curMgr->m_realmConnection->ResetHandler(SMSG_COMPRESSED_UPDATE_OBJECT);
    s_curMgr->m_realmConnection->ResetHandler(SMSG_DESTROY_OBJECT);
}

// VERIFY: 29ffe840 manual
// OFFSET: 0x004D5A80
void ClntObjMgrSetObjMirrorHandler(WOWGUID guid, OBJECT_TYPE_ID typeId, uint32_t fieldOffset, uint32_t fieldSize, MIRRORHANDLERFUNC function, void* param, HANDLER_PRIORITY priority, int32_t alwaysFire) {
    ObjectMgr* mgr = s_curMgr;
    CGObject_C* object = mgr->m_objects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (!object) {
        return;
    }

    uint32_t fieldByteOffset = fieldOffset + GetObjectTypeFieldByteOffset(typeId);
    uint32_t mirrorByteOffset = ObjDescriptorToMirrorOffset(object->m_typeId, guid == ClntObjMgrGetActivePlayer(), fieldByteOffset, fieldSize);
    AssignMirrorHandler(GetObjectMirrorList(typeId, object, fieldOffset >> 2), fieldByteOffset, mirrorByteOffset, fieldSize, function, param, priority, alwaysFire);
}

// VERIFY: a922342a manual
// OFFSET: 0x004D5B40
void ClntObjMgrUnsetObjMirrorHandler(WOWGUID guid, OBJECT_TYPE_ID typeId, uint32_t fieldOffset, MIRRORHANDLERFUNC function, void* param) {
    CGObject_C* object = s_curMgr->m_objects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (object) {
        UnassignMirrorHandler(GetObjectMirrorList(typeId, object, fieldOffset >> 2), function, param);
    }
}

// VERIFY: 8be6328a manual
// OFFSET: 0x004D5C80
void ClntObjMgrDestroyShared() {
    for (uint32_t typeId = 0; typeId < NUM_CLIENT_OBJECT_TYPES; typeId++) {
        for (uint32_t i = 0; i < 1326; i++) {
            TSList<CMirrorHandler, TSGetLink<CMirrorHandler> >* list = &s_typeMirrorHandlers[typeId][i];

            for (CMirrorHandler* handler = list->Head(); handler; handler = list->Head()) {
                list->DeleteNode(handler);
            }
        }
    }

    ConsoleCommandUnregister("ObjUsage");
}

// VERIFY: 9c3645a6 manual
// OFFSET: 0x004D6170
ObjectMgr::ObjectMgr(uint32_t type) {
    this->m_type = type;
    this->m_playerGuid = WOWGUID();
    this->m_mapId = 0;
    this->m_realmConnection = nullptr;
    this->m_movementGlobal = nullptr;
}

// VERIFY: 64bc6bb1 manual
// OFFSET: 0x004D6290
ObjectMgr::~ObjectMgr() {
    if (this->m_realmConnection) {
        this->m_realmConnection->m_objMgr = nullptr;
    }

    if (this == s_savedObjMgr) {
        s_savedObjMgr = nullptr;
    }
}

// VERIFY: 10156365 manual
// OFFSET: 0x004D63B0
static int32_t PostInitObject(uint32_t time, CDataStore* msg, bool reenable) {
    SmartGUID guid;
    *msg >> guid;

    uint8_t typeId;
    msg->Get(typeId);

    if (!guid.m_guid) {
        return 0;
    }

    CGObject_C* object = s_curMgr->m_objects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (!object) {
        return 0;
    }

    CClientObjCreate create;

    if (!create.Get(msg)) {
        return 0;
    }

    if ((object->m_modelFlags & 0x20000) && (object->m_obj->m_type & TYPE_UNIT)) {
      static_cast<CGUnit_C*>(object)->SetClientInitData(create, object->m_obj->m_guid == ClntObjMgrGetActivePlayer());
    }

    if (object->m_modelFlags & 0x40000) {
        return CallMirrorHandlers(msg, true, guid);
    }

    switch (typeId) {
        case ID_OBJECT: {
            object->PostInit(time, create, reenable);
            break;
        }

        case ID_ITEM:
        case ID_CONTAINER: {
            static_cast<CGItem_C*>(object)->PostInit(time, create, reenable);
            break;
        }

        case ID_UNIT: {
            static_cast<CGUnit_C*>(object)->PostInit(time, create, reenable);
            break;
        }

        case ID_PLAYER: {
            static_cast<CGPlayer_C*>(object)->PostInit(time, create, reenable);
            break;
        }

        case ID_GAMEOBJECT: {
            static_cast<CGGameObject_C*>(object)->PostInit(time, create, reenable);
            break;
        }

        case ID_DYNAMICOBJECT: {
            static_cast<CGDynamicObject_C*>(object)->PostInit(time, create, reenable);
            break;
        }

        case ID_CORPSE: {
            static_cast<CGCorpse_C*>(object)->PostInit(time, create, reenable);
            break;
        }

        default: {
            DebugLog("PostInitObject: unknown object type %d", typeId);
            return 0;
        }
    }

    return SkipPartialObjectUpdate(msg);
}

// VERIFY: 8ac7551f manual
// OFFSET: 0x004D6750
void ClntObjMgrDestroy() {
    if (!s_curMgr) {
        return;
    }

    DestroyAllObjects();

    ObjectMgr* mgr = s_curMgr;
    s_curMgr = nullptr;
    delete mgr;
}

// VERIFY: 362ba495 manual
// OFFSET: 0x004D6AE0
static CGObject_C* GetUpdateObject(WOWGUID guid, int32_t* reenabled) {
    *reenabled = 0;

    ObjectMgr* mgr = s_curMgr;
    CGObject_C* object = mgr->m_objects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (object) {
        object->SetDisablePending(0);
        return object;
    }

    object = mgr->m_lazyCleanupObjects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (!object) {
        return nullptr;
    }

    mgr->m_lazyCleanupObjects.Unlink(object);
    object->m_link.Unlink();
    mgr->m_objects.Insert(object, static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    ObjectMgr* current = s_curMgr;

    if (!current->m_visibleObjects.IsLinked(object) && !current->m_reenabledObjects.IsLinked(object)) {
        *reenabled = 1;
        current->m_reenabledObjects.LinkToTail(object);
    }

    return object;
}

// VERIFY: ebf77040 auto
// OFFSET: 0x004D6F10
void ClntObjMgrReenableObject(WOWGUID guid) {
    int32_t reenabled;
    CGObject_C* object = GetUpdateObject(guid, &reenabled);

    if (object && reenabled) {
        object->Reenable();
    }
}

// VERIFY: 3154e6f8 manual
// OFFSET: 0x004D6C00
static int32_t CreateObject(CDataStore* msg, uint32_t time) {
    SmartGUID guid;
    *msg >> guid;

    uint8_t typeId;
    msg->Get(typeId);

    if (typeId >= NUM_CLIENT_OBJECT_TYPES) {
        return 0;
    }

    int32_t reenabled;
    CGObject_C* object = GetUpdateObject(guid, &reenabled);

    if (object) {
        CClientObjCreate::Skip(msg);

        if (!FillInPartialObjectData(object, object->m_obj->m_guid, msg, false, true)) {
            return 0;
        }

        if (reenabled) {
            object->Reenable();
        }

        return 1;
    }

    CClientObjCreate create;

    if (!create.Get(msg)) {
        return 0;
    }

    if (create.m_updateFlags & 0x1) {
        s_curMgr->m_playerGuid = guid;
    }

    object = ClntObjMgrAllocObject(static_cast<OBJECT_TYPE_ID>(typeId), guid);
    SetupObjectStorage(object, static_cast<OBJECT_TYPE_ID>(typeId), guid);
    object->SetTypeID(static_cast<OBJECT_TYPE_ID>(typeId));

    if (!FillInPartialObjectData(object, guid, msg, true, false)) {
        return 0;
    }

    InitObject(object, create, time);
    s_curMgr->m_visibleObjects.LinkToTail(object);
    return 1;
}

// VERIFY: 09701ca8 auto
// OFFSET: 0x004D6DA0
static void PostMovementUpdate(CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;

    CClientMoveUpdate update;
    *msg >> update;

    if (guid == CGUnit_C::s_activeMover) {
        return;
    }

    int32_t reenabled;
    CGObject_C* object = GetUpdateObject(guid, &reenabled);

    if (!object) {
        DebugLog("Failed to update object movement.  Object (0x%016I64X) unknown to client!", guid.m_guid);
        return;
    }

    static_cast<CGUnit_C*>(object)->SetUpdateInfo(update, object->m_obj->m_guid == CGUnit_C::s_activeMover);

    if (reenabled) {
        object->Reenable();
    }
}

// VERIFY: fdb8eb0f auto
// OFFSET: 0x004D6E80
static int32_t PartialUpdateFromFullUpdate(CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;

    int32_t reenabled;
    CGObject_C* object = GetUpdateObject(guid, &reenabled);

    if (!object) {
        DebugLog("Failed to update object data.  Object (0x%016I64X) unknown to client!", guid.m_guid);
        return SkipPartialObjectUpdate(msg);
    }

    if (!FillInPartialObjectData(object, object->m_obj->m_guid, msg, false, false)) {
        return 0;
    }

    if (reenabled) {
        object->Reenable();
    }

    return 1;
}

// VERIFY: 70f27e51 auto
// OFFSET: 0x004D6F40
static void UpdateInRangeObjects(CDataStore* msg) {
    uint32_t count;
    msg->Get(count);

    while (count) {
        count--;

        SmartGUID guid;
        *msg >> guid;

        if (guid != ClntObjMgrGetActivePlayer()) {
            int32_t reenabled;
            CGObject_C* object = GetUpdateObject(guid, &reenabled);

            if (object && reenabled) {
                object->Reenable();
            }
        }
    }
}

// VERIFY: 9ca63081 manual
// OFFSET: 0x004D6FC0
static void ObjDelete(CGObject_C* object) {
    ClearMirrorLists(object);

    ObjectMgr* mgr = s_curMgr;
    mgr->m_objects.Unlink(object);

    CHashKeyGUID key = object->m_key;
    uint32_t hashval = object->m_hashval;

    if (mgr->m_visibleObjects.IsLinked(object)) {
        mgr->m_visibleObjects.UnlinkNode(object);
    }

    mgr->m_lazyCleanupObjects.Insert(object, hashval, key);
    s_curMgr->m_lazyCleanupFifo[object->m_typeId - 1].LinkToTail(object);
}

// VERIFY: 1ddf9f77 auto
// OFFSET: 0x004D7050
static int32_t ObjectUpdateFirstPass(uint32_t index, CDataStore* msg, uint32_t time, uint32_t count) {
    for (; index < count; index++) {
        uint8_t type;
        msg->Get(type);

        switch (type) {
            case 0: {
                if (!PartialUpdateFromFullUpdate(msg)) {
                    return 0;
                }

                break;
            }

            case 1: {
                SmartGUID guid;
                *msg >> guid;
                CClientMoveUpdate::Skip(msg);
                break;
            }

            case 2:
            case 3: {
                if (!CreateObject(msg, time)) {
                    return 0;
                }

                break;
            }

            case 5: {
                UpdateInRangeObjects(msg);
                break;
            }

            default: {
                DebugLog("Unknown client update packet type (%d)!", type);
                return 0;
            }
        }
    }

    return 1;
}

// VERIFY: df5394bc manual
// OFFSET: 0x004D7100
static int32_t ObjectUpdateSecondPass(CDataStore* msg, uint32_t time, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        uint8_t type;
        msg->Get(type);

        switch (type) {
            case 0: {
                if (!CallMirrorHandlers(msg, false, WOWGUID())) {
                    return 0;
                }

                break;
            }

            case 1: {
                PostMovementUpdate(msg);
                break;
            }

            case 2:
            case 3: {
                if (!PostInitObject(time, msg, type == 3)) {
                    return 0;
                }

                break;
            }

            case 4:
            case 5: {
                SkipSetOfObjects(msg);
                break;
            }
        }
    }

    CGObject_C* object;

    while ((object = s_curMgr->m_reenabledObjects.Head()) != nullptr) {
        s_curMgr->m_visibleObjects.LinkToTail(object);
        object->PostReenable();
    }

    return 1;
}

// VERIFY: 3bda6338 manual
// OFFSET: 0x004D7230
static void UpdateOutOfRangeObjects(CDataStore* msg) {
    uint32_t count;
    msg->Get(count);

    CVehiclePassenger_C::StartAddingPendingRescueTransitions();

    uint32_t read = msg->m_read;

    for (uint32_t i = count; i; i--) {
        SmartGUID guid;
        *msg >> guid;

        if (guid == ClntObjMgrGetActivePlayer()) {
            continue;
        }

        CGObject_C* object = s_curMgr->m_objects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

        if (!object) {
            continue;
        }

        if (CGBattlefieldInfo::m_instanceType == 4) {
            WOWGUID objectGuid = object->m_obj->m_guid;
            CGBattlefieldInfo::OnArenaUnitOutOfRange(objectGuid);
        }

        object->HandleOutOfRange(static_cast<OUT_OF_RANGE_TYPE>(0));

        if (object->IsObjectLocked()) {
            object->SetDisablePending(1);
        } else {
            object->SetDisablePending(0);
            object->Disable();
        }
    }

    msg->m_read = read;

    for (uint32_t i = count; i; i--) {
        SmartGUID guid;
        *msg >> guid;

        if (guid == ClntObjMgrGetActivePlayer()) {
            continue;
        }

        CGObject_C* object = s_curMgr->m_objects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

        if (object && !object->IsObjectLocked()) {
            ObjDelete(object);
        }
    }

    CVehiclePassenger_C::ExecutePendingRescueTransitions();
}

// VERIFY: 1da078ed manual
// OFFSET: 0x004D73A0
int32_t Packet_SMSG_UPDATE_OBJECT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t count;
    msg->Get(count);

    uint32_t read = msg->m_read;
    uint32_t index = 0;

    uint8_t type = 0;
    msg->Get(type);

    if (type == 4) {
        UpdateOutOfRangeObjects(msg);
        index = 1;
    } else {
        msg->m_read = read;
    }

    int32_t result = 0;

    if (ObjectUpdateFirstPass(index, msg, time, count)) {
        msg->m_read = read;
        result = ObjectUpdateSecondPass(msg, time, count);
    }

    for (uint32_t typeId = ID_ITEM; typeId < NUM_CLIENT_OBJECT_TYPES; typeId++) {
        ExpireOldObjects(120000, static_cast<OBJECT_TYPE_ID>(typeId));
    }

    return result;
}

// VERIFY: d5d28268 manual
// OFFSET: 0x004D74D0
int32_t Packet_SMSG_COMPRESSED_UPDATE_OBJECT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t size;
    msg->Get(size);

    uint32_t compressedSize = msg->m_size - msg->m_read;
    void* compressed;
    msg->GetDataInSitu(compressed, compressedSize);

    uint8_t* buffer;
    bool stackBuffer;

    if (size >= 0x2000) {
        buffer = static_cast<uint8_t*>(SMemAlloc(size, __FILE__, __LINE__, 0));
        stackBuffer = false;
    } else {
        buffer = static_cast<uint8_t*>(alloca(size));
        stackBuffer = true;
    }

    uint32_t uncompressedSize = size;
    int32_t result;

    if (zlib_uncompress(buffer, &uncompressedSize, static_cast<const uint8_t*>(compressed), compressedSize)) {
        result = 0;
    } else {
        WDataStore data;
        data.PutData(buffer, uncompressedSize);
        data.m_read = 0;
        result = Packet_SMSG_UPDATE_OBJECT(nullptr, SMSG_UPDATE_OBJECT, time, &data);
    }

    if (!stackBuffer) {
        SMemFree(buffer, __FILE__, __LINE__, 0);
    }

    return result;
}

// VERIFY: 2f490dee manual
// OFFSET: 0x004D7610
int32_t Packet_SMSG_DESTROY_OBJECT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);

    uint8_t death;
    msg->Get(death);

    CGObject_C* object = s_curMgr->m_objects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (!object) {
        return 1;
    }

    if (death && (object->m_obj->m_type & TYPE_UNIT) && static_cast<CGUnit_C*>(object)->m_unit->m_unitFieldHealth > 0) {
        static_cast<CGUnit_C*>(object)->OnDeath();
    }

    object->HandleOutOfRange(static_cast<OUT_OF_RANGE_TYPE>(1));

    if (object->IsObjectLocked()) {
        object->SetDisablePending(1);
        return 1;
    }

    object->SetDisablePending(0);
    object->Disable();
    ObjDelete(object);
    return 1;
}

// VERIFY: ed7998e3 manual
// OFFSET: 0x004D76E0
void ClntObjMgrInitialize() {
    s_curMgr->m_realmConnection->SetMessageHandler(SMSG_UPDATE_OBJECT, Packet_SMSG_UPDATE_OBJECT, nullptr);
    s_curMgr->m_realmConnection->SetMessageHandler(SMSG_COMPRESSED_UPDATE_OBJECT, Packet_SMSG_COMPRESSED_UPDATE_OBJECT, nullptr);
    s_curMgr->m_realmConnection->SetMessageHandler(SMSG_DESTROY_OBJECT, Packet_SMSG_DESTROY_OBJECT, nullptr);
}

// VERIFY: c3e30362 manual
// OFFSET: 0x004D7750
void ClntObjMgrInitializeStd(uint32_t mapId) {
    CGGameUI::SetLastInstanceTime(static_cast<int32_t>(time(nullptr)), CGGameUI::m_iCurrentMapID, mapId);

    ObjectMgr* mgr = new (__FILE__, __LINE__) ObjectMgr(0);
    g_clientConnection->m_objMgr = reinterpret_cast<ClntObjMgr*>(mgr);
    s_curMgr = mgr;
    mgr->m_realmConnection = g_clientConnection;
    ClntObjMgrInitialize();
    s_curMgr->m_mapId = mapId;
}

// VERIFY: 35db2401 manual
// OFFSET: 0x004D77E0
void ClntObjMgrUnlockObject(WOWGUID guid, const char* fileName, int32_t lineNumber) {
    CGObject_C* object = s_curMgr->m_objects.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (!object) {
        return;
    }

    object->SetObjectLocked(0);

    if (object->IsObjectLocked() || !(object->m_modelFlags & 0x100000)) {
        return;
    }

    object->HandleOutOfRange(static_cast<OUT_OF_RANGE_TYPE>(2));

    if (object->IsObjectLocked()) {
        object->SetDisablePending(1);
    } else {
        object->SetDisablePending(0);
        object->Disable();
        ObjDelete(object);
    }
}

// VERIFY: 88f08ff1 manual
// OFFSET: 0x004D4C20
CMirrorHandler::~CMirrorHandler() {
}

// VERIFY: 7a5ce34e manual
// OFFSET: 0x004038F0
CGPlayer_C* ClntObjMgrGetActivePlayerObj() {
    return static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
}

