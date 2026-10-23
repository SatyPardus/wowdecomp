#include "SpellTable.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Types.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/Base/CDataAllocator.h"
#include "WoW/Source/Ui/SkillInfo.h"
#include "WoW/Source/DB/Rec/SkillLineRec.h"
#include "WoW/Source/DB/Rec/SkillCostsDataRec.h"
#include "WoW/Source/DB/Rec/SkillRaceClassInfoRec.h"

// STATIC: 0x00D3F60C
static bool s_skillLineLinkInfoDirty;

// STATIC: 0x00D3F610
static TSHashTable<RACECLASSINFO, HASHKEY_NONE> s_raceClassInfo;

// STATIC: 0x00D3F638
static TSHashTable<SKILLCOSTSINFO, HASHKEY_NONE> s_skillCostsInfo;

class SkillLineRecTable : public TSHashTable<SKILLLINEREC, HASHKEY_SKILLLINE> {
    public:
    void InternalDelete(SKILLLINEREC* ptr) override;
    SKILLLINEREC* InternalNew(TSExplicitList<SKILLLINEREC>* listptr, size_t extrabytes, uint32_t flags) override;
};

// STATIC: 0x00D3F660
static SkillLineRecTable s_skillLineRecs;

// STATIC: 0x00D3F688
static TSGrowableArray<SkillLineLinkInfo> s_skillLineLinkInfo;

// STATIC: 0x00D3F698
static CDataAllocator s_skillLineRecAllocator(sizeof(SKILLLINEREC), 0x1000);

// VERIFY: ce9d9398 auto
// OFFSET: 0x00811C90
void SkillLineRecTable::InternalDelete(SKILLLINEREC* ptr) {
    s_skillLineRecAllocator.Delete(ptr);
}

// VERIFY: 4ba0fa22 manual
// OFFSET: 0x00811CC0
SKILLLINEREC* SkillLineRecTable::InternalNew(TSExplicitList<SKILLLINEREC>* listptr, size_t extrabytes, uint32_t flags) {
    SKILLLINEREC* rec = s_skillLineRecAllocator.New<SKILLLINEREC>(1);
    listptr->LinkToHead(rec);
    return rec;
}

// VERIFY: 87b0816a manual
// OFFSET: 0x00810320
bool SpellTableMatchRaceClass(int32_t race, int32_t classId, int32_t raceMask, int32_t classMask, int32_t excludeRace, int32_t excludeClass) {
    if (excludeRace) {
        raceMask = ~raceMask;
    }

    if (excludeClass) {
        classMask = ~classMask;
    }

    return (!raceMask || ((1 << (race - 1)) & raceMask)) && (!classMask || ((1 << (classId - 1)) & classMask));
}

// VERIFY: 57f2bfbe auto
// OFFSET: 0x00810410
SkillLineAbilityRec* SpellTableLookupPetAbility(int32_t skillLine, int32_t spellId) {
    for (int32_t i = 0; i < g_skillLineAbilityDB.m_numRecords; i++) {
        SkillLineAbilityRec* ability = (i < 0 || i >= g_skillLineAbilityDB.m_numRecords) ? nullptr : &g_skillLineAbilityDB.m_records[i];

        if (ability->m_skillLine == skillLine && ability->m_spell == spellId) {
            return ability;
        }
    }

    return nullptr;
}

// VERIFY: 96f84caf auto
// OFFSET: 0x008104A0
bool SpellTableGetSkillLineLinkInfo(int32_t skillLine, const SkillLineAbilityRec*& ability, uint32_t& count) {
    for (uint32_t i = 0; i < s_skillLineLinkInfo.Count(); i++) {
        if (s_skillLineLinkInfo.m_data[i].m_ability->m_skillLine == skillLine) {
            ability = s_skillLineLinkInfo.m_data[i].m_ability;
            count = s_skillLineLinkInfo.m_data[i].m_count;
            return true;
        }
    }

    return false;
}

// VERIFY: c2ad67d0 auto
// OFFSET: 0x00810ED0
const SkillRaceClassInfoRec* SpellTableLookupInfo(int32_t race, int32_t classId, int32_t skillId) {
    HASHKEY_NONE key;
    RACECLASSINFO* info = s_raceClassInfo.Ptr(skillId, key);

    if (!info) {
        return nullptr;
    }

    const SkillRaceClassInfoRec* rec = info->m_info;

    for (int32_t i = 0; i < info->m_count; i++, rec++) {
        if ((!rec->m_raceMask || ((1 << (race - 1)) & rec->m_raceMask)) && (!rec->m_classMask || ((1 << (classId - 1)) & rec->m_classMask))) {
            return rec;
        }
    }

    return nullptr;
}

// VERIFY: f2eba80a auto
// OFFSET: 0x00810F50
int32_t CGSkillInfo::GetSkillCostField(const SkillRaceClassInfoRec* info, int32_t skillValue) {
    SkillLineRec* skillLine = g_skillLineDB.GetRecord(info->m_skillID);

    if (!skillLine || !skillLine->m_skillCostsID) {
        return 0;
    }

    HASHKEY_NONE key;
    SKILLCOSTSINFO* costs = s_skillCostsInfo.Ptr(skillLine->m_skillCostsID, key);

    if (!costs || skillValue > costs->m_count) {
        return 0;
    }

    return costs->m_info[skillValue - 1].m_cost[info->m_skillCostIndex];
}

// VERIFY: 6585c0df manual
// OFFSET: 0x00811F20
static bool SpellTableAddSkillLineRec(int32_t race, int32_t classId, const SkillLineAbilityRec* ability) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && race != player->m_unit->m_unitFieldBytes0[0] && classId != player->m_unit->m_unitFieldBytes0[1]) {
        return false;
    }

    if (!SpellTableLookupInfo(race, classId, ability->m_skillLine) || !SpellTableMatchRaceClass(race, classId, ability->m_raceMask, ability->m_classMask, ability->m_excludeRace, ability->m_excludeClass)) {
        return false;
    }

    HASHKEY_SKILLLINE key;
    key.m_race = static_cast<uint8_t>(race);
    key.m_class = static_cast<uint8_t>(classId);
    key.m_spellId = ability->m_spell;

    SKILLLINEREC* rec = s_skillLineRecs.New(ability->m_spell, key, 0, 0);
    rec->m_ability = ability;
    return true;
}

// VERIFY: d2830bd1 manual
// OFFSET: 0x00812030
static void SpellTableBuildSkillLineLinkInfo() {
    SkillLineLinkInfo* link = nullptr;
    bool first = true;
    int32_t lastSkillLine = -1;

    for (int32_t i = 0; i < g_charBaseInfoDB.m_numRecords; i++) {
        CharBaseInfoRec* base = (i < 0 || i >= g_charBaseInfoDB.m_numRecords) ? nullptr : &g_charBaseInfoDB.m_records[i];
        uint8_t race = static_cast<uint8_t>(base->m_raceID);

        if (!g_chrRacesDB.GetRecord(race)) {
            continue;
        }

        for (int32_t j = 0; j < g_skillLineAbilityDB.m_numRecords; j++) {
            SkillLineAbilityRec* ability = (j < 0 || j >= g_skillLineAbilityDB.m_numRecords) ? nullptr : &g_skillLineAbilityDB.m_records[j];
            SpellTableAddSkillLineRec(race, static_cast<uint8_t>(base->m_classID), ability);

            if (!first) {
                continue;
            }

            if (lastSkillLine == ability->m_skillLine) {
                if (link) {
                    link->m_count++;
                }

                continue;
            }

            SkillLineRec* skillLine = g_skillLineDB.GetRecord(ability->m_skillLine);

            if (skillLine && skillLine->m_canLink) {
                link = s_skillLineLinkInfo.New();
                link->m_ability = ability;
                link->m_count = 1;
                lastSkillLine = ability->m_skillLine;
            } else {
                link = nullptr;
            }
        }

        first = false;
    }

    s_skillLineLinkInfoDirty = false;
}

// VERIFY: 60f45dcc manual
// OFFSET: 0x00812200
void SpellTableInitialize() {
    RACECLASSINFO* raceClassInfo = nullptr;
    int32_t lastSkill = -1;

    for (int32_t i = 0; i < g_skillRaceClassInfoDB.m_numRecords; i++) {
        SkillRaceClassInfoRec* rec = (i < 0 || i >= g_skillRaceClassInfoDB.m_numRecords) ? nullptr : &g_skillRaceClassInfoDB.m_records[i];

        if (rec->m_skillID == lastSkill) {
            raceClassInfo->m_count++;
            continue;
        }

        HASHKEY_NONE key;
        raceClassInfo = s_raceClassInfo.New(rec->m_skillID, key, 0, 0);
        raceClassInfo->m_info = rec;
        raceClassInfo->m_count = 1;
        lastSkill = rec->m_skillID;
    }

    SKILLCOSTSINFO* skillCostsInfo = nullptr;
    int32_t lastCosts = -1;

    for (int32_t i = 0; i < g_skillCostsDataDB.m_numRecords; i++) {
        SkillCostsDataRec* rec = (i < 0 || i >= g_skillCostsDataDB.m_numRecords) ? nullptr : &g_skillCostsDataDB.m_records[i];

        if (rec->m_skillCostsID == lastCosts) {
            skillCostsInfo->m_count++;
            continue;
        }

        HASHKEY_NONE key;
        skillCostsInfo = s_skillCostsInfo.New(rec->m_skillCostsID, key, 0, 0);
        skillCostsInfo->m_info = rec;
        skillCostsInfo->m_count = 1;
        lastCosts = rec->m_skillCostsID;
    }

    s_skillLineLinkInfoDirty = true;
}

// VERIFY: e0671a08 manual
// OFFSET: 0x00812410
SkillLineAbilityRec* SpellTableLookupAbility(int32_t race, int32_t classId, int32_t spellId) {
    if (s_skillLineLinkInfoDirty) {
        SpellTableBuildSkillLineLinkInfo();
    }

    HASHKEY_SKILLLINE key;
    key.m_race = static_cast<uint8_t>(race);
    key.m_class = static_cast<uint8_t>(classId);
    key.m_spellId = spellId;

    SKILLLINEREC* rec = s_skillLineRecs.Ptr(spellId, key);

    if (rec) {
        return const_cast<SkillLineAbilityRec*>(rec->m_ability);
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return nullptr;
    }

    if (player->m_unit->m_unitFieldBytes0[0] == race && player->m_unit->m_unitFieldBytes0[1] == classId) {
        return nullptr;
    }

    for (int32_t i = 0; i < g_skillLineAbilityDB.m_numRecords; i++) {
        SkillLineAbilityRec* ability = (i < 0 || i >= g_skillLineAbilityDB.m_numRecords) ? nullptr : &g_skillLineAbilityDB.m_records[i];

        if (ability->m_spell == spellId && SpellTableLookupInfo(race, classId, ability->m_skillLine) && SpellTableMatchRaceClass(race, classId, ability->m_raceMask, ability->m_classMask, ability->m_excludeRace, ability->m_excludeClass)) {
            return ability;
        }
    }

    return nullptr;
}

// VERIFY: 5dbfe9bd auto
// OFFSET: 0x00810460
int32_t GetSkillTierCost(const SkillRaceClassInfoRec* info, uint32_t step) {
    if (!info->m_skillTierID) {
        return 0;
    }

    SkillTiersRec* tier = g_skillTiersDB.GetRecord(info->m_skillTierID);

    if (!tier || step >= 16) {
        return 0;
    }

    return tier->m_cost[step];
}

// VERIFY: e9c3aee4 auto
// OFFSET: 0x00810920
void SpellTableDestroy() {
    s_skillLineRecs.Clear();
    s_skillLineLinkInfoDirty = true;
    s_raceClassInfo.Clear();
    s_skillCostsInfo.Clear();
    s_skillLineRecAllocator.Clear(typeid(SKILLLINEREC).raw_name(), SLOG_OBJECT, 1);
    s_skillLineLinkInfo.m_count = 0;
}
