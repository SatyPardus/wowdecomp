#ifndef WOW_SOURCE_CONSOLEDEVICE_CONSOLEDETECT_H
#define WOW_SOURCE_CONSOLEDEVICE_CONSOLEDETECT_H

#include <cstdint>
#include "WoW/Source/Types.h"

// Select the original DBC record/default tables without displaying UI or saving preferences.
bool ConsoleDetectSelectHardware(Hardware& hardware, uint32_t cpuIdx);
void ConsoleDetectDetectHardware(Hardware& hardware, bool& changed);
void ConsoleDetectSaveHardware(const Hardware& hardware, bool& changed);
void ConsoleDetectSetDefaults(DefaultSettings& defaults, const Hardware& hardware);
void ConsoleDetectSetDefaultsFormat(DefaultSettings& defaults, const Hardware& hardware);

#endif
