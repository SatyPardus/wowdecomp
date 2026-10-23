#ifndef ENGINE_SOURCE_FRAME_CSIMPLEFONTSTRINGATTRIBUTES_H
#define ENGINE_SOURCE_FRAME_CSIMPLEFONTSTRINGATTRIBUTES_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/RCString.h"
#include "Engine/Source/Frame/Types.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/CImVector.h"

class CSimpleFontString;

enum FONT_ATTRIBUTE_UPDATE_FLAGS : uint32_t {
    FLAG_FONT_UPDATE = 1,
    FLAG_STYLE_UPDATE = 2,
    FLAG_COLOR_UPDATE = 4,
    FLAG_SHADOW_UPDATE = 8,
    FLAG_SPACING_UPDATE = 16,
    FLAG_COMPLETE_UPDATE = 31,
};

class CSimpleFontStringAttributes {
    public:
    /* 0000 */ uint32_t m_flags;
    /* 0004 */ RCString m_font;
    /* 0010 */ float m_fontHeight;
    /* 0014 */ uint32_t m_fontFlags;
    /* 0018 */ float m_spacing;
    /* 001C */ uint32_t m_styleFlags;
    /* 0020 */ CImVector m_color;
    /* 0024 */ CImVector m_shadowColor;
    /* 0028 */ C2Vector m_shadowOffset;

    CSimpleFontStringAttributes();
    CSimpleFontStringAttributes& operator=(const CSimpleFontStringAttributes& source);
    int32_t SetFont(const char* fontName, float fontHeight, uint32_t fontFlags);
    void SetJustifyH(uint8_t justify);
    void SetJustifyV(uint8_t justify);
    void SetShadow(const CImVector& color, const C2Vector& offset);
    void SetShadowOffset(const C2Vector& offset);
    void Update(CSimpleFontString* string, int32_t flags);
    void Update(CSimpleFontStringAttributes& attributes, int32_t flags);
};

#endif
