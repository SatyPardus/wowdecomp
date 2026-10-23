#include <cstdint>
#include <cstring>
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/Base/ConvertUTF.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Storm/H/storm.h"
#include "GameVersion.h"

// STATIC: 0x00ADBD48
int32_t s_execState = EM_NOTACTIVE;

// STATIC: 0x00CA1A28
static char s_execBuffer[0x2000];

// STATIC: 0x00CA3A28
char g_commandHistory[32][1024];

// STATIC: 0x00CABA28
uint32_t g_commandHistoryIndex;

// STATIC: 0x00CABA2C
COMMANDHANDLER g_defaultCommand;

// STATIC: 0x00CABA30
TSHashTable<CONSOLECOMMAND, HASHKEY_CONSTSTRI> g_consoleCommandHash;

// STATIC: 0x00CABA58
static char s_commandName[64];

// STATIC: 0x00ADBC6C
static const char* s_noHelpText = "No help yet";

// STATIC: 0x00ADBC70
static CATEGORYINFO s_categories[NUM_CATEGORIES] = {
    { CATEGORY_DEBUG, "debug" },
    { CATEGORY_GRAPHICS, "graphics" },
    { CATEGORY_CONSOLE, "console" },
    { CATEGORY_COMBAT, "combat" },
    { CATEGORY_GAME, "game" },
    { CATEGORY_DEFAULT, "default" },
    { CATEGORY_NET, "net" },
    { CATEGORY_SOUND, "sound" },
    { CATEGORY_GM, "gm" },
};

// VERIFY: 62a5fb46 auto
// OFFSET: 0x00768410
static int32_t ConsoleCommand_Quit(const char* command, const char* arguments) {
    ConsolePostClose();

    return 1;
}

// VERIFY: a074ad54 auto
// OFFSET: 0x00768420
static int32_t ConsoleCommand_Ver(const char* command, const char* arguments) {
    ConsoleWrite("WoW [Release] Build " GAME_BUILD_STRING " (" GAME_BUILD_DATE ")", DEFAULT_COLOR);

    return 1;
}

// VERIFY: 021fc29a manual
// OFFSET: 0x00768660
static int32_t ConsoleCommand_Help(const char* command, const char* arguments) {
    if (!*arguments) {
        char buffer[512] = { 0 };
        ConsoleWrite("Console help categories: ", WARNING_COLOR);

        for (uint32_t i = 0; i < NUM_CATEGORIES; i++) {
            SStrPack(buffer, s_categories[i].m_name, sizeof(buffer));

            if (i + 1 < NUM_CATEGORIES) {
                SStrPack(buffer, ", ", sizeof(buffer));
            }
        }

        ConsoleWrite(buffer, WARNING_COLOR);
        ConsoleWrite("For more information type 'help [command] or [category]'", WARNING_COLOR);

        return 1;
    }

    CATEGORY category = NUM_CATEGORIES;

    for (uint32_t i = 0; i < NUM_CATEGORIES; i++) {
        if (!SStrCmpI(s_categories[i].m_name, arguments, STORM_MAX_STR)) {
            category = s_categories[i].m_category;
            break;
        }
    }

    if (category != NUM_CATEGORIES) {
        char buffer[512] = { 0 };
        SStrPrintf(buffer, sizeof(buffer), "Commands registered for the category %s:", arguments);
        ConsoleWrite(buffer, WARNING_COLOR);

        uint32_t count = 0;
        buffer[0] = '\0';

        for (CONSOLECOMMAND* cmd = g_consoleCommandHash.m_fulllist.Head(); cmd; cmd = g_consoleCommandHash.m_fulllist.Next(cmd)) {
            if (cmd->m_category != category) {
                continue;
            }

            SStrPack(buffer, cmd->m_key.m_str, sizeof(buffer));
            SStrPack(buffer, ", ", sizeof(buffer));

            if (++count == 8) {
                ConsoleWrite(buffer, WARNING_COLOR);
                buffer[0] = '\0';
                count = 0;
            }
        }

        if (buffer[0]) {
            char* comma = SStrChrR(buffer, ',');

            if (comma) {
                *comma = '\0';
            }

            ConsoleWrite(buffer, WARNING_COLOR);
        } else {
            ConsoleWrite("NONE", WARNING_COLOR);
        }
    }

    CONSOLECOMMAND* cmd = g_consoleCommandHash.Ptr(arguments);

    if (cmd) {
        char line[197];
        SStrPrintf(line, sizeof(line), "Help for command %s:", arguments);
        ConsoleWrite(line, WARNING_COLOR);
        SStrPrintf(line, sizeof(line), "     %s %s", arguments, cmd->m_helpText ? cmd->m_helpText : s_noHelpText);
        ConsoleWrite(line, WARNING_COLOR);
    }

    return 1;
}

static int32_t ConsoleCommand_SetMap(const char* command, const char* arguments) {
    return 1;
}

// VERIFY: 2d898524 auto
// OFFSET: 0x00768440
const char* ConsoleCommandHistory(uint32_t index) {
    return g_commandHistory[(g_commandHistoryIndex - index - 1) & 31];
}

// VERIFY: af8b52a7 auto
// OFFSET: 0x0095BFB0
uint32_t ConsoleCommandHistoryDepth() {
    return sizeof(g_commandHistory) / sizeof(g_commandHistory[0]);
}

// VERIFY: 34cad70f manual
// OFFSET: 0x007685C0
void ConsoleCommandDestroy() {
    g_consoleCommandHash.Clear();
}

// VERIFY: 4f3a49b3 manual
// OFFSET: 0x00768900
CONSOLECOMMAND* ParseCommand(const char* commandLine, const char** command, char* arguments, uint32_t argsbytes) {
    const char* src = commandLine;
    char* dst = s_commandName;
    int32_t space = sizeof(s_commandName) - 1;
    int32_t chars;

    do {
        uint32_t c = sgetu8(reinterpret_cast<const uint8_t*>(src), &chars);

        if (c == 0xFFFFFFFF || c == ' ' || chars > space) {
            break;
        }

        if (chars > 0) {
            space -= chars;
        }

        while (chars-- > 0) {
            *dst++ = *src++;
        }
    } while (space > 0);

    *dst = '\0';

    if (command) {
        *command = s_commandName;
    }

    if (arguments) {
        for (uint32_t c = sgetu8(reinterpret_cast<const uint8_t*>(src), &chars); c != 0xFFFFFFFF && c == ' '; c = sgetu8(reinterpret_cast<const uint8_t*>(src), &chars)) {
            src += chars;
        }

        SStrCopy(arguments, src, argsbytes);

        for (uint32_t length = SStrLen(arguments); length > 0 && arguments[length - 1] == ' '; length--) {
            arguments[length - 1] = '\0';
        }
    }

    return g_consoleCommandHash.Ptr(s_commandName);
}

// VERIFY: ab158368 auto
// OFFSET: 0x007689E0
void ConsoleCommandUnregister(const char* command) {
    if (!command) {
        return;
    }

    CONSOLECOMMAND* cmd = g_consoleCommandHash.Ptr(command);

    if (cmd) {
        g_consoleCommandHash.Delete(cmd);
    }
}

// VERIFY: 10548ffd manual
// OFFSET: 0x00768A20
int32_t ConsoleCommandComplete(const char* partial, const char** previous, int32_t direction) {
    CONSOLECOMMAND* cmd;

    if (*previous) {
        cmd = g_consoleCommandHash.Ptr(*previous);

        if (!cmd) {
            return 0;
        }

        cmd = direction ? g_consoleCommandHash.m_fulllist.Prev(cmd) : g_consoleCommandHash.m_fulllist.Next(cmd);
    } else {
        cmd = g_consoleCommandHash.m_fulllist.Head();
    }

    uint32_t length = SStrLen(partial);

    while (cmd) {
        if (!SStrCmpI(partial, cmd->m_key.m_str, length)) {
            *previous = cmd->m_key.m_str;
            return 1;
        }

        cmd = direction ? g_consoleCommandHash.m_fulllist.Prev(cmd) : g_consoleCommandHash.m_fulllist.Next(cmd);
    }

    return 0;
}

// VERIFY: 1884998c auto
// OFFSET: 0x00768AE0
void ConsoleCommandWriteHelp(const char* command) {
    ConsoleCommand_Help("help", command);
}

// VERIFY: 9528b391 manual
// OFFSET: 0x00769100
int32_t ConsoleCommandRegister(const char* command, COMMANDHANDLER handler, CATEGORY category, const char* helpText) {
    if (SStrLen(command) >= sizeof(s_commandName) || g_consoleCommandHash.Ptr(command)) {
        return 0;
    }

    CONSOLECOMMAND* cmd = g_consoleCommandHash.New(command, 0, 0);
    cmd->m_category = category;
    cmd->m_handler = handler;
    cmd->m_helpText = helpText;

    return 1;
}

// VERIFY: 29f09bfc auto
// OFFSET: 0x007691C0
void ConsoleInitializeClientCommand() {
    ConsoleCommandRegister("help", ConsoleCommand_Help, CATEGORY_CONSOLE, "Provides help information about a command.");
    ConsoleCommandRegister("quit", ConsoleCommand_Quit, CATEGORY_DEFAULT, nullptr);
    ConsoleCommandRegister("ver", ConsoleCommand_Ver, CATEGORY_DEFAULT, nullptr);
    ConsoleCommandRegister("setmap", ConsoleCommand_SetMap, CATEGORY_DEFAULT, nullptr);
}

// VERIFY: e5a78592 auto
// OFFSET: 0x0076B2A0
int32_t AddLineToExecFile(const char* line) {
    if (s_execState == EM_PROMPTOVERWRITE) {
        if (*line == 'n') {
            ConsoleWrite("Canceled File Creation", ECHO_COLOR);
            s_execState = EM_NOTACTIVE;
            return 0;
        }

        if (*line == 'y') {
            ConsoleWrite("Begin Typing the commands", ECHO_COLOR);
            s_execState = EM_RECORDING;
            return 0;
        }

        ConsoleWrite("You must type 'y' to confirm overwrite. Process aborted!", ERROR_COLOR);
        s_execState = EM_NOTACTIVE;
        return 0;
    }

    if (!SStrCmpI(line, "end", STORM_MAX_STR)) {
        if (s_execState != EM_APPEND) {
            s_execState = EM_WRITEFILE;
        }

        return 1;
    }

    char text[260];
    SStrPrintf(text, sizeof(text), "%s\n", line);

    if (sizeof(s_execBuffer) - 1 - SStrLen(s_execBuffer) != SStrLen(text)) {
        SStrPack(s_execBuffer, text, sizeof(s_execBuffer));
    }

    return 0;
}

// VERIFY: 92bdf171 auto
// OFFSET: 0x0076B3B0
void AddToHistory(const char* command) {
    SStrCopy(g_commandHistory[g_commandHistoryIndex], command, sizeof(g_commandHistory[0]));
    g_commandHistoryIndex = (g_commandHistoryIndex + 1) & 31;
}
