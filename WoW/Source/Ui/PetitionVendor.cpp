#include "PetitionVendor.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include <cstring>
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/ArenaTeamCache.h"
#include "WoW/Source/Object/CGPetition.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/ArenaTeamInfo.h"
#include "WoW/Source/Ui/ScriptEvents.h"
// STATIC: 0x00C0F7C8
WOWGUID CGPetitionVendorInfo::m_vendor;

// STATIC: 0x00C0F7D0
uint32_t CGPetitionVendorInfo::m_numPetitionItems;

// STATIC: 0x00C0F7D8
WOWGUID CGPetitionVendorInfo::m_petitionItem;

// STATIC: 0x00C0F7D4
uint8_t CGPetitionVendorInfo::m_petitionItemSlot;

// STATIC: 0x00C0F7E0
WOWGUID CGPetitionVendorInfo::m_petitionItemContainer;

// STATIC: 0x00C0F6D8
GuildPetitionInfo CGPetitionVendorInfo::m_petitionItems[10];

// STATIC: 0x00C0F7E8
uint32_t CGPetitionVendorInfo::m_arenaTeamSize;

// STATIC: 0x00ACF7D8
uint32_t CGPetitionVendorInfo::m_arenaBackgroundColor = 0xFFFFFFFF;

// STATIC: 0x00ACF7DC
uint32_t CGPetitionVendorInfo::m_arenaEmblemStyle = 0xFFFFFFFF;

// STATIC: 0x00ACF7E0
uint32_t CGPetitionVendorInfo::m_arenaEmblemColor = 0xFFFFFFFF;

// STATIC: 0x00ACF7E4
uint32_t CGPetitionVendorInfo::m_arenaBorderStyle = 0xFFFFFFFF;

// STATIC: 0x00ACF7E8
uint32_t CGPetitionVendorInfo::m_arenaBorderColor = 0xFFFFFFFF;


// VERIFY: 645b030e manual
// OFFSET: 0x005A2140
int32_t CGPetitionVendorInfo::ClickPetitionButton(WOWGUID item, WOWGUID container, uint8_t slot) {
    if (!CGPetitionVendorInfo::m_vendor) {
        return 0;
    }

    if (!item) {
        CGPetitionVendorInfo::m_petitionItem = WOWGUID();
        FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_VENDOR_UPDATE, nullptr);
        return 1;
    }

    CGItem_C* object = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__));

    if (!object || !(object->GetItemStaticFlags(0) & ITEM_FLAG_PETITION)) {
        return 0;
    }

    CGPetitionVendorInfo::m_petitionItem = item;
    CGPetitionVendorInfo::m_petitionItemContainer = container;
    CGPetitionVendorInfo::m_petitionItemSlot = slot;
    FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_VENDOR_UPDATE, nullptr);
    return 1;
}

// VERIFY: c7a461c2 manual
// OFFSET: 0x005A22A0
void CGPetitionVendorInfo::ShowPetitionList(WOWGUID vendor, const GuildPetitionInfo* petitions, uint32_t count) {
    CGGameUI::SetInteractTarget(vendor);
    CGPetitionVendorInfo::m_vendor = vendor;

    if (vendor) {
        CGPetitionVendorInfo::m_petitionItem = WOWGUID();
        FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_VENDOR_UPDATE, nullptr);
    }

    if (count >= 10) {
        count = 10;
    }

    memcpy(CGPetitionVendorInfo::m_petitionItems, petitions, sizeof(GuildPetitionInfo) * count);
    CGPetitionVendorInfo::m_numPetitionItems = count;
    CGPetitionVendorInfo::m_arenaTeamSize = 0;
    CGPetitionVendorInfo::m_arenaBackgroundColor = 0xFFFFFFFF;
    CGPetitionVendorInfo::m_arenaEmblemStyle = 0xFFFFFFFF;
    CGPetitionVendorInfo::m_arenaEmblemColor = 0xFFFFFFFF;
    CGPetitionVendorInfo::m_arenaBorderStyle = 0xFFFFFFFF;
    CGPetitionVendorInfo::m_arenaBorderColor = 0xFFFFFFFF;
    FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_VENDOR_SHOW, nullptr);
}

// VERIFY: 09a25e7d manual
// OFFSET: 0x005A2350
void CGPetitionVendorInfo::BuyPetition(uint32_t index, const char* name) {
    if (index >= CGPetitionVendorInfo::m_numPetitionItems || !name || !*name || !CGPetitionVendorInfo::m_vendor) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    if (!CGPetitionVendorInfo::m_petitionItems[index].m_charterType && player->m_player->m_playerGuildid) {
        CGGameUI::DisplayError(ERR_ALREADY_IN_GUILD);
        return;
    }

    if (CGPetitionVendorInfo::m_petitionItems[index].m_charterType == 1) {
        for (uint32_t i = 0; i < 3; i++) {
            ArenaTeam* team = CGArenaTeamInfo::GetArenaTeam(i);

            if (!team || !team->m_teamId) {
                continue;
            }

            WOWGUID none = WOWGUID();
            ArenaTeamCache* cache = g_arenaTeamCache.GetRecord(team->m_teamId, none, nullptr, nullptr, false);

            if (cache && cache->m_type == CGPetitionVendorInfo::m_petitionItems[index].m_requiredSignatures + 1) {
                CGGameUI::DisplayError(ERR_ALREADY_IN_ARENA_TEAM);
                return;
            }
        }
    }

    if (CGPetitionVendorInfo::m_petitionItems[index].m_charterType == 1) {
        int32_t level = player->m_unit->m_unitFieldLevel;

        if (level != 60 && level != 70 && level != 80) {
            int32_t required = 70;

            if (level > 70 && level < 80) {
                required = 80;
            }

            CGGameUI::DisplayError(ERR_ARENA_TEAM_LEVEL_TOO_LOW_I, required);
        }
    }

    if (player->GetCoinage() < CGPetitionVendorInfo::m_petitionItems[index].m_cost) {
        CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
        return;
    }

    uint32_t buffer[sizeof(CGPetition) / sizeof(uint32_t)];
    memset(buffer, 0, sizeof(buffer));
    CGPetition* petition = reinterpret_cast<CGPetition*>(buffer);
    SStrCopy(petition->m_title, name, sizeof(petition->m_title));
    petition->m_muid = CGPetitionVendorInfo::m_petitionItems[index].m_index;
    CGPlayer_C::BuyPetition(CGPetitionVendorInfo::m_vendor, petition);
}

// VERIFY: 03561478 auto
// OFFSET: 0x005A2880
void PetitionVendorClose() {
    if (!CGPetitionVendorInfo::m_vendor) {
        return;
    }

    CGGameUI::ClearInteractTarget(CGPetitionVendorInfo::m_vendor, 0);
    CGPetitionVendorInfo::m_vendor = WOWGUID();
    FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_VENDOR_CLOSED, nullptr);
}

// VERIFY: 91326819 auto
// OFFSET: 0x005A1F30
void PetitionVendorItemCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_VENDOR_UPDATE, nullptr);
    }
}

// VERIFY: 670e5b68 auto
// OFFSET: 0x005A1F50
void CGPetitionVendorInfo::EnterWorld() {
    CGPetitionVendorInfo::m_numPetitionItems = 0;
    CGPetitionVendorInfo::m_petitionItem = WOWGUID{ 0 };
}
