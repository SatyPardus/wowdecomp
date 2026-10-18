#include <cstdint>
#include "Engine/Source/ComSat/SPIRIT/ipmr.h"
#include "Engine/Source/ComSat/SPIRIT/SpiritStream.h"

extern "C" {

// VERIFY: b01f7f01 manual
// OFFSET: 0x00899830
void SPIRIT_IP_MR_V3_PutBits(int32_t value, SPIRIT_IPMR_STREAM* stream, int16_t count, int16_t skip) {
    if (count < 0 || count > 32) throw Spirit::InvalidStream{};
    if (!count) return;
    if (skip) {
        stream->m_bits += count;
        return;
    }

    Spirit::RequireBits(stream, count);

    uint8_t* data = const_cast<uint8_t*>(stream->m_data);
    uint32_t index = stream->m_bits >> 3;
    int32_t shift = static_cast<int32_t>(stream->m_bits) % 8;
    int16_t remaining = count;
    int16_t bits = 8 - shift < count ? static_cast<int16_t>(8 - shift) : count;

    while (true) {
        remaining -= bits;
        data[index] |= static_cast<uint8_t>(((1 << bits) - 1) & value) << shift;
        stream->m_bits += bits;

        if (remaining <= 0) {
            break;
        }

        index++;
        value >>= bits;
        bits = remaining > 8 ? 8 : remaining;
        shift = 0;
    }
}

// VERIFY: bceb97dd manual
// OFFSET: 0x00899A10
void SPIRIT_IP_MR_V3_WriteFrameHeader(const SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream, int16_t* counter, int16_t skip) {
    uint32_t start = stream->m_bits;

    if (frame->unk_02A8) {
        SPIRIT_IP_MR_V3_PutBitsClear(frame->unk_0288, stream, 2, skip);
    }

    if (frame->unk_02B0) {
        SPIRIT_IP_MR_V3_PutBitsClear(frame->unk_0294 ? 1 : 0, stream, 1, skip);
    }

    *counter += static_cast<int16_t>(stream->m_bits - start);
}

// VERIFY: 41ba366f manual
// OFFSET: 0x00899C30
void SPIRIT_IP_MR_V3_WriteFrameFlags(const SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream, int16_t skip) {
    const uint8_t* widths = frame->unk_02B4 >= 0 || frame->unk_02B0 == 3 ? s_ipmrTableB23754 : s_ipmrTableB23768;

    for (int32_t i = 0; i < 4; i++) {
        SPIRIT_IP_MR_V3_PutBits(frame->unk_02C8[i], stream, widths[i], skip);
    }
}

// VERIFY: e698e41e manual
// OFFSET: 0x00899CD0
void SPIRIT_IP_MR_V3_NAME_31(SPIRIT_IPMR_FRAME* frame, int16_t start) {
    for (int16_t t = start; t < frame->unk_02AE; t++) {
        int16_t shift = s_ipmrTableB235D8[t] - 6;
        int32_t mask = static_cast<int32_t>(s_ipmrTableB23628[shift]);
        int32_t low = static_cast<int32_t>(s_ipmrTableB23628[6]);

        if (t == 2 || t == 5) {
            int32_t* c0 = &frame->unk_0308[0][t].unk_0024;
            int32_t* c1 = &frame->unk_0308[1][t].unk_0024;
            int32_t* c2 = &frame->unk_0308[2][t].unk_0024;
            int32_t* c3 = &frame->unk_0308[3][t].unk_0024;
            int32_t value = (*c3 >> shift) + 38 * ((*c2 >> shift) + 38 * ((*c1 >> shift) + 38 * (*c0 >> shift)));
            *c0 = ((value & low) << shift) + (mask & *c0);
            value >>= 6;
            *c1 = ((value & low) << shift) + (mask & *c1);
            value >>= 6;
            *c2 = (mask & *c2) + ((value & low) << shift);
            *c3 = ((low & (value >> 6)) << shift) + (mask & *c3);
        } else {
            for (int32_t pair = 0; pair < 2; pair++) {
                int32_t* a = &frame->unk_0308[pair * 2][t].unk_0024;
                int32_t* b = &frame->unk_0308[pair * 2 + 1][t].unk_0024;
                int32_t high = *b;
                int32_t value = (high >> shift) + 45 * (*a >> shift);
                *a = ((low & value) << shift) + (mask & *a);
                *b = ((value >> 6) << shift) + (mask & high);
            }
        }
    }
}

// VERIFY: c29a08c9 manual
// OFFSET: 0x0089A2C0
int16_t SPIRIT_IP_MR_V3_NAME_107(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream, int16_t* sizes, int16_t* counters, int16_t skip) {
    if (frame->unk_02B0 == 4) {
        for (int32_t i = 0; i < 7; i++) {
            sizes[i] = 0;
        }

        return 0;
    }

    uint32_t start = stream->m_bits;
    uint8_t buffer[44];
    SPIRIT_IPMR_STREAM streams[7];
    streams[0].m_data = buffer;
    streams[0].m_bits = 0;
    streams[0].m_size = 43;
    int16_t primary;
    SPIRIT_IP_MR_V3_NAME_67(frame, sizes, &primary);

    for (int32_t i = 0; i < 6; i++) {
        streams[i + 1].m_data = buffer;
        streams[i + 1].m_bits = streams[i].m_bits + sizes[i];
        streams[i + 1].m_size = 43;
    }

    SPIRIT_IP_MR_V3_NAME_334(buffer, 43);
    int16_t secondary = primary == 0;

    if (static_cast<uint32_t>(frame->unk_02B0) <= 1) {
        for (int32_t i = 0; i < 4; i++) {
            SPIRIT_IP_MR_V3_PutBits(frame->unk_02B6[i], &streams[secondary], 1, skip);
            SPIRIT_IP_MR_V3_PutBits(frame->unk_02BE[i], &streams[secondary], 1, skip);
        }
    }

    if (frame->unk_02B0 != 3) {
        SPIRIT_IP_MR_V3_PutBits((frame->unk_02B4 + 1) % 4, &streams[secondary], 2, skip);
    }

    SPIRIT_IP_MR_V3_WriteFrameFlags(frame, &streams[SPIRIT_IP_MR_V3_NAME_68(0, frame, 0)], skip);

    if (frame->unk_02B0 == 3) {
        SPIRIT_IP_MR_V3_PutBits(frame->unk_9418, &streams[0], 5, skip);
        int16_t bits = static_cast<int16_t>(streams[0].m_bits);
        sizes[0] = 0;
        sizes[1] = bits;
        SPIRIT_IP_MR_V3_CopyBits(stream, buffer, 0, bits);
        *counters += static_cast<int16_t>(stream->m_bits - start);
        return static_cast<int16_t>(stream->m_bits - start);
    }

    int16_t toggle = 0;
    const int16_t* previous = &frame->unk_02B4;

    for (int16_t i = 0; i < 4; i++) {
        int16_t next = 1 - toggle;

        if (frame->unk_02B6[i]) {
            int16_t bits = next ? 8 : static_cast<int16_t>(2 * (previous[i] == 0) + 6);
            SPIRIT_IP_MR_V3_PutBits(frame->unk_02D8[i].unk_0008, &streams[SPIRIT_IP_MR_V3_NAME_68(1, frame, i)], bits, skip);
            SPIRIT_IP_MR_V3_PutBits(frame->unk_02D8[i].unk_000A, &streams[SPIRIT_IP_MR_V3_NAME_68(2, frame, i)], 4, skip);
        }

        toggle = next;
    }

    for (int16_t i = 0; i < 4; i++) {
        SPIRIT_IPMR_STREAM* target = &streams[SPIRIT_IP_MR_V3_NAME_68(3, frame, i)];
        const SPIRIT_IPMR_TRACK* track = &frame->unk_0308[i][0];

        if (frame->unk_02BE[i]) {
            SPIRIT_IP_MR_V3_PutBits(track->unk_0024, target, 17, skip);
            SPIRIT_IP_MR_V3_PutBits(track->unk_0028, target, 5, skip);
            SPIRIT_IP_MR_V3_PutBits(track->unk_002A, target, 4, skip);
        } else {
            SPIRIT_IP_MR_V3_PutBits(track->unk_0024, target, s_ipmrTableB235D8[0], skip);
            SPIRIT_IP_MR_V3_PutBits(track->unk_0028, target, s_ipmrTableB235E4[0], skip);
            SPIRIT_IP_MR_V3_PutBits(track->unk_002A, target, s_ipmrTableB235F0[0], skip);
        }
    }

    if (skip) {
        stream->m_bits += static_cast<int16_t>(streams[6].m_bits);
    } else {
        SPIRIT_IP_MR_V3_CopyBits(stream, buffer, 0, static_cast<int16_t>(streams[6].m_bits));
    }

    *counters += static_cast<int16_t>(stream->m_bits - start);

    for (int16_t t = 1; t < frame->unk_02AE; t++) {
        uint32_t before = stream->m_bits;

        for (int32_t i = 0; i < 4; i++) {
            const SPIRIT_IPMR_TRACK* track = &frame->unk_0308[i][t];
            SPIRIT_IP_MR_V3_PutBitsClear(track->unk_0024, stream, static_cast<int16_t>(s_ipmrTableB235D8[t] - s_ipmrTableB235FC[t][i]), skip);
            SPIRIT_IP_MR_V3_PutBitsClear(track->unk_0028, stream, s_ipmrTableB235E4[t], skip);
            SPIRIT_IP_MR_V3_PutBitsClear(track->unk_002A, stream, s_ipmrTableB235F0[t], skip);
        }

        counters[t] += static_cast<int16_t>(stream->m_bits - before);
    }

    return static_cast<int16_t>(stream->m_bits - start);
}

}
