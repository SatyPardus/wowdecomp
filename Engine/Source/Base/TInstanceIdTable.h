#ifndef ENGINE_SOURCE_BASE_TINSTANCEIDTABLE_H
#define ENGINE_SOURCE_BASE_TINSTANCEIDTABLE_H

#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Storm/Source/SLock.h"

typedef uint32_t INSTANCEID;

template <class T, int slotCount>
class TInstanceIdTable {
    public:
    /* 0000 */ SCritSect m_idCritsect;
    /* 0028 */ INSTANCEID m_id;
    /* 002C */ int32_t m_idWrapped;
    /* 0030 */ CSRWLock m_idLock[slotCount];
    /* 00F0 */ TSList<T, TSGetLink<T> > m_idList[slotCount];

    TInstanceIdTable();
    ~TInstanceIdTable();
    INSTANCEID Insert(T* instance);
    T* Ptr(INSTANCEID id, int32_t lockMode, int32_t* findMask, int32_t unused1, int32_t unused2);
    void Unlock(uint32_t slot, int32_t lockMode);

    private:
    INSTANCEID NextId();
};

// VERIFY: 1a8956cc manual
// OFFSET: 0x0047D840
template <class T, int slotCount>
TInstanceIdTable<T, slotCount>::TInstanceIdTable() {
}

// VERIFY: de41af07 manual
// OFFSET: 0x0047D7B0
template <class T, int slotCount>
TInstanceIdTable<T, slotCount>::~TInstanceIdTable() {
}

// VERIFY: 21cebef7 manual
// OFFSET: 0x0047E3D0
template <class T, int slotCount>
INSTANCEID TInstanceIdTable<T, slotCount>::Insert(T* instance) {
    if (instance->m_id) {
        return instance->m_id;
    }

    this->m_idCritsect.Enter();

    INSTANCEID id = this->NextId();
    instance->m_id = id;

    uint32_t slot = id & (slotCount - 1);
    CSRWLock* idLock = &this->m_idLock[slot];

    idLock->Enter(1);
    this->m_idList[slot].LinkToTail(instance);
    idLock->Leave(1);

    this->m_idCritsect.Leave();

    return id;
}

// VERIFY: 7b64f0e7 manual
// OFFSET: 0x0047D0E0
template <class T, int slotCount>
T* TInstanceIdTable<T, slotCount>::Ptr(INSTANCEID id, int32_t lockMode, int32_t* findMask, int32_t unused1, int32_t unused2) {
    if (id) {
        uint32_t slot = id & (slotCount - 1);
        CSRWLock* idLock = &this->m_idLock[slot];

        idLock->Enter(lockMode);

        for (T* ptr = this->m_idList[slot].Head(); ptr; ptr = this->m_idList[slot].Next(ptr)) {
            if (ptr->m_id == id) {
                *findMask = slot + (lockMode ? slotCount : 0);
                return ptr;
            }
        }

        idLock->Leave(lockMode);
    }

    *findMask = -1;
    return nullptr;
}

// INLINED
template <class T, int slotCount>
void TInstanceIdTable<T, slotCount>::Unlock(uint32_t slot, int32_t lockMode) {
    this->m_idLock[slot].Leave(lockMode);
}

// INLINED
template <class T, int slotCount>
INSTANCEID TInstanceIdTable<T, slotCount>::NextId() {
    INSTANCEID id;
    int32_t exists;

    do {
        while (true) {
            id = ++this->m_id;

            if (id) {
                break;
            }

            this->m_idWrapped = 1;
        }

        if (!this->m_idWrapped) {
            break;
        }

        uint32_t slot = id & (slotCount - 1);
        CSRWLock* idLock = &this->m_idLock[slot];

        idLock->Enter(0);

        exists = 0;

        for (T* ptr = this->m_idList[slot].Head(); ptr; ptr = this->m_idList[slot].Next(ptr)) {
            if (ptr->m_id == id) {
                exists = 1;
                break;
            }
        }

        idLock->Leave(0);
    } while (exists);

    return id;
}

#endif
