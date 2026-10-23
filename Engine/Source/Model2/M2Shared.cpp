#include <cstdint>
#include <cstdio>
#include <cstring>
#include <malloc.h>
#include <new>
#include <typeinfo>
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/SIMD.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/ShaderEffect/ShaderEffectManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Os/OsMemory.h"

// STATIC: 0x00A45644
static const uint32_t s_skinProfileBoneCounts[4] = { 256, 64, 53, 21 };

// STATIC: 0x00AF59D8
static uint32_t s_sequenceIndex = 0xFFFFFFFF;

// STATIC: 0x00AF59E8
static const EGxTexOp s_alphaOps[8] = { static_cast<EGxTexOp>(3), static_cast<EGxTexOp>(0), static_cast<EGxTexOp>(3), static_cast<EGxTexOp>(2), static_cast<EGxTexOp>(1), static_cast<EGxTexOp>(3), static_cast<EGxTexOp>(3), static_cast<EGxTexOp>(3) };

// STATIC: 0x00AF5A08
static const EGxTexOp s_colorOps[8] = { static_cast<EGxTexOp>(0), static_cast<EGxTexOp>(0), static_cast<EGxTexOp>(4), static_cast<EGxTexOp>(2), static_cast<EGxTexOp>(1), static_cast<EGxTexOp>(5), static_cast<EGxTexOp>(1), static_cast<EGxTexOp>(2) };

// STATIC: 0x00D41258
static uint8_t* s_sequenceData;

// STATIC: 0x00D4125C
static uint32_t s_sequenceSize;

// VERIFY: eb5cb8e9 auto
// OFFSET: 0x00835950
static void AsyncFileReadFreeA16(CAsyncObject* object) {
    void* buffer = object->m_buffer;
    AsyncFileReadDestroyObject(object);
    SIMD::FreeA16(buffer);
}

#if defined(WOWDECOMP_ARCH_64)
static void AsyncFileReadFreeNear(CAsyncObject* object) {
    void* buffer = object->m_buffer;
    AsyncFileReadDestroyObject(object);
    OsFreeNear(buffer);
}
#endif

// VERIFY: d89467e7 manual
// OFFSET: 0x00835970
uint32_t CM2Shared::AddRef() {
    if (!this->m_refCount && this->m_cache) {
        if (this->m_freePrev) {
            *this->m_freePrev = this->m_freeNext;
        }

        if (this->m_freeNext) {
            this->m_freeNext->m_freePrev = this->m_freePrev;
        } else {
            this->m_cache->m_freeListTail = this->m_freePrev;
        }

        this->m_freePrev = nullptr;
        this->m_freeNext = nullptr;
        this->m_freeTime = 0;
    }

    return ++this->m_refCount;
}

// VERIFY: 306d8f75 manual
// OFFSET: 0x008359C0
int32_t CM2Shared::CallbackWhenLoaded(CM2Model* model) {
    if (!(model->m_flags & 0x20)) {
        if ((this->m_flags & 1) && (this->m_flags & 2)) {
            model->InitializeLoaded();
            return 1;
        }

        model->LinkToCallbackListTail();
    }

    return 1;
}

// VERIFY: b11b96a7 auto
// OFFSET: 0x00835A00
void CM2Shared::LoadFailedCallback(void* param) {
    CM2Shared* shared = static_cast<CM2Shared*>(param);
    AsyncFileReadDestroyObject(shared->m_asyncObject);
    shared->m_asyncObject = nullptr;
}

// VERIFY: df13c778 manual
// OFFSET: 0x00835A20
static void MakeAnimFileName(const char* fileName, uint32_t id, uint32_t variation, char* result) {
    strcpy(result, fileName);

    char* extension = strrchr(result, '.');

    if (extension) {
        *extension = '\0';
    }

    sprintf(&result[strlen(result)], "%04d-%02d.anim", id, variation);
}

// VERIFY: 90b40e33 manual
// OFFSET: 0x00835A80
static void MakeSkinFileName(const char* fileName, uint32_t profile, char* result) {
    strcpy(result, fileName);

    char* extension = strrchr(result, '.');

    if (extension) {
        *extension = '\0';
    }

    sprintf(&result[strlen(result)], "%02d.skin", profile);
}

// VERIFY: b5295fba manual
// OFFSET: [0x00835AE0, 0x00835B30, 0x00835B80, 0x00835BD0, 0x00835C20, 0x00835DF0, 0x00835E40]
template <class T>
int32_t M2Init(uint8_t* base, uint32_t size, const M2Data& data, M2Array<T>& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(T) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    return 1;
}

// VERIFY: 22da6990 manual
// OFFSET: [0x00836AD0, 0x00836C00, 0x00836F80, 0x00837010, 0x008370A0, 0x00837130, 0x008371C0]
template <class T>
int32_t M2Init(uint8_t* base, uint32_t size, const M2Data& data, M2Array<M2Array<T> >& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(M2Array<T>) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    for (uint32_t i = 0; i < array.m_count; i++) {
        if (!M2Init(base, size, data, array[i])) {
            return 0;
        }
    }

    return 1;
}

// INLINED
int32_t M2Init(uint8_t* base, uint32_t size, const M2Data& data, M2TrackBase& track) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (track.m_loopIndex == 0xFFFF) {
            track.m_sequenceTimes.Relocate(base);

            for (uint32_t i = 0; i < track.m_sequenceTimes.m_count; i++) {
                if (data.m_sequences[i].m_flags & 0x20) {
                    track.m_sequenceTimes[i].Relocate(base);
                }
            }
        } else if (!M2Init(base, size, data, track.m_sequenceTimes)) {
            return 0;
        }
    } else if (s_sequenceIndex < track.m_sequenceTimes.m_count) {
        track.m_sequenceTimes[s_sequenceIndex].Relocate(s_sequenceData);
    }

    return 1;
}

// INLINED
template <class T>
int32_t M2Init(uint8_t* base, uint32_t size, const M2Data& data, M2Track<T>& track) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (track.m_loopIndex == 0xFFFF) {
            track.m_sequenceTimes.Relocate(base);

            for (uint32_t i = 0; i < track.m_sequenceTimes.m_count; i++) {
                if (data.m_sequences[i].m_flags & 0x20) {
                    track.m_sequenceTimes[i].Relocate(base);
                }
            }
        } else if (!M2Init(base, size, data, track.m_sequenceTimes)) {
            return 0;
        }

        if (track.m_loopIndex == 0xFFFF) {
            track.m_sequenceKeys.Relocate(base);

            for (uint32_t i = 0; i < track.m_sequenceTimes.m_count; i++) {
                if (data.m_sequences[i].m_flags & 0x20) {
                    track.m_sequenceKeys[i].Relocate(base);
                }
            }
        } else if (!M2Init(base, size, data, track.m_sequenceKeys)) {
            return 0;
        }
    } else {
        if (s_sequenceIndex < track.m_sequenceTimes.m_count) {
            track.m_sequenceTimes[s_sequenceIndex].Relocate(s_sequenceData);
        }

        if (s_sequenceIndex < track.m_sequenceKeys.m_count) {
            track.m_sequenceKeys[s_sequenceIndex].Relocate(s_sequenceData);
        }
    }

    return 1;
}

// VERIFY: 89b5df34 auto
// OFFSET: 0x00835C70
template <>
int32_t M2Init<M2Sequence>(uint8_t* base, uint32_t size, const M2Data& data, M2Array<M2Sequence>& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(M2Sequence) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    for (uint32_t i = 0; i < array.m_count; i++) {
        M2Sequence* sequence = &array[i];

        if (sequence->m_flags & 1) {
            sequence->m_flags |= 0x80;
        }

        switch (sequence->m_id) {
            case 0:
            case 4:
            case 5:
            case 13:
            case 41:
            case 42:
            case 43:
            case 44:
            case 45:
            case 69:
            case 119:
            case 120:
            case 143:
            case 223:
                sequence->m_flags &= ~1u;
                break;
            default:
                break;
        }
    }

    return 1;
}

// VERIFY: eccaa05a manual
// OFFSET: 0x00835E90
void CM2Shared::ConvertTextureCombosToValues() {
    for (uint32_t i = 0; i < this->m_skinData->m_batches.m_count; i++) {
        M2Batch* batch = &this->m_skinData->m_batches[i];

        if (batch->m_textureCount == 2) {
            uint16_t* combos = &this->m_data->m_textureCombos[batch->m_textureComboIndex];
            uint16_t* transforms = &this->m_data->m_textureTransformCombos[batch->m_textureTransformComboIndex];
            uint8_t transform0 = transforms[0] == 0xFFFF ? 0 : static_cast<uint8_t>(transforms[0] + 1);
            uint8_t transform1 = transforms[1] == 0xFFFF ? 0 : static_cast<uint8_t>(transforms[1] + 1);
            batch->m_textureComboIndex = static_cast<uint16_t>((combos[0] & 0xFF) | (combos[1] << 8));
            batch->m_textureTransformComboIndex = static_cast<uint16_t>(transform0 | (transform1 << 8));
        } else {
            uint16_t combo = this->m_data->m_textureCombos[batch->m_textureComboIndex];
            uint16_t transform = this->m_data->m_textureTransformCombos[batch->m_textureTransformComboIndex];
            batch->m_textureComboIndex = combo;
            batch->m_textureTransformComboIndex = transform == 0xFFFF ? 0 : static_cast<uint16_t>(transform + 1);
        }
    }
}

// VERIFY: 77d0f7de manual
// OFFSET: 0x00835F90
static void ConvertTextureComboEntry(TextureComboList* list, int32_t adjust, uint16_t value) {
    uint16_t first = value & 0xFF;
    uint16_t second = value >> 8;

    if (adjust) {
        first = first ? first - 1 : 0xFFFF;
        second = second ? second - 1 : 0xFFFF;
    }

    uint32_t count = list->m_count;
    int32_t last = static_cast<int32_t>(count) - 1;

    for (int32_t i = 0; i < last; i++) {
        if (list->m_data[i] == first && list->m_data[i + 1] == second) {
            return;
        }
    }

    uint32_t index = count;

    for (int32_t i = 0; i < last; i += 2) {
        if (list->m_data[i] > first || (list->m_data[i] == first && list->m_data[i + 1] > second)) {
            index = i;
            break;
        }
    }

    if (count + 2 != count) {
        list->m_count = count + 2;
    }

    memmove(&list->m_data[index + 2], &list->m_data[index], sizeof(uint16_t) * (count - index));
    list->m_data[index] = first;
    list->m_data[index + 1] = second;
}

// VERIFY: 59c9e83f manual
// OFFSET: 0x008360A0
int32_t CM2Shared::SetIndices() {
    if (!this->m_indexPool) {
        this->m_indexPool = g_theGxDevicePtr->PoolCreate(GxPoolTarget_Index, GxPoolUsage_Dynamic, sizeof(uint16_t) * this->m_instances * this->m_skinData->m_indices.m_count, GxPoolHintBit_Unk1, this->m_fileNameWithoutPath);
        this->m_indexBuf = g_theGxDevicePtr->BufCreate(this->m_indexPool, sizeof(uint16_t), this->m_instances * this->m_skinData->m_indices.m_count, 0);

        if (!this->m_indexPool || !this->m_indexBuf) {
            return 0;
        }
    }

    if (!this->m_indexBuf->m_hasData || !this->m_indexBuf->m_valid) {
        uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(this->m_indexBuf));

        if (!indices) {
            return 0;
        }

        int32_t sharedVertices = (this->m_cache->m_flags & 8) || (this->m_data->m_bones.m_count == 1 && (this->m_cache->m_flags & 0x40));
        uint32_t indexStart = 0;

        for (uint32_t i = 0; i < this->m_skinData->m_skinSections.m_count; i++) {
            M2SkinSection* section = &this->m_skinSections[i];
            uint32_t start = this->m_skinData->m_skinSections[i].m_indexStart;
            uint16_t offset = sharedVertices ? 0 : static_cast<uint16_t>(-section->m_vertexStart);

            for (uint32_t instance = 0; instance < this->m_instances; instance++) {
                for (uint32_t j = 0; j < section->m_indexCount; j++) {
                    indices[j] = offset + this->m_skinData->m_indices[start + j];
                }

                indices += section->m_indexCount;
                offset += sharedVertices ? static_cast<uint16_t>(this->m_skinData->m_vertices.m_count) : section->m_vertexCount;
            }

            section->m_indexStart = static_cast<uint16_t>(indexStart);
            indexStart += this->m_instances * section->m_indexCount;
        }

        CGxBuf* buf = this->m_indexBuf;
        g_theGxDevicePtr->BufUnlock(buf, 0);
        buf->m_hasData = 1;
    }

    g_theGxDevicePtr->PrimIndexPtr(this->m_indexBuf);

    return 1;
}

// VERIFY: fe5131e0 manual
// OFFSET: 0x008362B0
int32_t CM2Shared::SetVertices(uint32_t texCoordIndex) {
    if (!this->m_vertexPool) {
        this->m_vertexPool = g_theGxDevicePtr->PoolCreate(GxPoolTarget_Vertex, GxPoolUsage_Static, sizeof(M2Vertex) * this->m_instances * this->m_skinData->m_vertices.m_count, GxPoolHintBit_Unk1, this->m_fileNameWithoutPath);
        this->m_vertexBuf = g_theGxDevicePtr->BufCreate(this->m_vertexPool, sizeof(M2Vertex), this->m_instances * this->m_skinData->m_vertices.m_count, 0);

        if (!this->m_vertexPool || !this->m_vertexBuf) {
            return 0;
        }
    }

    if (!CShaderEffect::s_enableShaders) {
        if (!this->m_vertexBuf->m_hasData || !this->m_vertexBuf->m_valid) {
            M2VertexPNT2* vertices = reinterpret_cast<M2VertexPNT2*>(g_theGxDevicePtr->BufLock(this->m_vertexBuf));

            if (!vertices) {
                return 0;
            }

            for (uint32_t i = 0; i < this->m_skinData->m_skinSections.m_count; i++) {
                M2SkinSection* section = &this->m_skinData->m_skinSections[i];
                uint32_t end = section->m_vertexStart + section->m_vertexCount;

                for (uint32_t j = section->m_vertexStart; j < end; j++) {
                    M2Vertex* source = &this->m_data->m_vertices[this->m_skinData->m_vertices[j]];
                    M2VertexPNT2* vertex = &vertices[j];
                    vertex->m_position = source->m_position;
                    vertex->m_normal = source->m_normal;
                    vertex->m_texcoord[0] = source->m_texcoord[texCoordIndex];
                    vertex->m_texcoord[1] = source->m_texcoord[texCoordIndex == 0];
                }
            }

            CGxBuf* buf = this->m_vertexBuf;
            g_theGxDevicePtr->BufUnlock(buf, 0);
            buf->m_hasData = 1;
        }

        GxPrimVertexPtr(this->m_vertexBuf, GxVBF_PNT2);

        return 1;
    }

    if (!this->m_vertexBuf->m_hasData || !this->m_vertexBuf->m_valid) {
        M2Vertex* vertices = reinterpret_cast<M2Vertex*>(g_theGxDevicePtr->BufLock(this->m_vertexBuf));

        if (!vertices) {
            return 0;
        }

        for (uint32_t instance = 0; instance < this->m_instances; instance++) {
            for (uint32_t i = 0; i < this->m_skinData->m_skinSections.m_count; i++) {
                M2SkinSection* section = &this->m_skinData->m_skinSections[i];
                uint32_t boneOffset = 0x01010101 * instance * section->m_boneCount;
                uint32_t end = section->m_vertexStart + section->m_vertexCount;

                for (uint32_t j = section->m_vertexStart; j < end; j++) {
                    memcpy(&vertices[j], &this->m_data->m_vertices[this->m_skinData->m_vertices[j]], sizeof(M2Vertex));
                    *reinterpret_cast<uint32_t*>(&vertices[j].m_indices) = boneOffset + *reinterpret_cast<uint32_t*>(&this->m_skinData->m_bones[j]);
                }
            }

            vertices += this->m_skinData->m_vertices.m_count;
        }

        CGxBuf* buf = this->m_vertexBuf;
        g_theGxDevicePtr->BufUnlock(buf, 0);
        buf->m_hasData = 1;
    }

    GxPrimVertexPtr(this->m_vertexBuf, GxVBF_PBNT2);

    return 1;
}

// VERIFY: 76031c3f manual
// OFFSET: 0x00836600
CShaderEffect* CM2Shared::GetEffect(uint32_t textureCount, uint16_t shader, uint16_t textureCoordComboIndex) {
    uint32_t ops[2];
    ops[0] = (shader >> 4) & M2COMBINER_OP_MASK;
    ops[1] = shader & M2COMBINER_OP_MASK;

    const char* vertexShader;
    const char* pixelShader;

    if (textureCount == 1) {
        if (shader & (M2COMBINER_ENVMAP << 4)) {
            vertexShader = "Diffuse_Env";
        } else {
            vertexShader = "Diffuse_T2";

            if (!this->m_data->m_textureCoordCombos[textureCoordComboIndex]) {
                vertexShader = "Diffuse_T1";
            }
        }

        switch (ops[0]) {
            case M2COMBINER_OPAQUE:
                pixelShader = "Combiners_Opaque";
                break;
            case M2COMBINER_DECAL:
                pixelShader = "Combiners_Decal";
                break;
            case M2COMBINER_ADD:
                pixelShader = "Combiners_Add";
                break;
            case M2COMBINER_MOD2X:
                pixelShader = "Combiners_Mod2x";
                break;
            case M2COMBINER_FADE:
                pixelShader = "Combiners_Fade";
                break;
            default:
                pixelShader = "Combiners_Mod";
                break;
        }
    } else {
        if (shader & (M2COMBINER_ENVMAP << 4)) {
            vertexShader = (shader & M2COMBINER_ENVMAP) ? "Diffuse_Env_Env" : "Diffuse_Env_T2";
        } else {
            vertexShader = (shader & M2COMBINER_ENVMAP) ? "Diffuse_T1_Env" : "Diffuse_T1_T2";
        }

        switch (ops[0]) {
            case M2COMBINER_OPAQUE:
                switch (ops[1]) {
                    case M2COMBINER_OPAQUE:
                        pixelShader = "Combiners_Opaque_Opaque";
                        break;
                    case M2COMBINER_ADD:
                        pixelShader = "Combiners_Opaque_Add";
                        break;
                    case M2COMBINER_MOD2X:
                        pixelShader = "Combiners_Opaque_Mod2x";
                        break;
                    case M2COMBINER_MOD2X_NA:
                        pixelShader = "Combiners_Opaque_Mod2xNA";
                        break;
                    case M2COMBINER_ADD_NA:
                        pixelShader = "Combiners_Opaque_AddNA";
                        break;
                    default:
                        pixelShader = "Combiners_Opaque_Mod";
                        break;
                }
                break;
            case M2COMBINER_MOD:
                switch (ops[1]) {
                    case M2COMBINER_OPAQUE:
                        pixelShader = "Combiners_Mod_Opaque";
                        break;
                    case M2COMBINER_ADD:
                        pixelShader = "Combiners_Mod_Add";
                        break;
                    case M2COMBINER_MOD2X:
                        pixelShader = "Combiners_Mod_Mod2x";
                        break;
                    case M2COMBINER_MOD2X_NA:
                        pixelShader = "Combiners_Mod_Mod2xNA";
                        break;
                    case M2COMBINER_ADD_NA:
                        pixelShader = "Combiners_Mod_AddNA";
                        break;
                    default:
                        pixelShader = "Combiners_Mod_Mod";
                        break;
                }
                break;
            case M2COMBINER_ADD:
                if (ops[1] != M2COMBINER_MOD) {
                    return nullptr;
                }

                pixelShader = "Combiners_Add_Mod";
                break;
            case M2COMBINER_MOD2X:
                if (ops[1] == M2COMBINER_MOD) {
                    pixelShader = "Combiners_Mod_Mod2x";
                } else if (ops[1] == M2COMBINER_MOD2X) {
                    pixelShader = "Combiners_Mod2x_Mod2x";
                } else {
                    return nullptr;
                }
                break;
            default:
                return nullptr;
        }
    }

    char effectName[256];
    strcpy(effectName, vertexShader);
    strcat(effectName, pixelShader);

    CShaderEffect* effect = CShaderEffectManager::GetEffect(effectName);

    if (!effect) {
        effect = CShaderEffectManager::CreateEffect(effectName);
        effect->InitEffect(vertexShader, pixelShader);

        EGxTexOp colorOps[2];
        EGxTexOp alphaOps[2];

        for (uint32_t i = 0; i < textureCount; i++) {
            colorOps[i] = s_colorOps[ops[i]];
            alphaOps[i] = s_alphaOps[ops[i]];
        }

        effect->InitFixedFuncPass(colorOps, alphaOps, textureCount);
    }

    return effect;
}

// VERIFY: f6943826 auto
// OFFSET: 0x008368B0
void CM2Shared::DestroyBuffers() {
    if (this->m_indexBuf) {
        CGxBuf* buf = this->m_indexBuf;

        if (buf != g_theGxDevicePtr->BufStream(buf->m_pool->m_target, 0, 0)) {
            g_theGxDevicePtr->BufDestroy(buf);
        }

        this->m_indexBuf = nullptr;
    }

    if (this->m_indexPool) {
        g_theGxDevicePtr->PoolDestroy(this->m_indexPool);
        this->m_indexPool = nullptr;
    }

    if (this->m_vertexBuf) {
        CGxBuf* buf = this->m_vertexBuf;

        if (buf != g_theGxDevicePtr->BufStream(buf->m_pool->m_target, 0, 0)) {
            g_theGxDevicePtr->BufDestroy(buf);
        }

        this->m_vertexBuf = nullptr;
    }

    if (this->m_vertexPool) {
        g_theGxDevicePtr->PoolDestroy(this->m_vertexPool);
        this->m_vertexPool = nullptr;
    }
}

// VERIFY: dc20dca6 manual
// OFFSET: 0x00836980
void CM2Shared::SubstituteSimpleShaders() {
    for (uint32_t i = 0; i < this->m_skinData->m_batches.m_count; i++) {
        M2Batch* batch = &this->m_skinData->m_batches[i];

        if (batch->m_shader & 0x8000) {
            continue;
        }

        M2Material* material = &this->m_data->m_materials[batch->m_materialIndex];

        if (!(this->m_data->m_flags & 8)) {
            uint16_t coord = this->m_data->m_textureCoordCombos[batch->m_textureCoordComboIndex];
            uint16_t op = material->m_blendMode != M2BLEND_OPAQUE;

            if (coord > 2) {
                op |= M2COMBINER_ENVMAP;
            }

            batch->m_shader = op << 4;

            if (coord == 1) {
                batch->m_shader |= 0x4000;
            }
        } else {
            uint16_t combiner = batch->m_shader;
            uint32_t ops[2] = { M2COMBINER_OPAQUE, M2COMBINER_OPAQUE };
            batch->m_shader = 0;

            for (uint32_t j = 0; j < batch->m_textureCount; j++) {
                uint32_t op = this->m_data->m_textureCombinerCombos[combiner + j];

                if (j == 0 && material->m_blendMode == M2BLEND_OPAQUE) {
                    op = M2COMBINER_OPAQUE;
                }

                uint16_t coord = this->m_data->m_textureCoordCombos[batch->m_textureCoordComboIndex + j];
                ops[j] = op;

                if (coord > 2) {
                    ops[j] = op | M2COMBINER_ENVMAP;
                }

                if (coord == 1 && j + 1 == batch->m_textureCount) {
                    batch->m_shader |= 0x4000;
                }
            }

            batch->m_shader |= ops[1] | (ops[0] << 4);
        }
    }
}

// VERIFY: f05983e3 manual
// OFFSET: 0x00836B60
template <>
int32_t M2Init<M2Texture>(uint8_t* base, uint32_t size, const M2Data& data, M2Array<M2Texture>& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(M2Texture) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    for (uint32_t i = 0; i < array.m_count; i++) {
        if (!M2Init(base, size, data, array[i].m_filename)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 74236fcb manual
// OFFSET: 0x00836C90
CShaderEffect* CM2Shared::GetEffect(M2Batch* batch) {
    CShaderEffect* effect;

    if (!(batch->m_shader & 0x8000)) {
        effect = this->GetEffect(batch->m_textureCount, batch->m_shader, batch->m_textureCoordComboIndex);
    } else {
        const char* vertexShader = nullptr;
        const char* pixelShader = nullptr;
        EGxTexOp colorOp = static_cast<EGxTexOp>(0);
        EGxTexOp alphaOp = static_cast<EGxTexOp>(0);

        switch (batch->m_shader & 0x7FFF) {
            case 0:
                return nullptr;
            case 1:
                pixelShader = "Combiners_Opaque_Mod2xNA_Alpha";
                vertexShader = "Diffuse_T1_Env";
                colorOp = static_cast<EGxTexOp>(0);
                alphaOp = static_cast<EGxTexOp>(3);
                break;
            case 2:
                pixelShader = "Combiners_Opaque_AddAlpha";
                vertexShader = "Diffuse_T1_Env";
                colorOp = static_cast<EGxTexOp>(0);
                alphaOp = static_cast<EGxTexOp>(3);
                break;
            case 3:
                pixelShader = "Combiners_Opaque_AddAlpha_Alpha";
                vertexShader = "Diffuse_T1_Env";
                colorOp = static_cast<EGxTexOp>(0);
                alphaOp = static_cast<EGxTexOp>(3);
                break;
            default:
                break;
        }

        char effectName[256];
        strcpy(effectName, vertexShader);
        strcat(effectName, pixelShader);

        effect = CShaderEffectManager::GetEffect(effectName);

        if (effect) {
            return effect;
        }

        effect = CShaderEffectManager::CreateEffect(effectName);
        effect->InitEffect(vertexShader, pixelShader);
        effect->InitFixedFuncPass(&colorOp, &alphaOp, 1);
    }

    if (!effect) {
        return this->GetEffect(batch->m_textureCount, 0x11, batch->m_textureCoordComboIndex);
    }

    return effect;
}

// VERIFY: 02384724 auto
// OFFSET: 0x00836DF0
uint32_t CM2Shared::AllocInstances(uint32_t count) {
    if (this->m_instances < count && this->m_instances < this->m_maxInstances) {
        this->m_instances = (count + 15) & ~15u;

        if (this->m_instances > this->m_maxInstances) {
            this->m_instances = this->m_maxInstances;
        }

        this->DestroyBuffers();
    }

    return this->m_instances;
}

// VERIFY: f95bb01b manual
// OFFSET: 0x00836E40
template <>
int32_t M2Init<M2Event>(uint8_t* base, uint32_t size, const M2Data& data, M2Array<M2Event>& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(M2Event) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    for (uint32_t i = 0; i < array.m_count; i++) {
        if (!M2Init(base, size, data, array[i].m_enabled)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 6ef16f8f manual
// OFFSET: 0x00837250
void CM2Shared::ConvertTextureValuesToCombos() {
    uint32_t count = this->m_skinData->m_batches.m_count;

    TextureComboList combos;
    combos.m_data = static_cast<uint16_t*>(_alloca(sizeof(uint16_t) * 2 * count));
    combos.m_count = 0;

    TextureComboList transforms;
    transforms.m_data = static_cast<uint16_t*>(_alloca(sizeof(uint16_t) * 2 * count));
    transforms.m_count = 0;

    uint32_t materialIndex = 0xFFFFFFFF;

    for (uint32_t i = 0; i < this->m_skinData->m_batches.m_count; i++) {
        M2Batch* batch = &this->m_skinData->m_batches[i];

        if (batch->m_materialIndex != materialIndex) {
            materialIndex = batch->m_materialIndex;

            if (batch->m_textureCount >= 2) {
                ConvertTextureComboEntry(&combos, 0, batch->m_textureComboIndex);
                ConvertTextureComboEntry(&transforms, 1, batch->m_textureTransformComboIndex);
            }
        }
    }

    materialIndex = 0xFFFFFFFF;

    for (uint32_t i = 0; i < this->m_skinData->m_batches.m_count; i++) {
        M2Batch* batch = &this->m_skinData->m_batches[i];

        if (batch->m_materialIndex == materialIndex) {
            continue;
        }

        materialIndex = batch->m_materialIndex;

        if (batch->m_textureCount > 1) {
            continue;
        }

        uint16_t* combo = combos.m_data;
        uint16_t* combosEnd = combos.m_data + combos.m_count;

        while (combo != combosEnd && *combo != batch->m_textureComboIndex) {
            combo++;
        }

        if (combo == combosEnd) {
            new (&combos.m_data[combos.m_count++]) uint16_t(batch->m_textureComboIndex);
        }

        uint16_t transform = batch->m_textureTransformComboIndex ? batch->m_textureTransformComboIndex - 1 : 0xFFFF;
        uint16_t* entry = transforms.m_data;
        uint16_t* transformsEnd = transforms.m_data + transforms.m_count;

        while (entry != transformsEnd && *entry != transform) {
            entry++;
        }

        if (entry == transformsEnd) {
            new (&transforms.m_data[transforms.m_count++]) uint16_t(transform);
        }
    }

#if defined(WOWDECOMP_ARCH_64)
    uint32_t comboBytes = combos.m_count > this->m_data->m_textureCombos.m_count ? sizeof(uint16_t) * combos.m_count : 0;
    uint32_t transformBytes = transforms.m_count > this->m_data->m_textureTransformCombos.m_count ? sizeof(uint16_t) * transforms.m_count : 0;

    if (comboBytes || transformBytes) {
        uint32_t size = (this->m_fileSize + 15) & ~15u;
        M2Data* data = static_cast<M2Data*>(SIMD::AllocA16(size + comboBytes + transformBytes, __FILE__, __LINE__));
        memcpy(data, this->m_data, this->m_fileSize);
        SIMD::FreeA16(this->m_data);
        this->m_data = data;
        this->m_fileSize = size + comboBytes + transformBytes;
        uint8_t* tail = reinterpret_cast<uint8_t*>(data) + size;

        if (comboBytes) {
            data->m_textureCombos.Set(reinterpret_cast<uint16_t*>(tail));
            tail += comboBytes;
        }

        if (transformBytes) {
            data->m_textureTransformCombos.Set(reinterpret_cast<uint16_t*>(tail));
        }
    }
#else
    if (combos.m_count > this->m_data->m_textureCombos.m_count) {
        this->m_data->m_textureCombos.Set(static_cast<uint16_t*>(SMemAlloc(sizeof(uint16_t) * combos.m_count, __FILE__, __LINE__, 0)));
        this->m_data->m_flags |= 0x20;
    }

    if (transforms.m_count > this->m_data->m_textureTransformCombos.m_count) {
        this->m_data->m_textureTransformCombos.Set(static_cast<uint16_t*>(SMemAlloc(sizeof(uint16_t) * transforms.m_count, __FILE__, __LINE__, 0)));
        this->m_data->m_flags |= 0x40;
    }
#endif

    memcpy(this->m_data->m_textureCombos.Data(), combos.m_data, sizeof(uint16_t) * combos.m_count);
    this->m_data->m_textureCombos.m_count = combos.m_count;

    memcpy(this->m_data->m_textureTransformCombos.Data(), transforms.m_data, sizeof(uint16_t) * transforms.m_count);
    this->m_data->m_textureTransformCombos.m_count = transforms.m_count;
}

// VERIFY: 059df86f manual
// OFFSET: 0x008374A0
void CM2Shared::AssignBatchTextureComboIndices() {
    uint32_t materialIndex = 0xFFFFFFFF;

    for (uint32_t i = 0; i < this->m_skinData->m_batches.m_count; i++) {
        M2Batch* batch = &this->m_skinData->m_batches[i];

        if (batch->m_materialIndex == materialIndex) {
            continue;
        }

        materialIndex = batch->m_materialIndex;

        uint16_t transformIndex;

        if (batch->m_textureCount == 2) {
            M2Array<uint16_t>& combos = this->m_data->m_textureCombos;
            uint16_t first = batch->m_textureComboIndex & 0xFF;
            uint16_t second = batch->m_textureComboIndex >> 8;
            uint16_t comboIndex = 0;
            uint32_t j;

            for (j = 0; j < combos.m_count - 1; j++) {
                if (combos[j] == first && combos[j + 1] == second) {
                    break;
                }
            }

            if (j < combos.m_count - 1) {
                comboIndex = static_cast<uint16_t>(j);
            } else {
                DebugLog("Failed to fix up texture combo!\n");
            }

            batch->m_textureComboIndex = comboIndex;

            M2Array<uint16_t>& transforms = this->m_data->m_textureTransformCombos;
            uint16_t value = batch->m_textureTransformComboIndex;
            first = (value & 0xFF) ? (value & 0xFF) - 1 : 0xFFFF;
            second = (value >> 8) ? (value >> 8) - 1 : 0xFFFF;
            transformIndex = 0;

            for (j = 0; j < transforms.m_count - 1; j++) {
                if (transforms[j] == first && transforms[j + 1] == second) {
                    break;
                }
            }

            if (j < transforms.m_count - 1) {
                transformIndex = static_cast<uint16_t>(j);
            } else {
                DebugLog("Failed to fix up texture combo!\n");
            }
        } else {
            uint16_t* combo = this->m_data->m_textureCombos.Data();
            uint16_t* combosEnd = combo + this->m_data->m_textureCombos.m_count;

            while (combo != combosEnd && *combo != batch->m_textureComboIndex) {
                combo++;
            }

            batch->m_textureComboIndex = static_cast<uint16_t>(combo - this->m_data->m_textureCombos.Data());

            uint16_t transform = batch->m_textureTransformComboIndex ? batch->m_textureTransformComboIndex - 1 : 0xFFFF;
            uint16_t* entry = this->m_data->m_textureTransformCombos.Data();
            uint16_t* transformsEnd = entry + this->m_data->m_textureTransformCombos.m_count;

            while (entry != transformsEnd && *entry != transform) {
                entry++;
            }

            transformIndex = static_cast<uint16_t>(entry - this->m_data->m_textureTransformCombos.Data());
        }

        batch->m_textureTransformComboIndex = transformIndex;
    }
}

// VERIFY: 5e824557 manual
// OFFSET: 0x00837680
void CM2Shared::SubstituteSpecializedShaders() {
    M2SkinProfile* skinData = this->m_skinData;

    if (!skinData->m_indices.m_count) {
        return;
    }

    uint32_t layered;

    for (layered = 0; layered < skinData->m_batches.m_count; layered++) {
        if (skinData->m_batches[layered].m_materialLayer) {
            break;
        }
    }

    if (layered >= skinData->m_batches.m_count) {
        return;
    }

    int8_t state0 = 0;
    int8_t state1 = 0;

    this->ConvertTextureCombosToValues();

    M2Batch* base = nullptr;
    uint32_t materialIndex = 0xFFFFFFFF;
    bool sharedMaterials = false;
    bool combined = false;

    for (uint32_t i = 0; i < this->m_skinData->m_batches.m_count; i++) {
        M2Batch* batch = &this->m_skinData->m_batches[i];

        if (batch->m_materialIndex == materialIndex) {
            sharedMaterials = true;
            continue;
        }

        uint32_t op = batch->m_shader & M2COMBINER_OP_MASK;
        M2Material* material = &this->m_data->m_materials[batch->m_materialIndex];
        materialIndex = batch->m_materialIndex;

        if (!batch->m_materialLayer) {
            state0 = 0;
            state1 = 0;

            if (batch->m_textureCount && material->m_blendMode == M2BLEND_OPAQUE) {
                batch->m_shader &= 0xFF8F;
            }

            base = &this->m_skinData->m_batches[i];
        }

        switch (state0) {
            case 1:
                if ((material->m_blendMode == M2BLEND_ALPHA || material->m_blendMode == M2BLEND_ALPHA_KEY) && batch->m_textureCount == 1 && !((material->m_flags ^ this->m_data->m_materials[base->m_materialIndex].m_flags) & 1) && batch->m_textureComboIndex == (base->m_textureComboIndex & 0xFF) && this->m_data->m_textureWeightCombos[base->m_textureWeightComboIndex] == this->m_data->m_textureWeightCombos[batch->m_textureWeightComboIndex]) {
                    batch->m_shader = 0x8000;
                    base->m_shader = 0x8001;
                    state0 = 3;
                    continue;
                }

                state0 = 0;
                [[fallthrough]];
            case 0:
                if (material->m_blendMode == M2BLEND_OPAQUE && batch->m_textureCount == 2 && (op == M2COMBINER_MOD2X || op == M2COMBINER_MOD2X_NA)) {
                    uint16_t* coords = &this->m_data->m_textureCoordCombos[batch->m_textureCoordComboIndex];

                    if (!coords[0] && coords[1] > 2) {
                        state0 = 1;
                    }
                }
                break;
            default:
                break;
        }

        switch (state1) {
            case 0:
                break;
            case 1:
                if ((material->m_blendMode == M2BLEND_ADD || material->m_blendMode == M2BLEND_MOD2X) && batch->m_textureCount == 1 && this->m_data->m_textureCoordCombos[batch->m_textureCoordComboIndex] > 2 && this->m_data->m_textureWeightCombos[base->m_textureWeightComboIndex] == this->m_data->m_textureWeightCombos[batch->m_textureWeightComboIndex]) {
                    batch->m_shader = 0x8000;
                    combined = true;
                    base->m_shader = material->m_blendMode != M2BLEND_ADD ? 0xE : 0x8002;
                    base->m_textureCount = 2;
                    state1 = 2;
                    base->m_textureComboIndex = static_cast<uint16_t>((base->m_textureComboIndex & 0xFF) | (batch->m_textureComboIndex << 8));
                    base->m_textureTransformComboIndex = static_cast<uint16_t>((base->m_textureTransformComboIndex & 0xFF) | (batch->m_textureTransformComboIndex << 8));
                    continue;
                }

                state1 = 0;
                break;
            case 2:
                if ((material->m_blendMode == M2BLEND_ALPHA || material->m_blendMode == M2BLEND_ALPHA_KEY) && batch->m_textureCount == 1 && !((material->m_flags ^ this->m_data->m_materials[base->m_materialIndex].m_flags) & 1) && (base->m_textureComboIndex & 0xFF) == (batch->m_textureComboIndex & 0xFF) && this->m_data->m_textureWeightCombos[base->m_textureWeightComboIndex] == this->m_data->m_textureWeightCombos[batch->m_textureWeightComboIndex]) {
                    batch->m_shader = 0x8000;
                    state1 = 3;
                    base->m_shader = base->m_shader == 0x8002 ? 0x8003 : 0x8001;
                    continue;
                }

                state1 = 0;
                break;
            default:
                continue;
        }

        if (material->m_blendMode == M2BLEND_OPAQUE && batch->m_textureCount == 1 && !this->m_data->m_textureCoordCombos[batch->m_textureCoordComboIndex]) {
            state1 = 1;
        }
    }

    if (combined) {
        this->ConvertTextureValuesToCombos();
    }

    this->AssignBatchTextureComboIndices();

    if (sharedMaterials) {
        materialIndex = 0xFFFFFFFF;

        for (uint32_t i = 0; i < this->m_skinData->m_batches.m_count; i++) {
            M2Batch* batch = &this->m_skinData->m_batches[i];

            if (batch->m_materialIndex == materialIndex) {
                batch->m_shader = batch[-1].m_shader;
                batch->m_textureCount = batch[-1].m_textureCount;
                batch->m_textureComboIndex = batch[-1].m_textureComboIndex;
                batch->m_textureTransformComboIndex = batch[-1].m_textureTransformComboIndex;
            } else {
                materialIndex = batch->m_materialIndex;
            }
        }
    }
}

// VERIFY: 6736c39f manual
// OFFSET: 0x00837A40
int32_t CM2Shared::InitializeSkinProfile() {
    M2SkinProfile* skinData = this->m_skinData;

    this->m_maxInstances = skinData->m_indices.m_count ? 0x10000 / skinData->m_indices.m_count : 1;

    for (uint32_t i = 0; i < skinData->m_skinSections.m_count; i++) {
        uint32_t instances = skinData->m_boneCountMax / skinData->m_skinSections[i].m_boneCount;

        if (this->m_maxInstances > instances) {
            this->m_maxInstances = instances;
        }
    }

    if (!this->m_maxInstances) {
        this->m_maxInstances = 1;
    }

    this->m_instances = 1;

    uint32_t size = sizeof(M2SkinSection) * skinData->m_skinSections.m_count;

    if (skinData->m_indices.m_count) {
        size += sizeof(CShaderEffect*) * skinData->m_batches.m_count;
    }

    uint8_t* memory = static_cast<uint8_t*>(SMemAlloc(size, __FILE__, __LINE__, 0));

    if (!memory) {
        return 0;
    }

    this->m_skinSections = reinterpret_cast<M2SkinSection*>(memory);

    size_t sectionsSize = 0;

    if (this->m_skinData->m_skinSections.m_count) {
        sectionsSize = sizeof(M2SkinSection) * this->m_skinData->m_skinSections.m_count;
        memcpy(memory, this->m_skinData->m_skinSections.Data(), sectionsSize);
    }

    if (this->m_skinData->m_indices.m_count) {
        this->m_effects = reinterpret_cast<CShaderEffect**>(&memory[sectionsSize]);
        memset(this->m_effects, 0, sizeof(CShaderEffect*) * this->m_skinData->m_batches.m_count);
    }

    this->SubstituteSimpleShaders();
    this->SubstituteSpecializedShaders();

    if (this->m_skinData->m_indices.m_count) {
        for (uint32_t i = 0; i < this->m_skinData->m_batches.m_count; i++) {
            this->m_effects[i] = this->GetEffect(&this->m_skinData->m_batches[i]);
        }
    }

    if (!(this->m_cache->m_flags & 8)) {
        uint32_t count = this->m_skinData->m_vertices.m_count;
        M2Vertex* vertices = new (__FILE__, __LINE__) M2Vertex[count];

        for (uint32_t i = 0; i < this->m_skinData->m_skinSections.m_count; i++) {
            M2SkinSection* section = &this->m_skinData->m_skinSections[i];
            uint32_t end = section->m_vertexStart + section->m_vertexCount;

            for (uint32_t j = section->m_vertexStart; j < end; j++) {
                vertices[j] = this->m_data->m_vertices[this->m_skinData->m_vertices[j]];

                for (uint32_t k = 0; k < section->m_boneInfluences; k++) {
                    vertices[j].m_indices.b[k] = static_cast<uint8_t>(this->m_data->m_boneCombos[section->m_boneComboIndex + this->m_skinData->m_bones[j].b[k]]);
                }
            }
        }

        for (uint32_t i = 0; i < this->m_skinData->m_skinSections.m_count; i++) {
            this->m_skinSections[i].m_boneComboIndex = 0;
            this->m_skinData->m_skinSections[i].m_boneComboIndex = 0;
        }

        for (uint32_t i = 0; i < this->m_data->m_boneCombos.m_count; i++) {
            this->m_data->m_boneCombos[i] = static_cast<uint16_t>(i);
        }

        for (uint32_t i = 0; i < this->m_skinData->m_vertices.m_count; i++) {
            this->m_skinData->m_vertices[i] = static_cast<uint16_t>(i);
        }

        if (this->m_skinData->m_vertices.m_count <= this->m_data->m_vertices.m_count) {
            memcpy(this->m_data->m_vertices.Data(), vertices, sizeof(M2Vertex) * this->m_skinData->m_vertices.m_count);
            delete[] vertices;
        } else {
            this->m_flags |= 8;
#if defined(WOWDECOMP_ARCH_64)
            uint32_t bytes = sizeof(M2Vertex) * this->m_skinData->m_vertices.m_count;
            M2Vertex* buffer = static_cast<M2Vertex*>(OsAllocNear(this->m_data, bytes));
            memcpy(buffer, vertices, bytes);
            delete[] vertices;
            this->m_data->m_vertices.Set(buffer);
#else
            this->m_data->m_vertices.Set(vertices);
#endif
        }

        this->m_data->m_vertices.m_count = this->m_skinData->m_vertices.m_count;
    }

    if (!(this->m_cache->m_flags & 8)) {
        for (uint32_t i = 0; i < this->m_skinData->m_batches.m_count; i++) {
            M2Batch* batches = this->m_skinData->m_batches.Data();

            if (batches[i].m_textureCount > 1) {
                batches[i - batches[i].m_materialLayer].m_flags |= 0x40;
            }
        }

        for (uint32_t i = 0; i < this->m_skinData->m_batches.m_count; i++) {
            M2Batch* batches = this->m_skinData->m_batches.Data();
            uint16_t layer = batches[i].m_materialLayer;

            if (layer && (batches[i - layer].m_flags & 0x40)) {
                batches[i].m_flags |= 0x40;
            }
        }
    }

    return 1;
}

// VERIFY: 07cdd50c manual
// OFFSET: 0x00837EE0
template <>
int32_t M2Init<M2Color>(uint8_t* base, uint32_t size, const M2Data& data, M2Array<M2Color>& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(M2Color) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    for (uint32_t i = 0; i < array.m_count; i++) {
        M2Color* color = &array[i];

        if (!M2Init(base, size, data, color->m_color) || !M2Init(base, size, data, color->m_alpha)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: b51b71bf manual
// OFFSET: 0x008382A0
template <>
int32_t M2Init<M2TextureWeight>(uint8_t* base, uint32_t size, const M2Data& data, M2Array<M2TextureWeight>& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(M2TextureWeight) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    for (uint32_t i = 0; i < array.m_count; i++) {
        if (!M2Init(base, size, data, array[i].m_weight)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 2f176c2e auto
// OFFSET: 0x00838490
int32_t CM2Shared::FinishLoadingSkinProfile(uint32_t size) {
    if (this->m_flags & 2) {
        return 1;
    }

    M2Data* data = this->m_data;
    M2SkinProfile* skinData = this->m_skinData;
    uint8_t* base = reinterpret_cast<uint8_t*>(skinData);

    if (!M2Init(base, size, *data, skinData->m_vertices) || !M2Init(base, size, *data, skinData->m_indices) || !M2Init(base, size, *data, skinData->m_bones) || !M2Init(base, size, *data, skinData->m_skinSections) || !M2Init(base, size, *data, skinData->m_batches)) {
        DebugLog("Corrupt skin profile data: %s", this->m_fileName);
        return 0;
    }

    if (!this->InitializeSkinProfile()) {
        DebugLog("Failed to initialize model skin profile: %s", this->m_fileName);
        return 0;
    }

    this->m_flags |= 2;

    while (this->m_callbackList) {
        CM2Model* model = this->m_callbackList;
        model->UnlinkFromCallbackList();
        model->InitializeLoaded();
    }

    return 1;
}

// VERIFY: 48995230 manual
// OFFSET: 0x008385A0
template <>
int32_t M2Init<M2CompBone>(uint8_t* base, uint32_t size, const M2Data& data, M2Array<M2CompBone>& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(M2CompBone) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    for (uint32_t i = 0; i < array.m_count; i++) {
        M2CompBone* bone = &array[i];

        if (!M2Init(base, size, data, bone->m_translationTrack) || !M2Init(base, size, data, bone->m_rotationTrack) || !M2Init(base, size, data, bone->m_scaleTrack)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 37b60e50 manual
// OFFSET: 0x00838B10
template <>
int32_t M2Init<M2TextureTransform>(uint8_t* base, uint32_t size, const M2Data& data, M2Array<M2TextureTransform>& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(M2TextureTransform) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    for (uint32_t i = 0; i < array.m_count; i++) {
        M2TextureTransform* transform = &array[i];

        if (!M2Init(base, size, data, transform->m_translation) || !M2Init(base, size, data, transform->m_rotation) || !M2Init(base, size, data, transform->m_scaling)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: b489ace2 manual
// OFFSET: 0x00839080
template <>
int32_t M2Init<M2Attachment>(uint8_t* base, uint32_t size, const M2Data& data, M2Array<M2Attachment>& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(M2Attachment) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    for (uint32_t i = 0; i < array.m_count; i++) {
        if (!M2Init(base, size, data, array[i].m_animateAttached)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 038dfeab manual
// OFFSET: 0x00839270
template <>
int32_t M2Init<M2Light>(uint8_t* base, uint32_t size, const M2Data& data, M2Array<M2Light>& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(M2Light) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    for (uint32_t i = 0; i < array.m_count; i++) {
        M2Light* light = &array[i];

        if (!M2Init(base, size, data, light->m_ambientColor) || !M2Init(base, size, data, light->m_ambientIntensity) || !M2Init(base, size, data, light->m_diffuseColor) || !M2Init(base, size, data, light->m_diffuseIntensity) || !M2Init(base, size, data, light->m_attenuationStart) || !M2Init(base, size, data, light->m_attenuationEnd) || !M2Init(base, size, data, light->m_visibility)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: f7f844ee manual
// OFFSET: 0x00839EF0
template <>
int32_t M2Init<M2Camera>(uint8_t* base, uint32_t size, const M2Data& data, M2Array<M2Camera>& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(M2Camera) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    for (uint32_t i = 0; i < array.m_count; i++) {
        M2Camera* camera = &array[i];

        if (!M2Init(base, size, data, camera->m_positions) || !M2Init(base, size, data, camera->m_targetPositions) || !M2Init(base, size, data, camera->m_roll)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 6bc5fca7 manual
// OFFSET: 0x0083A460
template <>
int32_t M2Init<M2Ribbon>(uint8_t* base, uint32_t size, const M2Data& data, M2Array<M2Ribbon>& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(M2Ribbon) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    for (uint32_t i = 0; i < array.m_count; i++) {
        M2Ribbon* ribbon = &array[i];

        if (!M2Init(base, size, data, ribbon->m_textureIndices) || !M2Init(base, size, data, ribbon->m_materialIndices) || !M2Init(base, size, data, ribbon->m_colorTrack) || !M2Init(base, size, data, ribbon->m_alphaTrack) || !M2Init(base, size, data, ribbon->m_heightAboveTrack) || !M2Init(base, size, data, ribbon->m_heightBelowTrack) || !M2Init(base, size, data, ribbon->m_textureSlotTrack) || !M2Init(base, size, data, ribbon->m_visibilityTrack)) {
            return 0;
        }
    }

    return 1;
}

// INLINED
int32_t M2Init(uint8_t* base, uint32_t size, const M2Data& data, M2Particle& particle) {
    return M2Init(base, size, data, particle.m_geometryMdl) && M2Init(base, size, data, particle.m_recursionMdl) && M2Init(base, size, data, particle.m_speedTrack) && M2Init(base, size, data, particle.m_variationTrack) && M2Init(base, size, data, particle.m_latitudeTrack) && M2Init(base, size, data, particle.m_longitudeTrack) && M2Init(base, size, data, particle.m_gravityTrack) && M2Init(base, size, data, particle.m_lifeTrack) && M2Init(base, size, data, particle.m_emissionRateTrack) && M2Init(base, size, data, particle.m_widthTrack) && M2Init(base, size, data, particle.m_lengthTrack) && M2Init(base, size, data, particle.m_zsourceTrack) && M2Init(base, size, data, particle.m_colorTrack.m_times) && M2Init(base, size, data, particle.m_colorTrack.m_values) && M2Init(base, size, data, particle.m_alphaTrack.m_times) && M2Init(base, size, data, particle.m_alphaTrack.m_values) && M2Init(base, size, data, particle.m_scaleTrack.m_times) && M2Init(base, size, data, particle.m_scaleTrack.m_values) && M2Init(base, size, data, particle.m_headCellTrack.m_times) && M2Init(base, size, data, particle.m_headCellTrack.m_values) && M2Init(base, size, data, particle.m_tailCellTrack.m_times) && M2Init(base, size, data, particle.m_tailCellTrack.m_values) && M2Init(base, size, data, particle.m_spline) && M2Init(base, size, data, particle.m_visibilityTrack);
}

// VERIFY: 628c56d4 manual
// OFFSET: 0x0083AF90
template <>
int32_t M2Init<M2Particle>(uint8_t* base, uint32_t size, const M2Data& data, M2Array<M2Particle>& array) {
    if (s_sequenceIndex == 0xFFFFFFFF) {
        if (array.m_offset > size || array.m_offset + sizeof(M2Particle) * array.m_count > size) {
            return 0;
        }

        array.Relocate(base);
    }

    for (uint32_t i = 0; i < array.m_count; i++) {
        if (!M2Init(base, size, data, array[i])) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 95d98bd6 manual
// OFFSET: 0x0083C5F0
CM2Shared::CM2Shared(CM2Cache* cache) {
    this->m_flags &= ~0x7Fu;
    this->m_cache = cache;
    this->m_callbackList = nullptr;
    this->m_refCount = 1;
    this->m_asyncObject = nullptr;
    this->m_callbackListTail = &this->m_callbackList;
    this->m_lowPrioritySequenceCapacity = 0;
    this->m_lowPrioritySequenceBuffers = nullptr;
    this->m_lowPrioritySequenceCount = 0;
    this->m_freePrev = nullptr;
    this->m_freeNext = nullptr;
    this->m_freeTime = 0;
    this->m_fileNameWithoutPath = nullptr;
    this->m_previous = nullptr;
    this->m_next = nullptr;
    this->m_fileNameHash = 0;
    this->m_data = nullptr;
    this->m_fileSize = 0;
    this->m_skinData = nullptr;
    this->m_textures = nullptr;
    this->m_indexPool = nullptr;
    this->m_indexBuf = nullptr;
    this->m_vertexPool = nullptr;
    this->m_vertexBuf = nullptr;
    this->m_effects = nullptr;
    this->m_skinSections = nullptr;
    this->m_instances = 0;
    this->m_maxInstances = 0;
    this->m_billboardBoneCount = 0;
    this->m_lastUsedTime = 0;
    this->m_fileName[0] = '\0';
}

// INLINED
int32_t M2Init(uint8_t* base, uint32_t size, M2Data& data) {
    if (reinterpret_cast<uint8_t*>(&data) < base || reinterpret_cast<uint8_t*>(&data + 1) > base + size) {
        return 0;
    }

    if (data.m_magic != 0x3032444D) {
        return 0;
    }

    if (data.m_version < 0x108 || data.m_version > 0x108) {
        return 0;
    }

    return M2Init(base, size, data, data.m_name) && M2Init(base, size, data, data.m_loops) && M2Init(base, size, data, data.m_sequences) && M2Init(base, size, data, data.m_sequenceIdxHashById) && M2Init(base, size, data, data.m_bones) && M2Init(base, size, data, data.m_boneIndicesById) && M2Init(base, size, data, data.m_vertices) && M2Init(base, size, data, data.m_colors) && M2Init(base, size, data, data.m_textures) && M2Init(base, size, data, data.m_textureWeights) && M2Init(base, size, data, data.m_textureTransforms) && M2Init(base, size, data, data.m_textureIndicesById) && M2Init(base, size, data, data.m_materials) && M2Init(base, size, data, data.m_boneCombos) && M2Init(base, size, data, data.m_textureCombos) && M2Init(base, size, data, data.m_textureCoordCombos) && M2Init(base, size, data, data.m_textureWeightCombos) && M2Init(base, size, data, data.m_textureTransformCombos) && M2Init(base, size, data, data.m_collisionIndices) && M2Init(base, size, data, data.m_collisionPositions) && M2Init(base, size, data, data.m_collisionFaceNormals) && M2Init(base, size, data, data.m_attachments) && M2Init(base, size, data, data.m_attachmentIndicesById) && M2Init(base, size, data, data.m_events) && M2Init(base, size, data, data.m_lights) && M2Init(base, size, data, data.m_cameras) && M2Init(base, size, data, data.m_cameraIndicesById) && M2Init(base, size, data, data.m_ribbons) && M2Init(base, size, data, data.m_particles) && (!(data.m_flags & 8) || M2Init(base, size, data, data.m_textureCombinerCombos));
}

// VERIFY: 8000b812 manual
// OFFSET: 0x0083C6E0
int32_t CM2Shared::FinishLoadingLowPrioritySequence(uint32_t sequenceIndex, uint8_t* buffer, uint32_t size) {
    s_sequenceIndex = sequenceIndex;
    s_sequenceData = buffer;

    int32_t result = M2Init(reinterpret_cast<uint8_t*>(this->m_data), this->m_fileSize, *this->m_data);
    M2Sequence* sequence = &this->m_data->m_sequences[sequenceIndex];

    s_sequenceIndex = 0xFFFFFFFF;
    s_sequenceData = nullptr;
    s_sequenceSize = 0;

    if (!result) {
        DebugLog("Corrupt model sequence data: %s (%d)", this->m_fileName, sequenceIndex);
        return 0;
    }

    sequence->m_flags = (sequence->m_flags & ~0x30u) | 0x20;

    return 1;
}

// VERIFY: b8d661f9 manual
// OFFSET: 0x0083CA90
int32_t CM2Shared::FinishLoadingLowPrioritySequence(uint32_t sequenceIndex, CAsyncObject* object) {
    M2Sequence* sequence = &this->m_data->m_sequences[sequenceIndex];

    if (!this->FinishLoadingLowPrioritySequence(sequenceIndex, static_cast<uint8_t*>(object->m_buffer), object->m_size)) {
        return 0;
    }

    for (uint16_t* alias = &sequence->m_aliasNext; *alias != sequenceIndex; alias = &this->m_data->m_sequences[*alias].m_aliasNext) {
        if (!this->FinishLoadingLowPrioritySequence(*alias, static_cast<uint8_t*>(object->m_buffer), object->m_size)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 803bac57 auto
// OFFSET: 0x0083CB10
void CM2Shared::SkinProfileLoadedCallback(void* param) {
    CM2Shared* shared = static_cast<CM2Shared*>(param);
    shared->FinishLoadingSkinProfile(shared->m_asyncObject->m_size);
    AsyncFileReadDestroyObject(shared->m_asyncObject);
    shared->m_asyncObject = nullptr;
}

// VERIFY: 2fc725dd manual
// OFFSET: 0x0083CB40
int32_t CM2Shared::LoadSkinProfile(uint32_t profile) {
    char fileName[260];
    MakeSkinFileName(this->m_fileName, profile, fileName);

    SFileImpl* file;

    if (!SFile::OpenEx(nullptr, fileName, (this->m_flags & 4) >> 2, &file)) {
        DebugLog("Model2: File not found: %s\n", fileName);
        return 0;
    }

    uint32_t size = SFileGetFileSize(file, nullptr);
    this->m_skinData = static_cast<M2SkinProfile*>(SIMD::AllocA16(size, __FILE__, __LINE__));

    if (!this->m_skinData) {
        SFileCloseFile(file);
        return 0;
    }

    this->m_asyncObject = AsyncFileReadAllocObject();

    if (!this->m_asyncObject) {
        SFileCloseFile(file);
        SIMD::FreeA16(this->m_skinData);
        this->m_skinData = nullptr;
        return 0;
    }

    this->m_asyncObject->m_file = file;
    this->m_asyncObject->m_buffer = this->m_skinData;
    this->m_asyncObject->m_size = size;
    this->m_asyncObject->m_userArg = this;
    this->m_asyncObject->m_userPostloadCallback = reinterpret_cast<void*>(&CM2Shared::SkinProfileLoadedCallback);
    this->m_asyncObject->m_userPostUnloadCallback = reinterpret_cast<void*>(&CM2Shared::LoadFailedCallback);
    this->m_asyncObject->m_isRead = 0;
    this->m_asyncObject->m_isProcessed = 0;
    this->m_asyncObject->m_priority = 125;
    AsyncFileReadObject(this->m_asyncObject, 1);

    return 1;
}

// INLINED
uint32_t CM2Shared::GetSkinProfileIndex(uint32_t maxBones) {
    for (uint32_t i = 0; i < this->m_data->m_numSkinProfiles; i++) {
        if (s_skinProfileBoneCounts[4 - this->m_data->m_numSkinProfiles + i] <= maxBones) {
            return i;
        }
    }

    return 0xFFFFFFFF;
}

// VERIFY: 38f10509 manual
// OFFSET: 0x0083CC80
int32_t CM2Shared::Initialize() {
    uint32_t maxBones = 256;

    if (this->m_cache->m_flags & 8) {
        uint32_t constants = g_theGxDevicePtr->Caps().m_shaderConstants[0];
        uint32_t bones = (constants > 0xC0 ? constants - 64 : constants - 32) / 3;

        if (bones < maxBones) {
            maxBones = bones;
        }
    }

    this->m_skinData = nullptr;

    uint32_t profile = this->GetSkinProfileIndex(maxBones);

    if (profile == 0xFFFFFFFF) {
        DebugLog("Failed to choose skin profile: %s", this->m_fileName);
        return 0;
    }

    if (!this->LoadSkinProfile(profile)) {
        DebugLog("Failed to load skin profile: %s", this->m_fileName);
        return 0;
    }

    this->m_textures = static_cast<CTexture**>(SMemAlloc(sizeof(CTexture*) * this->m_data->m_textures.m_count, __FILE__, __LINE__, 8));

    if (!this->m_textures) {
        DebugLog("Failed to allocate texture array: %s", this->m_fileName);
        return 0;
    }

    for (uint32_t i = 0; i < this->m_data->m_materials.m_count; i++) {
        M2Material* material = &this->m_data->m_materials[i];

        if (material->m_blendMode >= M2BLEND_MOD) {
            material->m_flags |= 1;
        }
    }

    for (uint32_t i = 0; i < this->m_data->m_textures.m_count; i++) {
        M2Texture* texture = &this->m_data->m_textures[i];

        if (texture->m_filename.m_count > 1) {
            CGxTexFlags texFlags;
            texFlags.m_filter = GxTex_LinearMipLinear;
            texFlags.m_wrapU = 1;
            texFlags.m_wrapV = 1;

            uint32_t flags = (this->m_flags & 4) ? 2 : 0;

            if (this->m_flags & 0x40) {
                flags |= 0x30;
            }

            this->m_textures[i] = TextureCreate(texture->m_filename.Data(), texFlags, &GetGlobalStatusObj(), flags);
        } else {
            static const uint32_t white = 0xFFFFFFFF;
            this->m_textures[i] = TextureCreateSolid(*reinterpret_cast<const CImVector*>(&white));
        }
    }

    for (uint32_t i = 0; i < this->m_data->m_bones.m_count; i++) {
        if (this->m_data->m_bones[i].m_flags & 0x2F8) {
            this->m_billboardBoneCount++;
        }
    }

    return 1;
}

// INLINED
int32_t CM2Shared::FinishLoading() {
    if (this->m_flags & 1) {
        return 1;
    }

    if (!M2Init(reinterpret_cast<uint8_t*>(this->m_data), this->m_fileSize, *this->m_data)) {
        DebugLog("Corrupt model data: %s", this->m_fileName);
        return 0;
    }

    this->m_data->m_flags &= ~0x60u;

    if (!this->Initialize()) {
        DebugLog("Failed to initialize model: %s", this->m_fileName);
        return 0;
    }

    uint32_t count = 0;

    for (uint32_t i = 0; i < this->m_data->m_sequences.m_count; i++) {
        if (!(this->m_data->m_sequences[i].m_flags & 0x20)) {
            count++;
        }
    }

    this->m_lowPrioritySequenceBuffers = static_cast<uint8_t**>(SMemAlloc(sizeof(uint8_t*) * count, __FILE__, __LINE__, 8));
    this->m_lowPrioritySequenceCapacity = count;
    this->m_flags |= 1;
    this->m_lowPrioritySequenceCount = 0;

    return 1;
}

// VERIFY: 77f453a0 manual
// OFFSET: 0x0083D340
void CM2Shared::LoadSucceededCallback(void* param) {
    CM2Shared* shared = static_cast<CM2Shared*>(param);
    AsyncFileReadDestroyObject(shared->m_asyncObject);
    shared->m_asyncObject = nullptr;
    shared->FinishLoading();
}

// INLINED
CM2SequenceLoad::CM2SequenceLoad() {
    this->m_asyncObject = nullptr;
    this->m_shared = nullptr;
    this->m_sequenceIndex = 0;
    this->m_sequenceBufferIndex = 0;
}

// VERIFY: c0e38fed manual
// OFFSET: 0x0083D370
CM2SequenceLoad::~CM2SequenceLoad() {
    this->m_playbacks.Clear();

    if (this->m_asyncObject) {
        AsyncFileReadDestroyObject(this->m_asyncObject);
    }
}

// VERIFY: bacb8d39 manual
// OFFSET: 0x0083D410
int32_t CM2Shared::Load(SFileImpl* file, int32_t flags, CAaBox* boundingBox) {
    this->m_flags = (this->m_flags & ~4u) | (flags ? 4 : 0);
    this->m_fileSize = SFileGetFileSize(file, nullptr);
    this->m_data = static_cast<M2Data*>(SIMD::AllocA16(this->m_fileSize, __FILE__, __LINE__));

    if (!this->m_data) {
        return 0;
    }

    if (boundingBox) {
        this->m_boundingBox = *boundingBox;
    }

    this->m_asyncObject = AsyncFileReadAllocObject();

    if (!this->m_asyncObject) {
        return 0;
    }

    this->m_asyncObject->m_file = file;
    this->m_asyncObject->m_buffer = this->m_data;
    this->m_asyncObject->m_size = this->m_fileSize;
    this->m_asyncObject->m_userArg = this;
    this->m_asyncObject->m_userPostloadCallback = reinterpret_cast<void*>(&CM2Shared::LoadSucceededCallback);
    this->m_asyncObject->m_userPostUnloadCallback = reinterpret_cast<void*>(&CM2Shared::LoadFailedCallback);
    this->m_asyncObject->m_isRead = 0;
    this->m_asyncObject->m_isProcessed = 0;
    AsyncFileReadObject(this->m_asyncObject, 0);

    return 1;
}

// VERIFY: 33be3357 manual
// OFFSET: 0x0083D510
void CM2Shared::CancelAllDeferredSequences() {
    for (CM2SequenceLoad* load = this->m_sequenceLoads.Head(); !this->m_sequenceLoads.IsEnd(load); load = this->m_sequenceLoads.RawNext(load)) {
        if (load->m_asyncObject) {
#if defined(WOWDECOMP_ARCH_64)
            if (!AsyncFile::Cancel(load->m_asyncObject, AsyncFileReadFreeNear)) {
#else
            if (!AsyncFile::Cancel(load->m_asyncObject, AsyncFileReadFreeA16)) {
#endif
                this->m_lowPrioritySequenceBuffers[load->m_sequenceBufferIndex] = nullptr;
            }

            load->m_asyncObject = nullptr;
        }
    }

    if (this->m_flags & 0x10) {
        this->m_flags |= 0x20;
    } else {
        this->m_sequenceLoads.Clear();
    }
}

// VERIFY: 7f65a462 manual
// OFFSET: 0x0083D5B0
CM2Shared::~CM2Shared() {
    this->CancelAllDeferredSequences();

    int32_t pending = 0;

    if (this->m_asyncObject) {
        pending = !AsyncFile::Cancel(this->m_asyncObject, AsyncFileReadFreeA16);
    }

    if (this->m_cache) {
        if (this->m_previous) {
            *this->m_previous = this->m_next;
        }

        if (this->m_next) {
            this->m_next->m_previous = this->m_previous;
        }

        this->m_previous = nullptr;
        this->m_next = nullptr;
        this->m_updateLink.Unlink();
    }

    if (this->m_textures) {
        for (uint32_t i = 0; i < this->m_data->m_textures.m_count; i++) {
            if (this->m_textures[i]) {
                HandleClose(reinterpret_cast<HOBJECT>(this->m_textures[i]));
            }
        }

        SMemFree(this->m_textures, __FILE__, __LINE__, 0);
    }

    this->DestroyBuffers();

    if (this->m_skinSections) {
        SMemFree(this->m_skinSections, __FILE__, __LINE__, 0);
    }

    if (this->m_skinData) {
        if (pending) {
            pending = 0;
        } else {
            SIMD::FreeA16(this->m_skinData);
        }
    }

    if (this->m_lowPrioritySequenceCapacity) {
        for (uint32_t i = 0; i < this->m_lowPrioritySequenceCount; i++) {
#if defined(WOWDECOMP_ARCH_64)
            OsFreeNear(this->m_lowPrioritySequenceBuffers[i]);
#else
            SIMD::FreeA16(this->m_lowPrioritySequenceBuffers[i]);
#endif
        }

        SMemFree(this->m_lowPrioritySequenceBuffers, __FILE__, __LINE__, 0);
        this->m_lowPrioritySequenceBuffers = nullptr;
        this->m_lowPrioritySequenceCapacity = 0;
    }

    if (this->m_flags & 8) {
#if defined(WOWDECOMP_ARCH_64)
        OsFreeNear(this->m_data->m_vertices.Data());
#else
        delete[] this->m_data->m_vertices.Data();
#endif
    }

#if !defined(WOWDECOMP_ARCH_64)
    if (this->m_flags & 1) {
        if (this->m_data->m_flags & 0x20) {
            SMemFree(this->m_data->m_textureCombos.Data(), __FILE__, __LINE__, 0);
        }

        if (this->m_data->m_flags & 0x40) {
            SMemFree(this->m_data->m_textureTransformCombos.Data(), __FILE__, __LINE__, 0);
        }
    }
#endif

    if (!pending) {
        SIMD::FreeA16(this->m_data);
    }
}

// VERIFY: aeff737a manual
// OFFSET: 0x0083D840
void CM2Shared::LowPrioritySequenceLoadedCallback(void* param) {
    CM2SequenceLoad* load = static_cast<CM2SequenceLoad*>(param);
    CM2Shared* shared = load->m_shared;

    shared->m_flags |= 0x10;
    shared->FinishLoadingLowPrioritySequence(load->m_sequenceIndex, load->m_asyncObject);

    CM2SequencePlayBack* playback = load->m_playbacks.Head();

    while (!load->m_playbacks.IsEnd(playback)) {
        CM2SequencePlayBack* next = load->m_playbacks.RawNext(playback);

        if (playback->m_flags & 8) {
            load->m_playbacks.DeleteNode(playback);
            playback = next;
            continue;
        }

        CM2Model* model = playback->m_model;

        if (model->OnSequenceInterrupted(shared->m_data->m_bones[playback->m_boneIndex].m_boneId, playback->m_boneIndex)) {
            uint16_t boneIndex = playback->m_boneIndex;
            M2ModelBone* bone = &model->m_bones[boneIndex];

            if (playback->m_flags & 2) {
                M2Sequence* sequence = &shared->m_data->m_sequences[load->m_sequenceIndex];
                bone->m_sequenceId = sequence->m_id;
                bone->m_variationIndex = sequence->m_variationIndex;
                model->SetPrimaryBoneSequence(load->m_sequenceIndex, playback->m_boneIndex, playback->m_fallback, playback->m_time, playback->m_speed, playback->m_flags & 1);
                bone->m_sequence.m_pickRandomVariation = playback->m_flags & 4;
            } else {
                model->SetSecondaryBoneSequence(load->m_sequenceIndex, boneIndex, playback->m_fallback, playback->m_time, playback->m_speed);
                bone->m_secondarySequence.m_pickRandomVariation = playback->m_flags & 4;
            }

            playback->m_model = nullptr;
        }

        playback = next;
    }

    if (shared->m_flags & 0x20) {
        shared->m_sequenceLoads.Clear();
        shared->m_flags &= ~0x30u;
    } else {
        load->~CM2SequenceLoad();
        SMemFree(load, typeid(CM2SequenceLoad).raw_name(), SLOG_OBJECT, 0);
        shared->m_flags &= ~0x10u;
    }
}

// VERIFY: 8862ce16 auto
// OFFSET: 0x0083D9F0
void CM2Shared::LowPrioritySequenceFailedCallback(void* param) {
    CM2SequenceLoad* load = static_cast<CM2SequenceLoad*>(param);
    load->~CM2SequenceLoad();
    SMemFree(load, typeid(CM2SequenceLoad).raw_name(), SLOG_OBJECT, 0);
}

// VERIFY: 0ec0803c manual
// OFFSET: 0x0083DA10
CM2SequenceLoad* CM2Shared::LoadLowPrioritySequence(uint16_t sequenceIndex) {
    M2Sequence* sequence = &this->m_data->m_sequences[sequenceIndex];
    CM2SequenceLoad* load = this->m_sequenceLoads.NewNode(2, 0, 0);

    if (!load) {
        return nullptr;
    }

    load->m_sequenceIndex = sequenceIndex;
    load->m_shared = this;

    M2Sequence* source = sequence;

    while (source->m_flags & 0x40) {
        source = &this->m_data->m_sequences[source->m_aliasNext];
    }

    char fileName[260];
    MakeAnimFileName(this->m_fileName, source->m_id, source->m_variationIndex, fileName);

    SFileImpl* file;

    if (!SFile::OpenEx(nullptr, fileName, (this->m_flags & 4) >> 2, &file)) {
        DebugLog("Model2: File not found: %s\n", fileName);
        this->m_sequenceLoads.DeleteNode(load);
        return nullptr;
    }

    uint32_t size = SFileGetFileSize(file, nullptr);
#if defined(WOWDECOMP_ARCH_64)
    uint8_t* buffer = static_cast<uint8_t*>(OsAllocNear(this->m_data, size));
#else
    uint8_t* buffer = static_cast<uint8_t*>(SIMD::AllocA16(size, __FILE__, __LINE__));
#endif

    if (!buffer) {
        this->m_sequenceLoads.DeleteNode(load);
        SFileCloseFile(file);
        return nullptr;
    }

    load->m_asyncObject = AsyncFileReadAllocObject();

    if (!load->m_asyncObject) {
        SFileCloseFile(file);
#if defined(WOWDECOMP_ARCH_64)
        OsFreeNear(buffer);
#else
        SIMD::FreeA16(buffer);
#endif
        this->m_sequenceLoads.DeleteNode(load);
        return nullptr;
    }

    load->m_sequenceBufferIndex = static_cast<uint16_t>(this->m_lowPrioritySequenceCount);
    this->m_lowPrioritySequenceBuffers[this->m_lowPrioritySequenceCount++] = buffer;

    sequence->m_flags |= 0x10;

    for (uint16_t i = sequence->m_aliasNext; i != sequenceIndex; i = this->m_data->m_sequences[i].m_aliasNext) {
        this->m_data->m_sequences[i].m_flags |= 0x10;
    }

    load->m_asyncObject->m_file = file;
    load->m_asyncObject->m_buffer = buffer;
    load->m_asyncObject->m_size = size;
    load->m_asyncObject->m_userArg = load;
    load->m_asyncObject->m_userPostloadCallback = reinterpret_cast<void*>(&CM2Shared::LowPrioritySequenceLoadedCallback);
    load->m_asyncObject->m_userPostUnloadCallback = reinterpret_cast<void*>(&CM2Shared::LowPrioritySequenceFailedCallback);
    load->m_asyncObject->m_isRead = 0;
    load->m_asyncObject->m_isProcessed = 0;
    load->m_asyncObject->m_priority = 125;
    AsyncFileReadObject(load->m_asyncObject, 0);

    return load;
}

// VERIFY: 554f14ce auto
// OFFSET: 0x0083DC90
void CM2Shared::Release() {
    if (this->m_refCount-- != 1) {
        return;
    }

    if (this->m_cache && this->m_previous) {
        this->m_freeTime = OsGetAsyncTimeMs();
        this->m_freePrev = this->m_cache->m_freeListTail;
        this->m_freeNext = nullptr;
        *this->m_cache->m_freeListTail = this;
        this->m_cache->m_freeListTail = &this->m_freeNext;
    } else {
        delete this;
    }
}

// VERIFY: e4a58b26 manual
// OFFSET: 0x0083DCF0
void M2HeapSort(int32_t (*compare)(uint32_t, uint32_t, const void*), uint32_t* array, uint32_t count, const void* param) {
    if (count <= 1) {
        return;
    }

    for (uint32_t i = count >> 1; i;) {
        i--;

        uint32_t value = array[i];
        uint32_t parent = i;

        for (uint32_t child = 2 * i + 1; child < count; child = 2 * child + 1) {
            uint32_t childValue = array[child];

            if (child + 1 < count && compare(array[child + 1], array[child], param) > 0) {
                child++;
                childValue = array[child];
            }

            if (compare(childValue, value, param) <= 0) {
                break;
            }

            array[parent] = array[child];
            parent = child;
        }

        array[parent] = value;
    }

    for (uint32_t i = count - 1; i; i--) {
        uint32_t value = array[i];
        array[i] = array[0];

        uint32_t parent = 0;

        for (uint32_t child = 1; child < i; child = 2 * child + 1) {
            uint32_t childValue = array[child];

            if (child + 1 < i && compare(array[child + 1], array[child], param) > 0) {
                child++;
                childValue = array[child];
            }

            if (compare(childValue, value, param) <= 0) {
                break;
            }

            array[parent] = array[child];
            parent = child;
        }

        array[parent] = value;
    }
}
