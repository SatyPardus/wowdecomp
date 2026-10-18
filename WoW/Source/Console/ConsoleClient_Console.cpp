#include <cstdint>
#include <cstdio>
#if defined(__APPLE__)
#include <alloca.h>
#include "Engine/Source/ConsoleBase/Apple/Output.h"
#else
#include <malloc.h>
#endif
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Services/TextBlock.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "WoW/Source/Console/ConsoleClient.h"

// VERIFY: 7d2e703d auto
// OFFSET: 0x00763C10
int32_t ConsoleCommand_CloseConsole(const char* command, const char* arguments) {
    s_active = 0;

    return 0;
}

// VERIFY: a41a9191 manual
// OFFSET: 0x00763CB0
int32_t ConsoleCommand_RepeatHandler(const char* command, const char* arguments) {
    if (!arguments || !*arguments) {
        s_repeatCount = 0;

        return 1;
    }

    char* buffer = static_cast<char*>(alloca(SStrLen(arguments) + 1));
    SStrCopy(buffer, arguments, SStrLen(arguments) + 1);

    char* repeatCommand = buffer;

    while (*repeatCommand && *repeatCommand != '\t' && *repeatCommand != ' ') {
        repeatCommand++;
    }

    if (*repeatCommand) {
        *repeatCommand++ = 0;
    }

    uint32_t count = SStrToUnsigned(buffer);

    if (count && repeatCommand && *repeatCommand) {
        SStrPrintf(s_repeatBuffer, sizeof(s_repeatBuffer), "%s", repeatCommand);
        s_repeatCount = count;
    }

    return 1;
}

// VERIFY: cf5901b2 auto
// OFFSET: 0x00764E80
int32_t ConsoleCommand_ClearConsole(const char* command, const char* arguments) {
#if defined(__APPLE__)
    ConsoleDiscardOutput();
#endif
    s_NumLines = 0;
    s_linelist.Clear();

    return 1;
}

// VERIFY: 732da556 manual
// OFFSET: 0x00764EA0
int32_t ConsoleCommand_Proportional(const char* command, const char* arguments) {
    s_baseTextFlags ^= 0x10;

    for (CONSOLELINE* line = s_linelist.Head(); line; line = s_linelist.Next(line)) {
        GenerateNodeString(line);
    }

    return 1;
}

// VERIFY: 46e534de manual
// OFFSET: 0x00764EE0
int32_t ConsoleCommand_CharSpacing(const char* command, const char* arguments) {
    s_charSpacing = arguments && *arguments ? SStrToFloat(arguments) : 0.0f;

    for (CONSOLELINE* line = s_linelist.Head(); line; line = s_linelist.Next(line)) {
        GenerateNodeString(line);
    }

    return 1;
}

// VERIFY: f87a852d manual
// OFFSET: 0x00764F30
int32_t ConsoleCommand_DefaultSettings(const char* command, const char* arguments) {
    s_fontHeight = 0.02f;

    s_colorArray[INPUT_COLOR] = 0xFFFFFFFF;
    s_colorArray[DEFAULT_COLOR] = 0xFFFFFFFF;
    s_colorArray[ECHO_COLOR] = 0xFF808080;
    s_colorArray[ERROR_COLOR] = 0xFFFF0000;
    s_colorArray[WARNING_COLOR] = 0xFFFFFF00;
    s_colorArray[GLOBAL_COLOR] = 0xFFFFFFFF;
    s_colorArray[ADMIN_COLOR] = 0xFFFFFFFF;
    s_colorArray[BACKGROUND_COLOR] = 0xC0000000;
    s_colorArray[HIGHLIGHT_COLOR] = 0x80FFFFFF;

    SStrCopy(s_fontName, "Fonts\\ARIALN.ttf", sizeof(s_fontName));

    if (s_textFont) {
        HandleClose(reinterpret_cast<HOBJECT>(s_textFont));
    }

    s_textFont = TextBlockGenerateFont(s_fontName, 0, NDCToDDCHeight(s_fontHeight));

    for (CONSOLELINE* line = s_linelist.Head(); line; line = s_linelist.Next(line)) {
        GenerateNodeString(line);
    }

    s_consoleLines = 10.0f;
    s_consoleHeight = s_consoleLines * s_fontHeight;

    return 1;
}

// VERIFY: 44dcb448 auto
// OFFSET: 0x007650A0
int32_t ConsoleCommand_FontSize(const char* command, const char* arguments) {
    float height = SStrToFloat(arguments) * 0.001f;

    if (height < 0.01f) {
        height = 0.01f;
    } else if (height > 0.05f) {
        height = 0.05f;
    }

    s_fontHeight = height;

    if (s_textFont) {
        HandleClose(reinterpret_cast<HOBJECT>(s_textFont));
    }

    s_textFont = TextBlockGenerateFont(s_fontName, 0, NDCToDDCHeight(s_fontHeight));

    s_consoleLines = s_consoleHeight / s_fontHeight;
    s_consoleHeight = s_fontHeight * s_consoleLines;

    for (CONSOLELINE* line = s_linelist.Head(); line; line = s_linelist.Next(line)) {
        GenerateNodeString(line);
    }

    return 1;
}

// VERIFY: 5658d233 auto
// OFFSET: 0x00765170
int32_t ConsoleCommand_Font(const char* command, const char* arguments) {
    char fontName[260] = "Fonts\\";
    SStrPack(fontName, arguments, sizeof(fontName));
    SStrPack(fontName, ".ttf", sizeof(fontName));

    HTEXTFONT font = TextBlockGenerateFont(fontName, 0, NDCToDDCHeight(s_fontHeight));

    if (font) {
        if (s_textFont) {
            HandleClose(reinterpret_cast<HOBJECT>(s_textFont));
        }

        s_textFont = font;
        SStrCopy(s_fontName, fontName, STORM_MAX_STR);
    }

    for (CONSOLELINE* line = s_linelist.Head(); line; line = s_linelist.Next(line)) {
        GenerateNodeString(line);
    }

    return 1;
}

// VERIFY: a6270597 auto
// OFFSET: 0x00765400
int32_t ConsoleCommand_CurrentSettings(const char* command, const char* arguments) {
    char buffer[260];

    SStrPrintf(buffer, sizeof(buffer), "Font Height is %f", s_fontHeight);
    ConsoleWrite(buffer, DEFAULT_COLOR);

    SStrPrintf(buffer, sizeof(buffer), "Font Name is %s", s_fontName);
    ConsoleWrite(buffer, DEFAULT_COLOR);

    SStrPrintf(buffer, sizeof(buffer), "Number of Console Lines %f", s_consoleLines);
    ConsoleWrite(buffer, DEFAULT_COLOR);

    return 1;
}

// VERIFY: 24dc018f manual
// OFFSET: 0x007654A0
int32_t ConsoleCommand_FontColor(const char* command, const char* arguments) {
    char colorClass[64];
    uint32_t red;
    uint32_t green;
    uint32_t blue;

#if defined(__APPLE__)
    if (sscanf(arguments, "%63s %u %u %u", colorClass, &red, &green, &blue) != 4) {
#else
    if (sscanf(arguments, "%s %d %d %d", colorClass, &red, &green, &blue) != 4) {
#endif
        ConsoleWrite("Invalid number of parameters", ERROR_COLOR);
        ConsoleCommandWriteHelp(command);

        return 0;
    }

    int32_t colorType;

    if (!SStrCmpI(colorClass, "input", STORM_MAX_STR)) {
        colorType = INPUT_COLOR;
    } else if (!SStrCmpI(colorClass, "default", STORM_MAX_STR)) {
        colorType = DEFAULT_COLOR;
    } else if (!SStrCmpI(colorClass, "echo", STORM_MAX_STR)) {
        colorType = ECHO_COLOR;
    } else if (!SStrCmpI(colorClass, "error", STORM_MAX_STR)) {
        colorType = ERROR_COLOR;
    } else if (!SStrCmpI(colorClass, "warning", STORM_MAX_STR)) {
        colorType = WARNING_COLOR;
    } else if (!SStrCmpI(colorClass, "admin", STORM_MAX_STR)) {
        colorType = ADMIN_COLOR;
    } else if (!SStrCmpI(colorClass, "global", STORM_MAX_STR)) {
        colorType = GLOBAL_COLOR;
    } else {
        ConsoleWrite("Unknown color class. Choose 'input','echo','warning','admin',or 'global' as the first parameter", ERROR_COLOR);

        return 0;
    }

    if (red > 0xFF || green > 0xFF || blue > 0xFF) {
        ConsoleWrite("One or more colors are not in the 0 to 255 range or missing.", ERROR_COLOR);
        ConsoleWrite("Make sure to specify the red, green and blue colors.", ERROR_COLOR);

        return 0;
    }

    CImVector color;
    color.b = blue;
    color.g = green;
    color.r = red;
    color.a = 0xFF;

    if (colorType < NUM_COLORTYPES) {
        *reinterpret_cast<CImVector*>(&s_colorArray[colorType]) = color;
    }

    for (CONSOLELINE* line = s_linelist.Head(); line; line = s_linelist.Next(line)) {
        if (line->m_colorType == colorType && line->m_fontPointer) {
            GxuFontSetStringColor(line->m_fontPointer, color);
        }
    }

    return 1;
}

// VERIFY: 9395b76d auto
// OFFSET: 0x00765670
int32_t ConsoleCommand_BufferSize(const char* command, const char* arguments) {
    if (!arguments || !*arguments) {
        ConsoleWrite("Please specify how many lines to display", ERROR_COLOR);

        return 1;
    }

    float lines;

    if (sscanf(arguments, "%f", &lines) != 1) {
        ConsoleWrite("Invalid number of parameters", ERROR_COLOR);
        ConsoleCommandWriteHelp(command);

        return 0;
    }

    if (lines == 0.0f) {
        return 0;
    }

    s_consoleLines = lines;
    s_consoleHeight = lines * s_fontHeight;

    return 1;
}

// VERIFY: 0cba3f9b auto
// OFFSET: 0x00765700
int32_t ConsoleCommand_BackGroundColor(const char* command, const char* arguments) {
    uint32_t alpha;
    uint32_t red;
    uint32_t green;
    uint32_t blue;

    if (sscanf(arguments, "%d %d %d %d", &alpha, &red, &green, &blue) != 4) {
        ConsoleWrite("Invalid number of parameters", ERROR_COLOR);
        ConsoleCommandWriteHelp(command);

        return 0;
    }

    if (alpha > 0xFF || red > 0xFF || green > 0xFF || blue > 0xFF) {
        ConsoleWrite("One or more colors are not in the 0 to 255 range or missing.", ERROR_COLOR);
        ConsoleWrite("Make sure to specify the red, green and blue colors.", ERROR_COLOR);

        return 0;
    }

    CImVector color;
    color.b = blue;
    color.a = alpha;
    color.g = green;
    color.r = red;
    *reinterpret_cast<CImVector*>(&s_colorArray[BACKGROUND_COLOR]) = color;

    return 1;
}

// VERIFY: 83ee3329 auto
// OFFSET: 0x007657C0
int32_t ConsoleCommand_HighLightColor(const char* command, const char* arguments) {
    uint32_t alpha;
    uint32_t red;
    uint32_t green;
    uint32_t blue;

    if (sscanf(arguments, "%d %d %d %d", &alpha, &red, &green, &blue) != 4) {
        ConsoleWrite("Invalid number of parameters", ERROR_COLOR);
        ConsoleCommandWriteHelp(command);

        return 0;
    }

    if (alpha > 0xFF || red > 0xFF || green > 0xFF || blue > 0xFF) {
        ConsoleWrite("One or more colors are not in the 0 to 255 range or missing.", ERROR_COLOR);
        ConsoleWrite("Make sure to specify the red, green and blue colors.", ERROR_COLOR);

        return 0;
    }

    CImVector color;
    color.b = blue;
    color.a = alpha;
    color.g = green;
    color.r = red;
    *reinterpret_cast<CImVector*>(&s_colorArray[HIGHLIGHT_COLOR]) = color;

    return 1;
}

// VERIFY: 71965691 auto
// OFFSET: 0x00765880
int32_t ConsoleCommand_AppendLogToFile(const char* command, const char* arguments) {
    ConsoleWrite("Access Denied", ERROR_COLOR);

    return 0;
}
