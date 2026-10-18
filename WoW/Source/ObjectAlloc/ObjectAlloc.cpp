#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include "WoW/Source/ObjectAlloc/ObjectAlloc.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/SLock.h"

// STATIC: 0x00B4AFC0
static OBJALLOCGLOBALS s_globals;

// STATIC: 0x00B4AFD0
static SCritSect s_globalsLock;

// INLINED
CObjectHeap::CObjectHeap() {
    this->m_obj = nullptr;
    this->m_indexStack = nullptr;
    this->m_allocated = 0;
}

// INLINED
CObjectHeap::CObjectHeap(const CObjectHeap& source) {
    this->m_obj = source.m_obj;
    this->m_indexStack = source.m_indexStack;
    const_cast<CObjectHeap&>(source).m_obj = nullptr;
    const_cast<CObjectHeap&>(source).m_indexStack = nullptr;
    this->m_allocated = source.m_allocated;
}

// INLINED
CObjectHeap::~CObjectHeap() {
    if (this->m_obj) {
        SMemFree(this->m_obj, __FILE__, __LINE__, 0);
    }
}

// INLINED
void CObjectHeap::Delete(uint32_t index, uint32_t objSize, uint32_t count) {
    if (this->m_allocated && this->m_indexStack) {
        this->m_indexStack[--this->m_allocated] = index;
    }
}

// INLINED
void* CObjectHeap::Ptr(uint32_t index, uint32_t objSize, uint32_t count) {
    return static_cast<char*>(this->m_obj) + size_t(index) * objSize;
}

// INLINED
CObjectHeapList::CObjectHeapList() {
    this->m_objSize = 0;
    this->m_objsPerBlock = 128;
    this->m_numFullHeaps = 0;
    this->m_hasEmptyHeaps = 0;
    this->m_deleteCount = 0;
    this->m_fullestHeap = 0;
    this->m_numAllocs = 0;
    this->m_uint80 = 0;
    this->m_freeEmptyHeaps = 1;
}

// VERIFY: 1ee37614 manual
// OFFSET: 0x004D2AA0
bool CObjectHeap::Allocate(uint32_t objSize, uint32_t count, const char* name) {
    this->m_obj = SMemAlloc(count * (objSize + sizeof(uint32_t)), name, 0, 0x2000000);

    if (!this->m_obj) {
        return false;
    }

    this->m_indexStack = reinterpret_cast<uint32_t*>(static_cast<char*>(this->m_obj) + size_t(count) * objSize);

    for (uint32_t i = 0; i < count; i++) {
        this->m_indexStack[i] = i;
    }

    this->m_allocated = 0;

    return this->m_obj != nullptr;
}

// VERIFY: 763163e1 auto
// OFFSET: 0x004D2B10
OBJALLOCGLOBALS* GetObjAllocGlobals() {
    s_globalsLock.Enter();

    return &s_globals;
}

// VERIFY: 3bafdd5f auto
// OFFSET: 0x004D2B20
void ReleaseObjAllocGlobals() {
    s_globalsLock.Leave();
}

// VERIFY: 7d28d1a5 manual
// OFFSET: 0x004D2B30
void CObjectHeapList::FreeEmptyHeaps() {
    uint32_t count = this->m_heaps.Count();
    uint32_t allocated = 0;
    uint32_t capacity = 0;
    uint32_t emptyHeaps = 0;

    for (uint32_t i = 0; i < count; i++) {
        CObjectHeap& heap = this->m_heaps[i];

        if (heap.m_obj) {
            capacity += this->m_objsPerBlock;

            if (heap.m_allocated) {
                allocated += heap.m_allocated;
            } else {
                emptyHeaps++;
            }
        }
    }

    if (!emptyHeaps) {
        this->m_hasEmptyHeaps = 0;
        return;
    }

    uint32_t threshold = allocated + ((3 * this->m_objsPerBlock) >> 1);

    if (threshold < capacity) {
        for (uint32_t i = count; i > 0; i--) {
            if (threshold >= capacity || !emptyHeaps) {
                break;
            }

            CObjectHeap& heap = this->m_heaps[i - 1];

            if (heap.m_obj && !heap.m_allocated) {
                if (heap.m_obj) {
                    SMemFree(heap.m_obj, __FILE__, __LINE__, 0);
                }

                heap.m_obj = nullptr;
                capacity -= this->m_objsPerBlock;
                emptyHeaps--;
                this->m_deleteCount = 0;
            }
        }
    }

    this->m_hasEmptyHeaps = emptyHeaps != 0;
}

// VERIFY: a5a75ca6 manual
// OFFSET: 0x004D2C30
uint32_t CObjectHeapList::BlocksAllocated() {
    uint32_t allocated = 0;

    for (uint32_t i = 0; i < this->m_heaps.Count(); i++) {
        allocated += this->m_heaps[i].m_allocated;
    }

    return allocated;
}

// VERIFY: 8e6272eb auto
// OFFSET: 0x004D2C50
uint32_t CObjectHeapList::TotalHeaps() {
    uint32_t heaps = 0;

    for (uint32_t i = 0; i < this->m_heaps.Count(); i++) {
        if (this->m_heaps[i].m_obj) {
            heaps++;
        }
    }

    return heaps;
}

// VERIFY: fdc6e79c auto
// OFFSET: 0x004D2C80
uint32_t CObjectHeapList::PercentUsed() {
    uint32_t capacity = 0;
    uint32_t allocated = 0;

    for (uint32_t i = 0; i < this->m_heaps.Count(); i++) {
        if (this->m_heaps[i].m_obj) {
            capacity += this->m_objsPerBlock;
            allocated += this->m_heaps[i].m_allocated;
        }
    }

    return capacity ? 100 * allocated / capacity : 0;
}

// VERIFY: 6ca854e3 auto
// OFFSET: 0x004D2CC0
int32_t CObjectHeap::New(uint32_t objSize, uint32_t count, uint32_t* index, const char* name, void** ptr, bool zero) {
    if (this->m_allocated >= count) {
        return 0;
    }

    if (!this->m_obj && !this->Allocate(objSize, count, name)) {
        return 0;
    }

    *index = this->m_indexStack[this->m_allocated++];

    if (zero) {
        memset(static_cast<char*>(this->m_obj) + size_t(objSize) * *index, 0, objSize);
    }

    if (ptr) {
        *ptr = static_cast<char*>(this->m_obj) + size_t(objSize) * *index;
    }

    return 1;
}

// VERIFY: ec42cc99 auto
// OFFSET: 0x004D2D40
void* ObjectPtr(uint32_t heapId, uint32_t memHandle) {
    s_globalsLock.Enter();

    CObjectHeapList& list = s_globals.objects[heapId];
    void* ptr = list.m_heaps[memHandle / list.m_objsPerBlock].Ptr(memHandle % list.m_objsPerBlock, list.m_objSize, list.m_objsPerBlock);

    s_globalsLock.Leave();

    return ptr;
}

// VERIFY: 1c705ebd manual
// OFFSET: 0x004D2F00
void CObjectHeapList::Delete(uint32_t index) {
    uint32_t heapIndex = index / this->m_objsPerBlock;
    CObjectHeap& heap = this->m_heaps[heapIndex];

    if (heap.m_allocated == this->m_objsPerBlock) {
        this->m_numFullHeaps--;
    }

    heap.Delete(index % this->m_objsPerBlock, this->m_objSize, this->m_objsPerBlock);

    if (!this->m_freeEmptyHeaps) {
        return;
    }

    if (this->m_heaps[heapIndex].m_obj && !this->m_heaps[heapIndex].m_allocated) {
        this->m_hasEmptyHeaps = 1;
    }

    if (this->m_hasEmptyHeaps && !(this->m_deleteCount++ & ((this->m_objsPerBlock - 1) >> 2))) {
        this->FreeEmptyHeaps();
    }
}

// VERIFY: d5d48ee3 manual
// OFFSET: 0x004D2F90
static void HeapUse() {
    s_globalsLock.Enter();

    uint32_t count = s_globals.objects.Count();
    uint32_t used = 0;
    uint32_t allocated = 0;

    DebugLog("%u Heaps in use:\n", count);

    for (uint32_t i = 0; i < count; i++) {
        CObjectHeapList& list = s_globals.objects[i];

        DebugLog("    \"%24s\" %3u blocks, %5u objs per block (%6u bytes per obj): %6u objs allocated (%2d%%)\n", list.m_heapName, list.TotalHeaps(), list.m_objsPerBlock, list.m_objSize, list.BlocksAllocated(), list.PercentUsed());

        used += s_globals.objects[i].m_objSize * s_globals.objects[i].BlocksAllocated();
        allocated += s_globals.objects[i].TotalHeaps() * s_globals.objects[i].m_objSize * s_globals.objects[i].m_objsPerBlock;
    }

    DebugLog("%u total object bytes used, %u bytes allocated for heaps\n", used, allocated);

    s_globalsLock.Leave();
}

// VERIFY: 816bb6c5 auto
// OFFSET: 0x004D3100
void ObjectFree(uint32_t heapId, uint32_t memHandle) {
    s_globalsLock.Enter();
    s_globals.objects[heapId].Delete(memHandle);
    s_globalsLock.Leave();
}

// VERIFY: 862b80b0 manual
// OFFSET: 0x004D3250
int32_t CObjectHeapList::New(uint32_t* index, void** ptr, bool zero) {
    CObjectHeap* heap = nullptr;
    uint32_t count = this->m_heaps.Count();

    if (this->m_fullestHeap < count && this->m_heaps[this->m_fullestHeap].m_allocated != this->m_objsPerBlock && this->m_heaps[this->m_fullestHeap].m_obj) {
        heap = &this->m_heaps[this->m_fullestHeap];
    } else if (count == this->m_numFullHeaps) {
        this->m_fullestHeap = count;
        heap = this->m_heaps.New();

        if (!heap->Allocate(this->m_objSize, this->m_objsPerBlock, this->m_heapName)) {
            return 0;
        }
    } else {
        for (uint32_t i = 0; i < count; i++) {
            if (this->m_heaps[i].m_allocated != this->m_objsPerBlock) {
                this->m_fullestHeap = i;
                heap = &this->m_heaps[i];

                if (heap->m_obj) {
                    break;
                }
            }
        }
    }

    if (!heap->New(this->m_objSize, this->m_objsPerBlock, index, this->m_heapName, ptr, zero)) {
        return 0;
    }

    *index += this->m_fullestHeap * this->m_objsPerBlock;

    if (heap->m_allocated == this->m_objsPerBlock) {
        this->m_numFullHeaps++;
    }

    this->m_numAllocs++;

    return 1;
}

// VERIFY: 5a8067f9 auto
// OFFSET: 0x004D3330
int32_t ObjectAlloc(uint32_t heapId, uint32_t* memHandle, void** objectPtr, bool zero) {
    if (!memHandle) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    *memHandle = 0;
    s_globalsLock.Enter();

    if (heapId >= s_globals.objects.Count()) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        s_globalsLock.Leave();
        return 0;
    }

    uint32_t handle;
    void* ptr;

    if (!s_globals.objects[heapId].New(&handle, &ptr, zero)) {
        s_globalsLock.Leave();
        return 0;
    }

    if (objectPtr) {
        *objectPtr = ptr;
    }

    s_globalsLock.Leave();
    *memHandle = handle;

    return 1;
}

// VERIFY: 493549ac auto
// OFFSET: 0x004D3580
void ObjectAllocDestroy() {
    HeapUse();

    s_globalsLock.Enter();
    s_globals.objects.Clear();
    s_globalsLock.Leave();
}

// VERIFY: 739a3bb6 auto
// OFFSET: 0x004D3650
uint32_t ObjectAllocAddHeap(uint32_t objectSize, uint32_t objectsPerBlock, const char* name, bool freeEmptyHeaps) {
    if (!objectSize) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

#if defined(__APPLE__)
    // A game object may have trailing 32-bit mirror fields. Its payload length
    // is then only four-byte aligned even though the next native object needs
    // pointer/max_align_t alignment. Pad the slot, not the object or wire data.
    constexpr uint32_t alignment = alignof(std::max_align_t);
    static_assert((alignment & (alignment - 1)) == 0, "Object heap alignment");
    if (!objectsPerBlock || objectSize > std::numeric_limits<uint32_t>::max() - (alignment - 1)) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }
    objectSize = (objectSize + alignment - 1) & ~(alignment - 1);
#endif

    s_globalsLock.Enter();

    uint32_t heapId = s_globals.objects.Count();
    CObjectHeapList* list = s_globals.objects.New();
    list->m_objSize = objectSize;
    list->m_objsPerBlock = objectsPerBlock;
    SStrCopy(list->m_heapName, name, sizeof(list->m_heapName));
    list->m_freeEmptyHeaps = freeEmptyHeaps;

    s_globalsLock.Leave();

    return heapId;
}
