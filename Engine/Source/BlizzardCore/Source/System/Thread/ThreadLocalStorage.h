#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_THREAD_THREADLOCALSTORAGE_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_THREAD_THREADLOCALSTORAGE_H

#include <cstdint>

namespace Blizzard {
namespace Thread {

typedef void (*TLSDestructor)(void*);

struct TLSSlot {
#if defined(__APPLE__)
    uintptr_t m_key;
#else
    /* 0000 */ uint32_t m_key;
#endif
    /* 0004 */ TLSDestructor m_destructor;
};

template <class T>
class LocalObject {
    public:
    static void Delete(void* ptr);
    static void* New(void* param);

    /* 0000 */ TLSSlot m_slot;

    operator T&();
};

void* RegisterLocalStorage(TLSSlot* slot, void* (*constructor)(void*), void* param, TLSDestructor destructor);

}
}

namespace System_Thread {

extern Blizzard::Thread::TLSSlot* s_slotList[128];
extern uint32_t s_slotListUsed;

bool AllocateLocalStorage(Blizzard::Thread::TLSSlot* slot, Blizzard::Thread::TLSDestructor destructor);
bool AllocateTLSSlot(Blizzard::Thread::TLSSlot* slot, Blizzard::Thread::TLSDestructor destructor);
void InternalSetLocalStorage(const Blizzard::Thread::TLSSlot* slot, const void* value);
void RunTLSDestructors();

}

// VERIFY: 5e833541 auto
// OFFSET: [0x00429860, 0x00436070]
template <class T>
void Blizzard::Thread::LocalObject<T>::Delete(void* ptr) {
    delete static_cast<T*>(ptr);
}

// VERIFY: caab968f auto
// OFFSET: 0x00422280<System_SFile2::SFileFatalInfo>
template <class T>
void* Blizzard::Thread::LocalObject<T>::New(void* param) {
    return new T;
}

// VERIFY: 55d34611 manual
// OFFSET: [0x00422480, 0x00453770, 0x004537E0, 0x00453940]
template <class T>
Blizzard::Thread::LocalObject<T>::operator T&() {
    return *static_cast<T*>(RegisterLocalStorage(&this->m_slot, New, nullptr, Delete));
}

#endif
