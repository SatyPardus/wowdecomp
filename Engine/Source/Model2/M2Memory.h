#ifndef ENGINE_SOURCE_MODEL2_M2MEMORY_H
#define ENGINE_SOURCE_MODEL2_M2MEMORY_H

#include "Engine/Source/Base/SIMD.h"
#include "Engine/Source/Os/OsMemory.h"

// Apple malloc zones (and sanitizer heaps) can reserve more than the signed
// 32-bit relative-pointer range. Give model roots their own VM allocation so
// external animation/expanded vertex arrays can be placed nearby.
inline void* M2AllocData(uint32_t size) {
#if defined(__APPLE__)
    return OsAllocNear(nullptr, size);
#else
    return SIMD::AllocA16(size, __FILE__, __LINE__);
#endif
}
inline void M2FreeData(void* data) {
#if defined(__APPLE__)
    OsFreeNear(data);
#else
    SIMD::FreeA16(data);
#endif
}
#endif
