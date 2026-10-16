#ifndef WOW_SOURCE_WORLDCLIENT_RAIN_H
#define WOW_SOURCE_WORLDCLIENT_RAIN_H

#include <cstddef>
#include <cstdint>
#include <new>
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "WoW/Source/WorldClient/Mists.h"

class C33Matrix;
class CGxShader;
class CTexture;
struct CGxPool;
struct CGxBuf;

struct RainDropParticle {
    /* 0000 */ C3Vector m_position;
    /* 000C */ C3Vector m_velocity;
    /* 0018 */ CImVector m_color;
    /* 001C */ float m_startTime;
    /* 0020 */ float m_impactTime;
};

struct RainParticleBuffer {
    static const char* const s_packetName;
    static const uint32_t s_vertexSize = 36;
    static const uint32_t s_verticesPerParticle = 3;

    /* 0000 */ uint32_t m_particleCount;
    /* 0004 */ RainDropParticle m_particles [6144];

    RainParticleBuffer();
};

struct RainPatterParticle {
    /* 0000 */ C3Vector m_position;
    /* 000C */ float unused_000C;
    /* 0010 */ float m_startTime;
    /* 0014 */ float m_endTime;
};

struct RainPatterParticleBuffer {
    static const char* const s_packetName;
    static const uint32_t s_vertexSize = 24;
    static const uint32_t s_verticesPerParticle = 3;

    /* 0000 */ uint32_t m_particleCount;
    /* 0004 */ RainPatterParticle m_particles [6144];

    RainPatterParticleBuffer();
};

struct RainDropVertex {
    /* 0000 */ C3Vector p;
    /* 000C */ C3Vector n;
    /* 0018 */ CImVector c;
    /* 001C */ C2Vector tc;
};

template <class T>
struct Packet : public TSLinkedNode<Packet<T>> {
    /* 0008 */ T m_particlesBuffer;
    /* 3600C */ uint32_t m_currParticle;
    /* 36010 */ uint32_t m_packetStartTick;
    /* 36014 */ uint32_t m_packetEndTick;
    /* 36018 */ float m_packetBuildTime;
    /* 3601C */ C3Vector m_origin;
    /* 36028 */ CGxPool* m_gxPool;
    /* 3602C */ CGxBuf* m_vertexBufHandle;

    Packet();
    ~Packet();
};

template <class T>
struct PacketList {
    /* 0000 */ TSList<T, TSGetLink<T>> m_freePackets;
    /* 000C */ int32_t m_activePacketsCount;
    /* 0010 */ int32_t m_freePacketsCount;
    /* 0014 */ TSList<T, TSGetLink<T>> m_activePackets;

    ~PacketList();
    T* Alloc();
};

// VERIFY: 3d18abbb auto
// OFFSET: [0x00786920, 0x00786990, 0x00786A20]
template <class T>
Packet<T>::Packet() {
    this->m_origin.x = 0.0f;
    this->m_origin.y = 0.0f;
    this->m_origin.z = 0.0f;
    this->m_gxPool = g_theGxDevicePtr->PoolCreate(GxPoolTarget_Vertex, GxPoolUsage_Dynamic, T::s_vertexSize * 6144 * T::s_verticesPerParticle, GxPoolHintBit_Unk2, "WeatherPacket_vtx");
    this->m_vertexBufHandle = g_theGxDevicePtr->BufCreate(this->m_gxPool, T::s_vertexSize, 6144 * T::s_verticesPerParticle, 0);
}

// VERIFY: f5042ffb auto
// OFFSET: [0x00786C30, 0x00786CD0, 0x00786D70]
template <class T>
Packet<T>::~Packet() {
    CGxBuf* buf = this->m_vertexBufHandle;

    if (buf && buf != g_theGxDevicePtr->BufStream(buf->m_pool->m_target, 0, 0)) {
        g_theGxDevicePtr->BufDestroy(buf);
    }

    if (this->m_gxPool) {
        g_theGxDevicePtr->PoolDestroy(this->m_gxPool);
    }
}

// VERIFY: 828a4f90 manual
// OFFSET: [0x00788C10<Packet<RainParticleBuffer>>, 0x00788D50<Packet<RainPatterParticleBuffer>>, 0x00788E90<Packet<SnowFlakeParticleBuffer>>, 0x00788FD0<Packet<SandGrainParticleBuffer>>]
template <class T>
PacketList<T>::~PacketList() {
    T* packet = this->m_activePackets.Head();

    while (packet) {
        T* next = this->m_activePackets.Next(packet);
        this->m_activePacketsCount--;
        this->m_freePacketsCount++;
        this->m_freePackets.LinkToTail(packet);
        packet = next;
    }

    packet = this->m_freePackets.Head();

    while (packet) {
        T* next = this->m_freePackets.Next(packet);
        packet->~T();
        SMemFree(packet, decltype(packet->m_particlesBuffer)::s_packetName, SLOG_OBJECT, 0);
        packet = next;
    }
}

// VERIFY: aced9f3f manual
// OFFSET: [0x00787920<Packet<RainParticleBuffer>>, 0x00787A10<Packet<RainPatterParticleBuffer>>, 0x00787B00<Packet<SnowFlakeParticleBuffer>>, 0x00787BF0<Packet<SandGrainParticleBuffer>>]
template <class T>
T* PacketList<T>::Alloc() {
    T* packet = this->m_freePackets.Head();

    if (!packet) {
        void* mem = SMemAlloc(sizeof(T), decltype(packet->m_particlesBuffer)::s_packetName, SLOG_OBJECT, 8);
        packet = mem ? new (mem) T() : nullptr;
        this->m_freePackets.LinkToTail(packet);
        this->m_freePacketsCount++;
    }

    packet->m_packetBuildTime = 0.0f;
    packet->m_currParticle = 0;
    packet->m_packetStartTick = 0;
    packet->m_origin.x = 0.0f;
    packet->m_packetEndTick = 0;
    packet->m_origin.y = 0.0f;
    packet->m_origin.z = 0.0f;
    this->m_activePackets.LinkToTail(packet);
    this->m_activePacketsCount++;
    this->m_freePacketsCount--;
    return packet;
}

// VERIFY: 356aaf00 auto
// OFFSET: 0x007845F0
template <class T>
void __stdcall RealizeParticlePacket(Packet<T>* packet) {
    auto* vertices = reinterpret_cast<decltype(&packet->m_particlesBuffer.m_particles[0])>(g_theGxDevicePtr->BufLock(packet->m_vertexBufHandle));

    for (uint32_t i = 0; i < packet->m_currParticle; i++) {
        vertices[i].m_position = packet->m_particlesBuffer.m_particles[i].m_position;
        vertices[i].m_velocity = packet->m_particlesBuffer.m_particles[i].m_velocity;
        vertices[i].m_startTime = packet->m_particlesBuffer.m_particles[i].m_startTime;
        vertices[i].m_endTime = packet->m_particlesBuffer.m_particles[i].m_endTime;
    }

    GxBufUnlock(packet->m_vertexBufHandle, 0);
}

class Rain {
    public:
    static void __stdcall GetMotionBlurMatrix(C33Matrix* matrix);
    static float GetWindTiltFactor();
    static void __stdcall RealizePacket(Packet<RainParticleBuffer>* packet);
    static void __stdcall RealizePacket(Packet<RainPatterParticleBuffer>* packet);

    /* 0000 */ uint8_t m_useWeatherShaders;
    /* 0001 */ uint8_t m_fadeOut;
    /* 0002 */ uint8_t unused_0002[2];
    /* 0004 */ CAaBox m_bbox;
    /* 001C */ Mists m_mists;
    /* 0070 */ PacketList<Packet<RainParticleBuffer>> m_rainDropPacketList;
    /* 0090 */ PacketList<Packet<RainPatterParticleBuffer>> m_rainPatterPacketList;
    /* 00B0 */ Packet<RainParticleBuffer>* m_rainDropPacketPtr;
    /* 00B4 */ float m_maxBuildTime;
    /* 00B8 */ Packet<RainPatterParticleBuffer>* m_rainPatterPacketPtr;
    /* 00BC */ CTexture* m_rainDropTex;
    /* 00C0 */ CGxShader* m_shaderRain;
    /* 00C4 */ CTexture* m_rainDropSplashTex;
    /* 00C8 */ CGxShader* m_shaderPatter;
    /* 00CC */ float m_particlesPerFrame;
    /* 00D0 */ float m_intensity;

    Rain(float width, float height);
    ~Rain();
    int32_t CountActivePackets();
    void InitDrop(RainDropParticle* drop, float time);
    void Render();
    void RenderDropPackets();
    void RenderDropPacketsSW();
    void RenderPatterPackets();
    void RenderPatterPacketsSW();
    void SetStormIntensity(float intensity);
    void UpdatePacket(Packet<RainParticleBuffer>* packet, Packet<RainPatterParticleBuffer>* patter, float count);
    void UpdatePackets();
};

#endif
