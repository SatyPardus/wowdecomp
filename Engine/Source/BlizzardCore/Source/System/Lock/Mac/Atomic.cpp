#if defined(__APPLE__)

#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"

int32_t Blizzard::Lock::Atomic::Increment() {
    return __atomic_add_fetch(&this->m_value, 1, __ATOMIC_SEQ_CST);
}

int32_t Blizzard::Lock::Atomic::Decrement() {
    return __atomic_sub_fetch(&this->m_value, 1, __ATOMIC_SEQ_CST);
}

#endif
