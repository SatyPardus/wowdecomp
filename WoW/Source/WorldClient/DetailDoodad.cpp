#include "DetailDoodad.h"
#include <cmath>
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxMatrixStack.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Model2/M2Data.h"
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/ShaderEffect/ShadowCache.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C33Matrix.h"
#include "Engine/Source/Tempest/CImVector.h"
#include <cstring>
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/WorldClient/CDetailDoodadData.h"
#include "WoW/Source/WorldClient/DayNight.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "WoW/Source/WorldClient/MapLight.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"
#include "WoW/Source/WorldClient/Types.h"
#include "WoW/Source/WorldClient/World.h"

// STATIC: 0x00D1C4C4
static uint32_t s_indexBufferSize;

// STATIC: 0x00D1C4C8
static uint32_t s_vertexBufferSize;

// STATIC: 0x00D1C4DC
static uint32_t unk_D1C4DC;

// STATIC: 0x00D1C4E8
static uint32_t unk_D1C4E8;

// STATIC: 0x00D1C504
TSGrowableArray<CGxBuf*> CDetailDoodad::s_freeGxBufs;

// STATIC: 0x00D1C488
CGxShader* CDetailDoodad::s_pixelShader[8];

// STATIC: 0x00D1C4A8
CGxShader* CDetailDoodad::s_vertexShader[6];

// STATIC: 0x00D1C4C0
int32_t CDetailDoodad::s_bPoolsDirty;

// STATIC: 0x00D1C4CC
uint32_t CDetailDoodad::s_idxBufItems;

// STATIC: 0x00D1C4D0
uint32_t CDetailDoodad::s_vtxBufItems;

// STATIC: 0x00D1C4D4
CGxPool* CDetailDoodad::s_gxIndexPool;

// STATIC: 0x00D1C4D8
CGxPool* CDetailDoodad::s_gxVertexPool;

// STATIC: 0x00D1C4E0
CGxTex* CDetailDoodad::s_alphaRampTexture;

// STATIC: 0x00D1C4E4
uint32_t* CDetailDoodad::s_instHeap;

// STATIC: 0x00D1C4F0
int32_t CDetailDoodad::s_bUseShaders;

// STATIC: 0x00D1C4F4
TSGrowableArray<CDetailDoodadData*> CDetailDoodad::s_doodadList;

// STATIC: 0x00D1C518
CDetailDoodad::SShaderConstants CDetailDoodad::s_shaderConstants;

// VERIFY: 1a974b50 auto
// OFFSET: 0x007B1000
static void __stdcall BuildInstanceMatrix(C33Matrix& result, C33Matrix& rotation, const C33Matrix& basis, float angle, const C3Vector& axis) {
    float c = cos(angle);
    float s = sin(angle);
    float zz = axis.z * axis.z;
    float zs = s * axis.z;

    rotation.a0 = c;
    rotation.a1 = zs;
    rotation.b0 = -zs;
    rotation.b1 = c;
    rotation.c2 = zz * (1.0f - c) + c;

    result.a0 = c * basis.a0 + zs * basis.b0;
    result.a1 = rotation.a0 * basis.a1 + rotation.a1 * basis.b1;
    result.a2 = rotation.a0 * basis.a2 + rotation.a1 * basis.b2;
    result.b0 = rotation.b1 * basis.b0 + basis.a0 * rotation.b0;
    result.b1 = rotation.b1 * basis.b1 + basis.a1 * rotation.b0;
    result.b2 = rotation.b0 * basis.a2 + rotation.b1 * basis.b2;
    result.c0 = basis.c0 * rotation.c2;
    result.c1 = basis.c1 * rotation.c2;
    result.c2 = basis.c2 * rotation.c2;
}

// VERIFY: 15a97eda manual
// OFFSET: 0x007B2CA0
static void AcquireBuffers(uint32_t vertexCount, CGxBuf** vertexBuf, uint32_t indexCount, CGxBuf** indexBuf) {
    uint32_t count = CDetailDoodad::s_freeGxBufs.Count();

    if (count) {
        *vertexBuf = CDetailDoodad::s_freeGxBufs[count - 2];
        *indexBuf = CDetailDoodad::s_freeGxBufs[count - 1];
        CDetailDoodad::s_freeGxBufs.SetCount(count - 2);
        (*vertexBuf)->m_hasData = 0;
        (*indexBuf)->m_hasData = 0;
    } else {
        *vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPNCT), vertexCount);
        *indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), indexCount);
    }
}

// VERIFY: c83ff776 auto
// OFFSET: 0x007B0D40
int32_t CDetailDoodadInst::HasBatches() {
    for (uint32_t i = 0; i < 4; i++) {
        if (this->m_batches[i].m_vertexBuf) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: e98d17d0 auto
// OFFSET: 0x007B10E0
void CDetailDoodad::SetupVertexShader(const C44Matrix& world, uint32_t index) {
    uint32_t shadow = CShadowCache::GetShadowValue();

    if (shadow > 2) {
        shadow = 2;
    }

    g_theGxDevicePtr->RsSet(GxRs_VertexShader, CDetailDoodad::s_vertexShader[3 * index + shadow]);

    C44Matrix view;
    CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[GxXform_View];
    view = stack.m_mtx[stack.m_level];

    CDetailDoodad::s_shaderConstants.m_worldView = world * view;
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 0, reinterpret_cast<const float*>(&CDetailDoodad::s_shaderConstants), sizeof(CDetailDoodad::s_shaderConstants) / sizeof(C4Vector));
}

// VERIFY: f3428d3d manual
// OFFSET: 0x007B11B0
void CDetailDoodad::CreateAlphaRampTexture(const void*& texels) {
    static CImVector s_rampTexels[8][64];

    CImVector color;
    color.b = 255;
    color.g = 255;
    color.r = 255;

    for (uint32_t i = 0; i < 64; i++) {
        color.a = i * 4;

        for (uint32_t row = 0; row < 8; row++) {
            s_rampTexels[row][63 - i] = color;
        }
    }

    texels = s_rampTexels;
}

// VERIFY: e5f01c86 manual
// OFFSET: 0x007B1270
void CDetailDoodad::UpdateAlphaRampTexture(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    switch (command) {
        case GxTex_Latch:
            CDetailDoodad::CreateAlphaRampTexture(texels);
            texelStrideInBytes = width * 4;
            break;

        default:
            break;
    }
}

// VERIFY: c5224796 manual
// OFFSET: 0x007B12B0
void CDetailDoodadInstBatch::FillIndices() {
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(this->m_indexBuf));
    uint32_t index = 0;
    uint32_t base = 0;

    for (uint32_t i = 0; i < this->m_adds.Count(); i++) {
        CM2Model* model = CDetailDoodad::s_doodadList[this->m_adds[i].m_doodadId]->m_model;

        if (!(model->m_fFlags & 1)) {
            model->WaitForLoad(nullptr);
        }

        M2SkinProfile* skin = model->m_shared->m_skinData;

        for (uint32_t j = 0; j < skin->m_indices.m_count; j++) {
            indices[index++] = static_cast<uint16_t>(base + skin->m_indices[j]);
        }

        base += skin->m_vertices.m_count;
    }

    CGxBuf* buf = this->m_indexBuf;
    g_theGxDevicePtr->BufUnlock(buf, 0);
    buf->m_hasData = 1;
}

// VERIFY: 039ae168 manual
// OFFSET: 0x007B15D0
void CDetailDoodad::InitializeShaderConstants() {
    memset(&CDetailDoodad::s_shaderConstants, 0, sizeof(CDetailDoodad::s_shaderConstants));

    CGxDevice* device = g_theGxDevicePtr;
    C44Matrix view;
    CGxMatrixStack& stack = device->m_xforms[GxXform_View];
    view = stack.m_mtx[stack.m_level];

    SShaderConstants& consts = CDetailDoodad::s_shaderConstants;
    consts.m_worldView = C44Matrix();
    consts.m_projection = device->m_projNative;

    if (device->DeviceApi() == GxApi_OpenGl) {
        consts.m_projection.c0 *= -1.0f;
        consts.m_projection.c1 *= -1.0f;
        consts.m_projection.c2 *= -1.0f;
        consts.m_projection.c3 *= -1.0f;
    }

    C3Vector sunDir(0.0f, 0.0f, 0.0f);
    CM2Lighting lighting(sunDir);
    lighting.AddLight(&CMap::s_mapLight->m_light);

    if (lighting.GetSunLight(sunDir, consts.m_sunAmbient, consts.m_sunDiffuse, consts.m_sunSpecular)) {
        C33Matrix rotation(view);
        C3Vector dir = -sunDir * rotation;
        consts.m_sunDir.x = dir.x;
        consts.m_sunDir.y = dir.y;
        consts.m_sunDir.z = dir.z;
        consts.m_sunDir.w = 1.0f;
        consts.m_sunSpecularW = 0.0f;
    } else {
        consts.m_sunDir.x = 0.0f;
        consts.m_sunDir.y = 0.0f;
        consts.m_sunDir.z = 0.0f;
        consts.m_sunDir.w = 1.0f;
        consts.m_sunDiffuse = C3Vector(0.0f, 0.0f, 0.0f);
        consts.m_sunDiffuseW = 0.0f;
        consts.m_sunAmbient = C3Vector(1.0f, 1.0f, 1.0f);
        consts.m_sunAmbientW = 1.0f;
        consts.m_sunSpecular = C3Vector(0.0f, 0.0f, 0.0f);
        consts.m_sunSpecularW = 0.0f;
    }

    for (int32_t i = 0; i < 3; i++) {
        SShaderConstants::SPointLight& light = consts.m_pointLights[i];
        light.m_position = C3Vector(0.0f, 0.0f, 0.0f);
        light.m_positionW = 0.0f;
        light.m_color = C3Vector(0.0f, 0.0f, 0.0f);
        light.m_colorW = 0.0f;
        light.m_attenuation = C3Vector(1.0f, 0.0f, 0.0f);
        light.m_attenuationW = 0.0f;
    }

    if (g_theGxDevicePtr->MasterEnable(GxMasterEnable_Fog)) {
        DayNight::DNInfo* info = DayNight::GetInfo();
        CImVector color = info->m_fog.m_color;
        float end = info->m_fog.m_end;
        float range = 1.0f / (end - info->m_fog.m_start);
        consts.m_fog.x = -(CShaderEffect::s_fogMul * range);
        consts.m_fog.y = end * range;
        consts.m_fog.z = info->m_fog.m_density;
        consts.m_fog.w = 0.0f;

        if (g_theGxDevicePtr->Caps().unk_0134) {
            GxRsSet(GxRs_FogColor, *reinterpret_cast<int32_t*>(&color));
        } else {
            C4Vector fogColor(color);
            fogColor.w = static_cast<float>(static_cast<uint32_t>(CWorld::detailDoodadAlphaRef)) * 255.0f;
            g_theGxDevicePtr->ShaderConstantsSet(GxSh_Pixel, 2, &fogColor.x, 1);
        }

        if (g_theGxDevicePtr->Caps().unk_0138) {
            GxRsSet(GxRs_Fog, 1);
        }
    } else {
        consts.m_fog.x = 0.0f;
        consts.m_fog.y = 1.0f;
        consts.m_fog.z = 1.0f;
        consts.m_fog.w = 0.0f;

        if (g_theGxDevicePtr->Caps().unk_0138) {
            GxRsSet(GxRs_Fog, 0);
        }
    }

    float dist = CWorld::detailDoodadDist;
    float fade = 1.0f / (dist - dist * 0.85f);
    consts.m_fade.x = -1.0f * fade;
    consts.m_fade.y = dist * fade;
    consts.m_fade.z = 0.0f;
    consts.m_fade.w = 0.0f;
}

// VERIFY: 7ec422be manual
// OFFSET: 0x007B1B10
void CDetailDoodadData::ModelLoadedCallback(CM2Model* model, void* arg) {
    CDetailDoodadData* data = static_cast<CDetailDoodadData*>(arg);
    CM2Model* doodadModel = data->m_model;

    if (!(doodadModel->m_fFlags & 1)) {
        doodadModel->WaitForLoad(nullptr);
    }

    data->m_texture = CMap::LoadTexture(doodadModel->m_shared->m_data->m_textures[0].m_filename.Data());
}

// VERIFY: 69235ba2 manual
// OFFSET: 0x007B1B50
void CDetailDoodadInstBatch::FillVertices() {
    CGxBuf* buf = this->m_vertexBuf;
    CGxVertexPNCT* vertex = reinterpret_cast<CGxVertexPNCT*>(g_theGxDevicePtr->BufLock(buf));

    C3Vector axis(0.0f, 0.0f, 1.0f);
    C33Matrix basis(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    uint8_t built = 0;
    uint32_t cell = 0xFFFF;
    C33Matrix rotation(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    C33Matrix matrices[4] = {
        C33Matrix(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f),
        C33Matrix(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f),
        C33Matrix(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f),
        C33Matrix(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f)
    };

    for (uint32_t i = 0; i < this->m_adds.Count(); i++) {
        CDetailDoodadInstAdd& add = this->m_adds[i];
        CDetailDoodadData* data = CDetailDoodad::s_doodadList[add.m_doodadId];
        CM2Model* model = data->m_model;

        if (!(model->m_fFlags & 1)) {
            model->WaitForLoad(nullptr);
        }

        M2Data* m2Data = model->m_shared->m_data;
        model = data->m_model;

        if (!(model->m_fFlags & 1)) {
            model->WaitForLoad(nullptr);
        }

        M2SkinProfile* skin = model->m_shared->m_skinData;
        uint32_t vertexCount = skin->m_vertices.m_count;
        CImVector color = add.m_color;

        if (!CDetailDoodad::s_bUseShaders) {
            uint32_t scale = 76 * color.a + 45874;
            color.r = static_cast<uint8_t>((scale * color.r) >> 16);
            color.g = static_cast<uint8_t>((scale * color.g) >> 16);
            color.b = static_cast<uint8_t>((scale * color.b) >> 16);
            color.a = 0xFF;
        }

        if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
            CImVector swapped;
            swapped.a = color.a;
            swapped.g = color.g;
            swapped.r = color.b;
            swapped.b = color.r;
            color = swapped;
        }

        if (data->m_record->m_flags & 1) {
            if ((add.m_triIndex & 0xFC) != cell) {
                cell = add.m_triIndex & 0xFC;
                built = 0;
            }

            uint32_t fan = add.m_triIndex & 3;

            if (!(built & (1 << fan))) {
                const C3Vector& normal = add.m_normal;
                float inv = 1.0f / sqrt(-normal.y * -normal.y + normal.z * normal.z);
                C3Vector tangent(0.0f * inv, normal.z * inv, -normal.y * inv);
                C3Vector binormal(normal.y * tangent.z - normal.z * tangent.y, normal.z * tangent.x - tangent.z * normal.x, tangent.y * normal.x - tangent.x * normal.y);
                basis = C33Matrix(binormal.x, binormal.y, binormal.z, tangent.x, tangent.y, tangent.z, normal.x, normal.y, normal.z);
                BuildInstanceMatrix(matrices[fan], rotation, basis, add.m_rotation, axis);
                built |= 1 << fan;
            }

            const C33Matrix& matrix = matrices[fan];

            for (uint32_t j = 0; j < vertexCount; j++) {
                const M2Vertex& source = m2Data->m_vertices[skin->m_vertices[j]];
                C3Vector position = source.m_position * matrix;
                vertex->m_position.x = position.x * add.m_scale + add.m_position.x;
                vertex->m_position.y = position.y * add.m_scale + add.m_position.y;
                vertex->m_position.z = position.z * add.m_scale + add.m_position.z;
                vertex->m_normal = add.m_normal;
                vertex->m_color = color;
                vertex->m_texture = source.m_texcoord[0];
                vertex++;
            }
        } else {
            float c = cos(add.m_rotation);
            float s = sin(add.m_rotation);

            for (uint32_t j = 0; j < vertexCount; j++) {
                const M2Vertex& source = m2Data->m_vertices[skin->m_vertices[j]];
                vertex->m_position.x = (source.m_position.x * c - source.m_position.y * s) * add.m_scale + add.m_position.x;
                vertex->m_position.y = (source.m_position.y * c + source.m_position.x * s) * add.m_scale + add.m_position.y;
                vertex->m_position.z = source.m_position.z * add.m_scale + add.m_position.z;
                vertex->m_normal = add.m_normal;
                vertex->m_color = color;
                vertex->m_texture = source.m_texcoord[0];
                vertex++;
            }
        }
    }

    buf = this->m_vertexBuf;
    g_theGxDevicePtr->BufUnlock(buf, 0);
    buf->m_hasData = 1;
}

// VERIFY: a2c3ec0e manual
// OFFSET: 0x007B1380
void CDetailDoodad::DestroyModels() {
    for (uint32_t i = 0; i < CDetailDoodad::s_doodadList.Count(); i++) {
        if (CDetailDoodad::s_doodadList[i]) {
            CDetailDoodadData* data = CDetailDoodad::s_doodadList[i];

            if (data) {
                if (data->m_texture) {
                    HandleClose(reinterpret_cast<HOBJECT>(data->m_texture));
                    data->m_texture = nullptr;
                }

                if (data->m_model) {
                    data->m_model->Release();
                    data->m_model = nullptr;
                }

                data->m_record = nullptr;
                SMemFree(data, "delete", SLOG_FUNCTION, 0);
            }
        }

        CDetailDoodad::s_doodadList[i] = nullptr;
    }
}

// VERIFY: 9944e10b manual
// OFFSET: 0x007B2760
void CDetailDoodad::Initialize() {
    CDetailDoodad::s_instHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CDetailDoodadInst), 16, "WDETAILDOODADINST", true));

    uint32_t count = g_groundEffectDoodadDB.m_maxID + 1;
    CDetailDoodad::s_doodadList.SetCount(count);

    for (uint32_t i = 0; i < CDetailDoodad::s_doodadList.Count(); i++) {
        CDetailDoodad::s_doodadList[i] = nullptr;
    }

    for (uint32_t i = 0; i < count; i++) {
        GroundEffectDoodadRec* record = g_groundEffectDoodadDB.GetRecord(i);

        if (record) {
            CDetailDoodadData* data = new (__FILE__, __LINE__) CDetailDoodadData();
            data->m_record = record;
            CDetailDoodad::s_doodadList[i] = data;
        }
    }

    GxTexCreate(64, 8, GxTex_Argb8888, CGxTexFlags(), nullptr, CDetailDoodad::UpdateAlphaRampTexture, CDetailDoodad::s_alphaRampTexture);

    memset(CDetailDoodad::s_vertexShader, 0, sizeof(CDetailDoodad::s_vertexShader));
    memset(CDetailDoodad::s_pixelShader, 0, sizeof(CDetailDoodad::s_pixelShader));

    CDetailDoodad::s_vtxBufItems = 0;
    CDetailDoodad::s_idxBufItems = 0;
    s_vertexBufferSize = 0;
    s_indexBufferSize = 0;
    CDetailDoodad::s_freeGxBufs.m_count = 0;
    unk_D1C4DC = 0;
    CDetailDoodad::s_gxVertexPool = nullptr;
    CDetailDoodad::s_gxIndexPool = nullptr;
    CDetailDoodad::s_bPoolsDirty = 1;

    CDetailDoodad::s_bUseShaders = 1;

    if (!(CWorld::enables & Enable_PixelShaders)) {
        CDetailDoodad::s_bUseShaders = 0;
    }

    if (!(CWorld::enables2 & Enable_VertexShader)) {
        CDetailDoodad::s_bUseShaders = 0;
    } else if (CDetailDoodad::s_bUseShaders) {
        g_theGxDevicePtr->ShaderCreate(CDetailDoodad::s_vertexShader, GxSh_Vertex, "Shaders\\Vertex", "DetailDoodad", sizeof(CDetailDoodad::s_vertexShader) / sizeof(CDetailDoodad::s_vertexShader[0]));
        g_theGxDevicePtr->ShaderCreate(CDetailDoodad::s_pixelShader, GxSh_Pixel, "Shaders\\Pixel", "DetailDoodad", sizeof(CDetailDoodad::s_pixelShader) / sizeof(CDetailDoodad::s_pixelShader[0]));
    }
}

// VERIFY: 2ff521ae auto
// OFFSET: 0x007B29B0
void CDetailDoodad::FreeGxBufMat() {
    for (uint32_t i = 0; i < CDetailDoodad::s_freeGxBufs.Count(); i++) {
        CGxBuf* buf = CDetailDoodad::s_freeGxBufs[i];

        if (buf && buf != g_theGxDevicePtr->BufStream(buf->m_pool->m_target, 0, 0)) {
            g_theGxDevicePtr->BufDestroy(buf);
        }
    }

    CDetailDoodad::s_freeGxBufs.SetCount(0);
    unk_D1C4DC = 0;

    if (CDetailDoodad::s_gxVertexPool) {
        g_theGxDevicePtr->PoolDestroy(CDetailDoodad::s_gxVertexPool);
    }

    if (CDetailDoodad::s_gxIndexPool) {
        g_theGxDevicePtr->PoolDestroy(CDetailDoodad::s_gxIndexPool);
    }

    CDetailDoodad::s_vtxBufItems = 0;
    CDetailDoodad::s_idxBufItems = 0;
    s_vertexBufferSize = 0;
    s_indexBufferSize = 0;
    unk_D1C4DC = 0;
    CDetailDoodad::s_gxVertexPool = nullptr;
    CDetailDoodad::s_gxIndexPool = nullptr;
    CDetailDoodad::s_bPoolsDirty = 1;
}

// VERIFY: 08827cd3 manual
// OFFSET: 0x007B2A80
void CDetailDoodad::UpdatePools() {
    if (!CDetailDoodad::s_bPoolsDirty) {
        return;
    }

    CMap::ClearChunkDetailDoodads();
    CDetailDoodad::FreeGxBufMat();

    uint32_t vertexItems = World::s_detailDoodadDensity << 6;
    uint32_t indexItems = World::s_detailDoodadDensity << 6;
    CDetailDoodad::s_vtxBufItems = vertexItems;
    CDetailDoodad::s_idxBufItems = indexItems;

    if (vertexItems > 4096) {
        vertexItems = 4096;
        CDetailDoodad::s_vtxBufItems = 4096;
    }

    if (indexItems > 4096) {
        indexItems = 4096;
        CDetailDoodad::s_idxBufItems = 4096;
    }

    s_vertexBufferSize = sizeof(CGxVertexPNCT) * vertexItems;
    s_indexBufferSize = sizeof(uint16_t) * indexItems;

    CDetailDoodad::s_gxVertexPool = g_theGxDevicePtr->PoolCreate(GxPoolTarget_Vertex, GxPoolUsage_Dynamic, 128 * sizeof(CGxVertexPNCT) * vertexItems, GxPoolHintBit_Unk3, "CDetailDoodad_vtx");
    CDetailDoodad::s_gxIndexPool = g_theGxDevicePtr->PoolCreate(GxPoolTarget_Index, GxPoolUsage_Dynamic, 128 * sizeof(uint16_t) * indexItems, GxPoolHintBit_Unk0, "CDetailDoodad_idx");

    CDetailDoodad::s_freeGxBufs.Reserve(256, 0);

    for (uint32_t i = 0; i < 256; i += 2) {
        *CDetailDoodad::s_freeGxBufs.New() = g_theGxDevicePtr->BufCreate(CDetailDoodad::s_gxVertexPool, sizeof(CGxVertexPNCT), CDetailDoodad::s_vtxBufItems, sizeof(CGxVertexPNCT) * CDetailDoodad::s_vtxBufItems * (i >> 1));
        *CDetailDoodad::s_freeGxBufs.New() = g_theGxDevicePtr->BufCreate(CDetailDoodad::s_gxIndexPool, sizeof(uint16_t), CDetailDoodad::s_idxBufItems, sizeof(uint16_t) * CDetailDoodad::s_idxBufItems * (i >> 1));
        unk_D1C4E8 += 2;
    }

    unk_D1C4DC = 256;
    CDetailDoodad::s_bPoolsDirty = 0;
}

// VERIFY: 564457f1 manual
// OFFSET: 0x007B2D30
int32_t CDetailDoodad::InitializeRenderState() {
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_BlendingMode, 2);
    GxRsSet(GxRs_AlphaRef, CWorld::detailDoodadAlphaRef);
    GxRsSet(GxRs_DepthWrite, 1);

    if (CDetailDoodad::s_bUseShaders) {
        uint32_t shadow = CShadowCache::GetShadowValue();
        g_theGxDevicePtr->RsSet(GxRs_PixelShader, CDetailDoodad::s_pixelShader[4 * ((CWorld::enables2 >> 1) & 1) + shadow]);
        CDetailDoodad::InitializeShaderConstants();
        CShadowCache::SetShadowMapDetailDoodad();
        return 1;
    }

    GxRsSet(GxRs_ColorOp0, 0);
    GxRsSet(GxRs_AlphaOp0, 0);
    GxRsSet(GxRs_MatDiffuse, static_cast<int32_t>(0xFFFFFFFF));
    g_theGxDevicePtr->RsSet(GxRs_Texture1, CDetailDoodad::s_alphaRampTexture);
    GxRsSet(GxRs_ColorOp1, 0);
    GxRsSet(GxRs_AlphaOp1, 0);
    GxRsSet(GxRs_TexGen1, 3);
    GxRsSet(GxRs_TextureShader1, 1);

    C44Matrix matrix;
    float dist = CWorld::detailDoodadDist;
    matrix.Scale(1.0f / (dist * 0.15f));
    matrix.RotateAroundY(1.5707964f);
    matrix.Translate(C3Vector(0.0f, 0.0f, dist * -0.85f));

    CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[GxXform_Tex1];
    stack.m_dirty = 1;
    stack.m_flags[stack.m_level] &= ~CGxMatrixStack::F_Identity;
    stack.m_mtx[stack.m_level] = matrix;

    return 0;
}

// VERIFY: 168f79b0 auto
// OFFSET: 0x007B3050
int32_t CDetailDoodadData::Load() {
    if (!this->m_model) {
        char path[260];
        size_t length = SStrCopy(path, "World\\NoDXT\\Detail\\", STORM_MAX_STR);
        SStrCopy(path + length, this->m_record->m_doodadpath, STORM_MAX_STR);

        this->m_model = World::s_m2Scene->CreateModel(path, 0);

        if (!this->m_model) {
            return 0;
        }

        this->m_model->SetLoadedCallback(CDetailDoodadData::ModelLoadedCallback, this);
    }

    return 1;
}

// VERIFY: 50911cf6 manual
// OFFSET: 0x007B30D0
void CDetailDoodadInst::FreeBuffers() {
    for (uint32_t i = 0; i < 4; i++) {
        this->m_batches[i].FreeBuffers();
    }
}

// VERIFY: b905f512 manual
// OFFSET: 0x007B31E0
void CDetailDoodadInst::AddDoodad(uint32_t id, const C3Vector& position, float rotation, float scale, const C3Vector& normal, uint16_t triIndex, const CImVector& color) {
    CDetailDoodadData* data = CDetailDoodad::s_doodadList[id];
    CM2Model* model = data->m_model;

    if (!(model->m_fFlags & 1)) {
        model->WaitForLoad(nullptr);
    }

    M2SkinProfile* skin = model->m_shared->m_skinData;
    uint32_t vertexCount = skin->m_vertices.m_count;
    uint32_t indexCount = skin->m_indices.m_count;

    uint32_t i;

    for (i = 0; i < 4; i++) {
        CDetailDoodadInstBatch& batch = this->m_batches[i];

        if (!HandleCompare(reinterpret_cast<HOBJECT>(batch.m_texture), reinterpret_cast<HOBJECT>(data->m_texture)) && vertexCount + batch.m_vertexCount < CDetailDoodad::s_vtxBufItems && indexCount + batch.m_indexCount < CDetailDoodad::s_idxBufItems) {
            break;
        }
    }

    if (i == 4) {
        for (i = 0; i < 4; i++) {
            if (!this->m_batches[i].m_texture) {
                break;
            }
        }

        if (i == 4) {
            return;
        }

        this->m_batches[i].m_texture = data->m_texture;
    }

    CDetailDoodadInstBatch& batch = this->m_batches[i];
    CDetailDoodadInstAdd* add = batch.m_adds.New();
    add->m_doodadId = id;
    add->m_position = position;
    add->m_rotation = rotation;
    add->m_scale = scale;
    add->m_normal = normal;
    add->m_triIndex = triIndex;
    add->m_color = color;

    batch.m_vertexCount += vertexCount;
    batch.m_indexCount += indexCount;
}

// VERIFY: 9bfece74 manual
// OFFSET: 0x007B3390
void CDetailDoodadInstBatch::Draw() {
    if (!this->m_texture) {
        return;
    }

    CGxTex* gxTex = TextureGetGxTex(this->m_texture, 0, nullptr);

    if (!gxTex) {
        return;
    }

    if (!this->m_vertexBuf || this->m_vertexBuf->m_pool->m_usage == GxPoolUsage_Stream) {
        AcquireBuffers(this->m_vertexCount, &this->m_vertexBuf, this->m_indexCount, &this->m_indexBuf);
    }

    if (!this->m_vertexBuf->m_hasData || !this->m_vertexBuf->m_valid) {
        this->FillVertices();
    }

    GxPrimVertexPtr(this->m_vertexBuf, GxVBF_PNCT);

    if (!this->m_indexBuf->m_hasData || !this->m_indexBuf->m_valid) {
        this->FillIndices();
    }

    g_theGxDevicePtr->PrimIndexPtr(this->m_indexBuf);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);

    CGxBatch batch;
    batch.m_primType = GxPrim_Triangles;
    batch.m_start = 0;
    batch.m_count = this->m_indexCount;
    batch.m_minIndex = 0;
    batch.m_maxIndex = static_cast<uint16_t>(this->m_vertexCount - 1);
    g_theGxDevicePtr->Draw(&batch, 1);
}

// VERIFY: 84475972 auto
// OFFSET: 0x007B3480
void CDetailDoodad::Destroy() {
    CDetailDoodad::DestroyModels();
    CDetailDoodad::FreeGxBufMat();

    if (CDetailDoodad::s_alphaRampTexture) {
        GxTexDestroy(CDetailDoodad::s_alphaRampTexture);
    }

    CDetailDoodad::s_alphaRampTexture = nullptr;

    if (CDetailDoodad::s_instHeap) {
        SMemFree(CDetailDoodad::s_instHeap, "delete", SLOG_FUNCTION, 0);
    }

    CDetailDoodad::s_instHeap = nullptr;

    for (uint32_t i = 0; i < sizeof(CDetailDoodad::s_vertexShader) / sizeof(CDetailDoodad::s_vertexShader[0]); i++) {
        if (CDetailDoodad::s_vertexShader[i]) {
            g_theGxDevicePtr->ShaderDestroy(CDetailDoodad::s_vertexShader[i]);
        }
    }

    for (uint32_t i = 0; i < sizeof(CDetailDoodad::s_pixelShader) / sizeof(CDetailDoodad::s_pixelShader[0]); i++) {
        if (CDetailDoodad::s_pixelShader[i]) {
            g_theGxDevicePtr->ShaderDestroy(CDetailDoodad::s_pixelShader[i]);
        }
    }
}

// VERIFY: d3db66d2 auto
// OFFSET: 0x007B3530
int32_t CDetailDoodad::LoadModel(uint32_t id) {
    CDetailDoodadData* data = CDetailDoodad::s_doodadList[id];

    if (!data) {
        return 0;
    }

    if (!data->m_model) {
        data->Load();
        return 0;
    }

    return data->m_model->IsLoaded(0, 0);
}

// VERIFY: f62a81be manual
// OFFSET: 0x007B3560
void CDetailDoodadInst::Free() {
    if (this->m_chunk) {
        this->m_chunk->m_detailDoodadInst = nullptr;
        this->m_chunk = nullptr;
    }

    for (uint32_t i = 0; i < 4; i++) {
        this->m_batches[i].Free();
    }
}

// VERIFY: 104e29d5 auto
// OFFSET: 0x007B36B0
void CDetailDoodadInst::DrawBatches() {
    for (uint32_t i = 0; i < 4; i++) {
        if (this->m_batches[i].m_texture) {
            this->m_batches[i].Draw();
        }
    }
}

// INLINED
CDetailDoodadInstBatch::CDetailDoodadInstBatch() {
    this->m_texture = nullptr;
    this->m_vertexCount = 0;
    this->m_indexCount = 0;
    this->m_vertexBuf = nullptr;
    this->m_indexBuf = nullptr;
}

// VERIFY: 9e2e57b8 manual
// OFFSET: 0x007B36E0
CDetailDoodadInst::CDetailDoodadInst() {
    this->m_age = 0;
    this->m_chunk = nullptr;
}

// VERIFY: 166795d2 manual
// OFFSET: 0x007B3780
CDetailDoodadInst::~CDetailDoodadInst() {
    this->Free();
}

// VERIFY: dfac07db auto
// OFFSET: 0x007B3910
CDetailDoodadInst* CDetailDoodad::AllocInst() {
    uint32_t memHandle;
    void* object = nullptr;

    if (!ObjectAlloc(*CDetailDoodad::s_instHeap, &memHandle, &object, false)) {
        return nullptr;
    }

    CDetailDoodadInst* inst = new (object) CDetailDoodadInst();
    inst->m_objectIndex = memHandle;

    return inst;
}

// VERIFY: d19634a7 auto
// OFFSET: 0x007B3960
void CDetailDoodad::FreeInst(CDetailDoodadInst* inst) {
    uint32_t* heap = CDetailDoodad::s_instHeap;
    inst->~CDetailDoodadInst();
    ObjectFree(*heap, inst->m_objectIndex);
}
