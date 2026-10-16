#include <cmath>
#include <cstdlib>
#include <cstring>
#include "Engine/Source/FFX/FFX.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxVertexPCT.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Tempest/CRect.h"

// STATIC: 0x00B24A80
CGxBatch FFX::s_quadBatch = { GxPrim_TriangleStrip, 0, 4, 0, 3 };

// STATIC: 0x00B24A90
CGxBatch FFX::s_gridBatch = { GxPrim_Triangles, 0, 150, 0, 35 };

// STATIC: 0x00D45768
bool FFX::s_useRECT;

// STATIC: 0x00D45778
uint16_t FFX::s_quadIndices[4];

// STATIC: 0x00D45780
FFX::Effect* FFX::s_effect;

// STATIC: 0x00D45784
FFX::Tex FFX::s_screen;

// STATIC: 0x00D457A0
FFX::Tex FFX::s_full[2];

// STATIC: 0x00D457D8
FFX::Tex FFX::s_half[2];

// STATIC: 0x00D45810
FFX::Tex FFX::s_quart[2];

// STATIC: 0x00D45848
C3Vector FFX::s_gridPos[36];

// STATIC: 0x00D459F8
C2Vector FFX::s_gridTex[36];

// STATIC: 0x00D45B18
uint16_t FFX::s_gridIndices[150];

// STATIC: 0x00B24AA0
C44Matrix FFX::s_oldProjMtx;

// STATIC: 0x00B24AE0
C44Matrix FFX::s_oldViewMtx;

// STATIC: 0x00AD88C0
static CGxVertexAttrib s_vertexAttribsPCT[] = {
    { GxVA_Position, 4, 0, 24 },
    { GxVA_Color0, 0, 12, 24 },
    { GxVA_TexCoord0, 3, 16, 24 }
};

// STATIC: 0x00AD8940
static CGxVertexAttrib s_vertexAttribsPCT4[] = {
    { GxVA_Position, 4, 0, 48 },
    { GxVA_Color0, 0, 12, 48 },
    { GxVA_TexCoord0, 3, 16, 48 },
    { GxVA_TexCoord1, 3, 24, 48 },
    { GxVA_TexCoord2, 3, 32, 48 },
    { GxVA_TexCoord3, 3, 40, 48 }
};

// INLINED
static void ResetTex(FFX::Tex& tex) {
    tex.m_texture = nullptr;
    tex.m_texSize.x = 0;
    tex.m_texSize.y = 0;
    tex.m_size.x = 0;
    tex.m_size.y = 0;
    tex.m_texelSize.x = 0.0f;
    tex.m_texelSize.y = 0.0f;
}

// INLINED
static void DestroyTex(FFX::Tex& tex) {
    if (tex.m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(tex.m_texture));
    }

    ResetTex(tex);
}

// VERIFY: a6c53444 auto
// OFFSET: 0x008C0290
C44Matrix* FFX::GetOldViewMtx() {
    return &FFX::s_oldViewMtx;
}

// VERIFY: 2b290dfa manual
// OFFSET: 0x008C02E0
void FFX::SetEffect(Effect* effect) {
    Effect* previous = FFX::s_effect;
    FFX::s_effect = effect;

    if (previous) {
        previous->End();
    }
}

// VERIFY: 800fd8da manual
// OFFSET: 0x008C0300
FFX::Pass::Pass(Tex** inputs, uint32_t count, Tex* output, EGxBlend blend, bool unk) {
    memset(this->m_inputs, 0, sizeof(this->m_inputs));
    this->m_output = nullptr;

    for (uint32_t i = 0; i < count; i++) {
        this->m_inputs[i] = inputs[i];
    }

    this->m_output = output;
    this->m_blend = blend;
    this->m_flip = unk;
}

// VERIFY: 2d6c87ac manual
// OFFSET: 0x008C0360
void FFX::Destroy() {
    DestroyTex(FFX::s_full[0]);
    DestroyTex(FFX::s_full[1]);
    DestroyTex(FFX::s_half[0]);
    DestroyTex(FFX::s_half[1]);
    DestroyTex(FFX::s_quart[0]);
    DestroyTex(FFX::s_quart[1]);
    DestroyTex(FFX::s_screen);
}

// VERIFY: a8690e52 manual
// OFFSET: 0x008C0590
void FFX::SetupGeo_Quad(const C2iVector& dstSize, const C2iVector& srcSize, const C2iVector& srcTexSize, C3Vector* pos, C2Vector* tex, bool flip) {
    // STATIC: 0x00B24A74
    static float s_flipOffset = 1.0f;

    float minX = 0.0f;
    float minY = 0.0f;
    float height = static_cast<float>(dstSize.y);
    float width = static_cast<float>(dstSize.x);
    float maxX;
    float maxY;

    if (g_theGxDevicePtr->DeviceApi() == GxApi_OpenGl) {
        minY = -0.375f;
        minX = 0.375f;
        maxY = height - 0.375f;
        maxX = width + 0.375f;

        if (flip) {
            minY = maxY + s_flipOffset;
            maxY = s_flipOffset - 0.375f;
        }
    } else if (g_theGxDevicePtr->DeviceApi() == GxApi_GLL) {
        minY = -0.375f;
        minX = 0.375f;
        maxY = height - 0.375f;
        maxX = width + 0.375f;
    } else {
        maxY = height;
        maxX = width;
    }

    pos[0].x = minX;
    pos[0].y = minY;
    pos[0].z = 0.0f;
    pos[1].x = minX;
    pos[1].y = maxY;
    pos[1].z = 0.0f;
    pos[2].x = maxX;
    pos[2].y = maxY;
    pos[2].z = 0.0f;
    pos[3].x = maxX;
    pos[3].y = minY;
    pos[3].z = 0.0f;

    float invWidth = 1.0f / srcTexSize.x;
    float invHeight = 1.0f / srcTexSize.y;
    float maxU = (srcSize.x + 0.5f) * invWidth;
    float maxV = (srcSize.y + 0.5f) * invHeight;
    float minU = invWidth * 0.5f;
    float minV = 0.5f * invHeight;

    tex[0].x = minU;
    tex[0].y = maxV;
    tex[1].x = minU;
    tex[1].y = minV;
    tex[2].x = maxU;
    tex[2].y = minV;
    tex[3].x = maxU;
    tex[3].y = maxV;
}

// VERIFY: 98721882 manual
// OFFSET: 0x008C0740
void FFX::SetupGeo_Grid(const C2iVector& dstSize, const C2iVector& srcSize, const C2iVector& srcTexSize, C3Vector* pos, C2Vector* tex, int32_t count, bool flip) {
    // STATIC: 0x00B24A78
    static float s_flipOffset = 1.0f;

    float minX = 0.0f;
    float height = static_cast<float>(dstSize.y);
    float width = static_cast<float>(dstSize.x);
    float invWidth = 1.0f / fabsf(width);
    float invHeight = 1.0f / fabsf(height);
    float scaleX = invWidth * 2.0f;
    float scaleY = 2.0f * invHeight;
    float maxX = width;
    float minY;
    float maxY;

    if (g_theGxDevicePtr->DeviceApi() == GxApi_OpenGl) {
        minY = -0.5f;
        minX = 0.5f;
        maxY = height - 0.5f;
        maxX = width + 0.5f;

        if (flip) {
            minY = maxY + s_flipOffset;
            maxY = s_flipOffset - 0.5f;
        }
    } else if (g_theGxDevicePtr->DeviceApi() == GxApi_GLL) {
        minY = -0.375f;
        minX = 0.375f;
        maxY = height - 0.375f;
        maxX = width + 0.375f;
    } else {
        minY = 0.0f;
        maxY = height;
    }

    float invTexWidth = 1.0f / srcTexSize.x;
    float invTexHeight = 1.0f / srcTexSize.y;
    float minU = invTexWidth * 0.5f;
    float maxU = (srcSize.x + 0.5f) * invTexWidth;
    float maxV = invTexHeight * (srcSize.y + 0.5f);
    float minV = 0.5f * invTexHeight;
    float step = 1.0f / (count - 1);
    float stepX = (maxX - minX) * step;
    float stepY = (maxY - minY) * step;
    float stepU = (maxU - minU) * step;
    float stepV = step * (minV - maxV);

    int32_t index = 0;
    float y = minY;
    float v = maxV;

    for (int32_t row = 0; row < count; row++) {
        float x = minX;
        float u = minU;

        for (int32_t col = 0; col < count; col++) {
            pos[index].x = x * scaleX - 1.0f;
            pos[index].y = 1.0f - y * scaleY;
            pos[index].z = 0.0f;

            if (FFX::s_useRECT) {
                tex[index].x = invWidth * (srcTexSize.x * x);
                tex[index].y = (1.0f - y * invHeight) * srcTexSize.y;
            } else {
                tex[index].x = u;
                tex[index].y = v;
            }

            x += stepX;
            u += stepU;
            index++;
        }

        y += stepY;
        v += stepV;
    }
}

// VERIFY: 1f27610b manual
// OFFSET: 0x008C0C90
void FFX::SetupVB_PCT4(const C3Vector* pos, CImVector color, const C2Vector* tex, const C2Vector* offsets, const C2Vector& scale) {
    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(VertexPCT4), 4);
    VertexPCT4* vertex = reinterpret_cast<VertexPCT4*>(GxBufLock(buf));

    for (uint32_t i = 0; i < 4; i++) {
        vertex->p = pos[i];
        vertex->c = color;

        for (uint32_t j = 0; j < 4; j++) {
            vertex->tc[j].x = offsets[j].x * scale.x + tex[i].x;
            vertex->tc[j].y = offsets[j].y * scale.y + tex[i].y;
        }

        vertex++;
    }

    GxBufUnlock(buf, 0);
    g_theGxDevicePtr->PrimVertexFormat(buf, s_vertexAttribsPCT4, 6);
}

// VERIFY: 77a070ff manual
// OFFSET: 0x008C0DE0
void FFX::SetupVB_PCT(uint32_t count, const C3Vector* pos, const C2Vector* tex, const float* shade) {
    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPCT), count);
    CGxVertexPCT* vertex = reinterpret_cast<CGxVertexPCT*>(GxBufLock(buf));

    for (uint32_t i = 0; i < count; i++) {
        vertex->p = pos[i];

        uint8_t value = static_cast<uint8_t>((shade[i] + 1.0f) * 127.5f);
        vertex->c.a = value;
        vertex->c.b = value;
        vertex->c.g = value;
        vertex->c.r = value;
        vertex->tc = tex[i];

        vertex++;
    }

    GxBufUnlock(buf, 0);
    g_theGxDevicePtr->PrimVertexFormat(buf, s_vertexAttribsPCT, 3);
}

// VERIFY: 40a64451 auto
// OFFSET: 0x008C0EC0
void FFX::SetupIB() {
    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), 4);
    GxBufData(buf, FFX::s_quadIndices, sizeof(FFX::s_quadIndices), 0);
    g_theGxDevicePtr->PrimIndexPtr(buf);
}

// VERIFY: d2162e12 manual
// OFFSET: 0x008C0F00
void FFX::SetupIB(int32_t cols, int32_t rows) {
    int32_t count = (cols - 1) * (rows - 1) * 6;
    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), count);

    int32_t index = 0;
    int32_t rowStart = 0;

    for (int32_t row = 0; row < rows - 1; row++) {
        int32_t nextRow = rowStart + cols;

        for (int32_t col = 0; col < cols - 1; col++) {
            uint16_t topLeft = rowStart + col;
            uint16_t bottomRight = nextRow + col + 1;

            FFX::s_gridIndices[index++] = topLeft;
            FFX::s_gridIndices[index++] = bottomRight;
            FFX::s_gridIndices[index++] = nextRow + col;
            FFX::s_gridIndices[index++] = topLeft;
            FFX::s_gridIndices[index++] = rowStart + col + 1;
            FFX::s_gridIndices[index++] = bottomRight;
        }

        rowStart += cols;
    }

    GxBufData(buf, FFX::s_gridIndices, count * sizeof(uint16_t), 0);
    g_theGxDevicePtr->PrimIndexPtr(buf);
}

// VERIFY: c85eddce auto
// OFFSET: 0x008C1010
void FFX::Finish() {
    if (!FFX::s_cvarFFX->m_intValue || !FFX::s_effect || !FFX::s_effect->Valid()) {
        return;
    }

    C2iVector sourcePos = { 0, 0 };
    C2iVector destPos = { 0, 0 };
    GxTexCopy(TextureGetGxTex(FFX::s_full[0].m_texture, 1, nullptr), destPos, sourcePos, FFX::s_full[0].m_size, 0, 0);

    FFX::s_effect->Execute();

    GxXformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
}

// VERIFY: 4aebbae1 auto
// OFFSET: 0x008C10B0
EGxTexTarget FFX::GetTexTarget() {
    if (!FFX::s_cvarFFXRectangle->m_intValue || g_theGxDevicePtr->Caps().m_texNonPow2) {
        return GxTex_2d;
    }

    if (g_theGxDevicePtr->Caps().m_texTarget[GxTex_NonPow2]) {
        return GxTex_NonPow2;
    }

    return g_theGxDevicePtr->Caps().m_texTarget[GxTex_Rectangle] ? GxTex_Rectangle : GxTex_2d;
}

// VERIFY: c4cf9f53 manual
// OFFSET: 0x008C1100
void FFX::CreateTex(Tex& tex, const C2iVector& size, bool bit8, bool renderTarget) {
    EGxTexFormat format;

    if (g_theGxDevicePtr->Caps().m_rttFormat[GxTex_Argb8888]) {
        format = GxTex_Argb8888;
    } else if (g_theGxDevicePtr->Caps().m_rttFormat[GxTex_Abgr8888]) {
        format = GxTex_Abgr8888;
    } else {
        format = g_theGxDevicePtr->Caps().m_rttFormat[GxTex_Rgb565] ? GxTex_Rgb565 : GxTex_Unknown;
    }

    CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, renderTarget, 1, bit8, 0, 0);

    EGxTexTarget target = FFX::GetTexTarget();
    int32_t width = size.x;
    int32_t height = size.y;

    if (target == GxTex_2d) {
        for (height = 1; size.y > height; height *= 2) {
        }

        for (width = 1; size.x > width; width *= 2) {
        }
    }

    int32_t maxSize = static_cast<int32_t>(g_theGxDevicePtr->Caps().m_texMaxSize[target]);

    if (height >= maxSize) {
        height = maxSize;
    }

    if (width >= maxSize) {
        width = maxSize;
    }

    height = height > 8 ? height : 8;
    width = width > 8 ? width : 8;

    if (width != tex.m_texSize.x || height != tex.m_texSize.y) {
        if (tex.m_texture) {
            HandleClose(reinterpret_cast<HOBJECT>(tex.m_texture));
            tex.m_texture = nullptr;
        }

        tex.m_texture = TextureCreate(target, width, height, 0, format, format, flags, nullptr, GxTexNullUserFunc, "FFX::CreateTex", 0);
    }

    tex.m_size.x = size.x;
    tex.m_texSize.x = width;
    tex.m_texSize.y = height;
    tex.m_size.y = size.y;
    tex.m_texelSize.x = 1.0f / tex.m_texSize.x;
    tex.m_texelSize.y = 1.0f / tex.m_texSize.y;
}

// VERIFY: 3b1c5088 auto
// OFFSET: 0x008C1270
bool FFX::Effect::Valid() {
    if (!this->m_passes.Count()) {
        return false;
    }

    bool valid = true;

    for (uint32_t i = 0; i < this->m_passes.Count(); i++) {
        valid = valid && this->m_passes[i]->Valid();
    }

    if (this->m_cvar) {
        return valid && this->m_cvar->m_intValue;
    }

    return valid;
}

// VERIFY: 5cde7b6f auto
// OFFSET: 0x008C12C0
void FFX::Effect::Execute() {
    for (uint32_t i = 0; i < this->m_passes.Count(); i++) {
        this->m_passes[i]->Execute();
    }
}

// VERIFY: 60f3461e manual
// OFFSET: 0x008C12F0
void FFX::Init() {
    FFX::s_cvarFFX = CVar::Register("ffx", "full screen effects", 1, "1", FFX::CVarCallback, 1, false, nullptr, false);
    FFX::s_cvarFFXRectangle = CVar::Register("ffxRectangle", "use rectangle texture for full screen effects", 1, "1", FFX::CVarCallback, 1, false, nullptr, false);
    FFX::s_cvarMultisample = CVar::Lookup("gxMultisample");

    ResetTex(FFX::s_full[0]);
    ResetTex(FFX::s_full[1]);
    ResetTex(FFX::s_half[0]);
    ResetTex(FFX::s_half[1]);

    FFX::s_quadIndices[0] = 0;
    FFX::s_quadIndices[1] = 3;
    FFX::s_quadIndices[2] = 1;
    FFX::s_quadIndices[3] = 2;

    ResetTex(FFX::s_quart[0]);
    ResetTex(FFX::s_quart[1]);
    ResetTex(FFX::s_screen);

    FFX::s_effect = nullptr;
    FFX::s_useRECT = FFX::GetTexTarget() == GxTex_Rectangle;
}

// VERIFY: d90aa2b3 manual
// OFFSET: 0x008C1520
void FFX::SetupReset() {
    g_theGxDevicePtr->RsPop();
    GxXformSetView(FFX::s_oldViewMtx);
    GxXformSetProjection(FFX::s_oldProjMtx);
    GxXformPop(GxXform_World);
    GxRenderTargetSet(GxBuffers_Color, nullptr, 0);
    GxXformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
}

// VERIFY: 791f22ac auto
// OFFSET: 0x008C15F0
void FFX::AllocTex() {
    CRect rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_theGxDevicePtr->CapsWindowSize(rect);

    int32_t width = static_cast<int32_t>(rect.maxX - rect.minX);
    int32_t height = static_cast<int32_t>(rect.maxY - rect.minY);

    FFX::s_screen.m_size.x = width;
    FFX::s_screen.m_size.y = height;
    FFX::s_screen.m_texSize.x = width;
    FFX::s_screen.m_texSize.y = height;
    FFX::s_screen.m_texelSize.x = 1.0f / width;
    FFX::s_screen.m_texelSize.y = 1.0f / height;

    EGxTexTarget target = FFX::GetTexTarget();
    int32_t maxSize = static_cast<int32_t>(g_theGxDevicePtr->Caps().m_texMaxSize[target]);

    if (height >= maxSize) {
        height = maxSize;
    }

    if (width >= maxSize) {
        width = maxSize;
    }

    height = height > 8 ? height : 8;
    width = width > 8 ? width : 8;

    C2iVector size = { width, height };
    FFX::CreateTex(FFX::s_full[0], size, true, true);
    FFX::CreateTex(FFX::s_full[1], size, false, true);

    size.x = width / 2;
    size.y = height / 2;
    FFX::CreateTex(FFX::s_half[0], size, false, true);
    FFX::CreateTex(FFX::s_half[1], size, false, true);

    size.x = width / 4;
    size.y = height / 4;
    FFX::CreateTex(FFX::s_quart[0], size, false, true);
    FFX::CreateTex(FFX::s_quart[1], size, false, true);
}

// VERIFY: 7a420d16 auto
// OFFSET: 0x008C1770
void FFX::Start() {
    if (!FFX::s_cvarFFX->m_intValue || !FFX::s_effect || !FFX::s_effect->Valid()) {
        return;
    }

    CRect rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_theGxDevicePtr->CapsWindowSize(rect);

    int32_t width = static_cast<int32_t>(rect.maxX - rect.minX);
    int32_t height = static_cast<int32_t>(rect.maxY - rect.minY);

    if (width != FFX::s_screen.m_size.x || height != FFX::s_screen.m_size.y) {
        FFX::AllocTex();
    }

    float scaleX = 1.0f;
    float scaleY = 1.0f;

    if (rect.maxX - rect.minX > FFX::s_full[0].m_size.x) {
        scaleX = FFX::s_full[0].m_size.x / (rect.maxX - rect.minX);
    }

    if (rect.maxY - rect.minY > FFX::s_full[0].m_size.y) {
        scaleY = FFX::s_full[0].m_size.y / (rect.maxY - rect.minY);
    }

    GxXformSetViewport(0.0f, scaleX, 0.0f, scaleY, 0.0f, 1.0f);
}

// VERIFY: 408a281f auto
// OFFSET: 0x008C1870
FFX::Effect::Effect() {
    this->m_cvar = nullptr;
    this->m_defaultEnabled = true;
}

// VERIFY: fe602adb manual
// OFFSET: 0x008C1890
void FFX::SetupGx(Tex* target) {
    GxXformProjection(FFX::s_oldProjMtx);

    C44Matrix projection;
    GxuXformCreateOrtho(0.0f, static_cast<float>(target->m_size.x), 0.0f, static_cast<float>(target->m_size.y), -1.0f, 1.0f, projection);
    GxXformSetProjection(projection);

    GxXformView(FFX::s_oldViewMtx);

    C44Matrix view;
    GxXformSetView(view);

    GxXformPush(GxXform_World);
    g_theGxDevicePtr->m_xforms[GxXform_World].Identity();

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_DepthTest, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Fog, 0);

    CGxTex* gxTex = target->m_texture ? TextureGetGxTex(target->m_texture, 1, nullptr) : nullptr;
    GxRenderTargetSet(GxBuffers_Color, gxTex, 0);

    GxXformSetViewport(0.0f, static_cast<float>(target->m_size.x) / target->m_texSize.x, 0.0f, static_cast<float>(target->m_size.y) / target->m_texSize.y, 0.0f, 1.0f);
}

// VERIFY: f9c900f3 manual
// OFFSET: 0x007E8080
FFX::Pass::~Pass() {
}

// VERIFY: 51a6fc4f manual
// OFFSET: 0x007EA220
FFX::Effect::~Effect() {
}

// VERIFY: ef2a8af8 manual
// OFFSET: 0x005EEB70
void FFX::Effect::End() {
}

// VERIFY: 787a0f92 auto
// OFFSET: 0x00653A10
void FFX::Effect::SetParameters(int32_t count, const int32_t* params) {
}
