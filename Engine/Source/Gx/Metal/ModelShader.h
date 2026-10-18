#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Gx/Metal/PostProcessShader.h"

namespace GxMetal {
enum class ModelVertexEffect : uint32_t { Diffuse, Color, ColorDiffuse, DiffuseEnvironment, DiffuseDualUV, DiffuseProjected, ColorProjected };
constexpr bool UsesModelMaterial(ModelVertexEffect effect) {
    return effect == ModelVertexEffect::Diffuse || effect == ModelVertexEffect::DiffuseEnvironment || effect == ModelVertexEffect::DiffuseDualUV || effect == ModelVertexEffect::DiffuseProjected;
}
constexpr bool UsesModelProjection(ModelVertexEffect effect) {
    return effect == ModelVertexEffect::DiffuseProjected || effect == ModelVertexEffect::ColorProjected;
}
constexpr unsigned ModelColorMode(ModelVertexEffect effect) {
    return UsesModelMaterial(effect) ? 0 : effect == ModelVertexEffect::ColorDiffuse ? 2 : 1;
}
enum class ModelLayerOperation : uint32_t { None, Add, Multiply, BlendDoubleModulate, DoubleModulate, AddAlpha };
constexpr ModelLayerOperation ModelLayer(FragmentEffect effect) {
    return effect == FragmentEffect::ModelModAdd ? ModelLayerOperation::AddAlpha :
        effect == FragmentEffect::ModelOpaqueMod2x ? ModelLayerOperation::DoubleModulate :
        effect == FragmentEffect::ModelModMod ? ModelLayerOperation::Multiply :
        effect == FragmentEffect::ModelOpaqueMod2xNAAlpha ? ModelLayerOperation::BlendDoubleModulate :
        ModelTextureCount(effect) == 2 ? ModelLayerOperation::Add : ModelLayerOperation::None;
}
const char* ModelVertexFamily(ModelVertexEffect effect);
// Original SM3 model materials, environment reflection and colored particles.
// Diffuse supports rigid/1/4 weights; colored vertices currently support rigid
// streams. Other skinning and shadow handles fail explicitly when drawn.
void ValidateModelShader(const std::vector<uint8_t>& bytes, EGxShTarget target, FragmentEffect effect, ModelVertexEffect vertexEffect = ModelVertexEffect::Diffuse);
bool SupportsModelPermutation(EGxShTarget target, uint32_t permutation, ModelVertexEffect vertexEffect = ModelVertexEffect::Diffuse);
}
