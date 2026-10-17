#include <cstdint>
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Os/Tls.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00B41668
static uint32_t s_tlsIndex;

// VERIFY: b6429b9b auto
// OFFSET: 0x0047CBC0
void PropInitialize() {
    s_tlsIndex = OsTlsAlloc();
    OsTlsSetValue(s_tlsIndex, nullptr);
}

// VERIFY: 0c6ddc99 auto
// OFFSET: 0x0047CBE0
void PropDestroy() {
    OsTlsFree(s_tlsIndex);
}

// VERIFY: c381afef auto
// OFFSET: 0x0047CBF0
void PropSelectContext(HPROPCONTEXT context) {
    OsTlsSetValue(s_tlsIndex, context);
}

// VERIFY: 2ccb0493 auto
// OFFSET: 0x0047CC10
HPROPCONTEXT PropGetSelectedContext() {
    return static_cast<HPROPCONTEXT>(OsTlsGetValue(s_tlsIndex));
}

// VERIFY: 503ebd21 auto
// OFFSET: 0x0047CC20
void* PropGet(PROPERTY id) {
    void** context = static_cast<void**>(OsTlsGetValue(s_tlsIndex));
    return context ? context[id] : nullptr;
}

// VERIFY: 07673705 auto
// OFFSET: 0x0047CC50
void PropSet(PROPERTY id, void* value) {
    void** context = static_cast<void**>(OsTlsGetValue(s_tlsIndex));

    if (context) {
        context[id] = value;
    }
}

// VERIFY: d39268e3 manual
// OFFSET: 0x0047CC70
HPROPCONTEXT PropCreateContext() {
    return static_cast<HPROPCONTEXT>(SMemAlloc(PROPERTIES * sizeof(void*), __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY));
}

void PropDeleteContext(HPROPCONTEXT context) {
    SMemFree(context, __FILE__, __LINE__, 0);
}
