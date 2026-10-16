#ifndef WOW_SOURCE_WORLDCLIENT_MISTS_H
#define WOW_SOURCE_WORLDCLIENT_MISTS_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C3Vector.h"

class CTexture;

class Mists {
    public:
    struct Mist {
        /* 0000 */ uint32_t m_maxHeights;
        /* 0004 */ float m_heights[64];
        /* 0104 */ int32_t m_heightCount;
        /* 0108 */ C3Vector m_position;
        /* 0114 */ C3Vector m_velocity;
        /* 0120 */ uint32_t m_startTick;
        /* 0124 */ uint32_t m_endTick;
        /* 0128 */ float m_acceleration;

        Mist();
    };

    /* 0004 */ C3Vector m_extents;
    /* 0010 */ C3Vector unused_0010;
    /* 001C */ float m_size;
    /* 0020 */ float m_spawnRate;
    /* 0024 */ float m_angle;
    /* 0028 */ float m_angleVariance;
    /* 002C */ float m_spawnAccum;
    /* 0030 */ float m_fadeTime;
    /* 0034 */ float m_speed;
    /* 0038 */ float m_speedVariance;
    /* 003C */ uint8_t m_fadeOut;
    /* 003D */ uint8_t m_padding [3];
    /* 0040 */ CTexture* m_texture;
    /* 0044 */ TSGrowableArray<Mist> m_mists;

    virtual void InitMist(Mist* mist, uint32_t startTick);

    Mists(float size, const C3Vector& extents, const char* textureName, float angle, float angleVariance, float speed, float speedVariance, uint32_t count);
    ~Mists();
    void CreateMist(Mist* mist, uint32_t startTick);
    void Render();
};

#endif
