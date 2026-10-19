#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Gx/Metal/PostProcessShader.h"

namespace GxMetal {
enum class ModelVertexEffect : uint32_t { Diffuse, Color, ColorDiffuse, DiffuseEnvironment, DiffuseDualUV, DiffuseProjected, ColorProjected, DiffuseReflection };
constexpr bool UsesModelMaterial(ModelVertexEffect effect) {
    return effect == ModelVertexEffect::Diffuse || effect == ModelVertexEffect::DiffuseEnvironment || effect == ModelVertexEffect::DiffuseDualUV || effect == ModelVertexEffect::DiffuseProjected || effect == ModelVertexEffect::DiffuseReflection;
}
constexpr bool UsesModelProjection(ModelVertexEffect effect) {
    return effect == ModelVertexEffect::DiffuseProjected || effect == ModelVertexEffect::ColorProjected;
}
constexpr unsigned ModelColorMode(ModelVertexEffect effect) {
    return UsesModelMaterial(effect) ? 0 : effect == ModelVertexEffect::ColorDiffuse ? 2 : 1;
}
enum class ModelBaseOperation : uint32_t { Multiply, Decal, Fade };
constexpr ModelBaseOperation ModelBase(FragmentEffect effect) {
    return effect == FragmentEffect::ModelDecal ? ModelBaseOperation::Decal :
        effect == FragmentEffect::ModelFade ? ModelBaseOperation::Fade : ModelBaseOperation::Multiply;
}
enum class ModelLayerOperation : uint32_t { None, Add, Multiply, BlendDoubleModulate, DoubleModulate, AddAlpha, AlphaWeightedAdd, BlendAlphaWeightedAdd, DoubleModulateRGB, MultiplyRGB };
constexpr ModelLayerOperation ModelLayer(FragmentEffect effect) {
    return (effect == FragmentEffect::ModelModOpaque || effect == FragmentEffect::ModelOpaqueOpaque) ? ModelLayerOperation::MultiplyRGB :
        (effect == FragmentEffect::ModelOpaqueMod2xNA || effect == FragmentEffect::ModelModMod2xNA) ? ModelLayerOperation::DoubleModulateRGB :
        effect == FragmentEffect::ModelOpaqueAddAlpha ? ModelLayerOperation::AlphaWeightedAdd :
        effect == FragmentEffect::ModelOpaqueAddAlphaAlpha ? ModelLayerOperation::BlendAlphaWeightedAdd :
        (effect == FragmentEffect::ModelModAdd || effect == FragmentEffect::ModelOpaqueAdd) ? ModelLayerOperation::AddAlpha :
        (effect == FragmentEffect::ModelOpaqueMod2x || effect == FragmentEffect::ModelModMod2x) ? ModelLayerOperation::DoubleModulate :
        (effect == FragmentEffect::ModelModMod || effect == FragmentEffect::ModelOpaqueMod) ? ModelLayerOperation::Multiply :
        effect == FragmentEffect::ModelOpaqueMod2xNAAlpha ? ModelLayerOperation::BlendDoubleModulate :
        ModelTextureCount(effect) == 2 ? ModelLayerOperation::Add : ModelLayerOperation::None;
}
const char* ModelVertexFamily(ModelVertexEffect effect);
constexpr bool ModelShadowMaterial(FragmentEffect effect) {
    return IsModelEffect(effect);
}
// Original SM3 model materials, environment reflection and colored particles.
// Diffuse, environment, reflection, dual UV and projected materials support rigid/1/4 weights and R32F receivers;
// colored vertices support rigid streams and R32F receivers. Other skinning/shadow
// handles fail explicitly when drawn.
void ValidateModelShader(const std::vector<uint8_t>& bytes, EGxShTarget target, FragmentEffect effect, ModelVertexEffect vertexEffect = ModelVertexEffect::Diffuse);
bool SupportsModelPermutation(EGxShTarget target, uint32_t permutation, ModelVertexEffect vertexEffect = ModelVertexEffect::Diffuse,
    FragmentEffect effect = FragmentEffect::ModelOpaque);
inline uint32_t ShadowReceiverTextureMask(uint32_t mode) { return mode >= 2 ? 0xF0u : mode ? 0x10u : 0u; }
}
