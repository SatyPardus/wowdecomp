#pragma once
#include <cstdint>
#include <vector>

namespace GxMetal {
enum class FragmentEffect : uint32_t { Unlit, Box4, Gauss4, Glow, GlowWave, ModelOpaque, ModelMod, ModelOpaqueAddNA, ModelModAddNA };
constexpr bool IsModelEffect(FragmentEffect effect) {
    return effect == FragmentEffect::ModelOpaque || effect == FragmentEffect::ModelMod ||
        effect == FragmentEffect::ModelOpaqueAddNA || effect == FragmentEffect::ModelModAddNA;
}
constexpr unsigned ModelTextureCount(FragmentEffect effect) {
    return effect == FragmentEffect::ModelOpaqueAddNA || effect == FragmentEffect::ModelModAddNA ? 2 : 1;
}

// Original FFX filters and glow composition, including wave displacement.
// D3D9 permutations use normalized coordinates; RECT belongs to OpenGL assets.
class PostProcessShader {
public:
    static PostProcessShader FromBls(const std::vector<uint8_t>& bytes, FragmentEffect effect);
    FragmentEffect Effect() const { return m_effect; }
private:
    explicit PostProcessShader(FragmentEffect effect) : m_effect(effect) {}
    FragmentEffect m_effect;
};
}
