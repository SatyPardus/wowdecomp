#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include <cmath>
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/ShaderEffect/Types.h"

// VERIFY: 410884f1 auto
// OFFSET: 0x009ABB00
void DayNight::DNStars::Initialize() {
    this->m_scene = M2CreateScene();
    this->m_model = this->m_scene->CreateModel("Environments\\Stars\\stars.mdl", 0);
    this->m_lastTimeMs = OsGetAsyncTimeMs();
}

// VERIFY: e9833dc1 manual
// OFFSET: 0x009ABB60
void DayNight::Billboard(C44Matrix& mat, const C3Vector& dir) {
    mat.a0 = dir.x;
    mat.a1 = dir.y;
    mat.a2 = dir.z;

    float inv = 1.0f / sqrtf(mat.a2 * mat.a2 + mat.a1 * mat.a1 + mat.a0 * mat.a0);
    mat.a0 = mat.a0 * inv;
    mat.a1 = mat.a1 * inv;
    mat.a2 = inv * mat.a2;

    mat.b0 = -mat.a1;
    mat.b1 = mat.a0;
    mat.b2 = 0.0f;

    if (fabsf(mat.a0 * mat.b0) <= 0.0000099999997f) {
        mat.b0 = 0.0f;
        mat.b1 = 1.0f;
        mat.b2 = 0.0f;
    } else {
        float invB = 1.0f / sqrtf(mat.b0 * mat.b0 + mat.b1 * mat.b1);
        mat.b0 = mat.b0 * invB;
        mat.b1 = invB * mat.b1;
    }

    float c0 = mat.b2 * mat.a1 - mat.a2 * mat.b1;
    float c1 = mat.a2 * mat.b0 - mat.b2 * mat.a0;
    mat.c0 = c0;
    mat.c1 = c1;
    mat.c2 = mat.a0 * mat.b1 - mat.b0 * mat.a1;
}

// VERIFY: dedd196d manual
// OFFSET: 0x009ABD50
void DayNight::DNStars::Render() {
    if (this->m_color.a < 2) {
        return;
    }

    this->m_model->SetAnimating(1);

    CM2Model* model = this->m_model;
    CM2Model* parent = model->m_attachParent;

    if (parent) {
        model->m_fFlags |= 0x80;
    } else {
        model->m_fFlags |= 0x8;
    }

    if (parent) {
        model->m_fFlags |= 0x20000;
    } else {
        model->m_fFlags |= 0x10000;
    }

    this->m_model->m_fadeAlpha = this->m_color.a * 0.0039215689f;

    uint32_t elapsed = OsGetAsyncTimeMs() - this->m_lastTimeMs;
    this->m_scene->AdvanceTime(elapsed);
    this->m_lastTimeMs += elapsed;
    this->m_scene->Animate(C3Vector(0.0f, 0.0f, 0.0f));
    this->m_scene->Draw(M2PASS_0);
    this->m_scene->Draw(M2PASS_1);
}

// VERIFY: 5fee0e81 manual
// OFFSET: 0x009AC660
void DayNight::DNPlanet::Render() {
    C3Vector verts[6];
    C2Vector texCoords[6];
    CImVector colors[6] = {};
    uint16_t indices[6];
    uint32_t vertCount;
    uint32_t indexCount;

    this->GenGeometry(verts, texCoords, colors, indices, &vertCount, &indexCount);
    this->ClipGeometry(verts, texCoords, colors, indices, &vertCount, &indexCount);

    if (!vertCount) {
        return;
    }

    CGxTex* gxTex = TextureGetGxTex(this->m_texture, 0, nullptr);

    if (!gxTex) {
        return;
    }

    g_theGxDevicePtr->RsPush();

    C44Matrix view;
    CGxMatrixStack& viewStack = g_theGxDevicePtr->m_xforms[GxXform_View];
    view = viewStack.m_mtx[viewStack.m_level];

    DNInfo* info = DayNight::GetInfo();
    C3Vector forward(view.a2, view.b2, view.c2);

    C44Matrix billboard;
    DayNight::Billboard(billboard, forward);
    billboard.d0 = this->m_position.x - info->m_cameraPos.x;
    billboard.d1 = this->m_position.y - info->m_cameraPos.y;
    billboard.d2 = this->m_position.z - info->m_cameraPos.z;

    g_theGxDevicePtr->XformPush(GxXform_World);
    g_theGxDevicePtr->m_xforms[GxXform_World].Identity();
    g_theGxDevicePtr->XformSetView(billboard * view);

    GxRsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
    GxRsSet(GxRs_ColorOp0, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp0, GxTexOp_Mod);

    GxPrimVertexPtr(vertCount, verts, sizeof(C3Vector), nullptr, 0, colors, sizeof(CImVector), texCoords, sizeof(C2Vector), nullptr, 0);
    GxPrimIndexPtr(indexCount, indices);

    CGxBatch batch;
    batch.m_primType = GxPrim_TriangleStrip;
    batch.m_start = 0;
    batch.m_count = indexCount;
    batch.m_minIndex = 0;
    batch.m_maxIndex = vertCount - 1;
    g_theGxDevicePtr->Draw(&batch, 1);

    GxXformPop(GxXform_World);
    g_theGxDevicePtr->XformSetView(view);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: 0fb20dcd manual
// OFFSET: 0x009ACB00
void DayNight::DNSky::Render() {
    g_theGxDevicePtr->XformPush(GxXform_World);
    g_theGxDevicePtr->m_xforms[GxXform_World].Identity();

    C3Vector scale(6.6666665f, 6.6666665f, 6.6666665f);
    CGxMatrixStack& world = g_theGxDevicePtr->m_xforms[GxXform_World];
    world.m_dirty = 1;
    world.m_flags[world.m_level] &= ~CGxMatrixStack::F_Identity;
    world.m_mtx[world.m_level].Scale(scale);

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_BlendingMode, 3);
    GxUpdateAlphaRef(GxRs_AlphaRef);

    GxPrimVertexPtr(this->m_nVerts, this->m_geoVerts.m_data, sizeof(C3Vector), nullptr, 0, this->m_clrVerts.m_data, sizeof(CImVector), nullptr, 0, nullptr, 0);
    GxPrimIndexPtr(this->m_nIndices, this->m_indices.m_data);

    CGxBatch batch;
    batch.m_primType = GxPrim_TriangleStrip;
    batch.m_start = 0;
    batch.m_count = this->m_nIndices;
    batch.m_minIndex = 0;
    batch.m_maxIndex = this->m_nVerts - 1;
    g_theGxDevicePtr->Draw(&batch, 1);

    GxXformPop(GxXform_World);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: 62766f13 manual
// OFFSET: 0x009ACD40
void DayNight::DNClouds::Render() {
    if (!this->m_enabled) {
        return;
    }

    CGxTex* gxTex = TextureGetGxTex(this->m_textures[this->m_texIndex], 0, nullptr);

    if (!gxTex) {
        return;
    }

    g_theGxDevicePtr->XformPush(GxXform_World);
    g_theGxDevicePtr->m_xforms[GxXform_World].Identity();
    GxXformScale(GxXform_World, C3Vector(6.6666665f, 6.6666665f, 6.6666665f));

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Culling, 0);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);

    if (DayNight::s_lightFlags & 1) {
        CImVector glow;
        glow.b = 0xFF;
        glow.g = 0xFF;
        glow.r = 0xFF;
        glow.a = static_cast<uint8_t>(CMath::fint_(DayNight::s_glowBlend * 255.0f));
        GxPrimVertexPtr(this->m_nVerts, this->m_verts.m_data, sizeof(C3Vector), nullptr, 0, &glow, 0, this->m_texCoords.m_data, sizeof(C2Vector), nullptr, 0);
    } else {
        GxPrimVertexPtr(this->m_nVerts, this->m_verts.m_data, sizeof(C3Vector), nullptr, 0, this->m_colors.m_data, sizeof(CImVector), this->m_texCoords.m_data, sizeof(C2Vector), nullptr, 0);
    }

    GxPrimIndexPtr(this->m_nIndices, this->m_indices.m_data);

    CGxBatch batch;
    batch.m_primType = GxPrim_TriangleStrip;
    batch.m_start = 0;
    batch.m_count = this->m_nIndices;
    batch.m_minIndex = 0;
    batch.m_maxIndex = this->m_nVerts - 1;
    g_theGxDevicePtr->Draw(&batch, 1);

    g_theGxDevicePtr->RsPop();
    GxXformPop(GxXform_World);
}

// VERIFY: 31012b7c manual
// OFFSET: 0x009AD000
void DayNight::DNGlare::Initialize(const char* fileName) {
    CStatus status;
    this->m_texture = TextureCreate(fileName, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 1);
    SysMsgAdd(&status);

    if (g_theGxDevicePtr->Caps().m_occlusionQuery) {
        GxQueryCreate(this->m_occlusionQuery, GxQueryType_Occlusion);
    }

    this->m_occlusionPixels = 0;
}

// VERIFY: 5667c564 manual
// OFFSET: 0x009AD0B0
void DayNight::DNPlanet::Initialize(const char* fileName) {
    CStatus status;
    this->m_texture = TextureCreate(fileName, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 0);
    SysMsgAdd(&status);
}
