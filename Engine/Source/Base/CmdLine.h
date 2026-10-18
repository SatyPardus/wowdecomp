#ifndef ENGINE_SOURCE_BASE_CMDLINE_H
#define ENGINE_SOURCE_BASE_CMDLINE_H

#include <cstdint>
#include "Engine/Source/Base/Types.h"

int32_t CmdLineGetBool(CMDOPT opt);
const char* CmdLineGetString(CMDOPT opt);
int32_t CmdLineProcess();
#if defined(__APPLE__)
int32_t CmdLineProcess(int argc, const char* const* argv);
#endif

#endif
