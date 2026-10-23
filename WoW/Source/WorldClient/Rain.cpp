#include "Rain.h"
#include <cmath>
#include <cstdint>
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Gx/CGxDevice/CGxShader.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/CGxVertexPCT.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/Tempest/C33Matrix.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Vector.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Source/WorldClient/CPortalView.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/MapWeather.h"
#include "WoW/Source/WorldClient/World.h"
#include "Engine/Source/Gx/CGxVertexPT.h"
#include "Engine/Source/ShaderEffect/Types.h"

// STATIC: 0x00ADF220
static float s_maxWindTilt = 45.0f;

// STATIC: 0x00ADF224
static float s_maxWindSpeed = 30.0f;

// STATIC: 0x00ADF228
static float s_windLead = 1.75f;

const char* const RainParticleBuffer::s_packetName = ".?AU?$Packet@UDrop@Rain@@$0BIAA@$02@@";

const char* const RainPatterParticleBuffer::s_packetName = ".?AU?$Packet@UPatter@Rain@@$0BIAA@$02@@";

// VERIFY: f932ffe5 manual
// OFFSET: 0x007840B0
void Rain::SetStormIntensity(float intensity) {
    uint8_t useShaders = this->m_useWeatherShaders;
    this->m_intensity = intensity;
    float rate = useShaders ? 35000.0f : 6500.0f;
    this->m_particlesPerFrame = s_weatherDensity * rate * intensity;
    this->m_maxBuildTime = rate * 0.00016276042f;
    float mistRate = useShaders ? 38.0f : 18.0f;
    float excess = intensity - 0.5f;
    float scale = excess >= 0.0f ? s_weatherDensity * (excess + excess) : s_weatherDensity * (0.0f + 0.0f);
    this->m_mists.m_spawnRate = mistRate * scale;
}

// VERIFY: 1e7d808e manual
// OFFSET: 0x00784140
void __stdcall Rain::GetMotionBlurMatrix(C33Matrix* matrix) {
    C3Vector wind(World::s_weather->m_playerVelocityX, World::s_weather->m_playerVelocityY, World::s_weather->m_playerVelocityZ);
    float lengthSq = wind.y * wind.y + wind.x * wind.x;
    float tilt = sqrt(lengthSq) / s_maxWindSpeed;

    if (tilt < 0.0f) {
        tilt = 0.0f;
    } else if (tilt >= 1.0f) {
        tilt = 1.0f;
    }

    float angle = tilt * s_maxWindTilt;

    if (fabs(lengthSq) < 0.001f) {
        return;
    }

    float invLength = 1.0f / sqrt(lengthSq);
    float dirY = wind.y * invLength;
    float dirZ = invLength * 0.0f * 0.0f;
    float dirX = wind.x * invLength;
    C3Vector axis(dirY - dirZ, dirZ - dirX, dirX * 0.0f - 0.0f * dirY);
    float axisSq = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;

    if (fabs(axisSq) < 0.001f) {
        return;
    }

    float invAxis = 1.0f / sqrt(axisSq);
    axis.x *= invAxis;
    axis.y *= invAxis;
    axis.z *= invAxis;
    matrix->Rotate(angle * 0.017453292f, axis, true);
}

// VERIFY: 1f283a72 manual
// OFFSET: 0x00784270
float Rain::GetWindTiltFactor() {
    if (fabs(World::s_weather->m_playerVelocityZ * World::s_weather->m_playerVelocityZ + World::s_weather->m_playerVelocityY * World::s_weather->m_playerVelocityY + World::s_weather->m_playerVelocityX * World::s_weather->m_playerVelocityX) >= 0.001f) {
        return 0.0f;
    }

    return 2.0f;
}

// VERIFY: 79ce2133 auto
// OFFSET: 0x007842D0
void __stdcall Rain::RealizePacket(Packet<RainParticleBuffer>* packet) {
    RainDropVertex* vertices = reinterpret_cast<RainDropVertex*>(g_theGxDevicePtr->BufLock(packet->m_vertexBufHandle));

    for (uint32_t i = 0; i < packet->m_currParticle; i++) {
        RainDropParticle& drop = packet->m_particlesBuffer.m_particles[i];

        for (uint32_t j = 0; j < 3; j++) {
            vertices->p = drop.m_position;
            vertices->n = drop.m_velocity;
            CImVector color;
            color.b = 0;
            color.g = 0;
            color.r = 0;
            color.a = static_cast<uint8_t>(j);

            if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
                CImVector swapped;
                swapped.b = 0;
                swapped.g = 0;
                swapped.r = 0;
                swapped.a = static_cast<uint8_t>(j);
                vertices->c = swapped;
            } else {
                vertices->c = color;
            }

            vertices->tc.x = drop.m_startTime;
            vertices->tc.y = drop.m_impactTime;
            vertices++;
        }
    }

    GxBufUnlock(packet->m_vertexBufHandle, 0);
}

// VERIFY: 7ae1056b manual
// OFFSET: 0x007843E0
void __stdcall Rain::RealizePacket(Packet<RainPatterParticleBuffer>* packet) {
    CGxVertexPCT* vertices = reinterpret_cast<CGxVertexPCT*>(g_theGxDevicePtr->BufLock(packet->m_vertexBufHandle));

    for (uint32_t i = 0; i < packet->m_currParticle; i++) {
        RainPatterParticle& patter = packet->m_particlesBuffer.m_particles[i];

        for (uint32_t j = 0; j < 3; j++) {
            vertices->p = patter.m_position;
            CImVector color;
            color.b = 0;
            color.g = 0;
            color.r = 0;
            color.a = static_cast<uint8_t>(j);

            if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
                CImVector swapped;
                swapped.b = 0;
                swapped.g = 0;
                swapped.r = 0;
                swapped.a = static_cast<uint8_t>(j);
                vertices->c = swapped;
            } else {
                vertices->c = color;
            }

            vertices->tc.x = patter.m_startTime;
            vertices->tc.y = patter.m_endTime;
            vertices++;
        }
    }

    GxBufUnlock(packet->m_vertexBufHandle, 0);
}

// VERIFY: ed8fe804 manual
// OFFSET: 0x00785140
void Rain::InitDrop(RainDropParticle* drop, float time) {
    C3Vector size(this->m_bbox.t.x - this->m_bbox.b.x, this->m_bbox.t.y - this->m_bbox.b.y, this->m_bbox.t.z - this->m_bbox.b.z);
    float randomY = CRandom::ufloat_(g_rndSeed);
    C3Vector offset((CRandom::ufloat_(g_rndSeed) - 0.5f) * size.x, (randomY - 0.5f) * size.y, size.z * 0.0f);
    float randomAngle = CRandom::ufloat_(g_rndSeed);
    float angle = (randomAngle - 0.5f) * (this->m_intensity * 0.20943952f + 0.052359879f) - 1.5700001f;
    float c = cos(angle);
    float s = sin(angle);
    float baseSpeed = this->m_intensity * 9.4899998f + 0.0099999998f;
    float spread = (CRandom::ufloat_(g_rndSeed) - 0.5f) * this->m_intensity;
    float speed = spread + spread + baseSpeed;
    float fall = -28.0f - this->m_intensity * 4.0f;
    float randomFall = CRandom::ufloat_(g_rndSeed) * this->m_intensity;
    drop->m_velocity.x = s * speed;
    drop->m_velocity.y = speed * c;
    drop->m_velocity.z = randomFall * -2.0f + fall;
    float t = -(size.z * 0.5f / drop->m_velocity.z);
    drop->m_position.x = offset.x - drop->m_velocity.x * t;
    drop->m_position.y = offset.y - drop->m_velocity.y * t;
    drop->m_position.z = offset.z - t * drop->m_velocity.z;
    Weather* weather = World::s_weather;
    C3Vector axis(0.0f, 0.0f, 0.0f);
    CMath::sincos_(weather->m_facing - 1.5707964f, &axis.y, &axis.x);
    float tilt = weather->m_speed * 0.055555556f;

    if (tilt < 0.0f) {
        tilt = 0.0f;
    } else if (tilt >= 1.0f) {
        tilt = 1.0f;
    }

    C44Matrix rotation = C44Matrix::Rotation(tilt * 1.134464f, axis, false);
    drop->m_position = rotation * drop->m_position;
    C3Vector wind(World::s_weather->m_playerVelocityX, World::s_weather->m_playerVelocityY, World::s_weather->m_playerVelocityZ);
    drop->m_position.x = wind.x * s_windLead + drop->m_position.x;
    drop->m_position.y = wind.y * s_windLead + drop->m_position.y;
    drop->m_position.z = s_windLead * wind.z + drop->m_position.z;
    drop->m_position.x = drop->m_position.x + CWorldScene::s_activeWorldView.x;
    drop->m_position.y = drop->m_position.y + CWorldScene::s_activeWorldView.y;
    drop->m_position.z = drop->m_position.z + CWorldScene::s_activeWorldView.z;
    C3Vector end(drop->m_velocity.x * t + drop->m_position.x, drop->m_velocity.y * t + drop->m_position.y, drop->m_position.z + t * drop->m_velocity.z);

    if (World::s_weather->m_grid->GetHeight(&drop->m_position) < drop->m_position.z) {
        float scale = 2.0f;
        bool fast = World::s_weather->m_onTaxi || World::s_weather->m_transportVelocityZ * World::s_weather->m_transportVelocityZ + World::s_weather->m_transportVelocityY * World::s_weather->m_transportVelocityY + World::s_weather->m_transportVelocityX * World::s_weather->m_transportVelocityX > 2.0f;

        if (fast) {
            scale = 1.0f;
        }

        C3Vector farPoint(drop->m_velocity.x * t * scale + end.x, drop->m_velocity.y * t * scale + end.y, scale * (t * drop->m_velocity.z) + end.z);
        C3Vector hit(0.0f, 0.0f, 0.0f);

        if (fast || World::s_weather->m_grid->Raycast(&drop->m_position, &farPoint, &hit)) {
            if (fast) {
                hit = farPoint;
            }

            float invSpeed = 1.0f / sqrt(drop->m_velocity.z * drop->m_velocity.z + drop->m_velocity.y * drop->m_velocity.y + drop->m_velocity.x * drop->m_velocity.x);
            float dx = drop->m_position.x - hit.x;
            float dy = drop->m_position.y - hit.y;
            float dz = drop->m_position.z - hit.z;
            float distance = sqrt(dy * dy + dz * dz + dx * dx) * invSpeed;
            drop->m_position.x = hit.x - drop->m_velocity.x * distance;
            drop->m_position.y = hit.y - drop->m_velocity.y * distance;
            drop->m_position.z = hit.z - distance * drop->m_velocity.z;
            drop->m_startTime = time;
            float ex = drop->m_position.x - hit.x;
            float ey = drop->m_position.y - hit.y;
            float ez = drop->m_position.z - hit.z;
            drop->m_impactTime = invSpeed * sqrt(ez * ez + ex * ex + ey * ey) + time;
            return;
        }
    }

    drop->m_startTime = 0.0f;
    drop->m_impactTime = 0.0f;
}

// VERIFY: 46e19b2b manual
// OFFSET: 0x00785640
void Rain::UpdatePacket(Packet<RainParticleBuffer>* packet, Packet<RainPatterParticleBuffer>* patter, float count) {
    float tickSec = FrameTime::s_tickTimeSec;
    float available = static_cast<float>(6144 - packet->m_currParticle);

    if (count <= available) {
        available = count;
    }

    int32_t spawn = CMath::fint_(available - 0.5f);

    for (int32_t i = 0; i < spawn; i++) {
        RainDropParticle* drop = &packet->m_particlesBuffer.m_particles[packet->m_currParticle];
        this->InitDrop(drop, CRandom::ufloat_(g_rndSeed) * tickSec + packet->m_packetBuildTime);
        float life = (-2.0f / drop->m_velocity.z + drop->m_impactTime) * 1024.0f;
        uint32_t endTick = CMath::fint_(life - 0.5f) + packet->m_packetStartTick;

        if (endTick > packet->m_packetEndTick) {
            packet->m_packetEndTick = endTick;
        }

        if (drop->m_impactTime != drop->m_startTime && !World::s_weather->m_onTaxi && World::s_weather->m_transportVelocityZ * World::s_weather->m_transportVelocityZ + World::s_weather->m_transportVelocityY * World::s_weather->m_transportVelocityY + World::s_weather->m_transportVelocityX * World::s_weather->m_transportVelocityX <= 2.0f) {
            RainPatterParticle* splash = &patter->m_particlesBuffer.m_particles[patter->m_currParticle++];
            float dt = drop->m_impactTime - drop->m_startTime;
            splash->m_position.x = drop->m_velocity.x * dt + drop->m_position.x;
            splash->m_position.y = drop->m_velocity.y * dt + drop->m_position.y;
            splash->m_position.z = dt * drop->m_velocity.z + drop->m_position.z;
            splash->m_startTime = drop->m_impactTime;
            splash->m_endTime = drop->m_impactTime + 0.25f;
            uint32_t splashEnd = CMath::fint_(splash->m_endTime * 1024.0f - 0.5f) + patter->m_packetStartTick;

            if (splashEnd > patter->m_packetEndTick) {
                patter->m_packetEndTick = splashEnd;
            }
        }

        packet->m_currParticle++;
    }

    packet->m_packetBuildTime = packet->m_packetBuildTime + tickSec;
}

// VERIFY: 253ec7c3 manual
// OFFSET: 0x00786A90
int32_t Rain::CountActivePackets() {
    int32_t packets = 0;

    for (Packet<RainParticleBuffer>* packet = this->m_rainDropPacketList.m_activePackets.Head(); packet; packet = this->m_rainDropPacketList.m_activePackets.Next(packet)) {
        if (static_cast<uint32_t>(CWorld::s_elapsedTicks) < packet->m_packetEndTick) {
            packets++;
        }
    }

    int32_t mists = 0;

    for (uint32_t i = 0; i < this->m_mists.m_mists.Count(); i++) {
        Mists::Mist& mist = this->m_mists.m_mists.m_data[i];

        if (mist.m_startTick && static_cast<uint32_t>(CWorld::s_elapsedTicks) > mist.m_startTick && static_cast<uint32_t>(CWorld::s_elapsedTicks) <= mist.m_endTick) {
            mists++;
        }
    }

    return mists + packets;
}

// VERIFY: 35de1ab3 manual
// OFFSET: 0x00787CE0
void Rain::UpdatePackets() {
    if (this->m_fadeOut && this->m_rainDropPacketPtr) {
        this->m_rainDropPacketList.m_activePacketsCount--;
        this->m_rainDropPacketList.m_freePacketsCount++;
        this->m_rainDropPacketList.m_freePackets.LinkToTail(this->m_rainDropPacketPtr);
        this->m_rainDropPacketPtr = nullptr;
    }

    uint32_t now = CWorld::s_elapsedTicks;
    Packet<RainParticleBuffer>* packet = this->m_rainDropPacketList.m_activePackets.Head();

    while (packet) {
        Packet<RainParticleBuffer>* next = this->m_rainDropPacketList.m_activePackets.Next(packet);

        if (now > packet->m_packetEndTick) {
            this->m_rainDropPacketList.m_activePacketsCount--;
            this->m_rainDropPacketList.m_freePacketsCount++;
            this->m_rainDropPacketList.m_freePackets.LinkToTail(packet);
        } else if (this->m_fadeOut && World::s_weather->m_fogIntensity <= 0.25f && now <= packet->m_packetStartTick) {
            this->m_rainDropPacketList.m_activePacketsCount--;
            this->m_rainDropPacketList.m_freePacketsCount++;
            this->m_rainDropPacketList.m_freePackets.LinkToTail(packet);
        } else if ((packet->m_packetEndTick - packet->m_packetStartTick) * 0.0009765625f >= 60.0f) {
            DebugLog("Rain packet lifetime was too long.  curTime: %g, baseTime: %g, lastTime: %g, buildTime: %g\n", now * 0.0009765625, packet->m_packetStartTick * 0.0009765625, packet->m_packetEndTick * 0.0009765625, packet->m_packetBuildTime);
            this->m_rainDropPacketList.m_activePacketsCount--;
            this->m_rainDropPacketList.m_freePacketsCount++;
            this->m_rainDropPacketList.m_freePackets.LinkToTail(packet);
        } else if (!packet->m_vertexBufHandle->m_hasData || !packet->m_vertexBufHandle->m_valid) {
            Rain::RealizePacket(packet);
        }

        packet = next;
    }

    Packet<RainPatterParticleBuffer>* patter = this->m_rainPatterPacketList.m_activePackets.Head();

    while (patter) {
        Packet<RainPatterParticleBuffer>* next = this->m_rainPatterPacketList.m_activePackets.Next(patter);

        if (now > patter->m_packetEndTick) {
            this->m_rainPatterPacketList.m_activePacketsCount--;
            this->m_rainPatterPacketList.m_freePacketsCount++;
            this->m_rainPatterPacketList.m_freePackets.LinkToTail(patter);
        } else if (!patter->m_vertexBufHandle->m_hasData || !patter->m_vertexBufHandle->m_valid) {
            Rain::RealizePacket(patter);
        }

        patter = next;
    }

    if (this->m_fadeOut) {
        return;
    }

    float tick = 0.016666999f;

    if (FrameTime::s_tickTimeSec < 0.016666999f) {
        tick = FrameTime::s_tickTimeSec;
    }

    float count = tick * this->m_particlesPerFrame;

    if (count <= 1.0f) {
        return;
    }

    if (!this->m_rainDropPacketPtr) {
        this->m_rainDropPacketPtr = this->m_rainDropPacketList.Alloc();
        float buildTicks = 6144.0f / this->m_particlesPerFrame * 1024.0f;
        this->m_rainDropPacketPtr->m_packetStartTick = now + CMath::fint_(buildTicks - 0.5f);
        this->m_rainPatterPacketPtr = this->m_rainPatterPacketList.Alloc();
        this->m_rainPatterPacketPtr->m_packetStartTick = this->m_rainDropPacketPtr->m_packetStartTick;
    }

    this->UpdatePacket(this->m_rainDropPacketPtr, this->m_rainPatterPacketPtr, count);
    Packet<RainParticleBuffer>* current = this->m_rainDropPacketPtr;

    if (current->m_currParticle >= 6144 || this->m_maxBuildTime <= current->m_packetBuildTime || now > current->m_packetStartTick) {
        Rain::RealizePacket(this->m_rainDropPacketPtr);
        Packet<RainPatterParticleBuffer>* splash = this->m_rainPatterPacketPtr;
        this->m_rainDropPacketPtr = nullptr;
        Rain::RealizePacket(splash);
        this->m_rainPatterPacketPtr = nullptr;
    }
}

// VERIFY: e7a71254 manual
// OFFSET: 0x007891B0
void Rain::RenderPatterPacketsSW() {
    if (!this->m_rainPatterPacketList.m_activePackets.Head()) {
        return;
    }

    CGxTex* texture = TextureGetGxTex(this->m_rainDropSplashTex, 0, nullptr);

    if (!texture) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 5);
    GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_BlendingMode].m_value.m_data.i[0]]));
    GxRsSet(GxRs_ColorOp0, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp0, GxTexOp_Mod);
    GxRsSet(GxRs_MatDiffuse, *reinterpret_cast<int32_t*>(&World::s_weather->m_color));
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 1);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_FogStart, 70.0f);
    GxRsSet(GxRs_FogEnd, 75.0f);
    GxRsSet(GxRs_FogColor, static_cast<int32_t>(0x80808080));
    GxRsSet(GxRs_Texture0, texture);
    C44Matrix view;
    GxXformView(view);
    C3Vector right(view.a0 * 0.083333336f, view.b0 * 0.083333336f, view.c0 * 0.083333336f);
    C3Vector up(view.a1 * 0.16666667f, view.b1 * 0.16666667f, view.c1 * 0.16666667f);
    C3Vector left(-right.x, -right.y, -right.z);
    C3Vector camera = *World::GetCamPos();
    uint32_t now = CWorld::s_elapsedTicks;

    for (Packet<RainPatterParticleBuffer>* packet = this->m_rainPatterPacketList.m_activePackets.Head(); packet; packet = this->m_rainPatterPacketList.m_activePackets.Next(packet)) {
        if (packet->m_packetStartTick >= now) {
            continue;
        }

        float age = (now - packet->m_packetStartTick) * 0.0009765625f;
        CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPT), 3 * packet->m_currParticle);
        CGxVertexPT* vertices = reinterpret_cast<CGxVertexPT*>(g_theGxDevicePtr->BufLock(vertexBuf));
        uint32_t vertexCount = 0;

        for (uint32_t i = 0; i < packet->m_currParticle; i++) {
            const RainPatterParticle& particle = packet->m_particlesBuffer.m_particles[i];

            if (particle.m_startTime > age || particle.m_endTime < age) {
                continue;
            }

            C3Vector center = particle.m_position - camera;
            float scale = 1.0f / sqrt(center.x * center.x + center.z * center.z + center.y * center.y);
            C3Vector toCamera(-(center.x * scale), -(center.y * scale), -(center.z * scale));
            float facing = toCamera.x * 0.0f + toCamera.y * 0.0f + toCamera.z;

            if (facing <= 0.0f) {
                facing = 0.0f;
            }

            center.x = center.x - up.x * (0.5f * facing);
            center.y = center.y - up.y * (0.5f * facing);
            center.z = center.z - up.z * (0.5f * facing) + facing * 0.083333336f;
            float u = static_cast<float>(floor((age - particle.m_startTime) * 4.0f * 3.99f)) * 0.25f;
            float v = static_cast<float>(floor((1.0f - facing) * 3.99f)) * 0.25f;
            vertices[0].p = left + center;
            vertices[0].tc.x = u;
            vertices[0].tc.y = v + 0.25f;
            vertices[1].p = center + up;
            vertices[1].tc.x = u + 0.125f;
            vertices[1].tc.y = v + 0.04296875f;
            vertices[2].p = center + right;
            vertices[2].tc.x = u + 0.25f;
            vertices[2].tc.y = v + 0.25f;
            vertices += 3;
            vertexCount += 3;
        }

        g_theGxDevicePtr->BufUnlock(vertexBuf, 0);
        vertexBuf->m_hasData = 1;

        if (vertexCount) {
            GxPrimVertexPtr(vertexBuf, GxVBF_PT);
            CGxBatch batch;
            batch.m_primType = GxPrim_Triangles;
            batch.m_start = 0;
            batch.m_count = vertexCount;
            batch.m_minIndex = 0;
            batch.m_maxIndex = 0;
            g_theGxDevicePtr->Draw(&batch, 0);
        }
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: fc199a69 manual
// OFFSET: 0x007898A0
void Rain::RenderDropPacketsSW() {
    if (!this->m_rainDropPacketList.m_activePackets.Head()) {
        return;
    }

    CGxTex* texture = TextureGetGxTex(this->m_rainDropTex, 0, nullptr);

    if (!texture) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 5);
    GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_BlendingMode].m_value.m_data.i[0]]));
    GxRsSet(GxRs_ColorOp0, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp0, GxTexOp_Mod);
    GxRsSet(GxRs_MatDiffuse, *reinterpret_cast<int32_t*>(&World::s_weather->m_color));
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 1);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_FogStart, 70.0f);
    GxRsSet(GxRs_FogEnd, 75.0f);
    GxRsSet(GxRs_FogColor, static_cast<int32_t>(0x80808080));
    GxRsSet(GxRs_Texture0, texture);
    C3Vector camera = *World::GetCamPos();
    uint32_t now = CWorld::s_elapsedTicks;
    C33Matrix blur(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    Rain::GetMotionBlurMatrix(&blur);
    float tilt = Rain::GetWindTiltFactor();

    for (Packet<RainParticleBuffer>* packet = this->m_rainDropPacketList.m_activePackets.Head(); packet; packet = this->m_rainDropPacketList.m_activePackets.Next(packet)) {
        if (packet->m_packetStartTick >= now) {
            continue;
        }

        float age = (now - packet->m_packetStartTick) * 0.0009765625f;
        CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPT), 3 * packet->m_currParticle);
        CGxVertexPT* vertices = reinterpret_cast<CGxVertexPT*>(g_theGxDevicePtr->BufLock(vertexBuf));
        uint32_t vertexCount = 0;

        for (uint32_t i = 0; i < packet->m_currParticle; i++) {
            const RainDropParticle& particle = packet->m_particlesBuffer.m_particles[i];
            C2Vector tex0(0.0f, 1.0f);
            C2Vector tex1(1.0f, 1.0f);
            C2Vector tex2(0.5f, 0.0f);
            C3Vector offset = particle.m_position - camera;
            float tailTime = particle.m_impactTime - tilt / particle.m_velocity.z;

            if (age < particle.m_startTime || tailTime < age) {
                continue;
            }

            float headTime = (age >= particle.m_impactTime ? particle.m_impactTime : age) - particle.m_startTime;
            float trailTime = (tailTime <= age ? tailTime : age) - particle.m_startTime;
            C3Vector head = offset + particle.m_velocity * headTime;
            C3Vector tail = offset + particle.m_velocity * trailTime;
            C3Vector toCamera(-head.x, -head.y, -head.z);
            float scale = 1.0f / sqrt(toCamera.z * toCamera.z + toCamera.y * toCamera.y + toCamera.x * toCamera.x);
            toCamera.x *= scale;
            toCamera.y *= scale;
            toCamera.z *= scale;
            C3Vector direction(-particle.m_velocity.x, -particle.m_velocity.y, -particle.m_velocity.z);
            scale = 1.0f / sqrt(direction.z * direction.z + direction.y * direction.y + direction.x * direction.x);
            direction.x *= scale;
            direction.y *= scale;
            direction.z *= scale;
            C3Vector side(direction.z * toCamera.y - direction.y * toCamera.z, toCamera.z * direction.x - direction.z * toCamera.x, toCamera.x * direction.y - toCamera.y * direction.x);
            scale = 1.0f / sqrt(side.z * side.z + side.y * side.y + side.x * side.x);
            side.x *= scale;
            side.y *= scale;
            side.z *= scale;
            vertices[0].p = side * -0.050000001f + head;
            vertices[0].tc = tex0;
            vertices[1].p = head + side * 0.050000001f;
            vertices[1].tc = tex1;
            C3Vector streak = direction * 2.0f;
            vertices[2].p.x = blur.a0 * streak.x + blur.a1 * streak.y + blur.a2 * streak.z + tail.x;
            vertices[2].p.y = blur.b0 * streak.x + blur.b1 * streak.y + blur.b2 * streak.z + tail.y;
            vertices[2].p.z = blur.c0 * streak.x + blur.c1 * streak.y + blur.c2 * streak.z + tail.z;
            vertices[2].tc = tex2;
            vertices += 3;
            vertexCount += 3;
        }

        g_theGxDevicePtr->BufUnlock(vertexBuf, 0);
        vertexBuf->m_hasData = 1;

        if (vertexCount) {
            GxPrimVertexPtr(vertexBuf, GxVBF_PT);
            CGxBatch batch;
            batch.m_primType = GxPrim_Triangles;
            batch.m_start = 0;
            batch.m_count = vertexCount;
            batch.m_minIndex = 0;
            batch.m_maxIndex = 0;
            g_theGxDevicePtr->Draw(&batch, 0);
        }
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: b4ae734d manual
// OFFSET: 0x0078A030
void Rain::RenderPatterPackets() {
    if (!this->m_rainPatterPacketList.m_activePackets.Head()) {
        return;
    }

    CGxTex* texture = TextureGetGxTex(this->m_rainDropSplashTex, 0, nullptr);

    if (!texture) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 5);
    GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_BlendingMode].m_value.m_data.i[0]]));
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 1);
    GxRsSet(GxRs_Culling, 0);
    float fog[4] = { CShaderEffect::s_fogMul * -0.2f, 15.0f, 1.0f, 0.0f };
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 0, fog, 1);
    C4Vector color(World::s_weather->m_color);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 7, reinterpret_cast<const float*>(&color), 1);
    GxRsSet(GxRs_FogColor, static_cast<int32_t>(0x80808080));
    GxRsSet(GxRs_Texture0, texture);
    GxRsSet(GxRs_VertexShader, this->m_shaderPatter);
    C44Matrix view;
    GxXformView(view);
    C44Matrix projection;
    projection = g_theGxDevicePtr->m_projNative;

    if (g_theGxDevicePtr->DeviceApi() == GxApi_OpenGl) {
        projection.c0 *= -1.0f;
        projection.c1 *= -1.0f;
        projection.c2 *= -1.0f;
        projection.c3 = -1.0f * projection.c3;
    }

    float camera[4] = { CWorldScene::s_activeWorldView.x, CWorldScene::s_activeWorldView.y, CWorldScene::s_activeWorldView.z, 1.0f };
    C44Matrix viewProj = (view * projection).Transpose();
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 1, camera, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 2, reinterpret_cast<const float*>(&viewProj), 4);
    float rightX = view.a0 * 0.083333336f;
    float rightY = view.b0 * 0.083333336f;
    float rightZ = 0.083333336f * view.c0;
    float right[4] = { rightX, rightY, rightZ, 0.0f };
    float up[4] = { view.a1 * 0.16666667f, view.b1 * 0.16666667f, 0.16666667f * view.c1, 0.0f };
    float left[4] = { -rightX, -rightY, -rightZ, 0.0f };
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 8, left, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 9, up, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 10, right, 1);
    float texV[4] = { 0.0f, 0.25f, 0.0f, 0.0f };
    float anim[4] = { 0.125f, 0.04296875f, 0.0f, 0.0f };
    float texScale[4] = { 0.25f, 0.25f, 0.0f, 0.0f };
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 11, texV, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 12, anim, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 13, texScale, 1);
    uint32_t now = CWorld::s_elapsedTicks;

    for (Packet<RainPatterParticleBuffer>* packet = this->m_rainPatterPacketList.m_activePackets.Head(); packet; packet = this->m_rainPatterPacketList.m_activePackets.Next(packet)) {
        if (packet->m_packetStartTick >= now) {
            continue;
        }

        float age[4] = { (now - packet->m_packetStartTick) * 0.0009765625f, 0.0f, 0.0f, 0.0f };
        g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 6, age, 1);
        GxPrimVertexPtr(packet->m_vertexBufHandle, GxVBF_PCT);
        CGxBatch batch;
        batch.m_primType = GxPrim_Triangles;
        batch.m_start = 0;
        batch.m_count = 3 * packet->m_currParticle;
        batch.m_minIndex = 0;
        batch.m_maxIndex = 0;
        g_theGxDevicePtr->Draw(&batch, 0);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 81f86833 manual
// OFFSET: 0x0078A640
void Rain::RenderDropPackets() {
    if (!this->m_rainDropPacketList.m_activePackets.Head()) {
        return;
    }

    CGxTex* texture = TextureGetGxTex(this->m_rainDropTex, 0, nullptr);

    if (!texture) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 5);
    GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_BlendingMode].m_value.m_data.i[0]]));
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 1);
    GxRsSet(GxRs_Culling, 0);
    float fog[4] = { CShaderEffect::s_fogMul * -0.2f, 15.0f, 1.0f, 0.0f };
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 0, fog, 1);
    C4Vector color(World::s_weather->m_color);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 7, reinterpret_cast<const float*>(&color), 1);
    GxRsSet(GxRs_FogColor, static_cast<int32_t>(0x80808080));
    GxRsSet(GxRs_Texture0, texture);
    GxRsSet(GxRs_VertexShader, this->m_shaderRain);
    C44Matrix view;
    GxXformView(view);
    C44Matrix projection;
    projection = g_theGxDevicePtr->m_projNative;

    if (g_theGxDevicePtr->DeviceApi() == GxApi_OpenGl) {
        projection.c0 *= -1.0f;
        projection.c1 *= -1.0f;
        projection.c2 *= -1.0f;
        projection.c3 = -1.0f * projection.c3;
    }

    float camera[4] = { CWorldScene::s_activeWorldView.x, CWorldScene::s_activeWorldView.y, CWorldScene::s_activeWorldView.z, 1.0f };
    C44Matrix viewProj = (view * projection).Transpose();
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 1, camera, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 2, reinterpret_cast<const float*>(&viewProj), 4);
    C33Matrix blur(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    Rain::GetMotionBlurMatrix(&blur);
    float row0[4] = { blur.a0, blur.b0, blur.c0, 0.0f };
    float row1[4] = { blur.a1, blur.b1, blur.c1, 0.0f };
    float row2[4] = { blur.a2, blur.b2, blur.c2, 0.0f };
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 8, row0, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 9, row1, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 10, row2, 1);
    float tilt[4] = { Rain::GetWindTiltFactor(), 0.0f, 0.0f, 0.0f };
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 11, tilt, 1);
    float texV[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
    float texU[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
    float half[4] = { 0.5f, 0.0f, 0.0f, 0.0f };
    float widthLeft[4] = { -0.050000001f, -0.050000001f, -0.050000001f, 0.0f };
    float widthRight[4] = { 0.050000001f, 0.050000001f, 0.050000001f, 0.0f };
    float zero0[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float zero1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float zero2[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float length[4] = { 2.0f, 2.0f, 2.0f, 0.0f };
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 12, texV, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 13, texU, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 14, half, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 15, widthLeft, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 16, widthRight, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 17, zero0, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 18, zero1, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 19, zero2, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 20, length, 1);
    uint32_t now = CWorld::s_elapsedTicks;

    for (Packet<RainParticleBuffer>* packet = this->m_rainDropPacketList.m_activePackets.Head(); packet; packet = this->m_rainDropPacketList.m_activePackets.Next(packet)) {
        if (packet->m_packetStartTick >= now) {
            continue;
        }

        float age[4] = { (now - packet->m_packetStartTick) * 0.0009765625f, 0.0f, 0.0f, 0.0f };
        g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 6, age, 1);
        GxPrimVertexPtr(packet->m_vertexBufHandle, GxVBF_PNCT);
        CGxBatch batch;
        batch.m_primType = GxPrim_Triangles;
        batch.m_start = 0;
        batch.m_count = 3 * packet->m_currParticle;
        batch.m_minIndex = 0;
        batch.m_maxIndex = 0;
        g_theGxDevicePtr->Draw(&batch, 0);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 4b415121 auto
// OFFSET: 0x0078AE20
void Rain::Render() {
    this->UpdatePackets();

    if (CWorldScene::s_unk_ADF570.m_maxViewDepth < 0.0f) {
        return;
    }

    if (this->m_useWeatherShaders) {
        this->RenderDropPackets();
        this->RenderPatterPackets();
    } else {
        this->RenderDropPacketsSW();
        this->RenderPatterPacketsSW();
    }

    this->m_mists.Render();
}

// VERIFY: 42550704 manual
// OFFSET: 0x00789110
Rain::~Rain() {
    HandleClose(reinterpret_cast<HOBJECT>(this->m_rainDropTex));
    HandleClose(reinterpret_cast<HOBJECT>(this->m_rainDropSplashTex));

    if (this->m_shaderRain) {
        g_theGxDevicePtr->ShaderDestroy(this->m_shaderRain);
    }

    if (this->m_shaderPatter) {
        g_theGxDevicePtr->ShaderDestroy(this->m_shaderPatter);
    }
}

// VERIFY: 7c030b6b manual
// OFFSET: 0x0078CA90
Rain::Rain(float width, float height) : m_bbox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), m_mists(12.0f, C3Vector(44.0f, 44.0f, 25.0f), "textures\\Weather\\SnowMist01.blp", -1.5700001f, 0.34906587f, 5.0f, 1.2f, 128) {
    this->m_useWeatherShaders = 1;
    this->m_fadeOut = 0;
    this->m_rainDropPacketList.m_activePacketsCount = 0;
    this->m_rainDropPacketList.m_freePacketsCount = 0;
    this->m_rainPatterPacketList.m_activePacketsCount = 0;
    this->m_rainPatterPacketList.m_freePacketsCount = 0;
    this->m_rainDropPacketPtr = nullptr;
    this->m_rainPatterPacketPtr = nullptr;
    this->m_rainDropTex = nullptr;
    this->m_shaderRain = nullptr;
    this->m_rainDropSplashTex = nullptr;
    this->m_shaderPatter = nullptr;
    this->SetStormIntensity(0.0f);
    float halfWidth = width * 0.5f;
    float halfHeight = 0.5f * height;
    this->m_bbox.b.x = -halfWidth;
    this->m_bbox.b.y = -halfWidth;
    this->m_bbox.b.z = -halfHeight;
    this->m_bbox.t.x = halfWidth;
    this->m_bbox.t.y = halfWidth;
    this->m_bbox.t.z = halfHeight;
    g_theGxDevicePtr->ShaderCreate(&this->m_shaderRain, GxSh_Vertex, "Shaders\\Vertex", "rain", 1);
    CStatus status;
    this->m_rainDropTex = TextureCreate(World::s_weather->m_precipTexture, CGxTexFlags(GxTex_LinearMipLinear, 0, 0, 0, 0, 0, 1, 0, 0, 0), &status, 1);
    g_theGxDevicePtr->ShaderCreate(&this->m_shaderPatter, GxSh_Vertex, "Shaders\\Vertex", "patter", 1);
    this->m_rainDropSplashTex = TextureCreate("textures\\Weather\\RainDropSplash01.blp", CGxTexFlags(GxTex_LinearMipLinear, 0, 0, 0, 0, 0, 1, 0, 0, 0), &status, 1);

    if (!this->m_shaderRain || !this->m_shaderRain->Valid() || !this->m_shaderPatter || !this->m_shaderPatter->Valid()) {
        this->m_useWeatherShaders = 0;
    }

    if (!s_useShaders->m_intValue) {
        this->m_useWeatherShaders = 0;
    }
}

// VERIFY: 68a2d07d manual
// OFFSET: 0x007850C0
RainParticleBuffer::RainParticleBuffer()
    : m_particleCount(6144) {
}

// INLINED
RainPatterParticleBuffer::RainPatterParticleBuffer() {
    this->m_particleCount = 6144;

    for (uint32_t i = 0; i < 6144; i++) {
        this->m_particles[i].m_position.x = 0.0f;
        this->m_particles[i].m_position.y = 0.0f;
        this->m_particles[i].m_position.z = 0.0f;
        this->m_particles[i].unused_000C = 0.0f;
    }
}
