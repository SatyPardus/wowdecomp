#pragma once
#include "Engine/Source/Gx/Metal/PostProcessShader.h"
#include "Engine/Source/Gx/Types.h"

namespace GxMetal {
enum class WmoVertexEffect : uint32_t { Diffuse, Specular, Environment, UnifiedDiffuse, UnifiedSpecular, UnifiedEnvironment, Composite };
constexpr bool IsWmoEffect(FragmentEffect effect) { return effect >= FragmentEffect::WmoDiffuse && effect <= FragmentEffect::WmoComposite; }
constexpr bool WmoEnvironment(WmoVertexEffect effect) { return effect == WmoVertexEffect::Environment || effect == WmoVertexEffect::UnifiedEnvironment; }
constexpr bool WmoSpecular(WmoVertexEffect effect) { return effect == WmoVertexEffect::Specular || effect == WmoVertexEffect::UnifiedSpecular; }
constexpr unsigned WmoTextureCount(FragmentEffect effect) { return effect >= FragmentEffect::WmoEnvironment ? 2 : 1; }
constexpr bool SupportsWmoPermutation(EGxShTarget target, uint32_t permutation) {
    return target == GxSh_Vertex ? permutation < 60 : permutation < 16 && (!(permutation & 3) || (permutation & 7) == 1);
}
const char* WmoVertexFamily(WmoVertexEffect effect);
const char* WmoPixelFamily(FragmentEffect effect);
// Validate the exact supported permutations from the mounted D3D9 assets;
// R32F dynamic shadows are supported; remaining shadow variants fail explicitly.
void ValidateWmoShader(const std::vector<uint8_t>& bytes, EGxShTarget target, WmoVertexEffect vertex, FragmentEffect pixel);
}
