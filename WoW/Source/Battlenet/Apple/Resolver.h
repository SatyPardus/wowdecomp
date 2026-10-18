#pragma once
#include "WoW/Source/Battlenet/Battlenet.h"

namespace Battlenet {
// Original synchronous IPv4 contract: one address, or an empty failure result.
bool ResolveNativeIPv4(const char* hostname, Client::DNSInfo& info);
}
