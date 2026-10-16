#ifndef WOW_SOURCE_CONSOLEDEVICE_CONSOLEDEVICE_H
#define WOW_SOURCE_CONSOLEDEVICE_CONSOLEDEVICE_H

#include <cstdint>
#include "WoW/Source/Types.h"

class CGxFormat;
struct CVar;

enum OPTION_PAGE {
    OPTION_PAGE_0 = 0,
    OPTION_PAGE_1 = 1,
    OPTION_PAGE_2 = 2,
    NUM_OPTION_PAGES = 3,
};

extern DefaultSettings s_defaults;
extern CGxFormat s_desktopFormat;
extern CGxFormat s_fallbackFormat;
extern Hardware s_hardware;
extern CGxFormat s_lastGoodFormat;
extern CGxFormat s_requestedFormat;
extern CVar* s_cvFixedFunction;
extern CVar* s_cvGxFixLag;
extern CVar* s_cvGxMaximize;
extern CVar* s_cvGxWindow;
extern CVar* s_cvHwDetect;
extern CVar* s_cvVideoOptionsVersion;
extern char s_windowTitle[256];

void AddConsoleDeviceDefaultCallback(void (*callback)(OPTION_PAGE page));
int32_t CCGxRestart(const char* command, const char* arguments);
int32_t ConsoleDeviceExists();
void ConsoleDeviceDefaults(OPTION_PAGE page);
void ConsoleDeviceDestroy();
DefaultSettings* ConsoleDeviceGetDefaultSettings();
int32_t ConsoleDeviceHardwareChanged();
void ConsoleDeviceInitialize(const char* title, bool unused_a2, bool unused_a3);
Hardware* ConsoleGetHardware();
void ConsoleGxOverride(const char* list);
int32_t IsConsoleEnabled();
void RegisterGxCVars();
void RegisterStereoCVars();
void RemoveConsoleDeviceDefaultCallback(void (*callback)(OPTION_PAGE page));
bool SetGxCVars(const CGxFormat& format);
void __fastcall StereoChangedCallback();
bool UpdateGxCVars();
void ValidateFormatMonitor(CGxFormat& format);

#endif
