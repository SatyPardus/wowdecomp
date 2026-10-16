#include "Sand.h"
#include <cmath>
#include <cstdint>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Gx/CGxDevice/CGxShader.h"
#include "Engine/Source/Gx/CGxVertexPC.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Vector.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "Engine/Source/Tempest/CRect.h"
#include "WoW/Source/WorldClient/CPortalView.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/MapWeather.h"

// STATIC: 0x00ADF22C
static float s_windLead = 1.75f;

const char* const SandGrainParticleBuffer::s_packetName = ".?AU?$Packet@UGrain@Sand@@$0BIAA@$00@@";

// VERIFY: 185d2837 auto
// OFFSET: 0x00783FE0
CAaBox Sand::GetBoundingBox() const {
    return this->m_bbox;
}

// OFFSET: 0x0078C3E0
void Sand::Render() {
    this->UpdatePackets();

    if (CWorldScene::s_unk_ADF570.m_maxViewDepth < 0.0f) {
        return;
    }

    if (this->m_useWeatherShaders) {
        this->RenderPacketsShader();
    } else {
        this->RenderPackets();
    }

    this->m_mists.Render();
}

// VERIFY: 4118ecdc manual
// OFFSET: 0x00784580
void Sand::SetStormIntensity(float intensity) {
    uint8_t useShaders = this->m_useWeatherShaders;
    float rate = useShaders ? 32000.0f : 6000.0f;
    this->m_maxBuildTime = 0.00016276042f * rate;
    this->m_particlesPerFrame = rate * s_weatherDensity * intensity;
    float mistRate = useShaders ? s_weatherDensity * 64.0f : s_weatherDensity * 32.0f;
    this->m_mists.m_spawnRate = intensity * mistRate;
}

// INLINED
SandGrainParticleBuffer::SandGrainParticleBuffer()
    : m_particleCount(6144) {
}

// VERIFY: 903fb1ef manual
// OFFSET: 0x00785EA0
void Sand::InitGrain(SandGrainParticle* grain, float time) {
    C3Vector size(this->m_bbox.t.x - this->m_bbox.b.x, this->m_bbox.t.y - this->m_bbox.b.y, this->m_bbox.t.z - this->m_bbox.b.z);
    float randomZ = CRandom::ufloat_(g_rndSeed);
    float randomY = CRandom::ufloat_(g_rndSeed);
    float randomX = CRandom::ufloat_(g_rndSeed);
    grain->m_position.x = (randomX * 0.15000001f + 0.85000002f) * size.x;
    grain->m_position.y = (randomY - 0.5f) * size.y;
    grain->m_position.z = (randomZ - 0.5f) * size.z;
    float angle = (CRandom::ufloat_(g_rndSeed) - 0.5f) * 0.34906587f - 1.5700001f;
    float c = cos(angle);
    float s = sin(angle);
    float speed = (CRandom::ufloat_(g_rndSeed) - 0.5f) * 0.66666669f + 18.666666f;
    float rise = CRandom::ufloat_(g_rndSeed);
    grain->m_velocity.x = s * speed;
    grain->m_velocity.y = speed * c;
    grain->m_velocity.z = (rise - 0.5f) * 0.16666667f + 0.83333331f;
    C44Matrix rotation = C44Matrix::RotationAroundZ(-World::s_weather->m_facing);
    grain->m_velocity = rotation * grain->m_velocity;
    grain->m_position = rotation * grain->m_position;
    C3Vector wind(World::s_weather->m_playerVelocityX, World::s_weather->m_playerVelocityY, World::s_weather->m_playerVelocityZ);
    grain->m_position.x = wind.x * s_windLead + grain->m_position.x;
    grain->m_position.y = wind.y * s_windLead + grain->m_position.y;
    grain->m_position.z = s_windLead * wind.z + grain->m_position.z;
    grain->m_position.x = grain->m_position.x + CWorldScene::s_activeWorldView.x;
    grain->m_position.y = grain->m_position.y + CWorldScene::s_activeWorldView.y;
    grain->m_position.z = grain->m_position.z + CWorldScene::s_activeWorldView.z;
    float distance = (CRandom::ufloat_(g_rndSeed) - 0.5f) * 0.30000001f + 3.2f;

    if (World::s_weather->m_grid->GetHeight(&grain->m_position) >= grain->m_position.z) {
        grain->m_startTime = 0.0f;
        grain->m_endTime = 0.0f;
        return;
    }

    C3Vector end(distance * grain->m_velocity.x + grain->m_position.x, grain->m_velocity.y * distance + grain->m_position.y, distance * grain->m_velocity.z + grain->m_position.z);
    C3Vector hit(0.0f, 0.0f, 0.0f);
    World::s_weather->m_grid->Raycast(&grain->m_position, &end, &hit);
    grain->m_startTime = time;
    float dz = grain->m_position.z - hit.z;
    float dy = grain->m_position.y - hit.y;
    float dx = grain->m_position.x - hit.x;
    grain->m_endTime = time + sqrt(dx * dx + dy * dy + dz * dz) / sqrt(grain->m_velocity.z * grain->m_velocity.z + grain->m_velocity.y * grain->m_velocity.y + grain->m_velocity.x * grain->m_velocity.x);
}

// VERIFY: 2f65fdf5 manual
// OFFSET: 0x00786210
void Sand::UpdatePacket(Packet<SandGrainParticleBuffer>* packet, float count) {
    float tickSec = FrameTime::s_tickTimeSec;
    float available = static_cast<float>(6144 - packet->m_currParticle);

    if (count <= available) {
        available = count;
    }

    int32_t spawn = CMath::fint_(available - 0.5f);

    for (int32_t i = 0; i < spawn; i++) {
        SandGrainParticle* grain = &packet->m_particlesBuffer.m_particles[packet->m_currParticle];
        this->InitGrain(grain, CRandom::ufloat_(g_rndSeed) * tickSec + packet->m_packetBuildTime);
        uint32_t endTick = CMath::fint_(grain->m_endTime * 1024.0f - 0.5f) + packet->m_packetStartTick;

        if (endTick > packet->m_packetEndTick) {
            packet->m_packetEndTick = endTick;
        }

        packet->m_currParticle++;
    }

    packet->m_packetBuildTime = packet->m_packetBuildTime + tickSec;
}

// VERIFY: f0ecffd5 manual
// OFFSET: 0x00786B10
int32_t Sand::CountActivePackets() {
    int32_t packets = 0;

    for (Packet<SandGrainParticleBuffer>* packet = this->m_sandGrainPacketList.m_activePackets.Head(); packet; packet = this->m_sandGrainPacketList.m_activePackets.Next(packet)) {
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

// VERIFY: 4b54c0ca manual
// OFFSET: 0x00788660
void Sand::UpdatePackets() {
    if (this->m_fadeOut && this->m_sandGrainPacketPtr) {
        this->m_sandGrainPacketList.m_activePacketsCount--;
        this->m_sandGrainPacketList.m_freePacketsCount++;
        this->m_sandGrainPacketList.m_freePackets.LinkToTail(this->m_sandGrainPacketPtr);
        this->m_sandGrainPacketPtr = nullptr;
    }

    uint32_t now = CWorld::s_elapsedTicks;
    Packet<SandGrainParticleBuffer>* packet = this->m_sandGrainPacketList.m_activePackets.Head();

    while (packet) {
        Packet<SandGrainParticleBuffer>* next = this->m_sandGrainPacketList.m_activePackets.Next(packet);

        if (now > packet->m_packetEndTick) {
            this->m_sandGrainPacketList.m_activePacketsCount--;
            this->m_sandGrainPacketList.m_freePacketsCount++;
            this->m_sandGrainPacketList.m_freePackets.LinkToTail(packet);
        } else if (this->m_fadeOut && World::s_weather->m_fogIntensity <= 0.25f && now <= packet->m_packetStartTick) {
            this->m_sandGrainPacketList.m_activePacketsCount--;
            this->m_sandGrainPacketList.m_freePacketsCount++;
            this->m_sandGrainPacketList.m_freePackets.LinkToTail(packet);
        } else if ((packet->m_packetEndTick - packet->m_packetStartTick) * 0.0009765625f >= 60.0f) {
            DebugLog("Sand packet lifetime was too long.  curTime: %g, baseTime: %g, lastTime: %g, buildTime: %g\n", now * 0.0009765625, packet->m_packetStartTick * 0.0009765625, packet->m_packetEndTick * 0.0009765625, packet->m_packetBuildTime);
            this->m_sandGrainPacketList.m_activePacketsCount--;
            this->m_sandGrainPacketList.m_freePacketsCount++;
            this->m_sandGrainPacketList.m_freePackets.LinkToTail(packet);
        } else if (!packet->m_vertexBufHandle->m_hasData || !packet->m_vertexBufHandle->m_valid) {
            RealizeParticlePacket(packet);
        }

        packet = next;
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

    if (!this->m_sandGrainPacketPtr) {
        this->m_sandGrainPacketPtr = this->m_sandGrainPacketList.Alloc();
        float buildTicks = 6144.0f / this->m_particlesPerFrame * 1024.0f;
        this->m_sandGrainPacketPtr->m_packetStartTick = now + CMath::fint_(buildTicks - 0.5f);
    }

    this->UpdatePacket(this->m_sandGrainPacketPtr, count);
    Packet<SandGrainParticleBuffer>* current = this->m_sandGrainPacketPtr;

    if (current->m_currParticle >= 6144 || this->m_maxBuildTime <= current->m_packetBuildTime || now > current->m_packetStartTick) {
        RealizeParticlePacket(this->m_sandGrainPacketPtr);
        this->m_sandGrainPacketPtr = nullptr;
    }
}

// VERIFY: 64b22fde manual
// OFFSET: 0x0078BAA0
Sand::~Sand() {
    if (this->m_shader) {
        g_theGxDevicePtr->ShaderDestroy(this->m_shader);
    }
}

// VERIFY: b7b22b4a manual
// OFFSET: 0x0078BB00
void Sand::RenderPackets() {
    if (!this->m_sandGrainPacketList.m_activePackets.Head()) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 2);
    GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_BlendingMode].m_value.m_data.i[0]]));
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    C3Vector camera = *World::GetCamPos();
    uint32_t now = CWorld::s_elapsedTicks;
    CImVector color = DayNight::GetInfo()->m_fog.m_color;

    for (Packet<SandGrainParticleBuffer>* packet = this->m_sandGrainPacketList.m_activePackets.Head(); packet; packet = this->m_sandGrainPacketList.m_activePackets.Next(packet)) {
        if (packet->m_packetStartTick >= now) {
            continue;
        }

        float age = (now - packet->m_packetStartTick) * 0.0009765625f;
        CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPC), packet->m_currParticle);
        CGxVertexPC* vertices = reinterpret_cast<CGxVertexPC*>(g_theGxDevicePtr->BufLock(vertexBuf));
        uint32_t count = 0;

        for (uint32_t i = 0; i < packet->m_currParticle; i++) {
            SandGrainParticle& grain = packet->m_particlesBuffer.m_particles[i];

            if (age < grain.m_startTime || age > grain.m_endTime) {
                continue;
            }

            float duration = grain.m_endTime - grain.m_startTime;
            float t = age - grain.m_startTime;

            if (t < 0.0f) {
                t = 0.0f;
            } else if (t >= duration) {
                t = duration;
            }

            float fadeIn = 1.0f - 5.0f * t;

            if (fadeIn < 0.0f) {
                fadeIn = 0.0f;
            } else if (fadeIn >= 1.0f) {
                fadeIn = 1.0f;
            }

            float invFade = 1.0f / (duration - (duration - 0.2f));
            float fadeOut = duration * invFade + t * (-1.0f * invFade);

            if (fadeOut < 0.0f) {
                fadeOut = 0.0f;
            } else if (fadeOut >= 1.0f) {
                fadeOut = 1.0f;
            }

            int32_t alpha = static_cast<int32_t>((1.0f - fadeIn) * fadeOut * 255.0f);
            color.a = static_cast<uint8_t>(alpha);
            vertices->p.x = grain.m_velocity.x * t + grain.m_position.x - camera.x;
            vertices->p.y = grain.m_velocity.y * t + grain.m_position.y - camera.y;
            vertices->p.z = grain.m_velocity.z * t + grain.m_position.z - camera.z;

            if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
                CImVector swapped;
                swapped.b = color.r;
                swapped.g = color.g;
                swapped.r = color.b;
                swapped.a = static_cast<uint8_t>(alpha);
                vertices->c = swapped;
            } else {
                vertices->c = color;
            }

            vertices++;
            count++;
        }

        GxBufUnlock(vertexBuf, 0);

        if (count) {
            GxPrimVertexPtr(vertexBuf, GxVBF_PC);
            CGxBatch batch;
            batch.m_primType = GxPrim_Points;
            batch.m_start = 0;
            batch.m_count = count;
            batch.m_minIndex = 0;
            batch.m_maxIndex = 0;
            g_theGxDevicePtr->Draw(&batch, 0);
        }
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: 7e3f120a manual
// OFFSET: 0x0078BEE0
void Sand::RenderPacketsShader() {
    if (!this->m_sandGrainPacketList.m_activePackets.Head()) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 2);
    GxRsSet(GxRs_AlphaRef, static_cast<int32_t>(CGxDevice::s_alphaRef[g_theGxDevicePtr->m_appRenderStates.m_data[GxRs_BlendingMode].m_value.m_data.i[0]]));
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_PointSprite, 1);
    GxRsSet(GxRs_PointScaleMin, 1.0f);
    float pointScale = 24.0f;

    if (g_theGxDevicePtr->Caps().m_pointScaleMax < 24.0f) {
        pointScale = g_theGxDevicePtr->Caps().m_pointScaleMax;
    }

    GxRsSet(GxRs_PointScaleMax, pointScale);
    GxRsSet(GxRs_VertexShader, this->m_shader);
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
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 3, camera, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 4, reinterpret_cast<const float*>(&viewProj), 4);
    CRect window;
    window.minY = 0.0f;
    window.minX = 0.0f;
    window.maxY = 0.0f;
    window.maxX = 0.0f;
    g_theGxDevicePtr->CapsWindowSize(window);
    float params[4] = { -0.050000001f, 1.5f, (window.maxY - window.minY) * 0.0024999999f, 0.0f };
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 9, params, 1);
    C4Vector fog(DayNight::GetInfo()->m_fog.m_color);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 0, reinterpret_cast<const float*>(&fog), 1);
    uint32_t now = CWorld::s_elapsedTicks;

    for (Packet<SandGrainParticleBuffer>* packet = this->m_sandGrainPacketList.m_activePackets.Head(); packet; packet = this->m_sandGrainPacketList.m_activePackets.Next(packet)) {
        if (packet->m_packetStartTick >= now) {
            continue;
        }

        float age[4] = { (now - packet->m_packetStartTick) * 0.0009765625f, 0.0f, 0.0f, 0.0f };
        g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 8, age, 1);
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

// VERIFY: cd8bb19f manual
// OFFSET: 0x0078CF20
Sand::Sand(float width, float height) : m_bbox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), m_mists(12.0f, C3Vector(44.0f, 44.0f, 25.0f), "textures\\Weather\\WeatherMistGrainy01.blp", -1.5700001f, 0.34906587f, 15.0f, 4.5f, 128) {
    this->m_useWeatherShaders = 1;
    this->m_fadeOut = 0;
    this->m_sandGrainPacketList.m_activePacketsCount = 0;
    this->m_sandGrainPacketList.m_freePacketsCount = 0;
    this->m_sandGrainPacketPtr = nullptr;
    this->m_shader = nullptr;
    this->SetStormIntensity(0.0f);
    float halfWidth = width * 0.5f;
    float halfHeight = 0.5f * height;
    this->m_bbox.b.x = -halfWidth;
    this->m_bbox.b.y = -halfWidth;
    this->m_bbox.b.z = -halfHeight;
    this->m_bbox.t.x = halfWidth;
    this->m_bbox.t.y = halfWidth;
    this->m_bbox.t.z = halfHeight;
    g_theGxDevicePtr->ShaderCreate(&this->m_shader, GxSh_Vertex, "Shaders\\Vertex", "sand", 1);

    if (!this->m_shader || !this->m_shader->Valid() || !g_theGxDevicePtr->Caps().m_pointParameters) {
        this->m_useWeatherShaders = 0;
    }

    if (!s_useShaders->m_intValue) {
        this->m_useWeatherShaders = 0;
    }
}
