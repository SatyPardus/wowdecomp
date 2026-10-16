#ifndef ENGINE_SOURCE_GX_CGXMATRIXSTACK_H
#define ENGINE_SOURCE_GX_CGXMATRIXSTACK_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C44Matrix.h"

class CGxMatrixStack {
    public:
    enum EMatrixFlags {
        F_Identity = 0x1
    };

    /* 0000 */ uint32_t m_level;
    /* 0004 */ int8_t m_dirty;
    /* 0005 */ uint8_t unused_0005[3];
    /* 0008 */ C44Matrix m_mtx [4];
    /* 0108 */ uint32_t m_flags [4];

    CGxMatrixStack();
    void Identity();
};

#endif
