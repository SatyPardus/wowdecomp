#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Model2/M2Memory.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Skinning.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Base/SIMD.h"
#include "Engine/Source/Os/OsMemory.h"
#include "WoW/Source/WowServices/ClientData.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <ostream>
#include <stdexcept>

namespace {
void CheckSkinning(const M2Data& data, std::ostream& report) {
    if (!data.m_vertices.m_count) return;
    C44Matrix bones[256];
    for (unsigned i = 0; i < 256; ++i) {
        bones[i].Identity();
        bones[i].a0 = 0.25f + float(i % 17) / 16;
        bones[i].b1 = 0.5f + float(i % 11) / 16;
        bones[i].c2 = 0.75f + float(i % 7) / 8;
        bones[i].a1 = -0.125f; bones[i].c0 = 0.375f;
        bones[i].d0 = float(int(i) - 128) / 16; bones[i].d2 = float(i % 13) / 8;
    }
    const auto count = data.m_vertices.m_count;
    std::vector<float> scalar(count * 8);
    std::unique_ptr<void, decltype(&SIMD::FreeA16)> vector(SIMD::AllocA16(count * 32, __FILE__, __LINE__), SIMD::FreeA16);
    if (!vector) throw std::runtime_error("M2 skinning output allocation failed");
    float maximumError = 0;
    for (int single = 0; single < 2; ++single) for (int uv = 0; uv < 2; ++uv) {
        const auto reference = single ? M2TransformVerticesOneBone_cpp : M2TransformVertices_cpp;
        const auto native = single ? M2TransformVerticesOneBone_simd : M2TransformVertices_simd;
        reference(bones, data.m_vertices.Data(), count, reinterpret_cast<uint8_t*>(scalar.data()), uv);
        native(bones, data.m_vertices.Data(), count, static_cast<uint8_t*>(vector.get()), uv);
        const auto* result = static_cast<const float*>(vector.get());
        for (unsigned i = 0; i < count * 8; ++i) {
            const auto error = std::abs(result[i] - scalar[i]);
            if (!std::isfinite(result[i]) || error > 0.00001f * std::max(1.0f, std::abs(scalar[i])) ||
                (i % 8 >= 6 && error != 0)) throw std::runtime_error("M2 native vertex skinning differs from original scalar path");
            maximumError = std::max(error, maximumError);
        }
    }
    report << "  native CPU skinning: both UV sets, weighted/one-bone; maximum scalar difference " << maximumError << '\n';
}
}

void CheckM2Resources(const ClientData& client, std::ostream& report) {
    const char* names[] = {"Cameras\\FlyByDwarf.m2", "Character\\NorthrendSkeleton\\Male\\NorthrendSkeletonMale.M2"};
    for (const char* name : names) {
        const auto asset = client.Read(name, 64 * 1024 * 1024, true);
        CM2Shared shared(nullptr);
        shared.m_fileSize = uint32_t(asset.bytes.size());
        shared.m_data = static_cast<M2Data*>(M2AllocData(shared.m_fileSize));
        auto* memory = reinterpret_cast<uint8_t*>(shared.m_data);
        if (!memory) throw std::runtime_error("M2 allocation failed");
        std::memcpy(memory, asset.bytes.data(), asset.bytes.size());
        auto& data = *reinterpret_cast<M2Data*>(memory);
        if (!M2Init(static_cast<uint8_t*>(memory), uint32_t(asset.bytes.size()), data)) throw std::runtime_error("M2 relocation failed");
        report << "M2 relocated " << name << ": " << data.m_vertices.m_count << " vertices, "
            << data.m_bones.m_count << " bones, " << data.m_sequences.m_count << " sequences, " << data.m_cameras.m_count << " cameras\n";
        CheckSkinning(data, report);
        auto malformed = asset.bytes;
        auto& bad = *reinterpret_cast<M2Data*>(malformed.data());
        if (bad.m_bones.m_count) {
            auto* bone = reinterpret_cast<M2CompBone*>(malformed.data() + bad.m_bones.m_offset);
            bone->m_translationTrack.m_sequenceTimes.m_count = 1;
            bone->m_translationTrack.m_sequenceKeys.m_count = 1;
            bone->m_translationTrack.m_sequenceTimes.m_offset = 0xFFFFFFFC;
            if (M2Init(malformed.data(), uint32_t(malformed.size()), bad)) throw std::runtime_error("M2 invalid nested track offset accepted");
        }
        for (uint32_t i = 0; i < data.m_sequences.m_count; ++i) {
            const auto& sequence = data.m_sequences[i];
            if (sequence.m_flags & 0x60) continue;
            char suffix[32];
            std::snprintf(suffix, sizeof(suffix), "%04u-%02u.anim", sequence.m_id, sequence.m_variationIndex);
            const auto animationName = std::string(name).substr(0, std::string(name).find_last_of('.')) + suffix;
            const auto animation = client.Read(animationName, 64 * 1024 * 1024, true);
            std::unique_ptr<void, decltype(&OsFreeNear)> animationMemory(OsAllocNear(memory, animation.bytes.size()), OsFreeNear);
            if (!animationMemory) throw std::runtime_error("M2 external animation allocation failed");
            std::memcpy(animationMemory.get(), animation.bytes.data(), animation.bytes.size());
            std::vector<uint8_t> before(static_cast<uint8_t*>(memory), static_cast<uint8_t*>(memory) + asset.bytes.size());
            CAsyncObject object{};
            object.m_buffer = animationMemory.get();
            object.m_size = 1;
            if (shared.FinishLoadingLowPrioritySequence(i, &object) ||
                std::memcmp(before.data(), memory, before.size())) throw std::runtime_error("M2 truncated animation modified the loaded model");
            object.m_size = uint32_t(animation.bytes.size());
            if (!shared.FinishLoadingLowPrioritySequence(i, &object)) throw std::runtime_error("M2 external animation/alias relocation failed");
            report << "  external animation " << animationName << ": " << animation.bytes.size() << " bytes; truncated-load retry passed\n";
            break;
        }
        std::unique_ptr<void, decltype(&OsFreeNear)> nearby(OsAllocNear(memory, 1024 * 1024), OsFreeNear);
        if (!nearby) throw std::runtime_error("M2 nearby allocation failed");
        auto* first = reinterpret_cast<M2Array<uint8_t>*>(memory);
        first->m_count = 1;
        first->Set(static_cast<uint8_t*>(nearby.get()));
        if (first->Data() != nearby.get()) throw std::runtime_error("M2 relative pointer truncated");
    }
    C44Matrix bones[4];
    for (unsigned i = 0; i < 4; ++i) {
        auto* values = reinterpret_cast<float*>(&bones[i]);
        for (unsigned j = 0; j < 16; ++j) values[j] = float(int(i * 37 + j * 11) - 80) / 32;
    }
    for (ubyte4 weights : {ubyte4{{255, 0, 0, 0}}, ubyte4{{128, 127, 0, 0}}, ubyte4{{96, 80, 64, 15}}}) {
        C44Matrix scalar, vector;
        ubyte4 indices{{3, 1, 2, 0}};
        M2BlendBoneMatrices(bones, weights, indices, &scalar);
        M2BlendBoneMatricesSIMD(bones, weights, indices, &vector);
        if (std::memcmp(&scalar, &vector, sizeof(scalar))) throw std::runtime_error("M2 SIMD bone blend changed scalar results");
    }
    report << "M2 relative animation allocation and native SIMD bone blending passed.\n";
}
