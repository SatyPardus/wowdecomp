#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <TargetConditionals.h>
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Os/Path.h"
#include "Engine/Source/Os/Time.h"
#include "Tools/AppleBootstrap/Probe.h"
#include "Tools/AppleBootstrap/AudioProbe.h"
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Os/OsLibrary.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "Engine/Source/BLPFile/BlpImage.h"
#include "Engine/Source/BLPFile/Types.h"
#include "Engine/Source/BLPFile/blp.h"
#include "Engine/Source/Gx/ShaderBinary.h"
#include "Engine/Source/Gx/Metal/UIShader.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Base/CRTRandom.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOUnitContainer.h"
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/Posix/IOPosixFileUnit.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/MPQEncrypt.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFile.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFileArchives.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/IOMpqUnit.h"
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/Thread.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/Time/Time.h"
#include "Engine/Source/Storm/H/SCritSect.h"
#include "Engine/Source/Storm/Source/SThread.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lualib.h"
#include "Engine/Source/lua-5.1/src/lmemPool.h"

void CheckM2Resources(const ClientData& client, std::ostream& report);
void CheckNativeProcesses(const std::string& scratchDirectory);
void CheckBattlenetCore();
void CheckFileDownloads(const std::string& scratchDirectory);
void CheckTerrainResources(const ClientData& client, std::ostream& report);
void CheckWorldData(const ClientData& client, std::ostream& report);
void CheckGameNetwork();
void CheckURLDownloads();
void CheckKnowledgeBaseDownloads();
void CheckCommandLine(const std::string& scratch);
void CheckVersionInfo(const std::string& scratch);
void CheckInstalledVersions(const std::string& dataRoot);
void CheckMovieResources(const std::string& dataRoot, std::ostream& report);
void CheckGameLogin();
void CheckGameSession();
void CheckGameRealm();
void CheckRealmMetadata(const ClientData& client, std::ostream& report);
void CheckCharacterTextureCache(const ClientData* client);
void CheckCharacterNames();

namespace {
struct ReleaseUnit { void operator()(IOUnit* unit) const { if (unit) { unit->Release(); } } };
using Unit = std::unique_ptr<IOUnit, ReleaseUnit>;

void Require(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}

void CheckCRTRandom() {
    // Run in a fresh thread to check the original implicit seed without
    // replacing the host's random stream. Nested threads must not reseed it.
    bool passed = false;
    std::thread worker([&] {
        constexpr int expected[] = {41, 18467, 6334, 26500, 19169, 15724, 11478, 29358};
        bool valid = true;
        for (int value : expected) valid &= ClientRand() == value;
        ClientSrand(1);
        valid &= ClientRand() == 41;
        bool childValid = false;
        std::thread child([&] {
            childValid = ClientRand() == 41;
            ClientSrand(0);
            childValid &= ClientRand() == 38;
        });
        child.join();
        valid &= childValid && ClientRand() == 18467;
        ClientSrand(0xFFFFFFFFu);
        valid &= ClientRand() == 35;
        for (unsigned i = 0; i < 65536; ++i) {
            const int value = ClientRand();
            valid &= value >= 0 && value <= 32767;
        }
        passed = valid;
    });
    worker.join();
    Require(passed, "Client CRT random range, sequence, seed or thread isolation changed");
}

std::atomic<int> tlsDestroyed{0};
struct ThreadLocalProbe {
    uint32_t value = 0;
    ~ThreadLocalProbe() { ++tlsDestroyed; }
};
Blizzard::Thread::LocalObject<ThreadLocalProbe> threadLocalProbe{};
uint32_t CheckThreadLocal(void* parameter) {
    auto& local = static_cast<ThreadLocalProbe&>(threadLocalProbe);
    if (local.value) { return 0; }
    local.value = *static_cast<uint32_t*>(parameter);
    Blizzard::Thread::Sleep(2);
    return static_cast<ThreadLocalProbe&>(threadLocalProbe).value;
}

void CheckNativeThreads() {
    const int before = tlsDestroyed.load();
    std::array<Blizzard::Thread::ThreadRecord*, 8> threads{};
    std::array<uint32_t, 8> values{1, 2, 3, 4, 5, 6, 7, 8};
    for (size_t i = 0; i < threads.size(); ++i) {
        Require(Blizzard::Thread::Create(CheckThreadLocal, &values[i], &threads[i], "WoW archive worker", 0, 65536), "native thread creation failed");
    }
    bool success = true;
    for (size_t i = 0; i < threads.size(); ++i) {
        success &= Blizzard::Thread::WaitForever(threads[i]);
        success &= !Blizzard::Thread::IsRunning(threads[i]) && threads[i]->m_result == values[i];
        Blizzard::Thread::Close(threads[i]);
    }
    Require(success && tlsDestroyed == before + 8, "native thread result/TLS cleanup mismatch");
    // Cocoa/dispatch and other externally-created threads also use the engine TLS.
    std::thread external([&] { CheckThreadLocal(&values[0]); });
    external.join();
    Require(tlsDestroyed == before + 9, "external thread TLS cleanup failed");
    struct DetachedState {
        Blizzard::Lock::EventObject start;
        Blizzard::Lock::EventObject done;
    } detached;
    Blizzard::Thread::ThreadRecord* released;
    Require(Blizzard::Thread::Create([](void* value) -> uint32_t {
        auto& state = *static_cast<DetachedState*>(value);
        Blizzard::Lock::EventWaitForever(state.start.m_event);
        Blizzard::Lock::EventSet(state.done.m_event);
        return 42;
    }, &detached, &released, "detached lifetime", 0, 0), "detached thread creation failed");
    Require(Blizzard::Thread::Close(released), "running thread close failed");
    Blizzard::Lock::EventSet(detached.start.m_event);
    Require(Blizzard::Lock::EventWait(1000, detached.done.m_event), "closed thread did not finish");
    Blizzard::Thread::ThreadRecord* suspended = nullptr;
    Require(!Blizzard::Thread::Create(CheckThreadLocal, &values[0], &suspended, "suspended", 1, 0) && !suspended,
        "unsupported suspended thread did not fail explicitly");
}

void CheckStormSynchronization() {
    SEvent manual(1, 0), automatic(0, 0);
    SSemaphore semaphore(1, 2);
    SSyncObject* together[] = { &semaphore, &automatic };
    Require(SSyncObject::WaitMultiplePtr(2, together, 1, 1) == 258 && semaphore.Wait(0) == 0,
        "Storm wait-all consumed a partial set on timeout");
    automatic.Set(); automatic.Set();
    Require(automatic.Wait(0) == 0 && automatic.Wait(0) == 258, "Storm auto-reset signal/coalescing mismatch");
    automatic.Set(); semaphore.Signal(1);
    Require(SSyncObject::WaitMultiplePtr(2, together, 1, 1000) == 0 &&
        automatic.Wait(0) == 258 && semaphore.Wait(0) == 258, "Storm wait-all acquisition mismatch");
    SSyncObject* any[] = { nullptr, &automatic, &manual };
    manual.Set();
    Require(SSyncObject::WaitMultiplePtr(3, any, 0, 0) == 1 && manual.Wait(0) == 0,
        "Storm wait-any index/manual-reset mismatch");
    manual.Reset();
    std::atomic<int> awakened{0};
    std::thread a([&] { if (manual.Wait(1000) == 0) { ++awakened; } });
    std::thread b([&] { if (manual.Wait(1000) == 0) { ++awakened; } });
    manual.Set(); a.join(); b.join();
    Require(awakened == 2 && manual.Wait(0) == 0, "Storm manual event did not release all waiters");
    Require(semaphore.Signal(2) && !semaphore.Signal(1) && semaphore.Wait(0) == 0 &&
        semaphore.Wait(0) == 0 && semaphore.Wait(0) == 258, "Storm semaphore count/overflow mismatch");

    SMutex mutex;
    Require(mutex.Wait(0) == 0 && mutex.Wait(0) == 0 && mutex.Release(), "Storm recursive mutex failed");
    uint32_t blocked = 0;
    std::thread contender([&] { blocked = mutex.Wait(1); }); contender.join();
    Require(blocked == 258 && mutex.Release(), "Storm mutex recursion released ownership early");
    std::thread abandoned([&] { mutex.Wait(1000); }); abandoned.join();
    Require(mutex.Wait(0) == 0x80 && mutex.Release(), "Storm abandoned mutex did not transfer ownership");
    mutex.Create(0, "unsupported cross-process mutex");
    Require(!mutex.Valid(), "Storm named mutex silently became process-local");

    CSRWLock rw;
    rw.Enter(0);
    std::atomic<bool> secondReader{false};
    std::thread reader([&] { rw.Enter(0); secondReader = true; rw.Leave(0); });
    reader.join(); rw.Leave(0);
    Require(secondReader, "Storm shared reader lock failed");
    int protectedCount = 0;
    auto increment = [&] { for (int i = 0; i < 1000; ++i) { rw.Enter(1); ++protectedCount; rw.Leave(1); } };
    std::thread writerA(increment), writerB(increment); writerA.join(); writerB.join();
    Require(protectedCount == 2000, "Storm writer exclusion failed");
    CInitCritSect once;
    Require(once.Enter() == 1 && once.Enter() == 0, "Storm critical-section first-entry result mismatch");
    once.Leave(); once.Leave();

    const int before = tlsDestroyed.load();
    uint32_t value = 123;
    SThread first, second;
    Require(SThread::Create(CheckThreadLocal, &value, first, const_cast<char*>("Gxu file worker"), 65536) &&
        SThread::Create(CheckThreadLocal, &value, second, nullptr, 0), "Storm native thread creation failed");
    SSyncObject* threads[] = { &first, &second };
    Require(SSyncObject::WaitMultiplePtr(2, threads, 1, 1000) == 0 && tlsDestroyed == before + 2 &&
        first.Wait(0) == 0, "Storm thread wait returned before engine TLS destruction");
    struct Detached { SEvent start{1, 0}, done{1, 0}; } detached;
    SThread released;
    Require(SThread::Create([](void* data) -> uint32_t {
        auto& state = *static_cast<Detached*>(data);
        state.start.Wait(1000); state.done.Set(); return 0;
    }, &detached, released, nullptr, 0), "Storm detached thread creation failed");
    released.Close(); detached.start.Set();
    Require(!released.Valid() && detached.done.Wait(1000) == 0, "Storm closing a running thread cancelled its work");
}

uint32_t Read32(const uint8_t* bytes) {
    return uint32_t(bytes[0]) | (uint32_t(bytes[1]) << 8) | (uint32_t(bytes[2]) << 16) | (uint32_t(bytes[3]) << 24);
}

void CheckBlpLoading() {
    BLPHeader header{};
    header.m_magic = 0x32504C42;
    header.m_formatVersion = 1;
    header.m_colorEncoding = 1;
    header.m_alphaSize = 4;
    header.m_preferredFormat = PIXEL_ARGB8888;
    header.m_width = 2; header.m_height = 1;
    header.m_mipOffsets[0] = sizeof(header); header.m_mipSizes[0] = 3;
    header.m_extended.m_palette[1] = {10, 20, 30, 77};
    header.m_extended.m_palette[2] = {40, 50, 60, 88};
    std::vector<uint8_t> data(sizeof(header) + 3);
    std::memcpy(data.data(), &header, sizeof(header));
    data[sizeof(header)] = 1; data[sizeof(header) + 1] = 2; data[sizeof(header) + 2] = 0xF0;
    const auto image = BLP::Image::Load(data.data(), data.size(), 8);
    Require(image.mips.size() == 1 && image.mips[0].bytes == std::vector<uint8_t>({10, 20, 30, 0, 40, 50, 60, 255}),
        "BLP palette/packed alpha mismatch");
    CBLPFile original;
    Require(original.Source(data.data(), data.size()), "Original BLP rejected bounded palette input");
    uint8_t* pixels = nullptr; uint32_t stride = 0;
    Require(original.Lock(PIXEL_ARGB8888, 0, pixels, stride) && stride == 8 &&
        !std::memcmp(pixels, image.mips[0].bytes.data(), 8), "Original BLP palette differs from bounded decoder");
    original.Unlock(0);
    Require(!original.Source(data.data(), data.size() - 1) && !original.Source(data.data(), 20), "Original BLP accepted truncated input");
    auto rejects = [&](size_t size, size_t budget) {
        try { BLP::Image::Load(data.data(), size, budget); } catch (const std::runtime_error&) { return true; }
        return false;
    };
    Require(rejects(data.size() - 1, 8) && rejects(data.size(), 7), "BLP truncated mip/memory budget accepted");
    data[11] = 1;
    Require(rejects(data.size(), 16) && !original.Source(data.data(), data.size()), "BLP missing authored mip accepted");
    data[11] = 0;
    data[20] = data[21] = data[22] = data[23] = 255;
    Require(rejects(data.size(), 8) && !original.Source(data.data(), data.size()), "BLP out-of-range offset accepted");

    // Rectangular original UI BC2 assets have single-block placeholders once
    // the short side is below the Gx compressed mip limit. Complete small mips
    // must still be preserved, and a truncated reachable level must fail.
    header = {}; header.m_magic = 0x32504C42; header.m_formatVersion = 1;
    header.m_colorEncoding = 2; header.m_alphaSize = 8; header.m_preferredFormat = PIXEL_DXT3;
    header.m_hasMips = 1; header.m_width = 16; header.m_height = 4;
    size_t end = offsetof(BLPHeader, m_extended);
    for (unsigned i = 0; i < 5; ++i) {
        header.m_mipOffsets[i] = end;
        header.m_mipSizes[i] = i == 0 ? 64 : i == 1 ? 32 : 16;
        end += header.m_mipSizes[i];
    }
    data.assign(end, 0); std::memcpy(data.data(), &header, offsetof(BLPHeader, m_extended));
    Require(original.Source(data.data(), data.size()) && original.m_numLevels == 5, "Complete rectangular BC tail was removed");
    header.m_mipSizes[1] = 16; std::memcpy(data.data(), &header, offsetof(BLPHeader, m_extended));
    Require(original.Source(data.data(), data.size()) && original.m_numLevels == 1, "Original UI placeholder tail was rejected or exposed");
    header.m_mipSizes[0] = 63; std::memcpy(data.data(), &header, offsetof(BLPHeader, m_extended));
    Require(!original.Source(data.data(), data.size()), "Truncated reachable rectangular BC mip accepted");
    header.m_mipSizes[0] = 64; header.m_mipOffsets[1] = uint32_t(data.size());
    std::memcpy(data.data(), &header, offsetof(BLPHeader, m_extended));
    Require(!original.Source(data.data(), data.size()), "Out-of-file rectangular BC placeholder accepted");
}

void InspectArchive(const std::filesystem::path& path, std::ostream& out) {
    Blizzard::File::SetLastErrorStack(nullptr);
    Unit file(IOPosixFileUnit::OpenRead(path.c_str()));
    Require(bool(file), "cannot open archive read-only");
    int64_t size;
    Require(file->GetFileSize(&size), "cannot get archive size");
    std::array<uint8_t, 44> header{};
    Require(file->Read(header.data(), 0, 32, nullptr), "truncated MPQ header");
    Require(Read32(header.data()) == 0x1A51504D, "expected MPQ header at offset zero");
    const uint32_t version = header[12] | (uint32_t(header[13]) << 8);
    Require(version <= 1, "probe supports MPQ format versions 0 and 1 only");
    Require(Read32(header.data() + 4) >= (version ? 44u : 32u), "invalid MPQ header size");
    if (version) { Require(file->Read(header.data() + 32, 32, 12, nullptr), "truncated extended header"); }
    const uint32_t hashCount = Read32(header.data() + 24);
    const uint32_t blockCount = Read32(header.data() + 28);
    const uint64_t hashOffset = Read32(header.data() + 16) |
        (uint64_t(header[40] | (uint32_t(header[41]) << 8)) << 32);
    Require(hashCount && (hashCount & (hashCount - 1)) == 0, "invalid hash table count");
    Require(hashCount <= 1048576, "hash table exceeds probe's 16 MiB inspection limit");
    const uint64_t bytes = uint64_t(hashCount) * 16;
    Require(hashOffset <= uint64_t(size) && bytes <= uint64_t(size) - hashOffset, "hash table outside archive");
    std::vector<uint32_t> hashes(hashCount * 4);
    Require(file->Read(hashes.data(), static_cast<int64_t>(hashOffset), static_cast<uint32_t>(bytes), nullptr), "cannot read hash table");
    System_MPQEncrypt::Decrypt(hashes.data(), static_cast<uint32_t>(bytes), System_MPQEncrypt::Hash("(hash table)", 3));
    uint32_t live = 0;
    for (uint32_t i = 0; i < hashCount; ++i) {
        const uint32_t block = hashes[i * 4 + 3];
        if (block >= 0xFFFFFFFE) { continue; }
        Require(block < blockCount, "decrypted hash references invalid block");
        ++live;
    }
    out << path.filename().string() << ": " << size << " bytes, MPQ v" << version
        << ", " << live << " hash entries / " << blockCount << " blocks\n";
    struct CloseArchive { void operator()(System_SFile::ARCHIVEREC* archive) const { Blizzard::Mopaq::SFileCloseArchive(archive); } };
    System_SFile::ARCHIVEREC* opened = nullptr;
    Require(Blizzard::Mopaq::SFileOpenArchive(path.c_str(), 0, 0x800, &opened), "SFile archive open failed");
    std::unique_ptr<System_SFile::ARCHIVEREC, CloseArchive> archive(opened);
    void* contents = nullptr;
    uint32_t listBytes = 0;
    if (!Blizzard::Mopaq::SFileLoadFile(archive.get(), "(listfile)", &contents, &listBytes, 1, 0)) {
        if (Blizzard::Mopaq::SFileGetLastError() == 2) {
            out << "  SFile: no (listfile); archive lookup remains available for known asset names\n";
            return;
        }
        throw std::runtime_error("SFile listfile decompression failed: " + std::to_string(Blizzard::Mopaq::SFileGetLastError()) + " " +
            Blizzard::Debug::ErrorStackAsText(Blizzard::File::GetLastErrorStack()));
    }
    std::unique_ptr<void, decltype(&Blizzard::Mopaq::SFileUnloadFile)> listfile(contents, Blizzard::Mopaq::SFileUnloadFile);
    out << "  SFile: decompressed (listfile), " << listBytes << " bytes\n";
    std::istringstream names(std::string(static_cast<char*>(contents), listBytes));
    std::set<std::string> wanted{".dbc", ".blp", ".m2", ".wmo", ".adt", ".wav", ".ogg", ".lua", ".xml"};
    std::string name;
    while (std::getline(names, name) && !wanted.empty()) {
        if (!name.empty() && name.back() == '\r') { name.pop_back(); }
        std::string extension = std::filesystem::path(name).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });
        if (!wanted.count(extension)) { continue; }
        IOMpqUnit* openedFile = nullptr;
        if (!Blizzard::Mopaq::SFileOpenFileEx(archive.get(), name.c_str(), 0, &openedFile)) {
            if (Blizzard::Mopaq::SFileGetLastError() == 2) { continue; } // stale listfile entry
            throw std::runtime_error("SFile cannot open listed resource: " + name);
        }
        Unit asset(openedFile);
        const auto assetSize = Blizzard::Mopaq::SFileGetFileSize(openedFile, nullptr);
        if (assetSize > 64u * 1024 * 1024) { continue; } // bounded probe, not a loader limit
        std::vector<uint8_t> data(assetSize);
        uint32_t read = 0;
        if (!Blizzard::Mopaq::SFileReadFile(openedFile, data.data(), assetSize, &read) || read != assetSize) {
            throw std::runtime_error("SFile resource read/decompression failed: " + name + " " +
                Blizzard::Debug::ErrorStackAsText(Blizzard::File::GetLastErrorStack()));
        }
        Blizzard::Util::MD5 md5;
        md5.Process(data.data(), assetSize);
        uint8_t digest[16];
        md5.Finish(digest);
        const auto* expected = openedFile->GetMd5();
        const bool hasDigest = expected && *expected != System_SFile::s_emptyMd5;
        if (hasDigest && std::memcmp(digest, expected->m_data, sizeof(digest))) {
            throw std::runtime_error("Resource differs from archive MD5: " + name);
        }
        const char hex[] = "0123456789abcdef";
        std::string text;
        for (uint8_t byte : digest) { text += hex[byte >> 4]; text += hex[byte & 15]; }
        out << "  " << name << ": " << assetSize << " bytes, md5 " << text
            << (hasDigest ? " (archive MD5 matched)" : " (no archive MD5)") << '\n';
        if (extension == ".blp") {
            const auto image = BLP::Image::Load(data.data(), data.size(), 256 * 1024 * 1024);
            CBLPFile original;
            Require(original.Source(data.data(), data.size()), "Original BLP rejected a real bounded asset");
            const PIXEL_FORMAT formats[] = {PIXEL_ARGB8888, PIXEL_DXT1, PIXEL_DXT3, PIXEL_DXT5};
            for (size_t level = 0; level < image.mips.size(); ++level) {
                image.DecodeBGRA(level);
                MipBits* mips = nullptr;
                Require(original.LockChain(formats[unsigned(image.format)], mips, uint32_t(level)), "Original BLP mip loading failed");
                Require(!std::memcmp(mips[0].m_mip[0], image.mips[level].bytes.data(), image.mips[level].bytes.size()),
                    "Original BLP authored mip differs from bounded decoder");
                SMemFree(mips, __FILE__, __LINE__, 0);
            }
            out << "    BLP: " << image.mips[0].width << 'x' << image.mips[0].height << ' ' << BLP::FormatName(image.format)
                << ", " << image.mips.size() << " authored mips decoded\n";
        }
        wanted.erase(extension);
    }
}

struct ScratchFile {
    std::string path;
    std::string directory;
    explicit ScratchFile(const std::string& directory) {
        std::string pattern = (std::filesystem::path(directory) / "wow-core-XXXXXX").string();
        std::vector<char> name(pattern.begin(), pattern.end());
        name.push_back(0);
        Require(mkdtemp(name.data()) != nullptr, "cannot create scratch directory");
        this->directory = name.data();
        path = this->directory + "/offset-check";
    }
    ~ScratchFile() { unlink(path.c_str()); rmdir(directory.c_str()); }
};

struct CloseSFile { void operator()(SFileImpl* file) const { SFileCloseFile(file); } };
void CheckClientFile(const char* name, const std::vector<uint8_t>& expected) {
    Require(expected.size() > 16, "SFile2 probe requires a nonempty resource");
    Require(SFile::FileExists(name), "SFile2 file lookup failed");
    for (uint32_t flags : {0u, 0x20000u}) {
        SFileImpl* opened = nullptr;
        Require(SFile::OpenEx(nullptr, name, flags, &opened), "SFile2 open/preload failed");
        std::unique_ptr<SFileImpl, CloseSFile> file(opened);
        Require(SFileGetFileSize(file.get(), nullptr) == expected.size(), "SFile2 size mismatch");
        std::vector<uint8_t> bytes(expected.size());
        uint32_t read = 0;
        Require(SFileReadFile(file.get(), bytes.data(), uint32_t(bytes.size()), &read, nullptr, 0) &&
            read == bytes.size() && bytes == expected, "SFile2 sequential/preloaded data mismatch");
        Require(SFile::SetFilePointer(file.get(), 7, nullptr, 0) == 7, "SFile2 absolute seek failed");
        Require(SFileReadFile(file.get(), bytes.data(), 9, &read, nullptr, 0) && read == 9 &&
            !std::memcmp(bytes.data(), expected.data() + 7, 9), "SFile2 seek/read mismatch");
        Require(SFile::SetFilePointer(file.get(), -5, nullptr, 2) == expected.size() - 5, "SFile2 end-relative seek failed");
        Require(!SFileReadFile(file.get(), bytes.data(), 9, &read, nullptr, 0) && read == 5 &&
            !std::memcmp(bytes.data(), expected.data() + expected.size() - 5, 5), "SFile2 short read/EOF mismatch");
    }
    void* data = nullptr;
    uint32_t size = 0;
    Require(SFile::LoadFile(name, &data, &size, 4, nullptr), "SFile2 whole-file load failed");
    std::unique_ptr<void, decltype(&SFile::Unload)> loaded(data, SFile::Unload);
    Require(size == expected.size() && !std::memcmp(data, expected.data(), size), "SFile2 whole-file content mismatch");
    Require(!std::memcmp(static_cast<uint8_t*>(data) + size, "\0\0\0\0", 4), "SFile2 extra bytes were not zeroed");
}

void CheckClientLocalFile(const std::string& scratchDirectory) {
    ScratchFile scratch(scratchDirectory);
    Unit output(IOPosixFileUnit::CreateWritable(scratch.path.c_str()));
    const std::vector<uint8_t> expected{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
    Require(output && output->Write(expected.data(), 0, uint32_t(expected.size())), "SFile2 scratch creation failed");
    output.reset();
    SFile::Initialize();
    SFile::SetBasePath(scratch.directory.c_str());
    SFile::EnableDirectAccess(1);
    // Override indexing must terminate even when a linked asset directory
    // points back to an ancestor (common in simulator/development layouts).
    const auto loop = std::filesystem::path(scratch.directory) / "cycle";
    std::filesystem::create_directory_symlink(scratch.directory, loop);
    SFile::RebuildHash();
    CheckClientFile("OFFSET-CHECK", expected); // case-insensitive index, actual spelling on disk
    std::filesystem::remove(loop);
    std::filesystem::rename(scratch.path, scratch.path + ".bz");
    scratch.path += ".bz";
    SFile::RebuildHash();
    SFileImpl* unsupported = nullptr;
    Require(!SFileOpenFile("offset-check", &unsupported) && !unsupported && SErrGetLastError() == 120,
        "Unimplemented SFile2 compressed loose-file branch returned a live handle");
    SFile::EnableDirectAccess(0);
    SFile::SetBasePath("");
    SFile::RebuildHash();
}

void CheckStormLog(const std::string& scratchDirectory) {
    ScratchFile scratch(scratchDirectory);
    HSLOG handle = nullptr;
    Require(SLogCreate(scratch.path.c_str(), 0, &handle), "Storm deferred log creation failed");
    Require(!std::filesystem::exists(scratch.path), "Storm deferred log opened too early");
    SLogWrite(handle, "%s", "progress 100% ready");
    auto write = [&] { for (int i = 0; i < 50; ++i) { SLogWrite(handle, "record %d", i); } };
    std::thread a(write), b(write);
    a.join(); b.join();
    SLogFlush(handle);
    Require(std::filesystem::file_size(scratch.path) > 0, "Storm flush did not reach file");
    SLogClose(handle);
    SLogWrite(handle, "stale handle");
    Require(SLogCreate(scratch.path.c_str(), SLOG_FLAG_OPENFILE | SLOG_FLAG_APPEND, &handle), "Storm append failed");
    SLogWrite(handle, "appended");
    SLogClose(handle);
    std::ifstream stream(scratch.path);
    std::string contents((std::istreambuf_iterator<char>(stream)), {});
    Require(contents.find("progress 100% ready\r\n") != std::string::npos &&
        contents.find("appended\r\n") != std::string::npos &&
        std::count(contents.begin(), contents.end(), '\n') == 102, "Storm concurrent/append output mismatch");
    const auto size = std::filesystem::file_size(scratch.path);
    Require(SLogCreate(scratch.path.c_str(), SLOG_FLAG_NOFILE | SLOG_FLAG_OPENFILE, &handle), "Storm no-file log failed");
    SLogWrite(handle, "must not write");
    SLogClose(handle);
    Require(std::filesystem::file_size(scratch.path) == size, "Storm no-file flag modified disk log");
}

void CheckCVars(const std::string& scratchDirectory) {
    ScratchFile scratch(scratchDirectory);
    const std::string directory = scratch.directory + "/settings", config = directory + "/Config.wtf";
    struct Cleanup {
        std::string directory;
        ~Cleanup() { CVar::Destroy(); ConsoleCommandDestroy(); CStringManager::DestroyManager(); std::error_code error; std::filesystem::remove_all(directory, error); }
    } cleanup{directory};
    CVar::Initialize(config.c_str());
    auto validate = [](CVar*, const char*, const char* value, void*) -> bool { return std::strcmp(value, "bad") != 0; };
    auto* glow = CVar::Register("ffxGlow", "probe glow setting", 1, "1", validate, CATEGORY_GRAPHICS, false, nullptr, false);
    Require(glow && glow->m_intValue == 1, "CVar registration failed");
    ConsoleCommandExecute("set ffxGlow \"0\"", 1);
    Require(glow->m_intValue == 0 && CVar::m_needsSave && !std::strcmp(ConsoleCommandHistory(0), "set ffxGlow \"0\""), "Shared CVar command or history failed");
    Require(glow->Set("bad", true, false, false, true) && glow->m_intValue == 0, "CVar callback rejection changed its value");
    glow->SetReadOnly(true); ConsoleCommandExecute("set ffxGlow \"1\"", 0);
    Require(glow->m_intValue == 0, "CVar read-only state was ignored"); glow->SetReadOnly(false);
    const char* destination = CVar::s_configFile; CVar::s_configFile = directory.c_str();
    Require(!CVar::SaveAll() && CVar::m_needsSave, "Failed CVar save lost pending configuration");
    CVar::s_configFile = destination;
    Require(CVar::SaveAll() && !CVar::m_needsSave && OsFileExists(config.c_str()), "CVar absolute-path save failed");
    CVar::Destroy(); ConsoleCommandDestroy();
    { std::ofstream file(config, std::ios::binary); file << "\xef\xbb\xbfSET ffxGlow \"0\"\r\nSET gxMultisample \"1\"\r\n"; }
    CVar::Initialize(config.c_str());
    glow = CVar::Register("ffxGlow", "probe glow setting", 1, "1", validate, CATEGORY_GRAPHICS, false, nullptr, false);
    Require(glow->m_intValue == 0 && !std::strcmp(glow->m_defaultValue.m_str, "1") && CVar::Lookup("gxMultisample")->m_intValue == 1,
        "CVar BOM/config reload or registered default overwrote the saved setting");
    ConsoleCommandExecute("ffxGlow 1", 0);
    Require(glow->m_intValue == 1 && CVar::SaveAll(), "Direct CVar command failed");
    // Exercise the safe fallback for an unhandled command, including the
    // original dispatcher's fixed-size argument buffer.
    g_defaultCommand = [](const char*, const char*) -> int32_t { return 0; };
    ConsoleCommandExecute("not-a-command", 0); g_defaultCommand = nullptr;
    ConsoleCommandRegister("run", [](const char*, const char* args) -> int32_t { Require(std::strlen(args) == 1023, "Console fallback argument bound failed"); return 1; }, CATEGORY_DEFAULT, "probe fallback");
    ConsoleCommandExecute(std::string(2048, 'x').c_str(), 0);
    Require(CVar::Delete(config.c_str()) && !OsFileExists(config.c_str()), "CVar explicit-path delete failed");
}

void CheckNativeSettings() {
    Require(OsGetPhysicalMemory() >= 1024 * 1024, "Native physical memory query failed");
    auto uuid = CFUUIDCreate(kCFAllocatorDefault);
    auto uuidString = CFUUIDCreateString(kCFAllocatorDefault, uuid);
    char unique[64];
    Require(CFStringGetCString(uuidString, unique, sizeof(unique), kCFStringEncodingUTF8), "Cannot create preference check key");
    CFRelease(uuidString); CFRelease(uuid);
    const auto path = std::string("wow-port-check-")+unique;
    auto nativeName = path+"\nvalue";
    std::transform(nativeName.begin(), nativeName.end(), nativeName.begin(), [](unsigned char c) { return std::tolower(c); });
    auto nativeKey = CFStringCreateWithCString(kCFAllocatorDefault, nativeName.c_str(), kCFStringEncodingUTF8);
    struct Cleanup {
        CFStringRef key;
        ~Cleanup() {
            CFPreferencesSetValue(key, nullptr, CFSTR("org.wowdecomp.client"), kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
            CFPreferencesSynchronize(CFSTR("org.wowdecomp.client"), kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
            CFRelease(key);
        }
    } cleanup{nativeKey};
    const auto flags = SREG_ABSOLUTE | SREG_EXCLUDE_LOCAL_MACHINE;
    uint32_t value = 17;
    Require(!SRegLoadValue(path.c_str(), "Value", flags, &value) && value == 17 && SErrGetLastError() == 2, "Missing preference changed output");
    Require(SRegSaveValue(path.c_str(), "Value", flags | SREG_FLUSH_KEY, UINT32_MAX), "Native DWORD preference write failed");
    Require(SRegLoadValue(path.c_str(), "VALUE", flags, &value) && value == UINT32_MAX, "Native DWORD preference was truncated or case-sensitive");
    char text[64];
    Require(SRegLoadString(path.c_str(), "value", flags, text, sizeof(text)) && !std::strcmp(text, "4294967295"), "DWORD preference string conversion changed");
    for (const auto& sample : {std::pair<const char*,uint32_t>{"0x1234",0x1234}, {"-1",UINT32_MAX}, {"4294967296",UINT32_MAX}}) {
        Require(SRegSaveString(path.c_str(), "value", flags, sample.first) && SRegLoadValue(path.c_str(), "Value", flags, &value) &&
            value == sample.second, "Native string preference does not preserve 32-bit DWORD parsing");
    }
    const char* utf8 = "\xE4\xB8\x96\xE7\x95\x8C-native";
    Require(SRegSaveString(path.c_str(), "value", flags, utf8) && SRegLoadString(path.c_str(), "value", flags, text, sizeof(text)) &&
        !std::strcmp(text, utf8), "Native preference changed UTF-8 bytes");
    Require(SRegLoadString(path.c_str(), "value", flags, text, 1) && !text[0], "Native preference did not terminate a short output buffer");
    Require(!SRegLoadValue(path.c_str(), "value", SREG_ABSOLUTE | SREG_EXCLUDE_CURRENT_USER | SREG_EXCLUDE_LOCAL_MACHINE, &value),
        "Excluded preference scope was read");
    Require(!SRegLoadValue(nullptr, "value", flags, &value) && SErrGetLastError() == 87, "Invalid preference key was accepted");
}

void CheckOsFiles(const std::string& scratchDirectory) {
    ScratchFile scratch(scratchDirectory);
    const std::string directory = scratch.directory + "/settings", filePath = directory + "/Config.wtf";
    struct Cleanup { std::string directory; ~Cleanup() { std::error_code error; std::filesystem::remove_all(directory, error); } } cleanup{directory};
    Require(OsCreateDirectory(directory.c_str(), 1) && OsDirectoryExists(directory.c_str()) &&
        (OsGetFileAttributes(directory.c_str()) & OS_FILE_ATTRIBUTE_DIRECTORY), "OsFile directory attributes do not match game callers");
    const char text[] = "SET ffxGlow \"1\"\r\n";
    auto file = OsCreateFile(filePath.c_str(), OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);
    Require(file != OS_INVALID_FILE, "OsFile config create failed");
    uint32_t bytes = 0;
    Require(OsWriteFile(file, text, sizeof(text) - 1, &bytes) && bytes == sizeof(text) - 1 &&
        OsGetFileSize(file) == sizeof(text) - 1 && OsCloseFile(file), "OsFile config write/size failed");
    Require(OsCreateFile(filePath.c_str(), OS_GENERIC_WRITE, 0, 1, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY) == OS_INVALID_FILE,
        "OsFile exclusive create overwrote config");
    file = OsCreateFile(filePath.c_str(), OS_GENERIC_WRITE, 0, 4, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);
    Require(file != OS_INVALID_FILE && OsWriteFile(file, text, sizeof(text) - 1, &bytes) && OsCloseFile(file), "OsFile legacy append failed");
    std::string windowsPath = filePath; std::replace(windowsPath.begin(), windowsPath.end(), '/', '\\');
    file = OsCreateFile(windowsPath.c_str(), OS_GENERIC_READ, 1, OS_OPEN_EXISTING, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);
    char contents[128]{};
    Require(file != OS_INVALID_FILE && OsReadFile(file, contents, sizeof(contents), &bytes) && bytes == 2 * (sizeof(text) - 1) &&
        !std::memcmp(contents, text, sizeof(text) - 1) && !std::memcmp(contents + sizeof(text) - 1, text, sizeof(text) - 1),
        "OsFile config path, contents or EOF count failed");
    Require(OsReadFile(file, contents, sizeof(contents), &bytes) && bytes == 0 && !OsWriteFile(file, text, 1, &bytes) && bytes == 0,
        "OsFile EOF or read-only handle semantics failed");
    Require(OsCloseFile(file) && !OsCloseFile(OS_INVALID_FILE), "OsFile invalid handle was accepted");
    Require(OsSetFileAttributes(filePath.c_str(), 3) && (OsGetFileAttributes(filePath.c_str()) & 3) == 3, "OsFile hidden/read-only attributes failed");
    std::vector<std::string> names;
    auto collect = [](OS_FILE_DATA& data, void* arg) -> int32_t { static_cast<std::vector<std::string>*>(arg)->push_back(data.fileName); return 0; };
    Require(OsFileList((directory + '/').c_str(), "*.wtf", collect, &names, 0) && names.empty(), "OsFile enumeration included hidden config");
    Require(OsFileList((directory + '/').c_str(), "*.WTF", collect, &names, 1) && names == std::vector<std::string>{"Config.wtf"},
        "OsFile enumeration pattern, basename or hidden flag failed");
    Require(OsSetFileAttributes(filePath.c_str(), OS_FILE_ATTRIBUTE_NORMAL), "OsFile attribute reset failed");
    const std::string copy = directory + "/Copy.wtf", moved = directory + "/Moved.wtf";
    Require(OsCopyFile(filePath.c_str(), copy.c_str(), 1) && !OsCopyFile(filePath.c_str(), copy.c_str(), 1) &&
        OsMoveFile(copy.c_str(), moved.c_str()) && !OsFileExists(copy.c_str()) && OsFileExists(moved.c_str()), "OsFile copy/move behavior failed");
    file = OsCreateFile(filePath.c_str(), OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);
    Require(file != OS_INVALID_FILE && OsGetFileSize(file) == 0, "OsFile config replacement did not truncate");
    constexpr int64_t offset = (int64_t(1) << 32) + 29;
    Require(Blizzard::File::SetPos(reinterpret_cast<Blizzard::File::StreamRecord*>(file), offset, 0) &&
        OsWriteFile(file, text, 1, &bytes) && OsGetFileSize(file) == uint64_t(offset + 1) && OsCloseFile(file), "OsFile truncated a 64-bit file size");
    Require(OsDeleteFile(filePath.c_str()) && OsRemoveDirectoryRecurse(directory.c_str(), 0), "OsFile settings cleanup failed");
}

void CheckFileStack(const std::string& scratchDirectory) {
    namespace File = Blizzard::File;
    ScratchFile scratch(scratchDirectory);
    Require(File::Exists(scratch.directory.c_str()) == 2, "file stack directory lookup failed");
    Require(File::IsAbsolutePath(scratch.path.c_str()) && File::GetRootChars(scratch.path.c_str()) == 1, "POSIX root lookup failed");
    struct CloseStream { void operator()(File::StreamRecord* stream) const { if (stream) { File::Close(stream); } } };
    using Stream = std::unique_ptr<File::StreamRecord, CloseStream>;
    File::StreamRecord* opened = nullptr;
    Require(File::Open(scratch.path.c_str(), 0x803, opened), "file stack exclusive create failed");
    Stream file(opened);
    Require(!File::Open(scratch.path.c_str(), 0x803, opened), "exclusive create overwrote existing file");
    Require(File::SetPos(file.get(), 7, 0), "file stack seek failed");
    constexpr int64_t offset = (int64_t(1) << 32) + 23;
    const char marker[] = "engine-native-io";
    int32_t count = sizeof(marker);
    Require(File::Write(file.get(), marker, offset, &count) && count == sizeof(marker), "file stack 64-bit write failed");
    int64_t position;
    Require(File::GetPos(file.get(), &position) && position == 7, "positional IO changed sequential cursor");
    Require(File::GetFileInfo(file.get())->m_size == offset + sizeof(marker), "file stack cached size stale after write");
    bool success[2]{};
    auto read = [&](int index) {
        char buffer[64]{};
        int32_t bytes = sizeof(buffer);
        success[index] = File::Read(file.get(), buffer, offset, &bytes) && bytes == sizeof(marker) && !std::memcmp(buffer, marker, bytes);
    };
    std::thread a(read, 0), b(read, 1);
    a.join(); b.join();
    Require(success[0] && success[1], "concurrent positional file reads failed");
    Require(File::SetEOF(file.get(), 13, 0) && File::GetFileInfo(file.get())->m_size == 13, "file truncate/cache invalidation failed");
    file.reset();
    std::string backslash = scratch.path;
    std::replace(backslash.begin(), backslash.end(), '/', '\\');
    Require(File::Open(backslash.c_str(), 0x1005, opened), "engine path separator conversion failed");
    file.reset(opened);
    count = sizeof(marker);
    Require(!File::Write(file.get(), marker, 0, &count) && File::GetFileInfo(file.get())->m_size == 13, "file stack read-only mode accepted write");
    Require(File::GetLastErrorStack() && File::GetLastErrorStack()->FindCategory(1), "native file error lost details");
    file.reset();
    char formatted[64];
    Blizzard::String::Format(formatted, sizeof(formatted), "%2$s %1$I64d", int64_t(4294967297), "offset");
    Require(!std::strcmp(formatted, "offset 4294967297"), "ordered 64-bit error formatting failed");
    char small[4];
    Require(Blizzard::String::Format(small, sizeof(small), "%s", "abcdef") == 3 && !std::strcmp(small, "abc"), "bounded formatter truncated incorrectly");
    Blizzard::Time::TimeRec date{};
    Blizzard::Time::BreakTime(-1, date);
    Require(date.year == 1999 && date.month == 12 && date.day == 31 && date.nanosecond == 999999999 &&
        Blizzard::Time::MakeTime(date) == -1 && Blizzard::Time::FromFileTime(125911584000000000ULL) == 0, "Blizzard timestamp epoch mismatch");
}
}

bool AppleBootstrap::CheckCore(const std::string& scratchDirectory, std::string& report) {
    try {
        // Recursion and atomic publication are required by the existing engine.
        CCritSect critical;
        critical.Enter(); critical.Enter(0); critical.Leave(0); critical.Leave();
        Blizzard::Lock::Atomic count{};
        std::thread a([&] { for (int i = 0; i < 10000; ++i) { count.Increment(); } });
        std::thread b([&] { for (int i = 0; i < 10000; ++i) { count.Increment(); } });
        a.join(); b.join();
        Require(count.Decrement() == 19999, "atomic count mismatch");
        struct Constructed {
            std::array<uint32_t, 256> values;
            Constructed() { values.fill(0xA14); }
        };
        Blizzard::Lock::ConstructOnce<Constructed> once{};
        bool published[2]{};
        std::thread firstReader([&] { published[0] = once.Get()->values.back() == 0xA14; });
        std::thread secondReader([&] { published[1] = once.Get()->values.back() == 0xA14; });
        firstReader.join(); secondReader.join();
        Require(published[0] && published[1], "ConstructOnce publication failed");
        once.Get()->~Constructed();
        Blizzard::Lock::EventObject event;
        Require(!Blizzard::Lock::EventWait(0, event.m_event), "new event was signaled");
        Blizzard::Lock::EventSet(event.m_event);
        Blizzard::Lock::EventSet(event.m_event);
        Require(Blizzard::Lock::EventWait(0, event.m_event), "event lost signal");
        Require(!Blizzard::Lock::EventWait(1, event.m_event), "event failed to auto-reset/coalesce");
        std::thread signal([&] { Blizzard::Lock::EventSet(event.m_event); });
        auto* signaled = Blizzard::Lock::EventWait(1000, event.m_event);
        signal.join();
        Require(signaled, "event did not wake waiter");
        CheckNativeThreads();
        CheckCRTRandom();
        CheckStormSynchronization();
        CheckGameNetwork();
        CheckURLDownloads();
        CheckKnowledgeBaseDownloads();
        CheckCommandLine(scratchDirectory);
        CheckVersionInfo(scratchDirectory);
        CheckNativeProcesses(scratchDirectory);
        CheckBattlenetCore();
        CheckFileDownloads(scratchDirectory);
        {
            char executable[4096], directory[4096];
            const uint32_t length = GetExecutablePathUTF8(executable, sizeof(executable));
            Require(length == std::strlen(executable) && length && executable[0] == '/' &&
                std::filesystem::is_regular_file(executable), "Native executable path is not the actual process image");
            OsGetExePath(directory, sizeof(directory));
            Require(std::string(directory) == std::filesystem::path(executable).parent_path().string() + "/",
                "Native executable directory lost its path or trailing separator");
            OsPathStripFilename(executable);
            Require(!std::strcmp(executable, directory) && OsPathGetRootChars(executable) == 1 &&
                OsPathGetRootChars("Interface\\Icons\\Spell.blp") == 0, "Native path root/filename handling changed");
            char root[] = "/file"; OsPathStripFilename(root);
            Require(!std::strcmp(root, "/"), "Native path stripped the filesystem root");
            char boundary[] = {'x', 'g'};
            Require(!GetExecutablePathUTF8(boundary, 1) && boundary[0] == 0 && boundary[1] == 'g',
                "Native path truncation damaged the output boundary");
            Require(!GetModuleFileNameUTF8(reinterpret_cast<void*>(1), boundary, 1) && errno == ENOTSUP,
                "Opaque module handle was mistaken for a native image address");
            for (bool user : {false, true}) {
                auto query = user ? OsGetUserName : OsGetComputerName;
                uint32_t required = 0;
                const int result = query(nullptr, &required);
#if TARGET_OS_IPHONE
                // Simulator's process UID need not have an entry in its own
                // password database. The original bug-report caller already
                // omits this field when the platform query returns false.
                if (user && !result && errno == ENOENT) {
                    char unavailable[8] = "guard"; uint32_t capacity = sizeof(unavailable);
                    Require(!query(unavailable, &capacity) && errno == ENOENT && !std::strcmp(unavailable, "guard"),
                        "Unavailable native account query published a fabricated name");
                    Require(!query(unavailable, nullptr) && errno == EINVAL, "Native account query accepted a missing size");
                    std::fputs("Native startup queries: system has no username for this process UID; original caller skips the optional report field\n", stderr);
                    continue;
                }
#endif
                Require(!result && errno == ERANGE && required > 1 && required < 65536,
                    "Native identity query did not return its required buffer size");
                std::vector<char> name(required + 1, 'g'); uint32_t capacity = required;
                Require(query(name.data(), &capacity) && name[required] == 'g' &&
                    capacity == std::strlen(name.data()) + (user ? 1 : 0), "Native identity successful length differs from original API");
                capacity = required - 1;
                Require(!query(name.data(), &capacity) && capacity == required && errno == ERANGE && name[0] == 0,
                    "Native identity query truncated or overwrote a short buffer");
                Require(!query(name.data(), nullptr) && errno == EINVAL, "Native identity query accepted a missing size");
            }
            char version[128]; OsGetVersionString(version, sizeof(version));
            Require((!std::strncmp(version, "macOS ", 6) || !std::strncmp(version, "iOS ", 4)) && std::strchr(version, '.'),
                "Native OS report does not identify its actual platform/version");
            boundary[0] = 'x'; OsGetVersionString(boundary, 1);
            Require(boundary[0] == 0 && boundary[1] == 'g', "Native version string exceeded its buffer");
            const uint32_t before = OsGetAsyncTimeMs(); const float seconds = OsGetAsyncTimeSec(); const uint32_t after = OsGetAsyncTimeMs();
            Require(before > after || (seconds >= float(before) * .001f && seconds <= float(after) * .001f),
                "Original float seconds wrapper no longer uses the native millisecond clock");
            std::fprintf(stderr, "Native startup queries: executable/root paths, identity buffer/length rules, version and original float clock passed (%s)\n", version);
        }
        const int32_t previousRace = StreamingGetRace();
        for (int32_t race : {-1, 1, 4, 11}) {
            StreamingSetRace(race);
            StreamingSetState(1); StreamingSetEnabled(true); StartDataPreloadThread();
            Require(!CheckIfStreamingAndTrial() && !Blizzard::Streaming::IsStreamingMode() &&
                StreamingGetRace() == race && StreamingIsRaceDataReady(race) && StreamingGetLoadFraction(race) == 1.0f,
                "Local installation no longer follows original disabled-streaming semantics");
            StreamingSetState(0); StreamingSetEnabled(false);
        }
        StreamingSetRace(previousRace);
        Require(!Blizzard::Streaming::InitializeStreaming("", "", "", "", false) && errno == ENOTSUP,
            "Local installation incorrectly accepted a remote streaming request");
        struct Library {
            void* module = OsLoadLibrary("/usr/lib/libSystem.B.dylib");
            ~Library() { OsFreeLibrary(module); }
        } library, secondLibrary;
        Require(library.module && secondLibrary.module, "Cannot load native system library");
        using ProcessId = int (*)();
        auto processId = reinterpret_cast<ProcessId>(OsGetLibraryEntryPoint(library.module, "getpid"));
        Require(processId && processId() == getpid(), "Native library symbol invocation failed");
        OsFreeLibrary(library.module); library.module = nullptr;
        processId = reinterpret_cast<ProcessId>(OsGetLibraryEntryPoint(secondLibrary.module, "getpid"));
        Require(processId && processId() == getpid() && !OsGetLibraryEntryPoint(secondLibrary.module, "wow_missing_export") &&
            !OsGetLibraryEntryPoint(secondLibrary.module, reinterpret_cast<const char*>(3)) &&
            !OsGetLibraryEntryPoint(nullptr, "getpid") && !OsGetLibraryEntryPoint(secondLibrary.module, nullptr) &&
            !OsLoadLibrary(nullptr) && !OsLoadLibrary("") && !OsLoadLibrary("wow_missing_native_library.dylib"),
            "Native library ownership or unsupported export handling changed");
        CheckGameLogin();
        CheckGameSession();
        CheckGameRealm();
        CheckCharacterTextureCache(nullptr);
        CheckBlpLoading();

        IOUnitContainer container(3u, 17u, 0u);
        bool owner;
        void* first = IOUnitContainer::Alloc(&container, 3, &owner);
        Require(owner, "container lost owner");
        void* second = IOUnitContainer::Alloc(&container, 17, &owner);
        const bool aligned = reinterpret_cast<uintptr_t>(second) % alignof(std::max_align_t) == 0;
        IOUnitContainer::Free(first, true);
        Require(aligned && !owner, "IO unit allocation alignment/ownership mismatch");

        C44Matrix transform(1.0f);
        transform.Translate(C3Vector(10, 20, 30));
        const auto position = C3Vector(1, 2, 3) * transform;
        Require(position == C3Vector(11, 22, 33), "Tempest transform mismatch");
        CDataStore store;
        store.Put(uint32_t(0x12345678)).PutString("arm64");
        Require(store.m_size == 10 && store.m_data[0] == 0x78, "CDataStore encoding mismatch");
        CRndSeed seed(42);
        Require(CRandom::uint32_(seed) != 0, "Tempest random failed");

        for (int profile : {0, 1}) {
            // Use the client's custom allocator and profiler, not just stock Lua.
            std::unique_ptr<void, decltype(&luaM_freePool)> pool(luaM_initPool(), luaM_freePool);
            std::unique_ptr<lua_State, decltype(&lua_close)> lua(lua_newstate(luaM_reallocPool, pool.get(), profile), lua_close);
            Require(bool(lua), "Lua allocation failed");
            luaL_openlibs(lua.get());
            ClientSrand(1);
            Require(luaL_dostring(lua.get(), "assert(math.random() == 41/32767); assert(math.random(10) == 6)") == 0 &&
                        ClientRand() == 6334,
                    "Lua random does not share the original client CRT sequence/range");
            const int result = luaL_dostring(lua.get(), "local t = {}; for i=1,10000 do t[i]=i end; collectgarbage('collect'); assert(string.upper('abc') == 'ABC'); return t[10000] + 42");
            Require(result == 0 && lua_tonumber(lua.get(), -1) == 10042, "Lua execution/GC/profiling failed");
        }
        std::fprintf(stderr, "Client CRT random: original 15-bit sequence, thread-local seed and shared Lua stream passed\n");

        // Sparse scratch data exercises large offsets without allocating 4 GiB.
        ScratchFile scratch(scratchDirectory);
        Unit file(IOPosixFileUnit::CreateWritable(scratch.path.c_str()));
        Require(bool(file), "cannot open scratch output");
        constexpr int64_t offset = (int64_t(1) << 32) + 17;
        const char marker[] = "native-arm64";
        Require(file->Write(marker, offset, sizeof(marker)), "64-bit write failed");
        int64_t length;
        Require(file->GetFileSize(&length) && length == offset + sizeof(marker), "64-bit file size mismatch");
        Require(file->Flush(), "scratch flush failed");
        file.reset();
        Unit read(IOPosixFileUnit::OpenRead(scratch.path.c_str()));
        Require(bool(read), "cannot reopen scratch read-only");
        Require(!read->Write(marker, offset, sizeof(marker)), "read-only file accepted write");
        Unit range(IORangeUnit::Create(nullptr, offset, length, read.release()));
        Require(bool(range), "cannot create IO range");
        std::array<char, 64> buffer{};
        uint32_t bytes = 0;
        Require(range->Read(buffer.data(), 0, buffer.size(), &bytes) && bytes == sizeof(marker), "range short read failed");
        Require(memcmp(marker, buffer.data(), sizeof(marker)) == 0, "positional read mismatch");
        Require(!range->Read(buffer.data(), 0, buffer.size(), nullptr), "exact read incorrectly accepted EOF");
        Require(!range->Read(buffer.data(), -1, 1, &bytes), "negative offset accepted");
        CheckFileStack(scratchDirectory);
        CheckOsFiles(scratchDirectory);
        CheckCVars(scratchDirectory);
        CheckNativeSettings();
        CheckClientLocalFile(scratchDirectory);
        CheckStormLog(scratchDirectory);
        report = "PASS: native Blizzard/Storm locks, wait-all/any, manual/auto events, semaphores, mutex abandonment, threads/TLS cleanup, atomics/publication, aligned IO chains, Tempest, CDataStore, Lua pool/GC/profiling, Blizzard file stack/errors/timestamps, game OsFile config IO/attributes/enumeration, shared CVar save/reload/commands, native physical memory and scoped persistent preferences, SFile2 case-insensitive loose-file IO and read-only/64-bit file IO.\n";
        return true;
    } catch (const std::exception& error) {
        report = std::string("Core check failed: ") + error.what() + "\n";
        return false;
    }
}

bool AppleBootstrap::InspectData(const std::string& dataRoot, std::string& report, const std::string& locale) {
    std::ostringstream out;
    try {
        std::vector<std::filesystem::path> archives;
        for (const auto& item : std::filesystem::recursive_directory_iterator(dataRoot)) {
            if (!item.is_regular_file()) { continue; }
            std::string extension = item.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });
            if (extension == ".mpq") { archives.push_back(item.path()); }
        }
        Require(!archives.empty(), "no MPQ archives under the configured Data root");
        std::sort(archives.begin(), archives.end());
        bool success = true;
        for (const auto& path : archives) {
            try { InspectArchive(path, out); }
            catch (const std::exception& error) {
                success = false;
                out << path.string() << ": " << error.what() << '\n';
            }
        }
        out << "Inspected " << archives.size() << " archives read-only.\n";
        ClientData client(dataRoot, locale);
        SFile::Initialize();
        SFile::EnableDirectAccess(0);
        out << "Client archive layout: " << client.Mounts().size() << " mounts, locale " << client.Locale() << '\n';
        for (const auto& mount : client.Mounts()) {
            out << "  priority " << mount.priority << ": " << mount.path;
            if (!mount.prefix.empty()) { out << " [" << mount.prefix << "\\]"; }
            out << '\n';
        }
        const char* resources[] = {
            "DBFilesClient\\Map.dbc", "Interface\\GlueXML\\GlueStrings.lua", "Cameras\\FlyByDwarf.m2",
            "World\\Maps\\AhnQiraj\\AhnQiraj_26_46.adt", "World\\wmo\\Azeroth\\Buildings\\AltarOfStorms\\AltarOfStorms.wmo",
            "Character\\BloodElf\\Female\\BLOODELFFEMALEFACELOWER00_00.BLP",
            "Character\\NorthrendSkeleton\\Male\\NorthrendSkeletonMale.M2"
        };
        for (const char* name : resources) {
            const auto asset = client.Read(name, 64 * 1024 * 1024, true);
            CheckClientFile(name, asset.bytes);
            if (std::strcmp(name, resources[0]) == 0) {
                Require(asset.bytes.size() >= 20 && !std::memcmp(asset.bytes.data(), "WDBC", 4), "Map.dbc header is invalid");
                const auto* data = asset.bytes.data();
                const uint64_t size = 20 + uint64_t(Read32(data + 4)) * Read32(data + 12) + Read32(data + 16);
                Require(size == asset.bytes.size(), "Map.dbc record/string extent is invalid");
            }
            out << "  resolved " << name << ": " << asset.bytes.size() << " bytes from " << asset.archive
                << (asset.digestVerified ? " (archive MD5 matched)" : " (no archive MD5)") << '\n';
        }
        out << "SFile2 sequential, preloaded, seek/EOF and whole-file reads match all 7 mounted resources.\n";
        CheckM2Resources(client, out);
        CheckRealmMetadata(client, out);
        CheckCharacterTextureCache(&client);
        CheckCharacterNames();
        CheckInstalledVersions(dataRoot);
        CheckMovieResources(dataRoot, out);
        CheckTerrainResources(client, out);
        CheckWorldData(client, out);
        CheckAudioResources(client, out);
        for (unsigned version : {2, 3}) {
            const std::string profile = std::to_string(version) + "_0\\UI.bls";
            const auto vertex = client.Read("Shaders\\Vertex\\vs_" + profile, 1024 * 1024);
            const auto pixel = client.Read("Shaders\\Pixel\\ps_" + profile, 1024 * 1024);
            auto ui = GxMetal::UIShader::FromBls(vertex.bytes, pixel.bytes);
            Require(ui.VertexMajorVersion() == version && ui.PixelMajorVersion() == version, "UI shader profile mismatch");
            auto reject = [](auto&& operation) {
                bool failed = false;
                try { operation(); } catch (const std::runtime_error&) { failed = true; }
                Require(failed, "Unsupported/truncated/over-budget shader was accepted");
            };
            auto changed = vertex.bytes;
            changed[28] ^= 1;
            reject([&] { GxMetal::UIShader::FromBls(changed, pixel.bytes); });
            changed = vertex.bytes;
            changed.pop_back();
            reject([&] { GxReadShaderBinary(changed, 1024 * 1024); });
            reject([&] { GxReadShaderBinary(vertex.bytes, 16); });
        }
        out << "Original UI vs/ps_2_0 and vs/ps_3_0 BLS recognized; modified code, truncation and memory budget checks passed.\n";
        out << "Full local archive layout and locale fallback are active. Client signature policy, full world/game startup, remaining Gx/Metal effects and game networking remain to be integrated.\n";
        report = out.str();
        return success;
    } catch (const std::exception& error) {
        report = out.str() + "Data inspection failed: " + error.what() + "\n";
        return false;
    }
}
