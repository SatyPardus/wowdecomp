#include "SkillInfo.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include <cstdlib>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Magic/MagicClient/SpellTable.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/DB/Rec/SkillLineRec.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/Ui/ScriptEvents.h"

// STATIC: 0x00C22AE8
uint32_t CGSkillInfo::m_numLines;

// STATIC: 0x00C22AEC
uint32_t CGSkillInfo::m_numTypes;

// STATIC: 0x00C22AF0
uint32_t CGSkillInfo::m_numSkillLines;

// STATIC: 0x00C22AF4
int32_t CGSkillInfo::m_selectedSkill;

// STATIC: 0x00C22AF8
int32_t CGSkillInfo::m_expandedTypes;

// STATIC: 0x00C22AFC
int32_t CGSkillInfo::unk_C22AFC;

// STATIC: 0x00C22B00
int32_t CGSkillInfo::m_skillUpPoints;

// STATIC: 0x00C22B04
TSFixedArray<SkillLineInfo*> CGSkillInfo::m_lines;

// STATIC: 0x00C22B10
TSFixedArray<SkillTypeInfo*> CGSkillInfo::m_types;

// VERIFY: f1687dc0 manual
// OFFSET: 0x005CDBF0
void CGSkillInfo::AddSkillUp(uint32_t index) {
    if (index >= CGSkillInfo::m_numLines) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    SkillLineRec* skillLine = g_skillLineDB.GetRecord(CGSkillInfo::m_lines.m_data[index]->m_skillLineID);
    const SkillRaceClassInfoRec* info = SpellTableLookupInfo(player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[1], CGSkillInfo::m_lines.m_data[index]->m_skillLineID);

    if (!skillLine || !info) {
        return;
    }

    int32_t skillIndex = player->GetSkillIndex(CGSkillInfo::m_lines.m_data[index]->m_skillLineID);
    int32_t value = player->GetSkillValue(skillIndex);

    if (value + CGSkillInfo::m_lines.m_data[index]->m_skillUps >= static_cast<int32_t>(player->GetSkillMaxRank(skillIndex))) {
        return;
    }

    int32_t cost = CGSkillInfo::GetSkillCostField(info, CGSkillInfo::m_lines.m_data[index]->m_skillUps + value + 1);

    if (cost + CGSkillInfo::m_skillUpPoints <= player->GetCharacterPoints(1)) {
        CGSkillInfo::m_lines.m_data[index]->m_skillUps++;
        CGSkillInfo::m_skillUpPoints += cost;
        FrameScript_SignalEvent(SCRIPT_EVENT_SKILL_LINES_CHANGED, nullptr);
    }
}

// VERIFY: c1910f28 manual
// OFFSET: 0x005CDD20
void CGSkillInfo::RemoveSkillUp(uint32_t index) {
    if (index >= CGSkillInfo::m_numLines) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !CGSkillInfo::m_lines.m_data[index]->m_skillUps) {
        return;
    }

    SkillLineRec* skillLine = g_skillLineDB.GetRecord(CGSkillInfo::m_lines.m_data[index]->m_skillLineID);
    const SkillRaceClassInfoRec* info = SpellTableLookupInfo(player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[1], CGSkillInfo::m_lines.m_data[index]->m_skillLineID);

    if (!skillLine || !info) {
        return;
    }

    int32_t skillIndex = player->GetSkillIndex(CGSkillInfo::m_lines.m_data[index]->m_skillLineID);
    int32_t value = player->GetSkillValue(skillIndex);
    int32_t cost = CGSkillInfo::GetSkillCostField(info, value + CGSkillInfo::m_lines.m_data[index]->m_skillUps);
    CGSkillInfo::m_lines.m_data[index]->m_skillUps--;
    CGSkillInfo::m_skillUpPoints -= cost;
    FrameScript_SignalEvent(SCRIPT_EVENT_SKILL_LINES_CHANGED, nullptr);
}

// VERIFY: 579e798b manual
// OFFSET: 0x005CE880
void CGSkillInfo::InitializeGame() {
    uint32_t numTypes = g_skillLineCategoryDB.m_numRecords;
    uint32_t numLines = numTypes + 128;

    CGSkillInfo::m_lines.SetCount(numLines);

    for (uint32_t i = 0; i < numLines; i++) {
        CGSkillInfo::m_lines.m_data[i] = static_cast<SkillLineInfo*>(SMemAlloc(sizeof(SkillLineInfo), __FILE__, __LINE__, 0));
        CGSkillInfo::m_lines.m_data[i]->m_skillLineID = 0;
        CGSkillInfo::m_lines.m_data[i]->m_categoryID = 0;
        CGSkillInfo::m_lines.m_data[i]->m_expanded = 1;
    }

    CGSkillInfo::m_types.SetCount(numTypes);

    for (uint32_t i = 0; i < numTypes; i++) {
        CGSkillInfo::m_types.m_data[i] = static_cast<SkillTypeInfo*>(SMemAlloc(sizeof(SkillTypeInfo), __FILE__, __LINE__, 0));
        CGSkillInfo::m_types.m_data[i]->m_categoryID = 0;
        CGSkillInfo::m_types.m_data[i]->m_collapsed = 0;
    }

    CGSkillInfo::m_numLines = 0;
    CGSkillInfo::m_numTypes = 0;
    CGSkillInfo::m_numSkillLines = 0;
    CGSkillInfo::m_selectedSkill = 0;
    CGSkillInfo::m_skillUpPoints = 0;
    CGSkillInfo::m_expandedTypes = -1;
}

// VERIFY: c9c769d1 manual
// OFFSET: 0x005CD360
int32_t CGSkillInfo::GetSelectedIndex() {
    if (!CGSkillInfo::m_selectedSkill || !CGSkillInfo::m_numSkillLines) {
        return -1;
    }

    for (uint32_t i = 0; i < CGSkillInfo::m_numSkillLines; i++) {
        if (CGSkillInfo::m_lines.m_data[i]->m_skillLineID == CGSkillInfo::m_selectedSkill) {
            return i;
        }
    }

    return -1;
}

// VERIFY: 3729603c manual
// OFFSET: 0x005CD430
static int32_t SortSkillTypes(const void* a, const void* b) {
    const SkillTypeInfo* typeA = *static_cast<SkillTypeInfo* const*>(a);
    const SkillTypeInfo* typeB = *static_cast<SkillTypeInfo* const*>(b);
    SkillLineCategoryRec* recA = g_skillLineCategoryDB.GetRecord(typeA->m_categoryID);
    SkillLineCategoryRec* recB = g_skillLineCategoryDB.GetRecord(typeB->m_categoryID);
    return 2 * (recA->m_sortIndex >= recB->m_sortIndex) - 1;
}

// VERIFY: 4448ea95 manual
// OFFSET: 0x005CD4A0
static int32_t SortSkillLines(const void* a, const void* b) {
    const SkillLineInfo* lineA = *static_cast<SkillLineInfo* const*>(a);
    const SkillLineInfo* lineB = *static_cast<SkillLineInfo* const*>(b);

    if (!lineA->m_expanded || !lineB->m_expanded) {
        if (!lineA->m_expanded && !lineB->m_expanded) {
            return 0;
        }

        return 2 * (lineA->m_expanded == 0) - 1;
    }

    uint32_t typeA = 0;
    uint32_t typeB = 0;

    for (uint32_t i = 0; i < CGSkillInfo::m_numTypes; i++) {
        if (CGSkillInfo::m_types.m_data[i]->m_categoryID == lineA->m_categoryID) {
            typeA = i;
        }

        if (CGSkillInfo::m_types.m_data[i]->m_categoryID == lineB->m_categoryID) {
            typeB = i;
        }
    }

    if (typeA != typeB) {
        return 2 * (static_cast<int32_t>(typeA) >= static_cast<int32_t>(typeB)) - 1;
    }

    if (!lineA->m_skillLineID) {
        return -1;
    }

    if (!lineB->m_skillLineID) {
        return 1;
    }

    if (lineA->unk_000C != lineB->unk_000C) {
        return 2 * (lineA->unk_000C != 0) - 1;
    }

    SkillLineRec* recA = g_skillLineDB.GetRecord(lineA->m_skillLineID);
    SkillLineRec* recB = g_skillLineDB.GetRecord(lineB->m_skillLineID);

    if (!recA || !recB) {
        return 0;
    }

    return SStrSortUTF8I(recA->m_displayName, recB->m_displayName, STORM_MAX_STR);
}

// VERIFY: f7f8fa48 auto
// OFFSET: 0x005CD5E0
void CGSkillInfo::UpdateSkillLines() {
    CGSkillInfo::m_numSkillLines = CGSkillInfo::m_numLines;

    for (uint32_t i = 0; i < CGSkillInfo::m_numTypes; i++) {
        CGSkillInfo::m_types.m_data[i]->m_collapsed = !(CGSkillInfo::m_expandedTypes & (1 << i));
    }

    for (uint32_t i = 0; i < CGSkillInfo::m_numLines; i++) {
        SkillLineInfo* line = CGSkillInfo::m_lines.m_data[i];

        if (!line->m_skillLineID) {
            line->m_expanded = 1;
            continue;
        }

        uint32_t type;

        for (type = 0; type < CGSkillInfo::m_numTypes; type++) {
            if (CGSkillInfo::m_types.m_data[type]->m_categoryID == line->m_categoryID) {
                break;
            }
        }

        if (type == CGSkillInfo::m_numTypes) {
            continue;
        }

        line->m_expanded = !CGSkillInfo::m_types.m_data[type]->m_collapsed && (!CGSkillInfo::unk_C22AFC || !line->unk_000C);

        if (!CGSkillInfo::m_lines.m_data[i]->m_expanded) {
            CGSkillInfo::m_numSkillLines--;
        }
    }

    qsort(CGSkillInfo::m_types.m_data, CGSkillInfo::m_numTypes, sizeof(SkillTypeInfo*), SortSkillTypes);
    qsort(CGSkillInfo::m_lines.m_data, CGSkillInfo::m_numLines, sizeof(SkillLineInfo*), SortSkillLines);
    FrameScript_SignalEvent(SCRIPT_EVENT_SKILL_LINES_CHANGED, nullptr);
}

// VERIFY: 8003513c auto
// OFFSET: 0x005CD760
void CGSkillInfo::CollapseHeader(uint32_t index) {
    if (index >= CGSkillInfo::m_numLines) {
        return;
    }

    for (uint32_t i = 0; i < CGSkillInfo::m_numTypes; i++) {
        if (CGSkillInfo::m_types.m_data[i]->m_categoryID == CGSkillInfo::m_lines.m_data[index]->m_categoryID) {
            CGSkillInfo::m_expandedTypes &= ~(1 << i);
            CGSkillInfo::UpdateSkillLines();
            return;
        }
    }
}

// VERIFY: 58bf5833 auto
// OFFSET: 0x005CD7C0
void CGSkillInfo::ExpandHeader(uint32_t index) {
    if (index >= CGSkillInfo::m_numLines) {
        return;
    }

    for (uint32_t i = 0; i < CGSkillInfo::m_numTypes; i++) {
        if (CGSkillInfo::m_types.m_data[i]->m_categoryID == CGSkillInfo::m_lines.m_data[index]->m_categoryID) {
            CGSkillInfo::m_expandedTypes |= 1 << i;
            CGSkillInfo::UpdateSkillLines();
            return;
        }
    }
}

// VERIFY: c12af19b manual
// OFFSET: 0x005CD9B0
void PlayerSignalSkillStatChanges() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    CGSkillInfo::m_expandedTypes = -1;
    CGSkillInfo::m_numLines = 0;
    CGSkillInfo::m_numTypes = 0;
    CGSkillInfo::m_numSkillLines = 0;
    CGSkillInfo::m_skillUpPoints = 0;

    for (uint32_t i = 0; i < 128; i++) {
        CGPlayerLocalData* data = static_cast<CGPlayerLocalData*>(player->m_player);
        uint16_t skillId = player->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? data->m_playerSkillInfo[i].m_skillId : 0;

        if (!skillId) {
            continue;
        }

        SkillLineRec* rec = g_skillLineDB.GetRecord(skillId);
        const SkillRaceClassInfoRec* info = SpellTableLookupInfo(player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[1], skillId);

        if (!rec || !info || !g_skillLineCategoryDB.GetRecord(rec->m_categoryID) || (info->m_flags & 2)) {
            continue;
        }

        uint16_t rank = data->m_playerSkillInfo[i].m_rank;

        if (!(info->m_flags & 1) && !rank && (!(info->m_flags & 4) || static_cast<int32_t>(player->m_unit->m_unitFieldLevel) < info->m_minLevel)) {
            continue;
        }

        CGSkillInfo::m_lines.m_data[CGSkillInfo::m_numLines]->m_skillLineID = skillId;
        CGSkillInfo::m_lines.m_data[CGSkillInfo::m_numLines]->m_categoryID = rec->m_categoryID;
        CGSkillInfo::m_lines.m_data[CGSkillInfo::m_numLines]->unk_000C = rank == 0;
        CGSkillInfo::m_lines.m_data[CGSkillInfo::m_numLines]->m_skillUps = 0;
        CGSkillInfo::m_numLines++;

        uint32_t type;

        for (type = 0; type < CGSkillInfo::m_numTypes; type++) {
            if (CGSkillInfo::m_types.m_data[type]->m_categoryID == rec->m_categoryID) {
                break;
            }
        }

        if (type != CGSkillInfo::m_numTypes) {
            continue;
        }

        CGSkillInfo::m_types.m_data[CGSkillInfo::m_numTypes++]->m_categoryID = rec->m_categoryID;
        CGSkillInfo::m_lines.m_data[CGSkillInfo::m_numLines]->m_skillLineID = 0;
        CGSkillInfo::m_lines.m_data[CGSkillInfo::m_numLines]->m_categoryID = rec->m_categoryID;
        CGSkillInfo::m_numLines++;
    }

    CGSkillInfo::UpdateSkillLines();
}

// VERIFY: 8c467dbf auto
// OFFSET: 0x005CD710
int32_t CGSkillInfo::GetSkillTypeCollapsed(uint32_t index) {
    if (index >= CGSkillInfo::m_numLines) {
        return 0;
    }

    for (uint32_t i = 0; i < CGSkillInfo::m_numTypes; i++) {
        SkillTypeInfo* type = CGSkillInfo::m_types.m_data[i];

        if (type->m_categoryID == CGSkillInfo::m_lines.m_data[index]->m_categoryID) {
            return type->m_collapsed;
        }
    }

    return 0;
}

// VERIFY: 3727cac0 manual
// OFFSET: 0x005CD3A0
void CGSkillInfo::ShutdownGame() {
    for (uint32_t i = 0; i < CGSkillInfo::m_lines.m_count; i++) {
        delete CGSkillInfo::m_lines.m_data[i];
        CGSkillInfo::m_lines.m_data[i] = nullptr;
    }

    for (uint32_t i = 0; i < CGSkillInfo::m_types.m_count; i++) {
        delete CGSkillInfo::m_types.m_data[i];
        CGSkillInfo::m_types.m_data[i] = nullptr;
    }
}

// VERIFY: 0141b453 auto
// OFFSET: 0x005EEB70
void CGSkillInfo::LeaveWorld() {
}
