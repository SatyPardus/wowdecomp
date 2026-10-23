#include <cstddef>
#include <cstdint>
#include <cstring>
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/BLPFile/blp.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Images/MippedImg.h"
#include "Engine/Source/Images/tga.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Services/TextureBlob.h"
#include "Engine/Source/Services/TextureInt.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"

// STATIC: 0x009F1028
static const uint16_t s_pixelFormatBytesPerBlock[NUM_PIXEL_FORMATS] = { 8, 16, 0, 0, 0, 0, 0, 16, 0, 0 };

// STATIC: 0x009F103C
static const uint16_t s_texFormatBitDepth[GxTexFormats_Last] = { 0, 32, 32, 16, 16, 16, 4, 8, 8, 16, 32, 32, 32 };

// STATIC: 0x009F1058
static const uint16_t s_texFormatBytesPerBlock[GxTexFormats_Last] = { 0, 0, 0, 0, 0, 0, 8, 16, 16, 0, 0, 0, 0 };

// STATIC: 0x009F1074
static const int32_t s_pixelFormatToMipBitsCache[NUM_PIXEL_FORMATS] = { -1, -1, -1, 0, 0, 0, -1, -1, -1, 1 };

// STATIC: 0x009F1120
static const uint32_t s_texFormatBytesPerTexel[GxTexFormats_Last] = { 0, 4, 4, 2, 2, 2, 8, 16, 16, 2, 4, 4, 4 };

// STATIC: 0x009F1154
static const uint32_t s_texFormatTexelsPerBlock[GxTexFormats_Last] = { 1, 1, 1, 1, 1, 1, 4, 4, 4, 1, 1, 1, 1 };

// STATIC: 0x00AC3234
static EGxTexFormat s_pixelFormatToGxTexFormat[NUM_PIXEL_FORMATS] = { GxTex_Dxt1, GxTex_Dxt3, GxTex_Argb8888, GxTex_Argb1555, GxTex_Argb4444, GxTex_Rgb565, GxTex_Unknown, GxTex_Dxt5, GxTex_Unknown, GxTex_Unknown };

// STATIC: 0x00AC3298
static EGxTexFilter s_filterMode = GxTex_LinearMipNearest;

// STATIC: 0x00AC329C
static uint32_t s_maxAnisotropy = 1;

// STATIC: 0x00AC32A0
static int32_t s_createBlpAsync = 1;

// STATIC: 0x00AC3348
static TSExplicitList<CTexture, offsetof(CTexture, m_link)> s_textureList;

// STATIC: 0x00AC3354
static uint32_t s_defaultColor = 0xFF00FF00;

// STATIC: 0x00AC3358
static TSExplicitList<CGxTexCache, offsetof(CGxTexCache, m_link)> s_gxTexCacheFree;

// STATIC: 0x00AC3364
static TSExplicitList<CMipBitsCache, offsetof(CMipBitsCache, m_link)> s_mipBitsCacheFree;

// STATIC: 0x00AC3370
static TSExplicitList<CTextureAtlas, offsetof(CTextureAtlas, m_link)> s_atlasList;

// STATIC: 0x00AC337C
static TSExplicitList<CAsyncObject, offsetof(CAsyncObject, m_link)> s_asyncLoadList;

// STATIC: 0x00AC3388
static TSExplicitList<CAsyncObject, offsetof(CAsyncObject, m_link)> s_asyncTextureList;

// STATIC: 0x00B49C78
static uint32_t s_currentTime;

// STATIC: 0x00B49C7C
static char* s_substituteFrom;

// STATIC: 0x00B49C80
static char* s_substituteTo;

// STATIC: 0x00B49C84
static int32_t s_atlasEnable;

// STATIC: 0x00B49C90
static MipBits* s_mipBits;

// STATIC: 0x00B49C94
static int32_t s_pumping;

// STATIC: 0x00B49C98
static int32_t s_gxTexCacheSize;

// STATIC: 0x00B49C9C
static int32_t s_gxTexCacheMaxSize;

// STATIC: 0x00B49CA0
static uint32_t s_asyncLoadBufferUsed;

// STATIC: 0x00B49CA4
static TSHashTableReuse<CTexture, HASHKEY_TEXTUREFILE> s_textureCache;

// STATIC: 0x00B49CD8
static TSExplicitList<CGxTexCache, offsetof(CGxTexCache, m_link)> s_gxTexCache[6][6];

// STATIC: 0x00B49E88
static TSExplicitList<CMipBitsCache, offsetof(CMipBitsCache, m_link)> s_mipBitsCache[6][6][2];

// INLINED
HASHKEY_TEXTUREFILE::HASHKEY_TEXTUREFILE() {
    this->m_filename = nullptr;
    this->m_anyTexFlags = 0;
}

// INLINED
bool HASHKEY_TEXTUREFILE::operator==(const HASHKEY_TEXTUREFILE& key) const {
    return !SStrCmpI(this->m_filename, key.m_filename, 260) && (this->m_anyTexFlags || key.m_anyTexFlags || this->m_texFlags == key.m_texFlags);
}

// INLINED
CMipBitsCache::CMipBitsCache() {
    this->m_bits = nullptr;
}

// INLINED
CMipBitsCache::~CMipBitsCache() {
    if (this->m_bits) {
        SMemFree(this->m_bits, __FILE__, __LINE__, 0);
    }
}

// VERIFY: c74642f9 manual
// OFFSET: 0x005EEB70
void GxTexNullUserFunc(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
}

// VERIFY: 9d282c43 auto
// OFFSET: 0x004B50C0
EGxTexFormat TexturePixelFormatToGxTexFormat(PIXEL_FORMAT format) {
    return s_pixelFormatToGxTexFormat[format];
}

// VERIFY: 10032e81 auto
// OFFSET: 0x004B50D0
void CTextureAtlas::Update(CTexture* texture) {
    this->m_updateSlot = texture->m_atlasBlockIndex;
    GxTexUpdate(this->m_gxTex, (texture->m_atlasBlockIndex & 7) * this->m_width, ((texture->m_atlasBlockIndex >> 3) & 7) * this->m_height, (texture->m_atlasBlockIndex & 7) * this->m_width + this->m_width, ((texture->m_atlasBlockIndex >> 3) & 7) * this->m_height + this->m_height, 1);
    this->m_updateSlot = -1;
}

// VERIFY: 89c9895e manual
// OFFSET: 0x004B5130
void AsyncTextureDestroyedCallback(CAsyncObject* object) {
    s_asyncLoadBufferUsed -= object->m_size;
    void* buffer = object->m_buffer;
    AsyncFileReadDestroyObject(object);
    SMemFree(buffer, __FILE__, __LINE__, 0);
}

// VERIFY: 0a24e97f auto
// OFFSET: 0x004B5170
void TextureReportLoadError(const char* type, const char* fileName, CStatus* status) {
    char buffer[256];
    SErrGetErrorStr(SErrGetLastError(), buffer, sizeof(buffer));
    status->Add(STATUS_FATAL, "Error loading %s file \"%s\": %s\n", type, fileName, buffer);
    SErrSetLastError(0);
}

// VERIFY: 23747110 auto
// OFFSET: 0x004B51C0
char* OsPathFindExtensionWithDot(const char* path) {
    const char* slash = strrchr(path, '\\');

    if (!slash) {
        slash = strrchr(path, '/');
    }

    const char* dot = strrchr(path, '.');

    if (dot && (!slash || slash < dot)) {
        return const_cast<char*>(dot);
    }

    return const_cast<char*>(path + strlen(path));
}

// VERIFY: 463fb781 auto
// OFFSET: 0x004B5210
int32_t TextureBuildAliasPath(char* buffer, const char* fileName) {
    if (!s_substituteFrom) {
        return 0;
    }

    const char* name = SStrChrR(const_cast<char*>(fileName), '\\');
    name = name ? name + 1 : fileName;

    if (SStrCmpI(s_substituteFrom, name, STORM_MAX_STR)) {
        return 0;
    }

    SStrCopy(buffer, fileName, 260);
    SStrCopy(&buffer[name - fileName], s_substituteTo, 260 - (name - fileName));

    return 1;
}

// VERIFY: a6ae9dde manual
// OFFSET: 0x004B5280
MipBits* GetDefaultTexture(uint32_t width, uint32_t height) {
    MipBits* images = s_mipBits;
    MipBits* level = images;

    MippedImgSet(PIXEL_ARGB8888, width, height, images);

    while (width > 1 || height > 1) {
        memset(level->m_mip[0], 0xFF, 4 * height * width);
        level++;
        width = width >> 1 ? width >> 1 : 1;
        height >>= 1;

        if (!height) {
            height = 1;
        }
    }

    return images;
}

// VERIFY: ec360367 manual
// OFFSET: 0x004B5300
void AsyncTextureFailureCallback(CTexture* texture) {
    CAsyncObject* object = texture->m_asyncObject;
    s_asyncLoadBufferUsed -= object->m_size;
    void* buffer = object->m_buffer;
    AsyncFileReadDestroyObject(object);
    SMemFree(buffer, __FILE__, __LINE__, 0);
}

// VERIFY: 83041e31 auto
// OFFSET: 0x004B5340
SFileImpl* TextureCreateOpenFile(const char* fileName, int32_t flags) {
    SFileImpl* file = nullptr;
    SErrSetLastError(0);

    if (SFile::OpenEx(nullptr, fileName, flags != 0, &file)) {
        return file;
    }

    if (!SErrGetLastError()) {
        SErrSetLastError(STORM_ERROR_FILE_NOT_FOUND);
    }

    return nullptr;
}

// VERIFY: 47fab888 auto
// OFFSET: 0x004B5390
void TextureEnableAtlas(int32_t enable) {
    s_atlasEnable = enable;
}

// VERIFY: dd850258 auto
// OFFSET: 0x004B53A0
int32_t TextureIsSame(CTexture* texture, const char* fileName) {
    char name[260];
    uint32_t length = SStrCopy(name, fileName, sizeof(name));

    if (length >= 4 && name[length - 4] == '.') {
        length -= 4;
    }

    char* extension = &name[length];

    if (*extension == '.') {
        SStrLower(extension + 1);
        *extension = '\0';
    }

    if (!texture) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    return SStrCmpI(name, texture->m_filename, sizeof(texture->m_filename)) == 0;
}

// VERIFY: f5c866d4 auto
// OFFSET: 0x004B5430
void TextureSetUserData(CTexture* texture, GxTexUserFunc userFunc, void* userArg) {
    if (!texture) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    GxTexSetUserData(texture->m_gxTex, userFunc, userArg);
}

// VERIFY: 2db96900 auto
// OFFSET: 0x004B5460
int32_t TextureOnAtlas(CTexture* texture) {
    if (!texture) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    return texture->m_atlas != nullptr;
}

// VERIFY: 26865c8c auto
// OFFSET: 0x004B5490
int32_t TextureGetAtlasOffsetAndScale(CTexture* texture, C2Vector& offset, float& scale) {
    if (!texture) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!texture->m_atlas) {
        return 0;
    }

    scale = 0.125f;
    offset.x = 0.125f * (texture->m_atlasBlockIndex & 7);
    offset.y = ((texture->m_atlasBlockIndex >> 3) & 7) * scale;

    return 1;
}

// VERIFY: 71c27d9c auto
// OFFSET: 0x004B54F0
int32_t TextureIsOpaque(CTexture* texture) {
    if (!texture) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    return texture->m_flags & 1;
}

// VERIFY: 693ca984 manual
// OFFSET: 0x004B5510
int32_t TextureCalcMipCount(uint32_t width, uint32_t height) {
    int32_t count = 1;

    while (width > 1 || height > 1) {
        width >>= 1;
        count++;

        if (!width) {
            width = 1;
        }

        height >>= 1;

        if (!height) {
            height = 1;
        }
    }

    return count;
}

// VERIFY: 2866c66e manual
// OFFSET: 0x004B5550
void TextureGenerateMips(uint32_t width, uint32_t height, uint32_t firstLevel, uint32_t levelCount, MipBits* images) {
    uint32_t sourceWidth = width;
    uint32_t sourceHeight = height;
    uint32_t sourceLevel = 0;

    if (firstLevel) {
        sourceWidth = width >> (firstLevel - 1);
        sourceHeight = height >> (firstLevel - 1);
        sourceLevel = firstLevel - 1;
    }

    for (uint32_t level = 1; level < levelCount; level++) {
        width = width >> 1 ? width >> 1 : 1;
        height = height >> 1 ? height >> 1 : 1;

        if (level >= firstLevel) {
            FullShrink(images->m_mip[level], width, height, images->m_mip[sourceLevel], sourceWidth, sourceHeight);
        }
    }
}

// VERIFY: 806f5e3e manual
// OFFSET: 0x004B55E0
void TextureFreeMippedImg(MipBits* bits) {
    if (bits) {
        SMemFree(bits, __FILE__, __LINE__, 0);
    }
}

// VERIFY: 118d04a4 auto
// OFFSET: 0x004B5600
TEXFILETYPE TextureDiscoverFileType(const char* fileName) {
    const char* extension = SStrChrR(const_cast<char*>(fileName), '.');

    if (!extension || SStrLen(extension) != 4) {
        return TEXFILETYPE_UNKNOWN;
    }

    if (!SStrCmpI(extension, ".TGA", STORM_MAX_STR)) {
        return TEXFILETYPE_TGA;
    }

    return SStrCmpI(extension, ".BLP", STORM_MAX_STR) ? TEXFILETYPE_UNKNOWN : TEXFILETYPE_BLP;
}

// VERIFY: 813fa654 manual
// OFFSET: 0x004B5670
TEXFILETYPE TexturePickAlternateFilename(const char* fileName, TEXFILETYPE type, char* buffer, uint32_t size) {
    if (fileName != buffer) {
        SStrCopy(buffer, fileName, size);
    }

    if (!type) {
        return TEXFILETYPE_UNKNOWN;
    }

    char* extension = SStrChrR(buffer, '.');

    if (extension) {
        *extension = '\0';
    }

    if (type == TEXFILETYPE_TGA) {
        SStrPack(buffer, ".BLP", size);
        return TEXFILETYPE_BLP;
    }

    if (type == TEXFILETYPE_BLP) {
        SStrPack(buffer, ".TGA", size);
        return TEXFILETYPE_TGA;
    }

    return type;
}

// VERIFY: a342abd9 auto
// OFFSET: 0x004B56F0
const char* TextureGetFilename(CTexture* texture) {
    if (!texture) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    return texture->m_filename;
}

// VERIFY: 2c853d09 manual
// OFFSET: 0x004B5710
int32_t TextureIsLoaded(CTexture* texture) {
    return texture && !texture->m_asyncObject;
}

// VERIFY: b492d3ae auto
// OFFSET: 0x004B5730
void TextureSetTextureSubstitutions(const char* from, const char* to) {
    s_substituteFrom = const_cast<char*>(from);
    s_substituteTo = const_cast<char*>(to);
}

// VERIFY: f55f749a auto
// OFFSET: 0x004B5750
void TextureGetGxTexFlags(CTexture* texture, CGxTexFlags* texFlags) {
    if (!texture) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    *texFlags = texture->m_gxTexFlags;
}

// VERIFY: 2f2ecfb3 auto
// OFFSET: 0x004B5770
uint32_t TextureGetBytesPerBlock(PIXEL_FORMAT format) {
    return s_pixelFormatBytesPerBlock[format];
}

// VERIFY: 4ee5ff1e auto
// OFFSET: 0x004B5780
int32_t TextureLoadBlob(const char* fileName) {
    return CTextureBlob::Load(fileName);
}

// VERIFY: 669249b5 auto
// OFFSET: 0x004B5790
int32_t TextureUnloadBlob(const char* fileName) {
    return CTextureBlob::Unload(fileName);
}

// VERIFY: 3e458c99 manual
// OFFSET: 0x004B57A0
void TextureComputeLoadProgress(CTexture* texture, int64_t& loaded, int64_t& total) {
    char* extension = texture->m_filename + SStrLen(texture->m_filename);
    strcpy(extension, ".blp");
    SFile::FileGetIsLocalAmount(texture->m_filename, loaded, total);
    total++;

    if (!texture->m_asyncObject) {
        loaded++;
    }

    *extension = '\0';
}

// VERIFY: d1f055eb auto
// OFFSET: 0x004B5800
int32_t TextureIsBlob(CTexture* texture) {
    return texture->m_asyncObject && texture->m_gxTex;
}

// VERIFY: 762ce4de manual
// OFFSET: 0x004B5930
void ResolveMipLevel(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    // STATIC: 0x00B4A1E8
    static uint32_t s_emptyTexel;

    if (command == GxTex_Recreate) {
        return;
    }

    CTextureAtlas* atlas = static_cast<CTextureAtlas*>(userArg);

    if (atlas->m_updateSlot == -1) {
        atlas->m_dirty |= 1;
        texelStrideInBytes = 0;
        texels = &s_emptyTexel;
        return;
    }

    CTexture* texture = atlas->m_textures[atlas->m_updateSlot];

    switch (command) {
        case GxTex_Lock:
            break;

        case GxTex_Latch:
            if (s_pumping) {
                if (atlas->m_format < GxTex_Dxt1 || atlas->m_format > GxTex_Dxt5) {
                    texelStrideInBytes = (s_texFormatBitDepth[texture->m_dataFormat] * atlas->m_width) >> 3;
                } else {
                    texelStrideInBytes = (atlas->m_width >> 2) * s_texFormatBytesPerBlock[atlas->m_dataFormat];

                    if (texelStrideInBytes < s_texFormatBytesPerBlock[texture->m_dataFormat]) {
                        texelStrideInBytes = s_texFormatBytesPerBlock[texture->m_dataFormat];
                    }
                }

                texels = s_mipBits[level].m_mip[0];
            }

            break;
    }
}

// VERIFY: 30b88d54 manual
// OFFSET: 0x004B5A00
uint32_t LoadPredrawnMips(CTgaFile* image, const char* format, int32_t flags, MipBits* images) {
    uint32_t level = 1;
    uint32_t count = TextureCalcMipCount(image->Width(), image->Height()) - 1;
    uint32_t width = image->Width() >> 1;
    uint32_t height = image->Height() >> 1;

    if (!count) {
        return level;
    }

    int32_t onDisk = flags != 0;

    do {
        char fileName[260];
        SStrPrintf(fileName, sizeof(fileName), format, level);

        if (!SFile::FileExistsEx(fileName, onDisk)) {
            break;
        }

        CTgaFile mip;

        if (!mip.Open(fileName, flags) || width != mip.Width() || height != mip.Height() || !mip.LoadImageData(2)) {
            return level;
        }

        if (!mip.AlphaBits()) {
            mip.AddAlphaChannel(nullptr);
        }

        mip.SetTopDown(1);

        const TGA32Pixel* source = mip.ImageTGA32Pixel();
        C4Pixel* dest = images->m_mip[level++];

        for (uint32_t pixels = width * height; pixels; pixels--) {
            dest->m_b = source->m_b;
            dest->m_g = source->m_g;
            dest->m_r = source->m_r;
            dest->m_a = source->m_a;
            dest++;
            source++;
        }

        if (width > 1) {
            width >>= 1;
        }

        if (height > 1) {
            height >>= 1;
        }
    } while (--count);

    return level;
}

// VERIFY: 7a812510 auto
// OFFSET: 0x004B5BB0
void RequestImageDimensions(uint32_t* width, uint32_t* height, uint32_t* bestMip) {
    CGxCaps caps = g_theGxDevicePtr->Caps();
    uint32_t maxSize = caps.m_texMaxSize[0];

    if (!maxSize) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    while (*width > maxSize || *height > maxSize) {
        *width >>= 1;
        *height >>= 1;
        (*bestMip)++;

        if (!*width) {
            *width = 1;
        }

        if (!*height) {
            *height = 1;
        }
    }
}

// VERIFY: b37c3e13 manual
// OFFSET: 0x004B5C30
int32_t TextureLoadBlpMips(char* fileName, char* extension, int32_t flags, MipBits** images, uint32_t* width, uint32_t* height, EGxTexFormat* gxTexFormat, int32_t* opaque, uint32_t* alphaBits, PIXEL_FORMAT* pixelFormat) {
    if (extension) {
        SStrCopy(extension, ".blp", STORM_MAX_STR);
    }

    CBLPFile image;

    if (!image.Open(fileName, flags)) {
        return 0;
    }

    EGxTexFormat format = GxTex_Argb8888;
    PIXEL_FORMAT pixels = PIXEL_ARGB8888;

    if (image.m_header.m_colorEncoding == COLOR_DXT) {
        pixels = static_cast<PIXEL_FORMAT>(image.m_header.m_preferredFormat);

        if (pixels == PIXEL_DXT1) {
            if (g_theGxDevicePtr->Caps().m_texFmt[GxTex_Dxt1]) {
                format = GxTex_Dxt1;
            } else if (image.m_header.m_alphaSize) {
                format = GxTex_Argb1555;
                pixels = PIXEL_ARGB1555;
            } else {
                format = GxTex_Rgb565;
                pixels = PIXEL_RGB565;
            }
        } else if (pixels == PIXEL_DXT3) {
            if (g_theGxDevicePtr->Caps().m_texFmt[GxTex_Dxt3]) {
                format = GxTex_Dxt3;
            } else {
                format = GxTex_Argb4444;
                pixels = PIXEL_ARGB4444;
            }
        } else if (pixels == PIXEL_DXT5) {
            if (g_theGxDevicePtr->Caps().m_texFmt[GxTex_Dxt5]) {
                format = GxTex_Dxt5;
            } else {
                format = GxTex_Argb4444;
                pixels = PIXEL_ARGB4444;
            }
        }
    }

    uint32_t imageWidth = image.m_header.m_width;
    uint32_t bestMip = 0;
    uint32_t imageHeight = image.m_header.m_height;
    RequestImageDimensions(&imageWidth, &imageHeight, &bestMip);

    if (width) {
        *width = imageWidth;
    }

    if (height) {
        *height = imageHeight;
    }

    if (gxTexFormat) {
        *gxTexFormat = format;
    }

    uint8_t alphaSize = image.m_header.m_alphaSize;

    if (opaque) {
        *opaque = image.m_header.m_alphaSize == 0;
    }

    if (alphaBits) {
        *alphaBits = alphaSize;
    }

    if (pixelFormat) {
        *pixelFormat = pixels;
    }

    if (!image.LockChain(pixels, *images, bestMip)) {
        return 0;
    }

    return 1;
}

// VERIFY: 363481eb manual
// OFFSET: 0x004B5E10
int32_t LoadBlpWithAlias(char* fileName, int32_t flags, MipBits** images) {
    char buffer[260];
    char* extension = &fileName[SStrLen(fileName)];
    extension[0] = '.';
    extension[1] = 'b';
    extension[2] = 'l';
    extension[3] = 'p';
    extension[4] = '\0';

    if (TextureBuildAliasPath(buffer, fileName)) {
        fileName = buffer;
    }

    int32_t result = TextureLoadBlpMips(fileName, nullptr, flags, images, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    *extension = '\0';

    return result;
}

// VERIFY: aac6b028 manual
// OFFSET: 0x004B5E80
void UpdateBlpTextureAsync(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    CTexture* texture = static_cast<CTexture*>(userArg);

    switch (command) {
        case GxTex_Lock:
            if (!s_pumping && !LoadBlpWithAlias(texture->m_filename, texture->m_flags & 2, &s_mipBits)) {
                GetDefaultTexture(width, height);
                GetGlobalStatusObj().Add(STATUS_ERROR, "Texture %s not loaded -- replaced with default.\n", texture->m_filename);
            }

            break;

        case GxTex_Latch: {
            if (texture->m_gxTexFormat < GxTex_Dxt1 || texture->m_gxTexFormat > GxTex_Dxt5) {
                texelStrideInBytes = (width * s_texFormatBitDepth[texture->m_dataFormat]) >> 3;
            } else {
                texelStrideInBytes = (width >> 2) * s_texFormatBytesPerBlock[texture->m_dataFormat];

                if (texelStrideInBytes < s_texFormatBytesPerBlock[texture->m_dataFormat]) {
                    texelStrideInBytes = s_texFormatBytesPerBlock[texture->m_dataFormat];
                }
            }

            const uint8_t* bits = reinterpret_cast<const uint8_t*>(s_mipBits[level].m_mip[0]);
            texels = bits;

            if (texture->m_gxTexTarget == GxTex_CubeMap) {
                uint32_t order = face;

                switch (face) {
                    case 0:
                        order = 0;
                        break;

                    case 1:
                        order = 2;
                        break;

                    case 2:
                        order = 4;
                        break;

                    case 3:
                        order = 5;
                        break;

                    case 4:
                        order = 3;
                        break;

                    case 5:
                        order = 1;
                        break;
                }

                texels = bits + order * texelStrideInBytes;
                texelStrideInBytes *= 6;
            }

            break;
        }

        default:
            break;
    }
}

// VERIFY: 85789b70 manual
// OFFSET: 0x004B5FE0
void TextureGetTextureFormats(PIXEL_FORMAT& pixelFormat, EGxTexFormat& gxTexFormat, PIXEL_FORMAT preferredFormat, uint32_t alphaBits) {
    switch (preferredFormat) {
        case PIXEL_DXT1:
            if (g_theGxDevicePtr->Caps().m_texFmt[GxTex_Dxt1]) {
                gxTexFormat = GxTex_Dxt1;
                pixelFormat = PIXEL_DXT1;
            } else if (alphaBits) {
                gxTexFormat = GxTex_Argb1555;
                pixelFormat = PIXEL_ARGB1555;
            } else {
                gxTexFormat = GxTex_Rgb565;
                pixelFormat = PIXEL_RGB565;
            }

            break;

        case PIXEL_DXT3:
            if (g_theGxDevicePtr->Caps().m_texFmt[GxTex_Dxt3]) {
                gxTexFormat = GxTex_Dxt3;
                pixelFormat = PIXEL_DXT3;
            } else {
                gxTexFormat = GxTex_Argb4444;
                pixelFormat = PIXEL_ARGB4444;
            }

            break;

        case PIXEL_ARGB8888:
            gxTexFormat = GxTex_Argb8888;
            pixelFormat = PIXEL_ARGB8888;
            break;

        case PIXEL_ARGB1555:
            gxTexFormat = GxTex_Argb1555;
            pixelFormat = PIXEL_ARGB8888;
            break;

        case PIXEL_ARGB4444:
            gxTexFormat = GxTex_Argb4444;
            pixelFormat = PIXEL_ARGB8888;
            break;

        case PIXEL_RGB565:
            gxTexFormat = GxTex_Rgb565;
            pixelFormat = PIXEL_ARGB8888;
            break;

        case PIXEL_DXT5:
            if (g_theGxDevicePtr->Caps().m_texFmt[GxTex_Dxt5]) {
                gxTexFormat = GxTex_Dxt5;
                pixelFormat = PIXEL_DXT5;
            } else {
                gxTexFormat = GxTex_Argb4444;
                pixelFormat = PIXEL_ARGB4444;
            }

            break;

        case PIXEL_UNSPECIFIED:
            if (!alphaBits) {
                gxTexFormat = GxTex_Rgb565;
                pixelFormat = PIXEL_ARGB8888;
            } else if (alphaBits == 1) {
                gxTexFormat = GxTex_Argb1555;
                pixelFormat = PIXEL_ARGB8888;
            } else {
                gxTexFormat = alphaBits == 4 ? GxTex_Argb4444 : GxTex_Argb8888;
                pixelFormat = PIXEL_ARGB8888;
            }

            break;

        default:
            break;
    }
}

// VERIFY: 7e1eb12e manual
// OFFSET: 0x004B6180
uint32_t TextureCacheGetMaxSize() {
    uint32_t size = 0x4000000;

    if (OsGetPhysicalMemory() <= 0x40000000) {
        size = 0x2000000;
    }

    return g_theGxDevicePtr->DeviceApi() == GxApi_D3d9Ex ? 0 : size;
}

// VERIFY: 3db7e996 auto
// OFFSET: 0x004B61C0
void TextureSetFilterMode(EGxTexFilter filter) {
    if (filter >= GxTex_LinearMipLinear && !g_theGxDevicePtr->Caps().m_texFilterTrilinear) {
        s_maxAnisotropy = 1;
        s_filterMode = GxTex_LinearMipNearest;
    } else if (filter >= GxTex_Anisotropic && !g_theGxDevicePtr->Caps().m_texFilterAnisotropic) {
        s_maxAnisotropy = 1;
        s_filterMode = GxTex_LinearMipLinear;
    } else {
        s_filterMode = filter;
    }
}

// VERIFY: dadff56b auto
// OFFSET: 0x004B6230
void TextureSetMaxAnisotropy(uint32_t maxAnisotropy) {
    if (s_filterMode != GxTex_Anisotropic) {
        s_maxAnisotropy = 1;
        return;
    }

    if (maxAnisotropy > g_theGxDevicePtr->Caps().m_maxTexAnisotropy) {
        maxAnisotropy = g_theGxDevicePtr->Caps().m_maxTexAnisotropy;
    }

    s_maxAnisotropy = maxAnisotropy;
}

// VERIFY: 930620f8 auto
// OFFSET: 0x004B6280
int32_t TextureGetRefCnt(CTexture* texture) {
    if (!texture) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    return texture->m_refcount;
}

// VERIFY: 6b2323f9 auto
// OFFSET: 0x004B62A0
int32_t TextureIsReadyNow(CTexture* texture) {
    if (!texture || !texture->m_gxTex) {
        return 0;
    }

    return g_theGxDevicePtr->TexIsReadyNow(texture->m_gxTex);
}

// VERIFY: ebac48aa auto
// OFFSET: 0x004B62D0
int32_t TextureCanUpdate(CTexture* texture) {
    if (!texture || !texture->m_gxTex) {
        return 0;
    }

    return g_theGxDevicePtr->TexCanUpdate(texture->m_gxTex);
}

// VERIFY: 835e236e auto
// OFFSET: 0x004B6300
uint32_t TextureCalcGxTexSize(EGxTexFormat format, uint32_t width, uint32_t height, EGxTexFilter filter) {
    uint32_t baseMip = g_theGxDevicePtr->DeviceBaseMipLevel();
    width >>= baseMip;
    height >>= baseMip;

    if (!width) {
        width = 1;
    }

    if (!height) {
        height = 1;
    }

    uint32_t texelsPerBlock = s_texFormatTexelsPerBlock[format];
    uint32_t bytesPerTexel = s_texFormatBytesPerTexel[format];
    uint32_t size = (texelsPerBlock + width - 1) / texelsPerBlock * ((texelsPerBlock + height - 1) / texelsPerBlock) * bytesPerTexel;

    if (filter > GxTex_Linear) {
        while (width != 1 || height != 1) {
            if (width > 1) {
                width >>= 1;
            }

            if (height > 1) {
                height >>= 1;
            }

            size += (texelsPerBlock + width - 1) / texelsPerBlock * ((texelsPerBlock + height - 1) / texelsPerBlock) * bytesPerTexel;
        }
    }

    return size;
}

// VERIFY: b532779f manual
// OFFSET: 0x004B63B0
void CTextureAtlas::Reload() {
    for (uint32_t i = 0; i < 64; i++) {
        CTexture* texture = this->m_textures[i];

        if (!texture) {
            continue;
        }

        s_pumping = 1;

        if (!LoadBlpWithAlias(texture->m_filename, texture->m_flags & 2, &s_mipBits)) {
            GetGlobalStatusObj().Add(STATUS_ERROR, "Texture %s not loaded.\n", texture->m_filename);
        }

        this->Update(texture);
        s_pumping = 0;
    }

    this->m_dirty &= ~1;
}

// VERIFY: 7f5ee904 manual
// OFFSET: 0x004B64E0
void AsyncTextureProcess(CAsyncObject* object, int32_t immediate) {
    object->m_link.Unlink();
    object->m_buffer = SMemAlloc(object->m_size, __FILE__, __LINE__, 0);
    s_asyncLoadBufferUsed += object->m_size;
    AsyncFileReadObject(object, immediate);
}

// VERIFY: 22d68fde auto
// OFFSET: 0x004B6550
void AsyncTextureWait(CTexture* texture) {
    CAsyncObject* object = texture->m_asyncObject;

    if (object) {
        if (!object->m_buffer) {
            AsyncTextureProcess(object, 1);
        }

        AsyncFileReadWait(texture->m_asyncObject);
    }
}

// VERIFY: 6e25a772 manual
// OFFSET: 0x004B6580
void TextureSetCacheSize(int32_t size) {
    uint32_t maxSize = 0x4000000;

    if (OsGetPhysicalMemory() <= 0x40000000) {
        maxSize = 0x2000000;
    }

    if (g_theGxDevicePtr->DeviceApi() == GxApi_D3d9Ex) {
        maxSize = 0;
    }

    if (size < 0) {
        s_gxTexCacheMaxSize = 0;
        return;
    }

    s_gxTexCacheMaxSize = maxSize;

    if (size <= static_cast<int32_t>(maxSize)) {
        s_gxTexCacheMaxSize = size;
    }
}

// VERIFY: 579a09eb auto
// OFFSET: 0x004B65E0
void TextureNotifyGxRestart() {
    for (CTextureAtlas* atlas = s_atlasList.Head(); atlas; atlas = atlas->m_link.Next()) {
        atlas->Reload();
    }
}

// VERIFY: 7a43e0c3 auto
// OFFSET: 0x004B6610
int32_t TextureGetDimensions(CTexture* texture, uint32_t* width, uint32_t* height, int32_t wait) {
    CAsyncObject* object = texture->m_asyncObject;

    if (object) {
        if (!wait) {
            return 0;
        }

        if (!object->m_buffer) {
            AsyncTextureProcess(object, 1);
        }

        AsyncFileReadWait(texture->m_asyncObject);
    }

    if (width) {
        *width = texture->m_gxWidth;
    }

    if (height) {
        *height = texture->m_gxHeight;
    }

    return 1;
}

// VERIFY: b2a5ecf0 manual
// OFFSET: 0x004B6700
CTextureAtlas::CTextureAtlas() {
    this->m_slots = 64;
    this->m_count = 0;
    this->m_dirty = 0;
    this->m_width = 0;
    this->m_height = 0;
    this->m_capacity = 0;
    this->m_used = 0;
    this->m_freeSlot = -1;
    this->m_gxTex = nullptr;
    this->m_updateSlot = -1;
    memset(this->m_textures, 0, sizeof(this->m_textures));
}

// VERIFY: 00142d3e manual
// OFFSET: 0x004B6760
CGxTex* TextureAllocGxTex(EGxTexTarget target, uint32_t width, uint32_t height, uint32_t depth, EGxTexFormat format, CGxTexFlags flags, void* userArg, GxTexUserFunc userFunc, EGxTexFormat dataFormat, const char* name) {
    CGxTexParms parms;
    parms.m_target = target;
    parms.m_width = width;
    parms.m_height = height;
    parms.m_depth = depth;
    parms.m_format = format;
    parms.m_dataFormat = dataFormat;
    parms.m_flags = flags;
    parms.m_flags.m_generateMipMaps = 0;
    parms.m_userArg = userArg;
    parms.m_userFunc = userFunc;

    if (GxTexReusable(parms) && width - 32 <= 480 && height - 32 <= 480) {
        uint32_t w = width >> 5;
        uint32_t h = height >> 5;
        uint32_t x = 0;
        uint32_t y = 0;

        if (!(width & 0x20)) {
            do {
                w >>= 1;
                x++;
            } while (!(w & 1));
        }

        if (!(height & 0x20)) {
            do {
                h >>= 1;
                y++;
            } while (!(h & 1));
        }

        CGxTexParms cached;
        TSExplicitList<CGxTexCache, offsetof(CGxTexCache, m_link)>& list = s_gxTexCache[x][y];

        for (CGxTexCache* entry = list.Head(); entry; entry = list.Next(entry)) {
            GxTexParameters(entry->m_gxTex, cached);

            if (parms.m_flags.m_filter == cached.m_flags.m_filter && cached.m_format == parms.m_format && cached.m_target == parms.m_target && cached.m_depth == parms.m_depth) {
                s_gxTexCacheSize -= entry->m_size;
                entry->m_link.Unlink();
                CGxTex* gxTex = entry->m_gxTex;
                entry->m_gxTex = nullptr;
                s_gxTexCacheFree.LinkToHead(entry);
                GxTexSetDataFormat(gxTex, parms.m_dataFormat);
                GxTexSetUserData(gxTex, parms.m_userFunc, parms.m_userArg);
                GxTexSetFlags(gxTex, parms.m_flags);

                return gxTex;
            }
        }
    }

    CGxTex* gxTex = nullptr;
    GxTexCreate(parms, gxTex);

    return gxTex;
}

// VERIFY: ead404bb manual
// OFFSET: 0x004B6920
void FillInSolidTexture(const CImVector& color, CTexture* texture) {
    texture->m_gxTex = TextureAllocGxTex(GxTex_2d, 8, 8, 0, GxTex_Argb8888, CGxTexFlags(), reinterpret_cast<void*>(static_cast<uintptr_t>(*reinterpret_cast<const uint32_t*>(&color))), GxuUpdateSingleColorTexture, GxTex_Argb8888, texture->m_filename);

    if (color.a < 0xFE) {
        texture->m_flags &= ~1;
    } else {
        texture->m_flags |= 1;
    }

    texture->m_dataFormat = GxTex_Argb8888;
    texture->m_gxTexFormat = GxTex_Argb8888;
    texture->m_gxTexTarget = GxTex_2d;
    texture->m_gxWidth = 8;
    texture->m_gxHeight = 8;
    texture->m_gxTexFlags = CGxTexFlags();
    SStrCopy(texture->m_filename, "SolidTexture", 260);
}

// VERIFY: f77dd4dd manual
// OFFSET: 0x004B69E0
void AsyncTextureHandler() {
    uint32_t available = 0x400000 - s_asyncLoadBufferUsed;

    for (CAsyncObject* object = s_asyncLoadList.Head(); !s_asyncLoadList.IsEnd(object); ) {
        CAsyncObject* next = s_asyncLoadList.RawNext(object);

        if (available >= object->m_size) {
            AsyncTextureProcess(object, 0);
            available -= object->m_size;
        }

        object = next;
    }

    for (CAsyncObject* object = s_asyncTextureList.Head(); !s_asyncTextureList.IsEnd(object); ) {
        CAsyncObject* next = s_asyncTextureList.RawNext(object);

        if (available >= object->m_size) {
            AsyncTextureProcess(object, 0);
            available -= object->m_size;
        }

        object = next;
    }
}

// VERIFY: 7872daab manual
// OFFSET: 0x004B6AE0
void TextureCacheUpdate() {
    int32_t destroyed = 0;
    int32_t pressure;

    if (!s_gxTexCacheMaxSize) {
        pressure = 1;
    } else if (s_gxTexCacheSize > 2 * s_gxTexCacheMaxSize) {
        pressure = 2;
    } else if (s_gxTexCacheSize > s_gxTexCacheMaxSize) {
        pressure = 1;
    } else {
        return;
    }

    int32_t maxDestroyed = pressure > 1 ? 32 : 16;

    for (int32_t x = 0; x < 6 && s_gxTexCacheSize > s_gxTexCacheMaxSize; x++) {
        for (int32_t y = 0; y < 6 && s_gxTexCacheSize > s_gxTexCacheMaxSize; y++) {
            CGxTexCache* entry = s_gxTexCache[x][y].Head();

            while (entry && destroyed < maxDestroyed && s_gxTexCacheSize > s_gxTexCacheMaxSize) {
                s_gxTexCacheSize -= entry->m_size;
                GxTexDestroy(entry->m_gxTex);
                destroyed++;
                entry->m_gxTex = nullptr;
                entry->m_size = 0;
                entry->m_time = 0xFFFFFFFF;

                CGxTexCache* next = entry->m_link.Next();
                entry->m_link.Unlink();
                s_gxTexCacheFree.LinkToHead(entry);
                entry = next;
            }
        }
    }
}

// VERIFY: 04d8356e auto
// OFFSET: 0x004B6C50
void TextureIncreasePriority(CTexture* texture) {
    if (!SFile::IsStreamingMode()) {
        return;
    }

    if (texture->m_asyncObject->m_buffer) {
        AsyncFile::EnterQueueLock();
        CAsyncObject* object = texture->m_asyncObject;

        if (!object->m_isCurrent && !object->m_isRead && !object->m_isProcessed) {
            AsyncFileReadIncreasePriority(texture->m_asyncObject);
        }

        AsyncFile::LeaveQueueLock();
    } else {
        s_asyncLoadList.LinkToHead(texture->m_asyncObject);
    }
}

// VERIFY: 3c936638 auto
// OFFSET: 0x004B6CB0
CGxTex* TextureGetGxTex(CTexture* texture, int32_t wait, CStatus* status) {
    if (!texture) {
        DebugValidate("textureptr");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    if (texture->m_flags & 4) {
        if (texture->m_asyncObject) {
            if (wait != 1 && (wait != 2 || texture->m_asyncObject->m_char24)) {
                TextureIncreasePriority(texture);
                return nullptr;
            }

            AsyncTextureWait(texture);

            if (status) {
                status->Add(texture->m_status);
            }
        }

        if (texture->m_atlas) {
            if (texture->m_atlas->m_dirty & 1) {
                texture->m_atlas->Reload();
            }

            return texture->m_atlas->m_gxTex;
        }
    }

    if (texture->m_asyncObject) {
        TextureIncreasePriority(texture);
    }

    if (!texture->m_gxTex) {
        if (wait != 1 && (wait != 2 || texture->m_asyncObject->m_char24)) {
            return nullptr;
        }

        AsyncTextureWait(texture);

        if (status) {
            status->Add(texture->m_status);
        }
    }

    return texture->m_gxTex;
}

// VERIFY: 9761fb06 manual
// OFFSET: 0x004B6E90
CTextureAtlas* CTextureAtlas::Alloc(uint32_t width, uint32_t height, EGxTexFormat format, EGxTexFormat dataFormat, CGxTexFlags flags) {
    CTextureAtlas* atlas = new (__FILE__, __LINE__) CTextureAtlas;
    atlas->m_height = height;
    atlas->m_width = width;
    atlas->m_used = 0;
    atlas->m_freeSlot = -1;
    atlas->m_capacity = 64;
    atlas->m_gxTex = TextureAllocGxTex(GxTex_2d, 8 * width, 8 * height, 0, format, flags, atlas, ResolveMipLevel, dataFormat, "Atlas");
    atlas->m_dataFormat = dataFormat;
    atlas->m_format = format;
    atlas->m_texFlags = flags;

    return atlas;
}

// VERIFY: a49b1a09 auto
// OFFSET: 0x004B6F30
CTexture* TextureCacheGetTexture(const char* fileName) {
    uint32_t hashval = SStrHashHT(fileName);
    HASHKEY_TEXTUREFILE key;
    key.m_anyTexFlags = 1;
    key.m_filename = const_cast<char*>(fileName);
    CTexture* texture = s_textureCache.Ptr(hashval, key);

    if (!texture) {
        return nullptr;
    }

    return reinterpret_cast<CTexture*>(HandleCreate(texture, "HTEXTURE"));
}

// VERIFY: 841c7f74 auto
// OFFSET: 0x004B6FA0
CTexture* TextureCacheGetTexture(char* fileName, char* extension, CGxTexFlags texFlags) {
    if (extension) {
        *extension = '\0';
    }

    uint32_t hashval = SStrHashHT(fileName);
    HASHKEY_TEXTUREFILE key;
    key.m_filename = fileName;
    key.m_texFlags = texFlags;
    CTexture* texture = s_textureCache.Ptr(hashval, key);

    if (extension) {
        *extension = '.';
    }

    if (!texture) {
        return nullptr;
    }

    return reinterpret_cast<CTexture*>(HandleCreate(texture, "HTEXTURE"));
}

// VERIFY: 028fd060 auto
// OFFSET: 0x004B7020
CTexture* TextureCacheGetTexture(const CImVector& color) {
    CGxTexFlags texFlags;
    HASHKEY_TEXTUREFILE key;
    key.m_filename = const_cast<char*>("");
    key.m_texFlags = texFlags;
    CTexture* texture = s_textureCache.Ptr(*reinterpret_cast<const uint32_t*>(&color), key);

    if (!texture) {
        return nullptr;
    }

    return reinterpret_cast<CTexture*>(HandleCreate(texture, "HTEXTURE"));
}

// VERIFY: fb7f7312 manual
// OFFSET: 0x004B70A0
void TextureFreeGxTex(CGxTex* gxTex, const char* name) {
    CGxTexParms parms;
    GxTexParameters(gxTex, parms);

    if (GxTexReusable(gxTex) && parms.m_width - 32 <= 480 && parms.m_height - 32 <= 480) {
        uint32_t w = parms.m_width >> 5;
        uint32_t h = parms.m_height >> 5;
        uint32_t x = 0;
        uint32_t y = 0;

        if (!(parms.m_width & 0x20)) {
            do {
                w >>= 1;
                x++;
            } while (!(w & 1));
        }

        if (!(parms.m_height & 0x20)) {
            do {
                h >>= 1;
                y++;
            } while (!(h & 1));
        }

        CGxTexCache* entry = s_gxTexCacheFree.Head();

        if (!entry) {
            entry = s_gxTexCacheFree.NewNode(1, 0, 0);
        }

        entry->m_link.Unlink();
        entry->m_gxTex = gxTex;
        entry->m_time = s_currentTime;
        entry->m_size = TextureCalcGxTexSize(parms.m_format, parms.m_width, parms.m_height, static_cast<EGxTexFilter>(parms.m_flags.m_filter));
        s_gxTexCache[x][y].LinkToTail(entry);
        s_gxTexCacheSize += entry->m_size;
        GxTexSetUserData(entry->m_gxTex, GxTexNullUserFunc, nullptr);
    } else {
        TextureCalcGxTexSize(parms.m_format, parms.m_width, parms.m_height, static_cast<EGxTexFilter>(parms.m_flags.m_filter));
        GxTexDestroy(gxTex);
    }
}

// VERIFY: c22c0284 auto
// OFFSET: 0x004B7200
int32_t TextureUpdate(const void* data, void* param) {
    s_currentTime = OsGetAsyncTimeMs();
    TextureCacheUpdate();

    return 1;
}

// VERIFY: fea7fb26 manual
// OFFSET: 0x004B7220
MipBits* TextureAllocMippedImg(PIXEL_FORMAT format, uint32_t width, uint32_t height) {
    int32_t cache = s_pixelFormatToMipBitsCache[format];

    if (width < 8 || height < 8 || width > 256 || height > 256 || cache == -1) {
        return MippedImgAllocA(format, width, height, __FILE__, __LINE__);
    }

    uint32_t w = width >> 3;
    uint32_t h = height >> 3;
    uint32_t x = 0;
    uint32_t y = 0;

    if (!(width & 8)) {
        do {
            w >>= 1;
            x++;
        } while (!(w & 1));
    }

    if (!(height & 8)) {
        do {
            h >>= 1;
            y++;
        } while (!(h & 1));
    }

    CMipBitsCache* entry = s_mipBitsCache[x][y][cache].Head();

    if (!entry) {
        return MippedImgAllocA(format, width, height, __FILE__, __LINE__);
    }

    entry->m_link.Unlink();
    MipBits* bits = entry->m_bits;
    entry->m_bits = nullptr;
    s_mipBitsCacheFree.LinkToTail(entry);

    return bits;
}

// VERIFY: 8f6a0308 manual
// OFFSET: 0x004B7300
void TextureFreeMippedImg(MipBits* bits, PIXEL_FORMAT format, uint32_t width, uint32_t height) {
    int32_t cache = s_pixelFormatToMipBitsCache[format];

    if (width < 8 || height < 8 || width > 256 || height > 256 || cache == -1) {
        if (bits) {
            SMemFree(bits, __FILE__, __LINE__, 0);
        }

        return;
    }

    uint32_t w = width >> 3;
    uint32_t h = height >> 3;
    uint32_t x = 0;
    uint32_t y = 0;

    if (!(width & 8)) {
        do {
            w >>= 1;
            x++;
        } while (!(w & 1));
    }

    if (!(height & 8)) {
        do {
            h >>= 1;
            y++;
        } while (!(h & 1));
    }

    CMipBitsCache* entry = s_mipBitsCacheFree.Head();

    if (!entry) {
        entry = s_mipBitsCacheFree.NewNode(2, 0, 0);
        entry->m_link.Unlink();
    }

    entry->m_bits = bits;
    s_mipBitsCache[x][y][cache].LinkToTail(entry);
}

// VERIFY: a81fc586 manual
// OFFSET: 0x004B76E0
CTextureAtlas::~CTextureAtlas() {
    if (this->m_gxTex) {
        TextureFreeGxTex(this->m_gxTex, "ATLAS");
    }

    this->m_link.Unlink();
}

// VERIFY: cb110af1 manual
// OFFSET: 0x004B7770
CTextureAtlas* CTextureAtlas::Get(CTexture* texture) {
    uint16_t width = texture->m_gxWidth;
    uint16_t height = texture->m_gxHeight;

    if (width != height || width > 64 || height > 64) {
        return nullptr;
    }

    CGxTexFlags texFlags = texture->m_gxTexFlags;
    texFlags.m_bit15 = 1;
    CTextureAtlas* best = nullptr;

    for (CTextureAtlas* atlas = s_atlasList.Head(); atlas; atlas = atlas->m_link.Next()) {
        if (atlas->m_format == texture->m_gxTexFormat && atlas->m_texFlags == texFlags && atlas->m_dataFormat == texture->m_dataFormat && atlas->m_width == width && atlas->m_height == height) {
            if (static_cast<int32_t>(atlas->m_used) < static_cast<int32_t>(atlas->m_capacity) && (!best || static_cast<int32_t>(atlas->m_used) > static_cast<int32_t>(best->m_used))) {
                best = atlas;
            }
        }
    }

    CTextureAtlas* atlas = best;

    if (!atlas) {
        atlas = CTextureAtlas::Alloc(width, height, texture->m_gxTexFormat, texture->m_dataFormat, texFlags);
        s_atlasList.LinkToTail(atlas);
    }

    texture->m_atlasBlockIndex = -1;

    if (atlas->m_freeSlot == -1) {
        for (uint32_t i = 0; i < 64; i++) {
            if (!atlas->m_textures[i]) {
                texture->m_atlasBlockIndex = i;
                break;
            }
        }
    } else {
        texture->m_atlasBlockIndex = atlas->m_freeSlot;
        atlas->m_freeSlot = -1;
    }

    atlas->m_used++;
    atlas->m_count++;
    atlas->m_textures[texture->m_atlasBlockIndex] = texture;

    return atlas;
}

// VERIFY: b9a802ce manual
// OFFSET: 0x004B78A0
int32_t LoadTgaMips(char* fileName, char* extension, int32_t flags, MipBits** images, uint32_t* width, uint32_t* height, EGxTexFormat* gxTexFormat, int32_t* opaque, uint32_t* alphaBits, PIXEL_FORMAT* pixelFormat) {
    if (extension) {
        strcpy(extension, ".tga");
    }

    CTgaFile image;

    if (!image.Open(fileName, flags)) {
        return 0;
    }

    if (opaque) {
        *opaque = image.AlphaBits() == 0;
    }

    if (!image.LoadImageData(3)) {
        return 0;
    }

    image.SetTopDown(1);

    uint32_t imageWidth = image.Width();
    uint32_t imageHeight = image.Height();
    uint32_t mipCount = TextureCalcMipCount(imageWidth, imageHeight);

    if (*images) {
        MippedImgSet(PIXEL_ARGB8888, imageWidth, imageHeight, *images);
    } else {
        *images = TextureAllocMippedImg(PIXEL_ARGB8888, imageWidth, imageHeight);
    }

    const TGA32Pixel* source = image.ImageTGA32Pixel();
    C4Pixel* dest = (*images)->m_mip[0];

    for (uint32_t pixels = imageWidth * imageHeight; pixels; pixels--) {
        dest->m_b = source->m_b;
        dest->m_g = source->m_g;
        dest->m_r = source->m_r;
        dest->m_a = source->m_a;
        dest++;
        source++;
    }

    image.Close();

    char mipName[260];
    SStrCopy(mipName, fileName, sizeof(mipName));
    char* dot = SStrChrR(mipName, '.');

    if (dot) {
        *dot = '\0';
    }

    SStrPack(mipName, "_mip%d.tga", sizeof(mipName));
    uint32_t predrawn = LoadPredrawnMips(&image, mipName, flags, *images);
    TextureGenerateMips(image.Width(), image.Height(), predrawn, mipCount, *images);

    if (width) {
        *width = image.Width();
    }

    if (height) {
        *height = image.Height();
    }

    if (gxTexFormat) {
        *gxTexFormat = GxTex_Argb8888;
    }

    if (alphaBits) {
        *alphaBits = image.AlphaBits();
    }

    if (pixelFormat) {
        *pixelFormat = PIXEL_ARGB8888;
    }

    return 1;
}

// VERIFY: 8ee83473 manual
// OFFSET: 0x004B7AA0
void UpdateTgaTexture(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    CTexture* texture = static_cast<CTexture*>(userArg);

    switch (command) {
        case GxTex_Lock:
            if (!s_pumping) {
                char* extension = texture->m_filename + SStrLen(texture->m_filename);
                int32_t result = LoadTgaMips(texture->m_filename, extension, texture->m_flags & 2, &s_mipBits, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
                *extension = '\0';

                if (!result) {
                    GetDefaultTexture(width, height);
                    GetGlobalStatusObj().Add(STATUS_ERROR, "Texture %s not loaded -- replaced with default.\n", texture->m_filename);
                }
            }

            break;

        case GxTex_Latch: {
            texelStrideInBytes = 4 * width;

            const uint8_t* bits = reinterpret_cast<const uint8_t*>(s_mipBits[level].m_mip[0]);
            texels = bits;

            if (texture->m_gxTexTarget == GxTex_CubeMap) {
                uint32_t order = face;

                switch (face) {
                    case 0:
                        order = 0;
                        break;

                    case 1:
                        order = 2;
                        break;

                    case 2:
                        order = 4;
                        break;

                    case 3:
                        order = 5;
                        break;

                    case 4:
                        order = 3;
                        break;

                    case 5:
                        order = 1;
                        break;
                }

                texels = bits + order * texelStrideInBytes;
                texelStrideInBytes *= 6;
            }

            break;
        }

        default:
            break;
    }
}

// VERIFY: c0a503e5 manual
// OFFSET: 0x004B7BD0
int32_t PumpBlpTextureAsync(CTexture* texture, void* data) {
    CBLPFile image;

    if (!image.Source(data)) {
        texture->m_status.Add(STATUS_FATAL, "BLP Texture failure: \"%s\" invalid file version\n", texture->m_filename);
        return 0;
    }

    if ((texture->m_flags & 4) && !(image.m_header.m_hasMips & 0x10)) {
        texture->m_flags &= ~4;
    }

    texture->m_alphaBits = image.m_header.m_alphaSize;

    if (!image.m_header.m_alphaSize) {
        texture->m_flags |= 1;
    }

    uint32_t width = image.m_header.m_width;
    uint32_t bestMip = 0;
    uint32_t height = image.m_header.m_height;
    RequestImageDimensions(&width, &height, &bestMip);
    texture->m_bestMip = bestMip;

    PIXEL_FORMAT pixelFormat;
    EGxTexFormat gxTexFormat;
    TextureGetTextureFormats(pixelFormat, gxTexFormat, static_cast<PIXEL_FORMAT>(image.m_header.m_preferredFormat), image.m_header.m_alphaSize);

    s_pumping = 1;

    if (!image.LockChain2(texture->m_filename, pixelFormat, s_mipBits, texture->m_bestMip, 1)) {
        s_pumping = 0;
        texture->m_status.Add(STATUS_FATAL, "BLP Texture failure: \"%s\" decompression failed.\n", texture->m_filename);
        return 0;
    }

    uint32_t faceWidth = width;
    EGxTexTarget target = GxTex_2d;

    if (width == 6 * height) {
        target = GxTex_CubeMap;
        faceWidth = width / 6;
    }

    texture->m_gxHeight = height;
    texture->m_dataFormat = s_pixelFormatToGxTexFormat[pixelFormat];
    texture->m_gxTexTarget = target;
    texture->m_gxWidth = faceWidth;
    texture->m_gxTexFormat = gxTexFormat;

    if (faceWidth < 256 && image.m_numLevels == 1 && !texture->m_gxTexFlags.m_generateMipMaps) {
        texture->m_gxTexFlags.m_filter = GxTex_Nearest;
    }

    if (texture->m_flags & 4) {
        texture->m_atlas = CTextureAtlas::Get(texture);

        if (texture->m_atlas) {
            texture->m_atlas->Update(texture);
        } else {
            texture->m_flags &= ~4;
        }
    }

    if (!texture->m_atlas) {
        if (texture->m_gxTex) {
            TextureFreeGxTex(texture->m_gxTex, texture->m_filename);
            texture->m_gxTex = nullptr;
        }

        texture->m_gxTex = TextureAllocGxTex(texture->m_gxTexTarget, texture->m_gxWidth, texture->m_gxHeight, 0, texture->m_gxTexFormat, texture->m_gxTexFlags, texture, UpdateBlpTextureAsync, texture->m_dataFormat, texture->m_filename);

        if (!texture->m_gxTex) {
            s_pumping = 0;
            texture->m_status.Add(STATUS_FATAL, "BLP Texture failure: \"%s\" allocating %dx%d texture failed.\n", texture->m_filename, faceWidth, height);
            return 0;
        }

        GxTexUpdate(texture->m_gxTex, 0, 0, faceWidth, height, 1);
    }

    s_pumping = 0;

    return 1;
}

// VERIFY: 85a7ce70 manual
// OFFSET: 0x004B7E80
void AsyncTextureSuccessCallback(CTexture* texture) {
    if (!PumpBlpTextureAsync(texture, texture->m_asyncObject->m_buffer)) {
        if (!texture->m_status.IsEmpty()) {
            SysMsgAdd(&texture->m_status);
        }

        FillInSolidTexture(*reinterpret_cast<const CImVector*>(&s_defaultColor), texture);
    }

    SFileCloseFile(texture->m_asyncObject->m_file);
    texture->m_asyncObject->m_file = nullptr;
    s_asyncLoadBufferUsed -= texture->m_asyncObject->m_size;
    SMemFree(texture->m_asyncObject->m_buffer, __FILE__, __LINE__, 0);
    AsyncFileReadDestroyObject(texture->m_asyncObject);
    texture->m_asyncObject = nullptr;
}

// VERIFY: f4bfd60d manual
// OFFSET: 0x004B7F10
int32_t AsyncTextureStatus() {
    int32_t count = 0;

    for (CAsyncObject* object = s_asyncLoadList.Head(); !s_asyncLoadList.IsEnd(object); object = s_asyncLoadList.RawNext(object)) {
        count++;
    }

    for (CAsyncObject* object = s_asyncTextureList.Head(); !s_asyncTextureList.IsEnd(object); object = s_asyncTextureList.RawNext(object)) {
        count++;
    }

    return count;
}

// VERIFY: f4a9a3e6 manual
// OFFSET: 0x004B7F80
void TextureInitialize() {
    s_mipBits = static_cast<MipBits*>(SMemAlloc(MippedImgCalcSize(PIXEL_ARGB8888, 1024, 1024), __FILE__, __LINE__, 0));
    AsyncFileReadAddPollHandler(AsyncTextureHandler);
    AsyncFileReadAddStatusHandler(AsyncTextureStatus);
    s_currentTime = OsGetAsyncTimeMs();
    s_gxTexCacheSize = 0;
    s_gxTexCacheMaxSize = 0;
    EventRegisterEx(EVENT_ID_POLL, TextureUpdate, nullptr, EVENT_PRIORITY_NORMAL);
}

// VERIFY: 7711ba71 manual
// OFFSET: 0x004B8000
void TextureGxCacheClear() {
    for (int32_t x = 0; x < 6; x++) {
        for (int32_t y = 0; y < 6; y++) {
            for (CGxTexCache* entry = s_gxTexCache[x][y].Head(); !s_gxTexCache[x][y].IsEnd(entry); entry = entry->m_link.RawNext()) {
                s_gxTexCacheSize -= entry->m_size;
            }

            s_gxTexCache[x][y].Clear();
        }
    }
}

// VERIFY: f2cfa4ab auto
// OFFSET: 0x004B8060
void TextureGxCacheFlush() {
    TextureGxCacheClear();
    s_gxTexCacheFree.Clear();
}

// VERIFY: 2bc4b3ae manual
// OFFSET: 0x004B8070
int32_t LoadBlpMips(char* fileName, char* extension, int32_t flags, MipBits** images, uint32_t* width, uint32_t* height, int32_t* opaque, uint32_t* alphaBits, PIXEL_FORMAT* pixelFormat) {
    if (extension) {
        strcpy(extension, ".blp");
    }

    CBLPFile image;

    if (!image.Open(fileName, flags)) {
        return 0;
    }

    uint32_t alphaSize = image.m_header.m_alphaSize;
    uint32_t imageWidth = image.m_header.m_width;
    uint32_t imageHeight = image.m_header.m_height;
    PIXEL_FORMAT format;

    if (!pixelFormat || (format = *pixelFormat) == PIXEL_UNSPECIFIED) {
        if (image.m_header.m_alphaSize) {
            format = image.m_header.m_alphaSize != 1 ? PIXEL_ARGB4444 : PIXEL_ARGB1555;
        } else {
            format = PIXEL_RGB565;
        }
    }

    *images = TextureAllocMippedImg(format, image.m_header.m_width, image.m_header.m_height);

    if (!image.LockChain2(fileName, format, *images, 0, 0)) {
        return 0;
    }

    if (width) {
        *width = imageWidth;
    }

    if (height) {
        *height = imageHeight;
    }

    if (opaque) {
        *opaque = alphaSize == 0;
    }

    if (alphaBits) {
        *alphaBits = alphaSize;
    }

    if (pixelFormat) {
        *pixelFormat = format;
    }

    return 1;
}

// VERIFY: 604826aa manual
// OFFSET: 0x004B81D0
MipBits* TextureLoadImage(const char* fileName, uint32_t* width, uint32_t* height, PIXEL_FORMAT* dataFormat, int32_t* opaque, CStatus* status, uint32_t* alphaBits, int32_t flags) {
    if (!fileName) {
        DebugValidate("filename");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    if (!width) {
        DebugValidate("width");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    if (!height) {
        DebugValidate("height");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    if (!dataFormat) {
        DebugValidate("dataFormat");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    DebugLog("TextureLoadImage() blocking load: %s.\n", fileName);

    MipBits* images = nullptr;
    char name[260];
    strcpy(name, fileName);
    char* extension = OsPathFindExtensionWithDot(name);
    *extension = '\0';

    int32_t type = 1;

    for (int32_t tries = 0; tries < 2; tries++) {
        if (type == 0) {
            LoadTgaMips(name, extension, flags, &images, width, height, nullptr, opaque, alphaBits, dataFormat);
        } else if (type == 1) {
            LoadBlpMips(name, extension, flags, &images, width, height, opaque, alphaBits, dataFormat);
        }

        if (images) {
            return images;
        }

        type = (type + 1) % 2;
    }

    if (status) {
        status->Add(STATUS_FATAL, "Error loading texure file \"%s\": unsupported image format\n", fileName);
    }

    return images;
}

// VERIFY: 83c488d3 manual
// OFFSET: 0x004B8420
void TextureDestroy() {
    CTextureBlob::UnloadAll();
    TextureGxCacheClear();
    s_gxTexCacheFree.Clear();
    SMemFree(s_mipBits, __FILE__, __LINE__, 0);
    s_mipBits = nullptr;

    for (int32_t x = 0; x < 6; x++) {
        for (int32_t y = 0; y < 6; y++) {
            s_mipBitsCache[x][y][0].Clear();
            s_mipBitsCache[x][y][1].Clear();
        }
    }

    s_mipBitsCacheFree.Clear();

    for (CTexture* texture = s_textureList.Head(); !s_textureList.IsEnd(texture); texture = s_textureList.RawNext(texture)) {
        DebugLog("TextureDestroy() found unfreed texture: %s\n", texture->m_filename);
    }

    EventUnregisterEx(EVENT_ID_POLL, TextureUpdate, nullptr, 0xFFFFFFFF);
}

// VERIFY: 3e889496 manual
// OFFSET: 0x004B8720
void CTextureAtlas::Free(CTextureAtlas* atlas, CTexture* texture) {
    atlas->m_textures[texture->m_atlasBlockIndex] = nullptr;
    atlas->m_count--;
    atlas->m_used--;
    atlas->m_freeSlot = texture->m_atlasBlockIndex;

    if (static_cast<int16_t>(atlas->m_count) < 1) {
        delete atlas;
    }

    texture->m_atlasBlockIndex = -1;
}

// VERIFY: 0882c3ed manual
// OFFSET: 0x004B8770
CTexture::CTexture() {
    this->m_flags = 0;
    this->m_bestMip = 0;
    this->m_alphaBits = 0;
    this->m_asyncObject = nullptr;
    this->m_gxTex = nullptr;
    this->m_gxTexTarget = GxTex_2d;
    this->m_gxWidth = 0;
    this->m_gxHeight = 0;
    this->m_gxTexFormat = GxTex_Unknown;
    this->m_dataFormat = GxTex_Unknown;
    this->m_atlas = nullptr;
    this->m_atlasBlockIndex = 0;
    this->m_filename[0] = '\0';
    s_textureList.LinkToTail(this);
}

// VERIFY: 2820aa71 auto
// OFFSET: 0x004B8820
const char* CTexture::GetObjectName() {
    return this->m_filename;
}

// VERIFY: e23c2bfb manual
// OFFSET: [0x004B8830, 0x004B91D0]
CTexture::~CTexture() {
    if (this->m_gxTex) {
        TextureFreeGxTex(this->m_gxTex, this->m_filename);
    }

    if (this->m_atlas) {
        CTextureAtlas::Free(this->m_atlas, this);
    }

    if (this->m_asyncObject) {
        if (this->m_flags & 0x20) {
            this->m_asyncObject->m_userArg = this->m_asyncObject;
            this->m_asyncObject->m_userPostUnloadCallback = reinterpret_cast<void*>(AsyncTextureDestroyedCallback);
            this->m_asyncObject->m_userPostloadCallback = this->m_asyncObject->m_userPostUnloadCallback;
        } else {
            void* buffer = this->m_asyncObject->m_buffer;

            if (AsyncFile::Cancel(this->m_asyncObject, AsyncTextureDestroyedCallback)) {
                s_asyncLoadBufferUsed -= this->m_asyncObject->m_size;
                SMemFree(buffer, __FILE__, __LINE__, 0);
            }
        }
    }
}

// VERIFY: 996bc4c4 manual
// OFFSET: 0x004B8910
CTexture* CreateBlpSync(const char* fileName, char* extension, uint32_t flags, CGxTexFlags texFlags) {
    SFileImpl* file = nullptr;
    SErrSetLastError(0);

    if (!SFile::OpenEx(nullptr, fileName, (flags >> 1) & 1, &file)) {
        if (!SErrGetLastError()) {
            SErrSetLastError(STORM_ERROR_FILE_NOT_FOUND);
        }

        return nullptr;
    }

    if (!file) {
        return nullptr;
    }

    CTexture* texture = new ("HTEXTURE", SLOG_OBJECT) CTexture;
    texture->m_gxTexFlags = texFlags;

    if (flags & 2) {
        texture->m_flags |= 2;
    }

    if ((flags & 4) && s_atlasEnable) {
        texture->m_flags |= 4;
    }

    if (extension) {
        *extension = '\0';
    }

    SStrCopy(texture->m_filename, fileName, STORM_MAX_STR);
    uint32_t size = SFileGetFileSize(file, nullptr);
    void* data = SMemAlloc(size, __FILE__, __LINE__, 0);

    if (!SFileReadFile(file, data, size, nullptr, nullptr, 0)) {
        DebugLog("CreateBlpTexture() failed read: %s.\n", fileName);
    }

    if (!PumpBlpTextureAsync(texture, data)) {
        FillInSolidTexture(*reinterpret_cast<const CImVector*>(&s_defaultColor), texture);
    }

    SFileCloseFile(file);
    SMemFree(data, __FILE__, __LINE__, 0);

    return texture;
}

// VERIFY: 1c0f47e5 manual
// OFFSET: 0x004B8A50
CTexture* CreateBlpAsync(char* extension, const char* fileName, uint32_t flags, CGxTexFlags texFlags) {
    SFileImpl* file = nullptr;
    SErrSetLastError(0);

    if (!SFile::OpenEx(nullptr, fileName, (flags >> 1) & 1, &file)) {
        if (!SErrGetLastError()) {
            SErrSetLastError(STORM_ERROR_FILE_NOT_FOUND);
        }

        return nullptr;
    }

    if (!file) {
        return nullptr;
    }

    if (extension) {
        *extension = '\0';
    }

    CTextureBlobTexture* blobTexture = nullptr;

    if (!(flags & 4)) {
        blobTexture = CTextureBlob::GetTexture(fileName);
    }

    CTexture* texture = new ("HTEXTURE", SLOG_OBJECT) CTexture;
    texture->m_gxTexFlags = texFlags;

    if (flags & 2) {
        texture->m_flags |= 2;
    }

    if ((flags & 4) && s_atlasEnable) {
        texture->m_flags |= 4;
    }

    if ((flags & 0x20) && SFile::IsStreamingMode()) {
        texture->m_flags |= 0x20;
    }

    SStrCopy(texture->m_filename, fileName, STORM_MAX_STR);

    if (blobTexture) {
        CTextureBlob::CreateTexture(texture, blobTexture);
    }

    texture->m_asyncObject = AsyncFileReadAllocObject();
    texture->m_asyncObject->m_userArg = texture;
    texture->m_asyncObject->m_userPostloadCallback = reinterpret_cast<void*>(AsyncTextureSuccessCallback);
    texture->m_asyncObject->m_userPostUnloadCallback = reinterpret_cast<void*>(AsyncTextureFailureCallback);
    texture->m_asyncObject->m_file = file;
    texture->m_asyncObject->m_size = SFileGetFileSize(file, nullptr);

    if (blobTexture) {
        texture->m_asyncObject->m_priority = 131;
    } else if (flags & 0x10) {
        texture->m_asyncObject->m_priority = 129;
    } else {
        texture->m_asyncObject->m_priority = 130;
    }

    CAsyncObject* object = texture->m_asyncObject;

    if (0x400000 - s_asyncLoadBufferUsed < object->m_size) {
        s_asyncTextureList.LinkNode(object, 2, nullptr);
    } else {
        AsyncTextureProcess(object, 0);
    }

    return texture;
}

// VERIFY: 77916a1a auto
// OFFSET: 0x004B8BE0
CTexture* CreateBlpTexture(const char* fileName, char* extension, uint32_t flags, CGxTexFlags texFlags) {
    char buffer[260];

    if (extension) {
        SStrCopy(extension, ".blp", STORM_MAX_STR);
    }

    const char* name = fileName;

    if (TextureBuildAliasPath(buffer, fileName)) {
        name = buffer;
        extension = OsPathFindExtensionWithDot(buffer);
    }

    CTexture* texture = s_createBlpAsync ? CreateBlpAsync(extension, name, flags, texFlags) : CreateBlpSync(name, extension, flags, texFlags);

    if (!texture) {
        return nullptr;
    }

    return reinterpret_cast<CTexture*>(HandleCreate(texture, "HTEXTURE"));
}

// VERIFY: b54359c2 manual
// OFFSET: 0x004B8C80
CTexture* TextureCreate(EGxTexTarget target, uint32_t width, uint32_t height, uint32_t depth, EGxTexFormat format, EGxTexFormat dataFormat, CGxTexFlags texFlags, void* userArg, GxTexUserFunc userFunc, const char* name, int32_t defaultFilter) {
    CTexture* texture = new ("HTEXTURE", SLOG_OBJECT) CTexture;

    if (defaultFilter) {
        texFlags.m_filter = s_filterMode;
    }

    texFlags.m_maxAnisotropy = texFlags.m_filter == GxTex_Anisotropic ? s_maxAnisotropy : 1;
    texture->m_gxTex = TextureAllocGxTex(target, width, height, depth, format, texFlags, userArg, userFunc, dataFormat, name);
    texture->m_dataFormat = dataFormat;
    texture->m_gxWidth = width;
    texture->m_gxTexFormat = format;
    texture->m_gxTexTarget = target;
    texture->m_gxHeight = height;
    texture->m_gxTexFlags = texFlags;
    texture->m_asyncObject = nullptr;
    SStrCopy(texture->m_filename, name ? name : "UniqueTexture", sizeof(texture->m_filename));

    return reinterpret_cast<CTexture*>(HandleCreate(texture, "HTEXTURE"));
}

// VERIFY: 42c19e46 manual
// OFFSET: 0x004B8D70
CTexture* TextureCreateBlob(char* fileName, CGxTexFlags texFlags, CStatus* status, int32_t defaultFilter) {
    if (!fileName) {
        DebugValidate("fileName");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    if (!fileName[0]) {
        DebugValidate("fileName[0]");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    if (!status) {
        DebugValidate("status");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    if (defaultFilter) {
        texFlags.m_filter = s_filterMode;
    }

    texFlags.m_maxAnisotropy = texFlags.m_filter == GxTex_Anisotropic ? s_maxAnisotropy : 1;
    char* extension = OsPathFindExtensionWithDot(fileName);

    if (extension) {
        *extension = '\0';
    }

    CTextureBlobTexture* blobTexture = CTextureBlob::GetTexture(fileName);
    CTexture* handle = nullptr;

    if (blobTexture) {
        CTexture* texture = new ("HTEXTURE", SLOG_OBJECT) CTexture;
        texture->m_gxTexFlags = texFlags;
        SStrCopy(texture->m_filename, fileName, STORM_MAX_STR);
        CTextureBlob::CreateTexture(texture, blobTexture);
        handle = reinterpret_cast<CTexture*>(HandleCreate(texture, "HTEXTURE"));
    }

    if (extension) {
        *extension = '.';
    }

    return handle;
}

// VERIFY: c357de66 auto
// OFFSET: 0x004B9200
CTexture* TextureCreate(uint32_t width, uint32_t height, EGxTexFormat format, EGxTexFormat dataFormat, CGxTexFlags texFlags, void* userArg, GxTexUserFunc userFunc, const char* name, int32_t defaultFilter) {
    return TextureCreate(GxTex_2d, width, height, 0, format, dataFormat, texFlags, userArg, userFunc, name, defaultFilter);
}

// VERIFY: 70b37ccc auto
// OFFSET: 0x004B9420
void TextureCacheNewTexture(CTexture* texture) {
    uint32_t hashval = SStrHashHT(texture->m_filename);
    HASHKEY_TEXTUREFILE key;
    key.m_anyTexFlags = 0;
    key.m_texFlags = texture->m_gxTexFlags;
    key.m_filename = texture->m_filename;
    s_textureCache.Insert(texture, hashval, key);
}

// VERIFY: 07bcfe26 manual
// OFFSET: 0x004B9480
void TextureCacheNewTexture(CTexture* texture, CGxTexFlags texFlags) {
    uint32_t hashval = SStrHashHT(texture->m_filename);
    HASHKEY_TEXTUREFILE key;
    key.m_filename = texture->m_filename;
    key.m_texFlags = texFlags;
    s_textureCache.Insert(texture, hashval, key);
}

// VERIFY: 27dbedb5 manual
// OFFSET: 0x004B94E0
void TextureCacheNewTexture(CTexture* texture, const CImVector& color) {
    CGxTexFlags texFlags;
    HASHKEY_TEXTUREFILE key;
    key.m_filename = const_cast<char*>("");
    key.m_texFlags = texFlags;
    s_textureCache.Insert(texture, *reinterpret_cast<const uint32_t*>(&color), key);
}

// VERIFY: 875e1f5e auto
// OFFSET: 0x004B9550
CTexture* TextureCreateSolid(const CImVector& color) {
    CTexture* texture = TextureCacheGetTexture(color);

    if (texture) {
        return texture;
    }

    texture = new ("HTEXTURE", SLOG_OBJECT) CTexture;

    if (!texture) {
        return nullptr;
    }

    FillInSolidTexture(color, texture);
    CTexture* handle = reinterpret_cast<CTexture*>(HandleCreate(texture, "HTEXTURE"));
    TextureCacheNewTexture(handle, color);

    return handle;
}

// VERIFY: ea203deb manual
// OFFSET: 0x004B95B0
CTexture* CreateTgaTexture(const char* fileName, char* extension, int32_t flags, CGxTexFlags texFlags, CStatus* status) {
    if (extension) {
        SStrCopy(extension, ".tga", STORM_MAX_STR);
    }

    CTgaFile image;

    if (!image.Open(fileName, flags)) {
        return nullptr;
    }

    uint32_t width = image.Width();
    uint32_t height = image.Height();
    EGxTexTarget target = GxTex_2d;

    if (width == 6 * height) {
        target = GxTex_CubeMap;
        width /= 6;
    }

    if (width < 8 || height < 8) {
        status->Add(STATUS_FATAL, "Error loading file \"%s\": Texture size must be at least %dx%d\n", fileName, 8, 8);
        return TextureCreateSolid(*reinterpret_cast<const CImVector*>(&s_defaultColor));
    }

    CTexture* texture = new ("HTEXTURE", SLOG_OBJECT) CTexture;

    if (!texture) {
        return nullptr;
    }

    texture->m_alphaBits = image.AlphaBits();

    if (!texture->m_alphaBits) {
        texture->m_flags |= 1;
    }

    if (flags) {
        texture->m_flags |= 2;
    }

    if (extension) {
        *extension = '\0';
    }

    SStrCopy(texture->m_filename, fileName, sizeof(texture->m_filename));
    texture->m_gxTex = TextureAllocGxTex(target, width, height, 0, GxTex_Argb8888, texFlags, texture, UpdateTgaTexture, GxTex_Argb8888, texture->m_filename);

    if (!texture->m_gxTex) {
        texture->~CTexture();
        SMemFree(texture, __FILE__, __LINE__, 0);
        return nullptr;
    }

    texture->m_gxTexTarget = target;
    texture->m_gxWidth = width;
    texture->m_gxHeight = height;

    return reinterpret_cast<CTexture*>(HandleCreate(texture, "HTEXTURE"));
}

// VERIFY: 8821b223 manual
// OFFSET: 0x004B9760
CTexture* TextureCreate(const char* fileName, CGxTexFlags texFlags, CStatus* status, uint32_t flags) {
    if (!fileName) {
        DebugValidate("fileName");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    if (!fileName[0]) {
        DebugValidate("fileName[0]");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    if (!status) {
        DebugValidate("status");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    if (!(flags & 1)) {
        texFlags.m_filter = s_filterMode;
    }

    texFlags.m_maxAnisotropy = texFlags.m_filter == GxTex_Anisotropic ? s_maxAnisotropy : 1;
    CGxTexFlags requested = texFlags;

    int32_t tries = 2;
    int32_t type = 1;
    char name[260];
    SStrCopy(name, fileName, 260);
    char* extension = OsPathFindExtensionWithDot(name);

    if ((flags & 8) && extension) {
        tries = 1;
        type = SStrCmpI(extension, ".blp", STORM_MAX_STR) == 0;
        extension = nullptr;
    }

    CTexture* texture = TextureCacheGetTexture(name, extension, texFlags);

    if (texture) {
        return texture;
    }

    for (int32_t i = 0; i < tries; i++) {
        if (type == 0) {
            texture = CreateTgaTexture(name, extension, flags & 2, texFlags, status);
        } else if (type == 1) {
            texture = CreateBlpTexture(name, extension, flags, texFlags);
        } else {
            type = (type + 1) % 2;
            continue;
        }

        if (texture) {
            TextureCacheNewTexture(texture, requested);
            return texture;
        }

        texFlags = requested;
        type = (type + 1) % 2;
    }

    TextureReportLoadError("texture", fileName, status);

    return TextureCreateSolid(*reinterpret_cast<const CImVector*>(&s_defaultColor));
}
