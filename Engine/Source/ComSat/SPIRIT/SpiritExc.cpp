#include <cmath>
#include <cstdint>
#include "Engine/Source/ComSat/SPIRIT/ipmr.h"

extern "C" {

// VERIFY: 6e412523 manual
// OFFSET: 0x0089D3B0
int32_t SPIRIT_IP_MR_V3_NAME_37(float* lsp, const int32_t* indices, int16_t lost, float* lsf, const float* previous, const SPIRIT_IPMR_TABLES* tables) {
    if (lost) {
        for (int32_t i = 0; i < 10; i++) {
            lsf[i] = static_cast<float>(lsf[i] * static_cast<double>(0.9f) + tables->unk_B7C0[i] * static_cast<double>(0.100000024f));
        }
    } else if (previous) {
        SPIRIT_IP_MR_V3_NAME_190(lsf, tables->unk_B850);
        SPIRIT_IP_MR_V3_NAME_216(previous, lsp);
        const float* codebook;

        if (indices[3]) {
            codebook = tables->unk_4600;

            for (int32_t i = 0; i < 10; i++) {
                lsf[i] = (lsp[i] - tables->unk_B850[i]) * tables->unk_B7F0[i] + lsf[i];
            }
        } else {
            codebook = tables->unk_7800;

            for (int32_t i = 0; i < 10; i++) {
                lsf[i] = (lsp[i] - tables->unk_B850[i]) * tables->unk_B820[i] + lsf[i];
            }
        }

        for (int32_t s = 0; s < 3; s++) {
            const float* vector = &codebook[10 * indices[s]];

            for (int32_t i = 0; i < 10; i++) {
                lsf[i] = vector[i] + lsf[i];
            }

            codebook += 10 * s_ipmrTableB23758[s];
        }
    } else {
        SPIRIT_IP_MR_V3_NAME_190(lsf, tables->unk_B7C0);
        const float* codebook = tables->unk_0000;

        for (int32_t s = 0; s < 4; s++) {
            const float* vector = &codebook[10 * indices[s]];

            for (int32_t i = 0; i < 10; i++) {
                lsf[i] = vector[i] + lsf[i];
            }

            codebook += 10 * s_ipmrTableB23744[s];
        }
    }

    int32_t result = SPIRIT_IP_MR_V3_SortLsf(lsf);

    for (int32_t i = 0; i < 10; i++) {
        lsp[i] = static_cast<float>(cos(lsf[i] * static_cast<double>(3.1415927f)));
    }

    return result;
}

// VERIFY: ab2546f7 auto
// OFFSET: 0x0089D840
void SPIRIT_IP_MR_V3_NAME_29(const float* a, const float* coeffs, float* h) {
    SPIRIT_IP_MR_V3_NAME_389(coeffs, h, a);
}

// VERIFY: 16a0ba4d manual
// OFFSET: 0x0089D860
void SPIRIT_IP_MR_V3_NAME_147(const float* target, const float* a, const float* weighted, float* out, float* synthesis, float* weighting) {
    // The original stack block has ten history samples directly before the
    // forty filtered samples (0x89d860). Keep that adjacency in one array.
    float history[50];
    float* saved = history;
    float* zero = history + 10;
    float response[40];
    SPIRIT_IP_MR_V3_NAME_190(saved, synthesis);
    SPIRIT_IP_MR_V3_NAME_390(zero, a, synthesis);
    SPIRIT_IP_MR_V3_NAME_137(zero, response, weighted);
    SPIRIT_IP_MR_V3_NAME_148(response, response, weighted + 10, weighting, 0);

    for (int32_t i = 0; i < 40; i++) {
        out[i] = target[i] - response[i];
    }
}

// VERIFY: 24685002 manual
// OFFSET: 0x0089D9D0
void SPIRIT_IP_MR_V3_NAME_146(const float* excitation, float* residual, const float* a, const float* weighted, float* out, SPIRIT_IPMR_TRACK* tracks, float* memory, int16_t count) {
    // NAME_137 consumes x[-10..39]; the original saved history is its prefix.
    float history[50];
    float* saved = history;
    float* synthesized = history + 10;
    SPIRIT_IP_MR_V3_NAME_137(excitation, residual, a);
    SPIRIT_IP_MR_V3_NAME_148(residual, synthesized, a, memory + 10, 0);
    SPIRIT_IP_MR_V3_NAME_190(saved, memory + 10);
    SPIRIT_IP_MR_V3_NAME_137(synthesized, tracks->unk_00CC, weighted);
    SPIRIT_IP_MR_V3_NAME_148(tracks->unk_00CC, out, weighted + 10, memory + 20, 0);

    for (int16_t t = 0; t < count; t++) {
        SPIRIT_IP_MR_V3_NAME_190(saved, memory);
        SPIRIT_IP_MR_V3_NAME_390(synthesized, a, memory);
        SPIRIT_IP_MR_V3_NAME_190(saved, memory + 10);
        SPIRIT_IP_MR_V3_NAME_137(synthesized, tracks->unk_00CC, weighted);
        SPIRIT_IP_MR_V3_NAME_148(tracks->unk_00CC, tracks->unk_00CC, weighted + 10, memory + 20, 0);
        memory += 30;
        tracks++;
    }
}

// VERIFY: 915e965b manual
// OFFSET: 0x0089DAC0
void SPIRIT_IP_MR_V3_NAME_157(const float* excitation, const float* a, const float* weighted, float* synthesis, float* weighting, float* out) {
    // NAME_137 consumes x[-10..39]; the original saved history is its prefix.
    float history[50];
    float* saved = history;
    float* synthesized = history + 10;
    float response[40];
    SPIRIT_IP_MR_V3_NAME_190(saved, synthesis);
    SPIRIT_IP_MR_V3_NAME_148(excitation, synthesized, a, synthesis, 1);
    SPIRIT_IP_MR_V3_NAME_191(out, synthesized);
    SPIRIT_IP_MR_V3_NAME_137(synthesized, response, weighted);
    SPIRIT_IP_MR_V3_NAME_148(response, response, weighted + 10, weighting, 1);
}

// VERIFY: 4ada1b21 auto
// OFFSET: 0x0089DB40
void SPIRIT_IP_MR_V3_NAME_28(float* memory, int16_t first, int16_t last) {
    for (int32_t t = first; t <= last; t++) {
        SPIRIT_IP_MR_V3_NAME_314(&memory[30 * t]);
        SPIRIT_IP_MR_V3_NAME_314(&memory[30 * t + 10]);
        SPIRIT_IP_MR_V3_NAME_314(&memory[30 * t + 20]);
    }
}

// VERIFY: 0478c6cc manual
// OFFSET: 0x0089DB90
void SPIRIT_IP_MR_V3_SortPulses(int16_t count, SPIRIT_IPMR_TRACK* tracks) {
    for (int32_t t = 0; t < count; t++) {
        SPIRIT_IPMR_TRACK* track = &tracks[t];

        for (int16_t i = track->unk_0000 - 2; i >= 0; i--) {
            int16_t best = i;

            for (int16_t j = i + 1; j < track->unk_0000; j++) {
                int16_t position = track->unk_0018[j];

                if (track->unk_0018[best] > position) {
                    float amplitude = track->unk_0004[j];
                    track->unk_0018[j] = track->unk_0018[best];
                    track->unk_0004[j] = track->unk_0004[best];
                    track->unk_0018[best] = position;
                    track->unk_0004[best] = amplitude;
                    best = j;
                }
            }
        }
    }
}

// VERIFY: 40e4922a manual
// OFFSET: 0x0089DC60
void SPIRIT_IP_MR_V3_EncodePulsePositions(SPIRIT_IPMR_TRACK* tracks, int16_t count, const SPIRIT_IPMR_TABLES* tables) {
    int16_t taken[40] = {};

    for (int16_t t = 0; t < count; t++) {
        SPIRIT_IPMR_TRACK* track = &tracks[t];
        track->unk_0024 = 0;
        track->unk_002A = 0;

        if (t >= 6) {
            for (int32_t i = 0; i < 40; i++) {
                taken[i] = 0;
            }
        }

        for (int16_t k = track->unk_0000 - 1; k >= 0; k--) {
            int16_t position = track->unk_0018[k];
            int16_t index = position - taken[position];
            track->unk_0024 += tables->unk_B8AC[index][k];
            track->unk_002A = static_cast<int16_t>(track->unk_002A * 2);

            if (track->unk_0004[k] < 0.0f) {
                track->unk_002A++;
            }
        }

        for (int16_t i = 1; i < track->unk_0000; i++) {
            for (int16_t q = track->unk_0018[i - 1]; q < track->unk_0018[i]; q++) {
                taken[q] += i;
            }
        }

        int16_t pulses = track->unk_0000;

        for (int16_t q = track->unk_0018[pulses - 1]; q < 40; q++) {
            taken[q] += pulses;
        }
    }
}

// VERIFY: 1d98f2c2 manual
// OFFSET: 0x0089DE80
void SPIRIT_IP_MR_V3_NAME_35(const SPIRIT_IPMR_TRACK* tracks, int16_t first, int16_t last, float* out) {
    SPIRIT_IP_MR_V3_NAME_333(out);

    for (int16_t t = first; t <= last; t++) {
        const SPIRIT_IPMR_TRACK* track = &tracks[t];

        for (int16_t i = 0; i < track->unk_0000; i++) {
            out[track->unk_0018[i]] = track->unk_0004[i] + out[track->unk_0018[i]];
        }
    }
}

// VERIFY: af25792b manual
// OFFSET: 0x0089DF30
void SPIRIT_IP_MR_V3_QuantizeUniform(int16_t levels, float* value, int16_t* index, float low, float high) {
    float step = (high - low) / (levels - 1);
    float threshold = static_cast<float>(0.5 * step + low);
    int16_t level = 0;

    while (level < static_cast<int16_t>(levels - 1)) {
        if (threshold > *value) {
            break;
        }

        threshold = threshold + step;
        level++;
    }

    *value = low + step * level;
    *index = level;
}

// VERIFY: fa2f699c manual
// OFFSET: 0x0089DFB0
void SPIRIT_IP_MR_V3_NAME_158(SPIRIT_IPMR_UNK158* state, float gain, float energy) {
    for (int16_t i = 4; i > 0; i--) {
        state->unk_0000[i] = state->unk_0000[i - 1];
    }

    state->unk_0000[0] = gain;
    int16_t level = 2;

    if (gain < 0.6f) {
        level = gain > 0.9f ? 1 : 0;
    }

    if (state->unk_0014 + state->unk_0014 < energy) {
        state->unk_001A = 2;
    } else if (state->unk_001A > 0) {
        state->unk_001A--;
    }

    if (state->unk_001A) {
        if (level < 2) {
            level++;
        }
    } else {
        int16_t low = 0;

        for (int32_t i = 0; i < 5; i++) {
            if (0.6f > state->unk_0000[i]) {
                low++;
            }
        }

        if (low > 2) {
            state->unk_0014 = energy;
            state->unk_0018 = 0;
            return;
        }

        if (level > state->unk_0018 + 1) {
            state->unk_0014 = energy;
            state->unk_0018 = level - 1;
            return;
        }

        if (level < state->unk_0018 - 1) {
            level++;
        }
    }

    state->unk_0018 = level;
    state->unk_0014 = energy;
}

}
