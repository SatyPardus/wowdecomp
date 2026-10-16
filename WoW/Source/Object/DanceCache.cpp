#include "DanceCache.h"
#include <cstring>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Storm/H/storm.h"

// INLINED
DanceCache::DanceCache() {
    this->m_id = 0;
    this->m_guid = WOWGUID();
    this->m_name[0] = '\0';
    this->m_data = nullptr;
    this->m_dataCount = 0;
    this->m_version = 0;
}

// INLINED
DanceCache::~DanceCache() {
    delete[] this->m_data;
}

// VERIFY: 57b64e71 manual
// OFFSET: 0x006688B0
DanceCache& DanceCache::operator=(const DanceCache& source) {
    this->m_id = source.m_id;
    this->m_guid = source.m_guid;
    SStrCopy(this->m_name, source.m_name, sizeof(this->m_name));

    if (this->m_dataCount < source.m_dataCount) {
        if (this->m_data) {
            SMemReAlloc(this->m_data, source.m_dataCount, __FILE__, __LINE__, 0);
        } else {
            this->m_data = new (__FILE__, __LINE__) DANCEMOVE[source.m_dataCount];
        }
    }

    memcpy(this->m_data, source.m_data, 4 * source.m_dataCount);
    this->m_dataCount = source.m_dataCount;
    this->m_version = source.m_version;

    return *this;
}

// VERIFY: cb0294c9 manual
// OFFSET: 0x0098D290
void DanceCache::Pack(CDataStore* msg) {
    msg->Put(static_cast<uint32_t>(this->m_id));
    *msg << static_cast<const SmartGUID&>(this->m_guid);
    msg->PutString(this->m_name);
    msg->Put(this->m_dataCount);
    msg->PutData(this->m_data, 4 * this->m_dataCount);
    msg->Put(this->m_version);
}

// VERIFY: 54057452 manual
// OFFSET: 0x0098D300
void DanceCache::Unpack(CDataStore* msg) {
    msg->Get(reinterpret_cast<uint32_t&>(this->m_id));

    if (this->m_id < 0) {
        return;
    }

    *msg >> static_cast<SmartGUID&>(this->m_guid);
    msg->GetString(this->m_name, sizeof(this->m_name));
    msg->Get(this->m_dataCount);
    this->m_data = new (__FILE__, __LINE__) DANCEMOVE[this->m_dataCount];
    msg->GetData(this->m_data, 4 * this->m_dataCount);
    msg->Get(this->m_version);
}
