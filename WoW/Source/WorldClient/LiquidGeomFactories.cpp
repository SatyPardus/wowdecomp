#include "LiquidGeomFactories.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/WorldClient/CChunkLiquid.h"
#include "WoW/Source/LiquidSystem/Liquid.h"
#include "WoW/Source/WorldClient/MapChunkLiquid.h"
#include "WoW/Source/WorldClient/MapObj.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"

// STATIC: 0x00D31960
CDataAllocator Liquid::s_meshGeomFactoryAllocator(sizeof(Liquid::CMeshGeomFactory), 4);

// STATIC: 0x00D31974
CDataAllocator Liquid::s_chunkGeomFactoryAllocator(sizeof(Liquid::CChunkGeomFactory), 0x10);

// VERIFY: 90914174 auto
// OFFSET: 0x007D4360
void Liquid::CMeshGeomFactory::SetMagma(int32_t magma) {
    this->m_magma = magma;
}

// VERIFY: d6bbccff auto
// OFFSET: 0x007D4370
void Liquid::CMeshGeomFactory::SetTexCoord1ValueX(float value) {
    this->m_texCoord1ValueX = value;
}

// VERIFY: 443374e2 auto
// OFFSET: 0x007D43E0
void Liquid::CMeshGeomFactory::SetDiffuseColor(const CImVector& color) {
    this->m_diffuseColor = color;
}

// VERIFY: 09b8bb30 auto
// OFFSET: 0x007D47D0
void Liquid::ClearGeomFactories() {
    Liquid::s_meshGeomFactoryAllocator.Clear(".?AVCMeshGeomFactory@Liquid@@", SLOG_OBJECT, 1);
    Liquid::s_chunkGeomFactoryAllocator.Clear(".?AVCChunkGeomFactory@Liquid@@", SLOG_OBJECT, 1);
}

// VERIFY: 32ee76a0 manual
// OFFSET: 0x007D4380
void Liquid::CMeshGeomFactory::Purge() {
    if (this->m_mapObjGroup) {
        this->m_mapObjGroup->FreeLiquidVB();
    }
}

// VERIFY: 2fdfe76c auto
// OFFSET: 0x007D4390
void Liquid::CChunkGeomFactory::Purge() {
    if (this->m_buf) {
        CMapChunkLiquidBuf::Free(this->m_buf);
        this->m_buf = nullptr;
    }
}

// VERIFY: a71b0035 manual
// OFFSET: 0x007D43B0
void Liquid::CMeshGeomFactory::Release() {
    if (this->m_refCount-- == 1) {
        this->~CMeshGeomFactory();
        Liquid::s_meshGeomFactoryAllocator.PutData(this, nullptr, 0);
    }
}

// VERIFY: 670828bb manual
// OFFSET: 0x007D4760
void Liquid::CChunkGeomFactory::Release() {
    if (this->m_refCount-- == 1) {
        this->~CChunkGeomFactory();
        Liquid::s_chunkGeomFactoryAllocator.PutData(this, nullptr, 0);
    }
}

// VERIFY: de869a00 manual
// OFFSET: 0x007D4790
bool Liquid::CChunkGeomFactory::SetTransform(C44Matrix* transform) {
    if (this->m_transform == *transform) {
        return true;
    }

    this->m_transform = *transform;
    this->m_dirty = 1;

    return true;
}

// VERIFY: 05f0b938 manual
// OFFSET: 0x007D43F0
void Liquid::CMeshGeomFactory::GetBuffers(EGxVertexBufferFormat format, CGxBuf** vertexBuf, CGxBuf** indexBuf, CGxBatch* batch) {
    if (static_cast<uint16_t>(this->m_mapObjGroup->m_liquidMaterialId) >= static_cast<uint32_t>(this->m_mapObj->m_materialsCount)) {
        SErrDisplayAppFatal("Water in chunk \"%s\" of object \"%s\" has no materialId.", this->m_mapObjGroup->m_groupName, this->m_mapObj->m_wmoName);
    }

    uint32_t sharedCols = this->m_sharedVerts[0].m_count;
    uint32_t sharedRows = this->m_sharedVerts[1].m_count;
    uint32_t sharedTiles = this->m_mapObjGroup->GetNumSharedTiles();
    uint32_t vertexCount = (sharedRows + this->m_mapObjGroup->m_liquidVerts.y) * (sharedCols + this->m_mapObjGroup->m_liquidVerts.x) + 6 * sharedTiles;
    this->m_mapObjGroup->AllocLiquidVB(format, vertexCount, 3 * vertexCount, &this->m_batch);

    *vertexBuf = this->m_mapObjGroup->m_liquidVertsBlock->m_buffer;
    *indexBuf = this->m_mapObjGroup->m_liquidIndicesBlock->m_buffer;

    if (!(*vertexBuf)->m_hasData || !(*vertexBuf)->m_valid || !(*indexBuf)->m_hasData || !(*indexBuf)->m_valid) {
        char* vertices = g_theGxDevicePtr->BufLock(*vertexBuf);
        uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(*indexBuf));
        uint16_t* indexStart = indices;

        if (!vertices || !indices) {
            if (vertices) {
                CGxBuf* buf = *vertexBuf;
                g_theGxDevicePtr->BufUnlock(buf, 0);
                buf->m_hasData = 1;
            }

            if (indices) {
                CGxBuf* buf = *indexBuf;
                g_theGxDevicePtr->BufUnlock(buf, 0);
                buf->m_hasData = 1;
            }

            return;
        }

        uint32_t stride = GxVertexSize(format);
        C3Vector* position = reinterpret_cast<C3Vector*>(vertices + GxVertexAttribOffset(format, GxVA_Position));
        C3Vector* normal = nullptr;
        CImVector* color = nullptr;
        C2Vector* texCoord0 = nullptr;
        C2Vector* texCoord1 = nullptr;

        if (GxVertexHasAttrib(format, GxVA_Normal)) {
            normal = reinterpret_cast<C3Vector*>(vertices + GxVertexAttribOffset(format, GxVA_Normal));
        }

        if (GxVertexHasAttrib(format, GxVA_Color0)) {
            color = reinterpret_cast<CImVector*>(vertices + GxVertexAttribOffset(format, GxVA_Color0));
        }

        if (GxVertexHasAttrib(format, GxVA_TexCoord0)) {
            texCoord0 = reinterpret_cast<C2Vector*>(vertices + GxVertexAttribOffset(format, GxVA_TexCoord0));
        }

        if (GxVertexHasAttrib(format, GxVA_TexCoord1)) {
            texCoord1 = reinterpret_cast<C2Vector*>(vertices + GxVertexAttribOffset(format, GxVA_TexCoord1));
        }

        const CImVector* liquidColor = &this->m_diffuseColor;
        uint16_t count = this->m_mapObj->BuildLiquidVerts(this->m_mapObjGroup, C44Matrix(), liquidColor, this->m_magma, this->m_texCoord1ValueX, this->m_sharedVerts, stride, &position, &normal, &color, &texCoord0, &texCoord1);
        this->m_mapObj->BuildLiquidIndices(this->m_mapObjGroup, this->m_sharedVerts, &indices, 0);
        count += this->m_mapObj->ClipGroupGeometry(this->m_mapObjGroup, C44Matrix(), liquidColor, this->m_magma, this->m_texCoord1ValueX, stride, &position, &normal, &color, &texCoord0, &texCoord1, &indices, count);

        CGxBuf* buf = *vertexBuf;
        g_theGxDevicePtr->BufUnlock(buf, 0);
        buf->m_hasData = 1;

        if (indices) {
            buf = *indexBuf;
            g_theGxDevicePtr->BufUnlock(buf, 0);
            buf->m_hasData = 1;
        }

        CGxBatch* meshBatch = this->m_batch;
        meshBatch->m_primType = GxPrim_TriangleStrip;
        meshBatch->m_start = 0;
        meshBatch->m_count = indices - indexStart;
        meshBatch->m_minIndex = 0;
        meshBatch->m_maxIndex = count - 1;
    }

    *batch = *this->m_batch;
}

// VERIFY: ac0e5a30 manual
// OFFSET: 0x007D4800
Liquid::CMeshGeomFactory::~CMeshGeomFactory() {
    if (this->m_mapObjGroup) {
        this->m_mapObjGroup->FreeLiquidVB();
    }
}

// VERIFY: 2528fd80 manual
// OFFSET: 0x007D4850
Liquid::CChunkGeomFactory::CChunkGeomFactory() {
    this->m_refCount = 1;
    this->m_dirty = 1;
    this->m_buf = nullptr;
    this->m_batch.m_primType = GxPrims_Last;
    this->m_batch.m_start = 0;
    this->m_batch.m_count = 0;
    this->m_batch.m_minIndex = 0;
    this->m_batch.m_maxIndex = 0;
}

// VERIFY: ae53ab8b manual
// OFFSET: 0x007D48C0
Liquid::CChunkGeomFactory::~CChunkGeomFactory() {
    if (this->m_buf) {
        CMapChunkLiquidBuf::Free(this->m_buf);
        this->m_buf = nullptr;
    }
}

// INLINED
Liquid::CMeshGeomFactory::CMeshGeomFactory() {
    this->m_refCount = 1;
    this->m_mapObj = nullptr;
    this->m_mapObjGroup = nullptr;
    this->m_batch = nullptr;
    *reinterpret_cast<int32_t*>(&this->m_diffuseColor) = -1;
    this->m_texCoord1ValueX = 0.0f;
    this->m_magma = 0;
    this->unused_0024 = 0;
}

// VERIFY: 634ede17 auto
// OFFSET: 0x007D49B0
Liquid::CMeshGeomFactory* Liquid::CMeshGeomFactory::Create(CMapObj* mapObj, CMapObjGroup* mapObjGroup) {
    Liquid::CMeshGeomFactory* factory = Liquid::s_meshGeomFactoryAllocator.New<Liquid::CMeshGeomFactory>(0);
    factory->m_mapObj = mapObj;
    factory->m_mapObjGroup = mapObjGroup;

    return factory;
}

// VERIFY: 0b00c5ac manual
// OFFSET: 0x007D49D0
Liquid::CChunkGeomFactory* Liquid::CChunkGeomFactory::Create(const TSGrowableArray<CChunkLiquid*>& chunks) {
    void* mem = Liquid::s_chunkGeomFactoryAllocator.GetData(0, ".?AVCChunkGeomFactory@Liquid@@", SLOG_OBJECT);
    Liquid::CChunkGeomFactory* factory = mem ? new (mem) Liquid::CChunkGeomFactory() : nullptr;

    for (uint32_t i = 0; i < chunks.m_count; i++) {
        factory->m_chunks.New(chunks.m_data[i]);
    }

    return factory;
}

// VERIFY: bb721706 manual
// OFFSET: 0x007D4AB0
void Liquid::CChunkGeomFactory::GetBuffers(EGxVertexBufferFormat format, CGxBuf** vertexBuf, CGxBuf** indexBuf, CGxBatch* batch) {
    if (this->m_buf && this->m_buf->Valid() && !this->m_dirty) {
        *vertexBuf = this->m_buf->m_vertexBuf;
        *indexBuf = this->m_buf->m_indexBuf;
        *batch = this->m_batch;
        return;
    }

    static TSGrowableArray<uint16_t> s_vertexCounts;

    s_vertexCounts.Clear();

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;

    for (uint32_t i = 0; i < this->m_chunks.m_count; i++) {
        CChunkLiquid* liquid = this->m_chunks.m_data[i];
        uint16_t cols = liquid->m_tileEnd.y - liquid->m_tileBegin.y;
        uint16_t rows = liquid->m_tileEnd.x - liquid->m_tileBegin.x;
        uint16_t count = (cols + 1) * (rows + 1);
        s_vertexCounts.New(count);
        vertexCount += count;
        indexCount += 6 * cols * rows;
    }

    if (!this->m_buf) {
        this->m_buf = CMapChunkLiquidBuf::Alloc(vertexCount, indexCount, 0, format);
    }

    *vertexBuf = this->m_buf->m_vertexBuf;
    *indexBuf = this->m_buf->m_indexBuf;

    if (this->m_dirty) {
        (*vertexBuf)->m_hasData = 0;
        (*indexBuf)->m_hasData = 0;
    }

    CGxBuf* vb = *vertexBuf;

    if (!vb->m_hasData || !vb->m_valid) {
        char* data = g_theGxDevicePtr->BufLock(vb);

        if (data) {
            uint32_t stride = GxVertexSize(format);
            C3Vector* position = reinterpret_cast<C3Vector*>(data + GxVertexAttribOffset(format, GxVA_Position));
            C3Vector* normal = nullptr;
            uint32_t* color = nullptr;
            C2Vector* texCoord0 = nullptr;
            C2Vector* texCoord1 = nullptr;

            if (GxVertexHasAttrib(format, GxVA_Normal)) {
                normal = reinterpret_cast<C3Vector*>(data + GxVertexAttribOffset(format, GxVA_Normal));
            }

            if (GxVertexHasAttrib(format, GxVA_Color0)) {
                color = reinterpret_cast<uint32_t*>(data + GxVertexAttribOffset(format, GxVA_Color0));
            }

            if (GxVertexHasAttrib(format, GxVA_TexCoord0)) {
                texCoord0 = reinterpret_cast<C2Vector*>(data + GxVertexAttribOffset(format, GxVA_TexCoord0));
            }

            if (GxVertexHasAttrib(format, GxVA_TexCoord1)) {
                texCoord1 = reinterpret_cast<C2Vector*>(data + GxVertexAttribOffset(format, GxVA_TexCoord1));
            }

            CChunkLiquid* first = this->m_chunks.m_data[0];

            for (uint32_t i = 0; i < this->m_chunks.m_count; i++) {
                CChunkLiquid* liquid = this->m_chunks.m_data[i];
                C44Matrix transform = this->m_transform;
                C3Vector offset(liquid->m_topLeftCoords.x - first->m_topLeftCoords.x, liquid->m_topLeftCoords.y - first->m_topLeftCoords.y, liquid->m_topLeftCoords.z - first->m_topLeftCoords.z);
                transform.Translate(offset);
                liquid->BuildVertices(&transform, stride, &position, &normal, &color, &texCoord1, &texCoord0);
            }

            g_theGxDevicePtr->BufUnlock(vb, 0);
            vb->m_hasData = 1;
        }
    }

    CGxBuf* ib = *indexBuf;

    if (!ib->m_hasData || !ib->m_valid) {
        batch->m_primType = GxPrim_TriangleStrip;
        batch->m_start = 0;
        batch->m_count = 0;
        batch->m_minIndex = 0;
        batch->m_maxIndex = vertexCount - 1;

        uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(ib));
        uint16_t baseVertex = 0;

        for (uint32_t i = 0; i < this->m_chunks.m_count; i++) {
            this->m_chunks.m_data[i]->BuildIndices(indices + batch->m_count, baseVertex, batch);
            baseVertex += s_vertexCounts.m_data[i];
        }

        g_theGxDevicePtr->BufUnlock(ib, 0);
        ib->m_hasData = 1;
        this->m_batch = *batch;
    }

    this->m_format = format;
    this->m_dirty = 0;
}

// VERIFY: b01df36e auto
// OFFSET: 0x008813A0
bool Liquid::CMeshGeomFactory::SetTransform(C44Matrix* transform) {
    return false;
}
