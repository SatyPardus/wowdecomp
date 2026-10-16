#ifndef ENGINE_SOURCE_GX_CGXFORMAT_H
#define ENGINE_SOURCE_GX_CGXFORMAT_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C2iVector.h"

class CGxFormat {
    public:
    enum Format {
        Fmt_Rgb565 = 0,
        Fmt_ArgbX888 = 1,
        Fmt_Argb8888 = 2,
        Fmt_Argb2101010 = 3,
        Fmt_Ds160 = 4,
        Fmt_Ds24X = 5,
        Fmt_Ds248 = 6,
        Fmt_Ds320 = 7,
        Formats_Last = 8,
    };

    /* 0000 */ uint32_t m_apiSpecificModeID;
    /* 0004 */ bool m_hwTnL;
    /* 0005 */ bool m_hwCursor;
    /* 0006 */ int8_t m_fixLag;
    /* 0007 */ int8_t m_window;
    /* 0008 */ bool m_aspect;
    /* 0009 */ uint8_t unused_0009[3];
    /* 000C */ int32_t m_maximize;
    /* 0010 */ CGxFormat::Format m_depthFormat;
    /* 0014 */ C2iVector m_size;
    /* 001C */ uint32_t m_backBufferCount;
    /* 0020 */ uint32_t m_multisampleCount;
    /* 0024 */ float m_multisampleQuality;
    /* 0028 */ CGxFormat::Format m_colorFormat;
    /* 002C */ uint32_t m_refreshRate;
    /* 0030 */ uint32_t m_vsync;
    /* 0034 */ bool m_stereoEnabled;
    /* 0035 */ uint8_t unused_0035[3];
    /* 0038 */ int32_t m_fixedFunction [6];
    /* 0050 */ C2iVector m_pos;

    CGxFormat();
    CGxFormat(bool window, const C2iVector& size, CGxFormat::Format colorFormat, CGxFormat::Format depthFormat, uint32_t refreshRate, uint32_t vsync, bool hwTnL, bool fixLag, bool hwCursor, bool aspect, int32_t maximize);
};

#endif
