-- ==============================================================================
-- wowdecomp Event & Hook Registration Bus
-- ==============================================================================

local Hooks = {}
Hooks._listeners = {}
Hooks._registeredMetadata = {}

-- Register a listener callback for an engine or gameplay hook
-- priority: higher values execute earlier (default: 100)
function Hooks.Register(hookName, callback, priority, doc)
    if not hookName or type(callback) ~= "function" then
        if ModCore and ModCore.LogError then
            ModCore.LogError("Hooks.Register: Invalid arguments for hook '%s'", tostring(hookName))
        end
        return false
    end

    priority = priority or 100
    if not Hooks._listeners[hookName] then
        Hooks._listeners[hookName] = {}
    end

    table.insert(Hooks._listeners[hookName], {
        fn = callback,
        priority = priority
    })

    -- Sort listeners descending by priority
    table.sort(Hooks._listeners[hookName], function(a, b)
        return a.priority > b.priority
    end)

    if doc and not Hooks._registeredMetadata[hookName] then
        Hooks._registeredMetadata[hookName] = doc
    end
    return true
end

-- Unregister a callback from a hook
function Hooks.Unregister(hookName, callback)
    local list = Hooks._listeners[hookName]
    if not list then return end

    for i = #list, 1, -1 do
        if list[i].fn == callback then
            table.remove(list, i)
        end
    end
end

-- Check if any listener is registered for a hook
function Hooks.HasListeners(hookName)
    local list = Hooks._listeners[hookName]
    return list ~= nil and #list > 0
end

-- Fire an event across all registered listeners
-- Returns the first non-nil value if a listener intercepted/handled it
function Hooks.Fire(hookName, ...)
    local list = Hooks._listeners[hookName]
    if not list or #list == 0 then return nil end

    for _, entry in ipairs(list) do
        local ok, result = pcall(entry.fn, ...)
        if not ok then
            if ModCore and ModCore.LogError then
                ModCore.LogError("Error in hook '%s': %s", hookName, tostring(result))
            else
                print(string.format("[ERROR] Hook '%s' error: %s", hookName, tostring(result)))
            end
        elseif result ~= nil then
            return result -- Early exit / override
        end
    end
    return nil
end

-- Return metadata dictionary of all registered hooks (for AI / IDE documentation exporter)
function Hooks.GetMetadata()
    return Hooks._registeredMetadata
end

-- Return array of all active hook names
function Hooks.GetHookList()
    local names = {}
    for name, _ in pairs(Hooks._listeners) do
        table.insert(names, name)
    end
    table.sort(names)
    return names
end

-- Metatable for syntactic sugar: Hooks.OnWorldUpdate(fn, priority)
setmetatable(Hooks, {
    __index = function(t, key)
        if type(key) == "string" and string.sub(key, 1, 2) == "On" then
            return function(callback, priority, doc)
                return Hooks.Register(key, callback, priority, doc)
            end
        end
        return rawget(t, key)
    end
})

return Hooks
