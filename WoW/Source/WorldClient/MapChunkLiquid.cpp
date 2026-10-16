#include "MapChunkLiquid.h"
#include <cmath>
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/LiquidSystem/Liquid.h"
#include "WoW/Source/WorldClient/CChunkLiquid.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/LiquidEnvironment.h"
#include "WoW/Source/WorldClient/LiquidGeomFactories.h"
#include "WoW/Source/WorldClient/LiquidWaveManager.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapArea.h"
#include "WoW/Source/WorldClient/CMapBaseObjLink.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CFrustum.h"
#include "Engine/Source/Tempest/CiRect.h"

// STATIC: 0x00AF0A0C
TSList<CMapChunkLiquid, TSGetExplicitLink<CMapChunkLiquid> > CMapChunkLiquidBuf::s_freeLists = TSExplicitList<CMapChunkLiquid, offsetof(CMapChunkLiquid, m_link)>();

// STATIC: 0x00D2DD2C
Liquid::CWaveManager* CChunkLiquid::s_waveManager;

// STATIC: 0x00D2DD30
int32_t CChunkLiquid::s_waveManagerRefs;

// VERIFY: da69d24e auto
// OFFSET: 0x007CDE10
void CChunkLiquid::Purge() {
    if (this->m_instance) {
        this->m_instance->Release();
        this->m_instance = nullptr;
    }
}

// VERIFY: 0d06cb60 auto
// OFFSET: 0x007CDE30
void CChunkLiquid::UpdateResourcePurging() {
    if (this->m_resPurgeTimer >= 2.0f) {
        if (this->m_instance) {
            this->m_instance->Purge();
        }

        this->m_resPurgeTimer = -1.0f;
    }

    if (this->m_resPurgeTimer >= 0.0f) {
        this->m_resPurgeTimer = FrameTime::s_tickTimeSec + this->m_resPurgeTimer;
    }
}

// VERIFY: a8377593 manual
// OFFSET: 0x007CDE80
void CChunkLiquid::GetAaBox(CAaBox& box) const {
    box = this->m_owner->m_bbox;
    box.b.z = this->m_height.x;
    box.t.z = this->m_height.y;
}

// VERIFY: ab9c5bab auto
// OFFSET: 0x007CDF50
bool CMapChunkLiquidBuf::Valid() const {
    return this->m_vertexBuf && this->m_vertexBuf->m_hasData && this->m_vertexBuf->m_valid && this->m_indexBuf && this->m_indexBuf->m_hasData && this->m_indexBuf->m_valid;
}

// VERIFY: 8f8e7321 manual
// OFFSET: 0x007CDF80
void CChunkLiquid::CreateVertXY() {
    CMapChunk* owner = this->m_owner;
    int32_t chunkX = owner->m_cOffset.x;
    int32_t chunkY = owner->m_cOffset.y;
    int32_t beginY = this->m_tileBegin.y;
    int32_t rows = this->m_tileEnd.x - this->m_tileBegin.x + 1;
    int32_t beginX = this->m_tileBegin.x;
    int32_t cols = this->m_tileEnd.y - beginY + 1;

    C3Vector* vert = this->m_verts;
    uint32_t index = 0;

    float stepY = -(17066.666f - chunkX * 33.333332f - (17066.666f - (chunkX + 1) * 33.333332f)) * 0.125f;

    for (int32_t row = 0; row < rows; row++) {
        float stepX = 0.125f * -(17066.666f - chunkY * 33.333332f - (17066.666f - (chunkY + 1) * 33.333332f));
        float x = (row + beginX) * stepX;

        for (int32_t col = 0; col < cols; col++) {
            vert->x = x;
            vert->y = (col + beginY) * stepY;
            vert->z = this->m_vertexData->GetHeight(index++) - this->m_owner->m_topLeftCoords.z;
            vert++;
        }
    }
}

// VERIFY: 6dc3ffdb manual
// OFFSET: 0x007CE0B0
bool CChunkLiquid::GetHeight(const C2Vector& frac, const C2iVector& tile, float& height) const {
    if (tile.x < this->m_tileBegin.y || tile.y < this->m_tileBegin.x || tile.x > this->m_tileEnd.y || tile.y > this->m_tileEnd.x) {
        return false;
    }

    int32_t cols = this->m_tileEnd.y - this->m_tileBegin.y + 1;
    uint32_t index = (tile.x - this->m_tileBegin.y) + cols * (tile.y - this->m_tileBegin.x);
    uint32_t next = index + cols;

    float h00 = this->m_vertexData->GetHeight(index);
    float h01 = this->m_vertexData->GetHeight(index + 1);
    float h10 = this->m_vertexData->GetHeight(next);
    float h11 = this->m_vertexData->GetHeight(next + 1);

    float top = h00 + (h01 - h00) * frac.x;
    height = top + (h10 + (h11 - h10) * frac.x - top) * frac.y;

    return true;
}

// VERIFY: cf242b03 manual
// OFFSET: 0x007CE180
bool CChunkLiquid::GetTileInfo(const C2iVector& tile, int32_t* type, int32_t* flag6, int32_t* flag7) const {
    if (!this->m_tiles) {
        return false;
    }

    const uint8_t* entry = &this->m_tiles[tile.x + 8 * tile.y];
    *type = *entry & 0xF;
    *flag6 = (*entry >> 6) & 1;
    *flag7 = *entry >> 7;

    return *type != 15 && ((*type ^ (this->m_type - 1)) & 3) == 0;
}

// VERIFY: b3cebac0 auto
// OFFSET: 0x007CE1F0
bool CChunkLiquid::TileExists(uint32_t x, uint32_t y) const {
    if (this->m_tiles) {
        C2iVector tile;
        tile.x = x;
        tile.y = y;

        int32_t type;
        int32_t flag6;
        int32_t flag7;

        return this->GetTileInfo(tile, &type, &flag6, &flag7);
    }

    if (x < static_cast<uint32_t>(this->m_tileBegin.y)) {
        return false;
    }

    return y >= static_cast<uint32_t>(this->m_tileBegin.x) && x < static_cast<uint32_t>(this->m_tileEnd.y) && y < static_cast<uint32_t>(this->m_tileEnd.x) && this->m_tileBits.Get(x + (this->m_tileEnd.y - this->m_tileBegin.y) * (y - this->m_tileBegin.x) - this->m_tileBegin.y);
}

// VERIFY: 51ae5fe3 manual
// OFFSET: 0x007CE270
void CChunkLiquid::BuildIndices(uint16_t* indices, uint16_t baseVertex, CGxBatch* batch) const {
    uint16_t* out = indices;
    uint16_t vertex = baseVertex;
    uint16_t below = baseVertex + this->m_tileEnd.y - this->m_tileBegin.y + 1;
    uint16_t last = 0;
    int32_t inStrip = 0;

    for (int32_t row = this->m_tileBegin.x; row < this->m_tileEnd.x; row++) {
        uint16_t belowNext = below + 1;

        for (int32_t col = this->m_tileBegin.y; col < this->m_tileEnd.y; col++) {
            if (this->TileExists(col, row)) {
                if (!inStrip) {
                    *out++ = vertex;
                    *out++ = vertex;
                    *out++ = below;
                    inStrip = 1;
                }

                *out++ = vertex + 1;
                last = belowNext;
                *out++ = belowNext;
            } else if (inStrip) {
                inStrip = 0;
                *out++ = last;
            }

            below++;
            belowNext++;
            vertex++;
        }

        if (inStrip) {
            *out++ = last;
            inStrip = 0;
        }

        below++;
        vertex++;
    }

    batch->m_count += out - indices;
}

// VERIFY: a801c800 manual
// OFFSET: 0x007CE390
void CChunkLiquid::BuildVertices(const C44Matrix* transform, uint32_t stride, C3Vector** position, C3Vector** normal, uint32_t** color, C2Vector** texCoord1, C2Vector** texCoord0) const {
    DepthCoordTable* depthCoords = CMap::GetDepthCoordTable(this->m_type);
    bool customTexCoords = this->m_vertexFormat == 1;
    uint32_t index = 0;
    int32_t rows = this->m_tileEnd.x - this->m_tileBegin.x + 1;
    const C3Vector* vert = this->m_verts;
    int32_t cols = this->m_tileEnd.y - this->m_tileBegin.y + 1;

    for (int32_t row = 0; row < rows; row++) {
        for (int32_t col = 0; col < cols; col++) {
            C3Vector pos = *vert * *transform;

            if (*position) {
                **position = pos;
                *position = reinterpret_cast<C3Vector*>(reinterpret_cast<uint8_t*>(*position) + stride);
            }

            if (*normal) {
                (*normal)->x = 0.0f;
                (*normal)->y = 0.0f;
                (*normal)->z = 1.0f;
                *normal = reinterpret_cast<C3Vector*>(reinterpret_cast<uint8_t*>(*normal) + stride);
            }

            if (*color) {
                **color = 0xFFFFFFFF;
                *color = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(*color) + stride);
            }

            if (*texCoord1) {
                if (depthCoords) {
                    uint8_t depth = this->m_vertexData->GetDepth(index);
                    (*texCoord1)->x = 0.0f;
                    (*texCoord1)->y = (*depthCoords)[depth];
                } else {
                    (*texCoord1)->x = 0.0f;
                    (*texCoord1)->y = 0.0f;
                }

                *texCoord1 = reinterpret_cast<C2Vector*>(reinterpret_cast<uint8_t*>(*texCoord1) + stride);
            }

            if (*texCoord0) {
                if (customTexCoords) {
                    const uint16_t* uv = this->m_vertexData->GetTexCoord(index);
                    (*texCoord0)->x = 0.01171875f * uv[0];
                    (*texCoord0)->y = uv[1] * 0.01171875f;
                } else {
                    (*texCoord0)->x = 0.060000002f * pos.x;
                    (*texCoord0)->y = pos.y * 0.060000002f;
                }

                *texCoord0 = reinterpret_cast<C2Vector*>(reinterpret_cast<uint8_t*>(*texCoord0) + stride);
            }

            vert++;
            index++;
        }
    }
}

// VERIFY: ef77a0e3 manual
// OFFSET: 0x007CE520
CAaSphere* CChunkLiquid::GetSphere() {
    if (this->m_sphere.r < 0.0f) {
        CAaBox& bbox = this->m_owner->m_bbox;
        float top = this->m_height.y;

        this->m_sphere.c = C3Vector((bbox.b.x + bbox.t.x) * 0.5f, (bbox.b.y + bbox.t.y) * 0.5f, 0.5f * (this->m_height.x + top));

        float dy = bbox.t.y - this->m_sphere.c.y;
        float dx = bbox.t.x - this->m_sphere.c.x;
        this->m_sphere.r = sqrtf(dx * dx + dy * dy + (top - this->m_sphere.c.z) * (top - this->m_sphere.c.z));
    }

    return &this->m_sphere;
}

// INLINED
CMapChunkLiquidBuf::CMapChunkLiquidBuf() {
    this->m_flags = 0;
    this->m_flushTimer = 0.0f;
    this->m_vertexDataSize = 0;
    this->m_indexDataSize = 0;
    this->m_vertexBuf = nullptr;
    this->m_indexBuf = nullptr;
    this->m_vertexPool = nullptr;
    this->m_indexPool = nullptr;
}

// VERIFY: 1c787e83 manual
// OFFSET: 0x007CED30
CMapChunkLiquidBuf::~CMapChunkLiquidBuf() {
    if (this->m_vertexBuf) {
        CGxBuf* buf = this->m_vertexBuf;

        if (buf != g_theGxDevicePtr->BufStream(buf->m_pool->m_target, 0, 0)) {
            g_theGxDevicePtr->BufDestroy(buf);
        }
    }

    if (this->m_indexBuf) {
        CGxBuf* buf = this->m_indexBuf;

        if (buf != g_theGxDevicePtr->BufStream(buf->m_pool->m_target, 0, 0)) {
            g_theGxDevicePtr->BufDestroy(buf);
        }
    }

    if (this->m_vertexPool) {
        g_theGxDevicePtr->PoolDestroy(this->m_vertexPool);
    }

    if (this->m_indexPool) {
        g_theGxDevicePtr->PoolDestroy(this->m_indexPool);
    }
}

// VERIFY: 61df3203 manual
// OFFSET: 0x007CEE10
CChunkLiquid::CChunkLiquid() {
    this->m_tileBegin.x = 0;
    this->m_tileBegin.y = 0;
    this->m_tileEnd.x = 0;
    this->m_tileEnd.y = 0;
    this->m_resPurgeTimer = 0.0f;
    this->m_type = 0;
    this->m_sphere.r = -1.0f;
    this->m_vertexData = nullptr;
    this->m_tiles = nullptr;
    this->m_instance = nullptr;
    this->m_owner = nullptr;

    if (CChunkLiquid::s_waveManagerRefs == 0) {
        Liquid::CWaveManager* waveManager = Liquid::CWaveManager::Create();
        CChunkLiquid::s_waveManagerRefs++;
        CChunkLiquid::s_waveManager = waveManager;
    } else {
        CChunkLiquid::s_waveManager->AddRef();
        CChunkLiquid::s_waveManagerRefs++;
    }
}

// VERIFY: 10ec5edc manual
// OFFSET: 0x007CEEF0
CChunkLiquid::~CChunkLiquid() {
    if (CChunkLiquid::s_waveManagerRefs) {
        CChunkLiquid::s_waveManager->Release();
        CChunkLiquid::s_waveManagerRefs--;
    }

    if (this->m_vertexData) {
        delete this->m_vertexData;
        this->m_vertexData = nullptr;
    }
}

// VERIFY: 86f2eb61 manual
// OFFSET: 0x007CEFD0
CMapChunkLiquidBuf* CMapChunkLiquidBuf::CreateBuf(uint32_t vertexCount, uint32_t indexCount, int32_t flags, uint32_t vertexSize) {
    void* mem = SMemAlloc(sizeof(CMapChunkLiquidBuf), __FILE__, __LINE__, 0);

    if (!mem) {
        return nullptr;
    }

    CMapChunkLiquidBuf* buf = new (mem) CMapChunkLiquidBuf();

    CGxPool* vertexPool = g_theGxDevicePtr->PoolCreate(GxPoolTarget_Vertex, GxPoolUsage_Dynamic, vertexSize * vertexCount, GxPoolHintBit_Unk3, "CChunkBuf_Vertex");
    CGxPool* indexPool = g_theGxDevicePtr->PoolCreate(GxPoolTarget_Index, GxPoolUsage_Dynamic, 2 * indexCount, GxPoolHintBit_Unk3, "CChunkBuf_Index");
    CGxBuf* vertexBuf = g_theGxDevicePtr->BufCreate(vertexPool, vertexSize, vertexCount, 0);
    CGxBuf* indexBuf = g_theGxDevicePtr->BufCreate(indexPool, 2, indexCount, 0);

    buf->m_flushTimer = 0.0f;
    buf->m_indexDataSize = 2 * indexCount;
    buf->m_flags = flags;
    buf->m_vertexPool = vertexPool;
    buf->m_vertexDataSize = vertexSize * vertexCount;
    buf->m_indexBuf = indexBuf;
    buf->m_indexPool = indexPool;
    buf->m_vertexBuf = vertexBuf;

    return buf;
}

// INLINED
CMapChunkLiquid::CMapChunkLiquid() {
}

// VERIFY: 3a225d8f manual
// OFFSET: 0x007CF0C0
CMapChunkLiquid::~CMapChunkLiquid() {
}

// VERIFY: ac9225a1 manual
// OFFSET: 0x007CF140
CMapChunkLiquidBuf* CMapChunkLiquidBuf::Alloc(uint32_t vertexCount, uint32_t indexCount, int32_t flags, EGxVertexBufferFormat format) {
    uint32_t vertexSize = GxVertexSize(format);

    for (CMapChunkLiquid* list = CMapChunkLiquidBuf::s_freeLists.Head(); list; list = CMapChunkLiquidBuf::s_freeLists.Next(list)) {
        if (list->m_vertexDataSize == vertexCount * vertexSize && list->m_indexDataSize == 2 * indexCount && list->m_liquids.Head()) {
            CMapChunkLiquidBuf* buf = list->m_liquids.Head();
            buf->m_link.Unlink();
            buf->m_vertexBuf->m_hasData = 0;
            buf->m_indexBuf->m_hasData = 0;

            return buf;
        }
    }

    return CMapChunkLiquidBuf::CreateBuf(vertexCount, indexCount, flags, vertexSize);
}

// VERIFY: e344dd13 manual
// OFFSET: 0x007CF200
void CChunkLiquid::Batch() {
    static TSGrowableArray<CChunkLiquid*> s_batch;

    s_batch.Clear();
    s_batch.ReallocData(32);

    if (CMap::enableChunkBatching) {
        CMapArea* area = static_cast<CMapArea*>(this->m_owner->m_parentLinkList.Head()->m_ref);
        CMapChunk** row = &area->m_mapChunks[16 * (this->m_owner->m_aIndex.y & ~1) + (this->m_owner->m_aIndex.x & ~1)];

        for (int32_t y = 0; y < 2; y++) {
            CMapChunk** slot = row;

            for (int32_t x = 0; x < 2; x++) {
                CMapChunk* chunk = *slot;

                for (CChunkLiquid* liquid = chunk->m_liquidChunkLinkList.Head(); liquid; liquid = chunk->m_liquidChunkLinkList.Next(liquid)) {
                    if (liquid->m_type == this->m_type) {
                        s_batch.New(liquid);
                    }
                }

                slot++;
            }

            row += 16;
        }
    } else {
        CChunkLiquid* self = this;
        s_batch.New(self);
    }

    CAaBox bounds;
    Liquid::CChunkGeomFactory* factory = Liquid::CChunkGeomFactory::Create(s_batch);

    for (uint32_t i = 0; i < s_batch.m_count; i++) {
        CChunkLiquid* liquid = s_batch.m_data[i];

        C2Vector minXY = C2Vector(liquid->m_topLeftCoords);
        minXY.x = minXY.x + liquid->m_tileBegin.y * 4.1666665f;
        minXY.y = minXY.y + 4.1666665f * liquid->m_tileBegin.x;
        C2Vector maxXY = C2Vector(liquid->m_topLeftCoords);
        maxXY.x = maxXY.x + liquid->m_tileEnd.y * 4.1666665f;
        maxXY.y = maxXY.y + 4.1666665f * liquid->m_tileEnd.x;

        CAaBox box(C3Vector(minXY.x, minXY.y, liquid->m_height.x), C3Vector(maxXY.x, maxXY.y, liquid->m_height.y));

        if (i) {
            bounds.t.z = bounds.t.z > box.t.z ? bounds.t.z : box.t.z;
            bounds.t.y = bounds.t.y > box.t.y ? bounds.t.y : box.t.y;
            bounds.t.x = bounds.t.x > box.t.x ? bounds.t.x : box.t.x;
            bounds.b.z = bounds.b.z < box.b.z ? bounds.b.z : box.b.z;
            bounds.b.y = bounds.b.y < box.b.y ? bounds.b.y : box.b.y;
            bounds.b.x = bounds.b.x < box.b.x ? bounds.b.x : box.b.x;
        } else {
            bounds = box;
        }
    }

    C44Matrix transform;
    transform.Translate(s_batch.m_data[0]->m_topLeftCoords);

    Liquid::CInstance* instance = Liquid::CInstance::Create();
    instance->m_material = Liquid::CMaterialBank::GetMaterial(this->m_type);
    instance->m_settings = Liquid::CSettingsBank::GetSettings(this->m_type);
    Liquid::CClientEnvironment* environment = Liquid::CClientEnvironment::Create(0);
    instance->m_geomFactory = factory;
    instance->m_environment = environment;
    instance->m_waveManager = CChunkLiquid::s_waveManager;
    CChunkLiquid::s_waveManager->AddRef();
    instance->m_transform = transform;

    float dx = bounds.t.x - bounds.b.x;
    float dy = bounds.t.y - bounds.b.y;
    float dz = bounds.t.z - bounds.b.z;
    instance->m_bounds.c.x = (bounds.b.x + bounds.t.x) * 0.5f;
    instance->m_bounds.c.y = (bounds.t.y + bounds.b.y) * 0.5f;
    instance->m_bounds.c.z = (bounds.b.z + bounds.t.z) * 0.5f;
    instance->m_bounds.r = 0.5f * sqrtf(dy * dy + dx * dx + dz * dz);

    for (uint32_t i = 0; i < s_batch.m_count; i++) {
        s_batch.m_data[i]->m_instance = instance;
        instance->AddRef();
    }

    instance->Release();
}

// VERIFY: 0f8f770e manual
// OFFSET: 0x007CF790
void CMapChunkLiquidBuf::Free(CMapChunkLiquidBuf* buf) {
    CMapChunkLiquid* list;

    for (list = CMapChunkLiquidBuf::s_freeLists.Head(); list; list = CMapChunkLiquidBuf::s_freeLists.Next(list)) {
        if (list->m_vertexDataSize == buf->m_vertexDataSize && list->m_indexDataSize == buf->m_indexDataSize) {
            break;
        }
    }

    if (!list) {
        void* mem = SMemAlloc(sizeof(CMapChunkLiquid), __FILE__, __LINE__, 0);
        list = mem ? new (mem) CMapChunkLiquid() : nullptr;
        list->m_vertexDataSize = buf->m_vertexDataSize;
        list->m_indexDataSize = buf->m_indexDataSize;
        CMapChunkLiquidBuf::s_freeLists.LinkToTail(list);
    }

    list->m_liquids.LinkToTail(buf);
}

// VERIFY: 9b76b6fd manual
// OFFSET: 0x007CF840
void CMapChunkLiquidBuf::Update(float elapsed) {
    CMapChunkLiquid* list = CMapChunkLiquidBuf::s_freeLists.Head();

    while (list) {
        CMapChunkLiquid* nextList = CMapChunkLiquidBuf::s_freeLists.Next(list);

        CMapChunkLiquidBuf* buf = list->m_liquids.Head();

        while (buf) {
            CMapChunkLiquidBuf* next = list->m_liquids.Next(buf);

            if (!(buf->m_flags & 0x1)) {
                buf->m_flushTimer += elapsed;

                if (buf->m_flushTimer > 5.0f) {
                    buf->m_link.Unlink();
                    delete buf;
                }
            }

            buf = next;
        }

        if (!list->m_liquids.Head()) {
            list->m_link.Unlink();
            delete list;
        }

        list = nextList;
    }
}

// VERIFY: 3c1420a2 auto
// OFFSET: 0x007CF9A0
void CChunkLiquid::RenderPrep() {
    if (!this->m_instance) {
        this->Batch();
    }

    if (this->m_instance && this->m_instance->m_waveManager) {
        static_cast<Liquid::CWaveManager*>(this->m_instance->m_waveManager)->PrepareRenderTransform(World::GetCamPos());
    }

    this->m_resPurgeTimer = 0.0f;
}

// VERIFY: 87f4391a manual
// OFFSET: 0x007CF9E0
void CMap::FreeAllChunkLiquids() {
    CMapChunkLiquid* list = CMapChunkLiquidBuf::s_freeLists.Head();

    while (list) {
        CMapChunkLiquid* nextList = CMapChunkLiquidBuf::s_freeLists.Next(list);
        CMapChunkLiquidBuf* buf = list->m_liquids.Head();

        while (buf) {
            CMapChunkLiquidBuf* next = list->m_liquids.Next(buf);
            buf->m_link.Unlink();
            delete buf;
            buf = next;
        }

        list->m_link.Unlink();
        delete list;
        list = nextList;
    }
}

// VERIFY: 0b0a7aa2 manual
// OFFSET: 0x007CE5D0
bool CChunkLiquid::GetTris(World::TriData* triData, uint8_t* codes, const C2iVector& start, const C2iVector& size) const {
    int32_t stride = this->m_tileEnd.y - this->m_tileBegin.y + 1;
    int32_t vertTris[6];
    vertTris[0] = 0;
    vertTris[1] = stride;
    vertTris[2] = stride + 1;
    vertTris[3] = 0;
    vertTris[4] = stride + 1;
    vertTris[5] = 1;
    int32_t codeTris[6];
    codeTris[0] = 0;
    codeTris[1] = size.x + 1;
    codeTris[2] = size.x + 2;
    codeTris[3] = 0;
    codeTris[4] = size.x + 2;
    codeTris[5] = 1;
    int32_t vertex = (start.y - this->m_tileBegin.x) * stride - this->m_tileBegin.y + start.x;
    World::TriData::Batch* batch = nullptr;
    uint16_t* indices = nullptr;

    for (int32_t y = 0; y < size.y; y++) {
        for (int32_t x = 0; x < size.x; x++) {
            if (this->TileExists(start.x + x, start.y + y)) {
                for (uint32_t j = 0; j < 2; j++) {
                    const int32_t* codeTri = &codeTris[3 * j];

                    if (codes[codeTri[0]] & codes[codeTri[2]] & codes[codeTri[1]]) {
                        continue;
                    }

                    if (!batch) {
                        batch = triData->AllocBatch();

                        if (!batch) {
                            return false;
                        }

                        batch->m_def = reinterpret_cast<CMapObjDef*>(this->m_owner);
                        C44Matrix* matrix;

                        if (World::TriData::matrixPoolCount + 1 >= 32) {
                            World::TriData::statusFlags |= 1;
                            matrix = nullptr;
                        } else {
                            matrix = &World::TriData::matrixPool[World::TriData::matrixPoolCount++];
                        }

                        *matrix = C44Matrix();
                        matrix->d0 = this->m_owner->m_topLeftCoords.x;
                        matrix->d1 = this->m_owner->m_topLeftCoords.y;
                        matrix->d2 = this->m_owner->m_topLeftCoords.z;
                        batch->m_matrix = matrix;
                        batch->m_vertexList = const_cast<C3Vector*>(this->m_verts);
                        indices = triData->AllocIndices(6 * size.x * size.y);
                        batch->m_indices = indices;

                        if (!indices) {
                            return false;
                        }
                    }

                    const int32_t* vertTri = &vertTris[3 * j];

                    for (uint32_t k = 0; k < 3; k++) {
                        uint16_t index = static_cast<uint16_t>(vertex + vertTri[k]);
                        indices[batch->m_indexCount++] = index;
                        batch->m_minVertexIndex = batch->m_minVertexIndex < index ? batch->m_minVertexIndex : index;
                        batch->m_maxVertexIndex = batch->m_maxVertexIndex > index ? batch->m_maxVertexIndex : index;
                    }

                    batch->m_faceCount++;
                }
            }

            vertex++;
            codes++;
        }

        codes++;
        vertex += stride - size.x;
    }

    return batch != nullptr;
}

// VERIFY: 9cf4f78e manual
// OFFSET: 0x007CE960
bool CChunkLiquid::GetTris(World::TriData* triData, const CAaBox& box, const CiRect& rect) {
    CiRect query;
    query.minY = rect.minY;
    query.minX = rect.minX;
    query.maxY = rect.maxY + 1;
    query.maxX = rect.maxX + 1;
    CiRect clip = reinterpret_cast<const CiRect&>(this->m_tileBegin) & query;
    C2iVector size;
    size.x = clip.maxX - clip.minX;
    size.y = clip.maxY - clip.minY;
    int32_t stride = this->m_tileEnd.y - this->m_tileBegin.y + 1;
    C2iVector start;
    start.x = clip.minX;
    start.y = clip.minY;
    const C3Vector* vert = &this->m_verts[(clip.minY - this->m_tileBegin.x) * stride - this->m_tileBegin.y + clip.minX];
    uint8_t codes[81];
    uint8_t* code = codes;

    for (int32_t y = 0; y < size.y + 1; y++) {
        for (int32_t x = 0; x < size.x + 1; x++) {
            float below = vert->z - box.b.z + 0.019444443f;
            float above = box.t.z - vert->z + 0.019444443f;
            *code = static_cast<uint8_t>(((reinterpret_cast<uint32_t&>(below) >> 29) & 0x4) | ((reinterpret_cast<uint32_t&>(above) >> 26) & 0x20));
            vert++;
            code++;
        }

        vert += stride - (size.x + 1);
    }

    return this->GetTris(triData, codes, start, size);
}

// VERIFY: 22a3788a manual
// OFFSET: 0x007CEB80
bool CChunkLiquid::GetTris(World::TriData* triData, const CFrustum& frustum, const CiRect& rect) const {
    CiRect query;
    query.minY = rect.minY;
    query.minX = rect.minX;
    query.maxY = rect.maxY + 1;
    query.maxX = rect.maxX + 1;
    CiRect clip = reinterpret_cast<const CiRect&>(this->m_tileBegin) & query;
    C2iVector size;
    size.x = clip.maxX - clip.minX;
    size.y = clip.maxY - clip.minY;
    int32_t stride = this->m_tileEnd.y - this->m_tileBegin.y + 1;
    C2iVector start;
    start.x = clip.minX;
    start.y = clip.minY;
    const C3Vector* vert = &this->m_verts[(clip.minY - this->m_tileBegin.x) * stride - this->m_tileBegin.y + clip.minX];
    uint8_t codes[81];
    uint8_t* code = codes;

    for (int32_t y = 0; y < size.y + 1; y++) {
        for (int32_t x = 0; x < size.x + 1; x++) {
            frustum.Cull(*vert, *code);
            vert++;
            code++;
        }

        vert += stride - (size.x + 1);
    }

    return this->GetTris(triData, codes, start, size);
}
