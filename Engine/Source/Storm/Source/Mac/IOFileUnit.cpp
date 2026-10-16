#if defined(__APPLE__)

#include <errno.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>

#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/IOUnit/IOFileUnit.h"

namespace Storm {

namespace {

constexpr uint32_t GENERIC_READ_ = 0x80000000u;
constexpr uint32_t GENERIC_WRITE_ = 0x40000000u;
constexpr uint32_t FILE_SHARE_READ_ = 0x00000001u;
constexpr uint32_t FILE_ATTRIBUTE_NORMAL_ = 0x00000080u;
constexpr uint32_t FILE_FLAG_NO_BUFFERING_ = 0x20000000u;
constexpr uint32_t FILE_FLAG_WRITE_THROUGH_ = 0x80000000u;
constexpr uint32_t CREATE_NEW_ = 1;
constexpr uint32_t CREATE_ALWAYS_ = 2;
constexpr uint32_t OPEN_EXISTING_ = 3;
constexpr uint32_t OPEN_ALWAYS_ = 4;
constexpr uint32_t TRUNCATE_EXISTING_ = 5;

struct PosixFileHandle {
    int fd = -1;
    std::mutex mutex;
};

std::string NormalizePath(const char* path) {
    std::string result = path ? path : "";

    for (char& c : result) {
        if (c == '\\') {
            c = '/';
        }
    }

    return result;
}

int OpenFlags(const IOFileParam* param) {
    int flags = 0;
    bool read = (param->m_access & GENERIC_READ_) != 0;
    bool write = (param->m_access & GENERIC_WRITE_) != 0;

    if (read && write) {
        flags |= O_RDWR;
    } else if (write) {
        flags |= O_WRONLY;
    } else {
        flags |= O_RDONLY;
    }

    switch (param->m_creation) {
        case CREATE_NEW_:
            flags |= O_CREAT | O_EXCL;
            break;
        case CREATE_ALWAYS_:
            flags |= O_CREAT | O_TRUNC;
            break;
        case OPEN_EXISTING_:
            break;
        case OPEN_ALWAYS_:
            flags |= O_CREAT;
            break;
        case TRUNCATE_EXISTING_:
            flags |= O_TRUNC;
            break;
        default:
            break;
    }

    if (param->m_flags & FILE_FLAG_WRITE_THROUGH_) {
        flags |= O_SYNC;
    }

    return flags;
}

void* OpenFileHandle(const IOFileParam* param) {
    if (!param || !param->m_name) {
        SErrSetLastError(87);
        return nullptr;
    }

    std::string path = NormalizePath(param->m_name);
    int fd = open(path.c_str(), OpenFlags(param), 0666);

    if (fd < 0) {
        SErrSetLastError(errno);
        return nullptr;
    }

    PosixFileHandle* handle = new PosixFileHandle();
    handle->fd = fd;
    return handle;
}

uint32_t AlignmentMaskForPath(const char* path) {
    std::string normalized = NormalizePath(path);
    std::string probe = normalized;

    while (!probe.empty()) {
        struct statfs fs = {};

        if (statfs(probe.c_str(), &fs) == 0 && fs.f_bsize > 0 && !(fs.f_bsize & (fs.f_bsize - 1))) {
            return static_cast<uint32_t>(fs.f_bsize - 1);
        }

        size_t slash = probe.find_last_of('/');

        if (slash == std::string::npos) {
            break;
        }

        if (slash == 0) {
            probe = "/";
        } else {
            probe.resize(slash);
        }
    }

    return 0xFFFF;
}

} // namespace

IOUnit::IOUnit() {
    this->m_refcount = nullptr;
    this->m_count = 1;
}

void IOUnit::Release() {
    IORefcount* refcount = this->m_refcount;

    if (--this->m_count == 0) {
        delete this;
    }

    if (refcount) {
        refcount->Release();
    }
}

IOWin32FileUnit::IOWin32FileUnit(void* handle) {
    this->m_handle = handle;
    this->m_position = -1;
    this->Seek(0);
}

bool IOWin32FileUnit::Seek(int64_t offset) {
    PosixFileHandle* handle = static_cast<PosixFileHandle*>(this->m_handle);

    if (!handle) {
        this->m_position = -1;
        return false;
    }

    if (this->m_position != offset) {
        if (lseek(handle->fd, offset, SEEK_SET) == -1) {
            this->m_position = -1;
            SErrSetLastError(errno);
            return false;
        }

        this->m_position = offset;
    }

    return true;
}

bool IOWin32FileUnit::Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead) {
    uint32_t minimum = 1;
    uint32_t localRead;

    if (!bytesRead) {
        minimum = count;
        bytesRead = &localRead;
    }

    *bytesRead = 0;
    PosixFileHandle* handle = static_cast<PosixFileHandle*>(this->m_handle);

    if (!handle) {
        return false;
    }

    std::lock_guard<std::mutex> lock(handle->mutex);

    for (int32_t tries = 0; tries < 3; tries++) {
        if (!this->Seek(offset)) {
            return false;
        }

        ssize_t result = read(handle->fd, buffer, count);

        if (result >= 0) {
            *bytesRead = static_cast<uint32_t>(result);
            this->m_position = offset + *bytesRead;
            return *bytesRead >= minimum;
        }

        this->m_position = -1;
        SErrSetLastError(errno);
        usleep(10000);
    }

    return false;
}

bool IOWin32FileUnit::Write(const void* buffer, int64_t offset, uint32_t count) {
    PosixFileHandle* handle = static_cast<PosixFileHandle*>(this->m_handle);

    if (!handle) {
        return false;
    }

    std::lock_guard<std::mutex> lock(handle->mutex);

    if (!this->Seek(offset)) {
        return false;
    }

    const uint8_t* current = static_cast<const uint8_t*>(buffer);
    uint32_t remaining = count;

    while (remaining) {
        ssize_t written = write(handle->fd, current, remaining);

        if (written <= 0) {
            this->m_position = -1;
            SErrSetLastError(errno);
            return false;
        }

        current += written;
        remaining -= static_cast<uint32_t>(written);
        this->m_position += written;
    }

    return true;
}

bool IOWin32FileUnit::Flush() {
    PosixFileHandle* handle = static_cast<PosixFileHandle*>(this->m_handle);
    return handle && fsync(handle->fd) == 0;
}

bool IOWin32FileUnit::GetFileSize(int64_t* size) {
    PosixFileHandle* handle = static_cast<PosixFileHandle*>(this->m_handle);
    struct stat st = {};

    if (!handle || !size || fstat(handle->fd, &st) != 0) {
        if (size) {
            *size = 0;
        }

        SErrSetLastError(errno);
        return false;
    }

    *size = st.st_size;
    return true;
}

bool IOWin32FileUnit::SetFileSize(int64_t size) {
    PosixFileHandle* handle = static_cast<PosixFileHandle*>(this->m_handle);

    if (!handle) {
        return false;
    }

    if (ftruncate(handle->fd, size) != 0) {
        SErrSetLastError(errno);
        return false;
    }

    this->m_position = -1;
    return true;
}

void* IOWin32FileUnit::Win32Handle() {
    return this->m_handle;
}

bool IOUnbufferedFileUnit::Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead) {
    if (!(this->m_alignMask & reinterpret_cast<uintptr_t>(buffer)) && !(this->m_alignMask & count)) {
        return this->m_unit->Read(buffer, offset, count, bytesRead);
    }

    if (!this->m_buffer) {
        return false;
    }

    uint32_t total = 0;
    bool result = true;

    while (count) {
        uint32_t chunk = count >= 0x100000 ? 0x100000 : count;
        uint32_t size = chunk;

        if (chunk & this->m_alignMask) {
            size = (this->m_alignMask + chunk) & ~this->m_alignMask;
        }

        uint32_t readBytes;
        result = this->m_unit->Read(this->m_buffer, offset, size, &readBytes);

        if (!result) {
            break;
        }

        readBytes = size < chunk ? size : chunk;
        memcpy(buffer, this->m_buffer, readBytes);
        buffer = static_cast<uint8_t*>(buffer) + readBytes;
        offset += readBytes;
        count -= readBytes;
        total += readBytes;

        if (readBytes < chunk) {
            break;
        }
    }

    if (!bytesRead) {
        return count == 0;
    }

    *bytesRead = total;
    return result;
}

bool IOUnbufferedFileUnit::Write(const void* buffer, int64_t offset, uint32_t count) {
    if (!(this->m_alignMask & reinterpret_cast<uintptr_t>(buffer)) && !(this->m_alignMask & count)) {
        return this->m_unit->Write(buffer, offset, count);
    }

    if (!this->m_buffer) {
        return false;
    }

    bool result = true;

    while (count) {
        uint32_t chunk = count >= 0x100000 ? 0x100000 : count;
        memcpy(this->m_buffer, buffer, chunk);
        buffer = static_cast<const uint8_t*>(buffer) + chunk;

        uint32_t size = chunk;

        if (chunk & this->m_alignMask) {
            size = (this->m_alignMask + chunk) & ~this->m_alignMask;
        }

        if (size > chunk) {
            memset(static_cast<uint8_t*>(this->m_buffer) + chunk, 0, size - chunk);
        }

        result = this->m_unit->Write(this->m_buffer, offset, size);

        if (!result) {
            break;
        }

        offset += chunk;
        count -= chunk;
    }

    if (offset > this->m_size) {
        this->m_size = offset;
    }

    return result;
}

bool IOUnbufferedFileUnit::Flush() {
    return this->m_unit->Flush();
}

bool IOUnbufferedFileUnit::SetFileSize(int64_t size) {
    this->m_size = size;
    return this->m_unit->SetFileSize((size + this->m_alignMask) & ~this->m_alignMask);
}

bool IOUnbufferedFileUnit::GetFileSize(int64_t* size) {
    *size = this->m_size;
    return true;
}

IOUnit::~IOUnit() {
}

IOWin32FileUnit::~IOWin32FileUnit() {
    PosixFileHandle* handle = static_cast<PosixFileHandle*>(this->m_handle);

    if (handle) {
        close(handle->fd);
        delete handle;
    }

    this->m_handle = nullptr;
}

IOUnbufferedFileUnit::IOUnbufferedFileUnit(void* handle, const IOFileParam* param) {
    this->m_unit = new IOWin32FileUnit(handle);
    this->m_size = 0;
    this->m_unit->GetFileSize(&this->m_size);
    SStrCopy(this->m_path, param->m_name, sizeof(this->m_path));

    this->m_alignMask = AlignmentMaskForPath(this->m_path);

    if (posix_memalign(&this->m_buffer, this->m_alignMask + 1, 0x100000) != 0) {
        this->m_buffer = nullptr;
    }
}

void* IOUnbufferedFileUnit::Win32Handle() {
    return this->m_unit->Win32Handle();
}

IOUnbufferedFileUnit* IOUnbufferedFileUnit::Open(const IOFileParam* param) {
    void* handle = OpenFileHandle(param);

    if (!handle) {
        return nullptr;
    }

    return new IOUnbufferedFileUnit(handle, param);
}

IOWin32FileUnit* IOWin32FileUnit::Open(const IOFileParam* param) {
    void* handle = OpenFileHandle(param);

    if (!handle) {
        return nullptr;
    }

    return new IOWin32FileUnit(handle);
}

IOUnit* IOFileUnit::Create(const IOFileParam& param, IOUnit* next) {
    if (next) {
        return nullptr;
    }

    if (param.m_flags & FILE_FLAG_NO_BUFFERING_) {
        return IOUnbufferedFileUnit::Open(&param);
    }

    return IOWin32FileUnit::Open(&param);
}

IOUnbufferedFileUnit::~IOUnbufferedFileUnit() {
    free(this->m_buffer);

    if (this->m_unit) {
        delete this->m_unit;
    }

    IOFileParam param;
    param.m_name = this->m_path;
    param.m_access = GENERIC_WRITE_;
    param.m_share = FILE_SHARE_READ_;
    param.m_creation = OPEN_EXISTING_;
    param.m_flags = FILE_ATTRIBUTE_NORMAL_;

    IOUnit* unit = IOWin32FileUnit::Open(&param);

    if (unit) {
        unit->SetFileSize(this->m_size);
        unit->Release();
    }
}

} // namespace Storm

#endif
