-- ==============================================================================
-- wowdecomp Declarative Network & Packet RPC Engine
-- ==============================================================================

-- Ensure compatibility alias for GlueXML / legacy addons
if CreateCustomPacket and not CreateCustompacket then
    CreateCustompacket = CreateCustomPacket
end

local Net = {}
local packetDefs = {}
local opcodeMap = {}

-- Define a declarative packet schema
function Net.DefinePacket(name, def)
    if not def or not def.opcode then
        ModCore.LogError("Cannot define packet '%s': missing opcode", name)
        return
    end

    def.name = name
    packetDefs[name] = def
    opcodeMap[def.opcode] = def

    -- If this is a server-to-client packet with a handler, wire it to raw opcode receiver
    if def.direction == "SERVER_TO_CLIENT" or def.direction == "BIDIRECTIONAL" then
        if def.handler then
            Net.RegisterSchemaHandler(def.opcode, def)
        end
    end

    if ModCore and ModCore.ApiExport then
        ModCore.ApiExport.RegisterOpcode(name, def.opcode, def.direction, def.schema)
    end
end

-- Wire binary unpacking to registered schema
function Net.RegisterSchemaHandler(opcode, def)
    local rawCallback = function(reader)
        local data = {}
        for _, field in ipairs(def.schema or {}) do
            local fType = field.type
            if fType == "uint8" then
                data[field.name] = reader:ReadUInt8()
            elseif fType == "int8" then
                data[field.name] = reader:ReadInt8()
            elseif fType == "uint16" then
                data[field.name] = reader:ReadUInt16()
            elseif fType == "int16" then
                data[field.name] = reader:ReadInt16()
            elseif fType == "uint32" or fType == "integer" then
                data[field.name] = reader:ReadUInt32()
            elseif fType == "int32" then
                data[field.name] = reader:ReadInt32()
            elseif fType == "float" then
                data[field.name] = reader:ReadFloat()
            elseif fType == "double" then
                data[field.name] = reader:ReadDouble()
            elseif fType == "string" then
                data[field.name] = reader:ReadString()
            elseif fType == "guid" then
                local low = reader:ReadUInt32()
                local high = reader:ReadUInt32()
                data[field.name] = { low = low, high = high }
            elseif fType == "vector3" then
                data[field.name] = {
                    x = reader:ReadFloat(),
                    y = reader:ReadFloat(),
                    z = reader:ReadFloat()
                }
            elseif fType == "quat" then
                data[field.name] = {
                    w = reader:ReadFloat(),
                    x = reader:ReadFloat(),
                    y = reader:ReadFloat(),
                    z = reader:ReadFloat()
                }
            end
        end

        local ok, err = pcall(def.handler, data)
        if not ok then
            ModCore.LogError("Error in packet handler '%s' (0x%X): %s", def.name, opcode, tostring(err))
        end
    end

    if OnCustomPacket then
        OnCustomPacket(opcode, rawCallback)
    end
end

-- Send a custom packet matching its declarative schema
function Net.Send(name, payload)
    local def = packetDefs[name]
    if not def then
        ModCore.LogError("Net.Send: Unknown packet '%s'", name)
        return false
    end

    if not CreateCustomPacket then
        ModCore.LogError("Net.Send: CreateCustomPacket primitive is unavailable")
        return false
    end

    local writer = CreateCustomPacket(def.opcode, 256)
    for _, field in ipairs(def.schema or {}) do
        local val = payload[field.name]
        local fType = field.type

        if fType == "uint8" then
            writer:WriteUInt8(val or 0)
        elseif fType == "int8" then
            writer:WriteInt8(val or 0)
        elseif fType == "uint16" then
            writer:WriteUInt16(val or 0)
        elseif fType == "int16" then
            writer:WriteInt16(val or 0)
        elseif fType == "uint32" or fType == "integer" then
            writer:WriteUInt32(val or 0)
        elseif fType == "int32" then
            writer:WriteInt32(val or 0)
        elseif fType == "float" then
            writer:WriteFloat(val or 0.0)
        elseif fType == "double" then
            writer:WriteDouble(val or 0.0)
        elseif fType == "string" then
            writer:WriteString(val or "")
        elseif fType == "vector3" then
            writer:WriteFloat((val and val.x) or 0.0)
            writer:WriteFloat((val and val.y) or 0.0)
            writer:WriteFloat((val and val.z) or 0.0)
        elseif fType == "quat" then
            writer:WriteFloat((val and val.w) or 1.0)
            writer:WriteFloat((val and val.x) or 0.0)
            writer:WriteFloat((val and val.y) or 0.0)
            writer:WriteFloat((val and val.z) or 0.0)
        end
    end

    writer:Send()
    return true
end

-- Listen for raw custom opcodes without a schema
function Net.OnCustomPacket(opcode, handler)
    if OnCustomPacket then
        OnCustomPacket(opcode, handler)
    end
end

return Net
