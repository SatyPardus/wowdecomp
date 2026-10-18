#include "Engine/Source/Storm/Source/IOUnit/IOFileUnit.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/FileDownload.h"
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

namespace {
void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
struct Release { void operator()(Storm::IOUnit* unit) const { if (unit) unit->Release(); } };
using File = std::unique_ptr<Storm::IOUnit, Release>;
File Open(const std::string& path, uint32_t creation = 3, uint32_t access = 0xC0000000, uint32_t flags = 0) {
    return File(Storm::IOFileUnit::Create({path.c_str(), access, 0, creation, flags}, nullptr));
}
struct Ref final : Storm::IORefcount {
    unsigned& destroyed;
    Ref(unsigned& counter, Ref* next) : destroyed(counter) { m_count = 2; m_refcount = next; }
    ~Ref() override { ++destroyed; }
};
}

void CheckFileDownloads(const std::string& scratchDirectory) {
    std::string pattern = scratchDirectory + "/wow-download-XXXXXX";
    std::vector<char> temporary(pattern.begin(), pattern.end()); temporary.push_back(0);
    Require(mkdtemp(temporary.data()), "Cannot create native download scratch directory");
    struct Cleanup {
        const char* path;
        ~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); }
    } cleanup{temporary.data()};
    const std::string path = std::string(temporary.data()) + "/native-文件";
    const std::array<uint8_t, 7> payload{{0x13, 0xFE, 0x83, 0x15, 0, 0x92, 0xD7}};
    for (uint32_t flags : {0u, 0x20000000u, 0x80000080u}) {
        File file = Open(path, 2, 0xC0000000, flags);
        Require(file && file->Write(payload.data(), 3, payload.size()) && file->Flush(), "Native Storm create/write/flush failed");
        int64_t size = 0;
        Require(file->GetFileSize(&size) && size == 10, "Native Storm write exposed sector padding");
        std::array<uint8_t, 9> bytes{}; uint32_t read = 0;
        Require(file->Read(bytes.data(), 3, bytes.size(), &read) && read == payload.size() &&
            !std::memcmp(bytes.data(), payload.data(), payload.size()), "Native Storm short read changed bytes/count");
        Require(!file->Read(bytes.data(), 3, bytes.size(), nullptr) && !file->Read(bytes.data(), 10, 1, &read) && read == 0,
            "Native Storm exact-read/EOF convention changed");
        Require(file->Read(nullptr, 0, 0, nullptr) && !file->Read(nullptr, 0, 0, &read) && read == 0,
            "Native Storm zero-byte convention changed");
        Require(!file->Read(bytes.data(), -1, 1, &read) && read == 0 &&
            !file->Write(payload.data(), INT64_MAX, 2) && !file->SetFileSize(-1), "Native Storm accepted invalid file ranges");
        const int64_t large = (INT64_C(1) << 32) + 17;
        Require(file->Write(payload.data(), large, payload.size()) && file->GetFileSize(&size) && size == large + 7 &&
            file->Read(bytes.data(), large, 7, nullptr) && !std::memcmp(bytes.data(), payload.data(), 7),
            "Native Storm sparse range lost high offset bits");
        Require(file->SetFileSize(10) && file->GetFileSize(&size) && size == 10 && !file->Win32Handle() && errno == ENOTSUP,
            "Native Storm resize or unsupported handle convention changed");
        Require(!Open(path, 1) && errno == EEXIST, "Native Storm create-new replaced an existing file");
    }
    {
        File file = Open(path, 4);
        Require(file && file->Write(payload.data(), 0, 1), "Native Storm open-always failed");
        int64_t size;
        Require(file->GetFileSize(&size) && size == 10, "Native Storm open-always truncated or appended");
        Storm::IOFileParam parameters{path.c_str(), 0xC0000000, 0, 3, 0};
        Require(!Storm::IOFileUnit::Create(parameters, file.get()) && errno == EINVAL,
            "Native Storm leaf accepted a chained lower unit");
        parameters.m_flags = 0x40000000;
        Require(!Storm::IOFileUnit::Create(parameters, nullptr) && errno == ENOTSUP,
            "Native Storm falsely accepted overlapped IO");
        Require(!Open(path, 2, 0x80000000) && errno == EACCES && file->GetFileSize(&size) && size == 10,
            "Native Storm read-only truncate request modified a file");
    }
    {
        File readOnly = Open(path, 3, 0x80000000);
        Require(readOnly && !readOnly->Write(payload.data(), 0, 1), "Native Storm read-only file allowed writes");
    }
    {
        File file = Open(path, 5); int64_t size;
        Require(file && file->GetFileSize(&size) && size == 0, "Native Storm truncate-existing failed");
        unsigned destroyed = 0;
        file->m_count = 2; file->m_refcount = new Ref(destroyed, new Ref(destroyed, nullptr));
        file->Release();
        Require(!destroyed && file->GetFileSize(&size), "Native Storm refcount chain released a live file");
        file.reset();
        Require(destroyed == 2, "Native Storm refcount chain leaked or released twice");
    }
    Require(!Open(path + ".missing", 5) && errno == ENOENT, "Native Storm truncate-existing created a missing file");
    Require(Open(path + ".new", 1) != nullptr && Open(path + ".always", 4) != nullptr,
        "Native Storm creation dispositions failed");

    std::vector<uint8_t> content(1048913);
    for (size_t i = 0; i != content.size(); ++i) content[i] = uint8_t(i * 37 + (i >> 9));
    // Independent hashlib reference for this deterministic payload.
    const uint8_t md5[16] = {0x58,0xF2,0xA6,0x23,0x13,0xDB,0x31,0x4C,0x52,0x64,0x7D,0x4E,0x26,0x44,0x76,0xC4};
    const std::string target = std::string(temporary.data()) + "/patch.bin";
    uint64_t offset = UINT64_MAX;
    {
        GruntFileDownload download;
        Require(download.Initialize(target.c_str(), md5, content.size(), offset) && offset == 0, "Original download did not initialize its native partial file");
        download.Proceed(content.data(), 600000);
        Require(download.m_header.m_received == 600000 && download.GetProgress() > .5f && download.GetProgress() < .6f,
            "Original download byte/progress accounting changed");
        download.StopDownload();
        Require(download.GetStatus() == 3 && download.GetReason(nullptr) == 3, "Original download stop state changed");
    }
    {
        GruntFileDownload resumed;
        Require(resumed.Initialize(target.c_str(), md5, content.size(), offset) && offset == 600000,
            "Original download did not restore its saved offset/header");
        resumed.Proceed(content.data() + offset, uint32_t(content.size() - offset));
        Require(resumed.GetStatus() == 2 && resumed.GetProgress() == 1.0f, "Original download MD5 completion failed");
        resumed.CompleteDownload();
        Require(!std::filesystem::exists(target + ".partial") && std::filesystem::file_size(target) == content.size(),
            "Original completed download did not trim its header and rename");
        std::ifstream input(target, std::ios::binary); std::vector<uint8_t> saved(content.size());
        input.read(reinterpret_cast<char*>(saved.data()), saved.size());
        Require(input && saved == content, "Original downloaded file bytes changed");
    }
    {
        GruntFileDownload complete;
        Require(!complete.Initialize(target.c_str(), md5, content.size(), offset) && complete.GetStatus() == 2,
            "Original download failed to recognize an already completed file");
    }
    {
        GruntFileDownload invalid;
        const std::string corrupt = target + ".corrupt";
        Require(invalid.Initialize(corrupt.c_str(), md5, content.size(), offset), "Cannot start digest failure check");
        content[500] ^= 1;
        invalid.Proceed(content.data(), content.size());
        Require(invalid.GetStatus() == 3, "Original download accepted an incorrect MD5");
        invalid.Destroy(true);
        Require(!std::filesystem::exists(corrupt + ".partial"), "Original download cancellation did not clean its partial file");
    }
    std::fputs("Native Storm IO: creation/access/uncached/write-through, exact/short/EOF reads, >4 GiB offsets and refcount cleanup passed; original Grunt download stop/resume, MD5 accept/reject, header trim/rename and cancellation passed\n", stderr);
}
