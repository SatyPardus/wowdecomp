#ifndef WOW_SOURCE_WORLDCLIENT_SAND_H
#define WOW_SOURCE_WORLDCLIENT_SAND_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "WoW/Source/WorldClient/Mists.h"
#include "WoW/Source/WorldClient/Rain.h"

class CGxShader;
struct SandGrainParticleBuffer;

struct SandGrainParticle {
    /* 0000 */ C3Vector m_position;
    /* 000C */ C3Vector m_velocity;
    /* 0018 */ float m_startTime;
    /* 001C */ float m_endTime;
};

struct SandGrainParticleBuffer {
    static const char* const s_packetName;
    static const uint32_t s_vertexSize = 32;
    static const uint32_t s_verticesPerParticle = 1;

    /* 0000 */ uint32_t m_particleCount;
    /* 0004 */ SandGrainParticle m_particles [6144];

    SandGrainParticleBuffer();
};

class Sand {
    public:
    /* 0000 */ uint8_t m_useWeatherShaders;
    /* 0001 */ uint8_t m_fadeOut;
    /* 0002 */ uint8_t unused_0002[2];
    /* 0004 */ CAaBox m_bbox;
    /* 001C */ Mists m_mists;
    /* 0070 */ PacketList<Packet<SandGrainParticleBuffer>> m_sandGrainPacketList;
    /* 0090 */ Packet<SandGrainParticleBuffer>* m_sandGrainPacketPtr;
    /* 0094 */ float m_maxBuildTime;
    /* 0098 */ CGxShader* m_shader;
    /* 009C */ float m_particlesPerFrame;

    Sand(float width, float height);
    ~Sand();
    int32_t CountActivePackets();
    CAaBox GetBoundingBox() const;
    void Render();
    void SetStormIntensity(float intensity);
    void RenderPackets();
    void RenderPacketsShader();
    void UpdatePackets();
    void InitGrain(SandGrainParticle* grain, float time);
    void UpdatePacket(Packet<SandGrainParticleBuffer>* packet, float count);
};

#endif
