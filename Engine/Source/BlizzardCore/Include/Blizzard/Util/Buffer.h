#ifndef ENGINE_SOURCE_BLIZZARDCORE_INCLUDE_BLIZZARD_UTIL_BUFFER_H
#define ENGINE_SOURCE_BLIZZARDCORE_INCLUDE_BLIZZARD_UTIL_BUFFER_H

#include <cstddef>
#include <cstdint>

namespace Blizzard {
namespace Util {
template <class T, int N>
struct BufferData {
    /* 0000 */ uint32_t m_elements;
    /* 0004 */ T* m_data;
    /* 0008 */ T m_local [N];
};
}
}

namespace Blizzard {
namespace Util {
template <int N, class T>
struct Buffer {
    /* 0000 */ Blizzard::Util::BufferData<T, N> unused_0000;
};
}
}

#endif
