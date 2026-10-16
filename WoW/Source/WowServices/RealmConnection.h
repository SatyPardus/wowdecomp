#ifndef WOW_SOURCE_WOWSERVICES_REALMCONNECTION_H
#define WOW_SOURCE_WOWSERVICES_REALMCONNECTION_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Net/NetClient/NetClient.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/WowServices/Types.h"

class CHARACTER_INFO;
class RealmConnection;
class RealmResponse;
struct REALM_INFO;

class CHARACTER_INFO {
    public:
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ char m_name [48];
    /* 0038 */ uint32_t m_mapID;
    /* 003C */ uint32_t m_zoneID;
    /* 0040 */ uint32_t m_guildID;
    /* 0044 */ C3Vector m_position;
    /* 0050 */ uint32_t m_inventoryItemDisplayID [23];
    /* 00AC */ uint32_t m_inventoryItemType [23];
    /* 0108 */ uint32_t m_inventoryItemVisualID [23];
    /* 0164 */ uint32_t m_petDisplayInfoID;
    /* 0168 */ uint32_t m_petExperienceLevel;
    /* 016C */ uint32_t m_petCreatureFamilyID;
    /* 0170 */ uint32_t m_flags;
    /* 0174 */ uint32_t m_customizeFlags;
    /* 0178 */ uint8_t m_raceID;
    /* 0179 */ uint8_t m_classID;
    /* 017A */ uint8_t m_sexID;
    /* 017B */ uint8_t m_skinColorID;
    /* 017C */ uint8_t m_faceID;
    /* 017D */ uint8_t m_hairStyleID;
    /* 017E */ uint8_t m_hairColorID;
    /* 017F */ uint8_t m_facialHairStyleID;
    /* 0180 */ uint8_t m_experienceLevel;
    /* 0181 */ uint8_t m_firstLogin;
    /* 0182 */ uint8_t unused_0182[6];
};

class RealmConnection : public NetClient {
    public:
    struct REALMCONNECTIONNODE : public TSLinkedNode<RealmConnection::REALMCONNECTIONNODE> {
        /* 0008 */ RealmConnection* m_connection;

        REALMCONNECTIONNODE();
    };

    static TSList<RealmConnection::REALMCONNECTIONNODE> s_allRealmConnections;
    static SCritSect s_allRealmConnectionsCrit;

    static int32_t MessageHandler(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
    static void PollNet(const void* data, void* param);

    /* 2EE0 */ RealmResponse* m_realmResponse;
    /* 2EE4 */ uint32_t m_restrictHuman;
    /* 2EE8 */ uint32_t m_restrictDwarf;
    /* 2EEC */ uint32_t m_restrictGnome;
    /* 2EF0 */ uint32_t m_restrictNightElf;
    /* 2EF4 */ uint32_t m_restrictDraenei;
    /* 2EF8 */ uint32_t m_restrictOrc;
    /* 2EFC */ uint32_t m_restrictTroll;
    /* 2F00 */ uint32_t m_restrictTauren;
    /* 2F04 */ uint32_t m_restrictUndead;
    /* 2F08 */ uint32_t m_restrictBloodElf;
    /* 2F0C */ TSFixedArray<CHARACTER_INFO> m_characterList;
    /* 2F18 */ uint8_t m_authenticated;
    /* 2F19 */ uint8_t unused_2F19[3];
    /* 2F1C */ uint32_t m_queuePosition;
    /* 2F20 */ uint32_t m_freeCharacterMigration;
    /* 2F24 */ uint32_t m_billingTimeRemaining;
    /* 2F28 */ uint32_t m_billingTimeRested;
    /* 2F2C */ uint8_t m_billingFlags;
    /* 2F2D */ uint8_t m_accountExpansion;
    /* 2F2E */ uint8_t unused_2F2E[2];
    /* 2F30 */ uint32_t m_regionId;
    /* 2F34 */ uint32_t m_battlegroupId;
    /* 2F38 */ uint32_t m_realmId;

    ~RealmConnection() override;
    void HandleAuthChallenge(AuthenticationChallenge* challenge) override;
    int32_t HandleConnect() override;
    int32_t HandleDisconnect() override;
    int32_t HandleCantConnect() override;
    virtual void HandleCharacterCreate(uint8_t result) = 0;
    virtual void HandleCharacterDelete(uint8_t result) = 0;
    virtual void HandleCharacterLoginFailed(uint8_t result) = 0;
    virtual void HandleLogoutComplete() = 0;
    virtual void HandleLogoutResponse(int32_t result, int32_t instant) = 0;
    virtual void HandleLogoutAbortAck() = 0;

    RealmConnection(RealmResponse* realmResponse);
    int32_t HandleAddonInfo(uint32_t msgId, uint32_t time, CDataStore* msg);
    int32_t HandleAuthResponse(uint32_t msgId, uint32_t time, CDataStore* msg);
    int32_t HandleCharEnum(uint32_t msgId, uint32_t time, CDataStore* msg);
    int32_t HandleCharacterCreate(uint32_t msgId, uint32_t time, CDataStore* msg);
    int32_t HandleCharacterDelete(uint32_t msgId, uint32_t time, CDataStore* msg);
    int32_t HandleCharacterLoginFailed(uint32_t msgId, uint32_t time, CDataStore* msg);
    int32_t HandleClientCacheVersion(uint32_t msgId, uint32_t time, CDataStore* msg);
    int32_t HandleLogoutResponse(uint32_t msgId, uint32_t time, CDataStore* msg);
    void RequestCharacterEnum();
    void RequestCharacterLogin(WOWGUID guid, int32_t a2);
    void SetSelectedRealm(uint32_t regionId, uint32_t battlegroupId, uint32_t realmId);
};

class RealmResponse {
    public:
    virtual void LoginServerResult(LOGIN_STATE state, LOGIN_RESULT result, const char* message1, const char* message2, const char* message3) = 0;
    virtual void RealmListReceived(TSFixedArray<REALM_INFO>& realms) = 0;
    virtual void JoinRealmResult(uint32_t realmID, int32_t result, int32_t a3, int32_t a4, int32_t a5);
    virtual void HandleAuthResponse(RealmConnection* connection, uint8_t result) = 0;
    virtual void AddOnListReceived(RealmConnection* connection) = 0;
    virtual void CharacterListReceived(RealmConnection* connection, const TSFixedArray<CHARACTER_INFO>& characters, int32_t success) = 0;
    virtual void GameServerResult(RealmConnection* connection, const char* message1, const char* message2, const char* message3) = 0;
    virtual void MessageReceived(RealmConnection* connection, uint32_t message) = 0;
    virtual void OnChatReceived(RealmConnection* connection, WOWGUID guid, const char* text, uint8_t type) = 0;
    virtual void OnChatPlayerNotFound(RealmConnection* connection, const char* name);
    virtual void OnAccountOnline(RealmConnection* connection, int32_t a2, WOWGUID guid) = 0;
    virtual void OnCharacterRestored(RealmConnection* connection, bool success, uint32_t a3, uint32_t a4, uint32_t a5, uint64_t a6, uint64_t a7, uint32_t a8, const char* a9, uint32_t a10, CDataStore* msg, uint64_t a12, uint64_t a13);
    virtual void OnChangeArenaTeamRating(RealmConnection* connection, bool success, int32_t a3, int32_t a4);
    virtual void OnAFKMonitorInfoResponse(RealmConnection* connection, WOWGUID guid, int32_t a3, int32_t a4, int32_t a5, int32_t a6) = 0;
    virtual void OnServerInfo(RealmConnection* connection, int32_t a2, int32_t a3, char* a4);
    virtual void HandleConnect(RealmConnection* connection) = 0;
    virtual void HandleDisconnect(RealmConnection* connection) = 0;
    virtual void HandleCantConnect(RealmConnection* connection) = 0;
    virtual void HandleCharacterLoginFailed(RealmConnection* connection, uint8_t result);
    virtual void HandleReceivingObjects(RealmConnection* connection);
};

#endif
