#include <cstdint>
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Game/GameClient/tables.h"

// STATIC: 0x00AF4FC0
static ClientGameTable<gtBarberShopCostBaseRec> s_gtBarberShopCostBase("BarberShopCostBase", &g_gtBarberShopCostBaseDB);

// STATIC: 0x00AF4FD0
static ClientGameTable<gtCombatRatingsRec> s_gtCombatRatings("CombatRatings", &g_gtCombatRatingsDB);

// STATIC: 0x00AF4FE0
static ClientGameTable<gtChanceToMeleeCritRec> s_gtChanceToMeleeCrit("ChanceToMeleeCrit", &g_gtChanceToMeleeCritDB);

// STATIC: 0x00AF4FF0
static ClientGameTable<gtChanceToMeleeCritBaseRec> s_gtChanceToMeleeCritBase("ChanceToMeleeCritBase", &g_gtChanceToMeleeCritBaseDB);

// STATIC: 0x00AF5000
static ClientGameTable<gtChanceToSpellCritRec> s_gtChanceToSpellCrit("ChanceToSpellCrit", &g_gtChanceToSpellCritDB);

// STATIC: 0x00AF5010
static ClientGameTable<gtChanceToSpellCritBaseRec> s_gtChanceToSpellCritBase("ChanceToSpellCritBase", &g_gtChanceToSpellCritBaseDB);

// STATIC: 0x00AF5020
static ClientGameTable<gtNPCManaCostScalerRec> s_gtNPCManaCostScaler("NPCManaCostScaler", &g_gtNPCManaCostScalerDB);

// STATIC: 0x00AF5030
static ClientGameTable<gtOCTRegenHPRec> s_gtOCTRegenHP("OCTRegenHP", &g_gtOCTRegenHPDB);

// STATIC: 0x00AF5040
static ClientGameTable<gtOCTRegenMPRec> s_gtOCTRegenMP("OCTRegenMP", &g_gtOCTRegenMPDB);

// STATIC: 0x00AF5050
static ClientGameTable<gtRegenHPPerSptRec> s_gtRegenHPPerSpt("RegenHPPerSpt", &g_gtRegenHPPerSptDB);

// STATIC: 0x00AF5060
static ClientGameTable<gtRegenMPPerSptRec> s_gtRegenMPPerSpt("RegenMPPerSpt", &g_gtRegenMPPerSptDB);

// STATIC: 0x00AF5070
static ClientGameTable<gtOCTClassCombatRatingScalarRec> s_gtOCTClassCombatRatingScalar("OCTClassCombatRatingScalar", &g_gtOCTClassCombatRatingScalarDB);

// STATIC: 0x00AF4F90
static ClientGameTableBase* s_gameTables[] = { &s_gtBarberShopCostBase, &s_gtCombatRatings, &s_gtChanceToMeleeCrit, &s_gtChanceToMeleeCritBase, &s_gtChanceToSpellCrit, &s_gtChanceToSpellCritBase, &s_gtNPCManaCostScaler, &s_gtOCTRegenHP, &s_gtOCTRegenMP, &s_gtRegenHPPerSpt, &s_gtRegenMPPerSpt, &s_gtOCTClassCombatRatingScalar };

// INLINED
ClientGameTableBase::ClientGameTableBase(const char* name) {
    this->m_name = name;
    this->m_gameTable = nullptr;
}

// VERIFY: 001196df auto
// OFFSET: 0x007F6A10
void ClientGameTableBase::Initialize() {
    for (int32_t i = 0; i <= g_gameTablesDB.m_numRecords; i++) {
        GameTablesRec* rec = i >= 0 && i < g_gameTablesDB.m_numRecords ? &g_gameTablesDB.m_records[i] : nullptr;

        if (!SStrCmpI(this->m_name, rec->m_name, STORM_MAX_STR)) {
            this->m_gameTable = rec;

            return;
        }
    }
}

// VERIFY: 5f1fbdc9 auto
// OFFSET: 0x007F6A80
void GameTablesInitialize() {
    for (uint32_t i = 0; i < sizeof(s_gameTables) / sizeof(s_gameTables[0]); i++) {
        s_gameTables[i]->Initialize();
    }
}

// VERIFY: 166b2593 auto
// OFFSET: 0x007F6990
float GetGameTableValue(int32_t table, int32_t index, int32_t row) {
    return s_gameTables[table]->EvaluateTable(index, row);
}
