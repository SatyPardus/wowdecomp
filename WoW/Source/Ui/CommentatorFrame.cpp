#include "CommentatorFrame.h"
#include <cmath>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Tempest/C33Matrix.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/UIUtil/Types.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WorldClient/World.h"

// STATIC: 0x00ACE4A8
CGCommentator CGCommentator::s_Commentator;

// VERIFY: 892bcb14 manual
// OFFSET: 0x0056B5C0
void CGCommentator::UpdateCameraVelocity(uint32_t controlFlags, int32_t canControl) {
    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__) || !CGWorldFrame::GetActiveCamera()) {
        return;
    }

    if (!canControl) {
        this->m_forwardVelocity = 0.0f;
        this->m_strafeVelocity = 0.0f;
        this->m_verticalVelocity = 0.0f;
        this->m_yawVelocity = 0.0f;
        return;
    }

    int32_t forward = (controlFlags & INPUT_MOVE_PLAYER_AUTORUN) != 0;

    if (controlFlags & INPUT_MOVE_PLAYER_FORWARD_KEY) {
        forward++;
    }

    if ((controlFlags & INPUT_TURN_PLAYER) && (controlFlags & INPUT_TURN_CAMERA)) {
        forward++;
    }

    if (controlFlags & INPUT_MOVE_PLAYER_BACKWARD_KEY) {
        forward--;
    }

    if (forward) {
        CGPlayer_C::ClearAFK(0);
        this->m_forwardVelocity = forward > 0 ? 1.0f : -1.0f;
    } else {
        this->m_forwardVelocity = 0.0f;
    }

    int32_t strafe = (controlFlags & INPUT_STRAFE_PLAYER_LEFT_KEY) != 0;

    if ((controlFlags & INPUT_PLAYER_LOOK_MASK) && (controlFlags & INPUT_TURN_PLAYER_LEFT_KEY)) {
        strafe++;
    }

    if (controlFlags & INPUT_STRAFE_PLAYER_RIGHT_KEY) {
        strafe--;
    }

    if ((controlFlags & INPUT_PLAYER_LOOK_MASK) && (controlFlags & INPUT_TURN_PLAYER_RIGHT_KEY)) {
        strafe--;
    }

    if (strafe) {
        CGPlayer_C::ClearAFK(0);
        this->m_strafeVelocity = strafe > 0 ? 1.0f : -1.0f;
    } else {
        this->m_strafeVelocity = 0.0f;
    }

    int32_t vertical = (controlFlags & INPUT_ASCEND_PLAYER_KEY) != 0;

    if (controlFlags & INPUT_DESCEND_PLAYER_KEY) {
        vertical--;
    }

    this->m_verticalVelocity = static_cast<float>(vertical);

    int32_t turn = 0;

    if (controlFlags & INPUT_TURN_PLAYER_LEFT_KEY) {
        turn = 1;
    }

    if (controlFlags & INPUT_TURN_PLAYER_RIGHT_KEY) {
        turn--;
    }

    if (controlFlags & INPUT_PLAYER_LOOK_MASK) {
        turn = 0;
    }

    this->m_yawVelocity = turn * 0.050000001f;
}

// VERIFY: a105314a manual
// OFFSET: 0x0056BC80
void CGCommentator::Init() {
    this->m_position.x = 0.0f;
    this->m_position.y = 0.0f;
    this->m_position.z = 0.0f;
    this->m_lookAtTarget.m_guid = 0;
    this->m_forwardVelocity = 0.0f;
    this->m_followTarget.m_guid = 0;
    this->m_strafeVelocity = 0.0f;
    this->m_verticalVelocity = 0.0f;
    this->m_facing = 0.0f;
    this->m_cameraCollision = 1;
    this->m_yawVelocity = 0.0f;
    this->m_mapID = 0;
    this->m_pitch = 0.0f;
    this->m_distance = 12.0f;
    this->m_fov = 1.5707964f;
    this->m_targetFov = 1.5707964f;
    this->m_moveSpeed = 10.0f;
    this->m_targetHeightOffset = 0.0f;
    this->m_key.unk_0004 = 13;
    this->m_key.unk_0008 = 0;
    this->m_key.unk_0000 = 0;
    this->m_playerFaction = 0;
    this->m_playerIndex = 0;
    this->m_battlemaster.m_guid = 0;
    this->m_maps.Clear();
    this->m_skirmishQueue.Clear();
    this->m_skirmishMode = 1;
}

// VERIFY: 59295854 auto
// OFFSET: 0x0056C150
void CGCommentator::InitializeGame() {
    CGCommentator::s_Commentator.Init();
    ClientServices::SetMessageHandler(SMSG_COMMENTATOR_PLAYER_INFO, CGCommentator::OnReceivePlayerInfo, nullptr);
    ClientServices::SetMessageHandler(SMSG_COMMENTATOR_MAP_INFO, CGCommentator::OnReceiveMapInfo, nullptr);
    ClientServices::SetMessageHandler(SMSG_COMMENTATOR_STATE_CHANGED, CGCommentator::OnStateChanged, nullptr);
    ClientServices::SetMessageHandler(SMSG_COMMENTATOR_SKIRMISH_QUEUE_RESULT1, CGCommentator::OnSkirmishPlayersInQueue, nullptr);
    ClientServices::SetMessageHandler(SMSG_COMMENTATOR_SKIRMISH_QUEUE_RESULT2, CGCommentator::OnSkirmishPlayersModeState, nullptr);
}

// VERIFY: 4126a5f5 manual
// OFFSET: 0x0056B510
void CGCommentator::SnapCameraToPlayer() {
    if (!CGWorldFrame::s_currentWorldFrame) {
        return;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (!camera) {
        return;
    }

    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    this->m_pitch = player->GetPitch();
    this->m_facing = player->GetFacing();
    this->m_position = player->GetPosition();
    this->m_position.z += 1.5f;
    camera->CommentatorSetViewAndPosition(this->m_position, this->m_facing, this->m_pitch);
}

// VERIFY: a88d79fe auto
// OFFSET: 0x0056B830
void CGCommentator::PostInit(CGPlayer_C* player) {
    if (player && player->IsCommentatorUberOrInArena()) {
        World::ObjectSetShadow(player->m_worldObject, 2, 2);

        if (CGWorldFrame::s_currentWorldFrame) {
            CGWorldFrame::GetActiveCamera()->SetView(6, 2, 0);
            CGCommentator::s_Commentator.SnapCameraToPlayer();
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_COMMENTATOR_ENTER_WORLD, nullptr);
}

// VERIFY: acad482e auto
// OFFSET: 0x00568340
void CGCommentator::ShutdownGame() {
    ClientServices::ClearMessageHandler(SMSG_COMMENTATOR_PLAYER_INFO);
    ClientServices::ClearMessageHandler(SMSG_COMMENTATOR_MAP_INFO);
    ClientServices::ClearMessageHandler(SMSG_COMMENTATOR_STATE_CHANGED);
    ClientServices::ClearMessageHandler(SMSG_COMMENTATOR_SKIRMISH_QUEUE_RESULT1);
    ClientServices::ClearMessageHandler(SMSG_COMMENTATOR_SKIRMISH_QUEUE_RESULT2);
}

// VERIFY: dadf4593 auto
// OFFSET: 0x005682C0
void CommentatorInstanceKey::Write(CDataStore* msg) const {
    msg->Put(this->unk_0000);
    msg->Put(static_cast<uint16_t>(this->unk_0008));
    msg->Put(static_cast<uint8_t>(this->unk_0004));
}

// VERIFY: 605c92c6 auto
// OFFSET: 0x00568300
void CGCommentator::SetMoveSpeed(float speed) {
    if (speed >= 40.0f) {
        this->m_moveSpeed = 40.0f;
    } else if (speed <= 0.0f) {
        this->m_moveSpeed = 0.0f;
    } else {
        this->m_moveSpeed = speed;
    }
}

// VERIFY: ae69fb61 auto
// OFFSET: 0x00568420
int32_t CGCommentator::OnSkirmishPlayersModeState(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t mode = 0;
    msg->Get(mode);
    CGCommentator::s_Commentator.m_skirmishMode = mode;
    FrameScript_SignalEvent(SCRIPT_EVENT_COMMENTATOR_SKIRMISH_QUEUE_REQUEST, nullptr);
    return 1;
}

// VERIFY: b41bcf95 auto
// OFFSET: 0x005684B0
void CGCommentator::SetPosYawPitch(const C3Vector& position, float facing, float pitch) {
    if (!CGWorldFrame::s_currentWorldFrame) {
        return;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (!camera) {
        return;
    }

    this->m_pitch = pitch;
    this->m_facing = facing;
    this->m_position = position;
    camera->CommentatorSetViewAndPosition(this->m_position, facing, pitch);
}

// VERIFY: 874452b5 manual
// OFFSET: 0x00568510
void CGCommentator::SetFov(float fov) {
    this->m_targetFov = fov;

    if (fov >= 2.0943952f) {
        fov = 2.0943952f;
    }

    this->m_targetFov = fov;

    if (fov <= 0.017453292f) {
        this->m_targetFov = 0.017453292f;
    } else {
        this->m_targetFov = fov;
    }
}

// VERIFY: de8e1244 auto
// OFFSET: 0x005686B0
void CommentatorInstanceKey::Read(CDataStore* msg) {
    uint32_t value = 0;
    msg->Get(value);
    this->unk_0000 = value;

    uint16_t extra = 0;
    msg->Get(extra);
    this->unk_0008 = extra;

    uint8_t type = 0;
    msg->Get(type);
    this->unk_0004 = type;
}

// VERIFY: bbd3f031 manual
// OFFSET: 0x00568710
CGCommentator::MapInfo* CGCommentator::GetMapInfoByIndex(uint32_t index) {
    if (index >= this->m_maps.Count()) {
        return nullptr;
    }

    return &this->m_maps.m_data[index];
}

// VERIFY: d9a0e9d9 manual
// OFFSET: 0x005688C0
bool CGCommentator::GetCommentatorInfo(CGCommentator::MapInfo** map, CGCommentator::InstanceInfo** instance) {
    if (this->m_mapID || (this->m_key.unk_0004 < 13 && this->m_key.unk_0008 && this->m_key.unk_0000)) {
        for (uint32_t i = 0; i < this->m_maps.Count(); i++) {
            CGCommentator::MapInfo* info = &this->m_maps.m_data[i];

            for (uint32_t j = 0; j < info->m_instances.Count(); j++) {
                CGCommentator::InstanceInfo* entry = &info->m_instances.m_data[j];

                if (entry->m_mapID == this->m_mapID && this->m_key.unk_0000 == entry->m_key.unk_0000 && this->m_key.unk_0008 == entry->m_key.unk_0008 && this->m_key.unk_0004 == entry->m_key.unk_0004) {
                    *map = info;
                    *instance = entry;
                    return true;
                }
            }
        }
    }

    *map = nullptr;
    *instance = nullptr;
    return false;
}

// VERIFY: 2af662fa manual
// OFFSET: 0x005689A0
void CGCommentator::OnUpdate(float elapsed) {
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (!camera) {
        return;
    }

    C33Matrix rotation(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    rotation.FromEulerAnglesZYX(this->m_facing, this->m_pitch, 0.0f);

    CGObject_C* follow = this->m_followTarget ? ClntObjMgrObjectPtr(this->m_followTarget, TYPE_OBJECT, __FILE__, __LINE__) : nullptr;

    if (follow) {
        float distance = this->m_distance - this->m_moveSpeed * this->m_forwardVelocity * elapsed;
        this->m_distance = distance;

        if (distance >= 75.0f) {
            distance = 75.0f;
        }

        this->m_distance = distance;

        if (distance <= 1.0f) {
            distance = 1.0f;
        }

        this->m_distance = distance;

        C3Vector target = follow->GetPosition();

        if (follow->m_obj->m_type & TYPE_PLAYER) {
            target.z = static_cast<CGUnit_C*>(follow)->m_movementData.m_collisionHeight * 0.75f + this->m_targetHeightOffset + target.z;
        } else {
            target.z = this->m_targetHeightOffset + target.z + 1.5f;
        }

        float back = -distance;
        C3Vector eye(rotation.a0 * back + target.x, rotation.a1 * back + target.y, target.z + back * rotation.a2);
        C3Vector hit(0.0f, 0.0f, 0.0f);
        float hitDistance = 1.0f;

        if (this->m_cameraCollision && World::Intersect(target, eye, &hit, &hitDistance, 0x100171, nullptr)) {
            C3Vector direction(target.x - hit.x, target.y - hit.y, target.z - hit.z);
            direction.NormalizeSafe();
            this->m_position.x = direction.x * 0.80000001f + hit.x;
            this->m_position.y = direction.y * 0.80000001f + hit.y;
            this->m_position.z = direction.z * 0.80000001f + hit.z;
        } else {
            this->m_position = eye;
        }

        camera->SetFacing(this->m_facing, this->m_pitch, 0.0f);
        camera->CommentatorSetViewAndPosition(this->m_position, this->m_facing, this->m_pitch);
    } else {
        CGObject_C* lookAt = this->m_lookAtTarget ? ClntObjMgrObjectPtr(this->m_lookAtTarget, TYPE_OBJECT, __FILE__, __LINE__) : nullptr;

        float forward = this->m_forwardVelocity;
        float strafe = this->m_strafeVelocity;
        float vertical = this->m_verticalVelocity;
        float speed = this->m_moveSpeed;
        C3Vector next(((rotation.a0 * forward * speed + rotation.b0 * strafe * speed) + rotation.c0 * vertical * speed) * elapsed + this->m_position.x, (rotation.b1 * strafe * speed + rotation.a1 * forward * speed + rotation.c1 * vertical * speed) * elapsed + this->m_position.y, (strafe * rotation.b2 * speed + speed * (forward * rotation.a2) + vertical * rotation.c2 * speed) * elapsed + this->m_position.z);

        float dx = next.x - this->m_position.x;
        float dy = next.y - this->m_position.y;
        float dz = next.z - this->m_position.z;

        if (sqrt(dx * dx + dz * dz + dy * dy) > 0.0) {
            C3Vector hit(0.0f, 0.0f, 0.0f);
            float hitDistance = 1.0f;

            if (!this->m_cameraCollision || !World::Intersect(this->m_position, next, &hit, &hitDistance, 0x100171, nullptr)) {
                this->m_position = next;
            }
        }

        if (lookAt) {
            C3Vector target = lookAt->GetPosition();
            float z;

            if (lookAt->m_obj->m_type & TYPE_PLAYER) {
                z = static_cast<CGUnit_C*>(lookAt)->m_movementData.m_collisionHeight * 0.75f + this->m_targetHeightOffset + target.z;
            } else {
                z = this->m_targetHeightOffset + target.z + 1.5f;
            }

            C3Vector direction(target.x - this->m_position.x, target.y - this->m_position.y, z - this->m_position.z);
            direction.NormalizeSafe();

            if (direction.x * direction.x + direction.z * direction.z + direction.y * direction.y < 0.000099999997f) {
                direction.x = 1.0f;
                direction.y = 0.0f;
                direction.z = 0.0f;
            }

            float roll;
            camera->CSimpleCamera::SetFacing(direction);
            camera->m_facing.ToEulerAnglesZYX(this->m_facing, this->m_pitch, roll);
            camera->CommentatorSetViewAndPosition(this->m_position, this->m_facing, this->m_pitch);
        } else {
            camera->m_position = this->m_position;

            if (fnotequalz(this->m_yawVelocity, 0.0f, 0.001f)) {
                this->m_facing += this->m_yawVelocity;
                camera->UpdateYaw(this->m_yawVelocity);
            }

            camera->SetFacing(camera->m_yaw, camera->m_pitch, 0.0f);
        }
    }

    float delta = this->m_targetFov - this->m_fov;
    float maxDelta = (this->m_fov - 0.017453292f) * 0.19259088f + 0.2f;

    if (maxDelta <= delta) {
        delta = maxDelta;
    }

    if (-maxDelta >= delta) {
        delta = -maxDelta;
    }

    float rate;

    if (delta > 0.0f && delta < 1.0f) {
        rate = sqrt(delta);
    } else if (delta < 0.0f && delta > -1.0f) {
        rate = -sqrt(-delta);
    } else {
        rate = delta;
    }

    float fov = elapsed * rate + this->m_fov;
    this->m_fov = fov;

    if (rate < 0.0f ? this->m_targetFov >= fov : this->m_targetFov <= fov) {
        fov = this->m_targetFov;
    }

    this->m_fov = fov;
    this->m_fov = this->m_fov > 0.017453292f ? this->m_fov : 0.017453292f;

    if (this->m_fov >= 2.0943952f) {
        this->m_fov = 2.0943952f;
        camera->m_fov = 2.0943952f;
    } else {
        camera->m_fov = this->m_fov;
    }
}

// VERIFY: c38e2f9a manual
// OFFSET: 0x0056B1E0
uint64_t CGCommentator::GetArenaMember(int32_t index) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    CGCommentator::MapInfo* map;
    CGCommentator::InstanceInfo* instance;

    if (!player || !(player->m_player->m_playerFlags & 0x80000) || !this->GetCommentatorInfo(&map, &instance)) {
        return 0;
    }

    int32_t faction = 0;

    if (index >= 5) {
        faction = 1;
        index -= 5;
    }

    if (index >= 0 && static_cast<uint32_t>(index) < instance->m_numPlayers[faction]) {
        return instance->m_players[faction][index].m_guid.m_guid;
    }

    return 0;
}

// VERIFY: 5a8913ac manual
// OFFSET: 0x0056B280
int32_t CGCommentator::OnReceivePlayerInfo(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t mapID;
    CommentatorInstanceKey key;
    msg->Get(mapID);
    key.Read(msg);

    WOWGUID guid;
    msg->Get(guid.m_guid);

    CGCommentator::InstanceInfo* instance = nullptr;

    for (uint32_t i = 0; i < CGCommentator::s_Commentator.m_maps.Count(); i++) {
        CGCommentator::MapInfo* map = &CGCommentator::s_Commentator.m_maps.m_data[i];

        for (uint32_t j = 0; j < map->m_instances.Count(); j++) {
            CGCommentator::InstanceInfo* entry = &map->m_instances.m_data[j];

            if (entry->m_mapID == mapID && key.unk_0000 == entry->m_key.unk_0000 && key.unk_0008 == entry->m_key.unk_0008 && key.unk_0004 == entry->m_key.unk_0004 && entry->m_guid == guid) {
                instance = entry;
                break;
            }
        }
    }

    if (!instance) {
        return 1;
    }

    int32_t count = 0;
    msg->Get(reinterpret_cast<uint32_t&>(count));
    instance->m_numPlayers[0] = 0;
    instance->m_numPlayers[1] = 0;

    CGCommentator::PlayerInfo info;

    for (int32_t i = 0; i < count; i++) {
        msg->Get(info.m_guid.m_guid);
        msg->GetString(info.m_name, sizeof(info.m_name));
        msg->Get(info.unk_0070);
        msg->Get(info.unk_0071);
        msg->Get(info.unk_0072);
        msg->Get(info.unk_0008);
        msg->Get(info.unk_000C);
        msg->Get(info.unk_0010);
        msg->Get(info.unk_0014);
        msg->Get(info.m_faction);
        info.m_key.Read(msg);
        msg->Get(info.unk_0028);
        msg->Get(info.unk_002C);
        msg->Get(info.unk_0030);
        msg->Get(info.unk_0034);
        msg->Get(info.unk_0038);

        uint8_t value = 0;
        msg->Get(value);
        info.unk_003C = value;

        if (info.m_faction < 2) {
            instance->m_players[info.m_faction][instance->m_numPlayers[info.m_faction]] = info;
            instance->m_numPlayers[info.m_faction]++;
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_COMMENTATOR_PLAYER_UPDATE, nullptr);
    return 1;
}

// VERIFY: 3222d968 manual
// OFFSET: 0x0056B8A0
int32_t CGCommentator::OnStateChanged(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    guid.m_guid = 0;
    msg->Get(guid.m_guid);

    uint8_t enabled = 0;
    msg->Get(enabled);

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || player->m_obj->m_guid != guid) {
        return 1;
    }

    if (enabled && player->IsCommentatorUberOrInArena()) {
        World::ObjectSetShadow(player->m_worldObject, 2, 2);

        if (CGWorldFrame::s_currentWorldFrame) {
            CGWorldFrame::GetActiveCamera()->SetView(6, 2, 0);
            CGCommentator::s_Commentator.SnapCameraToPlayer();
        }
    } else {
        World::ObjectSetShadow(player->m_worldObject, 0, 2);

        if (CGWorldFrame::s_currentWorldFrame) {
            CGWorldFrame::GetActiveCamera()->SetView(1, 2, 0);
        }
    }

    return 1;
}

// VERIFY: 1ea1fca2 manual
// OFFSET: 0x0056B9A0
void CGCommentator::SetPlayersInQueue(CDataStore* msg) {
    uint32_t mode = 0;
    msg->Get(mode);
    this->m_skirmishMode = mode;
    this->m_skirmishQueue.Clear();

    uint32_t count = 0;
    msg->Get(count);
    this->m_skirmishQueue.SetCount(count);

    for (uint32_t i = 0; i < count; i++) {
        WOWGUID first;
        first.m_guid = 0;
        msg->Get(first.m_guid);
        this->m_skirmishQueue.m_data[i].unk_0000 = first;

        WOWGUID second;
        second.m_guid = 0;
        msg->Get(second.m_guid);
        this->m_skirmishQueue.m_data[i].unk_0008 = second;

        uint8_t flag = 0;
        msg->Get(flag);
        this->m_skirmishQueue.m_data[i].unk_0010 = flag;
    }
}

// VERIFY: afe682a1 auto
// OFFSET: 0x0056BB70
int32_t CGCommentator::OnSkirmishPlayersInQueue(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGCommentator::s_Commentator.SetPlayersInQueue(msg);
    FrameScript_SignalEvent(SCRIPT_EVENT_COMMENTATOR_SKIRMISH_QUEUE_REQUEST, nullptr);
    return 1;
}

// VERIFY: 8f556f09 manual
// OFFSET: 0x0056BF30
int32_t CGCommentator::OnReceiveMapInfo(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t count = 0;
    msg->Get(count);
    CGCommentator::s_Commentator.m_maps.SetCount(count);

    WOWGUID current;
    current.m_guid = 0;
    msg->Get(current.m_guid);

    bool found = false;
    uint32_t foundMapID = 0;
    CommentatorInstanceKey foundKey;

    for (uint32_t i = 0; i < CGCommentator::s_Commentator.m_maps.Count(); i++) {
        CGCommentator::MapInfo* map = i < CGCommentator::s_Commentator.m_maps.Count() ? &CGCommentator::s_Commentator.m_maps.m_data[i] : nullptr;

        uint32_t value = 0;
        msg->Get(value);
        map->unk_0000 = value;

        value = 0;
        msg->Get(value);
        map->unk_0004 = value;

        value = 0;
        msg->Get(value);
        map->unk_0008 = value;

        value = 0;
        msg->Get(value);
        map->m_instances.SetCount(value);

        for (uint32_t j = 0; j < map->m_instances.Count(); j++) {
            CGCommentator::InstanceInfo* instance = &map->m_instances.m_data[j];

            uint32_t mapID = 0;
            msg->Get(mapID);
            instance->m_mapID = mapID;
            instance->m_key.Read(msg);

            WOWGUID guid;
            guid.m_guid = 0;
            msg->Get(guid.m_guid);
            instance->m_guid = guid;

            value = 0;
            msg->Get(value);
            instance->unk_0018 = value;
            instance->m_numPlayers[0] = 0;
            instance->m_numPlayers[1] = 0;

            if (current && current == instance->m_guid) {
                foundMapID = instance->m_mapID;
                foundKey = instance->m_key;
                found = true;
            }
        }
    }

    if (found) {
        CGCommentator::s_Commentator.m_mapID = foundMapID;
        CGCommentator::s_Commentator.m_key = foundKey;
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_COMMENTATOR_MAP_UPDATE, nullptr);
    return 1;
}
