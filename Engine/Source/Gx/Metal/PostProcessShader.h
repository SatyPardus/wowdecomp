#pragma once
#include <cstdint>
#include <vector>

namespace GxMetal {
enum class FragmentEffect : uint32_t { Unlit, Box4, Gauss4, Glow, GlowWave, ModelOpaque };

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
