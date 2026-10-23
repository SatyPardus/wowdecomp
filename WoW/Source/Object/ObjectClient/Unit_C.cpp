#include "WoW/Source/Magic/Types.h"
#include "Unit_C.h"
#include <cmath>
#include <new>
#include <typeinfo>
#include <cstring>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C33Matrix.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Source/DB/DBClient/CreatureStats_C.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AnimationDataRec.h"
#include "WoW/Source/DB/Rec/AreaGroupRec.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/DB/Rec/ChrRacesRec.h"
#include "WoW/Source/DB/Rec/CreatureDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/CreatureModelDataRec.h"
#include "WoW/Source/DB/Rec/CreatureMovementInfoRec.h"
#include "WoW/Source/DB/Rec/CreatureSoundDataRec.h"
#include "WoW/Source/DB/Rec/EmotesRec.h"
#include "WoW/Source/DB/Rec/FactionRec.h"
#include "WoW/Source/DB/Rec/FactionTemplateRec.h"
#include "WoW/Source/DB/Rec/ItemDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/ItemRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/SpellVisualKitRec.h"
#include "WoW/Source/DB/Rec/SpellVisualRec.h"
#include "WoW/Source/DB/Rec/UnitBloodLevelsRec.h"
#include "WoW/Source/DB/Rec/VehicleSeatRec.h"
#include "WoW/Source/Game/GameClient/CDrawBowString.h"
#include "WoW/Source/Game/GameClient/PlayerName.h"
#include "WoW/Source/Game/GameClient/WorldText.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/MagicClient/SpellVisuals.h"
#include "WoW/Source/Net/NetClient/NetClient.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/CMoveSpline.h"
#include "WoW/Source/Object/CreatureStats.h"
#include "WoW/Source/Object/Object.h"
#include "WoW/Source/Object/ObjectClient/Corpse_C.h"
#include "WoW/Source/Object/ObjectClient/Effect_C.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Missile_C.h"
#include "WoW/Source/Object/ObjectClient/Movement_C.h"
#include "WoW/Source/Object/ObjectClient/ObjectEffect.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/UnitMissileTrajectory_C.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/Object/ObjectClient/VehicleCamera_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/ObjectMgrClient/CClientObjCreate.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/BattlefieldInfo.h"
#include "WoW/Source/Ui/ChatBubbleFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/MinimapFrame.h"
#include "WoW/Source/Ui/NamePlateFrame.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WorldClient/AreaList.h"
#include "WoW/Source/WorldClient/MapWeather.h"
#include "WoW/Source/WorldClient/World.h"
#if defined(_MSC_VER)
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Frame/CLayoutFrame.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "ThirdParty/zlib/zlib.h"
#include "WoW/Source/DB/DBClient/GameObjectStats_C.h"
#include "WoW/Source/DB/Rec/CreatureDisplayInfoExtraRec.h"
#include "WoW/Source/DB/Rec/CreatureFamilyRec.h"
#include "WoW/Source/DB/Rec/EnvironmentalDamageRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/SkillLineAbilityRec.h"
#include "WoW/Source/DB/Rec/SkillRaceClassInfoRec.h"
#include "WoW/Source/DB/Rec/SpellDispelTypeRec.h"
#include "WoW/Source/DB/Rec/SpellMissileMotionRec.h"
#include "WoW/Source/DB/Rec/SpellShapeshiftFormRec.h"
#include "WoW/Source/DB/Rec/SpellVisualEffectNameRec.h"
#include "WoW/Source/DB/Rec/TerrainTypeRec.h"
#include "WoW/Source/DB/Rec/UnitBloodRec.h"
#include "WoW/Source/DB/Rec/VehicleRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Game/GameClient/PetNameCache.h"
#include "WoW/Source/Game/GameClient/ProjectTex.h"
#include "WoW/Source/Game/GameClient/tables.h"
#include "WoW/Source/Game/GameClient/WTOBJECT.h"
#include "WoW/Source/Game/GameTime.h"
#include "WoW/Source/Game/ViolenceLevels.h"
#include "WoW/Source/Glue/ComponentData.h"
#include "WoW/Source/Magic/MagicClient/SpellTable.h"
#include "WoW/Source/Magic/Spell.h"
#include "WoW/Source/Object/CMovementStatus.h"
#include "WoW/Source/Object/MovementShared.h"
#include "WoW/Source/Object/ObjectClient/UnitCombat_C.h"
#include "WoW/Source/Object/ObjectClient/UnitCombatLog_C.h"
#include "WoW/Source/Object/ObjectClient/UnitSound_C.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/ObjectAlloc/ObjectAlloc.h"
#include "WoW/Source/Ui/BarberShop.h"
#include "WoW/Source/Ui/CGBankInfo.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/ClassTrainerFrame.h"
#include "WoW/Source/Ui/DuelInfo.h"
#include "WoW/Source/Ui/GossipInfo.h"
#include "WoW/Source/Ui/GuildBankFrame.h"
#include "WoW/Source/Ui/MailInfo.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/Ui/QuestFrame.h"
#include "WoW/Source/Ui/ReputationInfo.h"
#include "WoW/Source/Ui/RuneInfo.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/StableInfo.h"
#include "WoW/Source/Ui/TaxiMapFrame.h"
#include "WoW/Source/Ui/TotemInfo.h"
#include "WoW/Source/Ui/Tutorial.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/UIUtil/Cursor.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "WoW/Source/UIUtil/SmartScreenRect.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

static const float UNDERWATER_BUBBLE_THRESHOLD = 5.0f;
static const float MOUSE_LOOK_SEND_FACING_DELTA = 0.1f;

static const float TRACKMOVE_THRESHOLD_DISTANCE = 4.5f;
static const int32_t DEFAULT_MODEL_FADE_TIME = 1000;

static const float MAX_NAMEPLATE_DIST_SQUARED = 1681.0f;

static const float MISSILE_MIN_HEIGHT = 0.25f;

// STATIC: 0x00ADAC04
static INVENTORY_SLOTS s_virtualItemInvSlots[2] = { INVSLOT_MAINHAND, INVSLOT_OFFHAND };

// STATIC: 0x00CA1398
static char s_objectNameBuffers[2][305];

// STATIC: 0x00CA15FC
static uint32_t s_objectNameIndex;

// STATIC: 0x00ADAA10
static int32_t s_baseSpiritRegenCap = 50;

// STATIC: 0x00ADAA14
static const char* s_genderSuffix[2] = { "M", "F" };

// STATIC: 0x00CA11A8
static WOWGUID s_pendingInteractTarget;

// STATIC: 0x00CA11B0
static WOWGUID s_pendingSkinTarget;

// STATIC: 0x00CA11B8
static WOWGUID s_pendingUseObject;

// STATIC: 0x00CA11C0
static WOWGUID s_pendingLootTarget;

// STATIC: 0x00CA11C8
static WOWGUID s_pendingAttackTarget;

// STATIC: 0x00CA120C
int32_t g_environmentalDamageVisualKits[6];

// STATIC: 0x00CA1224
static EmotesRec* s_talkEmotes[TALKANIM_NUMTALKANIMS];

// STATIC: 0x00CA1244
static UnitThreatHeap* s_unitThreatPool;

// STATIC: 0x00CA1254
int32_t CGUnit_C::s_initialized;

// STATIC: 0x00CA1238
WOWGUID CGUnit_C::s_activeMover;

// STATIC: 0x00CA11F4
uint32_t CGUnit_C::s_trackingType;

// STATIC: 0x00CA12BC
static int32_t s_attackFlashToggle;

// STATIC: 0x00CA12C0
static uint32_t s_attackFlashTime;

// STATIC: 0x00A2D394
extern const uint32_t s_environmentalDamageSchools[6] = { 0, 0, 0, 2, 3, 2 };

// STATIC: 0x00A349B4
static const int32_t s_animTierFallback[5] = { 0, 0, 3, 0, 0 };

// STATIC: 0x00ADAC0C
static float s_trackCursorMinSize = 0.5f;

// STATIC: 0x00ADAC10
static float s_trackCursorMaxSize = 0.75f;

// STATIC: 0x00ADAC14
static uint32_t s_trackCursorPeriod = 500;

// STATIC: 0x00ADAC00
static uint8_t s_rippleDirectional[4] = { 0, 0, 1, 0 };

// STATIC: 0x00ADAA98
static uint8_t s_attackTargetColor[4] = { 0x00, 0x00, 0xFF, 0xFF };

// STATIC: 0x00CA11D8
float CGUnit_C::s_trackingFacing;

// STATIC: 0x00CA1208
uint32_t CGUnit_C::s_namePlateFlags;

// STATIC: 0x00CA0AB0
uint32_t CGUnit_C::s_missileTrajectoryTargetingSpell;

// STATIC: 0x00CA0AB4
uint32_t CGUnit_C::s_pitchMissileTrajectoryTargetingNextFrame;

// STATIC: 0x00A37170
const uint32_t CGUnit_C::s_impactTypes[4] = { 0, 8, 7, 9 };

// STATIC: 0x00CA1264
C3Vector CGUnit_C::s_trackingPosition;

// STATIC: 0x00CA11EC
float CGUnit_C::s_trackingDirection;

// STATIC: 0x00CA1370
VehicleSeatRec* CGUnit_C::s_vehiclePassengerCameraSeat;

// STATIC: 0x00CA1374
VehicleRec* CGUnit_C::s_vehiclePassengerCameraVehicle;

// STATIC: 0x00CA1378
uint32_t CGUnit_C::s_vehiclePassengerCameraActive;

// STATIC: 0x00CA12CC
int32_t CGUnit_C::s_dismountSoundKitId;

// STATIC: 0x00CA1198
WOWGUID CGUnit_C::s_lastNPCSoundGUID;

// STATIC: 0x00CA1248
WOWGUID CGUnit_C::s_deferredClientControlUpdateGUID;

// STATIC: 0x00CA1250
uint8_t CGUnit_C::s_deferredClientControlUpdateState;

// STATIC: 0x00CA1240
uint32_t CGUnit_C::s_flightStatusTime;

// STATIC: 0x00CA11D0
float CGUnit_C::s_unused_CA11D0;

// STATIC: 0x00CA11D4
uint32_t CGUnit_C::s_trackingStuckCount;

// STATIC: 0x00CA11F8
WOWGUID CGUnit_C::s_trackingGUID;

// STATIC: 0x00CA1200
uint32_t CGUnit_C::s_trackingFlags;

// STATIC: 0x00CA1204
CGNamePlateFrame* CGUnit_C::s_namePlateFocus;

// STATIC: 0x00ADAA80
static TSList<CGNamePlateFrame> s_namePlates;

// STATIC: 0x00ADAA8C
static TSList<CGNamePlateFrame> s_unusedNamePlates;

// STATIC: 0x00ADAAB8
static ClickToMoveActionInfo s_clickToMoveActions[12] = {
    { 0, { 0, 0, 0 }, 0.0f, 0, 0, { 0, 0 } },
    { 0, { 0, 0, 0 }, 0.0f, 0, 0, { 0, 0 } },
    { 0, { 0, 0, 0 }, 0.0f, 0, 0, { 0, 0 } },
    { 1, { 0, 0, 0 }, 900.0f, 0, 0, { 0, 0 } },
    { 1, { 0, 0, 0 }, 0.0f, 1, 0, { 0, 0 } },
    { 1, { 0, 0, 0 }, 6400.0f, 1, 0, { 0, 0 } },
    { 1, { 0, 0, 0 }, 6400.0f, 1, 0, { 0, 0 } },
    { 1, { 0, 0, 0 }, 6400.0f, 1, 0, { 0, 0 } },
    { 1, { 0, 0, 0 }, 0.0f, 1, 1, { 0, 0 } },
    { 1, { 0, 0, 0 }, 6400.0f, 1, 0, { 0, 0 } },
    { 1, { 0, 0, 0 }, 6400.0f, 1, 0, { 0, 0 } },
    { 1, { 0, 0, 0 }, 0.0f, 1, 0, { 0, 0 } },
};

// STATIC: 0x00ADAA20
static const uint32_t s_effectAttachmentIds[16] = { 20, 34, 19, 21, 22, 17, 23, 24, 25, 15, 16, 37, 38, 22, 47, 48 };

// STATIC: 0x00CA1190
static int32_t s_lastNPCSoundCount;

// STATIC: 0x00CA11A4
static float s_forcedRunSpeed;

// STATIC: 0x00CA11DC
static float s_trackingTurnRate;

// STATIC: 0x00CA11E0
static float s_trackingInteractDistanceSq;

// STATIC: 0x00CA11E4
static float s_trackingInteractDistance;

// STATIC: 0x00CA11E8
static float s_trackingObjectRange;

// STATIC: 0x00CA11F0
static uint32_t s_trackingLastTime;

// STATIC: 0x00CA1258
static C3Vector s_trackingStartPosition;

// STATIC: 0x00ADAA74
static float s_trackingStuckDistanceSq = 7.71604937e-06f;

// STATIC: 0x00ADAA9C
static float s_trackingFollowDistance = 2.77777767f;

// STATIC: 0x00ADAAA0
static float s_trackingFollowDistanceSq = 9.0f;

// STATIC: 0x00ADAAAC
static float s_trackingTerrainDistanceSq = 0.25f;

// STATIC: 0x00CA1270
static float s_trackingDefaultInteractDistanceSq = s_trackingFollowDistance * s_trackingFollowDistance;

// STATIC: 0x00ADAC18
static uint32_t s_spellInterruptTrackingMask = 1;

// STATIC: 0x00ADBFDC
static const char* s_combatTextTokens[MISS_NUMMISSTYPES] = { "COMBAT_TEXT_NONE", "COMBAT_TEXT_MISS", "COMBAT_TEXT_RESIST", "COMBAT_TEXT_DODGE", "COMBAT_TEXT_PARRY", "COMBAT_TEXT_BLOCK", "COMBAT_TEXT_EVADE", "COMBAT_TEXT_IMMUNE", "COMBAT_TEXT_IMMUNE", "COMBAT_TEXT_DEFLECT", "COMBAT_TEXT_ABSORB", "COMBAT_TEXT_REFLECT" };

// STATIC: 0x00A34C20
static const WORLDTEXT_STYLE s_combatTextTypes[MISS_NUMMISSTYPES] = { NUM_WORLDTEXTTYPES, WT_MISS, WT_MISS, WT_MISS, WT_MISS, WT_MISS, WT_MISS, WT_MISS, WT_MISS, WT_MISS, WT_ABSORB, WT_MISS };


// STATIC: 0x00ADAA6C
static uint32_t s_petSpellDamageColor = 0xFFFFDE00;

// STATIC: 0x00ADAA70
static uint32_t s_petMeleeDamageColor = 0xFFFF8400;

// VERIFY: b3615d25 auto
// OFFSET: 0x00716810
int32_t AuraStateUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    if (guid == ClntObjMgrGetActivePlayer() || guid == CGGameUI::m_lockedTarget) {
        CGSpellBook::UpdateUsable();
        FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_UPDATE_USABLE, nullptr);
    }

    return 1;
}

// VERIFY: 24ad9714 manual
// OFFSET: 0x00716860
int32_t DisplayIDUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        if ((unit->m_obj->m_type & TYPE_PLAYER) && unit->m_shapeshiftSuppressed && unit->m_displayId == unit->m_unit->m_unitFieldNativedisplayid) {
            unit->m_displayId = 0;
        }

        if (unit->m_obj->m_guid == ClntObjMgrGetActivePlayer() && CGBarberShop::m_barberShopEnabled) {
            CGBarberShop::DisableBarberShop();
        }

        unit->UpdateDisplayInfo(0);
    }

    return 1;
}

// VERIFY: 9a7c1358 manual
// OFFSET: 0x0071C9D0
int32_t NPCFlagsHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->NPCFlagChanged(*static_cast<const uint32_t*>(oldValue));
    }

    return 1;
}

// VERIFY: 1cda89af manual
// OFFSET: 0x007234D0
int32_t UnitPowerUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));
    POWER_TYPE powerType = static_cast<POWER_TYPE>((fieldOffset - offsetof(CGUnitData, m_unitFieldPower)) / sizeof(uint32_t));

    if (g_predictedPowerCVar->m_intValue && (guid == ClntObjMgrGetActivePlayer() || guid == CGPetInfo::GetPet(0))) {
        int32_t power = powerType == POWER_TYPE_HEALTH ? unit->m_unit->m_unitFieldHealth : unit->m_unit->m_unitFieldPower[powerType];
        int32_t predicted = g_predictedPowerCVar->m_intValue ? unit->m_predictedPower[powerType] : (powerType == POWER_TYPE_HEALTH ? unit->m_unit->m_unitFieldHealth : unit->m_unit->m_unitFieldPower[powerType]);

        if (power <= predicted) {
            return 1;
        }
    }

    if (powerType == POWER_TYPE_HEALTH) {
        unit->SetPredictedPower(powerType, unit->m_unit->m_unitFieldHealth);
        return 1;
    }

    unit->SetPredictedPower(powerType, unit->m_unit->m_unitFieldPower[powerType]);

    return 1;
}

// VERIFY: 7059bc7b manual
// OFFSET: 0x007235C0
int32_t UnitMaxPowerUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));
    POWER_TYPE powerType = static_cast<POWER_TYPE>((fieldOffset - offsetof(CGUnitData, m_unitFieldMaxpower)) / sizeof(uint32_t));

    if (powerType == POWER_TYPE_HEALTH) {
        unit->SetPredictedPower(powerType, unit->m_unit->m_unitFieldHealth);
        return 1;
    }

    unit->SetPredictedPower(powerType, unit->m_unit->m_unitFieldPower[powerType]);

    return 1;
}

// VERIFY: 7de81979 manual
// OFFSET: 0x00723620
int32_t UnitDisplayPowerUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));
    POWER_TYPE powerType = static_cast<POWER_TYPE>(unit->m_unit->m_unitFieldBytes0[3]);

    if (powerType == POWER_TYPE_HEALTH) {
        unit->SetPredictedPower(powerType, unit->m_unit->m_unitFieldHealth);
        return 1;
    }

    unit->SetPredictedPower(powerType, unit->m_unit->m_unitFieldPower[powerType]);

    return 1;
}

// VERIFY: 3ebc37c4 manual
// OFFSET: 0x00723680
int32_t UnitFactionUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->UpdateReaction(false);
    }

    return 1;
}

// VERIFY: 999f5087 manual
// OFFSET: 0x00728D20
int32_t UnitPvPFlagUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->OnPvPFlagChanged(*static_cast<const uint8_t*>(oldValue));
    }

    return 1;
}

// VERIFY: 66e89ebf manual
// OFFSET: 0x00728D60
int32_t UnitCharmedUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        if (unit->m_nameDesc) {
            PlayerNameTriggerNameRegenerate(unit->m_nameDesc);
        }

        if (unit->m_namePlateFrame) {
            unit->m_namePlateFrame->UpdateNameDisplay(unit);
        }

        unit->UpdateReaction(false);
        unit->m_clientFlags &= ~0x1000;
    }

    if (*static_cast<const WOWGUID*>(oldValue) == ClntObjMgrGetActivePlayer()) {
        Spell_C_RemovePeriodicClientTrigger(guid, 0);
        return 1;
    }

    if (unit->m_unit->m_unitFieldCharmedby == ClntObjMgrGetActivePlayer()) {
        unit->CheckApplyPeriodicClientTriggers();
    }

    return 1;
}

// VERIFY: 6c375ecf manual
// OFFSET: 0x00728E20
int32_t VirtualItemIDMirrorHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->OnVirtualItemChanged(static_cast<VIRTUAL_MONSTER_SLOT>((fieldOffset - offsetof(CGUnitData, m_unitVirtualItemSlotId)) / sizeof(uint32_t)), *static_cast<const int32_t*>(oldValue));
    }

    return 1;
}

// VERIFY: ce51f301 manual
// OFFSET: 0x0072CEB0
int32_t UnitModeUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return 1;
    }

    CreatureStats_C* stats = g_creatureDBCache.GetRecord(unit->m_obj->m_entryID, guid, CreatureQueryCallback, nullptr, false);

    if (stats) {
        unit->DestroyNamePlate();
        unit->m_creatureCacheEntry = stats;
        CGGameUI::UnitNameUpdate(guid);
        Script_SendUnitSignal(guid, 148);

        TotemInfo* totem = CGGameUI::GetTotemInfo(guid);

        if (totem) {
            totem->m_name = unit->GetUnitName(nullptr, 1);
            FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_TOTEM_UPDATE, "%d", totem->m_slot + 1);
        }
    }

    return 1;
}

// VERIFY: 0ef13f6b manual
// OFFSET: 0x0072CF70
int32_t PetNameChangeHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->GetUnitName(nullptr, 1);

        const WOWGUID* owner = unit->m_unit->m_unitFieldCharmedby ? &unit->m_unit->m_unitFieldCharmedby : &unit->m_unit->m_unitFieldSummonedby;

        if (*owner == ClntObjMgrGetActivePlayer()) {
            FrameScript_SignalEvent(SCRIPT_EVENT_LOCALPLAYER_PET_RENAMED, nullptr);
        }
    }

    return 1;
}

// VERIFY: d2db77ea manual
// OFFSET: 0x0072CFF0
int32_t PetNumberChangeHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->GetUnitName(nullptr, 1);
        unit->UpdateModelScale(nullptr);
        unit->DestroyNamePlate();
    }

    Script_SendUnitSignal(guid, 148);

    if (guid == CGPetInfo::GetPet(0)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PET_UI_UPDATE, nullptr);
    }

    return 1;
}

// VERIFY: d1d8ea1c manual
// OFFSET: 0x0072D070
int32_t UnitScaleUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->HandleScaleUpdate(*static_cast<const float*>(oldValue));
    }

    return 1;
}

// VERIFY: 80a48461 manual
// OFFSET: 0x00730050
int32_t UnitLevelUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    uint32_t oldLevel = *static_cast<const uint32_t*>(oldValue);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit && unit->m_unit->m_unitFieldLevel != oldLevel) {
        unit->OnLevelChange();
    }

    return 1;
}

// VERIFY: 9c8df896 manual
// OFFSET: 0x00737AA0
int32_t SheatheStateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return 1;
    }

    uint32_t oldState = *static_cast<const uint32_t*>(oldValue);

    if (unit->m_obj->m_guid != ClntObjMgrGetActivePlayer() || unit->m_sheatheState == oldState) {
        unit->SetSheatheState(unit->m_unit->m_unitFieldBytes2[0], 1, 1);
    }

    return 1;
}

// VERIFY: 42cd1170 manual
// OFFSET: 0x0073F270
int32_t UnitFlagUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->OnFlagChanged(*static_cast<const uint32_t*>(oldValue));
    }

    return 1;
}

// VERIFY: 2c27c5d6 manual
// OFFSET: 0x0073F2B0
int32_t UnitFlag2UpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->OnFlag2Changed(*static_cast<const uint32_t*>(oldValue));
    }

    return 1;
}

// VERIFY: fff011c6 manual
// OFFSET: 0x0073F2F0
int32_t UnitVisFlagUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->OnVisFlagChanged(*static_cast<const uint8_t*>(oldValue));
    }

    return 1;
}

// VERIFY: acc767d0 manual
// OFFSET: 0x0073F330
int32_t UnitHealthUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));
    unit->m_predictedHealth = unit->m_unit->m_unitFieldHealth;

    if (!(static_cast<float>(unit->m_unit->m_unitFieldHealth) / static_cast<int32_t>(unit->m_unit->m_unitFieldMaxhealth) < 0.2f) || unit->IsDeadOrGhost()) {
        unit->m_clientFlags &= ~0x2;
    } else if (unit->m_bloodlevels) {
        unit->m_clientFlags |= 0x2;
    }

    int32_t oldHealth = *static_cast<const int32_t*>(oldValue);

    if (oldHealth > unit->m_unit->m_unitFieldHealth && unit->m_namePlateFrame) {
        unit->m_namePlateFrame->TriggerDamageFeedback();
    }

    int32_t health = unit->m_unit->m_unitFieldHealth;

    if (health <= 0) {
        if (oldHealth > 0) {
            if (guid == CGUnit_C::s_activeMover) {
                CGInputControl::GetActive()->UpdatePlayer(OsGetAsyncTimeMs(), 1);
            }

            unit->OnDeath();
        }
    } else if (oldHealth <= 0) {
        if (guid == CGUnit_C::s_activeMover) {
            CGInputControl::GetActive()->UpdatePlayer(OsGetAsyncTimeMs(), 1);
        }

        unit->OnLife(false);
    }

    if (unit->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CGSpellBook::UpdateUsable();
    }

    return 1;
}

// VERIFY: 3f8e530c manual
// OFFSET: 0x0073F460
int32_t StandStateUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    if (guid != ClntObjMgrGetActivePlayer()) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            unit->UpdateClientStandState(static_cast<UNITSTANDSTATE>(unit->m_unit->m_unitFieldBytes1[0]));
        }
    }

    return 1;
}

// VERIFY: 75f11945 manual
// OFFSET: 0x0073F4B0
int32_t EmoteStateUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->UpdateBaseAnimation(false, -1);
    }

    return 1;
}

// VERIFY: 749adc30 manual
// OFFSET: 0x0073F4F0
int32_t ChannelSpellOrObjectChangeHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        const WOWGUID* oldObject = static_cast<const WOWGUID*>(oldValue);
        int32_t oldSpell = *reinterpret_cast<const int32_t*>(oldObject + 1);
        unit->OnChannelObjectChanged(*oldObject, oldSpell);
        unit->OnChannelSpellChanged(oldSpell);
    }

    return 1;
}

// VERIFY: 1cb365b5 manual
// OFFSET: 0x007419C0
int32_t MountDisplayIDUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->OnMountDisplayChanged(unit->m_unit->m_unitFieldMountdisplayid);
    }

    return 1;
}

// VERIFY: 47275a1f manual
// OFFSET: 0x00741A00
int32_t DynamicFlagsChangeHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->OnDynamicFlagsChanged(*static_cast<const uint32_t*>(oldValue));
    }

    return 1;
}

// VERIFY: 74767534 manual
// OFFSET: 0x00725840
void CGUnit_C::DestroyNamePlate() {
    if (!this->m_namePlateFrame) {
        return;
    }

    if (CGUnit_C::s_namePlateFocus == this->m_namePlateFrame) {
        CGUnit_C::s_namePlateFocus = nullptr;
    }

    s_unusedNamePlates.LinkToTail(this->m_namePlateFrame);
    this->m_namePlateFrame->Hide();
    this->m_namePlateFrame = nullptr;
    this->ObjectNameVisibilityChanged();
}

// VERIFY: 220d07a7 auto
// OFFSET: 0x0072B350
void CGUnit_C::PostAnimate(CGWorldFrame* worldFrame) {
    C3Vector position(0.0f, 0.0f, 0.0f);

    if (this->ShouldShowNamePlate(worldFrame, &position)) {
        this->NamePlateAddOrUpdate(position, worldFrame);
    } else {
        this->DestroyNamePlate();
    }
}

// VERIFY: ad7e4a0e manual
// OFFSET: 0x00734390
void CGUnit_C::UpdateDisplay(uint32_t time) {
    this->UpdateCritterDisplay();

    if ((this->m_modelFlags & 0x200000) && this->GetObjectModel()->IsLoaded(0, 0)) {
        this->ModelChanged();
    }

    if (this->m_missiles) {
        if (this->m_clientFlags & 0x80000000) {
            this->ReleaseMissiles(this->m_animEventPosition);
        }

        if (static_cast<int32_t>(time - this->m_lastMissileTime - 2000) >= 0) {
            C3Vector position(0.0f, 0.0f, 0.0f);
            position = this->GetMissileFirePosition();
            this->ReleaseMissiles(position);
        }

        if (this->m_specialIdleAnim == -1 || !IsSpecialIdleAnim(this->m_specialIdleAnim)) {
            C3Vector position(0.0f, 0.0f, 0.0f);
            position = this->GetMissileFirePosition();
            this->ReleaseMissiles(position);
        }
    }

    this->m_clientFlags &= ~0x80000000;

    if (this->m_clientFlags & 0x800) {
        if (this->m_missiles && this->m_missiles->m_model) {
            if (this->m_missiles->m_model->IsLoaded(0, 0)) {
                this->ReleaseMissiles();
                this->m_clientFlags &= ~0x800;
            }
        } else {
            this->m_clientFlags &= ~0x800;
        }
    }

    if (!this->m_worldObject || !this->GetObjectModel() || !this->GetObjectModel()->IsLoaded(0, 0)) {
        return;
    }

    if (!World::QueryObjectVisible(this->m_worldObject)) {
        this->DestroyNamePlate();
    }

    if (this->m_clientFlags & 0x2) {
        this->HandleBloodPool(time);
    }

    World::TickObject(this->m_worldObject);

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && !(this->m_obj->m_type & TYPE_PLAYER) && this->IsAutoTracking()) {
        this->HandleTracking(0.0f, 0.0f);
    }

    if (this->m_vehiclePassenger && (this->m_vehiclePassenger->m_flags & 0x10)) {
        this->UpdateWorldObject(0);
    }

    for (CEffect* effect = this->m_effectList; effect; ) {
        CEffect* next = effect->m_next;
        effect->EffectUpdate();
        effect = next;
    }
}

// VERIFY: 6d8b08cb manual
// OFFSET: 0x0071C050
static float GetExtraDisplayScale(const CreatureDisplayInfoRec* displayInfo) {
    CreatureDisplayInfoExtraRec* extra = g_creatureDisplayInfoExtraDB.GetRecord(displayInfo->m_extendedDisplayInfoID);

    if (!extra) {
        return 1.0f;
    }

    ChrRacesRec* race = g_chrRacesDB.GetRecord(extra->m_displayRaceID);

    if (!race) {
        return 1.0f;
    }

    int32_t displayId = 0;

    switch (extra->m_displaySexID) {
        case 0: {
            displayId = race->m_maleDisplayID;
            break;
        }

        case 1: {
            displayId = race->m_femaleDisplayID;
            break;
        }
    }

    CreatureDisplayInfoRec* rec = g_creatureDisplayInfoDB.GetRecord(displayId);

    return rec ? rec->m_creatureModelScale : 1.0f;
}

// VERIFY: 411bfb59 auto
// OFFSET: 0x0071C110
float CGUnit_C::GetModelScale(const CreatureDisplayInfoRec* displayInfo, const CreatureModelDataRec* modelData) {
    CreatureFamilyRec* family = g_creatureFamilyDB.GetRecord(this->m_creatureCacheEntry ? this->m_creatureCacheEntry->m_family : 0);
    float scale = GetExtraDisplayScale(displayInfo) * displayInfo->m_creatureModelScale * modelData->m_modelScale;
    float fraction = 0.0f;

    if (scale <= 0.0f) {
        scale = 1.0f;
    }

    if (family) {
        int32_t level = this->m_unit->m_unitFieldLevel;
        int32_t delta = level < family->m_minScaleLevel ? 0 : level - family->m_minScaleLevel;
        int32_t range = family->m_maxScaleLevel - family->m_minScaleLevel;

        if (delta > range) {
            delta = range;
        }

        if (range) {
            fraction = static_cast<float>(delta) / range;
        }

        float familyScale = fraction * (family->m_maxScale - family->m_minScale) + family->m_minScale;

        if (familyScale > scale || this->m_unit->m_unitFieldPetnumber) {
            return familyScale;
        }
    }

    return scale;
}

// VERIFY: 3f33188f manual
// OFFSET: 0x007370D0
void CGUnit_C::UpdateWorldObject(int32_t a2) {
    if (this->m_worldObject) {
        C44Matrix matrix;
        CM2Model* model = this->GetObjectModel();

        if (this->m_vehiclePassenger && this->m_vehiclePassenger->m_state && (this->m_vehiclePassenger->m_flags & 0x10) && model && (model->m_fFlags & 0x8000)) {
            matrix = model->GetAnimatedWorldTransform();
        } else {
            matrix = this->GetMatrix();
            matrix.Scale(this->GetTrueScale());
        }

        CAaBox box(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f));
        CAaSphere sphere;
        sphere.c = C3Vector(0.0f, 0.0f, 0.0f);
        sphere.r = 0.0f;
        C3Vector vec(0.0f, 0.0f, this->m_movementData.m_collisionHeight * 0.5f);

        if (model && model->IsLoaded(0, 0)) {
            if (model->IsLoaded(0, 1)) {
                box = model->GetMaxBoundingBoxPlusAttachments();
                sphere = model->GetMaxBoundingSpherePlusAttachments();
            } else {
                box = model->GetBoundingBox();
                sphere = model->GetBoundingSphere();
            }

            if (this->m_vehicle && this->m_vehicle->m_vehicleRec) {
                float radius = this->m_vehicle->m_largestPassengerBoundsRadius + this->m_vehicle->m_largestPassengerBoundsRadius;

                if (box.IsValid()) {
                    box.Expand(radius);
                } else {
                    box.Set(-radius, radius);
                }

                sphere.r = sphere.r + radius;
            }
        }

        CMap::ObjectUpdate(this->m_worldObject, matrix, box, sphere, vec, a2, this->m_torsoKeyBone);
    }

    if (this->m_vehicle && this->m_vehicle->m_vehicleRec) {
        for (CPassenger* passenger = this->m_vehicle->m_passengers.Head(); passenger; passenger = this->m_vehicle->m_passengers.Next(passenger)) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPE_UNIT, __FILE__, __LINE__));
            unit->UpdateWorldObject(0);
        }
    }
}

// VERIFY: 59a2b47b auto
// OFFSET: 0x0071F560
int32_t CGUnit_C::IsDeadOrAttachmentDying() {
    if (this->m_unit->m_unitFieldHealth > 0 && !(this->m_unit->m_unitFieldFlags2 & 0x1) && this->GetClientStandState() != UNIT_DEAD) {
        for (CEffect* effect = this->m_effectList; effect; effect = effect->m_next) {
            if (effect->m_kitRec && IsDeadOrDyingAnim(effect->m_kitRec->m_animID)) {
                return 1;
            }
        }

        return 0;
    }

    return 1;
}

// VERIFY: 87f21e3d manual
// OFFSET: 0x0072A480
int32_t CGUnit_C::DisplayInfoNeedsUpdate() {
    int32_t displayId = this->m_displayId;

    if (!displayId || this->m_unit->m_unitFieldNativedisplayid != this->m_unit->m_unitFieldDisplayid) {
        displayId = this->m_unit->m_unitFieldDisplayid;
    }

    CreatureDisplayInfoRec* displayInfo = g_creatureDisplayInfoDB.GetRecord(displayId);

    if (!displayInfo) {
        SysMsgPrintf(SYSMSG_ERROR, 2, "NOUNITDISPLAYID|%d|%s", displayId, this->GetUnitName(nullptr, 1));
        return 0;
    }

    if (displayInfo == this->m_displayInfo) {
        return 0;
    }

    if (displayInfo->m_extendedDisplayInfoID == this->m_displayInfo->m_extendedDisplayInfoID && g_creatureModelDataDB.GetRecord(displayInfo->m_modelID) == this->m_modelData) {
        return g_creatureSoundDataDB.GetRecord(displayInfo->m_soundID) != this->m_soundData;
    }

    return 1;
}

// VERIFY: 503fd361 manual
// OFFSET: 0x0073E410
void CGUnit_C::UpdateDisplayInfo(int32_t force) {
    if (!force && !this->DisplayInfoNeedsUpdate()) {
        return;
    }

    int32_t deathAnim = ANIM_COUNT;
    this->m_wheelMask = 0;

    if (this->IsDeadOrAttachmentDying()) {
        int32_t behavior = ANIM_COUNT;
        AnimationDataRec* animRec = g_animationDataDB.GetRecord(this->GetCurrentTorsoAnimId());

        if (animRec) {
            behavior = animRec->m_behaviorID;
        }

        if (IsDyingAnim(behavior) && this->GetTorsoAnimProgress() < 0.5f) {
            deathAnim = behavior;
        }
    }

    this->KillCreatureLoopSound();

    CreatureModelDataRec* oldModelData = this->m_modelData;
    this->RefreshDataPointers();
    this->m_animationState &= ~0x180;
    this->m_torsoKeyBone = -1;

    const char* fileName;
    this->GetModelFileName(&fileName);

    CM2Model* model = World::s_m2Scene->CreateModel(fileName, 0);

    if (!model) {
        return;
    }

    this->m_animationState |= 0x8000000;

    if (this->m_nameDesc) {
        PlayerNameDelete(this->m_nameDesc);
    }

    this->m_nameDesc = nullptr;

    uint32_t torsoAnim = this->GetCurrentTorsoAnimId();
    model->SetSequenceCallback(reinterpret_cast<void*>(&CGUnit_C::AnimEndCallback), this->m_obj->m_guid.m_guid);
    model->SetEventCallback(reinterpret_cast<void*>(&CGUnit_C::AnimEventCallback), this->m_obj->m_guid.m_guid);
    this->SetCharacterModel(model);
    model->Release();
    model->m_fFlags ^= (model->m_fFlags ^ (this->m_modelData->m_flags >> 7)) & 0x4;
    this->UpdateModelScale(oldModelData);
    this->Animate(0.0f);
    model->ForceAnimate();
    this->RecreateNamePlate();
    CCharacterComponent::ApplyMonsterGeosets(this->m_worldModel, this->m_displayInfo);
    CCharacterComponent::ReplaceMonsterSkin(this->m_worldModel, this->m_displayInfo, this->m_modelData);

    CreatureModelDataRec* modelData = this->m_modelData;
    this->m_footprintTextureId = modelData->m_footprintTextureID;
    this->m_footprintLength = modelData->m_footprintTextureWidth * 0.027777778f;
    this->m_footprintWidth = 0.027777778f * modelData->m_footprintTextureLength;
    this->m_footprintParticleScale = modelData->m_footprintParticleScale;
    this->CheckLoopSound();

    if (this->m_obj->m_guid == CGWorldFrame::GetActiveCamera()->m_targetGUID) {
        CGGameUI::ResetCamera(this->m_obj->m_guid);
    }

    this->RefreshInteractIcon();

    if (this->m_characterComponent) {
        CCharacterComponent::FreeComponent(this->m_characterComponent);
        this->m_characterComponent = nullptr;
        this->m_clientFlags = (this->m_clientFlags & ~0x20000) | 0x400000;
    }

    this->m_clientFlags = (this->m_clientFlags & ~0x20000) | 0x400000;
    this->ApplyModelFade();
    this->ScanAuraSpellsOnDisplayUpdate();
    this->UpdateChannelEffects();
    this->m_animStateFlags &= ~0x7;
    this->m_smoothSwimPitch = 0.0f;

    float facing = this->m_movementData.GetFacing(this->m_renderFacing);
    this->m_smoothFacing = facing;
    this->m_smoothFacingTarget = facing;
    this->m_smoothSwimRoll = 0.0f;

    if (this->m_modelData->m_flags & 0x8) {
        this->m_animationState |= 0x20000;
    } else {
        this->m_animationState &= ~0x20000;
    }

    this->m_animationState &= 0xF932DFF3;
    this->m_specialIdleAnim = -1;

    if (this->m_modelData->m_flags & 0x40) {
        this->m_clientFlags |= 0x2000000;
    } else {
        this->m_clientFlags &= ~0x2000000;
    }

    if (deathAnim != ANIM_COUNT) {
        this->PlayBaseAnimation(deathAnim, 0);
    } else {
        uint32_t currentAnim = this->GetCurrentTorsoAnimId();

        if (!currentAnim && torsoAnim != ANIM_COUNT && torsoAnim != 0xFFFFFFFF && this->m_movementData.IsFalling()) {
            this->PlayBaseAnimation(torsoAnim, 0);
        } else {
            bool dying = false;
            uint32_t flags = 0;

            for (CEffect* effect = this->m_effectList; effect; effect = effect->m_next) {
                const SpellVisualKitRec* kit = effect->m_kitRec;

                if (kit && IsDeadOrDyingAnim(kit->m_animID)) {
                    if (kit->m_flags & 0x4) {
                        flags |= 0x2;
                    }

                    if (kit->m_flags & 0x8) {
                        flags |= 0x4;
                    }

                    if (kit->m_flags & 0x10) {
                        flags |= 0x8;
                    }

                    if (kit->m_flags & 0x100) {
                        flags |= 0x20;
                    }

                    if (flags & 0x6) {
                        dying = true;
                        break;
                    }
                }
            }

            this->PlayBaseAnimation(currentAnim, dying ? flags : 0);
        }
    }

    if (this->m_mountModel) {
        model->AttachToParent(this->m_mountModel, 0, nullptr, 0);
    }

    this->CreateOrDestroyObjectEffectManager();
    this->m_animationState &= ~0x8000000;
}

// VERIFY: 8b1cb3ff auto
// OFFSET: 0x0071FD80
int32_t CGUnit_C::Animate(float elapsed) {
    if (this->m_vehiclePassenger) {
        this->m_vehiclePassenger->UpdateModel();
        this->UpdateMountTransitionModel();
        return 1;
    }

    CMovement_C* movement = this->m_movement;

    if ((movement->m_flags & 0x2200000) && ((movement->m_flags & 0x3) || (this->m_animStateFlags & 0x7))) {
        C44Matrix matrix;

        if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
            this->ComputeSmoothedSwimMatrix(&matrix, elapsed, movement->m_flags & 0x2000000);

            if (this->m_movement->m_flags & 0x3) {
                this->m_animStateFlags |= 0x1;
            }
        } else {
            this->GetSwimMatrix(matrix);
        }

        CM2Model* model = this->GetObjectModel();
        model->m_fFlags |= 0x8000;
        model->m_worldTransform = matrix;
        this->UpdateMountTransitionModel();
        return 1;
    }

    this->ResetSmoothFacing();

    if (this->m_mountTransition && this->m_mountTransitionEffect) {
        this->MountTransitionSetCharacterTransform(elapsed);
    } else {
        this->UpdateWorldTransform(elapsed);
    }

    this->UpdateMountTransitionModel();
    return 1;
}

// VERIFY: b38ff33e manual
// OFFSET: 0x00730F30
void CGUnit_C::ShouldRender(uint32_t flags, int32_t& culled, int32_t& hidden) {
    this->CGObject_C::ShouldRender(flags, culled, hidden);

    if ((this->m_clientFlags & 0x400000) && !this->InitializeComponent()) {
        hidden = 1;
    } else if (culled || hidden) {
        if (this->m_characterComponent) {
            this->m_characterComponent->Prep();
        }
    } else if (this->m_characterComponent && !this->m_characterComponent->RenderPrep(0)) {
        hidden = 1;
    } else {
        CM2Model* model = this->m_worldModel;
        CM2Model* child = nullptr;

        if (model && (model->m_fFlags & 0x4000)) {
            for (child = model->m_attachList; child; child = child->m_attachNext) {
                if (((child->m_attachParent ? child->m_fFlags >> 7 : child->m_fFlags >> 3) & 0x1)) {
                    break;
                }
            }
        }

        if (model && (model->m_fFlags & 0x4000) && !child) {
            hidden = 1;
        } else if (this->m_vehiclePassenger && this->m_vehiclePassenger->m_state && (this->m_vehiclePassenger->m_flags & 0x400)) {
            hidden = 1;
        }
    }

    if (this->m_mountModel) {
        uint32_t visible = (!culled && !hidden) & 0x1;
        CM2Model* model = this->m_worldModel;

        if (model->m_attachParent) {
            model->m_fFlags = (model->m_fFlags & ~0x80) | (visible << 7);
            model->m_fFlags = (model->m_fFlags & ~0x20000) | (visible << 17);
        } else {
            model->m_fFlags = (model->m_fFlags & ~0x8) | (visible << 3);
            model->m_fFlags = (model->m_fFlags & ~0x10000) | (visible << 16);
        }

        hidden = 0;
    }
}

// VERIFY: 0af46a3b manual
// OFFSET: 0x0073E840
void CGUnit_C::ModelLoaded(CM2Model* model) {
    this->CGObject_C::ModelLoaded(model);

    if (this->GetTransportGUID()) {
        C44Matrix matrix;
        MovementGetTransportMtxX(WOWGUID{ this->GetTransportGUID() }, &matrix, this->m_obj->m_guid, __FILE__, __LINE__);

        if (model->m_fFlags & 0x1) {
            model->m_frameOfReferenceMatrix = matrix;
            model->ChangeFrameOfReference(&model->m_frameOfReferenceMatrix);
        }
    }

    if (model == this->m_worldModel) {
        this->m_animationState &= ~0x180;

        if (model->HasBone(4)) {
            this->m_animationState |= 0x80;
        }

        if (model->HasBone(6)) {
            this->m_animationState |= 0x100;
        }

        if (this->m_animationState & 0x80) {
            this->m_torsoKeyBone = 4;
        } else {
            this->m_torsoKeyBone = (this->m_animationState & 0x100) ? 6 : -1;
        }

        this->UpdateInteractIconAttach();
    } else if (model == this->m_mountModel && !this->m_mountModel->HasAttachment(0)) {
        SysMsgPrintf(SYSMSG_ERROR, 0x10, "MOUNTDISPLAYIDNOMOUNTATTACHMENT|%d", this->m_mountDisplayId);
    }

    if (this->m_modelFlags & 0x40000) {
        this->UpdateBaseAnimation(true, -1);

        AnimationDataRec* animRec = g_animationDataDB.GetRecord(this->GetCurrentTorsoAnimId());

        if (animRec && animRec->m_behaviorID == 127) {
            this->m_fadeDuration = 0;
            this->m_alpha = this->m_fadeEndAlpha;
        }

        this->UpdateObjectEffectAnimationStates();
    }

    if (model == this->GetObjectModel()) {
        this->InitWheels();
    }

    if (this->m_vehiclePassenger && this->m_vehiclePassenger->m_state == 3) {
        CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ this->GetTransportGUID() }, TYPE_UNIT, __FILE__, __LINE__));

        if (vehicle) {
            if (vehicle->m_vehicle && vehicle->m_vehicle->m_vehicleRec) {
                vehicle->m_vehicle->UpdateLargestPassengerBoundsRadius();
            }

            vehicle->UpdateWorldObject(0);
        }
    }

    if (this->m_pendingModelAuraSpellId) {
        if (this->HasAuraBySpellId(this->m_pendingModelAuraSpellId)) {
            SpellRec spellRec;

            if (g_spellDB.GetRecord(this->m_pendingModelAuraSpellId, &spellRec)) {
                const SpellVisualRec* visual = GetSpellVisual(&spellRec);

                if (visual) {
                    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_stateKit);

                    if (kit) {
                        PlaySpellVisuaKitData data(&spellRec, kit, 2);
                        this->PlaySpellVisualKit(data);
                    }
                }
            }
        }

        this->m_pendingModelAuraSpellId = 0;
    }

    if (model == this->GetObjectModel()) {
        if (this->m_vehicle && this->m_vehicle->m_vehicleRec && this->m_vehicle->IsMirroringAnimsToPassengers()) {
            this->m_vehicle->StartMirroringAnimsToPassengers();
        }

        if (this->m_vehiclePassenger && this->m_vehiclePassenger->OverridesModelAnimation()) {
            this->m_vehiclePassenger->StartMirroringVehicleAnims();
        }
    }
}

// VERIFY: 33d20e31 manual
// OFFSET: 0x0073DAB0
void CGUnit_C::PreAnimate(CGWorldFrame* frame) {
    uint32_t time = FrameTime::s_curTimeMs;

    if (this->m_vehiclePassenger) {
        this->m_vehiclePassenger->CheckForVehicleTransitionAnimTimeout();
    }

    if (static_cast<int32_t>(time - this->m_nextBreathCheckTime) >= 0) {
        this->UpdateBreathState(time);
    }

    if (CGGameUI::m_lockedTarget == this->m_obj->m_guid) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player && ((player->m_combatModeFlags & 0x2) || player->m_combat.m_attacking.m_guid) && player->CanAttack(this)) {
            uint32_t flashTime = s_attackFlashTime;
            this->m_clientFlags |= 0x10;

            if (static_cast<int32_t>(time - flashTime - 500) >= 0) {
                s_attackFlashToggle = s_attackFlashToggle == 0;
                flashTime = time;
                s_attackFlashTime = time;
            }

            float t = static_cast<int32_t>(flashTime - time + 500) * 0.002f;

            if (s_attackFlashToggle) {
                t = 1.0f - t;
            }

            s_attackTargetColor[1] = static_cast<uint8_t>(static_cast<int32_t>(t * 128.0f));
            PlayerNameTriggerColorUpdate(this->m_nameDesc);
        } else if (this->m_clientFlags & 0x10) {
            this->m_clientFlags &= ~0x10;
            PlayerNameTriggerColorUpdate(this->m_nameDesc);
        }
    } else if (this->m_clientFlags & 0x10) {
        this->m_clientFlags &= ~0x10;
        PlayerNameTriggerColorUpdate(this->m_nameDesc);
    }

    PlayerNameUpdateDrawCallback(this->m_nameDesc);
    this->UpdateModelTint(time);

    if (this->m_proceduralFadeStart && static_cast<int32_t>(time - this->m_proceduralFadeDuration - this->m_proceduralFadeStart) >= 0) {
        this->DoFade(this->GetBaseAlpha(), this->m_proceduralFadeEndAlpha);
        this->m_proceduralFadeStart = 0;
    }

    this->CGObject_C::PreAnimate(frame);

    while (this->m_pendingMissiles && static_cast<int32_t>(time - this->m_pendingMissiles->m_destroyTime) >= 0) {
        this->m_pendingMissiles->DeleteSelf();
    }

    for (CMissile* missile = this->m_pendingMissiles; missile; missile = missile->m_next) {
        float t = static_cast<float>(missile->m_destroyTime - time) * 0.0005f;
        float alpha;

        if (t < 0.0f) {
            alpha = 0.0f;
        } else if (t <= 1.0f) {
            alpha = t * t * t;
        } else {
            alpha = 1.0f;
        }

        if (missile->m_model) {
            missile->m_model->m_fadeAlpha = alpha;
        }
    }

    this->ProcessEmoteQueue();
    this->CreateRipple(0);

    const VehicleRec* vehicleRec = this->m_vehicle ? this->m_vehicle->m_vehicleRec : nullptr;
    uint32_t pitchBone;
    int32_t fixedFacing;

    if (vehicleRec) {
        pitchBone = vehicleRec->m_flags & 0x200;
        fixedFacing = (pitchBone || (vehicleRec->m_flags & 0x1000)) ? 1 : 0;
    } else {
        pitchBone = 0;
        fixedFacing = 0;
    }

    uint32_t moveFlags;

    if (this->m_unit->m_unitFieldHealth <= 0 || (this->m_vehiclePassenger && (this->m_vehiclePassenger->m_state == 2 || this->m_vehiclePassenger->m_state == 5)) || ((moveFlags = this->m_movement->m_flags) & 0x2200000) || fixedFacing || !(this->m_animationState & 0x180)) {
        this->m_renderFacing = this->m_smoothTarget.m_value;
        this->m_facingVelocity = 0.0f;
        this->m_facingBlend = 0.0f;
    } else if (moveFlags & 0xC) {
        float offset = (this->m_movementData.m_flags & 0x3) ? 0.7853982f : 1.5707964f;
        uint32_t direction = this->m_movementData.m_flags & 0x6;

        if (direction == 6 || !direction) {
            offset = -offset;
        }

        this->m_facingBlend = 1.0f;
        this->SmoothFacingAngle(CMath::normalizeAnglePi(CMath::normalizeAnglePi(offset + (this->m_smoothTarget.m_value - this->m_renderFacing)) + this->m_renderFacing));
    } else if (moveFlags & 0x1003) {
        if (0.0f < this->m_facingBlend) {
            this->m_facingBlend = this->m_facingBlend - CGWorldFrame::s_currentWorldFrame->m_elapsedSec * 2.5f;

            if (1.0f < this->m_facingBlend) {
                this->m_facingBlend = 1.0f;
            }

            this->SmoothFacingAngle(this->m_smoothTarget.m_value);
        } else {
            this->m_renderFacing = this->m_smoothTarget.m_value;
            this->m_facingVelocity = 0.0f;
        }
    } else if (this->m_facingBlend < 1.0f) {
        this->m_facingBlend = CGWorldFrame::s_currentWorldFrame->m_elapsedSec * 2.5f + this->m_facingBlend;
    }

    float delta = CMath::normalizeAnglePi(this->m_smoothTarget.m_value - this->m_renderFacing);
    float absDelta = fabsf(delta);
    float turn = 0.0f;
    bool clearTwist = absDelta < 0.001f;

    if (!clearTwist) {
        if (1.5707964f < absDelta) {
            turn = delta < 0.0f ? -(absDelta - 1.5707964f) : absDelta - 1.5707964f;
        }

        if (!(this->m_movement->m_flags & 0xC) && !(this->m_animationState & 0x1)) {
            float maxTurn = static_cast<float>(OsGetAsyncTimeMs() - this->m_lastTurnTimeMs) * 0.001f * this->m_movementData.m_turnRate * 8.0f;
            float step = absDelta < maxTurn ? absDelta : maxTurn;
            turn = (delta < 0.0f ? -step : step) + turn;
        }

        this->m_renderFacing = CMath::normalizeAnglePi(turn + this->m_renderFacing);
        float remaining = fabsf(CMath::normalizeAnglePi(this->m_smoothTarget.m_value - this->m_renderFacing));

        if (remaining < 0.00001f) {
            clearTwist = true;
        } else {
            if (!this->m_mountModel && (this->m_animationState & 0x80)) {
                float twist = remaining;

                if (this->m_obj->m_guid != CGUnit_C::s_activeMover || CGUnit_C::s_trackingType == TRACKTYPE_NONE) {
                    twist = twist * 0.5f;
                }

                if (0.7853982f < twist) {
                    twist = 0.7853982f;
                }

                float angle = delta < 0.0f ? -twist : twist;
                this->m_worldModel->SetBoneFlags(4, 0x80, 0x80);
                CM2Model* model = this->m_worldModel;
                C3Vector axis(0.0f, 0.0f, 1.0f);
                model->SetBoneProceduralTransform(4, C44Matrix::Rotation(angle, axis, true));
                remaining = remaining - twist;
            }

            if (this->m_animationState & 0x100) {
                float twist = 0.7853982f <= remaining ? 0.7853982f : remaining;
                float angle = delta < 0.0f ? -twist : twist;
                this->m_worldModel->SetBoneFlags(6, 0x80, 0x80);
                CM2Model* model = this->m_worldModel;
                C3Vector axis(0.0f, 0.0f, 1.0f);
                model->SetBoneProceduralTransform(6, C44Matrix::Rotation(angle, axis, true));
            }
        }
    }

    if (clearTwist) {
        this->m_worldModel->SetBoneFlags(4, 0, 0x80);
        this->m_worldModel->SetBoneFlags(6, 0, 0x80);
    }

    if (pitchBone) {
        this->m_worldModel->SetBoneFlags(4, 0x80, 0x80);
        C3Vector axis(0.0f, 1.0f, 0.0f);
        CM2Model* model = this->m_worldModel;
        model->SetBoneProceduralTransform(4, C44Matrix::Rotation(-this->GetPitch(), axis, true));
    }

    this->RotateWheels();
    this->m_animationState &= ~0x1800;

    if (!(this->m_movementData.m_flags & 0x2E0100F) && (this->m_obj->m_guid != ClntObjMgrGetActivePlayer() || !static_cast<CGPlayer_C*>(this)->m_lootTarget.m_guid) && !this->GetClientStandState()) {
        if (turn > 0.00001f) {
            this->m_animationState |= 0x800;
        } else if (turn < -0.00001f) {
            this->m_animationState |= 0x1000;
        }

        uint32_t sequence = this->GetObjectModel()->GetBoneSequenceId(0xFFFFFFFF);
        uint32_t flags = this->m_movement->m_flags;
        uint32_t wanted = sequence;

        if ((flags & 0x10) || (this->m_animationState & 0x800)) {
            wanted = 11;
        } else if ((flags & 0x20) || (this->m_animationState & 0x1000)) {
            wanted = 12;
        } else if (sequence == 11 || sequence == 12) {
            wanted = 0;
        }

        // WoWFix335 npc-turn-attack-anim: original calls UpdateBaseAnimation(0, -1) when CanPlayTurnAnim() is nonzero
        if (sequence != wanted) {
            this->CanPlayTurnAnim();
        }
    }

    CM2Model* worldModel = this->m_worldModel;

    if (worldModel) {
        worldModel->m_fFlags ^= (worldModel->m_fFlags ^ (static_cast<uint8_t>(this->m_unit->m_unitFieldFlags2 >> 1) << 14)) & 0x4000;
    }

    this->UpdateDelayedSpellVisualKits();
}

// VERIFY: 8308766d manual
// OFFSET: 0x0071D940
int32_t IsEmoteAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);
    int32_t behavior = rec ? rec->m_behaviorID : 506;

    switch (behavior) {
        case 60:
        case 61:
        case 62:
        case 63:
        case 64:
        case 65:
        case 66:
        case 67:
        case 68:
        case 69:
        case 70:
        case 71:
        case 72:
        case 73:
        case 74:
        case 76:
        case 77:
        case 78:
        case 80:
        case 81:
        case 82:
        case 83:
        case 84:
        case 113:
        case 136:
        case 137:
        case 138:
        case 178:
        case 185:
        case 186:
        case 195:
            return 1;
        default:
            return 0;
    }
}

// VERIFY: 8dffe07d manual
// OFFSET: 0x0071D380
int32_t IsSpellCastAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);
    int32_t behavior = rec ? rec->m_behaviorID : 506;

    switch (behavior) {
        case 2:
        case 32:
        case 33:
        case 53:
        case 54:
            return 1;
        default:
            return 0;
    }
}

// VERIFY: ae898f98 manual
// OFFSET: 0x0071DA20
int32_t IsThrownWeaponAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec) {
        int32_t behavior = rec->m_behaviorID;

        if (behavior == 107 || (behavior > 110 && behavior <= 112)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: c7728f19 auto
// OFFSET: 0x0071DA60
int32_t IsBowAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec) {
        int32_t behavior = rec->m_behaviorID;

        if (behavior == 46 || behavior == 105 || behavior == 109) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 2e108049 auto
// OFFSET: 0x0071DAA0
int32_t IsRifleAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec) {
        int32_t behavior = rec->m_behaviorID;

        if (behavior == 49 || behavior == 106 || behavior == 110) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: a4dab725 auto
// OFFSET: 0x0071DE90
int32_t CGUnit_C::CanPlayTurnAnim() {
    if (!(this->m_movementData.m_flags & 0x30) && !(this->m_animationState & 0x1800)) {
        return 0;
    }

    if (this->m_movementData.IsSplineFlyerOrHoveringFlyingOrSwimming()) {
        return 0;
    }

    if (this->m_vehicle && this->m_vehicle->m_vehicleRec && this->m_vehicle->HasHighPriorityAnim()) {
        return 0;
    }

    uint32_t animId = this->GetCurrentTorsoAnimId();

    if (IsEmoteAnim(animId) || IsSpellCastAnim(animId) || IsThrownWeaponAnim(animId) || IsBowAnim(animId) || IsRifleAnim(animId) || (this->m_animationState & 0x40000C)) {
        return 0;
    }

    return 1;
}

// VERIFY: 150c4def manual
// OFFSET: 0x0071FA90
void CGUnit_C::UpdateBreathState(uint32_t time) {
    int32_t liquidType;
    float height;
    int32_t a4;
    this->m_clientFlags &= ~0x60;

    if (World::QueryObjectLiquid(this->m_worldObject, &liquidType, &height, &a4)) {
        float headHeight = this->m_height * this->m_scale;

        if (headHeight + UNDERWATER_BUBBLE_THRESHOLD < height - this->GetPosition().z) {
            this->m_clientFlags |= 0x20;
            this->m_nextBreathCheckTime = time + 10000;
            return;
        }
    }

    if (AreaListZoneHasBreathParticles(this->m_worldObject)) {
        this->m_clientFlags |= 0x40;
    }

    this->m_nextBreathCheckTime = time + 10000;
}

// VERIFY: 36b52055 manual
// OFFSET: 0x00720DB0
void CGUnit_C::UpdateModelTint(uint32_t time) {
    CImVector color;
    color.b = 0xFF;
    color.g = 0xFF;
    color.r = 0xFF;
    color.a = 0xFF;

    if (!this->GetFlashColor(time, color) && this->m_procEffects) {
        color = this->m_procEffects->m_color;
    }

    float r = color.r * 0.0039215689f;
    float g = color.g * 0.0039215689f;
    float b = 0.0039215689f * color.b;
    CM2Model* model = this->GetObjectModel();
    model->m_diffuseTintR = r;
    model->m_diffuseTintG = g;
    model->m_diffuseTintB = b;
}

// VERIFY: 179faf70 manual
// OFFSET: 0x0073ADC0
void CGUnit_C::ProcessEmoteQueue() {
    if (this->m_emoteQueue[0].m_emote == -1) {
        return;
    }

    uint32_t index;

    for (index = 0; index < 4; index++) {
        if (this->m_emoteQueue[index].m_emote) {
            break;
        }
    }

    if (index == 4) {
        this->m_animationState |= 0x8000;
        this->m_emoteQueue[0].m_emote = -1;
        return;
    }

    if (this->m_emoteQueue[index].m_delay > OsGetAsyncTimeMs()) {
        return;
    }

    EmotesRec* emote = g_emotesDB.GetRecord(this->m_emoteQueue[index].m_emote);
    this->PlayBaseAnimation(emote->m_animID, 0);
    this->m_emoteQueue[index].m_emote = 0;

    if (index < 3 && this->m_emoteQueue[index + 1].m_emote) {
        M2SequenceInfo info;
        info.Clear();
        this->m_worldModel->GetSequenceInfo(emote->m_animID, 0, info);
        this->m_emoteQueue[index + 1].m_delay += OsGetAsyncTimeMs() + info.m_duration + 500;
    } else {
        this->m_animationState |= 0x8000;
        this->m_emoteQueue[0].m_emote = -1;
    }
}

// VERIFY: b3e7f0e3 auto
// OFFSET: 0x00719660
void CGUnit_C::SmoothFacingAngle(float facing) {
    float target = this->m_smoothTarget.m_value;

    if (target + 3.1415927f < this->m_renderFacing) {
        target = target + 6.2831855f;
    } else if (target - 3.1415927f > this->m_renderFacing) {
        target = target - 6.2831855f;
    }

    if (this->m_renderFacing + 1.5704823f < target) {
        this->m_renderFacing = target - 1.5704823f;
    } else if (this->m_renderFacing - 1.5704823f > target) {
        this->m_renderFacing = target + 1.5704823f;
    }

    float render = CMath::normalizeAnglePi(this->m_renderFacing);
    this->m_renderFacing = render;

    if (facing + 3.1415927f < render) {
        facing = facing + 6.2831855f;
    } else if (facing - 3.1415927f > render) {
        facing = facing - 6.2831855f;
    }

    float elapsed = CGWorldFrame::s_currentWorldFrame->m_elapsedSec;
    float k = elapsed * 20.0f;
    float damping = 1.0f / (k + (k * k * k * 0.235f + k * k * 0.48f) + 1.0f);
    float offset = this->m_renderFacing - facing;
    float change = elapsed * (offset * 20.0f + this->m_facingVelocity);
    this->m_renderFacing = (offset + change) * damping + facing;
    this->m_facingVelocity = damping * (this->m_facingVelocity - 20.0f * change);
    this->m_renderFacing = facing * (1.0f - this->m_facingBlend) + this->m_renderFacing * this->m_facingBlend;
}

// VERIFY: a0432dd6 manual
// OFFSET: 0x0071BD20
void CGUnit_C::RotateWheels() {
    if (!this->m_wheelMask) {
        return;
    }

    CM2Model* model = this->GetObjectModel();
    int32_t elapsedMs = FrameTime::s_curTimeMs - this->m_lastWheelUpdateTime;

    if (elapsedMs <= 1) {
        elapsedMs = 1;
    } else if (elapsedMs >= 500) {
        elapsedMs = 500;
    }

    CMovement_C* movement = this->m_movement;
    this->m_lastWheelUpdateTime = FrameTime::s_curTimeMs;
    uint32_t flags = movement->m_flags;
    float elapsed = elapsedMs * 0.001f;
    float distance;

    if (flags & 0xD) {
        distance = movement->m_currentSpeed * elapsed;
    } else if (flags & 0x2) {
        distance = -(movement->m_currentSpeed * elapsed);
    } else {
        distance = 0.0f;
    }

    float turn;

    if (flags & 0x10) {
        turn = (-movement->m_turnRate - this->m_smoothTarget.m_velocity) * elapsed;
    } else if (flags & 0x20) {
        turn = (movement->m_turnRate - this->m_smoothTarget.m_velocity) * elapsed;
    } else {
        turn = -(elapsed * this->m_smoothTarget.m_velocity);
    }

    CMoveSpline* spline = movement->m_spline;

    if ((spline && !(spline->m_flags & 0x400) && (spline->m_flags & 0x2000)) || (flags & 0x2200000)) {
        for (int32_t i = 0; i < 8; i++) {
            if ((1 << i) & this->m_wheelMask) {
                model->SetBoneFlags(i + 27, 0x80, 0x80);
                C3Vector axis(0.0f, 1.0f, 0.0f);
                model->SetBoneProceduralTransform(i + 27, C44Matrix::Rotation(this->m_wheelAngles[i], axis, true));
            }
        }
    } else {
        for (int32_t i = 0; i < 8; i++) {
            if ((1 << i) & this->m_wheelMask) {
                C3Vector pivot = model->GetBonePivot(i + 27);

                if (fabsf(pivot.z) > 0.001f) {
                    C44Matrix transform = model->m_worldTransform;
                    float scale = sqrtf(transform.a1 * transform.a1 + transform.a0 * transform.a0 + transform.a2 * transform.a2);
                    pivot.x = pivot.x * scale;
                    pivot.y = pivot.y * scale;
                    pivot.z = pivot.z * scale;
                    this->m_wheelAngles[i] = (pivot.y * turn + distance) / pivot.z + this->m_wheelAngles[i];

                    while (this->m_wheelAngles[i] < -3.1415927f) {
                        this->m_wheelAngles[i] = this->m_wheelAngles[i] + 6.2831855f;
                    }

                    while (this->m_wheelAngles[i] > 3.1415927f) {
                        this->m_wheelAngles[i] = this->m_wheelAngles[i] - 6.2831855f;
                    }
                }

                model->SetBoneFlags(i + 27, 0x80, 0x80);
                C3Vector axis(0.0f, 1.0f, 0.0f);
                model->SetBoneProceduralTransform(i + 27, C44Matrix::Rotation(this->m_wheelAngles[i], axis, true));
            }
        }
    }
}

// VERIFY: 30525646 manual
// OFFSET: 0x0071CBA0
void CGUnit_C::CreateRipple(uint32_t a2) {
    int32_t liquidType;
    float liquidHeight;
    int32_t a4;

    if (!World::QueryObjectLiquid(this->m_worldObject, &liquidType, &liquidHeight, &a4)) {
        return;
    }

    int32_t flag8;
    int32_t flag9;
    World::QueryObjectLiquidFlags(this->m_worldObject, &flag8, &flag9);

    if (!flag8) {
        return;
    }

    uint32_t type = 0;

    if (a2) {
        this->m_nextRippleTime = 0;

        if (a2 == 201) {
            type = 3;
        }
    } else {
        uint32_t flags = this->m_movement->m_flags;

        if (flags & 0xF) {
            type = 2;
        } else if (flags & 0x30) {
            type = 1;
        }
    }

    float maxDepth = this->m_movementData.m_collisionHeight + this->m_movementData.m_collisionHeight;

    if (maxDepth <= 1.0f) {
        maxDepth = 1.0f;
    }

    float halfDepth = maxDepth * 0.5f;
    float depth = liquidHeight - this->GetPosition().z;

    if (!(depth < maxDepth)) {
        return;
    }

    uint32_t time = FrameTime::s_curTimeMs;

    if (this->m_nextRippleTime && static_cast<int32_t>(time - this->m_nextRippleTime) < 0) {
        return;
    }

    C3Vector position = this->GetPosition();
    position.z = liquidHeight;
    float speedFactor = 1.0f;
    float speed = this->m_movement->m_currentSpeed;

    if (s_rippleDirectional[type] && speed > 0.0001f) {
        if (speed >= 20.0f) {
            speedFactor = 2.5f / 20.0f;
        } else {
            speedFactor = 2.5f / speed;
        }
    }

    float invSpeed = 1.0f / speedFactor;
    float facing = this->GetFacing();
    float scale = this->GetScale() * 0.33333334f;
    float size = (CRandom::ufloat_(g_rndSeed) * 0.2f - 0.1f + 1.0f) * scale;

    if (size < 0.33333334f) {
        size = 0.33333334f;
    } else if (size >= 1.6666666f) {
        size = 1.6666666f;
    }

    float alpha = CRandom::ufloat_(g_rndSeed) * 0.1f - 0.05f + 0.65f;
    float rippleSpeed = (CRandom::ufloat_(g_rndSeed) * 1.5f - 0.75f + 3.75f) * 12.0f * 0.027777778f * invSpeed;
    float width = 0.16666667f;
    float depthFactor = 1.0f;

    if (depth > halfDepth) {
        depthFactor = (maxDepth - depth) / (maxDepth - halfDepth) * 0.5f + 0.5f;
        width = 0.16666667f * depthFactor;
        alpha = depthFactor * alpha;
        size = depthFactor * size;
    }

    if (!type) {
        width = width * 0.8f;
        rippleSpeed = rippleSpeed * 0.25f;
        size = size * 0.6f;
    }

    uint8_t directional = s_rippleDirectional[type];
    float angle;

    if (directional) {
        angle = facing;
        uint32_t flags = this->m_movement->m_flags;

        if (flags & 0x4) {
            if (flags & 0x1) {
                angle = angle + 0.7853982f;
            } else if (flags & 0x2) {
                angle = angle + 2.3561945f;
            } else {
                angle = angle + 1.5707964f;
            }
        } else if (flags & 0x8) {
            if (flags & 0x1) {
                angle = angle - 0.7853982f;
            } else if (flags & 0x2) {
                angle = angle - 2.3561945f;
            } else {
                angle = angle - 1.5707964f;
            }
        } else if (flags & 0x2) {
            angle = angle + 3.1415927f;
        }
    } else {
        angle = CRandom::ufloat_(g_rndSeed) * 6.2831855f;
    }

    World::CreateWaterRipple(position, angle, size, alpha, width, rippleSpeed, directional, this->m_obj->m_guid == CGUnit_C::s_activeMover);

    if (s_rippleDirectional[type]) {
        this->m_nextRippleTime = time - static_cast<int32_t>(depthFactor * speedFactor * 0.25f * -1000.0f);
    } else {
        this->m_nextRippleTime = static_cast<uint32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * 50) >> 32) + time + 400;
    }
}

// VERIFY: 1f29ad42 manual
// OFFSET: 0x00728140
void CGUnit_C::UpdateDelayedSpellVisualKits() {
    uint32_t count = this->m_delayedSpellVisualKits.m_count;
    int32_t pending = 0;

    if (!count) {
        return;
    }

    for (uint32_t i = 0; i < count; i++) {
        DelayedSpellVisualKit* delayed = &this->m_delayedSpellVisualKits.m_data[i];

        if (!(delayed->m_flags & 0x8)) {
            continue;
        }

        if (static_cast<int32_t>(FrameTime::s_curTimeMs - delayed->m_startTime) < 0) {
            pending++;
            continue;
        }

        if (delayed->unk_0028 == 1) {
            SpellRec spellRec;

            if (g_spellDB.GetRecord(delayed->m_spellId, &spellRec)) {
                SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(delayed->m_kitId);

                if (kit) {
                    PlaySpellVisuaKitData data(&spellRec, kit, delayed->m_kitType);

                    if (delayed->m_flags & 0x1) {
                        data.m_position = &delayed->m_position;
                        data.m_transportGUID = delayed->m_targetGuid;
                    }

                    data.m_playAnim = (delayed->m_flags >> 1) & 0x1;
                    data.m_harmful = (delayed->m_flags >> 2) & 0x1;
                    data.unused_0020 = 1;
                    data.unused_0024 = delayed->m_delay;
                    this->PlaySpellVisualKit(data);
                }
            }
        }

        delayed->m_flags &= ~0x8;
    }

    if (!pending && count) {
        this->m_delayedSpellVisualKits.SetCount(0);
    }
}

// VERIFY: 3961fdbc manual
// OFFSET: 0x00729F40
void CGUnit_C::RefreshInteractIcon() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    if (this->UnitReaction(player) < UNIT_REACTION_NEUTRAL) {
        this->m_questStatus = 0;
        this->m_interactIconType = 0;
        this->RemoveInteractIcon();
        return;
    }

    if (this->IsQuestGiver()) {
        player->UpdateQuestStatus(this);
    } else {
        this->m_questStatus = 0;
    }

    if (this->m_unit->m_unitNpcFlags & 0x2000) {
        player->UpdateTaxiStatus(this);
    } else {
        this->m_interactIconType = 0;
    }

    if (!this->IsQuestGiver() && !(this->m_unit->m_unitNpcFlags & 0x2000)) {
        this->RemoveInteractIcon();
    }
}

// VERIFY: 136d61d4 auto
// OFFSET: 0x00724D90
void CGUnit_C::OnPvPFlagChanged(uint8_t oldFlags) {
    if (oldFlags == this->m_unit->m_unitFieldBytes2[1]) {
        return;
    }

    this->UpdateReaction(false);

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && (this->m_unit->m_unitFieldBytes2[1] & 0x1)) {
        CGTutorial::TriggerTutorial(31);
    }
}

// VERIFY: 631e67e1 auto
// OFFSET: 0x00724DF0
void CGUnit_C::ScaleChangeUpdate() {
    this->UpdateSelectionRadius();
    this->UpdateEffectAttachments();
    this->UpdateInteractIconScale();

    if (this->m_vehicle && this->m_vehicle->m_vehicleRec) {
        this->m_vehicle->UpdateLargestPassengerBoundsRadius();
    }
}

// VERIFY: 42cc9506 auto
// OFFSET: 0x00724E20
void CGUnit_C::ScaleChangeFinished() {
    if (this->m_nameDesc) {
        PlayerNameTriggerNameRegenerate(this->m_nameDesc);
    }

    this->UpdateSelectionRadius();
    this->UpdateEffectAttachments();
    this->UpdateInteractIconScale();

    if (this->m_vehicle && this->m_vehicle->m_vehicleRec) {
        this->m_vehicle->UpdateLargestPassengerBoundsRadius();
    }
}

// VERIFY: 98d1f3d2 manual
// OFFSET: 0x007275C0
void CGUnit_C::RenderAutoTrackCursor() {
    if (this->m_obj->m_guid != CGUnit_C::s_activeMover || CGUnit_C::s_trackingType == TRACKTYPE_NONE || (CGUnit_C::s_trackingType != TRACKTYPE_MOVE && CGUnit_C::s_trackingType != TRACKTYPE_ATTACK)) {
        return;
    }

    int32_t phase = OsGetAsyncTimeMs() % s_trackCursorPeriod;
    float size = (sinf(phase * 6.2831855f / static_cast<int32_t>(s_trackCursorPeriod)) + 1.0f) * 0.5f * (s_trackCursorMaxSize - s_trackCursorMinSize) + s_trackCursorMinSize;
    C3Vector position = this->m_movementData.GetPosition(CGUnit_C::s_trackingPosition);
    CAaBox box(C3Vector(position.x - size, position.y - size, position.z - size), C3Vector(position.x + size, position.y + size, position.z + size));
    C44Matrix matrix;
    this->BuildSelectionRotMatrix(&matrix, &CGUnit_C::s_trackingPosition);
    g_theGxDevicePtr->RsPush();

    if (this->SetCircleRenderStates()) {
        CImVector color;
        color.b = 0x7F;
        color.g = 0xFF;
        color.r = 0xFF;
        color.a = 0xFF;
        ProjectTex::Render2d(box, color, &matrix, 0.5f, 0x200122, 1, 0.4f);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: dc8d76e1 auto
// OFFSET: 0x0072B7E0
void CGUnit_C::Render() {
    this->RenderAutoTrackCursor();
}

// VERIFY: e73a5793 manual
// OFFSET: 0x0073D2B0
void CGUnit_C::PlayFallLandAnimation(uint32_t oldFlags, int32_t jumping) {
    uint32_t landing = this->m_animationState & 0x1000000;
    this->m_animationState &= ~0x1000000;

    if (this->m_vehicle && this->m_vehicle->m_vehicleRec && this->m_vehicle->HasHighPriorityAnim()) {
        return;
    }

    if (this->m_vehiclePassenger && (this->m_vehiclePassenger->m_state == 4 || this->m_vehiclePassenger->m_state == 5)) {
        return;
    }

    if (this->m_animationState & 0x4000000) {
        this->PlayBaseAnimation(ANIM_DEATH_END, 0);
        return;
    }

    if (jumping || (oldFlags & MOVEMENTFLAG_FALLING_FAR) || landing) {
        if (!(this->m_movement->m_flags & 0x2200000)) {
            uint32_t flags = this->m_movementData.m_flags;

            if (!(flags & 0xF)) {
                this->PlayUnitSound(UNITSOUNDTYPE_JUMPEND, 1);
                this->PlayBaseAnimation(ANIM_JUMP_END, 0);
                return;
            }

            if (!(flags & 0x2) && !(flags & 0x100) && !this->IsRunning()) {
                this->PlayUnitSound(UNITSOUNDTYPE_JUMPEND, 1);
                this->PlayBaseAnimation(ANIM_JUMP_LAND_RUN, 0);
                return;
            }
        }
    } else if (!((this->m_movement->m_flags ^ oldFlags) & 0x40F)) {
        return;
    }

    this->UpdateBaseAnimation(false, -1);
}

// VERIFY: 8a0e5647 manual
// OFFSET: 0x0073D3D0
void CGUnit_C::OnCollideFallLand(uint32_t a1, int32_t a2) {
    this->PlayFallLandAnimation(a1, a2);

    if (!this->IsClientControlled() || this->m_movementData.IsSplineFlyerOrSwimmingOrSafeFalling()) {
        return;
    }

    float distance = this->m_movementData.GetDistanceFallen();

    if (distance >= 70.0f || (distance > 13.0f && !this->IsDeadOrGhost())) {
        if (!(this->m_obj->m_type & TYPE_PLAYER) || (!(static_cast<CGPlayer_C*>(this)->m_player->m_playerFlags & 0x4000) && this->m_unit->m_unitFieldHealth > 0)) {
            this->PlayUnitSound(UNITSOUNDTYPE_INJURY_FALL, 1);
        }

        this->HandleEnvironmentDamage(2, 0, 0, 0);
    }
}

// VERIFY: ccff415d auto
// OFFSET: 0x0073D4A0
int32_t CGUnit_C::OnCollideFallLandNotify(uint32_t time, uint32_t oldFlags, uint32_t oldFlags2, int32_t jumping) {
    this->OnCollideFallLand(oldFlags, jumping);
    int32_t sent = 0;

    if (((this->m_clientFlags & 0x80) || this->m_movementData.GetDistanceFallen() > 0.027777778f) && this->SendMovementUpdate(time, MSG_MOVE_FALL_LAND, 0.0f, 0, 0, 0xFF)) {
        sent = 1;
    }

    if (this->m_clientFlags & 0x100000) {
        this->m_clientFlags &= ~0x100000;
        static_cast<CGPlayer_C*>(this)->HandleRepopRequest(true);
    }

    return sent;
}

// VERIFY: dabb5164 manual
// OFFSET: 0x0071F2C0
void CGUnit_C::OnCollideFallReset(uint32_t time) {
    if (this->m_clientFlags & 0x80) {
        this->SendMovementUpdate(time, CMSG_MOVE_FALL_RESET, 0.0f, 0, 0, 0xFF);
    }
}

// VERIFY: d938b9d8 manual
// OFFSET: 0x0071F210
void CGUnit_C::SendSplineDone(uint32_t time, uint32_t splineId) {
    CDataStore msg;
    this->BuildMovementUpdate(time, CMSG_MOVE_SPLINE_DONE, &msg, 0.0f, 0);
    msg.Put(splineId);
    msg.Finalize();
    ClientServices::Send2(&msg);
    this->m_movementData.UpdateHeartbeatTimerA(time);
    CGInputControl* input = CGInputControl::GetActive();
    input->UpdateHalted();
    input->UpdatePlayer(time, 1);
}

// VERIFY: 0a448404 manual
// OFFSET: 0x0071D590
int32_t IsMeleeAttackAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);
    int32_t behavior = rec ? rec->m_behaviorID : 506;

    switch (behavior) {
        case 10:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 30:
        case 36:
        case 57:
        case 58:
        case 59:
        case 85:
        case 86:
        case 87:
        case 88:
        case 95:
        case 117:
        case 118:
        case 170:
        case 171:
        case 172:
        case 173:
        case 174:
        case 175:
        case 176:
        case 177:
        case 178:
        case 179:
        case 212:
            return 1;
        default:
            return 0;
    }
}

// VERIFY: 27ef24ab auto
// OFFSET: 0x00714CE0
void ApplyKitAnimFlags(uint32_t kitFlags, uint32_t* animFlags) {
    if (!animFlags) {
        return;
    }

    if (kitFlags & 0x4) {
        *animFlags |= 0x2;
    }

    if (kitFlags & 0x8) {
        *animFlags |= 0x4;
    }

    if (kitFlags & 0x10) {
        *animFlags |= 0x8;
    }

    if (kitFlags & 0x100) {
        *animFlags |= 0x20;
    }
}

// VERIFY: f4f51cb2 auto
// OFFSET: 0x0071DFC0
int32_t CGUnit_C::HasAnim466() {
    uint32_t animId = this->ResolveAnimForModel(ANIM_DEATH_START, nullptr);
    return this->m_worldModel && this->m_worldModel->HasSequence(animId);
}

// VERIFY: 6ce0c56f manual
// OFFSET: 0x00724060
int32_t CGUnit_C::ChooseDeathAnim(uint32_t mask, uint32_t* anim, uint32_t* animFlags) {
    if (!this->IsDeadOrAttachmentDying()) {
        return (mask & ~0x1) == 0;
    }

    if (this->m_vehiclePassenger && this->m_vehiclePassenger->m_state == 3 && !(this->m_unit->m_unitFieldFlags2 & 0x20000)) {
        return (mask & ~0x1) == 0;
    }

    uint32_t flags = 0;

    for (CEffect* effect = this->m_effectList; effect; effect = effect->m_next) {
        const SpellVisualKitRec* kit = effect->m_kitRec;

        if (kit && IsDeadOrDyingAnim(kit->m_animID)) {
            if (kit->m_flags & 0x4) {
                flags |= 0x2;
            }

            if (kit->m_flags & 0x8) {
                flags |= 0x4;
            }

            if (kit->m_flags & 0x10) {
                flags |= 0x8;
            }

            if (kit->m_flags & 0x100) {
                flags |= 0x20;
            }

            if (flags & 0x2) {
                *anim = effect->m_kitRec->m_animID;

                if (animFlags) {
                    *animFlags |= 0x2;
                }

                return 1;
            }
        }
    }

    uint32_t torsoAnim = this->GetCurrentTorsoAnimId();

    if ((this->m_animationState & 0x4000000) && IsDeadOrDyingAnim(torsoAnim)) {
        return 1;
    }

    if (this->HasAnim466()) {
        if ((this->m_movement->m_flags & MOVEMENTFLAG_FALLING) || this->m_movementData.IsOnFallingSpline() || this->m_movementData.IsOnParabolicSpline()) {
            *anim = ANIM_DEATH_START;
        } else {
            *anim = ANIM_DEATH_END_HOLD;
        }

        return 1;
    }

    uint32_t moveFlags = this->m_movementData.m_flags;

    if ((!(moveFlags & MOVEMENTFLAG_FALLING) || !(moveFlags & MOVEMENTFLAG_FALLING_FAR)) && !IsDyingAnim(torsoAnim) && (mask & 0x1)) {
        *anim = (moveFlags & MOVEMENTFLAG_SWIMMING) ? 132 : 6;
    }

    return 1;
}

// VERIFY: 4075ac71 manual
// OFFSET: 0x007224D0
int32_t CGUnit_C::GetSpellVisualKitAnim(uint32_t* anim, uint32_t* kitFlags) {
    if (this->m_specialIdleAnim != -1) {
        return 0;
    }

    bool casting = true;
    SpellRec spellRec;

    if (!g_spellDB.GetRecord(this->m_castSpellId, &spellRec)) {
        casting = false;

        if (!g_spellDB.GetRecord(this->m_unit->m_unitChannelSpell, &spellRec)) {
            return 0;
        }
    }

    const SpellVisualRec* visual = GetSpellVisual(&spellRec);

    if (!visual) {
        return 0;
    }

    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(casting ? visual->m_precastKit : visual->m_channelKit);

    if (!kit) {
        return 0;
    }

    int32_t kitAnim;

    if (!casting || !(this->m_animationState & 0x10000) || (kitAnim = kit->m_startAnimID) <= 0) {
        kitAnim = kit->m_animID;

        if (kitAnim <= 0) {
            return 0;
        }
    }

    if (kitFlags) {
        *kitFlags = kit->m_flags;
    }

    *anim = kitAnim;
    return 1;
}

// VERIFY: 2c7afb93 auto
// OFFSET: 0x00724330
int32_t CGUnit_C::ChooseSpellVisualKitAnim(uint32_t mask, uint32_t* anim, uint32_t* animFlags) {
    if ((!this->m_unit->m_unitChannelSpell && !(this->m_animationState & 0x400)) || !(this->m_animationState & 0x60) || this->m_mountModel) {
        return (mask & ~0x1F) == 0;
    }

    uint32_t kitAnim;
    uint32_t kitFlags = 0;

    if (!this->GetSpellVisualKitAnim(&kitAnim, &kitFlags)) {
        return (mask & ~0x1F) == 0;
    }

    ApplyKitAnimFlags(kitFlags, animFlags);

    if (kitAnim == this->GetCurrentTorsoAnimId() && this->GetCurrentTorsoAnimId() != this->GetCurrentBaseAnimId()) {
        *anim = kitAnim;
        return 1;
    }

    if (kitAnim != this->GetCurrentTorsoAnimId()) {
        if (mask & 0x10) {
            *anim = kitAnim;
        }

        return 1;
    }

    if ((*animFlags & 0x10) && (kitFlags & 0x40)) {
        *anim = kitAnim;
    }

    return 1;
}

// VERIFY: a6f466cd auto
// OFFSET: 0x0071E0D0
int32_t CGUnit_C::ChooseCombatAnim(uint32_t mask, uint32_t* anim, uint32_t* animFlags) {
    if (!(this->m_animationState & 0x60) || !this->m_combat.m_attacking.m_guid) {
        return (mask & ~0x3F) == 0;
    }

    if (!(mask & 0x20)) {
        return 1;
    }

    if (animFlags && (*animFlags & 0x40)) {
        uint32_t torsoAnim = this->GetCurrentTorsoAnimId();

        if (IsMeleeAttackAnim(torsoAnim)) {
            *anim = torsoAnim;
            return 1;
        }
    }

    int32_t meleeAnim = this->m_meleeAttackAnimId;

    if (meleeAnim == -1) {
        if (this->m_movement->m_flags & 0x2200000) {
            *anim = 41;
            return 1;
        }

        meleeAnim = this->GetCombatReadyAnim();
    }

    *anim = meleeAnim;
    return 1;
}

// VERIFY: ae3652c1 auto
// OFFSET: 0x0071E180
int32_t CGUnit_C::ChooseShuffleAnim(uint32_t mask, uint32_t* anim) {
    if (!this->CanPlayTurnAnim()) {
        return (mask & ~0x7F) == 0;
    }

    if ((this->m_animationState & 0x70) && (mask & 0x40)) {
        if (!(this->m_movementData.m_flags & MOVEMENTFLAG_TURN_LEFT) && !(this->m_animationState & 0x800)) {
            *anim = ANIM_SHUFFLE_RIGHT;
            return 1;
        }

        *anim = ANIM_SHUFFLE_LEFT;
    }

    return 1;
}

// VERIFY: acc35172 manual
// OFFSET: 0x0071E1F0
int32_t CGUnit_C::ChooseStandStateAnim(uint32_t mask, uint32_t* anim) {
    if (!(this->m_animationState & 0x40) || this->m_mountModel) {
        return (mask & ~0x1FF) == 0;
    }

    uint32_t standAnim;

    switch (this->GetClientStandState()) {
        case UNIT_STANDING:
            switch (this->m_animStandState) {
                case UNIT_SITTING:
                    standAnim = 98;
                    break;
                case UNIT_SLEEPING:
                    standAnim = 101;
                    break;
                case UNIT_KNEEL:
                    standAnim = 116;
                    break;
                default:
                    return (mask & ~0x1FF) == 0;
            }

            break;
        case UNIT_SITTING:
            standAnim = (!this->m_animStandState || this->GetCurrentTorsoAnimId() == 96) ? 96 : 97;
            break;
        case UNIT_SLEEPING:
            standAnim = (this->m_animStandState != UNIT_STANDING) + 99;

            if (standAnim == ANIM_COUNT) {
                return 1;
            }

            break;
        case UNIT_SITTINGCHAIRLOW:
            standAnim = 102;
            break;
        case UNIT_SITTINGCHAIRMEDIUM:
            standAnim = 103;
            break;
        case UNIT_SITTINGCHAIRHIGH:
            standAnim = 104;
            break;
        case UNIT_DEAD:
            if (this->m_animStandState == UNIT_DEAD) {
                return 1;
            }

            if (this->m_movement->m_flags & MOVEMENTFLAG_SWIMMING) {
                standAnim = 132;
            } else {
                standAnim = this->HasAnim466() ? ANIM_DEATH_END_HOLD : 6;
            }

            break;
        case UNIT_KNEEL:
            standAnim = (this->m_animStandState != UNIT_STANDING) + 114;

            if (standAnim == ANIM_COUNT) {
                return 1;
            }

            break;
        default:
            return (mask & ~0x1FF) == 0;
    }

    if (mask & 0x100) {
        *anim = standAnim;
    }

    return 1;
}

// VERIFY: c45e9335 auto
// OFFSET: 0x00724500
uint32_t CGUnit_C::ChooseAnimation(uint32_t mask, uint32_t* animFlags, int32_t* usedDefault) {
    uint32_t anim = ANIM_COUNT;

    if (this->ChooseDeathAnim(mask, &anim, animFlags) || this->ChooseSpecialIdleAnim(mask, &anim) || (this->m_vehiclePassenger && this->m_vehiclePassenger->PickHighPriorityAnimation(mask, &anim))) {
        return anim;
    }

    if (this->m_vehicle && this->m_vehicle->m_vehicleRec && this->m_vehicle->HasHighPriorityCastingAnim()) {
        this->ChooseSpellVisualKitAnim(mask, &anim, animFlags);
        return anim;
    }

    if (this->ChooseSubmergeAnim(mask, &anim) || this->ChooseFallAnim(&anim) || this->ChooseMovementAnim(mask, &anim) || this->ChooseLootAnim(mask, &anim) || this->ChooseSpellVisualKitAnim(mask, &anim, animFlags) || this->ChooseCombatAnim(mask, &anim, animFlags) || this->ChooseShuffleAnim(mask, &anim) || this->ChooseRangedLoadAnim(mask, &anim) || this->ChooseStandStateAnim(mask, &anim) || this->ChooseEmoteAnim(mask, &anim) || (this->m_vehiclePassenger && this->m_vehiclePassenger->ChooseSeatExitAnim(mask, &anim))) {
        return anim;
    }

    if (this->m_pendingAnimId != -1) {
        return this->m_pendingAnimId;
    }

    if (usedDefault) {
        *usedDefault = 1;
    }

    if (this->m_awaitingEndAnimId == -1) {
        this->ChooseDefaultAnim(anim, 0);
    }

    return anim;
}

// VERIFY: bb382509 manual
// OFFSET: 0x0073AC30
void CGUnit_C::UpdateBaseAnimation(uint32_t animFlags, int32_t mask) {
    if (this->m_worldModel && !this->m_worldModel->IsLoaded(0, 0)) {
        return;
    }

    uint32_t anim = this->ChooseAnimation(mask, &animFlags, nullptr);

    if (anim == ANIM_COUNT) {
        if (this->m_vehiclePassenger && this->m_vehiclePassenger->m_state) {
            this->PlayBaseAnimation(ANIM_STAND, animFlags);
        }
    } else {
        if (static_cast<int32_t>(anim) == this->m_pendingAnimId) {
            this->m_awaitingEndAnimId = anim;
        }

        this->PlayBaseAnimation(anim, animFlags);
    }

    if (this->m_animationState & 0x40) {
        if (this->GetCurrentTorsoAnimId() == 135 && this->m_obj->m_guid == CGWorldFrame::GetActiveCamera()->m_targetGUID) {
            CGGameUI::ResetCamera(this->m_obj->m_guid);
        }

        this->m_animStandState = this->GetClientStandState();
    }
}

// VERIFY: be6affca manual
// OFFSET: 0x0071EE70
void CGUnit_C::ClearLockedTarget() {
    if (this->m_obj->m_guid != CGGameUI::m_lockedTarget) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_combat.m_attacking.m_guid) {
        player->SetCombatMode();
    }

    Spell_C_CancelAutoRepeat(true);

    if (!(this->m_unit->m_unitDynamicFlags & 0x20)) {
        CGGameUI::Target(WOWGUID{ 0 });
        return;
    }

    WOWGUID guid = this->m_obj->m_guid;

    if (!CGGameUI::IsRaidMemberOrPet(guid)) {
        CGGameUI::Target(WOWGUID{ 0 });
    }
}

// VERIFY: 0f134e5c auto
// OFFSET: 0x0073AF80
void CGUnit_C::PlayDeathAnimation(int32_t force) {
    if (!force && IsDeadOrDyingAnim(this->GetCurrentTorsoAnimId())) {
        return;
    }

    CMovement_C* movement = this->m_movement;
    uint32_t anim = ANIM_DEATH_START;

    if (movement->m_flags & MOVEMENTFLAG_SWIMMING) {
        anim = ANIM_DROWN;
    } else if (movement->m_spline && movement->IsSplineFlyerOrHovering()) {
        C3Vector position(0.0f, 0.0f, 0.0f);
        position = this->m_movementData.GetDestination();
        float standHeight = this->GetStandHeight();
        int32_t liquidType;
        float liquidHeight;

        if (World::QueryLiquidStatus(&position, &liquidType, &liquidHeight) && !(liquidHeight - standHeight * 0.5f + 0.1f < position.z)) {
            anim = ANIM_DROWN;
        }
    }

    this->PlayBaseAnimation(anim, 0);
}

// VERIFY: e532d825 manual
// OFFSET: 0x0073F060
void CGUnit_C::UpdateClientStandState(UNITSTANDSTATE standState) {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        static_cast<CGPlayer_C*>(this)->SetClientStandState(standState);
    }

    if (this->m_sheatheState && this->GetClientStandState() && this->GetClientStandState() != UNIT_SITTINGCHAIR) {
        this->SetSheatheState(WEAPONMODE_SHEATHEDMODE, 1, 0);
    }

    if (this->GetClientStandState() != this->m_animStandState) {
        switch (this->GetClientStandState()) {
            case UNIT_STANDING:
                if (this->m_animStandState == UNIT_SUBMERGED) {
                    if (!this->m_vehicle || !this->m_vehicle->m_vehicleRec || !this->m_vehicle->HasHighPriorityAnim()) {
                        CM2Model* model = this->GetObjectModel();

                        if (model && model->IsLoaded(0, 0)) {
                            if (model->HasSequence(ANIM_BIRTH)) {
                                this->PlayBaseAnimation(ANIM_BIRTH, 0);
                            } else {
                                this->PlayBaseAnimation(ANIM_EMERGE, 0);
                            }
                        }
                    }

                    break;
                }

                this->m_animationState |= 0x40;
                this->UpdateBaseAnimation(0, -1);
                break;
            case UNIT_DEAD:
                this->ClearLockedTarget();
                this->PlayDeathAnimation(0);
                break;
            case UNIT_SUBMERGED:
                if (!this->m_vehicle || !this->m_vehicle->m_vehicleRec || !this->m_vehicle->HasHighPriorityAnim()) {
                    this->PlayBaseAnimation(ANIM_SUBMERGE, 0);
                }

                break;
            default:
                this->m_animationState |= 0x40;
                this->UpdateBaseAnimation(0, -1);
                break;
        }

        this->m_animStandState = this->GetClientStandState();
    }

    if (this->m_obj->m_guid == CGWorldFrame::GetActiveCamera()->m_targetGUID) {
        CGGameUI::ResetCamera(this->m_obj->m_guid);
    }
}

// VERIFY: c0743a42 manual
// OFFSET: 0x0071F770
UNIT_REACTION CGUnit_C::UnitReaction(int32_t factionTemplate, const CGUnit_C* unit) {
    FactionTemplateRec* faction = g_factionTemplateDB.GetRecord(factionTemplate);
    CGUnitData* data = unit->m_unit;
    FactionTemplateRec* other = g_factionTemplateDB.GetRecord(data->m_unitFieldFactiontemplate);

    if (!faction || !other) {
        return UNIT_REACTION_NEUTRAL;
    }

    if (data->m_unitFieldFlags & 0x8) {
        CGUnit_C* controller = const_cast<CGUnit_C*>(unit)->GetControllingPlayer();

        if (controller && controller->IsActivePlayer()) {
            if ((faction->m_flags & 0x1000) && (static_cast<CGPlayer_C*>(controller)->m_player->m_playerFlags & 0x100)) {
                return UNIT_REACTION_HOSTILE;
            }

            UNIT_REACTION forcedReaction;

            if (CGReputationInfo::GetForcedReaction(faction->m_faction, &forcedReaction)) {
                return forcedReaction;
            }

            if (!(unit->m_unit->m_unitFieldFlags2 & 0x4)) {
                int32_t factionId = faction->m_faction;

                if (CGUnit_C::FactionHasReputation(factionId)) {
                    return CGReputationInfo::GetFactionStandingReaction(factionId);
                }
            }
        }
    }

    return FactionTemplateReaction(faction, other);
}

// VERIFY: 374d4818 manual
// OFFSET: 0x007251C0
UNIT_REACTION CGUnit_C::UnitReaction(const CGUnit_C* unit) const {
    if (unit == this) {
        return UNIT_REACTION_FRIENDLY;
    }

    CGUnit_C* self = const_cast<CGUnit_C*>(this);
    CGUnit_C* other = const_cast<CGUnit_C*>(unit);
    CGUnitData* data = this->m_unit;

    if ((data->m_unitFieldFlags & 0x8) && (unit->m_unit->m_unitFieldFlags & 0x8)) {
        CGUnit_C* controller = self->GetControllingPlayer();
        CGUnit_C* otherController = other->GetControllingPlayer();

        if (controller && otherController) {
            CGPlayerLocalData* player = static_cast<CGPlayer_C*>(controller)->m_player;

            if (player->m_playerDuelTeam) {
                CGPlayerLocalData* otherPlayer = static_cast<CGPlayer_C*>(otherController)->m_player;

                if (otherPlayer->m_playerDuelTeam && player->m_playerDuelArbiter == otherPlayer->m_playerDuelArbiter) {
                    return player->m_playerDuelTeam != otherPlayer->m_playerDuelTeam ? UNIT_REACTION_HOSTILE : UNIT_REACTION_FRIENDLY;
                }
            }

            if (controller == otherController) {
                return UNIT_REACTION_FRIENDLY;
            }

            if (controller->IsActivePlayer()) {
                WOWGUID guid = otherController->m_obj->m_guid;

                if (CGGameUI::IsInParty(guid)) {
                    return CGUnit_C::UnitReaction(this->m_unit->m_unitFieldFactiontemplate, unit);
                }

                guid = otherController->m_obj->m_guid;

                if (CGGameUI::IsPartyMember(guid)) {
                    return CGUnit_C::UnitReaction(this->m_unit->m_unitFieldFactiontemplate, unit);
                }
            }

            if (otherController->IsActivePlayer()) {
                WOWGUID guid = controller->m_obj->m_guid;

                if (CGGameUI::IsInParty(guid)) {
                    return CGUnit_C::UnitReaction(this->m_unit->m_unitFieldFactiontemplate, unit);
                }

                guid = controller->m_obj->m_guid;

                if (CGGameUI::IsPartyMember(guid)) {
                    return CGUnit_C::UnitReaction(this->m_unit->m_unitFieldFactiontemplate, unit);
                }
            }
        } else if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && !otherController) {
            if (self->IsDuelingWith(other)) {
                return UNIT_REACTION_HOSTILE;
            }
        } else if (unit->m_obj->m_guid == ClntObjMgrGetActivePlayer() && !controller) {
            if (other->IsDuelingWith(self)) {
                return UNIT_REACTION_HOSTILE;
            }
        }

        data = this->m_unit;

        if ((data->m_unitFieldBytes2[1] & 0x4) && (unit->m_unit->m_unitFieldBytes2[1] & 0x4)) {
            return UNIT_REACTION_HOSTILE;
        }
    }

    if (data->m_unitFieldFlags & 0x8) {
        CGUnit_C* controller = self->GetControllingPlayer();

        if (controller && controller->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            FactionTemplateRec* faction = g_factionTemplateDB.GetRecord(unit->m_unit->m_unitFieldFactiontemplate);

            if (faction) {
                UNIT_REACTION reaction;

                if (CGReputationInfo::GetForcedReaction(faction->m_faction, &reaction)) {
                    return reaction;
                }

                if (!(controller->m_unit->m_unitFieldFlags2 & 0x4) && CGUnit_C::FactionHasReputation(faction->m_faction)) {
                    if ((faction->m_flags & 0x1000) && (static_cast<CGPlayer_C*>(controller)->m_player->m_playerFlags & 0x100)) {
                        return UNIT_REACTION_HOSTILE;
                    }

                    return CGReputationInfo::IsAtWar(faction->m_faction) ? UNIT_REACTION_HOSTILE : UNIT_REACTION_FRIENDLY;
                }
            }
        }
    }

    UNIT_REACTION reaction = CGUnit_C::UnitReaction(this->m_unit->m_unitFieldFactiontemplate, unit);

    if (reaction >= UNIT_REACTION_EXALTED) {
        return UNIT_REACTION_EXALTED;
    }

    return reaction;
}

// VERIFY: 5eefb777 auto
// OFFSET: 0x007279A0
float CGUnit_C::GetPlayerDiscount(const CGUnit_C* unit) const {
    FactionTemplateRec* factionTemplate = g_factionTemplateDB.GetRecord(this->m_unit->m_unitFieldFactiontemplate);

    if (!factionTemplate) {
        return 0.0f;
    }

    FactionRec* faction = g_factionDB.GetRecord(factionTemplate->m_faction);

    if (!faction || faction->m_reputationIndex < 0) {
        return 0.0f;
    }

    switch (this->UnitReaction(unit)) {
        case UNIT_REACTION_FRIENDLY:
            return 0.05f;

        case UNIT_REACTION_HONORED:
            return 0.1f;

        case UNIT_REACTION_REVERED:
            return 0.15f;

        case UNIT_REACTION_EXALTED:
            return 0.2f;

        default:
            return 0.0f;
    }
}

// VERIFY: bf2201d9 manual
// OFFSET: 0x004FD0E0
const char* CGUnit_C::GetVisibleUnitName(const char** realm, int32_t a2) {
    if (!(this->m_unit->m_unitFieldFlags & 0x2000000)) {
        return this->GetUnitName(realm, a2);
    }

    uint32_t high = static_cast<uint32_t>(this->m_obj->m_guid.m_guid >> 32);

    if ((this->m_obj->m_type & TYPE_PLAYER) || ((high & 0xF0F00000) == 0xF0500000 && this->GetControllingPlayer())) {
        return this->GetUnitName(realm, a2);
    }

    return nullptr;
}

// VERIFY: 7c461897 auto
// OFFSET: 0x00514050
bool CGUnit_C::IsFriendly(const CGUnit_C* unit) const {
    return this->UnitReaction(unit) >= UNIT_REACTION_FRIENDLY || this->IsTreatAsRaidUnit();
}

// VERIFY: 698db9c9 auto
// OFFSET: 0x00514080
bool CGUnit_C::IsHatedTowards(const CGUnit_C* unit) const {
    return this->UnitReaction(unit) <= UNIT_REACTION_HOSTILE;
}

// VERIFY: c1172a2b manual
// OFFSET: 0x00729740
bool CGUnit_C::CanAttack(const CGUnit_C* unit) const {
    if ((this->m_obj->m_type & TYPE_PLAYER) && (static_cast<const CGPlayer_C*>(this)->m_player->m_playerFlags & 0x80000)) {
        return false;
    }

    if ((unit->m_obj->m_type & TYPE_PLAYER) && (static_cast<const CGPlayer_C*>(unit)->m_player->m_playerFlags & 0x10)) {
        if (!this->m_creatureCacheEntry || !(this->m_creatureCacheEntry->m_typeFlags & 0x2)) {
            return false;
        }
    }

    uint32_t targetFlags = unit->m_unit->m_unitFieldFlags;

    if ((targetFlags & 0x2) || (targetFlags & 0x100000) || (targetFlags & 0x80) || (targetFlags & 0x10000) || (targetFlags & 0x2000000)) {
        return false;
    }

    uint32_t flags = this->m_unit->m_unitFieldFlags;

    if ((flags & 0x8) && (unit->m_unit->m_unitFieldFlags & 0x100)) {
        return false;
    }

    if (!(flags & 0x8) && (unit->m_unit->m_unitFieldFlags & 0x200)) {
        return false;
    }

    if ((unit->m_unit->m_unitFieldFlags & 0x8) && (flags & 0x100)) {
        return false;
    }

    if (!(unit->m_unit->m_unitFieldFlags & 0x8) && (flags & 0x200)) {
        return false;
    }

    CGUnit_C* self = const_cast<CGUnit_C*>(this);
    CGUnit_C* other = const_cast<CGUnit_C*>(unit);

    if ((this->m_unit->m_unitFieldFlags2 & 0x10000) || (unit->m_unit->m_unitFieldFlags2 & 0x10000)) {
        const VehicleRec* vehicleRec = this->m_vehicle ? this->m_vehicle->m_vehicleRec : nullptr;
        const VehicleRec* otherVehicleRec = unit->m_vehicle ? unit->m_vehicle->m_vehicleRec : nullptr;

        if (vehicleRec && (vehicleRec->m_flags & 0x20000000) && other->FindRootVehicle(self) == self) {
            return false;
        }

        if (otherVehicleRec && (otherVehicleRec->m_flags & 0x20000000) && self->FindRootVehicle(other) == other) {
            return false;
        }
    }

    CGUnitData* data = this->m_unit;

    if (!(data->m_unitFieldFlags & 0x8) && !(unit->m_unit->m_unitFieldFlags & 0x8)) {
        return this->IsHatedTowards(unit) || unit->IsHatedTowards(this);
    }

    if ((data->m_unitFieldFlags & 0x8) && (unit->m_unit->m_unitFieldFlags & 0x8)) {
        bool result = false;

        if (!this->IsFriendly(unit)) {
            CGUnit_C* controller = self->GetControllingPlayer();
            CGUnit_C* otherController = other->GetControllingPlayer();

            if (controller && otherController) {
                CGPlayerLocalData* player = static_cast<CGPlayer_C*>(controller)->m_player;
                CGPlayerLocalData* otherPlayer = static_cast<CGPlayer_C*>(otherController)->m_player;

                if (player->m_playerDuelTeam && otherPlayer->m_playerDuelTeam && player->m_playerDuelArbiter == otherPlayer->m_playerDuelArbiter) {
                    result = true;
                } else {
                    uint8_t otherPvp = unit->m_unit->m_unitFieldBytes2[1];

                    if (otherPvp & 0x1) {
                        if (!(this->m_unit->m_unitFieldBytes2[1] & 0x8)) {
                            result = !(otherPvp & 0x8);
                        }
                    } else {
                        uint8_t pvp = this->m_unit->m_unitFieldBytes2[1];

                        if ((pvp & 0x4) && (otherPvp & 0x4)) {
                            result = true;
                        } else if (((pvp & 0x2) || (otherPvp & 0x2)) && !(pvp & 0x8)) {
                            result = !(otherPvp & 0x8);
                        }
                    }
                }
            } else if (!(this->m_unit->m_unitFieldBytes2[1] & 0x8)) {
                result = !(unit->m_unit->m_unitFieldBytes2[1] & 0x8);
            }
        }

        return result;
    }

    if ((data->m_unitFieldFlags & 0x8) && (unit->m_unit->m_unitFieldBytes2[1] & 0x8)) {
        return false;
    }

    if ((unit->m_unit->m_unitFieldFlags & 0x8) && (data->m_unitFieldBytes2[1] & 0x8)) {
        return false;
    }

    return !this->IsFriendly(unit);
}

// VERIFY: 99d67f65 auto
// OFFSET: 0x006E6EE0
const char* CGUnit_C::GetObjectName() const {
    return const_cast<CGUnit_C*>(this)->GetUnitName(nullptr, 1);
}

// VERIFY: dc8a6025 auto
// OFFSET: 0x006E6FA0
int32_t CGUnit_C::UpdateObjectNameString(int32_t flags, char* buffer, uint32_t size) {
    return CGUnit_C::UpdateObjectNameStringForUnit(this, flags, buffer, size, 1);
}

// VERIFY: c83d3c4d auto
// OFFSET: 0x00715560
void CGUnit_C::ObjectNameVisibilityChanged() {
    if (!this->m_interactIcon) {
        return;
    }

    if (PlayerNameShowing(this->m_nameDesc) || this->m_namePlateFrame) {
        this->m_interactIcon->SetBoneSequence(0xFFFFFFFF, 190, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    } else {
        this->m_interactIcon->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }
}

// VERIFY: 928c5d8c manual
// OFFSET: 0x0071A390
int32_t CGUnit_C::CanHighlight() const {
    CGUnitData* data = this->m_unit;

    if (data->m_unitFieldFlags & 0x2000000) {
        if (data->m_unitFieldCreatedby != ClntObjMgrGetActivePlayer()) {
            return 0;
        }

        if (this->m_obj->m_guid != CGPetInfo::GetPet(0)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 9904e3a0 auto
// OFFSET: 0x00718AC0
int32_t CGUnit_C::GetSelectionHighlightColor(CImVector* color) const {
    if (this->m_clientFlags & 0x10) {
        *color = *reinterpret_cast<const CImVector*>(s_attackTargetColor);
    } else {
        WOWGUID guid = this->m_obj->m_guid;
        CGGameUI::GetSelectionHighlightColor(guid, color, 1);
    }

    return 1;
}

// VERIFY: 187ccec0 manual
// OFFSET: 0x004F5130
float CalculateFacingTo(const C3Vector* from, const C3Vector* to) {
    float dx = to->x - from->x;
    float dy = to->y - from->y;

    if (2.3841858e-7f <= fabsf(dx)) {
        if (fabsf(dy) < 2.3841858e-7f) {
            return from->x <= to->x ? 0.0f : 3.1415927f;
        }

        return static_cast<float>(atan2(dy, dx));
    }

    return 0.0f <= dy ? 0.5f * 3.1415927f : 1.5f * 3.1415927f;
}

// VERIFY: b5a3e88a manual
// OFFSET: 0x0071B700
uint64_t CGUnit_C::GetComboPointTarget() const {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        return static_cast<const CGPlayer_C*>(this)->m_lootTarget.m_guid;
    }

    return 0;
}

// VERIFY: 98235d06 manual
// OFFSET: 0x00735EF0
void CGUnit_C::PropagateSmoothFacing(float facing) {
    for (CPassenger* passenger = this->m_vehicle->m_passengers.Head(); passenger; passenger = this->m_vehicle->m_passengers.Next(passenger)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPE_UNIT, __FILE__, __LINE__));
        unit->UpdateSmoothFacing(&facing);
    }
}

// VERIFY: 6d763ece manual
// OFFSET: 0x0071F5C0
bool CGUnit_C::IsDuelingWith(CGUnit_C* unit) {
    if (!unit || !(unit->m_obj->m_type & TYPE_UNIT)) {
        return false;
    }

    bool dueling = false;

    if (!(this->m_unit->m_unitFieldFlags & 0x8) || !(unit->m_unit->m_unitFieldFlags & 0x8)) {
        return dueling;
    }

    CGUnit_C* controller = (this->m_obj->m_type & TYPE_PLAYER) ? this : this->GetControllingPlayer();
    CGUnit_C* otherController = (unit->m_obj->m_type & TYPE_PLAYER) ? unit : unit->GetControllingPlayer();

    if (controller && otherController) {
        CGPlayerLocalData* player = static_cast<CGPlayer_C*>(controller)->m_player;
        uint32_t team = player->m_playerDuelTeam;

        if (team) {
            CGPlayerLocalData* otherPlayer = static_cast<CGPlayer_C*>(otherController)->m_player;
            uint32_t otherTeam = otherPlayer->m_playerDuelTeam;

            if (otherTeam && player->m_playerDuelArbiter == otherPlayer->m_playerDuelArbiter && team != otherTeam) {
                dueling = true;
            }
        }

        return dueling;
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && !otherController) {
        if (unit->GetCharmerOrCreatorGuid()->m_guid && *unit->GetCharmerOrCreatorGuid() == CGDuelInfo::m_dueler) {
            dueling = true;
        }
    } else if (unit->m_obj->m_guid == ClntObjMgrGetActivePlayer() && !controller) {
        if (this->GetCharmerOrCreatorGuid()->m_guid && *this->GetCharmerOrCreatorGuid() == CGDuelInfo::m_dueler) {
            dueling = true;
        }
    }

    return dueling;
}

// VERIFY: 52f39eb2 auto
// OFFSET: 0x007160B0
void CSmoothAngle::Chase(const float* target, float rate, float elapsed) {
    float k = rate * elapsed;
    float damping = 1.0f / (k + (k * k * k * 0.235f + k * k * 0.48f) + 1.0f);
    float offset = this->m_value - *target;
    float change = elapsed * (offset * rate + this->m_velocity);
    this->m_value = (offset + change) * damping + *target;
    this->m_velocity = (this->m_velocity - rate * change) * damping;
}

// VERIFY: c4ca4109 manual
// OFFSET: 0x00735F60
void CGUnit_C::UpdateSmoothFacing(float* facingOffset) {
    float facing = this->m_movementData.m_facing;

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        this->m_smoothTarget.m_value = facing;
        this->m_smoothTarget.m_velocity = 0.0f;

        if ((this->m_movementData.m_flags & 0x30) || CGInputControl::GetActive()->CameraCanTurnPlayer()) {
            this->m_animationState |= 0x1;
        } else {
            this->m_animationState &= ~0x1;
            this->m_lastTurnTimeMs = OsGetAsyncTimeMs();
        }

        if ((this->m_movementData.m_flags & 0x30) || CGInputControl::GetActive()->IsMouseDragMoving()) {
            this->m_animationState |= 0x2;
        } else {
            this->m_animationState &= ~0x2;
        }

        if (this->m_vehicle && this->m_vehicle->m_vehicleRec) {
            if (facingOffset) {
                this->PropagateSmoothFacing(this->m_smoothTarget.m_value + *facingOffset);
            } else {
                this->PropagateSmoothFacing(this->GetSmoothFacing());
            }
        }

        return;
    }

    bool emoteFacing = false;
    uint32_t emoteState = this->m_unit->m_unitNpcEmotestate;

    if (emoteState) {
        EmotesRec* emote = g_emotesDB.GetRecord(emoteState);

        if (emote && !(emote->m_emoteFlags & 0x2000)) {
            emoteFacing = true;
        }
    }

    if (!(this->m_movement->m_flags & 0xF)) {
        CGUnitData* data = this->m_unit;

        if (!this->GetClientStandState() && !emoteFacing && !(data->m_unitFieldFlags & 0x40000) && !(data->m_unitFieldFlags2 & 0x8000)) {
            if (this->m_clientFlags & 0x1) {
                facing = this->m_scriptedFacing;
            } else if (!(data->m_unitFieldFlags & 0x1000000) && !(this->m_obj->m_type & TYPE_PLAYER) && this->CanTurnWhileCasting()) {
                bool seatAllows = true;

                if (this->HasVehicleTransport()) {
                    CGUnit_C* transport = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ this->GetTransportGUID() }, TYPE_UNIT, __FILE__, __LINE__));

                    if (transport) {
                        VehicleSeatRec* seat = transport->GetVehicleSeatRec(this->m_movementData.m_transportSeat);

                        if (seat && !(seat->m_flags & 0x400)) {
                            seatAllows = false;
                        }
                    }
                }

                if (seatAllows) {
                    WOWGUID target;

                    if (this->GetVehicleRec() && this->m_vehicle->m_forceFacingGuid.m_guid) {
                        target = this->m_vehicle->m_forceFacingGuid;
                    } else {
                        if (this->m_unit->m_unitChannelSpell && (this->m_clientFlags & 0x8000) && !this->IsClientControlled()) {
                            target = this->m_unit->m_unitFieldChannelObject;
                        } else {
                            target = this->m_unit->m_unitFieldTarget;
                        }

                        if (!target.m_guid) {
                            target = WOWGUID{ this->GetComboPointTarget() };
                        }
                    }

                    bool haveTarget = true;

                    if (!target.m_guid) {
                        if (CGGameUI::m_interactTarget != this->m_obj->m_guid) {
                            haveTarget = false;
                        } else {
                            target = ClntObjMgrGetActivePlayer();

                            if (!target.m_guid) {
                                haveTarget = false;
                            }
                        }
                    }

                    CGUnit_C* targetUnit = haveTarget ? static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPE_UNIT, __FILE__, __LINE__)) : nullptr;

                    if (targetUnit) {
                        C3Vector targetPosition = targetUnit->GetPosition();
                        C3Vector position = this->GetPosition();
                        float angle = CalculateFacingTo(&position, &targetPosition);

                        if (facingOffset) {
                            facing = CMath::normalizeangle0to2pi_(angle - *facingOffset);
                        } else if (this->HasVehicleTransport()) {
                            CGUnit_C* transport = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ this->GetTransportGUID() }, TYPE_UNIT, __FILE__, __LINE__));

                            if (transport) {
                                facing = CMath::normalizeangle0to2pi_(angle - transport->GetSmoothFacing());
                            } else {
                                facing = CMath::normalizeangle0to2pi_(facing);
                            }
                        } else if (this->GetTransportGUID()) {
                            facing = CMath::normalizeangle0to2pi_(angle - MovementGetTransportFacing(WOWGUID{ this->GetTransportGUID() }));
                        } else {
                            facing = CMath::normalizeangle0to2pi_(angle);
                        }
                    }
                }
            }
        }
    }

    CreatureMovementInfoRec* movementInfo = g_creatureMovementInfoDB.GetRecord(this->m_creatureCacheEntry ? this->m_creatureCacheEntry->m_movementId : 0);

    if (movementInfo && 0.00001f < movementInfo->m_smoothFacingChaseRate) {
        if (facing + 3.1415927f < this->m_smoothTarget.m_value) {
            facing = facing + 6.2831855f;
        } else if (facing - 3.1415927f > this->m_smoothTarget.m_value) {
            facing = facing - 6.2831855f;
        }

        this->m_smoothTarget.Chase(&facing, movementInfo->m_smoothFacingChaseRate, CGWorldFrame::s_currentWorldFrame->m_elapsedSec);
        this->m_smoothTarget.m_value = CMath::normalizeangle0to2pi_(this->m_smoothTarget.m_value);
    } else if ((this->m_unit->m_unitFieldFlags & 0x8) && !(this->m_obj->m_type & TYPE_PLAYER)) {
        if (facing + 3.1415927f < this->m_smoothTarget.m_value) {
            facing = facing + 6.2831855f;
        } else if (facing - 3.1415927f > this->m_smoothTarget.m_value) {
            facing = facing - 6.2831855f;
        }

        this->m_smoothTarget.Chase(&facing, this->IsInCombat() ? 30.0f : 20.0f, CGWorldFrame::s_currentWorldFrame->m_elapsedSec);
        this->m_smoothTarget.m_value = CMath::normalizeangle0to2pi_(this->m_smoothTarget.m_value);
    } else {
        float diff = facing - this->m_smoothTarget.m_value;

        if (3.1415927f < diff) {
            diff = diff - 6.2831855f;
        } else if (diff < -3.1415927f) {
            diff = diff + 6.2831855f;
        }

        if (fabsf(diff) <= 0.01f) {
            this->m_turnDelta[0] = 0.0f;
            this->m_smoothTarget.m_value = facing;

            if (this->m_vehicle && this->m_vehicle->m_vehicleRec) {
                if (facingOffset) {
                    this->PropagateSmoothFacing(facing + *facingOffset);
                } else {
                    this->PropagateSmoothFacing(this->GetSmoothFacing());
                }
            }

            return;
        }

        float step = diff;

        if (diff >= 0.0f) {
            if (this->m_turnDelta[0] < 0.0f) {
                this->m_turnDelta[0] = 0.0f;
            }
        } else if (this->m_turnDelta[0] > 0.0f) {
            this->m_turnDelta[0] = 0.0f;
        }

        if (this->m_turnDelta[0] == 0.0f) {
            this->m_turnDelta[0] = diff;
            this->m_turnDelta[1] = diff;
            this->m_turnDelta[2] = diff;
            this->m_turnDelta[3] = diff;
        } else {
            memmove(&this->m_turnDelta[1], &this->m_turnDelta[0], sizeof(float) * 3);
            this->m_turnDelta[0] = diff;
            float average = (this->m_turnDelta[0] + this->m_turnDelta[1] + this->m_turnDelta[2] + this->m_turnDelta[3]) * 0.25f;

            if (diff <= 0.0f) {
                if (average >= diff) {
                    step = average;
                }
            } else if (average <= diff) {
                step = average;
            }
        }

        this->m_smoothTarget.m_value = CMath::normalizeangle0to2pi_(step * 0.5f + this->m_smoothTarget.m_value);
    }

    if (this->m_vehicle && this->m_vehicle->m_vehicleRec) {
        if (facingOffset) {
            this->PropagateSmoothFacing(this->m_smoothTarget.m_value + *facingOffset);
        } else {
            this->PropagateSmoothFacing(this->GetSmoothFacing());
        }
    }
}

// VERIFY: 08c47d9c auto
// OFFSET: 0x0071D6B0
int32_t IsFullBodyAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (!rec) {
        return 0;
    }

    switch (rec->m_behaviorID) {
        case 0:
        case 1:
        case 3:
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 37:
        case 38:
        case 39:
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 92:
        case 93:
        case 94:
        case 95:
        case 119:
        case 120:
        case 127:
        case 131:
        case 132:
        case 135:
        case 143:
        case 187:
        case 193:
            return 1;
        default:
            return 0;
    }
}

// VERIFY: 3891afce auto
// OFFSET: 0x00714E80
int32_t IsLocomotionAnimId(uint32_t animId) {
    switch (animId) {
        case 4:
        case 5:
        case 11:
        case 12:
        case 13:
        case 37:
        case 38:
        case 39:
        case 42:
        case 43:
        case 44:
        case 45:
        case 119:
        case 135:
        case 143:
        case 187:
        case 223:
            return 1;
        default:
            return 0;
    }
}

// VERIFY: e7f0b392 manual
// OFFSET: 0x0071DD30
int32_t IsMeleeOrReadyAnim(uint32_t animId) {
    if (IsMeleeAttackAnim(animId)) {
        return 1;
    }

    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (!rec) {
        return 0;
    }

    int32_t behavior = rec->m_behaviorID;

    if (behavior < 25 || behavior > 29) {
        return 0;
    }

    return 1;
}

// VERIFY: 22da5b43 manual
// OFFSET: 0x0071DC70
int32_t IsSpellCastOrPrecastAnim(uint32_t animId) {
    if (IsSpellCastAnim(animId)) {
        return 1;
    }

    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (!rec) {
        return 0;
    }

    int32_t behavior = rec->m_behaviorID;

    if (behavior < 51 || behavior > 52) {
        return 0;
    }

    return 1;
}

// VERIFY: 82f0f706 manual
// OFFSET: 0x0071D410
int32_t IsSpellPrecastAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec) {
        int32_t behavior = rec->m_behaviorID;

        if (behavior >= 51 && behavior <= 52) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: f7572fcd manual
// OFFSET: 0x0071D800
int32_t IsUpperBodyAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);
    int32_t behavior = rec ? rec->m_behaviorID : 506;

    switch (behavior) {
        case 2:
        case 8:
        case 9:
        case 10:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 46:
        case 47:
        case 48:
        case 49:
        case 51:
        case 52:
        case 53:
        case 54:
        case 55:
        case 56:
        case 57:
        case 58:
        case 59:
        case 60:
        case 61:
        case 62:
        case 63:
        case 64:
        case 65:
        case 66:
        case 67:
        case 68:
        case 69:
        case 70:
        case 71:
        case 72:
        case 73:
        case 74:
        case 76:
        case 77:
        case 78:
        case 80:
        case 81:
        case 82:
        case 83:
        case 84:
        case 85:
        case 86:
        case 87:
        case 88:
        case 89:
        case 90:
        case 105:
        case 106:
        case 107:
        case 108:
        case 109:
        case 110:
        case 111:
        case 112:
        case 113:
        case 117:
        case 118:
        case 122:
        case 123:
        case 124:
        case 125:
        case 128:
        case 129:
        case 130:
        case 133:
        case 134:
        case 136:
        case 137:
        case 138:
        case 153:
        case 154:
        case 155:
        case 156:
        case 185:
        case 186:
        case 195:
        case 213:
        case 214:
        case 215:
        case 216:
        case 217:
        case 218:
        case 219:
        case 220:
        case 221:
        case 222:
        case 225:
            return 1;
        default:
            return 0;
    }
}

// VERIFY: 1afe3037 manual
// OFFSET: 0x0071DAE0
int32_t IsWeaponReadyAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec) {
        int32_t behavior = rec->m_behaviorID;

        if (behavior >= 25 && behavior <= 29) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: d48a51ea auto
// OFFSET: 0x0071D550
int32_t IsWeaponSpecialAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec) {
        int32_t behavior = rec->m_behaviorID;

        if (behavior >= 57 && (behavior <= 58 || behavior == 118)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 1d404a76 auto
// OFFSET: 0x0071D7C0
int32_t IsJumpAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec) {
        int32_t behavior = rec->m_behaviorID;

        if (behavior >= 37 && (behavior <= 39 || behavior == 187)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 4a7ccec0 auto
// OFFSET: 0x0071D2A0
int32_t IsJumpLandAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec) {
        int32_t behavior = rec->m_behaviorID;

        if (behavior == 39 || behavior == 187) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 0d963261 manual
// OFFSET: 0x0071DE50
int32_t IsFishingAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec) {
        int32_t behavior = rec->m_behaviorID;

        if (behavior >= 133 && behavior <= 134) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 5b0ed408 manual
// OFFSET: 0x0071D2E0
int32_t IsRangedWeaponAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);
    int32_t behavior = rec ? rec->m_behaviorID : 506;

    switch (behavior) {
        case 46:
        case 49:
        case 105:
        case 106:
        case 107:
        case 108:
        case 109:
        case 110:
        case 111:
        case 112:
            return 1;
        default:
            return 0;
    }
}

// VERIFY: d076d5ef manual
// OFFSET: 0x00717540
int32_t AnimationData::FindIdByBehaviorAndTier(uint32_t behavior, int32_t tier, uint32_t* anim) {
    int32_t count = g_animationDataDB.m_numRecords;

    if (static_cast<int32_t>(behavior) >= count) {
        return 0;
    }

    AnimationDataRec* records = g_animationDataDB.m_records;

    if (!tier && static_cast<int32_t>(behavior) >= 0) {
        AnimationDataRec* rec = &records[behavior];

        if (rec && rec->m_behaviorID == static_cast<int32_t>(behavior) && !rec->m_behaviorTier) {
            *anim = rec->m_id;
            return 1;
        }
    }

    AnimationDataRec* rec = g_animationDataDB.GetRecord(behavior);

    if (rec && rec->m_behaviorTier) {
        *anim = behavior;
        return 1;
    }

    for (int32_t i = 0; i < count; i++) {
        AnimationDataRec* candidate = (i < 0 || i >= count) ? nullptr : &records[i];

        if (candidate->m_behaviorID == static_cast<int32_t>(behavior) && candidate->m_behaviorTier == tier) {
            *anim = candidate->m_id;
            return 1;
        }
    }

    return 0;
}

// VERIFY: fea936b3 manual
// OFFSET: 0x007176F0
uint32_t CGUnit_C::ResolveAnimForModel(uint32_t animId, CM2Model* model) {
    int32_t tier = this->m_animTier;

    if (!(this->m_modelFlags & 0x40000) || (this->m_modelFlags & 0x20000) || !this->GetObjectModel() || !this->GetObjectModel()->IsLoaded(0, 0)) {
        return animId;
    }

    if (model) {
        if (!model->IsLoaded(0, 0)) {
            return animId;
        }
    } else {
        model = this->GetObjectModel();
    }

    uint32_t result = static_cast<uint16_t>(animId);
    int32_t visited[ANIM_COUNT];
    memset(visited, 0, sizeof(visited));
    uint32_t current = animId;

    while (!AnimationData::FindIdByBehaviorAndTier(current, tier, &result) || !model->IsSequenceLoaded(result)) {
        AnimationDataRec* rec = g_animationDataDB.GetRecord(current);

        if (current >= ANIM_COUNT || visited[current] || !rec || static_cast<int32_t>(current) == rec->m_fallback) {
            if (this->FindDefaultAnimForTier(tier, &result)) {
                return result;
            }

            tier = s_animTierFallback[tier];
            memset(visited, 0, sizeof(visited));
            current = animId;
            continue;
        }

        visited[current] = 1;
        current = rec->m_fallback;
    }

    return result;
}

// VERIFY: fa7abd5c auto
// OFFSET: 0x004F6250
bool CGUnit_C::IsVehiclePassengerSeated() const {
    return this->m_vehiclePassenger && this->m_vehiclePassenger->m_state == 3;
}

// VERIFY: ce80d12a auto
// OFFSET: 0x005140C0
const VehicleSeatRec* CGUnit_C::GetPassengerSeatRec() const {
    if (this->m_vehiclePassenger) {
        return this->m_vehiclePassenger->m_seatRec;
    }

    return nullptr;
}

// VERIFY: 49948b61 auto
// OFFSET: 0x00715BA0
void CGUnit_C::UpdateWeaponTrails(const uint32_t* a2, uint32_t a3) {
    if (this->m_sheatheState != WEAPONMODE_NORMALMODE) {
        return;
    }

    for (int32_t i = 0; i < 2; i++) {
        if (this->m_weaponTrails[i]) {
            WeaponTrailSetDrawTrail(this->m_weaponTrails[i], a2, a3);
        }
    }
}

// VERIFY: 03d43f0a manual
// OFFSET: 0x0071E400
int32_t CGUnit_C::ApplyAttachmentAnimOverride(const M2BoneSequenceInfo* mount, const M2BoneSequenceInfo* base, const M2BoneSequenceInfo* torso, int32_t hasTorso, M2BoneSequenceInfo* mountOut, M2BoneSequenceInfo* baseOut, M2BoneSequenceInfo* torsoOut, int32_t* keepBase, int32_t* keepTorso) {
    CEffect* effect = this->m_effectList;

    while (true) {
        if (!effect) {
            return 0;
        }

        CEffect* next = effect->m_next;

        if (!(effect->m_flags & 0x800000) && effect->m_kitRec && effect->m_kitRec->m_animID > 0 && (effect->m_flags & 0x1000)) {
            break;
        }

        effect = next;
    }

    if (mount && mountOut) {
        *mountOut = *mount;

        if (mountOut->m_sequenceId != static_cast<uint32_t>(effect->m_kitRec->m_animID)) {
            mountOut->m_time = 0;
        }

        if (effect->m_flags & 0x1000000) {
            mountOut->m_sequenceId = effect->m_kitRec->m_animID;
        } else {
            mountOut->m_sequenceId = this->ResolveAnimForModel(effect->m_kitRec->m_animID, this->m_mountModel);
        }

        mountOut->m_variationIndex = 0xFFFFFFFF;
    }

    if (base && baseOut) {
        *baseOut = *base;

        if (baseOut->m_sequenceId != static_cast<uint32_t>(effect->m_kitRec->m_animID)) {
            baseOut->m_time = 0;
        }

        if (effect->m_flags & 0x1000000) {
            baseOut->m_sequenceId = effect->m_kitRec->m_animID;
        } else {
            baseOut->m_sequenceId = this->ResolveAnimForModel(effect->m_kitRec->m_animID, this->m_worldModel);
        }

        baseOut->m_variationIndex = 0xFFFFFFFF;

        if (keepBase) {
            *keepBase = 0;
        }
    }

    if (hasTorso && torso && torsoOut) {
        *torsoOut = *torso;

        if (torsoOut->m_sequenceId != static_cast<uint32_t>(effect->m_kitRec->m_animID)) {
            torsoOut->m_time = 0;
        }

        if (effect->m_flags & 0x1000000) {
            torsoOut->m_sequenceId = effect->m_kitRec->m_animID;
        } else {
            torsoOut->m_sequenceId = this->ResolveAnimForModel(effect->m_kitRec->m_animID, this->m_worldModel);
        }

        torsoOut->m_variationIndex = 0xFFFFFFFF;

        if (keepTorso) {
            *keepTorso = 0;
        }
    }

    return 1;
}

// VERIFY: 9be483c3 manual
// OFFSET: 0x00737EF0
void CGUnit_C::ProcessAnimationEvent(M2BoneSequenceInfo* slot, uint32_t currentAnim, int32_t torso, uint32_t previousAnim, int32_t blend, int32_t keepVariation) {
    uint32_t anim = slot->m_sequenceId;
    uint32_t playing = anim;
    bool casting = this->m_unit->m_unitChannelSpell || (this->m_animationState & 0x400);

    if (this->m_combat.m_attacking.m_guid || casting || IsSpellCastOrPrecastAnim(previousAnim) || IsMeleeOrReadyAnim(previousAnim)) {
        AnimationDataRec* rec = g_animationDataDB.GetRecord(anim);

        if ((rec && rec->m_behaviorID >= 25 && rec->m_behaviorID <= 29) || !anim) {
            slot->m_variationIndex = 0;
        }
    }

    if (IsDeadOrDyingAnim(anim)) {
        slot->m_speed = 1.0f;
    }

    if (!(this->m_clientFlags & 0x80000)) {
        blend = 0;
    }

    if (!keepVariation && anim != 0xFFFFFFFF && slot->m_variationIndex != 0xFFFFFFFF) {
        M2SequenceInfo info;
        info.Clear();
        this->m_worldModel->GetSequenceInfo(anim, 0, info);
        playing = slot->m_sequenceId;

        if (info.m_animId != slot->m_sequenceId) {
            slot->m_variationIndex = 0;
        }
    }

    uint32_t bone = torso ? this->m_torsoKeyBone : 0xFFFFFFFF;

    if (currentAnim == playing) {
        if (!(fabsf(this->m_worldModel->GetBoneSequenceSpeed(bone) - slot->m_speed) < 2.3841858e-7f)) {
            this->SetKeyBoneSequenceSpeed(this->m_worldModel, bone, slot->m_speed, 0);
        }

        return;
    }

    if (playing == 0xFFFFFFFF || (torso && this->m_torsoKeyBone == -1)) {
        return;
    }

    this->SetKeyBoneSequence(this->m_worldModel, bone, playing, slot->m_variationIndex, slot->m_time, slot->m_speed, blend, 1, 0);

    switch (GetAnimBehaviorId(anim)) {
        case 39:
            this->m_animationState |= 0x4;
            break;
        case 121:
            this->m_animationState |= 0x80000;
            break;
        case 127:
            this->m_animationState |= 0x8;
            break;
        case 192:
        case 200:
            this->m_animationState |= 0x40000;
            break;
        case 201:
            this->m_animationState |= 0x400000;
            break;
        case 458:
        case 459:
        case 460:
            this->m_animationState |= 0x2000000;
            break;
        default:
            break;
    }

    if (IsSubmergedDeathAnim(anim) && this->m_animTier == 3 && (this->m_movementData.m_flags & 0x2201000)) {
        this->m_animationState |= 0x4000000;
    } else {
        this->m_animationState &= ~0x4000000;
    }

    if ((this->m_animationState & 0x20000) && (IsMeleeAttackAnim(anim) || IsSpellCastAnim(anim))) {
        this->m_animationState |= 0x2000;
    }
}

// VERIFY: 745c0966 manual
// OFFSET: 0x00723E30
int32_t CGUnit_C::ShouldSplitTorsoAnimation(uint32_t animId, uint32_t currentAnim) {
    if (currentAnim == 0xFFFFFFFF) {
        return 0;
    }

    if (this->m_vehiclePassenger && this->m_vehiclePassenger->m_state == 3 && this->m_vehiclePassenger->GetLowerAnim(this->m_vehiclePassenger->m_seatRec) != ANIM_COUNT) {
        return 1;
    }

    if (this->m_torsoKeyBone != -1 && this->m_vehicle && this->m_vehicle->m_vehicleRec && this->m_vehicle->IsBoneAnimPlaying(this->m_torsoKeyBone)) {
        return 0;
    }

    if (this->m_vehicle && this->m_vehicle->m_vehicleRec && this->m_vehicle->HasHighPriorityCastingAnim()) {
        return 0;
    }

    if (IsDeadOrDyingAnim(animId)) {
        return 0;
    }

    uint32_t moveFlags = this->m_movementData.m_flags;
    int32_t turning = ((moveFlags & 0x30) || (this->m_animationState & 0x1800)) ? 1 : 0;
    int32_t casting = (!this->m_unit->m_unitChannelSpell && (this->m_animationState & 0x400)) ? 1 : 0;
    int32_t split = 0;

    if (this->m_obj->m_type & TYPE_PLAYER) {
        split = static_cast<CGPlayer_C*>(this)->m_dance != 0;
    }

    if (IsUpperBodyAnim(animId) && ((this->m_animationState & 0x1800) || (moveFlags & 0x2E000FF) || this->m_movementData.IsFalling() || this->GetClientStandState() || (IsWeaponReadyAnim(animId) && static_cast<int32_t>(this->m_unit->m_unitFieldMountdisplayid) > 0) || split)) {
        split = 1;
    } else if ((IsJumpAnim(animId) && IsMeleeOrReadyAnim(currentAnim)) || (IsMeleeAttackAnim(animId) && (this->m_movement->m_flags & 0x1000)) || (casting && (this->m_movementData.m_flags & 0x2E0000F))) {
        split = 1;
    } else {
        split = 0;
    }

    if (IsJumpLandAnim(animId)) {
        split = 0;
    }

    if (IsFishingAnim(animId) && !this->IsStandStateSplitBody()) {
        split = 0;
    }

    if ((casting || this->m_unit->m_unitChannelSpell || this->m_combat.m_attacking.m_guid) && turning && !(this->m_movement->m_flags & 0xC)) {
        split = 0;
    }

    return split;
}

// VERIFY: 99a579ea manual
// OFFSET: 0x00738180
void CGUnit_C::UpdateHandItemAnimEffects(bool force) {
    uint32_t sequence;

    if (this->m_worldModel && this->m_worldModel->IsLoaded(0, 0)) {
        if (this->m_torsoKeyBone == -1 || (sequence = this->m_worldModel->GetBoneSequenceId(this->m_torsoKeyBone)) == 0xFFFFFFFF) {
            sequence = this->m_worldModel->GetBoneSequenceId(0xFFFFFFFF);
        }
    } else {
        sequence = 0xFFFFFFFF;
    }

    AnimationDataRec* rec = g_animationDataDB.GetRecord(sequence);

    if (rec && (rec->m_weaponflags & 0x4) && !force) {
        this->SetSheatheState(WEAPONMODE_SHEATHEDMODE, 1, 0);

        if (this->m_clientFlags & 0x10000) {
            this->m_clientFlags &= ~0x10000;
            this->ShowHandItemSpellEffects(0, 0);
        }

        return;
    }

    if (this->m_sheatheState == WEAPONMODE_RANGEDMODE) {
        if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
            this->SetSheatheState(WEAPONMODE_RANGEDMODE, 1, 0);
        }

        if (sequence != 105 && sequence != 106 && sequence != 112 && !IsRangedWeaponAnim(sequence) && rec) {
            if (rec->m_weaponflags & 0x10) {
                this->SetSheatheState(WEAPONMODE_SHEATHEDMODE, 1, 0);
            } else if (rec->m_weaponflags & 0x20) {
                this->SetSheatheState(WEAPONMODE_NORMALMODE, 1, 0);
            }
        }

        return;
    }

    if (this->m_castSpellId) {
        SpellRec spellRec;

        if (g_spellDB.GetRecord(this->m_castSpellId, &spellRec) && !(spellRec.m_attributes & 0x40000)) {
            this->SetSheatheState(WEAPONMODE_SHEATHEDMODE, 1, 0);
        }

        if (this->m_clientFlags & 0x10000) {
            this->m_clientFlags &= ~0x10000;
            this->ShowHandItemSpellEffects(0, 0);
        }

        return;
    }

    if (this->m_combat.m_attacking.m_guid && IsMeleeAttackAnim(sequence)) {
        this->SetSheatheState(WEAPONMODE_NORMALMODE, 1, 0);

        if (this->m_clientFlags & 0x10000) {
            this->m_clientFlags &= ~0x10000;
            this->ShowHandItemSpellEffects(0, 0);
        }

        return;
    }

    if (rec) {
        int32_t weaponFlags = rec->m_weaponflags;

        if ((weaponFlags & 0x10) && !force) {
            this->SetSheatheState(WEAPONMODE_SHEATHEDMODE, 1, 0);
            return;
        }

        if (weaponFlags & 0x20) {
            this->SetSheatheState(WEAPONMODE_NORMALMODE, 1, 0);
            return;
        }
    }

    if (this->m_combat.m_attacking.m_guid) {
        this->SetSheatheState(WEAPONMODE_NORMALMODE, 1, 0);
        return;
    }

    if (this->IsActivePlayer() || this->m_sheatheState == this->m_unit->m_unitFieldBytes2[0]) {
        this->SetHandItemSpellEffectsVisible(1);
    } else {
        this->SetSheatheState(this->m_unit->m_unitFieldBytes2[0], 1, 0);
    }
}

// VERIFY: 4d39ab41 manual
// OFFSET: 0x007385C0
void CGUnit_C::PlayBaseAnimation(uint32_t animId, int32_t flags) {
    if (!(this->m_modelFlags & 0x40000) || (this->m_modelFlags & 0x20000) || !this->GetObjectModel() || !this->m_worldModel) {
        return;
    }

    if (!this->GetObjectModel()->IsLoaded(0, 0) || !this->m_worldModel->IsLoaded(0, 0)) {
        if (animId != 0xFFFFFFFF) {
            this->m_pendingAnimId = animId;
        }

        return;
    }

    this->m_pendingAnimId = -1;

    if (this->IsDeadOrAttachmentDying() && !IsDeadOrDyingAnim(animId)) {
        return;
    }

    if (this->GetObjectModel()->m_lastAnimTime || (this->GetVehicleRec() && this->m_vehicle->IsSpecialBoneAnimPlaying())) {
        return;
    }

    uint32_t state = this->m_animationState;

    if ((state & 0xC0000) && !IsDeadOrDyingAnim(animId) && !IsBirthAnim(animId)) {
        return;
    }

    if (state & 0x2000000) {
        return;
    }

    if ((state & 0x2000) && !IsDeadOrDyingAnim(animId) && !IsBirthAnim(animId)) {
        return;
    }

    uint32_t anim;

    if (flags & 0x2) {
        anim = animId;
    } else {
        if (!(flags & 0x8) && (animId == ANIM_SPECIAL_1H || animId == ANIM_SPECIAL_2H)) {
            const CGUnitVirtualItem* mainHand = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, false);
            const CGUnitVirtualItem* offHand = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, false);

            if (!mainHand && !offHand) {
                animId = ANIM_SPECIAL_UNARMED;
            } else if (CGUnit_C::IsTwoHanded(mainHand) && !offHand) {
                animId = ANIM_SPECIAL_2H;
            } else {
                animId = ANIM_SPECIAL_1H;
            }
        }

        anim = (flags & 0x4) ? animId : this->ResolveAnimForModel(animId, nullptr);
    }

    M2BoneSequenceInfo mount;
    M2BoneSequenceInfo base;
    M2BoneSequenceInfo torso;
    this->GetCurrentAnimationInfo(&mount, &base, &torso, 0);

    uint32_t previousAnim;
    int32_t hasTorso;

    if (torso.m_sequenceId != 0xFFFFFFFF) {
        previousAnim = torso.m_sequenceId;
        hasTorso = 1;
    } else {
        previousAnim = base.m_sequenceId;
        hasTorso = 0;
    }

    if ((this->m_animationState & 0x20) && IsMeleeAttackAnim(previousAnim) && IsMeleeAttackAnim(anim)) {
        this->SetKeyBoneSequenceSpeed(this->m_worldModel, hasTorso ? this->m_torsoKeyBone : 0xFFFFFFFF, 2.0f, 0);
        this->m_meleeAttackAnimId = anim;
        return;
    }

    uint32_t trail = this->m_pendingWeaponTrail[1];
    this->m_meleeAttackAnimId = -1;

    if (trail) {
        this->UpdateWeaponTrails(&this->m_pendingWeaponTrail[0], trail);
        this->m_pendingWeaponTrail[1] = 0;
    }

    float speed = 1.0f;
    uint32_t time = 0;
    uint32_t variation = 0xFFFFFFFF;

    if (IsCorpseAnim(animId) && IsDeadOrDyingAnim(this->GetCurrentTorsoAnimId())) {
        this->GetObjectModel()->GetBoneSequenceAnimId(0xFFFFFFFF, &variation);
    }

    M2SequenceInfo info;
    info.Clear();
    this->GetObjectModel()->GetSequenceInfo(anim, 0, info);

    if (0.0f != info.m_moveSpeed && IsLocomotionAnimId(anim) && (this->m_movementData.m_flags & 0xC0000F)) {
        speed = this->m_movementData.GetBaseSpeed(0) / fabsf(info.m_moveSpeed);

        if (info.m_duration) {
            M2BoneSequenceInfo current;
            this->GetObjectModel()->GetBoneSequenceInfo(0xFFFFFFFF, current);
            M2SequenceInfo currentInfo;
            currentInfo.Clear();
            this->GetObjectModel()->GetSequenceInfo(current.m_sequenceId, current.m_variationIndex, currentInfo);

            if (0.0f != currentInfo.m_moveSpeed && currentInfo.m_duration) {
                time = current.m_time * info.m_duration / currentInfo.m_duration % info.m_duration;
            }
        }
    }

    int32_t blend = !(flags & 0x1);
    int32_t blendMount = blend;
    int32_t blendTorso = blend;
    int32_t blendBase = blend;
    state = this->m_animationState;
    uint32_t updateMount = state & 0x10;
    M2BoneSequenceInfo mountSlot = mount;
    uint32_t updateBase = state & 0x40;
    M2BoneSequenceInfo baseSlot = base;
    uint32_t updateTorso = state & 0x20;
    M2BoneSequenceInfo torsoSlot = torso;
    M2BoneSequenceInfo newSlot = {};
    newSlot.m_sequenceId = anim;
    newSlot.m_variationIndex = variation;
    newSlot.m_time = time;
    newSlot.m_speed = speed;
    int32_t keepBase = 1;
    int32_t keepTorso = 1;
    int32_t casting = (!this->m_unit->m_unitChannelSpell && (state & 0x400)) ? 1 : 0;
    int32_t split = this->ShouldSplitTorsoAnimation(anim, previousAnim);

    if (split && !updateTorso) {
        split = 0;
    }

    int32_t vehicleBoneAnim = (this->m_torsoKeyBone != -1 && this->GetVehicleRec() && this->m_vehicle->IsBoneAnimPlaying(this->m_torsoKeyBone)) ? 1 : 0;

    if (IsWeaponSpecialAnim(anim) || IsDeadOrDyingAnim(anim) || (this->m_animationState & 0x20000)) {
        split = 0;
        updateTorso = 0;
        updateBase = 1;
    }

    if (this->m_mountModel) {
        if (IsFullBodyAnim(anim)) {
            mountSlot = newSlot;
            mountSlot.m_sequenceId = this->ResolveAnimForModel(anim, this->m_mountModel);
        }

        if (!IsFullBodyAnim(anim) && IsUpperBodyAnim(anim)) {
            torsoSlot = newSlot;
            keepTorso = 0;
        }

        if (this->m_mountAnimBehaviorId != GetAnimBehaviorId(base.m_sequenceId)) {
            baseSlot.m_sequenceId = this->ResolveAnimForModel(this->m_mountAnimBehaviorId, this->m_worldModel);
            baseSlot.m_speed = 1.0f;
            baseSlot.m_variationIndex = 0xFFFFFFFF;
            baseSlot.m_time = 0;
            blendBase = 0;
            keepBase = 0;
        }
    } else if (anim == 61) {
        keepTorso = 0;
        updateTorso = 1;
        torsoSlot = newSlot;
    } else if (anim == 62 && (this->m_movement->m_flags & 0x2200000)) {
        keepTorso = 0;
        updateTorso = 1;
        updateBase = 0;
        torsoSlot = newSlot;
    } else if (split) {
        if ((IsEmoteAnim(anim) && anim != 69) || (!casting && !IsMeleeOrReadyAnim(previousAnim) && !IsSpellCastOrPrecastAnim(previousAnim) && this->m_sheatheState != WEAPONMODE_RANGEDMODE)) {
            keepTorso = 0;
            updateTorso = 1;
            torsoSlot = newSlot;
        } else if (!IsLocomotionAnimId(anim) && !IsWeaponReadyAnim(anim) && anim) {
            keepTorso = 0;
            torsoSlot = newSlot;
            updateTorso = 1;

            if ((this->m_movementData.m_flags & 0x2200000) && !(this->m_movementData.m_flags & 0xF)) {
                baseSlot.m_sequenceId = this->ResolveAnimForModel(41, nullptr);
                baseSlot.m_speed = 1.0f;
                baseSlot.m_variationIndex = 0xFFFFFFFF;
                baseSlot.m_time = 0;
                keepBase = 0;
            }
        } else {
            if (!hasTorso) {
                updateTorso = 1;
                torsoSlot = base;
                keepTorso = 1;
            }

            keepBase = 0;
            baseSlot = newSlot;
        }
    } else if (IsLocomotionAnimId(anim) && (IsMeleeAttackAnim(base.m_sequenceId) || IsSpellCastAnim(base.m_sequenceId))) {
        updateTorso = 1;
        torsoSlot = base;
        keepTorso = 1;
        keepBase = 0;
        baseSlot = newSlot;
    } else if (((IsMeleeOrReadyAnim(previousAnim) && (IsWeaponReadyAnim(anim) || !anim)) || (IsSpellCastOrPrecastAnim(previousAnim) && (IsSpellPrecastAnim(anim) || IsWeaponReadyAnim(anim) || !anim))) && hasTorso && !vehicleBoneAnim) {
        updateTorso = 1;
        keepBase = 1;
        torsoSlot.m_sequenceId = 0xFFFFFFFF;
        keepTorso = 0;
        baseSlot = torso;
    } else {
        keepBase = 0;
        baseSlot = newSlot;
    }

    if (this->m_movementData.IsSplineFlyerOrHovering() && !(this->m_movement->m_flags & 0x2200000)) {
        if (this->OverrideAnimForFlying(&baseSlot.m_sequenceId)) {
            keepBase = 0;
        }

        if (this->OverrideAnimForFlying(&torsoSlot.m_sequenceId)) {
            keepTorso = 0;
        }
    }

    if (IsWeaponSpecialAnim(anim) || IsDeadOrDyingAnim(anim) || (this->m_animationState & 0x20000)) {
        updateTorso = 0;
        updateBase = 1;
        baseSlot = newSlot;
        keepBase = (IsCorpseAnim(anim) && IsDeadOrDyingAnim(this->GetCurrentTorsoAnimId())) ? 1 : 0;
    }

    if (!IsDeadOrDyingAnim(anim) && this->ApplyAttachmentAnimOverride(&mount, &base, &torso, hasTorso, &mountSlot, &baseSlot, &torsoSlot, &keepBase, &keepTorso)) {
        updateBase = 1;

        if (hasTorso) {
            updateTorso = 1;
        }

        if (this->m_mountModel) {
            uint32_t baseAnim = baseSlot.m_sequenceId;

            if (!IsFullBodyAnim(baseAnim) && IsUpperBodyAnim(baseAnim)) {
                torsoSlot = baseSlot;
                baseAnim = baseSlot.m_sequenceId;
                keepTorso = keepBase;
                updateTorso = 1;
            }

            if (GetAnimBehaviorId(baseAnim) != this->m_mountAnimBehaviorId) {
                baseSlot.m_sequenceId = this->ResolveAnimForModel(this->m_mountAnimBehaviorId, this->m_worldModel);
                baseSlot.m_speed = 1.0f;
                baseSlot.m_variationIndex = 0xFFFFFFFF;
                baseSlot.m_time = 0;
                keepBase = 0;
                blendBase = 0;
            }
        }
    }

    if (this->IsVehiclePassengerSeated() && this->m_unit->m_unitFieldHealth > 0) {
        const VehicleSeatRec* seat = this->GetPassengerSeatRec();
        int32_t lowerAnim = this->m_vehiclePassenger->GetLowerAnim(seat);

        if (lowerAnim != ANIM_COUNT) {
            bool moveBaseToTorso = true;

            if (updateTorso) {
                if (GetAnimBehaviorId(torsoSlot.m_sequenceId) || !this->m_vehiclePassenger->GetUpperAnim(seat)) {
                    moveBaseToTorso = false;
                } else {
                    updateTorso = 0;
                }
            }

            if (moveBaseToTorso && GetAnimBehaviorId(baseSlot.m_sequenceId)) {
                torsoSlot = baseSlot;
                keepTorso = keepBase;
                updateTorso = 1;
            }

            baseSlot.m_sequenceId = lowerAnim;
            baseSlot.m_speed = 1.0f;
            baseSlot.m_variationIndex = 0xFFFFFFFF;
            baseSlot.m_time = 0;
            keepBase = 0;
        }
    }

    if (vehicleBoneAnim) {
        updateTorso = 0;
        previousAnim = 0xFFFFFFFF;
    }

    if (this->m_mountModel && torsoSlot.m_sequenceId == static_cast<uint32_t>(this->m_mountAnimBehaviorId) && baseSlot.m_sequenceId == static_cast<uint32_t>(this->m_mountAnimBehaviorId)) {
        updateTorso = 0;
        previousAnim = 0xFFFFFFFF;
        this->UnsetKeyBoneSequence(this->m_worldModel, this->m_torsoKeyBone, 1, 0, 0);
    }

    if (updateMount && this->m_mountModel && mountSlot.m_sequenceId != 0xFFFFFFFF && (mount.m_sequenceId != mountSlot.m_sequenceId || fabsf(mount.m_speed - mountSlot.m_speed) > 0.01f)) {
        this->SetKeyBoneSequence(this->m_mountModel, 0xFFFFFFFF, mountSlot.m_sequenceId, mountSlot.m_variationIndex, mountSlot.m_time, mountSlot.m_speed, blendMount, 1, 0);
    }

    if (IsDeadOrDyingAnim(anim)) {
        updateTorso = 0;

        if (this->m_torsoKeyBone != -1) {
            this->UnsetKeyBoneSequence(this->m_worldModel, this->m_torsoKeyBone, 1, 0, 0);
            this->UnsetKeyBoneSequence(this->m_worldModel, this->m_torsoKeyBone, 1, 1, 0);
        }

        this->UnsetKeyBoneSequence(this->m_worldModel, 0xFFFFFFFF, 1, 0, 0);
    }

    if ((!updateBase || torsoSlot.m_sequenceId != 0xFFFFFFFF || baseSlot.m_sequenceId != torso.m_sequenceId) && updateTorso) {
        int32_t torsoBlend;

        if (torso.m_sequenceId == 0xFFFFFFFF && torsoSlot.m_sequenceId == base.m_sequenceId) {
            torsoBlend = 0;
            blendBase = 1;
        } else {
            torsoBlend = blendTorso;
        }

        this->ProcessAnimationEvent(&torsoSlot, torso.m_sequenceId, 1, previousAnim, torsoBlend, keepTorso);
    }

    if (updateBase) {
        this->ProcessAnimationEvent(&baseSlot, base.m_sequenceId, 0, previousAnim, blendBase, keepBase);
    }

    this->UpdateHandItemAnimEffects((flags >> 5) & 0x1);
    this->UpdateObjectEffectAnimationStates();
}

// VERIFY: 3458f5db manual
// OFFSET: 0x00729C70
int32_t CGUnit_C::ShouldRenderObjectName(uint32_t flags) const {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    const WOWGUID* controllerGuid = this->m_unit->m_unitFieldCharmedby.m_guid ? &this->m_unit->m_unitFieldCharmedby : &this->m_unit->m_unitFieldCreatedby;
    CGUnit_C* controller = nullptr;

    if (controllerGuid->m_guid) {
        controller = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*controllerGuid, TYPE_UNIT, __FILE__, __LINE__));
    }

    if (!this->CanBeTargetted() || (CGGameUI::m_visible && (this->m_namePlateFrame || this->m_chatBubble))) {
        return 0;
    }

    if (this->IsControlledByViewer()) {
        return flags & 0x1;
    }

    if (this->m_obj->m_guid == CGGameUI::m_lockedTarget) {
        return 1;
    }

    if (this->m_obj->m_type & TYPE_PLAYER) {
        if (player->CanCooperate(this)) {
            return (flags >> 7) & 0x1;
        }

        if ((flags & 0x10) && !(this->m_unit->m_unitFieldBytes1[2] & 0x2)) {
            return 1;
        }

        return 0;
    }

    if (controller && (controller->m_obj->m_type & TYPE_PLAYER)) {
        bool friendly = !player->CanAttack(this);

        if (!friendly && (this->m_unit->m_unitFieldBytes1[2] & 0x2)) {
            return 0;
        }

        if ((this->HasCreator() && !this->HasSummoner() && (this->m_unit->m_unitFieldFlags & 0x200)) || this->GetCreatureType() == 8 || this->GetCreatureType() == 12) {
            return (flags >> 10) & 0x1;
        }

        if (this->GetCreatureType() == 11) {
            return friendly ? (flags >> 9) & 0x1 : (flags >> 6) & 0x1;
        }

        if (this->GetCreatorGuid()) {
            return (flags >> 12) & 0x1;
        }

        CGUnit_C* charmer = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_unit->m_unitFieldCharmedby, TYPE_UNIT, __FILE__, __LINE__));

        if (player->CanCooperate(this)) {
            return (flags >> 8) & 0x1;
        }

        if (charmer && charmer->GetTransportGUID() == this->m_obj->m_guid.m_guid && (player == charmer || player->CanCooperate(charmer))) {
            return (flags >> 8) & 0x1;
        }

        return (flags >> 5) & 0x1;
    }

    if ((this->m_unit->m_unitFieldBytes1[2] & 0x2) && player->CanAttack(this)) {
        return 0;
    }

    if (this->GetCreatureType() == 8 || this->GetCreatureType() == 12) {
        return (flags >> 10) & 0x1;
    }

    return (flags >> 1) & 0x1;
}

// VERIFY: 9e068ef2 manual
// OFFSET: 0x0071B090
bool CGUnit_C::CanAutoStand() const {
    return this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && g_autoStandCVar->m_intValue;
}

// VERIFY: dde625d3 auto
// OFFSET: 0x00721F50
bool CGUnit_C::CanAutoInteract() const {
    return this->m_unit->m_unitFieldHealth > 0 && this->m_obj->m_guid == CGUnit_C::s_activeMover && g_cvAutoInteract->m_intValue;
}

// VERIFY: 6c68fd90 manual
// OFFSET: 0x007207E0
int32_t CGUnit_C::IsLowPrioritySelection(uint32_t time) {
    if (this->m_unit->m_unitFieldFlags & 0x2000000) {
        return 1;
    }

    if (this->m_obj->m_type != HEIR_TYPE_UNIT || this->m_unit->m_unitFieldHealth > 0) {
        return 0;
    }

    if (this->CanBeLooted(time) && ClntObjMgrGetActivePlayerObj()->CanLoot(this)) {
        return 0;
    }

    if ((this->m_unit->m_unitFieldFlags & 0x4000000) && CGSpellBook::GetSkinningSpell(this)) {
        return 0;
    }

    return 1;
}

// VERIFY: 39fc65ca auto
// OFFSET: 0x00715EA0
int32_t CGUnit_C::GetCursorOverride(int32_t unable) {
    int32_t mode = this->GetQuestGiverCursor(unable);

    if (mode) {
        return mode;
    }

    CreatureStats_C* stats = this->m_creatureCacheEntry;

    if (!stats || !stats->m_iconName || !*stats->m_iconName) {
        return NO_CURSOR;
    }

    char name[40];

    if (unable) {
        SStrPrintf(name, sizeof(name), "Unable%s", stats->m_iconName);
    } else {
        SStrCopy(name, stats->m_iconName, sizeof(name));
    }

    mode = CursorGetModeFromName(name);
    return mode ? mode : NUM_CURSOR_MODES;
}

// VERIFY: 562a89fa manual
// OFFSET: 0x0071B130
bool CGUnit_C::CanAutoUnshift() const {
    if (this->m_unit->m_unitFieldFlags & 0x100000) {
        return false;
    }

    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer() || !g_autoUnshiftCVar->m_intValue) {
        return false;
    }

    if ((this->m_movement->m_flags & MOVEMENTFLAG_FLYING) && !g_autoDismountFlyingCVar->m_intValue) {
        return false;
    }

    SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(this->GetShapeshiftForm());
    return !form || !(form->m_flags & 0x100);
}

// VERIFY: 530432a2 manual
// OFFSET: 0x0071C570
int32_t CGUnit_C::IsInPartyOrRaid() const {
    if (!(this->m_unit->m_unitNpcFlags & 0x1000000) || (this->m_unit->m_unitFieldFlags2 & 0x2000)) {
        return 0;
    }

    if (!(this->m_unit->m_unitFieldFlags2 & 0x1000)) {
        return 1;
    }

    WOWGUID guid = this->m_obj->m_guid;

    if (CGPartyInfo::IsPlayerOrPetInParty(guid)) {
        return 1;
    }

    WOWGUID raidGuid = this->m_obj->m_guid;
    return CGRaidInfo::IsPlayerOrPetInRaid(raidGuid) ? 1 : 0;
}

// VERIFY: 64b57fd6 manual
// OFFSET: 0x00723050
bool CGUnit_C::CanBeSpellClickedByLocalPlayer(const CGPlayer_C* player) const {
    if (!(this->m_obj->m_type & TYPE_PLAYER)) {
        return false;
    }

    if (!this->m_vehicle || !this->m_vehicle->m_vehicleRec || !this->m_vehicle->m_freeSeatMask) {
        return false;
    }

    if (player->GetTransportGUID() == this->m_obj->m_guid.m_guid) {
        return false;
    }

    WOWGUID guid = this->m_obj->m_guid;

    if (CGPartyInfo::IsPlayerOrPetInParty(guid)) {
        return true;
    }

    WOWGUID raidGuid = this->m_obj->m_guid;
    return CGRaidInfo::IsPlayerOrPetInRaid(raidGuid) != 0;
}

// VERIFY: 5ccd8fa5 manual
// OFFSET: 0x00729620
bool CGUnit_C::CanInteractNow(const CGUnit_C* unit) const {
    CGUnitData* data = this->m_unit;

    if ((data->m_unitFieldFlags & 0x100000) || data->m_unitFieldCharmedby || data->m_unitFieldHealth <= 0) {
        return false;
    }

    if (this->IsShapeShifted()) {
        SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(this->GetShapeshiftForm());

        if (!(form && (form->m_flags & 0x8)) && !this->CanAutoUnshift()) {
            return false;
        }
    }

    CGUnitData* target = unit->m_unit;

    if (target->m_unitFieldHealth <= 0 && !(unit->m_creatureCacheEntry && (unit->m_creatureCacheEntry->m_typeFlags & 0x80))) {
        return false;
    }

    if (target->m_unitFieldCharmedby && !(unit->m_vehicle && unit->m_vehicle->m_vehicleRec)) {
        return false;
    }

    CreatureStats_C* stats = unit->m_creatureCacheEntry;

    if (unit->IsInCombat() && !(stats && (stats->m_typeFlags & 0x40000)) && !(this->m_unit->m_unitFieldFlags2 & 0x4000)) {
        return false;
    }

    if ((target->m_unitNpcFlags & 0x1000000) && !unit->IsInPartyOrRaid()) {
        return false;
    }

    return static_cast<const CGPlayer_C*>(this)->CanInteract(unit);
}

// VERIFY: 2220328b manual
// OFFSET: 0x00731260
void CGUnit_C::OnRightClick() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || (player->m_unit->m_unitFieldHealth <= 0 && !player->GetPossessedUnit())) {
        return;
    }

    if (!(player->m_combatModeFlags & 0x1) || player->GetTransportGUID() == player->GetFarSightGuid()) {
        bool vehicleAllows = player->VehicleAllowsInteraction();

        if (vehicleAllows) {
            CGUnitData* data = this->m_unit;

            if (data->m_unitFieldHealth <= 0 && !(data->m_unitDynamicFlags & 0x20) && player->IsActiveMover()) {
                if (this->CanBeLooted(FrameTime::s_curTimeMs)) {
                    if (player->GetClientStandState()) {
                        if (!player->CanAutoStand()) {
                            CGGameUI::DisplayError(ERR_LOOT_NOTSTANDING);
                            return;
                        }

                        player->ChangeStandState(UNIT_STANDING);
                    }

                    player->SetAutoLoot(CGGameUI::IsAutoLooting());
                    player->LootUnit(this, player->CanAutoInteract());
                    return;
                }

                if (!(data->m_unitFieldFlags & 0x4000000) || !CGSpellBook::GetSkinningSpell(this)) {
                    return;
                }

                if (this->m_obj->m_type & TYPE_PLAYER) {
                    if (!player->CanAssist(this, false)) {
                        player->HandleSkinning(this, player->CanAutoInteract(), true);
                    }

                    return;
                }

                SpellRec spell;
                int32_t minPoints = 0;
                int32_t maxPoints;
                g_spellDB.GetRecord(CGSpellBook::GetSkinningSpell(this), &spell);

                for (uint32_t i = 0; i < 3; i++) {
                    if (spell.m_effect[i] == SPELL_EFFECT_SKINNING) {
                        Spell_C_GetMinMaxPoints(&spell, i, &minPoints, &maxPoints, 0, 0, 0, 0);
                        break;
                    }
                }

                int32_t level = static_cast<int32_t>(this->m_unit->m_unitFieldLevel);
                int32_t required;

                if (level <= 10) {
                    required = 1;
                } else if (level < 20) {
                    required = (level * 5 - 50) * 2;
                } else {
                    required = level * 5;
                }

                if (minPoints < required) {
                    CGGameUI::DisplayError(ERR_USE_LOCKED_WITH_SPELL_KNOWN_SI, spell.m_name, required);
                    return;
                }

                player->HandleSkinning(this, player->CanAutoInteract(), true);
                return;
            }

            if (player->CanInteract(this)) {
                player->HandleOnRightClick(this, player->CanAutoInteract());
                return;
            }
        }

        WOWGUID target = this->m_obj->m_guid;

        if (!player->CanAttackNow(this) || !player->VerifyAttackIconPressed(&target, 1, 0, 0, true)) {
            if (vehicleAllows) {
                this->HandleNPCFeedback();
            }

            return;
        }

        target = this->m_obj->m_guid;
        player->CombatModeEnter(target, 0, player->CanAutoInteract());
        CGPlayer_C* mover = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

        if (mover && mover->m_obj->m_guid == player->m_obj->m_guid && !mover->IsInVehicle()) {
            mover->CTMAttackUnit(this);
        }
    }

    CGUnit_C* possessed = player->GetPossessedUnit();

    if (this->m_unit->m_unitFieldHealth > 0 && possessed && possessed->CanAttackNow(this)) {
        CGPetInfo::PetAttackTarget(this->m_obj->m_guid, 1);
    }
}

// VERIFY: b45861d0 auto
// OFFSET: 0x007165D0
static void CacheEmoteRecords() {
    int32_t index = g_emotesDB.m_numRecords;

    while (index--) {
        EmotesRec* rec = index >= 0 && index < g_emotesDB.m_numRecords ? &g_emotesDB.m_records[index] : nullptr;

        if (rec->m_emoteFlags & 0x8) {
            s_talkEmotes[TALKANIM_TALK] = rec;
        } else if (rec->m_emoteFlags & 0x10) {
            s_talkEmotes[TALKANIM_QUESTION] = rec;
        } else if (rec->m_emoteFlags & 0x20) {
            s_talkEmotes[TALKANIM_EXCLAMATION] = rec;
        } else if (rec->m_emoteFlags & 0x40) {
            s_talkEmotes[TALKANIM_SHOUT] = rec;
        } else if (rec->m_emoteFlags & 0x100) {
            s_talkEmotes[TALKANIM_LAUGH] = rec;
        }
    }
}

// VERIFY: 97b8f4cc auto
// OFFSET: 0x00741D00
void CGUnit_C::RegisterMirrorHandlers() {
    for (uint32_t offset = offsetof(CGUnitData, m_unitVirtualItemSlotId); offset < offsetof(CGUnitData, m_unitVirtualItemSlotId) + sizeof(CGUnitData::m_unitVirtualItemSlotId); offset += sizeof(uint32_t)) {
        ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offset, sizeof(uint32_t), VirtualItemIDMirrorHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    }

    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldLevel), sizeof(uint32_t), UnitLevelUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_OBJECT, offsetof(CGObjectData, m_entryID), sizeof(uint32_t), UnitModeUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldHealth), sizeof(int32_t), UnitHealthUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);

    for (uint32_t offset = offsetof(CGUnitData, m_unitFieldMaxpower); offset < offsetof(CGUnitData, m_unitFieldMaxpower) + sizeof(CGUnitData::m_unitFieldMaxpower); offset += sizeof(uint32_t)) {
        ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offset - (offsetof(CGUnitData, m_unitFieldMaxpower) - offsetof(CGUnitData, m_unitFieldPower)), sizeof(uint32_t), UnitPowerUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
        ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offset, sizeof(uint32_t), UnitMaxPowerUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    }

    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldBytes0) + 3, sizeof(uint8_t), UnitDisplayPowerUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldAurastate), sizeof(uint32_t), AuraStateUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldFlags), sizeof(uint32_t), UnitFlagUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldFlags2), sizeof(uint32_t), UnitFlag2UpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldBytes1) + 2, sizeof(uint8_t), UnitVisFlagUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldBytes2) + 1, sizeof(uint8_t), UnitPvPFlagUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldBytes1) + 3, sizeof(uint8_t), AnimTierChangeHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldMountdisplayid), sizeof(uint32_t), MountDisplayIDUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldFactiontemplate), sizeof(uint32_t), UnitFactionUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldCharmedby), 2 * sizeof(WOWGUID), UnitCharmedUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldDisplayid), sizeof(uint32_t), DisplayIDUpdateHandler, nullptr, HANDLER_PRIORITY_HIGH, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldBytes1), sizeof(uint8_t), StandStateUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitNpcFlags), sizeof(uint32_t), NPCFlagsHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitNpcEmotestate), sizeof(uint32_t), EmoteStateUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldPetNameTimestamp), sizeof(uint32_t), PetNameChangeHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitDynamicFlags), sizeof(uint32_t), DynamicFlagsChangeHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldChannelObject), sizeof(WOWGUID) + sizeof(uint32_t), ChannelSpellOrObjectChangeHandler, nullptr, HANDLER_PRIORITY_NORMAL, 1);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldPetnumber), sizeof(uint32_t), PetNumberChangeHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_OBJECT, offsetof(CGObjectData, m_scale), sizeof(float), UnitScaleUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldBytes2), sizeof(uint8_t), SheatheStateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldTarget), sizeof(WOWGUID), TargetChangeHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldHoverheight), sizeof(float), HoverHeightChangeHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
}

// VERIFY: ba56a0fb auto
// OFFSET: 0x00741FF0
void CGUnit_C::UnregisterMirrorHandlers() {
    for (uint32_t offset = offsetof(CGUnitData, m_unitVirtualItemSlotId); offset < offsetof(CGUnitData, m_unitVirtualItemSlotId) + sizeof(CGUnitData::m_unitVirtualItemSlotId); offset += sizeof(uint32_t)) {
        ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offset, VirtualItemIDMirrorHandler, nullptr);
    }

    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldLevel), UnitLevelUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_OBJECT, offsetof(CGObjectData, m_entryID), UnitModeUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldHealth), UnitHealthUpdateHandler, nullptr);

    for (uint32_t offset = offsetof(CGUnitData, m_unitFieldMaxpower); offset < offsetof(CGUnitData, m_unitFieldMaxpower) + sizeof(CGUnitData::m_unitFieldMaxpower); offset += sizeof(uint32_t)) {
        ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offset - (offsetof(CGUnitData, m_unitFieldMaxpower) - offsetof(CGUnitData, m_unitFieldPower)), UnitPowerUpdateHandler, nullptr);
        ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offset, UnitMaxPowerUpdateHandler, nullptr);
    }

    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldBytes0) + 3, UnitDisplayPowerUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldAurastate), AuraStateUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldFlags), UnitFlagUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldFlags2), UnitFlag2UpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldBytes1) + 2, UnitVisFlagUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldBytes2) + 1, UnitPvPFlagUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldBytes1) + 3, AnimTierChangeHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldMountdisplayid), MountDisplayIDUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldFactiontemplate), UnitFactionUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldCharmedby), UnitCharmedUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldDisplayid), DisplayIDUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldBytes1), StandStateUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitNpcFlags), NPCFlagsHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitNpcEmotestate), EmoteStateUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldPetNameTimestamp), PetNameChangeHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitDynamicFlags), DynamicFlagsChangeHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldChannelObject), ChannelSpellOrObjectChangeHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldPetnumber), PetNumberChangeHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_OBJECT, offsetof(CGObjectData, m_scale), UnitScaleUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_OBJECT, offsetof(CGUnitData, m_unitFieldBytes2), SheatheStateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldHoverheight), HoverHeightChangeHandler, nullptr);
}

// VERIFY: 5d824e82 auto
// OFFSET: 0x00742220
void CGUnit_C::Initialize() {
    ClientServices::SetMessageHandler(MSG_MOVE_START_FORWARD, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_START_BACKWARD, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_STOP, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_START_STRAFE_LEFT, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_START_STRAFE_RIGHT, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_STOP_STRAFE, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_START_ASCEND, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_START_DESCEND, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_STOP_ASCEND, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_JUMP, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_START_TURN_LEFT, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_START_TURN_RIGHT, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_STOP_TURN, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_START_PITCH_UP, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_START_PITCH_DOWN, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_STOP_PITCH, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_RUN_MODE, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_WALK_MODE, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_TELEPORT, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_FACING, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_PITCH, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_TOGGLE_COLLISION_CHEAT, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_GRAVITY_CHNG, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_RUN_SPEED, CGUnit_C::HandleSpeedChangePacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_RUN_BACK_SPEED, CGUnit_C::HandleSpeedChangePacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_WALK_SPEED, CGUnit_C::HandleSpeedChangePacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_SWIM_SPEED, CGUnit_C::HandleSpeedChangePacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_SWIM_BACK_SPEED, CGUnit_C::HandleSpeedChangePacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_FLIGHT_SPEED, CGUnit_C::HandleSpeedChangePacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_FLIGHT_BACK_SPEED, CGUnit_C::HandleSpeedChangePacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_TURN_RATE, CGUnit_C::HandleSpeedChangePacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_PITCH_RATE, CGUnit_C::HandleSpeedChangePacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_SET_COLLISION_HGT, CGUnit_C::HandleSpeedChangePacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_ROOT, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_UNROOT, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_START_SWIM, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_STOP_SWIM, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_START_SWIM_CHEAT, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_STOP_SWIM_CHEAT, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_HEARTBEAT, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_FALL_LAND, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_UPDATE_CAN_FLY, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_UPDATE_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_TELEPORT_ACK, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_TIME_SKIPPED, Packet_MSG_MOVE_TIME_SKIPPED, nullptr);
    ClientServices::SetMessageHandler(SMSG_ON_MONSTER_MOVE, CGUnit_C::HandleMonsterMovePacket, nullptr);
    ClientServices::SetMessageHandler(SMSG_MONSTER_MOVE_TRANSPORT, CGUnit_C::HandleMonsterMovePacket, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCE_RUN_SPEED_CHANGE, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCE_RUN_BACK_SPEED_CHANGE, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCE_SWIM_SPEED_CHANGE, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCE_SWIM_BACK_SPEED_CHANGE, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCE_FLIGHT_SPEED_CHANGE, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCE_WALK_SPEED_CHANGE, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCE_TURN_RATE_CHANGE, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCE_PITCH_RATE_CHANGE, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCE_MOVE_ROOT, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCE_MOVE_UNROOT, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SET_WATER_WALK, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SET_LAND_WALK, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SET_FEATHER_FALL, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SET_NORMAL_FALL, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SET_HOVERING, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_UNSET_HOVERING, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_DISABLE_GRAVITY, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_ENABLE_GRAVITY, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SET_COLLISION_HGT, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SET_CAN_FLY, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_UNSET_CAN_FLY, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_ENABLE_TRANSITION_BETWEEN_SWIM_AND_FLY, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_DISABLE_TRANSITION_BETWEEN_SWIM_AND_FLY, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_KNOCK_BACK, Packet_MoveMode, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOUNT_SPECIAL_ANIM, Packet_SMSG_MOUNTSPECIAL_ANIM, nullptr);
    ClientServices::SetMessageHandler(SMSG_AI_REACTION, Packet_SMSG_AI_REACTION, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_KNOCK_BACK, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_HOVER, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_FEATHER_FALL, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(MSG_MOVE_WATER_WALK, CGUnit_C::HandleMovementPacket, nullptr);
    ClientServices::SetMessageHandler(SMSG_PET_ACTION_SOUND, Packet_SMSG_PET_ACTION_SOUND, nullptr);
    ClientServices::SetMessageHandler(SMSG_PET_DISMISS_SOUND, Packet_SMSG_PET_DISMISS_SOUND, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_ROOT, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_DISABLE_GRAVITY, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_ENABLE_GRAVITY, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_UNROOT, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_FEATHER_FALL, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_NORMAL_FALL, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_HOVER, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_UNSET_HOVER, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_WATER_WALK, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_LAND_WALK, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_START_SWIM, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_STOP_SWIM, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_RUN_MODE, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_WALK_MODE, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_FLYING, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_UNSET_FLYING, Packet_SplineMoveEffect, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_RUN_SPEED, Packet_SplineSpeedChange, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_RUN_BACK_SPEED, Packet_SplineSpeedChange, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_SWIM_SPEED, Packet_SplineSpeedChange, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_SWIM_BACK_SPEED, Packet_SplineSpeedChange, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_FLIGHT_SPEED, Packet_SplineSpeedChange, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_FLIGHT_BACK_SPEED, Packet_SplineSpeedChange, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_WALK_BACK_SPEED, Packet_SplineSpeedChange, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_TURN_RATE, Packet_SplineSpeedChange, nullptr);
    ClientServices::SetMessageHandler(SMSG_MOVE_SPLINE_SET_PITCH_RATE, Packet_SplineSpeedChange, nullptr);
    ClientServices::SetMessageHandler(SMSG_STAND_STATE_UPDATE, Packet_SMSG_STANDSTATE_UPDATE, nullptr);
    ClientServices::SetMessageHandler(SMSG_COMPRESSED_MOVES, Packet_SMSG_COMPRESSED_MOVES, nullptr);
    ClientServices::SetMessageHandler(SMSG_MULTIPLE_MOVES, Packet_SMSG_MULTIPLE_MOVES, nullptr);
    ClientServices::SetMessageHandler(SMSG_CONTROL_UPDATE, Packet_SMSG_CLIENT_CONTROL_UPDATE, nullptr);
    ClientServices::SetMessageHandler(SMSG_FLIGHT_SPLINE_SYNC, Packet_SMSG_FLIGHT_SPLINE_SYNC, nullptr);
    ClientServices::SetMessageHandler(SMSG_AURA_UPDATE_ALL, Packet_AuraUpdate, nullptr);
    ClientServices::SetMessageHandler(SMSG_AURA_UPDATE, Packet_AuraUpdate, nullptr);
    ClientServices::SetMessageHandler(SMSG_DISMOUNT, Packet_SMSG_DISMOUNT, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOOT_LIST, Packet_SMSG_LOOT_LIST, nullptr);
    ClientServices::SetMessageHandler(SMSG_MIRROR_IMAGE_COMPONENTED_DATA, Packet_SMSG_MIRRORIMAGE_DATA, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCE_DISPLAY_UPDATE, Packet_SMSG_FORCE_DISPLAY_UPDATE, nullptr);
    ClientServices::SetMessageHandler(SMSG_CANCEL_AUTO_REPEAT, Packet_SMSG_CANCEL_AUTO_REPEAT, nullptr);
    ClientServices::SetMessageHandler(SMSG_HEALTH_UPDATE, Packet_SMSG_HEALTH_UPDATE, nullptr);
    ClientServices::SetMessageHandler(SMSG_POWER_UPDATE, Packet_SMSG_POWER_UPDATE, nullptr);
    ClientServices::SetMessageHandler(SMSG_HIGHEST_THREAT_UPDATE, Packet_ThreatUpdate, nullptr);
    ClientServices::SetMessageHandler(SMSG_THREAT_UPDATE, Packet_ThreatUpdate, nullptr);
    ClientServices::SetMessageHandler(SMSG_THREAT_REMOVE, Packet_SMSG_THREAT_REMOVE, nullptr);
    ClientServices::SetMessageHandler(SMSG_THREAT_CLEAR, Packet_SMSG_THREAT_CLEAR, nullptr);
    ClientServices::SetMessageHandler(SMSG_PRE_RESSURECT, Packet_SMSG_PRE_RESURRECT, nullptr);
    ClientServices::SetMessageHandler(SMSG_PLAYER_VEHICLE_DATA, Packet_SMSG_PLAYER_VEHICLE_DATA, nullptr);
    ClientServices::SetMessageHandler(SMSG_MULTIPLE_PACKETS_2, Packet_SMSG_MULTIPLE_PACKETS, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCE_ANIM, Packet_SMSG_UNKNOWN_1240, nullptr);
    UnitSoundInitialize();
    Spell_C_Initialize();
    UnitCombatClientInitialize();
    CGUnit_C::RegisterMirrorHandlers();
    CacheEmoteRecords();

    memset(g_environmentalDamageVisualKits, 0, sizeof(g_environmentalDamageVisualKits));

    int32_t index = g_environmentalDamageDB.m_numRecords;

    while (index--) {
        EnvironmentalDamageRec* rec = index >= 0 && index < g_environmentalDamageDB.m_numRecords ? &g_environmentalDamageDB.m_records[index] : nullptr;

        if (static_cast<uint32_t>(rec->m_enumID) < sizeof(g_environmentalDamageVisualKits) / sizeof(g_environmentalDamageVisualKits[0])) {
            g_environmentalDamageVisualKits[rec->m_enumID] = rec->m_visualkitID;
        }
    }

    CGUnit_C::s_trackingType = TRACKTYPE_NONE;
    CGUnit_C::s_trackingStuckCount = 0;
    s_pendingAttackTarget = WOWGUID();
    s_pendingLootTarget = WOWGUID();
    s_pendingUseObject = WOWGUID();
    s_pendingSkinTarget = WOWGUID();
    s_pendingInteractTarget = WOWGUID();
    CGUnit_C::s_activeMover = WOWGUID();
    CMissile::Initialize();
    CVehicle_C::InitSystem();
    CVehiclePassenger_C::InitSystem();
    CVehicleCamera_C::InitSystem();
    CGUnit_C::InitMissileTrajectorySystem();
    CGUnit_C::VehiclePassengerInitWorldCameraState();
    s_unitThreatPool = new (__FILE__, __LINE__) UnitThreatHeap{ ObjectAllocAddHeap(sizeof(UnitThreat), 16, "Unit Threat", true) };
    CGUnit_C::s_deferredClientControlUpdateGUID = WOWGUID();
    CGUnit_C::s_deferredClientControlUpdateState = 0;
    CGUnit_C::s_initialized = 1;
}

// VERIFY: 1c54f4da manual
// OFFSET: 0x00742BB0
void CGUnit_C::Shutdown() {
    CGUnit_C::s_initialized = 0;
    CMissile::RemoveMissiles();
    ClientServices::ClearMessageHandler(MSG_MOVE_START_FORWARD);
    ClientServices::ClearMessageHandler(MSG_MOVE_START_BACKWARD);
    ClientServices::ClearMessageHandler(MSG_MOVE_STOP);
    ClientServices::ClearMessageHandler(MSG_MOVE_START_STRAFE_LEFT);
    ClientServices::ClearMessageHandler(MSG_MOVE_START_STRAFE_RIGHT);
    ClientServices::ClearMessageHandler(MSG_MOVE_STOP_STRAFE);
    ClientServices::ClearMessageHandler(MSG_MOVE_START_ASCEND);
    ClientServices::ClearMessageHandler(MSG_MOVE_START_DESCEND);
    ClientServices::ClearMessageHandler(MSG_MOVE_STOP_ASCEND);
    ClientServices::ClearMessageHandler(MSG_MOVE_JUMP);
    ClientServices::ClearMessageHandler(MSG_MOVE_START_TURN_LEFT);
    ClientServices::ClearMessageHandler(MSG_MOVE_START_TURN_RIGHT);
    ClientServices::ClearMessageHandler(MSG_MOVE_STOP_TURN);
    ClientServices::ClearMessageHandler(MSG_MOVE_START_PITCH_UP);
    ClientServices::ClearMessageHandler(MSG_MOVE_START_PITCH_DOWN);
    ClientServices::ClearMessageHandler(MSG_MOVE_STOP_PITCH);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_RUN_MODE);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_WALK_MODE);
    ClientServices::ClearMessageHandler(MSG_MOVE_TELEPORT);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_FACING);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_PITCH);
    ClientServices::ClearMessageHandler(MSG_MOVE_TOGGLE_COLLISION_CHEAT);
    ClientServices::ClearMessageHandler(MSG_MOVE_GRAVITY_CHNG);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_RUN_SPEED);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_RUN_BACK_SPEED);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_WALK_SPEED);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_SWIM_SPEED);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_SWIM_BACK_SPEED);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_FLIGHT_SPEED);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_FLIGHT_BACK_SPEED);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_TURN_RATE);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_PITCH_RATE);
    ClientServices::ClearMessageHandler(MSG_MOVE_SET_COLLISION_HGT);
    ClientServices::ClearMessageHandler(MSG_MOVE_ROOT);
    ClientServices::ClearMessageHandler(MSG_MOVE_UNROOT);
    ClientServices::ClearMessageHandler(MSG_MOVE_START_SWIM);
    ClientServices::ClearMessageHandler(MSG_MOVE_STOP_SWIM);
    ClientServices::ClearMessageHandler(MSG_MOVE_START_SWIM_CHEAT);
    ClientServices::ClearMessageHandler(MSG_MOVE_STOP_SWIM_CHEAT);
    ClientServices::ClearMessageHandler(MSG_MOVE_HEARTBEAT);
    ClientServices::ClearMessageHandler(MSG_MOVE_FALL_LAND);
    ClientServices::ClearMessageHandler(MSG_MOVE_UPDATE_CAN_FLY);
    ClientServices::ClearMessageHandler(MSG_MOVE_UPDATE_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY);
    ClientServices::ClearMessageHandler(MSG_MOVE_TELEPORT_ACK);
    ClientServices::ClearMessageHandler(MSG_MOVE_TIME_SKIPPED);
    ClientServices::ClearMessageHandler(SMSG_ON_MONSTER_MOVE);
    ClientServices::ClearMessageHandler(SMSG_MONSTER_MOVE_TRANSPORT);
    ClientServices::ClearMessageHandler(SMSG_FORCE_RUN_SPEED_CHANGE);
    ClientServices::ClearMessageHandler(SMSG_FORCE_RUN_BACK_SPEED_CHANGE);
    ClientServices::ClearMessageHandler(SMSG_FORCE_SWIM_SPEED_CHANGE);
    ClientServices::ClearMessageHandler(SMSG_FORCE_SWIM_BACK_SPEED_CHANGE);
    ClientServices::ClearMessageHandler(SMSG_FORCE_FLIGHT_SPEED_CHANGE);
    ClientServices::ClearMessageHandler(SMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE);
    ClientServices::ClearMessageHandler(SMSG_FORCE_WALK_SPEED_CHANGE);
    ClientServices::ClearMessageHandler(SMSG_FORCE_TURN_RATE_CHANGE);
    ClientServices::ClearMessageHandler(SMSG_FORCE_PITCH_RATE_CHANGE);
    ClientServices::ClearMessageHandler(SMSG_FORCE_MOVE_ROOT);
    ClientServices::ClearMessageHandler(SMSG_FORCE_MOVE_UNROOT);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SET_WATER_WALK);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SET_LAND_WALK);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SET_FEATHER_FALL);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SET_NORMAL_FALL);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SET_HOVERING);
    ClientServices::ClearMessageHandler(SMSG_MOVE_UNSET_HOVERING);
    ClientServices::ClearMessageHandler(SMSG_MOVE_DISABLE_GRAVITY);
    ClientServices::ClearMessageHandler(SMSG_MOVE_ENABLE_GRAVITY);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SET_COLLISION_HGT);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SET_CAN_FLY);
    ClientServices::ClearMessageHandler(SMSG_MOVE_UNSET_CAN_FLY);
    ClientServices::ClearMessageHandler(SMSG_MOVE_ENABLE_TRANSITION_BETWEEN_SWIM_AND_FLY);
    ClientServices::ClearMessageHandler(SMSG_MOVE_DISABLE_TRANSITION_BETWEEN_SWIM_AND_FLY);
    ClientServices::ClearMessageHandler(SMSG_MOVE_KNOCK_BACK);
    ClientServices::ClearMessageHandler(SMSG_MOUNT_SPECIAL_ANIM);
    ClientServices::ClearMessageHandler(SMSG_AI_REACTION);
    ClientServices::ClearMessageHandler(MSG_MOVE_KNOCK_BACK);
    ClientServices::ClearMessageHandler(MSG_MOVE_HOVER);
    ClientServices::ClearMessageHandler(MSG_MOVE_FEATHER_FALL);
    ClientServices::ClearMessageHandler(MSG_MOVE_WATER_WALK);
    ClientServices::ClearMessageHandler(SMSG_PET_ACTION_SOUND);
    ClientServices::ClearMessageHandler(SMSG_PET_DISMISS_SOUND);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_ROOT);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_DISABLE_GRAVITY);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_ENABLE_GRAVITY);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_UNROOT);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_FEATHER_FALL);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_NORMAL_FALL);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_HOVER);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_UNSET_HOVER);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_WATER_WALK);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_LAND_WALK);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_START_SWIM);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_STOP_SWIM);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_RUN_MODE);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_WALK_MODE);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_FLYING);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_UNSET_FLYING);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_RUN_SPEED);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_RUN_BACK_SPEED);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_SWIM_SPEED);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_SWIM_BACK_SPEED);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_FLIGHT_SPEED);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_FLIGHT_BACK_SPEED);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_WALK_BACK_SPEED);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_TURN_RATE);
    ClientServices::ClearMessageHandler(SMSG_MOVE_SPLINE_SET_PITCH_RATE);
    ClientServices::ClearMessageHandler(SMSG_STAND_STATE_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_COMPRESSED_MOVES);
    ClientServices::ClearMessageHandler(SMSG_MULTIPLE_MOVES);
    ClientServices::ClearMessageHandler(SMSG_CONTROL_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_FLIGHT_SPLINE_SYNC);
    ClientServices::ClearMessageHandler(SMSG_AURA_UPDATE_ALL);
    ClientServices::ClearMessageHandler(SMSG_AURA_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_DISMOUNT);
    ClientServices::ClearMessageHandler(SMSG_LOOT_LIST);
    ClientServices::ClearMessageHandler(SMSG_MIRROR_IMAGE_COMPONENTED_DATA);
    ClientServices::ClearMessageHandler(SMSG_FORCE_DISPLAY_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_CANCEL_AUTO_REPEAT);
    ClientServices::ClearMessageHandler(SMSG_HEALTH_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_POWER_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_HIGHEST_THREAT_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_THREAT_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_THREAT_REMOVE);
    ClientServices::ClearMessageHandler(SMSG_THREAT_CLEAR);
    ClientServices::ClearMessageHandler(SMSG_PRE_RESSURECT);
    ClientServices::ClearMessageHandler(SMSG_PLAYER_VEHICLE_DATA);
    ClientServices::ClearMessageHandler(SMSG_MULTIPLE_PACKETS_2);
    ClientServices::ClearMessageHandler(SMSG_FORCE_ANIM);
    UnitSoundShutdown();
    Spell_C_Destroy();
    UnitCombatClientShutdown();
    CGUnit_C::UnregisterMirrorHandlers();

    if (s_unitThreatPool) {
        delete s_unitThreatPool;
        s_unitThreatPool = nullptr;
    }
}

// VERIFY: fa026898 auto
// OFFSET: 0x00715970
const char* CGUnit_C::GetDisplayRaceNameFromRecord(const ChrRacesRec* rec, int32_t sex, int32_t* displaySex) {
    if (displaySex) {
        *displaySex = sex;
    }

    if (!rec) {
        return nullptr;
    }

    if (sex == 0) {
        if (*rec->m_nameMale) {
            return rec->m_nameMale;
        }

        if (*rec->m_nameFemale) {
            if (displaySex) {
                *displaySex = 1;
            }

            return rec->m_nameFemale;
        }
    } else if (sex == 1) {
        if (*rec->m_nameFemale) {
            return rec->m_nameFemale;
        }

        if (*rec->m_nameMale) {
            if (displaySex) {
                *displaySex = 0;
            }

            return rec->m_nameMale;
        }
    }

    return rec->m_name;
}

// VERIFY: a01b044c auto
// OFFSET: 0x007159E0
const char* CGUnit_C::GetDisplayClassNameFromRecord(const ChrClassesRec* rec, int32_t sex, int32_t* displaySex) {
    if (displaySex) {
        *displaySex = sex;
    }

    if (!rec) {
        return nullptr;
    }

    if (sex == 0) {
        if (*rec->m_nameMale) {
            return rec->m_nameMale;
        }

        if (*rec->m_nameFemale) {
            if (displaySex) {
                *displaySex = 1;
            }

            return rec->m_nameFemale;
        }
    } else if (sex == 1) {
        if (*rec->m_nameFemale) {
            return rec->m_nameFemale;
        }

        if (*rec->m_nameMale) {
            if (displaySex) {
                *displaySex = 0;
            }

            return rec->m_nameMale;
        }
    }

    return rec->m_name;
}

// VERIFY: 39368271 auto
// OFFSET: 0x004D43C0
int32_t CGUnit_C::IsActiveMover() const {
    return this->m_obj->m_guid == s_activeMover;
}

// VERIFY: f85d0af8 auto
// OFFSET: 0x004F61D0
float CGUnit_C::GetDistanceToPos(const C3Vector& pos) const {
    C3Vector position = this->GetPosition();
    float dy = position.y - pos.y;
    float dz = position.z - pos.z;
    float dx = position.x - pos.x;
    return dx * dx + dz * dz + dy * dy;
}

// VERIFY: 21b1c818 auto
// OFFSET: 0x004F6210
VehicleRec* CGUnit_C::GetVehicleRec() const {
    if (this->m_vehicle) {
        return this->m_vehicle->m_vehicleRec;
    }

    return nullptr;
}

// VERIFY: c3019c7e auto
// OFFSET: 0x004F8850
uint32_t CGUnit_C::GetAuraCount() const {
    if (this->m_auras.m_count == 0xFFFFFFFF) {
        return this->m_auras.m_dynamic.m_count;
    }

    return this->m_auras.m_count;
}

// VERIFY: 540bfe58 manual
// OFFSET: 0x004F8870
int32_t CGUnit_C::GetAuraSpellId(uint32_t index) {
    return this->m_auras[index].m_spellId;
}

// VERIFY: 43e20507 auto
// OFFSET: 0x0051A230
int32_t CGUnit_C::IsMounted() const {
    return this->m_mountDisplayId > 0 && !(this->m_clientFlags & 0x10000000);
}

// VERIFY: cb916759 manual
// OFFSET: 0x00556E10
AuraData* CGUnit_C::GetAura(uint32_t index) {
    if (index >= this->GetAuraCount()) {
        return nullptr;
    }

    return &this->m_auras[index];
}

// VERIFY: ee76539f manual
// OFFSET: 0x00565510
uint8_t CGUnit_C::GetAuraFlags(uint32_t index) {
    return this->m_auras[index].m_flags;
}

// VERIFY: 649f7610 manual
// OFFSET: 0x005D3310
int32_t CGUnit_C::IsControllable() const {
    uint32_t flags = this->m_unit->m_unitFieldFlags;
    return !(flags & 0x40000) && !(flags & 0x800000) && !(flags & 0x400000);
}

// VERIFY: 0d046ee1 auto
// OFFSET: 0x005D3340
VehicleSeatRec* CGUnit_C::GetVehicleSeatRec(uint8_t seat) const {
    if (this->m_vehicle) {
        return this->m_vehicle->GetVehicleSeatRec(seat);
    }

    return nullptr;
}

// VERIFY: af7909ae manual
// OFFSET: 0x006D1DE0
int32_t __stdcall CGUnit_C::UnitCanGossip(CGUnit_C* unit) {
    return unit->IsQuestGiver() && unit->m_questStatus >= 2;
}

// VERIFY: c178f90d manual
// OFFSET: 0x006E6EC0
int32_t CGUnit_C::CanBeTargetted() const {
    return this->CanHighlight();
}

// VERIFY: ebdc15d6 auto
// OFFSET: 0x006E6ED0
int32_t CGUnit_C::IsHighlightSuppressed() const {
    return (this->m_clientFlags >> 25) & 1;
}

// VERIFY: 48845495 auto
// OFFSET: 0x006E6F10
C3Vector CGUnit_C::GetRawPosition() const {
    return this->m_movement->m_position;
}

// VERIFY: bfc83e4b auto
// OFFSET: 0x006E6F60
float CGUnit_C::GetRawFacing() const {
    return this->m_movement->m_facing;
}

// VERIFY: 4aabb319 auto
// OFFSET: 0x006E6F70
uint64_t CGUnit_C::GetTransportGUID() const {
    return this->m_movementData.m_transportGuid.m_guid;
}

// VERIFY: 38a0654c auto
// OFFSET: 0x006E6F80
CM2Model* CGUnit_C::GetObjectModel() const {
    if (this->m_mountModel) {
        return this->m_mountModel;
    }

    return this->m_worldModel;
}

// VERIFY: 6cda1b12 auto
// OFFSET: 0x006E6FC0
float CGUnit_C::GetPitch() const {
    return this->m_movementData.m_pitch;
}

// VERIFY: c74ca830 auto
// OFFSET: 0x006FBE30
void CGUnit_C::PreStartUpdatingMissileTrajectory(int32_t spellId, uint32_t castId) {
    this->m_missileTrajectorySpell = spellId;
    this->m_missileTrajectoryCastId = castId;
}

// VERIFY: cae7a4bc auto
// OFFSET: 0x006FBE50
void CGUnit_C::StartUpdatingMissileTrajectory(int32_t spellId, uint32_t start, uint32_t end) {
    if (spellId == this->m_missileTrajectorySpell) {
        this->m_missileTrajectoryStart = start;
        this->m_missileTrajectoryEnd = end;
    }
}

// VERIFY: e1295041 auto
// OFFSET: 0x006FBE80
void CGUnit_C::StopUpdatingMissileTrajectory(int32_t spellId) {
    if (spellId == this->m_missileTrajectorySpell) {
        this->m_missileTrajectorySpell = 0;
        this->m_missileTrajectoryCastId = 0;
        this->m_missileTrajectoryStart = 0;
        this->m_missileTrajectoryReleased = 0;
    }
}

// VERIFY: 9b961405 auto
// OFFSET: 0x006FBEB0
int32_t CGUnit_C::IsUpdatingMissileTrajectory() const {
    return this->m_missileTrajectoryCastId && this->m_missileTrajectoryStart;
}

// VERIFY: fb60f5d5 auto
// OFFSET: 0x006FBED0
int32_t CGUnit_C::IsMissileTrajectoryUpdateWaitingForMissileRelease() const {
    return this->m_missileTrajectoryCastId && this->m_missileTrajectoryStart && this->m_missileTrajectoryReleased;
}

// VERIFY: f0ab1eb4 auto
// OFFSET: 0x00714AC0
bool CGUnit_C::IsLocalClientControlled() const {
    return (this->m_clientFlags >> 10) & 1;
}

// VERIFY: ca03fd67 auto
// OFFSET: 0x00714BD0
void CGUnit_C::FreeComponent() {
    if (this->m_characterComponent) {
        CCharacterComponent::FreeComponent(this->m_characterComponent);
        this->m_characterComponent = nullptr;
        this->m_clientFlags = (this->m_clientFlags & 0xFFBDFFFF) | 0x400000;
    }
}

// VERIFY: 0e7d75c5 auto
// OFFSET: 0x00714C40
int32_t CGUnit_C::ShouldFadeout() {
    int32_t render = 0;
    int32_t fade = 0;
    this->ShouldRender(1, render, fade);
    return fade == 0;
}

// VERIFY: 8b66139f auto
// OFFSET: 0x00714C80
int32_t __stdcall CGUnit_C::IsTwoHanded(const CGUnitVirtualItem* item) {
    if (!item || item->m_classID != 2) {
        return 0;
    }

    switch (item->m_subclassID) {
        case 1:
        case 5:
        case 6:
        case 8:
        case 10:
        case 12:
        case 17:
        case 20:
            return 1;

        default:
            return 0;
    }
}

// VERIFY: 46787e5a auto
// OFFSET: 0x00714DD0
uint32_t CGUnit_C::GetCombatReadyAnim() const {
    const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, false);

    if (!item || item->m_classID != 2) {
        item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, false);
    }

    if (!item || item->m_classID != 2) {
        return ANIM_READY_UNARMED;
    }

    uint32_t anim = ANIM_READY_2H - (this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, false) != nullptr);

    switch (item->m_subclassID) {
        case 0:
        case 4:
        case 7:
        case 11:
        case 13:
        case 14:
        case 15:
        case 20:
            return ANIM_READY_1H;

        case 1:
        case 5:
        case 8:
        case 12:
            return anim;

        case 6:
        case 10:
        case 17:
            return ANIM_READY_2HL;

        default:
            return ANIM_READY_UNARMED;
    }
}

// VERIFY: cfd62b02 auto
// OFFSET: 0x00714F90
int32_t CGUnit_C::ChooseRangedLoadAnim(uint32_t flags, uint32_t* anim) const {
    if (this->m_sheatheState != WEAPONMODE_RANGEDMODE || !(this->m_animationState & 0x200)) {
        return (flags & 0xFFFFFF00) == 0;
    }

    if (!(flags & 0x80) || (this->m_animationState & 0x4000)) {
        return 1;
    }

    *anim = ANIM_READY_UNARMED;
    const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false);

    if (!item || item->m_classID != 2) {
        return 1;
    }

    switch (item->m_subclassID) {
        case 2: {
            *anim = 105;
            return 1;
        }

        case 3:
        case 18: {
            *anim = 106;
            return 1;
        }

        case 16: {
            *anim = 112;
            return 1;
        }

        case 19: {
            *anim = 111;
            return 1;
        }

        default: {
            return 1;
        }
    }
}

// VERIFY: 293f222c auto
// OFFSET: 0x007151A0
void CGUnit_C::RemoveMountModel() {
    if (this->m_mountModel) {
        this->m_mountModel->SetLoadedCallback(nullptr, nullptr);
        this->m_mountModel->SetSequenceCallback(nullptr, 0);
        this->m_mountModel->SetEventCallback(nullptr, 0);
        this->m_mountModel->Release();
        this->m_mountModel = nullptr;
    }
}

// VERIFY: 472acfa3 auto
// OFFSET: 0x00717910
void CGUnit_C::SetMountModel(CM2Model* model) {
    if (model == this->m_mountModel) {
        return;
    }

    this->CleanUpVehicleBoneAnimsBeforeObjectModelChange();

    if (model) {
        model->m_refCount++;
    }

    if (this->m_mountModel) {
        if (this->m_mountModel->m_attachParent) {
            this->m_mountModel->DetachFromParent();
        }

        this->m_mountModel->SetLoadedCallback(nullptr, nullptr);
        this->m_mountModel->Release();
    }

    this->m_mountModel = model;

    if (model) {
        model->SetLoadedCallback(CGObject_C::ModelLoadedCallback, this);
    } else if (this->GetObjectModel()->IsLoaded(0, 0)) {
        this->ModelChanged();
    } else {
        this->SetBoundsDirty();
    }

    if (this->m_worldObject) {
        World::ObjectSetModel(this->m_worldObject, this->GetObjectModel());
    }
}

// VERIFY: 615d1384 auto
// OFFSET: 0x00715270
void CGUnit_C::RotateFacing(float delta) {
    this->m_smoothTarget.m_value = CMath::normalizeangle0to2pi_(delta + this->m_smoothTarget.m_value);
    this->m_renderFacing = CMath::normalizeangle0to2pi_(delta + this->m_renderFacing);
}

// VERIFY: 9077e5e5 auto
// OFFSET: 0x00715380
void CGUnit_C::NamePlateSetResortAll(bool enable) {
    if (enable) {
        s_namePlateFlags |= 0x1;
    } else {
        s_namePlateFlags &= ~0x1;
    }
}

// VERIFY: 384acd97 auto
// OFFSET: 0x007153A0
void CGUnit_C::NamePlateUpdateLevels(bool enable) {
    if (enable) {
        s_namePlateFlags |= 0x2;
    } else {
        s_namePlateFlags &= ~0x2;
    }
}

// VERIFY: fb6389d8 auto
// OFFSET: 0x007153C0
void CGUnit_C::NamePlateUpdateColor(bool enable) {
    if (enable) {
        s_namePlateFlags |= 0x4;
    } else {
        s_namePlateFlags &= ~0x4;
    }
}

// VERIFY: 0cc4b6e6 auto
// OFFSET: 0x007153E0
uint32_t CGUnit_C::GetCreatureFamily() const {
    if (this->m_creatureCacheEntry) {
        return this->m_creatureCacheEntry->m_family;
    }

    return 0;
}

// VERIFY: 8fb86460 manual
// OFFSET: 0x00715400
void CGUnit_C::SaveChainSpellLocation(const C3Vector* location) {
    if (location) {
        this->m_chainSpellLocation = *location;
        this->m_clientFlags |= 0x800000;
    } else {
        this->m_clientFlags &= ~0x800000;
    }
}

// VERIFY: a3c8b445 manual
// OFFSET: 0x00715440
UNIT_REACTION FactionTemplateReaction(const FactionTemplateRec* faction, const FactionTemplateRec* other) {
    int32_t otherGroup = other->m_factionGroup;

    if (otherGroup & faction->m_enemyGroup) {
        return UNIT_REACTION_HOSTILE;
    }

    for (uint32_t i = 0; i < 4 && faction->m_enemies[i]; i++) {
        if (faction->m_enemies[i] == other->m_faction) {
            return UNIT_REACTION_HOSTILE;
        }
    }

    if (otherGroup & faction->m_friendGroup) {
        return UNIT_REACTION_FRIENDLY;
    }

    for (uint32_t i = 0; i < 4 && faction->m_friend[i]; i++) {
        if (faction->m_friend[i] == other->m_faction) {
            return UNIT_REACTION_FRIENDLY;
        }
    }

    if (faction->m_factionGroup & other->m_friendGroup) {
        return UNIT_REACTION_FRIENDLY;
    }

    for (uint32_t i = 0; i < 4 && other->m_friend[i]; i++) {
        if (other->m_friend[i] == faction->m_faction) {
            return UNIT_REACTION_FRIENDLY;
        }
    }

    return static_cast<UNIT_REACTION>((~(static_cast<uint32_t>(faction->m_flags) >> 12) & 2) | 1);
}

// VERIFY: 1d445e4d auto
// OFFSET: 0x00715670
void CGUnit_C::StartMountTransition(const MountTransitionObject* transition, CEffect* effect) {
    this->m_mountTransition = transition;
    this->m_mountTransitionEffect = effect;
}

// VERIFY: 3a7849d2 auto
// OFFSET: 0x00715690
void CGUnit_C::StopMountTransition() {
    this->m_mountTransition = nullptr;
    this->m_mountTransitionEffect = nullptr;
}

// VERIFY: d86295f0 auto
// OFFSET: 0x00715880
bool CGUnit_C::IsStandStateSplitBody() const {
    bool result = false;

    switch (this->GetClientStandState()) {
        case UNIT_STANDING:
        case UNIT_SITTING:
        case UNIT_SITTINGCHAIRLOW:
        case UNIT_SITTINGCHAIRMEDIUM:
        case UNIT_SITTINGCHAIRHIGH:
            result = true;
            break;
    }

    return result;
}

// VERIFY: 8bfb7b9e auto
// OFFSET: 0x00715930
const char* CGUnit_C::GetDisplayBakeName() const {
    if (this->m_displayInfoExtra) {
        return this->m_displayInfoExtra->m_bakeName;
    }

    return nullptr;
}

// VERIFY: 613e51c7 auto
// OFFSET: 0x00715950
const char* CGUnit_C::GetPortraitTextureName() const {
    if (this->m_displayInfo) {
        return this->m_displayInfo->m_portraitTextureName;
    }

    return nullptr;
}

// VERIFY: 24d7a4c0 auto
// OFFSET: 0x00715A50
void CGUnit_C::SetAnimEventPosition(const C3Vector* position) {
    this->m_clientFlags |= 0x80000000;
    this->m_animEventPosition = *position;
}

// VERIFY: 888d0b7d auto
// OFFSET: 0x00715A80
void CGUnit_C::SetCastingSpell(uint8_t castCount, int32_t spellId) {
    this->m_castCount = castCount;
    this->m_castSpellId = spellId;
}

// VERIFY: 39ee7f23 auto
// OFFSET: 0x00715AA0
void CGUnit_C::SetCastSchoolImmunities(int32_t immunities) {
    this->m_castSchoolImmunities = immunities;
}

// VERIFY: 82aea5d5 auto
// OFFSET: 0x00715AB0
void CGUnit_C::SetCastImmunities(int32_t immunities) {
    this->m_castImmunities = immunities;
}

// VERIFY: ed54196e auto
// OFFSET: 0x00715AC0
void CGUnit_C::SetCastingAutoRepeat(bool enable) {
    if (enable) {
        this->m_animationState |= 0x200;
    } else {
        this->m_animationState &= ~0x200;
    }
}

// VERIFY: 160a8121 auto
// OFFSET: 0x00715AF0
void CGUnit_C::SetCastingAnim(bool enable) {
    if (enable) {
        this->m_animationState |= 0x400;
    } else {
        this->m_animationState &= ~0x400;
    }
}

// VERIFY: e7dd87c8 auto
// OFFSET: 0x00715B50
float CGUnit_C::GetBaseAlpha() const {
    if (this->m_displayInfo) {
        return this->m_displayInfo->m_creatureModelAlpha * (1.0f / 255.0f);
    }

    return 1.0f;
}

// VERIFY: 5c5ddc60 auto
// OFFSET: 0x00715C60
uint32_t CGUnit_C::GetTrackingType() {
    return s_trackingType;
}

// VERIFY: 89710c35 auto
// OFFSET: 0x00715CF0
float CGUnit_C::GetTrackingTurn() {
    return s_trackingFacing;
}

// VERIFY: 77188ed2 auto
// OFFSET: 0x00715D70
bool CGUnit_C::IsBossMob() const {
    if (this->m_creatureCacheEntry) {
        return (this->m_creatureCacheEntry->m_typeFlags >> 2) & 1;
    }

    return false;
}

// VERIFY: 627af4b4 auto
// OFFSET: 0x00715D90
bool CGUnit_C::IsLinkAll() const {
    if (this->m_creatureCacheEntry) {
        return (this->m_creatureCacheEntry->m_typeFlags >> 22) & 1;
    }

    return false;
}

// VERIFY: 6082cbf5 auto
// OFFSET: 0x00715DB0
bool CGUnit_C::HasNoShadowBlob() const {
    if (this->m_creatureCacheEntry) {
        return (this->m_creatureCacheEntry->m_typeFlags >> 25) & 1;
    }

    return false;
}

// VERIFY: 472a80ea manual
// OFFSET: 0x00715DD0
bool CGUnit_C::ShowFactionTooltip() const {
    return !this->m_creatureCacheEntry || !(this->m_creatureCacheEntry->m_typeFlags & 0x10);
}

// VERIFY: 2a450047 auto
// OFFSET: 0x00715DF0
bool CGUnit_C::IsTreatAsRaidUnit() const {
    if (this->m_creatureCacheEntry) {
        return (this->m_creatureCacheEntry->m_typeFlags >> 26) & 1;
    }

    return false;
}

// VERIFY: 1e582e23 auto
// OFFSET: 0x00715E10
bool CGUnit_C::IsForceGossip() const {
    if (this->m_creatureCacheEntry) {
        return (this->m_creatureCacheEntry->m_typeFlags >> 27) & 1;
    }

    return false;
}

// VERIFY: 43b9bada auto
// OFFSET: 0x00715E30
bool CGUnit_C::CanInteractWhileDead() const {
    return this->m_creatureCacheEntry && (this->m_creatureCacheEntry->m_typeFlags & 0x80);
}

// VERIFY: 568ed656 manual
// OFFSET: 0x00715E50
uint32_t CGUnit_C::GetSkinningType() const {
    if (!this->m_creatureCacheEntry) {
        return 0;
    }

    if (this->m_creatureCacheEntry->m_typeFlags & 0x100) {
        return 1;
    }

    if (this->m_creatureCacheEntry->m_typeFlags & 0x200) {
        return 2;
    }

    if (this->m_creatureCacheEntry->m_typeFlags & 0x8000) {
        return 3;
    }

    return 0;
}

// VERIFY: 79224dcb auto
// OFFSET: 0x00715F20
void CGUnit_C::GetCursorFile(int32_t unable, char* buffer, uint32_t size) const {
    if (unable) {
        SStrPrintf(buffer, size, "Interface\\Cursor\\Unable%s.blp", this->m_creatureCacheEntry->m_iconName);
    } else {
        SStrPrintf(buffer, size, "Interface\\Cursor\\%s.blp", this->m_creatureCacheEntry->m_iconName);
    }
}

// VERIFY: c4fa9258 auto
// OFFSET: 0x00715F70
bool CGUnit_C::MountedCombatAllowed() const {
    if (this->m_creatureCacheEntry) {
        return (this->m_creatureCacheEntry->m_typeFlags >> 11) & 1;
    }

    return false;
}

// VERIFY: a73b6e27 auto
// OFFSET: 0x00715F90
bool CGUnit_C::CanAidPlayers() const {
    if (this->m_creatureCacheEntry) {
        return (this->m_creatureCacheEntry->m_typeFlags >> 12) & 1;
    }

    return false;
}

// VERIFY: 00017622 auto
// OFFSET: 0x00715FB0
uint64_t CGUnit_C::GetLooter() const {
    return this->m_looter.m_guid;
}

// VERIFY: 1900c06b auto
// OFFSET: 0x00715FC0
uint64_t CGUnit_C::GetMasterLooter() const {
    return this->m_masterLooter.m_guid;
}

// VERIFY: d8d121aa auto
// OFFSET: 0x00715FD0
void CGUnit_C::InitWheels() {
    this->m_wheelMask = 0;
    CM2Model* model = this->GetObjectModel();

    for (uint32_t bone = 27; bone <= 34; bone++) {
        if (model->HasBone(bone)) {
            this->m_wheelMask |= 1 << (bone - 27);
            this->m_wheelAngles[bone - 27] = 0.0f;
        }
    }
}

// VERIFY: 3f9e9d5b auto
// OFFSET: 0x00716050
void CGUnit_C::SetPredictedHealth(int32_t health) {
    this->m_predictedHealth = health;
}

// VERIFY: aff444a7 manual
// OFFSET: 0x00718080
float CGUnit_C::GetMaxCameraHeight() const {
    float height = this->m_movementData.m_collisionHeight;

    if ((this->m_obj->m_type & TYPE_PLAYER) || height <= 2.0277777f) {
        return height - 0.16666667f;
    }

    return 2.0277777f - 0.16666667f;
}

// VERIFY: 5cc27cbd auto
// OFFSET: 0x00718A00
uint32_t CGUnit_C::GetCreatureClassification() const {
    if (!this->m_creatureCacheEntry || this->m_unit->m_unitFieldPetnumber) {
        return 0;
    }

    return this->m_creatureCacheEntry->m_rank;
}

// VERIFY: d337d178 auto
// OFFSET: 0x00718A20
int32_t CGUnit_C::IsDeepInLiquid() const {
    float depth = 0.0f;
    int32_t liquidType;
    float height;
    int32_t flags;

    if (World::QueryObjectLiquid(this->m_worldObject, &liquidType, &height, &flags)) {
        depth = height - this->GetPosition().z;
    }

    return this->m_movementData.m_collisionHeight * 0.5f < depth;
}

// VERIFY: b255f639 manual
// OFFSET: 0x00718A90
bool CGUnit_C::DoNotLogDeath() const {
    if (this->m_obj->m_type & TYPE_PLAYER) {
        return false;
    }

    if (this->m_creatureCacheEntry) {
        return (this->m_creatureCacheEntry->m_typeFlags >> 10) & 1;
    }

    return true;
}

// VERIFY: a3b6dbb4 auto
// OFFSET: 0x00718B10
uint32_t CGUnit_C::GetVirtualItemDisplayID(VIRTUAL_MONSTER_SLOT slot) const {
    return this->m_virtualItemDisplayId[slot];
}

// VERIFY: 349e3c57 auto
// OFFSET: 0x00718B30
bool CGUnit_C::FactionHasReputation(int32_t factionId) {
    FactionRec* faction = g_factionDB.GetRecord(factionId);
    return faction && faction->m_reputationIndex >= 0;
}

// VERIFY: 60955254 manual
// OFFSET: 0x00718B70
CGUnit_C* CGUnit_C::GetControllingPlayer() {
    CGUnit_C* unit = this;
    const WOWGUID* controller = this->GetCharmerOrCreatorGuid();

    if (*controller) {
        controller = this->GetCharmerOrCreatorGuid();
        unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*controller, TYPE_UNIT, __FILE__, __LINE__));
    }

    if (!unit) {
        return nullptr;
    }

    if (!(unit->m_obj->m_type & TYPE_PLAYER)) {
        controller = unit->GetCharmerOrCreatorGuid();
        unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*controller, TYPE_UNIT, __FILE__, __LINE__));

        if (!unit || !(unit->m_obj->m_type & TYPE_PLAYER)) {
            return nullptr;
        }
    }

    return unit;
}

// VERIFY: 8777f146 auto
// OFFSET: 0x00718FC0
int32_t CGUnit_C::IsDisarmed(int32_t hand) const {
    if (hand == 2) {
        return 0;
    }

    if (this->m_unit->m_unitFieldFlags & 0x200000) {
        const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, true);

        if (!hand) {
            return item && item->m_classID == 2;
        }

        if (!this->IsDisarmed(0)) {
            const CGUnitVirtualItem* offhand = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, true);

            if (offhand && offhand->m_classID == 2) {
                return 1;
            }
        }
    }

    return hand == 1 && (this->m_unit->m_unitFieldFlags2 & 0x80);
}

// VERIFY: d1a9830c auto
// OFFSET: 0x007192C0
int32_t CGUnit_C::IsQuestGiver() const {
    return (this->m_unit->m_unitNpcFlags >> 1) & 1;
}

// VERIFY: 28dbc451 auto
// OFFSET: 0x007192E0
void CGUnit_C::UpdateInteractIcon() {
    if (this->m_unit->m_unitFieldHealth > 0 || (this->m_creatureCacheEntry && (this->m_creatureCacheEntry->m_typeFlags & 0x80))) {
        CGObject_C::UpdateInteractIcon();
    }
}

// VERIFY: 773add64 manual
// OFFSET: 0x00719310
void CGUnit_C::UpdateInteractIconAttach() {
    if (!this->m_interactIcon || !this->m_worldModel || !this->m_worldModel->IsLoaded(0, 0)) {
        return;
    }

    if (this->m_interactIcon->m_attachParent) {
        this->m_interactIcon->DetachFromParent();
    }

    uint32_t attachment;

    if (this->m_mountModel && this->m_worldModel->HasAttachment(29)) {
        attachment = 29;
    } else {
        if (!this->m_worldModel->HasAttachment(18)) {
            return;
        }

        attachment = 18;
    }

    this->m_interactIcon->AttachToParent(this->m_worldModel, attachment, nullptr, 0);
    this->UpdateInteractIconScale();
    this->ObjectNameVisibilityChanged();
}

// VERIFY: c0742b82 auto
// OFFSET: 0x007193C0
void CGUnit_C::UpdateInteractIconScale() {
    if (!this->m_mountModel || this->m_mountModel->IsLoaded(0, 0)) {
        CGObject_C::UpdateInteractIconScale();
    }
}

// VERIFY: 6ba1cbf2 auto
// OFFSET: 0x007195D0
CreatureSoundDataRec* CGUnit_C::GetMountSoundDataRec() const {
    CreatureDisplayInfoRec* displayInfo = g_creatureDisplayInfoDB.GetRecord(this->m_mountDisplayId);

    if (!displayInfo) {
        return nullptr;
    }

    CreatureSoundDataRec* soundData = g_creatureSoundDataDB.GetRecord(displayInfo->m_soundID);

    if (soundData) {
        return soundData;
    }

    CreatureModelDataRec* modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
    return g_creatureSoundDataDB.GetRecord(modelData->m_soundID);
}

// VERIFY: 7bdb435b manual
// OFFSET: 0x00719900
int32_t __stdcall CGUnit_C::GetRecordForTitle(int32_t maskId) {
    for (int32_t i = 0; i < g_charTitlesDB.m_numRecords; i++) {
        CharTitlesRec* rec = (i < 0 || i >= g_charTitlesDB.m_numRecords) ? nullptr : &g_charTitlesDB.m_records[i];

        if (rec && rec->m_maskID == maskId) {
            return rec->m_id;
        }
    }

    return 0;
}

// VERIFY: 4b0cd521 manual
// OFFSET: 0x00719950
const char* CGUnit_C::GetUnitTitle() const {
    if (!this->m_creatureCacheEntry) {
        return nullptr;
    }

    const char* title = this->m_creatureCacheEntry->m_subName;

    if (!*title || this->m_unit->m_unitFieldPetnumber) {
        return nullptr;
    }

    return title;
}

// VERIFY: 54b589a7 manual
// OFFSET: 0x0071A030
int32_t CGUnit_C::IsSplashing(const C3Vector& position) const {
    int32_t liquidType;
    float height;
    int32_t flags;

    if (World::QueryObjectLiquid(this->m_worldObject, &liquidType, &height, &flags)) {
        int32_t flag8;
        int32_t flag9;
        World::QueryObjectLiquidFlags(this->m_worldObject, &flag8, &flag9);

        if (flag9 && height + 0.0099999998f > position.z && !(this->m_movement->m_flags & MOVEMENTFLAG_SWIMMING)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 066b72ae auto
// OFFSET: 0x0071A380
UNITSTANDSTATE CGUnit_C::GetClientStandState() const {
    return static_cast<UNITSTANDSTATE>(this->m_unit->m_unitFieldBytes1[0]);
}

// VERIFY: 232c855f manual
// OFFSET: 0x0071A430
int32_t CGUnit_C::HasBakedNpcTexture() const {
    if ((this->m_obj->m_type & TYPE_PLAYER) && this->m_modelData && (this->m_modelData->m_flags & 0x4) && this->m_displayInfoExtra && (this->m_displayInfoExtra->m_flags & 0x1)) {
        return 1;
    }

    return 0;
}

// VERIFY: c01a2903 auto
// OFFSET: 0x0071A470
uint32_t CGUnit_C::GetDisplayRace() const {
    if (this->m_displayInfoExtra) {
        return this->m_displayInfoExtra->m_displayRaceID;
    }

    return this->m_unit->m_unitFieldBytes0[0];
}

// VERIFY: f436709c auto
// OFFSET: 0x0071A490
UNIT_SEX CGUnit_C::GetDisplaySex() const {
    if (this->m_displayInfoExtra) {
        return static_cast<UNIT_SEX>(this->m_displayInfoExtra->m_displaySexID);
    }

    return static_cast<UNIT_SEX>(this->m_unit->m_unitFieldBytes0[2]);
}

// VERIFY: d86707be auto
// OFFSET: 0x0071AA70
int32_t CGUnit_C::GetSpellRank(const SpellRec* spell) const {
    int32_t rank = 5 * this->m_unit->m_unitFieldLevel;

    if (spell->m_maxLevel > 0 && rank >= 5 * spell->m_maxLevel) {
        rank = 5 * spell->m_maxLevel;
    }

    if (rank < 0) {
        return 0;
    }

    return rank;
}

// VERIFY: d8b95a7c manual
// OFFSET: 0x0071AC30
void CGUnit_C::ApplyAlpha(uint32_t time) {
    CM2Model* model = this->GetObjectModel();

    if (this->m_forceWorldModelAlpha) {
        model = this->m_worldModel;
    }

    if (!model) {
        return;
    }

    if (this->m_fadeDuration) {
        if (static_cast<int32_t>(time - this->m_fadeStartTime - this->m_fadeDuration) < 0) {
            this->m_alpha = this->m_fadeStartAlpha + static_cast<int32_t>((time - this->m_fadeStartTime) * (this->m_fadeEndAlpha - this->m_fadeStartAlpha)) / static_cast<int32_t>(this->m_fadeDuration);
        } else {
            this->m_alpha = this->m_fadeEndAlpha;
            this->m_fadeDuration = 0;
        }
    }

    model->m_fadeAlpha = static_cast<float>(this->m_alphaScale) * this->m_alpha * (1.0f / 65025.0f);
}

// VERIFY: b788c577 auto
// OFFSET: 0x0071AD20
int32_t CGUnit_C::GetSpellCastingTime(const SpellRec* spell) const {
    if (!spell) {
        return 0;
    }

    SpellCastTimesRec* castTime = g_spellCastTimesDB.GetRecord(spell->m_castingTimeIndex);

    if (!castTime) {
        return 0;
    }

    int32_t time = castTime->m_base + castTime->m_perLevel * static_cast<int32_t>(static_cast<uint32_t>(this->GetSpellRank(spell)) / 5 - spell->m_baseLevel);

    if (time < castTime->m_minimum) {
        time = castTime->m_minimum;
    }

    if (!(spell->m_attributes & 0x30) && !(spell->m_attributesExC & 0x20000000) && time > 0) {
        CGUnitData* unit = this->m_unit;

        if (fnotequal(unit->m_unitModCastSpeed, 1.0f)) {
            time = static_cast<int32_t>(time * unit->m_unitModCastSpeed);
        }
    }

    if (spell->m_attributes & 0x2) {
        time = 0x7FFFFFFF;
    }

    return time <= 0 ? 0 : time;
}

// VERIFY: f55ea921 auto
// OFFSET: 0x0071AF70
uint32_t CGUnit_C::GetShapeshiftForm() const {
    if (this->m_shapeshiftSuppressed) {
        return 0;
    }

    return this->m_unit->m_unitFieldBytes2[3];
}

// VERIFY: ae4e25f4 auto
// OFFSET: 0x0071AF90
int32_t CGUnit_C::IsInMelee() const {
    return static_cast<bool>(this->m_combat.m_attacking);
}

// VERIFY: 7ef7ac88 manual
// OFFSET: 0x0071AFB0
int32_t CGUnit_C::IsInCombat() const {
    if (this->m_unit->m_unitFieldFlags & 0x800) {
        return 1;
    }

    return static_cast<bool>(this->m_combat.m_attacking);
}

// VERIFY: b236545b auto
// OFFSET: 0x0071AFE0
void CGUnit_C::UnRootEffects() {
    for (CEffect* effect = this->m_effectList; effect; effect = effect->m_next) {
        effect->m_flags &= ~0x4000;
    }
}

// VERIFY: e72ba487 auto
// OFFSET: 0x0071B600
uint64_t CGUnit_C::GetCreatorGuid() const {
    CGUnitData* unit = this->m_unit;

    if (unit->m_unitFieldCharmedby || unit->m_unitFieldSummonedby) {
        return 0;
    }

    return unit->m_unitFieldCreatedby.m_guid;
}

// VERIFY: b6dbb791 manual
// OFFSET: 0x0071B630
bool CGUnit_C::CanGainXP() const {
    CGUnitData* unit = this->m_unit;

    if (!unit->m_unitFieldPetnumber) {
        return false;
    }

    CGUnit_C* owner = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(unit->m_unitFieldCreatedby, TYPE_PLAYER, __FILE__, __LINE__));
    return owner && owner->m_unit->m_unitFieldBytes0[1] == 3;
}

// VERIFY: 6c86f70b manual
// OFFSET: 0x0071B680
int32_t CGUnit_C::IsPVPLeader() const {
    if (!(this->m_unit->m_unitFieldBytes2[1] & 0x1)) {
        return 0;
    }

    if (!this->m_creatureCacheEntry || !this->m_creatureCacheEntry->m_racialLeader) {
        return 0;
    }

    return 1;
}

// VERIFY: 931b5cd6 manual
// OFFSET: 0x0071B740
void CGUnit_C::GetRangedAttackSkillRank(int32_t& rank, int32_t& bonus) {
    if (this->m_obj->m_type & TYPE_PLAYER) {
        static_cast<CGPlayer_C*>(this)->GetRangedAttackSkillRank(rank, bonus);
        return;
    }

    rank = 0;
    bonus = 0;
}

// VERIFY: 514b93b6 auto
// OFFSET: 0x0071B770
uint32_t CGUnit_C::GetBasePowerForType(POWER_TYPE powerType) const {
    switch (powerType) {
        case POWER_TYPE_HEALTH:
            return this->m_unit->m_unitFieldBaseHealth;

        case POWER_TYPE_MANA:
            return this->m_unit->m_unitFieldBaseMana;

        case POWER_TYPE_RAGE:
        case POWER_TYPE_RUNIC_POWER:
            return 1000;

        case POWER_TYPE_FOCUS:
        case POWER_TYPE_ENERGY:
            return 100;

        case POWER_TYPE_RUNES:
            return 1;

        default:
            return 0;
    }
}

// VERIFY: b148d15a auto
// OFFSET: 0x0071B810
uint32_t CGUnit_C::GetCanFly() const {
    return this->m_movementData.m_flags & MOVEMENTFLAG_CAN_FLY;
}

// VERIFY: 54d048fe manual
// OFFSET: 0x0071B820
int32_t CGUnit_C::IsInMeleeRange(CGUnit_C* target) const {
    CGUnit* targetUnit = target ? static_cast<CGUnit*>(target) : nullptr;
    float range = this->m_unit->m_unitFieldCombatreach + targetUnit->m_unit->m_unitFieldCombatreach + 1.3333334f;

    if (range < 5.0f) {
        range = 5.0f;
    }

    C3Vector targetPosition = target->GetPosition();
    C3Vector position = this->GetPosition();
    float dx = targetPosition.x - position.x;
    float dy = targetPosition.y - position.y;
    float dz = targetPosition.z - position.z;
    return range * range > dx * dx + dz * dz + dy * dy;
}

// VERIFY: fd381439 manual
// OFFSET: 0x00716710
bool CGUnit_C::IsClientControlled() const {
    if (!(this->m_unit->m_unitFieldFlags & 0x2) && (this->m_unit->m_unitFieldFlags & 0xC00004)) {
        return false;
    }

    CGUnitData* unit = this->m_unit;
    uint32_t flags = unit->m_unitFieldFlags;

    if (flags & 0x1000000) {
        const WOWGUID* controller = this->GetCharmerOrCreatorGuid();
        CGUnit_C* owner = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*controller, TYPE_UNIT, __FILE__, __LINE__));

        if (!owner || !(owner->m_obj->m_type & TYPE_PLAYER)) {
            return false;
        }

        return !(owner->m_unit->m_unitFieldFlags & 0x1);
    }

    if (!(this->m_obj->m_type & TYPE_PLAYER) || unit->m_unitFieldCharmedby) {
        return false;
    }

    return !(flags & 0x1);
}

// VERIFY: a355861f manual
// OFFSET: 0x00716F10
void CGUnit_C::RequestMirrorImageData() {
    this->m_clientFlags |= 0x20000;

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GET_MIRROR_IMAGE_DATA));
    msg.Put(this->m_obj->m_guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: a2e758f2 auto
// OFFSET: 0x00716FD0
int32_t CGUnit_C::ChooseSpecialIdleAnim(uint32_t flags, uint32_t* anim) const {
    if (!(this->m_animationState & 0x40) || this->m_mountModel || this->m_animStandState != UNIT_NUMSTANDSTATES) {
        return (flags & 0xFFFFF800) == 0;
    }

    if (this->m_clientFlags & 0x40000000) {
        return 0;
    }

    if (!this->m_worldModel->HasSequence(127)) {
        return (flags & 0xFFFFFFFC) == 0;
    }

    *anim = 127;
    return 1;
}

// VERIFY: 5f734b90 auto
// OFFSET: 0x007171C0
int32_t CGUnit_C::ChooseEmoteAnim(uint32_t flags, uint32_t* anim) const {
    uint32_t state = this->m_animationState;

    if ((!(state & 0x40) || this->m_mountModel) && !(state & 0x20)) {
        return (flags & 0x200) == 0;
    }

    int32_t emoteState = this->m_unit->m_unitNpcEmotestate;

    if (!emoteState) {
        return (flags & 0x200) == 0;
    }

    EmotesRec* emote = g_emotesDB.GetRecord(emoteState);

    if (!emote || (CGGameUI::m_interactTarget == this->m_obj->m_guid && (emote->m_emoteFlags & 0x2000))) {
        return (flags & 0x200) == 0;
    }

    if (flags & 0x200) {
        *anim = emote->m_animID;
    }

    return 1;
}

// VERIFY: a913b882 auto
// OFFSET: 0x00717260
uint32_t CGUnit_C::GetCurrentTorsoAnimId() const {
    if (!this->m_worldModel || !this->m_worldModel->IsLoaded(0, 0)) {
        return 0xFFFFFFFF;
    }

    if (this->m_torsoKeyBone == -1) {
        return this->m_worldModel->GetBoneSequenceId(0xFFFFFFFF);
    }

    uint32_t anim = this->m_worldModel->GetBoneSequenceId(this->m_torsoKeyBone);

    if (anim == 0xFFFFFFFF) {
        return this->m_worldModel->GetBoneSequenceId(0xFFFFFFFF);
    }

    return anim;
}

// VERIFY: 98a0fb8b auto
// OFFSET: 0x007172B0
uint32_t CGUnit_C::GetCurrentBaseAnimId() const {
    if (this->m_worldModel && this->m_worldModel->IsLoaded(0, 0)) {
        return this->m_worldModel->GetBoneSequenceId(0xFFFFFFFF);
    }

    return 0xFFFFFFFF;
}

// VERIFY: 6b5cc13c auto
// OFFSET: 0x007172E0
uint32_t CGUnit_C::GetCurrentMountAnimId() const {
    if (this->m_mountModel && this->m_mountModel->IsLoaded(0, 0)) {
        return this->m_mountModel->GetBoneSequenceId(0xFFFFFFFF);
    }

    return 0xFFFFFFFF;
}

// VERIFY: ab38fe5b manual
// OFFSET: 0x007173F0
void CGUnit_C::GetCurrentAnimationInfo(M2BoneSequenceInfo* mount, M2BoneSequenceInfo* base, M2BoneSequenceInfo* torso, int32_t keepFinished) const {
    mount->m_sequenceId = 0;
    mount->m_variationIndex = 0;
    mount->m_time = 0;
    mount->m_speed = 0.0f;
    mount->m_startTime = 0;
    mount->m_endTime = 0;
    mount->m_finished = 0;
    mount->m_complete = 0;

    if (this->m_mountModel && this->m_mountModel->IsLoaded(0, 0)) {
        this->m_mountModel->GetBoneSequenceInfo(0xFFFFFFFF, *mount);
    } else {
        mount->m_finished = 1;
    }

    if (mount->m_complete) {
        mount->m_finished = 1;
    }

    if (mount->m_finished) {
        mount->m_sequenceId = 0xFFFFFFFF;
        mount->m_speed = 1.0f;
        mount->m_variationIndex = 0xFFFFFFFF;
        mount->m_time = 0;
    }

    base->m_sequenceId = 0;
    base->m_variationIndex = 0;
    base->m_time = 0;
    base->m_speed = 0.0f;
    base->m_startTime = 0;
    base->m_endTime = 0;
    base->m_finished = 0;
    base->m_complete = 0;
    this->m_worldModel->GetBoneSequenceInfo(0xFFFFFFFF, *base);

    if (base->m_complete) {
        base->m_finished = 1;
    }

    if (base->m_finished && !keepFinished) {
        base->m_sequenceId = 0xFFFFFFFF;
        base->m_speed = 1.0f;
        base->m_variationIndex = 0xFFFFFFFF;
        base->m_time = 0;
    }

    torso->m_sequenceId = 0;
    torso->m_variationIndex = 0;
    torso->m_time = 0;
    torso->m_speed = 0.0f;
    torso->m_startTime = 0;
    torso->m_endTime = 0;
    torso->m_finished = 0;
    torso->m_complete = 0;

    if (this->m_torsoKeyBone == -1) {
        torso->m_finished = 1;
    } else {
        this->m_worldModel->GetBoneSequenceInfo(this->m_torsoKeyBone, *torso);
    }

    if (torso->m_complete) {
        torso->m_finished = 1;
    }

    if (torso->m_finished && !keepFinished) {
        torso->m_sequenceId = 0xFFFFFFFF;
        torso->m_speed = 1.0f;
        torso->m_variationIndex = 0xFFFFFFFF;
        torso->m_time = 0;
    }
}

// VERIFY: 71fe5a98 manual
// OFFSET: 0x00717600
int32_t CGUnit_C::FindDefaultAnimForTier(int32_t tier, uint32_t* anim) const {
    if (g_animationDataDB.m_numRecords <= 0) {
        return 0;
    }

    AnimationDataRec* rec = g_animationDataDB.m_records;
    uint32_t id;

    if (!tier && rec && !rec->m_behaviorID && !rec->m_behaviorTier) {
        id = rec->m_id;
    } else {
        AnimationDataRec* stand = g_animationDataDB.GetRecord(0);

        if (stand && stand->m_behaviorTier) {
            id = 0;
        } else {
            int32_t i = 0;

            while (true) {
                AnimationDataRec* candidate = (i < 0 || i >= g_animationDataDB.m_numRecords) ? nullptr : rec;

                if (!candidate->m_behaviorID && candidate->m_behaviorTier == tier) {
                    id = candidate->m_id;
                    break;
                }

                i++;
                rec++;

                if (i >= g_animationDataDB.m_numRecords) {
                    return 0;
                }
            }
        }
    }

    if (!this->m_worldModel->HasSequence(id)) {
        return 0;
    }

    *anim = id;
    return 1;
}

// VERIFY: 272cc5b1 manual
// OFFSET: 0x00717A20
CreatureModelDataRec* CGUnit_C::GetModelData() const {
    int32_t displayId = this->m_displayId;

    if (!displayId || this->m_unit->m_unitFieldNativedisplayid != this->m_unit->m_unitFieldDisplayid) {
        displayId = this->m_unit->m_unitFieldDisplayid;
    }

    CreatureDisplayInfoRec* displayInfo = g_creatureDisplayInfoDB.GetRecord(displayId);

    if (!displayInfo) {
        SysMsgPrintf(SYSMSG_WARNING, 2, "NOCREATUREDISPLAYIDFOUND|%d", displayId);
        return nullptr;
    }

    CreatureModelDataRec* modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);

    if (!modelData) {
        SysMsgPrintf(SYSMSG_WARNING, 16, "INVALIDDISPLAYMODELRECORD|%d|%d", displayInfo->m_modelID, displayInfo->m_id);
        return nullptr;
    }

    return modelData;
}

// VERIFY: 2fde84be auto
// OFFSET: 0x00717B60
int32_t CGUnit_C::CanBeLooted(uint32_t time) const {
    CGUnitData* unit = this->m_unit;
    return unit->m_unitFieldHealth <= 0 && (!this->m_lootableTime || static_cast<int32_t>(time - this->m_lootableTime) >= 0) && (unit->m_unitDynamicFlags & 0x1);
}

// VERIFY: e9e8e2f9 auto
// OFFSET: 0x0071C0E0
float CGUnit_C::GetTrueScale() const {
    float scale = this->m_modelScale * this->m_scaleModifier * this->m_scale;

    if (this->m_mountModel) {
        return scale * this->m_mountScale;
    }

    return scale;
}

// VERIFY: bbc73873 manual
// OFFSET: 0x0071C260
void CGUnit_C::ModifyPredictedHealth(int32_t delta) {
    if (delta < 0 && (this->m_unit->m_unitFieldFlags2 & 0x100)) {
        return;
    }

    int32_t health = delta + this->m_predictedHealth;

    if (health < 1) {
        this->m_predictedHealth = 1;
    } else {
        if (health > static_cast<int32_t>(this->m_unit->m_unitFieldMaxhealth)) {
            health = this->m_unit->m_unitFieldMaxhealth;
        }

        this->m_predictedHealth = health;
    }
}

// VERIFY: bf8a5563 auto
// OFFSET: 0x0071C320
void CGUnit_C::InitPredictedPower() {
    this->m_predictedPower[0] = this->m_unit->m_unitFieldPower[0];
    this->m_predictedPower[1] = this->m_unit->m_unitFieldPower[1];
    this->m_predictedPower[2] = this->m_unit->m_unitFieldPower[2];
    this->m_predictedPower[3] = this->m_unit->m_unitFieldPower[3];
    this->m_predictedPower[4] = this->m_unit->m_unitFieldPower[4];
    this->m_predictedPower[5] = this->m_unit->m_unitFieldPower[5];
    this->m_predictedPower[6] = this->m_unit->m_unitFieldPower[6];
}

// VERIFY: 61272ca6 auto
// OFFSET: 0x0071C390
void CGUnit_C::SetManaRegenInterrupt() {
    this->m_manaRegenInterruptTime = FrameTime::s_curTimeMs + 5000;
}

// VERIFY: 7e46098c manual
// OFFSET: 0x0071C500
int32_t CGUnit_C::UsesNpcBakedTexture() const {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    if ((this->m_obj->m_type & TYPE_PLAYER) && this->m_modelData && (this->m_modelData->m_flags & 0x4) && this->m_displayInfoExtra && (this->m_displayInfoExtra->m_flags & 0x1)) {
        return 1;
    }

    if (!this->m_displayInfoExtra && (this->m_obj->m_type & TYPE_PLAYER) && (this->m_modelData->m_flags & 0x4)) {
        return 1;
    }

    return 0;
}

// VERIFY: 97e6d037 manual
// OFFSET: 0x0071C600
int32_t __stdcall CGUnit_C::MaybeDeferTeleport(const WOWGUID* transport) {
    return *transport && !ClntObjMgrObjectPtr(*transport, TYPE_OBJECT, __FILE__, __LINE__);
}

// VERIFY: 013dc073 auto
// OFFSET: 0x0071DF30
int32_t CGUnit_C::OverrideAnimForFlying(uint32_t* anim) const {
    if (anim[0] == 0xFFFFFFFF) {
        return 0;
    }

    M2SequenceInfo info = {};
    this->m_worldModel->GetSequenceInfo(anim[0], anim[1], info);

    if (info.m_animId && info.m_animId != 8 && info.m_animId != 25) {
        return 0;
    }

    anim[0] = 193;
    anim[1] = 0xFFFFFFFF;
    return 1;
}

// VERIFY: eab41c40 auto
// OFFSET: 0x0071F300
int32_t CGUnit_C::GetCreatureType() const {
    uint32_t form = this->m_shapeshiftSuppressed ? 0 : this->m_unit->m_unitFieldBytes2[3];
    SpellShapeshiftFormRec* shapeshift = g_spellShapeshiftFormDB.GetRecord(form);

    if (shapeshift && shapeshift->m_creatureType > 0) {
        return shapeshift->m_creatureType;
    }

    if (this->m_creatureCacheEntry) {
        return this->m_creatureCacheEntry->m_type;
    }

    ChrRacesRec* race = g_chrRacesDB.GetRecord(this->m_unit->m_unitFieldBytes0[0]);

    if (!race || race->m_creatureType <= 0) {
        return 0;
    }

    return race->m_creatureType;
}

// VERIFY: b7deec71 manual
// OFFSET: 0x0071F390
PetPersonalityRec* CGUnit_C::GetPetPersonality() const {
    CGUnitData* unit = this->m_unit;

    if (!unit->m_unitFieldPetnumber) {
        return nullptr;
    }

    CGUnit_C* owner = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(unit->m_unitFieldCreatedby, TYPE_PLAYER, __FILE__, __LINE__));

    if (!owner || owner->m_unit->m_unitFieldBytes0[1] != 3) {
        return nullptr;
    }

    return g_petPersonalityDB.GetRecord(1);
}

// VERIFY: 1beb8441 auto
// OFFSET: 0x0071FB60
void CGUnit_C::GetFootprintInfo(int32_t* textureId, float* size) const {
    float length;
    float width;

    if (this->m_mountDisplayId <= 0 || (this->m_clientFlags & 0x10000000) || !this->m_mountModel) {
        *textureId = this->m_footprintTextureId;
        float scale = this->GetTrueScale();
        length = this->m_footprintLength * scale;
        width = scale * this->m_footprintWidth;
    } else {
        *textureId = this->m_mountFootprintTextureId;
        float scale = this->GetTrueScale();
        length = this->m_mountFootprintLength * scale;
        width = scale * this->m_mountFootprintWidth;
    }

    size[1] = width;
    size[0] = length;
}

// VERIFY: 99ea3d4f auto
// OFFSET: 0x0071FFC0
int32_t CGUnit_C::IsControlledByViewer() const {
    return this->m_obj->m_guid == ClntObjMgrGetActivePlayer() || this->m_obj->m_guid == s_activeMover || (this->m_clientFlags & 0x1000);
}

// VERIFY: e21be286 manual
// OFFSET: 0x007202C0
int32_t CGUnit_C::CanEquipItemsInThisForm() const {
    if ((this->m_obj->m_type & TYPE_PLAYER) && this->m_modelData && (this->m_modelData->m_flags & 0x4) && this->m_displayInfoExtra && (this->m_displayInfoExtra->m_flags & 0x1)) {
        return 1;
    }

    if (this->m_shapeshiftSuppressed) {
        return 1;
    }

    CGUnitData* unit = this->m_unit;

    if (this->m_displayId) {
        return this->m_displayId == unit->m_unitFieldNativedisplayid;
    }

    return unit->m_unitFieldNativedisplayid == unit->m_unitFieldDisplayid;
}

// VERIFY: 8b19ad28 manual
// OFFSET: 0x00720AF0
uint32_t CGUnit_C::GetVoiceSex() const {
    if (this->m_displayInfoExtra) {
        return this->m_displayInfoExtra->m_displaySexID;
    }

    if (this->m_obj->m_type & TYPE_PLAYER) {
        return static_cast<const CGPlayer_C*>(this)->m_player->m_playerBytes3[0];
    }

    return this->m_unit->m_unitFieldBytes0[2];
}

// VERIFY: bc5ad5e9 auto
// OFFSET: 0x00721CA0
bool CGUnit_C::IsShapeShifted() const {
    if (this->m_shapeshiftSuppressed) {
        return false;
    }

    uint32_t form = this->m_unit->m_unitFieldBytes2[3];

    if (!form) {
        return false;
    }

    return !(g_spellShapeshiftFormDB.GetRecord(form)->m_flags & 0x1);
}

// VERIFY: ea61250a manual
// OFFSET: 0x00721CF0
void CGUnit_C::PreemptivelyCancelShapeshift() {
    if (!(this->m_obj->m_type & TYPE_PLAYER)) {
        return;
    }

    uint32_t form = this->m_shapeshiftSuppressed ? 0 : this->m_unit->m_unitFieldBytes2[3];
    SpellShapeshiftFormRec* shapeshift = g_spellShapeshiftFormDB.GetRecord(form);

    if (shapeshift) {
        CGUnitData* unit = this->m_unit;

        for (uint32_t i = 0; i < 4; i++) {
            if (unit->m_unitFieldDisplayid == shapeshift->m_creatureDisplayID[i]) {
                this->m_displayId = unit->m_unitFieldNativedisplayid;
                this->UpdateDisplayInfo(0);
                break;
            }
        }
    }

    this->m_shapeshiftSuppressed = 1;
    FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_SHAPESHIFT_FORM, nullptr);
}

// VERIFY: 0919a411 manual
// OFFSET: 0x00721E70
bool CGUnit_C::IsCancelShapeshiftAtFlightMaster() const {
    if (this->m_shapeshiftSuppressed) {
        return false;
    }

    uint32_t form = this->m_unit->m_unitFieldBytes2[3];

    if (!form) {
        return false;
    }

    int32_t flags = g_spellShapeshiftFormDB.GetRecord(form)->m_flags;
    return !(flags & 0x1) || (flags & 0x800);
}

// VERIFY: 1c8b9c45 auto
// OFFSET: 0x00721F90
int32_t CGUnit_C::IsAutoTracking() const {
    return this->m_obj->m_guid == s_activeMover && s_trackingType != TRACKTYPE_NONE;
}

// VERIFY: d88b4789 auto
// OFFSET: 0x00721FC0
bool CGUnit_C::IsActiveMoverTracking(uint32_t typeMask) const {
    if (s_trackingType == TRACKTYPE_NONE) {
        return false;
    }

    return this->m_obj->m_guid == s_activeMover && ((1 << s_trackingType) & typeMask);
}

// VERIFY: 33aad9dd auto
// OFFSET: 0x00722010
void CGUnit_C::TrackingMakeRelativeTo(const C44Matrix* transform, float facing) {
    if (this->m_obj->m_guid != s_activeMover) {
        return;
    }

    uint32_t type = s_trackingType;

    if (type == TRACKTYPE_NONE) {
        return;
    }

    if (type == TRACKTYPE_MOVE) {
        s_trackingPosition *= *transform;
        type = s_trackingType;
    }

    if (type == TRACKTYPE_FACE || type == TRACKTYPE_MOVE_DIRECTION) {
        s_trackingDirection = CMath::normalizeangle0to2pi_(s_trackingDirection + facing);
    }
}

// VERIFY: c2e94952 manual
// OFFSET: 0x007225E0
const char* CGUnit_C::GetFrameScriptText(const char* token, int32_t count) const {
    UNIT_SEX sex;

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        sex = static_cast<UNIT_SEX>(static_cast<const CGPlayer_C*>(this)->m_player->m_playerBytes3[0]);
    } else {
        sex = static_cast<UNIT_SEX>(this->m_unit->m_unitFieldBytes0[2]);
    }

    return FrameScript_GetText(token, count, sex == UNITSEX_FEMALE ? GENDER_FEMALE : GENDER_MALE);
}

// VERIFY: 2b9830ba manual
// OFFSET: 0x00722B50
C44Matrix CGUnit_C::GetMatrix() const {
    if (this->m_vehicle && this->m_vehicle->m_vehicleRec) {
        return this->m_vehicle->m_worldMatrix;
    }

    C44Matrix matrix;
    C3Vector position = this->GetRawPosition();
    matrix.d0 = position.x;
    matrix.d1 = position.y;
    matrix.d2 = position.z;
    matrix.RotateAroundZ(this->GetRawFacing());

    if (this->GetTransportGUID()) {
        CGObject_C* transport = ClntObjMgrObjectPtr(WOWGUID{ this->GetTransportGUID() }, TYPE_OBJECT, __FILE__, __LINE__);
        matrix *= transport->GetMatrix();
    }

    return matrix;
}

// VERIFY: d769e18c manual
// OFFSET: 0x00723490
uint64_t CGUnit_C::GetAuraCaster(uint32_t index) {
    const WOWGUID& creator = this->m_auras[index].m_creator;
    return creator.m_guid;
}

// VERIFY: f98b8e30 auto
// OFFSET: 0x0074B880
void CGUnit_C::VehiclePassengerInitWorldCameraState() {
    CGUnit_C::s_vehiclePassengerCameraActive = 0;
    CGUnit_C::s_vehiclePassengerCameraVehicle = nullptr;
    CGUnit_C::s_vehiclePassengerCameraSeat = nullptr;
}

// VERIFY: 352da6a6 auto
// OFFSET: 0x0074B8A0
void CGUnit_C::VehiclePassengerOnUninit() {
    this->m_vehiclePassenger = nullptr;
}

// VERIFY: 014f995c auto
// OFFSET: 0x0074B8B0
int32_t CGUnit_C::HasVehicleTransport() const {
    uint32_t high = static_cast<uint32_t>(this->m_movementData.m_transportGuid.m_guid >> 32);
    return (high & 0xF0F00000) == 0xF0500000 || (!(high & 0xF0000000) && ((high & 0xF07FFFFF) | static_cast<uint32_t>(this->m_movementData.m_transportGuid.m_guid)));
}

// VERIFY: 5a947930 auto
// OFFSET: 0x0056C220
bool CGUnit_C::IsInVehicle() const {
    return this->m_vehiclePassenger && this->m_vehiclePassenger->m_state;
}

// VERIFY: 2230d2c7 auto
// OFFSET: 0x00714AD0
int32_t Packet_SMSG_MULTIPLE_PACKETS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    while (!msg->IsRead() && msg->m_read <= msg->m_size) {
        ClientServices::Connection()->ProcessMessage(time, msg, 1);
    }

    return 1;
}

// VERIFY: 3b3b64a0 auto
// OFFSET: 0x00714B20
int32_t Packet_SMSG_UNKNOWN_1240(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    char text[512];
    msg->Get(guid.m_guid);
    msg->GetString(text, sizeof(text));
    return 1;
}

// VERIFY: e07f113e auto
// OFFSET: 0x00714C10
float CopySign(float value, float sign) {
    uint32_t bits = (*reinterpret_cast<uint32_t*>(&sign) ^ *reinterpret_cast<uint32_t*>(&value)) & 0x7FFFFFFF;
    bits ^= *reinterpret_cast<uint32_t*>(&sign);
    return *reinterpret_cast<float*>(&bits);
}

// VERIFY: d2d3074a manual
// OFFSET: 0x007167C0
int32_t AnimTierChangeHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        int32_t tier = unit->m_unit->m_unitFieldBytes1[3];

        if (tier != unit->m_animTier) {
            unit->m_animTier = tier;
        }
    }

    return 1;
}

// VERIFY: f5f87576 manual
// OFFSET: 0x00716900
int32_t TargetChangeHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->m_targetChangeTime = OsGetAsyncTimeMs();
    }

    return 1;
}

// VERIFY: 88a563c1 auto
// OFFSET: 0x00716C00
int32_t Packet_SMSG_PET_DISMISS_SOUND(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t modelId;
    msg->Get(modelId);

    C3Vector position;
    msg->Get(position.x);
    msg->Get(position.y);
    msg->Get(position.z);

    CreatureModelDataRec* modelData = g_creatureModelDataDB.GetRecord(modelId);

    if (modelData) {
        CreatureSoundDataRec* soundData = g_creatureSoundDataDB.GetRecord(modelData->m_soundID);

        if (soundData && soundData->m_soundPetDismissID) {
            position.z = position.z + 1.0f;
            SI2::PlaySoundKit(soundData->m_soundPetDismissID, &position, nullptr, nullptr, 0, nullptr, 1, 0);
        }
    }

    return 1;
}

// VERIFY: 3607660a manual
// OFFSET: 0x00717310
float CGUnit_C::GetTorsoAnimProgress() const {
    if (!this->m_worldModel || !this->m_worldModel->IsLoaded(0, 0)) {
        return 0.0f;
    }

    M2BoneSequenceInfo info;
    info.m_sequenceId = 0;
    info.m_variationIndex = 0;
    info.m_time = 0;
    info.m_speed = 0.0f;
    info.m_startTime = 0;
    info.m_finished = 0;
    info.m_complete = 0;
    info.m_endTime = 0;

    if (this->m_torsoKeyBone != -1) {
        this->m_worldModel->GetBoneSequenceInfo(this->m_torsoKeyBone, info);
    }

    if (info.m_sequenceId == 0xFFFFFFFF) {
        this->m_worldModel->GetBoneSequenceInfo(0xFFFFFFFF, info);
    }

    float duration = static_cast<float>(static_cast<int32_t>(info.m_endTime - info.m_startTime));

    if (duration < 0.00000011920929f) {
        return 0.0f;
    }

    float progress = info.m_time / duration;

    if (progress < 0.0f) {
        return 0.0f;
    }

    if (progress >= 1.0f) {
        return 1.0f;
    }

    return progress;
}

// VERIFY: 7d576472 manual
// OFFSET: 0x00717EC0
void CGUnit_C::BuildSmoothMatrix(C44Matrix& matrix) const {
    matrix = C44Matrix::RotationAroundZ(this->m_smoothTarget.m_value);

    C3Vector position = this->GetRawPosition();
    matrix.d0 = position.x;
    matrix.d1 = position.y;
    matrix.d2 = position.z;

    CGObject_C* transport = ClntObjMgrObjectPtr(WOWGUID{ this->GetTransportGUID() }, TYPE_OBJECT, __FILE__, __LINE__);

    if (transport) {
        C44Matrix transportMatrix;

        if (transport->m_obj->m_type & TYPE_UNIT) {
            static_cast<CGUnit_C*>(transport)->BuildSmoothMatrix(transportMatrix);
        } else {
            transportMatrix = transport->GetMatrix();
        }

        matrix *= transportMatrix;
    }
}

// VERIFY: 73623b2d manual
// OFFSET: 0x00719980
bool __stdcall CGUnit_C::IsSpellAllowedInArena(const SpellRec* spell) {
    MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);

    if (map && map->m_instanceType == 4) {
        int32_t attributes = spell->m_attributesExD;

        if (!(attributes & 0x20000) && (spell->m_recoveryTime > 900000 || spell->m_categoryRecoveryTime > 900000 || (attributes & 0x10000))) {
            return false;
        }
    }

    return true;
}

// VERIFY: bba780f3 manual
// OFFSET: 0x00719A90
void CGUnit_C::GetSwimMatrix(C44Matrix& matrix) const {
    C33Matrix rotation(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    rotation.FromEulerAnglesZYX(-(6.2831855f - this->GetRenderFacing()), 6.2831855f - this->m_movementData.m_pitch, 0.0f);

    matrix.a0 = rotation.a0;
    matrix.a1 = rotation.a1;
    matrix.a2 = rotation.a2;
    matrix.a3 = 0.0f;
    matrix.b0 = rotation.b0;
    matrix.b1 = rotation.b1;
    matrix.b2 = rotation.b2;
    matrix.b3 = 0.0f;
    matrix.c0 = rotation.c0;
    matrix.c1 = rotation.c1;
    matrix.c2 = rotation.c2;
    matrix.c3 = 0.0f;
    matrix.d0 = 0.0f;
    matrix.d1 = 0.0f;
    matrix.d2 = 0.0f;
    matrix.d3 = 1.0f;
    matrix.Scale(this->GetTrueScale());

    C3Vector position = this->GetPosition();
    matrix.d0 = position.x;
    matrix.d1 = position.y;
    matrix.d2 = position.z;
}

// VERIFY: 54b43bec manual
// OFFSET: 0x0071B6B0
int32_t CGUnit_C::IsLooting() const {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        const WOWGUID& target = static_cast<const CGPlayer_C*>(this)->m_lootTarget;
        return static_cast<bool>(target);
    }

    return (this->m_unit->m_unitFieldFlags >> 10) & 1;
}

// VERIFY: 51b8f049 auto
// OFFSET: 0x0071BFF0
void CGUnit_C::ClearPrecastMissileModel(uint8_t castCount) {
    if (castCount && castCount != this->m_precastCastCount) {
        return;
    }

    CM2Model* model = this->m_precastMissileModel;

    if (model) {
        if (model->m_attachParent) {
            model->DetachFromParent();
        }

        this->m_precastMissileModel->Release();
        this->m_precastMissileModel = nullptr;
        this->m_precastCastCount = 0;
    }

    this->m_clientFlags &= ~0x1000000;
}

// VERIFY: 991ba226 manual
// OFFSET: 0x0071C4D0
uint64_t CGUnit_C::GetCameraRelativeTo() const {
    if (this->m_vehicleCamera && this->m_vehicleCamera->m_state) {
        return this->m_vehicleCamera->GetRelativeTo();
    }

    return this->GetTransportGUID();
}

// VERIFY: 2b2bc1ef manual
// OFFSET: 0x0071CA10
int32_t HoverHeightChangeHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->m_movementData.m_hoverHeight = unit->m_unit->m_unitFieldHoverheight;
    }

    return 1;
}

// VERIFY: 79327c77 manual
// OFFSET: 0x0071DB20
int32_t IsBirthAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec) {
        int32_t behavior = rec->m_behaviorID;

        if (behavior == 127 || (behavior > 200 && behavior <= 202)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 923e44ce auto
// OFFSET: 0x0071DB70
int32_t IsSubmergedDeathAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec) {
        int32_t behavior = rec->m_behaviorID;

        if (behavior >= 466 && (behavior <= 468 || behavior == 472)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 7173da99 manual
// OFFSET: 0x0071DBC0
int32_t IsCorpseAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (!rec) {
        return 0;
    }

    int32_t behavior = rec->m_behaviorID;

    if (behavior > 468) {
        return behavior == 472;
    }

    if (behavior >= 467) {
        return 1;
    }

    return behavior == 6 || behavior == 132;
}

// VERIFY: 1b5d614f auto
// OFFSET: 0x0071DC20
int32_t IsAirborneAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec) {
        int32_t behavior = rec->m_behaviorID;

        if (behavior >= 37 && (behavior <= 40 || behavior == 467)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 2a3c94c3 manual
// OFFSET: 0x0071DD80
int32_t IsDyingAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (!rec) {
        return 0;
    }

    int32_t behavior = rec->m_behaviorID;

    if (behavior > 131) {
        if (behavior < 466 || behavior > 467) {
            return 0;
        }
    } else if (behavior != 131 && behavior != 1) {
        return 0;
    }

    return 1;
}

// VERIFY: 03ed6220 manual
// OFFSET: 0x0071DDE0
int32_t IsDeadOrDyingAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (!rec) {
        return 0;
    }

    int32_t behavior = rec->m_behaviorID;

    if (behavior > 132) {
        if (behavior < 466 || (behavior > 468 && behavior != 472)) {
            return 0;
        }
    } else if (behavior < 131 && behavior != 1 && behavior != 6) {
        return 0;
    }

    return 1;
}

// VERIFY: e05af72f auto
// OFFSET: 0x007176B0
int32_t GetAnimBehaviorId(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec) {
        return rec->m_behaviorID;
    }

    return 506;
}

// VERIFY: 211bd0bd auto
// OFFSET: 0x00734F70
void CGUnit_C::GetDefenseSkillRank(int32_t& rank, int32_t& bonus) const {
    rank = 5 * this->m_unit->m_unitFieldLevel;
    bonus = 0;
}

// VERIFY: fdf5655e auto
// OFFSET: 0x00734FA0
void CGUnit_C::GetAttackSkillRank(int32_t hand, int32_t& rank, int32_t& bonus) const {
    rank = 5 * this->m_unit->m_unitFieldLevel;
    bonus = 0;
}

// VERIFY: 1d778d00 manual
// OFFSET: 0x00737390
void CGUnit_C::SetMaxAlpha(float alpha) {
    CGUnit_C* transport;

    if (this->m_vehiclePassenger && this->m_vehiclePassenger->m_state == 3 && (transport = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ this->GetTransportGUID() }, TYPE_UNIT, __FILE__, __LINE__)))) {
        CM2Model* model = this->GetObjectModel();

        if (model && model->m_attachParent == transport->GetObjectModel()) {
            this->m_alphaScale = static_cast<uint8_t>(CMath::fint_(255.0f));
        } else {
            this->m_alphaScale = static_cast<uint8_t>(CMath::fint_(alpha * 255.0f));
        }
    } else {
        this->m_alphaScale = static_cast<uint8_t>(CMath::fint_(alpha * 255.0f));
    }

    CVehicle_C* vehicle = this->m_vehicle;

    if (vehicle && vehicle->m_vehicleRec) {
        for (CPassenger* passenger = vehicle->m_passengers.Head(); passenger; passenger = vehicle->m_passengers.Next(passenger)) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPE_UNIT, __FILE__, __LINE__));

            if (unit) {
                unit->SetMaxAlpha(alpha);
            }
        }
    }
}

// VERIFY: c0baa56a auto
// OFFSET: 0x0074B840
int32_t CGUnit_C::VehiclePassengerIsTransitionAllowed(const VehicleSeatRec* seat, int32_t checkSeat) const {
    return (this->m_clientFlags & 0x20000000) || (checkSeat && seat && (seat->m_flagsB & 0x1000000));
}

// VERIFY: 71f105c7 manual
// OFFSET: 0x00716650
int32_t CGUnit_C::ShouldFadeIn() const {
    CGUnitData* unit = this->m_unit;

    if ((unit->m_unitFieldFlags2 & 0x20) || (unit->m_unitFieldFlags & 0x2) || (unit->m_unitFieldBytes1[2] & 0x2)) {
        return 0;
    }

    if (!this->HasVehicleTransport()) {
        return 1;
    }

    CGUnit_C* transport = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ this->GetTransportGUID() }, TYPE_UNIT, __FILE__, __LINE__));

    if (!transport) {
        return 0;
    }

    VehicleSeatRec* seat = transport->GetVehicleSeatRec(static_cast<uint8_t>(this->m_movementData.m_transportSeat));
    return (seat && seat->m_attachmentID < 0) || transport->m_fadeDuration == 0;
}

// VERIFY: 785dd0e1 manual
// OFFSET: 0x00716FA0
int32_t CGUnit_C::IsRunning() const {
    return this->m_movementData.m_walkSpeed + this->m_movementData.m_walkSpeed >= this->m_movementData.GetBaseSpeed(0);
}

// VERIFY: 2be9606e manual
// OFFSET: 0x00717050
int32_t CGUnit_C::ChooseMovementAnim(uint32_t flags, uint32_t* anim) const {
    uint32_t moveFlags = this->m_movementData.m_flags;

    if (!(moveFlags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT))) {
        return (flags & 0xFFFFFFF8) == 0;
    }

    uint32_t state = this->m_animationState;

    if (!(state & 0x70) || !(flags & 0x4) || (state & 0x800000) || (state & 0x2000000)) {
        return 1;
    }

    if (moveFlags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) {
        if (moveFlags & (MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
            *anim = ANIM_SWIM_RIGHT - ((moveFlags & MOVEMENTFLAG_STRAFE_LEFT) != 0);
            return 1;
        }

        *anim = (moveFlags & MOVEMENTFLAG_BACKWARD) ? ANIM_SWIM_BACKWARDS : ANIM_SWIM;
        return 1;
    }

    if (this->IsOnFlyingSpline()) {
        *anim = ANIM_FLY;
        return 1;
    }

    if (moveFlags & MOVEMENTFLAG_BACKWARD) {
        *anim = ANIM_WALKBACKWARDS;
        return 1;
    }

    if (this->m_unit->m_unitFieldBytes1[2] & 0x2) {
        *anim = ANIM_STEALTH_WALK;
        return 1;
    }

    float speed = this->m_movementData.GetBaseSpeed(0);

    if (speed >= 11.0f) {
        *anim = ANIM_SPRINT;
        return 1;
    }

    if (this->m_movementData.m_walkSpeed + this->m_movementData.m_walkSpeed < speed) {
        *anim = ANIM_RUN;
        return 1;
    }

    *anim = ANIM_WALK;
    return 1;
}

// VERIFY: 2b271f9b manual
// OFFSET: 0x00717AD0
float CGUnit_C::GetStandHeight() const {
    CreatureModelDataRec* modelData = this->m_modelData;

    if (!modelData) {
        modelData = this->GetModelData();

        if (!modelData) {
            return 0.0f;
        }
    }

    float height = modelData->m_geoBoxMaxZ - modelData->m_geoBoxMinZ;
    return this->GetTrueScale() * height;
}

// VERIFY: 566bccef manual
// OFFSET: 0x00717B20
int32_t CGUnit_C::GetModelFileName(const char** name) const {
    CreatureModelDataRec* modelData = this->GetModelData();

    if (modelData) {
        *name = modelData->m_modelName;
        return *name != nullptr;
    }

    *name = "Spells\\ErrorCube.mdx";
    return 1;
}

// VERIFY: b60fda18 manual
// OFFSET: 0x00717FC0
C3Vector CGUnit_C::BuildSmoothPosition() const {
    C3Vector position = this->GetRawPosition();
    CGObject_C* transport = ClntObjMgrObjectPtr(WOWGUID{ this->GetTransportGUID() }, TYPE_OBJECT, __FILE__, __LINE__);

    if (transport) {
        C44Matrix transportMatrix;

        if (transport->m_obj->m_type & TYPE_UNIT) {
            static_cast<CGUnit_C*>(transport)->BuildSmoothMatrix(transportMatrix);
        } else {
            transportMatrix = transport->GetMatrix();
        }

        position *= transportMatrix;
    }

    return position;
}

// VERIFY: 24e3b6ca manual
// OFFSET: 0x0071A720
C3Vector CGUnit_C::GetMissileFirePosition() const {
    CM2Model* model = this->m_worldModel;

    if (model && model->IsLoaded(0, 0)) {
        if (model->HasEvent(0x4C534324)) {
            return model->GetEventPosition(0x4C534324);
        }

        if (model->HasEvent(0x52534324)) {
            return model->GetEventPosition(0x52534324);
        }

        if (model->HasEvent(0x54534324)) {
            return model->GetEventPosition(0x54534324);
        }

        if (this->m_vehiclePassenger) {
            return const_cast<CGUnit_C*>(this)->GetModelPosition();
        }
    }

    return this->GetPosition();
}

// VERIFY: 460736e3 auto
// OFFSET: 0x0071A940
void CGUnit_C::StartProceduralFade(uint8_t alpha, uint32_t type, uint32_t time, uint32_t hold, uint32_t endAlpha) {
    if (type < 12) {
        this->m_proceduralFadeAlpha = alpha;
        this->m_proceduralFadeType = type;
        this->m_proceduralFadeStart = FrameTime::s_curTimeMs;
        this->m_proceduralFadeDuration = time + hold;
        this->m_proceduralFadeEndAlpha = endAlpha;
        this->DoFade(alpha, time);
    }
}

// VERIFY: 98c8bae1 auto
// OFFSET: 0x0071B000
bool CGUnit_C::AnimSuppressesMovement() const {
    CM2Model* model = this->m_worldModel;

    if (!model || !model->IsLoaded(0, 0)) {
        return false;
    }

    uint32_t anim;

    if (this->m_torsoKeyBone == -1 || (anim = this->m_worldModel->GetBoneSequenceId(this->m_torsoKeyBone)) == 0xFFFFFFFF) {
        anim = this->m_worldModel->GetBoneSequenceId(0xFFFFFFFF);
    }

    if (anim > 505) {
        return false;
    }

    return (g_animationDataDB.GetRecord(this->GetCurrentTorsoAnimId())->m_bodyflags >> 7) & 1;
}

// VERIFY: 75c2d34b manual
// OFFSET: 0x0071CB30
int32_t Packet_SMSG_MOUNTSPECIAL_ANIM(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit && !unit->IsDeepInLiquid() && guid != ClntObjMgrGetActivePlayer()) {
        unit->OnSpecialMountAnim();
    }

    return 1;
}

// VERIFY: 486abade manual
// OFFSET: 0x0071DFF0
int32_t CGUnit_C::ChooseSubmergeAnim(uint32_t flags, uint32_t* anim) const {
    uint32_t state = this->m_animationState;

    if (state & 0x400008) {
        return 1;
    }

    if (!(state & 0x40) || this->m_mountModel) {
        return (flags & 0xFFFFFFFC) == 0;
    }

    uint32_t submerge;

    if (this->GetClientStandState() == UNIT_SUBMERGED) {
        submerge = (this->m_animStandState == UNIT_SUBMERGED) + ANIM_SUBMERGE;
    } else {
        if (this->m_animStandState != UNIT_SUBMERGED) {
            return (flags & 0xFFFFFFFC) == 0;
        }

        submerge = ANIM_EMERGE;

        if (!this->m_worldModel->HasSequence(ANIM_EMERGE)) {
            submerge = ANIM_BIRTH;
        }
    }

    if (!this->m_worldModel->HasSequence(submerge)) {
        return (flags & 0xFFFFFFFC) == 0;
    }

    if (this->GetCurrentTorsoAnimId() != submerge && (flags & 0x2)) {
        *anim = submerge;
    }

    return 1;
}

// VERIFY: 1313ac12 manual
// OFFSET: 0x0071ED80
CAaBox CGUnit_C::GetStandBox() const {
    CAaBox box;
    box.b.x = 0.0f;
    box.b.y = 0.0f;
    box.b.z = 0.0f;
    box.t.x = 0.0f;
    box.t.y = 0.0f;
    box.t.z = 0.0f;

    CreatureModelDataRec* modelData = this->m_modelData;

    if (modelData || (modelData = this->GetModelData())) {
        box = *reinterpret_cast<const CAaBox*>(&modelData->m_geoBoxMinX);
    }

    if (this->m_unit->m_unitFieldMountdisplayid > 0) {
        CreatureDisplayInfoRec* displayInfo = g_creatureDisplayInfoDB.GetRecord(this->m_mountDisplayId);

        if (displayInfo) {
            CreatureModelDataRec* mountModel = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);

            if (mountModel) {
                float offset = mountModel->m_mountHeight - (box.t.z - box.b.z) * 0.5f;
                box.b.z = box.b.z + offset;
                box.t.z = offset + box.t.z;
                box |= *reinterpret_cast<const CAaBox*>(&mountModel->m_geoBoxMinX);
            }
        }
    }

    return box;
}

// VERIFY: fa1de831 manual
// OFFSET: 0x0071F400
uint32_t CGUnit_C::GetHappinessIndex() const {
    PetPersonalityRec* personality = this->GetPetPersonality();

    if (!personality) {
        return 0;
    }

    uint32_t index = 1;

    while (this->m_unit->m_unitFieldPower[4] >= static_cast<uint32_t>(personality->m_happinessThreshold[index])) {
        if (++index >= 3) {
            break;
        }
    }

    return index - 1;
}

// VERIFY: e993503e auto
// OFFSET: 0x0071F440
const CGUnitVirtualItem* CGUnit_C::GetVirtualItem(VIRTUAL_MONSTER_SLOT slot, bool ignoreDisarm) const {
    if (!this->GetVirtualItemDisplayID(slot)) {
        return nullptr;
    }

    if (ignoreDisarm) {
        return &this->m_virtualItem[slot];
    }

    if (slot == VIRTUAL_MONSTER_SLOT_MAINHAND) {
        if (this->m_unit->m_unitFieldFlags & 0x200000) {
            const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, true);

            if (item) {
                if (item->m_classID != 2) {
                    return &this->m_virtualItem[VIRTUAL_MONSTER_SLOT_MAINHAND];
                }

                return nullptr;
            }
        }
    } else if (slot == VIRTUAL_MONSTER_SLOT_OFFHAND) {
        if (this->m_unit->m_unitFieldFlags & 0x200000) {
            this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, true);

            if (!this->IsDisarmed(0)) {
                const CGUnitVirtualItem* offhand = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, true);

                if (offhand && offhand->m_classID == 2) {
                    return nullptr;
                }
            }
        }

        if (this->m_unit->m_unitFieldFlags2 & 0x80) {
            return nullptr;
        }
    } else if (slot == VIRTUAL_MONSTER_SLOT_RANGED) {
        if (this->m_unit->m_unitFieldFlags2 & 0x400) {
            return nullptr;
        }
    }

    return &this->m_virtualItem[slot];
}

// VERIFY: 07176a35 auto
// OFFSET: 0x0071FEF0
C3Vector CGUnit_C::GetNamePosition() const {
    if (this->m_worldModel->IsLoaded(0, 0)) {
        if (this->m_mountDisplayId > 0 && !(this->m_clientFlags & 0x10000000) && this->m_worldModel->HasAttachment(29)) {
            return this->m_worldModel->GetAttachmentPosition(29);
        }

        if (this->m_worldModel->HasAttachment(18)) {
            return this->m_worldModel->GetAttachmentPosition(18);
        }
    }

    C3Vector position = this->m_vehiclePassenger ? const_cast<CGUnit_C*>(this)->GetModelPosition() : this->GetPosition();
    position.z = this->m_height * this->m_scale * 1.25f + position.z;
    return position;
}

// VERIFY: e6787c39 auto
// OFFSET: 0x00722180
bool CGUnit_C::DoNotPlayWoundAnim() const {
    if (this->m_vehicle && this->m_vehicle->m_vehicleRec && this->m_vehicle->HasHighPriorityAnim()) {
        return true;
    }

    if (this->m_creatureCacheEntry) {
        return (this->m_creatureCacheEntry->m_typeFlags >> 3) & 1;
    }

    return this->GetCurrentTorsoAnimId() == 121;
}

// VERIFY: 54fb09a7 manual
// OFFSET: 0x007221D0
void CGUnit_C::CreateFishingLineEffect() {
    CGObject_C* bobber = ClntObjMgrObjectPtr(this->m_unit->m_unitFieldChannelObject, TYPE_GAMEOBJECT, __FILE__, __LINE__);

    if (!bobber || !this->m_unit->m_unitChannelSpell || static_cast<CGGameObject_C*>(bobber)->m_gameObject->m_typeID != 17) {
        return;
    }

    for (CM2Model* attached = this->m_worldModel->m_attachList; attached; attached = attached->m_attachNext) {
        if (attached->m_attachmentId == 1) {
            if (!this->m_fishingLineEffect) {
                void* memory = SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0);
                this->m_fishingLineEffect = memory ? new (memory) CEffect() : nullptr;
                WOWGUID target = this->m_unit->m_unitFieldChannelObject;
                this->m_fishingLineEffect->AddFishingLine(this, target);
            }

            return;
        }
    }
}

// VERIFY: ef43f122 manual
// OFFSET: 0x00724200
int32_t CGUnit_C::ChooseFallAnim(uint32_t* anim) const {
    int32_t falling = this->m_movementData.IsFalling();

    if (!falling) {
        return falling;
    }

    uint32_t current = (this->m_worldModel && this->m_worldModel->IsLoaded(0, 0)) ? this->m_worldModel->GetBoneSequenceId(0xFFFFFFFF) : 0xFFFFFFFF;
    *anim = IsAirborneAnim(current) ? ANIM_COUNT : ANIM_FALL;
    return 1;
}

// VERIFY: 1fe5c010 manual
// OFFSET: 0x00724F50
void CGUnit_C::SaveChainSpellTargets(const TSStackArray<WOWGUID>& targets) {
    this->m_chainSpellTargets.m_count = 0;
    uint32_t i = targets.m_count;

    while (i) {
        i--;
        const WOWGUID& target = targets.m_data[i];

        if (target != this->m_obj->m_guid) {
            this->m_chainSpellTargets.New(target);
        }
    }
}

// VERIFY: 5984d3a5 manual
// OFFSET: 0x007282A0
bool CGUnit_C::HasAuraBySpellId(int32_t spellId) {
    uint32_t count = this->GetAuraCount();

    for (uint32_t i = 0; i < count; i++) {
        if (this->m_auras[i].m_spellId == spellId) {
            return true;
        }
    }

    return false;
}

// VERIFY: c93008c2 manual
// OFFSET: 0x00728300
bool CGUnit_C::HasAuraActive(int32_t spellId) {
    uint32_t count = this->GetAuraCount();

    for (uint32_t i = 0; i < count; i++) {
        AuraData* aura = this->GetAura(i);

        if (aura->m_spellId == spellId) {
            for (uint32_t effect = 0; effect < 3; effect++) {
                if ((1 << effect) & aura->m_flags) {
                    return true;
                }
            }
        }
    }

    return false;
}

// VERIFY: b1ef5b63 manual
// OFFSET: 0x007289C0
int32_t CGUnit_C::GetAuraIndex(int32_t spellId) {
    uint32_t count = this->GetAuraCount();

    for (uint32_t i = 0; i < count; i++) {
        if (this->m_auras[i].m_spellId == spellId) {
            return i;
        }
    }

    return -1;
}

// VERIFY: f3ee2cc8 manual
// OFFSET: 0x007226B0
bool CGUnit_C::IsPetBarUsed() const {
    if (this->m_creatureCacheEntry && (this->m_creatureCacheEntry->m_typeFlags & 0x2000)) {
        return false;
    }

    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || player->GetTransportGUID() != this->m_obj->m_guid.m_guid || !this->m_vehicle) {
        return true;
    }

    VehicleSeatRec* seat = this->m_vehicle->GetVehicleSeatRec(player->m_movementData.m_transportSeat);
    return !seat || seat->m_vehicleAbilityDisplay != 0;
}

// VERIFY: 5d3d4eef auto
// OFFSET: 0x006E6F40
float CGUnit_C::GetFacing() const {
    return this->m_movement->GetFacing(this->m_movement->m_facing);
}

// VERIFY: 003a9d67 auto
// OFFSET: 0x007156A0
float CGUnit_C::GetRenderFacing() const {
    return this->m_movementData.GetFacing(this->m_renderFacing);
}

// VERIFY: cd088da4 auto
// OFFSET: 0x00715810
void CGUnit_C::ResetSmoothFacing() {
    this->m_smoothSwimPitch = 0.0f;
    this->m_animStateFlags &= ~0x7u;

    float facing = this->m_movementData.GetFacing(this->m_renderFacing);
    this->m_smoothFacing = facing;
    this->m_smoothFacingTarget = facing;
    this->m_smoothSwimRoll = 0.0f;
}

// VERIFY: b052cacb manual
// OFFSET: 0x00717E50
float CGUnit_C::GetSmoothFacing() const {
    float facing = 0.0f;
    const CGUnit_C* unit = this;

    while (unit) {
        facing += unit->m_smoothTarget.m_value;

        if (!unit->HasVehicleTransport()) {
            break;
        }

        unit = static_cast<const CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ unit->GetTransportGUID() }, TYPE_UNIT, __FILE__, __LINE__));
    }

    return unit->m_movement->GetFacing(facing);
}

// VERIFY: 7a726f02 manual
// OFFSET: 0x0071E340
void CGUnit_C::ChooseDefaultAnim(uint32_t& anim, int32_t force) const {
    CVehiclePassenger_C* passenger = this->m_vehiclePassenger;

    if (passenger && passenger->m_state == 3 && passenger->GetLowerAnim(passenger->m_seatRec) != 506) {
        return;
    }

    if (this->m_animationState & 0x800004) {
        return;
    }

    uint32_t torso = this->GetCurrentTorsoAnimId();

    if (!force && GetAnimBehaviorId(torso) == 464) {
        anim = torso;
        return;
    }

    if (this->m_movementData.m_flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) {
        anim = ANIM_SWIM_IDLE;
        return;
    }

    if (this->m_unit->m_unitFieldBytes1[2] & 0x2) {
        anim = ANIM_STEALTH_STAND;
        return;
    }

    anim = this->m_movement->IsSplineFlyerOrHovering() ? ANIM_HOVER : ANIM_STAND;
}

// VERIFY: 3787d22a auto
// OFFSET: 0x00720330
void CGUnit_C::UpdateSelectionRadius() {
    CAaBox box = this->GetStandBox();
    float dx = box.t.x - box.b.x;
    float dy = box.t.y - box.b.y;

    if (fabs(dx) < 0.00000023841858f && fabs(dy) < 0.00000023841858f) {
        this->m_selectionRadius = 1.2f;
        return;
    }

    this->m_selectionRadius = sqrt(dx * dx + dy * dy) * 0.5f;
    float scaled = this->GetTrueScale() * this->m_selectionRadius;
    this->m_selectionRadius = sqrt(scaled);

    if (scaled > 5.0f) {
        this->m_selectionRadius = this->m_selectionRadius + (scaled - 5.0f) * (scaled - 5.0f) * 0.059999999f;
    }

    if (this->m_selectionRadius > 10.0f) {
        this->m_selectionRadius = 10.0f;
    }
}

// OFFSET: 0x008A1420
bool CGUnit_C::IsTransport() const {
    return true;
}

// VERIFY: 9da62ce8 auto
// OFFSET: 0x0074B810
int32_t CGUnit_C::AddPassenger(CPassenger* passenger, int32_t a2) {
    if (!this->m_vehicle) {
        return 0;
    }

    return this->m_vehicle->AddPassenger(passenger);
}

// VERIFY: 97ef0b7c auto
// OFFSET: 0x0071F540
int32_t CGUnit_C::GetVirtualItemDisplayRec(VIRTUAL_MONSTER_SLOT slot, ItemDisplayInfoRec& rec) const {
    return g_itemDisplayInfoDB.GetRecord(this->m_virtualItemDisplayId[slot], &rec);
}

// VERIFY: 068ad525 manual
// OFFSET: 0x00718C20
UNIT_REACTION CGUnit_C::UnitReaction(const CGCorpse_C* corpse) const {
    FactionTemplateRec* faction = g_factionTemplateDB.GetRecord(this->m_unit->m_unitFieldFactiontemplate);
    FactionTemplateRec* other = g_factionTemplateDB.GetRecord(corpse->GetFactionTemplate());

    if (faction && other) {
        return FactionTemplateReaction(faction, other);
    }

    return UNIT_REACTION_NEUTRAL;
}

// VERIFY: 53133dc2 manual
// OFFSET: 0x00716490
bool CGUnit_C::IsDeadOrGhost() const {
    return this->m_unit->m_unitFieldHealth <= 0;
}

// VERIFY: 32c13bf7 auto
// OFFSET: 0x0071A3F0
CreatureSoundDataRec* CGUnit_C::GetSoundData() const {
    if (this->m_soundDataOverride) {
        return this->m_soundDataOverride;
    }

    CreatureSoundDataRec* soundData = this->m_soundData;

    if (soundData && this->m_unit->m_unitFieldPetnumber && soundData->m_creatureSoundDataIdpet > 0) {
        return g_creatureSoundDataDB.GetRecord(soundData->m_creatureSoundDataIdpet);
    }

    return soundData;
}

// VERIFY: 0a47e594 manual
// OFFSET: 0x00720860
void CGUnit_C::PlayNPCSound(bool greeting) {
    if (this->m_obj->m_type & TYPE_PLAYER) {
        return;
    }

    CGUnit_C::s_lastNPCSoundGUID = WOWGUID();

    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || player->IsDeadOrGhost() || SI2::IsPlaying(&this->m_npcInteractSound)) {
        return;
    }

    if (this->m_unit->m_unitFieldHealth <= 0 || (this->m_unit->m_unitDynamicFlags & 0x20) || !this->m_displayInfo) {
        return;
    }

    NPCSoundsRec* sounds = g_npcSoundsDB.GetRecord(this->m_displayInfo->m_npcsoundID);

    if (!sounds) {
        return;
    }

    C3Vector position = this->GetPosition();
    SI2::PlaySoundKit(greeting ? sounds->m_soundID[NPCSOUND_HELLO] : sounds->m_soundID[NPCSOUND_GOODBYE], &position, &this->m_npcInteractSound, nullptr, 0, nullptr, 1, 0);
}

// VERIFY: a3ea8ac7 manual
// OFFSET: 0x00721DA0
bool CGUnit_C::CanCurrentFormMount() const {
    if (this->IsShapeShifted()) {
        return false;
    }

    if (this->m_unit->m_unitFieldNativedisplayid == this->m_unit->m_unitFieldDisplayid) {
        return true;
    }

    CreatureDisplayInfoRec* displayInfo = g_creatureDisplayInfoDB.GetRecord(this->m_unit->m_unitFieldDisplayid);
    CreatureDisplayInfoExtraRec* extra = displayInfo ? g_creatureDisplayInfoExtraDB.GetRecord(displayInfo->m_extendedDisplayInfoID) : nullptr;

    if (!extra) {
        return false;
    }

    if (displayInfo) {
        CreatureModelDataRec* modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);

        if (modelData && (modelData->m_flags & 0x80)) {
            return true;
        }
    }

    ChrRacesRec* race = g_chrRacesDB.GetRecord(extra->m_displayRaceID);
    return race && (race->m_flags & 0x4);
}

// VERIFY: f5fb6fd8 manual
// OFFSET: 0x0071D1F0
SummonPropertiesRec* CGUnit_C::GetSummonProperties() const {
    SpellRec spell;

    if (g_spellDB.GetRecord(this->m_unit->m_unitCreatedBySpell, &spell)) {
        for (int32_t i = 0; i < 3; i++) {
            if (spell.m_effect[i] == SPELL_EFFECT_SUMMON) {
                return g_summonPropertiesDB.GetRecord(spell.m_effectMiscValueB[i]);
            }
        }
    }

    return nullptr;
}

// VERIFY: cbaf6b39 auto
// OFFSET: 0x00722640
bool CGUnit_C::CanTurnWhileCasting() const {
    if (this->m_castSpellId) {
        SpellRec spell;

        if (g_spellDB.GetRecord(this->m_castSpellId, &spell) && (spell.m_attributesExE & 0x80000)) {
            return false;
        }
    }

    return true;
}

// VERIFY: 5ae50009 manual
// OFFSET: 0x007206A0
void CGUnit_C::UpdateAmmoModel(uint32_t displayId, uint32_t invType) {
    if (this->m_ammoDisplayId == displayId && this->m_ammoModel) {
        return;
    }

    int32_t noAttach = 0;

    if (this->m_ammoModel) {
        if (this->m_ammoModel->m_attachParent) {
            this->m_ammoModel->DetachFromParent();
        }

        this->m_ammoModel->Release();
        this->m_ammoModel = nullptr;
    }

    this->m_ammoDisplayId = displayId;

    if (!displayId) {
        return;
    }

    uint32_t ammoType = 24;
    const CGUnitVirtualItem* ranged = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false);

    if (invType == 30) {
        if (ranged) {
            ammoType = (ranged->m_inventoryType == 25) + 24;
        }
    } else {
        ammoType = invType;
    }

    if (ranged && ranged->m_classID == 2 && (ranged->m_subclassID == 3 || ranged->m_subclassID == 18)) {
        noAttach = 1;
    }

    ItemDisplayInfoRec rec;

    if (g_itemDisplayInfoDB.GetRecord(this->m_ammoDisplayId, &rec)) {
        this->m_ammoModel = CCharacterComponent::MakeAmmoModel(this->m_worldModel, &rec, ammoType);
    }

    if (this->m_ammoModel && !noAttach && (this->m_animationState & 0x4000) && ammoType == 24) {
        this->m_ammoModel->AttachToParent(this->m_worldModel, 0x23, nullptr, 0);
    }
}

// VERIFY: 2abd4122 auto
// OFFSET: 0x006E6EF0
C3Vector CGUnit_C::GetPosition() const {
    return this->m_movement->GetPosition(this->m_movement->m_position);
}

// VERIFY: 66196daa manual
// OFFSET: 0x00718860
void CGUnit_C::OnFlightLocal(uint32_t eventTime, int32_t flying) {
    this->m_movementData.OnFlightLocal(eventTime, flying);

    if (flying) {
        CGUnit_C::s_flightStatusTime = eventTime;
    }
}

// VERIFY: a7d1e04b auto
// OFFSET: 0x0071AE50
void CGUnit_C::ToggleRunModeLocal(uint32_t eventTime) {
    this->m_movementData.OnSetRunModeLocal(eventTime, this->m_movementData.m_flags & MOVEMENTFLAG_WALKING);
}

// VERIFY: cf000702 manual
// OFFSET: 0x0071F890
bool CGUnit_C::CanAssist(const CGCorpse_C* corpse) const {
    return this->UnitReaction(corpse) >= UNIT_REACTION_FRIENDLY;
}

// VERIFY: 2fefd292 manual
// OFFSET: 0x00727E70
void CGUnit_C::UpdateAuraEffectMask() {
    memset(this->m_auraEffectMask, 0, sizeof(this->m_auraEffectMask));

    SpellRec spell;

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        AuraData* aura = this->GetAura(i);

        if (!g_spellDB.GetRecord(aura->m_spellId, &spell)) {
            continue;
        }

        for (int32_t effect = 0; effect < 3; effect++) {
            if (aura->m_flags & (1 << effect)) {
                this->m_auraEffectMask[spell.m_effectAura[effect] / 8] |= 1 << (spell.m_effectAura[effect] % 8);
            }
        }
    }
}

// VERIFY: 49281665 manual
// OFFSET: 0x00716E50
void CGUnit_C::ApplyGuildTabard(int32_t guildId, const WOWGUID& guid, void* param, bool apply) {
    if (!guildId || !apply) {
        return;
    }

    WOWGUID cacheGuid = WOWGUID();

    if (!g_guildInfoCache.GetRecord(guildId, cacheGuid, nullptr, nullptr, false)) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    int32_t emblemStyle;
    int32_t emblemColor;
    int32_t borderStyle;
    int32_t borderColor;
    int32_t background;

    if (GuildGetGuildTabard(guid, guildId, nullptr, nullptr, emblemStyle, emblemColor, borderStyle, borderColor, background) && unit->m_characterComponent) {
        unit->m_characterComponent->ApplyGuildColor(emblemStyle, emblemColor, borderStyle, borderColor, background);
    }
}

// VERIFY: 97aaae3a manual
// OFFSET: 0x0071E5B0
void CGUnit_C::UpdateObjectEffectAnimationStates() {
    if (!this->m_effectManager) {
        return;
    }

    M2BoneSequenceInfo sequences[3];
    this->GetCurrentAnimationInfo(&sequences[0], &sequences[1], &sequences[2], 1);

    if (this->m_mountModel) {
        if (!this->m_mountModel->IsLoaded(0, 0)) {
            return;
        }

        if (sequences[0].m_sequenceId < 506) {
            sequences[0].m_sequenceId = this->m_mountModel->ResolveSequenceFallback(sequences[0].m_sequenceId);
        }
    }

    if (this->m_worldModel) {
        if (!this->m_worldModel->IsLoaded(0, 0)) {
            return;
        }

        if (sequences[1].m_sequenceId < 506) {
            sequences[1].m_sequenceId = this->m_worldModel->ResolveSequenceFallback(sequences[1].m_sequenceId);
        }

        if (sequences[2].m_sequenceId < 506) {
            sequences[2].m_sequenceId = this->m_worldModel->ResolveSequenceFallback(sequences[2].m_sequenceId);
        }
    }

    struct {
        uint32_t m_state;
        int32_t m_slot;
    } states[3];

    int32_t count = 0;

    for (int32_t slot = 0; slot < 3; slot++) {
        if (sequences[slot].m_sequenceId != 0xFFFFFFFF) {
            states[count].m_state = FindObjectEffectStateFromAnimID(sequences[slot].m_sequenceId, sequences[slot].m_variationIndex);
            states[count].m_slot = slot;
            count++;
        }
    }

    uint32_t previous[3];
    previous[0] = this->m_animEffectStates[0];
    previous[1] = this->m_animEffectStates[1];
    previous[2] = this->m_animEffectStates[2];

    for (int32_t i = 0; i < 3; i++) {
        if (this->m_animEffectStates[i]) {
            int32_t j;

            for (j = 0; j < count; j++) {
                if (this->m_animEffectStates[i] == states[j].m_state) {
                    break;
                }
            }

            if (j == count) {
                this->m_animEffectStates[i] = 0;
            }
        }
    }

    for (int32_t i = 0; i < count; i++) {
        uint32_t state = states[i].m_state;
        int32_t j;

        for (j = 0; j < 3; j++) {
            if (this->m_animEffectStates[j] == state) {
                break;
            }
        }

        if (j != 3) {
            continue;
        }

        if (state) {
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
        }

        this->m_animEffectStates[states[i].m_slot] = state;
    }

    for (int32_t i = 0; i < 3; i++) {
        if (previous[i]) {
            int32_t j;

            for (j = 0; j < count; j++) {
                if (previous[i] == states[j].m_state) {
                    break;
                }
            }

            if (j == count) {
                this->m_effectManager->ExitState(previous[i], 1);
            }
        }
    }
}

// VERIFY: 91123c63 manual
// OFFSET: 0x005A1120
bool CGUnit_C::HasAuraEffect(int32_t auraType) const {
    return (this->m_auraEffectMask[auraType / 8] & (1 << (auraType % 8))) != 0;
}

// VERIFY: ef1871f1 auto
// OFFSET: 0x00715D00
int32_t __stdcall CGUnit_C::ResolveSheathState(int32_t hand, const CGUnitVirtualItem* mainHand, const CGUnitVirtualItem* offHand) {
    if (hand == WEAPONMODE_NORMALMODE) {
        if (!mainHand && (!offHand || offHand->m_inventoryType == 23)) {
            return WEAPONMODE_SHEATHEDMODE;
        }
    } else if (hand == WEAPONMODE_SHEATHEDMODE) {
        if ((offHand && offHand->m_inventoryType == 14) || (mainHand && mainHand->m_classID == 2) || (offHand && offHand->m_classID == 2)) {
            return WEAPONMODE_NORMALMODE;
        }
    }

    return hand;
}

// VERIFY: 4b3a0738 auto
// OFFSET: 0x00716040
void CGUnit_C::RequestRepopOnLanding() {
    this->m_clientFlags |= 0x100000;
}

// VERIFY: 99f1723a manual
// OFFSET: 0x00717870
C3Spline_CatmullRom* CGUnit_C::GetSpline() {
    CMoveSpline* spline = this->m_movementData.m_spline;
    return spline ? &spline->m_spline : nullptr;
}

// VERIFY: 1e31eb25 auto
// OFFSET: 0x00717890
float CGUnit_C::GetSplineElapsedTime() {
    CMoveSpline* spline = this->m_movementData.m_spline;
    return spline ? static_cast<float>(static_cast<int32_t>(spline->m_start)) : 0.0f;
}

// VERIFY: 80a0af13 manual
// OFFSET: 0x007178B0
float CGUnit_C::GetSplineDuration() {
    CMoveSpline* spline = this->m_movementData.m_spline;
    return spline ? static_cast<float>(spline->m_duration) : 0.0f;
}

// VERIFY: ace397fc auto
// OFFSET: 0x007199E0
bool __stdcall CGUnit_C::IsAreaInAreaGroup(int32_t areaId, int32_t areaGroupId, int32_t* checked) {
    if (checked) {
        *checked = 0;
    }

    AreaTableRec* area = g_areaTableDB.GetRecord(areaId);

    if (!area) {
        return false;
    }

    while (areaGroupId) {
        AreaGroupRec* group = g_areaGroupDB.GetRecord(areaGroupId);

        if (!group) {
            return false;
        }

        for (uint32_t i = 0; i < 6; i++) {
            if (!group->m_areaID[i]) {
                break;
            }

            if (checked) {
                (*checked)++;
            }

            if (area->m_id == group->m_areaID[i] || area->m_parentAreaID == group->m_areaID[i]) {
                return true;
            }
        }

        areaGroupId = group->m_nextAreaID;
    }

    return false;
}

// VERIFY: b35207d3 manual
// OFFSET: 0x0071A7F0
int32_t CGUnit_C::HasAttachmentForEffect(uint32_t id, int32_t raw) const {
    CM2Model* model = this->m_worldModel;

    if (!model || !model->IsLoaded(0, 0)) {
        return 0;
    }

    if (raw) {
        return model->HasAttachment(id);
    }

    if (id == 13) {
        return model->HasAttachment(s_effectAttachmentIds[4]);
    }

    return model->HasAttachment(s_effectAttachmentIds[id]);
}

// VERIFY: 88f55858 manual
// OFFSET: 0x0071A860
C3Vector CGUnit_C::GetAttachmentPositionForEffect(uint32_t id, const C3Vector& offset, int32_t raw) {
    if (raw) {
        return this->GetAttachmentPosition(id, &offset);
    }

    if (id != 13) {
        return this->GetAttachmentPosition(s_effectAttachmentIds[id], &offset);
    }

    for (CM2Model* model = this->m_worldModel->m_attachList; model; model = model->m_attachNext) {
        if (model->m_attachmentId == 1) {
            for (int32_t attach = 4; attach >= 0; attach--) {
                if (model->HasAttachment(attach)) {
                    return offset * model->GetAttachmentWorldTransform(attach);
                }
            }

            break;
        }
    }

    return this->GetAttachmentPosition(s_effectAttachmentIds[4], &offset);
}

// VERIFY: cdf2a9f2 manual
// OFFSET: 0x0071A9A0
bool CGUnit_C::GetFlashColor(uint32_t time, CImVector& color) {
    if (!this->m_flashStartTime) {
        return false;
    }

    uint32_t start = this->m_flashStartTime + this->m_flashDelay;
    uint32_t elapsed = time - start;

    if (static_cast<int32_t>(elapsed) < 0) {
        color = this->m_flashColor;
        return true;
    }

    uint32_t duration = this->m_flashDuration;

    if (static_cast<int32_t>(time - duration - start) >= 0) {
        this->m_flashStartTime = 0;
        return false;
    }

    int32_t t = static_cast<int32_t>((1.0f - static_cast<float>(elapsed) / static_cast<float>(duration)) * 255.0f);
    CImVector flash = this->m_flashColor;

    if (t) {
        color.InterpColor(t, &flash);
    }

    return true;
}

// VERIFY: 7a167157 auto
// OFFSET: 0x0071AE80
int32_t CGUnit_C::IsFacingUnchangedSinceSent() const {
    if (fabs(this->GetRawFacing() - this->m_lastSentFacing) < MOUSE_LOOK_SEND_FACING_DELTA) {
        if (!(this->m_movement->m_flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) && !(this->m_movement->m_flags2 & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING)) {
            return 1;
        }

        if (fabs(this->GetPitch() - this->m_lastSentPitch) < MOUSE_LOOK_SEND_FACING_DELTA) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 3cad216e auto
// OFFSET: 0x0071B580
float __fastcall CGUnit_C::ComputeTrackingElevationAngle(const C3Vector& position, const C3Vector& target) {
    float dx = position.x - target.x;
    float dy = position.y - target.y;
    float dz = position.z - target.z;
    float scale = 1.0f / sqrt(dz * dz + dy * dy + dx * dx);
    dx *= scale;
    dy *= scale;
    dz *= scale;

    if (fabs(dx) < 0.001f && fabs(dy) < 0.001f) {
        return target.z >= position.z ? -1.5707964f : 1.5707964f;
    }

    return asin(dz);
}

// VERIFY: cb7d9a59 manual
// OFFSET: 0x007222A0
bool CGUnit_C::ShouldKneelForLoot() const {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return !(this->m_unit->m_unitFieldFlags & 0x10000000);
    }

    const WOWGUID& lootTarget = static_cast<const CGPlayer_C*>(this)->m_lootTarget;
    CGObject_C* target = ClntObjMgrObjectPtr(lootTarget, TYPE_OBJECT, __FILE__, __LINE__);

    if (!target) {
        return false;
    }

    if ((target->m_obj->m_type & TYPE_GAMEOBJECT) && static_cast<CGGameObject_C*>(target)->m_gameObject->m_typeID == 17) {
        return false;
    }

    if ((target->m_obj->m_type & TYPE_UNIT) && static_cast<CGUnit_C*>(target)->m_unit->m_unitFieldHealth > 0) {
        return false;
    }

    return !(target->m_obj->m_type & TYPE_ITEM);
}

// VERIFY: 1fe5076b manual
// OFFSET: 0x0071A260
void CGUnit_C::SetEmoteQueue(const QUESTGIVEREMOTENODE* nodes, uint32_t count) {
    this->m_emoteQueue[0].m_emote = -1;

    if (!count) {
        return;
    }

    uint32_t queued = 0;

    for (uint32_t i = 0; i < count; i++) {
        if (nodes[i].m_emote) {
            EmotesRec* emote = g_emotesDB.GetRecord(nodes[i].m_emote);

            if (emote && emote->m_emoteSpecProc == EMOTESPECPROC_NONE) {
                this->m_emoteQueue[queued].m_delay = nodes[i].m_delay;
                this->m_emoteQueue[queued].m_emote = nodes[i].m_emote;
                queued++;
            }
        }

        EmotesRec* current = g_emotesDB.GetRecord(this->m_emoteQueue[i].m_emote);

        if (current && current->m_animID) {
            this->m_animationState &= ~0x8000u;
            this->m_previousSheatheState = this->m_sheatheState;
        }
    }

    if (queued < 4) {
        this->m_emoteQueue[queued].m_emote = 0;
    }

    this->m_emoteQueue[0].m_delay += OsGetAsyncTimeMs();
}

// VERIFY: 088aa6dd auto
// OFFSET: 0x00728050
DelayedSpellVisualKit* CGUnit_C::DelaySpellVisualKit(int32_t spellId, int32_t kitId, int32_t kitType, const C3Vector* position, WOWGUID targetGuid, int32_t a7, int32_t a8, int32_t delay, int32_t a10, int32_t a11) {
    uint32_t index;

    for (index = 0; index < this->m_delayedSpellVisualKits.m_count; index++) {
        if (!(this->m_delayedSpellVisualKits.m_data[index].m_flags & 0x8)) {
            break;
        }
    }

    if (index == this->m_delayedSpellVisualKits.m_count) {
        this->m_delayedSpellVisualKits.SetCount(this->m_delayedSpellVisualKits.m_count + 1);
    }

    DelayedSpellVisualKit* kit = &this->m_delayedSpellVisualKits.m_data[index];
    kit->m_spellId = spellId;
    kit->m_kitType = kitType;
    kit->m_kitId = kitId;

    if (position) {
        kit->m_position = *position;
        kit->m_flags |= 0x1;
        kit->m_targetGuid = targetGuid;
    } else {
        kit->m_position.x = 0.0f;
        kit->m_position.y = 0.0f;
        kit->m_position.z = 0.0f;
        kit->m_flags &= ~0x1u;
        kit->m_targetGuid = WOWGUID();
    }

    kit->m_delay = delay;
    kit->m_flags = (kit->m_flags & ~0x6u) | (((a7 != 0) | (((a8 != 0) | 2) << 1)) << 1);
    kit->unk_0028 = a10;
    kit->m_startTime = a11;
    return kit;
}

// VERIFY: 6d9103d1 auto
// OFFSET: 0x00716470
C3Vector CGUnit_C::GetGroundNormal() const {
    return this->m_movement->GetGroundNormal();
}

// VERIFY: 2f69446a manual
// OFFSET: 0x0071BC50
bool CGUnit_C::IsMovingToward(CGObject_C* target) const {
    C3Vector position = this->GetPosition();
    C3Vector targetPosition = target->GetPosition();
    C3Vector delta(targetPosition.x - position.x, targetPosition.y - position.y, targetPosition.z - position.z);
    C3Vector direction = this->m_movement->GetDirection();
    return direction.x * delta.x + direction.y * delta.y + direction.z * delta.z >= 0.0f;
}

// VERIFY: fbe09df2 auto
// OFFSET: 0x00724280
int32_t CGUnit_C::ChooseLootAnim(uint32_t flags, uint32_t* animId) {
    if (this->m_mountModel || !(this->m_animationState & 0x40) || !this->ShouldKneelForLoot()) {
        return (flags & 0xFFFFFFF0) == 0;
    }

    uint32_t torso = this->GetCurrentTorsoAnimId();

    if (!this->IsLooting()) {
        if (torso != 188) {
            return (flags & 0xFFFFFFF0) == 0;
        }

        if (flags & 0x8) {
            *animId = 189;
        }

        return 1;
    }

    if (!(flags & 0x8) || torso == 188 || torso == 50) {
        return 1;
    }

    *animId = 50;
    return 1;
}

// VERIFY: ecaae3a5 auto
// OFFSET: 0x0071BCE0
int32_t CGUnit_C::IsInFrontOfTargetWithGuid(const WOWGUID& guid) const {
    CGObject_C* target = ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__);
    return target && this->IsMovingToward(target);
}

// VERIFY: c4de07e2 manual
// OFFSET: 0x007197D0
void CGUnit_C::UpdateWorldTransform(float elapsed) {
    C3Vector normal = this->m_movement->GetGroundNormal();

    if (normal.z >= 0.35721236f) {
        C3Vector delta(this->m_groundNormal.x - normal.x, this->m_groundNormal.y - normal.y, this->m_groundNormal.z - normal.z);
        float decay = pow(0.0018f, elapsed);
        this->m_groundNormal.x = delta.x * decay + normal.x;
        this->m_groundNormal.y = delta.y * decay + normal.y;
        this->m_groundNormal.z = delta.z * decay + normal.z;
    }

    CM2Model* model = this->GetObjectModel();

    if (model) {
        float scale = this->GetTrueScale();
        float facing = this->GetRenderFacing();
        model->SetWorldTransform(this->GetPosition(), facing, scale, this->m_groundNormal, 0xFFFFFFFF);
    }
}

// VERIFY: 315f8bea manual
// OFFSET: 0x00735CC0
void CGUnit_C::SetKeyBoneSequenceSpeed(CM2Model* model, uint32_t boneId, float speed, int32_t force) {
    if (!model) {
        return;
    }

    if (!force && this->m_vehiclePassenger && this->m_vehiclePassenger->OverridesModelAnimation()) {
        return;
    }

    model->SetBoneSequenceSpeed(boneId, speed);

    CVehicle_C* vehicle = this->m_vehicle;

    if (!vehicle || !vehicle->m_vehicleRec || !vehicle->IsMirroringAnimsToPassengers() || model != this->GetObjectModel()) {
        return;
    }

    for (CPassenger* passenger = vehicle->m_passengers.Head(); passenger; passenger = vehicle->m_passengers.Next(passenger)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit && unit->m_vehiclePassenger && unit->m_vehiclePassenger->OverridesModelAnimation()) {
            unit->SetKeyBoneSequenceSpeed(unit->GetObjectModel(), boneId, speed, 1);
        }
    }
}

// VERIFY: 4cfce839 auto
// OFFSET: 0x007178E0
void CGUnit_C::ReleaseEffectsBySpell(int32_t spellId) {
    CEffect* effect = this->m_procEffects;

    while (effect) {
        CEffect* next = effect->m_next;

        if (effect->m_spellId == spellId) {
            effect->Release();
        }

        effect = next;
    }
}

// VERIFY: e1b2b150 manual
// OFFSET: 0x00735820
void CGUnit_C::SetKeyBoneSequence(CM2Model* model, uint32_t boneId, uint32_t sequenceId, uint32_t variation, uint32_t time, float speed, int32_t blend, int32_t primary, int32_t force) {
    if (!model) {
        return;
    }

    if (!force && this->m_vehiclePassenger && this->m_vehiclePassenger->OverridesModelAnimation()) {
        return;
    }

    if (this->m_animationState & 0x8000000) {
        blend = 0;
    }

    model->SetBoneSequence(boneId, sequenceId, variation, time, speed, blend, primary);

    M2BoneSequenceInfo info;

    if (model->IsLoaded(0, 0) && model->HasBone(boneId) && (model->GetBoneSequenceInfo(boneId, info), info.m_sequenceId == sequenceId) && time && fabs(speed) > 0.001f && info.m_finished) {
        if (boneId == 0xFFFFFFFF || boneId == 26) {
            model->SetBoneSequence(boneId, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
        } else {
            model->UnsetBoneSequence(boneId, blend, primary);
        }

        return;
    }

    CVehicle_C* vehicle = this->m_vehicle;

    if (!vehicle || !vehicle->m_vehicleRec || !vehicle->IsMirroringAnimsToPassengers() || model != this->GetObjectModel()) {
        return;
    }

    uint32_t mirrorVariation;

    if (model->IsLoaded(0, 0) && model->HasBone(boneId)) {
        model->GetBoneSequenceInfo(boneId, info);
        mirrorVariation = info.m_variationIndex;
    } else {
        mirrorVariation = 0;
    }

    for (CPassenger* passenger = this->m_vehicle->m_passengers.Head(); passenger; passenger = this->m_vehicle->m_passengers.Next(passenger)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit && unit->m_vehiclePassenger && unit->m_vehiclePassenger->OverridesModelAnimation()) {
            CM2Model* unitModel = unit->GetObjectModel();

            if (unitModel) {
                uint32_t unitVariation = unitModel->IsLoaded(0, 0) && mirrorVariation < unitModel->GetSequenceVariationCount(sequenceId) ? mirrorVariation : 0xFFFFFFFF;
                unit->SetKeyBoneSequence(unitModel, boneId, sequenceId, unitVariation, time, speed, blend, primary, 1);
            }
        }
    }
}

// VERIFY: a814c576 manual
// OFFSET: 0x00735A60
int32_t CGUnit_C::UnsetKeyBoneSequence(CM2Model* model, uint32_t boneId, int32_t blend, int32_t primary, int32_t force) {
    if (!model) {
        return 0;
    }

    if (!force && this->m_vehiclePassenger && this->m_vehiclePassenger->OverridesModelAnimation()) {
        return 0;
    }

    int32_t result;

    if (model->IsLoaded(0, 0) && model->BoneHasParent(boneId)) {
        result = 1;
        model->UnsetBoneSequence(boneId, blend, primary);
    } else {
        result = 0;
    }

    CVehicle_C* vehicle = this->m_vehicle;

    if (vehicle && vehicle->m_vehicleRec && vehicle->IsMirroringAnimsToPassengers() && model == this->GetObjectModel()) {
        for (CPassenger* passenger = this->m_vehicle->m_passengers.Head(); passenger; passenger = this->m_vehicle->m_passengers.Next(passenger)) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPE_UNIT, __FILE__, __LINE__));

            if (unit && unit->m_vehiclePassenger && unit->m_vehiclePassenger->OverridesModelAnimation()) {
                unit->UnsetKeyBoneSequence(unit->GetObjectModel(), boneId, blend, primary, 1);
            }
        }
    }

    return result;
}

// VERIFY: 3f921c2a manual
// OFFSET: 0x00735BB0
void CGUnit_C::SetBoneSequenceTimeRecursive(CM2Model* model, uint32_t boneId, uint32_t time, int32_t force) {
    if (!model) {
        return;
    }

    if (!force && this->m_vehiclePassenger && this->m_vehiclePassenger->OverridesModelAnimation()) {
        return;
    }

    model->SetBoneSequenceTime(boneId, time);

    CVehicle_C* vehicle = this->m_vehicle;

    if (!vehicle || !vehicle->m_vehicleRec || !vehicle->IsMirroringAnimsToPassengers() || model != this->GetObjectModel()) {
        return;
    }

    for (CPassenger* passenger = this->m_vehicle->m_passengers.Head(); passenger; passenger = this->m_vehicle->m_passengers.Next(passenger)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit && unit->m_vehiclePassenger && unit->m_vehiclePassenger->OverridesModelAnimation()) {
            unit->SetBoneSequenceTimeRecursive(unit->GetObjectModel(), boneId, time, 1);
        }
    }
}

// VERIFY: 2a100620 manual
// OFFSET: 0x00735DD0
void CGUnit_C::UpdateBoneSequenceFlags(CM2Model* model, int32_t freeze, int32_t force) {
    if (!model) {
        return;
    }

    if (!force && this->m_vehiclePassenger && this->m_vehiclePassenger->OverridesModelAnimation()) {
        return;
    }

    if (freeze) {
        uint32_t time = model->m_scene->m_time;
        model->m_lastAnimTime = time ? time : 1;
    } else {
        model->m_lastAnimTime = 0;
    }

    CVehicle_C* vehicle = this->m_vehicle;

    if (!vehicle || !vehicle->m_vehicleRec || !vehicle->IsMirroringAnimsToPassengers() || model != this->GetObjectModel()) {
        return;
    }

    for (CPassenger* passenger = this->m_vehicle->m_passengers.Head(); passenger; passenger = this->m_vehicle->m_passengers.Next(passenger)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit && unit->m_vehiclePassenger && unit->m_vehiclePassenger->OverridesModelAnimation()) {
            unit->UpdateBoneSequenceFlags(unit->GetObjectModel(), freeze, 1);
        }
    }
}

// VERIFY: 4da122b5 manual
// OFFSET: 0x00738440
void CGUnit_C::ResetKeyBoneSequences() {
    CM2Model* model = this->m_worldModel;

    if (model) {
        if (this->m_animationState & 0x80) {
            uint32_t torsoBone = this->m_torsoKeyBone;

            if (!this->m_vehiclePassenger || !this->m_vehiclePassenger->OverridesModelAnimation()) {
                if (model->IsLoaded(0, 0) && model->BoneHasParent(torsoBone)) {
                    model->UnsetBoneSequence(torsoBone, 1, 1);
                }

                CVehicle_C* vehicle = this->m_vehicle;

                if (vehicle && vehicle->m_vehicleRec && vehicle->IsMirroringAnimsToPassengers() && model == this->GetObjectModel()) {
                    for (CPassenger* passenger = this->m_vehicle->m_passengers.Head(); passenger; passenger = this->m_vehicle->m_passengers.Next(passenger)) {
                        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*passenger->m_ownerGuid, TYPE_UNIT, __FILE__, __LINE__));

                        if (unit && unit->m_vehiclePassenger && unit->m_vehiclePassenger->OverridesModelAnimation()) {
                            unit->UnsetKeyBoneSequence(unit->GetObjectModel(), torsoBone, 1, 1, 1);
                        }
                    }
                }
            }
        }

        this->SetKeyBoneSequence(model, 0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
    }

    if (this->m_mountModel) {
        this->SetKeyBoneSequence(this->m_mountModel, 0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
    }
}

// VERIFY: 9afd57a2 manual
// OFFSET: 0x0061F890
uint8_t CGUnit_C::GetAuraLevel(uint32_t index) {
    return this->m_auras[index].m_level;
}

// VERIFY: 8b0d6c44 manual
// OFFSET: 0x0061F8D0
uint8_t CGUnit_C::GetAuraStackCount(uint32_t index) {
    return this->m_auras[index].m_stackCount;
}

// VERIFY: 4d921ef3 auto
// OFFSET: 0x00653A10
void CGUnit_C::GetAFKText(char* buffer, uint32_t size) const {
}

// VERIFY: 4f552ebd auto
// OFFSET: 0x00653A10
void CGUnit_C::GetDNDText(char* buffer, uint32_t size) const {
}

// VERIFY: 98c5ab58 auto
// OFFSET: 0x00653A10
void CGUnit_C::GetGMText(char* buffer, uint32_t size) const {
}

// VERIFY: 391de47a auto
// OFFSET: 0x00653A10
void CGUnit_C::GetDevText(char* buffer, uint32_t size) const {
}

// VERIFY: dbd6dbc3 auto
// OFFSET: 0x008E5250
const CGUnitVirtualItem* CGUnit_C::GetDefendingItem() const {
    return nullptr;
}

// VERIFY: dfd1f58b auto
// OFFSET: 0x007161C0
ATTACKROUNDINFO::ATTACKROUNDINFO() {
    uint32_t* words = reinterpret_cast<uint32_t*>(this);

    for (int32_t i = 0; i < 21; i++) {
        words[i] = 0;
    }

    for (int32_t i = 0; i < 6; i++) {
        words[21 + i] = 0;
    }

    for (int32_t i = 0; i < 8; i++) {
        this->unk_006C[i] = 0.0f;
    }

    for (int32_t i = 0; i < 5; i++) {
        this->unk_008C[i] = 0;
    }
}

// VERIFY: fd6d8103 manual
// OFFSET: 0x0073F660
CGUnit_C::CGUnit_C(uint32_t time, const CClientObjCreate& create)
    : CGObject_C(time, create),
      m_swimDepth(0.0f),
      m_movementData(create.m_move.m_status.m_position, create.m_move.m_status.m_facing, this->m_obj->m_guid, this),
      m_smoothFacing(0.0f),
      m_smoothSwimPitch(0.0f),
      m_smoothSwimRoll(0.0f),
      m_smoothFacingTarget(0.0f),
      unused_08E0(-1),
      unused_08E4(),
      m_splashSoundKitId(0),
      m_reactionSoundPriority(0),
      m_floodsafeTurnPitch(),
      m_creatureCacheEntry(nullptr),
      m_displayInfo(nullptr),
      m_displayInfoExtra(nullptr),
      m_modelData(nullptr),
      m_soundData(nullptr),
      m_soundDataOverride(nullptr),
      m_bloodlevels(nullptr),
      m_fishingLineEffect(nullptr),
      m_auraEffects(nullptr),
      unk_0988(nullptr),
      m_mountModel(nullptr),
      m_mountScale(1.0f),
      m_forceWorldModelAlpha(0),
      m_mountTransition(nullptr),
      m_mountTransitionEffect(nullptr),
      m_mountTransitionSpellId(0),
      m_critterDisplayCreatureId(0),
      m_displayId(0),
      m_groundNormal(0.0f, 0.0f, 1.0f),
      m_lastMissileTime(0),
      m_missiles(nullptr),
      m_pendingMissiles(nullptr),
      m_shapeshiftSuppressed(0),
      m_animStandState(UNIT_STANDING),
      m_nextBloodPoolTime(0),
      m_combat(),
      m_stopAttackSent(0),
      m_animationState(0x70),
      m_footprintTextureId(-1),
      m_terrainTypeId(-1),
      m_footprintLength(0.27777779f),
      m_footprintWidth(0.27777779f),
      m_footprintParticleScale(1.0f),
      m_lastSentFacing(-10.0f),
      m_lastSentPitch(3.4028235e38f),
      m_nextRippleTime(0),
      m_castCount(0),
      m_castSpellId(0),
      m_castSchoolImmunities(0),
      m_castImmunities(0),
      m_castingSpellId(0),
      m_castStartTime(0),
      m_castEndTime(0),
      m_channelSpellId(0),
      m_channelStartTime(0),
      m_channelEndTime(0),
      m_pendingModelAuraSpellId(0),
      m_scriptRegistration(0),
      m_renderFacing(m_movementData.m_facing),
      m_facingVelocity(0.0f),
      m_facingBlend(1.0f),
      m_lootableTime(0),
      m_mountFootprintLength(0.0f),
      m_mountFootprintWidth(0.0f),
      m_animEventPosition(0.0f, 0.0f, 0.0f),
      m_procEffects(nullptr),
      unused_0B00(),
      m_selectionRadius(0.0f),
      m_flashStartTime(0),
      m_proceduralFadeAlpha(0xFF),
      m_proceduralFadeType(0),
      m_proceduralFadeStart(0),
      m_proceduralFadeDuration(0),
      m_proceduralFadeEndAlpha(0),
      m_pendingWeaponTrail(),
      m_modelScale(1.0f),
      m_rangedWeaponModel(nullptr),
      m_ammoModel(nullptr),
      m_ammoDisplayId(0),
      m_characterComponent(nullptr),
      m_previousSheatheState(static_cast<WEAPONMODE>(this->m_unit->m_unitFieldBytes2[0])),
      m_sheatheState(static_cast<WEAPONMODE>(this->m_unit->m_unitFieldBytes2[0])),
      m_chainSpellLocation(0.0f, 0.0f, 0.0f),
      m_mountAnimBehaviorId(91),
      m_animTier(0),
      m_torsoKeyBone(-1),
      m_meleeAttackAnimId(-1),
      m_pendingAnimId(-1),
      m_awaitingEndAnimId(-1),
      m_namePlateFrame(nullptr),
      m_chatBubble(nullptr),
      m_masterLooter(),
      m_looter(),
      m_lightningSequence(0),
      m_vehicle(nullptr),
      m_vehiclePassenger(nullptr),
      m_vehicleCamera(nullptr),
      m_missileTrajectorySpell(0),
      m_missileTrajectoryCastId(0),
      m_missileTrajectoryStart(0),
      m_missileTrajectoryEnd(0),
      m_missileTrajectoryReleased(0),
      m_wheelMask(0),
      m_lastWheelUpdateTime(0),
      m_specialIdleAnim(-1),
      m_precastMissileModel(nullptr),
      m_predictedHealth(this->m_unit->m_unitFieldHealth),
      m_powerRegenRemainder(0),
      m_manaRegenInterruptTime(0) {
    this->m_movement = &this->m_movementData;
    this->m_animStateFlags = 0;
    this->m_clientFlags = 0x400000;

    this->SetClientInitData(create, false);

    if (static_cast<float>(this->m_unit->m_unitFieldHealth) / static_cast<int32_t>(this->m_unit->m_unitFieldMaxhealth) < 0.2f && this->m_unit->m_unitFieldHealth > 0 && this->m_bloodlevels) {
        this->m_clientFlags |= 0x2;
    }

    this->m_weaponTrails[0] = nullptr;
    this->m_weaponTrails[1] = nullptr;

    this->m_emoteQueue[0].m_delay = 0;
    this->m_emoteQueue[1].m_delay = 0;
    this->m_emoteQueue[1].m_emote = 0;
    this->m_emoteQueue[2].m_delay = 0;
    this->m_emoteQueue[2].m_emote = 0;
    this->m_emoteQueue[3].m_delay = 0;
    this->m_emoteQueue[3].m_emote = 0;
    this->m_emoteQueue[0].m_emote = -1;

    this->m_targetChangeTime = FrameTime::s_curTimeMs - 1000;

    this->InitPredictedPower();

    for (int32_t i = 0; i < NUM_VIRTUAL_MONSTER_SLOTS; i++) {
        ItemRec* rec = g_itemDB.GetRecord(this->m_unit->m_unitVirtualItemSlotId[i]);
        CGUnitVirtualItem item;

        if (rec) {
            this->m_virtualItemDisplayId[i] = rec->m_displayInfoID;
            item.m_classID = rec->m_classID;
            item.m_subclassID = rec->m_subclassID;
            item.m_soundOverrideSubclassID = rec->m_soundOverrideSubclassID;
            item.m_material = rec->m_material;
            item.m_inventoryType = rec->m_inventoryType;
            item.m_sheatheType = rec->m_sheatheType;
        } else {
            item.m_classID = 0;
            item.m_subclassID = 0;
            item.m_soundOverrideSubclassID = 0;
            item.m_material = 0;
            item.m_inventoryType = 0;
            item.m_sheatheType = 0;
            this->m_virtualItemDisplayId[i] = 0;
        }

        item.m_pad[0] = 0;
        item.m_pad[1] = 0;
        this->m_virtualItem[i] = item;
    }

    this->RefreshDataPointers();

    if (create.m_updateFlags & 0x1) {
        this->InitializeActivePlayerComponent();
    }
}

// VERIFY: 174f1165 manual
// OFFSET: 0x0073C260
void CGUnit_C::SetClientInitData(const CClientObjCreate& create, bool reenable) {
    this->m_combat.SetClientInitData(create);

    if (create.m_updateFlags & 0x80) {
        this->CreateVehicleData(&create, create.m_vehicleId);
    }

    if (reenable) {
        return;
    }

    int32_t activePlayer = create.m_updateFlags & 0x1;
    this->m_movementData.SetUpdateInfo(OsGetAsyncTimeMs(), &create.m_move, activePlayer);

    if (this->m_movementData.m_flags & MOVEMENTFLAG_FALLING_FAR) {
        this->OnCollideFalling();
    }

    if (create.m_updateFlags & 0x1) {
        this->m_clientFlags |= 0x80;
        Weather* weather = World::s_weather;
        C3Vector position = this->GetPosition();
        weather->m_positionX = position.x;
        weather->m_positionY = position.y;
        weather->m_positionZ = position.z;
    }

    if (create.m_updateFlags & 0x400) {
        this->m_clientFlags |= 0x40000000;
    }
}

// VERIFY: 62962b3e auto
// OFFSET: 0x0073AD00
void CGUnit_C::OnCollideFalling() {
    if (!(this->m_movementData.m_flags & MOVEMENTFLAG_FALLING_FAR) || this->m_unit->m_unitFieldHealth <= 0) {
        return;
    }

    if (this->m_vehicle && this->m_vehicle->m_vehicleRec && this->m_vehicle->HasHighPriorityAnim()) {
        return;
    }

    if (this->m_vehiclePassenger && this->m_vehiclePassenger->m_state) {
        return;
    }

    this->PlayBaseAnimation(ANIM_FALL, 0);
}

// VERIFY: 8cfa7612 auto
// OFFSET: 0x0072D940
void CGUnit_C::RefreshDataPointers() {
    int32_t displayId = this->m_displayId;

    if (!displayId || this->m_unit->m_unitFieldNativedisplayid != this->m_unit->m_unitFieldDisplayid) {
        displayId = this->m_unit->m_unitFieldDisplayid;
    }

    this->m_displayInfo = g_creatureDisplayInfoDB.GetRecord(displayId);

    if (!this->m_displayInfo) {
        SysMsgPrintf(SYSMSG_ERROR, 2, "NOUNITDISPLAYID|%d|%s", displayId, this->GetUnitName(nullptr, 1));
        this->m_displayInfo = g_creatureDisplayInfoDB.m_numRecords > 0 ? g_creatureDisplayInfoDB.m_records : nullptr;
    }

    CreatureDisplayInfoRec* displayInfo = this->m_displayInfo;
    this->m_displayInfoExtra = g_creatureDisplayInfoExtraDB.GetRecord(displayInfo->m_extendedDisplayInfoID);

    CreatureModelDataRec* modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);
    this->m_modelData = modelData;

    this->m_soundData = g_creatureSoundDataDB.GetRecord(displayInfo->m_soundID);

    if (!this->m_soundData) {
        this->m_soundData = g_creatureSoundDataDB.GetRecord(modelData->m_soundID);
    }

    this->m_bloodlevels = g_unitBloodLevelsDB.GetRecord(displayInfo->m_bloodID);

    if (!this->m_bloodlevels) {
        this->m_bloodlevels = g_unitBloodLevelsDB.GetRecord(modelData->m_bloodID);

        if (!this->m_bloodlevels) {
            this->m_bloodlevels = g_unitBloodLevelsDB.m_numRecords > 0 ? g_unitBloodLevelsDB.m_records : nullptr;
        }
    }

    if (this->m_obj->m_type == (TYPE_UNIT | TYPE_OBJECT)) {
        this->m_creatureCacheEntry = g_creatureDBCache.GetRecord(this->m_obj->m_entryID, this->m_obj->m_guid, &CreatureQueryCallback, nullptr, false);
    }

    if (this->m_unit->m_unitFieldNativedisplayid == this->m_unit->m_unitFieldDisplayid) {
        this->m_clientFlags |= 0x100;
    } else {
        this->m_clientFlags &= ~0x100u;
    }

    if (this->m_modelData->m_flags & 0x8) {
        this->m_animationState |= 0x20000;
    } else {
        this->m_animationState &= ~0x20000u;
    }

    if (this->m_modelData->m_flags & 0x40) {
        this->m_clientFlags |= 0x2000000;
    } else {
        this->m_clientFlags &= ~0x2000000u;
    }
}

// VERIFY: 6c3e7cce auto
// OFFSET: 0x0071CFA0
void CGUnit_C::InitializeActivePlayerComponent() {
    CCharacterComponent* component = CCharacterComponent::m_activePlayerComponent;

    if (!component) {
        return;
    }

    if (this->m_modelData && (this->m_modelData->m_flags & 0x4) && CGPlayer_C::s_displayId == this->m_unit->m_unitFieldDisplayid) {
        this->SetCharacterModel(CCharacterComponent::m_activePlayerComponent->m_data.m_model);
        this->m_characterComponent = component;
        CCharacterComponent::m_activePlayerComponent = nullptr;
    } else {
        CCharacterComponent::FreeComponent(CCharacterComponent::m_activePlayerComponent);
        CCharacterComponent::m_activePlayerComponent = nullptr;
    }
}

// VERIFY: a44c5498 manual
// OFFSET: 0x0073FCC0
void CGUnit_C::PostInit(uint32_t time, const CClientObjCreate& create, bool reenable) {
    this->m_movementData.UpdateTransportStatus(create);
    this->m_modelScale = this->GetModelScale(this->m_unit->m_unitFieldDisplayid);

    if (!(this->m_obj->m_type & TYPE_PLAYER)) {
        this->OnMoveUpdate(time, 1, 1);
    }

    this->UpdateSelectionRadius();
    this->UpdateEffectAttachments();
    CGObject_C::PostInit(time, create, reenable);
    this->VehiclePassengerInit();
    MovementUpdateCameraYaw(this->m_obj->m_guid, this->m_movementData.m_transportGuid);

    CM2Model* model = this->m_worldModel;
    model->SetSequenceCallback(reinterpret_cast<void*>(&CGUnit_C::AnimEndCallback), this->m_obj->m_guid.m_guid);
    model->SetEventCallback(reinterpret_cast<void*>(&CGUnit_C::AnimEventCallback), this->m_obj->m_guid.m_guid);

    if (this->m_displayInfo) {
        CCharacterComponent::ApplyMonsterGeosets(this->m_worldModel, this->m_displayInfo);
        CCharacterComponent::ReplaceMonsterSkin(this->m_worldModel, this->m_displayInfo, this->m_modelData);

        if (this->m_modelData) {
            this->m_worldModel->m_fFlags ^= (this->m_worldModel->m_fFlags ^ (this->m_modelData->m_flags >> 7)) & 0x4;
        }
    }

    this->m_mountDisplayId = this->m_unit->m_unitFieldMountdisplayid;

    CreatureModelDataRec* modelData = this->m_modelData;
    this->m_footprintTextureId = modelData->m_footprintTextureID;
    this->m_footprintLength = modelData->m_footprintTextureWidth * 0.027777778f;
    this->m_footprintWidth = modelData->m_footprintTextureLength * 0.027777778f;
    this->m_footprintParticleScale = modelData->m_footprintParticleScale;

    this->CheckLoopSound();

    if ((create.m_updateFlags & 0x1) && this->m_modelData && (this->m_modelData->m_flags & 0x4) && CGPlayer_C::s_displayId == this->m_unit->m_unitFieldDisplayid && this->m_characterComponent) {
        this->m_clientFlags &= 0xFFBDFFFF;
        static_cast<CGPlayer_C*>(this)->RefreshVisibleItems();
    } else {
        if (this->m_characterComponent) {
            CCharacterComponent::FreeComponent(this->m_characterComponent);
            this->m_characterComponent = nullptr;
            this->m_clientFlags = (this->m_clientFlags & 0xFFFDFFFF) | 0x400000;
        }

        this->m_clientFlags = (this->m_clientFlags & 0xFFFDFFFF) | 0x400000;
    }

    this->UpdateUnitCollisionBox(1, 1);

    ChrRacesRec* race = g_chrRacesDB.GetRecord(this->m_unit->m_unitFieldBytes0[0]);

    if (race) {
        this->m_splashSoundKitId = race->m_splashSoundID;
    }

    this->m_smoothTarget.m_value = CMath::normalizeangle0to2pi_(this->GetRawFacing());
    this->m_smoothTarget.m_velocity = 0.0f;
    this->m_turnDelta[0] = 0.0f;
    this->m_turnDelta[1] = 0.0f;
    this->m_turnDelta[2] = 0.0f;
    this->m_turnDelta[3] = 0.0f;
    this->Animate(0.0f);

    if (this->m_unit->m_unitFieldHealth > 0) {
        if (static_cast<int32_t>(this->m_unit->m_unitFieldMountdisplayid) > 0) {
            this->CreateUnitMount(1, 1);
        }
    } else {
        this->m_lootableTime = time;
        this->ClearEffectList(0);
        this->ShowLootEffect();
    }

    int32_t groundType;

    if (!ClntObjMgrGetPlayerType() && World::QueryGroundType(this->m_worldObject, &groundType)) {
        this->m_terrainTypeId = groundType;
    }

    this->UpdateScriptRegistration();

    if (this->m_nameDesc) {
        PlayerNameDelete(this->m_nameDesc);
    }

    WOWGUID guid = this->m_obj->m_guid;
    this->m_nameDesc = PlayerNameCreate(guid);

    this->UpdateBreathState(FrameTime::s_curTimeMs);
    this->UpdateChannelEffects();

    if (!this->ShouldFadeIn()) {
        this->m_fadeDuration = 0;
        this->m_alpha = this->m_fadeEndAlpha;
    }

    this->m_animTier = this->m_unit->m_unitFieldBytes1[3];
    this->m_movementData.UpdateHoverState(this->m_animTier == 2, 0);
    this->m_movementData.m_hoverHeight = this->m_unit->m_unitFieldHoverheight;
    this->CreateOrDestroyObjectEffectManager();

    if (reenable && !(this->m_obj->m_type & TYPE_PLAYER)) {
        this->m_animStandState = UNIT_NUMSTANDSTATES;
    }

    this->UpdateBaseAnimation(false, -1);

    uint32_t sequenceId;

    if (this->m_worldModel && this->m_worldModel->IsLoaded(0, 0)) {
        if (this->m_torsoKeyBone == -1 || (sequenceId = this->m_worldModel->GetBoneSequenceId(this->m_torsoKeyBone)) == 0xFFFFFFFF) {
            sequenceId = this->m_worldModel->GetBoneSequenceId(0xFFFFFFFF);
        }
    } else {
        sequenceId = 0xFFFFFFFF;
    }

    AnimationDataRec* anim = g_animationDataDB.GetRecord(sequenceId);

    if (anim && anim->m_behaviorID == 127) {
        this->m_fadeDuration = 0;
        this->m_alpha = this->m_fadeEndAlpha;
    }

    const WOWGUID& charmedBy = this->m_unit->m_unitFieldCharmedby;

    if (charmedBy) {
        Script_SendUnitSignal(charmedBy, 0);
    }

    const WOWGUID& summonedBy = this->m_unit->m_unitFieldSummonedby;

    if (summonedBy) {
        Script_SendUnitSignal(summonedBy, 2);
    }

    this->UpdatePartyMemberPetState();

    TotemInfo* totem = CGGameUI::GetTotemInfo(this->m_obj->m_guid);

    if (totem) {
        totem->m_name = this->GetUnitName(nullptr, 1);
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_TOTEM_UPDATE, "%d", totem->m_slot + 1);
    }

    this->m_movementData.SnapToGroundIfCloseEnough();

    VehicleRec* vehicleRec = this->m_vehicle ? this->m_vehicle->m_vehicleRec : nullptr;

    if (vehicleRec && (vehicleRec->m_flags & 0x10000000)) {
        WOWGUID vehicleGuid = this->m_obj->m_guid;
        CGBattlefieldInfo::AddVehicle(vehicleGuid);
    }

    this->m_clientFlags |= 0x80000;

    if (CGUnit_C::s_deferredClientControlUpdateGUID == this->m_obj->m_guid) {
        this->ExecuteClientControlUpdate(CGUnit_C::s_deferredClientControlUpdateGUID, CGUnit_C::s_deferredClientControlUpdateState);
        CGUnit_C::s_deferredClientControlUpdateGUID = WOWGUID();
    }

    CMissileCollision::MaybeAddUnitToSystem(this);

    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (CGBattlefieldInfo::m_instanceType == 4 && this->m_unit->m_unitFieldPetnumber && player && player->UnitReaction(this) <= UNIT_REACTION_HOSTILE) {
        WOWGUID petGuid = this->m_obj->m_guid;
        const WOWGUID* owner = this->m_unit->m_unitFieldCharmedby ? &this->m_unit->m_unitFieldCharmedby : &this->m_unit->m_unitFieldCreatedby;
        CGBattlefieldInfo::AddArenaOpponentPet(petGuid, *owner);
    }

    if (this->m_vehicle && this->m_vehicle->m_vehicleRec && this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        this->SignalPlayerGainsVehicleDataEvent();
    }
}

// VERIFY: ab6c33e7 manual
// OFFSET: 0x00722AE0
float CGUnit_C::GetModelScale(uint32_t displayId) {
    CreatureDisplayInfoRec* displayInfo = g_creatureDisplayInfoDB.GetRecord(displayId);

    if (!displayInfo) {
        return 1.0f;
    }

    CreatureModelDataRec* modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);

    if (!modelData) {
        return 1.0f;
    }

    return this->GetModelScale(displayInfo, modelData);
}

// VERIFY: 7ba41095 manual
// OFFSET: 0x0073AB20
void CGUnit_C::OnMoveUpdate(uint32_t time, int32_t a3, int32_t a4) {
    if (this->m_vehicle && this->m_vehicle->m_vehicleRec) {
        this->m_vehicle->UpdateWorldMatrix();
    }

    this->UpdateWorldObject(0);

    if (!this->m_worldObject || !World::QueryGroundType(this->m_worldObject, &this->m_terrainTypeId)) {
        this->m_terrainTypeId = -1;
    }

    this->UpdateFlightStatus(time);
    this->UpdateSwimmingStatus(time, a3);
}

// VERIFY: ac360cae auto
// OFFSET: 0x0074D070
void CGUnit_C::VehiclePassengerInit() {
    if (this->m_vehiclePassenger) {
        this->m_vehiclePassenger->Free();
        this->m_vehiclePassenger = nullptr;
    }

    CVehiclePassenger_C::Init(this);

    if (!this->m_vehicleCamera) {
        this->OnVehicleCameraPossiblyNeeded();
        return;
    }

    this->m_vehicleCamera->BeginTransition(FrameTime::s_curTimeMs);

    if (!this->m_vehicleCamera->m_state) {
        this->DestroyVehicleCamera();
    }
}

// VERIFY: 3c56a72f auto
// OFFSET: 0x007179D0
void CGUnit_C::SetCharacterModel(CM2Model* model) {
    if (this->m_worldModel == model) {
        return;
    }

    if (!this->m_mountModel) {
        this->CleanUpVehicleBoneAnimsBeforeObjectModelChange();
    }

    CM2Model* oldModel = this->SetModelStart(model);
    CMissile::UpdateAttachments(this->m_obj->m_guid);
    this->SetModelFinish(oldModel);
}

// VERIFY: c1bf7317 manual
// OFFSET: 0x00754FF0
void CGUnit_C::SetAttacking(WOWGUID guid) {
    this->m_combat.SetAttacking(guid);
    this->m_combat.m_attackSent = 0;
}

// VERIFY: 3ea9a78e manual
// OFFSET: 0x007237F0
void CGUnit_C::Reenable() {
    CGObject_C::Reenable();

    if (this->m_nameDesc) {
        PlayerNameDelete(this->m_nameDesc);
    }

    WOWGUID guid = this->m_obj->m_guid;
    this->m_nameDesc = PlayerNameCreate(guid);

    float facing = this->m_movementData.m_facing;
    this->m_smoothTarget.m_value = CMath::normalizeangle0to2pi_(facing);
    this->m_smoothTarget.m_velocity = 0.0f;
    this->m_turnDelta[0] = 0.0f;
    this->m_turnDelta[1] = 0.0f;
    this->m_turnDelta[2] = 0.0f;
    this->m_turnDelta[3] = 0.0f;
    this->m_renderFacing = facing;
    this->m_facingVelocity = 0.0f;
    this->m_facingBlend = 1.0f;

    this->m_predictedHealth = this->m_unit->m_unitFieldHealth;
    this->InitPredictedPower();
    this->m_powerRegenRemainder = 0;
    this->m_manaRegenInterruptTime = 0;

    if (static_cast<float>(this->m_unit->m_unitFieldHealth) / static_cast<int32_t>(this->m_unit->m_unitFieldMaxhealth) < 0.2f && !this->IsDeadOrGhost() && this->m_bloodlevels) {
        this->m_clientFlags |= 0x2;
    }

    this->m_animationState &= 0xFFFFDBFF;

    while (this->m_missiles) {
        this->m_missiles->DeleteSelf();
    }

    while (this->m_pendingMissiles) {
        this->m_pendingMissiles->DeleteSelf();
    }

    this->ClearEffectList(3);

    if (this->m_worldModel) {
        this->m_worldModel->m_lastAnimTime = 0;
    }

    this->ShowLootEffect();

    for (int32_t i = 0; i < NUM_VIRTUAL_MONSTER_SLOTS; i++) {
        ItemRec* rec = g_itemDB.GetRecord(this->m_unit->m_unitVirtualItemSlotId[i]);
        CGUnitVirtualItem item;

        if (rec) {
            this->m_virtualItemDisplayId[i] = rec->m_displayInfoID;
            item.m_classID = rec->m_classID;
            item.m_subclassID = rec->m_subclassID;
            item.m_soundOverrideSubclassID = rec->m_soundOverrideSubclassID;
            item.m_material = rec->m_material;
            item.m_inventoryType = rec->m_inventoryType;
            item.m_sheatheType = rec->m_sheatheType;
        } else {
            item.m_classID = 0;
            item.m_subclassID = 0;
            item.m_soundOverrideSubclassID = 0;
            item.m_material = 0;
            item.m_inventoryType = 0;
            item.m_sheatheType = 0;
            this->m_virtualItemDisplayId[i] = 0;
        }

        item.m_pad[0] = 0;
        item.m_pad[1] = 0;
        this->m_virtualItem[i] = item;
    }

    VehicleRec* vehicleRec = this->m_vehicle ? this->m_vehicle->m_vehicleRec : nullptr;

    if (vehicleRec && (vehicleRec->m_flags & 0x10000000)) {
        WOWGUID vehicleGuid = this->m_obj->m_guid;
        CGBattlefieldInfo::AddVehicle(vehicleGuid);
    }

    this->m_previousSheatheState = this->m_sheatheState;
    this->UpdateObjectEffectMovementStates();
}

// VERIFY: 6423fc74 manual
// OFFSET: 0x007402B0
void CGUnit_C::PostReenable() {
    CGObject_C::PostReenable();
    this->CheckLoopSound();
    this->UpdateChannelEffects();

    if (!this->ShouldFadeIn()) {
        this->m_fadeDuration = 0;
        this->m_alpha = this->m_fadeEndAlpha;
    }

    this->UpdateBaseAnimation(false, -1);
    this->DestroyUnitMount(1);
    this->UpdateDisplayInfo(0);

    if (static_cast<int32_t>(this->m_unit->m_unitFieldMountdisplayid) > 0) {
        this->CreateUnitMount(1, 1);
    }

    this->UpdateScriptRegistration();
    this->UpdatePartyMemberPetState();
    this->UpdateBreathState(FrameTime::s_curTimeMs);

    if (this->m_unit->m_unitFieldHealth <= 0 && !this->m_lootableTime) {
        this->m_lootableTime = FrameTime::s_curTimeMs;
        this->ShowLootEffect();
    }

    this->m_movementData.SnapToGroundIfCloseEnough();
    this->VehiclePassengerInit();

    if (CGUnit_C::s_deferredClientControlUpdateGUID == this->m_obj->m_guid) {
        this->ExecuteClientControlUpdate(CGUnit_C::s_deferredClientControlUpdateGUID, CGUnit_C::s_deferredClientControlUpdateState);
        CGUnit_C::s_deferredClientControlUpdateGUID = WOWGUID();
    }

    this->m_floodsafeTurnPitch[2] = 0;
    this->m_floodsafeTurnPitch[3] = 0;

    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (CGBattlefieldInfo::m_instanceType == 4 && this->m_unit->m_unitFieldPetnumber && player && player->UnitReaction(this) <= UNIT_REACTION_HOSTILE) {
        WOWGUID petGuid = this->m_obj->m_guid;
        const WOWGUID* owner = this->m_unit->m_unitFieldCharmedby ? &this->m_unit->m_unitFieldCharmedby : &this->m_unit->m_unitFieldCreatedby;
        CGBattlefieldInfo::AddArenaOpponentPet(petGuid, *owner);
    }

    if (this->m_vehicle && this->m_vehicle->m_vehicleRec) {
        this->m_vehicle->UpdateFreeSeatMask();
    }
}

// VERIFY: 1062337a auto
// OFFSET: 0x0070CBA0
void CGUnit_C::SetStorage(uint32_t* storage, uint32_t* saved) {
    this->CGObject_C::SetStorage(storage, saved);
    this->m_unit = reinterpret_cast<CGUnitData*>(storage + OBJECT_END);
    this->m_unitMirror = saved + CGObject::TotalFieldsSaved();
}

// VERIFY: 3ba2c3ae manual
// OFFSET: 0x00729220
void CGUnit_C::OnDeath() {
    if (this->m_obj->m_type & TYPE_PLAYER) {
        static_cast<CGPlayer_C*>(this)->OnDeath();
    }

    this->m_combat.m_attacking = WOWGUID();
    this->m_combat.m_attackSent = 0;
    this->m_stopAttackSent = 0;

    if (this->m_worldModel && this->m_worldModel->IsLoaded(0, 0)) {
        this->m_worldModel->m_lastAnimTime = 0;
    }

    if (!this->m_vehiclePassenger || this->m_vehiclePassenger->m_state != 3 || (this->m_unit->m_unitFieldFlags2 & 0x20000)) {
        this->OnDeathAnimate();
    }

    WOWGUID guid;

    if (!(this->m_unit->m_unitDynamicFlags & 0x20) || (guid = this->m_obj->m_guid, !CGGameUI::IsRaidMemberOrPet(guid))) {
        guid = this->m_obj->m_guid;
        Spell_C_CancelSpellByTarget(guid, 109);
    }

    this->DropTargetOnDeath();

    if (!this->m_creatureCacheEntry || !(this->m_creatureCacheEntry->m_typeFlags & 0x80)) {
        this->RemoveInteractIcon();
    }

    PlayerNameTriggerColorUpdate(this->m_nameDesc);

    if (this->m_namePlateFrame) {
        this->m_namePlateFrame->UpdateColorDisplay(this);
    }

    while (this->m_missiles) {
        this->m_missiles->DeleteSelf();
    }

    while (this->m_pendingMissiles) {
        this->m_pendingMissiles->DeleteSelf();
    }

    this->ClearEffectList(0);
    SI2::StopSound(&this->m_npcInteractSound, 1, -1.0f, 1);
    SI2::StopSound(&this->m_reactionSound, 1, -1.0f, 1);
    this->ClearTrackingTarget(false, true);

    guid = this->m_obj->m_guid;
    CGGameUI::ClearInteractTarget(guid, 1);
}

// VERIFY: 2de1faa3 auto
// OFFSET: 0x007272C0
void CGUnit_C::ClearTrackingTarget(bool a2, bool a3) {
    if (this->m_obj->m_guid != CGUnit_C::s_activeMover || CGUnit_C::s_trackingType == TRACKTYPE_NONE) {
        return;
    }

    if (CGUnit_C::s_trackingType == TRACKTYPE_FOLLOW) {
        FrameScript_SignalEvent(SCRIPT_EVENT_AUTOFOLLOW_END, nullptr);
    }

    uint32_t time = OsGetAsyncTimeMs();

    if (a2) {
        CGObject_C* target = ClntObjMgrObjectPtr(CGUnit_C::s_trackingGUID, TYPE_OBJECT, __FILE__, __LINE__);

        if (target) {
            C3Vector targetPosition = target->GetPosition();
            C3Vector position = this->GetPosition();
            CGUnit_C::s_trackingFacing = CalculateFacingTo(&position, &targetPosition);
            CGInputControl::GetActive()->ToggleControlBit(INPUT_TRACKING_TURN, 1, time, 0);
        }
    }

    if (a3) {
        if (s_clickToMoveActions[CGUnit_C::s_trackingType].m_enabled) {
            this->TrackingStopMove(time);
        }

        CGInputControl::GetActive()->ToggleControlBit(INPUT_TRACKING_TURN, 0, time, 0);
        CGInputControl::GetActive()->ToggleControlBit(INPUT_TRACKING_PITCH, 0, time, 0);
    }

    CGUnit_C::s_trackingGUID = WOWGUID();
    CGUnit_C::s_unused_CA11D0 = 0.0f;
    CGUnit_C::s_trackingFlags = 0;
    CGUnit_C::s_trackingStuckCount = 0;
    CGUnit_C::s_trackingType = TRACKTYPE_NONE;
}

// VERIFY: c1cb1df0 auto
// OFFSET: 0x00715B70
void CGUnit_C::ClearWeaponTrailHandles() {
    for (int32_t i = 0; i < 2; i++) {
        if (this->m_weaponTrails[i]) {
            WeaponTrailClose(this->m_weaponTrails[i]);
            this->m_weaponTrails[i] = nullptr;
        }
    }
}

// VERIFY: e7c232db manual
// OFFSET: [0x00734B50, 0x00737BA0]
CGUnit_C::~CGUnit_C() {
    while (this->m_missiles) {
        this->m_missiles->DeleteSelf();
    }

    while (this->m_pendingMissiles) {
        this->m_pendingMissiles->DeleteSelf();
    }

    this->RemoveMountModel();
    SI2::StopSound(&this->m_npcInteractSound, 1, -1.0f, 1);
    this->KillCreatureLoopSound();
    this->DestroyNamePlate();

    if (this->m_chatBubble) {
        CGChatBubbleFrame::Remove(this->m_chatBubble);
        this->m_chatBubble = nullptr;
    }

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        CGUnit_C::s_trackingType = TRACKTYPE_NONE;
        CGUnit_C::s_trackingStuckCount = 0;
        CGUnit_C::s_activeMover = WOWGUID();
    }

    if (this->m_nameDesc) {
        PlayerNameDelete(this->m_nameDesc);
    }

    this->m_nameDesc = nullptr;

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        if (this->m_worldModel) {
            this->m_worldModel->m_lastAnimTime = 0;
        }

        CCharacterComponent::m_activePlayerComponent = this->m_characterComponent;
        CGPlayer_C::s_displayId = this->m_unit->m_unitFieldDisplayid;
    } else if (this->m_characterComponent) {
        CCharacterComponent::FreeComponent(this->m_characterComponent);
        this->m_characterComponent = nullptr;
        this->m_clientFlags = (this->m_clientFlags & ~0x420000) | 0x400000;
    }

    this->ClearWeaponTrailHandles();

    for (CEffect* effect = this->m_effectList; effect; effect = effect->m_next) {
        if (effect->m_flags & 0x8) {
            effect->MoveToDestroyList();
            break;
        }
    }

    while (this->m_auraEffects) {
        this->m_auraEffects->Release();
    }

    while (this->m_procEffects) {
        this->m_procEffects->Release();
    }

    if (this->m_fishingLineEffect) {
        this->m_fishingLineEffect->Release();
        this->m_fishingLineEffect = nullptr;
    }

    if (this->unk_0988) {
        this->unk_0988->DeleteSelf();
    }

    if (this->m_rangedWeaponModel) {
        this->m_rangedWeaponModel->SetLoadedCallback(nullptr, nullptr);
        this->m_rangedWeaponModel->m_fFlags |= 0x20;
        this->m_rangedWeaponModel->m_drawCallback = nullptr;
        this->m_rangedWeaponModel->m_drawArg = nullptr;
        this->m_rangedWeaponModel->Release();
    }

    if (this->m_ammoModel) {
        this->m_ammoModel->Release();
    }

    g_nameDBCache.CancelCallback(this->m_obj->m_guid.m_guid, &NameQueryCallback, nullptr);
    g_petNameCache.CancelCallback(this->m_unit->m_unitFieldPetnumber, &NameQueryCallback, nullptr);

    if (this->m_precastMissileModel) {
        if (this->m_precastMissileModel->m_attachParent) {
            this->m_precastMissileModel->DetachFromParent();
        }

        this->m_precastMissileModel->Release();
        this->m_precastMissileModel = nullptr;
        this->m_precastCastCount = 0;
    }

    this->m_clientFlags &= ~0x1000000;
    this->DestroyVehicleData(0);

    if (this->m_vehiclePassenger) {
        this->m_vehiclePassenger->Free();
    }
}

// VERIFY: 1ffc0186 manual
// OFFSET: 0x00734FD0
void CGUnit_C::Disable() {
    CGObject_C::Disable();
    CMissileCollision::RemoveUnitFromSystem(this);

    VehicleRec* vehicleRec = this->m_vehicle ? this->m_vehicle->m_vehicleRec : nullptr;

    if (vehicleRec && (vehicleRec->m_flags & 0x10000000)) {
        WOWGUID guid = this->m_obj->m_guid;
        CGBattlefieldInfo::RemoveVehicle(guid);
    }

    this->DestroyVehicleCamera();

    if (this->m_vehicle) {
        this->m_vehicle->RemoveAllPassengersLocally();
    }

    this->m_chainSpellTargets.Clear();
    this->KillCreatureLoopSound();

    if (this->m_effectManager) {
        for (int32_t i = 1; i < 82; i++) {
            this->m_effectManager->ExitState(i, 0);
        }
    }

    this->m_clientFlags &= ~0x2;
    this->ClearThreatList();

    if (this->m_namePlateFrame) {
        if (CGUnit_C::s_namePlateFocus == this->m_namePlateFrame) {
            CGUnit_C::s_namePlateFocus = nullptr;
        }

        s_unusedNamePlates.LinkToTail(this->m_namePlateFrame);
        this->m_namePlateFrame->Hide();
        this->m_namePlateFrame = nullptr;
        this->ObjectNameVisibilityChanged();
    }

    if (this->m_chatBubble) {
        CGChatBubbleFrame::Remove(this->m_chatBubble);
        this->m_chatBubble = nullptr;
    }

    if (this->m_nameDesc) {
        PlayerNameDelete(this->m_nameDesc);
    }

    this->m_nameDesc = nullptr;

    WOWGUID guid = this->m_obj->m_guid;

    if (!CGGameUI::IsRaidMemberOrPet(guid) || this->IsDuelingWith(static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)))) {
        CGGameUI::BreakTarget(this->m_obj->m_guid);
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera->m_targetGUID == this->m_obj->m_guid) {
        camera->MakeRelativeTo(WOWGUID());
    }

    this->m_movementData.Disable();

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        if (CGUnit_C::s_trackingType != TRACKTYPE_NONE) {
            this->ClearTrackingTarget(false, true);
        }

        CGUnit_C::s_activeMover = WOWGUID();
    }

    guid = this->m_obj->m_guid;
    Spell_C_CancelSpellByCaster(guid);

    if (this->m_castSpellId) {
        this->m_castCount = 0;
        this->m_castSpellId = 0;
        this->m_castingSpellId = 0;
        this->m_castStartTime = 0;
        this->m_castEndTime = 0;
        this->UpdateSpellCastBars(FrameTime::s_curTimeMs);

        SpellRec spell;

        if (g_spellDB.GetRecord(this->m_castSpellId, &spell)) {
            guid = this->m_obj->m_guid;
            Script_SendUnitSpellSignal(guid, this->m_castCount, &spell, 325);
        }
    }

    if (this->m_unit->m_unitChannelSpell) {
        uint32_t elapsed = this->m_channelEndTime - this->m_channelStartTime;
        uint32_t now = OsGetAsyncTimeMs();
        this->m_channelEndTime = now;
        this->m_channelStartTime = now - elapsed;
        this->m_castingSpellId = 0;
        this->m_castStartTime = 0;
        this->m_castEndTime = 0;
        this->UpdateSpellCastBars(FrameTime::s_curTimeMs);
        this->m_channelSpellId = 0;
        this->m_channelStartTime = 0;
        this->m_channelEndTime = 0;
        this->UpdateSpellCastBars(FrameTime::s_curTimeMs);

        SpellRec spell;

        if (g_spellDB.GetRecord(this->m_unit->m_unitChannelSpell, &spell)) {
            guid = this->m_obj->m_guid;
            Script_SendUnitSpellSignal(guid, 0, &spell, 333);
        }
    }

    int32_t index;
    guid = this->m_obj->m_guid;

    if (CGPartyInfo::IsMemberPet(guid, &index) && index >= 0) {
        CGPartyMemberState* member = CGPartyInfo::m_members[index] ? &s_partyMemberStates[index] : nullptr;
        this->FillPartyMemberPetState(&member->m_pet);

        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_REQUEST_PARTY_MEMBER_STATS));
        msg.Put(member->m_guid.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    guid = this->m_obj->m_guid;

    if (CGRaidInfo::IsPetInRaid(guid, &index) && index >= 0) {
        CGRaidMember* raidMember = CGRaidInfo::m_members[index];
        CGPartyMemberState* member = raidMember->m_guid ? &raidMember->m_stats : nullptr;
        this->FillPartyMemberPetState(&member->m_pet);

        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_REQUEST_PARTY_MEMBER_STATS));
        msg.Put(member->m_guid.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    if (this->m_scriptRegistration) {
        this->m_scriptRegistration = 0;
        ScriptEventsUnregisterUnit(this);
    }

    while (this->m_auraEffects) {
        this->m_auraEffects->Release();
    }

    while (this->m_procEffects) {
        this->m_procEffects->Release();
    }

    if (this->m_fishingLineEffect) {
        this->m_fishingLineEffect->Release();
        this->m_fishingLineEffect = nullptr;
    }

    if (this->unk_0988) {
        this->unk_0988->DeleteSelf();
    }

    for (CEffect* effect = this->m_effectList; effect; effect = effect->m_next) {
        if (effect->m_flags & 0x8) {
            effect->MoveToDestroyList();
            break;
        }
    }

    while (this->m_missiles) {
        this->m_missiles->DeleteSelf();
    }

    while (this->m_pendingMissiles) {
        this->m_pendingMissiles->DeleteSelf();
    }

    this->ClearEffectList(2);

    for (uint32_t i = 0; i < (this->m_handItemSpellIds.m_count == 0xFFFFFFFF ? this->m_handItemSpellIds.m_dynamic.m_count : this->m_handItemSpellIds.m_count); i++) {
        this->m_handItemSpellIds[i] = 0;
    }

    this->m_animationState &= ~0x200;

    for (int32_t i = 0; i < 2; i++) {
        if (this->m_weaponTrails[i]) {
            WeaponTrailClose(this->m_weaponTrails[i]);
            this->m_weaponTrails[i] = nullptr;
        }
    }

    SI2::StopSound(&this->m_npcInteractSound, 0, 2.0f, 1);
    this->ClearTrackingTarget(false, true);

    this->m_pendingWeaponTrail[1] = 0;
    this->m_emoteQueue[0].m_delay = 0;
    this->m_emoteQueue[1].m_delay = 0;
    this->m_emoteQueue[1].m_emote = 0;
    this->m_emoteQueue[2].m_delay = 0;
    this->m_emoteQueue[2].m_emote = 0;
    this->m_emoteQueue[3].m_delay = 0;
    this->m_emoteQueue[3].m_emote = 0;
    this->m_animationState &= ~0x8000;
    this->m_emoteQueue[0].m_emote = -1;

    TotemInfo* totem = CGGameUI::GetTotemInfo(this->m_obj->m_guid);

    if (totem) {
        CGGameUI::DestroyTotem(totem->m_slot);
    }

    g_nameDBCache.CancelCallback(this->m_obj->m_guid.m_guid, &NameQueryCallback, nullptr);
    g_petNameCache.CancelCallback(this->m_unit->m_unitFieldPetnumber, &NameQueryCallback, nullptr);

    if (this->m_precastMissileModel) {
        if (this->m_precastMissileModel->m_attachParent) {
            this->m_precastMissileModel->DetachFromParent();
        }

        this->m_precastMissileModel->Release();
        this->m_precastMissileModel = nullptr;
        this->m_precastCastCount = 0;
    }

    this->m_clientFlags &= ~0x1000000;

    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (this->GetControllingPlayer() == player) {
        Spell_C_RemovePeriodicClientTrigger(this->m_obj->m_guid, 0);
    }

    if (this->m_vehiclePassenger) {
        this->m_vehiclePassenger->Uninit();
        this->m_vehiclePassenger = nullptr;
    }
}

// VERIFY: b83e445a auto
// OFFSET: 0x005EEB70
void WeaponTrailsInitialize() {
}

// VERIFY: bdb50cd0 manual
// OFFSET: 0x0072EA50
void CGUnit_C::OnSetRawFacingLocal(uint32_t eventTime, float facing) {
    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && CGUnit_C::s_trackingType != TRACKTYPE_NONE && !(CGUnit_C::s_trackingFlags & 0x1)) {
        this->ClearTrackingTarget(false, true);
    }

    this->m_movementData.OnSetRawFacingLocal(eventTime, facing);

    if (this->GetClientStandState() == UNIT_SLEEPING && (this->m_obj->m_type & TYPE_PLAYER)) {
        static_cast<CGPlayer_C*>(this)->ChangeStandState(UNIT_STANDING);
    }
}

// VERIFY: 0e4f2f38 manual
// OFFSET: 0x0072EAD0
void CGUnit_C::OnSetPitchLocal(uint32_t eventTime, float pitch) {
    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && CGUnit_C::s_trackingType != TRACKTYPE_NONE && !(CGUnit_C::s_trackingFlags & 0x1)) {
        this->ClearTrackingTarget(false, true);
    }

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player && player->IsLooting()) {
            CGGameUI::CloseLoot(true, true, false);
        }
    }

    this->m_movementData.OnSetPitchLocal(eventTime, pitch);
}

// VERIFY: ab2d4ed7 auto
// OFFSET: 0x0071C1E0
int32_t CGUnit_C::ClampRawAngleToLegalFacingRange(float& angle) const {
    float min;
    float max;

    if (!this->m_movementData.ComputeLegalRawFacingRange(min, max)) {
        return 0;
    }

    float wrapped = angle;
    CMovement_C::WrapFacingToRange(wrapped, min, max);

    if (wrapped < min) {
        angle = min;
        return 1;
    }

    if (max < wrapped) {
        angle = max;
        return 1;
    }

    return 0;
}

// VERIFY: eb34d923 manual
// OFFSET: 0x0072D3F0
void CGUnit_C::OnTurnToAngleLocal(uint32_t eventTime, float facing) {
    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && CGUnit_C::s_trackingType != TRACKTYPE_NONE && !(CGUnit_C::s_trackingFlags & 0x1)) {
        this->ClearTrackingTarget(false, true);
    }

    this->m_movementData.OnTurnToAngleLocal(eventTime, facing);

    if (this->GetClientStandState() == UNIT_SLEEPING && (this->m_obj->m_type & TYPE_PLAYER)) {
        static_cast<CGPlayer_C*>(this)->ChangeStandState(UNIT_STANDING);
    }
}

// VERIFY: 5a1f7b23 manual
// OFFSET: 0x0072D470
void CGUnit_C::OnPitchToAngleLocal(uint32_t eventTime, float pitch) {
    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && CGUnit_C::s_trackingType != TRACKTYPE_NONE && !(CGUnit_C::s_trackingFlags & 0x1)) {
        this->ClearTrackingTarget(false, true);
    }

    this->m_movementData.OnPitchToAngleLocal(eventTime, pitch);

    if (this->GetClientStandState() == UNIT_SLEEPING && (this->m_obj->m_type & TYPE_PLAYER)) {
        static_cast<CGPlayer_C*>(this)->ChangeStandState(UNIT_STANDING);
    }
}

// VERIFY: 356f0e0f auto
// OFFSET: 0x007189F0
void CGUnit_C::OnFallLocal(uint32_t eventTime) {
    this->m_movementData.OnFallLocal(eventTime);
}

// VERIFY: c6dbcf00 manual
// OFFSET: 0x0071A360
void CGUnit_C::ChangeStandState(UNITSTANDSTATE standState) {
    if (this->m_obj->m_type & TYPE_PLAYER) {
        static_cast<CGPlayer_C*>(this)->ChangeStandState(standState);
    }
}

// VERIFY: 63ea1979 auto
// OFFSET: 0x0071AE10
void CGUnit_C::OnMoveStopLocal(uint32_t eventTime) {
    this->m_movementData.OnMoveStopLocal(eventTime);
}

// VERIFY: a8a2c6c3 auto
// OFFSET: 0x0071AE20
void CGUnit_C::OnStrafeStopLocal(uint32_t eventTime) {
    this->m_movementData.OnStrafeStopLocal(eventTime);
}

// VERIFY: 32cb8207 auto
// OFFSET: 0x0071AE30
void CGUnit_C::OnAscendDescendStopLocal(uint32_t eventTime) {
    this->m_movementData.OnAscendDescendStopLocal(eventTime);
}

// VERIFY: 76f26165 auto
// OFFSET: 0x0071AE40
void CGUnit_C::OnTurnStopLocal(uint32_t eventTime) {
    this->m_movementData.OnTurnStopLocal(eventTime);
}

// VERIFY: c9647570 manual
// OFFSET: 0x0072E5D0
void CGUnit_C::OnMoveStartLocal(uint32_t eventTime, int32_t forward) {
    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && CGUnit_C::s_trackingType != TRACKTYPE_NONE && !(CGUnit_C::s_trackingFlags & 0x1)) {
        this->ClearTrackingTarget(false, true);
    }

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player && player->IsLooting()) {
            CGGameUI::CloseLoot(true, true, false);
        }
    }

    this->m_movementData.OnMoveStartLocal(eventTime, forward);
}

// VERIFY: 97b45c2f manual
// OFFSET: 0x0072E680
void CGUnit_C::OnStrafeStartLocal(uint32_t eventTime, int32_t left) {
    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && CGUnit_C::s_trackingType != TRACKTYPE_NONE && !(CGUnit_C::s_trackingFlags & 0x1)) {
        this->ClearTrackingTarget(false, true);
    }

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player && player->IsLooting()) {
            CGGameUI::CloseLoot(true, true, false);
        }
    }

    this->m_movementData.OnStrafeStartLocal(eventTime, left);
}

// VERIFY: 779d8288 manual
// OFFSET: 0x0072E730
void CGUnit_C::OnAscendDescendStartLocal(uint32_t eventTime, int32_t ascend) {
    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && CGUnit_C::s_trackingType != TRACKTYPE_NONE && !(CGUnit_C::s_trackingFlags & 0x1)) {
        this->ClearTrackingTarget(false, true);
    }

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player && player->IsLooting()) {
            CGGameUI::CloseLoot(true, true, false);
        }
    }

    this->m_movementData.OnAscendDescendStartLocal(eventTime, ascend);
}

// VERIFY: 9d72df24 manual
// OFFSET: 0x0072E7E0
void CGUnit_C::OnTurnStartLocal(uint32_t eventTime, int32_t left) {
    SpellRec spell;

    if (g_spellDB.GetRecord(this->m_unit->m_unitChannelSpell, &spell) && (spell.m_channelInterruptFlags & 0x10) && (spell.m_attributesEx & 0x4000) && this->IsAutoTracking()) {
        Spell_C_CancelChannelSpell(this->m_unit->m_unitChannelSpell);
    }

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && CGUnit_C::s_trackingType != TRACKTYPE_NONE && !(CGUnit_C::s_trackingFlags & 0x1)) {
        this->ClearTrackingTarget(false, true);
    }

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player && player->IsLooting()) {
            CGGameUI::CloseLoot(true, true, false);
        }
    }

    this->m_movementData.OnTurnStartLocal(eventTime, left);
}

// VERIFY: 4d6a8b9a manual
// OFFSET: 0x0072E900
void CGUnit_C::OnPitchStartLocal(uint32_t eventTime, int32_t up) {
    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && CGUnit_C::s_trackingType != TRACKTYPE_NONE && !(CGUnit_C::s_trackingFlags & 0x1)) {
        this->ClearTrackingTarget(false, true);
    }

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player && player->IsLooting()) {
            CGGameUI::CloseLoot(true, true, false);
        }
    }

    this->m_movementData.OnPitchStartLocal(eventTime, up);
}

// VERIFY: 978d0d24 manual
// OFFSET: 0x0072E9B0
void CGUnit_C::OnPitchStopLocal(uint32_t eventTime) {
    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && CGUnit_C::s_trackingType != TRACKTYPE_NONE && !(CGUnit_C::s_trackingFlags & 0x1)) {
        this->ClearTrackingTarget(false, true);
    }

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player && player->IsLooting()) {
            CGGameUI::CloseLoot(true, true, false);
        }
    }

    this->m_movementData.OnPitchStopLocal(eventTime);
}

// VERIFY: 6f683f46 manual
// OFFSET: 0x0072EB80
void CGUnit_C::TryJumpOrAscend(uint32_t eventTime) {
    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player && player->IsLooting()) {
            CGGameUI::CloseLoot(true, true, false);
        }
    }

    CreatureDisplayInfoRec* displayRec = g_creatureDisplayInfoDB.GetRecord(this->m_mountDisplayId);
    CreatureModelDataRec* modelRec = displayRec ? g_creatureModelDataDB.GetRecord(displayRec->m_modelID) : nullptr;

    if ((modelRec && (modelRec->m_flags & 0x400)) || this->m_mountDisplayId <= 0 || (this->m_clientFlags & 0x10000000) || (this->m_movementData.m_flags & 0x1000000) || ((this->m_movement->m_flags & 0xF) && !this->m_movementData.IsGravityDisabled())) {
        this->m_movementData.OnJumpLocal(eventTime);
        return;
    }

    if ((this->m_movement->m_flags & 0x30) || this->IsDeepInLiquid()) {
        return;
    }

    if (this->IsActivePlayer()) {
        this->OnSpecialMountAnim();
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_MOUNT_SPECIAL_ANIM));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 757413a3 auto
// OFFSET: 0x007151F0
static int32_t AckMessageHasValue(uint32_t msgId) {
    return msgId == CMSG_FORCE_RUN_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_RUN_BACK_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_SWIM_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_SWIM_BACK_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_FLIGHT_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_WALK_SPEED_CHANGE_ACK
        || msgId == CMSG_FORCE_TURN_RATE_CHANGE_ACK
        || msgId == CMSG_FORCE_PITCH_RATE_CHANGE_ACK
        || msgId == CMSG_MOVE_HOVER_ACK
        || msgId == CMSG_MOVE_SET_CAN_FLY_ACK
        || msgId == CMSG_MOVE_FEATHER_FALL_ACK
        || msgId == CMSG_MOVE_WATER_WALK_ACK
        || msgId == CMSG_MOVE_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY_ACK
        || msgId == CMSG_MOVE_SET_COLLISION_HGT_ACK;
}

// VERIFY: 4658bf14 manual
// OFFSET: 0x00717D90
void CGUnit_C::SendTimeSkip(uint32_t skip) {
    if (this->m_movement->IsOnSpline() || (this->m_movementData.m_flags & MOVEMENTFLAG_ONTRANSPORT)) {
        return;
    }

    CDataStore msg;
    SmartGUID guid;
    guid.m_guid = this->m_obj->m_guid.m_guid;
    (msg.Put(static_cast<uint32_t>(CMSG_MOVE_TIME_SKIPPED)) << guid).Put(skip);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 58c733be manual
// OFFSET: 0x0071EF20
int32_t CGUnit_C::IsAllowedToSendMessage(uint32_t msgId) const {
    if (this->m_obj->m_guid != CGUnit_C::s_activeMover) {
        return 0;
    }

    if (!this->m_movement->IsOnSpline() || IsMessageAllowedWhileOnSpline(msgId)) {
        return 1;
    }

    return 0;
}

// VERIFY: b1a3d933 auto
// OFFSET: 0x0071EF80
int32_t CGUnit_C::BuildMovementUpdate(uint32_t time, uint32_t msgId, CDataStore* msg, float value, uint32_t index) {
    msg->Put(msgId);
    SmartGUID guid;
    guid.m_guid = this->m_obj->m_guid.m_guid;
    *msg << guid;

    if (AckMessageNeedsIndex(msgId)) {
        msg->Put(index);
    }

    if (!IsAckMessage(msgId) && !this->IsAllowedToSendMessage(msgId)) {
        return 0;
    }

    this->m_movementData.WriteMovementStatusToPacket(msgId, time, msg);

    if (AckMessageHasValue(msgId)) {
        msg->Put(value);
    }

    if (!(this->m_movementData.m_flags & MOVEMENTFLAG_ONTRANSPORT)) {
        if (this->m_movement->m_flags & MOVEMENTFLAG_FALLING) {
            this->m_clientFlags |= 0x80;
        } else {
            this->m_clientFlags &= ~0x80u;
        }
    }

    return 1;
}

// VERIFY: fbc23c4e manual
// OFFSET: 0x0071F0C0
int32_t CGUnit_C::SendMovementUpdate(uint32_t time, uint32_t msgId, float value, uint32_t index, uint64_t transport, uint8_t seat) {
    CDataStore msg;
    this->m_lastSentFacing = this->GetRawFacing();

    if ((this->m_movement->m_flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || (this->m_movement->m_flags2 & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING)) {
        this->m_lastSentPitch = this->GetPitch();
    }

    if (!this->BuildMovementUpdate(time, msgId, &msg, value, index)) {
        return 0;
    }

    if (msgId == CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE) {
        SmartGUID transportGuid;
        transportGuid.m_guid = transport;
        msg << transportGuid;
        msg.Put(seat);
    }

    CMovement_C* movement = this->m_movement;

    if (movement->m_flags & (MOVEMENTFLAG_TURN_LEFT | MOVEMENTFLAG_TURN_RIGHT)) {
        this->m_clientFlags |= 0x4000000;
    }

    if (movement->m_flags & (MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN)) {
        this->m_clientFlags |= 0x8000000;
    }

    msg.Finalize();
    ClientServices::Send2(&msg);
    this->m_movementData.UpdateHeartbeatTimerA(time);
    return 1;
}

// VERIFY: e387a2ae manual
// OFFSET: 0x007219F0
int32_t CGUnit_C::HandleFloodsafeTurnEvent(uint32_t time, uint32_t msgId) {
    if (msgId != MSG_MOVE_STOP_TURN) {
        this->m_floodsafeTurnPitch[2] = msgId;
        this->m_floodsafeTurnPitch[0] = time;
        return 0;
    }

    if (!this->m_floodsafeTurnPitch[2] || (this->m_clientFlags & 0x4000000)) {
        this->SendMovementUpdate(time, MSG_MOVE_STOP_TURN, 0.0f, 0, 0, 255);
        this->m_clientFlags &= ~0x4000000u;
        this->m_floodsafeTurnPitch[2] = 0;
        return 1;
    }

    if (this->IsFacingUnchangedSinceSent()) {
        this->m_floodsafeTurnPitch[2] = 0;
        return 0;
    }

    this->SendMovementUpdate(time, MSG_MOVE_SET_FACING, 0.0f, 0, 0, 255);
    this->m_floodsafeTurnPitch[2] = 0;
    return 1;
}

// VERIFY: f8c93d37 manual
// OFFSET: 0x00721AC0
int32_t CGUnit_C::HandleFloodsafePitchEvent(uint32_t time, uint32_t msgId) {
    if (msgId != MSG_MOVE_STOP_PITCH) {
        this->m_floodsafeTurnPitch[3] = msgId;
        this->m_floodsafeTurnPitch[1] = time;
        return 0;
    }

    if (!this->m_floodsafeTurnPitch[3] || (this->m_clientFlags & 0x8000000)) {
        this->SendMovementUpdate(time, MSG_MOVE_STOP_PITCH, 0.0f, 0, 0, 255);
        this->m_clientFlags &= ~0x8000000u;
        this->m_floodsafeTurnPitch[3] = 0;
        return 1;
    }

    if (this->IsFacingUnchangedSinceSent()) {
        this->m_floodsafeTurnPitch[3] = 0;
        return 0;
    }

    this->SendMovementUpdate(time, MSG_MOVE_SET_PITCH, 0.0f, 0, 0, 255);
    this->m_floodsafeTurnPitch[3] = 0;
    return 1;
}

// VERIFY: 14912081 auto
// OFFSET: 0x00721B90
void CGUnit_C::UpdateFloodsafeMoveEvents(uint32_t time) {
    if (this->m_floodsafeTurnPitch[2] && time - this->m_floodsafeTurnPitch[0] > 100) {
        this->SendMovementUpdate(time, this->m_floodsafeTurnPitch[2], 0.0f, 0, 0, 255);
        this->m_floodsafeTurnPitch[2] = 0;
    }

    if (this->m_floodsafeTurnPitch[3] && time - this->m_floodsafeTurnPitch[1] > 100) {
        this->SendMovementUpdate(time, this->m_floodsafeTurnPitch[3], 0.0f, 0, 0, 255);
        this->m_floodsafeTurnPitch[3] = 0;
    }
}

// VERIFY: 02160bb9 auto
// OFFSET: 0x00721C20
int32_t CGUnit_C::FlushFloodsafeMoveEvents(uint32_t time) {
    int32_t sent = 0;

    if (this->m_floodsafeTurnPitch[2]) {
        if (this->SendMovementUpdate(time, this->m_floodsafeTurnPitch[2], 0.0f, 0, 0, 255)) {
            sent = 1;
        }

        this->m_floodsafeTurnPitch[2] = 0;
    }

    if (this->m_floodsafeTurnPitch[3]) {
        if (this->SendMovementUpdate(time, this->m_floodsafeTurnPitch[3], 0.0f, 0, 0, 255)) {
            sent = 1;
        }

        this->m_floodsafeTurnPitch[3] = 0;
    }

    return sent;
}

// VERIFY: 9b3f1307 manual
// OFFSET: 0x0071C8B0
int32_t CGUnit_C::IsBelowHalfBaseSpeed() const {
    return this->m_movementData.m_walkSpeed + this->m_movementData.m_walkSpeed < this->m_movementData.GetBaseSpeed(0);
}

// VERIFY: a27dcd81 manual
// OFFSET: 0x00721300
void CGUnit_C::UpdateObjectEffectMovementStates() {
    if (!this->m_effectManager) {
        return;
    }

    this->m_effectManager->EnterState((this->m_movement->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) ? 0x25 : 0x26, 1, 0);

    if (this->m_movement->m_flags & MOVEMENTFLAG_ASCENDING) {
        this->m_effectManager->EnterState(0x6, 1, 0);
    } else if (this->m_movement->m_flags & MOVEMENTFLAG_DESCENDING) {
        this->m_effectManager->EnterState(0x7, 1, 0);
    }

    this->m_effectManager->EnterState((this->m_movement->m_flags & (MOVEMENTFLAG_TURN_LEFT | MOVEMENTFLAG_TURN_RIGHT)) ? 0x2F : 0x30, 1, 0);

    if (this->m_movement->m_flags & MOVEMENTFLAG_FLYING) {
        this->m_effectManager->EnterState(0x3D, 1, 0);

        if (this->m_movement->m_flags & MOVEMENTFLAG_FORWARD) {
            this->m_effectManager->EnterState(0x3E, 1, 0);
        }

        if (this->m_movement->m_flags & MOVEMENTFLAG_BACKWARD) {
            this->m_effectManager->EnterState(0x3F, 1, 0);
        }

        if (this->m_movement->m_flags & (MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
            this->m_effectManager->EnterState(0x42, 1, 0);

            if (this->m_movement->m_flags & MOVEMENTFLAG_STRAFE_LEFT) {
                this->m_effectManager->EnterState(0x40, 1, 0);
            }

            if (this->m_movement->m_flags & MOVEMENTFLAG_STRAFE_RIGHT) {
                this->m_effectManager->EnterState(0x41, 1, 0);
            }
        }
    } else if (this->IsBelowHalfBaseSpeed()) {
        this->m_effectManager->EnterState(0x31, 1, 0);

        if (this->m_movement->m_flags & MOVEMENTFLAG_FORWARD) {
            this->m_effectManager->EnterState(0x32, 1, 0);
        }

        if (this->m_movement->m_flags & MOVEMENTFLAG_BACKWARD) {
            this->m_effectManager->EnterState(0x33, 1, 0);
        }

        if (this->m_movement->m_flags & (MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
            this->m_effectManager->EnterState(0x36, 1, 0);

            if (this->m_movement->m_flags & MOVEMENTFLAG_STRAFE_LEFT) {
                this->m_effectManager->EnterState(0x34, 1, 0);
            }

            if (this->m_movement->m_flags & MOVEMENTFLAG_STRAFE_RIGHT) {
                this->m_effectManager->EnterState(0x35, 1, 0);
            }
        }
    } else if (this->IsRunning()) {
        this->m_effectManager->EnterState(0x37, 1, 0);

        if (this->m_movement->m_flags & MOVEMENTFLAG_FORWARD) {
            this->m_effectManager->EnterState(0x38, 1, 0);
        }

        if (this->m_movement->m_flags & MOVEMENTFLAG_BACKWARD) {
            this->m_effectManager->EnterState(0x39, 1, 0);
        }

        if (this->m_movement->m_flags & (MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) {
            this->m_effectManager->EnterState(0x3C, 1, 0);

            if (this->m_movement->m_flags & MOVEMENTFLAG_STRAFE_LEFT) {
                this->m_effectManager->EnterState(0x3A, 1, 0);
            }

            if (this->m_movement->m_flags & MOVEMENTFLAG_STRAFE_RIGHT) {
                this->m_effectManager->EnterState(0x3B, 1, 0);
            }
        }
    }

    uint32_t flags = this->m_movement->m_flags;

    if (!(flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD))) {
        this->m_effectManager->ExitState(0x31, 1);
        this->m_effectManager->ExitState(0x32, 1);
        this->m_effectManager->ExitState(0x33, 1);
        this->m_effectManager->ExitState(0x37, 1);
        this->m_effectManager->ExitState(0x38, 1);
        this->m_effectManager->ExitState(0x39, 1);
        this->m_effectManager->ExitState(0x3D, 1);
        this->m_effectManager->ExitState(0x3E, 1);
        this->m_effectManager->ExitState(0x3F, 1);
    } else if (flags & MOVEMENTFLAG_FLYING) {
        this->m_effectManager->ExitState(0x37, 1);
        this->m_effectManager->ExitState(0x38, 1);
        this->m_effectManager->ExitState(0x39, 1);
        this->m_effectManager->ExitState(0x31, 1);
        this->m_effectManager->ExitState(0x32, 1);
        this->m_effectManager->ExitState(0x33, 1);
        this->m_effectManager->ExitState((this->m_movement->m_flags & MOVEMENTFLAG_FORWARD) ? 0x3F : 0x3E, 1);
    } else if (this->IsBelowHalfBaseSpeed()) {
        this->m_effectManager->ExitState(0x37, 1);
        this->m_effectManager->ExitState(0x38, 1);
        this->m_effectManager->ExitState(0x39, 1);
        this->m_effectManager->ExitState(0x3D, 1);
        this->m_effectManager->ExitState(0x3E, 1);
        this->m_effectManager->ExitState(0x3F, 1);
        this->m_effectManager->ExitState((this->m_movement->m_flags & MOVEMENTFLAG_FORWARD) ? 0x33 : 0x32, 1);
    } else if (this->IsRunning()) {
        this->m_effectManager->ExitState(0x3D, 1);
        this->m_effectManager->ExitState(0x3E, 1);
        this->m_effectManager->ExitState(0x3F, 1);
        this->m_effectManager->ExitState(0x31, 1);
        this->m_effectManager->ExitState(0x32, 1);
        this->m_effectManager->ExitState(0x33, 1);
        this->m_effectManager->ExitState((this->m_movement->m_flags & MOVEMENTFLAG_FORWARD) ? 0x39 : 0x38, 1);
    }

    flags = this->m_movement->m_flags;

    if (!(flags & (MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT))) {
        this->m_effectManager->ExitState(0x34, 1);
        this->m_effectManager->ExitState(0x35, 1);
        this->m_effectManager->ExitState(0x36, 1);
        this->m_effectManager->ExitState(0x3A, 1);
        this->m_effectManager->ExitState(0x3B, 1);
        this->m_effectManager->ExitState(0x3C, 1);
        this->m_effectManager->ExitState(0x40, 1);
        this->m_effectManager->ExitState(0x41, 1);
        this->m_effectManager->ExitState(0x42, 1);
    } else if (flags & MOVEMENTFLAG_FLYING) {
        this->m_effectManager->ExitState(0x34, 1);
        this->m_effectManager->ExitState(0x35, 1);
        this->m_effectManager->ExitState(0x36, 1);
        this->m_effectManager->ExitState(0x3A, 1);
        this->m_effectManager->ExitState(0x3B, 1);
        this->m_effectManager->ExitState(0x3C, 1);
        this->m_effectManager->ExitState((this->m_movement->m_flags & MOVEMENTFLAG_STRAFE_LEFT) ? 0x41 : 0x40, 1);
    } else if (this->IsBelowHalfBaseSpeed()) {
        this->m_effectManager->ExitState(0x3A, 1);
        this->m_effectManager->ExitState(0x3B, 1);
        this->m_effectManager->ExitState(0x3C, 1);
        this->m_effectManager->ExitState(0x40, 1);
        this->m_effectManager->ExitState(0x41, 1);
        this->m_effectManager->ExitState(0x42, 1);
        this->m_effectManager->ExitState((this->m_movement->m_flags & MOVEMENTFLAG_STRAFE_LEFT) ? 0x35 : 0x34, 1);
    } else if (this->IsRunning()) {
        this->m_effectManager->ExitState(0x34, 1);
        this->m_effectManager->ExitState(0x35, 1);
        this->m_effectManager->ExitState(0x36, 1);
        this->m_effectManager->ExitState(0x40, 1);
        this->m_effectManager->ExitState(0x41, 1);
        this->m_effectManager->ExitState(0x42, 1);
        this->m_effectManager->ExitState((this->m_movement->m_flags & MOVEMENTFLAG_STRAFE_LEFT) ? 0x3B : 0x3A, 1);
    }

    this->m_effectManager->ExitState((this->m_movement->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT)) ? 0x26 : 0x25, 1);
    this->m_effectManager->ExitState((this->m_movement->m_flags & (MOVEMENTFLAG_TURN_LEFT | MOVEMENTFLAG_TURN_RIGHT)) ? 0x30 : 0x2F, 1);

    if (!(this->m_movement->m_flags & MOVEMENTFLAG_ASCENDING)) {
        this->m_effectManager->ExitState(0x6, 1);
    }

    if (!(this->m_movement->m_flags & MOVEMENTFLAG_DESCENDING)) {
        this->m_effectManager->ExitState(0x7, 1);
    }
}

// VERIFY: d02e26a5 auto
// OFFSET: 0x0072AFE0
void CGUnit_C::CancelRangedMode() {
    this->m_animationState &= ~0x4000u;

    if (!CGUnit_C::s_trackingType) {
        this->ClearTrackingTarget(false, true);
    }

    CM2Model* ammoModel = this->m_ammoModel;

    if (ammoModel && ammoModel->m_attachParent) {
        ammoModel->DetachFromParent();
    }

    CM2Model* model = this->m_rangedWeaponModel;

    if (model && model->IsLoaded(0, 0) && model->GetBoneSequenceId(0xFFFFFFFF) != 161) {
        this->m_rangedWeaponModel->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }
}

// VERIFY: 4993dcb7 manual
// OFFSET: 0x0073ED10
void CGUnit_C::MoveEventHappened(uint32_t msgId) {
    switch (msgId) {
        case MSG_MOVE_START_FORWARD:
        case MSG_MOVE_START_BACKWARD:
        case MSG_MOVE_START_STRAFE_LEFT:
        case MSG_MOVE_START_STRAFE_RIGHT:
        case MSG_MOVE_START_SWIM:
        case MSG_MOVE_START_SWIM_CHEAT:
        case MSG_MOVE_START_ASCEND:
        case MSG_MOVE_START_DESCEND:
            this->CancelRangedMode();
            this->UpdateBaseAnimation(false, -1);
            break;

        case MSG_MOVE_STOP:
        case MSG_MOVE_STOP_STRAFE:
        case MSG_MOVE_START_TURN_LEFT:
        case MSG_MOVE_START_TURN_RIGHT:
        case MSG_MOVE_STOP_TURN:
        case MSG_MOVE_TELEPORT:
        case MSG_MOVE_TELEPORT_ACK:
        case MSG_MOVE_STOP_SWIM:
        case MSG_MOVE_TOGGLE_COLLISION_CHEAT:
        case CMSG_FORCE_MOVE_ROOT_ACK:
        case MSG_MOVE_ROOT:
        case SMSG_MOVE_SPLINE_ROOT:
        case MSG_MOVE_STOP_SWIM_CHEAT:
        case MSG_MOVE_STOP_ASCEND:
        case MSG_MOVE_UPDATE_CAN_FLY:
        case CMSG_MOVE_GRAVITY_DISABLE_ACK:
        case CMSG_MOVE_GRAVITY_ENABLE_ACK:
        case SMSG_MOVE_SPLINE_DISABLE_GRAVITY:
        case SMSG_MOVE_SPLINE_ENABLE_GRAVITY:
            this->UpdateBaseAnimation(false, -1);
            break;

        case MSG_MOVE_JUMP:
            this->CancelRangedMode();
            this->PlayUnitSound(UNITSOUNDTYPE_JUMPSTART, 1);

            if (!this->GetVehicleRec() || !this->m_vehicle->HasHighPriorityAnim()) {
                this->PlayBaseAnimation(ANIM_JUMP_START, 0);
            }

            break;

        case MSG_MOVE_SET_RUN_MODE:
        case MSG_MOVE_SET_WALK_MODE:
        case CMSG_FORCE_RUN_SPEED_CHANGE_ACK:
        case CMSG_FORCE_RUN_BACK_SPEED_CHANGE_ACK:
        case CMSG_FORCE_SWIM_SPEED_CHANGE_ACK:
        case CMSG_FORCE_WALK_SPEED_CHANGE_ACK:
        case CMSG_FORCE_SWIM_BACK_SPEED_CHANGE_ACK:
        case CMSG_FORCE_FLIGHT_SPEED_CHANGE_ACK:
        case CMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK:
            if (this->m_movement->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING)) {
                this->UpdateBaseAnimation(false, -1);
            }

            break;

        case CMSG_MOVE_KNOCK_BACK_ACK:
            if (!this->GetVehicleRec() || !this->m_vehicle->HasHighPriorityAnim()) {
                this->PlayBaseAnimation(ANIM_FALL, 0);
            }

            break;

        case CMSG_MOVE_SET_CAN_FLY_ACK:
            if (!(this->m_movementData.m_flags & MOVEMENTFLAG_CAN_FLY)) {
                this->m_animationState &= ~0x800000u;
            }

            break;

        case CMSG_MOVE_SET_FLY:
            if (this->m_movementData.m_flags & MOVEMENTFLAG_FLYING) {
                this->ResetSmoothFacing();

                if (this->GetCurrentTorsoAnimId() == ANIM_FALL || this->GetCurrentMountAnimId() == ANIM_FALL) {
                    this->UpdateBaseAnimation(false, -1);
                    break;
                }

                this->PlayUnitSound(UNITSOUNDTYPE_JUMPSTART, 1);

                if (!this->GetVehicleRec() || !this->m_vehicle->HasHighPriorityAnim()) {
                    this->PlayBaseAnimation(ANIM_JUMP_START, 0);
                }

                this->m_animationState |= 0x800000;
            } else {
                this->PlayUnitSound(UNITSOUNDTYPE_JUMPEND, 1);

                if (!this->GetVehicleRec() || !this->m_vehicle->HasHighPriorityAnim()) {
                    this->PlayBaseAnimation(ANIM_JUMP_LAND_RUN, 0);
                }

                this->m_animationState &= ~0x800000u;
            }

            break;
    }
}

// VERIFY: e6a99cff manual
// OFFSET: 0x007413F0
int32_t CGUnit_C::ProcessLocalMoveEvent(uint32_t eventTime, uint32_t msgId, bool send, float value, uint32_t index, uint64_t transport, uint8_t seat) {
    this->UpdateObjectEffectMovementStates();

    switch (msgId) {
        case MSG_MOVE_STOP:
        case MSG_MOVE_STOP_STRAFE:
        case MSG_MOVE_START_TURN_LEFT:
        case MSG_MOVE_START_TURN_RIGHT:
        case MSG_MOVE_STOP_TURN:
        case MSG_MOVE_SET_RUN_MODE:
        case MSG_MOVE_SET_WALK_MODE:
        case MSG_MOVE_SET_TURN_RATE_CHEAT:
        case MSG_MOVE_SET_TURN_RATE:
        case MSG_MOVE_TOGGLE_COLLISION_CHEAT:
        case MSG_MOVE_SET_FACING:
        case MSG_MOVE_SET_PITCH_RATE_CHEAT:
        case MSG_MOVE_SET_PITCH_RATE:
            break;

        case MSG_MOVE_START_PITCH_UP:
        case MSG_MOVE_START_PITCH_DOWN:
        case MSG_MOVE_STOP_PITCH:
        case MSG_MOVE_SET_PITCH:
            if (!(this->m_movement->m_flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) && !(this->m_movement->m_flags2 & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING)) {
                return 0;
            }

            break;

        default:
            if (!IsAckMessage(msgId) && this->GetClientStandState()) {
                this->ChangeStandState(UNIT_STANDING);
            }

            break;
    }

    int32_t sent = 0;

    if (send) {
        if (msgId >= MSG_MOVE_START_TURN_LEFT && msgId <= MSG_MOVE_STOP_TURN && (this->m_movement->m_flags2 & MOVEMENTFLAG2_FULL_SPEED_TURNING)) {
            sent = this->HandleFloodsafeTurnEvent(eventTime, msgId);
        } else if (msgId >= MSG_MOVE_START_PITCH_UP && msgId <= MSG_MOVE_STOP_PITCH && (this->m_movement->m_flags2 & MOVEMENTFLAG2_FULL_SPEED_PITCHING)) {
            sent = this->HandleFloodsafePitchEvent(eventTime, msgId);
        } else if ((msgId == MSG_MOVE_SET_FACING || msgId == MSG_MOVE_SET_PITCH) && this->IsFacingUnchangedSinceSent()) {
            sent = 0;
        } else {
            if (this->SendMovementUpdate(eventTime, msgId, value, index, transport, seat)) {
                sent = 1;
            }

            if (this->FlushFloodsafeMoveEvents(eventTime)) {
                sent = 1;
            }
        }
    }

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        switch (msgId) {
            case MSG_MOVE_TELEPORT_ACK:
                CGInputControl::GetActive()->UpdateHalted();
                CGInputControl::GetActive()->UpdatePlayer(eventTime, 1);
                this->m_nextBreathCheckTime = OsGetAsyncTimeMs();

                if (this->IsAutoTracking()) {
                    this->ClearTrackingTarget(false, true);
                }

                if (this->m_obj->m_guid == CGWorldFrame::GetActiveCamera()->m_targetGUID) {
                    CGGameUI::ResetCamera(this->m_obj->m_guid);
                }

                break;

            case CMSG_FORCE_MOVE_ROOT_ACK:
            case CMSG_FORCE_MOVE_UNROOT_ACK:
                CGInputControl::GetActive()->UpdatePlayer(eventTime, 1);

                if (!(this->m_movement->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_TURN_LEFT | MOVEMENTFLAG_TURN_RIGHT | MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING))) {
                    this->HandlePendingTrackEvents();
                }

                break;

            case MSG_MOVE_STOP:
            case MSG_MOVE_STOP_STRAFE:
                if (!(this->m_movement->m_flags & (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_TURN_LEFT | MOVEMENTFLAG_TURN_RIGHT | MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING))) {
                    this->HandlePendingTrackEvents();
                }

                break;

            case CMSG_MOVE_SET_FLY:
                CGInputControl::GetActive()->UpdatePlayer(eventTime, 1);
                break;
        }
    }

    this->MoveEventHappened(msgId);
    return sent;
}

// VERIFY: 0d3d04d2 auto
// OFFSET: 0x0073AF00
void CGUnit_C::SetAnimType(uint8_t animTier) {
    if (!this->m_animTier && (animTier == 3 || animTier == 2)) {
        this->PlayBaseAnimation(ANIM_TO_FLY, 0);
    } else if ((this->m_animTier == 3 || this->m_animTier == 2) && !animTier) {
        this->PlayBaseAnimation(ANIM_TO_GROUND, 0);
    }

    this->m_animTier = animTier;
}

// VERIFY: b75f286d auto
// OFFSET: 0x007158C0
void CGUnit_C::RegisterScript(uint32_t flags) {
    if (!this->m_scriptRegistration) {
        ScriptEventsRegisterUnit(this);
    }

    this->m_scriptRegistration |= flags;
}

// VERIFY: b297416b auto
// OFFSET: 0x00715900
void CGUnit_C::UnregisterScript(uint32_t flags) {
    if (this->m_scriptRegistration) {
        this->m_scriptRegistration &= ~flags;

        if (!this->m_scriptRegistration) {
            ScriptEventsUnregisterUnit(this);
        }
    }
}

// VERIFY: 8adb155d auto
// OFFSET: 0x00715B20
void CGUnit_C::StopNamePlateSpellbar() {
    if (this->m_namePlateFrame) {
        this->m_namePlateFrame->SetCastBarValues(0.0f, 0.0f, 0.0f, 0, false, false);
    }
}

// VERIFY: 86d9c5c1 manual
// OFFSET: 0x0071AB20
bool CGUnit_C::IsCastNotInterruptible() const {
    if (!(this->m_animStateFlags & 0x8)) {
        return false;
    }

    bool isPlayer = (this->m_obj->m_type & TYPE_PLAYER) != 0;
    uint32_t interruptSpecial = isPlayer ? 0 : CGSpellBook::m_interruptSpecialSchoolMask;
    uint32_t silenceSpecial = isPlayer ? 0 : CGSpellBook::m_silenceSpecialSchoolMask;

    return CGSpellBook::m_interruptSchoolMask || interruptSpecial || CGSpellBook::m_silenceSchoolMask || silenceSpecial;
}

// VERIFY: e5e2f834 manual
// OFFSET: 0x0071B0C0
bool CGUnit_C::CanAutoDismount() const {
    if (this->m_unit->m_unitFieldFlags & 0x100000) {
        return false;
    }

    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer() || !g_autoDismountCVar->m_intValue) {
        return false;
    }

    return !(this->m_movement->m_flags & MOVEMENTFLAG_FLYING) || g_autoDismountFlyingCVar->m_intValue;
}

// VERIFY: 2732ecd1 manual
// OFFSET: 0x00720E50
void CGUnit_C::UpdateSpellCastBars(uint32_t time) {
    CGNamePlateFrame* namePlate = this->m_namePlateFrame;

    if (!namePlate || this->m_obj->m_guid != CGGameUI::m_lockedTarget) {
        return;
    }

    int32_t channelSpell = this->m_channelSpellId;

    if (channelSpell) {
        uint32_t end = this->m_channelEndTime;
        bool notInterruptible = this->IsCastNotInterruptible();
        namePlate->SetCastBarValues(0.0f, (end - this->m_channelStartTime) * 0.001f, (end - time) * 0.001f, channelSpell, true, notInterruptible);
        return;
    }

    int32_t castSpell = this->m_castingSpellId;

    if (castSpell) {
        uint32_t start = this->m_castStartTime;
        bool notInterruptible = this->IsCastNotInterruptible();
        namePlate->SetCastBarValues(0.0f, (this->m_castEndTime - start) * 0.001f, (time - start) * 0.001f, castSpell, false, notInterruptible);
        return;
    }

    this->StopNamePlateSpellbar();
}

// VERIFY: 756cf89e auto
// OFFSET: 0x00726280
void CGUnit_C::OnChannelUpdate(int32_t remaining) {
    uint32_t duration = this->m_channelEndTime - this->m_channelStartTime;
    uint32_t end = remaining + OsGetAsyncTimeMs();
    this->m_channelEndTime = end;
    this->m_channelStartTime = end - duration;
    this->m_castingSpellId = 0;
    this->m_castStartTime = 0;
    this->m_castEndTime = 0;
    this->UpdateSpellCastBars(FrameTime::s_curTimeMs);
}

// VERIFY: 3a616bc4 manual
// OFFSET: 0x007262E0
bool CGUnit_C::ProcessCastNotInterruptible(const SpellRec* spell) {
    this->m_animStateFlags &= ~0x8u;

    if (spell->m_attributesExG & 0x2000) {
        return false;
    }

    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__) && spell->m_preventionType == 1) {
        bool isPlayer = (this->m_obj->m_type & TYPE_PLAYER) != 0;
        uint32_t interrupt = CGSpellBook::m_interruptSchoolMask;
        uint32_t silence = CGSpellBook::m_silenceSchoolMask;
        uint32_t interruptSpecial = isPlayer ? 0 : CGSpellBook::m_interruptSpecialSchoolMask;
        uint32_t silenceSpecial = !isPlayer ? CGSpellBook::m_silenceSpecialSchoolMask : 0;

        if (!interrupt && !interruptSpecial && !silence && !silenceSpecial) {
            return true;
        }

        uint32_t immunities = this->m_castImmunities;
        uint32_t schoolImmunities = this->m_castSchoolImmunities;

        if (immunities & 0x4000000) {
            interrupt = 0;
            interruptSpecial = 0;
        } else if (schoolImmunities) {
            interrupt &= ~schoolImmunities;
            interruptSpecial &= ~schoolImmunities;
        }

        if (immunities & 0x200) {
            silence = 0;
            silenceSpecial = 0;
        } else if (schoolImmunities) {
            silence &= ~schoolImmunities;
            silenceSpecial &= ~schoolImmunities;
        }

        if (interrupt || interruptSpecial || silence || silenceSpecial) {
            if (this->HasAuraEffect(SPELL_AURA_SCHOOL_IMMUNITY) || this->HasAuraEffect(SPELL_AURA_MECHANIC_IMMUNITY) || this->HasAuraEffect(SPELL_AURA_EFFECT_IMMUNITY) || this->HasAuraEffect(SPELL_AURA_STATE_IMMUNITY)) {
                SpellRec auraSpell;

                for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
                    if (!interrupt && !interruptSpecial && !silence && !silenceSpecial) {
                        break;
                    }

                    AuraData* aura = this->GetAura(i);

                    if (!g_spellDB.GetRecord(aura->m_spellId, &auraSpell)) {
                        continue;
                    }

                    for (uint32_t j = 0; j < 3; j++) {
                        switch (auraSpell.m_effectAura[j]) {
                            case SPELL_AURA_EFFECT_IMMUNITY:
                                if (auraSpell.m_effectMiscValue[j] == 68) {
                                    interrupt = 0;
                                    interruptSpecial = 0;
                                }

                                break;

                            case SPELL_AURA_STATE_IMMUNITY:
                                if (auraSpell.m_effectMiscValue[j] == 27) {
                                    silence = 0;
                                    silenceSpecial = 0;
                                }

                                break;

                            case SPELL_AURA_SCHOOL_IMMUNITY: {
                                uint32_t mask = auraSpell.m_effectMiscValue[j];

                                if (mask & interrupt) {
                                    interrupt &= ~mask;
                                }

                                if (mask & interruptSpecial) {
                                    interruptSpecial &= ~mask;
                                }

                                if (mask & silence) {
                                    silence &= ~mask;
                                }

                                if (mask & silenceSpecial) {
                                    silenceSpecial &= ~mask;
                                }

                                break;
                            }

                            case SPELL_AURA_MECHANIC_IMMUNITY:
                                if (auraSpell.m_effectMiscValue[j] == 9) {
                                    silence = 0;
                                    silenceSpecial = 0;
                                } else if (auraSpell.m_effectMiscValue[j] == 26) {
                                    interrupt = 0;
                                    interruptSpecial = 0;
                                }

                                break;

                            default:
                                break;
                        }
                    }
                }
            }

            if (interrupt || interruptSpecial || silence || silenceSpecial) {
                return false;
            }
        }
    }

    this->m_animStateFlags |= 0x8;
    return true;
}

// VERIFY: bade9591 manual
// OFFSET: 0x00728C80
bool CGUnit_C::IsNotHostileTo(const CGUnit_C* unit) const {
    return this->UnitReaction(unit) >= UNIT_REACTION_NEUTRAL;
}

// VERIFY: 8ddba777 manual
// OFFSET: 0x007293D0
bool CGUnit_C::CanAssist(const CGUnit_C* unit, bool ignoreFlags) const {
    uint32_t flags = unit->m_unit->m_unitFieldFlags;

    if ((flags & 0x2000000) || (!ignoreFlags && (((this->m_unit->m_unitFieldFlags & 0x8) && (flags & 0x100)) || (!(this->m_unit->m_unitFieldFlags & 0x8) && (flags & 0x200))))) {
        return false;
    }

    if (this->UnitReaction(unit) < UNIT_REACTION_FRIENDLY && (!this->m_creatureCacheEntry || !(this->m_creatureCacheEntry->m_typeFlags & 0x4000000))) {
        return false;
    }

    if (unit->m_unit->m_unitFieldFlags & 0x8) {
        CGPlayer_C* mine = static_cast<CGPlayer_C*>(const_cast<CGUnit_C*>(this)->GetControllingPlayer());
        CGPlayer_C* theirs = static_cast<CGPlayer_C*>(const_cast<CGUnit_C*>(unit)->GetControllingPlayer());

        if (mine && theirs && theirs->m_player->m_playerDuelTeam) {
            if (mine->m_player->m_playerDuelArbiter != theirs->m_player->m_playerDuelArbiter || mine->m_player->m_playerDuelTeam != theirs->m_player->m_playerDuelTeam) {
                return false;
            }
        }

        uint8_t bytes = unit->m_unit->m_unitFieldBytes2[1];

        if ((bytes & 0x4) && !(this->m_unit->m_unitFieldBytes2[1] & 0x4)) {
            return false;
        }

        return !(this->m_unit->m_unitFieldBytes2[1] & 0x8) || (bytes & 0x8) || !(bytes & 0x1);
    }

    if (!(this->m_unit->m_unitFieldFlags & 0x8)) {
        return true;
    }

    return ignoreFlags || (unit->m_unit->m_unitFieldBytes2[1] & 0x1) || unit->CanAidPlayers() || unit->IsTreatAsRaidUnit();
}

// VERIFY: bd65a3f6 manual
// OFFSET: 0x00729A70
bool CGUnit_C::CanAttackNow(const CGUnit_C* unit) const {
    if (this->IsDeadOrGhost()) {
        if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
            return false;
        }

        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (!player || !player->GetPossessedUnit()) {
            return false;
        }
    }

    if (this->m_mountDisplayId > 0 && !(this->m_clientFlags & 0x10000000) && !(this->m_creatureCacheEntry && (this->m_creatureCacheEntry->m_typeFlags & 0x800)) && !this->CanAutoDismount()) {
        return false;
    }

    if (unit->m_unit->m_unitFieldHealth <= 0 && !(unit->m_unit->m_unitDynamicFlags & 0x20)) {
        return false;
    }

    return this->CanAttack(unit);
}

// VERIFY: a850eba6 auto
// OFFSET: 0x00729B30
bool CGUnit_C::CanCooperate(const CGUnit_C* unit) const {
    if (unit == this) {
        return false;
    }

    if (this->m_unit->m_unitFieldCharmedby || unit->m_unit->m_unitFieldCharmedby) {
        return false;
    }

    FactionTemplateRec* mine = g_factionTemplateDB.GetRecord(this->m_unit->m_unitFieldFactiontemplate);
    FactionTemplateRec* theirs = g_factionTemplateDB.GetRecord(unit->m_unit->m_unitFieldFactiontemplate);

    if (mine && theirs && mine->m_factionGroup != theirs->m_factionGroup) {
        return false;
    }

    return !this->CanAttack(unit);
}

// VERIFY: 3e3b824a manual
// OFFSET: 0x0072A680
void CGUnit_C::HandleNPCFeedback() {
    if (this->m_obj->m_type & TYPE_PLAYER) {
        return;
    }

    if (SI2::IsPlaying(&this->m_npcInteractSound) || this->IsDeadOrAttachmentDying() || this->GetClientStandState() == UNIT_SLEEPING) {
        return;
    }

    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover || !this->IsNotHostileTo(mover) || !mover->IsNotHostileTo(this)) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || player->IsDeadOrGhost()) {
        return;
    }

    if (this->m_obj->m_guid != CGUnit_C::s_lastNPCSoundGUID) {
        s_lastNPCSoundCount = 0;
    }

    CGUnit_C::s_lastNPCSoundGUID = this->m_obj->m_guid;

    int32_t soundKitId;
    int32_t fileIndex;
    int32_t count = this->GetNewNPCSoundCount(s_lastNPCSoundCount, &soundKitId, &fileIndex);

    if (count == -1) {
        return;
    }

    C3Vector position = this->GetPosition();
    SE2SoundKitProperties properties;
    properties.ResetToDefaults();
    properties.m_fileIndex = fileIndex;
    SI2::PlaySoundKit(soundKitId, &position, &this->m_npcInteractSound, &properties, 0, nullptr, 1, 0);

    if (SI2::IsPlaying(&this->m_npcInteractSound)) {
        s_lastNPCSoundCount = count;
    }
}

// VERIFY: e4a08d14 manual
// OFFSET: 0x0071B7F0
bool CGUnit_C::IsOutdoors() const {
    return World::QueryOutdoors(this->m_worldObject) != 0;
}

// VERIFY: b89ed3c2 auto
// OFFSET: 0x0071C2C0
int32_t CGUnit_C::GetPredictedHealth() const {
    if (g_predictedHealthCVar->m_intValue) {
        return this->m_predictedHealth;
    }

    return this->m_unit->m_unitFieldHealth;
}

// VERIFY: cbdca188 auto
// OFFSET: 0x0071C2E0
int32_t CGUnit_C::GetPredictedPower(POWER_TYPE powerType) const {
    if (g_predictedPowerCVar->m_intValue) {
        return this->m_predictedPower[powerType];
    }

    if (powerType == POWER_TYPE_HEALTH) {
        return this->m_unit->m_unitFieldHealth;
    }

    return this->m_unit->m_unitFieldPower[powerType];
}

// VERIFY: 98a2c20f manual
// OFFSET: 0x0072AAF0
bool CGUnit_C::IgnoresAuraState(const SpellRec* spell) const {
    const CGUnit_C* unit = this;

    while (unit->m_unit->m_unitFieldCreatedby) {
        const CGUnit_C* creator = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(unit->m_unit->m_unitFieldCreatedby, TYPE_UNIT, __FILE__, __LINE__));

        if (!creator) {
            break;
        }

        unit = creator;
    }

    return unit->AffectedByAura(SPELL_AURA_ABILITY_IGNORE_AURASTATE, spell);
}

// VERIFY: 9a4ea4e4 auto
// OFFSET: 0x0072BC20
bool CGUnit_C::CheckStealthRules(const SpellRec* spell, int32_t* reason) const {
    if (!(spell->m_attributes & 0x20000) || (this->m_unit->m_unitFieldBytes1[2] & 0x2) || this->AffectedByAura(SPELL_AURA_MOD_IGNORE_SHAPESHIFT, spell)) {
        return true;
    }

    *reason = 95;
    return false;
}

// VERIFY: 2d1553ba manual
// OFFSET: 0x0072A000
const char* CGUnit_C::GetUnitName(const char** realm, int32_t followOverride) {
    if (followOverride && this->HasAuraEffect(SPELL_AURA_INITIALIZE_IMAGES)) {
        SpellRec spell;

        for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
            AuraData* aura = this->GetAura(i);

            if (!g_spellDB.GetRecord(aura->m_spellId, &spell)) {
                continue;
            }

            for (uint32_t j = 0; j < sizeof(spell.m_effectAura) / sizeof(spell.m_effectAura[0]); j++) {
                if (spell.m_effectAura[j] == SPELL_AURA_INITIALIZE_IMAGES && ((1 << j) & aura->m_flags)) {
                    CGUnit_C* creator = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(aura->m_creator, TYPE_UNIT, __FILE__, __LINE__));

                    if (creator) {
                        return creator->GetUnitName(realm, 0);
                    }

                    return GetObjectNameFromGuid(aura->m_creator);
                }
            }
        }
    }

    if (this->m_obj->m_type & TYPE_PLAYER) {
        WOWGUID requester = this->m_obj->m_guid;
        NameCache* name = g_nameDBCache.GetRecord(this->m_obj->m_guid.m_guid, requester, NameQueryCallback, nullptr, true);

        if (name) {
            if (realm && name->m_realmName[0]) {
                *realm = name->m_realmName;
            }

            return name->m_name;
        }
    } else if (this->m_unit->m_unitFieldPetnumber) {
        WOWGUID requester = this->m_obj->m_guid;
        PetNameCache* pet = g_petNameCache.GetRecord(this->m_unit->m_unitFieldPetnumber, requester, NameQueryCallback, nullptr, true);

        if (pet) {
            if (pet->m_timestamp == this->m_unit->m_unitFieldPetNameTimestamp) {
                return pet->m_name;
            }

            UnitCombatLogInvalidateName(pet->m_name);
            g_petNameCache.Invalidate(this->m_unit->m_unitFieldPetnumber);
            requester = this->m_obj->m_guid;
            g_petNameCache.GetRecord(this->m_unit->m_unitFieldPetnumber, requester, NameQueryCallback, nullptr, true);
        }
    } else if (this->m_creatureCacheEntry) {
        return this->m_creatureCacheEntry->m_name[0];
    }

    const char* text = FrameScript_GetText("UNKNOWNOBJECT", -1, GENDER_NOT_APPLICABLE);

    if (!text || !*text) {
        return "Unknown Being";
    }

    return text;
}

// VERIFY: 0393d565 manual
// OFFSET: 0x0072A290
char* CGUnit_C::GetUnitChosenTitle(char* buffer, uint32_t size, int32_t showTitle, int32_t* lines) {
    if (lines) {
        *lines = 1;
    }

    if (!(this->m_obj->m_type & TYPE_PLAYER) || !showTitle || static_cast<int32_t>(static_cast<CGPlayer_C*>(this)->m_player->m_playerChosenTitle) <= 0) {
        SStrCopy(buffer, this->GetUnitName(nullptr, 1), size);
        return buffer;
    }

    CharTitlesRec* title = g_charTitlesDB.GetRecord(CGUnit_C::GetRecordForTitle(static_cast<CGPlayer_C*>(this)->m_player->m_playerChosenTitle));

    if (!title) {
        return buffer;
    }

    uint32_t sex = this->IsActivePlayer() ? static_cast<CGPlayer_C*>(this)->m_player->m_playerBytes3[0] : this->m_unit->m_unitFieldBytes0[2];
    char format[400];

    if (sex == 1) {
        strcpy(format, strcmp(title->m_name1, "") ? title->m_name1 : title->m_name);
    } else {
        strcpy(format, strcmp(title->m_name, "") ? title->m_name : title->m_name1);
    }

    SStrPrintf(buffer, size, format, this->GetUnitName(nullptr, 1));

    uint8_t medal = static_cast<CGPlayer_C*>(this)->m_player->m_playerBytes3[2];

    if (medal) {
        char token[32];
        SStrPrintf(token, sizeof(token), "PVP_MEDAL%d", medal);
        SStrPack(buffer, "\n", size);
        SStrPack(buffer, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), size);

        if (lines) {
            *lines = 2;
        }
    }

    return buffer;
}

// VERIFY: 3a743645 manual
// OFFSET: 0x00720400
void CGUnit_C::ShowHandItemSpellEffects(int32_t show, int32_t force) {
    SpellRec spell;
    uint32_t flags = this->m_clientFlags;
    int32_t visible = ((flags >> 16) & 0x1) & (this->m_sheatheState == WEAPONMODE_SHEATHEDMODE) & (this->m_castSpellId == 0) & show;

    if (!force && (visible == static_cast<int32_t>((flags >> 14) & 0x1) || !(flags & 0x2000))) {
        return;
    }

    flags &= ~0x2000;
    this->m_clientFlags = flags;

    if (visible) {
        for (uint32_t i = 0; i < (this->m_handItemSpellIds.m_count == 0xFFFFFFFF ? this->m_handItemSpellIds.m_dynamic.m_count : this->m_handItemSpellIds.m_count); i++) {
            if (!this->m_handItemSpellIds[i] || !g_spellDB.GetRecord(this->m_handItemSpellIds[i], &spell)) {
                continue;
            }

            SpellVisualRec visualRec;
            const SpellVisualRec* visual = this->GetAppropriateSpellVisual(&spell, visualRec, 0, 0);

            if (visual && visual->m_stateKit && (visual->m_flags & 0x8)) {
                SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_stateKit);

                if (kit) {
                    if (!(this->m_clientFlags & 0x4000)) {
                        PlaySpellVisuaKitData data(&spell, kit, 2);
                        data.m_playAnim = 0;
                        this->PlaySpellVisualKit(data);
                        this->m_clientFlags |= 0x4000;
                    }

                    this->m_clientFlags |= 0x2000;
                }
            }
        }
    } else {
        this->m_clientFlags = flags & ~0x4000;

        for (uint32_t i = 0; i < (this->m_handItemSpellIds.m_count == 0xFFFFFFFF ? this->m_handItemSpellIds.m_dynamic.m_count : this->m_handItemSpellIds.m_count); i++) {
            if (!this->m_handItemSpellIds[i] || !g_spellDB.GetRecord(this->m_handItemSpellIds[i], &spell)) {
                continue;
            }

            SpellVisualRec visualRec;
            const SpellVisualRec* visual = this->GetAppropriateSpellVisual(&spell, visualRec, 0, 0);

            if (visual && visual->m_stateKit && (visual->m_flags & 0x8) && g_spellVisualKitDB.GetRecord(visual->m_stateKit)) {
                this->ClearSpellEffect(this->m_handItemSpellIds[i], 1);
                this->m_clientFlags |= 0x2000;
            }
        }
    }
}

// VERIFY: f056fdd8 auto
// OFFSET: 0x00726090
void CGUnit_C::SetHandItemSpellEffectsVisible(int32_t visible) {
    uint32_t flags = this->m_clientFlags;

    if (visible == static_cast<int32_t>((flags >> 16) & 0x1)) {
        return;
    }

    if (visible) {
        this->m_clientFlags = flags | 0x10000;
        this->ShowHandItemSpellEffects(visible, 0);
    } else {
        this->m_clientFlags = flags & ~0x10000;
        this->ShowHandItemSpellEffects(visible, 0);
    }
}

// VERIFY: 45efb9de manual
// OFFSET: 0x00741B60
int32_t CGUnit_C::HandleMovementPacket(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        return unit->OnMoveEvent(msgId, time, msg);
    }

    msg->m_read = msg->m_size;
    return 0;
}

// VERIFY: 775607d3 manual
// OFFSET: 0x00741B00
int32_t CGUnit_C::HandleSpeedChangePacket(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        return unit->OnSpeedChangeEvent(msgId, time, msg);
    }

    msg->m_read = msg->m_size;
    return 0;
}

// VERIFY: 07ceea0a manual
// OFFSET: 0x00732450
int32_t Packet_MoveMode(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        return unit->OnMoveModePacket(time, msgId, msg);
    }

    return 1;
}

// VERIFY: 4a4be5b9 manual
// OFFSET: 0x00741C30
int32_t Packet_SplineMoveEffect(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        return unit->OnSplineMoveEffectPacket(msgId);
    }

    msg->m_read = msg->m_size;
    return 0;
}

// VERIFY: 773da535 manual
// OFFSET: 0x00741BC0
int32_t Packet_SplineSpeedChange(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    float value;
    msg->Get(value);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        return unit->OnSplineSpeedChange(msgId, value);
    }

    msg->m_read = msg->m_size;
    return 0;
}

// VERIFY: 8d4c4c67 manual
// OFFSET: 0x0071CAB0
int32_t Packet_MSG_MOVE_TIME_SKIPPED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        msg->m_read = msg->m_size;
        return 0;
    }

    uint32_t skipped = 0;
    msg->Get(skipped);
    unit->m_movementData.OnSkipTime(skipped);
    return 1;
}

// VERIFY: 28b98c1d auto
// OFFSET: 0x00718890
int32_t CGUnit_C::OnTurnStart(uint32_t time, const CMovementStatus& status, int32_t left) {
    if (this->m_unit->m_unitFieldFlags & 0x40000) {
        return 0;
    }

    return this->m_movementData.OnTurnStart(time, status, left);
}

// VERIFY: 8ad39cf4 auto
// OFFSET: 0x007188C0
int32_t CGUnit_C::OnPitchStart(uint32_t time, const CMovementStatus& status, int32_t up) {
    if (this->m_unit->m_unitFieldFlags & 0x40000) {
        return 0;
    }

    return this->m_movementData.OnPitchStart(time, status, up);
}

// VERIFY: 12d095c0 auto
// OFFSET: 0x007188F0
int32_t CGUnit_C::OnMoveUnRoot(uint32_t time, const CMovementStatus& status) {
    this->m_clientFlags |= 0x20000000;
    int32_t result = this->m_movementData.OnMoveUnRoot(time, status);
    this->m_clientFlags &= ~0x20000000;
    return result;
}

// VERIFY: 9c252b1d auto
// OFFSET: 0x007189A0
int32_t CGUnit_C::OnUnitMoveEvent(uint32_t time, const CMovementStatus& status) {
    if (this->m_vehiclePassenger && (status.m_moveExtraFlags & 0x2000)) {
        this->VehiclePassengerStartWaitingForVehicleTransitionAnim(status.m_transportGuid.m_guid, status.m_transportSeat);
    }

    return this->m_movementData.OnTeleport(time, status);
}

// VERIFY: 2f664d59 auto
// OFFSET: 0x007187F0
int32_t CGUnit_C::OnKnockBack(uint32_t time, const CMovementStatus& status, CDataStore* msg) {
    C2Vector direction = { 0.0f, 0.0f };
    float speed;
    float verticalSpeed;
    msg->Get(direction.x);
    msg->Get(direction.y);
    msg->Get(speed);
    msg->Get(verticalSpeed);
    return this->m_movementData.OnLaunch(time, status, direction, speed, verticalSpeed);
}

// VERIFY: b95a85e3 auto
// OFFSET: 0x00740D30
int32_t CGUnit_C::OnMoveEvent(NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CMovementStatus status;
    *msg >> status;
    uint32_t now = OsGetAsyncTimeMs();
    int32_t changed;

    switch (msgId) {
        case MSG_MOVE_START_FORWARD:
            changed = this->m_movementData.OnMoveStart(now, status, 1);
            break;
        case MSG_MOVE_START_BACKWARD:
            changed = this->m_movementData.OnMoveStart(now, status, 0);
            break;
        case MSG_MOVE_STOP:
            changed = this->m_movementData.OnMoveStop(now, status);
            break;
        case MSG_MOVE_START_STRAFE_LEFT:
            changed = this->m_movementData.OnStrafeStart(now, status, 1);
            break;
        case MSG_MOVE_START_STRAFE_RIGHT:
            changed = this->m_movementData.OnStrafeStart(now, status, 0);
            break;
        case MSG_MOVE_STOP_STRAFE:
            changed = this->m_movementData.OnStrafeStop(now, status);
            break;
        case MSG_MOVE_JUMP:
            changed = this->m_movementData.OnJump(now, status);
            break;
        case MSG_MOVE_START_TURN_LEFT:
            changed = this->OnTurnStart(now, status, 1);
            break;
        case MSG_MOVE_START_TURN_RIGHT:
            changed = this->OnTurnStart(now, status, 0);
            break;
        case MSG_MOVE_STOP_TURN:
            changed = this->m_movementData.OnTurnStop(now, status);
            break;
        case MSG_MOVE_START_PITCH_UP:
            changed = this->OnPitchStart(now, status, 1);
            break;
        case MSG_MOVE_START_PITCH_DOWN:
            changed = this->OnPitchStart(now, status, 0);
            break;
        case MSG_MOVE_STOP_PITCH:
            changed = this->m_movementData.OnPitchStop(now, status);
            break;
        case MSG_MOVE_SET_RUN_MODE:
            changed = this->m_movementData.OnSetRunMode(now, status, 1);
            break;
        case MSG_MOVE_SET_WALK_MODE:
            changed = this->m_movementData.OnSetRunMode(now, status, 0);
            break;
        case MSG_MOVE_TELEPORT:
            changed = this->OnUnitMoveEvent(now, status);
            break;
        case MSG_MOVE_FALL_LAND:
        case MSG_MOVE_HEARTBEAT:
            changed = this->m_movementData.OnHeartBeat(now, status);
            break;
        case MSG_MOVE_START_SWIM:
        case MSG_MOVE_START_SWIM_CHEAT:
            changed = this->m_movementData.OnStartSwim(now, status);
            break;
        case MSG_MOVE_STOP_SWIM:
        case MSG_MOVE_STOP_SWIM_CHEAT:
            changed = this->m_movementData.OnStopSwim(now, status);
            break;
        case MSG_MOVE_TOGGLE_COLLISION_CHEAT:
            return 1;
        case MSG_MOVE_SET_FACING:
            changed = this->m_movementData.OnSetFacing(now, status);
            break;
        case MSG_MOVE_SET_PITCH:
            changed = this->m_movementData.OnSetPitch(now, status);
            break;
        case MSG_MOVE_ROOT:
            changed = this->m_movementData.OnMoveRoot(now, status);
            break;
        case MSG_MOVE_UNROOT:
            changed = this->OnMoveUnRoot(now, status);
            break;
        case MSG_MOVE_KNOCK_BACK:
            changed = this->OnKnockBack(now, status, msg);
            break;
        case MSG_MOVE_HOVER:
            changed = this->m_movementData.OnMoveHover(now, status);
            break;
        case MSG_MOVE_FEATHER_FALL:
            changed = this->m_movementData.OnSetFeatherFall(now, status);
            break;
        case MSG_MOVE_WATER_WALK:
            changed = this->m_movementData.OnSetWaterWalk(now, status);
            break;
        case MSG_MOVE_UPDATE_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY:
            changed = this->m_movementData.OnUpdateCanTransitionBetweenSwimAndFly(now, status);
            break;
        case MSG_MOVE_START_ASCEND:
            changed = this->m_movementData.OnStartAscendDescend(now, status, 1);
            break;
        case MSG_MOVE_STOP_ASCEND:
            changed = this->m_movementData.OnStopAscend(now, status);
            break;
        case MSG_MOVE_START_DESCEND:
            changed = this->m_movementData.OnStartAscendDescend(now, status, 0);
            break;
        case MSG_MOVE_UPDATE_CAN_FLY:
            changed = this->m_movementData.OnUpdateCanFly(now, status);
            break;
        case MSG_MOVE_GRAVITY_CHNG:
            changed = this->m_movementData.OnGravityChange(now, status);
            break;
        default:
            return 0;
    }

    if (changed) {
        this->MoveEventHappened(msgId);
        this->UpdateBaseAnimation(0, -1);
    }

    return 1;
}

// VERIFY: 0814618b auto
// OFFSET: 0x007406A0
int32_t CGUnit_C::OnSpeedChangeEvent(NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CMovementStatus status;
    *msg >> status;
    uint32_t now = OsGetAsyncTimeMs();
    float value;
    msg->Get(value);

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        switch (msgId) {
            case MSG_MOVE_SET_RUN_SPEED:
                this->m_movementData.OnRunSpeedChange(now, value);
                return 1;
            case MSG_MOVE_SET_RUN_BACK_SPEED:
                this->m_movementData.OnRunBackSpeedChange(now, value);
                return 1;
            case MSG_MOVE_SET_WALK_SPEED:
                this->m_movementData.OnWalkSpeedChange(now, value);
                return 1;
            case MSG_MOVE_SET_SWIM_SPEED:
                this->m_movementData.OnSwimSpeedChange(now, value);
                return 1;
            case MSG_MOVE_SET_SWIM_BACK_SPEED:
                this->m_movementData.OnSwimBackSpeedChange(now, value);
                return 1;
            case MSG_MOVE_SET_TURN_RATE:
                this->m_movementData.OnTurnRateChange(now, value);
                return 1;
            case MSG_MOVE_SET_FLIGHT_SPEED:
                this->m_movementData.OnFlightSpeedChange(now, value);
                return 1;
            case MSG_MOVE_SET_FLIGHT_BACK_SPEED:
                this->m_movementData.OnFlightBackSpeedChange(now, value);
                return 1;
            case MSG_MOVE_SET_PITCH_RATE:
                this->m_movementData.OnPitchRateChange(now, value);
                return 1;
            case MSG_MOVE_SET_COLLISION_HGT:
                this->m_movementData.OnSetCollisionHeight(now, value);
                return 1;
            default:
                return 0;
        }
    }

    int32_t changed;

    switch (msgId) {
        case MSG_MOVE_SET_RUN_SPEED:
            changed = this->m_movementData.OnRunSpeedChangeOther(now, status, value);
            break;
        case MSG_MOVE_SET_RUN_BACK_SPEED:
            changed = this->m_movementData.OnRunBackSpeedChangeOther(now, status, value);
            break;
        case MSG_MOVE_SET_WALK_SPEED:
            changed = this->m_movementData.OnWalkSpeedChangeOther(now, status, value);
            break;
        case MSG_MOVE_SET_SWIM_SPEED:
            changed = this->m_movementData.OnSwimSpeedChangeOther(now, status, value);
            break;
        case MSG_MOVE_SET_SWIM_BACK_SPEED:
            changed = this->m_movementData.OnSwimBackSpeedChangeOther(now, status, value);
            break;
        case MSG_MOVE_SET_TURN_RATE:
            changed = this->m_movementData.OnTurnRateChangeOther(now, status, value);
            break;
        case MSG_MOVE_SET_FLIGHT_SPEED:
            changed = this->m_movementData.OnFlightSpeedChangeOther(now, status, value);
            break;
        case MSG_MOVE_SET_FLIGHT_BACK_SPEED:
            changed = this->m_movementData.OnFlightBackSpeedChangeOther(now, status, value);
            break;
        case MSG_MOVE_SET_PITCH_RATE:
            changed = this->m_movementData.OnPitchRateChangeOther(now, status, value);
            break;
        case MSG_MOVE_SET_COLLISION_HGT:
            changed = this->m_movementData.OnSetCollisionHeightOther(now, status, value);
            break;
        default:
            return 0;
    }

    if (changed) {
        this->MoveEventHappened(msgId);
        this->UpdateBaseAnimation(0, -1);
    }

    return 1;
}

// VERIFY: 7d02f602 auto
// OFFSET: 0x007307A0
int32_t CGUnit_C::OnMoveModePacket(uint32_t time, NETMESSAGE msgId, CDataStore* msg) {
    uint32_t now = OsGetAsyncTimeMs();
    uint32_t ackCounter = 0;
    msg->Get(ackCounter);
    float value;

    switch (msgId) {
        case MSG_MOVE_TELEPORT_ACK:
            this->OnTeleportAck(now, ackCounter, msg);
            return 1;
        case SMSG_MOVE_SET_WATER_WALK:
            this->m_movementData.OnWaterWalkLocal(now, ackCounter, 1);
            return 1;
        case SMSG_MOVE_SET_LAND_WALK:
            this->m_movementData.OnWaterWalkLocal(now, ackCounter, 0);
            return 1;
        case SMSG_FORCE_RUN_SPEED_CHANGE: {
            uint8_t forced;
            msg->Get(forced);
            msg->Get(value);

            if (forced) {
                s_forcedRunSpeed = value;
            }

            this->m_movementData.OnRunSpeedChangeLocal(now, ackCounter, value);
            return 1;
        }
        case SMSG_FORCE_RUN_BACK_SPEED_CHANGE:
            msg->Get(value);
            this->m_movementData.OnRunBackSpeedChangeLocal(now, ackCounter, value);
            return 1;
        case SMSG_FORCE_SWIM_SPEED_CHANGE:
            msg->Get(value);
            this->m_movementData.OnSwimSpeedChangeLocal(now, ackCounter, value);
            return 1;
        case SMSG_FORCE_MOVE_ROOT:
            this->m_movementData.OnRootLocal(now, ackCounter, 1);
            return 1;
        case SMSG_FORCE_MOVE_UNROOT:
            this->m_movementData.OnRootLocal(now, ackCounter, 0);
            return 1;
        case SMSG_MOVE_KNOCK_BACK:
            this->OnKnockBackLocal(now, ackCounter, msg);
            return 1;
        case SMSG_MOVE_SET_FEATHER_FALL:
            this->m_movementData.OnFeatherFallLocal(now, ackCounter, 1);
            return 1;
        case SMSG_MOVE_SET_NORMAL_FALL:
            this->m_movementData.OnFeatherFallLocal(now, ackCounter, 0);
            return 1;
        case SMSG_MOVE_SET_HOVERING:
            this->m_movementData.OnHoverLocal(now, ackCounter, 1);
            return 1;
        case SMSG_MOVE_UNSET_HOVERING:
            this->m_movementData.OnHoverLocal(now, ackCounter, 0);
            return 1;
        case SMSG_FORCE_WALK_SPEED_CHANGE:
            msg->Get(value);
            this->m_movementData.OnWalkSpeedChangeLocal(now, ackCounter, value);
            return 1;
        case SMSG_FORCE_SWIM_BACK_SPEED_CHANGE:
            msg->Get(value);
            this->m_movementData.OnSwimBackSpeedChangeLocal(now, ackCounter, value);
            return 1;
        case SMSG_FORCE_TURN_RATE_CHANGE:
            msg->Get(value);
            this->m_movementData.OnTurnRateChangeLocal(now, ackCounter, value);
            return 1;
        case SMSG_MOVE_ENABLE_TRANSITION_BETWEEN_SWIM_AND_FLY:
            this->m_movementData.OnCanTransitionBetweenSwimAndFlyLocal(now, ackCounter, 1);
            return 1;
        case SMSG_MOVE_DISABLE_TRANSITION_BETWEEN_SWIM_AND_FLY:
            this->m_movementData.OnCanTransitionBetweenSwimAndFlyLocal(now, ackCounter, 0);
            return 1;
        case SMSG_MOVE_SET_CAN_FLY:
            this->m_movementData.OnCanFlyLocal(now, ackCounter, 1);
            return 1;
        case SMSG_MOVE_UNSET_CAN_FLY:
            this->m_movementData.OnCanFlyLocal(now, ackCounter, 0);
            return 1;
        case SMSG_FORCE_FLIGHT_SPEED_CHANGE:
            msg->Get(value);
            this->m_movementData.OnFlightSpeedChangeLocal(now, ackCounter, value);
            return 1;
        case SMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE:
            msg->Get(value);
            this->m_movementData.OnFlightBackSpeedChangeLocal(now, ackCounter, value);
            return 1;
        case SMSG_FORCE_PITCH_RATE_CHANGE:
            msg->Get(value);
            this->m_movementData.OnPitchRateChangeLocal(now, ackCounter, value);
            return 1;
        case SMSG_MOVE_DISABLE_GRAVITY:
            this->m_movementData.OnDisableGravityLocal(now, ackCounter, 1);
            return 1;
        case SMSG_MOVE_ENABLE_GRAVITY:
            this->m_movementData.OnDisableGravityLocal(now, ackCounter, 0);
            return 1;
        case SMSG_MOVE_SET_COLLISION_HGT:
            msg->Get(value);
            this->m_movementData.OnSetCollisionHeightLocal(now, ackCounter, value);
            return 1;
        default:
            return 1;
    }
}

// VERIFY: 8726701f auto
// OFFSET: 0x00740BA0
int32_t CGUnit_C::OnSplineMoveEffectPacket(NETMESSAGE msgId) {
    CMovement_C* movement = &this->m_movementData;
    movement->FlushMoveQueue(0, 0);

    switch (msgId) {
        case SMSG_MOVE_SPLINE_ROOT:
            movement->OnSplineRoot();
            break;
        case SMSG_MOVE_SPLINE_UNROOT:
            movement->OnSplineUnRoot();
            break;
        case SMSG_MOVE_SPLINE_SET_FEATHER_FALL:
            movement->OnFeatherFall(1);
            break;
        case SMSG_MOVE_SPLINE_SET_NORMAL_FALL:
            movement->OnFeatherFall(0);
            break;
        case SMSG_MOVE_SPLINE_SET_HOVER:
            movement->OnSplineToggleHover(1);
            break;
        case SMSG_MOVE_SPLINE_UNSET_HOVER:
            movement->OnSplineToggleHover(0);
            break;
        case SMSG_MOVE_SPLINE_SET_WATER_WALK:
            movement->OnWaterWalk(1);
            break;
        case SMSG_MOVE_SPLINE_SET_LAND_WALK:
            movement->OnWaterWalk(0);
            break;
        case SMSG_MOVE_SPLINE_START_SWIM:
            movement->OnSplineStartSwim();
            break;
        case SMSG_MOVE_SPLINE_STOP_SWIM:
            movement->OnSplineStopSwim();
            break;
        case SMSG_MOVE_SPLINE_SET_RUN_MODE:
            movement->OnSetRunMode(1);
            break;
        case SMSG_MOVE_SPLINE_SET_WALK_MODE:
            movement->OnSetRunMode(0);
            break;
        case SMSG_MOVE_SPLINE_SET_FLYING:
            movement->SplineToggleEnableFlying(1);
            break;
        case SMSG_MOVE_SPLINE_UNSET_FLYING:
            movement->SplineToggleEnableFlying(0);
            break;
        case SMSG_MOVE_SPLINE_DISABLE_GRAVITY:
            movement->OnSplineEnableGravity(0);
            break;
        case SMSG_MOVE_SPLINE_ENABLE_GRAVITY:
            movement->OnSplineEnableGravity(1);
            break;
        default:
            return 0;
    }

    this->MoveEventHappened(msgId);
    this->UpdateBaseAnimation(0, -1);
    return 1;
}

// VERIFY: db2bbe05 auto
// OFFSET: 0x00740A60
int32_t CGUnit_C::OnSplineSpeedChange(NETMESSAGE msgId, float value) {
    switch (msgId) {
        case SMSG_MOVE_SPLINE_SET_RUN_SPEED:
            this->m_movementData.OnRunSpeedChange(value);
            break;
        case SMSG_MOVE_SPLINE_SET_RUN_BACK_SPEED:
            this->m_movementData.OnRunBackSpeedChange(value);
            break;
        case SMSG_MOVE_SPLINE_SET_SWIM_SPEED:
            this->m_movementData.OnSwimSpeedChange(value);
            break;
        case SMSG_MOVE_SPLINE_SET_WALK_BACK_SPEED:
            this->m_movementData.OnWalkSpeedChange(value);
            break;
        case SMSG_MOVE_SPLINE_SET_SWIM_BACK_SPEED:
            this->m_movementData.OnSwimBackSpeedChange(value);
            break;
        case SMSG_MOVE_SPLINE_SET_TURN_RATE:
            this->m_movementData.OnTurnRateChange(value);
            break;
        case SMSG_MOVE_SPLINE_SET_FLIGHT_SPEED:
            this->m_movementData.OnFlightSpeedChange(value);
            break;
        case SMSG_MOVE_SPLINE_SET_FLIGHT_BACK_SPEED:
            this->m_movementData.OnFlightBackSpeedChange(value);
            break;
        case SMSG_MOVE_SPLINE_SET_PITCH_RATE:
            this->m_movementData.OnPitchRateChange(value);
            break;
        default:
            return 0;
    }

    this->MoveEventHappened(msgId);
    this->UpdateBaseAnimation(0, -1);
    return 1;
}

// VERIFY: 412af708 manual
// OFFSET: 0x007152B0
C3Vector* ReadPackedVector3(CDataStore* msg, const C3Vector& base, C3Vector* out) {
    uint32_t packed;
    msg->Get(packed);
    int32_t z = static_cast<int32_t>(packed) >> 22;
    float y = static_cast<float>(static_cast<int32_t>((packed >> 11) << 21) >> 21) * 0.25f;
    out->x = base.x - static_cast<float>(static_cast<int32_t>(packed << 21) >> 21) * 0.25f;
    out->y = base.y - y;
    out->z = base.z - 0.25f * static_cast<float>(z);
    return out;
}

// VERIFY: 6eed8c82 manual
// OFFSET: 0x007180C0
C3Vector* ComputeTransportRelativeMovement(const C3Vector& position, C3Vector* points, uint32_t& count, WOWGUID transport) {
    C3Vector pos = position;

    if (transport.m_guid) {
        C44Matrix matrix;
        MovementGetTransportMtxX(transport, &matrix, WOWGUID{ 0 }, __FILE__, __LINE__);
        pos *= matrix.AffineInverse();
    }

    if (count == 1) {
        if ((points[0].x - pos.x) * (points[0].x - pos.x) + (points[0].y - pos.y) * (points[0].y - pos.y) + (points[0].z - pos.z) * (points[0].z - pos.z) >= 0.00077160494f) {
            points[-1] = pos;
            count++;
            return points - 1;
        }

        count = 0;
        return nullptr;
    }

    int32_t segments = count - 1;
    int32_t i = 0;
    int32_t behind = 0;
    int32_t ahead = 0;

    for (; i < segments; i++) {
        C3Vector dir = points[i + 1] - points[i];
        float d = -(dir.z * pos.z + dir.y * pos.y + dir.x * pos.x);
        float start = dir.z * points[i].z + dir.y * points[i].y + dir.x * points[i].x + d;
        float end = dir.z * points[i + 1].z + dir.y * points[i + 1].y + dir.x * points[i + 1].x + d;

        if (start >= 0.0f && end >= 0.0f) {
            ahead++;
        } else if (start <= 0.0f && end <= 0.0f) {
            behind++;
        } else {
            break;
        }
    }

    if (ahead == segments) {
        if ((points[0].x - pos.x) * (points[0].x - pos.x) + (points[0].y - pos.y) * (points[0].y - pos.y) >= 1.0f) {
            points[-1] = pos;
            count++;
            return points - 1;
        }

        return points;
    }

    if (behind == segments) {
        C3Vector& last = points[count - 1];

        if ((last.x - pos.x) * (last.x - pos.x) + (last.z - pos.z) * (last.z - pos.z) + (last.y - pos.y) * (last.y - pos.y) >= 0.00077160494f) {
            points[0] = pos;
            points[1] = points[count - 1];
            count = 2;
            return points;
        }

        count = 0;
        return nullptr;
    }

    if (segments <= 0) {
        return points;
    }

    for (i = 0; i < static_cast<int32_t>(count - 1); i++) {
        C3Vector dir = points[i + 1] - points[i];
        float scale = 1.0f / sqrtf(dir.z * dir.z + dir.y * dir.y + dir.x * dir.x);
        dir.x *= scale;
        dir.y *= scale;
        dir.z *= scale;
        float d = -(pos.z * dir.z + dir.y * pos.y + dir.x * pos.x);
        float start = dir.z * points[i].z + dir.y * points[i].y + dir.x * points[i].x + d;
        float end = dir.x * points[i + 1].x + (dir.y * points[i + 1].y + dir.z * points[i + 1].z) + d;

        if (start > 0.0f || end < 0.0f) {
            if (start >= 0.0f && end >= 0.0f) {
                count -= i;
                C3Vector* result = &points[i];

                if ((result->y - pos.y) * (result->y - pos.y) + (result->x - pos.x) * (result->x - pos.x) < 1.0f) {
                    return result;
                }

                result[-1] = pos;
                count++;
                return result - 1;
            }

            continue;
        }

        float t = start / (start - end);
        C3Vector projected = { (points[i + 1].x - points[i].x) * t + points[i].x, (points[i + 1].y - points[i].y) * t + points[i].y, (points[i + 1].z - points[i].z) * t + points[i].z };

        if ((points[i + 1].y - projected.y) * (points[i + 1].y - projected.y) + (points[i + 1].z - projected.z) * (points[i + 1].z - projected.z) + (points[i + 1].x - projected.x) * (points[i + 1].x - projected.x) >= 0.00077160494f) {
            points[i] = projected;
        } else if ((points[i + 1].x - pos.x) * (points[i + 1].x - pos.x) + (points[i + 1].y - pos.y) * (points[i + 1].y - pos.y) + (points[i + 1].z - pos.z) * (points[i + 1].z - pos.z) >= 0.00077160494f) {
            points[i] = pos;
            count -= i;
            return &points[i];
        } else if (++i == segments) {
            count = 0;
            return nullptr;
        }

        count -= i;
        return &points[i];
    }

    return points;
}

// VERIFY: 6f9ef4df manual
// OFFSET: 0x00718930
void CGUnit_C::FaceTarget(const WOWGUID& target, int32_t flush) {
    CGObject_C* object = ClntObjMgrObjectPtr(target, TYPE_OBJECT, __FILE__, __LINE__);

    if (object) {
        C3Vector targetPosition = object->GetPosition();
        C3Vector position = this->GetPosition();
        this->m_movementData.SetFacingAndFlush(CalculateFacingTo(&position, &targetPosition), flush);
    }
}

// VERIFY: 12cc204b manual
// OFFSET: 0x0073F590
int32_t CGUnit_C::HandleMonsterMovePacket(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        msg->m_read = msg->m_size;
        return 0;
    }

    SmartGUID transport;
    uint8_t seat;

    if (msgId == SMSG_MONSTER_MOVE_TRANSPORT) {
        *msg >> transport;
        msg->Get(seat);
    } else {
        transport.m_guid = 0;
        seat = 0xFF;
    }

    uint8_t exitVoluntary = 0;
    msg->Get(exitVoluntary);
    unit->SetVehicleExitVoluntary(exitVoluntary);

    if (!unit->DeferVehicleTransitionSpline(msg, transport.m_guid, seat)) {
        unit->OnMonsterMove(msg, msgId, transport.m_guid, seat, 1);
    }

    return 1;
}

// VERIFY: 6e6a311e manual
// OFFSET: 0x0073C8E0
void CGUnit_C::OnMonsterMove(CDataStore* msg, NETMESSAGE msgId, uint64_t transport, uint8_t seat, int32_t flush) {
    this->m_clientFlags |= 0x20000000;

    if (flush) {
        this->m_movementData.FlushMoveQueue(0, 0);
    }

    this->m_movementData.ForceSetTransport(WOWGUID{ transport }, seat, 1);
    this->m_clientFlags &= ~0x20000000;

    if (this->m_movementData.m_transportGuid.m_guid != transport) {
        return;
    }

    this->m_movementData.SetAnimTierInTrans(0);
    C3Vector start = { 0.0f, 0.0f, 0.0f };
    msg->Get(start.x);
    msg->Get(start.y);
    msg->Get(start.z);
    uint32_t splineId;
    msg->Get(splineId);
    uint8_t faceType;
    msg->Get(faceType);
    C3Vector facePoint = { 0.0f, 0.0f, 0.0f };
    WOWGUID faceTarget = WOWGUID{ 0 };
    float faceAngle = 0.0f;

    switch (faceType) {
        case 1: {
            C3Vector position = this->GetRawPosition();

            if (CGUnit_C::s_cvPathingDistTolerance->m_floatValue * CGUnit_C::s_cvPathingDistTolerance->m_floatValue > (start.x - position.x) * (start.x - position.x) + (start.y - position.y) * (start.y - position.y) + (start.z - position.z) * (start.z - position.z)) {
                this->m_movementData.OnSplineSnap(splineId, start, 0, 1);
                this->UpdateBaseAnimation(0, -1);
                return;
            }

            break;
        }
        case 2:
            *msg >> facePoint;
            break;
        case 3:
            msg->Get(faceTarget.m_guid);
            break;
        case 4:
            msg->Get(faceAngle);
            break;
    }

    float verticalAcceleration = 0.0f;
    uint32_t parabolicStart = 0;
    uint32_t animStart = 0;
    uint8_t animType = 0;
    uint32_t duration;
    uint32_t splineFlags;
    uint32_t waypoints;

    if (faceType == 1) {
        duration = 0;
        splineFlags = 0x1000;
        waypoints = 1;
    } else {
        msg->Get(splineFlags);

        if (splineFlags & 0x200000) {
            msg->Get(animType);
            msg->Get(animStart);
        }

        msg->Get(duration);

        if (splineFlags & 0x800) {
            msg->Get(verticalAcceleration);
            msg->Get(parabolicStart);
        }

        msg->Get(waypoints);
    }

    C3Vector* points = static_cast<C3Vector*>(alloca(sizeof(C3Vector) * (waypoints + 4)));
    C3Vector direction = { 0.0f, 0.0f, 0.0f };
    C3Vector end = { 0.0f, 0.0f, 0.0f };
    float facing = this->GetRawFacing();
    direction.x = cosf(facing);
    direction.y = sinf(facing);
    uint32_t pointCount;

    if (faceType == 1) {
        C3Vector position = this->GetRawPosition();
        points[0] = position - direction;
        points[1] = position;
        points[2] = start;
        points[3] = start;
        end = start;
        pointCount = 4;
    } else if (!(splineFlags & 0x42000)) {
        *msg >> end;
        C3Vector* path = &points[2];
        path[0] = start;
        uint32_t count = 1;

        if (waypoints <= 1) {
            if ((end.x - start.x) * (end.x - start.x) + (end.y - start.y) * (end.y - start.y) + (end.z - start.z) * (end.z - start.z) > 0.00077160494f) {
                path[1] = end;
                count = 2;
            }
        } else {
            C3Vector middle = { (start.x + end.x) * 0.5f, (end.y + start.y) * 0.5f, 0.5f * (end.z + start.z) };

            for (uint32_t i = 0; i < waypoints - 1; i++) {
                C3Vector offset;
                ReadPackedVector3(msg, middle, &offset);
                path[count] = offset;
                count++;
            }

            path[count] = end;
            count++;
        }

        C3Vector position = this->GetPosition();
        C3Vector* trimmed = ComputeTransportRelativeMovement(position, path, count, WOWGUID{ transport });
        C3Vector* spline = trimmed - 1;

        if (!spline || !count) {
            pointCount = 0;
        } else {
            spline[count + 1] = spline[count];
            pointCount = count + 2;
            points = spline;
            spline[0] = spline[1] - (spline[2] - spline[1]);
        }
    } else {
        C3Vector position = this->GetRawPosition();
        points[0] = position - direction;
        points[1] = position;
        uint32_t count = 2;
        C3Vector point = { 0.0f, 0.0f, 0.0f };
        msg->Get(point.x);
        msg->Get(point.y);
        msg->Get(point.z);

        if ((point.x - position.x) * (point.x - position.x) + (point.y - position.y) * (point.y - position.y) + (point.z - position.z) * (point.z - position.z) >= 0.00077160494f) {
            points[2] = point;
            count = 3;
        }

        for (uint32_t i = 1; i < waypoints; i++) {
            msg->Get(point.x);
            msg->Get(point.y);
            msg->Get(point.z);
            points[count] = point;
            count++;
        }

        if (splineFlags & 0x80000) {
            points[count] = points[2];
            count++;
            points[count] = points[3];
            end = points[2];
        } else {
            points[count] = point;
            end = point;
        }

        pointCount = count + 1;
    }

    bool started = false;

    if (pointCount > 3) {
        float length = 0.0f;

        for (uint32_t i = pointCount - 3; i > 0; i--) {
            length += sqrtf((points[i + 1].x - points[i].x) * (points[i + 1].x - points[i].x) + (points[i + 1].y - points[i].y) * (points[i + 1].y - points[i].y) + (points[i + 1].z - points[i].z) * (points[i + 1].z - points[i].z));
        }

        if (length > 0.16666667f) {
            float speed = this->m_movement->m_runSpeed * 4.0f;

            if (speed <= 28.0f) {
                speed = 28.0f;
            }

            if (splineFlags & 0x42000) {
                speed = 50.0f;
            }

            if (duration) {
                float required = length / (static_cast<float>(duration) * 0.001f);

                if (required < speed) {
                    speed = required;
                }
            }

            if (speed > 0.00000095367432f) {
                float time = length / speed * 1000.0f;
                duration = CMath::fint_(time) > 1 ? CMath::fint_(time) : 1;

                if (this->m_movementData.OnSpline(points, pointCount, duration, splineFlags, splineId)) {
                    switch (faceType) {
                        case 2:
                            this->m_movementData.SetSplineFacePoint(facePoint);
                            break;
                        case 3:
                            this->m_movementData.SetSplineFaceTarget(faceTarget);
                            break;
                        case 4:
                            this->m_movementData.SetSplineFaceAngle(faceAngle);
                            break;
                    }

                    if (splineFlags & 0x800) {
                        this->m_movementData.SetSplineParabolic(verticalAcceleration, parabolicStart);
                    } else if (splineFlags & 0x200000) {
                        if ((this->m_animTier == 3 && (animType == 0 || animType == 2)) || (this->m_animTier == 2 && animType == 0)) {
                            M2SequenceInfo info;
                            info.Clear();
                            this->GetObjectModel()->GetSequenceInfo(463, 0, info);

                            if (duration > info.m_duration + animStart) {
                                animStart = duration - info.m_duration;
                            }
                        }

                        this->m_movementData.SetSplineAnimation(animType, animStart);
                    }

                    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
                        CGInputControl::GetActive()->UpdatePlayer(OsGetAsyncTimeMs(), 1);
                    }

                    started = true;
                }
            }
        }
    }

    if (!started) {
        switch (faceType) {
            case 2: {
                C3Vector target = this->m_movementData.CPassenger::GetPosition(facePoint);
                C3Vector position = this->GetPosition();
                this->m_movementData.SetFacingAndFlush(CalculateFacingTo(&position, &target), flush);
                break;
            }
            case 3:
                this->FaceTarget(faceTarget, flush);
                break;
            case 4:
                this->m_movementData.SetFacingAndFlush(this->m_movement->GetFacing(faceAngle), flush);
                break;
        }

        this->m_movementData.OnSplineSnap(splineId, end, splineFlags, flush);
    }

    if (this->m_vehicle && this->m_vehicle->m_vehicleRec) {
        this->m_vehicle->UpdateWorldMatrix();
    }

    if (this->m_obj->m_type & TYPE_PLAYER) {
        static_cast<CGPlayer_C*>(this)->ChangeStandState(UNIT_STANDING);
    }

    this->CancelRangedMode();

    if (this->m_fishingLineEffect) {
        this->m_fishingLineEffect->Release();
        this->m_fishingLineEffect = nullptr;
    }

    this->UpdateBaseAnimation(0, -1);
}

// VERIFY: c47943c5 auto
// OFFSET: 0x00724E70
int32_t CGUnit_C::SendMovementUpdate(uint32_t msgId) {
    return this->SendMovementUpdate(MovementGetIdleTime(), msgId, 0.0f, 0, 0, 0xFF);
}

// VERIFY: 7db16a4b manual
// OFFSET: 0x00729010
void CGUnit_C::SetActiveMover(uint64_t guid) {
    if (CGUnit_C::s_activeMover.m_guid == guid) {
        return;
    }

    uint32_t now = OsGetAsyncTimeMs();
    CGUnit_C* oldMover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (oldMover) {
        if (oldMover->m_obj->m_guid == CGUnit_C::s_activeMover && CGUnit_C::s_trackingType != TRACKTYPE_NONE) {
            oldMover->ClearTrackingTarget(false, true);
        }

        oldMover->m_movementData.m_flags &= ~0x80000000;

        if (CGUnit_C::s_activeMover != ClntObjMgrGetActivePlayer()) {
            oldMover->m_movementData.m_flags &= ~0x200;
        }

        if (!oldMover->m_movement->m_spline || (oldMover->m_movement->m_spline->m_flags & 0x400)) {
            oldMover->m_movementData.OnUnsetActiveMover();

            if (oldMover->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
                oldMover->SendMovementUpdate(CMSG_MOVE_NOT_ACTIVE_MOVER);
            }
        }
    }

    CGUnit_C::s_activeMover = WOWGUID{ guid };

    if (guid) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_SET_ACTIVE_MOVER));
        msg.Put(guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (mover) {
        if (CGUnit_C::s_activeMover != ClntObjMgrGetActivePlayer()) {
            CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

            if (player->m_movement->m_flags & 0x200) {
                mover->m_movementData.m_flags |= 0x200;
            }
        }

        mover->m_movementData.OnSetActiveMover();
        CGInputControl::GetActive()->UpdatePlayer(now, 1);
    }

    CVehiclePassenger_C::OnSetActiveMover(mover);
}

// VERIFY: e6e292bd manual
// OFFSET: 0x0072AEC0
void CGUnit_C::OnMovementInitiated(bool clearTracking, bool keepLoot) {
    if (clearTracking && this->m_obj->m_guid == CGUnit_C::s_activeMover && CGUnit_C::s_trackingType != TRACKTYPE_NONE && !(CGUnit_C::s_trackingFlags & 0x1)) {
        this->ClearTrackingTarget(false, true);
    }

    if (!keepLoot && this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player && player->IsLooting()) {
            CGGameUI::CloseLoot(true, true, false);
        }
    }
}

// VERIFY: 106d473a manual
// OFFSET: 0x0072D1B0
void CGUnit_C::OnKnockBackLocal(uint32_t time, uint32_t ackCounter, CDataStore* msg) {
    this->OnMovementInitiated(true, false);
    C2Vector direction = { 0.0f, 0.0f };
    float speed;
    float verticalSpeed;
    msg->Get(direction.x);
    msg->Get(direction.y);
    msg->Get(speed);
    msg->Get(verticalSpeed);

    if (this->m_obj->m_guid != CGUnit_C::s_activeMover) {
        CGUnit_C::SetActiveMover(this->m_obj->m_guid.m_guid);
    }

    this->m_movementData.OnLaunchLocal(time, ackCounter, direction, speed, verticalSpeed);
}

// VERIFY: d50d3ba0 manual
// OFFSET: 0x0072D2D0
void CGUnit_C::OnTeleportAck(uint32_t time, uint32_t ackCounter, CDataStore* msg) {
    CMovementStatus status;
    *msg >> status;

    if (ClntObjMgrGetPlayerType()) {
        CDataStore response;
        response.Put(static_cast<uint32_t>(MSG_MOVE_TELEPORT_ACK));
        response.Put(this->m_obj->m_guid.m_guid);
        response.Put(ackCounter);
        response.Put(time);
        response.Finalize();
        ClientServices::Send2(&response);
        return;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera) {
        camera->CameraTeleported(status.m_facing);
    }

    if (this->m_obj->m_guid != CGUnit_C::s_activeMover) {
        CGUnit_C::SetActiveMover(this->m_obj->m_guid.m_guid);
    }

    this->OnMovementInitiated(true, false);
    this->m_movementData.OnTeleportLocal(time, ackCounter, status);
}

// VERIFY: e5352685 auto
// OFFSET: 0x00755020
void TargetAttacker(const WOWGUID& attacker) {
    if (!CGGameUI::m_lockedTarget.m_guid) {
        CGGameUI::Target(attacker);
    }
}

// VERIFY: 62ed69aa manual
// OFFSET: 0x00755050
ATTACKROUNDINFO& ATTACKROUNDINFO::operator=(const ATTACKROUNDINFO& other) {
    memcpy(this, &other, sizeof(ATTACKROUNDINFO));
    return *this;
}

// VERIFY: f7a53fa2 auto
// OFFSET: 0x00755130
void CGUnit_C::PlayAttackerRound(const ATTACKROUNDINFO& info) {
    if (this->m_unit->m_unitFieldHealth <= 0 || (info.m_hitInfo & 0x40000)) {
        return;
    }

    bool skipAnimation = info.m_meleeSpellId != 0;
    uint32_t anim = 16;

    if (info.m_hitInfo & 0x4) {
        const CGUnitVirtualItem* offhand = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, false);

        if (offhand && offhand->m_classID == 2) {
            anim = (offhand->m_subclassID == 15) + 87;
        } else {
            anim = 117;
        }
    } else {
        const CGUnitVirtualItem* mainhand = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, false);

        if (mainhand && mainhand->m_classID == 2) {
            uint32_t oneHanded = 18 - (this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, false) != nullptr);

            switch (mainhand->m_subclassID) {
                case 0:
                case 4:
                case 7:
                case 11:
                case 14:
                    anim = 17;
                    break;
                case 1:
                case 5:
                case 8:
                case 12:
                    anim = oneHanded;
                    break;
                case 6:
                case 10:
                case 17:
                case 20:
                    anim = 19;
                    break;
                case 15:
                    anim = 85;
                    break;
            }
        }
    }

    if (!skipAnimation) {
        this->PlayBaseAnimation(anim, 0);
    }

    if (info.m_victimState) {
        this->PlayUnitSound(static_cast<UNITSOUNDTYPE>((info.m_hitInfo >> 9) & 0x1), 0);
    }
}

// VERIFY: dbeaaa32 manual
// OFFSET: 0x007300A0
int32_t Packet_AuraUpdate(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        msg->m_read = msg->m_size;
        return 0;
    }

    unit->OnAuraUpdate(msg, msgId == SMSG_AURA_UPDATE_ALL);
    return 1;
}

// VERIFY: dadb3184 manual
// OFFSET: 0x0071A670
SkillLineAbilityRec* CGUnit_C::LookupAbility(int32_t spellId) {
    if (!this->m_creatureCacheEntry) {
        return SpellTableLookupAbility(this->m_unit->m_unitFieldBytes0[0], this->m_unit->m_unitFieldBytes0[1], spellId);
    }

    SkillLineAbilityRec* ability = nullptr;
    CreatureFamilyRec* family = g_creatureFamilyDB.GetRecord(this->m_creatureCacheEntry->m_family);

    if (family) {
        for (uint32_t i = 0; i < 2; i++) {
            ability = SpellTableLookupPetAbility(family->m_skillLine[i], spellId);

            if (ability) {
                break;
            }
        }
    }

    return ability;
}

// VERIFY: 000a9432 auto
// OFFSET: 0x0072ED80
void CGUnit_C::SpellInterrupted(uint8_t castCount, int32_t spellId) {
    if (!this->m_castSpellId || spellId != this->m_castSpellId || castCount != this->m_castCount) {
        return;
    }

    this->m_animationState &= ~0x400;

    if (this->m_sheatheState == WEAPONMODE_RANGEDMODE) {
        this->CancelRangedMode();
    }

    if (this->m_fishingLineEffect) {
        this->m_fishingLineEffect->Release();
        this->m_fishingLineEffect = nullptr;
    }

    this->ReleaseMissiles();

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && this->IsActiveMoverTracking(s_spellInterruptTrackingMask)) {
        this->ClearTrackingTarget(false, true);
    }
}

// VERIFY: b8fe5935 auto
// OFFSET: 0x00726200
void CGUnit_C::ResetUICastingSpell() {
    this->m_castingSpellId = 0;
    this->m_castStartTime = 0;
    this->m_castEndTime = 0;
    this->UpdateSpellCastBars(FrameTime::s_curTimeMs);
}

// VERIFY: 102fec10 manual
// OFFSET: 0x00716510
void AuraData::Unpack(CGUnit_C* unit, uint32_t time, CDataStore* msg) {
    msg->Get(reinterpret_cast<uint32_t&>(this->m_spellId));

    if (!this->m_spellId) {
        return;
    }

    msg->Get(this->m_flags);
    msg->Get(this->m_level);
    msg->Get(this->m_stackCount);
    SmartGUID creator;

    if (this->m_flags & 0x8) {
        creator.m_guid = unit->m_obj->m_guid.m_guid;
    } else {
        *msg >> creator;
    }

    this->m_creator = creator;

    if (!(this->m_flags & 0x20)) {
        this->m_duration = 0;
        this->m_endTime = 0;
        return;
    }

    msg->Get(reinterpret_cast<uint32_t&>(this->m_duration));
    uint32_t remaining;
    msg->Get(remaining);
    this->m_endTime = remaining + time;

    if (!this->m_endTime) {
        this->m_endTime = 1;
    }
}

// VERIFY: 32bbf304 manual
// OFFSET: 0x0072F360
void CGUnit_C::AddAuraSlot(uint8_t slot) {
    uint32_t count = this->m_auras.m_count == 0xFFFFFFFF ? this->m_auras.m_dynamic.m_count : this->m_auras.m_count;

    if (count > slot) {
        return;
    }

    this->m_auras.SetCount(slot + 1);

    for (uint32_t i = count; i <= slot; i++) {
        this->m_auras[i].m_spellId = 0;
    }

    for (uint32_t j = 0; j < sizeof(this->m_sortedAuras) / sizeof(this->m_sortedAuras[0]); j++) {
        this->m_sortedAuras[j].SetCount(slot + 1);

        for (uint32_t i = count; i <= slot; i++) {
            this->m_sortedAuras[j][i].m_auraIndex = 256;
            this->m_sortedAuras[j][i].m_spellId = 0;
        }
    }

    this->m_handItemSpellIds.SetCount(slot + 1);

    for (uint32_t i = count; i <= slot; i++) {
        this->m_handItemSpellIds[i] = 0;
    }
}

// VERIFY: 5bbee06d manual
// OFFSET: 0x0072F5D0
void CGUnit_C::OnAuraUpdate(CDataStore* msg, bool updateAll) {
    uint32_t time = OsGetAsyncTimeMs();
    uint32_t oldCount = this->GetAuraCount();
    AuraData* oldAuras = static_cast<AuraData*>(alloca(sizeof(AuraData) * oldCount));
    memcpy(oldAuras, &this->m_auras[0], sizeof(AuraData) * oldCount);
    uint8_t updated[256];

    if (updateAll) {
        memset(&this->m_auras[0], 0, sizeof(AuraData) * this->GetAuraCount());
        memset(updated, 1, sizeof(updated));
    } else {
        memset(updated, 0, sizeof(updated));
    }

    while (!msg->IsRead()) {
        uint8_t slot;
        msg->Get(slot);
        updated[slot] = 1;
        this->AddAuraSlot(slot);
        this->m_auras[slot].Unpack(this, time, msg);
    }

    this->m_clientFlags &= ~0x40000;
    this->UpdateAuraEffectMask();
    bool changed = false;

    for (uint32_t i = 0; i < oldCount; i++) {
        if (!updated[i]) {
            continue;
        }

        int32_t oldSpellId = oldAuras[i].m_spellId;
        uint32_t oldActive = oldAuras[i].m_flags & 0x7;
        int32_t spellId = this->m_auras[i].m_spellId;
        bool wasActive = oldSpellId && oldActive;
        bool stillActive = oldSpellId == spellId && (this->m_auras[i].m_flags & 0x7);

        if ((wasActive && !stillActive) || !spellId) {
            if (updateAll) {
                SpellRec spell;

                if (g_spellDB.GetRecord(oldSpellId, &spell)) {
                    this->RemoveAuraEffect(i, &spell);
                }
            } else {
                this->OnAuraRemoved(i, oldAuras[i].m_flags >> 7, &oldAuras[i], oldSpellId);
                changed = true;
            }
        }
    }

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        if (!updated[i]) {
            continue;
        }

        bool hadOld = i < oldCount;
        int32_t oldSpellId = hadOld ? oldAuras[i].m_spellId : 0;
        uint32_t oldActive = hadOld ? oldAuras[i].m_flags & 0x7 : 0;
        AuraData& aura = this->m_auras[i];
        int32_t spellId = aura.m_spellId;
        bool same = oldSpellId == spellId && oldActive;

        if (!spellId || !(aura.m_flags & 0x7)) {
            continue;
        }

        if (updateAll) {
            SpellRec spell;

            if (g_spellDB.GetRecord(spellId, &spell)) {
                this->AddAuraEffect(i, &spell);
            }

            if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
                CGActionBar::OnAuraChanged(spellId);

                if (CGSpellBook::IsTrackingSpell(spellId)) {
                    CGMinimapFrame::SetTrackingSpell(spellId);
                }
            }
        } else if (!same) {
            this->OnAuraAdded(i, aura.m_flags >> 7, &aura, spellId);
            changed = true;
        } else if (hadOld) {
            if (oldAuras[i].m_stackCount != aura.m_stackCount) {
                UnitCombatLogAuraDose(this, &aura, spellId, oldAuras[i].m_stackCount, aura.m_stackCount, aura.m_flags >> 7);
            } else if (oldAuras[i].m_endTime != aura.m_endTime) {
                UnitCombatLogAuraRefreshed(this, &aura, aura.m_spellId, aura.m_flags >> 7);
            }
        }
    }

    WOWGUID guid = this->m_obj->m_guid;
    int32_t arenaIndex;
    int32_t arenaOpponent = CGBattlefieldInfo::GetArenaOpponentIndex(guid, &arenaIndex);

    if ((changed && (guid == ClntObjMgrGetActivePlayer() || this->m_unit->m_unitFieldCharmedby == ClntObjMgrGetActivePlayer() || *this->GetCharmerOrCreatorGuid() == ClntObjMgrGetActivePlayer() || this->m_obj->m_guid == CGGameUI::m_lockedTarget || this->m_obj->m_guid == CGGameUI::m_focusTarget)) || arenaOpponent) {
        CGSpellBook::UpdateUsable();

        if (this->m_obj->m_guid == CGGameUI::m_lockedTarget || this->m_obj->m_guid == CGGameUI::m_focusTarget || arenaOpponent) {
            SpellRec spell;

            if ((this->m_castingSpellId && g_spellDB.GetRecord(this->m_castingSpellId, &spell)) || (this->m_channelSpellId && g_spellDB.GetRecord(this->m_channelSpellId, &spell))) {
                if (spell.m_preventionType == 1) {
                    bool notInterruptible = this->IsCastNotInterruptible();

                    if (notInterruptible != this->ProcessCastNotInterruptible(&spell)) {
                        this->UpdateSpellCastBars(FrameTime::s_curTimeMs);
                        Script_SendUnitSignal(this->m_obj->m_guid, this->IsCastNotInterruptible() + 334);
                    }
                }
            }
        }
    }

    Script_SendUniqueSignal(this->m_obj->m_guid, SCRIPT_EVENT_UNIT_AURA);
}

// VERIFY: 80b6ec9d manual
// OFFSET: 0x00736D30
void CGUnit_C::SetSheatheState(uint32_t showWeapons, int32_t instant, int32_t noSend) {
    if (this->IsPlayingWeaponAnim()) {
        showWeapons = CGUnit_C::ResolveSheathState(showWeapons, this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, false), this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, false));
    }

    if (showWeapons == WEAPONMODE_RANGEDMODE && (this->m_obj->m_type & TYPE_PLAYER)) {
        ChrClassesRec* classRec = g_chrClassesDB.GetRecord(this->m_unit->m_unitFieldBytes0[1]);

        if (!classRec || (classRec->m_flags & 0x8)) {
            return;
        }
    }

    if (showWeapons == this->m_sheatheState) {
        return;
    }

    if (this->m_creatureCacheEntry && (this->m_creatureCacheEntry->m_typeFlags & 0x10000000) && !noSend) {
        return;
    }

    if (!this->m_worldModel) {
        return;
    }

    if (instant) {
        this->UnsetKeyBoneSequence(this->m_worldModel, 3, 1, 1, 0);
        this->UnsetKeyBoneSequence(this->m_worldModel, 2, 1, 1, 0);
    }

    WEAPONMODE oldState = this->m_sheatheState;
    this->m_sheatheState = static_cast<WEAPONMODE>(showWeapons);
    this->m_previousSheatheState = oldState;

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && !noSend) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_SET_SHEATHED));
        msg.Put(static_cast<uint32_t>(this->m_sheatheState));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    if (showWeapons) {
        this->ShowHandItemSpellEffects(0, 0);
    }

    if (instant) {
        this->SetSheatheStateInstant();
    } else {
        this->SetSheatheStateAnimate();
    }
}

// VERIFY: 4d7f7127 manual
// OFFSET: 0x00722CF0
void CGUnit_C::ModifyPredictedPower(POWER_TYPE powerType, int32_t delta) {
    if (delta < 0 && (this->m_unit->m_unitFieldFlags2 & 0x100)) {
        return;
    }

    if (powerType != this->m_unit->m_unitFieldBytes0[3]) {
        return;
    }

    int32_t value = delta + this->m_predictedPower[powerType];

    if (value < 0) {
        this->SetPredictedPower(powerType, 0);
        return;
    }

    int32_t maximum = powerType == POWER_TYPE_HEALTH ? this->m_unit->m_unitFieldMaxhealth : this->m_unit->m_unitFieldMaxpower[powerType];

    if (value > maximum) {
        if (powerType == POWER_TYPE_HEALTH) {
            this->SetPredictedPower(POWER_TYPE_HEALTH, this->m_unit->m_unitFieldMaxhealth);
            return;
        }

        value = this->m_unit->m_unitFieldMaxpower[powerType];
    }

    this->SetPredictedPower(powerType, value);
}

// VERIFY: 6c7ddb06 manual
// OFFSET: 0x00717C50
void CGUnit_C::InitActiveMover(const WOWGUID& guid) {
    CGUnit_C::s_activeMover = guid;

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SET_ACTIVE_MOVER));
    msg.Put(CGUnit_C::s_activeMover.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);

    uint32_t time = OsGetAsyncTimeMs();
    CGInputControl::GetActive()->UpdatePlayer(time, 1);
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (mover->m_movement->m_flags & 0xC0100F) {
        mover->m_movementData.UpdateHeartbeatTimerA(time);
    }

    WOWGUID transportGuid = WOWGUID{ mover->GetTransportGUID() };

    if (transportGuid) {
        CGGameObject_C* transport = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(transportGuid, TYPE_GAMEOBJECT, __FILE__, __LINE__));

        if (transport) {
            MovementSetTransportUpdateTime(transport->m_typeData->GetTransportUpdateTime());
        }
    }

    CVehiclePassenger_C::OnSetActiveMover(mover);
}

// VERIFY: 5f5b6429 manual
// OFFSET: 0x0071C930
void CGUnit_C::SetLocalClientControl(bool control) {
    if (control) {
        this->m_clientFlags |= 0xC000400;
    } else {
        this->m_clientFlags &= ~0x400;

        if (this->m_unit->m_unitFieldCharmedby) {
            this->m_clientFlags |= 0x1000;
        }

        this->m_movementData.ResetFallOnControlLoss();
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CGGameUI::OnClientControlChanged((this->m_clientFlags >> 10) & 1);

        if (this->m_clientFlags & 0x400) {
            this->VehiclePassengerUpdateWorldCameraStateInstantly();
        }
    }
}

// VERIFY: 39443c6e auto
// OFFSET: 0x0071D010
int32_t CGUnit_C::InitializeExtendedDisplay(CGPlayer_C* player, int32_t hasDisplayInfo) {
    this->m_characterComponent = CCharacterComponent::AllocComponent();
    ComponentData data;

    if (hasDisplayInfo) {
        data.m_preferences.m_raceID = this->m_displayInfoExtra->m_displayRaceID;
        data.m_preferences.m_sexID = this->m_displayInfoExtra->m_displaySexID;
    } else {
        data.m_preferences.m_raceID = this->m_unit->m_unitFieldBytes0[0];
        data.m_preferences.m_sexID = this->m_unit->m_unitFieldBytes0[2];
    }

    data.m_preferences.m_classId = this->m_unit->m_unitFieldBytes0[1];

    if (player) {
        data.m_preferences.m_skinID = player->m_player->m_playerBytes[0];
        data.m_preferences.m_faceId = player->m_player->m_playerBytes[1];
        data.m_preferences.m_hairStyleId = player->m_player->m_playerBytes[2];
        data.m_preferences.m_hairColorId = player->m_player->m_playerBytes[3];
        data.m_preferences.m_facialHairId = player->m_player->m_playerBytes2[0];
    } else {
        data.m_preferences.m_skinID = this->m_displayInfoExtra->m_skinID;
        data.m_preferences.m_faceId = this->m_displayInfoExtra->m_faceID;
        data.m_preferences.m_hairStyleId = this->m_displayInfoExtra->m_hairStyleID;
        data.m_preferences.m_hairColorId = this->m_displayInfoExtra->m_hairColorID;
        data.m_preferences.m_facialHairId = this->m_displayInfoExtra->m_facialHairID;

        if (!CGPlayer_C::IsXRayVisionActive() || !this->UsesNpcBakedTexture()) {
            if (!this->m_displayInfoExtra) {
                return 0;
            }

            const char* bakeName = this->m_displayInfoExtra->m_bakeName;

            if (!bakeName || !*bakeName) {
                return 0;
            }

            data.m_flags |= 0x1;
            SStrPrintf(data.m_npcSkinTexture, sizeof(data.m_npcSkinTexture), "%s%s", "Textures\\BakedNpcTextures\\", bakeName);
        }
    }

    data.m_model = this->m_worldModel;
    data.m_flags ^= (data.m_flags ^ (static_cast<uint32_t>(this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) << 1)) & 0x2;
    data.m_model->m_refCount++;

    if (player) {
        CCharacterComponent::ValidateComponentData(&data, COMPONENT_CONTEXT_1);
    } else {
        CCharacterComponent::ValidateComponentData(&data, COMPONENT_CONTEXT_2);
    }

    this->m_characterComponent->Init(&data, nullptr);
    return 1;
}

// VERIFY: a4a439c3 auto
// OFFSET: 0x00716E20
void CGUnit_C::ApplyComponentItemsFromEffects() {
    for (CEffect* effect = this->m_effectList; effect;) {
        CEffect* next = effect->m_next;

        if (effect->m_flags & 0x80000) {
            effect->ApplyItemVisual();
        }

        effect = next;
    }
}

// VERIFY: e06874b6 manual
// OFFSET: 0x00730100
int32_t CGUnit_C::InitializeComponent() {
    if (!this->m_worldModel || !this->m_worldModel->IsLoaded(0, 0)) {
        return 0;
    }

    this->m_clientFlags &= ~0x400000;

    if (this->m_characterComponent) {
        CCharacterComponent::FreeComponent(this->m_characterComponent);
        this->m_characterComponent = nullptr;
    }

    if (this->m_unit->m_unitFieldFlags2 & 0x10) {
        if (!(this->m_clientFlags & 0x20000)) {
            this->RequestMirrorImageData();
        }

        this->m_clientFlags |= 0x400000;
        return 0;
    }

    if (this->HasBakedNpcTexture()) {
        this->InitializeExtendedDisplay(static_cast<CGPlayer_C*>(this), 1);
    } else if (this->m_displayInfoExtra) {
        int32_t result = this->InitializeExtendedDisplay(nullptr, 1);

        if (!result) {
            return result;
        }
    } else if ((this->m_obj->m_type & TYPE_PLAYER) && (this->m_modelData->m_flags & 0x4)) {
        this->InitializeExtendedDisplay(static_cast<CGPlayer_C*>(this), 0);
    }

    if (!(this->m_obj->m_type & TYPE_PLAYER) || !static_cast<CGPlayer_C*>(this)->RefreshVisibleItems()) {
        if ((!CGPlayer_C::IsXRayVisionActive() || !this->UsesNpcBakedTexture()) && this->m_characterComponent && this->m_displayInfoExtra) {
            for (uint32_t i = 0; i < 11; i++) {
                int32_t displayId = this->m_displayInfoExtra->m_npcitemDisplay[i];

                if (displayId) {
                    this->m_characterComponent->AddItem(static_cast<ITEM_SLOT>(i), displayId, 0);
                }
            }
        }

        this->AddHandItem(0);
        this->AddHandItem(1);
        this->AddHandItem(2);
    }

    this->ApplyComponentItemsFromEffects();
    CGGameUI::UnitModelUpdate(this->m_obj->m_guid, 3);
    return 1;
}


// VERIFY: caa0a36b manual
// OFFSET: 0x0071AAB0
void CGUnit_C::BowModelLoaded(CM2Model* model, void* arg) {
    if (model->HasEvent('TTW$') && model->HasEvent('BTW$')) {
        model->m_fFlags |= 0x20;
        model->m_drawCallback = &CDrawBowString::UnitDrawBowStringCB;
        model->m_drawArg = arg;
        return;
    }

    if (!(model->m_fFlags & 0x1)) {
        model->WaitForLoad(nullptr);
    }

    SysMsgPrintf(SYSMSG_WARNING, "Bow model '%s' equipped without proper events.  Bowstring won't draw.", model->m_shared->m_data->m_name.Data());
}

// VERIFY: 2d7e1e97 manual
// OFFSET: 0x0072B7F0
int32_t CGUnit_C::ShowRangedWeapon(bool sheathed, bool* rightHand) {
    if (this->m_modelData->m_flags & 0x10) {
        return 0;
    }

    const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false);

    if (!item) {
        return 0;
    }

    this->CancelRangedMode();
    bool isRightHand = item->m_inventoryType == INVTYPE_RANGEDRIGHT || item->m_inventoryType == INVTYPE_THROWN;

    if (rightHand) {
        *rightHand = isRightHand;
    }

    if (this->m_rangedWeaponModel) {
        this->m_rangedWeaponModel->m_fFlags |= 0x20;
        this->m_rangedWeaponModel->m_drawCallback = nullptr;
        this->m_rangedWeaponModel->m_drawArg = nullptr;
        this->m_rangedWeaponModel->Release();
    }

    if (!sheathed) {
        this->m_worldModel->DetachAllChildrenById(ATTACH_HANDRIGHT);
        this->m_worldModel->DetachAllChildrenById(ATTACH_HANDLEFT);
        this->m_worldModel->DetachAllChildrenById(ATTACH_SHIELD);
    }

    ItemDisplayInfoRec displayRec;
    const ItemDisplayInfoRec* displayRecPtr = nullptr;

    if (this->GetVirtualItemDisplayRec(VIRTUAL_MONSTER_SLOT_RANGED, displayRec)) {
        displayRecPtr = &displayRec;
    }

    GEOCOMPONENTLINKS link = CCharacterComponent::AddHandItem(this->m_worldModel, displayRecPtr, INVSLOT_RANGED, static_cast<SHEATHE_TYPE>(item->m_sheatheType), sheathed, false, isRightHand, 0);

    if (this->m_characterComponent) {
        this->m_characterComponent->SetItemSlotUsingAttachSlot(static_cast<ITEM_SLOT>(-1), link);
    }

    CM2Model* child = this->m_worldModel->m_attachList;

    while (child && child->m_attachmentId != static_cast<uint32_t>(link)) {
        child = child->m_attachNext;
    }

    this->m_rangedWeaponModel = child;

    if (child) {
        child->m_refCount++;
        this->m_rangedWeaponModel->SetSequenceCallback(nullptr, 0);

        if (item->m_classID == 2 && item->m_subclassID == 2) {
            this->m_rangedWeaponModel->SetLoadedCallback(&CGUnit_C::BowModelLoaded, this);
        }
    }

    if (this->m_characterComponent && (this->m_obj->m_type & TYPE_PLAYER)) {
        if (sheathed) {
            this->m_characterComponent->RemoveItem(ITEMSLOT_11);
        } else if (!isRightHand) {
            CGItem_C* ammo = static_cast<CGPlayer_C*>(this)->m_bag.FindItemOfClass(11, 0, 2);

            if (ammo) {
                this->m_characterComponent->AddItem(ITEMSLOT_11, ammo->GetDisplayID(), 0);
            }
        }
    }

    bool track = false;

    if (this->m_castSpellId && (this->m_animationState & 0x400)) {
        SpellRec spell;

        if (g_spellDB.GetRecord(this->m_castSpellId, &spell) && (spell.m_attributes & 0x400000) && (!(spell.m_facingCasterFlags & 0x1) || this->IsInFrontOfTargetWithGuid(CGGameUI::m_lockedTarget))) {
            track = true;
        }
    } else if (this->m_unit->m_unitChannelSpell) {
        SpellRec spell;

        if (g_spellDB.GetRecord(this->m_unit->m_unitChannelSpell, &spell) && (spell.m_attributesEx & 0x4000)) {
            track = true;
        }
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && track) {
        this->SetTrackingTarget(CGGameUI::m_lockedTarget, TRACKTYPE_SPELLPRECAST);
    }

    if (sheathed) {
        this->ShowHandItemSpellEffects(1, 0);
    }

    return 1;
}

// VERIFY: d991186c manual
// OFFSET: 0x007310A0
int32_t CGUnit_C::SheatheHandItem(uint32_t hand, int32_t sheathe) {
    CM2Model* model = this->m_worldModel;

    if (!model) {
        return 0;
    }

    const CGUnitVirtualItem* item = this->GetVirtualItem(static_cast<VIRTUAL_MONSTER_SLOT>(hand), false);

    if (!item) {
        return 0;
    }

    bool mainHand = !hand || item->m_inventoryType == INVTYPE_RANGEDRIGHT || item->m_inventoryType == INVTYPE_THROWN;
    GEOCOMPONENTLINKS handLink;

    switch (hand) {
        case 0: {
            handLink = ATTACH_HANDRIGHT;
            break;
        }

        case 1: {
            handLink = item->m_inventoryType != INVTYPE_SHIELD ? ATTACH_HANDLEFT : ATTACH_SHIELD;
            break;
        }

        case 2: {
            handLink = mainHand ? ATTACH_HANDRIGHT : ATTACH_HANDLEFT;
            break;
        }

        default: {
            return 0;
        }
    }

    GEOCOMPONENTLINKS from;
    GEOCOMPONENTLINKS to;

    if (sheathe) {
        from = handLink;
        to = CCharacterComponent::GetSheatheLink(static_cast<SHEATHE_TYPE>(item->m_sheatheType), mainHand);
    } else {
        to = handLink;
        from = CCharacterComponent::GetSheatheLink(static_cast<SHEATHE_TYPE>(item->m_sheatheType), mainHand);
    }

    bool skipAdd = false;

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && CGBarberShop::m_barberShopEnabled && item->m_inventoryType == INVTYPE_SHIELD) {
        skipAdd = true;
        to = ATTACH_NONE;
    }

    CM2Model* child = model->m_attachList;

    while (child && child->m_attachmentId != static_cast<uint32_t>(from)) {
        child = child->m_attachNext;
    }

    if (child) {
        child->m_refCount++;
        child->DetachFromParent();

        if (to != ATTACH_NONE) {
            child->AttachToParent(model, to, nullptr, 0);
        }

        child->Release();

        if (handLink) {
            COMP_HAND_SLOT slot = static_cast<COMP_HAND_SLOT>(handLink != ATTACH_HANDRIGHT);

            if (to == handLink) {
                CCharacterComponent::ComponentCloseFingers(model, slot);
            } else {
                CCharacterComponent::ComponentOpenFingers(model, slot);
            }
        }

        if (hand == 2) {
            if (!sheathe) {
                return 1;
            }

            this->AddHandItem(0);
            this->AddHandItem(1);
        }
    } else if (!skipAdd) {
        this->AddHandItem(hand);
    }

    if (sheathe && mainHand) {
        this->ShowHandItemSpellEffects(1, 0);
    }

    return 1;
}

// VERIFY: 69944531 manual
// OFFSET: 0x00731F40
void CGUnit_C::SetSheatheStateInstant() {
    switch (this->m_sheatheState) {
        case WEAPONMODE_SHEATHEDMODE: {
            if (this->m_previousSheatheState == WEAPONMODE_NORMALMODE) {
                this->SheatheHandItem(0, 1);
                this->SheatheHandItem(1, 1);
            } else if (this->m_previousSheatheState == WEAPONMODE_RANGEDMODE) {
                this->m_worldModel->DetachAllChildrenById(35);

                if (this->m_characterComponent && (this->m_obj->m_type & TYPE_PLAYER)) {
                    this->m_characterComponent->RemoveItem(ITEMSLOT_11);
                }

                if (!CGUnit_C::s_trackingType) {
                    this->ClearTrackingTarget(false, true);
                }

                const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false);

                if (item) {
                    if (item->m_inventoryType == INVTYPE_RANGEDRIGHT || item->m_inventoryType == INVTYPE_THROWN) {
                        this->m_worldModel->DetachAllChildrenById(ATTACH_HANDRIGHT);
                        CCharacterComponent::ComponentOpenFingers(this->m_worldModel, COMP_HAND_RIGHT);
                    } else {
                        this->m_worldModel->DetachAllChildrenById(ATTACH_HANDLEFT);
                        CCharacterComponent::ComponentOpenFingers(this->m_worldModel, COMP_HAND_LEFT);
                    }

                    this->m_animationState &= ~0x300000;
                    return;
                }
            }

            this->m_animationState &= ~0x300000;
            return;
        }

        case WEAPONMODE_NORMALMODE: {
            if (this->m_previousSheatheState == WEAPONMODE_RANGEDMODE) {
                this->ShowRangedWeapon(true, nullptr);
            }

            if (this->SheatheHandItem(0, 0)) {
                this->m_animationState |= 0x100000;
            }

            if (this->SheatheHandItem(1, 0)) {
                this->m_animationState |= 0x200000;
            }

            return;
        }

        case WEAPONMODE_RANGEDMODE: {
            if (this->m_previousSheatheState == WEAPONMODE_NORMALMODE) {
                this->SheatheHandItem(0, 1);
                this->SheatheHandItem(1, 1);
            }

            this->m_animationState &= ~0x300000;
            bool rightHand;

            if (this->ShowRangedWeapon(false, &rightHand)) {
                if (rightHand) {
                    this->m_animationState |= 0x100000;
                } else {
                    this->m_animationState |= 0x200000;
                }
            }

            return;
        }

        default: {
            return;
        }
    }
}

// VERIFY: 8485e288 manual
// OFFSET: 0x007367B0
int32_t CGUnit_C::UnsheatheRightWeapon() {
    this->m_animationState |= 0x100000;

    if (!this->m_previousSheatheState) {
        return 0;
    }

    if (this->m_sheatheState == WEAPONMODE_NORMALMODE) {
        const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, false);

        if (!item) {
            return 0;
        }

        this->SetKeyBoneSequence(this->m_worldModel, 3, ((1 << item->m_sheatheType) & 0x88) ? 90 : 89, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
        return 1;
    }

    if (this->m_sheatheState == WEAPONMODE_RANGEDMODE) {
        const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false);

        if (item && (item->m_inventoryType == INVTYPE_RANGEDRIGHT || item->m_inventoryType == INVTYPE_THROWN)) {
            this->ShowRangedWeapon(true, nullptr);
            this->SetKeyBoneSequence(this->m_worldModel, 3, ((1 << item->m_sheatheType) & 0x88) ? 90 : 89, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
            return 1;
        }
    }

    return 0;
}

// VERIFY: b1c179df manual
// OFFSET: 0x007368B0
int32_t CGUnit_C::UnsheatheLeftWeapon() {
    this->m_animationState |= 0x200000;

    if (!this->m_previousSheatheState) {
        return 0;
    }

    if (this->m_sheatheState == WEAPONMODE_NORMALMODE) {
        const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, false);

        if (!item) {
            return 0;
        }

        this->SetKeyBoneSequence(this->m_worldModel, 2, ((1 << item->m_sheatheType) & 0x88) ? 90 : 89, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
        return 1;
    }

    if (this->m_sheatheState == WEAPONMODE_RANGEDMODE) {
        const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false);

        if (item && item->m_inventoryType != INVTYPE_RANGEDRIGHT && item->m_inventoryType != INVTYPE_THROWN) {
            this->ShowRangedWeapon(true, nullptr);
            this->SetKeyBoneSequence(this->m_worldModel, 2, ((1 << item->m_sheatheType) & 0x88) ? 90 : 89, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
            return 1;
        }
    }

    return 0;
}

// VERIFY: d7553528 manual
// OFFSET: 0x00736B60
void CGUnit_C::SetSheatheStateAnimate() {
    switch (this->m_previousSheatheState) {
        case WEAPONMODE_SHEATHEDMODE: {
            break;
        }

        case WEAPONMODE_NORMALMODE: {
            const CGUnitVirtualItem* right = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, false);
            const CGUnitVirtualItem* left = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, false);

            if (right) {
                this->SetKeyBoneSequence(this->m_worldModel, 3, ((1 << right->m_sheatheType) & 0x88) ? 90 : 89, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
                this->m_animationState &= ~0x100000;
            } else {
                this->UnsheatheRightWeapon();
            }

            if (left) {
                this->SetKeyBoneSequence(this->m_worldModel, 2, ((1 << left->m_sheatheType) & 0x88) ? 90 : 89, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
                this->m_animationState &= ~0x200000;
            } else {
                this->UnsheatheLeftWeapon();
            }

            return;
        }

        case WEAPONMODE_RANGEDMODE: {
            this->m_worldModel->DetachAllChildrenById(35);

            if (this->m_characterComponent && (this->m_obj->m_type & TYPE_PLAYER)) {
                this->m_characterComponent->RemoveItem(ITEMSLOT_11);
            }

            if (!CGUnit_C::s_trackingType) {
                this->ClearTrackingTarget(false, true);
            }

            const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false);

            if (!item) {
                break;
            }

            if (item->m_inventoryType == INVTYPE_RANGEDRIGHT || item->m_inventoryType == INVTYPE_THROWN) {
                this->SetKeyBoneSequence(this->m_worldModel, 3, 89, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
                this->m_animationState &= ~0x100000;
                this->UnsheatheLeftWeapon();
            } else {
                this->SetKeyBoneSequence(this->m_worldModel, 2, 89, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
                this->m_animationState &= ~0x200000;
                this->UnsheatheRightWeapon();
            }

            return;
        }

        default: {
            return;
        }
    }

    if (this->m_sheatheState == WEAPONMODE_NORMALMODE) {
        const CGUnitVirtualItem* right = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, false);
        const CGUnitVirtualItem* left = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, false);

        if (right) {
            this->SetKeyBoneSequence(this->m_worldModel, 3, ((1 << right->m_sheatheType) & 0x88) ? 90 : 89, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
            this->m_animationState |= 0x100000;
        }

        if (left) {
            this->SetKeyBoneSequence(this->m_worldModel, 2, ((1 << left->m_sheatheType) & 0x88) ? 90 : 89, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
            this->m_animationState |= 0x200000;
        }
    } else if (this->m_sheatheState == WEAPONMODE_RANGEDMODE) {
        const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false);

        if (item) {
            this->ShowRangedWeapon(true, nullptr);

            if (item->m_inventoryType == INVTYPE_RANGEDRIGHT || item->m_inventoryType == INVTYPE_THROWN) {
                this->SetKeyBoneSequence(this->m_worldModel, 3, ((1 << item->m_sheatheType) & 0x88) ? 90 : 89, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
                this->m_animationState |= 0x100000;
            } else {
                this->SetKeyBoneSequence(this->m_worldModel, 2, ((1 << item->m_sheatheType) & 0x88) ? 90 : 89, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
                this->m_animationState |= 0x200000;
            }
        }
    }
}

// VERIFY: 8939b08f manual
// OFFSET: 0x0073C090
void CGUnit_C::AnimEndCallback(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ param }, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        if (!unit->m_mountModel && unit->m_vehicle && unit->m_vehicle->m_vehicleRec && unit->m_vehicle->IsBoneAnimPlaying(boneId)) {
            unit->m_vehicle->BoneAnimEndHandler(model, boneId, sequenceId, interrupted, time);
        } else {
            unit->AnimEndHandler(model, boneId, sequenceId, interrupted, time);
        }

        return;
    }

    if (boneId != 0xFFFFFFFF && boneId != 26 && !interrupted && model) {
        model->UnsetBoneSequence(boneId, 1, 1);
    }
}

// VERIFY: a0318ed7 manual
// OFFSET: 0x0073BBD0
void CGUnit_C::AnimEndHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time) {
    if (this->m_vehiclePassenger && this->m_vehiclePassenger->m_state) {
        this->m_vehiclePassenger->OnAnimEnd(boneId);
    }

    if (this->m_awaitingEndAnimId == static_cast<int32_t>(sequenceId)) {
        this->m_awaitingEndAnimId = -1;
    }

    AnimationDataRec* anim = g_animationDataDB.GetRecord(sequenceId);
    int32_t behavior = anim ? anim->m_behaviorID : 506;

    if (behavior == 1 || behavior == 131 || behavior == 468) {
        if (!this->m_lootableTime) {
            this->m_lootableTime = FrameTime::s_curTimeMs;
        }

        this->ShowLootEffect();
    }

    if (static_cast<int32_t>(sequenceId) == this->GetPrecastStartAnim()) {
        this->m_animationState &= ~0x10000;
    }

    if (this->m_specialIdleAnim != -1 && this->m_worldModel && this->m_worldModel->IsLoaded(0, 0)) {
        int32_t torso = this->m_torsoKeyBone != -1 && static_cast<int32_t>(this->m_worldModel->GetBoneSequenceId(this->m_torsoKeyBone)) == this->m_specialIdleAnim;
        int32_t special = this->m_specialIdleAnim;
        int32_t base = static_cast<int32_t>(this->m_worldModel->GetBoneSequenceId(0xFFFFFFFF)) == special;

        if ((static_cast<int32_t>(sequenceId) == special && (!torso || !base)) || (!torso && !base)) {
            this->m_specialIdleAnim = -1;
        }
    }

    if (IsMeleeAttackAnim(sequenceId) && !IsMeleeAttackAnim(this->m_meleeAttackAnimId)) {
        this->m_meleeAttackAnimId = -1;
    }

    if (behavior == 192 || behavior == 200) {
        this->m_animationState &= ~0x40000;
    }

    if (behavior == 121) {
        this->m_animationState &= ~0x80000;
    }

    if ((this->m_animationState & 0x20000) && (IsMeleeAttackAnim(behavior) || IsSpellCastAnim(behavior))) {
        this->m_animationState &= ~0x2000;
    }

    if (behavior == 37) {
        this->m_animationState &= ~0x800000;
    }

    AnimationDataRec* ended = g_animationDataDB.GetRecord(sequenceId);

    if (ended && ended->m_behaviorID >= 458 && ended->m_behaviorID <= 460) {
        this->m_animationState &= ~0x2000000;
    }

    if (interrupted) {
        switch (behavior) {
            case 39: {
                this->m_animationState &= ~0x4;
                return;
            }

            case 89:
            case 90: {
                if (boneId == 3) {
                    C3Vector position = this->GetPosition();
                    this->ExchangeSheathedWeapon(&position, 0, 0x100000);
                } else {
                    C3Vector position = this->GetPosition();
                    this->ExchangeSheathedWeapon(&position, 1, 0x200000);
                }

                return;
            }

            case 127: {
                this->m_animationState &= ~0x8;
                return;
            }

            case 201: {
                this->m_animationState &= ~0x400000;
                return;
            }

            default: {
                return;
            }
        }
    }

    if (boneId != 0xFFFFFFFF && boneId != 26) {
        this->OnKeyBoneAnimEnd(model, boneId, sequenceId);
        return;
    }

    if (this->m_animationState & 0x8000) {
        this->SetSheatheState(this->m_previousSheatheState, 1, 0);
        this->m_animationState &= ~0x8000;
    }

    if (this->m_mountTransition && SpellVisualsIsMountTransitionFinished(this->m_mountTransition)) {
        return;
    }

    this->m_animationState = (this->m_animationState & ~0x30) | 0x40;

    if ((this->m_obj->m_type & TYPE_PLAYER) && static_cast<CGPlayer_C*>(this)->m_dance) {
        static_cast<CGPlayer_C*>(this)->DanceMoveAnimComplete();
        return;
    }

    this->OnBaseAnimEnd(model, boneId, sequenceId);
    this->m_animationState |= 0x70;
}

// VERIFY: 9cc99e71 manual
// OFFSET: 0x0073B510
void CGUnit_C::OnBaseAnimEnd(CM2Model* model, uint32_t boneId, uint32_t sequenceId) {
    CVehiclePassenger_C* passenger = this->m_vehiclePassenger;

    if (passenger && passenger->m_state == 3 && this->m_unit->m_unitFieldHealth > 0) {
        const VehicleSeatRec* seat = passenger->m_seatRec;

        if (seat) {
            bool ride;

            if (passenger->GetLowerAnim(seat) != 506) {
                ride = (seat->m_flags & 0x2) && (static_cast<int32_t>(sequenceId) == seat->m_rideAnimStart || static_cast<int32_t>(sequenceId) == seat->m_rideAnimLoop);
            } else {
                ride = (seat->m_flags & 0x4) && (static_cast<int32_t>(sequenceId) == seat->m_rideUpperAnimStart || static_cast<int32_t>(sequenceId) == seat->m_rideUpperAnimLoop);
            }

            if (ride) {
                this->UpdateBaseAnimation(0, -1);
                return;
            }
        }
    }

    uint32_t next = 506;
    AnimationDataRec* anim = g_animationDataDB.GetRecord(sequenceId);
    int32_t behavior = anim ? anim->m_behaviorID : 506;

    if (behavior >= 458 && behavior <= 460) {
        this->PlayBaseAnimation(464, 16);
        return;
    }

    switch (behavior) {
        case 1: {
            if (!this->IsDeadOrAttachmentDying()) {
                break;
            }

            M2BoneSequenceInfo info;
            model->GetBoneSequenceInfo(boneId, info);
            this->SetKeyBoneSequence(model, 0xFFFFFFFF, 6, info.m_variationIndex, 0, 1.0f, 1, 1, 0);
            this->UpdateObjectEffectAnimationStates();
            return;
        }

        case 6:
        case 472: {
            if (!this->IsDeadOrAttachmentDying()) {
                break;
            }

            this->UpdateObjectEffectAnimationStates();
            return;
        }

        case 37:
        case 38: {
            if (this->m_movement->m_flags & 0x2000000) {
                break;
            }

            this->PlayBaseAnimation(38, 16);
            return;
        }

        case 39: {
            this->m_animationState &= ~0x4;
            this->UpdateBaseAnimation(16, -1);
            return;
        }

        case 40: {
            this->PlayBaseAnimation(40, 16);
            return;
        }

        case 50: {
            this->PlayBaseAnimation(188, 16);
            return;
        }

        case 69: {
            if (this->IsInCombat() || this->m_combat.m_attacking.m_guid) {
                break;
            }

            this->PlayBaseAnimation(69, 16);
            return;
        }

        case 96:
        case 97: {
            next = (this->GetClientStandState() != UNIT_SITTING) + 97;

            if (next == 506) {
                break;
            }

            this->PlayBaseAnimation(next, 16);
            return;
        }

        case 99:
        case 100: {
            next = (this->GetClientStandState() != UNIT_SLEEPING) + 100;

            if (next == 506) {
                break;
            }

            this->PlayBaseAnimation(next, 16);
            return;
        }

        case 102: {
            if (this->GetClientStandState() != UNIT_SITTINGCHAIRLOW) {
                break;
            }

            this->PlayBaseAnimation(102, 16);
            return;
        }

        case 103: {
            if (this->GetClientStandState() != UNIT_SITTINGCHAIRMEDIUM) {
                break;
            }

            this->PlayBaseAnimation(103, 16);
            return;
        }

        case 104: {
            if (this->GetClientStandState() != UNIT_SITTINGCHAIRHIGH) {
                break;
            }

            this->PlayBaseAnimation(104, 16);
            return;
        }

        case 105: {
            this->PlayBaseAnimation(109, 16);
            return;
        }

        case 106: {
            this->PlayBaseAnimation(110, 16);
            return;
        }

        case 109: {
            if (!this->m_rangedWeaponModel) {
                break;
            }

            if (this->m_unit->m_unitChannelSpell || (this->m_animationState & 0x600)) {
                this->PlayBaseAnimation(109, 16);
                return;
            }

            if (!this->m_rangedWeaponModel || !this->m_ammoModel || !this->m_ammoModel->m_attachParent) {
                break;
            }

            this->m_ammoModel->DetachFromParent();
            this->m_animationState &= ~0x4000;
            this->m_rangedWeaponModel->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
            this->UpdateBaseAnimation(16, -1);
            return;
        }

        case 110: {
            if (!this->m_rangedWeaponModel || (!this->m_unit->m_unitChannelSpell && !(this->m_animationState & 0x400))) {
                break;
            }

            this->PlayBaseAnimation(110, 16);
            return;
        }

        case 111:
        case 112: {
            if (!this->m_rangedWeaponModel || (!this->m_unit->m_unitChannelSpell && !(this->m_animationState & 0x400))) {
                break;
            }

            this->PlayBaseAnimation(111, 16);
            return;
        }

        case 114:
        case 115: {
            next = (this->GetClientStandState() != UNIT_KNEEL) + 115;

            if (next == 506) {
                break;
            }

            this->PlayBaseAnimation(next, 16);
            return;
        }

        case 127: {
            this->m_animationState &= ~0x8;
            this->UpdateBaseAnimation(16, -1);
            return;
        }

        case 131: {
            if (!this->IsDeadOrAttachmentDying()) {
                break;
            }

            M2BoneSequenceInfo info;
            model->GetBoneSequenceInfo(boneId, info);
            this->SetKeyBoneSequence(model, 0xFFFFFFFF, 132, info.m_variationIndex, 0, 1.0f, 1, 1, 0);
            this->UpdateObjectEffectAnimationStates();
            return;
        }

        case 132: {
            if (!this->IsDeadOrAttachmentDying()) {
                break;
            }

            return;
        }

        case 133: {
            this->CreateFishingLineEffect();
            this->PlayBaseAnimation(134, 16);
            return;
        }

        case 201: {
            this->m_animationState &= ~0x400000;
            this->PlayBaseAnimation(202, 16);
            return;
        }

        case 464: {
            this->ChooseDefaultAnim(next, 1);

            if (next == 506) {
                break;
            }

            this->PlayBaseAnimation(next, 16);
            return;
        }

        case 466:
        case 467: {
            if (!this->IsDeadOrAttachmentDying()) {
                break;
            }

            this->PlayBaseAnimation((this->m_animationState & 0x4000000) ? 467 : 468, 16);
            return;
        }

        case 468: {
            if (!this->IsDeadOrAttachmentDying()) {
                break;
            }

            M2BoneSequenceInfo info;
            model->GetBoneSequenceInfo(boneId, info);
            this->SetKeyBoneSequence(model, 0xFFFFFFFF, this->ResolveAnimForModel(472, nullptr), info.m_variationIndex, 0, 1.0f, 1, 1, 0);
            this->UpdateObjectEffectAnimationStates();
            return;
        }

        default: {
            break;
        }
    }

    this->UpdateBaseAnimation(16, -1);
}

// VERIFY: 81228148 manual
// OFFSET: 0x00737BD0
void CGUnit_C::OnKeyBoneAnimEnd(CM2Model* model, uint32_t boneId, uint32_t sequenceId) {
    if (sequenceId == 15) {
        return;
    }

    if (sequenceId > 88 && sequenceId <= 90) {
        if (boneId == 2) {
            if (!(this->m_animationState & 0x200000) && this->UnsheatheLeftWeapon()) {
                return;
            }
        } else if (boneId == 3) {
            if (!(this->m_animationState & 0x100000) && this->UnsheatheRightWeapon()) {
                return;
            }
        }
    }

    M2BoneSequenceInfo torso;
    torso.m_sequenceId = 0xFFFFFFFF;
    torso.m_variationIndex = 0xFFFFFFFF;
    torso.m_time = 0;
    torso.m_speed = 1.0f;
    torso.m_startTime = 0;
    torso.m_endTime = 0;
    torso.m_finished = 0;
    torso.m_complete = 0;
    uint32_t anim = 0xFFFFFFFF;
    int32_t unsetTorso = 0;
    int32_t usedDefault = 0;

    if (this->m_worldModel && this->m_worldModel->IsLoaded(0, 0)) {
        uint32_t animFlags = 0;
        uint32_t chosen = this->ChooseAnimation(0xFFFFFFFF, &animFlags, &usedDefault);
        anim = this->ResolveAnimForModel(chosen, nullptr);
        torso.m_sequenceId = anim;
        CVehiclePassenger_C* passenger = this->m_vehiclePassenger;
        const VehicleSeatRec* seat = passenger ? passenger->m_seatRec : nullptr;
        int32_t ridingLower = passenger && passenger->m_state == 3 && passenger->GetLowerAnim(seat) != 506;

        if (static_cast<int32_t>(boneId) == this->m_torsoKeyBone) {
            if (!usedDefault && chosen == 506 && ridingLower) {
                this->UpdateObjectEffectAnimationStates();
                return;
            }
        }

        if (static_cast<int32_t>(boneId) != this->m_torsoKeyBone || !ridingLower || this->m_vehiclePassenger->GetUpperAnim(seat) == 506) {
            uint32_t current = this->m_worldModel->GetBoneSequenceId(0xFFFFFFFF);

            if (!IsUpperBodyAnim(anim) || !this->ShouldSplitTorsoAnimation(anim, current) || anim == current) {
                unsetTorso = 1;
            }
        }
    }

    M2BoneSequenceInfo overrideTorso;

    if (this->ApplyAttachmentAnimOverride(nullptr, nullptr, &torso, 1, nullptr, nullptr, &overrideTorso, nullptr, nullptr)) {
        this->SetKeyBoneSequence(model, boneId, overrideTorso.m_sequenceId, overrideTorso.m_variationIndex, overrideTorso.m_time, overrideTorso.m_speed, 1, 1, 0);
    } else if (!unsetTorso) {
        this->SetKeyBoneSequence(model, boneId, anim, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
    } else if (model && (!this->m_vehiclePassenger || !this->m_vehiclePassenger->OverridesModelAnimation())) {
        if (model->IsLoaded(0, 0) && model->BoneHasParent(boneId)) {
            model->UnsetBoneSequence(boneId, 1, 1);
        }

        CVehicle_C* vehicle = this->m_vehicle;

        if (vehicle && vehicle->m_vehicleRec && vehicle->IsMirroringAnimsToPassengers() && model == this->GetObjectModel()) {
            for (CPassenger* rider = vehicle->m_passengers.Head(); rider; rider = vehicle->m_passengers.Next(rider)) {
                CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*rider->m_ownerGuid, TYPE_UNIT, __FILE__, __LINE__));

                if (unit && unit->m_vehiclePassenger && unit->m_vehiclePassenger->OverridesModelAnimation()) {
                    unit->UnsetKeyBoneSequence(unit->GetObjectModel(), boneId, 1, 1, 1);
                }
            }
        }
    }

    this->UpdateObjectEffectAnimationStates();
}

// VERIFY: fab76f79 auto
// OFFSET: 0x00732500
void CGUnit_C::ExchangeSheathedWeapon(const C3Vector* position, uint32_t hand, uint32_t flag) {
    const CGUnitVirtualItem* item = this->GetVirtualItem(static_cast<VIRTUAL_MONSTER_SLOT>(hand), false);
    const CGUnitVirtualItem* ranged = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false);
    WEAPONMODE state;
    bool sheathe;

    if ((flag & this->m_animationState) && (state = this->m_sheatheState) != 0) {
        sheathe = false;
    } else {
        state = this->m_previousSheatheState;
        sheathe = true;
    }

    if (state != WEAPONMODE_RANGEDMODE) {
        this->SheatheHandItem(hand, sheathe);
        SI2::PlaySheatheSound(item, sheathe, position, this->m_obj->m_guid == CGUnit_C::s_activeMover);
    } else if (!ranged || (ranged->m_inventoryType == INVTYPE_RANGED ? hand != 0 : (static_cast<uint8_t>(ranged->m_inventoryType - INVTYPE_THROWN) > 1 || hand != 1))) {
        this->ShowRangedWeapon(sheathe, nullptr);
        SI2::PlaySheatheSound(ranged, sheathe, position, this->m_obj->m_guid == CGUnit_C::s_activeMover);
    }

    if (this->m_previousSheatheState == WEAPONMODE_RANGEDMODE) {
        this->AddHandItem(0);
    }

    WOWGUID guid = this->m_obj->m_guid;
    CGGameUI::UnitModelUpdate(guid, 1);
}

// VERIFY: 9b6e2325 manual
// OFFSET: 0x007228B0
int32_t CGUnit_C::GetPrecastStartAnim() {
    SpellRec spell;

    if (!g_spellDB.GetRecord(this->m_castSpellId, &spell)) {
        return -1;
    }

    const SpellVisualRec* visual = GetSpellVisual(&spell);

    if (!visual) {
        return -1;
    }

    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_precastKit);

    if (!kit) {
        return -1;
    }

    return kit->m_startAnimID;
}

// VERIFY: 1e4aa332 manual
// OFFSET: 0x007395C0
static int32_t UpdateAllSmoothFacingCallback(WOWGUID guid, void* param) {
    CGCamera* camera = static_cast<CGCamera*>(param);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__));

    if (unit && (unit->m_obj->m_type & TYPE_UNIT) && unit->m_obj->m_guid != camera->m_targetGUID && !unit->HasVehicleTransport()) {
        unit->UpdateSmoothFacing(nullptr);
    }

    return 1;
}

// VERIFY: 908e19e6 auto
// OFFSET: 0x00739630
void CGUnit_C::UpdateAllSmoothFacing() {
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera) {
        ClntObjMgrEnumVisibleObjects(UpdateAllSmoothFacingCallback, camera);
    }
}

// VERIFY: 274748d6 manual
// OFFSET: 0x007190A0
static bool IsOwnedByActivePlayer(const WOWGUID& guid, bool* isPlayer) {
    if (guid == ClntObjMgrGetActivePlayer()) {
        *isPlayer = true;
        return true;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit || !unit->GetCharmerOrCreatorGuid()->m_guid) {
        return false;
    }

    CGUnit_C* owner = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*unit->GetCharmerOrCreatorGuid(), TYPE_UNIT, __FILE__, __LINE__));

    if (*unit->GetCharmerOrCreatorGuid() == ClntObjMgrGetActivePlayer() || (owner && *owner->GetCharmerOrCreatorGuid() == ClntObjMgrGetActivePlayer())) {
        *isPlayer = false;
        return true;
    }

    return false;
}

// VERIFY: d0bc6fd3 auto
// OFFSET: 0x0071F990
void CGUnit_C::AddWorldText(const WOWGUID& source, MISS_REASON reason, const SpellRec* spell, bool periodic) {
    bool melee = !spell || (spell->m_attributesExC & 0x8000);
    bool isPlayer;

    if (this->m_obj->m_guid == CGUnit_C::s_activeMover || !IsOwnedByActivePlayer(source, &isPlayer) || !g_combatDamageCVar->m_intValue || (periodic && !s_cvCombatLogPeriodicSpells->m_intValue)) {
        return;
    }

    const CImVector* color;

    if (melee) {
        if (isPlayer) {
            color = nullptr;
        } else {
            if (!g_petMeleeDamageCVar->m_intValue) {
                return;
            }

            color = reinterpret_cast<const CImVector*>(&s_petMeleeDamageColor);
        }
    } else {
        if (!isPlayer && !g_petSpellDamageCVar->m_intValue) {
            return;
        }

        color = reinterpret_cast<const CImVector*>(&s_petSpellDamageColor);
    }

    const char* text = FrameScript_GetText(s_combatTextTokens[reason], -1, GENDER_NOT_APPLICABLE);

    if (text && *text) {
        PlayerNameCreateText(this->m_nameDesc, s_combatTextTypes[reason], text, color, nullptr);
    }
}

// VERIFY: 813b4b44 manual
// OFFSET: 0x00722340
void CGUnit_C::AddWorldDamageText(const WOWGUID& source, int32_t amount, const SpellRec* spell, bool crit, bool periodic, bool spellColor) {
    bool melee = !spellColor && (!spell || (spell->m_attributesExC & 0x8000));
    bool isPlayer;

    if (!amount || this->m_obj->m_guid == CGUnit_C::s_activeMover || !IsOwnedByActivePlayer(source, &isPlayer) || !g_combatDamageCVar->m_intValue || (periodic && !s_cvCombatLogPeriodicSpells->m_intValue)) {
        return;
    }

    char text[32];
    SStrPrintf(text, sizeof(text), "%d", amount);
    const CImVector* color;

    if (melee) {
        if (isPlayer) {
            color = nullptr;
        } else {
            if (!g_petMeleeDamageCVar->m_intValue) {
                return;
            }

            color = reinterpret_cast<const CImVector*>(&s_petMeleeDamageColor);
        }
    } else {
        if (!isPlayer && !g_petSpellDamageCVar->m_intValue) {
            return;
        }

        color = reinterpret_cast<const CImVector*>(&s_petSpellDamageColor);
    }

    PlayerNameCreateText(this->m_nameDesc, crit ? WT_CRIT : WT_DAMAGE, text, color, nullptr);
}

// VERIFY: 25d16696 auto
// OFFSET: 0x0073C220
void CGUnit_C::SetUpdateInfo(const CClientMoveUpdate& update, int32_t activeMover) {
    this->m_movementData.SetUpdateInfo(OsGetAsyncTimeMs(), &update, activeMover);

    if (this->m_movementData.m_flags & MOVEMENTFLAG_FALLING_FAR) {
        this->OnCollideFalling();
    }
}

// VERIFY: 70c70d86 manual
// OFFSET: 0x0071ABE0
void CGUnit_C::ApplyModelFade() {
    float alpha = this->GetBaseAlpha();
    CEffect* effect = this->m_auraEffects;
    uint32_t time = DEFAULT_MODEL_FADE_TIME;

    if (effect) {
        alpha *= effect->m_alpha;

        if (effect->m_fadeTime) {
            time = effect->m_fadeTime;
        }
    }

    this->DoFade(alpha, time);
}

// VERIFY: 6844e655 auto
// OFFSET: 0x0072CBB0
void CGUnit_C::UpdateModelScale(const CreatureModelDataRec* modelRec) {
    float oldScale = this->m_modelScale;
    this->m_modelScale = this->GetModelScale(this->m_unit->m_unitFieldDisplayid);
    float scale;

    if (modelRec) {
        scale = this->m_scale;
    } else {
        scale = oldScale / this->m_modelScale * this->m_scale;
    }

    this->CGObject_C::HandleScaleUpdate(scale);
    this->UpdateSelectionRadius();
    this->UpdateEffectAttachments();

    if (this->m_vehicle && this->m_vehicle->m_vehicleRec) {
        this->m_vehicle->UpdateLargestPassengerBoundsRadius();
    }
}

// VERIFY: 08a7bbd9 auto
// OFFSET: 0x004F6250
int32_t CGUnit_C::IsSeatedInVehicle() const {
    return this->m_vehiclePassenger && this->m_vehiclePassenger->m_state == 3;
}

// VERIFY: ca00d21d manual
// OFFSET: 0x00716CD0
int32_t Packet_SMSG_FORCE_DISPLAY_UPDATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->UpdateDisplayInfo(1);
    }

    return 1;
}

// VERIFY: de309a21 manual
// OFFSET: 0x0073F540
int32_t Packet_SMSG_STANDSTATE_UPDATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t standState = 0;
    msg->Get(standState);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->UpdateClientStandState(static_cast<UNITSTANDSTATE>(standState));
    }

    return 1;
}

// VERIFY: 5e9fce9e manual
// OFFSET: 0x00716940
int32_t Packet_SMSG_FLIGHT_SPLINE_SYNC(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    float progress;
    SmartGUID guid;
    msg->Get(progress);
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->m_movementData.ComputeSplineDurationModifier(progress);
    }

    return 1;
}

// VERIFY: 3b683e58 manual
// OFFSET: 0x00716D20
int32_t Packet_SMSG_HEALTH_UPDATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    uint32_t health;
    *msg >> guid;
    msg->Get(health);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->m_predictedHealth = health;
    }

    return 1;
}

// VERIFY: cde7ade5 manual
// OFFSET: 0x007236C0
int32_t Packet_SMSG_POWER_UPDATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    uint8_t powerType;
    uint32_t value;
    *msg >> guid;
    msg->Get(powerType);
    msg->Get(value);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->SetPredictedPower(static_cast<POWER_TYPE>(powerType), value);
    }

    return 1;
}

// VERIFY: 62c28594 manual
// OFFSET: 0x00718D70
bool CGUnit_C::IsInMyParty(const CGUnit_C* unit) const {
    if (unit == this) {
        return true;
    }

    if (unit->m_creatureCacheEntry && (unit->m_creatureCacheEntry->m_typeFlags & 0x4000000)) {
        return true;
    }

    if (!(this->m_unit->m_unitFieldFlags & 8) || !(unit->m_unit->m_unitFieldFlags & 8)) {
        return false;
    }

    CGUnit_C* controller = const_cast<CGUnit_C*>(this)->GetControllingPlayer();
    CGUnit_C* otherController = const_cast<CGUnit_C*>(unit)->GetControllingPlayer();

    if (!controller || !otherController) {
        return false;
    }

    if (controller->IsActivePlayer()) {
        WOWGUID guid = otherController->m_obj->m_guid;

        if (CGGameUI::IsPartyMember(guid)) {
            return true;
        }
    }

    if (otherController->IsActivePlayer()) {
        WOWGUID guid = controller->m_obj->m_guid;

        if (CGGameUI::IsPartyMember(guid)) {
            return true;
        }
    }

    return false;
}

// VERIFY: 9ca6f0b9 manual
// OFFSET: 0x0072CCA0
void CGUnit_C::ExecuteClientControlUpdate(WOWGUID guid, uint8_t state) {
    this->SetLocalClientControl(state != 0);
    CGCamera* camera = CGWorldFrame::GetActiveCamera();
    WOWGUID target = camera->m_targetGUID;

    if (target == guid) {
        if (this->m_clientFlags & 0x400) {
            CGUnit_C::SetActiveMover(guid.m_guid);
            return;
        }

        if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
            CGUnit_C::SetActiveMover(0);
        }
    } else if (this->m_obj->m_guid == CGUnit_C::s_activeMover && !(this->m_clientFlags & 0x400)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPE_OBJECT, __FILE__, __LINE__));

        if (unit && (unit->m_obj->m_type & TYPE_UNIT) && (unit->m_clientFlags & 0x400)) {
            CGUnit_C::SetActiveMover(target.m_guid);
            return;
        }

        CGUnit_C::SetActiveMover(0);
    }
}

// VERIFY: 05cda30d manual
// OFFSET: 0x00718CA0
bool CGUnit_C::IsInMyPartyOrRaid(const CGUnit_C* unit) const {
    if (unit == this) {
        return true;
    }

    if (!(this->m_unit->m_unitFieldFlags & 8) || !(unit->m_unit->m_unitFieldFlags & 8)) {
        return false;
    }

    CGUnit_C* controller = const_cast<CGUnit_C*>(this)->GetControllingPlayer();
    CGUnit_C* otherController = const_cast<CGUnit_C*>(unit)->GetControllingPlayer();

    if (!controller || !otherController) {
        return false;
    }

    if (controller->IsActivePlayer()) {
        WOWGUID guid = otherController->m_obj->m_guid;

        if (CGGameUI::IsInParty(guid)) {
            return true;
        }
    }

    if (otherController->IsActivePlayer()) {
        WOWGUID guid = controller->m_obj->m_guid;

        if (CGGameUI::IsInParty(guid)) {
            return true;
        }
    }

    return false;
}

// VERIFY: 0de3c03c auto
// OFFSET: 0x0073C830
void CGUnit_C::OnVisFlagChanged(uint8_t oldFlags) {
    uint8_t changed = oldFlags ^ this->m_unit->m_unitFieldBytes1[2];

    if (changed & 2) {
        this->UpdateBaseAnimation(0, -1);

        if (this->m_nameDesc) {
            PlayerNameTriggerNameRegenerate(this->m_nameDesc);
        }

        if (this->m_namePlateFrame) {
            this->m_namePlateFrame->UpdateNameDisplay(this);
        }

        if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_STEALTH, nullptr);
            CGSpellBook::UpdateUsable();
        }
    }

    if (changed & 1) {
        if (this->m_nameDesc) {
            PlayerNameTriggerNameRegenerate(this->m_nameDesc);
        }

        if (this->m_namePlateFrame) {
            this->m_namePlateFrame->UpdateNameDisplay(this);
        }
    }
}

// VERIFY: 230b0055 manual
// OFFSET: 0x007260E0
bool CGUnit_C::IsSpellKnown(int32_t spellId) {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        if (static_cast<uint32_t>(spellId) <= static_cast<uint32_t>(g_spellDB.m_maxID)) {
            return (s_knownSpellBits.m_bits.m_data[static_cast<uint32_t>(spellId) >> 5] & (1 << (spellId & 0x1F))) != 0;
        }
    } else if (this->m_obj->m_guid == CGPetInfo::GetPet(0)) {
        return HasPetSpell(spellId);
    }

    return false;
}

// VERIFY: 7a902183 manual
// OFFSET: 0x0071A4B0
const char* CGUnit_C::GetDisplayRaceName(const CGUnit_C* unit, const NameCache* name, int32_t* displaySex) {
    int32_t sex = 2;
    ChrRacesRec* rec;

    if (unit) {
        rec = g_chrRacesDB.GetRecord(unit->m_unit->m_unitFieldBytes0[0]);
        sex = unit->m_unit->m_unitFieldBytes0[2];
    } else {
        if (!name) {
            if (displaySex) {
                *displaySex = sex;
            }

            return nullptr;
        }

        rec = g_chrRacesDB.GetRecord(name->m_race);
        sex = name->m_sex;
    }

    if (rec) {
        return CGUnit_C::GetDisplayRaceNameFromRecord(rec, sex, displaySex);
    }

    if (displaySex) {
        *displaySex = sex;
    }

    return nullptr;
}

// VERIFY: 9bdb654f manual
// OFFSET: 0x0071A540
int32_t CGUnit_C::GetDisplayRaceGender(const CGUnit_C* unit, const NameCache* name) {
    if (unit) {
        if (!(unit->m_obj->m_type & TYPE_PLAYER)) {
            return unit->m_unit->m_unitFieldBytes0[2];
        }
    } else if (!name) {
        return 2;
    }

    int32_t displaySex = 0;
    CGUnit_C::GetDisplayRaceName(unit, name, &displaySex);
    return displaySex;
}

// VERIFY: b8f5220c manual
// OFFSET: 0x0071A590
const char* CGUnit_C::GetDisplayClassName(const CGUnit_C* unit, const NameCache* name, int32_t* displaySex) {
    int32_t sex = 2;
    ChrClassesRec* rec;

    if (unit) {
        rec = g_chrClassesDB.GetRecord(unit->m_unit->m_unitFieldBytes0[1]);
        sex = unit->m_unit->m_unitFieldBytes0[2];
    } else {
        if (!name) {
            if (displaySex) {
                *displaySex = sex;
            }

            return nullptr;
        }

        rec = g_chrClassesDB.GetRecord(name->m_class);
        sex = name->m_sex;
    }

    if (rec) {
        return CGUnit_C::GetDisplayClassNameFromRecord(rec, sex, displaySex);
    }

    if (displaySex) {
        *displaySex = sex;
    }

    return nullptr;
}

// VERIFY: b7519b2a manual
// OFFSET: 0x0071A620
int32_t CGUnit_C::GetDisplayClassGender(const CGUnit_C* unit, const NameCache* name) {
    if (unit) {
        if (!(unit->m_obj->m_type & TYPE_PLAYER)) {
            return unit->m_unit->m_unitFieldBytes0[2];
        }
    } else if (!name) {
        return 2;
    }

    int32_t displaySex = 0;
    CGUnit_C::GetDisplayClassName(unit, name, &displaySex);
    return displaySex;
}

// VERIFY: 61b56493 manual
// OFFSET: 0x007283A0
bool CGUnit_C::AffectedByAura(int32_t auraType, const SpellRec* spell) const {
    SpellRec auraSpell;

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        const AuraData* aura = &const_cast<CGUnit_C*>(this)->m_auras[i];

        if (!g_spellDB.GetRecord(aura->m_spellId, &auraSpell)) {
            continue;
        }

        for (int32_t effect = 0; effect < 3; effect++) {
            if (auraSpell.m_effectAura[effect] == auraType && ((1 << effect) & aura->m_flags) && SpellRec::IsAffectedBy(spell, &auraSpell, effect)) {
                return true;
            }
        }
    }

    return false;
}

// VERIFY: d821b797 manual
// OFFSET: 0x00726160
bool CGUnit_C::IsSpellSuperceded(int32_t spellId) {
    SkillLineAbilityRec* ability = this->LookupAbility(spellId);

    if (!ability) {
        return false;
    }

    while (ability->m_supercededBySpell > 0) {
        if (this->IsSpellKnown(ability->m_supercededBySpell)) {
            return true;
        }

        ability = this->LookupAbility(ability->m_supercededBySpell);

        if (!ability) {
            return false;
        }
    }

    return false;
}

// VERIFY: 9c1e6376 manual
// OFFSET: 0x0073AB90
void CGUnit_C::PlayEmoteAnimation(uint32_t emote) {
    EmotesRec* rec = g_emotesDB.GetRecord(emote);

    if (!rec) {
        return;
    }

    int32_t anim = rec->m_animID;

    if (anim < 0 || anim >= 506) {
        return;
    }

    if (this->GetCurrentTorsoAnimId() == static_cast<uint32_t>(anim)) {
        return;
    }

    bool flight = anim == ANIM_LIFT_OFF || anim == ANIM_LAND;

    if (this->m_unit->m_unitChannelSpell || (this->m_animationState & 0x400)) {
        return;
    }

    if (!flight && this->IsInCombat()) {
        return;
    }

    this->PlayBaseAnimation(anim, 0);
}

// VERIFY: 3273a6b5 auto
// OFFSET: 0x0073AD70
void CGUnit_C::RequestTalkEmote(TALKANIMATION index) {
    if (!s_talkEmotes[index]) {
        return;
    }

    if (this->m_movement->m_flags & 0x200000) {
        return;
    }

    if (this->GetClientStandState() == UNIT_SLEEPING) {
        return;
    }

    this->PlayEmoteAnimation(s_talkEmotes[index]->m_id);
}

// VERIFY: 0f92b828 auto
// OFFSET: 0x007207C0
void CGUnit_C::PlayEmotes(const QUESTGIVEREMOTEQUEUE& emotes) {
    this->SetEmoteQueue(emotes.m_nodes, emotes.m_count);
}

// VERIFY: f8b415d2 manual
// OFFSET: 0x0072AA70
const char* CGUnit_C::GetDisplayRaceName(const CGUnit_C* unit, const NameCache* name) {
    if (unit) {
        if (!(unit->m_obj->m_type & TYPE_PLAYER)) {
            return const_cast<CGUnit_C*>(unit)->GetUnitName(nullptr, 1);
        }
    } else if (!name) {
        return nullptr;
    }

    return CGUnit_C::GetDisplayRaceName(unit, name, nullptr);
}

// VERIFY: 7e92c457 manual
// OFFSET: 0x0072AAB0
const char* CGUnit_C::GetDisplayClassName(const CGUnit_C* unit, const NameCache* name) {
    if (unit) {
        if (!(unit->m_obj->m_type & TYPE_PLAYER)) {
            return const_cast<CGUnit_C*>(unit)->GetUnitName(nullptr, 1);
        }
    } else if (!name) {
        return nullptr;
    }

    return CGUnit_C::GetDisplayClassName(unit, name, nullptr);
}

// VERIFY: 4ca89022 auto
// OFFSET: 0x00726220
void CGUnit_C::ClearUIChannel() {
    this->m_channelSpellId = 0;
    this->m_channelStartTime = 0;
    this->m_channelEndTime = 0;
    this->UpdateSpellCastBars(FrameTime::s_curTimeMs);
}

// VERIFY: 96f9d57c auto
// OFFSET: 0x00726240
void CGUnit_C::DelayCast(int32_t delay) {
    this->m_castStartTime += delay;
    this->m_castEndTime += delay;
    this->m_channelSpellId = 0;
    this->m_channelStartTime = 0;
    this->m_channelEndTime = 0;
    this->UpdateSpellCastBars(FrameTime::s_curTimeMs);
}

// VERIFY: 3a39d598 auto
// OFFSET: 0x0072AC50
void CGUnit_C::SetUICastingDirect(int32_t spellId, const WOWGUID& target, uint32_t start, uint32_t end) {
    this->m_castingSpellId = spellId;
    this->m_castingObject = target;
    this->m_castStartTime = start;
    this->m_castEndTime = end;

    if (this->m_obj->m_guid == CGGameUI::m_lockedTarget || this->m_obj->m_guid == CGGameUI::m_focusTarget) {
        SpellRec spell;

        if (g_spellDB.GetRecord(spellId, &spell)) {
            this->ProcessCastNotInterruptible(&spell);
        }
    }

    this->m_channelSpellId = 0;
    this->m_channelStartTime = 0;
    this->m_channelEndTime = 0;
    this->UpdateSpellCastBars(FrameTime::s_curTimeMs);
}

// VERIFY: acda115f auto
// OFFSET: 0x0072AE00
void CGUnit_C::SetUIChannelDirect(int32_t spellId, uint32_t start, uint32_t end) {
    this->m_channelSpellId = spellId;
    this->m_channelStartTime = start;
    this->m_channelEndTime = end;

    if (this->m_obj->m_guid == CGGameUI::m_lockedTarget || this->m_obj->m_guid == CGGameUI::m_focusTarget) {
        SpellRec spell;

        if (g_spellDB.GetRecord(spellId, &spell)) {
            this->ProcessCastNotInterruptible(&spell);
        }
    }

    this->m_castingSpellId = 0;
    this->m_castStartTime = 0;
    this->m_castEndTime = 0;
    this->UpdateSpellCastBars(FrameTime::s_curTimeMs);
}

// VERIFY: 49e3df47 auto
// OFFSET: 0x00741210
void CGUnit_C::UpdateChainTargets(const TSStackArray<WOWGUID>& targets, int32_t spellId) {
    WOWGUID first = targets.m_data[0];

    if (first && first != this->m_unit->m_unitFieldChannelObject) {
        SpellRec spell;

        if (g_spellDB.GetRecord(spellId, &spell) && (spell.m_attributesEx & 0x4)) {
            this->m_unit->m_unitFieldChannelObject = targets.m_data[0];
        }
    }

    this->SaveChainSpellTargets(targets);
    this->OnChannelSpellChanged(spellId);
}

// VERIFY: 63490106 auto
// OFFSET: 0x0071F060
void WriteMovementPacket(CGUnit_C* unit, uint32_t msgId, CDataStore* msg, int32_t include) {
    if (!include || !unit || !unit->IsAllowedToSendMessage(msgId)) {
        msg->Put(static_cast<uint8_t>(0));
        return;
    }

    msg->Put(static_cast<uint8_t>(1));
    unit->BuildMovementUpdate(MovementGetIdleTime(), msgId, msg, 0.0f, 0);
}

// VERIFY: 0ca23980 manual
// OFFSET: 0x00728500
bool CGUnit_C::HasDispellableAura(int32_t dispelType, int32_t harmful, bool mechanic, bool steal) {
    uint32_t mask = 0;

    if (!mechanic) {
        SpellDispelTypeRec* rec = g_spellDispelTypeDB.GetRecord(dispelType);
        mask = rec && rec->m_mask ? rec->m_mask : 1 << dispelType;
    }

    bool charmed = false;

    if (this->m_unit->m_unitFieldCharmedby && (this->m_obj->m_guid.m_guid & 0xF0F0000000000000ull) != 0xF050000000000000ull) {
        if (!harmful) {
            return false;
        }

        if (harmful == 1) {
            harmful = 0;
            charmed = true;
        }
    }

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        uint32_t count = this->m_auras.m_count == 0xFFFFFFFF ? this->m_auras.m_dynamic.m_count : this->m_auras.m_count;
        AuraData* aura = i < count ? &this->m_auras[i] : nullptr;

        if (harmful == 1 && (aura->m_flags & 0x80)) {
            continue;
        }

        SpellRec spell;

        if (!g_spellDB.GetRecord(this->m_auras[i].m_spellId, &spell) || (spell.m_attributes & 0x40)) {
            continue;
        }

        bool area = false;

        for (uint32_t j = 0; j < 3; j++) {
            if ((aura->m_flags & (1 << j)) && Spell_IsAreaAuraEffect(spell.m_effect[j])) {
                area = true;
                break;
            }
        }

        if (area) {
            continue;
        }

        if (charmed) {
            bool charm = false;

            if ((spell.m_effect[0] == SPELL_AURA_MOD_CHARM || spell.m_effect[0] == SPELL_AURA_AOE_CHARM || spell.m_effect[0] == SPELL_AURA_MOD_POSSESS) && (this->m_auras[i].m_flags & 0x1)) {
                charm = true;
            }

            if ((spell.m_effect[1] == SPELL_AURA_MOD_CHARM || spell.m_effect[1] == SPELL_AURA_AOE_CHARM || spell.m_effect[1] == SPELL_AURA_MOD_POSSESS) && (this->m_auras[i].m_flags & 0x2)) {
                charm = true;
            }

            if ((spell.m_effect[2] == SPELL_AURA_MOD_CHARM || spell.m_effect[2] == SPELL_AURA_AOE_CHARM || spell.m_effect[2] == SPELL_AURA_MOD_POSSESS) && (this->m_auras[i].m_flags & 0x4)) {
                charm = true;
            }

            if (!charm) {
                continue;
            }
        }

        if (steal && ((spell.m_attributesEx & 0x44) || (spell.m_attributesExD & 0x40))) {
            continue;
        }

        if (mechanic) {
            if (spell.m_mechanic == dispelType) {
                return true;
            }

            int32_t effects = 0;

            if (spell.m_effect[0] && spell.m_effectMechanic[0] == dispelType) {
                effects = 1;
            }

            if (spell.m_effect[1] && spell.m_effectMechanic[1] == dispelType) {
                effects |= 2;
            }

            if (spell.m_effect[2] && spell.m_effectMechanic[2] == dispelType) {
                effects |= 4;
            }

            if (effects) {
                return true;
            }
        } else if ((1 << spell.m_dispelType) & mask) {
            return true;
        }
    }

    return false;
}

// VERIFY: 96853cac manual
// OFFSET: 0x0071C8F0
uint8_t CGUnit_C::GetActivePlayerFieldBytes0() {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    return static_cast<CGPlayer_C*>(this)->m_player->m_playerFieldBytes[0];
}

// VERIFY: b01b02ca auto
// OFFSET: 0x00720B30
bool CGUnit_C::GetSpellRequiredLevel(int32_t spellId, int32_t* level) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return false;
    }

    SkillLineAbilityRec* ability = this->LookupAbility(spellId);

    if (!ability) {
        return false;
    }

    const SkillRaceClassInfoRec* info = SpellTableLookupInfo(this->m_unit->m_unitFieldBytes0[0], this->m_unit->m_unitFieldBytes0[1], ability->m_skillLine);

    if (info) {
        int32_t required = info->m_minLevel;

        if (spell.m_spellLevel > required) {
            required = spell.m_spellLevel;
        }

        *level = required;
        return true;
    }

    *level = spell.m_spellLevel;
    return true;
}

// VERIFY: 51855a30 manual
// OFFSET: 0x00739650
bool CGUnit_C::CheckAndReportSpellInhibitFlags(PendingSpellCast* cast, const SpellRec* spell, const CGItem_C* item) {
    if (!spell) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return false;
    }

    MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);

    if (map) {
        if (!CGUnit_C::IsSpellAllowedInArena(spell)) {
            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_NOT_IN_ARENA, -1, -1, false);
            return false;
        }

        if (map->m_instanceType != 4 && (spell->m_attributesExF & 0x2)) {
            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_ONLY_IN_ARENA, -1, -1, false);
            return false;
        }
    }

    if (item) {
        if (Spell_C_GetItemCooldown(const_cast<CGItem_C*>(item), nullptr, nullptr, nullptr)) {
            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_ITEM_NOT_READY, -1, -1, false);
            return false;
        }
    } else {
        bool player = this->m_obj->m_guid == ClntObjMgrGetActivePlayer();

        if (Spell_C_GetSpellCooldown(spell->m_id, !player, nullptr, nullptr, nullptr)) {
            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_NOT_READY, -1, -1, false);
            return false;
        }

        if (!Spell_C_HaveSpellPower(this, spell)) {
            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_NO_POWER, -1, -1, false);
            return false;
        }
    }

    for (uint32_t i = 0; i < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); i++) {
        int32_t effect = spell->m_effect[i];

        if (effect == SPELL_EFFECT_LEARN_SPELL) {
            int32_t learn = spell->m_effectTriggerSpell[i];

            if (!learn) {
                WOWGUID none = { 0 };
                ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

                if (stats) {
                    for (uint32_t j = 0; j < 5; j++) {
                        if (stats->m_spellTrigger[j] == 6) {
                            learn = stats->m_spellId[j];
                            break;
                        }
                    }
                }
            }

            if (this->IsSpellKnown(learn) || this->IsSpellSuperceded(learn)) {
                SpellRec known;

                if (g_spellDB.GetRecord(learn, &known)) {
                    CGGameUI::DisplayError(ERR_SPELL_ALREADY_KNOWN_S, known.m_name);
                } else {
                    CGGameUI::DisplayError(ERR_SPELL_ALREADY_KNOWN_S, "UNKNOWN");
                }

                return false;
            }
        }

        if (effect == SPELL_EFFECT_SUMMON_PET || effect == SPELL_EFFECT_TAMECREATURE || spell->m_effectAura[i] == SPELL_AURA_MOD_CHARM || spell->m_effectAura[i] == SPELL_AURA_MOD_POSSESS) {
            const WOWGUID* controlled = &this->m_unit->m_unitFieldCharm;

            if (!*controlled) {
                controlled = &this->m_unit->m_unitFieldSummon;
            }

            if (*controlled && !(spell->m_attributesEx & 0x1)) {
                Spell_C_SpellFailed(cast, spell, this->m_unit->m_unitFieldCharm ? SPELL_FAILED_ALREADY_HAVE_CHARM : SPELL_FAILED_ALREADY_HAVE_SUMMON, -1, -1, false);
                return false;
            }
        }

        if (spell->m_implicitTargetA[i] == 5 || spell->m_implicitTargetB[i] == 5) {
            const WOWGUID* controlled = &this->m_unit->m_unitFieldCharm;

            if (!*controlled) {
                controlled = &this->m_unit->m_unitFieldSummon;
            }

            if (!*controlled) {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_NO_PET, -1, -1, false);
                return false;
            }

            if (effect == 36) {
                const WOWGUID* pet = &this->m_unit->m_unitFieldCharm;

                if (!*pet) {
                    pet = &this->m_unit->m_unitFieldSummon;
                }

                CGUnit_C* petUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*pet, TYPE_UNIT, __FILE__, __LINE__));

                if (petUnit) {
                    int32_t learn = spell->m_effectTriggerSpell[i];
                    int32_t level;

                    if (!petUnit->GetSpellRequiredLevel(learn, &level)) {
                        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_SPELL_UNAVAILABLE, -1, -1, false);
                        return false;
                    }

                    if (petUnit->IsSpellKnown(learn) || petUnit->IsSpellSuperceded(learn)) {
                        CGGameUI::DisplayError(ERR_PET_SPELL_ALREADY_KNOWN_S, spell->m_name);
                        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_DONT_REPORT, -1, -1, false);
                        return false;
                    }
                }
            }
        }
    }

    int32_t reason;

    if (this->m_unit->m_unitFieldHealth > 0) {
        if ((this->m_obj->m_type & TYPE_PLAYER) && (static_cast<CGPlayer_C*>(this)->m_player->m_playerFieldBytes[0] & 0x4)) {
            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_SPELL_IN_PROGRESS, -1, -1, false);
            return false;
        }

        if (this->m_unit->m_unitFieldCharmedby && this->m_unit->m_unitFieldCharmedby != ClntObjMgrGetActivePlayer() && !Spell_C_CancelsCharm(this, spell, &reason)) {
            if (reason) {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_PREVENTED_BY_MECHANIC, reason, -1, false);
            } else {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_CHARMED, -1, -1, false);
            }

            return false;
        }

        if ((this->m_unit->m_unitFieldFlags & 0x40000) && !(spell->m_attributesExE & 0x8) && !(spell->m_attributesExG & 0x100000) && !Spell_C_CancelsStun(this, spell, &reason)) {
            if (reason) {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_PREVENTED_BY_MECHANIC, reason, -1, false);
            } else {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_STUNNED, -1, -1, false);
            }

            return false;
        }

        if (spell->m_preventionType == 1 && (this->m_unit->m_unitFieldFlags & 0x2000) && !Spell_C_CancelsSilence(this, spell, &reason)) {
            if (reason) {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_PREVENTED_BY_MECHANIC, reason, -1, false);
            } else {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_SILENCED, -1, -1, false);
            }

            return false;
        }

        if (spell->m_preventionType == 2 && (this->m_unit->m_unitFieldFlags & 0x20000) && !Spell_C_CancelsPacify(this, spell, &reason)) {
            if (reason) {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_PREVENTED_BY_MECHANIC, reason, -1, false);
            } else {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_PACIFIED, -1, -1, false);
            }

            return false;
        }

        if ((this->m_unit->m_unitFieldFlags & 0x800000) && !(spell->m_attributesExE & 0x20000) && !(spell->m_attributesExG & 0x100000) && !Spell_C_CancelsFear(this, spell, &reason)) {
            if (reason) {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_PREVENTED_BY_MECHANIC, reason, -1, false);
            } else {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_FLEEING, -1, -1, false);
            }

            return false;
        }

        if ((this->m_unit->m_unitFieldFlags & 0x400000) && !(spell->m_attributesExE & 0x40000) && !(spell->m_attributesExG & 0x100000) && !Spell_C_CancelsConfuse(this, spell, &reason)) {
            if (reason) {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_PREVENTED_BY_MECHANIC, reason, -1, false);
            } else {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_CONFUSED, -1, -1, false);
            }

            return false;
        }
    }

    int32_t auraInterrupt = spell->m_auraInterruptFlags;

    if (((auraInterrupt & 0x200) && !this->m_sheatheState) || ((spell->m_attributesEx & 0x44) && (spell->m_channelInterruptFlags & 0x200) && !this->m_sheatheState)) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_NOT_UNSHEATHED, -1, -1, false);
        return false;
    }

    if (((auraInterrupt & 0x40) && (this->m_mountDisplayId <= 0 || (this->m_clientFlags & 0x10000000))) || ((spell->m_attributesEx & 0x44) && (spell->m_channelInterruptFlags & 0x40) && !this->IsMounted())) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_ONLY_MOUNTED, -1, -1, false);
        return false;
    }

    if (!(spell->m_attributes & 0x1000000) && this->m_mountDisplayId > 0 && !(this->m_clientFlags & 0x10000000) && !this->MountedCombatAllowed() && !this->CanAutoDismount()) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_NOT_MOUNTED, -1, -1, false);
        return false;
    }

    if (!(spell->m_attributes & 0x8000000) && this->GetClientStandState() && !this->CanAutoStand()) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_NOT_STANDING, -1, -1, false);
        return false;
    }

    if ((spell->m_attributes & 0x1000) && !g_clientGameTime.IsDaytime()) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_ONLY_DAYTIME, -1, -1, false);
        return false;
    }

    if ((spell->m_attributes & 0x2000) && g_clientGameTime.IsDaytime()) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_ONLY_NIGHTTIME, -1, -1, false);
        return false;
    }

    auraInterrupt = spell->m_auraInterruptFlags;
    int32_t channeled = spell->m_attributesEx & 0x44;

    if (((auraInterrupt & 0x100) && !(this->m_clientFlags & 0x200000)) || (channeled && (spell->m_channelInterruptFlags & 0x100) && !(this->m_clientFlags & 0x200000))) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_ONLY_UNDERWATER, -1, -1, false);
        return false;
    }

    if (((auraInterrupt & 0x80) && (this->m_clientFlags & 0x200000)) || (channeled && (spell->m_channelInterruptFlags & 0x80) && (this->m_clientFlags & 0x200000))) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_ONLY_ABOVEWATER, -1, -1, false);
        return false;
    }

    if ((channeled && (spell->m_channelInterruptFlags & 0x18) && (this->m_movement->m_flags & 0xC0100F) && !(spell->m_targets & 0x60)) || ((spell->m_interruptFlags & 0x1) && (this->m_movement->m_flags & 0xC0100F) && !(spell->m_targets & 0x60) && !(spell->m_attributesExB & 0x20) && (SpellRec::GetCastTime(spell, this->m_unit->m_unitFieldPetnumber != 0, 0, 0) || (spell->m_auraInterruptFlags & 0x18)))) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_MOVING, -1, -1, false);
        return false;
    }

    if (!this->CheckShapeshiftRules(spell, &reason) && (reason == 94 || !this->CanAutoUnshift())) {
        Spell_C_SpellFailed(cast, spell, reason, -1, -1, false);
        return false;
    }

    if (!this->CheckStealthRules(spell, &reason)) {
        Spell_C_SpellFailed(cast, spell, reason, -1, -1, false);
        return false;
    }

    if ((spell->m_attributes & 0x10000000) && (this->m_unit->m_unitFieldFlags & 0x80000) && !this->IgnoresAuraState(spell)) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_AFFECTING_COMBAT, -1, -1, false);
        return false;
    }

    if (spell->m_casterAuraState && !this->HasAuraState(spell->m_casterAuraState) && !this->IgnoresAuraState(spell)) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_CASTER_AURASTATE, -1, -1, false);
        return false;
    }

    if (spell->m_excludeCasterAuraState && this->HasAuraState(spell->m_excludeCasterAuraState) && !this->IgnoresAuraState(spell)) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_CASTER_AURASTATE, -1, -1, false);
        return false;
    }

    int32_t casterAura = QuestTextParser::Spell_C_SpellGetSpellIDForDifficulty(spell->m_casterAuraSpell);

    if (casterAura && !this->HasAuraBySpellId(casterAura) && !this->IgnoresAuraState(spell)) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_CASTER_AURASTATE, -1, -1, false);
        return false;
    }

    int32_t excludeCasterAura = QuestTextParser::Spell_C_SpellGetSpellIDForDifficulty(spell->m_excludeCasterAuraSpell);

    if (excludeCasterAura && this->HasAuraBySpellId(excludeCasterAura) && !this->IgnoresAuraState(spell)) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_CASTER_AURASTATE, -1, -1, false);
        return false;
    }

    if (spell->m_minFactionID && GetRepListRepValue(spell->m_minFactionID) < CGReputationInfo::m_reactionThresholds[spell->m_minReputation]) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_REPUTATION, -1, -1, false);
        return false;
    }

    if (spell->m_requiredAreasID > 0) {
        AreaTableRec* area = CGGameUI::GetCurrentSubzoneRec();

        if (!area) {
            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_INCORRECT_AREA, -1, -1, false);
            return false;
        }

        int32_t group = spell->m_requiredAreasID;
        int32_t checked;

        if (!CGUnit_C::IsAreaInAreaGroup(area->m_id, group, &checked)) {
            if (checked == 1) {
                AreaGroupRec* groupRec = g_areaGroupDB.GetRecord(group);

                if (groupRec) {
                    Spell_C_SpellFailed(cast, spell, SPELL_FAILED_REQUIRES_AREA, groupRec->m_areaID[0], -1, false);
                    return false;
                }
            }

            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_INCORRECT_AREA, -1, -1, false);
            return false;
        }
    }

    if (spell->m_attributes & 0xC000) {
        if (this->IsOutdoors()) {
            if (spell->m_attributes & 0x4000) {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_ONLY_INDOORS, -1, -1, false);
                return false;
            }
        } else if (spell->m_attributes & 0x8000) {
            Spell_C_SpellFailed(cast, spell, SpellRec::HasAura(spell, 78) ? SPELL_FAILED_NO_MOUNTS_ALLOWED : SPELL_FAILED_ONLY_OUTDOORS, -1, -1, false);
            return false;
        }
    }

    if ((spell->m_attributesExD & 0x4000000) && !static_cast<CGPlayer_C*>(this)->IsInFlyableZone(!(spell->m_attributesExG & 0x400000))) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_NOT_HERE, -1, -1, false);
        return false;
    }

    int32_t mapId = CGGameUI::m_iCurrentMapID;
    MapRec* current;

    if ((spell->m_attributesExC & 0x800) && (current = g_mapDB.GetRecord(mapId)) != nullptr && current->m_instanceType != 3) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_ONLY_BATTLEGROUNDS, -1, -1, false);
        return false;
    }

    if ((spell->m_attributesExF & 0x800) && (current = g_mapDB.GetRecord(mapId)) != nullptr && current->m_instanceType == 2) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_NOT_IN_RAID_INSTANCE, -1, -1, false);
        return false;
    }

    if ((spell->m_attributesExB & 0x10000) && (this->m_obj->m_type & TYPE_PLAYER) && (this->GetActivePlayerFieldBytes0() & 0x1)) {
        Spell_C_PetTameFailure(2);
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_DONT_REPORT, -1, -1, false);
        return false;
    }

    return true;
}

// VERIFY: 90d66722 manual
// OFFSET: 0x00727C00
void CGUnit_C::InsertSortedAura(uint32_t list, uint32_t auraIndex, int32_t position) {
    AuraData* aura = auraIndex < this->GetAuraCount() ? &this->m_auras[auraIndex] : nullptr;
    WOWGUID player = ClntObjMgrGetActivePlayer();
    TSCGrowableArray<SortedAuraInfo, 16>& sorted = this->m_sortedAuras[list];

    if (position) {
        if (CGSpellBook::IsAuraCaster(player, &aura->m_creator)) {
            int32_t i = position - 1;

            for (; i >= 0; i--) {
                uint32_t otherIndex = sorted[i].m_auraIndex;
                AuraData* other = otherIndex < this->GetAuraCount() ? &this->m_auras[otherIndex] : nullptr;

                if (CGSpellBook::IsAuraCaster(player, &other->m_creator)) {
                    break;
                }

                sorted[i + 1] = sorted[i];
            }

            position = i + 1;
        } else if (CGSpellBook::CanApplyAura(aura)) {
            int32_t i = position - 1;

            for (; i >= 0; i--) {
                AuraData* other = this->GetAura(sorted[i].m_auraIndex);

                if (CGSpellBook::IsAuraCaster(player, &other->m_creator) || CGSpellBook::CanApplyAura(other)) {
                    break;
                }

                sorted[i + 1] = sorted[i];
            }

            position = i + 1;
        }
    }

    sorted[position].m_auraIndex = auraIndex;
    sorted[position].m_spellId = aura->m_spellId;
}

// VERIFY: 2d8ce268 manual
// OFFSET: 0x0072C510
void CGUnit_C::SortAuras() {
    for (uint32_t list = 0; list < 2; list++) {
        TSCGrowableArray<SortedAuraInfo, 16>& sorted = this->m_sortedAuras[list];
        uint32_t i = 0;

        while (i < this->GetAuraCount() && sorted[i].m_spellId) {
            AuraData& aura = this->m_auras[sorted[i].m_auraIndex];

            if (aura.m_spellId == sorted[i].m_spellId && (aura.m_flags & 0x7)) {
                i++;
                continue;
            }

            uint32_t last = this->GetAuraCount() - 1;

            if (i < last) {
                memcpy(&sorted[i], &sorted[i + 1], sizeof(SortedAuraInfo) * (last - i));
            }

            sorted[last].m_auraIndex = 256;
            sorted[last].m_spellId = 0;
        }
    }

    SpellRec spell;

    for (uint32_t index = this->GetAuraCount(); index; ) {
        index--;

        AuraData* aura = index < this->GetAuraCount() ? &this->m_auras[index] : nullptr;

        if (!aura->m_spellId || !(aura->m_flags & 0x7) || !g_spellDB.GetRecord(aura->m_spellId, &spell)) {
            continue;
        }

        if (!CGSpellBook::CanShowAura(&spell, &aura->m_creator, (this->m_auras[index].m_flags & 0x8) != 0)) {
            continue;
        }

        for (uint32_t j = 0; j < this->GetAuraCount(); j++) {
            if (this->m_sortedAuras[0][j].m_spellId <= 0) {
                this->InsertSortedAura(0, index, j);
                break;
            }

            if (this->m_sortedAuras[0][j].m_spellId == aura->m_spellId && this->m_sortedAuras[0][j].m_auraIndex == index) {
                break;
            }
        }

        if ((!(aura->m_flags & 0x80) && CGSpellBook::FindRaidBuff(aura->m_spellId)) || ((aura->m_flags & 0x80) && CGSpellBook::FindRaidDispel(spell.m_dispelType))) {
            for (uint32_t j = 0; j < this->GetAuraCount(); j++) {
                if (this->m_sortedAuras[1][j].m_spellId <= 0) {
                    this->InsertSortedAura(1, index, j);
                    break;
                }

                if (this->m_sortedAuras[1][j].m_spellId == aura->m_spellId && this->m_sortedAuras[1][j].m_auraIndex == index) {
                    break;
                }
            }
        }
    }

    this->m_clientFlags |= 0x40000;
}

// VERIFY: 170ff0ae manual
// OFFSET: 0x0072C9B0
uint32_t CGUnit_C::GetSortedAuraSlot(int32_t index, const char* name, const char* rank, uint32_t filter) {
    if (index < 0 && !name) {
        return 0xFFFFFFFF;
    }

    if (!(this->m_clientFlags & 0x40000)) {
        this->SortAuras();
    }

    WOWGUID player = ClntObjMgrGetActivePlayer();
    bool helpful = filter & BUFF_FILTER_HELPFUL;
    bool playerOnly = (filter & BUFF_FILTER_PLAYER) != 0;
    int32_t found = 0;

    if (!this->GetAuraCount()) {
        return 0xFFFFFFFF;
    }

    TSCGrowableArray<SortedAuraInfo, 16>& sorted = this->m_sortedAuras[(filter & BUFF_FILTER_RAID) != 0];

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        uint32_t auraIndex = sorted[i].m_auraIndex;
        AuraData* aura = auraIndex < this->GetAuraCount() ? &this->m_auras[auraIndex] : nullptr;

        if (!aura || !aura->m_spellId || helpful != !(aura->m_flags & 0x80)) {
            continue;
        }

        if (playerOnly && !CGSpellBook::IsAuraCaster(player, &aura->m_creator)) {
            continue;
        }

        if ((aura->m_flags & 0x10) ? (filter & BUFF_FILTER_NOT_CANCELABLE) : (filter & BUFF_FILTER_CANCELABLE)) {
            continue;
        }

        if (index >= 0) {
            if (index == found) {
                return auraIndex;
            }

            found++;
            continue;
        }

        SpellRec spell;

        if (g_spellDB.GetRecord(this->GetAuraSpellId(auraIndex), &spell) && !SStrCmpUTF8I(name, spell.m_name, STORM_MAX_STR) && (!rank || !SStrCmpUTF8I(rank, spell.m_nameSubtext, STORM_MAX_STR))) {
            return auraIndex;
        }
    }

    return 0xFFFFFFFF;
}

// VERIFY: 35708e69 auto
// OFFSET: 0x0071AB80
void CGUnit_C::ReleaseAurasBySpell(int32_t spellId) {
    CEffect* effect = this->m_auraEffects;

    while (effect) {
        CEffect* next = effect->m_next;

        if (effect->m_spellId == static_cast<uint32_t>(spellId)) {
            this->m_forceWorldModelAlpha = effect->m_kitRec && (effect->m_kitRec->m_flags & 0x400);
            effect->Release();
        }

        effect = next;
    }
}

// VERIFY: 0c11fba1 auto
// OFFSET: 0x0071BB70
float CGUnit_C::GetSpellChanceToCrit() const {
    float base = GetGameTableValue(5, 0, this->m_unit->m_unitFieldBytes0[1] - 1);
    float ratio = GetGameTableValue(4, this->m_unit->m_unitFieldLevel - 1, this->m_unit->m_unitFieldBytes0[1] - 1);

    if (ratio == 0.0f) {
        return 0.0f;
    }

    int32_t intellect = this->m_unit->m_unitFieldStat[3];
    return (ratio * (intellect < 0 ? 0 : intellect) + base) * 100.0f;
}

// VERIFY: 8a2697bd auto
// OFFSET: 0x00722090
void CGUnit_C::OnAuraRemoved(uint32_t index, uint8_t a3, AuraData* aura, int32_t spellId) {
    SpellRec spell;
    memset(&spell, 0, sizeof(spell));
    g_spellDB.GetRecord(spellId, &spell);
    this->RemoveAuraEffect(index, &spell);

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CGActionBar::OnAuraChanged(spellId);

        if (CGSpellBook::IsTrackingSpell(spellId)) {
            CGMinimapFrame::SetTrackingSpell(0);
        }

        if (CGSpellBook::IsShapeshiftSpell(spellId) && !SpellRec::HasAura(&spell, 36)) {
            FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_SHAPESHIFT_FORM, nullptr);
        }
    }

    UnitCombatLogAuraAddedOrRemoved(this, aura, &spell, a3, 0);
}

// VERIFY: c0256707 auto
// OFFSET: 0x00727760
void CGUnit_C::OnAuraAdded(uint32_t index, uint8_t a3, AuraData* aura, int32_t spellId) {
    SpellRec spell;
    memset(&spell, 0, sizeof(spell));
    g_spellDB.GetRecord(spellId, &spell);
    this->AddAuraEffect(index, &spell);
    UnitCombatLogAuraAddedOrRemoved(this, aura, &spell, a3, 1);

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CGActionBar::OnAuraChanged(spellId);

        if (spellId != CGMinimapFrame::s_trackingSpell && CGSpellBook::IsTrackingSpell(spellId)) {
            CGMinimapFrame::SetTrackingSpell(spellId);
        }

        if (CGSpellBook::IsShapeshiftSpell(spellId) && !SpellRec::HasAura(&spell, 36)) {
            FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_SHAPESHIFT_FORM, nullptr);
        }
    }
}

// VERIFY: 1f07a103 manual
// OFFSET: 0x00727A70
void CGUnit_C::AuraVisionUpdated(uint8_t vision) {
    bool changed = false;

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        SpellRec spell;

        if (!g_spellDB.GetRecord(this->m_auras[i].m_spellId, &spell)) {
            continue;
        }

        uint8_t mask = GetRequiredAuraVisionBitmask(&spell);

        if (!(mask & vision)) {
            continue;
        }

        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (mask & reinterpret_cast<const uint8_t*>(&player->m_player->m_playerFieldBytes2)[3]) {
            this->AddAuraEffect(i, &spell);
        } else {
            this->RemoveAuraEffect(i, &spell);
        }

        changed = true;
    }

    if (changed) {
        Script_SendUniqueSignal(this->m_obj->m_guid, SCRIPT_EVENT_UNIT_AURA);
    }
}

// VERIFY: 9c454b2a manual
// OFFSET: 0x00728E70
void CGUnit_C::ScanAuraSpellsOnDisplayUpdate() {
    SpellRec spell;

    for (uint32_t i = this->GetAuraCount(); i; ) {
        i--;

        int32_t spellId = this->m_auras[i].m_spellId;

        if (spellId && g_spellDB.GetRecord(spellId, &spell)) {
            this->AddAuraEffect(i, &spell);
        }
    }
}

// VERIFY: 975b5867 manual
// OFFSET: 0x00724820
void CGUnit_C::AddAuraEffect(uint32_t index, const SpellRec* spell) {
    uint32_t count = this->m_handItemSpellIds.m_count == 0xFFFFFFFF ? this->m_handItemSpellIds.m_dynamic.m_count : this->m_handItemSpellIds.m_count;

    if (index >= count) {
        return;
    }

    int32_t spellId = this->m_auras[index].m_spellId;
    bool hasSpell = spell != nullptr;

    if (this->m_handItemSpellIds[index]) {
        if (!hasSpell) {
            goto store;
        }

        if (this->m_handItemSpellIds[index] == static_cast<uint32_t>(spell->m_id)) {
            return;
        }

        SpellRec current;

        if (g_spellDB.GetRecord(this->m_handItemSpellIds[index], &current) && current.m_spellPriority > spell->m_spellPriority) {
            return;
        }
    }

    if (hasSpell) {
        uint32_t mask = GetRequiredAuraVisionBitmask(spell);

        if (mask) {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (!player || !(static_cast<uint8_t>(mask) & reinterpret_cast<const uint8_t*>(&player->m_player->m_playerFieldBytes2)[3])) {
                return;
            }
        }
    }

store:
    this->m_handItemSpellIds[index] = spellId;

    if (!hasSpell) {
        return;
    }

    const int32_t* visualIds = spell->m_spellVisualID;

    if (!visualIds) {
        return;
    }

    if (this->IsActivePlayer()) {
        for (uint32_t i = 0; i < 3; i++) {
            switch (spell->m_effectAura[i]) {
                case SPELL_AURA_FAR_SIGHT:
                    CGWorldFrame::GetActiveCamera()->SetTelescope(spell->m_effectMiscValue[i]);
                    break;

                case SPELL_AURA_CHANGE_MODEL_FOR_ALL_HUMANOIDS:
                    EnumVisibleForAuraEffect(spell, spell->m_effectMiscValue[i], spell->m_effectMiscValueB[i]);
                    break;

                case SPELL_AURA_SCREEN_EFFECT:
                    CGWorldFrame::UpdateScreenEffect();
                    break;

                case SPELL_AURA_ABILITY_IGNORE_AURASTATE:
                case SPELL_AURA_MOD_IGNORE_SHAPESHIFT:
                    CGSpellBook::UpdateUsable();
                    break;

                case SPELL_AURA_X_RAY:
                    CGPlayer_C::UpdateXRayVision(spell);
                    break;

                default:
                    break;
            }
        }
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (spell->m_effectAura[i] == SPELL_AURA_MOUNTED) {
            CGSpellBook::SendCompanionUpdate(1);

            if (visualIds[0]) {
                SpellVisualRec* visual = g_spellVisualDB.GetRecord(visualIds[0]);
                SpellVisualKitRec* kit = visual ? g_spellVisualKitDB.GetRecord(visual->m_stateKit) : nullptr;

                if (kit && kit->m_animID >= 0) {
                    this->m_mountAnimBehaviorId = kit->m_animID;
                }
            }
        } else if (spell->m_effectAura[i] == SPELL_AURA_INITIALIZE_IMAGES) {
            WOWGUID guid = this->m_obj->m_guid;
            CGGameUI::UnitNameUpdate(guid);
        }
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (this->GetControllingPlayer() == player || this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        for (uint32_t i = 0; i < 3; i++) {
            if (spell->m_effectAura[i] == SPELL_AURA_PERIODIC_TRIGGER_SPELL_FROM_CLIENT) {
                int32_t period = spell->m_effectAuraPeriod[i];
                Spell_C_ModifySpellValue(spell, period, 19);
                Spell_C_AddPeriodicClientTrigger(this->m_obj->m_guid, spell->m_id, spell->m_effectTriggerSpell[i], period);
            }
        }
    }

    const SpellVisualRec* visual = GetSpellVisual(spell);

    if (!visual) {
        return;
    }

    if (visual->m_hasMissile) {
        for (CMissile* missile = this->m_missiles; missile; missile = missile->m_next) {
            if (missile->m_spellID == static_cast<uint32_t>(spellId)) {
                return;
            }
        }
    }

    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_stateKit);

    if (!kit) {
        return;
    }

    for (uint32_t i = 0; i < 4; i++) {
        if (kit->m_charProc[i] == 11 && !this->m_worldModel->IsLoaded(0, 0)) {
            this->m_pendingModelAuraSpellId = spell->m_id;
            goto handItems;
        }
    }

    {
        PlaySpellVisuaKitData data(spell, kit, 2);
        data.m_someGUID = this->m_auras[index].m_creator;
        this->PlaySpellVisualKit(data);
    }

handItems:
    if (visual->m_flags & 0x8) {
        this->ShowHandItemSpellEffects(1, 1);
    }
}

// VERIFY: e85d416f manual
// OFFSET: 0x0071E930
void CGUnit_C::RemoveAuraEffect(uint32_t index, const SpellRec* spell) {
    uint32_t count = this->m_handItemSpellIds.m_count == 0xFFFFFFFF ? this->m_handItemSpellIds.m_dynamic.m_count : this->m_handItemSpellIds.m_count;

    if (index >= count) {
        return;
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        for (uint32_t i = 0; i < 3; i++) {
            switch (spell->m_effectAura[i]) {
                case SPELL_AURA_FAR_SIGHT:
                    CGWorldFrame::GetActiveCamera()->SetTelescope(0);
                    break;

                case SPELL_AURA_MOUNTED: {
                    CGSpellBook::SendCompanionUpdate(1);

                    if (spell->m_spellVisualID[0]) {
                        SpellVisualRec* visual = g_spellVisualDB.GetRecord(spell->m_spellVisualID[0]);

                        if (visual) {
                            SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_stateKit);

                            if (kit && kit->m_animID >= 0 && this->m_mountAnimBehaviorId == kit->m_animID) {
                                this->m_mountAnimBehaviorId = 91;
                            }
                        }
                    }

                    break;
                }

                case SPELL_AURA_CHANGE_MODEL_FOR_ALL_HUMANOIDS:
                    EnumVisibleForAuraEffect(spell, 0, 0);
                    break;

                case SPELL_AURA_SCREEN_EFFECT:
                    CGWorldFrame::UpdateScreenEffect();
                    break;

                case SPELL_AURA_ABILITY_IGNORE_AURASTATE:
                case SPELL_AURA_MOD_IGNORE_SHAPESHIFT:
                    CGSpellBook::UpdateUsable();
                    break;

                case SPELL_AURA_X_RAY:
                    CGPlayer_C::UpdateXRayVision(spell);
                    break;

                case SPELL_AURA_OPEN_STABLE:
                    CGStableInfo::CloseStables(true);
                    break;

                default:
                    break;
            }
        }
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (spell->m_effectAura[i] == SPELL_AURA_INITIALIZE_IMAGES) {
            WOWGUID guid = this->m_obj->m_guid;
            CGGameUI::UnitNameUpdate(guid);
        }
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (this->GetControllingPlayer() == player || this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        for (uint32_t i = 0; i < 3; i++) {
            if (spell->m_effectAura[i] == SPELL_AURA_PERIODIC_TRIGGER_SPELL_FROM_CLIENT) {
                Spell_C_RemovePeriodicClientTrigger(this->m_obj->m_guid, spell->m_id);
            }
        }
    }

    if (!this->m_handItemSpellIds[index]) {
        return;
    }

    this->ReleaseAurasBySpell(this->m_handItemSpellIds[index]);

    float alpha = this->GetBaseAlpha();
    uint32_t time = 1000;

    if (this->m_auraEffects) {
        alpha *= this->m_auraEffects->m_alpha;

        if (this->m_auraEffects->m_fadeTime) {
            time = this->m_auraEffects->m_fadeTime;
        }
    }

    this->DoFade(alpha, time);

    uint32_t auraSpellId = this->m_handItemSpellIds[index];

    for (CEffect* effect = this->m_procEffects; effect; ) {
        CEffect* next = effect->m_next;

        if (effect->m_spellId == auraSpellId) {
            effect->Release();
        }

        effect = next;
    }

    this->ClearSpellEffect(this->m_handItemSpellIds[index], 1);

    const SpellVisualRec* visual = GetSpellVisual(spell);

    if (visual) {
        SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_stateDoneKit);

        if (kit) {
            PlaySpellVisuaKitData data(nullptr, kit, 8);
            this->PlaySpellVisualKit(data);
        }
    }

    this->m_handItemSpellIds[index] = 0;
}

// VERIFY: 5fefa40f auto
// OFFSET: 0x0072AB60
void CGUnit_C::SetUICastingSpell(int32_t spellId, const WOWGUID& target, int32_t castTime) {
    this->m_castingSpellId = spellId;
    this->m_castingObject = target;
    this->m_castStartTime = FrameTime::s_curTimeMs;
    this->m_castEndTime = castTime + FrameTime::s_curTimeMs;

    int32_t arenaIndex;

    if (this->m_obj->m_guid == CGGameUI::m_lockedTarget || this->m_obj->m_guid == CGGameUI::m_focusTarget || CGBattlefieldInfo::GetArenaOpponentIndex(this->m_obj->m_guid, &arenaIndex)) {
        SpellRec spell;

        if (g_spellDB.GetRecord(spellId, &spell)) {
            this->ProcessCastNotInterruptible(&spell);
        }
    }

    this->m_channelSpellId = 0;
    this->m_channelStartTime = 0;
    this->m_channelEndTime = 0;
    this->UpdateSpellCastBars(FrameTime::s_curTimeMs);
}

// VERIFY: 6ca1311a auto
// OFFSET: 0x0072AD20
void CGUnit_C::SetUIChannelSpell(int32_t spellId, int32_t duration) {
    this->m_channelSpellId = spellId;
    this->m_channelStartTime = FrameTime::s_curTimeMs;
    this->m_channelEndTime = duration + FrameTime::s_curTimeMs;

    int32_t arenaIndex;

    if (this->m_obj->m_guid == CGGameUI::m_lockedTarget || this->m_obj->m_guid == CGGameUI::m_focusTarget || CGBattlefieldInfo::GetArenaOpponentIndex(this->m_obj->m_guid, &arenaIndex)) {
        SpellRec spell;

        if (g_spellDB.GetRecord(spellId, &spell)) {
            this->ProcessCastNotInterruptible(&spell);
        }
    }

    this->m_castingSpellId = 0;
    this->m_castStartTime = 0;
    this->m_castEndTime = 0;
    this->UpdateSpellCastBars(FrameTime::s_curTimeMs);
}

// VERIFY: 4a2466af manual
// OFFSET: 0x0073EB50
void CGUnit_C::OnChannelSpellChanged(int32_t oldSpell) {
    if (oldSpell) {
        this->ReleaseAurasBySpell(oldSpell);

        float alpha = this->GetBaseAlpha();
        uint32_t time = 1000;

        if (this->m_auraEffects) {
            alpha *= this->m_auraEffects->m_alpha;

            if (this->m_auraEffects->m_fadeTime) {
                time = this->m_auraEffects->m_fadeTime;
            }
        }

        this->DoFade(alpha, time);
        this->ReleaseEffectsBySpell(oldSpell);
        this->ClearSpellEffect(oldSpell, 1);

        if (this->m_fishingLineEffect) {
            this->m_fishingLineEffect->Release();
            this->m_fishingLineEffect = nullptr;
        }

        for (uint32_t i = 0; i < (this->m_handItemSpellIds.m_count == 0xFFFFFFFF ? this->m_handItemSpellIds.m_dynamic.m_count : this->m_handItemSpellIds.m_count); i++) {
            if (this->m_handItemSpellIds[i] && this->m_handItemSpellIds[i] == static_cast<uint32_t>(oldSpell)) {
                this->m_handItemSpellIds[i] = 0;
            }
        }
    }

    if (this->m_unit->m_unitChannelSpell) {
        this->UpdateChannelEffects();
        return;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(oldSpell, &spell) && (spell.m_attributesEx & 0x4000) && this->IsActiveMoverTracking(2)) {
        this->ClearTrackingTarget(false, true);
    }

    this->ClearSpellEffect(oldSpell, 1);

    if (this->m_fishingLineEffect) {
        this->m_fishingLineEffect->Release();
        this->m_fishingLineEffect = nullptr;
    }

    // WoWFix335 missing-pre-cast-animation: original ends with UpdateBaseAnimation(0, -1)
}

// VERIFY: 47ba34f4 auto
// OFFSET: 0x00763570
void CGUnit_C::HandleSpellEventSound() {
    if (!this->m_castSpellId) {
        return;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(this->m_castSpellId, &spell)) {
        SysMsgPrintf(SYSMSG_WARNING, 2, "NOSPELLIDFOUND|%d", this->m_castSpellId);
        return;
    }

    SpellVisualRec visualRec;
    const SpellVisualRec* visual = this->GetAppropriateSpellVisual(&spell, visualRec, 0, 0);

    if (!visual) {
        SysMsgPrintf(SYSMSG_WARNING, 2, "SPELLVISUALIDNOTFOUND|%d", spell.m_spellVisualID[0]);
        return;
    }

    if (visual->m_animEventSoundID) {
        C3Vector position = this->GetPosition();
        position.z += 1.0f;
        SI2::PlaySoundKit(visual->m_animEventSoundID, &position, nullptr, nullptr, 0, nullptr, 1, 0);
    }
}

// VERIFY: 28e7526d auto
// OFFSET: 0x0072AF60
int32_t CGUnit_C::PlaySpellVisualKit_DelayLightningEffects(const SpellRec* spell, const SpellVisualKitRec* kit, SPELL_VISUAL_KIT_TYPE type, const C3Vector* position, WOWGUID guid, int32_t a6, int32_t a7, int32_t a8, int32_t a9, uint32_t a10, const SpellVisualRec* visual) {
    if (!spell || !visual || !Spell_C_SpellVisualHasChainEffect(visual) || a8 || !(kit->m_flags & 0x1)) {
        return 0;
    }

    this->DelaySpellVisualKit(spell->m_id, kit->m_id, type, position, guid, a6, a7, a9, 0, FrameTime::s_curTimeMs + 10000);
    return 1;
}

// VERIFY: 6c474fbc auto
// OFFSET: 0x0073A6C0
void CGUnit_C::PlaySpellVisualKit_HandleWeapons(const SpellRec* spell, const SpellVisualKitRec* kit, const C3Vector* position, WOWGUID guid, uint32_t a5, uint32_t flags, void (*callback)(CM2Model*, uint32_t, uint32_t, int32_t, uint32_t, uint64_t), int32_t* hasEffect) {
    int32_t spellId = 0;

    if (spell) {
        spellId = spell->m_id;
    }

    SpellVisualEffectNameRec* effectName = g_spellVisualEffectNameDB.GetRecord(kit->m_leftWeaponEffect);

    if (effectName && !(this->GetModelData()->m_flags & 0x10)) {
        if (!spell || !(spell->m_attributes & 0x40000)) {
            this->SetSheatheState(WEAPONMODE_SHEATHEDMODE, 1, 0);
        }

        this->PlayEffectName(2, 0, spellId, kit, effectName, &flags, callback, position, guid, a5, nullptr);
        *hasEffect = 1;
    }

    effectName = g_spellVisualEffectNameDB.GetRecord(kit->m_rightWeaponEffect);

    if (effectName && !(this->GetModelData()->m_flags & 0x10)) {
        if (!spell || !(spell->m_attributes & 0x40000)) {
            this->SetSheatheState(WEAPONMODE_SHEATHEDMODE, 1, 0);
        }

        this->PlayEffectName(1, 0, spellId, kit, effectName, &flags, callback, position, guid, a5, nullptr);
        *hasEffect = 1;
    }
}

// VERIFY: b9a18a7a manual
// OFFSET: 0x0071D510
static int32_t IsWoundAnim(int32_t animId) {
    AnimationDataRec* anim = g_animationDataDB.GetRecord(animId);
    return anim && anim->m_behaviorID >= 8 && anim->m_behaviorID <= 10;
}

// VERIFY: 810cddf4 manual
// OFFSET: 0x0073B140
void CGUnit_C::PlaySpellVisualKit_PlayAnims(const SpellRec* spell, const SpellVisualKitRec* kit, SPELL_VISUAL_KIT_TYPE type, const C3Vector* position, int32_t a5, int32_t a6, WOWGUID guid, uint32_t a8, const SpellVisualRec* visual, uint32_t a10, int32_t a11) {
    int32_t spellId = 0;

    if (spell) {
        spellId = spell->m_id;
    }

    this->m_animationState &= ~0x4000;

    if (spell && (spell->m_attributes & 0x2) && (type == 4 || type == 0)) {
        this->SetSheatheState(WEAPONMODE_RANGEDMODE, 1, 0);
    }

    if (a5) {
        uint8_t animFlags = 0;

        if (kit->m_flags & 0x4) {
            animFlags = 2;
        }

        if (kit->m_flags & 0x8) {
            animFlags |= 0x4;
        }

        if (kit->m_flags & 0x10) {
            animFlags |= 0x8;
        }

        if (kit->m_flags & 0x100) {
            animFlags |= 0x20;
        }

        if (spell && (spell->m_attributes & 0x40000)) {
            animFlags |= 0x20;
        }

        if (visual && static_cast<uint32_t>(visual->m_missileModel) >= 0xFFFFFFFE) {
            animFlags |= 0x20;
        }

        if ((a10 & 0x40) && kit->m_startAnimID >= 0) {
            this->PlayBaseAnimation(kit->m_startAnimID, animFlags);
            this->m_animationState |= 0x10000;
        } else if (kit->m_animID >= 0) {
            SpellVisualKitRec* castKit = visual ? g_spellVisualKitDB.GetRecord(visual->m_castKit) : nullptr;
            bool sameAnim = castKit && castKit->m_animID > 0 && castKit->m_animID == this->GetCurrentTorsoAnimId();
            bool noCastTime = this->GetSpellCastingTime(spell) <= 0;
            int32_t holdCast = (a10 & 0x40) && !noCastTime && sameAnim;
            int32_t holdChannel = (a10 & 0x1000) && sameAnim;

            if (type == 2) {
                if (!holdChannel && !a11) {
                    CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
                    WOWGUID owner = this->m_obj->m_guid;
                    effect->AddAnimKitEffect(spellId, kit, &owner, a10, a8);
                    effect->Release();
                }

                int32_t animId = kit->m_animID;

                if (this->GetCurrentTorsoAnimId() != animId) {
                    if (IsDeadOrDyingAnim(animId)) {
                        this->PlayBaseAnimation(animId, animFlags);
                    }

                    this->UpdateBaseAnimation(animFlags, -1);
                }
            } else if (!(a10 & 0x40) || (!noCastTime && !holdCast)) {
                if (IsWoundAnim(kit->m_animID)) {
                    this->PlayWoundAnimKit(kit->m_animID == ANIM_COMBAT_CRITICAL);
                } else {
                    this->PlayBaseAnimation(kit->m_animID, animFlags);
                }

                if (a10 & 0x80) {
                    this->m_specialIdleAnim = kit->m_animID;
                }
            }

            if (!a11 && (holdCast || holdChannel)) {
                CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
                WOWGUID owner = this->m_obj->m_guid;
                effect->AddAnimKitEffect(spellId, kit, &owner, a10, a8);
                effect->Release();
            }

            this->m_animationState &= ~0x10000;
        }

        if (a6) {
            this->PlayWoundAnimKit(0);
        }
    }

    this->CreateProceduralEffect(spell, visual, kit, 0, a10, guid);

    if (this->m_pendingWeaponTrail[1]) {
        this->UpdateWeaponTrails(&this->m_pendingWeaponTrail[0], this->m_pendingWeaponTrail[1]);
        this->m_pendingWeaponTrail[1] = 0;
    }

    if (!(a10 & 0x400) && kit->m_soundID) {
        CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
        effect->m_flags = a10;
        effect->m_kitRec = kit;
        effect->AddSoundEffect(kit->m_soundID, 0, spellId, position, this, a8);
        effect->Release();
    }

    if (!a11 && kit->m_shakeID > 0) {
        C3Vector objectPosition;
        const C3Vector* shakePosition = position;

        if (!shakePosition) {
            objectPosition = this->GetPosition();
            shakePosition = &objectPosition;
        }

        SpellVisualsPlayCameraShakeID(kit->m_shakeID, shakePosition);
    }
}

// VERIFY: c1ea52b3 manual
// OFFSET: 0x00736640
void CGUnit_C::PlayWoundAnimKit(int32_t crit) {
    if (this->IsDeadOrAttachmentDying() || !this->m_worldModel || !this->m_worldModel->IsLoaded(0, 0) || this->DoNotPlayWoundAnim()) {
        return;
    }

    for (CEffect* effect = this->m_effectList; effect; effect = effect->m_next) {
        if (effect->m_flags & 0x4) {
            return;
        }
    }

    uint32_t animId;
    uint32_t keyBone = this->m_torsoKeyBone;

    if (crit) {
        animId = 10;
    } else if (this->m_combat.m_attacking.m_guid) {
        animId = 9;
    } else {
        animId = 8;

        if (!this->m_mountModel && !(this->m_movement->m_flags & 0x2E0100F) && !this->m_movementData.IsSplineFlyerOrHovering() && !this->IsSitting() && GetAnimBehaviorId(this->GetCurrentTorsoAnimId()) != 98) {
            keyBone = 0xFFFFFFFF;
        }
    }

    if (keyBone == static_cast<uint32_t>(this->m_torsoKeyBone)) {
        uint32_t sequenceId = this->m_worldModel->GetBoneSequenceId(0xFFFFFFFF);

        if (IsWeaponReadyAnim(sequenceId) || !sequenceId) {
            keyBone = 0xFFFFFFFF;
        }
    }

    uint32_t resolved = this->ResolveAnimForModel(animId, nullptr);

    if (this->m_worldModel->HasSequence(resolved)) {
        this->SetKeyBoneSequence(this->m_worldModel, keyBone, resolved, 0xFFFFFFFF, 0, 1.0f, 0, 0, 0);
    }

    this->UpdateObjectEffectAnimationStates();
}

// VERIFY: fa22acc6 manual
// OFFSET: 0x00715D60
void CGUnit_C::TriggerDamageFeedback() {
    if (this->m_namePlateFrame) {
        this->m_namePlateFrame->TriggerDamageFeedback();
    }
}

// VERIFY: b7d9f2a8 manual
// OFFSET: 0x0072BDB0
int32_t CGUnit_C::VerifyAttackIconPressed(WOWGUID* target, int32_t useGiven, int32_t checkRange, bool autoTarget, bool showErrors) {
    if (this->IsDeadOrGhost()) {
        CGPlayer_C* player;

        if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer() || !(player = ClntObjMgrGetActivePlayerObj()) || !player->GetPossessedUnit()) {
            if (showErrors) {
                CGGameUI::DisplayError(ERR_ATTACK_DEAD);
            }

            return 0;
        }
    } else {
        if ((this->m_obj->m_type & TYPE_PLAYER) && (static_cast<CGPlayer_C*>(this)->m_player->m_playerFlags & 0x1000000)) {
            if (showErrors) {
                CGGameUI::DisplayError(ERR_CLIENT_LOCKED_OUT);
            }

            return 0;
        }

        int32_t mechanic;

        if (this->m_unit->m_unitFieldCharmedby && this->m_unit->m_unitFieldCharmedby != ClntObjMgrGetActivePlayer()) {
            Spell_C_CancelsCharm(this, nullptr, &mechanic);

            if (!showErrors) {
                return 0;
            }

            if (!mechanic) {
                CGGameUI::DisplayError(ERR_ATTACK_CHARMED);
                return 0;
            }
        } else {
            uint32_t flags = this->m_unit->m_unitFieldFlags;

            if (flags & 0x40000) {
                Spell_C_CancelsStun(this, nullptr, &mechanic);

                if (!showErrors) {
                    return 0;
                }

                if (!mechanic) {
                    CGGameUI::DisplayError(ERR_ATTACK_STUNNED);
                    return 0;
                }
            } else if (flags & 0x20000) {
                Spell_C_CancelsPacify(this, nullptr, &mechanic);

                if (!showErrors) {
                    return 0;
                }

                if (!mechanic) {
                    CGGameUI::DisplayError(ERR_ATTACK_PACIFIED);
                    return 0;
                }
            } else if (flags & 0x800000) {
                Spell_C_CancelsFear(this, nullptr, &mechanic);

                if (!showErrors) {
                    return 0;
                }

                if (!mechanic) {
                    CGGameUI::DisplayError(ERR_ATTACK_FLEEING);
                    return 0;
                }
            } else if (flags & 0x400000) {
                Spell_C_CancelsConfuse(this, nullptr, &mechanic);

                if (!showErrors) {
                    return 0;
                }

                if (!mechanic) {
                    CGGameUI::DisplayError(ERR_ATTACK_CONFUSED);
                    return 0;
                }
            } else {
                mechanic = -1;
            }
        }

        if (mechanic != -1) {
            SpellMechanicRec* mechanicRec = g_spellMechanicDB.GetRecord(mechanic);
            CGGameUI::DisplayError(ERR_ATTACK_PREVENTED_BY_MECHANIC_S, mechanicRec ? mechanicRec->m_stateName : "UNKNOWN");
            return 0;
        }

        if (this->m_unit->m_unitChannelSpell) {
            SpellRec spell;

            if (g_spellDB.GetRecord(this->m_unit->m_unitChannelSpell, &spell)) {
                if ((spell.m_attributesEx & 0x1000) && showErrors) {
                    CGGameUI::DisplayError(ERR_ATTACK_CHANNEL);
                }

                return 0;
            }
        }

        if (this->IsMounted() && !this->MountedCombatAllowed() && !this->CanAutoDismount()) {
            if (showErrors) {
                CGGameUI::DisplayError(ERR_ATTACK_MOUNTED);
            }

            return 0;
        }
    }

    if (!useGiven) {
        bool haveTarget = true;

        if (!*target) {
            *target = CGGameUI::m_lockedTarget;
            haveTarget = static_cast<bool>(*target);
        }

        if (haveTarget) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*target, TYPE_UNIT, __FILE__, __LINE__));

            if (unit && this->IsFriendly(unit)) {
                *target = WOWGUID();
            } else if (*target) {
                CGGameUI::Target(*target);
                *target = CGGameUI::m_lockedTarget;
                goto checkTarget;
            }
        }

        if (autoTarget) {
            TargetNearestEnemy(0, 1);
        }

        *target = CGGameUI::m_lockedTarget;
    }

checkTarget:
    if (!*target) {
        if (showErrors) {
            CGGameUI::DisplayError(ERR_NO_ATTACK_TARGET);
        }

        return 0;
    }

    if (this->IsActivePlayer()) {
        CGGameUI::Target(*target);
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*target, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return 0;
    }

    if (this->IsMounted() && !this->MountedCombatAllowed() && checkRange) {
        float minRange;
        float maxRange;
        int32_t spellId;

        if (this->IsActivePlayer() && (spellId = GetModalNextSpell())) {
            Spell_C::GetSpellRange(this, spellId, &minRange, &maxRange, unit);
        } else {
            maxRange = this->GetMeleeRange(unit);
        }

        if (maxRange * maxRange < this->GetDistanceToPos(unit->GetPosition())) {
            if (showErrors) {
                CGGameUI::DisplayError(ERR_OUT_OF_RANGE);
            }

            return 0;
        }
    }

    if (this->CanAttackNow(unit)) {
        return 1;
    }

    if (showErrors) {
        CGGameUI::DisplayError(ERR_INVALID_ATTACK_TARGET);
    }

    return 0;
}

// VERIFY: 34778608 manual
// OFFSET: 0x0072C2B0
int32_t CGUnit_C::OnAttackIconPressed(WOWGUID target, int32_t a3) {
    WOWGUID previousTarget = CGGameUI::m_lockedTarget;
    int32_t targetChanged = 0;

    if (!this->VerifyAttackIconPressed(&target, a3, 1, 1, 1)) {
        return 0;
    }

    if (previousTarget != target) {
        targetChanged = 1;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t controlsPet = 0;

    if (player && player->GetTransportGUID() == CGPetInfo::GetPet(0).m_guid) {
        const WOWGUID& charm = player->m_unit->m_unitFieldCharm ? player->m_unit->m_unitFieldCharm : player->m_unit->m_unitFieldSummon;

        if (charm == CGPetInfo::GetPet(0)) {
            controlsPet = 1;
        }
    }

    int32_t isPlayer = (ClntObjMgrGetActivePlayer() == this->m_obj->m_guid) || controlsPet;
    int32_t petAttack = (CGPetInfo::GetPet(0) == this->m_obj->m_guid) || controlsPet;

    if (isPlayer) {
        if (((player->m_combatModeFlags & 2) || player->m_combat.m_attacking) && CGGameUI::m_hasControl) {
            player->SetCombatMode();
        } else {
            player->CombatModeEnter(target, targetChanged, 0);
        }

        if (petAttack) {
            CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
            CGUnit_C* victim = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPE_UNIT, __FILE__, __LINE__));

            if (!pet || !victim || !pet->CanAttackNow(victim)) {
                petAttack = 0;
            }
        }

        if (player->m_obj->m_guid == CGUnit_C::s_activeMover && player->HasVehicleTransport()) {
            player->CheckVehicleExitOnCast();
        }
    }

    if (petAttack) {
        CGPetInfo::PetAttackTarget(target, a3);

        if (CGPetInfo::GetPet(0) == CGUnit_C::s_activeMover) {
            CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

            if (mover && mover->HasVehicleTransport()) {
                mover->CheckVehicleExitOnCast();
            }
        }
    }

    return 1;
}

// VERIFY: 96b12266 auto
// OFFSET: 0x0072BAC0
bool CGUnit_C::CheckShapeshiftRules(const SpellRec* spell, int32_t* reason) {
    int32_t form = this->m_shapeshiftSuppressed ? 0 : this->m_unit->m_unitFieldBytes2[3];

    if (SpellRec::NotUsableInShapeshiftForm(spell, form - 1)) {
        if (!this->AffectedByAura(SPELL_AURA_MOD_IGNORE_SHAPESHIFT, spell)) {
            *reason = 68;
            return false;
        }

        return true;
    }

    form = this->m_shapeshiftSuppressed ? 0 : this->m_unit->m_unitFieldBytes2[3];

    if (SpellRec::UsableInShapeshift(spell, form - 1)) {
        return true;
    }

    if (this->IsShapeShifted()) {
        if (spell->m_attributes & 0x10000) {
            if (!this->AffectedByAura(SPELL_AURA_MOD_IGNORE_SHAPESHIFT, spell)) {
                *reason = 68;
                return false;
            }

            return true;
        }

        form = this->m_shapeshiftSuppressed ? 0 : this->m_unit->m_unitFieldBytes2[3];
        SpellShapeshiftFormRec* shapeshift = g_spellShapeshiftFormDB.GetRecord(form);

        if (shapeshift && (shapeshift->m_flags & 0x400)) {
            if (!this->AffectedByAura(SPELL_AURA_MOD_IGNORE_SHAPESHIFT, spell)) {
                *reason = 68;
                return false;
            }

            return true;
        }
    } else if (spell->m_attributesExB & 0x80000) {
        return true;
    }

    if ((spell->m_shapeshiftMask[0] || spell->m_shapeshiftMask[1]) && !this->AffectedByAura(SPELL_AURA_MOD_IGNORE_SHAPESHIFT, spell)) {
        *reason = 94;
        return false;
    }

    return true;
}

// VERIFY: b86e26b0 manual
// OFFSET: 0x007191A0
void CGUnit_C::AddWorldXPGainText(int32_t amount) {
    char text[64] = {};
    char label[64];
    SStrCopy(label, FrameScript_GetText("XP", -1, GENDER_NOT_APPLICABLE), sizeof(label));
    SStrPrintf(text, sizeof(text), "%s: %d", label, amount);
    PlayerNameCreateText(this->m_nameDesc, WT_XPGAIN, text, nullptr, nullptr);
}

// VERIFY: b3ec4bfc manual
// OFFSET: 0x0072CD90
UnitThreat* UnitThreatHeap::New() {
    uint32_t memHandle;
    void* memory = nullptr;

    if (!ObjectAlloc(this->m_heapId, &memHandle, &memory, false)) {
        return nullptr;
    }

    UnitThreat* threat = memory ? new (memory) UnitThreat : nullptr;
    threat->m_memHandle = memHandle;

    return threat;
}

// VERIFY: 9e64e02c manual
// OFFSET: 0x007374C0
bool CGUnit_C::CalculateThreat(const WOWGUID& guid, uint8_t* status, uint8_t* rawPercent, float* percent, int32_t* threat) {
    UnitThreat* entry = this->m_threats.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (!this->m_highestThreatGuid || !entry) {
        if (status) {
            *status = entry != nullptr;
        }

        if (rawPercent) {
            *rawPercent = 0;
        }

        if (percent) {
            *percent = 0.0f;
        }

        if (threat) {
            *threat = 0;
        }

        return false;
    }

    if (status) {
        *status = entry->m_status;
    }

    if (rawPercent) {
        *rawPercent = entry->m_percent;
    }

    if (percent) {
        if (guid == this->m_highestThreatGuid) {
            *percent = 100.0f;
        } else {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));
            float range;

            if (unit && (range = this->GetMeleeRange(unit), range * range >= this->GetDistanceToPos(unit->GetPosition()))) {
                *percent = entry->m_percent * 0.90909088f;
            } else {
                *percent = entry->m_percent * 0.76923078f;
            }
        }
    }

    if (threat) {
        *threat = entry->m_threat;
    }

    return true;
}

// VERIFY: ebb6b704 manual
// OFFSET: 0x007345C0
void CGUnit_C::ClearThreatList() {
    if (!this->m_threats.m_fulllist.Head()) {
        return;
    }

    UnitThreat* entry;

    while ((entry = this->m_threats.m_fulllist.Head())) {
        WOWGUID target = entry->m_key.m_guid;
        Script_SendUniqueSignal(target, SCRIPT_EVENT_UNIT_THREAT_SITUATION_UPDATE);
        CGGameUI::ThreatMapRemove(this->m_obj->m_guid, target);
        this->m_threats.Unlink(entry);
        UnitThreatHeap* heap = s_unitThreatPool;
        entry->~UnitThreat();
        ObjectFree(heap->m_heapId, entry->m_memHandle);
    }

    this->m_highestThreatGuid = WOWGUID();
    Script_SendUniqueSignal(this->m_obj->m_guid, SCRIPT_EVENT_UNIT_THREAT_LIST_UPDATE);
    CGGameUI::UnitTooltipUpdate(this->m_obj->m_guid);
}

// VERIFY: 09b44ad3 auto
// OFFSET: 0x00719220
void CGUnit_C::AddWorldThreatUpdateText(int32_t oldStatus, int32_t newStatus) {
    if (!CGGameUI::m_threatWarningEnabled || !g_threatWorldTextCVar->m_intValue) {
        return;
    }

    if ((oldStatus == 2 && newStatus == 1) || (oldStatus == 3 && newStatus == 4)) {
        return;
    }

    char token[32];
    SStrPrintf(token, sizeof(token), "COMBAT_THREAT_%s_%d", newStatus > oldStatus ? "INCREASE" : "DECREASE", newStatus);
    const char* text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);

    if (*text) {
        PlayerNameDeleteText(this->m_nameDesc, WT_THREAT);
        PlayerNameCreateText(this->m_nameDesc, WT_THREAT, text, nullptr, nullptr);
    }
}

// VERIFY: a407c868 manual
// OFFSET: 0x007416F0
UnitThreat* CGUnit_C::CreateThreatEntry(const WOWGUID& guid) {
    UnitThreat* entry = this->m_threats.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (entry) {
        return entry;
    }

    entry = s_unitThreatPool->New();
    entry->m_status = 1;
    entry->m_percent = 0xFF;
    entry->m_threat = 0;
    this->m_threats.Insert(entry, static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (guid == ClntObjMgrGetActivePlayer()) {
        this->m_clientFlags |= 0x8;
    }

    CGGameUI::ThreatMapAdd(this->m_obj->m_guid, guid);
    return entry;
}

// VERIFY: 4feabe73 manual
// OFFSET: 0x0071C3B0
bool CGUnit_C::UpdateThreatEntry(UnitThreat* entry, int32_t highestThreat, bool activePlayer) {
    bool changed = false;
    uint8_t percent;

    if (!highestThreat) {
        percent = 100;
    } else if (entry->m_threat <= 0) {
        percent = 0;
    } else {
        int64_t scaled = 100ll * entry->m_threat / highestThreat;
        percent = scaled >= 250 ? 250 : static_cast<uint8_t>(scaled);
    }

    if (percent == entry->m_percent) {
        return changed;
    }

    entry->m_percent = percent;
    uint8_t status = percent < 100 ? 1 : 2;

    if (status != entry->m_status) {
        if (activePlayer) {
            if (s_cvThreatPlaySounds->m_intValue && status == 2 && entry->m_status == 1) {
                SI2::PlayUISound(15262);
            }

            if (!(this->m_clientFlags & 0x8)) {
                this->AddWorldThreatUpdateText(entry->m_status, status);
            }
        }

        entry->m_status = status;
        Script_SendUniqueSignal(entry->m_key.m_guid, SCRIPT_EVENT_UNIT_THREAT_SITUATION_UPDATE);
        changed = true;
    }

    if (activePlayer) {
        this->m_clientFlags &= ~0x8u;
        CGGameUI::UnitTooltipUpdate(this->m_obj->m_guid);
    }

    return changed;
}

// VERIFY: 1e748a95 manual
// OFFSET: 0x00737620
void CGUnit_C::UpdateThreatStatus() {
    UnitThreat* highest = this->m_threats.Ptr(static_cast<uint32_t>(this->m_highestThreatGuid.m_guid), CHashKeyGUID(this->m_highestThreatGuid.m_guid));

    if (!highest) {
        return;
    }

    uint8_t status = 4;

    for (UnitThreat* entry = this->m_threats.m_fulllist.Head(); entry; entry = this->m_threats.m_fulllist.Next(entry)) {
        if (entry->m_key.m_guid != this->m_highestThreatGuid && entry->m_status == 2) {
            status = 3;
        }
    }

    if (status != highest->m_status) {
        if (this->m_highestThreatGuid == ClntObjMgrGetActivePlayer() && !(this->m_clientFlags & 0x8)) {
            this->AddWorldThreatUpdateText(highest->m_status, status);

            if (s_cvThreatPlaySounds->m_intValue) {
                SI2::PlayUISound(15263);
            }
        }

        highest->m_status = status;
        Script_SendUniqueSignal(this->m_highestThreatGuid, SCRIPT_EVENT_UNIT_THREAT_SITUATION_UPDATE);
    }

    if (this->m_highestThreatGuid == ClntObjMgrGetActivePlayer()) {
        this->m_clientFlags &= ~0x8u;
        CGGameUI::UnitTooltipUpdate(this->m_obj->m_guid);
    }
}

// VERIFY: 84ad87b6 auto
// OFFSET: 0x007417A0
void CGUnit_C::UpdateThreat(CDataStore* msg, bool highest) {
    WOWGUID activePlayer = ClntObjMgrGetActivePlayer();
    bool highestInList = highest;

    if (highest) {
        SmartGUID highestGuid;
        *msg >> highestGuid;
        this->m_highestThreatGuid = highestGuid;

        if (highestGuid) {
            this->CreateThreatEntry(this->m_highestThreatGuid);
        }
    }

    uint32_t count;
    msg->Get(count);
    UnitThreat** entries = static_cast<UnitThreat**>(alloca(count * sizeof(UnitThreat*)));

    for (uint32_t i = 0; i < count; i++) {
        SmartGUID guid;
        *msg >> guid;
        uint32_t threat;
        msg->Get(threat);
        WOWGUID target = guid;
        UnitThreat* entry = this->CreateThreatEntry(target);
        entry->m_threat = threat;

        if (guid == this->m_highestThreatGuid) {
            highestInList = true;
        }

        entries[i] = entry;
    }

    bool changed = false;
    UnitThreat* top = this->m_threats.Ptr(static_cast<uint32_t>(this->m_highestThreatGuid.m_guid), CHashKeyGUID(this->m_highestThreatGuid.m_guid));

    if (this->m_highestThreatGuid && top) {
        int32_t highestThreat = top->m_threat;

        if (highestInList) {
            for (UnitThreat* entry = this->m_threats.m_fulllist.Head(); entry; entry = this->m_threats.m_fulllist.Next(entry)) {
                if (entry->m_key.m_guid != this->m_highestThreatGuid && this->UpdateThreatEntry(entry, highestThreat, entry->m_key.m_guid == activePlayer)) {
                    changed = true;
                }
            }
        } else {
            for (uint32_t i = 0; i < count; i++) {
                UnitThreat* entry = entries[i];

                if (entry->m_key.m_guid != this->m_highestThreatGuid && this->UpdateThreatEntry(entry, highestThreat, entry->m_key.m_guid == activePlayer)) {
                    changed = true;
                }
            }
        }
    }

    if (highest || changed) {
        this->UpdateThreatStatus();
    }

    Script_SendUniqueSignal(this->m_obj->m_guid, SCRIPT_EVENT_UNIT_THREAT_LIST_UPDATE);
}

// VERIFY: 2c8e0d1d manual
// OFFSET: 0x00737750
void CGUnit_C::RemoveThreat(const WOWGUID& guid) {
    UnitThreat* entry = this->m_threats.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (!entry) {
        return;
    }

    bool update = false;

    if (guid == this->m_highestThreatGuid) {
        this->m_highestThreatGuid = WOWGUID();
    } else if (entry->m_status > 1) {
        update = true;
    }

    CGGameUI::ThreatMapRemove(this->m_obj->m_guid, guid);
    this->m_threats.Unlink(entry);
    UnitThreatHeap* heap = s_unitThreatPool;
    entry->~UnitThreat();
    ObjectFree(heap->m_heapId, entry->m_memHandle);
    Script_SendUniqueSignal(guid, SCRIPT_EVENT_UNIT_THREAT_SITUATION_UPDATE);

    if (update) {
        this->UpdateThreatStatus();
    }

    Script_SendUniqueSignal(this->m_obj->m_guid, SCRIPT_EVENT_UNIT_THREAT_LIST_UPDATE);

    if (guid == ClntObjMgrGetActivePlayer()) {
        CGGameUI::UnitTooltipUpdate(this->m_obj->m_guid);
    }
}

// VERIFY: 57fe3eca manual
// OFFSET: 0x00741C90
int32_t Packet_ThreatUpdate(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->UpdateThreat(msg, msgId == SMSG_HIGHEST_THREAT_UPDATE);
    } else {
        msg->m_read = msg->m_size;
    }

    return 1;
}

// VERIFY: 4727a576 manual
// OFFSET: 0x00737B20
int32_t Packet_SMSG_THREAT_REMOVE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        *msg >> guid;
        WOWGUID target = guid;
        unit->RemoveThreat(target);
    } else {
        msg->m_read = msg->m_size;
    }

    return 1;
}

// VERIFY: cff5fd96 manual
// OFFSET: 0x00734B00
int32_t Packet_SMSG_THREAT_CLEAR(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->ClearThreatList();
    }

    return 1;
}

// VERIFY: b7f4a97f manual
// OFFSET: 0x00716B10
int32_t Packet_SMSG_AI_REACTION(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    uint32_t reaction;
    msg->Get(reaction);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        if (reaction == AI_REACT_ALERT) {
            unit->PlayUnitSound(UNITSOUNDTYPE_ALERT, 0);
        } else if (reaction == AI_REACT_HOSTILE) {
            unit->OnAiReaction(0);
        }
    }

    return 1;
}

// VERIFY: f7c851d8 manual
// OFFSET: 0x00716B90
int32_t Packet_SMSG_PET_ACTION_SOUND(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    uint32_t action;
    msg->Get(action);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        if (!action) {
            unit->OnAiReaction(1);
        } else if (action == 1) {
            unit->OnAiReaction(2);
        }
    }

    return 1;
}

// VERIFY: 4b8cff5c manual
// OFFSET: 0x00722D80
void CGUnit_C::DoPowerRegen(POWER_TYPE powerType, uint32_t elapsed) {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && (reinterpret_cast<const uint8_t*>(&static_cast<CGPlayer_C*>(this)->m_player->m_playerFieldBytes2)[2] & (1 << powerType))) {
        return;
    }

    bool interrupted = false;

    if (!powerType) {
        if (this->m_manaRegenInterruptTime) {
            if (static_cast<int32_t>(FrameTime::s_curTimeMs - this->m_manaRegenInterruptTime) < 0) {
                interrupted = true;
            } else {
                this->m_manaRegenInterruptTime = 0;
            }
        }

        int32_t channelSpell = this->m_unit->m_unitChannelSpell;

        if (channelSpell) {
            SpellRec spell;

            if (g_spellDB.GetRecord(channelSpell, &spell) && !(spell.m_attributesExB & 0x2000000) && !spell.m_powerType && Spell_C_GetPowerCost(&spell, this) > 0) {
                interrupted = true;
            }
        }
    } else {
        interrupted = (this->m_unit->m_unitFieldFlags & 0x80000) || (this->m_auraEffectMask[11] & 0x40);
    }

    float regen = this->GetPowerRegen(powerType, interrupted);

    if (fabs(regen) < 0.00000023841858f) {
        return;
    }

    float delta = regen * static_cast<float>(elapsed) * 0.001f;

    if (delta >= 0.0f) {
        int32_t maxPower = powerType == POWER_TYPE_HEALTH ? this->m_unit->m_unitFieldMaxhealth : this->m_unit->m_unitFieldMaxpower[powerType];

        if (this->GetPredictedPower(powerType) < maxPower) {
            int32_t predicted = this->GetPredictedPower(powerType);
            this->m_powerRegenRemainder += static_cast<int32_t>(delta * 1000.0f);
            this->SetPredictedPower(powerType, predicted + this->m_powerRegenRemainder / 1000);
            this->m_powerRegenRemainder %= 1000;
            return;
        }
    }

    if (delta < 0.0f && this->GetPredictedPower(powerType) > 0 && !(this->m_unit->m_unitFieldFlags2 & 0x100)) {
        int32_t predicted = this->GetPredictedPower(powerType);
        float value = delta + (static_cast<float>(this->m_powerRegenRemainder) * 0.001f + static_cast<float>(predicted));

        if (value < 0.0f) {
            value = 0.0f;
        }

        int32_t power = CMath::ftrunc(value);
        this->SetPredictedPower(powerType, power);
        float remainder = value * 1000.0f - static_cast<float>(static_cast<uint32_t>(power * 1000));
        this->m_powerRegenRemainder = CMath::fint_(remainder - 0.5f);
    }
}

// VERIFY: fb2878d0 manual
// OFFSET: 0x00728A20
void CGUnit_C::DoPowerRegen(uint32_t elapsed) {
    if (this->IsDeadOrGhost() || !(this->m_unit->m_unitFieldFlags2 & 0x800)) {
        return;
    }

    POWER_TYPE powerType = static_cast<POWER_TYPE>(this->m_unit->m_unitFieldBytes0[3]);
    this->DoPowerRegen(powerType, elapsed);

    if (this->m_unit->m_unitFieldBytes0[1] == 11) {
        if (powerType) {
            this->DoPowerRegen(POWER_TYPE_MANA, elapsed);
        }

        if (powerType != POWER_TYPE_ENERGY) {
            this->DoPowerRegen(POWER_TYPE_ENERGY, elapsed);
        }
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && powerType == POWER_TYPE_RUNIC_POWER) {
        uint32_t runes = CGRuneInfo::m_predictedRunes;
        uint32_t previous = CGRuneInfo::m_predictedRunes;
        uint32_t now = FrameTime::s_curTimeMs;

        for (int32_t i = 0; i < 8; i++) {
            uint32_t bit = 1 << i;

            if (!(bit & CGRuneInfo::m_predictedRunes) && static_cast<int32_t>(now - CGRuneInfo::GetRuneRechargeTime(i)) >= 0) {
                runes |= bit;
            }
        }

        if (runes != previous) {
            CGRuneInfo::SetPredictedRunes(previous, runes, nullptr);
        }
    }
}

// VERIFY: 08043232 manual
// OFFSET: 0x0072E3A0
void CGUnit_C::OnLevelChange() {
    void* memory = SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0);
    CEffect* effect = memory ? new (memory) CEffect() : nullptr;
    WOWGUID guid = this->m_obj->m_guid;
    effect->AddSpecialEffect(SPECIALEFFECT_LEVELUP, guid, CGObject_C::EffectOnceHandler);
    effect->m_flags |= 0x20;
    effect->Release();

    if (this->m_obj->m_type & TYPE_PLAYER) {
        static_cast<CGPlayer_C*>(this)->OnLevelChange();
    } else {
        this->UpdateModelScale(nullptr);
    }

    if (this->m_namePlateFrame) {
        this->m_namePlateFrame->UpdateLevelDisplay(this);
    }
}

// VERIFY: 0e2e20d4 manual
// OFFSET: 0x00720220
void CGUnit_C::HandleBloodPool(uint32_t time) {
    if (!this->m_bloodlevels) {
        return;
    }

    if (!g_unitBloodDB.GetRecord(this->m_bloodlevels->m_violencelevel[ViolenceGetLevel()])) {
        return;
    }

    if (static_cast<int32_t>(time - this->m_nextBloodPoolTime) < 0) {
        return;
    }

    this->m_nextBloodPoolTime = static_cast<uint32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * 3000) >> 32) + time + 2000;
    CRandom::sfloat_(g_rndSeed);
    CRandom::sfloat_(g_rndSeed);
}

// VERIFY: 92571cc6 auto
// OFFSET: 0x0071BC00
void CGUnit_C::ReadLootList(CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    this->m_masterLooter = guid;
    *msg >> guid;
    this->m_looter = guid;
}

// VERIFY: dcd2c019 manual
// OFFSET: 0x0071CA50
int32_t Packet_SMSG_LOOT_LIST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->ReadLootList(msg);
    } else {
        msg->m_read = msg->m_size;
    }

    return 1;
}

// VERIFY: f3d87698 auto
// OFFSET: 0x00717C20
void CGUnit_C::RemoveLootEffect() {
    for (CEffect* effect = this->m_effectList; effect; effect = effect->m_next) {
        if (effect->m_flags & 0x8) {
            effect->MoveToDestroyList();
            return;
        }
    }
}

// VERIFY: d074dfc8 auto
// OFFSET: 0x0073D530
void CGUnit_C::OnLife(bool a1) {
    this->m_clientFlags &= ~0x1u;
    this->CheckLoopSound();
    this->m_lootableTime = 0;
    this->SetKeyBoneSequenceSpeed(this->m_worldModel, 0xFFFFFFFF, 1.0f, 0);
    this->UpdateBaseAnimation(0, -1);
    PlayerNameTriggerColorUpdate(this->m_nameDesc);

    if (this->m_namePlateFrame) {
        this->m_namePlateFrame->UpdateColorDisplay(this);
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && !a1) {
        SendQuestGiverStatusMultipleQuery();
        CGGameUI::UnlockAllItems();
        FrameScript_SignalEvent(257, nullptr);
    }
}

// VERIFY: 8d2dd6f2 manual
// OFFSET: 0x00740570
void CGUnit_C::OnDynamicFlagsChanged(uint32_t oldFlags) {
    uint32_t flags = this->m_unit->m_unitDynamicFlags;

    if ((oldFlags & 1) && !(flags & 1)) {
        this->RemoveLootEffect();
    } else if (!(oldFlags & 1) && (flags & 1)) {
        CGTutorial::TriggerTutorial(6);
    }

    if ((oldFlags ^ this->m_unit->m_unitDynamicFlags) & 0x20) {
        Script_SendUnitSignal(this->m_obj->m_guid, 18);
        Script_SendUnitSignal(this->m_obj->m_guid, this->m_unit->m_unitFieldBytes0[3] + 19);

        if (this->m_unit->m_unitDynamicFlags & 0x20) {
            this->OnDeath();
        } else {
            this->OnLife(true);
        }
    }

    if (static_cast<int32_t>(this->m_unit->m_unitFieldHealth) <= 0 && (this->m_unit->m_unitDynamicFlags & 1)) {
        if (this->m_lootableTime) {
            this->ClearEffectList(0);
        }

        this->ShowLootEffect();
    }

    if ((oldFlags ^ this->m_unit->m_unitDynamicFlags) & 4) {
        Script_SendUnitSignal(this->m_obj->m_guid, 49);
    }
}

// VERIFY: 7f96f710 manual
// OFFSET: 0x00718F00
static int32_t PetColorUpdateProc(WOWGUID guid, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return 1;
    }

    const WOWGUID& owner = unit->m_unit->m_unitFieldCharmedby ? unit->m_unit->m_unitFieldCharmedby : unit->m_unit->m_unitFieldCreatedby;

    if (owner == static_cast<CGUnit_C*>(param)->m_obj->m_guid && !(unit->m_unit->m_unitFieldFlags & 0x100)) {
        PlayerNameTriggerColorUpdate(unit->m_nameDesc);

        if (unit->m_namePlateFrame) {
            unit->m_namePlateFrame->UpdateColorDisplay(unit);
        }

        Script_SendUnitSignal(unit->m_obj->m_guid, 49);
    }

    return 1;
}

// VERIFY: 56f522e1 auto
// OFFSET: 0x00718FB0
void CGUnit_C::UpdatePetReaction() {
    ClntObjMgrEnumVisibleObjects(PetColorUpdateProc, this);
}

// VERIFY: 632e6ebd manual
// OFFSET: 0x00717BA0
void CGUnit_C::ShowLootEffect() {
    if (!(this->m_unit->m_unitDynamicFlags & 1)) {
        return;
    }

    for (CEffect* effect = this->m_effectList; effect; effect = effect->m_next) {
        if (effect->m_flags & 0x8) {
            return;
        }
    }

    void* memory = SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0);
    CEffect* effect = memory ? new (memory) CEffect() : nullptr;
    effect->AddLootEffect(this, nullptr);
    effect->Release();
}

// VERIFY: 6fa47b02 auto
// OFFSET: 0x007412B0
void CGUnit_C::CompleteMountTransition() {
    if (this->m_mountTransitionEffect) {
        this->OnMountDisplayChanged(this->m_mountTransitionEffect->m_displayId);
    }

    this->m_mountTransitionEffect = nullptr;
    this->m_mountTransition = nullptr;
}

// VERIFY: 845aa7ca manual
// OFFSET: 0x00723730
void CGUnit_C::RefreshComponentItem(int32_t itemSlot) {
    if (this->CanEquipItemsInThisForm() && (this->m_obj->m_type & TYPE_PLAYER)) {
        if (!this->GetBag()) {
            return;
        }

        INVENTORY_SLOTS invSlot = CCharacterComponent::GetInvSlotForItemSlot(static_cast<ITEM_SLOT>(itemSlot));

        if (invSlot == -1) {
            return;
        }

        this->m_characterComponent->RemoveItem(static_cast<ITEM_SLOT>(itemSlot));
        const VisibleItem_C* item = static_cast<CGPlayer_C*>(this)->GetVisibleItem(invSlot);

        if (item && abs(item->m_itemId)) {
            static_cast<CGPlayer_C*>(this)->EquipVisibleItem(item, invSlot);
        }
    } else if (this->m_characterComponent && this->m_displayInfoExtra) {
        this->m_characterComponent->RemoveItem(static_cast<ITEM_SLOT>(itemSlot));
        int32_t displayId = this->m_displayInfoExtra->m_npcitemDisplay[itemSlot];

        if (displayId) {
            this->m_characterComponent->AddItem(static_cast<ITEM_SLOT>(itemSlot), displayId, 0);
        }
    }
}

// VERIFY: 0f53e72a manual
// OFFSET: 0x0072E430
void CGUnit_C::ClearCastingSpell(int32_t spellId, bool clearChannel, bool a4) {
    if (spellId != this->m_castSpellId) {
        return;
    }

    this->m_animationState &= ~0x400u;
    this->m_castCount = 0;
    this->m_castSpellId = 0;
    this->m_castSchoolImmunities = 0;
    this->m_castImmunities = 0;

    if (!a4) {
        return;
    }

    if (clearChannel) {
        if (this->IsAutoTracking() && !CGUnit_C::s_trackingType) {
            CGObject_C* target = ClntObjMgrObjectPtr(CGUnit_C::s_trackingGUID, TYPE_OBJECT, __FILE__, __LINE__);

            if (target) {
                C3Vector targetPosition = target->GetPosition();
                C3Vector position = this->GetPosition();
                float facing = CalculateFacingTo(&position, &targetPosition);
                C3Vector none = C3Vector();
                WOWGUID guid = WOWGUID();
                static_cast<CGPlayer_C*>(this)->ClickToMove(TRACKTYPE_FACE, &guid, &none, facing);
            }
        }

        SpellRec spell;

        if (g_spellDB.GetRecord(this->m_unit->m_unitChannelSpell, &spell) && (spell.m_attributesEx & 0x4000) && this->IsActiveMoverTracking(2)) {
            this->ClearTrackingTarget(false, true);
        }
    }

    this->m_chainSpellTargets.SetCount(0);

    if (spellId != Spell_C_GetAutoRepeatingSpell()) {
        this->CancelRangedMode();
    }

    if (this->m_fishingLineEffect) {
        this->m_fishingLineEffect->Release();
        this->m_fishingLineEffect = nullptr;
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CGGameUI::CloseLoot(true, (this->m_movement->m_flags & 0xC0100F) != 0, false);
    }
}

// VERIFY: d1a0635a manual
// OFFSET: 0x0072DF00
void CGUnit_C::ReleaseMissiles(const C3Vector& position) {
    if (this->m_missiles) {
        this->CancelRangedMode();
    }

    SpellRec spell;

    while (this->m_missiles) {
        CM2Model* model = this->m_missiles->m_model;

        if (model) {
            if (!model->IsLoaded(0, 0)) {
                this->m_clientFlags |= 0x800;
                break;
            }

            this->m_missiles->m_model->m_fadeAlpha = 1.0f;
        }

        if (this->IsMissileTrajectoryUpdateWaitingForMissileRelease() && this->m_missileTrajectoryReleased == this->m_missiles->m_castID) {
            this->SendFinalMissileTrajectoryUpdate();
        }

        PendingSpellCast* cast = Spell_C_GetDelayedTrajectoryCast();

        if (cast && this->m_missiles->m_castID == cast->m_data.m_castCount) {
            if (!Spell_C_SendDelayedMissileTrajectoryCast(this)) {
                if (!this->m_missiles) {
                    break;
                }

                if (this->m_precastMissileModel == this->m_missiles->m_model) {
                    if (this->m_precastMissileModel) {
                        if (this->m_precastMissileModel->m_attachParent) {
                            this->m_precastMissileModel->DetachFromParent();
                        }

                        this->m_precastMissileModel->Release();
                        this->m_precastMissileModel = nullptr;
                        this->m_precastCastCount = 0;
                    }

                    this->m_clientFlags &= ~0x1000000u;
                }

                this->m_missiles->DeleteSelf();
                continue;
            }

            if (!this->m_missiles) {
                break;
            }
        }

        CM2Model* precast = this->m_precastMissileModel;

        if (precast && precast == this->m_missiles->m_model) {
            precast->Animate();
            C44Matrix transform = precast->m_matrixF4 * precast->m_scene->m_viewInv;

            if (this->m_precastMissileModel) {
                if (this->m_precastMissileModel->m_attachParent) {
                    this->m_precastMissileModel->DetachFromParent();
                }

                this->m_precastMissileModel->Release();
                this->m_precastMissileModel = nullptr;
                this->m_precastCastCount = 0;
            }

            this->m_clientFlags &= ~0x1000000u;
            this->m_missiles->m_flags |= 0x2000000;
            CM2Model* missileModel = this->m_missiles->m_model;
            missileModel->m_fFlags |= 0x8000;
            missileModel->m_worldTransform = transform;
            C3Vector firePosition(transform.d0, transform.d1, transform.d2);
            this->m_missiles->FireMissiles(&firePosition);
            continue;
        }

        if (!g_spellDB.GetRecord(this->m_missiles->m_spellID, &spell)) {
            continue;
        }

        C3Vector overridePosition = C3Vector();

        if (this->OverrideMissileFirePos(&overridePosition, &spell)) {
            this->m_missiles->FireMissiles(&overridePosition);
        } else {
            this->m_missiles->FireMissiles(&position);
        }
    }

    if (this->m_precastMissileModel) {
        if (this->m_precastMissileModel->m_attachParent) {
            this->m_precastMissileModel->DetachFromParent();
        }

        this->m_precastMissileModel->Release();
        this->m_precastMissileModel = nullptr;
        this->m_precastCastCount = 0;
    }

    this->m_clientFlags &= ~0x1000000u;
}

// VERIFY: 402547fa manual
// OFFSET: 0x0073A520
void CGUnit_C::OnChannelObjectChanged(WOWGUID oldObject, int32_t oldSpell) {
    CGObject_C* object = ClntObjMgrObjectPtr(oldObject, TYPE_OBJECT, __FILE__, __LINE__);

    if (object && oldSpell) {
        object->ClearEffectKit(oldSpell, 2, 0, 0, 0);
        object->ClearEffectKit(oldSpell, 1, 0, 0, 0);
        object->ClearEffectKit(oldSpell, 5, 0, 0, 0);
        object->ClearEffectKit(oldSpell, 6, 0, 0, 0);
    }

    if (this->m_fishingLineEffect) {
        this->m_fishingLineEffect->Release();
        this->m_fishingLineEffect = nullptr;
    }

    CGGameObject_C* gameObject = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(this->m_unit->m_unitFieldChannelObject, TYPE_GAMEOBJECT, __FILE__, __LINE__));

    if (gameObject && this->m_unit->m_unitChannelSpell && gameObject->m_gameObject->m_typeID == 17) {
        if (this->m_sheatheState == WEAPONMODE_NORMALMODE) {
            this->CreateFishingLineEffect();
        } else {
            this->SetSheatheState(WEAPONMODE_NORMALMODE, 1, 0);
        }

        return;
    }

    if (!ClntObjMgrObjectPtr(this->m_unit->m_unitFieldChannelObject, TYPE_UNIT, __FILE__, __LINE__)) {
        return;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(this->m_unit->m_unitChannelSpell, &spell) && (spell.m_attributesEx & 0x4000) && this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        WOWGUID target = this->m_unit->m_unitFieldChannelObject;
        this->SetTrackingTarget(target, TRACKTYPE_SPELLCHANNEL);
    }
}

// VERIFY: 694d4b34 manual
// OFFSET: 0x0072BC70
void CGUnit_C::UpdateChannelEffects() {
    this->m_clientFlags &= ~0x8000u;
    SpellRec spell;

    if (!g_spellDB.GetRecord(this->m_unit->m_unitChannelSpell, &spell)) {
        return;
    }

    if (spell.m_attributesEx & 0x4000) {
        this->m_clientFlags |= 0x8000;

        if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            WOWGUID target = this->m_unit->m_unitFieldChannelObject;
            this->SetTrackingTarget(target, TRACKTYPE_SPELLCHANNEL);
        }
    }

    const SpellVisualRec* visual = GetSpellVisual(&spell);

    if (!visual) {
        return;
    }

    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_channelKit);

    if (!kit) {
        return;
    }

    uint32_t playAnim = 1;
    SpellVisualKitRec* castKit = g_spellVisualKitDB.GetRecord(visual->m_castKit);

    if (castKit && this->GetCurrentTorsoAnimId() == static_cast<uint32_t>(castKit->m_animID)) {
        playAnim = 0;
    }

    PlaySpellVisuaKitData data(&spell, kit, 2);
    data.m_playAnim = playAnim;
    this->PlaySpellVisualKit(data);
    this->CreateFishingLineEffect();
}

// VERIFY: 66625199 manual
// OFFSET: 0x0071F8B0
bool CGUnit_C::CanInteract(const CGGameObject_C* gameObject) const {
    return (!(this->m_obj->m_type & TYPE_PLAYER) || !(static_cast<const CGPlayer_C*>(this)->m_player->m_playerFlags & 0x10)) && gameObject->ObjectReaction(this) >= UNIT_REACTION_NEUTRAL;
}

// VERIFY: a3dabf99 manual
// OFFSET: 0x00718E50
static int32_t UnitColorUpdateProc(WOWGUID guid, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit || unit->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        return 1;
    }

    const WOWGUID& owner = unit->m_unit->m_unitFieldCharmedby ? unit->m_unit->m_unitFieldCharmedby : unit->m_unit->m_unitFieldCreatedby;

    if (owner != ClntObjMgrGetActivePlayer()) {
        PlayerNameTriggerColorUpdate(unit->m_nameDesc);

        if (unit->m_namePlateFrame) {
            unit->m_namePlateFrame->UpdateColorDisplay(unit);
        }

        Script_SendUnitSignal(unit->m_obj->m_guid, 49);
    }

    return 1;
}

// VERIFY: 550305c8 auto
// OFFSET: 0x0071F8F0
void CGUnit_C::UpdateReaction(bool a1) {
    PlayerNameTriggerColorUpdate(this->m_nameDesc);

    if (this->m_namePlateFrame) {
        this->m_namePlateFrame->UpdateColorDisplay(this);
    }

    Script_SendUnitSignal(this->m_obj->m_guid, 49);

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        SendQuestGiverStatusMultipleQuery();
        ClntObjMgrEnumVisibleObjects(UnitColorUpdateProc, nullptr);
        return;
    }

    if (a1) {
        ClntObjMgrEnumVisibleObjects(PetColorUpdateProc, this);
    }

    this->RefreshInteractIcon();
}

// VERIFY: 8da8cbd4 manual
// OFFSET: 0x00728F70
void CGUnit_C::OnFlagChangedActivePlayer(uint32_t oldFlags, uint32_t changed) {
    if (changed & 0x40000) {
        uint32_t time = OsGetAsyncTimeMs();
        CGInputControl::GetActive()->UpdatePlayer(time, 1);
    }

    if (changed & 0x1C20000) {
        CGSpellBook::UpdateUsable();
    }

    if (changed & 0x80000) {
        CGSpellBook::UpdateUsable();
        CGGameUI::SetRegenEnabled(!(this->m_unit->m_unitFieldFlags & 0x80000));
    }

    if (changed & 0x100000) {
        if (this->m_unit->m_unitFieldFlags & 0x100000) {
            this->ClearTrackingTarget(false, true);
        } else if (!CGGameUI::m_inCinematic) {
            CGChat::ZoneUpdate();
        }
    }
}

// VERIFY: a548f0b4 manual
// OFFSET: 0x0073C330
void CGUnit_C::OnFlagChanged(uint32_t oldFlags) {
    uint32_t flags = this->m_unit->m_unitFieldFlags;
    uint32_t changed = oldFlags ^ flags;

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        this->OnFlagChangedActivePlayer(flags, changed);
    } else if (static_cast<int32_t>(this->m_unit->m_unitFieldHealth) > 0 && (changed & 0x400)) {
        this->UpdateBaseAnimation(0, -1);
    }

    if (changed & 0x200000) {
        const CGUnitVirtualItem* mainHand = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, true);

        if (mainHand && mainHand->m_classID == 2) {
            const CGUnitVirtualItem* item;

            if ((this->m_unit->m_unitFieldFlags & 0x200000) && (item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, true)) && item->m_classID == 2 && this->m_sheatheState <= WEAPONMODE_NORMALMODE) {
                CCharacterComponent::RemoveHandItem(this->m_worldModel, INVSLOT_MAINHAND, static_cast<SHEATHE_TYPE>(this->m_sheatheState ? 0 : mainHand->m_sheatheType), false);
            } else {
                if (this->unk_0988) {
                    this->unk_0988->DeleteSelf();
                }

                this->AddHandItem(0);
            }
        }

        const CGUnitVirtualItem* offHand = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, true);

        if (offHand && offHand->m_classID == 2) {
            const CGUnitVirtualItem* item;

            if ((((this->m_unit->m_unitFieldFlags & 0x200000) && (this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, true), !this->IsDisarmed(0)) && (item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, true)) && item->m_classID == 2) || static_cast<int8_t>(this->m_unit->m_unitFieldFlags2) < 0) && this->m_sheatheState == WEAPONMODE_NORMALMODE) {
                CCharacterComponent::RemoveHandItem(this->m_worldModel, INVSLOT_OFFHAND, SHEATHE_NONE, false);
            } else {
                this->AddHandItem(1);
            }
        }

        if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            CGSpellBook::UpdateUsable();
        }
    }

    if (changed & 0x180) {
        this->UpdateReaction(false);
    }

    if (changed & 0x800) {
        const WOWGUID& owner = this->m_unit->m_unitFieldCharmedby ? this->m_unit->m_unitFieldCharmedby : this->m_unit->m_unitFieldSummonedby;

        if (owner == ClntObjMgrGetActivePlayer()) {
            CGPetInfo::SetIsAttackingTarget((flags >> 11) & 1);
        }
    }

    if (changed & 0x4000000) {
        CGGameUI::UnitTooltipUpdate(this->m_obj->m_guid);
    }

    if (changed & 0x8000000) {
        Script_SendUniqueSignal(this->m_obj->m_guid, 142);
    }

    if ((changed & 0x2000000) && (this->m_unit->m_unitFieldFlags & 0x2000000)) {
        CGGameUI::BreakTarget(this->m_obj->m_guid);
    }
}

// VERIFY: a4d452c0 manual
// OFFSET: 0x00734A40
void CGUnit_C::AnimEventCallback(CM2Model* model, uint32_t boneId, uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time, uint64_t param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ param }, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    if (std::isnan(static_cast<double>(position->x)) || std::isnan(static_cast<double>(position->y)) || std::isnan(static_cast<double>(position->z))) {
        unit->GetUnitName(nullptr, 1);
    }

    unit->HandleAnimEvent(boneId, eventId, data, position, time);
}

// VERIFY: 88971327 manual
// OFFSET: 0x00732650
void CGUnit_C::HandleAnimEvent(uint32_t boneId, uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time) {
    switch (eventId) {
        case 0x48414324:
        case 0x48544424:
        case 0x50504324:
        case 0x50574224:
        case 0x53534324:
        case 0x30484124:
        case 0x31484124:
        case 0x32484124:
        case 0x33484124:
            this->HandleCombatAnimEvent(eventId, data, position, time);
            return;

        case 0x31444624:
        case 0x32444624:
        case 0x33444624:
        case 0x34444624:
        case 0x35444624:
        case 0x36444624:
        case 0x37444624:
        case 0x38444624:
        case 0x39444624:
        case 0x58444624:
            this->HandlePlayStandSound(eventId, data);
            return;

        case 0x4C534324:
        case 0x52534324:
        case 0x54534324:
            if (this->m_missiles) {
                this->SetAnimEventPosition(position);
            }
            return;

        case 0x304C4224:
        case 0x304C4624:
        case 0x304C5224:
        case 0x304C5324:
        case 0x304C5724:
        case 0x314C4224:
        case 0x314C4624:
        case 0x314C5224:
        case 0x314C5324:
        case 0x314C5724:
        case 0x324C4224:
        case 0x324C4624:
        case 0x324C5224:
        case 0x324C5324:
        case 0x324C5724:
        case 0x334C4224:
        case 0x334C4624:
        case 0x334C5224:
        case 0x334C5324:
        case 0x334C5724:
            this->FootstepAnimEventHit(position, 1);
            return;

        case 0x30524224:
        case 0x30524624:
        case 0x30525224:
        case 0x30525324:
        case 0x30525724:
        case 0x31524224:
        case 0x31524624:
        case 0x31525224:
        case 0x31525324:
        case 0x31525724:
        case 0x32524224:
        case 0x32524624:
        case 0x32525224:
        case 0x32525324:
        case 0x32525724:
        case 0x33524224:
        case 0x33524624:
        case 0x33525224:
        case 0x33525324:
        case 0x33525724:
            this->FootstepAnimEventHit(position, 0);
            return;

        case 0x30475624:
        case 0x31475624:
        case 0x32475624:
        case 0x33475624:
        case 0x34475624:
        case 0x35475624:
        case 0x36475624:
        case 0x37475624:
        case 0x38475624:
            if (this->m_vehicle && this->m_vehicle->m_vehicleRec) {
                this->m_vehicle->HandleBoneAnimGrabEvent(boneId, eventId);
            }
            return;

        case 0x30545624:
        case 0x31545624:
        case 0x32545624:
        case 0x33545624:
        case 0x34545624:
        case 0x35545624:
        case 0x36545624:
        case 0x37545624:
        case 0x38545624:
            if (this->GetVehicleRec()) {
                this->m_vehicle->HandleBoneAnimThrowEvent(boneId, eventId);
            }
            return;

        case 0x44435324:
            if (this->m_soundData) {
                SI2::PlaySoundKit(this->m_soundData->m_spellCastDirectedSoundID, position, nullptr, nullptr, 0, nullptr, 1, 0);
            }
            return;

        case 0x4C485324:
            this->ExchangeSheathedWeapon(position, 1, 0x200000);
            return;

        case 0x52485324:
            this->ExchangeSheathedWeapon(position, 0, 0x100000);
            return;

        case 0x48544224: {
            if (ClntObjMgrGetPlayerType() == 1 || !this->m_modelData || (this->m_modelData->m_flags & 2)) {
                return;
            }

            int32_t type;

            if ((this->m_obj->m_type & TYPE_PLAYER) && static_cast<CGPlayer_C*>(this)->GetDrunknessTurnValue() >= 0.5f) {
                type = SPECIALEFFECT_INEBRIATEDBUBBLES;
            } else if (this->m_clientFlags & 0x20) {
                type = SPECIALEFFECT_UNDERWATERBUBBLES;
            } else if (this->m_clientFlags & 0x40) {
                type = SPECIALEFFECT_COLDBREATH;
            } else {
                return;
            }

            void* memory = SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0);
            CEffect* effect = memory ? new (memory) CEffect() : nullptr;
            WOWGUID guid = this->m_obj->m_guid;
            effect->AddSpecialEffect(type, guid, CGObject_C::EffectOnceHandler);
            effect->m_flags |= 0x20;
            effect->Release();
            return;
        }

        case 0x474D5324:
            if (this->m_soundData) {
                SI2::PlaySoundKit(this->m_soundData->m_submergeSoundID, position, nullptr, nullptr, 0, nullptr, 1, 0);
            }
            return;

        case 0x444D5324:
            if (this->m_soundData) {
                SI2::PlaySoundKit(this->m_soundData->m_submergedSoundID, position, nullptr, nullptr, 0, nullptr, 1, 0);
            }
            return;

        case 0x474E5724:
            this->PlayUnitSound(UNITSOUNDTYPE_WINGFLAP, 0);
            return;

        case 0x47475724:
            this->PlayUnitSound(UNITSOUNDTYPE_WINGGLIDE, 0);
            return;

        case 0x44534524:
            this->PlayEmoteStateSound(position);
            return;

        case 0x44534624:
            this->HandleFootfallAnimEvent(*position);
            return;

        case 0x44534324:
            this->PlayEmoteSound(data, true, -1);
            return;

        case 0x44525424:
            if (this->m_obj->m_type & TYPE_PLAYER) {
                static_cast<CGPlayer_C*>(this)->HandleSpellEventSound();
            }
            return;

        case 0x54524224:
            if (this->m_soundData) {
                SI2::PlaySoundKit(this->m_soundData->m_birthSoundID, position, nullptr, nullptr, 0, nullptr, 1, 0);
            }
            return;

        case 0x52574224: {
            if (!CGUnit_C::s_trackingType) {
                this->ClearTrackingTarget(false, true);
            }

            if (this->m_animationState & 0x4000) {
                this->m_animationState &= ~0x4000u;
            }

            if (this->m_missiles) {
                C3Vector eventPosition;

                if (IsBowAnim(this->GetCurrentTorsoAnimId()) && this->m_worldModel->HasAttachment(35)) {
                    eventPosition = this->m_worldModel->GetAttachmentPosition(35);

                    if (this->m_rangedWeaponModel) {
                        this->m_rangedWeaponModel->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
                    }
                } else if (IsRifleAnim(this->GetCurrentTorsoAnimId()) && this->m_worldModel->HasAttachment(36)) {
                    eventPosition = this->m_worldModel->GetAttachmentPosition(36);

                    if (this->m_rangedWeaponModel) {
                        this->m_rangedWeaponModel->SetBoneSequence(0xFFFFFFFF, 161, 0xFFFFFFFF, 0, 1.0f, 1, 1);
                    }
                } else {
                    eventPosition = *position;
                }

                this->SetAnimEventPosition(&eventPosition);
            }

            if (this->m_ammoModel && this->m_ammoModel->m_attachParent) {
                this->m_ammoModel->DetachFromParent();
            }

            return;
        }

        default:
            return;
    }
}

// VERIFY: 927340f4 auto
// OFFSET: 0x0073B050
void CGUnit_C::PlayParryAnimation() {
    if (this->IsDeadOrAttachmentDying() || !this->m_worldModel->IsLoaded(0, 0) || this->DoNotPlayWoundAnim()) {
        return;
    }

    const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, false);

    if (!item || item->m_classID != 2 || !this->m_unit->m_unitFieldBytes2[0]) {
        this->PlayBaseAnimation(ANIM_PARRY_UNARMED, 0);
        return;
    }

    uint32_t twoHandAnim = ANIM_PARRY_2H - (this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, false) != nullptr);

    switch (item->m_subclassID) {
        case 0:
        case 4:
        case 7:
        case 11:
        case 14:
        case 15:
        case 20:
            this->PlayBaseAnimation(ANIM_PARRY_1H, 0);
            break;

        case 1:
        case 5:
        case 8:
        case 12:
            this->PlayBaseAnimation(twoHandAnim, 0);
            break;

        case 6:
        case 10:
        case 17:
            this->PlayBaseAnimation(ANIM_PARRY_2HL, 0);
            break;

        case 13:
            this->PlayBaseAnimation(ANIM_PARRY_UNARMED, 0);
            break;

        default:
            break;
    }
}

// VERIFY: bb462764 manual
// OFFSET: 0x00732FF0
void CGUnit_C::CreateMissile(const WOWGUID* target, PendingSpellCastData* data, uint32_t ammoDisplayId, uint32_t ammoInventoryType, int32_t hit, uint32_t missReason, uint32_t reflectResult, const SpellVisualRec* visual, int32_t spellId, const TSStackArray<WOWGUID>* targets, const TSStackArray<uint32_t>* missReasons, const TSStackArray<uint32_t>* reflectResults, uint32_t delay) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    float scale = 1.0f;
    CM2Model* model = nullptr;

    if (this->m_precastMissileModel && !(this->m_clientFlags & 0x1000000)) {
        this->m_precastMissileModel->m_refCount++;
        this->m_clientFlags |= 0x1000000;
        model = this->m_precastMissileModel;
        SpellVisualEffectNameRec* effectName = g_spellVisualEffectNameDB.GetRecord(visual->m_missileModel);

        if (effectName && *effectName->m_fileName) {
            scale = effectName->m_scale;
        }
    } else if (visual->m_missileModel) {
        switch (visual->m_missileModel) {
            case -5:
            case -4:
                model = this->CreateItemModel(ammoDisplayId, ammoInventoryType, nullptr);
                break;

            case -3:
                model = this->CreateItemModel(2, &scale);
                break;

            case -2:
                model = this->CreateItemModel(1, &scale);
                break;

            case -1:
                model = this->CreateItemModel(0, &scale);
                break;

            default: {
                SpellVisualEffectNameRec* effectName = g_spellVisualEffectNameDB.GetRecord(visual->m_missileModel);

                if (effectName && *effectName->m_fileName) {
                    model = World::s_m2Scene->CreateModel(effectName->m_fileName, 0x40);
                    scale = effectName->m_scale;
                }

                break;
            }
        }
    } else if (ammoDisplayId) {
        ItemDisplayInfoRec displayRec;

        if (g_itemDisplayInfoDB.GetRecord(ammoDisplayId, &displayRec)) {
            model = CCharacterComponent::MakeAmmoModel(this->m_worldModel, &displayRec, ammoInventoryType);
        }
    }

    if (!model) {
        model = World::s_m2Scene->CreateModel("Spells\\ErrorCube.mdx", 0);
    }

    AnimationDataRec* animation = g_animationDataDB.GetRecord(this->GetCurrentTorsoAnimId());

    if (animation && (animation->m_behaviorID == 49 || animation->m_behaviorID == 106 || animation->m_behaviorID == 110)) {
        model->m_fadeAlpha = 0.0f;
    }

    CMissile* missile = CMissile::NewMissile();
    missile->m_model = model;
    missile->m_casterGuid = this->m_obj->m_guid;
    missile->m_targetGuid = *target;
    missile->m_spellID = spell.m_id;
    missile->m_castID = data->m_castCount;
    missile->m_speed = spell.m_speed;
    missile->m_baseSpeed = spell.m_speed;
    missile->m_hitResult = missReason;
    missile->m_missReason = reflectResult;

    if (hit) {
        missile->m_flags |= 1;
    }

    if (spell.m_attributes & 2) {
        const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false);

        if (item && item->m_classID == 2 && (item->m_subclassID == 2 || item->m_subclassID == 18 || item->m_subclassID == 16)) {
            missile->m_flags |= 4;
        }
    }

    if (missReason == 11 && (spell.m_attributesExG & 0x20000000)) {
        missile->m_flags |= 0x8000000;
    }

    missile->m_scale = scale;
    missile->m_soundKitID = visual->m_missileSound;
    const SpellVisualRec* spellVisual = GetSpellVisual(&spell);

    if (spellVisual) {
        float height = spellVisual->m_missileFollowGroundHeight * 0.001f;

        if (height <= MISSILE_MIN_HEIGHT) {
            height = MISSILE_MIN_HEIGHT;
        }

        missile->m_groundOffset = height;
        float rate = spellVisual->m_missileFollowGroundDropSpeed * 0.001f;

        if (rate <= 0.01f) {
            rate = 0.01f;
        }

        missile->m_followGroundRate = rate;
        float approach = 0.001f * spellVisual->m_missileFollowGroundApproach;

        if (approach < 0.0f) {
            approach = 0.0f;
        } else if (approach >= 1.0f) {
            approach = 1.0f;
        }

        missile->m_followGroundStart = approach;
        missile->m_motionFlags = spellVisual->m_missileFollowGroundFlags;

        if (spellVisual->m_missileFollowGroundFlags & 1) {
            missile->m_flags |= 0x7800;
        }
    }

    int32_t attachment = (visual->m_flags & 0x200) ? visual->m_missileDestinationAttachment : s_effectAttachmentIds[visual->m_missileDestinationAttachment];
    C3Vector offset(visual->m_missileImpactOffset[0], -visual->m_missileImpactOffset[1], visual->m_missileImpactOffset[2]);
    bool hasOffset = 0.0f != offset.x || 0.0f != offset.y || offset.z != 0.0f;

    if (*target) {
        missile->m_attachmentID = attachment;
        missile->m_attachOffset = offset;
        CGObject_C* object = ClntObjMgrObjectPtr(*target, TYPE_OBJECT, __FILE__, __LINE__);

        if (object) {
            missile->m_impactPos = object->GetPosition();
        }
    } else {
        if ((data->m_targetFlags & 0x60) == 0x60) {
            missile->m_position = data->m_sourcePos;
            missile->m_impactPos = data->m_destPos;
            missile->m_transportGuid = data->m_destTarget;
        } else if (data->m_targetFlags & 0x40) {
            missile->m_impactPos = data->m_destPos;
            missile->m_transportGuid = data->m_destTarget;
        } else if (data->m_targetFlags & 0x20) {
            missile->m_impactPos = data->m_sourcePos;
            missile->m_transportGuid = data->m_sourceTarget;
        } else {
            if (!targets->m_count) {
                missile->DeleteSelf();
                return;
            }

            missile->m_targetGuid = targets->m_data[0];
            missile->m_attachmentID = attachment;
            missile->m_attachOffset = offset;
            CGObject_C* object = ClntObjMgrObjectPtr(targets->m_data[0], TYPE_OBJECT, __FILE__, __LINE__);

            if (object) {
                missile->m_impactPos = object->GetPosition();
            }

            goto targetsDone;
        }

        missile->m_flags |= 1;
        missile->m_attachmentID = -1;

        if (hasOffset) {
            C44Matrix matrix = this->m_vehiclePassenger ? this->GetModelMatrix() : this->GetMatrix();
            matrix.d0 = 0.0f;
            matrix.d1 = 0.0f;
            matrix.d2 = 0.0f;
            C3Vector rotated = offset * matrix;
            missile->m_impactPos.x = missile->m_impactPos.x + rotated.x;
            missile->m_impactPos.y = rotated.y + missile->m_impactPos.y;
            missile->m_impactPos.z = rotated.z + missile->m_impactPos.z;
        }
    }

targetsDone:
    uint32_t multiTarget = spell.m_attributesExD & 0x40000;

    if (!*target && targets->m_count) {
        if (!multiTarget && !(data->m_targetFlags & 0x60)) {
            missile->m_flags &= ~0x200u;
            goto flagsDone;
        }

        missile->m_targets.Set(targets->m_count, reinterpret_cast<const uint64_t*>(targets->m_data));

        if (missReasons && missReasons->m_count) {
            missile->m_targetHitResults.Set(missReasons->m_count, missReasons->m_data);
        }

        if (reflectResults && reflectResults->m_count) {
            missile->m_targetMissReasons.Set(reflectResults->m_count, reflectResults->m_data);
        }
    }

    if (multiTarget) {
        missile->m_flags |= 0x200;
    } else {
        missile->m_flags &= ~0x200u;
    }

flagsDone:
    missile->m_currentTarget = 0;
    int32_t missileCount = 1;

    if (spellVisual && spellVisual->m_missileMotion) {
        missile->m_flags |= 0x3800;
        missile->InstallMotionScriptFunction(spellVisual->m_missileMotion);
        SpellMissileMotionRec* motion = g_spellMissileMotionDB.GetRecord(spellVisual->m_missileMotion);

        if (motion && motion->m_missileCount >= 1) {
            missileCount = motion->m_missileCount;
        }
    }

    missile->m_castTime = OsGetAsyncTimeMs();
    missile->SetReadyToFire(&this->m_missiles);
    this->m_lastMissileTime = missile->m_castTime;

    if (data->m_castFlags & 0x20000) {
        missile->m_pitch = data->m_elevation;
        missile->m_flags |= 0x10800;
        missile->m_trajectorySpeed = data->m_missileSpeed;
        missile->m_arrivalTime = delay + missile->m_castTime;
    }

    if ((this->m_clientFlags & 0x400) || (this->m_obj->m_guid == CGPetInfo::GetPet(0) && !this->m_unit->m_unitFieldCharmedby)) {
        missile->m_flags |= 0x200000;
    }

    missile->m_missileCount = missileCount;

    if (missileCount > 1) {
        missile->m_missileIndex = 0;

        for (int32_t i = 1; i < missileCount; i++) {
            missile = CMissile::CopyMissile(missile);
            missile->m_missileIndex = i;
            missile->SetReadyToFire(&this->m_missiles);
        }
    }

    if (visual->m_flags & 4) {
        C3Vector position = this->GetMissileFirePosition();
        this->ReleaseMissiles(position);
    }
}

// VERIFY: 9a6a877b manual
// OFFSET: 0x00733820
void CGUnit_C::CreateMissile(const SpellCastTargetPoint* point, PendingSpellCastData* data, uint32_t ammoDisplayId, uint32_t ammoInventoryType, const SpellVisualRec* visual, int32_t spellId, uint32_t delay) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    float scale = 1.0f;
    CM2Model* model = nullptr;

    if (this->m_precastMissileModel && !(this->m_clientFlags & 0x1000000)) {
        this->m_precastMissileModel->m_refCount++;
        this->m_clientFlags |= 0x1000000;
        model = this->m_precastMissileModel;
        SpellVisualEffectNameRec* effectName = g_spellVisualEffectNameDB.GetRecord(visual->m_missileModel);

        if (effectName && *effectName->m_fileName) {
            scale = effectName->m_scale;
        }
    } else if (visual->m_missileModel) {
        switch (visual->m_missileModel) {
            case -5:
            case -4:
                model = this->CreateItemModel(ammoDisplayId, ammoInventoryType, nullptr);
                break;

            case -3:
                model = this->CreateItemModel(2, &scale);
                break;

            case -2:
                model = this->CreateItemModel(1, &scale);
                break;

            case -1:
                model = this->CreateItemModel(0, &scale);
                break;

            default: {
                SpellVisualEffectNameRec* effectName = g_spellVisualEffectNameDB.GetRecord(visual->m_missileModel);

                if (effectName && *effectName->m_fileName) {
                    model = World::s_m2Scene->CreateModel(effectName->m_fileName, 0x40);
                    scale = effectName->m_scale;
                }

                break;
            }
        }
    } else if (ammoDisplayId) {
        ItemDisplayInfoRec displayRec;

        if (g_itemDisplayInfoDB.GetRecord(ammoDisplayId, &displayRec)) {
            model = CCharacterComponent::MakeAmmoModel(this->m_worldModel, &displayRec, ammoInventoryType);
        }
    }

    if (!model) {
        model = World::s_m2Scene->CreateModel("Spells\\ErrorCube.mdx", 0);
    }

    AnimationDataRec* animation = g_animationDataDB.GetRecord(this->GetCurrentTorsoAnimId());

    if (animation && (animation->m_behaviorID == 49 || animation->m_behaviorID == 106 || animation->m_behaviorID == 110)) {
        model->m_fadeAlpha = 0.0f;
    }

    CMissile* missile = CMissile::NewMissile();
    missile->m_model = model;
    missile->m_casterGuid = this->m_obj->m_guid;
    missile->m_targetGuid = WOWGUID();
    missile->m_spellID = spell.m_id;
    missile->m_castID = data->m_castCount;
    missile->m_speed = spell.m_speed;
    missile->m_baseSpeed = spell.m_speed;
    missile->m_flags |= 1;
    missile->m_hitResult = 0;
    missile->m_missReason = 0;

    if (spell.m_attributes & 2) {
        const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false);

        if (item && item->m_classID == 2 && (item->m_subclassID == 2 || item->m_subclassID == 18 || item->m_subclassID == 16)) {
            missile->m_flags |= 4;
        }
    }

    missile->m_scale = scale;
    missile->m_soundKitID = visual->m_missileSound;
    const SpellVisualRec* spellVisual = GetSpellVisual(&spell);

    if (spellVisual) {
        float height = spellVisual->m_missileFollowGroundHeight * 0.001f;

        if (height <= MISSILE_MIN_HEIGHT) {
            height = MISSILE_MIN_HEIGHT;
        }

        missile->m_groundOffset = height;
        float rate = spellVisual->m_missileFollowGroundDropSpeed * 0.001f;

        if (rate <= 0.01f) {
            rate = 0.01f;
        }

        missile->m_followGroundRate = rate;
        float approach = 0.001f * spellVisual->m_missileFollowGroundApproach;

        if (approach < 0.0f) {
            approach = 0.0f;
        } else if (approach >= 1.0f) {
            approach = 1.0f;
        }

        missile->m_followGroundStart = approach;
        missile->m_motionFlags = spellVisual->m_missileFollowGroundFlags;

        if (spellVisual->m_missileFollowGroundFlags & 1) {
            missile->m_flags |= 0x7800;
        }
    }

    C3Vector offset(visual->m_missileImpactOffset[0], -visual->m_missileImpactOffset[1], visual->m_missileImpactOffset[2]);
    bool hasOffset = 0.0f != offset.x || 0.0f != offset.y || offset.z != 0.0f;
    missile->m_impactPos = point->m_position;
    missile->m_transportGuid = point->m_transport;
    missile->m_flags |= 1;
    missile->m_attachmentID = -1;

    if (hasOffset) {
        C44Matrix matrix = this->m_vehiclePassenger ? this->GetModelMatrix() : this->GetMatrix();
        matrix.d0 = 0.0f;
        matrix.d1 = 0.0f;
        matrix.d2 = 0.0f;
        C3Vector rotated = offset * matrix;
        missile->m_impactPos.x = missile->m_impactPos.x + rotated.x;
        missile->m_impactPos.y = rotated.y + missile->m_impactPos.y;
        missile->m_impactPos.z = rotated.z + missile->m_impactPos.z;
    }

    missile->m_flags &= ~0x200u;
    missile->m_currentTarget = 0;
    int32_t missileCount = 1;

    if (spellVisual && spellVisual->m_missileMotion) {
        missile->m_flags |= 0x3800;
        missile->InstallMotionScriptFunction(spellVisual->m_missileMotion);
        SpellMissileMotionRec* motion = g_spellMissileMotionDB.GetRecord(spellVisual->m_missileMotion);

        if (motion && motion->m_missileCount >= 1) {
            missileCount = motion->m_missileCount;
        }
    }

    missile->m_castTime = OsGetAsyncTimeMs();
    missile->SetReadyToFire(&this->m_missiles);
    this->m_lastMissileTime = missile->m_castTime;

    if (data->m_castFlags & 0x20000) {
        missile->m_pitch = data->m_elevation;
        missile->m_flags |= 0x10800;
        missile->m_trajectorySpeed = data->m_missileSpeed;
        missile->m_arrivalTime = delay + missile->m_castTime;
    }

    if ((this->m_clientFlags & 0x400) || (this->m_obj->m_guid == CGPetInfo::GetPet(0) && !this->m_unit->m_unitFieldCharmedby)) {
        missile->m_flags |= 0x200000;
    }

    missile->m_missileCount = missileCount;

    if (missileCount > 1) {
        missile->m_missileIndex = 0;

        for (int32_t i = 1; i < missileCount; i++) {
            missile = CMissile::CopyMissile(missile);
            missile->m_missileIndex = i;
            missile->SetReadyToFire(&this->m_missiles);
        }
    }

    if (visual->m_flags & 4) {
        C3Vector position = this->GetMissileFirePosition();
        this->ReleaseMissiles(position);
    }
}

// VERIFY: e1531680 manual
// OFFSET: 0x00733E20
void CGUnit_C::CreateMissile(const TSStackArray<SpellCastTargetPoint>* points, PendingSpellCastData* data, uint32_t ammoDisplayId, uint32_t ammoInventoryType, const SpellVisualRec* visual, int32_t spellId, uint32_t delay) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    float scale = 1.0f;
    CM2Model* model = nullptr;

    if (this->m_precastMissileModel && !(this->m_clientFlags & 0x1000000)) {
        this->m_precastMissileModel->m_refCount++;
        this->m_clientFlags |= 0x1000000;
        model = this->m_precastMissileModel;
        SpellVisualEffectNameRec* effectName = g_spellVisualEffectNameDB.GetRecord(visual->m_missileModel);

        if (effectName && *effectName->m_fileName) {
            scale = effectName->m_scale;
        }
    } else if (visual->m_missileModel) {
        switch (visual->m_missileModel) {
            case -5:
            case -4:
                model = this->CreateItemModel(ammoDisplayId, ammoInventoryType, nullptr);
                break;

            case -3:
                model = this->CreateItemModel(2, &scale);
                break;

            case -2:
                model = this->CreateItemModel(1, &scale);
                break;

            case -1:
                model = this->CreateItemModel(0, &scale);
                break;

            default: {
                SpellVisualEffectNameRec* effectName = g_spellVisualEffectNameDB.GetRecord(visual->m_missileModel);

                if (effectName && *effectName->m_fileName) {
                    model = World::s_m2Scene->CreateModel(effectName->m_fileName, 0x40);
                    scale = effectName->m_scale;
                }

                break;
            }
        }
    } else if (ammoDisplayId) {
        ItemDisplayInfoRec displayRec;

        if (g_itemDisplayInfoDB.GetRecord(ammoDisplayId, &displayRec)) {
            model = CCharacterComponent::MakeAmmoModel(this->m_worldModel, &displayRec, ammoInventoryType);
        }
    }

    if (!model) {
        model = World::s_m2Scene->CreateModel("Spells\\ErrorCube.mdx", 0);
    }

    AnimationDataRec* animation = g_animationDataDB.GetRecord(this->GetCurrentTorsoAnimId());

    if (animation && (animation->m_behaviorID == 49 || animation->m_behaviorID == 106 || animation->m_behaviorID == 110)) {
        model->m_fadeAlpha = 0.0f;
    }

    CMissile* missile = CMissile::NewMissile();
    missile->m_model = model;
    missile->m_casterGuid = this->m_obj->m_guid;
    missile->m_targetGuid = WOWGUID();
    missile->m_spellID = spell.m_id;
    missile->m_castID = data->m_castCount;
    missile->m_speed = spell.m_speed;
    missile->m_baseSpeed = spell.m_speed;
    missile->m_flags |= 1;
    missile->m_hitResult = 0;
    missile->m_missReason = 0;

    if (spell.m_attributes & 2) {
        const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false);

        if (item && item->m_classID == 2 && (item->m_subclassID == 2 || item->m_subclassID == 18 || item->m_subclassID == 16)) {
            missile->m_flags |= 4;
        }
    }

    missile->m_scale = scale;
    missile->m_soundKitID = visual->m_missileSound;
    const SpellVisualRec* spellVisual = GetSpellVisual(&spell);

    if (spellVisual) {
        float height = spellVisual->m_missileFollowGroundHeight * 0.001f;

        if (height <= MISSILE_MIN_HEIGHT) {
            height = MISSILE_MIN_HEIGHT;
        }

        missile->m_groundOffset = height;
        float rate = spellVisual->m_missileFollowGroundDropSpeed * 0.001f;

        if (rate <= 0.01f) {
            rate = 0.01f;
        }

        missile->m_followGroundRate = rate;
        float approach = 0.001f * spellVisual->m_missileFollowGroundApproach;

        if (approach < 0.0f) {
            approach = 0.0f;
        } else if (approach >= 1.0f) {
            approach = 1.0f;
        }

        missile->m_followGroundStart = approach;
        missile->m_motionFlags = spellVisual->m_missileFollowGroundFlags;

        if (spellVisual->m_missileFollowGroundFlags & 1) {
            missile->m_flags |= 0x7800;
        }
    }

    int32_t attachment = (visual->m_flags & 0x200) ? visual->m_missileDestinationAttachment : s_effectAttachmentIds[visual->m_missileDestinationAttachment];
    missile->m_impactPos = points->m_data[0].m_position;
    missile->m_transportGuid = points->m_data[0].m_transport;
    missile->m_flags |= 1;
    missile->m_attachmentID = attachment;
    missile->m_attachOffset = C3Vector(visual->m_missileImpactOffset[0], -visual->m_missileImpactOffset[1], visual->m_missileImpactOffset[2]);
    missile->m_targetPoints.Set(points->m_count, points->m_data);
    missile->m_flags |= 0x200;
    missile->m_currentTarget = 0;
    int32_t missileCount = 1;

    if (spellVisual && spellVisual->m_missileMotion) {
        missile->m_flags |= 0x3800;
        missile->InstallMotionScriptFunction(spellVisual->m_missileMotion);
        SpellMissileMotionRec* motion = g_spellMissileMotionDB.GetRecord(spellVisual->m_missileMotion);

        if (motion && motion->m_missileCount >= 1) {
            missileCount = motion->m_missileCount;
        }
    }

    missile->m_castTime = OsGetAsyncTimeMs();
    missile->SetReadyToFire(&this->m_missiles);
    this->m_lastMissileTime = missile->m_castTime;

    if (data->m_castFlags & 0x20000) {
        missile->m_pitch = data->m_elevation;
        missile->m_flags |= 0x10800;
        missile->m_trajectorySpeed = data->m_missileSpeed;
        missile->m_arrivalTime = delay + missile->m_castTime;
    }

    if ((this->m_clientFlags & 0x400) || (this->m_obj->m_guid == CGPetInfo::GetPet(0) && !this->m_unit->m_unitFieldCharmedby)) {
        missile->m_flags |= 0x200000;
    }

    missile->m_missileCount = missileCount;

    if (missileCount > 1) {
        missile->m_missileIndex = 0;

        for (int32_t i = 1; i < missileCount; i++) {
            missile = CMissile::CopyMissile(missile);
            missile->m_missileIndex = i;
            missile->SetReadyToFire(&this->m_missiles);
        }
    }

    if (visual->m_flags & 4) {
        C3Vector position = this->GetMissileFirePosition();
        this->ReleaseMissiles(position);
    }
}

// VERIFY: 79c14509 auto
// OFFSET: 0x007157E0
void CGUnit_C::ChatBubbleRemove() {
    if (this->m_chatBubble) {
        CGChatBubbleFrame::Remove(this->m_chatBubble);
        this->m_chatBubble = nullptr;
    }
}

// VERIFY: 82092375 auto
// OFFSET: 0x00720010
void CGUnit_C::ChatBubbleShow(int32_t type, const char* text) {
    if (this->m_namePlateFrame || !text || !*text || !this->m_worldObject) {
        return;
    }

    switch (type) {
        case 1:
        case 2:
        case 6:
        case 12:
        case 14:
        case 51: {
            CVar* cvar = (type == 2 || type == 51) ? g_chatBubblesPartyCVar : g_chatBubblesCVar;

            if (!cvar->m_intValue) {
                return;
            }

            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (!player) {
                return;
            }

            C3Vector delta = this->GetPosition() - player->GetPosition();

            if (delta.y * delta.y + delta.z * delta.z + delta.x * delta.x <= 625.0f) {
                CImVector color = CGChat::GetChatColor(type);
                this->ChatBubbleRemove();
                this->m_chatBubble = CGChatBubbleFrame::GetNewChatBubble(CGWorldFrame::s_currentWorldFrame);
                WOWGUID guid = this->m_obj->m_guid;
                this->m_chatBubble->Initialize(guid, text, color);
            }

            break;
        }

        default:
            break;
    }
}

// VERIFY: 336fa0a3 manual
// OFFSET: 0x00616B10
uint32_t CGUnit_C::GetPortraitDisplayId() {
    uint32_t displayId = this->m_displayId;

    if (!displayId || this->m_unit->m_unitFieldNativedisplayid != this->m_unit->m_unitFieldDisplayid) {
        return this->m_unit->m_unitFieldDisplayid;
    }

    return displayId;
}

// VERIFY: 74cc072f auto
// OFFSET: 0x0071ACF0
bool CGUnit_C::IsTracked() const {
    return CGMinimapFrame::s_trackingInfo && CGMinimapFrame::s_trackingInfo->m_type == 1 && (this->m_unit->m_unitNpcFlags & CGMinimapFrame::s_trackingInfo->m_npcFlags);
}

// VERIFY: feea4653 manual
// OFFSET: 0x0071D450
int32_t IsUnarmedAnim(uint32_t animId) {
    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);
    int32_t behavior = rec ? rec->m_behaviorID : 506;

    switch (behavior) {
        case 16:
        case 20:
        case 25:
        case 117:
        case 118:
            return 1;

        default:
            return 0;
    }
}

// VERIFY: 12551dce auto
// OFFSET: 0x00715850
void CGUnit_C::ClearWeaponTrail(int32_t hand) {
    if (this->m_weaponTrails[hand]) {
        WeaponTrailClose(this->m_weaponTrails[hand]);
        this->m_weaponTrails[hand] = nullptr;
    }
}

// VERIFY: 84f2e615 auto
// OFFSET: 0x00721ED0
int32_t CGUnit_C::IsPlayingUnarmedAnim() {
    uint32_t sequence = 0xFFFFFFFF;

    if (this->m_worldModel && this->m_worldModel->IsLoaded(0, 0)) {
        if (this->m_torsoKeyBone == -1 || (sequence = this->m_worldModel->GetBoneSequenceId(this->m_torsoKeyBone)) == 0xFFFFFFFF) {
            sequence = this->m_worldModel->GetBoneSequenceId(0xFFFFFFFF);
        }
    }

    return IsUnarmedAnim(sequence) || (IsMeleeAttackAnim(sequence) && !this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, false));
}

// VERIFY: 570e3c87 manual
// OFFSET: 0x0072DBC0
void CGUnit_C::AddHandItem(int32_t hand) {
    if ((this->m_modelData->m_flags & 0x10) || !this->m_worldModel) {
        return;
    }

    const CGUnitVirtualItem* item = this->GetVirtualItem(static_cast<VIRTUAL_MONSTER_SLOT>(hand), false);

    if (!item) {
        return;
    }

    ItemDisplayInfoRec displayRec;

    if (!this->GetVirtualItemDisplayRec(static_cast<VIRTUAL_MONSTER_SLOT>(hand), displayRec)) {
        return;
    }

    SHEATHE_TYPE sheatheType = static_cast<SHEATHE_TYPE>(item->m_sheatheType);
    bool sheathed;

    if (this->m_sheatheState == WEAPONMODE_RANGEDMODE) {
        sheathed = hand != 2;
    } else if (hand == 1 && this->IsPlayingUnarmedAnim()) {
        sheathed = CGUnit_C::ResolveSheathState(this->m_sheatheState, this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, false), item) == WEAPONMODE_SHEATHEDMODE;
    } else {
        sheathed = this->m_sheatheState == WEAPONMODE_SHEATHEDMODE;
    }

    bool shield = item->m_inventoryType == 14;
    bool ranged = item->m_inventoryType == 26 || item->m_inventoryType == 25;
    uint32_t attachId;
    INVENTORY_SLOTS invSlot;
    bool mainHand;

    if (hand == 0) {
        const CGUnitVirtualItem* mainItem;

        if ((this->m_unit->m_unitFieldFlags & 0x200000) && (mainItem = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, true)) && mainItem->m_classID == 2) {
            return;
        }

        attachId = 1;
        invSlot = static_cast<INVENTORY_SLOTS>(15);
        mainHand = true;
    } else if (hand == 1) {
        const CGUnitVirtualItem* offItem;

        if (this->m_unit->m_unitFieldFlags & 0x200000) {
            this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, true);

            if (!this->IsDisarmed(0) && (offItem = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, true)) && offItem->m_classID == 2) {
                return;
            }
        }

        if (this->m_unit->m_unitFieldFlags2 & 0x80) {
            return;
        }

        const CGUnitVirtualItem* mainItem;

        if (!(this->m_obj->m_type & TYPE_PLAYER) && (mainItem = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, false)) && mainItem->m_classID == 2) {
            uint8_t subclass = mainItem->m_subclassID;

            if (subclass == 1 || subclass == 5 || subclass == 8 || subclass == 10 || subclass == 12 || subclass == 6 || subclass == 17 || subclass == 20) {
                return;
            }
        }

        attachId = 2;
        invSlot = static_cast<INVENTORY_SLOTS>(16);
        mainHand = false;
    } else {
        if (this->m_sheatheState != WEAPONMODE_RANGEDMODE) {
            CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(17), static_cast<SHEATHE_TYPE>(0), false);
            return;
        }

        if (!this->IsDisarmed(!ranged)) {
            this->ShowRangedWeapon(false, nullptr);
        }

        return;
    }

    CM2Model* detached = nullptr;

    if (this->m_sheatheState == WEAPONMODE_RANGEDMODE) {
        for (CM2Model* attached = this->m_worldModel->m_attachList; attached; attached = attached->m_attachNext) {
            if (attached->m_attachmentId == attachId) {
                attached->m_refCount++;
                detached = attached;
                attached->DetachFromParent();
                break;
            }
        }
    }

    GEOCOMPONENTLINKS link = CCharacterComponent::AddHandItem(this->m_worldModel, &displayRec, invSlot, sheatheType, sheathed, shield, ranged, 0);

    if (this->m_characterComponent) {
        this->m_characterComponent->SetItemSlotUsingAttachSlot(static_cast<ITEM_SLOT>(-1), link);
    }

    if (this->m_obj->m_type & TYPE_PLAYER) {
        GEOCOMPONENTLINKS visualLink = sheathed ? CCharacterComponent::GetSheatheLink(static_cast<SHEATHE_TYPE>(item->m_sheatheType), mainHand) : static_cast<GEOCOMPONENTLINKS>(attachId);
        static_cast<CGPlayer_C*>(this)->UpdateHandItemVisual(visualLink, hand);
    }

    if (detached) {
        detached->AttachToParent(this->m_worldModel, attachId, nullptr, 0);
        detached->Release();
    }
}

// VERIFY: 7add54cf manual
// OFFSET: 0x005140A0
CGUnit_C* CGUnit_C::GetVehicleUnit() const {
    return this->m_vehiclePassenger ? this->m_vehiclePassenger->GetVehicleUnit() : nullptr;
}

// VERIFY: a1d87fb0 auto
// OFFSET: 0x0071BAE0
float CGUnit_C::GetMeleeChanceToCrit() const {
    float base = GetGameTableValue(3, 0, this->m_unit->m_unitFieldBytes0[1] - 1);
    float ratio = GetGameTableValue(2, this->m_unit->m_unitFieldLevel - 1, this->m_unit->m_unitFieldBytes0[1] - 1);

    if (ratio == 0.0f) {
        return 0.0f;
    }

    int32_t agility = this->m_unit->m_unitFieldStat[1] < 0 ? 0 : this->m_unit->m_unitFieldStat[1];
    return (ratio * agility + base) * 100.0f;
}

// VERIFY: 0dddf6dd manual
// OFFSET: 0x0071B8D0
double CGUnit_C::GetHealthModifier() {
    if (this->m_unit->m_unitFieldPetnumber) {
        CGUnit_C* owner = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_unit->m_unitFieldCreatedby, TYPE_PLAYER, __FILE__, __LINE__));

        if (owner && owner->m_unit->m_unitFieldBytes0[1] == 3) {
            return 1.0;
        }
    }

    WOWGUID none = WOWGUID();
    CreatureStats_C* stats = g_creatureDBCache.GetRecord(this->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (stats) {
        return stats->m_hpModifier;
    }

    return 0.0;
}

// VERIFY: 213318bc manual
// OFFSET: 0x0071B960
double CGUnit_C::GetPowerModifier() {
    if (this->m_unit->m_unitFieldPetnumber) {
        CGUnit_C* owner = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_unit->m_unitFieldCreatedby, TYPE_PLAYER, __FILE__, __LINE__));

        if (owner && owner->m_unit->m_unitFieldBytes0[1] == 3) {
            return 1.0;
        }
    }

    WOWGUID none = WOWGUID();
    CreatureStats_C* stats = g_creatureDBCache.GetRecord(this->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (stats) {
        return stats->m_mpModifier;
    }

    return 0.0;
}

// VERIFY: 1cb19458 manual
// OFFSET: 0x0071B9F0
float CGUnit_C::GetManaRegenRateFromSpirit() const {
    int32_t spirit = this->m_unit->m_unitFieldStat[4] < 0 ? 0 : this->m_unit->m_unitFieldStat[4];
    int32_t intellect = this->m_unit->m_unitFieldStat[3] < 0 ? 0 : this->m_unit->m_unitFieldStat[3];
    return GetGameTableValue(10, this->m_unit->m_unitFieldLevel - 1, this->m_unit->m_unitFieldBytes0[1] - 1) * sqrt(static_cast<double>(intellect)) * spirit;
}

// VERIFY: b1a33141 manual
// OFFSET: 0x0071BA60
float CGUnit_C::GetHealthRegenRateFromSpirit() const {
    int32_t spirit = this->m_unit->m_unitFieldStat[4] < 0 ? 0 : this->m_unit->m_unitFieldStat[4];
    int32_t baseSpirit = spirit <= s_baseSpiritRegenCap ? spirit : s_baseSpiritRegenCap;
    int32_t classIndex = this->m_unit->m_unitFieldBytes0[1] - 1;
    int32_t levelIndex = this->m_unit->m_unitFieldLevel - 1;
    float baseRatio = GetGameTableValue(7, levelIndex, classIndex);
    float ratio = GetGameTableValue(9, levelIndex, classIndex);
    return ratio * (spirit - baseSpirit) + baseSpirit * baseRatio;
}

// VERIFY: da0b4434 auto
// OFFSET: 0x00715500
void CGUnit_C::UpdateUnitNameText() {
    if (this->m_nameDesc) {
        PlayerNameTriggerNameRegenerate(this->m_nameDesc);
    }

    if (this->m_namePlateFrame) {
        this->m_namePlateFrame->UpdateNameDisplay(this);
    }
}

// VERIFY: c9171312 manual
// OFFSET: 0x00728CA0
void NameQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    unit->DestroyNamePlate();
    CGGameUI::UnitNameUpdate(guid);

    TotemInfo* totem = CGGameUI::GetTotemInfo(guid);

    if (totem) {
        totem->m_name = unit->GetUnitName(nullptr, 1);
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_TOTEM_UPDATE, "%d", totem->m_slot + 1);
    }
}

// VERIFY: 0cbc8a3a manual
// OFFSET: 0x0072D4F0
int32_t CGUnit_C::UpdateObjectNameStringForUnit(CGUnit_C* unit, int32_t flags, char* buffer, uint32_t size, uint32_t a5) {
    if (a5 && (unit->m_auraEffectMask[279 / 8] & (1 << (279 % 8)))) {
        SpellRec spellRec;

        for (uint32_t i = 0; i < unit->GetAuraCount(); i++) {
            AuraData* aura = unit->GetAura(i);

            if (!g_spellDB.GetRecord(aura->m_spellId, &spellRec)) {
                continue;
            }

            for (int32_t effect = 0; effect < 3; effect++) {
                if (spellRec.m_effectAura[effect] != SPELL_AURA_INITIALIZE_IMAGES || !(aura->m_flags & (1 << effect))) {
                    continue;
                }

                CGUnit_C* creator = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(aura->m_creator, TYPE_UNIT, __FILE__, __LINE__));

                if (creator) {
                    return CGUnit_C::UpdateObjectNameStringForUnit(creator, flags, buffer, size, 0);
                }
            }
        }
    }

    char afkText[32] = {};
    char dndText[32] = {};
    char gmText[32] = {};
    char devText[32] = {};
    unit->GetAFKText(afkText, sizeof(afkText));
    unit->GetDNDText(dndText, sizeof(dndText));
    unit->GetGMText(gmText, sizeof(gmText));
    unit->GetDevText(devText, sizeof(devText));

    char titleText[1024] = {};
    char guildText[100] = {};
    char subNameText[1024] = {};
    char summonedText[1024] = {};
    char foreignText[8] = {};

    int32_t lines = 0;
    unit->GetUnitChosenTitle(titleText, sizeof(titleText), (flags & 8) != 0, &lines);

    if (unit->m_obj->m_type & TYPE_PLAYER) {
        if (flags & 4) {
            const char* guildName = static_cast<CGPlayer_C*>(unit)->GetPlayerGuildNameText();

            if (guildName && *guildName) {
                SStrPrintf(guildText, sizeof(guildText), "\n<%s>", guildName);
                lines++;
            }
        }

        const char* realm = nullptr;
        unit->GetUnitName(&realm, 1);

        if (realm && *realm && unit->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
            SStrPrintf(foreignText, sizeof(foreignText), FrameScript_GetText("FOREIGN_SERVER_LABEL", -1, GENDER_NOT_APPLICABLE));
        }
    } else {
        CreatureStats_C* stats = unit->m_creatureCacheEntry;

        if (stats && *stats->m_subName) {
            const char* subName = stats->m_subName;

            if (!unit->m_unit->m_unitFieldPetnumber && subName && *subName) {
                SStrPrintf(subNameText, sizeof(subNameText), "\n<%s>", subName);
                lines++;
            }
        }

        char summonedBy[128] = {};
        CGTooltip::GetSummonedByString(unit, summonedBy, sizeof(summonedBy));

        if (summonedBy[0]) {
            SStrPrintf(summonedText, sizeof(summonedText), "\n<%s>", summonedBy);
            lines++;
        }
    }

    SStrPrintf(buffer, size, "%s%s%s%s%s%s%s%s%s", afkText, dndText, gmText, devText, titleText, guildText, subNameText, summonedText, foreignText);

    return lines;
}

// STATIC
// VERIFY: f5c49047 auto
// OFFSET: 0x005EEB70
static void ObjectNameCacheCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
}

// VERIFY: c8c95be6 manual
// OFFSET: 0x0074D750
const char* GetObjectNameFromGuid(const WOWGUID& guid) {
    const char* name = nullptr;
    uint32_t high = static_cast<uint32_t>(guid.m_guid >> 32);

    if (!(high & 0xF0000000) && ((high & 0xF07FFFFF) | static_cast<uint32_t>(guid.m_guid))) {
        NameCache* record = g_nameDBCache.GetRecord(guid.m_guid, guid, ObjectNameCacheCallback, nullptr, true);

        if (record) {
            record->FullName(s_objectNameBuffers[s_objectNameIndex], sizeof(s_objectNameBuffers[0]));
            name = s_objectNameBuffers[s_objectNameIndex++];

            if (s_objectNameIndex >= 2) {
                s_objectNameIndex = 0;
            }
        }
    } else {
        uint32_t type = high & 0xF0F00000;

        if (type == 0xF0300000 || type == 0xF0500000) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

            if (unit) {
                name = unit->GetUnitName(nullptr, 1);
            } else {
                CreatureStats_C* stats = g_creatureDBCache.GetRecord(static_cast<int32_t>(guid.m_guid >> 24) & 0xFFFFFFF, guid, ObjectNameCacheCallback, nullptr, true);

                if (stats) {
                    name = stats->m_name[0];
                }
            }
        } else if (type == 0xF0400000) {
            PetNameCache* pet = g_petNameCache.GetRecord(static_cast<int32_t>(guid.m_guid >> 24) & 0xFFFFFFF, guid, ObjectNameCacheCallback, nullptr, true);

            if (pet) {
                name = pet->m_name;
            }
        } else if (type == 0xF0100000) {
            GameObjectStats_C* stats = g_gameObjectDBCache.GetRecord(static_cast<int32_t>(guid.m_guid >> 24) & 0xFFFFFFF, guid, ObjectNameCacheCallback, nullptr, true);

            if (stats) {
                name = stats->GetName(0);
            }
        }
    }

    if (!name || !*name) {
        name = FrameScript_GetText("UNKNOWNOBJECT", -1, GENDER_NOT_APPLICABLE);
    }

    if (!name || !*name) {
        return "UKNOWNOBJECT";
    }

    return name;
}

// VERIFY: a8083a71 auto
// OFFSET: 0x00715720
static bool GetNamePlateScreenPos(CGUnit_C* unit, CGWorldFrame* worldFrame, C3Vector* screen) {
    C3Vector position = unit->GetNamePosition();
    position.z = position.z + 0.6666667f;
    return worldFrame->GetScreenCoordinates(position, screen, nullptr);
}

// VERIFY: cdbf1d87 manual
// OFFSET: 0x00715760
static float NamePlateScreenCenterDistanceSquared(C3Vector position) {
    static C3Vector s_screenCenter(0.4f, 0.3f, 0.0f);

    C3Vector delta(position.x - s_screenCenter.x, position.y - s_screenCenter.y, position.z - s_screenCenter.z);
    C2Vector delta2(delta);

    return delta2.x * delta2.x + delta2.y * delta2.y;
}

// VERIFY: 7bc35de9 auto
// OFFSET: 0x00715C40
void CGUnit_C::ClearNamePlateFocus() {
    if (CGUnit_C::s_namePlateFocus) {
        CGUnit_C::s_namePlateFocus->OnLoseFocus();
    }

    CGUnit_C::s_namePlateFocus = nullptr;
}

// VERIFY: 4b93d3b3 auto
// OFFSET: 0x007198D0
void CGUnit_C::NamePlateUpdateRaidTarget() {
    if (this->m_namePlateFrame) {
        this->m_namePlateFrame->UpdateRaidTargetDisplay(GetRaidTargetIndexFromGuid(this->m_obj->m_guid));
    }
}

// VERIFY: e687536c auto
// OFFSET: 0x00719050
void CGUnit_C::RecreateNamePlate() {
    if (this->m_nameDesc) {
        PlayerNameDelete(this->m_nameDesc);
    }

    WOWGUID guid = this->m_obj->m_guid;
    this->m_nameDesc = PlayerNameCreate(guid);
}

// VERIFY: 0f4aff6f manual
// OFFSET: 0x007254E0
CGNamePlateFrame* CGNamePlateFrame::GetNewNameplateFrame(CSimpleFrame* parent) {
    CGNamePlateFrame* frame = s_unusedNamePlates.Head();

    if (!frame) {
        void* data = CGNamePlateFrame::s_namePlateHeap.GetData(0, typeid(CGNamePlateFrame).raw_name(), SLOG_OBJECT);
        return data ? new (data) CGNamePlateFrame(parent) : nullptr;
    }

    s_unusedNamePlates.UnlinkNode(frame);

    return frame;
}

// VERIFY: 2a5cc36c manual
// OFFSET: 0x00725530
static void NamePlateSort(CGNamePlateFrame* frame) {
    s_namePlates.UnlinkNode(frame);

    float distance = NamePlateScreenCenterDistanceSquared(frame->m_position);

    for (CGNamePlateFrame* node = s_namePlates.Head(); node; node = s_namePlates.Next(node)) {
        if (NamePlateScreenCenterDistanceSquared(node->m_position) <= distance) {
            s_namePlates.LinkNode(frame, 2, node);
            return;
        }
    }

    s_namePlates.LinkToTail(frame);
}

// VERIFY: 20cca2b5 manual
// OFFSET: 0x007256C0
void CGUnit_C::NamePlateAddOrUpdate(const C3Vector& position, CGWorldFrame* worldFrame) {
    if (!this->m_namePlateFrame) {
        this->m_namePlateFrame = CGNamePlateFrame::GetNewNameplateFrame(worldFrame);
        this->m_namePlateFrame->Initialize(this);
        this->m_namePlateFrame->Show();
        this->m_namePlateFrame->m_position = position;
        NamePlateSort(this->m_namePlateFrame);
        worldFrame->SetNamePlateUpdate();

        if (this->m_chatBubble) {
            CGChatBubbleFrame::Remove(this->m_chatBubble);
            this->m_chatBubble = nullptr;
        }

        this->UpdateSpellCastBars(FrameTime::s_curTimeMs);
        this->ObjectNameVisibilityChanged();
        return;
    }

    if (CGUnit_C::s_namePlateFlags & 0x2) {
        this->m_namePlateFrame->UpdateLevelDisplay(this);
    }

    if (CGUnit_C::s_namePlateFlags & 0x4) {
        this->m_namePlateFrame->UpdateColorDisplay(this);
    }

    C3Vector oldPosition = this->m_namePlateFrame->m_position;
    C3Vector delta(position.x - oldPosition.x, position.y - oldPosition.y, position.z - oldPosition.z);
    C2Vector delta2(delta);

    if ((CGUnit_C::s_namePlateFlags & 0x1) || delta2.y * delta2.y + delta2.x * delta2.x != 0.0f) {
        this->m_namePlateFrame->m_position = position;
        NamePlateSort(this->m_namePlateFrame);
        worldFrame->SetNamePlateUpdate();
    }
}

// VERIFY: b1479e18 auto
// OFFSET: 0x00725890
void CGUnit_C::UnitNameplatesReposition(CGWorldFrame* worldFrame) {
    CGNamePlateFrame* frame = s_namePlates.Head();

    while (frame) {
        CGNamePlateFrame* next = s_namePlates.Next(frame);

        if (s_cvNameplateAllowOverlap && s_cvNameplateAllowOverlap->m_intValue) {
            C3Vector position = frame->m_position;
            frame->SetPoint(FRAMEPOINT_TOP, worldFrame, FRAMEPOINT_BOTTOMLEFT, position.x, position.y, 1);
            frame->SetFrameDepth(position.z - worldFrame->m_layoutDepth, true);
        } else {
            C3Vector position = frame->m_position;
            SmartScreenRectPositionFrame(SRECTGRID_NAMEPLATES, frame, worldFrame, position, 0);
        }

        frame = next;
    }

    CLayoutFrame::ResizePending();
}

// VERIFY: fb0e4f71 manual
// OFFSET: 0x00726E10
void CGUnit_C::CancelOpenStable() {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer() || !this->HasAuraEffect(SPELL_AURA_OPEN_STABLE)) {
        return;
    }

    SpellRec spell;

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        if (!(this->GetAura(i)->m_flags & 0x10)) {
            continue;
        }

        if (!g_spellDB.GetRecord(this->GetAura(i)->m_spellId, &spell)) {
            continue;
        }

        for (uint32_t j = 0; j < 3; j++) {
            if (spell.m_effectAura[j] == SPELL_AURA_OPEN_STABLE) {
                Spell_C_CancelAura(&spell, this, -1);
            }
        }
    }
}

// VERIFY: aacf89f7 manual
// OFFSET: 0x00726F30
void CGUnit_C::CancelShapeshiftForMount() {
    if (this->IsShapeShifted()) {
        this->CancelShapeshift();
    }

    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer() || this->m_unit->m_unitFieldNativedisplayid == this->m_unit->m_unitFieldDisplayid) {
        return;
    }

    SpellRec spell;

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        AuraData& aura = this->m_auras[i];

        if (!(aura.m_flags & 0x10) || !g_spellDB.GetRecord(aura.m_spellId, &spell)) {
            continue;
        }

        for (uint32_t effect = 0; effect < 3; effect++) {
            if (spell.m_effectAura[effect] != SPELL_AURA_MOD_SHAPESHIFT) {
                continue;
            }

            SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(spell.m_effectMiscValue[effect]);

            if (!form) {
                continue;
            }

            for (uint32_t d = 0; d < 4; d++) {
                if (!form->m_creatureDisplayID[d]) {
                    continue;
                }

                CreatureDisplayInfoRec* display = g_creatureDisplayInfoDB.GetRecord(form->m_creatureDisplayID[d]);
                CreatureModelDataRec* model = display ? g_creatureModelDataDB.GetRecord(display->m_modelID) : nullptr;

                if (model && model->m_flags >= 0) {
                    Spell_C_CancelAura(&spell, this, -1);
                    break;
                }
            }
        }
    }
}

// VERIFY: 5f551aa3 manual
// OFFSET: 0x00727130
void CGUnit_C::NamePlateClearAll() {
    CGNamePlateFrame* frame = s_namePlates.Head();

    while (frame) {
        CGNamePlateFrame* next = s_namePlates.Next(frame);
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(frame->m_guid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit && unit->m_namePlateFrame) {
            if (CGUnit_C::s_namePlateFocus == unit->m_namePlateFrame) {
                CGUnit_C::s_namePlateFocus = nullptr;
            }

            s_unusedNamePlates.LinkToTail(unit->m_namePlateFrame);
            unit->m_namePlateFrame->Hide();
            unit->m_namePlateFrame = nullptr;
            unit->ObjectNameVisibilityChanged();
        }

        frame = next;
    }
}

// VERIFY: 80222452 manual
// OFFSET: 0x007271D0
void CGUnit_C::SetNamePlateFocus(const C2Vector& position) {
    if (CGInputControl::GetActive()->m_flags & 0x6000003) {
        return;
    }

    CGNamePlateFrame* focus = nullptr;

    for (CGNamePlateFrame* frame = s_namePlates.Head(); frame; frame = s_namePlates.Next(frame)) {
        if (frame->m_guid == CGGameUI::m_lockedTarget) {
            continue;
        }

        if (!frame->PtInFrameRect(position)) {
            continue;
        }

        if (!focus || focus->m_position.z > frame->m_position.z) {
            focus = frame;
        }
    }

    if (focus == CGUnit_C::s_namePlateFocus) {
        return;
    }

    if (CGUnit_C::s_namePlateFocus) {
        CGUnit_C::s_namePlateFocus->OnLoseFocus();
    }

    CGUnit_C::s_namePlateFocus = focus;

    if (focus) {
        focus->OnGainFocus();
    }
}

// VERIFY: 5739ef94 manual
// OFFSET: 0x0072B060
int32_t CGUnit_C::ShouldShowNamePlate(CGWorldFrame* worldFrame, C3Vector* position) {
    if (this->m_unit->m_unitFieldHealth <= 0) {
        return 0;
    }

    if (this->m_unit->m_unitFieldFlags & 0x2000000) {
        return 0;
    }

    if (!this->m_worldObject || !World::QueryObjectVisible(this->m_worldObject)) {
        return 0;
    }

    if (!CGGameUI::m_visible) {
        return 0;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (!camera) {
        return 0;
    }

    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(camera->m_targetGUID, TYPE_UNIT, __FILE__, __LINE__));

    if (!target || target == this) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    bool friendly;

    if (this->m_obj->m_type & TYPE_PLAYER) {
        friendly = player->CanCooperate(this);
    } else {
        friendly = !player->CanAttack(this);
    }

    if (!friendly && (this->m_unit->m_unitFieldBytes1[2] & 0x2)) {
        return 0;
    }

    if (this->m_creatureCacheEntry && (this->m_creatureCacheEntry->m_typeFlags & 0x100000)) {
        return 0;
    }

    if (!g_nameplateShowEnemiesCVar->m_intValue && !friendly) {
        return 0;
    }

    if (!g_nameplateShowFriendsCVar->m_intValue && friendly) {
        return 0;
    }

    if (this->GetCreatureType() == 8) {
        return 0;
    }

    if (this->GetCreatureType() == 12) {
        return 0;
    }

    if (this->m_unit->m_unitDynamicFlags & 0x20) {
        WOWGUID guid = this->m_obj->m_guid;

        if (!CGGameUI::IsRaidMemberOrPet(guid)) {
            return 0;
        }
    }

    CGUnit_C* owner = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*this->GetCharmerOrCreatorGuid(), TYPE_UNIT, __FILE__, __LINE__));

    if (owner && (owner->m_obj->m_type & TYPE_PLAYER)) {
        if (this->HasCreator() && !this->HasSummoner() && (this->m_unit->m_unitFieldFlags & 0x200)) {
            return 0;
        }

        if (this->GetCreatureType() == 11) {
            if (friendly ? !g_nameplateShowFriendlyTotemsCVar->m_intValue : !g_nameplateShowEnemyTotemsCVar->m_intValue) {
                return 0;
            }
        } else if (this->GetCreatorGuid()) {
            if (friendly ? !g_nameplateShowFriendlyGuardiansCVar->m_intValue : !g_nameplateShowEnemyGuardiansCVar->m_intValue) {
                return 0;
            }
        } else if (friendly ? !g_nameplateShowFriendlyPetsCVar->m_intValue : !g_nameplateShowEnemyPetsCVar->m_intValue) {
            return 0;
        }
    }

    if ((this->m_obj->m_type & TYPE_PLAYER) && !friendly && !player->CanAttack(this)) {
        return 0;
    }

    if (!World::QueryObjectVisible(this->m_worldObject) || !GetNamePlateScreenPos(this, worldFrame, position)) {
        return 0;
    }

    MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);

    if (!map || map->m_instanceType != 4) {
        C3Vector delta = this->GetPosition() - target->GetPosition();

        if (delta.y * delta.y + delta.x * delta.x + delta.z * delta.z > MAX_NAMEPLATE_DIST_SQUARED) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: d1c044ca manual
// OFFSET: 0x007155D0
void CGUnit_C::AddWorldHonorGainText(int32_t honor, int32_t pvpRank, const char* victimName) {
    char text[64];
    memset(text, 0, sizeof(text));
    const char* label;

    if (honor <= 0) {
        label = FrameScript_GetText("DK", -1, GENDER_NOT_APPLICABLE);

        if (!label || !*label) {
            label = "DK";
        }
    } else {
        label = FrameScript_GetText("HK", -1, GENDER_NOT_APPLICABLE);

        if (!label || !*label) {
            label = "HK";
        }
    }

    SStrPrintf(text, sizeof(text), "%s: %s", label, victimName);
    PlayerNameCreateText(this->m_nameDesc, WT_HONORGAIN, text, nullptr, &pvpRank);
}

// VERIFY: 5809ed27 auto
// OFFSET: 0x00722440
void CGUnit_C::AddWorldHealingText(const WOWGUID& source, int32_t amount, const SpellRec* spell, bool crit, int32_t unused_a6) {
    bool isPlayer;

    if (!amount || this->m_obj->m_guid == CGUnit_C::s_activeMover || !IsOwnedByActivePlayer(source, &isPlayer) || !g_combatHealingCVar->m_intValue) {
        return;
    }

    char text[32];
    SStrPrintf(text, sizeof(text), "+%d", amount);

    if (isPlayer) {
        PlayerNameCreateText(this->m_nameDesc, crit ? WT_CRIT_HEAL : WT_HEAL, text, nullptr, nullptr);
    }
}

// VERIFY: 5b54f94f auto
// OFFSET: 0x00714B60
float CGUnit_C::GetHeightAboveGround() {
    C3Vector start = this->GetPosition();
    C3Vector end = start;
    start.z += 1.0f;
    end.z -= 1.0f;
    C3Vector hit = end;
    float distance = 1.0f;
    World::Intersect(start, end, &hit, &distance, 0x100111, nullptr);

    return start.z - hit.z;
}

// VERIFY: c691843b auto
// OFFSET: 0x00721190
void CGUnit_C::RemoveChannelEffects(int32_t spellId) {
    for (uint32_t i = 0; i < (this->m_handItemSpellIds.m_count == 0xFFFFFFFF ? this->m_handItemSpellIds.m_dynamic.m_count : this->m_handItemSpellIds.m_count); i++) {
        if (this->m_handItemSpellIds[i] == static_cast<uint32_t>(spellId)) {
            return;
        }
    }

    CEffect* effect = this->m_effectList;

    while (effect) {
        CEffect* current = effect;
        effect = effect->m_next;

        if (current->m_spellId == static_cast<uint32_t>(spellId) && (current->m_flags & 0x3040)) {
            current->MoveToDestroyList();
        }
    }
}

// VERIFY: 7afdcd82 auto
// OFFSET: 0x00721210
void CGUnit_C::StartSwimming(uint32_t time, int32_t spline) {
    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        this->m_movementData.OnSwimStartLocal(time);
        CGTutorial::TriggerTutorial(27);
    } else if (spline || (this->m_movement->m_spline && !(this->m_movement->m_spline->m_flags & 0x400))) {
        this->m_movementData.OnSplineStartSwim();
    } else {
        this->m_movementData.OnSwimStart(time);
    }
}

// VERIFY: 87d9dc17 auto
// OFFSET: 0x00721290
void CGUnit_C::StopSwimming(uint32_t time, int32_t spline) {
    if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        this->m_movementData.OnSwimStopLocal(time);
    } else if (spline || (this->m_movement->m_spline && !(this->m_movement->m_spline->m_flags & 0x400))) {
        this->m_movementData.OnSplineStopSwim();
    } else {
        this->m_movementData.OnSwimStop(time);
    }
}

// VERIFY: 4865e105 manual
// OFFSET: 0x00730D10
void CGUnit_C::UpdateSwimmingStatus(uint32_t time, int32_t a3) {
    if (this->m_movement->m_flags2 & 0x4) {
        return;
    }

    int32_t liquidType;
    float height;
    int32_t liquidFlags;
    int32_t inLiquid = World::QueryObjectLiquid(this->m_worldObject, &liquidType, &height, &liquidFlags);
    float depth = 0.0f;

    if (inLiquid) {
        depth = height - this->GetPosition().z;
    }

    int32_t canSwim = this->CanSwim() && !this->GetTransportGUID();
    float swimDepth = this->m_movementData.m_collisionHeight * 0.75f;
    float exitDepth = swimDepth - 0.027777778f;
    uint32_t flags = this->m_movement->m_flags;

    if (flags & 0x200000) {
        if ((flags & 0x400000) && depth - swimDepth <= 0.33333334f && this->IsActiveMover()) {
            this->TryJumpOrAscend(time);
        }

        if (!(this->m_movementData.m_flags & 0x2000000) && (!inLiquid || exitDepth > depth || !canSwim)) {
            this->StopSwimming(time, a3);
        }
    } else {
        if (swimDepth < depth && canSwim && !this->m_movementData.IsJumpingUp()) {
            this->StartSwimming(time, a3);
        }

        if ((this->m_movementData.m_collisionHeight * 0.4f < depth) != (this->m_movementData.m_collisionHeight * 0.4f < this->m_swimDepth)) {
            C3Vector position = this->GetPosition();
            this->PlaySplashSound(&position);
            this->CreateRipple(201);
        }

        this->m_swimDepth = depth;
    }

    uint32_t wasSwimming = this->m_clientFlags & 0x200000;

    if ((this->m_movement->m_flags & 0x200000) || (swimDepth < depth && canSwim)) {
        this->m_clientFlags |= 0x200000;
    } else {
        this->m_clientFlags &= ~0x200000;
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && wasSwimming != (this->m_clientFlags & 0x200000)) {
        CGSpellBook::UpdateUsable();
    }
}

// VERIFY: c709791a manual
// OFFSET: 0x0073A890
void CGUnit_C::UpdateFlightStatus(uint32_t time) {
    uint32_t flags = this->m_movementData.m_flags;

    if (!(flags & 0x1000000)) {
        return;
    }

    if (!(flags & 0x2000000)) {
        if (!this->m_movementData.IsJumpingUp() && (this->m_movementData.m_flags & 0x1000) && this->IsActiveMover() && this->GetHeightAboveGround() >= 1.5f) {
            this->m_movementData.OnFlightLocal(time, 1);
            CGUnit_C::s_flightStatusTime = time;
        }

        return;
    }

    // STATIC: 0x00CA12B4
    static uint32_t s_nextFlightSoundTime = time + 2000;

    if (static_cast<int32_t>(time - s_nextFlightSoundTime) >= 0) {
        float bound;

        if (CMap::QueryFlightBounds(this->GetPosition(), &bound)) {
            float collisionHeight = this->m_movementData.m_collisionHeight;

            if (bound - 2.0f < this->GetPosition().z + collisionHeight) {
                if (!this->GetVehicleRec() || !this->m_vehicle->HasHighPriorityAnim()) {
                    this->PlayBaseAnimation(45, 0);
                    this->PlayWoundAnimKit(0);
                }

                CreatureSoundDataRec* sound = this->IsMounted() ? this->m_soundDataOverride : this->GetSoundData();

                if (sound) {
                    C3Vector position = this->GetPosition();

                    if (this->IsActivePlayer()) {
                        // STATIC: 0x00CA12B0
                        static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");

                        int32_t atCharacter = s_listenerAtCharacter->m_intValue;
                        SE2SoundKitProperties properties;
                        properties.ResetToDefaults();
                        properties.m_volumeScale = atCharacter ? 0.65f : 1.0f;
                        properties.m_priority = 110;
                        SI2::PlaySoundKit(sound->m_soundExertionID, atCharacter ? nullptr : &position, nullptr, &properties, 0, nullptr, 1, 0);
                    } else {
                        SI2::PlaySoundKit(sound->m_soundExertionID, &position, nullptr, nullptr, 0, nullptr, 1, 0);
                    }
                }

                s_nextFlightSoundTime = time + 2000;
            }
        }
    }

    if (time - CGUnit_C::s_flightStatusTime > 2000 && this->m_obj->m_guid == CGUnit_C::s_activeMover && this->GetHeightAboveGround() < 1.5f && (!(this->m_obj->m_type & TYPE_PLAYER) || !static_cast<CGPlayer_C*>(this)->IsCommentatorUberOrInArena())) {
        this->m_movementData.OnFlightLocal(time, 0);
    }
}

// VERIFY: 937a1e9e manual
// OFFSET: 0x00720170
void CGUnit_C::SetWeaponTrail(CM2Model* model, uint32_t hand) {
    if (hand > 1) {
        return;
    }

    if (this->m_weaponTrails[hand]) {
        WeaponTrailClose(this->m_weaponTrails[hand]);
    }

    this->m_weaponTrails[hand] = nullptr;
    const CGUnitVirtualItem* item = this->GetVirtualItem(static_cast<VIRTUAL_MONSTER_SLOT>(hand), false);

    if (!item) {
        return;
    }

    uint32_t link = this->m_unit->m_unitFieldBytes2[0] ? (hand != 0) + 1 : CCharacterComponent::GetSheatheLink(static_cast<SHEATHE_TYPE>(item->m_sheatheType), hand == 0);

    for (CM2Model* attached = model->m_attachList; attached; attached = attached->m_attachNext) {
        if (attached->m_attachmentId == link) {
            this->m_weaponTrails[hand] = WeaponTrailCreate(attached);
            return;
        }
    }
}

// VERIFY: 6cf62b86 auto
// OFFSET: 0x006CF350
float CGUnit_C::GetScaledModelScale(const CreatureDisplayInfoRec* displayInfo, const CreatureModelDataRec* modelData, float* modelScale) {
    float scale = this->GetScale();

    if (scale < 1.0f) {
        scale = 1.0f;
    }

    float model = this->GetModelScale(displayInfo, modelData);

    if (modelScale) {
        *modelScale = model;
    }

    return model * scale;
}

// VERIFY: 1dce3fba manual
// OFFSET: 0x00725F50
int32_t CGUnit_C::UpdateUnitCollisionBox(int32_t a2, int32_t a3) {
    if (this->m_obj->m_type & TYPE_PLAYER) {
        return static_cast<CGPlayer_C*>(this)->UpdateUnitCollisionBox(a2, a3);
    }

    CreatureDisplayInfoRec* displayInfo;

    if (!this->m_creatureCacheEntry || !(this->m_creatureCacheEntry->m_typeFlags & 0x20000) || !(displayInfo = g_creatureDisplayInfoDB.GetRecord(this->m_unit->m_unitFieldNativedisplayid))) {
        this->m_movementData.SetCollisionBox(0.6666667f, 2.0277777f, 1.0f, 1.0f, 0);
        return 1;
    }

    float width;
    float height;
    float scale;
    float modelScale;
    CreatureModelDataRec* modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);

    if (modelData) {
        float objectScale = this->GetScale();

        if (objectScale < 1.0f) {
            objectScale = 1.0f;
        }

        modelScale = this->GetModelScale(this->m_unit->m_unitFieldNativedisplayid);
        scale = modelScale * objectScale;
        height = modelData->m_collisionHeight;
        width = modelData->m_collisionWidth;
    } else {
        modelScale = 1.0f;
        scale = 1.0f;
        height = 2.0277777f;
        width = 0.6666667f;
    }

    this->m_movementData.SetCollisionBox(width, height, scale, modelScale, 0);
    return 1;
}

// VERIFY: 1b41a812 manual
// OFFSET: 0x0071FBF0
void CGUnit_C::UpdateMountTransitionModel() {
    if (!this->m_mountTransition || !this->m_mountTransitionEffect || !this->m_mountTransitionEffect->m_model) {
        return;
    }

    C3Vector position = { 0.0f, 0.0f, 0.0f };
    C3Vector up = { 0.0f, 0.0f, 0.0f };
    C44Matrix inverse;
    float facing;
    SpellVisualsGetMountTransitionTransform(this->m_mountTransition, &position, &up, &facing);

    C44Matrix world = this->m_worldModel->m_worldTransform;
    inverse = world.AffineInverse(this->GetTrueScale());

    this->m_mountTransitionEffect->m_model->SetWorldTransform(position, facing, this->GetTrueScale(), up, 0xFFFFFFFF);

    C44Matrix transform = this->m_mountTransitionEffect->m_model->m_worldTransform;
    CM2Model* model = this->m_mountTransitionEffect->m_model;
    model->SetWorldTransform(transform * inverse);

    if (model->m_attachParent) {
        model->m_fFlags |= 0x80;
    } else {
        model->m_fFlags |= 0x8;
    }

    if (model->m_attachParent) {
        model->m_fFlags |= 0x20000;
    } else {
        model->m_fFlags |= 0x10000;
    }

    float alpha = this->m_worldModel->m_fadeAlpha;
    this->m_mountTransitionEffect->m_model->m_fadeAlpha = SpellVisualsGetMountTransitionAlpha(this->m_mountTransition) * alpha;
}

// VERIFY: 05cf001e manual
// OFFSET: 0x00714A80
static int32_t IsArenaOpponent(const WOWGUID& guid) {
    for (int32_t i = 0; i < 5; i++) {
        if (CGBattlefieldInfo::m_arenaOpponents[i] == guid) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 52d446aa manual
// OFFSET: 0x00720960
void CGUnit_C::UpdateScriptRegistration() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t flags = 0;

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        flags = 0x1;
    }

    if (player) {
        const WOWGUID& pet = player->m_unit->m_unitFieldCharm.m_guid ? player->m_unit->m_unitFieldCharm : player->m_unit->m_unitFieldSummon;

        if (pet == this->m_obj->m_guid) {
            flags |= 0x2;
        }
    }

    if (!(flags & 0x1)) {
        WOWGUID guid = this->m_obj->m_guid;

        if (CGPartyInfo::IsMember(guid)) {
            flags |= 0x10;
        }
    }

    if (!(flags & 0x2)) {
        WOWGUID guid = this->m_obj->m_guid;

        if (CGPartyInfo::IsMemberPet(guid, nullptr)) {
            flags |= 0x20;
        }
    }

    WOWGUID guid = this->m_obj->m_guid;

    if (IsGuidInMyRaid(guid)) {
        flags |= 0x100;
    }

    guid = this->m_obj->m_guid;

    if (CGRaidInfo::IsPetInRaid(guid, nullptr)) {
        flags |= 0x200;
    }

    if (CGGameUI::m_lockedTarget == this->m_obj->m_guid) {
        flags |= 0x1000;
    }

    guid = this->m_obj->m_guid;

    if (IsArenaOpponent(guid)) {
        flags |= 0x4000;
    }

    if (flags == this->m_scriptRegistration) {
        return;
    }

    if (!this->m_scriptRegistration) {
        ScriptEventsRegisterUnit(this);
    } else if (!flags) {
        ScriptEventsUnregisterUnit(this);
    }

    this->m_scriptRegistration = flags;
}

// VERIFY: 04ade30d manual
// OFFSET: 0x00725DF0
void CGUnit_C::CreateOrDestroyObjectEffectManager() {
    int32_t displayId = this->m_mountDisplayId;

    if (!displayId) {
        displayId = this->m_displayId;

        if (!displayId || this->m_unit->m_unitFieldNativedisplayid != this->m_unit->m_unitFieldDisplayid) {
            displayId = this->m_unit->m_unitFieldDisplayid;
        }
    }

    CreatureDisplayInfoRec* displayInfo = g_creatureDisplayInfoDB.GetRecord(displayId);
    int32_t packageId = displayInfo ? displayInfo->m_objectEffectPackageID : 0;

    if (this->m_effectManager && this->m_effectManager->m_packageId != packageId) {
        this->m_animEffectStates[0] = 0;
        this->m_animEffectStates[1] = 0;
        this->m_animEffectStates[2] = 0;
        delete this->m_effectManager;
        this->m_effectManager = nullptr;
    }

    if (!packageId || this->m_effectManager) {
        return;
    }

    void* mem = SMemAlloc(sizeof(CObjectEffectManager), __FILE__, __LINE__, 0);
    this->m_effectManager = mem ? new (mem) CObjectEffectManager() : nullptr;

    if (this->m_effectManager->Init(displayInfo->m_objectEffectPackageID, this)) {
        this->m_animEffectStates[0] = 0;
        this->m_animEffectStates[1] = 0;
        this->m_animEffectStates[2] = 0;
        this->UpdateObjectEffectMovementStates();
    } else {
        delete this->m_effectManager;
        this->m_effectManager = nullptr;
    }
}

// VERIFY: 21e5fa5b auto
// OFFSET: 0x0072A950
void CGUnit_C::UpdatePartyMemberPetState() {
    WOWGUID guid = this->m_obj->m_guid;
    CGPartyMemberState* partyState = nullptr;
    CGPartyMemberState* raidState = nullptr;
    int32_t index;

    if (CGPartyInfo::IsMemberPet(guid, &index) && index >= 0) {
        partyState = CGPartyInfo::m_members[index] ? &s_partyMemberStates[index] : nullptr;
    }

    guid = this->m_obj->m_guid;

    if (CGRaidInfo::IsPetInRaid(guid, &index) && index >= 0) {
        raidState = CGRaidInfo::m_members[index]->m_guid ? &CGRaidInfo::m_members[index]->m_stats : nullptr;
    }

    CGPartyMemberState oldState;
    CGPartyMemberState* source;

    if (partyState) {
        oldState = *partyState;
        source = partyState;
    } else {
        if (!raidState) {
            return;
        }

        oldState = *raidState;
        source = &oldState;
    }

    CGPartyMemberState newState = *source;
    this->FillPartyMemberPetState(&newState.m_pet);
    uint32_t changes = CGPlayer_C::BuildPartyMemberStatsChanges(&oldState, &newState);
    CGPlayer_C::DispatchPartyMemberStatsChanges(newState.m_guid, changes, &oldState, &newState);
}

// VERIFY: 81db5902 manual
// OFFSET: 0x007265C0
void CGUnit_C::CreateProceduralEffect(const SpellRec* spell, const SpellVisualRec* visual, const SpellVisualKitRec* kit, int32_t expireTime, uint32_t flags, WOWGUID guid) {
    uint32_t spellId = spell ? spell->m_id : 0;

    for (uint32_t i = 0; i < 4; i++) {
        float param0 = kit->m_charParamZero[i];
        float param1 = kit->m_charParamOne[i];
        float param2 = kit->m_charParamTwo[i];
        float param3 = kit->m_charParamThree[i];

        switch (kit->m_charProc[i]) {
            case 0:
            case 12: {
                void* mem = SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0);
                CEffect* effect = mem ? new (mem) CEffect() : nullptr;
                WOWGUID self = this->m_obj->m_guid;
                effect->AddLightning(expireTime, spellId, kit, &self, flags & 0x300401);
                effect->Release();

                WOWGUID source = WOWGUID();
                CGUnit_C* target = this;
                const WOWGUID* targets;
                uint32_t count;

                if ((flags & 0x1000) && guid && guid != this->m_obj->m_guid) {
                    target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

                    if (!target) {
                        break;
                    }

                    source = this->m_obj->m_guid;
                    targets = &source;
                    count = 1;
                } else if (this->m_clientFlags & 0x800000) {
                    targets = nullptr;
                    count = 0;
                } else if (spellId == this->m_unit->m_unitChannelSpell && this->m_unit->m_unitFieldChannelObject && this->m_chainSpellTargets.Count() <= 1) {
                    targets = &this->m_unit->m_unitFieldChannelObject;
                    count = 1;
                } else {
                    if (!this->m_chainSpellTargets.Count()) {
                        this->m_chainSpellTargets.SetCount(0);
                        break;
                    }

                    targets = this->m_chainSpellTargets.m_data;
                    count = this->m_chainSpellTargets.Count();
                }

                const C3Vector* location = (this->m_clientFlags & 0x800000) ? &this->m_chainSpellLocation : nullptr;
                SpellVisualsCreateChainLightning(target, effect, static_cast<int32_t>(param0), param2 != 0.0f, param3 != 0.0f, spellId, targets, count, effect->m_lightning, 12, location, this->m_lightningSequence);
                this->m_chainSpellTargets.SetCount(0);

                if ((this->m_clientFlags & 0x800000) && spell && visual && visual->m_impactAreaKit != kit->m_id) {
                    SpellVisualKitRec* areaKit = g_spellVisualKitDB.GetRecord(visual->m_impactAreaKit);
                    C3Vector position = this->m_chainSpellLocation;

                    if (areaKit) {
                        PlaySpellVisuaKitData data(spell, areaKit, 3);
                        data.m_position = &position;
                        data.m_playAnim = 0;
                        data.unused_0024 = this->m_lightningSequence;
                        this->PlaySpellVisualKit(data);
                    }
                }

                this->m_lightningSequence += 2;
                break;
            }

            case 1: {
                if (this->m_creatureCacheEntry && (this->m_creatureCacheEntry->m_typeFlags & 0x40) && SpellRec::IsHelpfulOrHarmful(spell) == 2) {
                    break;
                }

                this->ReleaseEffectsBySpell(spellId);
                void* mem = SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0);
                CEffect* effect = mem ? new (mem) CEffect() : nullptr;
                effect->m_spellId = spellId;
                *reinterpret_cast<uint32_t*>(&effect->m_color) = static_cast<int32_t>(param0) | 0xFF000000;
                effect->LinkTo(&this->m_procEffects);
                break;
            }

            case 6: {
                CImVector color;
                *reinterpret_cast<uint32_t*>(&color) = static_cast<uint32_t>(static_cast<int64_t>(param0)) | 0xFF000000;
                SpellVisualsHandleEclipse(&color, param1, spell);
                break;
            }

            case 8: {
                this->m_pendingWeaponTrail[0] = (static_cast<int32_t>(param3) << 24) | static_cast<int32_t>(param0);
                this->m_pendingWeaponTrail[1] = static_cast<int32_t>(param2);
                break;
            }

            case 11: {
                void* mem = SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0);
                CEffect* effect = mem ? new (mem) CEffect() : nullptr;
                WOWGUID self = this->m_obj->m_guid;
                effect->CreateProceduralEffectImpl(expireTime, param0, spellId, kit, &self, flags & 0x300401);
                effect->Release();
                break;
            }

            case 13: {
                if (this->m_creatureCacheEntry && (this->m_creatureCacheEntry->m_typeFlags & 0x40) && SpellRec::IsHelpfulOrHarmful(spell) == 2) {
                    break;
                }

                uint32_t color = static_cast<int32_t>(param0) | 0xFF000000;
                int32_t duration = FloatToInt64(param2 * 1000.0f);
                int32_t delay = FloatToInt64(param1 * 1000.0f);
                this->m_flashStartTime = FrameTime::s_curTimeMs;
                *reinterpret_cast<uint32_t*>(&this->m_flashColor) = color;
                this->m_flashDelay = delay;
                this->m_flashDuration = duration;
                break;
            }

            case 14: {
                this->ReleaseAurasBySpell(spellId);

                if (param0 >= 0.0f && param0 <= 1.0f) {
                    void* mem = SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0);
                    CEffect* effect = mem ? new (mem) CEffect() : nullptr;
                    effect->m_spellId = spellId;
                    effect->m_alpha = param0;
                    effect->m_kitRec = kit;

                    if (kit->m_flags & 0x200) {
                        effect->m_flags |= 0x2000000;
                    }

                    this->m_forceWorldModelAlpha = (kit->m_flags >> 10) & 1;
                    effect->m_fadeTime = FloatToInt64(param2 * 1000.0f);
                    effect->LinkTo(&this->m_auraEffects);
                }

                this->ApplyModelFade();
                break;
            }

            case 15: {
                uint8_t alpha = static_cast<uint8_t>(FloatToInt64(this->GetBaseAlpha() * param0));
                int32_t endAlpha = FloatToInt64(param2);
                int32_t hold = FloatToInt64(param3);
                int32_t type = FloatToInt64(param1);
                this->StartProceduralFade(alpha, type, 500, hold, endAlpha);
                break;
            }

            case 16: {
                if (this->IsMounted() || this->m_mountModel) {
                    break;
                }

                void* mem = SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0);
                CEffect* effect = mem ? new (mem) CEffect() : nullptr;
                WOWGUID self = this->m_obj->m_guid;
                effect->AddMountTransition(spellId, kit, &self);
                effect->m_flags |= flags & 0xC0;
                effect->Release();
                break;
            }

            case 17: {
                void* mem = SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0);
                CEffect* effect = mem ? new (mem) CEffect() : nullptr;
                WOWGUID self = this->m_obj->m_guid;
                effect->InitProceduralEffect(expireTime, static_cast<int32_t>(param1), spellId, kit, &self, flags);
                effect->Release();
                break;
            }

            default: {
                break;
            }
        }
    }
}

// VERIFY: 368c1cc8 auto
// OFFSET: 0x005EEB70
static void CritterDisplayCacheCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
}

// VERIFY: 5b91be20 manual
// OFFSET: 0x0072CC30
int32_t CGUnit_C::IsHostileCritter() {
    if (this->GetCreatureType() != 7) {
        return 0;
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    if (!(GetVisibleAuraEffectFlags() & 0x1)) {
        return 1;
    }

    if (this->m_obj->m_type & TYPE_PLAYER) {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (!player || player->IsHatedTowards(this)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: f78dfe00 manual
// OFFSET: 0x0072E270
void CGUnit_C::UpdateCritterDisplay() {
    if (!this->IsHostileCritter()) {
        if (this->m_displayId) {
            this->m_critterDisplayCreatureId = 0;
            this->m_displayId = 0;
            this->UpdateDisplayInfo(0);
        }

        return;
    }

    if (!GetVisibleAuraEffectDisplay() || GetVisibleAuraEffectDisplay() == this->m_critterDisplayCreatureId) {
        ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__);

        if (!GetVisibleAuraEffectDisplay() && this->m_critterDisplayCreatureId) {
            this->m_critterDisplayCreatureId = 0;
            this->m_displayId = 0;
            this->UpdateDisplayInfo(0);
        }

        return;
    }

    WOWGUID guid = this->m_obj->m_guid;
    CreatureStats_C* stats = g_creatureDBCache.GetRecord(GetVisibleAuraEffectDisplay(), guid, CritterDisplayCacheCallback, nullptr, true);

    if (!stats) {
        return;
    }

    uint32_t count = (stats->m_displayId[3] != 0) + (stats->m_displayId[2] != 0) + (stats->m_displayId[1] != 0) + (stats->m_displayId[0] != 0);

    if (!count) {
        return;
    }

    uint32_t index = static_cast<uint32_t>(rand()) % count;
    this->m_critterDisplayCreatureId = GetVisibleAuraEffectDisplay();
    this->m_displayId = stats->m_displayId[index];
    this->UpdateDisplayInfo(0);
}

// VERIFY: b7b739bd manual
// OFFSET: 0x0071D450
static int32_t IsWeaponAnimBehavior(uint32_t animId) {
    AnimationDataRec* animation = g_animationDataDB.GetRecord(animId);
    int32_t behavior = animation ? animation->m_behaviorID : 506;

    switch (behavior) {
        case 16:
        case 20:
        case 25:
        case 117:
        case 118:
            return 1;

        default:
            return 0;
    }
}

// VERIFY: 8ff14a38 auto
// OFFSET: 0x00721ED0
bool CGUnit_C::IsPlayingWeaponAnim() const {
    uint32_t sequence;

    if (this->m_worldModel && this->m_worldModel->IsLoaded(0, 0)) {
        if (this->m_torsoKeyBone == -1 || (sequence = this->m_worldModel->GetBoneSequenceId(this->m_torsoKeyBone)) == 0xFFFFFFFF) {
            sequence = this->m_worldModel->GetBoneSequenceId(0xFFFFFFFF);
        }
    } else {
        sequence = 0xFFFFFFFF;
    }

    if (IsWeaponAnimBehavior(sequence)) {
        return true;
    }

    return IsMeleeAttackAnim(sequence) && !this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, false);
}

// VERIFY: 0401280d manual
// OFFSET: 0x0071EE70
void CGUnit_C::DropTargetOnDeath() {
    if (this->m_obj->m_guid != CGGameUI::m_lockedTarget) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_combat.m_attacking) {
        player->SetCombatMode();
    }

    Spell_C_CancelAutoRepeat(true);

    if (this->m_unit->m_unitDynamicFlags & 0x20) {
        WOWGUID guid = this->m_obj->m_guid;

        if (CGGameUI::IsRaidMemberOrPet(guid)) {
            return;
        }
    }

    CGGameUI::Target(WOWGUID());
}

// VERIFY: c8451e32 manual
// OFFSET: 0x0071B1E0
void CGUnit_C::HandlePendingTrackEvents() {
    if (s_pendingAttackTarget) {
        if (ClntObjMgrObjectPtr(s_pendingAttackTarget, TYPE_UNIT, __FILE__, __LINE__) && this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            static_cast<CGPlayer_C*>(this)->CombatModeEnter(s_pendingAttackTarget, 0, 0);
        }

        s_pendingAttackTarget = WOWGUID();
    } else if (s_pendingLootTarget) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(s_pendingLootTarget, TYPE_UNIT, __FILE__, __LINE__));

        if (unit && this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            static_cast<CGPlayer_C*>(this)->LootUnit(unit, false);
        }

        s_pendingLootTarget = WOWGUID();
    } else if (s_pendingUseObject) {
        CGGameObject_C* gameObject = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(s_pendingUseObject, TYPE_GAMEOBJECT, __FILE__, __LINE__));

        if (gameObject) {
            gameObject->UseObject();
        }

        s_pendingUseObject = WOWGUID();
    } else if (s_pendingSkinTarget) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(s_pendingSkinTarget, TYPE_OBJECT, __FILE__, __LINE__));

        if (unit && this->IsActivePlayer()) {
            static_cast<CGPlayer_C*>(this)->HandleSkinning(unit, false, false);
        }

        s_pendingSkinTarget = WOWGUID();
    } else if (s_pendingInteractTarget) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(s_pendingInteractTarget, TYPE_UNIT, __FILE__, __LINE__));

        if (unit && this->IsActivePlayer()) {
            static_cast<CGPlayer_C*>(this)->HandleOnRightClick(unit, false);
        }

        s_pendingInteractTarget = WOWGUID();
    }
}

// VERIFY: 0f2f9494 manual
// OFFSET: 0x0073D940
void CGUnit_C::DestroyUnitMount(int32_t a2) {
    if (!this->m_mountModel) {
        return;
    }

    CM2Model* parent = this->m_worldModel->m_attachParent;

    if (parent) {
        CM2Model* grandParent = parent->m_attachParent;
        uint32_t attachmentId = parent->m_attachmentId;
        this->m_worldModel->m_fFlags ^= (this->m_worldModel->m_fFlags ^ parent->m_fFlags) & 0x80000;
        this->m_worldModel->DetachFromParent();

        if (grandParent) {
            this->m_worldModel->AttachToParent(grandParent, attachmentId, nullptr, 0);
        }
    }

    CM2Model* mount = this->m_mountModel;
    this->m_worldModel->m_diffuseTintR = mount->m_diffuseTintR;
    this->m_worldModel->m_diffuseTintG = mount->m_diffuseTintG;
    this->m_worldModel->m_diffuseTintB = mount->m_diffuseTintB;
    this->SetMountModel(nullptr);
    this->SetKeyBoneSequence(this->m_worldModel, 0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 0, 1, 0);
    this->m_animationState &= 0xFF77DFFB;
    this->UpdateBaseAnimation(0, -1);
    this->UpdateSelectionRadius();
    this->UpdateEffectAttachments();
    this->m_soundDataOverride = nullptr;

    if (this->m_obj->m_guid == CGWorldFrame::GetActiveCamera()->m_targetGUID) {
        CGGameUI::ResetCamera(this->m_obj->m_guid);
    }

    this->PlayDismountSound();

    if (a2) {
        this->UpdateUnitCollisionBox(1, 0);
    }

    if (this->GetObjectModel()->IsLoaded(0, 0)) {
        this->InitWheels();
    } else {
        this->m_wheelMask = 0;
    }

    this->UpdateObjectEffectAnimationStates();
    this->m_clientFlags &= ~0x10000000;
}

// VERIFY: 842ee456 manual
// OFFSET: 0x0073D5D0
void CGUnit_C::CreateUnitMount(int32_t a2, int32_t a3) {
    this->m_wheelMask = 0;

    if (!this->m_mountModel) {
        CreatureDisplayInfoRec* displayInfo = g_creatureDisplayInfoDB.GetRecord(this->m_mountDisplayId);

        if (!displayInfo) {
            return;
        }

        CreatureModelDataRec* modelData = g_creatureModelDataDB.GetRecord(displayInfo->m_modelID);

        if (!modelData) {
            return;
        }

        this->m_mountScale = displayInfo->m_creatureModelScale;
        CM2Model* mount = World::s_m2Scene->CreateModel(modelData->m_modelName, 0);
        mount->SetSequenceCallback(reinterpret_cast<void*>(&CGUnit_C::MountAnimEndCallback), this->m_obj->m_guid.m_guid);
        mount->SetEventCallback(reinterpret_cast<void*>(&CGUnit_C::AnimEventCallback), this->m_obj->m_guid.m_guid);
        C44Matrix transform = this->m_worldModel->m_worldTransform;
        mount->m_fFlags |= 0x8000;
        mount->m_worldTransform = transform;
        CCharacterComponent::ApplyMonsterGeosets(mount, displayInfo);
        CCharacterComponent::ReplaceMonsterSkin(mount, displayInfo, modelData);
        this->SetMountModel(mount);
        mount->Release();
        this->m_mountFootprintTextureId = modelData->m_footprintTextureID;
        this->m_mountFootprintLength = modelData->m_footprintTextureWidth * 0.027777778f;
        this->m_mountFootprintWidth = 0.027777778f * modelData->m_footprintTextureLength;
        this->m_soundDataOverride = this->GetMountSoundDataRec();
        this->m_mountModel->m_fFlags ^= (this->m_mountModel->m_fFlags ^ this->m_worldModel->m_fFlags) & 0x80000;
        this->m_worldModel->m_fFlags &= ~0x80000;
        CM2Model* parent = this->m_worldModel->m_attachParent;
        uint32_t attachmentId = this->m_worldModel->m_attachmentId;

        if (parent) {
            this->m_worldModel->DetachFromParent();
        }

        this->m_worldModel->AttachToParent(this->m_mountModel, 0, nullptr, 0);

        if (parent) {
            this->m_mountModel->AttachToParent(parent, attachmentId, nullptr, 0);
        }

        this->SetKeyBoneSequence(this->m_worldModel, 0xFFFFFFFF, this->m_mountAnimBehaviorId, 0xFFFFFFFF, 0, 1.0f, 1, 1, 0);
        C44Matrix scale(1.0f / this->m_mountScale);
        this->m_worldModel->m_fFlags |= 0x8000;
        this->m_worldModel->m_worldTransform = scale;
        mount->m_diffuseTintR = this->m_worldModel->m_diffuseTintR;
        mount->m_diffuseTintG = this->m_worldModel->m_diffuseTintG;
        mount->m_diffuseTintB = this->m_worldModel->m_diffuseTintB;
        this->m_worldModel->m_fadeAlpha = 1.0f;
        this->m_worldModel->m_diffuseTintR = 1.0f;
        this->m_worldModel->m_diffuseTintG = 1.0f;
        this->m_worldModel->m_diffuseTintB = 1.0f;
        this->m_worldModel->m_emissiveTintR = 0.0f;
        this->m_worldModel->m_emissiveTintG = 0.0f;
        this->m_worldModel->m_emissiveTintB = 0.0f;

        if ((this->m_obj->m_type & TYPE_PLAYER) && a3 && !static_cast<CGPlayer_C*>(this)->UpdateMountedCollisionBox(modelData->m_mountHeight, a2)) {
            this->Dismount();
        }
    }

    CEffect* effect = this->m_mountTransitionEffect;

    if (effect && !(effect->m_flags & 0x100)) {
        effect->MoveToDestroyList();
    }

    this->m_animationState |= 0x10;
    this->UpdateBaseAnimation(0, -1);

    if (this->m_obj->m_guid == CGWorldFrame::GetActiveCamera()->m_targetGUID) {
        CGGameUI::ResetCamera(this->m_obj->m_guid);
    }

    this->UpdateObjectEffectAnimationStates();
}

// VERIFY: e8fc52d0 manual
// OFFSET: 0x0073C140
void CGUnit_C::MountAnimEndCallback(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ param }, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    if (unit->m_mountModel && unit->m_vehicle && unit->m_vehicle->m_vehicleRec && unit->m_vehicle->IsBoneAnimPlaying(boneId)) {
        unit->m_vehicle->BoneAnimEndHandler(model, boneId, sequenceId, interrupted, time);
    } else {
        unit->MountAnimEndHandler(model, boneId, sequenceId, interrupted, time);
    }
}

// VERIFY: 3001c8a3 auto
// OFFSET: 0x0073BFF0
void CGUnit_C::MountAnimEndHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time) {
    if (this->m_awaitingEndAnimId == static_cast<int32_t>(sequenceId)) {
        this->m_awaitingEndAnimId = -1;
    }

    if (sequenceId == 192 || sequenceId == 200) {
        this->m_animationState &= ~0x40000;
    }

    if (sequenceId == 121) {
        this->m_animationState &= ~0x80000;
    } else if (sequenceId == 37) {
        this->m_animationState &= ~0x800000;
    }

    if (interrupted) {
        if (sequenceId == 39 || sequenceId == 187) {
            this->m_animationState &= ~0x4;
        }

        return;
    }

    this->m_animationState = (this->m_animationState & ~0x60) | 0x10;
    this->OnBaseAnimEnd(model, boneId, sequenceId);
    this->m_animationState |= 0x70;
}

// VERIFY: 381b5889 manual
// OFFSET: 0x00724EA0
void CGUnit_C::PostShutdown() {
    CMissile::Destroy();
    CGNamePlateFrame* plate = s_unusedNamePlates.Head();

    while (!s_unusedNamePlates.IsEnd(plate)) {
        CGNamePlateFrame* next = s_unusedNamePlates.RawNext(plate);
        s_unusedNamePlates.UnlinkNode(plate);
        delete plate;
        plate = next;
    }

    CGUnit_C::s_lastNPCSoundGUID = WOWGUID();
    CVehicle_C::UninitSystem();
    CVehiclePassenger_C::UninitSystem();
    CVehicleCamera_C::UninitSystem();
    CGUnit_C::ShutdownMissileTrajectorySystem();
}

// VERIFY: dc577adb manual
// OFFSET: 0x00723A50
void CGUnit_C::FootstepAnimEventHit(const C3Vector* position, int32_t left) {
    if (this->m_unit->m_unitFieldPetnumber || (this->m_movement->m_flags & 0x40000000) || (this->m_unit->m_unitFieldBytes1[2] & 0x2) || this->GetTransportGUID()) {
        return;
    }

    if ((this->m_obj->m_type & TYPE_PLAYER) && (static_cast<CGPlayer_C*>(this)->m_player->m_playerFlags & 0x10)) {
        return;
    }

    const C3Vector& cameraPos = CGWorldFrame::GetActiveCamera()->m_position;
    float dx = position->x - cameraPos.x;
    float dy = position->y - cameraPos.y;
    float dz = position->z - cameraPos.z;
    float distanceSquared = dx * dx + (dy * dy + dz * dz);

    if (distanceSquared > 2500.0f) {
        return;
    }

    if ((CWorld::enables & 0x400) && !(this->m_modelData->m_flags & 0x20)) {
        float size[2];
        size[0] = 0.0f;
        size[1] = 0.0f;
        int32_t textureId;
        this->GetFootprintInfo(&textureId, size);
        WOWGUID guid = this->m_obj->m_guid;
        int32_t activePlayer = guid == ClntObjMgrGetActivePlayer() ? 1 : 0;
        World::NewFootprint(textureId, size, position, this->GetFacing(), left == 0, this->m_terrainTypeId, activePlayer);
    }

    if (this->m_modelData->m_footstepShakeSize) {
        CGWorldFrame::GetActiveCamera()->AddShakeById(this->m_modelData->m_footstepShakeSize, position);
    }

    if (distanceSquared > 625.0f || (this->m_movement->m_flags & 0x2) || (this->m_modelData->m_flags & 0x1) || !CGUnit_C::s_cvShowFootPrintParticles->m_intValue) {
        return;
    }

    int32_t liquidType;
    float liquidHeight;
    int32_t liquidFlags;

    if (World::QueryObjectLiquid(this->m_worldObject, &liquidType, &liquidHeight, &liquidFlags)) {
        int32_t flag8;
        int32_t flag9;
        World::QueryObjectLiquidFlags(this->m_worldObject, &flag8, &flag9);

        if (!flag9) {
            return;
        }

        float depth = liquidHeight - this->GetPosition().z;

        if (this->m_movementData.m_collisionHeight * 0.5f <= depth) {
            return;
        }

        C3Vector splash;
        splash.x = position->x;
        splash.y = position->y;
        splash.z = depth + position->z;
        CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
        WOWGUID owner = this->m_obj->m_guid;
        // The boolean selects water run (0) or water walk (1).
        effect->AddSpecialByPosition(!this->IsRunning(), splash, &owner, 0x220, CGObject_C::EffectOnceHandler);
        effect->Release();
        return;
    }

    TerrainTypeRec* terrain = g_terrainTypeDB.GetRecord(this->m_terrainTypeId);

    if (!terrain) {
        return;
    }

    SpellVisualEffectNameRec* effectName = g_spellVisualEffectNameDB.GetRecord(this->IsRunning() ? terrain->m_footstepSprayWalk : terrain->m_footstepSprayRun);

    if (!effectName) {
        return;
    }

    C3Vector effectPosition = *position;

    if (this->GetTransportGUID()) {
        C44Matrix transport;
        MovementGetTransportMtxX(WOWGUID{ this->GetTransportGUID() }, &transport, this->m_obj->m_guid, __FILE__, __LINE__);
        effectPosition *= transport.AffineInverse();
    }

    CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
    WOWGUID owner = this->m_obj->m_guid;
    effect->InitFromUnit(effectPosition, OsGetAsyncTimeMs(), 0, nullptr, effectName, &owner, 0x220, CGObject_C::EffectOnceHandler, 0, nullptr, WOWGUID{ this->GetTransportGUID() });
    effect->Release();
}

// VERIFY: 9e04967c manual
// OFFSET: 0x0071DCC0
int32_t IsSpecialIdleAnim(uint32_t animId) {
    if (IsSpellCastAnim(animId)) {
        return 1;
    }

    AnimationDataRec* rec = g_animationDataDB.GetRecord(animId);

    if (rec && rec->m_behaviorID == 107) {
        return 1;
    }

    rec = g_animationDataDB.GetRecord(animId);

    if (rec && rec->m_behaviorID == 46) {
        return 1;
    }

    return GetAnimBehaviorId(animId) == 49;
}

// VERIFY: 597f8f44 manual
// OFFSET: 0x0072CDE0
void CreatureQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    CreatureStats_C* stats = g_creatureDBCache.GetRecord(id, WOWGUID(), nullptr, nullptr, false);

    if (!stats) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->DestroyNamePlate();
        unit->m_creatureCacheEntry = stats;

        if (stats->m_family) {
            unit->UpdateModelScale(nullptr);
        }

        unit->UpdateUnitCollisionBox(1, 0);
    }

    CGGameUI::UnitNameUpdate(guid);
    Script_SendUnitSignal(guid, SCRIPT_EVENT_UNIT_CLASSIFICATION_CHANGED);
    TotemInfo* totem = CGGameUI::GetTotemInfo(guid);

    if (totem) {
        totem->m_name = unit->GetUnitName(nullptr, 1);
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_TOTEM_UPDATE, "%d", totem->m_slot + 1);
    }
}

// VERIFY: 8ffb4576 manual
// OFFSET: 0x00720BF0
int32_t CGUnit_C::OverrideMissileFirePos(C3Vector* position, const SpellRec* spell) {
    if (!this->m_worldModel || !this->m_worldModel->IsLoaded(0, 0)) {
        return 0;
    }

    const SpellVisualRec* visual = GetSpellVisual(spell);

    if (!visual) {
        return 0;
    }

    C3Vector offset;

    if (visual->m_missileAttachment == -1) {
        if (0.0f == visual->m_missileCastOffset[0] && 0.0f == visual->m_missileCastOffset[1] && 0.0f == visual->m_missileCastOffset[2]) {
            return 0;
        }

        CM2Model* model = this->m_worldModel;

        if (model && model->IsLoaded(0, 0) && model->HasAttachment(s_effectAttachmentIds[12])) {
            offset.x = visual->m_missileCastOffset[0];
            offset.y = -visual->m_missileCastOffset[1];
            offset.z = visual->m_missileCastOffset[2];
            *position = this->GetAttachmentPosition(s_effectAttachmentIds[12], &offset);
            return 1;
        }

        C44Matrix matrix = this->GetModelMatrix();
        matrix.d0 = 0.0f;
        matrix.d1 = 0.0f;
        matrix.d2 = 0.0f;
        offset.x = visual->m_missileCastOffset[0];
        offset.y = -visual->m_missileCastOffset[1];
        offset.z = visual->m_missileCastOffset[2];
        C3Vector rotated = offset * matrix;
        *position = this->GetModelPosition() + rotated;
        return 1;
    }

    int32_t raw = visual->m_flags & 0x200;

    if (!this->HasAttachmentForEffect(visual->m_missileAttachment, raw)) {
        return 0;
    }

    offset.x = visual->m_missileCastOffset[0];
    offset.y = -visual->m_missileCastOffset[1];
    offset.z = visual->m_missileCastOffset[2];
    *position = this->GetAttachmentPositionForEffect(visual->m_missileAttachment, offset, raw);
    return 1;
}

// VERIFY: 228a87fd manual
// OFFSET: 0x00732100
CM2Model* CGUnit_C::CreateItemModel(uint32_t displayId, uint32_t inventoryType, float* scale) {
    if (scale) {
        *scale = 1.0f;
    }

    const char* path;

    switch (inventoryType) {
        case 1:
            path = "Item\\ObjectComponents\\Head\\";
            break;
        case 3:
            path = "Item\\ObjectComponents\\Shoulder\\";
            break;
        case 14:
            path = "Item\\ObjectComponents\\Shield\\";
            break;
        case 24:
            path = "Item\\ObjectComponents\\Ammo\\";
            break;
        case 27:
            path = "Item\\ObjectComponents\\Quiver\\";
            break;
        default:
            path = "Item\\ObjectComponents\\Weapon\\";
            break;
    }

    ItemDisplayInfoRec rec;

    if (!g_itemDisplayInfoDB.GetRecord(displayId, &rec)) {
        return nullptr;
    }

    uint32_t index;

    if (*rec.m_modelName[0]) {
        index = 0;
    } else if (*rec.m_modelName[1]) {
        index = 1;
    } else {
        return nullptr;
    }

    char fileName[260];

    if (inventoryType == 1) {
        int32_t sex = this->m_displayInfoExtra ? this->m_displayInfoExtra->m_displaySexID : this->m_unit->m_unitFieldBytes0[2];
        ChrRacesRec* race = g_chrRacesDB.GetRecord(sex);

        if (!race) {
            return nullptr;
        }

        SStrPrintf(fileName, sizeof(fileName), "%s%s_%s%s.mdx", path, rec.m_modelName[index], race->m_clientPrefix, s_genderSuffix[sex]);
    } else {
        SStrPrintf(fileName, sizeof(fileName), "%s%s", path, rec.m_modelName[index]);
    }

    CM2Model* model = World::s_m2Scene->CreateModel(fileName, 0);

    if (!model) {
        return nullptr;
    }

    if (*rec.m_modelTexture[index]) {
        SStrPrintf(fileName, sizeof(fileName), "%s%s", path, rec.m_modelTexture[index]);
        CStatus status;
        CTexture* texture = CreateTexture(fileName, &status);

        if (texture) {
            model->ReplaceTexture(M2TEX_OBJECT_SKIN, texture);
            HandleClose(reinterpret_cast<HOBJECT>(texture));
        }
    }

    CCharacterComponent::ComponentUtilAddItemVisual(model, rec.m_itemVisual);
    CM2Model* worldModel = this->m_worldModel;

    if (scale && worldModel && worldModel->IsLoaded(0, 0)) {
        uint32_t attachId;

        switch (inventoryType) {
            case 1:
                attachId = 11;
                break;
            case 14:
                attachId = 0;
                break;
            case 22:
                attachId = 2;
                break;
            case 24:
            case 25:
                attachId = 0xFFFFFFFF;
                break;
            default:
                attachId = 1;
                break;
        }

        if (worldModel->HasAttachment(attachId)) {
            C44Matrix transform = worldModel->GetAttachmentWorldTransform(attachId);
            *scale = sqrtf(transform.a1 * transform.a1 + transform.a2 * transform.a2 + transform.a0 * transform.a0);
        }
    }

    return model;
}

// VERIFY: 0a6c1295 auto
// OFFSET: 0x00719FF0
UnitBloodRec* CGUnit_C::GetBloodRecord() {
    if (!this->m_bloodlevels) {
        return nullptr;
    }

    return g_unitBloodDB.GetRecord(this->m_bloodlevels->m_violencelevel[ViolenceGetLevel()]);
}

// VERIFY: ce51319d auto
// OFFSET: 0x00725010
void CGUnit_C::OnVirtualItemChanged(VIRTUAL_MONSTER_SLOT slot, int32_t oldItemId) {
    ItemRec* oldRec = g_itemDB.GetRecord(oldItemId);
    ItemRec* rec = g_itemDB.GetRecord(this->m_unit->m_unitVirtualItemSlotId[slot]);
    this->m_virtualItemDisplayId[slot] = rec ? rec->m_displayInfoID : 0;
    CGUnitVirtualItem item;
    item.m_pad[0] = 0;
    item.m_pad[1] = 0;

    if (rec) {
        item.m_classID = static_cast<uint8_t>(rec->m_classID);
        item.m_subclassID = static_cast<uint8_t>(rec->m_subclassID);
        item.m_soundOverrideSubclassID = static_cast<uint8_t>(rec->m_soundOverrideSubclassID);
        item.m_material = static_cast<uint8_t>(rec->m_material);
        item.m_inventoryType = static_cast<uint8_t>(rec->m_inventoryType);
        item.m_sheatheType = static_cast<uint8_t>(rec->m_sheatheType);
    } else {
        item.m_classID = 0;
        item.m_subclassID = 0;
        item.m_soundOverrideSubclassID = 0;
        item.m_material = 0;
        item.m_inventoryType = 0;
        item.m_sheatheType = 0;
    }

    this->m_virtualItem[slot] = item;
    SHEATHE_TYPE sheatheType = static_cast<SHEATHE_TYPE>(oldRec ? oldRec->m_sheatheType : 0);
    CCharacterComponent::RemoveHandItem(this->m_worldModel, s_virtualItemInvSlots[slot], sheatheType, false);

    if (slot == VIRTUAL_MONSTER_SLOT_OFFHAND) {
        CCharacterComponent::RemoveHandItem(this->m_worldModel, s_virtualItemInvSlots[1], sheatheType, true);
    }

    const CGUnitVirtualItem* current = this->GetVirtualItem(slot, false);
    ItemDisplayInfoRec displayRec;

    if (current && this->GetVirtualItemDisplayRec(slot, displayRec)) {
        GEOCOMPONENTLINKS link = CCharacterComponent::AddHandItem(this->m_worldModel, &displayRec, s_virtualItemInvSlots[slot], static_cast<SHEATHE_TYPE>(current->m_sheatheType), this->m_sheatheState == WEAPONMODE_SHEATHEDMODE, current->m_inventoryType == 14, false, 0);

        if (this->m_characterComponent) {
            this->m_characterComponent->SetItemSlotUsingAttachSlot(static_cast<ITEM_SLOT>(-1), link);
        }

        this->SetWeaponTrail(this->m_worldModel, slot);
    }
}

// VERIFY: b524e80c manual
// OFFSET: 0x00726CE0
void CGUnit_C::CancelShapeshift() {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer() || this->m_shapeshiftSuppressed) {
        return;
    }

    if (!this->m_unit->m_unitFieldBytes2[3]) {
        return;
    }

    SpellRec spell;

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        AuraData* aura = this->GetAura(i);

        if (!(aura->m_flags & 0x10) || !g_spellDB.GetRecord(this->GetAura(i)->m_spellId, &spell)) {
            continue;
        }

        for (uint32_t j = 0; j < 3; j++) {
            if (spell.m_effectAura[j] == SPELL_AURA_MOD_SHAPESHIFT && spell.m_effectMiscValue[j] == static_cast<int32_t>(this->GetShapeshiftForm())) {
                Spell_C_CancelAura(&spell, this, -1);
            }
        }
    }
}

// VERIFY: 8e89ab2b manual
// OFFSET: 0x00719B80
void CGUnit_C::ComputeSmoothedSwimMatrix(C44Matrix* matrix, float elapsed, int32_t a3) {
    if (elapsed < 0.011111111f) {
        elapsed = 0.011111111f;
    } else if (elapsed >= 0.050000001f) {
        elapsed = 0.050000001f;
    }

    uint32_t moveFlags = this->m_movement->m_flags;
    float pitch;

    if (moveFlags & 0x3) {
        uint32_t flags = this->m_movementData.m_flags;

        if (flags & 0x400000) {
            pitch = 0.78539819f;
        } else if (flags & 0x800000) {
            pitch = -0.78539819f;
        } else {
            pitch = this->m_movementData.m_pitch;
        }
    } else {
        pitch = 0.0f;
    }

    float basePitch = 0.0f;
    float targetPitch = pitch;

    if (this->m_movementData.m_flags & 0xC) {
        targetPitch = 0.0f;
    } else {
        basePitch = pitch;
    }

    if ((moveFlags & 0x2) && (this->m_movementData.m_flags & 0xC00000)) {
        targetPitch = basePitch * -1.0f;
    }

    float facing = this->GetRenderFacing();
    float facingDelta = facing - this->m_smoothFacing;
    this->m_smoothFacing = facing;

    if (facingDelta > 3.1415927f) {
        facingDelta = facingDelta - 6.2831855f;
    } else if (facingDelta < -3.1415927f) {
        facingDelta = facingDelta + 6.2831855f;
    }

    float pitchDelta = targetPitch - this->m_smoothSwimPitch;

    if (fabs(pitchDelta) >= 0.0099999998f) {
        if (pitchDelta > 3.1415927f) {
            pitchDelta = pitchDelta - 6.2831855f;
        } else if (pitchDelta < -3.1415927f) {
            pitchDelta = pitchDelta + 6.2831855f;
        }

        float rate = elapsed * 12.566371f;

        if (rate < 0.0f) {
            rate = 0.0f;
        } else if (rate >= 1.0f) {
            rate = 1.0f;
        }

        float smoothed = pitchDelta * rate + this->m_smoothSwimPitch;
        this->m_smoothSwimPitch = smoothed;

        if (smoothed > 3.1415927f) {
            this->m_smoothSwimPitch = smoothed - 6.2831855f;
        } else if (smoothed < -3.1415927f) {
            this->m_smoothSwimPitch = smoothed + 6.2831855f;
        }
    } else {
        this->m_smoothSwimPitch = targetPitch;
    }

    float targetRoll;

    if (a3) {
        targetRoll = facingDelta / elapsed * -0.15000001f;
        float rollDelta = targetRoll - this->m_smoothSwimRoll;
        float rate = elapsed * 1.5707964f;

        if (!(this->m_movement->m_flags & 0x3)) {
            rate = rate * 4.0f;
        }

        if (rate < 0.0f) {
            rate = 0.0f;
        } else if (rate >= 1.0f) {
            rate = 1.0f;
        }

        float roll = rollDelta * rate + this->m_smoothSwimRoll;

        if (roll < -0.78539819f) {
            this->m_smoothSwimRoll = -0.78539819f;
        } else if (roll >= 0.78539819f) {
            this->m_smoothSwimRoll = 0.78539819f;
        } else {
            this->m_smoothSwimRoll = roll;
        }
    } else {
        targetRoll = 0.0f;
    }

    if (fabs(facingDelta) > 1.5707964f) {
        this->m_animStateFlags |= 0x4;
    }

    if (this->m_animStateFlags & 0x4) {
        float yawDelta = facing - this->m_smoothFacingTarget;

        if (yawDelta > 3.1415927f) {
            yawDelta = yawDelta - 6.2831855f;
        } else if (yawDelta < -3.1415927f) {
            yawDelta = yawDelta + 6.2831855f;
        }

        float rate = elapsed * 12.566371f;
        float clamped = 0.0f;

        if (rate >= 0.0f) {
            clamped = rate;

            if (rate >= 1.0f) {
                clamped = 1.0f;
            }
        }

        this->m_smoothFacingTarget = CMath::normalizeangle0to2pi_(clamped * yawDelta + this->m_smoothFacingTarget);
    } else {
        this->m_smoothFacingTarget = facing;
    }

    C33Matrix rotation;
    rotation.FromEulerAnglesZYX(-(6.2831855f - this->m_smoothFacingTarget), 6.2831855f - this->m_smoothSwimPitch, this->m_smoothSwimRoll);
    matrix->a0 = rotation.a0;
    matrix->a1 = rotation.a1;
    matrix->a2 = rotation.a2;
    matrix->a3 = 0.0f;
    matrix->b0 = rotation.b0;
    matrix->b1 = rotation.b1;
    matrix->b2 = rotation.b2;
    matrix->b3 = 0.0f;
    matrix->c0 = rotation.c0;
    matrix->c1 = rotation.c1;
    matrix->c2 = rotation.c2;
    matrix->c3 = 0.0f;
    matrix->d0 = 0.0f;
    matrix->d1 = 0.0f;
    matrix->d2 = 0.0f;
    matrix->d3 = 1.0f;
    matrix->Scale(this->GetTrueScale());
    C3Vector position = this->GetPosition();
    matrix->d0 = position.x;
    matrix->d1 = position.y;
    matrix->d2 = position.z;
    uint32_t flags = this->m_animStateFlags & 0xFFFFFFF8;
    flags |= fabs(this->m_smoothSwimPitch - targetPitch) > 0.0099999998f;
    this->m_animStateFlags = flags;
    flags |= fabs(this->m_smoothSwimRoll - targetRoll) <= 0.0099999998f ? 0 : 2;
    this->m_animStateFlags = flags;

    if (fabs(this->m_smoothFacingTarget - facing) <= 0.0099999998f) {
        this->m_animStateFlags = flags;
    } else {
        this->m_animStateFlags = flags | 0x4;
    }
}

// VERIFY: 37162a11 manual
// OFFSET: 0x007169A0
static int32_t UnpackAndProcessOpcodes(CDataStore* msg, uint32_t time, int32_t compressed) {
    uint32_t size;
    msg->Get(size);
    uint32_t unpackedSize = size;
    void* data;

    if (compressed) {
        uint8_t* buffer = static_cast<uint8_t*>(alloca(size));
        uint32_t packedSize = msg->m_size - msg->m_read;
        void* packed;
        msg->GetDataInSitu(packed, packedSize);
        zlib_uncompress(buffer, &unpackedSize, static_cast<const uint8_t*>(packed), packedSize);
        data = buffer;
    } else {
        unpackedSize = msg->m_size - msg->m_read;
        msg->GetDataInSitu(data, unpackedSize);
    }

    CDataStore stream(static_cast<uint8_t*>(data), unpackedSize);

    if (unpackedSize) {
        do {
            uint8_t length;
            stream.Get(length);
            void* message;
            stream.GetDataInSitu(message, length);
            CDataStore packet(static_cast<uint8_t*>(message), length);
            ClientServices::Connection()->ProcessMessage(time, &packet, 0);
        } while (stream.m_read != stream.m_size);
    }

    return 1;
}

// VERIFY: 4e5b26bd auto
// OFFSET: 0x00716AD0
int32_t Packet_SMSG_COMPRESSED_MOVES(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    return UnpackAndProcessOpcodes(msg, time, 1);
}

// VERIFY: 01cd5559 auto
// OFFSET: 0x00716AF0
int32_t Packet_SMSG_MULTIPLE_MOVES(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    return UnpackAndProcessOpcodes(msg, time, 0);
}

// VERIFY: 3187bcbb auto
// OFFSET: 0x00716D80
int32_t Packet_SMSG_PRE_RESURRECT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CMissile::ClearInFlightMissilesWithTarget(guid);
    return 1;
}

// VERIFY: 6511af3b manual
// OFFSET: 0x0071A0B0
void CGUnit_C::NPCFlagChanged(uint32_t oldFlags) {
    uint32_t flags = this->m_unit->m_unitNpcFlags;
    uint32_t changed = oldFlags ^ flags;

    if ((changed & 0x1) && !(flags & 0x1) && CGGossipInfo::m_gossip == this->m_obj->m_guid) {
        CGGossipInfo::SetGossip(WOWGUID());
    }

    if (changed & 0x2) {
        this->RefreshInteractIcon();

        if (!(flags & 0x2) && CGQuestInfo::m_npc == this->m_obj->m_guid) {
            CGQuestInfo::QuestGiverFinished(0, 0, 0);
        }
    }

    if (changed & 0x2000) {
        this->RefreshInteractIcon();

        if (!(flags & 0x2000) && CGTaxiMap::m_unit == this->m_obj->m_guid) {
            CGTaxiMap::CloseMap();
        }
    }

    if ((changed & 0x10) && !(flags & 0x10) && CGClassTrainer::m_trainer == this->m_obj->m_guid) {
        CGClassTrainer::SetTrainer(WOWGUID(), TRAINER_TYPE_GENERAL);
    }

    if ((changed & 0x20000) && !(flags & 0x20000) && CGBankInfo::m_banker == this->m_obj->m_guid) {
        CGBankInfo::SetBanker(WOWGUID());
    }

    if ((changed & 0x800000) && !(flags & 0x800000) && CGGuildBankInfo::m_banker == this->m_obj->m_guid) {
        CGGuildBankInfo::SetGuildBanker(WOWGUID());
    }

    if ((changed & 0x4000000) && !(flags & 0x4000000) && CGMailInfo::m_object == this->m_obj->m_guid) {
        CGPlayer_C::TalkToMailbox(WOWGUID());
    }
}

// VERIFY: dacfedb2 manual
// OFFSET: 0x00740450
void CGUnit_C::OnMountDisplayChanged(int32_t displayId) {
    int32_t current = this->m_mountDisplayId;

    if (displayId == current) {
        return;
    }

    if (!displayId && this->m_vehicle && this->m_vehicle->m_passengers.Head()) {
        this->m_clientFlags |= 0x10000000;
        return;
    }

    this->m_clientFlags &= ~0x10000000;

    if (current) {
        this->DestroyUnitMount(displayId == 0);
    }

    int32_t wasMounted = this->m_mountDisplayId == 0;
    this->m_mountDisplayId = displayId;

    if (displayId) {
        bool noTransition = this->m_mountTransition == nullptr;
        this->CreateUnitMount(0, wasMounted);

        if (noTransition) {
            void* memory = SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0);
            CEffect* effect = memory ? new (memory) CEffect() : nullptr;
            WOWGUID guid = this->m_obj->m_guid;
            effect->AddSpecialEffect(SPECIALEFFECT_MOUNTPOOF, guid, CGObject_C::EffectOnceHandler);
            effect->m_flags |= 0x20;
            effect->Release();
        }
    }

    this->CheckLoopSound();
    this->UpdateInteractIconAttach();
    PlayerNameTriggerNameRegenerate(this->m_nameDesc);
    this->CreateOrDestroyObjectEffectManager();
}

// VERIFY: 9f0a7724 manual
// OFFSET: 0x007412E0
void CGUnit_C::Dismount() {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer() || (this->m_unit->m_unitFieldFlags & 0x100000)) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CANCEL_MOUNT_AURA));
    msg.Finalize();
    ClientServices::Send2(&msg);

    if (!this->m_mountDisplayId) {
        return;
    }

    if (this->m_vehicle && this->m_vehicle->m_passengers.Head()) {
        this->m_clientFlags |= 0x10000000;
        return;
    }

    this->m_clientFlags &= ~0x10000000;
    this->DestroyUnitMount(1);
    this->m_mountDisplayId = 0;
    this->CheckLoopSound();
    this->UpdateInteractIconAttach();
    PlayerNameTriggerNameRegenerate(this->m_nameDesc);
    this->CreateOrDestroyObjectEffectManager();
}

// VERIFY: 0fd6f153 manual
// OFFSET: 0x00741A40
int32_t Packet_SMSG_DISMOUNT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit || !unit->m_mountDisplayId) {
        return 1;
    }

    if (unit->m_vehicle && unit->m_vehicle->m_passengers.Head()) {
        unit->m_clientFlags |= 0x10000000;
        return 1;
    }

    unit->m_clientFlags &= ~0x10000000;
    unit->DestroyUnitMount(1);
    unit->m_mountDisplayId = 0;
    unit->CheckLoopSound();
    unit->UpdateInteractIconAttach();
    PlayerNameTriggerNameRegenerate(unit->m_nameDesc);
    unit->CreateOrDestroyObjectEffectManager();
    return 1;
}

// VERIFY: df49bcb3 auto
// OFFSET: 0x00716060
void CGUnit_C::SetDeferredClientControl(WOWGUID guid, uint8_t state) {
    if (!CGUnit_C::s_deferredClientControlUpdateGUID || CGUnit_C::s_deferredClientControlUpdateGUID == guid || !CGUnit_C::s_deferredClientControlUpdateState || state) {
        CGUnit_C::s_deferredClientControlUpdateGUID = guid;
        CGUnit_C::s_deferredClientControlUpdateState = state;
    }
}

// VERIFY: 1e9d4731 manual
// OFFSET: 0x0072D0B0
int32_t Packet_SMSG_CLIENT_CONTROL_UPDATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    uint8_t state;
    *msg >> guid;
    msg->Get(state);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->ExecuteClientControlUpdate(guid, state);
    } else {
        CGUnit_C::SetDeferredClientControl(guid, state);
    }

    return 1;
}

// VERIFY: b0ae9f03 manual
// OFFSET: 0x0072D130
int32_t Packet_SMSG_CANCEL_AUTO_REPEAT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        if (unit->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            Spell_C_CancelAutoRepeat(false);
        }

        unit->m_animationState &= ~0x200;
        unit->CancelRangedMode();
    }

    return 1;
}

// VERIFY: 1f03226b auto
// OFFSET: 0x00715530
void CGUnit_C::UpdateUnitNameColor() {
    PlayerNameTriggerColorUpdate(this->m_nameDesc);

    if (this->m_namePlateFrame) {
        this->m_namePlateFrame->UpdateColorDisplay(this);
    }
}

// VERIFY: c0f70d76 manual
// OFFSET: 0x00724410
int32_t CGUnit_C::CanPlayEmoteAnim() {
    if (this->IsDeadOrAttachmentDying() || this->IsDeadOrGhost() || this->GetClientStandState() == UNIT_SUBMERGED) {
        return 0;
    }

    uint32_t flags = this->m_movementData.m_flags;

    if ((flags & 0x1000) && (flags & 0x2000)) {
        return 0;
    }

    if (this->IsJumping() || !(flags & 0xF)) {
        return 0;
    }

    if (this->IsLooting() && this->ShouldKneelForLoot()) {
        return 0;
    }

    if (this->m_unit->m_unitChannelSpell || (this->m_animationState & 0x400) || this->IsInMelee() || this->CanPlayTurnAnim()) {
        return 0;
    }

    if (this->m_sheatheState == WEAPONMODE_RANGEDMODE && (this->m_animationState & 0x200)) {
        return 0;
    }

    return !this->GetClientStandState() && !this->m_animStandState;
}

// VERIFY: 21fc02ae auto
// OFFSET: 0x0071A700
int32_t CGUnit_C::GetAbilitySkillLine(int32_t spellId) {
    SkillLineAbilityRec* ability = this->LookupAbility(spellId);
    return ability ? ability->m_skillLine : 0;
}

// VERIFY: 5405d5b8 auto
// OFFSET: 0x00715BF0
static void __stdcall EnableTrackingPitch(uint32_t time, float pitch) {
    CGInputControl::GetActive()->ToggleControlBit(INPUT_TRACKING_PITCH, 1, time, 0);
}

// VERIFY: 4d29d174 auto
// OFFSET: 0x00715C10
static void __stdcall DisableTrackingPitch(uint32_t time) {
    CGInputControl::GetActive()->ToggleControlBit(INPUT_TRACKING_PITCH, 0, time, 0);
}

// VERIFY: b7701f28 manual
// OFFSET: 0x00715C70
bool CGUnit_C::CanMoveTo(int32_t action, const C3Vector& position) {
    if (!s_clickToMoveActions[action].m_enabled || 0.0f == s_clickToMoveActions[action].m_stopDistanceSq) {
        return true;
    }

    C3Vector current = this->GetPosition();
    float dx = position.x - current.x;
    float dy = position.y - current.y;
    float dz = position.z - current.z;

    if (dx * dx + dz * dz + dy * dy < s_clickToMoveActions[action].m_stopDistanceSq) {
        return true;
    }

    CGGameUI::DisplayError(ERR_AUTOFOLLOW_TOO_FAR);
    return false;
}

// VERIFY: 665b03ae manual
// OFFSET: 0x0071AEE0
void CGUnit_C::TrackingStopMove(uint32_t time) {
    if ((this->m_unit->m_unitFieldFlags2 & 0x40) || this->IsVehicleCurrentlyUnstoppable()) {
        return;
    }

    CGInputControl::GetActive()->ToggleControlBit(INPUT_TRACKING_MOVE, 0, time, 0);

    if (CGInputControl::GetActive()->CanControl(this) && (this->m_movement->m_flags & 0x3)) {
        CGUnit_C::s_trackingFlags |= 0x1;
        this->m_movementData.OnMoveStopLocal(time);
        CGInputControl::GetActive()->UpdateMoveStopped();
        CGUnit_C::s_trackingFlags &= ~0x1;
    }
}

// VERIFY: a60a434d manual
// OFFSET: 0x0071B3B0
bool CGUnit_C::GetInteractDistanceOfAction(int32_t action, const WOWGUID& guid, float* distance) {
    if (!s_clickToMoveActions[action].m_enabled || s_clickToMoveActions[action].m_ignoreInteractRange) {
        return true;
    }

    float minRange;
    float maxRange;

    switch (action) {
        case TRACKTYPE_FOLLOW:
            *distance = s_trackingFollowDistanceSq;
            return true;

        case TRACKTYPE_MOVE:
            *distance = s_trackingTerrainDistanceSq;
            return true;

        case TRACKTYPE_INTERACT: {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

            if (unit) {
                float range = unit->m_unit->m_unitFieldCombatreach * 0.5f + 2.0f;
                *distance = range * range;
            } else {
                *distance = s_trackingDefaultInteractDistanceSq;
            }

            return true;
        }

        case TRACKTYPE_LOOT:
        case TRACKTYPE_ATTACK: {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));
            float range = unit ? this->CGUnit::GetMeleeRange(unit) - 1.3333334f : 5.0f;
            *distance = range * range;
            return true;
        }

        case TRACKTYPE_USE_OBJECT:
            *distance = s_trackingObjectRange * s_trackingObjectRange;
            return true;

        case TRACKTYPE_SKIN: {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__));

            if (!unit) {
                return false;
            }

            int32_t spellId = CGSpellBook::GetSkinningSpell(unit);

            if (!spellId) {
                return false;
            }

            Spell_C::GetSpellRange(this, spellId, &minRange, &maxRange, unit);
            break;
        }

        case TRACKTYPE_RANGED_ATTACK: {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

            if (!unit) {
                return false;
            }

            Spell_C::GetSpellRange(this, CGSpellBook::m_autoRangedSpell, &minRange, &maxRange, unit);
            break;
        }

        default:
            return false;
    }

    float range = maxRange * 0.9f;
    *distance = range * range;
    return true;
}

// VERIFY: 1484b729 manual
// OFFSET: 0x00727400
bool CGPlayer_C::ClickToMove(int32_t type, const WOWGUID* guid, const C3Vector* position, float facing) {
    if (this->m_obj->m_guid == CGUnit_C::s_activeMover && CGUnit_C::s_trackingType != TRACKTYPE_NONE) {
        if (CGUnit_C::s_trackingType == TRACKTYPE_FOLLOW) {
            FrameScript_SignalEvent(SCRIPT_EVENT_AUTOFOLLOW_END, nullptr);
        }

        OsGetAsyncTimeMs();
        CGUnit_C::s_unused_CA11D0 = 0.0f;
        CGUnit_C::s_trackingGUID = WOWGUID();
        CGUnit_C::s_trackingFlags = 0;
        CGUnit_C::s_trackingStuckCount = 0;
        CGUnit_C::s_trackingType = TRACKTYPE_NONE;
    }

    if (!this->CanMoveTo(type, *position) || !this->GetInteractDistanceOfAction(type, *guid, &s_trackingInteractDistanceSq)) {
        return false;
    }

    CGUnit_C::s_trackingType = type;
    s_trackingLastTime = OsGetAsyncTimeMs();
    s_trackingStartPosition = this->m_movementData.GetPosition(this->GetPosition());
    CGUnit_C::s_trackingDirection = facing;
    CGUnit_C::s_trackingGUID = *guid;
    CGUnit_C::s_trackingPosition = *position;
    s_trackingTurnRate = (this->m_movement->m_flags2 & 0x8) ? this->m_movement->m_turnRate : 3.1415927f;
    CGUnit_C::s_trackingStuckCount = 0;
    s_trackingInteractDistance = sqrt(s_trackingInteractDistanceSq);
    CGUnit_C::s_unused_CA11D0 = 0.0f;

    if (0.0f == s_forcedRunSpeed) {
        s_forcedRunSpeed = this->m_movementData.m_runSpeed;
    }

    if (this->GetClientStandState() && (this->m_obj->m_type & TYPE_PLAYER)) {
        this->ChangeStandState(UNIT_STANDING);
    }

    return true;
}

// VERIFY: ba7cd01f manual
// OFFSET: 0x00729BD0
bool CGUnit_C::IsValidFollowTarget(CGUnit_C* unit) {
    if (unit == this || !(unit->m_obj->m_type & TYPE_PLAYER) || unit->m_unit->m_unitFieldCharmedby) {
        return false;
    }

    FactionTemplateRec* faction = g_factionTemplateDB.GetRecord(this->m_unit->m_unitFieldFactiontemplate);
    FactionTemplateRec* other = g_factionTemplateDB.GetRecord(unit->m_unit->m_unitFieldFactiontemplate);

    if (faction && other && faction->m_factionGroup != other->m_factionGroup) {
        return false;
    }

    return !this->CanAttack(unit);
}

// VERIFY: b7e6c0b3 auto
// OFFSET: 0x0072B3A0
void CGUnit_C::HandleTrackingMoveThreshold() {
    switch (CGUnit_C::s_trackingType) {
        case TRACKTYPE_MOVE:
            this->ClearTrackingTarget(true, true);
            break;

        case TRACKTYPE_INTERACT:
            s_pendingInteractTarget = CGUnit_C::s_trackingGUID;
            this->ClearTrackingTarget(true, true);
            break;

        case TRACKTYPE_LOOT:
            s_pendingLootTarget = CGUnit_C::s_trackingGUID;
            this->ClearTrackingTarget(true, true);
            break;

        case TRACKTYPE_USE_OBJECT:
            s_pendingUseObject = CGUnit_C::s_trackingGUID;
            this->ClearTrackingTarget(true, true);
            break;

        case TRACKTYPE_SKIN:
            s_pendingSkinTarget = CGUnit_C::s_trackingGUID;
            this->ClearTrackingTarget(true, true);
            break;

        case TRACKTYPE_ATTACK:
        case TRACKTYPE_RANGED_ATTACK:
            s_pendingAttackTarget = CGUnit_C::s_trackingGUID;
            this->ClearTrackingTarget(true, true);
            break;

        default:
            this->TrackingStopMove(OsGetAsyncTimeMs());
            break;
    }
}

// VERIFY: 0e869071 manual
// OFFSET: 0x0072B4A0
bool CGUnit_C::SetTrackingTarget(const WOWGUID& target, int32_t type) {
    if (target == this->m_obj->m_guid || this->m_obj->m_guid != CGUnit_C::s_activeMover) {
        return false;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(target, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return false;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(object);

    switch (type) {
        case TRACKTYPE_SPELLPRECAST:
        case TRACKTYPE_SPELLCHANNEL:
        case TRACKTYPE_INTERACT:
            if (!(object->m_obj->m_type & TYPE_UNIT)) {
                return false;
            }

            break;

        case TRACKTYPE_FOLLOW:
            if (!this->IsValidFollowTarget(unit)) {
                CGGameUI::DisplayError(ERR_INVALID_FOLLOW_TARGET);
                return false;
            }

            if (this->m_unit->m_unitFieldHealth <= 0) {
                CGGameUI::DisplayError(ERR_PLAYER_DEAD);
                return false;
            }

            if (this->m_unit->m_unitFieldFlags & 0x40000) {
                CGGameUI::DisplayError(ERR_GENERIC_STUNNED);
                return false;
            }

            if (this->m_unit->m_unitChannelSpell) {
                CGGameUI::DisplayError(ERR_TOOBUSYTOFOLLOW);
                return false;
            }

            break;

        case TRACKTYPE_LOOT:
        case TRACKTYPE_SKIN:
            if (!(object->m_obj->m_type & TYPE_CORPSE) && (!(object->m_obj->m_type & TYPE_UNIT) || unit->m_unit->m_unitFieldHealth > 0)) {
                return false;
            }

            break;

        default:
            return false;
    }

    if (this->m_unit->m_unitFieldHealth <= 0) {
        return false;
    }

    C3Vector position = object->GetPosition();

    if (!static_cast<CGPlayer_C*>(this)->ClickToMove(type, &target, &position, 0.0f)) {
        return false;
    }

    if (type == TRACKTYPE_FOLLOW) {
        FrameScript_SignalEvent(SCRIPT_EVENT_AUTOFOLLOW_BEGIN, "%s", unit->GetUnitName(nullptr, 1));
    }

    return true;
}

// VERIFY: a13af266 auto
// OFFSET: 0x0072B660
bool CGPlayer_C::CTMFace(float facing) {
    if (!this->CanAutoInteract()) {
        return false;
    }

    C3Vector position = C3Vector();
    WOWGUID guid = WOWGUID();
    return this->ClickToMove(TRACKTYPE_FACE, &guid, &position, facing);
}

// VERIFY: a8c5df60 auto
// OFFSET: 0x0072B6D0
bool CGPlayer_C::CTMClickTerrain(const C3Vector& position) {
    if (!this->CanAutoInteract()) {
        return false;
    }

    WOWGUID guid = WOWGUID();
    return this->ClickToMove(TRACKTYPE_MOVE, &guid, &position, 0.0f);
}

// VERIFY: 04389893 manual
// OFFSET: 0x0072B730
bool CGPlayer_C::CTMInteractGO(const WOWGUID& guid, float range) {
    if (!this->CanAutoInteract()) {
        return false;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_GAMEOBJECT, __FILE__, __LINE__);

    if (!object) {
        return false;
    }

    C3Vector position = object->GetPosition();

    if (!this->ClickToMove(TRACKTYPE_USE_OBJECT, &guid, &position, 0.0f)) {
        return false;
    }

    s_trackingObjectRange = range * 0.9f;
    return true;
}

// VERIFY: 79f8d6ee auto
// OFFSET: 0x0072ECF0
void CGUnit_C::TrackingStartMove(uint32_t time) {
    CGInputControl::GetActive()->ToggleControlBit(INPUT_TRACKING_MOVE, 1, time, 0);

    if (CGInputControl::GetActive()->CanControl(this)) {
        CGUnit_C::s_trackingFlags |= 0x1;
        this->OnMoveStartLocal(time, 1);
        CGUnit_C::s_trackingFlags &= ~0x1;
    }
}

// VERIFY: ed86e3fe auto
// OFFSET: 0x0072ED40
void CGUnit_C::TrackingSetPitch(uint32_t time, float pitch) {
    if (CGInputControl::GetActive()->CanControl(this)) {
        CGUnit_C::s_trackingFlags |= 0x1;
        this->OnSetPitchLocal(time, pitch);
        CGUnit_C::s_trackingFlags &= ~0x1;
    }
}

// VERIFY: 8bd70d40 manual
// OFFSET: 0x0072EE20
bool CGPlayer_C::CTMAttackUnit(CGUnit_C* unit) {
    int32_t modalSpell = 0;
    float range = static_cast<CGUnit*>(unit)->m_unit->m_unitFieldCombatreach + this->m_unit->m_unitFieldCombatreach + 1.3333334f;

    if (range < 5.0f) {
        range = 5.0f;
    }

    C3Vector target = unit->GetPosition();
    C3Vector position = this->GetPosition();
    float dz = position.z - target.z;
    float dy = position.y - target.y;
    float dx = position.x - target.x;

    if (range * range > dx * dx + (dy * dy + dz * dz)) {
        C3Vector current = this->GetPosition();
        return this->CTMFace(CalculateFacingTo(&current, &target));
    }

    if (this->m_unit->m_unitFieldHealth <= 0 || this->m_obj->m_guid != CGUnit_C::s_activeMover || !g_cvAutoInteract->m_intValue) {
        return false;
    }

    CAaBox box;
    box.b = target;
    box.t = target;

    // STATIC: 0x00CA1284
    static World::FacetData s_facets;

    s_facets.facets.SetCount(0);
    s_facets.facetIds.m_count = 0;

    if (World::GetFacets(&box, &s_facets, 0x100111, nullptr) && s_facets.facets.m_count) {
        const C4Plane& plane = s_facets.facets.m_data[0].plane;
        target.z = -((plane.n.y * target.y + plane.n.x * target.x + plane.d) / plane.n.z);
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        modalSpell = GetModalNextSpell();
    }

    WOWGUID guid = unit->m_obj->m_guid;
    return this->ClickToMove((modalSpell != 0) + TRACKTYPE_ATTACK, &guid, &target, 0.0f);
}

// VERIFY: c211e617 manual
// OFFSET: 0x0072F040
void CGUnit_C::SetTrackingDirection(const C3Vector& direction) {
    if (!this->CanAutoInteract()) {
        return;
    }

    C3Vector none = C3Vector();

    if (!this->CanMoveTo(TRACKTYPE_MOVE_DIRECTION, none)) {
        return;
    }

    C3Vector dir = direction;
    float scale = 1.0f / sqrt(dir.y * dir.y + dir.x * dir.x + dir.z * dir.z);
    dir.x *= scale;
    dir.y *= scale;
    dir.z *= scale;

    bool flying = (this->m_movement->m_flags & 0x2200000) != 0;
    float up = fabs((dir.x + dir.y) * 0.0f + dir.z);

    if (flying) {
        float pitch = 0.0f;

        if (up > 0.86602002f) {
            pitch = CopySign(1.5707964f, dir.z);
        } else if (up > 0.17364f) {
            pitch = asin(dir.z);
        }

        this->OnSetPitchLocal(OsGetAsyncTimeMs(), pitch);
    }

    C3Vector current = this->GetPosition();
    C3Vector target;
    target.x = current.x + dir.x;
    target.y = current.y + dir.y;
    target.z = current.z + dir.z;

    C3Vector position = this->GetPosition();
    float facing = CalculateFacingTo(&position, &target);
    C3Vector zero = C3Vector();
    WOWGUID guid = WOWGUID();
    static_cast<CGPlayer_C*>(this)->ClickToMove(TRACKTYPE_MOVE_DIRECTION, &guid, &zero, facing);
}

// VERIFY: f1632e4a auto
// OFFSET: 0x0072F1F0
bool CGUnit_C::HandleTrackingPitch(float targetZ, float z, float pitch, float maxStep, uint32_t time, bool* overshoot) {
    int32_t liquidType;
    int32_t liquidFlags;

    if (z < targetZ && World::QueryObjectLiquid(this->m_worldObject, &liquidType, &targetZ, &liquidFlags) && targetZ > z && fabs(targetZ - z - this->m_movementData.m_collisionHeight * 0.75f) < 0.001f) {
        if (0.0f != this->GetPitch()) {
            EnableTrackingPitch(time, 0.0f);
            this->TrackingSetPitch(time, 0.0f);
        }

        return false;
    }

    CGInputControl::GetActive()->ToggleControlBit(INPUT_TRACKING_PITCH, 1, time, 0);

    float current = this->m_movementData.m_pitch;

    if (fabs(pitch - current) < 0.001f) {
        return false;
    }

    float newPitch = pitch;

    if (pitch <= current) {
        float step = current - maxStep;

        if (step > newPitch) {
            newPitch = step;
        }
    } else {
        float step = current + maxStep;

        if (step < newPitch) {
            newPitch = step;
        }
    }

    this->TrackingSetPitch(time, newPitch);
    *overshoot = fabs(newPitch - pitch) > 0.39269909f;
    return true;
}

// VERIFY: 2ca09313 auto
// OFFSET: 0x00731600
void CGUnit_C::TrackingSetTurn(uint32_t time, float facing) {
    if (CGInputControl::GetActive()->CanControl(this)) {
        CGUnit_C::s_trackingFlags |= 0x1;
        this->OnSetRawFacingLocal(time, this->m_movement->GetRawFacing(facing));
        CGUnit_C::s_trackingFlags &= ~0x1;
    }
}

// VERIFY: 6ba835b4 auto
// OFFSET: 0x00731650
bool CGUnit_C::HandleTrackingFacing(float facing, float maxTurn, uint32_t time) {
    CGUnit_C::s_trackingFacing = facing;
    CGInputControl::GetActive()->ToggleControlBit(INPUT_TRACKING_TURN, 1, time, 0);

    float delta = CMath::normalizeangle0to2pi_(facing - this->GetFacing());

    if (fabs(delta) < 0.001f || fabs(delta - 6.2831855f) < 0.001f) {
        return false;
    }

    CMovementShared* movement = this->m_movement;

    if (movement->m_flags2 & 0x8) {
        s_trackingTurnRate = movement->m_turnRate;
    } else if (3.1415927f == s_trackingTurnRate) {
        s_trackingTurnRate = (delta <= 2.0943952f || delta >= 4.1887903f ? 800.0f : 1200.0f) * 0.017453292f;
    }

    float turn = delta <= 3.1415927f ? delta : 6.2831855f - delta;

    if (maxTurn <= turn) {
        turn = maxTurn;
    }

    float newFacing = delta <= 3.1415927f ? this->GetFacing() + turn : this->GetFacing() - turn;
    this->TrackingSetTurn(time, CMath::normalizeangle0to2pi_(newFacing));
    return true;
}

// VERIFY: 4ccaa4b6 manual
// OFFSET: 0x007317A0
void CGUnit_C::HandleTracking(float drunkness, float turnOffset) {
    C3Vector target = C3Vector();
    bool useTarget = true;
    bool stuck = false;

    switch (CGUnit_C::s_trackingType) {
        case TRACKTYPE_FACE:
        case TRACKTYPE_MOVE_DIRECTION:
            useTarget = false;
            break;

        case TRACKTYPE_MOVE:
        case TRACKTYPE_ATTACK:
            target = this->m_movementData.GetPosition(CGUnit_C::s_trackingPosition);
            break;

        default: {
            CGObject_C* object = ClntObjMgrObjectPtr(CGUnit_C::s_trackingGUID, TYPE_OBJECT, __FILE__, __LINE__);

            if (!object) {
                this->ClearTrackingTarget(false, true);
                return;
            }

            if (object->m_obj->m_type & TYPE_UNIT) {
                CGUnit_C* unit = static_cast<CGUnit_C*>(object);

                if ((unit->m_unit->m_unitFieldFlags & 0x100000) || (unit->m_unit->m_unitFieldHealth <= 0 && CGUnit_C::s_trackingType != TRACKTYPE_SKIN && CGUnit_C::s_trackingType != TRACKTYPE_LOOT)) {
                    this->ClearTrackingTarget(false, true);
                    return;
                }
            }

            target = object->GetPosition();
            break;
        }
    }

    C3Vector position = this->GetPosition();
    C3Vector start = this->m_movementData.GetPosition(s_trackingStartPosition);
    s_trackingStartPosition = this->GetRawPosition();

    float dx = position.x - target.x;
    float dy = position.y - target.y;
    float dz = position.z - target.z;
    float distSq2 = dx * dx + dy * dy;
    float distSq3 = dz * dz + distSq2;
    float distSq = ((this->m_movement->m_flags & 0x2200000) || (this->m_movementData.m_flags & 0x1000000)) ? distSq3 : distSq2;

    if (s_clickToMoveActions[CGUnit_C::s_trackingType].m_enabled && 0.0f != s_clickToMoveActions[CGUnit_C::s_trackingType].m_stopDistanceSq && !(distSq < s_clickToMoveActions[CGUnit_C::s_trackingType].m_stopDistanceSq)) {
        this->ClearTrackingTarget(false, true);
        return;
    }

    float speedScale = this->m_movement->GetBaseSpeed(0) * 0.14285715f < 1.0f ? 1.0f : this->m_movement->GetBaseSpeed(0) * 0.14285715f;

    if (s_clickToMoveActions[CGUnit_C::s_trackingType].m_enabled && (CGUnit_C::s_trackingFlags & 0x2)) {
        switch (CGUnit_C::s_trackingType) {
            case TRACKTYPE_FACE:
            case TRACKTYPE_FOLLOW:
            case TRACKTYPE_INTERACT:
            case TRACKTYPE_MOVE_DIRECTION:
                break;

            default: {
                float sx = start.x - target.x;
                float sy = start.y - target.y;
                float sz = start.z - target.z;

                if (sx * sx + sy * sy + sz * sz < distSq3) {
                    stuck = true;

                    if (++CGUnit_C::s_trackingStuckCount >= 5) {
                        this->ClearTrackingTarget(false, true);
                        return;
                    }
                }

                break;
            }
        }
    }

    uint32_t time = OsGetAsyncTimeMs();
    float elapsed = fabs(static_cast<float>(time - s_trackingLastTime)) * 0.001f;
    s_trackingLastTime = time;

    float facingTo = CMath::normalizeangle0to2pi_(useTarget ? CalculateFacingTo(&position, &target) : CGUnit_C::s_trackingDirection);
    float desired = facingTo;
    bool moving = false;

    if (drunkness > 0.0f) {
        moving = true;

        if ((this->m_movement->m_flags & 0xF) && (CGUnit_C::s_trackingFlags & 0x2)) {
            desired = this->GetFacing() + turnOffset;
        }
    }

    bool turning = this->HandleTrackingFacing(desired, static_cast<float>(fmod(static_cast<double>(s_trackingTurnRate * elapsed), 6.283185307179586)), time);

    if (!turning) {
        CGInputControl::GetActive()->ToggleControlBit(INPUT_TRACKING_TURN, 0, time, 0);

        if (CGUnit_C::s_trackingType == TRACKTYPE_FACE) {
            this->ClearTrackingTarget(false, true);
            return;
        }

        CGUnit_C::s_trackingFlags |= 0x2;
    } else if (moving && CGUnit_C::s_trackingType == TRACKTYPE_FOLLOW && CRandom::ufloat_(g_rndSeed) * drunkness < 0.07f) {
        CGUnit_C::s_trackingFlags &= ~0x2;
    }

    bool overshoot = false;
    float pitchStep = static_cast<float>(fmod(static_cast<double>(elapsed * 3.1415927f), 6.283185307179586));
    bool pitching = false;
    uint32_t moveFlags = this->m_movement->m_flags;

    if ((moveFlags & 0x2200000) && useTarget && s_trackingStuckDistanceSq < distSq3) {
        float pitch;

        if (moving && (moveFlags & 0x3) && (CGUnit_C::s_trackingFlags & 0x4)) {
            pitch = this->GetPitch() + turnOffset;
        } else {
            pitch = CGUnit_C::ComputeTrackingElevationAngle(target, position);
        }

        pitching = this->HandleTrackingPitch(target.z, position.z, pitch, pitchStep, time, &overshoot);

        if (!pitching) {
            DisableTrackingPitch(time);
            CGUnit_C::s_trackingFlags |= 0x4;
        }
    }

    if (!s_clickToMoveActions[CGUnit_C::s_trackingType].m_enabled) {
        return;
    }

    if (moving && (CGUnit_C::s_trackingFlags & 0x2)) {
        float diff = CMath::normalizeangle0to2pi_(facingTo - this->GetFacing());

        if (diff > 2.7925267f && diff < 3.8397243f) {
            this->ClearTrackingTarget(false, true);
            return;
        }
    }

    if ((this->m_movement->m_flags & 0x3) && !turning && !pitching && s_clickToMoveActions[CGUnit_C::s_trackingType].m_detectStuck) {
        float mx = position.x - start.x;
        float my = position.y - start.y;
        float mz = position.z - start.z;

        if (mx * mx + my * my + mz * mz < s_trackingStuckDistanceSq) {
            stuck = true;

            if (++CGUnit_C::s_trackingStuckCount >= 5) {
                this->ClearTrackingTarget(false, true);
                return;
            }
        }
    }

    if (!s_clickToMoveActions[CGUnit_C::s_trackingType].m_ignoreInteractRange) {
        float range = s_trackingInteractDistance;

        if (CGUnit_C::s_trackingType == TRACKTYPE_MOVE) {
            range = (this->m_movement->GetBaseSpeed(0) * 0.14285715f < 1.0f ? 1.0f : this->m_movement->GetBaseSpeed(0) * 0.14285715f) * range;
        }

        if (!(range * range < distSq)) {
            this->HandleTrackingMoveThreshold();
            return;
        }
    }

    if (!stuck) {
        CGUnit_C::s_trackingStuckCount = 0;
    }

    // STATIC: 0x00CA12A8
    static float s_flightAngleCos = cos(0.7853981852531433);

    if (!pitching && useTarget && !(this->m_movement->m_flags & 0x2200000) && (this->m_movementData.m_flags & 0x1000000)) {
        C3Vector dir;
        dir.x = target.x - position.x;
        dir.y = target.y - position.y;
        dir.z = target.z - position.z;

        if (dir.x * dir.x + dir.y * dir.y + dir.z * dir.z > 2.0f) {
            dir.Normalize();

            if (fabs((dir.y + dir.x) * 0.0f + dir.z) > s_flightAngleCos) {
                this->m_movementData.OnFlightLocal(time, 1);
                CGUnit_C::s_flightStatusTime = time;
                return;
            }
        }
    }

    float speed;

    if (this->m_movementData.m_flags & 0x100) {
        speed = this->m_movementData.m_walkSpeed;
    } else {
        speed = this->m_movementData.m_runSpeed;
    }

    int32_t canMove = (!pitching || !overshoot) && this->CGUnit::CanMove() && this->IsActiveMover() && this->m_unit->m_unitFieldHealth > 0 && !this->CGUnit::IsOnSpline() && !this->AnimSuppressesMovement();

    if (canMove == static_cast<int32_t>(this->m_movement->m_flags & 0x3)) {
        return;
    }

    if (!canMove) {
        this->TrackingStopMove(time);
        return;
    }

    float scale = speed * 0.14285715f;

    if (scale < 1.0f) {
        scale = 1.0f;
    }

    scale *= TRACKMOVE_THRESHOLD_DISTANCE;

    if (CGUnit_C::s_trackingType != TRACKTYPE_FOLLOW || scale * scale <= distSq3) {
        this->TrackingStartMove(time);
    }
}

// VERIFY: d76f111e manual
// OFFSET: 0x00725980
void CGUnit_C::RenderTargetSelection() const {
    if (!CGGameUI::m_visible) {
        return;
    }

    C3Vector position = const_cast<CGUnit_C*>(this)->GetModelPosition();
    float radius = this->m_selectionRadius;
    CAaBox box;
    box.b.x = position.x - radius;
    box.b.y = position.y - radius;
    box.b.z = position.z - radius;
    box.t.x = position.x + radius;
    box.t.y = position.y + radius;
    box.t.z = position.z + radius;

    CImVector color;
    *reinterpret_cast<uint32_t*>(&color) = 0;
    this->GetSelectionHighlightColor(&color);

    C44Matrix matrix;
    CGObject_C::BuildSelectionRotMatrix(&matrix, &position);
    g_theGxDevicePtr->RsPush();
    int32_t states = CGObject_C::SetCircleRenderStates();

    if (states) {
        ProjectTex::Render2d(box, color, &matrix, 0.5f, 0x200122, 1, 0.4f);
    }

    if (this->m_clientFlags & 0x10) {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
        uint32_t now = FrameTime::s_curTimeMs;
        uint32_t start = player->m_combatModeTime;
        uint32_t end = start + CGPlayer_C::GetCombatModeFlashDuration();

        if (static_cast<int32_t>(now - start) >= 0 && static_cast<int32_t>(now - end) < 0) {
            float t = static_cast<float>(static_cast<int32_t>(now - start)) / CGPlayer_C::GetCombatModeFlashDuration();
            float shrink = (1.0f - t) * ((1.0f - t) * (1.0f - t));
            float grow = (1.6f * radius + 2.3f - radius) * shrink;
            float fade = 1.0f - shrink;

            if (t > 0.5f) {
                float half = t - 0.5f + t - 0.5f;
                fade = (1.0f - half * half) * fade;
            }

            color.a = static_cast<uint8_t>(fade * color.a);
            C3Vector amount(grow, grow, grow);
            box.Expand(amount);

            if (states) {
                ProjectTex::Render2d(box, color, &matrix, 0.5f, 0x200122, 1, 0.4f);
            }
        }
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 652f3dea manual
// OFFSET: 0x00725BF0
void CGUnit_C::RenderPetTargetSelection() const {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    const WOWGUID& petGuid = player->m_unit->m_unitFieldCharm ? player->m_unit->m_unitFieldCharm : player->m_unit->m_unitFieldSummon;
    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(petGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (!pet) {
        return;
    }

    uint32_t elapsed = FrameTime::s_curTimeMs - pet->m_targetChangeTime;

    if (static_cast<int32_t>(elapsed) < 0 || static_cast<int32_t>(elapsed - 1000) >= 0) {
        return;
    }

    C3Vector position = const_cast<CGUnit_C*>(this)->GetModelPosition();
    float radius = this->m_selectionRadius;
    float t = elapsed * 0.001f;
    CImVector color;
    *reinterpret_cast<uint32_t*>(&color) = 0xFF605FFF;
    float size = radius + (1.0f - t) * ((1.0f - t) * (1.0f - t)) * (radius * 1.5f + 2.3f - radius);

    if (t > 0.75f) {
        float x = (t - 0.75f) * 4.0f;
        color.a = static_cast<uint8_t>((1.0f - x * x) * 255.0f);
    } else if (t < 0.25f) {
        float x = t * 4.0f;
        color.a = static_cast<uint8_t>(x * x * 255.0f);
    }

    C3Vector amount(size, size, size + size);
    CAaBox box;
    box.b = position;
    box.t = position;
    box.Expand(amount);

    C44Matrix matrix;
    CGObject_C::BuildSelectionRotMatrix(&matrix, &position);
    g_theGxDevicePtr->RsPush();

    if (CGObject_C::SetCircleRenderStates()) {
        ProjectTex::Render2d(box, color, &matrix, 0.5f, 0x200122, 1, 0.4f);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 24ac8386 auto
// OFFSET: 0x0073AD60
void CGUnit_C::OnSpecialMountAnim() {
    this->PlayBaseAnimation(ANIM_MOUNT_SPECIAL, 0);
}

// VERIFY: d5c90727 manual
// OFFSET: 0x00720F80
const SpellVisualRec* CGUnit_C::GetAppropriateSpellVisual(const SpellRec* spell, SpellVisualRec& visual, int32_t visualId, int32_t lowViolenceVisualId) const {
    int32_t id = visualId;
    const SpellVisualRec* weapon = nullptr;
    const SpellVisualRec* base;
    const SpellVisualRec* primary = nullptr;

    if (visualId) {
        if (ViolenceGetLevel() < VIOLENCELEVEL_HIGH && lowViolenceVisualId) {
            id = lowViolenceVisualId;
        }

        base = g_spellVisualDB.GetRecord(id);
        primary = GetSpellVisual(spell);
    } else {
        base = GetSpellVisual(spell);
    }

    if (spell->m_attributes & 0x2) {
        uint32_t displayId = this->GetVirtualItemDisplayID(VIRTUAL_MONSTER_SLOT_RANGED);
        ItemDisplayInfoRec itemRec;

        if (g_itemDisplayInfoDB.GetRecord(displayId, &itemRec) && itemRec.m_spellVisualID) {
            weapon = g_spellVisualDB.GetRecord(itemRec.m_spellVisualID);
        }
    }

    if (base) {
        visual = *base;

        if (primary) {
            visual.m_precastKit = primary->m_precastKit;
            visual.m_castKit = primary->m_castKit;
            visual.m_impactKit = primary->m_impactKit;
            visual.m_casterImpactKit = primary->m_casterImpactKit;
            visual.m_targetImpactKit = primary->m_targetImpactKit;
        }
    } else {
        if (!weapon) {
            return nullptr;
        }

        memset(&visual, 0, sizeof(visual));
    }

    if (weapon) {
        if (!visual.m_precastKit) {
            visual.m_precastKit = weapon->m_precastKit;
        }

        if (!visual.m_castKit) {
            visual.m_castKit = weapon->m_castKit;
        }

        if (!visual.m_impactKit) {
            visual.m_impactKit = weapon->m_impactKit;
        }

        if (!visual.m_casterImpactKit) {
            visual.m_casterImpactKit = weapon->m_casterImpactKit;
        }

        if (!visual.m_targetImpactKit) {
            visual.m_targetImpactKit = weapon->m_targetImpactKit;
        }

        if (!visual.m_hasMissile && weapon->m_hasMissile) {
            visual.m_hasMissile = 1;
            visual.m_missileModel = weapon->m_missileModel;
        }

        if (!visual.m_missileAttachment) {
            visual.m_missileAttachment = weapon->m_missileAttachment;
        }

        if (!visual.m_missileDestinationAttachment) {
            visual.m_missileDestinationAttachment = weapon->m_missileDestinationAttachment;
        }

        if (!visual.m_missileFollowGroundHeight) {
            visual.m_missileFollowGroundHeight = weapon->m_missileFollowGroundHeight;
        }

        if (!visual.m_missileFollowGroundDropSpeed) {
            visual.m_missileFollowGroundDropSpeed = weapon->m_missileFollowGroundDropSpeed;
        }

        if (!visual.m_missileFollowGroundApproach) {
            visual.m_missileFollowGroundApproach = weapon->m_missileFollowGroundApproach;
        }

        if (!visual.m_missileFollowGroundFlags) {
            visual.m_missileFollowGroundFlags = weapon->m_missileFollowGroundFlags;
        }

        if (!visual.m_missileMotion) {
            visual.m_missileMotion = weapon->m_missileMotion;
        }

        if (!visual.m_missileSound) {
            visual.m_missileSound = weapon->m_missileSound;
        }

        if (!visual.m_animEventSoundID) {
            visual.m_animEventSoundID = weapon->m_animEventSoundID;
        }
    }

    return &visual;
}

// VERIFY: 8b5900cb manual
// OFFSET: 0x00722760
void CGUnit_C::ConnectToLightningThisFrame(uint32_t spellId, int32_t attachment) {
    CGObject_C::ConnectToLightningThisFrame(spellId, attachment);

    SpellRec spellRec;
    spellRec.m_id = 0;

    for (uint32_t i = 0; i < this->m_delayedSpellVisualKits.m_count; i++) {
        DelayedSpellVisualKit* delayed = &this->m_delayedSpellVisualKits.m_data[i];

        if (!(delayed->m_flags & 0x8) || delayed->unk_0028 || delayed->m_spellId != static_cast<int32_t>(spellId) || delayed->m_delay != attachment) {
            continue;
        }

        if (spellRec.m_id || g_spellDB.GetRecord(spellId, &spellRec)) {
            SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(delayed->m_kitId);

            if (kit) {
                PlaySpellVisuaKitData data(&spellRec, kit, delayed->m_kitType);

                if (delayed->m_flags & 0x1) {
                    data.m_position = &delayed->m_position;
                    data.m_transportGUID = delayed->m_targetGuid;
                }

                data.m_harmful = (delayed->m_flags >> 2) & 0x1;
                data.m_playAnim = (delayed->m_flags >> 1) & 0x1;
                data.unused_0020 = 1;
                data.unused_0024 = delayed->m_delay;
                this->PlaySpellVisualKit(data);
            }
        }

        this->m_delayedSpellVisualKits.m_data[i].m_flags &= ~0x8;
    }
}

// VERIFY: e43ed8ee manual
// OFFSET: 0x00716DB0
int32_t Packet_SMSG_PLAYER_VEHICLE_DATA(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    uint32_t vehicleId;
    msg->Get(vehicleId);

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        if (vehicleId) {
            unit->CreateVehicleData(nullptr, vehicleId);
        } else {
            unit->DestroyVehicleData(1);
        }
    }

    return 1;
}

// VERIFY: dc4851b4 manual
// OFFSET: 0x007193F0
void CGUnit_C::MountTransitionSetCharacterTransform(float elapsed) {
    if (SpellVisualsGetMountTransitionHeight(this->m_mountTransition) == 0.0f) {
        this->CGObject_C::Animate(elapsed);
        return;
    }

    C3Vector normal = this->m_movement->GetGroundNormal();

    if (normal.z >= 0.35721236f) {
        float dx = this->m_groundNormal.x - normal.x;
        float dy = this->m_groundNormal.y - normal.y;
        float dz = this->m_groundNormal.z - normal.z;
        float decay = static_cast<float>(pow(0.0018f, elapsed));
        this->m_groundNormal.x = dx * decay + normal.x;
        this->m_groundNormal.y = dy * decay + normal.y;
        this->m_groundNormal.z = dz * decay + normal.z;
    }

    CM2Model* mountModel = this->m_mountTransitionEffect->m_model;

    if (!(mountModel->m_fFlags & 0x1)) {
        mountModel->WaitForLoad(nullptr);
    }

    uint32_t flags = mountModel->m_shared->m_data->m_flags;
    CM2Model* model = this->GetObjectModel();

    if (model) {
        model->SetWorldTransform(this->GetPosition(), this->GetRenderFacing(), this->GetTrueScale(), this->m_groundNormal, flags & 0x3);
    }

    C44Matrix transform = this->GetObjectModel()->m_worldTransform;
    C3Vector jump(0.0f, 0.0f, 0.0f);
    SpellVisualsComputeMountTransitionJumpVec(this->m_mountTransition, &jump);
    transform.d0 = transform.d0 + jump.x;
    transform.d1 = transform.d1 + jump.y;
    transform.d2 = transform.d2 + jump.z;

    CM2Model* objectModel = this->GetObjectModel();
    objectModel->m_fFlags |= 0x8000;
    objectModel->m_worldTransform = transform;

    if (this->m_mountTransitionEffect) {
        this->m_mountTransitionSpellId = this->m_mountTransitionEffect->m_spellId;
    }
}

// VERIFY: 304cf368 auto
// OFFSET: 0x00722950
void CGUnit_C::AttachPrecastMissileModel(const SpellVisualRec* visual, uint8_t castCount) {
    CM2Model* precast = this->m_precastMissileModel;

    if (precast) {
        if (precast->m_attachParent) {
            precast->DetachFromParent();
        }

        this->m_precastMissileModel->Release();
        this->m_precastMissileModel = nullptr;
        this->m_precastCastCount = 0;
    }

    this->m_clientFlags &= ~0x1000000;

    if (!visual || visual->m_missileAttachment == -1) {
        return;
    }

    int32_t raw = visual->m_flags & 0x200;

    if (!this->GetObjectModel() || !this->GetObjectModel()->IsLoaded(0, 0) || !this->HasAttachmentForEffect(visual->m_missileAttachment, raw)) {
        return;
    }

    SpellVisualEffectNameRec* effectName = g_spellVisualEffectNameDB.GetRecord(visual->m_missileModel);

    if (!effectName || !*effectName->m_fileName) {
        return;
    }

    uint32_t attachId = raw ? visual->m_missileAttachment : s_effectAttachmentIds[visual->m_missileAttachment];
    this->m_precastMissileModel = World::s_m2Scene->CreateModel(effectName->m_fileName, 64);
    C44Matrix world = this->GetObjectModel()->m_worldTransform;
    C3Vector position;
    position.x = 0.0f;
    position.y = 0.0f;
    position.z = 0.0f;
    float scale = effectName->m_scale / sqrt(world.a1 * world.a1 + world.a2 * world.a2 + world.a0 * world.a0);
    this->m_precastMissileModel->SetWorldTransform(position, -1.5707964f, scale);
    this->m_precastMissileModel->AttachToParent(this->GetObjectModel(), attachId, nullptr, 0);
    this->m_precastCastCount = castCount;
}

// VERIFY: 6c6ecf06 manual
// OFFSET: 0x00727860
bool CGUnit_C::CheckSelfResurrect() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !(this->m_auraEffectMask[314 / 8] & (1 << (314 % 8)))) {
        return true;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(player->m_player->m_playerSelfResSpell, &spell) && (spell.m_attributesExG & 0x8000000)) {
        return true;
    }

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        if (!g_spellDB.GetRecord(this->m_auras[i].m_spellId, &spell)) {
            continue;
        }

        for (uint32_t effect = 0; effect < 3; effect++) {
            if (spell.m_effectAura[effect] == SPELL_AURA_PREVENT_RESURRECTION) {
                char buffer[1024];
                GetCustomSpellFailedString(buffer, sizeof(buffer), spell.m_effectMiscValue[effect]);
                return false;
            }
        }
    }

    return true;
}

// VERIFY: 6e009ea7 manual
// OFFSET: 0x00728880
void CGUnit_C::CheckApplyPeriodicClientTriggers() {
    SpellRec spell;

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        if (!g_spellDB.GetRecord(this->m_auras[i].m_spellId, &spell)) {
            continue;
        }

        for (uint32_t effect = 0; effect < 3; effect++) {
            if (spell.m_effectAura[effect] == SPELL_AURA_PERIODIC_TRIGGER_SPELL_FROM_CLIENT) {
                int32_t period = spell.m_effectAuraPeriod[effect];
                Spell_C_ModifySpellValue(&spell, period, 19);
                Spell_C_AddPeriodicClientTrigger(this->m_obj->m_guid, spell.m_id, spell.m_effectTriggerSpell[effect], period);
            }
        }
    }
}

// VERIFY: d5357500 manual
// OFFSET: 0x0072A560
void CGUnit_C::HandleScaleUpdate(float scale) {
    int32_t growing = this->GetScale() <= scale;
    int32_t mountDisplayId = this->m_unit->m_unitFieldMountdisplayid;
    int32_t updated = 0;

    if (mountDisplayId <= 0 || !(this->m_obj->m_type & TYPE_PLAYER)) {
        updated = this->UpdateUnitCollisionBox(growing, 0);
    } else {
        CreatureDisplayInfoRec* displayRec = g_creatureDisplayInfoDB.GetRecord(mountDisplayId);

        if (displayRec) {
            CreatureModelDataRec* modelRec = g_creatureModelDataDB.GetRecord(displayRec->m_modelID);

            if (modelRec) {
                updated = static_cast<CGPlayer_C*>(this)->UpdateMountedCollisionBox(modelRec->m_mountHeight, growing);
            }
        }
    }

    if (!updated) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_CANCEL_GROWTH_AURA));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    this->UpdateInteractIconScale();
    this->UpdateSelectionRadius();
    this->UpdateEffectAttachments();
}

// VERIFY: 5e614a02 manual
// OFFSET: 0x0072A7F0
void CGUnit_C::FillPartyMemberPetState(CGPartyPetState* state) {
    state->m_guid = this->m_obj->m_guid;
    SStrCopy(state->m_name, this->GetUnitName(nullptr, 1), sizeof(state->m_name));
    state->m_displayId = this->m_unit->m_unitFieldDisplayid;
    state->m_health = this->m_unit->m_unitFieldHealth;
    state->m_maxHealth = this->m_unit->m_unitFieldMaxhealth;
    uint8_t powerType = this->m_unit->m_unitFieldBytes0[3];
    state->m_powerType = powerType;
    state->m_power = this->m_unit->m_unitFieldPower[powerType];
    state->m_maxPower = this->m_unit->m_unitFieldMaxpower[powerType];

    uint32_t count = 0;

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        AuraData* aura = this->GetAura(i);

        if (aura->m_spellId && (aura->m_flags & 0x7)) {
            state->m_auras[count] = aura->m_spellId;
            state->m_auraFlags[count] = aura->m_flags;
            count++;

            if (count == 64) {
                return;
            }
        }
    }

    memset(&state->m_auras[count], 0, sizeof(state->m_auras[0]) * (64 - count));
}

// VERIFY: 80a37933 manual
// OFFSET: 0x00730290
void CGUnit_C::OnReceiveMirrorImageDataFromServer(CDataStore* msg) {
    ComponentData data;
    this->m_clientFlags &= ~0x20000;

    if (!this->m_worldModel || !this->m_worldModel->IsLoaded(0, 0)) {
        return;
    }

    uint32_t displayId;
    msg->Get(displayId);

    if (displayId != this->m_unit->m_unitFieldDisplayid) {
        msg->m_read = msg->m_size;
        return;
    }

    uint8_t value;
    msg->Get(value);
    data.m_preferences.m_raceID = value;

    if (value) {
        msg->Get(value);
        data.m_preferences.m_sexID = value;
        msg->Get(value);
        data.m_preferences.m_classId = value;
        msg->Get(value);
        data.m_preferences.m_skinID = value;
        msg->Get(value);
        data.m_preferences.m_faceId = value;
        msg->Get(value);
        data.m_preferences.m_hairStyleId = value;
        msg->Get(value);
        data.m_preferences.m_hairColorId = value;
        msg->Get(value);
        data.m_preferences.m_facialHairId = value;

        uint32_t guildId;
        msg->Get(guildId);

        this->m_characterComponent = CCharacterComponent::AllocComponent();
        data.m_model = this->m_worldModel;
        data.m_flags ^= (data.m_flags ^ (static_cast<uint32_t>(this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) << 1)) & 0x2;
        data.m_model->m_refCount++;
        CCharacterComponent::ValidateComponentData(&data, COMPONENT_CONTEXT_1);
        this->m_characterComponent->Init(&data, nullptr);

        uint32_t itemDisplayId;

        msg->Get(itemDisplayId);

        if (itemDisplayId) {
            this->m_characterComponent->AddItemBySlot(INVSLOT_HEAD, itemDisplayId, 0);
        }

        msg->Get(itemDisplayId);

        if (itemDisplayId) {
            this->m_characterComponent->AddItemBySlot(INVSLOT_SHOULDER, itemDisplayId, 0);
        }

        msg->Get(itemDisplayId);

        if (itemDisplayId) {
            this->m_characterComponent->AddItemBySlot(INVSLOT_BODY, itemDisplayId, 0);
        }

        msg->Get(itemDisplayId);

        if (itemDisplayId) {
            this->m_characterComponent->AddItemBySlot(INVSLOT_CHEST, itemDisplayId, 0);
        }

        msg->Get(itemDisplayId);

        if (itemDisplayId) {
            this->m_characterComponent->AddItemBySlot(INVSLOT_WAIST, itemDisplayId, 0);
        }

        msg->Get(itemDisplayId);

        if (itemDisplayId) {
            this->m_characterComponent->AddItemBySlot(INVSLOT_LEGS, itemDisplayId, 0);
        }

        msg->Get(itemDisplayId);

        if (itemDisplayId) {
            this->m_characterComponent->AddItemBySlot(INVSLOT_FEET, itemDisplayId, 0);
        }

        msg->Get(itemDisplayId);

        if (itemDisplayId) {
            this->m_characterComponent->AddItemBySlot(INVSLOT_WRIST, itemDisplayId, 0);
        }

        msg->Get(itemDisplayId);

        if (itemDisplayId) {
            this->m_characterComponent->AddItemBySlot(INVSLOT_HAND, itemDisplayId, 0);
        }

        msg->Get(itemDisplayId);

        if (itemDisplayId) {
            this->m_characterComponent->AddItemBySlot(INVSLOT_BACK, itemDisplayId, 0);
        }

        msg->Get(itemDisplayId);

        if (itemDisplayId) {
            this->m_characterComponent->AddItemBySlot(INVSLOT_TABARD, itemDisplayId, 0);
            ItemDisplayInfoRec itemRec;
            int32_t emblemStyle;
            int32_t emblemColor;
            int32_t borderStyle;
            int32_t borderColor;
            int32_t background;

            if (g_itemDisplayInfoDB.GetRecord(itemDisplayId, &itemRec) && (itemRec.m_flags & 0x1) && GuildGetGuildTabard(this->m_obj->m_guid, guildId, CGUnit_C::ApplyGuildTabard, this, emblemStyle, emblemColor, borderStyle, borderColor, background)) {
                this->m_characterComponent->ApplyGuildColor(emblemStyle, emblemColor, borderStyle, borderColor, background);
            }
        }
    } else if (this->m_displayInfoExtra) {
        this->m_characterComponent = CCharacterComponent::AllocComponent();
        const char* bakeName = this->GetDisplayBakeName();

        if (!bakeName || !*bakeName) {
            return;
        }

        CreatureDisplayInfoExtraRec* extra = this->m_displayInfoExtra;
        data.m_preferences.m_raceID = extra->m_displayRaceID;
        data.m_preferences.m_sexID = extra->m_displaySexID;
        data.m_preferences.m_classId = 0;
        data.m_preferences.m_skinID = extra->m_skinID;
        data.m_preferences.m_faceId = extra->m_faceID;
        data.m_preferences.m_hairStyleId = extra->m_hairStyleID;
        data.m_preferences.m_hairColorId = extra->m_hairColorID;
        data.m_flags |= 0x1;
        data.m_preferences.m_facialHairId = extra->m_facialHairID;
        SStrPrintf(data.m_npcSkinTexture, sizeof(data.m_npcSkinTexture), "%s%s", "Textures\\BakedNpcTextures\\", bakeName);
        data.m_model = this->m_worldModel;
        data.m_model->m_refCount++;
        this->m_characterComponent->Init(&data, nullptr);

        if (this->m_characterComponent && this->m_displayInfoExtra) {
            for (uint32_t i = 0; i < 11; i++) {
                int32_t itemDisplay = this->m_displayInfoExtra->m_npcitemDisplay[i];

                if (itemDisplay) {
                    this->m_characterComponent->AddItem(static_cast<ITEM_SLOT>(i), itemDisplay, 0);
                }
            }
        }
    }

    this->AddHandItem(0);
    this->AddHandItem(1);
    this->AddHandItem(2);
    this->ApplyComponentItemsFromEffects();
    WOWGUID guid = this->m_obj->m_guid;
    CGGameUI::UnitModelUpdate(guid, 3);
    this->m_clientFlags &= ~0x400000;
}

// VERIFY: 3b5e7768 auto
// OFFSET: 0x00732400
CM2Model* CGUnit_C::CreateItemModel(uint32_t hand, float* scale) {
    const CGUnitVirtualItem* item = this->GetVirtualItem(static_cast<VIRTUAL_MONSTER_SLOT>(hand), true);

    if (!item) {
        return nullptr;
    }

    return this->CreateItemModel(this->GetVirtualItemDisplayID(static_cast<VIRTUAL_MONSTER_SLOT>(hand)), item->m_inventoryType, scale);
}

// VERIFY: 4a0f01e4 manual
// OFFSET: 0x007324B0
int32_t Packet_SMSG_MIRRORIMAGE_DATA(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->OnReceiveMirrorImageDataFromServer(msg);
    }

    return 1;
}

// VERIFY: 999144a4 auto
// OFFSET: 0x0073C5D0
void CGUnit_C::OnFlag2Changed(uint32_t oldFlags) {
    uint32_t flags = this->m_unit->m_unitFieldFlags2;
    uint32_t changed = oldFlags ^ flags;

    if (changed & 0x1) {
        if (flags & 0x1) {
            if (this->m_obj->m_guid == CGUnit_C::s_activeMover) {
                CGInputControl::GetActive()->UpdatePlayer(OsGetAsyncTimeMs(), 0);
            }

            this->CheckLoopSound();
            this->OnAiReaction(4);
            this->PlayDeathAnimation(1);
        } else {
            this->CheckLoopSound();
            this->UpdateBaseAnimation(0, -1);
        }
    }

    if ((changed & 0x8) && this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CGChat::UpdateLanguages();
    }

    if ((changed & 0x10) && !(flags & 0x10)) {
        this->UpdateDisplayInfo(1);
    }

    if (changed & 0x80) {
        const CGUnitVirtualItem* offHand = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, true);

        if (offHand) {
            bool hide = false;

            if ((this->m_unit->m_unitFieldFlags & 0x200000)) {
                this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, true);

                if (!this->IsDisarmed(0)) {
                    const CGUnitVirtualItem* item = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, true);

                    if (item && item->m_classID == 2) {
                        hide = true;
                    }
                }
            }

            if (!hide && (this->m_unit->m_unitFieldFlags2 & 0x80)) {
                hide = true;
            }

            if (hide && this->m_sheatheState <= WEAPONMODE_NORMALMODE) {
                CCharacterComponent::RemoveHandItem(this->m_worldModel, INVSLOT_OFFHAND, static_cast<SHEATHE_TYPE>(this->m_sheatheState ? 0 : offHand->m_sheatheType), offHand->m_inventoryType == 14);
            } else {
                this->AddHandItem(1);
            }
        }

        if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            CGSpellBook::UpdateUsable();
        }
    }

    if (changed & 0x400) {
        if ((this->m_unit->m_unitFieldFlags2 & 0x400) && this->m_sheatheState == WEAPONMODE_RANGEDMODE) {
            const CGUnitVirtualItem* ranged = this->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, true);

            if (ranged) {
                if (ranged->m_inventoryType == 26 || ranged->m_inventoryType == 25) {
                    this->m_worldModel->DetachAllChildrenById(1);
                } else {
                    this->m_worldModel->DetachAllChildrenById(2);
                }
            }
        } else {
            this->AddHandItem(2);
        }

        if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            CGSpellBook::UpdateUsable();
        }
    }

    if ((changed & 0x40) && this->m_obj->m_guid == CGUnit_C::s_activeMover) {
        CGInputControl::GetActive()->UpdatePlayer(OsGetAsyncTimeMs(), 1);
    }
}

// VERIFY: b98d7092 auto
// OFFSET: 0x00754ED0
void CGUnit_C::ShowPlayerPVPHonorGained(WOWGUID victim, int32_t rank, int32_t honor) {
    CGChat::LogHonorGain(this, victim, rank, honor);
}

// VERIFY: 3204fedf manual
// OFFSET: 0x0074B830
void CGUnit_C::CleanUpVehicleBoneAnimsBeforeObjectModelChange() {
    CVehicle_C* vehicle = this->m_vehicle;

    if (!vehicle || !(vehicle->m_flags & 0x1)) {
        return;
    }

    CM2Model* model = vehicle->m_unit->GetObjectModel();

    if (!model) {
        return;
    }

    if (model->IsLoaded(0, 0)) {
        for (uint32_t bone = 0; bone < 35; bone++) {
            if (vehicle->m_playingBoneAnims[bone >> 5] & (1 << (bone & 0x1F))) {
                vehicle->PlayBoneAnimOrFallback(model, bone);
            }
        }
    }

    vehicle->m_playingBoneAnims[0] = 0;
    vehicle->m_playingBoneAnims[1] = 0;

    for (uint32_t i = 0; i < 16; i++) {
        vehicle->m_boneAnimListeners[i].m_guid = WOWGUID();
    }

    vehicle->m_flags &= ~0x1;
    vehicle->m_forceFacingGuid = WOWGUID();
}
