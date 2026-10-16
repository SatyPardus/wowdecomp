#ifndef WOW_SOURCE_NET_NETINTERNAL_H
#define WOW_SOURCE_NET_NETINTERNAL_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"

class NetClient;

class NetEventQueue {
    public:
    /* 0000 */ NetClient* m_client;
    /* 0004 */ uint32_t m_critSect;
    /* 0008 */ uint32_t unk_0008;
    /* 000C */ uint32_t unk_000C;
    /* 0010 */ uint32_t unk_0010;
    /* 0014 */ uint32_t unk_0014;
    /* 0018 */ uint32_t unk_0018;
    /* 001C */ uint32_t unk_001C;
    /* 0020 */ uint32_t unk_0020;
    /* 0024 */ uint32_t unk_0024;
    /* 0028 */ uint32_t unk_0028;
    /* 002C */ uint32_t unk_002C;
    /* 0030 */ uint32_t unk_0030;
    /* 0034 */ TSList<void> unk_0034;
};

#endif
