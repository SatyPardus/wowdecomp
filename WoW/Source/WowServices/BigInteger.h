#ifndef WOW_SOURCE_WOWSERVICES_BIGINTEGER_H
#define WOW_SOURCE_WOWSERVICES_BIGINTEGER_H

#include <cstddef>
#include <cstdint>

class BigData;

typedef BigData* BigInteger;

void BigIntegerAdd(BigInteger a, BigInteger b, BigInteger c);
void BigIntegerAddInt(BigInteger a, BigInteger b, uint32_t value);
uint32_t BigIntegerBitLen(BigInteger a);
int32_t BigIntegerCmp(BigInteger a, BigInteger b);
int32_t BigIntegerCmpInt(BigInteger a, uint32_t value);
void BigIntegerFree(BigInteger a);
BigInteger BigIntegerFromBytes(const void* data, uint32_t bytes);
BigInteger BigIntegerFromInt(uint32_t value);
void BigIntegerMod(BigInteger a, BigInteger b, BigInteger c);
void BigIntegerModExp(BigInteger a, BigInteger b, BigInteger c, BigInteger d);
void BigIntegerMul(BigInteger a, BigInteger b, BigInteger c);
void BigIntegerSub(BigInteger a, BigInteger b, BigInteger c);
uint32_t BigIntegerToBytes(BigInteger a, uint8_t* data, uint32_t maxBytes);

#endif
