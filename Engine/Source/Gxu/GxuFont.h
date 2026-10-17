#ifndef ENGINE_SOURCE_GXU_GXUFONT_H
#define ENGINE_SOURCE_GXU_GXUFONT_H

#include <cstddef>
#include <cstdint>
#include <freetype/freetype.h>
#include "Engine/Source/Gxu/GxuFontString.h"
#include "Engine/Source/Gxu/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C3Vector.h"

class C2Vector;
class CGxShader;
class CImVector;
struct CGxFont;
struct CGxStringBatch;

extern FT_Library g_FTLibrary;
extern CGxShader* g_fontPixelShader;
extern CGxShader* g_fontMsdfPixelShader;
extern TSList<CGxFont, TSGetLink<CGxFont> > g_fonts;
extern uint8_t g_fontStatsEnabled;
extern CGxShader* g_fontVertexShader[2];
extern TSList<CGxString, TSGetLink<CGxString> > g_freeStrings;
extern uint32_t g_heightPixels;
extern float g_indentNormWidth;
extern float g_indentPixelWidth;
extern const int32_t g_quotedCodeLengths[NUM_QUOTEDCODES];
extern TSList<CGxString, TSGetLink<CGxString> > g_strings;
extern uint32_t g_widthPixels;

QUOTEDCODE GxuDetermineQuotedCode(const char* text, int32_t& advance, CImVector* color, uint32_t flags, uint32_t& wide);
void GxuFontAddShadow(CGxString* string, const CImVector& color, const C2Vector& offset);
int32_t GxuFontAddToBatch(CGxStringBatch* batch, CGxString* string);
int32_t GxuFontClearBatch(CGxStringBatch* batch);
CGxStringBatch* GxuFontCreateBatch(bool billboarded, bool worldSpace);
int32_t GxuFontCreateFont(const char* name, float fontHeight, CGxFont*& face, uint32_t flags);
int32_t GxuFontCreateString(CGxFont* face, const char* text, float fontHeight, const C3Vector& position, float blockWidth, float blockHeight, float spacing, CGxString*& string, EGxFontVJusts vertJust, EGxFontHJusts horzJust, uint32_t flags, const CImVector& color, float unused, float scale);
int32_t GxuFontDestroyBatch(CGxStringBatch* batch);
void GxuFontDestroyFont(CGxFont*& face);
void GxuFontDestroyString(CGxString*& string);
uint32_t GxuFontGetFontFlags(CGxFont* face);
const char* GxuFontGetFontName(CGxFont* face);
uint32_t GxuFontGetMaxCharsWithinWidth(CGxFont* face, const char* text, float fontHeight, float maxWidth, uint32_t numBytes, float* extent, float shadowWidth, float scale, float unused, uint32_t flags);
uint32_t GxuFontGetMaxCharsWithinWidthAndHeight(CGxFont* face, const char* text, float fontHeight, float blockWidth, float blockHeight, uint32_t numBytes, float shadowWidth, float scale, float spacing, uint32_t flags);
uint32_t GxuFontGetMaxCharsWithinWidthFromEnd(CGxFont* face, const char* text, float fontHeight, float maxWidth, uint32_t numBytes, float* extent, float shadowWidth, float scale, float unused, uint32_t flags);
float GxuFontGetOneToOneHeight(CGxFont* face);
C3Vector GxuFontGetStringPosition(CGxString* string);
void GxuFontGetTextExtent(CGxFont* face, const char* text, uint32_t numBytes, float fontHeight, float* extent, float shadowWidth, float scale, float unused, uint32_t flags);
float GxuFontGetWrappedTextHeight(CGxFont* face, const char* text, float fontHeight, float blockWidth, const C2Vector& shadowOffset, float scale, float spacing, uint32_t flags);
void GxuFontInitialize();
void GxuFontRemoveShadow(CGxString* string);
void GxuFontRenderBatch(CGxStringBatch* batch);
int32_t GxuFontSetStringColor(CGxString* string, const CImVector& color);
void GxuFontSetStringPosition(CGxString* string, const C3Vector& position);
void GxuFontShutdown();
uint32_t GxuFontStringEmbeddedTextureInfo(CGxString* string, const GXUEMBEDDEDTEXTUREINFO*& info);
uint32_t GxuFontStringHyperLinkInfo(CGxString* string, const GXUFONTHYPERLINKINFO*& info);
int32_t GxuFontStringSetGradient(CGxString* string, int32_t start, int32_t length);
const char* GxuFontStripEscapeCodes(const char* text, uint32_t textLength, uint32_t flags, char* buffer, uint32_t bufferSize);
void GxuFontUpdate();
void GxuFontWindowSizeChanged();
uint32_t GxuFontWrapText(CGxFont* face, const char* text, uint32_t textLength, float fontHeight, float blockWidth, uint32_t* lineStarts, uint32_t maxLines, float shadowWidth, float scale, float unused, uint32_t flags);

#endif
