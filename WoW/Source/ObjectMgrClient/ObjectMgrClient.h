#ifndef WOW_SOURCE_OBJECTMGRCLIENT_OBJECTMGRCLIENT_H
#define WOW_SOURCE_OBJECTMGRCLIENT_OBJECTMGRCLIENT_H

#include <cstddef>
#include <cstdint>
#include <windows.h>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/Object/ObjectClient/Types.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/WGUID.h"

class CDataStore;
class CGObject_C;
class CGPlayer_C;
class CHashKeyGUID;
class CMovementGlobals;
class RealmConnection;
enum HANDLER_PRIORITY : int32_t {
    HANDLER_PRIORITY_NORMAL = 0,
    HANDLER_PRIORITY_HIGH = 1,
};

typedef int32_t (*MIRRORHANDLERFUNC)(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param);
typedef int32_t (*OBJECTENUMFUNC)(WOWGUID guid, void* param);

class CMirrorHandler : public TSLinkedNode<CMirrorHandler> {
    public:
    /* 0008 */ TSLink<CMirrorHandler> m_link2;
    /* 0010 */ MIRRORHANDLERFUNC m_function;
    /* 0014 */ void* m_functionParam;
    /* 0018 */ uint32_t m_count;
    /* 001C */ uint32_t m_fieldByteOffset;
    /* 0020 */ uint32_t m_mirrorByteOffset;
    /* 0024 */ uint32_t m_fieldByteSize;
    /* 0028 */ uint32_t m_linkPositionSelector;
    /* 002C */ uint8_t m_dispatching;
    /* 002D */ uint8_t m_deletePending;
    /* 002E */ uint8_t m_alwaysFire;
    /* 002F */ uint8_t unused_002F;

    ~CMirrorHandler();
};

class ObjectMgr {
    public:
    /* 0000 */ TSHashTable<CGObject_C, CHashKeyGUID> m_objects;
    /* 0028 */ TSHashTable<CGObject_C, CHashKeyGUID> m_lazyCleanupObjects;
    /* 0050 */ TSExplicitList<CGObject_C, offsetof(CGObject_C, m_link)> m_lazyCleanupFifo [NUM_CLIENT_OBJECT_TYPES - 1];
    /* 00A4 */ TSExplicitList<CGObject_C, offsetof(CGObject_C, m_link)> m_visibleObjects;
    /* 00B0 */ TSExplicitList<CGObject_C, offsetof(CGObject_C, m_link)> m_reenabledObjects;
    /* 00BC */ uint32_t unused_00BC;
    /* 00C0 */ WOWGUID m_playerGuid;
    /* 00C8 */ uint32_t m_type;
    /* 00CC */ uint32_t m_mapId;
    /* 00D0 */ RealmConnection* m_realmConnection;
    /* 00D4 */ CMovementGlobals* m_movementGlobal;

    ObjectMgr(uint32_t type);
    ~ObjectMgr();
};

CGObject_C* ClntObjMgrAllocObject(OBJECT_TYPE_ID typeId, WOWGUID guid);
void ClntObjMgrDestroy();
void ClntObjMgrDestroyShared();
int32_t ClntObjMgrEnumVisibleObjects(OBJECTENUMFUNC func, void* param);
WOWGUID ClntObjMgrGetActivePlayer();
CGPlayer_C* ClntObjMgrGetActivePlayerObj();
ObjectMgr* ClntObjMgrGetCurrent();
uint32_t ClntObjMgrGetMapID();
CMovementGlobals* ClntObjMgrGetMovementGlobals();
uint32_t ClntObjMgrGetPlayerType();
void ClntObjMgrHideObject(WOWGUID guid);
void ClntObjMgrInitialize();
void ClntObjMgrInitializeShared();
void ClntObjMgrInitializeStd(uint32_t mapId);
int32_t ClntObjMgrIsValid(int32_t a1);
void ClntObjMgrLinkInNewObject(CGObject_C* object);
void ClntObjMgrLockObject(WOWGUID guid);
void ClntObjMgrReenableObject(WOWGUID guid);
CGObject_C* ClntObjMgrObjectPtr(WOWGUID guid, OBJECT_TYPE mask, const char* fileName, int32_t lineNumber);
void ClntObjMgrSetMovementGlobals(CMovementGlobals* globals);
void ClntObjMgrSetObjMirrorHandler(WOWGUID guid, OBJECT_TYPE_ID typeId, uint32_t fieldOffset, uint32_t fieldSize, MIRRORHANDLERFUNC function, void* param, HANDLER_PRIORITY priority, int32_t alwaysFire);
void ClntObjMgrSetTypeMirrorHandler(OBJECT_TYPE_ID typeId, uint32_t fieldOffset, uint32_t fieldSize, MIRRORHANDLERFUNC function, void* param, HANDLER_PRIORITY priority, int32_t alwaysFire);
void ClntObjMgrShowObject(WOWGUID guid);
void ClntObjMgrUnlockObject(WOWGUID guid, const char* fileName, int32_t lineNumber);
void ClntObjMgrUnsetObjMirrorHandler(WOWGUID guid, OBJECT_TYPE_ID typeId, uint32_t fieldOffset, MIRRORHANDLERFUNC function, void* param);
void ClntObjMgrUnsetTypeMirrorHandler(OBJECT_TYPE_ID typeId, uint32_t fieldOffset, MIRRORHANDLERFUNC function, void* param);
void MirrorInitialize();
int32_t Packet_SMSG_COMPRESSED_UPDATE_OBJECT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_DESTROY_OBJECT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_UPDATE_OBJECT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

#endif
