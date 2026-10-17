-- ==============================================================================
-- wowdecomp API Auto-Export Subsystem (for AI & IDE Tools)
-- ==============================================================================

local ApiExport = {}
ApiExport.Classes = {}
ApiExport.Functions = {}
ApiExport.Hooks = {}
ApiExport.Opcodes = {}

if ModCore then
    ModCore.ApiExport = ApiExport
end

function ApiExport.RegisterClass(className, doc, fields, methods)
    ApiExport.Classes[className] = {
        name = className,
        doc = doc or "",
        fields = fields or {},
        methods = methods or {}
    }
end

function ApiExport.RegisterFunction(funcName, doc, params, returnType)
    ApiExport.Functions[funcName] = {
        name = funcName,
        doc = doc or "",
        params = params or {},
        returnType = returnType or "any"
    }
end

function ApiExport.RegisterHook(hookName, doc, signature, category)
    ApiExport.Hooks[hookName] = {
        name = hookName,
        doc = doc or "",
        signature = signature or "fun(...): any?",
        category = category or "General"
    }
end

function ApiExport.RegisterOpcode(name, opcode, direction, schema)
    ApiExport.Opcodes[name] = {
        name = name,
        opcode = opcode,
        direction = direction or "CLIENT_TO_SERVER",
        schema = schema or {}
    }
end

-- Return structured table for frontend runtime inspection
function ApiExport.GetSchemaTable()
    return {
        version = ModCore and ModCore.Version or "1.1.0",
        classes = ApiExport.Classes,
        functions = ApiExport.Functions,
        hooks = ApiExport.Hooks,
        opcodes = ApiExport.Opcodes
    }
end

-- Generates the EmmyLua / LuaLS .d.lua file string
function ApiExport.GenerateEmmyLua()
    local lines = {}
    table.insert(lines, "---@meta wowdecomp")
    table.insert(lines, "-- Auto-generated API definitions for wowdecomp Universal Modding Framework")
    table.insert(lines, "")

    -- Hook Aliases
    for name, hook in pairs(ApiExport.Hooks) do
        table.insert(lines, string.format("---@alias HookCallback_%s %s", name, hook.signature))
    end
    table.insert(lines, "")

    -- Classes
    for _, cls in pairs(ApiExport.Classes) do
        table.insert(lines, string.format("---@class %s", cls.name))
        for _, f in ipairs(cls.fields) do
            table.insert(lines, string.format("---@field %s %s %s", f.name, f.type or "any", f.doc or ""))
        end
        table.insert(lines, string.format("local %s = {}", cls.name))
        table.insert(lines, "")

        for _, m in ipairs(cls.methods) do
            if m.doc and m.doc ~= "" then
                table.insert(lines, string.format("---%s", m.doc))
            end
            for _, p in ipairs(m.params or {}) do
                table.insert(lines, string.format("---@param %s %s", p.name, p.type or "any"))
            end
            if m.returnType then
                table.insert(lines, string.format("---@return %s", m.returnType))
            end
            local paramNames = {}
            for _, p in ipairs(m.params or {}) do
                table.insert(paramNames, p.name)
            end
            table.insert(lines, string.format("function %s:%s(%s) end", cls.name, m.name, table.concat(paramNames, ", ")))
            table.insert(lines, "")
        end
    end

    -- Global Functions
    for _, fn in pairs(ApiExport.Functions) do
        if fn.doc and fn.doc ~= "" then
            table.insert(lines, string.format("---%s", fn.doc))
        end
        for _, p in ipairs(fn.params or {}) do
            table.insert(lines, string.format("---@param %s %s", p.name, p.type or "any"))
        end
        if fn.returnType then
            table.insert(lines, string.format("---@return %s", fn.returnType))
        end
        local paramNames = {}
        for _, p in ipairs(fn.params or {}) do
            table.insert(paramNames, p.name)
        end
        table.insert(lines, string.format("function %s(%s) end", fn.name, table.concat(paramNames, ", ")))
        table.insert(lines, "")
    end

    return table.concat(lines, "\n")
end

-- ==============================================================================
-- Baseline System Registrations
-- ==============================================================================

ApiExport.RegisterClass("C3Vector", "3D coordinate vector in World space", {
    { name = "x", type = "number", doc = "X coordinate" },
    { name = "y", type = "number", doc = "Y coordinate" },
    { name = "z", type = "number", doc = "Z coordinate" }
})

ApiExport.RegisterClass("WOWGUID", "64-bit entity identifier in World of Warcraft", {
    { name = "low", type = "integer", doc = "Low 32 bits" },
    { name = "high", type = "integer", doc = "High 32 bits" }
}, {
    { name = "IsValid", doc = "Returns true if GUID is non-zero", returnType = "boolean" }
})

-- Lifecycle Hooks
ApiExport.RegisterHook("OnEngineInit", "Fired when client engine completes initialization", "fun()", "Lifecycle")
ApiExport.RegisterHook("OnEngineShutdown", "Fired when client engine begins shutdown", "fun()", "Lifecycle")
ApiExport.RegisterHook("OnGameInitialize", "Fired when game world session initializes", "fun()", "Lifecycle")
ApiExport.RegisterHook("OnGameDestroy", "Fired when game world session shuts down", "fun()", "Lifecycle")
ApiExport.RegisterHook("OnFrameTick", "Fired every engine loop tick", "fun(dt: number)", "Lifecycle")

-- World & Map Hooks
ApiExport.RegisterHook("OnEnterWorld", "Fired when player enters 3D world", "fun()", "World")
ApiExport.RegisterHook("OnLeaveWorld", "Fired when player leaves 3D world", "fun()", "World")
ApiExport.RegisterHook("OnMapChange", "Fired when active map changes", "fun(mapName: string, zoneId: integer)", "World")
ApiExport.RegisterHook("OnWorldUpdate", "Fired every game update tick", "fun(dt: number)", "World")
ApiExport.RegisterHook("OnWorldRender", "Fired during 3D world rendering pass", "fun(worldFrame: any)", "World")
ApiExport.RegisterHook("OnWorldHitTest", "Fired on raycast entity/surface hit", "fun(guid: number, point: any, hitObject: boolean)", "World")
ApiExport.RegisterHook("OnTerrainTrack", "Fired during terrain tracking query", "fun(point: any): boolean?", "World")
ApiExport.RegisterHook("OnTerrainClick", "Fired when clicking terrain ground", "fun(button: integer, x: number, y: number, z: number): boolean?", "World")

-- Glue / UI Hooks
ApiExport.RegisterHook("OnGlueResume", "Fired when Glue screen resumes", "fun()", "Glue")
ApiExport.RegisterHook("OnGlueIdle", "Fired each frame on Glue screen", "fun()", "Glue")
ApiExport.RegisterHook("OnGlueScreenChange", "Fired on Glue screen transition", "fun(screenName: string)", "Glue")
ApiExport.RegisterHook("IsLoginBlocked", "Queried before allowing realm login", "fun(): boolean?", "Glue")
ApiExport.RegisterHook("OnCharacterSelect", "Fired when character is selected", "fun(charIndex: integer)", "Glue")
ApiExport.RegisterHook("OnCharacterCreate", "Fired on character creation submission", "fun(name: string, race: integer, classId: integer, gender: integer)", "Glue")

-- Network Hooks
ApiExport.RegisterHook("OnPacketReceived", "Fired when custom network packet arrives", "fun(opcode: integer, time: integer, reader: any): boolean?", "Network")
ApiExport.RegisterHook("OnPacketSend", "Fired before sending network packet", "fun(opcode: integer, size: integer, reader: any): boolean?", "Network")
ApiExport.RegisterHook("OnDisconnect", "Fired when disconnected from network", "fun()", "Network")

-- Spell Hooks
ApiExport.RegisterHook("OnSpellQuery", "Fired on dynamic spell attribute query", "fun(queryType: string, spellId: integer, ...): any?", "Spells")
ApiExport.RegisterHook("OnSpellCast", "Fired when spell cast starts", "fun(spellId: integer, casterGuid: number, targetGuid: number)", "Spells")
ApiExport.RegisterHook("OnChannelStart", "Fired when channeling starts", "fun(msg: any)", "Spells")
ApiExport.RegisterHook("OnChannelUpdate", "Fired on channeling update", "fun(msg: any)", "Spells")
ApiExport.RegisterHook("OnChannelStop", "Fired when channeling stops", "fun()", "Spells")
ApiExport.RegisterHook("OnSpellDestroy", "Fired when spell instance is destroyed", "fun()", "Spells")

-- Tooltip Hooks
ApiExport.RegisterHook("OnTooltipEvaluate", "Fired when evaluating dynamic tooltip text", "fun(spellId: integer, caster: any, defaultText: string): string?", "Tooltips")
ApiExport.RegisterHook("OnTooltipSetSpell", "Fired when setting spell tooltip", "fun(tooltip: any, spellId: integer)", "Tooltips")
ApiExport.RegisterHook("OnTooltipSetItem", "Fired when setting item tooltip", "fun(tooltip: any, itemId: integer)", "Tooltips")
ApiExport.RegisterHook("OnTooltipSetUnit", "Fired when setting unit tooltip", "fun(tooltip: any, unitGuid: number)", "Tooltips")

-- Object Management Hooks
ApiExport.RegisterHook("OnObjectCreate", "Fired when entity spawns", "fun(guid: number, typeId: integer, objectPtr: any)", "Objects")
ApiExport.RegisterHook("OnObjectDestroy", "Fired when entity despawns", "fun(guid: number, typeId: integer, objectPtr: any)", "Objects")
ApiExport.RegisterHook("OnTargetChanged", "Fired when player target changes", "fun(oldGuid: number, newGuid: number)", "Objects")

-- Input Hooks
ApiExport.RegisterHook("OnKeyDown", "Fired on key press", "fun(key: integer, modifiers: integer, keyString: string): boolean?", "Input")
ApiExport.RegisterHook("OnKeyUp", "Fired on key release", "fun(key: integer, modifiers: integer, keyString: string): boolean?", "Input")
ApiExport.RegisterHook("OnMouseDown", "Fired on mouse press", "fun(button: integer, x: integer, y: integer): boolean?", "Input")
ApiExport.RegisterHook("OnMouseUp", "Fired on mouse release", "fun(button: integer, x: integer, y: integer): boolean?", "Input")
ApiExport.RegisterHook("OnMouseWheel", "Fired on mouse wheel scroll", "fun(delta: integer): boolean?", "Input")

-- Chat Hooks
ApiExport.RegisterHook("OnChatMessage", "Fired on incoming chat message", "fun(type: integer, sender: string, message: string, channel: string): boolean?", "Chat")
ApiExport.RegisterHook("OnSendChatMessage", "Fired on outgoing chat message", "fun(message: string, type: integer, language: integer, target: string): boolean?", "Chat")

-- Environment & Weather Hooks
ApiExport.RegisterHook("OnWeatherChange", "Fired when weather type or intensity changes in active zone", "fun(type: integer, intensity: number)", "Environment")
ApiExport.RegisterHook("OnZoneChange", "Fired when player crosses zone or subzone boundary", "fun(areaId: integer, subzoneId: integer, zoneName: string)", "World")

-- Loot Hooks
ApiExport.RegisterHook("OnLootOpen", "Fired when loot window opens for an entity or container", "fun(guid: number, coins: integer, lootType: integer)", "Loot")
ApiExport.RegisterHook("OnLootSlot", "Fired when an item slot is looted from the active loot window", "fun(slot: integer)", "Loot")
ApiExport.RegisterHook("OnLootClose", "Fired when active loot window is closed", "fun()", "Loot")

-- Merchant Hooks
ApiExport.RegisterHook("OnMerchantBuy", "Fired when buying an item from an active merchant", "fun(index: integer, count: integer): boolean?", "Merchant")

-- Quest & Gossip Hooks
ApiExport.RegisterHook("OnQuestAccept", "Fired when accepting a quest from a quest giver", "fun(questGiverGuid: number): boolean?", "Quests")
ApiExport.RegisterHook("OnQuestComplete", "Fired when completing a quest with a quest giver", "fun(questGiverGuid: number): boolean?", "Quests")
ApiExport.RegisterHook("OnQuestReward", "Fired when selecting and claiming a quest reward", "fun(rewardIndex: integer): boolean?", "Quests")
ApiExport.RegisterHook("OnGossipSelect", "Fired when selecting an option in NPC gossip dialog", "fun(optionIndex: integer): boolean?", "Quests")

-- Action & Macro Hooks
ApiExport.RegisterHook("OnActionUse", "Fired when an action bar button is activated", "fun(slot: integer): boolean?", "ActionBar")
ApiExport.RegisterHook("OnMacroExecute", "Fired before executing macro commands or text", "fun(name: string, body: string): boolean?", "ActionBar")

-- Cooldown Hooks
ApiExport.RegisterHook("OnCooldownSet", "Fired when a spell or item cooldown is initialized or refreshed", "fun(name: string, start: number, duration: number)", "Cooldowns")

-- Sound & Music Hooks
ApiExport.RegisterHook("OnPlaySound", "Fired when a sound or sound kit is requested to play", "fun(soundKitId: integer): boolean?", "Sound")
ApiExport.RegisterHook("OnPlayMusic", "Fired when a music track or file is requested to play", "fun(musicFile: string): boolean?", "Sound")

-- Item Hooks
ApiExport.RegisterHook("OnItemUse", "Fired when an item in a container or bag is used or activated", "fun(bagIndex: integer, slotIndex: integer): boolean?", "Items")

-- Trade Hooks
ApiExport.RegisterHook("OnTradeInitiate", "Fired when a player-to-player trade session is initiated", "fun(targetGuid: number, useCursorItem: boolean)", "Trade")
ApiExport.RegisterHook("OnTradeAccept", "Fired when player clicks Accept in the trade window", "fun()", "Trade")

-- Combat Feedback Hooks
ApiExport.RegisterHook("OnCombatFeedback", "Fired on combat feedback / floating combat text events", "fun(guid: number, amount: integer, schoolMask: integer, flags: integer): boolean?", "Combat")

-- Camera Hooks
ApiExport.RegisterHook("OnCameraUpdate", "Fired every frame when active world camera updates position and FOV", "fun(x: number, y: number, z: number, fov: number)", "Camera")

-- UI & Nameplate Hooks
ApiExport.RegisterHook("OnNamePlateCreate", "Fired when a nameplate frame is created or initialized for a unit", "fun(unitGuid: number, namePlatePtr: any)", "UI")

return ApiExport
