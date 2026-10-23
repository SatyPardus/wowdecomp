#include "GameObject_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include <cmath>
#include <cstring>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Base/Types.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Model2/M2Data.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C4Quaternion.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/DBClient/GameObjectStats_C.h"
#include "WoW/Source/DB/Rec/DestructibleModelDataRec.h"
#include "WoW/Source/DB/Rec/GameObjectDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/SpellEffectCameraShakesRec.h"
#include "WoW/Source/DB/Rec/SpellVisualEffectNameRec.h"
#include "WoW/Source/Game/GameClient/PlayerName.h"
#include "WoW/Source/Game/GameClient/WorldText.h"
#include "WoW/Source/LoadingScreen.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/GameObjectDef.h"
#include "WoW/Source/Object/GameObjectStats.h"
#include "WoW/Source/Object/MovementShared.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_AreaDamage.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_AuraGenerator.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_BarberChair.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_Binder.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_Button.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_Camera.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_CapturePoint.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_Chair.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_Chest.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_Door.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_DuelArbiter.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_DungeonDifficulty.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_FishingHole.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_FlagDrop.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_FlagStand.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_Generic.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_Goober.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_Mailbox.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_MapObj.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_MiniGame.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_QuestGiver.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_SpellCaster.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_SpellFocus.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_Text.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_Transport.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_Trap.h"
#include "WoW/Source/Object/ObjectClient/CGGameObject_C_Type_TrapDoor.h"
#include "WoW/Source/Object/ObjectClient/Effect_C.h"
#include "WoW/Source/Object/ObjectClient/Movement_C.h"
#include "WoW/Source/Object/ObjectClient/ObjectEffect.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/Object/Passenger.h"
#include "WoW/Source/ObjectMgrClient/CClientObjCreate.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/ItemTextFrame.h"
#include "WoW/Source/Ui/MinimapFrame.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/UIUtil/Cursor.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/WorldClient/CWorldMath.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapEntity.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

enum ANIMATED_OBJECT_STATE : int32_t {
    STATE_OPENING = 0,
    STATE_CLOSING = 1,
    STATE_DESTROYED = 2,
};

// STATIC: 0x00A335E8
static const uint32_t s_oneShotAnims[7] = { 0xFFFFFFFF, ANIMSTATE_SPAWN, ANIMSTATE_CUSTOM0, ANIMSTATE_CUSTOM1, ANIMSTATE_CUSTOM2, ANIMSTATE_CUSTOM3, ANIMSTATE_DESPAWN };

// STATIC: 0x00ADA974
static uint32_t s_buildingDamageColor = 0xFFFF8400;

// STATIC: 0x00ADA938
uint32_t CGGameObject_C_TypeAnimated::s_animSequenceIds[NUM_GAMEOBJ_ANIMSTATES] = { 145, 147, 148, 149, 146, 150, 151, 152, 153, 154, 155, 156, 157 };

// STATIC: 0x00ADA978
const char* CGGameObject_C_TypeAnimated::s_animStateNames[NUM_GAMEOBJ_ANIMSTATES] = { "Spawn", "Closed", "Opening", "Open", "Closing", "Destroying", "Destroyed", "Rebuilding", "Custom0", "Custom1", "Custom2", "Custom3", "Despawn" };

// VERIFY: 82a5bb1b manual
// OFFSET: 0x00712AE0
static void GameObjectStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    CGGameObject_C* object = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(guid, TYPE_GAMEOBJECT, __FILE__, __LINE__));
    WOWGUID requester = WOWGUID();
    GameObjectStats_C* stats = g_gameObjectDBCache.GetRecord(id, requester, nullptr, nullptr, false);

    if (stats) {
        object->LoadBaseObject(stats);
    }
}

// VERIFY: fb01868b manual
// OFFSET: 0x0070CAC0
void CGGameObject_C::AnimEventCallback(CM2Model* model, uint32_t boneId, uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time, uint64_t param) {
    CGGameObject_C* object = (CGGameObject_C*)ClntObjMgrObjectPtr(WOWGUID{ param }, TYPE_GAMEOBJECT, __FILE__, __LINE__);

    if (object) {
        object->m_typeData->HandleAnimEvent(eventId, data, *position, time);
    }
}

// VERIFY: bca2a2a9 manual
// OFFSET: 0x0070CB60
int32_t CGGameObject_C::OnAnimProgressChanged(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGGameObject_C* object = (CGGameObject_C*)ClntObjMgrObjectPtr(guid, TYPE_GAMEOBJECT, __FILE__, __LINE__);

    if (object->m_typeData) {
        object->m_typeData->OnUpdateAnimProgress();
    }

    return 1;
}

// VERIFY: 52bb7f99 manual
// OFFSET: 0x0070ED00
int32_t CGGameObject_C::OnFlagsChanged(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGGameObject_C* object = (CGGameObject_C*)ClntObjMgrObjectPtr(guid, TYPE_GAMEOBJECT, __FILE__, __LINE__);

    if (object->m_typeData) {
        object->m_typeData->OnUpdateFlags(*static_cast<const uint32_t*>(oldValue) ^ object->m_gameObject->m_flags);
    }

    return 1;
}

// VERIFY: cd8cb98e manual
// OFFSET: 0x00710BE0
void CGGameObject_C_Type_DestructibleBuilding::PostInitWithStats() {
    CGGameObject_C* owner = this->m_owner;
    this->m_states[0].m_displayId = owner->m_stats->m_displayID;
    GameObjectDisplayInfoRec* display = g_gameObjectDisplayInfoDB.GetRecord(this->m_states[0].m_displayId);

    if (display) {
        this->m_states[0].m_modelName = display->m_modelName;
    } else {
        SysMsgPrintf(SYSMSG_ERROR, 2, "Destructible building - Game object id %d is missing its display record %d", owner->m_obj->m_entryID, owner->m_gameObject->m_displayID);
    }

    this->m_modelData = g_destructibleModelDataDB.GetRecord(this->m_owner->GetPropertyValue(CGameObjectDef::GetPropNum(this->m_owner->m_gameObject->m_typeID, 124)));

    if (this->m_modelData) {
        this->m_states[1].m_displayId = this->m_modelData->m_state1WMO;
        this->m_states[2].m_displayId = this->m_modelData->m_state2WMO;
        this->m_states[3].m_displayId = this->m_modelData->m_state3WMO;

        for (int32_t i = 1; i < 4; i++) {
            DestructibleBuildingState& entry = this->m_states[i];

            if (entry.m_displayId) {
                GameObjectDisplayInfoRec* stateDisplay = g_gameObjectDisplayInfoDB.GetRecord(entry.m_displayId);

                if (stateDisplay) {
                    entry.m_modelName = stateDisplay->m_modelName;
                    continue;
                }

                SysMsgPrintf(SYSMSG_ERROR, 2, "Destructible building - Game object id %d is missing its display record %d for state %d", this->m_owner->m_obj->m_entryID, this->m_owner->m_gameObject->m_displayID, i);
            }

            entry.m_displayId = this->m_states[i - 1].m_displayId;
            entry.m_modelName = this->m_states[i - 1].m_modelName;
        }
    }

    C3Vector position = this->m_owner->m_passenger.GetPosition(this->m_owner->m_passenger.m_position);

    for (int32_t i = 0; i < 4; i++) {
        DestructibleBuildingState& entry = this->m_states[i];

        if (entry.m_object || !entry.m_modelName) {
            continue;
        }

        int16_t doodadSets[3];
        doodadSets[0] = this->GetDestructionDoodadSet(2 * i);
        doodadSets[1] = this->GetImpactEffectDoodadSet(2 * i);
        doodadSets[2] = this->GetAmbientDoodadSet(2 * i);
        uint32_t uniqueId = i ? 0 : this->m_proxy.m_uniqueId;
        uint64_t guid = this->m_owner->m_obj->m_guid.m_guid;
        entry.m_object = static_cast<CMapEntity*>(CWorld::ObjectCreate(entry.m_modelName, position, this->m_owner->GetFacing(), 0, 0, guid, 3, doodadSets, &this->m_proxy.m_sphere.r, uniqueId));

        if (entry.m_object) {
            World::ObjectSetRenderEnabled(entry.m_object, 0);
            World::ObjectEnableCollision(entry.m_object, 0);
        }

        entry.m_loading = 1;
    }

    this->m_worldObject = this->m_states[((this->m_owner->m_gameObject->m_flags >> 8) & 0xE) >> 1].m_object;

    if (this->m_worldObject) {
        World::ObjectSetRenderEnabled(this->m_worldObject, 1);
        World::ObjectEnableCollision(this->m_worldObject, 1);
    }

    this->m_loadPending = 1;

    if (this->m_modelData) {
        int32_t repairFx = this->m_modelData->m_repairGroundFx;

        if (repairFx) {
            GameObjectDisplayInfoRec* fxDisplay = g_gameObjectDisplayInfoDB.GetRecord(repairFx);

            if (fxDisplay) {
                this->m_rebuildEffect = static_cast<CMapEntity*>(CWorld::ObjectCreate(fxDisplay->m_modelName, position, this->m_owner->GetFacing(), 0, 0, 0, 0, nullptr, nullptr, 0));

                if (this->m_rebuildEffect) {
                    World::ObjectSetAlwaysExterior(this->m_rebuildEffect, 1);
                    World::ObjectSetRenderEnabled(this->m_rebuildEffect, 0);
                    World::ObjectEnableCollision(this->m_rebuildEffect, 0);
                }
            } else {
                SysMsgPrintf(SYSMSG_ERROR, 2, "Destructible building - Game object id %d is missing its rebuild FX display record %d", this->m_owner->m_obj->m_entryID, this->m_owner->m_gameObject->m_displayID);
            }
        }

        if (this->m_states[0].m_modelName) {
            uint64_t guid = this->m_owner->m_obj->m_guid.m_guid;
            this->m_baseCollision = static_cast<CMapEntity*>(CWorld::ObjectCreate(this->m_states[0].m_modelName, position, this->m_owner->GetFacing(), 0, 0, guid, 0, nullptr, nullptr, 0));

            if (this->m_baseCollision) {
                World::ObjectSetRenderEnabled(this->m_baseCollision, 0);
                World::ObjectEnableCollision(this->m_baseCollision, 0);
            }
        }
    }

    CGGameObject_C* object = this->m_owner;

    if (object->m_nameDesc) {
        PlayerNameDelete(object->m_nameDesc);
    }

    WOWGUID guid = object->m_obj->m_guid;
    object->m_nameDesc = ObjectNameCreate(guid);

    if (!this->m_proxy.m_uniqueId || !(this->m_owner->m_modelFlags & 0x10000)) {
        World::FreeDestructibleProxy(&this->m_proxy);
    }
}

// VERIFY: e11279e9 manual
// OFFSET: 0x00711050
int32_t CGGameObject_C::OnStateChanged(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGGameObject_C* object = (CGGameObject_C*)ClntObjMgrObjectPtr(guid, TYPE_GAMEOBJECT, __FILE__, __LINE__);
    int32_t state = object->m_gameObject->m_state;
    int32_t oldState = object->m_displayState;

    if (state != oldState || object->m_gameObject->m_typeID == 15) {
        object->m_displayState = state;

        if (object->m_typeData) {
            object->m_typeData->UpdateState(oldState, state);
        }
    }

    return 1;
}

// VERIFY: 39a16e63 manual
// OFFSET: 0x0070BE30
int32_t Packet_SMSG_GAMEOBJECT_PAGETEXT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_GAMEOBJECT, __FILE__, __LINE__);

    if (object) {
        WOWGUID objectGuid = object->m_obj->m_guid;
        CGItemText::SetItem(objectGuid, 0);
    }

    return 1;
}

// VERIFY: 77bd799e manual
// OFFSET: 0x00711210
void CGGameObject_C::CheckForPassiveHighlight() {
    CGPlayer_C* player;

    if ((this->m_gameObject->m_dynamicFlags & 0x8) || ((player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__))) != nullptr && player->CanTrackObject(this))) {
        this->m_modelFlags |= 0x400000;
        this->ShowLootEffect();
    } else {
        this->m_modelFlags &= ~0x400000;

        if (this->m_lootEffect) {
            if (this->m_lootEffect->m_attachParent) {
                this->m_lootEffect->DetachFromParent();
            }

            this->m_lootEffect->Release();
            this->m_lootEffect = nullptr;
        }
    }
}

// VERIFY: 84ec4568 manual
// OFFSET: 0x00712B40
static int32_t OnUpdateDynamicFlags(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGGameObject_C* object = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(guid, TYPE_GAMEOBJECT, __FILE__, __LINE__));

    if ((static_cast<uint8_t>(object->m_gameObject->m_dynamicFlags) ^ *static_cast<const uint8_t*>(oldValue)) & 0x8) {
        object->CheckForPassiveHighlight();
    }

    return 1;
}

// VERIFY: de6b9de5 manual
// OFFSET: 0x00713DA0
void ApplyGameObjectArtKit(CM2Model* model, int32_t artKit) {
    GameObjectArtKitRec* rec = g_gameObjectArtKitDB.GetRecord(artKit);

    for (uint32_t i = 0; i < 4; i++) {
        model->DetachAllChildrenById(i);
    }

    if (rec) {
        CStatus status;
        CGxTexFlags flags(GxTex_LinearMipNearest, 1, 1, 0, 0, 0, 1, 0, 0, 0);

        for (uint32_t i = 0; i < 3; i++) {
            if (*rec->m_textureVariation[i]) {
                CTexture* texture = TextureCreate(rec->m_textureVariation[i], flags, &status, 0);

                if (texture) {
                    model->ReplaceTexture(i + M2TEX_MONSTER_1, texture);
                    HandleClose(reinterpret_cast<HOBJECT>(texture));
                }
            }
        }

        for (uint32_t i = 0; i < 4; i++) {
            if (*rec->m_attachModel[i]) {
                CM2Model* child = model->m_scene->CreateModel(rec->m_attachModel[i], 0);

                if (child) {
                    child->AttachToParent(model, i, nullptr, 0);
                    child->Release();
                }
            }
        }
    }
}

// VERIFY: 6bcc74fd manual
// OFFSET: 0x00713ED0
static int32_t OnUpdateArtKit(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGGameObject_C* object = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(guid, TYPE_GAMEOBJECT, __FILE__, __LINE__));

    if (object->IsObjectModelLoaded()) {
        ApplyGameObjectArtKit(object->m_worldModel, object->m_gameObject->m_artKit);
    }

    return 1;
}

// VERIFY: 4d2fc566 auto
// OFFSET: 0x0070D040
void CGGameObject_C::UpdateDisplay(uint32_t time) {
    this->m_typeData->UpdateDisplay(time);

    if (this->m_worldObject) {
        World::TickObject(this->m_worldObject);
    }
}

// VERIFY: 948c52a8 auto
// OFFSET: 0x007140A0
void CGGameObject_C::Initialize() {
    ClientServices::SetMessageHandler(SMSG_PAGE_TEXT, Packet_SMSG_GAMEOBJECT_PAGETEXT, nullptr);
    ClientServices::SetMessageHandler(SMSG_GAME_OBJECT_CUSTOM_ANIM, Packet_SMSG_GAMEOBJECT_CUSTOM_ANIM, nullptr);
    ClientServices::SetMessageHandler(SMSG_GAMEOBJECT_DESPAWN_ANIM, Packet_SMSG_GAMEOBJECT_DESPAWN_ANIM, nullptr);
    ClntObjMgrSetTypeMirrorHandler(ID_GAMEOBJECT, offsetof(CGGameObjectData, m_state), sizeof(int8_t), CGGameObject_C::OnStateChanged, nullptr, HANDLER_PRIORITY_NORMAL, 1);
    ClntObjMgrSetTypeMirrorHandler(ID_GAMEOBJECT, offsetof(CGGameObjectData, m_flags), sizeof(uint32_t), CGGameObject_C::OnFlagsChanged, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_GAMEOBJECT, offsetof(CGGameObjectData, m_artKit), sizeof(int8_t), OnUpdateArtKit, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_GAMEOBJECT, offsetof(CGGameObjectData, m_pathProgress), sizeof(uint16_t), CGGameObject_C::OnAnimProgressChanged, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_GAMEOBJECT, offsetof(CGGameObjectData, m_dynamicFlags), sizeof(uint16_t), OnUpdateDynamicFlags, nullptr, HANDLER_PRIORITY_NORMAL, 0);
}

// VERIFY: 0add9943 auto
// OFFSET: 0x00714150
void CGGameObject_C::Shutdown() {
    ClientServices::ClearMessageHandler(SMSG_PAGE_TEXT);
    ClientServices::ClearMessageHandler(SMSG_GAME_OBJECT_CUSTOM_ANIM);
    ClientServices::ClearMessageHandler(SMSG_GAMEOBJECT_DESPAWN_ANIM);
    ClntObjMgrUnsetTypeMirrorHandler(ID_GAMEOBJECT, offsetof(CGGameObjectData, m_state), CGGameObject_C::OnStateChanged, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_GAMEOBJECT, offsetof(CGGameObjectData, m_flags), CGGameObject_C::OnFlagsChanged, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_GAMEOBJECT, offsetof(CGGameObjectData, m_artKit), OnUpdateArtKit, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_GAMEOBJECT, offsetof(CGGameObjectData, m_pathProgress), CGGameObject_C::OnAnimProgressChanged, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_GAMEOBJECT, offsetof(CGGameObjectData, m_dynamicFlags), OnUpdateDynamicFlags, nullptr);
}

// VERIFY: e9485d9b manual
// OFFSET: 0x0070CB10
void CGGameObject_C::SequenceCallback(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    CGGameObject_C* object = (CGGameObject_C*)ClntObjMgrObjectPtr(WOWGUID{ param }, TYPE_GAMEOBJECT, __FILE__, __LINE__);

    if (!object) {
        return;
    }

    if (interrupted) {
        object->m_typeData->HandleAnimInterrupted();
    } else {
        object->m_typeData->HandleAnimFinished();
    }
}

// VERIFY: 6d6cb557 manual
// OFFSET: 0x0070BA40
void CGGameObject_C::AddToolTipInfo(CGTooltip* tooltip) {
    if (this->m_typeData) {
        this->m_typeData->AddToolTipInfo(tooltip);
    }
}

// VERIFY: 5a089c89 auto
// OFFSET: 0x00712EB0
int32_t CGGameObject_C::AddPassenger(CPassenger* passenger, int32_t a2) {
    this->m_typeData->AddPassenger(passenger, a2);
    return 1;
}

// VERIFY: d2a54236 auto
// OFFSET: 0x0070B930
int32_t CGGameObject_C::Animate(float elapsed) {
    return this->m_typeData->Animate(elapsed) != 0;
}

// VERIFY: 297d76f4 auto
// OFFSET: 0x0070BA00
bool CGGameObject_C::CanUse() const {
    return this->m_typeData->CanUse();
}

// VERIFY: 6c3c1365 auto
// OFFSET: 0x0070BA10
int32_t CGGameObject_C::CanUseNow() const {
    return this->m_typeData->CanUseNow(nullptr, nullptr, nullptr);
}

// VERIFY: fc3ac502 manual
// OFFSET: 0x0070BA30
int32_t CGGameObject_C::CheckRange() const {
    return this->m_typeData->CheckRange();
}

// VERIFY: e2291194 manual
// OFFSET: 0x0070BAC0
void CGGameObject_C::CloseInteraction() {
    this->m_typeData->CloseInteraction();
}

// VERIFY: c0263fe5 auto
// OFFSET: 0x0070B9A0
float CGGameObject_C::GetBaseAlpha() const {
    return this->m_typeData->GetBaseAlpha();
}

// VERIFY: f4df747d auto
// OFFSET: 0x0070BA90
const char* CGGameObject_C::GetCloseText() const {
    if (this->m_stats) {
        return this->m_stats->m_closeText;
    }

    return nullptr;
}

// VERIFY: a6a6a215 auto
// OFFSET: 0x0070CEC0
void CGGameObject_C::GetCursorFile(char* buffer, uint32_t size) const {
    if (!this->m_stats) {
        return;
    }

    if (this->m_typeData->CanUseNow(nullptr, nullptr, nullptr)) {
        SStrPrintf(buffer, size, "Interface\\Cursor\\%s.blp", this->m_stats->m_iconName);
    } else {
        SStrPrintf(buffer, size, "Interface\\Cursor\\Unable%s.blp", this->m_stats->m_iconName);
    }
}

// VERIFY: ee92213d manual
// OFFSET: 0x0070BAD0
float CGGameObject_C::GetInteractDistanceSquared() const {
    if (!this->m_typeData) {
        return 0.0f;
    }

    CGGameObject_C_TypeBase* type = this->m_typeData;
    float distance = type->GetInteractDistance();
    return type->GetInteractDistance() * distance;
}

// VERIFY: e456868d manual
// OFFSET: 0x00712EE0
C44Matrix CGGameObject_C::GetMatrix() const {
    return this->m_matrix;
}

// VERIFY: 728f32b7 manual
// OFFSET: 0x0070EE30
const char* CGGameObject_C::GetModelFileNameInternal() const {
    int32_t displayID = this->m_gameObject->m_displayID;
    GameObjectDisplayInfoRec* rec = g_gameObjectDisplayInfoDB.GetRecord(displayID);

    if (rec) {
        return rec->m_modelName;
    }

    SysMsgPrintf(SYSMSG_ERROR, 2, "Game object id %d is missing its display record %d", this->m_obj->m_entryID, displayID);
    return nullptr;
}

// VERIFY: 29c8c641 auto
// OFFSET: 0x0070EE80
int32_t CGGameObject_C::GetModelFileName(const char** name) const {
    switch (this->m_gameObject->m_typeID) {
        case TYPE_TRANSPORT:
        case TYPE_MAP_OBJECT:
        case TYPE_MO_TRANSPORT:
        case TYPE_DESTRUCTIBLE_BUILDING:
        case TYPE_TRAPDOOR: {
            return 0;
        }

        default: {
            const char* file = this->GetModelFileNameInternal();
            *name = file;
            return file != nullptr;
        }
    }
}

// VERIFY: c19341d5 auto
// OFFSET: 0x0070CDF0
const char* CGGameObject_C::GetObjectName() const {
    if (this->m_stats) {
        return this->m_stats->m_name[0];
    }

    return "";
}

// VERIFY: 69157b14 auto
// OFFSET: 0x0070BA70
const char* CGGameObject_C::GetOpenText() const {
    if (this->m_stats) {
        return this->m_stats->m_castBarCaption;
    }

    return nullptr;
}

// VERIFY: 15b3b472 auto
// OFFSET: 0x0070B9B0
uint32_t CGGameObject_C::GetPropertyValue(uint32_t index) const {
    if (this->m_stats && index < 24) {
        return this->m_stats->m_propValue[index];
    }

    return 0;
}

// VERIFY: 44b076db auto
// OFFSET: 0x00712D50
C3Vector CGGameObject_C::GetRawPosition() const {
    C3Vector position = this->m_typeData ? this->m_typeData->GetRawPosition() : this->m_passenger.m_position;
    return position;
}

// VERIFY: 9ad4d30c manual
// OFFSET: 0x00712E40
ShipPath* CGGameObject_C::GetShipPath() const {
    if (this->m_typeData) {
        return this->m_typeData->GetShipPath();
    }

    return nullptr;
}

// VERIFY: f6bd5400 manual
// OFFSET: 0x00712F00
float CGGameObject_C::GetSpeed() const {
    if (this->m_typeData) {
        return this->m_typeData->GetSpeed();
    }

    return -1.0f;
}

// VERIFY: e90da76c auto
// OFFSET: 0x00706560
uint64_t CGGameObject_C::GetTransportGUID() const {
    return this->m_passenger.m_transportGuid.m_guid;
}

// VERIFY: 551dd805 manual
// OFFSET: 0x00712E80
int32_t CGGameObject_C::IsHighlightSuppressed() const {
    return this->m_typeData->IsHighlightSuppressed();
}

// VERIFY: 4bfff0e2 manual
// OFFSET: 0x00712E90
int32_t CGGameObject_C::IsPointInside(const C3Vector& point) const {
    return this->m_typeData->IsPointInside(point);
}

// VERIFY: de769fdd auto
// OFFSET: 0x0070F7B0
int32_t CGGameObject_C::IsQuestGiver() const {
    return this->m_gameObject->m_typeID == 2;
}

// VERIFY: 142300a5 auto
// OFFSET: 0x00712E60
int32_t CGGameObject_C::IsSolidSelectable() const {
    if (this->m_hasWorldBounds) {
        return 1;
    }

    if (this->CanHighlight()) {
        return 1;
    }

    return 0;
}

// VERIFY: 1492bbb3 auto
// OFFSET: 0x00712F20
bool CGGameObject_C::IsTransport() const {
    return (this->m_gameObject->m_flags >> 3) & 1;
}

// VERIFY: da4967fc manual
// OFFSET: 0x0070EF90
bool CGGameObject_C::IsValidOpenAction(int32_t action) const {
    if (action == 4) {
        return this->m_displayState != 2;
    }

    int32_t state = this->m_displayState;

    if (state == 2) {
        return false;
    }

    if ((action == 0 || action == 1 || action == 3) && state != 1) {
        return false;
    }

    if (action == 0) {
        return !(this->m_gameObject->m_flags & 2);
    }

    if (action == 1) {
        if (!(this->m_gameObject->m_flags & 2)) {
            return false;
        }
    } else if (action == 2 && state) {
        return false;
    }

    return true;
}

// VERIFY: d9a403af manual
// OFFSET: 0x0070BDE0
void CGGameObject_C::PlayCustomAnim(uint32_t anim) {
    uint32_t pending = anim + 2;

    if (this->IsObjectModelLoaded()) {
        this->m_typeData->ActivateOneShotAnim(s_oneShotAnims[pending]);
        this->m_pendingOneShotAnim = 0;
    } else {
        this->m_pendingOneShotAnim = pending;
    }
}

// VERIFY: 5ceaabac auto
// OFFSET: 0x0070F810
void CGGameObject_C::ResetDisplayState() {
    int32_t state = this->m_gameObject->m_state;
    int32_t oldState = this->m_displayState;

    if (state != oldState || this->m_gameObject->m_typeID == 15) {
        this->m_displayState = state;

        if (this->m_typeData) {
            this->m_typeData->UpdateState(oldState, state);
        }
    }
}

// VERIFY: fdc6ecb0 auto
// OFFSET: 0x0070F7D0
void CGGameObject_C::SetDisplayState(int32_t state) {
    int32_t oldState = this->m_displayState;

    if (state != oldState || this->m_gameObject->m_typeID == 15) {
        this->m_displayState = state;

        if (this->m_typeData) {
            this->m_typeData->UpdateState(oldState, state);
        }
    }
}

// VERIFY: eb7fb820 auto
// OFFSET: 0x0070D010
void CGGameObject_C::SetPathProgress(float progress) {
    this->m_gameObject->m_pathProgress = (int32_t)(float)(progress * 65535.0f);
}

// VERIFY: 562579c7 auto
// OFFSET: 0x0070B9E0
int32_t CGGameObject_C::ShouldFadeIn() const {
    return this->m_typeData->ShouldFadeIn();
}

// VERIFY: 9a0a90f2 manual
// OFFSET: 0x0070B960
void CGGameObject_C::ShouldRender(uint32_t flags, int32_t& render, int32_t& hidden) {
    CGObject_C::ShouldRender(flags, render, hidden);
    this->m_typeData->ShouldRender(flags, render, hidden);
}

// VERIFY: 981f9d46 manual
// OFFSET: 0x0070BAB0
void CGGameObject_C::StartInteraction() {
    this->m_typeData->StartInteraction();
}

// VERIFY: ac212d62 auto
// OFFSET: 0x0070F550
int32_t CGGameObject_C::IsCollidable(int32_t flags) const {
    if ((flags & 0x8000) && !this->m_gameObject->m_typeID) {
        return 0;
    }

    return this->m_hasWorldBounds;
}

// VERIFY: 08d3c981 auto
// OFFSET: 0x0070CF30
void CGGameObject_C::UpdateInteractIconAttach() {
    if (!this->m_interactIcon) {
        return;
    }

    CM2Model* worldModel = this->m_worldModel;

    if (!worldModel || !worldModel->IsLoaded(0, 0)) {
        return;
    }

    if (this->m_interactIcon->m_attachParent) {
        this->m_interactIcon->DetachFromParent();
    }

    if (!this->m_worldModel->HasAttachment(18)) {
        return;
    }

    this->m_interactIcon->AttachToParent(this->m_worldModel, 18, nullptr, 0);
    C44Matrix transform = this->m_worldModel->GetAttachmentWorldTransform(18);

    if (transform.c1 < 0.99000001f) {
        C44Matrix inverse = transform.AffineInverse();
        inverse.d0 = 0.0f;
        inverse.d1 = 0.0f;
        inverse.d2 = 0.0f;
        this->m_interactIcon->SetWorldTransform(inverse);
    }

    this->UpdateInteractIconScale();
    this->ObjectNameVisibilityChanged();
}

// INLINED
CGGameObject_C_TypeBase::CGGameObject_C_TypeBase(CGGameObject_C* owner) {
    this->m_owner = owner;
    this->m_useErrorType = ERR_USE_LOCKED;
    this->m_interactDistance = MAX_LOOT_DISTANCE;
}

// INLINED
CGGameObject_C_TypeBase::~CGGameObject_C_TypeBase() {
}

// VERIFY: e4285ce5 auto
// OFFSET: 0x0070BDD0
float CGGameObject_C_TypeBase::GetInteractDistance() const {
    return this->m_interactDistance;
}

// VERIFY: 5624b8db manual
// OFFSET: 0x0070C2E0
C3Vector CGGameObject_C_TypeBase::GetRawPosition() const {
    return this->m_owner->m_passenger.m_position;
}

// INLINED
CGGameObject_C_Type_Null::CGGameObject_C_Type_Null(CGGameObject_C* owner) : CGGameObject_C_TypeBase(owner) {
}

// VERIFY: cb78c258 auto
// OFFSET: 0x0070C340
const char* CGGameObject_C_Type_Null::DebugStatus() {
    return "Unknown Object Type";
}

// VERIFY: f43bc69d manual
// OFFSET: 0x007124B0
CGGameObject_C_TypeAnimated::CGGameObject_C_TypeAnimated(CGGameObject_C* owner) : CGGameObject_C_TypeBase(owner), m_animState(-1), m_pendingAnimState(-1), m_nextAnimTime(-1) {
    this->ResetAnimState();
}

// VERIFY: b69c4cb1 manual
// OFFSET: 0x00713D10
CGGameObject_C_TypeAnimated::~CGGameObject_C_TypeAnimated() {
    SI2::StopSound(&this->m_sound, 0, -1.0f, 1);
}

// VERIFY: 50f95140 auto
// OFFSET: 0x0070C480
void CGGameObject_C_TypeAnimated::ActivateOneShotAnim(uint32_t anim) {
    if (!this->m_owner->m_worldModel->HasSequence(s_animSequenceIds[anim])) {
        return;
    }

    this->UpdateAnimState(anim);

    if (!anim) {
        this->m_owner->DoFade(this->m_owner->GetBaseAlpha(), 0);
    }
}

// VERIFY: 893eef11 auto
// OFFSET: 0x0070B260
const char* CGGameObject_C_TypeAnimated::DebugStatus() {
    return s_animStateNames[this->m_animState];
}

// VERIFY: bf8c5652 auto
// OFFSET: 0x0070C350
void CGGameObject_C_TypeAnimated::Disable() {
    SI2::StopSound(&this->m_sound, 0, -1.0f, 1);
}

// VERIFY: 176bb2cb auto
// OFFSET: 0x0070C3C0
int32_t CGGameObject_C_TypeAnimated::Animate(float elapsed) {
    CGGameObject_C* owner = this->m_owner;

    if (owner->m_modelFlags & 0x100000) {
        CM2Model* model = owner->m_worldModel;

        if (model && model->IsLoaded(0, 0) && !model->m_animatePrev) {
            if (model->m_lastAnimTime || (this->m_nextAnimTime != -1 && (int32_t)(FrameTime::s_curTimeMs - this->m_nextAnimTime) >= 0)) {
                return this->HandleAnimInterrupted();
            }
        }
    }

    return 1;
}

// VERIFY: 0af705ec auto
// OFFSET: 0x0070D160
void CGGameObject_C_TypeAnimated::OnUpdateFlags(uint32_t changed) {
    if (!(changed & 0x80)) {
        return;
    }

    CM2Model* model = this->m_owner->m_worldModel;

    if (!model || !model->IsLoaded(0, 0)) {
        return;
    }

    int32_t state = this->m_animState;

    if (state != ANIMSTATE_OPENING && state != ANIMSTATE_CLOSING && state != ANIMSTATE_DESTROYING && state != ANIMSTATE_REBUILDING) {
        return;
    }

    if ((this->m_owner->m_gameObject->m_flags >> 7) & 1) {
        uint32_t time = model->m_scene->m_time;

        if (!time) {
            time = 1;
        }

        model->m_lastAnimTime = time;
    } else {
        model->m_lastAnimTime = 0;
    }
}

// VERIFY: 366e7076 manual
// OFFSET: 0x0070C370
void CGGameObject_C_TypeAnimated::PostInit(bool a1) {
    if (!a1) {
        return;
    }

    CGGameObject_C* owner = this->m_owner;

    if (owner->IsObjectModelLoaded()) {
        owner->m_typeData->ActivateOneShotAnim(ANIMSTATE_SPAWN);
        owner->m_pendingOneShotAnim = 0;
    } else {
        owner->m_pendingOneShotAnim = 1;
    }
}

// VERIFY: 0e70b882 auto
// OFFSET: 0x0070FCE0
void CGGameObject_C_TypeAnimated::PostReenable() {
    CM2Model* model = this->m_owner->m_worldModel;

    if (model && model->IsLoaded(0, 0)) {
        this->ResetAnimState();
    }
}

// VERIFY: 21d999bd auto
// OFFSET: 0x0070D600
void CGGameObject_C_TypeAnimated::ResetAnimState() {
    CGGameObject_C* owner = this->m_owner;
    int32_t state = -1;

    switch (owner->m_displayState) {
        case 0: {
            state = (owner->m_gameObject->m_pathProgress == 0xFFFF) + ANIMSTATE_OPENING;
            break;
        }

        case 1: {
            state = owner->m_gameObject->m_pathProgress != 0xFFFF ? ANIMSTATE_CLOSING : ANIMSTATE_CLOSED;
            break;
        }

        case 2: {
            state = (owner->m_gameObject->m_pathProgress == 0xFFFF) + ANIMSTATE_DESTROYING;
            break;
        }
    }

    this->m_animState = -1;
    this->m_pendingAnimState = -1;
    this->m_nextAnimTime = -1;
    this->UpdateAnimState(state);
}

// VERIFY: 0978367d manual
// OFFSET: 0x00712510
bool CGGameObject_C_TypeAnimated::ShouldFadeIn() const {
    return this->m_animState != ANIMSTATE_SPAWN;
}

// VERIFY: 1c670d12 auto
// OFFSET: 0x0070B230
void CGGameObject_C_TypeAnimated::UpdateDisplay(uint32_t time) {
    C3Vector position = this->GetPosition();
    SI2::Set3DPosition(&this->m_sound, &position);
}

// VERIFY: 2da8644c auto
// OFFSET: 0x0070D690
void CGGameObject_C_TypeAnimated::UpdateState(int32_t oldState, int32_t newState) {
    if (this->m_owner->m_modelFlags & 0x20000) {
        return;
    }

    SI2::StopSound(&this->m_sound, 0, -1.0f, 1);

    switch (newState) {
        case STATE_OPENING: {
            if (oldState == STATE_CLOSING || this->m_owner->m_gameObject->m_pathProgress != 0xFFFF) {
                this->UpdateAnimState(ANIMSTATE_OPENING);
            } else {
                this->UpdateAnimState(ANIMSTATE_OPEN);
            }

            break;
        }

        case STATE_CLOSING: {
            int32_t state = this->m_owner->m_gameObject->m_pathProgress == 0xFFFF ? oldState : STATE_OPENING;

            switch (state) {
                case STATE_OPENING: {
                    this->UpdateAnimState(ANIMSTATE_CLOSING);
                    break;
                }

                case STATE_DESTROYED: {
                    this->UpdateAnimState(ANIMSTATE_REBUILDING);
                    break;
                }

                default: {
                    this->UpdateAnimState(ANIMSTATE_CLOSED);
                    break;
                }
            }

            break;
        }

        case STATE_DESTROYED: {
            if (oldState == STATE_CLOSING || this->m_owner->m_gameObject->m_pathProgress != 0xFFFF) {
                this->UpdateAnimState(ANIMSTATE_DESTROYING);
            } else {
                this->UpdateAnimState(ANIMSTATE_DESTROYED);
            }

            break;
        }
    }
}

// VERIFY: c1ad28de manual
// OFFSET: 0x00712030
bool CGGameObject_C_Type_FishingNode::CanUse() const {
    CGObject_C* player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__);

    if (player && *(const WOWGUID*)&((CGUnit_C*)player)->m_unit->m_unitFieldChannelObject == this->m_owner->m_obj->m_guid) {
        return CGGameObject_C_TypeBase::CanUse();
    }

    return false;
}

// VERIFY: f61c006c auto
// OFFSET: 0x0070B910
int32_t CGGameObject_C_Type_GuildBank::CanChangeCursor() const {
    return this->CanUseNow(nullptr, nullptr, nullptr) ? BUY_CURSOR : BUY_ERROR_CURSOR;
}

// VERIFY: 1e1b3af8 auto
// OFFSET: 0x0070C540
void CGGameObject_C_Type_TransportBase::Reenable() {
    this->m_currentState = -1;
    this->m_worldObjectReady = 0;
}

// VERIFY: ed8aebb4 auto
// OFFSET: 0x007126A0
C3Vector CGGameObject_C_Type_TransportBase::GetRawPosition() const {
    return this->m_position;
}

// VERIFY: 3677cf38 auto
// OFFSET: 0x007126C0
float CGGameObject_C_Type_TransportBase::GetSpeed() const {
    return this->m_speed;
}

// VERIFY: 83fbb5b1 auto
// OFFSET: 0x00959D00
uint32_t CGGameObject_C_Type_TransportBase::GetTransportUpdateTime() const {
    return this->m_transportUpdateTime;
}

// VERIFY: db902dce auto
// OFFSET: 0x00711CD0
void CGGameObject_C_Type_MapObjTransport::AddPassenger(CPassenger* passenger, int32_t a2) {
    CGGameObject_C_Type_TransportBase::AddPassenger(passenger, a2);
}

// VERIFY: a94a9743 manual
// OFFSET: 0x007141D0
CGGameObject_C_Type_MapObjTransport::CGGameObject_C_Type_MapObjTransport(CGGameObject_C* owner) : CGGameObject_C_Type_TransportBase(owner), m_currentLeg(0) {
}

// VERIFY: d75d2da1 auto
// OFFSET: 0x00714230
ShipPath* CGGameObject_C_Type_MapObjTransport::GetShipPath() const {
    return (ShipPath*)&this->m_shipPath;
}

// INLINED
CGGameObject_C_Type_WorldObj::CGGameObject_C_Type_WorldObj(CGGameObject_C* owner) : CGGameObject_C_TypeBase(owner) {
    this->m_worldObject = nullptr;
}

// VERIFY: 1c0f2dcf auto
// OFFSET: 0x0070B290
void CGGameObject_C_Type_WorldObj::DestroyWorldObj() {
    if (this->m_worldObject) {
        CWorld::ObjectDestroy(this->m_worldObject);
        this->m_worldObject = nullptr;
    }
}

// VERIFY: fb2d9d67 auto
// OFFSET: 0x0070B2B0
void CGGameObject_C_Type_WorldObj::SequenceFinishedHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    if (interrupted) {
        return;
    }

    uint32_t next;

    switch (sequenceId) {
        case 146: {
            next = 147;
            break;
        }

        case 148: {
            next = 149;
            break;
        }

        case 162: {
            next = 163;
            break;
        }

        case 164: {
            next = 0;
            break;
        }

        default: {
            return;
        }
    }

    model->SetBoneSequence(0xFFFFFFFF, next, 0xFFFFFFFF, 0, 1.0f, 1, 1);
}

// VERIFY: 5f0fe2c4 manual
// OFFSET: 0x007128D0
CGGameObject_C_Type_DestructibleBuilding::CGGameObject_C_Type_DestructibleBuilding(CGGameObject_C* owner) : CGGameObject_C_Type_WorldObj(owner) {
    memset(this->m_states, 0, sizeof(this->m_states));
    this->m_baseZ = 0.0f;
    this->m_sinkDepth = 0.0f;
    this->m_animStartTime = 0;
    this->m_animDuration = 0;
    this->m_previousSinkDepth = 0.0f;
    this->m_animating = 0;
    this->m_rebuildEffect = nullptr;
    this->m_baseCollision = nullptr;
    this->m_modelData = nullptr;
    this->m_loadPending = 1;
    this->m_previousState = -1;
}

// VERIFY: 3e8f3f39 auto
// OFFSET: 0x0070B870
void CGGameObject_C_Type_DestructibleBuilding::AmbientSequenceFinishedHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    if (!interrupted && sequenceId != 154) {
        model->SetBoneSequence(0xFFFFFFFF, 154, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }
}

// VERIFY: 73f5d046 manual
// OFFSET: 0x0070CA40
void CGGameObject_C_Type_DestructibleBuilding::AnimEventCallback(CM2Model* model, uint32_t boneId, uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time, uint64_t param) {
    CGGameObject_C* object = (CGGameObject_C*)ClntObjMgrObjectPtr(WOWGUID{ param }, TYPE_GAMEOBJECT, __FILE__, __LINE__);

    if (object) {
        object->m_typeData->HandleAnimEvent(eventId, data, *position, time);
    }
}

// VERIFY: 2e2a861c auto
// OFFSET: 0x0070B6A0
void CGGameObject_C_Type_DestructibleBuilding::PostReenable() {
    if (!this->m_worldObject) {
        return;
    }

    World::ObjectSetRenderEnabled(this->m_worldObject, 1);
    World::ObjectEnableCollision(this->m_worldObject, 1);

    if (this->m_proxy.m_uniqueId) {
        World::FreeDestructibleProxy(&this->m_proxy);
    }
}

// VERIFY: d518c5b7 auto
// OFFSET: 0x0070B6E0
void CGGameObject_C_Type_DestructibleBuilding::Disable() {
    if (!this->m_worldObject) {
        return;
    }

    World::ObjectSetRenderEnabled(this->m_worldObject, 0);
    World::ObjectEnableCollision(this->m_worldObject, 0);

    if (this->m_proxy.m_uniqueId) {
        World::FindOrCreateDestructibleProxy(&this->m_proxy);
    }
}

// VERIFY: 7ce2b97a auto
// OFFSET: 0x0070B720
uint16_t CGGameObject_C_Type_DestructibleBuilding::GetImpactEffectDoodadSet(int32_t state) const {
    if (this->m_modelData) {
        switch (state >> 1) {
            case 0:
                return this->m_modelData->m_state0ImpactEffectDoodadSet;

            case 1:
                return this->m_modelData->m_state1ImpactEffectDoodadSet;

            case 2:
                return this->m_modelData->m_state2ImpactEffectDoodadSet;
        }
    }

    return 0;
}

// VERIFY: b79cc54a auto
// OFFSET: 0x0070B760
uint16_t CGGameObject_C_Type_DestructibleBuilding::GetDestructionDoodadSet(int32_t state) const {
    if (this->m_modelData) {
        switch (state >> 1) {
            case 1:
                return this->m_modelData->m_state1DestructionDoodadSet;

            case 2:
                return this->m_modelData->m_state2DestructionDoodadSet;

            case 3:
                return this->m_modelData->m_state3InitDoodadSet;
        }
    }

    return 0;
}

// VERIFY: 8496e81e auto
// OFFSET: 0x0070B7A0
uint16_t CGGameObject_C_Type_DestructibleBuilding::GetAmbientDoodadSet(int32_t state) const {
    if (!this->m_modelData) {
        return 0;
    }

    switch (state >> 1) {
        case 0:
            return this->m_modelData->m_state0AmbientDoodadSet;

        case 1:
            return this->m_modelData->m_state1AmbientDoodadSet;

        case 2:
            return this->m_modelData->m_state2AmbientDoodadSet;

        case 3:
            return this->m_modelData->m_state3AmbientDoodadSet;

        default:
            return 0;
    }
}

// VERIFY: ed94b365 auto
// OFFSET: 0x0070B800
void CGGameObject_C_Type_DestructibleBuilding::DestructSequenceFinishedHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    if (!interrupted && sequenceId) {
        model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }
}

// VERIFY: 850a3d5b auto
// OFFSET: 0x0070B830
void CGGameObject_C_Type_DestructibleBuilding::ImpactEffectSequenceFinishedHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    if (interrupted) {
        return;
    }

    if (sequenceId == 153) {
        model->SetBoneSequence(0xFFFFFFFF, 154, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    } else {
        model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }
}

// VERIFY: d0d0873a auto
// OFFSET: 0x0070B8B0
void CGGameObject_C_Type_DestructibleBuilding::MovedDoodadSequenceHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    if (interrupted) {
        return;
    }

    if (sequenceId == 153 || sequenceId == 154) {
        model->SetBoneSequence(0xFFFFFFFF, 155, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    } else if (sequenceId == 155) {
        model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }
}

// VERIFY: fa4d5319 manual
// OFFSET: 0x0070CA90
int32_t CGGameObject_C_Type_DestructibleBuilding::IsHighlightSuppressed() const {
    if (this->m_modelData && this->m_modelData->m_doNotHighlight) {
        return 1;
    }

    CGGameObject_C* owner = this->m_owner;

    if (!owner || owner->m_displayState == 1) {
        return 0;
    }

    return 1;
}

// VERIFY: 6612f5da manual
// OFFSET: 0x0070C550
float CGGameObject_C_Type_TransportBase::UpdateCurrentVelocity(uint32_t elapsed, const C3Vector& position, C3Vector* direction) {
    float dx = position.x - this->m_position.x;
    float dy = position.y - this->m_position.y;
    float dz = position.z - this->m_position.z;
    float distance = sqrt(dx * dx + dy * dy + dz * dz);
    float speed = distance / ((float)elapsed * 0.001f);

    if (this->m_speed != speed && this->m_owner->m_effectManager) {
        this->m_owner->m_effectManager->PropertyChanged(1);
    }

    this->m_speed = speed;

    if (direction && !(fabs(distance) < 2.3841858e-7f)) {
        float scale = 1.0f / distance;
        direction->x = dx * scale;
        direction->y = dy * scale;
        direction->z = scale * dz;
    }

    return distance;
}

// VERIFY: 1b4606a4 manual
// OFFSET: 0x0070BEF0
int32_t Packet_SMSG_GAMEOBJECT_DESPAWN_ANIM(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);

    CGGameObject_C* object = (CGGameObject_C*)ClntObjMgrObjectPtr(guid, TYPE_GAMEOBJECT, __FILE__, __LINE__);

    if (object) {
        if (!object->IsObjectModelLoaded()) {
            object->m_pendingOneShotAnim = 6;
            return 1;
        }

        object->m_typeData->ActivateOneShotAnim(12);
        object->m_pendingOneShotAnim = 0;
    }

    return 1;
}

// VERIFY: 222a19b6 auto
// OFFSET: 0x0070B390
void CGGameObject_C_Type_MapObjTransport::UpdateObjectEffectState(uint32_t anim) {
    CObjectEffectManager* manager = this->m_owner->m_effectManager;

    if (!manager) {
        return;
    }

    switch (anim) {
        case 0: {
            manager->EnterState(2, 1, 0);
            this->m_owner->m_effectManager->ExitState(3, 1);
            this->m_owner->m_effectManager->ExitState(4, 1);
            this->m_owner->m_effectManager->ExitState(5, 1);
            break;
        }

        case 162: {
            manager->EnterState(3, 1, 0);
            this->m_owner->m_effectManager->ExitState(2, 1);
            this->m_owner->m_effectManager->ExitState(4, 1);
            this->m_owner->m_effectManager->ExitState(5, 1);
            break;
        }

        case 163: {
            manager->EnterState(4, 1, 0);
            this->m_owner->m_effectManager->ExitState(3, 1);
            this->m_owner->m_effectManager->ExitState(2, 1);
            this->m_owner->m_effectManager->ExitState(5, 1);
            break;
        }

        case 164: {
            manager->EnterState(5, 1, 0);
            this->m_owner->m_effectManager->ExitState(3, 1);
            this->m_owner->m_effectManager->ExitState(4, 1);
            this->m_owner->m_effectManager->ExitState(2, 1);
            break;
        }
    }
}

// VERIFY: c5cf4c85 auto
// OFFSET: 0x0070D080
void CGGameObject_C::ShowLootEffect() {
    if (this->m_lootEffect) {
        return;
    }

    if (!this->m_worldModel || !this->m_worldModel->IsLoaded(0, 0)) {
        return;
    }

    const SpellVisualEffectNameRec* rec = GetSpecialSpellVisualEffectNameRec(SPECIALEFFECT_LOOTART);
    this->m_lootEffect = this->m_worldModel->m_scene->CreateModel(rec->m_fileName, 0);

    if (!this->m_lootEffect) {
        return;
    }

    this->m_lootEffect->AttachToParent(this->m_worldModel, 19, nullptr, 1);

    float scale = fclamp(this->GetScale() * this->m_height * 1.15f, 1.0f, 5.0f);
    C44Matrix matrix;
    matrix.Scale(scale);

    CM2Model* effect = this->m_lootEffect;
    effect->m_fFlags |= 0x8000;
    effect->m_worldTransform = matrix;
}

// VERIFY: e08bcb80 auto
// OFFSET: 0x0070BB10
void CGGameObject_C::UpdateObjectEffectAnimationStates(uint32_t anim) {
    if (!this->m_effectManager) {
        return;
    }

    uint32_t state = FindObjectEffectStateFromAnimID(anim, 0);

    if (!state || state == this->m_effectAnimState) {
        return;
    }

    this->m_effectManager->ExitState(this->m_effectAnimState, 1);
    this->m_effectManager->EnterState(state, 1, 0);

    if (state == 15 || state == 81) {
        this->m_effectManager->ExitState(37, 1);
        this->m_effectManager->ExitState(38, 1);
        this->m_effectManager->ExitState(6, 1);
        this->m_effectManager->ExitState(7, 1);
        this->m_effectManager->ExitState(47, 1);
        this->m_effectManager->ExitState(48, 1);
        this->m_effectManager->ExitState(49, 1);
        this->m_effectManager->ExitState(50, 1);
        this->m_effectManager->ExitState(51, 1);
        this->m_effectManager->ExitState(52, 1);
        this->m_effectManager->ExitState(53, 1);
        this->m_effectManager->ExitState(54, 1);
        this->m_effectManager->ExitState(55, 1);
        this->m_effectManager->ExitState(56, 1);
        this->m_effectManager->ExitState(57, 1);
        this->m_effectManager->ExitState(58, 1);
        this->m_effectManager->ExitState(59, 1);
        this->m_effectManager->ExitState(60, 1);
        this->m_effectManager->ExitState(61, 1);
        this->m_effectManager->ExitState(62, 1);
        this->m_effectManager->ExitState(63, 1);
        this->m_effectManager->ExitState(64, 1);
        this->m_effectManager->ExitState(65, 1);
        this->m_effectManager->ExitState(66, 1);
    }

    this->m_effectAnimState = state;
}

// VERIFY: 8e612f17 auto
// OFFSET: 0x00712700
CGGameObject_C_Type_FishingNode::CGGameObject_C_Type_FishingNode(CGGameObject_C* owner) : CGGameObject_C_TypeAnimated(owner) {
    this->m_interactDistance = MAX_OBJ_INTEREST_RADIUS;
}

// VERIFY: 2b1b955c manual
// OFFSET: [0x00712980, 0x00713D70]
CGGameObject_C_Type_DestructibleBuilding::~CGGameObject_C_Type_DestructibleBuilding() {
    if (this->m_proxy.m_uniqueId) {
        World::FindOrCreateDestructibleProxy(&this->m_proxy);
    }

    for (uint32_t i = 0; i < 4; i++) {
        if (this->m_states[i].m_object) {
            CWorld::ObjectDestroy(this->m_states[i].m_object);
            this->m_states[i].m_object = nullptr;
        }
    }

    if (this->m_rebuildEffect) {
        CWorld::ObjectDestroy(this->m_rebuildEffect);
        this->m_rebuildEffect = nullptr;
    }

    this->m_worldObject = nullptr;

    if (this->m_baseCollision) {
        CWorld::ObjectDestroy(this->m_baseCollision);
        this->m_baseCollision = nullptr;
    }

    SI2::StopSound(&this->m_sound, 0, -1.0f, 1);
}

// VERIFY: b2424cfc auto
// OFFSET: 0x00712AC0
CGGameObject_C_Type_GuildBank::CGGameObject_C_Type_GuildBank(CGGameObject_C* owner) : CGGameObject_C_TypeAnimated(owner) {
}

// VERIFY: a72bb2d6 auto
// OFFSET: 0x00712790
CGGameObject_C_Type_MeetingStone::CGGameObject_C_Type_MeetingStone(CGGameObject_C* owner) : CGGameObject_C_TypeAnimated(owner) {
}

// VERIFY: 9c1d06e1 auto
// OFFSET: 0x00712730
CGGameObject_C_Type_Ritual::CGGameObject_C_Type_Ritual(CGGameObject_C* owner) : CGGameObject_C_TypeAnimated(owner) {
}

// VERIFY: 39f3e875 auto
// OFFSET: 0x007130A0
void CGGameObject_C::Disable() {
    CGObject_C::Disable();

    if (this->m_stats) {
        this->m_typeData->Disable();
    } else {
        g_gameObjectDBCache.CancelCallback(this->m_obj->m_entryID, &GameObjectStatsCallback, this);
    }

    this->m_passenger.m_link.Unlink();

    if (this->m_lootEffect) {
        if (this->m_lootEffect->m_attachParent) {
            this->m_lootEffect->DetachFromParent();
        }

        this->m_lootEffect->Release();
        this->m_lootEffect = nullptr;
    }

    if (this->m_nameDesc) {
        PlayerNameDelete(this->m_nameDesc);
    }

    this->m_nameDesc = nullptr;
}

// VERIFY: f0999c9b manual
// OFFSET: 0x00713250
CGGameObject_C_Type_TransportBase::CGGameObject_C_Type_TransportBase(CGGameObject_C* owner) : CGGameObject_C_Type_WorldObj(owner) {
    this->m_position = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_pathTime = 0;
    this->m_worldObjectReady = 0;
    this->m_speed = 0.0f;
    this->m_transportUpdateTime = 0;
    this->m_currentState = -1;
    this->m_position = owner->m_passenger.GetPosition(owner->m_passenger.m_position);
}

// VERIFY: f74bd918 auto
// OFFSET: 0x0070B330
void CGGameObject_C_Type_TransportBase::PostInitWithStats() {
    this->CreateWorldObj();

    if (this->m_worldObject) {
        MovementAddTransport(this->m_owner);
    }
}

// VERIFY: f102a3a9 auto
// OFFSET: 0x00710190
void CGGameObject_C_Type_MapObjTransport::PostReenable() {
    this->CreateWorldObj();

    if (this->m_worldObject) {
        MovementAddTransport(this->m_owner);
    }

    this->InitializeMovement();
}

// VERIFY: 6e818bac manual
// OFFSET: 0x0070C2B0
C3Vector CGGameObject_C_TypeBase::GetPosition() const {
    return this->m_owner->m_passenger.GetPosition(this->m_owner->m_passenger.m_position);
}

// VERIFY: 109c062c manual
// OFFSET: [0x00712B80, 0x00713F20]
CGGameObject_C::~CGGameObject_C() {
    if (!this->m_stats) {
        g_gameObjectDBCache.CancelCallback(this->m_obj->m_entryID, &GameObjectStatsCallback, this);
    }

    this->m_modelFlags &= ~0x400000;

    if (this->m_lootEffect) {
        if (this->m_lootEffect->m_attachParent) {
            this->m_lootEffect->DetachFromParent();
        }

        this->m_lootEffect->Release();
        this->m_lootEffect = nullptr;
    }

    if (this->m_nameDesc) {
        PlayerNameDelete(this->m_nameDesc);
    }

    this->m_nameDesc = nullptr;

    if (this->m_typeData) {
        delete this->m_typeData;
    }
}

// VERIFY: 9c20623b manual
// OFFSET: 0x00712CF0
C3Vector CGGameObject_C::GetPosition() const {
    if (this->m_typeData) {
        return this->m_typeData->GetPosition();
    }

    return this->m_passenger.GetPosition(this->m_passenger.m_position);
}

// VERIFY: ca51d521 manual
// OFFSET: 0x0070BE90
int32_t Packet_SMSG_GAMEOBJECT_CUSTOM_ANIM(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    uint32_t anim;
    msg->Get(guid.m_guid);
    msg->Get(anim);

    CGGameObject_C* object = (CGGameObject_C*)ClntObjMgrObjectPtr(guid, TYPE_GAMEOBJECT, __FILE__, __LINE__);

    if (object && anim < 4) {
        object->PlayCustomAnim(anim);
    }

    return 1;
}

// VERIFY: 0f64befc auto
// OFFSET: 0x008A1420
bool CGGameObject_C_TypeBase::CanBeSolid() const {
    return true;
}

// VERIFY: c3a2df23 auto
// OFFSET: 0x008A1420
bool CGGameObject_C_TypeBase::ShouldFadeIn() const {
    return true;
}

// VERIFY: 3f057594 auto
// OFFSET: 0x0070BDB0
bool CGGameObject_C_TypeBase::CanHighlight() const {
    return this->CanUse();
}

// VERIFY: 092326ba auto
// OFFSET: 0x008E5250
int32_t CGGameObject_C_TypeBase::IsHighlightSuppressed() const {
    return 0;
}

// VERIFY: 8ee24f6a auto
// OFFSET: 0x00632050
void CGGameObject_C_TypeBase::OnUpdateFlags(uint32_t changed) {
}

// VERIFY: 296b109d auto
// OFFSET: 0x005EEB70
void CGGameObject_C_TypeBase::OnUpdateAnimProgress() {
}

// VERIFY: 6b1c2c32 auto
// OFFSET: 0x00653A10
void CGGameObject_C_TypeBase::UpdateState(int32_t oldState, int32_t newState) {
}

// VERIFY: 6dd7e229 auto
// OFFSET: 0x00959DE0
int32_t CGGameObject_C_TypeBase::Animate(float elapsed) {
    return 1;
}

// VERIFY: 13be4357 auto
// OFFSET: 0x0047ADD0
void CGGameObject_C_TypeBase::ShouldRender(uint32_t flags, int32_t& render, int32_t& hidden) {
}

// VERIFY: 849dbeae auto
// OFFSET: 0x004D5FF0
float CGGameObject_C_TypeBase::GetBaseAlpha() const {
    return 1.0f;
}

// VERIFY: c27bdd8e auto
// OFFSET: 0x006B1B80
void CGGameObject_C_TypeBase::HandleAnimEvent(uint32_t eventId, uint32_t data, const C3Vector& position, uint32_t time) {
}

// VERIFY: 9e8df6c3 auto
// OFFSET: 0x008C8DE0
int32_t CGGameObject_C_TypeBase::HandleAnimInterrupted() {
    return 1;
}

// VERIFY: d522ab25 auto
// OFFSET: 0x005EEB70
void CGGameObject_C_TypeBase::HandleAnimFinished() {
}

// VERIFY: 4d2d7931 auto
// OFFSET: 0x00632050
void CGGameObject_C_TypeBase::ActivateOneShotAnim(uint32_t anim) {
}

// VERIFY: a3b01406 auto
// OFFSET: 0x0070B220
const char* CGGameObject_C_TypeBase::DebugStatus() {
    return "";
}

// VERIFY: 9dd812d6 auto
// OFFSET: 0x006F1700
float CGGameObject_C_TypeBase::GetSpeed() const {
    return -1.0f;
}

// VERIFY: 4c00802d auto
// OFFSET: 0x00653A10
void CGGameObject_C_TypeBase::AddPassenger(CPassenger* passenger, int32_t a2) {
}

// VERIFY: 8d4e2b06 auto
// OFFSET: 0x004899F0
int32_t CGGameObject_C_TypeBase::IsPointInside(const C3Vector& point) const {
    return 0;
}

// VERIFY: d47c3162 auto
// OFFSET: 0x008E5250
CMapBaseObj* CGGameObject_C_TypeBase::GetMapObjectTransportID() const {
    return nullptr;
}

// VERIFY: 980ed0eb auto
// OFFSET: 0x00632050
void CGGameObject_C_TypeBase::PostInit(bool a1) {
}

// VERIFY: 8b5d6ada auto
// OFFSET: 0x005EEB70
void CGGameObject_C_TypeBase::PostInitWithStats() {
}

// VERIFY: d0d1bd54 auto
// OFFSET: 0x005EEB70
void CGGameObject_C_TypeBase::Reenable() {
}

// VERIFY: 97ab290a auto
// OFFSET: 0x005EEB70
void CGGameObject_C_TypeBase::Disable() {
}

// VERIFY: 8e070758 auto
// OFFSET: 0x005EEB70
void CGGameObject_C_TypeBase::PostReenable() {
}

// VERIFY: 247ea7ab auto
// OFFSET: 0x008E5250
ShipPath* CGGameObject_C_TypeBase::GetShipPath() const {
    return nullptr;
}

// VERIFY: 06edc0db auto
// OFFSET: 0x00653A10
void CGGameObject_C_TypeBase::UpdateMovement(uint32_t time, uint32_t elapsed) {
}

// VERIFY: a4cd3ea3 auto
// OFFSET: 0x00632050
void CGGameObject_C_TypeBase::UpdateDisplay(uint32_t time) {
}

// VERIFY: 9289b4b5 auto
// OFFSET: 0x005EEB70
void CGGameObject_C_TypeBase::UpdateDisabled() {
}

// VERIFY: 9690f15d auto
// OFFSET: 0x005EEB70
void CGGameObject_C_TypeBase::StartInteraction() {
}

// VERIFY: abeac73c auto
// OFFSET: 0x005EEB70
void CGGameObject_C_TypeBase::CloseInteraction() {
}

// VERIFY: c9a9e833 auto
// OFFSET: 0x00632050
void CGGameObject_C_TypeBase::AddToolTipInfo(CGTooltip* tooltip) {
}

// VERIFY: 7648e923 auto
// OFFSET: 0x0070BDC0
uint32_t CGGameObject_C_TypeBase::GetSequenceTimeFromRealTime(uint32_t time) const {
    return time;
}

// VERIFY: 0abdada4 auto
// OFFSET: 0x008E5250
uint32_t CGGameObject_C_TypeBase::GetTransportUpdateTime() const {
    return 0;
}

// VERIFY: b57f809b auto
// OFFSET: 0x00427A90
bool CGGameObject_C_Type_Null::CanUse() const {
    return false;
}

// VERIFY: 1a45ca41 auto
// OFFSET: 0x0070FCD0
void CGGameObject_C_TypeAnimated::UpdateDisabled() {
    this->ResetAnimState();
}

// VERIFY: 8cc82559 auto
// OFFSET: 0x0070CBA0
void CGGameObject_C::SetStorage(uint32_t* storage, uint32_t* saved) {
    this->CGObject_C::SetStorage(storage, saved);
    this->m_gameObject = reinterpret_cast<CGGameObjectData*>(storage + OBJECT_END);
    this->m_gameObjectMirror = saved + CGObject::TotalFieldsSaved();
}

// VERIFY: dfb71ec2 manual
// OFFSET: 0x00714250
CGGameObject_C::CGGameObject_C(uint32_t time, const CClientObjCreate& create) : CGObject_C(time, create), m_passenger(create.m_move.m_status.m_transportGuid, create.m_move.m_status.m_transportPosition, this->m_obj->m_guid), m_typeData(nullptr), m_stats(nullptr) {
    this->m_worldBounds.b = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_worldBounds.t = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_pendingOneShotAnim = 0;
    this->m_hasWorldBounds = 0;
    this->m_lootEffect = nullptr;
    this->m_displayState = this->m_gameObject->m_state;
    *reinterpret_cast<uint64_t*>(&this->m_passenger.m_facing) = create.m_packedRotation;
    this->m_passenger.m_passengerFlags |= 0x2;

    ClntObjMgrHideObject(this->m_obj->m_guid);

    if (create.m_updateFlags & 0x2) {
        this->m_transportTimeOffset = create.m_transportTime - time;
    }

    switch (this->m_gameObject->m_typeID) {
        case TYPE_DOOR:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Door), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Door(this);
            break;
        case TYPE_BUTTON:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Button), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Button(this);
            break;
        case TYPE_CHEST:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Chest), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Chest(this);
            break;
        case TYPE_TRAP:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Trap), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Trap(this);
            break;
        case TYPE_AREADAMAGE:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_AreaDamage), __FILE__, __LINE__, 0)) CGGameObject_C_Type_AreaDamage(this);
            break;
        case TYPE_QUESTGIVER:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_QuestGiver), __FILE__, __LINE__, 0)) CGGameObject_C_Type_QuestGiver(this);
            break;
        case TYPE_BINDER:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Binder), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Binder(this);
            break;
        case TYPE_GENERIC:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Generic), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Generic(this);
            break;
        case TYPE_CHAIR:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Chair), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Chair(this);
            break;
        case TYPE_SPELL_FOCUS:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_SpellFocus), __FILE__, __LINE__, 0)) CGGameObject_C_Type_SpellFocus(this);
            break;
        case TYPE_TEXT:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Text), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Text(this);
            break;
        case TYPE_GOOBER:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Goober), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Goober(this);
            break;
        case TYPE_TRANSPORT:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Transport), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Transport(this);
            break;
        case TYPE_TRAPDOOR:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_TrapDoor), __FILE__, __LINE__, 0)) CGGameObject_C_Type_TrapDoor(this);
            break;
        case TYPE_CAMERA:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Camera), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Camera(this);
            break;
        case TYPE_MAP_OBJECT:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_MapObj), __FILE__, __LINE__, 0)) CGGameObject_C_Type_MapObj(this);
            break;
        case TYPE_MO_TRANSPORT:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_MapObjTransport), __FILE__, __LINE__, 0)) CGGameObject_C_Type_MapObjTransport(this);
            break;
        case TYPE_DUEL_ARBITER:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_DuelArbiter), __FILE__, __LINE__, 0)) CGGameObject_C_Type_DuelArbiter(this);
            break;
        case TYPE_FISHINGHOLE:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_FishingHole), __FILE__, __LINE__, 0)) CGGameObject_C_Type_FishingHole(this);
            break;
        case TYPE_FISHINGNODE:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_FishingNode), __FILE__, __LINE__, 0)) CGGameObject_C_Type_FishingNode(this);
            break;
        case TYPE_RITUAL:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Ritual), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Ritual(this);
            break;
        case TYPE_MAILBOX:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Mailbox), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Mailbox(this);
            break;
        case TYPE_SPELLCASTER:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_SpellCaster), __FILE__, __LINE__, 0)) CGGameObject_C_Type_SpellCaster(this);
            break;
        case TYPE_MEETINGSTONE:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_MeetingStone), __FILE__, __LINE__, 0)) CGGameObject_C_Type_MeetingStone(this);
            break;
        case TYPE_FLAGSTAND:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_FlagStand), __FILE__, __LINE__, 0)) CGGameObject_C_Type_FlagStand(this);
            break;
        case TYPE_FLAGDROP:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_FlagDrop), __FILE__, __LINE__, 0)) CGGameObject_C_Type_FlagDrop(this);
            break;
        case TYPE_MINIGAME:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_MiniGame), __FILE__, __LINE__, 0)) CGGameObject_C_Type_MiniGame(this);
            break;
        case TYPE_CAPTUREPOINT:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_CapturePoint), __FILE__, __LINE__, 0)) CGGameObject_C_Type_CapturePoint(this);
            break;
        case TYPE_AURAGENERATOR:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_AuraGenerator), __FILE__, __LINE__, 0)) CGGameObject_C_Type_AuraGenerator(this);
            break;
        case TYPE_DUNGEON_DIFFICULTY:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_DungeonDifficulty), __FILE__, __LINE__, 0)) CGGameObject_C_Type_DungeonDifficulty(this);
            break;
        case TYPE_BARBER_CHAIR:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_BarberChair), __FILE__, __LINE__, 0)) CGGameObject_C_Type_BarberChair(this);
            break;
        case TYPE_DESTRUCTIBLE_BUILDING:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_DestructibleBuilding), __FILE__, __LINE__, 0)) CGGameObject_C_Type_DestructibleBuilding(this);
            break;
        case TYPE_GUILD_BANK:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_GuildBank), __FILE__, __LINE__, 0)) CGGameObject_C_Type_GuildBank(this);
            break;
        default:
            this->m_typeData = new (SMemAlloc(sizeof(CGGameObject_C_Type_Null), __FILE__, __LINE__, 0)) CGGameObject_C_Type_Null(this);
            SysMsgPrintf(SYSMSG_FATAL, 2, "BADBASEGAMEOBJECT|%d", this->m_gameObject->m_typeID);
            break;
    }

    this->m_effectAnimState = 0;
}

// VERIFY: cd300a2b auto
// OFFSET: 0x00712F30
void CGGameObject_C::PostInit(uint32_t time, const CClientObjCreate& create, bool reenable) {
    this->m_passenger.PostInit(create.m_move.m_status.m_position);
    this->CGObject_C::PostInit(time, create, reenable);

    CM2Model* model = this->m_worldModel;

    if (model) {
        ClntObjMgrShowObject(this->m_obj->m_guid);
        model->SetEventCallback(reinterpret_cast<void*>(&CGGameObject_C::AnimEventCallback), this->m_obj->m_guid.m_guid);
        model->SetSequenceCallback(reinterpret_cast<void*>(&CGGameObject_C::SequenceCallback), this->m_obj->m_guid.m_guid);

        if (model->IsLoaded(0, 0)) {
            CM2Model* worldModel = this->m_worldModel;

            if (!(worldModel->m_shared->m_flags & 0x1)) {
                worldModel->WaitForLoad(nullptr);
            }

            CAaBox bounds = worldModel->m_shared->m_data->m_collisionBounds.m_extent;
            CWorldMath::TransformAABox(this->m_matrix, bounds, this->m_worldBounds);

            if (this->m_typeData->CanBeSolid()) {
                this->m_hasWorldBounds = !this->m_worldBounds.IsEmpty();
            }
        }
    }

    this->m_typeData->PostInit(reenable);

    WOWGUID guid = this->m_obj->m_guid;
    GameObjectStats_C* stats = g_gameObjectDBCache.GetRecord(this->m_obj->m_entryID, guid, &GameObjectStatsCallback, this, false);

    if (stats) {
        this->LoadBaseObject(stats);
    }
}

// VERIFY: 1254baa1 auto
// OFFSET: 0x004268C0
bool CGGameObject_C_Type_Null::CanUseNow(GAME_ERROR_TYPE* error, float* distance) const {
    return false;
}

// VERIFY: d1466c05 manual
// OFFSET: 0x0070D900
CGGameObject_C_Type_WorldObj::~CGGameObject_C_Type_WorldObj() {
    if (this->m_worldObject) {
        CWorld::ObjectDestroy(this->m_worldObject);
        this->m_worldObject = nullptr;
    }
}

// VERIFY: 155035c0 auto
// OFFSET: 0x00427A90
bool CGGameObject_C_Type_WorldObj::CanBeSolid() const {
    return false;
}

// VERIFY: 4e81b6bf auto
// OFFSET: 0x00427A90
bool CGGameObject_C_Type_WorldObj::CanHighlight() const {
    return false;
}

// VERIFY: 9ea49663 auto
// OFFSET: 0x00427A90
bool CGGameObject_C_Type_WorldObj::CanUse() const {
    return false;
}

// VERIFY: 3f8abdcd auto
// OFFSET: 0x0095CB40
CMapBaseObj* CGGameObject_C_Type_WorldObj::GetMapObjectTransportID() const {
    return this->m_worldObject;
}

// VERIFY: f4c8758a manual
// OFFSET: 0x0070B270
void CGGameObject_C_Type_WorldObj::PostInitWithStats() {
    this->CreateWorldObj();
}

// VERIFY: dbdf67a5 manual
// OFFSET: 0x0070B280
void CGGameObject_C_Type_WorldObj::Disable() {
    this->DestroyWorldObj();
}

// VERIFY: ed9fece3 manual
// OFFSET: 0x0070B270
void CGGameObject_C_Type_WorldObj::PostReenable() {
    this->CreateWorldObj();
}

// VERIFY: 209f54dd manual
// OFFSET: 0x00712610
CGGameObject_C_Type_TransportBase::~CGGameObject_C_Type_TransportBase() {
    if (this->m_worldObject) {
        MovementRemoveTransport(this->m_owner);
    }
}

// VERIFY: 1e87d942 auto
// OFFSET: 0x007126A0
C3Vector CGGameObject_C_Type_TransportBase::GetPosition() const {
    return this->m_position;
}

// VERIFY: d5e93300 auto
// OFFSET: 0x0070B360
int32_t CGGameObject_C_Type_TransportBase::IsPointInside(const C3Vector& point) const {
    if (!this->m_worldObject) {
        return 1;
    }

    return World::ObjectTestConvexVolume(this->m_worldObject, point);
}

// VERIFY: ff1b15c0 auto
// OFFSET: 0x0070B330
void CGGameObject_C_Type_TransportBase::PostReenable() {
    this->CreateWorldObj();

    if (this->m_worldObject) {
        MovementAddTransport(this->m_owner);
    }
}

// VERIFY: 68c80836 manual
// OFFSET: 0x007101C0
void CGGameObject_C_Type_MapObjTransport::UpdateState(int32_t oldState, int32_t newState) {
    uint32_t pausePointsIndex = CGameObjectDef::GetPropNum(this->m_owner->m_gameObject->m_typeID, 128);
    GameObjectStats* pausePointsStats = this->m_owner->m_stats;
    uint32_t pausePoints = (pausePointsStats && pausePointsIndex < 24) ? pausePointsStats->m_propValue[pausePointsIndex] : 0;

    if (!pausePoints) {
        return;
    }

    bool closed = newState == 1;

    if (oldState == newState) {
        this->m_shipPath.WaitAtNextPausePoint(this->m_pathTime, !closed);
    }

    this->m_shipPath.WaitAtNextPausePoint(this->m_pathTime, closed);
}

// VERIFY: 70dac489 manual
// OFFSET: 0x0070DDD0
void CGGameObject_C_Type_DestructibleBuilding::PostInit(bool a1) {
    GameObjectDisplayInfoRec* display = g_gameObjectDisplayInfoDB.GetRecord(this->m_owner->m_gameObject->m_displayID);

    if (display) {
        CGGameObject_C* owner = this->m_owner;
        C3Vector position = owner->m_passenger.GetPosition(owner->m_passenger.m_position);
        this->m_proxy.m_sphere.c = position;
        float x = display->m_geoBoxMax[0] * display->m_geoBoxMax[0];
        float minX = display->m_geoBoxMin[0] * display->m_geoBoxMin[0];

        if (minX >= x) {
            x = minX;
        }

        x = sqrt(x);
        float y = display->m_geoBoxMax[1] * display->m_geoBoxMax[1];
        float minY = display->m_geoBoxMin[1] * display->m_geoBoxMin[1];

        if (minY >= y) {
            y = minY;
        }

        y = sqrt(y);
        float z = display->m_geoBoxMax[2] * display->m_geoBoxMax[2];
        float minZ = display->m_geoBoxMin[2] * display->m_geoBoxMin[2];

        if (minZ >= z) {
            z = minZ;
        }

        z = sqrt(z);
        this->m_proxy.m_sphere.r = sqrt(z * z + y * y + x * x);
        this->m_proxy.m_uniqueId = reinterpret_cast<const uint32_t&>(this->m_owner->m_gameObject->m_parentRotation[0]) == 0x5476ED ? 0x5476ED : 0;

        if (this->m_proxy.m_sphere.r < 0.001f) {
            this->m_proxy.m_sphere.r = 50.0f;
            SysMsgPrintf(SYSMSG_ERROR, 2, "Destructible building WMO(%s) has invalid geobox", display->m_modelName);
        }

        World::FindOrCreateDestructibleProxy(&this->m_proxy);
    }

    if (this->m_owner) {
        this->m_owner->UpdateWorldObject(0);
    }
}

// VERIFY: 508fc5c2 manual
// OFFSET: 0x0070DF30
bool CGGameObject_C_Type_DestructibleBuilding::CanHighlight() const {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && this->m_owner->m_obj->m_guid.m_guid == World::QueryObjectParentParam64(player->m_worldObject)) {
        return false;
    }

    return this->m_modelData && !this->m_modelData->m_doNotHighlight;
}

// VERIFY: aad75636 manual
// OFFSET: 0x0070DFA0
void CGGameObject_C_Type_DestructibleBuilding::UpdateDestructibleState() {
    int32_t current = 0;

    do {
        if (this->m_states[current].m_object == this->m_worldObject) {
            break;
        }

        current++;
    } while (current < 4);

    int32_t state = ((this->m_owner->m_gameObject->m_flags >> 8) & 0xE) >> 1;

    if (state == current) {
        return;
    }

    SI2::StopSound(&this->m_sound, 0, -1.0f, 1);
    DestructibleBuildingState& entry = this->m_states[state];
    CMapEntity* object = entry.m_object;

    if (object) {
        World::ObjectSetRenderEnabled(object, 1);
        World::ObjectEnableCollision(object, 1);
    }

    if (this->m_worldObject) {
        World::ObjectSetRenderEnabled(this->m_worldObject, 0);
        World::ObjectEnableCollision(this->m_worldObject, 0);
    }

    if (this->m_worldObject && state > current && state != 3 && state) {
        CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

        if (mover && !mover->HasVehicleTransport() && !(mover->m_movement->m_flags & 0x2000000)) {
            CAaBox extents;
            extents.b = C3Vector(0.0f, 0.0f, 0.0f);
            extents.t = C3Vector(0.0f, 0.0f, 0.0f);
            World::ObjectGetExtents(this->m_worldObject, &extents);
            C44Matrix inverse = this->m_owner->GetMatrix().AffineInverse();

            if (extents.ContainsPointExclusive(mover->GetPosition() * inverse)) {
                mover->OnFallLocal(OsGetAsyncTimeMs());
            }
        }
    }

    uint16_t moved[2] = { this->GetImpactEffectDoodadSet(2 * current), this->GetAmbientDoodadSet(2 * current) };
    uint16_t offset = 100 * (current + 1);

    for (uint32_t i = 0; i < 2; i++) {
        uint16_t set = moved[i];

        if (!set) {
            continue;
        }

        World::ObjectSetSequence(this->m_worldObject, 155, 0, set);
        World::ObjectSetSequenceFinishedCallback(this->m_worldObject, &CGGameObject_C_Type_DestructibleBuilding::MovedDoodadSequenceHandler, static_cast<uint64_t>(static_cast<int32_t>(set + offset)) | (static_cast<uint64_t>(reinterpret_cast<uintptr_t>(this)) << 32), set);
        World::ObjectSetEventCallback(this->m_worldObject, &CGGameObject_C_Type_DestructibleBuilding::AnimEventCallback, this->m_owner->m_obj->m_guid.m_guid, set);
        uint16_t target = set + offset;
        World::ObjectMoveDoodads(this->m_worldObject, set, object, target);
        World::ObjectSetDoodadsEnabled(object, 1, target);
    }

    if (this->m_previousState != -1) {
        uint16_t restored[2] = { this->GetImpactEffectDoodadSet(2 * this->m_previousState), this->GetAmbientDoodadSet(2 * this->m_previousState) };

        for (uint32_t i = 0; i < 2; i++) {
            uint16_t set = restored[i];

            if (!set) {
                continue;
            }

            World::ObjectMoveDoodads(this->m_worldObject, 100 * (this->m_previousState + 1) + set, this->m_states[this->m_previousState].m_object, set);
            World::ObjectSetSequenceFinishedCallback(this->m_states[this->m_previousState].m_object, nullptr, 0, set);
            World::ObjectSetDoodadsEnabled(this->m_states[this->m_previousState].m_object, 0, set);
            World::ObjectSetSequence(this->m_states[this->m_previousState].m_object, 0, 0, set);
            World::ObjectSetEventCallback(this->m_worldObject, &CGGameObject_C_Type_DestructibleBuilding::AnimEventCallback, this->m_owner->m_obj->m_guid.m_guid, set);
        }
    }

    this->m_worldObject = object;
    this->m_previousState = current;
    entry.m_animate = !state || state == 3;

    if (!this->m_worldObject || entry.m_loading) {
        return;
    }

    if (entry.m_animate) {
        if (this->m_modelData) {
            float speed = 12.0f;
            float healSpeed = static_cast<float>(this->m_modelData->m_healEffectSpeed);

            if (healSpeed > 0.0f) {
                speed = healSpeed;
            }

            int32_t effect = this->m_modelData->m_healEffect;

            if (effect != 4 && speed > 0.0f) {
                CAaBox extents;
                extents.b = C3Vector(0.0f, 0.0f, 0.0f);
                extents.t = C3Vector(0.0f, 0.0f, 0.0f);
                World::ObjectGetExtents(this->m_worldObject, &extents);
                this->m_sinkDepth = extents.t.z - extents.b.z;
                float z = this->m_owner->GetPosition().z;
                C3Vector position(z, z, z);
                this->m_baseZ = z;
                World::ObjectUpdate(this->m_worldObject, &position, this->m_owner->GetFacing(), 0.0f, 0.0f, 0);
                this->m_animating = 1;
                this->m_baseZ = position.z;
                this->m_animStartTime = FrameTime::s_curTimeMs;
                this->m_animDuration = static_cast<uint32_t>((extents.t.z - extents.b.z) / (speed * 0.33333334f) * 1000.0f);

                if (this->m_previousState != -1 && effect != 3) {
                    World::ObjectGetExtents(this->m_states[this->m_previousState].m_object, &extents);
                    this->m_previousSinkDepth = extents.t.z - extents.b.z;
                    World::ObjectSetRenderEnabled(this->m_states[this->m_previousState].m_object, 1);
                    World::ObjectEnableCollision(this->m_states[this->m_previousState].m_object, 1);
                }

                if (this->m_baseCollision) {
                    World::ObjectEnableCollision(this->m_baseCollision, 1);
                }
            }
        }
    } else if (this->m_modelData) {
        uint16_t set;

        switch (((this->m_owner->m_gameObject->m_flags >> 8) & 0xE) >> 1) {
            case 1:
                set = this->m_modelData->m_state1DestructionDoodadSet;
                break;

            case 2:
                set = this->m_modelData->m_state2DestructionDoodadSet;
                break;

            case 3:
                set = this->m_modelData->m_state3InitDoodadSet;
                break;

            default:
                set = 0;
                break;
        }

        if (set) {
            World::ObjectSetDoodadsEnabled(this->m_worldObject, 1, set);
            World::ObjectSetSequence(this->m_worldObject, 153, 0, set);
            World::ObjectSetSequenceFinishedCallback(this->m_worldObject, &CGGameObject_C_Type_DestructibleBuilding::DestructSequenceFinishedHandler, set, set);
            World::ObjectSetEventCallback(this->m_worldObject, &CGGameObject_C_Type_DestructibleBuilding::AnimEventCallback, this->m_owner->m_obj->m_guid.m_guid, set);
        }
    }

    if (!this->m_modelData) {
        return;
    }

    uint16_t ambient;

    switch (((this->m_owner->m_gameObject->m_flags >> 8) & 0xE) >> 1) {
        case 0:
            ambient = this->m_modelData->m_state0AmbientDoodadSet;
            break;

        case 1:
            ambient = this->m_modelData->m_state1AmbientDoodadSet;
            break;

        case 2:
            ambient = this->m_modelData->m_state2AmbientDoodadSet;
            break;

        case 3:
            ambient = this->m_modelData->m_state3AmbientDoodadSet;
            break;

        default:
            return;
    }

    if (ambient) {
        World::ObjectSetDoodadsEnabled(this->m_worldObject, 1, ambient);
        World::ObjectSetSequence(this->m_worldObject, 153, 0, ambient);
        World::ObjectSetSequenceFinishedCallback(this->m_worldObject, &CGGameObject_C_Type_DestructibleBuilding::AmbientSequenceFinishedHandler, ambient, ambient);
        World::ObjectSetEventCallback(this->m_worldObject, &CGGameObject_C_Type_DestructibleBuilding::AnimEventCallback, this->m_owner->m_obj->m_guid.m_guid, ambient);
    }
}

// VERIFY: 934c0c45 auto
// OFFSET: 0x0070E6A0
void CGGameObject_C_Type_DestructibleBuilding::OnUpdateFlags(uint32_t changed) {
    if (!this->m_modelData || !(changed & 0xF00)) {
        return;
    }

    if (changed & 0x100) {
        uint16_t set = this->GetImpactEffectDoodadSet((this->m_owner->m_gameObject->m_flags >> 8) & 0xF);

        if (set) {
            World::ObjectSetDoodadsEnabled(this->m_worldObject, 1, set);
            World::ObjectSetSequence(this->m_worldObject, 153, 0, set);
            World::ObjectSetSequenceFinishedCallback(this->m_worldObject, &CGGameObject_C_Type_DestructibleBuilding::ImpactEffectSequenceFinishedHandler, set, set);
            World::ObjectSetEventCallback(this->m_worldObject, &CGGameObject_C_Type_DestructibleBuilding::AnimEventCallback, this->m_owner->m_obj->m_guid.m_guid, set);
        }
    }

    this->UpdateDestructibleState();
}

// VERIFY: 8250b3e2 manual
// OFFSET: 0x0070E750
void CGGameObject_C_Type_DestructibleBuilding::UpdateDisplay(uint32_t time) {
    if (this->m_loadPending) {
        this->m_loadPending = 0;
        DestructibleBuildingState* entry = this->m_states;

        for (int32_t state = 0; state < 8; state += 2, entry++) {
            if (!entry->m_loading || !entry->m_object) {
                continue;
            }

            if (!World::ObjectIsLoaded(entry->m_object)) {
                this->m_loadPending = 1;
                continue;
            }

            entry->m_loading = 0;
            uint16_t sets[3];
            sets[0] = this->GetDestructionDoodadSet(state);
            sets[1] = this->GetImpactEffectDoodadSet(state);
            sets[2] = this->GetAmbientDoodadSet(state);

            for (uint32_t i = 0; i < 3; i++) {
                uint16_t set = sets[i];

                if (set) {
                    World::ObjectSetDoodadEmittersIgnoresDistance(entry->m_object, 1, set);
                    World::ObjectSetDoodadsEnabled(entry->m_object, 0, set);
                }
            }

            if (entry->m_object != this->m_worldObject) {
                continue;
            }

            uint16_t ambient = this->GetAmbientDoodadSet((this->m_owner->m_gameObject->m_flags >> 8) & 0xF);

            if (ambient) {
                World::ObjectSetDoodadsEnabled(this->m_worldObject, 1, ambient);
                World::ObjectSetSequence(this->m_worldObject, 153, 0, ambient);
                World::ObjectSetSequenceFinishedCallback(this->m_worldObject, &CGGameObject_C_Type_DestructibleBuilding::AmbientSequenceFinishedHandler, ambient, ambient);
                World::ObjectSetEventCallback(this->m_worldObject, &CGGameObject_C_Type_DestructibleBuilding::AnimEventCallback, this->m_owner->m_obj->m_guid.m_guid, ambient);
            }
        }

        return;
    }

    if (!this->m_animating) {
        return;
    }

    if (time < this->m_animStartTime + this->m_animDuration) {
        C3Vector position = this->m_owner->GetPosition();
        float progress = static_cast<float>(time - this->m_animStartTime) / static_cast<float>(this->m_animDuration);
        int32_t effect;

        if (!this->m_modelData) {
            effect = 4;
        } else {
            effect = this->m_modelData->m_healEffect;

            if (!effect) {
                float sink = 0.0f;

                if (progress > 0.5f) {
                    float t = 1.0f - ((progress - 0.5f) + (progress - 0.5f));
                    sink = 1.0f - t * (t * t);
                }

                C3Vector target = position;
                target.z = this->m_baseZ - (1.0f - sink) * this->m_sinkDepth;
                World::ObjectUpdate(this->m_worldObject, &target, this->m_owner->GetFacing(), 0.0f, 0.0f, 0);

                if (this->m_previousState != -1) {
                    float scale = progress >= 0.5f ? 1.0f : progress * (progress * progress) * 8.0f;
                    C3Vector previous = this->m_owner->GetPosition();
                    previous.z = this->m_baseZ - this->m_previousSinkDepth * scale;
                    World::ObjectUpdate(this->m_states[this->m_previousState].m_object, &previous, this->m_owner->GetFacing(), 0.0f, 0.0f, 0);
                }

                goto rebuild;
            }
        }

        {
            C3Vector target = position;
            float remaining = 1.0f - progress;
            target.z = this->m_baseZ - (1.0f - (1.0f - remaining * (remaining * remaining))) * this->m_sinkDepth;
            World::ObjectUpdate(this->m_worldObject, &target, this->m_owner->GetFacing(), 0.0f, 0.0f, 0);

            if (this->m_previousState != -1 && effect == 2) {
                C3Vector previous = this->m_owner->GetPosition();
                previous.z = this->m_baseZ - progress * (progress * progress) * this->m_previousSinkDepth;
                World::ObjectUpdate(this->m_states[this->m_previousState].m_object, &previous, this->m_owner->GetFacing(), 0.0f, 0.0f, 0);
            }
        }

    rebuild:
        if (this->m_rebuildEffect) {
            World::ObjectSetRenderEnabled(this->m_rebuildEffect, 1);
            World::ObjectEnableCollision(this->m_rebuildEffect, 1);
            World::ObjectUpdate(this->m_rebuildEffect, &position, this->m_owner->GetFacing(), 0.0f, 0.0f, 0);
        }

        return;
    }

    if (this->m_rebuildEffect) {
        C3Vector position = this->m_owner->GetPosition();
        World::ObjectUpdate(this->m_rebuildEffect, &position, this->m_owner->GetFacing(), 0.0f, 0.0f, 0);
        World::ObjectSetRenderEnabled(this->m_rebuildEffect, 0);
        World::ObjectEnableCollision(this->m_rebuildEffect, 0);
    }

    C3Vector position = this->m_owner->GetPosition();
    position.z = this->m_baseZ;
    World::ObjectUpdate(this->m_worldObject, &position, this->m_owner->GetFacing(), 0.0f, 0.0f, 0);
    this->m_animating = 0;

    if (this->m_previousState != -1) {
        World::ObjectSetRenderEnabled(this->m_states[this->m_previousState].m_object, 0);
        World::ObjectEnableCollision(this->m_states[this->m_previousState].m_object, 0);
        World::ObjectUpdate(this->m_states[this->m_previousState].m_object, &position, this->m_owner->GetFacing(), 0.0f, 0.0f, 0);
    }

    if (this->m_baseCollision) {
        World::ObjectEnableCollision(this->m_baseCollision, 0);
    }
}

// VERIFY: 3805df66 auto
// OFFSET: 0x0070EC80
void CGGameObject_C_Type_DestructibleBuilding::HandleAnimEvent(uint32_t eventId, uint32_t data, const C3Vector& position, uint32_t time) {
    GameObject_C_ProcessAnimEvent(eventId, this->m_owner->m_gameObject->m_displayID, data, position, &this->m_sound);
}

// VERIFY: 427d6342 auto
// OFFSET: 0x005EEB70
void CGGameObject_C_Type_DestructibleBuilding::CreateWorldObj() {
}

// VERIFY: 3411ab45 auto
// OFFSET: 0x005EEB70
void CGGameObject_C_Type_DestructibleBuilding::DestroyWorldObj() {
}

// VERIFY: 273ab916 manual
// OFFSET: 0x0070ECB0
bool CGGameObject_C_Type_GuildBank::Use() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        WOWGUID guid = this->m_owner->m_obj->m_guid;
        CGPlayer_C::TalkToGuildBanker(guid);
    }

    return true;
}

// VERIFY: 1f9c8c22 manual
// OFFSET: 0x0070C9F0
void CGGameObject_C_Type_GuildBank::StartInteraction() {
    CGGameObject_C* owner = this->m_owner;

    if (owner->IsObjectModelLoaded()) {
        owner->m_typeData->ActivateOneShotAnim(8);
        owner->m_pendingOneShotAnim = 0;
    } else {
        owner->m_pendingOneShotAnim = 2;
    }
}

// VERIFY: cf09afa2 auto
// OFFSET: 0x004F58A0
CAaBox CGGameObject_C::GetWorldBounds() const {
    return this->m_worldBounds;
}

// VERIFY: 218fceeb manual
// OFFSET: 0x0070CBE0
void CGGameObject_C::UpdateWorldObject(int32_t a2) {
    this->m_matrix.Identity();
    this->m_matrix.Translate(this->GetPosition());
    this->m_matrix.Rotate(this->GetRotation());
    this->m_matrix.Scale(this->GetScale());
    CM2Model* model = this->m_worldModel;
    CAaBox box(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f));
    CAaSphere sphere;
    sphere.c = C3Vector(0.0f, 0.0f, 0.0f);
    sphere.r = 0.0f;
    C3Vector center(0.0f, 0.0f, 0.0f);

    if (model) {
        if (model->IsLoaded(0, 0)) {
            if (!(model->m_shared->m_flags & 0x1)) {
                model->WaitForLoad(nullptr);
            }

            CAaBox bounds = model->m_shared->m_data->m_collisionBounds.m_extent;
            CWorldMath::TransformAABox(this->m_matrix, bounds, this->m_worldBounds);
            box = model->GetBoundingBox();
            sphere = model->GetBoundingSphere();
            center = C3Vector((bounds.t.x + bounds.b.x) * 0.5f, (bounds.t.y + bounds.b.y) * 0.5f, 0.5f * (bounds.t.z + bounds.b.z));
        }

        model->m_fFlags |= 0x8000;
        model->m_worldTransform = this->m_matrix;
    }

    if (this->m_worldObject) {
        CMap::ObjectUpdate(this->m_worldObject, this->m_matrix, box, sphere, center, a2, 0xFFFFFFFF);
    }
}

// VERIFY: 68b8aeae auto
// OFFSET: 0x00712DB0
float CGGameObject_C::GetFacing() const {
    if (this->m_typeData) {
        return this->m_typeData->GetFacing();
    }

    return this->m_passenger.GetFacing(this->m_passenger.GetRawFacingFromRotation());
}

// VERIFY: e7d3bbca manual
// OFFSET: 0x00712DF0
float CGGameObject_C::GetRawFacing() const {
    if (this->m_typeData) {
        return this->m_typeData->GetRawFacing();
    }

    return this->m_passenger.GetRawFacingFromRotation();
}

// VERIFY: c3c2da56 auto
// OFFSET: 0x00712E20
C4Quaternion CGGameObject_C::GetRotation() const {
    return this->m_passenger.GetRotation();
}

// VERIFY: 7b5a289f manual
// OFFSET: 0x00713F50
void CGGameObject_C::ModelLoaded(CM2Model* model) {
    CAaBox box = model->GetBoundingBox();
    this->m_height = box.t.z - box.b.z;

    if ((this->m_modelFlags & 0x40000) && !(this->m_modelFlags & 0x10000)) {
        this->UpdateWorldObject(0);
    }

    if (this->m_typeData->CanBeSolid()) {
        this->m_hasWorldBounds = !this->m_worldBounds.IsEmpty();
    }

    ApplyGameObjectArtKit(model, this->m_gameObject->m_artKit);
    this->m_typeData->UpdateDisabled();
    this->UpdateInteractIconAttach();

    if (this->m_modelFlags & 0x400000) {
        this->m_modelFlags |= 0x400000;
        this->ShowLootEffect();
    }

    int32_t pending = this->m_pendingOneShotAnim;

    if (pending) {
        if (!this->IsObjectModelLoaded()) {
            this->m_pendingOneShotAnim = pending;
            model->m_fFlags |= 0x4;
            return;
        }

        this->m_typeData->ActivateOneShotAnim(s_oneShotAnims[pending]);
        this->m_pendingOneShotAnim = 0;
    }

    model->m_fFlags |= 0x4;
}

// VERIFY: e5a829d0 manual
// OFFSET: 0x0070C330
float CGGameObject_C_TypeBase::GetRawFacing() const {
    return this->m_owner->m_passenger.GetRawFacingFromRotation();
}

// VERIFY: 4dbf2cb2 auto
// OFFSET: 0x0070C310
float CGGameObject_C_TypeBase::GetFacing() const {
    CPassenger& passenger = this->m_owner->m_passenger;
    return passenger.GetFacing(passenger.GetRawFacingFromRotation());
}

// VERIFY: 901f38d6 auto
// OFFSET: 0x0070C430
int32_t CGGameObject_C_TypeAnimated::HandleAnimInterrupted() {
    int32_t state = this->m_pendingAnimState;

    if (state == -1 || state == ANIMSTATE_OPEN || state == ANIMSTATE_CLOSED || state == ANIMSTATE_DESTROYED) {
        this->m_pendingAnimState = -1;
        return 1;
    }

    ClntObjMgrUnlockObject(this->m_owner->m_obj->m_guid, __FILE__, __LINE__);
    return 0;
}

// VERIFY: 4f1c02d7 auto
// OFFSET: 0x0070EF30
LockRec* CGGameObject_C::GetLockRec() const {
    if (!this->m_stats) {
        return nullptr;
    }

    uint32_t lockId = this->GetPropertyValue(CGameObjectDef::GetPropNum(this->m_gameObject->m_typeID, 4));
    return lockId ? g_lockDB.GetRecord(lockId) : nullptr;
}

// VERIFY: eef7dd91 manual
// OFFSET: 0x0070F010
bool CGGameObject_C::IsValidTargetForSpell(const WOWGUID& item, int32_t spellId) const {
    LockRec* lock = this->GetLockRec();

    if (!lock) {
        return false;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return false;
    }

    CGItem_C* key = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__));

    for (uint32_t i = 0; i < 3; i++) {
        if (spell.m_effect[i] != SPELL_EFFECT_OPEN_LOCK) {
            continue;
        }

        for (int32_t j = 0; j < 8; j++) {
            switch (lock->m_type[j]) {
                case 1:
                    if (!key || static_cast<int32_t>(key->m_obj->m_entryID) != lock->m_index[j]) {
                        continue;
                    }

                    break;

                case 2:
                    if (spell.m_effectMiscValue[i] != lock->m_index[j]) {
                        continue;
                    }

                    break;

                case 3:
                    if (spell.m_id != lock->m_index[j]) {
                        continue;
                    }

                    break;

                default:
                    continue;
            }

            if (this->IsValidOpenAction(lock->m_action[j])) {
                return true;
            }
        }
    }

    return false;
}

// VERIFY: 81b3c5cd auto
// OFFSET: 0x0070CE10
int32_t CGGameObject_C::CanChangeCursor() const {
    if (!this->m_typeData->CanUse()) {
        return NO_CURSOR;
    }

    if (!this->m_stats || !this->m_stats->m_iconName || !*this->m_stats->m_iconName) {
        return this->m_typeData->CanChangeCursor();
    }

    char name[40];

    if (this->m_typeData->CanUseNow(nullptr, nullptr, nullptr)) {
        SStrCopy(name, this->m_stats->m_iconName, sizeof(name));
    } else {
        SStrPrintf(name, sizeof(name), "Unable%s", this->m_stats->m_iconName);
    }

    int32_t mode = CursorGetModeFromName(name);
    return mode ? mode : NUM_CURSOR_MODES;
}

// VERIFY: 9f6a62fd manual
// OFFSET: 0x00711AB0
void CGGameObject_C_Type_TransportBase::AddPassenger(CPassenger* passenger, int32_t a2) {
    passenger->m_link.Unlink();
    this->m_passengers.LinkToTail(passenger);

    if (a2 && (passenger->m_passengerFlags & 1) && (!this->m_worldObject || !World::ObjectIsLoaded(this->m_worldObject))) {
        static_cast<CMovementShared*>(passenger)->SetSplineEnabled();
    }

    if (*passenger->m_ownerGuid == CGUnit_C::s_activeMover) {
        MovementSetTransportUpdateTime(this->m_transportUpdateTime);
    }
}

// VERIFY: 2c95d1e2 auto
// OFFSET: 0x007102C0
int32_t CGGameObject_C::GetPageTextLanguage() const {
    return this->GetPropertyValue(CGameObjectDef::GetPropNum(this->m_gameObject->m_typeID, 16));
}

// VERIFY: 1c117624 auto
// OFFSET: 0x00710300
int32_t CGGameObject_C::GetPageTextMaterial() const {
    return this->GetPropertyValue(CGameObjectDef::GetPropNum(this->m_gameObject->m_typeID, 17));
}

// VERIFY: b1bd3768 auto
// OFFSET: 0x0070ED50
void CGGameObject_C::Reenable() {
    CGObject_C::Reenable();

    if (this->m_stats) {
        this->m_typeData->Reenable();
    } else {
        ClntObjMgrHideObject(this->m_obj->m_guid);
    }

    if (this->m_gameObject->m_typeID == 33) {
        if (this->m_nameDesc) {
            PlayerNameDelete(this->m_nameDesc);
        }

        WOWGUID guid = this->m_obj->m_guid;
        this->m_nameDesc = ObjectNameCreate(guid);
    }
}

// VERIFY: d9d6f845 manual
// OFFSET: 0x0070EDD0
UNIT_REACTION CGGameObject_C::ObjectReaction(const CGUnit_C* unit) const {
    CGUnit_C* creator = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_gameObject->m_createdBy, TYPE_UNIT, __FILE__, __LINE__));

    if (creator) {
        return creator->UnitReaction(unit);
    }

    if (this->m_gameObject->m_faction) {
        return CGUnit_C::UnitReaction(this->m_gameObject->m_faction, unit);
    }

    return UNIT_REACTION_NEUTRAL;
}

// VERIFY: 7fd3c3af auto
// OFFSET: 0x0070F850
bool CGGameObject_C::IsTracked() const {
    return CGMinimapFrame::s_trackingInfo && CGMinimapFrame::s_trackingInfo->m_type == 2 && CGMinimapFrame::s_trackingInfo->m_npcFlags == static_cast<uint32_t>(this->m_gameObject->m_typeID);
}

// VERIFY: a1c0c530 auto
// OFFSET: 0x0070F880
void CGGameObject_C::AddWorldDamageText(int32_t amount) {
    if (amount && g_combatDamageCVar->m_intValue && this->m_gameObject->m_typeID == 33) {
        char text[32];
        SStrPrintf(text, sizeof(text), "%d", amount);
        PlayerNameCreateText(this->m_nameDesc, WT_BUILDING_DAMAGE, text, reinterpret_cast<const CImVector*>(&s_buildingDamageColor), nullptr);
    }
}

// VERIFY: 879b3c82 auto
// OFFSET: 0x0070F8E0
void CGGameObject_C::AddWorldHealingText(int32_t amount, const SpellRec* spell) {
    if (amount && g_combatHealingCVar->m_intValue && this->m_gameObject->m_typeID == 33) {
        char text[32];
        SStrPrintf(text, sizeof(text), "+%d", amount);
        PlayerNameCreateText(this->m_nameDesc, WT_BUILDING_HEAL, text, nullptr, nullptr);
    }
}

// VERIFY: c0bf3ae0 manual
// OFFSET: 0x00712400
void CGGameObject_C::LoadBaseObject(GameObjectStats_C* stats) {
    this->m_stats = stats;
    this->m_typeData->PostInitWithStats();

    GameObjectDisplayInfoRec* displayInfo = g_gameObjectDisplayInfoDB.GetRecord(this->m_stats->m_displayID);

    if (displayInfo && displayInfo->m_objectEffectPackageID) {
        void* mem = SMemAlloc(sizeof(CObjectEffectManager), __FILE__, __LINE__, 0);
        this->m_effectManager = mem ? new (mem) CObjectEffectManager() : nullptr;
        this->m_effectManager->Init(displayInfo->m_objectEffectPackageID, this);
    }

    this->CheckForPassiveHighlight();
}

// VERIFY: ac5cb052 manual
// OFFSET: 0x0070D510
void CGGameObject_C_TypeAnimated::UpdateAnimState(uint32_t state) {
    if (static_cast<int32_t>(state) == this->m_animState) {
        return;
    }

    CM2Model* model = nullptr;

    if (this->m_owner->IsObjectModelLoaded()) {
        model = this->m_owner->m_worldModel;

        if (model && this->m_owner->m_gameObject->m_pathProgress == 0xFFFF) {
            int32_t current = this->m_animState;

            if ((current == ANIMSTATE_OPENING && state == ANIMSTATE_CLOSING) || (current == ANIMSTATE_CLOSING && state == ANIMSTATE_OPENING) || (current == ANIMSTATE_DESTROYING && state == ANIMSTATE_REBUILDING) || (current == ANIMSTATE_REBUILDING && state == ANIMSTATE_DESTROYING)) {
                M2BoneSequenceInfo info;
                model->GetBoneSequenceInfo(0xFFFFFFFF, info);

                if (static_cast<int32_t>(info.m_endTime - info.m_startTime) > 0) {
                    uint32_t elapsed = model->m_scene->m_time - info.m_startTime;
                    float progress = static_cast<float>(elapsed) / static_cast<float>(info.m_endTime - info.m_startTime);

                    if (progress <= 1.0f) {
                        this->m_owner->SetPathProgress(1.0f - progress);
                    }
                }
            }
        }
    }

    this->m_animState = state;

    if (model) {
        this->SetSequence(model);
    }
}

// VERIFY: a9620568 auto
// OFFSET: 0x00711B50
void CGGameObject_C_Type_MapObjTransport::PostInitWithStats() {
    this->CreateWorldObj();

    if (this->m_worldObject) {
        MovementAddTransport(this->m_owner);
    }

    uint32_t physicsIndex = CGameObjectDef::GetPropNum(this->m_owner->m_gameObject->m_typeID, 99);
    GameObjectStats* physicsStats = this->m_owner->m_stats;
    int32_t physicsId = (physicsStats && physicsIndex < 24) ? physicsStats->m_propValue[physicsIndex] : 0;
    TransportPhysicsRec* physics = g_transportPhysicsDB.GetRecord(physicsId);

    uint32_t accelRateIndex = CGameObjectDef::GetPropNum(this->m_owner->m_gameObject->m_typeID, 43);
    GameObjectStats* accelRateStats = this->m_owner->m_stats;
    int32_t accelRate = (accelRateStats && accelRateIndex < 24) ? accelRateStats->m_propValue[accelRateIndex] : 0;

    uint32_t moveSpeedIndex = CGameObjectDef::GetPropNum(this->m_owner->m_gameObject->m_typeID, 36);
    GameObjectStats* moveSpeedStats = this->m_owner->m_stats;
    int32_t moveSpeed = (moveSpeedStats && moveSpeedIndex < 24) ? moveSpeedStats->m_propValue[moveSpeedIndex] : 0;

    uint32_t taxiPathIndex = CGameObjectDef::GetPropNum(this->m_owner->m_gameObject->m_typeID, 35);
    GameObjectStats* taxiPathStats = this->m_owner->m_stats;
    uint32_t taxiPathId = (taxiPathStats && taxiPathIndex < 24) ? taxiPathStats->m_propValue[taxiPathIndex] : 0;

    this->m_shipPath.Init(taxiPathId, static_cast<float>(moveSpeed), static_cast<float>(accelRate), physics);
    this->m_shipPath.SetTotalPathTime(this->m_owner->m_gameObject->m_level);
    this->InitializeMovement();

    if (this->m_worldObject) {
        World::ObjectUpdate(this->m_worldObject, this->m_owner->GetMatrix());
    }
}

// VERIFY: 6ae2280a manual
// OFFSET: 0x0070D1E0
void CGGameObject_C_TypeAnimated::SetSequence(CM2Model* model) {
    float speed = 1.0f;
    uint32_t sequence = CGGameObject_C_TypeAnimated::s_animSequenceIds[this->m_animState];

    if (model->IsLoaded(0, 0) && !model->HasSequence(sequence)) {
        switch (sequence) {
            case 146:
                if (!model->HasSequence(148)) {
                    sequence = 147;
                }

                break;

            case 147:
                if (!model->HasSequence(146)) {
                    if (!model->HasSequence(148)) {
                        sequence = 0;
                        goto effects;
                    }

                    sequence = 148;
                    speed = 0.0f;
                }

                break;

            case 148:
                if (!model->HasSequence(146)) {
                    sequence = model->HasSequence(150) ? 150 : 149;
                }

                break;

            case 149:
                if (!model->HasSequence(148)) {
                    if (model->HasSequence(146)) {
                        sequence = 146;
                        speed = 0.0f;
                    } else {
                        sequence = 151;
                    }
                }

                break;
        }

        if (sequence == model->GetBoneSequenceId(0xFFFFFFFF)) {
            return;
        }
    }

effects:
    if (this->m_owner->m_effectManager) {
        uint32_t current = model->GetBoneSequenceId(0xFFFFFFFF);

        if (current != sequence) {
            int32_t exitState = FindObjectEffectStateFromAnimID(current, 0);

            if (exitState) {
                this->m_owner->m_effectManager->ExitState(exitState, 1);
            }

            int32_t enterState = FindObjectEffectStateFromAnimID(sequence, 0);

            if (enterState) {
                this->m_owner->m_effectManager->EnterState(enterState, 1, 0);
            }
        }
    }

    M2SequenceInfo info;
    info.m_radius = 0.0f;
    model->GetSequenceInfo(sequence, 0, info);

    int32_t state = this->m_animState;
    int32_t startTime = 0;
    uint32_t keepTime = 0;

    if (state == ANIMSTATE_OPENING || state == ANIMSTATE_CLOSING || state == ANIMSTATE_DESTROYING || state == ANIMSTATE_REBUILDING) {
        keepTime = (this->m_owner->m_gameObject->m_flags >> 7) & 1;
        uint16_t progress = this->m_owner->m_gameObject->m_pathProgress;

        if (progress != 0xFFFF) {
            startTime = CMath::fint_(static_cast<float>(progress) * 1.5259022e-05f * static_cast<float>(info.m_duration));
            this->m_owner->SetPathProgress(1.0f);
        }
    }

    model->SetBoneSequence(0xFFFFFFFF, sequence, 0xFFFFFFFF, startTime, speed, this->m_animState != ANIMSTATE_SPAWN, 1);

    if (keepTime) {
        uint32_t time = model->m_scene->m_time;
        model->m_lastAnimTime = time ? time : 1;
    } else {
        model->m_lastAnimTime = 0;
    }

    state = this->m_animState;

    if (state != ANIMSTATE_OPEN && state != ANIMSTATE_CLOSED && state != ANIMSTATE_DESTROYED) {
        ClntObjMgrLockObject(this->m_owner->m_obj->m_guid);
    }

    this->m_pendingAnimState = this->m_animState;

    if (speed > 0.0f) {
        this->m_nextAnimTime = FrameTime::s_curTimeMs + static_cast<uint32_t>(static_cast<float>(info.m_duration - startTime) / speed);
    } else {
        this->m_nextAnimTime = FrameTime::s_curTimeMs;
    }
}

// VERIFY: 52c14f17 manual
// OFFSET: 0x00710410
bool CGGameObject_C::IsCreatorOnlyUse() const {
    uint32_t creatorOnlyIndex = CGameObjectDef::GetPropNum(this->m_gameObject->m_typeID, 94);
    GameObjectStats* creatorOnlyStats = this->m_stats;
    return creatorOnlyStats && creatorOnlyIndex < 24 && creatorOnlyStats->m_propValue[creatorOnlyIndex];
}

// VERIFY: 78a5991c manual
// OFFSET: 0x007112A0
bool CGGameObject_C_TypeBase::CanUse() const {
    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return false;
    }

    if (this->m_owner->m_gameObject->m_typeID == 6) {
        if (!this->m_owner->GetLockRec() || this->m_owner->ObjectReaction(player) > UNIT_REACTION_HOSTILE) {
            return false;
        }

        WOWGUID creatorGuid = this->m_owner->m_gameObject->m_createdBy;

        if (!(creatorGuid.m_guid & 0xF000000000000000ULL) && (creatorGuid.m_guid & 0xF07FFFFFFFFFFFFFULL)) {
            CGUnit_C* creator = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(creatorGuid, TYPE_PLAYER, __FILE__, __LINE__));

            if (!creator) {
                return false;
            }

            bool ffa = (creator->m_unit->m_unitFieldBytes2[1] & 0x4) && (!CGPartyInfo::InParty() || (!CGPartyInfo::IsMember(creator->m_obj->m_guid) && !IsGuidInMyRaid(creator->m_obj->m_guid)));

            if (!(creator->m_unit->m_unitFieldBytes2[1] & 0x1) && !creator->IsDuelingWith(player) && !ffa) {
                return false;
            }
        }
    } else if (this->m_owner->ObjectReaction(player) <= UNIT_REACTION_HOSTILE) {
        return false;
    }

    CGGameObject_C* owner = this->m_owner;
    uint16_t dynamicFlags = owner->m_gameObject->m_dynamicFlags;

    if (dynamicFlags & 0x4) {
        return false;
    }

    uint32_t flags = owner->m_gameObject->m_flags;

    if ((flags & 0x11) || (owner->m_modelFlags & 0x100000) || ((flags & 0x4) && !(dynamicFlags & 0x1))) {
        return false;
    }

    if (owner->IsCreatorOnlyUse() && player->m_obj->m_guid != this->m_owner->m_gameObject->m_createdBy) {
        return false;
    }

    return true;
}

// VERIFY: e3ea0361 auto
// OFFSET: 0x0070D7B0
void CGGameObject_C_TypeAnimated::HandleAnimEvent(uint32_t eventId, uint32_t data, const C3Vector& position, uint32_t time) {
    GameObject_C_ProcessAnimEvent(eventId, this->m_owner->m_gameObject->m_displayID, data, position, &this->m_sound);
}

// VERIFY: c5be7d0b manual
// OFFSET: 0x007110B0
CMapEntity* CGGameObject_C::CreateWorldObject(uint64_t guid, int32_t a4, uint32_t doodadSetCount, const int16_t* doodadSets) {
    const char* name = this->GetModelFileNameInternal();

    if (!name) {
        return nullptr;
    }

    C3Vector position = this->m_passenger.GetPosition(this->m_passenger.m_position);
    CMapBaseObj* object = CWorld::ObjectCreate(name, position, this->GetFacing(), a4, 0, guid, doodadSetCount, doodadSets, nullptr, 0);

    if (!object) {
        SysMsgPrintf(SYSMSG_ERROR, 2, "Game object (%d): failed to load: \"%s\"", this->m_obj->m_entryID, name);
    }

    return static_cast<CMapEntity*>(object);
}

// VERIFY: b54abd63 auto
// OFFSET: 0x00711A10
void CGGameObject_C_Type_WorldObj::CreateWorldObj() {
    switch (this->m_owner->m_gameObject->m_typeID) {
        case TYPE_TRANSPORT:
        case TYPE_MO_TRANSPORT:
        case TYPE_TRAPDOOR:
            this->m_worldObject = this->m_owner->CreateWorldObject(this->m_owner->m_obj->m_guid.m_guid, 0, 0, nullptr);

            if (this->m_worldObject && this->m_owner->m_gameObject->m_typeID == TYPE_TRANSPORT) {
                World::ObjectSetMoveable(this->m_worldObject, 1);
            }

            break;

        case TYPE_DESTRUCTIBLE_BUILDING:
            return;

        default:
            this->m_worldObject = this->m_owner->CreateWorldObject(0, 0, 0, nullptr);
            break;
    }
}

// VERIFY: 6459dfad auto
// OFFSET: 0x0070FFD0
void CGGameObject_C_Type_TransportBase::Disable() {
    CGCamera* camera = CGWorldFrame::GetActiveCamera();
    uint32_t time = OsGetAsyncTimeMs();

    for (CPassenger* passenger = this->m_passengers.Head(); passenger; ) {
        CPassenger* next = this->m_passengers.Next(passenger);

        if (camera->m_targetGUID == *passenger->m_ownerGuid) {
            camera->MakeRelativeTo(WOWGUID());
        }

        if (passenger->m_passengerFlags & 0x1) {
            CMovement_C* movement = static_cast<CMovement_C*>(passenger);
            movement->ClearSplineEnabled();

            if (*passenger->m_ownerGuid == CGUnit_C::s_activeMover) {
                movement->OnFallLocal(time);
            } else {
                movement->TryStartFallingLocal();
            }

            movement->ForceSetTransport(WOWGUID(), 255, 1);
        } else {
            passenger->SetTransport(WOWGUID());
        }

        passenger = next;
    }

    if (this->m_worldObject) {
        MovementRemoveTransport(this->m_owner);
    }

    this->DestroyWorldObj();
}

// VERIFY: 3191030f auto
// OFFSET: 0x007100D0
void CGGameObject_C_Type_MapObjTransport::InitializeMovement() {
    uint32_t time = OsGetAsyncTimeMs();
    CGGameObject_C* owner = this->m_owner;
    uint32_t index = CGameObjectDef::GetPropNum(owner->m_gameObject->m_typeID, 128);
    GameObjectStats* stats = owner->m_stats;

    if (stats && index < 24 && stats->m_propValue[index]) {
        uint32_t offsetTime = time + this->m_owner->m_transportTimeOffset;
        this->m_shipPath.SetTimeOffset(offsetTime, this->m_owner->m_gameObject->m_pathProgress * 0.000015259022f);
        CGGameObjectData* gameObject = this->m_owner->m_gameObject;

        if (gameObject->m_dynamicFlags & 0x10) {
            this->m_shipPath.WaitAtCurrentLocation(offsetTime);
        } else {
            this->m_shipPath.WaitAtNextPausePoint(offsetTime, gameObject->m_state == 1);
        }
    }

    this->UpdateMovement(time, 0);
}

// VERIFY: c0432d12 manual
// OFFSET: 0x0070D7E0
void CGGameObject_C_TypeAnimated::HandleAnimFinished() {
    int32_t state = this->m_animState;
    int32_t pending = this->m_pendingAnimState;
    this->m_pendingAnimState = -1;

    switch (state) {
        case ANIMSTATE_SPAWN:
        case ANIMSTATE_CUSTOM0:
        case ANIMSTATE_CUSTOM1:
        case ANIMSTATE_CUSTOM2:
        case ANIMSTATE_CUSTOM3:
            if (!(this->m_owner->m_modelFlags & 0x100000)) {
                this->UpdateState(this->m_owner->m_displayState, this->m_owner->m_displayState);
            }

            break;

        case ANIMSTATE_CLOSED:
        case ANIMSTATE_OPEN:
        case ANIMSTATE_DESTROYED:
            this->SetSequence(this->m_owner->m_worldModel);
            break;

        case ANIMSTATE_OPENING:
            this->UpdateAnimState(ANIMSTATE_OPEN);
            break;

        case ANIMSTATE_CLOSING:
        case ANIMSTATE_REBUILDING:
            this->UpdateAnimState(ANIMSTATE_CLOSED);
            break;

        case ANIMSTATE_DESTROYING:
            this->UpdateAnimState(ANIMSTATE_DESTROYED);
            break;
    }

    if (pending != -1 && pending != ANIMSTATE_OPEN && pending != ANIMSTATE_CLOSED && pending != ANIMSTATE_DESTROYED) {
        ClntObjMgrUnlockObject(this->m_owner->m_obj->m_guid, __FILE__, __LINE__);
    }
}

// VERIFY: 96d91adb auto
// OFFSET: 0x0070BF70
static void GameObject_C_PlayAnimatedSound(uint32_t displayId, int32_t index, const C3Vector& position, SE2SoundKitObject* sound) {
    if (index == -1) {
        return;
    }

    GameObjectDisplayInfoRec* displayInfo = g_gameObjectDisplayInfoDB.GetRecord(displayId);

    if (!displayInfo) {
        return;
    }

    int32_t soundKitId = displayInfo->m_sound[index];
    SE2SoundKitProperties properties;

    if (!SI2::IsSoundKitLooping(soundKitId)) {
        properties.ResetToDefaults();
        properties.m_maxAudibleBehavior = 0;
        SI2::PlaySoundKit(soundKitId, &position, nullptr, &properties, 0, nullptr, 1, 0);
        return;
    }

    if (!sound || SI2::IsPlaying(sound) || SI2::IsTooCloseToExistingSoundKit(soundKitId, &position)) {
        return;
    }

    properties.ResetToDefaults();
    properties.m_loopMode = 1;
    properties.m_streaming = true;
    properties.m_software = 1;
    properties.m_maxAudibleBehavior = 0;
    SI2::PlaySoundKit(soundKitId, &position, sound, &properties, 0, nullptr, 1, 0);
}

// VERIFY: cfabb88c auto
// OFFSET: 0x0070C050
void GameObject_C_ProcessAnimEvent(uint32_t eventId, uint32_t displayId, uint32_t data, const C3Vector& position, SE2SoundKitObject* sound) {
    switch (eventId) {
        case '0OG$':
            GameObject_C_PlayAnimatedSound(displayId, 0, position, sound);
            break;

        case '1OG$':
            GameObject_C_PlayAnimatedSound(displayId, 1, position, sound);
            break;

        case '2OG$':
            GameObject_C_PlayAnimatedSound(displayId, 2, position, sound);
            break;

        case '3OG$':
            GameObject_C_PlayAnimatedSound(displayId, 3, position, sound);
            break;

        case '4OG$':
            GameObject_C_PlayAnimatedSound(displayId, 4, position, sound);
            break;

        case '5OG$':
            GameObject_C_PlayAnimatedSound(displayId, 5, position, sound);
            break;

        case '0CG$':
            GameObject_C_PlayAnimatedSound(displayId, 6, position, sound);
            break;

        case '1CG$':
            GameObject_C_PlayAnimatedSound(displayId, 7, position, sound);
            break;

        case '2CG$':
            GameObject_C_PlayAnimatedSound(displayId, 8, position, sound);
            break;

        case '3CG$':
            GameObject_C_PlayAnimatedSound(displayId, 9, position, sound);
            break;

        case 'DNS$':
        case 'OSD$':
            SI2::PlaySoundKit(data, &position, nullptr, nullptr, 0, nullptr, 1, 0);
            break;

        case 'LSD$':
            if (sound && !SI2::IsPlaying(sound) && !SI2::IsTooCloseToExistingSoundKit(data, &position)) {
                SE2SoundKitProperties properties;
                properties.ResetToDefaults();
                properties.m_fadeInTime = 1.0f;
                properties.m_fadeOutTime = 1.0f;
                properties.m_loopMode = 1;
                properties.m_maxAudibleBehavior = 0;
                properties.m_streaming = true;
                properties.m_software = 1;
                SI2::PlaySoundKit(data, &position, sound, &properties, 0, nullptr, 1, 0);
            }

            break;

        case 'KHS$': {
            SpellEffectCameraShakesRec* shakes = g_spellEffectCameraShakesDB.GetRecord(data);

            if (shakes) {
                for (int32_t i = 0; i < 3; i++) {
                    CGWorldFrame::GetActiveCamera()->AddShakeById(shakes->m_cameraShake[i], &position);
                }
            }

            break;
        }
    }
}

// VERIFY: 03222568 manual
// OFFSET: 0x007134A0
void CGGameObject_C_Type_MapObjTransport::UpdateMovement(uint32_t time, uint32_t elapsed) {
    C3Vector position(0.0f, 0.0f, 0.0f);
    int32_t mapId = -1;
    int32_t initial = this->m_currentState == -1;
    uint32_t pathTime = this->m_owner->m_transportTimeOffset + time;
    uint32_t sequence;
    float facing;
    uint32_t leg;
    float roll;
    float pitch;
    this->m_shipPath.GetPosition(pathTime, elapsed, &mapId, &sequence, &position, &facing, &leg, &roll, &pitch, 0);

    if (mapId != static_cast<int32_t>(ClntObjMgrGetMapID())) {
        CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

        if (mover && mover->GetTransportGUID() == this->m_owner->m_obj->m_guid.m_guid) {
            MovementSetTransportUpdateTime(this->m_transportUpdateTime);
        }

        return;
    }

    this->m_pathTime = pathTime;
    this->m_transportUpdateTime = this->GetSequenceTimeFromRealTime(pathTime);
    this->UpdateCurrentVelocity(elapsed, position, nullptr);
    this->m_position = position;
    C44Matrix& matrix = this->m_owner->m_matrix;
    matrix.d3 = 1.0f;
    matrix.c2 = 1.0f;
    matrix.b1 = 1.0f;
    matrix.a0 = 1.0f;
    matrix.d2 = 0.0f;
    matrix.d1 = 0.0f;
    matrix.d0 = 0.0f;
    matrix.c3 = 0.0f;
    matrix.c1 = 0.0f;
    matrix.c0 = 0.0f;
    matrix.b3 = 0.0f;
    matrix.b2 = 0.0f;
    matrix.b0 = 0.0f;
    matrix.a3 = 0.0f;
    matrix.a2 = 0.0f;
    matrix.a1 = 0.0f;
    matrix.d0 = this->m_position.x;
    matrix.d1 = this->m_position.y;
    matrix.d2 = this->m_position.z;
    matrix.RotateAroundZ(facing);
    matrix.RotateAroundY(pitch);
    matrix.RotateAroundX(roll);
    CGGameObject_C* owner = this->m_owner;
    C4Quaternion rotation;
    rotation.x = 0.0f;
    rotation.y = 0.0f;
    rotation.z = 0.0f;
    rotation.w = 0.0f;
    rotation.FromRotationMatrix(owner->m_matrix);
    reinterpret_cast<C4QuaternionCompressed&>(owner->m_passenger.m_facing) = rotation;
    owner->m_passenger.m_passengerFlags |= 0x2;

    if (this->m_worldObject) {
        World::ObjectUpdate(this->m_worldObject, matrix);

        if (!this->m_worldObjectReady && World::ObjectIsLoaded(this->m_worldObject)) {
            World::ObjectSetSequenceFinishedCallback(this->m_worldObject, CGGameObject_C_Type_WorldObj::SequenceFinishedHandler, 0, 0);
            this->m_worldObjectReady = 1;
        }

        if (this->m_worldObjectReady && this->m_currentState != static_cast<int32_t>(sequence)) {
            World::ObjectSetSequence(this->m_worldObject, sequence, 0, 0);
            this->m_currentState = sequence;
            this->UpdateObjectEffectState(sequence);
        }
    }

    int32_t moved;
    this->MovePassengers(&moved);
    uint32_t previousLeg = this->m_currentLeg;

    if (previousLeg == leg) {
        return;
    }

    this->m_currentLeg = leg;

    if (!moved || initial) {
        return;
    }

    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGWorldFrame::GetActiveCamera()->m_targetGUID, TYPE_UNIT, __FILE__, __LINE__));
    AsyncFileReadSetProgressCallback(LoadingScreenAsyncCallback, nullptr);
    World::SetLoadProgressCallback(LoadingScreenWorldCallback, nullptr);
    LoadingScreenEnableShipLeg(this->m_owner->m_obj->m_entryID, previousLeg);
    World::Preload(target->GetPosition(), 0);
    CGObject_C::UpdateAllWorldObjects();
    LoadingScreenDisable();
    AsyncFileReadSetProgressCallback(nullptr, nullptr);
    World::SetLoadProgressCallback(nullptr, nullptr);
}

// VERIFY: 901b1381 auto
// OFFSET: 0x00714240
uint32_t CGGameObject_C_Type_MapObjTransport::GetSequenceTimeFromRealTime(uint32_t time) const {
    return this->m_shipPath.GetPathTime(time);
}

// VERIFY: 83b573d2 manual
// OFFSET: 0x007132E0
void CGGameObject_C_Type_TransportBase::MovePassengers(int32_t* moved) {
    *moved = 0;
    C44Matrix& matrix = this->m_owner->m_matrix;
    int32_t isWMO = 0;

    if (this->m_worldObject) {
        World::ObjectUpdate(this->m_worldObject, matrix);
        isWMO = World::ObjectIsWMO(this->m_worldObject);
    }

    WOWGUID cameraTarget = CGWorldFrame::GetActiveCamera()->m_targetGUID;
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    for (CPassenger* passenger = this->m_passengers.Head(); passenger; passenger = this->m_passengers.Next(passenger)) {
        CMovementShared* movement = static_cast<CMovementShared*>(passenger);

        if (this->m_worldObjectReady && (passenger->m_passengerFlags & 0x1) && (movement->m_flags & 0x8000000)) {
            movement->ClearSplineEnabled();
        }

        if (*passenger->m_ownerGuid == cameraTarget) {
            *moved = 1;
        }

        CGObject_C* object = static_cast<CGObject_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPE_OBJECT, __FILE__, __LINE__));
        uint64_t guid = passenger->m_ownerGuid->m_guid;
        uint32_t high = static_cast<uint32_t>(guid >> 32);

        if ((high & 0xF0F00000) == 0xF0500000 || (!(high & 0xF0000000) && (guid & 0xF07FFFFFFFFFFFFFull))) {
            CVehicle_C* vehicle = static_cast<CGUnit_C*>(object)->m_vehicle;

            if (vehicle && vehicle->m_vehicleRec) {
                vehicle->BuildWorldMatrix(&matrix);
            }
        }

        object->UpdateWorldObject(object != mover && isWMO);
        object->SetFrameOfReference(&matrix);
    }

    if (mover && mover->GetTransportGUID() == this->m_owner->m_obj->m_guid.m_guid) {
        MovementSetTransportUpdateTime(this->m_transportUpdateTime);
    }
}

// VERIFY: 142d2a6b manual
// OFFSET: 0x0070EC10
C3Vector CGGameObject_C_Type_DestructibleBuilding::GetNamePosition() const {
    uint32_t state = (this->m_owner->m_gameObject->m_flags >> 9) & 0x7;
    CAaBox extents;
    extents.b = C3Vector(0.0f, 0.0f, 0.0f);
    extents.t = C3Vector(0.0f, 0.0f, 0.0f);
    World::ObjectGetExtents(this->m_states[state].m_object, &extents);
    C3Vector position = this->GetPosition();
    position.z = extents.t.y - extents.b.y + position.z;
    return position;
}

// VERIFY: da097b7f auto
// OFFSET: 0x0070EEF0
uint32_t CGGameObject_C::GetProperty(int32_t prop) const {
    uint32_t index = CGameObjectDef::GetPropNum(this->m_gameObject->m_typeID, prop);

    if (this->m_stats && index < 24) {
        return this->m_stats->m_propValue[index];
    }

    return 0;
}

// VERIFY: 9dd592e6 manual
// OFFSET: 0x0070F160
bool CGGameObject_C::IsLocked(int32_t* spellId, int32_t* skillValue, int32_t* requiredSkill, CGItem_C** key, uint32_t* entry) const {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return false;
    }

    LockRec* lock = this->GetLockRec();

    if (!lock) {
        return false;
    }

    bool locked = false;

    for (uint32_t i = 0; i < 8; i++) {
        int32_t type = lock->m_type[i];

        if (!type) {
            continue;
        }

        if (type == 2) {
            locked = true;

            if (!this->IsValidOpenAction(lock->m_action[i])) {
                continue;
            }

            for (uint32_t j = 0; j < s_openLockSpells.m_count; j++) {
                SpellRec spell;
                g_spellDB.GetRecord(s_openLockSpells.m_data[j], &spell);

                for (uint32_t effect = 0; effect < 3; effect++) {
                    if (spell.m_effect[effect] != SPELL_EFFECT_OPEN_LOCK || spell.m_effectMiscValue[effect] != lock->m_index[i]) {
                        continue;
                    }

                    int32_t required = lock->m_skill[i] ? lock->m_skill[i] : 5 * this->m_gameObject->m_level;
                    int32_t minPoints;
                    int32_t maxPoints;
                    Spell_C_GetMinMaxPoints(&spell, effect, &minPoints, &maxPoints, 0, 0, 0, 0);

                    if (spellId) {
                        *spellId = s_openLockSpells.m_data[j];
                    }

                    if (skillValue) {
                        *skillValue = minPoints;
                    }

                    if (requiredSkill) {
                        *requiredSkill = required;
                    }

                    if (minPoints >= required) {
                        if (entry) {
                            *entry = i;
                        }

                        return false;
                    }
                }

                if (Spell_C_IsTargeting()) {
                    int32_t targetingSpell = Spell_C_GetTargetingSpell();
                    SpellRec targeting;
                    g_spellDB.GetRecord(targetingSpell, &targeting);

                    for (uint32_t effect = 0; effect < 3; effect++) {
                        if (targeting.m_effect[effect] != SPELL_EFFECT_OPEN_LOCK || targeting.m_effectMiscValue[effect] != lock->m_index[i]) {
                            continue;
                        }

                        int32_t required = lock->m_skill[i] ? lock->m_skill[i] : 5 * this->m_gameObject->m_level;
                        int32_t minPoints;
                        int32_t maxPoints;
                        Spell_C_GetMinMaxPoints(&targeting, effect, &minPoints, &maxPoints, 0, 0, 0, 0);

                        if (spellId) {
                            *spellId = targetingSpell;
                        }

                        if (skillValue) {
                            *skillValue = minPoints;
                        }

                        if (requiredSkill) {
                            *requiredSkill = required;
                        }

                        if (minPoints >= required) {
                            if (entry) {
                                *entry = i;
                            }

                            return false;
                        }
                    }
                }
            }
        } else if (type == 3) {
            locked = true;

            if (!this->IsValidOpenAction(lock->m_action[i])) {
                continue;
            }

            SpellRec spell;
            g_spellDB.GetRecord(lock->m_index[i], &spell);

            for (uint32_t effect = 0; effect < 3; effect++) {
                if (spell.m_effect[effect] == SPELL_EFFECT_OPEN_LOCK) {
                    if (spellId) {
                        *spellId = spell.m_id;
                    }

                    if (entry) {
                        *entry = i;
                    }

                    return false;
                }
            }
        } else if (type == 1) {
            locked = true;

            if (!this->IsValidOpenAction(lock->m_action[i])) {
                continue;
            }

            CGItem_C* item = player->m_bag.FindItemOfType(lock->m_index[i], 0);

            if (item) {
                if (spellId) {
                    *spellId = item->GetEnchantmentSpell(33);
                }

                if (key) {
                    *key = item;
                }

                if (entry) {
                    *entry = i;
                }

                return false;
            }
        }
    }

    return locked;
}

// VERIFY: a7ddafb5 manual
// OFFSET: 0x0070F580
int32_t CGGameObject_C::CanHighlight() const {
    if (!this->m_typeData->CanHighlight()) {
        return 0;
    }

    int32_t questId = static_cast<int32_t>(this->GetPropertyValue(CGameObjectDef::GetPropNum(this->m_gameObject->m_typeID, 20)));

    if (questId <= 0) {
        return 1;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    for (uint32_t i = 0; i < 25; i++) {
        if (player->GetQuestLogEntry(i)->m_questId == static_cast<uint32_t>(questId)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: cf65be3a auto
// OFFSET: 0x0070F630
int32_t CGGameObject_C::FloatingTooltip() const {
    return this->GetPropertyValue(CGameObjectDef::GetPropNum(this->m_gameObject->m_typeID, 19)) != 0;
}

// VERIFY: 3b7ec338 manual
// OFFSET: 0x0070F680
void CGGameObject_C::UseObject() {
    GAME_ERROR_TYPE error;
    float distance;
    const char* errorText = nullptr;

    if (this->m_typeData->CanUseNow(&error, &distance, &errorText)) {
        this->m_typeData->Use();
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_GAME_OBJ_REPORT_USE));
        msg.Put(this->m_obj->m_guid.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
        return;
    }

    if (error == ERR_USE_TOO_FAR && this->m_gameObject->m_typeID != 17) {
        WOWGUID guid = this->m_obj->m_guid;
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (player->CTMInteractGO(guid, distance)) {
            return;
        }
    }

    if (errorText) {
        CGGameUI::DisplayError(static_cast<UIErrors>(error), errorText);
    } else {
        CGGameUI::DisplayError(static_cast<UIErrors>(error));
    }
}

// VERIFY: 3d05c647 auto
// OFFSET: 0x0070F940
C3Vector CGGameObject_C::GetNamePosition() const {
    if (this->m_gameObject->m_typeID == 33) {
        return static_cast<CGGameObject_C_Type_DestructibleBuilding*>(this->m_typeData)->GetNamePosition();
    }

    C3Vector position = this->GetPosition();
    position.z = this->m_height * this->m_scale * 1.25f + position.z;
    return position;
}

// VERIFY: 114aa488 auto
// OFFSET: 0x0070F9B0
int32_t CGGameObject_C_TypeBase::CanChangeCursor() const {
    bool canUse = this->CanUseNow(nullptr, nullptr, nullptr);
    LockRec* lock = this->m_owner->GetLockRec();

    if (lock) {
        LockTypeRec* lockType = g_lockTypeDB.GetRecord(lock->m_index[0]);

        if (lockType && lockType->m_cursorName && *lockType->m_cursorName) {
            char cursor[32];

            if (lockType->m_id == 1 || canUse) {
                SStrCopy(cursor, lockType->m_cursorName, sizeof(cursor));
            } else {
                SStrPrintf(cursor, sizeof(cursor), "Unable%s", lockType->m_cursorName);
            }

            return CursorGetModeFromName(cursor);
        }
    }

    return canUse ? INTERACT_CURSOR : INTERACT_ERROR_CURSOR;
}

// VERIFY: 7a1f42bf manual
// OFFSET: 0x0070FA70
int32_t CGGameObject_C_TypeBase::CheckRange() const {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    float range = this->m_interactDistance;
    int32_t spellId = 0;

    if (this->m_owner->IsLocked(&spellId, nullptr, nullptr, nullptr, nullptr) || !spellId) {
        if (this->m_owner->m_displayState == 2) {
            return 0;
        }
    } else {
        float minRange;
        Spell_C::GetSpellRange(player, spellId, &minRange, &range, nullptr);
    }

    CGGameObject_C* owner = this->m_owner;

    if (owner->m_gameObject->m_typeID == 8) {
        C3Vector position = owner->GetPosition();
        return range * range >= player->GetDistanceToPos(position);
    }

    CM2Model* model = owner->m_worldModel;
    CAaBox box;

    if (!owner->m_gameObject->m_typeID) {
        M2BoneSequenceInfo info;
        model->GetBoneSequenceInfo(0xFFFFFFFF, info);
        M2SequenceInfo sequence;
        sequence.Clear();
        model->GetSequenceInfo(info.m_sequenceId, info.m_variationIndex, sequence);
        box = sequence.m_bounds;
    } else {
        GameObjectDisplayInfoRec* displayInfo = owner->m_stats ? g_gameObjectDisplayInfoDB.GetRecord(owner->m_stats->m_displayID) : nullptr;

        if (displayInfo) {
            box.b = C3Vector(displayInfo->m_geoBoxMin[0], displayInfo->m_geoBoxMin[1], displayInfo->m_geoBoxMin[2]);
            box.t = C3Vector(displayInfo->m_geoBoxMax[0], displayInfo->m_geoBoxMax[1], displayInfo->m_geoBoxMax[2]);
        } else if (model->IsLoaded(0, 0)) {
            box = model->GetBoundingBox();
        } else {
            return 0;
        }
    }

    float scale = this->m_owner->m_obj->m_scale;
    box *= scale;
    box.b.x = box.b.x - range;
    box.b.y = box.b.y - range;
    box.b.z = box.b.z - range;
    box.t.x = box.t.x + range;
    box.t.y = box.t.y + range;
    box.t.z = range + box.t.z;

    C44Matrix matrix = this->m_owner->GetMatrix();
    matrix.Scale(1.0f / scale);
    C44Matrix inverse = matrix.AffineInverse();
    C3Vector local = player->GetPosition() * inverse;
    return box.ContainsPointExclusive(local);
}

// VERIFY: ab880379 auto
// OFFSET: 0x0070FD10
void CGGameObject_C_TypeAnimated::OnUpdateAnimProgress() {
    CGGameObject_C* owner = this->m_owner;

    if (!(owner->m_modelFlags & 0x20000) && owner->IsObjectModelLoaded() && this->m_owner->m_worldModel) {
        this->SetSequence(this->m_owner->m_worldModel);
    }
}

// VERIFY: 3078ec2b auto
// OFFSET: 0x00710280
int32_t CGGameObject_C::GetPageTextID(void (*callback)(int32_t, const WOWGUID&, void*, bool)) const {
    return this->GetPropertyValue(CGameObjectDef::GetPropNum(this->m_gameObject->m_typeID, 15));
}

// VERIFY: 78d5d80c auto
// OFFSET: 0x00710340
bool CGGameObject_C::IsUsableInCombat() const {
    return !this->GetPropertyValue(CGameObjectDef::GetPropNum(this->m_gameObject->m_typeID, 62));
}

// VERIFY: 1e16ed95 auto
// OFFSET: 0x00710390
bool CGGameObject_C::IsUsableMounted() const {
    int8_t type = this->m_gameObject->m_typeID;

    if (type == 19) {
        return true;
    }

    if (type == 32) {
        return false;
    }

    return this->GetProperty(91) != 0;
}

// VERIFY: b8153735 auto
// OFFSET: 0x007103C0
bool CGGameObject_C::IsUsableWhileImmune() const {
    int8_t type = this->m_gameObject->m_typeID;

    if (type == 3) {
        return false;
    }

    return !this->GetPropertyValue(CGameObjectDef::GetPropNum(type, 57));
}

// VERIFY: 2c6373c2 auto
// OFFSET: 0x007108D0
void CGGameObject_C_Type_MeetingStone::AddToolTipInfo(CGTooltip* tooltip) {
    CGGameObject_C* owner = this->m_owner;
    AreaTableRec* area = g_areaTableDB.GetRecord(owner->GetPropertyValue(CGameObjectDef::GetPropNum(owner->m_gameObject->m_typeID, 53)));

    if (area) {
        tooltip->AddLine(area->m_areaName, nullptr, 0);
    }
}

// VERIFY: 76e49e4b manual
// OFFSET: 0x00711140
void CGGameObject_C::OnRightClick() {
    if (!this->m_typeData->CanUse()) {
        return;
    }

    CGGameUI::CloseLoot(true, false, false);
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->SetAutoLoot(CGGameUI::IsAutoLooting());
    }

    this->UseObject();
}

// VERIFY: 14267f07 manual
// OFFSET: 0x007111A0
void CGGameObject_C::RefreshInteractIcon() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    if (this->ObjectReaction(player) >= UNIT_REACTION_NEUTRAL && this->IsQuestGiver()) {
        player->UpdateQuestStatus(this);
    } else {
        this->m_questStatus = 0;
        this->m_interactIconType = 0;
        this->RemoveInteractIcon();
    }
}

// VERIFY: 4f272c71 manual
// OFFSET: 0x00711470
bool CGGameObject_C_TypeBase::CanUseNow(GAME_ERROR_TYPE* error, float* distance, const char** errorText) const {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || player->IsDeadOrGhost()) {
        if (error) {
            *error = static_cast<GAME_ERROR_TYPE>(ERR_PLAYER_DEAD);
        }

        return false;
    }

    if (!CGGameUI::m_hasControl) {
        if (error) {
            *error = static_cast<GAME_ERROR_TYPE>(ERR_CLIENT_LOCKED_OUT);
        }

        return false;
    }

    if (player->m_unit->m_unitFieldFlags & 0x40000) {
        if (error) {
            int32_t mechanic;
            Spell_C_CancelsStun(player, nullptr, &mechanic);

            if (mechanic && errorText) {
                *errorText = g_spellMechanicDB.GetRecord(mechanic)->m_stateName;
                *error = static_cast<GAME_ERROR_TYPE>(ERR_USE_PREVENTED_BY_MECHANIC_S);
            } else {
                *error = static_cast<GAME_ERROR_TYPE>(ERR_GENERIC_STUNNED);
            }
        }

        return false;
    }

    if (!this->m_owner->GetLockRec()) {
        if (player->IsMounted() && !this->m_owner->IsUsableMounted() && !player->CanAutoDismount()) {
            if (error) {
                *error = static_cast<GAME_ERROR_TYPE>(ERR_NOT_WHILE_MOUNTED);
            }

            return false;
        }

        if (player->IsShapeShifted()) {
            SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(player->GetShapeshiftForm());

            if ((!form || !(form->m_flags & 0x8)) && !player->CanAutoUnshift()) {
                if (error) {
                    *error = static_cast<GAME_ERROR_TYPE>(ERR_NOT_WHILE_SHAPESHIFTED);
                }

                return false;
            }
        }
    }

    if ((player->m_unit->m_unitFieldFlags & 0x80000) && !this->m_owner->IsUsableInCombat()) {
        if (error) {
            *error = static_cast<GAME_ERROR_TYPE>(ERR_NOT_IN_COMBAT);
        }

        return false;
    }

    if ((player->m_unit->m_unitFieldFlags & 0x80000000) && !this->m_owner->IsUsableWhileImmune()) {
        if (error) {
            *error = static_cast<GAME_ERROR_TYPE>(ERR_USE_CANT_IMMUNE);
        }

        return false;
    }

    CGGameObject_C* owner = this->m_owner;

    if ((owner->m_gameObject->m_flags & 0x2) && owner->IsLocked(nullptr, nullptr, nullptr, nullptr, nullptr)) {
        if (error) {
            *error = static_cast<GAME_ERROR_TYPE>(this->m_useErrorType);
        }

        return false;
    }

    if (distance) {
        *distance = this->m_interactDistance;
    }

    bool result = this->CheckRange() != 0;

    if (!result && error) {
        *error = static_cast<GAME_ERROR_TYPE>(ERR_USE_TOO_FAR);
    }

    return result;
}

// VERIFY: b1cd0162 manual
// OFFSET: 0x007116C0
bool CGGameObject_C_TypeBase::Use() {
    int32_t spellId = 0;
    CGItem_C* key = nullptr;
    uint32_t entry;

    if (this->m_owner->IsLocked(&spellId, nullptr, nullptr, &key, &entry)) {
        LockRec* lock = this->m_owner->GetLockRec();
        CGGameObject_C* owner = this->m_owner;

        if (!owner->IsValidOpenAction(lock->m_action[0])) {
            if (owner->m_gameObject->m_typeID == 3 && !owner->m_displayState) {
                CGGameUI::DisplayError(ERR_CHEST_IN_USE);
            }

            return false;
        }

        if (lock->m_type[0] != 1) {
            if (lock->m_type[0] == 2) {
                LockTypeRec* lockType = g_lockTypeDB.GetRecord(lock->m_index[0]);
                int32_t required = lock->m_skill[0] ? lock->m_skill[0] : 5 * owner->m_gameObject->m_level;

                if (spellId) {
                    CGGameUI::DisplayError(ERR_USE_LOCKED_WITH_SPELL_KNOWN_SI, lockType ? lockType->m_name : "UNKNOWN", required);
                } else {
                    CGGameUI::DisplayError(ERR_USE_LOCKED_WITH_SPELL_S, lockType ? lockType->m_name : "UNKNOWN");
                }
            } else if (lock->m_type[0] == 3) {
                SpellRec spell;

                if (g_spellDB.GetRecord(lock->m_index[0], &spell)) {
                    CGGameUI::DisplayError(ERR_USE_LOCKED_WITH_SPELL_S, spell.m_name);
                }
            } else {
                CGGameUI::DisplayError(ERR_USE_CANT_OPEN);
            }

            return false;
        }

        WOWGUID guid = owner->m_obj->m_guid;
        ItemStats_C* stats = g_itemDBCache.GetRecord(lock->m_index[0], guid, nullptr, nullptr, false);

        if (stats) {
            CGGameUI::DisplayError(ERR_USE_LOCKED_WITH_ITEM_S, stats->GetName(0));
        }

        return false;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (spellId) {
        WOWGUID none = WOWGUID();

        if (!Spell_C::CastSpell(spellId, key, none, false)) {
            return false;
        }

        Spell_C_HandleSpriteClick(this->m_owner, nullptr);
    } else {
        if (!player) {
            return false;
        }

        if (player->m_mountDisplayId > 0 && !(player->m_clientFlags & 0x10000000) && !this->m_owner->IsUsableMounted()) {
            player->Dismount();
        }

        if (player->IsShapeShifted()) {
            SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(player->GetShapeshiftForm());

            if (!form || !(form->m_flags & 0x8)) {
                player->CancelShapeshift();
            }
        }

        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_GAME_OBJ_USE));
        msg.Put(this->m_owner->m_obj->m_guid.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    if (player && player->HasVehicleTransport()) {
        player->CheckVehicleExitOnCast();
    }

    return true;
}

// VERIFY: dd557bea manual
// OFFSET: 0x00712080
bool CGGameObject_C_Type_Ritual::CanUseNow(GAME_ERROR_TYPE* error, float* distance, const char** errorText) const {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return false;
    }

    SpellRec spell;
    CGGameObject_C* owner = this->m_owner;
    int32_t spellId = owner->GetPropertyValue(CGameObjectDef::GetPropNum(owner->m_gameObject->m_typeID, 10));

    if (g_spellDB.GetRecord(spellId, &spell) && (spell.m_attributes & 0x10000000) && player->IsInCombat() && !player->IgnoresAuraState(&spell)) {
        return false;
    }

    const WOWGUID& creator = this->m_owner->m_gameObject->m_createdBy;

    if (!(creator.m_guid & 0xF000000000000000ULL) && (creator.m_guid & 0xF07FFFFFFFFFFFFFULL)) {
        CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(creator, TYPE_PLAYER, __FILE__, __LINE__));

        if (!caster || (!player->IsInMyPartyOrRaid(caster) && !player->IsInMyParty(caster))) {
            return false;
        }
    }

    return CGGameObject_C_TypeBase::CanUseNow(error, distance, errorText);
}

// VERIFY: 27f027c7 auto
// OFFSET: 0x007121E0
int32_t CGGameObject_C_Type_MeetingStone::CheckUseRange(uint32_t level) const {
    CGGameObject_C* owner = this->m_owner;

    if (level < owner->GetPropertyValue(CGameObjectDef::GetPropNum(owner->m_gameObject->m_typeID, 51))) {
        return false;
    }

    owner = this->m_owner;
    return level <= owner->GetPropertyValue(CGameObjectDef::GetPropNum(owner->m_gameObject->m_typeID, 52));
}

// VERIFY: 3fa35c2b manual
// OFFSET: 0x00712270
bool CGGameObject_C_Type_MeetingStone::Use() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return false;
    }

    if (!CGPartyInfo::m_members[0] && !CGRaidInfo::m_numMembers) {
        CGGameUI::DisplayError(ERR_MEETING_STONE_NEED_PARTY);
        return false;
    }

    if (!CGGameUI::m_lockedTarget) {
        CGGameUI::DisplayError(ERR_GENERIC_NO_TARGET);
        return false;
    }

    if (!this->CheckUseRange(player->m_unit->m_unitFieldLevel)) {
        CGGameUI::DisplayError(ERR_MEETING_STONE_INVALID_LEVEL);
        return false;
    }

    if (!CGPartyInfo::IsMember(CGGameUI::m_lockedTarget) && !IsGuidInMyRaid(CGGameUI::m_lockedTarget)) {
        CGGameUI::DisplayError(ERR_MEETING_STONE_TARGET_NOT_IN_PARTY);
        return false;
    }

    uint32_t level;
    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGGameUI::m_lockedTarget, TYPE_UNIT, __FILE__, __LINE__));

    if (target) {
        level = target->m_unit->m_unitFieldLevel;
    } else {
        WOWGUID guid = CGGameUI::m_lockedTarget;
        CGPartyMemberState* stats = CGPartyInfo::GetRemoteStats(guid);

        if (!stats) {
            stats = CGRaidInfo::GetRemoteStats(guid);
        }

        if (!stats || !stats->m_level || !(stats->m_flags & 0x1)) {
            CGGameUI::DisplayError(ERR_MEETING_STONE_NOT_FOUND);
            return false;
        }

        level = stats->m_level;
    }

    if (this->CheckUseRange(level)) {
        return CGGameObject_C_TypeBase::Use();
    }

    CGGameUI::DisplayError(ERR_MEETING_STONE_TARGET_INVALID_LEVEL);
    return false;
}

// VERIFY: 0015bde7 auto
// OFFSET: 0x007124A0
void CGGameObject_C::UpdateInteractIcon() {
    this->CGObject_C::UpdateInteractIcon();
    this->CheckForPassiveHighlight();
}

// VERIFY: cf171b41 auto
// OFFSET: 0x00713130
void CGGameObject_C::PostReenable() {
    this->m_passenger.Reenable();
    this->CGObject_C::PostReenable();

    if (this->m_stats) {
        this->m_typeData->PostReenable();
        this->CheckForPassiveHighlight();
        return;
    }

    WOWGUID guid = this->m_obj->m_guid;
    GameObjectStats_C* stats = g_gameObjectDBCache.GetRecord(this->m_obj->m_entryID, guid, GameObjectStatsCallback, this, false);

    if (stats) {
        this->LoadBaseObject(stats);
    }
}
