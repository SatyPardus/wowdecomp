#include "Engine/Source/ComSat/ComSatSoundLevelCalculator.h"
#include <cmath>
#include <cstring>

// VERIFY: 8d8479ca auto
// OFFSET: 0x00887DE0
void ComSat::Sound::LevelCalculator16Bit::CalculateLevelForFrame(const uint8_t* data, uint32_t size, float& level, float& decibels) {
    level = 0.0f;
    decibels = 0.0f;
    int32_t count = size >> 1;
    int16_t maximum = -32768;
    int16_t minimum = 32767;
    if (count > 0) {
        for (int32_t i = 0; i < count; i++) {
            int16_t sample;
            std::memcpy(&sample, data + size_t(i) * sizeof(sample), sizeof(sample));
            if (sample < minimum) {
                minimum = sample;
            }
            if (sample > maximum) {
                maximum = sample;
            }
        }
        if (minimum <= maximum) {
            level = static_cast<float>(maximum - minimum) * (1.0f / 65536.0f);
            decibels = log10f((static_cast<float>(maximum) - minimum) * 0.002f) * 20.0f;
        }
    }
}

// VERIFY: d71b6a92 auto
// OFFSET: 0x00887E80
float ComSat::Sound::LevelCalculator16Bit::GetMaxDecibelLevel() {
    return static_cast<float>(log10(131.072) * 20.0f);
}
