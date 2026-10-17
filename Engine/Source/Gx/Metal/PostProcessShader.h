#pragma once
#include <cstdint>
#include <vector>

namespace GxMetal {
enum class FragmentEffect : uint32_t { Unlit, Box4, Gauss4, Glow };

// The original FFX filters and glow composition. Both D3D9 permutations use normalized
// coordinates; the RECT variation belongs to the OpenGL assets only.
class PostProcessShader {
public:
    static PostProcessShader FromBls(const std::vector<uint8_t>& bytes, FragmentEffect effect);
    FragmentEffect Effect() const { return m_effect; }
private:
    explicit PostProcessShader(FragmentEffect effect) : m_effect(effect) {}
    FragmentEffect m_effect;
};
}
