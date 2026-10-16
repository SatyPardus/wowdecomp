#include <cstdint>
#include "Engine/Source/Base/BitArray.h"

// VERIFY: 08b8f5fb auto
// OFFSET: 0x0095DA10
CBitArray::CBitArray() {
    this->m_array = nullptr;
    this->m_size = 0;
    this->m_ownsArray = false;
}

// VERIFY: 24197791 auto
// OFFSET: 0x0095DA20
int32_t CBitArray::Get(uint32_t bitIdx) const {
    return (this->m_array[bitIdx >> 3] & (1 << (bitIdx & 7))) != 0;
}

// VERIFY: 8b0b5341 manual
// OFFSET: 0x0095DA50
void CBitArray::Set(uint32_t bitIdx, bool value) {
    char* byte = &this->m_array[bitIdx >> 3];
    char mask = 1 << (bitIdx & 7);

    if (value) {
        *byte |= mask;
    } else {
        *byte &= ~mask;
    }
}

// VERIFY: 93b36a31 manual
// OFFSET: 0x0095DA80
CBitArray::~CBitArray() {
    if (this->m_array && this->m_ownsArray) {
        delete[] this->m_array;
    }
}

// VERIFY: 0d9be8fd manual
// OFFSET: 0x0095DAA0
void CBitArray::SetRawData(char* data, uint32_t size, bool owned) {
    if (this->m_array && this->m_ownsArray) {
        delete[] this->m_array;
    }

    this->m_array = data;
    this->m_size = size;
    this->m_ownsArray = owned;
}
