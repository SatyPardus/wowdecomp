#include "shadow.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CiRect.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/Game/GameClient/ProjectTex.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxVertexPC.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/Tempest/C4Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C33Matrix.h"

// STATIC: 0x00AF3E08
int32_t s_shadowLOD = 1;

// STATIC: 0x00AF3E20
static uint32_t s_shadowTexelCount = 64 * 8;

// STATIC: 0x00AF3E24
static CImVector s_shadowTexels[64 * 8];

// STATIC: 0x00AF3E0C
static int32_t s_queryFrame = -1;

// STATIC: 0x00AF4644
static C3Vector s_projectNormal(0.0f, 0.0f, 1.0f);

// STATIC: 0x00D38010
static int32_t s_queryDoodads;

// STATIC: 0x00D38014
static CM2Model* s_doodadModelBuffer[10];

// STATIC: 0x00D38050
static uint32_t s_queryTriFlags;

// STATIC: 0x00D38054
static uint32_t s_doodadModelCount;

// STATIC: 0x00D38058
static CAaBox s_queryBox;

// STATIC: 0x00D38070
static World::TriData s_queryTriData;

// STATIC: 0x00D3803C
static CTexture* s_shadowModTexture;

// STATIC: 0x00D38040
static CTexture* s_shadowAddTexture;

// STATIC: 0x00D38044
static CTexture* s_shadowBlobTexture;

// VERIFY: 489bdfc7 auto
// OFFSET: 0x007E2C80
void ShadowDestroy() {
    if (s_shadowBlobTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_shadowBlobTexture));
    }

    if (s_shadowModTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_shadowModTexture));
    }

    if (s_shadowAddTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_shadowAddTexture));
    }

    s_shadowBlobTexture = nullptr;
    s_shadowModTexture = nullptr;
    s_shadowAddTexture = nullptr;
}

// VERIFY: 0b8064cf manual
// OFFSET: 0x007E36E0
static void ShadowAddCallback(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    switch (command) {
        case GxTex_Lock: {
            s_shadowTexelCount = width * height;
            break;
        }

        case GxTex_Latch: {
            if (mipLevel != 0) {
                break;
            }

            texelStrideInBytes = width * sizeof(CImVector);
            texels = s_shadowTexels;

            CImVector* row = s_shadowTexels;

            for (uint32_t y = 0; y < height; y++) {
                CImVector color;
                color.b = 0xFF;
                color.g = 0xFF;
                color.r = 0xFF;

                for (uint32_t x = 0; x < width; x++) {
                    float t = static_cast<float>(x) / static_cast<float>(width - 1) * 12.0f;
                    float alpha;

                    if (t < 2.0f) {
                        alpha = t * 0.5f;
                    } else if (t < 10.0f) {
                        alpha = 1.0f;
                    } else {
                        alpha = (12.0f - t) * 0.5f;

                        if (alpha <= 0.0f) {
                            alpha = 0.0f;
                        }
                    }

                    color.a = static_cast<uint8_t>(alpha * 255.0f);
                    row[x] = color;
                }

                row += width;
            }

            break;
        }

        default:
            break;
    }
}

// VERIFY: 9a26fdc2 manual
// OFFSET: 0x007E3820
static void ShadowModCallback(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    switch (command) {
        case GxTex_Lock: {
            s_shadowTexelCount = width * height;
            break;
        }

        case GxTex_Latch: {
            if (mipLevel != 0) {
                break;
            }

            texelStrideInBytes = width * sizeof(CImVector);
            texels = s_shadowTexels;

            CImVector* row = s_shadowTexels;

            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    float t = static_cast<float>(x) / static_cast<float>(width - 1) * 12.0f;
                    float alpha;

                    if (t < 2.0f) {
                        alpha = t * 0.5f;
                    } else if (t < 10.0f) {
                        alpha = 1.0f;
                    } else {
                        alpha = (12.0f - t) * 0.5f;

                        if (alpha <= 0.0f) {
                            alpha = 0.0f;
                        }
                    }

                    uint8_t value = static_cast<uint8_t>(CMath::fint_((1.0f - alpha) * 255.0f));

                    CImVector color;
                    color.b = value;
                    color.g = value;
                    color.r = value;
                    color.a = value == 0xFF ? 0x00 : 0xFF;
                    row[x] = color;
                }

                row += width;
            }

            break;
        }

        default:
            break;
    }
}

// VERIFY: a405a61e auto
// OFFSET: 0x007E3980
void ShadowSetLOD(int32_t lod) {
    s_shadowLOD = lod;

    CiRect rect;
    rect.maxY = 8;
    rect.maxX = 64;

    if (lod == 1) {
        TextureSetUserData(s_shadowAddTexture, ShadowAddCallback, nullptr);
        TextureSetUserData(s_shadowModTexture, ShadowModCallback, nullptr);
        GxTexUpdate(TextureGetGxTex(s_shadowAddTexture, 1, nullptr), rect, 0);
        GxTexUpdate(TextureGetGxTex(s_shadowModTexture, 1, nullptr), rect, 0);
    }
}

// VERIFY: c53c978c manual
// OFFSET: 0x007E4A40
void ShadowInit() {
    CStatus status;

    s_shadowBlobTexture = TextureCreate("Textures\\ShadowBlob.blp", CGxTexFlags(), &status, 1);
    s_shadowAddTexture = TextureCreate(64, 8, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(), nullptr, ShadowAddCallback, "ShadowAdd", 0);
    s_shadowModTexture = TextureCreate(64, 8, GxTex_Argb8888, GxTex_Argb8888, CGxTexFlags(), nullptr, ShadowModCallback, "ShadowMod", 0);
    s_cvShadowLOD = CVar::Register("shadowLOD", "Unit shadow LOD", 1, "1", ShadowLODCallback, CATEGORY_GRAPHICS, false, nullptr, false);
    s_cvExtShadowQuality = CVar::Lookup("extShadowQuality");
}

// VERIFY: d5bef9e8 auto
// OFFSET: 0x007E2C60
CGxTex* ProjectTex2dGetAddFadeTex() {
    return TextureGetGxTex(s_shadowAddTexture, 1, nullptr);
}

// VERIFY: ff93aa7d auto
// OFFSET: 0x007E49E0
void CWorldScene::RequestModelStreaming(CM2Model* model, const CAaBox* box) {
    if (model && model->IsDrawable(0, 0) && !(model->m_fFlags & 0x4000) && !box->IsEmpty() && s_cvExtShadowQuality->m_intValue <= 0 && s_shadowLOD == 1) {
        CWorldScene::RenderStreamingRequestDecal(model, box, 0.4f);
    }
}

// STATIC: 0x00AF3E14
static float s_decalDepthBelow = 1.6666666f;

// STATIC: 0x00AF3E18
static float s_decalDepthAbove = 1.0f;

// VERIFY: e1de2e5e manual
// OFFSET: 0x007E4480
void CWorldScene::RenderStreamingRequestDecal(CM2Model* model, const CAaBox* box, float scale) {
    if (!(box->b.x <= box->t.x || box->b.y <= box->t.y || box->b.z <= box->t.z)) {
        return;
    }

    float halfX = (box->t.x - box->b.x) * 0.5f;
    float halfY = 0.5f * (box->t.y - box->b.y);

    if (fabs(halfX) < 0.00000023841858f || fabs(halfY) < 0.00000023841858f || fabs(box->t.z - box->b.z) < 0.00000023841858f) {
        return;
    }

    CAaBox local;
    local.b.x = -halfX;
    local.b.y = -halfY;
    local.b.z = box->b.z;
    local.t.x = halfX;
    local.t.y = halfY;
    local.t.z = box->t.z;
    C44Matrix transform = model->m_worldTransform;
    float modelScale = sqrt(transform.a0 * transform.a0 + transform.a1 * transform.a1 + transform.a2 * transform.a2);
    local *= modelScale;
    local.b.Minimize(C3Vector(5.0f, 5.0f, 5.0f));
    local.b.Maximize(C3Vector(-5.0f, -5.0f, -5.0f));
    local.t.Minimize(C3Vector(5.0f, 5.0f, 5.0f));
    local.t.Maximize(C3Vector(-5.0f, -5.0f, -5.0f));
    float halfZ = (local.t.z - local.b.z) * 0.5f;
    C33Matrix rotation(transform);

    if (!fequal(modelScale, 1.0f)) {
        float inverse = 1.0f / modelScale;
        rotation.Scale(C3Vector(inverse, inverse, inverse));
    }

    C3Vector corners[4];
    corners[0] = C3Vector(local.t.x, local.t.y, 0.0f) * rotation;
    corners[1] = C3Vector(local.t.x, local.b.y, 0.0f) * rotation;
    corners[2] = C3Vector(local.b.x, local.b.y, 0.0f) * rotation;
    corners[3] = C3Vector(local.b.x, local.t.y, 0.0f) * rotation;
    CAaBox bounds = CAaBox::Bounding(corners, 4);
    bounds.b.x += transform.d0;
    bounds.b.y += transform.d1;
    bounds.t.x += transform.d0;
    bounds.t.y += transform.d1;
    bounds.b.z = transform.d2 - s_decalDepthBelow * halfZ;
    bounds.t.z = transform.d2 + halfZ * s_decalDepthAbove;

    C33Matrix boundsScale(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    boundsScale.Scale(C3Vector(bounds.t.y - bounds.b.y, bounds.t.x - bounds.b.x, 1.0f));
    C33Matrix localScale(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    localScale.Scale(1.0f / fabs(local.t.y - local.b.y), 1.0f / fabs(local.t.x - local.b.x), 1.0f);
    C33Matrix transpose(rotation.a0, rotation.b0, rotation.c0, rotation.a1, rotation.b1, rotation.c1, rotation.a2, rotation.b2, rotation.c2);
    C44Matrix projection(boundsScale * transpose * localScale);

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 4);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(s_shadowBlobTexture, 1, nullptr));
    GxRsSet(GxRs_FogColor, static_cast<int32_t>(0xFFFFFFFF));
    GxRsSet(GxRs_ColorOp0, 5);
    GxRsSet(GxRs_AlphaOp0, 3);
    float alpha = 0.0f;

    if (0.0f == scale) {
        GxRsSet(GxRs_DepthFunc, 1);
        alpha = 0.0f;
    }

    if (model->m_fadeAlpha >= alpha) {
        alpha = model->m_fadeAlpha;

        if (alpha > 1.0f) {
            alpha = 1.0f;
        }
    }

    CImVector color;
    color.b = 0xFF;
    color.g = 0xFF;
    color.r = 0xFF;
    color.a = static_cast<uint8_t>(static_cast<int32_t>(alpha * 255.0f + 0.5f));
    ProjectTex::Render2d(bounds, color, &projection, 0.5f, 0x220122, 0, scale);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: 5c146ae6 auto
// OFFSET: 0x007E2C40
CGxTex* ProjectTex2dGetModFadeTex() {
    return TextureGetGxTex(s_shadowModTexture, 1, nullptr);
}

// VERIFY: 04f96856 manual
// OFFSET: 0x007E2FD0
static void ProjectTexBuildVertexBuffer(World::TriData::Batch* triBatch, CGxBatch* batch, uint32_t flags) {
    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPN), triBatch->m_indexCount);
    CGxVertexPN* vertex = reinterpret_cast<CGxVertexPN*>(g_theGxDevicePtr->BufLock(buf));

    if (triBatch->m_heightList) {
        for (uint16_t i = 0; i < triBatch->m_indexCount; i++) {
            uint16_t index = triBatch->m_indices[i];
            vertex->m_position = triBatch->m_vertexList[index];
            vertex->m_position.z = triBatch->m_heightList[index];
            vertex->m_normal = s_projectNormal;
            vertex++;
        }
    } else {
        batch->m_count = 0;

        if (flags & 0x4) {
            for (uint32_t i = 0; i < triBatch->m_indexCount; i += 3) {
                const C3Vector& v0 = triBatch->m_vertexList[triBatch->m_indices[i]];
                const C3Vector& v1 = triBatch->m_vertexList[triBatch->m_indices[i + 1]];
                const C3Vector& v2 = triBatch->m_vertexList[triBatch->m_indices[i + 2]];
                vertex[0].m_position = v0;
                vertex[0].m_normal = s_projectNormal;
                vertex[1].m_position = v1;
                vertex[1].m_normal = s_projectNormal;
                vertex[2].m_position = v2;
                vertex[2].m_normal = s_projectNormal;
                batch->m_count += 3;
                vertex += 3;
            }
        } else {
            for (uint16_t i = 0; i < triBatch->m_indexCount; i += 3) {
                const C3Vector& v0 = triBatch->m_vertexList[triBatch->m_indices[i]];
                const C3Vector& v1 = triBatch->m_vertexList[triBatch->m_indices[i + 1]];
                const C3Vector& v2 = triBatch->m_vertexList[triBatch->m_indices[i + 2]];

                if ((v2.y - v0.y) * (v1.x - v0.x) - (v1.y - v0.y) * (v2.x - v0.x) >= 0.0f) {
                    vertex[0].m_position = v0;
                    vertex[0].m_normal = s_projectNormal;
                    vertex[1].m_position = v1;
                    vertex[1].m_normal = s_projectNormal;
                    vertex[2].m_position = v2;
                    vertex[2].m_normal = s_projectNormal;
                    vertex += 3;
                    batch->m_count += 3;
                }
            }
        }
    }

    g_theGxDevicePtr->BufUnlock(buf, 0);
    buf->m_hasData = 1;
    GxPrimVertexPtr(buf, GxVBF_PN);
}

// VERIFY: 2dba67bb manual
// OFFSET: 0x007E32F0
static void ProjectTexBuildVertexBufferVariant(World::TriData::Batch* triBatch, CImVector color, CGxBatch* batch, uint32_t flags) {
    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPC), triBatch->m_indexCount);
    CGxVertexPC* vertex = reinterpret_cast<CGxVertexPC*>(g_theGxDevicePtr->BufLock(buf));

    if (triBatch->m_heightList) {
        for (uint32_t i = 0; i < triBatch->m_indexCount; i++) {
            uint16_t index = triBatch->m_indices[i];
            vertex->p = triBatch->m_vertexList[index];
            vertex->p.z = triBatch->m_heightList[index];
            vertex->c = color;
            vertex++;
        }
    } else {
        batch->m_count = 0;

        if (flags & 0x4) {
            for (uint32_t i = 0; i < triBatch->m_indexCount; i += 3) {
                const C3Vector& v0 = triBatch->m_vertexList[triBatch->m_indices[i]];
                const C3Vector& v1 = triBatch->m_vertexList[triBatch->m_indices[i + 1]];
                const C3Vector& v2 = triBatch->m_vertexList[triBatch->m_indices[i + 2]];
                vertex[0].p = v0;
                vertex[0].c = color;
                vertex[1].p = v1;
                vertex[1].c = color;
                vertex[2].p = v2;
                vertex[2].c = color;
                batch->m_count += 3;
                vertex += 3;
            }
        } else {
            for (uint16_t i = 0; i < triBatch->m_indexCount; i += 3) {
                const C3Vector& v0 = triBatch->m_vertexList[triBatch->m_indices[i]];
                const C3Vector& v1 = triBatch->m_vertexList[triBatch->m_indices[i + 1]];
                const C3Vector& v2 = triBatch->m_vertexList[triBatch->m_indices[i + 2]];

                if ((v2.y - v0.y) * (v1.x - v0.x) - (v1.y - v0.y) * (v2.x - v0.x) >= 0.0f) {
                    vertex[0].p = v0;
                    vertex[0].c = color;
                    vertex[1].p = v1;
                    vertex[1].c = color;
                    vertex[2].p = v2;
                    vertex[2].c = color;
                    vertex += 3;
                    batch->m_count += 3;
                }
            }
        }
    }

    g_theGxDevicePtr->BufUnlock(buf, 0);
    buf->m_hasData = 1;
    GxPrimVertexPtr(buf, GxVBF_PC);
}

// VERIFY: 2df38cbe auto
// OFFSET: 0x007E3580
static void ProjectTexRenderIndices(World::TriData::Batch* triBatch) {
    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), triBatch->m_indexCount);
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(buf));

    for (uint32_t i = 0; i < triBatch->m_indexCount; i++) {
        indices[i] = static_cast<uint16_t>(i);
    }

    g_theGxDevicePtr->BufUnlock(buf, 0);
    buf->m_hasData = 1;
    g_theGxDevicePtr->PrimIndexPtr(buf);
}

// VERIFY: 3c679037 auto
// OFFSET: 0x007E35F0
static int32_t ProjectTexRenderGetQuery2d(const CAaBox& box, uint32_t flags, uint32_t triFlags, int32_t* doodads, int32_t* tris) {
    *doodads = flags & 0x1;
    s_doodadModelCount = 0;
    s_queryTriFlags = 0;
    s_queryDoodads = 0;
    *tris = triFlags != 0;
    s_queryBox = box;
    s_queryFrame = CWorld::frameCnt;

    if (*doodads) {
        s_queryDoodads = 1;
        s_doodadModelCount = World::GetIntersectingDoodadModels(s_doodadModelBuffer, 10, box, 0x2000000);
    }

    if (*tris) {
        s_queryTriFlags = triFlags;
        World::TriData::statusFlags = 0;
        World::TriData::nBatches = 0;
        World::TriData::faceIndexCursor = 0;
        World::TriData::indexCursor = 0;
        World::TriData::matrixPoolCount = 0;
        World::GetTris(box, &s_queryTriData, triFlags);
    }

    return s_doodadModelCount || World::TriData::nBatches;
}

// VERIFY: f32d24b7 manual
// OFFSET: 0x007E3AA0
void ProjectTex2dDrawShaderEffect(const CAaBox& box, const CImVector& color, uint32_t triFlags, uint32_t flags, float polygonOffset) {
    int32_t doodads;
    int32_t tris;

    if (!ProjectTexRenderGetQuery2d(box, flags, triFlags, &doodads, &tris)) {
        return;
    }

    CGxAppRenderState* states = g_theGxDevicePtr->m_appRenderStates.m_data;

    if (states[GxRs_BlendingMode].m_value.m_data.i[0] == GxBlend_Mod) {
        g_theGxDevicePtr->RsSet(GxRs_Texture1, TextureGetGxTex(s_shadowModTexture, 1, nullptr));
    } else if (!states[GxRs_Texture1].m_value.m_data.i[0]) {
        g_theGxDevicePtr->RsSet(GxRs_Texture1, TextureGetGxTex(s_shadowAddTexture, 1, nullptr));
    }

    float savedOffset = 0.0f;

    if (polygonOffset != 0.0f) {
        savedOffset = g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_PolygonOffset].m_value.m_data.f[0];
        GxRsSet(GxRs_PolygonOffset, (polygonOffset + polygonOffset) * 3.0518044e-05f);
    }

    CImVector packed = color;

    if (flags & 0x2) {
        CShaderEffect::SetDiffuse(C4Vector(color));
    } else {
        GxFormatColor(packed);
    }

    C3Vector cameraPos(0.0f, 0.0f, 0.0f);
    CGWorldFrame::GetCameraPosition(&cameraPos);
    C44Matrix view;
    view.a0 = 1.0f;
    view.a1 = 0.0f;
    view.a2 = 0.0f;
    view.a3 = 0.0f;
    view.b0 = 0.0f;
    view.b1 = 1.0f;
    view.b2 = 0.0f;
    view.b3 = 0.0f;
    view.c0 = 0.0f;
    view.c1 = 0.0f;
    view.c2 = 1.0f;
    view.c3 = 0.0f;
    view.d0 = -cameraPos.x;
    view.d1 = -cameraPos.y;
    view.d2 = -cameraPos.z;
    view.d3 = 1.0f;
    g_theGxDevicePtr->XformPush(GxXform_World);

    if (tris) {
        for (uint32_t i = 0; i < World::TriData::nBatches; i++) {
            World::TriData::Batch* triBatch = &World::TriData::batches[i];

            if (3 * triBatch->m_indexCount > 0x10000 || !triBatch->m_matrix || !triBatch->m_vertexList || !triBatch->m_indices) {
                continue;
            }

            GxXformSet(GxXform_World, *triBatch->m_matrix * view);
            CGxBatch batch;
            batch.m_primType = GxPrim_Triangles;
            batch.m_start = 0;
            batch.m_count = triBatch->m_indexCount;
            batch.m_minIndex = 0;
            batch.m_maxIndex = triBatch->m_indexCount - 1;

            if (flags & 0x2) {
                ProjectTexBuildVertexBuffer(triBatch, &batch, flags);
            } else {
                ProjectTexBuildVertexBufferVariant(triBatch, packed, &batch, flags);
            }

            ProjectTexRenderIndices(triBatch);
            CShaderEffect::SetDefaultShaders(0);
            CShaderEffect::UpdateWorldViewMatrix();

            if (batch.m_count) {
                GxDraw(&batch, 1);
            }
        }
    }

    if (doodads) {
        for (uint32_t i = 0; i < s_doodadModelCount; i++) {
            CM2Model* model = s_doodadModelBuffer[i];
            C44Matrix world = model->m_worldTransform;
            GxXformSet(GxXform_World, world * view);
            CShaderEffect::UpdateWorldViewMatrix();
            CShaderEffect::SetDiffuse(C4Vector(color));
            model->RenderModelBatchesForProjectedTexture();
        }
    }

    GxXformPop(GxXform_World);

    if (polygonOffset != 0.0f) {
        GxRsSet(GxRs_PolygonOffset, savedOffset);
    }
}

// VERIFY: f18827e8 manual
// OFFSET: 0x007E3E80
static void ProjectTexRender2d(const CAaBox& box, const CImVector& color, uint32_t triFlags, uint32_t flags, float polygonOffset) {
    int32_t doodads;
    int32_t tris;

    if (!ProjectTexRenderGetQuery2d(box, flags, triFlags, &doodads, &tris)) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    CGxAppRenderState* states = g_theGxDevicePtr->m_appRenderStates.m_data;

    if (states[GxRs_BlendingMode].m_value.m_data.i[0] == GxBlend_Mod) {
        g_theGxDevicePtr->RsSet(GxRs_Texture1, TextureGetGxTex(s_shadowModTexture, 1, nullptr));
        GxRsSet(GxRs_ColorOp1, 2);
        GxRsSet(GxRs_AlphaOp1, 2);
    } else if (!states[GxRs_Texture1].m_value.m_data.i[0]) {
        g_theGxDevicePtr->RsSet(GxRs_Texture1, TextureGetGxTex(s_shadowAddTexture, 1, nullptr));
        GxRsSet(GxRs_ColorOp1, 0);
        GxRsSet(GxRs_AlphaOp1, 0);
    }

    if (polygonOffset != 0.0f) {
        GxRsSet(GxRs_PolygonOffset, (polygonOffset + polygonOffset) * 3.0518044e-05f);
    }

    GxRsSet(GxRs_TexGen0, 2);
    GxRsSet(GxRs_TexGen1, 2);
    GxRsSet(GxRs_TextureShader0, 1);
    GxRsSet(GxRs_TextureShader1, 1);

    CImVector packed = color;

    if (flags & 0x2) {
        GxRsSet(GxRs_MatDiffuse, *reinterpret_cast<const int32_t*>(&color));
    } else {
        GxFormatColor(packed);
    }

    C3Vector cameraPos(0.0f, 0.0f, 0.0f);
    CGWorldFrame::GetCameraPosition(&cameraPos);
    C44Matrix view;
    view.a0 = 1.0f;
    view.a1 = 0.0f;
    view.a2 = 0.0f;
    view.a3 = 0.0f;
    view.b0 = 0.0f;
    view.b1 = 1.0f;
    view.b2 = 0.0f;
    view.b3 = 0.0f;
    view.c0 = 0.0f;
    view.c1 = 0.0f;
    view.c2 = 1.0f;
    view.c3 = 0.0f;
    view.d0 = -cameraPos.x;
    view.d1 = -cameraPos.y;
    view.d2 = -cameraPos.z;
    view.d3 = 1.0f;
    g_theGxDevicePtr->XformPush(GxXform_World);

    if (tris) {
        for (uint32_t i = 0; i < World::TriData::nBatches; i++) {
            World::TriData::Batch* triBatch = &World::TriData::batches[i];

            if (3 * triBatch->m_indexCount > 0x10000 || !triBatch->m_matrix || !triBatch->m_vertexList || !triBatch->m_indices) {
                continue;
            }

            GxXformSet(GxXform_World, *triBatch->m_matrix * view);
            CGxBatch batch;
            batch.m_primType = GxPrim_Triangles;
            batch.m_start = 0;
            batch.m_count = triBatch->m_indexCount;
            batch.m_minIndex = 0;
            batch.m_maxIndex = triBatch->m_indexCount - 1;

            if (flags & 0x2) {
                ProjectTexBuildVertexBuffer(triBatch, &batch, flags);
            } else {
                ProjectTexBuildVertexBufferVariant(triBatch, packed, &batch, flags);
            }

            ProjectTexRenderIndices(triBatch);

            if (batch.m_count) {
                GxDraw(&batch, 1);
            }
        }
    }

    if (doodads) {
        for (uint32_t i = 0; i < s_doodadModelCount; i++) {
            CM2Model* model = s_doodadModelBuffer[i];
            C44Matrix world = model->m_worldTransform;
            GxXformSet(GxXform_World, world * view);
            GxRsSet(GxRs_MatDiffuse, *reinterpret_cast<const int32_t*>(&color));
            model->RenderModelBatchesForProjectedTexture();
        }
    }

    GxXformPop(GxXform_World);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: 5f932eb3 manual
// OFFSET: 0x007E4370
void ProjectTex::Render2d(const CAaBox& box, const CImVector& color, const C44Matrix* matrix, float fade, uint32_t a5, uint32_t a6, float polygonOffset) {
    C44Matrix texMatrix;
    C44Matrix fadeMatrix;
    ProjectTex2dMakeMatrices(texMatrix, fadeMatrix, box, matrix, fade, 0);
    g_theGxDevicePtr->XformPushAndSet(GxXform_Tex0, texMatrix);
    g_theGxDevicePtr->XformPushAndSet(GxXform_Tex1, fadeMatrix);
    ProjectTexRender2d(box, color, a5, a6, polygonOffset);
    GxXformPop(GxXform_Tex0);
    GxXformPop(GxXform_Tex1);
}
