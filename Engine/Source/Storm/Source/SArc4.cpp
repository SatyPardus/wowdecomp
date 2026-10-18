#include <cstdint>
#include <cstring>
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: c3553682 manual
// OFFSET: 0x00774EA0
void SARC4ProcessBuffer(uint8_t* data, uint32_t length, const SARC4Key* inKey, SARC4Key* outKey) {
    if (inKey != outKey) {
        memcpy(outKey, inKey, sizeof(SARC4Key));
    }

    uint8_t* state = outKey->m_state;
    uint8_t x = outKey->m_x;
    uint8_t y = outKey->m_y;
    uint32_t i = 0;

    for (; i < (length & ~3u); i += 4) {
        uint8_t swap;

        x++;
        y += state[x];
        swap = state[x];
        state[x] = state[y];
        state[y] = swap;
        uint32_t stream = state[static_cast<uint8_t>(state[x] + state[y])];

        x++;
        y += state[x];
        swap = state[x];
        state[x] = state[y];
        state[y] = swap;
        stream |= state[static_cast<uint8_t>(state[x] + state[y])] << 8;

        x++;
        y += state[x];
        swap = state[x];
        state[x] = state[y];
        state[y] = swap;
        stream |= state[static_cast<uint8_t>(state[x] + state[y])] << 16;

        x++;
        y += state[x];
        swap = state[x];
        state[x] = state[y];
        state[y] = swap;
        stream |= uint32_t(state[static_cast<uint8_t>(state[x] + state[y])]) << 24;

        // Packet payloads can begin immediately after a 2/3-byte header.
        uint32_t value;
        memcpy(&value, data + i, sizeof(value));
        value ^= stream;
        memcpy(data + i, &value, sizeof(value));
    }

    for (; i < length; i++) {
        x++;
        y += state[x];

        uint8_t swap = state[x];
        state[x] = state[y];
        state[y] = swap;

        data[i] ^= state[static_cast<uint8_t>(state[x] + state[y])];
    }

    outKey->m_y = y;
    outKey->m_x = x;
}

// VERIFY: 765600fe manual
// OFFSET: 0x00775040
void SARC4PrepareKey(const void* key, uint32_t length, SARC4Key* outKey) {
    if (!key) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    const uint8_t* bytes = static_cast<const uint8_t*>(key);

    for (uint32_t i = 0; i < 256; i++) {
        outKey->m_state[i] = i;
    }

    outKey->m_x = 0;
    outKey->m_y = 0;

    uint32_t keyIndex = 0;
    uint8_t j = 0;

    for (uint32_t i = 0; i < 256; i++) {
        j += outKey->m_state[i] + bytes[keyIndex];

        uint8_t swap = outKey->m_state[i];
        outKey->m_state[i] = outKey->m_state[j];
        outKey->m_state[j] = swap;

        keyIndex = (keyIndex + 1) % length;
    }
}
