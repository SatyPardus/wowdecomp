#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Storm/Source/SThread.h"
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
void Require(bool value, const char* message) { if (!value) { throw std::runtime_error(message); } }
struct Read {
    CAsyncObject* object = nullptr;
    std::vector<uint8_t> expected, bytes;
    int succeeded = 0, failed = 0;
    bool cleanupOnFailure = false;
    uint32_t mainThread = SGetCurrentThreadId();
    Read(const char* name, bool pastEnd = false) {
        SFileImpl* file = nullptr;
        Require(SFile::OpenEx(nullptr, name, 0, &file), "Cannot open async probe game resource");
        const uint32_t size = SFileGetFileSize(file, nullptr);
        expected.resize(size);
        Require(SFileReadFile(file, expected.data(), size, nullptr, nullptr, 0), "Cannot read async reference bytes");
        SFile::SetFilePointer(file, 0, nullptr, 0);
        bytes.resize(size + (pastEnd ? 1 : 0));
        object = AsyncFileReadAllocObject();
        object->m_file = file; object->m_buffer = bytes.data(); object->m_size = uint32_t(bytes.size());
        object->m_userArg = this;
        object->m_userPostloadCallback = reinterpret_cast<void*>(Success);
        object->m_userPostUnloadCallback = reinterpret_cast<void*>(Failure);
    }
    ~Read() { if (object) { AsyncFileReadDestroyObject(object); } }
    static void Success(void* arg) {
        auto& read = *static_cast<Read*>(arg);
        Require(SGetCurrentThreadId() == read.mainThread, "Async resource callback escaped the game thread");
        Require(read.bytes == read.expected && !read.object->m_readFailed, "Async game resource bytes changed");
        ++read.succeeded;
    }
    static void Failure(void* arg) {
        auto& read = *static_cast<Read*>(arg);
        Require(SGetCurrentThreadId() == read.mainThread, "Async resource cleanup escaped the game thread");
        ++read.failed;
        if (read.cleanupOnFailure) { AsyncFileReadDestroyObject(read.object); read.object = nullptr; }
    }
};
int32_t Initialize(const void*, void*) { AsyncFileReadInitialize(0, 20); return 1; }
int32_t Destroy(const void*, void*) { AsyncFileReadDestroy(); return 1; }
}

void CheckGameAsyncFiles() {
    for (int cycle = 0; cycle < 2; ++cycle) {
        PropInitialize(); EventInitialize(1, 0);
        struct Cleanup { bool active = true; ~Cleanup() { if (active) { EventDestroy(); } PropDestroy(); } } cleanup;
        EventCreateContextEx(1, Initialize, Destroy, 1, 0);
        EventProcessPending();
        {
            std::vector<std::unique_ptr<Read>> reads;
            for (const char* name : {"DBFilesClient\\Map.dbc", "Interface\\GlueXML\\GlueStrings.lua", "Cameras\\FlyByDwarf.m2",
                "World\\Maps\\AhnQiraj\\AhnQiraj_26_46.adt", "World\\wmo\\Azeroth\\Buildings\\AltarOfStorms\\AltarOfStorms.wmo"}) {
                reads.push_back(std::make_unique<Read>(name));
                AsyncFileReadObject(reads.back()->object, cycle);
            }
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            bool done = false;
            while (!done && std::chrono::steady_clock::now() < deadline) {
                EventProcessPending(); done = true;
                for (const auto& read : reads) { done &= read->succeeded == 1; }
                if (!done) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
            }
            Require(done, "Async game resource worker did not complete");
            // Waiting on already processed objects must not poison later waits.
            for (const auto& read : reads) { AsyncFileReadWait(read->object); }
            Read failed("DBFilesClient\\Map.dbc", true);
            AsyncFileReadObject(failed.object, 1);
            AsyncFileReadWait(failed.object);
            Require(failed.failed == 1 && failed.succeeded == 0 && failed.object->m_readFailed,
                "Failed async read was reported as successful");
            Read canceled("Cameras\\FlyByDwarf.m2");
            AsyncFile::EnterQueueLock();
            AsyncFileReadObject(canceled.object, 0);
            const auto canceledNow = AsyncFile::Cancel(canceled.object, nullptr);
            AsyncFile::LeaveQueueLock();
            Require(canceledNow == 1, "Queued async read was not canceled under its queue lock");
            canceled.object = nullptr; // Cancel returned the object to the recycler.
            AsyncFileReadWaitAll();
            Require(!AsyncFileReadIsReading(), "Async file queue did not become idle");
        }
        // A queued or just-completed read is cleaned up exactly once during
        // shutdown, without a success callback after its context is destroyed.
        Read abandoned("World\\Maps\\AhnQiraj\\AhnQiraj_26_46.adt");
        abandoned.cleanupOnFailure = true;
        AsyncFileReadObject(abandoned.object, 0);
        EventDestroy(); cleanup.active = false;
        Require(abandoned.failed == 1 && !abandoned.succeeded && !abandoned.object,
            "Async shutdown lost or duplicated pending resource cleanup");
    }
}
