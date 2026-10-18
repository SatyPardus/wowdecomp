#include "Engine/Source/Os/OsVersionHash.h"
#import <Foundation/Foundation.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {
struct InvalidVersion {};
void Check(bool ok) { if (!ok) throw InvalidVersion{}; }
using Bytes = std::vector<uint8_t>;
void Range(size_t offset, size_t count, size_t size) { Check(offset <= size && count <= size - offset); }
uint16_t U16(const Bytes& bytes, size_t offset) {
    Range(offset, 2, bytes.size());
    return uint16_t(bytes[offset]) | uint16_t(bytes[offset + 1]) << 8;
}
uint32_t U32(const Bytes& bytes, size_t offset) {
    return uint32_t(U16(bytes, offset)) | uint32_t(U16(bytes, offset + 2)) << 16;
}
class File {
    int fd = -1;
    uint64_t size = 0;
public:
    explicit File(const char* path) {
        std::string name(path);
        std::replace(name.begin(), name.end(), '\\', '/');
        fd = open(name.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK);
        struct stat info{};
        if (fd < 0 || fstat(fd, &info) || !S_ISREG(info.st_mode) || info.st_size < 0) {
            if (fd >= 0) close(fd);
            fd = -1;
            throw InvalidVersion{};
        }
        size = uint64_t(info.st_size);
    }
    ~File() { if (fd >= 0) close(fd); }
    Bytes Read(uint64_t offset, size_t count) const {
        Check(offset <= size && count <= size - offset && count <= 16 * 1024 * 1024);
        Bytes result(count);
        size_t done = 0;
        while (done < count) {
            const auto got = pread(fd, result.data() + done, count - done, off_t(offset + done));
            if (got < 0 && errno == EINTR) continue;
            Check(got > 0);
            done += size_t(got);
        }
        return result;
    }
};
struct Versions {
    std::array<uint16_t, 4> file{}, product{};
    Bytes data;
};
Versions VersionBlock(Bytes bytes) {
    // Microsoft VS_VERSIONINFO / VS_FIXEDFILEINFO, little-endian disk data:
    // https://learn.microsoft.com/en-us/windows/win32/menurc/vs-versioninfo
    const size_t length = U16(bytes, 0);
    Check(length <= bytes.size() && U16(bytes, 2) == 52 && U16(bytes, 4) == 0);
    constexpr char key[] = "VS_VERSION_INFO";
    for (size_t i = 0; i < sizeof(key); ++i) Check(U16(bytes, 6 + 2 * i) == uint8_t(key[i]));
    constexpr size_t value = (6 + 2 * sizeof(key) + 3) & ~size_t(3);
    Range(value, 52, length);
    Check(U32(bytes, value) == 0xFEEF04BD);
    Versions result;
    for (size_t i = 0; i < 4; ++i) {
        const size_t word = (i ^ 1) * 2;
        result.file[i] = U16(bytes, value + 8 + word);
        result.product[i] = U16(bytes, value + 16 + word);
    }
    bytes.resize(length); result.data = std::move(bytes);
    return result;
}
Versions PEVersion(const char* path) {
    // Data-only PE32/PE32+ resource traversal; never map executable pages or
    // invoke the DLL. Resource layout: Microsoft PE Format, .rsrc section.
    // https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#the-rsrc-section
    File file(path);
    const auto dos = file.Read(0, 64);
    Check(U16(dos, 0) == 0x5A4D);
    const uint32_t peOffset = U32(dos, 60);
    Check(peOffset >= 64);
    const auto coff = file.Read(peOffset, 24);
    Check(U32(coff, 0) == 0x00004550);
    const uint16_t sectionCount = U16(coff, 6), optionalSize = U16(coff, 20);
    Check(sectionCount && sectionCount <= 96 && optionalSize <= 4096);
    const auto optional = file.Read(uint64_t(peOffset) + 24, optionalSize);
    const uint16_t magic = U16(optional, 0);
    Check(magic == 0x10B || magic == 0x20B);
    const size_t directories = magic == 0x10B ? 96 : 112;
    Check(U32(optional, directories - 4) > 2);
    const uint32_t resourceRva = U32(optional, directories + 16), resourceSize = U32(optional, directories + 20);
    Check(resourceRva && resourceSize >= 16 && resourceSize <= 16 * 1024 * 1024);
    const auto sections = file.Read(uint64_t(peOffset) + 24 + optionalSize, size_t(sectionCount) * 40);
    auto readRva = [&](uint32_t rva, size_t count) {
        Check(count <= UINT32_MAX && uint64_t(rva) + count <= uint64_t(UINT32_MAX) + 1);
        bool found = false;
        uint64_t offset = 0;
        const uint32_t headers = U32(optional, 60);
        if (rva < headers && count <= headers - rva) { found = true; offset = rva; }
        for (size_t i = 0; i < sectionCount; ++i) {
            const size_t section = i * 40;
            const uint32_t address = U32(sections, section + 12), rawSize = U32(sections, section + 16);
            if (rva < address || uint64_t(rva) - address >= rawSize || count > uint64_t(rawSize) - (rva - address)) continue;
            Check(!found); // Ambiguous overlapping virtual ranges are invalid.
            found = true;
            offset = uint64_t(U32(sections, section + 20)) + (rva - address);
        }
        Check(found);
        return file.Read(offset, count);
    };
    const auto resources = readRva(resourceRva, resourceSize);
    auto entries = [&](size_t directory) {
        Range(directory, 16, resources.size());
        const size_t count = size_t(U16(resources, directory + 12)) + U16(resources, directory + 14);
        Check(count <= 4096);
        Range(directory + 16, count * 8, resources.size());
        return count;
    };
    auto child = [&](size_t directory, uint32_t id) {
        uint32_t result = 0; bool found = false;
        const size_t count = entries(directory);
        for (size_t i = 0; i < count; ++i) {
            const size_t entry = directory + 16 + i * 8;
            if (U32(resources, entry) != id) continue;
            Check(!found);
            found = true; result = U32(resources, entry + 4);
        }
        Check(found && (result & 0x80000000));
        return size_t(result & 0x7FFFFFFF);
    };
    const size_t names = child(0, 16); // RT_VERSION
    Check(names != 0);
    const size_t languages = child(names, 1); // VS_VERSION_INFO resource ID
    Check(languages != 0 && languages != names);
    const size_t count = entries(languages);
    Check(count && count <= 256);
    Versions result;
    for (size_t i = 0; i < count; ++i) {
        const uint32_t leaf = U32(resources, languages + 20 + i * 8);
        Check(!(leaf & 0x80000000));
        Range(leaf, 16, resources.size());
        const uint32_t size = U32(resources, leaf + 4);
        Check(size && size <= 65535);
        auto version = VersionBlock(readRva(U32(resources, leaf), size));
        // Numeric versions normally agree across language resources. If they
        // conflict, don't guess a Windows UI-language/MUI selection on Apple.
        if (!result.data.empty()) Check(result.file == version.file && result.product == version.product);
        else result = std::move(version);
    }
    return result;
}
bool ParseVersion(NSString* text, std::array<uint16_t, 4>& output) {
    if (![text isKindOfClass:NSString.class] || !text.length) return false;
    NSArray<NSString*>* parts = [text componentsSeparatedByString:@"."];
    if (parts.count > output.size()) return false;
    for (NSUInteger i = 0; i < parts.count; ++i) {
        NSString* part = parts[i];
        if (!part.length || part.length > 5) return false;
        uint32_t value = 0;
        for (NSUInteger j = 0; j < part.length; ++j) {
            const unichar digit = [part characterAtIndex:j];
            if (digit < '0' || digit > '9') return false;
            value = value * 10 + digit - '0';
        }
        if (value > UINT16_MAX) return false;
        output[i] = uint16_t(value);
    }
    return true;
}
Versions AppVersion() {
    Versions result;
    NSDictionary* info = NSBundle.mainBundle.infoDictionary;
    NSString* file = info[@"CFBundleVersion"], *product = info[@"CFBundleShortVersionString"];
    Check(ParseVersion(file, result.file) && ParseVersion(product, result.product));
    // m_data indicates that version metadata exists. Preserve its real source,
    // rather than fabricating a Windows resource for the native application.
    NSString* source = [NSString stringWithFormat:@"CFBundleVersion=%@\nCFBundleShortVersionString=%@\n", file, product];
    NSData* data = [source dataUsingEncoding:NSUTF8StringEncoding];
    const auto* bytes = static_cast<const uint8_t*>(data.bytes);
    result.data.assign(bytes, bytes + data.length);
    return result;
}
}

OsFileVersionInfo::OsFileVersionInfo(const char* filename) : m_fileVersion{}, m_productVersion{} {
    @autoreleasepool {
        try {
            auto result = filename ? PEVersion(filename) : AppVersion();
            m_data.SetCount(uint32_t(result.data.size()));
            std::memcpy(m_data.Ptr(), result.data.data(), result.data.size());
            std::copy(result.file.begin(), result.file.end(), m_fileVersion);
            std::copy(result.product.begin(), result.product.end(), m_productVersion);
        } catch (const InvalidVersion&) {
            m_data.Clear(); // Original API: unavailable/malformed metadata has no version.
        }
    }
}
