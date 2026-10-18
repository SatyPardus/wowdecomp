#include "WoW/Common/ComponentCore/CharacterCompressionNeon.h"
#if defined(__aarch64__) || defined(__arm64__)
#include <arm_neon.h>
#include <algorithm>
#include <cmath>

// Original x86 SIMD component compression expressed as BGRA channel vectors.
// Keep its integer mean, covariance accumulation, eight power iterations,
// ties-to-even quantization and DXT1 palette ordering. Native division/sqrt
// replace the x86 approximate reciprocal instructions; endpoint rounding may
// differ at their quantization boundaries. Do not contract multiply-adds.
void CompressCharacterBlockNeon(const uint8_t* source, uint32_t stride, uint32_t* output) {
    uint16x8_t channelSums = vdupq_n_u16(0);
    float32x4_t pixels[16];
    const float32x4_t scale = vdupq_n_f32(1.0f / 255.0f);
    for (unsigned row = 0; row < 4; ++row) {
        const uint8x16_t bytes = vld1q_u8(source + row * stride);
        const uint16x8_t low = vmovl_u8(vget_low_u8(bytes)), high = vmovl_u8(vget_high_u8(bytes));
        channelSums = vaddq_u16(vaddq_u16(channelSums, low), high);
        pixels[row * 4] = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(low))), scale);
        pixels[row * 4 + 1] = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(low))), scale);
        pixels[row * 4 + 2] = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(high))), scale);
        pixels[row * 4 + 3] = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(high))), scale);
    }
    // The original SIMD path truncates the integer channel mean before scaling.
    const uint16x4_t meanBytes = vshr_n_u16(vadd_u16(vget_low_u16(channelSums), vget_high_u16(channelSums)), 4);
    const float32x4_t mean = vmulq_f32(vcvtq_f32_u32(vmovl_u16(meanBytes)), scale);
    float32x4_t covarianceB = vdupq_n_f32(0), covarianceG = covarianceB, covarianceR = covarianceB;
    for (unsigned row = 0; row < 4; ++row) {
        for (int column = 3; column >= 0; --column) {
            auto& delta = pixels[row * 4 + column]; delta = vsubq_f32(delta, mean);
            covarianceB = vaddq_f32(covarianceB, vmulq_laneq_f32(delta, delta, 0));
            covarianceG = vaddq_f32(covarianceG, vmulq_laneq_f32(delta, delta, 1));
            covarianceR = vaddq_f32(covarianceR, vmulq_laneq_f32(delta, delta, 2));
        }
    }
    float32x4_t axis = vdupq_n_f32(1);
    for (unsigned iteration = 0; iteration < 8; ++iteration) {
        const float32x4_t next = vaddq_f32(vaddq_f32(vmulq_laneq_f32(covarianceR, axis, 2), vmulq_laneq_f32(covarianceG, axis, 1)), vmulq_laneq_f32(covarianceB, axis, 0));
        const float largest = vmaxvq_f32(next);
        axis = largest == 0 ? vdupq_n_f32(0) : vmulq_n_f32(next, 1.0f / largest);
    }
    const auto square = vmulq_f32(axis, axis);
    const auto pair = vaddq_f32(square, vrev64q_f32(square));
    const float length = vgetq_lane_f32(pair, 0) + vgetq_lane_f32(pair, 2);
    axis = length == 0 ? vdupq_n_f32(0) : vmulq_n_f32(axis, 1.0f / std::sqrt(length));
    float projections[16], minimum = 0, maximum = 0;
    for (unsigned pixel = 0; pixel < 16; ++pixel) {
        const auto terms = vmulq_f32(pixels[pixel], axis);
        const float projection = (vgetq_lane_f32(terms, 0) + vgetq_lane_f32(terms, 1)) + vgetq_lane_f32(terms, 2);
        projections[pixel] = projection; minimum = std::min(minimum, projection); maximum = std::max(maximum, projection);
    }
    auto endpoint = [&](float position) {
        const auto rgb = vcvtnq_s32_f32(vmulq_n_f32(vaddq_f32(vmulq_n_f32(axis, position), mean), 255.0f));
        const auto saturated = vqmovn_u16(vcombine_u16(vqmovun_s32(rgb), vdup_n_u16(0)));
        return (uint32_t(vget_lane_u8(saturated, 2) >> 3) << 11) |
            (uint32_t(vget_lane_u8(saturated, 1) >> 2) << 5) | (vget_lane_u8(saturated, 0) >> 3);
    };
    uint32_t first = endpoint(maximum), second = endpoint(minimum), indices = 0;
    if (first != second) {
        const float quantize = 3.0f / (maximum - minimum);
        for (unsigned row = 0; row < 4; ++row) {
            const auto codes = vcvtnq_s32_f32(vmulq_n_f32(vsubq_f32(vld1q_f32(projections + row * 4), vdupq_n_f32(minimum)), quantize));
            int32_t rounded[4]; vst1q_s32(rounded, codes);
            const uint32_t palette[4] = {1, 3, 2, 0};
            for (unsigned column = 0; column < 4; ++column) indices |= palette[rounded[column]] << (2 * (row * 4 + column));
        }
        if (first < second) { std::swap(first, second); indices ^= 0x55555555u; }
    }
    output[0] = first | (second << 16); output[1] = indices;
}

#endif
