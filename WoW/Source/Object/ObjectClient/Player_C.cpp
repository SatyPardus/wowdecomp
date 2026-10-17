#include "Player_C.h"
#include <malloc.h>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ChrRacesRec.h"
#include "WoW/Source/DB/Rec/CreatureDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/CreatureModelDataRec.h"
#include "WoW/Source/DB/Rec/DanceMovesRec.h"
#include "WoW/Source/DB/Rec/FactionTemplateRec.h"
#include "WoW/Source/DB/Rec/GemPropertiesRec.h"
#include "WoW/Source/DB/Rec/GlyphPropertiesRec.h"
#include "WoW/Source/DB/Rec/ItemBagFamilyRec.h"
#include "WoW/Source/DB/Rec/ItemRec.h"
#include "WoW/Source/DB/Rec/ItemSetRec.h"
#include "WoW/Source/DB/Rec/ItemSubClassRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/SpellCastTimesRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/SpellVisualKitRec.h"
#include "WoW/Source/DB/Rec/SpellVisualRec.h"
#include "WoW/Source/DB/Rec/TeamContributionPointsRec.h"
#include "WoW/Source/DB/Rec/VehicleRec.h"
#include "WoW/Source/DB/Rec/VehicleSeatRec.h"
#include "WoW/Source/Game/GameClient/CDrawBowString.h"
#include "WoW/Source/Game/GameClient/ComSatClient.h"
#include "WoW/Source/LoadingScreen.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/CGPetition.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Effect_C.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/UnitCombatLog_C.h"
#include "WoW/Source/Object/ObjectClient/UnitSound_C.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/Object/ObjectClient/VisibleItem_C.h"
#include "WoW/Source/Object/QuestCache.h"
#include "WoW/Source/Object/Types.h"
#include "WoW/Source/ObjectMgrClient/CClientObjCreate.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/BarberShop.h"
#include "WoW/Source/Ui/BattlefieldInfo.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/ClassTrainerFrame.h"
#include "WoW/Source/Ui/CommentatorFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/GossipInfo.h"
#include "WoW/Source/Ui/GuildBankFrame.h"
#include "WoW/Source/Ui/GuildInfo.h"
#include "WoW/Source/Ui/ItemTextFrame.h"
#include "WoW/Source/Ui/LFGInfo.h"
#include "WoW/Source/Ui/LootFrame.h"
#include "WoW/Source/Ui/MerchantFrame.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/Ui/PetitionInfo.h"
#include "WoW/Source/Ui/QuestFrame.h"
#include "WoW/Source/Ui/QuestLog.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/RuneInfo.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/TalentInfo.h"
#include "WoW/Source/Ui/Tutorial.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/UIUtil/Cursor.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include <new>
#include <ctime>
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/LockRec.h"
#include "WoW/Source/DB/Rec/SpellItemEnchantmentRec.h"
#include "WoW/Source/Object/GameObject.h"
#include "WoW/Source/Ui/CGTabardCreationFrame.h"
#include "WoW/Source/WorldClient/AreaList.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/FriendList.h"
#include <malloc.h>
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/DB/Rec/ItemLimitCategoryRec.h"
#include "WoW/Source/DB/Rec/SpellEffectCameraShakesRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Magic/MagicClient/SpellTable.h"
#include "WoW/Source/Object/Item.h"
#include "WoW/Source/Object/ObjectClient/AuraData.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Container_C.h"
#include "WoW/Source/Object/ObjectClient/Corpse_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/Ui/Calendar.h"
#include "WoW/Source/Ui/CGAutoCompleteName.h"
#include "WoW/Source/Ui/CGBankInfo.h"
#include "WoW/Source/Ui/ContainerFrame.h"
#include "WoW/Source/Ui/GossipInfo.h"
#include "WoW/Source/Ui/MinimapFrame.h"
#include "WoW/Source/Ui/PaperDollInfoFrame.h"
#include "WoW/Source/Ui/ReputationInfo.h"
#include "WoW/Source/Ui/RuneInfo.h"
#include "WoW/Source/Ui/TradeSkillFrame.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include <cstdlib>
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/Rec/SpellShapeshiftFormRec.h"
#include "WoW/Source/Object/ObjectClient/GuildStats_C.h"
#include "WoW/Source/Object/ObjectClient/VisibleItem_C.h"
#include "WoW/Source/Ui/TradeFrame.h"
#include <cmath>
#include "WoW/Source/DB/Rec/ItemDisplayInfoRec.h"
#include "WoW/Source/Game/GameClient/CDrawBowString.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include <cstdio>
#include <cstring>
#include "WoW/Source/DB/Rec/EmotesRec.h"
#include "WoW/Source/DB/Rec/EmotesTextRec.h"
#include "WoW/Source/DB/Rec/TaxiNodesRec.h"
#include "WoW/Source/Game/GameClient/PlayerName.h"
#include "WoW/Source/Object/ObjectClient/LootRoll.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/Ui/ArenaTeamInfo.h"
#include "WoW/Source/Ui/GuildRegistrar.h"
#include "WoW/Source/Ui/PetitionInfo.h"
#include "WoW/Source/Ui/PetitionVendor.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/TaxiMapFrame.h"
#include "WoW/Source/Ui/Tutorial.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/PortraitButton.h"

struct GUILDUPDATETIMESTAMP {
    /* 0000 */ uint32_t m_guildId;
    /* 0004 */ uint32_t m_timestamp;
};

struct InitialSpellStruct {
    /* 0000 */ uint32_t m_spellId;
    /* 0004 */ uint16_t m_slot;
    /* 0006 */ uint16_t unused_0006;
};

struct QuestItemLoot {
    uint32_t m_itemId;
    uint32_t m_count;
    uint32_t m_totalCount;
};

struct RestStateInfo {
    UIErrors m_error;
    uint32_t m_tutorial;
};

// STATIC: 0x00C9EB28
int32_t CGPlayer_C::s_turnInArenaCharter;

// STATIC: 0x00AD9C94
static const UIErrors s_taxiReplyErrors[13] = {
    ERR_MAX_VALUE,
    ERR_TAXIUNSPECIFIEDSERVERERROR,
    ERR_TAXINOSUCHPATH,
    ERR_TAXINOTENOUGHMONEY,
    ERR_TAXITOOFARAWAY,
    ERR_TAXINOVENDORNEARBY,
    ERR_TAXINOTVISITED,
    ERR_TAXIPLAYERBUSY,
    ERR_TAXIPLAYERALREADYMOUNTED,
    ERR_TAXIPLAYERSHAPESHIFTED,
    ERR_TAXIPLAYERMOVING,
    ERR_TAXISAMENODE,
    ERR_TAXINOTSTANDING,
};

// STATIC: 0x00B6B87C
uint32_t CGPlayer_C::s_displayId;

// STATIC: 0x00C9D328
static int32_t s_areaTriggerEnd;

// STATIC: 0x00C9D32C
static int32_t s_areaTriggerStart;

// STATIC: 0x00C9D330
static AreaTriggerRec* s_currentAreaTrigger;

// STATIC: 0x00C9D334
static uint32_t s_areaTriggerTimer;

// STATIC: 0x00C9D338
static int32_t s_areaTriggerMapID;

// STATIC: 0x00C9D33C
static int32_t s_defenseSpellId;

// STATIC: 0x00C9D4F0
static uint32_t s_proficiency[17];

// STATIC: 0x00C9D534
static uint32_t s_bindAreaId;

// STATIC: 0x00C9D538
static uint32_t s_bindMapId;

// STATIC: 0x00C9D53C
static int32_t s_bindPointSet;

// STATIC: 0x00C9D540
static int32_t s_playerRenderDisabled;

// STATIC: 0x00C9D544
static uint32_t s_attackSwingErrorTime;

// STATIC: 0x00C9EA90
WOWGUID CGPlayer_C::s_lastVendorListReceived;

// STATIC: 0x00C9EAA0
static WOWGUID s_lastBinderGuid;

// STATIC: 0x00C9EAA8
static int32_t s_attackSwingError;

// STATIC: 0x00C9EAAC
static uint32_t s_pendingMovie;

// STATIC: 0x00C9EAB8
WOWGUID CGPlayer_C::s_resurrectOfferer;

// STATIC: 0x00C9EAB0
static uint32_t s_visibleAuraEffectDisplay;

// STATIC: 0x00C9EAC0
uint32_t CGPlayer_C::s_resurrectOfferSickness;

// STATIC: 0x00C9EAC4
uint32_t CGPlayer_C::s_resurrectOfferUseTimer;

// STATIC: 0x00C9EAB4
static uint32_t s_visibleAuraEffectFlags;

// STATIC: 0x00C9EACC
static uint8_t s_tabardSavePending;

// STATIC: 0x00ADA2C8
static const UIErrors s_guildEmblemErrors[6] = { ERR_GUILDEMBLEM_SUCCESS, ERR_GUILDEMBLEM_INVALID_TABARD_COLORS, ERR_GUILDEMBLEM_NOGUILD, ERR_GUILDEMBLEM_NOTGUILDMASTER, ERR_GUILDEMBLEM_NOTENOUGHMONEY, ERR_MAX_VALUE };

// STATIC: 0x00C9EAD0
static WOWGUID s_giftWrapItem;

// STATIC: 0x00C9EAE0
static InspectData* s_inspectData;

// STATIC: 0x00C9EC78
static InspectItem s_emptyInspectItem;

// STATIC: 0x00C9EAE4
uint32_t CGPlayer_C::s_lastUpdatePVPSequence;

// STATIC: 0x00C9EAE8
WOWGUID CGPlayer_C::s_lastUpdatePVPGroup;

// STATIC: 0x00C9EAF0
uint32_t CGPlayer_C::s_lastUpdatePVPTime;

// STATIC: 0x00C9EAF4
uint32_t CGPlayer_C::s_lastUpdateSequence;

// STATIC: 0x00C9EAF8
WOWGUID CGPlayer_C::s_lastUpdateGroup;

// STATIC: 0x00C9EB00
uint32_t CGPlayer_C::s_lastUpdateTime;

// STATIC: 0x00C9EAD8
static WOWGUID s_lastTrainerRequest;

// STATIC: 0x00C9D7D0
static MerchantItem s_vendorItems[150];

// STATIC: 0x00C9EB08
static int32_t s_questsAccepted;

// STATIC: 0x00C9D548
static WOWGUID s_questListNpc;

// STATIC: 0x00C9D750
static uint32_t s_questListIds[32];

// STATIC: 0x00C9D6D0
static uint32_t s_questListStatus[32];

// STATIC: 0x00C9D650
static uint32_t s_questListLevels[32];

// STATIC: 0x00C9D5D0
static uint32_t s_questListFlags[32];

// STATIC: 0x00C9D550
static uint32_t s_questListRepeatable[32];

// STATIC: 0x00C9EB04
static uint32_t s_questFailedReason;

// STATIC: 0x00C9EB10
static WOWGUID s_spiritHealer;

// STATIC: 0x00C9EB18
static WOWGUID s_talentMaster;

// STATIC: 0x00C9EB20
static WOWGUID s_binder;

// STATIC: 0x00C9EB2C
static C3Vector s_bindPosition;

// STATIC: 0x00AD9F6C
static int32_t s_actionButtons[144];

// STATIC: 0x00C9EB38
static TSGrowableArray<InitialSpellStruct> s_initialSpells;

// STATIC: 0x00C9EB48
static TSFixedArray<uint32_t> s_weaponSkills;

// STATIC: 0x00C9EB54
static TSGrowableArray<GUILDUPDATETIMESTAMP> s_guildUpdateTimestamps;

// STATIC: 0x00C9EB64
static TSGrowableArray<ITEMSWAP> s_itemSwaps;

// STATIC: 0x00A31BB8
static const int32_t s_trivialLevelDifference[20] = {
    4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

// STATIC: 0x00C9ECC0
static uint32_t s_combatModeExitCount;

// STATIC: 0x00C9EAC8
static int32_t s_dualWieldSpell;

// STATIC: 0x00D3F4EC
static uint32_t s_missileTrajectorySequence;

// STATIC: 0x00A32450
static const RestStateInfo s_restStateInfo[3] = {
    { ERR_MAX_VALUE, 60 },
    { ERR_EXHAUSTION_RESTED, 25 },
    { ERR_EXHAUSTION_NORMAL, 60 },
};

// STATIC: 0x00C9EC68
static int32_t s_zoneUpdateCounter;

// STATIC: 0x00C9EC6C
static bool s_dailyQuestsUntouched;

// VERIFY: a84630a0 auto
// OFFSET: 0x006CDD80
static int32_t OnUpdatePVPTitle(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGGameUI::UnitNameUpdate(guid);

    return 1;
}

// VERIFY: 6e2cb893 manual
// OFFSET: 0x006D1970
static int32_t PetChangeHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    if (guid == ClntObjMgrGetActivePlayer() || CGGameUI::IsPartyMember(guid)) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

        if (!player) {
            SErrDisplayAppFatalMessage("", "", 0);
        }

        for (int32_t i = 0; i < 2; i++) {
            WOWGUID oldGuid = static_cast<const WOWGUID*>(oldValue)[i];
            WOWGUID newGuid = i ? player->m_unit->m_unitFieldSummon : player->m_unit->m_unitFieldCharm;

            if (oldGuid) {
                CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(oldGuid, TYPEMASK_UNIT, __FILE__, __LINE__));

                if (unit) {
                    unit->UpdateScriptRegistration();
                } else if (oldGuid != newGuid) {
                    CGGameUI::ClearTarget(oldGuid, 1);

                    if (CGGameUI::m_focusTarget == oldGuid) {
                        WOWGUID focus = WOWGUID();
                        CGGameUI::SetFocusTarget(focus);
                    }
                }
            }

            if (newGuid) {
                CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(newGuid, TYPEMASK_UNIT, __FILE__, __LINE__));

                if (unit) {
                    unit->UpdateScriptRegistration();
                }
            }
        }

        if (guid == ClntObjMgrGetActivePlayer()) {
            CGPetInfo::FindPossessedSpell();
        }
    }

    return 1;
}

// VERIFY: 0af6d015 auto
// OFFSET: 0x006D1AE0
static int32_t GuildRankUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    if (guid == ClntObjMgrGetActivePlayer()) {
        CGGuildInfo::QueryPermissions();
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_GUILD_UPDATE, nullptr);

    return 1;
}

// VERIFY: c6201d41 manual
// OFFSET: 0x006D1B20
static int32_t DuelTeamUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->UpdateReaction(true);
    }

    return 1;
}

// VERIFY: 02a74688 auto
// OFFSET: 0x006D3010
static int32_t GetWeaponSubClassCount(int32_t* count) {
    if (!count) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    int32_t maxSubClass = 0;
    int32_t found = 0;
    int32_t index = g_itemSubClassDB.m_numRecords;

    while (index--) {
        const ItemSubClassRec* rec = index >= 0 && index < g_itemSubClassDB.m_numRecords ? &g_itemSubClassDB.m_records[index] : nullptr;

        if (rec->m_classID == 2) {
            found = 1;

            if (maxSubClass <= rec->m_subClassID) {
                maxSubClass = rec->m_subClassID;
            }
        }
    }

    *count = maxSubClass + 1;

    return found;
}

// VERIFY: d6521b3e manual
// OFFSET: 0x006DA6B0
static int32_t OnUpdateInebriation(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        float drunkness = player->GetDrunknessTurnValue();

        if (player->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGGameUI::m_lockedTarget, TYPEMASK_UNIT, __FILE__, __LINE__));

            if (target && player->CanAttack(target)) {
                FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_LEVEL, "%s", "target");
            }

            CGUnit_C::NamePlateUpdateLevels(true);
            CGWorldFrame::GetActiveCamera()->UpdateInebriation(drunkness, 0);
        }
    }

    return 1;
}

// VERIFY: 3507ab43 manual
// OFFSET: 0x006DA7F0
static int32_t OnUpdatePlayerHairColor(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        if (player->m_characterComponent) {
            player->m_characterComponent->SetHairColor(player->m_player->m_playerBytes[3], true, nullptr);
        }

        if (ClntObjMgrGetActivePlayer() == guid) {
            CGBarberShop::AppearanceApplied();
            return 1;
        }

        CGGameUI::UnitModelUpdate(guid, 3);
    }

    return 1;
}

// VERIFY: f44b3c3d manual
// OFFSET: 0x006DA870
static int32_t OnUpdatePlayerSkinID(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        if (player->m_characterComponent) {
            player->m_characterComponent->SetSkinColor(player->m_player->m_playerBytes[0], true, false, nullptr);
        }

        if (ClntObjMgrGetActivePlayer() == guid) {
            CGBarberShop::AppearanceApplied();
            return 1;
        }

        CGGameUI::UnitModelUpdate(guid, 3);
    }

    return 1;
}

// VERIFY: d2633809 manual
// OFFSET: 0x006DF530
static int32_t OnUpdatePlayerHairStyle(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        bool activePlayer = ClntObjMgrGetActivePlayer() == guid;
        CCharacterComponent* component = player->m_characterComponent;

        if (component) {
            component->SetHairStyle(player->m_player->m_playerBytes[2], nullptr);

            if (!(player->m_player->m_playerFlags & 0x400) && (!activePlayer || !CGBarberShop::m_barberShopEnabled)) {
                VisibleItem_C* head = &player->m_player->m_playerVisibleItems[0];

                if (head && head->GetDisplayID() && player->CanEquipItemsInThisForm()) {
                    component->AddItemBySlot(static_cast<INVENTORY_SLOTS>(0), head->GetDisplayID(), 0);
                }
            }
        }

        if (activePlayer) {
            CGBarberShop::AppearanceApplied();
            return 1;
        }

        CGGameUI::UnitModelUpdate(guid, 3);
    }

    return 1;
}

// VERIFY: 77d10d5a manual
// OFFSET: 0x006DF620
static int32_t OnUpdatePlayerFacialStyle(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        bool activePlayer = ClntObjMgrGetActivePlayer() == guid;
        CCharacterComponent* component = player->m_characterComponent;

        if (component) {
            component->SetBeardStyle(player->m_player->m_playerBytes2[0], true, nullptr);

            if (!(player->m_player->m_playerFlags & 0x400) && (!activePlayer || !CGBarberShop::m_barberShopEnabled)) {
                VisibleItem_C* head = &player->m_player->m_playerVisibleItems[0];

                if (head && head->GetDisplayID() && player->CanEquipItemsInThisForm()) {
                    component->AddItemBySlot(static_cast<INVENTORY_SLOTS>(0), head->GetDisplayID(), 0);
                }
            }
        }

        if (activePlayer) {
            CGBarberShop::AppearanceApplied();
            return 1;
        }

        CGGameUI::UnitModelUpdate(guid, 3);
    }

    return 1;
}

// VERIFY: 48d07e5a auto
// OFFSET: 0x006E0340
static void InitWeaponSkills() {
    int32_t count;

    if (!GetWeaponSubClassCount(&count)) {
        return;
    }

    s_weaponSkills.SetCount(count);

    for (uint32_t i = 0; i < static_cast<uint32_t>(count); i++) {
        s_weaponSkills.m_data[i] = 0;
    }

    s_defenseSpellId = 0;

    SpellRec rec;
    int32_t index = g_spellDB.m_numRecords;

    while (index--) {
        if (!g_spellDB.GetRecordByIndex(index, &rec)) {
            SErrDisplayAppFatalMessage("", "", 0);
        }

        if (!(rec.m_attributes & 0x40)) {
            continue;
        }

        if (!s_defenseSpellId && rec.m_effect[0] == 26) {
            s_defenseSpellId = rec.m_id;
        }

        if (rec.m_equippedItemClass != 2) {
            continue;
        }

        int32_t mask = rec.m_equippedItemSubclass;
        int32_t singleBit = 1;

        if (!mask || ((mask - 1) & mask)) {
            singleBit = 0;
        }

        int32_t subClass = 0x7FFFFFFF;

        for (int32_t bit = 0; static_cast<uint32_t>(bit) < 32; bit++) {
            if (((1 << bit) & mask) && bit < subClass) {
                subClass = bit;
            }
        }

        if (!mask) {
            subClass = -1;
        }

        if (singleBit && subClass < count) {
            s_weaponSkills.m_data[subClass] = rec.m_id;
        }
    }
}

// VERIFY: 07dc9718 manual
// OFFSET: 0x006E1B40
static int32_t GuildUpdateHandler(WOWGUID guid, void* param) {
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPEMASK_OBJECT, __FILE__, __LINE__);

    if (object && (object->m_obj->m_type & TYPEMASK_PLAYER) && static_cast<CGPlayer_C*>(object)->m_player->m_playerGuildid == static_cast<uint32_t>(reinterpret_cast<uintptr_t>(param))) {
        static_cast<CGPlayer_C*>(object)->OnGuildChanged(0);
    }

    return 1;
}

// VERIFY: 4f0aa2b8 auto
// OFFSET: 0x006E1B90
static void GuildTimestampChanged(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        ClntObjMgrEnumVisibleObjects(GuildUpdateHandler, reinterpret_cast<void*>(id));
    }
}

// VERIFY: 5687d777 manual
// OFFSET: 0x006E1BB0
static int32_t GuildIDUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->OnGuildChanged(1);

        if (guid == ClntObjMgrGetActivePlayer()) {
            CGChat::UpdateGuildStatus();
            CGGuildInfo::QueryPermissions();

            if (CGMerchantInfo::m_merchant) {
                FrameScript_SignalEvent(SCRIPT_EVENT_MERCHANT_UPDATE, nullptr);
            }

            CGGuildBankInfo::ClearAllTabs();
        }
    }

    return 1;
}

// VERIFY: cdac6d5b manual
// OFFSET: 0x006E1C20
static int32_t OnUpdatePlayerFlags(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->TogglePlayerFlag(*static_cast<const uint32_t*>(oldValue));
    }

    return 1;
}

// VERIFY: 3504720c manual
// OFFSET: 0x006E1C60
static int32_t OnUpdateGuildTimeStamp(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 1;
    }

    uint32_t timestamp = player->m_player->m_playerGuildTimestamp;
    uint32_t guildId = player->m_player->m_playerGuildid;
    int32_t index = -1;

    for (uint32_t i = 0; i < s_guildUpdateTimestamps.m_count; i++) {
        if (s_guildUpdateTimestamps.m_data[i].m_guildId == guildId) {
            if (s_guildUpdateTimestamps.m_data[i].m_timestamp == timestamp) {
                return 1;
            }

            index = i;
            break;
        }
    }

    if (index == -1) {
        index = s_guildUpdateTimestamps.m_count;
        s_guildUpdateTimestamps.SetCount(index + 1);
        s_guildUpdateTimestamps.m_data[index].m_guildId = guildId;
    }

    s_guildUpdateTimestamps.m_data[index].m_timestamp = timestamp;
    g_guildInfoCache.Invalidate(guildId);
    g_guildInfoCache.GetRecord(guildId, guid, GuildTimestampChanged, nullptr, false);

    return 1;
}

// VERIFY: bdd9f9e6 manual
// OFFSET: 0x006E4580
static int32_t OnUpdateInventoryComponent(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->UpdateInventoryComponent(static_cast<INVENTORY_SLOTS>((fieldOffset - offsetof(CGPlayerData, m_playerVisibleItems)) / sizeof(VisibleItem_C)), static_cast<const VisibleItem_C*>(oldValue));
    }

    return 1;
}

// VERIFY: e141464d auto
// OFFSET: 0x006E45D0
void CGPlayer_C::Initialize() {
    CGPlayer_C::s_lastVendorListReceived = WOWGUID();
    s_giftWrapItem = WOWGUID();
    s_bindPointSet = 0;
    s_visibleAuraEffectDisplay = 0;
    s_visibleAuraEffectFlags = 0;
    CGPlayer_C::s_lastUpdatePVPGroup = WOWGUID();
    CGPlayer_C::s_lastUpdatePVPSequence = 0;
    CGPlayer_C::s_lastUpdatePVPTime = 0;
    CGPlayer_C::s_lastUpdateGroup = WOWGUID();
    CGPlayer_C::s_lastUpdateSequence = 0;
    CGPlayer_C::s_lastUpdateTime = 0;
    InitWeaponSkills();
    s_attackSwingError = 0;
    s_attackSwingErrorTime = FrameTime::s_curTimeMs;

    for (uint32_t offset = offsetof(CGPlayerData, m_playerVisibleItems); offset <= offsetof(CGPlayerData, m_playerVisibleItems) + sizeof(CGPlayerData::m_playerVisibleItems) - sizeof(VisibleItem_C); offset += sizeof(VisibleItem_C)) {
        ClntObjMgrSetTypeMirrorHandler(ID_PLAYER, offset, sizeof(VisibleItem_C), OnUpdateInventoryComponent, nullptr, static_cast<HANDLER_PRIORITY>(1), 0);
    }

    ClntObjMgrSetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldCharm), 2 * sizeof(WOWGUID), PetChangeHandler, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerGuildid), sizeof(uint32_t), GuildIDUpdateHandler, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerGuildrank), sizeof(uint32_t), GuildRankUpdateHandler, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerDuelTeam), sizeof(uint32_t), DuelTeamUpdateHandler, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerFlags), sizeof(uint32_t), OnUpdatePlayerFlags, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerGuildTimestamp), sizeof(uint32_t), OnUpdateGuildTimeStamp, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerBytes3) + 2, sizeof(uint8_t), OnUpdatePVPTitle, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerBytes3) + 1, sizeof(uint8_t), OnUpdateInebriation, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerChosenTitle), sizeof(uint32_t), OnUpdatePVPTitle, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerBytes) + 3, sizeof(uint8_t), OnUpdatePlayerHairColor, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerBytes) + 2, sizeof(uint8_t), OnUpdatePlayerHairStyle, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerBytes2), sizeof(uint8_t), OnUpdatePlayerFacialStyle, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerBytes), sizeof(uint8_t), OnUpdatePlayerSkinID, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
}

// VERIFY: 22f04900 manual
// OFFSET: 0x006E4780
void CGPlayer_C::Shutdown() {
    s_guildUpdateTimestamps.Clear();
    s_initialSpells.Clear();
    s_itemSwaps.Clear();
    s_weaponSkills.Clear();
    s_lastBinderGuid = WOWGUID();
    s_tabardSavePending = 0;

    if (s_inspectData) {
        delete s_inspectData;
        s_inspectData = nullptr;
    }

    for (uint32_t offset = offsetof(CGPlayerData, m_playerVisibleItems); offset <= offsetof(CGPlayerData, m_playerVisibleItems) + sizeof(CGPlayerData::m_playerVisibleItems) - sizeof(VisibleItem_C); offset += sizeof(VisibleItem_C)) {
        ClntObjMgrUnsetTypeMirrorHandler(ID_PLAYER, offset, OnUpdateInventoryComponent, nullptr);
    }

    ClntObjMgrUnsetTypeMirrorHandler(ID_UNIT, offsetof(CGUnitData, m_unitFieldCharm), PetChangeHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerGuildid), GuildIDUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerGuildrank), GuildRankUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerDuelTeam), DuelTeamUpdateHandler, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerFlags), OnUpdatePlayerFlags, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerGuildTimestamp), OnUpdateGuildTimeStamp, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerBytes3) + 2, OnUpdatePVPTitle, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerBytes3) + 1, OnUpdateInebriation, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerChosenTitle), OnUpdatePVPTitle, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerBytes) + 3, OnUpdatePlayerHairColor, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerBytes) + 2, OnUpdatePlayerHairStyle, nullptr);
    ClntObjMgrUnsetTypeMirrorHandler(ID_PLAYER, offsetof(CGPlayerData, m_playerBytes2), OnUpdatePlayerFacialStyle, nullptr);
}

// VERIFY: 78b02f45 auto
// OFFSET: 0x006E8EE0
void PlayerClientInitialize() {
    CGPlayer_C::PlayerInitializeMessageHandlers();

    for (uint32_t i = 0; i < 17; i++) {
        s_proficiency[i] = 0;
    }

    CGPlayer_C::s_resurrectOfferer = WOWGUID{ 0 };
    CGPlayer_C::s_resurrectOfferSickness = 0;
    CGPlayer_C::s_resurrectOfferUseTimer = 0;
    s_playerRenderDisabled = 1;
    PlayerInitializeSounds();
    AreaTriggersInitialize(ClntObjMgrGetMapID());
}

// VERIFY: f3fc01f5 auto
// OFFSET: 0x005EEB70
void PlayerInitializeSounds() {
}

// VERIFY: 9cc59dbb auto
// OFFSET: 0x006D5080
void CGPlayer_C::UpdateQuestStatus(CGObject_C* object) {
    if (!object) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    WOWGUID guid = object->m_obj->m_guid;
    CGPlayer_C::QueryQuestGiverStatus(guid);
}

// VERIFY: c64646b5 manual
// OFFSET: 0x006D5130
void CGPlayer_C::UpdateTaxiStatus(CGUnit_C* unit) {
    if (unit->UnitReaction(this) <= 1 || !(unit->m_unit->m_unitNpcFlags & 0x2000)) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_TAXI_NODE_STATUS_QUERY));
    msg.Put(unit->m_obj->m_guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: faef9760 auto
// OFFSET: 0x006D2900
void CGPlayer_C::TurnOnGuildTabardPurchase() {
    if (this->m_characterComponent) {
        this->m_characterComponent->m_flags |= 0x44;
        this->m_characterComponent->RenderPrep(1);
    }

    WOWGUID guid = this->m_obj->m_guid;
    CGGameUI::UnitModelUpdate(guid, 3);
}

// VERIFY: ed82bd8b manual
// OFFSET: 0x006D2950
void CGPlayer_C::HandleRepopRequest(bool a2) {
    if (!this->IsDeadOrGhost() || !this->CheckSelfResurrect()) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_REPOP_REQUEST));
    msg.Put(static_cast<uint8_t>(a2 != 0));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 63c9a7c3 manual
// OFFSET: 0x006DCB40
void CGPlayer_C::ChangeStandState(uint32_t standState) {
    if (standState >= 11) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return;
    }

    if (this->m_mountDisplayId > 0 && !(this->m_clientFlags & 0x10000000)) {
        return;
    }

    CGUnitData* unit = this->m_unit;

    if ((unit->m_unitFieldFlags & 0x100000) || unit->m_unitFieldHealth <= 0 || (unit->m_unitFieldFlags2 & 0x1) || this->IsInVehicle() || (this->m_animationState & 0x400) || this->m_castSpellId || !this->IsLocalClientControlled()) {
        return;
    }

    if (this->m_unit->m_unitChannelSpell) {
        SpellRec spellRec;
        g_spellDB.GetRecord(this->m_unit->m_unitChannelSpell, &spellRec);

        if (!(spellRec.m_attributes & 0x8000000)) {
            return;
        }
    }

    uint32_t moveFlags = this->m_movement->m_flags;

    if ((moveFlags & 0x30) && (standState == 3 || standState == 1)) {
        return;
    }

    if (standState && (moveFlags & 0x2E0100F)) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_STAND_STATE_CHANGE));
    msg.Put(standState);
    msg.Finalize();
    ClientServices::Send2(&msg);
    this->UpdateClientStandState(standState);
}

// VERIFY: b464ce44 auto
// OFFSET: 0x006E2B30
void CGPlayer_C::SetClientStandState(uint32_t standState) {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (this->m_clientStandState == standState) {
        return;
    }

    this->m_clientStandState = standState;

    if (!standState) {
        CGInputControl::GetActive()->UpdatePlayer(OsGetAsyncTimeMs(), 1);
    } else {
        if (this->IsLooting()) {
            CGGameUI::CloseLoot(true, true, false);
        }

        if (this->CGUnit_C::IsInMelee()) {
            this->SetCombatMode();
        }

        if (standState >= 4 && standState <= 6) {
            return;
        }
    }

    if (CGBarberShop::m_barberShopEnabled) {
        CGBarberShop::DisableBarberShop();
    }
}

// VERIFY: c0e35919 manual
// OFFSET: 0x006D5EB0
void CGPlayer_C::PreAnimate(CGWorldFrame* frame) {
    if (!frame) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    this->CGUnit_C::PreAnimate(frame);

    uint32_t time = FrameTime::s_curTimeMs;

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && this->m_combat.m_attacking.m_guid && static_cast<int32_t>(time - s_attackSwingErrorTime) >= 0) {
        if (s_attackSwingError == 1) {
            s_attackSwingErrorTime = time + 4000;
            CGGameUI::DisplayError(ERR_BADATTACKPOS);
        } else if (s_attackSwingError == 2) {
            s_attackSwingErrorTime = time + 4000;
            CGGameUI::DisplayError(ERR_BADATTACKFACING);
        }
    }
}

// VERIFY: a6c965e0 manual
// OFFSET: 0x006E0840
void CGPlayer_C::ShouldRender(uint32_t flags, int32_t& culled, int32_t& hidden) {
    this->CGUnit_C::ShouldRender(flags, culled, hidden);

    if (this->IsCommentatorUberOrInArena() || ((this->m_clientFlags & 0x400000) && this->m_pendingItemCacheCount)) {
        hidden = 1;
    } else if (!s_playerRenderDisabled && this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        hidden = 1;
    }
}

// VERIFY: 486d0703 auto
// OFFSET: 0x006D5C30
uint32_t GetDisplayIdFromRaceAndGender(uint32_t race, uint32_t gender) {
    if (gender >= 3) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ChrRacesRec* raceRec = g_chrRacesDB.GetRecord(race);

    if (!raceRec) {
        return 0;
    }

    switch (gender) {
        case 0: {
            return raceRec->m_maleDisplayID;
        }

        case 1: {
            return raceRec->m_femaleDisplayID;
        }

        default: {
            return 0;
        }
    }
}

// VERIFY: 24f1b2a8 manual
// OFFSET: 0x006DC810
CreatureModelDataRec* GetModelDataRec(uint32_t race, uint32_t gender) {
    if (gender >= 3) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    uint32_t displayID = GetDisplayIdFromRaceAndGender(race, gender);

    if (!displayID) {
        return nullptr;
    }

    CreatureDisplayInfoRec* displayRec = g_creatureDisplayInfoDB.GetRecord(displayID);

    if (!displayRec) {
        DebugLog("Error, unknown displayInfo %d specified for player race %d sex %d!", displayID, race, gender);
    }

    CreatureModelDataRec* modelRec = g_creatureModelDataDB.GetRecord(displayRec->m_modelID);

    if (!modelRec) {
        DebugLog("Error, unknown model record %d specified for player race %d sex %d!", displayRec->m_modelID, race, gender);
    }

    return modelRec;
}

// VERIFY: e9fe3eb9 manual
// OFFSET: 0x006D7AA0
bool CGPlayer_C::VehicleAllowsInteraction() const {
    if (!this->HasVehicleTransport()) {
        return true;
    }

    CGUnit_C* transport = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_movementData.m_transportGuid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!transport) {
        return true;
    }

    if (!transport->m_vehicle) {
        return false;
    }

    VehicleSeatRec* seat = transport->m_vehicle->GetVehicleSeatRec(static_cast<uint8_t>(this->m_movementData.m_transportSeat));
    return seat && (seat->m_flags & 0x80000000);
}

// VERIFY: eb153e9f auto
// OFFSET: 0x006CDE50
void CGPlayer_C::SetAutoLoot(int32_t autoLoot) {
    this->m_autoLoot = autoLoot != 0;

    if (autoLoot && this->m_lootTarget.m_guid) {
        CGLootInfo::DoAutoLoot(0);
    }
}

// VERIFY: 21ce503c manual
// OFFSET: 0x00729530
bool CGPlayer_C::CanInteract(const CGUnit_C* unit) const {
    bool isPlayer = (this->m_obj->m_type & TYPEMASK_PLAYER) != 0;

    if (isPlayer && (this->m_player->m_playerFlags & 0x10) && (!unit->m_creatureCacheEntry || !(unit->m_creatureCacheEntry->m_typeFlags & 0x2))) {
        return false;
    }

    CGUnitData* data = unit->m_unit;
    uint32_t npcFlags = data->m_unitNpcFlags;

    if ((npcFlags & 0x1) && isPlayer && !(unit->m_obj->m_type & TYPEMASK_PLAYER) && unit->m_creatureCacheEntry && (unit->m_creatureCacheEntry->m_typeFlags & 0x800000) && data->m_unitFieldCreatedby != this->m_obj->m_guid) {
        return false;
    }

    return !(data->m_unitFieldFlags & 0x2000000) && npcFlags && ((unit->UnitReaction(this) >= 3 && this->UnitReaction(unit) >= 3) || (unit->m_unit->m_unitFieldFlags2 & 0x4000));
}

// VERIFY: c3ce918a manual
// OFFSET: 0x004F7310
uint64_t CGPlayer_C::GetFarSightGuid() const {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    return this->m_player->m_playerFarsight.m_guid;
}

// VERIFY: 274a51a9 auto
// OFFSET: 0x006E4940
void CGPlayer_C::SetFarSightFocus(CGObject_C* object) {
    this->ToggleFarSight(object);
}

// VERIFY: ab2cdd89 manual
// OFFSET: 0x0051A250
uint32_t CGPlayer_C::GetSkillValue(int32_t index) const {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    uint32_t rank = this->m_player->m_playerSkillInfo[index].m_rank;

    if (rank) {
        rank += this->m_player->m_playerSkillInfo[index].m_permBonus;
    }

    return rank;
}

// VERIFY: afe2eea5 auto
// OFFSET: 0x0051A2B0
uint32_t CGPlayer_C::GetCoinage() const {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    return this->m_player->m_playerFieldCoinage;
}

// VERIFY: cd4355a9 manual
// OFFSET: 0x0052E6D0
const char* CGPlayer_C::GetHairCustomizationName() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        ChrRacesRec* rec = g_chrRacesDB.GetRecord(player->m_unit->m_unitFieldBytes0[0]);

        if (rec) {
            return rec->m_hairCustomization;
        }
    }

    return "NORMAL";
}

// VERIFY: 08168323 manual
// OFFSET: 0x0052E730
const char* CGPlayer_C::GetFacialHairCustomizationName() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        ChrRacesRec* rec = g_chrRacesDB.GetRecord(player->m_unit->m_unitFieldBytes0[0]);

        if (rec) {
            return rec->m_facialHairCustomization[player->m_player->m_playerBytes3[0]];
        }
    }

    return "NORMAL";
}

// VERIFY: ad2d261f manual
// OFFSET: 0x00594710
uint32_t CGPlayer_C::GetSkillMaxRank(int32_t index) const {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    uint32_t rank = this->m_player->m_playerSkillInfo[index].m_maxRank;

    if (rank) {
        rank += this->m_player->m_playerSkillInfo[index].m_permBonus;
    }

    return rank;
}

// VERIFY: 637f9a95 auto
// OFFSET: 0x0060A5D0
uint32_t CGPlayer_C::GetCurrentXP() const {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    return this->m_player->m_playerXp;
}

// VERIFY: fc18c204 auto
// OFFSET: 0x006CBCF0
int32_t OnSummonCancel(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    FrameScript_SignalEvent(SCRIPT_EVENT_CANCEL_SUMMON, nullptr);

    return 1;
}

// VERIFY: 0993e05f auto
// OFFSET: 0x006CDDD0
int32_t OnUpdatePlayerScriptEvent(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    FrameScript_SignalEvent(reinterpret_cast<uintptr_t>(param), "%s", "player");

    return 1;
}

// VERIFY: 9f64b8a4 auto
// OFFSET: 0x006CDE90
uint32_t CGPlayer_C::GetProficiency(uint8_t itemClass) {
    if (itemClass >= sizeof(s_proficiency) / sizeof(s_proficiency[0])) {
        return 0;
    }

    return s_proficiency[itemClass];
}

// VERIFY: 340d9418 auto
// OFFSET: 0x006CDEB0
int32_t Packet_SMSG_SET_PROFICIENCY(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t itemClass;
    uint32_t mask;
    msg->Get(itemClass);
    msg->Get(mask);
    s_proficiency[itemClass] = mask;
    ConsolePrintf("Proficiency in item class %d set to %08x", itemClass, mask);

    return 1;
}

// VERIFY: dfb6fe0a manual
// OFFSET: 0x006CEB00
bool CGPlayer_C::CanDualWield() {
    return s_dualWieldSpell != 0;
}

// VERIFY: 3a070edb manual
// OFFSET: 0x006D5A60
int32_t CGPlayer_C::CanLoot(const CGObject_C* object) {
    if (!object) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    bool autoInteract = this->CanAutoInteract();
    float rangeSq = 0.0f;
    bool checkRange = !autoInteract;

    if (checkRange) {
        float range;

        if (object->m_obj->m_type & TYPEMASK_UNIT) {
            range = this->GetMeleeRange(static_cast<const CGUnit_C*>(object));
        } else {
            range = 5.0f;
        }

        rangeSq = range * range;
    }

    if (this->IsShapeShifted()) {
        SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(this->GetShapeshiftForm());

        if (form && (form->m_flags & 0x100)) {
            return 0;
        }
    }

    C3Vector objectPosition = object->GetPosition();
    C3Vector position = this->GetPosition();

    if (checkRange) {
        float dx = position.x - objectPosition.x;
        float dy = position.y - objectPosition.y;
        float dz = position.z - objectPosition.z;

        if (!(dx * dx + dz * dz + dy * dy <= rangeSq)) {
            return 0;
        }
    }

    if (this->m_lootTarget == object->m_obj->m_guid || this->m_lootTarget) {
        return 0;
    }

    return !this->IsDeadOrGhost() && !(this->m_movement->m_flags & 0x1000) && !(this->m_unit->m_unitFieldFlags & 0x40000) && this->IsLocalClientControlled();
}

// VERIFY: 37ec0ce6 auto
// OFFSET: 0x006CED80
int32_t __stdcall CGPlayer_C::OnLootItemNotify(CDataStore* msg) {
    WOWGUID guid;
    uint8_t slot;
    uint8_t unk;
    uint32_t itemId;
    char name[64];
    msg->Get(guid.m_guid);
    msg->Get(slot);
    msg->Get(unk);
    msg->Get(itemId);
    msg->GetString(name, sizeof(name));

    return 1;
}

// VERIFY: dbeea1b7 auto
// OFFSET: 0x006CEE30
int32_t Player_C_TogglePlayerRender() {
    s_playerRenderDisabled = s_playerRenderDisabled == 0;

    return s_playerRenderDisabled;
}

// VERIFY: 45c68f81 auto
// OFFSET: 0x006CEEC0
void CGPlayer_C::SaveBindPoint(CDataStore* msg) {
    msg->Get(s_bindPosition.x);
    msg->Get(s_bindPosition.y);
    msg->Get(s_bindPosition.z);
    msg->Get(s_bindMapId);
    msg->Get(s_bindAreaId);
    s_bindPointSet = 1;
}

// VERIFY: 5ce7e8a8 auto
// OFFSET: 0x006CEF10
uint32_t CGPlayer_C::GetBindAreaID() {
    return s_bindAreaId;
}

// VERIFY: f0168031 auto
// OFFSET: 0x006CEF20
void __stdcall SignalPlayerAttackEvent(int32_t unused_a1) {
    FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_ATTACK, "%s", "player");
}

// VERIFY: 1b4218d9 auto
// OFFSET: 0x006CEF40
void SignalPlayerSkillStatEvents() {
    FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_ATTACK, "%s", "player");
    FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_DEFENSE, "%s", "player");
    FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_RANGED_ATTACK_POWER, "%s", "player");
}

// VERIFY: 7ca6e35b auto
// OFFSET: 0x006CEFB0
int32_t CGPlayer_C::IsGiftWrapping() {
    return static_cast<bool>(s_giftWrapItem);
}

// VERIFY: 58af1e18 auto
// OFFSET: 0x006CF240
PlaySpellVisuaKitData::PlaySpellVisuaKitData(const SpellRec* spellRec, const SpellVisualKitRec* impactVisualKit, uint32_t kitType) {
    this->m_spellRec = spellRec;
    this->m_kitType = kitType;
    this->m_impactVisualKit = impactVisualKit;
    this->m_position = nullptr;
    this->m_transportGUID = WOWGUID();
    this->m_playAnim = 1;
    this->m_harmful = 0;
    this->unused_0020 = 0;
    this->unused_0024 = -1;
    this->m_someGUID = WOWGUID();
    this->m_effectFadeTime = 0;
}

// VERIFY: 6bc8ba4b auto
// OFFSET: 0x006CF290
uint32_t CGPlayer_C::GetPendingMovie() {
    return s_pendingMovie;
}

// VERIFY: 6650af23 auto
// OFFSET: 0x006CF2D0
int32_t CGPlayer_C::GetCombatModeFlashDuration() {
    return 500;
}

// VERIFY: 14843d40 auto
// OFFSET: 0x006CF440
int32_t CGPlayer_C::GetPlayedTime() const {
    if (this->m_playedTime < 0) {
        return 0;
    }

    return this->m_playedTime + static_cast<int32_t>(time(nullptr)) - this->m_playedTimeBase;
}

// VERIFY: e7f0a743 auto
// OFFSET: 0x006CF470
void CGPlayer_C::SetPlayedTime(int32_t playedTime) {
    this->m_playedTime = playedTime;
    this->m_playedTimeBase = static_cast<uint32_t>(time(nullptr));
}

// VERIFY: 14d81fae manual
// OFFSET: 0x006D1CB0
int32_t SignalPetSpellPowerUpdate(WOWGUID guid) {
    if (ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PET_SPELL_POWER_UPDATE, nullptr);
    }

    return 1;
}

// VERIFY: e266ed2d manual
// OFFSET: 0x006D1D30
void __stdcall CGPlayer_C::AcceptResurrectRequest(int32_t accept) {
    if (!CGPlayer_C::s_resurrectOfferer) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_RESURRECT_RESPONSE));
    msg.Put(CGPlayer_C::s_resurrectOfferer.m_guid);
    msg.Put(static_cast<uint8_t>(accept));
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGPlayer_C::s_resurrectOfferer = WOWGUID();
}

// VERIFY: a7e8dc5b manual
// OFFSET: 0x006D1FC0
int32_t CGPlayer_C::CheckSpiritHealerDistance() {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(s_spiritHealer, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return 0;
    }

    C3Vector position = this->GetPosition();
    C3Vector unitPosition = unit->GetPosition();
    float dx = unitPosition.x - position.x;
    float dy = unitPosition.y - position.y;
    float dz = unitPosition.z - position.z;
    float range = unit->m_unit->m_unitFieldCombatreach + 4.0f;

    return dy * dy + dx * dx + dz * dz <= range * range;
}

// VERIFY: fdee41de manual
// OFFSET: 0x006D2070
int32_t CGPlayer_C::CheckTalentMasterDistance() {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(s_talentMaster, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return 0;
    }

    C3Vector position = this->GetPosition();
    C3Vector unitPosition = unit->GetPosition();
    float dx = unitPosition.x - position.x;
    float dy = unitPosition.y - position.y;
    float dz = unitPosition.z - position.z;
    float range = unit->m_unit->m_unitFieldCombatreach + 4.0f;

    return dy * dy + dx * dx + dz * dz <= range * range;
}

// VERIFY: 9db57599 manual
// OFFSET: 0x006D2290
int32_t CGPlayer_C::CheckBinderDistance() {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(s_binder, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return 0;
    }

    C3Vector position = this->GetPosition();
    C3Vector unitPosition = unit->GetPosition();
    float dx = unitPosition.x - position.x;
    float dy = unitPosition.y - position.y;
    float dz = unitPosition.z - position.z;
    float range = unit->m_unit->m_unitFieldCombatreach + 4.0f;

    return dy * dy + dx * dx + dz * dz <= range * range;
}

// VERIFY: 76878623 manual
// OFFSET: 0x006D2340
void __stdcall CGPlayer_C::TalkToBanker(const WOWGUID& guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BANKER_ACTIVATE));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 201bfc5e manual
// OFFSET: 0x006D2480
void __stdcall CGPlayer_C::TalkToPetitionVendor(const WOWGUID& guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PETITION_SHOW_LIST));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: c275d25e manual
// OFFSET: 0x006D25C0
void __stdcall CGPlayer_C::TalkToBattleMaster(const WOWGUID& guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BATTLEMASTER_HELLO));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: eb868649 manual
// OFFSET: 0x006D2640
void __stdcall CGPlayer_C::TalkToAuctioneer(const WOWGUID& guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_AUCTION_HELLO));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 553fae36 manual
// OFFSET: 0x006D26C0
void __stdcall CGPlayer_C::TalkToStableMaster(const WOWGUID& guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_LIST_STABLED_PETS));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: a0931e1c manual
// OFFSET: 0x006D2740
void __stdcall CGPlayer_C::TalkToSpellClick(const WOWGUID& guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SPELL_CLICK));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: d56325e8 manual
// OFFSET: 0x006D27C0
void __stdcall CGPlayer_C::TalkToRideVehicle(const WOWGUID& guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PLAYER_VEHICLE_ENTER));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 1e5377fe manual
// OFFSET: 0x006D29F0
uint8_t CGPlayer_C::FindSlotIndex(WOWGUID guid) const {
    uint32_t numSlots = this->m_bag.m_numSlots;

    if (!numSlots) {
        return 0xFF;
    }

    for (uint8_t slot = 0; slot < numSlots; slot++) {
        WOWGUID slotGuid = slot < static_cast<uint32_t>(this->m_bag.m_numSlots) ? this->m_bag.m_slots[slot] : WOWGUID();

        if (slotGuid == guid) {
            return slot;
        }
    }

    return 0xFF;
}

// VERIFY: da1b57c4 manual
// OFFSET: 0x006D2C20
void __stdcall CGPlayer_C::AutoStoreLootItem(uint8_t slot) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_AUTOSTORE_LOOT_ITEM));
    msg.Put(slot);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 5a267c7f manual
// OFFSET: 0x006D2D40
void CGPlayer_C::SellItem(WOWGUID vendor, WOWGUID item, uint32_t count) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SELL_ITEM));
    msg.Put(vendor.m_guid);
    msg.Put(item.m_guid);
    msg.Put(count);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: c8640d9d manual
// OFFSET: 0x006D4300
void CGPlayer_C::GroupInvite(const char* name, uint32_t flags) {
    CGTutorial::FlagTutorial(17);

    if (!SStrCmpUTF8I(name, this->GetUnitName(nullptr, 1), STORM_MAX_STR)) {
        CGGameUI::DisplayError(ERR_INVITE_SELF);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GROUP_INVITE));
    msg.PutString(name);
    msg.Put(flags);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 9095114f manual
// OFFSET: 0x006D43C0
void __stdcall CGPlayer_C::Uninvite(WOWGUID guid, const char* name) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GROUP_UNINVITE_GUID));
    msg.Put(guid.m_guid);
    msg.PutString(name);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: c3f83a27 manual
// OFFSET: 0x006D4450
void __stdcall CGPlayer_C::AcceptGroup(uint32_t roles) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GROUP_ACCEPT));
    msg.Put(roles);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 662f9a0a manual
// OFFSET: 0x006D44D0
void __stdcall CGPlayer_C::DeclineGroup() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GROUP_DECLINE));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 1e5ae87e manual
// OFFSET: 0x006D4540
void __stdcall CGPlayer_C::LeaveGroup() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GROUP_DISBAND));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 8d68652a manual
// OFFSET: 0x006D45B0
void __stdcall CGPlayer_C::SilencePlayer(WOWGUID guid, bool silence) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PARTY_SILENCE));
    msg.Put(guid.m_guid);
    msg.Put(static_cast<uint8_t>(silence));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 321e8245 manual
// OFFSET: 0x006D4640
void __stdcall CGPlayer_C::UnSilencePlayer(WOWGUID guid, bool silence) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PARTY_UNSILENCE));
    msg.Put(guid.m_guid);
    msg.Put(static_cast<uint8_t>(silence));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 2afbda5e manual
// OFFSET: 0x006D46D0
void __stdcall CGPlayer_C::SetLootMethod(uint32_t method, WOWGUID master, uint32_t threshold) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SET_LOOT_METHOD));
    msg.Put(method);
    msg.Put(master.m_guid);
    msg.Put(threshold);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 12aae8b7 manual
// OFFSET: 0x006D4760
void __stdcall CGPlayer_C::AcceptGuild() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_ACCEPT));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 26140604 manual
// OFFSET: 0x006D47D0
void __stdcall CGPlayer_C::DeclineGuild() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_DECLINE_INVITATION));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: d46256ad manual
// OFFSET: 0x006D4840
void __stdcall CGPlayer_C::AcceptArenaTeam() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ARENA_TEAM_ACCEPT));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 40a1eebd manual
// OFFSET: 0x006D48B0
void __stdcall CGPlayer_C::DeclineArenaTeam() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ARENA_TEAM_DECLINE));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 69a73445 auto
// OFFSET: 0x006D49C0
bool CGPlayer_C::HasEquipBypassSkill(int32_t skillId) const {
    for (uint32_t i = 0; i < this->m_equipBypassSkills.m_count; i++) {
        if (this->m_equipBypassSkills.m_data[i] == skillId) {
            return true;
        }
    }

    return false;
}

// VERIFY: c02a1585 manual
// OFFSET: 0x006D4B90
void CGPlayer_C::LootMoney() {
    if (!this->m_lootTarget) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LOOT_MONEY));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 7a72f86c manual
// OFFSET: 0x006D4C10
void __stdcall CGPlayer_C::QueryQuest(const WOWGUID& questGiver, int32_t questId) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_QUEST_GIVER_QUERY_QUEST));
    msg.Put(questGiver.m_guid);
    msg.Put(static_cast<uint32_t>(questId));
    msg.Put(static_cast<uint8_t>(1));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: c799eb8b manual
// OFFSET: 0x006D4CA0
void __stdcall CGPlayer_C::AcceptQuest(const WOWGUID& questGiver, int32_t questId, int32_t a3) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_QUEST_GIVER_ACCEPT_QUEST));
    msg.Put(questGiver.m_guid);
    msg.Put(static_cast<uint32_t>(questId));
    msg.Put(static_cast<uint32_t>(a3));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: aaae61a9 manual
// OFFSET: 0x006D4D40
void __stdcall CGPlayer_C::CompleteQuest(const WOWGUID& questGiver, int32_t questId) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_QUEST_GIVER_COMPLETE_QUEST));
    msg.Put(questGiver.m_guid);
    msg.Put(static_cast<uint32_t>(questId));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: d323253c manual
// OFFSET: 0x006D4DD0
void __stdcall CGPlayer_C::GiveQuestItems(const WOWGUID& questGiver, int32_t questId) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_QUEST_GIVER_REQUEST_REWARD));
    msg.Put(questGiver.m_guid);
    msg.Put(static_cast<uint32_t>(questId));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 922d1ead manual
// OFFSET: 0x006D4E60
void __stdcall CGPlayer_C::GetQuestReward(const WOWGUID& questGiver, int32_t questId, int32_t reward) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_QUEST_GIVER_CHOOSE_REWARD));
    msg.Put(questGiver.m_guid);
    msg.Put(static_cast<uint32_t>(questId));
    msg.Put(static_cast<uint32_t>(reward));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 3e324e1c manual
// OFFSET: 0x006D4F00
void __stdcall CGPlayer_C::PushQuestToParty(int32_t questId) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PUSH_QUEST_TO_PARTY));
    msg.Put(static_cast<uint32_t>(questId));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 2068fcba manual
// OFFSET: 0x006D4F80
void __stdcall CGPlayer_C::QuestLogRemoveQuest(uint8_t slot) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_QUEST_LOG_REMOVE_QUEST));
    msg.Put(slot);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 513fa025 manual
// OFFSET: 0x006D5000
void __stdcall CGPlayer_C::QueryQuestGiverStatus(const WOWGUID& questGiver) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_QUEST_GIVER_STATUS_QUERY));
    msg.Put(questGiver.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 8b0806a0 manual
// OFFSET: 0x006D50C0
void CGPlayer_C::QueryQuestGiverStatusMultiple() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_QUEST_GIVER_STATUS_MULTIPLE_QUERY));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: d5ef3b86 manual
// OFFSET: 0x006D5BE0
const CGUnitVirtualItem* CGPlayer_C::GetDefendingItem() const {
    const CGBag_C* bag = this->GetBag();
    WOWGUID guid = static_cast<uint32_t>(bag->m_numSlots) > 4 ? bag->m_slots[4] : WOWGUID();
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (item) {
        return &item->m_virtualItem;
    }

    return this->CGUnit_C::GetDefendingItem();
}

// VERIFY: b515a05c manual
// OFFSET: 0x006D5C90
void __stdcall CGPlayer_C::ReadItem(uint8_t bag, uint8_t slot) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_READ_ITEM));
    msg.Put(bag);
    msg.Put(slot);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 9f933d46 auto
// OFFSET: 0x006D5E70
uint32_t CGPlayer_C::GetDefaultLanguage() const {
    ChrRacesRec* rec = g_chrRacesDB.GetRecord(this->m_unit->m_unitFieldBytes0[0]);

    return rec ? rec->m_baseLanguage : 0;
}

// VERIFY: 3cb1369e manual
// OFFSET: 0x006D62A0
void __stdcall CGPlayer_C::EnableTaxiNode(const WOWGUID& guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ENABLE_TAXI_NODE));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 726c606a manual
// OFFSET: 0x006D6320
void __stdcall CGPlayer_C::QueryTaxiNodes(const WOWGUID& guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_TAXI_QUERY_AVAILABLE_NODES));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 50da28c3 manual
// OFFSET: 0x006D63A0
void CGPlayer_C::StartTaxi(WOWGUID guid, uint32_t source, uint32_t destination) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ACTIVATE_TAXI));
    msg.Put(guid.m_guid);
    msg.Put(source);
    msg.Put(destination);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: ad70ac6e manual
// OFFSET: 0x006D6430
void CGPlayer_C::StartTaxiExpress(WOWGUID guid, const uint32_t* nodes, int32_t count) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ACTIVATE_TAXI_EXPRESS));
    msg.Put(guid.m_guid);
    msg.Put(static_cast<uint32_t>(count));

    for (int32_t i = 0; i < count; i++) {
        msg.Put(nodes[i]);
    }

    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: daeba6e5 auto
// OFFSET: 0x006D64E0
uint32_t CGPlayer_C::GetClientStandState() const {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        return this->m_clientStandState;
    }

    return this->m_unit->m_unitFieldBytes1[0];
}

// VERIFY: ed967909 auto
// OFFSET: 0x006D6D20
int32_t CGPlayer_C::OnPetitionShowSignatures(CDataStore* msg) {
    WOWGUID petition;
    WOWGUID owner;
    uint32_t id;
    uint8_t count;
    msg->Get(petition.m_guid);
    msg->Get(owner.m_guid);
    msg->Get(id);
    msg->Get(count);
    WOWGUID* guids = static_cast<WOWGUID*>(alloca(sizeof(WOWGUID) * count));
    uint32_t* choices = static_cast<uint32_t*>(alloca(sizeof(uint32_t) * count));

    for (uint8_t i = 0; i < count; i++) {
        msg->Get(guids[i].m_guid);
        msg->Get(choices[i]);
    }

    if (!FriendList::s_friendList->IsIgnored(owner, true)) {
        CGPetitionInfo::SetPetition(petition, id, owner);
        CGPetitionInfo::SetSignatures(count, guids, choices);
    }

    return 1;
}

// VERIFY: 8b250d1e manual
// OFFSET: 0x006D6E00
void __stdcall CGPlayer_C::RequestPetitionSignatures(WOWGUID petition) {
    if (!petition) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PETITION_SHOW_SIGNATURES));
    msg.Put(petition.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 1f8b3e30 auto
// OFFSET: 0x006D6E90
int32_t CGPlayer_C::GetPVPFactionIndex() const {
    ChrRacesRec* raceRec = g_chrRacesDB.GetRecord(this->m_unit->m_unitFieldBytes0[0]);

    if (!raceRec) {
        return -1;
    }

    FactionTemplateRec* factionRec = g_factionTemplateDB.GetRecord(raceRec->m_factionID);

    if (!factionRec) {
        return -1;
    }

    if (factionRec->m_factionGroup & 0x4) {
        return 0;
    }

    return (factionRec->m_factionGroup & 0x2) ? 1 : -1;
}

// VERIFY: 101d568d manual
// OFFSET: 0x006D7120
void __stdcall CGPlayer_C::RemoveTempEnchantment(int32_t slot) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CANCEL_TEMP_ENCHANTMENT));
    msg.Put(static_cast<uint32_t>(slot));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 0f647542 manual
// OFFSET: 0x006D7200
void __stdcall CGPlayer_C::SetPartyAssignment(uint8_t assignment, WOWGUID guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_PARTY_ASSIGNMENT));
    msg.Put(assignment);
    msg.Put(static_cast<uint8_t>(1));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 501f6a3e manual
// OFFSET: 0x006D7290
void __stdcall CGPlayer_C::ClearPartyAssignment(uint8_t assignment, WOWGUID guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_PARTY_ASSIGNMENT));
    msg.Put(assignment);
    msg.Put(static_cast<uint8_t>(0));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 42e7c0e2 auto
// OFFSET: 0x006D7B70
uint32_t CGPlayer_C::GetCreatureTracking() const {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    return this->m_player->m_playerTrackCreatures;
}

// VERIFY: 43e0888c manual
// OFFSET: 0x006DAC10
bool CGPlayer_C::IsDeadOrGhost() const {
    return this->m_unit->m_unitFieldHealth <= 0 || ((this->m_player->m_playerFlags >> 4) & 1);
}

// VERIFY: 3028b833 auto
// OFFSET: 0x006DC1C0
int32_t CGPlayer_C::GetSkillIndex(int32_t skillId) const {
    int32_t index = 0;

    for (uint32_t i = 0; i < 128; i++) {
        uint16_t id = this->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? this->m_player->m_playerSkillInfo[i].m_skillId : 0;

        if (id == skillId) {
            break;
        }

        index++;
    }

    if (index == 128) {
        return -1;
    }

    return index;
}

// VERIFY: 43bc2c57 manual
// OFFSET: 0x006DC8D0
void CGPlayer_C::GetAFKText(char* buffer, uint32_t size) const {
    if ((this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && CGChat::m_afk) || (this->m_player->m_playerFlags & 0x2)) {
        const char* text = FrameScript_GetText("CHAT_FLAG_AFK", -1, GENDER_NOT_APPLICABLE);

        if (text && *text) {
            SStrPack(buffer, text, size);
        }
    }
}

// VERIFY: 817b7a91 manual
// OFFSET: 0x006DC940
void CGPlayer_C::GetDNDText(char* buffer, uint32_t size) const {
    if (this->m_player->m_playerFlags & 0x4) {
        const char* text = FrameScript_GetText("CHAT_FLAG_DND", -1, GENDER_NOT_APPLICABLE);

        if (text && *text) {
            SStrPack(buffer, text, size);
        }
    }
}

// VERIFY: aa26e3f6 manual
// OFFSET: 0x006DC980
void CGPlayer_C::GetGMText(char* buffer, uint32_t size) const {
    uint32_t flags = this->m_player->m_playerFlags;

    if ((flags & 0x8) && !((flags >> 15) & 1)) {
        const char* text = FrameScript_GetText("CHAT_FLAG_GM", -1, GENDER_NOT_APPLICABLE);

        if (text && *text) {
            SStrPack(buffer, text, size);
        }
    }
}

// VERIFY: 66d20b22 manual
// OFFSET: 0x006DC9D0
void CGPlayer_C::GetDevText(char* buffer, uint32_t size) const {
    if (this->m_player->m_playerFlags & 0x8000) {
        SStrPack(buffer, "<Dev>", size);
    }
}

// VERIFY: 67d51c6d manual
// OFFSET: 0x006DE130
bool CGPlayer_C::UnitIsTrivial(const CGUnit_C* unit) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player || !unit || ((unit->m_obj->m_type >> 4) & 1)) {
        return false;
    }

    int32_t level = player->m_unit->m_unitFieldLevel;
    int32_t difference = level - unit->m_unit->m_unitFieldLevel;

    if (difference <= 0) {
        return false;
    }

    if (static_cast<uint32_t>(level / 5) >= sizeof(s_trivialLevelDifference) / sizeof(s_trivialLevelDifference[0])) {
        return difference > 8;
    }

    return difference > s_trivialLevelDifference[level / 5];
}

// VERIFY: 57e71b87 manual
// OFFSET: 0x006DE330
const VisibleItem_C* CGPlayer_C::GetVisibleItem(int32_t slot) const {
    if (slot < 0 || slot >= 19) {
        return nullptr;
    }

    return &this->m_player->m_playerVisibleItems[slot];
}

// VERIFY: 4d46d50f manual
// OFFSET: 0x006DE750
void CGPlayer_C::SetPvpCooldownTimestamp(uint32_t flags) {
    uint32_t playerFlags = this->m_player->m_playerFlags;

    if (playerFlags & 0x200) {
        this->m_pvpCooldownTimestamp = 0;
    } else if (!((playerFlags >> 18) & 1) || !(flags & 0x40000)) {
        this->m_pvpCooldownTimestamp = FrameTime::s_curTimeMs + 300000;
    }
}

// VERIFY: c787d6b7 manual
// OFFSET: 0x006DE7A0
int32_t CGPlayer_C::GetPVPToggleTimer() const {
    if (!(this->m_player->m_playerFlags & 0x40000)) {
        return 300000;
    }

    uint32_t timestamp = this->m_pvpCooldownTimestamp;

    if (timestamp && static_cast<int32_t>(FrameTime::s_curTimeMs - timestamp) < 0) {
        return timestamp - FrameTime::s_curTimeMs;
    }

    return -1;
}

// VERIFY: 8ab92411 auto
// OFFSET: 0x006DE7E0
uint32_t CGPlayer_C::GetEquippedAmmoDisplayId() const {
    int32_t ammoId = this->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? this->m_player->m_playerAmmoId : 0;
    ItemRec* rec = g_itemDB.GetRecord(ammoId);

    return rec ? rec->m_displayInfoID : 0;
}

// VERIFY: f842d174 manual
// OFFSET: 0x006DE980
int32_t CGPlayer_C::IsCommentatorUberOrInArena() const {
    MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);
    uint32_t flags = this->m_player->m_playerFlags;

    return ((flags >> 19) & 1) && (((flags >> 22) & 1) || (map && map->m_instanceType == 4));
}

// VERIFY: 881f2596 manual
// OFFSET: 0x006E61B0
void CGPlayer_C::QueueItemEnchantTime(WOWGUID item, int32_t slot, int32_t duration) {
    PlayerPendingItemExpiration* expiration = this->m_pendingItemExpirations.NewNode(2, 0, 0);
    expiration->m_itemGuid = item;
    expiration->m_slot = slot;
    expiration->m_duration = duration;
}

// VERIFY: 585bb9b6 auto
// OFFSET: 0x006CEE50
int32_t Player_C_SetPlayerRenderDisabled(int32_t disabled) {
    int32_t previous = s_playerRenderDisabled;
    s_playerRenderDisabled = disabled;

    return previous;
}

// VERIFY: d4574fd6 auto
// OFFSET: 0x006D4920
int32_t CGPlayer_C::SpellUsesWeaponSkill(const SpellRec* spell) {
    if (spell->m_targets & 0x10) {
        return spell->m_id;
    }

    if ((spell->m_attributesExB & 0x8000000) || spell->m_effect[0] == 25 || spell->m_equippedItemClass != 2 || !spell->m_equippedItemSubclass) {
        return spell->m_id;
    }

    for (uint32_t slot = 0; slot < 3; slot++) {
        const CGUnitVirtualItem* item = this->GetVirtualItem(slot, false);

        if (item && item->m_classID == spell->m_equippedItemClass && ((1 << item->m_subclassID) & spell->m_equippedItemSubclass) && s_weaponSkills.m_data[item->m_subclassID]) {
            return s_weaponSkills.m_data[item->m_subclassID];
        }
    }

    return spell->m_id;
}

// VERIFY: 22b545a2 manual
// OFFSET: 0x006D5FC0
void __stdcall CGPlayer_C::OnTaxiNodeStatus(CDataStore* msg) {
    WOWGUID guid = WOWGUID();
    msg->Get(guid.m_guid);

    uint8_t status = 0;
    msg->Get(status);

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit && ((unit->m_unit->m_unitNpcFlags >> 13) & 1)) {
        unit->m_interactIconType = status ? 0 : 7;
        unit->UpdateInteractIcon();
    }
}

// VERIFY: 2a53878e manual
// OFFSET: 0x006D71A0
int32_t CGItem::IsUsable() const {
    uint32_t flags = this->m_item->m_itemFieldFlags;

    return (((flags >> 3) & 1) || !this->m_item->m_itemFieldMaxdurability || this->m_item->m_itemFieldDurability) && !((flags >> 4) & 1);
}

// VERIFY: f7a9f1f8 manual
// OFFSET: 0x006DA3C0
int32_t OnHonorCurrencyChanged(WOWGUID guid, uint32_t offset, uint32_t count, const void* value) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerFieldHonorCurrency < *static_cast<const uint32_t*>(value)) {
        SI2::PlaySoundKit("LOOTWINDOWCOINSOUND", nullptr, nullptr, nullptr);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_HONOR_CURRENCY_UPDATE, nullptr);
    FrameScript_SignalEvent(SCRIPT_EVENT_CURRENCY_DISPLAY_UPDATE, nullptr);

    return 1;
}

// VERIFY: d617a5c3 manual
// OFFSET: 0x006DA430
int32_t OnArenaCurrencyChanged(WOWGUID guid, uint32_t offset, uint32_t count, const void* value) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player && player->m_player->m_playerFieldArenaCurrency < *static_cast<const uint32_t*>(value)) {
        SI2::PlaySoundKit("LOOTWINDOWCOINSOUND", nullptr, nullptr, nullptr);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_HONOR_CURRENCY_UPDATE, nullptr);
    FrameScript_SignalEvent(SCRIPT_EVENT_CURRENCY_DISPLAY_UPDATE, nullptr);

    return 1;
}

// VERIFY: fa2916ef manual
// OFFSET: 0x006DAA80
int32_t OnUpdatePlayerGlyph(WOWGUID guid, uint32_t offset, uint32_t count, const void* value) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 1;
    }

    uint32_t slot = (offset - offsetof(CGPlayerLocalData, m_playerFieldGlyphs1)) / sizeof(uint32_t);

    if (value && g_glyphPropertiesDB.GetRecord(*static_cast<const int32_t*>(value))) {
        if (g_glyphPropertiesDB.GetRecord(player->m_player->m_playerFieldGlyphs1[slot])) {
            FrameScript_SignalEvent(SCRIPT_EVENT_GLYPH_UPDATED, "%d", slot + 1);
        } else {
            FrameScript_SignalEvent(SCRIPT_EVENT_GLYPH_REMOVED, "%d", slot + 1);
        }
    } else if (g_glyphPropertiesDB.GetRecord(player->m_player->m_playerFieldGlyphs1[slot])) {
        FrameScript_SignalEvent(SCRIPT_EVENT_GLYPH_ADDED, "%d", slot + 1);
    }

    return 1;
}

// VERIFY: 22efc675 manual
// OFFSET: 0x006DAB80
int32_t OnUpdatePlayerGlyphSlotsEnabled(WOWGUID guid, uint32_t offset, uint32_t count, const void* value) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        uint32_t enabled = *static_cast<const uint32_t*>(value);

        for (uint32_t i = 0; i < 6; i++) {
            uint32_t bit = 1 << i;

            if (player->m_player->m_playerGlyphsEnabled & bit) {
                if (!(bit & enabled)) {
                    FrameScript_SignalEvent(SCRIPT_EVENT_GLYPH_ENABLED, "%d", i + 1);
                }
            } else if (bit & enabled) {
                FrameScript_SignalEvent(SCRIPT_EVENT_GLYPH_DISABLED, "%d", i + 1);
            }
        }
    }

    return 1;
}

// VERIFY: baa57619 auto
// OFFSET: 0x006DF850
uint32_t FindEmptySwapIndex() {
    uint32_t index = 0;

    for (; index < s_itemSwaps.m_count; index++) {
        if (!s_itemSwaps.m_data[index].m_srcContainer) {
            break;
        }
    }

    if (index == s_itemSwaps.m_count) {
        s_itemSwaps.SetCount(s_itemSwaps.m_count + 1);
    }

    return index;
}

// VERIFY: fb3e8173 manual
// OFFSET: 0x007076F0
int32_t CGPlayer_C::GetEquipBypassSkill(uint32_t index) const {
    if (index < this->m_equipBypassSkills.m_count) {
        return this->m_equipBypassSkills.m_data[index];
    }

    return 0;
}

// VERIFY: 2b34eb63 auto
// OFFSET: 0x005140E0
int32_t CGPlayer_C::IsInMelee() const {
    return (this->m_combatModeFlags & 0x2) || this->CGUnit_C::IsInMelee();
}

// VERIFY: 03afa9dc manual
// OFFSET: 0x00546150
int32_t CGPlayer_C::HasZoneExplored(int32_t zone) {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    return this->CGPlayer::HasZoneExplored(zone);
}

// VERIFY: e1bf01f5 manual
// OFFSET: 0x006D2A40
void CGPlayer_C::AutoStoreBankItem(WOWGUID bagGuid, uint32_t slot) {
    CGObject_C* bagObject = ClntObjMgrObjectPtr(bagGuid, TYPEMASK_OBJECT, __FILE__, __LINE__);

    if (!bagObject || !bagObject->GetBag()) {
        return;
    }

    CGBag_C* bag = bagObject->GetBag();

    if (slot >= static_cast<uint32_t>(bag->m_numSlots) || !bag->m_slots[slot]) {
        return;
    }

    uint8_t bagIndex = this->FindSlotIndex(bagGuid);

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_AUTOSTORE_BANK_ITEM));
    msg.Put(bagIndex);
    msg.Put(static_cast<uint8_t>(slot));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 6ce36b74 manual
// OFFSET: 0x006D2B30
void CGPlayer_C::AutoBankItem(WOWGUID bagGuid, uint32_t slot) {
    CGObject_C* bagObject = ClntObjMgrObjectPtr(bagGuid, TYPEMASK_OBJECT, __FILE__, __LINE__);

    if (!bagObject || !bagObject->GetBag()) {
        return;
    }

    CGBag_C* bag = bagObject->GetBag();

    if (slot >= static_cast<uint32_t>(bag->m_numSlots) || !bag->m_slots[slot]) {
        return;
    }

    uint8_t bagIndex = this->FindSlotIndex(bagGuid);

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_AUTOBANK_ITEM));
    msg.Put(bagIndex);
    msg.Put(static_cast<uint8_t>(slot));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: b0bc7cf8 manual
// OFFSET: 0x006D2CA0
void CGPlayer_C::PutLootInSlot(WOWGUID bagGuid, uint8_t slot, uint8_t lootSlot) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_STORE_LOOT_IN_SLOT));
    msg.Put(this->FindSlotIndex(bagGuid));
    msg.Put(slot);
    msg.Put(lootSlot);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: b4f04b3e auto
// OFFSET: 0x006D5D10
void CGPlayer_C::ReadItem(WOWGUID guid, uint8_t slot) {
    if (guid == this->m_obj->m_guid) {
        CGPlayer_C::ReadItem(static_cast<uint8_t>(255), slot);
        return;
    }

    for (uint32_t index = 19; index < 23; index++) {
        WOWGUID bagGuid = index < static_cast<uint32_t>(this->m_bag.m_numSlots) ? this->m_bag.m_slots[index] : WOWGUID();

        if (bagGuid == guid) {
            CGPlayer_C::ReadItem(static_cast<uint8_t>(index), slot);
            return;
        }
    }
}

// VERIFY: 63fa2f91 manual
// OFFSET: 0x006D69F0
int32_t CGPlayer_C::OnPetitionShowList(CDataStore* msg) {
    static GuildPetitionInfo petitionList[10];
    WOWGUID vendor;
    uint8_t count = 0;
    msg->Get(vendor.m_guid);
    msg->Get(count);
    memset(petitionList, 0, sizeof(petitionList));

    for (uint32_t i = 0; i < count; i++) {
        msg->Get(petitionList[i].m_index);
        msg->Get(petitionList[i].m_itemID);
        msg->Get(petitionList[i].m_displayID);
        msg->Get(petitionList[i].m_cost);
        msg->Get(petitionList[i].m_charterType);
        msg->Get(petitionList[i].m_requiredSignatures);
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(vendor, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return 1;
    }

    if ((unit->m_unit->m_unitNpcFlags & 0x40000) && (unit->m_unit->m_unitNpcFlags & 0x80000)) {
        CGGuildRegistrar::SetRegistrar(vendor, petitionList);
        return 1;
    }

    CGPetitionVendorInfo::ShowPetitionList(vendor, petitionList, count);
    return 1;
}

// VERIFY: b5faad94 manual
// OFFSET: 0x006D6B10
void __stdcall CGPlayer_C::BuyPetition(const WOWGUID& vendor, CGPetition* petition) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PETITION_BUY));
    msg.Put(vendor.m_guid);
    petition->Pack(&msg);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 027e5ec0 auto
// OFFSET: 0x006D6F00
const char* CGPlayer_C::GetPVPRankString(int32_t rank) const {
    char token[32];
    SStrPrintf(token, sizeof(token), "PVP_RANK_%d_%d", rank, this->GetPVPFactionIndex());

    return this->GetFrameScriptText(token, -1);
}

// VERIFY: 24dda942 manual
// OFFSET: 0x006DC230
bool CGPlayer_C::GetExpandedSkillRank(int32_t skillId, int32_t& rank, int32_t& bonus) const {
    int32_t index = this->GetSkillIndex(skillId);

    if (index < 0) {
        return false;
    }

    rank = this->GetSkillValue(index);
    bonus = this->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? this->m_player->m_playerSkillInfo[index].m_tempBonus : 0;

    return true;
}

// VERIFY: 0f64563c manual
// OFFSET: 0x006DC2C0
int32_t CGPlayer_C::GetSkillRank(int32_t skillId) const {
    int32_t index = this->GetSkillIndex(skillId);

    if (index < 0) {
        return 0;
    }

    int16_t bonus = this->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? this->m_player->m_playerSkillInfo[index].m_tempBonus : 0;
    int32_t rank = bonus + this->GetSkillValue(index);

    return rank < 0 ? 0 : rank;
}

// VERIFY: ca1d9a08 manual
// OFFSET: 0x006DCA00
bool CGPlayer_C::CanTrackUnit(const CGUnit_C* unit) {
    if (this->IsDeadOrGhost()) {
        return false;
    }

    uint8_t flags = unit->m_unit->m_unitFieldBytes1[2];

    if (flags & 0x4) {
        return false;
    }

    if (((unit->m_unit->m_unitDynamicFlags >> 1) & 1) || ((flags & 0x2) && (this->m_player->m_playerFieldBytes[0] & 0x2))) {
        return true;
    }

    int32_t creatureType = unit->GetCreatureType();

    return creatureType && (this->GetCreatureTracking() & (1 << (creatureType - 1)));
}

// VERIFY: 1b989ff9 manual
// OFFSET: 0x006E6250
void CGPlayer_C::GetPendingEnchantTime(CGItem_C* item) {
    if (!item) {
        return;
    }

    PlayerPendingItemExpiration* expiration = this->m_pendingItemExpirations.Head();

    while (expiration) {
        PlayerPendingItemExpiration* next = expiration->Next();

        if (expiration->m_itemGuid == item->m_obj->m_guid) {
            item->UpdateEnchantmentTime(expiration->m_slot, expiration->m_duration);
            expiration->m_link.Unlink();
            delete expiration;
        }

        expiration = next;
    }
}

// VERIFY: b9d03315 auto
// OFFSET: 0x006CEE70
void SetAttackSwingError(int32_t error) {
    s_attackSwingError = error;
    s_attackSwingErrorTime = FrameTime::s_curTimeMs;
}

// VERIFY: 9d0eb63c manual
// OFFSET: 0x006D11C0
int32_t __stdcall CGPlayer_C::OnQuestGiverStatus(CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);

    uint8_t status;
    msg->Get(status);

    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPEMASK_OBJECT, __FILE__, __LINE__);

    if (object && object->IsQuestGiver()) {
        object->SetQuestStatus(status);
    }

    return 1;
}

// VERIFY: a22feec7 manual
// OFFSET: 0x006D1230
int32_t __stdcall CGPlayer_C::OnQuestGiverStatusMultiple(CDataStore* msg) {
    uint32_t count;
    msg->Get(count);

    for (uint32_t i = 0; i < count; i++) {
        WOWGUID guid;
        msg->Get(guid.m_guid);

        uint8_t status;
        msg->Get(status);

        CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPEMASK_OBJECT, __FILE__, __LINE__);

        if (object && object->IsQuestGiver()) {
            object->SetQuestStatus(status);
        }
    }

    return 1;
}

// VERIFY: 3e1e45a2 manual
// OFFSET: 0x006D1650
int32_t __stdcall CGPlayer_C::OnRefundInfo(CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);

    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (!item) {
        msg->m_read = msg->m_size;
        return 1;
    }

    CGItem_C::ItemPurchaseData* data = item->m_itemPurchaseData;

    if (!data) {
        data = new (__FILE__, __LINE__) CGItem_C::ItemPurchaseData;

        if (!data) {
            msg->m_read = msg->m_size;
            return 1;
        }

        item->SetItemPurchaseData(data);
    }

    msg->Get(data->m_money);
    msg->Get(data->m_honorPoints);
    msg->Get(data->m_arenaPoints);

    for (uint32_t i = 0; i < 5; i++) {
        msg->Get(data->m_itemId[i]);
        msg->Get(data->m_itemCount[i]);

        WOWGUID requester = WOWGUID();
        g_itemDBCache.GetRecord(data->m_itemId[i], requester, nullptr, nullptr, true);
    }

    msg->Get(data->unk_0034);
    msg->Get(data->m_refundTimeLeft);

    return 1;
}

// VERIFY: a104b077 manual
// OFFSET: 0x006D4AD0
void CGPlayer_C::UpdateAttackPosition(const CGUnit_C* target) {
    s_attackSwingError = 0;
    s_attackSwingErrorTime = FrameTime::s_curTimeMs;

    if (!target) {
        return;
    }

    float range = this->m_unit->m_unitFieldCombatreach + target->m_unit->m_unitFieldCombatreach + 1.3333334f;

    if (range < 5.0f) {
        range = 5.0f;
    }

    range *= 4.0f;

    C3Vector targetPosition = target->GetPosition();
    C3Vector position = this->GetPosition();
    float dx = position.x - targetPosition.x;
    float dy = position.y - targetPosition.y;
    float dz = position.z - targetPosition.z;

    if (range * range <= dx * dx + dz * dz + dy * dy) {
        this->StopAttack();
    }
}

// VERIFY: f794ab61 manual
// OFFSET: 0x006D52D0
void __stdcall CGPlayer_C::ClearAFK(int32_t force) {
    if (!CGChat::m_afk || (!force && !g_autoClearAFKCVar->m_intValue)) {
        return;
    }

    CGChat::m_afk = 0;
    CGChat::AddChatMessage(FrameScript_GetText("CLEARED_AFK", -1, GENDER_NOT_APPLICABLE), 0, 0, 0, 0, 0, nullptr, WOWGUID(), 0, WOWGUID(), 0, 0, 0);

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_MESSAGECHAT));
    msg.Put(static_cast<uint32_t>(23));
    msg.Put(static_cast<uint32_t>(0));
    msg.PutString("");
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: e056d4fe manual
// OFFSET: 0x006E6B40
CGPlayer_C::CGPlayer_C(uint32_t time, const CClientObjCreate& create)
    : CGUnit_C(time, create),
      unused_1854(-1),
      m_combatModeFlags(0),
      m_pendingItemCacheCount(0),
      m_companionSpells(),
      m_lootTarget(),
      m_autoLoot(0) {
    CGPlayerLocalData* player = this->m_player;
    bool activePlayer = this->m_obj->m_guid == ClntObjMgrGetActivePlayer();
    this->m_bag.m_guid = this->m_obj->m_guid;
    this->m_bag.m_hasBank = 1;
    this->m_bag.m_numSlots = activePlayer ? 150 : 0;
    this->m_bag.m_slots = reinterpret_cast<WOWGUID*>(player->m_playerFieldInvSlotHead);
    this->m_combatTarget.m_guid = 0;

    if (!this->m_unit->m_unitFieldDisplayid) {
        this->GetUnitName(nullptr, 1);
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        for (uint32_t i = 0; i < 1002; i++) {
            new (&this->m_playerLocalMirrorLists[i]) TSList<void>();
        }

        if (this->m_unit->m_unitFieldHealth <= 0) {
            this->RequestRepopOnLanding();
        }

        CGWorldFrame::UpdateScreenEffect();
    }

    this->m_combatModeTime = FrameTime::s_curTimeMs - 500;
    this->m_pvpCooldownTimestamp = 0;
    this->m_gemCounts[0] = 0;
    this->m_gemCounts[1] = 0;
    this->m_gemCounts[2] = 0;
    this->m_gemCounts[3] = 0;
    this->m_dance = nullptr;
    this->m_danceMoveIndex = 0;

    if (create.m_updateFlags & 0x1) {
        Spell_C_SeedMissileTrajectorySequence(static_cast<uint32_t>(this->m_obj->m_guid.m_guid));
    }

    this->m_playedTime = -1;
    this->m_playedTimeBase = 0;

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_REQUEST_PLAYED_TIME));
        msg.Put(static_cast<uint8_t>(0));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    this->m_actionButtonsPending = 0;
}

// VERIFY: 1bf91d47 auto
// OFFSET: 0x006D1CF0
void CGPlayer_C::SetStorage(uint32_t* storage, uint32_t* saved) {
    this->CGUnit_C::SetStorage(storage, saved);
    this->m_player = reinterpret_cast<CGPlayerLocalData*>(storage + OBJECT_END + sizeof(CGUnitData) / sizeof(uint32_t));
    this->m_playerMirror = saved + CGUnit::TotalFields();
}

// VERIFY: aa03190b auto
// OFFSET: 0x006E8280
void CGPlayer_C::PostInit(uint32_t time, const CClientObjCreate& create, bool reenable) {
    this->m_clientStandState = this->m_unit->m_unitFieldBytes1[0];
    this->CGUnit_C::PostInit(time, create, reenable);
    this->GetUnitName(nullptr, 1);
    this->OnGuildChanged(0);
    CGChat::UpdateGuildStatus();
    this->Animate(0.0f);
    this->UpdateAmmoModel(this->GetEquippedItemDisplayId(), 30);
    CGPartyInfo::EnableMember(this, 1);
    CGRaidInfo::EnableMember(this, 1);

    if (CGBattlefieldInfo::m_instanceType == 4 && this->m_player->m_playerBytes3[3] != CGBattlefieldInfo::m_arenaFaction) {
        WOWGUID guid = this->m_obj->m_guid;
        CGBattlefieldInfo::AddArenaOpponent(guid);

        int32_t index;
        guid = this->m_obj->m_guid;

        if (CGBattlefieldInfo::GetArenaOpponentIndex(guid, &index)) {
            CGBattlefieldInfo::m_arenaOpponentSeen[index] = 1;
        }
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        this->PostInitActivePlayer();
    } else {
        this->UpdatePartyMemberState();
    }

    this->UpdatePetReaction();
    this->OnMoveUpdate(time, 1, 1);
}

// VERIFY: 5414e18a auto
// OFFSET: 0x006E6150
void CGPlayer_C::Reenable() {
    this->m_clientStandState = this->m_unit->m_unitFieldBytes1[0];
    this->CGUnit_C::Reenable();
    this->m_pendingItemCacheCount = 0;

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        this->SetActiveMirrorHandlers();
        CGGameUI::EnterWorld();
    }
}

// VERIFY: 2463b98f auto
// OFFSET: 0x006E1D70
void CGPlayer_C::PostReenable() {
    this->CGUnit_C::PostReenable();
    CGPartyInfo::EnableMember(this, 1);
    CGRaidInfo::EnableMember(this, 1);

    if (CGBattlefieldInfo::m_instanceType == 4 && this->m_player->m_playerBytes3[3] != CGBattlefieldInfo::m_arenaFaction) {
        WOWGUID guid = this->m_obj->m_guid;
        CGBattlefieldInfo::AddArenaOpponent(guid);

        int32_t index;
        WOWGUID guid2 = this->m_obj->m_guid;

        if (CGBattlefieldInfo::GetArenaOpponentIndex(guid2, &index)) {
            CGBattlefieldInfo::m_arenaOpponentSeen[index] = 1;
        }
    }

    this->UpdatePartyMemberState();
    this->UpdatePetReaction();
}

// VERIFY: 46563e0e auto
// OFFSET: 0x006CF3A0
static float ClampPitch(float pitch) {
    if (pitch > 1.5707964f) {
        return 1.5707964f;
    }

    if (pitch < -1.5707964f) {
        return -1.5707964f;
    }

    return pitch;
}

// VERIFY: 83512664 manual
// OFFSET: 0x006CF3D0
static float GetDrunkTurnOffset(uint32_t time, float drunkness) {
    if (drunkness == 0.0f) {
        return 0.0f;
    }

    float phase = time * 0.017453292f * drunkness;
    return (cosf(0.154f * phase) + cosf(phase * 0.195f) + cosf(phase * 0.093f)) * 0.333333f * (drunkness * 0.01308997f);
}

// VERIFY: e7725984 manual
// OFFSET: 0x006DE9E0
int32_t CGPlayer_C::CanGrantLevel(WOWGUID guid) {
    if (!guid) {
        return 8;
    }

    CGPlayer_C* target = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!target) {
        return 4;
    }

    if (!CGPlayer_C::IsRAFPlayer(guid)) {
        return 1;
    }

    FactionTemplateRec* faction = g_factionTemplateDB.GetRecord(this->m_unit->m_unitFieldFactiontemplate);
    FactionTemplateRec* targetFaction = g_factionTemplateDB.GetRecord(target->m_unit->m_unitFieldFactiontemplate);

    if (faction && targetFaction && faction->m_factionGroup != targetFaction->m_factionGroup) {
        return 5;
    }

    if (target->m_unit->m_unitFieldLevel >= 60) {
        return 7;
    }

    if (this->m_unit->m_unitFieldLevel <= target->m_unit->m_unitFieldLevel) {
        return 2;
    }

    if (!this->CanAssist(target, false)) {
        return 6;
    }

    return this->m_player->m_playerFieldBytes[1] == 0 ? 3 : 0;
}

// VERIFY: 192067b6 auto
// OFFSET: 0x006DEB30
void CGPlayer_C::UpdateActiveMoverDisplay(uint32_t time) {
    float offset = 0.0f;
    float drunkness = this->GetDrunknessTurnValue();

    if (drunkness > 0.0f) {
        offset = GetDrunkTurnOffset(time, drunkness);
    }

    if (this->IsAutoTracking()) {
        this->HandleTracking(drunkness, offset);
        return;
    }

    if (drunkness <= 0.0f || !(this->m_movement->m_flags & 0xF)) {
        return;
    }

    if (!CGInputControl::GetActive()->CanTurn(this)) {
        return;
    }

    if (!(this->m_movement->m_flags & 0x30) || CGInputControl::GetActive()->CameraCanTurnPlayer()) {
        this->OnSetRawFacingLocal(time, CMath::normalizeangle0to2pi_(this->GetRawFacing() + offset));
    }

    if ((this->m_movement->m_flags & 0x2200000) || (this->m_movement->m_flags2 & 0x20)) {
        if (CGInputControl::GetActive()->CameraCanTurnPlayer() || !(this->m_movementData.m_flags & 0xC0)) {
            this->OnSetPitchLocal(time, ClampPitch(this->GetPitch() + offset * 4.0f));
        }
    }
}

// VERIFY: 1d09cacb manual
// OFFSET: 0x006D5290
static int32_t TrackingMaskUpdateProc(WOWGUID guid, void* param) {
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPEMASK_OBJECT, __FILE__, __LINE__);

    if (object && (object->m_obj->m_type & TYPEMASK_GAMEOBJECT)) {
        static_cast<CGGameObject_C*>(object)->CheckForPassiveHighlight();
    }

    return 1;
}

// VERIFY: 93a46999 manual
// OFFSET: 0x006D7030
static int32_t AuraVisionUpdateHandler(WOWGUID guid, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->AuraVisionUpdated(*static_cast<uint8_t*>(param));
    }

    return 1;
}

// VERIFY: ef96589a manual
// OFFSET: 0x006E7F50
void CGPlayer_C::PostInitActivePlayer() {
    this->m_movementData.m_flags |= 0x200;
    this->SetActiveMirrorHandlers();
    SI2::InitVocalErrorSounds(this->m_unit->m_unitFieldBytes0[0], this->m_player->m_playerBytes3[0]);
    CGSpellBook::ClearSpells();

    for (uint32_t i = 0; i < s_initialSpells.Count(); i++) {
        this->AddKnownSpell(s_initialSpells[i].m_spellId, static_cast<int16_t>(s_initialSpells[i].m_slot), 0, 1);
    }

    CGSpellBook::UpdateSpells(1, 0, 1);
    CGSpellBook::UpdateCompanions();
    CGClassTrainer::RefreshList();

    if (!ClntObjMgrGetPlayerType()) {
        for (uint32_t i = 0; i < sizeof(s_actionButtons) / sizeof(s_actionButtons[0]); i++) {
            if ((s_actionButtons[i] & 0xF0000000) || this->IsSpellKnown(s_actionButtons[i])) {
                CGActionBar::SetAction(i, s_actionButtons[i], 0, 1);
            }
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_SLOT_CHANGED, "%d", 0);
    }

    if (this->IsShapeShifted()) {
        CGSpellBook::UpdateUsable();
        CGSpellBook::UpdateSelection();
        CGActionBar::UpdateShapeShiftBar();
        CGActionBar::UpdateBonusBar();
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_SHAPESHIFT_FORM, nullptr);
    }

    uint8_t auraVision = 0xFF;
    CGWorldFrame::UpdateScreenEffect();
    ClntObjMgrEnumVisibleObjects(AuraVisionUpdateHandler, &auraVision);
    CGWorldFrame::GetActiveCamera()->SetTelescope(0);

    if (!ClntObjMgrGetPlayerType()) {
        this->SetLocalClientControl(true);

        WOWGUID farsight = this->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? this->m_player->m_playerFarsight : WOWGUID();
        WOWGUID charm = this->m_unit->m_unitFieldCharm;
        CGUnit_C* charmed = nullptr;
        WOWGUID controlled;

        if (charm) {
            charmed = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(charm, TYPEMASK_UNIT, __FILE__, __LINE__));
        }

        if (charmed && (charmed->m_unit->m_unitFieldFlags & 0x1000000) && farsight == charm) {
            charmed->SetLocalClientControl(true);
            controlled = charm;
        } else {
            controlled = this->m_obj->m_guid;
        }

        CGGameUI::InitClientControlState(controlled);

        if (controlled != this->m_obj->m_guid) {
            this->ToggleFarSight(charmed);
        }

        CGGameUI::EnterWorld();
        CGGameUI::UpdateActivePlayer();
    }

    ClientServices::Connection()->CharacterSetInGame(1);

    if (s_pendingMovie) {
        CGGameUI::StartCinematic(s_pendingMovie);
        s_pendingMovie = 0;
    } else if (this->GetTransportGUID()) {
        LoadingScreenSetTransparent(true);
    } else {
        LoadingScreenDisable();
    }

    Spell_C_SetPlayerClass(this->m_unit->m_unitFieldBytes0[1]);
    this->CountEquippedGems();

    if (this->m_player->m_playerFlags & 0x200) {
        this->m_pvpCooldownTimestamp = 0;
    } else if (!(this->m_player->m_playerFlags & 0x40000)) {
        this->m_pvpCooldownTimestamp = FrameTime::s_curTimeMs + 300000;
    }

    ClntObjMgrEnumVisibleObjects(TrackingMaskUpdateProc, nullptr);
    CGCommentator::PostInit(this);

    if (SFile::IsStreamingMode() && this->GetTransportGUID()) {
        LoadingScreenDisable();
        World::Preload(this->GetPosition(), 1);
    }
}


// VERIFY: 87cc1c54 auto
// OFFSET: 0x006CEF80
void CGPlayer_C::CancelGiftWrap() {
    CGGameUI::UnlockItem(s_giftWrapItem);
    s_giftWrapItem = WOWGUID();
    CursorSetResetMode(1);
    CursorSetMode(1);
}

// VERIFY: e37e66de manual
// OFFSET: 0x006DD060
CGUnit_C* CGPlayer_C::GetPossessedUnit() {
    if (!(this->m_combatModeFlags & 0x1)) {
        return nullptr;
    }

    WOWGUID farsight;

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        farsight = this->m_player->m_playerFarsight;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(farsight, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return nullptr;
    }

    WOWGUID guid = this->m_obj->m_guid;

    if (!unit->IsPossessedBy(guid)) {
        return nullptr;
    }

    return unit;
}

// VERIFY: 3e8a1a90 manual
// OFFSET: 0x006E1660
void CGPlayer_C::SetCombatMode() {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && (this->CGUnit_C::IsInMelee() || this->m_combat.m_attackSent)) {
        this->StopAttack();
        Spell_C_CancelMeleeSpells();
    }

    if (Spell_C_GetAutoRangedCombatSpell()) {
        Spell_C_SetAutoRangedCombatSpell(0);
        int32_t autoRangedSpell = s_cvAutoRangedCombat->m_intValue ? CGSpellBook::m_autoRangedSpell : 0;

        if (Spell_C_GetAutoRepeatingSpell() == autoRangedSpell) {
            Spell_C_CancelAutoRepeat(true);
        }
    }

    Spell_C_ClearLastAutoRangedCombatError();
    Spell_C_SetAutoRangedCombatSpell(0);
    EventUnregister(EVENT_ID_IDLE, CGPlayer_C::AutoCombatModeEventHandler);
    this->m_combatModeFlags &= ~0xEu;
    CGGameUI::UpdateCombatMode();

    if (static_cast<int32_t>(this->m_unit->m_unitFieldLevel) < 2) {
        return;
    }

    if (CGTutorial::m_tutorialFlags.m_count && (reinterpret_cast<uint8_t*>(CGTutorial::m_tutorialFlags.m_bits.m_data)[7] & 0x1)) {
        return;
    }

    uint32_t count = ++s_combatModeExitCount;

    if (CGTutorial::m_tutorialFlags.m_count && (CGTutorial::m_tutorialFlags.m_bits.m_data[1] & 0x800000) && count > 4) {
        if (IsTutorialFlagged(5) && count > 10) {
            CGTutorial::TriggerTutorial(56);
        }

        CGTutorial::TriggerTutorial(5);
    }

    CGTutorial::TriggerTutorial(55);
}

// VERIFY: b85de264 auto
// OFFSET: 0x006CBC50
int32_t Packet_SMSG_QUEST_CONFIRM_ACCEPT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (msgId == SMSG_QUEST_CONFIRM_ACCEPT) {
        uint32_t questId;
        char title[1024];
        WOWGUID guid;
        msg->Get(questId);
        msg->GetString(title, sizeof(title));
        msg->Get(guid.m_guid);
        CGQuestInfo::ConfirmAcceptQuest(questId, title, &guid);
    }

    return 1;
}

// VERIFY: 36fc807e auto
// OFFSET: 0x006CBCB0
int32_t Packet_SMSG_QUEST_POI_QUERY_RESPONSE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (msgId != SMSG_QUEST_POI_QUERY_RESPONSE) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    CGQuestLog::HandlePOIQueryResponse(msg);
    return 1;
}

// VERIFY: 445c4b27 auto
// OFFSET: 0x006CBD10
int32_t Packet_SMSG_GROUP_CANCEL(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    char name[48];
    msg->GetString(name, sizeof(name));
    CGGameUI::CancelPartyInvite();
    return 1;
}

// VERIFY: 7c272313 auto
// OFFSET: 0x006CBD40
int32_t Packet_SMSG_GROUP_DECLINE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    char name[48];
    msg->GetString(name, sizeof(name));
    CGGameUI::DisplayError(ERR_DECLINE_GROUP_S, name);
    return 1;
}

// VERIFY: 19b171e9 auto
// OFFSET: 0x006CBE10
int32_t Packet_SMSG_GROUP_UNINVITE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGPartyInfo::RemoveAll(1);
    CGGameUI::DisplayError(ERR_UNINVITE_YOU);
    CGLookingForGroup::ClearPartyInfo();
    FrameScript_SignalEvent(SCRIPT_EVENT_CANCEL_SUMMON, nullptr);
    return 1;
}

// VERIFY: 83ee7b00 auto
// OFFSET: 0x006CBE40
int32_t Packet_SMSG_GROUP_DESTROYED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (CGPartyInfo::GetNumPartyMembers()) {
        CGGameUI::DisplayError(ERR_GROUP_DISBANDED);
    }

    CGPartyInfo::RemoveAll(1);
    CGLookingForGroup::ClearPartyInfo();
    FrameScript_SignalEvent(SCRIPT_EVENT_CANCEL_SUMMON, nullptr);
    return 1;
}

// VERIFY: 6ce7a822 auto
// OFFSET: 0x006CBE80
int32_t Packet_SMSG_GROUPACTION_THROTTLED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGGameUI::DisplayError(ERR_GROUP_ACTION_THROTTLED);
    return 1;
}

// VERIFY: 9e39a3ab auto
// OFFSET: 0x006CBEA0
int32_t Packet_SMSG_BATTLEGROUND_INFO_THROTTLED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGGameUI::DisplayError(ERR_BATTLEGROUND_INFO_THROTTLED);
    return 1;
}

// VERIFY: 4881b969 manual
// OFFSET: 0x006CD270
int32_t Packet_SMSG_PLAY_TIME_WARNING(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t flags;
    int32_t seconds;
    msg->Get(reinterpret_cast<uint32_t&>(flags));
    msg->Get(reinterpret_cast<uint32_t&>(seconds));
    char remaining[30];
    SStrPrintf(remaining, sizeof(remaining), "%2d:%02d", seconds / 60 / 60, seconds / 60 % 60);

    if (flags & 0x1000) {
        CGGameUI::DisplayError(ERR_APPROACHING_PARTIAL_PLAY_TIME, remaining);
    } else if (flags & 0x40000000) {
        CGGameUI::DisplayError(ERR_APPROACHING_PARTIAL_PLAY_TIME_2, remaining);
    } else if (flags & 0x2000) {
        CGGameUI::DisplayError(ERR_APPROACHING_NO_PLAY_TIME, remaining);
    } else if (flags & 0x20000000) {
        CGGameUI::DisplayError(ERR_APPROACHING_NO_PLAY_TIME_2, remaining);
    } else if (flags < 0) {
        CGGameUI::DisplayError(ERR_UNHEALTHY_TIME);
    }

    return 1;
}

// VERIFY: 758b4afe auto
// OFFSET: 0x006CD380
int32_t Packet_SMSG_CALENDAR_COMMAND_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t command;
    char eventName[128];
    char name[48];
    uint32_t result;
    msg->Get(command);
    msg->GetString(eventName, sizeof(eventName));
    msg->GetString(name, sizeof(name));
    msg->Get(result);

    if (!result) {
        return 1;
    }

    switch (result) {
        case 1: {
            CGCalendar::CalendarError("CALENDAR_ERROR_GUILD_EVENTS_EXCEEDED", 100);
            break;
        }

        case 2: {
            CGCalendar::CalendarError("CALENDAR_ERROR_EVENTS_EXCEEDED", 30);
            break;
        }

        case 3: {
            CGCalendar::CalendarError("CALENDAR_ERROR_SELF_INVITES_EXCEEDED", 0);
            break;
        }

        case 4: {
            CGCalendar::CalendarError("CALENDAR_ERROR_OTHER_INVITES_EXCEEDED", reinterpret_cast<uintptr_t>(name));
            break;
        }

        case 5: {
            CGCalendar::CalendarError("CALENDAR_ERROR_PERMISSIONS", 0);
            break;
        }

        case 6: {
            CGCalendar::CalendarError("CALENDAR_ERROR_EVENT_INVALID", 0);
            break;
        }

        case 7: {
            CGCalendar::CalendarError("CALENDAR_ERROR_NOT_INVITED", 0);
            break;
        }

        case 8: {
            CGCalendar::CalendarError("CALENDAR_ERROR_INTERNAL", 0);
            break;
        }

        case 9: {
            CGCalendar::CalendarError("ERR_GUILD_PLAYER_NOT_IN_GUILD", 0);
            break;
        }

        case 10: {
            CGCalendar::CalendarError("CALENDAR_ERROR_ALREADY_INVITED_TO_EVENT_S", reinterpret_cast<uintptr_t>(name));
            break;
        }

        case 11: {
            CGCalendar::CalendarError("PLAYER_NOT_FOUND", 0);
            break;
        }

        case 12: {
            CGCalendar::CalendarError("CALENDAR_ERROR_NOT_ALLIED", 0);
            break;
        }

        case 13: {
            CGCalendar::CalendarError("ERR_IGNORING_YOU_S", reinterpret_cast<uintptr_t>(name));
            break;
        }

        case 14: {
            CGCalendar::CalendarError("CALENDAR_ERROR_INVITES_EXCEEDED", 100);
            break;
        }

        case 16: {
            CGCalendar::CalendarError("CALENDAR_ERROR_INVALID_DATE", 0);
            break;
        }

        case 17: {
            CGCalendar::CalendarError("CALENDAR_ERROR_INVALID_TIME", 0);
            break;
        }

        case 19: {
            CGCalendar::CalendarError("CALENDAR_ERROR_NEEDS_TITLE", 0);
            break;
        }

        case 20: {
            CGCalendar::CalendarError("CALENDAR_ERROR_EVENT_PASSED", 0);
            break;
        }

        case 21: {
            CGCalendar::CalendarError("CALENDAR_ERROR_EVENT_LOCKED", 0);
            break;
        }

        case 22: {
            CGCalendar::CalendarError("CALENDAR_ERROR_DELETE_CREATOR_FAILED", 0);
            break;
        }

        case 24: {
            CGCalendar::CalendarError("ERR_SYSTEM_DISABLED", 0);
            break;
        }

        case 25: {
            CGCalendar::CalendarError("ERR_RESTRICTED_ACCOUNT", 0);
            break;
        }

        case 26: {
            CGCalendar::CalendarError("CALENDAR_ERROR_ARENA_EVENTS_EXCEEDED", 0);
            break;
        }

        case 27: {
            CGCalendar::CalendarError("CALENDAR_ERROR_RESTRICTED_LEVEL", 0);
            break;
        }

        case 28: {
            CGCalendar::CalendarError("ERR_USER_SQUELCHED", 0);
            break;
        }

        case 29: {
            CGCalendar::CalendarError("CALENDAR_ERROR_NO_INVITE", 0);
            break;
        }

        case 36: {
            CGCalendar::CalendarError("CALENDAR_ERROR_EVENT_WRONG_SERVER", 0);
            break;
        }

        case 37: {
            CGCalendar::CalendarError("CALENDAR_ERROR_INVITE_WRONG_SERVER", 0);
            break;
        }

        case 38: {
            CGCalendar::CalendarError("CALENDAR_ERROR_NO_GUILD_INVITES", 0);
            break;
        }

        case 39: {
            CGCalendar::CalendarError("CALENDAR_ERROR_INVALID_SIGNUP", 0);
            break;
        }

        case 40: {
            CGCalendar::CalendarError("CALENDAR_ERROR_NO_MODERATOR", 0);
            break;
        }
    }

    return 1;
}

// VERIFY: 1cf32762 auto
// OFFSET: 0x006CD770
int32_t Packet_SMSG_TALENTS_INFO(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t pet = 0;
    msg->Get(pet);

    if (pet) {
        CGTalentInfo::UpdatePetTalentData(msg);
    } else {
        CGTalentInfo::UpdateTalentData(msg);
    }

    return 1;
}

// VERIFY: 8c7e06a6 auto
// OFFSET: 0x006CDF00
int32_t Packet_SMSG_PLAYER_SKINNED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t freeRepop;
    msg->Get(freeRepop);
    CGGameUI::OpenPlayerSkinned(freeRepop != 0);
    return 1;
}

// VERIFY: 3b2739cc auto
// OFFSET: 0x006CE260
int32_t Packet_SMSG_SPELLINSTAKILLLOG(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID casterGuid;
    WOWGUID targetGuid;
    msg->Get(casterGuid.m_guid);
    msg->Get(targetGuid.m_guid);
    uint32_t spellId = 0;
    msg->Get(spellId);
    UnitCombatLogInstakill(casterGuid, targetGuid, spellId);
    return 1;
}

// VERIFY: 1f59b848 auto
// OFFSET: 0x006CE3B0
int32_t Packet_Group_19(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    return CombatLog::HandleSpellRemoveLog(msgId, msg, 0);
}

// VERIFY: 0e8f27d4 auto
// OFFSET: 0x006CE3D0
int32_t Packet_SMSG_SPELLORDAMAGE_IMMUNE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint64_t value = 0;
    msg->Get(value);
    WOWGUID casterGuid = WOWGUID{ value };
    value = 0;
    msg->Get(value);
    WOWGUID targetGuid = WOWGUID{ value };
    uint32_t spellId = 0;
    msg->Get(spellId);
    uint8_t debug = 0;
    msg->Get(debug);
    bool isDebug = debug != 0;
    UnitCombatLogSpellMissed(casterGuid, targetGuid, spellId, 7, !isDebug, isDebug, 0);
    return 1;
}

// VERIFY: 62c887fa auto
// OFFSET: 0x006CE470
int32_t Packet_SMSG_RESURRECT_FAILED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGGameUI::UpdateActivePlayer();
    return 1;
}

// VERIFY: c9571cc5 manual
// OFFSET: 0x006CFE70
int32_t Packet_SMSG_EMOTE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t emote;
    WOWGUID guid;
    msg->Get(emote);
    msg->Get(guid.m_guid);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit && unit->m_unit->m_unitFieldBytes1[0] != 3 && !(unit->m_movement->m_flags & 0x200000)) {
        unit->PlayEmoteAnimation(emote);
    }

    return 1;
}

// VERIFY: d53bbf7b auto
// OFFSET: 0x006D3200
int32_t Packet_SMSG_SPELLLOGMISS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    return CombatLog::HandleSpellLogMiss(msg, 0);
}

// VERIFY: 3b3f5c71 auto
// OFFSET: 0x006D3730
int32_t Packet_SMSG_SPELLLOGEXECUTE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    return CombatLog::HandleSpellLogExecute(msg, 0);
}

// VERIFY: 2a0b9a11 manual
// OFFSET: 0x006D3750
int32_t Packet_SMSG_SPELLDAMAGESHIELD(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint64_t value = 0;
    msg->Get(value);
    WOWGUID victimGuid = WOWGUID{ value };
    value = 0;
    msg->Get(value);
    WOWGUID casterGuid = WOWGUID{ value };
    uint32_t spellId = 0;
    msg->Get(spellId);
    uint32_t damage = 0;
    msg->Get(damage);
    uint32_t overkill = 0;
    msg->Get(overkill);
    uint32_t schoolMask = 0;
    msg->Get(schoolMask);
    UnitCombatLogDamageShield(victimGuid, casterGuid, spellId, damage, overkill, schoolMask);
    CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(casterGuid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (caster) {
        caster->AddWorldDamageText(victimGuid, damage, nullptr, false, false, true);
    }

    CGGameUI::ShowCombatFeedback(casterGuid, damage, schoolMask, 0);
    return 1;
}

// VERIFY: dd286bdb auto
// OFFSET: 0x006D3C10
int32_t Packet_SMSG_SPELLNONMELEEDAMAGELOG(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    return CombatLog::HandleSpellNonMeleeDamageLog(msg, 0);
}

// VERIFY: 759667b2 auto
// OFFSET: 0x006D3DD0
int32_t Packet_SMSG_SPELLHEALLOG(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    return CombatLog::HandleSpellHealLog(msg, 0);
}

// VERIFY: f2c3971c auto
// OFFSET: 0x006D3EF0
int32_t Packet_SMSG_SPELLENERGIZELOG(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    return CombatLog::HandleSpellLogEnergizeLog(msg, 0);
}

struct DamageCalcLabel {
    /* 0000 */ int32_t m_type;
    /* 0004 */ const char* m_name;
    /* 0008 */ int32_t m_id;
    /* 000C */ int32_t m_flags;
};

// STATIC: 0x00AD9CD0
static const DamageCalcLabel s_damageCalcLabels[41] = { { 0, "Base", 1, 1 }, { 3, "DPS Scaler", 2, 1 }, { 2, "Chain Reduction", 3, 0 }, { 2, "Combo Point Damage", 4, 0 }, { 3, "School Damage Done", 5, 0 }, { 3, "Creature Damage Done", 6, 0 }, { 3, "Creature Weapon Damage Done", 7, 0 }, { 2, "Weapon Effect", 8, 1 }, { 2, "Weapon Effect", 9, 0 }, { 3, "Spell Modifier", 10, 1 }, { 3, "Spell Modifier", 11, 1 }, { 3, "Offhand Penalty", 12, 0 }, { 3, "Attack Power Damage", 13, 1 }, { 3, "Creature Multiplier", 14, 1 }, { 3, "School Damage Done", 15, 1 }, { 3, "Weapon Multiplier", 16, 1 }, { 3, "School Damage Taken", 17, 1 }, { 3, "Source Damage Taken", 18, 1 }, { 3, "School Damage Taken", 19, 1 }, { 3, "Source Damage Taken", 20, 1 }, { 3, "Crit Damage Multiplier", 21, 0 }, { 4, "Crit Damage", 22, 1 }, { 2, "Resistance Coefficient", 23, 1 }, { 2, "Bonus Coefficient", 24, 2 }, { 3, "Critical Chance", 25, 2 }, { 3, "Critical Roll", 26, 2 }, { 3, "Seed For Rounding", 27, 0 }, { 4, "Fraction Adjust", 28, 1 }, { 3, "Resilience Modifier", 29, 1 }, { 3, "Armor Factor", 30, 0 }, { 4, "Armor Reduction", 31, 0 }, { 2, "Shield Block", 32, 1 }, { 2, "Melee Crit/Glance/Crush", 33, 1 }, { 3, "Pet Happiness Mult", 34, 1 }, { 3, "Creature Damage Done", 35, 0 }, { 2, "Number of Targets for Area Damage Cap", 36, 0 }, { 2, "Number of Targets", 37, 1 }, { 2, "Crowd Damage", 38, 1 }, { 3, "Mechanic Modifier", 39, 0 }, { 3, "Bonus From Attack Power Coefficient", 40, 1 }, { 3, "Aura State Modifier", 0x3F000000, static_cast<int32_t>(0xFFFFDE00) } };

// VERIFY: 42d7b454 manual
// OFFSET: 0x006D3F10
int32_t Packet_SMSG_DAMAGE_CALC_LOG(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID attacker;
    WOWGUID target;
    uint32_t sourceType;
    uint32_t spellId;
    int32_t count;
    msg->Get(attacker.m_guid);
    msg->Get(target.m_guid);
    msg->Get(sourceType);
    msg->Get(spellId);
    msg->Get(reinterpret_cast<uint32_t&>(count));
    CGUnit_C* attackerUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(attacker, TYPEMASK_UNIT, __FILE__, __LINE__));
    CGUnit_C* targetUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPEMASK_UNIT, __FILE__, __LINE__));
    SpellRec spell;
    const char* spellName;

    if (spellId && g_spellDB.GetRecord(spellId, &spell)) {
        spellName = spell.m_name;
    } else {
        spellName = "None";
    }

    const char* targetName = targetUnit ? targetUnit->GetUnitName(nullptr, 1) : "Unknown Unit";
    const char* attackerName = attackerUnit ? attackerUnit->GetUnitName(nullptr, 1) : "Non-unit";
    ConsolePrintf("DMG: '%s' attacks '%s' sourceType=%d, spell='%s'", attackerName, targetName, sourceType, spellName);

    for (int32_t i = 0; i < count; i++) {
        char label[30];
        int32_t index;
        float oldValue;
        float newValue;
        msg->GetString(label, sizeof(label));
        msg->Get(reinterpret_cast<uint32_t&>(index));
        msg->Get(oldValue);
        msg->Get(newValue);

        if (!SStrCmpI(label, "val", STORM_MAX_STR)) {
            ConsolePrintf("DMG: NOT VALIDATED: Old=%f, New=%f", oldValue, newValue);
        } else if (index >= 0 && index < 41) {
            ConsolePrintf("DMG: (%s) '%s'=%8.2f, total=%f", label, s_damageCalcLabels[index].m_name, oldValue, newValue);
        } else {
            ConsolePrintf("DMG: (%s) '%d'=%8.2f, total=%f", label, index, oldValue, newValue);
        }
    }

    ConsolePrintf("DMG: ==============================================");
    return 1;
}

// VERIFY: 3850fbbd auto
// OFFSET: 0x006D4110
int32_t Packet_SMSG_COMBAT_LOG_MULTIPLE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t count;
    msg->Get(reinterpret_cast<uint32_t&>(count));
    uint32_t baseTime = 0;
    msg->Get(baseTime);

    for (int32_t i = 0; i < count; i++) {
        uint32_t entryTime = 0;
        msg->Get(entryTime);
        int32_t timeOffset = baseTime - entryTime;
        uint32_t type;
        msg->Get(type);

        switch (type) {
            case SMSG_SPELL_MISS_LOG:
                CombatLog::HandleSpellLogMiss(msg, timeOffset);
                break;
            case SMSG_SPELL_BREAK_LOG:
            case SMSG_SPELL_DISPELL_LOG:
            case SMSG_SPELL_STEAL_LOG:
                CombatLog::HandleSpellRemoveLog(static_cast<NETMESSAGE>(type), msg, timeOffset);
                break;
            case SMSG_SPELL_HEAL_LOG:
                CombatLog::HandleSpellHealLog(msg, timeOffset);
                break;
            case SMSG_SPELL_ENERGIZE_LOG:
                CombatLog::HandleSpellLogEnergizeLog(msg, timeOffset);
                break;
            case SMSG_SPELL_EXECUTE_LOG:
                CombatLog::HandleSpellLogExecute(msg, timeOffset);
                break;
            case SMSG_SPELL_PERIODIC_AURA_LOG:
                CombatLog::HandlePeriodicAuraLog(msg, timeOffset);
                break;
            case SMSG_SPELL_NON_MELEE_DAMAGE_LOG:
                CombatLog::HandleSpellNonMeleeDamageLog(msg, timeOffset);
                break;
        }
    }

    return 1;
}

// VERIFY: 4accacac manual
// OFFSET: 0x006D7F10
int32_t Packet_Group_12(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID activePlayer = ClntObjMgrGetActivePlayer();

    if (!activePlayer.m_guid || !ClntObjMgrObjectPtr(activePlayer, TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        return 0;
    }

    switch (msgId) {
        case SMSG_QUEST_GIVER_STATUS:
            return CGPlayer_C::OnQuestGiverStatus(msg);
        case SMSG_QUEST_GIVER_QUEST_LIST_MESSAGE:
            return CGPlayer_C::OnQuestGiverQuestList(msg);
        case SMSG_QUEST_GIVER_QUEST_DETAILS:
            return CGPlayer_C::OnQuestGiverQuestDetails(msg);
        case SMSG_QUEST_GIVER_REQUEST_ITEMS:
            return CGPlayer_C::OnQuestGiverRequestItems(msg);
        case SMSG_QUEST_GIVER_OFFER_REWARD_MESSAGE:
            return CGPlayer_C::OnQuestGiverOfferReward(msg);
        case SMSG_QUEST_GIVER_INVALID_QUEST:
            return CGPlayer_C::OnQuestGiverQuestInvalid(msg);
        case SMSG_QUEST_GIVER_QUEST_COMPLETE:
            return CGPlayer_C::OnQuestGiverQuestComplete(msg);
        case SMSG_QUEST_GIVER_QUEST_FAILED:
            return CGPlayer_C::OnQuestGiverQuestFailed(msg);
        case SMSG_QUEST_LOG_FULL:
            CGGameUI::DisplayError(ERR_QUEST_LOG_FULL);
            return 1;
        case SMSG_QUEST_GIVER_STATUS_MULTIPLE:
            return CGPlayer_C::OnQuestGiverStatusMultiple(msg);
        default:
            return 0;
    }
}

// VERIFY: 1a66db48 manual
// OFFSET: 0x006D8030
int32_t Packet_Group_17(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t questId;
    msg->Get(questId);
    int32_t expired = CGQuestLog::IsQuestExpiredByID(questId);

    if (!expired) {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (!player) {
            return 1;
        }

        for (int32_t i = 0; i < 25; i++) {
            PlayerQuestLogEntry* entry = i >= 0 && i <= 25 ? &player->m_player->m_playerQuestLog[i] : nullptr;

            if (entry && entry->m_questId == questId) {
                if (entry->m_state & 0x2) {
                    expired = 1;
                }

                break;
            }
        }
    }

    WOWGUID none = WOWGUID();
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, nullptr, nullptr, false);

    if (!quest) {
        ConsoleWriteA("Unknown questID (%d) in QuestUpdate", DEFAULT_COLOR, questId);
    }

    if (msgId == SMSG_QUEST_UPDATE_ADD_PVP_CREDIT) {
        uint32_t count;
        msg->Get(count);

        if (expired) {
            return 1;
        }

        if (!quest) {
            return 0;
        }

        CGGameUI::DisplayError(ERR_QUEST_ADD_PLAYER_KILL_SII, count, quest->m_requiredPlayersKilled);
        FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_WATCH_UPDATE, "%d", CGQuestLog::GetQuestIndex(questId));
        return 1;
    }

    switch (msgId) {
        case SMSG_QUEST_UPDATE_FAILED:
        case SMSG_QUEST_UPDATE_FAILED_TIMER: {
            if (msgId == SMSG_QUEST_UPDATE_FAILED_TIMER) {
                CGQuestLog::SetQuestExpiredByID(questId);
            }

            if (quest && !expired) {
                CGGameUI::DisplayError(ERR_QUEST_FAILED_S, quest->m_name);
            }

            return 1;
        }

        case SMSG_QUEST_UPDATE_COMPLETE: {
            if (!quest || expired) {
                return 1;
            }

            if (!quest->m_endText[0]) {
                CGGameUI::DisplayError(ERR_QUEST_UNKNOWN_COMPLETE);
                return 1;
            }

            CGGameUI::DisplayError(ERR_QUEST_OBJECTIVE_COMPLETE_S, quest->m_endText);
            FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_WATCH_UPDATE, "%d", CGQuestLog::GetQuestIndex(questId));
            return 1;
        }

        case SMSG_QUEST_UPDATE_ADD_KILL: {
            int32_t entry;
            uint32_t count;
            uint32_t required;
            WOWGUID guid;
            msg->Get(reinterpret_cast<uint32_t&>(entry));
            msg->Get(count);
            msg->Get(required);
            msg->Get(guid.m_guid);

            if (expired) {
                return 1;
            }

            if (quest) {
                for (uint32_t i = 0; i < 4; i++) {
                    if (static_cast<int32_t>(quest->m_objectiveId[i]) == entry && quest->m_objectiveText[i][0]) {
                        CGGameUI::DisplayError(ERR_QUEST_ADD_FOUND_SII, quest->m_objectiveText[i], count, required);
                        FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_WATCH_UPDATE, "%d", CGQuestLog::GetQuestIndex(questId));
                        return 1;
                    }
                }
            }

            if (entry >= 0) {
                WOWGUID requester = WOWGUID();
                CreatureStats_C* creature = g_creatureDBCache.GetRecord(entry, requester, nullptr, nullptr, false);

                if (creature) {
                    const char* name = creature->GetName(FrameScript_GetPluralIndex(required));
                    CGGameUI::DisplayError(ERR_QUEST_ADD_KILL_SII, name, count, required);
                    FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_WATCH_UPDATE, "%d", CGQuestLog::GetQuestIndex(questId));
                }
            } else {
                WOWGUID requester = WOWGUID();
                GameObjectStats_C* gameObject = g_gameObjectDBCache.GetRecord(entry & 0x7FFFFFFF, requester, nullptr, nullptr, false);

                if (gameObject) {
                    const char* name = gameObject->GetName(FrameScript_GetPluralIndex(required));
                    CGGameUI::DisplayError(ERR_QUEST_ADD_FOUND_SII, name, count, required);
                    FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_WATCH_UPDATE, "%d", CGQuestLog::GetQuestIndex(questId));
                }
            }

            return 1;
        }

        default: {
            return 0;
        }
    }
}

// VERIFY: 7f42c5ae manual
// OFFSET: 0x006D8370
int32_t Packet_SMSG_QUEST_FORCE_REMOVE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (msgId != SMSG_QUEST_FORCE_REMOVED) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        return 0;
    }

    uint32_t questId;
    msg->Get(questId);
    QuestCache* quest = g_questDBCache.GetRecord(questId, WOWGUID{ 0 }, nullptr, nullptr, false);

    if (quest) {
        CGGameUI::DisplayError(ERR_QUEST_FORCE_REMOVED_S, quest->m_name);
    }

    return 1;
}

// VERIFY: 4ed46ae3 manual
// OFFSET: 0x006D8410
int32_t Packet_Group_13(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID activePlayer = ClntObjMgrGetActivePlayer();

    if (!activePlayer.m_guid || !ClntObjMgrObjectPtr(activePlayer, TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        return 0;
    }

    if (msgId == SMSG_TRAINER_LIST) {
        return CGPlayer_C::OnTrainerList(msg);
    }

    if (msgId != SMSG_TRAINER_BUY_FAILED) {
        return 0;
    }

    WOWGUID trainer;
    uint32_t service;
    uint32_t reason;
    msg->Get(trainer.m_guid);
    msg->Get(service);
    msg->Get(reason);

    if (reason == 0) {
        ConsoleWriteA("Trainer service %d unavailable", DEFAULT_COLOR, service);
    } else if (reason == 1) {
        ConsoleWriteA("Not enough money for trainer service %d", DEFAULT_COLOR, service);
    } else if (reason == 2) {
        ConsoleWriteA("Not enough skill points for trainer service %d", DEFAULT_COLOR, service);
    }

    return 1;
}

// VERIFY: 77263fbe auto
// OFFSET: 0x006D84F0
int32_t PH_LOOT_OPCODES(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    switch (msgId) {
        case SMSG_LOOT_RESPONSE:
            return player->OnLootResponse(time, msg);
        case SMSG_LOOT_RELEASE:
            return player->OnLootReleaseResponse(msg);
        case SMSG_LOOT_REMOVED:
            return player->OnLootRemoved(msg);
        case SMSG_LOOT_MONEY_NOTIFY:
            return CGPlayer_C::OnLootMoneyNotify(msg);
        case SMSG_LOOT_ITEM_NOTIFY:
            return CGPlayer_C::OnLootItemNotify(msg);
        case SMSG_LOOT_CLEAR_MONEY:
            CGPlayer_C::OnLootClearMoney();
            return 1;
        case SMSG_TEST_DROP_RATE_RESULT: {
            char text[1024];
            msg->GetString(text, sizeof(text));
            ConsoleWrite(text, DEFAULT_COLOR);
            return 1;
        }
        case SMSG_LOOT_ALL_PASSED:
            return LootRollAllPassed(msg);
        case SMSG_LOOT_ROLL_WON:
            return LootRollWon(msg);
        case SMSG_LOOT_START_ROLL:
            return LootRollStart(msg);
        case SMSG_LOOT_ROLL:
            return LootRollRolled(msg);
        case SMSG_LOOT_MASTER_LIST:
            return LootRollMasterList(msg);
        case SMSG_LOOT_SLOT_CHANGED:
            return player->OnLootSlotChanged(msg);
        default:
            return 0;
    }
}

// VERIFY: b3f1870a manual
// OFFSET: 0x006D8680
int32_t Packet_SMSG_SUMMON_REQUEST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID summoner;
    uint32_t area;
    uint32_t timeout;
    msg->Get(summoner.m_guid);
    msg->Get(area);
    msg->Get(timeout);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 1;
    }

    if (!player->IsDeadOrGhost()) {
        CGGameUI::SetSummonRequest(summoner, area, timeout);
        return 1;
    }

    WDataStore response;
    response.Put(static_cast<uint32_t>(CMSG_SUMMON_RESPONSE));
    response.Put(summoner.m_guid);
    response.Put(static_cast<uint8_t>(0));
    response.Finalize();
    ClientServices::Send2(&response);
    return 0;
}

// VERIFY: 5a3c9684 manual
// OFFSET: 0x006D8750
int32_t Packet_SMSG_ACTION_BUTTONS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t state;
    msg->Get(state);
    uint32_t mode = state;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (mode == 2) {
        if (player) {
            player->m_actionButtonsPending = 1;
        }

        return 1;
    }

    if (mode > 1) {
        return 1;
    }

    for (uint32_t i = 0; i < sizeof(s_actionButtons) / sizeof(s_actionButtons[0]); i++) {
        msg->Get(reinterpret_cast<uint32_t&>(s_actionButtons[i]));
    }

    if (mode == 1 && player) {
        player->m_actionButtonsPending = 0;

        for (uint32_t i = 0; i < sizeof(s_actionButtons) / sizeof(s_actionButtons[0]); i++) {
            int32_t action = s_actionButtons[i];

            if (((action & 0xF0000000) || player->IsSpellKnown(action)) && ((action & 0xF0000000) != 0x20000000 || CGActionBar::IsEquipmentSetValid(action & 0xDFFFFFFF))) {
                CGActionBar::SetAction(i, action, 0, 1);
            } else {
                CGActionBar::RemoveAction(i);
            }
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_SLOT_CHANGED, "%d", 0);
    }

    return 1;
}

// VERIFY: b8ad26b7 manual
// OFFSET: 0x006DBDF0
int32_t Packet_Group_14(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::ReadItemResult(msgId, msg);
    }

    return 1;
}

// VERIFY: fe6d19d8 manual
// OFFSET: 0x006DC010
int32_t Packet_SMSG_TIME_SYNC_REQ(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t counter = 0;
    msg->Get(counter);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->m_movementData.OnTimeSyncRequest(OsGetAsyncTimeMs(), counter);
    }

    return 1;
}

// VERIFY: 74f608d7 manual
// OFFSET: 0x006DEEF0
int32_t Packet_Group_15(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID activePlayer = ClntObjMgrGetActivePlayer();

    if (!activePlayer.m_guid) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(activePlayer, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    switch (msgId) {
        case SMSG_PETITION_SHOW_LIST:
            return CGPlayer_C::OnPetitionShowList(msg);
        case SMSG_PETITION_SHOW_SIGNATURES:
            return CGPlayer_C::OnPetitionShowSignatures(msg);
        case SMSG_PETITION_SIGN_RESULTS:
            return player->OnSignedResults(msg);
        case MSG_PETITION_DECLINE:
            return CGPlayer_C::OnPetitionDecline(msg);
        case SMSG_TURN_IN_PETITION_RESULT:
            return CGPlayer_C::OnTurnInPetitionResults(msg);
        case MSG_PETITION_RENAME:
            return CGPlayer_C::OnPetitionRename(msg);
        default:
            return 0;
    }
}

// VERIFY: 9f33c465 manual
// OFFSET: 0x006DEFA0
int32_t Packet_Group_10(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID activePlayer = ClntObjMgrGetActivePlayer();

    if (!activePlayer.m_guid) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(activePlayer, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    switch (msgId) {
        case SMSG_VENDOR_INVENTORY:
            return player->OnListInventory(msg);
        case SMSG_SELL_ITEM:
            return CGPlayer_C::OnSellResponse(msg);
        case SMSG_BUY_SUCCEEDED:
            return CGPlayer_C::OnBuySucceeded(msg);
        case SMSG_BUY_FAILED:
            return CGPlayer_C::OnBuyFailed(msg);
        case SMSG_ITEM_REFUND_INFO_RESPONSE:
            return CGPlayer_C::OnRefundInfo(msg);
        case SMSG_ITEM_PURCHASE_REFUND_RESULT:
            return player->OnItemRefundResult(msg);
        default:
            return 0;
    }
}

// VERIFY: 5322b599 manual
// OFFSET: 0x006DF050
int32_t Packet_SMSG_INITIAL_SPELLS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t talentSpec;
    msg->Get(talentSpec);
    uint16_t count;
    msg->Get(count);
    s_initialSpells.SetCount(count);

    for (uint16_t i = 0; i < count; i++) {
        msg->Get(s_initialSpells[i].m_spellId);
        msg->Get(s_initialSpells[i].m_slot);
    }

    msg->Get(count);

    for (int32_t i = 0; i < count; i++) {
        uint32_t spellId;
        uint16_t itemId;
        uint16_t category;
        uint32_t spellCooldown;
        uint32_t categoryCooldown;
        msg->Get(spellId);
        msg->Get(itemId);
        msg->Get(category);
        msg->Get(spellCooldown);
        msg->Get(categoryCooldown);
        bool onHold = (categoryCooldown >> 31) & 0x1;
        categoryCooldown &= 0x7FFFFFFF;
        Spell_C_SetCooldownLeft(spellId, itemId, category, spellCooldown, categoryCooldown, onHold, 0);
    }

    return 1;
}

// VERIFY: 8fa31e36 manual
// OFFSET: 0x006DF1A0
int32_t Packet_SMSG_GROUP_INVITE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t canAccept = 0;
    msg->Get(canAccept);
    char name[48];
    msg->GetString(name, sizeof(name));
    uint32_t dungeonType = 0;
    msg->Get(dungeonType);
    uint8_t count = 0;
    TSGrowableArray<uint32_t> dungeons;
    msg->Get(count);
    dungeons.SetCount(count);

    for (uint32_t i = 0; i < count; i++) {
        msg->Get(dungeons[i]);
    }

    uint32_t unk = 0;
    msg->Get(unk);

    if (canAccept) {
        SignalEvent_EVENT_PARTY_INVITE_REQUEST(name);
        CGGameUI::DisplayError(ERR_INVITED_TO_GROUP_SS, name, name);
    } else {
        CGGameUI::DisplayError(ERR_INVITED_ALREADY_IN_GROUP_SS, name, name);
    }

    return 1;
}

// VERIFY: 5526bdf2 manual
// OFFSET: 0x006E2210
int32_t Packet_SMSG_CANCEL_COMBAT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->SetCombatMode();
    }

    return 1;
}

// VERIFY: 8c7c4c56 manual
// OFFSET: 0x006E6330
int32_t Packet_Group_18(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (!msg) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    WOWGUID itemGuid;
    msg->Get(itemGuid.m_guid);
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(itemGuid, TYPEMASK_ITEM, __FILE__, __LINE__));

    switch (msgId) {
        case SMSG_ITEM_TIME_UPDATE: {
            uint32_t seconds;
            msg->Get(seconds);

            if (item) {
                item->UpdateExpirationTime(seconds);
            }

            return 1;
        }
        case SMSG_ITEM_ENCHANT_TIME_UPDATE: {
            uint32_t slot;
            uint32_t seconds;
            WOWGUID playerGuid;
            msg->Get(slot);
            msg->Get(seconds);
            msg->Get(playerGuid.m_guid);

            if (item) {
                item->UpdateEnchantmentTime(slot, seconds);
                return 1;
            }

            CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(playerGuid, TYPEMASK_PLAYER, __FILE__, __LINE__));

            if (player || (player = ClntObjMgrGetActivePlayerObj())) {
                player->QueueItemEnchantTime(itemGuid, slot, seconds);
            }

            return 1;
        }
        case SMSG_SOCKET_GEMS: {
            uint32_t a2 = 0;
            msg->Get(a2);
            uint32_t a3 = 0;
            msg->Get(a3);
            uint32_t a4 = 0;
            msg->Get(a4);
            uint32_t a5 = 0;
            msg->Get(a5);
            item->UpdateSockets(a2, a3, a4, a5);
            return 1;
        }
        default:
            return 0;
    }
}

// VERIFY: 33f6984f manual
// OFFSET: 0x006E7840
int32_t Packet_SMSG_REMOVED_SPELL(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t spellId;
    msg->Get(spellId);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->DelKnownSpell(spellId, 0);
        return 1;
    }

    for (uint32_t i = 0; i < s_initialSpells.Count(); i++) {
        if (s_initialSpells[i].m_spellId == spellId) {
            s_initialSpells[i].m_spellId = 0;
            s_initialSpells[i].m_slot = 0;
        }
    }

    return 1;
}

// VERIFY: 299341f9 manual
// OFFSET: 0x006E7D60
int32_t Packet_SMSG_LEARNED_SPELL(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t spellId;
    uint16_t slot;
    msg->Get(spellId);
    msg->Get(slot);

    if (SFile::IsStreamingMode() && !LoadingScreenDrawing()) {
        PrefetchSpellVisuals(spellId);
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->AddKnownSpell(spellId, static_cast<int16_t>(slot), 1, 1);
    } else {
        InitialSpellStruct* spell = s_initialSpells.New();
        spell->m_spellId = spellId;
        spell->m_slot = slot;
    }

    return 1;
}

// VERIFY: 84e6a81b manual
// OFFSET: 0x006E7E00
int32_t Packet_SMSG_SUPERCEDED_SPELL(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t oldSpellId;
    uint32_t newSpellId;
    msg->Get(oldSpellId);
    msg->Get(newSpellId);

    if (SFile::IsStreamingMode() && !LoadingScreenDrawing()) {
        PrefetchSpellVisuals(newSpellId);
    }

    CGActionBar::ReplaceSpell(oldSpellId, newSpellId);
    CGSpellBook::ReplaceSpell(oldSpellId, newSpellId);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->DelKnownSpell(oldSpellId, 1);
        player->AddKnownSpell(newSpellId, 0, 0, 0);
        CGClassTrainer::RefreshList();
        return 1;
    }

    uint32_t i;

    for (i = 0; i < s_initialSpells.Count(); i++) {
        if (s_initialSpells[i].m_spellId == oldSpellId) {
            s_initialSpells[i].m_spellId = newSpellId;
            break;
        }
    }

    if (i == s_initialSpells.Count()) {
        InitialSpellStruct* spell = s_initialSpells.New();
        spell->m_spellId = newSpellId;
        spell->m_slot = 0;
    }

    CGClassTrainer::RefreshList();
    return 1;
}

int32_t Packet_SMSG_DEBUG_SERVER_GEO(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    return 1;
}

// VERIFY: 97769787 auto
// OFFSET: 0x006E83B0
void CGPlayer_C::PlayerInitializeMessageHandlers() {
    ClientServices::SetMessageHandler(SMSG_MOUNT_RESULT, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_DISMOUNT_RESULT, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_INVENTORY_CHANGE_FAILURE, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_OPEN_CONTAINER, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_ITEM_PUSH_RESULT, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_VENDOR_INVENTORY, Packet_Group_10, nullptr);
    ClientServices::SetMessageHandler(SMSG_BUY_FAILED, Packet_Group_10, nullptr);
    ClientServices::SetMessageHandler(SMSG_BUY_SUCCEEDED, Packet_Group_10, nullptr);
    ClientServices::SetMessageHandler(SMSG_SELL_ITEM, Packet_Group_10, nullptr);
    ClientServices::SetMessageHandler(SMSG_ITEM_REFUND_INFO_RESPONSE, Packet_Group_10, nullptr);
    ClientServices::SetMessageHandler(SMSG_ITEM_PURCHASE_REFUND_RESULT, Packet_Group_10, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOOT_RESPONSE, PH_LOOT_OPCODES, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOOT_RELEASE, PH_LOOT_OPCODES, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOOT_REMOVED, PH_LOOT_OPCODES, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOOT_SLOT_CHANGED, PH_LOOT_OPCODES, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOOT_MONEY_NOTIFY, PH_LOOT_OPCODES, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOOT_ITEM_NOTIFY, PH_LOOT_OPCODES, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOOT_CLEAR_MONEY, PH_LOOT_OPCODES, nullptr);
    ClientServices::SetMessageHandler(SMSG_LEARNED_SPELL, Packet_SMSG_LEARNED_SPELL, nullptr);
    ClientServices::SetMessageHandler(SMSG_UNLEARNED_SPELLS, Packet_SMSG_REMOVED_SPELL, nullptr);
    ClientServices::SetMessageHandler(SMSG_SUPERCEDED_SPELLS, Packet_SMSG_SUPERCEDED_SPELL, nullptr);
    ClientServices::SetMessageHandler(SMSG_SEND_KNOWN_SPELLS, Packet_SMSG_INITIAL_SPELLS, nullptr);
    ClientServices::SetMessageHandler(SMSG_UPDATE_ACTION_BUTTONS, Packet_SMSG_ACTION_BUTTONS, nullptr);
    ClientServices::SetMessageHandler(SMSG_GROUP_INVITE, Packet_SMSG_GROUP_INVITE, nullptr);
    ClientServices::SetMessageHandler(SMSG_GROUP_CANCEL, Packet_SMSG_GROUP_CANCEL, nullptr);
    ClientServices::SetMessageHandler(SMSG_GROUP_DECLINE, Packet_SMSG_GROUP_DECLINE, nullptr);
    ClientServices::SetMessageHandler(SMSG_GROUP_UNINVITE, Packet_SMSG_GROUP_UNINVITE, nullptr);
    ClientServices::SetMessageHandler(SMSG_GROUP_SET_LEADER, Packet_SMSG_GROUP_SET_LEADER, nullptr);
    ClientServices::SetMessageHandler(SMSG_GROUP_DESTROYED, Packet_SMSG_GROUP_DESTROYED, nullptr);
    ClientServices::SetMessageHandler(SMSG_PARTY_COMMAND_RESULT, Packet_SMSG_PARTY_COMMAND_RESULT, nullptr);
    ClientServices::SetMessageHandler(SMSG_GROUP_LIST, Packet_SMSG_GROUP_LIST, nullptr);
    ClientServices::SetMessageHandler(SMSG_REAL_GROUP_UPDATE, Packet_SMSG_REAL_GROUP_UPDATE, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_GIVER_QUEST_LIST_MESSAGE, Packet_Group_12, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_GIVER_INVALID_QUEST, Packet_Group_12, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_GIVER_QUEST_DETAILS, Packet_Group_12, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_GIVER_REQUEST_ITEMS, Packet_Group_12, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_GIVER_OFFER_REWARD_MESSAGE, Packet_Group_12, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_GIVER_QUEST_COMPLETE, Packet_Group_12, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_GIVER_QUEST_FAILED, Packet_Group_12, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_GIVER_STATUS, Packet_Group_12, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_GIVER_STATUS_MULTIPLE, Packet_Group_12, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_LOG_FULL, Packet_Group_12, nullptr);
    ClientServices::SetMessageHandler(SMSG_TRAINER_LIST, Packet_Group_13, nullptr);
    ClientServices::SetMessageHandler(SMSG_TRAINER_BUY_FAILED, Packet_Group_13, nullptr);
    ClientServices::SetMessageHandler(SMSG_SET_PROFICIENCY, Packet_SMSG_SET_PROFICIENCY, nullptr);
    ClientServices::SetMessageHandler(SMSG_RESURRECT_REQUEST, Packet_SMSG_RESURRECT_REQUEST, nullptr);
    ClientServices::SetMessageHandler(SMSG_PLAYER_SKINNED, Packet_SMSG_PLAYER_SKINNED, nullptr);
    ClientServices::SetMessageHandler(SMSG_INSPECT_RESULTS_UPDATE, Packet_SMSG_INSPECT, nullptr);
    ClientServices::SetMessageHandler(SMSG_INSPECT_TALENT, Packet_SMSG_INSPECT_TALENT, nullptr);
    ClientServices::SetMessageHandler(SMSG_READ_ITEM_RESULT_OK, Packet_Group_14, nullptr);
    ClientServices::SetMessageHandler(SMSG_READ_ITEM_RESULT_FAILED, Packet_Group_14, nullptr);
    ClientServices::SetMessageHandler(SMSG_CANCEL_COMBAT, Packet_SMSG_CANCEL_COMBAT, nullptr);
    ClientServices::SetMessageHandler(SMSG_TAXI_NODE_STATUS, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_SHOW_TAXI_NODES, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_ACTIVATE_TAXI_REPLY, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_GUILD_INVITE, Packet_SMSG_GUILD_INVITE, nullptr);
    ClientServices::SetMessageHandler(SMSG_GUILD_DECLINE, Packet_SMSG_GUILD_DECLINE, nullptr);
    ClientServices::SetMessageHandler(SMSG_GUILD_INFO, Packet_SMSG_GUILD_INFO, nullptr);
    ClientServices::SetMessageHandler(SMSG_GUILD_EVENT, Packet_SMSG_GUILD_EVENT, nullptr);
    ClientServices::SetMessageHandler(SMSG_GUILD_COMMAND_RESULT, Packet_SMSG_GUILD_COMMAND_RESULT, nullptr);
    ClientServices::SetMessageHandler(MSG_SAVE_GUILD_EMBLEM, Packet_MSG_SAVE_GUILD_EMBLEM, nullptr);
    ClientServices::SetMessageHandler(MSG_TABARDVENDOR_ACTIVATE, Packet_MSG_TABARDVENDOR_ACTIVATE, nullptr);
    ClientServices::SetMessageHandler(SMSG_PETITION_SHOW_LIST, Packet_Group_15, nullptr);
    ClientServices::SetMessageHandler(SMSG_PETITION_SHOW_SIGNATURES, Packet_Group_15, nullptr);
    ClientServices::SetMessageHandler(SMSG_PETITION_SIGN_RESULTS, Packet_Group_15, nullptr);
    ClientServices::SetMessageHandler(MSG_PETITION_DECLINE, Packet_Group_15, nullptr);
    ClientServices::SetMessageHandler(SMSG_TURN_IN_PETITION_RESULT, Packet_Group_15, nullptr);
    ClientServices::SetMessageHandler(MSG_PETITION_RENAME, Packet_Group_15, nullptr);
    ClientServices::SetMessageHandler(SMSG_BIND_POINT_UPDATE, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_BINDZONEREPLY, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_EMOTE, Packet_SMSG_EMOTE, nullptr);
    ClientServices::SetMessageHandler(SMSG_PLAYER_BOUND, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_PLAYERBINDERROR, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_NEW_TAXI_PATH, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_PET_NAME_INVALID, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_EXPLORATION_EXPERIENCE, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_PARTY_MEMBER_STATS, Packet_HandlePartyMemberStats, nullptr);
    ClientServices::SetMessageHandler(SMSG_PARTY_MEMBER_STATS_FULL, Packet_HandlePartyMemberStats, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_UPDATE_FAILED, Packet_Group_17, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_UPDATE_FAILED_TIMER, Packet_Group_17, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_UPDATE_COMPLETE, Packet_Group_17, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_UPDATE_ADD_KILL, Packet_Group_17, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_UPDATE_ADD_PVP_CREDIT, Packet_Group_17, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_CONFIRM_ACCEPT, Packet_SMSG_QUEST_CONFIRM_ACCEPT, nullptr);
    ClientServices::SetMessageHandler(SMSG_SHOW_BANK, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_BUY_BANK_SLOT_RESULT, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_LEVEL_UP_INFO, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(MSG_MINIMAP_PING, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_TRIGGER_CINEMATIC, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_ITEM_TIME_UPDATE, Packet_Group_18, nullptr);
    ClientServices::SetMessageHandler(SMSG_ITEM_ENCHANT_TIME_UPDATE, Packet_Group_18, nullptr);
    ClientServices::SetMessageHandler(MSG_RANDOM_ROLL, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_FISH_NOT_HOOKED, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_FISH_ESCAPED, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCEACTIONSHOW, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_GOD_MODE, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_PETGODMODE, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_IGNORE_REQUIREMENTS_CHEAT, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_IGNORE_DIMINISHING_RETURNS_CHEAT, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_CLEAR_FAR_SIGHT_IMMEDIATE, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_CHAT_WRONG_FACTION, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_CHAT_PLAYER_NOTFOUND, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_CHAT_RESTRICTED, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_CHAT_NOT_IN_PARTY, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_GM_TICKET_GET_SYSTEM_STATUS, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPIRIT_HEALER_CONFIRM, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(MSG_TALENT_WIPE_CONFIRM, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_BINDER_CONFIRM, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_EXECUTE_LOG, Packet_SMSG_SPELLLOGEXECUTE, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_MISS_LOG, Packet_SMSG_SPELLLOGMISS, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_DAMAGE_SHIELD, Packet_SMSG_SPELLDAMAGESHIELD, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_INSTAKILL_LOG, Packet_SMSG_SPELLINSTAKILLLOG, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_NON_MELEE_DAMAGE_LOG, Packet_SMSG_SPELLNONMELEEDAMAGELOG, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_HEAL_LOG, Packet_SMSG_SPELLHEALLOG, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_ENERGIZE_LOG, Packet_SMSG_SPELLENERGIZELOG, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_DISPELL_LOG, Packet_Group_19, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_STEAL_LOG, Packet_Group_19, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_BREAK_LOG, Packet_Group_19, nullptr);
    ClientServices::SetMessageHandler(SMSG_RESURRECT_FAILED, Packet_SMSG_RESURRECT_FAILED, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_OR_DAMAGE_IMMUNE, Packet_SMSG_SPELLORDAMAGE_IMMUNE, nullptr);
    ClientServices::SetMessageHandler(MSG_QUEST_PUSH_RESULT, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_DAMAGE_CALC_LOG, Packet_SMSG_DAMAGE_CALC_LOG, nullptr);
    ClientServices::SetMessageHandler(SMSG_RAID_GROUP_ONLY, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_INSTANCE_LOCK_WARNING_QUERY, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOOT_START_ROLL, PH_LOOT_OPCODES, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOOT_ALL_PASSED, PH_LOOT_OPCODES, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOOT_ROLL_WON, PH_LOOT_OPCODES, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOOT_ROLL, PH_LOOT_OPCODES, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOOT_MASTER_LIST, PH_LOOT_OPCODES, nullptr);
    ClientServices::SetMessageHandler(SMSG_TEST_DROP_RATE_RESULT, PH_LOOT_OPCODES, nullptr);
    ClientServices::SetMessageHandler(SMSG_SUMMON_REQUEST, Packet_SMSG_SUMMON_REQUEST, nullptr);
    ClientServices::SetMessageHandler(SMSG_SUMMON_CANCEL, OnSummonCancel, nullptr);
    ClientServices::SetMessageHandler(SMSG_PLAY_TIME_WARNING, Packet_SMSG_PLAY_TIME_WARNING, nullptr);
    ClientServices::SetMessageHandler(SMSG_ARENA_TEAM_INVITE, Packet_SMSG_ARENA_TEAM_INVITE, nullptr);
    ClientServices::SetMessageHandler(SMSG_ARENA_TEAM_EVENT, Packet_SMSG_ARENA_TEAM_EVENT, nullptr);
    ClientServices::SetMessageHandler(SMSG_ARENA_TEAM_COMMAND_RESULT, Packet_SMSG_ARENA_TEAM_COMMAND_RESULT, nullptr);
    ClientServices::SetMessageHandler(SMSG_ARENA_ERROR, Packet_SMSG_ARENA_ERROR, nullptr);
    ClientServices::SetMessageHandler(SMSG_DEATH_RELEASE_LOC, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_FORCED_DEATH_UPDATE, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_TIME_SYNC_REQUEST, Packet_SMSG_TIME_SYNC_REQ, nullptr);
    ClientServices::SetMessageHandler(SMSG_VOICE_SESSION_ROSTER_UPDATE, Packet_SMSG_VOICE_SESSION_ROSTER_UPDATE, nullptr);
    ClientServices::SetMessageHandler(SMSG_VOICE_SESSION_LEAVE, Packet_SMSG_VOICE_SESSION_LEAVE, nullptr);
    ClientServices::SetMessageHandler(SMSG_VOICE_SET_TALKER_MUTED, Packet_SMSG_VOICE_SET_TALKER_MUTED, nullptr);
    ClientServices::SetMessageHandler(SMSG_VOICE_PARENTAL_CONTROLS, Packet_SMSG_VOICE_PARENTAL_CONTROLS, nullptr);
    ClientServices::SetMessageHandler(SMSG_CROSSED_INEBRIATION_THRESHOLD, Packet_SMSG_CROSSED_INEBRIATION_THRESHOLD, nullptr);
    ClientServices::SetMessageHandler(SMSG_COMPLAINT_RESULT, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_FEATURE_SYSTEM_STATUS, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_AVAILABLE_VOICE_CHANNEL, Packet_SMSG_AVAILABLE_VOICE_CHANNEL, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_COMMAND_RESULT, Packet_SMSG_CALENDAR_COMMAND_RESULT, nullptr);
    ClientServices::SetMessageHandler(SMSG_REPORT_PVP_AFK_RESULT, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_GROUP_ACTION_THROTTLED, Packet_SMSG_GROUPACTION_THROTTLED, nullptr);
    ClientServices::SetMessageHandler(SMSG_SEND_UNLEARN_SPELLS, Packet_SMSG_SEND_UNLEARN_SPELLS, nullptr);
    ClientServices::SetMessageHandler(SMSG_CONVERT_RUNE, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_RESYNC_RUNES, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_ADD_RUNE_POWER, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_BATTLEGROUND_INFO_THROTTLED, Packet_SMSG_BATTLEGROUND_INFO_THROTTLED, nullptr);
    ClientServices::SetMessageHandler(SMSG_UPDATE_TALENT_DATA, Packet_SMSG_TALENTS_INFO, nullptr);
    ClientServices::SetMessageHandler(SMSG_ARENA_TEAM_CHANGE_FAILED_QUEUED, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_POI_QUERY_RESPONSE, Packet_SMSG_QUEST_POI_QUERY_RESPONSE, nullptr);
    ClientServices::SetMessageHandler(SMSG_TOGGLE_XP_GAIN, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_DEBUG_SERVER_GEO, Packet_SMSG_DEBUG_SERVER_GEO, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUEST_FORCE_REMOVED, Packet_SMSG_QUEST_FORCE_REMOVE, nullptr);
    ClientServices::SetMessageHandler(SMSG_CAMERA_SHAKE, Packet_Group_9, nullptr);
    ClientServices::SetMessageHandler(SMSG_SOCKET_GEMS, Packet_Group_18, nullptr);
    ClientServices::SetMessageHandler(SMSG_COMBAT_LOG_MULTIPLE, Packet_SMSG_COMBAT_LOG_MULTIPLE, nullptr);
}

// VERIFY: ea8acbe5 manual
// OFFSET: 0x006E4950
void CGPlayer_C::CombatModeEnter(const WOWGUID& target, int32_t queueRanged, int32_t silent) {
    if (!CGGameUI::CanPerformAction(1)) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    int32_t rangedSpell = s_cvAutoRangedCombat->m_intValue ? CGSpellBook::m_autoRangedSpell : 0;

    if (this->m_mountDisplayId > 0 && !(this->m_clientFlags & 0x10000000) && !this->MountedCombatAllowed()) {
        float minRange;
        float maxRange;

        if (rangedSpell) {
            Spell_C::GetSpellRange(this, rangedSpell, &minRange, &maxRange, unit);
        } else {
            maxRange = this->CGUnit::GetMeleeRange(unit);
        }

        C3Vector position = unit->GetPosition();

        if (maxRange * maxRange < this->GetDistanceToPos(position)) {
            if (!silent) {
                CGGameUI::DisplayError(ERR_OUT_OF_RANGE);
            }

            return;
        }

        this->Dismount();
    }

    if ((!(this->m_combatModeFlags & 0x2) && !this->CGUnit_C::IsInMelee()) || target != this->m_combatTarget) {
        this->m_combatModeTime = FrameTime::s_curTimeMs;
    }

    this->m_combatTarget = target;

    if (!rangedSpell) {
        this->MeleeModeEnter(target);
        return;
    }

    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer() || (this->m_unit->m_unitFieldFlags & 0x20000) || !CGGameUI::m_hasControl) {
        return;
    }

    if (this->m_player->m_playerFlags & 0x1000000) {
        CGGameUI::DisplayError(ERR_CLIENT_LOCKED_OUT);
        return;
    }

    if (!this->CanAttackNow(unit)) {
        if (this->IsInMelee()) {
            this->SetCombatMode();
        }

        return;
    }

    EventRegister(EVENT_ID_IDLE, CGPlayer_C::AutoCombatModeEventHandler);
    this->m_combatModeFlags |= 0x2;
    CGGameUI::UpdateCombatMode();

    if (!this->IsInMeleeRange(unit)) {
        if (queueRanged) {
            this->m_combatModeFlags |= 0x8;
        }

        this->m_combatModeFlags |= 0x4;
    }
}

// VERIFY: bbf5e390 manual
// OFFSET: 0x006E0640
int32_t CGPlayer_C::GetLanguageSkill(uint32_t language, uint32_t& skill) {
    skill = 0;

    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    int32_t languageSpell = CGSpellBook::m_languageSpells.m_data[language];

    if (!languageSpell) {
        if (!(this->m_unit->m_unitFieldFlags2 & 0x8)) {
            return 0;
        }

        SpellRec spell;

        for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
            if (!g_spellDB.GetRecord(this->m_auras[i].m_spellId, &spell)) {
                continue;
            }

            for (uint32_t j = 0; j < 3; j++) {
                if (spell.m_effectAura[j] == 244 && spell.m_effectMiscValue[j] == static_cast<int32_t>(language)) {
                    skill = 300;
                    return 1;
                }
            }
        }

        return 0;
    }

    SkillLineAbilityRec* ability = SpellTableLookupAbility(this->m_unit->m_unitFieldBytes0[0], this->m_unit->m_unitFieldBytes0[1], languageSpell);

    if (!ability) {
        return 0;
    }

    for (int32_t i = 0; i < 128; i++) {
        uint16_t skillId = this->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? this->m_player->m_playerSkillInfo[i].m_skillId : 0;

        if (skillId == ability->m_skillLine) {
            if (!g_skillLineDB.GetRecord(ability->m_skillLine)) {
                return 0;
            }

            int32_t modifier = this->GetSkillModifier(i);
            skill = this->GetSkillValue(i) + modifier;
            return 1;
        }
    }

    return 0;
}

// VERIFY: e61db75c manual
// OFFSET: 0x005CD920
int16_t CGPlayer_C::GetSkillModifier(int32_t index) {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    return this->m_player->m_playerSkillInfo[index].m_tempBonus;
}

// VERIFY: c0b0e280 auto
// OFFSET: 0x006E7B00
void CGPlayer_C::AddKnownSpell(int32_t spellId, int32_t a2, int32_t a3, int32_t a4) {
    if (spellId > g_spellDB.m_maxID) {
        SysMsgPrintf(STATUS_ERROR, 2, "NOSPELLIDFOUND|%d", spellId);
        return;
    }

    if (a4) {
        if (a3) {
            CGActionBar::ReplaceSpell(0, spellId);
        }

        CGSpellBook::AddKnownSpell(spellId, a3, 0);
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    SkillLineAbilityRec* ability = SpellTableLookupAbility(this->m_unit->m_unitFieldBytes0[0], this->m_unit->m_unitFieldBytes0[1], spellId);
    SkillLineRec* skillLine = ability ? g_skillLineDB.GetRecord(ability->m_skillLine) : nullptr;

    if (skillLine && skillLine->m_id == 778) {
        this->m_companionSpells.Add(1, &spellId);
    }

    if (spell.m_attributes & 0x20) {
        if (skillLine) {
            HASHKEY_NONE key;
            TRADESKILLLINE* line = this->m_tradeSkillLines.Ptr(skillLine->m_id, key);

            if (!line) {
                line = this->m_tradeSkillLines.InternalNewNode(skillLine->m_id, 0, 0);
                line->m_hashval = skillLine->m_id;
            }

            *line->m_spells.New() = spellId;
        }

        CGTradeSkillInfo::RefreshList(1);
    } else if (spell.m_effect[0] == 101) {
        SpellSetFeedPetSpell(spell.m_id);
    } else if (spell.m_effect[0] == 155) {
        if (!this->HasEquipBypassSkill(spellId)) {
            this->m_equipBypassSkills.Add(1, &spellId);
        }
    } else {
        bool proficiency = false;

        for (uint32_t i = 0; i < 3; i++) {
            if (spell.m_effect[i] == 60) {
                proficiency = true;
                break;
            }
        }

        if (proficiency) {
            for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
                ItemSubClassRec* rec = (i < 0 || i >= g_itemSubClassDB.m_numRecords) ? nullptr : &g_itemSubClassDB.m_records[i];

                if (rec->m_classID <= 0 || rec->m_classID != spell.m_equippedItemClass || 1 << rec->m_subClassID != spell.m_equippedItemSubclass) {
                    continue;
                }

                int32_t skill = rec->m_generatedID;

                if (skill) {
                    HASHKEY_NONE key;
                    PROFICIENCYSPELL* proficiencySpell = this->m_proficiencySpells.Ptr(skill, key);

                    if (!proficiencySpell) {
                        proficiencySpell = this->m_proficiencySpells.InternalNewNode(skill, 0, 0);
                        proficiencySpell->m_hashval = skill;
                    }

                    proficiencySpell->m_spellId = spellId;
                }

                break;
            }
        }
    }

    if (a3) {
        CGClassTrainer::RefreshList();
    }

    if (spell.m_effect[0] == 40) {
        s_dualWieldSpell = spellId;
    }
}

// VERIFY: f0445597 auto
// OFFSET: 0x006D71E0
int32_t GetModalNextSpell() {
    return s_cvAutoRangedCombat->m_intValue ? CGSpellBook::m_autoRangedSpell : 0;
}

// VERIFY: 3f2316b1 manual
// OFFSET: 0x006DE840
int32_t CGPlayer_C::IsXRayVisionActive() {
    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    return (player->m_auraEffectMask[0x22] >> 1) & 1;
}


// VERIFY: a9d2639d auto
// OFFSET: 0x006CEAF0
int32_t CGPlayer_C::GetNewContinentID() {
    return ClntObjMgrGetMapID();
}

// VERIFY: e3365355 auto
// OFFSET: 0x007FE510
void Spell_C_SeedMissileTrajectorySequence(uint32_t guidLow) {
    s_missileTrajectorySequence = guidLow;
}

// INLINED
static uint32_t SetTrajectoryCheckBit(uint32_t value, uint32_t shift) {
    uint32_t masked = value & ~(1u << shift);
    uint32_t fold = masked ^ (masked >> 16);
    fold ^= fold >> 8;
    fold = (fold ^ (fold >> 4)) & 0xF;
    return masked | (((0x28D7 >> fold) & 1) << shift);
}

// VERIFY: 75b1d789 manual
// OFFSET: 0x007FE520
uint32_t Spell_C_EncodeSequenceIntoMissileTrajectory(uint32_t* pitch, uint32_t* speed, uint32_t* source, uint32_t* dest) {
    uint32_t sequence = s_missileTrajectorySequence++;
    uint32_t key[2];
    key[0] = (((source[0] ^ (source[2] & 0xF000)) & 0x2F000) ^ (source[1] & 0x300EE630) ^ (pitch[0] & 0x6000CF0) ^ (dest[1] & 0x60E0400) ^ (speed[0] & 0x240E00) ^ (dest[0] & 0xA9070F01) ^ sequence) | 0x1010101;
    key[1] = 0;
    uint32_t hash = SStrHash(reinterpret_cast<const char*>(key), 1, 0);

    pitch[0] = (pitch[0] & ~0x3u) | ((hash >> 10) & 1) | ((hash >> 27) & 2);
    speed[0] = (speed[0] & ~0x9u) | ((hash >> 1) & 1) | ((hash >> 19) & 8);
    source[0] = (source[0] & ~0x6u) | (((hash & 0x8000) | ((hash >> 9) & 0x10000)) >> 14);
    source[1] ^= ((hash >> 1) ^ source[1]) & 2;
    source[1] ^= ((hash >> 11) ^ source[1]) & 8;
    source[2] ^= ((hash >> 6) ^ source[2]) & 8;
    source[2] ^= ((hash >> 28) ^ source[2]) & 4;
    dest[0] = (dest[0] & ~0x6u) | ((hash >> 6) & 2) | ((hash >> 3) & 4);
    dest[1] ^= ((hash >> 29) ^ dest[1]) & 1;
    dest[1] ^= ((hash >> 3) ^ dest[1]) & 8;
    dest[2] ^= ((hash >> 20) ^ dest[2]) & 1;
    dest[2] ^= ((hash >> 10) ^ dest[2]) & 4;

    pitch[0] = SetTrajectoryCheckBit(pitch[0], 3);
    speed[0] = SetTrajectoryCheckBit(speed[0], 1);
    source[0] = SetTrajectoryCheckBit(source[0], 3);
    source[1] = SetTrajectoryCheckBit(source[1], 2);
    source[2] = SetTrajectoryCheckBit(source[2], 0);
    dest[0] = SetTrajectoryCheckBit(dest[0], 0);
    dest[1] = SetTrajectoryCheckBit(dest[1], 2);
    dest[2] = SetTrajectoryCheckBit(dest[2], 1);
    return dest[2];
}

// VERIFY: 3100171d manual
// OFFSET: 0x0060A600
uint32_t CGPlayer_C::GetNextLevelXP() const {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        return this->m_player->m_playerNextLevelXp;
    }

    return 0;
}

// VERIFY: 0defdfc2 manual
// OFFSET: 0x006CBEC0
int32_t Packet_SMSG_PARTY_COMMAND_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t operation;
    char name[48];
    uint32_t result;
    int32_t value;
    msg->Get(operation);
    msg->GetString(name, sizeof(name));
    msg->Get(result);
    msg->Get(reinterpret_cast<uint32_t&>(value));
    char duration[256];

    if (!result) {
        if (!operation) {
            if (name[0]) {
                CGGameUI::DisplayError(ERR_INVITE_PLAYER_S, name);
            }
        } else if (operation == 2) {
            CGPartyInfo::RemoveAll(1);
            CGGameUI::DisplayError(ERR_LEFT_GROUP_YOU);
            CGLookingForGroup::ClearPartyInfo();
            FrameScript_SignalEvent(SCRIPT_EVENT_CANCEL_SUMMON, nullptr);
        }

        return 1;
    }

    switch (result) {
        case 1: {
            CGGameUI::DisplayError(ERR_BAD_PLAYER_NAME_S, name);
            break;
        }

        case 2: {
            CGGameUI::DisplayError(ERR_TARGET_NOT_IN_GROUP_S, name);
            break;
        }

        case 3: {
            CGGameUI::DisplayError(ERR_TARGET_NOT_IN_INSTANCE_S, name);
            break;
        }

        case 4: {
            CGGameUI::DisplayError(ERR_GROUP_FULL);
            break;
        }

        case 5: {
            CGGameUI::DisplayError(ERR_ALREADY_IN_GROUP_S, name);
            break;
        }

        case 6: {
            CGGameUI::DisplayError(ERR_NOT_IN_GROUP);
            break;
        }

        case 7: {
            CGGameUI::DisplayError(ERR_NOT_LEADER);
            break;
        }

        case 8: {
            CGGameUI::DisplayError(ERR_PLAYER_WRONG_FACTION);
            break;
        }

        case 9: {
            CGGameUI::DisplayError(ERR_IGNORING_YOU_S, name);
            break;
        }

        case 12: {
            CGGameUI::DisplayError(ERR_LFG_PENDING);
            break;
        }

        case 13: {
            CGGameUI::DisplayError(ERR_INVITE_RESTRICTED);
            break;
        }

        case 14: {
            if (operation == 4) {
                CGGameUI::DisplayError(ERR_GROUP_SWAP_FAILED);
                FrameScript_SignalEvent(SCRIPT_EVENT_RAID_ROSTER_UPDATE, nullptr);
            } else {
                CGGameUI::DisplayError(ERR_INVITE_IN_COMBAT);
            }

            break;
        }

        case 15: {
            CGGameUI::DisplayError(ERR_INVITE_UNKNOWN_REALM);
            break;
        }

        case 16: {
            CGGameUI::DisplayError(ERR_INVITE_NO_PARTY_SERVER);
            break;
        }

        case 17: {
            CGGameUI::DisplayError(ERR_INVITE_PARTY_BUSY);
            break;
        }

        case 18: {
            CGGameUI::DisplayError(ERR_PARTY_TARGET_AMBIGUOUS);
            break;
        }

        case 19: {
            CGGameUI::DisplayError(ERR_PARTY_LFG_INVITE_RAID_LOCKED, name);
            break;
        }

        case 20: {
            CGGameUI::DisplayError(ERR_PARTY_LFG_BOOT_LIMIT);
            break;
        }

        case 21: {
            if (value < 0) {
                value = 1;
            }

            CGTooltip::GetDurationString(duration, sizeof(duration), 1000 * value, "INT_SPELL_DURATION", nullptr, 1, 0);
            CGGameUI::DisplayError(ERR_PARTY_LFG_BOOT_COOLDOWN_S, duration);
            break;
        }

        case 22: {
            CGGameUI::DisplayError(ERR_PARTY_LFG_BOOT_IN_PROGRESS);
            break;
        }

        case 23: {
            CGGameUI::DisplayError(ERR_PARTY_LFG_BOOT_TOO_FEW_PLAYERS);
            break;
        }

        case 24: {
            if (value < 0) {
                value = 1;
            }

            CGTooltip::GetDurationString(duration, sizeof(duration), 1000 * value, "INT_SPELL_DURATION", nullptr, 1, 0);
            CGGameUI::DisplayError(ERR_PARTY_LFG_BOOT_NOT_ELIGIBLE_S, duration);
            break;
        }

        case 25: {
            CGGameUI::DisplayError(ERR_RAID_DISALLOWED_BY_LEVEL);
            break;
        }

        case 26: {
            CGGameUI::DisplayError(ERR_PARTY_LFG_BOOT_IN_COMBAT);
            break;
        }

        case 27: {
            FrameScript_SignalEvent(SCRIPT_EVENT_VOTE_KICK_REASON_NEEDED, "%s", name);
            break;
        }

        case 28: {
            CGGameUI::DisplayError(ERR_PARTY_LFG_BOOT_DUNGEON_COMPLETE);
            break;
        }

        case 29: {
            CGGameUI::DisplayError(ERR_PARTY_LFG_BOOT_LOOT_ROLLS);
            break;
        }

        case 30: {
            CGGameUI::DisplayError(ERR_PARTY_LFG_TELEPORT_IN_COMBAT);
            break;
        }
    }

    return 1;
}

// VERIFY: 1f3eed29 auto
// OFFSET: 0x006CC300
int32_t Packet_SMSG_REAL_GROUP_UPDATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t flags;
    uint32_t count;
    WOWGUID leader;
    msg->Get(flags);
    msg->Get(count);
    msg->Get(leader.m_guid);

    if (flags & 0x2) {
        if (count) {
            CGRaidInfo::UpdateRealStats(count + 1, leader);
        } else {
            CGRaidInfo::UpdateRealStats(0, leader);
        }
    } else {
        if (CGRaidInfo::m_realNumMembers) {
            CGRaidInfo::UpdateRealStats(0, leader);
        }

        CGPartyInfo::UpdateRealStats(count, leader);
    }

    Script_SendUniqueSignal(SCRIPT_EVENT_PARTY_MEMBERS_CHANGED);
    return 1;
}

// VERIFY: fdb5b347 auto
// OFFSET: 0x006CC3B0
int32_t Packet_SMSG_GUILD_INVITE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    char guild[96];
    char inviter[48];
    msg->GetString(inviter, sizeof(inviter));
    msg->GetString(guild, sizeof(guild));
    CGGameUI::OpenGuildInvite(inviter, guild);
    CGGameUI::DisplayError(ERR_INVITED_TO_GUILD_SSS, inviter, inviter, guild);
    return 1;
}

// VERIFY: baaff1e8 auto
// OFFSET: 0x006CC440
int32_t Packet_SMSG_GUILD_INFO(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WowTime created;
    char name[96];
    uint32_t numMembers;
    uint32_t numAccounts;
    msg->GetString(name, sizeof(name));
    *msg >> created;
    msg->Get(numMembers);
    msg->Get(numAccounts);
    char buffer[3000];
    SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("GUILD_NAME_TEMPLATE", -1, GENDER_NOT_APPLICABLE), name);
    CGChat::AddChatMessage(buffer, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("GUILD_INFO_TEMPLATE", -1, GENDER_NOT_APPLICABLE), created.m_month + 1, created.m_monthDay + 1, created.m_year + 2000, numMembers, numAccounts);
    CGChat::AddChatMessage(buffer, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    return 1;
}

// VERIFY: 07f58a8c auto
// OFFSET: 0x006CC560
int32_t Packet_MSG_TABARDVENDOR_ACTIVATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID vendor;
    msg->Get(vendor.m_guid);
    CGTabardCreationFrame::Open(vendor);
    return 1;
}

// VERIFY: c2ac9c5f manual
// OFFSET: 0x006CC590
int32_t Packet_SMSG_GUILD_COMMAND_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t command;
    char name[96];
    uint32_t result;
    msg->Get(command);
    msg->GetString(name, sizeof(name));
    msg->Get(result);

    if (!result) {
        switch (command) {
            case 0: {
                CGGameUI::DisplayError(ERR_GUILD_CREATE_S, name);
                break;
            }

            case 1: {
                CGGameUI::DisplayError(ERR_GUILD_INVITE_S, name);
                break;
            }

            case 3: {
                CGAutoCompleteName::RemoveFlagsFromAllNodes(2);
                CGGameUI::DisplayError(ERR_GUILD_QUIT_S, name);
                WOWGUID none = WOWGUID();
                CGGuildBankInfo::SetGuildBanker(none);
                break;
            }

            case 14: {
                CGGameUI::DisplayError(ERR_GUILD_FOUNDER_S, name);
                break;
            }

            case 19:
            case 20: {
                UpdateGuildRoster(1);
                break;
            }
        }

        return 1;
    }

    switch (result) {
        case 1: {
            CGGameUI::DisplayError(ERR_GUILD_INTERNAL);
            break;
        }

        case 2: {
            CGGameUI::DisplayError(ERR_ALREADY_IN_GUILD);
            break;
        }

        case 3: {
            CGGameUI::DisplayError(ERR_ALREADY_IN_GUILD_S, name);
            break;
        }

        case 4: {
            CGGameUI::DisplayError(ERR_INVITED_TO_GUILD);
            break;
        }

        case 5: {
            CGGameUI::DisplayError(ERR_ALREADY_INVITED_TO_GUILD_S, name);
            break;
        }

        case 6: {
            CGGameUI::DisplayNameError(1);
            break;
        }

        case 7: {
            CGGameUI::DisplayError(ERR_GUILD_NAME_EXISTS_S, name);
            break;
        }

        case 8: {
            if (command == 3) {
                CGGameUI::DisplayError(ERR_GUILD_LEADER_LEAVE);
            } else {
                CGGameUI::DisplayError(ERR_GUILD_PERMISSIONS);
            }

            break;
        }

        case 9: {
            CGGameUI::DisplayError(ERR_GUILD_PLAYER_NOT_IN_GUILD);
            break;
        }

        case 10: {
            CGGameUI::DisplayError(ERR_GUILD_PLAYER_NOT_IN_GUILD_S, name);
            break;
        }

        case 11: {
            CGGameUI::DisplayError(ERR_GUILD_PLAYER_NOT_FOUND_S, name);
            break;
        }

        case 12: {
            CGGameUI::DisplayError(ERR_GUILD_NOT_ALLIED);
            break;
        }

        case 13: {
            CGGameUI::DisplayError(ERR_GUILD_RANK_TOO_HIGH_S, name);
            break;
        }

        case 14: {
            CGGameUI::DisplayError(ERR_GUILD_RANK_TOO_LOW_S, name);
            break;
        }

        case 17: {
            CGGameUI::DisplayError(ERR_GUILD_RANKS_LOCKED);
            break;
        }

        case 18: {
            CGGameUI::DisplayError(ERR_GUILD_RANK_IN_USE);
            break;
        }

        case 19: {
            CGGameUI::DisplayError(ERR_IGNORING_YOU_S, name);
            break;
        }

        case 20: {
            if (command == 5) {
                UpdateGuildRoster(1);
            }

            break;
        }

        case 25: {
            CGGameUI::DisplayError(ERR_GUILD_WITHDRAW_LIMIT);
            break;
        }

        case 26: {
            CGGameUI::DisplayError(ERR_GUILD_NOT_ENOUGH_MONEY);
            break;
        }

        case 28: {
            CGGameUI::DisplayError(ERR_GUILD_BANK_FULL);
            break;
        }

        case 29: {
            CGGameUI::DisplayError(ERR_ITEM_NOT_FOUND);
            break;
        }
    }

    return 1;
}

// VERIFY: 9746b975 auto
// OFFSET: 0x006CC980
int32_t Packet_SMSG_ARENA_TEAM_EVENT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t event = 0;
    msg->Get(event);
    uint8_t count;
    msg->Get(count);
    char strings[3][256];

    for (uint8_t i = 0; i < count; i++) {
        msg->GetString(strings[i], sizeof(strings[i]));
    }

    UIErrors error;
    WOWGUID guid;

    switch (event) {
        case 3: {
            if (SStrCmpUTF8I(ClientServices::GetCharacterName(), strings[0], STORM_MAX_STR)) {
                error = ERR_ARENA_TEAM_JOIN_SS;
            } else {
                error = ERR_ARENA_TEAM_YOU_JOIN_S;
                SStrCopy(strings[0], strings[1], sizeof(strings[0]));
                count = 1;
            }

            msg->Get(guid.m_guid);
            break;
        }

        case 4: {
            error = ERR_ARENA_TEAM_LEAVE_SS;
            msg->Get(guid.m_guid);
            break;
        }

        case 5: {
            error = ERR_ARENA_TEAM_REMOVE_SSS;
            break;
        }

        case 6: {
            error = ERR_ARENA_TEAM_LEADER_IS_SS;
            break;
        }

        case 7: {
            error = ERR_ARENA_TEAM_LEADER_CHANGED_SSS;
            break;
        }

        case 8: {
            error = ERR_ARENA_TEAM_DISBANDED_S;
            break;
        }

        default: {
            error = ERR_GUILD_INTERNAL;
            break;
        }
    }

    switch (count) {
        case 1: {
            CGGameUI::DisplayError(error, strings[0]);
            break;
        }

        case 2: {
            CGGameUI::DisplayError(error, strings[0], strings[1]);
            break;
        }

        case 3: {
            CGGameUI::DisplayError(error, strings[0], strings[1], strings[2]);
            break;
        }

        default: {
            CGGameUI::DisplayError(error);
            break;
        }
    }

    for (uint32_t i = 0; i < 3; i++) {
        CGArenaTeamInfo::UpdateRoster(i, 1);
    }

    return 1;
}

// VERIFY: 67b36da5 auto
// OFFSET: 0x006CC410
int32_t Packet_SMSG_GUILD_DECLINE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    char name[48];
    msg->GetString(name, sizeof(name));
    CGGameUI::DisplayError(ERR_GUILD_DECLINE_S, name);
    return 1;
}

// VERIFY: 9b9205f5 auto
// OFFSET: 0x006D6520
void CGPlayer_C::OnLootGameObject(const WOWGUID& guid) {
    this->m_lootTarget = guid;

    if (this->ShouldKneelForLoot()) {
        this->PlayBaseAnimation(ANIM_LOOT, 0);
    }
}

// VERIFY: f3a09cad auto
// OFFSET: 0x006CC910
int32_t Packet_SMSG_ARENA_TEAM_INVITE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    char inviter[48];
    char team[96];
    msg->GetString(inviter, sizeof(inviter));
    msg->GetString(team, sizeof(team));
    FrameScript_SignalEvent(SCRIPT_EVENT_ARENA_TEAM_INVITE_REQUEST, "%s%s", inviter, team);
    CGGameUI::DisplayError(ERR_INVITED_TO_GUILD_SSS, inviter, inviter, team);
    return 1;
}

// VERIFY: 3830e7e6 manual
// OFFSET: 0x006CCE90
int32_t Packet_SMSG_ARENA_ERROR(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t error;
    msg->Get(error);

    if (error == 0) {
        uint8_t type;
        msg->Get(type);
        CGGameUI::DisplayError(ERR_ARENA_NO_TEAM_II, type, type);
    } else if (error == 1) {
        CGGameUI::DisplayError(ERR_ARENA_EXPIRED_CAIS);
    } else if (error == 2) {
        CGGameUI::DisplayError(ERR_LFG_CANT_USE_BATTLEGROUND);
    }

    return 1;
}

// VERIFY: 2d2bd7b2 manual
// OFFSET: 0x006CBD70
int32_t Packet_SMSG_GROUP_SET_LEADER(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    char name[48];
    msg->GetString(name, sizeof(name));

    if (SStrCmpUTF8I(ClientServices::GetCharacterName(), name, STORM_MAX_STR)) {
        if (CGPartyInfo::m_lfgRestricted) {
            CGGameUI::DisplayError(ERR_NEW_GUIDE_S, name);
        } else {
            CGGameUI::DisplayError(ERR_NEW_LEADER_S, name);
        }
    } else {
        if (CGPartyInfo::m_lfgRestricted) {
            CGGameUI::DisplayError(ERR_NEW_GUIDE_YOU);
        } else {
            CGGameUI::DisplayError(ERR_NEW_LEADER_YOU);
        }
    }

    return 1;
}

// VERIFY: f608a6e3 auto
// OFFSET: 0x006CF140
int32_t CGPlayer_C::OnPetitionDecline(CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (name) {
        CGGameUI::DisplayError(ERR_PETITION_DECLINED_S, name->m_name);
    }

    return 1;
}

// VERIFY: 454dfb59 manual
// OFFSET: 0x006CF1A0
int32_t __stdcall CGPlayer_C::OnTurnInPetitionResults(CDataStore* msg) {
    uint32_t result;
    msg->Get(result);

    switch (result) {
        case 0: {
            FrameScript_SignalEvent(SCRIPT_EVENT_GUILD_REGISTRAR_CLOSED, nullptr);
            break;
        }

        case 2: {
            CGGameUI::DisplayError(ERR_PETITION_IN_GUILD);
            break;
        }

        case 4: {
            CGGameUI::DisplayError(ERR_PETITION_NOT_ENOUGH_SIGNATURES);
            break;
        }

        case 11: {
            CGGameUI::DisplayError(ERR_GUILD_PERMISSIONS);
            break;
        }

        default: {
            ConsoleWrite("Petition error", DEFAULT_COLOR);
            break;
        }
    }

    return 1;
}

// VERIFY: fc2ddd1d auto
// OFFSET: 0x006CCB20
int32_t Packet_SMSG_ARENA_TEAM_COMMAND_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t command;
    char team[96];
    char player[48];
    uint32_t error;
    msg->Get(command);
    msg->GetString(team, sizeof(team));
    msg->GetString(player, sizeof(player));
    msg->Get(error);

    if (error == 0) {
        switch (command) {
            case 0: {
                CGGameUI::DisplayError(ERR_ARENA_TEAM_CREATE_S, team);
                break;
            }

            case 1: {
                CGGameUI::DisplayError(ERR_ARENA_TEAM_INVITE_SS, player, team);
                break;
            }

            case 3: {
                CGGameUI::DisplayError(ERR_ARENA_TEAM_QUIT_S, team);
                break;
            }

            case 14: {
                CGGameUI::DisplayError(ERR_ARENA_TEAM_FOUNDER_S, team);
                break;
            }
        }

        return 1;
    }

    switch (error) {
        case 1: {
            CGGameUI::DisplayError(ERR_ARENA_TEAM_INTERNAL);
            break;
        }

        case 2: {
            CGGameUI::DisplayError(ERR_ALREADY_IN_ARENA_TEAM);
            break;
        }

        case 3: {
            CGGameUI::DisplayError(ERR_ALREADY_IN_ARENA_TEAM_S, player);
            break;
        }

        case 4: {
            CGGameUI::DisplayError(ERR_INVITED_TO_ARENA_TEAM);
            break;
        }

        case 5: {
            CGGameUI::DisplayError(ERR_ALREADY_INVITED_TO_ARENA_TEAM_S, player);
            break;
        }

        case 6: {
            CGGameUI::DisplayError(ERR_ARENA_TEAM_NAME_INVALID);
            break;
        }

        case 7: {
            CGGameUI::DisplayError(ERR_ARENA_TEAM_NAME_EXISTS_S, team);
            break;
        }

        case 8: {
            if (command == 3) {
                CGGameUI::DisplayError(ERR_ARENA_TEAM_LEADER_LEAVE_S, team);
            } else {
                CGGameUI::DisplayError(ERR_ARENA_TEAM_PERMISSIONS);
            }

            break;
        }

        case 9: {
            CGGameUI::DisplayError(ERR_ARENA_TEAM_PLAYER_NOT_IN_TEAM);
            break;
        }

        case 10: {
            CGGameUI::DisplayError(ERR_ARENA_TEAM_PLAYER_NOT_IN_TEAM_SS, player, team);
            break;
        }

        case 11: {
            CGGameUI::DisplayError(ERR_ARENA_TEAM_PLAYER_NOT_FOUND_S, player);
            break;
        }

        case 12: {
            CGGameUI::DisplayError(ERR_ARENA_TEAM_NOT_ALLIED);
            break;
        }

        case 19: {
            CGGameUI::DisplayError(ERR_IGNORING_YOU_S, player);
            break;
        }

        case 21: {
            CGGameUI::DisplayError(ERR_ARENA_TEAM_TARGET_TOO_LOW_S, player);
            break;
        }

        case 22: {
            CGGameUI::DisplayError(ERR_ARENA_TEAM_TARGET_TOO_HIGH_S, player);
            break;
        }

        case 23: {
            CGGameUI::DisplayError(ERR_ARENA_TEAM_TOO_MANY_MEMBERS_S, team);
            break;
        }

        case 27: {
            CGGameUI::DisplayError(ERR_ARENA_TEAM_NOT_FOUND);
            break;
        }

        case 30: {
            CGGameUI::DisplayError(ERR_ARENA_TEAMS_LOCKED);
            break;
        }
    }

    return 1;
}

// VERIFY: 068abde1 manual
// OFFSET: 0x006DCA90
bool CGPlayer_C::CanTrackObject(CGObject_C* object) {
    CGGameObject_C* gameObject = static_cast<CGGameObject_C*>(object);

    if (this->IsDeadOrGhost() || (gameObject->m_gameObject->m_typeID != 25 && !gameObject->CanUse())) {
        return false;
    }

    uint32_t trackResources = this->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? this->m_player->m_playerTrackResources : 0;
    LockRec* lock = gameObject->GetLockRec();

    if (!lock) {
        return false;
    }

    for (uint32_t i = 0; i < 8; i++) {
        if (lock->m_type[i] == 2 && ((1 << (static_cast<uint8_t>(lock->m_index[i]) - 1)) & trackResources)) {
            return true;
        }
    }

    return false;
}

// VERIFY: c6a9b523 auto
// OFFSET: 0x006E2240
int32_t Packet_SMSG_SEND_UNLEARN_SPELLS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (!msg) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (msgId != SMSG_SEND_UNLEARN_SPELLS) {
        return 1;
    }

    uint32_t count;
    msg->Get(count);
    CGSpellBook::m_unlearnedSpells.SetCount(count);

    for (int32_t i = 0; i < static_cast<int32_t>(count); i++) {
        uint32_t spellId;
        msg->Get(spellId);
        CGSpellBook::m_unlearnedSpells.m_data[i] = spellId;
    }

    return 1;
}

// VERIFY: 93a358d4 manual
// OFFSET: 0x006D74B0
void __stdcall CGPlayer_C::ReportPVPAFK(WOWGUID guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_REPORT_PVP_PLAYER_AFK));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (name) {
        char buffer[3000];
        SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("REPORT_PVP_AFK_SENT", -1, GENDER_NOT_APPLICABLE), name->m_name);
        CGChat::AddChatMessage(buffer, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }
}

// VERIFY: 6bd01845 manual
// OFFSET: 0x006D7590
bool __stdcall CGPlayer_C::IsRAFPlayer(WOWGUID guid) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        return (player->m_unit->m_unitDynamicFlags >> 6) & 1;
    }

    CGPartyMemberState* stats = CGPartyInfo::GetRemoteStats(guid);

    if (!stats) {
        stats = CGRaidInfo::GetRemoteStats(guid);
    }

    if (stats && (stats->m_flags & 0x100)) {
        return true;
    }

    if (FriendList::s_friendList) {
        FriendInfo* info = FriendList::s_friendList->GetFriend(guid);

        if (info) {
            return (info->m_flags >> 3) & 1;
        }
    }

    return false;
}

// VERIFY: d74d5ba5 auto
// OFFSET: 0x006E6FD0
const CGBag_C* CGPlayer_C::GetBag() const {
    return &this->m_bag;
}

// VERIFY: 66a7ab59 auto
// OFFSET: 0x006E6FD0
CGBag_C* CGPlayer_C::GetBag() {
    return &this->m_bag;
}

// VERIFY: a02355d6 manual
// OFFSET: 0x006D7BB0
int32_t Player_C_ZoneUpdateHandler(const void* data, void* param) {
    if (++s_zoneUpdateCounter >= 10) {
        s_zoneUpdateCounter -= 10;
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (unit || (unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__))) != nullptr) {
            AreaListRegisterLocation(ClntObjMgrGetMapID(), unit->m_worldObject);
        }
    }

    return 1;
}

// VERIFY: b64cbab6 manual
// OFFSET: 0x006DE1C0
bool CGPlayer_C::CanSaveTabardNow() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return false;
    }

    WOWGUID none = WOWGUID();

    if (g_guildInfoCache.GetRecord(player->m_player->m_playerGuildid, none, nullptr, nullptr, false) && !s_tabardSavePending) {
        return true;
    }

    return false;
}

// VERIFY: 042060de manual
// OFFSET: 0x006DE230
const CGItem_C* CGPlayer_C::EquippedItemMeetSpellRequirements(const SpellRec* spell, uint32_t slotMask) {
    if ((this->m_unit->m_unitFieldFlags & 0x200000) || (this->m_unit->m_unitFieldFlags2 & 0x80)) {
        if (this->IsDisarmed(0)) {
            slotMask &= ~0x8000;
        }

        if (this->IsDisarmed(1)) {
            slotMask &= ~0x10000;
        }
    }

    if (this->m_unit->m_unitFieldFlags2 & 0x400) {
        slotMask &= ~0x20000;
    }

    for (int32_t slot = 0; slot < 23; slot++) {
        if (!((1 << slot) & slotMask)) {
            continue;
        }

        CGItem_C* item = this->m_bag.GetItemPointer(slot);

        if (item && item->IsUsable() && spell->m_equippedItemClass == static_cast<int32_t>(item->GetClassID()) && ((1 << item->GetSubtypeID()) & spell->m_equippedItemSubclass)) {
            return item;
        }
    }

    return nullptr;
}

// VERIFY: 6a792bb7 manual
// OFFSET: 0x00578210
int32_t CGPlayer_C::GetModDamageDonePos(int32_t school) const {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        return this->m_player->m_playerFieldModDamageDonePos[school];
    }

    return 0;
}

// VERIFY: 816c9e22 manual
// OFFSET: 0x00578250
int32_t CGPlayer_C::GetModDamageDoneNeg(int32_t school) const {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        return this->m_player->m_playerFieldModDamageDoneNeg[school];
    }

    return 0;
}

// VERIFY: 55676844 manual
// OFFSET: 0x00578290
float CGPlayer_C::GetModDamageDonePct(int32_t school) const {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        return this->m_player->m_playerFieldModDamageDonePct[school];
    }

    return 0.0f;
}

// VERIFY: 68b495c6 manual
// OFFSET: 0x006DC550
bool CGPlayer_C::MeetsItemSetSkill(const ItemSetRec* set) const {
    if (set->m_requiredSkill) {
        int32_t index = this->GetSkillIndex(set->m_requiredSkill);

        if (index < 0 || static_cast<int32_t>(this->GetSkillValue(index)) < set->m_requiredSkillRank) {
            return false;
        }
    }

    return true;
}

// VERIFY: 95312415 manual
// OFFSET: 0x006DE360
InspectItem* CGPlayer_C::GetInspectedItem(int32_t slot) {
    if (s_inspectData && s_inspectData->m_guid == this->m_obj->m_guid) {
        if (slot < 0) {
            return nullptr;
        }

        if (static_cast<uint32_t>(slot) < 19) {
            return &s_inspectData->m_items[slot];
        }
    }

    if (slot < 0 || static_cast<uint32_t>(slot) >= 19) {
        return nullptr;
    }

    VisibleItem_C* visible = &this->m_player->m_playerVisibleItems[slot];

    if (!visible) {
        return nullptr;
    }

    memset(&s_emptyInspectItem, 0, sizeof(s_emptyInspectItem));
    s_emptyInspectItem.m_itemId = abs(visible->m_itemId);
    s_emptyInspectItem.m_enchants[0] = visible->m_enchantment[0];
    s_emptyInspectItem.m_enchants[1] = visible->m_enchantment[1];

    if (visible->m_itemId <= 0) {
        s_emptyInspectItem.m_flags |= 1;
    }

    return &s_emptyInspectItem;
}

// VERIFY: bd15a857 manual
// OFFSET: 0x006DE490
static void CountGemsItemCacheCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->CountEquippedGems();
    }
}

// VERIFY: 456c3dda manual
// OFFSET: 0x006DE4D0
void CGPlayer_C::CountEquippedGems() {
    this->m_gemCounts[0] = 0;
    this->m_gemCounts[1] = 0;
    this->m_gemCounts[2] = 0;
    this->m_gemCounts[3] = 0;

    for (uint32_t slot = 0; slot <= 18; slot++) {
        CGItem_C* item = nullptr;
        InspectItem* inspect = nullptr;

        if (ClntObjMgrGetActivePlayer() == this->m_obj->m_guid) {
            item = this->m_bag.GetItemPointer(slot);

            if (!item || !item->IsUsable()) {
                continue;
            }
        } else {
            inspect = this->GetInspectedItem(slot);

            if (!inspect || !(inspect->m_flags & 1)) {
                continue;
            }
        }

        for (uint32_t socket = 2; socket <= 4; socket++) {
            int32_t enchantId;

            if (ClntObjMgrGetActivePlayer() == this->m_obj->m_guid) {
                enchantId = (item->GetItemStaticFlags(0) & 0x2000) ? 0 : item->m_item->m_itemFieldEnchantment[socket].m_id;
            } else if (socket <= 11) {
                enchantId = inspect->m_enchants[socket];
            } else {
                enchantId = 0;
            }

            SpellItemEnchantmentRec* enchant = g_spellItemEnchantmentDB.GetRecord(enchantId);

            if (!enchant || !enchant->m_srcItemID) {
                continue;
            }

            ItemStats_C* gem = g_itemDBCache.GetRecord(enchant->m_srcItemID, WOWGUID{ 0 }, &CountGemsItemCacheCallback, nullptr, false);

            if (!gem) {
                continue;
            }

            GemPropertiesRec* properties = g_gemPropertiesDB.GetRecord(gem->m_gemProperties);

            if (!properties || !properties->m_type) {
                continue;
            }

            if (properties->m_type & 1) {
                this->m_gemCounts[0]++;
            }

            if (properties->m_type & 2) {
                this->m_gemCounts[1]++;
            }

            if (properties->m_type & 4) {
                this->m_gemCounts[2]++;
            }

            if (properties->m_type & 8) {
                this->m_gemCounts[3]++;
            }
        }
    }
}

// VERIFY: 106512a4 manual
// OFFSET: 0x006D1E20
void __stdcall CGPlayer_C::TalkToGossip(const WOWGUID& guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GOSSIP_HELLO));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 99b0048a manual
// OFFSET: 0x006D1EA0
void __stdcall CGPlayer_C::TalkToQuestUnit(const WOWGUID& guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_QUEST_GIVER_HELLO));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 6f91a651 manual
// OFFSET: 0x006D1F20
void __stdcall CGPlayer_C::TalkToTrainer(const WOWGUID& guid) {
    s_lastTrainerRequest = guid;
    WOWGUID none = { 0 };
    CGClassTrainer::SetTrainer(none, 0);

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_TRAINER_LIST));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 1d12cc15 manual
// OFFSET: 0x006D2120
void CGPlayer_C::TalkToBinder(const WOWGUID& guid) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid ? guid : s_binder, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    C3Vector position = this->GetPosition();
    C3Vector unitPosition = unit->GetPosition();
    float dx = unitPosition.x - position.x;
    float dy = unitPosition.y - position.y;
    float dz = unitPosition.z - position.z;
    float range = unit->m_unit->m_unitFieldCombatreach + 4.0f;

    if (dy * dy + dx * dx + dz * dz > range * range) {
        return;
    }

    if (guid) {
        s_binder = guid;
        AreaTableRec* area = CGGameUI::GetCurrentSubzoneRec();
        const char* name = area ? area->m_areaName : FrameScript_GetText("HOME_INN", -1, GENDER_NOT_APPLICABLE);
        FrameScript_SignalEvent(SCRIPT_EVENT_CONFIRM_BINDER, "%s", name);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BINDER_ACTIVATE));
    msg.Put(s_binder.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 525e9f8e manual
// OFFSET: 0x006D23C0
void __stdcall CGPlayer_C::TalkToGuildBanker(const WOWGUID& guid) {
    if (CGGuildBankInfo::m_banker && CGGuildBankInfo::m_banker == guid) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_BANK_ACTIVATE));
    msg.Put(guid.m_guid);
    msg.Put(static_cast<uint8_t>(CGGuildBankInfo::m_guildBankTabInfo[0].m_needsFullUpdate != 0));
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGGuildBankInfo::m_pendingBanker = guid;
}

// VERIFY: 7ea073a1 manual
// OFFSET: 0x006D2500
void CGPlayer_C::TalkToTabardVendor(const WOWGUID& guid) {
    if (CGTabardCreationFrame::m_vendor == guid) {
        return;
    }

    if (!this->CanEquipItemsInThisForm()) {
        CGGameUI::DisplayError(ERR_EMBLEMERROR_NOTABARDGEOSET);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_TABARDVENDOR_ACTIVATE));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: eb9c79a5 manual
// OFFSET: 0x006DAF60
void __stdcall CGPlayer_C::ShopFromMerchant(const WOWGUID& guid) {
    WOWGUID item = CGGameUI::GetCursorItem();

    if (item) {
        CGPlayer_C::SellItem(guid, item, 0);
        WOWGUID none = { 0 };
        CGGameUI::SetCursorItem(none, none, 0, 0, 0, 1);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LIST_INVENTORY));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: e4ffe0d2 manual
// OFFSET: 0x006DB010
void CGPlayer_C::TalkToSpiritHealer(const WOWGUID& guid) {
    if (!(this->m_player->m_playerFlags & 0x10)) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid ? guid : s_spiritHealer, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    C3Vector position = this->GetPosition();
    C3Vector unitPosition = unit->GetPosition();
    float dx = unitPosition.x - position.x;
    float dy = unitPosition.y - position.y;
    float dz = unitPosition.z - position.z;
    float range = unit->m_unit->m_unitFieldCombatreach + 4.0f;

    if (dy * dy + dx * dx + dz * dz > range * range) {
        return;
    }

    if (guid) {
        s_spiritHealer = guid;
        FrameScript_SignalEvent(SCRIPT_EVENT_CONFIRM_XP_LOSS, "%d", static_cast<int32_t>(static_cast<int32_t>(this->GetNextLevelXP()) * 0.05f));
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SPIRIT_HEALER_ACTIVATE));
    msg.Put(s_spiritHealer.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 25092c88 manual
// OFFSET: 0x006DB180
void CGPlayer_C::TalkToSpiritGuide(const WOWGUID& guid) {
    if (this->m_player->m_playerFlags & 0x10) {
        WOWGUID none = { 0 };
        CGGameUI::SetAreaSpiritHealer(none);
        CGGameUI::SetAreaSpiritHealer(guid);
    }
}

// VERIFY: c270301e manual
// OFFSET: 0x006DD0F0
void CGPlayer_C::HideHelm() {
    if (this->CanEquipItemsInThisForm() && this->m_characterComponent) {
        this->m_characterComponent->RemoveItemBySlot(static_cast<INVENTORY_SLOTS>(0));
    }

    if (!(this->m_player->m_playerFlags & 0x400) && this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_SHOWING_HELM));
        msg.Put(static_cast<uint8_t>(0));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    this->ApplyComponentItemsFromEffects();
}

// VERIFY: 9eabe29a manual
// OFFSET: 0x006DD1B0
void CGPlayer_C::HideCloak() {
    if (this->CanEquipItemsInThisForm() && this->m_characterComponent) {
        this->m_characterComponent->RemoveItemBySlot(static_cast<INVENTORY_SLOTS>(14));
    }

    if (!(this->m_player->m_playerFlags & 0x800) && this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_SHOWING_CLOAK));
        msg.Put(static_cast<uint8_t>(0));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }
}

// VERIFY: b78bf9a5 manual
// OFFSET: 0x006DD270
int32_t CGPlayer_C::OnPetitionRename(CDataStore* msg) {
    WOWGUID guid;
    char name[96];
    msg->Get(guid.m_guid);
    msg->GetString(name, sizeof(name));

    if (!name[0]) {
        return 1;
    }

    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (!item || !(item->GetItemStaticFlags(0) & 0x2000) || !item->m_item->m_itemFieldEnchantment[0].m_id) {
        return 1;
    }

    WOWGUID none = WOWGUID();
    int32_t id = (item->GetItemStaticFlags(0) & 0x2000) ? item->m_item->m_itemFieldEnchantment[0].m_id : 0;
    CGPetition* petition = g_petitionCache.GetRecord(id, none, nullptr, nullptr, false);

    if (petition) {
        SStrCopy(petition->m_title, name, sizeof(petition->m_title));
        FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_SHOW, nullptr);
    }

    return 1;
}

// VERIFY: f6071b71 manual
// OFFSET: 0x006DD360
void GuildCharterTurnInCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->TurnInGuildCharter();
    }
}

// OFFSET: 0x006DD3A0
void CGPlayer_C::TurnInGuildCharter() {
    for (uint32_t i = 23; i <= 38; i++) {
        WOWGUID guid = i < static_cast<uint32_t>(this->m_bag.m_numSlots) ? this->m_bag.m_slots[i] : WOWGUID();

        if (!guid) {
            continue;
        }

        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

        if (!item || !(item->GetItemStaticFlags(0) & 0x2000)) {
            continue;
        }

        int32_t id = (item->GetItemStaticFlags(0) & 0x2000) ? item->m_item->m_itemFieldEnchantment[0].m_id : 0;
        CGPetition* petition = g_petitionCache.GetRecord(id, guid, GuildCharterTurnInCallback, nullptr, false);

        if (!petition) {
            return;
        }

        if (!petition->m_staticType) {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_TURN_IN_PETITION));
            msg.Put(item->m_obj->m_guid.m_guid);
            msg.Finalize();
            ClientServices::Send2(&msg);
            return;
        }
    }

    for (uint32_t i = 19; i <= 22; i++) {
        WOWGUID bagGuid = i < static_cast<uint32_t>(this->m_bag.m_numSlots) ? this->m_bag.m_slots[i] : WOWGUID();
        CGItem_C* container = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(bagGuid, TYPEMASK_ITEM, __FILE__, __LINE__));

        if (!container) {
            continue;
        }

        for (uint32_t j = 0; j < static_cast<uint32_t>(container->GetBag()->m_numSlots); j++) {
            CGBag_C* bag = container->GetBag();
            WOWGUID guid = j < static_cast<uint32_t>(bag->m_numSlots) ? bag->m_slots[j] : WOWGUID();

            if (!guid) {
                continue;
            }

            CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

            if (!item || !(item->GetItemStaticFlags(0) & 0x2000)) {
                continue;
            }

            int32_t id = (item->GetItemStaticFlags(0) & 0x2000) ? item->m_item->m_itemFieldEnchantment[0].m_id : 0;
            CGPetition* petition = g_petitionCache.GetRecord(id, guid, GuildCharterTurnInCallback, nullptr, false);

            if (!petition) {
                return;
            }

            if (!petition->m_staticType) {
                CDataStore msg;
                msg.Put(static_cast<uint32_t>(CMSG_TURN_IN_PETITION));
                msg.Put(item->m_obj->m_guid.m_guid);
                msg.Finalize();
                ClientServices::Send2(&msg);
                return;
            }
        }
    }

    CGGameUI::DisplayError(ERR_NO_GUILD_CHARTER);
}

// VERIFY: 947b57e0 manual
// OFFSET: 0x006DD630
void ArenaCharterTurnInCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && player->TurnInArenaCharter(CGPlayer_C::s_turnInArenaCharter)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_VENDOR_UPDATE, nullptr);
    }
}

// VERIFY: 553471e9 manual
// OFFSET: 0x006DD680
int32_t CGPlayer_C::TurnInArenaCharter(int32_t turnIn) {
    int32_t found = 0;
    CGItem_C* charter = nullptr;
    uint32_t teamSize = CGPetitionVendorInfo::m_arenaTeamSize;
    uint32_t background = CGPetitionVendorInfo::m_arenaBackgroundColor;
    uint32_t emblemStyle = CGPetitionVendorInfo::m_arenaEmblemStyle;
    uint32_t emblemColor = CGPetitionVendorInfo::m_arenaEmblemColor;
    uint32_t borderStyle = CGPetitionVendorInfo::m_arenaBorderStyle;
    uint32_t borderColor = CGPetitionVendorInfo::m_arenaBorderColor;
    CGPlayer_C::s_turnInArenaCharter = turnIn;

    for (uint32_t i = 23; i <= 38 && !found; i++) {
        WOWGUID guid = i < static_cast<uint32_t>(this->m_bag.m_numSlots) ? this->m_bag.m_slots[i] : WOWGUID();

        if (!guid) {
            continue;
        }

        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));
        charter = item;

        if (!item || !(item->GetItemStaticFlags(0) & 0x2000)) {
            continue;
        }

        int32_t id = (item->GetItemStaticFlags(0) & 0x2000) ? item->m_item->m_itemFieldEnchantment[0].m_id : 0;
        CGPetition* petition = g_petitionCache.GetRecord(id, guid, ArenaCharterTurnInCallback, nullptr, false);

        if (petition && petition->m_staticType == 1 && (!turnIn || petition->m_minSignatures == teamSize - 1)) {
            found = 1;
        }
    }

    for (uint32_t i = 19; i <= 22 && !found; i++) {
        WOWGUID bagGuid = i < static_cast<uint32_t>(this->m_bag.m_numSlots) ? this->m_bag.m_slots[i] : WOWGUID();
        CGItem_C* container = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(bagGuid, TYPEMASK_ITEM, __FILE__, __LINE__));

        if (!container) {
            continue;
        }

        for (uint32_t j = 0; j < static_cast<uint32_t>(container->GetBag()->m_numSlots); j++) {
            CGBag_C* bag = container->GetBag();
            WOWGUID guid = j < static_cast<uint32_t>(bag->m_numSlots) ? bag->m_slots[j] : WOWGUID();

            if (!guid) {
                continue;
            }

            CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

            if (!item) {
                continue;
            }

            charter = item;

            if (!(item->GetItemStaticFlags(0) & 0x2000)) {
                continue;
            }

            int32_t id = (item->GetItemStaticFlags(0) & 0x2000) ? item->m_item->m_itemFieldEnchantment[0].m_id : 0;
            CGPetition* petition = g_petitionCache.GetRecord(id, guid, ArenaCharterTurnInCallback, nullptr, false);

            if (petition && petition->m_staticType == 1 && (!turnIn || petition->m_minSignatures == teamSize - 1)) {
                found = 1;
                break;
            }
        }
    }

    if (!turnIn) {
        return found;
    }

    if (!found) {
        CGGameUI::DisplayError(ERR_NO_ARENA_CHARTER);
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_TURN_IN_PETITION));
    msg.Put(charter->m_obj->m_guid.m_guid);
    msg.Put(background);
    msg.Put(emblemStyle);
    msg.Put(emblemColor);
    msg.Put(borderStyle);
    msg.Put(borderColor);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 1;
}

// VERIFY: 856bb8dc manual
// OFFSET: 0x006DDBB0
void CGPlayer_C::HandleOnRightClick(CGUnit_C* unit, bool autoInteract) {
    if (this->IsShapeShifted()) {
        SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(this->GetShapeshiftForm());

        if (!form || !(form->m_flags & 0x8)) {
            if (!this->CanAutoUnshift()) {
                CGGameUI::DisplayError(ERR_CANT_INTERACT_SHAPESHIFTED);
                return;
            }

            this->CancelShapeshift();
        }
    }

    C3Vector unitPosition = unit->GetPosition();
    C3Vector position = this->GetPosition();
    float dx = position.x - unitPosition.x;
    float dy = position.y - unitPosition.y;
    float dz = position.z - unitPosition.z;
    float distanceSq = dz * dz + dy * dy + dx * dx;

    if (autoInteract) {
        float reach = unit->m_unit->m_unitFieldCombatreach + 4.0f;

        if (reach * reach < distanceSq) {
            WOWGUID guid = unit->m_obj->m_guid;

            if (this->SetTrackingTarget(guid, 5)) {
                return;
            }
        }
    }

    float reach = unit->m_unit->m_unitFieldCombatreach + 4.0f;

    if (reach * reach < distanceSq) {
        if (!autoInteract) {
            CGGameUI::DisplayError(ERR_TOO_FAR_TO_INTERACT);
        }

        return;
    }

    if (unit->m_obj->m_guid == CGGameUI::m_interactTarget) {
        return;
    }

    if (unit->m_questStatus == 1) {
        CGTutorial::TriggerTutorial(0x2A);
    }

    WOWGUID guid = unit->m_obj->m_guid;

    if (unit->m_interactIconType == 7) {
        CGPlayer_C::EnableTaxiNode(guid);

        if (!CGUnit_C::UnitCanGossip(unit)) {
            return;
        }
    }

    if (unit->m_unit->m_unitNpcFlags & 0x1) {
        CGPlayer_C::TalkToGossip(guid);
        this->RequestTalkEmote(0);
        return;
    }

    if (CGUnit_C::UnitCanGossip(unit)) {
        CGPlayer_C::TalkToQuestUnit(guid);
        this->RequestTalkEmote(0);
        return;
    }

    uint32_t npcFlags = unit->m_unit->m_unitNpcFlags;

    if (npcFlags & 0x2000) {
        CGPlayer_C::QueryTaxiNodes(guid);
        this->RequestTalkEmote(0);
    } else if (npcFlags & 0x80) {
        CGPlayer_C::ShopFromMerchant(guid);
        this->RequestTalkEmote(0);
    } else if (npcFlags & 0x10) {
        CGPlayer_C::TalkToTrainer(guid);
        this->RequestTalkEmote(0);
    } else if (npcFlags & 0x4000) {
        this->TalkToSpiritHealer(guid);
        this->RequestTalkEmote(0);
    } else if (npcFlags & 0x8000) {
        this->TalkToSpiritGuide(guid);
        this->RequestTalkEmote(0);
    } else if (npcFlags & 0x10000) {
        this->TalkToBinder(guid);
        this->RequestTalkEmote(0);
    } else if (npcFlags & 0x20000) {
        CGPlayer_C::TalkToBanker(guid);
        this->RequestTalkEmote(0);
    } else if (unit->IsGuildMaster()) {
        CGPlayer_C::TalkToPetitionVendor(guid);
        this->RequestTalkEmote(0);
    } else if (npcFlags & 0x40000) {
        CGPlayer_C::TalkToPetitionVendor(guid);
        this->RequestTalkEmote(0);
    } else if (npcFlags & 0x80000) {
        this->TalkToTabardVendor(guid);
        this->RequestTalkEmote(0);
    } else if (npcFlags & 0x100000) {
        CGPlayer_C::TalkToBattleMaster(guid);
        this->RequestTalkEmote(0);
    } else if (npcFlags & 0x200000) {
        CGPlayer_C::TalkToAuctioneer(guid);
        this->RequestTalkEmote(0);
    } else if (npcFlags & 0x400000) {
        CGPlayer_C::TalkToStableMaster(guid);
        this->RequestTalkEmote(0);
    } else if (npcFlags & 0x800000) {
        CGPlayer_C::TalkToGuildBanker(guid);
        this->RequestTalkEmote(0);
    } else if (unit->IsInPartyOrRaid()) {
        CGPlayer_C::TalkToSpellClick(guid);
    } else if (unit->CanBeSpellClickedByLocalPlayer(this)) {
        CGPlayer_C::TalkToRideVehicle(guid);
    } else if (unit->m_unit->m_unitNpcFlags & 0x4000000) {
        CGPlayer_C::TalkToMailbox(unit->m_obj->m_guid);
    }
}

// VERIFY: 2014c9e0 manual
// OFFSET: 0x006D1760
void CGPlayer_C::RequestAllItemPurchaseData() {
    for (int32_t i = 0; i <= 18; i++) {
        if (static_cast<uint32_t>(i) >= static_cast<uint32_t>(this->m_bag.m_numSlots) || !this->m_bag.m_slots[i]) {
            continue;
        }

        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(this->m_bag.m_slots[i], TYPEMASK_ITEM, __FILE__, __LINE__));

        if (item && !item->m_itemPurchaseData && !(item->m_itemFlags & 0x4) && item->GetStats() && (item->GetStats()->m_flags[0] & 0x1000)) {
            item->RequestItemPurchaseData();
        }
    }

    for (int32_t i = 23; i <= 38; i++) {
        if (static_cast<uint32_t>(i) >= static_cast<uint32_t>(this->m_bag.m_numSlots) || !this->m_bag.m_slots[i]) {
            continue;
        }

        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(this->m_bag.m_slots[i], TYPEMASK_ITEM, __FILE__, __LINE__));

        if (item && !item->m_itemPurchaseData && !(item->m_itemFlags & 0x4) && item->GetStats() && (item->GetStats()->m_flags[0] & 0x1000)) {
            item->RequestItemPurchaseData();
        }
    }

    for (int32_t i = 19; i <= 22; i++) {
        WOWGUID guid = static_cast<uint32_t>(i) < static_cast<uint32_t>(this->m_bag.m_numSlots) ? this->m_bag.m_slots[i] : WOWGUID();
        CGItem_C* container = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

        if (!container) {
            continue;
        }

        for (uint32_t j = 0; j < static_cast<uint32_t>(container->GetBag()->m_numSlots); j++) {
            CGBag_C* bag = container->GetBag();

            if (j >= static_cast<uint32_t>(bag->m_numSlots) || !bag->m_slots[j]) {
                continue;
            }

            CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(bag->m_slots[j], TYPEMASK_ITEM, __FILE__, __LINE__));

            if (item && !item->m_itemPurchaseData && !(item->m_itemFlags & 0x4) && item->GetStats() && (item->GetStats()->m_flags[0] & 0x1000)) {
                item->RequestItemPurchaseData();
            }
        }
    }
}

// VERIFY: 1167de4b manual
// OFFSET: 0x006D9960
int32_t CGPlayer_C::OnListInventory(CDataStore* msg) {
    for (int32_t i = 0; i < 150; i++) {
        s_vendorItems[i].m_slot = 0;
    }

    WOWGUID vendor;
    msg->Get(vendor.m_guid);

    uint8_t count;
    msg->Get(count);

    if (count > 150) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    CGPlayer_C::s_lastVendorListReceived = vendor;
    uint8_t error = 0xFF;

    if (count) {
        for (uint8_t i = 0; i < count; i++) {
            MerchantItem& item = s_vendorItems[i];
            msg->Get(reinterpret_cast<uint32_t&>(item.m_slot));
            msg->Get(reinterpret_cast<uint32_t&>(item.m_itemID));
            msg->Get(reinterpret_cast<uint32_t&>(item.m_displayID));
            msg->Get(reinterpret_cast<uint32_t&>(item.m_maxCount));
            msg->Get(reinterpret_cast<uint32_t&>(item.m_price));
            msg->Get(reinterpret_cast<uint32_t&>(item.m_durability));
            msg->Get(reinterpret_cast<uint32_t&>(item.m_buyCount));
            msg->Get(reinterpret_cast<uint32_t&>(item.m_extendedCostID));
        }
    } else {
        msg->Get(error);

        switch (error) {
            case 0:
                ConsoleWrite("Vendor has no inventory", DEFAULT_COLOR);
                break;

            case 1:
                ConsoleWrite("I don't think he likes you very much", DEFAULT_COLOR);
                break;

            case 2:
                ConsoleWrite("You are too far away", DEFAULT_COLOR);
                break;

            case 3:
                ConsoleWrite("Vendor is dead", DEFAULT_COLOR);
                break;

            case 4:
                ConsoleWrite("You can't shop while dead.", DEFAULT_COLOR);
                break;

            default:
                break;
        }
    }

    if (count || !error) {
        this->RequestAllItemPurchaseData();
        CGMerchantInfo::SetMerchant(vendor, s_vendorItems, count);
    }

    return 1;
}

// VERIFY: 56e2df00 manual
// OFFSET: 0x006D0460
int32_t __stdcall CGPlayer_C::OnQuestGiverQuestDetails(CDataStore* msg) {
    WOWGUID npc;
    msg->Get(npc.m_guid);

    WOWGUID inform;
    msg->Get(inform.m_guid);

    uint32_t questId;
    msg->Get(questId);

    char title[512];
    msg->GetString(title, sizeof(title));

    char details[3000];
    msg->GetString(details, sizeof(details));

    char objectives[3000];
    msg->GetString(objectives, sizeof(objectives));

    uint8_t autoLaunched;
    msg->Get(autoLaunched);

    uint32_t flags;
    msg->Get(flags);

    uint32_t suggestedGroupNum;
    msg->Get(suggestedGroupNum);

    uint8_t startCheat;
    msg->Get(startCheat);

    uint32_t numChoices;
    msg->Get(numChoices);

    uint32_t choiceItems[6] = { 0 };
    uint32_t choiceDisplays[6] = { 0 };
    uint32_t choiceCounts[6];

    for (int32_t i = 0; i < static_cast<int32_t>(numChoices); i++) {
        msg->Get(choiceItems[i]);
        msg->Get(choiceCounts[i]);
        msg->Get(choiceDisplays[i]);
    }

    uint32_t numRewards;
    msg->Get(numRewards);

    uint32_t rewardItems[4] = { 0 };
    uint32_t rewardDisplays[4] = { 0 };
    uint32_t rewardCounts[4];

    for (int32_t i = 0; i < static_cast<int32_t>(numRewards); i++) {
        msg->Get(rewardItems[i]);
        msg->Get(rewardCounts[i]);
        msg->Get(rewardDisplays[i]);
    }

    uint32_t money;
    msg->Get(money);

    uint32_t xp;
    msg->Get(xp);

    uint32_t honor;
    msg->Get(honor);

    float honorMultiplier;
    msg->Get(honorMultiplier);

    uint32_t spell;
    msg->Get(spell);

    uint32_t spellCast;
    msg->Get(spellCast);

    uint32_t rewardTitle;
    msg->Get(rewardTitle);

    uint32_t talents;
    msg->Get(talents);

    uint32_t arenaPoints;
    msg->Get(arenaPoints);

    uint32_t factionMask;
    msg->Get(factionMask);

    uint32_t factionIDs[5];

    for (uint32_t i = 0; i < 5; i++) {
        msg->Get(factionIDs[i]);
    }

    uint32_t factionValues[5];

    for (uint32_t i = 0; i < 5; i++) {
        msg->Get(factionValues[i]);
    }

    uint32_t factionOverrides[5];

    for (uint32_t i = 0; i < 5; i++) {
        msg->Get(factionOverrides[i]);
    }

    uint32_t emoteCount;
    msg->Get(emoteCount);

    QUESTGIVEREMOTEQUEUE emotes;
    emotes.m_nodes = static_cast<QUESTGIVEREMOTENODE*>(alloca(emoteCount * sizeof(QUESTGIVEREMOTENODE)));
    emotes.m_count = emoteCount;

    for (int32_t i = 0; i < static_cast<int32_t>(emoteCount); i++) {
        msg->Get(reinterpret_cast<uint32_t&>(emotes.m_nodes[i].m_emote));
        msg->Get(emotes.m_nodes[i].m_delay);
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(npc, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit) {
        if (!(unit->m_obj->m_type & TYPEMASK_PLAYER)) {
            unit->PlayEmotes(emotes);
        }

        if ((unit->m_obj->m_type & TYPEMASK_PLAYER) && CGQuestInfo::m_npc) {
            CDataStore response;
            response.Put(static_cast<uint32_t>(MSG_QUEST_PUSH_RESULT));
            response.Put(inform ? inform.m_guid : npc.m_guid);
            response.Put(questId);
            response.Put(static_cast<uint8_t>(4));
            response.Finalize();
            ClientServices::Send2(&response);
            return 1;
        }
    }

    CGQuestInfo::SetState(npc, inform, 1, details, questId, startCheat);
    CGQuestInfo::SetLogDescription(objectives);
    CGQuestInfo::AddReward(title, choiceItems, choiceDisplays, choiceCounts, numChoices, rewardItems, rewardDisplays, rewardCounts, numRewards, money, xp, honor, honorMultiplier, spell, spellCast, autoLaunched, suggestedGroupNum, rewardTitle, talents, arenaPoints, factionMask, factionIDs, factionValues, factionOverrides, flags);
    return 1;
}

// VERIFY: ba5bd839 manual
// OFFSET: 0x006D2DE0
void CGPlayer_C::BuyItem(WOWGUID vendor, const MerchantItem* item, uint32_t count, uint8_t a5) {
    if (!item) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BUY_ITEM));
    msg.Put(vendor.m_guid);
    msg.Put(static_cast<uint32_t>(item->m_itemID));
    msg.Put(static_cast<uint32_t>(item->m_slot));
    msg.Put(count);
    msg.Put(a5);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 604e7050 manual
// OFFSET: 0x006DC3F0
int32_t CGPlayer_C::CanUseItem(const ItemStats_C* stats, int32_t& error) {
    error = ERR_MAX_VALUE;

    if (!stats) {
        error = ERR_ITEM_NOT_FOUND;
        return 0;
    }

    CGUnitData* unit = this->m_unit;

    if (static_cast<int32_t>(stats->m_requiredLevel) > static_cast<int32_t>(unit->m_unitFieldLevel) || !((1 << (unit->m_unitFieldBytes0[1] - 1)) & stats->m_allowableClass) || !((1 << (unit->m_unitFieldBytes0[0] - 1)) & stats->m_allowableRace)) {
        error = ERR_CANT_USE_ITEM;
        return 0;
    }

    uint8_t itemClass = static_cast<uint8_t>(stats->m_class);

    if (itemClass < 17 && s_proficiency[itemClass] && !(s_proficiency[itemClass] & (1 << stats->m_subClass))) {
        error = ERR_PROFICIENCY_NEEDED;
        return 0;
    }

    if (stats->m_requiredSkill && !(stats->m_flags[0] & 0x20040000)) {
        int32_t index = this->GetSkillIndex(stats->m_requiredSkill);

        if (index < 0 || static_cast<int32_t>(this->GetSkillValue(index)) < static_cast<int32_t>(stats->m_requiredSkillRank)) {
            error = ERR_CANT_USE_ITEM;
            return 0;
        }
    }

    if (stats->m_requiredSpell && !this->IsSpellKnown(stats->m_requiredSpell) && !this->IsSpellSuperceded(stats->m_requiredSpell)) {
        error = ERR_CANT_USE_ITEM;
        return 0;
    }

    if (stats->m_requiredHonorRank && static_cast<int32_t>(this->m_player->m_playerFieldBytes[3]) < static_cast<int32_t>(stats->m_requiredHonorRank)) {
        error = ERR_CANT_USE_ITEM;
        return 0;
    }

    if (stats->m_requiredCityRank && !this->HasPvpMedal(stats->m_requiredCityRank)) {
        error = ERR_CANT_USE_ITEM;
        return 0;
    }

    if (stats->m_requiredReputationFaction && GetRepListRepValue(stats->m_requiredReputationFaction) < CGReputationInfo::m_reactionThresholds[stats->m_requiredReputationRank]) {
        error = ERR_CANT_USE_ITEM;
        return 0;
    }

    return 1;
}

// VERIFY: c1869d85 manual
// OFFSET: 0x006D0AB0
int32_t __stdcall CGPlayer_C::OnQuestGiverOfferReward(CDataStore* msg) {
    WOWGUID npc;
    msg->Get(npc.m_guid);

    uint32_t questId;
    msg->Get(questId);

    char title[512];
    msg->GetString(title, sizeof(title));

    char text[3000];
    msg->GetString(text, sizeof(text));

    uint8_t autoLaunched;
    msg->Get(autoLaunched);

    uint32_t flags;
    msg->Get(flags);

    uint32_t suggestedGroupNum;
    msg->Get(suggestedGroupNum);

    uint32_t emoteCount;
    msg->Get(emoteCount);

    QUESTGIVEREMOTEQUEUE emotes;
    emotes.m_nodes = static_cast<QUESTGIVEREMOTENODE*>(alloca(emoteCount * sizeof(QUESTGIVEREMOTENODE)));
    emotes.m_count = emoteCount;

    for (int32_t i = 0; i < static_cast<int32_t>(emoteCount); i++) {
        msg->Get(emotes.m_nodes[i].m_delay);
        msg->Get(reinterpret_cast<uint32_t&>(emotes.m_nodes[i].m_emote));
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(npc, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->PlayEmotes(emotes);
    }

    uint32_t numChoices;
    msg->Get(numChoices);

    uint32_t choiceItems[6] = { 0 };
    uint32_t choiceDisplays[6] = { 0 };
    uint32_t choiceCounts[6];

    for (int32_t i = 0; i < static_cast<int32_t>(numChoices); i++) {
        msg->Get(choiceItems[i]);
        msg->Get(choiceCounts[i]);
        msg->Get(choiceDisplays[i]);
    }

    uint32_t numRewards;
    msg->Get(numRewards);

    uint32_t rewardItems[4] = { 0 };
    uint32_t rewardDisplays[4] = { 0 };
    uint32_t rewardCounts[4];

    for (int32_t i = 0; i < static_cast<int32_t>(numRewards); i++) {
        msg->Get(rewardItems[i]);
        msg->Get(rewardCounts[i]);
        msg->Get(rewardDisplays[i]);
    }

    uint32_t money;
    msg->Get(money);

    uint32_t xp;
    msg->Get(xp);

    uint32_t honor;
    msg->Get(honor);

    float honorMultiplier;
    msg->Get(honorMultiplier);

    uint32_t unknown;
    msg->Get(unknown);

    uint32_t spell;
    msg->Get(spell);

    uint32_t spellCast;
    msg->Get(spellCast);

    uint32_t rewardTitle;
    msg->Get(rewardTitle);

    uint32_t talents;
    msg->Get(talents);

    uint32_t arenaPoints;
    msg->Get(arenaPoints);

    uint32_t factionMask;
    msg->Get(factionMask);

    uint32_t factionIDs[5];

    for (uint32_t i = 0; i < 5; i++) {
        msg->Get(factionIDs[i]);
    }

    uint32_t factionValues[5];

    for (uint32_t i = 0; i < 5; i++) {
        msg->Get(factionValues[i]);
    }

    uint32_t factionOverrides[5];

    for (uint32_t i = 0; i < 5; i++) {
        msg->Get(factionOverrides[i]);
    }

    WOWGUID none = { 0 };
    CGQuestInfo::SetState(npc, none, 3, text, questId, 0);
    CGQuestInfo::AddReward(title, choiceItems, choiceDisplays, choiceCounts, numChoices, rewardItems, rewardDisplays, rewardCounts, numRewards, money, xp, honor, honorMultiplier, spell, spellCast, autoLaunched, suggestedGroupNum, rewardTitle, talents, arenaPoints, factionMask, factionIDs, factionValues, factionOverrides, flags);
    return 1;
}

// VERIFY: 2d06226f manual
// OFFSET: 0x007246A0
float CGPlayer_C::GetQuestXPMultiplier() {
    float multiplier = 1.0f;

    if (!(this->m_auraEffectMask[291 >> 3] & (1 << (291 & 7)))) {
        return multiplier;
    }

    SpellRec spell;

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        if (!g_spellDB.GetRecord(this->GetAura(i)->m_spellId, &spell)) {
            continue;
        }

        for (uint32_t j = 0; j < 3; j++) {
            if (spell.m_effectAura[j] == 291) {
                multiplier = (spell.m_effectBasePoints[j] + 1) * 0.0099999998f + multiplier;
            }
        }
    }

    return multiplier;
}

// VERIFY: 6e33a08e auto
// OFFSET: 0x006DF2B0
static void QuestAcceptCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(id, none, nullptr, nullptr, false);

    if (!quest) {
        return;
    }

    CGGameUI::DisplayError(ERR_QUEST_ACCEPTED_S, quest->m_name);

    if (quest->m_infoId == 1) {
        CGTutorial::TriggerTutorial(0x28);
        CGTutorial::TriggerTutorial(0x26);
    }

    if (!IsTutorialFlagged(0x14) && ++s_questsAccepted >= 4) {
        CGTutorial::TriggerTutorial(0x14);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_ACCEPTED, "%d", CGQuestLog::GetQuestIndex(quest->m_id));
}

// VERIFY: ac232887 manual
// OFFSET: 0x006DF370
int32_t OnUpdateQuest(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 1;
    }

    if (player->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CGPlayer_C::QueryQuestGiverStatusMultiple();
    }

    CGQuestLog::Update(1);
    uint32_t slot = (offset - 0x28) / sizeof(PlayerQuestLogEntry);
    PlayerQuestLogEntry* entry = slot > 25 ? nullptr : &player->m_player->m_playerQuestLog[slot];
    const PlayerQuestLogEntry* old = static_cast<const PlayerQuestLogEntry*>(value);
    uint32_t questId = entry->m_questId;
    WOWGUID none = { 0 };

    if (questId && (questId != old->m_questId || (!(entry->m_state & 2) && (old->m_state & 2)))) {
        QuestCache* quest = g_questDBCache.GetRecord(questId, none, QuestAcceptCallback, nullptr, false);

        if (quest) {
            CGGameUI::DisplayError(ERR_QUEST_ACCEPTED_S, quest->m_name);

            if (quest->m_infoId == 1) {
                CGTutorial::TriggerTutorial(0x28);
                CGTutorial::TriggerTutorial(0x26);
            }

            if (!IsTutorialFlagged(0x14) && ++s_questsAccepted >= 4) {
                CGTutorial::TriggerTutorial(0x14);
            }

            FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_ACCEPTED, "%d", CGQuestLog::GetQuestIndex(quest->m_id));
        }
    }

    QuestCache* quest = g_questDBCache.GetRecord(entry->m_questId, none, nullptr, nullptr, false);

    if (quest) {
        int32_t index = CGQuestLog::GetQuestIndex(quest->m_id) - 1;

        if (index >= 0 && (quest->m_flags & 0x10000) && CGQuestLog::IsQuestCompleteEx(index, 1)) {
            WOWGUID self = player->m_obj->m_guid;
            CGPlayer_C::CompleteQuest(self, quest->m_id);
        }
    }

    if (old && old->m_questId && entry->m_questId != old->m_questId) {
        RemoveQuestPointDisplay(old->m_questId);
    }

    return 1;
}

// VERIFY: 1103b817 manual
// OFFSET: 0x006E5180
void CGPlayer_C::SetActiveMirrorHandlers() {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_UNIT, 0x10, 8, CritterUpdate, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);

    for (uint32_t i = 0x2C0; i < 0x378; i += 8) {
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, 8, InventoryUpdate, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    }

    for (uint32_t i = 0x3F8; i <= 0x4D0; i += 8) {
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, 8, InventoryUpdate, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    }

    for (uint32_t i = 0x4D8; i <= 0x508; i += 8) {
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, 8, InventoryUpdate, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    }

    for (uint32_t i = 0x570; i <= 0x668; i += 8) {
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, 8, InventoryUpdate, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    }

    for (uint32_t i = 0x670; i <= 0x768; i += 8) {
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, 8, InventoryUpdate, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    }

    for (uint32_t i = 0x28; i < 0x21C; i += 20) {
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, 20, OnUpdateQuest, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    }

    for (uint32_t i = 0x11B0; i < 0x1214; i += 4) {
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, 4, ReportDailyQuestsRemaining, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    }

    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0xFF8, 4, OnUpdateMoney, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_UNIT, 0x18, 8, CharmChangeHandler, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_UNIT, 0x1D3, 1, OnUpdateShapeshiftForm, nullptr, static_cast<HANDLER_PRIORITY>(0), 1);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x770, 8, FarsightChangeHandler, nullptr, static_cast<HANDLER_PRIORITY>(0), 1);

    for (uint32_t i = 0x778; i < 0x790; i += 8) {
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, 8, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_KNOWN_TITLES_UPDATE)), static_cast<HANDLER_PRIORITY>(0), 0);
    }

    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x790, 8, SignalKnownCurrencyTypesUpdate, nullptr, static_cast<HANDLER_PRIORITY>(0), 1);

    for (uint32_t i = 0x7A6; i < 0xDA6; i += 12) {
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i - 2, 2, SkillRankChangeHandler, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, 2, SkillMaxRankChangeHandler, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i + 4, 2, SkillMaxRankChangeHandler, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i + 2, 2, SignalPlayerSkillStatEventsAndRefresh, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    }

    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1B, 1, OnUpdateRestState, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0xFF4, 4, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_UPDATE_EXHAUSTION)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x798, 4, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_XP_UPDATE)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x79C, 4, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_XP_UPDATE)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0xFFC, 0x1C, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1018, 0x1C, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1034, 0x1C, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1050, 4, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10D4, 2, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_PVP_KILLS_CHANGED)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10E0, 4, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_PVP_KILLS_CHANGED)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10D8, 4, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_PVP_KILLS_CHANGED)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1C, 4, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_PVP_RANK_CHANGED)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10E7, 1, OnUpdatePlayerAuraVision, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10E4, 2, OnUpdatePlayerOverrideSpellsID, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10E8, 4, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_UPDATE_FACTION)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1150, 0x54, OnArenaTeamsChanged, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x11A4, 4, reinterpret_cast<MIRRORHANDLERFUNC>(OnHonorCurrencyChanged), nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x11A8, 4, reinterpret_cast<MIRRORHANDLERFUNC>(OnArenaCurrencyChanged), nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10EC, 0x64, SignalCombatRatingUpdate, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0xDEC, 4, SignalCombatRatingUpdate, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0xDB0, 0x34, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_UNIT, 0x88, 0x38, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)), static_cast<HANDLER_PRIORITY>(0), 0);

    for (uint32_t i = 0x1214; i < 0x1224; i += 4) {
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, 4, OnUpdatePlayerRuneRegen, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    }

    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0xDAC, 4, OnUpdatePlayerTrackingMask, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1060, 4, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)), static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x105C, 4, SignalCombatRatingUpdate, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1224, 0xC, OnUpdatePlayerNoReagentCost, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);

    for (uint32_t i = 0x1248; i < 0x1260; i += 4) {
        ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, 4, reinterpret_cast<MIRRORHANDLERFUNC>(OnUpdatePlayerGlyph), nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    }

    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1260, 4, reinterpret_cast<MIRRORHANDLERFUNC>(OnUpdatePlayerGlyphSlotsEnabled), nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1264, 4, reinterpret_cast<MIRRORHANDLERFUNC>(SignalPetSpellPowerUpdate), nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
}

// VERIFY: 7c2e2f8b manual
// OFFSET: 0x006E5960
void CGPlayer_C::UnsetActiveMirrorHandlers() {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_UNIT, 0x10, CritterUpdate, nullptr);

    for (uint32_t i = 0x2C0; i < 0x378; i += 8) {
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, InventoryUpdate, nullptr);
    }

    for (uint32_t i = 0x3F8; i <= 0x4D0; i += 8) {
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, InventoryUpdate, nullptr);
    }

    for (uint32_t i = 0x4D8; i <= 0x508; i += 8) {
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, InventoryUpdate, nullptr);
    }

    for (uint32_t i = 0x570; i <= 0x668; i += 8) {
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, InventoryUpdate, nullptr);
    }

    for (uint32_t i = 0x670; i <= 0x768; i += 8) {
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, InventoryUpdate, nullptr);
    }

    for (uint32_t i = 0x28; i < 0x21C; i += 20) {
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, OnUpdateQuest, nullptr);
    }

    for (uint32_t i = 0x11B0; i < 0x1214; i += 4) {
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, ReportDailyQuestsRemaining, nullptr);
    }

    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0xFF8, OnUpdateMoney, nullptr);
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_UNIT, 0x18, CharmChangeHandler, nullptr);
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x770, FarsightChangeHandler, nullptr);

    for (uint32_t i = 0x778; i < 0x790; i += 8) {
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_KNOWN_TITLES_UPDATE)));
    }

    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x790, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_KNOWN_CURRENCY_TYPES_UPDATE)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_UNIT, 0x1D3, OnUpdateShapeshiftForm, nullptr);

    for (uint32_t i = 0x7A6; i < 0xDA6; i += 12) {
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i - 2, SkillRankChangeHandler, nullptr);
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, SkillMaxRankChangeHandler, nullptr);
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i + 4, SkillMaxRankChangeHandler, nullptr);
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i + 2, SignalPlayerSkillStatEventsAndRefresh, nullptr);
    }

    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1B, OnUpdateRestState, nullptr);
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0xFF4, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_UPDATE_EXHAUSTION)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x798, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_XP_UPDATE)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x79C, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_XP_UPDATE)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0xFFC, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1018, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1034, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1050, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10D4, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_PVP_KILLS_CHANGED)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10E0, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_PVP_KILLS_CHANGED)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10D8, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_PVP_KILLS_CHANGED)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1C, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_PVP_RANK_CHANGED)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10E7, OnUpdatePlayerAuraVision, nullptr);
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10E4, OnUpdatePlayerOverrideSpellsID, nullptr);
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10E8, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_UPDATE_FACTION)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1150, OnArenaTeamsChanged, nullptr);
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x11A4, reinterpret_cast<MIRRORHANDLERFUNC>(OnHonorCurrencyChanged), nullptr);
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x11A8, reinterpret_cast<MIRRORHANDLERFUNC>(OnArenaCurrencyChanged), nullptr);
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x10EC, SignalCombatRatingUpdate, nullptr);
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0xDEC, SignalCombatRatingUpdate, nullptr);
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0xDB0, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x88, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)));

    for (uint32_t i = 0x1214; i < 0x1224; i += 4) {
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, OnUpdatePlayerRuneRegen, nullptr);
    }

    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0xDAC, OnUpdatePlayerTrackingMask, nullptr);
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1060, OnUpdatePlayerScriptEvent, reinterpret_cast<void*>(static_cast<uintptr_t>(SCRIPT_EVENT_PLAYER_DAMAGE_DONE_MODS)));
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x105C, SignalCombatRatingUpdate, nullptr);
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1224, OnUpdatePlayerNoReagentCost, nullptr);

    for (uint32_t i = 0x1248; i < 0x1260; i += 4) {
        ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, i, reinterpret_cast<MIRRORHANDLERFUNC>(OnUpdatePlayerGlyph), nullptr);
    }

    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1260, reinterpret_cast<MIRRORHANDLERFUNC>(OnUpdatePlayerGlyphSlotsEnabled), nullptr);
    ClntObjMgrUnsetObjMirrorHandler(this->m_obj->m_guid, ID_PLAYER, 0x1264, reinterpret_cast<MIRRORHANDLERFUNC>(SignalPetSpellPowerUpdate), nullptr);
}

struct QuestCompleteRewards {
    /* 0000 */ int32_t m_xp;
    /* 0004 */ int32_t m_money;
    /* 0008 */ int32_t m_honor;
    /* 000C */ int32_t m_talents;
    /* 0010 */ int32_t m_arenaPoints;
};

// VERIFY: fcb2dc3e manual
// OFFSET: 0x006CFFF0
static void CrossedInebriationThresholdItemCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    int32_t threshold = static_cast<int32_t>(reinterpret_cast<intptr_t>(param));
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    char token[32];
    char buffer[3000];
    WOWGUID none = WOWGUID();
    ItemStats_C* stats;

    if (id && (stats = g_itemDBCache.GetRecord(id, none, nullptr, nullptr, false)) != nullptr) {
        if (player->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
            SStrPrintf(token, sizeof(token), "DRUNK_MESSAGE_ITEM_OTHER%d", threshold + 1);
            const char* link = CGTooltip::GetItemHyperlinkString(id, stats->m_quality, 0, 0, 0);
            SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), player->GetUnitName(nullptr, 1), link);
            CGChat::AddChatMessage(buffer, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            return;
        }

        SStrPrintf(token, sizeof(token), "DRUNK_MESSAGE_ITEM_SELF%d", threshold + 1);
        const char* link = CGTooltip::GetItemHyperlinkString(id, stats->m_quality, 0, 0, 0);
        SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), link);
    } else {
        if (player->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            SStrPrintf(token, sizeof(token), "DRUNK_MESSAGE_SELF%d", threshold + 1);
        } else {
            SStrPrintf(token, sizeof(token), "DRUNK_MESSAGE_OTHER%d", threshold + 1);
        }

        SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), player->GetUnitName(nullptr, 1));
    }

    CGChat::AddChatMessage(buffer, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
}

// VERIFY: 63353f77 manual
// OFFSET: 0x006D01B0
int32_t Packet_SMSG_CROSSED_INEBRIATION_THRESHOLD(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    uint32_t threshold;
    uint32_t itemId;
    msg->Get(guid.m_guid);
    msg->Get(threshold);
    msg->Get(itemId);

    if (!itemId) {
        CrossedInebriationThresholdItemCallback(0, guid, reinterpret_cast<void*>(static_cast<intptr_t>(threshold)), true);
        return 1;
    }

    if (g_itemDBCache.GetRecord(itemId, guid, &CrossedInebriationThresholdItemCallback, reinterpret_cast<void*>(static_cast<intptr_t>(threshold)), false)) {
        CrossedInebriationThresholdItemCallback(itemId, guid, reinterpret_cast<void*>(static_cast<intptr_t>(threshold)), true);
    }

    return 1;
}

// VERIFY: 42e86b51 manual
// OFFSET: 0x006D0240
int32_t __stdcall CGPlayer_C::OnQuestGiverQuestList(CDataStore* msg) {
    WOWGUID npc;
    msg->Get(npc.m_guid);

    char greeting[2048];
    msg->GetString(greeting, sizeof(greeting));

    QUESTGIVEREMOTENODE emote;
    msg->Get(emote.m_delay);
    msg->Get(reinterpret_cast<uint32_t&>(emote.m_emote));

    uint8_t count;
    msg->Get(count);

    s_questListNpc = npc;
    memset(s_questListIds, 0, sizeof(s_questListIds));
    memset(s_questListStatus, 0, sizeof(s_questListStatus));
    memset(s_questListLevels, 0, sizeof(s_questListLevels));
    memset(s_questListFlags, 0, sizeof(s_questListFlags));
    memset(s_questListRepeatable, 0, sizeof(s_questListRepeatable));
    CGQuestInfo::SetState(npc, WOWGUID(), 0, greeting, 0, 0);

    char titles[32][512];

    for (uint32_t i = 0; i < count; i++) {
        msg->Get(s_questListIds[i]);
        msg->Get(s_questListStatus[i]);
        msg->Get(s_questListLevels[i]);
        msg->Get(s_questListFlags[i]);

        uint8_t repeatable = 0;
        msg->Get(repeatable);
        s_questListRepeatable[i] = repeatable;

        msg->GetString(titles[i], sizeof(titles[i]));

        uint32_t status = s_questListStatus[i];

        if (status == 3 || status == 4) {
            CGQuestInfo::AddQuestInProgress(s_questListIds[i], titles[i], s_questListLevels[i], s_questListFlags[i], s_questListRepeatable[i]);
        } else {
            CGQuestInfo::AddQuest(s_questListIds[i], titles[i], s_questListLevels[i], s_questListFlags[i], status == 0, s_questListRepeatable[i]);
        }
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(npc, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->SetEmoteQueue(&emote, 1);
    }

    CGQuestInfo::EndQuestList();
    return 1;
}

// VERIFY: 1dea9fc1 manual
// OFFSET: 0x006D08F0
int32_t __stdcall CGPlayer_C::OnQuestGiverRequestItems(CDataStore* msg) {
    WOWGUID npc;
    msg->Get(npc.m_guid);

    uint32_t questId;
    msg->Get(questId);

    char title[512];
    msg->GetString(title, sizeof(title));

    char text[3000];
    msg->GetString(text, sizeof(text));

    QUESTGIVEREMOTENODE emote;
    msg->Get(emote.m_delay);
    msg->Get(reinterpret_cast<uint32_t&>(emote.m_emote));

    uint32_t autoLaunched;
    msg->Get(autoLaunched);

    uint32_t flags;
    msg->Get(flags);

    uint32_t suggestedGroupNum;
    msg->Get(suggestedGroupNum);

    uint32_t requiredMoney;
    msg->Get(requiredMoney);

    uint32_t numItems;
    msg->Get(numItems);

    uint32_t itemIds[6] = { 0 };
    uint32_t itemCounts[6];
    uint32_t itemDisplays[6];

    for (int32_t i = 0; i < static_cast<int32_t>(numItems); i++) {
        msg->Get(itemIds[i]);
        msg->Get(itemCounts[i]);
        msg->Get(itemDisplays[i]);
    }

    uint32_t completable0;
    msg->Get(completable0);

    uint32_t completable1;
    msg->Get(completable1);

    uint32_t completable2;
    msg->Get(completable2);

    uint32_t completable3;
    msg->Get(completable3);

    CGQuestInfo::SetState(npc, WOWGUID(), 2, text, questId, 0);
    int32_t canComplete = completable0 && completable1 && completable2 && completable3;
    CGQuestInfo::AddItemRequest(title, itemIds, itemCounts, itemDisplays, numItems, requiredMoney, canComplete, autoLaunched, suggestedGroupNum, flags);

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(npc, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->SetEmoteQueue(&emote, 1);
    }

    return 1;
}

// VERIFY: e3fa33e0 auto
// OFFSET: 0x006CD7C0
int32_t __stdcall CGPlayer_C::OnQuestGiverQuestInvalid(CDataStore* msg) {
    uint32_t reason;
    msg->Get(reason);

    switch (reason) {
        case 1:
            CGGameUI::DisplayError(ERR_QUEST_FAILED_LOW_LEVEL);
            break;

        case 6:
            CGGameUI::DisplayError(ERR_QUEST_FAILED_WRONG_RACE);
            break;

        case 7:
            CGGameUI::DisplayError(ERR_QUEST_ALREADY_DONE);
            break;

        case 12:
            CGGameUI::DisplayError(ERR_QUEST_ONLY_ONE_TIMED);
            break;

        case 13:
        case 18:
            CGGameUI::DisplayError(ERR_QUEST_ALREADY_ON);
            break;

        case 16:
            CGGameUI::DisplayError(ERR_QUEST_FAILED_EXPANSION);
            break;

        case 21:
            CGGameUI::DisplayError(ERR_QUEST_FAILED_MISSING_ITEMS);
            break;

        case 23:
            CGGameUI::DisplayError(ERR_QUEST_FAILED_NOT_ENOUGH_MONEY);
            break;

        case 26:
            CGGameUI::DisplayError(ERR_QUEST_FAILED_TOO_MANY_DAILY_QUESTS_I, 25);
            break;

        case 27:
            CGGameUI::DisplayError(ERR_QUEST_FAILED_CAIS);
            break;

        case 29:
            CGGameUI::DisplayError(ERR_QUEST_ALREADY_DONE_DAILY);
            break;

        default:
            CGGameUI::DisplayError(ERR_QUEST_NEED_PREREQS);
            break;
    }

    CGQuestInfo::QuestGiverFinished(0, 0, 0);
    return 1;
}

// VERIFY: 73d5b3db auto
// OFFSET: 0x006CD8C0
static void QuestRewardItemCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    WOWGUID none = { 0 };
    ItemStats_C* stats = g_itemDBCache.GetRecord(id, none, nullptr, nullptr, false);

    if (!stats) {
        return;
    }

    const char* link = CGTooltip::GetItemHyperlinkString(id, stats->m_quality, 0, 0, 0);
    int32_t count = static_cast<int32_t>(reinterpret_cast<intptr_t>(param));

    if (count <= 1) {
        CGGameUI::DisplayError(ERR_QUEST_REWARD_ITEM_S, link);
    } else {
        CGGameUI::DisplayError(ERR_QUEST_REWARD_ITEM_MULT_IS, count, link);
    }
}

// VERIFY: bafe4a7d auto
// OFFSET: 0x006CD940
static void QuestFailedCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(id, none, nullptr, nullptr, false);

    if (!quest) {
        return;
    }

    switch (s_questFailedReason) {
        case 4:
        case 16:
        case 50:
            CGGameUI::DisplayError(ERR_QUEST_FAILED_BAG_FULL_S, quest->m_name);
            CGGameUI::DisplayError(ERR_INV_FULL);
            break;

        case 17:
            CGGameUI::DisplayError(ERR_QUEST_FAILED_MAX_COUNT_S, quest->m_name);
            break;

        case 74:
            CGGameUI::DisplayError(ERR_QUEST_FAILED_TOO_MANY_DAILY_QUESTS_I, 25);
            break;

        default:
            if (!CGQuestLog::IsQuestExpiredByID(id)) {
                CGGameUI::DisplayError(ERR_QUEST_FAILED_S, quest->m_name);
            }

            break;
    }
}

// VERIFY: b475afdb auto
// OFFSET: 0x006CDA70
int32_t __stdcall CGPlayer_C::OnQuestGiverQuestFailed(CDataStore* msg) {
    uint32_t questId;
    msg->Get(questId);
    msg->Get(s_questFailedReason);

    QuestCache* quest = g_questDBCache.GetRecord(questId, CGQuestInfo::m_npc, QuestFailedCallback, nullptr, false);

    if (quest) {
        switch (s_questFailedReason) {
            case 4:
            case 16:
            case 50:
                CGGameUI::DisplayError(ERR_QUEST_FAILED_BAG_FULL_S, quest->m_name);
                CGGameUI::DisplayError(ERR_INV_FULL);
                break;

            case 17:
                CGGameUI::DisplayError(ERR_QUEST_FAILED_MAX_COUNT_S, quest->m_name);
                break;

            case 74:
                CGGameUI::DisplayError(ERR_QUEST_FAILED_TOO_MANY_DAILY_QUESTS_I, 25);
                break;

            default:
                if (!CGQuestLog::IsQuestExpiredByID(questId)) {
                    CGGameUI::DisplayError(ERR_QUEST_FAILED_S, quest->m_name);
                }

                break;
        }
    }

    CGQuestInfo::QuestGiverFinished(0, questId, 0);
    return 1;
}

// VERIFY: 2b919027 manual
// OFFSET: 0x006D0E10
static void QuestCompleteCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    QuestCompleteRewards* rewards = static_cast<QuestCompleteRewards*>(param);

    if (!success) {
        delete rewards;
        return;
    }

    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(id, none, nullptr, nullptr, false);

    if (!quest) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    CGGameUI::DisplayError(ERR_QUEST_COMPLETE_S, quest->m_name);
    SI2::PlaySoundKit("igQuestListComplete", nullptr, nullptr, nullptr);

    if (rewards->m_xp) {
        CGGameUI::DisplayError(ERR_QUEST_REWARD_EXP_I, rewards->m_xp);
    }

    if (rewards->m_money > 0) {
        char money[1024];
        GetCurrencyText(rewards->m_money, money, sizeof(money), ", ");
        CGGameUI::DisplayError(ERR_QUEST_REWARD_MONEY_S, money);
    }

    char text[3000];

    if (rewards->m_honor > 0) {
        rewards->m_honor = static_cast<int32_t>(static_cast<uint32_t>(rewards->m_honor) / 10 + 0.5);
        const char* format = FrameScript_GetText("COMBATLOG_HONORAWARD", -1, GENDER_NOT_APPLICABLE);

        if (format && *format) {
            SStrPrintf(text, sizeof(text), format, rewards->m_honor);
            CGChat::AddChatMessage(text, CHAT_MSG_COMBAT_HONOR_GAIN, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
        }

        SendToCombatText(43, rewards->m_honor);
    }

    if (rewards->m_talents > 0) {
        const char* format = FrameScript_GetText("LEVEL_UP_CHAR_POINTS", -1, GENDER_NOT_APPLICABLE);

        if (format && *format) {
            SStrPrintf(text, sizeof(text), format, rewards->m_talents);
            CGChat::AddChatMessage(text, CHAT_MSG_MONEY, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
        }
    }

    if (rewards->m_arenaPoints > 0) {
        const char* format = FrameScript_GetText("COMBATLOG_ARENAPOINTSAWARD", -1, GENDER_NOT_APPLICABLE);

        if (format && *format) {
            SStrPrintf(text, sizeof(text), format, rewards->m_arenaPoints);
            CGChat::AddChatMessage(text, CHAT_MSG_ARENA_POINTS, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
        }

        SendToCombatText(47, rewards->m_arenaPoints);
    }

    WOWGUID player = ClntObjMgrGetActivePlayer();

    for (uint32_t i = 0; i < 4; i++) {
        int32_t itemId = static_cast<int32_t>(quest->m_rewardItem[i]);
        void* count = reinterpret_cast<void*>(static_cast<uintptr_t>(quest->m_rewardItemCount[i]));

        if (itemId > 0 && g_itemDBCache.GetRecord(itemId, player, QuestRewardItemCallback, count, false)) {
            QuestRewardItemCallback(itemId, player, count, true);
        }
    }

    int32_t chosen = CGQuestInfo::m_lastChosenItem;

    if (chosen) {
        if (g_itemDBCache.GetRecord(chosen, player, QuestRewardItemCallback, nullptr, false)) {
            WOWGUID none2 = { 0 };
            ItemStats_C* stats = g_itemDBCache.GetRecord(chosen, none2, nullptr, nullptr, false);

            if (stats) {
                CGGameUI::DisplayError(ERR_QUEST_REWARD_ITEM_S, CGTooltip::GetItemHyperlinkString(chosen, stats->m_quality, 0, 0, 0));
            }
        }

        CGQuestInfo::m_lastChosenItem = 0;
    }

    CGQuestInfo::QuestGiverFinished(quest->m_nextQuestId != 0, id, 0);
    delete rewards;
}

// VERIFY: bd6214c4 manual
// OFFSET: 0x006D1110
int32_t __stdcall CGPlayer_C::OnQuestGiverQuestComplete(CDataStore* msg) {
    QuestCompleteRewards* rewards = new (__FILE__, __LINE__) QuestCompleteRewards;

    uint32_t questId;
    msg->Get(questId);
    msg->Get(reinterpret_cast<uint32_t&>(rewards->m_xp));
    msg->Get(reinterpret_cast<uint32_t&>(rewards->m_money));
    msg->Get(reinterpret_cast<uint32_t&>(rewards->m_honor));
    msg->Get(reinterpret_cast<uint32_t&>(rewards->m_talents));
    msg->Get(reinterpret_cast<uint32_t&>(rewards->m_arenaPoints));

    WOWGUID none = { 0 };

    if (g_questDBCache.GetRecord(questId, none, QuestCompleteCallback, rewards, false)) {
        QuestCompleteCallback(questId, none, rewards, true);
    }

    return 1;
}

// VERIFY: 6f6228c3 manual
// OFFSET: 0x00518B30
uint32_t CGItem::GetEnchantmentId(int32_t slot) {
    if (this->GetItemStaticFlags(0) & 0x2000) {
        return 0;
    }

    return this->m_item->m_itemFieldEnchantment[slot].m_id;
}

// VERIFY: 9ac27aa0 manual
// OFFSET: 0x005CD970
int32_t CGPlayer_C::GetCharacterPoints(int32_t index) const {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    return this->m_player->m_playerCharacterPoints[index];
}

// VERIFY: 970a6a97 manual
// OFFSET: 0x005E8290
uint32_t CGItem::GetEnchantmentDuration(int32_t slot) {
    if (this->GetItemStaticFlags(0) & 0x2000) {
        return 0;
    }

    return this->m_item->m_itemFieldEnchantment[slot].m_duration;
}

// VERIFY: 1b68385f auto
// OFFSET: 0x006D7B30
void CGPlayer_C::CastRandomCritter() {
    uint32_t count = this->m_companionSpells.m_count;

    if (count) {
        Spell_C::CastSpell(this->m_companionSpells.m_data[rand() % count], nullptr, WOWGUID{ 0 }, false);
    }
}

// VERIFY: f9b626f7 manual
// OFFSET: 0x006D7320
int32_t ClientPlaySpellVisualHandler(WOWGUID guid, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit && unit->IsHostileCritter()) {
        const SpellRec* spell = static_cast<const SpellRec*>(param);

        if (!spell) {
            SErrDisplayAppFatalMessage("", "", 0);
        }

        const SpellVisualRec* visual = GetSpellVisual(spell);

        if (visual) {
            SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_targetImpactKit);

            if (kit) {
                PlaySpellVisuaKitData data(spell, kit, 6);
                unit->PlaySpellVisualKit(data);
            }
        }
    }

    return 1;
}

// VERIFY: 27cd2fc0 auto
// OFFSET: 0x006D73B0
void EnumVisibleForAuraEffect(const SpellRec* spell, int32_t display, int32_t flags) {
    s_visibleAuraEffectDisplay = display;
    s_visibleAuraEffectFlags = display ? flags : 0;
    ClntObjMgrEnumVisibleObjects(ClientPlaySpellVisualHandler, const_cast<SpellRec*>(spell));
}

// VERIFY: 8d94fa79 auto
// OFFSET: 0x006D7490
void CGPlayer_C::UpdateXRayVision(const SpellRec* spell) {
    ClntObjMgrEnumVisibleObjects(CGPlayer_C::XRayVisionEffectProc, const_cast<SpellRec*>(spell));
}

// VERIFY: 371bd036 manual
// OFFSET: 0x006D1C30
int32_t OnUpdatePlayerOverrideSpellsID(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    if (ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        CGSpellBook::UpdateUsable();
        CGSpellBook::UpdateSelection();
        CGActionBar::UpdateOverrideSpellBar();
        CGActionBar::UpdateBonusBar();
    }

    return 1;
}

// VERIFY: ad8b210c manual
// OFFSET: 0x006DA770
int32_t OnUpdatePlayerAuraVision(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        uint8_t changed = *static_cast<const uint8_t*>(value) ^ reinterpret_cast<const uint8_t*>(&player->m_player->m_playerFieldBytes2)[3];

        if (changed & 0x40) {
            CGWorldFrame::UpdateScreenEffect();
        }

        ClntObjMgrEnumVisibleObjects(AuraVisionUpdateHandler, &changed);
    }

    return 1;
}

// VERIFY: 5071e977 manual
// OFFSET: 0x006E71D0
void CGPlayer_C::DelKnownSpell(int32_t spellId, int32_t silent) {
    if (spellId > g_spellDB.m_maxID) {
        SysMsgPrintf(STATUS_ERROR, 2, "NOSPELLIDFOUND|%d", spellId);
        return;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    bool notify = silent == 0;

    if (spell.m_attributes & 0x20) {
        SkillLineAbilityRec* ability = SpellTableLookupAbility(this->m_unit->m_unitFieldBytes0[0], this->m_unit->m_unitFieldBytes0[1], spellId);
        SkillLineRec* skillLine = ability ? g_skillLineDB.GetRecord(ability->m_skillLine) : nullptr;

        if (skillLine) {
            HASHKEY_NONE key;
            TRADESKILLLINE* line = this->m_tradeSkillLines.Ptr(skillLine->m_id, key);

            if (line) {
                for (uint32_t i = line->m_spells.m_count; i; ) {
                    i--;

                    if (line->m_spells.m_data[i] == spellId) {
                        uint32_t last = line->m_spells.m_count - 1;

                        if (i < last) {
                            memcpy(&line->m_spells.m_data[i], &line->m_spells.m_data[i + 1], sizeof(int32_t) * (last - i));
                        }

                        line->m_spells.SetCount(last);
                        CGTradeSkillInfo::RefreshList(1);
                        break;
                    }
                }
            }
        }
    } else {
        if (spell.m_effect[0] == 155) {
            this->RemoveEquipBypassSkill(spellId);
        }

        if (notify && !(spell.m_attributes & 0x80)) {
            if (*spell.m_nameSubtext) {
                char text[512];
                SStrPrintf(text, sizeof(text), "%s (%s)", spell.m_name, spell.m_nameSubtext);
                CGGameUI::DisplayError(ERR_SPELL_UNLEARNED_S, text);
            } else {
                CGGameUI::DisplayError(ERR_SPELL_UNLEARNED_S, spell.m_name);
            }
        }
    }

    CGSpellBook::DelKnownSpell(spellId, 0);
    CGActionBar::RemoveMacro(spellId);

    if (!silent) {
        CGClassTrainer::RefreshList();
    }

    if (spellId == s_dualWieldSpell) {
        s_dualWieldSpell = 0;
    }

    if (spellId == CGGameUI::GetCursorSpell()) {
        CGGameUI::DropCursorSpell();
    }
}

// VERIFY: 332a79d2 manual
// OFFSET: 0x006E22C0
void CGPlayer_C::RemoveEquipBypassSkill(int32_t spellId) {
    bool found = false;

    for (uint32_t i = 0; i < this->m_equipBypassSkills.m_count; i++) {
        if (this->m_equipBypassSkills.m_data[i] == spellId) {
            found = true;
        } else if (!found) {
            continue;
        }

        uint32_t count = this->m_equipBypassSkills.m_count;

        if (i) {
            if (i < count - 1) {
                this->m_equipBypassSkills.m_data[i - 1] = this->m_equipBypassSkills.m_data[i];
            }

            if (i == this->m_equipBypassSkills.m_count - 1) {
                this->m_equipBypassSkills.SetCount(i);
            }
        } else if (count == 1) {
            this->m_equipBypassSkills.m_count = 0;
        }
    }
}

// VERIFY: 0cc2c313 manual
// OFFSET: 0x006CDF30
static void ReadInspectItems(CDataStore* msg) {
    if (!s_inspectData) {
        return;
    }

    memset(s_inspectData->m_items, 0, sizeof(s_inspectData->m_items));
    uint32_t mask = 0;
    msg->Get(mask);

    for (uint32_t i = 0; i < 19; i++, mask >>= 1) {
        if (!(mask & 1)) {
            continue;
        }

        InspectItem* item = &s_inspectData->m_items[i];
        int32_t itemId = 0;
        msg->Get(reinterpret_cast<uint32_t&>(itemId));

        if (itemId < 0) {
            item->m_flags |= 1;
            itemId = -itemId;
        }

        item->m_itemId = itemId;
        uint16_t enchantMask = 0;
        msg->Get(enchantMask);
        int32_t* enchant = item->m_enchants;

        for (uint32_t bits = enchantMask; bits; bits >>= 1, enchant++) {
            if (bits & 1) {
                uint16_t enchantId = 0;
                msg->Get(enchantId);
                *enchant = enchantId;
            }
        }

        uint16_t randomPropertiesId = 0;
        msg->Get(randomPropertiesId);
        item->m_randomPropertiesId = randomPropertiesId;
        SmartGUID creator;
        *msg >> creator;
        item->m_creator = creator;
        uint32_t seed = 0;
        msg->Get(seed);
        item->m_propertySeed = seed;
    }
}

// VERIFY: 70ae18be auto
// OFFSET: 0x006CE070
int32_t Packet_SMSG_INSPECT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;

    if (guid == CGCharacterInfo::m_inspectTarget) {
        ReadInspectItems(msg);
    } else {
        msg->m_read = msg->m_size;
    }

    return 1;
}

// VERIFY: 9cb6b1b5 manual
// OFFSET: 0x006CE0C0
int32_t Packet_SMSG_INSPECT_TALENT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;

    if (guid == CGCharacterInfo::m_inspectTarget) {
        CGTalentInfo_Inspect::UpdateTalentData(msg);
        FrameScript_SignalEvent(SCRIPT_EVENT_INSPECT_TALENT_READY, nullptr);

        if (!s_inspectData) {
            s_inspectData = static_cast<InspectData*>(SMemAlloc(sizeof(InspectData), __FILE__, __LINE__, 0));
        }

        s_inspectData->m_guid = guid;
        ReadInspectItems(msg);
    }

    return 1;
}

// VERIFY: 7c69ee68 auto
// OFFSET: 0x006CF2A0
void CGPlayer_C::ExitAutoCombatMode() {
    Spell_C_SetAutoRangedCombatSpell(0);
    EventUnregister(EVENT_ID_IDLE, CGPlayer_C::AutoCombatModeEventHandler);
    this->m_combatModeFlags &= ~0xEu;
    CGGameUI::UpdateCombatMode();
}

// VERIFY: 3148365b manual
// OFFSET: 0x006D5F70
void CGPlayer_C::MeleeModeLeave() {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && (this->CGUnit_C::IsInMelee() || this->m_combat.m_attackSent)) {
        this->StopAttack();
        Spell_C_CancelMeleeSpells();
    }
}

// VERIFY: a9e0121f auto
// OFFSET: 0x006CEB10
static int32_t IsCastAnimation(uint32_t anim) {
    switch (anim) {
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

// VERIFY: 310d0239 auto
// OFFSET: 0x006E23A0
void CGPlayer_C::ToggleSheathe() {
    int32_t casting = 0;

    if (this->m_unit->m_unitChannelSpell || (this->m_animationState & 0x400)) {
        casting = 1;
    }

    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer() || !this->IsLocalClientControlled()) {
        return;
    }

    if (this->m_mountDisplayId > 0 && !(this->m_clientFlags & 0x10000000)) {
        return;
    }

    if ((this->m_unit->m_unitFieldFlags & 0x40000) || this->m_unit->m_unitFieldHealth <= 0) {
        return;
    }

    if (this->m_combat.m_attacking || casting || this->m_castSpellId) {
        return;
    }

    if (IsCastAnimation(this->GetCurrentTorsoAnimId()) || this->GetClientStandState()) {
        return;
    }

    CM2Model* model = this->m_worldModel;

    if (model) {
        if (!model->IsLoaded(0, 0)) {
            return;
        }

        int32_t hasSheathe = model->HasSequence(90);
        int32_t hasUnsheathe = model->HasSequence(89);

        if (!hasSheathe || !hasUnsheathe || !model->HasBone(3) || !model->HasBone(2)) {
            return;
        }

        if (model->GetBoneSequenceId(3) == 90 || model->GetBoneSequenceId(2) == 90 || model->GetBoneSequenceId(3) == 89 || model->GetBoneSequenceId(2) == 89) {
            return;
        }
    }

    const CGUnitVirtualItem* mainHand = this->GetVirtualItem(0, false);
    const CGUnitVirtualItem* offHand = this->GetVirtualItem(1, false);
    const CGUnitVirtualItem* ranged = this->GetVirtualItem(2, false);
    ChrClassesRec* playerClass = g_chrClassesDB.GetRecord(this->m_unit->m_unitFieldBytes0[1]);

    if (!playerClass || (playerClass->m_flags & 0x8)) {
        ranged = nullptr;
    }

    switch (this->m_sheatheState) {
        case 0:
            if (mainHand || offHand) {
                this->SetSheatheState(1, 0, 0);
            } else if (ranged) {
                this->SetSheatheState(2, 0, 0);
            }

            if (this->CGUnit_C::IsInMelee()) {
                this->SetCombatMode();
            }

            break;

        case 1:
            if (ranged) {
                this->SetSheatheState(2, 0, 0);
            } else {
                this->SetSheatheState(0, 0, 0);
            }

            break;

        case 2:
            this->SetSheatheState(0, 0, 0);
            break;
    }
}

// VERIFY: b949cd28 manual
// OFFSET: 0x006E2610
void CGPlayer_C::MeleeModeEnter(const WOWGUID& target) {
    if (!CGGameUI::CanPerformAction(1)) {
        return;
    }

    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer() || (this->m_unit->m_unitFieldFlags & 0x20000)) {
        return;
    }

    if (!CGGameUI::m_hasControl || (this->m_player->m_playerFlags & 0x1000000)) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    if (!this->CanAttackNow(unit)) {
        if (this->CGUnit_C::IsInMelee()) {
            this->SetCombatMode();
        }

        return;
    }

    int16_t overrideId = static_cast<int16_t>(this->m_player->m_playerFieldBytes2);
    OverrideSpellDataRec* overrideRec;

    if (overrideId && (overrideRec = g_overrideSpellDataDB.GetRecord(overrideId)) && !(overrideRec->m_flags & 2)) {
        return;
    }

    if (this->IsVehiclePassengerSeated()) {
        WOWGUID transport;
        transport.m_guid = this->GetTransportGUID();
        CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(transport, TYPEMASK_UNIT, __FILE__, __LINE__));
        VehicleSeatRec* seat;

        if (!vehicle || !(seat = vehicle->GetVehicleSeatRec(this->m_movementData.m_transportSeat)) || !(seat->m_flags & 0x4000)) {
            return;
        }
    }

    Spell_C_CancelRangedSpells();

    if (this->m_unit->m_unitFieldBytes1[0]) {
        this->ChangeStandState(0);
    }

    CGGameUI::Target(unit->m_obj->m_guid);
    WOWGUID self = this->m_obj->m_guid;

    if (Spell_C_IsCastingInterruptedByCombat(self) && !this->m_combat.m_attacking) {
        return;
    }

    if (this->m_stopAttackSent || (!this->m_combat.m_attacking && !this->m_combat.m_attackSent)) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_ATTACK_SWING));
        msg.Put(unit->m_obj->m_guid.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
        this->m_combat.SetAttackSent(unit->m_obj->m_guid);
    }

    this->SetSheatheState(1, 1, 0);
    Spell_C_SetAutoRangedCombatSpell(0);
    this->SpellInterrupted(this->m_castCount, this->m_castSpellId);
    Spell_C_CancelAutoRepeat(true);
    Spell_C_ClearLastAutoRangedCombatError();
    // WoWFix335 melee-swing-right-click: original ends with UpdateBaseAnimation(0x40, -1)
}

// VERIFY: d5f87a32 manual
// OFFSET: 0x006E2BE0
int32_t CGPlayer_C::AutoCombatModeEventHandler(const void* data, void* param) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 1;
    }

    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGGameUI::m_lockedTarget, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!target) {
        return 1;
    }

    if (!s_cvAutoRangedCombat->m_intValue || !CGSpellBook::m_autoRangedSpell) {
        Spell_C_SetAutoRangedCombatSpell(0);
        return 1;
    }

    if (player->IsInMeleeRange(target)) {
        if (!player->CGUnit_C::IsInMelee()) {
            WOWGUID guid = target->m_obj->m_guid;
            player->MeleeModeEnter(guid);
        }

        player->m_combatModeFlags &= ~4u;
        return 1;
    }

    if (Spell_C_GetAutoRepeatingSpell()) {
        return 1;
    }

    if (!player->CanAttackNow(target)) {
        player->SetCombatMode();
        return 1;
    }

    if (player->m_combatModeFlags & 8) {
        player->ExitAutoCombatMode();
        return 1;
    }

    C3Vector toTarget = target->GetPosition() - player->GetPosition();
    float minRange;
    float maxRange;
    Spell_C::GetSpellRange(player, CGSpellBook::m_autoRangedSpell, &minRange, &maxRange, target);
    float distanceSq = toTarget.y * toTarget.y + toTarget.z * toTarget.z + toTarget.x * toTarget.x;

    if (minRange * minRange < distanceSq || !player->CGUnit_C::IsInMelee()) {
        if (player->CGUnit_C::IsInMelee()) {
            player->MeleeModeLeave();
        }

        if (!Spell_C_GetAutoRangedCombatSpell()) {
            Spell_C_SetAutoRangedCombatSpell(CGSpellBook::m_autoRangedSpell);
            player->m_combatModeFlags |= 4;
        }

        C3Vector direction = player->m_movementData.GetDirection();

        if (direction.y * toTarget.y + direction.z * toTarget.z + direction.x * toTarget.x > 0.0f && !(player->m_movement->m_flags & 0x33) && maxRange * maxRange > distanceSq && Spell_C::CastSpell(CGSpellBook::m_autoRangedSpell, nullptr, target->m_obj->m_guid, false)) {
            Spell_C_ClearLastAutoRangedCombatError();
        }
    }

    return 1;
}

// VERIFY: b36390c6 auto
// OFFSET: 0x006D66E0
void CGPlayer_C::OnLevelChange() {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CGSpellBook::UpdateSpells(0, 0, 0);
        CGClassTrainer::RefreshList();
        CGUnit_C::NamePlateUpdateLevels(true);
        FrameScript_SignalEvent(SCRIPT_EVENT_PVPQUEUE_ANYWHERE_UPDATE_AVAILABLE, nullptr);
    }
}

// VERIFY: a8506f00 auto
// OFFSET: 0x006CDDB0
int32_t SkillMaxRankChangeHandler(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    CGClassTrainer::RefreshList();
    CGTradeSkillInfo::RefreshList(0);
    PlayerSignalSkillStatChanges();
    return 1;
}

// STATIC: 0x00C9D340
static PlayerLootItem s_lootItems[18];

// VERIFY: 9e4784c1 auto
// OFFSET: 0x006CEB80
int32_t CGPlayer_C::GetLootItem(uint32_t slot) {
    if (slot >= 18) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    return s_lootItems[slot].m_itemId;
}

// VERIFY: b65d7e62 auto
// OFFSET: 0x006CEBB0
int32_t CGPlayer_C::GetLootItemDisplayId(uint32_t slot) {
    if (slot >= 18) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    return s_lootItems[slot].m_displayId;
}

// VERIFY: dcfad75c auto
// OFFSET: 0x006CEBE0
int32_t CGPlayer_C::GetLootItemCount(uint32_t slot) {
    if (slot >= 18) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    return s_lootItems[slot].m_count;
}

// VERIFY: 5618845f auto
// OFFSET: 0x006CEC10
int32_t CGPlayer_C::GetLootItemRandomSuffix(uint32_t slot) {
    if (slot >= 18) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    return s_lootItems[slot].m_randomSuffix;
}

// VERIFY: 44db345a auto
// OFFSET: 0x006CEC40
int32_t CGPlayer_C::GetLootItemRandomPropertyId(uint32_t slot) {
    if (slot >= 18) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    return s_lootItems[slot].m_randomPropertyId;
}

// VERIFY: ad884501 auto
// OFFSET: 0x006CEC70
int32_t CGPlayer_C::GetLootItemSlotType(uint32_t slot) {
    if (slot >= 18) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    return s_lootItems[slot].m_slotType;
}

// VERIFY: 5e3e02c7 manual
// OFFSET: 0x006DC770
void CGPlayer_C::ClearLootItemSlotType(WOWGUID lootObject, uint32_t slot) {
    if (slot >= 18) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && player->m_lootTarget == lootObject) {
        s_lootItems[slot].m_slotType = 0;
    }
}

// VERIFY: 6d310e19 auto
// OFFSET: 0x006CECA0
int32_t __stdcall CGPlayer_C::OnLootMoneyNotify(CDataStore* msg) {
    uint32_t amount;
    uint8_t alone;
    msg->Get(amount);
    msg->Get(alone);

    if (amount) {
        char money[1024];
        GetCurrencyText(amount, money, sizeof(money), ", ");
        char text[3000];
        SStrPrintf(text, sizeof(text), FrameScript_GetText(alone ? "YOU_LOOT_MONEY" : "LOOT_MONEY_SPLIT", -1, GENDER_NOT_APPLICABLE), money);
        CGChat::AddChatMessage(text, 28, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    return 1;
}

// VERIFY: aaf305cc auto
// OFFSET: 0x006CEDD0
void CGPlayer_C::ClearLootingObject() {
    this->m_lootTarget = WOWGUID();
    this->UpdateBaseAnimation(0, -1);
}

// VERIFY: 5787f2d9 manual
// OFFSET: 0x006D53B0
int32_t CGPlayer_C::OnLootResponse(uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    uint8_t lootType;
    msg->Get(guid.m_guid);
    msg->Get(lootType);

    if (!this->m_lootTarget || this->m_lootTarget != guid) {
        if (this->m_lootTarget || (lootType != 2 && lootType != 3 && lootType != 4 && lootType != 6 && lootType != 7 && lootType != 8)) {
            if (lootType) {
                CDataStore release;
                release.Put(static_cast<uint32_t>(CMSG_LOOT_RELEASE));
                release.Put(guid.m_guid);
                release.Finalize();
                ClientServices::Send2(&release);
            } else if (msg->m_size != msg->m_read) {
                uint8_t error;
                msg->Get(error);

                if (error == 15) {
                    CGGameUI::DisplayError(ERR_ALREADY_PICKPOCKETED);
                }
            }

            this->m_lootTarget = WOWGUID();
            this->UpdateBaseAnimation(0, -1);
            CGGameUI::UnlockItem(guid);
            return 1;
        }
    }

    if (lootType) {
        uint32_t coins;
        uint8_t count;
        msg->Get(coins);
        msg->Get(count);

        if (count > 18) {
            count = 18;
        }

        CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPEMASK_OBJECT, __FILE__, __LINE__);

        if (!object) {
            CGGameUI::DisplayError(ERR_LOOT_DIDNT_KILL);
            CDataStore release;
            release.Put(static_cast<uint32_t>(CMSG_LOOT_RELEASE));
            release.Put(guid.m_guid);
            release.Finalize();
            ClientServices::Send2(&release);
            return 1;
        }

        if (object->m_obj->m_type & TYPEMASK_ITEM) {
            CGGameUI::LockItem(guid);
        }

        this->m_lootTarget = guid;
        memset(s_lootItems, 0, sizeof(s_lootItems));

        for (uint32_t i = 0; i < count; i++) {
            uint8_t slot;
            msg->Get(slot);
            PlayerLootItem& item = s_lootItems[slot];
            msg->Get(reinterpret_cast<uint32_t&>(item.m_itemId));
            msg->Get(reinterpret_cast<uint32_t&>(item.m_count));
            msg->Get(reinterpret_cast<uint32_t&>(item.m_displayId));
            msg->Get(reinterpret_cast<uint32_t&>(item.m_randomSuffix));
            msg->Get(reinterpret_cast<uint32_t&>(item.m_randomPropertyId));
            uint8_t slotType;
            msg->Get(slotType);
            s_lootItems[slot].m_slotType = slotType;
        }

        CGGameUI::OpenLoot(object, coins, lootType);

        if (this->m_movement->m_flags & 0xC000FF) {
            CGGameUI::CloseLoot(true, true, false);
        }

        return 1;
    }

    uint8_t error;
    msg->Get(error);

    switch (error) {
    case 4:
        CGGameUI::DisplayError(ERR_LOOT_TOO_FAR);
        break;
    case 5:
        CGGameUI::DisplayError(ERR_LOOT_BAD_FACING);
        break;
    case 6:
        CGGameUI::DisplayError(ERR_LOOT_LOCKED);
        break;
    case 8:
        CGGameUI::DisplayError(ERR_LOOT_NOTSTANDING);
        break;
    case 9:
        CGGameUI::DisplayError(ERR_LOOT_STUNNED);
        break;
    case 10:
        CGGameUI::DisplayError(ERR_LOOT_PLAYER_NOT_FOUND);
        return 1;
    case 11:
        CGGameUI::DisplayError(ERR_PLAY_TIME_EXCEEDED);
        break;
    case 12:
        CGGameUI::DisplayError(ERR_LOOT_MASTER_INV_FULL);
        return 1;
    case 13:
        CGGameUI::DisplayError(ERR_LOOT_MASTER_UNIQUE_ITEM);
        return 1;
    case 14:
        CGGameUI::DisplayError(ERR_LOOT_MASTER_OTHER);
        return 1;
    case 16:
        CGGameUI::DisplayError(ERR_NOT_WHILE_SHAPESHIFTED);
        return 1;
    default:
        CGGameUI::DisplayError(ERR_LOOT_DIDNT_KILL);
        break;
    }

    this->m_lootTarget = WOWGUID();
    this->UpdateBaseAnimation(0, -1);
    CGGameUI::UnlockItem(guid);
    this->UpdateBaseAnimation(0, -1);
    return 1;
}

// VERIFY: 6a9eb816 manual
// OFFSET: 0x006D5860
int32_t CGPlayer_C::OnLootRemoved(CDataStore* msg) {
    uint8_t slot;
    msg->Get(slot);

    if (ClntObjMgrObjectPtr(this->m_lootTarget, TYPEMASK_OBJECT, __FILE__, __LINE__)) {
        memset(&s_lootItems[slot], 0, sizeof(s_lootItems[slot]));
        CGGameUI::ClearLootSlot(slot);
    }

    return 1;
}

// VERIFY: fc79bef2 manual
// OFFSET: 0x006D58D0
int32_t CGPlayer_C::OnLootSlotChanged(CDataStore* msg) {
    WOWGUID guid;
    uint8_t slot;
    msg->Get(guid.m_guid);
    msg->Get(slot);

    if (!ClntObjMgrObjectPtr(this->m_lootTarget, TYPEMASK_OBJECT, __FILE__, __LINE__) || this->m_lootTarget != guid) {
        msg->m_read = msg->m_size;
        return 1;
    }

    PlayerLootItem& item = s_lootItems[slot];
    msg->Get(reinterpret_cast<uint32_t&>(item.m_itemId));
    msg->Get(reinterpret_cast<uint32_t&>(item.m_displayId));
    msg->Get(reinterpret_cast<uint32_t&>(item.m_randomSuffix));
    msg->Get(reinterpret_cast<uint32_t&>(item.m_randomPropertyId));
    msg->Get(reinterpret_cast<uint32_t&>(item.m_count));
    CGGameUI::LootSlotChanged(slot);
    return 1;
}

// VERIFY: 27a68345 auto
// OFFSET: 0x006D59E0
int32_t CGPlayer_C::OnLootReleaseResponse(CDataStore* msg) {
    WOWGUID guid;
    uint8_t released;
    msg->Get(guid.m_guid);
    msg->Get(released);

    if (released && guid == this->m_lootTarget) {
        this->m_lootTarget = WOWGUID();
        this->UpdateBaseAnimation(0, -1);
        CGGameUI::CloseLoot(false, false, false);
    }

    return 1;
}

// VERIFY: 346301d0 manual
// OFFSET: 0x006DAC40
void CGPlayer_C::LootCorpse(CGCorpse_C* corpse, bool autoInteract) {
    if (!this->CanLoot(corpse) || !corpse->CanBeLooted() || (this->m_movement->m_flags & 0xC010FF)) {
        return;
    }

    if (autoInteract) {
        C3Vector corpsePosition = corpse->GetPosition();
        C3Vector position = this->GetPosition();
        float dz = position.z - corpsePosition.z;
        float dy = position.y - corpsePosition.y;
        float dx = position.x - corpsePosition.x;

        if (dx * dx + (dy * dy + dz * dz) > 25.0f) {
            WOWGUID guid = corpse->m_obj->m_guid;

            if (this->SetTrackingTarget(guid, 6)) {
                return;
            }
        }
    }

    CGGameUI::CloseLoot(true, false, false);
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LOOT_UNIT));
    msg.Put(corpse->m_obj->m_guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
    this->m_lootTarget = corpse->m_obj->m_guid;
    this->PlayBaseAnimation(ANIM_LOOT, 0);
}

// VERIFY: 4c4a93c3 manual
// OFFSET: 0x006DAD90
void CGPlayer_C::LootUnit(CGUnit_C* unit, bool autoInteract) {
    if (!this->CanLoot(unit) || !unit->CanBeLooted(FrameTime::s_curTimeMs)) {
        return;
    }

    if (autoInteract) {
        float range = this->GetMeleeRange(unit);
        C3Vector unitPosition = unit->GetPosition();
        C3Vector position = this->GetPosition();
        float dz = position.z - unitPosition.z;
        float dy = position.y - unitPosition.y;
        float dx = position.x - unitPosition.x;

        if (range * range < dx * dx + (dy * dy + dz * dz)) {
            WOWGUID guid = unit->m_obj->m_guid;

            if (this->SetTrackingTarget(guid, 6)) {
                return;
            }
        }
    }

    if (this->m_movement->m_flags & 0xC010FF) {
        return;
    }

    if (this->m_mountDisplayId > 0 && !(this->m_clientFlags & 0x10000000) && !this->MountedCombatAllowed()) {
        if (!this->CanAutoDismount()) {
            CGGameUI::DisplayError(ERR_NOT_WHILE_MOUNTED);
            return;
        }

        this->Dismount();
    }

    if (this->HasVehicleTransport()) {
        this->CheckVehicleExitOnCast();
    }

    CGGameUI::CloseLoot(true, false, false);
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_LOOT_UNIT));
    msg.Put(unit->m_obj->m_guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
    this->m_lootTarget = unit->m_obj->m_guid;
    this->PlayBaseAnimation(ANIM_LOOT, 0);
}

// VERIFY: 84a4b9ef manual
// OFFSET: 0x006D51F0
static int32_t InteractUpdateProc(WOWGUID guid, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(param);
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPEMASK_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return 1;
    }

    uint32_t type = object->m_obj->m_type;

    if (type != (TYPEMASK_UNIT | TYPEMASK_OBJECT) && type != (TYPEMASK_GAMEOBJECT | TYPEMASK_OBJECT)) {
        return 1;
    }

    if (type & TYPEMASK_GAMEOBJECT) {
        if (player->CGUnit_C::CanInteract(static_cast<CGGameObject_C*>(object))) {
            return 1;
        }
    } else {
        if (player->CanInteract(static_cast<CGUnit_C*>(object))) {
            if (static_cast<CGUnit_C*>(object)->m_unit->m_unitNpcFlags & 0x2000) {
                player->UpdateTaxiStatus(static_cast<CGUnit_C*>(object));
            }

            return 1;
        }
    }

    object->m_questStatus = 0;
    object->m_interactIconType = 0;
    object->RemoveInteractIcon();
    return 1;
}

// VERIFY: c1f1a966 manual
// OFFSET: 0x006DC5A0
void SendQuestGiverStatusMultipleQuery() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        ClntObjMgrEnumVisibleObjects(InteractUpdateProc, player);
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_QUEST_GIVER_STATUS_MULTIPLE_QUERY));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: d34ef321 manual
// OFFSET: 0x005946C0
uint16_t CGPlayer_C::GetSkillLineID(int32_t index) const {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        return this->m_player->m_playerSkillInfo[index].m_skillId;
    }

    return 0;
}

// VERIFY: b7e4b9ce manual
// OFFSET: 0x006DA4A0
int32_t SkillRankChangeHandler(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    uint32_t newRank = *static_cast<const uint16_t*>(value);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));
    uint32_t index = (offset - offsetof(CGPlayerLocalData, m_playerSkillInfo[0].m_rank)) / sizeof(PlayerSkillInfo);

    if (index >= 128) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (player && player->GetSkillValue(index) != newRank) {
        SignalPlayerSkillStatEvents();
        uint32_t oldRank = player->GetSkillValue(index);
        ConsolePrintf("Skill %d increased from %d to %d", player->GetSkillLineID(index), newRank, oldRank);
        PlayerSignalSkillStatChanges();
        SkillLineRec* skillLine = g_skillLineDB.GetRecord(player->GetSkillLineID(index));
        const SkillRaceClassInfoRec* info = SpellTableLookupInfo(player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[1], player->GetSkillLineID(index));

        if (skillLine) {
            if (info && !(info->m_flags & 0x402)) {
                if (!newRank) {
                    CGGameUI::DisplayError(ERR_SKILL_GAINED_S, skillLine->m_displayName);
                } else if (static_cast<int32_t>(player->GetSkillValue(index)) > 0) {
                    CGGameUI::DisplayError(ERR_SKILL_UP_SI, skillLine->m_displayName, player->GetSkillValue(index));
                }
            }

            if (skillLine->m_categoryID == 9 || skillLine->m_categoryID == 11) {
                CGPlayer_C::QueryQuestGiverStatusMultiple();
            }
        }
    }

    CGClassTrainer::RefreshList();
    CGTradeSkillInfo::RefreshList(0);
    CGMerchantInfo::RefreshList();
    return 1;
}

// VERIFY: 2ea51292 manual
// OFFSET: 0x006DCCC0
bool CGPlayer_C::GetPackAndSlot(const CGItem_C* item, uint8_t* bag, uint8_t* slot) {
    WOWGUID container = item->m_item->m_itemFieldContained;

    if (container == this->m_obj->m_guid) {
        *bag = 0xFF;
        *slot = this->FindSlotIndex(item->m_obj->m_guid);
        return *slot != 0xFF;
    }

    *bag = this->FindSlotIndex(container);

    if (*bag == 0xFF) {
        return false;
    }

    CGContainer_C* object = static_cast<CGContainer_C*>(ClntObjMgrObjectPtr(container, TYPEMASK_CONTAINER, __FILE__, __LINE__));

    if (!object) {
        return false;
    }

    int32_t index = object->GetBag()->FindItemSlot(item->m_obj->m_guid);

    if (index < 0) {
        return false;
    }

    *slot = static_cast<uint8_t>(index);
    return true;
}

// VERIFY: f458e054 manual
// OFFSET: 0x006DCD70
void CGPlayer_C::OpenLootItem(CGItem_C* item) {
    uint8_t bag;
    uint8_t slot;

    if (!this->GetPackAndSlot(item, &bag, &slot)) {
        return;
    }

    CGGameUI::CloseLoot(true, false, false);
    this->m_autoLoot = CGGameUI::IsAutoLooting() != 0;

    if (this->m_autoLoot && this->m_lootTarget) {
        CGLootInfo::DoAutoLoot(0);
    }

    this->m_lootTarget = item->m_obj->m_guid;
    CGGameUI::LockItem(item->m_obj->m_guid);
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_OPEN_ITEM));
    msg.Put(bag);
    msg.Put(slot);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// STATIC: 0x00ADA1AC
static const UIErrors s_mountResultErrors[] = { ERR_MOUNT_INVALIDMOUNTEE, ERR_MOUNT_TOOFARAWAY, ERR_MOUNT_ALREADYMOUNTED, ERR_MOUNT_NOTMOUNTABLE, ERR_MOUNT_NOTYOURPET, ERR_MOUNT_OTHER, ERR_MOUNT_LOOTING, ERR_MOUNT_RACECANTMOUNT, ERR_MOUNT_SHAPESHIFTED, ERR_MOUNT_FORCEDDISMOUNT, ERR_MAX_VALUE };

// STATIC: 0x00ADA1D8
static const UIErrors s_dismountResultErrors[] = { ERR_DISMOUNT_NOPET, ERR_DISMOUNT_NOTMOUNTED, ERR_DISMOUNT_NOTYOURPET, ERR_MAX_VALUE };

// STATIC: 0x00A32618
static const UIErrors s_buyBankSlotErrors[] = { ERR_BANKSLOT_FAILED_TOO_MANY, ERR_BANKSLOT_INSUFFICIENT_FUNDS, ERR_BANKSLOT_NOTBANKER };

// VERIFY: eb850af4 auto
// OFFSET: 0x006CEDF0
static void __stdcall DisplayMountResult(uint32_t result) {
    if (result < 11) {
        CGGameUI::DisplayError(s_mountResultErrors[result]);
    }
}

// VERIFY: 6eebaa29 auto
// OFFSET: 0x006CEE10
static void __stdcall DisplayDismountResult(uint32_t result) {
    if (result < 4) {
        CGGameUI::DisplayError(s_dismountResultErrors[result]);
    }
}

// VERIFY: 3e53c616 auto
// OFFSET: 0x006CB990
static void RandomRollNameQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    uint32_t* roll = static_cast<uint32_t*>(param);

    if (!roll) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (success) {
        WOWGUID requester = WOWGUID();
        NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, requester, nullptr, nullptr, false);

        if (name) {
            char text[3000];
            SStrPrintf(text, sizeof(text), FrameScript_GetText("RANDOM_ROLL_RESULT", -1, GENDER_NOT_APPLICABLE), name->m_name, roll[2], roll[0], roll[1]);
            CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
        }
    }

    operator delete(roll);
}

// VERIFY: 7e47e286 manual
// OFFSET: 0x006E2E90
int32_t Packet_Group_9(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (!msg) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    switch (msgId) {
        case SMSG_FORCEACTIONSHOW: {
            uint32_t a1;
            uint32_t a2;
            msg->Get(a1);
            msg->Get(a2);
            return 1;
        }

        case SMSG_PETGODMODE: {
            uint8_t enabled = 0;
            msg->Get(enabled);
            ConsoleWrite(enabled ? "Pet Godmode enabled" : "Pet Godmode disabled", DEFAULT_COLOR);
            return 1;
        }

        case SMSG_GOD_MODE: {
            uint8_t enabled = 0;
            msg->Get(enabled);
            ConsoleWrite(enabled ? "Godmode enabled" : "Godmode disabled", DEFAULT_COLOR);
            return 1;
        }

        case SMSG_TRIGGER_CINEMATIC: {
            uint32_t cinematic;
            msg->Get(cinematic);

            if (ClientServices::Connection()->m_inGame) {
                CGGameUI::StartCinematic(cinematic);
            } else {
                s_pendingMovie = cinematic;
            }

            return 1;
        }

        case SMSG_INVENTORY_CHANGE_FAILURE: {
            uint8_t result = 0;
            msg->Get(result);

            if (!result) {
                return 1;
            }

            UIErrors error = static_cast<UIErrors>(GetDisplayErrorByInventoryError(result));
            WOWGUID item;
            WOWGUID item2;
            uint8_t bag;
            msg->Get(item.m_guid);
            msg->Get(item2.m_guid);
            msg->Get(bag);
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            switch (result) {
                case 1:
                case 87: {
                    uint32_t requiredLevel = 0;
                    msg->Get(requiredLevel);
                    CGGameUI::DisplayError(error, requiredLevel);
                    break;
                }

                case 16: {
                    CGPlayer_C* activePlayer = ClntObjMgrGetActivePlayerObj();

                    if (!activePlayer || !activePlayer->ReportBagItemSubtypeMismatch(bag)) {
                        CGGameUI::DisplayError(error);
                    }

                    break;
                }

                case 81: {
                    uint64_t a2 = 0;
                    uint32_t a3 = 0;
                    uint64_t a4 = 0;
                    msg->Get(a2);
                    msg->Get(a3);
                    msg->Get(a4);

                    if (player) {
                        CGPlayer_C::ServerBindConfirm(item, a2, a3, a4, bag);
                    }

                    break;
                }

                case 84:
                case 85:
                case 89: {
                    uint32_t category = 0;
                    msg->Get(category);
                    ItemLimitCategoryRec* rec = nullptr;

                    if (!category || !(rec = g_itemLimitCategoryDB.GetRecord(category))) {
                        SErrDisplayAppFatalMessage("", "", 0);
                    }

                    CGGameUI::DisplayError(error, rec->m_quantity, rec->m_name);
                    break;
                }

                default:
                    CGGameUI::DisplayError(error);
                    break;
            }

            if (item) {
                if (player) {
                    if (item == player->m_lootTarget) {
                        CGGameUI::CloseLoot(true, false, false);
                        player->UpdateBaseAnimation(0, -1);
                    }

                    Spell_C_CancelSpellByCaster(item);
                }

                CGGameUI::UnlockItem(item);
            }

            if (item2) {
                CGGameUI::UnlockItem(item2);
            }

            if (CGLootInfo::m_lootObj && player && player->m_autoLoot) {
                FrameScript_SignalEvent(SCRIPT_EVENT_LOOT_OPENED, "%d", 1);
            }

            return 1;
        }

        case SMSG_OPEN_CONTAINER: {
            WOWGUID guid;
            msg->Get(guid.m_guid);
            CGContainerInfo::OpenContainer(guid);
            return 1;
        }

        case SMSG_INSTANCE_LOCK_WARNING_QUERY: {
            uint32_t lockTime = 0;
            msg->Get(lockTime);
            uint32_t a2 = 0;
            msg->Get(a2);
            uint8_t a3 = 0;
            msg->Get(a3);
            CGGameUI::SetInstanceLockTime(lockTime, a2, a3);
            return 1;
        }

        case SMSG_BIND_POINT_UPDATE:
            CGPlayer_C::SaveBindPoint(msg);
            return 1;

        case SMSG_BINDZONEREPLY: {
            uint32_t a1;
            uint32_t a2;
            msg->Get(a1);
            msg->Get(a2);
            return 1;
        }

        case SMSG_PLAYER_BOUND: {
            WOWGUID binder;
            uint32_t areaId;
            msg->Get(binder.m_guid);
            msg->Get(areaId);
            s_lastBinderGuid = binder;
            SI2::PlaySoundKit(0x475, nullptr, nullptr, nullptr, 0, nullptr, 1, 0);
            AreaTableRec* area = g_areaTableDB.GetRecord(areaId);

            if (area) {
                CGGameUI::DisplayError(ERR_DEATHBIND_SUCCESS_S, area->m_areaName);
            }

            return 1;
        }

        case SMSG_ITEM_PUSH_RESULT: {
            WOWGUID guid;
            uint32_t received;
            uint32_t created;
            uint32_t showInChat;
            uint8_t bag;
            uint32_t slot;
            uint32_t itemId;
            uint32_t suffixFactor;
            uint32_t randomPropertyId;
            uint32_t count;
            uint32_t totalCount;
            msg->Get(guid.m_guid);
            msg->Get(received);
            msg->Get(created);
            msg->Get(showInChat);
            msg->Get(bag);
            msg->Get(slot);
            msg->Get(itemId);
            msg->Get(suffixFactor);
            msg->Get(randomPropertyId);
            msg->Get(count);
            msg->Get(totalCount);
            OnItemPushResult(guid, bag, slot, itemId, suffixFactor, randomPropertyId, received, created, showInChat, count);
            CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

            if (player) {
                player->CheckForQuestItemLoot(itemId, count, totalCount);
            }

            return 1;
        }

        case SMSG_MOUNT_RESULT:
            if (ClntObjMgrGetActivePlayerObj()) {
                uint32_t result = 0;
                msg->Get(result);
                DisplayMountResult(result);
            }

            return 1;

        case SMSG_DISMOUNT_RESULT:
            if (ClntObjMgrGetActivePlayerObj()) {
                uint32_t result = 0;
                msg->Get(result);
                DisplayDismountResult(result);
            }

            return 1;

        case SMSG_PET_NAME_INVALID: {
            uint32_t error;
            msg->Get(error);
            CGGameUI::DisplayNameError(error);
            char name[80];
            msg->GetString(name, sizeof(name));
            uint8_t declined;
            msg->Get(declined);

            if (declined) {
                char declensions[5][96];

                for (uint32_t i = 0; i < 5; i++) {
                    msg->GetString(declensions[i], sizeof(declensions[i]));
                }

                FrameScript_SignalEvent(SCRIPT_EVENT_PET_FORCE_NAME_DECLENSION, "%s%s%s%s%s%s", name, declensions[0], declensions[1], declensions[2], declensions[3], declensions[4]);
            }

            return 1;
        }

        case SMSG_SHOW_TAXI_NODES:
            if (ClntObjMgrGetActivePlayerObj()) {
                CGPlayer_C::ShowTaxiNodes(msg);
            }

            return 1;

        case SMSG_TAXI_NODE_STATUS:
            if (ClntObjMgrGetActivePlayerObj()) {
                CGPlayer_C::OnTaxiNodeStatus(msg);
            }

            return 1;

        case SMSG_ACTIVATE_TAXI_REPLY:
            if (ClntObjMgrGetActivePlayerObj()) {
                uint32_t reply = 0;
                msg->Get(reply);
                CGPlayer_C::HandleActivateTaxiReply(reply);
            }

            return 1;

        case SMSG_NEW_TAXI_PATH:
            CGGameUI::DisplayError(ERR_NEWTAXIPATH);
            CGGossipInfo::SetGossip(WOWGUID());
            return 1;

        case SMSG_PLAYERBINDERROR:
            CGGameUI::DisplayError(ERR_DEATHBINDALREADYBOUND);
            return 1;

        case SMSG_SHOW_BANK: {
            WOWGUID banker;
            msg->Get(banker.m_guid);
            CGBankInfo::SetBanker(banker);
            return 1;
        }

        case SMSG_BUY_BANK_SLOT_RESULT: {
            uint32_t result;
            msg->Get(result);

            if (result < 3) {
                CGGameUI::DisplayError(s_buyBankSlotErrors[result]);
            }

            return 1;
        }

        case SMSG_FISH_NOT_HOOKED:
            CGGameUI::DisplayError(ERR_FISH_NOT_HOOKED);
            return 1;

        case SMSG_FISH_ESCAPED:
            CGGameUI::DisplayError(ERR_FISH_ESCAPED);
            return 1;

        case SMSG_LEVEL_UP_INFO: {
            uint32_t level;
            uint32_t health;
            uint32_t power[7];
            uint32_t stats[5];
            msg->Get(level);
            msg->Get(health);

            for (uint32_t i = 0; i < 7; i++) {
                msg->Get(power[i]);
            }

            for (uint32_t i = 0; i < 5; i++) {
                msg->Get(stats[i]);
            }

            if (!ClntObjMgrGetActivePlayerObj()) {
                return 1;
            }

            int32_t talentPoints = level >= 10;

            if (level >= 2) {
                CGTutorial::TriggerTutorial(0x36);
            }

            if (level >= 4) {
                CGTutorial::TriggerTutorial(0xD);
            }

            if (level >= 6) {
                CGTutorial::TriggerTutorial(0x16);
            }

            if (level >= 7) {
                CGTutorial::TriggerTutorial(0x15);
                CGTutorial::TriggerTutorial(0x25);
            }

            if (level >= 8) {
                CGTutorial::TriggerTutorial(0x20);
            }

            if (level >= 10) {
                CGTutorial::TriggerTutorial(0xC);
            }

            if (level >= 12) {
                CGTutorial::TriggerTutorial(0x26);
            }

            if (level >= 15) {
                CGTutorial::TriggerTutorial(0x32);
            }

            FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_LEVEL_UP, "%d%d%d%d%d%d%d%d%d", level, health, power[0], talentPoints, stats[0], stats[1], stats[2], stats[3], stats[4]);
            return 1;
        }

        case MSG_MINIMAP_PING: {
            WOWGUID guid;
            C2Vector position(0.0f, 0.0f);
            msg->Get(guid.m_guid);
            msg->Get(position.x);
            msg->Get(position.y);
            CGMinimapFrame::SetPingPosition(guid, position);
            return 1;
        }

        case SMSG_EXPLORATION_EXPERIENCE: {
            uint32_t areaId;
            int32_t experience;
            msg->Get(areaId);
            msg->Get(reinterpret_cast<uint32_t&>(experience));
            AreaTableRec* area = g_areaTableDB.GetRecord(areaId);

            if (!area) {
                return 1;
            }

            CGGameUI::DisplayError(ERR_ZONE_EXPLORED, area->m_areaName);
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (experience > 0) {
                if (!player) {
                    return 1;
                }

                if (!(player->m_player->m_playerFlags & 0x2000000)) {
                    CGGameUI::DisplayError(ERR_ZONE_EXPLORED_XP, area->m_areaName, experience);
                }
            }

            if (!player) {
                return 1;
            }

            ChrRacesRec* race = g_chrRacesDB.GetRecord(player->m_unit->m_unitFieldBytes0[0]);

            if (race && race->m_explorationSoundID) {
                SI2::PlaySoundKit(race->m_explorationSoundID, nullptr, nullptr, nullptr, 0, nullptr, 1, 0);
            }

            return 1;
        }

        case MSG_RANDOM_ROLL: {
            uint32_t* roll = static_cast<uint32_t*>(SMemAlloc(3 * sizeof(uint32_t), __FILE__, __LINE__, 0));
            msg->Get(roll[0]);
            msg->Get(roll[1]);
            msg->Get(roll[2]);
            WOWGUID guid;
            msg->Get(guid.m_guid);

            if (!FriendList::s_friendList || !FriendList::s_friendList->IsIgnored(guid, true)) {
                NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, guid, RandomRollNameQueryCallback, roll, false);

                if (!name) {
                    return 1;
                }

                char text[3000];
                SStrPrintf(text, sizeof(text), FrameScript_GetText("RANDOM_ROLL_RESULT", -1, GENDER_NOT_APPLICABLE), name->m_name, roll[2], roll[0], roll[1]);
                CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            }

            operator delete(roll);
            return 1;
        }

        case SMSG_CLEAR_FAR_SIGHT_IMMEDIATE: {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player) {
                player->ClearFarSight();
            }

            return 1;
        }

        case SMSG_CHAT_WRONG_FACTION:
            CGGameUI::DisplayError(ERR_CHAT_WRONG_FACTION);
            return 1;

        case SMSG_GM_TICKET_GET_SYSTEM_STATUS: {
            uint32_t status;
            msg->Get(status);
            FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_GM_STATUS, "%d", status);
            return 1;
        }

        case SMSG_SPIRIT_HEALER_CONFIRM: {
            WOWGUID guid;
            msg->Get(guid.m_guid);
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player) {
                player->TalkToSpiritHealer(guid);
            }

            return 1;
        }

        case MSG_QUEST_PUSH_RESULT: {
            WOWGUID guid;
            uint8_t result;
            msg->Get(guid.m_guid);
            msg->Get(result);
            WOWGUID requester = WOWGUID();
            NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, requester, nullptr, nullptr, false);

            if (!name) {
                return 1;
            }

            switch (result) {
                case 0:
                    CGGameUI::DisplayError(ERR_QUEST_PUSH_SUCCESS_S, name->m_name);
                    break;

                case 1:
                    CGGameUI::DisplayError(ERR_QUEST_PUSH_INVALID_S, name->m_name);
                    break;

                case 2:
                    CGGameUI::DisplayError(ERR_QUEST_PUSH_ACCEPTED_S, name->m_name);
                    break;

                case 3:
                    CGGameUI::DisplayError(ERR_QUEST_PUSH_DECLINED_S, name->m_name);
                    break;

                case 4:
                    CGGameUI::DisplayError(ERR_QUEST_PUSH_BUSY_S, name->m_name);
                    break;

                case 5:
                    CGGameUI::DisplayError(ERR_QUEST_PUSH_LOG_FULL_S, name->m_name);
                    break;

                case 6:
                    CGGameUI::DisplayError(ERR_QUEST_PUSH_ONQUEST_S, name->m_name);
                    break;

                case 7:
                    CGGameUI::DisplayError(ERR_QUEST_PUSH_ALREADY_DONE_S, name->m_name);
                    break;

                case 8:
                    CGGameUI::DisplayError(ERR_QUEST_PUSH_NOT_DAILY_S, name->m_name);
                    break;

                case 9:
                    CGGameUI::DisplayError(ERR_QUEST_PUSH_TIMER_EXPIRED_S, name->m_name);
                    break;

                case 10:
                    CGGameUI::DisplayError(ERR_QUEST_PUSH_NOT_IN_PARTY_S, name->m_name);
                    break;

                case 11:
                    CGGameUI::DisplayError(ERR_QUEST_PUSH_DIFFERENT_SERVER_DAILY_S, name->m_name);
                    break;
            }

            return 1;
        }

        case SMSG_RAID_GROUP_ONLY: {
            uint32_t bootTime;
            uint32_t reason = 0;
            msg->Get(bootTime);
            msg->Get(reason);

            if (!bootTime) {
                switch (reason) {
                    case 1:
                        CGGameUI::DisplayError(ERR_RAID_GROUP_LOWLEVEL);
                        break;

                    case 2:
                        CGGameUI::DisplayError(ERR_RAID_GROUP_ONLY);
                        break;

                    case 3:
                        CGGameUI::DisplayError(ERR_RAID_GROUP_FULL);
                        break;

                    case 4:
                        CGGameUI::DisplayError(ERR_RAID_GROUP_REQUIREMENTS_UNMATCH);
                        break;
                }
            }

            CGGameUI::SetInstanceBootTime(bootTime);
            return 1;
        }

        case SMSG_CHAT_NOT_IN_PARTY: {
            uint32_t type;
            msg->Get(type);

            if (type == 2 || type == 51) {
                CGGameUI::DisplayError(ERR_NOT_IN_GROUP);
            } else if (type == 3 || type == 39 || type == 40) {
                CGGameUI::DisplayError(ERR_NOT_IN_RAID);
            }

            return 1;
        }

        case SMSG_CHAT_PLAYER_NOTFOUND: {
            char name[305];
            msg->GetString(name, sizeof(name));
            CGGameUI::DisplayError(ERR_CHAT_PLAYER_NOT_FOUND_S, name);
            CGAutoCompleteName::RemoveFlagsByName(name, 32, 1, 0);
            return 1;
        }

        case MSG_TALENT_WIPE_CONFIRM: {
            WOWGUID guid;
            uint32_t cost;
            msg->Get(guid.m_guid);
            msg->Get(cost);

            if (guid) {
                CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

                if (player) {
                    player->TalkToTalentMaster(guid, cost);
                }
            } else {
                CGGameUI::DisplayError(ERR_TALENT_WIPE_ERROR);
            }

            return 1;
        }

        case SMSG_BINDER_CONFIRM: {
            WOWGUID guid;
            msg->Get(guid.m_guid);
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player) {
                player->TalkToBinder(guid);
            }

            return 1;
        }

        case SMSG_CHAT_RESTRICTED: {
            uint8_t type;
            msg->Get(type);

            switch (type) {
                case 0:
                    CGGameUI::DisplayError(ERR_CHAT_RESTRICTED);
                    break;

                case 1:
                    CGGameUI::DisplayError(ERR_CHAT_THROTTLED);
                    break;

                case 2:
                    CGGameUI::DisplayError(ERR_USER_SQUELCHED);
                    break;

                case 3:
                    CGGameUI::DisplayError(ERR_YELL_RESTRICTED);
                    break;
            }

            return 1;
        }

        case SMSG_CHAT_PLAYER_AMBIGUOUS: {
            char name[305];
            msg->GetString(name, sizeof(name));
            CGGameUI::DisplayError(ERR_CHAT_PLAYER_AMBIGUOUS_S, name);
            return 1;
        }

        case SMSG_DEATH_RELEASE_LOC: {
            int32_t mapId;
            C3Vector position(0.0f, 0.0f, 0.0f);
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
            msg->Get(reinterpret_cast<uint32_t&>(mapId));
            msg->Get(position.x);
            msg->Get(position.y);
            msg->Get(position.z);

            if (!player || (player->m_player->m_playerFlags & 0x10)) {
                position = C3Vector(0.0f, 0.0f, 0.0f);
                CGGameUI::SetDeathReleaseLocation(-1, position);
            } else {
                CGGameUI::SetDeathReleaseLocation(mapId, position);
            }

            return 1;
        }

        case SMSG_FORCED_DEATH_UPDATE: {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (!player) {
                SErrDisplayAppFatalMessage("", "", 0);
                return 0;
            }

            player->UpdateRepopTimer();
            CGGameUI::UpdateActivePlayer();
            return 1;
        }

        case SMSG_COMPLAINT_RESULT: {
            uint8_t result;
            uint8_t reason;
            msg->Get(result);
            msg->Get(reason);

            if (result == 1) {
                CGChat::m_complaintsSystemStatus = 0;
            }

            if (reason == 12) {
                CGCalendar::CalendarError("COMPLAINT_ADDED", 0);
                return 1;
            }

            CGChat::AddChatMessage(FrameScript_GetText("COMPLAINT_ADDED", -1, GENDER_NOT_APPLICABLE), CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            return 1;
        }

        case SMSG_FEATURE_SYSTEM_STATUS: {
            uint8_t status;
            msg->Get(status);
            CGChat::m_complaintsSystemStatus = status;
            msg->Get(status);
            CGChat::m_voiceChatSystemStatus = status;
            return 1;
        }

        case SMSG_REPORT_PVP_AFK_RESULT: {
            uint8_t result;
            uint8_t a2;
            uint8_t a3;
            WOWGUID guid;
            msg->Get(result);
            msg->Get(a2);
            msg->Get(a3);
            msg->Get(guid.m_guid);
            const char* text;

            if (result == 5) {
                if (g_enablePVPNotifyAFKCVar->m_intValue) {
                    return 1;
                }

                g_enablePVPNotifyAFKCVar->Set(1, true, false, false, true);
                text = FrameScript_GetText("PVP_REPORT_AFK_SYSTEM_ENABLED", -1, GENDER_NOT_APPLICABLE);
            } else {
                if (result != 6 || !g_enablePVPNotifyAFKCVar->m_intValue) {
                    return 1;
                }

                g_enablePVPNotifyAFKCVar->Set(0, true, false, false, true);
                text = FrameScript_GetText("PVP_REPORT_AFK_SYSTEM_DISABLED", -1, GENDER_NOT_APPLICABLE);
            }

            CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            return 1;
        }

        case SMSG_CONVERT_RUNE: {
            uint8_t rune;
            uint8_t type;
            msg->Get(rune);
            msg->Get(type);
            CGRuneInfo::ConvertRune(rune, type);
            return 1;
        }

        case SMSG_RESYNC_RUNES:
            CGRuneInfo::ResyncRunes(msg);
            return 1;

        case SMSG_ADD_RUNE_POWER:
            CGRuneInfo::AddRunePower(msg);
            return 1;

        case SMSG_ARENA_TEAM_CHANGE_FAILED_QUEUED: {
            uint32_t failed = 0;
            msg->Get(failed);

            if (failed) {
                CGGameUI::DisplayError(ERR_ARENA_TEAM_CHANGE_FAILED_QUEUED);
            }

            return 1;
        }

        case SMSG_TOGGLE_XP_GAIN: {
            uint8_t enabled = 0;
            msg->Get(enabled);
            ConsoleWrite(enabled ? "XP Gain Enabled" : "XP Gain Disabled", DEFAULT_COLOR);
            return 1;
        }

        case SMSG_CAMERA_SHAKE: {
            uint32_t shakeId;
            uint32_t soundId;
            msg->Get(shakeId);
            msg->Get(soundId);
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (!player) {
                SErrDisplayAppFatalMessage("", "", 0);
                return 0;
            }

            C3Vector position = player->GetPosition();
            SpellEffectCameraShakesRec* rec = g_spellEffectCameraShakesDB.GetRecord(shakeId);

            if (rec) {
                for (uint32_t i = 0; i < 3; i++) {
                    CGWorldFrame::GetActiveCamera()->AddShakeById(rec->m_cameraShake[i], &position);
                }
            }

            if (soundId) {
                SI2::PlaySoundKit(soundId, &position, nullptr, nullptr, 0, nullptr, 1, 0);
            }

            return 1;
        }

        default:
            return 0;
    }
}

// VERIFY: 78d7dcff manual
// OFFSET: 0x006D2EA0
void CGPlayer_C::BuyItemInSlot(WOWGUID merchant, const MerchantItem* item, uint32_t count, WOWGUID bag, uint8_t slot) {
    if (!item) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BUY_ITEM_IN_SLOT));
    msg.Put(merchant.m_guid);
    msg.Put(static_cast<uint32_t>(item->m_itemID));
    msg.Put(static_cast<uint32_t>(item->m_slot));
    msg.Put(bag.m_guid);
    msg.Put(slot);
    msg.Put(count);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 2389dd48 manual
// OFFSET: 0x006D6820
uint8_t CGPlayer_C::FindItemSlot(WOWGUID container, const CGItem_C* item) {
    CGObject_C* object = ClntObjMgrObjectPtr(container, TYPEMASK_OBJECT, __FILE__, __LINE__);

    if (!object || !object->GetBag()) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (!object->GetBag()->m_numSlots) {
        return 0xFF;
    }

    for (uint8_t slot = 0; slot < static_cast<uint32_t>(object->GetBag()->m_numSlots); slot++) {
        if (object->GetBag()->GetItemGuid(slot) == item->m_obj->m_guid.m_guid) {
            return slot;
        }
    }

    return 0xFF;
}

// VERIFY: 5354e118 manual
// OFFSET: 0x006DB7C0
static void AutoEquipItemCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(id, none, nullptr, nullptr, false);
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    for (uint32_t i = 0; i < s_itemSwaps.m_count; i++) {
        ITEMSWAP& swap = s_itemSwaps.m_data[i];

        if (swap.m_itemId != id) {
            continue;
        }

        if (stats && player) {
            player->AutoEquipItemToSlot(swap.m_item, swap.m_srcSlot, 0);
        }

        ITEMSWAP& entry = s_itemSwaps.m_data[i];
        entry.m_srcContainer = WOWGUID();
        entry.m_dstContainer = WOWGUID();
        entry.m_srcSlot = 0xFFFFFFFF;
        entry.m_dstSlot = 0xFFFFFFFF;
        entry.m_itemId = 0;
        entry.m_item = nullptr;
    }
}

// VERIFY: c6c5bb76 manual
// OFFSET: 0x006DB880
void CGPlayer_C::SwapGuildBankItem(int32_t itemId, uint32_t virtualSlot, uint8_t bag, uint8_t slot, uint32_t count) {
    WOWGUID banker = CGGuildBankInfo::m_banker;
    uint8_t tab = virtualSlot / 98;
    uint8_t tabSlot = virtualSlot % 98;
    CDataStore msg;

    if (tab < 6 && CGGuildInfo::m_rankInfo[this->m_player->m_playerGuildrank].m_bankTabStacksPerDay[tab]) {
        msg.Put(static_cast<uint32_t>(CMSG_GUILD_BANK_SWAP_ITEMS));
        msg.Put(banker.m_guid);
        msg.Put(static_cast<uint8_t>(0));
        msg.Put(tab);
        msg.Put(tabSlot);
        msg.Put(static_cast<uint32_t>(itemId));
        msg.Put(static_cast<uint8_t>(0));
        msg.Put(bag);
        msg.Put(slot);
        msg.Put(static_cast<uint8_t>(1));
        msg.Put(count);
        msg.Finalize();
        ClientServices::Send2(&msg);
    } else {
        CGGameUI::DisplayError(ERR_GUILD_PERMISSIONS);
    }
}

// VERIFY: b05cbb48 manual
// OFFSET: 0x006DB9D0
void CGPlayer_C::SplitItem(WOWGUID item, WOWGUID srcContainer, uint32_t srcSlot, WOWGUID dstContainer, uint32_t dstSlot, uint32_t count) {
    if (srcSlot > 255 || dstSlot > 255) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (static_cast<int32_t>(count) <= 0) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    CDataStore msg;

    if (!srcContainer) {
        return;
    }

    uint8_t srcBag = this->FindSlotIndex(srcContainer);
    uint8_t dstBag = this->FindSlotIndex(dstContainer);
    msg.Put(static_cast<uint32_t>(CMSG_SPLIT_ITEM));
    msg.Put(srcBag);
    msg.Put(static_cast<uint8_t>(srcSlot));
    msg.Put(dstBag);
    msg.Put(static_cast<uint8_t>(dstSlot));
    msg.Put(count);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 5170bd2e manual
// OFFSET: 0x006DB6F0
static void SwapItemsStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(id, none, nullptr, nullptr, false);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    for (uint32_t i = 0; i < s_itemSwaps.m_count; i++) {
        ITEMSWAP& swap = s_itemSwaps.m_data[i];

        if (swap.m_itemId != id) {
            continue;
        }

        if (stats && player) {
            player->SwapItems(WOWGUID(), swap.m_srcContainer, swap.m_srcSlot, swap.m_dstContainer, swap.m_dstSlot, 0);
        }

        swap.m_srcContainer = WOWGUID();
        swap.m_dstContainer = WOWGUID();
        swap.m_srcSlot = 0xFFFFFFFF;
        swap.m_dstSlot = 0xFFFFFFFF;
        swap.m_itemId = 0;
        swap.m_item = 0;
    }
}

// VERIFY: 1cc37cfa manual
// OFFSET: 0x006DCF20
void CGPlayer_C::GiftWrap(CGItem_C* item) {
    CGItem_C* wrap = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(s_giftWrapItem, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (!wrap || !item || (item->m_itemFlags & 1)) {
        return;
    }

    WOWGUID wrapContainer = wrap->m_item->m_itemFieldContained;
    uint8_t wrapBag = this->FindSlotIndex(wrapContainer);
    uint8_t wrapSlot = this->FindItemSlot(wrapContainer, wrap);
    WOWGUID itemContainer = item->m_item->m_itemFieldContained;
    uint8_t itemBag = this->FindSlotIndex(itemContainer);
    uint8_t itemSlot = this->FindItemSlot(itemContainer, item);
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_WRAP_ITEM));
    msg.Put(wrapBag);
    msg.Put(wrapSlot);
    msg.Put(itemBag);
    msg.Put(itemSlot);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGPlayer_C::CancelGiftWrap();
}

// VERIFY: 0b114c36 manual
// OFFSET: 0x006DF890
void CGPlayer_C::SwapItems(WOWGUID item, WOWGUID srcContainer, uint32_t srcSlot, WOWGUID dstContainer, uint32_t dstSlot, int32_t ignoreBind) {
    if (srcSlot > 255 || dstSlot > 255) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (!srcContainer) {
        return;
    }

    WOWGUID self = this->m_obj->m_guid;

    if ((srcContainer == self || dstContainer == self) && !CGGameUI::CanPerformAction(22)) {
        return;
    }

    CDataStore msg;
    bool srcEquipped = srcContainer == self && (srcSlot < 23 || srcSlot - 67 <= 6);
    bool dstEquipped = dstContainer == self && (dstSlot < 23 || dstSlot - 67 <= 6);

    if (srcEquipped != dstEquipped) {
        WOWGUID container = srcEquipped ? dstContainer : srcContainer;
        uint32_t slot = srcEquipped ? dstSlot : srcSlot;
        CGObject_C* object = ClntObjMgrObjectPtr(container, TYPEMASK_OBJECT, __FILE__, __LINE__);

        if (!object || !object->GetBag()) {
            return;
        }

        WOWGUID itemGuid;
        itemGuid.m_guid = object->GetBag()->GetItemGuid(slot);
        CGItem_C* bagItem = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(itemGuid, TYPEMASK_ITEM, __FILE__, __LINE__));

        if (bagItem && !bagItem->IsBound() && !ignoreBind) {
            ItemStats_C* stats = g_itemDBCache.GetRecord(bagItem->m_obj->m_entryID, bagItem->m_obj->m_guid, SwapItemsStatsCallback, nullptr, false);

            if (!stats) {
                uint32_t index = FindEmptySwapIndex();
                s_itemSwaps.m_data[index].m_srcContainer = srcContainer;
                s_itemSwaps.m_data[index].m_srcSlot = srcSlot;
                s_itemSwaps.m_data[index].m_dstContainer = dstContainer;
                s_itemSwaps.m_data[index].m_dstSlot = dstSlot;
                s_itemSwaps.m_data[index].m_itemId = bagItem->m_obj->m_entryID;
                return;
            }

            if (stats->m_bonding == 2 && this->CanUseItem(stats, ignoreBind)) {
                uint32_t index = FindEmptySwapIndex();
                s_itemSwaps.m_data[index].m_srcContainer = srcContainer;
                s_itemSwaps.m_data[index].m_srcSlot = srcSlot;
                s_itemSwaps.m_data[index].m_dstContainer = dstContainer;
                s_itemSwaps.m_data[index].m_dstSlot = dstSlot;
                s_itemSwaps.m_data[index].m_itemId = bagItem->m_obj->m_entryID;
                FrameScript_SignalEvent(SCRIPT_EVENT_EQUIP_BIND_CONFIRM, "%d", index);
                return;
            }
        }
    }

    if (srcContainer == dstContainer && srcContainer == this->m_obj->m_guid) {
        msg.Put(static_cast<uint32_t>(CMSG_SWAP_INV_ITEM));
        msg.Put(static_cast<uint8_t>(dstSlot));
        msg.Put(static_cast<uint8_t>(srcSlot));
    } else {
        uint8_t srcBag = this->FindSlotIndex(srcContainer);
        uint8_t dstBag = this->FindSlotIndex(dstContainer);

        if (dstBag == 0xFF && dstContainer != this->m_obj->m_guid) {
            return;
        }

        msg.Put(static_cast<uint32_t>(CMSG_SWAP_ITEM));
        msg.Put(dstBag);
        msg.Put(static_cast<uint8_t>(dstSlot));
        msg.Put(srcBag);
        msg.Put(static_cast<uint8_t>(srcSlot));
    }

    msg.Finalize();
    ClientServices::Send2(&msg);
    CGGameUI::ClearCursor(0, 1);
}

// VERIFY: f8859e1e manual
// OFFSET: 0x006D4A00
int32_t CGPlayer_C::ValidateSlot(uint32_t slot, WOWGUID item) {
    CGItem_C* itemObject = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (!itemObject) {
        return 0;
    }

    if ((itemObject->m_obj->m_type & TYPEMASK_CONTAINER) && (slot < 19 || slot > 22) && (slot < 67 || slot > 73) && itemObject->GetBag()->GetNumItems()) {
        return 0;
    }

    ChrClassesRec* classRec = g_chrClassesDB.GetRecord(this->m_unit->m_unitFieldBytes0[1]);

    if (classRec && (classRec->m_flags & 0x8)) {
        return itemObject->CanGoInSlot(slot, 1);
    }

    return itemObject->CanGoInSlot(slot, 0);
}

// VERIFY: 03aa23c8 manual
// OFFSET: 0x006DBAE0
void CGPlayer_C::AutoStoreItemInBag(WOWGUID item, WOWGUID srcContainer, uint32_t srcSlot, WOWGUID dstBag, int32_t unused_a5) {
    if (srcSlot > 255) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    CDataStore msg;

    if (srcContainer) {
        uint8_t srcBag = this->FindSlotIndex(srcContainer);
        uint8_t dstBagIndex = this->FindSlotIndex(dstBag);

        if (dstBagIndex == 0xFF && dstBag != this->m_obj->m_guid) {
            return;
        }

        msg.Put(static_cast<uint32_t>(CMSG_AUTO_STORE_BAG_ITEM));
        msg.Put(srcBag);
        msg.Put(static_cast<uint8_t>(srcSlot));
        msg.Put(dstBagIndex);
    }

    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 523fd827 manual
// OFFSET: 0x006DBBE0
static void AutoEquipStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    WOWGUID none = WOWGUID();

    if (!g_itemDBCache.GetRecord(id, none, nullptr, nullptr, false)) {
        return;
    }

    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGGameUI::GetCursorItem(), TYPEMASK_ITEM, __FILE__, __LINE__));

    if (item && static_cast<int32_t>(item->m_obj->m_entryID) == id) {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (player) {
            player->AutoEquipCursorItem(0);
        }
    }
}

// VERIFY: 2b35572b manual
// OFFSET: 0x006DFC40
void CGPlayer_C::AutoEquipCursorItem(int32_t ignoreBind) {
    if (!CGGameUI::CanPerformAction(22)) {
        return;
    }

    if (CGGameUI::m_cursorItemType == 9) {
        int32_t ammo = CGGameUI::GetCursorVirtualItem();

        if (ammo) {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_SET_AMMO));
            msg.Put(static_cast<uint32_t>(ammo));
            msg.Finalize();
            ClientServices::Send2(&msg);
            CGGameUI::ClearCursor(1, 1);
        }

        return;
    }

    WOWGUID item;
    WOWGUID container;
    uint32_t slot;
    CGGameUI::GetCursorItem(item, container, slot);

    if (!item) {
        return;
    }

    if (container == this->m_obj->m_guid && slot < 23) {
        CGGameUI::ClearCursor(1, 1);
        return;
    }

    int32_t clearMode = 0;
    CDataStore msg;

    if (container) {
        CGItem_C* itemObject = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPEMASK_ITEM, __FILE__, __LINE__));

        if (!itemObject) {
            return;
        }

        uint8_t bag = this->FindSlotIndex(container);

        if (!itemObject->IsBound() && !ignoreBind) {
            ItemStats_C* stats = g_itemDBCache.GetRecord(itemObject->m_obj->m_entryID, item, AutoEquipStatsCallback, nullptr, false);

            if (!stats) {
                return;
            }

            if (stats->m_bonding == 2 && this->CanUseItem(stats, ignoreBind)) {
                uint32_t index = FindEmptySwapIndex();
                s_itemSwaps.m_data[index].m_srcContainer = container;
                s_itemSwaps.m_data[index].m_srcSlot = slot;
                s_itemSwaps.m_data[index].m_dstContainer = WOWGUID();
                s_itemSwaps.m_data[index].m_dstSlot = 0;
                s_itemSwaps.m_data[index].m_itemId = itemObject->m_obj->m_entryID;
                FrameScript_SignalEvent(SCRIPT_EVENT_AUTOEQUIP_BIND_CONFIRM, "%d", index);
                return;
            }
        }

        if (itemObject->GetClassID() == 6) {
            clearMode = 1;
            msg.Put(static_cast<uint32_t>(CMSG_SET_AMMO));
            msg.Put(static_cast<uint32_t>(itemObject->m_obj->m_entryID));
        } else {
            CGGameUI::LockItem(itemObject->m_obj->m_guid);
            msg.Put(static_cast<uint32_t>(CMSG_AUTO_EQUIP_ITEM));
            msg.Put(bag);
            msg.Put(static_cast<uint8_t>(slot));
        }
    }

    msg.Finalize();
    ClientServices::Send2(&msg);
    CGGameUI::ClearCursor(clearMode, 1);
}

// VERIFY: fbdbff93 manual
// OFFSET: 0x006D6C10
void CGPlayer_C::UpdateObjComponentVisualsSlot(const VisibleItem_C* item, int32_t slot) {
    int32_t hand;

    if (slot == 15) {
        hand = 0;
    } else if (slot == 16) {
        hand = 1;
    } else {
        return;
    }

    const CGUnitVirtualItem* virtualItem = this->GetVirtualItem(hand, false);

    if (!virtualItem) {
        return;
    }

    SHEATHE_TYPE sheatheType = static_cast<SHEATHE_TYPE>(virtualItem->m_sheatheType);

    if (this->m_sheatheState) {
        if (slot == 15) {
            this->UpdateObjComponentVisuals(item, CCharacterComponent::GetSheatheLink(sheatheType, true));
            this->UpdateObjComponentVisuals(item, static_cast<GEOCOMPONENTLINKS>(1));
        } else if (const_cast<VisibleItem_C*>(item)->GetInventoryType() == 14) {
            this->UpdateObjComponentVisuals(item, static_cast<GEOCOMPONENTLINKS>(0));
        } else {
            this->UpdateObjComponentVisuals(item, CCharacterComponent::GetSheatheLink(sheatheType, false));
            this->UpdateObjComponentVisuals(item, static_cast<GEOCOMPONENTLINKS>(2));
        }
    } else {
        GEOCOMPONENTLINKS link = CCharacterComponent::GetSheatheLink(sheatheType, slot == 15);
        this->UpdateObjComponentVisuals(item, static_cast<GEOCOMPONENTLINKS>((slot != 15) + 1));
        this->UpdateObjComponentVisuals(item, link);
    }
}

// VERIFY: b088396b manual
// OFFSET: 0x006DE7E0
uint32_t CGPlayer_C::GetEquippedItemDisplayId() {
    int32_t ammo = this->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? this->m_player->m_playerAmmoId : 0;
    ItemRec* rec = g_itemDB.GetRecord(ammo);
    return rec ? rec->m_displayInfoID : 0;
}

// VERIFY: 79a55964 manual
// OFFSET: 0x006DFEE0
void CGPlayer_C::AutoEquipItem(WOWGUID container, uint32_t slot, int32_t ignoreBind) {
    CGObject_C* object = ClntObjMgrObjectPtr(container, TYPEMASK_OBJECT, __FILE__, __LINE__);

    if (!object || !object->GetBag()) {
        return;
    }

    CGBag_C* bag = object->GetBag();

    if (slot >= static_cast<uint32_t>(bag->m_numSlots)) {
        return;
    }

    WOWGUID item = bag->m_slots[slot];

    if (!item) {
        return;
    }

    if (CGTradeInfo::IsItemBeingTraded(item)) {
        CGGameUI::DisplayError(ERR_EQUIP_TRADE_ITEM);
        return;
    }

    CGGameUI::SetCursorItem(item, container, slot, 1, 0, 0);
    this->AutoEquipCursorItem(ignoreBind);
}

// VERIFY: d775d48d manual
// OFFSET: 0x006E08C0
void CGPlayer_C::EquipVisibleItem(const VisibleItem_C* item, int32_t slot) {
    if (!item || !abs(item->m_itemId) || static_cast<int32_t>(const_cast<VisibleItem_C*>(item)->GetDisplayID()) <= 0) {
        return;
    }

    switch (slot) {
        case 15:
            this->AddHandItem(0);
            this->SetWeaponTrail(this->m_worldModel, 0);
            return;

        case 16:
            this->AddHandItem(1);
            this->SetWeaponTrail(this->m_worldModel, 1);
            return;

        case 17:
            this->AddHandItem(2);
            this->SetWeaponTrail(this->m_worldModel, 2);
            return;

        default:
            break;
    }

    if (!this->m_characterComponent || !this->CanEquipItemsInThisForm() || (CGPlayer_C::IsXRayVisionActive() && this->UsesNpcBakedTexture())) {
        return;
    }

    if (slot == 18) {
        this->LoadGuildTextures(const_cast<VisibleItem_C*>(item)->GetDisplayID());
        return;
    }

    if ((slot == 0 && (this->m_player->m_playerFlags & 0x400)) || (slot == 14 && (this->m_player->m_playerFlags & 0x800))) {
        return;
    }

    this->m_characterComponent->AddItemBySlot(static_cast<INVENTORY_SLOTS>(slot), const_cast<VisibleItem_C*>(item)->GetDisplayID(), 0);
}

// VERIFY: 0dbd89fa manual
// OFFSET: 0x006E09E0
int32_t CGPlayer_C::RefreshVisibleItems() {
    if (!this->CanEquipItemsInThisForm()) {
        return 0;
    }

    if (!this->GetBag()) {
        return 1;
    }

    for (uint32_t slot = 0; slot <= 18; slot++) {
        VisibleItem_C* item = &this->m_player->m_playerVisibleItems[slot];

        if (abs(item->m_itemId) && item->m_itemId <= 0) {
            switch (slot) {
                case 15:
                    if (this->m_sheatheState <= 1) {
                        CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(15), static_cast<SHEATHE_TYPE>(0), false);
                    }

                    break;

                case 16:
                    if (this->m_sheatheState <= 1) {
                        CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(16), static_cast<SHEATHE_TYPE>(0), false);
                        CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(16), static_cast<SHEATHE_TYPE>(0), true);
                    }

                    break;

                case 17:
                    if (this->m_sheatheState == 2 || !this->m_sheatheState) {
                        CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(17), static_cast<SHEATHE_TYPE>(0), false);
                    }

                    if (this->m_sheatheState == 2) {
                        CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(16), static_cast<SHEATHE_TYPE>(0), false);
                        CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(15), static_cast<SHEATHE_TYPE>(0), false);
                    }

                    break;

                default:
                    if (this->m_characterComponent) {
                        this->m_characterComponent->RemoveItemBySlot(static_cast<INVENTORY_SLOTS>(slot));
                    }

                    break;
            }
        }

        this->EquipVisibleItem(item, slot);

        if (!item->m_itemId && slot == 15) {
            CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(15), static_cast<SHEATHE_TYPE>(0), false);
            CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(15), static_cast<SHEATHE_TYPE>(1), false);
            CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(15), static_cast<SHEATHE_TYPE>(2), false);
            CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(15), static_cast<SHEATHE_TYPE>(3), false);
        }
    }

    return 1;
}

// VERIFY: aaebd191 manual
// OFFSET: 0x006E0D30
void CGPlayer_C::GetRangedAttackSkillRank(int32_t& rank, int32_t& bonus) {
    bonus = 0;
    rank = 0;
    int32_t spellId = 0;
    const CGUnitVirtualItem* item = this->GetVirtualItem(2, false);

    if (!item || item->m_classID == 2) {
        uint32_t subclass = item ? item->m_subclassID : ClientDBGetUnarmedWeapon();

        if (subclass < s_weaponSkills.m_count) {
            spellId = s_weaponSkills.m_data[subclass];
        }
    }

    SkillLineAbilityRec* ability = SpellTableLookupAbility(this->m_unit->m_unitFieldBytes0[0], this->m_unit->m_unitFieldBytes0[1], spellId);

    if (ability && ability->m_spell == spellId) {
        this->GetExpandedSkillRank(ability->m_skillLine, rank, bonus);
    }

    bonus += static_cast<int32_t>(this->GetCombatRatingPct(0) - 0.5f);
    bonus += static_cast<int32_t>(this->GetCombatRatingPct(22) - 0.5f);
}

// VERIFY: d1cf8e16 manual
// OFFSET: 0x006E0E00
void CGPlayer_C::ShowHelm() {
    VisibleItem_C* head = &this->m_player->m_playerVisibleItems[0];

    if (head && head->GetDisplayID() && this->CanEquipItemsInThisForm() && this->m_characterComponent) {
        this->m_characterComponent->AddItemBySlot(static_cast<INVENTORY_SLOTS>(0), head->GetDisplayID(), 0);
    }

    if ((this->m_player->m_playerFlags & 0x400) && this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_SHOWING_HELM));
        msg.Put(static_cast<uint8_t>(1));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    this->ApplyComponentItemsFromEffects();
}

// VERIFY: bde8b931 manual
// OFFSET: 0x006E0EF0
void CGPlayer_C::ShowCloak() {
    VisibleItem_C* back = &this->m_player->m_playerVisibleItems[14];

    if (back && back->GetDisplayID() && this->CanEquipItemsInThisForm() && this->m_characterComponent) {
        this->m_characterComponent->AddItemBySlot(static_cast<INVENTORY_SLOTS>(14), back->GetDisplayID(), 0);
    }

    if ((this->m_player->m_playerFlags & 0x800) && this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_SHOWING_CLOAK));
        msg.Put(static_cast<uint8_t>(1));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }
}

// VERIFY: 008ff140 manual
// OFFSET: 0x006E1290
void CGPlayer_C::UpdateHandItemVisual(GEOCOMPONENTLINKS link, int32_t hand) {
    uint32_t slot;

    if (hand == 0) {
        slot = 15;
    } else if (hand == 1) {
        slot = 16;
    } else {
        return;
    }

    VisibleItem_C* item = &this->m_player->m_playerVisibleItems[slot];

    if (item && abs(item->m_itemId)) {
        this->UpdateObjComponentVisuals(item, link);
    }
}

// VERIFY: 8210ca7a manual
// OFFSET: 0x006E1E10
void CGPlayer_C::UpdateInventoryComponent(INVENTORY_SLOTS slot, const VisibleItem_C* oldItem) {
    VisibleItem_C* item = nullptr;

    if (!oldItem || slot > 18 || !(item = &this->m_player->m_playerVisibleItems[slot])) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    VisibleItem_C* old = const_cast<VisibleItem_C*>(oldItem);

    if (abs(old->m_itemId) == abs(item->m_itemId) && (old->m_itemId > 0) == (item->m_itemId > 0)) {
        this->UpdateObjComponentVisualsSlot(item, slot);
        return;
    }

    CGTradeSkillInfo::RefreshList(0);
    CGCharacterInfo::UpdateInventoryAlerts();

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CGActionBar::UpdateUsable();
        CGActionBar::UpdateItem(abs(old->m_itemId));
        CGActionBar::UpdateItem(abs(item->m_itemId));

        switch (slot) {
            case 15:
                SignalPlayerAttackEvent(0);
                CGSpellBook::UpdateSpells(0, 0, 0);
                break;

            case 16:
                SignalPlayerAttackEvent(1);
                break;

            case 17:
                FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_RANGED_ATTACK_POWER, "%s", "player");
                CGSpellBook::UpdateSpells(0, 0, 0);
                break;

            default:
                break;
        }
    }

    const CGUnitVirtualItem* rangedItem = this->GetVirtualItem(2, false);
    bool thrown = rangedItem && (rangedItem->m_inventoryType == 26 || rangedItem->m_inventoryType == 25);
    CM2Model* detached = nullptr;

    switch (slot) {
        case 15:
            this->ClearWeaponTrail(0);

            if (!thrown || this->m_sheatheState != 2) {
                CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(15), static_cast<SHEATHE_TYPE>(old->GetSheatheType()), false);
                break;
            }

            for (CM2Model* attached = this->m_worldModel->m_attachList; attached; attached = attached->m_attachNext) {
                if (attached->m_attachmentId == 1) {
                    attached->m_refCount++;
                    detached = attached;
                    attached->DetachFromParent();
                    break;
                }
            }

            CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(15), static_cast<SHEATHE_TYPE>(old->GetSheatheType()), false);

            if (detached) {
                detached->AttachToParent(this->m_worldModel, 1, nullptr, 0);
                detached->Release();
            }

            break;

        case 16:
            this->ClearWeaponTrail(1);

            if (thrown || this->m_sheatheState != 2) {
                CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(16), static_cast<SHEATHE_TYPE>(old->GetSheatheType()), old->GetInventoryType() == 14);
                break;
            }

            for (CM2Model* attached = this->m_worldModel->m_attachList; attached; attached = attached->m_attachNext) {
                if (attached->m_attachmentId == 2) {
                    attached->m_refCount++;
                    detached = attached;
                    attached->DetachFromParent();
                    break;
                }
            }

            CCharacterComponent::RemoveHandItem(this->m_worldModel, static_cast<INVENTORY_SLOTS>(16), static_cast<SHEATHE_TYPE>(old->GetSheatheType()), old->GetInventoryType() == 14);

            if (detached) {
                detached->AttachToParent(this->m_worldModel, 2, nullptr, 0);
                detached->Release();
            }

            break;

        case 17:
            if (this->m_sheatheState == 2) {
                uint32_t inventoryType = old->GetInventoryType();
                this->m_worldModel->DetachAllChildrenById(inventoryType == 26 || inventoryType == 25 ? 1 : 2);
            }

            if (this->m_characterComponent) {
                this->m_characterComponent->RemoveItem(ITEMSLOT_11);
            }

            if (!this->m_rangedWeaponModel) {
                goto applyEffects;
            }

            if (this->m_sheatheState != 2) {
                if (this->m_rangedWeaponModel->m_attachParent) {
                    this->m_rangedWeaponModel->DetachFromParent();
                }

                this->m_rangedWeaponModel->m_fFlags |= 0x20;
                this->m_rangedWeaponModel->m_drawCallback = nullptr;
                this->m_rangedWeaponModel->m_drawArg = nullptr;
                this->m_rangedWeaponModel->Release();
                this->m_rangedWeaponModel = nullptr;
                goto applyEffects;
            }

            break;

        default:
            if (this->CanEquipItemsInThisForm() && this->m_characterComponent) {
                this->m_characterComponent->RemoveItemBySlot(slot);
            }

            break;
    }

    this->EquipVisibleItem(item, slot);

applyEffects:
    this->ApplyComponentItemsFromEffects();

    if (this->m_sheatheState == 2 && slot == 17) {
        this->SetSheatheState(2, 0, 0);
        this->UpdateBaseAnimation(0, -1);
    }

    CGGameUI::UnitModelUpdate(this->m_obj->m_guid, 3);
}

// VERIFY: f9d20eba manual
// OFFSET: 0x005E82C0
int32_t CGItem::GetEnchantmentCharges(int32_t slot) {
    if (this->GetItemStaticFlags(0) & 0x2000) {
        return 0;
    }

    return this->m_item->m_itemFieldEnchantment[slot].m_charges;
}

// VERIFY: a2058e4d manual
// OFFSET: 0x006D7070
int32_t CGPlayer_C::GetAttackPowerContributionFromStat(int32_t stat, int32_t value) {
    ChrClassesRec* classRec = g_chrClassesDB.GetRecord(this->m_unit->m_unitFieldBytes0[1]);

    if (!classRec) {
        return 0;
    }

    bool strengthClass = classRec->m_damageBonusStat == 1;
    int32_t bonus = value - 10 < 0 ? 0 : value - 10;
    int32_t attackPower = 0;

    if (stat == 0) {
        attackPower = bonus;

        if (!strengthClass) {
            return 2 * bonus;
        }
    } else if (stat == 1) {
        if (strengthClass) {
            attackPower = bonus;
        }

        SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(this->GetShapeshiftForm());

        if (form && (form->m_flags & 0x20)) {
            attackPower += value;
        }
    }

    return attackPower;
}

// VERIFY: 93bc731c manual
// OFFSET: 0x006DE410
float CGPlayer_C::GetCombatRatingPct(int32_t rating) {
    float base = GetGameTableValue(1, this->m_unit->m_unitFieldLevel - 1, rating);
    float scalar = GetGameTableValue(11, rating + 1, this->m_unit->m_unitFieldBytes0[1] - 1);

    if (base == 0.0f || scalar == 0.0f) {
        return 0.0f;
    }

    return scalar * (this->m_player->m_playerFieldCombatRating1[rating] / base);
}

// VERIFY: 1e41332c manual
// OFFSET: 0x006DEDD0
int32_t InventoryUpdate(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 1;
    }

    uint32_t slot = (offset - offsetof(CGPlayerLocalData, m_playerFieldInvSlotHead)) / sizeof(WOWGUID);
    WOWGUID item;
    item.m_guid = player->m_bag.GetItemGuid(slot);

    if (*static_cast<const WOWGUID*>(value) != item) {
        CGGameUI::UnlockItem(item);
    }

    if (slot == 15 || slot == 16) {
        CGActionBar::UpdateAttackAction();
    } else if (slot == 17) {
        CGActionBar::UpdateRangedAttackAction();
    }

    if (slot <= 18) {
        player->CountEquippedGems();

        if (item) {
            FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_EQUIPMENT_CHANGED, "%d%d", slot + 1, 1);
        } else {
            FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_EQUIPMENT_CHANGED, "%d", slot + 1);
        }

        return 1;
    }

    if ((slot >= 39 && slot <= 66) || slot - 67 <= 6) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYERBANKSLOTS_CHANGED, "%d", slot - 38);
    }

    return 1;
}

// VERIFY: da16b28f manual
// OFFSET: 0x006D1C70
void PlayerNameGuildCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->UpdateUnitNameText();
    }
}

// VERIFY: 512fb7ae manual
// OFFSET: 0x006DB6A0
const char* CGPlayer_C::GetPlayerGuildNameText() {
    if (!this->m_player->m_playerGuildid) {
        return nullptr;
    }

    WOWGUID guid = this->m_obj->m_guid;
    GuildStats_C* stats = g_guildInfoCache.GetRecord(this->m_player->m_playerGuildid, guid, PlayerNameGuildCallback, this, false);

    if (!stats) {
        return nullptr;
    }

    return stats->m_name;
}

// VERIFY: 00424069 manual
// OFFSET: 0x00594760
uint16_t CGPlayer_C::GetSkillStep(int32_t index) const {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return 0;
    }

    return this->m_player->m_playerSkillInfo[index].m_step;
}

// VERIFY: 975de1f5 manual
// OFFSET: 0x006D6560
static int32_t SoulStoneCompare(const CGItem_C* item, void* param) {
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    for (uint32_t i = 0; i < 5; i++) {
        int32_t spellId = stats->m_spellId[i];

        if (spellId < g_spellDB.m_minID || spellId > g_spellDB.m_maxID || !g_spellDB.m_recordsById[spellId - g_spellDB.m_minID] || stats->m_spellTrigger[i]) {
            continue;
        }

        SpellRec spell;
        g_spellDB.GetRecord(spellId, &spell);

        for (uint32_t j = 0; j < 3; j++) {
            if (spell.m_effect[j] == 94) {
                return 1;
            }
        }
    }

    return 0;
}

// VERIFY: 9d69e881 auto
// OFFSET: 0x006D6640
CGItem_C* CGPlayer_C::GetSoulstone() {
    return this->m_bag.FindItem(&SoulStoneCompare, nullptr, 0);
}

// VERIFY: 1ab76c61 auto
// OFFSET: 0x006D6660
int32_t CGPlayer_C::GetSkillLineForSlot(int32_t hand) {
    int32_t spell = 0;

    if (hand < 3) {
        const CGUnitVirtualItem* item = this->GetVirtualItem(hand, false);

        if (!item || item->m_classID == 2) {
            uint32_t subclass = item ? item->m_subclassID : ClientDBGetUnarmedWeapon();

            if (subclass < s_weaponSkills.Count()) {
                spell = s_weaponSkills.m_data[subclass];
            }
        }
    }

    SkillLineAbilityRec* ability = SpellTableLookupAbility(this->m_unit->m_unitFieldBytes0[0], this->m_unit->m_unitFieldBytes0[1], spell);

    if (ability && ability->m_spell == spell) {
        return ability->m_skillLine;
    }

    return 0;
}

// VERIFY: caa91b18 manual
// OFFSET: 0x006CE140
static bool AreaTriggerContains(const AreaTriggerRec* rec, const C3Vector& position) {
    if (rec->m_continentID != s_areaTriggerMapID) {
        return false;
    }

    if (rec->m_radius == 0.0f) {
        CAaBox box;
        box.t.x = rec->m_boxLength * 0.5f;
        box.b.x = -box.t.x;
        box.t.y = rec->m_boxWidth * 0.5f;
        box.b.y = -box.t.y;
        box.t.z = rec->m_boxHeight * 0.5f;
        box.b.z = -box.t.z;

        C44Matrix matrix;
        matrix.Translate(*reinterpret_cast<const C3Vector*>(rec->m_pos));
        matrix.RotateAroundZ(rec->m_boxYaw);
        C3Vector local = position * matrix.AffineInverse();

        return box.ContainsPointExclusive(local);
    }

    float dx = rec->m_pos[0] - position.x;
    float dy = rec->m_pos[1] - position.y;
    float dz = rec->m_pos[2] - position.z;

    return dy * dy + dz * dz + dx * dx <= rec->m_radius * rec->m_radius;
}

// VERIFY: c8df8e18 manual
// OFFSET: 0x006D2F70
void AreaTriggersInitialize(uint32_t mapId) {
    int32_t start = -1;
    int32_t i = 0;
    s_currentAreaTrigger = nullptr;
    s_areaTriggerMapID = mapId;
    s_areaTriggerStart = -1;

    for (; i < g_areaTriggerDB.m_numRecords; i++) {
        AreaTriggerRec* rec = g_areaTriggerDB.GetRecordByIndex(i);

        if (!rec) {
            s_areaTriggerStart = start;
            SErrDisplayAppFatalMessage("", "", 0);
        }

        if (rec->m_continentID == static_cast<int32_t>(mapId)) {
            if (start == -1) {
                start = i;
            }
        } else if (rec->m_continentID > static_cast<int32_t>(mapId)) {
            break;
        }
    }

    s_areaTriggerStart = start;
    s_areaTriggerEnd = i;
    s_areaTriggerTimer = ClientSetTimer(100, AreaTriggerCheck, nullptr);
}

// VERIFY: f1281917 manual
// OFFSET: 0x006DBE30
int32_t AreaTriggerCheck(const void* data, void* param) {
    s_areaTriggerTimer = ClientSetTimer(100, AreaTriggerCheck, nullptr);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player || player->m_unit->m_unitFieldHealth <= 0) {
        return 1;
    }

    if (ClntObjMgrGetMapID() != s_areaTriggerMapID) {
        if (s_areaTriggerTimer) {
            ClientKillTimer(s_areaTriggerTimer, AreaTriggerCheck, "AreaTriggerCheck");
            s_areaTriggerTimer = 0;
        }

        AreaTriggersInitialize(ClntObjMgrGetMapID());
    }

    if (s_areaTriggerStart == -1) {
        return 1;
    }

    C3Vector position = player->GetPosition();

    if (s_currentAreaTrigger) {
        if (AreaTriggerContains(s_currentAreaTrigger, position)) {
            return 1;
        }

        s_currentAreaTrigger = nullptr;
    }

    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return 1;
    }

    for (int32_t i = s_areaTriggerStart; i < s_areaTriggerEnd; i++) {
        AreaTriggerRec* rec = g_areaTriggerDB.GetRecordByIndex(i);

        if (!rec) {
            SErrDisplayAppFatalMessage("", "", 0);
        }

        if (AreaTriggerContains(rec, position)) {
            mover->SendMovementUpdate(MSG_MOVE_HEARTBEAT);

            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_AREA_TRIGGER));
            msg.Put(static_cast<uint32_t>(rec->m_id));
            msg.Finalize();
            ClientServices::Send2(&msg);
            s_currentAreaTrigger = rec;

            return 1;
        }
    }

    return 1;
}

// VERIFY: 7b489e11 manual
// OFFSET: 0x006D7630
int32_t __stdcall CGPlayer_C::CanSummonFriend(WOWGUID guid) {
    if (!guid) {
        return 8;
    }

    if (!CGPlayer_C::IsRAFPlayer(guid)) {
        return 1;
    }

    if (!CGPartyInfo::IsMember(guid) && !IsGuidInMyRaid(guid)) {
        return 9;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        if (static_cast<int32_t>(player->m_unit->m_unitFieldLevel) > 60) {
            return 10;
        }
    } else {
        CGPartyMemberState* stats = CGPartyInfo::GetRemoteStats(guid);

        if (!stats) {
            stats = CGRaidInfo::GetRemoteStats(guid);

            if (!stats) {
                return 9;
            }
        }

        if (!(stats->m_flags & 0x1)) {
            return 13;
        }

        if (stats->m_level > 60) {
            return 10;
        }
    }

    return Spell_C_GetSpellCooldown(CGSpellBook::m_summonFriendSpell, 0, nullptr, nullptr, nullptr) ? 11 : 0;
}

// VERIFY: 0c3db187 auto
// OFFSET: 0x006D7720
int32_t CGPlayer_C::UpdateMountedCollisionBox(float mountHeight, int32_t force) {
    CreatureDisplayInfoRec* displayInfo = g_creatureDisplayInfoDB.GetRecord(this->m_unit->m_unitFieldNativedisplayid);
    CreatureModelDataRec* modelData = displayInfo ? g_creatureModelDataDB.GetRecord(displayInfo->m_modelID) : nullptr;

    if (!modelData) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    float modelScale;
    float scale = this->GetScaledModelScale(displayInfo, modelData, &modelScale);
    float height = this->m_mountScale * mountHeight + modelData->m_collisionHeight * 0.5f;

    if (height <= modelData->m_collisionHeight) {
        height = modelData->m_collisionHeight;
    } else if (!force && this->IsActivePlayer()) {
        C3Vector position = this->GetPosition();
        CAaBox box(position);
        box.b.z = this->m_movementData.m_collisionHeight + box.b.z;
        box.t.z = height * scale + box.t.z;
        box.b.x = box.b.x - this->m_movementData.m_collisionRadius;
        box.t.x = box.t.x + this->m_movementData.m_collisionRadius;
        box.b.y = box.b.y - this->m_movementData.m_collisionRadius;
        box.t.y = box.t.y + this->m_movementData.m_collisionRadius;

        if (this->m_movementData.CollisionBoxDoesntFit(box)) {
            return 0;
        }
    }

    this->m_movementData.SetCollisionBox(this->m_movementData.m_collisionRadius + this->m_movementData.m_collisionRadius, height, scale, modelScale, force);

    return 1;
}

// VERIFY: 5cad3e09 auto
// OFFSET: 0x006D78C0
int32_t CGPlayer_C::UpdateUnitCollisionBox(int32_t a2, int32_t a3) {
    CreatureDisplayInfoRec* displayInfo = g_creatureDisplayInfoDB.GetRecord(this->m_unit->m_unitFieldNativedisplayid);
    CreatureModelDataRec* modelData = displayInfo ? g_creatureModelDataDB.GetRecord(displayInfo->m_modelID) : nullptr;

    if (!modelData) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    float modelScale;
    float scale = this->GetScaledModelScale(displayInfo, modelData, &modelScale);

    if (fabsf(modelData->m_collisionWidth) < 0.00000095367432f || fabsf(modelData->m_collisionHeight) < 0.00000095367432f) {
        SysMsgPrintf(STATUS_ERROR, 0x10, "NOCOLLISIONBOX|%s", modelData->m_modelName);
        return 1;
    }

    if (CMath::fequal(scale, 0.0f)) {
        SysMsgPrintf(STATUS_ERROR, 2, "ZEROSCALEUNIT|%d|%s", this->m_obj->m_entryID, this->GetUnitName(nullptr, 1));
        return 1;
    }

    if (modelData->m_collisionWidth + 0.00000011920929f < 0.4167f) {
        SysMsgPrintf(STATUS_ERROR, 2, "CreatureModelData record %d (%s) collision width is too small.", displayInfo->m_modelID, modelData->m_modelName);
    }

    if (!a2 && this->IsActivePlayer() && this->m_movementData.CollisionBoxDoesntFit(modelData->m_collisionWidth, modelData->m_collisionHeight, scale)) {
        return 0;
    }

    this->m_movementData.SetCollisionBox(modelData->m_collisionWidth, modelData->m_collisionHeight, scale, modelScale, a3);
    return 1;
}

// VERIFY: 2a4f40b4 auto
// OFFSET: 0x006DF7A0
bool CGPlayer_C::OnGuildChanged(int32_t a2) {
    this->UpdateUnitNameText();
    FrameScript_SignalEvent(SCRIPT_EVENT_TABARD_CANSAVE_CHANGED, nullptr);
    WOWGUID guid = this->m_obj->m_guid;
    Script_SendUnitSignal(guid, SCRIPT_EVENT_PLAYER_GUILD_UPDATE);

    if (a2 && ClntObjMgrGetActivePlayer() == this->m_obj->m_guid) {
        CGCalendar::GuildChanged();
    }

    VisibleItem_C* tabard = &this->m_player->m_playerVisibleItems[18];

    if (!tabard || tabard->GetInventoryType() != 19) {
        return false;
    }

    int32_t displayId = tabard->GetDisplayID();

    if (!displayId) {
        return false;
    }

    if (this->CanEquipItemsInThisForm()) {
        this->LoadGuildTextures(displayId);
    }

    return true;
}

// VERIFY: bdcb68fa manual
// OFFSET: 0x006E73B0
TSGrowableArray<int32_t>* CGPlayer_C::GetTradeSkillSpells(int32_t skillLine) {
    TRADESKILLLINE* line = this->m_tradeSkillLines.Ptr(skillLine, HASHKEY_NONE());

    return line ? &line->m_spells : nullptr;
}

// VERIFY: 1d2b7f27 auto
// OFFSET: 0x006CF2E0
uint32_t GetVisibleAuraEffectDisplay() {
    return s_visibleAuraEffectDisplay;
}

// VERIFY: 189d8dee auto
// OFFSET: 0x006CF2F0
uint32_t GetVisibleAuraEffectFlags() {
    return s_visibleAuraEffectFlags;
}

// VERIFY: 07b73796 auto
// OFFSET: 0x005EEB70
void PlayerShutdownSounds() {
}

// VERIFY: a218f4b9 manual
// OFFSET: 0x006DFF90
void CGPlayer_C::AutoEquipItemToSlot(CGItem_C* item, uint32_t slot, int32_t ignoreBind) {
    if (!item) {
        return;
    }

    WOWGUID guid = item->m_obj->m_guid;

    if (CGTradeInfo::IsItemBeingTraded(guid)) {
        CGGameUI::DisplayError(ERR_EQUIP_TRADE_ITEM);
        return;
    }

    if (!CGGameUI::CanPerformAction(22)) {
        return;
    }

    CDataStore msg;

    if (!item->IsBound() && !ignoreBind) {
        ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, guid, &AutoEquipItemCallback, nullptr, false);

        if (!stats) {
            uint32_t index = FindEmptySwapIndex();
            s_itemSwaps.m_data[index].m_item = item;
            s_itemSwaps.m_data[index].m_srcSlot = slot;
            s_itemSwaps.m_data[index].m_srcContainer = item->m_item->m_itemFieldContained;
            s_itemSwaps.m_data[index].m_itemId = item->m_obj->m_entryID;
            return;
        }

        int32_t error;

        if (stats->m_bonding == 2 && this->CanUseItem(stats, error)) {
            uint32_t index = FindEmptySwapIndex();
            s_itemSwaps.m_data[index].m_item = item;
            s_itemSwaps.m_data[index].m_srcSlot = slot;
            s_itemSwaps.m_data[index].m_srcContainer = item->m_item->m_itemFieldContained;
            s_itemSwaps.m_data[index].m_itemId = item->m_obj->m_entryID;
            FrameScript_SignalEvent(SCRIPT_EVENT_EQUIP_BIND_CONFIRM, "%d", index);
            return;
        }
    }

    CGGameUI::LockItem(guid);
    msg.Put(static_cast<uint32_t>(CMSG_AUTO_EQUIP_ITEM_SLOT));
    msg.Put(guid.m_guid);
    msg.Put(static_cast<uint8_t>(slot));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: ea3302ea manual
// OFFSET: 0x006E01A0
void CGPlayer_C::ClearPendingEquip(uint32_t index, int32_t equip) {
    if (index >= s_itemSwaps.m_count) {
        return;
    }

    ITEMSWAP* swap = &s_itemSwaps.m_data[index];

    if (equip) {
        if (swap->m_item) {
            this->AutoEquipItemToSlot(swap->m_item, swap->m_srcSlot, 1);
        } else if (swap->m_dstContainer) {
            this->SwapItems(WOWGUID(), swap->m_srcContainer, swap->m_srcSlot, swap->m_dstContainer, swap->m_dstSlot, 1);
        } else {
            this->AutoEquipItem(swap->m_srcContainer, swap->m_srcSlot, 1);
        }
    } else {
        CGObject_C* source = ClntObjMgrObjectPtr(swap->m_srcContainer, TYPEMASK_OBJECT, __FILE__, __LINE__);

        if (source && source->GetBag()) {
            WOWGUID item;
            item.m_guid = source->GetBag()->GetItemGuid(swap->m_srcSlot);
            CGGameUI::UnlockItem(item);
        }

        CGObject_C* destination = ClntObjMgrObjectPtr(swap->m_dstContainer, TYPEMASK_OBJECT, __FILE__, __LINE__);

        if (destination && destination->GetBag()) {
            WOWGUID item;
            item.m_guid = destination->GetBag()->GetItemGuid(swap->m_dstSlot);
            CGGameUI::UnlockItem(item);
        }
    }

    swap = &s_itemSwaps.m_data[index];
    swap->m_srcContainer = WOWGUID();
    swap->m_dstContainer = WOWGUID();
    swap->m_itemId = 0;
    swap->m_item = nullptr;
    swap->m_srcSlot = 0xFFFFFFFF;
    swap->m_dstSlot = 0xFFFFFFFF;
}

// VERIFY: 71adbe9a auto
// OFFSET: 0x006E0300
void PlayerClientShutdown() {
    CGPlayer_C::PlayerShutdownMessageHandlers();
    PlayerShutdownSounds();

    if (s_areaTriggerTimer) {
        ClientKillTimer(s_areaTriggerTimer, AreaTriggerCheck, "AreaTriggerCheck");
        s_areaTriggerTimer = 0;
    }
}

// VERIFY: 8cf14677 auto
// OFFSET: 0x006CE480
void CGPlayer_C::PlayerShutdownMessageHandlers() {
    ClientServices::ClearMessageHandler(SMSG_MOUNT_RESULT);
    ClientServices::ClearMessageHandler(SMSG_DISMOUNT_RESULT);
    ClientServices::ClearMessageHandler(SMSG_INVENTORY_CHANGE_FAILURE);
    ClientServices::ClearMessageHandler(SMSG_OPEN_CONTAINER);
    ClientServices::ClearMessageHandler(SMSG_ITEM_PUSH_RESULT);
    ClientServices::ClearMessageHandler(SMSG_VENDOR_INVENTORY);
    ClientServices::ClearMessageHandler(SMSG_BUY_FAILED);
    ClientServices::ClearMessageHandler(SMSG_BUY_SUCCEEDED);
    ClientServices::ClearMessageHandler(SMSG_SELL_ITEM);
    ClientServices::ClearMessageHandler(SMSG_ITEM_REFUND_INFO_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_ITEM_PURCHASE_REFUND_RESULT);
    ClientServices::ClearMessageHandler(SMSG_LOOT_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_LOOT_RELEASE);
    ClientServices::ClearMessageHandler(SMSG_LOOT_REMOVED);
    ClientServices::ClearMessageHandler(SMSG_LOOT_SLOT_CHANGED);
    ClientServices::ClearMessageHandler(SMSG_LOOT_MONEY_NOTIFY);
    ClientServices::ClearMessageHandler(SMSG_LOOT_ITEM_NOTIFY);
    ClientServices::ClearMessageHandler(SMSG_LOOT_CLEAR_MONEY);
    ClientServices::ClearMessageHandler(SMSG_LEARNED_SPELL);
    ClientServices::ClearMessageHandler(SMSG_UNLEARNED_SPELLS);
    ClientServices::ClearMessageHandler(SMSG_SUPERCEDED_SPELLS);
    ClientServices::ClearMessageHandler(SMSG_SEND_KNOWN_SPELLS);
    ClientServices::ClearMessageHandler(SMSG_UPDATE_ACTION_BUTTONS);
    ClientServices::ClearMessageHandler(SMSG_GROUP_INVITE);
    ClientServices::ClearMessageHandler(SMSG_GROUP_CANCEL);
    ClientServices::ClearMessageHandler(SMSG_GROUP_DECLINE);
    ClientServices::ClearMessageHandler(SMSG_GROUP_UNINVITE);
    ClientServices::ClearMessageHandler(SMSG_GROUP_SET_LEADER);
    ClientServices::ClearMessageHandler(SMSG_GROUP_DESTROYED);
    ClientServices::ClearMessageHandler(SMSG_PARTY_COMMAND_RESULT);
    ClientServices::ClearMessageHandler(SMSG_GROUP_LIST);
    ClientServices::ClearMessageHandler(SMSG_REAL_GROUP_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_QUEST_GIVER_QUEST_LIST_MESSAGE);
    ClientServices::ClearMessageHandler(SMSG_QUEST_GIVER_INVALID_QUEST);
    ClientServices::ClearMessageHandler(SMSG_QUEST_GIVER_QUEST_DETAILS);
    ClientServices::ClearMessageHandler(SMSG_QUEST_GIVER_REQUEST_ITEMS);
    ClientServices::ClearMessageHandler(SMSG_QUEST_GIVER_OFFER_REWARD_MESSAGE);
    ClientServices::ClearMessageHandler(SMSG_QUEST_GIVER_QUEST_COMPLETE);
    ClientServices::ClearMessageHandler(SMSG_QUEST_GIVER_QUEST_FAILED);
    ClientServices::ClearMessageHandler(SMSG_QUEST_GIVER_STATUS);
    ClientServices::ClearMessageHandler(SMSG_QUEST_GIVER_STATUS_MULTIPLE);
    ClientServices::ClearMessageHandler(SMSG_QUEST_LOG_FULL);
    ClientServices::ClearMessageHandler(SMSG_TRAINER_LIST);
    ClientServices::ClearMessageHandler(SMSG_TRAINER_BUY_FAILED);
    ClientServices::ClearMessageHandler(SMSG_SET_PROFICIENCY);
    ClientServices::ClearMessageHandler(SMSG_RESURRECT_REQUEST);
    ClientServices::ClearMessageHandler(SMSG_PLAYER_SKINNED);
    ClientServices::ClearMessageHandler(SMSG_INSPECT_RESULTS_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_INSPECT_TALENT);
    ClientServices::ClearMessageHandler(SMSG_READ_ITEM_RESULT_OK);
    ClientServices::ClearMessageHandler(SMSG_READ_ITEM_RESULT_FAILED);
    ClientServices::ClearMessageHandler(SMSG_CANCEL_COMBAT);
    ClientServices::ClearMessageHandler(SMSG_TAXI_NODE_STATUS);
    ClientServices::ClearMessageHandler(SMSG_SHOW_TAXI_NODES);
    ClientServices::ClearMessageHandler(SMSG_ACTIVATE_TAXI_REPLY);
    ClientServices::ClearMessageHandler(SMSG_GUILD_INVITE);
    ClientServices::ClearMessageHandler(SMSG_GUILD_DECLINE);
    ClientServices::ClearMessageHandler(SMSG_GUILD_INFO);
    ClientServices::ClearMessageHandler(SMSG_GUILD_EVENT);
    ClientServices::ClearMessageHandler(SMSG_GUILD_COMMAND_RESULT);
    ClientServices::ClearMessageHandler(MSG_SAVE_GUILD_EMBLEM);
    ClientServices::ClearMessageHandler(MSG_TABARDVENDOR_ACTIVATE);
    ClientServices::ClearMessageHandler(SMSG_PETITION_SHOW_LIST);
    ClientServices::ClearMessageHandler(SMSG_PETITION_SHOW_SIGNATURES);
    ClientServices::ClearMessageHandler(SMSG_PETITION_SIGN_RESULTS);
    ClientServices::ClearMessageHandler(MSG_PETITION_DECLINE);
    ClientServices::ClearMessageHandler(SMSG_TURN_IN_PETITION_RESULT);
    ClientServices::ClearMessageHandler(MSG_PETITION_RENAME);
    ClientServices::ClearMessageHandler(SMSG_BIND_POINT_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_BINDZONEREPLY);
    ClientServices::ClearMessageHandler(SMSG_EMOTE);
    ClientServices::ClearMessageHandler(SMSG_PLAYER_BOUND);
    ClientServices::ClearMessageHandler(SMSG_PLAYERBINDERROR);
    ClientServices::ClearMessageHandler(SMSG_NEW_TAXI_PATH);
    ClientServices::ClearMessageHandler(SMSG_PET_NAME_INVALID);
    ClientServices::ClearMessageHandler(SMSG_EXPLORATION_EXPERIENCE);
    ClientServices::ClearMessageHandler(SMSG_PARTY_MEMBER_STATS);
    ClientServices::ClearMessageHandler(SMSG_PARTY_MEMBER_STATS_FULL);
    ClientServices::ClearMessageHandler(SMSG_QUEST_UPDATE_FAILED);
    ClientServices::ClearMessageHandler(SMSG_QUEST_UPDATE_FAILED_TIMER);
    ClientServices::ClearMessageHandler(SMSG_QUEST_UPDATE_COMPLETE);
    ClientServices::ClearMessageHandler(SMSG_QUEST_UPDATE_ADD_KILL);
    ClientServices::ClearMessageHandler(SMSG_QUEST_UPDATE_ADD_PVP_CREDIT);
    ClientServices::ClearMessageHandler(SMSG_QUEST_CONFIRM_ACCEPT);
    ClientServices::ClearMessageHandler(SMSG_SHOW_BANK);
    ClientServices::ClearMessageHandler(SMSG_BUY_BANK_SLOT_RESULT);
    ClientServices::ClearMessageHandler(SMSG_LEVEL_UP_INFO);
    ClientServices::ClearMessageHandler(MSG_MINIMAP_PING);
    ClientServices::ClearMessageHandler(SMSG_TRIGGER_CINEMATIC);
    ClientServices::ClearMessageHandler(SMSG_ITEM_TIME_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_ITEM_ENCHANT_TIME_UPDATE);
    ClientServices::ClearMessageHandler(MSG_RANDOM_ROLL);
    ClientServices::ClearMessageHandler(SMSG_FISH_NOT_HOOKED);
    ClientServices::ClearMessageHandler(SMSG_FISH_ESCAPED);
    ClientServices::ClearMessageHandler(SMSG_FORCEACTIONSHOW);
    ClientServices::ClearMessageHandler(SMSG_GOD_MODE);
    ClientServices::ClearMessageHandler(SMSG_PETGODMODE);
    ClientServices::ClearMessageHandler(SMSG_IGNORE_REQUIREMENTS_CHEAT);
    ClientServices::ClearMessageHandler(SMSG_IGNORE_DIMINISHING_RETURNS_CHEAT);
    ClientServices::ClearMessageHandler(SMSG_CLEAR_FAR_SIGHT_IMMEDIATE);
    ClientServices::ClearMessageHandler(SMSG_CHAT_WRONG_FACTION);
    ClientServices::ClearMessageHandler(SMSG_CHAT_PLAYER_NOTFOUND);
    ClientServices::ClearMessageHandler(SMSG_CHAT_RESTRICTED);
    ClientServices::ClearMessageHandler(SMSG_CHAT_NOT_IN_PARTY);
    ClientServices::ClearMessageHandler(SMSG_GM_TICKET_GET_SYSTEM_STATUS);
    ClientServices::ClearMessageHandler(SMSG_SPIRIT_HEALER_CONFIRM);
    ClientServices::ClearMessageHandler(MSG_TALENT_WIPE_CONFIRM);
    ClientServices::ClearMessageHandler(SMSG_BINDER_CONFIRM);
    ClientServices::ClearMessageHandler(SMSG_SPELL_EXECUTE_LOG);
    ClientServices::ClearMessageHandler(SMSG_SPELL_MISS_LOG);
    ClientServices::ClearMessageHandler(SMSG_SPELL_DAMAGE_SHIELD);
    ClientServices::ClearMessageHandler(SMSG_SPELL_INSTAKILL_LOG);
    ClientServices::ClearMessageHandler(SMSG_SPELL_NON_MELEE_DAMAGE_LOG);
    ClientServices::ClearMessageHandler(SMSG_SPELL_HEAL_LOG);
    ClientServices::ClearMessageHandler(SMSG_SPELL_ENERGIZE_LOG);
    ClientServices::ClearMessageHandler(SMSG_SPELL_DISPELL_LOG);
    ClientServices::ClearMessageHandler(SMSG_SPELL_STEAL_LOG);
    ClientServices::ClearMessageHandler(SMSG_SPELL_BREAK_LOG);
    ClientServices::ClearMessageHandler(SMSG_RESURRECT_FAILED);
    ClientServices::ClearMessageHandler(SMSG_SPELL_OR_DAMAGE_IMMUNE);
    ClientServices::ClearMessageHandler(MSG_QUEST_PUSH_RESULT);
    ClientServices::ClearMessageHandler(SMSG_DAMAGE_CALC_LOG);
    ClientServices::ClearMessageHandler(SMSG_RAID_GROUP_ONLY);
    ClientServices::ClearMessageHandler(SMSG_INSTANCE_LOCK_WARNING_QUERY);
    ClientServices::ClearMessageHandler(SMSG_LOOT_START_ROLL);
    ClientServices::ClearMessageHandler(SMSG_LOOT_ALL_PASSED);
    ClientServices::ClearMessageHandler(SMSG_LOOT_ROLL_WON);
    ClientServices::ClearMessageHandler(SMSG_LOOT_ROLL);
    ClientServices::ClearMessageHandler(SMSG_LOOT_MASTER_LIST);
    ClientServices::ClearMessageHandler(SMSG_TEST_DROP_RATE_RESULT);
    ClientServices::ClearMessageHandler(SMSG_SUMMON_REQUEST);
    ClientServices::ClearMessageHandler(SMSG_SUMMON_CANCEL);
    ClientServices::ClearMessageHandler(SMSG_PLAY_TIME_WARNING);
    ClientServices::ClearMessageHandler(SMSG_ARENA_TEAM_INVITE);
    ClientServices::ClearMessageHandler(SMSG_ARENA_TEAM_EVENT);
    ClientServices::ClearMessageHandler(SMSG_ARENA_TEAM_COMMAND_RESULT);
    ClientServices::ClearMessageHandler(SMSG_ARENA_ERROR);
    ClientServices::ClearMessageHandler(SMSG_DEATH_RELEASE_LOC);
    ClientServices::ClearMessageHandler(SMSG_FORCED_DEATH_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_TIME_SYNC_REQUEST);
    ClientServices::ClearMessageHandler(SMSG_VOICE_SESSION_ROSTER_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_VOICE_SESSION_LEAVE);
    ClientServices::ClearMessageHandler(SMSG_VOICE_SET_TALKER_MUTED);
    ClientServices::ClearMessageHandler(SMSG_VOICE_PARENTAL_CONTROLS);
    ClientServices::ClearMessageHandler(SMSG_CROSSED_INEBRIATION_THRESHOLD);
    ClientServices::ClearMessageHandler(SMSG_COMPLAINT_RESULT);
    ClientServices::ClearMessageHandler(SMSG_FEATURE_SYSTEM_STATUS);
    ClientServices::ClearMessageHandler(SMSG_AVAILABLE_VOICE_CHANNEL);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_COMMAND_RESULT);
    ClientServices::ClearMessageHandler(SMSG_REPORT_PVP_AFK_RESULT);
    ClientServices::ClearMessageHandler(SMSG_GROUP_ACTION_THROTTLED);
    ClientServices::ClearMessageHandler(SMSG_SEND_UNLEARN_SPELLS);
    ClientServices::ClearMessageHandler(SMSG_CONVERT_RUNE);
    ClientServices::ClearMessageHandler(SMSG_RESYNC_RUNES);
    ClientServices::ClearMessageHandler(SMSG_ADD_RUNE_POWER);
    ClientServices::ClearMessageHandler(SMSG_BATTLEGROUND_INFO_THROTTLED);
    ClientServices::ClearMessageHandler(SMSG_UPDATE_TALENT_DATA);
    ClientServices::ClearMessageHandler(SMSG_ARENA_TEAM_CHANGE_FAILED_QUEUED);
    ClientServices::ClearMessageHandler(SMSG_QUEST_POI_QUERY_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_TOGGLE_XP_GAIN);
    ClientServices::ClearMessageHandler(SMSG_DEBUG_SERVER_GEO);
    ClientServices::ClearMessageHandler(SMSG_QUEST_FORCE_REMOVED);
    ClientServices::ClearMessageHandler(SMSG_CAMERA_SHAKE);
    ClientServices::ClearMessageHandler(SMSG_SOCKET_GEMS);
    ClientServices::ClearMessageHandler(SMSG_COMBAT_LOG_MULTIPLE);
}

// VERIFY: 06825a0b manual
// OFFSET: 0x006DB320
void CGPlayer_C::SaveTabard(int32_t emblemStyle, int32_t emblemColor, int32_t borderStyle, int32_t borderColor, int32_t background, WOWGUID npc) {
    if (emblemStyle < 0 || emblemColor < 0 || borderStyle < 0 || borderColor < 0 || background < 0 || emblemStyle >= 170 || emblemColor >= 17 || borderStyle >= 6 || borderColor >= 17 || background >= 51) {
        CGGameUI::DisplayError(ERR_GUILDEMBLEM_INVALID_TABARD_COLORS);
        return;
    }

    WOWGUID none = WOWGUID();
    GuildStats_C* stats = g_guildInfoCache.GetRecord(this->m_player->m_playerGuildid, none, nullptr, nullptr, false);

    if (!stats) {
        CGGameUI::DisplayError(ERR_GUILDEMBLEM_NOGUILD);
        return;
    }

    if (emblemStyle == stats->m_emblemStyle && emblemColor == stats->m_emblemColor && borderStyle == stats->m_borderStyle && borderColor == stats->m_borderColor && background == stats->m_background) {
        CGGameUI::DisplayError(ERR_GUILDEMBLEM_SAME);
        return;
    }

    if (this->m_player->m_playerGuildrank) {
        CGGameUI::DisplayError(ERR_GUILDEMBLEM_NOTGUILDMASTER);
        return;
    }

    if (this->GetCoinage() < GuildGetTabardCost()) {
        CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_SAVE_GUILD_EMBLEM));
    msg.Put(npc.m_guid);
    msg.Put(static_cast<uint32_t>(emblemStyle));
    msg.Put(static_cast<uint32_t>(emblemColor));
    msg.Put(static_cast<uint32_t>(borderStyle));
    msg.Put(static_cast<uint32_t>(borderColor));
    msg.Put(static_cast<uint32_t>(background));
    msg.Finalize();
    ClientServices::Send2(&msg);
    s_tabardSavePending = 1;
    FrameScript_SignalEvent(SCRIPT_EVENT_TABARD_SAVE_PENDING, nullptr);
}

// VERIFY: d10a2943 auto
// OFFSET: 0x006DB510
bool CGPlayer_C::LoadGuildTextures(int32_t displayId) {
    ItemDisplayInfoRec rec;

    if (!this->m_characterComponent || !g_itemDisplayInfoDB.GetRecordByIndex(displayId, &rec) || (this->m_characterComponent->m_flags & 0x40)) {
        return false;
    }

    this->m_characterComponent->AddItemBySlot(INVSLOT_TABARD, displayId, 0);

    if (rec.m_flags & 0x1) {
        int32_t emblemStyle;
        int32_t emblemColor;
        int32_t borderStyle;
        int32_t borderColor;
        int32_t background;

        if (!GuildGetGuildTabard(this->m_obj->m_guid, this->m_player->m_playerGuildid, CGPlayer_C::TabardDrawCallback, this, emblemStyle, emblemColor, borderStyle, borderColor, background)) {
            return false;
        }

        this->m_characterComponent->ApplyGuildColor(emblemStyle, emblemColor, borderStyle, borderColor, background);
    }

    return true;
}

// VERIFY: 90dc58ea manual
// OFFSET: 0x006D8870
int32_t Packet_SMSG_GROUP_LIST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t groupType;
    msg->Get(groupType);
    bool isRaid = (groupType & 0x2) != 0;
    uint8_t isBattleground = groupType & 0x1;
    uint8_t subgroup;
    msg->Get(subgroup);
    uint8_t assignments;
    msg->Get(assignments);
    uint8_t roles;
    msg->Get(roles);
    uint8_t lfgFlags = 0;
    uint32_t lfgSlot = 0;

    if (groupType & 0x8) {
        msg->Get(lfgFlags);
        msg->Get(lfgSlot);
    }

    WOWGUID group;
    msg->Get(group.m_guid);
    uint32_t sequence;
    msg->Get(sequence);

    if (group && sequence) {
        if (groupType & 0x1) {
            if (CGPlayer_C::s_lastUpdatePVPGroup == group && CGPlayer_C::s_lastUpdatePVPTime && CGPlayer_C::s_lastUpdatePVPSequence >= sequence && static_cast<int32_t>(static_cast<uint32_t>(::time(nullptr)) - CGPlayer_C::s_lastUpdatePVPTime - 60) < 0) {
                msg->m_read = msg->m_size;
                return 1;
            }

            CGPlayer_C::s_lastUpdatePVPGroup = group;
            CGPlayer_C::s_lastUpdatePVPSequence = sequence;
            CGPlayer_C::s_lastUpdatePVPTime = static_cast<uint32_t>(::time(nullptr));
        } else {
            if (CGPlayer_C::s_lastUpdateGroup == group && CGPlayer_C::s_lastUpdateTime && CGPlayer_C::s_lastUpdateSequence >= sequence && static_cast<int32_t>(static_cast<uint32_t>(::time(nullptr)) - CGPlayer_C::s_lastUpdateTime - 60) < 0) {
                msg->m_read = msg->m_size;
                return 1;
            }

            CGPlayer_C::s_lastUpdateGroup = group;
            CGPlayer_C::s_lastUpdateSequence = sequence;
            CGPlayer_C::s_lastUpdateTime = static_cast<uint32_t>(::time(nullptr));
        }
    }

    uint32_t memberCount;
    msg->Get(memberCount);
    WOWGUID oldMembers[4] = {};
    WOWGUID partyGuids[4] = {};
    int32_t wasInGroup = 0;
    WOWGUID raidGuids[40] = {};
    uint32_t dungeonDifficulty = CGGameUI::GetCurrentDungeonDifficulty();
    uint32_t raidDifficulty = CGGameUI::GetCurrentRaidDifficulty();
    uint8_t oldRoles = CGPartyInfo::m_playerRole;
    uint32_t oldCount = 0;
    WOWGUID oldLeader = CGPartyInfo::m_leader;
    CGPartyMemberState oldStates[4];
    PartyReadyCheckInfo oldReadyChecks[4];

    for (uint32_t i = 0; i < 4; i++) {
        oldMembers[i] = CGGameUI::GetPartyMember(i);

        if (!oldMembers[i]) {
            continue;
        }

        const CGPartyMemberState* state = CGPartyInfo::m_members[i] ? &s_partyMemberStates[i] : nullptr;
        memcpy(&oldStates[i], state, sizeof(CGPartyMemberState));
        CGPartyInfo::GetReadyCheckInfo(&oldReadyChecks[i], i);
        oldCount++;
        wasInGroup = 1;
    }

    CGPartyInfo::RemoveAll(0);

    if (CGRaidInfo::m_numMembers) {
        wasInGroup = 1;
    }

    uint32_t partyCount = 0;
    int32_t joined = 0;
    uint32_t subgroups[40];
    uint8_t raidFlags[40];
    uint32_t raidRoles[40];
    uint8_t online[41];
    char name[48];

    for (uint32_t j = 0; j < memberCount; j++) {
        msg->GetString(name, sizeof(name));
        WOWGUID guid;
        msg->Get(guid.m_guid);
        msg->Get(online[j]);
        uint8_t memberSubgroup;
        msg->Get(memberSubgroup);
        uint8_t memberFlags;
        msg->Get(memberFlags);
        uint8_t memberRoles;
        msg->Get(memberRoles);

        if (guid == ClntObjMgrGetActivePlayer()) {
            continue;
        }

        subgroups[j] = memberSubgroup;
        raidGuids[j] = guid;
        raidFlags[j] = memberFlags;
        raidRoles[j] = memberRoles;

        if (memberSubgroup != subgroup || partyCount >= 4) {
            continue;
        }

        partyGuids[partyCount] = guid;
        uint32_t i = 4;

        if (wasInGroup) {
            for (i = 0; i < 4; i++) {
                if (oldMembers[i] == guid) {
                    break;
                }
            }

            if (!isRaid && i == 4) {
                CGGameUI::DisplayError(ERR_JOINED_GROUP_S, name);
                joined = 1;
            }
        } else {
            joined = 1;
        }

        if (guid == ClntObjMgrGetActivePlayer()) {
            SErrDisplayAppFatalMessage("", "", 0);
        }

        const CGPartyMemberState* state = nullptr;
        const PartyReadyCheckInfo* readyCheck = nullptr;

        if (i < 4) {
            state = &oldStates[i];
            readyCheck = &oldReadyChecks[i];
        }

        CGPartyInfo::AddMember(guid, online[j] != 0, state, memberFlags, memberRoles, readyCheck);
        partyCount++;
    }

    WOWGUID leader;
    msg->Get(leader.m_guid);

    if (!isBattleground) {
        if (isRaid) {
            bool converted = CGPartyInfo::m_realNumMembers && !CGRaidInfo::m_realNumMembers;
            CGRaidInfo::UpdateRealStats(memberCount ? memberCount + 1 : 0, leader);

            if (converted && CGRaidInfo::m_realNumMembers) {
                Script_SendUniqueSignal(SCRIPT_EVENT_PARTY_CONVERTED_TO_RAID);
            }
        } else {
            CGRaidInfo::UpdateRealStats(0, leader);
        }

        CGPartyInfo::UpdateRealStats(partyCount, leader);
    }

    CGPartyInfo::SetLeader(leader);
    CGPartyInfo::SetLfgSlot(lfgSlot);
    CGPartyInfo::SetLfgRestricted((groupType >> 2) & 1);
    CGGameUI::UpdateThreatWarningFromCVar();
    Script_SendUniqueSignal(SCRIPT_EVENT_PARTY_MEMBERS_CHANGED);

    if (joined) {
        SI2::PlaySoundKit("igPlayerInviteAccept", nullptr, nullptr, nullptr);

        if (!wasInGroup && leader == ClntObjMgrGetActivePlayer() && name[0]) {
            CGGameUI::DisplayError(ERR_JOINED_GROUP_S, name);
        }
    }

    if (!isRaid) {
        for (uint32_t j = 0; j < 4; j++) {
            WOWGUID old = oldMembers[j];

            if (!old) {
                continue;
            }

            uint32_t k = 0;

            while (k < memberCount && partyGuids[k] != old) {
                k++;
            }

            if (k != memberCount) {
                continue;
            }

            WOWGUID none = WOWGUID();
            NameCache* oldName = g_nameDBCache.GetRecord(old.m_guid, none, nullptr, nullptr, false);

            if (oldName) {
                CGGameUI::DisplayError(ERR_LEFT_GROUP_S, oldName->m_name);
            }

            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(old, TYPEMASK_UNIT, __FILE__, __LINE__));

            if (unit) {
                const WOWGUID& petGuid = unit->m_unit->m_unitFieldCharm.m_guid ? unit->m_unit->m_unitFieldCharm : unit->m_unit->m_unitFieldSummon;

                if (petGuid && !ClntObjMgrObjectPtr(petGuid, TYPEMASK_UNIT, __FILE__, __LINE__)) {
                    CGGameUI::ClearTarget(petGuid, 1);
                }
            } else {
                CGGameUI::ClearTarget(old, 1);
            }
        }
    }

    if (memberCount) {
        uint8_t lootMethod;
        msg->Get(lootMethod);
        WOWGUID lootMaster;
        msg->Get(lootMaster.m_guid);
        uint8_t lootThreshold;
        msg->Get(lootThreshold);
        uint8_t partyDungeonDifficulty;
        msg->Get(partyDungeonDifficulty);
        uint8_t partyRaidDifficulty;
        msg->Get(partyRaidDifficulty);
        uint8_t dynamicDifficulty;
        msg->Get(dynamicDifficulty);
        CGPartyInfo::m_playerAssignments = assignments;
        CGPartyInfo::m_playerRole = roles;
        CGPartyInfo::m_playerLFGFlags = lfgFlags;
        CGPartyInfo::SetLootMethod(lootMethod, lootMaster, lootThreshold, wasInGroup == 0);
        CGPartyInfo::m_dungeonDifficulty = partyDungeonDifficulty;
        CGPartyInfo::m_raidDifficulty = partyRaidDifficulty;
        CGPartyInfo::m_dynamicDifficulty = dynamicDifficulty;
    } else {
        CGPartyInfo::ResetLootMethod();
    }

    bool wasRaid = CGRaidInfo::m_numMembers != 0;

    if (isRaid) {
        if (memberCount >= 40) {
            SErrDisplayAppFatalMessage("", "", 0);
        }

        raidGuids[memberCount] = ClntObjMgrGetActivePlayer();
        subgroups[memberCount] = subgroup;
        raidFlags[memberCount] = assignments;
        raidRoles[memberCount] = roles;
        online[memberCount] = 1;
        memberCount++;
        CGRaidInfo::UpdateRoster(raidGuids, subgroups, raidFlags, raidRoles, online, memberCount, isBattleground);
    } else {
        if (CGRaidInfo::m_numMembers || !CGPartyInfo::GetNumPartyMembers()) {
            CGRaidInfo::ClearRoster();
        }

        if (!wasInGroup && memberCount) {
            CDataStore target;
            target.Put(static_cast<uint32_t>(MSG_RAID_TARGET_UPDATE));
            target.Put(static_cast<uint8_t>(255));
            target.Finalize();
            ClientServices::Send2(&target);
        }
    }

    if (wasRaid != (CGRaidInfo::m_numMembers != 0) && ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        CDataStore query;
        query.Put(static_cast<uint32_t>(CMSG_QUEST_GIVER_STATUS_MULTIPLE_QUERY));
        query.Finalize();
        ClientServices::Send2(&query);
    }

    if ((!wasInGroup && joined) || (!wasRaid && isRaid)) {
        CGChat::JoiningParty(isRaid, CGRaidInfo::IsPVPRaid());
        CGLookingForGroup::JoiningParty();
    }

    CGGameUI::CheckInstanceDifficultyMessages(dungeonDifficulty, raidDifficulty);

    if (oldRoles != CGPartyInfo::m_playerRole) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_ROLES_ASSIGNED, nullptr);
    }

    CGActionBar::UpdateUsable();
    int32_t lost = static_cast<int32_t>(oldCount - partyCount);
    CGRaidInfo::RosterUpdateRefreshReadyCheck(lost > 0 ? lost : 0, oldLeader);
    return 1;
}

// VERIFY: 2b36ab34 manual
// OFFSET: 0x006D9230
int32_t Packet_MSG_SAVE_GUILD_EMBLEM(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t result;
    msg->Get(result);

    if (result > 5) {
        return 1;
    }

    if (s_guildEmblemErrors[result] != ERR_MAX_VALUE) {
        CGGameUI::DisplayError(s_guildEmblemErrors[result]);
    }

    s_tabardSavePending = 0;

    if (result) {
        FrameScript_SignalEvent(SCRIPT_EVENT_TABARD_SAVE_PENDING, nullptr);
        return 1;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        g_guildInfoCache.Invalidate(player->m_player->m_playerGuildid);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_GUILDTABARD_UPDATE, nullptr);
    return 1;
}

// VERIFY: e750e015 manual
// OFFSET: 0x006D92D0
int32_t Packet_SMSG_GUILD_EVENT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t event = 0;
    uint8_t numStrings;
    char strings[3][1024];
    msg->Get(event);
    msg->Get(numStrings);

    for (uint8_t i = 0; i < numStrings; i++) {
        msg->GetString(strings[i], sizeof(strings[i]));
        TruncateNewLines(strings[i]);
    }

    UIErrors error;
    WOWGUID guid;

    switch (event) {
        case 0:
            error = ERR_GUILD_PROMOTE_SSS;
            break;

        case 1:
            error = ERR_GUILD_DEMOTE_SSS;
            break;

        case 2:
            FrameScript_SignalEvent(SCRIPT_EVENT_GUILD_MOTD, "%s", strings[0]);
            CGGuildInfo::SetMOTD(strings[0]);
            UpdateGuildRoster(1);
            return 1;

        case 3:
            error = ERR_GUILD_JOIN_S;
            msg->Get(guid.m_guid);
            CGAutoCompleteName::AddFlags(guid, 34, 0, strings[0]);
            break;

        case 4:
            error = ERR_GUILD_LEAVE_S;
            msg->Get(guid.m_guid);
            CGAutoCompleteName::RemoveFlags(guid, 2);
            break;

        case 5:
            error = ERR_GUILD_REMOVE_SS;
            CGAutoCompleteName::RemoveFlagsByName(strings[0], 2, 1, 0);

            if (!SStrCmpUTF8I(ClientServices::GetCharacterName(), strings[0], STORM_MAX_STR)) {
                CGAutoCompleteName::RemoveFlagsFromAllNodes(2);
            }

            break;

        case 6:
            error = ERR_GUILD_LEADER_IS_S;
            break;

        case 7:
            error = ERR_GUILD_LEADER_CHANGED_SS;
            break;

        case 8:
            error = ERR_GUILD_DISBANDED;
            CGAutoCompleteName::RemoveFlagsFromAllNodes(2);
            CGGuildBankInfo::SetGuildBanker(WOWGUID());
            memset(CGGuildBankInfo::m_guildBankTabInfo, 0, sizeof(CGGuildBankInfo::m_guildBankTabInfo));
            break;

        case 10: {
            CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

            if (!player) {
                return 1;
            }

            WOWGUID none = WOWGUID();
            GuildStats_C* stats = g_guildInfoCache.GetRecord(player->m_player->m_playerGuildid, none, nullptr, nullptr, false);

            if (!stats) {
                return 1;
            }

            uint32_t rank = SStrToInt(strings[0]);
            char name[64];
            SStrCopy(name, strings[1], sizeof(name));
            ScrubString(name, stats->m_rankNames[rank], 16, sizeof(stats->m_rankNames[rank]), 1);
            stats->m_numRanks = SStrToInt(strings[2]);
            UpdateGuildRoster(1);

            if (player->m_player->m_playerGuildrank != rank) {
                return 1;
            }

            CGGuildInfo::QueryPermissions();

            if (CGGuildBankInfo::m_banker == WOWGUID()) {
                return 1;
            }

            SendGuildBankQueryTabOpcode(CGGuildBankInfo::m_currentTab);
            return 1;
        }

        case 11: {
            CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

            if (!player) {
                return 1;
            }

            WOWGUID none = WOWGUID();
            GuildStats_C* stats = g_guildInfoCache.GetRecord(player->m_player->m_playerGuildid, none, nullptr, nullptr, false);

            if (!stats) {
                return 1;
            }

            stats->m_numRanks = SStrToInt(strings[0]);
            UpdateGuildRoster(1);
            return 1;
        }

        case 12:
            msg->Get(guid.m_guid);
            CGAutoCompleteName::AddFlags(guid, 32, 0, strings[0]);

            if (!g_guildMemberNotifyCVar->m_intValue || !SStrCmp(ClientServices::GetCharacterName(), strings[0], STORM_MAX_STR) || FriendList::s_friendList->IsFriend(guid)) {
                return 1;
            }

            CGGameUI::DisplayError(ERR_FRIEND_ONLINE_SS, strings[0], strings[0]);
            return 1;

        case 13:
            msg->Get(guid.m_guid);
            CGAutoCompleteName::RemoveFlags(guid, 32);

            if (!g_guildMemberNotifyCVar->m_intValue || !SStrCmp(ClientServices::GetCharacterName(), strings[0], STORM_MAX_STR) || FriendList::s_friendList->IsFriend(guid)) {
                return 1;
            }

            CGGameUI::DisplayError(ERR_FRIEND_OFFLINE_S, strings[0]);
            return 1;

        case 14:
            SendGuildBankQueryTabOpcode(CGGuildBankInfo::m_currentTab);
            FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANKBAGSLOTS_CHANGED, nullptr);
            return 1;

        case 15:
            CGGuildBankInfo::m_numTabs++;
            FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANK_UPDATE_TABS, nullptr);
            return 1;

        case 16:
            CGGuildBankInfo::UpdateTabInfo(static_cast<uint8_t>(SStrToInt(strings[0])), strings[1], strings[2]);
            return 1;

        case 17: {
            uint64_t money;
            sscanf(strings[0], "%016I64X", &money);
            CGGuildBankInfo::m_money = money;
            CDataStore query;
            query.Put(static_cast<uint32_t>(MSG_GUILD_BANK_MONEY_WITHDRAWN));
            query.Finalize();
            ClientServices::Send2(&query);
            FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANK_UPDATE_MONEY, nullptr);
            return 1;
        }

        case 18: {
            SendGuildBankQueryTabOpcode(CGGuildBankInfo::m_currentTab);
            CDataStore query;
            query.Put(static_cast<uint32_t>(MSG_GUILD_BANK_MONEY_WITHDRAWN));
            query.Finalize();
            ClientServices::Send2(&query);
            return 1;
        }

        case 19: {
            int32_t tab = atol(strings[0]);
            CGGuildBankInfo::SetTextUpdate(tab, 1);
            FrameScript_SignalEvent(SCRIPT_EVENT_GUILDBANK_TEXT_CHANGED, "%d", tab + 1);
            return 1;
        }

        default:
            error = ERR_GUILD_INTERNAL;
            break;
    }

    switch (numStrings) {
        case 1:
            CGGameUI::DisplayError(error, strings[0]);
            break;

        case 2:
            CGGameUI::DisplayError(error, strings[0], strings[1]);
            break;

        case 3:
            CGGameUI::DisplayError(error, strings[0], strings[1], strings[2]);
            break;

        default:
            CGGameUI::DisplayError(error);
            break;
    }

    UpdateGuildRoster(1);
    return 1;
}

// VERIFY: 4f7b81f9 manual
// OFFSET: 0x007261B0
bool CGPlayer_C::IsInFlyableZone(int32_t checkContinent) {
    return IsInFlyableArea() && (!(this->m_obj->m_type & TYPEMASK_PLAYER) || !checkContinent || CGGameUI::m_iCurrentMapID != 571 || HasSpell(static_cast<uint32_t>(54197)));
}

// VERIFY: 0c1f1342 manual
// OFFSET: 0x006DA130
int32_t OnUpdateShapeshiftForm(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (unit) {
        if (unit->m_shapeshiftSuppressed) {
            unit->m_shapeshiftSuppressed = 0;

            if (unit->m_displayId == static_cast<int32_t>(unit->m_unit->m_unitFieldNativedisplayid)) {
                unit->m_displayId = 0;
                unit->UpdateDisplayInfo(0);
            }
        }

        if (unit->IsShapeShifted()) {
            SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(unit->GetShapeshiftForm());

            if (form && !(form->m_flags & 0x8)) {
                CGGameUI::ClearInteractTarget(CGGameUI::m_interactTarget, 0);
            }

            if (!CGMinimapFrame::s_trackingSpell && !CGMinimapFrame::s_trackingInfo) {
                int32_t shapeshift = unit->GetShapeshiftForm();
                SpellRec spell;

                for (uint32_t i = 0; i < s_trackingSpells.m_count; i++) {
                    g_spellDB.GetRecord(s_trackingSpells.m_data[i], &spell);

                    if (SpellRec::UsableInShapeshift(&spell, shapeshift - 1)) {
                        Spell_C::CastSpell(s_trackingSpells.m_data[i], nullptr, WOWGUID(), false);
                        break;
                    }
                }
            }
        } else if (value) {
            uint8_t oldForm = *static_cast<const uint8_t*>(value);

            if (oldForm == 1 || oldForm == 2 || oldForm == 3 || oldForm == 4 || oldForm == 5 || oldForm == 8 || oldForm == 29 || oldForm == 31) {
                C3Vector position = unit->GetPosition();

                if (CGUnit_C::s_activeMover == unit->m_obj->m_guid || unit->IsActivePlayer()) {
                    static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");
                    int32_t listenerAtCharacter = s_listenerAtCharacter->m_intValue;
                    SE2SoundKitProperties properties;
                    properties.ResetToDefaults();
                    properties.m_volumeScale = listenerAtCharacter ? 0.64999998f : 1.0f;
                    properties.m_priority = 110;
                    SI2::PlaySoundKit(10063, listenerAtCharacter ? nullptr : &position, nullptr, &properties, 0, nullptr, 1, 0);
                } else {
                    SI2::PlaySoundKit(10063, &position, nullptr, nullptr, 0, nullptr, 1, 0);
                }
            }
        }
    }

    CGSpellBook::UpdateUsable();
    CGSpellBook::UpdateSelection();
    CGActionBar::UpdateShapeShiftBar();
    CGActionBar::UpdateBonusBar();
    FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_SHAPESHIFT_FORM, nullptr);
    return 1;
}

// VERIFY: 9ad301d7 auto
// OFFSET: 0x006CDDA0
int32_t OnArenaTeamsChanged(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    CGArenaTeamInfo::UpdateTeams(1);
    return 1;
}

// VERIFY: 8779741b auto
// OFFSET: 0x006CDDF0
int32_t SignalCombatRatingUpdate(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_RATING_UPDATE, nullptr);
    PlayerSignalSkillStatChanges();
    return 1;
}

// VERIFY: d21fa08f auto
// OFFSET: 0x006CDE10
int32_t OnUpdatePlayerNoReagentCost(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    CGSpellBook::UpdateUsable();
    return 1;
}

// VERIFY: 2bb05660 manual
// OFFSET: 0x006CF6C0
int32_t CritterUpdate(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        CGActionBar::UpdateCritter((static_cast<const WOWGUID*>(value)->m_guid >> 24) & 0xFFFFFFF);
        CGActionBar::UpdateCritter((player->m_unit->m_unitFieldCritter.m_guid >> 24) & 0xFFFFFFF);
        CGSpellBook::SendCompanionUpdate(0);
    }

    return 1;
}

// VERIFY: e642f3fb manual
// OFFSET: 0x006D1B60
int32_t CharmChangeHandler(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->UpdateReaction(false);

        if (guid == ClntObjMgrGetActivePlayer()) {
            FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_UPDATE, nullptr);
        }
    }

    return 1;
}

// VERIFY: 7c2a6122 manual
// OFFSET: 0x006D1BC0
int32_t SignalPlayerSkillStatEventsAndRefresh(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    if (ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_ATTACK, "%s", "player");
        FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_DEFENSE, "%s", "player");
        FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_RANGED_ATTACK_POWER, "%s", "player");
    }

    PlayerSignalSkillStatChanges();
    return 1;
}

// VERIFY: ac8d5681 manual
// OFFSET: 0x006D9F20
int32_t ReportDailyQuestsRemaining(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 1;
    }

    if (player->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        CGPlayer_C::QueryQuestGiverStatusMultiple();
    }

    int32_t completed = 0;

    for (uint32_t i = 0; i < 25; i++) {
        if (player->m_player->m_playerFieldDailyQuests1[i]) {
            completed++;
        }
    }

    if (!completed && s_dailyQuestsUntouched) {
        return 1;
    }

    char text[3000];

    if (completed == 25) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("NO_DAILY_QUESTS_REMAINING", -1, GENDER_NOT_APPLICABLE));
        s_dailyQuestsUntouched = false;
    } else {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("DAILY_QUESTS_REMAINING", -1, GENDER_NOT_APPLICABLE), 25 - completed);
        s_dailyQuestsUntouched = completed == 0;
    }

    if (text[0]) {
        CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    return 1;
}

// VERIFY: 8b4afe19 manual
// OFFSET: 0x006DA070
int32_t OnUpdateMoney(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    SI2::PlaySoundKit("LOOTWINDOWCOINSOUND", nullptr, nullptr, nullptr);
    CGClassTrainer::RefreshList();
    FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_MONEY, nullptr);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        CGPlayer_C::QueryQuestGiverStatusMultiple();
    }

    uint32_t tradeMoney = CGTradeInfo::GetPlayerTradeMoney();

    if (!tradeMoney || !player) {
        return 1;
    }

    uint32_t coinage = player->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? player->m_player->m_playerFieldCoinage : 0;

    if (tradeMoney > coinage) {
        CGTradeInfo::RemoveMoney(tradeMoney - player->GetCoinage());
    }

    return 1;
}

// VERIFY: a46bd4bb manual
// OFFSET: 0x006DA630
int32_t OnUpdateRestState(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_EXHAUSTION, nullptr);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        uint32_t state = player->m_player->m_playerBytes2[3];

        if (state < 3) {
            if (s_restStateInfo[state].m_error != ERR_MAX_VALUE) {
                CGGameUI::DisplayError(s_restStateInfo[state].m_error);
            }

            if (s_restStateInfo[state].m_tutorial != 60) {
                CGTutorial::TriggerTutorial(s_restStateInfo[state].m_tutorial);
            }
        }
    }

    return 1;
}

// VERIFY: 912eed7b auto
// OFFSET: 0x006DA7D0
int32_t OnUpdatePlayerTrackingMask(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    ClntObjMgrEnumVisibleObjects(TrackingMaskUpdateProc, nullptr);
    return 1;
}

// VERIFY: 6945a18b manual
// OFFSET: 0x006CDBA0
int32_t __stdcall CGPlayer_C::OnBuyFailed(CDataStore* msg) {
    WOWGUID vendor;
    uint32_t itemId;
    uint8_t result;
    msg->Get(vendor.m_guid);
    msg->Get(itemId);
    msg->Get(result);

    if (result == 1 && vendor == CGPlayer_C::s_lastVendorListReceived) {
        for (uint32_t i = 0; i < 150; i++) {
            if (s_vendorItems[i].m_slot == itemId) {
                s_vendorItems[i].m_maxCount = 0;
                CGMerchantInfo::UpdateItemQuantity(vendor, itemId, 0);
            }
        }
    }

    switch (result) {
        case 1:
        case 7:
            CGGameUI::DisplayError(ERR_VENDOR_SOLD_OUT);
            break;

        case 2:
            CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
            break;

        case 4:
            CGGameUI::DisplayError(ERR_VENDOR_HATES_YOU);
            break;

        case 5:
            CGGameUI::DisplayError(ERR_VENDOR_TOO_FAR);
            break;

        case 6:
            break;

        case 8:
            CGGameUI::DisplayError(ERR_ITEM_MAX_COUNT);
            break;

        case 11:
            CGGameUI::DisplayError(ERR_CANT_EQUIP_RANK);
            break;

        case 12:
            CGGameUI::DisplayError(ERR_CANT_EQUIP_REPUTATION);
            break;

        default:
            CGGameUI::DisplayError(ERR_ITEM_NOT_FOUND);
            break;
    }

    return 1;
}

// VERIFY: db15a6c9 auto
// OFFSET: 0x006CDD10
int32_t __stdcall CGPlayer_C::OnBuySucceeded(CDataStore* msg) {
    WOWGUID vendor;
    uint32_t slot;
    uint32_t quantity;
    uint32_t count;
    msg->Get(vendor.m_guid);
    msg->Get(slot);
    msg->Get(quantity);
    msg->Get(count);

    if (vendor == CGPlayer_C::s_lastVendorListReceived) {
        CGMerchantInfo::UpdateItemQuantity(vendor, slot, quantity);
    }

    return 1;
}

// VERIFY: 8626f273 auto
// OFFSET: 0x006CDE20
void CGPlayer_C::ReceiveResurrectRequest(const char* name) {
    if (name && *name && this->IsDeadOrGhost()) {
        CGGameUI::OpenResurrectRequest(name);
    }
}

// VERIFY: f78961f2 manual
// OFFSET: 0x006D1540
int32_t __stdcall CGPlayer_C::OnSellResponse(CDataStore* msg) {
    WOWGUID vendor;
    WOWGUID item;
    uint8_t result;
    msg->Get(vendor.m_guid);
    msg->Get(item.m_guid);
    msg->Get(result);

    if (!result) {
        return 1;
    }

    UIErrors error;

    switch (result) {
        case 1:
            error = ERR_ITEM_NOT_FOUND;
            break;

        case 2:
            error = ERR_VENDOR_NOT_INTERESTED;
            break;

        case 3:
            error = ERR_VENDOR_HATES_YOU;
            break;

        case 4:
            error = ERR_NOT_OWNER;
            break;

        case 6:
            error = ERR_DESTROY_NONEMPTY_BAG;
            break;

        case 7:
            error = ERR_VENDOR_DOESNT_BUY;
            break;

        case 8:
            error = ERR_MUST_REPAIR_DURABILITY;
            break;

        case 9:
            error = ERR_INTERNAL_BAG_ERROR;
            break;

        default:
            error = ERR_MAX_VALUE;
            break;
    }

    if (error != ERR_MAX_VALUE) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(vendor, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (unit) {
            CGGameUI::AddErrorMessage(unit->GetFrameScriptText(CGGameUI::GetErrorToken(error), -1), 1);
        }
    }

    if (item) {
        CGGameUI::UnlockItem(item);
    }

    return 1;
}

// VERIFY: e74f4e9d manual
// OFFSET: 0x006D7C30
static int32_t OnQuestItemLoot(QuestCache* quest, int32_t itemId, int32_t count, int32_t totalCount) {
    if (!quest) {
        return 0;
    }

    uint32_t index = 0;

    while (quest->m_collectItemId[index] != static_cast<uint32_t>(itemId)) {
        if (++index >= 6) {
            return 0;
        }
    }

    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, WOWGUID(), nullptr, nullptr, false);

    if (!stats) {
        return 1;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player || totalCount - count >= static_cast<int32_t>(quest->m_collectItemCount[index])) {
        return 1;
    }

    int32_t required = quest->m_collectItemCount[index];
    const char* name = stats->m_name[FrameScript_GetPluralIndex(required)];

    if (!name || !*name) {
        name = stats->m_name[0];
    }

    CGGameUI::DisplayError(ERR_QUEST_ADD_ITEM_SII, name, totalCount > required ? required : totalCount, required);
    FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_WATCH_UPDATE, "%d", CGQuestLog::GetQuestIndex(quest->m_id));

    if ((quest->m_flags & 0x10000) && CGQuestLog::IsQuestCompleteEx(CGQuestLog::GetQuestIndex(quest->m_id) - 1, 1)) {
        WOWGUID guid = player->m_obj->m_guid;
        CGPlayer_C::CompleteQuest(guid, quest->m_id);
    }

    return 1;
}

// VERIFY: efdb7757 manual
// OFFSET: 0x006D7D70
static void QuestLootQuestQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    QuestItemLoot* loot = static_cast<QuestItemLoot*>(param);

    if (success) {
        QuestCache* quest = g_questDBCache.GetRecord(id, WOWGUID(), nullptr, nullptr, false);
        OnQuestItemLoot(quest, loot->m_itemId, loot->m_count, loot->m_totalCount);
    }

    delete loot;
}

// VERIFY: 86cb9fde manual
// OFFSET: 0x006D7DE0
void CGPlayer_C::CheckForQuestItemLoot(uint32_t itemId, uint32_t count, uint32_t totalCount) {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return;
    }

    for (uint32_t i = 0; i < 25; i++) {
        PlayerQuestLogEntry* entry = &this->m_player->m_playerQuestLog[i];

        if (static_cast<int32_t>(entry->m_questId) <= 0 || CGQuestLog::IsQuestExpiredByID(entry->m_questId)) {
            continue;
        }

        QuestCache* quest = g_questDBCache.GetRecord(entry->m_questId, WOWGUID(), nullptr, nullptr, false);

        if (quest) {
            if (OnQuestItemLoot(quest, itemId, count, totalCount)) {
                return;
            }
        } else {
            QuestItemLoot* loot = new (__FILE__, __LINE__) QuestItemLoot;
            loot->m_itemId = itemId;
            loot->m_count = count;
            loot->m_totalCount = totalCount;

            if (g_questDBCache.GetRecord(entry->m_questId, WOWGUID(), QuestLootQuestQueryCallback, loot, false)) {
                SErrDisplayAppFatalMessage("", "", 0);
            }
        }
    }
}

// VERIFY: fd900ed7 manual
// OFFSET: 0x006DBC60
static void ResurrectNameQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    NameCache* name = g_nameDBCache.GetRecord(CGPlayer_C::s_resurrectOfferer.m_guid, CGPlayer_C::s_resurrectOfferer, nullptr, nullptr, false);

    if (!name) {
        CGPlayer_C::s_resurrectOfferer.m_guid = 0;
        CGPlayer_C::s_resurrectOfferSickness = 0;
        CGPlayer_C::s_resurrectOfferUseTimer = 0;
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player && name->m_name[0] && player->IsDeadOrGhost()) {
        CGGameUI::OpenResurrectRequest(name->m_name);
    }
}

// VERIFY: ee9e13c1 manual
// OFFSET: 0x006DBD00
int32_t Packet_SMSG_RESURRECT_REQUEST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID offerer;
    uint32_t length;
    void* data;
    uint8_t sickness;
    uint8_t useTimer;
    msg->Get(offerer.m_guid);
    msg->Get(length);
    msg->GetDataInSitu(data, length);
    const char* name = static_cast<const char*>(data);
    msg->Get(sickness);
    msg->Get(useTimer);
    CGPlayer_C::s_resurrectOfferer = offerer;
    CGPlayer_C::s_resurrectOfferSickness = sickness;
    CGPlayer_C::s_resurrectOfferUseTimer = useTimer;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 1;
    }

    if (*name) {
        if (player->IsDeadOrGhost()) {
            CGGameUI::OpenResurrectRequest(name);
        }

        return 1;
    }

    NameCache* record = g_nameDBCache.GetRecord(offerer.m_guid, offerer, ResurrectNameQueryCallback, nullptr, false);

    if (record) {
        player->ReceiveResurrectRequest(record->m_name);
    }

    return 1;
}

// VERIFY: 97eb027d manual
// OFFSET: 0x006DC070
void CGPlayer_C::UpdateRepopTimer() {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    uint8_t bytes = this->m_player->m_playerFieldBytes[0];

    if (bytes & 0x10) {
        CGGameUI::SetRepopTimeLeft(0);
    } else if ((bytes & 0x8) && !(this->m_player->m_playerFlags & 0x4000)) {
        CGGameUI::SetRepopTimeLeft(360000);
    } else {
        CGGameUI::ClearRepopTime();
    }
}

// VERIFY: d3f70371 manual
// OFFSET: 0x006DC0F0
void CGPlayer_C::OnDeath() {
    if (this->m_unit->m_unitDynamicFlags & 0x20) {
        return;
    }

    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        WOWGUID guid = this->m_obj->m_guid;

        if (CGGameUI::IsInParty(guid)) {
            CGGameUI::DisplayError(ERR_PLAYER_DIED_S, this->GetUnitName(nullptr, 1));
        }

        return;
    }

    this->UpdateRepopTimer();
    CGGameUI::Target(WOWGUID());
    CGGameUI::ClearInteractTarget(CGGameUI::m_interactTarget, 0);
    CGGameUI::CloseLoot(true, false, false);
    Spell_C_CancelPlayerSpells();

    if (this->m_unit->m_unitFieldFlags & 0x100000) {
        this->HandleRepopRequest(false);
    }
}

// VERIFY: 28bd17c5 auto
// OFFSET: 0x006CEE90
void __stdcall CGPlayer_C::HandleActivateTaxiReply(uint32_t reply) {
    if (reply >= 13) {
        return;
    }

    if (reply) {
        CGGameUI::DisplayError(s_taxiReplyErrors[reply]);
    } else {
        CGTaxiMap::CloseMap();
    }
}

// VERIFY: fb8d8c6e manual
// OFFSET: 0x006D6050
void __stdcall CGPlayer_C::ShowTaxiNodes(CDataStore* msg) {
    WOWGUID unit;
    uint32_t currentNode = 0;
    uint32_t show = 0;
    msg->Get(show);

    if (show) {
        msg->Get(unit.m_guid);
        msg->Get(currentNode);
    }

    uint64_t* nodeMask = new (__FILE__, __LINE__) uint64_t[CGTaxiMap::m_nodeMaskSize];
    uint32_t knownNodes = 0;

    for (uint32_t i = 0; i < CGTaxiMap::m_nodeMaskSize; i++) {
        uint64_t mask = 0;
        msg->Get(mask);
        nodeMask[i] = mask;

        for (uint32_t bit = 0; bit < 64; bit++) {
            if (mask & (1ull << bit)) {
                knownNodes++;
            }
        }
    }

    if (show) {
        if (knownNodes >= 2) {
            CGTaxiMap::SetupMap(unit, currentNode, nodeMask);
        } else {
            CGGameUI::DisplayError(ERR_TAXINOPATHS);
            CGGossipInfo::SetGossip(WOWGUID());
        }
    } else {
        int32_t nodeID = 1;

        for (uint32_t i = 0; i < CGTaxiMap::m_nodeMaskSize; i++) {
            for (uint32_t bit = 0; bit < 64; bit++) {
                TaxiNodesRec* node;

                if ((nodeMask[i] & (1ull << bit)) && (node = g_taxiNodesDB.GetRecord(nodeID))) {
                    ConsolePrintf("[%02d]: %s", nodeID, node->m_name);
                }

                nodeID++;
            }
        }
    }

    delete[] nodeMask;
}

// VERIFY: b3eb651b manual
// OFFSET: 0x006CBA60
uint32_t CGPlayer_C::BuildPartyMemberStatsChanges(const CGPartyMemberState* oldState, const CGPartyMemberState* newState) {
    uint32_t changes = newState->m_flags != oldState->m_flags;

    if (newState->m_health != oldState->m_health) {
        changes |= 0x2;
    }

    if (newState->m_maxHealth != oldState->m_maxHealth) {
        changes |= 0x4;
    }

    if (newState->m_powerType != oldState->m_powerType) {
        changes |= 0x8;
    }

    if (newState->m_power != oldState->m_power) {
        changes |= 0x10;
    }

    if (newState->m_maxPower != oldState->m_maxPower) {
        changes |= 0x20;
    }

    if (newState->m_level != oldState->m_level) {
        changes |= 0x40;
    }

    if (newState->m_zoneID != oldState->m_zoneID) {
        changes |= 0x80;
    }

    if (newState->m_posX != oldState->m_posX || newState->m_posY != oldState->m_posY) {
        changes |= 0x100;
    }

    for (uint32_t i = 0; i < 64; i++) {
        if (newState->m_auras[i] != oldState->m_auras[i] || newState->m_auraFlags[i] != oldState->m_auraFlags[i]) {
            changes |= 0x200;
            break;
        }
    }

    if (newState->m_pet.m_guid != oldState->m_pet.m_guid) {
        changes |= 0x400;
    }

    if (SStrCmp(newState->m_pet.m_name, oldState->m_pet.m_name, STORM_MAX_STR)) {
        changes |= 0x800;
    }

    if (newState->m_pet.m_displayId != oldState->m_pet.m_displayId) {
        changes |= 0x1000;
    }

    if (newState->m_pet.m_health != oldState->m_pet.m_health) {
        changes |= 0x2000;
    }

    if (newState->m_pet.m_maxHealth != oldState->m_pet.m_maxHealth) {
        changes |= 0x4000;
    }

    if (newState->m_pet.m_powerType != oldState->m_pet.m_powerType) {
        changes |= 0x8000;
    }

    if (newState->m_pet.m_power != oldState->m_pet.m_power) {
        changes |= 0x10000;
    }

    if (newState->m_pet.m_maxPower != oldState->m_pet.m_maxPower) {
        changes |= 0x20000;
    }

    for (uint32_t i = 0; i < 64; i++) {
        if (newState->m_pet.m_auras[i] != oldState->m_pet.m_auras[i] || newState->m_pet.m_auraFlags[i] != oldState->m_pet.m_auraFlags[i]) {
            changes |= 0x40000;
            break;
        }
    }

    if (newState->m_vehicleSeatId != oldState->m_vehicleSeatId) {
        changes |= 0x80000;
    }

    return changes;
}

// VERIFY: 9b1bc63a manual
// OFFSET: 0x006CF740
void CGPlayer_C::DispatchPartyMemberStatsChanges(const WOWGUID& guid, uint32_t changes, const CGPartyMemberState* oldState, const CGPartyMemberState* newState) {
    if (changes & 0x1) {
        Script_SendUnitSignal(guid, SCRIPT_EVENT_UNIT_HEALTH);
        Script_SendUnitSignal(guid, newState->m_powerType + SCRIPT_EVENT_UNIT_MANA);
        changes &= ~0x12;
    }

    if (changes & 0x40) {
        Script_SendUnitSignal(guid, SCRIPT_EVENT_UNIT_LEVEL);
    }

    if (changes & 0x2) {
        Script_SendUnitSignal(guid, SCRIPT_EVENT_UNIT_HEALTH);
    }

    if (changes & 0x4) {
        Script_SendUnitSignal(guid, SCRIPT_EVENT_UNIT_MAXHEALTH);
    }

    if (changes & 0x8) {
        Script_SendUnitSignal(guid, SCRIPT_EVENT_UNIT_DISPLAYPOWER);
    }

    if (changes & 0x10) {
        Script_SendUnitSignal(guid, newState->m_powerType + SCRIPT_EVENT_UNIT_MANA);
    }

    if (changes & 0x20) {
        Script_SendUnitSignal(guid, newState->m_powerType + SCRIPT_EVENT_UNIT_MAXMANA);
    }

    if ((newState->m_flags ^ oldState->m_flags) & 0x12) {
        Script_SendUnitSignal(guid, SCRIPT_EVENT_UNIT_FACTION);
    }

    if (changes & 0x200) {
        Script_SendUniqueSignal(guid, SCRIPT_EVENT_UNIT_AURA);
    }

    if (changes & 0x400) {
        CGUnit_C* oldPet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(oldState->m_pet.m_guid, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (oldPet) {
            oldPet->UpdateScriptRegistration();
        }

        CGUnit_C* newPet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(newState->m_pet.m_guid, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (newPet) {
            newPet->UpdateScriptRegistration();
        }

        Script_SendUnitSignal(guid, SCRIPT_EVENT_UNIT_PET);
    }

    if (newState->m_pet.m_guid) {
        if (changes & 0x800) {
            CGGameUI::UnitNameUpdate(newState->m_pet.m_guid);
        }

        if (changes & 0x1000) {
            CGGameUI::UnitModelUpdate(newState->m_pet.m_guid, 3);
        }

        if (changes & 0x2000) {
            Script_SendUnitSignal(newState->m_pet.m_guid, SCRIPT_EVENT_UNIT_HEALTH);
        }

        if (changes & 0x4000) {
            Script_SendUnitSignal(newState->m_pet.m_guid, SCRIPT_EVENT_UNIT_MAXHEALTH);
        }

        if (changes & 0x8000) {
            Script_SendUnitSignal(newState->m_pet.m_guid, SCRIPT_EVENT_UNIT_DISPLAYPOWER);
        }

        if (changes & 0x10000) {
            Script_SendUnitSignal(newState->m_pet.m_guid, newState->m_pet.m_powerType + SCRIPT_EVENT_UNIT_MANA);
        }

        if (changes & 0x20000) {
            Script_SendUnitSignal(newState->m_pet.m_guid, newState->m_pet.m_powerType + SCRIPT_EVENT_UNIT_MAXMANA);
        }

        if (changes & 0x40000) {
            Script_SendUniqueSignal(newState->m_pet.m_guid, SCRIPT_EVENT_UNIT_AURA);
        }
    }

    if (changes & 0x80000) {
        VehicleSeatRec* seat = g_vehicleSeatDB.GetRecord(newState->m_vehicleSeatId);

        if (seat) {
            CVehiclePassenger_C::SendTransitionUIEvents(guid, 0, seat, 0, 3);
        } else {
            CVehiclePassenger_C::SendTransitionUIEvents(guid, 0, nullptr, 3, 0);
        }
    }
}

// VERIFY: 2543c387 manual
// OFFSET: 0x006CF9B0
int32_t Packet_HandlePartyMemberStats(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t arena = 0;

    if (msgId == SMSG_PARTY_MEMBER_STATS_FULL) {
        msg->Get(arena);
    }

    SmartGUID guid;
    *msg >> guid;

    if (arena) {
        CGBattlefieldInfo::AddArenaOpponent(guid);
    }

    CGPartyMemberState* state = CGPartyMemberStateRepository::GetState(guid);
    CGPartyMemberState oldState;

    if (state) {
        memcpy(&oldState, state, sizeof(CGPartyMemberState));
    } else {
        memset(&oldState, 0, sizeof(CGPartyMemberState));
    }

    CGPartyMemberState newState;

    if (msgId == SMSG_PARTY_MEMBER_STATS_FULL) {
        memset(&newState, 0, sizeof(CGPartyMemberState));
    } else {
        memcpy(&newState, &oldState, sizeof(CGPartyMemberState));
    }

    uint32_t mask;
    msg->Get(mask);

    if (mask & 0x1) {
        msg->Get(newState.m_flags);
        WOWGUID member = guid;

        if (newState.m_flags & 0x1) {
            CGAutoCompleteName::AddFlags(member, 0x20, 0, nullptr);
        } else {
            CGAutoCompleteName::RemoveFlags(member, 0x20);
        }
    }

    if (static_cast<int32_t>(mask) >= 0) {
        newState.m_flags &= ~0x20;
    } else {
        newState.m_flags |= 0x20;
    }

    if (mask & 0x2) {
        msg->Get(newState.m_health);
    }

    if (mask & 0x4) {
        msg->Get(newState.m_maxHealth);
    }

    if (mask & 0x8) {
        msg->Get(newState.m_powerType);
    }

    if (mask & 0x10) {
        msg->Get(newState.m_power);
    }

    if (mask & 0x20) {
        msg->Get(newState.m_maxPower);
    }

    if (mask & 0x40) {
        msg->Get(newState.m_level);
    }

    if (mask & 0x80) {
        msg->Get(newState.m_zoneID);
    }

    if (mask & 0x100) {
        msg->Get(reinterpret_cast<uint16_t&>(newState.m_posX));
        msg->Get(reinterpret_cast<uint16_t&>(newState.m_posY));
    }

    if (mask & 0x200) {
        uint64_t auraMask;
        msg->Get(auraMask);

        for (uint32_t i = 0; i < 64; i++) {
            if (auraMask & (static_cast<uint64_t>(1) << i)) {
                msg->Get(reinterpret_cast<uint32_t&>(newState.m_auras[i]));
                msg->Get(newState.m_auraFlags[i]);
            }
        }
    }

    if (mask & 0x400) {
        msg->Get(newState.m_pet.m_guid.m_guid);

        if (arena && newState.m_pet.m_guid) {
            WOWGUID owner = guid;
            CGBattlefieldInfo::AddArenaOpponentPet(newState.m_pet.m_guid, owner);
        }
    }

    if (mask & 0x800) {
        msg->GetString(newState.m_pet.m_name, sizeof(newState.m_pet.m_name));
    }

    if (mask & 0x1000) {
        msg->Get(newState.m_pet.m_displayId);
    }

    if (mask & 0x2000) {
        msg->Get(newState.m_pet.m_health);
    }

    if (mask & 0x4000) {
        msg->Get(newState.m_pet.m_maxHealth);
    }

    if (mask & 0x8000) {
        msg->Get(newState.m_pet.m_powerType);
    }

    if (mask & 0x10000) {
        msg->Get(newState.m_pet.m_power);
    }

    if (mask & 0x20000) {
        msg->Get(newState.m_pet.m_maxPower);
    }

    if (mask & 0x40000) {
        uint64_t auraMask;
        msg->Get(auraMask);

        for (uint32_t i = 0; i < 64; i++) {
            if (auraMask & (static_cast<uint64_t>(1) << i)) {
                msg->Get(reinterpret_cast<uint32_t&>(newState.m_pet.m_auras[i]));
                msg->Get(newState.m_pet.m_auraFlags[i]);
            }
        }
    }

    if (mask & 0x80000) {
        msg->Get(newState.m_vehicleSeatId);
    }

    uint32_t changes = CGPlayer_C::BuildPartyMemberStatsChanges(&oldState, &newState);
    int32_t index;

    if (CGBattlefieldInfo::GetArenaOpponentIndex(guid, &index) && CGBattlefieldInfo::m_arenaOpponentSeen[index]) {
        changes = 0xFFFFFFFF;

        if (CGBattlefieldInfo::GetArenaOpponentIndex(guid, &index)) {
            CGBattlefieldInfo::m_arenaOpponentSeen[index] = 0;
        }
    }

    if (state) {
        if (state->m_flags & 0x100) {
            newState.m_flags |= 0x100;
        }

        memcpy(state, &newState, sizeof(CGPartyMemberState));

        CGPartyMemberState* remote = CGRaidInfo::GetRemoteStats(guid);

        if (remote) {
            memcpy(remote, &newState, sizeof(CGPartyMemberState));
        }
    }

    CGPlayer_C::DispatchPartyMemberStatsChanges(guid, changes, &oldState, &newState);
    return 1;
}

// VERIFY: 646d304f manual
// OFFSET: 0x006DF710
void CGPlayer_C::UpdateGhostStatus() {
    if (this->m_player->m_playerFlags & 0x10) {
        CGTutorial::TriggerTutorial(24);
    } else {
        CGGameUI::SetAreaSpiritHealer(WOWGUID());
        C3Vector position(0.0f, 0.0f, 0.0f);
        CGGameUI::SetDeathReleaseLocation(-1, position);
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_UNGHOST, nullptr);
    }

    CGGameUI::UpdateCorpseLocation();
    CGWorldFrame::UpdateScreenEffect();
    SendQuestGiverStatusMultipleQuery();
    ClntObjMgrEnumVisibleObjects(TrackingMaskUpdateProc, nullptr);
    PlayerNameTriggerColorUpdateAll();
    this->UpdateReaction(false);
}

// VERIFY: f8ed8b8a manual
// OFFSET: 0x006E0FD0
void CGPlayer_C::TogglePlayerFlag(uint32_t oldFlags) {
    uint32_t changed = oldFlags ^ this->m_player->m_playerFlags;

    if (changed & 0x800E) {
        PlayerNameTriggerNameRegenerate(this->m_nameDesc);

        if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            CGChat::m_afk = this->m_player->m_playerFlags & 0x2;
        }
    }

    if (changed & 0x40300) {
        this->SetPvpCooldownTimestamp(oldFlags);
    }

    if (changed & 0xC00) {
        UpdateModelForUI(this->m_obj->m_guid, 3);
    }

    Script_SendUnitSignal(this->m_obj->m_guid, SCRIPT_EVENT_PLAYER_FLAGS_CHANGED);

    if (changed & 0x400) {
        if (this->m_player->m_playerFlags & 0x400) {
            this->HideHelm();
        } else {
            this->ShowHelm();
        }
    }

    if (changed & 0x800) {
        if (this->m_player->m_playerFlags & 0x800) {
            this->HideCloak();
        } else {
            this->ShowCloak();
        }
    }

    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        return;
    }

    if (changed & 0x10) {
        this->UpdateGhostStatus();
    }

    if (changed & 0x20) {
        if (this->m_player->m_playerFlags & 0x20) {
            CGTutorial::TriggerTutorial(29);
            CGTutorial::TriggerTutorial(30);
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_UPDATE_RESTING, nullptr);
    }

    if (changed & 0x200) {
        const char* text;

        if (this->m_player->m_playerFlags & 0x200) {
            CGGameUI::DisplayError(ERR_PVP_TOGGLE_ON);
            text = FrameScript_GetText("PVP_TOGGLE_ON_VERBOSE", -1, GENDER_NOT_APPLICABLE);
        } else {
            CGGameUI::DisplayError(ERR_PVP_TOGGLE_OFF);
            text = FrameScript_GetText("PVP_TOGGLE_OFF_VERBOSE", -1, GENDER_NOT_APPLICABLE);
        }

        CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    if (changed & 0x3000) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PLAYTIME_CHANGED, nullptr);
    }

    if (changed & 0x20000) {
        if (this->m_player->m_playerFlags & 0x20000) {
            FrameScript_SignalEvent(SCRIPT_EVENT_ENABLE_TAXI_BENCHMARK, nullptr);
        } else {
            FrameScript_SignalEvent(SCRIPT_EVENT_DISABLE_TAXI_BENCHMARK, nullptr);
        }
    }

    if (changed & 0x1800000) {
        CGSpellBook::UpdateUsable();
    }

    if (changed & 0x2000000) {
        if (this->m_player->m_playerFlags & 0x2000000) {
            FrameScript_SignalEvent(SCRIPT_EVENT_DISABLE_XP_GAIN, nullptr);
        } else {
            FrameScript_SignalEvent(SCRIPT_EVENT_ENABLE_XP_GAIN, nullptr);
        }
    }

    if (changed & 0x10000) {
        if (this->m_player->m_playerFlags & 0x10000) {
            FrameScript_SignalEvent(SCRIPT_EVENT_ENABLE_LOW_LEVEL_RAID, nullptr);
        } else {
            FrameScript_SignalEvent(SCRIPT_EVENT_DISABLE_LOW_LEVEL_RAID, nullptr);
        }
    }
}

// STATIC: 0x00ADA2E0
static uint32_t s_textEmoteStandStates = 0x10B;

// VERIFY: 1c3eae7c manual
// OFFSET: 0x006DD9E0
void CGPlayer_C::SendTextEmote(const EmotesTextRec* emote, WOWGUID target) {
    if (!emote || (this->m_unit->m_unitFieldFlags & 0x1000000)) {
        return;
    }

    EmotesRec* rec = g_emotesDB.GetRecord(emote->m_emoteID);

    if (!rec) {
        return;
    }

    int32_t standUp;

    if (!this->CanPlayEmote(rec->m_emoteFlags, 1, &standUp)) {
        return;
    }

    if (standUp && this->IsClientControlled()) {
        CGGameUI::DisplayError(ERR_NOEMOTEWHILERUNNING);
        return;
    }

    if (this->GetClientStandState() == 3 && rec->m_emoteSpecProc != 1) {
        return;
    }

    if (target == this->m_obj->m_guid) {
        target = WOWGUID();
    }

    this->SetSheatheState(0, 1, 0);

    if (rec->m_emoteSpecProc == 1 && ((1 << rec->m_emoteSpecProcParam) & s_textEmoteStandStates)) {
        this->ChangeStandState(rec->m_emoteSpecProcParam);
    } else if (rec->m_animID && this->CanPlayEmoteAnim()) {
        this->PlayBaseAnimation(rec->m_animID, 0);
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SEND_TEXT_EMOTE));
    msg.Put(static_cast<uint32_t>(emote->m_id));
    uint32_t sound;

    if (this->m_unit->m_unitFieldNativedisplayid == this->m_unit->m_unitFieldDisplayid) {
        sound = SI2::GetTextEmoteSoundIndex(emote->m_id, this->m_unit->m_unitFieldBytes0[0], this->m_unit->m_unitFieldBytes0[2]);
    } else {
        uint32_t sex = this->GetDisplaySex();
        uint32_t race = this->GetDisplayRace();
        sound = SI2::GetTextEmoteSoundIndex(emote->m_id, race, sex);
    }

    msg.Put(sound);
    msg.Put(target.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: e2d4b50d manual
// OFFSET: 0x006CF300
void CGPlayer_C::StopDance() {
    PlayerDance* dance = this->m_dance;

    if (dance) {
        delete[] dance->m_moves;
        dance->m_moves = nullptr;
        delete dance;
    }

    this->m_dance = nullptr;
}

// VERIFY: 36496829 manual
// OFFSET: 0x006DE870
void CGPlayer_C::Dance() {
    PlayerDance* dance = this->m_dance;

    if (!dance) {
        return;
    }

    int32_t index = this->m_danceMoveIndex;

    if (index < dance->m_count) {
        DanceMovesRec* move = g_danceMovesDB.GetRecord(dance->m_moves[index]);

        if (move) {
            this->PlayDanceMove(move);
        } else {
            char text[512];
            SStrPrintf(text, sizeof(text), "ERROR! danceMovesDB does not have record id %d", dance->m_moves[index]);
            ConsoleWrite(text, ERROR_COLOR);
        }

        return;
    }

    delete[] dance->m_moves;
    dance->m_moves = nullptr;
    delete dance;
    this->m_dance = nullptr;

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_STOP_DANCE));
    msg.Put(static_cast<uint8_t>(0));
    msg.Finalize();
    ClientServices::Send2(&msg);
    this->UpdateBaseAnimation(0, -1);
}

// VERIFY: fb947d00 auto
// OFFSET: 0x006E1780
void CGPlayer_C::StartDance(PlayerDance* dance) {
    if (this->m_dance) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    this->m_dance = dance;
    this->m_danceMoveIndex = dance->m_start;
    this->Dance();
}

// VERIFY: daebf535 manual
// OFFSET: 0x006E17C0
void CGPlayer_C::DanceMoveComplete() {
    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_SYNC_DANCE));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    this->m_danceMoveIndex++;
    this->Dance();
}

// VERIFY: 5cf70f73 manual
// OFFSET: 0x006E1890
static int32_t DanceTimerCallback(const void* data, uint64_t guid, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(WOWGUID{ guid }, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->DanceMoveComplete();
    }

    return 1;
}

// VERIFY: 1a816aa2 manual
// OFFSET: 0x006E18D0
void CGPlayer_C::PlayDanceMove(const DanceMovesRec* move) {
    switch (move->m_type) {
        case 0:
            this->PlayBaseAnimation(move->m_param, 0);
            break;

        case 1: {
            SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(move->m_param);

            if (kit) {
                PlaySpellVisuaKitData data(nullptr, kit, 0);
                this->PlaySpellVisualKit(data);
            }

            break;
        }

        case 2: {
            C3Vector position = this->GetPosition();
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player && (CGPartyInfo::IsMember(this->m_obj->m_guid) || player->m_unit->m_unitFieldTarget == this->m_obj->m_guid || ClntObjMgrGetActivePlayer() == this->m_obj->m_guid)) {
                SI2::PlaySoundKit(move->m_param, &position, nullptr, nullptr, 0, nullptr, 1, 0);
            }

            this->DanceMoveComplete();
            break;
        }

        case 3:
        case 4: {
            ClientSetTimer(1000 * move->m_param, DanceTimerCallback, this->m_obj->m_guid.m_guid, nullptr);

            if (move->m_type == 4) {
                DanceMovesRec* previous = g_danceMovesDB.GetRecord(this->m_dance->m_moves[this->m_danceMoveIndex - 1]);

                if (previous->m_type == 4 || previous->m_type == 3) {
                    SErrDisplayAppFatalMessage("", "", 0);
                }

                this->PlayDanceMove(previous);
            }

            break;
        }

        default:
            break;
    }
}

// VERIFY: e19725e3 auto
// OFFSET: 0x006E2E10
void CGPlayer_C::DanceMoveAnimComplete() {
    PlayerDance* dance = this->m_dance;

    if (!dance) {
        return;
    }

    int32_t index = this->m_danceMoveIndex;
    DanceMovesRec* move = g_danceMovesDB.GetRecord(dance->m_moves[index]);

    if (move->m_type == 4 && index > 0) {
        this->PlayDanceMove(g_danceMovesDB.GetRecord(dance->m_moves[index - 1]));
    } else {
        this->DanceMoveComplete();
    }
}

// VERIFY: 2c8a74b4 manual
// OFFSET: 0x006E5090
void CGPlayer_C::DestructActivePlayer() {
    if (this->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (ClntObjMgrGetPlayerType() != 1) {
        if (this->m_unit->m_unitFieldFlags & 0x80000) {
            CGGameUI::SetRegenEnabled(1);
        }

        CGGameUI::LeaveWorld();
    }

    for (uint32_t i = 0; i < 1002; i++) {
        this->m_playerLocalMirrorLists[i].~TSList<void>();
    }

    Spell_C_SetAutoRangedCombatSpell(0);
    EventUnregister(EVENT_ID_IDLE, CGPlayer_C::AutoCombatModeEventHandler);
    this->m_combatModeFlags &= ~0xEu;
    CGGameUI::UpdateCombatMode();
    s_visibleAuraEffectDisplay = 0;
    s_visibleAuraEffectFlags = 0;
}

// VERIFY: 87e023cc manual
// OFFSET: 0x006E6020
void CGPlayer_C::Disable() {
    int32_t guildId = this->m_player->m_playerGuildid;

    if (guildId) {
        WOWGUID none = WOWGUID();

        if (!g_guildInfoCache.GetRecord(guildId, none, nullptr, nullptr, false)) {
            g_guildInfoCache.CancelCallback(this->m_player->m_playerGuildid, PlayerNameGuildCallback, this);
            g_guildInfoCache.CancelCallback(this->m_player->m_playerGuildid, CGPlayer_C::TabardDrawCallback, this);
        }
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        this->UnsetActiveMirrorHandlers();
        CGGameUI::CancelMirrorTimers();
        CGGameUI::LeaveWorld();
    }

    CGPartyInfo::EnableMember(this, 0);
    CGRaidInfo::EnableMember(this, 0);

    if (this->m_obj->m_guid == CGPetitionInfo::m_petitionItem) {
        CGPetitionInfo::SetPetition(WOWGUID(), 0, WOWGUID());
    }

    CGUnit_C::Disable();
    PlayerDance* dance = this->m_dance;

    if (dance) {
        delete[] dance->m_moves;
        dance->m_moves = nullptr;
        delete dance;
    }

    this->m_dance = nullptr;

    if (s_inspectData && s_inspectData->m_guid == this->m_obj->m_guid) {
        s_inspectData->m_guid = WOWGUID();
    }
}

// VERIFY: cdeaa713 manual
// OFFSET: [0x006E6FE0, 0x006E78D0]
CGPlayer_C::~CGPlayer_C() {
    this->m_tradeSkillLines.InternalClear(0);
    this->m_proficiencySpells.InternalClear(0);
    this->m_pendingItemExpirations.Clear();

    if (!(this->m_modelFlags & 0x10000)) {
        CGPartyInfo::EnableMember(this, 0);
        CGRaidInfo::EnableMember(this, 0);
    }

    int32_t guildId = this->m_player->m_playerGuildid;

    if (guildId) {
        WOWGUID none = WOWGUID();

        if (!g_guildInfoCache.GetRecord(guildId, none, nullptr, nullptr, false)) {
            g_guildInfoCache.CancelCallback(this->m_player->m_playerGuildid, PlayerNameGuildCallback, this);
            g_guildInfoCache.CancelCallback(this->m_player->m_playerGuildid, CGPlayer_C::TabardDrawCallback, this);
        }
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        this->DestructActivePlayer();
    }

    PlayerDance* dance = this->m_dance;

    if (dance) {
        delete[] dance->m_moves;
        dance->m_moves = nullptr;
        delete dance;
    }

    this->m_dance = nullptr;
}

// VERIFY: f05ee4d4 auto
// OFFSET: 0x006D68D0
int32_t CGPlayer_C::GetSpellCastingTime(const SpellRec* spell) const {
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

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        Spell_C_ModifySpellValue(spell, time, 10);
    }

    if (!(spell->m_attributes & 0x30) && !(spell->m_attributesExC & 0x20000000) && time > 0) {
        CGUnitData* unit = this->m_unit;

        if (fnotequal(unit->m_unitModCastSpeed, 1.0f)) {
            time = static_cast<int32_t>(time * unit->m_unitModCastSpeed);
        }
    }

    if (spell->m_attributes & 0x2) {
        time += 500;
    }

    return time <= 0 ? 0 : time;
}

// VERIFY: b8dca1f6 auto
// OFFSET: 0x006DC330
int32_t CGPlayer_C::GetSpellRank(const SpellRec* spell) const {
    SpellRec weaponSpell;
    const SpellRec* rankSpell = spell;
    int32_t spellId = const_cast<CGPlayer_C*>(this)->SpellUsesWeaponSkill(spell);

    if (spellId != spell->m_id) {
        if (!g_spellDB.GetRecord(spellId, &weaponSpell)) {
            return 0;
        }

        rankSpell = &weaponSpell;
    }

    SkillLineAbilityRec* ability = SpellTableLookupAbility(this->m_unit->m_unitFieldBytes0[0], this->m_unit->m_unitFieldBytes0[1], spellId);

    if (!ability) {
        return 0;
    }

    int32_t rank = this->GetSkillRank(ability->m_skillLine);
    int32_t maxLevel = rankSpell->m_maxLevel;

    if (maxLevel > 0 && rank >= 5 * maxLevel) {
        rank = 5 * maxLevel;
    }

    if (rank < 0) {
        rank = 0;
    }

    return rank;
}

// VERIFY: b01606c2 auto
// OFFSET: 0x006DE6D0
void CGPlayer_C::GetDefenseSkillRank(int32_t& rank, int32_t& bonus) const {
    int32_t defenseSpell = s_defenseSpellId;
    bonus = 0;
    rank = 0;
    SkillLineAbilityRec* ability = SpellTableLookupAbility(this->m_unit->m_unitFieldBytes0[0], this->m_unit->m_unitFieldBytes0[1], defenseSpell);

    if (ability && ability->m_spell == s_defenseSpellId) {
        this->GetExpandedSkillRank(ability->m_skillLine, rank, bonus);
    }

    bonus += static_cast<int32_t>(const_cast<CGPlayer_C*>(this)->GetCombatRatingPct(1) - 0.5f);
}

// VERIFY: 7a00b174 manual
// OFFSET: 0x006E0C30
void CGPlayer_C::GetAttackSkillRank(int32_t hand, int32_t& rank, int32_t& bonus) const {
    bonus = 0;
    rank = 0;
    int32_t skillSpell = 0;

    if (hand < 3) {
        const CGUnitVirtualItem* item = this->GetVirtualItem(hand, false);
        uint32_t subclass;

        if (!item) {
            subclass = ClientDBGetUnarmedWeapon();
        } else if (item->m_classID == 2) {
            subclass = item->m_subclassID;
        } else {
            subclass = 0xFFFFFFFF;
        }

        if (subclass < s_weaponSkills.Count()) {
            skillSpell = s_weaponSkills.m_data[subclass];
        }
    }

    SkillLineAbilityRec* ability = SpellTableLookupAbility(this->m_unit->m_unitFieldBytes0[0], this->m_unit->m_unitFieldBytes0[1], skillSpell);

    if (ability && ability->m_spell == skillSpell) {
        this->GetExpandedSkillRank(ability->m_skillLine, rank, bonus);
    }

    CGPlayer_C* player = const_cast<CGPlayer_C*>(this);
    bonus += static_cast<int32_t>(player->GetCombatRatingPct(0) - 0.5f);

    if (hand == 0) {
        bonus += static_cast<int32_t>(player->GetCombatRatingPct(20) - 0.5f);
    } else if (hand == 1) {
        bonus += static_cast<int32_t>(player->GetCombatRatingPct(21) - 0.5f);
    }
}

// VERIFY: a1437c5c manual
// OFFSET: 0x006E04D0
const CGUnitVirtualItem* CGPlayer_C::GetVirtualItem(uint32_t slot, bool ignoreDisarm) const {
    VisibleItem_C* item = nullptr;

    if (slot == 0) {
        if (!ignoreDisarm && this->IsDisarmed(0)) {
            return nullptr;
        }

        item = &this->m_player->m_playerVisibleItems[15];
    } else if (slot == 1) {
        if (!ignoreDisarm && this->IsDisarmed(1)) {
            return nullptr;
        }

        item = &this->m_player->m_playerVisibleItems[16];
    } else if (slot == 2) {
        if (!ignoreDisarm && (this->m_unit->m_unitFieldFlags2 & 0x400)) {
            return nullptr;
        }

        item = &this->m_player->m_playerVisibleItems[17];
    }

    if ((ignoreDisarm || (item && item->m_itemId > 0)) && item && abs(item->m_itemId)) {
        CGUnitVirtualItem* virtualItem = &const_cast<CGPlayer_C*>(this)->m_virtualItems[slot];
        virtualItem->m_classID = item->GetClassID();
        virtualItem->m_subclassID = item->GetSubtypeID();
        virtualItem->m_material = item->GetMaterial();
        virtualItem->m_inventoryType = item->GetInventoryType();
        virtualItem->m_sheatheType = item->GetSheatheType();
        virtualItem->m_soundOverrideSubclassID = item->GetSoundOverrideSubclassID();
        return virtualItem;
    }

    return nullptr;
}

// VERIFY: db19cffe manual
// OFFSET: 0x006E05D0
uint32_t CGPlayer_C::GetVirtualItemDisplayID(uint32_t slot) const {
    VisibleItem_C* item;

    if (slot == 0) {
        if (this->IsDisarmed(0)) {
            return 0;
        }

        item = &this->m_player->m_playerVisibleItems[15];
    } else if (slot == 1) {
        if (this->IsDisarmed(1)) {
            return 0;
        }

        item = &this->m_player->m_playerVisibleItems[16];
    } else if (slot == 2) {
        item = &this->m_player->m_playerVisibleItems[17];
    } else {
        return 0;
    }

    return item ? item->GetDisplayID() : 0;
}

// VERIFY: 5d485eba manual
// OFFSET: 0x0073C1D0
void CGPlayer_C::OnSpellEffectClear(CEffect* effect) {
    bool updateAnimation = effect->m_kitRec && effect->m_kitRec->m_animID > 0 && (effect->m_flags & 0x1000);
    CGObject_C::OnSpellEffectClear(effect);

    if (updateAnimation) {
        this->UpdateBaseAnimation(0, -1);
    }
}

// STATIC: 0x00ADBA74
static const uint32_t s_playerSoundChance[14] = { 35, 100, 30, 100, 100, 40, 100, 100, 100, 100, 100, 100, 100, 0 };

// VERIFY: 904e0dd6 manual
// OFFSET: 0x007631A0
void CGPlayer_C::PlayUnitSound(uint32_t soundType, int32_t force) {
    if (!force && static_cast<uint32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * 101) >> 32) > s_playerSoundChance[soundType]) {
        return;
    }

    if (!CheckUnitSoundTimer(soundType)) {
        return;
    }

    int32_t soundKitId = GetSoundID(this->GetSoundData(), soundType);

    if (!soundKitId) {
        return;
    }

    int32_t group;

    if ((soundType == 2 || soundType == 3 || soundType == 9) && this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        group = 12;
    } else {
        group = this->GetUnitSoundKitIndex(soundType);
    }

    C3Vector position = this->GetPosition();
    position.z += 2.0f;
    static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");
    int32_t listenerAtCharacter = s_listenerAtCharacter->m_intValue;
    SE2SoundKitProperties properties;
    const C3Vector* soundPosition;

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        properties.ResetToDefaults();
        properties.m_group = group;

        if (listenerAtCharacter) {
            properties.m_volumeScale = 0.64999998f;
        }

        properties.m_priority = 110;

        if (soundType == 7) {
            properties.m_maxAudibleBehavior = 0;
        } else if (soundType != 5 && soundType != 6 && soundType != 10 && soundType != 11 && soundType != 12 && soundType != 13 && this->m_unit->m_unitFieldBytes0[1] == 6) {
            properties.m_useDspChain = 1;
            SI2::DK_GetPlayerFilterName(this, properties.m_dspChainName, sizeof(properties.m_dspChainName));
        }

        soundPosition = listenerAtCharacter ? nullptr : &position;
    } else {
        properties.ResetToDefaults();
        properties.m_group = group;

        if (soundType != 5 && soundType != 6 && soundType != 7 && soundType != 10 && soundType != 11 && soundType != 12 && soundType != 13 && this->m_unit->m_unitFieldBytes0[1] == 6) {
            properties.m_useDspChain = 1;
            SI2::DK_GetPlayerFilterName(this, properties.m_dspChainName, sizeof(properties.m_dspChainName));
        }

        soundPosition = &position;
    }

    SI2::PlaySoundKit(soundKitId, soundPosition, nullptr, &properties, 0, nullptr, 1, 0);
}

// VERIFY: f8cee1ae manual
// OFFSET: 0x007633F0
void CGPlayer_C::PlayFoleySound() {
    static CVar* s_enableForSelf = CVar::Lookup("Sound_EnableArmorFoleySoundForSelf");
    static CVar* s_enableForOthers = CVar::Lookup("Sound_EnableArmorFoleySoundForOthers");

    if ((this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && s_enableForSelf && s_enableForSelf->m_intValue) || (this->m_obj->m_guid != ClntObjMgrGetActivePlayer() && s_enableForOthers && s_enableForOthers->m_intValue)) {
        VisibleItem_C* chest = const_cast<VisibleItem_C*>(this->GetVisibleItem(4));

        if (chest) {
            int32_t local = this->m_obj->m_guid == ClntObjMgrGetActivePlayer();
            C3Vector position = this->GetPosition();
            SI2::PlayFoleySound(chest->GetMaterial(), &position, local);
        }
    }
}

// STATIC: 0x00C9EB0C
static uint32_t s_talentWipeCost;

// VERIFY: 0a6ad881 manual
// OFFSET: 0x006CEFD0
int32_t CGPlayer_C::OnSignedResults(CDataStore* msg) {
    WOWGUID petition;
    msg->Get(petition.m_guid);
    WOWGUID signer;
    msg->Get(signer.m_guid);
    uint32_t result;
    msg->Get(result);

    if (signer != this->m_obj->m_guid) {
        CGPetitionInfo::AddSignature(petition, signer);
        return 1;
    }

    switch (result) {
        case 0:
            CGGameUI::DisplayError(ERR_PETITION_SIGNED);
            FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_CLOSED, nullptr);
            break;
        case 1:
            CGGameUI::DisplayError(ERR_PETITION_ALREADY_SIGNED);
            break;
        case 2:
            CGGameUI::DisplayError(ERR_PETITION_IN_GUILD);
            break;
        case 3:
            CGGameUI::DisplayError(ERR_PETITION_CREATOR);
            break;
        case 5:
            CGGameUI::DisplayError(ERR_PETITION_NOT_SAME_SERVER);
            break;
        case 8:
            CGGameUI::DisplayError(ERR_PETITION_FULL);
            break;
        case 10:
            CGGameUI::DisplayError(ERR_PETITION_ALREADY_SIGNED_OTHER);
            break;
        case 11:
            CGGameUI::DisplayError(ERR_PETITION_RESTRICTED_ACCOUNT);
            break;
        default:
            ConsoleWrite("Petition error", DEFAULT_COLOR);
            break;
    }

    return 1;
}

// VERIFY: c09c936c auto
// OFFSET: 0x006D12C0
int32_t __stdcall CGPlayer_C::OnTrainerList(CDataStore* msg) {
    WOWGUID trainer;
    msg->Get(trainer.m_guid);
    uint32_t type;
    msg->Get(type);
    uint32_t count;
    msg->Get(count);

    int32_t* spellIds = static_cast<int32_t*>(alloca(sizeof(int32_t) * count));
    uint8_t* states = static_cast<uint8_t*>(alloca(count));
    int32_t* moneyCosts = static_cast<int32_t*>(alloca(sizeof(int32_t) * count));
    int32_t* talentCosts = static_cast<int32_t*>(alloca(sizeof(int32_t) * count));
    int32_t* professionCosts = static_cast<int32_t*>(alloca(sizeof(int32_t) * count));
    uint8_t* levelReqs = static_cast<uint8_t*>(alloca(count));
    int32_t* skillReqs = static_cast<int32_t*>(alloca(sizeof(int32_t) * count));
    int32_t* skillRankReqs = static_cast<int32_t*>(alloca(sizeof(int32_t) * count));
    int32_t* stepSpells = static_cast<int32_t*>(alloca(sizeof(int32_t) * count));
    int32_t* abilityReqs0 = static_cast<int32_t*>(alloca(sizeof(int32_t) * count));
    int32_t* abilityReqs1 = static_cast<int32_t*>(alloca(sizeof(int32_t) * count));
    int32_t* abilityReqs2 = static_cast<int32_t*>(alloca(sizeof(int32_t) * count));

    for (uint32_t i = 0; i < count; i++) {
        msg->Get(reinterpret_cast<uint32_t&>(spellIds[i]));
        msg->Get(states[i]);
        msg->Get(reinterpret_cast<uint32_t&>(moneyCosts[i]));
        msg->Get(reinterpret_cast<uint32_t&>(talentCosts[i]));
        msg->Get(reinterpret_cast<uint32_t&>(professionCosts[i]));
        msg->Get(levelReqs[i]);
        msg->Get(reinterpret_cast<uint32_t&>(skillReqs[i]));
        msg->Get(reinterpret_cast<uint32_t&>(skillRankReqs[i]));
        stepSpells[i] = 0;
        msg->Get(reinterpret_cast<uint32_t&>(abilityReqs0[i]));
        msg->Get(reinterpret_cast<uint32_t&>(abilityReqs1[i]));
        msg->Get(reinterpret_cast<uint32_t&>(abilityReqs2[i]));
    }

    char greeting[2048];
    msg->GetString(greeting, sizeof(greeting));
    const int32_t* pointCosts[2] = { talentCosts, professionCosts };
    const int32_t* abilityReqs[3] = { abilityReqs0, abilityReqs1, abilityReqs2 };

    if (trainer == s_lastTrainerRequest || trainer == CGGossipInfo::m_gossip) {
        CGClassTrainer::SetTrainer(trainer, type);
        CGClassTrainer::AddServices(count, spellIds, moneyCosts, pointCosts, levelReqs, skillReqs, skillRankReqs, stepSpells, abilityReqs, states, greeting);
    }

    return 1;
}

// VERIFY: 17f8c3c1 manual
// OFFSET: 0x006D2840
void CGPlayer_C::TabardDrawCallback(int32_t guildID, const WOWGUID& guid, void* param, bool success) {
    if (!guildID || !success) {
        return;
    }

    WOWGUID none = WOWGUID();

    if (!g_guildInfoCache.GetRecord(guildID, none, nullptr, nullptr, false)) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    int32_t emblemStyle;
    int32_t emblemColor;
    int32_t borderStyle;
    int32_t borderColor;
    int32_t background;

    if (GuildGetGuildTabard(guid, guildID, nullptr, nullptr, emblemStyle, emblemColor, borderStyle, borderColor, background) && player->m_characterComponent) {
        player->m_characterComponent->ApplyGuildColor(emblemStyle, emblemColor, borderStyle, borderColor, background);
    }
}

// VERIFY: 061077e2 manual
// OFFSET: 0x006D5D90
void __stdcall CGPlayer_C::ReadItemResult(NETMESSAGE msgId, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);

    if (msgId == SMSG_READ_ITEM_RESULT_OK) {
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

        if (!item) {
            return;
        }

        item->SetTranslated();
        CGItemText::DisplayText(guid, 1);
        return;
    }

    uint8_t result;
    msg->Get(result);

    if (!result) {
        CGItemText::DisplayText(guid, 1);
    } else if (result == 1) {
        CGItemText::DisplayText(guid, 0);
        uint32_t delay;
        msg->Get(delay);
        FrameScript_SignalEvent(SCRIPT_EVENT_ITEM_TEXT_TRANSLATION, "%f", delay * 0.001);
    } else if (guid == CGItemText::m_itemGUID) {
        FrameScript_SignalEvent(SCRIPT_EVENT_ITEM_TEXT_CLOSED, nullptr);
    }
}

// VERIFY: 196810c7 manual
// OFFSET: 0x006D6720
bool CGPlayer_C::ReportBagItemSubtypeMismatch(uint8_t bag) {
    if (bag == 0xFF) {
        return false;
    }

    WOWGUID guid = bag < this->m_bag.m_numSlots ? this->m_bag.m_slots[bag] : WOWGUID();
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (!item || !item->GetBagFamily()) {
        return false;
    }

    int32_t index = CMath::fint_n(CMath::log2_(static_cast<float>(item->GetBagFamily()))) + 1;

    if (!index) {
        return false;
    }

    ItemBagFamilyRec* family = g_itemBagFamilyDB.GetRecord(index);

    if (!family) {
        return false;
    }

    CGGameUI::DisplayError(ERR_WRONG_BAG_TYPE_SUBCLASS, family->m_name);
    return true;
}

// VERIFY: 70226cb9 manual
// OFFSET: 0x006D67E0
void CGPlayer_C::StartGiftWrap(CGItem_C* item) {
    s_giftWrapItem = item->m_obj->m_guid;
    CGGameUI::LockItem(s_giftWrapItem);
    CursorSetResetMode(2);
    CursorSetMode(2);
}

// VERIFY: 35dbeab8 manual
// OFFSET: 0x006D6BA0
void CGPlayer_C::UpdateObjComponentVisuals(const VisibleItem_C* item, GEOCOMPONENTLINKS link) {
    int32_t visual = const_cast<VisibleItem_C*>(item)->GetItemEnchantmentVisualID();

    if (visual == -1) {
        return;
    }

    for (CM2Model* model = this->m_worldModel->m_attachList; model; model = model->m_attachNext) {
        if (model->m_attachmentId == static_cast<uint32_t>(link)) {
            CCharacterComponent::ComponentUtilAddItemVisual(model, visual);
            CGGameUI::UnitModelUpdate(this->m_obj->m_guid, 3);
            return;
        }
    }
}

// VERIFY: b478364d auto
// OFFSET: 0x006D6F40
void CGPlayer_C::HandleSkinning(CGObject_C* object, bool autoInteract, bool a4) {
    float minRange;
    float maxRange;
    Spell_C::GetSpellRange(this, CGSpellBook::GetSkinningSpell(object), &minRange, &maxRange, object);
    C3Vector target = object->GetPosition();
    C3Vector position = this->GetPosition();
    float dx = position.x - target.x;
    float dy = position.y - target.y;
    float dz = position.z - target.z;

    if (maxRange * maxRange <= dx * dx + dz * dz + dy * dy && autoInteract) {
        this->SetTrackingTarget(object->m_obj->m_guid, 9);
    } else {
        Spell_C::CastSpell(CGSpellBook::GetSkinningSpell(object), nullptr, object->m_obj->m_guid, false);
    }

    if (a4) {
        this->m_autoLoot = CGGameUI::IsAutoLooting() != 0;

        if (this->m_autoLoot && this->m_lootTarget) {
            CGLootInfo::DoAutoLoot(0);
        }
    }
}

// VERIFY: 1b6ef3fb manual
// OFFSET: 0x006D73E0
int32_t CGPlayer_C::XRayVisionEffectProc(WOWGUID guid, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit || !unit->UsesNpcBakedTexture()) {
        return 1;
    }

    CCharacterComponent* component = unit->m_characterComponent;

    if (component) {
        component->RemoveItemBySlot(INVSLOT_HEAD);
        component->RemoveItemBySlot(INVSLOT_SHOULDER);
    }

    unit->FreeComponent();
    const SpellRec* spell = static_cast<const SpellRec*>(param);

    if (!spell) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    const SpellVisualRec* visual = GetSpellVisual(spell);

    if (visual) {
        SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_targetImpactKit);

        if (kit) {
            PlaySpellVisuaKitData data(spell, kit, 6);
            unit->PlaySpellVisualKit(data);
        }
    }

    return 1;
}

// VERIFY: e620b509 manual
// OFFSET: 0x006D9B40
int32_t CGPlayer_C::OnItemRefundResult(CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    uint32_t result;
    msg->Get(result);

    if (guid) {
        ClntObjMgrObjectPtr(guid, TYPEMASK_ITEM, __FILE__, __LINE__);
    }

    if (result) {
        char text[256];

        if (result == 10) {
            CGGameUI::DisplayError(ERR_INV_FULL);
        } else if (result == 11) {
            CGGameUI::DisplayError(ERR_CURRENCY_FULL);
        } else {
            CGChat::AddChatMessage(FrameScript_GetText("UNABLE_TO_REFUND_ITEM", -1, GENDER_NOT_APPLICABLE), 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
        }

        SStrPrintf(text, sizeof(text), "Item purchase refund error code %d", result);
        ConsoleWrite(text, ERROR_COLOR);
        return 1;
    }

    int32_t money;
    msg->Get(reinterpret_cast<uint32_t&>(money));
    int32_t honor;
    msg->Get(reinterpret_cast<uint32_t&>(honor));
    int32_t arena;
    msg->Get(reinterpret_cast<uint32_t&>(arena));
    int32_t items[5];
    int32_t counts[5];

    for (uint32_t i = 0; i < 5; i++) {
        msg->Get(reinterpret_cast<uint32_t&>(items[i]));
        msg->Get(reinterpret_cast<uint32_t&>(counts[i]));
    }

    char text[3000];
    SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_REFUND_MSG", -1, GENDER_NOT_APPLICABLE));
    CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);

    if (money > 0) {
        GetCurrencyText(money, text, sizeof(text), ", ");
        CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    if (honor > 0) {
        uint32_t current = this->m_player->m_playerFieldHonorCurrency;

        if (current + honor <= 75000) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("CURRENCY_AMOUNT_REFUND_FORMAT", -1, GENDER_NOT_APPLICABLE), honor, FrameScript_GetText("HONOR_POINTS", -1, GENDER_NOT_APPLICABLE));
        } else {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("CURRENCY_AMOUNT_REFUND_FORMAT", -1, GENDER_NOT_APPLICABLE), 75000 - current, FrameScript_GetText("HONOR_POINTS", -1, GENDER_NOT_APPLICABLE));
        }

        CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    if (arena > 0) {
        uint32_t current = this->m_player->m_playerFieldArenaCurrency;

        if (current + arena <= 10000) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("CURRENCY_AMOUNT_REFUND_FORMAT", -1, GENDER_NOT_APPLICABLE), arena, FrameScript_GetText("ARENA_POINTS", -1, GENDER_NOT_APPLICABLE));
        } else {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("CURRENCY_AMOUNT_REFUND_FORMAT", -1, GENDER_NOT_APPLICABLE), 10000 - current, FrameScript_GetText("ARENA_POINTS", -1, GENDER_NOT_APPLICABLE));
        }

        CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    for (uint32_t i = 0; i < 5; i++) {
        if (counts[i] <= 0) {
            continue;
        }

        WOWGUID none = WOWGUID();
        ItemStats_C* stats = g_itemDBCache.GetRecord(items[i], none, nullptr, nullptr, false);

        if (!stats) {
            continue;
        }

        const char* name = stats->m_name[FrameScript_GetPluralIndex(counts[i])];

        if (!name || !*name) {
            name = stats->m_name[0];
        }

        SStrPrintf(text, sizeof(text), FrameScript_GetText("CURRENCY_AMOUNT_REFUND_FORMAT", -1, GENDER_NOT_APPLICABLE), counts[i], name);
        CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    return 1;
}

// VERIFY: 76b03ffe manual
// OFFSET: 0x006DA8F0
int32_t OnUpdatePlayerRuneRegen(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 1;
    }

    float rate = *static_cast<const float*>(value);
    uint32_t type = (offset - 0x1214) >> 2;
    uint32_t predicted = CGRuneInfo::m_predictedRunes;
    float current = player->m_player->m_playerRuneRegen1[type];

    if (rate < 0.0099999998f || current < 0.0099999998f) {
        return 1;
    }

    for (int32_t i = 0; i < 8; i++) {
        if (static_cast<uint32_t>(CGRuneInfo::GetRuneTypeByIndex(i, false)) != type || ((1 << i) & predicted) || !CGRuneInfo::GetRuneLastUsedTime(i)) {
            continue;
        }

        uint32_t now = FrameTime::s_curTimeMs;
        double progress = (now - CGRuneInfo::GetRuneLastUsedTime(i)) * rate * 0.001;
        float clamped = static_cast<float>(progress);

        if (progress > 1.0) {
            progress = 1.0;
            clamped = 1.0f;
        } else if (progress < 0.0) {
            progress = 0.0;
            clamped = 0.0f;
        }

        CGRuneInfo::SetRuneLastUsedTime(i, FrameTime::s_curTimeMs - static_cast<uint32_t>(progress * 1000.0 / current));
        CGRuneInfo::ComputeRuneRechargeTime(i, clamped);
        CGActionBar::SignalUpdateCooldown();
    }

    return 1;
}

// VERIFY: 0d412843 manual
// OFFSET: 0x006DB1C0
void CGPlayer_C::TalkToTalentMaster(const WOWGUID& guid, uint32_t cost) {
    WOWGUID target = guid ? guid : s_talentMaster;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    C3Vector position = this->GetPosition();
    C3Vector unitPosition = unit->GetPosition();
    float dy = unitPosition.y - position.y;
    float dx = unitPosition.x - position.x;
    float reach = unit->m_unit->m_unitFieldCombatreach + 4.0f;

    if (reach * reach < dx * dx + dy * dy + (unitPosition.z - position.z) * (unitPosition.z - position.z)) {
        return;
    }

    if (guid) {
        s_talentMaster = guid;
        s_talentWipeCost = cost;
        FrameScript_SignalEvent(SCRIPT_EVENT_CONFIRM_TALENT_WIPE, "%d", cost);
    } else if (s_talentWipeCost <= this->GetCoinage()) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(MSG_TALENT_WIPE_CONFIRM));
        msg.Put(s_talentMaster.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
    } else {
        CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
    }
}

// VERIFY: 8224adbd manual
// OFFSET: 0x006DB5F0
void CGPlayer_C::TurnOffGuildTabardPurchase() {
    if (this->m_characterComponent) {
        this->m_characterComponent->TurnOffGuildTabardPurchase();
    }

    CGBag_C* bag = this->GetBag();

    if (!bag) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    WOWGUID tabard = bag->m_numSlots > 18 ? bag->m_slots[18] : WOWGUID();
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(tabard, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (item && item->GetInventoryType() == 19) {
        int32_t displayId = item->GetDisplayID();

        if (displayId) {
            this->LoadGuildTextures(displayId);
        }
    }

    CGGameUI::UnitModelUpdate(this->m_obj->m_guid, 3);
}

// VERIFY: 670b0b5c manual
// OFFSET: 0x006DC640
void CGPlayer_C::SetAFK(const char* message) {
    const char* text = message;

    if (!text || !*text) {
        text = FrameScript_GetText("DEFAULT_AFK_MESSAGE", -1, GENDER_NOT_APPLICABLE);
    }

    if (!CGChat::m_afk) {
        if (this->m_player->m_playerFlags & 0x4) {
            CGChat::AddChatMessage(FrameScript_GetText("CLEARED_DND", -1, GENDER_NOT_APPLICABLE), 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
        }

        char buffer[3000];
        SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("MARKED_AFK_MESSAGE", -1, GENDER_NOT_APPLICABLE), text);
        CGChat::AddChatMessage(buffer, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
        CGChat::m_afk = 1;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_MESSAGECHAT));
    msg.Put(static_cast<uint32_t>(23));
    msg.Put(static_cast<uint32_t>(0));
    msg.PutString(text);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 498dd8bc manual
// OFFSET: 0x006DCE70
void CGPlayer_C::OpenWrappedItem(CGItem_C* item) {
    uint8_t bag;
    uint8_t slot;

    if (!this->GetPackAndSlot(item, &bag, &slot)) {
        return;
    }

    SI2::PlaySoundKit("UnwrapGift", nullptr, nullptr, nullptr);
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_OPEN_ITEM));
    msg.Put(bag);
    msg.Put(slot);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 4d76dd50 auto
// OFFSET: 0x006E0B90
void CGPlayer_C::EquipItem(CGItem_C* item) {
    if (!item || !this->m_characterComponent || item->GetDisplayID() <= 0) {
        return;
    }

    VisibleItem_C visible;
    visible.m_itemId = item->m_obj->m_entryID;
    visible.m_enchantment[0] = 0;
    visible.m_enchantment[1] = 0;

    for (int32_t i = 0; i < 19; i++) {
        if (!((1 << i) & 0x7C3FD)) {
            continue;
        }

        WOWGUID slotGuid = i < this->m_bag.m_numSlots ? this->m_bag.m_slots[i] : WOWGUID();

        if (slotGuid == item->m_obj->m_guid) {
            this->EquipVisibleItem(&visible, i);
        }
    }
}

// VERIFY: a64adceb manual
// OFFSET: 0x006E12E0
void CGPlayer_C::FillPartyMemberState(CGPartyMemberState* state) {
    state->m_flags = 1;

    if (this->m_player->m_playerFlags & 0x10) {
        state->m_flags = 9;
    }

    if (this->m_unit->m_unitFieldBytes2[1] & 0x1) {
        state->m_flags |= 0x2;
    }

    if (this->m_unit->m_unitFieldBytes2[1] & 0x4) {
        state->m_flags |= 0x10;
    }

    if (this->m_unit->m_unitFieldHealth <= 0) {
        state->m_flags |= 0x4;
    }

    if (this->m_player->m_playerFlags & 0x2) {
        state->m_flags |= 0x40;
    }

    if (this->m_player->m_playerFlags & 0x4) {
        state->m_flags |= 0x80;
    }

    if (this->m_unit->m_unitDynamicFlags & 0x40) {
        state->m_flags |= 0x100;
    }

    if (this->HasVehicleTransport()) {
        state->m_flags |= 0x200;
    }

    state->m_health = this->m_unit->m_unitFieldHealth;
    state->m_maxHealth = this->m_unit->m_unitFieldMaxhealth;
    uint8_t powerType = this->m_unit->m_unitFieldBytes0[3];
    state->m_powerType = powerType;
    state->m_power = this->m_unit->m_unitFieldPower[powerType];
    state->m_maxPower = this->m_unit->m_unitFieldMaxpower[powerType];
    state->m_level = this->m_unit->m_unitFieldLevel;

    if (this->GetTransportGUID()) {
        state->m_zoneID = 0;
    } else {
        state->m_zoneID = CGGameUI::m_areaID;
        C3Vector position = this->GetPosition();
        state->m_posX = static_cast<int16_t>(position.x);
        state->m_posY = static_cast<int16_t>(position.y);
    }

    uint32_t count = 0;

    for (uint32_t i = 0; i < this->GetAuraCount(); i++) {
        AuraData* aura = this->GetAura(i);

        if (!aura->m_spellId || !(aura->m_flags & 0x7)) {
            continue;
        }

        state->m_auras[count] = aura->m_spellId;
        state->m_auraFlags[count] = aura->m_flags;
        count++;

        if (count == 64) {
            break;
        }
    }

    if (count < 64) {
        memset(&state->m_auras[count], 0, sizeof(int32_t) * (64 - count));
    }

    CGUnitData* unit = this->m_unit;
    state->m_pet.m_guid = unit->m_unitFieldCharm ? unit->m_unitFieldCharm : unit->m_unitFieldSummon;

    if (!(state->m_flags & 0x200)) {
        state->m_vehicleSeatId = 0;
        return;
    }

    CGUnit_C* transport = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ this->GetTransportGUID() }, TYPEMASK_UNIT, __FILE__, __LINE__));
    VehicleRec* vehicle = transport && transport->m_vehicle ? transport->m_vehicle->m_vehicleRec : nullptr;
    uint8_t seat = this->m_movementData.m_transportSeat;

    if (vehicle && seat < 8) {
        state->m_vehicleSeatId = vehicle->m_seatID[seat];
    } else {
        state->m_vehicleSeatId = 0;
    }
}

// VERIFY: a376ebe3 manual
// OFFSET: 0x006E15A0
void CGPlayer_C::UpdatePartyMemberState() {
    CGPartyMemberState* remote = CGPartyInfo::GetRemoteStats(this->m_obj->m_guid);
    CGPartyMemberState* raid = CGRaidInfo::GetRemoteStats(this->m_obj->m_guid);

    if (!remote) {
        if (!raid) {
            return;
        }

        remote = raid;
    }

    CGPartyMemberState oldState = *remote;
    CGPartyMemberState newState = oldState;
    this->FillPartyMemberState(&newState);
    uint32_t changes = CGPlayer_C::BuildPartyMemberStatsChanges(&oldState, &newState);
    CGPlayer_C::DispatchPartyMemberStatsChanges(this->m_obj->m_guid, changes, &oldState, &newState);
}

// VERIFY: 1ec0ea5b manual
// OFFSET: 0x006E1A70
void __stdcall CGPlayer_C::ServerBindConfirm(WOWGUID item, uint64_t srcContainer, uint32_t srcSlot, uint64_t dstContainer, uint8_t dstSlot) {
    CGItem_C* object = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (!object) {
        return;
    }

    CGGameUI::SetCursorItem(item, WOWGUID{ srcContainer }, srcSlot, 1, 0, 1);
    uint32_t index = FindEmptySwapIndex();
    ITEMSWAP* swap = &s_itemSwaps.m_data[index];
    swap->m_srcContainer.m_guid = srcContainer;
    swap->m_srcSlot = srcSlot;
    swap->m_dstContainer.m_guid = dstContainer;
    swap->m_dstSlot = dstSlot;
    swap->m_itemId = object->m_obj->m_entryID;
    FrameScript_SignalEvent(SCRIPT_EVENT_AUTOEQUIP_BIND_CONFIRM, "%d", index);
}

// VERIFY: 0cd657ae manual
// OFFSET: 0x006E2880
bool CGPlayer_C::ToggleFarSight(CGObject_C* object) {
    WOWGUID farsight = this->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? this->m_player->m_playerFarsight : WOWGUID();
    CGObject_C* target = ClntObjMgrObjectPtr(farsight, TYPEMASK_OBJECT, __FILE__, __LINE__);
    bool hasFarsight = this->m_obj->m_guid == ClntObjMgrGetActivePlayer() && this->m_player->m_playerFarsight;

    if (hasFarsight && !target) {
        if (Spell_C_IsTargeting()) {
            Spell_C_StopTargeting();
        }

        CGGameUI::ResetCamera(WOWGUID());
        CGUnit_C::SetActiveMover(this->IsLocalClientControlled() ? this->m_obj->m_guid.m_guid : 0);
        CGInputControl::GetActive()->UpdatePlayer(FrameTime::s_curTimeMs, 1);
        return false;
    }

    if (target && (object || !(this->m_combatModeFlags & 0x1))) {
        if (!(this->m_combatModeFlags & 0x1)) {
            WDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_FAR_SIGHT));
            msg.Put(static_cast<uint8_t>(1));
            msg.Finalize();
            ClientServices::Send2(&msg);
            this->m_combatModeFlags |= 0x1;
        }

        this->SetCombatMode();

        if ((target->m_obj->m_type & TYPEMASK_UNIT) && static_cast<CGUnit_C*>(target)->IsLocalClientControlled()) {
            CGUnit_C::SetActiveMover(target->m_obj->m_guid.m_guid);
            CGWorldFrame::s_currentWorldFrame->SetCameraTarget(target, 0);
        } else {
            CGUnit_C::SetActiveMover(this->IsLocalClientControlled() ? this->m_obj->m_guid.m_guid : 0);
            CGInputControl::GetActive()->UpdatePlayer(OsGetAsyncTimeMs(), 1);
            CGWorldFrame::s_currentWorldFrame->SetCameraTarget(target, 0);
        }

        CGInputControl::GetActive()->UpdatePlayer(FrameTime::s_curTimeMs, 1);
        return true;
    }

    if (this->m_combatModeFlags & 0x1) {
        this->m_combatModeFlags &= ~0x1u;
        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_FAR_SIGHT));
        msg.Put(static_cast<uint8_t>(0));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    if (Spell_C_IsTargeting()) {
        Spell_C_StopTargeting();
    }

    CGGameUI::ResetCamera(WOWGUID());
    CGUnit_C::SetActiveMover(this->IsLocalClientControlled() ? this->m_obj->m_guid.m_guid : 0);
    CGInputControl::GetActive()->UpdatePlayer(FrameTime::s_curTimeMs, 1);
    return true;
}

// VERIFY: 8ae4c78f auto
// OFFSET: 0x006E2B00
void CGPlayer_C::ClearFarSight() {
    if (!(this->m_combatModeFlags & 0x1)) {
        return;
    }

    this->ToggleFarSight(nullptr);
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera) {
        camera->PickVehicleCamera();
        this->VehiclePassengerUpdateWorldCameraStateInstantly();
    }
}

// VERIFY: 53fbb7d1 manual
// OFFSET: 0x006E4FD0
int32_t FarsightChangeHandler(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    if (guid != ClntObjMgrGetActivePlayer()) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 1;
    }

    CGObject_C* target = ClntObjMgrObjectPtr(WOWGUID{ player->GetFarSightGuid() }, TYPEMASK_OBJECT, __FILE__, __LINE__);

    if (target) {
        player->ToggleFarSight(target);
        CGPetInfo::FindPossessedSpell();
    } else {
        player->ClearFarSight();
        CGPetInfo::SetPossessedSpell(0);
    }

    CGPetInfo::UpdatePetTempBar();
    FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_FARSIGHT_FOCUS_CHANGED, nullptr);

    if (player->m_unit->m_unitFieldHealth <= 0) {
        CGGameUI::UpdateActivePlayer();
    }

    return 1;
}

// VERIFY: d6443501 manual
// OFFSET: 0x007554F0
float CGPlayer_C::CalcRewardHonor(float honor) {
    if (!(this->m_obj->m_type & TYPEMASK_PLAYER)) {
        return 0.0f;
    }

    int32_t index = this->m_unit->m_unitFieldLevel - 1;

    if (index >= g_teamContributionPointsDB.m_numRecords) {
        return 0.0f;
    }

    return g_teamContributionPointsDB.GetRecordByIndex(index)->m_data * honor;
}

// VERIFY: bef8bf80 manual
// OFFSET: 0x006CCF10
int32_t Packet_SMSG_VOICE_SESSION_ROSTER_UPDATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint64_t session = 0;
    msg->Get(session);
    uint16_t networkId = 0;
    msg->Get(networkId);
    uint8_t type = 0;
    msg->Get(type);
    char name[128];
    msg->GetString(name, sizeof(name));
    uint8_t key[16];
    msg->GetArray(key, sizeof(key));
    uint32_t ip = 0;
    msg->Get(ip);
    uint16_t port = 0;
    msg->Get(port);
    char address[32];
    SStrPrintf(address, sizeof(address), "%d.%d.%d.%d:%d", ip >> 24, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, port);
    uint8_t count = 0;
    msg->Get(count);
    uint64_t self = 0;
    msg->Get(self);
    uint8_t selfNetworkId = 0;
    msg->Get(selfNetworkId);
    uint8_t selfFlags = 0;
    msg->Get(selfFlags);
    VoiceRosterMember members[40];
    int32_t remote = count - 1;

    for (int32_t i = 0; i < remote; i++) {
        uint64_t guid = 0;
        msg->Get(guid);
        members[i].m_guid.m_guid = guid;
        uint8_t value = 0;
        msg->Get(value);
        members[i].m_priority = value;
        value = 0;
        msg->Get(value);
        members[i].m_talkerPriority = value;
        value = 0;
        msg->Get(value);
        members[i].m_flags = value;
        SStrCopy(members[i].m_address, address, sizeof(members[i].m_address));
    }

    VoiceSessionRosterUpdate(session, networkId, type, name, key, self, selfNetworkId, selfFlags, remote, members);
    ConsolePrintf("VoiceSessionRosterUpdate - type %d, channelName <%s>, %d members", type, name, count);
    return 1;
}

// VERIFY: f847071c auto
// OFFSET: 0x006CD0E0
int32_t Packet_SMSG_AVAILABLE_VOICE_CHANNEL(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint64_t session = 0;
    msg->Get(session);
    uint8_t type = 0;
    msg->Get(type);
    char name[128];
    msg->GetString(name, sizeof(name));
    uint64_t self = 0;
    msg->Get(self);
    int32_t current = ComSatClient_IsCurrentSession(session);
    VoiceSessionLeave(self, session, type);
    ComSatClient_PopulateSessionDataWithoutJoin(session, type, name, self);

    if (current) {
        ComSatClient_SetCurrentSession(type, name);
    }

    ConsolePrintf("OnAvailableVoiceSession - type %d, name = <%s>", type, name);
    return 1;
}

// VERIFY: 6a474b57 auto
// OFFSET: 0x006CD1B0
int32_t Packet_SMSG_VOICE_SESSION_LEAVE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint64_t talker = 0;
    msg->Get(talker);
    uint64_t session = 0;
    msg->Get(session);
    VoiceSessionLeave(talker, session, 4);
    return 1;
}

// VERIFY: d2f40e6b auto
// OFFSET: 0x006CD210
int32_t Packet_SMSG_VOICE_SET_TALKER_MUTED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    guid.m_guid = 0;
    msg->Get(guid.m_guid);
    uint8_t muted = 0;
    msg->Get(muted);
    VoiceSessionSetTalkerMuted(guid, muted == 0);
    return 1;
}

// VERIFY: 56fcd2e9 auto
// OFFSET: 0x006CFEF0
int32_t Packet_SMSG_VOICE_PARENTAL_CONTROLS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t value = 0;
    msg->Get(value);
    int32_t voiceChat = value;
    value = 0;
    msg->Get(value);
    int32_t microphone = value;

    if (!s_cvarEnableVoiceChat || !s_cvarEnableMicrophone) {
        return 1;
    }

    int32_t oldVoiceChat = s_cvarEnableVoiceChat->m_intValue;
    int32_t oldMicrophone = s_cvarEnableMicrophone->m_intValue;
    char text[10];
    SStrPrintf(text, sizeof(text), "%i", voiceChat);
    s_cvarEnableVoiceChat->Set(text, true, false, false, true);
    SStrPrintf(text, sizeof(text), "%i", microphone);
    s_cvarEnableMicrophone->Set(text, true, false, false, true);

    if (!voiceChat && oldVoiceChat) {
        if (CGGameUI::m_initialized) {
            CGChat::DisplayParentControlsNotice(ERR_VOICE_CHAT_PARENTAL_DISABLE_ALL);
        }
    } else if (!microphone && oldMicrophone && CGGameUI::m_initialized) {
        CGChat::DisplayParentControlsNotice(ERR_VOICE_CHAT_PARENTAL_DISABLE_MIC);
    }

    return 1;
}

// VERIFY: a5006ce1 auto
// OFFSET: 0x006DC7E0
int32_t CGPlayer_C::GetVirtualItemDisplayRec(uint32_t slot, ItemDisplayInfoRec& rec) const {
    return g_itemDisplayInfoDB.GetRecord(this->GetVirtualItemDisplayID(slot), &rec);
}
