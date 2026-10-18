#include "Engine/Source/Base/CRTRandom.h"
#include "RandomSeed.h"
#include <cstdlib>
#include <ctime>
#include "Engine/Source/Tempest/CMath.h"

// VERIFY: 20bd83da manual
// OFFSET: 0x007F6AA0
static void GenerateRandomValues(_RandomSeed* seed) {
    uint32_t a = seed->m_a;
    uint32_t b = seed->m_b;

    for (uint32_t i = 0; i < 256; i += 4) {
        uint32_t x = seed->m_mem[i];
        a = ((a << 19) ^ (a >> 13)) + seed->m_mem[(i + 128) & 0xFF];
        uint32_t y = seed->m_mem[x & 0xFF] + b + a;
        seed->m_mem[i] = y;
        b = seed->m_mem[(y >> 8) & 0xFF] + x;
        seed->m_rsl[i] = b;

        x = seed->m_mem[i + 1];
        a = ((a << 19) ^ (a >> 13)) + seed->m_mem[(i + 129) & 0xFF];
        y = seed->m_mem[x & 0xFF] + b + a;
        seed->m_mem[i + 1] = y;
        b = seed->m_mem[(y >> 8) & 0xFF] + x;
        seed->m_rsl[i + 1] = b;

        x = seed->m_mem[i + 2];
        a = ((a << 19) ^ (a >> 13)) + seed->m_mem[(i + 130) & 0xFF];
        y = seed->m_mem[x & 0xFF] + b + a;
        seed->m_mem[i + 2] = y;
        b = seed->m_mem[(y >> 8) & 0xFF] + x;
        seed->m_rsl[i + 2] = b;

        x = seed->m_mem[i + 3];
        a = ((a << 19) ^ (a >> 13)) + seed->m_mem[(i + 131) & 0xFF];
        y = seed->m_mem[x & 0xFF] + b + a;
        seed->m_mem[i + 3] = y;
        b = seed->m_mem[(y >> 8) & 0xFF] + x;
        seed->m_rsl[i + 3] = b;
    }

    seed->m_b = b;
    seed->m_a = a;
}

// VERIFY: f330564e auto
// OFFSET: 0x007F6C10
static void SeedRandomValues(_RandomSeed* seed, int32_t value) {
    seed->m_b = 0;
    seed->m_a = 0;
    int32_t x = value | 1;

    for (uint32_t i = 0; i < 256; i++) {
        x = 16807 * (x % 127773) - 2836 * (x / 127773);

        if (x < 0) {
            x += 0x7FFFFFFF;
        }

        seed->m_mem[i] = x;
    }

    GenerateRandomValues(seed);
    seed->m_count = 256;
}

// VERIFY: c41f952f manual
// OFFSET: 0x007F6C70
void InitializeRandomSeed(_RandomSeed* seed, uint32_t value) {
    if (value) {
        ClientSrand(value);
    } else {
        ClientSrand(static_cast<uint32_t>(time(nullptr)));
    }

    SeedRandomValues(seed, ClientRand());
}

// VERIFY: 78b9370d manual
// OFFSET: 0x007F6CC0
static double GetRandomFraction(_RandomSeed* seed) {
    int32_t count = seed->m_count--;
    uint32_t value;

    if (count) {
        value = seed->m_rsl[count - 1];
    } else {
        GenerateRandomValues(seed);
        seed->m_count = 255;
        value = seed->m_rsl[255];
    }

    return value * 2.3283064e-10f;
}

// VERIFY: c890b33b manual
// OFFSET: 0x007F6D10
uint32_t GetRandomValue(uint32_t max, _RandomSeed* seed) {
    float value = static_cast<float>(GetRandomFraction(seed) * max);
    uint32_t result = CMath::fint_(value - 0.5f);
    return result < max - 1 ? result : max - 1;
}
