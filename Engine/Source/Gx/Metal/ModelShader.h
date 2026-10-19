#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Gx/Metal/PostProcessShader.h"

namespace GxMetal {
enum class ModelVertexEffect : uint32_t { Diffuse, Color, ColorDiffuse, DiffuseEnvironment, DiffuseDualUV, DiffuseProjected, ColorProjected, DiffuseReflection, DiffuseSecondUV, DiffuseDualReflection, DiffuseReflectionUV, Count };
constexpr bool UsesModelMaterial(ModelVertexEffect effect) {
    return effect < ModelVertexEffect::Count && effect != ModelVertexEffect::Color && effect != ModelVertexEffect::ColorDiffuse && effect != ModelVertexEffect::ColorProjected;
}
constexpr bool UsesModelProjection(ModelVertexEffect effect) {
    return effect == ModelVertexEffect::DiffuseProjected || effect == ModelVertexEffect::ColorProjected;
}
// Coordinate sources and matrices are paired as in the original BLS declarations.
// In particular, Diffuse_Env_T2 reads TEXCOORD0/c6-c7, despite its family name.
enum class ModelUVSource : uint32_t { None, AuthoredFirst, AuthoredSecond, Reflection, ProjectedFirst, ProjectedSecond };
constexpr ModelUVSource ModelPrimaryUV(ModelVertexEffect effect) {
    return UsesModelProjection(effect) ? ModelUVSource::ProjectedFirst :
        effect == ModelVertexEffect::DiffuseSecondUV ? ModelUVSource::AuthoredSecond :
        (effect == ModelVertexEffect::DiffuseReflection || effect == ModelVertexEffect::DiffuseDualReflection || effect == ModelVertexEffect::DiffuseReflectionUV) ? ModelUVSource::Reflection : ModelUVSource::AuthoredFirst;
}
constexpr ModelUVSource ModelSecondaryUV(ModelVertexEffect effect) {
    return UsesModelProjection(effect) ? ModelUVSource::ProjectedSecond :
        effect == ModelVertexEffect::DiffuseDualUV ? ModelUVSource::AuthoredSecond :
        effect == ModelVertexEffect::DiffuseReflectionUV ? ModelUVSource::AuthoredFirst :
        (effect == ModelVertexEffect::DiffuseEnvironment || effect == ModelVertexEffect::DiffuseDualReflection) ? ModelUVSource::Reflection : ModelUVSource::None;
}
constexpr unsigned ModelColorMode(ModelVertexEffect effect) {
    return UsesModelMaterial(effect) ? 0 : effect == ModelVertexEffect::ColorDiffuse ? 2 : 1;
}
enum class ModelBaseOperation : uint32_t { Multiply, Decal, Fade, Add, DoubleMultiply };
constexpr ModelBaseOperation ModelBase(FragmentEffect effect) {
    return (effect == FragmentEffect::ModelAdd || effect == FragmentEffect::ModelAddMod) ? ModelBaseOperation::Add :
        effect == FragmentEffect::ModelMod2x ? ModelBaseOperation::DoubleMultiply :
        effect == FragmentEffect::ModelDecal ? ModelBaseOperation::Decal :
        effect == FragmentEffect::ModelFade ? ModelBaseOperation::Fade : ModelBaseOperation::Multiply;
}
enum class ModelLayerOperation : uint32_t { None, Add, Multiply, BlendDoubleModulate, DoubleModulate, AddAlpha, AlphaWeightedAdd, BlendAlphaWeightedAdd, DoubleModulateRGB, MultiplyRGB, QuadrupleModulate };
constexpr ModelLayerOperation ModelLayer(FragmentEffect effect) {
    return effect == FragmentEffect::ModelMod2xMod2x ? ModelLayerOperation::QuadrupleModulate :
        (effect == FragmentEffect::ModelModOpaque || effect == FragmentEffect::ModelOpaqueOpaque) ? ModelLayerOperation::MultiplyRGB :
        (effect == FragmentEffect::ModelOpaqueMod2xNA || effect == FragmentEffect::ModelModMod2xNA) ? ModelLayerOperation::DoubleModulateRGB :
        effect == FragmentEffect::ModelOpaqueAddAlpha ? ModelLayerOperation::AlphaWeightedAdd :
        effect == FragmentEffect::ModelOpaqueAddAlphaAlpha ? ModelLayerOperation::BlendAlphaWeightedAdd :
        (effect == FragmentEffect::ModelModAdd || effect == FragmentEffect::ModelOpaqueAdd) ? ModelLayerOperation::AddAlpha :
        (effect == FragmentEffect::ModelOpaqueMod2x || effect == FragmentEffect::ModelModMod2x) ? ModelLayerOperation::DoubleModulate :
        (effect == FragmentEffect::ModelModMod || effect == FragmentEffect::ModelOpaqueMod || effect == FragmentEffect::ModelAddMod) ? ModelLayerOperation::Multiply :
        effect == FragmentEffect::ModelOpaqueMod2xNAAlpha ? ModelLayerOperation::BlendDoubleModulate :
        ModelTextureCount(effect) == 2 ? ModelLayerOperation::Add : ModelLayerOperation::None;
}
const char* ModelVertexFamily(ModelVertexEffect effect);
constexpr bool ModelShadowMaterial(FragmentEffect effect) {
    return IsModelEffect(effect);
}
// Original SM3 model materials, environment reflection and colored particles.
// Diffuse, environment, reflection, dual UV and projected materials support rigid/1/4 weights;
// colored vertices support rigid streams. Receivers accept R32F or native PCF. Other skinning
// handles fail explicitly when drawn.
void ValidateModelShader(const std::vector<uint8_t>& bytes, EGxShTarget target, FragmentEffect effect, ModelVertexEffect vertexEffect = ModelVertexEffect::Diffuse);
bool SupportsModelPermutation(EGxShTarget target, uint32_t permutation, ModelVertexEffect vertexEffect = ModelVertexEffect::Diffuse,
    FragmentEffect effect = FragmentEffect::ModelOpaque);
inline uint32_t ShadowReceiverTextureMask(uint32_t mode) { return mode >= 2 ? 0xF0u : mode ? 0x10u : 0u; }
}
