#include "CustomPacket.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Net/NetClient/Types.h"

#include <sstream>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace ModCore
{
    // =========================================================================
    // CustomPacketWriter
    // =========================================================================

    CustomPacketWriter::CustomPacketWriter(uint16_t opcode, size_t initialCapacity)
        : m_opcode(opcode)
    {
        if (initialCapacity > 0)
        {
            m_buffer.reserve(initialCapacity);
        }
    }

    void CustomPacketWriter::WriteBytes(const void* data, size_t size)
    {
        if (!data || size == 0) return;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
        m_buffer.insert(m_buffer.end(), bytes, bytes + size);
    }

    void CustomPacketWriter::WriteString(const char* str, size_t len)
    {
        uint32_t length = static_cast<uint32_t>(len);
        Write(length);
        if (len > 0 && str)
        {
            WriteBytes(str, len);
        }
    }

    void CustomPacketWriter::WriteString(const std::string& str)
    {
        WriteString(str.data(), str.size());
    }

    void CustomPacketWriter::Send()
    {
        ClientConnection* conn = ClientServices::Connection();
        if (!conn || conn->m_netState != NS_CONNECTED)
        {
            return;
        }

        if (m_buffer.empty())
        {
            CDataStore p;
            p.Put(static_cast<uint32_t>(CLIENT_TO_SERVER_OPCODE));
            CustomPacketHeader hdr{ 0, 1, m_opcode };
            p.PutData(reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr));
            p.Finalize();
            ClientServices::SendPacket(&p);
            return;
        }

        uint16_t totalFrags = static_cast<uint16_t>((m_buffer.size() + MAX_FRAGMENT_SIZE - 1) / MAX_FRAGMENT_SIZE);
        for (uint16_t frag = 0; frag < totalFrags; ++frag)
        {
            size_t offset = frag * MAX_FRAGMENT_SIZE;
            size_t chunkSize = std::min(MAX_FRAGMENT_SIZE, m_buffer.size() - offset);

            CDataStore p;
            p.Put(static_cast<uint32_t>(CLIENT_TO_SERVER_OPCODE));
            CustomPacketHeader hdr{ frag, totalFrags, m_opcode };
            p.PutData(reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr));
            if (chunkSize > 0)
            {
                p.PutData(m_buffer.data() + offset, static_cast<uint32_t>(chunkSize));
            }
            p.Finalize();
            ClientServices::SendPacket(&p);
        }
    }

    // =========================================================================
    // CustomPacketReader
    // =========================================================================

    CustomPacketReader::CustomPacketReader(uint16_t opcode, const uint8_t* data, size_t size)
        : m_opcode(opcode), m_readPos(0)
    {
        if (data && size > 0)
        {
            m_buffer.assign(data, data + size);
        }
    }

    void CustomPacketReader::ReadBytes(void* dest, size_t size)
    {
        if (!dest || size == 0) return;
        if (m_readPos + size <= m_buffer.size())
        {
            memcpy(dest, m_buffer.data() + m_readPos, size);
            m_readPos += size;
        }
        else
        {
            size_t available = m_buffer.size() > m_readPos ? m_buffer.size() - m_readPos : 0;
            if (available > 0)
            {
                memcpy(dest, m_buffer.data() + m_readPos, available);
                m_readPos += available;
            }
            memset(reinterpret_cast<uint8_t*>(dest) + available, 0, size - available);
        }
    }

    std::string CustomPacketReader::ReadString(const std::string& def)
    {
        uint32_t len = Read<uint32_t>(UINT32_MAX);
        if (len == UINT32_MAX)
            return def;
        if (len == 0)
            return "";
        if (m_readPos + len > m_buffer.size())
            return def;
        std::string str(reinterpret_cast<const char*>(m_buffer.data() + m_readPos), len);
        m_readPos += len;
        return str;
    }

    // =========================================================================
    // CustomPacketManager
    // =========================================================================

    CustomPacketManager& CustomPacketManager::Get()
    {
        static CustomPacketManager instance;
        return instance;
    }

    CustomPacketWriter* CustomPacketManager::GetWriter(uint32_t id)
    {
        auto itr = m_writers.find(id);
        if (itr != m_writers.end())
            return &itr->second;
        return nullptr;
    }

    uint32_t CustomPacketManager::CreateWriter(uint16_t opcode, size_t capacity)
    {
        uint32_t id = m_nextWriterId++;
        m_writers.emplace(id, CustomPacketWriter(opcode, capacity));
        return id;
    }

    void CustomPacketManager::RemoveWriter(uint32_t id)
    {
        m_writers.erase(id);
    }

    void CustomPacketManager::RegisterHandler(uint16_t opcode, std::function<void(CustomPacketReader&)> handler)
    {
        m_cppHandlers[opcode].push_back(std::move(handler));
    }

    bool CustomPacketManager::HandlePacket(uint32_t opcode, uint32_t time, void* dataStore)
    {
        if (opcode != SERVER_TO_CLIENT_OPCODE && opcode != ALT_SERVER_OPCODE)
        {
            return false;
        }

        if (!dataStore)
            return true;

        CDataStore* msg = reinterpret_cast<CDataStore*>(dataStore);
        if (!msg || msg->m_size <= msg->m_read)
            return true;

        uint32_t payloadSize = msg->m_size - msg->m_read;
        const uint8_t* payloadData = reinterpret_cast<const uint8_t*>(msg->m_data + msg->m_read);

        if (payloadSize < sizeof(CustomPacketHeader))
            return true;

        CustomPacketHeader hdr;
        memcpy(&hdr, payloadData, sizeof(hdr));

        const uint8_t* chunkData = payloadData + sizeof(hdr);
        size_t chunkSize = payloadSize - sizeof(hdr);

        if (hdr.totalFrags <= 1)
        {
            CustomPacketReader reader(hdr.opcode, chunkData, chunkSize);
            m_curRead = &reader;

            char lua[64];
            snprintf(lua, sizeof(lua), "__FireCustomPacket(%u)", static_cast<unsigned int>(hdr.opcode));
            FrameScript_Execute(lua, "ModCoreNetwork", nullptr);

            auto itr = m_cppHandlers.find(hdr.opcode);
            if (itr != m_cppHandlers.end())
            {
                for (const auto& cb : itr->second)
                {
                    reader.Reset();
                    cb(reader);
                }
            }

            m_curRead = nullptr;
        }
        else
        {
            if (hdr.fragmentId == 0)
            {
                m_reassemblyBuffer.clear();
                m_pendingOpcode = hdr.opcode;
                m_pendingTotalFrags = hdr.totalFrags;
            }

            if (hdr.totalFrags == m_pendingTotalFrags && hdr.opcode == m_pendingOpcode)
            {
                m_reassemblyBuffer.insert(m_reassemblyBuffer.end(), chunkData, chunkData + chunkSize);

                if (hdr.fragmentId == hdr.totalFrags - 1)
                {
                    CustomPacketReader reader(m_pendingOpcode, m_reassemblyBuffer.data(), m_reassemblyBuffer.size());
                    m_curRead = &reader;

                    char lua[64];
                    snprintf(lua, sizeof(lua), "__FireCustomPacket(%u)", static_cast<unsigned int>(m_pendingOpcode));
                    FrameScript_Execute(lua, "ModCoreNetwork", nullptr);

                    auto itr = m_cppHandlers.find(m_pendingOpcode);
                    if (itr != m_cppHandlers.end())
                    {
                        for (const auto& cb : itr->second)
                        {
                            reader.Reset();
                            cb(reader);
                        }
                    }

                    m_curRead = nullptr;
                    m_reassemblyBuffer.clear();
                }
            }
        }

        return true;
    }

    // =========================================================================
    // Lua Native Dispatch and Scripts
    // =========================================================================

    namespace
    {
        enum class LuaNetworkOpcode
        {
            WRITE_SIZE   = 0,
            READ_SIZE    = 1,
            WRITE_UINT8  = 2,
            WRITE_INT8   = 3,
            WRITE_UINT16 = 4,
            WRITE_INT16  = 5,
            WRITE_UINT32 = 6,
            WRITE_INT32  = 7,
            WRITE_UINT64 = 8,
            WRITE_INT64  = 9,
            WRITE_FLOAT  = 10,
            WRITE_DOUBLE = 11,
            WRITE_STRING = 12,

            READ_UINT8          = 13,
            READ_INT8           = 14,
            READ_UINT16         = 15,
            READ_INT16          = 16,
            READ_UINT32         = 17,
            READ_INT32          = 18,
            READ_UINT64         = 19,
            READ_INT64          = 20,
            READ_FLOAT          = 21,
            READ_DOUBLE         = 22,
            READ_STRING         = 23,
            MAKE_CUSTOM_PACKET  = 24,
            SEND_CUSTOM_PACKET  = 25,
            RESET_CUSTOM_PACKET = 26,
        };

        int Script_LuaLog(lua_State* L)
        {
            int level = static_cast<int>(lua_tonumber(L, 1));
            const char* msg = lua_tolstring(L, 2, nullptr);
            if (!msg) return 0;

            const char* prefix = "[LUA]";
            switch (level)
            {
                case 0: prefix = "[LUA DEBUG]"; break;
                case 1: prefix = "[LUA INFO]"; break;
                case 2: prefix = "[LUA WARN]"; break;
                case 3: prefix = "[LUA ERROR]"; break;
                default: break;
            }
            std::cout << prefix << " " << msg << std::endl;
            return 0;
        }

        int Script_CLIENT_NETWORK(lua_State* L)
        {
            auto& mgr = CustomPacketManager::Get();
            auto opcode = static_cast<LuaNetworkOpcode>(static_cast<int>(lua_tonumber(L, 1)));
            switch (opcode)
            {
                case LuaNetworkOpcode::WRITE_SIZE:
                {
                    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 2));
                    auto* w = mgr.GetWriter(id);
                    lua_pushnumber(L, w ? static_cast<lua_Number>(w->Size()) : 0.0);
                    return 1;
                }
                case LuaNetworkOpcode::READ_SIZE:
                {
                    auto* r = mgr.GetCurrentReader();
                    lua_pushnumber(L, r ? static_cast<lua_Number>(r->Size()) : 0.0);
                    return 1;
                }
                case LuaNetworkOpcode::WRITE_UINT8:
                {
                    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 2));
                    if (auto* w = mgr.GetWriter(id))
                        w->Write<uint8_t>(static_cast<uint8_t>(lua_tonumber(L, 3)));
                    return 0;
                }
                case LuaNetworkOpcode::WRITE_INT8:
                {
                    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 2));
                    if (auto* w = mgr.GetWriter(id))
                        w->Write<int8_t>(static_cast<int8_t>(lua_tonumber(L, 3)));
                    return 0;
                }
                case LuaNetworkOpcode::WRITE_UINT16:
                {
                    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 2));
                    if (auto* w = mgr.GetWriter(id))
                        w->Write<uint16_t>(static_cast<uint16_t>(lua_tonumber(L, 3)));
                    return 0;
                }
                case LuaNetworkOpcode::WRITE_INT16:
                {
                    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 2));
                    if (auto* w = mgr.GetWriter(id))
                        w->Write<int16_t>(static_cast<int16_t>(lua_tonumber(L, 3)));
                    return 0;
                }
                case LuaNetworkOpcode::WRITE_UINT32:
                {
                    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 2));
                    if (auto* w = mgr.GetWriter(id))
                        w->Write<uint32_t>(static_cast<uint32_t>(lua_tonumber(L, 3)));
                    return 0;
                }
                case LuaNetworkOpcode::WRITE_INT32:
                {
                    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 2));
                    if (auto* w = mgr.GetWriter(id))
                        w->Write<int32_t>(static_cast<int32_t>(lua_tonumber(L, 3)));
                    return 0;
                }
                case LuaNetworkOpcode::WRITE_UINT64:
                {
                    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 2));
                    if (auto* w = mgr.GetWriter(id))
                    {
                        uint64_t v = 0;
                        if (lua_isstring(L, 3))
                        {
                            std::stringstream ss(lua_tostring(L, 3));
                            ss >> v;
                        }
                        else
                        {
                            v = static_cast<uint64_t>(lua_tonumber(L, 3));
                        }
                        w->Write<uint64_t>(v);
                    }
                    return 0;
                }
                case LuaNetworkOpcode::WRITE_INT64:
                {
                    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 2));
                    if (auto* w = mgr.GetWriter(id))
                    {
                        int64_t v = 0;
                        if (lua_isstring(L, 3))
                        {
                            std::stringstream ss(lua_tostring(L, 3));
                            ss >> v;
                        }
                        else
                        {
                            v = static_cast<int64_t>(lua_tonumber(L, 3));
                        }
                        w->Write<int64_t>(v);
                    }
                    return 0;
                }
                case LuaNetworkOpcode::WRITE_FLOAT:
                {
                    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 2));
                    if (auto* w = mgr.GetWriter(id))
                        w->Write<float>(static_cast<float>(lua_tonumber(L, 3)));
                    return 0;
                }
                case LuaNetworkOpcode::WRITE_DOUBLE:
                {
                    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 2));
                    if (auto* w = mgr.GetWriter(id))
                        w->Write<double>(static_cast<double>(lua_tonumber(L, 3)));
                    return 0;
                }
                case LuaNetworkOpcode::WRITE_STRING:
                {
                    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 2));
                    const char* s = lua_tostring(L, 3);
                    if (auto* w = mgr.GetWriter(id))
                    {
                        if (s)
                            w->WriteString(s, strlen(s));
                        else
                            w->WriteString("", 0);
                    }
                    return 0;
                }
                case LuaNetworkOpcode::READ_UINT8:
                {
                    auto* r = mgr.GetCurrentReader();
                    lua_pushnumber(L, r ? r->Read<uint8_t>(static_cast<uint8_t>(lua_tonumber(L, 2))) : lua_tonumber(L, 2));
                    return 1;
                }
                case LuaNetworkOpcode::READ_INT8:
                {
                    auto* r = mgr.GetCurrentReader();
                    lua_pushnumber(L, r ? r->Read<int8_t>(static_cast<int8_t>(lua_tonumber(L, 2))) : lua_tonumber(L, 2));
                    return 1;
                }
                case LuaNetworkOpcode::READ_UINT16:
                {
                    auto* r = mgr.GetCurrentReader();
                    lua_pushnumber(L, r ? r->Read<uint16_t>(static_cast<uint16_t>(lua_tonumber(L, 2))) : lua_tonumber(L, 2));
                    return 1;
                }
                case LuaNetworkOpcode::READ_INT16:
                {
                    auto* r = mgr.GetCurrentReader();
                    lua_pushnumber(L, r ? r->Read<int16_t>(static_cast<int16_t>(lua_tonumber(L, 2))) : lua_tonumber(L, 2));
                    return 1;
                }
                case LuaNetworkOpcode::READ_UINT32:
                {
                    auto* r = mgr.GetCurrentReader();
                    lua_pushnumber(L, r ? r->Read<uint32_t>(static_cast<uint32_t>(lua_tonumber(L, 2))) : lua_tonumber(L, 2));
                    return 1;
                }
                case LuaNetworkOpcode::READ_INT32:
                {
                    auto* r = mgr.GetCurrentReader();
                    lua_pushnumber(L, r ? r->Read<int32_t>(static_cast<int32_t>(lua_tonumber(L, 2))) : lua_tonumber(L, 2));
                    return 1;
                }
                case LuaNetworkOpcode::READ_UINT64:
                {
                    uint64_t def = 0;
                    if (lua_isstring(L, 2))
                    {
                        std::stringstream ss(lua_tostring(L, 2));
                        ss >> def;
                    }
                    else
                    {
                        def = static_cast<uint64_t>(lua_tonumber(L, 2));
                    }
                    auto* r = mgr.GetCurrentReader();
                    uint64_t val = r ? r->Read<uint64_t>(def) : def;
                    lua_pushstring(L, std::to_string(val).c_str());
                    return 1;
                }
                case LuaNetworkOpcode::READ_INT64:
                {
                    int64_t def = 0;
                    if (lua_isstring(L, 2))
                    {
                        std::stringstream ss(lua_tostring(L, 2));
                        ss >> def;
                    }
                    else
                    {
                        def = static_cast<int64_t>(lua_tonumber(L, 2));
                    }
                    auto* r = mgr.GetCurrentReader();
                    int64_t val = r ? r->Read<int64_t>(def) : def;
                    lua_pushstring(L, std::to_string(val).c_str());
                    return 1;
                }
                case LuaNetworkOpcode::READ_FLOAT:
                {
                    auto* r = mgr.GetCurrentReader();
                    lua_pushnumber(L, r ? r->Read<float>(static_cast<float>(lua_tonumber(L, 2))) : lua_tonumber(L, 2));
                    return 1;
                }
                case LuaNetworkOpcode::READ_DOUBLE:
                {
                    auto* r = mgr.GetCurrentReader();
                    lua_pushnumber(L, r ? r->Read<double>(static_cast<double>(lua_tonumber(L, 2))) : lua_tonumber(L, 2));
                    return 1;
                }
                case LuaNetworkOpcode::READ_STRING:
                {
                    std::string def = lua_isstring(L, 2) ? lua_tostring(L, 2) : "";
                    auto* r = mgr.GetCurrentReader();
                    lua_pushstring(L, (r ? r->ReadString(def) : def).c_str());
                    return 1;
                }
                case LuaNetworkOpcode::MAKE_CUSTOM_PACKET:
                {
                    uint16_t pktOpcode = static_cast<uint16_t>(lua_tonumber(L, 2));
                    size_t capacity = static_cast<size_t>(lua_tonumber(L, 3));
                    uint32_t id = mgr.CreateWriter(pktOpcode, capacity);
                    lua_pushnumber(L, id);
                    return 1;
                }
                case LuaNetworkOpcode::SEND_CUSTOM_PACKET:
                {
                    uint32_t id = static_cast<uint32_t>(lua_tonumber(L, 2));
                    if (auto* w = mgr.GetWriter(id))
                    {
                        w->Send();
                    }
                    mgr.RemoveWriter(id);
                    return 0;
                }
                case LuaNetworkOpcode::RESET_CUSTOM_PACKET:
                {
                    if (auto* r = mgr.GetCurrentReader())
                    {
                        r->Reset();
                    }
                    return 0;
                }
                default:
                    return 0;
            }
        }

        static const char* s_clientNetworkLua = R"lua(
local LuaNetworkOpcode = {
    ["WRITE_SIZE"]          = 0,
    ["READ_SIZE"]           = 1,
    ["WRITE_UINT8"]         = 2,
    ["WRITE_INT8"]          = 3,
    ["WRITE_UINT16"]        = 4,
    ["WRITE_INT16"]         = 5,
    ["WRITE_UINT32"]        = 6,
    ["WRITE_INT32"]         = 7,
    ["WRITE_UINT64"]        = 8,
    ["WRITE_INT64"]         = 9,
    ["WRITE_FLOAT"]         = 10,
    ["WRITE_DOUBLE"]        = 11,
    ["WRITE_STRING"]        = 12,

    ["READ_UINT8"]          = 13,
    ["READ_INT8"]           = 14,
    ["READ_UINT16"]         = 15,
    ["READ_INT16"]          = 16,
    ["READ_UINT32"]         = 17,
    ["READ_INT32"]          = 18,
    ["READ_UINT64"]         = 19,
    ["READ_INT64"]          = 20,
    ["READ_FLOAT"]          = 21,
    ["READ_DOUBLE"]         = 22,
    ["READ_STRING"]         = 23,
    ["MAKE_CUSTOM_PACKET"]  = 24,
    ["SEND_CUSTOM_PACKET"]  = 25,
    ["RESET_CUSTOM_PACKET"] = 26,
};

function CreateCustomPacket(opcode, size)
    local writer = { id = _CLIENT_NETWORK(LuaNetworkOpcode.MAKE_CUSTOM_PACKET, opcode, size or 0) }

    function writer:WriteUInt8(value) _CLIENT_NETWORK(LuaNetworkOpcode.WRITE_UINT8, self.id, value); return self end
    function writer:WriteInt8(value) _CLIENT_NETWORK(LuaNetworkOpcode.WRITE_INT8, self.id, value); return self end
    function writer:WriteUInt16(value) _CLIENT_NETWORK(LuaNetworkOpcode.WRITE_UINT16, self.id, value); return self end
    function writer:WriteInt16(value) _CLIENT_NETWORK(LuaNetworkOpcode.WRITE_INT16, self.id, value); return self end
    function writer:WriteUInt32(value) _CLIENT_NETWORK(LuaNetworkOpcode.WRITE_UINT32, self.id, value); return self end
    function writer:WriteInt32(value) _CLIENT_NETWORK(LuaNetworkOpcode.WRITE_INT32, self.id, value); return self end
    function writer:WriteUInt64(value) _CLIENT_NETWORK(LuaNetworkOpcode.WRITE_UINT64, self.id, value); return self end
    function writer:WriteInt64(value) _CLIENT_NETWORK(LuaNetworkOpcode.WRITE_INT64, self.id, value); return self end
    function writer:WriteFloat(value) _CLIENT_NETWORK(LuaNetworkOpcode.WRITE_FLOAT, self.id, value); return self end
    function writer:WriteDouble(value) _CLIENT_NETWORK(LuaNetworkOpcode.WRITE_DOUBLE, self.id, value); return self end
    function writer:WriteString(value) _CLIENT_NETWORK(LuaNetworkOpcode.WRITE_STRING, self.id, value); return self end

    function writer:Size() return _CLIENT_NETWORK(LuaNetworkOpcode.WRITE_SIZE, self.id) end

    function writer:Send()
        _CLIENT_NETWORK(LuaNetworkOpcode.SEND_CUSTOM_PACKET, self.id)
        return self
    end

    return writer
end

CreateCustompacket = CreateCustomPacket

function __ReadCustomPacket()
    local reader = {}
    function reader:ReadUInt8() return _CLIENT_NETWORK(LuaNetworkOpcode.READ_UINT8) end
    function reader:ReadInt8() return _CLIENT_NETWORK(LuaNetworkOpcode.READ_INT8) end
    function reader:ReadUInt16() return _CLIENT_NETWORK(LuaNetworkOpcode.READ_UINT16) end
    function reader:ReadInt16() return _CLIENT_NETWORK(LuaNetworkOpcode.READ_INT16) end
    function reader:ReadUInt32() return _CLIENT_NETWORK(LuaNetworkOpcode.READ_UINT32) end
    function reader:ReadInt32() return _CLIENT_NETWORK(LuaNetworkOpcode.READ_INT32) end
    function reader:ReadUInt64() return _CLIENT_NETWORK(LuaNetworkOpcode.READ_UINT64) end
    function reader:ReadInt64() return _CLIENT_NETWORK(LuaNetworkOpcode.READ_INT64) end
    function reader:ReadFloat() return _CLIENT_NETWORK(LuaNetworkOpcode.READ_FLOAT) end
    function reader:ReadDouble() return _CLIENT_NETWORK(LuaNetworkOpcode.READ_DOUBLE) end
    function reader:ReadString() return _CLIENT_NETWORK(LuaNetworkOpcode.READ_STRING) end
    function reader:Size() return _CLIENT_NETWORK(LuaNetworkOpcode.READ_SIZE) end
    return reader
end

__callbacks = __callbacks or {}
function OnCustomPacket(opcode, cb)
    if __callbacks[opcode] == nil then
        __callbacks[opcode] = {}
    end
    table.insert(__callbacks[opcode], cb)
end

function __FireCustomPacket(opcode)
    if __callbacks[opcode] == nil then return end
    for _, v in pairs(__callbacks[opcode]) do
        v(__ReadCustomPacket())
        _CLIENT_NETWORK(LuaNetworkOpcode.RESET_CUSTOM_PACKET)
    end
end
)lua";

        static const char* s_loggerLua = R"lua(
local function serializeTable(val, name, skipnewlines, depth)
    skipnewlines = skipnewlines or false
    depth = depth or 0
    local tmp = string.rep(" ", depth)
    if name then
        tmp = tmp .. name .. " = "
    end
    if type(val) == "table" then
        tmp = tmp .. "{" .. (not skipnewlines and "\n" or "")
        for k, v in pairs(val) do
            tmp = tmp .. serializeTable(v, k, skipnewlines, depth + 1) .. "," .. (not skipnewlines and "\n" or "")
        end
        tmp = tmp .. string.rep(" ", depth) .. "}"
    elseif type(val) == "number" then
        tmp = tmp .. tostring(val)
    elseif type(val) == "string" then
        tmp = tmp .. string.format("%q", val)
    elseif type(val) == "boolean" then
        tmp = tmp .. (val and "true" or "false")
    else
        tmp = tmp .. "\"[inserializeable datatype:" .. type(val) .. "]\""
    end
    return tmp
end

local function stringify(...)
    local str = ""
    for i = 1, select("#", ...) do
        local v = select(i, ...)
        if type(v) == "string" then
            str = str .. v
        elseif type(v) == "number" or type(v) == "boolean" then
            str = str .. tostring(v)
        elseif type(v) == "table" then
            str = str .. serializeTable(v, nil, true)
        else
            str = str .. type(v)
        end
    end
    return str
end

function LOG_DEBUG(...)
    if _LUA_LOG then _LUA_LOG(0, stringify(...)) end
end

function LOG_INFO(...)
    if _LUA_LOG then _LUA_LOG(1, stringify(...)) end
end

function LOG_WARN(...)
    if _LUA_LOG then _LUA_LOG(2, stringify(...)) end
end

function LOG_ERROR(...)
    if _LUA_LOG then _LUA_LOG(3, stringify(...)) end
end
)lua";
    }

    void CustomPacketManager::RegisterLuaBindings(lua_State* L)
    {
        int top = L ? lua_gettop(L) : 0;
        FrameScript_RegisterFunction("_CLIENT_NETWORK", Script_CLIENT_NETWORK);
        FrameScript_RegisterFunction("_LUA_LOG", Script_LuaLog);

        FrameScript_Execute(s_clientNetworkLua, "ClientNetwork.lua", nullptr);
        FrameScript_Execute(s_loggerLua, "Logger.lua", nullptr);
        if (L)
        {
            lua_settop(L, top);
        }
    }
}
