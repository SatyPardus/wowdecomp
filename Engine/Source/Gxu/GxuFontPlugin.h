#ifndef ENGINE_SOURCE_GXU_GXUFONTPLUGIN_H
#define ENGINE_SOURCE_GXU_GXUFONTPLUGIN_H

#include <cstddef>
#include <cstdint>
#include <freetype/freetype.h>

struct GLYPHBITMAPDATA;

typedef void* (*FontAllocFn)(size_t size);
typedef void (*FontFreeFn)(void* ptr);

// Abstract interface for external font rasterizers (e.g. MSDF plugin)
struct IGxuFontBackend {
    virtual ~IGxuFontBackend() = default;
    virtual bool Initialize(FontAllocFn allocFn, FontFreeFn freeFn) = 0;
    virtual void Shutdown() = 0;
    virtual bool GenerateGlyph(FT_Face face, uint32_t pixelSize, uint32_t code, uint32_t baseline, GLYPHBITMAPDATA* dataPtr, uint32_t padding) = 0;
    virtual float ComputeScreenPxRange(uint32_t renderedPixelSize) = 0;
    virtual const void* GetPixelShaderBytecode(uint32_t* outSize) = 0;
    virtual const char* GetPixelShaderHlsl() = 0;
};

// C-ABI Registration
void Gxu_RegisterFontBackend(IGxuFontBackend* backend);
IGxuFontBackend* Gxu_GetFontBackend();

#endif // ENGINE_SOURCE_GXU_GXUFONTPLUGIN_H
