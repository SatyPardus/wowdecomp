#if defined(__APPLE__)
#include "Engine/Source/Storm/Source/IOUnit/IOFileUnit.h"
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/Posix/IOPosixFileUnit.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <new>
#include <unistd.h>

namespace Storm {
IORefcount::~IORefcount() = default;
IOUnit::IOUnit() : m_refcount(nullptr), m_count(1) {}
IOUnit::~IOUnit() = default;
void IOUnit::Release() {
    IORefcount* refcount = m_refcount;
    if (--m_count == 0) delete this;
    if (refcount) refcount->Release();
}

namespace {
class NativeFile final : public IOUnit {
public:
    explicit NativeFile(IOPosixFileUnit* file) : file_(file) {}
    ~NativeFile() override { file_->Release(); }
    bool Read(void* data, int64_t offset, uint32_t count, uint32_t* read) override {
        const bool result = file_->Read(data, offset, count, read);
        // This Storm interface requires at least one byte when bytesRead is
        // provided, including a zero-byte request; the other IOUnit API differs.
        return result && (!read || *read != 0);
    }
    bool Write(const void* data, int64_t offset, uint32_t count) override { return file_->Write(data, offset, count); }
    bool Flush() override { return file_->Flush(); }
    bool GetFileSize(int64_t* size) override { return file_->GetFileSize(size); }
    bool SetFileSize(int64_t size) override { return file_->SetFileSize(size); }
    void* Win32Handle() override { errno = ENOTSUP; return nullptr; }
private:
    IOPosixFileUnit* file_;
};
}

IOUnit* IOFileUnit::Create(const IOFileParam& param, IOUnit* next) {
    constexpr uint32_t read = 0x80000000, write = 0x40000000;
    constexpr uint32_t normal = 0x80, uncached = 0x20000000, writeThrough = 0x80000000;
    if (next || !param.m_name || !*param.m_name || !(param.m_access & (read | write)) ||
        (param.m_access & ~(read | write)) || (param.m_share & ~7u) || param.m_creation < 1 || param.m_creation > 5) {
        errno = EINVAL; return nullptr;
    }
    if (param.m_flags & ~(normal | uncached | writeThrough)) { errno = ENOTSUP; return nullptr; }
    if ((param.m_creation == 2 || param.m_creation == 5) && !(param.m_access & write)) { errno = EACCES; return nullptr; }
    char path[PATH_MAX];
    if (!Blizzard::File::MakeAbsolutePath(param.m_name, path, sizeof(path), false)) return nullptr;
    int flags = param.m_access == (read | write) ? O_RDWR : (param.m_access & write) ? O_WRONLY : O_RDONLY;
    const int creation[] = {0, O_CREAT | O_EXCL, O_CREAT | O_TRUNC, 0, O_CREAT, O_TRUNC};
    flags |= creation[param.m_creation] | O_CLOEXEC;
    if (param.m_flags & writeThrough) flags |= O_SYNC;
    int descriptor;
    do { descriptor = open(path, flags, 0600); } while (descriptor < 0 && errno == EINTR);
    if (descriptor < 0) return nullptr;
    // F_NOCACHE gives native uncached IO without exposing Windows sector-sized
    // tail padding to the download or changing its logical file size.
    if ((param.m_flags & uncached) && fcntl(descriptor, F_NOCACHE, 1) < 0) {
        const int error = errno; close(descriptor); errno = error; return nullptr;
    }
    auto* file = IOPosixFileUnit::AdoptDescriptor(descriptor);
    if (!file) return nullptr;
    auto* unit = new (std::nothrow) NativeFile(file);
    if (!unit) { file->Release(); errno = ENOMEM; }
    // Windows cross-process share-denial is not emulated by this local backend.
    return unit;
}
}
#endif
