#ifndef ENGINE_SOURCE_GXU_GXUFONTMSDF_H
#define ENGINE_SOURCE_GXU_GXUFONTMSDF_H

#include <cstddef>
#include <cstdint>
#include <freetype/freetype.h>
#include "Engine/Source/Gxu/GxuFontPlugin.h"

struct GLYPHBITMAPDATA;

// MSDF configuration constants
static constexpr uint32_t MSDF_ATLAS_SIZE = 512;          // Atlas texture dimensions (512x512)
static constexpr uint32_t MSDF_GLYPH_SIZE = 48;           // Glyph cell size in atlas pixels
static constexpr double   MSDF_PIXEL_RANGE = 3.0;         // SDF range in pixels
static constexpr float    MSDF_INV_ATLAS_SIZE = 1.0f / 512.0f;

// Global MSDF enable flag (1 if plugin backend is active, 0 for vanilla FreeType fallback)
extern uint8_t g_msdfEnabled;

// Initialize / shutdown the MSDF subsystem
void MsdfInitialize();
void MsdfShutdown();

// Generate an MSDF glyph for the given codepoint using the registered backend
bool MsdfGenerateGlyph(FT_Face face, uint32_t pixelSize, uint32_t code, uint32_t baseline, GLYPHBITMAPDATA* dataPtr, uint32_t padding);

// Compute the screenPxRange shader constant
float MsdfComputeScreenPxRange(uint32_t renderedPixelSize);

class CGxShader;
// Create the MSDF pixel shader from registered backend
CGxShader* GxuFontCreateMsdfPixelShader();

#endif // ENGINE_SOURCE_GXU_GXUFONTMSDF_H
