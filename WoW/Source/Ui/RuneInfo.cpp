#include "RuneInfo.h"
#include "Engine/Source/Os/TimeManager.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/SpellRuneCostRec.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/Types.h"
// STATIC: 0x00C24304
int32_t CGRuneInfo::m_runes[8];

// STATIC: 0x00C24324
int32_t CGRuneInfo::m_baseRunes[8];

// STATIC: 0x00C24344
uint32_t CGRuneInfo::m_runeCooldowns[8];

// STATIC: 0x00C24364
uint32_t CGRuneInfo::m_runeLastUsed[8];

// STATIC: 0x00C24384
uint32_t CGRuneInfo::m_numRunes;

// STATIC: 0x00C24388
uint32_t CGRuneInfo::m_predictedRunes;


// VERIFY: 6c8047e1 auto
// OFFSET: 0x005EDEF0
void CGRuneInfo::InitializeGame() {
    for (uint32_t i = 0; i < 8; i++) {
        CGRuneInfo::m_runeLastUsed[i] = 0;
    }

    for (uint32_t i = 0; i < 8; i++) {
        CGRuneInfo::m_runeCooldowns[i] = 0;
    }

    CGRuneInfo::m_runes[0] = 0;
    CGRuneInfo::m_baseRunes[0] = 0;
    CGRuneInfo::m_runes[1] = 0;
    CGRuneInfo::m_baseRunes[1] = 0;
    CGRuneInfo::m_runes[2] = 1;
    CGRuneInfo::m_baseRunes[2] = 1;
    CGRuneInfo::m_runes[3] = 1;
    CGRuneInfo::m_baseRunes[3] = 1;
    CGRuneInfo::m_predictedRunes = 0xFFFFFFFF;
    CGRuneInfo::m_numRunes = 6;
    CGRuneInfo::m_runes[4] = 2;
    CGRuneInfo::m_baseRunes[4] = 2;
    CGRuneInfo::m_runes[5] = 2;
    CGRuneInfo::m_baseRunes[5] = 2;
}

// VERIFY: 2b125080 manual
// OFFSET: 0x005EDFC0
bool CGRuneInfo::IsRuneIndexValid(int32_t index) {
    if (index > 7) {
        return false;
    }

    if (CGRuneInfo::m_numRunes) {
        return static_cast<uint32_t>(index) < CGRuneInfo::m_numRunes;
    }

    return index < 8;
}

// VERIFY: afb005fb auto
// OFFSET: 0x005EDFF0
int32_t CGRuneInfo::GetRuneTypeByIndex(int32_t index, bool base) {
    if (base) {
        return CGRuneInfo::m_baseRunes[index];
    }

    return CGRuneInfo::m_runes[index];
}

// VERIFY: e47d74df manual
// OFFSET: 0x005EE1F0
uint32_t CGRuneInfo::GetRuneRechargeTime(int32_t index) {
    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__) || ((1 << index) & CGRuneInfo::m_predictedRunes)) {
        return 0;
    }

    return CGRuneInfo::m_runeCooldowns[index];
}

// VERIFY: c02488c7 manual
// OFFSET: 0x005EE3D0
int32_t CGRuneInfo::CountPredictedRunesOfType(int32_t type) {
    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        return 0;
    }

    int32_t count = 0;

    for (uint32_t i = 0; i < CGRuneInfo::m_numRunes; i++) {
        if (CGRuneInfo::m_runes[i] == type && ((1 << i) & CGRuneInfo::m_predictedRunes)) {
            count++;
        }
    }

    return count;
}

// VERIFY: e0b2be36 manual
// OFFSET: 0x005EE5B0
bool CGRuneInfo::HasRunesForSpell(int32_t spellId, int32_t* needed) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return false;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return false;
    }

    int32_t school = spell.m_schoolMask;
    int32_t pct = player->GetSpellCostModifier(school) + 100;
    pct = static_cast<int32_t>(player->GetSpellCostMultiplier(school) * pct);
    Spell_C_ModifySpellValue(&spell, pct, 14);

    if (!pct) {
        return true;
    }

    if (spell.m_powerType != POWER_TYPE_RUNES) {
        return true;
    }

    SpellRuneCostRec* cost = g_spellRuneCostDB.GetRecord(spell.m_runeCostID);

    if (!cost) {
        return true;
    }

    int32_t missing = 0;
    bool short1 = false;
    bool short2 = false;

    if (cost->m_blood) {
        int32_t have = CGRuneInfo::CountPredictedRunesOfType(0);
        int32_t need = cost->m_blood;

        if (pct > 100) {
            need = pct * need / 100;
        }

        if (have < need) {
            missing = need - have;
            short1 = true;

            if (needed) {
                needed[0] = need - have;
            }
        }
    }

    if (cost->m_unholy) {
        int32_t have = CGRuneInfo::CountPredictedRunesOfType(1);
        int32_t need = cost->m_unholy;

        if (pct > 100) {
            need = pct * need / 100;
        }

        if (have < need) {
            missing += need - have;
            short1 = true;

            if (needed) {
                needed[1] = need - have;
            }
        }
    }

    if (cost->m_frost) {
        int32_t have = CGRuneInfo::CountPredictedRunesOfType(2);
        int32_t need = cost->m_frost;

        if (pct > 100) {
            need = pct * need / 100;
        }

        if (have < need) {
            missing += need - have;
            short2 = true;

            if (needed) {
                needed[2] = need - have;
            }
        }
    }

    if (!short1 && !short2) {
        return true;
    }

    if (CGRuneInfo::CountPredictedRunesOfType(3) >= missing) {
        return true;
    }

    if (needed) {
        needed[3] = missing - CGRuneInfo::CountPredictedRunesOfType(3);
    }

    return false;
}

// VERIFY: f7ba20b6 manual
// OFFSET: 0x005EE7E0
bool CGRuneInfo::ComputeRuneSpellCooldown(const SpellRec* spell, uint32_t* cooldown) {
    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        return false;
    }

    SpellRuneCostRec* cost = g_spellRuneCostDB.GetRecord(spell->m_runeCostID);
    int32_t needed[4] = { 0, 0, 0, 0 };

    if (!cost || CGRuneInfo::HasRunesForSpell(spell->m_id, needed)) {
        return false;
    }

    uint32_t latest = 0;
    int32_t latestIndex = 0;
    uint32_t times[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    for (int32_t i = 0; i < 8; i++) {
        int32_t type = CGRuneInfo::m_runes[i];

        if (type != 3 && needed[type]) {
            uint32_t time = CGRuneInfo::GetRuneRechargeTime(i);
            times[i] = time;

            if (!latest || static_cast<int32_t>(time - latest) >= 0) {
                latest = time;
                latestIndex = i;
            }
        }
    }

    for (int32_t i = 0; i < 8; i++) {
        if (CGRuneInfo::m_runes[i] != 3 || !ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__) || ((1 << i) & CGRuneInfo::m_predictedRunes) || !CGRuneInfo::m_runeCooldowns[i]) {
            continue;
        }

        uint32_t time = CGRuneInfo::GetRuneRechargeTime(i);

        if (!latest) {
            latest = time;
            latestIndex = i;
            times[i] = time;
        } else if (time < latest) {
            times[latestIndex] = CGRuneInfo::GetRuneRechargeTime(i);
            latestIndex = 0;
            latest = 0;

            for (int32_t j = 0; j < 8; j++) {
                if (CGRuneInfo::m_runes[j] != 3 && (!latest || static_cast<int32_t>(times[j] - latest) >= 0)) {
                    latest = times[j];
                    latestIndex = j;
                }
            }
        }
    }

    uint32_t blood = 0;

    if (needed[0] == 1) {
        if (!times[0] || (times[1] && times[0] >= times[1])) {
            blood = times[1];
        } else {
            blood = times[0];
        }
    } else if (needed[0] == 2) {
        blood = times[0] <= times[1] ? times[1] : times[0];
    }

    uint32_t unholy = 0;

    if (needed[1] == 1) {
        unholy = times[2] && (!times[3] || times[2] < times[3]) ? times[2] : times[3];
    } else if (needed[1] == 2) {
        unholy = times[2] > times[3] ? times[2] : times[3];
    }

    uint32_t frost = 0;

    if (needed[2] == 1) {
        frost = times[4] && (!times[5] || times[4] < times[5]) ? times[4] : times[5];
    } else if (needed[2] == 2) {
        frost = times[4] > times[5] ? times[4] : times[5];
    }

    uint32_t other = unholy <= frost ? frost : unholy;
    *cooldown = blood <= other ? other : blood;
    return true;
}

// VERIFY: 31a366a2 auto
// OFFSET: 0x005EEB70
void CGRuneInfo::ShutdownGame() {
}

// VERIFY: 8a4f7690 auto
// OFFSET: 0x005EDFA0
void CGRuneInfo::SetRuneLastUsedTime(int32_t index, uint32_t time) {
    CGRuneInfo::m_runeLastUsed[index] = time;
}

// VERIFY: 89906422 manual
// OFFSET: 0x005EE110
uint32_t CGRuneInfo::GetRuneLastUsedTime(int32_t index) {
    if (!ClntObjMgrGetActivePlayerObj()) {
        return 0;
    }

    return CGRuneInfo::m_runeLastUsed[index];
}

// VERIFY: 0b8f3a32 manual
// OFFSET: 0x005EE140
void CGRuneInfo::ComputeRuneRechargeTime(int32_t index, float progress) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    float rate = player->m_player->m_playerRuneRegen1[CGRuneInfo::m_runes[index]];

    if (rate >= 0.0099999998f) {
        CGRuneInfo::m_runeCooldowns[index] = OsGetAsyncTimeMs() - static_cast<int32_t>(1.0f / rate * (1.0f - progress) * -1000.0f);
    } else {
        CGRuneInfo::m_runeCooldowns[index] = 0;
    }
}

// VERIFY: 4de899c0 auto
// OFFSET: 0x005EE020
void CGRuneInfo::ConvertRune(uint8_t rune, uint8_t type) {
    CGRuneInfo::m_runes[rune] = type;
    FrameScript_SignalEvent(SCRIPT_EVENT_RUNE_TYPE_UPDATE, "%d", rune + 1);
    CGSpellBook::UpdateUsable();
}

// VERIFY: c2cf4661 manual
// OFFSET: 0x005EE240
void CGRuneInfo::SetPredictedRunes(uint8_t available, uint8_t used, const uint8_t* cooldowns) {
    if (!ClntObjMgrGetActivePlayerObj()) {
        return;
    }

    CGRuneInfo::m_predictedRunes = used;

    for (int32_t i = 0; i < 8; i++) {
        int32_t bit = 1 << i;

        if ((available & bit) && !(used & bit)) {
            if (!CGRuneInfo::GetRuneLastUsedTime(i)) {
                float progress = 0.0f;

                if (cooldowns) {
                    progress = cooldowns[i] * 0.0039215689f;

                    if (progress > 1.0f) {
                        progress = 1.0f;
                    }
                }

                CGRuneInfo::m_runeLastUsed[i] = OsGetAsyncTimeMs() - static_cast<int32_t>(10000.0f * progress);
                CGRuneInfo::ComputeRuneRechargeTime(i, progress);
                FrameScript_SignalEvent(SCRIPT_EVENT_RUNE_POWER_UPDATE, "%d%b", i + 1, 0);
            }
        } else if ((!(available & bit) && (used & bit)) || ((CGRuneInfo::m_predictedRunes & bit) && CGRuneInfo::GetRuneLastUsedTime(i))) {
            CGRuneInfo::m_runeLastUsed[i] = 0;
            CGRuneInfo::m_runeCooldowns[i] = 0;
            FrameScript_SignalEvent(SCRIPT_EVENT_RUNE_POWER_UPDATE, "%d%b", i + 1, 1);
        }
    }

    CGSpellBook::UpdateUsable();
}

// VERIFY: ff3c5eda auto
// OFFSET: 0x005EE440
void CGRuneInfo::ResyncRunes(CDataStore* msg) {
    msg->Get(CGRuneInfo::m_numRunes);

    for (int32_t i = 0; i < static_cast<int32_t>(CGRuneInfo::m_numRunes); i++) {
        uint8_t type;
        msg->Get(type);
        CGRuneInfo::m_runes[i] = type;
        CGRuneInfo::m_baseRunes[i] = type;
        uint8_t cooldown;
        msg->Get(cooldown);
        float progress = cooldown * 0.0039215689f;

        if (progress > 1.0f) {
            progress = 1.0f;
        }

        CGRuneInfo::m_runeLastUsed[i] = OsGetAsyncTimeMs() - static_cast<int32_t>(10000.0f * progress);
        CGRuneInfo::ComputeRuneRechargeTime(i, progress);
        FrameScript_SignalEvent(SCRIPT_EVENT_RUNE_POWER_UPDATE, "%d%b", i + 1, 0);
    }
}

// VERIFY: a086b4eb auto
// OFFSET: 0x005EE520
void CGRuneInfo::AddRunePower(CDataStore* msg) {
    uint32_t runes;
    msg->Get(runes);

    for (int32_t i = 0; i < static_cast<int32_t>(CGRuneInfo::m_numRunes); i++) {
        if (((1 << i) & runes) && !((1 << i) & CGRuneInfo::m_predictedRunes)) {
            CGRuneInfo::m_runeLastUsed[i] = OsGetAsyncTimeMs() - 10000;
            CGRuneInfo::ComputeRuneRechargeTime(i, 1.0f);
            FrameScript_SignalEvent(SCRIPT_EVENT_RUNE_POWER_UPDATE, "%d%b", i + 1, 0);
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_ACTIONBAR_UPDATE_COOLDOWN, nullptr);
}

// VERIFY: 59e48eda auto
// OFFSET: 0x005EEB70
void CGRuneInfo::LeaveWorld() {
}
