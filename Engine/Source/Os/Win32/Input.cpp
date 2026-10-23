#if defined(WOWDECOMP_ARCH_32)

#include <cstdint>
#include <windows.h>
#include <imm.h>
#include <malloc.h>
#include "Engine/Source/Event/Types.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/Joystick.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2iVector.h"

#define WM_POPUPSYSTEMMENU 0x313

// STATIC: 0x00B1C228
static uint32_t s_latin1lookup[32] = {
    0xFFFE, 0xFFFE, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFE, 0xFFFE, 0xFFFE,
    0xFFFE, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFE, 0xFFFE, 0x0178,
};

// STATIC: 0x00B1C2A8
static uint32_t s_latin2lookup[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
    0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
    0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,
    0x20AC, 0xFFFE, 0x201A, 0xFFFE, 0x201E, 0x2026, 0x2020, 0x2021,
    0xFFFE, 0x2030, 0x0160, 0x2039, 0x015A, 0x0164, 0x017D, 0x0179,
    0xFFFE, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0xFFFE, 0x2122, 0x0161, 0x203A, 0x015B, 0x0165, 0x017E, 0x017A,
    0x00A0, 0x02C7, 0x02D8, 0x0141, 0x00A4, 0x0104, 0x00A6, 0x00A7,
    0x00A8, 0x00A9, 0x015E, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x017B,
    0x00B0, 0x00B1, 0x02DB, 0x0142, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
    0x00B8, 0x0105, 0x015F, 0x00BB, 0x013D, 0x02DD, 0x013E, 0x017C,
    0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
    0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
    0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,
    0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
    0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
    0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
    0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,
    0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9,
};

// STATIC: 0x00B1C6A8
static uint32_t s_cyrilliclookup[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
    0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
    0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,
    0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
    0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
    0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x0020, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
    0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
    0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
    0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
    0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
    0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
    0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
};

// STATIC: 0x00B1CAA8
static uint32_t s_latin5lookup[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
    0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
    0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,
    0x20AC, 0x0020, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x0020, 0x0020, 0x0020,
    0x0020, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x0020, 0x0020, 0x0178,
    0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
    0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
    0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
    0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
    0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
    0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
    0x011E, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
    0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x0130, 0x015E, 0x00DF,
    0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
    0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
    0x011F, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
    0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x0131, 0x015F, 0x00FF,
};

// STATIC: 0x00B1CEA8
static uint32_t s_thailookup[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
    0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
    0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,
    0x20AC, 0x0020, 0x0020, 0x0020, 0x0020, 0x2026, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x00A0, 0x0E01, 0x0E02, 0x0E03, 0x0E04, 0x0E05, 0x0E06, 0x0E07,
    0x0E08, 0x0E09, 0x0E0A, 0x0E0B, 0x0E0C, 0x0E0D, 0x0E0E, 0x0E0F,
    0x0E10, 0x0E11, 0x0E12, 0x0E13, 0x0E14, 0x0E15, 0x0E16, 0x0E17,
    0x0E18, 0x0E19, 0x0E1A, 0x0E1B, 0x0E1C, 0x0E1D, 0x0E1E, 0x0E1F,
    0x0E20, 0x0E21, 0x0E22, 0x0E23, 0x0E24, 0x0E25, 0x0E26, 0x0E27,
    0x0E28, 0x0E29, 0x0E2A, 0x0E2B, 0x0E2C, 0x0E2D, 0x0E2E, 0x0E2F,
    0x0E30, 0x0E31, 0x0E32, 0x0E33, 0x0E34, 0x0E35, 0x0E36, 0x0E37,
    0x0E38, 0x0E39, 0x0E3A, 0x0020, 0x0020, 0x0020, 0x0020, 0x0E3F,
    0x0E40, 0x0E41, 0x0E42, 0x0E43, 0x0E44, 0x0E45, 0x0E46, 0x0E47,
    0x0E48, 0x0E49, 0x0E4A, 0x0E4B, 0x0E4C, 0x0E4D, 0x0E4E, 0x0E4F,
    0x0E50, 0x0E51, 0x0E52, 0x0E53, 0x0E54, 0x0E55, 0x0E56, 0x0E57,
    0x0E58, 0x0E59, 0x0E5A, 0x0E5B, 0x0020, 0x0020, 0x0020, 0x0020,
};
// STATIC: 0x00B1C220
static int32_t s_appActive = 1;

// STATIC: 0x00B1D2A8
static uint32_t s_rshiftScanCode = 0x100;

// STATIC: 0x00D413D0
static int16_t s_numLockState;

// STATIC: 0x00D413D4
static RECT s_dragRect;

// STATIC: 0x00D413E4
static C2iVector s_dragStart;

// STATIC: 0x00D413EC
static C2iVector s_centerMousePos;

// STATIC: 0x00D413F4
static HWND s_mouseWnd;

// STATIC: 0x00D413F8
static C2iVector s_mousePos;

// STATIC: 0x00D41400
static int32_t s_queueHead;

// STATIC: 0x00D41404
static int32_t s_queueTail;

// STATIC: 0x00D41408
static OSEVENT s_queue[16];

// STATIC: 0x00D41548
static int32_t s_mouseSpeed;

// STATIC: 0x00D4154C
static int32_t s_savedMouseSpeed;

// STATIC: 0x00D41550
static int32_t s_releasingCapture;

// STATIC: 0x00D41554
static uint32_t s_osButtonState;

// STATIC: 0x00D41558
static RECT s_defaultWindowRect;

// STATIC: 0x00D41568
static intptr_t s_windowSize;

// STATIC: 0x00D4156C
static OS_MOUSE_MODE s_osMouseMode;

// STATIC: 0x00D41570
static int32_t s_dragHitTest;

// STATIC: 0x00D41574
static int32_t s_dragging;

// STATIC: 0x00D41578
static int32_t s_dragCancel;

// STATIC: 0x00D4157C
static uint32_t s_codePage;

// STATIC: 0x00D41580
static int32_t s_WindowResizeLock;

// STATIC: 0x00D41584
static intptr_t (*s_chainWindowProc)(void* window, uint32_t message, uintptr_t wparam, intptr_t lparam);

// STATIC: 0x00D41588
static uint32_t s_extraButtonState;

// STATIC: 0x00D4158C
static void (*s_eventPollProc)();

// STATIC: 0x00D41590
static BOOL (WINAPI* s_getPhysicalCursorPos)(POINT* point);

// STATIC: 0x00D41594
static BOOL (WINAPI* s_setPhysicalCursorPos)(int32_t x, int32_t y);

// STATIC: 0x00D41598
static void (WINAPI* s_logicalToPhysicalPoint)(HWND hwnd, POINT* point);

// VERIFY: e59d4ab5 auto
// OFFSET: 0x00868C10
static BOOL OsGetPhysicalCursorPos(POINT* point) {
    if (!s_getPhysicalCursorPos) {
        HMODULE user32 = LoadLibraryA("user32.dll");
        if (user32) {
            s_getPhysicalCursorPos = reinterpret_cast<decltype(s_getPhysicalCursorPos)>(GetProcAddress(user32, "GetPhysicalCursorPos"));
            if (!s_getPhysicalCursorPos) {
                s_getPhysicalCursorPos = reinterpret_cast<decltype(s_getPhysicalCursorPos)>(GetProcAddress(user32, "GetCursorPos"));
            }

            FreeLibrary(user32);
        }

        if (!s_getPhysicalCursorPos) {
            return FALSE;
        }
    }

    return s_getPhysicalCursorPos(point);
}

// VERIFY: 4c15d9db auto
// OFFSET: 0x00868C70
static BOOL OsSetPhysicalCursorPos(int32_t x, int32_t y) {
    if (!s_setPhysicalCursorPos) {
        HMODULE user32 = LoadLibraryA("user32.dll");
        if (user32) {
            s_setPhysicalCursorPos = reinterpret_cast<decltype(s_setPhysicalCursorPos)>(GetProcAddress(user32, "SetPhysicalCursorPos"));
            if (!s_setPhysicalCursorPos) {
                s_setPhysicalCursorPos = reinterpret_cast<decltype(s_setPhysicalCursorPos)>(GetProcAddress(user32, "SetCursorPos"));
            }

            FreeLibrary(user32);
        }

        if (!s_setPhysicalCursorPos) {
            return FALSE;
        }
    }

    return s_setPhysicalCursorPos(x, y);
}

static void WINAPI LogicalToPhysicalPointNop(HWND hwnd, POINT* point) {
}

// VERIFY: 521ae11a auto
// OFFSET: 0x00868CE0
static void OsLogicalToPhysicalPoint(HWND hwnd, POINT* point) {
    if (!s_logicalToPhysicalPoint) {
        HMODULE user32 = LoadLibraryA("user32.dll");
        if (user32) {
            s_logicalToPhysicalPoint = reinterpret_cast<decltype(s_logicalToPhysicalPoint)>(GetProcAddress(user32, "LogicalToPhysicalPoint"));
            FreeLibrary(user32);
        }

        if (!s_logicalToPhysicalPoint) {
            s_logicalToPhysicalPoint = LogicalToPhysicalPointNop;
        }
    }

    s_logicalToPhysicalPoint(hwnd, point);
}

// VERIFY: a463d7ac manual
// OFFSET: 0x00868D40
void OsQueuePut(OSINPUT id, int32_t param0, int32_t param1, int32_t param2, int32_t param3) {
    int32_t next = s_queueTail == 15 ? 0 : s_queueTail + 1;

    if (next == s_queueHead) {
        s_queueHead = s_queueHead == 15 ? 0 : s_queueHead + 1;
    }

    OSEVENT& event = s_queue[s_queueTail];
    event.id = id;
    event.param[0] = param0;
    event.param[1] = param1;
    event.param[2] = param2;
    event.param[3] = OsGetAsyncTimeMs();

    s_queueTail = next;
}

// VERIFY: 75299399 manual
// OFFSET: 0x00868DB0
int32_t OsQueueGet(OSINPUT* id, int32_t* param0, int32_t* param1, int32_t* param2, int32_t* param3) {
    if (s_queueHead == s_queueTail) {
        return 0;
    }

    OSEVENT& event = s_queue[s_queueHead];
    *id = event.id;
    *param0 = event.param[0];
    *param1 = event.param[1];
    *param2 = event.param[2];
    *param3 = event.param[3];

    s_queueHead = s_queueHead == 15 ? 0 : s_queueHead + 1;

    return 1;
}

// VERIFY: 5830d143 manual
// OFFSET: 0x00868E30
static int32_t ConvertKeyCode(uint32_t vkey, KEY* key) {
    if (vkey >= VK_F1 && vkey <= VK_F12) {
        *key = static_cast<KEY>(vkey + (KEY_F1 - VK_F1));
        return 1;
    }

    if (vkey >= '0' && vkey <= '9') {
        *key = static_cast<KEY>(vkey);
        return 1;
    }

    switch (vkey) {
        case VK_LSHIFT:
            *key = KEY_LSHIFT;
            return 1;

        case VK_RSHIFT:
            *key = KEY_RSHIFT;
            return 1;

        case VK_LCONTROL:
            *key = KEY_LCONTROL;
            return 1;

        case VK_RCONTROL:
            *key = KEY_RCONTROL;
            return 1;

        case VK_LMENU:
            *key = KEY_LALT;
            return 1;

        case VK_RMENU:
            *key = KEY_RALT;
            return 1;

        case VK_ESCAPE:
            *key = KEY_ESCAPE;
            return 1;

        case VK_RETURN:
            *key = KEY_ENTER;
            return 1;

        case VK_BACK:
            *key = KEY_BACKSPACE;
            return 1;

        case VK_TAB:
            *key = KEY_TAB;
            return 1;

        case VK_LEFT:
            *key = KEY_LEFT;
            return 1;

        case VK_UP:
            *key = KEY_UP;
            return 1;

        case VK_RIGHT:
            *key = KEY_RIGHT;
            return 1;

        case VK_DOWN:
            *key = KEY_DOWN;
            return 1;

        case VK_INSERT:
            *key = KEY_INSERT;
            return 1;

        case VK_DELETE:
            *key = KEY_DELETE;
            return 1;

        case VK_HOME:
            *key = KEY_HOME;
            return 1;

        case VK_END:
            *key = KEY_END;
            return 1;

        case VK_PRIOR:
            *key = KEY_PAGEUP;
            return 1;

        case VK_NEXT:
            *key = KEY_PAGEDOWN;
            return 1;

        case VK_CAPITAL:
            *key = KEY_CAPSLOCK;
            return 1;

        case VK_NUMLOCK:
            *key = KEY_NUMLOCK;
            return 1;

        case VK_SCROLL:
            *key = KEY_SCROLLLOCK;
            return 1;

        case VK_PAUSE:
            *key = KEY_PAUSE;
            return 1;

        case VK_SNAPSHOT:
            *key = KEY_PRINTSCREEN;
            return 1;

        case VK_SPACE:
            *key = KEY_SPACE;
            return 1;

        case VK_NUMPAD0:
            *key = KEY_NUMPAD0;
            return 1;

        case VK_NUMPAD1:
            *key = KEY_NUMPAD1;
            return 1;

        case VK_NUMPAD2:
            *key = KEY_NUMPAD2;
            return 1;

        case VK_NUMPAD3:
            *key = KEY_NUMPAD3;
            return 1;

        case VK_NUMPAD4:
            *key = KEY_NUMPAD4;
            return 1;

        case VK_NUMPAD5:
            *key = KEY_NUMPAD5;
            return 1;

        case VK_NUMPAD6:
            *key = KEY_NUMPAD6;
            return 1;

        case VK_NUMPAD7:
            *key = KEY_NUMPAD7;
            return 1;

        case VK_NUMPAD8:
            *key = KEY_NUMPAD8;
            return 1;

        case VK_NUMPAD9:
            *key = KEY_NUMPAD9;
            return 1;

        case VK_ADD:
            *key = KEY_NUMPAD_PLUS;
            return 1;

        case VK_SUBTRACT:
            *key = KEY_NUMPAD_MINUS;
            return 1;

        case VK_MULTIPLY:
            *key = KEY_NUMPAD_MULTIPLY;
            return 1;

        case VK_DIVIDE:
            *key = KEY_NUMPAD_DIVIDE;
            return 1;

        case VK_DECIMAL:
            *key = KEY_NUMPAD_DECIMAL;
            return 1;

        default: {
            uint32_t ch = MapVirtualKeyA(vkey, MAPVK_VK_TO_CHAR) & 0xFFFF;
            *key = static_cast<KEY>(ch);
            if (ch == 0 || ch > 0xFF) {
                return 0;
            }

            return 1;
        }
    }
}

// VERIFY: 2daef69c manual
// OFFSET: 0x008691C0
static int32_t ConvertKeyToVirtualKey(KEY key, uint32_t* vkey) {
    if (key >= KEY_F1 && key <= KEY_F12) {
        *vkey = key - (KEY_F1 - VK_F1);
        return 1;
    }

    if (key >= KEY_0 && key <= KEY_9) {
        *vkey = key;
        return 1;
    }

    switch (key) {
        case KEY_LSHIFT:
            *vkey = VK_LSHIFT;
            return 1;

        case KEY_RSHIFT:
            *vkey = VK_RSHIFT;
            return 1;

        case KEY_LCONTROL:
            *vkey = VK_LCONTROL;
            return 1;

        case KEY_RCONTROL:
            *vkey = VK_RCONTROL;
            return 1;

        case KEY_LALT:
            *vkey = VK_LMENU;
            return 1;

        case KEY_RALT:
            *vkey = VK_RMENU;
            return 1;

        case KEY_SPACE:
            *vkey = VK_SPACE;
            return 1;

        case KEY_NUMPAD0:
            *vkey = VK_NUMPAD0;
            return 1;

        case KEY_NUMPAD1:
            *vkey = VK_NUMPAD1;
            return 1;

        case KEY_NUMPAD2:
            *vkey = VK_NUMPAD2;
            return 1;

        case KEY_NUMPAD3:
            *vkey = VK_NUMPAD3;
            return 1;

        case KEY_NUMPAD4:
            *vkey = VK_NUMPAD4;
            return 1;

        case KEY_NUMPAD5:
            *vkey = VK_NUMPAD5;
            return 1;

        case KEY_NUMPAD6:
            *vkey = VK_NUMPAD6;
            return 1;

        case KEY_NUMPAD7:
            *vkey = VK_NUMPAD7;
            return 1;

        case KEY_NUMPAD8:
            *vkey = VK_NUMPAD8;
            return 1;

        case KEY_NUMPAD9:
            *vkey = VK_NUMPAD9;
            return 1;

        case KEY_NUMPAD_PLUS:
            *vkey = VK_ADD;
            return 1;

        case KEY_NUMPAD_MINUS:
            *vkey = VK_SUBTRACT;
            return 1;

        case KEY_NUMPAD_MULTIPLY:
            *vkey = VK_MULTIPLY;
            return 1;

        case KEY_NUMPAD_DIVIDE:
            *vkey = VK_DIVIDE;
            return 1;

        case KEY_NUMPAD_DECIMAL:
            *vkey = VK_DECIMAL;
            return 1;

        case KEY_ESCAPE:
            *vkey = VK_ESCAPE;
            return 1;

        case KEY_ENTER:
            *vkey = VK_RETURN;
            return 1;

        case KEY_BACKSPACE:
            *vkey = VK_BACK;
            return 1;

        case KEY_TAB:
            *vkey = VK_TAB;
            return 1;

        case KEY_LEFT:
            *vkey = VK_LEFT;
            return 1;

        case KEY_UP:
            *vkey = VK_UP;
            return 1;

        case KEY_RIGHT:
            *vkey = VK_RIGHT;
            return 1;

        case KEY_DOWN:
            *vkey = VK_DOWN;
            return 1;

        case KEY_INSERT:
            *vkey = VK_INSERT;
            return 1;

        case KEY_DELETE:
            *vkey = VK_DELETE;
            return 1;

        case KEY_HOME:
            *vkey = VK_HOME;
            return 1;

        case KEY_END:
            *vkey = VK_END;
            return 1;

        case KEY_PAGEUP:
            *vkey = VK_PRIOR;
            return 1;

        case KEY_PAGEDOWN:
            *vkey = VK_NEXT;
            return 1;

        case KEY_CAPSLOCK:
            *vkey = VK_CAPITAL;
            return 1;

        case KEY_NUMLOCK:
            *vkey = VK_NUMLOCK;
            return 1;

        case KEY_SCROLLLOCK:
            *vkey = VK_SCROLL;
            return 1;

        case KEY_PAUSE:
            *vkey = VK_PAUSE;
            return 1;

        case KEY_PRINTSCREEN:
            *vkey = VK_SNAPSHOT;
            return 1;

        default: {
            uint32_t vk = VkKeyScanA(static_cast<CHAR>(key)) & 0xFF;
            *vkey = vk;
            if (vk == 0 || vk > 0xFF) {
                return 0;
            }

            return 1;
        }
    }
}

// VERIFY: 4f2eb8e2 manual
// OFFSET: 0x008694E0
static int32_t ConvertButton(uint32_t message, uintptr_t wparam, MOUSEBUTTON* button) {
    switch (message) {
        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONUP:
        case WM_NCLBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            *button = MOUSE_BUTTON_LEFT;
            return 1;

        case WM_NCRBUTTONDOWN:
        case WM_NCRBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            *button = MOUSE_BUTTON_RIGHT;
            return 1;

        case WM_NCMBUTTONDOWN:
        case WM_NCMBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            *button = MOUSE_BUTTON_MIDDLE;
            return 1;

        case WM_NCXBUTTONDOWN:
        case WM_NCXBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            switch (GET_XBUTTON_WPARAM(wparam)) {
                case XBUTTON1:
                    *button = MOUSE_BUTTON_XBUTTON1;
                    return 1;

                case XBUTTON2:
                    *button = MOUSE_BUTTON_XBUTTON2;
                    return 1;

                default:
                    break;
            }
            break;

        default:
            break;
    }

    *button = MOUSE_BUTTON_NONE;
    return 0;
}

// VERIFY: 535070f3 auto
// OFFSET: 0x008695B0
static void RestoreMouse() {
    POINT point;
    point.x = s_mousePos.x;
    point.y = s_mousePos.y;

    ClientToScreen(s_mouseWnd, &point);
    OsLogicalToPhysicalPoint(s_mouseWnd, &point);
    OsSetPhysicalCursorPos(point.x, point.y);
}

// VERIFY: 6e7171b8 auto
// OFFSET: 0x00869600
static void SaveMouse(HWND hwnd, const POINT& point) {
    if (s_osMouseMode == OS_MOUSE_MODE_RELATIVE) {
        return;
    }

    s_mouseWnd = hwnd;
    s_mousePos.x = point.x;
    s_mousePos.y = point.y;
}

// VERIFY: c77eb699 auto
// OFFSET: 0x00869620
void OsInputSetWindowResizeLock(int32_t resizeLock) {
    s_WindowResizeLock = resizeLock;
}

// VERIFY: 7a315de8 auto
// OFFSET: 0x00869630
float OsInputGetMouseSpeed() {
    return s_mouseSpeed * 0.1f;
}

// VERIFY: 536c65bf manual
// OFFSET: 0x00869640
int32_t OsInputIsKeyDown(KEY key) {
    uint32_t vkey;
    if (!ConvertKeyToVirtualKey(key, &vkey)) {
        return 0;
    }

    return (GetAsyncKeyState(vkey) >> 15) & 1;
}

// VERIFY: 5eb89080 manual
// OFFSET: 0x00869670
int32_t OsInputIsMouseButtonDown(MOUSEBUTTON button) {
    switch (button) {
        case MOUSE_BUTTON_LEFT:
            return (GetAsyncKeyState(VK_LBUTTON) >> 15) & 1;

        case MOUSE_BUTTON_MIDDLE:
            return (GetAsyncKeyState(VK_MBUTTON) >> 15) & 1;

        case MOUSE_BUTTON_RIGHT:
            return (GetAsyncKeyState(VK_RBUTTON) >> 15) & 1;

        case MOUSE_BUTTON_XBUTTON1:
            return (GetAsyncKeyState(VK_XBUTTON1) >> 15) & 1;

        case MOUSE_BUTTON_XBUTTON2:
            return (GetAsyncKeyState(VK_XBUTTON2) >> 15) & 1;

        default:
            return 0;
    }
}

// VERIFY: 7da0b025 auto
// OFFSET: 0x00869720
void OsInputInitialize() {
    s_numLockState = GetAsyncKeyState(VK_NUMLOCK);
    s_savedMouseSpeed = 10;
    SystemParametersInfoA(SPI_GETMOUSESPEED, 0, &s_savedMouseSpeed, 0);
    s_mouseSpeed = s_savedMouseSpeed;
}

// VERIFY: 93192a66 auto
// OFFSET: 0x00869760
void OsInputDestroy() {
    if (GetAsyncKeyState(VK_NUMLOCK) != s_numLockState) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_NUMLOCK;
        SendInput(1, &input, sizeof(input));
    }

    if (s_appActive && s_mouseSpeed != s_savedMouseSpeed) {
        SystemParametersInfoA(SPI_SETMOUSESPEED, 0, reinterpret_cast<void*>(static_cast<intptr_t>(s_savedMouseSpeed)), 0);
    }
}

// VERIFY: 1f154fc9 auto
// OFFSET: 0x008697E0
static int32_t ProcessMouseEvent(uint32_t message, HWND hwnd, MOUSEBUTTON button, OSINPUT id) {
    POINT point;

    if (s_osMouseMode == OS_MOUSE_MODE_RELATIVE) {
        point.x = s_mousePos.x;
        point.y = s_mousePos.y;
    } else {
        GetCursorPos(&point);
        ScreenToClient(hwnd, &point);
    }

    OsQueuePut(id, button, point.x, point.y, 0);

    return message == WM_XBUTTONDOWN || message == WM_XBUTTONUP || message == WM_NCXBUTTONDOWN || message == WM_NCXBUTTONUP;
}

// VERIFY: 50290f71 auto
// OFFSET: 0x00869870
static int32_t HandleMouseDown(uint32_t message, uintptr_t wparam, int32_t* xbutton, HWND hwnd) {
    MOUSEBUTTON button;
    if (!ConvertButton(message, wparam, &button)) {
        return 0;
    }

    if (!s_osButtonState) {
        SetCapture(hwnd);
    }

    s_osButtonState |= button;

    int32_t result = ProcessMouseEvent(message, hwnd, button, OS_INPUT_MOUSE_DOWN);
    if (xbutton) {
        *xbutton = result;
    }

    return 1;
}

// VERIFY: a4738bfd auto
// OFFSET: 0x008698C0
static int32_t HandleMouseUp(uint32_t message, uintptr_t wparam, int32_t* xbutton, HWND hwnd) {
    MOUSEBUTTON button;
    if (!ConvertButton(message, wparam, &button)) {
        return 0;
    }

    s_osButtonState &= ~button;

    if (!s_osButtonState) {
        s_releasingCapture = 1;
        ReleaseCapture();
        s_releasingCapture = 0;
    }

    int32_t result = ProcessMouseEvent(message, hwnd, button, OS_INPUT_MOUSE_UP);
    if (xbutton) {
        *xbutton = result;
    }

    return 1;
}

// VERIFY: 256ddccc auto
// OFFSET: 0x00869930
static void MoveDragWindow(HWND hwnd, const C2iVector& delta) {
    MoveWindow(hwnd, s_dragRect.left + delta.x, s_dragRect.top + delta.y, s_dragRect.right - s_dragRect.left, s_dragRect.bottom - s_dragRect.top, TRUE);
}

// VERIFY: 6934b733 auto
// OFFSET: 0x00869970
static void ResizeDragWindow(int32_t hitTest, HWND hwnd, const C2iVector& delta) {
    if (s_WindowResizeLock) {
        return;
    }

    RECT rect = s_dragRect;

    switch (hitTest) {
        case HTLEFT:
        case HTTOPLEFT:
        case HTBOTTOMLEFT:
            rect.left += delta.x;
            break;

        case HTRIGHT:
        case HTTOPRIGHT:
        case HTBOTTOMRIGHT:
            rect.right += delta.x;
            break;

        default:
            break;
    }

    switch (hitTest) {
        case HTTOP:
        case HTTOPLEFT:
        case HTTOPRIGHT:
            rect.top += delta.y;
            break;

        case HTBOTTOM:
        case HTBOTTOMLEFT:
        case HTBOTTOMRIGHT:
            rect.bottom += delta.y;
            break;

        default:
            break;
    }

    if (SendMessageA(hwnd, WM_SIZING, hitTest - (HTLEFT - WMSZ_LEFT), reinterpret_cast<LPARAM>(&rect))) {
        MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
    }
}

// VERIFY: 04761905 auto
// OFFSET: 0x00869A70
static void DispatchMouseMove(HWND hwnd, int32_t point) {
    if (!s_dragging) {
        return;
    }

    C2iVector delta;
    delta.x = static_cast<int16_t>(point) - s_dragStart.x;
    delta.y = static_cast<int16_t>(point >> 16) - s_dragStart.y;

    if (s_dragHitTest == HTCAPTION) {
        MoveDragWindow(hwnd, delta);
    } else {
        ResizeDragWindow(s_dragHitTest, hwnd, delta);
    }
}

// VERIFY: 130c80fa manual
// OFFSET: 0x00869AD0
static void BeginDrag(int32_t hitTest, HWND hwnd, int32_t point) {
    s_dragCancel = 0;
    s_dragHitTest = hitTest;

    if (hitTest == HTCAPTION || (hitTest >= HTLEFT && hitTest <= HTBOTTOMRIGHT)) {
        SetFocus(hwnd);

        WINDOWPLACEMENT placement;
        if (GetWindowPlacement(hwnd, &placement) && placement.showCmd != SW_SHOWMAXIMIZED) {
            GetWindowRect(hwnd, &s_dragRect);
            s_dragStart.x = static_cast<int16_t>(point);
            s_dragStart.y = static_cast<int16_t>(point >> 16);
            SetCapture(hwnd);
            s_dragging = 1;
            return;
        }
    }

    s_dragging = 0;
}

// VERIFY: 218cae0c manual
// OFFSET: 0x00869B60
static void HandleNcDblClick(HWND hwnd, int32_t hitTest) {
    s_dragCancel = 1;

    if (s_dragging) {
        ReleaseCapture();
        s_dragging = 0;
    }

    switch (hitTest) {
        case HTCAPTION:
        case HTMAXBUTTON:
            ShowWindow(hwnd, IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
            break;

        case HTSYSMENU:
        case HTCLOSE:
            PostQuitMessage(0);
            break;

        case HTMINBUTTON:
            ShowWindow(hwnd, SW_MINIMIZE);
            break;

        default:
            break;
    }
}

// VERIFY: 61176f42 manual
// OFFSET: 0x00869C00
static void HandleDragEnd(int32_t hitTest, HWND hwnd, int32_t point) {
    if (!s_dragCancel) {
        if (s_dragHitTest == hitTest) {
            switch (hitTest) {
                case HTCAPTION:
                    if (s_dragging) {
                        C2iVector delta;
                        delta.x = static_cast<int16_t>(point) - s_dragStart.x;
                        delta.y = static_cast<int16_t>(point >> 16) - s_dragStart.y;
                        MoveDragWindow(hwnd, delta);
                    }
                    break;

                case HTMINBUTTON:
                    ShowWindow(hwnd, SW_MINIMIZE);
                    break;

                case HTMAXBUTTON:
                    ShowWindow(hwnd, IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
                    break;

                case HTLEFT:
                case HTRIGHT:
                case HTTOP:
                case HTTOPLEFT:
                case HTTOPRIGHT:
                case HTBOTTOM:
                case HTBOTTOMLEFT:
                case HTBOTTOMRIGHT:
                    DispatchMouseMove(hwnd, point);
                    break;

                case HTCLOSE:
                    PostQuitMessage(0);
                    break;

                default:
                    break;
            }
        }

        if (s_dragging) {
            ReleaseCapture();
            s_dragging = 0;
        }
    }

    s_dragCancel = 0;
}

// VERIFY: d8010977 manual
// OFFSET: 0x00869CF0
static bool DispatchMouseButtonChanges(HWND hwnd, uint32_t message, uintptr_t wparam, int32_t* xbutton) {
    uint32_t state = static_cast<uint32_t>(wparam) & ~0x1F;
    uint32_t changed = s_extraButtonState ^ state;
    s_extraButtonState = state;

    for (uint32_t i = 5; i < 32; i++) {
        if ((changed >> i) & 1) {
            MOUSEBUTTON button = static_cast<MOUSEBUTTON>(1 << i);
            ProcessMouseEvent(message, hwnd, button, (button & state) ? OS_INPUT_MOUSE_DOWN : OS_INPUT_MOUSE_UP);

            if (!s_osButtonState) {
                SetCapture(hwnd);
            }

            s_osButtonState |= button;
        }
    }

    if (xbutton) {
        *xbutton = 1;
    }

    return true;
}

// VERIFY: 43689e9e auto
// OFFSET: 0x00869D90
void OsInputSetEventPollProc(void (*pollProc)()) {
    s_eventPollProc = pollProc;
}

// VERIFY: 9aad8011 auto
// OFFSET: 0x00869DA0
static void CALLBACK OsInputCallEventPollProc(HWND hwnd, UINT message, UINT_PTR id, DWORD time) {
    s_eventPollProc();
}

// VERIFY: daae85b1 auto
// OFFSET: 0x00869DB0
static void CenterMouse() {
    // The offset-editor fix uses the saved cursor point as its relative anchor, keeping the pointer from jumping to screen center.
    HWND hwnd = static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX));
    RECT clientRect;
    if (!hwnd || !GetClientRect(hwnd, &clientRect)) {
        return;
    }

    auto nudgeInsideSafeMargin = [](int32_t& position, int32_t min, int32_t max) {
        constexpr int32_t safeMargin = 50;
        if (max - min <= safeMargin * 2) {
            position = min + (max - min) / 2;
        } else if (position < min + safeMargin) {
            ++position;
        } else if (position > max - safeMargin) {
            --position;
        }
    };

    nudgeInsideSafeMargin(s_mousePos.x, clientRect.left, clientRect.right);
    nudgeInsideSafeMargin(s_mousePos.y, clientRect.top, clientRect.bottom);

    POINT anchor = { s_mousePos.x, s_mousePos.y };
    if (!ClientToScreen(hwnd, &anchor)) {
        return;
    }

    OsLogicalToPhysicalPoint(hwnd, &anchor);
    s_centerMousePos.x = anchor.x;
    s_centerMousePos.y = anchor.y;
    OsSetPhysicalCursorPos(s_centerMousePos.x, s_centerMousePos.y);
}

// VERIFY: 0ffe5fb1 auto
// OFFSET: 0x00869E00
int32_t OsInputGet(OSINPUT* id, int32_t* param0, int32_t* param1, int32_t* param2, int32_t* param3) {
    *id = static_cast<OSINPUT>(-1);

    if (s_windowSize && !IsIconic(static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX)))) {
        s_windowSize = 0;

        RECT windowRect;
        GetWindowRect(static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX)), &windowRect);

        RECT adjust = { 0, 0, 0, 0 };
        AdjustWindowRectEx(&adjust, GetWindowLongA(static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX)), GWL_STYLE), FALSE, 0);

        int32_t width = (windowRect.right - adjust.right) - (windowRect.left - adjust.left);
        int32_t height = (windowRect.bottom - adjust.bottom) - (windowRect.top - adjust.top);

        if (width != s_defaultWindowRect.right || height != s_defaultWindowRect.bottom) {
            s_defaultWindowRect.left = 0;
            s_defaultWindowRect.top = 0;
            s_defaultWindowRect.right = width;
            s_defaultWindowRect.bottom = height;

            *id = OS_INPUT_SIZE;
            *param0 = width;
            *param1 = height;
            *param2 = 0;
            *param3 = 0;

            return 1;
        }
    }

    if (s_queueHead == s_queueTail) {
        OsPollJoysticks(s_appActive);

        MSG msg;
        BOOL available = PeekMessageA(&msg, nullptr, 0, 0, PM_NOREMOVE);

        while (s_queueHead == s_queueTail) {
            if (!available) {
                return 0;
            }

            if (!GetMessageA(&msg, nullptr, 0, 0)) {
                *id = OS_INPUT_SHUTDOWN;
                return 1;
            }

            if (OsGuiProcessMessage(&msg) || s_queueHead != s_queueTail) {
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);

            if (s_queueHead != s_queueTail) {
                break;
            }

            available = PeekMessageA(&msg, nullptr, 0, 0, PM_NOREMOVE);
        }
    }

    OsQueueGet(id, param0, param1, param2, param3);

    return 1;
}

// VERIFY: 11a72f4a auto
// OFFSET: 0x0086A020
void OsInputSetMouseMode(OS_MOUSE_MODE mode) {
    if (mode >= OS_MOUSE_MODES) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    if (mode == s_osMouseMode) {
        return;
    }

    if (mode == OS_MOUSE_MODE_RELATIVE) {
        HWND hwnd = static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX));
        POINT point;
        if (hwnd && GetCursorPos(&point) && ScreenToClient(hwnd, &point)) {
            SaveMouse(hwnd, point);
        }

        s_osMouseMode = mode;
        CenterMouse();
    } else if (s_osMouseMode == OS_MOUSE_MODE_RELATIVE) {
        s_osMouseMode = OS_MOUSE_MODE_NORMAL;
        RestoreMouse();
    }
}

// VERIFY: c38ece61 auto
// OFFSET: 0x0086A070
void OsInputSetMouseSpeed(float speed) {
    float clamped = speed < 0.1f ? 0.1f : (speed >= 2.0f ? 2.0f : speed);
    s_mouseSpeed = static_cast<int32_t>(clamped * 10.0f);

    if (s_appActive) {
        SystemParametersInfoA(SPI_SETMOUSESPEED, 0, reinterpret_cast<void*>(static_cast<intptr_t>(s_mouseSpeed)), 0);
    }
}

// VERIFY: 9440f9a0 auto
// OFFSET: 0x0086A0D0
void OsInputGetMousePosition(int32_t* x, int32_t* y) {
    HWND hwnd = static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX));

    POINT point;
    GetCursorPos(&point);
    ScreenToClient(hwnd, &point);
    SaveMouse(hwnd, point);

    if (x) {
        *x = point.x;
    }

    if (y) {
        *y = point.y;
    }
}

// VERIFY: 28ed4ab4 auto
// OFFSET: 0x0086A130
void OsInputSetMousePosition(int32_t x, int32_t y) {
    HWND hwnd = static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX));

    POINT point;
    point.x = x;
    point.y = y;
    SaveMouse(hwnd, point);

    ClientToScreen(hwnd, &point);
    OsLogicalToPhysicalPoint(s_mouseWnd, &point);
    OsSetPhysicalCursorPos(point.x, point.y);
}

// VERIFY: 97923916 auto
// OFFSET: 0x0086A1A0
int32_t OsGetDefaultWindowRect(tagRECT* rect) {
    if (!rect) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!s_defaultWindowRect.right || !s_defaultWindowRect.bottom) {
        if (!GetClientRect(static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX)), &s_defaultWindowRect)) {
            return 0;
        }
    }

    *rect = s_defaultWindowRect;

    return 1;
}

// VERIFY: 5f1e1652 manual
// OFFSET: 0x0086A210
intptr_t OsWindowProc(void* window, uint32_t message, uintptr_t wparam, intptr_t lparam) {
    HWND hwnd = static_cast<HWND>(window);
    UINT_PTR timer = 0;

    switch (message) {
        case WM_SIZE:
        case WM_DISPLAYCHANGE:
            s_windowSize = lparam;
            if (s_appActive && s_osMouseMode == OS_MOUSE_MODE_RELATIVE) {
                CenterMouse();
            }
            break;

        case WM_MOVE:
            if (s_appActive && s_osMouseMode == OS_MOUSE_MODE_RELATIVE) {
                CenterMouse();
            }
            break;

        case WM_ACTIVATE: {
            BOOL minimized = IsIconic(hwnd);
            s_appActive = LOWORD(wparam) != WA_INACTIVE && !minimized ? 1 : 0;
            s_osButtonState = 0;
            SystemParametersInfoA(SPI_SETMOUSESPEED, 0, reinterpret_cast<void*>(static_cast<intptr_t>(s_appActive ? s_mouseSpeed : s_savedMouseSpeed)), 0);
            OsQueuePut(OS_INPUT_FOCUS, s_appActive != 0, 0, 0, 0);
            if (s_appActive && s_osMouseMode == OS_MOUSE_MODE_RELATIVE) {
                CenterMouse();
            }
            break;
        }

        case WM_CLOSE:
            OsQueuePut(OS_INPUT_CLOSE, 0, 0, 0, 0);
            return 0;

        case WM_INPUTLANGCHANGE: {
            OsQueuePut(OS_INPUT_IME, OS_INPUT_IME_NOTIFYLANGUAGE, 0, 0, 0);

            CHARSETINFO charset;
            if (TranslateCharsetInfo(reinterpret_cast<DWORD*>(wparam), &charset, TCI_SRCCHARSET)) {
                s_codePage = charset.ciACP;
            }
            break;
        }

        case WM_IME_STARTCOMPOSITION:
            OsQueuePut(OS_INPUT_IME, OS_INPUT_IME_STARTCOMPOSITION, 0, 0, 0);
            return 1;

        case WM_IME_ENDCOMPOSITION:
            OsQueuePut(OS_INPUT_IME, OS_INPUT_IME_ENDCOMPOSITION, 0, 0, 0);
            return 1;

        case WM_IME_COMPOSITION: {
            int32_t flags = 0;
            if (lparam & GCS_CURSORPOS) {
                flags = 1;
            }
            if (lparam & GCS_COMPSTR) {
                flags |= 2;
            }
            if (lparam & GCS_RESULTSTR) {
                flags |= 4;
            }

            OsQueuePut(OS_INPUT_IME, OS_INPUT_IME_COMPOSITION, 0, flags, 0);
            return 1;
        }

        case WM_CHAR: {
            if (wparam < 0x20) {
                break;
            }

            uint32_t character = static_cast<uint32_t>(wparam);
            uint32_t ch = wparam & 0xFF;

            if (ch >= 0x80) {
                if (!s_codePage) {
                    s_codePage = GetACP();
                }

                switch (s_codePage) {
                    case 874:
                        character = s_thailookup[ch];
                        break;

                    case 1250:
                        character = s_latin2lookup[ch];
                        break;

                    case 1251:
                        character = s_cyrilliclookup[ch];
                        break;

                    case 1252:
                        if (ch < 0xA0) {
                            character = s_latin1lookup[ch & 0x7F];
                        }
                        break;

                    case 1254:
                        character = s_latin5lookup[ch];
                        break;

                    default:
                        break;
                }
            }

            OsQueuePut(OS_INPUT_CHAR, character, static_cast<int16_t>(lparam), 0, 0);
            return 0;
        }

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP: {
            int32_t down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
            uint32_t vkey = static_cast<uint32_t>(wparam);

            if (vkey == VK_SHIFT) {
                if (!down) {
                    OsQueuePut(OS_INPUT_KEY_UP, KEY_LSHIFT, 0, 0, 0);
                    OsQueuePut(OS_INPUT_KEY_UP, KEY_RSHIFT, 0, 0, 0);
                    return 0;
                }

                if (s_rshiftScanCode == 0x100) {
                    s_rshiftScanCode = MapVirtualKeyA(VK_RSHIFT, MAPVK_VK_TO_VSC);
                }

                vkey = ((lparam >> 16) & 0xFF) == s_rshiftScanCode ? VK_RSHIFT : VK_LSHIFT;
            } else if (vkey == VK_CONTROL) {
                vkey = (lparam & 0x1000000) ? VK_RCONTROL : VK_LCONTROL;
            } else if (vkey == VK_MENU) {
                vkey = (lparam & 0x1000000) ? VK_RMENU : VK_LMENU;
            }

            KEY key;
            if (!ConvertKeyCode(vkey, &key)) {
                break;
            }

            OsQueuePut(down ? OS_INPUT_KEY_DOWN : OS_INPUT_KEY_UP, key, LOWORD(lparam), 0, 0);

            if (key == KEY_F4 && OsGuiIsModifierKeyDown(OSGUI_KEY_ALT)) {
                break;
            }

            return 0;
        }

        case WM_NCLBUTTONDOWN:
            BeginDrag(static_cast<int32_t>(wparam), hwnd, static_cast<int32_t>(lparam));
            return 0;

        case WM_NCLBUTTONUP:
            HandleDragEnd(static_cast<int32_t>(wparam), hwnd, static_cast<int32_t>(lparam));
            return 0;

        case WM_NCLBUTTONDBLCLK:
            HandleNcDblClick(hwnd, static_cast<int32_t>(wparam));
            return 0;

        case WM_NCRBUTTONDOWN:
        case WM_NCRBUTTONUP:
        case WM_POPUPSYSTEMMENU:
            if (s_eventPollProc) {
                timer = SetTimer(hwnd, 0, 33, OsInputCallEventPollProc);
            }
            break;

        case WM_SYSCOMMAND:
            if (wparam == SC_CLOSE) {
                OsQueuePut(OS_INPUT_CLOSE, 0, 0, 0, 0);
                return 0;
            }

            if (!s_chainWindowProc && wparam == SC_KEYMENU) {
                return 0;
            }
            break;

        case WM_MOUSEMOVE: {
            if (s_dragging) {
                POINT point;
                point.x = static_cast<int16_t>(lparam);
                point.y = static_cast<int16_t>(lparam >> 16);
                ClientToScreen(hwnd, &point);
                DispatchMouseMove(hwnd, MAKELPARAM(point.x, point.y));
                return 0;
            }

            POINT point;

            if (s_osMouseMode != OS_MOUSE_MODE_NORMAL) {
                if (!OsGetPhysicalCursorPos(&point)) {
                    break;
                }

                if (point.x != s_centerMousePos.x || point.y != s_centerMousePos.y) {
                    OsQueuePut(OS_INPUT_MOUSE_MOVE_RELATIVE, 0, point.x - s_centerMousePos.x, point.y - s_centerMousePos.y, 0);
                    CenterMouse();
                }
            } else {
                GetCursorPos(&point);
                ScreenToClient(hwnd, &point);
                OsQueuePut(OS_INPUT_MOUSE_MOVE, 0, point.x, point.y, 0);
                SaveMouse(hwnd, point);
            }
            break;
        }

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN: {
            int32_t xbutton;
            if (HandleMouseDown(message, wparam, &xbutton, hwnd)) {
                return xbutton;
            }
            break;
        }

        case WM_LBUTTONUP:
            if (s_dragging) {
                POINT point;
                point.x = static_cast<int16_t>(lparam);
                point.y = static_cast<int16_t>(lparam >> 16);
                ClientToScreen(hwnd, &point);
                HandleDragEnd(s_dragHitTest, hwnd, MAKELPARAM(point.x, point.y));
                return 0;
            }

        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP: {
            int32_t xbutton;
            if (HandleMouseUp(message, wparam, &xbutton, hwnd)) {
                return xbutton;
            }
            break;
        }

        case WM_MOUSEWHEEL: {
            POINT point;

            if (s_osMouseMode == OS_MOUSE_MODE_RELATIVE) {
                point.x = s_mousePos.x;
                point.y = s_mousePos.y;
            } else {
                GetCursorPos(&point);
            }

            ScreenToClient(hwnd, &point);
            OsQueuePut(OS_INPUT_MOUSE_WHEEL, static_cast<int16_t>(wparam >> 16), point.x, point.y, 0);
            return 0;
        }

        case WM_CAPTURECHANGED: {
            if (s_releasingCapture) {
                break;
            }

            s_osButtonState = 0;

            POINT point;
            GetCursorPos(&point);
            ScreenToClient(hwnd, &point);
            OsQueuePut(OS_INPUT_CAPTURE_CHANGED, 0, point.x, point.y, 0);
            return 0;
        }

        case WM_IME_SETCONTEXT:
            OsQueuePut(OS_INPUT_IME, OS_INPUT_IME_NOTIFYLANGUAGE, 0, 0, 0);
            lparam = 0;
            break;

        case WM_IME_NOTIFY:
            switch (wparam) {
                case IMN_CHANGECANDIDATE:
                    OsQueuePut(OS_INPUT_IME, OS_INPUT_IME_CHANGECANDIDATES, 0, static_cast<int32_t>(lparam), 0);
                    break;

                case IMN_CLOSECANDIDATE:
                    OsQueuePut(OS_INPUT_IME, OS_INPUT_IME_CLOSECANDIDATES, 0, 0, 0);
                    break;

                case IMN_OPENCANDIDATE:
                    OsQueuePut(OS_INPUT_IME, OS_INPUT_IME_OPENCANDIDATES, 0, static_cast<int32_t>(lparam), 0);
                    break;

                case IMN_SETCONVERSIONMODE:
                    OsQueuePut(OS_INPUT_KEY_UP, KEY_LSHIFT, 0, 0, 0);
                    OsQueuePut(OS_INPUT_KEY_UP, KEY_RSHIFT, 0, 0, 0);
                    OsQueuePut(OS_INPUT_IME, OS_INPUT_IME_NOTIFYLANGUAGE, 0, 0, 0);
                    break;

                case IMN_SETOPENSTATUS:
                    OsQueuePut(OS_INPUT_IME, OS_INPUT_IME_NOTIFYLANGUAGE, 0, 0, 0);
                    break;

                default:
                    break;
            }
            break;

        case WM_IME_CONTROL:
        case WM_IME_COMPOSITIONFULL:
        case WM_IME_SELECT:
            return 0;

        case WM_USER + 3: {
            int32_t xbutton;
            if (DispatchMouseButtonChanges(hwnd, message, wparam, &xbutton)) {
                return xbutton;
            }
            break;
        }

        default:
            break;
    }

    intptr_t result;
    if (s_chainWindowProc) {
        result = s_chainWindowProc(window, message, wparam, lparam);
    } else {
        result = DefWindowProcA(hwnd, message, wparam, lparam);
    }

    if (timer) {
        KillTimer(hwnd, 0);
    }

    return result;
}

void OsInputEnableSecureMode(int32_t enable) {
}

#endif
