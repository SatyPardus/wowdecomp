#ifndef ENGINE_SOURCE_GX_CGXTEX_H
#define ENGINE_SOURCE_GX_CGXTEX_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/CGxDevice/Types.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/CiRect.h"

class CGxTex;

typedef void (*GxTexUserFunc)(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels);

class CGxTexParms {
    public:
    /* 0000 */ EGxTexTarget m_target;
    /* 0004 */ uint32_t m_width;
    /* 0008 */ uint32_t m_height;
    /* 000C */ uint32_t m_depth;
    /* 0010 */ EGxTexFormat m_format;
    /* 0014 */ EGxTexFormat m_dataFormat;
    /* 0018 */ CGxTexFlags m_flags;
    /* 001C */ void* m_userArg;
    /* 0020 */ GxTexUserFunc m_userFunc;
};

class CGxTex {
    public:
    /* 0000 */ CiRect m_updateRect;
    /* 0010 */ int16_t m_updatePlaneMin;
    /* 0012 */ int16_t m_updatePlaneMax;
    /* 0014 */ uint32_t m_width;
    /* 0018 */ uint32_t m_height;
    /* 001C */ uint32_t m_depth;
    /* 0020 */ EGxTexTarget m_target;
    /* 0024 */ EGxTexFormat m_format;
    /* 0028 */ EGxTexFormat m_dataFormat;
    /* 002C */ CGxTexFlags m_flags;
    /* 0030 */ void* m_userArg;
    /* 0034 */ GxTexUserFunc m_userFunc;
    /* 0038 */ void* m_apiSpecificData;
    /* 003C */ void* m_apiSpecificData2;
    /* 0040 */ TSLink<CGxTex> m_link;
    /* 0048 */ TSLink<CGxTex> m_link2;
    /* 0050 */ uint32_t m_lastUsedFrame;
    /* 0054 */ uint32_t unused_0054;
    /* 0058 */ int8_t m_char58;
    /* 0059 */ int8_t m_char59;
    /* 005A */ int8_t m_needsUpdate;
    /* 005B */ int8_t m_needsCreation;
    /* 005C */ int8_t m_needsFlagUpdate;
    /* 005D */ int8_t m_char5D;
    /* 005E */ uint16_t m_pad;

    CGxTex(EGxTexTarget target, uint32_t width, uint32_t height, uint32_t depth, EGxTexFormat format, EGxTexFormat dataFormat, CGxTexFlags flags, void* userArg, GxTexUserFunc userFunc, const char* name);
    ~CGxTex();
};

#endif
