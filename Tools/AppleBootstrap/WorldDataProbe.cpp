#include "WoW/Common/AaBsp.h"
#include "WoW/Common/Types.h"
#include "WoW/Source/WorldClient/Types.h"
#include "WoW/Source/WorldClient/MapLowDetailData.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "Engine/Source/Base/BitArray.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <vector>

void CheckWmoRoots(const ClientData& client, std::ostream& report);

namespace {
void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
uint32_t Word(const std::vector<uint8_t>& bytes, size_t offset) {
    Require(offset <= bytes.size() && bytes.size()-offset >= 4, "Truncated world data fixture");
    uint32_t result; std::memcpy(&result, bytes.data()+offset, 4); return result;
}
template<class T> std::vector<T> Chunk(const std::vector<uint8_t>& bytes, uint32_t token) {
    // Group geometry follows the 68-byte MOGP header. Actual WMO child payloads
    // need not be aligned: copy them before passing typed arrays to the core.
    Require(Word(bytes, 12) == 0x4D4F4750 && Word(bytes, 16) == bytes.size()-20, "Unexpected WMO group container");
    for (size_t offset = 88; offset < bytes.size();) {
        const auto size = Word(bytes, offset+4);
        Require(size <= bytes.size()-offset-8, "Invalid WMO child extent");
        if (Word(bytes, offset) == token) {
            Require(size % sizeof(T) == 0, "WMO child has partial record");
            std::vector<T> result(size/sizeof(T)); std::memcpy(result.data(), bytes.data()+offset+8, size); return result;
        }
        offset += 8+size;
    }
    throw std::runtime_error("Required WMO group child missing");
}
void CheckWdl(const ClientData& client, std::ostream& report) {
    unsigned total = 0, placements = 0;
    for (const char* map : {"Azeroth", "Northrend", "Expansion01"}) {
        const std::string path = std::string("World\\Maps\\") + map + "\\" + map + ".wdl";
        const auto asset = client.Read(path, MapLowDetailData::MaxBytes);
        const auto& bytes = asset.bytes;
        // Move the file base too: every typed output must own its alignment.
        std::vector<uint8_t> shifted(bytes.size() + 1);
        std::memcpy(shifted.data() + 1, bytes.data(), bytes.size());
        MapLowDetailData data(shifted.data() + 1, bytes.size());
        size_t indexAt = 12, objectAt = 0, namesAt = 0;
        if (Word(bytes, indexAt) == 0x4D574D4F) {
            const auto namesSize = Word(bytes, indexAt + 4);
            Require(data.names.size() == namesSize && (!namesSize || !std::memcmp(data.names.data(), bytes.data() + indexAt + 8, namesSize)), "WDL filenames changed");
            namesAt = indexAt;
            indexAt += namesSize + 8;
            const auto offsetsSize = Word(bytes, indexAt + 4);
            Require(data.nameOffsets.size() * 4 == offsetsSize && (!offsetsSize || !std::memcmp(data.nameOffsets.data(), bytes.data() + indexAt + 8, offsetsSize)), "WDL filename offsets changed");
            indexAt += offsetsSize + 8;
            objectAt = indexAt;
            const auto objectsSize = Word(bytes, indexAt + 4);
            Require(data.objects.size() * sizeof(SMMapObjDef) == objectsSize && (!objectsSize || !std::memcmp(data.objects.data(), bytes.data() + indexAt + 8, objectsSize)), "WDL placements changed");
            indexAt += objectsSize + 8;
        }
        Require(!std::memcmp(data.areaOffsets.data(), bytes.data() + indexAt + 8, sizeof(data.areaOffsets)), "WDL area offsets changed");
        size_t first = data.areaOffsets.size();
        for (size_t i = 0; i < data.areaOffsets.size(); ++i) {
            const auto offset = Word(bytes, indexAt + 8 + i * 4);
            if (!offset) { Require(data.areaIndices[i] == -1, "Missing WDL area acquired data"); continue; }
            if (first == data.areaOffsets.size()) first = i;
            const auto& area = data.areas.at(data.areaIndices[i]);
            Require(!std::memcmp(area.heights.data(), bytes.data() + offset + 8, sizeof(area.heights)), "WDL heights changed");
            const size_t next = size_t(offset) + Word(bytes, offset + 4) + 8;
            const bool holes = next < bytes.size() && Word(bytes, next) == 0x4D41484F;
            Require(area.hasHoles == holes && (!holes || !std::memcmp(area.holes.data(), bytes.data() + next + 8, sizeof(area.holes))), "WDL hole masks changed");
            ++total;
        }
        Require(first < data.areaOffsets.size(), "WDL fixture contains no horizon tiles");
        placements += data.objects.size();
        const auto originalHeight = data.areas.at(data.areaIndices[first]).heights[0];
        std::fill(shifted.begin(), shifted.end(), 0);
        Require(data.areas.at(data.areaIndices[first]).heights[0] == originalHeight, "WDL retained source storage");
        auto reject = [&](std::vector<uint8_t> changed) {
            bool failed = false;
            try { MapLowDetailData invalid(changed.data(), changed.size()); } catch (const std::runtime_error&) { failed = true; }
            Require(failed, "Invalid WDL was accepted");
        };
        auto write = [](std::vector<uint8_t>& target, size_t offset, uint32_t value) { std::memcpy(target.data() + offset, &value, 4); };
        auto changed = bytes; changed.pop_back(); reject(changed);
        changed = bytes; write(changed, indexAt + 8 + first * 4, data.areaOffsets[first] + 1); reject(changed);
        changed = bytes; write(changed, indexAt + 8 + first * 4, uint32_t(bytes.size())); reject(changed);
        changed = bytes; write(changed, data.areaOffsets[first] + 4, 1088); reject(changed);
        changed = bytes; write(changed, 8, 17); reject(changed);
        if (!data.objects.empty()) {
            changed = bytes; write(changed, objectAt + 8, uint32_t(data.nameOffsets.size())); reject(changed);
            const size_t nameIndex = namesAt + 8 + Word(bytes, namesAt + 4);
            changed = bytes; write(changed, nameIndex + 8, uint32_t(data.names.size())); reject(changed);
        }
        // The final area may omit MAHO. No speculative read beyond EOF is valid.
        changed = bytes;
        for (size_t i = 0; i < data.areaOffsets.size(); ++i) if (i != first) write(changed, indexAt + 8 + i * 4, 0);
        changed.resize(size_t(data.areaOffsets[first]) + 8 + 1090);
        MapLowDetailData noHoles(changed.data(), changed.size());
        Require(noHoles.areas.size() == 1 && !noHoles.areas[0].hasHoles && noHoles.areas[0].heights[0] == originalHeight, "WDL optional final hole mask failed");
    }
    report << "Original WDL: " << total << " horizon tiles and " << placements << " WMO placements preserve all height/hole/table bytes; odd alignment, source independence, bounds and optional final holes passed.\n";
}
}

void CheckWorldData(const ClientData& client, std::ostream& report) {
    CheckWdl(client, report);
    CheckWmoRoots(client, report);
    g_footprintTexturesDB.Load(__FILE__, __LINE__);
    g_groundEffectDoodadDB.Load(__FILE__, __LINE__);
    auto checkEffects = [&](const char* name, unsigned width, unsigned count, auto recordAt, auto pathAt) {
        const auto asset = client.Read(name, 1024*1024, true);
        Require(Word(asset.bytes, 4) == count && Word(asset.bytes, 12) == width, "World effect DBC extent differs");
        const size_t strings = 20+size_t(count)*width;
        for (unsigned i = 0; i < count; ++i) {
            const auto row = 20+size_t(i)*width;
            const auto* record = recordAt(i);
            Require(record && uint32_t(record->m_id) == Word(asset.bytes,row), "World effect record ID changed");
            const auto offset = Word(asset.bytes,row+4);
            Require(strings+offset < asset.bytes.size() && !std::strcmp(pathAt(record), reinterpret_cast<const char*>(asset.bytes.data()+strings+offset)),
                "World effect resource name changed on arm64");
        }
    };
    checkEffects(FootprintTexturesRec::GetFilename(), 8, g_footprintTexturesDB.m_numRecords,
        [](unsigned i) { return g_footprintTexturesDB.GetRecordByIndex(i); }, [](const FootprintTexturesRec* r) { return r->m_footstepFilename; });
    checkEffects(GroundEffectDoodadRec::GetFilename(), 12, g_groundEffectDoodadDB.m_numRecords,
        [](unsigned i) { return g_groundEffectDoodadDB.GetRecordByIndex(i); }, [](const GroundEffectDoodadRec* r) { return r->m_doodadpath; });
    report << "Original world effect tables: " << g_footprintTexturesDB.m_numRecords << " footprint textures, "
        << g_groundEffectDoodadDB.m_numRecords << " ground doodads; native IDs/resource names passed.\n";

    const auto asset = client.Read("World\\wmo\\Azeroth\\Buildings\\AltarOfStorms\\AltarOfStorms_000.wmo", 64*1024*1024, true);
    auto nodes = Chunk<CAaBspNode>(asset.bytes, 0x4D4F424E);
    auto references = Chunk<uint16_t>(asset.bytes, 0x4D4F4252);
    auto faces = Chunk<SMOPoly>(asset.bytes, 0x4D4F5059);
    auto indices = Chunk<uint16_t>(asset.bytes, 0x4D4F5649);
    auto vertices = Chunk<C3Vector>(asset.bytes, 0x4D4F5654);
    Require(indices.size() == faces.size()*3, "WMO face/index count mismatch");
    for (auto index : indices) Require(index < vertices.size(), "WMO vertex index out of bounds");
    for (auto face : references) Require(face < faces.size(), "WMO BSP face out of bounds");
    for (const auto& node : nodes)
        Require(node.m_faceStart <= references.size() && node.m_nFaces <= references.size()-node.m_faceStart,
            "WMO BSP face range out of bounds");
    auto cache = std::make_unique<CAaBspDigestCache>();
    Require(!g_BspDigestCache, "World digest cache unexpectedly initialized");
    g_BspDigestCache = cache.get();
    struct ClearGlobal { ~ClearGlobal() { g_BspDigestCache = nullptr; } } clearGlobal;
    CAaBsp bsp; bsp.Set(nodes.data(), uint32_t(nodes.size()), references.data(), uint32_t(references.size()), CAaBox::Bounding(vertices.data(), uint32_t(vertices.size())));
    unsigned leaves = 0, digestedFaces = 0;
    for (const auto& node : nodes) if (node.m_flags & 4) {
        ++leaves;
        const auto* digest = cache->GetDigest(bsp, &node, faces.data(), vertices.data(), indices.data());
        if (!digest) {
            // Original caller falls back to the uncompressed face list when a
            // leaf exceeds the digest's 300-face/450-vertex capacity.
            std::vector<uint16_t> unique;
            for (unsigned i = 0; i < node.m_nFaces; ++i) for (unsigned j = 0; j < 3; ++j)
                unique.push_back(indices[references[node.m_faceStart+i]*3+j]);
            std::sort(unique.begin(), unique.end()); unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
            Require(node.m_nFaces > 300 || unique.size() > 450, "BSP digest unexpectedly unavailable");
            continue;
        }
        Require(cache->GetDigest(bsp, &node, faces.data(), vertices.data(), indices.data()) == digest, "BSP digest cache missed a live leaf");
        Require(digest->m_nFaces == node.m_nFaces && digest->m_nVertices <= 450, "BSP digest count changed");
        for (unsigned i = 0; i < digest->m_nFaces; ++i) {
            const auto face = references[node.m_faceStart+i];
            Require(digest->m_faceIndices[i] == face && digest->m_faceFlags[i] == (faces[face].m_flags & ~0x80), "BSP digest lost original face flags/order");
            for (unsigned j = 0; j < 3; ++j) {
                const auto slot = digest->m_faceVertexIndices[i*3+j], vertex = indices[face*3+j];
                Require(slot < digest->m_nVertices && digest->m_vertexIndices[slot] == vertex &&
                    !std::memcmp(&digest->m_vertices[slot], &vertices[vertex], sizeof(C3Vector)), "BSP digest changed geometry");
            }
        }
        digestedFaces += digest->m_nFaces;
    }
    bsp.Clear();
    for (const auto* node : cache->m_nodes) Require(!node, "BSP clear retained stale leaf pointers");
    Require(leaves && digestedFaces, "Real WMO did not exercise BSP digests");
    CBitArray visited; visited.SetRawData(new char[(faces.size()+7)/8]{}, uint32_t(faces.size()), true);
    for (auto face : references) visited.Set(face, true);
    unsigned marked = 0; for (unsigned i = 0; i < faces.size(); ++i) marked += visited.Get(i) != 0;
    Require(marked > 0, "World face bit-array did not preserve marks");
    for (auto face : references) visited.Set(face, false);
    for (unsigned i = 0; i < faces.size(); ++i) Require(!visited.Get(i), "World face bit-array clear failed");
    g_liquidTypeDB.Load(__FILE__, __LINE__); g_liquidMaterialDB.Load(__FILE__, __LINE__);
    Require(g_liquidTypeDB.m_loaded && g_liquidMaterialDB.m_loaded && g_liquidTypeDB.m_numRecords > 0, "Liquid databases failed to load");
    const auto liquids = client.Read("DBFilesClient\\LiquidType.dbc", 1024*1024, true);
    Require(Word(liquids.bytes, 4) == unsigned(g_liquidTypeDB.m_numRecords) && Word(liquids.bytes, 12) == 180, "Liquid DBC fixture differs from active database");
    const auto strings = 20+g_liquidTypeDB.m_numRecords*180;
    for (int i = 0; i < g_liquidTypeDB.m_numRecords; ++i) {
        const size_t row = 20+i*180; auto* record = g_liquidTypeDB.GetRecordByIndex(i);
        Require(record && record == g_liquidTypeDB.GetRecord(Word(liquids.bytes, row)), "Liquid record lookup changed");
        const auto name = Word(liquids.bytes, row+4);
        Require(strings+name < liquids.bytes.size() && !std::strcmp(record->m_name, reinterpret_cast<const char*>(liquids.bytes.data()+strings+name)), "Liquid name pointer changed on arm64");
        Require(record->m_flags == int32_t(Word(liquids.bytes, row+8)) && record->m_materialID == int32_t(Word(liquids.bytes, row+56)), "Liquid flags/material changed");
        Require(g_liquidMaterialDB.GetRecord(record->m_materialID), "Liquid refers to missing material record");
        for (unsigned j = 0; j < 6; ++j) {
            const auto texture = Word(liquids.bytes, row+60+j*4);
            Require(strings+texture < liquids.bytes.size() && !std::strcmp(record->m_texture[j], reinterpret_cast<const char*>(liquids.bytes.data()+strings+texture)), "Liquid texture pointer changed on arm64");
        }
        Require(!std::memcmp(record->m_color, liquids.bytes.data()+row+84, 8) && !std::memcmp(record->m_float, liquids.bytes.data()+row+92, 72) &&
            !std::memcmp(record->m_int, liquids.bytes.data()+row+164, 16), "Liquid render parameters changed");
    }
    report << "  world support: " << nodes.size() << " real WMO BSP nodes / " << leaves << " leaves / " << digestedFaces
        << " digested faces, cache invalidation and bit-array marks; " << g_liquidTypeDB.m_numRecords << " original liquid records and "
        << g_liquidMaterialDB.m_numRecords << " material records passed\n";
}
