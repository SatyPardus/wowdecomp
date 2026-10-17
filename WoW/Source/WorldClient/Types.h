#ifndef WOW_SOURCE_WORLDCLIENT_TYPES_H
#define WOW_SOURCE_WORLDCLIENT_TYPES_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Storm/H/stpl.h"

class CMapChunk;
class CMapChunkDoodadDefLink;
class CMapDoodadDef;
class CMapObjDef;
class CMapObjDefGroup;
class CMapObjDefMapObjDefGroupLink;
class CTexture;

class CMapAreaTexture {
    public:
    /* 0000 */ char* m_textureName;
    /* 0004 */ CTexture* m_texture;
};

class CMapChunkDoodadDefLink {
    public:
    /* 0000 */ uint32_t m_objectIndex;
    /* 0004 */ CMapDoodadDef* m_owner;
    /* 0008 */ CMapChunk* m_ref;
    /* 000C */ TSLink<CMapChunkDoodadDefLink> m_refLink;
    /* 0014 */ TSLink<CMapChunkDoodadDefLink> m_ownerLink;
};

class CMapObjDefMapObjDefGroupLink {
    public:
    /* 0000 */ uint32_t m_objectIndex;
    /* 0004 */ CMapObjDefGroup* m_owner;
    /* 0008 */ CMapObjDef* m_ref;
    /* 000C */ TSLink<CMapObjDefMapObjDefGroupLink> m_refLink;
    /* 0014 */ TSLink<CMapObjDefMapObjDefGroupLink> m_ownerLink;
};

enum Enables : uint32_t {
    Enable_Doodads = 1,
    Enable_Chunks = 2,
    Enable_Lod = 4,
    Enable_Texture = 8,
    Enable_Sky = 16,
    Enable_Culling = 32,
    Enable_Shadow = 64,
    Enable_Collision = 128,
    Enable_MapObjs = 0x100,
    Enable_MapObjLight = 0x200,
    Enable_VertexLight = 0x400,
    Enable_MapObjTex = 0x800,
    Enable_Portals = 0x1000,
    Enable_PortalVis = 0x2000,
    Enable_NoFullAlpha = 0x4000,
    Enable_NoAnimation = 0x8000,
    Enable_DebugBSP = 0x10000,
    Enable_CrappyBatches = 0x20000,
    Enable_ZoneBounds = 0x40000,
    Enable_MapObjBSP = 0x80000,
    Enable_DetailDoodads = 0x100000,
    Enable_ShowQuery = 0x200000,
    Enable_AABoxes = 0x400000,
    Enable_Trilinear = 0x800000,
    Enable_Water = 0x1000000,
    Enable_Particulates = 0x2000000,
    Enable_LowDetail = 0x4000000,
    Enable_Specular = 0x8000000,
    Enable_PixelShaders = 0x10000000,
    Enable_ShowTris = 0x20000000,
    Enable_ShowNormals = 0x40000000,
    Enable_Anisotropic = 0x80000000u,
};

enum Enables2 {
    Enable_VertexShader = 1,
    Enable_HwPcf = 2,
};

enum MapObjFlag {
    MAPOBJ_FLAG_UNPLACED = 1,
    MAPOBJ_FLAG_INTERIOR = 2,
    MAPOBJ_FLAG_EXTERIOR = 4,
    MAPOBJ_FLAG_REFS_CREATED = 8,
    MAPOBJ_FLAG_GROUP_INIT = 16,
    MAPOBJ_FLAG_DISABLED = 32,
    MAPOBJ_FLAG_IMPASSABLE = 64,
    MAPOBJ_FLAG_PREPARED = 128,
    MAPOBJ_FLAG_NO_HITTEST = 0x100,
    MAPOBJ_FLAG_IN_MAPOBJ = 0x200,
    MAPOBJ_FLAG_NO_DEPTH_SORT = 0x400,
    MAPOBJ_FLAG_BIODOME = 0x800,
    MAPOBJ_FLAG_PROJ_TEX = 0x1000,
    MAPOBJ_FLAG_NO_MAPOBJ_LINK = 0x2000,
    MAPOBJ_FLAG_INT_FOG = 0x8000,
    MAPOBJ_FLAG_MAPOBJDEF = 0x10000,
    MAPOBJ_FLAG_SHADOW_20000 = 0x20000,
};

struct SIffChunk {
    /* 0000 */ uint32_t m_token;
    /* 0004 */ uint32_t m_size;
    /* 0008 */ void* m_data;
};

struct SMLayer {
    /* 0000 */ uint32_t m_textureId;
    /* 0004 */ uint32_t m_flags;
    /* 0008 */ uint32_t m_offsetInMCAL;
    /* 000C */ uint32_t m_offectId;
};

struct SMOPoly {
    /* 0000 */ uint8_t m_flags;
    /* 0001 */ uint8_t m_materialId;
};

struct SMOPortalRef {
    /* 0000 */ uint16_t m_portalIndex;
    /* 0002 */ uint16_t m_groupIndex;
    /* 0004 */ int16_t m_side;
    /* 0006 */ uint16_t m_filler;
};

typedef float DepthCoordTable[256];

#endif
