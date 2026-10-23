#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <emmintrin.h>
#include <new>
#include <typeinfo>
#if defined(_MSC_VER)
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Os/Path.h"
#include "Engine/Source/Os/Processor.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "Engine/Source/Storm/Source/SThread.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Common/ComponentCore/ComponentUtils.h"
#include "WoW/Common/TextureCache.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/CharHairGeosetsRec.h"
#include "WoW/Source/DB/Rec/CharSectionsRec.h"
#include "WoW/Source/DB/Rec/CharacterFacialHairStylesRec.h"
#include "WoW/Source/DB/Rec/ChrRacesRec.h"
#include "WoW/Source/DB/Rec/CreatureDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/CreatureModelDataRec.h"
#include "WoW/Source/DB/Rec/HelmetGeosetVisDataRec.h"
#include "WoW/Source/DB/Rec/ItemDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/ItemVisualEffectsRec.h"
#include "WoW/Source/DB/Rec/ItemVisualsRec.h"
#include "WoW/Source/DB/Rec/ParticleColorRec.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"

// STATIC: 0x009F6A00
static int32_t s_itemPriority[NUM_ITEM_SLOTS][NUM_COMPONENT_SECTIONS] = {
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 0, 0, -1, 0, 0, -1, -1, -1, -1, -1 },
    { 1, 1, -1, 1, 1, 1, 1, -1, -1, -1 },
    { -1, -1, -1, -1, 5, 2, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1, 0, 0, -1, -1, -1 },
    { -1, -1, -1, -1, -1, -1, 2, 0, -1, -1 },
    { -1, 2, -1, -1, -1, -1, -1, -1, -1, -1 },
    { -1, 3, 0, -1, -1, -1, -1, -1, -1, -1 },
    { -1, -1, -1, 4, 4, -1, -1, -1, -1, -1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

// STATIC: 0x00AC4680
static const char* s_componentSections[8] = { "ArmUpperTexture", "ArmLowerTexture", "HandTexture", "TorsoUpperTexture", "TorsoLowerTexture", "LegUpperTexture", "LegLowerTexture", "FootTexture" };

// STATIC: 0x00AC46A0
static const char* s_fileDecorations[3] = { "M", "F", "U" };

// STATIC: 0x00AC46D0
static CStatus s_status;

// STATIC: 0x00AC46E4
TSExplicitList<CCharacterComponent, 0> CCharacterComponent::s_queue;

// STATIC: 0x00AC46F0
static TSExplicitList<CComponentRequest, offsetof(CComponentRequest, m_link)> s_requestList;

// STATIC: 0x00AC46FC
static TSExplicitList<CComponentRequest, offsetof(CComponentRequest, m_link)> s_finishedList;

// STATIC: 0x00AC4708
static TSExplicitList<CComponentRequest, offsetof(CComponentRequest, m_link)> s_requestFreeList;

// STATIC: 0x00AC4714
static TSExplicitList<CComponentMipBits, offsetof(CComponentMipBits, m_link)> s_mipBitsFreeList;

// STATIC: 0x00AC4720
static TSExplicitList<CComponentMipBits, offsetof(CComponentMipBits, m_link)> s_mipBitsCompressedFreeList;

// STATIC: 0x00B6B4B8
void (CCharacterComponent::*CCharacterComponent::s_requestFunc[NUM_COMPONENT_SECTIONS])();

// STATIC: 0x00B6B4E0
static HPROPCONTEXT s_threadPropContext;

// STATIC: 0x00B6B4E4
int32_t CCharacterComponent::s_bComponentCompression;

// STATIC: 0x00B6B4E8
int32_t CCharacterComponent::s_bComponentThread;

// STATIC: 0x00B6B4EC
static char* s_pathEnd2;

// STATIC: 0x00B6B4F0
static char* s_pathEnd;

// STATIC: 0x00B6B4F4
uint32_t CCharacterComponent::s_mipLevels;

// STATIC: 0x00B6B4F8
static char s_path2[260];

// STATIC: 0x00B6B5FC
static uint32_t s_textureSize;

// STATIC: 0x00B6B600
static char s_path[260];

// STATIC: 0x00B6B704
EGxTexFormat CCharacterComponent::s_gxFormatHigh;

// STATIC: 0x00B6B708
static char s_buffer[260];

// STATIC: 0x00B6B80C
void (CCharacterComponent::*CCharacterComponent::m_itemFunc[NUM_COMPONENT_SECTIONS])(ITEM_SLOT, const ItemDisplayInfoRec*, bool);

// STATIC: 0x00B6B834
void (CCharacterComponent::*CCharacterComponent::m_prepFunc[NUM_COMPONENT_SECTIONS])();

// STATIC: 0x00B6B85C
EGxTexFormat CCharacterComponent::s_gxFormat;

// STATIC: 0x00B6B860
uint32_t* CCharacterComponent::s_characterFacialHairStylesList;

// STATIC: 0x00B6B864
st_race* CCharacterComponent::s_chrVarArray;

// STATIC: 0x00B6B868
static MipBits* s_threadTextureBuffer;

// STATIC: 0x00B6B86C
static MipBits* s_compressedTextureBuffer;

// STATIC: 0x00B6B870
MipBits* CCharacterComponent::s_textureBuffer;

// STATIC: 0x00B6B874
uint32_t CCharacterComponent::s_chrVarArrayLength;

// STATIC: 0x00B6B878
CCharacterComponent* CCharacterComponent::m_activePlayerComponent;

// STATIC: 0x00B6B880
static uint32_t* s_componentHeap;

// STATIC: 0x00B6B884
int32_t CCharacterComponent::s_bInRenderPrep;

// STATIC: 0x00B6B888
static CompSectionInfo s_sectionInfo[NUM_COMPONENT_SECTIONS];

// STATIC: 0x00B6B928
CompSectionInfo CCharacterComponent::s_sectionInfoRaw[NUM_COMPONENT_SECTIONS] = {
    { { 0, 0 }, { 256, 128 } },
    { { 0, 128 }, { 256, 128 } },
    { { 0, 256 }, { 256, 64 } },
    { { 256, 0 }, { 256, 128 } },
    { { 256, 128 }, { 256, 64 } },
    { { 256, 192 }, { 256, 128 } },
    { { 256, 320 }, { 256, 128 } },
    { { 256, 448 }, { 256, 64 } },
    { { 0, 320 }, { 256, 64 } },
    { { 0, 384 }, { 256, 128 } },
};

// STATIC: 0x00B6B9C8
static SThread s_thread;

// STATIC: 0x00B6B9CC
static SEvent s_threadBeginEvent(0, 0);

// STATIC: 0x00B6B9D0
static SCritSect s_requestListCritSect;

// STATIC: 0x00B6B9F8
static SCritSect s_finishedListCritSect;

// STATIC: 0x00B6BA20
static SCritSect s_mipBitsCritSect;

// VERIFY: e87c3da6 auto
// OFFSET: 0x00401FF0
bool CCharacterComponent::Initialize() {
    CCharacterComponent::g_componentTextureLevelVar = CVar::Register("componentTextureLevel", "Number of mip levels used for character component textures", 0x1, "8", &ComponentVarHandler, 0, false, nullptr, false);
    CCharacterComponent::g_componentThreadVar = CVar::Register("componentThread", "Multi thread character component processing", 0x1, "1", &ComponentVarHandler, 0, false, nullptr, false);
    CCharacterComponent::g_componentCompressVar = CVar::Register("componentCompress", "Character component texture compression", 0x1, "1", &ComponentVarHandler, 0, false, nullptr, false);

    uint32_t mipLevels = CCharacterComponent::g_componentTextureLevelVar->m_intValue;
    int32_t useThread = CCharacterComponent::g_componentThreadVar->m_intValue;
    int32_t useCompression = CCharacterComponent::g_componentCompressVar->m_intValue;

    if (OsGetNumberOfProcessors() < 2) {
        useThread = 0;
    }

    if (!useThread) {
        if (mipLevels > 8) {
            mipLevels = 8;
        }

        useCompression = 0;
    } else if (mipLevels > 9) {
        return CCharacterComponent::Initialize(GxTex_Rgb565, 9, useThread, useCompression);
    }

    if (mipLevels < 7) {
        mipLevels = 7;
    }

    return CCharacterComponent::Initialize(GxTex_Rgb565, mipLevels, useThread, useCompression);
}

// VERIFY: 5682b94b manual
// OFFSET: 0x004DFED0
void CCharacterComponent::GetPreferences(CharacterPreferences* preferences) {
    if (preferences) {
        *preferences = this->m_data.m_preferences;
    }
}

// VERIFY: f0e7f328 manual
// OFFSET: 0x004E6F90
static int32_t CopyStringToBuffer(const char* src, char* dst) {
    strcpy(dst, src);

    return 1;
}

// VERIFY: 576ded8c auto
// OFFSET: 0x004E6FB0
HTEXTURECACHE CCharacterComponent::LoadItemComponentTexture(const ItemDisplayInfoRec* displayRec, COMPONENT_SECTIONS section) {
    SStrPrintf(s_path, sizeof(s_path), "Item\\TextureComponents\\%s\\%s_%s.blp", s_componentSections[section], displayRec->m_texture[section], s_fileDecorations[2]);

    if (!SFile::FileExists(s_path)) {
        s_path[strlen(s_path) - 5] = s_fileDecorations[this->m_data.m_preferences.m_sexID][0];
    }

    return TextureCacheCreateTexture(s_path);
}

// VERIFY: c2d32b47 manual
// OFFSET: 0x004E7030
static void CompressBlockSSE(const uint8_t* src, uint32_t stride, uint32_t* out) {
    const __m128i zero = _mm_setzero_si128();
    const __m128 scale = _mm_set1_ps(1.0f / 255.0f);

    __m128i row0 = _mm_load_si128(reinterpret_cast<const __m128i*>(src));
    __m128i row1 = _mm_load_si128(reinterpret_cast<const __m128i*>(src + stride));
    __m128i row2 = _mm_load_si128(reinterpret_cast<const __m128i*>(src + 2 * stride));
    __m128i row3 = _mm_load_si128(reinterpret_cast<const __m128i*>(src + 3 * stride));

    __m128i sum = _mm_add_epi16(_mm_unpacklo_epi8(row0, zero), _mm_unpackhi_epi8(row0, zero));
    sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(row1, zero));
    sum = _mm_add_epi16(sum, _mm_unpackhi_epi8(row1, zero));
    sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(row2, zero));
    sum = _mm_add_epi16(sum, _mm_unpackhi_epi8(row2, zero));
    sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(row3, zero));
    sum = _mm_add_epi16(sum, _mm_unpackhi_epi8(row3, zero));

    __m128 mean = _mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_srli_epi16(_mm_add_epi16(sum, _mm_shuffle_epi32(sum, 0x4E)), 4), zero)), scale);

    __m128 cov0 = _mm_setzero_ps();
    __m128 cov1 = _mm_setzero_ps();
    __m128 cov2 = _mm_setzero_ps();
    const uint8_t* row = src;

    for (int32_t y = 0; y < 4; y++, row += stride) {
        __m128i pixels = _mm_load_si128(reinterpret_cast<const __m128i*>(row));
        __m128i high = _mm_unpackhi_epi8(pixels, zero);
        __m128i low = _mm_unpacklo_epi8(pixels, zero);
        __m128 d3 = _mm_sub_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpackhi_epi16(high, zero)), scale), mean);
        __m128 d2 = _mm_sub_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(high, zero)), scale), mean);
        __m128 d1 = _mm_sub_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpackhi_epi16(low, zero)), scale), mean);
        __m128 d0 = _mm_sub_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(low, zero)), scale), mean);

        cov0 = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(d3, d3, 0x00), d3), cov0), _mm_mul_ps(_mm_shuffle_ps(d2, d2, 0x00), d2)), _mm_mul_ps(_mm_shuffle_ps(d1, d1, 0x00), d1)), _mm_mul_ps(_mm_shuffle_ps(d0, d0, 0x00), d0));
        cov1 = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(d3, d3, 0x55), d3), cov1), _mm_mul_ps(_mm_shuffle_ps(d2, d2, 0x55), d2)), _mm_mul_ps(_mm_shuffle_ps(d1, d1, 0x55), d1)), _mm_mul_ps(_mm_shuffle_ps(d0, d0, 0x55), d0));
        cov2 = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(d3, d3, 0xAA), d3), cov2), _mm_mul_ps(_mm_shuffle_ps(d2, d2, 0xAA), d2)), _mm_mul_ps(_mm_shuffle_ps(d1, d1, 0xAA), d1)), _mm_mul_ps(_mm_shuffle_ps(d0, d0, 0xAA), d0));
    }

    const __m128 zeroPs = _mm_setzero_ps();
    __m128 axis = _mm_set1_ps(1.0f);

    for (int32_t i = 0; i < 8; i++) {
        __m128 next = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(axis, axis, 0xAA), cov2), _mm_mul_ps(_mm_shuffle_ps(axis, axis, 0x55), cov1)), _mm_mul_ps(_mm_shuffle_ps(axis, axis, 0x00), cov0));
        __m128 largest = _mm_max_ps(next, _mm_shuffle_ps(next, next, 0xB1));
        largest = _mm_max_ps(largest, _mm_shuffle_ps(largest, largest, 0x4E));
        axis = _mm_mul_ps(_mm_and_ps(_mm_cmpneq_ps(zeroPs, largest), _mm_rcp_ps(largest)), next);
    }

    __m128 length = _mm_mul_ps(axis, axis);
    length = _mm_add_ps(_mm_shuffle_ps(length, length, 0xB1), length);
    length = _mm_add_ps(_mm_shuffle_ps(length, length, 0x4E), length);
    axis = _mm_and_ps(_mm_mul_ps(_mm_rsqrt_ps(length), axis), _mm_cmpneq_ps(zeroPs, length));

    __m128 minProj = _mm_set_ss(1.0f);
    __m128 maxProj = _mm_set_ss(-1.0f);
    __m128 axisX = _mm_shuffle_ps(axis, axis, 0x00);
    __m128 axisY = _mm_shuffle_ps(axis, axis, 0x55);
    __m128 axisZ = _mm_shuffle_ps(axis, axis, 0xAA);
    __m128 meanX = _mm_shuffle_ps(mean, mean, 0x00);
    __m128 meanY = _mm_shuffle_ps(mean, mean, 0x55);
    __m128 meanZ = _mm_shuffle_ps(mean, mean, 0xAA);
    __m128 proj[4];
    row = src;

    for (int32_t y = 0; y < 4; y++, row += stride) {
        __m128i pixels = _mm_load_si128(reinterpret_cast<const __m128i*>(row));
        __m128i shuffled = _mm_unpackhi_epi8(_mm_shuffle_epi32(pixels, 0x8D), _mm_shuffle_epi32(pixels, 0xD8));
        __m128i planar = _mm_unpackhi_epi16(_mm_slli_si128(shuffled, 8), shuffled);
        __m128i channels = _mm_unpacklo_epi8(planar, zero);
        __m128 p = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(channels, zero)), scale), meanX), axisX), _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpackhi_epi16(channels, zero)), scale), meanY), axisY));
        p = _mm_add_ps(p, _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_unpackhi_epi8(planar, zero), zero)), scale), meanZ), axisZ));
        proj[y] = p;
        minProj = _mm_min_ps(minProj, p);
        maxProj = _mm_max_ps(maxProj, p);
    }

    minProj = _mm_min_ps(minProj, _mm_shuffle_ps(minProj, minProj, 0xB1));
    maxProj = _mm_max_ps(maxProj, _mm_shuffle_ps(maxProj, maxProj, 0xB1));
    minProj = _mm_min_ps(minProj, _mm_shuffle_ps(minProj, minProj, 0x4E));
    maxProj = _mm_max_ps(maxProj, _mm_shuffle_ps(maxProj, maxProj, 0x4E));

    const __m128 k255 = _mm_set1_ps(255.0f);
    __m128i endpoints = _mm_packs_epi32(_mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(minProj, axis), mean), k255)), _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(maxProj, axis), mean), k255)));
    uint32_t maxColor = _mm_cvtsi128_si32(_mm_packus_epi16(_mm_srli_si128(endpoints, 8), zero));
    uint32_t minColor = _mm_cvtsi128_si32(_mm_packus_epi16(endpoints, zero));
    uint32_t color0 = ((maxColor & 0xF8) | (((maxColor & 0xFC00) | ((maxColor >> 3) & 0x1F0000)) >> 2)) >> 3;
    uint32_t color1 = ((minColor & 0xF8) | (((minColor & 0xFC00) | ((minColor >> 3) & 0x1F0000)) >> 2)) >> 3;
    uint32_t indices = 0;

    if (color0 != color1) {
        __m128 indexScale = _mm_div_ps(_mm_set1_ps(3.0f), _mm_sub_ps(maxProj, minProj));
        __m128i index01 = _mm_packs_epi32(_mm_cvtps_epi32(_mm_mul_ps(_mm_sub_ps(proj[0], minProj), indexScale)), _mm_cvtps_epi32(_mm_mul_ps(_mm_sub_ps(proj[1], minProj), indexScale)));
        __m128i index23 = _mm_packs_epi32(_mm_cvtps_epi32(_mm_mul_ps(_mm_sub_ps(proj[2], minProj), indexScale)), _mm_cvtps_epi32(_mm_mul_ps(_mm_sub_ps(proj[3], minProj), indexScale)));
        __m128i one = _mm_cvtsi32_si128(1);
        one = _mm_shuffle_epi32(_mm_unpacklo_epi16(one, one), 0x00);
        __m128i packed = _mm_packs_epi16(_mm_add_epi16(index01, _mm_srli_epi16(_mm_add_epi16(index01, one), 1)), _mm_add_epi16(index23, _mm_srli_epi16(_mm_add_epi16(index23, one), 1)));

        int32_t bits0 = _mm_cvtsi128_si32(packed);
        int32_t result = (bits0 & 0x3) | (((bits0 & 0x300) | (((bits0 & 0x30000) | ((bits0 >> 6) & 0xC0000)) >> 6)) >> 6);
        packed = _mm_srli_si128(packed, 4);
        int32_t bits1 = _mm_cvtsi128_si32(packed);
        result |= (4 * ((bits1 & 0x300) | ((bits1 & 0x3) << 6))) | (((bits1 & 0x30000) | ((bits1 >> 6) & 0xC0000)) >> 4);
        packed = _mm_srli_si128(packed, 4);
        int32_t bits2 = _mm_cvtsi128_si32(packed);
        int32_t bits3 = _mm_cvtsi128_si32(_mm_srli_si128(packed, 4));
        result |= (0xC00000 & (bits2 >> 2)) | (16 * ((bits2 & 0x30000) | (((bits2 & 0x300) | ((bits2 & 0x3) << 6)) << 6)));
        result |= ((bits3 & static_cast<int32_t>(0xFF000000)) | (((bits3 & 0x30000) | (((bits3 & 0x300) | ((bits3 & 0x3) << 6)) << 6)) << 6)) << 6;
        indices = result ^ 0x55555555;

        if (color0 < color1) {
            uint32_t swap = color0;
            color0 = color1;
            color1 = swap;
            indices ^= 0x55555555;
        }
    }

    out[0] = color0 | (color1 << 16);
    out[1] = indices;
}

// VERIFY: 4cd9c152 manual
// OFFSET: 0x004E7650
void CCharacterComponent::DestroyRenderTextures() {
    for (int32_t section = 0; section < NUM_COMPONENT_SECTIONS; section++) {
        for (int32_t layer = 0; layer < 7; layer++) {
            if (this->m_section[section].m_layerItemDisplayId[layer] && this->m_section[section].m_layerTex[layer]) {
                TextureCacheDestroyTexture(this->m_section[section].m_layerTex[layer]);
                this->m_section[section].m_layerTex[layer] = nullptr;
            }
        }
    }
}

// VERIFY: 47092af8 auto
// OFFSET: 0x004E76D0
CharSectionsRec* CCharacterComponent::GetSectionsRecord(CHARTEXTUREVARIATIONS variation, int32_t variationIndex, int32_t colorIndex, bool* found) {
    return ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, variation, variationIndex, colorIndex, found);
}

// VERIFY: d8815212 manual
// OFFSET: 0x004E7700
void CCharacterComponent::ComponentCloseFingers(CM2Model* model, COMP_HAND_SLOT hand) {
    uint32_t first;
    uint32_t last;

    if (hand) {
        first = 13;
        last = 17;
    } else {
        first = 8;
        last = 12;
    }

    for (uint32_t bone = first; bone <= last; bone++) {
        model->SetBoneSequence(bone, 15, 0xFFFFFFFF, 0, 1.0f, 0, 1);
    }
}

// VERIFY: d95c47f8 manual
// OFFSET: 0x004E7750
void CCharacterComponent::ComponentOpenFingers(CM2Model* model, COMP_HAND_SLOT hand) {
    uint32_t first;
    uint32_t last;

    if (hand) {
        first = 13;
        last = 17;
    } else {
        first = 8;
        last = 12;
    }

    for (uint32_t bone = first; bone <= last; bone++) {
        model->UnsetBoneSequence(bone, 0, 1);
    }
}

// VERIFY: d3d12b40 auto
// OFFSET: 0x004E7790
void CCharacterComponent::ApplyMonsterGeosets(CM2Model* model, const CreatureDisplayInfoRec* displayRec) {
    if (!model || !displayRec || !displayRec->m_creatureGeosetData) {
        return;
    }

    for (uint32_t group = 100, shift = 0; group < 900; group += 100, shift += 4) {
        uint32_t geoset = (displayRec->m_creatureGeosetData >> shift) & 0xF;

        if (geoset) {
            model->SetGeometryVisible(group, group + 99, 0);
            model->SetGeometryVisible(group + geoset, group + geoset, 1);
        }
    }

    model->OptimizeVisibleGeometry();
}

// VERIFY: 6f039c48 manual
// OFFSET: 0x004E7800
int32_t CCharacterComponent::ComposeHelmModelFilePath(const ChrRacesRec* raceRec, const ItemDisplayInfoRec* displayRec, char* buffer, uint32_t size) {
    char path[260];
    strcpy(path, "Item\\ObjectComponents\\Head\\");
    strcat(path, displayRec->m_modelName[0]);

    char* ext = &path[strlen(path)];

    for (; ext > path; ext--) {
        if (*ext == '.') {
            break;
        }
    }

    if (ext == path) {
        ext = &path[strlen(path)];
    }

    *ext = '_';
    strcpy(ext + 1, raceRec->m_clientPrefix);
    strcat(path, s_fileDecorations[this->m_data.m_preferences.m_sexID]);
    strcat(path, ".mdx");
    strcpy(buffer, path);

    return 1;
}

// VERIFY: b4f875e0 manual
// OFFSET: 0x004E7940
GEOCOMPONENTLINKS CCharacterComponent::GetSheatheLink(SHEATHE_TYPE sheatheType, bool mainHand) {
    switch (sheatheType) {
        case SHEATHE_MAINHAND:
            return static_cast<GEOCOMPONENTLINKS>(ATTACH_SHEATH_OFFHAND - (mainHand != 0));

        case SHEATHE_BACK:
            return static_cast<GEOCOMPONENTLINKS>(ATTACH_BACK_OFFHAND - (mainHand != 0));

        case SHEATHE_HIP:
            return static_cast<GEOCOMPONENTLINKS>(ATTACH_HIP_OFFHAND - (mainHand != 0));

        case SHEATHE_SHIELD:
            return ATTACH_SHEATH_SHIELD;

        default:
            return ATTACH_NONE;
    }
}

// VERIFY: 0e3f578e manual
// OFFSET: 0x004E79A0
void CCharacterComponent::RemoveLinkpt(CM2Model* model, GEOCOMPONENTLINKS link) {
    if (link == ATTACH_NONE) {
        return;
    }

    if (link == ATTACH_SHIELD || link == ATTACH_HANDLEFT) {
        for (uint32_t bone = 13; bone <= 17; bone++) {
            model->UnsetBoneSequence(bone, 0, 1);
        }
    } else if (link == ATTACH_HANDRIGHT) {
        for (uint32_t bone = 8; bone <= 12; bone++) {
            model->UnsetBoneSequence(bone, 0, 1);
        }
    }

    if (model && model->IsLoaded(0, 0) && model->HasAttachment(link)) {
        model->DetachAllChildrenById(link);
    }
}

// VERIFY: 91d44195 auto
// OFFSET: 0x004E7A30
ITEM_SLOT CCharacterComponent::GetItemSlotForInvType(INVENTORY_TYPES invType) {
    switch (invType) {
        case INVTYPE_HEAD:
            return ITEMSLOT_HEAD;

        case INVTYPE_SHOULDERS:
            return ITEMSLOT_SHOULDER;

        case INVTYPE_BODY:
            return ITEMSLOT_SHIRT;

        case INVTYPE_CHEST:
        case INVTYPE_ROBE:
            return ITEMSLOT_CHEST;

        case INVTYPE_WAIST:
            return ITEMSLOT_BELT;

        case INVTYPE_LEGS:
            return ITEMSLOT_PANTS;

        case INVTYPE_FEET:
            return ITEMSLOT_BOOTS;

        case INVTYPE_WRISTS:
            return ITEMSLOT_BRACERS;

        case INVTYPE_HANDS:
            return ITEMSLOT_GLOVES;

        case INVTYPE_CLOAK:
            return ITEMSLOT_CAPE;

        case INVTYPE_TABARD:
            return ITEMSLOT_TABARD;

        default:
            return NUM_ITEM_SLOTS;
    }
}

// VERIFY: 3e0b0936 auto
// OFFSET: 0x004E7AF0
INVENTORY_SLOTS CCharacterComponent::GetInvSlotForItemSlot(ITEM_SLOT itemSlot) {
    switch (itemSlot) {
        case ITEMSLOT_HEAD:
            return INVSLOT_HEAD;

        case ITEMSLOT_SHOULDER:
            return INVSLOT_SHOULDER;

        case ITEMSLOT_SHIRT:
            return INVSLOT_BODY;

        case ITEMSLOT_CHEST:
            return INVSLOT_CHEST;

        case ITEMSLOT_BELT:
            return INVSLOT_WAIST;

        case ITEMSLOT_PANTS:
            return INVSLOT_LEGS;

        case ITEMSLOT_BOOTS:
            return INVSLOT_FEET;

        case ITEMSLOT_BRACERS:
            return INVSLOT_WRIST;

        case ITEMSLOT_GLOVES:
            return INVSLOT_HAND;

        case ITEMSLOT_TABARD:
            return INVSLOT_TABARD;

        case ITEMSLOT_CAPE:
            return INVSLOT_BACK;

        default:
            return INVSLOT_NONE;
    }
}

// VERIFY: 751bc9a8 auto
// OFFSET: 0x004E7B80
int32_t CCharacterComponent::GetNumSkins(int32_t raceID, int32_t sexID, int32_t classID, COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, classID);
    int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_SKIN, 0);

    if (numColors <= 0) {
        return 0;
    }

    int32_t count = 0;

    for (int32_t i = 0; i < numColors; i++) {
        CharSectionsRec* rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_SKIN, 0, i, nullptr);

        if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
            count++;
        }
    }

    return count;
}

// VERIFY: ab3047bb manual
// OFFSET: 0x004E7C10
int32_t CCharacterComponent::GetNumHairStylesForColor(int32_t raceID, int32_t sexID, int32_t classID, int32_t hairColorID, COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, classID);
    int32_t numVariations = ComponentGetNumVariations(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_HAIR);

    if (numVariations <= 0) {
        return 0;
    }

    int32_t count = 0;

    for (int32_t i = 0; i < numVariations; i++) {
        CharSectionsRec* rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_HAIR, i, hairColorID, nullptr);

        if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
            count++;
        }
    }

    return count;
}

// VERIFY: b4bdb701 manual
// OFFSET: 0x004E7CB0
int32_t CCharacterComponent::GetNumHairColorsForStyle(int32_t raceID, int32_t sexID, int32_t classID, int32_t hairStyleID, COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, classID);
    int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_HAIR, hairStyleID);

    if (numColors <= 0) {
        return 0;
    }

    int32_t count = 0;

    for (int32_t i = 0; i < numColors; i++) {
        CharSectionsRec* rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_HAIR, hairStyleID, i, nullptr);

        if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
            count++;
        }
    }

    return count;
}

// VERIFY: 6463d880 manual
// OFFSET: 0x004E7D50
int32_t CCharacterComponent::GetNumFacesForSkin(int32_t raceID, int32_t sexID, int32_t classID, int32_t skinID, COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, classID);
    int32_t numVariations = ComponentGetNumVariations(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_FACE);

    if (numVariations <= 0) {
        return 0;
    }

    int32_t count = 0;

    for (int32_t i = 0; i < numVariations; i++) {
        CharSectionsRec* rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_FACE, i, skinID, nullptr);

        if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
            count++;
        }
    }

    return count;
}

// VERIFY: 4a05dc36 manual
// OFFSET: 0x004E7DF0
int32_t CCharacterComponent::GetNumFacialFeaturesForHairColor(int32_t raceID, int32_t sexID, int32_t classID, int32_t hairColorID, COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, classID);
    int32_t numVariations = ComponentGetNumVariations(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_FACIALHAIR);

    if (!numVariations) {
        return CCharacterComponent::s_characterFacialHairStylesList[raceID * 2 + sexID];
    }

    int32_t count = 0;

    for (int32_t i = 0; i < numVariations; i++) {
        CharSectionsRec* rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_FACIALHAIR, i, hairColorID, nullptr);

        if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
            count++;
        }
    }

    return count;
}

// VERIFY: 14158ef3 manual
// OFFSET: 0x004E7E90
static int32_t ComponentGetSkinColor(int32_t raceID, int32_t sexID, int32_t index, COMPONENT_SELECTION selection) {
    int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_SKIN, 0);

    if (numColors <= 0) {
        return -1;
    }

    int32_t found = 0;

    for (int32_t i = 0; i < numColors; i++) {
        CharSectionsRec* rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_SKIN, 0, i, nullptr);

        if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
            if (index == found) {
                return rec->m_colorIndex;
            }

            found++;
        }
    }

    return -1;
}

// VERIFY: ea087dcf manual
// OFFSET: 0x004E7F20
static int32_t ComponentGetHairVariation(int32_t raceID, int32_t sexID, int32_t hairColorID, int32_t index, COMPONENT_SELECTION selection) {
    int32_t numVariations = ComponentGetNumVariations(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_HAIR);

    if (numVariations <= 0) {
        return -1;
    }

    int32_t found = 0;

    for (int32_t i = 0; i < numVariations; i++) {
        CharSectionsRec* rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_HAIR, i, hairColorID, nullptr);

        if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
            if (index == found) {
                return rec->m_variationIndex;
            }

            found++;
        }
    }

    return -1;
}

// VERIFY: 2ed94613 manual
// OFFSET: 0x004E7FB0
static int32_t ComponentGetHairColor(int32_t raceID, int32_t sexID, int32_t hairStyleID, int32_t index, COMPONENT_SELECTION selection) {
    int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_HAIR, hairStyleID);

    if (numColors <= 0) {
        return -1;
    }

    int32_t found = 0;

    for (int32_t i = 0; i < numColors; i++) {
        CharSectionsRec* rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_HAIR, hairStyleID, i, nullptr);

        if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
            if (index == found) {
                return rec->m_colorIndex;
            }

            found++;
        }
    }

    return -1;
}

// VERIFY: 06e939c6 manual
// OFFSET: 0x004E8050
static int32_t ComponentGetFaceVariation(int32_t raceID, int32_t sexID, int32_t skinID, int32_t index, COMPONENT_SELECTION selection) {
    int32_t numVariations = ComponentGetNumVariations(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_FACE);

    if (numVariations <= 0) {
        return -1;
    }

    int32_t found = 0;

    for (int32_t i = 0; i < numVariations; i++) {
        CharSectionsRec* rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_FACE, i, skinID, nullptr);

        if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
            if (index == found) {
                return rec->m_variationIndex;
            }

            found++;
        }
    }

    return -1;
}

// VERIFY: 99eac5a5 manual
// OFFSET: 0x004E80E0
int32_t CCharacterComponent::GetNthFacialFeatureIndex(int32_t raceID, int32_t sexID, int32_t classID, int32_t hairColorID, int32_t facialHairID, int32_t index, COMPONENT_SELECTION selection) {
    bool found;
    ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_FACIALHAIR, facialHairID, hairColorID, &found);

    if (!found) {
        if (index < static_cast<int32_t>(CCharacterComponent::s_characterFacialHairStylesList[raceID * 2 + sexID])) {
            return index;
        }

        return -1;
    }

    int32_t count = CCharacterComponent::GetNumFacialFeaturesForHairColor(raceID, sexID, classID, hairColorID, GetContextFromSelection(selection));

    if (count <= 0) {
        return -1;
    }

    int32_t matched = 0;

    for (int32_t i = 0; i < count; i++) {
        CharSectionsRec* rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, raceID, sexID, CHARTEXTUREVAR_FACIALHAIR, i, hairColorID, nullptr);

        if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
            if (index == matched) {
                return rec->m_variationIndex;
            }

            matched++;
        }
    }

    return -1;
}

// VERIFY: c44af7eb auto
// OFFSET: 0x004E81C0
char* CCharacterComponent::GetTabardBackgroundFileName(COMPONENT_SECTIONS section, int32_t background, char* buffer, int32_t size) {
    *buffer = '\0';

    if (section == SECTION_TORSO_UPPER) {
        SStrPrintf(buffer, size, "Textures\\GuildEmblems\\Background_%02d_TU_U", background);
    } else if (section == SECTION_TORSO_LOWER) {
        SStrPrintf(buffer, size, "Textures\\GuildEmblems\\Background_%02d_TL_U", background);
    }

    return buffer;
}

// VERIFY: 1c9627fe auto
// OFFSET: 0x004E8210
char* CCharacterComponent::GetTabardEmblemFileName(COMPONENT_SECTIONS section, int32_t emblem, int32_t color, char* buffer, int32_t size) {
    *buffer = '\0';

    if (section == SECTION_TORSO_UPPER) {
        SStrPrintf(buffer, size, "Textures\\GuildEmblems\\Emblem_%02d_%02d_TU_U", emblem, color);
    } else if (section == SECTION_TORSO_LOWER) {
        SStrPrintf(buffer, size, "Textures\\GuildEmblems\\Emblem_%02d_%02d_TL_U", emblem, color);
    }

    return buffer;
}

// VERIFY: 81ff3b61 auto
// OFFSET: 0x004E8260
char* CCharacterComponent::GetTabardBorderFileName(COMPONENT_SECTIONS section, int32_t border, int32_t color, char* buffer, int32_t size) {
    *buffer = '\0';

    if (section == SECTION_TORSO_UPPER) {
        SStrPrintf(buffer, size, "Textures\\GuildEmblems\\Border_%02d_%02d_TU_U", border, color);
    } else if (section == SECTION_TORSO_LOWER) {
        SStrPrintf(buffer, size, "Textures\\GuildEmblems\\Border_%02d_%02d_TL_U", border, color);
    }

    return buffer;
}

// VERIFY: 9acefab0 manual
// OFFSET: 0x004E82B0
void CCharacterComponent::SetItemSlotUsingAttachSlot(ITEM_SLOT itemSlot, GEOCOMPONENTLINKS link) {
    if (static_cast<uint32_t>(link) < NUM_ATTACH_LINKS) {
        this->m_itemSlotForAttachSlot[link] = itemSlot;
    }
}

// VERIFY: 1db943d8 manual
// OFFSET: 0x004E82D0
void CCharacterComponent::PasteCrappyGreen(MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcSize, TCTEXTUREINFO& srcInfo, uint32_t srcMipLevel, int32_t dstMipOffset) {
    uint32_t width = srcSize.x;
    uint32_t height = srcSize.y;

    for (uint32_t mip = srcMipLevel; mip < srcInfo.m_mipCount; mip++) {
        uint8_t* dst = reinterpret_cast<uint8_t*>(dstMips->m_mip[mip + dstMipOffset]) + dstStride * dstPos.y + 4 * dstPos.x;

        for (uint8_t* end = dst + dstStride * height; dst < end; dst += dstStride) {
            uint32_t* pixel = reinterpret_cast<uint32_t*>(dst);

            for (uint32_t x = 0; x < width; x++) {
                pixel[x] = 0xFF00FF00;
            }
        }

        width = (width >> 1) ? width >> 1 : 1;
        height = (height >> 1) ? height >> 1 : 1;
        dstPos.x >>= 1;
        dstPos.y >>= 1;
        dstStride >>= 1;
    }
}

// VERIFY: 6c996c19 manual
// OFFSET: 0x004E83A0
void CCharacterComponent::PasteOpaque(HTEXTURECACHE texture, const BlpPalPixel* pal, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo, uint32_t srcMipLevel, int32_t dstMipOffset) {
    uint32_t srcWidth = srcInfo.m_width;
    uint32_t width = srcSize.x;
    uint32_t height = srcSize.y;

    if (srcMipLevel) {
        srcWidth >>= srcMipLevel;
    }

    for (uint32_t mip = srcMipLevel; mip < srcInfo.m_mipCount; mip++) {
        const uint8_t* src = static_cast<const uint8_t*>(TextureCacheGetMip(texture, mip)) + srcPos.x + srcWidth * srcPos.y;
        uint8_t* dst = reinterpret_cast<uint8_t*>(dstMips->m_mip[mip + dstMipOffset]) + dstStride * dstPos.y + 4 * dstPos.x;

        for (const uint8_t* end = src + srcWidth * height; src < end; src += srcWidth, dst += dstStride) {
            uint32_t* pixel = reinterpret_cast<uint32_t*>(dst);

            for (uint32_t x = 0; x < width; x++) {
                const BlpPalPixel& color = pal[src[x]];
                pixel[x] = color.m_b | ((color.m_g | ((color.m_r | 0xFFFFFF00) << 8)) << 8);
            }
        }

        width = (width >> 1) ? width >> 1 : 1;
        height = (height >> 1) ? height >> 1 : 1;
        dstPos.x >>= 1;
        dstPos.y >>= 1;
        srcPos.x >>= 1;
        srcPos.y >>= 1;
        srcWidth >>= 1;
        dstStride >>= 1;
    }
}

// VERIFY: 212be31e manual
// OFFSET: 0x004E84F0
void CCharacterComponent::PasteTransparent1Bit(HTEXTURECACHE texture, const BlpPalPixel* pal, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo, uint32_t srcMipLevel, int32_t dstMipOffset) {
    uint32_t srcWidth = srcInfo.m_width;
    uint32_t width = srcSize.x;
    uint32_t height = srcSize.y;

    if (srcMipLevel) {
        srcWidth >>= srcMipLevel;
    }

    for (uint32_t mip = srcMipLevel; mip < srcInfo.m_mipCount; mip++) {
        const uint8_t* src = static_cast<const uint8_t*>(TextureCacheGetMip(texture, mip));
        uint8_t* dst = reinterpret_cast<uint8_t*>(dstMips->m_mip[mip + dstMipOffset]) + dstStride * dstPos.y + 4 * dstPos.x;
        const uint8_t* end = src + srcWidth * height;
        const uint8_t* alpha = end;

        for (; src < end; src += srcWidth, dst += dstStride, alpha += srcWidth >> 3) {
            uint32_t* pixel = reinterpret_cast<uint32_t*>(dst);

            for (uint32_t x = 0; x < width; x++) {
                if ((1 << (x & 7)) & alpha[x >> 3]) {
                    const BlpPalPixel& color = pal[src[x]];
                    pixel[x] = color.m_b | ((color.m_g | ((color.m_r | 0xFFFFFF00) << 8)) << 8);
                }
            }
        }

        width = (width >> 1) ? width >> 1 : 1;
        height = (height >> 1) ? height >> 1 : 1;
        dstPos.x >>= 1;
        dstPos.y >>= 1;
        srcPos.x >>= 1;
        srcPos.y >>= 1;
        srcWidth >>= 1;
        dstStride >>= 1;
    }
}

// VERIFY: d1dc9035 manual
// OFFSET: 0x004E8660
void CCharacterComponent::PasteTransparent4Bit(HTEXTURECACHE texture, const BlpPalPixel* pal, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo, uint32_t srcMipLevel, int32_t dstMipOffset) {
    uint32_t srcWidth = srcInfo.m_width;
    uint32_t width = srcSize.x;
    uint32_t height = srcSize.y;

    if (srcMipLevel) {
        srcWidth >>= srcMipLevel;
    }

    for (uint32_t mip = srcMipLevel; mip < srcInfo.m_mipCount; mip++) {
        const uint8_t* src = static_cast<const uint8_t*>(TextureCacheGetMip(texture, mip));
        const uint8_t* end = src + srcWidth * height;
        const uint8_t* alpha = end;
        uint8_t* dst = reinterpret_cast<uint8_t*>(dstMips->m_mip[mip + dstMipOffset]) + dstStride * dstPos.y + 4 * dstPos.x;

        for (; src < end; src += srcWidth, dst += dstStride, alpha += srcWidth >> 1) {
            uint8_t* pixel = dst;

            for (uint32_t x = 0; x < width; x++, pixel += 4) {
                const BlpPalPixel& color = pal[src[x]];
                uint32_t shift = 4 * (x & 1);
                uint32_t a = (alpha[x] & (0xF << shift)) >> shift;
                a |= a << 4;
                pixel[3] = 0xFF;
                pixel[2] = ((255 - a) * pixel[2] + a * color.m_r) >> 8;
                pixel[1] = ((255 - a) * pixel[1] + a * color.m_g) >> 8;
                pixel[0] = ((255 - a) * pixel[0] + a * color.m_b) >> 8;
            }
        }

        width = (width >> 1) ? width >> 1 : 1;
        height = (height >> 1) ? height >> 1 : 1;
        dstPos.x >>= 1;
        dstPos.y >>= 1;
        srcPos.x >>= 1;
        srcPos.y >>= 1;
        dstStride >>= 1;
        srcWidth >>= 1;
    }
}

// VERIFY: 4d979816 manual
// OFFSET: 0x004E8840
void CCharacterComponent::PasteTransparent8Bit(HTEXTURECACHE texture, const BlpPalPixel* pal, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo, uint32_t srcMipLevel, int32_t dstMipOffset) {
    uint32_t srcWidth = srcInfo.m_width;
    uint32_t width = srcSize.x;
    uint32_t height = srcSize.y;

    if (srcMipLevel) {
        srcWidth >>= srcMipLevel;
    }

    for (uint32_t mip = srcMipLevel; mip < srcInfo.m_mipCount; mip++) {
        const uint8_t* src = static_cast<const uint8_t*>(TextureCacheGetMip(texture, mip));
        const uint8_t* end = src + srcWidth * height;
        const uint8_t* alpha = end;
        uint8_t* dst = reinterpret_cast<uint8_t*>(dstMips->m_mip[mip + dstMipOffset]) + dstStride * dstPos.y + 4 * dstPos.x;

        for (; src < end; src += srcWidth, alpha += srcWidth, dst += dstStride) {
            uint8_t* pixel = dst;

            for (uint32_t x = 0; x < width; x++, pixel += 4) {
                const BlpPalPixel& color = pal[src[x]];
                uint32_t a = alpha[x];
                pixel[3] = 0xFF;
                pixel[2] = ((255 - a) * pixel[2] + a * color.m_r) >> 8;
                pixel[1] = ((255 - a) * pixel[1] + a * color.m_g) >> 8;
                pixel[0] = ((255 - a) * pixel[0] + a * color.m_b) >> 8;
            }
        }

        width = (width >> 1) ? width >> 1 : 1;
        height = (height >> 1) ? height >> 1 : 1;
        dstPos.x >>= 1;
        dstPos.y >>= 1;
        srcPos.x >>= 1;
        srcPos.y >>= 1;
        srcWidth >>= 1;
        dstStride >>= 1;
    }
}

// VERIFY: 8ec155b7 manual
// OFFSET: 0x004E89F0
void CCharacterComponent::PasteOpaqueScale(HTEXTURECACHE texture, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo) {
    uint32_t srcWidth = srcInfo.m_width;
    uint32_t width = srcSize.x;
    uint32_t height = srcSize.y;
    BlpPalPixel* pal = TextureCacheGetPal(texture);

    if (!pal) {
        return;
    }

    srcPos.x >>= 1;
    srcPos.y >>= 1;
    const uint8_t* src = static_cast<const uint8_t*>(TextureCacheGetMip(texture, 0)) + srcPos.x + srcWidth * srcPos.y;
    uint8_t* row = reinterpret_cast<uint8_t*>(dstMips->m_mip[0]) + dstStride * dstPos.y + 4 * dstPos.x;

    for (uint32_t y = 0; y < height; y++, row += dstStride) {
        uint32_t y0 = y >> 1;
        uint32_t y1 = y0 + 1;

        if (y1 >= (height >> 1) - 1) {
            y1 = (height >> 1) - 1;
        }

        uint8_t* pixel = row;

        if (y & 1) {
            const uint8_t* line0 = src + srcWidth * y0;
            const uint8_t* line1 = src + srcWidth * y1;

            for (uint32_t x = 0; x < width; x++, pixel += 4) {
                uint32_t x0 = x >> 1;
                uint32_t x1 = x0 + 1;

                if (x1 >= (width >> 1) - 1) {
                    x1 = (width >> 1) - 1;
                }

                const BlpPalPixel& c00 = pal[line0[x0]];

                if (x & 1) {
                    const BlpPalPixel& c01 = pal[line0[x1]];
                    const BlpPalPixel& c10 = pal[line1[x0]];
                    const BlpPalPixel& c11 = pal[line1[x1]];
                    pixel[3] = 0xFF;
                    pixel[2] = (c00.m_r + c01.m_r + c10.m_r + c11.m_r) >> 2;
                    pixel[1] = (c00.m_g + c01.m_g + c10.m_g + c11.m_g) >> 2;
                    pixel[0] = (c01.m_b + c10.m_b + c11.m_b + c00.m_b) >> 2;
                } else {
                    const BlpPalPixel& c10 = pal[line1[x0]];
                    pixel[3] = 0xFF;
                    pixel[2] = (c00.m_r + c10.m_r) >> 1;
                    pixel[1] = (c00.m_g + c10.m_g) >> 1;
                    pixel[0] = (c00.m_b + c10.m_b) >> 1;
                }
            }
        } else {
            const uint8_t* line0 = src + srcWidth * y0;

            for (uint32_t x = 0; x < width; x++, pixel += 4) {
                uint32_t x1 = (x >> 1) + 1;

                if (x1 >= (width >> 1) - 1) {
                    x1 = (width >> 1) - 1;
                }

                const BlpPalPixel& c00 = pal[line0[x >> 1]];

                if (x & 1) {
                    const BlpPalPixel& c01 = pal[line0[x1]];
                    pixel[3] = 0xFF;
                    pixel[2] = (c00.m_r + c01.m_r) >> 1;
                    pixel[1] = (c00.m_g + c01.m_g) >> 1;
                    pixel[0] = (c00.m_b + c01.m_b) >> 1;
                } else {
                    pixel[3] = 0xFF;
                    pixel[2] = c00.m_r;
                    pixel[1] = c00.m_g;
                    pixel[0] = c00.m_b;
                }
            }
        }
    }

    int32_t halfWidth = srcSize.x >> 1;
    srcSize.x = halfWidth < 1 ? 1 : halfWidth;
    int32_t halfHeight = srcSize.y >> 1;
    srcSize.y = halfHeight < 1 ? 1 : halfHeight;
    dstPos.x >>= 1;
    dstPos.y >>= 1;
    CCharacterComponent::PasteOpaque(texture, pal, dstMips, dstPos, dstStride >> 1, srcPos, srcSize, srcInfo, 0, 1);
}

// VERIFY: 049c43f4 auto
// OFFSET: 0x004E8D30
CTexture* CreateTexture(const char* fileName, CStatus* status) {
    CGxTexFlags flags(GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1);

    return TextureCreate(fileName, flags, status, 0);
}

// VERIFY: 11ef4f44 auto
// OFFSET: 0x004E8D70
void CComponentRequest::AddLayer(uint8_t section, HTEXTURECACHE texture) {
    if (this->m_layerCount < sizeof(this->m_layerTextures) / sizeof(this->m_layerTextures[0])) {
        this->m_layerTextures[this->m_layerCount] = TextureCacheCopyTexture(texture);
        this->m_layerSections[this->m_layerCount++] = section;
    }
}

// VERIFY: 1e391893 auto
// OFFSET: 0x004E8DB0
void CComponentRequest::DestroyCachedTextures() {
    for (uint32_t i = 0; i < NUM_COMPONENT_SECTIONS; i++) {
        TextureCacheDestroyTexture(this->m_baseTextures[i]);
        this->m_baseTextures[i] = nullptr;
    }

    for (uint32_t i = 0; i < this->m_layerCount; i++) {
        TextureCacheDestroyTexture(this->m_layerTextures[i]);
        this->m_layerTextures[i] = nullptr;
        this->m_layerSections[i] = 0xFF;
    }

    this->m_layerCount = 0;
}

// VERIFY: 29f640c9 auto
// OFFSET: 0x004E8E20
void CCharacterComponent::SetDirtySections(uint32_t mask) {
    this->m_dirtySections |= mask;

    if (this->m_request) {
        this->m_request->m_flags &= ~0x1;
        this->m_request = nullptr;
    }

    this->m_flags &= ~0x8;
}

// VERIFY: c73fe2d4 auto
// OFFSET: 0x004E8E50
void CCharacterComponent::SetTexturesDirty() {
    this->m_flags |= 0x1;

    if (this->m_request) {
        this->m_request->m_flags &= ~0x1;
        this->m_request = nullptr;
    }

    this->m_flags &= ~0x8;
}

// VERIFY: ca5f5399 auto
// OFFSET: 0x004E8E70
void CCharacterComponent::CreateRequestAU() {
    this->m_request->m_baseTextures[SECTION_ARM_UPPER] = TextureCacheCopyTexture(this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0]);

    for (int32_t layer = 0; layer < 2; layer++) {
        if ((1 << layer) & this->m_section[SECTION_ARM_UPPER].m_layerMask) {
            this->m_request->AddLayer(SECTION_ARM_UPPER, this->m_section[SECTION_ARM_UPPER].m_layerTex[layer]);
        }
    }
}

// VERIFY: 48a06e0d auto
// OFFSET: 0x004E8F00
void CCharacterComponent::CreateRequestAL() {
    this->m_request->m_baseTextures[SECTION_ARM_LOWER] = TextureCacheCopyTexture(this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0]);

    for (int32_t layer = 0; layer < 7; layer++) {
        if ((1 << layer) & this->m_section[SECTION_ARM_LOWER].m_layerMask) {
            this->m_request->AddLayer(SECTION_ARM_LOWER, this->m_section[SECTION_ARM_LOWER].m_layerTex[layer]);
        }
    }
}

// VERIFY: 27aef1fc auto
// OFFSET: 0x004E8F90
void CCharacterComponent::CreateRequestHA() {
    this->m_request->m_baseTextures[SECTION_HAND] = TextureCacheCopyTexture(this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0]);

    if (this->m_section[SECTION_HAND].m_layerMask & 0x1) {
        this->m_request->AddLayer(SECTION_HAND, this->m_section[SECTION_HAND].m_layerTex[0]);
    }
}

// VERIFY: 8b4affb8 auto
// OFFSET: 0x004E8FF0
void CCharacterComponent::CreateRequestHU() {
    CharSectionsRec* skinRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, this->m_data.m_preferences.m_skinID, nullptr);

    if (skinRec && (skinRec->m_flags & 0x8)) {
        this->m_request->m_baseTextures[SECTION_HEAD_UPPER] = TextureCacheCopyTexture(this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0]);
    }

    if (this->m_baseVariation[CHARTEXTUREVAR_FACE].m_texture[1]) {
        this->m_request->AddLayer(SECTION_HEAD_UPPER, this->m_baseVariation[CHARTEXTUREVAR_FACE].m_texture[1]);
    }

    if (this->m_baseVariation[CHARTEXTUREVAR_FACIALHAIR].m_texture[1]) {
        this->m_request->AddLayer(SECTION_HEAD_UPPER, this->m_baseVariation[CHARTEXTUREVAR_FACIALHAIR].m_texture[1]);
    }

    if (this->m_baseVariation[CHARTEXTUREVAR_HAIR].m_texture[2]) {
        this->m_request->AddLayer(SECTION_HEAD_UPPER, this->m_baseVariation[CHARTEXTUREVAR_HAIR].m_texture[2]);
    }
}

// VERIFY: 6b9fa30d auto
// OFFSET: 0x004E90E0
void CCharacterComponent::CreateRequestHL() {
    CharSectionsRec* skinRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, this->m_data.m_preferences.m_skinID, nullptr);

    if (skinRec && (skinRec->m_flags & 0x8)) {
        this->m_request->m_baseTextures[SECTION_HEAD_LOWER] = TextureCacheCopyTexture(this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0]);
    }

    if (this->m_baseVariation[CHARTEXTUREVAR_FACE].m_texture[0]) {
        this->m_request->AddLayer(SECTION_HEAD_LOWER, this->m_baseVariation[CHARTEXTUREVAR_FACE].m_texture[0]);
    }

    if (this->m_baseVariation[CHARTEXTUREVAR_FACIALHAIR].m_texture[0]) {
        this->m_request->AddLayer(SECTION_HEAD_LOWER, this->m_baseVariation[CHARTEXTUREVAR_FACIALHAIR].m_texture[0]);
    }

    if (this->m_baseVariation[CHARTEXTUREVAR_HAIR].m_texture[1]) {
        this->m_request->AddLayer(SECTION_HEAD_LOWER, this->m_baseVariation[CHARTEXTUREVAR_HAIR].m_texture[1]);
    }
}

// VERIFY: 5d1d019e manual
// OFFSET: 0x004E91D0
void CCharacterComponent::CreateRequestTU() {
    this->m_request->m_baseTextures[SECTION_TORSO_UPPER] = TextureCacheCopyTexture(this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0]);

    if (!(this->m_section[SECTION_TORSO_UPPER].m_layerMask & 0x7) && this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[1]) {
        this->m_request->AddLayer(SECTION_TORSO_UPPER, this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[1]);
    }

    for (int32_t layer = 0; layer < 5; layer++) {
        if ((1 << layer) & this->m_section[SECTION_TORSO_UPPER].m_layerMask) {
            this->m_request->AddLayer(SECTION_TORSO_UPPER, this->m_section[SECTION_TORSO_UPPER].m_layerTex[layer]);
        }
    }
}

// VERIFY: 0a0fcd15 auto
// OFFSET: 0x004E9280
void CCharacterComponent::CreateRequestTL() {
    this->m_request->m_baseTextures[SECTION_TORSO_LOWER] = TextureCacheCopyTexture(this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0]);

    for (int32_t layer = 0; layer < 7; layer++) {
        if ((1 << layer) & this->m_section[SECTION_TORSO_LOWER].m_layerMask) {
            this->m_request->AddLayer(SECTION_TORSO_LOWER, this->m_section[SECTION_TORSO_LOWER].m_layerTex[layer]);
        }
    }
}

// VERIFY: c8aaa938 manual
// OFFSET: 0x004E9310
void CCharacterComponent::CreateRequestLU() {
    this->m_request->m_baseTextures[SECTION_LEG_UPPER] = TextureCacheCopyTexture(this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0]);

    if (((this->m_flags & 0x20) || !(this->m_section[SECTION_LEG_UPPER].m_layerMask & 0x3)) && this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[0]) {
        this->m_request->AddLayer(SECTION_LEG_UPPER, this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[0]);
    }

    int32_t first = (this->m_flags & 0x20) != 0;
    int32_t last = 3 - first;

    for (int32_t layer = first; layer < last; layer++) {
        if ((1 << layer) & this->m_section[SECTION_LEG_UPPER].m_layerMask) {
            this->m_request->AddLayer(SECTION_LEG_UPPER, this->m_section[SECTION_LEG_UPPER].m_layerTex[layer]);
        }
    }
}

// VERIFY: 6ef99de9 manual
// OFFSET: 0x004E9400
void CCharacterComponent::CreateRequestLL() {
    this->m_request->m_baseTextures[SECTION_LEG_LOWER] = TextureCacheCopyTexture(this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0]);

    int32_t first = (this->m_flags & 0x20) != 0;
    int32_t last = 2 * ((this->m_flags & 0x20) == 0) + 4;

    for (int32_t layer = first; layer < last; layer++) {
        if ((1 << layer) & this->m_section[SECTION_LEG_LOWER].m_layerMask) {
            this->m_request->AddLayer(SECTION_LEG_LOWER, this->m_section[SECTION_LEG_LOWER].m_layerTex[layer]);
        }
    }
}

// VERIFY: ce9392ca auto
// OFFSET: 0x004E94B0
void CCharacterComponent::CreateRequestFO() {
    this->m_request->m_baseTextures[SECTION_FOOT] = TextureCacheCopyTexture(this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0]);

    if (this->m_section[SECTION_FOOT].m_layerMask & 0x1) {
        this->m_request->AddLayer(SECTION_FOOT, this->m_section[SECTION_FOOT].m_layerTex[0]);
    }
}

// VERIFY: b56204ff auto
// OFFSET: 0x004E9510
void CCharacterComponent::UpdateSections() {
    if (this->m_flags & 0x1) {
        GxTexUpdate(TextureGetGxTex(this->m_baseSkinTexture, 1, nullptr), 0, 0, s_textureSize, s_textureSize, 1);
        return;
    }

    for (int32_t section = 0; section < NUM_COMPONENT_SECTIONS; section++) {
        if ((1 << section) & this->m_dirtySections) {
            const CompSectionInfo& info = s_sectionInfo[section];
            GxTexUpdate(TextureGetGxTex(this->m_baseSkinTexture, 1, nullptr), info.m_pos.x, info.m_pos.y, info.m_pos.x + info.m_size.x, info.m_pos.y + info.m_size.y, 1);
        }
    }
}

// VERIFY: 58d8dc90 manual
// OFFSET: 0x004E95B0
static void CompressBlock(const uint8_t* src, uint32_t stride, uint8_t* out) {
    float colors[16][3];

    for (int32_t y = 0; y < 4; y++) {
        for (int32_t x = 0; x < 4; x++) {
            const uint8_t* pixel = src + y * stride + 4 * x;
            float* color = colors[y * 4 + x];
            color[0] = pixel[0] * 0.0039215689f;
            color[1] = pixel[1] * 0.0039215689f;
            color[2] = pixel[2] * 0.0039215689f;
        }
    }

    float mean[3] = { 0.0f, 0.0f, 0.0f };

    for (int32_t i = 0; i < 16; i++) {
        mean[0] = mean[0] + colors[i][0] * 0.0625f;
        mean[1] = mean[1] + colors[i][1] * 0.0625f;
        mean[2] = mean[2] + colors[i][2] * 0.0625f;
    }

    float c00 = 0.0f;
    float c01 = 0.0f;
    float c02 = 0.0f;
    float c11 = 0.0f;
    float c12 = 0.0f;
    float c22 = 0.0f;

    for (int32_t i = 0; i < 16; i++) {
        float d0 = colors[i][0] - mean[0];
        float d1 = colors[i][1] - mean[1];
        float d2 = colors[i][2] - mean[2];
        c00 = c00 + d0 * d0;
        c02 = c02 + d0 * d2;
        c11 = d1 * d1 + c11;
        c01 = c01 + d1 * d0;
        c12 = c12 + d1 * d2;
        c22 = d2 * d2 + c22;
    }

    float axis[3] = { 1.0f, 1.0f, 1.0f };

    for (int32_t i = 0; i < 8; i++) {
        float x = c00 * axis[0] + axis[2] * c02 + axis[1] * c01;
        float y = axis[2] * c12 + axis[1] * c11 + axis[0] * c01;
        float z = axis[2] * c22 + axis[1] * c12 + axis[0] * c02;
        float largest = x <= y ? y : x;

        if (largest <= z) {
            largest = z;
        }

        float inv = 0.0f == largest ? 0.0f : 1.0f / largest;
        axis[1] = y * inv;
        axis[0] = x * inv;
        axis[2] = z * inv;
    }

    float length = axis[2] * axis[2] + axis[1] * axis[1] + axis[0] * axis[0];

    if (0.0f != length) {
        float inv = 1.0f / sqrt(length);
        axis[0] = axis[0] * inv;
        axis[1] = axis[1] * inv;
        axis[2] = axis[2] * inv;
    }

    float minProj = 1.0f;
    float maxProj = -1.0f;

    for (int32_t i = 0; i < 16; i++) {
        float proj = (colors[i][0] - mean[0]) * axis[0] + (colors[i][1] - mean[1]) * axis[1] + (colors[i][2] - mean[2]) * axis[2];

        if (proj < minProj) {
            minProj = proj;
        }

        if (maxProj < proj) {
            maxProj = proj;
        }
    }

    uint8_t low[4];
    uint8_t high[3];

    for (int32_t i = 0; i < 3; i++) {
        int32_t value = static_cast<int32_t>((minProj * axis[i] + mean[i]) * 255.0f + 0.5f);
        low[i] = value < 0 ? 0 : (value > 255 ? 0xFF : value);
        value = static_cast<int32_t>((maxProj * axis[i] + mean[i]) * 255.0f + 0.5f);
        high[i] = value < 0 ? 0 : (value > 255 ? 0xFF : value);
    }

    uint32_t color0 = (((((high[2] & ~7) << 5) | high[1]) & ~3) << 3) | (high[0] >> 3);
    uint32_t color1 = (((((low[2] & ~7) << 5) | low[1]) & ~3) << 3) | (low[0] >> 3);
    int32_t map[4] = { 1, 3, 2, 0 };

    if (color0 < color1) {
        uint32_t swap = color0;
        color0 = color1;
        color1 = swap;
        map[0] = 0;
        map[1] = 2;
        map[2] = 3;
        map[3] = 1;
    }

    out[0] = color0;
    out[1] = color0 >> 8;
    out[2] = color1;
    out[3] = color1 >> 8;

    if (color0 == color1) {
        out[6] = 0;
        out[5] = 0;
        out[4] = 0;
        out[7] = 0;
        return;
    }

    float indexScale = 3.0f / (maxProj - minProj);
    uint32_t indices = 0;

    for (uint32_t i = 0; i < 16; i++) {
        float proj = (colors[i][0] - mean[0]) * axis[0] + (colors[i][1] - mean[1]) * axis[1] + (colors[i][2] - mean[2]) * axis[2];
        uint32_t index = static_cast<uint32_t>((proj - minProj) * indexScale + 0.5f);
        indices |= map[index] << (2 * i);
    }

    out[4] = indices;
    out[7] = indices >> 24;
    out[5] = indices >> 8;
    out[6] = indices >> 16;
}

// VERIFY: b6708f12 manual
// OFFSET: 0x004E9BA0
static void ComputeMipLevels(MipBits* src, MipBits* dst) {
    int32_t size = s_textureSize;

    if ((OsGetProcessorFeatures() & 0x14) == 0x14) {
        for (uint32_t level = 0; level < CCharacterComponent::s_mipLevels; level++, size >>= 1) {
            if (size < 4) {
                CompressBlockSSE(reinterpret_cast<const uint8_t*>(src->m_mip[CCharacterComponent::s_mipLevels - 2]), 16, reinterpret_cast<uint32_t*>(dst->m_mip[level]));
                continue;
            }

            int32_t blocks = size / 4;
            uint32_t stride = 4 * size;
            const uint8_t* in = reinterpret_cast<const uint8_t*>(src->m_mip[level]);
            uint32_t* out = reinterpret_cast<uint32_t*>(dst->m_mip[level]);

            for (int32_t by = 0; by < blocks; by++) {
                for (int32_t bx = 0; bx < blocks; bx++) {
                    CompressBlockSSE(in, stride, out);
                    out += 2;
                    in += 16;
                }

                in += 3 * stride;
            }
        }
    } else {
        for (uint32_t level = 0; level < CCharacterComponent::s_mipLevels; level++, size >>= 1) {
            if (size < 4) {
                CompressBlock(reinterpret_cast<const uint8_t*>(src->m_mip[CCharacterComponent::s_mipLevels - 2]), 16, reinterpret_cast<uint8_t*>(dst->m_mip[level]));
                continue;
            }

            int32_t blocks = size / 4;
            uint32_t stride = 4 * size;
            const uint8_t* in = reinterpret_cast<const uint8_t*>(src->m_mip[level]);
            uint8_t* out = reinterpret_cast<uint8_t*>(dst->m_mip[level]);

            for (int32_t by = 0; by < blocks; by++) {
                for (int32_t bx = 0; bx < blocks; bx++) {
                    CompressBlock(in, stride, out);
                    out += 8;
                    in += 16;
                }

                in += 3 * stride;
            }
        }
    }
}

// VERIFY: 1f11ee93 manual
// OFFSET: 0x004E9D50
void CCharacterComponent::ValidateComponentData(ComponentData* data, COMPONENT_CONTEXT context) {
    CharacterPreferences& prefs = data->m_preferences;
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, prefs.m_classId);

    if (!ComponentValidateSkin(CCharacterComponent::s_chrVarArray, prefs.m_raceID, prefs.m_sexID, prefs.m_classId, prefs.m_skinID, context)) {
        int32_t numSkins = CCharacterComponent::GetNumSkins(prefs.m_raceID, prefs.m_sexID, prefs.m_classId, context);

        if (numSkins <= 0) {
            prefs.m_skinID = 0;
        } else {
            prefs.m_skinID = ComponentGetSkinColor(prefs.m_raceID, prefs.m_sexID, static_cast<int32_t>(prefs.m_skinID) % numSkins, selection);
        }
    }

    if (!ComponentValidateFace(CCharacterComponent::s_chrVarArray, prefs.m_raceID, prefs.m_sexID, prefs.m_classId, prefs.m_skinID, prefs.m_faceId, context)) {
        int32_t numFaces = CCharacterComponent::GetNumFacesForSkin(prefs.m_raceID, prefs.m_sexID, prefs.m_classId, prefs.m_skinID, context);

        if (numFaces <= 0) {
            prefs.m_faceId = 0;
        } else {
            prefs.m_faceId = ComponentGetFaceVariation(prefs.m_raceID, prefs.m_sexID, prefs.m_skinID, static_cast<int32_t>(prefs.m_faceId) % numFaces, selection);
        }
    }

    if (!ComponentValidateHair(CCharacterComponent::s_chrVarArray, prefs.m_raceID, prefs.m_sexID, prefs.m_classId, prefs.m_hairColorId, prefs.m_hairStyleId, context)) {
        int32_t numStyles = CCharacterComponent::GetNumHairStylesForColor(prefs.m_raceID, prefs.m_sexID, prefs.m_classId, prefs.m_hairColorId, context);

        if (numStyles > 0) {
            prefs.m_hairStyleId = ComponentGetHairVariation(prefs.m_raceID, prefs.m_sexID, prefs.m_hairColorId, static_cast<int32_t>(prefs.m_hairStyleId) % numStyles, selection);
        } else {
            int32_t numVariations = ComponentGetNumVariations(CCharacterComponent::s_chrVarArray, prefs.m_raceID, prefs.m_sexID, CHARTEXTUREVAR_HAIR);
            int32_t* numColors = static_cast<int32_t*>(alloca(sizeof(int32_t) * numVariations));
            int32_t numValid = 0;

            for (int32_t i = 0; i < numVariations; i++) {
                numColors[i] = CCharacterComponent::GetNumHairColorsForStyle(prefs.m_raceID, prefs.m_sexID, prefs.m_classId, i, context);

                if (numColors[i] > 0) {
                    numValid++;
                }
            }

            if (numValid <= 0) {
                prefs.m_hairColorId = 0;
                prefs.m_hairStyleId = 0;
            } else {
                for (int32_t i = 0; i < numVariations; i++) {
                    if (numColors[i] > 0 && static_cast<int32_t>(prefs.m_hairStyleId) % numValid == i) {
                        prefs.m_hairStyleId = i;
                        break;
                    }
                }

                prefs.m_hairColorId = ComponentGetHairColor(prefs.m_raceID, prefs.m_sexID, prefs.m_hairStyleId, static_cast<int32_t>(prefs.m_hairColorId) % numColors[prefs.m_hairStyleId], selection);
            }
        }
    }

    if (!ComponentValidateFacialFeature(CCharacterComponent::s_chrVarArray, CCharacterComponent::s_characterFacialHairStylesList, prefs.m_raceID, prefs.m_sexID, prefs.m_classId, prefs.m_hairColorId, prefs.m_facialHairId, context)) {
        int32_t numFeatures = CCharacterComponent::GetNumFacialFeaturesForHairColor(prefs.m_raceID, prefs.m_sexID, prefs.m_classId, prefs.m_hairColorId, context);

        if (numFeatures <= 0) {
            prefs.m_facialHairId = 0;
        } else {
            prefs.m_facialHairId = CCharacterComponent::GetNthFacialFeatureIndex(prefs.m_raceID, prefs.m_sexID, prefs.m_classId, prefs.m_hairColorId, prefs.m_facialHairId, static_cast<int32_t>(prefs.m_facialHairId) % numFeatures, selection);
        }
    }
}

// VERIFY: 43fe9206 auto
// OFFSET: 0x004EA000
static CharacterFacialHairStylesRec* GetConditionalFacialHairStyle(CharacterPreferences* prefs) {
    for (int32_t i = 0; i < g_characterFacialHairStylesDB.m_numRecords; i++) {
        CharacterFacialHairStylesRec* rec = (i < 0 || i >= g_characterFacialHairStylesDB.m_numRecords) ? nullptr : &g_characterFacialHairStylesDB.m_records[i];

        if (!rec) {
            return nullptr;
        }

        if (prefs->m_raceID == rec->m_raceID && prefs->m_sexID == rec->m_sexID && prefs->m_facialHairId == rec->m_variationID) {
            return rec;
        }
    }

    return nullptr;
}

// VERIFY: 82a8a223 auto
// OFFSET: 0x004EA050
static int32_t GetConditionalGeoset(CharacterPreferences* prefs) {
    for (int32_t i = 0; i < g_charHairGeosetsDB.m_numRecords; i++) {
        CharHairGeosetsRec* rec = (i < 0 || i >= g_charHairGeosetsDB.m_numRecords) ? nullptr : &g_charHairGeosetsDB.m_records[i];

        if (!rec) {
            return 1;
        }

        if (prefs->m_raceID == rec->m_raceID && prefs->m_sexID == rec->m_sexID && prefs->m_hairStyleId == rec->m_variationID) {
            if (rec->m_geosetID > 0) {
                return rec->m_geosetID;
            }

            return 1;
        }
    }

    return 1;
}

// VERIFY: 4a5d8b79 manual
// OFFSET: 0x004EA0B0
void CCharacterComponent::ReplaceExtraSkinTexture(const char* debug) {
    if (!ComponentValidateBase(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, this->m_data.m_preferences.m_skinID)) {
        return;
    }

    const char* textureName = CCharacterComponent::s_chrVarArray[this->m_data.m_preferences.m_raceID * 2 + this->m_data.m_preferences.m_sexID].m_variation[CHARTEXTUREVAR_SKIN].m_variation[0].m_color[this->m_data.m_preferences.m_skinID]->m_textureName[1];

    if (!*textureName) {
        return;
    }

    strcpy(s_pathEnd, textureName);
    CTexture* texture = CreateTexture(s_path, &s_status);

    if (texture) {
        this->m_data.m_model->ReplaceTexture(M2TEX_EXTRA_SKIN, texture);
        HandleClose(reinterpret_cast<HOBJECT>(texture));
    }
}

// VERIFY: d35306aa manual
// OFFSET: 0x004EA150
void CCharacterComponent::ReplaceHairTexture(int32_t hairStyleID, const char* debug) {
    if (!ComponentValidateBase(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR, hairStyleID, this->m_data.m_preferences.m_hairColorId)) {
        return;
    }

    const char* textureName = CCharacterComponent::s_chrVarArray[this->m_data.m_preferences.m_raceID * 2 + this->m_data.m_preferences.m_sexID].m_variation[CHARTEXTUREVAR_HAIR].m_variation[hairStyleID].m_color[this->m_data.m_preferences.m_hairColorId]->m_textureName[0];

    if (!*textureName) {
        return;
    }

    strcpy(s_pathEnd, textureName);
    CTexture* texture = CreateTexture(s_path, &s_status);

    if (texture) {
        this->m_data.m_model->ReplaceTexture(M2TEX_HAIR, texture);
        HandleClose(reinterpret_cast<HOBJECT>(texture));
    }
}

// VERIFY: 88329d82 manual
// OFFSET: 0x004EA1F0
void CCharacterComponent::LoadBaseVariation(CHARTEXTUREVARIATIONS variation, int32_t textureIndex, int32_t variationIndex, int32_t colorIndex, COMPONENT_SECTIONS section, const char* debug) {
    HTEXTURECACHE& texture = this->m_baseVariation[variation].m_texture[textureIndex];

    if (texture) {
        TextureCacheDestroyTexture(texture);
        texture = nullptr;
    }

    if (!ComponentValidateBase(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, variation, variationIndex, colorIndex)) {
        return;
    }

    const char* textureName = CCharacterComponent::s_chrVarArray[this->m_data.m_preferences.m_raceID * 2 + this->m_data.m_preferences.m_sexID].m_variation[variation].m_variation[variationIndex].m_color[colorIndex]->m_textureName[textureIndex];

    if (*textureName) {
        strcpy(s_pathEnd, textureName);
        texture = TextureCacheCreateTexture(s_path);

        if (!texture) {
            DebugLog("**** Unable to Load Texture %s\n", s_path);
        }
    }

    this->SetDirtySections(1 << section);
}

// VERIFY: 63815769 auto
// OFFSET: 0x004EA2F0
void CCharacterComponent::SetHairColor(int32_t hairColorID, bool loadHair, const char* debug) {
    if (!ComponentValidateBase(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR, this->m_data.m_preferences.m_hairStyleId, hairColorID)) {
        return;
    }

    this->m_data.m_preferences.m_hairColorId = hairColorID;
    ChrRacesRec* raceRec = g_chrRacesDB.GetRecord(this->m_data.m_preferences.m_raceID);

    if (!this->m_data.m_preferences.m_sexID && !this->m_data.m_preferences.m_hairStyleId && (raceRec->m_flags & 0x8)) {
        this->ReplaceHairTexture(1, debug);
    }

    this->ReplaceHairTexture(this->m_data.m_preferences.m_hairStyleId, debug);

    if (this->m_data.m_flags & 0x1) {
        return;
    }

    if (loadHair) {
        this->LoadBaseVariation(CHARTEXTUREVAR_HAIR, 1, this->m_data.m_preferences.m_hairStyleId, this->m_data.m_preferences.m_hairColorId, SECTION_HEAD_LOWER, debug);
        this->LoadBaseVariation(CHARTEXTUREVAR_HAIR, 2, this->m_data.m_preferences.m_hairStyleId, this->m_data.m_preferences.m_hairColorId, SECTION_HEAD_UPPER, debug);
    }

    this->LoadBaseVariation(CHARTEXTUREVAR_FACIALHAIR, 0, this->m_data.m_preferences.m_facialHairId, this->m_data.m_preferences.m_hairColorId, SECTION_HEAD_LOWER, debug);
    this->LoadBaseVariation(CHARTEXTUREVAR_FACIALHAIR, 1, this->m_data.m_preferences.m_facialHairId, this->m_data.m_preferences.m_hairColorId, SECTION_HEAD_UPPER, debug);
}

// VERIFY: 2d880572 manual
// OFFSET: 0x004EA3E0
void CCharacterComponent::SetHairStyle(int32_t hairStyleID, const char* debug) {
    if (!ComponentValidateBase(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR, hairStyleID, this->m_data.m_preferences.m_hairColorId)) {
        return;
    }

    this->m_data.m_preferences.m_hairStyleId = hairStyleID;
    this->m_data.m_geosets[CHARGEOSET_HAIR] = GetConditionalGeoset(&this->m_data.m_preferences);

    if (!(this->m_data.m_flags & 0x1)) {
        this->LoadBaseVariation(CHARTEXTUREVAR_HAIR, 1, hairStyleID, this->m_data.m_preferences.m_hairColorId, SECTION_HEAD_LOWER, debug);
        this->LoadBaseVariation(CHARTEXTUREVAR_HAIR, 2, this->m_data.m_preferences.m_hairStyleId, this->m_data.m_preferences.m_hairColorId, SECTION_HEAD_UPPER, debug);
    }

    this->SetHairColor(this->m_data.m_preferences.m_hairColorId, false, debug);
    this->m_flags |= 0x4;
    this->SetDirtySections(0x300);
}

// VERIFY: 901d909d auto
// OFFSET: 0x004EA490
void CCharacterComponent::SetFace(int32_t faceID, bool loadHair, const char* debug) {
    if (this->m_data.m_flags & 0x1) {
        return;
    }

    CharSectionsRec* skinRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, this->m_data.m_preferences.m_skinID, nullptr);

    if (skinRec && (skinRec->m_flags & 0x8)) {
        return;
    }

    if (!ComponentValidateBase(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACE, faceID, this->m_data.m_preferences.m_skinID)) {
        return;
    }

    this->m_data.m_preferences.m_faceId = faceID;
    this->LoadBaseVariation(CHARTEXTUREVAR_FACE, 0, faceID, this->m_data.m_preferences.m_skinID, SECTION_HEAD_LOWER, debug);
    this->LoadBaseVariation(CHARTEXTUREVAR_FACE, 1, this->m_data.m_preferences.m_faceId, this->m_data.m_preferences.m_skinID, SECTION_HEAD_UPPER, debug);

    if (loadHair) {
        this->LoadBaseVariation(CHARTEXTUREVAR_HAIR, 1, this->m_data.m_preferences.m_hairStyleId, this->m_data.m_preferences.m_hairColorId, SECTION_HEAD_LOWER, debug);
        this->LoadBaseVariation(CHARTEXTUREVAR_HAIR, 2, this->m_data.m_preferences.m_hairStyleId, this->m_data.m_preferences.m_hairColorId, SECTION_HEAD_UPPER, debug);
    }

    this->m_flags |= 0x4;
    this->SetDirtySections(0x300);
}

// VERIFY: 336da2ea auto
// OFFSET: 0x004EA590
void CCharacterComponent::SetBeardStyle(int32_t facialHairID, bool loadVariation, const char* debug) {
    if (facialHairID < 0 || facialHairID > static_cast<int32_t>(CCharacterComponent::s_characterFacialHairStylesList[this->m_data.m_preferences.m_raceID * 2 + this->m_data.m_preferences.m_sexID])) {
        return;
    }

    this->m_data.m_preferences.m_facialHairId = facialHairID;
    CharacterFacialHairStylesRec* styleRec = GetConditionalFacialHairStyle(&this->m_data.m_preferences);

    if (styleRec) {
        this->m_data.m_geosets[CHARGEOSET_BEARD] = styleRec->m_geoset[0] + 100;
        this->m_data.m_geosets[CHARGEOSET_MOUSTACHE] = styleRec->m_geoset[1] + 300;
        this->m_data.m_geosets[CHARGEOSET_SIDEBURN] = styleRec->m_geoset[2] + 200;
        this->m_data.m_geosets[CHARGEOSET_FACIALJEWELRY] = styleRec->m_geoset[3] + 1600;
        this->m_flags |= 0x4;
        this->m_data.m_geosets[CHARGEOSET_EYEEFFECTS] = styleRec->m_geoset[4] + 1700;

        if ((styleRec->m_geoset[0] || styleRec->m_geoset[1] || styleRec->m_geoset[2]) && (!this->m_baseVariation[CHARTEXTUREVAR_HAIR].m_texture[1] || !this->m_baseVariation[CHARTEXTUREVAR_HAIR].m_texture[2])) {
            this->ReplaceHairTexture(1, debug);
        }
    }

    if (this->m_data.m_flags & 0x1) {
        return;
    }

    if (loadVariation) {
        this->LoadBaseVariation(CHARTEXTUREVAR_FACIALHAIR, 0, this->m_data.m_preferences.m_facialHairId, this->m_data.m_preferences.m_hairColorId, SECTION_HEAD_LOWER, debug);
        this->LoadBaseVariation(CHARTEXTUREVAR_FACIALHAIR, 1, this->m_data.m_preferences.m_facialHairId, this->m_data.m_preferences.m_hairColorId, SECTION_HEAD_UPPER, debug);
    }

    this->SetDirtySections(0x300);
}

// VERIFY: 3c40af57 auto
// OFFSET: 0x004EA6B0
void CCharacterComponent::SetSkinColor(int32_t skinID, bool loadHair, bool renderAll, const char* debug) {
    this->m_data.m_preferences.m_skinID = skinID;

    if ((this->m_data.m_flags & 0x1) || !ComponentValidateBase(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, skinID)) {
        return;
    }

    int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0);
    CharSectionsRec* skinRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, skinID, nullptr);

    if (skinID < numColors && skinRec && !(skinRec->m_flags & 0x8)) {
        CharSectionsRec* underwearRec = this->GetSectionsRecord(VARIATION_UNDERWEAR, 0, skinID, nullptr);

        if (this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[0]) {
            TextureCacheDestroyTexture(this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[0]);
            this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[0] = nullptr;
        }

        if (this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[1]) {
            TextureCacheDestroyTexture(this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[1]);
            this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[1] = nullptr;
        }

        if (*underwearRec->m_textureName[0] && CopyStringToBuffer(underwearRec->m_textureName[0], s_pathEnd)) {
            this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[0] = TextureCacheCreateTexture(s_path);

            if (!this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[0]) {
                DebugLog("**** Unable to Load Texture %s\n", s_path);
            }
        }

        if (*underwearRec->m_textureName[1] && CopyStringToBuffer(underwearRec->m_textureName[1], s_pathEnd)) {
            this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[1] = TextureCacheCreateTexture(s_path);

            if (!this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[1]) {
                DebugLog("**** Unable to Load Texture %s\n", s_path);
            }
        }
    }

    this->ReplaceExtraSkinTexture(debug);
    this->LoadBaseVariation(CHARTEXTUREVAR_SKIN, 0, 0, this->m_data.m_preferences.m_skinID, SECTION_TORSO_UPPER, debug);
    this->SetFace(this->m_data.m_preferences.m_faceId, loadHair, debug);

    if (renderAll) {
        this->SetTexturesDirty();
    }

    this->m_dirtySections = 0xFFFFFFFF;

    if (this->m_request) {
        this->m_request->m_flags &= ~0x1;
        this->m_request = nullptr;
    }

    this->m_flags &= ~0x8;
}

// VERIFY: 0266cc45 manual
// OFFSET: 0x004EA880
void CCharacterComponent::FreeSectionTexture(COMPONENT_SECTIONS section, int32_t layer) {
    if (layer == -1) {
        return;
    }

    if (this->m_section[section].m_layerTex[layer]) {
        TextureCacheDestroyTexture(this->m_section[section].m_layerTex[layer]);
        this->m_section[section].m_layerTex[layer] = nullptr;
    }

    this->m_section[section].m_layerItemDisplayId[layer] = 0;
    this->m_section[section].m_layerMask &= ~(1 << layer);
}

// VERIFY: edb8c917 auto
// OFFSET: 0x004EA8F0
void CCharacterComponent::ComponentUtilAddItemVisual(CM2Model* model, int32_t itemVisual) {
    while (model->m_attachList) {
        model->m_attachList->DetachFromParent();
    }

    ItemVisualsRec* visualRec = g_itemVisualsDB.GetRecord(itemVisual);

    if (!visualRec) {
        return;
    }

    for (uint32_t i = 0; i < 5; i++) {
        ItemVisualEffectsRec* effectRec = g_itemVisualEffectsDB.GetRecord(visualRec->m_slot[i]);

        if (effectRec && *effectRec->m_model) {
            strcpy(s_pathEnd, effectRec->m_model);
            CM2Model* effectModel = model->m_scene->CreateModel(s_path, 0);

            if (effectModel) {
                effectModel->AttachToParent(model, i, nullptr, 0);
                effectModel->Release();
            }
        }
    }
}

// VERIFY: 7234ffe0 manual
// OFFSET: 0x004EA9E0
static void ReplaceParticleColor(int32_t particleColorID, CM2Model* model) {
    if (!model || !particleColorID) {
        return;
    }

    ParticleColorRec* colorRec = g_particleColorDB.GetRecord(particleColorID);

    if (colorRec) {
        for (uint32_t i = 0; i < 3; i++) {
            model->ReplaceParticleColor(i + 11, *reinterpret_cast<CImVector*>(&colorRec->m_start[i]), *reinterpret_cast<CImVector*>(&colorRec->m_mid[i]), *reinterpret_cast<CImVector*>(&colorRec->m_end[i]));
        }
    } else {
        CImVector green;
        *reinterpret_cast<uint32_t*>(&green) = 0xFF00FF00;

        for (uint32_t i = 0; i < 3; i++) {
            model->ReplaceParticleColor(i + 11, green, green, green);
        }
    }
}

// VERIFY: a5287802 manual
// OFFSET: 0x004EAA70
void CCharacterComponent::AddLink(CM2Model* parent, GEOCOMPONENTLINKS link, const char* modelPath, const char* texturePath, int32_t itemVisual, const ItemDisplayInfoRec* displayRec) {
    if (!parent) {
        return;
    }

    CM2Model* model = parent->m_scene->CreateModel(modelPath, 0);

    if (!model) {
        return;
    }

    CTexture* texture = CreateTexture(texturePath, &s_status);

    if (!texture) {
        model->Release();
        return;
    }

    model->ReplaceTexture(M2TEX_OBJECT_SKIN, texture);
    HandleClose(reinterpret_cast<HOBJECT>(texture));

    if (itemVisual > 0) {
        CCharacterComponent::ComponentUtilAddItemVisual(model, itemVisual);
    }

    parent->DetachAllChildrenById(link);
    model->AttachToParent(parent, link, nullptr, 0);

    if (link == ATTACH_HANDRIGHT) {
        CCharacterComponent::ComponentCloseFingers(parent, COMP_HAND_RIGHT);
    } else if (link == ATTACH_HANDLEFT) {
        CCharacterComponent::ComponentCloseFingers(parent, COMP_HAND_LEFT);
    }

    ReplaceParticleColor(displayRec->m_particleColorID, model);
    model->Release();
}

// VERIFY: 41de2a53 manual
// OFFSET: 0x004EAB50
CM2Model* CCharacterComponent::MakeAmmoModel(CM2Model* model, const ItemDisplayInfoRec* displayRec, uint32_t invType) {
    if (!displayRec) {
        return nullptr;
    }

    const char* modelName = displayRec->m_modelName[1];

    if (!*modelName || !*displayRec->m_modelTexture[1] || (invType != INVTYPE_AMMO && invType != INVTYPE_THROWN)) {
        return nullptr;
    }

    const char* folder = "Item\\ObjectComponents\\Ammo\\";

    if (invType == INVTYPE_THROWN) {
        folder = "Item\\ObjectComponents\\Weapon\\";
    }

    char modelPath[260];
    strcpy(modelPath, folder);
    strcat(modelPath, modelName);

    char texturePath[260];
    strcpy(texturePath, folder);
    strcat(texturePath, displayRec->m_modelTexture[1]);

    CM2Model* ammoModel = model->m_scene->CreateModel(modelPath, 0);

    if (!ammoModel) {
        return nullptr;
    }

    ammoModel->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    CTexture* texture = CreateTexture(texturePath, &s_status);

    if (!texture) {
        ammoModel->Release();
        return nullptr;
    }

    ammoModel->ReplaceTexture(M2TEX_OBJECT_SKIN, texture);
    HandleClose(reinterpret_cast<HOBJECT>(texture));
    CCharacterComponent::ComponentUtilAddItemVisual(ammoModel, displayRec->m_itemVisual);

    return ammoModel;
}

// VERIFY: eb4afbd7 manual
// OFFSET: 0x004EACD0
GEOCOMPONENTLINKS CCharacterComponent::AddHandItem(CM2Model* model, const ItemDisplayInfoRec* displayRec, INVENTORY_SLOTS invSlot, SHEATHE_TYPE sheatheType, bool sheathed, bool shield, bool mainHand, int32_t itemVisual) {
    if (!model || !displayRec || static_cast<uint32_t>(invSlot) > INVSLOT_TABARD) {
        return ATTACH_NONE;
    }

    const char* folder = "Item\\ObjectComponents\\Weapon\\";
    GEOCOMPONENTLINKS hand;
    GEOCOMPONENTLINKS sheath;

    if (invSlot == INVSLOT_MAINHAND || (invSlot == INVSLOT_RANGED && mainHand)) {
        hand = ATTACH_HANDRIGHT;
        sheath = CCharacterComponent::GetSheatheLink(sheatheType, true);
    } else if (invSlot == INVSLOT_OFFHAND) {
        hand = ATTACH_HANDLEFT;
        sheath = CCharacterComponent::GetSheatheLink(sheatheType, false);

        if (shield) {
            folder = "Item\\ObjectComponents\\Shield\\";
            hand = ATTACH_SHIELD;
        }
    } else if (invSlot == INVSLOT_RANGED) {
        hand = ATTACH_HANDLEFT;
        sheath = CCharacterComponent::GetSheatheLink(sheatheType, false);
    } else {
        return ATTACH_NONE;
    }

    CCharacterComponent::RemoveLinkpt(model, hand);
    CCharacterComponent::RemoveLinkpt(model, sheath);

    GEOCOMPONENTLINKS link = sheathed ? sheath : hand;

    if (model->IsLoaded(0, 0) && !model->HasAttachment(link)) {
        return ATTACH_NONE;
    }

    strcpy(s_buffer, folder);
    strcat(s_buffer, displayRec->m_modelName[0]);
    strcpy(s_pathEnd, s_buffer);

    strcpy(s_buffer, folder);
    strcat(s_buffer, displayRec->m_modelTexture[0]);
    strcat(s_buffer, ".blp");
    strcpy(s_pathEnd2, s_buffer);

    CCharacterComponent::AddLink(model, link, s_path, s_path2, itemVisual ? itemVisual : displayRec->m_itemVisual, displayRec);

    return link;
}

// VERIFY: 9faafec0 auto
// OFFSET: 0x004EAF70
void CCharacterComponent::RemoveVisuals(CM2Model* model) {
    model->DetachAllChildrenById(15);
    model->DetachAllChildrenById(16);
    model->DetachAllChildrenById(17);
    model->DetachAllChildrenById(18);
    model->DetachAllChildrenById(29);
    model->DetachAllChildrenById(19);
    model->DetachAllChildrenById(20);
    model->DetachAllChildrenById(38);
    model->DetachAllChildrenById(21);
    model->DetachAllChildrenById(22);
    model->DetachAllChildrenById(23);
    model->DetachAllChildrenById(24);
    model->DetachAllChildrenById(25);
    model->DetachAllChildrenById(34);
    model->DetachAllChildrenById(35);
    model->DetachAllChildrenById(12);
    model->m_emissiveTintR = 0.0f;
    model->m_emissiveTintG = 0.0f;
    model->m_fadeAlpha = 1.0f;
    model->m_emissiveTintB = 0.0f;
    model->m_diffuseTintR = 1.0f;
    model->m_diffuseTintG = 1.0f;
    model->m_diffuseTintB = 1.0f;
}

// VERIFY: eec03863 manual
// OFFSET: 0x004EB070
void CCharacterComponent::RemoveHandItem(CM2Model* model, INVENTORY_SLOTS invSlot, SHEATHE_TYPE sheatheType, bool shield) {
    if (!model || static_cast<uint32_t>(invSlot - INVSLOT_BACK) > 4) {
        return;
    }

    GEOCOMPONENTLINKS hand = ATTACH_NONE;
    GEOCOMPONENTLINKS sheath = ATTACH_NONE;

    if (invSlot == INVSLOT_MAINHAND) {
        hand = ATTACH_HANDRIGHT;
        sheath = CCharacterComponent::GetSheatheLink(sheatheType, true);
    } else if (invSlot > INVSLOT_MAINHAND && invSlot <= INVSLOT_RANGED) {
        hand = ATTACH_HANDLEFT;
        sheath = CCharacterComponent::GetSheatheLink(sheatheType, false);

        if (shield) {
            hand = ATTACH_SHIELD;
        }
    }

    CCharacterComponent::RemoveLinkpt(model, hand);
    CCharacterComponent::RemoveLinkpt(model, sheath);
}

// VERIFY: cdfacfc2 manual
// OFFSET: 0x004EB150
bool CCharacterComponent::SetNextSkin(COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0);

    if (numColors <= 0) {
        return false;
    }

    int32_t skinID = this->m_data.m_preferences.m_skinID + 1;

    if (skinID >= numColors) {
        skinID = 0;
    }

    if (skinID == static_cast<int32_t>(this->m_data.m_preferences.m_skinID)) {
        return false;
    }

    CharSectionsRec* skinRec;

    while (true) {
        skinRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, skinID, nullptr);
        CharSectionsRec* faceRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACE, this->m_data.m_preferences.m_faceId, skinID, nullptr);
        CharSectionsRec* underwearRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, VARIATION_UNDERWEAR, 0, skinID, nullptr);

        if (skinRec && ComponentFlagsMatch(skinRec->m_flags, selection) && faceRec && ComponentFlagsMatch(faceRec->m_flags, selection) && (context == COMPONENT_CONTEXT_2 || (underwearRec && ComponentFlagsMatch(underwearRec->m_flags, selection)))) {
            break;
        }

        if (++skinID >= numColors) {
            skinID = 0;
        }

        if (skinID == static_cast<int32_t>(this->m_data.m_preferences.m_skinID)) {
            return false;
        }
    }

    this->SetSkinColor(skinRec->m_colorIndex, true, true, nullptr);

    return true;
}

// VERIFY: 249fa1e9 manual
// OFFSET: 0x004EB290
bool CCharacterComponent::SetPrevSkin(COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0);

    if (numColors <= 0) {
        return false;
    }

    int32_t skinID = this->m_data.m_preferences.m_skinID - 1;

    if (skinID < 0) {
        skinID = numColors - 1;
    }

    if (skinID == static_cast<int32_t>(this->m_data.m_preferences.m_skinID)) {
        return false;
    }

    CharSectionsRec* skinRec;

    while (true) {
        skinRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, skinID, nullptr);
        CharSectionsRec* faceRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACE, this->m_data.m_preferences.m_faceId, skinID, nullptr);
        CharSectionsRec* underwearRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, VARIATION_UNDERWEAR, 0, skinID, nullptr);

        if (skinRec && ComponentFlagsMatch(skinRec->m_flags, selection) && faceRec && ComponentFlagsMatch(faceRec->m_flags, selection) && (context == COMPONENT_CONTEXT_2 || (underwearRec && ComponentFlagsMatch(underwearRec->m_flags, selection)))) {
            break;
        }

        if (--skinID < 0) {
            skinID = numColors - 1;
        }

        if (skinID == static_cast<int32_t>(this->m_data.m_preferences.m_skinID)) {
            return false;
        }
    }

    this->SetSkinColor(skinRec->m_colorIndex, true, true, nullptr);

    return true;
}

// VERIFY: e740de70 manual
// OFFSET: 0x004EB3E0
bool CCharacterComponent::SetRandomSkin(COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    int32_t numSkins = CCharacterComponent::GetNumSkins(this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, this->m_data.m_preferences.m_classId, context);
    int32_t index = numSkins > 0 ? CRandom::dice_(numSkins, g_rndSeed) : 0;
    int32_t skinID = ComponentGetSkinColor(this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, index, selection);

    if (skinID < 0) {
        return false;
    }

    this->SetSkinColor(skinID, true, true, nullptr);

    return true;
}

// VERIFY: 9088bbfb manual
// OFFSET: 0x004EB470
bool CCharacterComponent::SetRandomHairStyle(COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    int32_t numStyles = CCharacterComponent::GetNumHairStylesForColor(this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, this->m_data.m_preferences.m_classId, this->m_data.m_preferences.m_hairColorId, context);
    int32_t index = numStyles > 0 ? CRandom::dice_(numStyles, g_rndSeed) : 0;
    int32_t hairStyleID = ComponentGetHairVariation(this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, this->m_data.m_preferences.m_hairColorId, index, selection);

    if (hairStyleID < 0) {
        return false;
    }

    this->SetHairStyle(hairStyleID, nullptr);

    return true;
}

// VERIFY: 2f3a54f4 auto
// OFFSET: 0x004EB500
bool CCharacterComponent::SetNextHairColor(COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR, this->m_data.m_preferences.m_hairStyleId);

    if (numColors <= 0) {
        return false;
    }

    int32_t hairColorID = this->m_data.m_preferences.m_hairColorId + 1;

    if (hairColorID >= numColors) {
        hairColorID = 0;
    }

    if (hairColorID == static_cast<int32_t>(this->m_data.m_preferences.m_hairColorId)) {
        return false;
    }

    CharSectionsRec* rec;

    while (true) {
        rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR, this->m_data.m_preferences.m_hairStyleId, hairColorID, nullptr);

        if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
            break;
        }

        if (++hairColorID >= numColors) {
            hairColorID = 0;
        }

        if (hairColorID == static_cast<int32_t>(this->m_data.m_preferences.m_hairColorId)) {
            return false;
        }
    }

    this->SetHairColor(rec->m_colorIndex, true, nullptr);

    return true;
}

// VERIFY: d3df073c auto
// OFFSET: 0x004EB5C0
bool CCharacterComponent::SetPrevHairColor(COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR, this->m_data.m_preferences.m_hairStyleId);

    if (numColors <= 0) {
        return false;
    }

    int32_t hairColorID = this->m_data.m_preferences.m_hairColorId - 1;

    if (hairColorID < 0) {
        hairColorID = numColors - 1;
    }

    if (hairColorID == static_cast<int32_t>(this->m_data.m_preferences.m_hairColorId)) {
        return false;
    }

    CharSectionsRec* rec;

    while (true) {
        rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR, this->m_data.m_preferences.m_hairStyleId, hairColorID, nullptr);

        if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
            break;
        }

        if (--hairColorID < 0) {
            hairColorID = numColors - 1;
        }

        if (hairColorID == static_cast<int32_t>(this->m_data.m_preferences.m_hairColorId)) {
            return false;
        }
    }

    this->SetHairColor(rec->m_colorIndex, true, nullptr);

    return true;
}

// VERIFY: 9837420d manual
// OFFSET: 0x004EB680
bool CCharacterComponent::SetRandomHairColor(COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    int32_t numColors = CCharacterComponent::GetNumHairColorsForStyle(this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, this->m_data.m_preferences.m_classId, this->m_data.m_preferences.m_hairStyleId, context);
    int32_t index = numColors > 0 ? CRandom::dice_(numColors, g_rndSeed) : 0;
    int32_t hairColorID = ComponentGetHairColor(this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, this->m_data.m_preferences.m_hairStyleId, index, selection);

    if (hairColorID < 0) {
        return false;
    }

    this->SetHairColor(hairColorID, true, nullptr);

    return true;
}

// VERIFY: a45f4034 manual
// OFFSET: 0x004EB710
bool CCharacterComponent::SetNextFace(COMPONENT_CONTEXT context, int32_t colorOffset) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    int32_t numVariations = ComponentGetNumVariations(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACE);

    if (numVariations <= 0) {
        return false;
    }

    int32_t faceID = this->m_data.m_preferences.m_faceId + 1;

    if (faceID >= numVariations) {
        faceID = 0;
    }

    if (faceID == static_cast<int32_t>(this->m_data.m_preferences.m_faceId)) {
        return false;
    }

    int32_t skinID;

    while (true) {
        int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACE, faceID);
        bool matched = false;

        for (int32_t i = 0; i < numColors; i = skinID + 1) {
            skinID = (i + colorOffset) % numColors;
            CharSectionsRec* faceRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACE, faceID, skinID, nullptr);
            CharSectionsRec* skinRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, skinID, nullptr);
            CharSectionsRec* underwearRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, VARIATION_UNDERWEAR, 0, skinID, nullptr);

            if (faceRec && ComponentFlagsMatch(faceRec->m_flags, selection) && skinRec && ComponentFlagsMatch(skinRec->m_flags, selection) && (context == COMPONENT_CONTEXT_2 || (underwearRec && ComponentFlagsMatch(underwearRec->m_flags, selection)))) {
                matched = true;
                break;
            }
        }

        if (matched) {
            break;
        }

        if (++faceID >= numVariations) {
            faceID = 0;
        }

        if (faceID == static_cast<int32_t>(this->m_data.m_preferences.m_faceId)) {
            return false;
        }
    }

    CharSectionsRec* faceRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACE, faceID, this->m_data.m_preferences.m_skinID, nullptr);
    CharSectionsRec* skinRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, this->m_data.m_preferences.m_skinID, nullptr);
    CharSectionsRec* underwearRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, VARIATION_UNDERWEAR, 0, this->m_data.m_preferences.m_skinID, nullptr);

    if (!faceRec || !ComponentFlagsMatch(faceRec->m_flags, selection) || !skinRec || !ComponentFlagsMatch(skinRec->m_flags, selection) || (context != COMPONENT_CONTEXT_2 && (!underwearRec || !ComponentFlagsMatch(underwearRec->m_flags, selection)))) {
        this->SetSkinColor(skinID, false, true, nullptr);
    }

    this->SetFace(faceID, true, nullptr);

    return true;
}

// VERIFY: 14384726 auto
// OFFSET: 0x004EB990
bool CCharacterComponent::SetPrevFace(COMPONENT_CONTEXT context, int32_t colorOffset) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    int32_t numVariations = ComponentGetNumVariations(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACE);

    if (numVariations <= 0) {
        return false;
    }

    int32_t faceID = this->m_data.m_preferences.m_faceId - 1;

    if (faceID < 0) {
        faceID = numVariations - 1;
    }

    if (faceID == static_cast<int32_t>(this->m_data.m_preferences.m_faceId)) {
        return false;
    }

    int32_t skinID;

    while (true) {
        int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACE, faceID);
        bool matched = false;

        for (int32_t i = 0; i < numColors; i = skinID + 1) {
            skinID = (i + colorOffset) % numColors;
            CharSectionsRec* faceRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACE, faceID, skinID, nullptr);
            CharSectionsRec* skinRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, skinID, nullptr);
            CharSectionsRec* underwearRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, VARIATION_UNDERWEAR, 0, skinID, nullptr);

            if (faceRec && ComponentFlagsMatch(faceRec->m_flags, selection) && skinRec && ComponentFlagsMatch(skinRec->m_flags, selection) && (context == COMPONENT_CONTEXT_2 || (underwearRec && ComponentFlagsMatch(underwearRec->m_flags, selection)))) {
                matched = true;
                break;
            }
        }

        if (matched) {
            break;
        }

        if (--faceID < 0) {
            faceID = numVariations - 1;
        }

        if (faceID == static_cast<int32_t>(this->m_data.m_preferences.m_faceId)) {
            return false;
        }
    }

    CharSectionsRec* faceRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACE, faceID, this->m_data.m_preferences.m_skinID, nullptr);
    CharSectionsRec* skinRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, this->m_data.m_preferences.m_skinID, nullptr);
    CharSectionsRec* underwearRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, VARIATION_UNDERWEAR, 0, this->m_data.m_preferences.m_skinID, nullptr);

    if (!faceRec || !ComponentFlagsMatch(faceRec->m_flags, selection) || !skinRec || !ComponentFlagsMatch(skinRec->m_flags, selection) || (context != COMPONENT_CONTEXT_2 && (!underwearRec || !ComponentFlagsMatch(underwearRec->m_flags, selection)))) {
        this->SetSkinColor(skinID, false, true, nullptr);
    }

    this->SetFace(faceID, true, nullptr);

    return true;
}

// VERIFY: 951a025a manual
// OFFSET: 0x004EBC10
bool CCharacterComponent::SetRandomFace(COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    int32_t numFaces = CCharacterComponent::GetNumFacesForSkin(this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, this->m_data.m_preferences.m_classId, this->m_data.m_preferences.m_skinID, context);
    int32_t index = numFaces > 0 ? CRandom::dice_(numFaces, g_rndSeed) : 0;
    int32_t faceID = ComponentGetFaceVariation(this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, this->m_data.m_preferences.m_skinID, index, selection);

    if (faceID < 0) {
        return false;
    }

    this->SetFace(faceID, true, nullptr);

    return true;
}

// VERIFY: d40caea4 manual
// OFFSET: 0x004EBCA0
bool CCharacterComponent::SetNextFacialFeature(COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    bool found;
    ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACIALHAIR, this->m_data.m_preferences.m_facialHairId, this->m_data.m_preferences.m_hairColorId, &found);

    if (!found) {
        int32_t count = CCharacterComponent::s_characterFacialHairStylesList[this->m_data.m_preferences.m_raceID * 2 + this->m_data.m_preferences.m_sexID];
        int32_t facialHairID = this->m_data.m_preferences.m_facialHairId + 1;
        this->SetBeardStyle(facialHairID >= count ? 0 : facialHairID, true, nullptr);

        return true;
    }

    int32_t numVariations = ComponentGetNumVariations(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACIALHAIR);

    if (numVariations <= 0) {
        return false;
    }

    int32_t facialHairID = this->m_data.m_preferences.m_facialHairId + 1;

    if (facialHairID >= numVariations) {
        facialHairID = 0;
    }

    if (facialHairID == static_cast<int32_t>(this->m_data.m_preferences.m_facialHairId)) {
        return false;
    }

    CharSectionsRec* rec = nullptr;

    while (true) {
        int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACIALHAIR, facialHairID);
        int32_t color;

        for (color = 0; color < numColors; color++) {
            rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACIALHAIR, facialHairID, color, nullptr);

            if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
                break;
            }
        }

        if (color < numColors) {
            break;
        }

        if (++facialHairID >= numVariations) {
            facialHairID = 0;
        }

        if (facialHairID == static_cast<int32_t>(this->m_data.m_preferences.m_facialHairId)) {
            return false;
        }
    }

    CharSectionsRec* keepRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACIALHAIR, facialHairID, this->m_data.m_preferences.m_hairColorId, nullptr);

    if (keepRec && ComponentFlagsMatch(keepRec->m_flags, selection)) {
        this->SetBeardStyle(keepRec->m_variationIndex, true, nullptr);
    } else {
        this->SetHairColor(rec->m_colorIndex, false, nullptr);
        this->SetBeardStyle(rec->m_variationIndex, true, nullptr);
    }

    return true;
}

// VERIFY: 5aebe048 manual
// OFFSET: 0x004EBE80
bool CCharacterComponent::SetPrevFacialFeature(COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    bool found;
    ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACIALHAIR, this->m_data.m_preferences.m_facialHairId, this->m_data.m_preferences.m_hairColorId, &found);

    if (!found) {
        uint32_t count = CCharacterComponent::s_characterFacialHairStylesList[this->m_data.m_preferences.m_raceID * 2 + this->m_data.m_preferences.m_sexID];
        int32_t facialHairID = this->m_data.m_preferences.m_facialHairId - 1;
        this->SetBeardStyle(facialHairID >= 0 ? facialHairID : count - 1, true, nullptr);

        return true;
    }

    int32_t numVariations = ComponentGetNumVariations(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACIALHAIR);

    if (numVariations <= 0) {
        return false;
    }

    int32_t facialHairID = this->m_data.m_preferences.m_facialHairId - 1;

    if (facialHairID < 0) {
        facialHairID = numVariations - 1;
    }

    if (facialHairID == static_cast<int32_t>(this->m_data.m_preferences.m_facialHairId)) {
        return false;
    }

    CharSectionsRec* rec = nullptr;

    while (true) {
        int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACIALHAIR, facialHairID);
        int32_t color;

        for (color = 0; color < numColors; color++) {
            rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACIALHAIR, facialHairID, color, nullptr);

            if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
                break;
            }
        }

        if (color < numColors) {
            break;
        }

        if (--facialHairID < 0) {
            facialHairID = numVariations - 1;
        }

        if (facialHairID == static_cast<int32_t>(this->m_data.m_preferences.m_facialHairId)) {
            return false;
        }
    }

    CharSectionsRec* keepRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACIALHAIR, facialHairID, this->m_data.m_preferences.m_hairColorId, nullptr);

    if (ComponentFlagsMatch(keepRec->m_flags, selection)) {
        this->SetBeardStyle(keepRec->m_variationIndex, true, nullptr);
    } else {
        this->SetHairColor(rec->m_colorIndex, false, nullptr);
        this->SetBeardStyle(rec->m_variationIndex, true, nullptr);
    }

    return true;
}

// VERIFY: 0a57ee92 manual
// OFFSET: 0x004EC050
bool CCharacterComponent::SetRandomFacialFeature(COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    int32_t numFeatures = CCharacterComponent::GetNumFacialFeaturesForHairColor(this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, this->m_data.m_preferences.m_classId, this->m_data.m_preferences.m_hairColorId, context);
    int32_t index = numFeatures > 0 ? CRandom::dice_(numFeatures, g_rndSeed) : 0;
    int32_t facialHairID = CCharacterComponent::GetNthFacialFeatureIndex(this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, this->m_data.m_preferences.m_classId, this->m_data.m_preferences.m_hairColorId, this->m_data.m_preferences.m_facialHairId, index, selection);

    if (facialHairID < 0) {
        return false;
    }

    this->SetBeardStyle(facialHairID, true, nullptr);

    return true;
}

// VERIFY: e78a94ea manual
// OFFSET: 0x004EC0E0
void CCharacterComponent::ClearGuildTexture(int32_t count) {
    for (int32_t layer = count; layer > 1; layer--) {
        if (this->m_section[SECTION_TORSO_UPPER].m_layerTex[layer]) {
            TextureCacheDestroyTexture(this->m_section[SECTION_TORSO_UPPER].m_layerTex[layer]);
            this->m_section[SECTION_TORSO_UPPER].m_layerTex[layer] = nullptr;
        }

        this->m_section[SECTION_TORSO_UPPER].m_layerMask &= ~(1 << layer);
        this->m_section[SECTION_TORSO_UPPER].m_layerItemDisplayId[layer] = 0;
        this->SetDirtySections(1 << SECTION_TORSO_UPPER);
    }

    for (int32_t layer = count; layer > 1; layer--) {
        if (this->m_section[SECTION_TORSO_LOWER].m_layerTex[layer]) {
            TextureCacheDestroyTexture(this->m_section[SECTION_TORSO_LOWER].m_layerTex[layer]);
            this->m_section[SECTION_TORSO_LOWER].m_layerTex[layer] = nullptr;
        }

        this->m_section[SECTION_TORSO_LOWER].m_layerMask &= ~(1 << layer);
        this->m_section[SECTION_TORSO_LOWER].m_layerItemDisplayId[layer] = 0;
        this->SetDirtySections(1 << SECTION_TORSO_LOWER);
    }
}

// VERIFY: 32375028 auto
// OFFSET: 0x004EC1C0
void CCharacterComponent::ApplyGuildColor(int32_t emblem, int32_t emblemColor, int32_t border, int32_t borderColor, int32_t background) {
    this->ClearGuildTexture(4);

    CCharacterComponent::GetTabardBackgroundFileName(SECTION_TORSO_UPPER, background, s_buffer, sizeof(s_buffer));
    SStrPack(s_buffer, ".blp", sizeof(s_buffer));
    if (this->m_section[SECTION_TORSO_UPPER].m_layerTex[2]) {
        TextureCacheDestroyTexture(this->m_section[SECTION_TORSO_UPPER].m_layerTex[2]);
    }

    this->m_section[SECTION_TORSO_UPPER].m_layerTex[2] = TextureCacheCreateTexture(s_buffer);
    this->m_section[SECTION_TORSO_UPPER].m_layerItemDisplayId[2] = 0;
    this->m_section[SECTION_TORSO_UPPER].m_layerMask &= ~(1 << 2);

    if (!this->m_section[SECTION_TORSO_UPPER].m_layerTex[2]) {
        DebugLog("**** Unable to Load Texture %s\n", s_buffer);
    }

    CCharacterComponent::GetTabardBackgroundFileName(SECTION_TORSO_LOWER, background, s_buffer, sizeof(s_buffer));
    SStrPack(s_buffer, ".blp", sizeof(s_buffer));
    if (this->m_section[SECTION_TORSO_LOWER].m_layerTex[2]) {
        TextureCacheDestroyTexture(this->m_section[SECTION_TORSO_LOWER].m_layerTex[2]);
    }

    this->m_section[SECTION_TORSO_LOWER].m_layerTex[2] = TextureCacheCreateTexture(s_buffer);
    this->m_section[SECTION_TORSO_LOWER].m_layerItemDisplayId[2] = 0;
    this->m_section[SECTION_TORSO_LOWER].m_layerMask &= ~(1 << 2);

    if (!this->m_section[SECTION_TORSO_LOWER].m_layerTex[2]) {
        DebugLog("**** Unable to Load Texture %s\n", s_buffer);
    }

    CCharacterComponent::GetTabardEmblemFileName(SECTION_TORSO_UPPER, emblem, emblemColor, s_buffer, sizeof(s_buffer));
    SStrPack(s_buffer, ".blp", sizeof(s_buffer));
    if (this->m_section[SECTION_TORSO_UPPER].m_layerTex[4]) {
        TextureCacheDestroyTexture(this->m_section[SECTION_TORSO_UPPER].m_layerTex[4]);
    }

    this->m_section[SECTION_TORSO_UPPER].m_layerTex[4] = TextureCacheCreateTexture(s_buffer);
    this->m_section[SECTION_TORSO_UPPER].m_layerItemDisplayId[4] = 0;
    this->m_section[SECTION_TORSO_UPPER].m_layerMask &= ~(1 << 4);

    if (!this->m_section[SECTION_TORSO_UPPER].m_layerTex[4]) {
        DebugLog("**** Unable to Load Texture %s\n", s_buffer);
    }

    CCharacterComponent::GetTabardEmblemFileName(SECTION_TORSO_LOWER, emblem, emblemColor, s_buffer, sizeof(s_buffer));
    SStrPack(s_buffer, ".blp", sizeof(s_buffer));
    if (this->m_section[SECTION_TORSO_LOWER].m_layerTex[4]) {
        TextureCacheDestroyTexture(this->m_section[SECTION_TORSO_LOWER].m_layerTex[4]);
    }

    this->m_section[SECTION_TORSO_LOWER].m_layerTex[4] = TextureCacheCreateTexture(s_buffer);
    this->m_section[SECTION_TORSO_LOWER].m_layerItemDisplayId[4] = 0;
    this->m_section[SECTION_TORSO_LOWER].m_layerMask &= ~(1 << 4);

    if (!this->m_section[SECTION_TORSO_LOWER].m_layerTex[4]) {
        DebugLog("**** Unable to Load Texture %s\n", s_buffer);
    }

    CCharacterComponent::GetTabardBorderFileName(SECTION_TORSO_UPPER, border, borderColor, s_buffer, sizeof(s_buffer));
    SStrPack(s_buffer, ".blp", sizeof(s_buffer));
    if (this->m_section[SECTION_TORSO_UPPER].m_layerTex[3]) {
        TextureCacheDestroyTexture(this->m_section[SECTION_TORSO_UPPER].m_layerTex[3]);
    }

    this->m_section[SECTION_TORSO_UPPER].m_layerTex[3] = TextureCacheCreateTexture(s_buffer);
    this->m_section[SECTION_TORSO_UPPER].m_layerItemDisplayId[3] = 0;
    this->m_section[SECTION_TORSO_UPPER].m_layerMask &= ~(1 << 3);

    if (!this->m_section[SECTION_TORSO_UPPER].m_layerTex[3]) {
        DebugLog("**** Unable to Load Texture %s\n", s_buffer);
    }

    CCharacterComponent::GetTabardBorderFileName(SECTION_TORSO_LOWER, border, borderColor, s_buffer, sizeof(s_buffer));
    SStrPack(s_buffer, ".blp", sizeof(s_buffer));
    if (this->m_section[SECTION_TORSO_LOWER].m_layerTex[3]) {
        TextureCacheDestroyTexture(this->m_section[SECTION_TORSO_LOWER].m_layerTex[3]);
    }

    this->m_section[SECTION_TORSO_LOWER].m_layerTex[3] = TextureCacheCreateTexture(s_buffer);
    this->m_section[SECTION_TORSO_LOWER].m_layerItemDisplayId[3] = 0;
    this->m_section[SECTION_TORSO_LOWER].m_layerMask &= ~(1 << 3);

    if (!this->m_section[SECTION_TORSO_LOWER].m_layerTex[3]) {
        DebugLog("**** Unable to Load Texture %s\n", s_buffer);
    }

    this->SetDirtySections((1 << SECTION_TORSO_UPPER) | (1 << SECTION_TORSO_LOWER));
}

// VERIFY: 5954ffe3 auto
// OFFSET: 0x004EC510
void CCharacterComponent::TurnOffGuildTabardPurchase() {
    if (!(this->m_flags & 0x40)) {
        return;
    }

    this->m_flags = (this->m_flags & ~0x44) | 0x4;
    this->ClearGuildTexture(4);
    this->SetDirtySections((1 << SECTION_TORSO_UPPER) | (1 << SECTION_TORSO_LOWER));
}

// VERIFY: 5f4d62d3 manual
// OFFSET: 0x004EC550
void CCharacterComponent::Paste(HTEXTURECACHE texture, MipBits* dstMips, const C2iVector& dstPos, const C2iVector& srcPos, const C2iVector& srcSize, TCTEXTUREINFO& srcInfo, uint32_t srcMipLevel) {
    C2iVector pos = dstPos;
    C2iVector src = srcPos;
    C2iVector size = srcSize;
    uint32_t stride = 4 * s_textureSize;
    BlpPalPixel* pal = TextureCacheGetPal(texture);

    if (!pal) {
        CCharacterComponent::PasteCrappyGreen(dstMips, pos, stride, size, srcInfo, srcMipLevel, -static_cast<int32_t>(srcMipLevel));
        return;
    }

    switch (srcInfo.m_alphaSize) {
        case 0:
            CCharacterComponent::PasteOpaque(texture, pal, dstMips, pos, stride, src, size, srcInfo, srcMipLevel, -static_cast<int32_t>(srcMipLevel));
            break;

        case 1:
            CCharacterComponent::PasteTransparent1Bit(texture, pal, dstMips, pos, stride, src, size, srcInfo, srcMipLevel, -static_cast<int32_t>(srcMipLevel));
            break;

        case 4:
            CCharacterComponent::PasteTransparent4Bit(texture, pal, dstMips, pos, stride, src, size, srcInfo, srcMipLevel, -static_cast<int32_t>(srcMipLevel));
            break;

        case 8:
            CCharacterComponent::PasteTransparent8Bit(texture, pal, dstMips, pos, stride, src, size, srcInfo, srcMipLevel, -static_cast<int32_t>(srcMipLevel));
            break;
    }
}

// VERIFY: 4a30e6aa manual
// OFFSET: 0x004EC690
void CCharacterComponent::PasteTransparent1BitScale(HTEXTURECACHE texture, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo) {
    uint32_t srcRowPitch = srcInfo.m_width;
    uint32_t copyWidth = srcSize.x;
    uint32_t copyHeight = srcSize.y;

    BlpPalPixel* pal = TextureCacheGetPal(texture);

    if (!pal) {
        return;
    }

    srcPos.x >>= 1;
    srcPos.y >>= 1;

    const uint8_t* srcBase = static_cast<const uint8_t*>(TextureCacheGetMip(texture, 0));
    const uint8_t* alphaBase = srcBase + srcRowPitch * srcInfo.m_height;

    uint8_t* dstRow = reinterpret_cast<uint8_t*>(dstMips->m_mip[0]) + dstStride * dstPos.y + 4 * dstPos.x;

    for (uint32_t y = 0; y < copyHeight; y++) {
        uint32_t y0 = y >> 1;
        uint32_t y1 = y0 + 1;

        if (y1 >= (copyHeight >> 1) - 1) {
            y1 = (copyHeight >> 1) - 1;
        }

        uint32_t rowOff0 = srcRowPitch * y0;
        uint32_t rowOff1 = srcRowPitch * y1;

        C4Pixel* dst = reinterpret_cast<C4Pixel*>(dstRow);

        if (y & 1) {
            const uint8_t* row0 = srcBase + rowOff0;
            const uint8_t* row1 = srcBase + rowOff1;

            for (uint32_t x = 0; x < copyWidth; x++) {
                uint32_t x0 = x >> 1;
                uint32_t x1 = x0 + 1;

                if (x1 >= (copyWidth >> 1) - 1) {
                    x1 = (copyWidth >> 1) - 1;
                }

                const BlpPalPixel& p00 = pal[row0[x0]];
                const BlpPalPixel& p10 = pal[row1[x0]];

                uint32_t a00 = (alphaBase[(rowOff0 + x0) >> 3] & (1 << (x0 & 7))) ? 255 : 0;
                uint32_t a10 = (alphaBase[(rowOff1 + x0) >> 3] & (1 << (x0 & 7))) ? 255 : 0;

                uint32_t a;
                uint32_t srcB;
                uint32_t srcG;
                uint32_t srcR;

                if (x & 1) {
                    const BlpPalPixel& p01 = pal[row0[x1]];
                    const BlpPalPixel& p11 = pal[row1[x1]];

                    uint32_t a01 = (alphaBase[(rowOff0 + x1) >> 3] & (1 << (x1 & 7))) ? 255 : 0;
                    uint32_t a11 = (alphaBase[(rowOff1 + x1) >> 3] & (1 << (x1 & 7))) ? 255 : 0;

                    a = (a00 + a10 + a01 + a11) >> 2;
                    srcB = (p00.m_b + p01.m_b + p10.m_b + p11.m_b) >> 2;
                    srcG = (p00.m_g + p01.m_g + p10.m_g + p11.m_g) >> 2;
                    srcR = (p00.m_r + p01.m_r + p10.m_r + p11.m_r) >> 2;
                } else {
                    a = (a00 + a10) >> 1;
                    srcB = (p00.m_b + p10.m_b) >> 1;
                    srcG = (p00.m_g + p10.m_g) >> 1;
                    srcR = (p00.m_r + p10.m_r) >> 1;
                }

                dst[x].m_b = ((255 - a) * dst[x].m_b + a * srcB) >> 8;
                dst[x].m_g = ((255 - a) * dst[x].m_g + a * srcG) >> 8;
                dst[x].m_r = ((255 - a) * dst[x].m_r + a * srcR) >> 8;
                dst[x].m_a = 0xFF;
            }
        } else {
            const uint8_t* row0 = srcBase + rowOff0;

            for (uint32_t x = 0; x < copyWidth; x++) {
                uint32_t x0 = x >> 1;
                uint32_t x1 = x0 + 1;

                if (x1 >= (copyWidth >> 1) - 1) {
                    x1 = (copyWidth >> 1) - 1;
                }

                const BlpPalPixel& p00 = pal[row0[x0]];

                uint32_t a00 = (alphaBase[(rowOff0 + x0) >> 3] & (1 << (x0 & 7))) ? 255 : 0;

                uint32_t a;
                uint32_t srcB;
                uint32_t srcG;
                uint32_t srcR;

                if (x & 1) {
                    const BlpPalPixel& p01 = pal[row0[x1]];

                    uint32_t a01 = (alphaBase[(rowOff0 + x1) >> 3] & (1 << (x1 & 7))) ? 255 : 0;

                    a = (a00 + a01) >> 1;
                    srcB = (p00.m_b + p01.m_b) >> 1;
                    srcG = (p00.m_g + p01.m_g) >> 1;
                    srcR = (p00.m_r + p01.m_r) >> 1;
                } else {
                    a = a00;
                    srcB = p00.m_b;
                    srcG = p00.m_g;
                    srcR = p00.m_r;
                }

                dst[x].m_b = ((255 - a) * dst[x].m_b + a * srcB) >> 8;
                dst[x].m_g = ((255 - a) * dst[x].m_g + a * srcG) >> 8;
                dst[x].m_r = ((255 - a) * dst[x].m_r + a * srcR) >> 8;
                dst[x].m_a = 0xFF;
            }
        }

        dstRow += dstStride;
    }

    srcSize.x = (srcSize.x >> 1) < 1 ? 1 : (srcSize.x >> 1);
    srcSize.y = (srcSize.y >> 1) < 1 ? 1 : (srcSize.y >> 1);

    dstPos.x >>= 1;
    dstPos.y >>= 1;

    CCharacterComponent::PasteTransparent1Bit(texture, pal, dstMips, dstPos, dstStride >> 1, srcPos, srcSize, srcInfo, 0, 1);
}

// VERIFY: 3cc4e3c5 manual
// OFFSET: 0x004ECC20
void CCharacterComponent::PasteTransparent4BitScale(HTEXTURECACHE texture, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo) {
    uint32_t srcRowPitch = srcInfo.m_width;
    uint32_t copyWidth = srcSize.x;
    uint32_t copyHeight = srcSize.y;

    BlpPalPixel* pal = TextureCacheGetPal(texture);

    if (!pal) {
        return;
    }

    srcPos.x >>= 1;
    srcPos.y >>= 1;

    const uint8_t* srcBase = static_cast<const uint8_t*>(TextureCacheGetMip(texture, 0));
    const uint8_t* alphaBase = srcBase + srcRowPitch * srcInfo.m_height;

    uint8_t* dstRow = reinterpret_cast<uint8_t*>(dstMips->m_mip[0]) + dstStride * dstPos.y + 4 * dstPos.x;

    for (uint32_t y = 0; y < copyHeight; y++) {
        uint32_t y0 = y >> 1;
        uint32_t y1 = y0 + 1;

        if (y1 >= (copyHeight >> 1) - 1) {
            y1 = (copyHeight >> 1) - 1;
        }

        uint32_t rowOff0 = srcRowPitch * y0;
        uint32_t rowOff1 = srcRowPitch * y1;

        C4Pixel* dst = reinterpret_cast<C4Pixel*>(dstRow);

        if (y & 1) {
            const uint8_t* row0 = srcBase + rowOff0;
            const uint8_t* row1 = srcBase + rowOff1;

            for (uint32_t x = 0; x < copyWidth; x++) {
                uint32_t x0 = x >> 1;
                uint32_t x1 = x0 + 1;

                if (x1 >= (copyWidth >> 1) - 1) {
                    x1 = (copyWidth >> 1) - 1;
                }

                uint32_t shift0 = 4 * (x0 & 1);

                const BlpPalPixel& p00 = pal[row0[x0]];
                const BlpPalPixel& p10 = pal[row1[x0]];

                uint32_t n00 = (alphaBase[(rowOff0 + x0) >> 1] & (15 << shift0)) >> shift0;
                uint32_t n10 = (alphaBase[(rowOff1 + x0) >> 1] & (15 << shift0)) >> shift0;

                uint32_t a00 = (n00 << 4) | n00;
                uint32_t a10 = (n10 << 4) | n10;

                uint32_t a;
                uint32_t srcB;
                uint32_t srcG;
                uint32_t srcR;

                if (x & 1) {
                    uint32_t shift1 = 4 * (x1 & 1);

                    const BlpPalPixel& p01 = pal[row0[x1]];
                    const BlpPalPixel& p11 = pal[row1[x1]];

                    uint32_t n01 = (alphaBase[(rowOff0 + x1) >> 1] & (15 << shift1)) >> shift1;
                    uint32_t n11 = (alphaBase[(rowOff1 + x1) >> 1] & (15 << shift1)) >> shift1;

                    uint32_t a01 = (n01 << 4) | n01;
                    uint32_t a11 = (n11 << 4) | n11;

                    a = (a00 + a01 + a10 + a11) >> 2;
                    srcB = (p00.m_b + p01.m_b + p10.m_b + p11.m_b) >> 2;
                    srcG = (p00.m_g + p01.m_g + p10.m_g + p11.m_g) >> 2;
                    srcR = (p00.m_r + p01.m_r + p10.m_r + p11.m_r) >> 2;
                } else {
                    a = (a00 + a10) >> 1;
                    srcB = (p00.m_b + p10.m_b) >> 1;
                    srcG = (p00.m_g + p10.m_g) >> 1;
                    srcR = (p00.m_r + p10.m_r) >> 1;
                }

                dst[x].m_b = ((255 - a) * dst[x].m_b + a * srcB) >> 8;
                dst[x].m_g = ((255 - a) * dst[x].m_g + a * srcG) >> 8;
                dst[x].m_r = ((255 - a) * dst[x].m_r + a * srcR) >> 8;
                dst[x].m_a = 0xFF;
            }
        } else {
            const uint8_t* row0 = srcBase + rowOff0;

            for (uint32_t x = 0; x < copyWidth; x++) {
                uint32_t x0 = x >> 1;
                uint32_t x1 = x0 + 1;

                if (x1 >= (copyWidth >> 1) - 1) {
                    x1 = (copyWidth >> 1) - 1;
                }

                uint32_t shift0 = 4 * (x0 & 1);

                const BlpPalPixel& p00 = pal[row0[x0]];

                uint32_t n00 = (alphaBase[(rowOff0 + x0) >> 1] & (15 << shift0)) >> shift0;
                uint32_t a00 = (n00 << 4) | n00;

                uint32_t a;
                uint32_t srcB;
                uint32_t srcG;
                uint32_t srcR;

                if (x & 1) {
                    uint32_t shift1 = 4 * (x1 & 1);

                    const BlpPalPixel& p01 = pal[row0[x1]];

                    uint32_t n01 = (alphaBase[(rowOff0 + x1) >> 1] & (15 << shift1)) >> shift1;
                    uint32_t a01 = (n01 << 4) | n01;

                    a = (a00 + a01) >> 1;
                    srcB = (p00.m_b + p01.m_b) >> 1;
                    srcG = (p00.m_g + p01.m_g) >> 1;
                    srcR = (p00.m_r + p01.m_r) >> 1;
                } else {
                    a = a00;
                    srcB = p00.m_b;
                    srcG = p00.m_g;
                    srcR = p00.m_r;
                }

                dst[x].m_b = ((255 - a) * dst[x].m_b + a * srcB) >> 8;
                dst[x].m_g = ((255 - a) * dst[x].m_g + a * srcG) >> 8;
                dst[x].m_r = ((255 - a) * dst[x].m_r + a * srcR) >> 8;
                dst[x].m_a = 0xFF;
            }
        }

        dstRow += dstStride;
    }

    srcSize.x = (srcSize.x >> 1) < 1 ? 1 : (srcSize.x >> 1);
    srcSize.y = (srcSize.y >> 1) < 1 ? 1 : (srcSize.y >> 1);

    dstPos.x >>= 1;
    dstPos.y >>= 1;

    CCharacterComponent::PasteTransparent4Bit(texture, pal, dstMips, dstPos, dstStride >> 1, srcPos, srcSize, srcInfo, 0, 1);
}

// VERIFY: 523c6ae3 manual
// OFFSET: 0x004ED200
void CCharacterComponent::PasteTransparent8BitScale(HTEXTURECACHE texture, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo) {
    uint32_t srcRowPitch = srcInfo.m_width;
    uint32_t copyWidth = srcSize.x;
    uint32_t copyHeight = srcSize.y;

    BlpPalPixel* pal = TextureCacheGetPal(texture);

    if (!pal) {
        return;
    }

    srcPos.x >>= 1;
    srcPos.y >>= 1;

    const uint8_t* srcBase = static_cast<const uint8_t*>(TextureCacheGetMip(texture, 0));
    const uint8_t* alphaBase = srcBase + srcRowPitch * srcInfo.m_height;

    uint8_t* dstRow = reinterpret_cast<uint8_t*>(dstMips->m_mip[0]) + dstStride * dstPos.y + 4 * dstPos.x;

    for (uint32_t y = 0; y < copyHeight; y++) {
        uint32_t y0 = y >> 1;
        uint32_t y1 = y0 + 1;

        if (y1 >= (copyHeight >> 1) - 1) {
            y1 = (copyHeight >> 1) - 1;
        }

        uint32_t rowOff0 = srcRowPitch * y0;
        uint32_t rowOff1 = srcRowPitch * y1;

        C4Pixel* dst = reinterpret_cast<C4Pixel*>(dstRow);

        if (y & 1) {
            const uint8_t* row0 = srcBase + rowOff0;
            const uint8_t* row1 = srcBase + rowOff1;

            const uint8_t* alphaRow0 = alphaBase + rowOff0;
            const uint8_t* alphaRow1 = alphaBase + rowOff1;

            for (uint32_t x = 0; x < copyWidth; x++) {
                uint32_t x0 = x >> 1;
                uint32_t x1 = x0 + 1;

                if (x1 >= (copyWidth >> 1) - 1) {
                    x1 = (copyWidth >> 1) - 1;
                }

                const BlpPalPixel& p00 = pal[row0[x0]];
                const BlpPalPixel& p10 = pal[row1[x0]];

                uint32_t a;
                uint32_t srcB;
                uint32_t srcG;
                uint32_t srcR;

                if (x & 1) {
                    const BlpPalPixel& p01 = pal[row0[x1]];
                    const BlpPalPixel& p11 = pal[row1[x1]];

                    a = (alphaRow1[x0] + alphaRow1[x1] + alphaRow0[x0] + alphaRow0[x1]) >> 2;
                    srcB = (p00.m_b + p01.m_b + p10.m_b + p11.m_b) >> 2;
                    srcG = (p00.m_g + p01.m_g + p10.m_g + p11.m_g) >> 2;
                    srcR = (p00.m_r + p01.m_r + p10.m_r + p11.m_r) >> 2;
                } else {
                    a = (alphaRow1[x0] + alphaRow0[x0]) >> 1;
                    srcB = (p00.m_b + p10.m_b) >> 1;
                    srcG = (p00.m_g + p10.m_g) >> 1;
                    srcR = (p00.m_r + p10.m_r) >> 1;
                }

                dst[x].m_b = ((255 - a) * dst[x].m_b + a * srcB) >> 8;
                dst[x].m_g = ((255 - a) * dst[x].m_g + a * srcG) >> 8;
                dst[x].m_r = ((255 - a) * dst[x].m_r + a * srcR) >> 8;
                dst[x].m_a = 0xFF;
            }
        } else {
            const uint8_t* row0 = srcBase + rowOff0;

            const uint8_t* alphaRow0 = alphaBase + rowOff0;

            for (uint32_t x = 0; x < copyWidth; x++) {
                uint32_t x0 = x >> 1;
                uint32_t x1 = x0 + 1;

                if (x1 >= (copyWidth >> 1) - 1) {
                    x1 = (copyWidth >> 1) - 1;
                }

                const BlpPalPixel& p00 = pal[row0[x0]];

                uint32_t a;
                uint32_t srcB;
                uint32_t srcG;
                uint32_t srcR;

                if (x & 1) {
                    const BlpPalPixel& p01 = pal[row0[x1]];

                    a = (alphaRow0[x0] + alphaRow0[x1]) >> 1;
                    srcB = (p00.m_b + p01.m_b) >> 1;
                    srcG = (p00.m_g + p01.m_g) >> 1;
                    srcR = (p00.m_r + p01.m_r) >> 1;
                } else {
                    a = alphaRow0[x0];
                    srcB = p00.m_b;
                    srcG = p00.m_g;
                    srcR = p00.m_r;
                }

                dst[x].m_b = ((255 - a) * dst[x].m_b + a * srcB) >> 8;
                dst[x].m_g = ((255 - a) * dst[x].m_g + a * srcG) >> 8;
                dst[x].m_r = ((255 - a) * dst[x].m_r + a * srcR) >> 8;
                dst[x].m_a = 0xFF;
            }
        }

        dstRow += dstStride;
    }

    srcSize.x = (srcSize.x >> 1) < 1 ? 1 : (srcSize.x >> 1);
    srcSize.y = (srcSize.y >> 1) < 1 ? 1 : (srcSize.y >> 1);

    dstPos.x >>= 1;
    dstPos.y >>= 1;

    CCharacterComponent::PasteTransparent8Bit(texture, pal, dstMips, dstPos, dstStride >> 1, srcPos, srcSize, srcInfo, 0, 1);
}

// VERIFY: 48812964 auto
// OFFSET: 0x004ED640
bool CCharacterComponent::VariationsLoaded(int32_t wait) {
    TCTEXTUREINFO info;
    info.m_width = 0;
    info.m_height = 0;
    info.m_mipCount = 0;
    info.m_alphaSize = 0;
    info.m_opaque = 1;

    for (int32_t variation = 0; variation < CHARTEXTUREVARIATIONS_NUM; variation++) {
        for (int32_t i = 0; i < 3; i++) {
            if (this->m_baseVariation[variation].m_texture[i] && !TextureCacheGetInfo(this->m_baseVariation[variation].m_texture[i], info, wait)) {
                return false;
            }
        }
    }

    this->m_flags &= ~0x2;

    return true;
}

// VERIFY: 904485bb manual
// OFFSET: 0x004ED6D0
bool CCharacterComponent::ItemsLoaded(int32_t wait) {
    bool loaded = true;
    TCTEXTUREINFO info;
    info.m_width = 0;
    info.m_height = 0;
    info.m_mipCount = 0;
    info.m_alphaSize = 0;
    info.m_opaque = 1;

    for (int32_t section = 0; section < NUM_COMPONENT_SECTIONS; section++) {
        if (!((1 << section) & this->m_dirtySections)) {
            continue;
        }

        CharacterSection& sectionData = this->m_section[section];

        for (int32_t layer = 0; layer < 7; layer++) {
            if (sectionData.m_layerItemDisplayId[layer]) {
                if (!sectionData.m_layerTex[layer]) {
                    ItemDisplayInfoRec rec;

                    if (g_itemDisplayInfoDB.GetRecord(sectionData.m_layerItemDisplayId[layer], &rec)) {
                        sectionData.m_layerTex[layer] = this->LoadItemComponentTexture(&rec, static_cast<COMPONENT_SECTIONS>(section));
                    }
                }
            } else if (!sectionData.m_layerTex[layer]) {
                continue;
            }

            if (wait) {
                if (!sectionData.m_layerTex[layer]) {
                    continue;
                }

                TextureCacheGetInfo(sectionData.m_layerTex[layer], info, 1);
            } else if (!TextureCacheGetInfo(sectionData.m_layerTex[layer], info, 0)) {
                loaded = false;
                continue;
            }

            if (TextureCacheHasMips(sectionData.m_layerTex[layer])) {
                sectionData.m_layerMask |= 1 << layer;
            } else {
                sectionData.m_layerMask &= ~(1 << layer);
            }
        }
    }

    return loaded;
}

// VERIFY: e7ea4ca9 auto
// OFFSET: 0x004ED8C0
static bool __stdcall ItemDisplayHasGeosetGroup(uint32_t displayID, int32_t group, ItemDisplayInfoRec& rec) {
    if (!displayID) {
        return false;
    }

    return g_itemDisplayInfoDB.GetRecord(displayID, &rec) && rec.m_geosetGroup[group];
}

// VERIFY: 197cf0f7 manual
// OFFSET: 0x004ED900
void CCharacterComponent::GeosRenderPrep() {
    bool eyeGlow = false;

    if (this->m_data.m_preferences.m_classId == 6) {
        eyeGlow = true;
    } else {
        CharSectionsRec* faceRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_FACE, this->m_data.m_preferences.m_faceId, this->m_data.m_preferences.m_skinID, nullptr);

        if (faceRec && (faceRec->m_flags & 0x4)) {
            eyeGlow = true;
        }
    }

    CM2Model* model = this->m_data.m_model;
    model->SetGeometryVisible(0, 2000, 0);
    model->SetGeometryVisible(0, 0, 1);

    for (int32_t i = 0; i < NUM_CHARGEOSETS; i++) {
        if (i == CHARGEOSET_EYEEFFECTS && eyeGlow) {
            this->m_data.m_model->SetGeometryVisible(1703, 1703, 1);
        } else {
            this->m_data.m_model->SetGeometryVisible(this->m_data.m_geosets[i], this->m_data.m_geosets[i], 1);
        }
    }

    ItemDisplayInfoRec rec;

    if (this->m_itemDisplayID[ITEMSLOT_GLOVES] && g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_GLOVES], &rec) && rec.m_geosetGroup[0]) {
        this->m_data.m_model->SetGeometryVisible(401, 499, 0);
        this->m_data.m_model->SetGeometryVisible(rec.m_geosetGroup[0] + 401, rec.m_geosetGroup[0] + 401, 1);
    } else if (this->m_itemDisplayID[ITEMSLOT_CHEST] && g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_CHEST], &rec) && rec.m_geosetGroup[0]) {
        this->m_data.m_model->SetGeometryVisible(rec.m_geosetGroup[0] + 801, rec.m_geosetGroup[0] + 801, 1);
    }

    int32_t layer;

    for (layer = 1; layer < 7; layer++) {
        if ((1 << layer) & this->m_section[SECTION_ARM_LOWER].m_layerMask) {
            break;
        }
    }

    if (layer >= 7 && this->m_itemDisplayID[ITEMSLOT_SHIRT] && g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_SHIRT], &rec) && rec.m_geosetGroup[0]) {
        this->m_data.m_model->SetGeometryVisible(rec.m_geosetGroup[0] + 801, rec.m_geosetGroup[0] + 801, 1);
    }

    bool robe = false;
    bool dress = false;
    bool tabard = false;

    if (this->m_itemDisplayID[ITEMSLOT_CHEST] && g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_CHEST], &rec) && rec.m_geosetGroup[2]) {
        this->m_data.m_model->SetGeometryVisible(501, 599, 0);
        this->m_data.m_model->SetGeometryVisible(902, 999, 0);
        this->m_data.m_model->SetGeometryVisible(1100, 1199, 0);
        this->m_data.m_model->SetGeometryVisible(1300, 1399, 0);
        this->m_data.m_model->SetGeometryVisible(rec.m_geosetGroup[2] + 1301, rec.m_geosetGroup[2] + 1301, 1);
        robe = true;
    } else if (this->m_itemDisplayID[ITEMSLOT_PANTS] && g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_PANTS], &rec) && rec.m_geosetGroup[2] && !(this->m_flags & 0x20)) {
        this->m_data.m_model->SetGeometryVisible(501, 599, 0);
        this->m_data.m_model->SetGeometryVisible(902, 999, 0);
        this->m_data.m_model->SetGeometryVisible(1100, 1199, 0);
        this->m_data.m_model->SetGeometryVisible(1300, 1399, 0);
        this->m_data.m_model->SetGeometryVisible(rec.m_geosetGroup[2] + 1301, rec.m_geosetGroup[2] + 1301, 1);
        dress = true;
    } else {
        if (this->m_itemDisplayID[ITEMSLOT_BOOTS] && g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_BOOTS], &rec) && rec.m_geosetGroup[0]) {
            this->m_data.m_model->SetGeometryVisible(501, 599, 0);
            this->m_data.m_model->SetGeometryVisible(901, 901, 1);
            this->m_data.m_model->SetGeometryVisible(rec.m_geosetGroup[0] + 501, rec.m_geosetGroup[0] + 501, 1);
        } else if (this->m_itemDisplayID[ITEMSLOT_PANTS] && g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_PANTS], &rec) && rec.m_geosetGroup[1] && !(this->m_flags & 0x20)) {
            this->m_data.m_model->SetGeometryVisible(rec.m_geosetGroup[1] + 901, rec.m_geosetGroup[1] + 901, 1);
        } else {
            this->m_data.m_model->SetGeometryVisible(901, 901, 1);
        }

        if (this->m_itemDisplayID[ITEMSLOT_TABARD] && g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_TABARD], &rec) && rec.m_geosetGroup[0]) {
            this->m_data.m_model->SetGeometryVisible(rec.m_geosetGroup[0] + 1201, rec.m_geosetGroup[0] + 1201, 1);
            tabard = true;
        }
    }

    if (this->m_flags & 0x40) {
        this->m_data.m_model->SetGeometryVisible(1201, 1201, 1);

        if (!robe && !dress) {
            this->m_data.m_model->SetGeometryVisible(1202, 1202, 1);
        }
    }

    if (!robe) {
        if (!tabard && this->m_itemDisplayID[ITEMSLOT_SHIRT] && g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_SHIRT], &rec) && rec.m_geosetGroup[1]) {
            this->m_data.m_model->SetGeometryVisible(rec.m_geosetGroup[1] + 1001, rec.m_geosetGroup[1] + 1001, 1);
        }

        if (!(this->m_flags & 0x20) && this->m_itemDisplayID[ITEMSLOT_PANTS] && g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_PANTS], &rec) && rec.m_geosetGroup[0]) {
            if (rec.m_geosetGroup[0] > 2) {
                this->m_data.m_model->SetGeometryVisible(1300, 1399, 0);
                this->m_data.m_model->SetGeometryVisible(rec.m_geosetGroup[0] + 1101, rec.m_geosetGroup[0] + 1101, 1);
            } else if (!tabard) {
                this->m_data.m_model->SetGeometryVisible(rec.m_geosetGroup[0] + 1101, rec.m_geosetGroup[0] + 1101, 1);
            }
        }
    }

    if (this->m_itemDisplayID[ITEMSLOT_CAPE] && g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_CAPE], &rec) && rec.m_geosetGroup[0]) {
        this->m_data.m_model->SetGeometryVisible(1500, 1599, 0);
        this->m_data.m_model->SetGeometryVisible(rec.m_geosetGroup[0] + 1501, rec.m_geosetGroup[0] + 1501, 1);
    }

    if (this->m_itemDisplayID[ITEMSLOT_BELT] && g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_BELT], &rec) && rec.m_geosetGroup[0]) {
        this->m_data.m_model->SetGeometryVisible(1800, 1899, 0);
        this->m_data.m_model->SetGeometryVisible(rec.m_geosetGroup[0] + 1801, rec.m_geosetGroup[0] + 1801, 1);
    }

    this->m_data.m_model->OptimizeVisibleGeometry();
    this->m_flags &= ~0x4;
}

// VERIFY: f3bbc113 manual
// OFFSET: 0x004EE0D0
void CCharacterComponent::PrepSections() {
    for (int32_t section = 0; section < NUM_COMPONENT_SECTIONS; section++) {
        if (!((1 << section) & this->m_dirtySections)) {
            continue;
        }

        (this->*CCharacterComponent::m_prepFunc[section])();

        if (!(this->m_flags & 0x1) && this->m_gxTexFormat != GxTex_Dxt1) {
            const CompSectionInfo& info = s_sectionInfo[section];
            GxTexUpdate(TextureGetGxTex(this->m_baseSkinTexture, 1, nullptr), info.m_pos.x, info.m_pos.y, info.m_pos.x + info.m_size.x, info.m_pos.y + info.m_size.y, 1);
        }
    }

    if (!(this->m_flags & 0x1) && this->m_gxTexFormat != GxTex_Dxt1) {
        return;
    }

    if (this->m_gxTexFormat == GxTex_Dxt1) {
        ComputeMipLevels(CCharacterComponent::s_textureBuffer, s_compressedTextureBuffer);
    }

    if (this->m_flags & 0x1) {
        GxTexUpdate(TextureGetGxTex(this->m_baseSkinTexture, 1, nullptr), 0, 0, s_textureSize, s_textureSize, 1);
        return;
    }

    for (int32_t section = 0; section < NUM_COMPONENT_SECTIONS; section++) {
        if ((1 << section) & this->m_dirtySections) {
            const CompSectionInfo& info = s_sectionInfo[section];
            GxTexUpdate(TextureGetGxTex(this->m_baseSkinTexture, 1, nullptr), info.m_pos.x, info.m_pos.y, info.m_pos.x + info.m_size.x, info.m_pos.y + info.m_size.y, 1);
        }
    }
}

// VERIFY: bd1b190b manual
// OFFSET: 0x004EE200
CComponentMipBits* CCharacterComponent::AllocComponentMipBits(EGxTexFormat format) {
    s_mipBitsCritSect.Enter();

    CComponentMipBits* mipBits = format == GxTex_Dxt1 ? s_mipBitsCompressedFreeList.Head() : s_mipBitsFreeList.Head();

    if (mipBits) {
        mipBits->m_link.Unlink();
    }

    s_mipBitsCritSect.Leave();

    return mipBits;
}

// VERIFY: 7bf610ab manual
// OFFSET: 0x004EE2A0
void CCharacterComponent::RenderPrepAll() {
    this->SetTexturesDirty();
    this->m_dirtySections = 0xFFFFFFFF;

    if (this->m_request) {
        this->m_request->m_flags &= ~0x1;
        this->m_request = nullptr;
    }

    this->m_flags &= ~0x8;
    this->VariationsLoaded(1);
    this->ItemsLoaded(1);

    for (int32_t section = 0; section < NUM_COMPONENT_SECTIONS; section++) {
        (this->*CCharacterComponent::m_prepFunc[section])();
    }

    if (this->m_gxTexFormat == GxTex_Dxt1) {
        ComputeMipLevels(CCharacterComponent::s_textureBuffer, s_compressedTextureBuffer);
    }

    this->m_flags &= ~0x1;
    this->m_dirtySections = 0;
    this->DestroyRenderTextures();
    this->m_link.Unlink();
}

// VERIFY: 9391fea1 manual
// OFFSET: 0x004EE380
int32_t CCharacterComponent::UpdateTextureSlot(COMPONENT_SECTIONS section, const ItemDisplayInfoRec* displayRec, int32_t layer) {
    if (this->m_data.m_flags & 0x1) {
        return 0;
    }

    CharacterSection& sectionData = this->m_section[section];

    if (sectionData.m_layerItemDisplayId[layer]) {
        ItemDisplayInfoRec rec;

        if (g_itemDisplayInfoDB.GetRecord(sectionData.m_layerItemDisplayId[layer], &rec) && rec.m_texture[section] == displayRec->m_texture[section]) {
            return 0;
        }
    }

    sectionData.m_layerMask &= ~(1 << layer);

    if (sectionData.m_layerTex[layer]) {
        TextureCacheDestroyTexture(sectionData.m_layerTex[layer]);
        sectionData.m_layerTex[layer] = nullptr;
    }

    sectionData.m_layerItemDisplayId[layer] = displayRec->m_id;

    return 1;
}

// INLINED
inline ITEM_SLOT CCharacterComponent::GetItemSlotForInvSlot(INVENTORY_SLOTS invSlot) {
    switch (invSlot) {
        case INVSLOT_HEAD:
            return ITEMSLOT_HEAD;

        case INVSLOT_SHOULDER:
            return ITEMSLOT_SHOULDER;

        case INVSLOT_BODY:
            return ITEMSLOT_SHIRT;

        case INVSLOT_CHEST:
            return ITEMSLOT_CHEST;

        case INVSLOT_WAIST:
            return ITEMSLOT_BELT;

        case INVSLOT_LEGS:
            return ITEMSLOT_PANTS;

        case INVSLOT_FEET:
            return ITEMSLOT_BOOTS;

        case INVSLOT_WRIST:
            return ITEMSLOT_BRACERS;

        case INVSLOT_HAND:
            return ITEMSLOT_GLOVES;

        case INVSLOT_BACK:
            return ITEMSLOT_CAPE;

        case INVSLOT_TABARD:
            return ITEMSLOT_TABARD;

        default:
            return NUM_ITEM_SLOTS;
    }
}

// VERIFY: d14b01cc manual
// OFFSET: 0x004EE460
void CCharacterComponent::RemoveItem(ITEM_SLOT itemSlot) {
    if (!this->m_itemDisplayID[itemSlot]) {
        return;
    }

    this->m_flags |= 0x4;

    switch (itemSlot) {
        case ITEMSLOT_HEAD: {
            this->m_data.m_geosets[CHARGEOSET_HAIR] = GetConditionalGeoset(&this->m_data.m_preferences);
            CCharacterComponent::RemoveLinkpt(this->m_data.m_model, ATTACH_HELM);
            CharacterFacialHairStylesRec* styleRec = GetConditionalFacialHairStyle(&this->m_data.m_preferences);

            if (styleRec) {
                this->m_data.m_geosets[CHARGEOSET_BEARD] = styleRec->m_geoset[0] + 100;
                this->m_data.m_geosets[CHARGEOSET_MOUSTACHE] = styleRec->m_geoset[1] + 300;
                this->m_data.m_geosets[CHARGEOSET_SIDEBURN] = styleRec->m_geoset[2] + 200;
                this->m_data.m_geosets[CHARGEOSET_EAR] = 702;
                this->m_data.m_geosets[CHARGEOSET_FACIALJEWELRY] = styleRec->m_geoset[3] + 1600;
                this->m_data.m_geosets[CHARGEOSET_EYEEFFECTS] = styleRec->m_geoset[4] + 1700;
            }

            this->m_itemDisplayID[itemSlot] = 0;
            return;
        }

        case ITEMSLOT_SHOULDER:
            CCharacterComponent::RemoveLinkpt(this->m_data.m_model, static_cast<GEOCOMPONENTLINKS>(6));
            CCharacterComponent::RemoveLinkpt(this->m_data.m_model, static_cast<GEOCOMPONENTLINKS>(5));
            this->m_itemDisplayID[itemSlot] = 0;
            return;

        case ITEMSLOT_CHEST:
            this->m_flags &= ~0x20;
            break;

        case ITEMSLOT_TABARD:
            if (this->m_flags & 0x40) {
                this->m_itemDisplayID[itemSlot] = 0;
                return;
            }

            this->m_data.m_geosets[CHARGEOSET_CAPE] = 1501;
            this->ClearGuildTexture(4);
            break;

        case ITEMSLOT_CAPE:
            this->m_data.m_geosets[CHARGEOSET_CAPE] = 1501;
            this->m_itemDisplayID[itemSlot] = 0;
            return;

        case ITEMSLOT_11:
            if (this->m_itemSlotForAttachSlot[ATTACH_SHEATH_MAINHAND] == ITEMSLOT_11) {
                CCharacterComponent::RemoveLinkpt(this->m_data.m_model, ATTACH_SHEATH_MAINHAND);
                this->m_itemSlotForAttachSlot[ATTACH_SHEATH_MAINHAND] = -1;
            }

            this->m_itemDisplayID[itemSlot] = 0;
            return;

        default:
            break;
    }

    const ItemDisplayInfoRec* displayRec = nullptr;
    ItemDisplayInfoRec rec;

    if (g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[itemSlot], &rec)) {
        displayRec = &rec;
    }

    this->m_itemDisplayID[itemSlot] = 0;

    for (int32_t section = 0; section < 8; section++) {
        (this->*CCharacterComponent::m_itemFunc[section])(itemSlot, displayRec, false);
    }
}

// VERIFY: 9f6bde71 manual
// OFFSET: 0x004EE6D0
void CCharacterComponent::RemoveItemBySlot(INVENTORY_SLOTS invSlot) {
    ITEM_SLOT itemSlot = CCharacterComponent::GetItemSlotForInvSlot(invSlot);

    if (itemSlot != NUM_ITEM_SLOTS) {
        this->RemoveItem(itemSlot);
    }
}

// VERIFY: c379cf14 manual
// OFFSET: 0x004EEB30
void CCharacterComponent::RemoveItemByType(INVENTORY_TYPES invType) {
    ITEM_SLOT itemSlot = CCharacterComponent::GetItemSlotForInvType(invType);

    if (itemSlot != NUM_ITEM_SLOTS) {
        this->RemoveItem(itemSlot);
    }
}

// VERIFY: 3d7c34fc manual
// OFFSET: 0x004EEFA0
int32_t CCharacterComponent::GetHelmModelFilePath(ItemDisplayInfoRec& rec, const ItemDisplayInfoRec*& displayRec) {
    if (!this->m_itemDisplayID[ITEMSLOT_HEAD]) {
        return 0;
    }

    if (!g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_HEAD], &rec) || !*rec.m_modelName[0]) {
        return 0;
    }

    displayRec = &rec;
    ChrRacesRec* raceRec = g_chrRacesDB.GetRecord(this->m_data.m_preferences.m_raceID);

    if (!raceRec) {
        return 0;
    }

    return this->ComposeHelmModelFilePath(raceRec, &rec, s_pathEnd, 260);
}

// VERIFY: 05b10ee3 auto
// OFFSET: 0x004EF020
bool CCharacterComponent::IsHelmModelCorrect() {
    CM2Model* attached = this->m_data.m_model->m_attachList;

    while (attached && attached->m_attachmentId != ATTACH_HELM) {
        attached = attached->m_attachNext;
    }

    if (!attached) {
        return false;
    }

    ItemDisplayInfoRec rec;
    const ItemDisplayInfoRec* displayRec;

    if (!this->GetHelmModelFilePath(rec, displayRec)) {
        return false;
    }

    strcpy(OsPathFindExtensionWithoutDot(s_path), "m2");

    return SStrCmpI(attached->m_shared->m_fileName, s_path, 0x7FFFFFFF) == 0;
}

// VERIFY: 9417cc2f manual
// OFFSET: 0x004EF0D0
void CCharacterComponent::AddHelm(int32_t itemVisual) {
    ItemDisplayInfoRec rec;
    const ItemDisplayInfoRec* displayRec;

    if (!this->GetHelmModelFilePath(rec, displayRec)) {
        return;
    }

    strcpy(s_buffer, "Item\\ObjectComponents\\Head\\");
    strcat(s_buffer, displayRec->m_modelTexture[0]);
    strcat(s_buffer, ".blp");
    strcpy(s_pathEnd2, s_buffer);

    CCharacterComponent::AddLink(this->m_data.m_model, ATTACH_HELM, s_path, s_path2, itemVisual, displayRec);

    HelmetGeosetVisDataRec* visRec = g_helmetGeosetVisDataDB.GetRecord(displayRec->m_helmetGeosetVisID[this->m_data.m_preferences.m_sexID]);

    if (!visRec) {
        return;
    }

    uint32_t raceMask = 1 << this->m_data.m_preferences.m_raceID;

    if (raceMask & visRec->m_hideGeoset[0]) {
        this->m_data.m_geosets[CHARGEOSET_HAIR] = 1;
    }

    if (raceMask & visRec->m_hideGeoset[1]) {
        this->m_data.m_geosets[CHARGEOSET_BEARD] = 101;
    }

    if (raceMask & visRec->m_hideGeoset[2]) {
        this->m_data.m_geosets[CHARGEOSET_SIDEBURN] = 201;
    }

    if (raceMask & visRec->m_hideGeoset[3]) {
        this->m_data.m_geosets[CHARGEOSET_MOUSTACHE] = 301;
    }

    if (raceMask & visRec->m_hideGeoset[4]) {
        this->m_data.m_geosets[CHARGEOSET_EAR] = 701;
    }

    if (raceMask & visRec->m_hideGeoset[5]) {
        this->m_data.m_geosets[CHARGEOSET_FACIALJEWELRY] = 1601;
    }

    if (raceMask & visRec->m_hideGeoset[6]) {
        this->m_data.m_geosets[CHARGEOSET_EYEEFFECTS] = 1701;
    }
}

// VERIFY: 596646c0 manual
// OFFSET: 0x004EF250
int32_t CCharacterComponent::GetQuiverModelFilePath(ItemDisplayInfoRec& rec, const ItemDisplayInfoRec*& displayRec) {
    if (!this->m_itemDisplayID[ITEMSLOT_11]) {
        return 0;
    }

    if (!g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_11], &rec) || !*rec.m_modelName[0]) {
        return 0;
    }

    displayRec = &rec;
    strcpy(s_buffer, "Item\\ObjectComponents\\Quiver\\");
    strcat(s_buffer, displayRec->m_modelName[0]);
    strcpy(s_pathEnd, s_buffer);

    return 1;
}

// VERIFY: f69cbf50 auto
// OFFSET: 0x004EF300
bool CCharacterComponent::IsQuiverModelCorrect() {
    CM2Model* attached = this->m_data.m_model->m_attachList;

    while (attached && attached->m_attachmentId != ATTACH_SHEATH_MAINHAND) {
        attached = attached->m_attachNext;
    }

    if (!attached) {
        return false;
    }

    ItemDisplayInfoRec rec;
    const ItemDisplayInfoRec* displayRec;

    if (!this->GetQuiverModelFilePath(rec, displayRec)) {
        return false;
    }

    strcpy(OsPathFindExtensionWithoutDot(s_path), "m2");

    return SStrCmpI(attached->m_shared->m_fileName, s_path, 0x7FFFFFFF) == 0;
}

// VERIFY: 15c4da05 manual
// OFFSET: 0x004EF3B0
void CCharacterComponent::AddQuiver(int32_t itemVisual) {
    ItemDisplayInfoRec rec;
    const ItemDisplayInfoRec* displayRec;

    if (!this->GetQuiverModelFilePath(rec, displayRec)) {
        return;
    }

    strcpy(s_buffer, "Item\\ObjectComponents\\Quiver\\");
    strcat(s_buffer, displayRec->m_modelTexture[0]);
    strcat(s_buffer, ".blp");
    strcpy(s_pathEnd2, s_buffer);

    this->m_itemSlotForAttachSlot[ATTACH_SHEATH_MAINHAND] = ITEMSLOT_11;
    CCharacterComponent::AddLink(this->m_data.m_model, ATTACH_SHEATH_MAINHAND, s_path, s_path2, itemVisual, displayRec);
}

// VERIFY: 54fa8c8a manual
// OFFSET: 0x004EF4B0
int32_t CCharacterComponent::BuildShoulderItemPaths(ItemDisplayInfoRec& rec, const ItemDisplayInfoRec*& displayRec, char* leftModel, char* rightModel, char* leftTexture, char* rightTexture) {
    *leftModel = '\0';
    *rightModel = '\0';

    if (leftTexture) {
        *leftTexture = '\0';
    }

    if (rightTexture) {
        *rightTexture = '\0';
    }

    if (!this->m_itemDisplayID[ITEMSLOT_SHOULDER] || !g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_SHOULDER], &rec)) {
        return 0;
    }

    displayRec = &rec;

    if (*rec.m_modelName[0]) {
        strcpy(s_buffer, "Item\\ObjectComponents\\Shoulder\\");
        strcat(s_buffer, displayRec->m_modelName[0]);
        strcpy(leftModel, s_buffer);

        if (leftTexture) {
            strcpy(s_buffer, "Item\\ObjectComponents\\Shoulder\\");
            strcat(s_buffer, displayRec->m_modelTexture[0]);
            strcat(s_buffer, ".blp");
            strcpy(leftTexture, s_buffer);
        }
    }

    if (*displayRec->m_modelName[1]) {
        strcpy(s_buffer, "Item\\ObjectComponents\\Shoulder\\");
        strcat(s_buffer, displayRec->m_modelName[1]);
        strcpy(rightModel, s_buffer);

        if (rightTexture) {
            strcpy(s_buffer, "Item\\ObjectComponents\\Shoulder\\");
            strcat(s_buffer, displayRec->m_modelTexture[1]);
            strcat(s_buffer, ".blp");
            strcpy(rightTexture, s_buffer);
        }
    }

    return 1;
}

// VERIFY: 9f80d347 auto
// OFFSET: 0x004EF710
int32_t CCharacterComponent::AreShoulderModelsCorrect() {
    CM2Model* left = this->m_data.m_model->m_attachList;

    while (left && left->m_attachmentId != 6) {
        left = left->m_attachNext;
    }

    if (!left) {
        return 0;
    }

    CM2Model* right = this->m_data.m_model->m_attachList;

    while (right && right->m_attachmentId != 5) {
        right = right->m_attachNext;
    }

    if (!right) {
        return 0;
    }

    ItemDisplayInfoRec rec;
    const ItemDisplayInfoRec* displayRec;

    if (!this->BuildShoulderItemPaths(rec, displayRec, s_path, s_path2, nullptr, nullptr)) {
        return 0;
    }

    strcpy(OsPathFindExtensionWithoutDot(s_path), "m2");
    strcpy(OsPathFindExtensionWithoutDot(s_path2), "m2");

    if (SStrCmpI(left->m_shared->m_fileName, s_path, 0x7FFFFFFF) || SStrCmpI(right->m_shared->m_fileName, s_path2, 0x7FFFFFFF)) {
        return 0;
    }

    return 1;
}

// VERIFY: fb7d3f74 auto
// OFFSET: 0x004EF840
void CCharacterComponent::AddShoulders(int32_t itemVisual) {
    ItemDisplayInfoRec rec;
    const ItemDisplayInfoRec* displayRec;
    char leftModel[260];
    char rightModel[260];
    char leftTexture[260];
    char rightTexture[260];

    if (!this->BuildShoulderItemPaths(rec, displayRec, leftModel, rightModel, leftTexture, rightTexture)) {
        return;
    }

    if (leftModel[0]) {
        CCharacterComponent::AddLink(this->m_data.m_model, static_cast<GEOCOMPONENTLINKS>(6), leftModel, leftTexture, itemVisual, displayRec);
    }

    if (rightModel[0]) {
        CCharacterComponent::AddLink(this->m_data.m_model, static_cast<GEOCOMPONENTLINKS>(5), rightModel, rightTexture, itemVisual, displayRec);
    }
}

// VERIFY: 181a9fc1 auto
// OFFSET: 0x004EF900
void CCharacterComponent::SetHandItemDisplay(int32_t displayID, uint32_t hand, INVENTORY_SLOTS invSlot, bool shield) {
    ItemDisplayInfoRec rec;

    if (g_itemDisplayInfoDB.GetRecord(displayID, &rec)) {
        this->m_handItemDisplayID[hand] = displayID;
        CCharacterComponent::AddHandItem(this->m_data.m_model, &rec, invSlot, SHEATHE_NONE, false, shield, false, 0);
    }
}

// VERIFY: c7c8b21d auto
// OFFSET: 0x004EF970
void CCharacterComponent::SetMainHandItemDisplay(int32_t displayID) {
    this->SetHandItemDisplay(displayID, 0, INVSLOT_MAINHAND, false);
}

// VERIFY: 77fdc192 auto
// OFFSET: 0x004EF990
void CCharacterComponent::SetOffHandItemDisplay(int32_t displayID) {
    this->SetHandItemDisplay(displayID, 1, INVSLOT_OFFHAND, false);
}

// VERIFY: 38eb3fe0 auto
// OFFSET: 0x004EF9B0
void CCharacterComponent::SetShieldItemDisplay(int32_t displayID) {
    this->SetHandItemDisplay(displayID, 1, INVSLOT_OFFHAND, true);
}

// VERIFY: e411dd8f manual
// OFFSET: 0x004EF9D0
void CCharacterComponent::PasteScale(HTEXTURECACHE texture, MipBits* dstMips, const C2iVector& dstPos, const C2iVector& srcPos, const C2iVector& srcSize, TCTEXTUREINFO& srcInfo) {
    C2iVector pos = dstPos;
    C2iVector src = srcPos;
    C2iVector size = srcSize;

    switch (srcInfo.m_alphaSize) {
        case 0:
            CCharacterComponent::PasteOpaqueScale(texture, dstMips, pos, 4 * s_textureSize, src, size, srcInfo);
            break;

        case 1:
            CCharacterComponent::PasteTransparent1BitScale(texture, dstMips, pos, 4 * s_textureSize, src, size, srcInfo);
            break;

        case 4:
            CCharacterComponent::PasteTransparent4BitScale(texture, dstMips, pos, 4 * s_textureSize, src, size, srcInfo);
            break;

        case 8:
            CCharacterComponent::PasteTransparent8BitScale(texture, dstMips, pos, 4 * s_textureSize, src, size, srcInfo);
            break;
    }
}

// VERIFY: 16ff2fc9 manual
// OFFSET: 0x004EFAB0
void CCharacterComponent::ComputeLoadProgress(int64_t& local, int64_t& total) {
    for (int32_t variation = 0; variation < CHARTEXTUREVARIATIONS_NUM; variation++) {
        for (int32_t i = 0; i < 3; i++) {
            if (this->m_baseVariation[variation].m_texture[i]) {
                TextureCacheComputeLoadProgress(this->m_baseVariation[variation].m_texture[i], local, total);
            }
        }
    }

    for (int32_t section = 0; section < NUM_COMPONENT_SECTIONS; section++) {
        for (int32_t layer = 0; layer < 7; layer++) {
            if (this->m_section[section].m_layerTex[layer]) {
                TextureCacheComputeLoadProgress(this->m_section[section].m_layerTex[layer], local, total);
            }
        }
    }

    total += 0x2000;

    if (!this->m_link.m_next && this->m_baseSkinTexture && !(this->m_dirtySections | -static_cast<int32_t>(this->m_flags & 0x1))) {
        local += 0x2000;
    }
}

// VERIFY: 5c27f786 auto
// OFFSET: 0x004EFB80
static bool __stdcall ItemDisplayHasGeosetGroup(uint32_t displayID, int32_t group) {
    ItemDisplayInfoRec rec;

    return displayID && g_itemDisplayInfoDB.GetRecord(displayID, &rec) && rec.m_geosetGroup[group];
}

// INLINED
inline CharacterSection::CharacterSection() {
    for (int32_t layer = 0; layer < 7; layer++) {
        this->m_layerTex[layer] = nullptr;
        this->m_layerItemDisplayId[layer] = 0;
    }

    this->m_layerMask = 0;
}

// INLINED
inline CharacterSection::~CharacterSection() {
    for (int32_t layer = 0; layer < 7; layer++) {
        if (this->m_layerTex[layer]) {
            TextureCacheDestroyTexture(this->m_layerTex[layer]);
        }

        this->m_layerTex[layer] = nullptr;
        this->m_layerItemDisplayId[layer] = 0;
    }
}

// INLINED
inline CharacterBaseVariation::~CharacterBaseVariation() {
    HTEXTURECACHE* texture = this->m_texture;

    for (int32_t i = 0; i < 7; i++) {
        if (texture[i]) {
            TextureCacheDestroyTexture(texture[i]);
        }

        texture[i] = nullptr;
    }
}

// VERIFY: 1d57f8e3 manual
// OFFSET: 0x004EFBE0
CCharacterComponent::CCharacterComponent() {
    memset(this->m_baseVariation, 0, sizeof(this->m_baseVariation));
    this->m_flags |= 0x7;
    this->m_dirtySections = 0xFFFFFFFF;
    this->m_request = nullptr;
    this->m_baseSkinTexture = nullptr;
    this->m_gxTexFormat = CCharacterComponent::s_gxFormat;
    memset(this->m_itemDisplayID, 0, sizeof(this->m_itemDisplayID));
    memset(this->m_itemSlotForAttachSlot, 0xFF, sizeof(this->m_itemSlotForAttachSlot));
    this->m_flags &= ~0x20;
    this->m_data.m_model = nullptr;
}

// VERIFY: 4f3c1a66 manual
// OFFSET: 0x004EFCA0
CCharacterComponent::~CCharacterComponent() {
    if (this->m_request) {
        this->m_request->m_flags &= ~0x1;
        this->m_request = nullptr;
    }

    if (this->m_baseSkinTexture) {
        GxTexSetCannotUpdate(TextureGetGxTex(this->m_baseSkinTexture, 1, nullptr));
        HandleClose(reinterpret_cast<HOBJECT>(this->m_baseSkinTexture));
        this->m_baseSkinTexture = nullptr;
    }

    if (this->m_data.m_model) {
        this->m_data.m_model->Release();
        this->m_data.m_model = nullptr;
    }

    this->m_link.Unlink();
}

// VERIFY: 87fa3e57 manual
// OFFSET: 0x004EFDF0
void CCharacterComponent::UpdateBaseTexture(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    CCharacterComponent* component = static_cast<CCharacterComponent*>(userArg);

    switch (command) {
        case GxTex_Lock:
            if (!CCharacterComponent::s_bInRenderPrep) {
                component->RenderPrepAll();
            }

            break;

        case GxTex_Latch:
            if (component->m_gxTexFormat == GxTex_Dxt1) {
                uint32_t blockSize = TextureGetBytesPerBlock(PIXEL_DXT1);
                texelStrideInBytes = blockSize * (width >> 2);

                if (texelStrideInBytes < blockSize) {
                    texelStrideInBytes = blockSize;
                }
            } else {
                texelStrideInBytes = 4 * width;
            }

            if (component->m_request) {
                texels = component->m_request->m_mipBits->m_mipBits->m_mip[level];
            } else {
                MipBits* mipBits = s_compressedTextureBuffer;

                if (component->m_gxTexFormat != GxTex_Dxt1) {
                    mipBits = CCharacterComponent::s_textureBuffer;
                }

                texels = mipBits->m_mip[level];
            }

            break;

        case GxTex_Recreate:
            if (!(component->m_data.m_flags & 0x2)) {
                component->SetTexturesDirty();
                component->SetDirtySections(0xFFFFFFFF);
            }

            break;

        default:
            break;
    }
}

// VERIFY: 128e28ab auto
// OFFSET: 0x004EFED0
void CCharacterComponent::Prep() {
    if (this->m_data.m_flags & 0x1) {
        if (this->m_flags & 0x4) {
            this->GeosRenderPrep();
        }

        return;
    }

    if (this->m_dirtySections || (this->m_flags & 0x1)) {
        if (!this->m_link.m_next) {
            CCharacterComponent::s_queue.LinkToTail(this);
        }
    } else if (this->m_flags & 0x4) {
        this->GeosRenderPrep();
    }
}

// VERIFY: f0225731 auto
// OFFSET: 0x004EFF10
void CCharacterComponent::CreateBaseTexture() {
    EGxTexFormat dataFormat = GxTex_Argb8888;

    if (this->m_gxTexFormat == GxTex_Dxt1) {
        dataFormat = GxTex_Dxt1;
    }

    CGxTexFlags flags(GxTex_LinearMipLinear, 0, 0, 0, 0, 0, 1);

    if (g_theGxDevicePtr->DeviceApi() == GxApi_GLL) {
        flags.m_bit14 = 1;
    }

    this->m_baseSkinTexture = TextureCreate(s_textureSize, s_textureSize, this->m_gxTexFormat, dataFormat, flags, this, &CCharacterComponent::UpdateBaseTexture, "CharacterBaseSkin", 1);
    this->m_data.m_model->ReplaceTexture(M2TEX_SKIN, this->m_baseSkinTexture);
}

// VERIFY: 51c5a21d manual
// OFFSET: 0x004EFFA0
void CCharacterComponent::UpdateItem(ITEM_SLOT itemSlot, COMPONENT_SECTIONS section, const ItemDisplayInfoRec* displayRec, bool apply) {
    int32_t layer = s_itemPriority[itemSlot][section];

    if (section == SECTION_ARM_LOWER) {
        if (displayRec && displayRec->m_geosetGroup[0]) {
            if (itemSlot == ITEMSLOT_CHEST) {
                layer = 5;
            } else if (itemSlot == ITEMSLOT_GLOVES) {
                layer = 6;
            }
        }
    } else if (section == SECTION_LEG_LOWER) {
        if (itemSlot == ITEMSLOT_CHEST) {
            if (displayRec && displayRec->m_geosetGroup[2]) {
                layer = 4;
                this->m_flags |= 0x4;
            }
        } else if (itemSlot == ITEMSLOT_PANTS) {
            if (displayRec && displayRec->m_geosetGroup[2]) {
                uint32_t chestID = this->m_itemDisplayID[ITEMSLOT_CHEST];

                if (chestID && ItemDisplayHasGeosetGroup(chestID, 2)) {
                    layer = 3;
                } else {
                    layer = 4;
                }

                this->m_flags |= 0x4;
            }
        } else if (itemSlot == ITEMSLOT_BOOTS && displayRec && displayRec->m_geosetGroup[0]) {
            this->m_flags |= 0x4;
            layer = 3;
        }
    } else if (section == SECTION_FOOT && (this->m_flags & 0x10)) {
        apply = false;
        this->FreeSectionTexture(section, s_itemPriority[itemSlot][section]);
    }

    if (section != SECTION_FOOT || !(this->m_flags & 0x10)) {
        if (apply) {
            if (!this->UpdateTextureSlot(section, displayRec, layer)) {
                return;
            }
        } else {
            this->FreeSectionTexture(section, layer);
        }
    }

    if (section == SECTION_LEG_LOWER && !apply) {
        ItemDisplayInfoRec rec;

        if (itemSlot == ITEMSLOT_CHEST) {
            if (ItemDisplayHasGeosetGroup(this->m_itemDisplayID[ITEMSLOT_PANTS], 2, rec)) {
                this->FreeSectionTexture(SECTION_LEG_LOWER, 3);
                this->UpdateTextureSlot(SECTION_LEG_LOWER, &rec, 4);
            }

            if (ItemDisplayHasGeosetGroup(this->m_itemDisplayID[ITEMSLOT_BOOTS], 0, rec)) {
                this->UpdateTextureSlot(SECTION_LEG_LOWER, &rec, 3);
            }
        } else if (itemSlot == ITEMSLOT_PANTS && ItemDisplayHasGeosetGroup(this->m_itemDisplayID[ITEMSLOT_PANTS], 2, rec)) {
            this->UpdateTextureSlot(SECTION_LEG_LOWER, &rec, 4);
        }
    }

    if (layer != -1) {
        this->SetDirtySections(1 << section);
    }
}

// VERIFY: d1dcf932 manual
// OFFSET: 0x004F01A0
void CCharacterComponent::UpdateItemAU(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply) {
    int32_t layer = s_itemPriority[itemSlot][SECTION_ARM_UPPER];

    if (apply) {
        if (!this->UpdateTextureSlot(SECTION_ARM_UPPER, displayRec, layer)) {
            return;
        }
    } else {
        this->FreeSectionTexture(SECTION_ARM_UPPER, layer);
    }

    if (layer != -1) {
        this->SetDirtySections(1 << SECTION_ARM_UPPER);
    }
}

// VERIFY: 72469327 manual
// OFFSET: 0x004F0200
void CCharacterComponent::UpdateItemAL(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply) {
    int32_t layer = s_itemPriority[itemSlot][SECTION_ARM_LOWER];

    if (displayRec && displayRec->m_geosetGroup[0]) {
        if (itemSlot == ITEMSLOT_CHEST) {
            layer = 5;
        } else if (itemSlot == ITEMSLOT_GLOVES) {
            layer = 6;
        }
    }

    if (apply) {
        if (!this->UpdateTextureSlot(SECTION_ARM_LOWER, displayRec, layer)) {
            return;
        }
    } else {
        this->FreeSectionTexture(SECTION_ARM_LOWER, layer);
    }

    if (layer != -1) {
        this->SetDirtySections(1 << SECTION_ARM_LOWER);
    }
}

// VERIFY: b2a641bd manual
// OFFSET: 0x004F0280
void CCharacterComponent::UpdateItemHA(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply) {
    int32_t layer = s_itemPriority[itemSlot][SECTION_HAND];

    if (apply) {
        if (!this->UpdateTextureSlot(SECTION_HAND, displayRec, layer)) {
            return;
        }
    } else {
        this->FreeSectionTexture(SECTION_HAND, layer);
    }

    if (layer != -1) {
        this->SetDirtySections(1 << SECTION_HAND);
    }
}

// VERIFY: 48c37d5b auto
// OFFSET: 0x0047ADD0
void CCharacterComponent::UpdateItemHU(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply) {
}

// VERIFY: 00cdc5bc auto
// OFFSET: 0x0047ADD0
void CCharacterComponent::UpdateItemHL(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply) {
}

// VERIFY: de458089 manual
// OFFSET: 0x004F02E0
void CCharacterComponent::UpdateItemTU(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply) {
    int32_t layer = s_itemPriority[itemSlot][SECTION_TORSO_UPPER];

    if (apply) {
        if (!this->UpdateTextureSlot(SECTION_TORSO_UPPER, displayRec, layer)) {
            return;
        }
    } else {
        this->FreeSectionTexture(SECTION_TORSO_UPPER, layer);
    }

    if (layer != -1) {
        this->SetDirtySections(1 << SECTION_TORSO_UPPER);
    }
}

// VERIFY: b3ca5ecd manual
// OFFSET: 0x004F0340
void CCharacterComponent::UpdateItemTL(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply) {
    int32_t layer = s_itemPriority[itemSlot][SECTION_TORSO_LOWER];

    if (apply) {
        if (!this->UpdateTextureSlot(SECTION_TORSO_LOWER, displayRec, layer)) {
            return;
        }
    } else {
        this->FreeSectionTexture(SECTION_TORSO_LOWER, layer);
    }

    if (layer != -1) {
        this->SetDirtySections(1 << SECTION_TORSO_LOWER);
    }
}

// VERIFY: 6227a595 manual
// OFFSET: 0x004F03A0
void CCharacterComponent::UpdateItemLU(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply) {
    int32_t layer = s_itemPriority[itemSlot][SECTION_LEG_UPPER];

    if (apply) {
        if (!this->UpdateTextureSlot(SECTION_LEG_UPPER, displayRec, layer)) {
            return;
        }
    } else {
        this->FreeSectionTexture(SECTION_LEG_UPPER, layer);
    }

    if (layer != -1) {
        this->SetDirtySections(1 << SECTION_LEG_UPPER);
    }
}

// VERIFY: 1aee57db auto
// OFFSET: 0x004F0400
void CCharacterComponent::UpdateItemLL(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply) {
    this->UpdateItem(itemSlot, SECTION_LEG_LOWER, displayRec, apply);
}

// VERIFY: 0de7be2d auto
// OFFSET: 0x004F0420
void CCharacterComponent::UpdateItemFO(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply) {
    int32_t layer = s_itemPriority[itemSlot][SECTION_FOOT];

    if (!(this->m_flags & 0x10) && apply) {
        if (!this->UpdateTextureSlot(SECTION_FOOT, displayRec, layer)) {
            return;
        }
    } else {
        this->FreeSectionTexture(SECTION_FOOT, layer);
    }

    if (layer != -1) {
        this->SetDirtySections(1 << SECTION_FOOT);
    }
}

// VERIFY: 4a14bd22 auto
// OFFSET: 0x004F0490
bool CCharacterComponent::SetNextHairStyle(COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    int32_t numVariations = ComponentGetNumVariations(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR);

    if (numVariations <= 0) {
        return false;
    }

    int32_t hairStyleID = this->m_data.m_preferences.m_hairStyleId + 1;

    if (hairStyleID >= numVariations) {
        hairStyleID = 0;
    }

    if (hairStyleID == static_cast<int32_t>(this->m_data.m_preferences.m_hairStyleId)) {
        return false;
    }

    CharSectionsRec* rec = nullptr;

    while (true) {
        int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR, hairStyleID);
        int32_t color;

        for (color = 0; color < numColors; color++) {
            rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR, hairStyleID, color, nullptr);

            if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
                break;
            }
        }

        if (color < numColors) {
            break;
        }

        if (++hairStyleID >= numVariations) {
            hairStyleID = 0;
        }

        if (hairStyleID == static_cast<int32_t>(this->m_data.m_preferences.m_hairStyleId)) {
            return false;
        }
    }

    CharSectionsRec* keepRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR, hairStyleID, this->m_data.m_preferences.m_hairColorId, nullptr);

    if (keepRec && ComponentFlagsMatch(keepRec->m_flags, selection)) {
        this->SetHairStyle(keepRec->m_variationIndex, nullptr);
        return true;
    }

    this->SetHairColor(rec->m_colorIndex, true, nullptr);
    this->SetHairStyle(rec->m_variationIndex, nullptr);

    int32_t facialHairID = CCharacterComponent::GetNthFacialFeatureIndex(this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, this->m_data.m_preferences.m_classId, this->m_data.m_preferences.m_hairColorId, this->m_data.m_preferences.m_facialHairId, 0, GetSelectionFromContext(context, this->m_data.m_preferences.m_classId));

    if (facialHairID >= 0) {
        this->SetBeardStyle(facialHairID, true, nullptr);
    }

    return true;
}

// VERIFY: bd306e5b auto
// OFFSET: 0x004F0630
bool CCharacterComponent::SetPrevHairStyle(COMPONENT_CONTEXT context) {
    COMPONENT_SELECTION selection = GetSelectionFromContext(context, this->m_data.m_preferences.m_classId);
    int32_t numVariations = ComponentGetNumVariations(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR);

    if (numVariations <= 0) {
        return false;
    }

    int32_t hairStyleID = this->m_data.m_preferences.m_hairStyleId - 1;

    if (hairStyleID < 0) {
        hairStyleID = numVariations - 1;
    }

    if (hairStyleID == static_cast<int32_t>(this->m_data.m_preferences.m_hairStyleId)) {
        return false;
    }

    CharSectionsRec* rec = nullptr;

    while (true) {
        int32_t numColors = ComponentGetNumColors(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR, hairStyleID);
        int32_t color;

        for (color = 0; color < numColors; color++) {
            rec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR, hairStyleID, color, nullptr);

            if (rec && ComponentFlagsMatch(rec->m_flags, selection)) {
                break;
            }
        }

        if (color < numColors) {
            break;
        }

        if (--hairStyleID < 0) {
            hairStyleID = numVariations - 1;
        }

        if (hairStyleID == static_cast<int32_t>(this->m_data.m_preferences.m_hairStyleId)) {
            return false;
        }
    }

    CharSectionsRec* keepRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_HAIR, hairStyleID, this->m_data.m_preferences.m_hairColorId, nullptr);

    if (keepRec && ComponentFlagsMatch(keepRec->m_flags, selection)) {
        this->SetHairStyle(keepRec->m_variationIndex, nullptr);
        return true;
    }

    this->SetHairColor(rec->m_colorIndex, true, nullptr);
    this->SetHairStyle(rec->m_variationIndex, nullptr);

    int32_t facialHairID = CCharacterComponent::GetNthFacialFeatureIndex(this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, this->m_data.m_preferences.m_classId, this->m_data.m_preferences.m_hairColorId, this->m_data.m_preferences.m_facialHairId, 0, GetSelectionFromContext(context, this->m_data.m_preferences.m_classId));

    if (facialHairID >= 0) {
        this->SetBeardStyle(facialHairID, true, nullptr);
    }

    return true;
}

// VERIFY: f3be180c auto
// OFFSET: 0x004F07D0
void CCharacterComponent::PasteFromSkin(COMPONENT_SECTIONS section, HTEXTURECACHE texture, MipBits* dstMips) {
    if (!TextureCacheHasMips(texture)) {
        return;
    }

    TCTEXTUREINFO info;
    info.m_width = 0;
    info.m_height = 0;
    info.m_mipCount = 0;
    info.m_alphaSize = 0;
    info.m_opaque = 1;
    TextureCacheGetInfo(texture, info, 1);
    uint32_t width = info.m_width;
    info.m_alphaSize = 0;

    if (info.m_width < s_textureSize && info.m_height < s_textureSize) {
        CCharacterComponent::PasteScale(texture, dstMips, s_sectionInfo[section].m_pos, s_sectionInfo[section].m_pos, s_sectionInfo[section].m_size, info);
        return;
    }

    uint32_t mipLevel = 0;

    while (width != s_textureSize) {
        width >>= 1;
        mipLevel++;
    }

    CCharacterComponent::Paste(texture, dstMips, s_sectionInfo[section].m_pos, s_sectionInfo[section].m_pos, s_sectionInfo[section].m_size, info, mipLevel);
}

// VERIFY: c9c0de88 auto
// OFFSET: 0x004F08A0
void CCharacterComponent::PasteToSection(COMPONENT_SECTIONS section, HTEXTURECACHE texture, MipBits* dstMips) {
    if (!TextureCacheHasMips(texture)) {
        return;
    }

    TCTEXTUREINFO info;
    info.m_width = 0;
    info.m_height = 0;
    info.m_mipCount = 0;
    info.m_alphaSize = 0;
    info.m_opaque = 1;
    TextureCacheGetInfo(texture, info, 1);
    uint32_t width = info.m_width;
    uint32_t sectionWidth = s_sectionInfo[section].m_size.x;
    C2iVector srcPos;

    if (info.m_width < sectionWidth && info.m_height < static_cast<uint32_t>(s_sectionInfo[section].m_size.y)) {
        srcPos.x = 0;
        srcPos.y = 0;
        CCharacterComponent::PasteScale(texture, dstMips, s_sectionInfo[section].m_pos, srcPos, s_sectionInfo[section].m_size, info);
        return;
    }

    uint32_t mipLevel = 0;

    while (width != sectionWidth) {
        width >>= 1;
        mipLevel++;
    }

    srcPos.x = 0;
    srcPos.y = 0;
    CCharacterComponent::Paste(texture, dstMips, s_sectionInfo[section].m_pos, srcPos, s_sectionInfo[section].m_size, info, mipLevel);
}

// VERIFY: a9b97d50 auto
// OFFSET: 0x004F0980
CCharacterComponent* CCharacterComponent::AllocComponent() {
    uint32_t memHandle;
    void* object = nullptr;

    if (!ObjectAlloc(*s_componentHeap, &memHandle, &object, false)) {
        return nullptr;
    }

    CCharacterComponent* component = new (object) CCharacterComponent();
    component->m_heapIndex = memHandle;

    return component;
}

// VERIFY: 88917dfa manual
// OFFSET: 0x004F09D0
void CCharacterComponent::RenderPrepAU() {
    CCharacterComponent::PasteFromSkin(SECTION_ARM_UPPER, this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0], CCharacterComponent::s_textureBuffer);

    for (int32_t layer = 0; layer < 2; layer++) {
        if ((1 << layer) & this->m_section[SECTION_ARM_UPPER].m_layerMask) {
            CCharacterComponent::PasteToSection(SECTION_ARM_UPPER, this->m_section[SECTION_ARM_UPPER].m_layerTex[layer], CCharacterComponent::s_textureBuffer);
        }
    }
}

// VERIFY: 274a74cd manual
// OFFSET: 0x004F0A30
void CCharacterComponent::RenderPrepAL() {
    CCharacterComponent::PasteFromSkin(SECTION_ARM_LOWER, this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0], CCharacterComponent::s_textureBuffer);

    for (int32_t layer = 0; layer < 7; layer++) {
        if ((1 << layer) & this->m_section[SECTION_ARM_LOWER].m_layerMask) {
            CCharacterComponent::PasteToSection(SECTION_ARM_LOWER, this->m_section[SECTION_ARM_LOWER].m_layerTex[layer], CCharacterComponent::s_textureBuffer);
        }
    }
}

// VERIFY: 7c6cc546 auto
// OFFSET: 0x004F0A90
void CCharacterComponent::RenderPrepHA() {
    CCharacterComponent::PasteFromSkin(SECTION_HAND, this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0], CCharacterComponent::s_textureBuffer);

    if (this->m_section[SECTION_HAND].m_layerMask & 0x1) {
        CCharacterComponent::PasteToSection(SECTION_HAND, this->m_section[SECTION_HAND].m_layerTex[0], CCharacterComponent::s_textureBuffer);
    }
}

// VERIFY: ba0a1b45 auto
// OFFSET: 0x004F0AD0
void CCharacterComponent::RenderPrepHU() {
    CharSectionsRec* skinRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, this->m_data.m_preferences.m_skinID, nullptr);

    if (skinRec && (skinRec->m_flags & 0x8)) {
        CCharacterComponent::PasteFromSkin(SECTION_HEAD_UPPER, this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0], CCharacterComponent::s_textureBuffer);
    }

    if (this->m_baseVariation[CHARTEXTUREVAR_FACE].m_texture[1]) {
        CCharacterComponent::PasteToSection(SECTION_HEAD_UPPER, this->m_baseVariation[CHARTEXTUREVAR_FACE].m_texture[1], CCharacterComponent::s_textureBuffer);
    }

    if (this->m_baseVariation[CHARTEXTUREVAR_FACIALHAIR].m_texture[1]) {
        CCharacterComponent::PasteToSection(SECTION_HEAD_UPPER, this->m_baseVariation[CHARTEXTUREVAR_FACIALHAIR].m_texture[1], CCharacterComponent::s_textureBuffer);
    }

    if (this->m_baseVariation[CHARTEXTUREVAR_HAIR].m_texture[2]) {
        CCharacterComponent::PasteToSection(SECTION_HEAD_UPPER, this->m_baseVariation[CHARTEXTUREVAR_HAIR].m_texture[2], CCharacterComponent::s_textureBuffer);
    }
}

// VERIFY: 617f148e auto
// OFFSET: 0x004F0B70
void CCharacterComponent::RenderPrepHL() {
    CharSectionsRec* skinRec = ComponentGetSectionsRecord(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, this->m_data.m_preferences.m_skinID, nullptr);

    if (skinRec && (skinRec->m_flags & 0x8)) {
        CCharacterComponent::PasteFromSkin(SECTION_HEAD_LOWER, this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0], CCharacterComponent::s_textureBuffer);
    }

    if (this->m_baseVariation[CHARTEXTUREVAR_FACE].m_texture[0]) {
        CCharacterComponent::PasteToSection(SECTION_HEAD_LOWER, this->m_baseVariation[CHARTEXTUREVAR_FACE].m_texture[0], CCharacterComponent::s_textureBuffer);
    }

    if (this->m_baseVariation[CHARTEXTUREVAR_FACIALHAIR].m_texture[0]) {
        CCharacterComponent::PasteToSection(SECTION_HEAD_LOWER, this->m_baseVariation[CHARTEXTUREVAR_FACIALHAIR].m_texture[0], CCharacterComponent::s_textureBuffer);
    }

    if (this->m_baseVariation[CHARTEXTUREVAR_HAIR].m_texture[1]) {
        CCharacterComponent::PasteToSection(SECTION_HEAD_LOWER, this->m_baseVariation[CHARTEXTUREVAR_HAIR].m_texture[1], CCharacterComponent::s_textureBuffer);
    }
}

// VERIFY: 9b31b577 manual
// OFFSET: 0x004F0C10
void CCharacterComponent::RenderPrepTU() {
    CCharacterComponent::PasteFromSkin(SECTION_TORSO_UPPER, this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0], CCharacterComponent::s_textureBuffer);

    if (!(this->m_section[SECTION_TORSO_UPPER].m_layerMask & 0x7) && this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[1]) {
        CCharacterComponent::PasteToSection(SECTION_TORSO_UPPER, this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[1], CCharacterComponent::s_textureBuffer);
    }

    for (int32_t layer = 0; layer < 5; layer++) {
        if ((1 << layer) & this->m_section[SECTION_TORSO_UPPER].m_layerMask) {
            CCharacterComponent::PasteToSection(SECTION_TORSO_UPPER, this->m_section[SECTION_TORSO_UPPER].m_layerTex[layer], CCharacterComponent::s_textureBuffer);
        }
    }
}

// VERIFY: 7c74bc85 manual
// OFFSET: 0x004F0CA0
void CCharacterComponent::RenderPrepTL() {
    CCharacterComponent::PasteFromSkin(SECTION_TORSO_LOWER, this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0], CCharacterComponent::s_textureBuffer);

    for (int32_t layer = 0; layer < 7; layer++) {
        if ((1 << layer) & this->m_section[SECTION_TORSO_LOWER].m_layerMask) {
            CCharacterComponent::PasteToSection(SECTION_TORSO_LOWER, this->m_section[SECTION_TORSO_LOWER].m_layerTex[layer], CCharacterComponent::s_textureBuffer);
        }
    }
}

// VERIFY: a637f73d manual
// OFFSET: 0x004F0D00
void CCharacterComponent::RenderPrepLU() {
    CCharacterComponent::PasteFromSkin(SECTION_LEG_UPPER, this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0], CCharacterComponent::s_textureBuffer);

    if (((this->m_flags & 0x20) || !(this->m_section[SECTION_LEG_UPPER].m_layerMask & 0x3)) && this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[0]) {
        CCharacterComponent::PasteToSection(SECTION_LEG_UPPER, this->m_baseVariation[VARIATION_UNDERWEAR].m_texture[0], CCharacterComponent::s_textureBuffer);
    }

    int32_t last = 3 - ((this->m_flags & 0x20) != 0);

    for (int32_t layer = (this->m_flags & 0x20) != 0; layer < last; layer++) {
        if ((1 << layer) & this->m_section[SECTION_LEG_UPPER].m_layerMask) {
            CCharacterComponent::PasteToSection(SECTION_LEG_UPPER, this->m_section[SECTION_LEG_UPPER].m_layerTex[layer], CCharacterComponent::s_textureBuffer);
        }
    }
}

// VERIFY: 65c5e194 manual
// OFFSET: 0x004F0DB0
void CCharacterComponent::RenderPrepLL() {
    CCharacterComponent::PasteFromSkin(SECTION_LEG_LOWER, this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0], CCharacterComponent::s_textureBuffer);

    int32_t last = 2 * ((this->m_flags & 0x20) == 0) + 4;

    for (int32_t layer = (this->m_flags & 0x20) != 0; layer < last; layer++) {
        if ((1 << layer) & this->m_section[SECTION_LEG_LOWER].m_layerMask) {
            CCharacterComponent::PasteToSection(SECTION_LEG_LOWER, this->m_section[SECTION_LEG_LOWER].m_layerTex[layer], CCharacterComponent::s_textureBuffer);
        }
    }
}

// VERIFY: 90820596 auto
// OFFSET: 0x004F0E40
void CCharacterComponent::RenderPrepFO() {
    CCharacterComponent::PasteFromSkin(SECTION_FOOT, this->m_baseVariation[CHARTEXTUREVAR_SKIN].m_texture[0], CCharacterComponent::s_textureBuffer);

    if (this->m_section[SECTION_FOOT].m_layerMask & 0x1) {
        CCharacterComponent::PasteToSection(SECTION_FOOT, this->m_section[SECTION_FOOT].m_layerTex[0], CCharacterComponent::s_textureBuffer);
    }
}

// VERIFY: 3f0fe5f0 auto
// OFFSET: 0x004F0E80
void CCharacterComponent::ProcessFinishedRequest() {
    CCharacterComponent::s_bInRenderPrep = 1;

    if (this->m_flags & 0x4) {
        this->GeosRenderPrep();
    }

    if (!this->m_baseSkinTexture) {
        this->CreateBaseTexture();
    }

    this->UpdateSections();
    this->m_request->m_flags &= ~0x1;
    this->m_flags &= ~0x1;
    this->m_request = nullptr;
    this->m_dirtySections = 0;
    this->DestroyRenderTextures();
    CCharacterComponent::s_bInRenderPrep = 0;
}

// VERIFY: f8ef570f auto
// OFFSET: 0x004F0EE0
void CCharacterComponent::ProcessComponentRequest(CComponentRequest* request) {
    MipBits* dstMips = request->m_mipBits->m_mipBits;

    if (request->m_format == GxTex_Dxt1) {
        dstMips = s_threadTextureBuffer;
    }

    for (int32_t section = 0; section < NUM_COMPONENT_SECTIONS; section++) {
        if (request->m_baseTextures[section]) {
            CCharacterComponent::PasteFromSkin(static_cast<COMPONENT_SECTIONS>(section), request->m_baseTextures[section], dstMips);
        }
    }

    for (uint32_t i = 0; i < request->m_layerCount; i++) {
        CCharacterComponent::PasteToSection(static_cast<COMPONENT_SECTIONS>(request->m_layerSections[i]), request->m_layerTextures[i], dstMips);
    }

    if (request->m_format == GxTex_Dxt1) {
        ComputeMipLevels(dstMips, request->m_mipBits->m_mipBits);
    }
}

// VERIFY: aaf196df manual
// OFFSET: 0x004F0F70
void CCharacterComponent::CreateComponentFreeLists() {
    int32_t count = 10;
    int32_t compressedCount = 0;

    if (CCharacterComponent::s_gxFormat == GxTex_Dxt1) {
        count = 1;
        compressedCount = 9;
    }

    for (int32_t i = 0; i < count; i++) {
        CComponentMipBits* mipBits = s_mipBitsFreeList.NewNode(2, 0, 0);
        mipBits->m_format = PIXEL_ARGB8888;
        mipBits->m_mipBits = TextureAllocMippedImg(PIXEL_ARGB8888, s_textureSize, s_textureSize);
    }

    for (int32_t i = 0; i < compressedCount; i++) {
        CComponentMipBits* mipBits = s_mipBitsCompressedFreeList.NewNode(2, 0, 0);
        mipBits->m_format = PIXEL_DXT1;
        mipBits->m_mipBits = TextureAllocMippedImg(PIXEL_DXT1, s_textureSize, s_textureSize);
    }
}

// VERIFY: 96925206 manual
// OFFSET: 0x004F10E0
CComponentRequest* CCharacterComponent::AllocComponentRequest() {
    CComponentRequest* request = s_requestFreeList.Head();

    if (!request) {
        request = s_requestFreeList.NewNode(1, 0, 0);
    }

    request->m_link.Unlink();
    request->m_flags = (request->m_flags & ~0x3) | 0x1;
    request->m_mipBits = nullptr;
    request->m_layerCount = 0;

    return request;
}

// VERIFY: 3f06c490 manual
// OFFSET: 0x004F1180
void CCharacterComponent::ProcessComponentFinishedList(int32_t all) {
    s_finishedListCritSect.Enter();

    for (CComponentRequest* request = s_finishedList.Head(); request;) {
        CComponentRequest* next = s_finishedList.Next(request);

        if (all || !(request->m_flags & 0x1)) {
            request->m_link.Unlink();
            request->DestroyCachedTextures();

            if (request->m_mipBits) {
                s_mipBitsCritSect.Enter();

                if (request->m_mipBits->m_format) {
                    s_mipBitsFreeList.LinkToTail(request->m_mipBits);
                } else {
                    s_mipBitsCompressedFreeList.LinkToTail(request->m_mipBits);
                }

                s_mipBitsCritSect.Leave();
                request->m_mipBits = nullptr;
            }

            s_requestFreeList.LinkToTail(request);
        }

        request = next;
    }

    s_finishedListCritSect.Leave();
}

// VERIFY: 76c73ebf manual
// OFFSET: 0x004F12B0
uint32_t STORMAPI CCharacterComponent::ThreadProc(void* param) {
    PropSelectContext(s_threadPropContext);
    s_threadBeginEvent.Wait(0xFFFFFFFF);

    while (CCharacterComponent::s_bComponentThread) {
        while (true) {
            s_requestListCritSect.Enter();
            CComponentRequest* request = s_requestList.Head();

            if (request) {
                request->m_link.Unlink();
            }

            s_requestListCritSect.Leave();

            if (!request) {
                break;
            }

            if (!(request->m_flags & 0x1)) {
                s_finishedListCritSect.Enter();
                s_finishedList.LinkToTail(request);
                s_finishedListCritSect.Leave();
                request->m_flags |= 0x2;
                continue;
            }

            CComponentMipBits* mipBits = CCharacterComponent::AllocComponentMipBits(request->m_format);

            if (!mipBits) {
                s_requestListCritSect.Enter();
                s_requestList.LinkToHead(request);
                s_requestListCritSect.Leave();
                OsSleep(10);
                continue;
            }

            request->m_mipBits = mipBits;
            CCharacterComponent::ProcessComponentRequest(request);
            s_finishedListCritSect.Enter();
            s_finishedList.LinkToTail(request);
            s_finishedListCritSect.Leave();
            request->m_flags |= 0x2;
            OsSleep(1);
        }

        OsSleep(1);
        s_threadBeginEvent.Wait(0xFFFFFFFF);
    }

    return 0;
}

// VERIFY: 39b04695 auto
// OFFSET: 0x004F14A0
void CCharacterComponent::RenderPrepSections() {
    CCharacterComponent::s_bInRenderPrep = 1;

    if (this->m_flags & 0x4) {
        this->GeosRenderPrep();
    }

    if (!this->m_baseSkinTexture) {
        this->CreateBaseTexture();
    }

    this->PrepSections();
    this->m_flags &= ~0x1;
    this->m_dirtySections = 0;
    this->DestroyRenderTextures();
    this->m_link.Unlink();
    CCharacterComponent::s_bInRenderPrep = 0;
}

// VERIFY: e78118c6 manual
// OFFSET: 0x004F1520
bool CCharacterComponent::RenderPrep(int32_t wait) {
    if (this->m_data.m_flags & 0x1) {
        if (this->m_flags & 0x4) {
            this->GeosRenderPrep();
        }

        return true;
    }

    if (!this->m_dirtySections && !(this->m_flags & 0x1)) {
        if (this->m_flags & 0x4) {
            this->GeosRenderPrep();
        }

        return true;
    }

    if (!(this->m_flags & 0x1)) {
        if (!this->m_link.m_next) {
            CCharacterComponent::s_queue.LinkToTail(this);
        }

        return true;
    }

    if (!wait) {
        if (!this->m_link.m_next) {
            CCharacterComponent::s_queue.LinkToTail(this);
        }

        return false;
    }

    if (this->m_request) {
        this->m_request->m_flags &= ~0x1;
        this->m_request = nullptr;
    }

    this->VariationsLoaded(1);
    this->ItemsLoaded(1);
    this->m_flags |= 0x8;
    this->RenderPrepSections();
    this->m_link.Unlink();

    return true;
}

// VERIFY: 3d69252b auto
// OFFSET: 0x004F16C0
void CCharacterComponent::FreeComponent(CCharacterComponent* component) {
    uint32_t* heap = s_componentHeap;
    component->~CCharacterComponent();
    ObjectFree(*heap, component->m_heapIndex);
}

// VERIFY: 6daa984b auto
// OFFSET: 0x004F16F0
int32_t CCharacterComponent::ThreadInitialize() {
    CCharacterComponent::s_requestFunc[SECTION_ARM_UPPER] = &CCharacterComponent::CreateRequestAU;
    CCharacterComponent::s_requestFunc[SECTION_ARM_LOWER] = &CCharacterComponent::CreateRequestAL;
    CCharacterComponent::s_requestFunc[SECTION_HAND] = &CCharacterComponent::CreateRequestHA;
    CCharacterComponent::s_requestFunc[SECTION_HEAD_UPPER] = &CCharacterComponent::CreateRequestHU;
    CCharacterComponent::s_requestFunc[SECTION_HEAD_LOWER] = &CCharacterComponent::CreateRequestHL;
    CCharacterComponent::s_requestFunc[SECTION_TORSO_UPPER] = &CCharacterComponent::CreateRequestTU;
    CCharacterComponent::s_requestFunc[SECTION_TORSO_LOWER] = &CCharacterComponent::CreateRequestTL;
    CCharacterComponent::s_requestFunc[SECTION_LEG_UPPER] = &CCharacterComponent::CreateRequestLU;
    CCharacterComponent::s_requestFunc[SECTION_LEG_LOWER] = &CCharacterComponent::CreateRequestLL;
    CCharacterComponent::s_requestFunc[SECTION_FOOT] = &CCharacterComponent::CreateRequestFO;
    CCharacterComponent::CreateComponentFreeLists();
    s_threadBeginEvent.Reset();
    s_threadPropContext = PropGetSelectedContext();

    return SThread::Create(&CCharacterComponent::ThreadProc, nullptr, s_thread, const_cast<char*>("Component"), 0);
}

// VERIFY: 672afabf manual
// OFFSET: 0x004F1790
void CCharacterComponent::CreateComponentRequest() {
    this->m_request = CCharacterComponent::AllocComponentRequest();
    this->m_request->m_format = this->m_gxTexFormat;

    for (int32_t section = 0; section < NUM_COMPONENT_SECTIONS; section++) {
        if ((1 << section) & this->m_dirtySections) {
            (this->*CCharacterComponent::s_requestFunc[section])();
        }
    }

    s_requestListCritSect.Enter();
    s_requestList.LinkToTail(this->m_request);
    s_requestListCritSect.Leave();
    s_threadBeginEvent.Set();
}

// VERIFY: bdaa94c5 manual
// OFFSET: 0x004F1850
void CCharacterComponent::DestroyComponentFreeLists() {
    for (CComponentMipBits* mipBits = s_mipBitsFreeList.Head(); mipBits; mipBits = s_mipBitsFreeList.Next(mipBits)) {
        TextureFreeMippedImg(mipBits->m_mipBits, mipBits->m_format, s_textureSize, s_textureSize);
    }

    for (CComponentMipBits* mipBits = s_mipBitsCompressedFreeList.Head(); mipBits; mipBits = s_mipBitsCompressedFreeList.Next(mipBits)) {
        TextureFreeMippedImg(mipBits->m_mipBits, mipBits->m_format, s_textureSize, s_textureSize);
    }

    s_mipBitsFreeList.Clear();
    s_mipBitsCompressedFreeList.Clear();
    s_requestFreeList.Clear();
}

// VERIFY: 0f866952 manual
// OFFSET: 0x004F18F0
int32_t CCharacterComponent::Update(const void* data, void* param) {
    int32_t hasContext = g_theGxDevicePtr ? g_theGxDevicePtr->CapsHasContext(-1) : 0;

    for (CCharacterComponent* component = CCharacterComponent::s_queue.Head(); component;) {
        CCharacterComponent* next = CCharacterComponent::s_queue.Next(component);

        if (component->m_request) {
            if (hasContext && (component->m_request->m_flags & 0x2)) {
                component->ProcessFinishedRequest();
                component->m_link.Unlink();
            }
        } else if (component->VariationsLoaded(0) & component->ItemsLoaded(0)) {
            component->m_flags |= 0x8;

            if (CCharacterComponent::s_bComponentThread) {
                component->CreateComponentRequest();
            } else if (hasContext) {
                component->RenderPrepSections();
                component->m_link.Unlink();
            }
        }

        component = next;
    }

    if (CCharacterComponent::s_bComponentThread) {
        CCharacterComponent::ProcessComponentFinishedList(0);
    }

    return 1;
}

// VERIFY: fa1ec921 manual
// OFFSET: 0x004F1A20
bool CCharacterComponent::Initialize(EGxTexFormat format, uint32_t mipLevels, int32_t useThread, int32_t useCompression) {
    s_componentHeap = new (__FILE__, __LINE__) uint32_t(ObjectAllocAddHeap(sizeof(CCharacterComponent), 32, "CCharacterComponent", true));
    s_pathEnd = s_path;
    s_pathEnd2 = s_path2;

    CCharacterComponent::m_prepFunc[SECTION_ARM_UPPER] = &CCharacterComponent::RenderPrepAU;
    CCharacterComponent::m_prepFunc[SECTION_ARM_LOWER] = &CCharacterComponent::RenderPrepAL;
    CCharacterComponent::m_prepFunc[SECTION_HAND] = &CCharacterComponent::RenderPrepHA;
    CCharacterComponent::m_prepFunc[SECTION_HEAD_UPPER] = &CCharacterComponent::RenderPrepHU;
    CCharacterComponent::m_prepFunc[SECTION_HEAD_LOWER] = &CCharacterComponent::RenderPrepHL;
    CCharacterComponent::m_prepFunc[SECTION_TORSO_UPPER] = &CCharacterComponent::RenderPrepTU;
    CCharacterComponent::m_prepFunc[SECTION_TORSO_LOWER] = &CCharacterComponent::RenderPrepTL;
    CCharacterComponent::m_prepFunc[SECTION_LEG_UPPER] = &CCharacterComponent::RenderPrepLU;
    CCharacterComponent::m_prepFunc[SECTION_LEG_LOWER] = &CCharacterComponent::RenderPrepLL;
    CCharacterComponent::m_prepFunc[SECTION_FOOT] = &CCharacterComponent::RenderPrepFO;

    CCharacterComponent::m_itemFunc[SECTION_ARM_UPPER] = &CCharacterComponent::UpdateItemAU;
    CCharacterComponent::m_itemFunc[SECTION_ARM_LOWER] = &CCharacterComponent::UpdateItemAL;
    CCharacterComponent::m_itemFunc[SECTION_HAND] = &CCharacterComponent::UpdateItemHA;
    CCharacterComponent::m_itemFunc[SECTION_HEAD_UPPER] = &CCharacterComponent::UpdateItemHU;
    CCharacterComponent::m_itemFunc[SECTION_HEAD_LOWER] = &CCharacterComponent::UpdateItemHL;
    CCharacterComponent::m_itemFunc[SECTION_TORSO_UPPER] = &CCharacterComponent::UpdateItemTU;
    CCharacterComponent::m_itemFunc[SECTION_TORSO_LOWER] = &CCharacterComponent::UpdateItemTL;
    CCharacterComponent::m_itemFunc[SECTION_LEG_UPPER] = &CCharacterComponent::UpdateItemLU;
    CCharacterComponent::m_itemFunc[SECTION_LEG_LOWER] = &CCharacterComponent::UpdateItemLL;
    CCharacterComponent::m_itemFunc[SECTION_FOOT] = &CCharacterComponent::UpdateItemFO;

    if (mipLevels > 9) {
        mipLevels = 9;
    } else if (mipLevels < 6) {
        mipLevels = 6;
    }

    if (!useCompression && mipLevels > 8) {
        mipLevels = 8;
    }

    CCharacterComponent::s_mipLevels = mipLevels;
    s_textureSize = 1 << mipLevels;

    for (int32_t section = 0; section < NUM_COMPONENT_SECTIONS; section++) {
        s_sectionInfo[section].m_pos.x = CCharacterComponent::s_sectionInfoRaw[section].m_pos.x >> (9 - mipLevels);
        s_sectionInfo[section].m_pos.y = CCharacterComponent::s_sectionInfoRaw[section].m_pos.y >> (9 - mipLevels);
        s_sectionInfo[section].m_size.x = CCharacterComponent::s_sectionInfoRaw[section].m_size.x >> (9 - mipLevels);
        s_sectionInfo[section].m_size.y = CCharacterComponent::s_sectionInfoRaw[section].m_size.y >> (9 - mipLevels);
    }

    TextureCacheResetLoadCount();

    uint32_t count = 2 * g_chrRacesDB.m_maxID + 2;
    CCharacterComponent::s_chrVarArrayLength = count;
    BuildComponentArray(count, &CCharacterComponent::s_chrVarArray);
    CountFacialFeatures(count, &CCharacterComponent::s_characterFacialHairStylesList);

    CCharacterComponent::s_bComponentThread = useThread;
    CCharacterComponent::s_bComponentCompression = 0;
    CCharacterComponent::s_gxFormat = format;

    if (useThread) {
        if (useCompression) {
            CCharacterComponent::s_bComponentCompression = 1;
            CCharacterComponent::s_gxFormat = GxTex_Dxt1;
        }

        CCharacterComponent::ThreadInitialize();
    } else {
        for (int32_t section = 0; section < NUM_COMPONENT_SECTIONS; section++) {
            CCharacterComponent::s_requestFunc[section] = nullptr;
        }
    }

    CCharacterComponent::s_gxFormatHigh = GxTex_Argb8888;
    CCharacterComponent::s_textureBuffer = nullptr;
    s_compressedTextureBuffer = nullptr;
    s_threadTextureBuffer = nullptr;
    CCharacterComponent::s_textureBuffer = TextureAllocMippedImg(PIXEL_ARGB8888, s_textureSize, s_textureSize);

    if (CCharacterComponent::s_bComponentCompression) {
        s_compressedTextureBuffer = TextureAllocMippedImg(PIXEL_DXT1, s_textureSize, s_textureSize);
    }

    if (CCharacterComponent::s_bComponentThread && CCharacterComponent::s_bComponentCompression) {
        s_threadTextureBuffer = TextureAllocMippedImg(PIXEL_ARGB8888, s_textureSize, s_textureSize);
    }

    EventRegisterEx(EVENT_ID_POLL, &CCharacterComponent::Update, nullptr, EVENT_PRIORITY_NORMAL);

    return true;
}

// VERIFY: f71b7ed9 manual
// OFFSET: 0x004F1ED0
void CCharacterComponent::ThreadDestroy() {
    CCharacterComponent::s_bComponentThread = 0;
    s_threadBeginEvent.Set();
    s_thread.Wait(0xFFFFFFFF);

    for (CComponentRequest* request = s_requestList.Head(); request;) {
        CComponentRequest* next = s_requestList.Next(request);
        request->m_flags &= ~0x1;
        s_finishedList.LinkToTail(request);
        request = next;
    }

    CCharacterComponent::ProcessComponentFinishedList(1);
    CCharacterComponent::DestroyComponentFreeLists();

    for (int32_t section = 0; section < NUM_COMPONENT_SECTIONS; section++) {
        CCharacterComponent::s_requestFunc[section] = nullptr;
    }
}

// VERIFY: e9017855 manual
// OFFSET: 0x004F1FC0
bool CCharacterComponent::SkinNpc(const char* debug) {
    this->m_data.m_flags &= ~0x1;

    if (!this->m_data.m_npcSkinTexture[0]) {
        return false;
    }

    CStatus status;
    strcpy(s_pathEnd, this->m_data.m_npcSkinTexture);
    CGxTexFlags flags(GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1);
    CTexture* texture = TextureCreate(s_path, flags, &status, 0);

    if (!texture) {
        return false;
    }

    this->m_data.m_model->ReplaceTexture(M2TEX_SKIN, texture);
    HandleClose(reinterpret_cast<HOBJECT>(texture));
    this->ReplaceExtraSkinTexture(debug);
    this->m_data.m_flags |= 0x1;
    this->m_flags |= 0x5;
    this->SetDirtySections(0);

    return true;
}

// VERIFY: 505c73ad manual
// OFFSET: 0x004F20C0
void CCharacterComponent::ReplaceMonsterSkin(CM2Model* model, const CreatureDisplayInfoRec* displayRec, const CreatureModelDataRec* modelRec) {
    if (!model || !displayRec || !modelRec) {
        return;
    }

    CStatus status;
    char path[260];
    strcpy(path, modelRec->m_modelName);

    char* fileName = strrchr(path, '\\');
    fileName = fileName ? fileName + 1 : path;

    CGxTexFlags flags(GxTex_LinearMipLinear, 1, 1, 0, 0, 0, 1);

    for (uint32_t i = 0; i < 3; i++) {
        if (*displayRec->m_textureVariation[i]) {
            strcpy(fileName, displayRec->m_textureVariation[i]);
            CTexture* texture = TextureCreate(path, flags, &status, 0);

            if (texture) {
                model->ReplaceTexture(i + M2TEX_MONSTER_1, texture);
                HandleClose(reinterpret_cast<HOBJECT>(texture));
            }
        }
    }

    ReplaceParticleColor(displayRec->m_particleColorID, model);
}

// VERIFY: c53bdd4d manual
// OFFSET: 0x004F21E0
void CCharacterComponent::AddCape() {
    if (!this->m_itemDisplayID[ITEMSLOT_CAPE]) {
        return;
    }

    ItemDisplayInfoRec rec;

    if (!g_itemDisplayInfoDB.GetRecord(this->m_itemDisplayID[ITEMSLOT_CAPE], &rec) || !*rec.m_modelTexture[0]) {
        return;
    }

    strcpy(s_buffer, "Item\\ObjectComponents\\Cape\\");
    strcat(s_buffer, rec.m_modelTexture[0]);
    strcat(s_buffer, ".blp");
    strcpy(s_pathEnd, s_buffer);

    CStatus status;
    CTexture* texture = CreateTexture(s_path, &status);

    if (texture) {
        this->m_data.m_model->ReplaceTexture(M2TEX_OBJECT_SKIN, texture);
        HandleClose(reinterpret_cast<HOBJECT>(texture));
    }
}

// VERIFY: c90ee445 manual
// OFFSET: 0x004F2320
bool CCharacterComponent::Destroy() {
    if (CCharacterComponent::m_activePlayerComponent) {
        CCharacterComponent::FreeComponent(CCharacterComponent::m_activePlayerComponent);
        CCharacterComponent::m_activePlayerComponent = nullptr;
    }

    if (CCharacterComponent::s_bComponentThread) {
        CCharacterComponent::ThreadDestroy();
    }

    if (CCharacterComponent::s_textureBuffer) {
        TextureFreeMippedImg(CCharacterComponent::s_textureBuffer, PIXEL_ARGB8888, s_textureSize, s_textureSize);
        CCharacterComponent::s_textureBuffer = nullptr;
    }

    if (s_compressedTextureBuffer) {
        TextureFreeMippedImg(s_compressedTextureBuffer, PIXEL_DXT1, s_textureSize, s_textureSize);
        s_compressedTextureBuffer = nullptr;
    }

    if (s_threadTextureBuffer) {
        TextureFreeMippedImg(s_threadTextureBuffer, PIXEL_ARGB8888, s_textureSize, s_textureSize);
        s_threadTextureBuffer = nullptr;
    }

    for (uint32_t i = 0; i < CCharacterComponent::s_chrVarArrayLength; i++) {
        for (int32_t variation = 0; variation < CHARTEXTUREVARIATIONS_NUM; variation++) {
            CharVariationList& list = CCharacterComponent::s_chrVarArray[i].m_variation[variation];

            for (int32_t j = 0; j < list.m_numVariations; j++) {
                delete[] list.m_variation[j].m_color;
            }

            delete[] list.m_variation;
        }
    }

    delete[] CCharacterComponent::s_chrVarArray;
    CCharacterComponent::s_chrVarArray = nullptr;
    CCharacterComponent::s_chrVarArrayLength = 0;
    TextureCacheDestroy();

    if (CCharacterComponent::s_characterFacialHairStylesList) {
        SMemFree(CCharacterComponent::s_characterFacialHairStylesList, __FILE__, __LINE__, 0);
    }

    delete s_componentHeap;
    s_componentHeap = nullptr;
    CGPlayer_C::s_displayId = 0;
    EventUnregisterEx(EVENT_ID_POLL, &CCharacterComponent::Update, nullptr, 0xFFFFFFFF);

    return true;
}

// VERIFY: 0acf5d15 manual
// OFFSET: 0x004F24D0
bool CCharacterComponent::Init(ComponentData* data, const char* debug) {
    if (this->m_data.m_model) {
        this->m_data.m_model->Release();
    }

    memcpy(&this->m_data, data, sizeof(this->m_data));
    memset(this->m_baseVariation, 0, sizeof(this->m_baseVariation));
    memset(this->m_section, 0, sizeof(this->m_section));
    this->m_handItemDisplayID[0] = 0;
    this->m_handItemDisplayID[1] = 0;
    this->m_handItemDisplayID[2] = 0;

    if (static_cast<int32_t>(data->m_preferences.m_raceID) < 0 || static_cast<int32_t>(data->m_preferences.m_raceID) > g_chrRacesDB.m_maxID || !ComponentValidateBase(CCharacterComponent::s_chrVarArray, this->m_data.m_preferences.m_raceID, this->m_data.m_preferences.m_sexID, CHARTEXTUREVAR_SKIN, 0, this->m_data.m_preferences.m_skinID)) {
        return false;
    }

    this->m_flags = (this->m_flags & ~0x45) | 0x5;
    this->m_dirtySections = 0xFFFFFFFF;
    this->m_request = nullptr;

    if (!(this->m_data.m_flags & 0x1) && this->m_baseSkinTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_baseSkinTexture));
        this->m_baseSkinTexture = nullptr;
    }

    uint32_t dataFlags = this->m_data.m_flags;

    if (dataFlags & 0x2) {
        this->m_gxTexFormat = CCharacterComponent::s_gxFormatHigh;
    }

    ChrRacesRec* raceRec = g_chrRacesDB.GetRecord(this->m_data.m_preferences.m_raceID);
    this->m_flags = (this->m_flags & ~0x10) | (((raceRec->m_flags & 0x2) != 0) << 4);

    if (dataFlags & 0x1) {
        this->SkinNpc(debug);
    }

    this->SetSkinColor(this->m_data.m_preferences.m_skinID, false, true, debug);
    this->SetHairStyle(this->m_data.m_preferences.m_hairStyleId, debug);
    this->SetBeardStyle(this->m_data.m_preferences.m_facialHairId, false, debug);

    return true;
}

// VERIFY: 474cb696 manual
// OFFSET: 0x004F2640
void CCharacterComponent::AddItem(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, int32_t itemVisual) {
    this->m_flags |= 0x4;
    this->m_itemDisplayID[itemSlot] = displayRec->m_id;

    switch (itemSlot) {
        case ITEMSLOT_HEAD:
            if (!this->IsHelmModelCorrect()) {
                if (this->m_itemDisplayID[ITEMSLOT_HEAD]) {
                    CCharacterComponent::RemoveLinkpt(this->m_data.m_model, ATTACH_HELM);
                }

                this->AddHelm(itemVisual);
            }

            break;

        case ITEMSLOT_SHOULDER:
            if (!this->AreShoulderModelsCorrect()) {
                if (this->m_itemDisplayID[ITEMSLOT_SHOULDER]) {
                    CCharacterComponent::RemoveLinkpt(this->m_data.m_model, static_cast<GEOCOMPONENTLINKS>(6));
                    CCharacterComponent::RemoveLinkpt(this->m_data.m_model, static_cast<GEOCOMPONENTLINKS>(5));
                }

                this->AddShoulders(itemVisual);
            }

            break;

        case ITEMSLOT_CAPE:
            this->AddCape();
            break;

        case ITEMSLOT_11:
            if (!this->IsQuiverModelCorrect()) {
                if (this->m_itemDisplayID[ITEMSLOT_11]) {
                    CCharacterComponent::RemoveLinkpt(this->m_data.m_model, ATTACH_SHEATH_MAINHAND);
                }

                this->AddQuiver(itemVisual);
            }

            break;

        default:
            if (itemSlot == ITEMSLOT_CHEST) {
                this->m_flags = (this->m_flags & ~0x20) | (32 * ((displayRec->m_flags >> 2) & 0x1));
            }

            if (this->m_data.m_flags & 0x1) {
                break;
            }

            for (int32_t section = 0; section < 8; section++) {
                if (*displayRec->m_texture[section] && s_itemPriority[itemSlot][section] != -1) {
                    (this->*CCharacterComponent::m_itemFunc[section])(itemSlot, displayRec, true);
                }
            }

            break;
    }
}

// VERIFY: 1692162c auto
// OFFSET: 0x004F2830
void CCharacterComponent::AddItem(ITEM_SLOT itemSlot, int32_t displayID, int32_t itemVisual) {
    if (displayID <= 0) {
        return;
    }

    ItemDisplayInfoRec rec;

    if (g_itemDisplayInfoDB.GetRecord(displayID, &rec)) {
        this->AddItem(itemSlot, &rec, itemVisual);
    }
}

// VERIFY: 433a7621 manual
// OFFSET: 0x004F2880
void CCharacterComponent::AddItemBySlot(INVENTORY_SLOTS invSlot, int32_t displayID, int32_t itemVisual) {
    switch (invSlot) {
        case INVSLOT_HEAD:
            this->AddItem(ITEMSLOT_HEAD, displayID, itemVisual);
            break;

        case INVSLOT_SHOULDER:
            this->AddItem(ITEMSLOT_SHOULDER, displayID, itemVisual);
            break;

        case INVSLOT_BODY:
            this->AddItem(ITEMSLOT_SHIRT, displayID, itemVisual);
            break;

        case INVSLOT_CHEST:
            this->AddItem(ITEMSLOT_CHEST, displayID, itemVisual);
            break;

        case INVSLOT_WAIST:
            this->AddItem(ITEMSLOT_BELT, displayID, itemVisual);
            break;

        case INVSLOT_LEGS:
            this->AddItem(ITEMSLOT_PANTS, displayID, itemVisual);
            break;

        case INVSLOT_FEET:
            this->AddItem(ITEMSLOT_BOOTS, displayID, itemVisual);
            break;

        case INVSLOT_WRIST:
            this->AddItem(ITEMSLOT_BRACERS, displayID, itemVisual);
            break;

        case INVSLOT_HAND:
            this->AddItem(ITEMSLOT_GLOVES, displayID, itemVisual);
            break;

        case INVSLOT_BACK:
            this->AddItem(ITEMSLOT_CAPE, displayID, itemVisual);
            break;

        case INVSLOT_TABARD:
            this->AddItem(ITEMSLOT_TABARD, displayID, itemVisual);
            break;

        default:
            break;
    }
}

// VERIFY: a17aa04e manual
// OFFSET: 0x004F29C0
void CCharacterComponent::AddItemByType(INVENTORY_TYPES invType, int32_t displayID) {
    this->RemoveItemByType(invType);

    if (displayID <= 0) {
        return;
    }

    switch (invType) {
        case INVTYPE_HEAD:
            this->AddItem(ITEMSLOT_HEAD, displayID, 0);
            break;

        case INVTYPE_SHOULDERS:
            this->AddItem(ITEMSLOT_SHOULDER, displayID, 0);
            break;

        case INVTYPE_BODY:
            this->AddItem(ITEMSLOT_SHIRT, displayID, 0);
            break;

        case INVTYPE_CHEST:
        case INVTYPE_ROBE:
            this->AddItem(ITEMSLOT_CHEST, displayID, 0);
            break;

        case INVTYPE_WAIST:
            this->AddItem(ITEMSLOT_BELT, displayID, 0);
            break;

        case INVTYPE_LEGS:
            this->AddItem(ITEMSLOT_PANTS, displayID, 0);
            break;

        case INVTYPE_FEET:
            this->AddItem(ITEMSLOT_BOOTS, displayID, 0);
            break;

        case INVTYPE_WRISTS:
            this->AddItem(ITEMSLOT_BRACERS, displayID, 0);
            break;

        case INVTYPE_HANDS:
            this->AddItem(ITEMSLOT_GLOVES, displayID, 0);
            break;

        case INVTYPE_WEAPON:
        case INVTYPE_2HWEAPON:
        case INVTYPE_WEAPONMAINHAND:
            this->SetMainHandItemDisplay(displayID);
            break;

        case INVTYPE_SHIELD:
            this->SetShieldItemDisplay(displayID);
            break;

        case INVTYPE_CLOAK:
            this->AddItem(ITEMSLOT_CAPE, displayID, 0);
            break;

        case INVTYPE_TABARD:
            this->AddItem(ITEMSLOT_TABARD, displayID, 0);
            break;

        case INVTYPE_WEAPONOFFHAND:
            this->SetOffHandItemDisplay(displayID);
            break;

        default:
            break;
    }
}
