#ifndef ENGINE_SOURCE_STORM_H_STORM_H
#define ENGINE_SOURCE_STORM_H_STORM_H

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/Types.h"

#define STORM_MAX_STR 0x7FFFFFFF

#define SCRC_INITIALIZE 0x1
#define SCRC_PROCESS 0x2
#define SCRC_FINALIZE 0x4

#define STORM_ERROR_SUCCESS 0
#define STORM_ERROR_FILE_NOT_FOUND 2
#define STORM_ERROR_NOT_ENOUGH_MEMORY 8
#define STORM_ERROR_INVALID_PARAMETER 87
#define STORM_ERROR_ASSERTION 0x85100000
#define STORM_ERROR_BAD_ARGUMENT 0x85100065
#define STORM_ERROR_NOT_ARCHIVE 0x8510006C
#define STORM_ERROR_NOT_ENOUGH_ARGUMENTS 0x8510006D
#define STORM_ERROR_VERSION_MISMATCH 0x85100079
#define STORM_ERROR_MEM_NOT_ALLOCATED 0x8510007A
#define STORM_ERROR_MEM_CORRUPTED 0x8510007B
#define STORM_ERROR_MEM_INVALID 0x8510007C
#define STORM_ERROR_MEM_NOT_FREED 0x8510007E
#define STORM_ERROR_RESOURCES_NOT_RELEASED 0x8510007F
#define STORM_ERROR_OUT_OF_BOUNDS 0x85100080
#define STORM_ERROR_FILE_CORRUPTED 0x85100083
#define STORM_ERROR_FATAL 0x85100084
#define STORM_ERROR_FATAL_CONDITION 0x85100086

#define SLOG_EXPRESSION 0
#define SLOG_FUNCTION (-1)
#define SLOG_OBJECT (-2)
#define SLOG_HANDLE (-3)
#define SLOG_FILE (-4)
#define SLOG_EXCEPTION (-5)

#define SCMD_TYPE_BOOL 0x0
#define SCMD_TYPE_NUMBER 0x10000
#define SCMD_TYPE_STRING 0x20000
#define SCMD_TYPE_MASK 0x30000
#define SCMD_ARG_FLAG 0x0
#define SCMD_ARG_OPTIONAL 0x1000000
#define SCMD_ARG_REQUIRED 0x2000000
#define SCMD_ARG_MASK 0x3000000
#define SCMD_BOOL_CLEAR 0x1
#define SCMD_NUM_SIGNED 0x1
#define SCMD_CASE_SENSITIVE 0x100

#define SLOG_FLAG_OPENFILE 0x1
#define SLOG_FLAG_NOFILE 0x2
#define SLOG_FLAG_APPEND 0x4

#define SMEM_FLAG_ZEROMEMORY 0x8
#define SMEM_FLAG_NOREALLOC 0x10
#define SMEM_REALLOC_NULL 0xB00BEEE5
#define SMEM_DEBUG_NOEXITONFAILURE 0x10
#define SMEM_DEBUG_HEAPTRACKING 0x20

#define SREG_NONE 0x0
#define SREG_EXCLUDE_LOCAL_MACHINE 0x1
#define SREG_BATTLE_NET 0x2
#define SREG_EXCLUDE_CURRENT_USER 0x4
#define SREG_FLUSH_KEY 0x8
#define SREG_ABSOLUTE 0x10

#define SRGN_AND 1
#define SRGN_OR 2
#define SRGN_XOR 3
#define SRGN_DIFF 4
#define SRGN_COPY 5
#define SRGN_PARAMONLY 6

#define MPQ_COMPRESSION_HUFFMANN 0x01
#define MPQ_COMPRESSION_ZLIB 0x02
#define MPQ_COMPRESSION_BZIP2 0x10
#define MPQ_COMPRESSION_SPARSE 0x20
#define MPQ_COMPRESSION_ADPCM_MONO 0x40
#define MPQ_COMPRESSION_ADPCM_STEREO 0x80

#define SSTR_HASH_CASESENSITIVE 0x1

#define STORM_OPTION_SERR_LEAK_SILENT_WARNING 1
#define STORM_OPTION_WAVE_CHUNK_SIZE 2
#define STORM_OPTION_SMEM_LEAK_SILENT_WARNING 3
#define STORM_OPTION_ALIGN_STREAMING_WAVE_DATA 4
#define STORM_OPTION_ECHO_TO_OUTPUT_DEBUG_STRING 5
#define STORM_OPTION_ALLOC_COUNTS 6
#define STORM_OPTION_SERR_SUPPRESS_LOGS 7
#define STORM_OPTION_CRC_ENABLED 9
#define STORM_OPTION_ORDERED_PRINTF_ENABLED 10
#define STORM_OPTION_UNICODE_ENABLED 11
#define STORM_OPTION_UNK_0024 12
#define STORM_OPTION_UNK_0028 13

#define STORM_SERVICE_GETALLOCATOR 1
#define STORM_SERVICE_GENERATEREPORT 2
#define STORM_SERVICE_I64TOSTRING 3
#define STORM_SERVICE_MARKALLHEAPS 4

class BigData;
struct HINSTANCE__;
struct HWND__;
struct SSignatureData;

void SARC4PrepareKey(const void* key, uint32_t length, SARC4Key* outKey);
void SARC4ProcessBuffer(uint8_t* data, uint32_t length, const SARC4Key* inKey, SARC4Key* outKey);

void STORMAPI SBigAdd(BigData* a, const BigData* b, const BigData* c);
void STORMAPI SBigBitLen(BigData* a, uint32_t* bits);
int32_t STORMAPI SBigCompare(const BigData* a, const BigData* b);
void STORMAPI SBigDel(BigData* num);
void STORMAPI SBigFromBinary(BigData* num, const void* data, uint32_t bytes);
void STORMAPI SBigFromUnsigned(BigData* num, uint32_t val);
void STORMAPI SBigMod(BigData* a, const BigData* b, const BigData* c);
void STORMAPI SBigMul(BigData* a, const BigData* b, const BigData* c);
void STORMAPI SBigNew(BigData** num);
void STORMAPI SBigPowMod(BigData* a, const BigData* b, const BigData* c, const BigData* d);
void STORMAPI SBigSub(BigData* a, const BigData* b, const BigData* c);
void STORMAPI SBigToBinaryBuffer(const BigData* num, void* data, uint32_t maxBytes, uint32_t* bytes);

int32_t STORMAPI SCmdDestroy();
int32_t STORMAPI SCmdGetBool(uint32_t id);
uint32_t STORMAPI SCmdGetNum(uint32_t id);
int32_t STORMAPI SCmdGetString(uint32_t id, char* buffer, uint32_t bufferchars);
int32_t STORMAPI SCmdProcess(const char* cmdline, int32_t skipprogname, int32_t (STORMAPI* extracallback)(const char*), void (STORMAPI* errorcallback)(_CMDERROR*));
int32_t STORMAPI SCmdProcessCommandLine(int32_t (STORMAPI* extracallback)(const char*), void (STORMAPI* errorcallback)(_CMDERROR*));
int32_t STORMAPI SCmdRegisterArgList(const _ARGLIST* listptr, uint32_t numargs);
int32_t STORMAPI SCmdRegisterArgument(uint32_t flags, uint32_t id, const char* name, void* variableptr, uint32_t variablebytes, uint32_t setvalue, uint32_t setmask, int32_t (STORMAPI* callback)(_CMDPARAMS*, const char*));

uint32_t SCrcBuffer(const void* buffer, uint32_t length, uint32_t* crc, uint32_t flags);
uint32_t STORMAPI SCrcCompute(const void* buffer, uint32_t length, uint32_t* crc, uint32_t flags);

int32_t STORMAPI SCompDestroy();

void STORMAPI SErrCatchUnhandledExceptions();
int32_t STORMAPI SErrDestroy();
void SErrDisplayAppFatal(const char* format, ...);
[[noreturn]] void SErrDisplayAppFatalMessage(const char* message, const char* filename, int32_t linenumber);
void SErrDisplayAppFatalCustom(uint32_t errorcode, const char* format, ...);
void SErrDisplayAppFatalCustomV(uint32_t errorcode, const char* format, va_list args);
int32_t STORMAPI SErrDisplayError(uint32_t errorcode, const char* filename, int32_t linenumber, const char* description, int32_t recoverable, uint32_t exitcode, uint32_t unknown);
int32_t SErrDisplayErrorFmt(uint32_t errorcode, const char* filename, int32_t linenumber, int32_t recoverable, uint32_t exitcode, const char* format, ...);
int32_t STORMAPI SErrGetErrorStr(uint32_t errorcode, char* buffer, uint32_t bufferchars);
uint32_t STORMAPI SErrGetLastError();
int32_t STORMAPI SErrGetLogLastPath(char* buf, uint32_t size);
void SErrInitialize();
int32_t STORMAPI SErrIsDisplayingError();
void STORMAPI SErrPrepareAppFatal(const char* filename, int32_t linenumber);
void STORMAPI SErrRegisterHandler(int32_t (STORMAPI* handler)(uint32_t, const char*, const char*, int32_t, const char*));
void STORMAPI SErrReportNamedResourceLeak(const char* handlename, const char* resourcename);
void STORMAPI SErrSetLastError(uint32_t errorcode);
void STORMAPI SErrSetLogCallback(int32_t (STORMAPI* callback)(char*, uint32_t));
void STORMAPI SErrSetLogTitleString(const char* title);

int32_t STORMAPI SEvtDestroy();
int32_t STORMAPI SEvtUnregisterType(uint32_t type, uintptr_t subtype);

void STORMAPI SLogClose(HSLOG log);
int32_t STORMAPI SLogCreate(const char* filename, uint32_t flags, HSLOG* log);
void STORMAPI SLogDestroy();
void STORMAPI SLogFlush(HSLOG log);
void STORMAPI SLogFlushAll();
void STORMAPI SLogInitialize();
void STORMAPI SLogVWrite(HSLOG log, const char* format, va_list arglist);
void SLogWrite(HSLOG log, const char* format, ...);

void SDebugPrintf(const char* format, ...);

struct MEMREPORTENTRY {
    /* 0000 */ uint8_t unused_0000[16];
    /* 0010 */ uint32_t unk_0010;
    /* 0014 */ uint32_t unk_0014;
    /* 0018 */ uint8_t unused_0018[8];
    /* 0020 */ uint32_t unk_0020;
    /* 0024 */ char unk_0024[1];
};

extern int32_t g_memFullError;

void* STORMAPI SMemAlloc(size_t bytes, const char* filename, int32_t linenumber, uint32_t flags);
int32_t STORMAPI SMemDestroy();
int32_t STORMAPI SMemFree(void* ptr, const char* filename, int32_t linenumber, uint32_t flags);
int32_t STORMAPI SMemIsValidPointer(void* ptr, size_t bytes, uint32_t flags);
void STORMAPI SMemInitialize();
void* STORMAPI SMemReAlloc(void* ptr, size_t bytes, const char* filename, int32_t linenumber, uint32_t flags);
void STORMAPI SMemSetDebugFlags(uint32_t flags, uint8_t changeMask);
void SMemGenerateReport(SMEMREPORTTYPE type, void (STORMAPI* callback)(void*, const MEMREPORTENTRY*), void* context);

int32_t STORMAPI SRegGetBaseKey(uint32_t flags, char* buffer, uint32_t bufferchars);
int32_t STORMAPI SRegLoadString(const char* keyname, const char* valuename, uint32_t flags, char* buffer, uint32_t buffersize);
int32_t STORMAPI SRegLoadValue(const char* keyname, const char* valuename, uint32_t flags, uint32_t* value);
int32_t STORMAPI SRegSaveString(const char* keyname, const char* valuename, uint32_t flags, const char* string);
int32_t STORMAPI SRegSaveValue(const char* keyname, const char* valuename, uint32_t flags, uint32_t value);

void STORMAPI SRgnCombineRectf(HSRGN handle, const RECTF* rect, void* param, int32_t combinemode);
void STORMAPI SRgnCreate(HSRGN* handle, uint32_t reserved);
void STORMAPI SRgnDelete(HSRGN handle);
void STORMAPI SRgnDestroy();
void STORMAPI SRgnGetBoundingRectf(HSRGN handle, RECTF* rect);

int32_t SSignatureVerify(const uint8_t* data, uint32_t size, const uint8_t* modulus, uint32_t modulusSize, const uint8_t* pubExponent, uint32_t pubExponentSize);
void SSignatureVerifyStream_Begin(SSignatureData** token, uint32_t modulusSize, uint32_t pubExponentSize);
int32_t SSignatureVerifyStream_Finish(SSignatureData* token, const uint8_t* modulus, const uint8_t* pubExponent);
uint32_t SSignatureVerifyStream_GetSignatureLength(SSignatureData* token);
void SSignatureVerifyStream_ProvideData(SSignatureData* token, const uint8_t* data, uint32_t size);

const char* STORMAPI SStrAdvanceCharsUTF8(const char* string, size_t chars);
char* SStrChr(const char* string, char search);
char* SStrChrR(const char* string, char search);
int32_t STORMAPI SStrCmp(const char* string1, const char* string2, size_t maxchars);
int32_t STORMAPI SStrCmpI(const char* string1, const char* string2, size_t maxchars);
int32_t STORMAPI SStrCmpUTF8I(const char* string1, const char* string2, size_t maxchars);
size_t STORMAPI SStrCopy(char* dest, const char* source, size_t destsize);
size_t STORMAPI SStrCopyUTF8(char* dest, const char* source, size_t destsize, size_t maxchars);
char* STORMAPI SStrDupA(const char* string, const char* filename, int32_t linenumber);
uint32_t STORMAPI SStrHash(const char* string, uint32_t flags, uint32_t seed);
uint32_t STORMAPI SStrHashHT(const char* string);
void STORMAPI SStrDestroy();
void STORMAPI SStrInitialize();
size_t STORMAPI SStrLen(const char* string);
size_t STORMAPI SStrLen(const uint16_t* string);
size_t STORMAPI SStrLenUTF8(const char* string);
size_t STORMAPI SStrLenUTF8(const char* string, size_t maxchars);
void STORMAPI SStrLower(char* string);
size_t STORMAPI SStrPack(char* dest, const char* source, size_t destsize);
size_t SStrPrintf(char* dest, size_t maxchars, const char* format, ...);
int32_t STORMAPI SStrSortUTF8I(const char* string1, const char* string2, size_t maxchars);
char* SStrStr(const char* string, const char* search);
char* SStrStrI(const char* string, const char* search);
const char* SStrStrUTF8I(const char* string, const char* search);
float STORMAPI SStrToFloat(const char* string);
int32_t STORMAPI SStrToInt(const char* string);
int64_t STORMAPI SStrToInt64(const char* string);
uint32_t STORMAPI SStrToUnsigned(const char* string);
uint32_t STORMAPI SStrToUnsigned(const char** string);
void STORMAPI SStrTokenize(const char** string, char* buffer, size_t bufferchars, const char* whitespace, int32_t* quoted);
void STORMAPI SStrUpper(char* string);
size_t SStrVPrintf(char* dest, size_t maxchars, const char* format, va_list arglist);

char* Int64ToString(int64_t num, char* buf, size_t destsize);
int32_t vsnoprintf(char* out, size_t outSize, const char* format, va_list argumentList);
int32_t vsoprintf(char* out, const char* format, va_list argumentList);

void* SCreateThread(uint32_t dwStackSize, uint32_t (STORMAPI* lpStartAddress)(void*), void* lpParameter, uint32_t dwCreationFlags, uint32_t* lpThreadId, char* threadName);
void* STORMAPI SCreateThread(uint32_t (STORMAPI* lpStartAddress)(void*), void* lpParameter, uint32_t* lpThreadId, void* linuxData, char* threadName, uint32_t stackSize);
int32_t STORMAPI SCreateProcess(const char* appName, char* commandLine, void (*callbackWhenProcessCompletes)(void*), void* callbackData);
uint32_t STORMAPI SGetCurrentProcessId();
uint32_t STORMAPI SGetCurrentThreadId();
int32_t STORMAPI SGetCurrentThreadPriority();
int32_t STORMAPI SGetThreadName(uint32_t threadId, char* buffer, int32_t bufferchars);
void SServerDestroy();
void SServerInitialize();
int32_t STORMAPI SSetCurrentProcessAffinityMask(size_t mask);
int32_t STORMAPI SSetCurrentThreadPriority(int32_t priority);

int32_t STORMAPI SUniConvertUTF16to8(char* dst, uint32_t dstmaxchars, const uint16_t* src, uint32_t srcmaxchars, uint32_t* dstchars, uint32_t* srcchars);
int32_t STORMAPI SUniConvertUTF8to16(uint16_t* dst, uint32_t dstmaxchars, const char* src, uint32_t srcmaxchars, uint32_t* dstchars, uint32_t* srcchars);
int32_t STORMAPI SUniConvertUTF8to16Len(const char* src, uint32_t srcmaxchars, uint32_t* srcchars);

int32_t STORMAPI SMsgDestroy();
HWND__* STORMAPI SMsgGetDefaultWindow();

int32_t STORMAPI zlib_compress(uint8_t* dest, uint32_t* destLen, const uint8_t* source, uint32_t sourceLen, int32_t level);
int32_t STORMAPI zlib_uncompress(uint8_t* dest, uint32_t* destLen, const uint8_t* source, uint32_t sourceLen);

int32_t StormCallService(int32_t selector, ...);
void STORMAPI StormDestroy();
[[noreturn]] void StormExit(int32_t code);
[[noreturn]] void StormExitProcess(int32_t code);
HINSTANCE__* STORMAPI StormGetInstance();
void STORMAPI StormInitialize();
void StormRtlDestroy();
void StormRtlInitialize();
int32_t STORMAPI StormSetOption(int32_t optname, void* optval, uint32_t optlen);

#ifdef NDEBUG
inline void DebugLog(const char* format, ...) {
}

inline void DebugLog(int32_t target, const char* format, ...) {
}

inline void DebugValidate(const char* expression) {
}
#else
void DebugLog(const char* format, ...);
void DebugLog(int32_t target, const char* format, ...);
void DebugValidate(const char* expression);
#endif

// INLINED
inline void* operator new(size_t bytes, const char* filename, int32_t linenumber) {
    return SMemAlloc(bytes, filename, linenumber, 0);
}

// INLINED
inline void operator delete(void* ptr, const char* filename, int32_t linenumber) {
    SMemFree(ptr, filename, linenumber, 0);
}

// INLINED
inline void* operator new[](size_t bytes, const char* filename, int32_t linenumber) {
    return SMemAlloc(bytes, filename, linenumber, 0);
}

// INLINED
inline void operator delete[](void* ptr, const char* filename, int32_t linenumber) {
    SMemFree(ptr, filename, linenumber, 0);
}

#endif
