#include "WoW/Source/DB/WowClientDB.h"

// STATIC: 0x00C5DEA0
int32_t g_clientDbPerformCompress;

// INLINED
WowClientDB_Base::WowClientDB_Base() {
    this->m_loaded = 0;
    this->m_numRecords = 0;
    this->m_maxID = -1;
    this->m_minID = 0xFFFFFFF;
    this->m_strings = nullptr;
}

// VERIFY: b5021150 manual
// OFFSET: 0x008C6780
WowClientDB_Base::~WowClientDB_Base() {
}

// VERIFY: 60c92aa6 auto
// OFFSET: 0x004CFBB0
uint8_t* DecompressRow(const uint8_t* src, uint32_t size, uint8_t* dst) {
    uint8_t* end = &dst[size];
    *dst = *src;
    uint8_t* out = dst + 1;

    for (const uint8_t* p = src + 1; out < end; p++) {
        *out++ = *p;

        if (*p == p[-1]) {
            int32_t run = p[1];

            while (run) {
                run--;
                *out++ = *p;
            }

            p += 2;

            if (out < end) {
                *out++ = *p;
            }
        }
    }

    return out;
}

// VERIFY: bbb5632f manual
// OFFSET: 0x0095D980
uint32_t CompressRow(const uint8_t* src, uint32_t size, uint8_t* dst) {
    uint8_t* out = dst;
    const uint8_t* end = &src[size];

    if (dst) {
        *dst = *src;
        out = dst + 1;
    }

    uint32_t count = 1;

    for (const uint8_t* p = src + 1; p < end; p++) {
        if (out) {
            *out++ = *p;
        }

        uint8_t value = *p;
        count++;

        if (*p == p[-1]) {
            p++;

            uint32_t run;

            for (run = 0; p < end; run++) {
                if (*p != value || run >= 0xFF) {
                    break;
                }

                p++;
            }

            if (out) {
                *out++ = static_cast<uint8_t>(run);
            }

            count++;

            if (p == end) {
                return count;
            }

            if (out) {
                *out++ = *p;
            }

            count++;
        }
    }

    return count;
}
