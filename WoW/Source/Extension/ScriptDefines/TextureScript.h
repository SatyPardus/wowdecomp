#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_TEXTURESCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_TEXTURESCRIPT_H

#include "WoW/Source/Extension/ScriptObject.h"

class CTexture;

// File textures (Engine/Source/Services/Texture.cpp). TextureCreate(fileName, ...) is the one
// entry every BLP/TGA texture request goes through (models, terrain, WMOs, UI): it consults the
// texture cache first and, on a miss, creates the CTexture (CreateBlpTexture / CreateTgaTexture,
// the pixel data arriving asynchronously) and registers it in the cache. Procedural textures
// (TextureCreate with dimensions, TextureCreateSolid, blobs) do not pass through this hook.
class TextureScript : public ScriptObject {
    protected:
    explicit TextureScript(const char* name);

    public:
    // TextureCreate(const char* fileName, ...) right after the new texture is registered in the
    // cache: the CTexture exists and will be handed to the caller; its image data may still be
    // loading. fileName is the path as requested (before the .blp/.tga extension retry).
    // Cache hits do not fire: this is once per distinct file and flag set, not once per use.
    virtual void OnTextureCreated(CTexture& texture, const char* fileName) {}

    // PumpBlpTextureAsync on success: a BLP texture's pixel data is decoded and uploaded
    // (texture.m_gxTex allocated with TextureAllocGxTex and filled with GxTexUpdate). Both load
    // paths end here, the synchronous one inside TextureCreate and the asynchronous one from
    // AsyncTextureSuccessCallback when the file read completes. texture.m_filename is the path.
    // Not fired for a BLP whose data failed to decode (the engine fills it with the default
    // colour instead) nor for TGA or procedural textures.
    virtual void OnTextureLoaded(CTexture& texture) {}
};

#endif
