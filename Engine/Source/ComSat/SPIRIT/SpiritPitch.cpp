#include <cmath>
#include <cstdint>
#include "Engine/Source/ComSat/SPIRIT/SpiritFloat.h"
#include "Engine/Source/ComSat/SPIRIT/ipmr.h"

extern "C" {

// STATIC: 0x00A58EB0
const float s_ipmrTableA58EB0[48] = {
    0.898528993f, 0.0959167033f, -0.0849303976f, 0.0689392015f, -0.0508423001f, 0.0335387997f,
    -0.0193480998f, 0.00939941034f, -0.00366211007f, 0.00103759998f, 0.0f, 0.0f,
    0.769257009f, -0.134338006f, 0.0369567983f, 0.0f, -0.0141602f, 0.0167846996f,
    -0.0137633998f, 0.00903320033f, -0.00503539992f, 0.00277710008f, 0.0f, 0.0f,
    0.448639005f, -0.178527996f, 0.0955199972f, -0.0504150018f, 0.0230713002f, -0.0074768099f,
    0.0f, 0.00238036993f, -0.00241089007f, 0.00213623f, 0.0f, 0.0f,
    0.0959167033f, -0.0849303976f, 0.0689392015f, -0.0508423001f, 0.0335387997f, -0.0193480998f,
    0.00939941034f, -0.00366211007f, 0.00103759998f, 0.0f, 0.0f, 0.0f,
};

// STATIC: 0x00A58E70
const float s_ipmrTableA58E70[16] = {
    0.900847971f, 0.76007098f, 0.424071997f, 0.0840758979f, -0.105559997f, -0.121123999f,
    -0.0476378985f, 0.0162963998f, 0.0312194992f, 0.0157471001f, 0.0f, -0.00592041016f,
    0.0f, 0.0f, 0.0f, 0.0f,
};

// STATIC: 0x00B236B0
const float s_ipmrTableB236B0[16] = {
    0.0f, 0.200012207f, 0.400146484f, 0.5f, 0.599975586f, 0.700012207f,
    0.75f, 0.799987793f, 0.849975586f, 0.900024414f, 0.950012207f, 1.0f,
    1.04998779f, 1.09997559f, 1.15002441f, 1.20001221f,
};

// VERIFY: 8d127739 manual
// OFFSET: 0x0089EDE0
int16_t SPIRIT_IP_MR_V3_NAME_111(const float* x) {
    float best = -3.4028235e38f;
    int32_t lag = 143;

    for (int32_t t = 143; t >= 80; t--) {
        float sum = 0.0f;

        for (int32_t sf = 0; sf < 2; sf++) {
            sum = SPIRIT_IP_MR_V3_NAME_42(&x[40 * sf], &x[40 * sf - t], 40) + sum;
        }

        if (best <= sum) {
            best = sum;
            lag = t;
        }
    }

    float energy = SPIRIT_IP_MR_V3_NAME_42(&x[-lag], &x[-lag], 80);
    float normalized = 0.0f;

    if (0.0f < energy) {
        normalized = Spirit::PitchInverseRoot(energy) * best;
    }

    float best2 = -3.4028235e38f;
    int32_t lag2 = 79;

    for (int32_t t = 79; t >= 40; t--) {
        float sum = 0.0f;

        for (int32_t sf = 0; sf < 2; sf++) {
            sum = SPIRIT_IP_MR_V3_NAME_42(&x[40 * sf], &x[40 * sf - t], 40) + sum;
        }

        if (best2 <= sum) {
            best2 = sum;
            lag2 = t;
        }
    }

    energy = SPIRIT_IP_MR_V3_NAME_42(&x[-lag2], &x[-lag2], 80);

    if (0.0f < energy) {
        best2 = Spirit::PitchInverseRoot(energy) * best2;

        if (best2 > normalized * static_cast<double>(0.85f)) {
            normalized = best2;
            lag = lag2;
        }
    }

    float best3 = -3.4028235e38f;
    int32_t lag3 = 39;

    for (int32_t t = 39; t >= 20; t--) {
        float sum = 0.0f;

        for (int32_t sf = 0; sf < 2; sf++) {
            sum = SPIRIT_IP_MR_V3_NAME_42(&x[40 * sf], &x[40 * sf - t], 40) + sum;
        }

        if (best3 <= sum) {
            best3 = sum;
            lag3 = t;
        }
    }

    energy = SPIRIT_IP_MR_V3_NAME_42(&x[-lag3], &x[-lag3], 80);

    if (0.0f < energy) {
        float normalized3 = Spirit::PitchInverseRoot(energy) * best3;

        if (normalized3 > normalized * static_cast<double>(0.85f)) {
            return static_cast<int16_t>(lag3);
        }
    }

    return static_cast<int16_t>(lag);
}

// VERIFY: cb2a9a1a manual
// OFFSET: 0x0089F0B0
void SPIRIT_IP_MR_V3_PitchCorrelationFiltered(float* out, const float* excitation, const float* target, const float* h, int16_t first, int16_t last) {
    const float* past = &excitation[-first];
    float y[40];

    for (int32_t n = 0; n < 40; n++) {
        float sum = 0.0f;

        for (int32_t k = 0; k <= n; k++) {
            sum += past[k] * h[n - k];
        }

        y[n] = sum;
    }

    for (int32_t lag = first; lag <= last; lag++) {
        float energy = 0.0f;
        float cross = 0.0f;

        for (int32_t i = 0; i < 40; i++) {
            energy += y[i] * y[i];
            cross += target[i] * y[i];
        }

        if (energy != 0.0f) {
            cross = cross * Spirit::PitchInverseRoot(energy);
        }

        out[lag] = cross;

        if (lag != last) {
            past--;

            for (int32_t n = 39; n > 0; n--) {
                y[n] = h[n] * past[0] + y[n - 1];
            }

            y[0] = past[0];
        }
    }
}

// VERIFY: 5ca6db75 manual
// OFFSET: 0x0089F3B0
void SPIRIT_IP_MR_V3_PitchCorrelationNormalized(int16_t first, const float* excitation, const float* target, int16_t last, float* out) {
    const float* past = &excitation[-first];
    float energy = SPIRIT_IP_MR_V3_NAME_42(past, past, 40);

    for (int32_t lag = first; lag <= last; lag++) {
        float cross = SPIRIT_IP_MR_V3_NAME_42(target, past, 40);

        if (energy == 0.0f) {
            out[lag] = cross;
        } else {
            out[lag] = Spirit::PitchInverseRoot(energy) * cross;
        }

        if (lag != last) {
            energy = energy - past[39] * past[39];
            past--;
            energy = past[0] * past[0] + energy;
        }
    }
}

// VERIFY: 9bdc2bf2 manual
// OFFSET: 0x0089F470
void SPIRIT_IP_MR_V3_RefineFractionalPitch(const float* correlation, int16_t* lag, int16_t* fraction, int16_t last) {
    float best = -3.4028235e38f;
    const float* x1 = &correlation[*lag];
    const float* x0 = x1 - 1;
    int32_t c = 1;
    int32_t e = 2;

    for (int32_t f = *fraction; f <= last; f++) {
        if (!f) {
            x0++;
            x1++;
            c = 0;
            e = 3;
        }

        float sum = x1[0] * s_ipmrTableA58E70[e] + s_ipmrTableA58E70[c] * x0[0] + 0.0f;
        sum = s_ipmrTableA58E70[e + 3] * x1[1] + s_ipmrTableA58E70[c + 3] * x0[-1] + sum;
        sum = sum + (s_ipmrTableA58E70[e + 6] * x1[2] + s_ipmrTableA58E70[c + 6] * x0[-2]);
        sum = sum + (s_ipmrTableA58E70[e + 9] * x1[3] + s_ipmrTableA58E70[c + 9] * x0[-3]);
        c++;
        e--;

        if (best < sum) {
            best = sum;
            *fraction = static_cast<int16_t>(f);
        }
    }

    if (*fraction == -2) {
        *fraction = 1;
        (*lag)--;
    }

    if (*fraction == 2) {
        *fraction = -1;
        (*lag)++;
    }
}

// VERIFY: cf503282 manual
// OFFSET: 0x0089F580
int16_t SPIRIT_IP_MR_V3_QuantizePitchGain(float* gain) {
    int32_t i;

    for (i = 1; i < 15; i++) {
        if (*gain <= s_ipmrTableB236B0[i]) {
            break;
        }
    }

    if (fabsf(*gain - s_ipmrTableB236B0[i - 1]) < fabsf(*gain - s_ipmrTableB236B0[i])) {
        i--;
    }

    *gain = s_ipmrTableB236B0[i];
    return static_cast<int16_t>(i);
}

// VERIFY: 2a4468a7 manual
// OFFSET: 0x0089F5E0
void SPIRIT_IP_MR_V3_NAME_79(float* excitation, int16_t lag, int16_t fraction) {
    const float* x = &excitation[-lag];
    int16_t f = -fraction;

    if (f < 0) {
        f += 3;
        x--;
    }

    const float* left = &s_ipmrTableA58EB0[12 * f];
    const float* right = &s_ipmrTableA58EB0[36 - 12 * f];

    for (int32_t n = 0; n < 40; n++) {
        float sum = 0.0f;

        for (int32_t i = 0; i < 10; i++) {
            sum += x[n - i] * left[i];
        }

        for (int32_t j = 0; j < 10; j++) {
            sum += x[n + 1 + j] * right[j];
        }

        excitation[n] = sum;
    }
}

// VERIFY: 43430781 manual
// OFFSET: 0x0089F6E0
void SPIRIT_IP_MR_V3_NAME_34(float* excitation, int16_t lag, int16_t fraction, float gain) {
    SPIRIT_IP_MR_V3_NAME_79(excitation, lag, fraction);

    for (int32_t i = 0; i < 40; i++) {
        excitation[i] = gain * excitation[i];
    }
}

// VERIFY: e7b41862 manual
// OFFSET: 0x0089F760
void SPIRIT_IP_MR_V3_NAME_153(int16_t index, int16_t* previous, int16_t* lag, int16_t* fraction, int16_t mode) {
    if (!mode) {
        if (index < 197) {
            *lag = static_cast<int16_t>(((10923 * (index + 2)) >> 15) + 19);
            *fraction = static_cast<int16_t>(index - 3 * *lag + 58);
        } else {
            *lag = index - 112;
            *fraction = 0;
        }
    } else if (mode == 1) {
        int16_t low = *previous - 10;

        if (low < 20) {
            low = 20;
        }

        if (static_cast<int16_t>(low + 19) > 143) {
            low = 124;
        }

        int16_t step = static_cast<int16_t>(((10923 * (index + 2)) >> 15) - 1);
        *lag = low + step;
        *fraction = static_cast<int16_t>(index - 3 * step - 2);
    } else {
        int16_t low = *previous - 2;

        if (low < 20) {
            low = 20;
        }

        *lag = static_cast<int16_t>(index / 3 + low);
        *fraction = static_cast<int16_t>(index % 3 - 1);
    }

    *previous = *lag;
}

// VERIFY: 7006c5e4 manual
// OFFSET: 0x0089F870
void SPIRIT_IP_MR_V3_SearchClosedLoopPitch(int16_t open, int16_t* previous, SPIRIT_IPMR_SUBFRAME* subframe, const float* excitation, const float* target, const float* h, int16_t mode) {
    int16_t fraction = -2;
    int32_t fine = 1;
    int16_t low;
    int16_t high;

    if (!mode) {
        low = open - 3;
        fine = 0;

        if (low < 20) {
            low = 20;
        }

        high = low + 6;

        if (high > 143) {
            low = 137;
            high = 143;
        }
    } else if (mode == 1) {
        low = *previous - 10;

        if (low < 20) {
            low = 20;
        }

        high = low + 19;

        if (high > 143) {
            low = 124;
            high = 143;
        }
    } else {
        low = open - 2;
        high = open + 2;

        if (low < 20) {
            low = 20;
        }

        if (high > 143) {
            high = 143;
        }
    }

    // Index by actual lag, including four interpolation taps at either end.
    // The x86 stack-address trick formed a pointer before its array even when
    // the later indexed accesses landed in storage; that is undefined C++.
    float correlation[148];

    if (h) {
        SPIRIT_IP_MR_V3_PitchCorrelationFiltered(correlation, excitation, target, h, low - 4, high + 4);
    } else {
        SPIRIT_IP_MR_V3_PitchCorrelationNormalized(low - 4, excitation, target, high + 4, correlation);
    }

    int16_t lag = low;
    float best = correlation[low];

    for (int16_t t = low + 1; t <= high; t++) {
        if (best < correlation[t]) {
            best = correlation[t];
            lag = t;
        }
    }

    if (!fine && lag > 84) {
        fraction = 0;
    } else {
        SPIRIT_IP_MR_V3_RefineFractionalPitch(correlation, &lag, &fraction, 2);

        if (mode == 2) {
            if (lag < low) {
                lag = low;
                fraction = -1;
            } else if (lag > high) {
                lag = high;
                fraction = 1;
            }
        }
    }

    if (h) {
        if (!mode) {
            if (lag <= 85) {
                subframe->unk_0008 = static_cast<int16_t>(3 * lag + fraction - 58);
            } else {
                subframe->unk_0008 = lag + 112;
            }
        } else if (mode == 1) {
            subframe->unk_0008 = static_cast<int16_t>(3 * (lag - low) + fraction + 2);
        } else {
            subframe->unk_0008 = static_cast<int16_t>(3 * (lag - low) + fraction + 1);
        }
    }

    *previous = lag;
    subframe->unk_0002 = fraction;
    subframe->unk_0000 = lag;
}

// VERIFY: 9c76d169 manual
// OFFSET: 0x0089FAE0
void SPIRIT_IP_MR_V3_NAME_14(int16_t* previous, int16_t open, SPIRIT_IPMR_SUBFRAME* subframe, const float* h, float* excitation, float* target, int16_t mode) {
    SPIRIT_IP_MR_V3_SearchClosedLoopPitch(open, previous, subframe, excitation, target, h, mode);
    SPIRIT_IP_MR_V3_NAME_79(excitation, subframe->unk_0000, subframe->unk_0002);
    float filtered[40];

    for (int16_t n = 0; n < 40; n++) {
        float sum = 0.0f;

        for (int16_t k = 0; k <= n; k++) {
            sum = h[n - k] * excitation[k] + sum;
        }

        filtered[n] = sum;
    }

    double cross = SPIRIT_IP_MR_V3_NAME_42(target, filtered, 40);
    float gain = static_cast<float>(cross / (SPIRIT_IP_MR_V3_NAME_42(filtered, filtered, 40) + static_cast<double>(0.01f)));

    if (0.0f > gain) {
        gain = 0.0f;
    }

    int16_t index = SPIRIT_IP_MR_V3_QuantizePitchGain(&gain);
    subframe->unk_0004 = gain;
    subframe->unk_000A = index;

    for (int32_t i = 0; i < 40; i++) {
        target[i] = target[i] - filtered[i] * gain;
        excitation[i] = excitation[i] * gain;
    }
}

// VERIFY: 32997665 auto
// OFFSET: 0x0089FCF0
void SPIRIT_IP_MR_V3_NAME_15(int16_t* previous, int16_t open, SPIRIT_IPMR_SUBFRAME* subframe, float* excitation, const float* target, int16_t mode) {
    SPIRIT_IP_MR_V3_SearchClosedLoopPitch(open, previous, subframe, excitation, target, nullptr, mode);
    SPIRIT_IP_MR_V3_NAME_79(excitation, subframe->unk_0000, subframe->unk_0002);
    double cross = SPIRIT_IP_MR_V3_NAME_42(target, excitation, 40);
    float gain = static_cast<float>(cross / (SPIRIT_IP_MR_V3_NAME_42(excitation, excitation, 40) + static_cast<double>(0.01f)));

    if (0.0f > gain) {
        gain = 0.0f;
    }

    gain = static_cast<float>(sqrt(gain));
    subframe->unk_0004 = gain;
    SPIRIT_IP_MR_V3_NAME_248(excitation, gain);
}

}
