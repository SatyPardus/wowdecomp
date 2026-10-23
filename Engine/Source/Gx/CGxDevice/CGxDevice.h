#ifndef ENGINE_SOURCE_GX_CGXDEVICE_CGXDEVICE_H
#define ENGINE_SOURCE_GX_CGXDEVICE_CGXDEVICE_H

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxShader.h"
#include "Engine/Source/Gx/CGxDevice/EmergencyMem.h"
#include "Engine/Source/Gx/CGxDevice/Types.h"
#include "Engine/Source/Gx/CGxFormat.h"
#include "Engine/Source/Gx/CGxGammaRamp.h"
#include "Engine/Source/Gx/CGxLight.h"
#include "Engine/Source/Gx/CGxMatrixStack.h"
#include "Engine/Source/Gx/CGxTex.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Storm/H/Types.h"
#include "Engine/Source/Tempest/C2iVector.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Plane.h"
#include "Engine/Source/Tempest/C4Vector.h"
#include "Engine/Source/Tempest/CBoundingBox.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CiRect.h"
#include "Engine/Source/Tempest/CRect.h"

struct CGxAppRenderState;
struct CGxBuf;
struct CGxPool;
struct tagRECT;
class CGxQuery;
class CGxStateBom;
class CGxTex;
class CImVector;

class ShadowConstants {
    public:
    /* 0000 */ float constants[256 * 4];
    /* 1000 */ uint32_t dirtyMax;
    /* 1004 */ uint32_t dirtyMin;
};

class CGxStateBom {
    public:
    /* 0000 */ union {
        int32_t i[3];
        uint32_t u[3];
        float f[3];
        void* p;
    } m_data;
    /* 000C */ int32_t m_filler;
};

struct CGxPushedRenderState {
    /* 0000 */ EGxRenderState m_which;
    /* 0004 */ CGxStateBom m_value;
    /* 0014 */ uint32_t m_stackDepth;
};

struct CGxAppRenderState {
    /* 0000 */ CGxStateBom m_value;
    /* 0010 */ uint32_t m_stackDepth;
    /* 0014 */ int32_t m_dirty;
};

typedef void (__fastcall* GxDeviceCallback)();

class CGxDevice {
    public:
    struct TextureTarget {
        /* 0000 */ CGxTex* m_texture;
        /* 0004 */ uint32_t m_plane;
        /* 0008 */ void* m_apiSpecific;
    };

    /* 0004 */ TSGrowableArray<CGxPushedRenderState> m_pushedStates;
    /* 0014 */ TSGrowableArray<uint32_t> m_stackOffsets;
    /* 0024 */ TSGrowableArray<EGxRenderState> m_dirtyStates;
    /* 0034 */ EGxPrim m_primType;
    /* 0038 */ int32_t m_indexLocked;
    /* 003C */ int32_t m_vertexLocked;
    /* 0040 */ int32_t m_inBeginEnd;
    /* 0044 */ C3Vector m_primVertex;
    /* 0050 */ C2Vector m_primTexCoord [8];
    /* 0090 */ C3Vector m_primNormal;
    /* 009C */ CImVector m_primColor;
    /* 00A0 */ TSGrowableArray<C3Vector> m_primVertexArray;
    /* 00B0 */ TSGrowableArray<C2Vector> m_primTexCoordArray [8];
    /* 0130 */ TSGrowableArray<C3Vector> m_primNormalArray;
    /* 0140 */ TSGrowableArray<CImVector> m_primColorArray;
    /* 0150 */ TSGrowableArray<uint16_t> m_primIndexArray;
    /* 0160 */ uint32_t m_primMask;
    /* 0164 */ CRect m_defWindowRect;
    /* 0174 */ CRect m_curWindowRect;
    /* 0184 */ TSGrowableArray<GxDeviceCallback> m_deviceRestoredCallbacks;
    /* 0194 */ TSGrowableArray<GxDeviceCallback> m_textureRecreationCallbacks;
    /* 01A4 */ TSGrowableArray<GxDeviceCallback> m_stereoChangedCallbacks;
    /* 01B4 */ EGxApi m_api;
    /* 01B8 */ uint32_t m_cpuFeatures;
    /* 01BC */ CGxFormat m_format;
    /* 0214 */ CGxCaps m_caps;
    /* 0350 */ uint32_t m_baseMipLevel;
    /* 0354 */ CGxGammaRamp m_gammaRamp;
    /* 0954 */ CGxGammaRamp m_systemGammaRamp;
    /* 0F54 */ GxWindowProc m_windowProc;
    /* 0F58 */ int32_t m_context;
    /* 0F5C */ int32_t m_intF5C;
    /* 0F60 */ int32_t m_windowVisible;
    /* 0F64 */ int32_t m_windowFocus;
    /* 0F68 */ int32_t m_frameCount;
    /* 0F6C */ int32_t m_viewportDirty;
    /* 0F70 */ CBoundingBox m_viewport;
    /* 0F88 */ C44Matrix m_projection;
    /* 0FC8 */ C44Matrix m_projNative;
    /* 1008 */ CGxMatrixStack m_xforms [GxXforms_Last];
    /* 1C10 */ CGxMatrixStack m_texGen [8];
    /* 24D0 */ uint32_t m_clipPlaneMask;
    /* 24D4 */ C4Plane m_clipPlane [6];
    /* 2534 */ int32_t m_scissorRectDirty;
    /* 2538 */ CRect m_scissorRect;
    /* 2548 */ CGxApiLight m_lights [4];
    /* 2668 */ TSHashTable<CGxShader, HASHKEY_STRI> m_shaderList [6];
    /* 2758 */ uint32_t m_appMasterEnables;
    /* 275C */ uint32_t m_hwMasterEnables;
    /* 2760 */ TSList<CGxPool> m_poolList;
    /* 276C */ TSList<CGxBuf> m_bufList;
    /* 2778 */ CGxBuf* m_bufLocked [2];
    /* 2780 */ CGxPool* m_vertexPool;
    /* 2784 */ CGxPool* m_indexPool;
    /* 2788 */ CGxBuf* m_streamBufs [2];
    /* 2790 */ CGxVertexAttrib m_primVertexFormatAttrib [14];
    /* 2870 */ CGxBuf* m_primVertexFormatBuf [14];
    /* 28A8 */ uint32_t m_primVertexMask;
    /* 28AC */ uint32_t m_primVertexDirty;
    /* 28B0 */ EGxVertexBufferFormat m_primVertexFormat;
    /* 28B4 */ CGxBuf* m_primVertexBuf;
    /* 28B8 */ uint32_t m_primVertexSize;
    /* 28BC */ CGxBuf* m_primIndexBuf;
    /* 28C0 */ int32_t m_primIndexDirty;
    /* 28C4 */ EmergencyMem m_emergencyMem [2];
    /* 28EC */ TSFixedArray<CGxAppRenderState> m_appRenderStates;
    /* 28F8 */ TSFixedArray<CGxStateBom> m_hwRenderStates;
    /* 2904 */ TSExplicitList<CGxTex, offsetof(CGxTex, m_link)> m_textures;
    /* 2910 */ CGxDevice::TextureTarget m_textureTarget [2];
    /* 2928 */ TSExplicitList<CGxQuery, offsetof(CGxQuery, m_link)> m_queryList;
    /* 2934 */ int32_t m_scrShotClick;
    /* 2938 */ uint32_t m_scrShotWidth;
    /* 293C */ uint32_t m_scrShotHeight;
    /* 2940 */ TSGrowableArray<CImVector> m_scrShotPixels;
    /* 2950 */ int32_t m_cursorVisible;
    /* 2954 */ int32_t m_hwCursor;
    /* 2958 */ uint32_t m_cursorHotspotX;
    /* 295C */ uint32_t m_cursorHotspotY;
    /* 2960 */ uint32_t m_cursor [1024];
    /* 3960 */ CGxTex* m_cursorTexture;
    /* 3964 */ float m_cursorDepth;

    virtual void ITexMarkAsUpdated(CGxTex* texId);
    virtual void IRsSendToHw(EGxRenderState which) = 0;
    virtual void ICursorCreate(const CGxFormat& format);
    virtual void ICursorDestroy();
    virtual void ICursorDraw();
    virtual void NotifyOnDeviceRestored();
    virtual void NotifyOnTextureRecreation();
    virtual void NotifyOnStereoChanged();
    virtual ~CGxDevice();
    virtual int32_t DeviceCreate(GxWindowProc windowProc, const CGxFormat& format);
    virtual int32_t DeviceCreate(void* window, const CGxFormat& format);
    virtual void DeviceDestroy();
    virtual void DeviceEvictResources();
    virtual int32_t DeviceSetFormat(const CGxFormat& format);
    virtual void DeviceSetBaseMipLevel(uint32_t level);
    virtual void DeviceSetGamma(float gamma);
    virtual void DeviceSetGamma(const CGxGammaRamp& ramp);
    virtual void* DeviceWindow() = 0;
    virtual void DeviceTakeScreenShot();
    virtual void DeviceReadScreenShot(uint32_t& width, uint32_t& height, const CImVector*& pixels);
    virtual void DeviceReadPixels(CiRect& rect, TSGrowableArray<CImVector>& pixels) = 0;
    virtual void DeviceReadDepths(CiRect& rect, TSGrowableArray<float>& depths) = 0;
    virtual int32_t DeviceWM(EGxWM wm, uintptr_t param1, uintptr_t param2) = 0;
    virtual void DeviceSetRenderTarget(EGxBuffer buffer, CGxTex* texture, uint32_t plane);
    virtual void DeviceSaveDepthSurface();
    virtual void DeviceRestoreDepthSurface();
    virtual void DeviceResolveDepthBuffer(CGxTex* texture);
    virtual void DeviceCopyTex(CGxTex* source, uint32_t sourcePlane, CGxTex* dest, uint32_t destPlane);
    virtual void DeviceOverride(EGxOverride override, uint32_t value);
    virtual void AddDeviceRestoredCallback(GxDeviceCallback callback);
    virtual void RemoveDeviceRestoredCallback(GxDeviceCallback callback);
    virtual void AddTextureRecreationCallback(GxDeviceCallback callback);
    virtual void RemoveTextureRecreationCallback(GxDeviceCallback callback);
    virtual void AddStereoChangedCallback(GxDeviceCallback callback);
    virtual void RemoveStereoChangedCallback(GxDeviceCallback callback);
    virtual void CapsWindowSize(CRect& rect) = 0;
    virtual void CapsWindowSizeInScreenCoords(CRect& rect) = 0;
    virtual void LogCrashInfo(char* buffer, uint32_t size);
    virtual void ScenePresent();
    virtual void SceneClear(uint32_t mask, CImVector color);
    virtual void XformSetProjection(const C44Matrix& matrix);
    virtual void XformSetView(const C44Matrix& matrix);
    virtual void Draw(CGxBatch* batch, int32_t count);
    virtual void PrimBegin(EGxPrim primType);
    virtual void PrimDrawElements();
    virtual void PrimVertex(const C3Vector& v);
    virtual void PrimTexCoord(uint32_t tmu, const C2Vector& t);
    virtual void PrimNormal(const C3Vector& n);
    virtual void PrimColor(const CImVector& c);
    virtual void PrimPointSize(float size);
    virtual void PrimLineWidth(float width);
    virtual void MasterEnableSet(EGxMasterEnables state, int32_t enable);
    virtual void PoolSizeSet(CGxPool* pool, uint32_t size) = 0;
    virtual void PoolDestroy(CGxPool* pool);
    virtual char* BufLock(CGxBuf* buf);
    virtual int32_t BufUnlock(CGxBuf* buf, uint32_t size);
    virtual void BufData(CGxBuf* buf, const void* data, uint32_t size, uint32_t offset);
    virtual int32_t TexCreate(EGxTexTarget target, uint32_t width, uint32_t height, uint32_t depth, EGxTexFormat format, EGxTexFormat dataFormat, CGxTexFlags flags, void* userArg, GxTexUserFunc userFunc, const char* name, CGxTex*& texId);
    virtual void TexDestroy(CGxTex* texId);
    virtual int32_t TexCopy(CGxTex* dest, const C2iVector& destPos, const C2iVector& sourcePos, const C2iVector& size, uint32_t level, uint32_t plane) = 0;
    virtual int32_t TexStretch(CGxTex* dest, const CiRect* destRect, const CiRect* sourceRect, uint32_t level, uint32_t face) = 0;
    virtual void TexSetCacheSize(uint32_t size);
    virtual void QueryCreate(CGxQuery*& query, EGxQueryType type);
    virtual void QueryDestroy(CGxQuery*& query);
    virtual bool QueryBegin(CGxQuery* query) = 0;
    virtual bool QueryEnd(CGxQuery* query) = 0;
    virtual bool QueryGetParam(CGxQuery* query, EGxQueryParam param, uint32_t& data) = 0;
    virtual bool QueryGetData(CGxQuery* query, uint32_t& data) = 0;
    virtual void ShaderCreate(CGxShader** shaders, EGxShTarget target, const char* path, const char* name, uint32_t permutations);
    virtual void ShaderDestroy(CGxShader*& shader);
    virtual void ShaderConstantsSet(EGxShTarget target, uint32_t index, const float* constants, uint32_t count);
    virtual void IShaderReload(CGxShader* shader, const char* path, const char* name) = 0;
    virtual void IShaderCreate(CGxShader* shader) = 0;
    virtual void CursorSetVisible(int32_t visible);
    virtual uint32_t* CursorLock();
    virtual void CursorUnlock(uint32_t x, uint32_t y);
    virtual void StereoSetConvergence(float convergence) = 0;
    virtual float StereoGetConvergence() = 0;
    virtual void StereoSetSeparation(float separation) = 0;
    virtual float StereoGetSeparation() = 0;
    virtual bool StereoEnabled() = 0;
    virtual void DeviceUnk81(uint32_t a, uint32_t b) = 0;
    virtual void DeviceUnk82() = 0;
    virtual void DeviceUnk83(uint32_t a, uint32_t b) = 0;

    static HSLOG m_log;
    static C2iVector s_clientAdjust;
    static const char* s_pixelShaderTargetNames[14];
    static const char* s_vertexShaderTargetNames[12];
    static C3Vector s_pointScaleAttenuation;
    static CGxShader* s_uiPixelShader;
    static CGxShader* s_uiVertexShader[2];
    static ShadowConstants s_shadowConstants[2];
    static uint32_t s_alphaRef[12];
    static const uint32_t s_texFormatBitDepth[GxTexFormats_Last];
    static const uint32_t s_texFormatBlockSize[GxTexFormats_Last];
    static const uint32_t s_texFormatBytesPerBlock[GxTexFormats_Last];
    static uint32_t m_logBytes;

    static int32_t AdapterDesktopMode(CGxMonitorMode& mode);
    static int32_t AdapterFormats(EGxApi api, TSGrowableArray<CGxFormat>& formats);
    static int32_t AdapterMonitorModes(TSGrowableArray<CGxMonitorMode>& modes);
    static int32_t AdjustWindowRectToWorkArea(float aspectRatio, C2iVector* adjust, tagRECT* rect);
    static int32_t D3d9ExAdapterFormats(TSGrowableArray<CGxFormat>& formats);
    static int32_t D3dAdapterFormats(TSGrowableArray<CGxFormat>& formats);
    static int32_t DeviceAdapterID(uint16_t& vendorID, uint16_t& deviceID, uint32_t& driverVersionHi, uint32_t& driverVersionLo);
    static int32_t DeviceAdapterInfer(uint16_t& deviceID);
    static int32_t FormatExists(TSGrowableArray<CGxFormat>& formats, const CGxFormat& format);
    static int32_t OpenGlAdapterFormats(TSGrowableArray<CGxFormat>& formats);
    static void ClampRectBottom(CiRect& windowRect);
    static void ClampRectBottomByAspect(CiRect& windowRect);
    static void ClampRectLeftByAspect(CiRect& windowRect);
    static void ClampRectRightByAspect(CiRect& windowRect);
    static void ClampRectTopByAspect(CiRect& windowRect);
    static void Log(const char* format, ...);
    static void Log(const char* format, va_list args);
    static void LogClose();
    static void LogOpen();
    static void ClampRectLeft(CiRect& windowRect);
    static void ClampRectRight(CiRect& windowRect);
    static void ClampRectTop(CiRect& windowRect);
    static void ICursorUpdate(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels);

    CGxDevice();
    CGxBuf* BufCreate(CGxPool* pool, uint32_t itemSize, uint32_t itemCount, uint32_t index);
    void BufDestroy(CGxBuf*& buf);
    void DeviceCreatePools();
    void DeviceCreateStreamBufs();
    void IShaderLoad(CGxShader** shaders, EGxShTarget target, const char* path, const char* name, uint32_t permutations);
    CGxPool* PoolCreate(EGxPoolTarget target, EGxPoolUsage usage, int32_t size, EGxPoolHintBits hint, const char* name);
    CGxBuf* BufStream(EGxPoolTarget target, uint32_t itemSize, uint32_t itemCount);
    void BufSizeSet(CGxBuf* buf, uint32_t itemSize, uint32_t itemCount);
    CGxCaps& Caps();
    int32_t CapsHasContext(int32_t context);
    int32_t CapsIsWindowVisible(int32_t visible);
    int32_t CapsWindowHasFocus(int32_t focus);
    void ClampRectToWindow(CiRect& rect);
    void Log(const CGxCaps& caps) const;
    void Log(const CGxFormat& format) const;
    void IRsDirty(EGxRenderState which);
    void IRsForceUpdate();
    void IRsForceUpdate(EGxRenderState which);
    void IRsInit();
    void IRsSync(int32_t force);
    void RsPop();
    void RsInit();
    void RsPush();
    void RsSet(EGxRenderState which, void* value);
    void RsSet(EGxRenderState which, int32_t value);
    void RsSet(EGxRenderState which, float value);
    void RsSet(EGxRenderState which, const C3Vector& value);
    void TexSetFlags(CGxTex* texId, CGxTexFlags flags);
    void TexSetWrap(CGxTex* texId, EGxTexWrapMode wrapU, EGxTexWrapMode wrapV);
    void ClipPlaneSet(uint32_t index, const C4Plane& plane);
    void CursorSetDepth(float depth);
    EGxApi DeviceApi();
    uint32_t DeviceBaseMipLevel();
    void DeviceClearScreenShot();
    const CRect& DeviceCurWindow();
    const CRect& DeviceDefWindow();
    void DeviceGamma(CGxGammaRamp& ramp);
    void DeviceGetRenderTarget(EGxBuffer buffer, CGxTex*& texture);
    void DeviceScreenShot();
    void DeviceSetCurWindow(const CRect& rect);
    void DeviceSetDefWindow(const CRect& rect);
    void GetSurveyModeInfo(uint32_t& desktopWidth, uint32_t& desktopHeight, uint32_t& desktopBpp, uint32_t& width, uint32_t& height, uint32_t& bpp);
    int32_t IDevIsWindowed();
    void InitLights();
    uint32_t ITexBitsSize(CGxTex* texId, uint32_t width, uint32_t height);
    void ITexWHDStartEnd(CGxTex* texId, uint32_t& width, uint32_t& height, uint32_t& baseMip, uint32_t& mipCount);
    void Light(uint32_t index, CGxLight& light);
    void LightEnable(uint32_t index, int32_t enable);
    void LightSet(uint32_t index, const CGxLight& light, const C3Vector& pos);
    int32_t MasterEnable(EGxMasterEnables state);
    int32_t NeedsUpdate(uint32_t app, uint32_t hw, uint32_t appDisables, uint32_t hwDisables, uint32_t flagPos, int32_t& enable);
    uint32_t PrimCalcCount(EGxPrim primType, uint32_t indexCount);
    void PrimIndexPtr(CGxBuf* buf);
    void PrimVertexFormat(CGxBuf* buf, const CGxVertexAttrib* attribs, uint32_t count);
    void PrimVertexMask(uint32_t mask);
    void PrimVertexPtr(CGxBuf* buf, EGxVertexBufferFormat format);
    void SetScissor(const CRect& rect);
    void ShaderConstantsClear();
    float* ShaderConstantsLock(EGxShTarget target);
    void ShaderConstantsUnlock(EGxShTarget target, uint32_t index, uint32_t count);
    int32_t TexCanUpdate(CGxTex* texId);
    int32_t TexIsReadyNow(CGxTex* texId);
    void TexMarkForUpdate(CGxTex* texId, const CiRect& updateRect, int32_t immediate);
    void TexParameters(const CGxTex* texId, CGxTexParms& parms);
    void TexSetCannotUpdate(CGxTex* texId);
    void TexSetDataFormat(CGxTex* texId, EGxTexFormat dataFormat);
    void TexSetUserData(CGxTex* texId, GxTexUserFunc userFunc, void* userArg);
    void WaitForFPSCap();
    void XformPush(EGxXform xf);
    void XformPushAndSet(EGxXform xf, const C44Matrix& matrix);
    void XformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);
};

extern CGxDevice* g_theGxDevicePtr;
extern const char* s_formatToInt[CGxFormat::Formats_Last];

#endif
