#pragma once
#include <cstdint>

// One four-by-four BGRA block, retaining the original SIMD compressor semantics.
// Input rows need not be SIMD aligned; output is two little-endian DXT1 words.
void CompressCharacterBlockNeon(const uint8_t* source, uint32_t stride, uint32_t* output);
