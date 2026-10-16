#ifndef WOW_SOURCE_UI_COMMENTATORFRAME_H
#define WOW_SOURCE_UI_COMMENTATORFRAME_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/Net/Types.h"

class CDataStore;
struct lua_State;

class CGPlayer_C;

struct CommentatorInstanceKey {
    /* 0000 */ uint32_t unk_0000;
    /* 0004 */ uint32_t unk_0004;
    /* 0008 */ uint32_t unk_0008;

    CommentatorInstanceKey();
    void Write(CDataStore* msg) const;
    void Read(CDataStore* msg);
};

// INLINED
inline CommentatorInstanceKey::CommentatorInstanceKey() {
    this->unk_0000 = 0;
    this->unk_0004 = 13;
    this->unk_0008 = 0;
}

class CGCommentator {
    public:
    struct PlayerInfo {
        /* 0000 */ WOWGUID m_guid;
        /* 0008 */ uint32_t unk_0008;
        /* 000C */ uint32_t unk_000C;
        /* 0010 */ uint32_t unk_0010;
        /* 0014 */ uint32_t unk_0014;
        /* 0018 */ uint32_t m_faction;
        /* 001C */ CommentatorInstanceKey m_key;
        /* 0028 */ uint32_t unk_0028;
        /* 002C */ uint32_t unk_002C;
        /* 0030 */ uint32_t unk_0030;
        /* 0034 */ uint32_t unk_0034;
        /* 0038 */ uint32_t unk_0038;
        /* 003C */ uint32_t unk_003C;
        /* 0040 */ char m_name[48];
        /* 0070 */ uint8_t unk_0070;
        /* 0071 */ uint8_t unk_0071;
        /* 0072 */ uint8_t unk_0072;
    };

    struct InstanceInfo {
        /* 0000 */ WOWGUID m_guid;
        /* 0008 */ uint32_t m_mapID;
        /* 000C */ CommentatorInstanceKey m_key;
        /* 0018 */ uint32_t unk_0018;
        /* 001C */ uint32_t m_numPlayers[2];
        /* 0024 */ uint32_t unused_0024;
        /* 0028 */ CGCommentator::PlayerInfo m_players[2][5];
    };

    struct MapInfo {
        /* 0000 */ uint32_t unk_0000;
        /* 0004 */ uint32_t unk_0004;
        /* 0008 */ uint32_t unk_0008;
        /* 000C */ uint32_t unused_000C;
        /* 0010 */ TSGrowableArray<CGCommentator::InstanceInfo> m_instances;
    };

    struct QueueNode {
        /* 0000 */ WOWGUID unk_0000;
        /* 0008 */ WOWGUID unk_0008;
        /* 0010 */ int32_t unk_0010;
        /* 0014 */ int32_t unused_0014;
    };

    static void PostInit(CGPlayer_C* player);
    static CGCommentator s_Commentator;

    static void InitializeGame();
    static void ShutdownGame();
    static void RegisterScriptFunctions();
    static void UnregisterScriptFunctions();

    /* 0000 */ float m_facing;
    /* 0004 */ float m_pitch;
    /* 0008 */ float m_yawVelocity;
    /* 000C */ C3Vector m_position;
    /* 0018 */ float m_forwardVelocity;
    /* 001C */ float m_strafeVelocity;
    /* 0020 */ float m_verticalVelocity;
    /* 0024 */ uint32_t unused_0024;
    /* 0028 */ WOWGUID m_lookAtTarget;
    /* 0030 */ WOWGUID m_followTarget;
    /* 0038 */ float m_distance;
    /* 003C */ float m_fov;
    /* 0040 */ float m_targetFov;
    /* 0044 */ float m_moveSpeed;
    /* 0048 */ int32_t m_cameraCollision;
    /* 004C */ float m_targetHeightOffset;
    /* 0050 */ TSGrowableArray<CGCommentator::MapInfo> m_maps;
    /* 0060 */ uint32_t m_mapID;
    /* 0064 */ CommentatorInstanceKey m_key;
    /* 0070 */ uint32_t m_playerFaction;
    /* 0074 */ uint32_t m_playerIndex;
    /* 0078 */ WOWGUID m_battlemaster;
    /* 0080 */ TSGrowableArray<CGCommentator::QueueNode> m_skirmishQueue;
    /* 0090 */ int32_t m_skirmishMode;

    static int32_t OnReceivePlayerInfo(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
    static int32_t OnReceiveMapInfo(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
    static int32_t OnStateChanged(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
    static int32_t OnSkirmishPlayersInQueue(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
    static int32_t OnSkirmishPlayersModeState(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

    uint64_t GetArenaMember(int32_t index);
    void Init();
    void OnUpdate(float elapsed);
    void SnapCameraToPlayer();
    void UpdateCameraVelocity(uint32_t controlFlags, int32_t canControl);
    void SetMoveSpeed(float speed);
    void SetPosYawPitch(const C3Vector& position, float facing, float pitch);
    void SetFov(float fov);
    CGCommentator::MapInfo* GetMapInfoByIndex(uint32_t index);
    bool GetCommentatorInfo(CGCommentator::MapInfo** map, CGCommentator::InstanceInfo** instance);
    void SetPlayersInQueue(CDataStore* msg);
};

class Commentator {
    public:
    /* 0000 */ uint32_t unused_0000;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint8_t unused_0008[4];
    /* 000C */ uint32_t unused_000C;
    /* 0010 */ uint32_t unused_0010;
    /* 0014 */ uint32_t unused_0014;
    /* 0018 */ uint8_t unused_0018[40];
    /* 0040 */ uint32_t unused_0040;
};


#endif
