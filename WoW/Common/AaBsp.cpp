#include <cstddef>
#include <cstdint>
#include <cstring>
#include "WoW/Common/AaBsp.h"
#include "WoW/Common/Types.h"
#include "WoW/Source/WorldClient/Types.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00CDD7A0
CAaBspDigestCache* g_BspDigestCache;

// STATIC: 0x00CDF7BC
int32_t CAaBspDigestCache::s_digestReplaceIndex;

// VERIFY: d3f1a428 auto
// OFFSET: 0x0079ADC0
void CAaBsp::Set(CAaBspNode* nodes, uint32_t nNodes, uint16_t* nodeFaceIndices, uint32_t nNodeFaceIndices, const CAaBox& aaBox) {
    this->m_nodes = nodes;
    this->m_rootNode = nodes;
    this->m_nNodes = nNodes;
    this->m_nNodeFaceIndices = nNodeFaceIndices;
    this->m_bFree = 0;
    this->m_nodeFaceIndices = nodeFaceIndices;
    this->m_aaBox = aaBox;
}

// VERIFY: 0794718c auto
// OFFSET: 0x0079AE10
void CAaBspDigestCache::RemoveEntry(const CAaBspNode* node) {
    uint32_t bucket = ((reinterpret_cast<uintptr_t>(node) >> 5) ^ node->m_nFaces) & 0x7F;

    for (int32_t i = 0; i < 8; i++) {
        if (this->m_nodes[bucket * 8 + i] == node) {
            this->m_nodes[bucket * 8 + i] = nullptr;
            memset(&this->m_digests[bucket * 8 + i], 0, sizeof(this->m_digests[bucket * 8 + i]));
            return;
        }
    }
}

// VERIFY: b71147d6 manual
// OFFSET: 0x0079AE80
void CAaBspDigestCache::GenerateDigest(CAaBspNodeDigest* digest, const CAaBsp& bsp, const CAaBspNode* node, SMOPoly* faces, const C3Vector* vertices, const uint16_t* faceVertexIndices) {
    uint16_t slots[1024];
    uint16_t hash[1024];

    memset(digest, 0, sizeof(*digest));
    digest->m_node = node;
    digest->m_status = 0;
    digest->m_nVertices = 0;
    digest->m_nFaces = 0;
    memset(hash, 0xFF, sizeof(hash));

    if (node->m_nFaces > sizeof(digest->m_faceIndices) / sizeof(digest->m_faceIndices[0])) {
        digest->m_status = 1;
        return;
    }

    const uint16_t* faceIndices = &bsp.m_nodeFaceIndices[node->m_faceStart];

    for (int32_t i = 0; i < node->m_nFaces; i++) {
        uint16_t face = faceIndices[i];
        digest->m_faceIndices[i] = face;
        const uint16_t* indices = &faceVertexIndices[face * 3];

        for (int32_t j = 0; j < 3; j++) {
            uint16_t vertex = indices[j];
            int32_t index = vertex & 0x3FF;
            int32_t found = -1;

            do {
                if (hash[index] == vertex) {
                    found = index;
                } else if (hash[index] == 0xFFFF) {
                    if (digest->m_nVertices >= sizeof(digest->m_vertices) / sizeof(digest->m_vertices[0])) {
                        digest->m_status = 2;
                        return;
                    }

                    digest->m_vertices[digest->m_nVertices] = vertices[vertex];
                    digest->m_vertexIndices[digest->m_nVertices] = vertex;
                    slots[index] = digest->m_nVertices;
                    hash[index] = vertex;
                    digest->m_nVertices++;
                    found = index;
                } else {
                    index = (index + 1) & 0x3FF;
                }
            } while (found < 0);

            digest->m_faceVertexIndices[digest->m_nFaces * 3 + j] = slots[found];
        }

        digest->m_faceFlags[digest->m_nFaces] = faces[face].m_flags & ~0x80;
        digest->m_nFaces++;
    }
}

// VERIFY: f0141529 auto
// OFFSET: 0x0079B070
CAaBsp::CAaBsp() {
    this->unused_0064 = 0;
    this->m_rootNode = nullptr;
    this->m_nodes = nullptr;
    this->m_nodeFaceIndices = nullptr;
    this->m_nNodes = 0;
    this->m_nNodeFaceIndices = 0;
    this->m_faceVertexIndices = nullptr;
    this->m_nFaceVertexIndices = 0;
    this->m_vertices = nullptr;
    this->m_nVertices = 0;
    this->m_nodeSize = 0;
    this->m_nodeNext = 0;
    this->m_nodeFaceIndicesSize = 0;
    this->m_nodeFaceIndicesNext = 0;
    this->m_buildFaceIndices = nullptr;
    this->m_buildFaceIndicesNext = 0;
    this->m_buildFaceIndicesSize = 0;
    this->m_treeDepth = 0;
    this->m_bFree = 1;
    this->m_avgNodeFaces = 0;
}

// VERIFY: d4f35ea3 manual
// OFFSET: 0x0079B0D0
void CAaBsp::Free() {
    if (g_BspDigestCache) {
        for (uint32_t i = 0; i < this->m_nNodes; i++) {
            if (this->m_nodes[i].m_flags & 0x4) {
                g_BspDigestCache->RemoveEntry(&this->m_nodes[i]);
            }
        }
    }

    if (this->m_bFree) {
        if (this->m_nodes) {
            SMemFree(this->m_nodes, __FILE__, __LINE__, 0);
        }

        if (this->m_nodeFaceIndices) {
            SMemFree(this->m_nodeFaceIndices, __FILE__, __LINE__, 0);
        }
    }

    if (this->m_buildFaceIndices) {
        SMemFree(this->m_buildFaceIndices, __FILE__, __LINE__, 0);
    }
}

// VERIFY: 9ace68d7 manual
// OFFSET: 0x0079B160
CAaBspDigestCache::CAaBspDigestCache() {
    memset(this->m_nodes, 0, sizeof(this->m_nodes));
    memset(this->m_digests, 0, sizeof(this->m_digests));
}

// VERIFY: e8af2097 manual
// OFFSET: 0x0079B1C0
void CAaBspDigestCache::Reset() {
    memset(this->m_nodes, 0, sizeof(this->m_nodes));
    memset(this->m_digests, 0, sizeof(this->m_digests));
}

// VERIFY: e236ae07 manual
// OFFSET: 0x0079B1F0
CAaBspNodeDigest* CAaBspDigestCache::GetDigest(const CAaBsp& bsp, const CAaBspNode* node, SMOPoly* faces, const C3Vector* vertices, const uint16_t* faceVertexIndices) {
    uint32_t bucket = ((reinterpret_cast<uintptr_t>(node) >> 5) ^ node->m_nFaces) & 0x7F;
    int32_t freeSlot = -1;
    int32_t foundSlot = -1;

    for (int32_t i = 0; i < 8; i++) {
        const CAaBspNode* entry = this->m_nodes[bucket * 8 + i];

        if (entry == node) {
            foundSlot = i;
            break;
        }

        if (!entry) {
            freeSlot = i;
            break;
        }
    }

    CAaBspNodeDigest* digest;

    if (foundSlot >= 0) {
        digest = &this->m_digests[bucket * 8 + foundSlot];
    } else {
        if (freeSlot < 0) {
            freeSlot = ++CAaBspDigestCache::s_digestReplaceIndex & 7;
        }

        this->m_nodes[bucket * 8 + freeSlot] = node;
        digest = &this->m_digests[bucket * 8 + freeSlot];
        this->GenerateDigest(digest, bsp, node, faces, vertices, faceVertexIndices);
    }

    return digest->m_status ? nullptr : digest;
}

// VERIFY: d2b6bd78 auto
// OFFSET: 0x0079B2B0
CAaBsp::~CAaBsp() {
    this->Free();
}

// VERIFY: cd56c434 auto
// OFFSET: 0x0079B2C0
void CAaBsp::Clear() {
    this->Free();
    this->m_rootNode = nullptr;
    this->m_nodes = nullptr;
    this->m_nodeFaceIndices = nullptr;
    this->m_nNodes = 0;
    this->m_nNodeFaceIndices = 0;
    this->m_faceVertexIndices = nullptr;
    this->m_nFaceVertexIndices = 0;
    this->m_vertices = nullptr;
    this->m_nVertices = 0;
    this->m_nodeSize = 0;
    this->m_nodeNext = 0;
    this->m_nodeFaceIndicesSize = 0;
    this->m_nodeFaceIndicesNext = 0;
    this->m_buildFaceIndices = nullptr;
    this->m_buildFaceIndicesNext = 0;
    this->m_buildFaceIndicesSize = 0;
    this->m_treeDepth = 0;
    this->m_bFree = 1;
    this->m_avgNodeFaces = 0;
}
