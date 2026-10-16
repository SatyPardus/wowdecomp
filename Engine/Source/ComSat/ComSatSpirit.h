#ifndef ENGINE_SOURCE_COMSAT_COMSATSPIRIT_H
#define ENGINE_SOURCE_COMSAT_COMSATSPIRIT_H

#include <cstddef>
#include <cstdint>

struct SPIRIT_IPMR_PARAMS {
    /* 0000 */ int32_t unk_0000;
    /* 0004 */ int16_t unk_0004;
    /* 0006 */ int16_t unk_0006;
};

struct SPIRIT_IPMR_STREAM {
    /* 0000 */ const uint8_t* m_data;
    /* 0004 */ uint32_t m_bits;
    /* 0008 */ uint32_t m_size;
};

struct SPIRIT_IPMR_INFO {
    /* 0000 */ int16_t unk_0000;
    /* 0002 */ int16_t unk_0002[7];
    /* 0010 */ int16_t unk_0010[4];
    /* 0018 */ int16_t unk_0018;
    /* 001A */ uint8_t unused_001A[2];
};

extern "C" int32_t SPIRIT_IP_MR_V3_1_GenerateTables(void* tables);
extern "C" int32_t SPIRIT_IP_MR_V3_1_Init(void* state, const void* tables);
extern "C" int32_t SPIRIT_IP_MR_V3_1_Encode(void* state, const uint8_t* pcm, SPIRIT_IPMR_PARAMS* params, SPIRIT_IPMR_STREAM* stream, SPIRIT_IPMR_INFO* info);
extern "C" int32_t SPIRIT_IP_MR_V3_1_Decode(void* state, uint8_t* pcm, SPIRIT_IPMR_PARAMS* params, SPIRIT_IPMR_STREAM* stream, int32_t lost);

#endif
