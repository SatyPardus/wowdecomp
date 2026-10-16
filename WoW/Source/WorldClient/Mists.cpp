#include "Mists.h"
#include <cmath>
#include <cstdint>
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/CGxVertexPCT.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/MapWeather.h"
#include "WoW/Source/WorldClient/World.h"

// INLINED
Mists::Mist::Mist() {
    this->m_maxHeights = 64;
    this->m_position.x = 0.0f;
    this->m_position.y = 0.0f;
    this->m_position.z = 0.0f;
    this->m_velocity.x = 0.0f;
    this->m_velocity.y = 0.0f;
    this->m_velocity.z = 0.0f;
    this->m_startTick = 0;
}

// VERIFY: 03b6110f manual
// OFFSET: 0x00786330
void Mists::CreateMist(Mist* mist, uint32_t startTick) {
    this->InitMist(mist, startTick);
    int32_t ticks = mist->m_endTick - mist->m_startTick;
    C3Vector position = mist->m_position;
    float life = ticks * 0.0009765625f;
    float cellSize;
    float cellsPerYard;
    World::s_weather->m_grid->GetCellSpacing(&cellSize, &cellsPerYard);
    float speed = sqrt(mist->m_velocity.x * mist->m_velocity.x + mist->m_velocity.y * mist->m_velocity.y);
    float invSpeed = 1.0f / speed;
    float stepX = mist->m_velocity.x * invSpeed * cellSize;
    float stepY = mist->m_velocity.y * invSpeed * cellSize;
    float stepZ = cellSize * (invSpeed * 0.0f);
    int32_t count = static_cast<int32_t>(static_cast<int64_t>(speed * life / sqrt(stepY * stepY + stepX * stepX)));
    mist->m_heightCount = count;

    if (count >= 64) {
        count = 64;
    }

    mist->m_heightCount = count;

    for (int32_t i = 0; i < mist->m_heightCount; i++) {
        mist->m_heights[i] = World::s_weather->m_grid->GetHeight(&position);
        position.x += stepX;
        position.y += stepY;
        position.z += stepZ;
    }

    if (mist->m_heightCount - 3 <= 0) {
        return;
    }

    int32_t index = 0;

    while (mist->m_heights[index + 3] - mist->m_heights[index] <= 1.0f && mist->m_heights[index + 2] - mist->m_heights[index] <= 0.75f && mist->m_heights[index + 1] - mist->m_heights[index] <= 0.5f) {
        if (++index >= mist->m_heightCount - 3) {
            return;
        }
    }

    float endTicks = static_cast<float>(index + 1) / mist->m_heightCount * life * 1024.0f;
    mist->m_heightCount = index;
    mist->m_endTick = CMath::fint_(endTicks - 0.5f) + mist->m_startTick;

    if (!index) {
        mist->m_endTick = 0;
        mist->m_startTick = 0;
    }
}

// VERIFY: 7b333740 manual
// OFFSET: 0x00786560
void Mists::InitMist(Mist* mist, uint32_t startTick) {
    mist->m_startTick = startTick;
    float angle = (CRandom::ufloat_(g_rndSeed) - 0.5f) * this->m_angleVariance + this->m_angle;
    float c = cos(angle);
    float s = sin(angle);
    float speed = (CRandom::ufloat_(g_rndSeed) - 0.5f) * this->m_speedVariance + this->m_speed;
    float rise = CRandom::ufloat_(g_rndSeed);
    mist->m_velocity.x = s * speed;
    mist->m_velocity.y = speed * c;
    mist->m_velocity.z = (rise - 0.5f) * 0.033333335f + 0.33333334f;
    C44Matrix rotation = C44Matrix::RotationAroundZ(-World::s_weather->m_facing);
    mist->m_velocity = rotation * mist->m_velocity;
    float randomZ = CRandom::ufloat_(g_rndSeed);
    float randomY = CRandom::ufloat_(g_rndSeed);
    float randomX = CRandom::ufloat_(g_rndSeed);
    mist->m_position.x = (randomX - 0.5f) * this->m_extents.x;
    mist->m_position.y = (randomY - 0.5f) * this->m_extents.y;
    mist->m_position.z = (randomZ - 0.5f) * this->m_extents.z;
    mist->m_position = rotation * mist->m_position;
    float z = CWorldScene::s_activeWorldView.z - 1.5f * mist->m_velocity.z;
    float y = CWorldScene::s_activeWorldView.y - mist->m_velocity.y * 1.5f;
    mist->m_position.x = CWorldScene::s_activeWorldView.x - mist->m_velocity.x * 1.5f + mist->m_position.x;
    mist->m_position.y = y + mist->m_position.y;
    mist->m_position.z = z + mist->m_position.z;
    float height = World::s_weather->m_grid->GetHeight(&mist->m_position);

    if (height < mist->m_position.z) {
        height = mist->m_position.z;
    }

    mist->m_position.z = height + this->m_size * 0.5f;
    mist->m_acceleration = (CRandom::ufloat_(g_rndSeed) - 0.5f) * 3.3333333f;
    float life = ((CRandom::ufloat_(g_rndSeed) - 0.5f) * 0.3f + 2.7f) * 1024.0f;
    mist->m_endTick = CMath::fint_(life - 0.5f) + mist->m_startTick;
}

// VERIFY: 58cf251e manual
// OFFSET: 0x00786E10
void Mists::Render() {
    CGxTex* texture = TextureGetGxTex(this->m_texture, 0, nullptr);

    if (!texture) {
        return;
    }

    DayNight::DNInfo* info = DayNight::GetInfo();
    float halfSize = this->m_size * 0.5f;
    C44Matrix view;
    GxXformView(view);
    float rightX = view.a0 * halfSize;
    float rightY = view.b0 * halfSize;
    float rightZ = view.c0 * halfSize;
    float upX = view.a1 * halfSize;
    float upY = view.b1 * halfSize;
    float upZ = halfSize * view.c1;
    C3Vector corners[4];
    corners[0] = C3Vector(-rightX + upX, -rightY + upY, -rightZ + upZ);
    corners[1] = C3Vector(upX + rightX, upY + rightY, upZ + rightZ);
    corners[2] = C3Vector(rightX - upX, rightY - upY, rightZ - upZ);
    corners[3] = C3Vector(-upX - rightX, -rightY - upY, -rightZ - upZ);
    uint32_t now = CWorld::s_elapsedTicks;
    C2Vector texCoords[4] = { C2Vector(0.0f, 0.0f), C2Vector(1.0f, 0.0f), C2Vector(1.0f, 1.0f), C2Vector(0.0f, 1.0f) };
    float tickSec = FrameTime::s_tickTimeSec;
    uint32_t prevTick = CWorld::s_elapsedTicks - FrameTime::s_tickTimeFixed;
    uint32_t tickFixed = FrameTime::s_tickTimeFixed;
    float tickSecSquared = FrameTime::s_tickTimeSec * FrameTime::s_tickTimeSec;
    this->m_spawnAccum = FrameTime::s_tickTimeSec * this->m_spawnRate + this->m_spawnAccum;

    if (static_cast<float>(this->m_mists.Count()) < this->m_spawnAccum) {
        this->m_spawnAccum = static_cast<float>(this->m_mists.Count());
    }

    CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPCT), 4 * this->m_mists.Count());
    CGxVertexPCT* vertices = reinterpret_cast<CGxVertexPCT*>(g_theGxDevicePtr->BufLock(vertexBuf));
    CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), 6 * this->m_mists.Count());
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
    uint16_t vertexCount = 0;

    for (uint32_t i = 0; i < this->m_mists.Count(); i++) {
        Mist* mist = &this->m_mists.m_data[i];

        if (mist->m_startTick && now > mist->m_endTick) {
            mist->m_startTick = 0;
        }

        if (this->m_fadeOut && now <= mist->m_startTick) {
            mist->m_startTick = 0;
        }

        if (!mist->m_startTick && 1.0f <= this->m_spawnAccum && !this->m_fadeOut) {
            this->m_spawnAccum = this->m_spawnAccum - 1.0f;
            float offset = CRandom::ufloat_(g_rndSeed) * tickFixed;
            this->CreateMist(mist, prevTick + CMath::fint_(offset - 0.5f));
        }

        if (!mist->m_startTick) {
            continue;
        }

        float life = (mist->m_endTick - mist->m_startTick) * 0.0009765625f;
        float acceleration = tickSecSquared * mist->m_acceleration * 0.5f;
        float moveY = mist->m_velocity.y * tickSec + acceleration;
        float moveZ = tickSec * mist->m_velocity.z + acceleration;
        mist->m_position.x = mist->m_velocity.x * tickSec + acceleration + mist->m_position.x;
        mist->m_position.y = moveY + mist->m_position.y;
        mist->m_position.z = moveZ + mist->m_position.z;
        float prevAge = prevTick <= mist->m_startTick ? 0.0f : (prevTick - mist->m_startTick) * 0.0009765625f;
        float invLife = 1.0f / life;
        float prevT = prevAge * invLife;
        float curT = 0.0009765625f * (invLife * (now - mist->m_startTick));

        if (curT >= 1.0f) {
            curT = 1.0f;
        }

        int32_t index0 = CMath::fint_(prevT * mist->m_heightCount - 0.5f);
        int32_t index1 = CMath::fint_(mist->m_heightCount * curT - 0.5f);

        if (index1 == index0) {
            index1 = index0 + 1;
        }

        if (index1 >= mist->m_heightCount - 1) {
            index1 = mist->m_heightCount - 1;
        }

        float height0 = mist->m_heights[index0];
        float height = height0 + (mist->m_heights[index1] - height0) * (mist->m_heightCount * prevT - index0) + this->m_size * 0.5f;

        if (height > mist->m_position.z) {
            mist->m_acceleration = mist->m_acceleration + 1.6666666f;
            float rise = height - mist->m_position.z;
            float limit = this->m_size * 0.25f;
            mist->m_position.z = limit <= rise ? limit + mist->m_position.z : rise + mist->m_position.z;
        }

        CImVector color = info->m_fog.m_color;
        float age = (now - mist->m_startTick) * 0.0009765625f;
        float fadeIn = this->m_fadeTime * (1.0f / this->m_fadeTime) + 1.0f / this->m_fadeTime * -1.0f * age;

        if (fadeIn < 0.0f) {
            fadeIn = 0.0f;
        } else if (fadeIn >= 1.0f) {
            fadeIn = 1.0f;
        }

        float invFade = 1.0f / (life - (life - this->m_fadeTime));
        float fadeOut = life * invFade + age * (-1.0f * invFade);

        if (fadeOut < 0.0f) {
            fadeOut = 0.0f;
        } else if (fadeOut >= 1.0f) {
            fadeOut = 1.0f;
        }

        float alpha = (1.0f - fadeIn) * fadeOut;

        for (uint32_t j = 0; j < 4; j++) {
            C3Vector position(corners[j].x + mist->m_position.x - CWorldScene::s_activeWorldView.x, corners[j].y + mist->m_position.y - CWorldScene::s_activeWorldView.y, mist->m_position.z + corners[j].z - CWorldScene::s_activeWorldView.z);
            float proximity = 1.5f - sqrt(position.y * position.y + position.z * position.z + position.x * position.x) * 0.083333336f;

            if (proximity < 0.0f) {
                proximity = 0.0f;
            } else if (proximity >= 1.0f) {
                proximity = 1.0f;
            }

            int32_t a = static_cast<int32_t>((1.0f - proximity) * alpha * 255.0f);
            vertices->p = position;
            color.a = static_cast<uint8_t>(a);

            if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
                CImVector swapped;
                swapped.b = color.r;
                swapped.g = color.g;
                swapped.r = color.b;
                swapped.a = static_cast<uint8_t>(a);
                vertices->c = swapped;
            } else {
                vertices->c = color;
            }

            vertices->tc = texCoords[j];
            vertices++;
        }

        indices[1] = vertexCount + 1;
        indices[0] = vertexCount;
        indices[3] = vertexCount;
        indices[2] = vertexCount + 2;
        indices[4] = vertexCount + 2;
        indices[5] = vertexCount + 3;
        vertexCount += 4;
        indices += 6;
    }

    GxBufUnlock(indexBuf, 0);
    GxBufUnlock(vertexBuf, 0);

    if (!vertexCount) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 2);
    GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_BlendingMode].m_value.m_data.i[0]]));
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Texture0, texture);
    GxPrimVertexPtr(vertexBuf, GxVBF_PCT);
    g_theGxDevicePtr->PrimIndexPtr(indexBuf);
    CGxBatch batch;
    batch.m_primType = GxPrim_Triangles;
    batch.m_start = 0;
    batch.m_count = (6 * vertexCount) >> 2;
    batch.m_minIndex = 0;
    batch.m_maxIndex = vertexCount - 1;
    g_theGxDevicePtr->Draw(&batch, 1);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: b91247de manual
// OFFSET: 0x0078C420
Mists::Mists(float size, const C3Vector& extents, const char* textureName, float angle, float angleVariance, float speed, float speedVariance, uint32_t count) {
    this->m_extents = extents;
    this->unused_0010.x = 0.0f;
    this->unused_0010.y = 0.0f;
    this->unused_0010.z = 0.0f;
    this->m_fadeOut = 0;
    this->m_size = size;
    this->m_texture = nullptr;
    this->m_spawnRate = 0.0f;
    this->m_angle = angle;
    this->m_angleVariance = angleVariance;
    this->m_spawnAccum = 0.0f;
    this->m_speed = speed;
    this->m_speedVariance = speedVariance;
    this->m_fadeTime = 0.4f;
    this->m_mists.SetCount(count);
    CStatus status;
    this->m_texture = TextureCreate(textureName, CGxTexFlags(GxTex_LinearMipLinear, 0, 0, 0, 0, 0, 1, 0, 0, 0), &status, 1);
}

// INLINED
Mists::~Mists() {
    HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
}
