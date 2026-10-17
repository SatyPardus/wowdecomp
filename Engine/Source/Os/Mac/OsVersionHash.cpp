#if defined(__APPLE__)

#include <cstdint>
#include <cstring>
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Os/OsVersionHash.h"
#include "Engine/Source/Os/Path.h"
#include "Engine/Source/Storm/H/storm.h"

OsVersionHash::OsVersionHash() : m_entries(nullptr), m_count(0), m_capacity(0) {
}

OsVersionHash::~OsVersionHash() {
    if (m_entries) {
        SMemFree(m_entries, __FILE__, __LINE__, 0);
    }
}

bool OsVersionHash::AddEntry(const OsVersionHashEntry& entry) {
    if (m_count >= m_capacity) {
        uint32_t newCapacity = m_capacity ? m_capacity * 2 : 256;
        void* memory = m_entries ? SMemReAlloc(m_entries, newCapacity * sizeof(OsVersionHashEntry), __FILE__, __LINE__, 0) : SMemAlloc(newCapacity * sizeof(OsVersionHashEntry), __FILE__, __LINE__, 0);
        if (!memory) {
            return false;
        }
        m_entries = static_cast<OsVersionHashEntry*>(memory);
        m_capacity = newCapacity;
    }
    m_entries[m_count++] = entry;
    return true;
}

int32_t IOsVersionChecksum(IOsVersionHashCallback& callback, const char* path, int32_t loaded) {
    char exe[1024];
    if (!path && loaded) {
        GetModuleFileNameUTF8(nullptr, exe, sizeof(exe));
        path = exe;
    }
    if (!path) {
        return 0;
    }

    HOSFILE file = OsCreateFile(path, OS_GENERIC_READ, 1, OS_OPEN_EXISTING, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);
    if (file == OS_INVALID_FILE) {
        return 0;
    }

    char buffer[16384];
    uint32_t bytesRead = 0;
    int32_t result = 1;
    while (OsReadFile(file, buffer, sizeof(buffer), &bytesRead) && bytesRead) {
        callback.Process(buffer, bytesRead);
        if (bytesRead < sizeof(buffer)) {
            break;
        }
    }
    OsCloseFile(file);
    return result;
}

OsFileVersionInfo::OsFileVersionInfo(const char* filename) : m_fileVersion(), m_productVersion() {
    (void)filename;
    m_data.Clear();
    memset(m_fileVersion, 0, sizeof(m_fileVersion));
    memset(m_productVersion, 0, sizeof(m_productVersion));
}

#endif
