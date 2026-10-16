#include <cmath>
#include <cstdint>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif
#include "Engine/Source/ComSat/SPIRIT/ipmr.h"

extern "C" {

// STATIC: 0x00B236F0
const uint8_t s_ipmrTableB236F0[8] = {
    0x00, 0x00, 0x00, 0x00, 0x14, 0x28, 0x42, 0x64,
};

// STATIC: 0x00B236F8
const uint8_t s_ipmrTableB236F8[10] = {
    0x00, 0x00, 0x00, 0x00, 0x14, 0x28, 0x42, 0x64, 0x82, 0x9C,
};

// STATIC: 0x00B23704
const int16_t s_ipmrTableB23704[16] = {
    0, 0, 0, 0, 20, 40, 66, 100, 130, 156, 180, 204, 228, 256, 276, 300,
};

// STATIC: 0x00B23724
const int32_t s_ipmrTableB23724[8] = {
    6000, 8350, 10600, 13900, 22600, 30200, 40000, 50800,
};

// VERIFY: 855e8083 manual
// OFFSET: 0x00897370
void SPIRIT_IP_MR_V3_PutBitsClear(int32_t value, SPIRIT_IPMR_STREAM* stream, int16_t count, int16_t skip) {
    if (skip) {
        stream->m_bits += count;
        return;
    }

    uint8_t* data = const_cast<uint8_t*>(stream->m_data);
    int16_t index = static_cast<int16_t>(stream->m_bits) >> 3;
    int32_t shift = static_cast<int32_t>(stream->m_bits) % 8;

    if (!shift) {
        data[index] = 0;
    }

    int16_t remaining = count;
    int16_t bits = 8 - shift < count ? static_cast<int16_t>(8 - shift) : count;

    while (true) {
        data[index] |= static_cast<uint8_t>(((1 << bits) - 1) & value) << shift;
        stream->m_bits += bits;
        remaining -= bits;

        if (remaining <= 0) {
            break;
        }

        value >>= bits;
        index++;
        data[index] = 0;
        bits = remaining > 8 ? 8 : remaining;
        shift = 0;
    }
}

// VERIFY: 7a8f786f manual
// OFFSET: 0x00897440
void SPIRIT_IP_MR_V3_CopyBits(SPIRIT_IPMR_STREAM* stream, const uint8_t* src, int16_t start, int16_t count) {
    uint8_t* data = const_cast<uint8_t*>(stream->m_data);
    data[static_cast<int32_t>(stream->m_bits) >> 3] &= 0xFF >> (8 - static_cast<int32_t>(stream->m_bits) % 8);
    int32_t end = start + count;

    for (int16_t i = start; i < end; i++) {
        if (!(static_cast<int32_t>(stream->m_bits) % 8)) {
            data[static_cast<int32_t>(stream->m_bits) >> 3] = 0;
        }

        data[static_cast<int32_t>(stream->m_bits) >> 3] |= ((src[i >> 3] >> (i & 7)) & 1) << (stream->m_bits & 7);
        stream->m_bits++;
    }
}

// VERIFY: 58f766f0 manual
// OFFSET: 0x00898030
int32_t SPIRIT_IP_MR_V3_NAME_323(int32_t mode, int32_t rate, int16_t a3, int16_t a4, SPIRIT_IPMR_FRAME* frame) {
    int32_t result = 1;
    frame->unk_02AA = 1;
    frame->unk_9420 = 1;
    frame->unk_02A0 = 1;
    frame->unk_02A6 = 1;

    switch (mode) {
        case 1:
            frame->unk_02A0 = 0;
            frame->unk_02AA = 0;

        case 0:
            frame->unk_02A6 = 0;

        case 2:
        case 3: {
            if (rate == 8000) {
                frame->unk_02A0 = 0;
            }

            int32_t factor = rate / 8000;
            int16_t samples = static_cast<int16_t>(factor * 160);
            frame->unk_02A4 = static_cast<int16_t>(factor);
            frame->unk_0298 = samples;
            int32_t length = mode == 0 || mode == 2 ? 80 : 160;
            int16_t count = static_cast<int16_t>(samples / static_cast<int16_t>(factor * length));
            frame->unk_0292 = 0;
            frame->unk_0296 = 0;
            frame->unk_02B0 = 0;
            frame->unk_9426 = count;
            frame->unk_02A2 = frame->unk_02A0;
            frame->unk_02A8 = frame->unk_02A6;
            frame->unk_9424 = static_cast<int16_t>(samples / count);
            frame->unk_02AC = frame->unk_02AA;
            frame->unk_9422 = 1;

            if (a4) {
                frame->unk_0296 = 1;
                frame->unk_0292 = 1;
                frame->unk_02B0 = 2;
            }

            if (a3) {
                frame->unk_0292 = 2;
            }

            if (frame->unk_0292 != 1) {
                frame->m_rate = rate;
                frame->m_mode = mode;
                result = 0;
            }

            break;
        }
    }

    return result;
}

// VERIFY: d9ff397b manual
// OFFSET: 0x008981A0
int16_t SPIRIT_IP_MR_V3_NAME_387(SPIRIT_IPMR_FRAME* frame) {
    int32_t bits = 0;

    switch (frame->m_rate) {
        case 8000:
            bits = s_ipmrTableB236F0[7];
            break;

        case 16000:
            bits = s_ipmrTableB236F8[9];
            break;

        case 32000:
            bits = s_ipmrTableB23704[15];
            break;
    }

    return static_cast<int16_t>(s_ipmrTableB23724[3] / (400 * frame->unk_9426) + static_cast<int16_t>(bits) / frame->unk_9426);
}

// VERIFY: d795f91a manual
// OFFSET: 0x00898220
int16_t SPIRIT_IP_MR_V3_NAME_61(int32_t index, SPIRIT_IPMR_FRAME* frame) {
    int16_t bits = 0;

    switch (frame->m_rate) {
        case 8000:
            bits = s_ipmrTableB236F0[index];
            break;

        case 16000:
            bits = s_ipmrTableB236F8[index];
            break;

        case 32000:
            bits = s_ipmrTableB23704[index];
            break;
    }

    int16_t result = static_cast<int16_t>(bits / frame->unk_9426);

    if (frame->unk_0288 || frame->unk_02B0 == 3) {
        int32_t rate = index <= 3 ? s_ipmrTableB23724[index] : s_ipmrTableB23724[3];
        result += static_cast<int16_t>(rate / (400 * frame->unk_9426));
    }

    return result;
}

// VERIFY: 9fb000dd manual
// OFFSET: 0x008982C0
int32_t SPIRIT_IP_MR_V3_NAME_309(int32_t index, SPIRIT_IPMR_FRAME* frame) {
    if (index < 0) {
        return 1;
    }

    if (frame->m_rate == 8000) {
        if (index >= 8) {
            return 1;
        }
    } else if (frame->m_rate == 16000) {
        if (index >= 10) {
            return 1;
        }
    } else if (frame->m_rate == 32000 && index >= 16) {
        return 1;
    }

    switch (frame->unk_0288) {
        case 0:
            frame->unk_02AC = 1;

            if (index <= 3) {
                frame->unk_02AE = static_cast<int16_t>(index + 1);
                frame->unk_9422 = 0;
            } else {
                frame->unk_02AE = 4;
                frame->unk_9422 = 1;
            }

            break;

        case 1:
            frame->unk_02AC = 0;
            frame->unk_9422 = 1;
            frame->unk_02AE = 0;
            break;

        case 2:
        case 3:
            frame->unk_02AC = 1;
            frame->unk_9422 = 1;

            if (index <= 3) {
                frame->unk_02AE = static_cast<int16_t>(index + 1);
            } else {
                frame->unk_02AE = 4;
            }

            break;

        default:
            return 3;
    }

    int16_t bits = SPIRIT_IP_MR_V3_NAME_387(frame);
    frame->unk_942A = 0;

    if (bits) {
        int16_t count = 0;

        do {
            count++;
            bits >>= 1;
        } while (bits);

        frame->unk_942A = count;
    }

    frame->unk_9428 = SPIRIT_IP_MR_V3_NAME_61(index, frame);
    frame->unk_028C = index;
    return 0;
}

// VERIFY: 4cd77cdf auto
// OFFSET: 0x00898420
void SPIRIT_IP_MR_V3_NAME_311(SPIRIT_IPMR_FRAME* frame) {
    if (frame->m_mode == 0) {
        frame->unk_0288 = 0;
    } else if (frame->m_mode == 1) {
        frame->unk_0288 = 1;
    }
}

// VERIFY: b17cbb1e manual
// OFFSET: 0x00898450
float SPIRIT_IP_MR_V3_SquaredDistance10(const float* a, const float* b) {
    float sum = 0.0f;

    for (int32_t i = 0; i < 10; i++) {
        float d = a[i] - b[i];
        sum = d * d + sum;
    }

    return sum;
}

// VERIFY: 6534c739 auto
// OFFSET: 0x00898540
void SPIRIT_IP_MR_V3_NAME_85(SPIRIT_IPMR_ENC_UNK0D20* state, const float* input) {
    SPIRIT_IP_MR_V3_NAME_189(state->unk_0010, &state->unk_0010[160], 200);
    SPIRIT_IP_MR_V3_NAME_189(state->unk_1F98, input, 160);
    SPIRIT_IP_MR_V3_NAME_189(state->unk_05B0, &state->unk_05B0[160], 143);
    SPIRIT_IP_MR_V3_NAME_189(state->unk_0A70, &state->unk_0A70[160], 154);
    SPIRIT_IP_MR_V3_NAME_189(state->unk_0F60, &state->unk_0F60[160], 154);
    SPIRIT_IP_MR_V3_NAME_189(state->unk_1450, &state->unk_1450[160], 154);
}

// VERIFY: 7282632a manual
// OFFSET: 0x008985E0
void SPIRIT_IP_MR_V3_HighPassFilter(float* memory, const float* input, float* output) {
    float y = 0.0f;

    for (int32_t i = 0; i < 160; i++) {
        float x2 = memory[1];
        memory[1] = memory[0];
        memory[0] = input[i];
        y = static_cast<float>(memory[0] * 0.9780304792065596 - memory[1] * 1.956060958413119 + x2 * 0.9780304792065596 + memory[2] * 1.955578240315035 - memory[3] * 0.9565436765112032);
        output[i] = y;
        memory[3] = memory[2];
        memory[2] = y;
    }

    if (fabs(y) + fabs(memory[3]) < 1.0e-10) {
        memory[2] = 0.0f;
        memory[3] = 0.0f;
    }
}

// VERIFY: bfacf75f manual
// OFFSET: 0x00898940
void SPIRIT_IP_MR_V3_NAME_199(const int16_t* pcm, float* dst, int16_t count) {
    int32_t blocks = count / 4;

    for (int32_t i = 0; i < blocks * 4; i++) {
        dst[i] = pcm[i];
    }
}

// VERIFY: 57f64a0d manual
// OFFSET: 0x00898C60
void SPIRIT_IP_MR_V3_NAME_200(const float* src, int16_t* pcm, int16_t count) {
    int32_t blocks = count / 4;

    for (int32_t i = 0; i < blocks * 4; i++) {
#if defined(__SSE__)
        int32_t value = _mm_cvtss_si32(_mm_set_ss(src[i]));
#else
        int32_t value = static_cast<int32_t>(std::lrint(src[i]));
#endif

        if (value > 32767) {
            value = 32767;
        } else if (value < -32768) {
            value = -32768;
        }

        pcm[i] = static_cast<int16_t>(value);
    }
}

}
