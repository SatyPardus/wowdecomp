#if defined(WOWDECOMP_ARCH_64)

#include "WoW/Source/Game/GameClient/WardenClient.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x00D31A58
static uint32_t s_lastProcessTime;

// STATIC: 0x00D31A60
static SCritSect s_wardenCritSect;

// OFFSET: 0x007DA850
static int32_t ReceiveWardenData(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (msgId != SMSG_WARDEN_DATA) {
        return 0;
    }

    s_wardenCritSect.Enter();

    void* data;
    msg->GetDataInSitu(data, msg->m_size - msg->m_read);

    s_wardenCritSect.Leave();

    return 1;
}

// OFFSET: 0x007DA3D0
void WardenClient_Process() {
    s_wardenCritSect.Enter();
    s_lastProcessTime = OsGetAsyncTimeMs();
    s_wardenCritSect.Leave();
}

// OFFSET: 0x007DA420
void WardenClient_Destroy() {
    s_wardenCritSect.Enter();
    ClientServices::ClearMessageHandler(SMSG_WARDEN_DATA);
    s_wardenCritSect.Leave();
}

// OFFSET: 0x007DA8C0
bool WardenClient_Initialize() {
    s_wardenCritSect.Enter();
    WardenClient_Destroy();
    s_lastProcessTime = OsGetAsyncTimeMs();
    ClientServices::SetMessageHandler(SMSG_WARDEN_DATA, ReceiveWardenData, nullptr);
    s_wardenCritSect.Leave();

    return false;
}

#endif
