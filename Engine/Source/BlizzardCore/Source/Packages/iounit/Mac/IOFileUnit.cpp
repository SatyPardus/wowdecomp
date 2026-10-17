#if defined(__APPLE__)

#include <algorithm>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOFileUnit.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/Memory/MemoryStorm.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"

namespace {
std::string NativePath(const char* path) {
    std::string out = path ? path : "";
    std::replace(out.begin(), out.end(), '\\', '/');
    return out;
}
}

bool IOFileUnit::ReadNative(uint32_t offset, const char* path, void* buffer, uint32_t count, uint32_t* bytesRead) {
    int fd = open(NativePath(path).c_str(), O_RDONLY);
    if (fd < 0) {
        return false;
    }
    ssize_t got = pread(fd, buffer, count, offset);
    close(fd);
    if (got < 0) {
        return false;
    }
    if (bytesRead) {
        *bytesRead = static_cast<uint32_t>(got);
    }
    return true;
}

IOUnbufferedFileUnit::IOUnbufferedFileUnit(Blizzard::File::StreamRecord* stream, bool ownsMemory) : IOFileUnit(stream, ownsMemory) {
    Blizzard::File::FileInfo* info = Blizzard::File::GetFileInfo(stream);
    this->m_size = info->m_size;
    this->m_alignMask = 0xFFF;
    this->m_buffer = Blizzard::Memory::Allocate(this->m_alignMask + 0x100000);
    this->m_alignedBuffer = reinterpret_cast<void*>(~static_cast<uintptr_t>(this->m_alignMask) & (reinterpret_cast<uintptr_t>(this->m_buffer) + this->m_alignMask));
}

#endif
