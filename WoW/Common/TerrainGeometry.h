#pragma once
#include <cstdint>
#include "Engine/Source/Tempest/C2iVector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"

class CGxVertexPN {
    public:
    /* 0000 */ C3Vector m_position;
    /* 000C */ C3Vector m_normal;
};

class CGxVertexPNC {
    public:
    /* 0000 */ C3Vector m_position;
    /* 000C */ C3Vector m_normal;
    /* 0018 */ CImVector m_color;
};

namespace Terrain {
// Views into validated/aligned chunk arrays; one chunk has 145 vertices.
struct GeometryView {
    C2iVector chunkOffset;
    float baseHeight;
    const float* heights;
    const int8_t* normals;
    const uint32_t* colors;
    bool swapRedBlue;
};
void CreateVerticesWorld(CGxVertexPN* vertices, const GeometryView& geometry);
void CreateVerticesWorld(CGxVertexPNC* vertices, const GeometryView& geometry);
uint16_t CreateIndices(uint16_t* indices, uint16_t holes, int32_t baseVertex = 0);
}
