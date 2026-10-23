#ifndef WOW_SOURCE_OBJECT_PLAYER_H
#define WOW_SOURCE_OBJECT_PLAYER_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/ObjectClient/VisibleItem_C.h"
#include "WoW/Source/Object/WGUID.h"

class CGPlayerLocalData;

struct PlayerQuestLogEntry {
    /* 0000 */ uint32_t m_questId;
    /* 0004 */ uint32_t m_state;
    /* 0008 */ uint16_t m_counts [4];
    /* 0010 */ uint32_t m_time;
};

struct PlayerSkillInfo {
    /* 0000 */ uint16_t m_skillId;
    /* 0002 */ uint16_t m_step;
    /* 0004 */ uint16_t m_rank;
    /* 0006 */ uint16_t m_maxRank;
    /* 0008 */ uint16_t m_tempBonus;
    /* 000A */ uint16_t m_permBonus;
};

class CGPlayer {
    public:
    PlayerQuestLogEntry* GetQuestLogEntry(uint32_t slot);
    uint32_t GetBuybackPrice(uint32_t slot);
    bool HasPvpMedal(uint32_t medal);
    bool HasAuraVision(uint8_t vision);
    static const uint32_t s_mirrorFields[173];
    static const uint32_t s_localMirrorFields[870];

    static uint32_t DescriptorToMirrorOffset(uint32_t offset, uint32_t size, int32_t local, uint32_t base);
    static uint32_t GetSavedIndexFromDataIndex(uint32_t index, uint32_t count, int32_t local);
    static int32_t TotalFields();
    static int32_t TotalRemoteFields();

    /* 0000 */ CGPlayerLocalData* m_player;
    /* 0004 */ void* m_playerMirror;

    float GetDrunknessTurnValue();
    int32_t HasZoneExplored(int32_t zone);
    bool IsDrunk();
};

class CGPlayerData {
    public:
    /* 0000 */ WOWGUID m_playerDuelArbiter;
    /* 0008 */ uint32_t m_playerFlags;
    /* 000C */ uint32_t m_playerGuildid;
    /* 0010 */ uint32_t m_playerGuildrank;
    /* 0014 */ uint8_t m_playerBytes [4];
    /* 0018 */ uint8_t m_playerBytes2 [4];
    /* 001C */ uint8_t m_playerBytes3 [4];
    /* 0020 */ uint32_t m_playerDuelTeam;
    /* 0024 */ uint32_t m_playerGuildTimestamp;
    /* 0028 */ PlayerQuestLogEntry m_playerQuestLog [25];
    /* 021C */ VisibleItem_C m_playerVisibleItems [19];
    /* 02B4 */ uint32_t m_playerChosenTitle;
    /* 02B8 */ int32_t m_playerFakeInebriation;
    /* 02BC */ uint32_t m_playerFieldPad0;
};

class CGPlayerLocalData : public CGPlayerData {
    public:
    /* 02C0 */ WOWGUID m_playerFieldInvSlotHead [23];
    /* 0378 */ WOWGUID m_playerFieldPackSlot1 [16];
    /* 03F8 */ WOWGUID m_playerFieldBankSlot1 [28];
    /* 04D8 */ WOWGUID m_playerFieldBankbagSlot1 [7];
    /* 0510 */ WOWGUID m_playerFieldVendorbuybackSlot1 [12];
    /* 0570 */ WOWGUID m_playerFieldKeyringSlot1 [32];
    /* 0670 */ WOWGUID m_playerFieldCurrencytokenSlot1 [32];
    /* 0770 */ WOWGUID m_playerFarsight;
    /* 0778 */ uint64_t m_playerFieldKnownTitles [3];
    /* 0790 */ WOWGUID m_playerFieldKnownCurrencies;
    /* 0798 */ uint32_t m_playerXp;
    /* 079C */ uint32_t m_playerNextLevelXp;
    /* 07A0 */ PlayerSkillInfo m_playerSkillInfo [128];
    /* 0DA0 */ int32_t m_playerCharacterPoints [2];
    /* 0DA8 */ uint32_t m_playerTrackCreatures;
    /* 0DAC */ uint32_t m_playerTrackResources;
    /* 0DB0 */ float m_playerBlockPercentage;
    /* 0DB4 */ float m_playerDodgePercentage;
    /* 0DB8 */ float m_playerParryPercentage;
    /* 0DBC */ uint32_t m_playerExpertise;
    /* 0DC0 */ uint32_t m_playerOffhandExpertise;
    /* 0DC4 */ float m_playerCritPercentage;
    /* 0DC8 */ float m_playerRangedCritPercentage;
    /* 0DCC */ float m_playerOffhandCritPercentage;
    /* 0DD0 */ float m_playerSpellCritPercentage1 [7];
    /* 0DEC */ uint32_t m_playerShieldBlock;
    /* 0DF0 */ float m_playerShieldBlockCritPercentage;
    /* 0DF4 */ uint8_t m_playerExploredZones1 [512];
    /* 0FF4 */ uint32_t m_playerRestStateExperience;
    /* 0FF8 */ uint32_t m_playerFieldCoinage;
    /* 0FFC */ uint32_t m_playerFieldModDamageDonePos [7];
    /* 1018 */ int32_t m_playerFieldModDamageDoneNeg [7];
    /* 1034 */ float m_playerFieldModDamageDonePct [7];
    /* 1050 */ uint32_t m_playerFieldModHealingDonePos;
    /* 1054 */ float m_playerFieldModHealingPct;
    /* 1058 */ float m_playerFieldModHealingDonePct;
    /* 105C */ uint32_t m_playerFieldModTargetResistance;
    /* 1060 */ uint32_t m_playerFieldModTargetPhysicalResistance;
    /* 1064 */ uint8_t m_playerFieldBytes [4];
    /* 1068 */ uint32_t m_playerAmmoId;
    /* 106C */ uint32_t m_playerSelfResSpell;
    /* 1070 */ uint32_t m_playerFieldPvpMedals;
    /* 1074 */ uint32_t m_playerFieldBuybackPrice1 [12];
    /* 10A4 */ uint32_t m_playerFieldBuybackTimestamp1 [12];
    /* 10D4 */ uint16_t m_playerFieldKills [2];
    /* 10D8 */ uint32_t m_playerFieldTodayContribution;
    /* 10DC */ uint32_t m_playerFieldYesterdayContribution;
    /* 10E0 */ uint32_t m_playerFieldLifetimeHonorbaleKills;
    /* 10E4 */ uint32_t m_playerFieldBytes2;
    /* 10E8 */ uint32_t m_playerFieldWatchedFactionIndex;
    /* 10EC */ uint32_t m_playerFieldCombatRating1 [25];
    /* 1150 */ uint32_t m_playerFieldArenaTeamInfo11 [21];
    /* 11A4 */ uint32_t m_playerFieldHonorCurrency;
    /* 11A8 */ uint32_t m_playerFieldArenaCurrency;
    /* 11AC */ uint32_t m_playerFieldMaxLevel;
    /* 11B0 */ uint32_t m_playerFieldDailyQuests1 [25];
    /* 1214 */ float m_playerRuneRegen1 [4];
    /* 1224 */ uint32_t m_playerNoReagentCost1 [3];
    /* 1230 */ uint32_t m_playerFieldGlyphSlots1 [6];
    /* 1248 */ uint32_t m_playerFieldGlyphs1 [6];
    /* 1260 */ uint32_t m_playerGlyphsEnabled;
    /* 1264 */ uint32_t m_playerPetSpellPower;
};

#endif
