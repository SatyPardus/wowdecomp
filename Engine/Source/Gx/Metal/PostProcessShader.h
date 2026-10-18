#pragma once
#include <cstdint>
#include <vector>

namespace GxMetal {
enum class FragmentEffect : uint32_t { Unlit, Box4, Gauss4, Glow, GlowWave, ModelOpaque, ModelMod, ModelOpaqueAddNA, ModelModAddNA, ModelModMod, ModelOpaqueMod2xNAAlpha, ModelOpaqueMod2x, ModelModAdd, ModelModMod2x, Terrain, WmoDiffuse, WmoOpaque, WmoSpecular, WmoMetal, WmoEnvironment, WmoEnvironmentMetal, WmoComposite, LiquidWater, LiquidWaterNoSpec, LiquidMagma, ShadowMap, DetailDoodad, WaterRipple, Terrain2, Terrain3, Terrain0, Terrain0Environment, TerrainShadowMap };
constexpr bool IsModelEffect(FragmentEffect effect) {
    return effect == FragmentEffect::ModelOpaque || effect == FragmentEffect::ModelMod ||
        effect == FragmentEffect::ModelOpaqueAddNA || effect == FragmentEffect::ModelModAddNA ||
        effect == FragmentEffect::ModelModMod || effect == FragmentEffect::ModelOpaqueMod2xNAAlpha || effect == FragmentEffect::ModelOpaqueMod2x || effect == FragmentEffect::ModelModAdd || effect == FragmentEffect::ModelModMod2x;
}
constexpr unsigned ModelTextureCount(FragmentEffect effect) {
    return IsModelEffect(effect) && effect != FragmentEffect::ModelOpaque && effect != FragmentEffect::ModelMod ? 2 : 1;
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
