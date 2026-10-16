#include "PetitionInfo.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Object/CGPetition.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/Types.h"

// STATIC: 0x00C22B20
WOWGUID CGPetitionInfo::m_petitionGUID;

// STATIC: 0x00C22B28
WOWGUID CGPetitionInfo::m_petitionItem;

// STATIC: 0x00C22B30
uint32_t CGPetitionInfo::m_petitionId;

// STATIC: 0x00C22B34
int32_t CGPetitionInfo::m_signed;

// STATIC: 0x00C22B38
uint32_t CGPetitionInfo::m_numPetitionNames;

// STATIC: 0x00C22B3C
int32_t CGPetitionInfo::m_pendingNameQueries;

// STATIC: 0x00C22B40
CGPetition* CGPetitionInfo::m_petition;

// STATIC: 0x00C22B44
TSGrowableArray<PetitionSignerInfo> CGPetitionInfo::m_signers;

// VERIFY: 81bb7d5a auto
// OFFSET: 0x005CEA00
int32_t Packet_SMSG_OFFER_PETITION_ERROR(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (name) {
        CGGameUI::DisplayError(ERR_ALREADY_IN_ARENA_TEAM_S, name->m_name);
    }

    return 1;
}

// VERIFY: 48b05410 auto
// OFFSET: 0x005CEA60
void CGPetitionInfo::SetPetitionStats(int32_t id) {
    if (CGPetitionInfo::m_petitionId != id) {
        return;
    }

    WOWGUID none = WOWGUID();
    CGPetitionInfo::m_petition = g_petitionCache.GetRecord(id, none, nullptr, nullptr, false);

    if (!CGPetitionInfo::m_pendingNameQueries && CGPetitionInfo::m_petition) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_SHOW, nullptr);
    }
}

// VERIFY: 4cd799fe auto
// OFFSET: 0x005CEBF0
void SignatureNameQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (CGPetitionInfo::m_pendingNameQueries && !--CGPetitionInfo::m_pendingNameQueries && CGPetitionInfo::m_petition) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_SHOW, nullptr);
    }
}

// VERIFY: 8d5349d7 auto
// OFFSET: 0x005CEC20
void PetitionQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        CGPetitionInfo::SetPetitionStats(id);
    }
}

// VERIFY: a4620e5c auto
// OFFSET: 0x005CEC40
void CGPetitionInfo::InitializeGame() {
    ClientServices::SetMessageHandler(SMSG_OFFER_PETITION_ERROR, Packet_SMSG_OFFER_PETITION_ERROR, nullptr);
}

// VERIFY: 9b0171ce auto
// OFFSET: 0x005CEC60
void CGPetitionInfo::ClearSignatures() {
    if (CGPetitionInfo::m_pendingNameQueries) {
        for (uint32_t i = 0; i < CGPetitionInfo::m_numPetitionNames; i++) {
            g_nameDBCache.CancelCallback(CGPetitionInfo::m_signers.m_data[i].m_guid.m_guid, SignatureNameQueryCallback, nullptr);
        }
    }

    CGPetitionInfo::m_numPetitionNames = 0;
    CGPetitionInfo::m_pendingNameQueries = 0;
}

// VERIFY: 1e070bc4 manual
// OFFSET: 0x005CEEF0
void CGPetitionInfo::SetPetition(WOWGUID petition, int32_t id, WOWGUID item) {
    if (CGPetitionInfo::m_petitionGUID) {
        if (!CGPetitionInfo::m_signed && CGPetitionInfo::m_petition && CGPetitionInfo::m_petition->m_ownerGuid != ClntObjMgrGetActivePlayer()) {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(MSG_PETITION_DECLINE));
            msg.Put(CGPetitionInfo::m_petitionGUID.m_guid);
            msg.Finalize();
            ClientServices::Send2(&msg);
        }

        CGPetitionInfo::ClearSignatures();
        CGPetitionInfo::m_petitionGUID = WOWGUID();
        CGPetitionInfo::m_petitionItem = WOWGUID();
        FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_CLOSED, nullptr);
    }

    CGPetitionInfo::m_petitionItem = item;
    CGPetitionInfo::m_petitionGUID = petition;
    CGPetitionInfo::m_petitionId = id;

    if (petition) {
        CGPetitionInfo::m_petition = g_petitionCache.GetRecord(id, petition, PetitionQueryCallback, nullptr, false);
        CGPetitionInfo::m_signed = 0;
    } else {
        CGPetitionInfo::m_petition = nullptr;
        CGPetitionInfo::m_signed = 0;
    }
}

// VERIFY: 0a010502 manual
// OFFSET: 0x005CF580
void CGPetitionInfo::EnterWorld() {
    CGPetitionInfo::m_petitionGUID = WOWGUID{ 0 };
    CGPetitionInfo::m_petitionItem = WOWGUID{ 0 };
    CGPetitionInfo::m_petitionId = 0;
    CGPetitionInfo::m_numPetitionNames = 0;
    CGPetitionInfo::m_pendingNameQueries = 0;
    CGPetitionInfo::m_petition = nullptr;
    CGPetitionInfo::m_signers.SetCount(10);
}

// VERIFY: 8452d77d auto
// OFFSET: 0x005CE9F0
void CGPetitionInfo::ShutdownGame() {
    ClientServices::ClearMessageHandler(SMSG_OFFER_PETITION_ERROR);
}

// VERIFY: c20958fb auto
// OFFSET: 0x005CF650
void CGPetitionInfo::SetSignatures(uint8_t count, const WOWGUID* guids, const uint32_t* choices) {
    CGPetitionInfo::ClearSignatures();

    if (CGPetitionInfo::m_signers.m_count < count) {
        CGPetitionInfo::m_signers.SetCount(count);
    }

    CGPetitionInfo::m_numPetitionNames = count;

    for (uint32_t i = 0; i < count; i++) {
        CGPetitionInfo::m_signers.m_data[i].m_guid = guids[i];
        CGPetitionInfo::m_signers.m_data[i].unused_0008 = choices[i];

        if (!g_nameDBCache.GetRecord(guids[i].m_guid, guids[i], SignatureNameQueryCallback, nullptr, false)) {
            CGPetitionInfo::m_pendingNameQueries++;
        }
    }

    if (!CGPetitionInfo::m_pendingNameQueries && CGPetitionInfo::m_petition) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_SHOW, nullptr);
    }
}

// VERIFY: 3e6bcccf manual
// OFFSET: 0x005CF710
void CGPetitionInfo::AddSignature(WOWGUID petition, WOWGUID signer) {
    uint32_t count = CGPetitionInfo::m_numPetitionNames;

    for (uint32_t i = 0; i < count; i++) {
        if (CGPetitionInfo::m_signers.m_data[i].m_guid == signer) {
            return;
        }
    }

    if (CGPetitionInfo::m_numPetitionNames == CGPetitionInfo::m_signers.Count()) {
        CGPetitionInfo::m_signers.SetCount(CGPetitionInfo::m_signers.Count() + 1);
    }

    CGPetitionInfo::m_signers.m_data[CGPetitionInfo::m_numPetitionNames].m_guid = signer;
    CGPetitionInfo::m_numPetitionNames++;
    NameCache* name = g_nameDBCache.GetRecord(signer.m_guid, signer, SignatureNameQueryCallback, nullptr, false);

    if (name) {
        CGGameUI::DisplayError(ERR_PETITION_SIGNED_S, name->m_name);
    } else {
        CGPetitionInfo::m_pendingNameQueries++;
    }

    if (!CGPetitionInfo::m_pendingNameQueries && CGPetitionInfo::m_petition) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PETITION_SHOW, nullptr);
    }
}

// VERIFY: 3777712c auto
// OFFSET: 0x005CF610
void CGPetitionInfo::LeaveWorld() {
    CGPetitionInfo::m_signers.Clear();
}
