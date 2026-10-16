#ifndef WOW_SOURCE_OBJECT_QUESTCACHE_H
#define WOW_SOURCE_OBJECT_QUESTCACHE_H

#include <cstddef>
#include <cstdint>

class CDataStore;

class QuestCache {
    public:
    static const uint32_t s_cacheVersion = 3;

    /* 0000 */ int32_t m_id;
    /* 0004 */ uint32_t m_method;
    /* 0008 */ uint32_t m_level;
    /* 000C */ uint32_t m_requiredLevel;
    /* 0010 */ uint32_t m_areaIdOrSortId;
    /* 0014 */ uint32_t m_infoId;
    /* 0018 */ uint32_t m_suggestedPlayers;
    /* 001C */ uint32_t m_friendlyFactionId;
    /* 0020 */ uint32_t m_friendlyFactionAmount;
    /* 0024 */ uint32_t m_hostileFactionId;
    /* 0028 */ uint32_t m_hostileFactionAmount;
    /* 002C */ uint32_t m_nextQuestId;
    /* 0030 */ uint32_t m_xPId;
    /* 0034 */ int32_t m_rewardMoney;
    /* 0038 */ uint32_t m_rewardMoneyInsteadOfXp;
    /* 003C */ uint32_t m_rewardSpellId;
    /* 0040 */ uint32_t m_effectOnPlayer;
    /* 0044 */ uint32_t m_rewardHonor;
    /* 0048 */ float m_rewardHonorBonus;
    /* 004C */ uint32_t m_startingItemId;
    /* 0050 */ uint32_t m_flags;
    /* 0054 */ uint32_t m_rewardItem[4];
    /* 0064 */ uint32_t m_rewardItemCount[4];
    /* 0074 */ uint32_t m_rewardChoiceItem[6];
    /* 008C */ uint32_t m_rewardChoiceItemCount[6];
    /* 00A4 */ uint32_t m_pointMapId;
    /* 00A8 */ float m_pointX;
    /* 00AC */ float m_pointY;
    /* 00B0 */ uint32_t m_pointOptional;
    /* 00B4 */ char m_name[512];
    /* 02B4 */ char m_objText[3000];
    /* 0E6C */ char m_description[3000];
    /* 1A24 */ char m_endText[512];
    /* 1C24 */ uint32_t m_objectiveId[4];
    /* 1C34 */ uint32_t m_objectiveRequiredCount[4];
    /* 1C44 */ uint32_t m_collectItemId[6];
    /* 1C5C */ uint32_t m_collectItemCount[6];
    /* 1C74 */ uint32_t m_intermediateItemId[4];
    /* 1C84 */ uint32_t m_intermediateItemCount[4];
    /* 1C94 */ char m_objectiveText[4][256];
    /* 2094 */ uint32_t m_rewardTitleId;
    /* 2098 */ uint32_t m_requiredPlayersKilled;
    /* 209C */ uint32_t m_rewardTalentPoints;
    /* 20A0 */ uint32_t m_rewardArenaPoints;
    /* 20A4 */ char m_completionText[2048];
    /* 28A4 */ uint32_t m_rewardReputationFaction[5];
    /* 28B8 */ uint32_t m_factionRewardId[5];
    /* 28CC */ int32_t m_rewardReputationOverride[5];
    /* 28E0 */ uint32_t m_rewardReputationMask;

    QuestCache();
    QuestCache& operator=(const QuestCache& source);
    void Pack(CDataStore* msg);
    void Unpack(CDataStore* msg);
};

#endif
