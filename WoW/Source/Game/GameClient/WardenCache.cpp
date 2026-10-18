#include "WoW/Source/Game/GameClient/WardenClient.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Storm/H/storm.h"

// OFFSET: 0x007DA190
CDataStore& operator<<(CDataStore& msg, const CWardenKey& key) {
    for (uint32_t i = 0; i < sizeof(key.m_key); i++) {
        msg.Put(key.m_key[i]);
    }

    return msg;
}

// OFFSET: 0x007DA1C0
CDataStore& operator>>(CDataStore& msg, CWardenKey& key) {
    for (uint32_t i = 0; i < sizeof(key.m_key); i++) {
        msg.Get(key.m_key[i]);
    }

    return msg;
}

// OFFSET: 0x007DA1F0
WardenCachedModule::WardenCachedModule() {
    this->m_size = 0;
    this->m_data = nullptr;
}

// OFFSET: 0x007DA200
WardenCachedModule::~WardenCachedModule() {
    if (this->m_data) {
        SMemFree(this->m_data, __FILE__, __LINE__, 0);
    }
}

// OFFSET: 0x007DA220
void WardenCachedModule::Pack(CDataStore* msg) {
    msg->Put(this->m_data ? this->m_size : 0);

    for (uint32_t i = 0; i < this->m_size; i++) {
        msg->Put(this->m_data[i]);
    }
}

// OFFSET: 0x007DA260
void WardenCachedModule::Unpack(CDataStore* msg) {
    msg->Get(this->m_size);

    if (!this->m_size) {
        this->m_data = nullptr;
        return;
    }

    this->m_data = static_cast<uint8_t*>(SMemAlloc(this->m_size, __FILE__, __LINE__, 0));

    for (uint32_t i = 0; i < this->m_size; i++) {
        msg->Get(this->m_data[i]);
    }
}

