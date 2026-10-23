#include "ActionBarFrame.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ItemRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/OverrideSpellDataRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/EquipmentManager.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Ui/UIBindings.h"
#include "WoW/Source/Ui/UIMacros.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/DB/Rec/SummonPropertiesRec.h"

// STATIC: 0x00C1DC98
int32_t CGActionBar::m_slotNoMana[144];

// STATIC: 0x00C1DED8
int32_t CGActionBar::m_slotUsable[144];

// STATIC: 0x00C1E118
int32_t CGActionBar::m_slotItemCount[144];

// STATIC: 0x00C1E358
uint32_t CGActionBar::m_slotActions[144];

// STATIC: 0x00C1E59C
int32_t CGActionBar::m_bonusBarOffset;

// STATIC: 0x00C1E5A0
uint32_t CGActionBar::m_tempPageActiveFlags;

// STATIC: 0x00C1E598
uint32_t CGActionBar::m_currentPage;

// VERIFY: b0c0fee1 auto
// OFFSET: 0x005A7A80
void CGActionBar::HideGrid() {
    FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_HIDEGRID, nullptr);
}

// VERIFY: a8f31305 auto
// OFFSET: 0x005648A0
int32_t CGActionBar::IsMacro(uint32_t slot) {
    uint32_t action = CGActionBar::m_slotActions[slot];
    return action && (action & 0xF0000000) == 0x40000000;
}

// VERIFY: 4dbd4b01 manual
// OFFSET: 0x005648D0
uint32_t CGActionBar::GetMacroID(uint32_t slot) {
    uint32_t action = CGActionBar::m_slotActions[slot];

    if (action && (action & 0xF0000000) == 0x40000000) {
        return action & 0xBFFFFFFF;
    }

    return 0;
}

// VERIFY: 0d7360f9 auto
// OFFSET: 0x005A7890
int32_t CGActionBar::IsItem(uint32_t slot) {
    uint32_t action = CGActionBar::m_slotActions[slot];
    return action && (action & 0xF0000000) == 0x80000000;
}

// VERIFY: 2927bc93 auto
// OFFSET: 0x005A78C0
int32_t CGActionBar::IsPetAction(uint32_t slot) {
    uint32_t action = CGActionBar::m_slotActions[slot];
    return action && (action & 0xF0000000) == 0x10000000;
}

// VERIFY: ac5a6409 auto
// OFFSET: 0x005A78F0
int32_t CGActionBar::IsEquipmentSet(uint32_t slot) {
    uint32_t action = CGActionBar::m_slotActions[slot];
    return action && (action & 0xF0000000) == 0x20000000;
}

// VERIFY: b18cab5c auto
// OFFSET: 0x005A7950
int32_t CGActionBar::GetEquipmentSetID(uint32_t slot) {
    uint32_t action = CGActionBar::m_slotActions[slot];

    if (action && (action & 0xF0000000) == 0x20000000) {
        return action & 0xDFFFFFFF;
    }

    return -1;
}

// VERIFY: e56c7d55 auto
// OFFSET: 0x005A7A40
int32_t CGActionBar::HasMacro(uint32_t slot) {
    return CGUIMacros::GetMacroIndexByID(CGActionBar::m_slotActions[slot] & 0xBFFFFFFF) != -1;
}

// VERIFY: 6f49ff29 auto
// OFFSET: 0x005A8070
int32_t CGActionBar::GetItem(uint32_t slot) {
    if (slot >= sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0])) {
        return 0;
    }

    uint32_t action = CGActionBar::m_slotActions[slot];

    if (action && (action & 0xF0000000) == 0x40000000 && CGActionBar::HasMacro(slot)) {
        return CGActionBar::GetMacroItem(slot);
    }

    action = CGActionBar::m_slotActions[slot];

    if (action && (action & 0xF0000000) == 0x80000000) {
        return action & 0x7FFFFFFF;
    }

    return 0;
}

// VERIFY: 272afc23 manual
// OFFSET: 0x005A86B0
CGItem_C* CGActionBar::FindUseEnchantItemPtr(int32_t itemId) {
    if (CGItem_C::GetUseSpell(itemId)) {
        return nullptr;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return nullptr;
    }

    CGItem_C* item = player->m_bag.FindItemOfType(itemId, 161);

    if (!item) {
        return player->m_bag.FindItemOfType(itemId, 129);
    }

    return item;
}

// VERIFY: 1114cca9 auto
// OFFSET: 0x005A8C30
int32_t CGActionBar::GetSpell(uint32_t slot, int32_t& petSpell) {
    petSpell = 0;

    if (slot >= sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0])) {
        return 0;
    }

    uint32_t action = CGActionBar::m_slotActions[slot];

    if (action && (action & 0xF0000000) == 0x40000000 && CGActionBar::HasMacro(slot)) {
        return CGActionBar::GetMacroSpell(slot, petSpell);
    }

    action = CGActionBar::m_slotActions[slot];

    if (action && !(action & 0xF0000000)) {
        return action;
    }

    if (CGActionBar::IsItem(slot)) {
        return CGActionBar::GetItemSpell(slot);
    }

    if (CGActionBar::IsPetAction(slot)) {
        int32_t spellId = CGActionBar::GetPetActionSpell(slot);
        SpellRec spell;

        if (!g_spellDB.GetRecord(spellId, &spell) || !(spell.m_attributesExB & 0x40000)) {
            petSpell = 1;
        }

        return spellId;
    }

    return 0;
}

// VERIFY: 356cf1ff manual
// OFFSET: 0x005A95E0
int32_t CGActionBar::ActionHasRange(uint32_t slot) {
    if (slot > 143) {
        return 0;
    }

    SpellRec spell;
    int32_t petSpell;
    int32_t spellId = CGActionBar::GetSpell(slot, petSpell);

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return 0;
    }

    CGUnit_C* caster = petSpell ? static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__)) : ClntObjMgrGetActivePlayerObj();

    if (!caster) {
        return 0;
    }

    float minRange;
    float maxRange;
    Spell_C_GetMinMaxRange(caster, &spell, &minRange, &maxRange, nullptr);

    if (fequal(minRange, 0.0f) && fequal(maxRange, 0.0f)) {
        return 0;
    }

    return 1;
}

// VERIFY: c3489cdd auto
// OFFSET: 0x005A96D0
int32_t CGActionBar::IsAttackAction(uint32_t slot) {
    if (slot >= sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0])) {
        return 0;
    }

    SpellRec spell;
    int32_t petSpell;

    if (g_spellDB.GetRecord(CGActionBar::GetSpell(slot, petSpell), &spell) && spell.m_effect[0] == SPELL_EFFECT_ATTACK) {
        return 1;
    }

    return 0;
}

// VERIFY: 67ae0d31 auto
// OFFSET: 0x005A9760
int32_t CGActionBar::IsRangedAttackAction(uint32_t slot) {
    if (slot >= sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0])) {
        return 0;
    }

    SpellRec spell;
    int32_t petSpell;

    if (g_spellDB.GetRecord(CGActionBar::GetSpell(slot, petSpell), &spell) && (spell.m_attributes & 0x2) && (spell.m_attributesExB & 0x20)) {
        return 1;
    }

    return 0;
}

// VERIFY: 22df6d59 manual
// OFFSET: 0x005A9E20
int32_t CGActionBar::UpdateUsableAction(uint32_t slot, int32_t& noMana) {
    noMana = 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    int16_t overrideId = static_cast<int16_t>(player->m_player->m_playerFieldBytes2);
    OverrideSpellDataRec* overrideRec = nullptr;

    if (overrideId) {
        overrideRec = g_overrideSpellDataDB.GetRecord(overrideId);
    }

    int32_t itemId = CGActionBar::GetItem(slot);

    if (itemId) {
        if (!CGActionBar::m_slotItemCount[slot] || Spell_C_NeedsCooldownEvent(itemId) || (overrideRec && !(overrideRec->m_flags & 0x1))) {
            return 0;
        }

        WOWGUID none = { 0 };
        ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

        if (stats) {
            MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);

            if (map && map->m_instanceType == 4 && !(stats->m_flags[0] & 0x200000)) {
                CGItem_C* item = CGActionBar::FindUseEnchantItemPtr(itemId);
                int32_t charges;
                uint32_t cooldown;
                uint32_t categoryCooldown;
                uint32_t category;

                if (item) {
                    charges = item->GetUsableSpellCharges();
                    item->GetSpellCooldownInfo(cooldown, categoryCooldown, category);
                } else {
                    charges = CGItem_C::NumBaseCharges(itemId);
                    CGItem_C::GetCooldowns(itemId, cooldown, categoryCooldown, category);
                }

                if (charges < 0 || cooldown > 900000 || categoryCooldown > 900000) {
                    return 0;
                }
            }
        }
    }

    int32_t attack;

    if (CGActionBar::IsAttackAction(slot) || CGActionBar::IsRangedAttackAction(slot)) {
        attack = 1;

        if (overrideRec && !(overrideRec->m_flags & 0x2)) {
            return 0;
        }
    } else {
        attack = 0;
    }

    uint32_t action = CGActionBar::m_slotActions[slot];

    if (action && (action & 0xF0000000) == 0x20000000) {
        if (!CGEquipmentManager::CanUseEquipmentSets(false)) {
            return 0;
        }

        const EQUIPMENT_SET* set = CGEquipmentManager::GetSetByID(CGActionBar::GetEquipmentSetID(slot));

        if (!set || CGEquipmentManager::SetContainsLockedItems(set) || CGEquipmentManager::SetUsesLockedSlots(set)) {
            return 0;
        }

        if (player->m_unit->m_unitFieldFlags & 0x100000) {
            return 1;
        }
    }

    int32_t petSpell;
    int32_t spellId = CGActionBar::GetSpell(slot, petSpell);

    if (spellId) {
        if (!itemId && !attack && overrideRec && !(overrideRec->m_flags & 0x4) && !Spell_C_IsOverride(spellId, overrideRec)) {
            return 0;
        }

        CGUnit_C* caster;

        if (petSpell) {
            caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
        } else {
            caster = player;
        }

        if (!caster) {
            return 0;
        }

        SpellRec spell;

        if (!g_spellDB.GetRecord(spellId, &spell)) {
            return 0;
        }

        WOWGUID target = { 0 };

        if (CGActionBar::IsMacro(slot)) {
            MACRONODE* macro = CGUIMacros::GetMacroByID(CGActionBar::GetMacroID(slot));

            if (macro) {
                target = macro->m_target;
            }
        }

        ItemStats_C* stats = nullptr;

        if (itemId) {
            WOWGUID none = { 0 };
            stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);
        }

        if (!Spell_C_IsUsableAction(caster, &spell, target, stats, false)) {
            return 0;
        }

        noMana = !Spell_C_HaveSpellPower(caster, &spell);
        return 1;
    }

    if (!CGGameUI::m_hasControl) {
        return 0;
    }

    if (attack) {
        return !(player->m_unit->m_unitFieldFlags & 0x20000) && !(player->m_player->m_playerFlags & 0x1000000);
    }

    if (CGActionBar::IsItem(slot) || (CGActionBar::IsMacro(slot) && CGActionBar::HasMacro(slot))) {
        return 1;
    }

    if (CGActionBar::IsPetAction(slot)) {
        return 1;
    }

    if (CGActionBar::IsEquipmentSet(slot)) {
        return 1;
    }

    return 0;
}

// VERIFY: dc4895e6 auto
// OFFSET: 0x005AA470
void CGActionBar::UpdateUsable() {
    int32_t changed = 0;

    for (uint32_t i = 0; i < sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0]); i++) {
        int32_t noMana;
        int32_t usable = CGActionBar::UpdateUsableAction(i, noMana);

        if (usable != CGActionBar::m_slotUsable[i] || noMana != CGActionBar::m_slotNoMana[i]) {
            CGActionBar::m_slotUsable[i] = usable;
            CGActionBar::m_slotNoMana[i] = noMana;
            changed = 1;
        }
    }

    if (changed) {
        FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_UPDATE_USABLE, nullptr);
    }
}

// VERIFY: 321887df auto
// OFFSET: 0x005A80E0
int32_t CGActionBar::GetPetActionSpell(uint32_t slot) {
    uint32_t action = CGActionBar::m_slotActions[slot];
    uint32_t index = action && (action & 0xF0000000) == 0x10000000 ? action & 0xEFFFFFFF : 0;

    if (!CGPetInfo::GetPet(0) || index >= sizeof(CGPetInfo::m_actions) / sizeof(CGPetInfo::m_actions[0])) {
        return 0;
    }

    uint32_t petAction = CGPetInfo::m_actions[index].m_data;

    if (!petAction) {
        return 0;
    }

    uint32_t type = (petAction >> 24) & 0x3F;

    if (type != 1 && type - 8 > 9) {
        return 0;
    }

    return petAction & 0xFFFFFF;
}

// VERIFY: 1b3f1725 auto
// OFFSET: 0x005A89B0
int32_t CGActionBar::GetItemSpell(uint32_t slot) {
    int32_t itemId = CGActionBar::GetItem(slot);
    int32_t spell = CGItem_C::GetUseSpell(itemId);

    if (spell) {
        return spell;
    }

    CGItem_C* item = CGActionBar::FindUseEnchantItemPtr(itemId);

    if (!item) {
        return 0;
    }

    return item->GetEnchantmentSpell(0);
}

// VERIFY: f09a82bf auto
// OFFSET: 0x005A89F0
int32_t CGActionBar::GetMacroSpell(uint32_t slot, int32_t& petSpell) {
    uint32_t action = CGActionBar::m_slotActions[slot];
    uint32_t id = action && (action & 0xF0000000) == 0x40000000 ? action & 0xBFFFFFFF : 0;
    MACRONODE* macro = CGUIMacros::GetMacroByID(id);

    if (!macro) {
        return 0;
    }

    if (macro->m_itemID) {
        return CGActionBar::GetItemSpell(slot);
    }

    petSpell = macro->m_spellFlags;
    return macro->m_spellID;
}

// VERIFY: 3cf4ef31 auto
// OFFSET: 0x005A7860
int32_t CGActionBar::IsSpell(uint32_t slot) {
    uint32_t action = CGActionBar::m_slotActions[slot];
    return action && !(action & 0xF0000000);
}

// VERIFY: 9a0d1665 auto
// OFFSET: 0x005A7980
uint32_t CGPetInfo::GetPetMode() {
    if (CGPetInfo::m_petMode & 0x8000000) {
        return 0;
    }

    return static_cast<uint8_t>(CGPetInfo::m_petMode);
}

// VERIFY: 385d5d88 auto
// OFFSET: 0x005A79A0
PetAction* GetPetAction(uint32_t index) {
    if (CGPetInfo::GetPet(0) && index < sizeof(CGPetInfo::m_actions) / sizeof(CGPetInfo::m_actions[0])) {
        return &CGPetInfo::m_actions[index];
    }

    return nullptr;
}

// VERIFY: f0baee3c auto
// OFFSET: 0x005A7A70
void CGActionBar::ShowGrid() {
    FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_SHOWGRID, nullptr);
}

// VERIFY: efb36ab0 auto
// OFFSET: 0x005A7AE0
static int32_t GetMultiCastTotemCategory(int32_t slot) {
    if (slot < 132 || slot >= sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0])) {
        return 0;
    }

    switch ((slot - 132) % 4) {
        case 0:
            return 4;
        case 1:
            return 2;
        case 2:
            return 5;
        case 3:
            return 3;
    }

    return 0;
}

// VERIFY: 6d9ca407 auto
// OFFSET: 0x005A7B50
uint32_t CGActionBar::GetMultiCastSlotMaskForTotemCategory(int32_t category) {
    switch (category) {
        case 2:
            return 0x2;
        case 3:
            return 0x8;
        case 4:
            return 0x1;
        case 5:
            return 0x4;
        case 21:
            return 0xF;
    }

    return 0;
}

// VERIFY: 48490470 manual
// OFFSET: 0x005A7BC0
int32_t IsMultiCastSpellForSlot(int32_t slot, const SpellRec* spell, int32_t* category) {
    int32_t totem = (spell->m_attributesExG & 0x20) ? 1 : 0;
    *category = GetMultiCastTotemCategory(slot);

    if (!totem || !*category) {
        return totem;
    }

    for (uint32_t i = 0; i < sizeof(spell->m_requiredTotemCategoryID) / sizeof(spell->m_requiredTotemCategoryID[0]); i++) {
        if (spell->m_requiredTotemCategoryID[i] == *category) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 84bcf960 auto
// OFFSET: 0x005A7CB0
void CGActionBar::UpdateSelection() {
    FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_UPDATE_STATE, nullptr);
}

// VERIFY: 437cce19 auto
// OFFSET: 0x005A8160
int32_t CGActionBar::HasAction(uint32_t slot) {
    uint32_t action = CGActionBar::m_slotActions[slot];

    if (action && (action & 0xF0000000) == 0x40000000 && CGUIMacros::GetMacroIndexByID(action & 0xBFFFFFFF) == -1) {
        return 0;
    }

    action = CGActionBar::m_slotActions[slot];

    if (!action || (action & 0xF0000000) != 0x10000000) {
        return action != 0;
    }

    PetAction* petAction = GetPetAction((slot - 120) & 0xEFFFFFFF);

    if (!petAction || !petAction->m_data) {
        return 0;
    }

    if (((petAction->m_data >> 24) & 0x3F) - 6 <= 1) {
        return 1;
    }

    return CGActionBar::GetPetActionSpell(slot) != 0;
}

// VERIFY: 08b2d00e manual
// OFFSET: 0x005A84D0
const char* CGActionBar::GetAttackTexture() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return nullptr;
    }

    uint32_t form = player->GetShapeshiftForm();

    if (form) {
        SpellShapeshiftFormRec* formRec = g_spellShapeshiftFormDB.GetRecord(form);

        if (formRec && formRec->m_attackIconID) {
            SpellIconRec* icon = g_spellIconDB.GetRecord(formRec->m_attackIconID);

            if (icon) {
                return icon->m_textureFilename;
            }
        }
    }

    if (player->IsDisarmed(0)) {
        return "Interface\\Buttons\\Spell-Reset";
    }

    CGBag_C* bag = player->GetBag();
    WOWGUID guid = 15 < static_cast<uint32_t>(bag->m_numSlots) ? bag->m_slots[15] : WOWGUID();
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

    if (!item) {
        return "Interface\\Buttons\\Spell-Reset";
    }

    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);

    // STATIC: 0x00C1E5A8
    static char s_texture[260];

    SStrPrintf(s_texture, sizeof(s_texture), "%s%s%s", path, *path ? "\\" : "", item->GetInventoryArt());
    return s_texture;
}

// VERIFY: 26633ab9 manual
// OFFSET: 0x005A85E0
const char* CGActionBar::GetRangedTexture() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return nullptr;
    }

    if (player->m_unit->m_unitFieldFlags2 & 0x400) {
        return nullptr;
    }

    CGBag_C* bag = player->GetBag();
    WOWGUID guid = 17 < static_cast<uint32_t>(bag->m_numSlots) ? bag->m_slots[17] : WOWGUID();
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

    if (!item || item->GetSubtypeID() == 16) {
        return nullptr;
    }

    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);

    // STATIC: 0x00C1E6B0
    static char s_texture[260];

    SStrPrintf(s_texture, sizeof(s_texture), "%s%s%s", path, *path ? "\\" : "", item->GetInventoryArt());
    return s_texture;
}

// VERIFY: 36690456 manual
// OFFSET: 0x005A8A60
int32_t CGActionBar::CalculateItemCount(int32_t itemId) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    int32_t count = player->GetBag()->GetItemTypeCount(itemId, 128);

    if (!count) {
        return 0;
    }

    WOWGUID none = { 0 };
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (!stats || static_cast<int32_t>(stats->m_stackable) > 1) {
        return count;
    }

    CGItem_C* item = CGActionBar::FindUseEnchantItemPtr(itemId);
    int32_t charges = item ? item->GetUsableSpellCharges() : CGItem_C::NumBaseCharges(itemId);

    if (charges < 0) {
        return player->GetBag()->GetItemTypeCharges(itemId, 0);
    }

    int32_t spellId = item ? item->GetEnchantmentSpell(0) : CGItem_C::GetUseSpell(itemId);
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return count;
    }

    for (uint32_t i = 0; i < sizeof(spell.m_reagent) / sizeof(spell.m_reagent[0]); i++) {
        if (spell.m_reagent[i] && spell.m_reagentCount[i] > 0) {
            return Spell_C_GetSpellReagentUses(&spell);
        }
    }

    return count;
}

// VERIFY: 2be59043 manual
// OFFSET: 0x005A8E40
void CGActionBar::GetCooldown(int32_t slot, uint32_t& start, uint32_t& duration, uint32_t& enable) {
    start = 0;
    duration = 0;

    if (slot < 0 || slot >= sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0])) {
        return;
    }

    int32_t itemId = CGActionBar::GetItem(slot);

    if (itemId) {
        WOWGUID none = { 0 };

        if (g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false)) {
            CGItem_C* item = CGActionBar::FindUseEnchantItemPtr(itemId);

            if (item) {
                Spell_C_GetItemCooldown(item, &duration, &start, &enable);
            } else {
                Spell_C_GetItemCooldown(itemId, &duration, &start, &enable);
            }
        }
    } else {
        int32_t petSpell;
        int32_t spellId = CGActionBar::GetSpell(slot, petSpell);

        if (spellId) {
            Spell_C_GetSpellCooldown(spellId, petSpell, &duration, &start, &enable);
        }
    }
}

// VERIFY: 109f816f auto
// OFFSET: 0x005A97F0
const char* CGActionBar::GetTexture(int32_t slot) {
    if (slot < 0 || slot >= sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0])) {
        return nullptr;
    }

    // STATIC: 0x00C1E8BC
    static const char* s_iconPath = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);

    // STATIC: 0x00C1E7B8
    static char s_texture[260];

    if (!CGActionBar::m_slotActions[slot]) {
        return nullptr;
    }

    if (CGActionBar::IsAttackAction(slot)) {
        return CGActionBar::GetAttackTexture();
    }

    if (CGActionBar::IsRangedAttackAction(slot)) {
        const char* texture = CGActionBar::GetRangedTexture();

        if (texture && *texture) {
            return texture;
        }
    }

    if (CGActionBar::IsItem(slot)) {
        int32_t displayId = CGItem_C::GetDisplayID(CGActionBar::GetItem(slot));
        SStrPrintf(s_texture, sizeof(s_texture), "%s%s%s", s_iconPath, *s_iconPath ? "\\" : "", CGItem_C::GetInventoryArt(displayId));
        return s_texture;
    }

    if (CGActionBar::IsSpell(slot)) {
        SpellRec spell;
        int32_t petSpell;

        if (g_spellDB.GetRecord(CGActionBar::GetSpell(slot, petSpell), &spell)) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(petSpell ? CGPetInfo::GetPet(0) : ClntObjMgrGetActivePlayer(), TYPE_UNIT, __FILE__, __LINE__));
            SpellIconRec* icon = g_spellIconDB.GetRecord(Spell_C_IsActiveSpell(&spell, unit) && spell.m_activeIconID ? spell.m_activeIconID : spell.m_spellIconID);

            if (icon) {
                return icon->m_textureFilename;
            }
        }

        return nullptr;
    }

    if (CGActionBar::IsMacro(slot) && CGActionBar::HasMacro(slot)) {
        CGUIMacros::GetFullIconPath(CGActionBar::GetMacroID(slot), s_texture, sizeof(s_texture));
        return s_texture;
    }

    if (CGActionBar::IsEquipmentSet(slot)) {
        CGEquipmentManager::GetFullIconPath(CGActionBar::GetEquipmentSetID(slot), s_texture, sizeof(s_texture));
        return s_texture;
    }

    if (!CGActionBar::IsPetAction(slot)) {
        return nullptr;
    }

    PetAction* action = GetPetAction((slot - 120) & 0xEFFFFFFF);

    if (!action || !action->m_data) {
        return nullptr;
    }

    switch ((action->m_data >> 24) & 0x3F) {
        case 1:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17: {
            SpellRec spell;

            if (g_spellDB.GetRecord(action->m_data & 0xFFFFFF, &spell)) {
                CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
                SpellIconRec* icon = g_spellIconDB.GetRecord(Spell_C_IsActiveSpell(&spell, unit) && spell.m_activeIconID ? spell.m_activeIconID : spell.m_spellIconID);

                if (icon) {
                    return icon->m_textureFilename;
                }
            }

            return nullptr;
        }
        case 6:
            SStrPrintf(s_texture, sizeof(s_texture), "PET_%s_TEXTURE", CGPetInfo::GetModeToken(action->m_data & 0xFFFFFF));
            return FrameScript_GetText(s_texture, -1, GENDER_NOT_APPLICABLE);
        case 7:
            SStrPrintf(s_texture, sizeof(s_texture), "PET_%s_TEXTURE", CGPetInfo::GetOrdersToken(action->m_data & 0xFFFFFF));
            return FrameScript_GetText(s_texture, -1, GENDER_NOT_APPLICABLE);
    }

    return nullptr;
}

// VERIFY: 8513f070 auto
// OFFSET: 0x005AA240
int32_t CGActionBar::IsCurrentAction(int32_t slot) {
    if (slot < 0 || slot >= sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0]) || !CGActionBar::m_slotActions[slot]) {
        return 0;
    }

    int32_t itemId = CGActionBar::GetItem(slot);

    if (itemId) {
        return Spell_C_IsCurrentItem(itemId);
    }

    if (CGActionBar::IsAttackAction(slot)) {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
        return player && player->m_combat.m_attacking.m_guid;
    }

    int32_t petSpell;

    if (CGActionBar::IsPetAction(slot)) {
        PetAction* action = GetPetAction((slot - 120) & 0xEFFFFFFF);

        if (!action || !action->m_data) {
            return 0;
        }

        uint32_t id = action->m_data & 0xFFFFFF;

        switch ((action->m_data >> 24) & 0x3F) {
            case 6:
                return CGPetInfo::GetPetMode() == id;
            case 7:
                if ((CGPetInfo::m_petMode >> 8) == id || (id == 2 && CGPetInfo::m_attackingTarget)) {
                    return 1;
                }
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
                return Spell_C_IsCurrentSpell(CGActionBar::GetSpell(slot, petSpell));
        }

        return 0;
    }

    return Spell_C_IsCurrentSpell(CGActionBar::GetSpell(slot, petSpell));
}

// VERIFY: b884f81a manual
// OFFSET: 0x005AA390
void CGActionBar::SlotChanged(int32_t slot, int32_t sendToServer, int32_t silent) {
    if (slot < 0 || slot >= sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0])) {
        return;
    }

    int32_t noMana;
    CGActionBar::m_slotUsable[slot] = CGActionBar::UpdateUsableAction(slot, noMana);
    CGActionBar::m_slotNoMana[slot] = noMana;

    if (sendToServer) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_SET_ACTION_BUTTON));
        msg.Put(static_cast<uint8_t>(slot));
        msg.Put(CGActionBar::m_slotActions[slot]);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    if (!silent) {
        FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_SLOT_CHANGED, "%d", slot + 1);
    }
}

// VERIFY: 87a7805d auto
// OFFSET: 0x005AADB0
void ItemStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        CGActionBar::UpdateItem(id);
    }
}

// VERIFY: b0ac32c1 auto
// OFFSET: 0x005AADD0
void CGActionBar::RemoveMacroActions(uint32_t macroId) {
    if (!macroId) {
        return;
    }

    for (uint32_t slot = 0; slot < 144; slot++) {
        uint32_t action = CGActionBar::m_slotActions[slot];
        uint32_t id = action && (action & 0xF0000000) == 0x40000000 ? action & 0xBFFFFFFF : 0;

        if (id == macroId) {
            CGActionBar::RemoveAction(slot);
        }
    }
}

// VERIFY: bd659b06 manual
// OFFSET: 0x005AAE20
void CGActionBar::RemoveEquipmentSet(uint32_t id) {
    if (id > 9) {
        return;
    }

    for (uint32_t i = 0; i < 144; i++) {
        uint32_t action = CGActionBar::m_slotActions[i];
        uint32_t set = action && (action & 0xF0000000) == 0x20000000 ? action & 0xDFFFFFFF : static_cast<uint32_t>(-1);

        if (set == id) {
            CGActionBar::RemoveAction(i);
        }
    }
}

// VERIFY: 8e72a9c5 manual
// OFFSET: 0x005AAE80
void CGActionBar::SetAction(int32_t slot, uint32_t action, int32_t sendToServer, int32_t silent) {
    if (slot < 0 || slot >= sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0])) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || player->m_actionButtonsPending) {
        return;
    }

    if (silent && (action & 0xF0000000) == 0x80000000) {
        WOWGUID none = { 0 };
        g_itemDBCache.GetRecord(action & 0x7FFFFFFF, none, ItemStatsCallback, nullptr, true);
    }

    if (CGActionBar::IsMacro(slot)) {
        MACRONODE* macro = CGUIMacros::GetMacroByID(CGActionBar::GetMacroID(slot));

        if (macro) {
            macro->DecrementActionBarCount();
        }
    }

    if ((action & 0xF0000000) == 0x40000000) {
        MACRONODE* macro = CGUIMacros::GetMacroByID(action & 0xBFFFFFFF);

        if (macro) {
            macro->IncrementActionBarCount();
        }
    }

    CGActionBar::m_slotActions[slot] = action;
    int32_t count = 0;

    if (CGActionBar::GetItem(slot)) {
        count = CGActionBar::CalculateItemCount(CGActionBar::GetItem(slot));
    } else {
        int32_t petSpell;
        int32_t spellId = CGActionBar::GetSpell(slot, petSpell);

        if (spellId && !petSpell) {
            count = Spell_C_GetSpellReagentUses(spellId);
        }
    }

    CGActionBar::m_slotItemCount[slot] = count;
    CGActionBar::SlotChanged(slot, sendToServer, silent);
}

// VERIFY: 5c508967 manual
// OFFSET: 0x005AB120
void CGActionBar::PutActionInSlot(int32_t slot) {
    if (slot < 0 || slot >= sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0])) {
        return;
    }

    if (!CGGameUI::CanPerformAction(14)) {
        CGGameUI::ClearCursor(1, 1);
        return;
    }

    int32_t cursorSpell = CGGameUI::GetCursorSpell();
    int32_t itemId = CGGameUI::m_cursorItemType == UICURSOR_ACTIONBAR ? CGGameUI::GetCursorVirtualItem() : 0;
    CGItem_C* item = nullptr;
    int32_t cursorMacro = CGGameUI::m_cursorMacro;

    if (!itemId && CGGameUI::GetCursorItem()) {
        item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGGameUI::GetCursorItem(), TYPE_ITEM, __FILE__, __LINE__));

        if (item) {
            itemId = item->m_obj->m_entryID;
        }
    }

    int32_t cursorSet = CGGameUI::m_cursorEquipmentSet;
    SpellRec spell;
    int32_t haveSpell = 0;

    if (slot >= 132) {
        if (cursorSpell) {
            haveSpell = g_spellDB.GetRecord(cursorSpell, &spell);

            if (!haveSpell) {
                return;
            }
        }

        int32_t category = GetMultiCastTotemCategory(slot);

        if (!cursorSpell || !IsMultiCastSpellForSlot(slot, &spell, &category)) {
            TotemCategoryRec* totem = g_totemCategoryDB.GetRecord(category);

            if (totem) {
                CGGameUI::DisplayError(ERR_MULTI_CAST_ACTION_TOTEM_S, totem->m_name);
            }

            return;
        }
    }

    int32_t petSpell;
    uint32_t action;

    if (cursorSpell > 0) {
        if (!haveSpell && !g_spellDB.GetRecord(cursorSpell, &spell)) {
            return;
        }

        if (spell.m_attributes & 0x40) {
            CGGameUI::DisplayError(ERR_PASSIVE_ABILITY);
            return;
        }

        if (CGActionBar::IsItem(slot)) {
            int32_t actionItemId = CGActionBar::GetItem(slot);
            CGGameUI::SetCursorVirtualItem(actionItemId, CGItem_C::GetDisplayID(actionItemId), slot, UICURSOR_ACTIONBAR, 0);
        } else if (CGActionBar::IsSpell(slot)) {
            int32_t spellId = CGActionBar::GetSpell(slot, petSpell);

            if (spellId == cursorSpell) {
                CGGameUI::DropCursorSpell();
                return;
            }

            CGGameUI::SetCursorSpell(spellId);
        } else if (CGActionBar::IsMacro(slot) && CGActionBar::HasMacro(slot)) {
            CGGameUI::SetCursorMacro(CGActionBar::GetMacroID(slot));
        } else if (CGActionBar::IsEquipmentSet(slot) && CGActionBar::IsEquipmentSetValid(CGActionBar::GetEquipmentSetID(slot))) {
            int32_t setId = CGActionBar::GetEquipmentSetID(slot);

            if (cursorSet == setId) {
                CGGameUI::ClearCursor(1, 1);
                return;
            }

            CGGameUI::SetCursorEquipmentSet(setId);
        } else {
            CGGameUI::ClearCursor(1, 1);
        }

        action = cursorSpell;
    } else if (itemId > 0) {
        if (!ClntObjMgrGetActivePlayerObj()) {
            return;
        }

        uint32_t inventoryType = CGItem_C::GetInventoryType(itemId);

        if (!CGItem_C::GetUseSpell(itemId) && ((!item && !(item = CGActionBar::FindUseEnchantItemPtr(itemId))) || !item->GetEnchantmentSpell(0))) {
            if (!inventoryType || inventoryType == 18) {
                return;
            }
        }

        if (CGActionBar::IsItem(slot)) {
            int32_t slotItem = CGActionBar::GetItem(slot);

            if (itemId == slotItem) {
                CGGameUI::ClearCursor(1, 1);
                return;
            }

            CGGameUI::SetCursorVirtualItem(slotItem, CGItem_C::GetDisplayID(slotItem), slot, UICURSOR_ACTIONBAR, 0);
        } else if (CGActionBar::IsSpell(slot)) {
            CGGameUI::SetCursorSpell(CGActionBar::GetSpell(slot, petSpell));
        } else if (CGActionBar::IsMacro(slot) && CGActionBar::HasMacro(slot)) {
            CGGameUI::SetCursorMacro(CGActionBar::GetMacroID(slot));
        } else if (CGActionBar::IsEquipmentSet(slot) && CGActionBar::IsEquipmentSetValid(CGActionBar::GetEquipmentSetID(slot))) {
            int32_t setId = CGActionBar::GetEquipmentSetID(slot);

            if (cursorSet == setId) {
                CGGameUI::ClearCursor(1, 1);
                return;
            }

            CGGameUI::SetCursorEquipmentSet(setId);
        } else {
            CGGameUI::ClearCursor(1, 1);
        }

        action = itemId | 0x80000000;
    } else if (cursorMacro) {
        if (CGActionBar::IsItem(slot)) {
            int32_t actionItemId = CGActionBar::GetItem(slot);
            CGGameUI::SetCursorVirtualItem(actionItemId, CGItem_C::GetDisplayID(actionItemId), slot, UICURSOR_ACTIONBAR, 0);
        } else if (CGActionBar::IsSpell(slot)) {
            CGGameUI::SetCursorSpell(CGActionBar::GetSpell(slot, petSpell));
        } else if (CGActionBar::IsMacro(slot) && CGActionBar::HasMacro(slot)) {
            int32_t macroId = CGActionBar::GetMacroID(slot);

            if (cursorMacro == macroId) {
                CGGameUI::ClearCursor(1, 1);
                return;
            }

            CGGameUI::SetCursorMacro(macroId);
        } else if (CGActionBar::IsEquipmentSet(slot) && CGActionBar::IsEquipmentSetValid(CGActionBar::GetEquipmentSetID(slot))) {
            int32_t setId = CGActionBar::GetEquipmentSetID(slot);

            if (cursorSet == setId) {
                CGGameUI::ClearCursor(1, 1);
                return;
            }

            CGGameUI::SetCursorEquipmentSet(setId);
        } else {
            CGGameUI::ClearCursor(1, 1);
        }

        action = cursorMacro | 0x40000000;
    } else {
        if (cursorSet < 0) {
            return;
        }

        if (CGActionBar::IsItem(slot)) {
            int32_t actionItemId = CGActionBar::GetItem(slot);
            CGGameUI::SetCursorVirtualItem(actionItemId, CGItem_C::GetDisplayID(actionItemId), slot, UICURSOR_ACTIONBAR, 0);
        } else if (CGActionBar::IsSpell(slot)) {
            CGGameUI::SetCursorSpell(CGActionBar::GetSpell(slot, petSpell));
        } else if (CGActionBar::IsMacro(slot) && CGActionBar::HasMacro(slot)) {
            int32_t macroId = CGActionBar::GetMacroID(slot);

            if (cursorMacro == macroId) {
                CGGameUI::ClearCursor(1, 1);
                return;
            }

            CGGameUI::SetCursorMacro(macroId);
        } else if (CGActionBar::IsEquipmentSet(slot) && CGActionBar::IsEquipmentSetValid(CGActionBar::GetEquipmentSetID(slot))) {
            int32_t setId = CGActionBar::GetEquipmentSetID(slot);

            if (cursorSet == setId) {
                CGGameUI::ClearCursor(1, 1);
                return;
            }

            CGGameUI::SetCursorEquipmentSet(setId);
        } else {
            CGGameUI::ClearCursor(1, 1);
        }

        action = cursorSet | 0x20000000;
    }

    CGActionBar::SetAction(slot, action, 1, 0);
}

// VERIFY: 2e4eae48 manual
// OFFSET: 0x005AB9D0
void CGActionBar::UpdateMultiCastBar() {
    if (s_cvAutoFilledMultiCastSlots) {
        int32_t filled = s_cvAutoFilledMultiCastSlots->m_intValue;
        int32_t newFilled = filled;

        for (uint32_t i = 0; i < sizeof(CGSpellBook::m_totemSpells) / sizeof(CGSpellBook::m_totemSpells[0]); i++) {
            int32_t bit = 1 << i;
            TSGrowableArray<int32_t>* totems = i < sizeof(CGSpellBook::m_totemSpells) / sizeof(CGSpellBook::m_totemSpells[0]) ? &CGSpellBook::m_totemSpells[i] : nullptr;

            if (filled & bit) {
                if (!totems || !totems->m_count) {
                    newFilled &= ~bit;
                }
            } else if (totems && totems->m_count) {
                int32_t spellId = totems->m_data[0];

                for (int32_t slot = i + 132; slot < static_cast<int32_t>(sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0])); slot += 4) {
                    if (!CGActionBar::HasAction(slot)) {
                        CGActionBar::SetAction(slot, spellId, 1, 1);
                    }
                }

                newFilled |= bit;
            }
        }

        if (newFilled != filled) {
            s_cvAutoFilledMultiCastSlots->Set(newFilled, true, false, false, true);
        }
    }

    Script_SendUniqueSignal(SCRIPT_EVENT_UPDATE_MULTI_CAST_ACTIONBAR);
}

// VERIFY: d16e1aa6 manual
// OFFSET: 0x005ABBC0
void CGActionBar::UseAction(int32_t slot, const WOWGUID& target, const char* button) {
    if (slot < 0 || slot >= sizeof(CGActionBar::m_slotActions) / sizeof(CGActionBar::m_slotActions[0])) {
        return;
    }

    if (!CGUIBindings::s_bindings->m_keyStateOverride && (slot < 120 || slot >= 132) && (CGGameUI::GetCursorSpell() || CGGameUI::GetCursorItem() || CGGameUI::m_cursorMacro || CGGameUI::GetCursorVirtualItemOfType(UICURSOR_ACTIONBAR) || CGGameUI::m_cursorEquipmentSet >= 0)) {
        CGActionBar::PutActionInSlot(slot);
        return;
    }

    uint32_t action = CGActionBar::m_slotActions[slot];

    if (!action) {
        return;
    }

    if (CGActionBar::IsItem(slot)) {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (!player) {
            return;
        }

        int32_t itemId = CGActionBar::GetItem(slot);
        uint32_t inventoryType = CGItem_C::GetInventoryType(itemId);

        CGItem_C* enchantItem = CGActionBar::FindUseEnchantItemPtr(itemId);
        int32_t charges = enchantItem ? enchantItem->GetUsableSpellCharges() : CGItem_C::NumBaseCharges(itemId);
        CGItem_C* item;

        if (inventoryType) {
            uint32_t flags = charges != -1 && charges ? 161 : 129;
            item = player->m_bag.FindItemOfType(itemId, flags);

            if (!item) {
                WOWGUID container;
                uint32_t itemSlot = 0;

                if ((((flags & 0x20) && (item = player->m_bag.FindItemOfType(itemId, container, itemSlot, 32)) != nullptr) || (item = player->m_bag.FindItemOfType(itemId, container, itemSlot, 128)) != nullptr) && !(item->m_itemFlags & 0x1)) {
                    player->AutoEquipItem(container, itemSlot, 0);
                }

                return;
            }
        } else {
            uint32_t flags = charges != -1 && charges ? 288 : 256;
            item = player->m_bag.FindItemOfType(itemId, flags);

            if (!item && (flags & 0x20)) {
                item = player->m_bag.FindItemOfType(itemId, flags & ~0x20);
            }

            if (!item) {
                return;
            }
        }

        item->UseItem(const_cast<WOWGUID*>(&target), 0);
    } else if (CGActionBar::IsMacro(slot)) {
        CGUIMacros::RunMacro(CGActionBar::GetMacroID(slot), button);
    } else if (CGActionBar::IsEquipmentSet(slot)) {
        CGEquipmentManager::EquipSet(CGActionBar::GetEquipmentSetID(slot));
    } else if (CGActionBar::IsPetAction(slot)) {
        PetAction* petAction = GetPetAction(action & 0xEFFFFFFF);

        if (petAction && petAction->m_data) {
            WOWGUID none = { 0 };
            CGPetInfo::SendPetAction(*petAction, none, 0);
            SI2::PlaySoundKit("GAMEABILITYACTIVATE", nullptr, nullptr, nullptr);
            CGActionBar::UpdateSelection();
        }
    } else {
        int32_t petSpell;
        Spell_C_ClickSpell(CGActionBar::GetSpell(slot, petSpell), nullptr, target);
    }
}

// VERIFY: cb09cbdf auto
// OFFSET: 0x005A7C50
static int32_t FormHasActionBar(const SpellShapeshiftFormRec* form) {
    if (!form) {
        return 0;
    }

    for (uint32_t i = 0; i < sizeof(form->m_presetSpellID) / sizeof(form->m_presetSpellID[0]); i++) {
        if (form->m_presetSpellID[i]) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 1f6e9776 manual
// OFFSET: 0x005A83C0
void CGActionBar::UpdateBonusBar() {
    int32_t offset = 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        if (CGActionBar::m_tempPageActiveFlags) {
            offset = 5;
        } else {
            SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(player->GetShapeshiftForm());

            if (form) {
                offset = 5;

                if (!FormHasActionBar(form)) {
                    offset = form->m_bonusActionBar;
                }
            }
        }
    }

    if (offset != CGActionBar::m_bonusBarOffset) {
        int32_t wasTemp = CGActionBar::m_bonusBarOffset == 5;
        CGActionBar::m_bonusBarOffset = offset;

        if (wasTemp) {
            FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_PAGE_CHANGED, nullptr);
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_BONUS_ACTIONBAR, nullptr);
    }
}

// VERIFY: de5be03c auto
// OFFSET: 0x005AC0E0
void CGActionBar::EnterWorld() {
    CGActionBar::UpdateUsable();
    CGActionBar::UpdateShapeShiftBar();
    CGActionBar::UpdateOverrideSpellBar();
    CGActionBar::UpdateBonusBar();
}

// VERIFY: a12b35d4 auto
// OFFSET: 0x005A79D0
void CGActionBar::InitializeGame() {
    memset(CGActionBar::m_slotActions, 0, sizeof(CGActionBar::m_slotActions));
    memset(CGActionBar::m_slotUsable, 0, sizeof(CGActionBar::m_slotUsable));
    memset(CGActionBar::m_slotItemCount, 0, sizeof(CGActionBar::m_slotItemCount));
    memset(CGActionBar::m_slotNoMana, 0, sizeof(CGActionBar::m_slotNoMana));
    CGActionBar::m_currentPage = 0;
    CGActionBar::m_bonusBarOffset = 0;
    CGActionBar::m_tempPageActiveFlags = 0;
}

// VERIFY: 8aaa3eee manual
// OFFSET: 0x005AA4E0
void CGActionBar::UpdateAttackAction() {
    for (uint32_t i = 0; i < 144; i++) {
        if (CGActionBar::IsAttackAction(i)) {
            int32_t noMana;
            CGActionBar::m_slotUsable[i] = CGActionBar::UpdateUsableAction(i, noMana);
            CGActionBar::m_slotNoMana[i] = noMana;
            FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_SLOT_CHANGED, "%d", i + 1);
        }
    }
}

// VERIFY: 360c7e77 manual
// OFFSET: 0x005AA550
void CGActionBar::UpdateRangedAttackAction() {
    for (uint32_t i = 0; i < 144; i++) {
        if (CGActionBar::IsRangedAttackAction(i)) {
            int32_t noMana;
            CGActionBar::m_slotUsable[i] = CGActionBar::UpdateUsableAction(i, noMana);
            CGActionBar::m_slotNoMana[i] = noMana;
            FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_SLOT_CHANGED, "%d", i + 1);
        }
    }
}

// VERIFY: 7f087db2 manual
// OFFSET: 0x005A8470
int32_t IsStackableAucItem(uint32_t slot) {
    if (slot > 143) {
        return 0;
    }

    WOWGUID requester = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(CGActionBar::GetItem(slot), requester, nullptr, nullptr, false);
    return stats && stats->m_stackable > 1;
}

// VERIFY: e7c2b372 manual
// OFFSET: 0x005A88B0
int32_t CGActionBar::IsEquippedAction(uint32_t slot) {
    if (slot > 143) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    int32_t itemId = CGActionBar::GetItem(slot);

    if (!CGItem_C::GetInventoryType(itemId)) {
        return 0;
    }

    CGItem_C* enchantItem = CGActionBar::FindUseEnchantItemPtr(itemId);
    int32_t charges = enchantItem ? enchantItem->GetUsableSpellCharges() : CGItem_C::NumBaseCharges(itemId);
    uint32_t flags = charges != -1 && charges ? 161 : 129;
    CGBag_C* bag = &player->m_bag;
    return bag->FindItemOfType(itemId, flags) || ((flags & 0x20) && !bag->FindItemOfType(itemId, 32) && bag->FindItemOfType(itemId, flags & ~0x20));
}

// VERIFY: 20f78ef5 manual
// OFFSET: 0x005A9360
int32_t CGActionBar::IsConsumableAction(uint32_t slot) {
    if (slot > 143) {
        return 0;
    }

    int32_t itemId = CGActionBar::GetItem(slot);
    WOWGUID requester = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, requester, nullptr, nullptr, false);

    if (stats) {
        if (stats->m_inventoryType == 24 || stats->m_inventoryType == 25) {
            return 1;
        }

        CGItem_C* enchantItem = CGActionBar::FindUseEnchantItemPtr(itemId);
        int32_t charges = enchantItem ? enchantItem->GetUsableSpellCharges() : CGItem_C::NumBaseCharges(itemId);

        if (charges < 0) {
            return 1;
        }
    }

    SpellRec spell;
    int32_t petSpell;

    if (!g_spellDB.GetRecord(CGActionBar::GetSpell(slot, petSpell), &spell) || petSpell) {
        return 0;
    }

    for (int32_t i = 0; i < 8; i++) {
        if (spell.m_reagent[i] && spell.m_reagentCount[i] > 0) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 167e06af auto
// OFFSET: 0x005A7CC0
void CGActionBar::SignalUpdateCooldown() {
    FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_UPDATE_COOLDOWN, nullptr);
}

// VERIFY: 243b49e9 manual
// OFFSET: 0x005AAB10
void CGActionBar::OnAuraChanged(int32_t spellId) {
    if (!spellId) {
        return;
    }

    for (uint32_t i = 0; i < 144; i++) {
        int32_t petSpell;

        if (CGActionBar::GetSpell(i, petSpell) == spellId && !petSpell) {
            CGActionBar::SlotChanged(i, 0, 0);
        }
    }
}

// VERIFY: 8f9b1721 manual
// OFFSET: 0x005A7920
uint32_t CGActionBar::GetSlotSpellAction(uint32_t slot) {
    uint32_t action = CGActionBar::m_slotActions[slot];

    if (action && (action & 0xF0000000) == 0x10000000) {
        return action & 0xEFFFFFFF;
    }

    return 0;
}

// VERIFY: eaef0f5d auto
// OFFSET: 0x005A7C80
static int32_t HasOverrideSpellData(const OverrideSpellDataRec* rec) {
    if (!rec) {
        return 0;
    }

    for (uint32_t i = 0; i < 10; i++) {
        if (rec->m_spells[i]) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 5c0eac99 auto
// OFFSET: 0x005A7CF0
static int32_t GetOverrideSpell(uint32_t index, const void* data) {
    if (data && index < 10) {
        return static_cast<const OverrideSpellDataRec*>(data)->m_spells[index];
    }

    return 0;
}

// VERIFY: 4f453604 manual
// OFFSET: 0x005ABB40
void CGActionBar::UpdateOverrideSpellBar() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    OverrideSpellDataRec* rec;

    if (player && (rec = g_overrideSpellDataDB.GetRecord(static_cast<int16_t>(player->m_player->m_playerFieldBytes2))) && HasOverrideSpellData(rec)) {
        CGActionBar::UpdateTemporaryActionBar(&GetOverrideSpell, rec);
        CGActionBar::m_tempPageActiveFlags |= 0x4;
    } else {
        CGActionBar::m_tempPageActiveFlags &= ~0x4;
    }
}

// VERIFY: a5c3e3b2 auto
// OFFSET: 0x005AAB90
void CGActionBar::RemoveMacro(int32_t spellId) {
    for (uint32_t i = 0; i < 144; i++) {
        int32_t petSpell;

        if (CGActionBar::GetSpell(i, petSpell) != spellId) {
            continue;
        }

        uint32_t action = CGActionBar::m_slotActions[i];

        if (action && (action & 0xF0000000) == 0x40000000) {
            CGUIMacros::GetMacroByID(action & 0xBFFFFFFF)->FindSpellID();
        }

        CGActionBar::RemoveAction(i);
    }
}

// VERIFY: 751f1f9d manual
// OFFSET: 0x005AAA00
void CGActionBar::UpdateEquipmentSet(uint32_t id) {
    if (id > 9) {
        return;
    }

    for (uint32_t i = 0; i < 144; i++) {
        uint32_t action = CGActionBar::m_slotActions[i];
        uint32_t set = action && (action & 0xF0000000) == 0x20000000 ? action & 0xDFFFFFFF : static_cast<uint32_t>(-1);

        if (set == id) {
            int32_t noMana;
            CGActionBar::m_slotUsable[i] = CGActionBar::UpdateUsableAction(i, noMana);
            CGActionBar::m_slotNoMana[i] = noMana;
            FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_SLOT_CHANGED, "%d", i + 1);
        }
    }
}

// VERIFY: d191a5f1 manual
// OFFSET: 0x005AAA90
void CGActionBar::RemoveAction(uint32_t slot) {
    if (static_cast<int32_t>(slot) < 0 || slot >= 144) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || player->m_actionButtonsPending) {
        return;
    }

    if (CGActionBar::IsMacro(slot)) {
        MACRONODE* macro = CGUIMacros::GetMacroByID(CGActionBar::GetMacroID(slot));

        if (macro) {
            macro->DecrementActionBarCount();
        }
    }

    CGActionBar::m_slotActions[slot] = 0;
    CGActionBar::SlotChanged(slot, 1, 0);
}

// VERIFY: 9df78218 auto
// OFFSET: 0x005AA920
void CGActionBar::UpdateMacro(uint32_t macroId) {
    if (!macroId) {
        return;
    }

    for (uint32_t i = 0; i < 144; i++) {
        uint32_t action = CGActionBar::m_slotActions[i];
        uint32_t id = action && (action & 0xF0000000) == 0x40000000 ? action & 0xBFFFFFFF : 0;

        if (id != macroId) {
            continue;
        }

        int32_t count = 0;

        if (CGActionBar::GetItem(i)) {
            count = CGActionBar::CalculateItemCount(CGActionBar::GetItem(i));
        } else {
            int32_t petSpell;
            int32_t spellId = CGActionBar::GetSpell(i, petSpell);

            if (spellId && !petSpell) {
                count = Spell_C_GetSpellReagentUses(spellId);
            }
        }

        CGActionBar::m_slotItemCount[i] = count;
        int32_t noMana;
        CGActionBar::m_slotUsable[i] = CGActionBar::UpdateUsableAction(i, noMana);
        CGActionBar::m_slotNoMana[i] = noMana;
        FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_SLOT_CHANGED, "%d", i + 1);
    }
}

// VERIFY: c1c240db auto
// OFFSET: 0x005A7C20
void CGActionBar::SetTemporaryActionBarFlags(uint32_t flags, int32_t enable) {
    if (enable) {
        CGActionBar::m_tempPageActiveFlags |= flags;
    } else {
        CGActionBar::m_tempPageActiveFlags &= ~flags;
    }
}

// VERIFY: 990974c3 auto
// OFFSET: 0x005AB800
void CGActionBar::UpdateTemporaryActionBar(int32_t (*getSpell)(uint32_t index, const void* data), const void* data) {
    if (!getSpell) {
        return;
    }

    for (uint32_t i = 0; i < 12; i++) {
        CGActionBar::SetAction(i + 120, getSpell(i, data), 0, 0);
    }
}

// VERIFY: c3611dc4 manual
// OFFSET: 0x005A8D30
void CGActionBar::UpdateUsablePower() {
    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    SpellRec spell;
    int32_t changed = 0;

    for (uint32_t i = 0; i < 144; i++) {
        if (!CGActionBar::m_slotUsable[i]) {
            continue;
        }

        int32_t noMana = 0;
        int32_t petSpell;
        int32_t spellId = CGActionBar::GetSpell(i, petSpell);

        if (spellId && g_spellDB.GetRecord(spellId, &spell)) {
            CGUnit_C* caster = petSpell ? static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__)) : player;

            if (caster) {
                noMana = !Spell_C_HaveSpellPower(caster, &spell);
            }
        }

        if (noMana != CGActionBar::m_slotNoMana[i]) {
            CGActionBar::m_slotNoMana[i] = noMana;
            changed = 1;
        }
    }

    if (changed) {
        FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_UPDATE_USABLE, nullptr);
    }
}

// VERIFY: 6a47ffb3 auto
// OFFSET: 0x005A7CD0
static int32_t GetFormPresetSpell(uint32_t index, const void* data) {
    const SpellShapeshiftFormRec* form = static_cast<const SpellShapeshiftFormRec*>(data);

    if (form && index < 8) {
        return form->m_presetSpellID[index];
    }

    return 0;
}

// VERIFY: 82a1fae7 manual
// OFFSET: 0x005ABAC0
void CGActionBar::UpdateShapeShiftBar() {
    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    SpellShapeshiftFormRec* form;

    if (player && (form = g_spellShapeshiftFormDB.GetRecord(player->GetShapeshiftForm())) && FormHasActionBar(form)) {
        CGActionBar::UpdateTemporaryActionBar(GetFormPresetSpell, form);
        CGActionBar::m_tempPageActiveFlags |= 2;
    } else {
        CGActionBar::m_tempPageActiveFlags &= ~2u;
    }
}

// VERIFY: 3645420e manual
// OFFSET: 0x005A9470
int32_t CGActionBar::IsAutoRepeatAction(uint32_t slot) {
    if (slot > 143) {
        return 0;
    }

    int32_t petSpell;
    int32_t spellId = CGActionBar::GetSpell(slot, petSpell);

    return spellId && !petSpell && (Spell_C_GetAutoRepeatingSpell() == spellId || Spell_C_GetAutoRangedCombatSpell() == spellId);
}

// VERIFY: 7fa85525 manual
// OFFSET: 0x005A94C0
int32_t CGActionBar::HasValidTarget(uint32_t slot, WOWGUID target, int32_t* inRange) {
    if (slot > 143) {
        return 0;
    }

    SpellRec spell;
    int32_t petSpell;

    if (!g_spellDB.GetRecord(CGActionBar::GetSpell(slot, petSpell), &spell)) {
        return 0;
    }

    if (!target && CGActionBar::IsMacro(slot)) {
        MACRONODE* macro = CGUIMacros::GetMacroByID(CGActionBar::GetMacroID(slot));

        if (macro) {
            target = macro->m_target;
        }
    }

    CGUnit_C* caster = petSpell ? static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__)) : ClntObjMgrGetActivePlayerObj();
    uint8_t result;

    if (caster && Spell_C_RangeCheckSelected(caster, &spell, target, &result, false, nullptr)) {
        *inRange = result;
        return 1;
    }

    return 0;
}

// VERIFY: 5e49e6d0 manual
// OFFSET: 0x005AA5C0
void CGActionBar::UpdateItem(int32_t itemId) {
    if (!itemId) {
        return;
    }

    int32_t count = CGActionBar::CalculateItemCount(itemId);

    for (uint32_t i = 0; i < 144; i++) {
        uint32_t action = CGActionBar::m_slotActions[i];

        if (action && (action & 0xF0000000) == 0x40000000) {
            MACRONODE* macro = CGUIMacros::GetMacroByID(action & 0xBFFFFFFF);

            if (macro && macro->m_spellID < 0) {
                macro->FindSpellID();
            }
        }

        int32_t item = CGActionBar::GetItem(i);

        if (!item) {
            int32_t petSpell;
            int32_t spellId = CGActionBar::GetSpell(i, petSpell);

            if (!spellId || petSpell) {
                continue;
            }

            SpellRec spell;

            if (g_spellDB.GetRecord(spellId, &spell) && Spell_C_SpellUsesReagent(&spell, itemId)) {
                CGActionBar::m_slotItemCount[i] = Spell_C_GetSpellReagentUses(&spell);
                int32_t noMana;
                CGActionBar::m_slotUsable[i] = CGActionBar::UpdateUsableAction(i, noMana);
                CGActionBar::m_slotNoMana[i] = noMana;
                FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_SLOT_CHANGED, "%d", i + 1);
            }

            continue;
        }

        if (item == itemId) {
            if (!count) {
                WOWGUID none = WOWGUID();
                ItemStats_C* stats = g_itemDBCache.GetRecord(item, none, nullptr, nullptr, false);

                if (stats) {
                    uint32_t index = 0;

                    while (stats->m_spellTrigger[index] != 6) {
                        if (++index >= 5) {
                            break;
                        }
                    }

                    if (index < 5) {
                        uint32_t spellId = stats->m_spellId[index];

                        if (spellId && spellId <= static_cast<uint32_t>(g_spellDB.m_maxID) && (s_knownSpellBits.m_bits.m_data[spellId >> 5] & (1 << (spellId & 0x1F)))) {
                            CGActionBar::SetAction(i, spellId, 1, 0);
                            continue;
                        }
                    }
                }
            }

            CGActionBar::m_slotItemCount[i] = count;
            int32_t noMana;
            CGActionBar::m_slotUsable[i] = CGActionBar::UpdateUsableAction(i, noMana);
            CGActionBar::m_slotNoMana[i] = noMana;
            FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_SLOT_CHANGED, "%d", i + 1);
            continue;
        }

        WOWGUID none = WOWGUID();
        ItemStats_C* stats = g_itemDBCache.GetRecord(item, none, nullptr, nullptr, false);

        if (!stats || stats->m_stackable > 1) {
            continue;
        }

        CGItem_C* enchantItem = CGActionBar::FindUseEnchantItemPtr(item);
        int32_t charges = enchantItem ? enchantItem->GetUsableSpellCharges() : CGItem_C::NumBaseCharges(item);

        if (charges < 0) {
            continue;
        }

        int32_t spellId = enchantItem ? enchantItem->GetEnchantmentSpell(0) : CGItem_C::GetUseSpell(item);

        if (!spellId) {
            continue;
        }

        SpellRec spell;

        if (!g_spellDB.GetRecord(spellId, &spell)) {
            continue;
        }

        uint32_t reagent = 0;

        while (!spell.m_reagent[reagent] || spell.m_reagentCount[reagent] <= 0) {
            if (++reagent >= 8) {
                break;
            }
        }

        if (reagent < 8 && Spell_C_SpellUsesReagent(&spell, itemId)) {
            CGActionBar::m_slotItemCount[i] = Spell_C_GetSpellReagentUses(&spell);
            int32_t noMana;
            CGActionBar::m_slotUsable[i] = CGActionBar::UpdateUsableAction(i, noMana);
            CGActionBar::m_slotNoMana[i] = noMana;
            FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_SLOT_CHANGED, "%d", i + 1);
        }
    }
}

// VERIFY: b69420a0 manual
// OFFSET: 0x005AE4F0
bool CGActionBar::IsEquipmentSetValid(int32_t setId) {
    for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); node; node = CGEquipmentManager::s_equipmentSets.Next(node)) {
        if (node->m_set->m_id == setId) {
            return true;
        }
    }

    return false;
}

// VERIFY: 82e513fa manual
// OFFSET: 0x005ABE70
void CGActionBar::PickupAction(uint32_t slot) {
    if (slot > 143 || !CGGameUI::CanPerformAction(14) || (slot >= 120 && slot < 132)) {
        return;
    }

    if (CGGameUI::GetCursorSpell() || CGGameUI::GetCursorItem() || CGGameUI::m_cursorMacro || CGGameUI::GetCursorVirtualItemOfType(UICURSOR_ACTIONBAR) || CGGameUI::m_cursorEquipmentSet >= 0) {
        CGActionBar::PutActionInSlot(slot);
        return;
    }

    if (!CGActionBar::m_slotActions[slot]) {
        return;
    }

    if (CGActionBar::IsItem(slot)) {
        int32_t itemId = CGActionBar::GetItem(slot);
        CGGameUI::SetCursorVirtualItem(itemId, CGItem_C::GetDisplayID(itemId), slot, UICURSOR_ACTIONBAR, 0);
        CGActionBar::RemoveAction(slot);
    } else if (CGActionBar::IsSpell(slot)) {
        int32_t petSpell;
        int32_t spell = CGActionBar::GetSpell(slot, petSpell);
        CGGameUI::SetCursorSpell(spell);
        CGActionBar::RemoveAction(slot);
    } else if (CGActionBar::IsMacro(slot) && CGActionBar::HasMacro(slot)) {
        CGGameUI::SetCursorMacro(CGActionBar::GetMacroID(slot));
        CGActionBar::RemoveAction(slot);
    } else {
        if (CGActionBar::IsEquipmentSet(slot) && CGActionBar::IsEquipmentSetValid(CGActionBar::GetEquipmentSetID(slot))) {
            CGGameUI::SetCursorEquipmentSet(CGActionBar::GetEquipmentSetID(slot));
        }

        CGActionBar::RemoveAction(slot);
    }
}

// VERIFY: 01a1fb58 auto
// OFFSET: 0x005EEB70
void CGActionBar::ShutdownGame() {
}

// VERIFY: 58205ae6 auto
// OFFSET: 0x005A7A90
int32_t CGActionBar::GetMacroItem(uint32_t slot) {
    uint32_t action = CGActionBar::m_slotActions[slot];
    uint32_t macroId = (action && (action & 0xF0000000) == 0x40000000) ? action & 0xBFFFFFFF : 0;
    MACRONODE* macro = CGUIMacros::GetMacroByID(macroId);

    if (macro) {
        return macro->m_itemID;
    }

    return 0;
}

// VERIFY: 8c44577f auto
// OFFSET: 0x005AAFD0
void CGActionBar::ReplaceSpell(int32_t oldSpell, int32_t newSpell) {
    SpellRec newRec;
    int32_t found = g_spellDB.GetRecord(newSpell, &newRec);
    int32_t newRank = found ? CGSpellBook::GetSpellProgressionRank(&newRec) : 0;

    for (uint32_t slot = 0; slot < 144; slot++) {
        int32_t petSpell;
        int32_t spellId = CGActionBar::GetSpell(slot, petSpell);

        if (!spellId) {
            continue;
        }

        if (spellId != oldSpell) {
            bool replace = false;
            SpellRec rec;

            if (found && g_spellDB.GetRecord(spellId, &rec) && !SStrCmp(rec.m_name, newRec.m_name, STORM_MAX_STR) && newRank == CGSpellBook::GetSpellProgressionRank(&rec) + 1) {
                replace = true;
            }

            if (!replace) {
                continue;
            }
        }

        uint32_t action = CGActionBar::m_slotActions[slot];

        if (!action) {
            continue;
        }

        if (!(action & 0xF0000000)) {
            CGActionBar::SetAction(slot, newSpell, 1, 0);
        } else if ((action & 0xF0000000) == 0x40000000) {
            CGUIMacros::GetMacroByID(action & 0xBFFFFFFF)->m_spellID = newSpell;
        }
    }
}

// VERIFY: c90d9c09 auto
// OFFSET: 0x005AAC00
void CGActionBar::UpdateCritter(int32_t critter) {
    if (!critter) {
        return;
    }

    SpellRec spell;

    for (uint32_t slot = 0; slot < 144; slot++) {
        int32_t petSpell;

        if (!g_spellDB.GetRecord(CGActionBar::GetSpell(slot, petSpell), &spell)) {
            continue;
        }

        for (uint32_t i = 0; i < 3; i++) {
            if (spell.m_effect[i] == SPELL_EFFECT_SUMMON && g_summonPropertiesDB.GetRecord(spell.m_effectMiscValueB[0]) && spell.m_effectMiscValue[i] == critter) {
                int32_t noMana;
                CGActionBar::m_slotUsable[slot] = CGActionBar::UpdateUsableAction(slot, noMana);
                CGActionBar::m_slotNoMana[slot] = noMana;
                FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_SLOT_CHANGED, "%d", slot + 1);
                break;
            }
        }
    }
}

// VERIFY: 8b8940d1 auto
// OFFSET: 0x005EEB70
void CGActionBar::LeaveWorld() {
}
