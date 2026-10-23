#include "Particulate.h"
#include <cmath>
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxVertexPCT.h"
#include "Engine/Source/Gx/Gx.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/LiquidTypeRec.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/World.h"

static const float PTSIZE = 0.19921875f;

// STATIC: 0x00A3F970
static const int32_t s_texSlots[5][8] = {
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 8, 9, 10, 11, 8, 9, 10, 11 },
    { 12, 12, 12, 12, 12, 12, 12, 12 },
    { 13, 14, 15, 16, 13, 14, 15, 16 },
    { 17, 18, 19, 20, 21, 22, 23, 24 },
};

// STATIC: 0x00ADF808
static C3Vector s_corners[4] = {
    C3Vector(-0.5f, 0.5f, 0.0f),
    C3Vector(-0.5f, -0.5f, 0.0f),
    C3Vector(0.5f, 0.5f, 0.0f),
    C3Vector(0.5f, -0.5f, 0.0f),
};

// STATIC: 0x00ADF838
static C2Vector s_texCoords[25][4] = {
    { C2Vector(0.0f, 0.0f), C2Vector(0.0f, PTSIZE), C2Vector(PTSIZE, 0.0f), C2Vector(PTSIZE, PTSIZE) },
    { C2Vector(PTSIZE, 0.0f), C2Vector(PTSIZE, PTSIZE), C2Vector(0.3984375f, 0.0f), C2Vector(0.3984375f, PTSIZE) },
    { C2Vector(0.3984375f, 0.0f), C2Vector(0.3984375f, PTSIZE), C2Vector(0.59765625f, 0.0f), C2Vector(0.59765625f, PTSIZE) },
    { C2Vector(0.59765625f, 0.0f), C2Vector(0.59765625f, PTSIZE), C2Vector(0.796875f, 0.0f), C2Vector(0.796875f, PTSIZE) },
    { C2Vector(0.0f, PTSIZE), C2Vector(0.0f, 0.3984375f), C2Vector(PTSIZE, PTSIZE), C2Vector(PTSIZE, 0.3984375f) },
    { C2Vector(PTSIZE, PTSIZE), C2Vector(PTSIZE, 0.3984375f), C2Vector(0.3984375f, PTSIZE), C2Vector(0.3984375f, 0.3984375f) },
    { C2Vector(0.3984375f, PTSIZE), C2Vector(0.3984375f, 0.3984375f), C2Vector(0.59765625f, PTSIZE), C2Vector(0.59765625f, 0.3984375f) },
    { C2Vector(0.59765625f, PTSIZE), C2Vector(0.59765625f, 0.3984375f), C2Vector(0.796875f, PTSIZE), C2Vector(0.796875f, 0.3984375f) },
    { C2Vector(0.0f, 0.3984375f), C2Vector(0.0f, 0.59765625f), C2Vector(PTSIZE, 0.3984375f), C2Vector(PTSIZE, 0.59765625f) },
    { C2Vector(PTSIZE, 0.3984375f), C2Vector(PTSIZE, 0.59765625f), C2Vector(0.3984375f, 0.3984375f), C2Vector(0.3984375f, 0.59765625f) },
    { C2Vector(0.3984375f, 0.3984375f), C2Vector(0.3984375f, 0.59765625f), C2Vector(0.59765625f, 0.3984375f), C2Vector(0.59765625f, 0.59765625f) },
    { C2Vector(0.59765625f, 0.3984375f), C2Vector(0.59765625f, 0.59765625f), C2Vector(0.796875f, 0.3984375f), C2Vector(0.796875f, 0.59765625f) },
    { C2Vector(0.796875f, 0.0f), C2Vector(0.796875f, PTSIZE), C2Vector(0.99609375f, 0.0f), C2Vector(0.99609375f, PTSIZE) },
    { C2Vector(0.796875f, PTSIZE), C2Vector(0.796875f, 0.3984375f), C2Vector(0.99609375f, PTSIZE), C2Vector(0.99609375f, 0.3984375f) },
    { C2Vector(0.796875f, 0.3984375f), C2Vector(0.796875f, 0.59765625f), C2Vector(0.99609375f, 0.3984375f), C2Vector(0.99609375f, 0.59765625f) },
    { C2Vector(0.796875f, 0.59765625f), C2Vector(0.796875f, 0.796875f), C2Vector(0.99609375f, 0.59765625f), C2Vector(0.99609375f, 0.796875f) },
    { C2Vector(0.796875f, 0.796875f), C2Vector(0.796875f, 0.99609375f), C2Vector(0.99609375f, 0.796875f), C2Vector(0.99609375f, 0.99609375f) },
    { C2Vector(0.0f, 0.59765625f), C2Vector(0.0f, 0.796875f), C2Vector(PTSIZE, 0.59765625f), C2Vector(PTSIZE, 0.796875f) },
    { C2Vector(PTSIZE, 0.59765625f), C2Vector(PTSIZE, 0.796875f), C2Vector(0.3984375f, 0.59765625f), C2Vector(0.3984375f, 0.796875f) },
    { C2Vector(0.3984375f, 0.59765625f), C2Vector(0.3984375f, 0.796875f), C2Vector(0.59765625f, 0.59765625f), C2Vector(0.59765625f, 0.796875f) },
    { C2Vector(0.59765625f, 0.59765625f), C2Vector(0.59765625f, 0.796875f), C2Vector(0.796875f, 0.59765625f), C2Vector(0.796875f, 0.796875f) },
    { C2Vector(0.0f, 0.796875f), C2Vector(0.0f, 0.99609375f), C2Vector(PTSIZE, 0.796875f), C2Vector(PTSIZE, 0.99609375f) },
    { C2Vector(PTSIZE, 0.796875f), C2Vector(PTSIZE, 0.99609375f), C2Vector(0.3984375f, 0.796875f), C2Vector(0.3984375f, 0.99609375f) },
    { C2Vector(0.3984375f, 0.796875f), C2Vector(0.3984375f, 0.99609375f), C2Vector(0.59765625f, 0.796875f), C2Vector(0.59765625f, 0.99609375f) },
    { C2Vector(0.59765625f, 0.796875f), C2Vector(0.59765625f, 0.99609375f), C2Vector(0.796875f, 0.796875f), C2Vector(0.796875f, 0.99609375f) },
};

// STATIC: 0x00ADFBBC
static float s_sinkSpeed = -0.02f;

// STATIC: 0x00ADFBC0
static float s_riseSpeed = 0.02f;

// VERIFY: 464f8a1f auto
// OFFSET: 0x0079B360
void Particulate::SetScale(float scale) {
    this->m_particleSize = scale;
}

// VERIFY: 1f1e47d5 manual
// OFFSET: 0x0079E100
Particulate::Particulate(float particleSize, float spawnAreaSize, const char* textureName) {
    this->m_enabled = 0;
    this->m_texture = nullptr;
    this->m_particleSize = particleSize;
    this->m_maxParticles = sizeof(this->m_particles) / sizeof(this->m_particles[0]);
    this->m_spawnAreaSize = spawnAreaSize;
    this->SetTexture(textureName);
    this->InitParticles(2);
    this->InitMovement();
}

// VERIFY: 1e6bbe9a auto
// OFFSET: 0x0079B340
Particulate::~Particulate() {
    if (this->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
    }
}

// VERIFY: 2471da3f manual
// OFFSET: 0x0079B8E0
void Particulate::InitParticles(uint32_t count) {
    float minSize = this->m_particleSize * 0.5f;
    float sizeRange = this->m_particleSize * 1.5f - minSize;
    float halfArea = 0.5f * this->m_spawnAreaSize;

    for (uint32_t i = 0; i < this->m_maxParticles; i++) {
        this->m_particles[i].m_position = C3Vector(CRandom::ufloat_(g_rndSeed) * this->m_spawnAreaSize - halfArea, CRandom::ufloat_(g_rndSeed) * this->m_spawnAreaSize - halfArea, CRandom::ufloat_(g_rndSeed) * this->m_spawnAreaSize - halfArea);
        this->m_particles[i].m_size = CRandom::ufloat_(g_rndSeed) * sizeRange + minSize;
    }

    this->m_liquidType = count;
}

// VERIFY: d68e0ec8 manual
// OFFSET: 0x0079BCC0
void Particulate::InitMovement() {
    float theta = CRandom::sfloat_(g_rndSeed) * 3.1415927f;
    float phi = CRandom::sfloat_(g_rndSeed) * 3.1415927f;
    float sinTheta = sin(theta);

    this->m_direction.y = sin(phi) * sinTheta;
    this->m_direction.x = sinTheta * cos(phi);
    this->m_direction.z = cos(theta) * 0.25f;

    if (this->m_direction.z < 0.0f) {
        this->m_direction.z = -this->m_direction.z;
    }

    float invLength = 1.0f / sqrt(this->m_direction.x * this->m_direction.x + this->m_direction.y * this->m_direction.y + this->m_direction.z * this->m_direction.z);
    this->m_direction.x = this->m_direction.x * invLength;
    this->m_direction.y = this->m_direction.y * invLength;
    this->m_direction.z = this->m_direction.z * invLength;

    this->m_timer = 0.0f;
    this->m_speed = (CRandom::ufloat_(g_rndSeed) + 1.0f) * 0.0125f;
    this->m_rotationSpeed = (CRandom::ufloat_(g_rndSeed) + 1.0f) * 0.005f;
}

// VERIFY: d40a0168 manual
// OFFSET: 0x0079DFF0
void Particulate::SetTexture(const char* textureName) {
    if (this->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
    }

    CStatus status;
    this->m_texture = TextureCreate(textureName, CGxTexFlags(GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1), &status, 0);
    SysMsgAdd(&status);
}

// VERIFY: 830c4e88 manual
// OFFSET: 0x0079BE50
C3Vector Particulate::ComputeMovement(float elapsed) {
    LiquidTypeRec* liquidType = g_liquidTypeDB.GetRecord(this->m_liquidType);

    if (liquidType->m_particleMovement == 1) {
        return C3Vector(0.0f, 0.0f, s_sinkSpeed * elapsed);
    }

    if (liquidType->m_particleMovement == 2) {
        return C3Vector(0.0f, 0.0f, s_riseSpeed * elapsed);
    }

    if (liquidType->m_particleMovement) {
        return C3Vector(0.0f, 0.0f, 0.0f);
    }

    this->m_timer = elapsed + this->m_timer;
    float phase = this->m_timer * this->m_speed;

    if (phase > 0.5f) {
        this->InitMovement();
        phase = 0.0f;
    }

    float amount = CMath::sin_(phase * 6.2831855f) * this->m_rotationSpeed;
    return C3Vector(this->m_direction.x * amount, this->m_direction.y * amount, amount * this->m_direction.z);
}

// VERIFY: 680e77ec manual
// OFFSET: 0x0079BF40
void Particulate::Update() {
    if (!this->m_enabled) {
        return;
    }

    float halfArea = this->m_spawnAreaSize * 0.5f;
    C3Vector delta(this->m_prevCameraPos.x - CWorldScene::s_activeWorldView.x, this->m_prevCameraPos.y - CWorldScene::s_activeWorldView.y, this->m_prevCameraPos.z - CWorldScene::s_activeWorldView.z);
    this->m_prevCameraPos = CWorldScene::s_activeWorldView;

    if (this->m_spawnAreaSize * this->m_spawnAreaSize < delta.x * delta.x + delta.y * delta.y + delta.z * delta.z) {
        this->InitParticles(this->m_liquidType);
        delta.x = 0.0f;
        delta.y = 0.0f;
        delta.z = 0.0f;
    }

    C3Vector movement = this->ComputeMovement(FrameTime::s_tickTimeSec);
    float moveX = delta.x + movement.x;
    float moveY = movement.y + delta.y;
    float moveZ = movement.z + delta.z;

    for (uint32_t i = 0; i < this->m_maxParticles; i++) {
        C3Vector& position = this->m_particles[i].m_position;
        float x = position.x + moveX;
        position.x = x;
        float y = position.y + moveY;
        position.y = y;
        float z = position.z + moveZ;
        position.z = z;

        if (x > halfArea) {
            position.x = x - this->m_spawnAreaSize;
        } else if (-halfArea > x) {
            position.x = x + this->m_spawnAreaSize;
        }

        if (y > halfArea) {
            position.y = y - this->m_spawnAreaSize;
        } else if (-halfArea > y) {
            position.y = y + this->m_spawnAreaSize;
        }

        if (z > halfArea) {
            position.z = z - this->m_spawnAreaSize;
        } else if (-halfArea > z) {
            position.z = z + this->m_spawnAreaSize;
        }
    }
}

// VERIFY: 74f44f8f manual
// OFFSET: 0x0079CA70
void Particulate::Render() {
    if (!this->m_enabled) {
        return;
    }

    CGxTex* texture = TextureGetGxTex(this->m_texture, 0, nullptr);

    if (!texture) {
        return;
    }

    CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPCT), 2664);
    CGxVertexPCT* vertices = reinterpret_cast<CGxVertexPCT*>(g_theGxDevicePtr->BufLock(vertexBuf));
    CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), 3996);
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
    C44Matrix view;
    GxXformView(view);
    C44Matrix identity;
    g_theGxDevicePtr->XformSetView(identity);
    LiquidTypeRec* liquidType = g_liquidTypeDB.GetRecord(this->m_liquidType);
    uint32_t vertexCount = 0;
    int32_t texSlot = 8;

    for (uint32_t i = 0; i < this->m_maxParticles; i++) {
        LiquidParticle& particle = this->m_particles[i];
        float viewX = particle.m_position.z * view.c0 + view.b0 * particle.m_position.y + view.a0 * particle.m_position.x;
        float depth = particle.m_position.z * view.c2 + view.b2 * particle.m_position.y + view.a2 * particle.m_position.x;

        if (depth > 0.0f && depth > viewX && -depth < viewX) {
            float viewY = particle.m_position.z * view.c1 + view.a1 * particle.m_position.x + view.b1 * particle.m_position.y;

            if (viewY < depth && viewY > -depth) {
                for (uint32_t j = 0; j < 4; j++) {
                    vertices->p.x = s_corners[j].x * particle.m_size + viewX;
                    vertices->p.y = s_corners[j].y * particle.m_size + viewY;
                    vertices->p.z = depth;
                    vertices->c = CImVector(0xFF, 0xFF, 0xFF, 0xFF);
                    vertices->tc = s_texCoords[texSlot][j];
                    vertices++;
                }

                indices[0] = vertexCount;
                indices[3] = vertexCount + 3;
                indices[1] = vertexCount + 1;
                indices[2] = vertexCount + 2;
                indices[4] = vertexCount + 2;
                indices[5] = vertexCount + 1;
                vertexCount += 4;
                indices += 6;

                if ((vertexCount & ~0x3) >= 2664) {
                    break;
                }
            }
        }

        texSlot = s_texSlots[liquidType->m_particleTexSlots][i & 7];
    }

    GxBufUnlock(indexBuf, 0);
    GxBufUnlock(vertexBuf, 0);
    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_Fog, (liquidType->m_flags >> 4) & 1);
    GxRsSet(GxRs_Texture0, texture);
    GxRsSet(GxRs_BlendingMode, 2);
    GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_BlendingMode].m_value.m_data.i[0]]));
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxPrimVertexPtr(vertexBuf, GxVBF_PCT);
    g_theGxDevicePtr->PrimIndexPtr(indexBuf);
    CGxBatch batch;
    batch.m_primType = GxPrim_Triangles;
    batch.m_start = 0;
    batch.m_count = 6 * (vertexCount >> 2);
    batch.m_minIndex = 0;
    batch.m_maxIndex = vertexCount - 1;
    g_theGxDevicePtr->Draw(&batch, 1);
    g_theGxDevicePtr->RsPop();
    g_theGxDevicePtr->XformSetView(view);
}
