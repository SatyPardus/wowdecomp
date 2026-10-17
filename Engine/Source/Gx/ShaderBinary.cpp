#include "Engine/Source/Gx/ShaderBinary.h"
#include <stdexcept>

namespace {
uint32_t Read32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
}
std::vector<GxShaderPermutation> GxReadShaderBinary(const std::vector<uint8_t>& bytes, size_t memoryBudget) {
    if (bytes.size() < 12 || Read32(bytes.data()) != 0x47585348 || Read32(bytes.data() + 4) != 0x10003) {
        throw std::runtime_error("Invalid or unsupported Gx BLS header");
    }
    const uint32_t count = Read32(bytes.data() + 8);
    if (count > (bytes.size() - 12) / 16 || count > memoryBudget / sizeof(GxShaderPermutation)) {
        throw std::runtime_error("Gx BLS permutation count exceeds input or memory budget");
    }
    memoryBudget -= size_t(count) * sizeof(GxShaderPermutation);
    std::vector<GxShaderPermutation> result;
    result.reserve(count);
    size_t offset = 12;
    for (uint32_t i = 0; i < count; ++i) {
        if (bytes.size() - offset < 16) { throw std::runtime_error("Truncated Gx BLS record"); }
        const uint8_t* p = bytes.data() + offset;
        const uint32_t length = Read32(p + 12);
        if (length > bytes.size() - offset - 16 || length > memoryBudget) { throw std::runtime_error("Gx BLS code exceeds input or memory budget"); }
        const size_t padded = (size_t(length) + 3) & ~size_t(3);
        if (padded > bytes.size() - offset - 16) { throw std::runtime_error("Truncated Gx BLS padding"); }
        result.push_back({Read32(p), Read32(p + 4), uint16_t(p[8] | p[9] << 8), uint16_t(p[10] | p[11] << 8),
            std::vector<uint8_t>(p + 16, p + 16 + length)});
        offset += 16 + padded;
        memoryBudget -= length;
    }
    if (offset != bytes.size()) { throw std::runtime_error("Unexpected trailing Gx BLS data"); }
    return result;
}
