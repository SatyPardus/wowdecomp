#include <cstdint>
#include "Engine/Source/ComSat/SPIRIT/ipmr.h"
#include "Engine/Source/ComSat/SPIRIT/SpiritStream.h"

extern "C" {

// VERIFY: 01f10cbd auto
// OFFSET: 0x00896100
int32_t SPIRIT_IP_MR_V3_1_GenerateTables(void* tables) {
    if (!tables) {
        return 1;
    }

    SPIRIT_IP_MR_V3_NAME_201(reinterpret_cast<SPIRIT_IPMR_TABLES*>((reinterpret_cast<uintptr_t>(tables) + 16) & ~static_cast<uintptr_t>(15)));
    return 0;
}

// VERIFY: 6c316bba auto
// OFFSET: 0x00896120
int32_t SPIRIT_IP_MR_V3_1_Init(void* state, const void* tables) {
    if (!state || !tables) {
        return 1;
    }

    const SPIRIT_IPMR_TABLES* aligned = reinterpret_cast<const SPIRIT_IPMR_TABLES*>((reinterpret_cast<uintptr_t>(tables) + 16) & ~static_cast<uintptr_t>(15));
    int32_t result = 1;

    if (aligned->m_initialized == 1) {
        SPIRIT_IPMR_STATE* codec = reinterpret_cast<SPIRIT_IPMR_STATE*>((reinterpret_cast<uintptr_t>(state) + 16) & ~static_cast<uintptr_t>(15));
        result = SPIRIT_IP_MR_V3_NAME_197(&codec->m_encoder, aligned, 0, 0);

        if (!result) {
            result = SPIRIT_IP_MR_V3_NAME_194(&codec->m_decoder, aligned, 0, 0, 0);
        }
    }

    return result;
}

// VERIFY: 4faf2da5 auto
// OFFSET: 0x00896180
int32_t SPIRIT_IP_MR_V3_1_Encode(void* state, const uint8_t* pcm, SPIRIT_IPMR_PARAMS* params, SPIRIT_IPMR_STREAM* stream, SPIRIT_IPMR_INFO* info) {
    if (!state || !pcm || !params || !stream || !info) {
        return 1;
    }

    info->unk_0000 = 0;
    info->unk_0018 = 0;
    SPIRIT_IP_MR_V3_NAME_334(info->unk_0010, sizeof(info->unk_0010));
    SPIRIT_IP_MR_V3_NAME_334(info->unk_0002, sizeof(info->unk_0002));

    SPIRIT_IPMR_STATE* codec = reinterpret_cast<SPIRIT_IPMR_STATE*>((reinterpret_cast<uintptr_t>(state) + 16) & ~static_cast<uintptr_t>(15));
    try {
        return SPIRIT_IP_MR_V3_NAME_195(&codec->m_encoder, pcm, stream, params->unk_0000, 1, params->unk_0004, params->unk_0004, &params->unk_0006, info->unk_0002, info->unk_0010, &info->unk_0018, &info->unk_0000);
    } catch (const Spirit::InvalidStream&) {
        return 2;
    }
}

// VERIFY: 23dc633b manual
// OFFSET: 0x00896210
int32_t SPIRIT_IP_MR_V3_1_Decode(void* state, uint8_t* pcm, SPIRIT_IPMR_PARAMS* params, SPIRIT_IPMR_STREAM* stream, int32_t lost) {
    int16_t bits = 0;

    if (!state || !pcm || !params || !stream) {
        return 1;
    }

    SPIRIT_IPMR_STATE* codec = reinterpret_cast<SPIRIT_IPMR_STATE*>((reinterpret_cast<uintptr_t>(state) + 16) & ~static_cast<uintptr_t>(15));
    try {
        return SPIRIT_IP_MR_V3_NAME_192(&codec->m_decoder, stream, pcm, 0, 0, params->unk_0000, 1, params->unk_0004, params->unk_0004, lost, &bits);
    } catch (const Spirit::InvalidStream&) {
        return 2;
    }
}

// VERIFY: bcb46639 auto
// OFFSET: 0x008967B0
void SPIRIT_IP_MR_V3_ResetDecoderState(SPIRIT_IPMR_DEC_UNK0020* state, const SPIRIT_IPMR_TABLES* tables) {
    SPIRIT_IP_MR_V3_NAME_334(state, sizeof(SPIRIT_IPMR_DEC_UNK0020));
    state->unk_03B4 = &state->unk_0000[154];
    SPIRIT_IP_MR_V3_NAME_190(state->unk_0310, s_ipmrTableB234D0);
    SPIRIT_IP_MR_V3_NAME_190(state->unk_0340, s_ipmrTableB234D0);
    state->unk_03B8.unk_02D0 = 0.83f;
    state->unk_03B8.unk_02D4 = 0.9f;
    SPIRIT_IP_MR_V3_NAME_114(&state->unk_06A8, tables->unk_B7C0);
}

// VERIFY: 1581115b auto
// OFFSET: 0x00896820
int32_t SPIRIT_IP_MR_V3_NAME_194(SPIRIT_IPMR_DECODER* decoder, const SPIRIT_IPMR_TABLES* tables, int32_t rateIn, int32_t rateOut, uint32_t mode) {
    if (!decoder || !tables) {
        return 1;
    }

    decoder->m_initialized = 0;
    decoder->m_tables = const_cast<SPIRIT_IPMR_TABLES*>(tables);

    if (mode > 3) {
        return 1;
    }

    decoder->unk_0010 = mode;
    decoder->unk_0008 = 8000;
    decoder->unk_0004 = SPIRIT_IP_MR_V3_NAME_66(rateIn);
    decoder->unk_000C = SPIRIT_IP_MR_V3_NAME_66(rateOut);

    if (!decoder->unk_0004 || !decoder->unk_000C) {
        return 1;
    }

    decoder->unk_001C = decoder->unk_0010 == 1;
    SPIRIT_IP_MR_V3_ResetDecoderState(&decoder->unk_0020, tables);
    SPIRIT_IP_MR_V3_NAME_44(&decoder->unk_0730);
    decoder->m_initialized = 1;
    return 0;
}

// VERIFY: 7154a6db manual
// OFFSET: 0x00896D10
void SPIRIT_IP_MR_V3_ResetEncoderState(SPIRIT_IPMR_ENC_UNK0D20* state) {
    SPIRIT_IP_MR_V3_NAME_334(state, sizeof(SPIRIT_IPMR_ENC_UNK0D20));
    state->unk_1F98 = &state->unk_0010[200];
    state->unk_1F9C = &state->unk_0010[160];
    state->unk_1FA4 = &state->unk_0A70[154];
    state->unk_1FA0 = &state->unk_05B0[143];
    state->unk_1FA8 = &state->unk_0F60[154];
    state->unk_1FAC = &state->unk_1450[154];
    SPIRIT_IP_MR_V3_NAME_189(state->unk_1960, s_ipmrTableB234D0, 10);
    SPIRIT_IP_MR_V3_NAME_189(state->unk_1938, s_ipmrTableB234D0, 10);
    state->unk_1F78 = 0.08f;
    SPIRIT_IP_MR_V3_NAME_189(state->unk_1988, s_ipmrTableB234D0, 10);
    SPIRIT_IP_MR_V3_NAME_189(state->unk_19B0, s_ipmrTableB234D0, 10);
}

// VERIFY: 3f75b507 auto
// OFFSET: 0x00896DD0
int32_t SPIRIT_IP_MR_V3_NAME_197(SPIRIT_IPMR_ENCODER* encoder, const SPIRIT_IPMR_TABLES* tables, int32_t rate, uint32_t mode) {
    encoder->m_tables = const_cast<SPIRIT_IPMR_TABLES*>(tables);
    encoder->m_initialized = 0;

    if (mode > 3) {
        return 1;
    }

    encoder->unk_0010 = mode;
    encoder->unk_000C = 8000;
    encoder->unk_0004 = SPIRIT_IP_MR_V3_NAME_66(rate);
    encoder->unk_0008 = encoder->unk_0004;

    if (!encoder->unk_0004) {
        return 1;
    }

    SPIRIT_IP_MR_V3_ResetEncoderState(&encoder->unk_0D20);
    SPIRIT_IP_MR_V3_NAME_334(encoder->unk_0014, sizeof(encoder->unk_0014));
    SPIRIT_IP_MR_V3_NAME_46(&encoder->unk_0020);
    encoder->m_initialized = 1;
    return 0;
}

// VERIFY: 39e4429d manual
// OFFSET: 0x00897A00
void SPIRIT_IP_MR_V3_NAME_46(SPIRIT_IPMR_ENC_UNK0020* state) {
    state->unk_0160 = 0;
    state->unk_0162 = 0;
    state->unk_0164 = 0;
    state->unk_0166 = 1;
}

// VERIFY: c029701d auto
// OFFSET: 0x00897A30
void SPIRIT_IP_MR_V3_NAME_44(SPIRIT_IPMR_DEC_UNK0730* state) {
    state->unk_0004 = 0.0f;
    state->unk_0084 = 0;
    state->unk_0088 = 0x1234;
    SPIRIT_IP_MR_V3_NAME_190(state->unk_0034, s_ipmrTableB234D0);
}

// VERIFY: 4d0c21eb manual
// OFFSET: 0x008974F0
void SPIRIT_IP_MR_V3_NAME_114(SPIRIT_IPMR_UNK114* state, const float* init) {
    state->unk_005A = 0;
    state->unk_005C = 0;
    state->unk_005E = 0;
    SPIRIT_IP_MR_V3_NAME_190(state->unk_0000, init);

    for (int32_t i = 0; i < 5; i++) {
        state->unk_0028[i] = 0.1f;
    }

    for (int32_t i = 0; i < 5; i++) {
        state->unk_0040[i] = 0.5f;
    }

    state->unk_0058 = 40;
    state->unk_0060 = 0x1234;
    state->unk_003C = 1.0f;
    state->unk_0054 = 0.5f;
}

// VERIFY: fde4045a manual
// OFFSET: 0x00897560
void SPIRIT_IP_MR_V3_NAME_144(SPIRIT_IPMR_UNK114* state, int16_t flag) {
    if (flag) {
        state->unk_005A++;
    } else {
        state->unk_005A = state->unk_005A == 6 ? 5 : 0;
    }

    if (state->unk_005A > 6) {
        state->unk_005A = 6;
    }
}

}
