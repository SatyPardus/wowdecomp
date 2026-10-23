#ifndef ENGINE_SOURCE_GX_GX_H
#define ENGINE_SOURCE_GX_GX_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/CGxDevice/Types.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Gx/CGxTex.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2iVector.h"

static const float oo255 = (1.0f / 255.0f);

class C2Vector;
class C3Vector;
class C44Matrix;
class CGxBatch;
class CGxDevice;
class CGxFormat;
struct CGxBuf;
class CGxQuery;
class CGxTex;
class CGxTexParms;
class CImVector;
class CiRect;
class CRect;

class GxuDrawListState {
    public:
    virtual ~GxuDrawListState();
    virtual void Begin() = 0;
    virtual void End() = 0;
};

class GxuDrawListItem {
    public:
    virtual ~GxuDrawListItem();
    virtual float GetDepth(const C3Vector& viewPos) = 0;
    virtual GxuDrawListState* GetState() = 0;
    virtual uint32_t GetVtxStream() = 0;
    virtual void Discard() = 0;
    virtual void Draw(const C3Vector& viewPos) = 0;
};

class GxDrawListEntry {
    public:
    /* 0000 */ GxuDrawListItem* m_item;
    /* 0004 */ float m_depth;
    /* 0008 */ uint32_t m_vtxStream;

    GxDrawListEntry();
};

CGxDevice* CreateCGxDeviceD3d();
CGxDevice* CreateCGxDeviceD3d9Ex();
CGxDevice* CreateCGxDeviceOpenGl();
CGxDevice* CreateGxDevice(EGxApi api, GxWindowProc windowProc, const CGxFormat& format);
int32_t GxAdapterDesktopMode(CGxMonitorMode& mode);
int32_t GxAdapterID(uint16_t& vendorID, uint16_t& deviceID, uint32_t& driverVersionHi, uint32_t& driverVersionLo);
int32_t GxAdapterInfer(uint16_t& deviceID);
int32_t GxAdapterMonitorModes(TSGrowableArray<CGxMonitorMode>& modes);
void GxReadPixels(CiRect& rect, TSGrowableArray<CImVector>& pixels);
int32_t GxVertexHasAttrib(EGxVertexBufferFormat format, EGxVertexAttrib attrib);
void GxXformSet(EGxXform xf, const C44Matrix& matrix);
char* GxBufLock(CGxBuf* buf);
void GxBufUnlock(CGxBuf* buf, uint32_t size);
void GxBufDestroy(CGxBuf* buf);
void GxRenderTargetSet(EGxBuffer buffer, CGxTex* texture, uint32_t plane);
void GxBufData(CGxBuf* buf, const void* data, uint32_t size, uint32_t offset);
CGxBuf* GxBufStream(EGxPoolTarget target, uint32_t itemSize, uint32_t itemCount);
void GxPrimIndexPtr(uint32_t indexCount, const uint16_t* indices);
void GxPrimLockVertexPtrs(uint32_t vertexCount, const C3Vector* pos, uint32_t posStride, const C3Vector* normal, uint32_t normalStride, const CImVector* color, uint32_t colorStride, const uint8_t* bone, uint32_t boneStride, const C2Vector* tex0, uint32_t tex0Stride, const C2Vector* tex1, uint32_t tex1Stride);
void GxPrimVertexPtr(CGxBuf* buf, EGxVertexBufferFormat format);
void GxPrimVertexPtr(uint32_t vertexCount, const C3Vector* pos, uint32_t posStride, const C3Vector* normal, uint32_t normalStride, const CImVector* color, uint32_t colorStride, const C2Vector* tex0, uint32_t tex0Stride, const C2Vector* tex1, uint32_t tex1Stride);
void GxPrimLockIndexPtr(EGxPrim primType, uint32_t indexCount, const uint16_t* indices);
void GxPrimUnlockVertexPtrs();
void GxTexParameters(const CGxTex* texId, CGxTexParms& parms);
void GxTexSetCannotUpdate(CGxTex* texId);
void GxTexSetDataFormat(CGxTex* texId, EGxTexFormat dataFormat);
void GxTexSetUserData(CGxTex* texId, GxTexUserFunc userFunc, void* userArg);
void GxTexUpdate(CGxTex* texId, int32_t minX, int32_t minY, int32_t maxX, int32_t maxY, int32_t immediate);
void GxTexUpdate(CGxTex* texId, CiRect& updateRect, int32_t immediate);
int32_t GxTexCreate(const CGxTexParms& parms, CGxTex*& texId);
int32_t GxTexCreate(EGxTexTarget target, uint32_t width, uint32_t height, uint32_t depth, EGxTexFormat format, EGxTexFormat dataFormat, CGxTexFlags flags, void* userArg, GxTexUserFunc userFunc, const char* name, CGxTex*& texId);
int32_t GxTexStretch(CGxTex* dest, const CiRect* destRect, const CiRect* sourceRect, uint32_t level, uint32_t face);
int32_t GxTexCreate(uint32_t width, uint32_t height, EGxTexFormat format, CGxTexFlags flags, void* userArg, GxTexUserFunc userFunc, CGxTex*& texId);
void GxXformPush(EGxXform xf, const C44Matrix& matrix);
void GxCapsWindowSizeInScreenCoords(CRect& rect);
void GxFormatColor(CImVector& color);
CImVector GxSwizzleColor(const CImVector& color);
void GxDraw(CGxBatch* batch, int32_t count);
void GxDevDestroy(CGxDevice* device);
void GxDrawLockedElements();
void GxDrawLockedElements(EGxPrim primType, uint32_t indexCount, const uint16_t* indices);
extern const char* g_gxApiNames[GxApis_Last];

BlitFormat GxGetBlitFormat(EGxTexFormat format);
uint32_t GxGetMaxFPS();
uint32_t GxGetMaxFPSBk();
int32_t GxHasContext();
void GxLog(const char* format, ...);
void GxLogClose();
void GxLogCrashInfo(char* buffer, uint32_t size);
void GxLogOpen();
int32_t GxIsWindowVisible();
int32_t GxMasterEnable(EGxMasterEnables state);
void GxMasterEnableSet(EGxMasterEnables state, int32_t enable);
bool GxQueryBegin(CGxQuery* query);
void GxQueryCreate(CGxQuery*& query, EGxQueryType type);
void GxQueryDestroy(CGxQuery*& query);
bool GxQueryEnd(CGxQuery* query);
bool GxQueryGetData(CGxQuery* query, uint32_t* data);
bool GxQueryGetParam(CGxQuery* query, EGxQueryParam param, uint32_t* data);
void GxRsSet(EGxRenderState which, void* value);
void GxRsSet(EGxRenderState which, int32_t value);
void GxRsSet(EGxRenderState which, float value);
void GxUpdateAlphaRef(EGxRenderState which);
// mask is a combination of GxBufferBit_* values.
void GxSceneClear(uint32_t mask, CImVector color);
void GxScenePresent();
void GxuClearDrawList(EGxuDrawListCategory category);
void GxuFlushDrawList(EGxuDrawListCategory category, const C3Vector& viewPos);
void GxuSortDrawList(EGxuDrawListCategory category, EGxuDrawListSorting sorting, const C3Vector& viewPos);
void GxTexSetFlags(CGxTex* texId, CGxTexFlags flags);
void GxTexSetWrap(CGxTex* texId, EGxTexWrapMode wrapU, EGxTexWrapMode wrapV);
void GxSetMaxFPS(uint32_t maxFPS);
void GxSetMaxFPSBk(uint32_t maxFPSBk);
void GxShaderConstantsSet(EGxShTarget target, uint32_t index, const float* constants, uint32_t count);
int32_t GxTexCopy(CGxTex* dest, const C2iVector& destPos, const C2iVector& sourcePos, const C2iVector& size, uint32_t level, uint32_t plane);
void GxTexDestroy(CGxTex* texId);
int32_t GxTexReusable(CGxTexParms& parms);
int32_t GxTexReusable(CGxTex* texId);
void GxXformPop(EGxXform xf);
void GxXformProjection(C44Matrix& matrix);
void GxXformPush(EGxXform xf);
void GxXformScale(EGxXform xf, const C3Vector& scale);
void GxXformSetProjection(const C44Matrix& matrix);
void GxXformSetView(const C44Matrix& matrix);
void GxXformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);
void GxXformTranslate(EGxXform xf, const C3Vector& move);
void GxXformView(C44Matrix& matrix);
void GxXformViewport(float& minX, float& maxX, float& minY, float& maxY, float& minZ, float& maxZ);
void GxXformViewProj(C44Matrix& matrix);
void GxXformViewProjNativeTranspose(C44Matrix& matrix);
int32_t GxApiSupported(EGxApi api);
EGxApi GxDefaultApi();
uint32_t GxCalcTexelStrideInBytes(EGxTexFormat format, uint32_t width);
uint32_t GxVertexAttribOffset(EGxVertexBufferFormat format, EGxVertexAttrib attrib);
uint32_t GxVertexSize(EGxVertexBufferFormat format);

#endif
