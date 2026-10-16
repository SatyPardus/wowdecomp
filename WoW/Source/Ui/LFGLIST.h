#ifndef WOW_SOURCE_UI_LFGLIST_H
#define WOW_SOURCE_UI_LFGLIST_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/Ui/LFGListParty.h"

class LFGLIST {
    public:
    /* 0000 */ uint32_t m_lfgID;
    /* 0004 */ uint32_t m_time;
    /* 0008 */ uint32_t m_totalPlayers;
    /* 000C */ uint32_t m_totalParties;
    /* 0010 */ uint8_t unused_0010;
    /* 0014 */ uint32_t m_pendingNames;
    /* 0018 */ TSHashTable<LFGListParty, CHashKeyGUID> m_parties;
    /* 0040 */ TSHashTable<LFGListPlayer, CHashKeyGUID> m_players;
    /* 0068 */ TSGrowableArray<LFGListParty*> m_partyList;
    /* 0078 */ TSGrowableArray<LFGListPlayer*> m_playerList;

    ~LFGLIST();
    void Clear();
    bool Copy(LFGLIST& source);
    void DeleteParty(LFGListParty* party);
    void Reindex();
    void RemoveParty(LFGListParty* party);
    void RemovePlayer(LFGListPlayer* player);
};

void LFGNameCacheCallback(int32_t id, const WOWGUID& guid, void* param, bool success);

#endif
