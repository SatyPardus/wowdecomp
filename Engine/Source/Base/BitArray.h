#ifndef ENGINE_SOURCE_BASE_BITARRAY_H
#define ENGINE_SOURCE_BASE_BITARRAY_H

#include <cstdint>

class CBitArray {
    public:
    /* 0000 */ char* m_array;
    /* 0004 */ uint32_t m_size;
    /* 0008 */ bool m_ownsArray;

    CBitArray();
    ~CBitArray();
    int32_t Get(uint32_t bitIdx) const;
    void Set(uint32_t bitIdx, bool value);
    void SetRawData(char* data, uint32_t size, bool owned);
};

#endif
