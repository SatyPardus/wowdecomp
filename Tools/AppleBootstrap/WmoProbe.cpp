#include "WoW/Source/WorldClient/MapObj.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/WmoData.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Base/ObjectAlloc.h"
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace {
constexpr const char* roots[] = {
    "World\\wmo\\Azeroth\\Buildings\\AltarOfStorms\\AltarOfStorms.wmo",
    "World\\wmo\\Azeroth\\Buildings\\Stormwind\\Stormwind.wmo",
    "World\\wmo\\Northrend\\Dalaran\\ND_Dalaran.wmo"
};
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void SetBytes(CMapObj& object, const std::vector<uint8_t>& bytes) {
    object.m_pWmoData = SMemAlloc(bytes.size(), __FILE__, __LINE__, 0);
    object.m_wmoFileSize = int32_t(bytes.size());
    std::memcpy(object.m_pWmoData, bytes.data(), bytes.size());
}
}

void CheckWmoRoots(const ClientData& client, std::ostream& report) {
    CMapChunk chunk; CMapChunk::InitializeVertexGrid();
    Require(chunk.m_type == 5 && CMapChunk::vertexList[144].x < 0, "Original world chunk did not initialize");
    uint32_t materials = 0, groups = 0, doodads = 0;
    for (const char* path : roots) {
        const auto asset = client.Read(path, WmoData::MaxBytes, true);
        WmoData original(asset.bytes.data(), asset.bytes.size());
        auto& sourceMaterials = original.Require(0x4D4F4D54);
        CMapObj object;
        for (unsigned cycle = 0; cycle < 2; ++cycle) {
            SetBytes(object, asset.bytes); object.Load(); object.CreateMaterials();
            Require(object.m_groupInfoCount > 0 && object.m_doodadDefCount > 0 &&
                object.m_materialsCount == int32_t(sourceMaterials.Count<SMOMaterial>()), "WMO root counts changed");
            Require(reinterpret_cast<uintptr_t>(object.m_materialList) % alignof(void*) == 0, "WMO native material handles are unaligned");
            for (int32_t i = 0; i < object.m_materialsCount; ++i) {
                Require(!std::memcmp(&object.m_materialList[i], &sourceMaterials.Records<SMOMaterial>()[i], 48), "WMO authored material fields changed");
                Require(!object.m_materialList[i].Textures()[0] && !object.m_materialList[i].Textures()[1], "WMO serialized padding became a runtime pointer");
            }
            Require(!std::memcmp(object.m_pWmoData, asset.bytes.data(), asset.bytes.size()), "WMO loader modified its original file bytes");
            if (!cycle) { materials += object.m_materialsCount; groups += object.m_groupInfoCount; doodads += object.m_doodadDefCount; }
            object.Clear();
            Require(!object.m_rootData && !object.m_header && !object.m_materialList && !object.m_materialsCount && !object.m_isGroupLoaded,
                "WMO clear retained native resource views");
        }
        auto truncated = asset.bytes; truncated.pop_back(); SetBytes(object, truncated);
        bool rejected = false;
        try { object.Load(); } catch (const std::runtime_error&) { rejected = true; }
        Require(rejected && !object.m_header && !object.m_materialList, "Truncated WMO published partial runtime data");
    }
    report << "  WMO roots: " << materials << " materials, " << groups << " groups, " << doodads
           << " doodads; original chunk counts, aligned runtime handles, immutable source bytes, truncation and recreation passed\n";
}

void CheckGameWmoRoots() {
    // Retain the probe heap just like the other shared ObjectAlloc caches.
    static uint32_t groupHeap = ObjectAllocAddHeap(sizeof(CMapObjGroup), 32, "WMO root probe", true);
    struct Restore { uint32_t* previous; ~Restore() { CMap::mapObjGroupHeap = previous; } } restore{CMap::mapObjGroupHeap};
    CMap::mapObjGroupHeap = &groupHeap;
    for (const char* path : roots) {
        CMapObj object;
        std::vector<char> name(path, path + std::strlen(path) + 1);
        Require(object.Read(name.data()), "Original WMO async read did not start"); object.WaitLoad();
        Require(!object.m_asyncObject && object.m_isGroupLoaded && object.m_mapObjGroupCount == object.m_groupInfoCount,
            "Original WMO postload did not create its groups");
        if (path == roots[0]) for (int32_t i = 0; i < object.m_materialsCount; ++i) {
            object.CreateMaterial(uint8_t(i)); CStatus status;
            auto* texture = object.m_materialList[i].Textures()[0];
            Require(texture && TextureGetGxTex(texture, 1, &status) && status.IsEmpty(), "Original WMO material texture did not become resident");
        }
    }
    CMapObj failed;
    std::vector<char> name(roots[0], roots[0] + std::strlen(roots[0]) + 1);
    {
        AsyncFile::EnterQueueLock();
        struct Unlock { ~Unlock() { AsyncFile::LeaveQueueLock(); } } unlock;
        Require(failed.Read(name.data()), "Cannot queue WMO failure check");
        // Request the original length starting one byte into the real file.
        // The worker can write at most size-1 bytes and must report EOF failure.
        SFile::SetFilePointer(failed.m_asyncObject->m_file, 1, nullptr, 0);
    }
    bool rejected = false;
    try { failed.WaitLoad(); } catch (const std::runtime_error&) { rejected = true; }
    Require(rejected && failed.m_rootReadFailed && !failed.m_asyncObject && !failed.m_pWmoData && !failed.m_header,
        "Failed WMO read retained pending or partial resources");
    Require(failed.Read(name.data()), "Cannot retry WMO read after failure"); failed.WaitLoad();
    Require(failed.m_isGroupLoaded && !failed.m_rootReadFailed, "WMO read did not recover after failure"); failed.Clear();
    {
        AsyncFile::EnterQueueLock();
        struct Unlock { ~Unlock() { AsyncFile::LeaveQueueLock(); } } unlock;
        Require(failed.Read(name.data()), "Cannot queue WMO cancellation check"); failed.Clear();
    }
    Require(!failed.m_asyncObject && !failed.m_pWmoData && !CMapObj::s_asyncLoadQueue.Head(), "Canceled WMO retained its queue entry");
}
