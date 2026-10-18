#include "Engine/Source/ComSat/ComSatBitPacker.h"
#include <cstring>

// VERIFY: 09242d98 auto
// OFFSET: 0x00885730
ComSat::BitPacker::BitPacker(uint8_t* buffer, uint32_t bitLength) {
    this->m_buffer = buffer;
    this->m_bitLength = bitLength;
    this->m_byteOffset = 0;
    this->m_bitOffset = 0;
}

// VERIFY: 4151e984 auto
// OFFSET: 0x00885760
ComSat::BitPacker::~BitPacker() {
}

// VERIFY: 2517a981 auto
// OFFSET: 0x008858E0
uint32_t ComSat::BitPacker::Finalize() {
    if (this->m_bitOffset) {
        this->m_buffer[this->m_byteOffset] &= (1 << this->m_bitOffset) - 1;
        this->m_byteOffset++;
        this->m_bitOffset = 0;
    }
    return this->m_byteOffset;
}

// VERIFY: c6ad9043 manual
// OFFSET: 0x00885770
bool ComSat::BitPacker::PackBits(const uint8_t* data, uint32_t bits) {
    if (uint64_t(this->m_bitLength) < uint64_t(bits) + this->m_bitOffset + 8ull * this->m_byteOffset) {
        return false;
    }
    if (!bits) return true;
    if (!data || !this->m_buffer) return false;
    int32_t bytes = bits >> 3;
    uint32_t remainder = bits & 7;
    int32_t i;
    if (this->m_bitOffset) {
        for (i = 0; i < bytes; i++) {
            this->m_buffer[this->m_byteOffset] &= (1 << this->m_bitOffset) - 1;
            this->m_buffer[this->m_byteOffset] |= data[i] << this->m_bitOffset;
            this->m_buffer[this->m_byteOffset + 1] &= ~uint32_t(0) << this->m_bitOffset;
            this->m_buffer[this->m_byteOffset + 1] |= data[i] >> (8 - this->m_bitOffset);
            this->m_byteOffset++;
        }
    } else {
        memcpy(&this->m_buffer[this->m_byteOffset], data, bytes);
        this->m_byteOffset += bytes;
        i = bytes;
    }
    if (remainder) {
        uint8_t value = data[i] & ((1 << remainder) - 1);
        this->m_buffer[this->m_byteOffset] &= (0xFFu >> (8 - this->m_bitOffset)) | (~uint32_t(0) << (this->m_bitOffset + remainder));
        this->m_buffer[this->m_byteOffset] |= value << this->m_bitOffset;
        if (this->m_bitOffset + remainder >= 8) {
            // The original binary (0x885894) also touches the next byte here.
            // An exact byte boundary has no spill byte (possibly no storage
            // beyond this byte at all).
            if (this->m_bitOffset + remainder > 8) {
                this->m_buffer[this->m_byteOffset + 1] &= ~uint32_t(0) << this->m_bitOffset;
                this->m_buffer[this->m_byteOffset + 1] |= value >> (8 - this->m_bitOffset);
            }
            this->m_byteOffset++;
            this->m_bitOffset += remainder - 8;
            return true;
        }
        this->m_bitOffset += remainder;
    }
    return true;
}

// VERIFY: 670e65cb auto
// OFFSET: 0x00885730
ComSat::BitUnpacker::BitUnpacker(const uint8_t* buffer, uint32_t bitLength) {
    this->m_buffer = buffer;
    this->m_bitLength = bitLength;
    this->m_byteOffset = 0;
    this->m_bitOffset = 0;
}

// VERIFY: aaf78467 auto
// OFFSET: [0x00885760, 0x00885AA0]
ComSat::BitUnpacker::~BitUnpacker() {
}

// VERIFY: 6ca95947 manual
// OFFSET: 0x00885910
bool ComSat::BitUnpacker::UnpackBits(uint8_t* data, uint32_t bits) {
    if (uint64_t(this->m_bitLength) < uint64_t(this->m_bitOffset) + bits + 8ull * this->m_byteOffset) {
        return false;
    }
    if (!bits) return true;
    if (!data || !this->m_buffer) return false;
    int32_t bytes = bits >> 3;
    int32_t remainder = bits & 7;
    memset(data, 0, remainder ? bytes + 1 : bytes);
    int32_t i;
    if (this->m_bitOffset) {
        for (i = 0; i < bytes; i++) {
            data[i] = this->m_buffer[this->m_byteOffset] >> this->m_bitOffset;
            data[i] |= (((1 << this->m_bitOffset) - 1) & this->m_buffer[this->m_byteOffset++ + 1]) << (8 - this->m_bitOffset);
        }
    } else {
        memcpy(data, &this->m_buffer[this->m_byteOffset], bytes);
        this->m_byteOffset += bytes;
        i = bytes;
    }
    if (remainder) {
        if (!this->m_bitOffset) {
            data[i] = ((1 << remainder) - 1) & this->m_buffer[this->m_byteOffset];
            this->m_bitOffset = remainder;
            return true;
        }
        if (remainder > static_cast<int32_t>(8 - this->m_bitOffset)) {
            data[i] = this->m_buffer[this->m_byteOffset] >> this->m_bitOffset;
            this->m_byteOffset++;
            int32_t bitOffset = remainder + this->m_bitOffset - 8;
            data[i] |= (((1 << bitOffset) - 1) & this->m_buffer[this->m_byteOffset]) << (8 - this->m_bitOffset);
            this->m_bitOffset = bitOffset;
            return true;
        }
        data[i] = ((1 << remainder) - 1) & (this->m_buffer[this->m_byteOffset] >> this->m_bitOffset);
        this->m_bitOffset = (remainder + this->m_bitOffset) & 7;
        // The original binary (0x885a85) likewise omits this advancement.
        // Continue at the next byte after consuming the current byte exactly.
        if (!this->m_bitOffset) this->m_byteOffset++;
    }
    return true;
}
