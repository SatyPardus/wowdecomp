#include "BigInteger.h"
#include <cstring>
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: 06d3b0b1 auto
// OFFSET: 0x009A8AA0
BigInteger BigIntegerFromInt(uint32_t value) {
    BigInteger num;
    SBigNew(&num);
    SBigFromUnsigned(num, value);

    return num;
}

// VERIFY: 7c2fbe3f auto
// OFFSET: 0x009A8AD0
BigInteger BigIntegerFromBytes(const void* data, uint32_t bytes) {
    BigInteger num;
    SBigNew(&num);
    SBigFromBinary(num, data, bytes);

    return num;
}

// VERIFY: 2a883226 auto
// OFFSET: 0x009A8B00
uint32_t BigIntegerToBytes(BigInteger a, uint8_t* data, uint32_t maxBytes) {
    uint32_t bytes;
    SBigToBinaryBuffer(a, data, maxBytes, &bytes);

    if (bytes < maxBytes) {
        memset(&data[bytes], 0, maxBytes - bytes);
    }

    return bytes;
}

// VERIFY: 391518d5 auto
// OFFSET: 0x009A8B40
int32_t BigIntegerCmp(BigInteger a, BigInteger b) {
    return SBigCompare(a, b);
}

// VERIFY: 056d7004 auto
// OFFSET: 0x009A8B60
int32_t BigIntegerCmpInt(BigInteger a, uint32_t value) {
    BigInteger num;
    SBigNew(&num);
    SBigFromUnsigned(num, value);

    int32_t result = SBigCompare(a, num);
    SBigDel(num);

    return result;
}

// VERIFY: 499b1760 auto
// OFFSET: 0x009A8BA0
void BigIntegerAdd(BigInteger a, BigInteger b, BigInteger c) {
    SBigAdd(a, b, c);
}

// VERIFY: 2f861ec5 auto
// OFFSET: 0x009A8BC0
void BigIntegerAddInt(BigInteger a, BigInteger b, uint32_t value) {
    BigInteger num;
    SBigNew(&num);
    SBigFromUnsigned(num, value);
    SBigAdd(a, b, num);
    SBigDel(num);
}

// VERIFY: a631e894 auto
// OFFSET: 0x009A8C00
void BigIntegerSub(BigInteger a, BigInteger b, BigInteger c) {
    SBigSub(a, b, c);
}

// VERIFY: 4294c073 auto
// OFFSET: 0x009A8C20
void BigIntegerMul(BigInteger a, BigInteger b, BigInteger c) {
    SBigMul(a, b, c);
}

// VERIFY: ae6e9c63 auto
// OFFSET: 0x009A8C40
void BigIntegerMod(BigInteger a, BigInteger b, BigInteger c) {
    SBigMod(a, b, c);
}

// VERIFY: 59eb8ef8 auto
// OFFSET: 0x009A8C60
void BigIntegerModExp(BigInteger a, BigInteger b, BigInteger c, BigInteger d) {
    SBigPowMod(a, b, c, d);
}

// VERIFY: 5176e70c auto
// OFFSET: 0x009A8C80
void BigIntegerFree(BigInteger a) {
    SBigDel(a);
}

// VERIFY: 4b4c7966 auto
// OFFSET: 0x009A8C90
uint32_t BigIntegerBitLen(BigInteger a) {
    uint32_t bits;
    SBigBitLen(a, &bits);

    return bits;
}
