#include "SBig.h"
#include <cstdint>
#include <cstring>
#if defined(__APPLE__)
#include <alloca.h>
#define _alloca alloca
#else
#include <malloc.h>
#endif
#include <vector>

// INLINED
inline uint32_t ExtractLowPart(uint64_t& value) {
    uint32_t low = static_cast<uint32_t>(value);
    value >>= 32;
    return low;
}

// INLINED
inline uint32_t ExtractLowPartSx(uint64_t& value) {
    uint32_t low = static_cast<uint32_t>(value);
    value >>= 32;

    if (value >= 0x80000000) {
        value |= 0xFFFFFFFF00000000ULL;
    }

    return low;
}

// INLINED
inline uint32_t ExtractLowPartLargeSum(uint64_t& value, uint64_t add) {
    value += add;
    uint32_t carry = value < add;
    uint32_t low = ExtractLowPart(value);
    value += static_cast<uint64_t>(carry) << 32;
    return low;
}

// INLINED
inline void InsertLowPart(uint64_t& value, uint32_t low) {
    value = (value << 32) + low;
}

// INLINED
inline uint64_t MakeLarge(uint32_t low, uint32_t high) {
    return low + (static_cast<uint64_t>(high) << 32);
}

// VERIFY: 1f93de79 manual
// OFFSET: 0x00443D30
Blizzard::BigBuffer::BigBuffer() {
    this->m_offset = 0;
}

// VERIFY: f4f2b38e manual
// OFFSET: 0x00446260
uint32_t& Blizzard::BigBuffer::operator[](uint32_t index) {
    this->GrowToFit(index);
    return this->m_data[index + this->m_offset];
}

// VERIFY: e08d76c1 manual
// OFFSET: 0x00431060
uint32_t Blizzard::BigBuffer::operator[](uint32_t index) const {
    return this->IsUsed(index) ? this->m_data[index + this->m_offset] : 0;
}

// INLINED
inline void Blizzard::BigBuffer::Clear() {
    this->m_data.resize(this->m_offset, 0);
}

// VERIFY: e1bff0cb manual
// OFFSET: 0x0042BBA0
uint32_t Blizzard::BigBuffer::Count() const {
    return static_cast<uint32_t>(this->m_data.size()) - this->m_offset;
}

// INLINED
inline void Blizzard::BigBuffer::GrowToFit(uint32_t index) {
    uint32_t count = index + this->m_offset + 1;
    uint32_t size = static_cast<uint32_t>(this->m_data.size());
    this->m_data.resize(count < size ? size : count, 0);
}

// INLINED
inline int32_t Blizzard::BigBuffer::IsUsed(uint32_t index) const {
    return index + this->m_offset < this->m_data.size();
}

// INLINED
inline void Blizzard::BigBuffer::SetCount(uint32_t count) {
    this->m_data.resize(count + this->m_offset, 0);
}

// INLINED
inline void Blizzard::BigBuffer::SetOffset(uint32_t offset) {
    this->m_offset = offset;

    if (offset) {
        this->GrowToFit(0xFFFFFFFF);
    }
}

// VERIFY: e1005b4f manual
// OFFSET: 0x004395D0
void Blizzard::BigBuffer::Trim() const {
    while (this->Count() && !this->m_data.back()) {
        this->m_data.pop_back();
    }
}

// INLINED
inline Blizzard::BigStack::BigStack() {
    this->m_used = 0;
}

// INLINED
inline Blizzard::BigStack::~BigStack() {
}

// INLINED
inline Blizzard::BigBuffer& Blizzard::BigStack::Alloc(uint32_t* count) {
    if (count) {
        ++*count;
    }

    return this->m_buffer[this->m_used++];
}

// INLINED
inline void Blizzard::BigStack::Free(uint32_t count) {
    this->m_used -= count;
}

// INLINED
inline Blizzard::BigBuffer& Blizzard::BigStack::MakeDistinct(BigBuffer& orig, int32_t required) {
    return required ? this->Alloc(nullptr) : orig;
}

// INLINED
inline void Blizzard::BigStack::UnmakeDistinct(BigBuffer& orig, BigBuffer& distinct) {
    if (&orig != &distinct) {
        orig = distinct;
        this->Free(1);
    }
}

// INLINED
inline std::vector<uint8_t>& Blizzard::BigData::Output() const {
    return this->m_output;
}

// INLINED
inline Blizzard::BigBuffer& Blizzard::BigData::Primary() {
    return this->m_primary;
}

// INLINED
inline const Blizzard::BigBuffer& Blizzard::BigData::Primary() const {
    return this->m_primary;
}

// INLINED
inline Blizzard::BigStack& Blizzard::BigData::Stack() const {
    return this->m_stack;
}

// VERIFY: 693c6ebe manual
// OFFSET: 0x004310B0
static int32_t Compare(const Blizzard::BigBuffer& a, const Blizzard::BigBuffer& b) {
    int32_t result = 0;

    for (uint32_t i = 0; a.IsUsed(i) || b.IsUsed(i); ++i) {
        if (a[i] != b[i]) {
            result = b[i] < a[i] ? 1 : -1;
        }
    }

    return result;
}

// VERIFY: cb9622bc manual
// OFFSET: 0x00431220
static uint32_t HighBitPos(const Blizzard::BigBuffer& a) {
    uint32_t index = a.Count();

    while (index) {
        --index;

        if (a[index]) {
            uint32_t mask = 0x80000000;

            for (uint32_t bit = 32; bit; mask >>= 1) {
                --bit;

                if (a[index] & mask) {
                    return bit + 32 * index;
                }
            }
        }
    }

    return 0;
}

// VERIFY: 4e76115e manual
// OFFSET: 0x004462D0
static void Div(Blizzard::BigBuffer& a, uint32_t* b, const Blizzard::BigBuffer& c, uint64_t d) {
    a.SetCount(c.Count());

    uint64_t data = 0;
    uint32_t i = c.Count();

    while (i) {
        --i;
        InsertLowPart(data, c[i]);
        a[i] = static_cast<uint32_t>(data / d);
        data %= d;
    }

    a.Trim();
    *b = static_cast<uint32_t>(data);
}

// VERIFY: 93bc6b11 manual
// OFFSET: 0x00446420
static void Mul(Blizzard::BigBuffer& a, const Blizzard::BigBuffer& b, uint64_t c) {
    uint64_t carry = 0;
    uint32_t i = 0;

    for (; carry || b.IsUsed(i); ++i) {
        carry += b[i] * c;
        a[i] = ExtractLowPart(carry);
    }

    a.SetCount(i);
}

// VERIFY: 82495513 manual
// OFFSET: 0x004464F0
static void Mul(Blizzard::BigBuffer& a, const Blizzard::BigBuffer& b, const Blizzard::BigBuffer& c, Blizzard::BigStack& stack) {
    Blizzard::BigBuffer& aa = stack.MakeDistinct(a, &a == &b || &a == &c);
    aa.Clear();

    for (uint32_t bIndex = 0; b.IsUsed(bIndex); ++bIndex) {
        uint64_t carry = 0;
        uint32_t cIndex = 0;

        for (; c.IsUsed(cIndex); ++cIndex) {
            carry += aa[bIndex + cIndex] + static_cast<uint64_t>(b[bIndex]) * c[cIndex];
            aa[bIndex + cIndex] = ExtractLowPart(carry);
        }

        aa[bIndex + cIndex] = ExtractLowPart(carry);
    }

    stack.UnmakeDistinct(a, aa);
}

// VERIFY: d5fbc759 manual
// OFFSET: 0x004467B0
static void Shl(Blizzard::BigBuffer& a, const Blizzard::BigBuffer& b, uint32_t c) {
    uint32_t cWords = c >> 5;
    uint32_t cBits = c & 0x1F;
    uint32_t aCount = b.Count() + cWords + 1;
    uint32_t aIndex = aCount;

    while (aIndex) {
        --aIndex;
        uint32_t value = 0;

        if (aIndex >= cWords) {
            value = b[aIndex - cWords] << cBits;

            if (aIndex > cWords) {
                if (cBits) {
                    value += b[aIndex - cWords - 1] >> (32 - cBits);
                }
            }
        }

        a[aIndex] = value;
    }

    a.SetCount(aCount);
    a.Trim();
}

// VERIFY: 69d2980e manual
// OFFSET: 0x00446940
static void Shr(Blizzard::BigBuffer& a, const Blizzard::BigBuffer& b, uint32_t c) {
    uint32_t cWords = c >> 5;
    uint32_t cBits = c & 0x1F;
    uint32_t i = 0;

    for (; b.IsUsed(i + cWords); ++i) {
        uint32_t value = b[i + cWords] >> cBits;

        if (cBits) {
            value += b[i + cWords + 1] << (32 - cBits);
        }

        a[i] = value;
    }

    a.SetCount(i);
}

// VERIFY: 4b844ac8 manual
// OFFSET: 0x00446A40
static void Square(Blizzard::BigBuffer& a, const Blizzard::BigBuffer& b, Blizzard::BigStack& stack) {
    Blizzard::BigBuffer& aa = stack.MakeDistinct(a, &a == &b);
    aa.Clear();

    for (uint32_t i = 0; b.IsUsed(i); ++i) {
        uint64_t carry = 0;
        uint32_t j = 0;

        for (; j <= i; ++j) {
            uint64_t mul = static_cast<uint64_t>(b[i]) * b[j];
            uint64_t add = mul + aa[i + j];

            if (j < i) {
                carry += mul;
            }

            aa[i + j] = ExtractLowPartLargeSum(carry, add);
        }

        aa[i + j] = ExtractLowPart(carry);
    }

    stack.UnmakeDistinct(a, aa);
}

// VERIFY: 0d85ba57 manual
// OFFSET: 0x00446D00
static void Sub(Blizzard::BigBuffer& a, const Blizzard::BigBuffer& b, const Blizzard::BigBuffer& c) {
    uint64_t borrow = 0;
    uint32_t i = 0;

    for (; b.IsUsed(i) || c.IsUsed(i); ++i) {
        borrow += b[i] - static_cast<uint64_t>(c[i]);
        a[i] = ExtractLowPartSx(borrow);
    }

    a.SetCount(i);
}

// INLINED
inline void SetZero(Blizzard::BigBuffer& a) {
    a.Clear();
}

// INLINED
inline void SetOne(Blizzard::BigBuffer& a) {
    a.SetCount(1);
    a[0] = 1;
}

// VERIFY: 3076dfa4 auto
// OFFSET: 0x00446E30
static void FromBinary(Blizzard::BigBuffer& a, const void* data, uint32_t bytes) {
    a.Clear();

    for (uint32_t i = 0; i < bytes; ++i) {
        uint32_t byte = static_cast<const uint8_t*>(data)[i];
        a[i >> 2] = ((i & 3) ? a[i >> 2] : 0) + (byte << (8 * (i & 3)));
    }
}

// VERIFY: 2972bf63 manual
// OFFSET: 0x00443D50
static void ToBinaryAppend(std::vector<uint8_t>& output, const Blizzard::BigBuffer& a) {
    for (uint32_t i = 0; i < 4 * a.Count(); ++i) {
        uint32_t value = a[i >> 2] >> (8 * (i & 3));

        if (value || (i >> 2) + 1 < a.Count()) {
            output.push_back(static_cast<uint8_t>(value));
        }
    }
}

// INLINED
inline void ToBinary(std::vector<uint8_t>& output, const Blizzard::BigBuffer& a) {
    output.clear();
    ToBinaryAppend(output, a);
}

// VERIFY: a5083fc6 manual
// OFFSET: 0x004494A0
static void Div(Blizzard::BigBuffer& a, Blizzard::BigBuffer& b, const Blizzard::BigBuffer& c, const Blizzard::BigBuffer& d, Blizzard::BigStack& stack) {
    c.Trim();
    d.Trim();

    uint32_t cCount = c.Count();
    uint32_t dCount = d.Count();

    if (dCount > cCount) {
        b = c;
        SetZero(a);
        return;
    }

    if (dCount <= 1) {
        Div(a, &b[0], c, d[0]);
        b.SetCount(1);
        return;
    }

    uint32_t allocCount = 0;
    Blizzard::BigBuffer& cc = stack.Alloc(&allocCount);
    Blizzard::BigBuffer& dd = stack.Alloc(&allocCount);
    Blizzard::BigBuffer& work = stack.Alloc(&allocCount);

    uint32_t shift = 31 - (HighBitPos(d) & 0x1F);
    Shl(cc, c, shift);
    Shl(dd, d, shift);

    uint32_t t = dd[dCount - 1] + 1;
    uint32_t index = cCount - dCount + 1;
    a.SetCount(index);

    while (index) {
        --index;
        a.SetOffset(index);
        cc.SetOffset(index);

        if (t) {
            a[0] = static_cast<uint32_t>(MakeLarge(cc[dCount - 1], cc[dCount]) / t);
        } else {
            a[0] = cc[dCount];
        }

        if (a[0]) {
            Mul(work, dd, a[0]);
            Sub(cc, cc, work);
        }

        while (cc[dCount] || Compare(cc, dd) >= 0) {
            ++a[0];
            Sub(cc, cc, dd);
        }
    }

    Shr(b, cc, shift);
    b.Trim();
    stack.Free(allocCount);
}

// INLINED
inline void MulMod(Blizzard::BigBuffer& a, const Blizzard::BigBuffer& b, const Blizzard::BigBuffer& c, const Blizzard::BigBuffer& d, Blizzard::BigStack& stack) {
    uint32_t allocCount = 0;
    Blizzard::BigBuffer& temp = stack.Alloc(&allocCount);
    Mul(temp, b, c, stack);
    Div(temp, a, temp, d, stack);
    stack.Free(allocCount);
}

// VERIFY: 49fdfca5 manual
// OFFSET: 0x00449820
static void PowMod(Blizzard::BigBuffer& a, const Blizzard::BigBuffer& b, const Blizzard::BigBuffer& c, const Blizzard::BigBuffer& d, Blizzard::BigStack& stack) {
    c.Trim();

    if (!c.Count()) {
        SetOne(a);
        return;
    }

    uint32_t allocCount = 0;
    Blizzard::BigBuffer& temp = stack.Alloc(&allocCount);
    Blizzard::BigBuffer& b2 = stack.Alloc(&allocCount);
    Blizzard::BigBuffer& b3 = stack.Alloc(&allocCount);
    Blizzard::BigBuffer& aa = stack.MakeDistinct(a, &a == &b || &a == &c || &a == &d);

    MulMod(b2, b, b, d, stack);
    MulMod(b3, b2, b, d, stack);

    const Blizzard::BigBuffer* bPower[3] = { &b, &b2, &b3 };
    SetOne(aa);

    uint32_t cIndex = c.Count();

    while (cIndex) {
        --cIndex;
        uint32_t ci = c[cIndex];
        uint32_t ciBits = 32;

        if (cIndex + 1 == c.Count()) {
            while (!(ci & 0xC0000000)) {
                ci <<= 2;
                ciBits -= 2;
            }
        }

        for (uint32_t bit = 0; bit < ciBits; bit += 2) {
            Square(aa, aa, stack);
            Div(temp, aa, aa, d, stack);
            Square(aa, aa, stack);
            Div(temp, aa, aa, d, stack);

            if (ci >> 30) {
                MulMod(aa, aa, *bPower[(ci >> 30) - 1], d, stack);
            }

            ci <<= 2;
        }
    }

    stack.UnmakeDistinct(a, aa);
    stack.Free(allocCount);
}

// INLINED
inline void Blizzard::SBigDel(BigData* num) {
    delete num;
}

// INLINED
inline void Blizzard::SBigFromBinary(BigData* num, const void* data, uint32_t bytes) {
    FromBinary(num->Primary(), data, bytes);
}

// VERIFY: 78e6fe12 manual
// OFFSET: 0x00446EB0
void Blizzard::SBigNew(BigData** num) {
    *num = new BigData;
}

// INLINED
inline void Blizzard::SBigPowMod(BigData* a, const BigData* b, const BigData* c, const BigData* d) {
    PowMod(a->Primary(), b->Primary(), c->Primary(), d->Primary(), a->Stack());
}

// VERIFY: 38a8ab58 manual
// OFFSET: 0x00446F20
void Blizzard::SBigToBinaryBuffer(const BigData* num, void* data, uint32_t maxBytes, uint32_t* bytes) {
    std::vector<uint8_t>& output = num->Output();
    ToBinary(output, num->Primary());

    uint32_t count = maxBytes;

    if (output.size() < maxBytes) {
        count = static_cast<uint32_t>(output.size());
    }

    memcpy(data, output.data(), count);

    if (bytes) {
        *bytes = count;
    }
}

// INLINED
inline Blizzard::BigNum::BigNum() {
    SBigNew(&this->m_data);
}

// INLINED
inline Blizzard::BigNum::~BigNum() {
    SBigDel(this->m_data);
}

// INLINED
inline void Blizzard::BigNum::FromBinary(const void* data, uint32_t bytes) {
    SBigFromBinary(this->m_data, data, bytes);
}

// INLINED
inline Blizzard::BigNum& Blizzard::BigNum::PowMod(const BigNum& b, const BigNum& c, const BigNum& d) {
    SBigPowMod(this->m_data, b.m_data, c.m_data, d.m_data);
    return *this;
}

// INLINED
inline void* Blizzard::BigNum::ToBinaryBuffer(void* data, uint32_t bytes) const {
    uint32_t written;
    SBigToBinaryBuffer(this->m_data, data, bytes, &written);

    if (written < bytes) {
        memset(static_cast<uint8_t*>(data) + written, 0, bytes - written);
    }

    return data;
}

// INLINED
inline void Blizzard::Crypt::RSA::Prepare(const void* modulus, uint32_t mLength, const void* exponent, uint32_t eLength) {
    this->m_modulus.FromBinary(modulus, mLength);
    this->m_exponent.FromBinary(exponent, eLength);
}

// VERIFY: ce01fdce manual
// OFFSET: 0x0044B0E0
void Blizzard::Crypt::RSA::Process(uint8_t* data, uint32_t length) {
    BigNum src;
    src.FromBinary(data, length);
    BigNum dst;
    dst.PowMod(src, this->m_exponent, this->m_modulus);
    dst.ToBinaryBuffer(data, length);
}

// VERIFY: 518d5063 manual
// OFFSET: 0x00435430
void Blizzard::SSignatureVerifyStream_Begin(SSignatureData** token, uint32_t modulusSize, uint32_t pubExponentSize) {
    *token = new SSignatureData;
    (*token)->modulusSize = modulusSize;
    (*token)->pubExponentSize = pubExponentSize;
    (*token)->magicBufferUsed = 0;
    (*token)->magicBufferSize = modulusSize + 4;
    (*token)->magicBuffer.SetCount((*token)->magicBufferSize);
    memset((*token)->magicBuffer.m_data, 0, (*token)->magicBufferSize);
    (*token)->sha.Prepare();
}

// VERIFY: fb9a0f93 auto
// OFFSET: 0x0042C950
void Blizzard::SSignatureVerifyStream_ProvideData(SSignatureData* token, const uint8_t* data, uint32_t size) {
    int32_t excess = size - token->magicBufferSize;

    if (excess >= 0) {
        if (token->magicBufferUsed) {
            token->sha.Process(token->magicBuffer.m_data, token->magicBufferUsed);
        }

        if (excess > 0) {
            token->sha.Process(data, excess);
        }

        memcpy(token->magicBuffer.m_data, data + excess, token->magicBufferSize);
        token->magicBufferUsed = token->magicBufferSize;
    } else {
        int32_t flush = size + token->magicBufferUsed - token->magicBufferSize;

        if (flush > 0) {
            token->sha.Process(token->magicBuffer.m_data, flush);
            token->magicBufferUsed -= flush;
            memmove(token->magicBuffer.m_data, token->magicBuffer.m_data + flush, token->magicBufferUsed);
        }

        memcpy(token->magicBuffer.m_data + token->magicBufferUsed, data, size);
        token->magicBufferUsed += size;
    }
}

// VERIFY: 75edd647 manual
// OFFSET: 0x0044B2B0
bool Blizzard::SSignatureVerifyStream_Finish(SSignatureData* token, const uint8_t* modulus, const uint8_t* pubExponent) {
    bool result = false;

    if (token->magicBufferUsed == token->magicBufferSize && *reinterpret_cast<uint32_t*>(token->magicBuffer.m_data) == 'SIGN') {
        uint8_t* generated = static_cast<uint8_t*>(alloca(token->modulusSize));
        memset(generated, 0xBB, token->modulusSize);
        generated[token->modulusSize - 1] = 0x0B;
        token->sha.Finish(*reinterpret_cast<SHA1::Digest*>(generated));

        uint8_t* stored = static_cast<uint8_t*>(alloca(token->modulusSize));
        memcpy(stored, token->magicBuffer.m_data + 4, token->modulusSize);

        Crypt::RSA decoder;
        decoder.Prepare(modulus, token->modulusSize, pubExponent, token->pubExponentSize);
        decoder.Process(stored, token->modulusSize);
        result = memcmp(stored, generated, token->modulusSize) == 0;
    }

    if (token->magicBuffer.m_data) {
        delete[] token->magicBuffer.m_data;
    }

    delete token;

    return result;
}
