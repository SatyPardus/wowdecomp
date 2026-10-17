#include "WoW/Common/AaBsp.h"
#include "WoW/Common/Types.h"
#include "WoW/Source/WorldClient/Types.h"
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
}

void CheckWorldData(const ClientData& client, std::ostream& report) {
    CheckWmoRoots(client, report);
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
