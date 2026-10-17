#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <unistd.h>
#include "Tools/AppleBootstrap/Probe.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOUnitContainer.h"
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/Posix/IOPosixFileUnit.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/MPQEncrypt.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/Thread.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/Time/Time.h"
#include "Engine/Source/Storm/H/SCritSect.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lualib.h"
#include "Engine/Source/lua-5.1/src/lmemPool.h"

namespace {
struct ReleaseUnit { void operator()(IOUnit* unit) const { if (unit) { unit->Release(); } } };
using Unit = std::unique_ptr<IOUnit, ReleaseUnit>;

void Require(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
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

uint32_t Read32(const uint8_t* bytes) {
    return uint32_t(bytes[0]) | (uint32_t(bytes[1]) << 8) | (uint32_t(bytes[2]) << 16) | (uint32_t(bytes[3]) << 24);
}

void InspectArchive(const std::filesystem::path& path, std::ostream& out) {
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
            const int result = luaL_dostring(lua.get(), "local t = {}; for i=1,10000 do t[i]=i end; collectgarbage('collect'); assert(string.upper('abc') == 'ABC'); return t[10000] + 42");
            Require(result == 0 && lua_tonumber(lua.get(), -1) == 10042, "Lua execution/GC/profiling failed");
        }

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
        report = "PASS: native locks/events, threads/TLS cleanup, atomics/publication, aligned IO chains, Tempest, CDataStore, Lua pool/GC/profiling, Blizzard file stack/errors/timestamps and read-only/64-bit file IO.\n";
        return true;
    } catch (const std::exception& error) {
        report = std::string("Core check failed: ") + error.what() + "\n";
        return false;
    }
}

bool AppleBootstrap::InspectData(const std::string& dataRoot, std::string& report) {
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
        out << "Inspected " << archives.size() << " archives read-only. Inspection order is not game mount order.\n"
            << "Not yet available: SFile mounting/patch and locale resolution, game loop, Metal, audio, networking.\n";
        report = out.str();
        return success;
    } catch (const std::exception& error) {
        report = out.str() + "Data inspection failed: " + error.what() + "\n";
        return false;
    }
}
