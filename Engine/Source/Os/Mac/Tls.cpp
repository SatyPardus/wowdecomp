#if defined(__APPLE__)

#include <cstdint>
#include <mutex>
#include <pthread.h>
#include <vector>
#include "Engine/Source/Os/Tls.h"

static std::mutex s_tlsMutex;
static std::vector<pthread_key_t> s_tlsKeys;

uint32_t OsTlsAlloc() {
    pthread_key_t key;
    if (pthread_key_create(&key, nullptr) != 0) {
        return 0xFFFFFFFFu;
    }
    std::lock_guard<std::mutex> lock(s_tlsMutex);
    s_tlsKeys.push_back(key);
    return static_cast<uint32_t>(s_tlsKeys.size() - 1);
}

void OsTlsFree(uint32_t index) {
    std::lock_guard<std::mutex> lock(s_tlsMutex);
    if (index < s_tlsKeys.size()) {
        pthread_key_delete(s_tlsKeys[index]);
    }
}

void* OsTlsGetValue(uint32_t index) {
    std::lock_guard<std::mutex> lock(s_tlsMutex);
    if (index >= s_tlsKeys.size()) {
        return nullptr;
    }
    return pthread_getspecific(s_tlsKeys[index]);
}

int32_t OsTlsSetValue(uint32_t index, void* value) {
    std::lock_guard<std::mutex> lock(s_tlsMutex);
    if (index >= s_tlsKeys.size()) {
        return 0;
    }
    return pthread_setspecific(s_tlsKeys[index], value) == 0;
}

#endif
