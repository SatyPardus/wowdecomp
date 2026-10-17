#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

struct GxShaderPermutation {
    uint32_t metadata0, metadata1;
    uint16_t metadata2, metadata3;
    std::vector<uint8_t> code;
};

// The BLS container used by CGxDevice::IShaderLoad. Reads little-endian fields
// without unaligned casts and accounts for both record storage and code bytes.
std::vector<GxShaderPermutation> GxReadShaderBinary(const std::vector<uint8_t>& bytes, size_t memoryBudget);
