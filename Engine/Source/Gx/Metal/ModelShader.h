#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Gx/Metal/PostProcessShader.h"

namespace GxMetal {
enum class ModelVertexEffect : uint32_t { Diffuse, Color, ColorDiffuse };
const char* ModelVertexFamily(ModelVertexEffect effect);
// Original SM3 model materials and Color_T1/CDiffuse_T1 particle vertices.
// Diffuse supports rigid/1/4 weights; colored vertices currently support rigid
// streams. Other skinning and shadow handles fail explicitly when drawn.
void ValidateModelShader(const std::vector<uint8_t>& bytes, EGxShTarget target, FragmentEffect effect, ModelVertexEffect vertexEffect = ModelVertexEffect::Diffuse);
bool SupportsModelPermutation(EGxShTarget target, uint32_t permutation, ModelVertexEffect vertexEffect = ModelVertexEffect::Diffuse);
}
