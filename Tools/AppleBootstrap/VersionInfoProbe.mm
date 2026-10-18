#include "Engine/Source/Os/OsVersionHash.h"
#import <Foundation/Foundation.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <sys/stat.h>
#include <vector>

namespace {
using Bytes = std::vector<uint8_t>;
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Put16(Bytes& bytes, size_t offset, uint16_t value) { bytes.at(offset) = uint8_t(value); bytes.at(offset + 1) = uint8_t(value >> 8); }
void Put32(Bytes& bytes, size_t offset, uint32_t value) { Put16(bytes, offset, uint16_t(value)); Put16(bytes, offset + 2, uint16_t(value >> 16)); }
void Save(const std::string& path, const Bytes& bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    Require(bool(file), "Cannot write version metadata fixture");
}
Bytes Image(bool wide) {
    Bytes bytes(1024);
    Put16(bytes, 0, 0x5A4D); Put32(bytes, 60, 128); Put32(bytes, 128, 0x4550);
    Put16(bytes, 132, wide ? 0x8664 : 0x14C); Put16(bytes, 134, 1);
    const size_t optional = 152, optionalSize = wide ? 240 : 224;
    Put16(bytes, 148, optionalSize); Put16(bytes, optional, wide ? 0x20B : 0x10B);
    Put32(bytes, optional + 60, 512);
    const size_t directories = optional + (wide ? 112 : 96);
    Put32(bytes, directories - 4, 16); Put32(bytes, directories + 16, 0x1000); Put32(bytes, directories + 20, 512);
    const size_t section = optional + optionalSize;
    Put32(bytes, section + 8, 512); Put32(bytes, section + 12, 0x1000);
    Put32(bytes, section + 16, 512); Put32(bytes, section + 20, 512);
    const size_t root = 512;
    Put16(bytes, root + 14, 1); Put32(bytes, root + 16, 16); Put32(bytes, root + 20, 0x80000018);
    Put16(bytes, root + 0x18 + 14, 1); Put32(bytes, root + 0x28, 1); Put32(bytes, root + 0x2C, 0x80000030);
    Put16(bytes, root + 0x30 + 14, 1); Put32(bytes, root + 0x40, 0x409); Put32(bytes, root + 0x44, 0x48);
    Put32(bytes, root + 0x48, 0x1080); Put32(bytes, root + 0x4C, 92);
    const size_t block = root + 0x80;
    Put16(bytes, block, 92); Put16(bytes, block + 2, 52);
    const char key[] = "VS_VERSION_INFO";
    for (size_t i = 0; i < sizeof(key); ++i) Put16(bytes, block + 6 + i * 2, key[i]);
    Put32(bytes, block + 40, 0xFEEF04BD); Put32(bytes, block + 44, 0x10000);
    Put32(bytes, block + 48, 0x00030003); Put32(bytes, block + 52, 0x00053034);
    Put32(bytes, block + 56, 0x00010002); Put32(bytes, block + 60, 0x0003FFFF);
    return bytes;
}
void CheckVersions(const OsFileVersionInfo& info, std::array<uint16_t, 4> file, std::array<uint16_t, 4> product) {
    Require(info.m_data.m_count && !std::memcmp(info.m_fileVersion, file.data(), 8) &&
        !std::memcmp(info.m_productVersion, product.data(), 8), "File/product version metadata changed");
}
}

void CheckVersionInfo(const std::string& scratch) {
    std::string pattern = (std::filesystem::path(scratch) / "wow-version-XXXXXX").string();
    std::vector<char> directory(pattern.begin(), pattern.end()); directory.push_back(0);
    Require(mkdtemp(directory.data()), "Cannot create version metadata scratch directory");
    struct Scope { std::string path; ~Scope() { std::error_code error; std::filesystem::remove_all(path, error); } } scope{directory.data()};
    const std::string filename = scope.path + "/module.dll";
    auto reject = [&](const Bytes& bytes) {
        Save(filename, bytes); OsFileVersionInfo info(filename.c_str());
        const std::array<uint16_t, 4> zero{};
        Require(!info.m_data.Count() && !std::memcmp(info.m_fileVersion, zero.data(), 8) && !std::memcmp(info.m_productVersion, zero.data(), 8),
            "Malformed version metadata published partial results");
    };
    for (bool wide : {false, true}) {
        const auto valid = Image(wide);
        Save(filename, valid);
        OsFileVersionInfo info(filename.c_str());
        CheckVersions(info, {3,3,5,12340}, {1,2,3,65535});
        Require(info.m_data.Count() == 92 && !std::memcmp(info.m_data.Ptr(), valid.data() + 640, 92), "Version resource bytes were rewritten");
        for (size_t length : {0u, 1u, 63u, 127u, 151u, 375u, 415u, 511u, 527u, 551u, 575u, 591u, 639u, 731u, 1023u})
            reject(Bytes(valid.begin(), valid.begin() + length));
        const size_t directories = 152 + (wide ? 112 : 96), section = 152 + (wide ? 240 : 224);
        for (const auto& mutation : std::vector<std::pair<size_t, uint32_t>>{
                {60,UINT32_MAX}, {directories - 4,2}, {directories + 16,UINT32_MAX}, {directories + 20,16 * 1024 * 1024 + 1},
                {section + 20,UINT32_MAX}, {512 + 20,0x80000000}, {512 + 0x2C,0x80000018}, {512 + 0x44,0x80000030},
                {512 + 0x44,500}, {512 + 0x48,0xFFFFF000}, {512 + 0x4C,65536}, {640 + 40,0xDEADBEEF}}) {
            auto bytes = valid; Put32(bytes, mutation.first, mutation.second); reject(bytes);
        }
        for (const auto& mutation : std::vector<std::pair<size_t, uint16_t>>{{134,97},{148,1},{152,0x999},{640,40},{642,0},{646,'X'}}) {
            auto bytes = valid; Put16(bytes, mutation.first, mutation.second); reject(bytes);
        }
        auto overlapping = valid;
        Put16(overlapping,134,2); std::memcpy(overlapping.data() + section + 40, overlapping.data() + section, 40); reject(overlapping);
        auto multilingual = valid;
        Put16(multilingual,512 + 0x3E,2);
        Put32(multilingual,512 + 0x44,0x68); Put32(multilingual,512 + 0x48,0x804); Put32(multilingual,512 + 0x4C,0x78);
        Put32(multilingual,512 + 0x68,0x1100); Put32(multilingual,512 + 0x6C,92);
        Put32(multilingual,512 + 0x78,0x1180); Put32(multilingual,512 + 0x7C,92);
        std::memcpy(multilingual.data() + 768,valid.data() + 640,92);
        std::memcpy(multilingual.data() + 896,valid.data() + 640,92);
        Save(filename,multilingual); CheckVersions(OsFileVersionInfo(filename.c_str()),{3,3,5,12340},{1,2,3,65535});
        Put32(multilingual,896 + 48,0x00040003); reject(multilingual);
        Save(filename,valid); CheckVersions(OsFileVersionInfo(filename.c_str()),{3,3,5,12340},{1,2,3,65535});
    }
    const std::string link = scope.path + "/alias.dll", fifo = scope.path + "/pipe.dll";
    std::filesystem::create_symlink(filename, link);
    CheckVersions(OsFileVersionInfo(link.c_str()),{3,3,5,12340},{1,2,3,65535});
    Require(mkfifo(fifo.c_str(), 0600) == 0, "Cannot create non-regular version fixture");
    Require(!OsFileVersionInfo(fifo.c_str()).m_data.Count() && !OsFileVersionInfo(scope.path.c_str()).m_data.Count() &&
        !OsFileVersionInfo("").m_data.Count() && !OsFileVersionInfo((scope.path + "/missing").c_str()).m_data.Count(),
        "Non-file version metadata was accepted");
    OsFileVersionInfo current(nullptr);
    if ([NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleVersion"])
        CheckVersions(current,{1,0,0,0},{0,1,0,0}); // Actual native bootstrap bundle metadata.
    else Require(!current.m_data.Count(), "Unversioned command-line executable invented version metadata");
    std::puts("Native version info: PE32/PE32+ resource bytes and numeric versions, truncation/range/budget/cycles, language agreement, retry, file types and native bundle metadata passed");
}

void CheckInstalledVersions(const std::string& dataRoot) {
    const auto root = std::filesystem::path(dataRoot).parent_path();
    struct Item { const char* name; std::array<uint16_t, 4> file, product; };
    const Item items[] = {{"Wow.exe",{3,3,5,12340},{3,3,0,0}}, {"Scan.dll",{2,2,84,77},{2,2,84,77}}, {"unicows.dll",{1,0,4018,0},{1,0,4018,0}}};
    for (const auto& item : items) {
        const auto path = root / item.name;
        if (!std::filesystem::exists(path)) continue; // A Data-only iOS installation need not contain Windows binaries.
        OsFileVersionInfo info(path.c_str()); CheckVersions(info,item.file,item.product);
        std::fprintf(stderr,"Original %s version: %u.%u.%u.%u; %u resource bytes read without loading code\n", item.name,
            info.m_fileVersion[0],info.m_fileVersion[1],info.m_fileVersion[2],info.m_fileVersion[3],info.m_data.Count());
    }
    const auto divx = root / "DivxDecoder.dll";
    if (std::filesystem::exists(divx)) Require(!OsFileVersionInfo(divx.c_str()).m_data.Count(), "Unversioned Divx DLL invented version metadata");
}
