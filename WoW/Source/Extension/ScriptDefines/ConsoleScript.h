#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_CONSOLESCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_CONSOLESCRIPT_H

#include <cstdint>
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/Types.h"
#include "WoW/Source/Extension/ScriptObject.h"

struct CVar;

// A CVar change callback for ExtensionMgr::RegisterCVar, the shape CVar::Register takes: the
// variable, its previous and new string values, and the registration's user pointer. Return true
// to accept the new value.
typedef bool (*ExtensionCVarCallback)(CVar* cvar, const char* oldValue, const char* newValue, void* arg);

// The console: commands (ConsoleCommandRegister, Engine/Source/ConsoleBase/ConsoleCommand.cpp, one
// name-keyed table alive from ConsoleInitializeClientCommand in InitializeGlobal to
// ConsoleCommandDestroy in BeginCloseGame) and CVars (CVar::Register, alive from CVar::Initialize to
// CVar::Destroy over the same span). Both exist before the extensions are loaded, so the
// registration services on ExtensionMgr apply at once. Commands run through
// ConsoleCommandExecute (WoW/Source/Console/ConsoleClient.cpp) for the console window, the
// "/console" chat command, Config.wtf exec files and the repeat buffer.
class ConsoleScript : public ScriptObject {
    protected:
    explicit ConsoleScript(const char* name);

    public:
    // ConsoleCommandExecute once the line is resolved to a registered command, right before its
    // handler runs: command is the registered name as typed, arguments the rest of the line.
    // Lines that match no command (handed to the default command or to "run") do not come here.
    virtual void OnConsoleCommand(const char* command, const char* arguments) {}
};

#endif
