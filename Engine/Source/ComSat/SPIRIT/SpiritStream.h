#ifndef ENGINE_SOURCE_COMSAT_SPIRIT_SPIRITSTREAM_H
#define ENGINE_SOURCE_COMSAT_SPIRIT_SPIRITSTREAM_H

#include "Engine/Source/ComSat/ComSatSpirit.h"

namespace Spirit {
// Internal parser failure, caught at the public codec entry points. Abort at
// the first invalid read rather than synthesizing from a fabricated value.
struct InvalidStream {};
inline void RequireBits(const SPIRIT_IPMR_STREAM* stream, int32_t count) {
    if (!stream || count < 0 || !stream->m_data ||
        uint64_t(stream->m_bits) + uint32_t(count) > uint64_t(stream->m_size) * 8)
        throw InvalidStream{};
}
}
#endif
