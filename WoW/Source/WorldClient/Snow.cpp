#include "MapWeather.h"
#include <cmath>
#include <cstdint>
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxShader.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Spline.h"
#include "Engine/Source/Tempest/C3Spline_CatmullRom.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Vector.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Source/Object/CMoveSpline.h"
#include "WoW/Source/Object/MovementShared.h"
#include "WoW/Source/Object/ObjectClient/Movement_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/WorldClient/CPortalView.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/World.h"

const char* const SnowFlakeParticleBuffer::s_packetName = ".?AU?$Packet@UFlake@Snow@@$0BIAA@$00@@";

// VERIFY: da6ee014 manual
// OFFSET: 0x00784010
C3Vector* Weather::GetPosition(C3Vector* position) {
    position->x = this->m_positionX;
    position->y = this->m_positionY;
    position->z = this->m_positionZ;
    return position;
}

// VERIFY: 735159a8 manual
// OFFSET: 0x00785100
SnowFlakeParticleBuffer::SnowFlakeParticleBuffer()
    : m_particleCount(6144) {
}

// VERIFY: d04403c6 manual
// OFFSET: 0x00785880
void Snow::InitFlake(SnowFlakeParticle* flake, float time, const C3Vector* origin, const C3Vector* wind) {
    C3Vector size(this->m_bbox.t.x - this->m_bbox.b.x, this->m_bbox.t.y - this->m_bbox.b.y, this->m_bbox.t.z - this->m_bbox.b.z);
    float randomY = CRandom::ufloat_(g_rndSeed);
    float randomX = CRandom::ufloat_(g_rndSeed);
    C3Vector offset((randomX - 0.5f) * size.x, (randomY - 0.5f) * size.y, size.z * 0.0f);
    float randomAngle = CRandom::ufloat_(g_rndSeed);
    float angle = (6.2831855f - this->m_intensity * 5.9341197f) * (randomAngle - 0.5f) - 1.5700001f;
    float c = cos(angle);
    float s = sin(angle);
    float baseSpeed = this->m_intensity * 5.9850001f + 0.015f;
    float speed = (CRandom::ufloat_(g_rndSeed) - 0.5f) * this->m_intensity + baseSpeed;
    float fall = -2.0f - this->m_intensity * 3.5f;
    float randomFall = CRandom::ufloat_(g_rndSeed) * this->m_intensity;
    flake->m_velocity.x = s * speed;
    flake->m_velocity.y = speed * c;
    flake->m_velocity.z = randomFall * -1.0f + fall;
    float t = -(size.z * 0.5f / flake->m_velocity.z);
    flake->m_position.x = offset.x - flake->m_velocity.x * t;
    flake->m_position.y = offset.y - flake->m_velocity.y * t;
    flake->m_position.z = offset.z - t * flake->m_velocity.z;
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
    flake->m_position = rotation * flake->m_position;
    flake->m_position.x = wind->x * 1.75f + flake->m_position.x;
    flake->m_position.y = wind->y * 1.75f + flake->m_position.y;
    flake->m_position.z = 1.75f * wind->z + flake->m_position.z;
    flake->m_position.x = flake->m_position.x + origin->x;
    flake->m_position.y = flake->m_position.y + origin->y;
    flake->m_position.z = flake->m_position.z + origin->z;
    C3Vector end(flake->m_velocity.x * t + flake->m_position.x, flake->m_velocity.y * t + flake->m_position.y, flake->m_position.z + t * flake->m_velocity.z);

    if (World::s_weather->m_grid->GetHeight(&flake->m_position) < flake->m_position.z) {
        float scale = 2.0f;
        bool fast = World::s_weather->m_onTaxi || World::s_weather->m_transportVelocityZ * World::s_weather->m_transportVelocityZ + World::s_weather->m_transportVelocityY * World::s_weather->m_transportVelocityY + World::s_weather->m_transportVelocityX * World::s_weather->m_transportVelocityX > 2.0f;

        if (fast) {
            scale = 1.0f;
        }

        C3Vector farPoint(flake->m_velocity.x * t * scale + end.x, flake->m_velocity.y * t * scale + end.y, scale * (t * flake->m_velocity.z) + end.z);
        C3Vector hit(0.0f, 0.0f, 0.0f);

        if (fast || World::s_weather->m_grid->Raycast(&flake->m_position, &farPoint, &hit)) {
            if (fast) {
                hit = farPoint;
            }

            float invSpeed = 1.0f / sqrt(flake->m_velocity.z * flake->m_velocity.z + flake->m_velocity.y * flake->m_velocity.y + flake->m_velocity.x * flake->m_velocity.x);
            float dx = flake->m_position.x - hit.x;
            float dy = flake->m_position.y - hit.y;
            float dz = flake->m_position.z - hit.z;
            float distance = sqrt(dx * dx + dy * dy + dz * dz) * invSpeed;
            flake->m_position.x = hit.x - flake->m_velocity.x * distance;
            flake->m_position.y = hit.y - flake->m_velocity.y * distance;
            flake->m_position.z = hit.z - distance * flake->m_velocity.z;
            flake->m_startTime = time;
            float ex = flake->m_position.x - hit.x;
            float ey = flake->m_position.y - hit.y;
            float ez = flake->m_position.z - hit.z;
            flake->m_endTime = invSpeed * sqrt(ey * ey + ez * ez + ex * ex) + time;
            return;
        }
    }

    flake->m_startTime = 0.0f;
    flake->m_endTime = -0.25f;
}

// VERIFY: 5d175e0a manual
// OFFSET: 0x00785D60
void Snow::UpdatePacket(Packet<SnowFlakeParticleBuffer>* packet, float count, const C3Vector* origin, const C3Vector* wind) {
    float tickSec = FrameTime::s_tickTimeSec;
    float available = static_cast<float>(6144 - packet->m_currParticle);

    if (count <= available) {
        available = count;
    }

    int32_t spawn = CMath::fint_(available - 0.5f);

    for (int32_t i = 0; i < spawn; i++) {
        SnowFlakeParticle* flake = &packet->m_particlesBuffer.m_particles[packet->m_currParticle];
        this->InitFlake(flake, CRandom::ufloat_(g_rndSeed) * tickSec + packet->m_packetBuildTime, origin, wind);
        float end = 0.0f;

        if (flake->m_endTime > 0.0f) {
            end = flake->m_endTime;
        }

        float life = (end + 0.25f) * 1024.0f;
        uint32_t endTick = CMath::fint_(life - 0.5f) + packet->m_packetStartTick;

        if (endTick > packet->m_packetEndTick) {
            packet->m_packetEndTick = endTick;
        }

        packet->m_currParticle++;
    }

    packet->m_packetBuildTime = packet->m_packetBuildTime + tickSec;
}

// VERIFY: 2ea6bffd manual
// OFFSET: 0x00786B10
int32_t Snow::CountActivePackets() {
    int32_t packets = 0;

    for (Packet<SnowFlakeParticleBuffer>* packet = this->m_snowFlakePacketList.m_activePackets.Head(); packet; packet = this->m_snowFlakePacketList.m_activePackets.Next(packet)) {
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

// VERIFY: 7169f077 manual
// OFFSET: 0x00788090
void Snow::UpdatePackets() {
    if (this->m_fadeOut && this->m_snowFlakePacketPtr) {
        this->m_snowFlakePacketList.m_activePacketsCount--;
        this->m_snowFlakePacketList.m_freePacketsCount++;
        this->m_snowFlakePacketList.m_freePackets.LinkToTail(this->m_snowFlakePacketPtr);
        this->m_snowFlakePacketPtr = nullptr;
    }

    uint32_t now = CWorld::s_elapsedTicks;
    Packet<SnowFlakeParticleBuffer>* packet = this->m_snowFlakePacketList.m_activePackets.Head();

    while (packet) {
        Packet<SnowFlakeParticleBuffer>* next = this->m_snowFlakePacketList.m_activePackets.Next(packet);

        if (now > packet->m_packetEndTick) {
            this->m_snowFlakePacketList.m_activePacketsCount--;
            this->m_snowFlakePacketList.m_freePacketsCount++;
            this->m_snowFlakePacketList.m_freePackets.LinkToTail(packet);
            packet = next;
            continue;
        }

        bool expired = this->m_fadeOut && World::s_weather->m_fogIntensity <= 0.25f && now <= packet->m_packetStartTick;

        if (!expired) {
            C3Vector* camera = World::GetCamPos();
            float dz = packet->m_origin.z - camera->z;
            float dy = packet->m_origin.y - camera->y;
            float dx = packet->m_origin.x - camera->x;
            expired = sqrt(dx * dx + dy * dy + dz * dz) > 200.0f;
        }

        if (expired) {
            this->m_snowFlakePacketList.m_activePacketsCount--;
            this->m_snowFlakePacketList.m_freePacketsCount++;
            this->m_snowFlakePacketList.m_freePackets.LinkToTail(packet);
        } else if ((packet->m_packetEndTick - packet->m_packetStartTick) * 0.0009765625f >= 60.0f) {
            DebugLog("Snow packet lifetime was too long.  curTime: %g, baseTime: %g, lastTime: %g, buildTime: %g\n", now * 0.0009765625, packet->m_packetStartTick * 0.0009765625, packet->m_packetEndTick * 0.0009765625, packet->m_packetBuildTime);
            this->m_snowFlakePacketList.m_activePacketsCount--;
            this->m_snowFlakePacketList.m_freePacketsCount++;
            this->m_snowFlakePacketList.m_freePackets.LinkToTail(packet);
        } else if (!packet->m_vertexBufHandle->m_hasData || !packet->m_vertexBufHandle->m_valid) {
            RealizeParticlePacket(packet);
        }

        packet = next;
    }

    if (this->m_fadeOut) {
        return;
    }

    C3Vector origin = CWorldScene::s_activeWorldView;
    C3Vector wind(World::s_weather->m_playerVelocityX, World::s_weather->m_playerVelocityY, World::s_weather->m_playerVelocityZ);

    if (World::s_weather->m_onTaxi) {
        WOWGUID active = ClntObjMgrGetActivePlayer();

        if (active) {
            CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(active, TYPE_PLAYER, __FILE__, __LINE__));

            if (player && player->m_movement->m_spline && !(player->m_movement->m_spline->m_flags & 0x400)) {
                C3Spline_CatmullRom* spline = player->GetSpline();

                if (spline && spline->m_pointCount > 2) {
                    float elapsed = player->GetSplineElapsedTime();
                    float duration = player->GetSplineDuration();

                    if (duration > 0.0f) {
                        Weather* weather = World::s_weather;
                        float speed = weather->m_speed;
                        float lead;

                        if (fequalz(speed, 0.0f, 0.001f)) {
                            lead = 0.0f;
                        } else {
                            C3Vector position;
                            C3Vector delta = CWorldScene::s_activeWorldView - *weather->GetPosition(&position);
                            lead = sqrt(delta.z * delta.z + delta.y * delta.y + delta.x * delta.x) / speed;
                        }

                        C3Vector velocity(0.0f, 0.0f, 0.0f);
                        float t = fclamp((elapsed - lead + 2350.0f) / duration, 0.0f, 1.0f);
                        spline->Vel(t, velocity, C3Spline::EVAL_ARCLENGTH);
                        spline->Vel(1.0f, velocity, C3Spline::EVAL_ARCLENGTH);
                        spline->Vel(0.0f, velocity, C3Spline::EVAL_ARCLENGTH);
                        spline->Vel(2.0f, velocity, C3Spline::EVAL_ARCLENGTH);
                        velocity.NormalizeSafe();
                        C3Vector scaled(velocity.x * speed, velocity.y * speed, speed * velocity.z);
                        velocity = scaled;

                        if (scaled.z < -10.0f) {
                            static float s_low = CMath::exp_(1.0f);
                            static float s_high = CMath::exp_(2.25f);
                            float fallScale = static_cast<float>(CMath::exp_(velocity.z * -0.1f));
                            float factor = fclamp((fallScale - s_low) / (s_high - s_low), 0.0f, 1.0f);
                            origin.z = origin.z - factor * (this->m_bbox.t.z - this->m_bbox.b.z);
                        }
                    }
                }
            }
        }
    }

    float tick = 0.016666999f;

    if (FrameTime::s_tickTimeSec < 0.016666999f) {
        tick = FrameTime::s_tickTimeSec;
    }

    float count = tick * this->m_particlesPerFrame;

    if (count <= 1.0f) {
        return;
    }

    if (!this->m_snowFlakePacketPtr) {
        this->m_snowFlakePacketPtr = this->m_snowFlakePacketList.Alloc();
        float buildTicks = 6144.0f / this->m_particlesPerFrame * 1024.0f;
        this->m_snowFlakePacketPtr->m_packetStartTick = now + CMath::fint_(buildTicks - 0.5f);
        this->m_snowFlakePacketPtr->m_origin = origin;
    }

    this->UpdatePacket(this->m_snowFlakePacketPtr, count, &origin, &wind);
    Packet<SnowFlakeParticleBuffer>* current = this->m_snowFlakePacketPtr;

    if (current->m_currParticle >= 6144 || this->m_maxBuildTime <= current->m_packetBuildTime || now > current->m_packetStartTick) {
        RealizeParticlePacket(this->m_snowFlakePacketPtr);
        this->m_snowFlakePacketPtr = nullptr;
    }
}

// VERIFY: 380fceb3 manual
// OFFSET: 0x0078AE70
Snow::~Snow() {
    HandleClose(reinterpret_cast<HOBJECT>(this->m_snowFlakeTex));

    if (this->m_shaderPtr) {
        g_theGxDevicePtr->ShaderDestroy(this->m_shaderPtr);
    }
}

// VERIFY: eb52147a manual
// OFFSET: 0x0078AEE0
void Snow::RenderPackets() {
    if (!this->m_snowFlakePacketList.m_activePackets.Head()) {
        return;
    }

    CGxTex* texture = TextureGetGxTex(this->m_snowFlakeTex, 0, nullptr);

    if (!texture) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 2);
    GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_BlendingMode].m_value.m_data.i[0]]));
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    float pointScale = 24.0f;

    if (g_theGxDevicePtr->Caps().m_pointScaleMax < 24.0f) {
        pointScale = g_theGxDevicePtr->Caps().m_pointScaleMax;
    }

    GxRsSet(GxRs_PointScaleMax, pointScale);
    GxRsSet(GxRs_PointSprite, 1);
    GxRsSet(GxRs_Texture0, texture);
    C4Vector color(World::s_weather->m_color);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 8, reinterpret_cast<const float*>(&color), 1);
    GxRsSet(GxRs_VertexShader, this->m_shaderPtr);
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
    float params[4] = { -0.02f, 1.0f, 14.0f, 0.0f };
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 7, params, 1);
    uint32_t now = CWorld::s_elapsedTicks;

    for (Packet<SnowFlakeParticleBuffer>* packet = this->m_snowFlakePacketList.m_activePackets.Head(); packet; packet = this->m_snowFlakePacketList.m_activePackets.Next(packet)) {
        if (packet->m_packetStartTick >= now) {
            continue;
        }

        float age[4] = { (now - packet->m_packetStartTick) * 0.0009765625f, 0.0f, 0.0f, 0.0f };
        g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 6, age, 1);
        GxPrimVertexPtr(packet->m_vertexBufHandle, GxVBF_PNT);
        CGxBatch batch;
        batch.m_primType = GxPrim_Points;
        batch.m_start = 0;
        batch.m_count = packet->m_currParticle;
        batch.m_minIndex = 0;
        batch.m_maxIndex = 0;
        g_theGxDevicePtr->Draw(&batch, 0);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 48bcd365 manual
// OFFSET: 0x0078B370
void Snow::RenderPacketsSW() {
    if (!this->m_snowFlakePacketList.m_activePackets.Head()) {
        return;
    }

    CGxTex* texture = TextureGetGxTex(this->m_snowFlakeTex, 0, nullptr);

    if (!texture) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 2);
    GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_BlendingMode].m_value.m_data.i[0]]));
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Fog, 0);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, texture);

    C44Matrix view;
    GxXformView(view);
    float upX = view.a1;
    float upY = view.b1;
    float upZ = view.c1;
    C3Vector corners[3];
    C3Vector right(view.a0 - upX, view.b0 - upY, view.c0 - upZ);
    C3Vector left(-view.a0 - upX, -view.b0 - upY, -view.c0 - upZ);
    float upScale = 1.0f / sqrtf(upZ * upZ + upY * upY + upX * upX);
    float rightScale = 1.0f / sqrtf(right.y * right.y + right.x * right.x + right.z * right.z);
    float leftScale = 1.0f / sqrtf(left.x * left.x + left.z * left.z + left.y * left.y);
    corners[0].x = upX * upScale * 0.083333336f;
    corners[0].y = upY * upScale * 0.083333336f;
    corners[0].z = upZ * upScale * 0.083333336f;
    corners[1].x = right.x * rightScale * 0.083333336f;
    corners[1].y = right.y * rightScale * 0.083333336f;
    corners[1].z = right.z * rightScale * 0.083333336f;
    corners[2].x = left.x * leftScale * 0.083333336f;
    corners[2].y = left.y * leftScale * 0.083333336f;
    corners[2].z = 0.083333336f * (leftScale * left.z);

    C3Vector camera = *World::GetCamPos();
    uint32_t now = CWorld::s_elapsedTicks;
    CImVector color = World::s_weather->m_color;
    GxFormatColor(color);

    for (Packet<SnowFlakeParticleBuffer>* packet = this->m_snowFlakePacketList.m_activePackets.Head(); packet; packet = this->m_snowFlakePacketList.m_activePackets.Next(packet)) {
        if (packet->m_packetStartTick >= now) {
            continue;
        }

        float age = (now - packet->m_packetStartTick) * 0.0009765625f;
        CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, 24, 3 * packet->m_currParticle);
        uint8_t* vertices = reinterpret_cast<uint8_t*>(g_theGxDevicePtr->BufLock(buf));
        uint32_t vertexCount = 0;

        for (uint32_t i = 0; i < packet->m_currParticle; i++) {
            static C2Vector s_texCoords[3] = { C2Vector(0.5f, -0.75f), C2Vector(-0.5f, 1.5f), C2Vector(1.5f, 1.5f) };
            const SnowFlakeParticle& flake = packet->m_particlesBuffer.m_particles[i];

            if (age < flake.m_startTime || flake.m_endTime + 0.25f < age) {
                continue;
            }

            float elapsed = (age >= flake.m_endTime ? flake.m_endTime : age) - flake.m_startTime;
            float faded = age - flake.m_endTime;
            float fadeTime = faded >= 0.0f ? faded : 0.0f;
            C3Vector position(flake.m_velocity.x * elapsed + flake.m_position.x, flake.m_velocity.y * elapsed + flake.m_position.y, flake.m_velocity.z * elapsed + flake.m_position.z);
            float fade = 1.0f - fadeTime * 4.0f;
            float fadeAlpha = 0.0f;

            if (fade >= 0.0f) {
                fadeAlpha = fade >= 1.0f ? 1.0f : fade;
            }

            float alpha = elapsed <= 1.0f ? elapsed : 1.0f;

            if (fadeAlpha < 1.0f) {
                alpha = fadeAlpha;
            }

            color.a = static_cast<uint8_t>(static_cast<int32_t>(alpha * 255.0f));
            C3Vector relative(position.x - camera.x, position.y - camera.y, position.z - camera.z);

            for (uint32_t j = 0; j < 3; j++) {
                C3Vector* p = reinterpret_cast<C3Vector*>(vertices);
                p->x = relative.x + corners[j].x;
                p->y = relative.y + corners[j].y;
                p->z = relative.z + corners[j].z;
                *reinterpret_cast<CImVector*>(vertices + 12) = color;
                *reinterpret_cast<C2Vector*>(vertices + 16) = s_texCoords[j];
                vertices += 24;
            }

            vertexCount += 3;
        }

        g_theGxDevicePtr->BufUnlock(buf, 0);
        buf->m_hasData = 1;

        if (vertexCount) {
            GxPrimVertexPtr(buf, GxVBF_PCT);
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

// VERIFY: d0ca6942 auto
// OFFSET: 0x0078BA60
void Snow::Render() {
    this->UpdatePackets();

    if (CWorldScene::s_unk_ADF570.m_maxViewDepth < 0.0f) {
        return;
    }

    if (this->m_useWeatherShaders) {
        this->RenderPackets();
    } else {
        this->RenderPacketsSW();
    }

    this->m_mists.Render();
}

// VERIFY: 97f25e33 manual
// OFFSET: 0x0078CD30
Snow::Snow(float width, float height) : m_bbox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), m_mists(12.0f, C3Vector(44.0f, 44.0f, 25.0f), "textures\\Weather\\SnowMist01.blp", -1.5700001f, 0.34906587f, 9.0f, 3.0f, 128) {
    this->m_useWeatherShaders = 1;
    this->m_fadeOut = 0;
    this->m_snowFlakePacketList.m_activePacketsCount = 0;
    this->m_snowFlakePacketList.m_freePacketsCount = 0;
    this->m_snowFlakePacketPtr = nullptr;
    this->m_snowFlakeTex = nullptr;
    this->m_shaderPtr = nullptr;
    this->SetStormIntensity(0.0f);
    float halfWidth = width * 0.5f;
    float halfHeight = 0.5f * height;
    this->m_bbox.b.x = -halfWidth;
    this->m_bbox.b.y = -halfWidth;
    this->m_bbox.b.z = -halfHeight;
    this->m_bbox.t.x = halfWidth;
    this->m_bbox.t.y = halfWidth;
    this->m_bbox.t.z = halfHeight;
    g_theGxDevicePtr->ShaderCreate(&this->m_shaderPtr, GxSh_Vertex, "Shaders\\Vertex", "snowpoint", 1);
    CStatus status;
    this->m_snowFlakeTex = TextureCreate(World::s_weather->m_precipTexture, CGxTexFlags(GxTex_LinearMipLinear, 0, 0, 0, 0, 0, 1, 0, 0, 0), &status, 1);

    if (!this->m_shaderPtr || !this->m_shaderPtr->Valid() || !g_theGxDevicePtr->Caps().m_pointSprite) {
        this->m_useWeatherShaders = 0;
    }

    if (!s_useShaders->m_intValue) {
        this->m_useWeatherShaders = 0;
    }
}
