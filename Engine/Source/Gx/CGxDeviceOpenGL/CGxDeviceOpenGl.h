#ifndef ENGINE_SOURCE_GX_CGXDEVICEOPENGL_CGXDEVICEOPENGL_H
#define ENGINE_SOURCE_GX_CGXDEVICEOPENGL_CGXDEVICEOPENGL_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/CGxDeviceOpenGL/GlExt.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"

class CGxDeviceOpenGl : public CGxDevice {
    public:
    enum EDeviceState {
        Ds_DepthMask = 0,
        Ds_ColorMask = 1,
        Ds_ActiveTexture = 2,
        Ds_TexTarget0 = 3,
        Ds_TexGenS0 = 11,
        Ds_TexGenT0 = 19,
        Ds_TexGenR0 = 27,
        Ds_TexGenQ0 = 35,
        Ds_ColorOp0 = 43,
        Ds_AlphaOp0 = 51,
        Ds_VertexArray0 = 59,
        Ds_TexCoordArray0 = 65,
        Ds_Reserved73 = 73,
        Ds_PolygonOffsetFill = 74,
        Ds_PolygonOffset = 75,
        Ds_Blend = 76,
        Ds_AlphaTest = 77,
        Ds_RegisterCombiners = 78,
        Ds_PerStageConstants = 79,
        Ds_TextureShader = 80,
        Ds_FragmentProgram = 81,
        Ds_VertexProgram = 82,
        Ds_VertexProgramNV = 83,
        Ds_MatrixMode = 84,
        Ds_BlendFunc = 85,
        Ds_ArrayBuffer = 86,
        Ds_ElementArrayBuffer = 87,
        Ds_CullFace = 88,
        Ds_ClipPlanes = 89,
        DeviceStates_Last = 90,
    };

    /* 3968 */ uint32_t m_deviceStates [DeviceStates_Last];
    /* 3AD0 */ uint32_t unused_3AD0;
    /* 3AD4 */ uint32_t m_materialDirty;
    /* 3AD8 */ GLenum m_sphereMapMode;
    /* 3ADC */ int32_t m_renderTargetMode;
    /* 3AE0 */ const GLubyte* m_glVendor;
    /* 3AE4 */ const GLubyte* m_glRenderer;
    /* 3AE8 */ const GLubyte* m_glVersion;
    /* 3AEC */ HWND m_hwnd;
    /* 3AF0 */ int32_t m_ownhwnd;
    /* 3AF4 */ ATOM m_hwndClass;
    /* 3AF6 */ uint16_t unused_3AF6;
    /* 3AF8 */ HDC m_hdc;
    /* 3AFC */ HGLRC m_hglrc;
    /* 3B00 */ HPBUFFERARB m_pbuffer;
    /* 3B04 */ HDC m_pbufferDC;
    /* 3B08 */ HGLRC m_pbufferRC;
    /* 3B0C */ TSFixedArray<C3Vector> m_vertexArray;
    /* 3B18 */ TSFixedArray<C3Vector> m_normalArray;
    /* 3B24 */ TSFixedArray<CImVector> m_colorArray;
    /* 3B30 */ TSFixedArray<C2Vector> m_texCoordArray0;
    /* 3B3C */ TSFixedArray<C2Vector> m_texCoordArray1;
    /* 3B48 */ uint32_t unused_3B48;
    /* 3B4C */ uint32_t unused_3B4C;
    /* 3B50 */ uint32_t unused_3B50;
    /* 3B54 */ uintptr_t m_indexPtr;
    /* 3B58 */ int32_t m_viewDirty;
    /* 3B5C */ uint32_t unused_3B5C;
    /* 3B60 */ int32_t m_useVboBroker [2][3];
    /* 3B78 */ int32_t m_useVbo [2][3];
    /* 3B90 */ GLuint m_boundTexture;
    /* 3B94 */ GLuint m_fbo;
    /* 3B98 */ GLuint m_boundFbo;
    /* 3B9C */ int32_t m_ambientOnlyMode;

    virtual void ITexMarkAsUpdated(CGxTex* texId);
    virtual void IRsSendToHw(EGxRenderState which);
    virtual ~CGxDeviceOpenGl();
    virtual int32_t DeviceCreate(GxWindowProc windowProc, const CGxFormat& format);
    virtual int32_t DeviceCreate(void* window, const CGxFormat& format);
    virtual void DeviceDestroy();
    virtual int32_t DeviceSetFormat(const CGxFormat& format);
    virtual void DeviceSetBaseMipLevel(uint32_t level);
    virtual void DeviceSetGamma(float gamma);
    virtual void DeviceSetGamma(const CGxGammaRamp& ramp);
    virtual void* DeviceWindow();
    virtual void DeviceReadPixels(CiRect& rect, TSGrowableArray<CImVector>& pixels);
    virtual void DeviceReadDepths(CiRect& rect, TSGrowableArray<float>& depths);
    virtual int32_t DeviceWM(EGxWM wm, uintptr_t param1, uintptr_t param2);
    virtual void DeviceSetRenderTarget(EGxBuffer buffer, CGxTex* texture, uint32_t plane);
    virtual void DeviceOverride(EGxOverride override, uint32_t value);
    virtual void CapsWindowSize(CRect& rect);
    virtual void CapsWindowSizeInScreenCoords(CRect& rect);
    virtual void LogCrashInfo(char* buffer, uint32_t size);
    virtual void ScenePresent();
    virtual void SceneClear(uint32_t mask, CImVector color);
    virtual void XformSetProjection(const C44Matrix& matrix);
    virtual void XformSetView(const C44Matrix& matrix);
    virtual void Draw(CGxBatch* batch, int32_t count);
    virtual void PrimPointSize(float size);
    virtual void PrimLineWidth(float width);
    virtual void PoolSizeSet(CGxPool* pool, uint32_t size);
    virtual void PoolDestroy(CGxPool* pool);
    virtual char* BufLock(CGxBuf* buf);
    virtual int32_t BufUnlock(CGxBuf* buf, uint32_t size);
    virtual void BufData(CGxBuf* buf, const void* data, uint32_t size, uint32_t offset);
    virtual void TexDestroy(CGxTex* texId);
    virtual int32_t TexCopy(CGxTex* dest, const C2iVector& destPos, const C2iVector& sourcePos, const C2iVector& size, uint32_t level, uint32_t plane);
    virtual int32_t TexStretch(CGxTex* dest, const CiRect* destRect, const CiRect* sourceRect, uint32_t level, uint32_t face);
    virtual void QueryCreate(CGxQuery*& query, EGxQueryType type);
    virtual void QueryDestroy(CGxQuery*& query);
    virtual bool QueryBegin(CGxQuery* query);
    virtual bool QueryEnd(CGxQuery* query);
    virtual bool QueryGetParam(CGxQuery* query, EGxQueryParam param, uint32_t& data);
    virtual bool QueryGetData(CGxQuery* query, uint32_t& data);
    virtual void ShaderCreate(CGxShader** shaders, EGxShTarget target, const char* path, const char* name, uint32_t permutations);
    virtual void ShaderDestroy(CGxShader*& shader);
    virtual void ShaderConstantsSet(EGxShTarget target, uint32_t index, const float* constants, uint32_t count);
    virtual void IShaderReload(CGxShader* shader, const char* path, const char* name);
    virtual void IShaderCreate(CGxShader* shader);
    virtual void StereoSetConvergence(float convergence);
    virtual float StereoGetConvergence();
    virtual void StereoSetSeparation(float separation);
    virtual float StereoGetSeparation();
    virtual bool StereoEnabled();
    virtual void DeviceUnk81(uint32_t a, uint32_t b);
    virtual void DeviceUnk82();
    virtual void DeviceUnk83(uint32_t a, uint32_t b);

    static LRESULT CALLBACK WindowProcGl(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    CGxDeviceOpenGl();

    void BindTexture(CGxTex* texId, uint32_t tmu);
    void CreatePoolAPI(CGxPool* pool);
    void DestroyPoolAPI(CGxPool* pool);
    void DeviceCreatePbuffer();
    void DeviceDestroyPbuffer();
    void DeviceDestroyPoolAPI();
    void DeviceQueryPbuffer();
    void DsInit();
    void DsSet(EDeviceState state, uint32_t value, int32_t force);
    int32_t IDevAttachGlContext(const CGxFormat& format);
    void IDevRemoveGlContext();
    void IDevSetFocus(int32_t focus, const CGxFormat& format);
    void IDeviceSetRenderTargetCopy(EGxBuffer buffer, CGxTex* texture, uint32_t plane);
    void IDeviceSetRenderTargetFBO(EGxBuffer buffer, CGxTex* texture, uint32_t plane);
    void IBindPixelShader(CGxShader* shader);
    void IBindVertexShader(CGxShader* shader);
    void IQueryRelease();
    void IPixelShaderPatch(CGxShader* shader);
    void IShaderConstantsClear();
    void IShaderConstantsSend(EGxShTarget target, uint32_t index, const float* constants, uint32_t count);
    void IShaderCreatePixel(CGxShader* shader);
    void IShaderCreateVertex(CGxShader* shader);
    void IShaderDeletePixel(CGxShader*& shader);
    void IShaderDeleteVertex(CGxShader*& shader);
    void IShaderConstantsFlush();
    void IShaderForceRecreation();
    void ISetAlphaOp(uint32_t tmu, uint32_t op);
    void ISetCaps(const CGxFormat& format);
    void ISetColorOp(uint32_t tmu, uint32_t op);
    void ISetTexGen(uint32_t tmu, int32_t texGen);
    void ISetTexture(uint32_t tmu, CGxTex* texId);
    void IStateSetContextDefaults();
    int32_t SetFormatMode(CGxFormat& format);
    void IStateSync();
    void IStateSyncClipPlanes();
    void IStateSyncEnables();
    void IStateSyncIndexPtr();
    void IStateSyncLights();
    void IStateSyncMaterial();
    void IStateSyncScissorRect();
    void IStateSyncVertexPtrs();
    void IStateSyncViewport();
    void IStateSyncXforms();
    void ITexDownload(CGxTex* texId, uint32_t width, uint32_t height, uint32_t face, uint32_t mip, uint32_t baseMip, uint32_t stride, const void* texels);
    void ITexForceRecreation();
    void ITexMarkAsUpdated(CGxTex* texId, uint32_t tmu);
    void ITexSetFlags(CGxTex* texId);
    void IXformGLModelView(const C44Matrix& view, C44Matrix& world);
    void IVertexShaderPatch(CGxShader* shader);
    void IXformSetModelView(const C44Matrix& matrix);
    void IXformSetProjection(const C44Matrix& matrix);
};

CGxDevice* CreateCGxDeviceOpenGl();

#endif
