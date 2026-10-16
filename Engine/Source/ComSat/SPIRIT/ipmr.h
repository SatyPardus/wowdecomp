#ifndef ENGINE_SOURCE_COMSAT_SPIRIT_IPMR_H
#define ENGINE_SOURCE_COMSAT_SPIRIT_IPMR_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/ComSat/ComSatSpirit.h"

struct SPIRIT_IPMR_TABLES {
    /* 0000 */ float unk_0000[4480];
    /* 4600 */ float unk_4600[3200];
    /* 7800 */ float unk_7800[3200];
    /* AA00 */ float unk_AA00[240];
    /* ADC0 */ uint8_t unused_ADC0[192];
    /* AE80 */ float unk_AE80[11][12];
    /* B090 */ float unk_B090[11][12];
    /* B2A0 */ float unk_B2A0[5][12];
    /* B390 */ float unk_B390[5][12];
    /* B480 */ float unk_B480[5][12];
    /* B570 */ float unk_B570[5][12];
    /* B660 */ float unk_B660[61];
    /* B754 */ uint8_t unused_B754[12];
    /* B760 */ float unk_B760[12];
    /* B790 */ float unk_B790[12];
    /* B7C0 */ float unk_B7C0[12];
    /* B7F0 */ float unk_B7F0[12];
    /* B820 */ float unk_B820[12];
    /* B850 */ float unk_B850[12];
    /* B880 */ float unk_B880[11];
    /* B8AC */ int32_t unk_B8AC[40][5];
    /* BBCC */ int16_t m_initialized;
    /* BBCE */ uint8_t unused_BBCE[2];
};

struct SPIRIT_IPMR_UNK114 {
    /* 0000 */ float unk_0000[10];
    /* 0028 */ float unk_0028[5];
    /* 003C */ float unk_003C;
    /* 0040 */ float unk_0040[5];
    /* 0054 */ float unk_0054;
    /* 0058 */ int16_t unk_0058;
    /* 005A */ int16_t unk_005A;
    /* 005C */ int16_t unk_005C;
    /* 005E */ int16_t unk_005E;
    /* 0060 */ int32_t unk_0060;
};

struct SPIRIT_IPMR_ENC_UNK0020 {
    /* 0000 */ float unk_0000[80];
    /* 0140 */ float unk_0140[8];
    /* 0160 */ int16_t unk_0160;
    /* 0162 */ int16_t unk_0162;
    /* 0164 */ int16_t unk_0164;
    /* 0166 */ int16_t unk_0166;
    /* 0168 */ uint8_t unused_0168[2968];
};

struct SPIRIT_IPMR_ENC_UNK0D20 {
    /* 0000 */ float unk_0000[4];
    /* 0010 */ float unk_0010[360];
    /* 05B0 */ float unk_05B0[303];
    /* 0A6C */ uint8_t unused_0A6C[4];
    /* 0A70 */ float unk_0A70[314];
    /* 0F58 */ uint8_t unused_0F58[8];
    /* 0F60 */ float unk_0F60[314];
    /* 1448 */ uint8_t unused_1448[8];
    /* 1450 */ float unk_1450[314];
    /* 1938 */ float unk_1938[10];
    /* 1960 */ float unk_1960[10];
    /* 1988 */ float unk_1988[10];
    /* 19B0 */ float unk_19B0[10];
    /* 19D8 */ uint8_t unused_19D8[40];
    /* 1A00 */ float unk_1A00[10];
    /* 1A28 */ uint8_t unused_1A28[40];
    /* 1A50 */ float unk_1A50[330];
    /* 1F78 */ float unk_1F78;
    /* 1F7C */ uint8_t unused_1F7C[24];
    /* 1F94 */ int32_t unk_1F94;
    /* 1F98 */ float* unk_1F98;
    /* 1F9C */ float* unk_1F9C;
    /* 1FA0 */ float* unk_1FA0;
    /* 1FA4 */ float* unk_1FA4;
    /* 1FA8 */ float* unk_1FA8;
    /* 1FAC */ float* unk_1FAC;
    /* 1FB0 */ int16_t unk_1FB0;
    /* 1FB2 */ int16_t unk_1FB2;
    /* 1FB4 */ uint8_t unused_1FB4[4];
    /* 1FB8 */ int32_t unk_1FB8;
    /* 1FBC */ uint8_t unused_1FBC[4];
};

struct SPIRIT_IPMR_ENCODER {
    /* 0000 */ int32_t m_initialized;
    /* 0004 */ int32_t unk_0004;
    /* 0008 */ int32_t unk_0008;
    /* 000C */ int32_t unk_000C;
    /* 0010 */ int32_t unk_0010;
    /* 0014 */ uint8_t unk_0014[10];
    /* 001E */ uint8_t unused_001E[2];
    /* 0020 */ SPIRIT_IPMR_ENC_UNK0020 unk_0020;
    /* 0D20 */ SPIRIT_IPMR_ENC_UNK0D20 unk_0D20;
    /* 2CE0 */ SPIRIT_IPMR_TABLES* m_tables;
    /* 2CE4 */ uint8_t unused_2CE4[12];
};

struct SPIRIT_IPMR_POSTFILTER {
    /* 0000 */ float unk_0000[10];
    /* 0028 */ float unk_0028[160];
    /* 02A8 */ float unk_02A8[10];
    /* 02D0 */ float unk_02D0;
    /* 02D4 */ float unk_02D4;
    /* 02D8 */ float unk_02D8;
    /* 02DC */ float unk_02DC;
};

struct SPIRIT_IPMR_UNK158 {
    /* 0000 */ float unk_0000[5];
    /* 0014 */ float unk_0014;
    /* 0018 */ int16_t unk_0018;
    /* 001A */ int16_t unk_001A;
};

struct SPIRIT_IPMR_DEC_UNK0020 {
    /* 0000 */ float unk_0000[194];
    /* 0308 */ uint8_t unused_0308[8];
    /* 0310 */ float unk_0310[12];
    /* 0340 */ float unk_0340[12];
    /* 0370 */ float unk_0370[10];
    /* 0398 */ SPIRIT_IPMR_UNK158 unk_0398;
    /* 03B4 */ float* unk_03B4;
    /* 03B8 */ SPIRIT_IPMR_POSTFILTER unk_03B8;
    /* 0698 */ uint8_t unused_0698[16];
    /* 06A8 */ SPIRIT_IPMR_UNK114 unk_06A8;
    /* 070C */ uint8_t unused_070C[4];
};

struct SPIRIT_IPMR_DEC_UNK0730 {
    /* 0000 */ uint8_t unused_0000[4];
    /* 0004 */ float unk_0004;
    /* 0008 */ float unk_0008;
    /* 000C */ uint8_t unused_000C[40];
    /* 0034 */ float unk_0034[10];
    /* 005C */ float unk_005C[10];
    /* 0084 */ int16_t unk_0084;
    /* 0086 */ uint8_t unused_0086[2];
    /* 0088 */ int32_t unk_0088;
};

struct SPIRIT_IPMR_DECODER {
    /* 0000 */ int32_t m_initialized;
    /* 0004 */ int32_t unk_0004;
    /* 0008 */ int32_t unk_0008;
    /* 000C */ int32_t unk_000C;
    /* 0010 */ int32_t unk_0010;
    /* 0014 */ int32_t unk_0014;
    /* 0018 */ int16_t unk_0018;
    /* 001A */ int16_t unk_001A;
    /* 001C */ int32_t unk_001C;
    /* 0020 */ SPIRIT_IPMR_DEC_UNK0020 unk_0020;
    /* 0730 */ SPIRIT_IPMR_DEC_UNK0730 unk_0730;
    /* 07BC */ SPIRIT_IPMR_TABLES* m_tables;
};

struct SPIRIT_IPMR_SUBFRAME {
    /* 0000 */ int16_t unk_0000;
    /* 0002 */ int16_t unk_0002;
    /* 0004 */ float unk_0004;
    /* 0008 */ int16_t unk_0008;
    /* 000A */ int16_t unk_000A;
};

struct SPIRIT_IPMR_TRACK {
    /* 0000 */ int16_t unk_0000;
    /* 0002 */ uint8_t unused_0002[2];
    /* 0004 */ float unk_0004[5];
    /* 0018 */ int16_t unk_0018[6];
    /* 0024 */ int32_t unk_0024;
    /* 0028 */ int16_t unk_0028;
    /* 002A */ int16_t unk_002A;
    /* 002C */ uint8_t unused_002C[160];
    /* 00CC */ float unk_00CC[40];
    /* 016C */ uint8_t unused_016C[160];
    /* 020C */ float unk_020C[40];
    /* 02AC */ uint8_t unused_02AC[160];
};

struct SPIRIT_IPMR_FRAME {
    /* 0000 */ float unk_0000[160];
    /* 0280 */ int32_t m_rate;
    /* 0284 */ int32_t m_mode;
    /* 0288 */ int32_t unk_0288;
    /* 028C */ int32_t unk_028C;
    /* 0290 */ int16_t unk_0290;
    /* 0292 */ int16_t unk_0292;
    /* 0294 */ int16_t unk_0294;
    /* 0296 */ int16_t unk_0296;
    /* 0298 */ int16_t unk_0298;
    /* 029A */ uint8_t unused_029A[2];
    /* 029C */ int32_t unk_029C;
    /* 02A0 */ int16_t unk_02A0;
    /* 02A2 */ int16_t unk_02A2;
    /* 02A4 */ int16_t unk_02A4;
    /* 02A6 */ int16_t unk_02A6;
    /* 02A8 */ int16_t unk_02A8;
    /* 02AA */ int16_t unk_02AA;
    /* 02AC */ int16_t unk_02AC;
    /* 02AE */ int16_t unk_02AE;
    /* 02B0 */ int32_t unk_02B0;
    /* 02B4 */ int16_t unk_02B4;
    /* 02B6 */ int16_t unk_02B6[4];
    /* 02BE */ int16_t unk_02BE[4];
    /* 02C6 */ uint8_t unused_02C6[2];
    /* 02C8 */ int32_t unk_02C8[4];
    /* 02D8 */ SPIRIT_IPMR_SUBFRAME unk_02D8[4];
    /* 0308 */ SPIRIT_IPMR_TRACK unk_0308[4][11];
    /* 9418 */ int16_t unk_9418;
    /* 941A */ uint8_t unused_941A[2];
    /* 941C */ float unk_941C;
    /* 9420 */ int16_t unk_9420;
    /* 9422 */ int16_t unk_9422;
    /* 9424 */ int16_t unk_9424;
    /* 9426 */ int16_t unk_9426;
    /* 9428 */ int16_t unk_9428;
    /* 942A */ int16_t unk_942A;
    /* 942C */ int16_t unk_942C[2];
    /* 9430 */ uint8_t unk_9430[2][500];
    /* 9818 */ uint8_t unused_9818[8];
};

struct SPIRIT_IPMR_STATE {
    /* 0000 */ SPIRIT_IPMR_ENCODER m_encoder;
    /* 2CF0 */ SPIRIT_IPMR_DECODER m_decoder;
};

extern "C" {

extern const int16_t s_ipmrTableB1DE08[10];
extern const int16_t s_ipmrTableB1DE1C[10];
extern const int16_t s_ipmrTableB1DE30[10];
extern const int16_t s_ipmrTableB1DE44[10];
extern const int16_t s_ipmrTableB1DE58[3200];
extern const int16_t s_ipmrTableB1F758[3200];
extern const int16_t s_ipmrTableB21058[4480];
extern const float s_ipmrTableB234D0[10];
extern const float s_ipmrTableB234F8[11];
extern const float s_ipmrTableB23524[11];
extern const float s_ipmrTableB23550[5];
extern const float s_ipmrTableB23564[5];
extern const float s_ipmrTableB23578[5];
extern const float s_ipmrTableB2358C[5];
extern const uint8_t s_ipmrTableB236F0[8];
extern const uint8_t s_ipmrTableB236F8[10];
extern const int16_t s_ipmrTableB23704[16];
extern const int32_t s_ipmrTableB23724[8];
extern const uint8_t s_ipmrTableB235A0[11];
extern const uint8_t s_ipmrTableB235D8[12];
extern const uint8_t s_ipmrTableB235E4[12];
extern const uint8_t s_ipmrTableB235F0[12];
extern const uint8_t s_ipmrTableB235FC[11][4];
extern const uint8_t s_ipmrTableB23754[4];
extern const uint8_t s_ipmrTableB23768[4];
extern const uint32_t s_ipmrTableB23628[33];
extern const float s_ipmrTableB23358[7];
extern const float s_ipmrTableB23374[7];
extern const float s_ipmrTableA58F70[4][3];
extern const int32_t s_ipmrTableB23744[4];
extern const int32_t s_ipmrTableB23758[3];
extern const float s_ipmrTableB23390[40];
extern const float s_ipmrTableB23430[40];
extern const float s_ipmrTableA58EB0[48];
extern const float s_ipmrTableA58E70[16];
extern const float s_ipmrTableB236B0[16];

void SPIRIT_IP_MR_V3_DequantizeTable(float* dst, const int16_t* src, float scale, int32_t count);
void SPIRIT_IP_MR_V3_NAME_201(SPIRIT_IPMR_TABLES* tables);
void SPIRIT_IP_MR_V3_ResetDecoderState(SPIRIT_IPMR_DEC_UNK0020* state, const SPIRIT_IPMR_TABLES* tables);
int32_t SPIRIT_IP_MR_V3_NAME_194(SPIRIT_IPMR_DECODER* decoder, const SPIRIT_IPMR_TABLES* tables, int32_t rateIn, int32_t rateOut, uint32_t mode);
void SPIRIT_IP_MR_V3_ResetEncoderState(SPIRIT_IPMR_ENC_UNK0D20* state);
int32_t SPIRIT_IP_MR_V3_NAME_197(SPIRIT_IPMR_ENCODER* encoder, const SPIRIT_IPMR_TABLES* tables, int32_t rate, uint32_t mode);
void SPIRIT_IP_MR_V3_NAME_191(float* dst, const float* src);
void SPIRIT_IP_MR_V3_NAME_190(float* dst, const float* src);
void SPIRIT_IP_MR_V3_NAME_189(float* dst, const float* src, int32_t count);
void SPIRIT_IP_MR_V3_NAME_333(float* dst);
void SPIRIT_IP_MR_V3_NAME_314(float* dst);
void SPIRIT_IP_MR_V3_NAME_332(float* dst, int32_t count);
void SPIRIT_IP_MR_V3_NAME_334(void* dst, uint32_t size);
void SPIRIT_IP_MR_V3_NAME_179(float* dst, const float* src, int32_t count);
void SPIRIT_IP_MR_V3_AddSubframe(float* dst, const float* src);
void SPIRIT_IP_MR_V3_NAME_386(float* dst, float value);
void SPIRIT_IP_MR_V3_NAME_322(float* dst, const float* src);
void SPIRIT_IP_MR_V3_NAME_321(float* dst, const float* src, int32_t count);
void SPIRIT_IP_MR_V3_NAME_248(float* dst, float value);
void SPIRIT_IP_MR_V3_NAME_114(SPIRIT_IPMR_UNK114* state, const float* init);
void SPIRIT_IP_MR_V3_NAME_144(SPIRIT_IPMR_UNK114* state, int16_t flag);
void SPIRIT_IP_MR_V3_NAME_46(SPIRIT_IPMR_ENC_UNK0020* state);
void SPIRIT_IP_MR_V3_NAME_44(SPIRIT_IPMR_DEC_UNK0730* state);
float SPIRIT_IP_MR_V3_NAME_42(const float* a, const float* b, int32_t count);
int32_t SPIRIT_IP_MR_V3_NAME_66(int32_t index);
int32_t SPIRIT_IP_MR_V3_NAME_65(int32_t rate);
int32_t SPIRIT_IP_MR_V3_NAME_323(int32_t mode, int32_t rate, int16_t a3, int16_t a4, SPIRIT_IPMR_FRAME* frame);
int16_t SPIRIT_IP_MR_V3_NAME_387(SPIRIT_IPMR_FRAME* frame);
int16_t SPIRIT_IP_MR_V3_NAME_61(int32_t index, SPIRIT_IPMR_FRAME* frame);
int32_t SPIRIT_IP_MR_V3_NAME_309(int32_t index, SPIRIT_IPMR_FRAME* frame);
void SPIRIT_IP_MR_V3_NAME_311(SPIRIT_IPMR_FRAME* frame);
void SPIRIT_IP_MR_V3_PutBitsClear(int32_t value, SPIRIT_IPMR_STREAM* stream, int16_t count, int16_t skip);
void SPIRIT_IP_MR_V3_CopyBits(SPIRIT_IPMR_STREAM* stream, const uint8_t* src, int16_t start, int16_t count);
void SPIRIT_IP_MR_V3_NAME_199(const int16_t* pcm, float* dst, int16_t count);
void SPIRIT_IP_MR_V3_NAME_200(const float* src, int16_t* pcm, int16_t count);
float SPIRIT_IP_MR_V3_SquaredDistance10(const float* a, const float* b);
void SPIRIT_IP_MR_V3_NAME_85(SPIRIT_IPMR_ENC_UNK0D20* state, const float* input);
void SPIRIT_IP_MR_V3_HighPassFilter(float* memory, const float* input, float* output);
int32_t SPIRIT_IP_MR_V3_GetBits(SPIRIT_IPMR_STREAM* stream, int16_t count);
void SPIRIT_IP_MR_V3_ReadFrameHeader(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream);
void SPIRIT_IP_MR_V3_ValidateStreamIndex(int16_t count, int16_t* index);
void SPIRIT_IP_MR_V3_ReadFrameFlags(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream);
int32_t SPIRIT_IP_MR_V3_NAME_68(int16_t kind, const SPIRIT_IPMR_FRAME* frame, int16_t index);
int16_t SPIRIT_IP_MR_V3_NAME_152(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream);
int16_t SPIRIT_IP_MR_V3_NAME_155(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream, int16_t lost);
void SPIRIT_IP_MR_V3_NAME_38(SPIRIT_IPMR_FRAME* frame, int16_t start);
int16_t SPIRIT_IP_MR_V3_NAME_67(const SPIRIT_IPMR_FRAME* frame, int16_t* sizes, int16_t* primary);
void SPIRIT_IP_MR_V3_UnpackFrameParameters(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream);
int32_t SPIRIT_IP_MR_V3_Reconfigure(SPIRIT_IPMR_DECODER* decoder, int32_t rateIn, uint32_t mode);
void SPIRIT_IP_MR_V3_SaveFrameState(const SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_DECODER* decoder);
void SPIRIT_IP_MR_V3_RestoreFrameState(const SPIRIT_IPMR_DECODER* decoder, SPIRIT_IPMR_FRAME* frame);
void SPIRIT_IP_MR_V3_PutBits(int32_t value, SPIRIT_IPMR_STREAM* stream, int16_t count, int16_t skip);
void SPIRIT_IP_MR_V3_WriteFrameHeader(const SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream, int16_t* counter, int16_t skip);
void SPIRIT_IP_MR_V3_WriteFrameFlags(const SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream, int16_t skip);
void SPIRIT_IP_MR_V3_NAME_31(SPIRIT_IPMR_FRAME* frame, int16_t start);
int16_t SPIRIT_IP_MR_V3_NAME_107(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_STREAM* stream, int16_t* sizes, int16_t* counters, int16_t skip);
int32_t SPIRIT_IP_MR_V3_NAME_195(SPIRIT_IPMR_ENCODER* encoder, const uint8_t* pcm, SPIRIT_IPMR_STREAM* stream, int32_t a4, int32_t a5, int16_t a6, int16_t a7, int16_t* a8, int16_t* a9, int16_t* a10, int16_t* a11, int16_t* a12);
int32_t SPIRIT_IP_MR_V3_NAME_192(SPIRIT_IPMR_DECODER* decoder, SPIRIT_IPMR_STREAM* stream, uint8_t* pcm, int32_t a4, uint32_t a5, int32_t a6, int16_t a7, int16_t a8, int16_t a9, int32_t a10, int16_t* a11);
float GetMedian(const float* values);
float SPIRIT_IP_MR_V3_NAME_113(const SPIRIT_IPMR_UNK114* state);
void SPIRIT_IP_MR_V3_NAME_63(SPIRIT_IPMR_UNK114* state, SPIRIT_IPMR_SUBFRAME* subframe, SPIRIT_IPMR_TRACK* track);
void SPIRIT_IP_MR_V3_UpdateGainHistory(int16_t lost, SPIRIT_IPMR_UNK114* state, const SPIRIT_IPMR_SUBFRAME* subframe);
void SPIRIT_IP_MR_V3_NAME_137(const float* x, float* y, const float* a);
void SPIRIT_IP_MR_V3_NAME_148(const float* x, float* y, const float* a, float* mem, int16_t update);
void SPIRIT_IP_MR_V3_NAME_389(const float* coeffs, float* h, const float* a);
void SPIRIT_IP_MR_V3_NAME_390(float* y, const float* a, const float* mem);
int32_t SPIRIT_IP_MR_V3_SortLsf(float* lsf);
void SPIRIT_IP_MR_V3_NAME_215(const float* lsp, float* a);
void SPIRIT_IP_MR_V3_NAME_391(float* out, const float* src, const float* w1, const float* w2);
void SPIRIT_IP_MR_V3_NAME_216(const float* lsp, float* lsf);
void SPIRIT_IP_MR_V3_NAME_205(const float* prev, const float* cur, int16_t index, float* a);
void SPIRIT_IP_MR_V3_ComputeLpc(float* a, const float* signal, const SPIRIT_IPMR_TABLES* tables);
float SPIRIT_IP_MR_V3_ChebyshevEval(float x, const float* f);
int32_t SPIRIT_IP_MR_V3_LpcToLsp(const float* a, float* lsp, const float* grid);
void SPIRIT_IP_MR_V3_FilterSubframes(float* out, const float* coeffs, const float* src, float* mem);
float SPIRIT_IP_MR_V3_EvalLpcSpectrum(float w, const float* a);
void SPIRIT_IP_MR_V3_NAME_330(float* out, const float* lsf, const float* a);
void SPIRIT_IP_MR_V3_AddVector(int32_t count, float* dst, const float* src);
float* SPIRIT_IP_MR_V3_CopyVector(int32_t count, float* dst, const float* src);
void SPIRIT_IP_MR_V3_SubVector(int32_t count, float* dst, const float* src);
float SPIRIT_IP_MR_V3_NAME_331(const float* codebook, const float* target, const float* mean, const float* predictor, const float* previous, const int32_t* sizes, const float* weights, float* quantized, int32_t* indices, int32_t stages);
int32_t SPIRIT_IP_MR_V3_NAME_121(float* lsp, const float* lpc, int32_t* indices, const float* previous, const SPIRIT_IPMR_TABLES* tables);
void SPIRIT_IP_MR_V3_NAME_78(SPIRIT_IPMR_ENC_UNK0D20* state, const SPIRIT_IPMR_TABLES* tables, float* lpc, float* weighted, SPIRIT_IPMR_FRAME* frame);
int32_t SPIRIT_IP_MR_V3_NAME_37(float* lsp, const int32_t* indices, int16_t lost, float* lsf, const float* previous, const SPIRIT_IPMR_TABLES* tables);
void SPIRIT_IP_MR_V3_NAME_29(const float* a, const float* coeffs, float* h);
void SPIRIT_IP_MR_V3_NAME_147(const float* target, const float* a, const float* weighted, float* out, float* synthesis, float* weighting);
void SPIRIT_IP_MR_V3_NAME_146(const float* excitation, float* residual, const float* a, const float* weighted, float* out, SPIRIT_IPMR_TRACK* tracks, float* memory, int16_t count);
void SPIRIT_IP_MR_V3_NAME_157(const float* excitation, const float* a, const float* weighted, float* synthesis, float* weighting, float* out);
void SPIRIT_IP_MR_V3_NAME_28(float* memory, int16_t first, int16_t last);
void SPIRIT_IP_MR_V3_SortPulses(int16_t count, SPIRIT_IPMR_TRACK* tracks);
void SPIRIT_IP_MR_V3_EncodePulsePositions(SPIRIT_IPMR_TRACK* tracks, int16_t count, const SPIRIT_IPMR_TABLES* tables);
void SPIRIT_IP_MR_V3_NAME_35(const SPIRIT_IPMR_TRACK* tracks, int16_t first, int16_t last, float* out);
void SPIRIT_IP_MR_V3_QuantizeUniform(int16_t levels, float* value, int16_t* index, float low, float high);
void SPIRIT_IP_MR_V3_NAME_158(SPIRIT_IPMR_UNK158* state, float gain, float energy);
void SPIRIT_IP_MR_V3_NAME_109(int16_t mode, const SPIRIT_IPMR_TRACK* tracks, int16_t first, int16_t last, float* out);
void SPIRIT_IP_MR_V3_NAME_156(float* excitation, const float* target, const float* response, const float* a, const float* weighted, const SPIRIT_IPMR_TRACK* tracks, float* memory, float* out, int16_t count);
void SPIRIT_IP_MR_V3_SearchTrackPulses(SPIRIT_IPMR_TRACK* track, float* target, const float* h, float* correlation, const float* phi, int16_t* used, int16_t skip);
void SPIRIT_IP_MR_V3_NAME_59(float* target, SPIRIT_IPMR_TRACK* tracks, int16_t count, const float* h, int16_t skip);
void SPIRIT_IP_MR_V3_SearchFixedCodebook(float* target, const float* h, const float* unused, SPIRIT_IPMR_TRACK* tracks, int16_t count, int16_t skip, const SPIRIT_IPMR_TABLES* tables);
void SPIRIT_IP_MR_V3_NAME_149(SPIRIT_IPMR_TRACK* tracks, int16_t count, const SPIRIT_IPMR_TABLES* tables);
int16_t SPIRIT_IP_MR_V3_NAME_111(const float* x);
void SPIRIT_IP_MR_V3_PitchCorrelationFiltered(float* out, const float* excitation, const float* target, const float* h, int16_t first, int16_t last);
void SPIRIT_IP_MR_V3_PitchCorrelationNormalized(int16_t first, const float* excitation, const float* target, int16_t last, float* out);
void SPIRIT_IP_MR_V3_RefineFractionalPitch(const float* correlation, int16_t* lag, int16_t* fraction, int16_t last);
int16_t SPIRIT_IP_MR_V3_QuantizePitchGain(float* gain);
void SPIRIT_IP_MR_V3_NAME_79(float* excitation, int16_t lag, int16_t fraction);
void SPIRIT_IP_MR_V3_NAME_34(float* excitation, int16_t lag, int16_t fraction, float gain);
void SPIRIT_IP_MR_V3_NAME_153(int16_t index, int16_t* previous, int16_t* lag, int16_t* fraction, int16_t mode);
void SPIRIT_IP_MR_V3_SearchClosedLoopPitch(int16_t open, int16_t* previous, SPIRIT_IPMR_SUBFRAME* subframe, const float* excitation, const float* target, const float* h, int16_t mode);
void SPIRIT_IP_MR_V3_NAME_14(int16_t* previous, int16_t open, SPIRIT_IPMR_SUBFRAME* subframe, const float* h, float* excitation, float* target, int16_t mode);
void SPIRIT_IP_MR_V3_NAME_15(int16_t* previous, int16_t open, SPIRIT_IPMR_SUBFRAME* subframe, float* excitation, const float* target, int16_t mode);
void SPIRIT_IP_MR_V3_NAME_399(SPIRIT_IPMR_DEC_UNK0730* state, SPIRIT_IPMR_FRAME* frame);
void SPIRIT_IP_MR_V3_NAME_400(SPIRIT_IPMR_ENC_UNK0020* state, SPIRIT_IPMR_FRAME* frame);
void SPIRIT_IP_MR_V3_UpdateFixedCodebookEnergy(int16_t lost, SPIRIT_IPMR_UNK114* state, const SPIRIT_IPMR_TRACK* tracks, int16_t count);
void SPIRIT_IP_MR_V3_NAME_60(SPIRIT_IPMR_ENC_UNK0D20* state, SPIRIT_IPMR_FRAME* frame, const int16_t* open);
void SPIRIT_IP_MR_V3_SynthesisFilter32(const float* in, float* out, const float* a);
void SPIRIT_IP_MR_V3_TiltCompensation(float* x, float* state, float mu);
void SPIRIT_IP_MR_V3_AdaptiveGainControl(float* gain, float alpha, float* signal, const float* reference);
void SPIRIT_IP_MR_V3_NAME_115(SPIRIT_IPMR_POSTFILTER* state, float* signal, const float* lpc, const float* w1, const float* w2);
void SPIRIT_IP_MR_V3_NAME_116(SPIRIT_IPMR_DEC_UNK0020* state, const SPIRIT_IPMR_TABLES* tables, float* signal, const float* lpc, const SPIRIT_IPMR_FRAME* frame);
void SPIRIT_IP_MR_V3_DecodeSubframeGains(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_SUBFRAME* subframe, SPIRIT_IPMR_UNK114* state, int16_t* previous, int16_t index, int32_t lost);
void SPIRIT_IP_MR_V3_NAME_324(SPIRIT_IPMR_DEC_UNK0020* state, const SPIRIT_IPMR_TABLES* tables, float* out, SPIRIT_IPMR_FRAME* frame, float* lpc, int16_t lost);
void SPIRIT_IP_MR_V3_SynthesizeFrame(SPIRIT_IPMR_FRAME* frame, const SPIRIT_IPMR_TABLES* tables, SPIRIT_IPMR_DEC_UNK0020* state, float* out);
void SPIRIT_IP_MR_V3_NAME_181(SPIRIT_IPMR_ENC_UNK0D20* state, const SPIRIT_IPMR_TABLES* tables, const float* signal, SPIRIT_IPMR_FRAME* frame);
void SPIRIT_IP_MR_V3_AnalyzeFrame(SPIRIT_IPMR_FRAME* frame, SPIRIT_IPMR_ENC_UNK0D20* state, const SPIRIT_IPMR_TABLES* tables, const float* input);

}

#endif
