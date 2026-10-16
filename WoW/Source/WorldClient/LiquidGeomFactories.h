#ifndef WOW_SOURCE_WORLDCLIENT_LIQUIDGEOMFACTORIES_H
#define WOW_SOURCE_WORLDCLIENT_LIQUIDGEOMFACTORIES_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "WoW/Source/LiquidSystem/Liquid.h"

class CChunkLiquid;
class CMapChunkLiquidBuf;
class CMapObj;
class CMapObjGroup;

namespace Liquid {
    class CChunkGeomFactory : public IGeomFactory {
        public:
        static CChunkGeomFactory* Create(const TSGrowableArray<CChunkLiquid*>& chunks);

        /* 0004 */ int32_t m_refCount;
        /* 0008 */ int32_t m_dirty;
        /* 000C */ TSGrowableArray<CChunkLiquid*> m_chunks;
        /* 001C */ CMapChunkLiquidBuf* m_buf;
        /* 0020 */ CGxBatch m_batch;
        /* 0030 */ EGxVertexBufferFormat m_format;
        /* 0034 */ C44Matrix m_transform;

        virtual ~CChunkGeomFactory();
        virtual void Release();
        virtual void GetBuffers(EGxVertexBufferFormat format, CGxBuf** vertexBuf, CGxBuf** indexBuf, CGxBatch* batch);
        virtual void Purge();
        virtual bool SetTransform(C44Matrix* transform);

        CChunkGeomFactory();
    };

    class CMeshGeomFactory : public IGeomFactory {
        public:
        static CMeshGeomFactory* Create(CMapObj* mapObj, CMapObjGroup* mapObjGroup);

        void SetDiffuseColor(const CImVector& color);
        void SetMagma(int32_t magma);
        void SetTexCoord1ValueX(float value);

        /* 0004 */ int32_t m_refCount;
        /* 0008 */ CMapObj* m_mapObj;
        /* 000C */ CMapObjGroup* m_mapObjGroup;
        /* 0010 */ CGxBatch* m_batch;
        /* 0014 */ CImVector m_diffuseColor;
        /* 0018 */ int32_t unused_0018;
        /* 001C */ int32_t m_magma;
        /* 0020 */ float m_texCoord1ValueX;
        /* 0024 */ int32_t unused_0024;
        /* 0028 */ TSGrowableArray<uint8_t> m_sharedVerts[2];

        virtual ~CMeshGeomFactory();
        virtual void Release();
        virtual void GetBuffers(EGxVertexBufferFormat format, CGxBuf** vertexBuf, CGxBuf** indexBuf, CGxBatch* batch);
        virtual void Purge();
        virtual bool SetTransform(C44Matrix* transform);

        CMeshGeomFactory();
    };

    extern CDataAllocator s_meshGeomFactoryAllocator;
    extern CDataAllocator s_chunkGeomFactoryAllocator;

    void ClearGeomFactories();
}

#endif
