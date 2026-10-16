#include <cstdint>
#include <cstring>
#if defined(_MSC_VER)
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "RSA.h"
#include "SSignature.h"
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: 95b95c12 auto
// OFFSET: 0x00770C90
uint32_t SSignatureVerifyStream_GetSignatureLength(SSignatureData* token) {
    return token->magicBufferSize;
}

// VERIFY: e0416eee auto
// OFFSET: 0x00770CA0
void SSignatureVerifyStream_ProvideData(SSignatureData* token, const uint8_t* data, uint32_t size) {
    int32_t excess = size - token->magicBufferSize;

    if (excess >= 0) {
        if (token->magicBufferUsed) {
            token->sha.Append(token->magicBuffer, token->magicBufferUsed);
        }

        if (excess > 0) {
            token->sha.Append(data, excess);
        }

        memcpy(token->magicBuffer, data + excess, token->magicBufferSize);
        token->magicBufferUsed = token->magicBufferSize;
    } else {
        int32_t flush = size + token->magicBufferUsed - token->magicBufferSize;

        if (flush > 0) {
            token->sha.Append(token->magicBuffer, flush);
            token->magicBufferUsed -= flush;
            memmove(token->magicBuffer, token->magicBuffer + flush, token->magicBufferUsed);
        }

        memcpy(token->magicBuffer + token->magicBufferUsed, data, size);
        token->magicBufferUsed += size;
    }
}

// VERIFY: 4d755db1 manual
// OFFSET: 0x00770D50
void SSignatureVerifyStream_Begin(SSignatureData** token, uint32_t modulusSize, uint32_t pubExponentSize) {
    *token = static_cast<SSignatureData*>(SMemAlloc(sizeof(SSignatureData), __FILE__, __LINE__, 0));
    (*token)->modulusSize = modulusSize;
    (*token)->pubExponentSize = pubExponentSize;
    (*token)->magicBufferUsed = 0;
    (*token)->magicBufferSize = modulusSize + 4;
    (*token)->magicBuffer = static_cast<uint8_t*>(SMemAlloc((*token)->magicBufferSize, __FILE__, __LINE__, 0x8));
    (*token)->sha.Initialize();
}

// VERIFY: 877d78b4 manual
// OFFSET: 0x00770DB0
int32_t SSignatureVerifyStream_Finish(SSignatureData* token, const uint8_t* modulus, const uint8_t* pubExponent) {
    int32_t result = 0;

    if (token->magicBufferUsed == token->magicBufferSize && *reinterpret_cast<uint32_t*>(token->magicBuffer) == 'SIGN') {
        uint8_t* generated = static_cast<uint8_t*>(alloca(token->modulusSize));
        memset(generated, 0xBB, token->modulusSize);
        generated[token->modulusSize - 1] = 0x0B;
        token->sha.Finalize(generated);

        uint8_t* stored = static_cast<uint8_t*>(alloca(token->modulusSize));
        memcpy(stored, token->magicBuffer + 4, token->modulusSize);

        Crypt::RSA decoder;
        decoder.Prepare(modulus, token->modulusSize, pubExponent, token->pubExponentSize);
        decoder.Process(stored, token->modulusSize);
        result = memcmp(stored, generated, token->modulusSize) == 0;
    }

    SMemFree(token->magicBuffer, __FILE__, __LINE__, 0);
    delete token;

    return result;
}

// VERIFY: 9d03c903 auto
// OFFSET: 0x00770F30
int32_t SSignatureVerify(const uint8_t* data, uint32_t size, const uint8_t* modulus, uint32_t modulusSize, const uint8_t* pubExponent, uint32_t pubExponentSize) {
    SSignatureData* token;
    SSignatureVerifyStream_Begin(&token, modulusSize, pubExponentSize);
    SSignatureVerifyStream_ProvideData(token, data, size);

    return SSignatureVerifyStream_Finish(token, modulus, pubExponent);
}
