#include <cstdint>
#include "Engine/Source/ComSat/SPIRIT/ipmr.h"
#include "Engine/Source/ComSat/SPIRIT/SpiritStream.h"

extern "C" {

// STATIC: 0x00B235A0
const uint8_t s_ipmrTableB235A0[11] = {
    0x01, 0x01, 0x01, 0x02, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
};

// STATIC: 0x00B235D8
const uint8_t s_ipmrTableB235D8[12] = {
    0x06, 0x06, 0x06, 0x0A, 0x13, 0x12, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00,
};

// STATIC: 0x00B235E4
const uint8_t s_ipmrTableB235E4[12] = {
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x00,
};

// STATIC: 0x00B235F0
const uint8_t s_ipmrTableB235F0[12] = {
    0x01, 0x01, 0x01, 0x02, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x00,
};

// STATIC: 0x00B23754
const uint8_t s_ipmrTableB23754[4] = {
    0x07, 0x07, 0x07, 0x06,
};

// STATIC: 0x00B23768
const uint8_t s_ipmrTableB23768[4] = {
    0x07, 0x07, 0x06, 0x01,
};

// STATIC: 0x00B235FC
const uint8_t s_ipmrTableB235FC[11][4] = {
    { 0x00, 0x01, 0x00, 0x01 },
    { 0x00, 0x01, 0x00, 0x01 },
    { 0x00, 0x00, 0x00, 0x03 },
    { 0x00, 0x01, 0x00, 0x01 },
    { 0x00, 0x01, 0x00, 0x01 },
    { 0x00, 0x00, 0x00, 0x03 },
    { 0x00, 0x01, 0x00, 0x01 },
    { 0x00, 0x01, 0x00, 0x01 },
    { 0x00, 0x01, 0x00, 0x01 },
    { 0x00, 0x01, 0x00, 0x01 },
    { 0x00, 0x01, 0x00, 0x01 },
};

// STATIC: 0x00B23628
const uint32_t s_ipmrTableB23628[33] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007, 0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F,
    0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF,
    0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
    0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF,
    0xFFFFFFFF,
};

// VERIFY: 0f7acabc manual
// OFFSET: 0x008998E0
int32_t SPIRIT_IP_MR_V3_GetBits(SPIRIT_IPMR_STREAM* stream, int16_t count) {
    Spirit::RequireBits(stream, count);
    if (count > 32) throw Spirit::InvalidStream{};
    if (!count) return 0;
    int32_t result = 0;
    uint32_t index = stream->m_bits >> 3;
    int32_t shift = static_cast<int32_t>(stream->m_bits) % 8;
    int16_t remaining = count;
    int16_t bits = 8 - shift < count ? static_cast<int16_t>(8 - shift) : count;
    int32_t position = 0;

    while (true) {
        uint32_t value = stream->m_data[index];
        stream->m_bits += bits;
        remaining -= bits;
        result |= ((value >> shift) & ((1u << bits) - 1)) << position;

        if (remaining <= 0) {
            break;
        }

        position += bits;
        bits = remaining > 8 ? 8 : remaining;
        shift = 0;
        index++;
    }

    return result;
}

// VERIFY: 14a25117 auto
// OFFSET: 0x00899990
void SPIRIT_IP_MR_V3_ReadFrameHeader(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream) {
    if (frame->unk_02A8) {
        frame->unk_0288 = SPIRIT_IP_MR_V3_GetBits(stream, 2);
    } else if (frame->m_mode == 0) {
        frame->unk_0288 = 0;
    } else if (frame->m_mode == 1) {
        frame->unk_0288 = 1;
    }

    if (frame->unk_02B0) {
        int16_t flag = static_cast<int16_t>(SPIRIT_IP_MR_V3_GetBits(stream, 1));
        frame->unk_0294 = flag;
        frame->unk_02B0 = 2 * (flag == 0) + 1;
    } else {
        frame->unk_0294 = 1;
    }
}

// VERIFY: af3a02c9 manual
// OFFSET: 0x00899EE0
void SPIRIT_IP_MR_V3_ValidateStreamIndex(int16_t count, int16_t* index) {
    if (count == 0) {
        return;
    }

    if (count == 1 || *index > count - 2) {
        *index = -1;
    }
}

// VERIFY: b2932813 manual
// OFFSET: 0x00899E60
void SPIRIT_IP_MR_V3_ReadFrameFlags(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream) {
    const uint8_t* widths = frame->unk_02B4 >= 0 || frame->unk_02B0 == 3 ? s_ipmrTableB23754 : s_ipmrTableB23768;

    for (int32_t i = 0; i < 4; i++) {
        frame->unk_02C8[i] = SPIRIT_IP_MR_V3_GetBits(stream, widths[i]);
    }
}

// VERIFY: eba06dee manual
// OFFSET: 0x00899B80
int32_t SPIRIT_IP_MR_V3_NAME_68(int16_t kind, const SPIRIT_IPMR_FRAME* frame, int16_t index) {
    switch (kind) {
        case 0:
            if (frame->unk_02B0 == 3) {
                return 0;
            }

            return frame->unk_02B4 >= 0 ? 0 : 4;

        case 1:
            return frame->unk_02B6[index] != 0 ? 1 : 6;

        case 2:
            return frame->unk_02B6[index] == 0 ? 6 : 2;

        case 3:
            return frame->unk_02BE[index] == 0 ? 5 : 3;
    }

    return 6;
}

// VERIFY: 0bd3b66c manual
// OFFSET: 0x00899A80
int16_t SPIRIT_IP_MR_V3_NAME_152(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream) {
    int16_t start = static_cast<int16_t>(stream->m_bits);

    if (static_cast<uint32_t>(frame->unk_02B0) < 2) {
        for (int32_t i = 0; i < 4; i++) {
            frame->unk_02B6[i] = static_cast<int16_t>(SPIRIT_IP_MR_V3_GetBits(stream, 1));
            frame->unk_02BE[i] = static_cast<int16_t>(SPIRIT_IP_MR_V3_GetBits(stream, 1));
        }
    } else {
        frame->unk_02B6[3] = 0;
        frame->unk_02B6[2] = 0;
        frame->unk_02B6[1] = 0;
        frame->unk_02B6[0] = 0;
        frame->unk_02BE[3] = 0;
        frame->unk_02BE[2] = 0;
        frame->unk_02BE[1] = 0;
        frame->unk_02BE[0] = 0;
    }

    if (frame->unk_02B0 == 3) {
        frame->unk_02B4 = 2;
    } else {
        frame->unk_02B4 = static_cast<int16_t>((SPIRIT_IP_MR_V3_GetBits(stream, 2) - 1) % 4);

        if (frame->unk_02B4 == 3) {
            frame->unk_02B4 = -1;
        }
    }

    return static_cast<int16_t>(stream->m_bits - start);
}

// VERIFY: 69ac0237 manual
// OFFSET: 0x0089A0B0
int16_t SPIRIT_IP_MR_V3_NAME_67(const SPIRIT_IPMR_FRAME* frame, int16_t* sizes, int16_t* primary) {
    *primary = 1;

    for (int32_t i = 0; i < 7; i++) {
        sizes[i] = 0;
    }

    int16_t header = 0;

    if (frame->unk_02B0 == 4) {
        return 0;
    }

    if (frame->unk_02B0 == 0 || (header = 1, frame->unk_02B0 == 1)) {
        header += 8;
    }

    if (frame->unk_02B0 != 3) {
        header += 2;
    }

    if (*primary) {
        sizes[0] += header;
    } else {
        sizes[1] += header;
    }

    const uint8_t* widths = frame->unk_02B4 >= 0 || frame->unk_02B0 == 3 ? s_ipmrTableB23754 : s_ipmrTableB23768;
    int16_t lsf = 0;

    for (int32_t i = 0; i < 4; i++) {
        lsf += widths[i];
    }

    sizes[SPIRIT_IP_MR_V3_NAME_68(0, frame, 0)] += lsf;

    if (frame->unk_02B0 == 3) {
        sizes[0] += 5;
        return sizes[0];
    }

    int16_t toggle = 0;

    for (int16_t i = 0; i < 4; i++) {
        int16_t next = 1 - toggle;

        if (frame->unk_02B6[i]) {
            const int16_t* previous = &frame->unk_02B4;
            int16_t bits = next ? 8 : static_cast<int16_t>(2 * (previous[i] == 0) + 6);
            sizes[SPIRIT_IP_MR_V3_NAME_68(1, frame, i)] += bits;
            sizes[SPIRIT_IP_MR_V3_NAME_68(2, frame, i)] += 4;
        }

        toggle = next;
    }

    for (int16_t i = 0; i < 4; i++) {
        int16_t bits = frame->unk_02BE[i] ? 26 : static_cast<int16_t>(s_ipmrTableB235D8[0] + s_ipmrTableB235E4[0] + s_ipmrTableB235F0[0]);
        sizes[SPIRIT_IP_MR_V3_NAME_68(3, frame, i)] += bits;
    }

    int16_t total = sizes[0];

    for (int32_t i = 1; i < 7; i++) {
        total += sizes[i];
    }

    return total;
}

// VERIFY: a5cfc0b4 manual
// OFFSET: 0x0089A78E
int16_t SPIRIT_IP_MR_V3_NAME_155(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream, int16_t lost) {
    SPIRIT_IPMR_STREAM streams[7];
    streams[0] = *stream;
    int16_t sizes[8];
    int16_t primary;
    SPIRIT_IP_MR_V3_NAME_67(frame, sizes, &primary);
    int16_t secondary = primary == 0;

    if (frame->unk_02B0 == 0 || frame->unk_02B0 == 1) {
        sizes[secondary] -= 8;
    }

    if (frame->unk_02B0 != 3) {
        sizes[secondary] -= 2;
    }

    if (lost >= 2) {
        SPIRIT_IP_MR_V3_NAME_334(&sizes[lost - 1], 2 * (8 - lost));
    }

    for (int32_t i = 0; i < 6; i++) {
        streams[i + 1].m_data = stream->m_data;
        streams[i + 1].m_bits = streams[i].m_bits + sizes[i];
        streams[i + 1].m_size = stream->m_size;
    }

    int16_t index = static_cast<int16_t>(SPIRIT_IP_MR_V3_NAME_68(0, frame, 0));
    SPIRIT_IP_MR_V3_ValidateStreamIndex(lost, &index);

    if (index != -1) {
        SPIRIT_IP_MR_V3_ReadFrameFlags(frame, &streams[index]);
    }

    if (frame->unk_02B0 == 3) {
        frame->unk_9418 = static_cast<int16_t>(SPIRIT_IP_MR_V3_GetBits(&streams[0], 5));
        stream->m_bits = streams[0].m_bits;
        return static_cast<int16_t>(streams[0].m_bits);
    }

    int16_t toggle = 0;
    int16_t bits[4];
    const int16_t* previous = &frame->unk_02B4;

    for (int16_t i = 0; i < 4; i++) {
        toggle = 1 - toggle;

        if (frame->unk_02B6[i]) {
            bits[i] = toggle ? 8 : static_cast<int16_t>(2 * (previous[i] == 0) + 6);
            index = static_cast<int16_t>(SPIRIT_IP_MR_V3_NAME_68(1, frame, i));
            SPIRIT_IP_MR_V3_ValidateStreamIndex(lost, &index);

            if (index != -1) {
                frame->unk_02D8[i].unk_0008 = static_cast<int16_t>(SPIRIT_IP_MR_V3_GetBits(&streams[index], bits[i]));
            }

            index = static_cast<int16_t>(SPIRIT_IP_MR_V3_NAME_68(2, frame, i));
            SPIRIT_IP_MR_V3_ValidateStreamIndex(lost, &index);

            if (index != -1) {
                frame->unk_02D8[i].unk_000A = static_cast<int16_t>(SPIRIT_IP_MR_V3_GetBits(&streams[index], 4));
            }
        }
    }

    for (int16_t i = 0; i < 4; i++) {
        index = static_cast<int16_t>(SPIRIT_IP_MR_V3_NAME_68(3, frame, i));
        SPIRIT_IP_MR_V3_ValidateStreamIndex(lost, &index);

        if (index != -1) {
            SPIRIT_IPMR_STREAM* source = &streams[index];
            SPIRIT_IPMR_TRACK* track = &frame->unk_0308[i][0];

            if (frame->unk_02BE[i]) {
                track->unk_0024 = SPIRIT_IP_MR_V3_GetBits(source, 17);
                track->unk_0028 = static_cast<int16_t>(SPIRIT_IP_MR_V3_GetBits(source, 5));
                track->unk_002A = static_cast<int16_t>(SPIRIT_IP_MR_V3_GetBits(source, 4));
            } else {
                track->unk_0024 = SPIRIT_IP_MR_V3_GetBits(source, s_ipmrTableB235D8[0]);
                track->unk_0028 = static_cast<int16_t>(SPIRIT_IP_MR_V3_GetBits(source, s_ipmrTableB235E4[0]));
                track->unk_002A = static_cast<int16_t>(SPIRIT_IP_MR_V3_GetBits(source, s_ipmrTableB235F0[0]));
            }
        }
    }

    stream->m_bits = streams[6].m_bits;

    if (!lost) {
        for (int16_t t = 1; t < frame->unk_02AE; t++) {
            for (int32_t i = 0; i < 4; i++) {
                SPIRIT_IPMR_TRACK* track = &frame->unk_0308[i][t];
                track->unk_0024 = SPIRIT_IP_MR_V3_GetBits(stream, static_cast<int16_t>(s_ipmrTableB235D8[t] - s_ipmrTableB235FC[t][i]));
                track->unk_0028 = static_cast<int16_t>(SPIRIT_IP_MR_V3_GetBits(stream, s_ipmrTableB235E4[t]));
                track->unk_002A = static_cast<int16_t>(SPIRIT_IP_MR_V3_GetBits(stream, s_ipmrTableB235F0[t]));
            }
        }
    }

    return 1;
}

// VERIFY: 8aa9f311 manual
// OFFSET: 0x00899EFF
void SPIRIT_IP_MR_V3_NAME_38(SPIRIT_IPMR_FRAME* frame, int16_t start) {
    for (int16_t t = start; t < frame->unk_02AE; t++) {
        int16_t shift = s_ipmrTableB235D8[t] - 6;
        int32_t mask = static_cast<int32_t>(s_ipmrTableB23628[shift]);

        if (t == 2 || t == 5) {
            int32_t* c0 = &frame->unk_0308[0][t].unk_0024;
            int32_t* c1 = &frame->unk_0308[1][t].unk_0024;
            int32_t* c2 = &frame->unk_0308[2][t].unk_0024;
            int32_t* c3 = &frame->unk_0308[3][t].unk_0024;
            int32_t value = ((((*c1 >> shift) + (((*c2 >> shift) + ((*c3 >> shift) << 6)) << 6)) << 6) + (*c0 >> shift));
            int32_t digit = value % 38;
            value = (value - digit) / 38;
            *c3 = (digit << shift) + (*c3 & mask);
            digit = value % 38;
            value = (value - digit) / 38;
            *c2 = (digit << shift) + (mask & *c2);
            *c1 = ((value % 38) << shift) + (*c1 & mask);
            *c0 = (*c0 & mask) + ((((value - value % 38) / 38) % 38) << shift);
        } else {
            for (int32_t pair = 0; pair < 2; pair++) {
                int32_t* a = &frame->unk_0308[pair * 2][t].unk_0024;
                int32_t* b = &frame->unk_0308[pair * 2 + 1][t].unk_0024;
                int32_t value = (*a >> shift) + ((*b >> shift) << 6);
                int32_t high = *b;
                *a = (mask & *a) + (((value - value % 45) / 45) << shift);
                *b = ((value % 45) << shift) + (mask & high);
            }
        }
    }
}

// VERIFY: 5c2d5821 auto
// OFFSET: 0x00896950
void SPIRIT_IP_MR_V3_UnpackFrameParameters(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream) {
    if (frame->unk_02AC) {
        SPIRIT_IP_MR_V3_NAME_152(frame, stream);
        SPIRIT_IP_MR_V3_NAME_155(frame, stream, static_cast<int16_t>(frame->unk_029C));

        if (!frame->unk_029C) {
            SPIRIT_IP_MR_V3_NAME_38(frame, 1);
        }
    }
}

// VERIFY: 5676890b manual
// OFFSET: 0x00896990
int32_t SPIRIT_IP_MR_V3_Reconfigure(SPIRIT_IPMR_DECODER* decoder, int32_t rateIn, uint32_t mode) {
    if (decoder->unk_0004 == SPIRIT_IP_MR_V3_NAME_66(rateIn) && decoder->unk_0010 == static_cast<int32_t>(mode)) {
        return 0;
    }

    return SPIRIT_IP_MR_V3_NAME_194(decoder, decoder->m_tables, rateIn, SPIRIT_IP_MR_V3_NAME_65(decoder->unk_000C), mode);
}

// VERIFY: 9e292457 auto
// OFFSET: 0x008969E0
void SPIRIT_IP_MR_V3_SaveFrameState(const SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_DECODER* decoder) {
    decoder->unk_0014 = frame->unk_028C;
    decoder->unk_0018 = frame->unk_0292;
    decoder->unk_001A = frame->unk_0296;

    switch (frame->unk_0288) {
        case 0:
        case 3:
            decoder->unk_001C = 0;
            break;

        case 1:
        case 2:
            decoder->unk_001C = 1;
            break;
    }
}

// VERIFY: f07a19d6 manual
// OFFSET: 0x00896A40
void SPIRIT_IP_MR_V3_RestoreFrameState(const SPIRIT_IPMR_DECODER* decoder, SPIRIT_IPMR_FRAME* frame) {
    frame->unk_0288 = decoder->unk_001C;
    frame->unk_028C = decoder->unk_0014;
    SPIRIT_IP_MR_V3_NAME_309(decoder->unk_0014, frame);
    SPIRIT_IP_MR_V3_NAME_334(frame->unk_02B6, sizeof(frame->unk_02B6));

    for (int16_t i = 0; i < frame->unk_9426; i++) {
        frame->unk_942C[i] = 0;
        frame->unk_9430[i][0] = 0;
    }
}

}
