#ifndef ENGINE_SOURCE_SERVICES_LIGHTNING_H
#define ENGINE_SOURCE_SERVICES_LIGHTNING_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"

class CTexture;
struct LightningJoint;
class SpellChainEffectsRec;

class CLightning {
    public:
    /* 0000 */ C3Vector m_srcPos;
    /* 000C */ C3Vector m_dstPos;
    /* 0018 */ CImVector m_color;
    /* 001C */ const SpellChainEffectsRec* m_chainEffects;
    /* 0020 */ TSFixedArray<LightningJoint> m_joints;
    /* 002C */ TSFixedArray<C3Vector> m_points;
    /* 0038 */ TSFixedArray<C3Vector> m_positions;
    /* 0044 */ TSFixedArray<C2Vector> m_texCoords;
    /* 0050 */ TSFixedArray<CImVector> m_colors;
    /* 005C */ TSFixedArray<uint16_t> m_indices;
    /* 0068 */ uint32_t unused_0068;
    /* 006C */ uint32_t unused_006C;
    /* 0070 */ uint32_t unused_0070;
    /* 0074 */ uint32_t unused_0074;
    /* 0078 */ uint32_t unused_0078;
    /* 007C */ uint32_t unused_007C;
    /* 0080 */ uint32_t unused_0080;
    /* 0084 */ CTexture* m_texture;
    /* 0088 */ void (*m_coordUpdate)(void*, uint32_t, C3Vector*, C3Vector*);
    /* 008C */ void* m_coordUpdateParam;
    /* 0090 */ uint32_t m_flags;
    /* 0094 */ uint32_t unused_0094;
    /* 0098 */ uint32_t unused_0098;
    /* 009C */ uint32_t unused_009C;
    /* 00A0 */ uint32_t unused_00A0;
    /* 00A4 */ uint32_t unused_00A4;
    /* 00A8 */ uint32_t unused_00A8;
    /* 00AC */ uint32_t unused_00AC;
    /* 00B0 */ uint32_t unused_00B0;
};

#endif
