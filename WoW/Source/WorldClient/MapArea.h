#ifndef WOW_SOURCE_WORLDCLIENT_MAPAREA_H
#define WOW_SOURCE_WORLDCLIENT_MAPAREA_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/LiquidSystem/Liquid.h"
#include "WoW/Source/WorldClient/MapBaseObj.h"
#include "Engine/Source/Tempest/C2iVector.h"
#include "WoW/Source/WorldClient/World.h"

class CiRect;

class CAsyncObject;
class SFileImpl;
class CMapArea;
class CMapAreaChunkLink;
class CMapBaseObjLink;
class CMapAreaTexture;
class CMapChunk;
struct SMAreaHeader;
struct SMChunkInfo;
struct SMDoodadDef;
struct SMMapObjDef;

class CMapAreaChunkLink {
    public:
    /* 0000 */ uint32_t m_objectIndex;
    /* 0004 */ CMapChunk* m_owner;
    /* 0008 */ CMapArea* m_ref;
    /* 000C */ TSLink<CMapAreaChunkLink> m_refLink;
    /* 0014 */ TSLink<CMapAreaChunkLink> m_ownerLink;
};

class CMapArea : public CMapBaseObj {
    public:
    void QueryLiquidSounds(C3Vector* pos, float radius, TSGrowableArray<World::LiquidQueryResult>& results);
    static void Destroy();
    static void Initialize();
    static void* AllocAsyncLoadBuffer(int32_t fileSize);
    static void AsyncCallback(void* param);
    static void FreeAsyncLoadBuffer(void* buffer, int32_t size);
    static void FreeAsyncObject(CAsyncObject* object);

    /* 0024 */ CAaBox m_bounds;
    /* 003C */ C3Vector m_topLeft2;
    /* 0048 */ C2iVector m_index;
    /* 0050 */ C2iVector m_tileChunkIndex;
    /* 0058 */ TSGrowableArray<CMapAreaTexture> m_textures;
    /* 0068 */ SMAreaHeader* m_header;
    /* 006C */ SFileImpl* m_file;
    /* 0070 */ CAsyncObject* m_asyncObject;
    /* 0074 */ TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > m_chunkLinkList;
    /* 0080 */ void* m_fileBuffer;
    /* 0084 */ int32_t m_fileSize;
    /* 0088 */ SMChunkInfo* m_chunkInfo;
    /* 008C */ int32_t unused_008C;
    /* 0090 */ SMDoodadDef* m_doodadDef;
    /* 0094 */ SMMapObjDef* m_mapObjDef;
    /* 0098 */ int32_t m_doodadDefCount;
    /* 009C */ int32_t m_mapObjDefCount;
    /* 00A0 */ char* m_m2FileNames;
    /* 00A4 */ char* m_wmoFileNames;
    /* 00A8 */ uint32_t* m_modelFilenamesOffsets;
    /* 00AC */ uint32_t* m_wmoFilenamesOffsets;
    /* 00B0 */ int16_t* m_flyingBbox;
    /* 00B4 */ int32_t* m_textureFlags;
    /* 00B8 */ Liquid::CAreaData* m_liquid;
    /* 00BC */ CMapChunk* m_mapChunks [256];

    CMapArea();
    ~CMapArea() override;
    void Load(const char* fileName);
    void LoadTextures(char* fileNames, uint32_t fileNamesSize);
    void LoadTerrainTexture(CMapAreaTexture* areaTexture, int32_t textureId);
    void ProcessIffChunks();
    CMapChunk* PrepareChunk(int32_t chunkX, int32_t chunkY);
    void PurgeChunks(CiRect* chunkRect);
    static void PurgeChunk(CMapChunk* chunk);
    void Purge();
    void Update(int32_t addToScene, CiRect* chunkRect);
    void BatchChunks(const C2iVector& pos);
};

struct SMAreaHeader {
    /* 0000 */ uint32_t m_flags;
    /* 0004 */ uint32_t m_mcin;
    /* 0008 */ uint32_t m_mtex;
    /* 000C */ uint32_t m_mmdx;
    /* 0010 */ uint32_t m_mmid;
    /* 0014 */ uint32_t m_mwmo;
    /* 0018 */ uint32_t m_mwid;
    /* 001C */ uint32_t m_mddf;
    /* 0020 */ uint32_t m_modf;
    /* 0024 */ uint32_t m_mfbo;
    /* 0028 */ uint32_t m_mh2o;
    /* 002C */ uint32_t m_mtxf;
    /* 0030 */ uint8_t m_mampValue;
    /* 0031 */ uint8_t m_padding [3];
    /* 0034 */ uint32_t m_unused [3];
};

struct SMChunkInfo {
    /* 0000 */ uint32_t m_offset;
    /* 0004 */ uint32_t m_size;
    /* 0008 */ uint32_t m_flags;
    /* 000C */ uint8_t unused_000C[4];
};

class CMapAreaMed : public CMapBaseObj {
    public:
    /* 0024 */ uint8_t unused_0024[33368];
};

#endif
