#ifndef ENGINE_SOURCE_CONSOLEBASE_APPLE_OUTPUT_H
#define ENGINE_SOURCE_CONSOLEBASE_APPLE_OUTPUT_H
#include "Engine/Source/ConsoleBase/Types.h"

// The UI owns this capture lifetime. Drain only on the selected game/Event
// thread; worker diagnostics must not touch the shared font or screen caches.
void ConsoleCaptureOutput(bool enabled);
void ConsoleDrainOutput(void (*write)(const char*, COLOR_T));
void ConsoleDiscardOutput();
#endif
