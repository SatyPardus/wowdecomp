-- ==============================================================================
-- wowdecomp Modding Framework Bootstrap
-- ==============================================================================

ModCore = ModCore or {}
ModCore.Version = "1.1.0"
ModCore.LoadedMods = {}

local function Log(level, fmt, ...)
    local ok, msg = pcall(string.format, fmt, ...)
    if not ok then msg = tostring(fmt) end
    if _MODCORE_LOG then
        _MODCORE_LOG(level, msg)
    else
        print(string.format("[%s] [ModCore] %s", level, msg))
    end
end

ModCore.LogDebug = function(...) Log("DEBUG", ...) end
ModCore.LogInfo  = function(...) Log("INFO", ...) end
ModCore.LogWarn  = function(...) Log("WARN", ...) end
ModCore.LogError = function(...) Log("ERROR", ...) end

ModCore.LogInfo("Initializing wowdecomp Universal Modding Framework v%s", ModCore.Version)

-- Setup package table and safe require() for FrameScript
local basePath = ModCore.BasePath or "mods/"

package = package or {}
package.loaded = package.loaded or {}
package.path = basePath .. "?.lua;" ..
               basePath .. "?/init.lua;" ..
               basePath .. "system/?.lua;" ..
               basePath .. "user_mods/?.lua;" ..
               (package.path or "")

if not require then
    function require(modname)
        if package.loaded[modname] ~= nil then
            return package.loaded[modname]
        end

        local rel = string.gsub(modname, "%.", "/")
        local tried = {}
        for pattern in string.gmatch(package.path, "[^;]+") do
            local filename = string.gsub(pattern, "%?", rel)
            local fn, err = loadfile(filename)
            if fn then
                local res = fn()
                if res == nil then res = true end
                package.loaded[modname] = res
                return res
            else
                table.insert(tried, string.format("no file '%s'", filename))
            end
        end

        error(string.format("module '%s' not found:\n\t%s", modname, table.concat(tried, "\n\t")), 2)
    end
end

-- Load Event & Hook Bus
ModCore.Hooks = require("system.hooks")

-- Load Declarative Network Engine
ModCore.Net = require("system.net")

-- Load Auto-Export Engine (used for AI schema & EmmyLua generator)
ModCore.ApiExport = require("system.api_export")

-- Load Virtual DBC Table Engine
ModCore.DBC = require("system.dbc")

-- ==============================================================================
-- Unified Hook Dispatch & Frontend Convenience APIs
-- ==============================================================================

-- Direct hook dispatch invoked by C++ engine
function ModCore.DispatchHook(hookName, ...)
    return ModCore.Hooks.Fire(hookName, ...)
end

-- Frontend sugar: ModCore.On("OnWorldUpdate", callback, priority)
function ModCore.On(hookName, callback, priority, doc)
    return ModCore.Hooks.Register(hookName, callback, priority, doc)
end

-- Frontend aliases
ModCore.RegisterHook   = ModCore.Hooks.Register
ModCore.UnregisterHook = ModCore.Hooks.Unregister
ModCore.FireHook       = ModCore.Hooks.Fire
ModCore.HasListeners   = ModCore.Hooks.HasListeners

-- Engine Primitives & World Interaction
ModCore.SetCastAtCursor = function(enable)
    if _ModCore_SetCastAtCursor then _ModCore_SetCastAtCursor(enable) end
end

ModCore.IsCastAtCursor = function()
    if _ModCore_IsCastAtCursor then return _ModCore_IsCastAtCursor() end
    return false
end

ModCore.ClickTerrain = function(x, y, z)
    if _ModCore_ClickTerrain then return _ModCore_ClickTerrain(x, y, z) end
end

ModCore.ClickSelfTerrain = function()
    if _ModCore_ClickTerrain then return _ModCore_ClickTerrain() end
end

ModCore.GetLastHitGuid = function()
    if _ModCore_GetLastHitGuid then return _ModCore_GetLastHitGuid() end
    return "0", 0
end

ModCore.GetLastHitPoint = function()
    if _ModCore_GetLastHitPoint then return _ModCore_GetLastHitPoint() end
    return 0.0, 0.0, 0.0
end

-- Returns array of all active hooks
function ModCore.GetHookList()
    return ModCore.Hooks.GetHookList()
end

-- Returns machine-readable API schema for frontend tools and UI extensions
function ModCore.GetApiSchema()
    if ModCore.ApiExport and ModCore.ApiExport.GetSchemaTable then
        return ModCore.ApiExport.GetSchemaTable()
    end
    return {
        version = ModCore.Version,
        hooks = ModCore.Hooks.GetHookList()
    }
end

-- Triggers runtime re-export of all IDE definitions, JSON schema, and AI guides
function ModCore.ExportApi()
    if ModCore_ExportApi then
        return ModCore_ExportApi()
    elseif ModCore.ApiExport and ModCore.ApiExport.ExportAll then
        return ModCore.ApiExport.ExportAll()
    end
    return false
end

-- Metatable routing: ModCore.OnEvent(...) dynamically dispatches to Hooks.Fire("OnEvent", ...)
local modCoreMt = {
    __index = function(t, key)
        if type(key) == "string" and string.sub(key, 1, 2) == "On" then
            return function(...)
                return ModCore.Hooks.Fire(key, ...)
            end
        end
        return rawget(t, key)
    end
}
setmetatable(ModCore, modCoreMt)

-- ==============================================================================
-- Mod Mounting & Discovery
-- ==============================================================================

function ModCore.MountMod(modDir, manifest)
    if not manifest or not manifest.id then
        ModCore.LogError("Cannot mount mod at %s: missing manifest.id", modDir)
        return false
    end

    if ModCore.LoadedMods[manifest.id] then
        ModCore.LogWarn("Mod %s already mounted, skipping duplicate", manifest.id)
        return false
    end

    ModCore.LogInfo("Mounting Mod: %s (%s) v%s by %s", manifest.name or manifest.id, manifest.id, manifest.version or "1.0", manifest.author or "Unknown")

    local modContext = {
        id = manifest.id,
        dir = modDir,
        manifest = manifest,
        state = {}
    }

    -- Execute entry point if present
    local entryPoint = manifest.entryPoint or manifest.entryPoints and manifest.entryPoints.onEngineInit or "init.lua"
    local entryFullPath = modDir .. "/" .. entryPoint
    local initFunc, err = loadfile(entryFullPath)
    if initFunc then
        local success, runErr = pcall(initFunc, modContext)
        if not success then
            ModCore.LogError("Error executing entrypoint %s: %s", entryFullPath, tostring(runErr))
        end
    else
        if manifest.entryPoint then
            ModCore.LogError("Failed to load mod entrypoint %s: %s", entryFullPath, tostring(err))
        end
    end

    ModCore.LoadedMods[manifest.id] = modContext
    ModCore.Hooks.Fire("OnModMounted", manifest.id, modContext)
    return true
end

ModCore.LogInfo("Bootstrap completed successfully. Hook bus ready.")
