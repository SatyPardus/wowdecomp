#include "DBCacheInstances.h"
#include <cstring>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/DB/DBClient/CreatureStats_C.h"
#include "WoW/Source/DB/DBClient/GameObjectStats_C.h"
#include "WoW/Source/Game/GameClient/PetNameCache.h"
#include "WoW/Source/Game/GameClient/WardenClient.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/ArenaTeamCache.h"
#include "WoW/Source/Object/CGPetition.h"
#include "WoW/Source/Object/DanceCache.h"
#include "WoW/Source/Object/ItemNameCache.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ItemTextCache.h"
#include "WoW/Source/Object/NPCText.h"
#include "WoW/Source/Object/ObjectClient/GuildStats_C.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/Object/PageTextCache.h"
#include "WoW/Source/Object/QuestCache.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/DB/DBClient/DBClient.h"

#ifdef DISABLE_ITEM_CACHE
static constexpr bool ITEM_CACHE_ENABLED = false;
#else
static constexpr bool ITEM_CACHE_ENABLED = true;
#endif

// STATIC: 0x00C5D690
DBCache<CreatureStats_C, int32_t, HASHKEY_INT> g_creatureDBCache('WMOB', "creaturecache.wdb", CMSG_QUERY_CREATURE, 0, true, true, 0);

// STATIC: 0x00C5D718
DBCache<GameObjectStats_C, int32_t, HASHKEY_INT> g_gameObjectDBCache('WGOB', "gameobjectcache.wdb", CMSG_QUERY_GAME_OBJECT, 0, true, true, 0);

// STATIC: 0x00C5D7A0
DBCache<ItemNameCache_C, int32_t, HASHKEY_INT> g_itemNameDBCache('WNDB', "itemnamecache.wdb", CMSG_ITEM_NAME_QUERY, 0, true, ITEM_CACHE_ENABLED, 0);

// STATIC: 0x00C5D828
DBCache<ItemStats_C, int32_t, HASHKEY_INT> g_itemDBCache('WIDB', "itemcache.wdb", CMSG_ITEM_QUERY_SINGLE, CMSG_ITEM_QUERY_MULTIPLE, false, ITEM_CACHE_ENABLED, 512);

// STATIC: 0x00C5D8B0
DBCache<NPCText, int32_t, HASHKEY_INT> g_npcTextDBCache('WNPC', "npccache.wdb", CMSG_QUERY_NPC_TEXT, 0, true, true, 0);

// STATIC: 0x00C5D938
DBCache<NameCache, uint64_t, CHashKeyGUID> g_nameDBCache('WNAM', "namecache.wdb", CMSG_NAME_QUERY, 0, false, false, 256);

// STATIC: 0x00C5D9C0
DBCache<GuildStats_C, int32_t, HASHKEY_INT> g_guildInfoCache('WGLD', "guildcache.wdb", CMSG_QUERY_GUILD_INFO, 0, false, false, 0);

// STATIC: 0x00C5DA48
DBCache<QuestCache, int32_t, HASHKEY_INT> g_questDBCache('WQST', "questcache.wdb", CMSG_QUERY_QUEST_INFO, 0, false, true, 0);

// STATIC: 0x00C5DAD0
DBCache<PageTextCache_C, int32_t, HASHKEY_INT> g_pageTextCache('WPTX', "pagetextcache.wdb", CMSG_QUERY_PAGE_TEXT, 0, true, true, 0);

// STATIC: 0x00C5DB58
DBCache<PetNameCache, int32_t, HASHKEY_INT> g_petNameCache('WPNM', "petnamecache.wdb", CMSG_QUERY_PET_NAME, 0, true, false, 0);

// STATIC: 0x00C5DBE0
DBCache<CGPetition, int32_t, HASHKEY_INT> g_petitionCache('WPTN', "petitioncache.wdb", CMSG_PETITION_QUERY, 0, true, false, 0);

// STATIC: 0x00C5DC68
DBCache<ItemTextCache_C, uint64_t, CHashKeyGUID> g_itemTextDBCache('WITX', "itemtextcache.wdb", CMSG_ITEM_TEXT_QUERY, 0, false, true, 0);

// STATIC: 0x00C5DCF0
DBCache<WardenCachedModule, CWardenKey, CWardenKey> g_wardenCache('WRDN', "wowcache.wdb", 0, 0, true, true, 0);

// STATIC: 0x00C5DD78
DBCache<ArenaTeamCache, int32_t, HASHKEY_INT> g_arenaTeamCache('WATM', "arenateamcache.wdb", CMSG_ARENA_TEAM_QUERY, 0, false, false, 0);

// STATIC: 0x00C5DE00
DBCache<DanceCache, int32_t, HASHKEY_INT> g_danceCache('WDAN', "dancecache.wdb", CMSG_DANCE_QUERY, 0, false, false, 0);

// VERIFY: 404fe8cb auto
// OFFSET: 0x00635060
static void LoadDBCaches() {
    g_creatureDBCache.Load();
    g_gameObjectDBCache.Load();
    g_itemDBCache.Load();
    g_itemNameDBCache.Load();
    g_npcTextDBCache.Load();
    g_nameDBCache.Load();
    g_guildInfoCache.Load();
    g_questDBCache.Load();
    g_pageTextCache.Load();
    g_petNameCache.Load();
    g_petitionCache.Load();
    g_itemTextDBCache.Load();
    g_wardenCache.Load();
    g_arenaTeamCache.Load();
    g_danceCache.Load();
}

// VERIFY: aae02c55 auto
// OFFSET: 0x00635190
static int32_t ReceiveCreature(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    g_creatureDBCache.AddItems(msg, true);

    return 1;
}

// VERIFY: 6b3f4f7f auto
// OFFSET: 0x006351B0
static int32_t ReceiveGameObject(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    g_gameObjectDBCache.AddItems(msg, true);

    return 1;
}

// VERIFY: ba1d0f7e auto
// OFFSET: 0x006351D0
static int32_t ReceiveItemName(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    g_itemNameDBCache.AddItems(msg, true);

    return 1;
}

// VERIFY: 1cb3f926 auto
// OFFSET: 0x006351F0
static int32_t ReceiveMultipleItems(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    g_itemDBCache.AddItems(msg, false);

    return 1;
}

// VERIFY: 97876dbd auto
// OFFSET: 0x00635210
static int32_t ReceiveNPCText(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    g_npcTextDBCache.AddItems(msg, true);

    return 1;
}

// VERIFY: 798f40eb auto
// OFFSET: 0x00635230
static int32_t ReceiveQuest(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    QuestCache quest;
    quest.Unpack(msg);

    if (quest.m_name[0]) {
        g_questDBCache.AddItem(&quest, quest.m_id);
    } else {
        g_questDBCache.DenyItem(quest.m_id);
    }

    return 1;
}

// VERIFY: 5a0cf716 auto
// OFFSET: 0x006352A0
static int32_t ReceivePageText(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    g_pageTextCache.AddItems(msg, true);

    return 1;
}

// VERIFY: 13092814 manual
// OFFSET: 0x006352C0
static int32_t ReceivePetName(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    PetNameCache petName;
    msg->Get(petName.m_petNumber);
    msg->GetString(petName.m_name, sizeof(petName.m_name));
    msg->Get(petName.m_timestamp);

    uint8_t hasDeclinedNames;
    msg->Get(hasDeclinedNames);

    if (hasDeclinedNames) {
        petName.m_declinedNames = new (__FILE__, __LINE__) PetNameCache::DeclinedNames;

        for (uint32_t i = 0; i < sizeof(petName.m_declinedNames->m_names) / sizeof(petName.m_declinedNames->m_names[0]); i++) {
            msg->GetString(petName.m_declinedNames->m_names[i], sizeof(petName.m_declinedNames->m_names[i]));
        }
    }

    if (petName.m_name[0]) {
        g_petNameCache.AddItem(&petName, petName.m_petNumber);
    } else {
        g_petNameCache.DenyItem(petName.m_petNumber);
    }

    return 1;
}

// VERIFY: 5064d6d6 auto
// OFFSET: 0x00635390
static int32_t ReceivePetition(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGPetition petition;
    petition.Unpack(msg);

    if (petition.m_id > 0) {
        g_petitionCache.AddItem(&petition, petition.m_id);
    } else {
        g_petitionCache.DenyItem(-petition.m_id);
    }

    return 1;
}

// VERIFY: b919f464 auto
// OFFSET: 0x00635400
static int32_t ReceiveInvalidatePlayer(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);

    if ((guid.m_guid & 0xF000000000000000ULL) == 0x1000000000000000ULL && (guid.m_guid & 0x0FF0000000000000ULL) == 0x0FF0000000000000ULL) {
        g_guildInfoCache.Invalidate(static_cast<int32_t>(guid.m_guid));
    } else {
        g_nameDBCache.Invalidate(guid.m_guid);
        CGGameUI::InvalidatePlayer(guid);
    }

    return 1;
}

// VERIFY: 79eadcd8 auto
// OFFSET: 0x00635480
static int32_t ReceiveArenaTeam(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    ArenaTeamCache arenaTeam;
    arenaTeam.Unpack(msg);

    if (arenaTeam.m_name[0]) {
        g_arenaTeamCache.AddItem(&arenaTeam, arenaTeam.m_id);
    } else {
        g_arenaTeamCache.DenyItem(arenaTeam.m_id);
    }

    return 1;
}

// VERIFY: 00011d21 auto
// OFFSET: 0x006354D0
static int32_t ReceiveSingleItem(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    g_itemDBCache.AddItems(msg, true);

    return 1;
}

// VERIFY: 2b1fc0ca auto
// OFFSET: 0x006354F0
static int32_t ReceiveInvalidateDance(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t danceID;
    msg->Get(danceID);
    g_danceCache.Invalidate(danceID);

    return 1;
}

// VERIFY: aed6f77d auto
// OFFSET: 0x00635710
void SetClientCacheVersion(uint32_t version) {
    g_creatureDBCache.VerifyAndUpdateVersion(version);
    g_creatureDBCache.VerifyAndUpdateVersion(version);
    g_gameObjectDBCache.VerifyAndUpdateVersion(version);
    g_itemDBCache.VerifyAndUpdateVersion(version);
    g_itemNameDBCache.VerifyAndUpdateVersion(version);
    g_npcTextDBCache.VerifyAndUpdateVersion(version);
    g_nameDBCache.VerifyAndUpdateVersion(version);
    g_guildInfoCache.VerifyAndUpdateVersion(version);
    g_questDBCache.VerifyAndUpdateVersion(version);
    g_pageTextCache.VerifyAndUpdateVersion(version);
    g_petNameCache.VerifyAndUpdateVersion(version);
    g_petitionCache.VerifyAndUpdateVersion(version);
    g_itemTextDBCache.VerifyAndUpdateVersion(version);
    g_wardenCache.VerifyAndUpdateVersion(version);
    g_arenaTeamCache.VerifyAndUpdateVersion(version);
    g_danceCache.VerifyAndUpdateVersion(version);
}

// VERIFY: 33bf72e2 manual
// OFFSET: 0x006357D0
static int32_t ReceiveName(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    NameCache name;
    SmartGUID guid;
    *msg >> guid;
    name.m_guid.m_guid = guid.m_guid;

    uint8_t result;
    msg->Get(result);

    switch (result) {
        case 0: {
            msg->GetString(name.m_name, sizeof(name.m_name));
            msg->GetString(name.m_realmName, sizeof(name.m_realmName));

            uint8_t race;
            uint8_t sex;
            uint8_t classID;
            msg->Get(race);
            msg->Get(sex);
            msg->Get(classID);
            name.m_race = race;
            name.m_sex = sex;
            name.m_class = classID;

            uint8_t hasDeclinedNames;
            msg->Get(hasDeclinedNames);

            if (hasDeclinedNames) {
                name.m_declinedNames = new (__FILE__, __LINE__) NameCache::DeclinedNames;

                for (uint32_t i = 0; i < sizeof(name.m_declinedNames->m_names) / sizeof(name.m_declinedNames->m_names[0]); i++) {
                    msg->GetString(name.m_declinedNames->m_names[i], sizeof(name.m_declinedNames->m_names[i]));
                }
            }

            g_nameDBCache.AddItem(&name, name.m_guid.m_guid);

            if ((name.m_guid.m_guid & 0xF000000000000000ULL) == 0x1000000000000000ULL && (name.m_guid.m_guid & 0x0FF0000000000000ULL) == 0x0FD0000000000000ULL) {
                g_nameDBCache.SetTemporary(name.m_guid.m_guid);
            }

            break;
        }

        case 2:
            g_nameDBCache.RetryItem(name.m_guid.m_guid);
            break;

        case 3:
            SStrCopy(name.m_name, "?", sizeof(name.m_name));
            memset(name.m_realmName, 0, sizeof(name.m_realmName));
            name.m_race = 0;
            name.m_sex = 0;
            name.m_class = 0;
            name.m_declinedNames = nullptr;
            g_nameDBCache.AddItem(&name, name.m_guid.m_guid);
            g_nameDBCache.SetTemporary(name.m_guid.m_guid);
            break;

        default:
            g_nameDBCache.DenyItem(name.m_guid.m_guid);
            break;
    }

    return 1;
}

// VERIFY: 79263b94 auto
// OFFSET: 0x006359E0
static int32_t ReceiveGuildInfo(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    GuildStats_C guild;
    guild.Unpack(msg);

    if (guild.m_name[0]) {
        g_guildInfoCache.AddItem(&guild, guild.m_id);
    } else {
        g_guildInfoCache.DenyItem(guild.m_id);
    }

    return 1;
}

// VERIFY: 205d9c3c auto
// OFFSET: 0x00635A40
static int32_t ReceiveItemText(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    ItemTextCache_C itemText;

    uint8_t result;
    msg->Get(result);

    if (!result) {
        WOWGUID itemGuid;
        msg->Get(itemGuid.m_guid);
        msg->GetString(itemText.m_text, sizeof(itemText.m_text));
        g_itemTextDBCache.AddItem(&itemText, itemGuid.m_guid);
    }

    return 1;
}

// VERIFY: 8d62794a manual
// OFFSET: 0x00635AB0
static int32_t ReceiveDance(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    DanceCache dance;
    dance.Unpack(msg);

    if (dance.m_id & 0x80000000) {
        g_danceCache.DenyItem(dance.m_id & 0x7FFFFFFF);
    } else {
        g_danceCache.AddItem(&dance, dance.m_id);
    }

    return 1;
}

// VERIFY: b8ba4c49 auto
// OFFSET: 0x00635520
void DBCache_Initialize() {
    LoadDBCaches();
}

// VERIFY: e22c2ee3 manual
// OFFSET: 0x00635540
void DBCache_Destroy() {
    g_creatureDBCache.Destroy();
    g_gameObjectDBCache.Destroy();
    g_itemDBCache.Destroy();
    g_itemNameDBCache.Destroy();
    g_npcTextDBCache.Destroy();
    g_nameDBCache.Destroy();
    g_guildInfoCache.Destroy();
    g_questDBCache.Destroy();
    g_pageTextCache.Destroy();
    g_petNameCache.Destroy();
    g_petitionCache.Destroy();
    g_itemTextDBCache.Destroy();
    g_wardenCache.Destroy();
    g_arenaTeamCache.Destroy();
    g_danceCache.Destroy();
}

// VERIFY: 10083554 auto
// OFFSET: 0x006355E0
void DBCache_ClearHandlers() {
    ClientServices::ClearMessageHandler(SMSG_QUERY_CREATURE_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_QUERY_GAME_OBJECT_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_ITEM_NAME_QUERY_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_ITEM_QUERY_SINGLE_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_ITEM_QUERY_MULTIPLE_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_QUERY_NPC_TEXT_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_QUERY_PLAYER_NAME_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_QUERY_GUILD_INFO_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_QUERY_QUEST_INFO_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_QUERY_PAGE_TEXT_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_QUERY_PET_NAME_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_PETITION_QUERY_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_QUERY_ITEM_TEXT_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_INVALIDATE_PLAYER);
    ClientServices::ClearMessageHandler(SMSG_ARENA_TEAM_QUERY_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_CHEAT_DUMP_ITEMS_DEBUG_ONLY_RESPONSE);
    ClientServices::ClearMessageHandler(SMSG_INVALIDATE_DANCE);
    ClientServices::ClearMessageHandler(SMSG_DANCE_QUERY_RESPONSE);
}

// VERIFY: 00e71ce2 auto
// OFFSET: 0x00635B40
void DBCache_RegisterHandlers() {
    ClientServices::SetMessageHandler(SMSG_QUERY_CREATURE_RESPONSE, ReceiveCreature, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUERY_GAME_OBJECT_RESPONSE, ReceiveGameObject, nullptr);
    ClientServices::SetMessageHandler(SMSG_ITEM_NAME_QUERY_RESPONSE, ReceiveItemName, nullptr);
    ClientServices::SetMessageHandler(SMSG_ITEM_QUERY_SINGLE_RESPONSE, ReceiveSingleItem, nullptr);
    ClientServices::SetMessageHandler(SMSG_ITEM_QUERY_MULTIPLE_RESPONSE, ReceiveMultipleItems, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUERY_NPC_TEXT_RESPONSE, ReceiveNPCText, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUERY_PLAYER_NAME_RESPONSE, ReceiveName, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUERY_GUILD_INFO_RESPONSE, ReceiveGuildInfo, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUERY_QUEST_INFO_RESPONSE, ReceiveQuest, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUERY_PAGE_TEXT_RESPONSE, ReceivePageText, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUERY_PET_NAME_RESPONSE, ReceivePetName, nullptr);
    ClientServices::SetMessageHandler(SMSG_PETITION_QUERY_RESPONSE, ReceivePetition, nullptr);
    ClientServices::SetMessageHandler(SMSG_QUERY_ITEM_TEXT_RESPONSE, ReceiveItemText, nullptr);
    ClientServices::SetMessageHandler(SMSG_INVALIDATE_PLAYER, ReceiveInvalidatePlayer, nullptr);
    ClientServices::SetMessageHandler(SMSG_ARENA_TEAM_QUERY_RESPONSE, ReceiveArenaTeam, nullptr);
    ClientServices::SetMessageHandler(SMSG_CHEAT_DUMP_ITEMS_DEBUG_ONLY_RESPONSE, ReceiveSingleItem, nullptr);
    ClientServices::SetMessageHandler(SMSG_INVALIDATE_DANCE, ReceiveInvalidateDance, nullptr);
    ClientServices::SetMessageHandler(SMSG_DANCE_QUERY_RESPONSE, ReceiveDance, nullptr);
}

// VERIFY: 4b275620 auto
// OFFSET: 0x00635C70
void DBCache_ClearTemporaryCaches() {
    if (!g_creatureDBCache.m_enabled) {
        g_creatureDBCache.Clear();
    }

    if (!g_gameObjectDBCache.m_enabled) {
        g_gameObjectDBCache.Clear();
    }

    if (!g_itemDBCache.m_enabled) {
        g_itemDBCache.Clear();
    }

    if (!g_itemNameDBCache.m_enabled) {
        g_itemNameDBCache.Clear();
    }

    if (!g_npcTextDBCache.m_enabled) {
        g_npcTextDBCache.Clear();
    }

    if (!g_nameDBCache.m_enabled) {
        g_nameDBCache.Clear();
    }

    if (!g_guildInfoCache.m_enabled) {
        g_guildInfoCache.Clear();
    }

    if (!g_questDBCache.m_enabled) {
        g_questDBCache.Clear();
    }

    if (!g_pageTextCache.m_enabled) {
        g_pageTextCache.Clear();
    }

    if (!g_petNameCache.m_enabled) {
        g_petNameCache.Clear();
    }

    if (!g_petitionCache.m_enabled) {
        g_petitionCache.Clear();
    }

    if (!g_itemTextDBCache.m_enabled) {
        g_itemTextDBCache.Clear();
    }

    if (!g_wardenCache.m_enabled) {
        g_wardenCache.Clear();
    }

    if (!g_arenaTeamCache.m_enabled) {
        g_arenaTeamCache.Clear();
    }

    if (!g_danceCache.m_enabled) {
        g_danceCache.Clear();
    }
}

// VERIFY: db411930 manual
// OFFSET: 0x0066FFE0
template <>
DBCache<NameCache, uint64_t, CHashKeyGUID>::REVERSEENTRY* DBCache<NameCache, uint64_t, CHashKeyGUID>::FindReverseEntry(const char* name) {
    if (this->m_reverse.m_slotmask == 0xFFFFFFFF) {
        return nullptr;
    }

    uint32_t hashval = SStrHashHT(name);
    TSExplicitList<REVERSEENTRY>& list = this->m_reverse.m_slotlistarray[hashval & this->m_reverse.m_slotmask];

    for (REVERSEENTRY* entry = list.Head(); entry; entry = list.Next(entry)) {
        if (entry->m_hashval == hashval && (entry->m_key.m_str == name || !SStrCmpUTF8I(entry->m_key.m_str, name, STORM_MAX_STR))) {
            return entry;
        }
    }

    return nullptr;
}

// VERIFY: 4c9eb627 auto
// OFFSET: 0x006701F0
template <>
NameCache* DBCache<NameCache, uint64_t, CHashKeyGUID>::GetRecordByName(const char* name) {
    REVERSEENTRY* entry = this->FindReverseEntry(name);
    return entry ? entry->m_record : nullptr;
}

// INLINED
template <>
DBCache<PetNameCache, int32_t, HASHKEY_INT>::REVERSEENTRY* DBCache<PetNameCache, int32_t, HASHKEY_INT>::FindReverseEntry(const char* name) {
    if (this->m_reverse.m_slotmask == 0xFFFFFFFF) {
        return nullptr;
    }

    uint32_t hashval = SStrHashHT(name);
    TSExplicitList<REVERSEENTRY>& list = this->m_reverse.m_slotlistarray[hashval & this->m_reverse.m_slotmask];

    for (REVERSEENTRY* entry = list.Head(); entry; entry = list.Next(entry)) {
        if (entry->m_hashval == hashval && (entry->m_key.m_str == name || !SStrCmpUTF8I(entry->m_key.m_str, name, STORM_MAX_STR))) {
            return entry;
        }
    }

    return nullptr;
}

// VERIFY: cb2e9c40 auto
// OFFSET: 0x006701F0
template <>
PetNameCache* DBCache<PetNameCache, int32_t, HASHKEY_INT>::GetRecordByName(const char* name) {
    REVERSEENTRY* entry = this->FindReverseEntry(name);
    return entry ? entry->m_record : nullptr;
}

// VERIFY: fd608d3a manual
// OFFSET: 0x00678F20
template <>
void DBCache<NameCache, uint64_t, CHashKeyGUID>::RemoveReverseEntry(const char* name) {
    REVERSEENTRY* entry = this->FindReverseEntry(name);

    if (!entry) {
        SErrSetLastError(87);
        return;
    }

    this->m_reverse.Delete(entry);
}

// VERIFY: a4de3f42 auto
// OFFSET: 0x00680C20
template <>
void DBCache<NameCache, uint64_t, CHashKeyGUID>::ItemAdded(NameCache* item) {
    if (this->FindReverseEntry(item->m_name)) {
        return;
    }

    uint32_t hashval = SStrHashHT(item->m_name);
    REVERSEENTRY* entry = this->m_reverse.InternalNewNode(hashval, 0, 0);
    entry->m_hashval = hashval;
    entry->m_key.m_str = item->m_name;
    entry->m_record = item;
}

// VERIFY: 499cc0d7 auto
// OFFSET: 0x0067A430
template <>
void DBCache<NameCache, uint64_t, CHashKeyGUID>::Invalidate(uint64_t key) {
    DBCACHEHASH* hash = this->m_entries.Ptr(DBCacheHashKey(key), key);

    if (!hash) {
        return;
    }

    if (!hash->m_loaded) {
        this->DenyItem(key);
    } else if (hash->m_dispatching) {
        hash->m_invalidatePending = true;
    } else {
        this->RemoveReverseEntry(hash->m_record.m_name);
        this->m_entries.Delete(hash);
    }
}

// VERIFY: dcddca51 auto
// OFFSET: 0x00680C60
template <>
void DBCache<NameCache, uint64_t, CHashKeyGUID>::AddItem(NameCache* item, uint64_t key) {
    DBCACHEHASH* hash = this->m_entries.Ptr(DBCacheHashKey(key), key);

    if (!hash) {
        hash = this->m_entries.New(DBCacheHashKey(key), key, 0, 0);
    }

    hash->m_dispatching = true;
    hash->m_record.Copy(*item);
    hash->m_loaded = true;
    hash->m_id = key;
    this->ItemAdded(&hash->m_record);

    for (DBCACHECALLBACK* node = hash->m_callbacks.Head(); node;) {
        DBCACHECALLBACK* next = hash->m_callbacks.Next(node);
        node->m_callback(static_cast<int32_t>(key), node->m_guid, node->m_param, true);
        node = next;
    }

    hash->m_callbacks.Clear();
    bool invalidate = hash->m_invalidatePending;
    hash->m_dispatching = false;

    if (invalidate) {
        this->Invalidate(key);
    }
}

// VERIFY: b863bbfc auto
// OFFSET: 0x00635100
void DBCache_Idle() {
    g_creatureDBCache.Idle();
    g_gameObjectDBCache.Idle();
    g_itemDBCache.Idle();
    g_itemNameDBCache.Idle();
    g_npcTextDBCache.Idle();
    g_nameDBCache.Idle();
    g_guildInfoCache.Idle();
    g_questDBCache.Idle();
    g_pageTextCache.Idle();
    g_petNameCache.Idle();
    g_petitionCache.Idle();
    g_itemTextDBCache.Idle();
    g_wardenCache.Idle();
    g_arenaTeamCache.Idle();
}

// VERIFY: 2a9b337b auto
// OFFSET: 0x00635680
void DBCache_ClearPending() {
    g_creatureDBCache.ClearPending();
    g_gameObjectDBCache.ClearPending();
    g_itemDBCache.ClearPending();
    g_itemNameDBCache.ClearPending();
    g_npcTextDBCache.ClearPending();
    g_nameDBCache.ClearPending();
    g_guildInfoCache.ClearPending();
    g_questDBCache.ClearPending();
    g_pageTextCache.ClearPending();
    g_petNameCache.ClearPending();
    g_petitionCache.ClearPending();
    g_itemTextDBCache.ClearPending();
    g_arenaTeamCache.ClearPending();
    g_danceCache.ClearPending();
}
