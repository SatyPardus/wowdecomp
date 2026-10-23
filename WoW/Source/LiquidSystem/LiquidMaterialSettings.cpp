#include "Liquid.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CMath.h"

enum LiquidTextureCounts : uint32_t {
    NUM_LIQUID_TEX_FRAMES = 30,
};

// VERIFY: c119ee84 auto
// OFFSET: 0x008A16C0
int32_t Liquid::CMaterialSettings::GetInt(uint32_t index) const {
    return this->m_ints[index];
}

// VERIFY: 7e415766 auto
// OFFSET: 0x008A16E0
float Liquid::CMaterialSettings::GetFloat(uint32_t index) const {
    return this->m_floats[index];
}

// VERIFY: 64d572f1 manual
// OFFSET: 0x008A1C90
Liquid::CMaterialSettings::~CMaterialSettings() {
}

// VERIFY: 0f1ff559 manual
// OFFSET: 0x008A1D00
void Liquid::CMaterialSettings::Destroy() {
    for (uint32_t i = 0; i < 6; i++) {
        for (uint32_t j = 0; j < this->m_textures[i].Count(); j++) {
            HandleClose(reinterpret_cast<HOBJECT>(this->m_textures[i].m_data[j]));
        }

        this->m_textures[i].m_count = 0;

        for (uint32_t j = 0; j < this->m_pendingTextures[i].Count(); j++) {
            HandleClose(reinterpret_cast<HOBJECT>(this->m_pendingTextures[i].m_data[j]));
        }

        this->m_pendingTextures[i].m_count = 0;
    }
}

// VERIFY: d9c082b2 manual
// OFFSET: 0x008A1D60
CGxTex* Liquid::CMaterialSettings::GetTexture(uint32_t index, uint32_t period) {
    TSGrowableArray<CTexture*>* textures = &this->m_textures[index];
    uint32_t count = textures->Count();

    if (!count) {
        return nullptr;
    }

    if (count == 1) {
        return TextureGetGxTex(textures->m_data[0], 0, nullptr);
    }

    if (!this->m_texturesReady[index]) {
        TSGrowableArray<CTexture*>* pending = &this->m_pendingTextures[index];

        if (pending->Count()) {
            bool ready = true;

            for (uint32_t i = 0; i < count && ready; i++) {
                CTexture* texture = textures->m_data[i];

                if (TextureIsBlob(texture)) {
                    TextureIncreasePriority(texture);
                    ready = false;
                }
            }

            if (ready) {
                this->m_texturesReady[index] = 1;

                for (uint32_t i = 0; i < pending->Count(); i++) {
                    HandleClose(reinterpret_cast<HOBJECT>(pending->m_data[i]));
                }

                pending->m_count = 0;
            } else {
                textures = pending;
            }
        } else {
            bool loaded = true;

            for (uint32_t i = 0; i < count && loaded; i++) {
                if (!TextureGetGxTex(textures->m_data[i], 0, nullptr)) {
                    loaded = false;
                }
            }

            if (!loaded) {
                return nullptr;
            }

            this->m_texturesReady[index] = 1;
        }
    }

    if (!period) {
        period = 1;
    }

    float frame = static_cast<float>(OsGetAsyncTimeMs() % period) / static_cast<float>(period) * static_cast<float>(count);

    return TextureGetGxTex(textures->m_data[CMath::fint_(frame - 0.5f)], 1, nullptr);
}

// VERIFY: 7226420f manual
// OFFSET: 0x008A2100
Liquid::CMaterialSettings::CMaterialSettings() {
}

// VERIFY: d19ea806 manual
// OFFSET: 0x008A2450
void Liquid::CMaterialSettings::InitTexture() {
    for (uint32_t i = 0; i < 6; i++) {
        this->m_texturesReady[i] = 0;

        const char* name = this->m_textureNames[i];

        if (!name[0]) {
            continue;
        }

        int32_t procedural = SStrStrI(name, "procedural") != nullptr;
        CGxTexFlags flags(procedural ? GxTex_Linear : GxTex_LinearMipLinear, !procedural, !procedural, 0, 0, 0, 1, 0, 0, 0);
        bool animated = SStrStrI(name, "%d") != nullptr;
        CStatus status;

        if (animated) {
            char fileName[128];
            int32_t hasBlob = 0;

            for (uint32_t frame = 1; frame <= NUM_LIQUID_TEX_FRAMES; frame++) {
                SStrPrintf(fileName, sizeof(fileName), name, frame);
                CTexture* texture = TextureCreate(fileName, flags, &status, 0);
                this->m_textures[i].New(texture);

                if (TextureIsBlob(texture)) {
                    hasBlob = 1;
                }
            }

            if (hasBlob) {
                for (uint32_t frame = 1; frame <= NUM_LIQUID_TEX_FRAMES; frame++) {
                    SStrPrintf(fileName, sizeof(fileName), name, frame);
                    CTexture* texture = TextureCreateBlob(fileName, flags, &status, 1);

                    if (texture) {
                        this->m_pendingTextures[i].New(texture);
                    }
                }

                if (this->m_pendingTextures[i].Count() != this->m_textures[i].Count()) {
                    for (uint32_t j = 0; j < this->m_pendingTextures[i].Count(); j++) {
                        HandleClose(reinterpret_cast<HOBJECT>(this->m_pendingTextures[i].m_data[j]));
                    }

                    this->m_pendingTextures[i].m_count = 0;
                }
            }
        } else {
            CTexture* texture;

            if (procedural) {
                texture = TextureCacheGetTexture(name);

                if (!texture) {
                    CImVector color;
                    color.b = 0x00;
                    color.g = 0xFF;
                    color.r = 0x00;
                    color.a = 0xFF;
                    texture = TextureCreateSolid(color);
                }
            } else {
                texture = TextureCreate(name, flags, &status, 0);
            }

            if (texture) {
                this->m_textures[i].New(texture);
            }
        }
    }
}
