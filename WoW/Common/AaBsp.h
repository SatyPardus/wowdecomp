#ifndef WOW_COMMON_AABSP_H
#define WOW_COMMON_AABSP_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"

class C3Vector;
class CAaBspNode;
struct SMOPoly;

class CAaBsp {
    public:
    /* 0000 */ CAaBspNode* m_rootNode;
    /* 0004 */ CAaBspNode* m_nodes;
    /* 0008 */ uint16_t* m_nodeFaceIndices;
    /* 000C */ uint32_t m_nNodes;
    /* 0010 */ uint32_t m_nNodeFaceIndices;
    /* 0014 */ uint16_t* m_faceVertexIndices;
    /* 0018 */ uint32_t m_nFaceVertexIndices;
    /* 001C */ C3Vector* m_vertices;
    /* 0020 */ uint32_t m_nVertices;
    /* 0024 */ uint32_t m_nodeSize;
    /* 0028 */ uint32_t m_nodeNext;
    /* 002C */ uint32_t m_nodeFaceIndicesSize;
    /* 0030 */ uint32_t m_nodeFaceIndicesNext;
    /* 0034 */ uint16_t* m_buildFaceIndices;
    /* 0038 */ uint32_t m_buildFaceIndicesSize;
    /* 003C */ uint32_t m_buildFaceIndicesNext;
    /* 0040 */ uint32_t m_treeDepth;
    /* 0044 */ uint32_t m_avgNodeFaces;
    /* 0048 */ int32_t m_bFree;
    /* 004C */ CAaBox m_aaBox;
    /* 0064 */ int32_t unused_0064;

    CAaBsp();
    ~CAaBsp();
    void Clear();
    void Free();
    void Set(CAaBspNode* nodes, uint32_t nNodes, uint16_t* nodeFaceIndices, uint32_t nNodeFaceIndices, const CAaBox& aaBox);
};

class CAaBspNodeDigest {
    public:
    /* 0000 */ const CAaBspNode* m_node;
    /* 0004 */ uint8_t m_status;
    /* 0005 */ uint8_t unused_0005[1];
    /* 0006 */ uint16_t m_nVertices;
    /* 0008 */ C3Vector m_vertices [450];
    /* 1520 */ uint16_t m_vertexIndices [450];
    /* 18A4 */ uint16_t m_nFaces;
    /* 18A6 */ uint16_t m_faceVertexIndices [900];
    /* 1FAE */ uint16_t m_faceFlags [300];
    /* 2206 */ uint16_t m_faceIndices [300];
    /* 245E */ uint8_t unused_245E[2];
};

class CAaBspDigestCache {
    public:
    static int32_t s_digestReplaceIndex;

    /* 0000 */ const CAaBspNode* m_nodes [1024];
    /* 1000 */ CAaBspNodeDigest m_digests [1024];

    CAaBspDigestCache();
    void GenerateDigest(CAaBspNodeDigest* digest, const CAaBsp& bsp, const CAaBspNode* node, SMOPoly* faces, const C3Vector* vertices, const uint16_t* faceVertexIndices);
    CAaBspNodeDigest* GetDigest(const CAaBsp& bsp, const CAaBspNode* node, SMOPoly* faces, const C3Vector* vertices, const uint16_t* faceVertexIndices);
    void RemoveEntry(const CAaBspNode* node);
    void Reset();
};

extern CAaBspDigestCache* g_BspDigestCache;

#endif
