#if defined(__APPLE__)
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/ConsoleBase/Apple/Output.h"
#include "Engine/Source/Storm/H/storm.h"
#include <cstdarg>
#include <cstdio>
#include <deque>
#include <mutex>
#include <string>

namespace {
std::mutex outputMutex;
bool captureOutput = false;
struct Line { std::string text; COLOR_T color; };
std::deque<Line> pendingOutput;
}
void ConsoleCaptureOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(outputMutex);
    captureOutput = enabled; pendingOutput.clear();
}
void ConsoleDiscardOutput() {
    std::lock_guard<std::mutex> lock(outputMutex);
    pendingOutput.clear();
}
void ConsoleDrainOutput(void (*write)(const char*, COLOR_T)) {
    std::deque<Line> lines;
    { std::lock_guard<std::mutex> lock(outputMutex); lines.swap(pendingOutput); }
    for (const auto& line : lines) write(line.text.c_str(), line.color);
}
void ConsoleWrite(const char* text, COLOR_T color) {
    if (!text || !*text) return;
    std::fprintf(stderr, "[WoW console] %s\n", text);
    std::lock_guard<std::mutex> lock(outputMutex);
    if (captureOutput) {
        // Same retained-line limit as the original interactive console.
        if (pendingOutput.size() == 256) pendingOutput.pop_front();
        pendingOutput.push_back({text, color});
    }
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
