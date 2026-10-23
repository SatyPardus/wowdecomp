#include "Liquid.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Tempest/CiRect.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/NTempest.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"

static const int32_t WATERTEX_HEIGHT = 64;
static const float kDeepDarken = 0.9f;

// STATIC: 0x00D43B48
uint8_t Liquid::s_unk_D43B48[4];

// STATIC: 0x00D43B4C
CTexture* Liquid::s_unk_D43B4C;

// STATIC: 0x00D43B50
CTexture* Liquid::s_unk_D43B50[32];

// STATIC: 0x00D43BD0
CTexture* Liquid::s_wmoWaterTex;

// STATIC: 0x00D43BD4
CTexture* Liquid::s_oceanDepthTex;

// STATIC: 0x00D43BD8
CTexture* Liquid::s_riverDepthTex;

// VERIFY: 901024cc manual
// OFFSET: 0x008A2980
void Liquid::DestroyProceduralTextures() {
    if (Liquid::s_unk_D43B4C) {
        HandleClose(reinterpret_cast<HOBJECT>(Liquid::s_unk_D43B4C));
        Liquid::s_unk_D43B4C = nullptr;
    }

    if (Liquid::s_riverDepthTex) {
        HandleClose(reinterpret_cast<HOBJECT>(Liquid::s_riverDepthTex));
        Liquid::s_riverDepthTex = nullptr;
    }

    if (Liquid::s_oceanDepthTex) {
        HandleClose(reinterpret_cast<HOBJECT>(Liquid::s_oceanDepthTex));
        Liquid::s_oceanDepthTex = nullptr;
    }

    if (Liquid::s_wmoWaterTex) {
        HandleClose(reinterpret_cast<HOBJECT>(Liquid::s_wmoWaterTex));
        Liquid::s_wmoWaterTex = nullptr;
    }

    for (uint32_t i = 0; i < sizeof(Liquid::s_unk_D43B50) / sizeof(Liquid::s_unk_D43B50[0]); i++) {
        if (Liquid::s_unk_D43B50[i]) {
            HandleClose(reinterpret_cast<HOBJECT>(Liquid::s_unk_D43B50[i]));
            Liquid::s_unk_D43B50[i] = nullptr;
        }
    }
}

// VERIFY: bbba7190 auto
// OFFSET: 0x008A2A10
void Liquid::CacheProceduralTextures() {
    CGxTexFlags flags;

    TextureCacheNewTexture(Liquid::s_riverDepthTex);
    TextureCacheNewTexture(Liquid::s_oceanDepthTex);
    TextureCacheNewTexture(Liquid::s_wmoWaterTex);

    for (uint32_t i = 0; i < sizeof(Liquid::s_unk_D43B50) / sizeof(Liquid::s_unk_D43B50[0]); i++) {
        if (Liquid::s_unk_D43B50[i]) {
            TextureCacheNewTexture(Liquid::s_unk_D43B50[i]);
        }
    }

    if (Liquid::s_unk_D43B4C) {
        TextureCacheNewTexture(Liquid::s_unk_D43B4C);
    }

    Liquid::CSettingsBank::ResetTextures();
}

// VERIFY: 5d576297 auto
// OFFSET: 0x008A2E20
void Liquid::CreateProceduralTextures() {
    Liquid::s_unk_D43B48[3] = 0;
    Liquid::s_unk_D43B48[2] = 0;
    Liquid::s_unk_D43B48[1] = 0;
    Liquid::s_unk_D43B48[0] = 0;

    CGxTexFlags flags;
    Liquid::s_riverDepthTex = TextureCreate(8, WATERTEX_HEIGHT, GxTex_Argb8888, GxTex_Argb8888, flags, reinterpret_cast<void*>(1), Liquid::ExtWaterDepthTexCallback, "proceduralRiverDepthTex", 0);

    flags = CGxTexFlags();
    Liquid::s_oceanDepthTex = TextureCreate(8, WATERTEX_HEIGHT, GxTex_Argb8888, GxTex_Argb8888, flags, nullptr, Liquid::ExtWaterDepthTexCallback, "proceduralOceanDepthTex", 0);

    flags = CGxTexFlags();
    Liquid::s_wmoWaterTex = TextureCreate(8, WATERTEX_HEIGHT, GxTex_Argb8888, GxTex_Argb8888, flags, nullptr, Liquid::WmoWaterTexCallback, "proceduralWmoWaterTex", 0);

    Liquid::CacheProceduralTextures();
}

// VERIFY: 7e3d28c3 manual
// OFFSET: 0x008A2F00
void Liquid::UpdateProceduralTextures() {
    if (!Liquid::s_unk_D43B48[3] && Liquid::s_oceanDepthTex) {
        CGxTex* gxTex = TextureGetGxTex(Liquid::s_oceanDepthTex, 0, nullptr);

        if (gxTex) {
            CiRect rect;
            rect.minY = 0;
            rect.minX = 0;
            rect.maxY = WATERTEX_HEIGHT;
            rect.maxX = 8;
            GxTexUpdate(gxTex, rect, 0);
            Liquid::s_unk_D43B48[3] = 1;
        }
    }

    if (!Liquid::s_unk_D43B48[2] && Liquid::s_riverDepthTex) {
        CGxTex* gxTex = TextureGetGxTex(Liquid::s_riverDepthTex, 0, nullptr);

        if (gxTex) {
            CiRect rect;
            rect.minY = 0;
            rect.minX = 0;
            rect.maxY = WATERTEX_HEIGHT;
            rect.maxX = 8;
            GxTexUpdate(gxTex, rect, 0);
            Liquid::s_unk_D43B48[2] = 1;
        }
    }

    if (!Liquid::s_unk_D43B48[1] && Liquid::s_wmoWaterTex) {
        CGxTex* gxTex = TextureGetGxTex(Liquid::s_wmoWaterTex, 0, nullptr);

        if (gxTex) {
            CiRect rect;
            rect.minY = 0;
            rect.minX = 0;
            rect.maxY = WATERTEX_HEIGHT;
            rect.maxX = 8;
            GxTexUpdate(gxTex, rect, 0);
            Liquid::s_unk_D43B48[1] = 1;
        }
    }

    if (!Liquid::s_unk_D43B48[0]) {
        for (uint32_t i = 0; i < sizeof(Liquid::s_unk_D43B50) / sizeof(Liquid::s_unk_D43B50[0]); i++) {
            if (Liquid::s_unk_D43B50[i]) {
                GxTexUpdate(TextureGetGxTex(Liquid::s_unk_D43B50[i], 0, nullptr), 0, 0, 0, 0, 1);
            }
        }

        Liquid::s_unk_D43B48[0] = 1;
    }

    if (Liquid::s_unk_D43B4C) {
        CGxTex* gxTex = TextureGetGxTex(Liquid::s_unk_D43B4C, 1, nullptr);

        if (gxTex) {
            GxTexStretch(gxTex, nullptr, nullptr, 0, 0);
        }
    }
}

// VERIFY: ff857bf4 auto
// OFFSET: 0x008A2AA0
void Liquid::InvalidateProceduralTextures() {
    Liquid::s_unk_D43B48[3] = 0;
    Liquid::s_unk_D43B48[2] = 0;
    Liquid::s_unk_D43B48[1] = 0;
}

// VERIFY: 331f7c3c manual
// OFFSET: 0x008A2AC0
void Liquid::WmoWaterTexCallback(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    // STATIC: 0x00D43BE0
    static CImVector s_texels[WATERTEX_HEIGHT * 8];

    if (command != GxTex_Latch) {
        return;
    }

    DayNight::DNInfo* info = DayNight::GetInfo();
    CImVector band = info->m_bands.m_waterColors[3];
    uint8_t shallowAlpha = CMath::fint_(info->m_bands.m_waterShallowAlpha * 255.0f);
    uint8_t deepAlpha = CMath::fint_(info->m_bands.m_waterDeepAlpha * 255.0f);
    int32_t alphaStep = ((deepAlpha - shallowAlpha) << 8) >> 6;
    int32_t alpha = shallowAlpha << 8;

    CImVector color;
    color.b = band.b;
    color.g = band.g;
    color.r = band.r;

    CImVector white;
    white.b = 0xFF;
    white.g = 0xFF;
    white.r = 0xFF;

    for (uint32_t y = 0; y < WATERTEX_HEIGHT; y++) {
        for (uint32_t x = 0; x < 4; x++) {
            color.a = alpha >> 8;
            s_texels[y * 8 + x] = color;
        }

        for (uint32_t x = 4; x < 8; x++) {
            white.a = alpha >> 8;
            s_texels[y * 8 + x] = white;
        }

        alpha += alphaStep;
    }

    texelStrideInBytes = 8 * sizeof(CImVector);
    texels = s_texels;
}

// VERIFY: aa313e0a manual
// OFFSET: 0x008A2BF0
void Liquid::ExtWaterDepthTexCallback(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    // STATIC: 0x00D443E8
    static CImVector s_texels[WATERTEX_HEIGHT * 8];

    if (command != GxTex_Latch) {
        return;
    }

    DayNight::DNInfo* info = DayNight::GetInfo();

    uint8_t shallowAlpha[2];
    shallowAlpha[0] = CMath::fint_(info->m_bands.m_oceanShallowAlpha * 255.0f);
    shallowAlpha[1] = CMath::fint_(info->m_bands.m_waterShallowAlpha * 255.0f);

    uint8_t deepAlpha[2];
    deepAlpha[0] = CMath::fint_(info->m_bands.m_oceanDeepAlpha * 255.0f);
    deepAlpha[1] = CMath::fint_(info->m_bands.m_waterDeepAlpha * 255.0f);

    uintptr_t type = reinterpret_cast<uintptr_t>(userArg);
    CImVector shallowColor = info->m_bands.m_waterColors[type * 2];
    CImVector deepColor = info->m_bands.m_waterColors[type * 2 + 1];

    int32_t alphaStep = ((deepAlpha[type] - shallowAlpha[type]) << 8) >> 6;
    int32_t redStep = ((deepColor.r - shallowColor.r) << 8) >> 6;
    int32_t blueStep = ((deepColor.b - shallowColor.b) << 8) >> 6;
    int32_t greenStep = ((deepColor.g - shallowColor.g) << 8) >> 6;

    int32_t alpha = shallowAlpha[type] << 8;
    int32_t red = shallowColor.r << 8;
    int32_t green = shallowColor.g << 8;
    int32_t blue = shallowColor.b << 8;

    CImVector* row = s_texels;

    for (uint32_t y = 0; y < height; y++) {
        CImVector color;
        color.b = blue >> 8;
        color.g = green >> 8;
        color.r = red >> 8;
        color.a = alpha >> 8;

        if (y == height - 1 && type == 0) {
            C3Vector rgb(color);
            C3Vector hsv;
            RGBtoHSV(rgb, hsv);
            hsv.z *= kDeepDarken;
            HSVtoRGB(hsv, rgb);
            color = rgb;
        }

        for (uint32_t x = 0; x < width; x++) {
            row[x] = color;
        }

        green += greenStep;
        blue += blueStep;
        alpha += alphaStep;
        red += redStep;
        row += width;
    }

    texelStrideInBytes = width * sizeof(CImVector);
    texels = s_texels;
}
