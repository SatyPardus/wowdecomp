#ifndef ENGINE_SOURCE_GX_TYPES_H
#define ENGINE_SOURCE_GX_TYPES_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/CGxDevice/Types.h"
#include "Engine/Source/Storm/H/stpl.h"

struct CGxBuf;
struct CGxPool;
class CGxQuery;

enum EGxApi {
    GxApi_OpenGl = 0,
    GxApi_D3d9 = 1,
    GxApi_D3d9Ex = 2,
    GxApi_D3d10 = 3,
    GxApi_D3d11 = 4,
    GxApi_GLL = 5,
    GxApis_Last = 6,
};

enum EGxBlend {
    GxBlend_Opaque = 0,
    GxBlend_AlphaKey = 1,
    GxBlend_Alpha = 2,
    GxBlend_Add = 3,
    GxBlend_Mod = 4,
    GxBlend_Mod2x = 5,
    GxBlend_ModAdd = 6,
    GxBlend_InvSrcAlphaAdd = 7,
    GxBlend_InvSrcAlphaOpaque = 8,
    GxBlend_SrcAlphaOpaque = 9,
    GxBlend_NoAlphaAdd = 10,
    GxBlend_ConstantAlpha = 11,
    GxBlends_Last = 12,
};

enum EGxCmp : int32_t {
    GxCmp_LEqual = 0,
    GxCmp_Equal = 1,
    GxCmp_GEqual = 2,
    GxCmp_Less = 3,
    GxCmps_Last = 4,
};

enum EGxTexFormat {
    GxTex_Unknown = 0,
    GxTex_Abgr8888 = 1,
    GxTex_Argb8888 = 2,
    GxTex_Argb4444 = 3,
    GxTex_Argb1555 = 4,
    GxTex_Rgb565 = 5,
    GxTex_Dxt1 = 6,
    GxTex_Dxt3 = 7,
    GxTex_Dxt5 = 8,
    GxTex_Uv88 = 9,
    GxTex_Gr1616F = 10,
    GxTex_R32F = 11,
    GxTex_D24X8 = 12,
    GxTexFormats_Last = 13,
};

enum EGxVertexBufferFormat {
    GxVBF_P = 0,
    GxVBF_PN = 1,
    GxVBF_PNC = 2,
    GxVBF_PNT = 3,
    GxVBF_PNCT = 4,
    GxVBF_PNT2 = 5,
    GxVBF_PNCT2 = 6,
    GxVBF_PC = 7,
    GxVBF_PCT = 8,
    GxVBF_PCT2 = 9,
    GxVBF_PT = 10,
    GxVBF_PT2 = 11,
    GxVBF_PBNT2 = 12,
    GxVBF_PNC2T2 = 13,
    GxVertexBufferFormats_Last = 14,
};

enum EGxBuffer {
    GxBuffers_Color = 0,
    GxBuffers_Depth = 1,
    GxBuffers_Last = 2,
};

enum EGxBufferBit : uint32_t {
    GxBufferBit_Color = 0x1,
    GxBufferBit_Depth = 0x2,
};

enum EGxOverride {
    GxOverride_PixelShader = 0,
    GxOverride_Unk1 = 1,
    GxOverride_Unk2 = 2,
    GxOverride_Unk3 = 3,
    GxOverride_Unk4 = 4,
    GxOverride_Unk5 = 5,
    GxOverride_Unk6 = 6,
    GxOverride_Unk7 = 7,
    GxOverride_Unk8 = 8,
    GxOverrides_Last = 9,
};

enum EGxQueryParam {
    GxQueryParam_Unk0 = 0,
    GxQueryParam_Unk1 = 1,
    GxQueryParams_Last = 2,
};

enum EGxTexCommand {
    GxTex_Lock = 0,
    GxTex_Latch = 1,
    GxTex_Unlock = 2,
    GxTex_Recreate = 3,
    GxTexCommands_Last = 4,
};

enum EGx : int32_t {
    Gx_SizeMinimized = 1,
    Gx_SizeMaxHide = 2,
};

enum EGxWM {
    GxWM_Size = 0,
    GxWM_DisplayChange = 1,
    GxWM_Destroy = 2,
    GxWM_SetFocus = 3,
    GxWM_KillFocus = 4,
};

enum EGxTexCubeFace : uint32_t {
    GxTex_PositiveX = 0,
    GxTex_NegativeX = 1,
    GxTex_PositiveY = 2,
    GxTex_NegativeY = 3,
    GxTex_PositiveZ = 4,
    GxTex_NegativeZ = 5,
    GxTexCubeFaces_Last = 6,
};

enum EGxTexGen {
    GxTexGen_Disabled = 0,
    GxTexGen_ObjectLinear = 1,
    GxTexGen_World = 2,
    GxTexGen_View = 3,
    GxTexGen_ReflectionMap = 4,
    GxTexGen_Normal = 5,
    GxTexGen_SphereMap = 6,
    GxTexGens_Last = 7,
};

enum EGxTexFilter {
    GxTex_Nearest = 0,
    GxTex_Linear = 1,
    GxTex_NearestMipNearest = 2,
    GxTex_LinearMipNearest = 3,
    GxTex_LinearMipLinear = 4,
    GxTex_Anisotropic = 5,
    GxTexFilters_Last = 6,
};

enum EGxTexWrapMode {
    GxTex_Clamp = 0,
    GxTex_Wrap = 1,
};

enum EGxXform {
    GxXform_Tex0 = 0,
    GxXform_Tex1 = 1,
    GxXform_Tex2 = 2,
    GxXform_Tex3 = 3,
    GxXform_Tex4 = 4,
    GxXform_Tex5 = 5,
    GxXform_Tex6 = 6,
    GxXform_Tex7 = 7,
    GxXform_World = 8,
    GxXform_Projection = 9,
    GxXform_View = 10,
    GxXforms_Last = 11,
};

enum BlitFormat {
    BlitFormat_Unknown = 0,
    BlitFormat_Abgr8888 = 1,
    BlitFormat_Argb8888 = 2,
    BlitFormat_Argb4444 = 3,
    BlitFormat_Argb1555 = 4,
    BlitFormat_Rgb565 = 5,
    BlitFormat_Dxt1 = 6,
    BlitFormat_Dxt3 = 7,
    BlitFormat_Dxt5 = 8,
    BlitFormat_Uv88 = 9,
    BlitFormat_Gr1616F = 10,
    BlitFormat_R32F = 11,
    BlitFormat_D24X8 = 12,
    BlitFormats_Last = 13,
};

class CGxBatch {
    public:
    /* 0000 */ EGxPrim m_primType;
    /* 0004 */ uint32_t m_start;
    /* 0008 */ uint32_t m_count;
    /* 000C */ uint16_t m_minIndex;
    /* 000E */ uint16_t m_maxIndex;
};

enum EGxColorFormat {
    GxCF_argb = 0,
    GxCF_rgba = 1,
    GxColorFormats_Last = 2,
};

class CGxCaps {
    public:
    /* 0000 */ int32_t m_numTmus;
    /* 0004 */ int32_t m_pixelCenterOnEdge;
    /* 0008 */ int32_t m_texelCenterOnEdge;
    /* 000C */ int32_t m_numStreams;
    /* 0010 */ int32_t m_streamOffset;
    /* 0014 */ EGxColorFormat m_colorFormat;
    /* 0018 */ int32_t unk_0018;
    /* 001C */ uint32_t m_maxIndex;
    /* 0020 */ int32_t m_generateMipMaps;
    /* 0024 */ int32_t m_texFmt [13];
    /* 0058 */ int32_t m_texTarget [4];
    /* 0068 */ uint32_t m_texNonPow2;
    /* 006C */ uint32_t m_texMaxSize [4];
    /* 007C */ int32_t m_rttFormat [13];
    /* 00B0 */ uint32_t unk_00B0;
    /* 00B4 */ int32_t m_shaderTargets [6];
    /* 00CC */ uint32_t m_shaderConstants [6];
    /* 00E4 */ int32_t m_texFilterTrilinear;
    /* 00E8 */ int32_t m_texFilterAnisotropic;
    /* 00EC */ uint32_t m_maxTexAnisotropy;
    /* 00F0 */ int32_t m_depthBias;
    /* 00F4 */ int32_t m_colorWrite;
    /* 00F8 */ int32_t m_maxClipPlanes;
    /* 00FC */ int32_t m_hwCursor;
    /* 0100 */ int32_t m_occlusionQuery;
    /* 0104 */ int32_t m_pointParameters;
    /* 0108 */ float m_pointScaleMax;
    /* 010C */ int32_t m_pointSprite;
    /* 0110 */ uint32_t m_blendFactor;
    /* 0114 */ uint32_t unused_0114 [5];
    /* 0128 */ int32_t m_oglRtAlpha;
    /* 012C */ int32_t m_stereoAvailable;
    /* 0130 */ int32_t m_shaderAlphaRef;
    /* 0134 */ int32_t unk_0134;
    /* 0138 */ int32_t unk_0138;
};

class CGxQuery {
    public:
    /* 0000 */ void* m_apiSpecific;
    /* 0004 */ EGxQueryType m_type;
    /* 0008 */ TSLink<CGxQuery> m_link;

    CGxQuery(EGxQueryType type);
};

// INLINED
inline CGxQuery::CGxQuery(EGxQueryType type) {
    this->m_apiSpecific = nullptr;
    this->m_type = type;
}

enum EGxPoolTarget {
    GxPoolTarget_Vertex = 0,
    GxPoolTarget_Index = 1,
    GxPoolTargets_Last = 2,
};

enum EGxPoolUsage {
    GxPoolUsage_Static = 0,
    GxPoolUsage_Dynamic = 1,
    GxPoolUsage_Stream = 2,
    GxPoolUsages_Last = 3,
};

enum EGxRenderState {
    GxRs_PolygonOffset = 0,
    GxRs_MatDiffuse = 1,
    GxRs_MatEmissive = 2,
    GxRs_MatSpecular = 3,
    GxRs_MatSpecularExp = 4,
    GxRs_NormalizeNormals = 5,
    GxRs_BlendingMode = 6,
    GxRs_AlphaRef = 7,
    GxRs_FogStart = 8,
    GxRs_FogEnd = 9,
    GxRs_FogColor = 10,
    GxRs_Lighting = 11,
    GxRs_Fog = 12,
    GxRs_DepthTest = 13,
    GxRs_DepthFunc = 14,
    GxRs_DepthWrite = 15,
    GxRs_ColorWrite = 16,
    GxRs_Culling = 17,
    GxRs_ClipPlaneMask = 18,
    GxRs_Multisample = 19,
    GxRs_ScissorTest = 20,
    GxRs_Texture0 = 21,
    GxRs_Texture1 = 22,
    GxRs_Texture2 = 23,
    GxRs_Texture3 = 24,
    GxRs_Texture4 = 25,
    GxRs_Texture5 = 26,
    GxRs_Texture6 = 27,
    GxRs_Texture7 = 28,
    GxRs_Texture8 = 29,
    GxRs_Texture9 = 30,
    GxRs_Texture10 = 31,
    GxRs_Texture11 = 32,
    GxRs_Texture12 = 33,
    GxRs_Texture13 = 34,
    GxRs_Texture14 = 35,
    GxRs_Texture15 = 36,
    GxRs_ColorOp0 = 37,
    GxRs_ColorOp1 = 38,
    GxRs_ColorOp2 = 39,
    GxRs_ColorOp3 = 40,
    GxRs_ColorOp4 = 41,
    GxRs_ColorOp5 = 42,
    GxRs_ColorOp6 = 43,
    GxRs_ColorOp7 = 44,
    GxRs_AlphaOp0 = 45,
    GxRs_AlphaOp1 = 46,
    GxRs_AlphaOp2 = 47,
    GxRs_AlphaOp3 = 48,
    GxRs_AlphaOp4 = 49,
    GxRs_AlphaOp5 = 50,
    GxRs_AlphaOp6 = 51,
    GxRs_AlphaOp7 = 52,
    GxRs_TexGen0 = 53,
    GxRs_TexGen1 = 54,
    GxRs_TexGen2 = 55,
    GxRs_TexGen3 = 56,
    GxRs_TexGen4 = 57,
    GxRs_TexGen5 = 58,
    GxRs_TexGen6 = 59,
    GxRs_TexGen7 = 60,
    GxRs_TextureShader0 = 61,
    GxRs_TextureShader1 = 62,
    GxRs_TextureShader2 = 63,
    GxRs_TextureShader3 = 64,
    GxRs_TextureShader4 = 65,
    GxRs_TextureShader5 = 66,
    GxRs_TextureShader6 = 67,
    GxRs_TextureShader7 = 68,
    GxRs_TextureCoord0 = 69,
    GxRs_TextureCoord1 = 70,
    GxRs_TextureCoord2 = 71,
    GxRs_TextureCoord3 = 72,
    GxRs_TextureCoord4 = 73,
    GxRs_TextureCoord5 = 74,
    GxRs_TextureCoord6 = 75,
    GxRs_TextureCoord7 = 76,
    GxRs_VertexShader = 77,
    GxRs_PixelShader = 78,
    GxRs_PointScale = 79,
    GxRs_PointScaleAttenuation = 80,
    GxRs_PointScaleMin = 81,
    GxRs_PointScaleMax = 82,
    GxRs_PointSprite = 83,
    GxRs_BlendFactor = 84,
    GxRs_ColorMaterial = 85,
    GxRenderStates_Last = 86,
};

enum EGxShTarget {
    GxSh_Vertex = 0,
    GxSh_Hull = 1,
    GxSh_Domain = 2,
    GxSh_Geometry = 3,
    GxSh_Pixel = 4,
    GxSh_Compute = 5,
    GxShTargets_Last = 6,
};

typedef intptr_t (*GxWindowProc)(void* window, uint32_t message, uintptr_t wparam, intptr_t lparam);

enum EGxuDrawListCategory {
    GxuCat_0 = 0,
    GxuCat_1 = 1,
    GxuCat_2 = 2,
    GxuCats_Last = 3,
};

enum EGxuDrawListSorting {
    GxuSort_B2F = 0,
    GxuSort_VtxStream = 1,
    GxuSorts_Last = 2,
};

#endif
