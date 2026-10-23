#include <cfloat>
#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <new>
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Gx/Buffer.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxDevice/CGxShader.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Os/Processor.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/ShaderEffect/Types.h"

// STATIC: 0x00A2DAF8
const uint32_t CGxDevice::s_texFormatBitDepth[GxTexFormats_Last] = { 0, 32, 32, 16, 16, 16, 4, 8, 8, 16, 32, 32, 32 };

// STATIC: 0x00A2DB2C
const uint32_t CGxDevice::s_texFormatBlockSize[GxTexFormats_Last] = { 1, 1, 1, 1, 1, 1, 4, 4, 4, 1, 1, 1, 1 };

// STATIC: 0x00A2DB60
const uint32_t CGxDevice::s_texFormatBytesPerBlock[GxTexFormats_Last] = { 0, 4, 4, 2, 2, 2, 8, 16, 16, 2, 4, 4, 4 };

// STATIC: 0x00AD8B4C
static uint32_t s_primVertsPerPrim[GxPrims_Last] = { 1, 2, 1, 3, 1, 1 };

// STATIC: 0x00AD8B64
static uint32_t s_primVertsExtra[GxPrims_Last] = { 0, 0, 1, 0, 2, 2 };

// STATIC: 0x00AD87C4
const char* s_formatToInt[CGxFormat::Formats_Last] = { "16", "24", "24", "30", "16", "24", "24", "32" };

// STATIC: 0x00AD87FC
const char* CGxDevice::s_pixelShaderTargetNames[14] = { "none", "ps_1_1", "ps_1_4", "ps_2_0", "ps_3_0", "ps_4_0", "ps_5_0", "nvrc", "nvts", "nvts2", "nvts3", "nvfp2", "arbfp1", "glsl" };

// STATIC: 0x00AD8834
const char* CGxDevice::s_vertexShaderTargetNames[12] = { "none", "vs_1_1", "vs_2_0", "vs_3_0", "vs_4_0", "vs_5_0", "arbvp1", "arbvp1_cg12", "nvvp", "nvvp2", "nvvp3", "glsl" };

// STATIC: 0x00AD8864
static const char* s_geometryShaderTargetNames[3] = { "none", "gs_4_0", "gs_5_0" };

// STATIC: 0x00AD8870
static const char* s_hullShaderTargetNames[2] = { "none", "hs_5_0" };

// STATIC: 0x00AD8878
static const char* s_domainShaderTargetNames[2] = { "none", "ds_5_0" };

// STATIC: 0x00AD8880
static const char* s_computeShaderTargetNames[4] = { "none", "cs_5_0", "cs_cuda", "cs_ocl" };

// STATIC: 0x00AD8890
static const char** s_shaderTargetNames[GxShTargets_Last] = { CGxDevice::s_vertexShaderTargetNames, s_hullShaderTargetNames, s_domainShaderTargetNames, s_geometryShaderTargetNames, CGxDevice::s_pixelShaderTargetNames, s_computeShaderTargetNames };

// STATIC: 0x00AD8B7C
uint32_t CGxDevice::s_alphaRef[12] = { 0, 224, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };

// STATIC: 0x00AD8BAC
int32_t s_streamVertexPoolSize = 0x2C0000;

// STATIC: 0x00AD8BB0
int32_t s_streamIndexPoolSize = 0x40000;

// STATIC: 0x00AD8BB4
C3Vector CGxDevice::s_pointScaleAttenuation(1.0f, 0.0f, 0.0f);

// STATIC: 0x00AD8C80
static const char* s_formatNames[CGxFormat::Formats_Last] = { "Rgb565", "ArgbX888", "Argb8888", "Argb2101010", "Ds160", "Ds24X", "Ds248", "Ds320" };

// STATIC: 0x00AD8BC0
static C2iVector kWinMinSize = { 320, 240 };

// STATIC: 0x00C5DF88
CGxDevice* g_theGxDevicePtr;

// STATIC: 0x00C5DFD8
CGxShader* CGxDevice::s_uiVertexShader[2];

// STATIC: 0x00C5DFE0
ShadowConstants CGxDevice::s_shadowConstants[2];

// STATIC: 0x00C5FFFC
CGxShader* CGxDevice::s_uiPixelShader;

// STATIC: 0x00C5FFF4
uint32_t CGxDevice::m_logBytes;

// STATIC: 0x00C5FFF8
HSLOG CGxDevice::m_log;

// STATIC: 0x00C5FFF0
static float kWinAspect;

// STATIC: 0x00C60300
C2iVector CGxDevice::s_clientAdjust;

// STATIC: 0x00C60308
static uint32_t s_nextFrameTime;

void CGxDevice::DeviceEvictResources() {
}

void CGxDevice::DeviceSaveDepthSurface() {
}

void CGxDevice::DeviceRestoreDepthSurface() {
}

void CGxDevice::DeviceResolveDepthBuffer(CGxTex* texture) {
}

void CGxDevice::DeviceCopyTex(CGxTex* source, uint32_t sourcePlane, CGxTex* dest, uint32_t destPlane) {
}

void CGxDevice::LogCrashInfo(char* buffer, uint32_t size) {
}

void CGxDevice::SceneClear(uint32_t mask, CImVector color) {
}

void CGxDevice::Draw(CGxBatch* batch, int32_t count) {
}

void CGxDevice::PrimPointSize(float size) {
}

void CGxDevice::PrimLineWidth(float width) {
}

void CGxDevice::TexSetCacheSize(uint32_t size) {
}

// VERIFY: c758af65 manual
// OFFSET: 0x00682CB0
int32_t CGxDevice::DeviceCreate(GxWindowProc windowProc, const CGxFormat& format) {
    this->m_windowProc = windowProc;
    return this->DeviceSetFormat(format);
}

// VERIFY: c06d856e auto
// OFFSET: 0x00682CD0
int32_t CGxDevice::DeviceCreate(void* window, const CGxFormat& format) {
    this->m_format = format;
    return 1;
}

// VERIFY: e7b62b2b auto
// OFFSET: 0x00682CF0
void CGxDevice::DeviceDestroy() {
    this->m_windowProc = nullptr;
}

// VERIFY: 25e504a6 auto
// OFFSET: 0x00682D00
void CGxDevice::DeviceSetBaseMipLevel(uint32_t level) {
    this->m_baseMipLevel = level;
}

// VERIFY: 524ed3c5 auto
// OFFSET: 0x00682D30
void CGxDevice::DeviceTakeScreenShot() {
    this->m_scrShotClick = 1;
}

// VERIFY: 4da47b00 auto
// OFFSET: 0x00682E50
void CGxDevice::ScenePresent() {
    this->m_frameCount++;
    this->m_scrShotClick = 0;
    this->m_intF5C = 0;
}

// VERIFY: 7c26f822 auto
// OFFSET: 0x00682F70
void CGxDevice::PrimNormal(const C3Vector& n) {
    this->m_primNormal = n;
    this->m_primMask |= PrimMask_Normal;
}

// VERIFY: c4ffca2f auto
// OFFSET: 0x00682FA0
void CGxDevice::PrimTexCoord(uint32_t tmu, const C2Vector& t) {
    this->m_primTexCoord[tmu] = t;
    this->m_primMask |= PrimMask_TexCoord << tmu;
}

// VERIFY: c119ac3a manual
// OFFSET: 0x00683130
void CGxDevice::BufData(CGxBuf* buf, const void* data, uint32_t size, uint32_t offset) {
    buf->m_dirty = 1;
    buf->unk_001F = 0;
}

// VERIFY: 0d3647c3 manual
// OFFSET: 0x00683150
char* CGxDevice::BufLock(CGxBuf* buf) {
    buf->m_dirty = 1;
    buf->unk_001F = 0;
    this->m_bufLocked[buf->m_pool->m_target] = buf;
    return nullptr;
}

// VERIFY: da5b4d82 auto
// OFFSET: 0x00683180
int32_t CGxDevice::BufUnlock(CGxBuf* buf, uint32_t size) {
    this->m_bufLocked[buf->m_pool->m_target] = nullptr;
    return 1;
}

// VERIFY: 51c4b8e7 auto
// OFFSET: 0x00683640
void CGxDevice::CursorSetVisible(int32_t visible) {
    this->m_cursorVisible = visible;
}

// VERIFY: dfe1cbff auto
// OFFSET: 0x00683650
uint32_t* CGxDevice::CursorLock() {
    return this->m_cursor;
}

// VERIFY: fd4d7197 manual
// OFFSET: 0x006840F0
int32_t CGxDevice::DeviceSetFormat(const CGxFormat& format) {
    this->m_format = format;

    if (this->m_format.m_size.x < kWinMinSize.x && this->m_format.m_size.y < kWinMinSize.y) {
        this->m_format.m_size = kWinMinSize;
    }

    if (this->m_format.m_aspect) {
        kWinAspect = static_cast<float>(this->m_format.m_size.x) / static_cast<float>(this->m_format.m_size.y);
    } else {
        kWinAspect = 0.0f;
    }

    return 1;
}

// VERIFY: 142cf1ae auto
// OFFSET: 0x00684170
void CGxDevice::DeviceSetGamma(float gamma) {
    this->m_gammaRamp.Set(gamma);
}

// VERIFY: 9e53fc0c auto
// OFFSET: 0x00684190
void CGxDevice::DeviceSetGamma(const CGxGammaRamp& ramp) {
    this->m_gammaRamp = ramp;
}

// VERIFY: d081145f auto
// OFFSET: 0x00684260
void CGxDevice::DeviceReadScreenShot(uint32_t& width, uint32_t& height, const CImVector*& pixels) {
    width = this->m_scrShotWidth;
    height = this->m_scrShotHeight;
    pixels = this->m_scrShotPixels.Count() ? this->m_scrShotPixels.Ptr() : nullptr;
}

// VERIFY: 29912845 manual
// OFFSET: 0x006842A0
void CGxDevice::DeviceSetRenderTarget(EGxBuffer buffer, CGxTex* texture, uint32_t plane) {
    CGxDevice::TextureTarget& target = this->m_textureTarget[buffer];
    target.m_plane = plane;
    target.m_texture = texture;

    if (texture && (this->m_textureTarget[GxBuffers_Color].m_texture || this->m_textureTarget[GxBuffers_Depth].m_texture)) {
        this->m_curWindowRect.minY = 0.0f;
        this->m_curWindowRect.minX = 0.0f;
        this->m_curWindowRect.maxY = static_cast<float>(texture->m_height);
        this->m_curWindowRect.maxX = static_cast<float>(texture->m_width);
    } else {
        this->m_curWindowRect = this->m_defWindowRect;
    }
}

// VERIFY: 48d0bcc7 auto
// OFFSET: 0x006843B0
void CGxDevice::NotifyOnDeviceRestored() {
    for (uint32_t i = 0; i < this->m_deviceRestoredCallbacks.Count(); i++) {
        this->m_deviceRestoredCallbacks[i]();
    }
}

// VERIFY: 27328acc auto
// OFFSET: 0x006843E0
void CGxDevice::NotifyOnTextureRecreation() {
    for (uint32_t i = 0; i < this->m_textureRecreationCallbacks.Count(); i++) {
        this->m_textureRecreationCallbacks[i]();
    }
}

// VERIFY: 1bc77089 auto
// OFFSET: 0x00684410
void CGxDevice::NotifyOnStereoChanged() {
    for (uint32_t i = 0; i < this->m_stereoChangedCallbacks.Count(); i++) {
        this->m_stereoChangedCallbacks[i]();
    }
}

// VERIFY: e5049b8a auto
// OFFSET: 0x00684590
void CGxDevice::PrimColor(const CImVector& c) {
    this->m_primColor = c;
    this->m_primMask |= PrimMask_Color;
}

// VERIFY: df433744 auto
// OFFSET: 0x00684900
void CGxDevice::ITexMarkAsUpdated(CGxTex* texId) {
    if (texId->m_needsUpdate) {
        texId->m_updateRect.minY = texId->m_height;
        texId->m_updateRect.minX = texId->m_width;
        texId->m_updateRect.maxY = 0;
        texId->m_updateRect.maxX = 0;
        texId->m_needsUpdate = 0;
    }
}

// VERIFY: 62bd795e manual
// OFFSET: 0x006853B0
void CGxDevice::AddDeviceRestoredCallback(GxDeviceCallback callback) {
    this->m_deviceRestoredCallbacks.New(callback);
}

// VERIFY: 657b2662 manual
// OFFSET: 0x006853D0
void CGxDevice::RemoveDeviceRestoredCallback(GxDeviceCallback callback) {
    uint32_t count = this->m_deviceRestoredCallbacks.Count();

    for (uint32_t i = 0; i < count; i++) {
        if (this->m_deviceRestoredCallbacks[i] == callback) {
            this->m_deviceRestoredCallbacks[i] = this->m_deviceRestoredCallbacks[count - 1];
            this->m_deviceRestoredCallbacks.SetCount(count - 1);
            return;
        }
    }
}

// VERIFY: 8ea3cd66 manual
// OFFSET: 0x00685460
void CGxDevice::AddTextureRecreationCallback(GxDeviceCallback callback) {
    this->m_textureRecreationCallbacks.New(callback);
}

// VERIFY: 50ad83fc manual
// OFFSET: 0x00685480
void CGxDevice::RemoveTextureRecreationCallback(GxDeviceCallback callback) {
    uint32_t count = this->m_textureRecreationCallbacks.Count();

    for (uint32_t i = 0; i < count; i++) {
        if (this->m_textureRecreationCallbacks[i] == callback) {
            this->m_textureRecreationCallbacks[i] = this->m_textureRecreationCallbacks[count - 1];
            this->m_textureRecreationCallbacks.SetCount(count - 1);
            return;
        }
    }
}

// VERIFY: 3c7c9e71 manual
// OFFSET: 0x00685510
void CGxDevice::AddStereoChangedCallback(GxDeviceCallback callback) {
    this->m_stereoChangedCallbacks.New(callback);
}

// VERIFY: 94dac580 manual
// OFFSET: 0x00685530
void CGxDevice::RemoveStereoChangedCallback(GxDeviceCallback callback) {
    uint32_t count = this->m_stereoChangedCallbacks.Count();

    for (uint32_t i = 0; i < count; i++) {
        if (this->m_stereoChangedCallbacks[i] == callback) {
            this->m_stereoChangedCallbacks[i] = this->m_stereoChangedCallbacks[count - 1];
            this->m_stereoChangedCallbacks.SetCount(count - 1);
            return;
        }
    }
}

// VERIFY: df04dabc manual
// OFFSET: 0x006855C0
void CGxDevice::PrimBegin(EGxPrim primType) {
    this->m_primType = primType;
    this->m_inBeginEnd = 1;
    this->m_primMask = 0;
    this->m_primIndexArray.SetCount(0);
    this->m_primVertexArray.SetCount(0);

    for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_caps.m_numTmus); i++) {
        this->m_primTexCoordArray[i].SetCount(0);
    }

    this->m_primNormalArray.SetCount(0);
    this->m_primColorArray.SetCount(0);
}

// VERIFY: b1a5b43a manual
// OFFSET: 0x00685640
void CGxDevice::PrimVertex(const C3Vector& v) {
    this->m_primVertex = v;
    this->m_primMask |= PrimMask_Vertex;

    uint16_t index = static_cast<uint16_t>(this->m_primVertexArray.Count());
    *this->m_primIndexArray.New() = index;
    *this->m_primVertexArray.New() = this->m_primVertex;

    for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_caps.m_numTmus); i++) {
        *this->m_primTexCoordArray[i].New() = this->m_primTexCoord[i];
    }

    *this->m_primNormalArray.New() = this->m_primNormal;
    *this->m_primColorArray.New() = this->m_primColor;

    uint32_t perPrim = s_primVertsPerPrim[this->m_primType];
    uint32_t indexCount = this->m_primIndexArray.Count();
    int32_t prims = (perPrim == 1 ? indexCount : indexCount / perPrim) - s_primVertsExtra[this->m_primType];
    uint32_t maxVerts = perPrim == 1 ? 0x20000 : 0x20000 / perPrim;

    if (prims == static_cast<int32_t>(maxVerts - s_primVertsExtra[this->m_primType])) {
        uint32_t mask = this->m_primMask;
        this->PrimDrawElements();
        this->PrimBegin(this->m_primType);
        this->m_primMask = mask;
    }
}

// VERIFY: 37074a54 manual
// OFFSET: 0x006877C0
void CGxDevice::QueryCreate(CGxQuery*& query, EGxQueryType type) {
    query = new (SMemAlloc(sizeof(CGxQuery), __FILE__, __LINE__, 0)) CGxQuery(type);
    this->m_queryList.LinkToTail(query);
}

// VERIFY: d0147a38 manual
// OFFSET: 0x006879F0
void CGxDevice::QueryDestroy(CGxQuery*& query) {
    query->m_link.Unlink();
    delete query;
    query = nullptr;
}

// VERIFY: 2daa7013 manual
// OFFSET: 0x00687A90
void CGxDevice::ICursorDraw() {
    if (!this->m_cursorVisible || this->m_hwCursor) {
        return;
    }

    int32_t x;
    int32_t y;
    OsInputGetMousePosition(&x, &y);

    if (x < 0 || y < 0 || !(x < this->m_curWindowRect.maxX) || !(y < this->m_curWindowRect.maxY)) {
        return;
    }

    g_theGxDevicePtr->RsPush();

    GxRsSet(GxRs_PolygonOffset, 0);
    GxRsSet(GxRs_NormalizeNormals, 0);
    GxRsSet(GxRs_BlendingMode, 1);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_ClipPlaneMask, 0);
    GxRsSet(GxRs_Texture0, this->m_cursorTexture);
    GxRsSet(GxRs_Texture1, nullptr);
    GxRsSet(GxRs_ColorOp0, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp0, GxTexOp_Mod);
    GxRsSet(GxRs_TexGen0, 0);
    GxRsSet(GxRs_TextureShader0, 0);

    C44Matrix world;
    GxXformPush(GxXform_World, world);

    float depth = 1.0f;
    C44Matrix projection;

    if (this->StereoEnabled() && CGxDevice::s_uiVertexShader[1] && CGxDevice::s_uiVertexShader[1]->Valid() && CGxDevice::s_uiPixelShader && CGxDevice::s_uiPixelShader->Valid()) {
        depth = this->m_cursorDepth;

        float minX;
        float maxX;
        float minY;
        float maxY;
        float minZ;
        float maxZ;
        GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);
        GxXformProjection(projection);

        C44Matrix ortho;
        ortho.a0 = 2.0f / (maxX - minX);
        ortho.a1 = 0.0f;
        ortho.a2 = 0.0f;
        ortho.a3 = 0.0f;
        ortho.b0 = 0.0f;
        ortho.b1 = 2.0f / (maxY - minY);
        ortho.b2 = 0.0f;
        ortho.b3 = 0.0f;
        ortho.c0 = 0.0f;
        ortho.c1 = 0.0f;
        ortho.c2 = 1.00008f;
        ortho.c3 = 1.0f;
        ortho.d0 = -((maxX + minX) / (maxX - minX));
        ortho.d1 = -((maxY + minY) / (maxY - minY));
        ortho.d2 = -0.400016f;
        ortho.d3 = 0.0f;
        GxXformSetProjection(ortho);

        GxRsSet(GxRs_VertexShader, CGxDevice::s_uiVertexShader[1]);
        GxRsSet(GxRs_PixelShader, CGxDevice::s_uiPixelShader);

        C44Matrix viewProj;
        GxXformViewProjNativeTranspose(viewProj);
        GxShaderConstantsSet(GxSh_Vertex, 0, reinterpret_cast<const float*>(&viewProj), 4);
    } else {
        GxRsSet(GxRs_VertexShader, nullptr);
        GxRsSet(GxRs_PixelShader, nullptr);
    }

    CGxBuf* buf = GxBufStream(GxPoolTarget_Vertex, 24, 4);
    uint32_t* vertices = reinterpret_cast<uint32_t*>(GxBufLock(buf));
    if (!vertices) {
        return;
    }

    float invWidth = this->m_curWindowRect.maxX > 0.0f ? 1.0f / this->m_curWindowRect.maxX : 0.0f;
    float invHeight = this->m_curWindowRect.maxY > 0.0f ? 1.0f / this->m_curWindowRect.maxY : 0.0f;

    float left = static_cast<float>(x - this->m_cursorHotspotX) * invWidth;
    float right = static_cast<float>(x - this->m_cursorHotspotX + 32) * invWidth;
    float top = 1.0f - static_cast<float>(y - this->m_cursorHotspotY) * invHeight;
    float bottom = 1.0f - static_cast<float>(y - this->m_cursorHotspotY + 32) * invHeight;

    if (this->m_api == GxApi_D3d9 || this->m_api == GxApi_D3d9Ex) {
        left -= invWidth * 0.5f;
        right -= invWidth * 0.5f;
        top += invHeight * 0.5f;
        bottom += invHeight * 0.5f;
    }

    float* v = reinterpret_cast<float*>(vertices);

    v[0] = left;
    v[1] = top;
    v[2] = depth;
    vertices[3] = 0xFFFFFFFF;
    v[4] = 0.0f;
    v[5] = 0.0f;

    v[6] = left;
    v[7] = bottom;
    v[8] = depth;
    vertices[9] = 0xFFFFFFFF;
    v[10] = 0.0f;
    v[11] = 1.0f;

    v[12] = right;
    v[13] = top;
    v[14] = depth;
    vertices[15] = 0xFFFFFFFF;
    v[16] = 1.0f;
    v[17] = 0.0f;

    v[18] = right;
    v[19] = bottom;
    v[20] = depth;
    vertices[21] = 0xFFFFFFFF;
    v[22] = 1.0f;
    v[23] = 1.0f;

    GxBufUnlock(buf, 0);
    GxPrimVertexPtr(buf, GxVBF_PCT);

    CGxBatch batch;
    batch.m_primType = GxPrim_TriangleStrip;
    batch.m_start = 0;
    batch.m_count = 4;
    batch.m_minIndex = 0;
    batch.m_maxIndex = 3;
    GxDraw(&batch, 0);

    GxXformPop(GxXform_World);

    if (this->StereoEnabled()) {
        GxXformSetProjection(projection);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: d0a23aa3 auto
// OFFSET: 0x00689040
void CGxDevice::XformSetProjection(const C44Matrix& matrix) {
    this->m_projection = matrix;
}

// VERIFY: 1e7393f5 manual
// OFFSET: 0x0057C3A0
void CGxDevice::XformPush(EGxXform xf) {
    CGxMatrixStack& stack = this->m_xforms[xf];
    stack.m_level++;

    if (stack.m_level >= 3) {
        stack.m_level = 3;
    }

    stack.m_mtx[stack.m_level] = stack.m_mtx[stack.m_level - 1];
    stack.m_flags[stack.m_level] = stack.m_flags[stack.m_level - 1];
    stack.m_dirty = 1;
}

// VERIFY: a5c0394a auto
// OFFSET: 0x00682D20
EGxApi CGxDevice::DeviceApi() {
    return this->m_api;
}

// VERIFY: 8d0aae56 manual
// OFFSET: 0x00682DC0
int32_t CGxDevice::CapsIsWindowVisible(int32_t visible) {
    if (visible < 0) {
        return this->m_windowVisible;
    }

    this->m_windowVisible = visible > 0;
    return visible > 0;
}

// VERIFY: be5adc5f manual
// OFFSET: 0x00682E20
int32_t CGxDevice::CapsHasContext(int32_t context) {
    if (context < 0) {
        return this->m_context;
    }

    this->m_context = context > 0;
    return context > 0;
}

// VERIFY: 2c348324 auto
// OFFSET: 0x00683100
int32_t CGxDevice::MasterEnable(EGxMasterEnables state) {
    return ((1 << state) & this->m_appMasterEnables) != 0;
}

// VERIFY: c1bb2ca3 auto
// OFFSET: 0x00685370
void CGxDevice::DeviceClearScreenShot() {
    this->m_scrShotHeight = 0;
    this->m_scrShotWidth = 0;
    this->m_scrShotPixels.Clear();
}

// VERIFY: dfe43898 auto
// OFFSET: 0x00532AF0
CGxCaps& CGxDevice::Caps() {
    return this->m_caps;
}

// VERIFY: 6ef0a158 manual
// OFFSET: 0x00616A30
void CGxDevice::XformPushAndSet(EGxXform xf, const C44Matrix& matrix) {
    this->XformPush(xf);

    CGxMatrixStack& stack = this->m_xforms[xf];
    stack.m_dirty = 1;
    stack.m_flags[stack.m_level] &= ~CGxMatrixStack::F_Identity;
    stack.m_mtx[stack.m_level] = matrix;
}

// VERIFY: d13c8956 manual
// OFFSET: 0x00682C50
void CGxDevice::InitLights() {
    for (uint32_t i = 0; i < 4; i++) {
        this->m_lights[i].m_flags = 0xFF;
        this->m_lights[i].unk_0046 = 0xFF;
        this->m_lights[i].m_enable = 0;
    }
}

// VERIFY: dfbdc17f auto
// OFFSET: 0x00682D10
uint32_t CGxDevice::DeviceBaseMipLevel() {
    return this->m_baseMipLevel;
}

// VERIFY: 9260b546 auto
// OFFSET: 0x00682D40
int32_t CGxDevice::IDevIsWindowed() {
    return this->m_format.m_window;
}

// VERIFY: f81d278e auto
// OFFSET: 0x00682D70
const CRect& CGxDevice::DeviceCurWindow() {
    return this->m_curWindowRect;
}

// VERIFY: a019ace1 auto
// OFFSET: 0x00682D80
const CRect& CGxDevice::DeviceDefWindow() {
    return this->m_defWindowRect;
}

// VERIFY: e58361ab auto
// OFFSET: 0x00682D90
void CGxDevice::DeviceSetCurWindow(const CRect& rect) {
    this->m_curWindowRect = rect;
}

// VERIFY: a202f01f manual
// OFFSET: 0x00682DF0
int32_t CGxDevice::CapsWindowHasFocus(int32_t focus) {
    if (focus < 0) {
        return this->m_windowFocus;
    }

    this->m_windowFocus = focus > 0;
    return focus > 0;
}

// VERIFY: efe49605 auto
// OFFSET: 0x00682E70
void CGxDevice::SetScissor(const CRect& rect) {
    this->m_scissorRectDirty = 1;
    this->m_scissorRect = rect;
}

// VERIFY: 712d3c39 auto
// OFFSET: 0x00682EB0
void CGxDevice::PrimVertexMask(uint32_t mask) {
    this->m_primVertexDirty |= mask ^ this->m_primVertexMask;
    this->m_primVertexMask = mask;
    this->m_primVertexFormat = GxVertexBufferFormats_Last;
}

// VERIFY: a26b1a58 auto
// OFFSET: 0x00682F10
void CGxDevice::PrimIndexPtr(CGxBuf* buf) {
    if (buf->m_dirty || this->m_primIndexBuf != buf) {
        buf->m_dirty = 0;
        this->m_primIndexDirty = 1;
        this->m_primIndexBuf = buf;
    }
}

// VERIFY: 31489fa9 manual
// OFFSET: 0x00682F40
uint32_t CGxDevice::PrimCalcCount(EGxPrim primType, uint32_t indexCount) {
    uint32_t perPrim = s_primVertsPerPrim[primType];
    return (perPrim == 1 ? indexCount : indexCount / perPrim) - s_primVertsExtra[primType];
}

// VERIFY: 341ce950 manual
// OFFSET: 0x00683080
void CGxDevice::LightEnable(uint32_t index, int32_t enable) {
    CGxApiLight& light = this->m_lights[index];

    if (light.m_enable != enable) {
        light.m_flags |= 0x1;
        light.m_enable = enable;
    }
}

// VERIFY: 45b10c41 manual
// OFFSET: 0x006830B0
int32_t CGxDevice::NeedsUpdate(uint32_t app, uint32_t hw, uint32_t appDisables, uint32_t hwDisables, uint32_t flagPos, int32_t& enable) {
    enable = (app & (1 << flagPos) & ~appDisables) != 0;
    return enable != ((hw & (1 << flagPos) & ~hwDisables) != 0);
}

// VERIFY: 30703cd8 auto
// OFFSET: 0x006831A0
void CGxDevice::BufSizeSet(CGxBuf* buf, uint32_t itemSize, uint32_t itemCount) {
    buf->m_itemSize = itemSize;
    buf->m_itemCount = itemCount;
    buf->m_size = itemCount * itemSize;
    buf->m_hasData = 0;
}

// VERIFY: fdced826 auto
// OFFSET: 0x006831C0
int32_t CGxDevice::TexIsReadyNow(CGxTex* texId) {
    return texId->m_apiSpecificData && !texId->m_needsUpdate && !texId->m_needsCreation;
}

// VERIFY: 9e848655 auto
// OFFSET: 0x006831F0
int32_t CGxDevice::TexCanUpdate(CGxTex* texId) {
    return texId->m_userFunc != nullptr;
}

// VERIFY: 6ff3bc84 auto
// OFFSET: 0x00683320
void CGxDevice::TexSetCannotUpdate(CGxTex* texId) {
    texId->m_userFunc = nullptr;
    texId->m_userArg = nullptr;
}

// VERIFY: bec86cc4 auto
// OFFSET: 0x00683340
void CGxDevice::TexSetDataFormat(CGxTex* texId, EGxTexFormat dataFormat) {
    texId->m_dataFormat = dataFormat;
}

// VERIFY: 8d0ba344 auto
// OFFSET: 0x00683350
void CGxDevice::TexParameters(const CGxTex* texId, CGxTexParms& parms) {
    parms.m_target = texId->m_target;
    parms.m_width = texId->m_width;
    parms.m_height = texId->m_height;
    parms.m_depth = texId->m_depth;
    parms.m_format = texId->m_format;
    parms.m_dataFormat = texId->m_dataFormat;
    parms.m_flags = texId->m_flags;
    parms.m_userArg = texId->m_userArg;
    parms.m_userFunc = texId->m_userFunc;
}

// VERIFY: 48e3bca3 auto
// OFFSET: 0x006833A0
void CGxDevice::ShaderConstantsClear() {
    for (uint32_t i = 0; i < 256 * 4; i++) {
        s_shadowConstants[0].constants[i] = FLT_MAX;
    }

    for (uint32_t i = 0; i < 256 * 4; i++) {
        s_shadowConstants[1].constants[i] = FLT_MAX;
    }

    s_shadowConstants[1].dirtyMin = 255;
    s_shadowConstants[1].dirtyMax = 0;
    s_shadowConstants[0].dirtyMin = 255;
    s_shadowConstants[0].dirtyMax = 0;
}

// VERIFY: 4ce72d69 manual
// OFFSET: 0x006833E0
void CGxDevice::ShaderConstantsSet(EGxShTarget target, uint32_t index, const float* constants, uint32_t count) {
    ShadowConstants& shadow = s_shadowConstants[target ? 0 : 1];
    float* dest = &shadow.constants[index * 4];

    for (uint32_t i = index; i < index + count; i++) {
        int32_t changed = 0;

        for (uint32_t c = 0; c < 4; c++) {
            if (constants[c] != dest[c]) {
                dest[c] = constants[c];
                changed = 1;
            }
        }

        if (changed) {
            if (shadow.dirtyMin > i) {
                shadow.dirtyMin = i;
            }

            if (shadow.dirtyMax < i) {
                shadow.dirtyMax = i;
            }
        }

        dest += 4;
        constants += 4;
    }
}

// VERIFY: 713bcd86 manual
// OFFSET: 0x00683560
float* CGxDevice::ShaderConstantsLock(EGxShTarget target) {
    return s_shadowConstants[target ? 0 : 1].constants;
}

// VERIFY: 636e6d3d manual
// OFFSET: 0x00683580
void CGxDevice::ShaderConstantsUnlock(EGxShTarget target, uint32_t index, uint32_t count) {
    ShadowConstants& shadow = s_shadowConstants[target ? 0 : 1];

    if (shadow.dirtyMin > index) {
        shadow.dirtyMin = index;
    }

    if (shadow.dirtyMax < index + count - 1) {
        shadow.dirtyMax = index + count - 1;
    }
}

// VERIFY: ab58eaf0 auto
// OFFSET: 0x006835E0
void CGxDevice::ICursorDestroy() {
    if (this->m_cursorTexture) {
        GxTexDestroy(this->m_cursorTexture);
        this->m_cursorTexture = nullptr;
    }
}

// VERIFY: 63ceb0b1 auto
// OFFSET: 0x00683610
void CGxDevice::ICursorUpdate(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    if (command == GxTex_Latch) {
        texelStrideInBytes = 32 * sizeof(uint32_t);
        texels = static_cast<CGxDevice*>(userArg)->m_cursor;
    }
}

// VERIFY: 81324e32 auto
// OFFSET: 0x00683660
void CGxDevice::CursorSetDepth(float depth) {
    this->m_cursorDepth = depth;
}

// VERIFY: c79f0867 manual
// OFFSET: 0x00683EA0
void CGxDevice::ClampRectLeft(CiRect& windowRect) {
    if (windowRect.maxX - windowRect.minX - s_clientAdjust.x < kWinMinSize.x) {
        windowRect.minX = windowRect.maxX - s_clientAdjust.x - kWinMinSize.x;
    }
}

// VERIFY: c52fe6ce auto
// OFFSET: 0x00683ED0
void CGxDevice::ClampRectRight(CiRect& windowRect) {
    if (windowRect.maxX - windowRect.minX - s_clientAdjust.x < kWinMinSize.x) {
        windowRect.maxX = kWinMinSize.x + s_clientAdjust.x + windowRect.minX;
    }
}

// VERIFY: 641be1e3 manual
// OFFSET: 0x00683F00
void CGxDevice::ClampRectTop(CiRect& windowRect) {
    if (windowRect.maxY - windowRect.minY - s_clientAdjust.y < kWinMinSize.y) {
        windowRect.minY = windowRect.maxY - s_clientAdjust.y - kWinMinSize.y;
    }
}

// VERIFY: 4d3cc8da auto
// OFFSET: 0x00683F30
void CGxDevice::ClampRectBottom(CiRect& windowRect) {
    if (windowRect.maxY - windowRect.minY - s_clientAdjust.y < kWinMinSize.y) {
        windowRect.maxY = kWinMinSize.y + s_clientAdjust.y + windowRect.minY;
    }
}

// VERIFY: 0b022779 auto
// OFFSET: 0x006841B0
void CGxDevice::DeviceGamma(CGxGammaRamp& ramp) {
    ramp = this->m_systemGammaRamp;
}

// VERIFY: 17f58e3d manual
// OFFSET: 0x006841D0
void CGxDevice::DeviceScreenShot() {
    this->m_scrShotWidth = static_cast<uint32_t>(this->m_curWindowRect.maxX);
    this->m_scrShotHeight = static_cast<uint32_t>(this->m_curWindowRect.maxY);

    CiRect rect;
    rect.minY = 0;
    rect.minX = 0;
    rect.maxY = this->m_scrShotHeight;
    rect.maxX = this->m_scrShotWidth;
    this->DeviceReadPixels(rect, this->m_scrShotPixels);
}

// VERIFY: 5f171551 auto
// OFFSET: 0x00684360
void CGxDevice::DeviceSetDefWindow(const CRect& rect) {
    this->m_defWindowRect = rect;
    this->m_curWindowRect = rect;
}

// VERIFY: f4bada04 manual
// OFFSET: 0x00684440
void CGxDevice::ClipPlaneSet(uint32_t index, const C4Plane& plane) {
    C4Plane& current = this->m_clipPlane[index];

    if (plane.n.x != current.n.x || plane.n.y != current.n.y || plane.n.z != current.n.z || plane.d != current.d) {
        this->m_clipPlaneMask |= 1 << index;
        current.n.x = plane.n.x;
        current.n.y = plane.n.y;
        current.n.z = plane.n.z;
        current.d = plane.d;
    }
}

// VERIFY: dba46566 auto
// OFFSET: 0x006844C0
void CGxDevice::PrimVertexFormat(CGxBuf* buf, const CGxVertexAttrib* attribs, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        const CGxVertexAttrib& attrib = attribs[i];
        CGxVertexAttrib& current = this->m_primVertexFormatAttrib[attrib.m_attrib];

        if (buf->m_dirty || this->m_primVertexFormatBuf[attrib.m_attrib] != buf || ((current.m_offset - attrib.m_offset) | (current.m_type - attrib.m_type) | (current.m_bufSize - attrib.m_bufSize))) {
            this->m_primVertexDirty |= 1 << attrib.m_attrib;
        }

        this->m_primVertexFormatBuf[attrib.m_attrib] = buf;
        current = attrib;
    }

    buf->m_dirty = 0;
    this->m_primVertexFormat = GxVertexBufferFormats_Last;
}

// VERIFY: 73552709 manual
// OFFSET: 0x006847D0
void CGxDevice::LightSet(uint32_t index, const CGxLight& light, const C3Vector& pos) {
    CGxApiLight& apiLight = this->m_lights[index];
    apiLight = light;

    if ((light.m_flags & 0x2) && (pos.x != 0.0f || pos.y != 0.0f || pos.z != 0.0f)) {
        apiLight.m_flags |= 0x2;
        apiLight.m_dir.x = apiLight.m_dir.x - pos.x;
        apiLight.m_dir.y = apiLight.m_dir.y - pos.y;
        apiLight.m_dir.z = apiLight.m_dir.z - pos.z;
    }
}

// VERIFY: 97a9c89d manual
// OFFSET: 0x00684850
CGxBuf* CGxDevice::BufStream(EGxPoolTarget target, uint32_t itemSize, uint32_t itemCount) {
    CGxBuf* buf = this->m_streamBufs[target];
    CGxPool* pool = buf->m_pool;

    if (pool && static_cast<uint32_t>(pool->m_size) < itemCount * itemSize) {
        this->PoolSizeSet(pool, itemCount * itemSize);
    }

    buf->m_itemSize = itemSize;
    buf->m_size = itemCount * itemSize;
    buf->m_itemCount = itemCount;
    buf->m_hasData = 0;
    return buf;
}

// VERIFY: 06465d73 auto
// OFFSET: 0x006848A0
void CGxDevice::TexMarkForUpdate(CGxTex* texId, const CiRect& updateRect, int32_t immediate) {
    texId->m_needsUpdate = 1;

    if (updateRect.minY >= updateRect.maxY || updateRect.minX >= updateRect.maxX) {
        texId->m_updateRect.minY = 0;
        texId->m_updateRect.minX = 0;
        texId->m_updateRect.maxY = texId->m_height;
        texId->m_updateRect.maxX = texId->m_width;
    } else {
        texId->m_updateRect = updateRect;
    }

    if (immediate) {
        this->ITexMarkAsUpdated(texId);
    }
}

// VERIFY: a4937c7c auto
// OFFSET: 0x00684930
void CGxDevice::TexSetUserData(CGxTex* texId, GxTexUserFunc userFunc, void* userArg) {
    texId->m_userFunc = userFunc;
    texId->m_userArg = userArg;
    texId->m_updateRect.minY = 0;
    texId->m_updateRect.minX = 0;
    texId->m_updateRect.maxY = texId->m_height;
    texId->m_updateRect.maxX = texId->m_width;
    texId->m_needsUpdate = 1;
}

// VERIFY: 6017c596 manual
// OFFSET: 0x00684AD0
void CGxDevice::ICursorCreate(const CGxFormat& format) {
    this->m_hwCursor = format.m_hwCursor && this->m_caps.m_hwCursor;

    if (!this->m_hwCursor && !this->m_cursorTexture) {
        CGxTexFlags flags;
        flags.m_filter = GxTex_Linear;
        flags.m_wrapU = 0;
        flags.m_wrapV = 0;
        flags.m_forceMipTracking = 0;
        flags.m_generateMipMaps = 0;
        flags.m_renderTarget = 0;
        flags.m_maxAnisotropy = 1;
        GxTexCreate(32, 32, GxTex_Argb8888, flags, this, CGxDevice::ICursorUpdate, this->m_cursorTexture);
    }
}

// VERIFY: 4216d1f6 auto
// OFFSET: 0x00684B50
void CGxDevice::CursorUnlock(uint32_t x, uint32_t y) {
    this->m_cursorHotspotY = y;
    this->m_cursorHotspotX = x;

    if (this->m_cursorTexture) {
        CiRect rect;
        rect.maxY = 32;
        rect.maxX = 32;
        GxTexUpdate(this->m_cursorTexture, rect, 0);
    }
}

// VERIFY: efed35aa manual
// OFFSET: 0x00685C60
int32_t CGxDevice::TexCreate(EGxTexTarget target, uint32_t width, uint32_t height, uint32_t depth, EGxTexFormat format, EGxTexFormat dataFormat, CGxTexFlags flags, void* userArg, GxTexUserFunc userFunc, const char* name, CGxTex*& texId) {
    texId = new (SMemAlloc(sizeof(CGxTex), __FILE__, __LINE__, 0)) CGxTex(target, width, height, depth, format, dataFormat, flags, userArg, userFunc, name);
    this->m_textures.LinkToHead(texId);
    return 1;
}

// VERIFY: ff832600 manual
// OFFSET: 0x00687980
void CGxDevice::TexDestroy(CGxTex* texId) {
    texId->m_link.Unlink();
    delete texId;
}

// VERIFY: 87fe3127 manual
// OFFSET: 0x00688290
void CGxDevice::BufDestroy(CGxBuf*& buf) {
    if (!buf) {
        return;
    }

    if (buf->m_pool) {
        buf->m_pool->UnlinkBuf(buf);
    }

    buf->m_link.Unlink();
    delete buf;
    buf = nullptr;
}

// VERIFY: e4c51956 auto
// OFFSET: 0x00682EE0
void CGxDevice::PrimVertexPtr(CGxBuf* buf, EGxVertexBufferFormat format) {
    this->m_primVertexFormat = format;
    this->m_primVertexBuf = buf;
    this->m_primVertexSize = Buffer::s_vertexBufDesc[format].size;
}

// VERIFY: dabc8a7e manual
// OFFSET: 0x00409670
void CGxDevice::RsPush() {
    uint32_t offset = this->m_pushedStates.Count();
    *this->m_stackOffsets.New() = offset;
}

// VERIFY: 6c531365 manual
// OFFSET: 0x00685970
void CGxDevice::IRsDirty(EGxRenderState which) {
    CGxAppRenderState& state = this->m_appRenderStates[which];

    if (!state.m_dirty) {
        *this->m_dirtyStates.New() = which;
        state.m_dirty = 1;
    }

    if (state.m_stackDepth != this->m_stackOffsets.Count()) {
        CGxPushedRenderState* pushed = this->m_pushedStates.New();
        pushed->m_which = which;
        pushed->m_value = state.m_value;
        pushed->m_stackDepth = state.m_stackDepth;
        state.m_stackDepth = this->m_stackOffsets.Count();
    }
}

// VERIFY: 2dc8f39a manual
// OFFSET: 0x006859E0
void CGxDevice::IRsForceUpdate(EGxRenderState which) {
    if (!this->m_context) {
        return;
    }

    uint32_t count = this->m_dirtyStates.Count();
    this->m_dirtyStates.SetCount(count + 1);
    this->m_dirtyStates[count] = which;

    CGxAppRenderState& state = this->m_appRenderStates[which];
    state.m_dirty = 1;

    CGxStateBom& hw = this->m_hwRenderStates[which];
    hw.m_data.i[0] = ~state.m_value.m_data.i[0];
    hw.m_data.i[1] = ~state.m_value.m_data.i[1];
    hw.m_data.i[2] = ~state.m_value.m_data.i[2];
    hw.m_filler = ~state.m_value.m_filler;
}

// VERIFY: a888e50a manual
// OFFSET: 0x00685A70
void CGxDevice::IRsForceUpdate() {
    for (uint32_t which = 0; which < GxRenderStates_Last; which++) {
        *this->m_dirtyStates.New() = static_cast<EGxRenderState>(which);

        CGxAppRenderState& state = this->m_appRenderStates[which];
        state.m_dirty = 1;

        CGxStateBom& hw = this->m_hwRenderStates[which];
        hw.m_data.i[0] = ~state.m_value.m_data.i[0];
        hw.m_data.i[1] = ~state.m_value.m_data.i[1];
        hw.m_data.i[2] = ~state.m_value.m_data.i[2];
        hw.m_filler = ~state.m_value.m_filler;
    }
}

// VERIFY: ef9949e8 manual
// OFFSET: 0x00685B50
void CGxDevice::IRsSync(int32_t force) {
    if (force) {
        this->IRsForceUpdate();
    }

    for (uint32_t i = this->m_dirtyStates.Count(); i; i--) {
        EGxRenderState which = this->m_dirtyStates[i - 1];
        CGxAppRenderState& state = this->m_appRenderStates[which];
        CGxStateBom& hw = this->m_hwRenderStates[which];

        if (state.m_dirty) {
            if ((state.m_value.m_data.i[0] - hw.m_data.i[0]) | (state.m_value.m_data.i[1] - hw.m_data.i[1]) | (state.m_value.m_data.i[2] - hw.m_data.i[2]) | (state.m_value.m_filler - hw.m_filler)) {
                this->IRsSendToHw(which);
            }
        }

        hw = state.m_value;
        state.m_dirty = 0;
    }

    this->m_dirtyStates.SetCount(0);
}

// VERIFY: 57344373 manual
// OFFSET: 0x00685CD0
void CGxDevice::TexSetFlags(CGxTex* texId, CGxTexFlags flags) {
    if (!(texId->m_flags != flags)) {
        return;
    }

    if (texId->m_flags.m_filter != flags.m_filter) {
        texId->m_char5D = 0;
    }

    texId->m_flags = flags;
    texId->m_needsFlagUpdate = 1;

    for (uint32_t i = GxRs_Texture0; i <= GxRs_Texture15; i++) {
        if (this->m_hwRenderStates[i].m_data.p == texId) {
            this->IRsForceUpdate(static_cast<EGxRenderState>(i));
        }
    }
}

// VERIFY: 1b273417 manual
// OFFSET: 0x00685D40
void CGxDevice::TexSetWrap(CGxTex* texId, EGxTexWrapMode wrapU, EGxTexWrapMode wrapV) {
    if (texId->m_flags.m_wrapU == static_cast<uint32_t>(wrapU) && texId->m_flags.m_wrapV == static_cast<uint32_t>(wrapV)) {
        return;
    }

    texId->m_flags.m_wrapU = wrapU;
    texId->m_flags.m_wrapV = wrapV;
    texId->m_needsFlagUpdate = 1;

    for (uint32_t i = GxRs_Texture0; i <= GxRs_Texture15; i++) {
        if (this->m_hwRenderStates[i].m_data.p == texId) {
            this->IRsForceUpdate(static_cast<EGxRenderState>(i));
        }
    }
}

// VERIFY: 69ad163c auto
// OFFSET: 0x00685EB0
void CGxDevice::MasterEnableSet(EGxMasterEnables state, int32_t enable) {
    this->m_appMasterEnables = ((enable & 1) << state) | (this->m_appMasterEnables & ~(1 << state));

    switch (state) {
        case GxMasterEnable_Lighting:
            this->IRsForceUpdate(GxRs_Lighting);
            break;

        case GxMasterEnable_Fog:
            this->IRsForceUpdate(GxRs_Fog);
            break;

        case GxMasterEnable_DepthTest:
            this->IRsForceUpdate(GxRs_DepthTest);
            break;

        case GxMasterEnable_DepthWrite:
            this->IRsForceUpdate(GxRs_DepthWrite);
            break;

        case GxMasterEnable_ColorWrite:
            this->IRsForceUpdate(GxRs_ColorWrite);
            break;

        case GxMasterEnable_Culling:
            this->IRsForceUpdate(GxRs_Culling);
            break;

        default:
            break;
    }
}

// VERIFY: 5ca93879 auto
// OFFSET: 0x00685F50
void CGxDevice::RsSet(EGxRenderState which, void* value) {
    if (!this->m_context) {
        return;
    }

    CGxAppRenderState& state = this->m_appRenderStates[which];

    if (state.m_value.m_data.p == value) {
        return;
    }

    this->IRsDirty(which);
    state.m_value.m_data.p = value;

    if (value && which >= GxRs_Texture0 && which <= GxRs_Texture15) {
        CGxTex* texId = static_cast<CGxTex*>(value);

        if (texId->m_flags.m_renderTarget && texId->m_needsUpdate) {
            this->ITexMarkAsUpdated(texId);
        }
    }
}

// VERIFY: cbb13905 manual
// OFFSET: 0x00685FB0
void CGxDevice::RsPop() {
    uint32_t offset = this->m_stackOffsets[this->m_stackOffsets.Count() - 1];

    for (uint32_t i = this->m_pushedStates.Count(); i > offset; i--) {
        CGxPushedRenderState& pushed = this->m_pushedStates[i - 1];
        CGxAppRenderState& state = this->m_appRenderStates[pushed.m_which];

        if (!state.m_dirty) {
            *this->m_dirtyStates.New() = pushed.m_which;
            state.m_dirty = 1;
        }

        state.m_value = pushed.m_value;
        state.m_stackDepth = pushed.m_stackDepth;
    }

    this->m_pushedStates.SetCount(offset);
    this->m_stackOffsets.SetCount(this->m_stackOffsets.Count() - 1);
}

// VERIFY: 8af231b1 manual
// OFFSET: 0x00686120
void CGxDevice::IRsInit() {
    this->m_appRenderStates.SetCount(GxRenderStates_Last);
    this->m_hwRenderStates.SetCount(GxRenderStates_Last);
    memset(this->m_appRenderStates.Ptr(), 0, sizeof(CGxAppRenderState) * GxRenderStates_Last);
    memset(this->m_hwRenderStates.Ptr(), 0, sizeof(CGxStateBom) * GxRenderStates_Last);

    this->m_appRenderStates[GxRs_PolygonOffset].m_value.m_data.i[0] = 0;
    this->m_appRenderStates[GxRs_MatDiffuse].m_value.m_data.i[0] = -1;
    this->m_appRenderStates[GxRs_MatEmissive].m_value.m_data.i[0] = 0;
    this->m_appRenderStates[GxRs_MatSpecular].m_value.m_data.i[0] = 0;
    this->m_appRenderStates[GxRs_MatSpecularExp].m_value.m_data.f[0] = 0.0f;
    this->m_appRenderStates[GxRs_NormalizeNormals].m_value.m_data.i[0] = 0;
    this->m_appRenderStates[GxRs_BlendingMode].m_value.m_data.i[0] = 0;
    this->m_appRenderStates[GxRs_FogStart].m_value.m_data.f[0] = 1.0f;
    this->m_appRenderStates[GxRs_FogEnd].m_value.m_data.f[0] = 0.0f;
    this->m_appRenderStates[GxRs_FogColor].m_value.m_data.i[0] = 0xFF808080;
    this->m_appRenderStates[GxRs_Lighting].m_value.m_data.i[0] = 1;
    this->m_appRenderStates[GxRs_Fog].m_value.m_data.i[0] = 1;
    this->m_appRenderStates[GxRs_DepthTest].m_value.m_data.i[0] = 1;
    this->m_appRenderStates[GxRs_DepthFunc].m_value.m_data.i[0] = GxCmp_LEqual;
    this->m_appRenderStates[GxRs_DepthWrite].m_value.m_data.i[0] = 1;
    this->m_appRenderStates[GxRs_ColorWrite].m_value.m_data.i[0] = 15;
    this->m_appRenderStates[GxRs_Culling].m_value.m_data.i[0] = 1;
    this->m_appRenderStates[GxRs_ClipPlaneMask].m_value.m_data.i[0] = 0;
    this->m_appRenderStates[GxRs_Multisample].m_value.m_data.i[0] = 1;
    this->m_appRenderStates[GxRs_ScissorTest].m_value.m_data.i[0] = 0;

    for (uint32_t i = 0; i < 8; i++) {
        this->m_appRenderStates[GxRs_Texture0 + i].m_value.m_data.i[0] = 0;
        this->m_appRenderStates[GxRs_ColorOp0 + i].m_value.m_data.i[0] = GxTexOp_Mod;
        this->m_appRenderStates[GxRs_AlphaOp0 + i].m_value.m_data.i[0] = GxTexOp_Mod;
        this->m_appRenderStates[GxRs_TexGen0 + i].m_value.m_data.i[0] = 0;
        this->m_appRenderStates[GxRs_TextureShader0 + i].m_value.m_data.i[0] = 0;
        this->m_appRenderStates[GxRs_TextureCoord0 + i].m_value.m_data.i[0] = i;
    }

    for (uint32_t i = GxRs_Texture8; i <= GxRs_Texture15; i++) {
        this->m_appRenderStates[i].m_value.m_data.i[0] = 0;
    }

    this->m_appRenderStates[GxRs_PixelShader].m_value.m_data.i[0] = 0;
    this->m_appRenderStates[GxRs_VertexShader].m_value.m_data.i[0] = 0;
    this->m_appRenderStates[GxRs_PointScale].m_value.m_data.f[0] = 1.0f;
    this->m_appRenderStates[GxRs_PointScaleAttenuation].m_value.m_data.f[0] = s_pointScaleAttenuation.x;
    this->m_appRenderStates[GxRs_PointScaleAttenuation].m_value.m_data.f[1] = s_pointScaleAttenuation.y;
    this->m_appRenderStates[GxRs_PointScaleAttenuation].m_value.m_data.f[2] = s_pointScaleAttenuation.z;
    this->m_appRenderStates[GxRs_PointScaleMin].m_value.m_data.f[0] = 0.0f;
    this->m_appRenderStates[GxRs_PointScaleMax].m_value.m_data.f[0] = 1.0f;
    this->m_appRenderStates[GxRs_PointSprite].m_value.m_data.i[0] = 0;
    this->m_appRenderStates[GxRs_BlendFactor].m_value.m_data.f[0] = 0.0f;
    this->m_appRenderStates[GxRs_ColorMaterial].m_value.m_data.i[0] = 0;
}

// VERIFY: 255ba7d0 manual
// OFFSET: 0x00689050
void CGxDevice::XformSetView(const C44Matrix& matrix) {
    CGxMatrixStack& stack = this->m_xforms[GxXform_View];
    stack.m_dirty = 1;
    stack.m_flags[stack.m_level] &= ~CGxMatrixStack::F_Identity;
    stack.m_mtx[stack.m_level] = matrix;

    for (uint32_t i = 0; i < 8; i++) {
        if (static_cast<uint32_t>(this->m_appRenderStates[GxRs_TexGen0 + i].m_value.m_data.i[0] - 1) <= 1) {
            this->IRsForceUpdate(static_cast<EGxRenderState>(GxRs_TexGen0 + i));
        }
    }
}

// VERIFY: b93f77d5 auto
// OFFSET: 0x00683670
void CGxDevice::LogOpen() {
    if (!CGxDevice::m_log) {
        OsCreateDirectory("Logs", 0);
        SLogCreate("Logs\\gx.log", 0, &CGxDevice::m_log);
        CGxDevice::m_logBytes = 0;
    }
}

// VERIFY: 1023b26f auto
// OFFSET: 0x006836B0
void CGxDevice::LogClose() {
    if (CGxDevice::m_log) {
        SLogClose(CGxDevice::m_log);
        CGxDevice::m_log = nullptr;
    }
}

// VERIFY: 7008afc9 auto
// OFFSET: 0x00683CE0
void CGxDevice::ClampRectToWindow(CiRect& rect) {
    CiRect window;
    window.minY = static_cast<int32_t>(this->m_curWindowRect.minY);
    window.minX = static_cast<int32_t>(this->m_curWindowRect.minX);
    window.maxY = static_cast<int32_t>(this->m_curWindowRect.maxY);
    window.maxX = static_cast<int32_t>(this->m_curWindowRect.maxX);
    rect = rect & window;
}

// VERIFY: 9718483f auto
// OFFSET: 0x00683D60
void CGxDevice::ClampRectTopByAspect(CiRect& windowRect) {
    if (kWinAspect != 0.0f) {
        windowRect.minY = windowRect.maxY - static_cast<int32_t>(static_cast<float>(windowRect.maxX - windowRect.minX - s_clientAdjust.x) / kWinAspect + 0.5f + s_clientAdjust.y);
    }
}

// VERIFY: d339c793 auto
// OFFSET: 0x00683DB0
void CGxDevice::ClampRectBottomByAspect(CiRect& windowRect) {
    if (kWinAspect != 0.0f) {
        windowRect.maxY = windowRect.minY + static_cast<int32_t>(static_cast<float>(windowRect.maxX - windowRect.minX - s_clientAdjust.x) / kWinAspect + 0.5f + s_clientAdjust.y);
    }
}

// VERIFY: 46215ef6 auto
// OFFSET: 0x00683E00
void CGxDevice::ClampRectLeftByAspect(CiRect& windowRect) {
    if (kWinAspect != 0.0f) {
        windowRect.minX = windowRect.maxX - static_cast<int32_t>(kWinAspect * static_cast<float>(windowRect.maxY - windowRect.minY - s_clientAdjust.y) + 0.5f + s_clientAdjust.x);
    }
}

// VERIFY: 9342553f auto
// OFFSET: 0x00683E50
void CGxDevice::ClampRectRightByAspect(CiRect& windowRect) {
    if (kWinAspect != 0.0f) {
        windowRect.maxX = windowRect.minX + static_cast<int32_t>(kWinAspect * static_cast<float>(windowRect.maxY - windowRect.minY - s_clientAdjust.y) + 0.5f + s_clientAdjust.x);
    }
}

// VERIFY: 5e425514 manual
// OFFSET: 0x00684BA0
void CGxDevice::Log(const char* format, va_list args) {
    if (!CGxDevice::m_log) {
        return;
    }

    char buffer[2048];
    _vsnprintf(buffer, sizeof(buffer), format, args);

    if (CGxDevice::m_logBytes < 0x200000) {
        SLogWrite(CGxDevice::m_log, buffer);
        CGxDevice::m_logBytes += static_cast<uint32_t>(strlen(buffer));
    }
}

// VERIFY: 4fc450e6 auto
// OFFSET: 0x00684C20
void CGxDevice::Log(const char* format, ...) {
    va_list args;
    va_start(args, format);

    if (CGxDevice::m_log) {
        CGxDevice::Log(format, args);
    }

    va_end(args);
}

// VERIFY: e580fa38 auto
// OFFSET: 0x00684C40
void CGxDevice::Log(const CGxCaps& caps) const {
    CGxDevice::Log("Caps:");
    CGxDevice::Log("\tnumTmus: %d", caps.m_numTmus);
    CGxDevice::Log("\tgenerateMipMaps: %d", caps.m_generateMipMaps);
    CGxDevice::Log("\ttexFilterAnisotropic: %d, %d", caps.m_texFilterTrilinear, caps.m_maxTexAnisotropy);
    CGxDevice::Log("\trttFormat: %d, %d", caps.m_rttFormat[2], caps.m_rttFormat[5]);
    CGxDevice::Log("\tpixelShaderTarget: %s", s_pixelShaderTargetNames[caps.m_shaderTargets[GxSh_Pixel]]);
    CGxDevice::Log("\tvertexShaderTarget: %s", s_vertexShaderTargetNames[caps.m_shaderTargets[GxSh_Vertex]]);
    CGxDevice::Log("\tvertexShaderConstants: %d", caps.m_shaderConstants[GxSh_Vertex]);
    CGxDevice::Log("\tnumStreams: %d", caps.m_numStreams);
    CGxDevice::Log("\tstereoAvailable: %d", caps.m_stereoAvailable);
}

// VERIFY: e3c13afb auto
// OFFSET: 0x00684D10
void CGxDevice::Log(const CGxFormat& format) const {
    if (format.m_window) {
        CGxDevice::Log("\tFormat: %d x %d Window, %s, multisample %d", format.m_size.x, format.m_size.y, s_formatNames[format.m_depthFormat], format.m_multisampleCount);
    } else {
        CGxDevice::Log("\tFormat %d x %d @ %d Fullscreen, %s, %s, multisample %d", format.m_size.x, format.m_size.y, format.m_refreshRate, s_formatNames[format.m_colorFormat], s_formatNames[format.m_depthFormat], format.m_multisampleCount);
    }
}

// VERIFY: d7e6dbd8 manual
// OFFSET: 0x00688690
CGxDevice::CGxDevice() : m_defWindowRect(), m_curWindowRect(), m_viewport(), m_scissorRect(), m_appMasterEnables(0), m_hwMasterEnables(0) {
    this->IRsInit();
    this->m_frameCount = 0;
    this->m_primType = GxPrims_Last;
    this->m_api = GxApis_Last;
    this->m_windowProc = nullptr;
    this->m_cpuFeatures = OsGetProcessorFeatures();
    memset(&this->m_caps, 0, sizeof(this->m_caps));
    this->m_baseMipLevel = 0;
    this->m_caps.m_maxTexAnisotropy = 1;
    this->m_viewport.m_x.m_l = 0.0f;
    this->m_viewport.m_x.m_h = 1.0f;
    this->m_viewport.m_y.m_l = 0.0f;
    this->m_viewport.m_y.m_h = 1.0f;
    this->m_viewport.m_z.m_l = 0.0f;
    this->m_viewport.m_z.m_h = 1.0f;
    this->m_viewportDirty = 1;
    this->InitLights();
    this->m_appMasterEnables = 0x1FF;
    this->m_hwMasterEnables = 0x1FF;
    this->m_scrShotClick = 0;
    this->m_scrShotWidth = 0;
    this->m_scrShotHeight = 0;
    this->m_indexLocked = 0;
    this->m_vertexLocked = 0;
    this->m_inBeginEnd = 0;
    this->m_bufLocked[0] = nullptr;
    this->m_bufLocked[1] = nullptr;
    memset(this->m_primVertexFormatBuf, 0, sizeof(this->m_primVertexFormatBuf));
    this->m_vertexPool = nullptr;
    this->m_indexPool = nullptr;
    this->m_streamBufs[0] = nullptr;
    this->m_streamBufs[1] = nullptr;
    this->m_clipPlaneMask = 0;
    memset(this->m_clipPlane, 0, sizeof(this->m_clipPlane));
    this->m_scissorRect.minY = 0.0f;
    this->m_scissorRect.minX = 0.0f;
    this->m_scissorRect.maxY = 0.0f;
    this->m_scissorRect.maxX = 0.0f;
    this->m_scissorRectDirty = 1;
    this->m_primVertexMask = 0;
    this->m_primVertexDirty = 0;
    this->m_primVertexFormat = GxVertexBufferFormats_Last;
    this->m_primVertexBuf = nullptr;
    this->m_primIndexBuf = nullptr;
    this->m_primIndexDirty = 0;
    this->m_gammaRamp.Set(1.0f);
    this->m_systemGammaRamp.Set(1.0f);

    for (uint32_t i = 0; i < 2; i++) {
        this->m_textureTarget[i].m_texture = nullptr;
        this->m_textureTarget[i].m_plane = 0;
        this->m_textureTarget[i].m_apiSpecific = nullptr;
    }

    this->m_cursorVisible = 1;
    this->m_hwCursor = 0;
    this->m_cursorHotspotX = 0;
    this->m_cursorHotspotY = 0;
    memset(this->m_cursor, 0, sizeof(this->m_cursor));
    this->m_cursorDepth = 1.0f;
    this->m_cursorTexture = nullptr;
    this->m_context = 0;
    this->m_intF5C = 0;
    this->m_windowVisible = 0;
    this->m_windowFocus = 0;
    this->ShaderConstantsClear();
}

// VERIFY: ea9f4c67 manual
// OFFSET: [0x006890C0, 0x00689690]
CGxDevice::~CGxDevice() {
    for (CGxTex* texId = this->m_textures.Head(); texId; texId = this->m_textures.Next(texId)) {
        if (texId->m_apiSpecificData) {
            char buffer[256];
            sprintf(buffer, "Unfreed texture (%d x %d) with user function address %08x.", texId->m_width, texId->m_height, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(texId->m_userFunc)));
            DebugLog(buffer);
        }
    }

    this->BufDestroy(this->m_streamBufs[0]);
    this->BufDestroy(this->m_streamBufs[1]);
    this->PoolDestroy(this->m_vertexPool);
    this->PoolDestroy(this->m_indexPool);
    this->m_bufList.Clear();
    this->m_poolList.Clear();
}

// VERIFY: aaaa19ed manual
// OFFSET: 0x00687660
CGxBuf* CGxDevice::BufCreate(CGxPool* pool, uint32_t itemSize, uint32_t itemCount, uint32_t index) {
    CGxBuf* buf = new (SMemAlloc(sizeof(CGxBuf), __FILE__, __LINE__, 0)) CGxBuf(pool, itemSize, itemCount, index);
    pool->m_bufList.LinkToTail(buf);
    buf->m_pool = pool;
    return buf;
}

// VERIFY: f38621c0 manual
// OFFSET: 0x006876D0
CGxPool* CGxDevice::PoolCreate(EGxPoolTarget target, EGxPoolUsage usage, int32_t size, EGxPoolHintBits hint, const char* name) {
    CGxPool* pool = new (SMemAlloc(sizeof(CGxPool), __FILE__, __LINE__, 0)) CGxPool(target, usage, size, hint, name);
    this->m_poolList.LinkToTail(pool);
    return pool;
}

// VERIFY: 9730452c manual
// OFFSET: 0x00688340
void CGxDevice::PoolDestroy(CGxPool* pool) {
    if (pool) {
        this->m_poolList.UnlinkNode(pool);
        delete pool;
    }
}

// VERIFY: 7426c187 auto
// OFFSET: 0x00687900
void CGxDevice::DeviceCreateStreamBufs() {
    this->m_streamBufs[0] = this->BufCreate(this->m_vertexPool, 0, 0, 0);
    this->m_streamBufs[1] = this->BufCreate(this->m_indexPool, 0, 0, 0);
}

// VERIFY: 9a2a4bda auto
// OFFSET: 0x00687940
void CGxDevice::DeviceCreatePools() {
    this->m_vertexPool = this->PoolCreate(GxPoolTarget_Vertex, GxPoolUsage_Stream, s_streamVertexPoolSize, GxPoolHintBit_Unk3, "Stream_vtx");
    this->m_indexPool = this->PoolCreate(GxPoolTarget_Index, GxPoolUsage_Stream, s_streamIndexPoolSize, GxPoolHintBit_Unk3, "Stream_idx");
}

// VERIFY: 3ce50085 manual
// OFFSET: 0x00684970
void CGxDevice::IShaderLoad(CGxShader** shaders, EGxShTarget target, const char* path, const char* name, uint32_t permutations) {
    enum : uint32_t {
        Magic = 0x47585348, // 'GXSH'
        Version = 0x10003,
    };

    int32_t profile = this->m_caps.m_shaderTargets[target];

    if (profile <= 0) {
        return;
    }

    while (true) {
        char filename[260];
        sprintf(filename, "%s\\%s\\%s.bls", path, s_shaderTargetNames[target][profile], name);

        SFileImpl* file;
        SFileOpenFile(filename, &file);

        if (file) {
            uint32_t magic;
            SFileReadFile(file, &magic, 4, nullptr, nullptr, 0);

            if (magic == 'GXSH') {
                uint32_t version;
                SFileReadFile(file, &version, 4, nullptr, nullptr, 0);

                if (version == 0x10003) {
                    uint32_t unknown;
                    SFileReadFile(file, &unknown, 4, nullptr, nullptr, 0);

                    for (uint32_t i = 0; i < permutations; i++) {
                        shaders[i]->Load(file);
                        shaders[i]->m_loaded = 0;
                        shaders[i]->unused_0034 = 0;
                    }

                    profile = 0;
                }
            }

            SFileCloseFile(file);
        }

        if (target == GxSh_Vertex) {
            switch (profile) {
                case 2:
                    profile = 1;
                    break;

                case 3:
                    profile = 2;
                    break;

                default:
                    return;
            }
        } else if (target == GxSh_Pixel) {
            switch (profile) {
                case 2:
                    profile = 1;
                    break;

                case 3:
                    profile = 2;
                    break;

                case 4:
                    profile = 3;
                    break;

                case 9:
                case 10:
                    profile = 8;
                    break;

                default:
                    return;
            }
        } else {
            return;
        }
    }
}

// VERIFY: 9b12ed92 manual
// OFFSET: 0x006897C0
void CGxDevice::ShaderCreate(CGxShader** shaders, EGxShTarget target, const char* path, const char* name, uint32_t permutations) {
    if (permutations == 1) {
        shaders[0] = this->m_shaderList[target].Ptr(name);

        if (shaders[0]) {
            shaders[0]->m_refCount++;
            return;
        }

        shaders[0] = this->m_shaderList[target].New(name, 0, 0);
        shaders[0]->m_refCount++;
        shaders[0]->m_target = target;
    } else {
        char buffer[256];

        memset(shaders, 0, sizeof(CGxShader*) * permutations);

        uint32_t i;

        for (i = 0; i < permutations; i++) {
            sprintf(buffer, "%s:%d", name, i);
            shaders[i] = this->m_shaderList[target].Ptr(buffer);

            if (!shaders[i]) {
                break;
            }
        }

        if (i >= permutations) {
            for (i = 0; i < permutations; i++) {
                if (shaders[i]) {
                    shaders[i]->m_refCount++;
                }
            }

            return;
        }

        for (i = 0; i < permutations; i++) {
            sprintf(buffer, "%s:%d", name, i);
            shaders[i] = this->m_shaderList[target].New(buffer, 0, 0);
            shaders[i]->m_refCount++;
            shaders[i]->m_target = target;
        }
    }

    this->IShaderLoad(shaders, target, path, name, permutations);
}

// VERIFY: 10cd5175 manual
// OFFSET: 0x00687820
void CGxDevice::ShaderDestroy(CGxShader*& shader) {
    shader->m_refCount--;

    if (!shader->m_refCount) {
        this->m_shaderList[shader->m_target].Delete(shader);
    }

    shader = nullptr;
}

// VERIFY: 4f26ed72 manual
// OFFSET: 0x00684FE0
void CGxDevice::DeviceOverride(EGxOverride override, uint32_t value) {
    CGxDevice::Log("CGxDevice::DeviceOverride(): %d set to %d", override, value);

    if (override == GxOverride_Unk8) {
        int32_t enable = value > 1;
        this->m_caps.m_pointParameters = enable;
        this->m_caps.m_pointSprite = enable;
        this->m_caps.m_pointScaleMax = static_cast<float>(value);
    }
}

// VERIFY: 0ed001fc manual
// OFFSET: 0x006845B0
void CGxDevice::PrimDrawElements() {
    this->m_inBeginEnd = 0;

    if (this->m_primVertexArray.Count()) {
        GxPrimLockVertexPtrs(this->m_primVertexArray.Count(), this->m_primVertexArray.m_data, sizeof(C3Vector), this->m_primNormalArray.m_data, sizeof(C3Vector), this->m_primColorArray.m_data, sizeof(CImVector), nullptr, 0, this->m_primTexCoordArray[0].m_data, sizeof(C2Vector), this->m_primTexCoordArray[1].m_data, sizeof(C2Vector));
        GxPrimLockIndexPtr(this->m_primType, this->m_primIndexArray.Count(), this->m_primIndexArray.m_data);
        GxDrawLockedElements();
    }
}

// VERIFY: e7eb4342 auto
// OFFSET: 0x006836D0
void CGxDevice::WaitForFPSCap() {
    uint32_t maxFps = GxGetMaxFPS() ? GxGetMaxFPS() : 0xFFFFFFFF;
    uint32_t maxFpsBk = GxGetMaxFPSBk() ? GxGetMaxFPSBk() : 0xFFFFFFFF;

    if (!this->m_windowFocus && maxFps >= maxFpsBk) {
        maxFps = maxFpsBk;
    }

    if (maxFps <= 8) {
        maxFps = 8;
    } else if (maxFps == 0xFFFFFFFF) {
        return;
    }

    uint32_t frameMs = 1000 / maxFps;
    uint32_t now = OsGetAsyncTimeMs();

    if (s_nextFrameTime - now < 1000) {
        OsSleep(s_nextFrameTime - now);
        now = OsGetAsyncTimeMs();
    }

    s_nextFrameTime = now + frameMs;
}

// VERIFY: c1bc4801 manual
// OFFSET: 0x006A5EF0
void CGxDevice::ITexWHDStartEnd(CGxTex* texId, uint32_t& width, uint32_t& height, uint32_t& baseMip, uint32_t& mipCount) {
    width = texId->m_width;
    height = texId->m_height;

    if ((texId->m_flags.m_filter == GxTex_Nearest || texId->m_flags.m_filter == GxTex_Linear || texId->m_flags.m_generateMipMaps) && !texId->m_flags.m_forceMipTracking) {
        baseMip = 0;
        mipCount = 1;
        return;
    }

    if (texId->m_format == GxTex_Dxt1 || texId->m_format == GxTex_Dxt3 || texId->m_format == GxTex_Dxt5) {
        uint32_t size = texId->m_width < texId->m_height ? texId->m_width : texId->m_height;
        mipCount = 1;

        while (size > 4) {
            size >>= 1;
            mipCount++;
        }
    } else {
        uint32_t size = texId->m_width > texId->m_height ? texId->m_width : texId->m_height;
        mipCount = 1;

        while (size != 1) {
            size >>= 1;
            mipCount++;
        }
    }

    baseMip = this->m_baseMipLevel < mipCount - 1 ? this->m_baseMipLevel : mipCount - 1;
    width >>= baseMip;
    height >>= baseMip;

    if (texId->m_flags.m_forceMipTracking) {
        mipCount = baseMip + 1;
    }
}

// VERIFY: 4458969e manual
// OFFSET: 0x00682D50
void CGxDevice::DeviceGetRenderTarget(EGxBuffer buffer, CGxTex*& texture) {
    texture = this->m_textureTarget[buffer].m_texture;
}

// INLINED
void CGxDevice::RsSet(EGxRenderState which, int32_t value) {
    if (!this->m_context) {
        return;
    }

    CGxAppRenderState& state = this->m_appRenderStates[which];

    if (state.m_value.m_data.p != reinterpret_cast<void*>(static_cast<intptr_t>(value))) {
        this->IRsDirty(which);
        state.m_value.m_data.p = reinterpret_cast<void*>(static_cast<intptr_t>(value));
    }
}

// INLINED
void CGxDevice::RsSet(EGxRenderState which, float value) {
    if (!this->m_context) {
        return;
    }

    CGxAppRenderState& state = this->m_appRenderStates[which];

    if (value != state.m_value.m_data.f[0]) {
        this->IRsDirty(which);
        state.m_value.m_data.f[0] = value;
    }
}

// INLINED
void CGxDevice::RsSet(EGxRenderState which, const C3Vector& value) {
    if (!this->m_context) {
        return;
    }

    CGxAppRenderState& state = this->m_appRenderStates[which];

    if (value.x != state.m_value.m_data.f[0] || value.y != state.m_value.m_data.f[1] || value.z != state.m_value.m_data.f[2]) {
        this->IRsDirty(which);
        state.m_value.m_data.f[0] = value.x;
        state.m_value.m_data.f[1] = value.y;
        state.m_value.m_data.f[2] = value.z;
    }
}

// VERIFY: 737cf993 manual
// OFFSET: 0x00682FD0
void CGxDevice::Light(uint32_t index, CGxLight& light) {
    CGxApiLight& apiLight = this->m_lights[index];
    light.m_flags ^= (light.m_flags ^ apiLight.m_enable) & 0x1;

    if (light.m_flags & 0x1) {
        light.m_flags ^= (light.m_flags ^ (2 * (apiLight.m_dir.w != 0.0f))) & 0x2;
        light.m_dir.x = apiLight.m_dir.x;
        light.m_dir.y = apiLight.m_dir.y;
        light.m_dir.z = apiLight.m_dir.z;
        light.m_ambientColor = apiLight.m_ambColor;
        light.m_dirColor = apiLight.m_dirColor;
        light.m_specularColor = apiLight.m_specColor;
        light.m_constantAttenuation = apiLight.m_constantAttenuation;
        light.m_linearAttenuation = apiLight.m_linearAttenuation;
        light.m_quadraticAttenuation = apiLight.m_quadraticAttenuation;
    }
}

// VERIFY: ef4fa9c6 auto
// OFFSET: 0x00683210
uint32_t CGxDevice::ITexBitsSize(CGxTex* texId, uint32_t width, uint32_t height) {
    if (width == 0xFFFFFFFF) {
        width = texId->m_width;

        if (texId->m_flags.m_filter > GxTex_Linear) {
            width >>= this->m_baseMipLevel;
        }
    }

    if (height == 0xFFFFFFFF) {
        height = texId->m_height;

        if (texId->m_flags.m_filter > GxTex_Linear) {
            height >>= this->m_baseMipLevel;
        }
    }

    uint32_t blockSize = s_texFormatBlockSize[texId->m_format];
    uint32_t size = s_texFormatBytesPerBlock[texId->m_format] * (blockSize + width - 1) / blockSize * ((blockSize + height - 1) / blockSize);

    if (texId->m_flags.m_filter > GxTex_Linear) {
        while (width != 1 || height != 1) {
            if (width > 1) {
                width >>= 1;
            }

            if (height > 1) {
                height >>= 1;
            }

            blockSize = s_texFormatBlockSize[texId->m_format];
            size += s_texFormatBytesPerBlock[texId->m_format] * (blockSize + width - 1) / blockSize * ((blockSize + height - 1) / blockSize);
        }
    }

    if (texId->m_target == GxTex_CubeMap) {
        return GxTexCubeFaces_Last * size;
    }

    return size;
}

// VERIFY: 4e7d27bc auto
// OFFSET: 0x00683F60
int32_t CGxDevice::FormatExists(TSGrowableArray<CGxFormat>& formats, const CGxFormat& format) {
    for (uint32_t i = 0; i < formats.Count(); i++) {
        CGxFormat& entry = formats[i];

        if (entry.m_colorFormat == format.m_colorFormat && entry.m_depthFormat == format.m_depthFormat && entry.m_multisampleCount == format.m_multisampleCount) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 7045de90 manual
// OFFSET: 0x00685040
void CGxDevice::GetSurveyModeInfo(uint32_t& desktopWidth, uint32_t& desktopHeight, uint32_t& desktopBpp, uint32_t& width, uint32_t& height, uint32_t& bpp) {
    CGxMonitorMode mode;
    mode.m_size.x = 0;
    mode.m_size.y = 0;

    if (CGxDevice::AdapterDesktopMode(mode)) {
        desktopWidth = mode.m_size.x;
        desktopHeight = mode.m_size.y;
        desktopBpp = mode.m_bpp;
    }

    width = this->m_format.m_size.x;
    height = this->m_format.m_size.y;

    if (this->m_format.m_window) {
        bpp = desktopBpp;
    } else if (this->m_format.m_colorFormat >= 0 && this->m_format.m_colorFormat < CGxFormat::Formats_Last && s_formatToInt[this->m_format.m_colorFormat]) {
        bpp = SStrToUnsigned(s_formatToInt[this->m_format.m_colorFormat]);
    }
}

// VERIFY: f49a3c9d manual
// OFFSET: 0x006865B0
void CGxDevice::RsInit() {
    this->RsSet(GxRs_PolygonOffset, 0.0f);
    this->RsSet(GxRs_MatDiffuse, -1);
    this->RsSet(GxRs_MatEmissive, 0.0f);
    this->RsSet(GxRs_MatSpecular, 0.0f);
    this->RsSet(GxRs_NormalizeNormals, 0);
    this->RsSet(GxRs_BlendingMode, 0);
    this->RsSet(GxRs_FogStart, 0.0f);
    this->RsSet(GxRs_FogEnd, 1.0f);
    this->RsSet(GxRs_FogColor, static_cast<int32_t>(0xFFFF0000));
    this->RsSet(GxRs_Lighting, 1);
    this->RsSet(GxRs_Fog, 1);
    this->RsSet(GxRs_DepthTest, 1);
    this->RsSet(GxRs_DepthFunc, GxCmp_LEqual);
    this->RsSet(GxRs_Culling, 1);
    this->RsSet(GxRs_ClipPlaneMask, 0);
    this->RsSet(GxRs_ScissorTest, 0);

    uint32_t tmu;

    for (tmu = 0; tmu < 8; tmu++) {
        this->RsSet(static_cast<EGxRenderState>(GxRs_Texture0 + tmu), nullptr);
        this->RsSet(static_cast<EGxRenderState>(GxRs_ColorOp0 + tmu), GxTexOp_Mod);
        this->RsSet(static_cast<EGxRenderState>(GxRs_AlphaOp0 + tmu), GxTexOp_Mod);
        this->RsSet(static_cast<EGxRenderState>(GxRs_TexGen0 + tmu), 0);
        this->RsSet(static_cast<EGxRenderState>(GxRs_TextureShader0 + tmu), 0);
        this->RsSet(static_cast<EGxRenderState>(GxRs_TextureCoord0 + tmu), static_cast<int32_t>(tmu));
    }

    for (; tmu < 16; tmu++) {
        this->RsSet(static_cast<EGxRenderState>(GxRs_Texture0 + tmu), nullptr);
    }

    this->RsSet(GxRs_PixelShader, nullptr);
    this->RsSet(GxRs_VertexShader, nullptr);
    this->RsSet(GxRs_PointScale, 1.0f);
    this->RsSet(GxRs_PointScaleAttenuation, C3Vector(1.0f, 0.0f, 0.0f));
    this->RsSet(GxRs_PointScaleMin, 0.0f);
    this->RsSet(GxRs_PointScaleMax, 1.0f);
    this->RsSet(GxRs_PointSprite, 0);
    this->RsSet(GxRs_BlendFactor, 0.0f);
    this->RsSet(GxRs_ColorMaterial, 0);
}

// INLINED
CGxMonitorMode::CGxMonitorMode() {
    this->m_size.x = 0;
    this->m_size.y = 0;
}
