#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Source/Tempest/C44Matrix.h"

namespace GxMetal {
struct UITransform {
    C44Matrix clipFromModel;
    bool screenAtDepth;
};

// Native semantic implementation of the original UI BLS family. Recognition
// prevents a patched/overridden shader from silently selecting unrelated MSL.
// Fingerprints identify supported code; they are not the client's signature policy.
class UIShader {
public:
    static UIShader FromBls(const std::vector<uint8_t>& vertex, const std::vector<uint8_t>& pixel);
    static void ValidateDesaturate(const std::vector<uint8_t>& pixel);
    UITransform VertexTransform(const float* constantRegisters, uint32_t registerCount, uint32_t permutation) const;
    uint32_t VertexMajorVersion() const { return m_vertexMajor; }
    uint32_t PixelMajorVersion() const { return m_pixelMajor; }
private:
    UIShader(uint32_t vertex, uint32_t pixel) : m_vertexMajor(vertex), m_pixelMajor(pixel) {}
    uint32_t m_vertexMajor, m_pixelMajor;
};
}
