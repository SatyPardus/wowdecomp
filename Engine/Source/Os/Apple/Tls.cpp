#if defined(__APPLE__)
#include "Engine/Source/Os/Tls.h"
#include <cerrno>
#include <pthread.h>
#include <limits>

uint32_t OsTlsAlloc() {
    pthread_key_t key;
    const int error = pthread_key_create(&key, nullptr);
    if (error) { errno = error; return UINT32_MAX; }
    if (key > UINT32_MAX) { pthread_key_delete(key); errno = EOVERFLOW; return UINT32_MAX; }
    return uint32_t(key);
}
void OsTlsFree(uint32_t index) { pthread_key_delete(index); }
void* OsTlsGetValue(uint32_t index) { return pthread_getspecific(index); }
int32_t OsTlsSetValue(uint32_t index, void* value) {
    const int error = pthread_setspecific(index, value);
    if (error) { errno = error; }
    return error == 0;
}
#endif
