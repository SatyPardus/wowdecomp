#ifndef WOW_SOURCE_WORLDCLIENT_MAPRENDERCHUNK_H
#define WOW_SOURCE_WORLDCLIENT_MAPRENDERCHUNK_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Gx/CGxTex.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Vector.h"
#include "Engine/Source/Tempest/CImVector.h"

class CMapArea;
class CM2Lighting;
class CGxShader;
struct SMLayer;

struct CGxBuf;
struct CGxPool;
class CMapChunk;
class CMapChunkBufBlock;
class CMapRenderChunk;
class CMapChunkBuf;
class CTexture;

class CMapChunkBuf {
    public:
    /* 0000 */ CGxBuf* m_vertexBuf;
    /* 0004 */ CGxBuf* m_indexBuf;
    /* 0008 */ CMapRenderChunk* m_renderChunk;
    /* 000C */ CMapChunkBufBlock* m_block;
    /* 0010 */ uint32_t m_unkFlags;
    /* 0014 */ TSLink<CMapChunkBuf> m_freeLink;

    CMapChunkBuf();
};

class CMapChunkBufBlock {
    public:
    /* 0000 */ uint32_t m_vertexCount;
    /* 0004 */ CMapChunkBuf m_bufs [2];
    /* 003C */ TSLink<CMapChunkBufBlock> m_blockLink;
    /* 0044 */ TSLink<CMapChunkBufBlock> m_bufLink;

    CMapChunkBufBlock();
    ~CMapChunkBufBlock();
};

class CMapRenderChunkLayer {
    public:
    /* 0000 */ int16_t m_flags;
    /* 0002 */ int16_t m_layerIndex;
    /* 0004 */ CTexture* m_texture;
    /* 0008 */ int32_t m_textureId;
    /* 000C */ CTexture* m_layerTexture;
    /* 0010 */ CMapRenderChunk* m_owner;

    CMapRenderChunkLayer();
};

class CMapRenderChunk {
    public:
    struct SLayerAlpha;
    struct SVertexShaderConstants;
    typedef void (*RenderLayersFunc)(CMapRenderChunk* renderChunk);

    static int32_t s_bPoolsDirty;
    static EGxVertexBufferFormat s_gxBufVertexFormat;
    static uint32_t s_pnEstimateVertex;
    static uint32_t s_pnEstimateIndex;
    static CGxPool* s_gxVertexPool;
    static CGxPool* s_gxIndexPool;
    static TSGrowableArray<CMapChunkBufBlock> s_chunkBlockArray;
    static TSExplicitList<CMapChunkBuf, offsetof(CMapChunkBuf, m_freeLink)> s_renderChunkBufFreeList;
    static TSExplicitList<CMapChunkBufBlock, offsetof(CMapChunkBufBlock, m_blockLink)> s_chunkBufBlockFreeList;
    static TSExplicitList<CMapChunkBufBlock, offsetof(CMapChunkBufBlock, m_bufLink)> s_bufList;
    static CGxBatch s_chunkBatch;
    static uint32_t s_defaultTex[8192];
    static CGxShader* s_currentShader[4];
    static CGxShader* s_currentShaderX[4];
    static CGxShader* s_pixelShader[2];
    static RenderLayersFunc s_renderLayersFunc;
    static CImVector s_fogColor;
    static SVertexShaderConstants s_vertexShaderConsts;

    static void Destroy();
    static void DestroyBufs();
    static void Initialize();
    static void SetDirty();
    static void UpdatePools();
    static void SetVertexFormat(EGxVertexBufferFormat format);
    static void PurgeChunkBufBlocks();
    static CMapChunkBuf* AllocBuf(int32_t a1, CMapRenderChunk* renderChunk);
    static CTexture* AllocTexture(uint32_t width, uint32_t height, void* userArg, GxTexUserFunc userFunc, EGxTexFormat format, EGxTexFormat dataFormat);
    static void UpdateShaderGxTexture(EGxTexCommand cmd, uint32_t w, uint32_t h, uint32_t d, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels);
    static void UpdateLayerGxTexture(EGxTexCommand cmd, uint32_t w, uint32_t h, uint32_t d, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels);
    static void UpdateShadowGxTexture(EGxTexCommand cmd, uint32_t w, uint32_t h, uint32_t d, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels);
    static void RenderMultiPassAlpha(CMapRenderChunk* renderChunk);
    static void RenderMultiPassAdditive(CMapRenderChunk* renderChunk);
    static void SetShaders(int32_t a1, int32_t a2);
    static void InitializeVertexShaderConstants(const C44Matrix& world, const C44Matrix& view);
    static CGxShader* GetVertexShader(int32_t pointLights, int32_t layers, int32_t specular, int32_t vertexColor, int32_t a5, int32_t shadow);
    static void ComputeTexMatrices(C44Matrix& layerMatrix, C44Matrix& alphaMatrix, const C3Vector& offset, float scale);
    static void RenderMultiPassPixelShaderBigAlpha(CMapRenderChunk* renderChunk);
    static void RenderMultiPassPixelShader(CMapRenderChunk* renderChunk);
    static void RenderMultiPassVertexPixelShaderBigAlpha(CMapRenderChunk* renderChunk);
    static void RenderMultiPassVertexPixelShader(CMapRenderChunk* renderChunk);
    static void SetVertexShader(CMapRenderChunk* renderChunk, int32_t layerCount, int32_t a3);
    static uint8_t* DecodeAlphaRLE(uint8_t* src, uint8_t* dst, int32_t count);
    static void SetupAlphaRows(uint8_t** shadowCursor, uint8_t** shadowRow, SLayerAlpha* layers, uint32_t size, uint8_t** alphaRows);
    static void SetupAlphaRows4444(uint32_t size, uint8_t** shadowRow, uint8_t** alphaRows, SLayerAlpha* layers, uint8_t** shadowCursor);
    static void __stdcall UnpackAlphaBitsFixed4444Mip0(uint16_t* dst, uint32_t size, SLayerAlpha* layer);
    static void __stdcall UnpackAlphaBitsFixed4444Mip1(uint16_t* dst, uint32_t size, SLayerAlpha* layer);
    static void __stdcall UnpackAlphaBitsUnfixed4444Mip0(uint16_t* dst, uint32_t size, SLayerAlpha* layer);
    static void __stdcall UnpackAlphaBitsUnfixed4444Mip1(uint16_t* dst, uint32_t size, SLayerAlpha* layer);
    static void __stdcall ExpandShadowBitsToTexels(uint16_t* dst, uint32_t size, uint8_t* shadowMap);
    static void __stdcall ExpandShadowBitsToTexelsMip(uint16_t* dst, uint32_t size, uint8_t* shadowMap);
    static void __stdcall UnpackAlphaShadowBitsFixed8888Mip0(CImVector* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, int32_t alphaSlot, uint8_t* shadowMap);
    static void __stdcall UnpackAlphaShadowBitsFixed8888Mip1(CImVector* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, int32_t alphaSlot, uint8_t* shadowMap);
    static void __stdcall UnpackAlphaShadowBitsFixed4444Mip0(uint16_t* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, uint8_t* shadowMap);
    static void __stdcall UnpackAlphaShadowBitsFixed4444Mip1(uint16_t* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, uint8_t* shadowMap);
    static void PackAlphaShadow4444(uint16_t* dst, uint8_t** alphaRows, uint8_t* shadowRow, uint32_t size);
    static void __stdcall UnpackAlphaShadowBitsUnfixed4444Mip0(uint16_t* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, uint8_t* shadowMap);
    static void __stdcall UnpackAlphaShadowBitsUnfixed4444Mip1(uint16_t* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, uint8_t* shadowMap);
    static void __stdcall RecreateAlphaBitsFixed8888Mip0(CImVector* dst, uint32_t size, SLayerAlpha* layer, uint8_t* shadowMap);
    static void __stdcall RecreateAlphaBitsFixed8888Mip1(CImVector* dst, uint32_t size, SLayerAlpha* layer, uint8_t* shadowMap);

    /* 0000 */ TSLink<CMapRenderChunk> m_renderChunkLink;
    /* 0008 */ uint8_t m_unkFlags;
    /* 0009 */ uint8_t m_layersCount;
    /* 000A */ int16_t m_renderFlags;
    /* 000C */ float m_lastUpdateTime;
    /* 0010 */ CMapChunk* m_mapChunks [2];
    /* 0018 */ C3Vector m_vec1;
    /* 0024 */ CAaSphere m_sphere;
    /* 0034 */ CMapRenderChunkLayer m_layers [4];
    /* 0084 */ CTexture* m_terrainBlendTexture;
    /* 0088 */ CTexture* m_shadowTexture;
    /* 008C */ CMapChunkBuf* m_chunkBuf;
    /* 0090 */ CGxBatch m_batch;

    ~CMapRenderChunk();
    CMapRenderChunk();
    void AddBatch(CMapChunk* chunk1, CMapChunk* chunk2, C3Vector* position, uint8_t flags);
    void RenderPrep();
    void CreateLayers();
    void CreateLayer(CMapArea* area, SMLayer* layer, int32_t checkDuplicate);
    void AllocLayerTextures();
    void AllocShaderTexture();
    void AllocLayerTexture(CMapRenderChunkLayer* layer);
    void AllocShadowTexture();
    void UpdateLoaded();
    void UseStreamingBufs();
    void RenderPrepBufs(CGxBuf* vertexBuf, CGxBuf* indexBuf);
    void CreateShaderTexture();
    void CreateChunkLayerTex(CMapRenderChunkLayer* layer);
    void CreateShadowTex();
    void CollectLayerAlphaPtrs(CMapChunk* chunk, SLayerAlpha* layers);
    void UnpackAlphaShadowBits(void* dstBase, int32_t dstOffset, int32_t dstPitch, uint32_t size, SLayerAlpha* cursors, int32_t alphaSlot, uint8_t* shadowMap, int32_t genFormat, int32_t doNotFixAlphaMap);
    void UnpackAlphaBitsFixed8888Mip0(CImVector* dst, uint32_t size, uint8_t* shadowMap);
    void UnpackAlphaBitsFixed8888Mip1(CImVector* dst, uint32_t size, uint8_t* shadowMap);
    void UnpackAlphaBits(void* dst, uint32_t size, SLayerAlpha* layerInfo, uint8_t* shadowMap, int32_t layerMode, int32_t bigAlpha);
    void RenderSetup(int32_t a2);
    void RenderSolid();
    void RenderSolidVertexPixelShader();
    void FreeBuf();
    void FreeLayers();
    void SelectLights(CM2Lighting* lighting);
    void RenderSolidPixelShader();
    void RenderSinglePassPixelShader();
    void RenderSinglePassVertexPixelShader();
    void RenderSinglePass();
    void RenderZoneDebug();
};

struct CMapRenderChunk::SVertexShaderConstants {
    struct SPointLight {
        /* 0000 */ C3Vector m_position;
        /* 000C */ float m_positionW;
        /* 0010 */ C3Vector m_color;
        /* 001C */ float m_colorW;
        /* 0020 */ C3Vector m_attenuation;
        /* 002C */ float m_attenuationW;
    };

    /* 0000 */ C44Matrix m_worldView;
    /* 0040 */ C44Matrix m_projection;
    /* 0080 */ C44Matrix m_viewTranspose;
    /* 00C0 */ C4Vector m_fog;
    /* 00D0 */ C4Vector m_texOffset [4];
    /* 0110 */ C4Vector m_texScale [6];
    /* 0170 */ C4Vector m_position;
    /* 0180 */ C4Vector m_sunDir;
    /* 0190 */ C3Vector m_sunAmbient;
    /* 019C */ float m_sunAmbientW;
    /* 01A0 */ C3Vector m_sunDiffuse;
    /* 01AC */ float m_sunDiffuseW;
    /* 01B0 */ C3Vector m_sunSpecular;
    /* 01BC */ float m_sunSpecularW;
    /* 01C0 */ SPointLight m_pointLights [3];
};

struct SMMapHeader {
    /* 0000 */ uint32_t m_flags;
    /* 0004 */ uint32_t m_something;
    /* 0008 */ uint32_t m_unused [6];
};

struct CMapRenderChunk::SLayerAlpha {
    /* 0000 */ uint32_t m_flags;
    /* 0004 */ uint8_t* m_alphaData;
};

#endif
