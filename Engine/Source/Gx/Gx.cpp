#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include "Engine/Source/Gx/Buffer.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00A2D57C
static const uint32_t s_texelSize[GxTexFormats_Last] = { 0, 4, 4, 2, 2, 2, 2, 4, 4, 2, 4, 4, 4 };

// STATIC: 0x00A2D5B0
static const int32_t s_vertexAttribOffset[GxVertexBufferFormats_Last][GxVAs_Last] = {
    { 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 0, -1, -1, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 0, -1, -1, 12, 24, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 0, -1, -1, 12, -1, -1, 24, -1, -1, -1, -1, -1, -1, -1 },
    { 0, -1, -1, 12, 24, -1, 28, -1, -1, -1, -1, -1, -1, -1 },
    { 0, -1, -1, 12, -1, -1, 24, 32, -1, -1, -1, -1, -1, -1 },
    { 0, -1, -1, 12, 24, -1, 28, 36, -1, -1, -1, -1, -1, -1 },
    { 0, -1, -1, -1, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 0, -1, -1, -1, 12, -1, 16, -1, -1, -1, -1, -1, -1, -1 },
    { 0, -1, -1, -1, 12, -1, 16, 24, -1, -1, -1, -1, -1, -1 },
    { 0, -1, -1, -1, -1, -1, 12, -1, -1, -1, -1, -1, -1, -1 },
    { 0, -1, -1, -1, -1, -1, 12, 20, -1, -1, -1, -1, -1, -1 },
    { 0, 12, 16, 20, -1, -1, 32, 40, -1, -1, -1, -1, -1, -1 },
    { 0, -1, -1, 12, 24, 28, 32, 40, -1, -1, -1, -1, -1, -1 }
};

// STATIC: 0x00AD8AF8
static BlitFormat s_blitFormats[GxTexFormats_Last] = {
    BlitFormat_Unknown, BlitFormat_Abgr8888, BlitFormat_Argb8888, BlitFormat_Argb4444, BlitFormat_Argb1555,
    BlitFormat_Rgb565, BlitFormat_Dxt1, BlitFormat_Dxt3, BlitFormat_Dxt5, BlitFormat_Uv88, BlitFormat_Gr1616F,
    BlitFormat_R32F, BlitFormat_D24X8
};

// STATIC: 0x00AD87E4
const char* g_gxApiNames[GxApis_Last] = { "OpenGL", "D3D9", "D3D9Ex", "D3D10", "D3D11", "GLL" };

// STATIC: 0x00AD8AC0
static uint32_t s_vertexSize[GxVertexBufferFormats_Last] = { 12, 24, 28, 32, 36, 40, 44, 16, 24, 32, 20, 28, 48, 48 };

// STATIC: 0x00C5DF74
static uint32_t s_maxFPSBk;

// STATIC: 0x00C5DF78
static EGxPrim s_lockPrimType;

// STATIC: 0x00C5DF7C
static uint32_t s_maxFPS;

// STATIC: 0x00C5DF80
static uint32_t s_lockIndexCount;

// STATIC: 0x00C5DF84
static uint32_t s_lockVertexCount;

// STATIC: 0x00C5DF8C
static bool s_drawListBusy;

// STATIC: 0x00C5DF90
static TSGrowableArray<void (*)()> s_destroyCallbacks;

// STATIC: 0x00C5DFA4
static TSGrowableArray<GxDrawListEntry> s_drawLists[GxuCats_Last];

// VERIFY: 39e42562 auto
// OFFSET: 0x00681310
static void DrawListPreprocessB2F(GxDrawListEntry* entry, const C3Vector& viewPos) {
    entry->m_depth = entry->m_item->GetDepth(viewPos);
}

// VERIFY: b2b7569f auto
// OFFSET: 0x00681330
static int DrawListSorterB2F(const void* a, const void* b) {
    float depthA = static_cast<const GxDrawListEntry*>(a)->m_depth;
    float depthB = static_cast<const GxDrawListEntry*>(b)->m_depth;

    if (depthB < depthA) {
        return -1;
    }

    return depthB > depthA;
}

// VERIFY: 78c775d6 auto
// OFFSET: 0x00681370
static void DrawListPreprocessVtxStream(GxDrawListEntry* entry, const C3Vector& viewPos) {
    entry->m_vtxStream = entry->m_item->GetVtxStream();
}

// VERIFY: 1935d204 auto
// OFFSET: 0x00681390
static int DrawListSorterVtxStream(const void* a, const void* b) {
    uint32_t streamA = static_cast<const GxDrawListEntry*>(a)->m_vtxStream;
    uint32_t streamB = static_cast<const GxDrawListEntry*>(b)->m_vtxStream;

    if (streamB > streamA) {
        return -1;
    }

    return streamB < streamA;
}

// STATIC: 0x00AD88A8
static void (*s_drawListPreprocess[GxuSorts_Last])(GxDrawListEntry*, const C3Vector&) = { DrawListPreprocessB2F, DrawListPreprocessVtxStream };

// STATIC: 0x00AD88B0
static int (*s_drawListSorters[GxuSorts_Last])(const void*, const void*) = { DrawListSorterB2F, DrawListSorterVtxStream };

// VERIFY: 0b88acbc auto
// OFFSET: 0x008C8DE0
EGxApi GxDefaultApi() {
#if defined(_WIN32)
    return GxApi_D3d9;
#else
    return GxApi_OpenGl;
#endif
}

// VERIFY: cef2b995 auto
// OFFSET: 0x006811D0
int32_t GxApiSupported(EGxApi api) {
    switch (api) {
        case GxApi_OpenGl:
#if defined(_WIN32)
        case GxApi_D3d9:
        case GxApi_D3d9Ex:
#endif
            return 1;

        default:
            return 0;
    }
}

// VERIFY: 619cba02 auto
// OFFSET: 0x00681230
uint32_t GxVertexSize(EGxVertexBufferFormat format) {
    return s_vertexSize[format];
}

// VERIFY: f1ff254c auto
// OFFSET: 0x006811F0
int32_t GxAdapterID(uint16_t& vendorID, uint16_t& deviceID, uint32_t& driverVersionHi, uint32_t& driverVersionLo) {
    return CGxDevice::DeviceAdapterID(vendorID, deviceID, driverVersionHi, driverVersionLo);
}

// VERIFY: d8dc86c0 auto
// OFFSET: 0x00681200
int32_t GxAdapterInfer(uint16_t& deviceID) {
    return CGxDevice::DeviceAdapterInfer(deviceID);
}

// VERIFY: 033ca462 auto
// OFFSET: 0x00681210
int32_t GxAdapterMonitorModes(TSGrowableArray<CGxMonitorMode>& modes) {
    return CGxDevice::AdapterMonitorModes(modes);
}

// VERIFY: 6be0031b auto
// OFFSET: 0x00681220
int32_t GxAdapterDesktopMode(CGxMonitorMode& mode) {
    return CGxDevice::AdapterDesktopMode(mode);
}

// VERIFY: d108cfe2 auto
// OFFSET: 0x00681240
uint32_t GxVertexAttribOffset(EGxVertexBufferFormat format, EGxVertexAttrib attrib) {
    return s_vertexAttribOffset[format][attrib];
}

// VERIFY: 6233e69b auto
// OFFSET: 0x00681260
int32_t GxVertexHasAttrib(EGxVertexBufferFormat format, EGxVertexAttrib attrib) {
    return s_vertexAttribOffset[format][attrib] != -1;
}

// VERIFY: ca53a8f5 auto
// OFFSET: 0x00681290
CGxDevice* CreateGxDevice(EGxApi api, GxWindowProc windowProc, const CGxFormat& format) {
    CGxDevice* device;

    switch (api) {
        case GxApi_OpenGl:
            device = CreateCGxDeviceOpenGl();
            g_theGxDevicePtr = device;
            break;

        case GxApi_D3d9:
#if defined(_WIN32)
            device = CreateCGxDeviceD3d();
            g_theGxDevicePtr = device;
            break;
#else
            device = CreateCGxDeviceOpenGl();
            g_theGxDevicePtr = device;
            break;
#endif

        case GxApi_D3d9Ex:
#if defined(_WIN32)
            device = CreateCGxDeviceD3d9Ex();
            g_theGxDevicePtr = device;
            break;
#else
            device = CreateCGxDeviceOpenGl();
            g_theGxDevicePtr = device;
            break;
#endif

        default:
            device = g_theGxDevicePtr;
            break;
    }

    if (device->DeviceCreate(windowProc, format)) {
        return g_theGxDevicePtr;
    }

    if (g_theGxDevicePtr) {
        delete g_theGxDevicePtr;
    }

    return nullptr;
}

// VERIFY: 930fa88b manual
// OFFSET: 0x006815F0
uint32_t GxCalcTexelStrideInBytes(EGxTexFormat format, uint32_t width) {
    if (format < GxTex_Dxt1 || format > GxTex_Dxt5) {
        return s_texelSize[format] * width;
    }

    return s_texelSize[format] * ((width + 3) & ~3u);
}

// VERIFY: 1ff77454 manual
// OFFSET: 0x00408030
void GxXformSetProjection(const C44Matrix& matrix) {
    g_theGxDevicePtr->XformSetProjection(matrix);
}

// VERIFY: d15ab073 manual
// OFFSET: 0x00408050
void GxXformSetView(const C44Matrix& matrix) {
    g_theGxDevicePtr->XformSetView(matrix);
}

// VERIFY: fb2b6e6c auto
// OFFSET: 0x00408070
void GxXformViewport(float& minX, float& maxX, float& minY, float& maxY, float& minZ, float& maxZ) {
    minX = g_theGxDevicePtr->m_viewport.m_x.m_l;
    maxX = g_theGxDevicePtr->m_viewport.m_x.m_h;
    minY = g_theGxDevicePtr->m_viewport.m_y.m_l;
    maxY = g_theGxDevicePtr->m_viewport.m_y.m_h;
    minZ = g_theGxDevicePtr->m_viewport.m_z.m_l;
    maxZ = g_theGxDevicePtr->m_viewport.m_z.m_h;
}

// VERIFY: 346b4596 auto
// OFFSET: 0x004080E0
void GxXformView(C44Matrix& matrix) {
    CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[GxXform_View];
    matrix = stack.m_mtx[stack.m_level];
}

// VERIFY: 30d43d0d manual
// OFFSET: 0x00408110
void GxXformViewProjNativeTranspose(C44Matrix& matrix) {
    CGxDevice* device = g_theGxDevicePtr;
    C44Matrix view;
    C44Matrix proj;

    view = device->m_xforms[GxXform_View].m_mtx[device->m_xforms[GxXform_View].m_level];
    proj = device->m_projNative;

    if (device->DeviceApi() == GxApi_OpenGl) {
        proj.c0 = proj.c0 * -1.0f;
        proj.c1 = proj.c1 * -1.0f;
        proj.c2 = proj.c2 * -1.0f;
        proj.c3 = -1.0f * proj.c3;
    }

    matrix = (view * proj).Transpose();
}

// VERIFY: 45fd1052 manual
// OFFSET: 0x00408210
void GxShaderConstantsSet(EGxShTarget target, uint32_t index, const float* constants, uint32_t count) {
    if (constants) {
        g_theGxDevicePtr->ShaderConstantsSet(target, index, constants, count);
    }
}

// VERIFY: 2d7b5b76 manual
// OFFSET: 0x004829D0
char* GxBufLock(CGxBuf* buf) {
    return g_theGxDevicePtr->BufLock(buf);
}

// VERIFY: d2aedfb6 manual
// OFFSET: 0x004829F0
void GxBufUnlock(CGxBuf* buf, uint32_t size) {
    g_theGxDevicePtr->BufUnlock(buf, size);
    buf->m_hasData = 1;
}

// VERIFY: 13b86081 manual
// OFFSET: 0x00482A40
void GxDraw(CGxBatch* batch, int32_t count) {
    g_theGxDevicePtr->Draw(batch, count);
}

// VERIFY: 24d4e729 manual
// OFFSET: 0x00493BF0
void GxCapsWindowSizeInScreenCoords(CRect& rect) {
    g_theGxDevicePtr->CapsWindowSizeInScreenCoords(rect);
}

// VERIFY: d8579ed1 auto
// OFFSET: 0x004BEC70
void GxXformProjection(C44Matrix& matrix) {
    matrix = g_theGxDevicePtr->m_projection;
}

// VERIFY: 8bf3caad auto
// OFFSET: 0x0057C400
void GxXformPush(EGxXform xf) {
    g_theGxDevicePtr->XformPush(xf);
}

// VERIFY: 42304661 manual
// OFFSET: 0x0057C420
void GxXformPop(EGxXform xf) {
    CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[xf];
    stack.m_dirty = 1;

    if (stack.m_level) {
        stack.m_level--;
    }
}

// VERIFY: 25db6ccb manual
// OFFSET: 0x0057C4A0
void GxXformTranslate(EGxXform xf, const C3Vector& move) {
    CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[xf];
    stack.m_dirty = 1;
    stack.m_flags[stack.m_level] &= ~CGxMatrixStack::F_Identity;
    stack.m_mtx[stack.m_level].Translate(move);
}

// VERIFY: b3c95c9e manual
// OFFSET: 0x0057C4F0
void GxXformScale(EGxXform xf, const C3Vector& scale) {
    CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[xf];
    stack.m_dirty = 1;
    stack.m_flags[stack.m_level] &= ~CGxMatrixStack::F_Identity;
    stack.m_mtx[stack.m_level].Scale(scale);
}

// VERIFY: cd7c4ef1 manual
// OFFSET: 0x00408010
int32_t GxHasContext() {
    return g_theGxDevicePtr ? g_theGxDevicePtr->CapsHasContext(-1) : 0;
}

// VERIFY: bd3d99bd manual
// OFFSET: 0x00407FF0
int32_t GxIsWindowVisible() {
    return g_theGxDevicePtr ? g_theGxDevicePtr->CapsIsWindowVisible(-1) : 0;
}

// VERIFY: 4ed7a557 auto
// OFFSET: 0x00616AF0
int32_t GxMasterEnable(EGxMasterEnables state) {
    return g_theGxDevicePtr->MasterEnable(state);
}

// VERIFY: cdcd566a auto
// OFFSET: 0x00401260
void GxMasterEnableSet(EGxMasterEnables state, int32_t enable) {
    g_theGxDevicePtr->MasterEnableSet(state, enable);
}

// VERIFY: 480127f6 manual
// OFFSET: 0x006813B0
void GxSceneClear(uint32_t mask, CImVector color) {
    g_theGxDevicePtr->SceneClear(mask, color);
}

// VERIFY: 061b1844 manual
// OFFSET: 0x00681470
void GxTexDestroy(CGxTex* texId) {
    g_theGxDevicePtr->TexDestroy(texId);
}

// VERIFY: bc2effe7 auto
// OFFSET: 0x006814D0
int32_t GxTexCopy(CGxTex* dest, const C2iVector& destPos, const C2iVector& sourcePos, const C2iVector& size, uint32_t level, uint32_t plane) {
    if (dest && dest->m_flags.m_renderTarget && (dest->m_target != GxTex_CubeMap || plane < 6)) {
        return g_theGxDevicePtr->TexCopy(dest, destPos, sourcePos, size, level, plane);
    }

    SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
    return 0;
}

// VERIFY: 1ff03c68 auto
// OFFSET: 0x00681580
int32_t GxTexReusable(CGxTex* texId) {
    return texId && texId->m_char5D && texId->m_depth == 0 && !texId->m_flags.m_generateMipMaps && !texId->m_flags.m_renderTarget && texId->m_target == GxTex_2d && texId->m_format <= GxTex_Dxt5;
}

// VERIFY: d90f6ed2 auto
// OFFSET: 0x006815C0
int32_t GxTexReusable(CGxTexParms& parms) {
    return parms.m_depth == 0 && !parms.m_flags.m_generateMipMaps && !parms.m_flags.m_renderTarget && parms.m_target == GxTex_2d && parms.m_format <= GxTex_Dxt5;
}

// VERIFY: 01ea0eb4 manual
// OFFSET: 0x00681630
void GxQueryCreate(CGxQuery*& query, EGxQueryType type) {
    if (type < 1) {
        g_theGxDevicePtr->QueryCreate(query, type);
    } else {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
    }
}

// VERIFY: be57cc7e manual
// OFFSET: 0x00681660
void GxQueryDestroy(CGxQuery*& query) {
    if (query) {
        g_theGxDevicePtr->QueryDestroy(query);
    } else {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
    }
}

// VERIFY: 955d4344 auto
// OFFSET: 0x00681770
void GxSetMaxFPS(uint32_t maxFPS) {
    s_maxFPS = maxFPS;
}

// VERIFY: b5658956 auto
// OFFSET: 0x00681780
uint32_t GxGetMaxFPS() {
    return s_maxFPS;
}

// VERIFY: 07048e1b auto
// OFFSET: 0x00681790
void GxSetMaxFPSBk(uint32_t maxFPSBk) {
    s_maxFPSBk = maxFPSBk;
}

// VERIFY: 0d2f3abb auto
// OFFSET: 0x006817A0
uint32_t GxGetMaxFPSBk() {
    return s_maxFPSBk;
}

// VERIFY: 80231ea8 auto
// OFFSET: 0x006817B0
BlitFormat GxGetBlitFormat(EGxTexFormat format) {
    return s_blitFormats[format];
}

// VERIFY: 286a063f auto
// OFFSET: 0x00681890
void CGxDevice::XformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
    if (minX == this->m_viewport.m_x.m_l && maxX == this->m_viewport.m_x.m_h && minY == this->m_viewport.m_y.m_l && maxY == this->m_viewport.m_y.m_h && minZ == this->m_viewport.m_z.m_l && maxZ == this->m_viewport.m_z.m_h) {
        return;
    }

    this->m_viewportDirty = 1;
    this->m_viewport.m_x.m_l = minX;
    this->m_viewport.m_x.m_h = maxX;
    this->m_viewport.m_y.m_l = minY;
    this->m_viewport.m_y.m_h = maxY;
    this->m_viewport.m_z.m_l = minZ;
    this->m_viewport.m_z.m_h = maxZ;
}

// VERIFY: 08ab74c6 manual
// OFFSET: 0x00681A20
void GxBufData(CGxBuf* buf, const void* data, uint32_t size, uint32_t offset) {
    if (!size) {
        size = buf->m_itemSize * buf->m_itemCount;
    }

    g_theGxDevicePtr->BufData(buf, data, size, offset);
    buf->m_hasData = 1;
}

// VERIFY: 9c35593e manual
// OFFSET: 0x00681A60
void GxDrawLockedElements() {
    if (!s_lockVertexCount) {
        return;
    }

    CGxBatch batch;
    batch.m_primType = s_lockPrimType;
    batch.m_start = 0;
    batch.m_count = s_lockIndexCount;
    batch.m_minIndex = 0;
    batch.m_maxIndex = static_cast<uint16_t>(s_lockVertexCount - 1);
    g_theGxDevicePtr->Draw(&batch, 1);
}

// VERIFY: 2f4f1ca9 auto
// OFFSET: 0x00681F60
void GxXformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
    float cleanMinX = minX > 0.0f ? minX : 0.0f;
    float cleanMaxX = maxX < 1.0f ? maxX : 1.0f;
    float cleanMinY = minY > 0.0f ? minY : 0.0f;
    float cleanMaxY = maxY < 1.0f ? maxY : 1.0f;

    if (!(cleanMinX < cleanMaxX)) {
        DebugLog("\"%s\", %s = %g, %s = %g", "cleanMinX < cleanMaxX", "minX", minX, "maxX", maxX);
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    if (!(cleanMinY < cleanMaxY)) {
        DebugLog("\"%s\", %s = %g, %s = %g", "cleanMinY < cleanMaxY", "minY", minY, "maxY", maxY);
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    if (!(minZ <= maxZ)) {
        DebugLog("\"%s\", %s = %g, %s = %g", "minZ <= maxZ", "minZ", minZ, "maxZ", maxZ);
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    if (!(minZ >= 0.0f)) {
        DebugLog("\"%s\", %s = %g", "minZ >= 0.0f", "minZ", minZ);
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    if (!(maxZ <= 1.0f)) {
        DebugLog("\"%s\", %s = %g", "maxZ <= 1.0f", "maxZ", maxZ);
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    g_theGxDevicePtr->XformSetViewport(cleanMinX, cleanMaxX, cleanMinY, cleanMaxY, minZ, maxZ);
}

// VERIFY: 2b10a3f5 manual
// OFFSET: 0x00682130
void GxXformViewProj(C44Matrix& matrix) {
    CGxDevice* device = g_theGxDevicePtr;
    C44Matrix view;
    C44Matrix proj;

    view = device->m_xforms[GxXform_View].m_mtx[device->m_xforms[GxXform_View].m_level];
    proj = device->m_projection;
    matrix = view * proj;
}

// VERIFY: cc1eb20e auto
// OFFSET: 0x00482A20
CGxBuf* GxBufStream(EGxPoolTarget target, uint32_t itemSize, uint32_t itemCount) {
    return g_theGxDevicePtr->BufStream(target, itemSize, itemCount);
}

// VERIFY: e93d7c49 auto
// OFFSET: 0x00616AD0
void GxXformPush(EGxXform xf, const C44Matrix& matrix) {
    g_theGxDevicePtr->XformPushAndSet(xf, matrix);
}

// VERIFY: 870a4b79 auto
// OFFSET: 0x006813F0
void GxTexSetCannotUpdate(CGxTex* texId) {
    g_theGxDevicePtr->TexSetCannotUpdate(texId);
}

// VERIFY: 5e6cbd33 auto
// OFFSET: 0x00681410
void GxTexSetUserData(CGxTex* texId, GxTexUserFunc userFunc, void* userArg) {
    g_theGxDevicePtr->TexSetUserData(texId, userFunc, userArg);
}

// VERIFY: 354bf1c4 auto
// OFFSET: 0x00681490
void GxTexParameters(const CGxTex* texId, CGxTexParms& parms) {
    g_theGxDevicePtr->TexParameters(texId, parms);
}

// VERIFY: 8ea5ed6d auto
// OFFSET: 0x006814B0
void GxTexSetDataFormat(CGxTex* texId, EGxTexFormat dataFormat) {
    g_theGxDevicePtr->TexSetDataFormat(texId, dataFormat);
}

// VERIFY: cf3f18d6 manual
// OFFSET: 0x00681AB0
void GxPrimIndexPtr(uint32_t indexCount, const uint16_t* indices) {
    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), indexCount);
    g_theGxDevicePtr->BufData(buf, indices, buf->m_itemSize * buf->m_itemCount, 0);
    buf->m_hasData = 1;
    g_theGxDevicePtr->PrimIndexPtr(buf);
}

// VERIFY: 75e347bc auto
// OFFSET: 0x00681F20
void GxTexUpdate(CGxTex* texId, int32_t minX, int32_t minY, int32_t maxX, int32_t maxY, int32_t immediate) {
    CiRect rect;
    rect.minY = minY;
    rect.minX = minX;
    rect.maxY = maxY;
    rect.maxX = maxX;
    g_theGxDevicePtr->TexMarkForUpdate(texId, rect, immediate);
}

// VERIFY: dfb2cc84 manual
// OFFSET: 0x006823A0
void GxPrimLockIndexPtr(EGxPrim primType, uint32_t indexCount, const uint16_t* indices) {
    s_lockPrimType = primType;
    s_lockIndexCount = indexCount;

    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), indexCount);
    g_theGxDevicePtr->BufData(buf, indices, buf->m_itemSize * buf->m_itemCount, 0);
    buf->m_hasData = 1;
    g_theGxDevicePtr->PrimIndexPtr(buf);
}

// VERIFY: 7adb9aa8 auto
// OFFSET: 0x006813D0
void GxTexUpdate(CGxTex* texId, CiRect& updateRect, int32_t immediate) {
    g_theGxDevicePtr->TexMarkForUpdate(texId, updateRect, immediate);
}

// VERIFY: 49a2a643 manual
// OFFSET: 0x00681CB0
int32_t GxTexCreate(uint32_t width, uint32_t height, EGxTexFormat format, CGxTexFlags flags, void* userArg, GxTexUserFunc userFunc, CGxTex*& texId) {
    texId = nullptr;

    if (width > g_theGxDevicePtr->Caps().m_texMaxSize[GxTex_2d] || height > g_theGxDevicePtr->Caps().m_texMaxSize[GxTex_2d] || (width & (width - 1)) || (height & (height - 1)) || format > GxTexFormats_Last) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (flags.m_generateMipMaps && (!g_theGxDevicePtr->Caps().m_generateMipMaps || (format >= GxTex_Dxt1 && format <= GxTex_Dxt5))) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if ((flags.m_filter == GxTex_Anisotropic && !g_theGxDevicePtr->Caps().m_texFilterAnisotropic) || !userFunc || width < 8 || height < 8) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    return g_theGxDevicePtr->TexCreate(GxTex_2d, width, height, 0, format, format, flags, userArg, userFunc, "Unknown", texId);
}

// VERIFY: a44e4684 manual
// OFFSET: 0x00681D90
int32_t GxTexCreate(EGxTexTarget target, uint32_t width, uint32_t height, uint32_t depth, EGxTexFormat format, EGxTexFormat dataFormat, CGxTexFlags flags, void* userArg, GxTexUserFunc userFunc, const char* name, CGxTex*& texId) {
    texId = nullptr;

    if (target > GxTexTargets_Last || g_theGxDevicePtr->Caps().m_texTarget[target] != 1 || width < 8 || height < 8 || width > g_theGxDevicePtr->Caps().m_texMaxSize[target] || height > g_theGxDevicePtr->Caps().m_texMaxSize[target]) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (target == GxTex_Rectangle) {
        if (flags.m_filter > GxTex_Linear) {
            SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
            return 0;
        }
    } else if (target != GxTex_NonPow2) {
        if ((width & (width - 1)) || (height & (height - 1))) {
            SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
            return 0;
        }
    }

    if (format > GxTexFormats_Last || (flags.m_generateMipMaps && (!g_theGxDevicePtr->Caps().m_generateMipMaps || (format >= GxTex_Dxt1 && format <= GxTex_Dxt5))) || (flags.m_filter == GxTex_Anisotropic && !g_theGxDevicePtr->Caps().m_texFilterAnisotropic) || dataFormat > GxTexFormats_Last || !userFunc) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    return g_theGxDevicePtr->TexCreate(target, width, height, depth, format, dataFormat, flags, userArg, userFunc, name, texId);
}

// VERIFY: 6866ac7c auto
// OFFSET: 0x00681EE0
int32_t GxTexCreate(const CGxTexParms& parms, CGxTex*& texId) {
    return GxTexCreate(parms.m_target, parms.m_width, parms.m_height, parms.m_depth, parms.m_format, parms.m_dataFormat, parms.m_flags, parms.m_userArg, parms.m_userFunc, "", texId);
}

// VERIFY: 83fc849d auto
// OFFSET: 0x00681B00
void GxPrimVertexPtr(CGxBuf* buf, EGxVertexBufferFormat format) {
    const VertexBufDesc& desc = Buffer::s_vertexBufDesc[format];
    g_theGxDevicePtr->PrimVertexFormat(buf, desc.attribs, desc.attribCount);
    g_theGxDevicePtr->PrimVertexMask(desc.mask);
    g_theGxDevicePtr->PrimVertexPtr(buf, format);
}

// VERIFY: d5a113e1 manual
// OFFSET: 0x00682400
void GxPrimVertexPtr(uint32_t vertexCount, const C3Vector* pos, uint32_t posStride, const C3Vector* normal, uint32_t normalStride, const CImVector* color, uint32_t colorStride, const C2Vector* tex0, uint32_t tex0Stride, const C2Vector* tex1, uint32_t tex1Stride) {
    EGxVertexBufferFormat format = GxVBF_PN;

    if (pos) {
        if (normal) {
            if (color) {
                if (!tex0 && !tex1) {
                    format = GxVBF_PNC;
                } else if (tex0 && !tex1) {
                    format = GxVBF_PNCT;
                } else if (tex0 && tex1) {
                    format = GxVBF_PNCT2;
                }
            } else {
                if (!tex0 && !tex1) {
                    format = GxVBF_PN;
                } else if (tex0 && !tex1) {
                    format = GxVBF_PNT;
                } else if (tex0 && tex1) {
                    format = GxVBF_PNT2;
                }
            }
        } else if (color) {
            if (tex0) {
                format = tex1 ? GxVBF_PCT2 : GxVBF_PCT;
            } else if (!tex1) {
                format = GxVBF_PC;
            }
        } else if (tex0) {
            format = tex1 ? GxVBF_PT2 : GxVBF_PT;
        } else if (!tex1) {
            format = GxVBF_P;
        }
    }

    uint32_t vertexSize = s_vertexSize[format];
    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, vertexSize, vertexCount);
    char* data = g_theGxDevicePtr->BufLock(buf);

    C3Vector emptyNormal;
    C2Vector emptyTex1;
    C2Vector emptyTex0;
    CImVector emptyColor;

    char* posDst = data + s_vertexAttribOffset[format][GxVA_Position];
    char* normalDst = normal ? data + s_vertexAttribOffset[format][GxVA_Normal] : reinterpret_cast<char*>(&emptyNormal);
    uint32_t normalDstStride = normal ? vertexSize : 0;
    char* colorDst = color ? data + s_vertexAttribOffset[format][GxVA_Color0] : reinterpret_cast<char*>(&emptyColor);
    uint32_t colorDstStride = color ? vertexSize : 0;
    char* tex0Dst = tex0 ? data + s_vertexAttribOffset[format][GxVA_TexCoord0] : reinterpret_cast<char*>(&emptyTex0);
    uint32_t tex0DstStride = tex0 ? vertexSize : 0;
    char* tex1Dst = tex1 ? data + s_vertexAttribOffset[format][GxVA_TexCoord1] : reinterpret_cast<char*>(&emptyTex1);
    uint32_t tex1DstStride = tex1 ? vertexSize : 0;

    if (!normal) {
        normal = &emptyNormal;
        normalStride = 0;
    }

    if (!color) {
        color = &emptyColor;
        colorStride = 0;
    }

    if (!tex0) {
        tex0 = &emptyTex0;
        tex0Stride = 0;
    }

    if (!tex1) {
        tex1 = &emptyTex1;
        tex1Stride = 0;
    }

    for (uint32_t i = 0; i < vertexCount; i++) {
        *reinterpret_cast<C3Vector*>(posDst) = *pos;
        pos = reinterpret_cast<const C3Vector*>(reinterpret_cast<const char*>(pos) + posStride);
        posDst += vertexSize;

        *reinterpret_cast<C3Vector*>(normalDst) = *normal;
        normal = reinterpret_cast<const C3Vector*>(reinterpret_cast<const char*>(normal) + normalStride);
        normalDst += normalDstStride;

        CImVector c;

        if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
            c.b = color->r;
            c.g = color->g;
            c.r = color->b;
            c.a = color->a;
        } else {
            c = *color;
        }

        *reinterpret_cast<CImVector*>(colorDst) = c;
        color = reinterpret_cast<const CImVector*>(reinterpret_cast<const char*>(color) + colorStride);
        colorDst += colorDstStride;

        *reinterpret_cast<C2Vector*>(tex0Dst) = *tex0;
        tex0Dst += tex0DstStride;
        tex0 = reinterpret_cast<const C2Vector*>(reinterpret_cast<const char*>(tex0) + tex0Stride);

        *reinterpret_cast<C2Vector*>(tex1Dst) = *tex1;
        tex1 = reinterpret_cast<const C2Vector*>(reinterpret_cast<const char*>(tex1) + tex1Stride);
        tex1Dst += tex1DstStride;
    }

    g_theGxDevicePtr->BufUnlock(buf, vertexCount * vertexSize);
    buf->m_hasData = 1;

    const VertexBufDesc& desc = Buffer::s_vertexBufDesc[format];
    g_theGxDevicePtr->PrimVertexFormat(buf, desc.attribs, desc.attribCount);
    g_theGxDevicePtr->PrimVertexMask(desc.mask);
    g_theGxDevicePtr->PrimVertexPtr(buf, format);
}

// VERIFY: a230e3a0 auto
// OFFSET: 0x006828C0
void GxPrimLockVertexPtrs(uint32_t vertexCount, const C3Vector* pos, uint32_t posStride, const C3Vector* normal, uint32_t normalStride, const CImVector* color, uint32_t colorStride, const uint8_t* bone, uint32_t boneStride, const C2Vector* tex0, uint32_t tex0Stride, const C2Vector* tex1, uint32_t tex1Stride) {
    s_lockVertexCount = vertexCount;
    GxPrimVertexPtr(vertexCount, pos, posStride, normal, normalStride, color, colorStride, tex0, tex0Stride, tex1, tex1Stride);
}

// VERIFY: 5e637469 auto
// OFFSET: 0x00408240
void GxRsSet(EGxRenderState which, void* value) {
    g_theGxDevicePtr->RsSet(which, value);
}

// VERIFY: 194fac0e auto
// OFFSET: 0x00408BF0
void GxRsSet(EGxRenderState which, int32_t value) {
    if (!g_theGxDevicePtr->m_context) {
        return;
    }

    CGxAppRenderState& state = g_theGxDevicePtr->m_appRenderStates[which];

    if (state.m_value.m_data.p != reinterpret_cast<void*>(static_cast<intptr_t>(value))) {
        g_theGxDevicePtr->IRsDirty(which);
        state.m_value.m_data.p = reinterpret_cast<void*>(static_cast<intptr_t>(value));
    }
}

// VERIFY: d06b58a9 manual
// OFFSET: 0x00408C30
void GxUpdateAlphaRef(EGxRenderState which) {
    if (which != GxRs_AlphaRef || !g_theGxDevicePtr->m_context) {
        return;
    }

    int32_t value = CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates[GxRs_BlendingMode].m_value.m_data.i[0]];
    CGxAppRenderState& state = g_theGxDevicePtr->m_appRenderStates[GxRs_AlphaRef];

    if (state.m_value.m_data.i[0] != value) {
        g_theGxDevicePtr->IRsDirty(GxRs_AlphaRef);
        state.m_value.m_data.i[0] = value;
    }
}

// VERIFY: 3f036f15 auto
// OFFSET: 0x00763C70
void GxRsSet(EGxRenderState which, float value) {
    if (!g_theGxDevicePtr->m_context) {
        return;
    }

    CGxAppRenderState& state = g_theGxDevicePtr->m_appRenderStates[which];

    if (value != state.m_value.m_data.f[0]) {
        g_theGxDevicePtr->IRsDirty(which);
        state.m_value.m_data.f[0] = value;
    }
}

// VERIFY: d54d4b83 auto
// OFFSET: 0x00681430
void GxTexSetFlags(CGxTex* texId, CGxTexFlags flags) {
    g_theGxDevicePtr->TexSetFlags(texId, flags);
}

// VERIFY: afbfe546 auto
// OFFSET: 0x00681450
void GxTexSetWrap(CGxTex* texId, EGxTexWrapMode wrapU, EGxTexWrapMode wrapV) {
    g_theGxDevicePtr->TexSetWrap(texId, wrapU, wrapV);
}

// VERIFY: 9afde255 manual
// OFFSET: 0x006817C0
void GxLogOpen() {
    CGxDevice::LogOpen();
}

// VERIFY: d45cbace manual
// OFFSET: 0x006817D0
void GxLogClose() {
    CGxDevice::LogClose();
}

// VERIFY: b65f4ddb auto
// OFFSET: 0x006817E0
void GxLog(const char* format, ...) {
    va_list args;
    va_start(args, format);
    CGxDevice::Log(format, args);
    va_end(args);
}

// VERIFY: b49a76f0 manual
// OFFSET: 0x00682200
void GxLogCrashInfo(char* buffer, uint32_t size) {
    if (!g_theGxDevicePtr) {
        return;
    }

    SStrPack(buffer, "\r\n----------------------------------------\r\n", STORM_MAX_STR);
    SStrPack(buffer, "               GxInfo\r\n", STORM_MAX_STR);
    SStrPack(buffer, "----------------------------------------\r\n", STORM_MAX_STR);

    char line[260];
    SStrPrintf(line, sizeof(line), "GxApi: %s\r\n", g_gxApiNames[g_theGxDevicePtr->DeviceApi()]);
    SStrPack(buffer, line, size);
    g_theGxDevicePtr->LogCrashInfo(buffer, size);
    SStrPack(buffer, "\r\n", size);
}

// VERIFY: a9e848ff auto
// OFFSET: 0x006C42B0
void GxBufDestroy(CGxBuf* buf) {
    if (buf && buf != g_theGxDevicePtr->BufStream(buf->m_pool->m_target, 0, 0)) {
        g_theGxDevicePtr->BufDestroy(buf);
    }
}

// VERIFY: a44ff4ef auto
// OFFSET: 0x006C42F0
void GxFormatColor(CImVector& color) {
    if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
        CImVector formatted;
        formatted.b = color.r;
        formatted.g = color.g;
        formatted.r = color.b;
        formatted.a = color.a;
        color = formatted;
    }
}

// VERIFY: 25db0bac manual
// OFFSET: 0x0057E4F0
void GxRenderTargetSet(EGxBuffer buffer, CGxTex* texture, uint32_t plane) {
    GxRsSet(GxRs_ScissorTest, 0);
    g_theGxDevicePtr->DeviceSetRenderTarget(buffer, texture, plane);
}

// VERIFY: 9ee00156 manual
// OFFSET: 0x0057C450
void GxXformSet(EGxXform xf, const C44Matrix& matrix) {
    CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[xf];
    stack.m_dirty = 1;
    stack.m_flags[stack.m_level] &= ~CGxMatrixStack::F_Identity;
    stack.m_mtx[stack.m_level] = matrix;
}

// VERIFY: 00114247 auto
// OFFSET: 0x00616AB0
void GxReadPixels(CiRect& rect, TSGrowableArray<CImVector>& pixels) {
    g_theGxDevicePtr->DeviceReadPixels(rect, pixels);
}

// VERIFY: fdd977f9 auto
// OFFSET: 0x00681530
int32_t GxTexStretch(CGxTex* dest, const CiRect* destRect, const CiRect* sourceRect, uint32_t level, uint32_t face) {
    if (dest && dest->m_flags.m_renderTarget && (dest->m_target != GxTex_CubeMap || face < 6)) {
        return g_theGxDevicePtr->TexStretch(dest, destRect, sourceRect, level, face);
    }

    SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
    return 0;
}

// VERIFY: 37bbf5ec auto
// OFFSET: 0x00681690
bool GxQueryBegin(CGxQuery* query) {
    if (query) {
        return g_theGxDevicePtr->QueryBegin(query);
    }

    SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
    return false;
}

// VERIFY: 4f399bec auto
// OFFSET: 0x006816C0
bool GxQueryEnd(CGxQuery* query) {
    if (query) {
        return g_theGxDevicePtr->QueryEnd(query);
    }

    SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
    return false;
}

// VERIFY: ed0eecd3 manual
// OFFSET: 0x006816F0
bool GxQueryGetParam(CGxQuery* query, EGxQueryParam param, uint32_t* data) {
    if (query && param < 2 && data) {
        return g_theGxDevicePtr->QueryGetParam(query, param, *data);
    }

    SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
    return false;
}

// VERIFY: 03a1b391 manual
// OFFSET: 0x00681730
bool GxQueryGetData(CGxQuery* query, uint32_t* data) {
    if (query && data) {
        return g_theGxDevicePtr->QueryGetData(query, *data);
    }

    SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
    return false;
}

// VERIFY: 5aea5c65 auto
// OFFSET: 0x00682340
void GxDrawLockedElements(EGxPrim primType, uint32_t indexCount, const uint16_t* indices) {
    if (!s_lockVertexCount) {
        return;
    }

    GxPrimIndexPtr(indexCount, indices);

    CGxBatch batch;
    batch.m_primType = primType;
    batch.m_start = 0;
    batch.m_count = indexCount;
    batch.m_minIndex = 0;
    batch.m_maxIndex = static_cast<uint16_t>(s_lockVertexCount - 1);
    g_theGxDevicePtr->Draw(&batch, 1);
}

// VERIFY: 97b9ed19 manual
// OFFSET: 0x00681B50
static void GxuPreprocessDrawList(EGxuDrawListCategory category, EGxuDrawListSorting sorting, const C3Vector& viewPos) {
    for (uint32_t i = 0; i < s_drawLists[category].Count(); i++) {
        s_drawListPreprocess[sorting](&s_drawLists[category][i], viewPos);
    }
}

// VERIFY: 916be93a manual
// OFFSET: 0x00681BA0
void GxuSortDrawList(EGxuDrawListCategory category, EGxuDrawListSorting sorting, const C3Vector& viewPos) {
    GxuPreprocessDrawList(category, sorting, viewPos);
    qsort(s_drawLists[category].Ptr(), s_drawLists[category].Count(), sizeof(GxDrawListEntry), s_drawListSorters[sorting]);
}

// VERIFY: 631533e2 auto
// OFFSET: 0x00682900
void GxuClearDrawList(EGxuDrawListCategory category) {
    s_drawListBusy = true;

    for (uint32_t i = 0; i < s_drawLists[category].Count(); i++) {
        s_drawLists[category][i].m_item->Discard();
    }

    s_drawLists[category].SetCount(0);
    s_drawListBusy = false;
}

// VERIFY: 5e771d38 auto
// OFFSET: 0x00682960
void GxuFlushDrawList(EGxuDrawListCategory category, const C3Vector& viewPos) {
    GxuDrawListState* current = nullptr;

    for (uint32_t i = 0; i < s_drawLists[category].Count(); i++) {
        GxuDrawListItem* item = s_drawLists[category][i].m_item;
        GxuDrawListState* state = item->GetState();

        if (state && state != current) {
            if (current) {
                current->End();
            }

            state->Begin();
            current = state;
        }

        item->Draw(viewPos);
    }

    if (current) {
        current->End();
    }

    GxuClearDrawList(category);
}

// VERIFY: 456ff928 auto
// OFFSET: 0x00682A00
void GxScenePresent() {
    C3Vector viewPos(0.0f, 0.0f, 0.0f);
    GxuFlushDrawList(GxuCat_2, viewPos);
    g_theGxDevicePtr->ScenePresent();
}

// VERIFY: 6d471ae7 auto
// OFFSET: 0x00682B80
void GxDevDestroy(CGxDevice* device) {
    if (!device) {
        return;
    }

    for (uint32_t i = 0; i < GxuCats_Last; i++) {
        GxuClearDrawList(static_cast<EGxuDrawListCategory>(i));
    }

    for (uint32_t i = 0; i < s_destroyCallbacks.Count(); i++) {
        s_destroyCallbacks[i]();
    }

    s_destroyCallbacks.Clear();

    if (device != g_theGxDevicePtr) {
        DebugLog("devicePtr == g_theGxDevicePtr");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    g_theGxDevicePtr->DeviceDestroy();
    delete g_theGxDevicePtr;
    g_theGxDevicePtr = nullptr;
}

// VERIFY: a84aeead manual
// OFFSET: 0x005EEB70
void GxPrimUnlockVertexPtrs() {
}

// INLINED
GxDrawListEntry::GxDrawListEntry() {
    this->m_item = nullptr;
    this->m_depth = 0.0f;
    this->m_vtxStream = 0;
}

// VERIFY: e3f930cc auto
// OFFSET: 0x00482A60
CImVector GxSwizzleColor(const CImVector& color) {
    CImVector result;

    if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
        result.b = color.r;
        result.g = color.g;
        result.r = color.b;
        result.a = color.a;
    } else {
        result = color;
    }

    return result;
}
