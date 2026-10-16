#ifndef WOW_SOURCE_TYPES_H
#define WOW_SOURCE_TYPES_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/Types.h"

class CGxFormat;
struct CpuHardware;
struct SoundHardware;
class VideoHardwareRec;

struct DefaultSettings {
    /* 0000 */ float m_farClip;
    /* 0004 */ uint32_t m_terrainShadowLOD;
    /* 0008 */ uint32_t m_detailDoodadDensity;
    /* 000C */ uint32_t m_detailDoodadAlpha;
    /* 0010 */ bool m_animatingDoodads;
    /* 0011 */ bool m_trilinear;
    /* 0012 */ uint8_t unused_0012[2];
    /* 0014 */ uint32_t m_numLights;
    /* 0018 */ bool m_specularity;
    /* 0019 */ bool unk_0019;
    /* 001A */ bool m_specular;
    /* 001B */ uint8_t unused_001B[1];
    /* 001C */ uint32_t m_waterLOD;
    /* 0020 */ float m_particleDensity;
    /* 0024 */ float m_unitDrawDist;
    /* 0028 */ float m_smallCull;
    /* 002C */ float m_distCull;
    /* 0030 */ CGxFormat* m_format;
    /* 0034 */ uint32_t m_baseMipLevel;
    /* 0038 */ uint32_t m_numChannels;
    /* 003C */ bool m_fivePointOne;
    /* 003D */ uint8_t unused_003D[3];
};

struct Hardware {
    struct Device {
        /* 0000 */ uint16_t m_vendorID;
        /* 0002 */ uint16_t m_deviceID;
        /* 0004 */ uint32_t m_driverVersionHi;
        /* 0008 */ uint32_t m_driverVersionLo;
    };

    /* 0000 */ Hardware::Device m_videoDevice;
    /* 000C */ Hardware::Device m_soundDevice;
    /* 0018 */ uint32_t m_cpuIdx;
    /* 001C */ uint32_t m_videoID;
    /* 0020 */ uint32_t m_soundIdx;
    /* 0024 */ uint32_t m_memIdx;
    /* 0028 */ VideoHardwareRec* m_videoHw;
    /* 002C */ CpuHardware* m_cpuHw;
    /* 0030 */ SoundHardware* m_soundHw;
};

#endif
