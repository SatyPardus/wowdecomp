-- ==============================================================================
-- wowdecomp DBC Virtualization & Transformation Engine
-- ==============================================================================

local DBC = {}
local tableSchemas = {}
local tableRows = {}
local virtualRows = {}
local rowOverrides = {}
local lookupHooks = {}

-- Register a custom DBC table schema and optional initial rows
function DBC.RegisterTable(tableName, schema, initialRows)
    if not tableName then return end
    tableSchemas[tableName] = schema or {}
    tableRows[tableName] = tableRows[tableName] or {}
    if initialRows then
        for id, row in pairs(initialRows) do
            DBC.DefineRow(tableName, id, row)
        end
    end
    if ModCore and ModCore.LogDebug then
        ModCore.LogDebug("DBC: Registered table %s with %d columns", tableName, schema and #schema or 0)
    end
end

-- Define or insert a row into a DBC table
function DBC.DefineRow(tableName, id, rowData)
    if not tableName or not id or not rowData then return end
    tableRows[tableName] = tableRows[tableName] or {}
    rowData.id = id
    tableRows[tableName][id] = rowData
end

-- Inject a new virtual row into a DBC table (runtime extension)
function DBC.RegisterVirtualRow(tableName, id, rowData)
    if not virtualRows[tableName] then
        virtualRows[tableName] = {}
    end
    rowData.id = id
    virtualRows[tableName][id] = rowData
    if ModCore and ModCore.LogDebug then
        ModCore.LogDebug("DBC: Registered virtual row in %s [ID: %d]", tableName, id)
    end
end

-- Override specific fields on an existing DBC row
function DBC.OverrideRow(tableName, id, fieldOverrides)
    if not rowOverrides[tableName] then
        rowOverrides[tableName] = {}
    end
    rowOverrides[tableName][id] = fieldOverrides
end

-- Hook dynamic row lookups
function DBC.OnRowLookup(tableName, callback)
    if not lookupHooks[tableName] then
        lookupHooks[tableName] = {}
    end
    table.insert(lookupHooks[tableName], callback)
end

-- Retrieve a DBC row table by table name and ID
function DBC.GetRow(tableName, id)
    if not tableName or not id then return nil end
    id = tonumber(id) or id

    -- 1. Check custom lookup hooks
    if lookupHooks[tableName] then
        for _, hook in ipairs(lookupHooks[tableName]) do
            local res = hook(id)
            if res ~= nil then return res end
        end
    end

    -- 2. Check virtual rows added by mods
    if virtualRows[tableName] and virtualRows[tableName][id] then
        return virtualRows[tableName][id]
    end

    -- 3. Check registered Lua DBC table rows
    local row = nil
    if tableRows[tableName] and tableRows[tableName][id] then
        row = tableRows[tableName][id]
    end

    -- 4. Apply row overrides if any
    if row and rowOverrides[tableName] and rowOverrides[tableName][id] then
        local copy = {}
        for k, v in pairs(row) do copy[k] = v end
        for k, v in pairs(rowOverrides[tableName][id]) do copy[k] = v end
        return copy
    end

    return row
end

-- Retrieve a row unpacked as multiple return values matching C++ luaGet calling conventions
function DBC.GetRowUnpacked(tableName, id)
    local row = DBC.GetRow(tableName, id)
    if not row then return nil end

    local schema = tableSchemas[tableName]
    if schema and #schema > 0 then
        local values = {}
        for i, colName in ipairs(schema) do
            values[i] = row[colName]
        end
        return unpack(values)
    end

    -- If no schema, check if row is array-indexed
    if row[1] ~= nil then
        return unpack(row)
    end

    return row
end

-- Get all rows in a DBC table
function DBC.GetRows(tableName)
    return tableRows[tableName] or {}
end

-- Get total number of rows in a DBC table
function DBC.GetNumRows(tableName)
    local count = 0
    if tableRows[tableName] then
        for _ in pairs(tableRows[tableName]) do
            count = count + 1
        end
    end
    return count
end

-- Export global helpers for backward compatibility with legacy scripts
_G.DBC = DBC
_G._CDBC_GET = DBC.GetRowUnpacked
_G.GetCDBCRow = DBC.GetRowUnpacked

-- Hook into ModCore API registration
if ModCore and ModCore.ApiExport then
    ModCore.ApiExport.RegisterClass("DbcEngine", "Virtual DBC tables and row injection", {}, {
        { name = "RegisterTable", params = {{name="tableName", type="string"}, {name="schema", type="table"}, {name="initialRows", type="table?"}}, doc = "Registers a DBC table with column schema and initial rows" },
        { name = "DefineRow", params = {{name="tableName", type="string"}, {name="id", type="integer"}, {name="rowData", type="table"}}, doc = "Defines a row in a DBC table" },
        { name = "GetRow", params = {{name="tableName", type="string"}, {name="id", type="integer"}}, returnType = "table?", doc = "Retrieves a DBC row by table and ID" },
        { name = "GetRowUnpacked", params = {{name="tableName", type="string"}, {name="id", type="integer"}}, doc = "Retrieves a DBC row unpacked as multiple values" },
        { name = "RegisterVirtualRow", params = {{name="tableName", type="string"}, {name="id", type="integer"}, {name="rowData", type="table"}}, doc = "Injects a virtual row into client memory" },
        { name = "OverrideRow", params = {{name="tableName", type="string"}, {name="id", type="integer"}, {name="fieldOverrides", type="table"}}, doc = "Overrides specific fields on a DBC row" }
    })
end

return DBC
