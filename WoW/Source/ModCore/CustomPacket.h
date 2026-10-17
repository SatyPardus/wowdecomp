#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <functional>

struct lua_State;

namespace ModCore
{
#pragma pack(push, 1)
    struct CustomPacketHeader
    {
        uint16_t fragmentId;
        uint16_t totalFrags;
        uint16_t opcode;
    };
#pragma pack(pop)

    constexpr uint16_t SERVER_TO_CLIENT_OPCODE = 0x102;
    constexpr uint16_t CLIENT_TO_SERVER_OPCODE = 0x51f;
    constexpr uint16_t ALT_SERVER_OPCODE       = 0x7A00;
    constexpr size_t   MAX_FRAGMENT_SIZE       = 30000;

    class CustomPacketWriter
    {
    public:
        CustomPacketWriter(uint16_t opcode = 0, size_t initialCapacity = 0);

        void WriteBytes(const void* data, size_t size);
        void WriteString(const char* str, size_t len);
        void WriteString(const std::string& str);

        template <typename T>
        void Write(T val)
        {
            WriteBytes(&val, sizeof(val));
        }

        size_t Size() const { return m_buffer.size(); }
        uint16_t Opcode() const { return m_opcode; }
        void Send();

    private:
        uint16_t m_opcode;
        std::vector<uint8_t> m_buffer;
    };

    class CustomPacketReader
    {
    public:
        CustomPacketReader(uint16_t opcode, const uint8_t* data, size_t size);
        CustomPacketReader() : m_opcode(0), m_readPos(0) {}

        size_t Size() const { return m_buffer.size(); }
        uint16_t Opcode() const { return m_opcode; }
        void Reset() { m_readPos = 0; }

        void ReadBytes(void* dest, size_t size);
        std::string ReadString(const std::string& def = "");

        template <typename T>
        T Read(T def)
        {
            if (m_readPos + sizeof(T) > m_buffer.size())
                return def;
            T val;
            memcpy(&val, m_buffer.data() + m_readPos, sizeof(T));
            m_readPos += sizeof(T);
            return val;
        }

    private:
        uint16_t m_opcode;
        std::vector<uint8_t> m_buffer;
        size_t m_readPos;
    };

    class CustomPacketManager
    {
    public:
        static CustomPacketManager& Get();

        void RegisterLuaBindings(lua_State* L);
        bool HandlePacket(uint32_t opcode, uint32_t time, void* dataStore);
        void RegisterHandler(uint16_t opcode, std::function<void(CustomPacketReader&)> handler);

        CustomPacketWriter* GetWriter(uint32_t id);
        uint32_t CreateWriter(uint16_t opcode, size_t capacity);
        void RemoveWriter(uint32_t id);

        CustomPacketReader* GetCurrentReader() { return m_curRead; }
        void SetCurrentReader(CustomPacketReader* reader) { m_curRead = reader; }

    private:
        CustomPacketManager() = default;

        uint32_t m_nextWriterId = 1;
        std::map<uint32_t, CustomPacketWriter> m_writers;
        CustomPacketReader* m_curRead = nullptr;

        // Fragment reassembly
        uint16_t m_pendingOpcode = 0;
        uint16_t m_pendingTotalFrags = 0;
        std::vector<uint8_t> m_reassemblyBuffer;

        std::map<uint16_t, std::vector<std::function<void(CustomPacketReader&)>>> m_cppHandlers;
    };
}
