#ifndef WOW_SOURCE_UI_SKILLINFO_H
#define WOW_SOURCE_UI_SKILLINFO_H

#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"

class SkillRaceClassInfoRec;
struct lua_State;

struct SkillLineInfo {
    /* 0000 */ int32_t m_skillLineID;
    /* 0004 */ int32_t m_categoryID;
    /* 0008 */ int32_t m_expanded;
    /* 000C */ int32_t unk_000C;
    /* 0010 */ int32_t m_skillUps;
};

struct SkillTypeInfo {
    /* 0000 */ int32_t m_categoryID;
    /* 0004 */ int32_t m_collapsed;
    /* 0008 */ int32_t unused_0008;
};

class CGSkillInfo {
    public:
    static void LeaveWorld();
    static uint32_t m_numLines;
    static uint32_t m_numTypes;
    static uint32_t m_numSkillLines;
    static int32_t m_selectedSkill;
    static int32_t m_expandedTypes;
    static int32_t unk_C22AFC;
    static int32_t m_skillUpPoints;
    static TSFixedArray<SkillLineInfo*> m_lines;
    static TSFixedArray<SkillTypeInfo*> m_types;
    static void InitializeGame();
    static void ShutdownGame();
    static int32_t GetSelectedIndex();
    static void UpdateSkillLines();
    static int32_t GetSkillTypeCollapsed(uint32_t index);
    static int32_t GetSkillCostField(const SkillRaceClassInfoRec* info, int32_t skillValue);
    static void CollapseHeader(uint32_t index);
    static void ExpandHeader(uint32_t index);
    static void AddSkillUp(uint32_t index);
    static void RemoveSkillUp(uint32_t index);
};

void SkillInfoRegisterScriptFunctions();
void SkillInfoUnregisterScriptFunctions();

#endif
