#if defined(__APPLE__)
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Storm/H/storm.h"
#include <cstdarg>
#include <cstdio>

// Native startup diagnostics until the original interactive console UI is
// connected. Command dispatch and CVar behavior remain in shared ConsoleBase.
void ConsoleWrite(const char* text, COLOR_T) {
    if (text && *text) { std::fprintf(stderr, "[WoW console] %s\n", text); }
}
void ConsoleWriteA(const char* format, COLOR_T color, ...) {
    if (!format) { return; }
    char buffer[1024]; va_list args; va_start(args, color);
    SStrVPrintf(buffer, sizeof(buffer), format, args); va_end(args);
    ConsoleWrite(buffer, color);
}
void ConsolePrintf(const char* format, ...) {
    if (!format) { return; }
    char buffer[4096]; va_list args; va_start(args, format);
    SStrVPrintf(buffer, sizeof(buffer), format, args); va_end(args);
    ConsoleWrite(buffer, DEFAULT_COLOR);
}
#endif
