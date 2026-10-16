#ifndef WOW_SOURCE_UI_PETITIONINFO_H
#define WOW_SOURCE_UI_PETITIONINFO_H

#include <cstdint>
#include "WoW/Source/Net/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/WGUID.h"

struct lua_State;
class CGPetition;

struct PetitionSignerInfo {
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ uint32_t unused_0008;
    /* 000C */ uint32_t unused_000C;
};

class CGPetitionInfo {
    public:
    static void ShutdownGame();
    static WOWGUID m_petitionGUID;
    static WOWGUID m_petitionItem;
    static uint32_t m_petitionId;
    static int32_t m_signed;
    static uint32_t m_numPetitionNames;
    static int32_t m_pendingNameQueries;
    static CGPetition* m_petition;
    static TSGrowableArray<PetitionSignerInfo> m_signers;

    static void EnterWorld();
    static void LeaveWorld();
    static void InitializeGame();
    static void SetPetitionStats(int32_t id);
    static void ClearSignatures();
    static void SetPetition(WOWGUID petition, int32_t id, WOWGUID item);
    static void SetSignatures(uint8_t count, const WOWGUID* guids, const uint32_t* choices);
    static void AddSignature(WOWGUID petition, WOWGUID signer);
};

void PetitionInfoRegisterScriptFunctions();
void PetitionInfoUnregisterScriptFunctions();

class CDataStore;

int32_t Packet_SMSG_OFFER_PETITION_ERROR(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
void SignatureNameQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void PetitionQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success);

#endif
