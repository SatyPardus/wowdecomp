#if defined(__APPLE__)

#include <cstdint>
#include <cstring>
#include <pthread.h>
#include "Engine/Source/Os/OsCall.h"
#include "Engine/Source/Storm/H/storm.h"

static pthread_key_t s_contextKey;
static pthread_once_t s_keyOnce = PTHREAD_ONCE_INIT;

static void MakeKey() {
    pthread_key_create(&s_contextKey, nullptr);
}

void OsCallSetContext(void* context) {
    pthread_once(&s_keyOnce, MakeKey);
    pthread_setspecific(s_contextKey, context);
}

void OsCallResetContext(void* context) {
    pthread_once(&s_keyOnce, MakeKey);
    if (pthread_getspecific(s_contextKey) == context) {
        pthread_setspecific(s_contextKey, nullptr);
    }
}

OsCallContext::~OsCallContext() {
}

void OsCallDestroy() {
    pthread_once(&s_keyOnce, MakeKey);
    pthread_setspecific(s_contextKey, nullptr);
}

void OsCallInitialize(const char* name) {
    (void)name;
    pthread_once(&s_keyOnce, MakeKey);
}

void* OsCallInitializeContext(const char* name) {
    OsCallContext* context = new OsCallContext();
    memset(context, 0, sizeof(*context));
    SStrCopy(context->m_name, name ? name : "", sizeof(context->m_name));
    context->unk_000C = -1;
    return context;
}

void OsCallDestroyContext(void* context) {
    delete static_cast<OsCallContext*>(context);
}

#endif
