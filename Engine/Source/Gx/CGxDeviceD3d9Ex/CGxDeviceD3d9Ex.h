#ifndef ENGINE_SOURCE_GX_CGXDEVICED3D9EX_CGXDEVICED3D9EX_H
#define ENGINE_SOURCE_GX_CGXDEVICED3D9EX_CGXDEVICED3D9EX_H

#include <cstddef>
#include <cstdint>
#include <windows.h>
#include <d3d9.h>
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxTex.h"
#include "Engine/Source/Storm/H/stpl.h"

class CGxDeviceD3d9Ex : public CGxDevice {
    public:
    enum EDeviceState {
        Ds_SrcBlend = 0,
        Ds_DstBlend = 1,
        Ds_TssMagFilter0 = 2,
        Ds_TssMagFilter1 = 3,
        Ds_TssMagFilter2 = 4,
        Ds_TssMagFilter3 = 5,
        Ds_TssMagFilter4 = 6,
        Ds_TssMagFilter5 = 7,
        Ds_TssMagFilter6 = 8,
        Ds_TssMagFilter7 = 9,
        Ds_TssMagFilter8 = 10,
        Ds_TssMagFilter9 = 11,
        Ds_TssMagFilter10 = 12,
        Ds_TssMagFilter11 = 13,
        Ds_TssMagFilter12 = 14,
        Ds_TssMagFilter13 = 15,
        Ds_TssMagFilter14 = 16,
        Ds_TssMagFilter15 = 17,
        Ds_TssMinFilter0 = 18,
        Ds_TssMinFilter1 = 19,
        Ds_TssMinFilter2 = 20,
        Ds_TssMinFilter3 = 21,
        Ds_TssMinFilter4 = 22,
        Ds_TssMinFilter5 = 23,
        Ds_TssMinFilter6 = 24,
        Ds_TssMinFilter7 = 25,
        Ds_TssMinFilter8 = 26,
        Ds_TssMinFilter9 = 27,
        Ds_TssMinFilter10 = 28,
        Ds_TssMinFilter11 = 29,
        Ds_TssMinFilter12 = 30,
        Ds_TssMinFilter13 = 31,
        Ds_TssMinFilter14 = 32,
        Ds_TssMinFilter15 = 33,
        Ds_TssMipFilter0 = 34,
        Ds_TssMipFilter1 = 35,
        Ds_TssMipFilter2 = 36,
        Ds_TssMipFilter3 = 37,
        Ds_TssMipFilter4 = 38,
        Ds_TssMipFilter5 = 39,
        Ds_TssMipFilter6 = 40,
        Ds_TssMipFilter7 = 41,
        Ds_TssMipFilter8 = 42,
        Ds_TssMipFilter9 = 43,
        Ds_TssMipFilter10 = 44,
        Ds_TssMipFilter11 = 45,
        Ds_TssMipFilter12 = 46,
        Ds_TssMipFilter13 = 47,
        Ds_TssMipFilter14 = 48,
        Ds_TssMipFilter15 = 49,
        Ds_TssWrapU0 = 50,
        Ds_TssWrapU1 = 51,
        Ds_TssWrapU2 = 52,
        Ds_TssWrapU3 = 53,
        Ds_TssWrapU4 = 54,
        Ds_TssWrapU5 = 55,
        Ds_TssWrapU6 = 56,
        Ds_TssWrapU7 = 57,
        Ds_TssWrapU8 = 58,
        Ds_TssWrapU9 = 59,
        Ds_TssWrapU10 = 60,
        Ds_TssWrapU11 = 61,
        Ds_TssWrapU12 = 62,
        Ds_TssWrapU13 = 63,
        Ds_TssWrapU14 = 64,
        Ds_TssWrapU15 = 65,
        Ds_TssWrapV0 = 66,
        Ds_TssWrapV1 = 67,
        Ds_TssWrapV2 = 68,
        Ds_TssWrapV3 = 69,
        Ds_TssWrapV4 = 70,
        Ds_TssWrapV5 = 71,
        Ds_TssWrapV6 = 72,
        Ds_TssWrapV7 = 73,
        Ds_TssWrapV8 = 74,
        Ds_TssWrapV9 = 75,
        Ds_TssWrapV10 = 76,
        Ds_TssWrapV11 = 77,
        Ds_TssWrapV12 = 78,
        Ds_TssWrapV13 = 79,
        Ds_TssWrapV14 = 80,
        Ds_TssWrapV15 = 81,
        Ds_TssTexTransformFlags0 = 82,
        Ds_TssTexTransformFlags1 = 83,
        Ds_TssTexTransformFlags2 = 84,
        Ds_TssTexTransformFlags3 = 85,
        Ds_TssTexTransformFlags4 = 86,
        Ds_TssTexTransformFlags5 = 87,
        Ds_TssTexTransformFlags6 = 88,
        Ds_TssTexTransformFlags7 = 89,
        Ds_TssMaxAnisotropy0 = 90,
        Ds_TssMaxAnisotropy1 = 91,
        Ds_TssMaxAnisotropy2 = 92,
        Ds_TssMaxAnisotropy3 = 93,
        Ds_TssMaxAnisotropy4 = 94,
        Ds_TssMaxAnisotropy5 = 95,
        Ds_TssMaxAnisotropy6 = 96,
        Ds_TssMaxAnisotropy7 = 97,
        Ds_TssMaxAnisotropy8 = 98,
        Ds_TssMaxAnisotropy9 = 99,
        Ds_TssMaxAnisotropy10 = 100,
        Ds_TssMaxAnisotropy11 = 101,
        Ds_TssMaxAnisotropy12 = 102,
        Ds_TssMaxAnisotropy13 = 103,
        Ds_TssMaxAnisotropy14 = 104,
        Ds_TssMaxAnisotropy15 = 105,
        Ds_TssTexCoordIndex0 = 106,
        Ds_TssTexCoordIndex1 = 107,
        Ds_TssTexCoordIndex2 = 108,
        Ds_TssTexCoordIndex3 = 109,
        Ds_TssTexCoordIndex4 = 110,
        Ds_TssTexCoordIndex5 = 111,
        Ds_TssTexCoordIndex6 = 112,
        Ds_TssTexCoordIndex7 = 113,
        Ds_TssColorOp0 = 114,
        Ds_TssColorOp1 = 115,
        Ds_TssColorOp2 = 116,
        Ds_TssColorOp3 = 117,
        Ds_TssColorOp4 = 118,
        Ds_TssColorOp5 = 119,
        Ds_TssColorOp6 = 120,
        Ds_TssColorOp7 = 121,
        Ds_TssAlphaOp0 = 122,
        Ds_TssAlphaOp1 = 123,
        Ds_TssAlphaOp2 = 124,
        Ds_TssAlphaOp3 = 125,
        Ds_TssAlphaOp4 = 126,
        Ds_TssAlphaOp5 = 127,
        Ds_TssAlphaOp6 = 128,
        Ds_TssAlphaOp7 = 129,
        Ds_TssColorArg10 = 130,
        Ds_TssColorArg11 = 131,
        Ds_TssColorArg12 = 132,
        Ds_TssColorArg13 = 133,
        Ds_TssColorArg14 = 134,
        Ds_TssColorArg15 = 135,
        Ds_TssColorArg16 = 136,
        Ds_TssColorArg17 = 137,
        Ds_TssColorArg20 = 138,
        Ds_TssColorArg21 = 139,
        Ds_TssColorArg22 = 140,
        Ds_TssColorArg23 = 141,
        Ds_TssColorArg24 = 142,
        Ds_TssColorArg25 = 143,
        Ds_TssColorArg26 = 144,
        Ds_TssColorArg27 = 145,
        Ds_TssAlphaArg10 = 146,
        Ds_TssAlphaArg11 = 147,
        Ds_TssAlphaArg12 = 148,
        Ds_TssAlphaArg13 = 149,
        Ds_TssAlphaArg14 = 150,
        Ds_TssAlphaArg15 = 151,
        Ds_TssAlphaArg16 = 152,
        Ds_TssAlphaArg17 = 153,
        Ds_TssAlphaArg20 = 154,
        Ds_TssAlphaArg21 = 155,
        Ds_TssAlphaArg22 = 156,
        Ds_TssAlphaArg23 = 157,
        Ds_TssAlphaArg24 = 158,
        Ds_TssAlphaArg25 = 159,
        Ds_TssAlphaArg26 = 160,
        Ds_TssAlphaArg27 = 161,
        Ds_AmbientMaterialSource = 162,
        Ds_DiffuseMaterialSource = 163,
        Ds_SpecularMaterialSource = 164,
        Ds_EmissiveMaterialSource = 165,
        Ds_Ambient = 166,
        Ds_AlphaBlendEnable = 167,
        Ds_AlphaTestEnable = 168,
        Ds_AlphaRef = 169,
        Ds_FogEnable = 170,
        Ds_ZWriteEnable = 171,
        Ds_ColorWriteEnable = 172,
        Ds_Lighting = 173,
        Ds_SpecularEnable = 174,
        Ds_CullMode = 175,
        Ds_ClipPlaneEnable = 176,
        Ds_ZFunc = 177,
        Ds_PointScaleA = 178,
        Ds_PointScaleB = 179,
        Ds_PointScaleC = 180,
        Ds_PointScaleEnable = 181,
        DeviceStates_Last = 182,
    };

    struct GxVertexDecl {
        /* 0000 */ uint32_t m_count;
        /* 0004 */ D3DVERTEXELEMENT9 m_elements [15];
        /* 007C */ IDirect3DVertexDeclaration9* m_d3dDecl;

        bool Compare(const D3DVERTEXELEMENT9* elements, uint32_t count);
    };

    struct ScratchTexture {
        /* 0000 */ LPDIRECT3DTEXTURE9 m_textures [1];
        /* 0004 */ uint32_t m_index;
    };

    /* 3968 */ HWND m_hwnd;
    /* 396C */ ATOM m_hwndClass;
    /* 396E */ uint16_t unused_396E;
    /* 3970 */ int32_t m_ownhwnd;
    /* 3974 */ HINSTANCE m_d3dLib;
    /* 3978 */ LPDIRECT3D9 m_d3d;
    /* 397C */ LPDIRECT3DDEVICE9 m_d3dDevice;
    /* 3980 */ D3DCAPS9 m_d3dCaps;
    /* 3AB0 */ int32_t m_d3dIsHwDevice;
    /* 3AB4 */ int32_t m_d3dNVAPI;
    /* 3AB8 */ uint32_t m_d3dStereoEnabled;
    /* 3ABC */ uint32_t m_d3dStereoRestore;
    /* 3AC0 */ void* m_d3dStereoHandle;
    /* 3AC4 */ float m_d3dStereoConvergence;
    /* 3AC8 */ float m_d3dStereoSeparation;
    /* 3ACC */ int32_t m_d3dStereoDirty;
    /* 3AD0 */ TSGrowableArray<CGxDeviceD3d9Ex::GxVertexDecl> m_gxVertexDecl;
    /* 3AE0 */ LPDIRECT3DVERTEXDECLARATION9 m_d3dVertexDecl [14];
    /* 3B18 */ D3DDISPLAYMODE m_desktopDisplayMode;
    /* 3B28 */ int32_t m_inScene;
    /* 3B2C */ int32_t m_ambientOnlyMode;
    /* 3B30 */ D3DFORMAT m_devDepthFormat;
    /* 3B34 */ D3DFORMAT m_devAdapterFormat;
    /* 3B38 */ LPDIRECT3DSURFACE9 m_defDepthStencilSurface;
    /* 3B3C */ LPDIRECT3DSURFACE9 m_defColorSurface;
    /* 3B40 */ LPDIRECT3DSURFACE9 m_defDepthSurface;
    /* 3B44 */ LPDIRECT3DSURFACE9 m_surface3B44;
    /* 3B48 */ LPDIRECT3DQUERY9 m_eventQuery;
    /* 3B4C */ int32_t m_hwCursorDirty;
    /* 3B50 */ LPDIRECT3DTEXTURE9 m_hwCursorTexture;
    /* 3B54 */ LPDIRECT3DSURFACE9 m_hwCursorBitmap;
    /* 3B58 */ LPDIRECT3DVERTEXDECLARATION9 m_d3dCurrentVertexDecl;
    /* 3B5C */ LPDIRECT3DINDEXBUFFER9 m_d3dCurrentIndexBuf;
    /* 3B60 */ LPDIRECT3DVERTEXBUFFER9 m_d3dVertexStreamBuf [8];
    /* 3B80 */ uint32_t m_d3dVertexStreamOfs [8];
    /* 3BA0 */ uint32_t m_d3dVertexStreamStride [8];
    /* 3BC0 */ CGxDeviceD3d9Ex::ScratchTexture m_scratchTextures [13];
    /* 3C28 */ TSExplicitList<CGxTex, offsetof(CGxTex, m_link2)> m_texCacheList;
    /* 3C34 */ TSExplicitList<CGxTex, offsetof(CGxTex, m_link)> m_texDestroyList;
    /* 3C40 */ uint32_t m_texCacheUsed;
    /* 3C44 */ uint32_t m_texCacheSize;
    /* 3C48 */ uint32_t m_deviceStates [DeviceStates_Last];
    /* 3F20 */ bool m_texEnabled [8];

    virtual void ITexMarkAsUpdated(CGxTex* texId);
    virtual void IRsSendToHw(EGxRenderState which);
    virtual void ICursorCreate(const CGxFormat& format);
    virtual void ICursorDestroy();
    virtual void ICursorDraw();
    virtual int32_t DeviceCreate(GxWindowProc windowProc, const CGxFormat& format);
    virtual int32_t DeviceCreate(void* window, const CGxFormat& format);
    virtual void DeviceDestroy();
    virtual void DeviceEvictResources();
    virtual int32_t DeviceSetFormat(const CGxFormat& format);
    virtual void DeviceSetBaseMipLevel(uint32_t level);
    virtual void DeviceSetGamma(float gamma);
    virtual void DeviceSetGamma(const CGxGammaRamp& ramp);
    virtual void* DeviceWindow();
    virtual void DeviceReadPixels(CiRect& rect, TSGrowableArray<CImVector>& pixels);
    virtual void DeviceReadDepths(CiRect& rect, TSGrowableArray<float>& depths);
    virtual int32_t DeviceWM(EGxWM wm, uintptr_t param1, uintptr_t param2);
    virtual void DeviceSetRenderTarget(EGxBuffer buffer, CGxTex* texture, uint32_t plane);
    virtual void DeviceSaveDepthSurface();
    virtual void DeviceRestoreDepthSurface();
    virtual void DeviceResolveDepthBuffer(CGxTex* texture);
    virtual void DeviceCopyTex(CGxTex* source, uint32_t sourcePlane, CGxTex* dest, uint32_t destPlane);
    virtual void DeviceOverride(EGxOverride override, uint32_t value);
    virtual void CapsWindowSize(CRect& rect);
    virtual void CapsWindowSizeInScreenCoords(CRect& rect);
    virtual void LogCrashInfo(char* buffer, uint32_t size);
    virtual void ScenePresent();
    virtual void SceneClear(uint32_t mask, CImVector color);
    virtual void XformSetProjection(const C44Matrix& matrix);
    virtual void XformSetView(const C44Matrix& matrix);
    virtual void Draw(CGxBatch* batch, int32_t count);
    virtual void PoolSizeSet(CGxPool* pool, uint32_t size);
    virtual void PoolDestroy(CGxPool* pool);
    virtual char* BufLock(CGxBuf* buf);
    virtual int32_t BufUnlock(CGxBuf* buf, uint32_t size);
    virtual void BufData(CGxBuf* buf, const void* data, uint32_t size, uint32_t offset);
    virtual void TexDestroy(CGxTex* texId);
    virtual int32_t TexCopy(CGxTex* dest, const C2iVector& destPos, const C2iVector& sourcePos, const C2iVector& size, uint32_t level, uint32_t plane);
    virtual int32_t TexStretch(CGxTex* dest, const CiRect* destRect, const CiRect* sourceRect, uint32_t level, uint32_t face);
    virtual void TexSetCacheSize(uint32_t size);
    virtual void QueryCreate(CGxQuery*& query, EGxQueryType type);
    virtual void QueryDestroy(CGxQuery*& query);
    virtual bool QueryBegin(CGxQuery* query);
    virtual bool QueryEnd(CGxQuery* query);
    virtual bool QueryGetParam(CGxQuery* query, EGxQueryParam param, uint32_t& data);
    virtual bool QueryGetData(CGxQuery* query, uint32_t& data);
    virtual void ShaderCreate(CGxShader** shaders, EGxShTarget target, const char* path, const char* name, uint32_t permutations);
    virtual void ShaderDestroy(CGxShader*& shader);
    virtual void IShaderReload(CGxShader* shader, const char* path, const char* name);
    virtual void IShaderCreate(CGxShader* shader);
    virtual void CursorSetVisible(int32_t visible);
    virtual void CursorUnlock(uint32_t x, uint32_t y);
    virtual void StereoSetConvergence(float convergence);
    virtual float StereoGetConvergence();
    virtual void StereoSetSeparation(float separation);
    virtual float StereoGetSeparation();
    virtual bool StereoEnabled();
    virtual void DeviceUnk81(uint32_t a, uint32_t b);
    virtual void DeviceUnk82();
    virtual void DeviceUnk83(uint32_t a, uint32_t b);

    static const D3DFORMAT s_GxFormatToD3dFormat[CGxFormat::Formats_Last];
    static const D3DFORMAT s_GxTexFmtToD3dFmt[GxTexFormats_Last];
    static EGxTexFormat s_GxTexFormatToUse[GxTexFormats_Last];
    static const D3DCUBEMAP_FACES s_faceTypes[GxTexCubeFaces_Last];
    static const D3DPRIMITIVETYPE s_primitiveConversion[GxPrims_Last];
    static const D3DBLEND s_srcBlend[12];
    static const D3DBLEND s_dstBlend[12];
    static const D3DCMPFUNC s_cmpFunc[GxCmps_Last];
    static const D3DCULL s_cullMode[3];
    static D3DTEXTUREFILTERTYPE s_filterModes[GxTexFilters_Last][3];
    static const D3DTEXTUREADDRESS s_wrapModes[2];
    static const uint32_t s_gxAttribToD3dAttribSize[7];
    static D3DDECLTYPE s_gxAttribToD3dAttribType[7];
    static const D3DDECLUSAGE s_gxAttribToD3dAttribUsage[GxVAs_Last];
    static const uint32_t s_gxAttribToD3dAttribUsageIndex[GxVAs_Last];

    static int32_t ILoadD3dLib(HINSTANCE& d3dLib, LPDIRECT3D9& d3d);
    static void IUnloadD3dLib(HINSTANCE& d3dLib, LPDIRECT3D9& d3d);
    static LRESULT CALLBACK WindowProcD3d(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    CGxDeviceD3d9Ex();
    virtual ~CGxDeviceD3d9Ex();
    void DsSet(EDeviceState state, uint32_t value);
    int32_t ICheckDepthStencilFormat(D3DFORMAT format);
    void ICreateDepthStencilSurface(uint32_t width, uint32_t height);
    void ICreateReadbackSurface();
    void IBindPixelShader(CGxShader* shader);
    void IBindVertexShader(CGxShader* shader);
    char* IBufLock(CGxBuf* buf);
    void IBufUnlock(CGxBuf* buf);
    LPDIRECT3DVERTEXDECLARATION9 ICreateD3dVertexDecl(const D3DVERTEXELEMENT9* elements, uint32_t count);
    int32_t CreatePoolAPI(CGxPool* pool);
    int32_t ICreateD3d();
    int32_t ICreateD3dDevice(const CGxFormat& format);
    LPDIRECT3DINDEXBUFFER9 ICreateD3dIB(EGxPoolUsage usage, uint32_t size);
    LPDIRECT3DVERTEXBUFFER9 ICreateD3dVB(EGxPoolUsage usage, uint32_t size);
    int32_t ICreateWindow(CGxFormat& format);
    void IDestroyD3d();
    void IReleaseD3dPool(CGxPool* pool);
    void IReleaseD3dPools(int32_t freePools);
    void IReleaseD3dQueries();
    void IReleaseD3dResources(int32_t freeTextures);
    void IReleaseD3dShaders(int32_t freeShaders);
    void IReleaseD3dVertexDecl();
    void ISceneBegin();
    void ISceneEnd();
    void ISetAlphaOp(uint32_t tmu, uint32_t op);
    void ISetCaps(const CGxFormat& format);
    void ISetColorOp(uint32_t tmu, uint32_t op);
    void ISetPresentParms(D3DPRESENT_PARAMETERS& d3dpp, const CGxFormat& format);
    void ISetMaterial(CImVector diffuse, CImVector emissive, CImVector specular, float specularExp);
    void ISetTexCoord(uint32_t tmu, uint32_t coord);
    void ISetTexCoordIndex(uint32_t tmu, int32_t texGen, uint32_t coord);
    void ISetTexGen(uint32_t tmu, int32_t texGen);
    void ISetTexture(uint32_t tmu, CGxTex* texId);
    void ISetVertexBuffer(uint32_t stream, LPDIRECT3DVERTEXBUFFER9 buffer, uint32_t offset, uint32_t stride);
    void ISetWindowFocus(int32_t focus);
    void IShaderCreatePixel(CGxShader* shader);
    void IShaderConstantsFlush();
    void IShaderCreateVertex(CGxShader* shader);
    void IStateSetD3DDefaults();
    void IStateSync();
    void IStateSyncClipPlanes();
    void IStateSyncEnables();
    void IStateSyncLights();
    void IStateSyncMaterial();
    void IStateSyncScissorRect();
    void IStateSyncVertexPtrs();
    void IStateSyncXforms();
    void ITexAcquire(CGxTex* texId);
    int32_t ITexAllocD3dTexture(CGxTex* texId);
    void ITexCreate(CGxTex* texId);
    int32_t ITexEvictFor(CGxTex* texId, uint32_t size);
    LPDIRECT3DTEXTURE9 ITexGetScratch(EGxTexFormat format);
    void ITexLinkBound();
    void ITexReleaseD3dTexture(CGxTex* texId);
    void ITexTakeD3dTexture(CGxTex* dest, CGxTex* source);
    void ITexTouch(CGxTex* texId);
    void ITexUnlinkBound();
    void ITexUploadAll(CGxTex* texId);
    void ITexUploadRect(CGxTex* texId, uint32_t face, uint32_t level, const RECT& rect, const void* bits, uint32_t pitch);
    void ITexForceRecreation(int32_t all);
    void ITexUpload(CGxTex* texId);
    void IXformSetTex(uint32_t tmu);
    void IXformSetViewport();
    void IXformSetWorld();
};

CGxDevice* CreateCGxDeviceD3d9Ex();

#endif
