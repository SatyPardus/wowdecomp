#if defined(__APPLE__)
#include <algorithm>
#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <new>
#include <sys/stat.h>
#include <unistd.h>
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/Posix/IOPosixFileUnit.h"
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOUnitContainer.h"

static_assert(sizeof(off_t) == 8, "Game archives require 64-bit file offsets");

IOPosixFileUnit::IOPosixFileUnit(int descriptor) : IOUnit(nullptr, true), m_descriptor(descriptor) {}
IOPosixFileUnit::~IOPosixFileUnit() { Close(); }

IOPosixFileUnit* IOPosixFileUnit::Open(const char* path, int flags) {
    int descriptor;
    do {
        descriptor = open(path, flags | O_CLOEXEC, 0600);
    } while (descriptor == -1 && errno == EINTR);
    if (descriptor == -1) {
        return nullptr;
    }
    bool ownsMemory;
    void* memory = IOUnitContainer::Alloc(nullptr, sizeof(IOPosixFileUnit), &ownsMemory);
    if (!memory) {
        close(descriptor);
        return nullptr;
    }
    return new (memory) IOPosixFileUnit(descriptor);
}

IOPosixFileUnit* IOPosixFileUnit::OpenRead(const char* path) { return Open(path, O_RDONLY); }
IOPosixFileUnit* IOPosixFileUnit::CreateWritable(const char* path) { return Open(path, O_RDWR | O_CREAT | O_EXCL); }
uint32_t IOPosixFileUnit::GetType() { return 'file'; }

bool IOPosixFileUnit::Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead) {
    if (bytesRead) { *bytesRead = 0; }
    if (offset < 0 || offset > INT64_MAX - count || (!buffer && count)) {
        errno = EINVAL;
        return false;
    }
    uint32_t total = 0;
    while (total < count) {
        const size_t chunk = std::min<uint32_t>(count - total, INT_MAX);
        const ssize_t amount = pread(m_descriptor, static_cast<char*>(buffer) + total, chunk, offset + total);
        if (amount < 0) {
            if (errno == EINTR) { continue; }
            if (bytesRead) { *bytesRead = total; }
            return false;
        }
        if (!amount) { break; }
        total += static_cast<uint32_t>(amount);
    }
    if (bytesRead) { *bytesRead = total; }
    // Match IOFileUnit: a caller with bytesRead accepts a short read, but a
    // caller without it requires the entire requested range.
    return bytesRead ? (total != 0 || count == 0) : total == count;
}

bool IOPosixFileUnit::Write(const void* buffer, int64_t offset, uint32_t count) {
    if (offset < 0 || offset > INT64_MAX - count || (!buffer && count)) {
        errno = EINVAL;
        return false;
    }
    uint32_t total = 0;
    while (total < count) {
        const size_t chunk = std::min<uint32_t>(count - total, INT_MAX);
        const ssize_t amount = pwrite(m_descriptor, static_cast<const char*>(buffer) + total, chunk, offset + total);
        if (amount < 0 && errno == EINTR) { continue; }
        if (amount <= 0) { return false; }
        total += static_cast<uint32_t>(amount);
    }
    return true;
}

bool IOPosixFileUnit::Flush() {
    int result;
    do { result = fsync(m_descriptor); } while (result == -1 && errno == EINTR);
    return result == 0;
}
bool IOPosixFileUnit::GetFileSize(int64_t* size) {
    struct stat status;
    if (!size || fstat(m_descriptor, &status) != 0) { return false; }
    *size = status.st_size;
    return true;
}
bool IOPosixFileUnit::SetFileSize(int64_t size) {
    if (size < 0) { errno = EINVAL; return false; }
    return ftruncate(m_descriptor, size) == 0;
}
void* IOPosixFileUnit::Win32Handle() { errno = ENOTSUP; return nullptr; }
bool IOPosixFileUnit::Close() {
    if (m_descriptor < 0) { return true; }
    const int descriptor = m_descriptor;
    m_descriptor = -1;
    // Never retry close: another thread can already have reused the descriptor.
    return close(descriptor) == 0;
}
#endif
