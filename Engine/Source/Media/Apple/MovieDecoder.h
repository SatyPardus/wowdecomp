#pragma once
#include <cstdint>
namespace AppleMovie {
// Allocate a free native slot atomically; destruction need not be LIFO.
int32_t CreateDecoder(int32_t width, int32_t height);
}
