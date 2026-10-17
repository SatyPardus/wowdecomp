#pragma once
#include <cstdint>
#include <vector>

namespace GxMetal {
enum class FragmentEffect : uint32_t { Unlit, Box4, Gauss4 };

// The original FFX four-tap filters. Both D3D9 permutations use normalized
// coordinates; the RECT variation belongs to the OpenGL assets only.
class Filter4Shader {
public:
    static Filter4Shader FromBls(const std::vector<uint8_t>& bytes, FragmentEffect effect);
    FragmentEffect Effect() const { return m_effect; }
private:
    explicit Filter4Shader(FragmentEffect effect) : m_effect(effect) {}
    FragmentEffect m_effect;
};
}
