#include "WoW/Source/Extension/ScriptDefines/ConsoleScript.h"
#include <vector>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Extension/ExtensionMgrMacros.h"

// Console commands registered by extensions, so they can be taken out again at unload (the
// engine's own table outlives the extensions by a few calls in BeginCloseGame).
static std::vector<const char*> s_consoleCommands;

bool ExtensionMgr::RegisterConsoleCommand(const char* name, COMMANDHANDLER handler, CATEGORY category, const char* help) {
    if (!name || !handler) {
        return false;
    }

    // ConsoleCommandRegister refuses a name already in the table (the engine's commands are in
    // first), so a collision leaves the stock command in place.
    if (!ConsoleCommandRegister(name, handler, category, help)) {
        Log("Extensions: console command %s not registered, the name is taken", name);
        return false;
    }

    s_consoleCommands.push_back(name);
    Log("Extensions: console command %s registered", name);

    return true;
}

void ExtensionMgr::UnregisterConsoleCommand(const char* name) {
    for (size_t i = 0; i < s_consoleCommands.size(); i++) {
        if (!SStrCmpI(s_consoleCommands[i], name, STORM_MAX_STR)) {
            ConsoleCommandUnregister(name);
            s_consoleCommands.erase(s_consoleCommands.begin() + i);
            return;
        }
    }
}

CVar* ExtensionMgr::RegisterCVar(const char* name, const char* help, uint32_t flags, const char* defaultValue, ExtensionCVarCallback callback, CATEGORY category, void* arg) {
    if (!name) {
        return nullptr;
    }

    // CVar::Register returns the existing variable when the name is known (Config.wtf values are
    // loaded before any registration and attached when the owner registers), so the default only
    // applies the first time the client ever sees the variable.
    CVar* cvar = CVar::Register(name, help, flags, defaultValue ? defaultValue : "", callback, category, false, arg, false);

    if (cvar) {
        Log("Extensions: CVar %s registered (value \"%s\")", name, cvar->m_stringValue.m_str);
    }

    return cvar;
}

void ExtensionMgr::ClearConsoleRegistrations() {
    for (const char* name : s_consoleCommands) {
        ConsoleCommandUnregister(name);
    }

    s_consoleCommands.clear();
}

void ExtensionMgr::OnConsoleCommand(const char* command, const char* arguments) {
    CALL_HOOKS(ConsoleScript, script->OnConsoleCommand(command, arguments));
}

ConsoleScript::ConsoleScript(const char* name) : ScriptObject(name) {
    ScriptRegistry<ConsoleScript>::AddScript(this);
    sExtensionMgr->Log("Extensions: registered ConsoleScript %s", name);
}
