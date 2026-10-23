#include "CurrencyTypes.h"
#include <cstdlib>
#include <cstring>
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/CurrencyCategoryRec.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/CurrencyTypesRec.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"

// STATIC: 0x00C1F44C
int32_t CGCurrencyTypes::m_listSize;

// STATIC: 0x00C1E960
CurrencyListEntry CGCurrencyTypes::m_list[95];

// STATIC: 0x00C1F248
CurrencyCategoryInfo CGCurrencyTypes::m_categories[32];

// STATIC: 0x00C1F448
int32_t CGCurrencyTypes::m_numCategories;

// VERIFY: 74b83f99 auto
// OFFSET: 0x005AFC40
int32_t CGCurrencyTypes::CurrencyCategorySortCallback(const void* a, const void* b) {
    const CurrencyCategoryInfo* categoryA = static_cast<const CurrencyCategoryInfo*>(a);
    const CurrencyCategoryInfo* categoryB = static_cast<const CurrencyCategoryInfo*>(b);

    if (!categoryA || !categoryB) {
        return 0;
    }

    int32_t result = (categoryA->m_flags & 1) - (categoryB->m_flags & 1);

    if (!result) {
        return SStrSortUTF8I(categoryA->m_name, categoryB->m_name, 0x7FFFFFFF);
    }

    return result;
}

// VERIFY: a269ad8c manual
// OFFSET: 0x005AFC80
CurrencyListEntry* CGCurrencyTypes::GetDisplayListInfo(int32_t index) {
    if (index < 0 || index >= CGCurrencyTypes::m_listSize) {
        return nullptr;
    }

    return &CGCurrencyTypes::m_list[index];
}

// VERIFY: 32676bed manual
// OFFSET: 0x005AFCB0
void CGCurrencyTypes::SetCategoryExpand(int32_t index, int32_t expand) {
    if (index < 0 || index >= CGCurrencyTypes::m_listSize || !CGCurrencyTypes::m_list[index].m_isHeader) {
        return;
    }

    for (int32_t i = 0; i < CGCurrencyTypes::m_numCategories; i++) {
        if (CGCurrencyTypes::m_categories[i].m_id == CGCurrencyTypes::m_list[index].m_categoryID) {
            CGCurrencyTypes::m_categories[i].m_expanded = expand;
            return;
        }
    }
}

// VERIFY: 7bd12f82 manual
// OFFSET: 0x005AFD90
void CGCurrencyTypes::InitializeGame() {
    int32_t numCategories = g_currencyCategoryDB.m_numRecords;
    CGCurrencyTypes::m_numCategories = g_currencyCategoryDB.m_numRecords;
    memset(CGCurrencyTypes::m_categories, 0, sizeof(CGCurrencyTypes::m_categories));

    for (int32_t i = 0; i < numCategories; i++) {
        CurrencyCategoryRec* rec = i < 0 || i >= numCategories ? nullptr : &g_currencyCategoryDB.m_records[i];

        CGCurrencyTypes::m_categories[i].m_id = rec->m_id;
        CGCurrencyTypes::m_categories[i].m_name = rec->m_name;
        CGCurrencyTypes::m_categories[i].m_flags = rec->m_flags;
        CGCurrencyTypes::m_categories[i].m_expanded = 1;
    }

    qsort(CGCurrencyTypes::m_categories, numCategories, sizeof(CurrencyCategoryInfo), CGCurrencyTypes::CurrencyCategorySortCallback);
    CGCurrencyTypes::m_listSize = 0;
    memset(CGCurrencyTypes::m_list, 0, sizeof(CGCurrencyTypes::m_list));
}

// VERIFY: cd06e9a4 manual
// OFFSET: 0x005AFE30
int32_t CGCurrencyTypes::CategoryHasKnownTypes(int32_t index, uint32_t known1, uint32_t known2, uint32_t unused1, uint32_t unused2) {
    for (int32_t i = 0; i < g_currencyTypesDB.m_numRecords; i++) {
        CurrencyTypesRec* rec = g_currencyTypesDB.GetRecordByIndex(i);

        if (!rec || (CGCurrencyTypes::m_categories[index].m_id != rec->m_categoryID && index != CGCurrencyTypes::m_numCategories - 1)) {
            continue;
        }

        uint32_t known = 0;
        uint32_t unused = 0;
        uint32_t mask = 0;

        if (rec->m_bitIndex <= 32) {
            if (rec->m_bitIndex > 0) {
                known = known1;
                unused = unused1;
                mask = 1 << (rec->m_bitIndex - 1);
            }
        } else {
            known = known2;
            unused = unused2;
            mask = 1 << (rec->m_bitIndex - 33);
        }

        if ((CGCurrencyTypes::m_categories[index].m_id == rec->m_categoryID && (mask & known)) || (index == CGCurrencyTypes::m_numCategories - 1 && (mask & unused))) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: ef8fc97b manual
// OFFSET: 0x005AFF10
int32_t CGCurrencyTypes::DisplayListSortCallback(const void* a, const void* b) {
    const CurrencyListEntry* entryA = static_cast<const CurrencyListEntry*>(a);
    const CurrencyListEntry* entryB = static_cast<const CurrencyListEntry*>(b);

    if (!entryA || !entryB) {
        return 0;
    }

    ItemStats_C* statsA = g_itemDBCache.GetRecord(entryA->m_itemID, WOWGUID{ 0 }, nullptr, nullptr, false);
    ItemStats_C* statsB = g_itemDBCache.GetRecord(entryB->m_itemID, WOWGUID{ 0 }, nullptr, nullptr, false);

    if (!statsA) {
        return statsB != nullptr;
    }

    if (!statsB) {
        return -1;
    }

    const char* nameA = statsA->GetName(0);
    const char* nameB = statsB->GetName(0);

    if (!nameA) {
        return nameB != nullptr;
    }

    if (!nameB) {
        return -1;
    }

    return SStrSortUTF8I(nameA, nameB, STORM_MAX_STR);
}

// VERIFY: f01a5b7c manual
// OFFSET: 0x005B0030
void CGCurrencyTypes::BuildDisplayList() {
    CGCurrencyTypes::m_listSize = 0;
    memset(CGCurrencyTypes::m_list, 0, sizeof(CGCurrencyTypes::m_list));
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    uint64_t knownCurrencies = static_cast<CGPlayerLocalData*>(player->m_player)->m_playerFieldKnownCurrencies.m_guid;
    uint32_t known1 = static_cast<uint32_t>(knownCurrencies);
    uint32_t known2 = static_cast<uint32_t>(knownCurrencies >> 32);
    uint32_t unused1 = g_currencyTokensUnused1CVar->m_intValue;
    uint32_t unused2 = g_currencyTokensUnused2CVar->m_intValue;
    uint32_t backpack1 = g_currencyTokensBackpack1CVar->m_intValue;
    uint32_t backpack2 = g_currencyTokensBackpack2CVar->m_intValue;
    auto callback = reinterpret_cast<void (*)(int32_t, const WOWGUID&, void*, bool)>(&CGCurrencyTypes::EnterWorld);

    for (int32_t category = 0; category < CGCurrencyTypes::m_numCategories; category++) {
        if (!CGCurrencyTypes::CategoryHasKnownTypes(category, known1, known2, unused1, unused2)) {
            continue;
        }

        CurrencyListEntry* header = &CGCurrencyTypes::m_list[CGCurrencyTypes::m_listSize++];
        header->m_name = CGCurrencyTypes::m_categories[category].m_name;
        header->m_isHeader = 1;
        header->m_isExpanded = CGCurrencyTypes::m_categories[category].m_expanded;
        header->m_categoryID = CGCurrencyTypes::m_categories[category].m_id;

        if (!CGCurrencyTypes::m_categories[category].m_expanded) {
            continue;
        }

        int32_t first = CGCurrencyTypes::m_listSize;

        for (int32_t i = 0; i < g_currencyTypesDB.m_numRecords; i++) {
            CurrencyTypesRec* rec = g_currencyTypesDB.GetRecordByIndex(i);

            if (!rec) {
                continue;
            }

            uint32_t known;
            uint32_t unused;
            uint32_t backpack;
            uint32_t mask;

            if (rec->m_bitIndex <= 32) {
                if (rec->m_bitIndex <= 0) {
                    continue;
                }

                known = known1;
                unused = unused1;
                backpack = backpack1;
                mask = 1 << (rec->m_bitIndex - 1);
            } else {
                known = known2;
                unused = unused2;
                backpack = backpack2;
                mask = 1 << (rec->m_bitIndex - 33);
            }

            if (!(mask & known)) {
                continue;
            }

            const char* name;

            if (category == CGCurrencyTypes::m_numCategories - 1) {
                if (!(mask & unused)) {
                    continue;
                }

                ItemStats_C* stats = g_itemDBCache.GetRecord(rec->m_itemID, ClntObjMgrGetActivePlayer(), callback, nullptr, true);
                name = stats ? stats->m_name[0] : nullptr;
            } else {
                if (rec->m_categoryID != CGCurrencyTypes::m_categories[category].m_id || (mask & unused)) {
                    continue;
                }

                ItemStats_C* stats = g_itemDBCache.GetRecord(rec->m_itemID, ClntObjMgrGetActivePlayer(), callback, nullptr, true);
                name = stats ? stats->GetName(0) : nullptr;
            }

            CurrencyListEntry* entry = &CGCurrencyTypes::m_list[CGCurrencyTypes::m_listSize++];
            entry->m_name = name;
            entry->m_itemID = rec->m_itemID;
            entry->m_categoryID = rec->m_categoryID;
            entry->m_isWatched = (backpack & mask) != 0;
        }

        if (CGCurrencyTypes::m_listSize - first > 1) {
            qsort(&CGCurrencyTypes::m_list[first], CGCurrencyTypes::m_listSize - first, sizeof(CurrencyListEntry), CGCurrencyTypes::DisplayListSortCallback);
        }
    }
}

// VERIFY: b46648dc auto
// OFFSET: 0x005B0660
void CGCurrencyTypes::EnterWorld() {
    CGCurrencyTypes::BuildDisplayList();
    FrameScript_SignalEvent(SCRIPT_EVENT_CURRENCY_DISPLAY_UPDATE, nullptr);
}

// VERIFY: 2c0c3c91 manual
// OFFSET: 0x005B0340
CurrencyTypesRec* CGCurrencyTypes::GetBackpackCurrencyInfo(int32_t index) {
    if (index < 0) {
        return nullptr;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return nullptr;
    }

    uint32_t known1 = static_cast<uint32_t>(player->m_player->m_playerFieldKnownCurrencies.m_guid);
    uint32_t known2 = static_cast<uint32_t>(player->m_player->m_playerFieldKnownCurrencies.m_guid >> 32);
    int32_t found = 0;

    for (int32_t i = 0; i < g_currencyTypesDB.m_numRecords; i++) {
        CurrencyTypesRec* rec = g_currencyTypesDB.GetRecordByIndex(i);

        if (!rec) {
            continue;
        }

        uint32_t known = 0;
        uint32_t shown = 0;
        uint32_t bit = 0;

        if (rec->m_bitIndex > 32) {
            known = known2;
            shown = g_currencyTokensBackpack2CVar->m_intValue;
            bit = 1 << (rec->m_bitIndex - 33);
        } else if (rec->m_bitIndex > 0) {
            known = known1;
            shown = g_currencyTokensBackpack1CVar->m_intValue;
            bit = 1 << (rec->m_bitIndex - 1);
        }

        if (!(bit & known) || !(bit & shown)) {
            continue;
        }

        if (found == index) {
            return rec;
        }

        found++;
    }

    return nullptr;
}

// VERIFY: 6cfe7d42 auto
// OFFSET: 0x005EEB70
void CGCurrencyTypes::ShutdownGame() {
}

// VERIFY: 01c6b2ac manual
// OFFSET: 0x005AFFD0
CurrencyTypesRec* CGCurrencyTypes::GetCurrencyRec(int32_t index) {
    if (index < 0 || index >= CGCurrencyTypes::m_listSize) {
        return nullptr;
    }

    for (int32_t i = 0; i < g_currencyTypesDB.m_numRecords; i++) {
        CurrencyTypesRec* rec = &g_currencyTypesDB.m_records[i];

        if (rec->m_itemID == CGCurrencyTypes::m_list[index].m_itemID) {
            return rec;
        }
    }

    return nullptr;
}

// VERIFY: a4265f41 manual
// OFFSET: 0x005B0440
void CGCurrencyTypes::SetUnused(int32_t index, int32_t unused) {
    if (index < 0 || index >= CGCurrencyTypes::m_listSize || CGCurrencyTypes::m_list[index].m_isHeader) {
        return;
    }

    CurrencyTypesRec* rec = CGCurrencyTypes::GetCurrencyRec(index);

    if (!rec) {
        return;
    }

    int32_t bit = rec->m_bitIndex;

    if (bit <= 32) {
        if (bit <= 0) {
            return;
        }

        int32_t mask = 1 << (bit - 1);

        if (unused) {
            g_currencyTokensUnused1CVar->Set(mask | g_currencyTokensUnused1CVar->m_intValue, true, false, false, true);
        } else {
            g_currencyTokensUnused1CVar->Set(~mask & g_currencyTokensUnused1CVar->m_intValue, true, false, false, true);
        }
    } else {
        int32_t mask = 1 << (bit - 33);

        if (unused) {
            g_currencyTokensUnused2CVar->Set(mask | g_currencyTokensUnused2CVar->m_intValue, true, false, false, true);
        } else {
            g_currencyTokensUnused2CVar->Set(~mask & g_currencyTokensUnused2CVar->m_intValue, true, false, false, true);
        }
    }
}

// VERIFY: 86e21a80 manual
// OFFSET: 0x005B04F0
int32_t CGCurrencyTypes::IsCurrencyShown(int32_t index) {
    if (index < 0 || index >= CGCurrencyTypes::m_listSize) {
        return 0;
    }

    if (CGCurrencyTypes::m_list[index].m_isHeader) {
        return CGCurrencyTypes::m_list[index].m_categoryID == CGCurrencyTypes::m_categories[CGCurrencyTypes::m_numCategories - 1].m_id;
    }

    CurrencyTypesRec* rec = CGCurrencyTypes::GetCurrencyRec(index);

    if (!rec) {
        return 0;
    }

    int32_t bit = rec->m_bitIndex;

    if (bit > 32) {
        return (g_currencyTokensUnused2CVar->m_intValue & (1 << (bit - 33))) != 0;
    }

    return bit > 0 && (g_currencyTokensUnused1CVar->m_intValue & (1 << (bit - 1))) != 0;
}

// VERIFY: 6b745ff7 manual
// OFFSET: 0x005B0590
void CGCurrencyTypes::SetBackpack(int32_t index, int32_t backpack) {
    if (index < 0 || index >= CGCurrencyTypes::m_listSize || CGCurrencyTypes::m_list[index].m_isHeader) {
        return;
    }

    CurrencyTypesRec* rec = CGCurrencyTypes::GetCurrencyRec(index);

    if (!rec) {
        return;
    }

    int32_t bit = rec->m_bitIndex;

    if (bit <= 32) {
        if (bit <= 0) {
            return;
        }

        int32_t mask = 1 << (bit - 1);

        if (backpack) {
            g_currencyTokensBackpack1CVar->Set(mask | g_currencyTokensBackpack1CVar->m_intValue, true, false, false, true);
        } else {
            g_currencyTokensBackpack1CVar->Set(~mask & g_currencyTokensBackpack1CVar->m_intValue, true, false, false, true);
        }
    } else {
        int32_t mask = 1 << (bit - 33);

        if (backpack) {
            g_currencyTokensBackpack2CVar->Set(mask | g_currencyTokensBackpack2CVar->m_intValue, true, false, false, true);
        } else {
            g_currencyTokensBackpack2CVar->Set(~mask & g_currencyTokensBackpack2CVar->m_intValue, true, false, false, true);
        }
    }
}

// VERIFY: ceeaf02e auto
// OFFSET: 0x005B0640
int32_t SignalKnownCurrencyTypesUpdate(WOWGUID guid, uint32_t offset, uint32_t count, const void* value, void* param) {
    CGCurrencyTypes::BuildDisplayList();
    FrameScript_SignalEvent(SCRIPT_EVENT_KNOWN_CURRENCY_TYPES_UPDATE, nullptr);
    return 1;
}

// VERIFY: e1912fc6 auto
// OFFSET: 0x005EEB70
void CGCurrencyTypes::LeaveWorld() {
}
