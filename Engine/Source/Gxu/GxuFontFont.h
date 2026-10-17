#ifndef ENGINE_SOURCE_GXU_GXUFONTFONT_H
#define ENGINE_SOURCE_GXU_GXUFONTFONT_H

#include <cstddef>
#include <cstdint>
#include <freetype/freetype.h>
#include "Engine/Source/Gxu/GxuFontMiscClasses.h"
#include "Engine/Source/Gxu/GxuFontString.h"
#include "Engine/Source/Gxu/Types.h"
#include "Engine/Source/Storm/H/stpl.h"

struct KERNINGHASHKEY {
    /* 0000 */ uint32_t m_code;

    KERNINGHASHKEY& operator=(const KERNINGHASHKEY& key);
    bool operator==(const KERNINGHASHKEY& key);
};

struct KERNNODE : public TSHashObject<KERNNODE, KERNINGHASHKEY> {
    /* 0018 */ uint32_t m_flags;
    /* 001C */ float m_proportionalSpacing;
    /* 0020 */ float m_fixedWidthSpacing;
};

struct CGxFont : public TSLinkedNode<CGxFont> {
    /* 0008 */ TSExplicitList<CGxString, offsetof(CGxString, m_fontStringLink)> m_strings;
    /* 0014 */ TSHashTable<CHARCODEDESC, HASHKEY_NONE> m_activeCharacters;
    /* 003C */ TSHashTable<KERNNODE, KERNINGHASHKEY> m_kernInfo;
    /* 0064 */ TSExplicitList<CHARCODEDESC, offsetof(CHARCODEDESC, m_fontGlyphLink)> m_activeCharacterCache;
    /* 0070 */ HFACE m_fontFaceHandle;
    /* 0074 */ char m_fontName[260];
    /* 0178 */ uint32_t m_cellHeight;
    /* 017C */ uint32_t m_baseline;
    /* 0180 */ uint32_t m_fontFlags;
    /* 0184 */ float m_fontHeight;
    /* 0188 */ float m_pixelsPerUnit;
    /* 018C */ TEXTURECACHE m_textureCache[8];
    /* 024C */ uint32_t m_pixelSize;
    /* 0250 */ uint8_t m_useMsdf;

    CGxFont();
    ~CGxFont();
    int32_t CheckStringGlyphs(const char* string);
    void ClearGlyphs();
    float ComputeStep(uint32_t code, uint32_t nextCode);
    float ComputeStepFixedWidth(uint32_t code, uint32_t nextCode);
    float FindNearestFixedSize(FT_Face face, float height);
    float GetGlyphAdvance(uint32_t code);
    float GetGlyphBearing(const CHARCODEDESC* desc, bool scaled, float height);
    bool GetGlyphData(GLYPHBITMAPDATA* data, uint32_t code);
    const char* GetName();
    uint32_t GetPixelSize();
    void HandleScreenSizeChange();
    int32_t Initialize(const char* name, uint32_t fontFlags, float fontHeight);
    CHARCODEDESC* NewCodeDesc(uint32_t code);
    void RegisterEvictNotice(uint32_t texturePage);
    int32_t UpdateDimensions();
};

#endif
