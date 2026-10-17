#include "WoW/Source/Extension/ExtensionMgr.h"
#include <cstdarg>
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Extension/ExtensionLoader.h"

static HSLOG s_log;
static bool s_initialized;

ExtensionMgr::ExtensionMgr() : m_scriptCount(0) {
}

ExtensionMgr::~ExtensionMgr() {
}

ExtensionMgr* ExtensionMgr::instance() {
    static ExtensionMgr instance;
    return &instance;
}

void ExtensionMgr::Log(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    SStrVPrintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (s_log) {
        SLogWrite(s_log, "%s", buffer);
        SLogFlush(s_log);
    }

    // Dropped until the console screen is initialised (InitializeEngineCallback); see the header.
    ConsolePrintf("%s", buffer);
}

void ExtensionMgr::Initialize() {
    if (s_initialized) {
        return;
    }

    s_initialized = true;

    // SLogCreate creates the Logs directory; the path is relative to the client directory set by
    // SetPaths(), so this is bin/Win/Logs/Extensions.log for a tree build.
    if (!SLogCreate("Logs\\Extensions.log", SLOG_FLAG_OPENFILE, &s_log)) {
        s_log = nullptr;
    }

    Log("Extensions: %u compiled in", g_extensionCount);

    for (const EXTENSIONENTRY* entry = g_extensionEntries; entry->m_name; entry++) {
        Log("Extensions: loading %s", entry->m_name);
        entry->m_add();
    }

    Log("Extensions: %u script(s) registered", GetScriptCount());
}

void ExtensionMgr::Unload() {
    if (!s_initialized) {
        return;
    }

    ScriptRegistry<AssetScript>::Clear();
    ScriptRegistry<ClientDBScript>::Clear();
    ScriptRegistry<ClientScript>::Clear();
    ScriptRegistry<ConsoleScript>::Clear();
    ScriptRegistry<LuaScript>::Clear();
    ScriptRegistry<MapObjectScript>::Clear();
    ScriptRegistry<ModelScript>::Clear();
    ScriptRegistry<NetScript>::Clear();
    ScriptRegistry<ObjectScript>::Clear();
    ScriptRegistry<RenderScript>::Clear();
    ScriptRegistry<TerrainScript>::Clear();
    ScriptRegistry<TextureScript>::Clear();
    ScriptRegistry<UIScript>::Clear();
    ScriptRegistry<UnitScript>::Clear();
    ScriptRegistry<WorldScript>::Clear();
    m_scriptCount = 0;

    ClearMessageHandlers();
    ClearLuaRegistrations();
    ClearXmlElements();
    ClearConsoleRegistrations();

    Log("Extensions: unloaded");

    if (s_log) {
        SLogClose(s_log);
        s_log = nullptr;
    }

    s_initialized = false;
}
