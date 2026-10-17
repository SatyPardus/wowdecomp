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
void CheckGroup(CMapObjGroup& group) {
    Require((group.m_unkLoadedFlag & 1) && group.m_vertexListCount && group.m_indicesCount && group.m_cAaBspNodePtr1.m_nNodes,
        "Original WMO group geometry did not load");
    WmoData source(static_cast<const uint8_t*>(group.m_filePtr) + 88, group.m_fileSize - 88, WmoData::Stream::Children);
    const void* views[] = {group.m_polyList, group.m_indices, group.m_vertexList, group.m_normalList, group.m_textureVertexList, group.m_batchList};
    for (unsigned i = 0; i < 6; ++i)
        Require(!std::memcmp(views[i], source.At(i).Data(), source.At(i).size), "WMO authored geometry or batch bytes changed");
    Require(reinterpret_cast<uintptr_t>(group.m_cAaBspNodePtr1.m_nodes) % alignof(CAaBspNode) == 0,
        "WMO BSP nodes are unaligned");
    if (group.m_colorVertexList) {
        auto* colors = source.Require(0x4D4F4356).Records<CImVector>();
        const uint32_t split = group.m_transparencyBatchesCount ? group.m_batchList[group.m_transparencyBatchesCount - 1].m_vertexEnd + 1 : 0;
        for (int i = 0; i < group.m_colorVertexListSize; ++i) {
            CImVector expected = colors[i];
            if (!(group.m_parent->m_header->m_flags & 8)) {
                for (auto* value : {&expected.r, &expected.g, &expected.b}) {
                    const unsigned scaled = uint32_t(i) < split ? *value / 2 : (*value + (expected.a * *value / 64)) / 2;
                    *value = uint8_t(scaled > 255 ? 255 : scaled);
                }
                if (uint32_t(i) >= split) expected.a = 255;
            }
            Require(!std::memcmp(&expected, group.m_colorVertexList + i, sizeof(expected)), "WMO original vertex color correction changed");
        }
    }
    if (group.m_liquidVertexList) {
        const auto* raw = static_cast<const uint8_t*>(source.Require(0x4D4C4951).Data());
        const size_t count = size_t(group.m_liquidVerts.x) * group.m_liquidVerts.y;
        Require(reinterpret_cast<uintptr_t>(group.m_liquidVertexList) % alignof(SMOLiquidVert) == 0 &&
            !std::memcmp(group.m_liquidVertexList, raw + 30, count * sizeof(SMOLiquidVert)), "WMO liquid vertices changed or remain unaligned");
        for (size_t i = 0; i < count; ++i)
            Require(group.m_liquidVertArray->m_verts[i].z == group.m_liquidVertexList[i].m_waterVert.m_height,
                "Original WMO liquid generation changed authored heights");
        Require(!std::memcmp(group.m_liquidTileList, raw + 30 + count * sizeof(SMOLiquidVert),
            size_t(group.m_liquidTiles.x) * group.m_liquidTiles.y), "WMO liquid tile flags changed");
    }
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
        std::vector<unsigned> indices{0};
        if (path == roots[1]) indices.insert(indices.end(), {224, 281, 282, 283, 284});
        if (path == roots[2]) indices.insert(indices.end(), {48, 65, 69, 70, 71, 72, 73});
        for (auto index : indices) {
            object.ReadGroup(index); object.WaitLoadGroup(index);
            CheckGroup(*object.m_mapObjGroupArray[index]);
        }
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
    Require(failed.m_isGroupLoaded && !failed.m_rootReadFailed, "WMO read did not recover after failure");
    auto* group = failed.m_mapObjGroupArray[0];
    {
        AsyncFile::EnterQueueLock();
        struct Unlock { ~Unlock() { AsyncFile::LeaveQueueLock(); } } unlock;
        failed.ReadGroup(0);
        SFile::SetFilePointer(group->m_asyncObjPtr->m_file, 1, nullptr, 0);
    }
    rejected = false;
    try { failed.WaitLoadGroup(0); } catch (const std::runtime_error&) { rejected = true; }
    Require(rejected && group->m_groupReadFailed && !group->m_asyncObjPtr && !group->m_filePtr && !group->m_groupData &&
        !CMapObj::s_loadedGroups.Head(), "Failed WMO group read retained resources or queue entry");
    failed.ReadGroup(0, true); CheckGroup(*group);
    Require(!group->m_groupReadFailed, "WMO synchronous group retry did not recover");
    group->Clear();
    {
        AsyncFile::EnterQueueLock();
        struct Unlock { ~Unlock() { AsyncFile::LeaveQueueLock(); } } unlock;
        failed.ReadGroup(0); group->Clear();
    }
    Require(!group->m_asyncObjPtr && !group->m_filePtr && !CMapObj::s_loadedGroups.Head(), "Canceled WMO group retained resources");
    failed.ReadGroup(0); failed.WaitLoadGroup(0); CheckGroup(*group);
    failed.Clear();
    {
        AsyncFile::EnterQueueLock();
        struct Unlock { ~Unlock() { AsyncFile::LeaveQueueLock(); } } unlock;
        Require(failed.Read(name.data()), "Cannot queue WMO cancellation check"); failed.Clear();
    }
    Require(!failed.m_asyncObject && !failed.m_pWmoData && !CMapObj::s_asyncLoadQueue.Head(), "Canceled WMO retained its queue entry");
}
