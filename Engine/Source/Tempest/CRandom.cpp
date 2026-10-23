#include <cmath>
#include "Engine/Source/Tempest/CMath.h"
#include <cstdint>
#include <cstdlib>
#include "Engine/Source/Tempest/CRandom.h"

// STATIC: 0x009F1700
static const uint32_t s_rndTable[64] = {
    0x9927148E, 0x08C7AAFD, 0x1F3EE6D5, 0xDA55BBF6, 0x6A4AA075, 0xFF97BDE8, 0x9FBC9BDE, 0x46A18A81,
    0x63E30B6E, 0x5D6C7A76, 0xCA69D388, 0x25B947C3, 0x3FA2AB83, 0xBA7C41A6, 0x0195ACE5, 0xC109CF7E,
    0x717062D9, 0x0205DB8D, 0x54EF8724, 0x3037D4C6, 0x7BCB1BD0, 0xECD8E4B8, 0xDCADCE49, 0xC494A913,
    0x0DAE398F, 0x0EDD5218, 0x85F5FA78, 0x6DAFD258, 0x3B53B2A4, 0xBE50A551, 0x11F42DFC, 0xF1169848,
    0x663DDF86, 0x2F2E445E, 0x176B0736, 0xB64C298B, 0xE75F89E2, 0xE121A7CD, 0xED65C94D, 0x239CEEFE,
    0x04B77D33, 0x402A9A9E, 0xF35B10B3, 0x921C7782, 0x571E4E20, 0x8C067222, 0xFB732C67, 0xBF0AC259,
    0x0CF95C79, 0x68121A28, 0x42193474, 0xF884C0B1, 0x9D15F038, 0x6F3AF260, 0x91EB90B4, 0x61357F1D,
    0x5603325A, 0x932BC5A3, 0x434B0F80, 0x3CE0A8F7, 0x2664D196, 0x4FCC45D7, 0xB5E9B0C8, 0xEA31D600
};

// VERIFY: bd1b3a6b manual
// OFFSET: 0x00464580
uint32_t CRandom::uint32_(CRndSeed& seed) {
    uint32_t vls = seed.rndvls;
    uint32_t acc = seed.rndacc;

    int32_t i3 = static_cast<int32_t>(vls >> 24) - 4;
    int32_t i2 = static_cast<int32_t>((vls >> 16) & 0xFF) - 12;
    int32_t i1 = static_cast<int32_t>((vls >> 8) & 0xFF) - 24;
    int32_t i0 = static_cast<int32_t>(vls & 0xFF) - 28;

    if (i3 < 0) {
        i3 += 188;
    }

    if (i2 < 0) {
        i2 += 212;
    }

    if (i1 < 0) {
        i1 += 236;
    }

    if (i0 < 0) {
        i0 += 244;
    }

    const uint8_t* table = reinterpret_cast<const uint8_t*>(s_rndTable);
    uint32_t t3 = *reinterpret_cast<const uint32_t*>(table + i3);
    uint32_t t2 = *reinterpret_cast<const uint32_t*>(table + i2);
    uint32_t t1 = *reinterpret_cast<const uint32_t*>(table + i1);
    uint32_t t0 = *reinterpret_cast<const uint32_t*>(table + i0);

    acc += _rotl(t3, 1) ^ _rotl(t2, 2) ^ _rotl(t1, 3) ^ t0;

    seed.rndvls = (i3 << 24) | (i2 << 16) | (i1 << 8) | i0;
    seed.rndacc = acc;
    return acc;
}

// INLINED
CRndSeed::CRndSeed(uint32_t seed) {
    this->SetSeed(seed);
}

// VERIFY: d644d7cb manual
// OFFSET: 0x004C1510
void CRndSeed::SetSeed(uint32_t seed) {
    this->rndacc = seed;
    this->rndvls = ((seed % 59) << 10) | ((seed % 61) << 2) | ((seed % 53) << 18) | ((seed % 47) << 26);
}

// VERIFY: cab91f8f manual
// OFFSET: 0x004C1590
float CRandom::ufloat_(CRndSeed& seed) {
    uint32_t bits = (CRandom::uint32_(seed) & 0x7FFFFF) | 0x3F800000;

    return *reinterpret_cast<float*>(&bits) - 1.0f;
}

// VERIFY: 1fb5680e manual
// OFFSET: 0x004C15C0
float CRandom::sfloat_(CRndSeed& seed) {
    uint32_t value = CRandom::uint32_(seed);
    uint32_t bits = (value & 0x7FFFFF) | 0x3F800000;
    float f = *reinterpret_cast<float*>(&bits);

    if (static_cast<int32_t>(value) >= 0) {
        return f - 2.0f;
    }

    return 2.0f - f;
}

// VERIFY: 4687ef93 manual
// OFFSET: 0x004C1600
uint32_t CRandom::dice_(uint32_t sides, CRndSeed& seed) {
    return static_cast<uint32_t>((static_cast<uint64_t>(sides) * CRandom::uint32_(seed)) >> 32);
}

// VERIFY: de69b9b7 manual
// OFFSET: 0x004C1620
C2Vector CRandom::C2Vector_(CRndSeed& seed) {
    float angle = CRandom::ufloat_(seed) * TWO_PI;
    float c = cos(angle);
    float s = sin(angle);

    C2Vector result;
    result.x = c;
    result.y = s;
    return result;
}

// VERIFY: f1a8f83a manual
// OFFSET: 0x004C1680
C3Vector CRandom::C3Vector_(CRndSeed& seed) {
    float z = CRandom::sfloat_(seed);
    float angle = CRandom::ufloat_(seed) * TWO_PI;
    float r = sqrt(1.0f - z * z);
    float c = cos(angle);
    float s = sin(angle);

    return C3Vector(c * r, r * s, z);
}

// VERIFY: 2db79c5a manual
// OFFSET: 0x00982310
float __stdcall CRandom::ufloat_(float range, CRndSeed& seed) {
    return CRandom::ufloat_(seed) * range;
}
