#include "ModCore.h"
#include "BytecodeCache.h"
#include "AutoExport.h"
#include "FileWatcher.h"
#include "CustomPacket.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Ui/WorldFrame.h"

#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lualib.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Ui/GameUI.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>

namespace ModCore
{
    namespace
    {
        int Script_ModCoreLog(lua_State* L)
        {
            const char* level = lua_tostring(L, 1);
            const char* msg = lua_tostring(L, 2);
            if (!msg) return 0;

            std::cout << "[LUA " << (level ? level : "INFO") << "] " << msg << std::endl;
            return 0;
        }

        int Script_LoadFile(lua_State* L)
        {
            const char* filename = luaL_checkstring(L, 1);
            if (!filename)
            {
                lua_pushnil(L);
                lua_pushstring(L, "loadfile: invalid filename");
                return 2;
            }
            if (BytecodeCache::Get().LoadScript(L, filename, filename))
            {
                return 1;
            }
            lua_pushnil(L);
            lua_pushstring(L, "loadfile: failed to load script");
            return 2;
        }

        int Script_ExportApi(lua_State* L)
        {
            AutoExport::Get().ExportAll(L);
            lua_pushboolean(L, 1);
            return 1;
        }

        int Script_ModCoreSetCastAtCursor(lua_State* L)
        {
            bool enable = lua_toboolean(L, 1) != 0;
            ModCore::Manager().SetCastAtCursor(enable);
            return 0;
        }

        int Script_ModCoreIsCastAtCursor(lua_State* L)
        {
            lua_pushboolean(L, ModCore::Manager().IsCastAtCursor() ? 1 : 0);
            return 1;
        }

        int Script_ModCoreClickTerrain(lua_State* L)
        {
            CTerrainClickEvent evt;
            evt.m_guid = WOWGUID();
            if (lua_gettop(L) >= 3)
            {
                evt.m_point.x = static_cast<float>(lua_tonumber(L, 1));
                evt.m_point.y = static_cast<float>(lua_tonumber(L, 2));
                evt.m_point.z = static_cast<float>(lua_tonumber(L, 3));
            }
            else
            {
                CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
                if (player)
                {
                    evt.m_point = player->GetPosition();
                }
            }
            evt.m_button = MOUSE_BUTTON_LEFT;
            CGGameUI::HandleTerrainClick(evt);
            return 0;
        }

        int Script_ModCoreGetLastHitGuid(lua_State* L)
        {
            uint64_t guid = ModCore::Manager().GetLastHitGuid();
            char buf[32];
            snprintf(buf, sizeof(buf), "0x%llX", static_cast<unsigned long long>(guid));
            lua_pushstring(L, buf);
            lua_pushnumber(L, static_cast<lua_Number>(guid));
            return 2;
        }

        int Script_ModCoreGetLastHitPoint(lua_State* L)
        {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            ModCore::Manager().GetLastHitPoint(x, y, z);
            lua_pushnumber(L, x);
            lua_pushnumber(L, y);
            lua_pushnumber(L, z);
            return 3;
        }

        std::string ReadSmallFile(const std::filesystem::path& path)
        {
            std::ifstream file(path);
            if (!file.is_open()) return "";
            std::stringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }

        // Minimal JSON parser for simple key-value config
        bool ParseSimpleBool(const std::string& json, const std::string& key, bool defaultVal)
        {
            size_t pos = json.find("\"" + key + "\"");
            if (pos == std::string::npos) return defaultVal;
            size_t colon = json.find(':', pos);
            if (colon == std::string::npos) return defaultVal;
            size_t truePos = json.find("true", colon);
            size_t falsePos = json.find("false", colon);
            size_t comma = json.find(',', colon);
            size_t brace = json.find('}', colon);
            size_t endPos = std::min(comma, brace);

            if (truePos != std::string::npos && truePos < endPos) return true;
            if (falsePos != std::string::npos && falsePos < endPos) return false;
            return defaultVal;
        }
    }

    ModManager& ModManager::Get()
    {
        static ModManager instance;
        return instance;
    }

    ModManager::~ModManager()
    {
        Shutdown();
    }

    std::filesystem::path ModManager::ResolveModsDirectory()
    {
        // 1. Try directory containing current executable
        wchar_t exePathBuffer[MAX_PATH];
        if (GetModuleFileNameW(nullptr, exePathBuffer, MAX_PATH))
        {
            std::filesystem::path exeDir = std::filesystem::path(exePathBuffer).parent_path();
            if (std::filesystem::exists(exeDir / "mods"))
                return exeDir / "mods";
        }

        // 2. Try current working directory
        std::filesystem::path cwd = std::filesystem::current_path();
        if (std::filesystem::exists(cwd / "mods"))
            return cwd / "mods";

        // 3. Try bin/Win/mods
        if (std::filesystem::exists(cwd / "bin" / "Win" / "mods"))
            return cwd / "bin" / "Win" / "mods";

        // Fallback default
        return cwd / "mods";
    }

    void ModManager::LoadConfig()
    {
        std::filesystem::path configFile = m_modsPath / "config.json";
        if (!std::filesystem::exists(configFile))
            return;

        std::string json = ReadSmallFile(configFile);
        m_devMode = ParseSimpleBool(json, "devMode", true);
        m_cacheEnabled = ParseSimpleBool(json, "cacheEnabled", true);
        m_hotReloadEnabled = ParseSimpleBool(json, "hotReload", true);
        m_aiExportOnBoot = ParseSimpleBool(json, "aiExportOnBoot", true);
    }

    void ModManager::Initialize()
    {
        if (m_initialized)
            return;

        m_modsPath = ResolveModsDirectory();
        std::cout << "[ModCore] Initializing ModManager. Mods Root: " << m_modsPath.string() << std::endl;

        LoadConfig();

        BytecodeCache::Get().Initialize(m_modsPath / "cache", m_cacheEnabled);
        AutoExport::Get().Initialize(m_modsPath / "api");

        m_initialized = true;
    }

    void ModManager::Shutdown()
    {
        if (!m_initialized)
            return;

        FileWatcher::Get().Stop();
        m_initialized = false;
        std::cout << "[ModCore] ModManager shut down." << std::endl;
    }

    void ModManager::RegisterLuaBindings(lua_State* L)
    {
        if (!L)
            L = FrameScript_GetContext();
        if (!L)
            return;

        m_L = L;
        int top = lua_gettop(m_L);
        std::cout << "[ModCore] Registering Lua bindings and executing bootstrap..." << std::endl;

        // Register loadfile helper via BytecodeCache
        FrameScript_RegisterFunction("loadfile", Script_LoadFile);

        // Register internal logging helper
        FrameScript_RegisterFunction("_MODCORE_LOG", Script_ModCoreLog);

        // Register API export helper
        FrameScript_RegisterFunction("ModCore_ExportApi", Script_ExportApi);

        // Register engine interaction primitives
        FrameScript_RegisterFunction("_ModCore_SetCastAtCursor", Script_ModCoreSetCastAtCursor);
        FrameScript_RegisterFunction("_ModCore_IsCastAtCursor", Script_ModCoreIsCastAtCursor);
        FrameScript_RegisterFunction("_ModCore_ClickTerrain", Script_ModCoreClickTerrain);
        FrameScript_RegisterFunction("_ModCore_GetLastHitGuid", Script_ModCoreGetLastHitGuid);
        FrameScript_RegisterFunction("_ModCore_GetLastHitPoint", Script_ModCoreGetLastHitPoint);

        // Register custom packet primitives and embedded scripts
        CustomPacketManager::Get().RegisterLuaBindings(m_L);

        // Set BasePath in global table
        std::string setBasePathLua = "ModCore = ModCore or {}; ModCore.BasePath = [[" + m_modsPath.generic_string() + "/]];";
        FrameScript_Execute(setBasePathLua.c_str(), "ModCore_BasePath", nullptr);

        // Load and execute mods/system/bootstrap.lua via BytecodeCache
        std::filesystem::path bootstrapPath = m_modsPath / "system" / "bootstrap.lua";
        if (BytecodeCache::Get().LoadScript(m_L, bootstrapPath, "bootstrap.lua"))
        {
            if (lua_pcall(m_L, 0, 0, 0) != 0)
            {
                const char* err = lua_tostring(m_L, -1);
                std::cerr << "[ModCore] Error executing bootstrap.lua: " << (err ? err : "") << std::endl;
                lua_pop(m_L, 1);
            }
            else
            {
                std::cout << "[ModCore] Bootstrap executed successfully." << std::endl;
            }
        }

        // Mount all mods
        DiscoverAndMountMods();

        // Run AI Auto-Export
        if (m_aiExportOnBoot)
        {
            AutoExport::Get().ExportAll(m_L);
        }

        // Start file watcher if hot-reload is enabled
        if (m_devMode && m_hotReloadEnabled)
        {
            FileWatcher::Get().Start(m_modsPath);
            std::cout << "[ModCore] FileWatcher active on " << m_modsPath.string() << std::endl;
        }

        // Ensure stack is restored for caller
        lua_settop(m_L, top);
    }

    void ModManager::DiscoverAndMountMods()
    {
        if (!std::filesystem::exists(m_modsPath))
            return;

        // Mount user_mods
        std::filesystem::path userMods = m_modsPath / "user_mods";
        if (std::filesystem::exists(userMods))
        {
            for (const auto& entry : std::filesystem::directory_iterator(userMods))
            {
                if (entry.is_directory() && std::filesystem::exists(entry.path() / "mod.json"))
                {
                    MountMod(entry.path());
                }
            }
        }

        // Mount any other mods in mods/ root (excluding system, cache, api, user_mods)
        for (const auto& entry : std::filesystem::directory_iterator(m_modsPath))
        {
            if (entry.is_directory())
            {
                std::string name = entry.path().filename().string();
                if (name != "system" && name != "cache" && name != "api" && name != "user_mods")
                {
                    if (std::filesystem::exists(entry.path() / "mod.json"))
                    {
                        MountMod(entry.path());
                    }
                }
            }
        }
    }

    bool ModManager::MountMod(const std::filesystem::path& modDir)
    {
        if (!m_L)
            return false;

        std::filesystem::path manifestPath = modDir / "mod.json";
        std::string manifestContent = ReadSmallFile(manifestPath);
        if (manifestContent.empty())
            return false;

        // Call ModCore.MountMod in Lua
        std::string mountLua = "if ModCore and ModCore.MountMod then "
                               "  ModCore.MountMod([[" + modDir.generic_string() + "]], { "
                               "    id = [[" + modDir.filename().generic_string() + "]], "
                               "    dir = [[" + modDir.generic_string() + "]] "
                               "  }) "
                               "end";
        FrameScript_Execute(mountLua.c_str(), "MountMod", nullptr);
        return true;
    }

    bool ModManager::ReloadScript(const std::filesystem::path& scriptPath)
    {
        if (!m_L || !std::filesystem::exists(scriptPath))
            return false;

        int top = lua_gettop(m_L);
        std::cout << "[ModCore] Hot-reloading script: " << scriptPath.filename().string() << std::endl;
        if (BytecodeCache::Get().LoadScript(m_L, scriptPath, scriptPath.filename().string()))
        {
            if (lua_pcall(m_L, 0, 0, 0) != 0)
            {
                const char* err = lua_tostring(m_L, -1);
                std::cerr << "[ModCore] Error hot-reloading " << scriptPath.string() << ": " << (err ? err : "") << std::endl;
                lua_pop(m_L, 1);
                lua_settop(m_L, top);
                return false;
            }
            std::cout << "[ModCore] Successfully reloaded: " << scriptPath.filename().string() << std::endl;
            lua_settop(m_L, top);
            return true;
        }
        lua_settop(m_L, top);
        return false;
    }

    // =========================================================================
    // Generic Hook Dispatcher
    // =========================================================================

    bool ModManager::DispatchHookEx(const char* hookName, 
                                    std::function<int(lua_State*)> pushArgs, 
                                    std::function<void(lua_State*, int)> readResult)
    {
        lua_State* L = FrameScript_GetContext();
        if (!L || !hookName) return false;
        m_L = L;
        int top = lua_gettop(m_L);

        bool handled = false;
        lua_getglobal(m_L, "ModCore");
        if (lua_istable(m_L, -1))
        {
            lua_getfield(m_L, -1, "DispatchHook");
            if (lua_isfunction(m_L, -1))
            {
                lua_pushstring(m_L, hookName);
                int argCount = 1;
                if (pushArgs)
                {
                    argCount += pushArgs(m_L);
                }

                if (lua_pcall(m_L, argCount, 1, 0) == 0)
                {
                    handled = !lua_isnil(m_L, -1);
                    if (readResult)
                    {
                        readResult(m_L, -1);
                    }
                    lua_pop(m_L, 1);
                }
                else
                {
                    const char* err = lua_tostring(m_L, -1);
                    std::cerr << "[ModCore] Error running hook '" << hookName << "': " << (err ? err : "") << std::endl;
                    lua_pop(m_L, 1);
                }
            }
            else
            {
                lua_pop(m_L, 1); // pop non-function
            }
        }
        lua_pop(m_L, 1); // pop ModCore
        lua_settop(m_L, top);
        return handled;
    }

    bool ModManager::DispatchHook(const char* hookName)
    {
        return DispatchHookEx(hookName);
    }

    // =========================================================================
    // Lifecycle Dispatches
    // =========================================================================

    void ModManager::OnEngineInit()
    {
        DispatchHook("OnEngineInit");
    }

    void ModManager::OnEngineShutdown()
    {
        DispatchHook("OnEngineShutdown");
    }

    void ModManager::OnGameInitialize()
    {
        DispatchHook("OnGameInitialize");
    }

    void ModManager::OnGameDestroy()
    {
        DispatchHook("OnGameDestroy");
    }

    void ModManager::OnFrameTick(float dt)
    {
        DispatchHookEx("OnFrameTick", [=](lua_State* L) {
            lua_pushnumber(L, dt);
            return 1;
        });
    }

    // =========================================================================
    // World / Map Dispatches
    // =========================================================================

    void ModManager::OnEnterWorld()
    {
        DispatchHook("OnEnterWorld");
    }

    void ModManager::OnLeaveWorld()
    {
        DispatchHook("OnLeaveWorld");
    }

    void ModManager::OnMapChange(const char* mapName, uint32_t zoneId)
    {
        DispatchHookEx("OnMapChange", [=](lua_State* L) {
            lua_pushstring(L, mapName ? mapName : "");
            lua_pushinteger(L, zoneId);
            return 2;
        });
    }

    void ModManager::OnWorldUpdate(float dt)
    {
        // Process hot-reloaded files
        if (m_hotReloadEnabled)
        {
            auto modifiedFiles = FileWatcher::Get().PopModifiedFiles();
            for (const auto& path : modifiedFiles)
            {
                ReloadScript(path);
            }
        }

        DispatchHookEx("OnWorldUpdate", [=](lua_State* L) {
            lua_pushnumber(L, dt);
            return 1;
        });
    }

    void ModManager::OnWorldRender(void* worldFrame)
    {
        DispatchHookEx("OnWorldRender", [=](lua_State* L) {
            lua_pushlightuserdata(L, worldFrame);
            return 1;
        });
    }

    void ModManager::OnWorldHitTest(uint64_t guid, const void* point, bool hitObject)
    {
        m_lastHitGuid = guid;
        if (point)
        {
            const auto* pt = static_cast<const C3Vector*>(point);
            m_lastHitPointX = pt->x;
            m_lastHitPointY = pt->y;
            m_lastHitPointZ = pt->z;
        }
        else
        {
            m_lastHitPointX = 0.0f;
            m_lastHitPointY = 0.0f;
            m_lastHitPointZ = 0.0f;
        }

        DispatchHookEx("OnWorldHitTest", [=](lua_State* L) {
            lua_pushnumber(L, static_cast<lua_Number>(guid));
            if (point)
            {
                const auto* pt = static_cast<const C3Vector*>(point);
                lua_createtable(L, 0, 3);
                lua_pushnumber(L, pt->x);
                lua_setfield(L, -2, "x");
                lua_pushnumber(L, pt->y);
                lua_setfield(L, -2, "y");
                lua_pushnumber(L, pt->z);
                lua_setfield(L, -2, "z");
            }
            else
            {
                lua_pushnil(L);
            }
            lua_pushboolean(L, hitObject ? 1 : 0);
            return 3;
        });
    }

    bool ModManager::OnTerrainTrack(const void* point)
    {
        bool consumed = false;
        DispatchHookEx("OnTerrainTrack", [=](lua_State* L) {
            if (point)
            {
                const auto* pt = static_cast<const C3Vector*>(point);
                lua_createtable(L, 0, 3);
                lua_pushnumber(L, pt->x);
                lua_setfield(L, -2, "x");
                lua_pushnumber(L, pt->y);
                lua_setfield(L, -2, "y");
                lua_pushnumber(L, pt->z);
                lua_setfield(L, -2, "z");
            }
            else
            {
                lua_pushnil(L);
            }
            return 1;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    bool ModManager::OnTerrainClick(int32_t button, float x, float y, float z)
    {
        bool consumed = false;
        DispatchHookEx("OnTerrainClick", [=](lua_State* L) {
            lua_pushinteger(L, button);
            lua_pushnumber(L, x);
            lua_pushnumber(L, y);
            lua_pushnumber(L, z);
            return 4;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    // =========================================================================
    // Glue / UI Dispatches
    // =========================================================================

    void ModManager::OnGlueResume()
    {
        DispatchHook("OnGlueResume");
    }

    void ModManager::OnGlueIdle()
    {
        DispatchHook("OnGlueIdle");
    }

    void ModManager::OnGlueScreenChange(const char* screenName)
    {
        DispatchHookEx("OnGlueScreenChange", [=](lua_State* L) {
            lua_pushstring(L, screenName ? screenName : "");
            return 1;
        });
    }

    bool ModManager::IsLoginBlocked()
    {
        bool blocked = false;
        DispatchHookEx("IsLoginBlocked", nullptr, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                blocked = (lua_toboolean(L, idx) != 0);
        });
        return blocked;
    }

    void ModManager::OnCharacterSelect(int32_t charIndex)
    {
        DispatchHookEx("OnCharacterSelect", [=](lua_State* L) {
            lua_pushinteger(L, charIndex);
            return 1;
        });
    }

    void ModManager::OnCharacterCreate(const char* name, uint8_t race, uint8_t classId, uint8_t gender)
    {
        DispatchHookEx("OnCharacterCreate", [=](lua_State* L) {
            lua_pushstring(L, name ? name : "");
            lua_pushinteger(L, race);
            lua_pushinteger(L, classId);
            lua_pushinteger(L, gender);
            return 4;
        });
    }

    // =========================================================================
    // Network Dispatches
    // =========================================================================

    bool ModManager::OnPacketReceived(uint32_t opcode, uint32_t time, void* dataStore)
    {
        // 1. Intercept custom network packets (0x102 / 0x7A00)
        if (CustomPacketManager::Get().HandlePacket(opcode, time, dataStore))
        {
            return true;
        }

        bool handled = false;
        DispatchHookEx("OnPacketReceived", [=](lua_State* L) {
            lua_pushinteger(L, opcode);
            lua_pushinteger(L, time);
            lua_pushlightuserdata(L, dataStore);
            return 3;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                handled = (lua_toboolean(L, idx) != 0);
        });
        return handled;
    }

    bool ModManager::OnPacketSend(uint32_t opcode, uint32_t size, void* dataStore)
    {
        bool consumed = false;
        DispatchHookEx("OnPacketSend", [=](lua_State* L) {
            lua_pushinteger(L, opcode);
            lua_pushinteger(L, size);
            lua_pushlightuserdata(L, dataStore);
            return 3;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    void ModManager::OnDisconnect()
    {
        DispatchHook("OnDisconnect");
    }

    // =========================================================================
    // Spell Dispatches
    // =========================================================================

    bool ModManager::OnSpellQuery(const char* queryType, uint32_t spellId, int32_t* outResult)
    {
        bool handled = false;
        DispatchHookEx("OnSpellQuery", [=](lua_State* L) {
            lua_pushstring(L, queryType);
            lua_pushinteger(L, spellId);
            return 2;
        }, [&](lua_State* L, int idx) {
            if (lua_isnumber(L, idx))
            {
                if (outResult)
                    *outResult = static_cast<int32_t>(lua_tointeger(L, idx));
                handled = true;
            }
            else if (lua_isboolean(L, idx))
            {
                if (outResult)
                    *outResult = lua_toboolean(L, idx);
                handled = true;
            }
        });
        return handled;
    }

    void ModManager::OnSpellCast(uint32_t spellId, uint64_t casterGuid, uint64_t targetGuid)
    {
        DispatchHookEx("OnSpellCast", [=](lua_State* L) {
            lua_pushinteger(L, spellId);
            lua_pushnumber(L, static_cast<lua_Number>(casterGuid));
            lua_pushnumber(L, static_cast<lua_Number>(targetGuid));
            return 3;
        });
    }

    void ModManager::OnSpellPacket(uint32_t opcode, void* msg)
    {
        DispatchHookEx("OnSpellPacket", [=](lua_State* L) {
            lua_pushinteger(L, opcode);
            lua_pushlightuserdata(L, msg);
            return 2;
        });
    }

    void ModManager::OnChannelStart(void* msg)
    {
        DispatchHookEx("OnChannelStart", [=](lua_State* L) {
            lua_pushlightuserdata(L, msg);
            return 1;
        });
    }

    void ModManager::OnChannelUpdate(void* msg)
    {
        DispatchHookEx("OnChannelUpdate", [=](lua_State* L) {
            lua_pushlightuserdata(L, msg);
            return 1;
        });
    }

    void ModManager::OnChannelStop()
    {
        DispatchHook("OnChannelStop");
    }

    void ModManager::OnSpellDestroy()
    {
        DispatchHook("OnSpellDestroy");
    }

    // =========================================================================
    // Tooltip Dispatches
    // =========================================================================

    bool ModManager::OnTooltipEvaluate(uint32_t spellId, void* caster, const char* defaultText, std::string& outText)
    {
        bool overridden = false;
        DispatchHookEx("OnTooltipEvaluate", [=](lua_State* L) {
            lua_pushinteger(L, spellId);

            // Push a table wrapper for caster so caster:GetAttackPower() and caster:GetSpellPower() work
            lua_newtable(L);
            lua_pushstring(L, "player");
            lua_setfield(L, -2, "unit");
            lua_pushlightuserdata(L, caster);
            lua_setfield(L, -2, "_ptr");

            lua_pushcfunction(L, [](lua_State* state) -> int {
                CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
                if (player && player->m_unit) {
                    float ap = static_cast<float>(player->m_unit->m_unitFieldRangedAttackPower + player->m_unit->m_unitFieldRangedAttackPowerMods[0] + player->m_unit->m_unitFieldRangedAttackPowerMods[1]);
                    float meleeAp = static_cast<float>(player->m_unit->m_unitFieldAttackPower + player->m_unit->m_unitFieldAttackPowerMods[0] + player->m_unit->m_unitFieldAttackPowerMods[1]);
                    lua_pushnumber(state, (std::max)(ap, meleeAp));
                    return 1;
                }
                lua_pushnumber(state, 0);
                return 1;
            });
            lua_setfield(L, -2, "GetAttackPower");

            lua_pushcfunction(L, [](lua_State* state) -> int {
                CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
                if (!player) {
                    lua_pushnumber(state, 0);
                    return 1;
                }
                uint32_t school = 0;
                if (lua_isstring(state, 2)) {
                    const char* s = lua_tostring(state, 2);
                    if (_stricmp(s, "HOLY") == 0) school = 1;
                    else if (_stricmp(s, "FIRE") == 0) school = 2;
                    else if (_stricmp(s, "NATURE") == 0) school = 3;
                    else if (_stricmp(s, "FROST") == 0) school = 4;
                    else if (_stricmp(s, "SHADOW") == 0) school = 5;
                    else if (_stricmp(s, "ARCANE") == 0) school = 6;
                } else if (lua_isnumber(state, 2)) {
                    school = static_cast<uint32_t>(lua_tonumber(state, 2));
                }
                int32_t sp = player->GetModDamageDonePos(school) + player->GetModDamageDoneNeg(school);
                lua_pushnumber(state, sp > 0 ? sp : 0);
                return 1;
            });
            lua_setfield(L, -2, "GetSpellPower");

            lua_pushstring(L, defaultText ? defaultText : "");
            return 3;
        }, [&](lua_State* L, int idx) {
            if (lua_isstring(L, idx))
            {
                outText = lua_tostring(L, idx);
                overridden = true;
            }
        });
        return overridden;
    }

    void ModManager::OnTooltipSetSpell(void* tooltip, uint32_t spellId)
    {
        DispatchHookEx("OnTooltipSetSpell", [=](lua_State* L) {
            lua_pushlightuserdata(L, tooltip);
            lua_pushinteger(L, spellId);
            return 2;
        });
    }

    void ModManager::OnTooltipSetItem(void* tooltip, uint32_t itemId)
    {
        DispatchHookEx("OnTooltipSetItem", [=](lua_State* L) {
            lua_pushlightuserdata(L, tooltip);
            lua_pushinteger(L, itemId);
            return 2;
        });
    }

    void ModManager::OnTooltipSetUnit(void* tooltip, uint64_t unitGuid)
    {
        DispatchHookEx("OnTooltipSetUnit", [=](lua_State* L) {
            lua_pushlightuserdata(L, tooltip);
            lua_pushnumber(L, static_cast<lua_Number>(unitGuid));
            return 2;
        });
    }

    // =========================================================================
    // ObjectMgr Dispatches
    // =========================================================================

    void ModManager::OnObjectCreate(uint64_t guid, uint32_t typeId, void* objectPtr)
    {
        DispatchHookEx("OnObjectCreate", [=](lua_State* L) {
            lua_pushnumber(L, static_cast<lua_Number>(guid));
            lua_pushinteger(L, typeId);
            lua_pushlightuserdata(L, objectPtr);
            return 3;
        });
    }

    void ModManager::OnObjectDestroy(uint64_t guid, uint32_t typeId, void* objectPtr)
    {
        DispatchHookEx("OnObjectDestroy", [=](lua_State* L) {
            lua_pushnumber(L, static_cast<lua_Number>(guid));
            lua_pushinteger(L, typeId);
            lua_pushlightuserdata(L, objectPtr);
            return 3;
        });
    }

    void ModManager::OnTargetChanged(uint64_t oldGuid, uint64_t newGuid)
    {
        DispatchHookEx("OnTargetChanged", [=](lua_State* L) {
            lua_pushnumber(L, static_cast<lua_Number>(oldGuid));
            lua_pushnumber(L, static_cast<lua_Number>(newGuid));
            return 2;
        });
    }

    // =========================================================================
    // Input Dispatches
    // =========================================================================

    bool ModManager::OnKeyDown(uint32_t key, uint32_t modifiers, const char* keyString)
    {
        bool consumed = false;
        DispatchHookEx("OnKeyDown", [=](lua_State* L) {
            lua_pushinteger(L, key);
            lua_pushinteger(L, modifiers);
            lua_pushstring(L, keyString ? keyString : "");
            return 3;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    bool ModManager::OnKeyUp(uint32_t key, uint32_t modifiers, const char* keyString)
    {
        bool consumed = false;
        DispatchHookEx("OnKeyUp", [=](lua_State* L) {
            lua_pushinteger(L, key);
            lua_pushinteger(L, modifiers);
            lua_pushstring(L, keyString ? keyString : "");
            return 3;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    bool ModManager::OnMouseDown(int32_t button, int32_t x, int32_t y)
    {
        bool consumed = false;
        DispatchHookEx("OnMouseDown", [=](lua_State* L) {
            lua_pushinteger(L, button);
            lua_pushinteger(L, x);
            lua_pushinteger(L, y);
            return 3;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    bool ModManager::OnMouseUp(int32_t button, int32_t x, int32_t y)
    {
        bool consumed = false;
        DispatchHookEx("OnMouseUp", [=](lua_State* L) {
            lua_pushinteger(L, button);
            lua_pushinteger(L, x);
            lua_pushinteger(L, y);
            return 3;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    bool ModManager::OnMouseWheel(int32_t delta)
    {
        bool consumed = false;
        DispatchHookEx("OnMouseWheel", [=](lua_State* L) {
            lua_pushinteger(L, delta);
            return 1;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    // =========================================================================
    // Chat Dispatches
    // =========================================================================

    bool ModManager::OnChatMessage(int32_t type, const char* sender, const char* message, const char* channel)
    {
        bool consumed = false;
        DispatchHookEx("OnChatMessage", [=](lua_State* L) {
            lua_pushinteger(L, type);
            lua_pushstring(L, sender ? sender : "");
            lua_pushstring(L, message ? message : "");
            lua_pushstring(L, channel ? channel : "");
            return 4;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    bool ModManager::OnSendChatMessage(const char* message, int32_t type, int32_t language, const char* target)
    {
        bool consumed = false;
        DispatchHookEx("OnSendChatMessage", [=](lua_State* L) {
            lua_pushstring(L, message ? message : "");
            lua_pushinteger(L, type);
            lua_pushinteger(L, language);
            lua_pushstring(L, target ? target : "");
            return 4;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    // =========================================================================
    // Environment & Weather Dispatches
    // =========================================================================

    void ModManager::OnWeatherChange(uint32_t type, float intensity)
    {
        DispatchHookEx("OnWeatherChange", [=](lua_State* L) {
            lua_pushinteger(L, type);
            lua_pushnumber(L, intensity);
            return 2;
        });
    }

    void ModManager::OnZoneChange(uint32_t areaId, uint32_t subzoneId, const char* zoneName)
    {
        DispatchHookEx("OnZoneChange", [=](lua_State* L) {
            lua_pushinteger(L, areaId);
            lua_pushinteger(L, subzoneId);
            lua_pushstring(L, zoneName ? zoneName : "");
            return 3;
        });
    }

    // =========================================================================
    // Loot Dispatches
    // =========================================================================

    void ModManager::OnLootOpen(uint64_t guid, int32_t coins, uint32_t lootType)
    {
        DispatchHookEx("OnLootOpen", [=](lua_State* L) {
            lua_pushnumber(L, static_cast<lua_Number>(guid));
            lua_pushinteger(L, coins);
            lua_pushinteger(L, lootType);
            return 3;
        });
    }

    void ModManager::OnLootSlot(uint32_t slot)
    {
        DispatchHookEx("OnLootSlot", [=](lua_State* L) {
            lua_pushinteger(L, slot);
            return 1;
        });
    }

    void ModManager::OnLootClose()
    {
        DispatchHook("OnLootClose");
    }

    // =========================================================================
    // Merchant Dispatches
    // =========================================================================

    bool ModManager::OnMerchantBuy(int32_t index, int32_t count)
    {
        bool consumed = false;
        DispatchHookEx("OnMerchantBuy", [=](lua_State* L) {
            lua_pushinteger(L, index);
            lua_pushinteger(L, count);
            return 2;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    // =========================================================================
    // Quest & Gossip Dispatches
    // =========================================================================

    bool ModManager::OnQuestAccept(uint64_t questGiverGuid)
    {
        bool consumed = false;
        DispatchHookEx("OnQuestAccept", [=](lua_State* L) {
            lua_pushnumber(L, static_cast<lua_Number>(questGiverGuid));
            return 1;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    bool ModManager::OnQuestComplete(uint64_t questGiverGuid)
    {
        bool consumed = false;
        DispatchHookEx("OnQuestComplete", [=](lua_State* L) {
            lua_pushnumber(L, static_cast<lua_Number>(questGiverGuid));
            return 1;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    bool ModManager::OnQuestReward(int32_t rewardIndex)
    {
        bool consumed = false;
        DispatchHookEx("OnQuestReward", [=](lua_State* L) {
            lua_pushinteger(L, rewardIndex);
            return 1;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    bool ModManager::OnGossipSelect(int32_t optionIndex)
    {
        bool consumed = false;
        DispatchHookEx("OnGossipSelect", [=](lua_State* L) {
            lua_pushinteger(L, optionIndex);
            return 1;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    // =========================================================================
    // Action & Macro Dispatches
    // =========================================================================

    bool ModManager::OnActionUse(int32_t slot)
    {
        bool consumed = false;
        DispatchHookEx("OnActionUse", [=](lua_State* L) {
            lua_pushinteger(L, slot);
            return 1;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    bool ModManager::OnMacroExecute(const char* name, const char* body)
    {
        bool consumed = false;
        DispatchHookEx("OnMacroExecute", [=](lua_State* L) {
            lua_pushstring(L, name ? name : "");
            lua_pushstring(L, body ? body : "");
            return 2;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    // =========================================================================
    // Cooldown Dispatches
    // =========================================================================

    void ModManager::OnCooldownSet(const char* name, float start, float duration)
    {
        DispatchHookEx("OnCooldownSet", [=](lua_State* L) {
            lua_pushstring(L, name ? name : "");
            lua_pushnumber(L, start);
            lua_pushnumber(L, duration);
            return 3;
        });
    }

    // =========================================================================
    // Sound & Music Dispatches
    // =========================================================================

    bool ModManager::OnPlaySound(int32_t soundKitId)
    {
        bool consumed = false;
        DispatchHookEx("OnPlaySound", [=](lua_State* L) {
            lua_pushinteger(L, soundKitId);
            return 1;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    bool ModManager::OnPlayMusic(const char* musicFile)
    {
        bool consumed = false;
        DispatchHookEx("OnPlayMusic", [=](lua_State* L) {
            lua_pushstring(L, musicFile ? musicFile : "");
            return 1;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    // =========================================================================
    // Item & Container Dispatches
    // =========================================================================

    bool ModManager::OnItemUse(int32_t bagIndex, int32_t slotIndex)
    {
        bool consumed = false;
        DispatchHookEx("OnItemUse", [=](lua_State* L) {
            lua_pushinteger(L, bagIndex);
            lua_pushinteger(L, slotIndex);
            return 2;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    // =========================================================================
    // Trade Dispatches
    // =========================================================================

    void ModManager::OnTradeInitiate(uint64_t targetGuid, int32_t useCursorItem)
    {
        DispatchHookEx("OnTradeInitiate", [=](lua_State* L) {
            lua_pushnumber(L, static_cast<lua_Number>(targetGuid));
            lua_pushboolean(L, useCursorItem ? 1 : 0);
            return 2;
        });
    }

    void ModManager::OnTradeAccept()
    {
        DispatchHook("OnTradeAccept");
    }

    // =========================================================================
    // Combat Feedback / Floating Text Dispatches
    // =========================================================================

    bool ModManager::OnCombatFeedback(uint64_t guid, int32_t amount, uint32_t schoolMask, uint32_t flags)
    {
        bool consumed = false;
        DispatchHookEx("OnCombatFeedback", [=](lua_State* L) {
            lua_pushnumber(L, static_cast<lua_Number>(guid));
            lua_pushinteger(L, amount);
            lua_pushinteger(L, schoolMask);
            lua_pushinteger(L, flags);
            return 4;
        }, [&](lua_State* L, int idx) {
            if (lua_isboolean(L, idx))
                consumed = (lua_toboolean(L, idx) != 0);
        });
        return consumed;
    }

    // =========================================================================
    // Camera Dispatches
    // =========================================================================

    void ModManager::OnCameraUpdate(float x, float y, float z, float fov)
    {
        DispatchHookEx("OnCameraUpdate", [=](lua_State* L) {
            lua_pushnumber(L, x);
            lua_pushnumber(L, y);
            lua_pushnumber(L, z);
            lua_pushnumber(L, fov);
            return 4;
        });
    }

    // =========================================================================
    // Nameplate Dispatches
    // =========================================================================

    void ModManager::OnNamePlateCreate(uint64_t unitGuid, void* namePlatePtr)
    {
        DispatchHookEx("OnNamePlateCreate", [=](lua_State* L) {
            lua_pushnumber(L, static_cast<lua_Number>(unitGuid));
            lua_pushlightuserdata(L, namePlatePtr);
            return 2;
        });
    }
}
