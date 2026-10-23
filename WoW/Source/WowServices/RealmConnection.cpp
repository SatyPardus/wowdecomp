#include "RealmConnection.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Util/SHA1.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/UIUtil/AddOns.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "GameVersion.h"

// STATIC: 0x00AB953C
TSList<RealmConnection::REALMCONNECTIONNODE> RealmConnection::s_allRealmConnections;

// STATIC: 0x00B38CA8
SCritSect RealmConnection::s_allRealmConnectionsCrit;

// VERIFY: 1d949124 auto
// INLINED
RealmConnection::REALMCONNECTIONNODE::REALMCONNECTIONNODE() {
    this->m_connection = nullptr;
}

// VERIFY: ec31eb67 manual
// OFFSET: 0x00464C10
int32_t RealmConnection::HandleCharEnum(uint32_t msgId, uint32_t time, CDataStore* msg) {
    if (this->m_realmResponse) {
        this->m_realmResponse->GameServerResult(this, "SMSG_CHAR_ENUM", nullptr, nullptr);
    }

    uint8_t count;
    msg->Get(count);

    int32_t invalid = 0;

    if (count > 10) {
        count = 0;
        invalid = 1;
    }

    this->m_characterList.Clear();
    this->m_characterList.SetCount(count);

    for (uint8_t i = 0; i < count; i++) {
        CHARACTER_INFO* info = &this->m_characterList.m_data[i];

        msg->Get(info->m_guid.m_guid);
        msg->GetString(info->m_name, sizeof(info->m_name));
        msg->Get(info->m_raceID);
        msg->Get(info->m_classID);
        msg->Get(info->m_sexID);
        msg->Get(info->m_skinColorID);
        msg->Get(info->m_faceID);
        msg->Get(info->m_hairStyleID);
        msg->Get(info->m_hairColorID);
        msg->Get(info->m_facialHairStyleID);
        msg->Get(info->m_experienceLevel);
        msg->Get(info->m_zoneID);
        msg->Get(info->m_mapID);
        msg->Get(info->m_position.x);
        msg->Get(info->m_position.y);
        msg->Get(info->m_position.z);
        msg->Get(info->m_guildID);
        msg->Get(info->m_flags);
        msg->Get(info->m_customizeFlags);
        msg->Get(info->m_firstLogin);
        msg->Get(info->m_petDisplayInfoID);
        msg->Get(info->m_petExperienceLevel);
        msg->Get(info->m_petCreatureFamilyID);

        for (int32_t j = 0; j < 23; j++) {
            msg->Get(info->m_inventoryItemDisplayID[j]);

            uint8_t inventoryType;
            msg->Get(inventoryType);
            info->m_inventoryItemType[j] = inventoryType;

            msg->Get(info->m_inventoryItemVisualID[j]);
        }
    }

    int32_t success = 0;

    if (msg->IsRead()) {
        if (!invalid) {
            success = 1;
        }
    } else if (!invalid) {
        msg->Get(this->m_restrictHuman);
        msg->Get(this->m_restrictDwarf);
        msg->Get(this->m_restrictGnome);
        msg->Get(this->m_restrictNightElf);
        msg->Get(this->m_restrictDraenei);
        msg->Get(this->m_restrictOrc);
        msg->Get(this->m_restrictTroll);
        msg->Get(this->m_restrictTauren);
        msg->Get(this->m_restrictUndead);
        msg->Get(this->m_restrictBloodElf);

        if (msg->IsRead()) {
            success = 1;
        }
    }

    if (!success) {
        this->m_characterList.Clear();
    }

    this->m_realmResponse->CharacterListReceived(this, this->m_characterList, success);

    return 1;
}

// VERIFY: fca783d3 auto
// OFFSET: 0x00464F50
int32_t RealmConnection::MessageHandler(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    RealmConnection* connection = static_cast<RealmConnection*>(param);
    int32_t result = 0;

    switch (msgId) {
        case SMSG_CHAR_CREATE:
            result = connection->HandleCharacterCreate(msgId, time, msg);
            break;

        case SMSG_ENUM_CHARACTERS_RESULT:
            result = connection->HandleCharEnum(msgId, time, msg);
            break;

        case SMSG_DELETE_CHAR:
            result = connection->HandleCharacterDelete(msgId, time, msg);
            break;

        case SMSG_CHARACTER_LOGIN_FAILED:
            result = connection->HandleCharacterLoginFailed(msgId, time, msg);
            break;

        case SMSG_LOGOUT_RESPONSE:
            result = connection->HandleLogoutResponse(msgId, time, msg);
            break;

        case SMSG_LOGOUT_COMPLETE:
            connection->HandleLogoutComplete();
            result = 1;
            break;

        case SMSG_LOGOUT_CANCEL_ACK:
            connection->HandleLogoutAbortAck();
            result = 1;
            break;

        case SMSG_AUTH_RESPONSE:
            result = connection->HandleAuthResponse(SMSG_AUTH_RESPONSE, time, msg);
            break;

        case SMSG_ADDON_INFO:
            result = connection->HandleAddonInfo(SMSG_ADDON_INFO, time, msg);
            break;

        case SMSG_CACHE_VERSION:
            result = connection->HandleClientCacheVersion(SMSG_CACHE_VERSION, time, msg);
            break;

        default:
            break;
    }

    if (!msg->IsRead()) {
        char text[64];
        SStrPrintf(text, sizeof(text), "ID:%d Read:%d Size:%d", msgId, msg->m_read, msg->m_size);

        if (connection->m_realmResponse) {
            connection->m_realmResponse->GameServerResult(connection, "RealmConnection::MessageHandler", "Message Under Read!", text);
        }

        msg->Reset();
    }

    return result;
}

// VERIFY: 0d14ec4d manual
// OFFSET: 0x004650E0
RealmConnection::RealmConnection(RealmResponse* realmResponse) : m_realmResponse(realmResponse), m_authenticated(0), m_queuePosition(0), m_freeCharacterMigration(0), m_billingTimeRemaining(0), m_billingTimeRested(0), m_billingFlags(0), m_accountExpansion(0), m_regionId(0), m_battlegroupId(0), m_realmId(0) {
    this->Initialize();

    this->SetMessageHandler(SMSG_AUTH_CHALLENGE, RealmConnection::MessageHandler, this);
    this->SetMessageHandler(SMSG_AUTH_RESPONSE, RealmConnection::MessageHandler, this);
    this->SetMessageHandler(SMSG_ADDON_INFO, RealmConnection::MessageHandler, this);
    this->SetMessageHandler(SMSG_ENUM_CHARACTERS_RESULT, RealmConnection::MessageHandler, this);
    this->SetMessageHandler(SMSG_CHAR_CREATE, RealmConnection::MessageHandler, this);
    this->SetMessageHandler(SMSG_CHARACTER_LOGIN_FAILED, RealmConnection::MessageHandler, this);
    this->SetMessageHandler(SMSG_LOGOUT_COMPLETE, RealmConnection::MessageHandler, this);
    this->SetMessageHandler(SMSG_LOGOUT_CANCEL_ACK, RealmConnection::MessageHandler, this);
    this->SetMessageHandler(SMSG_LOGOUT_RESPONSE, RealmConnection::MessageHandler, this);
    this->SetMessageHandler(SMSG_DELETE_CHAR, RealmConnection::MessageHandler, this);
    this->SetMessageHandler(SMSG_CACHE_VERSION, RealmConnection::MessageHandler, this);

    RealmConnection::s_allRealmConnectionsCrit.Enter();
    RealmConnection::REALMCONNECTIONNODE* node = RealmConnection::s_allRealmConnections.NewNode(2, 0, 0);
    node->m_connection = this;
    RealmConnection::s_allRealmConnectionsCrit.Leave();
}

// VERIFY: a4dc8cc2 auto
// OFFSET: 0x00464350
int32_t RealmConnection::HandleConnect() {
    if (this->m_realmResponse) {
        this->m_realmResponse->GameServerResult(this, "Connected", nullptr, nullptr);
    }

    this->m_authenticated = 0;
    this->m_realmResponse->HandleConnect(this);

    return NetClient::HandleConnect();
}

// VERIFY: 2bad756d auto
// OFFSET: 0x00464390
int32_t RealmConnection::HandleDisconnect() {
    if (this->m_realmResponse) {
        this->m_realmResponse->GameServerResult(this, "Disconnected", nullptr, nullptr);
    }

    this->m_realmResponse->HandleDisconnect(this);

    return NetClient::HandleDisconnect();
}

// VERIFY: e27d0b04 auto
// OFFSET: 0x004643D0
int32_t RealmConnection::HandleCantConnect() {
    if (this->m_realmResponse) {
        this->m_realmResponse->GameServerResult(this, "Can't Connect", nullptr, nullptr);
    }

    this->m_realmResponse->HandleCantConnect(this);

    return NetClient::HandleCantConnect();
}

// VERIFY: 2f707bc2 auto
// OFFSET: 0x00464410
int32_t RealmConnection::HandleAddonInfo(uint32_t msgId, uint32_t time, CDataStore* msg) {
    if (this->m_realmResponse) {
        this->m_realmResponse->GameServerResult(this, "SMSG_ADDON_INFO", nullptr, nullptr);
    }

    SetAddOnInfoRequest(msg);
    this->m_realmResponse->AddOnListReceived(this);

    return 1;
}

// VERIFY: b88b6288 auto
// OFFSET: 0x00464460
int32_t RealmConnection::HandleCharacterLoginFailed(uint32_t msgId, uint32_t time, CDataStore* msg) {
    uint8_t result;
    msg->Get(result);
    this->HandleCharacterLoginFailed(result);

    return 1;
}

// VERIFY: 197a26a8 auto
// OFFSET: 0x00464490
void RealmConnection::SetSelectedRealm(uint32_t regionId, uint32_t battlegroupId, uint32_t realmId) {
    this->m_regionId = regionId;
    this->m_battlegroupId = battlegroupId;
    this->m_realmId = realmId;
}

// VERIFY: b87ab906 auto
// OFFSET: 0x00464640
int32_t RealmConnection::HandleAuthResponse(uint32_t msgId, uint32_t time, CDataStore* msg) {
    if (this->m_realmResponse) {
        this->m_realmResponse->GameServerResult(this, "SMSG_AUTH_RESPONSE", nullptr, nullptr);
    }

    uint8_t result;
    msg->Get(result);

    if (result == 12 || result == 27) {
        if (result == 12) {
            this->m_authenticated = 1;
        }

        if (msg->m_size - (result == 27 ? 5 : 0) - msg->m_read >= 10) {
            msg->Get(this->m_billingTimeRemaining);
            msg->Get(this->m_billingFlags);
            msg->Get(this->m_billingTimeRested);
            msg->Get(this->m_accountExpansion);
        }

        if (result == 27) {
            msg->Get(this->m_queuePosition);

            uint8_t freeCharacterMigration = 0;
            msg->Get(freeCharacterMigration);
            this->m_freeCharacterMigration = freeCharacterMigration;
        }
    }

    this->m_realmResponse->HandleAuthResponse(this, result);

    return 1;
}

// VERIFY: 2c945e3c auto
// OFFSET: 0x00464730
int32_t RealmConnection::HandleClientCacheVersion(uint32_t msgId, uint32_t time, CDataStore* msg) {
    if (this->m_realmResponse) {
        this->m_realmResponse->GameServerResult(this, "SMSG_CLIENTCACHE_VERSION", nullptr, nullptr);
    }

    uint32_t version;
    msg->Get(version);
    SetClientCacheVersion(version);

    return 1;
}

// VERIFY: f89adfdd auto
// OFFSET: 0x00464780
int32_t RealmConnection::HandleCharacterCreate(uint32_t msgId, uint32_t time, CDataStore* msg) {
    uint8_t result;
    msg->Get(result);
    this->HandleCharacterCreate(result);

    return 1;
}

// VERIFY: 58fb7628 auto
// OFFSET: 0x004647B0
int32_t RealmConnection::HandleCharacterDelete(uint32_t msgId, uint32_t time, CDataStore* msg) {
    uint8_t result;
    msg->Get(result);
    this->HandleCharacterDelete(result);

    return 1;
}

// VERIFY: be3ce643 auto
// OFFSET: 0x004647E0
int32_t RealmConnection::HandleLogoutResponse(uint32_t msgId, uint32_t time, CDataStore* msg) {
    uint32_t result = 0;
    msg->Get(result);

    uint8_t instant = 0;
    msg->Get(instant);
    this->HandleLogoutResponse(result, instant);

    return 1;
}

// VERIFY: a1616851 manual
// OFFSET: 0x00464830
void RealmConnection::HandleAuthChallenge(AuthenticationChallenge* challenge) {
    if (this->m_realmResponse) {
        this->m_realmResponse->GameServerResult(this, "SMSG_AUTH_CHALLENGE", nullptr, nullptr);
    }

    this->m_authenticated = 0;

    CRndSeed seed;
    seed.SetSeed(0);
    seed.SetSeed(OsGetAsyncTimeMs());

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_AUTH_SESSION));
    msg.Put(static_cast<uint32_t>(GAME_BUILD));
    msg.Put(static_cast<uint32_t>(this->m_loginData.m_loginServerID));
    msg.PutString(this->m_loginData.m_account);
    msg.Put(static_cast<uint32_t>(this->m_loginData.m_loginServerType));

    uint32_t clientSeed = CRandom::uint32_(seed);
    msg.Put(clientSeed);
    msg.Put(this->m_regionId);
    msg.Put(this->m_battlegroupId);
    msg.Put(this->m_realmId);

    uint32_t zero = 0;
    msg.Put(challenge->m_proof);

    SHA1_CONTEXT context;
    SHA1_Init(&context);
    SHA1_Update(&context, reinterpret_cast<const uint8_t*>(this->m_loginData.m_account), SStrLen(this->m_loginData.m_account));
    SHA1_Update(&context, reinterpret_cast<const uint8_t*>(&zero), sizeof(zero));
    SHA1_Update(&context, reinterpret_cast<const uint8_t*>(&clientSeed), sizeof(clientSeed));
    SHA1_Update(&context, reinterpret_cast<const uint8_t*>(&challenge->m_serverSeed), sizeof(challenge->m_serverSeed));
    SHA1_Update(&context, this->m_loginData.m_sessionKey, sizeof(this->m_loginData.m_sessionKey));

    uint8_t digest[20];
    SHA1_Final(digest, &context);
    msg.PutData(digest, sizeof(digest));

    GetAddOnInfoRequest(&msg);
    msg.Finalize();
    this->Send(&msg);
}

// VERIFY: 43999034 manual
// OFFSET: 0x00464A40
void RealmConnection::RequestCharacterEnum() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ENUM_CHARACTERS));
    msg.Finalize();
    this->Send(&msg);
}

// VERIFY: 490618f4 manual
// OFFSET: 0x00464AB0
void RealmConnection::RequestCharacterLogin(WOWGUID guid, int32_t a2) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PLAYER_LOGIN));
    msg.Put(guid.m_guid);
    msg.Finalize();
    this->Send(&msg);
}

// VERIFY: 1d949124 auto
// OFFSET: 0x00464BC0
void RealmConnection::PollNet(const void* data, void* param) {
    RealmConnection::s_allRealmConnectionsCrit.Enter();

    RealmConnection::REALMCONNECTIONNODE* node = RealmConnection::s_allRealmConnections.Head();

    while (!TSList<RealmConnection::REALMCONNECTIONNODE, TSGetLink<RealmConnection::REALMCONNECTIONNODE> >::IsEnd(node)) {
        RealmConnection::REALMCONNECTIONNODE* next = RealmConnection::s_allRealmConnections.RawNext(node);
        node->m_connection->PollEventQueue();
        node = next;
    }

    RealmConnection::s_allRealmConnectionsCrit.Leave();
}

// VERIFY: 5c037920 manual
// OFFSET: [0x00465320, 0x00465450]
RealmConnection::~RealmConnection() {
    RealmConnection::s_allRealmConnectionsCrit.Enter();

    for (RealmConnection::REALMCONNECTIONNODE* node = RealmConnection::s_allRealmConnections.Head(); node; node = RealmConnection::s_allRealmConnections.Next(node)) {
        if (node->m_connection == this) {
            RealmConnection::s_allRealmConnections.DeleteNode(node);
            break;
        }
    }

    this->ResetHandler(SMSG_AUTH_CHALLENGE);
    this->ResetHandler(SMSG_AUTH_RESPONSE);
    this->ResetHandler(SMSG_ADDON_INFO);
    this->ResetHandler(SMSG_ENUM_CHARACTERS_RESULT);
    this->ResetHandler(SMSG_CHAR_CREATE);
    this->ResetHandler(SMSG_CHARACTER_LOGIN_FAILED);
    this->ResetHandler(SMSG_LOGOUT_COMPLETE);
    this->ResetHandler(SMSG_LOGOUT_CANCEL_ACK);
    this->ResetHandler(SMSG_LOGOUT_RESPONSE);
    this->ResetHandler(SMSG_DELETE_CHAR);
    this->ResetHandler(SMSG_CACHE_VERSION);
    this->Destroy();

    RealmConnection::s_allRealmConnectionsCrit.Leave();

    this->m_characterList.Clear();

    RealmConnection::s_allRealmConnectionsCrit.Enter();
    this->m_realmResponse = nullptr;
    RealmConnection::s_allRealmConnectionsCrit.Leave();
}

// VERIFY: a9aa9a7d auto
// OFFSET: 0x006B1B90
void RealmResponse::JoinRealmResult(uint32_t realmID, int32_t result, int32_t a3, int32_t a4, int32_t a5) {
}

// VERIFY: 01c03df0 auto
// OFFSET: 0x00653A10
void RealmResponse::OnChatPlayerNotFound(RealmConnection* connection, const char* name) {
}

// VERIFY: 0ef00e5d auto
// OFFSET: 0x006B0960
void RealmResponse::OnCharacterRestored(RealmConnection* connection, bool success, uint32_t a3, uint32_t a4, uint32_t a5, uint64_t a6, uint64_t a7, uint32_t a8, const char* a9, uint32_t a10, CDataStore* msg, uint64_t a12, uint64_t a13) {
}

// VERIFY: 4201e0ab auto
// OFFSET: 0x006B1B80
void RealmResponse::OnChangeArenaTeamRating(RealmConnection* connection, bool success, int32_t a3, int32_t a4) {
}

// VERIFY: ceede6fc auto
// OFFSET: 0x006B1B80
void RealmResponse::OnServerInfo(RealmConnection* connection, int32_t a2, int32_t a3, char* a4) {
}

// VERIFY: d7d1893d auto
// OFFSET: 0x00653A10
void RealmResponse::HandleCharacterLoginFailed(RealmConnection* connection, uint8_t result) {
}

// VERIFY: 44e6f046 auto
// OFFSET: 0x00632050
void RealmResponse::HandleReceivingObjects(RealmConnection* connection) {
}
