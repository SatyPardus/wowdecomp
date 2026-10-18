#include "BytecodeCache.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace ModCore
{
    namespace
    {
        int DumpWriter(lua_State* L, const void* p, size_t sz, void* ud)
        {
            auto* vec = static_cast<std::vector<uint8_t>*>(ud);
            const auto* bytes = static_cast<const uint8_t*>(p);
            vec->insert(vec->end(), bytes, bytes + sz);
            return 0;
        }

        std::string ToHex(uint64_t val)
        {
            std::stringstream ss;
            ss << std::hex << std::setfill('0') << std::setw(16) << val;
            return ss.str();
        }
    }

    BytecodeCache& BytecodeCache::Get()
    {
        static BytecodeCache instance;
        return instance;
    }

    void BytecodeCache::Initialize(const std::filesystem::path& cacheDir, bool enabled)
    {
        m_cacheDir = cacheDir;
        m_enabled = enabled;
        if (m_enabled && !std::filesystem::exists(m_cacheDir))
        {
            std::error_code ec;
            std::filesystem::create_directories(m_cacheDir, ec);
        }
    }

    uint64_t BytecodeCache::ComputeFileHash(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return 0;

        // FNV-1a 64-bit hash
        uint64_t hash = 14695981039346656037ull;
        char buffer[4096];
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
        {
            size_t bytes = static_cast<size_t>(file.gcount());
            for (size_t i = 0; i < bytes; ++i)
            {
                hash ^= static_cast<uint8_t>(buffer[i]);
                hash *= 1099511628211ull;
            }
        }
        return hash;
    }

    bool BytecodeCache::LoadScript(lua_State* L, const std::filesystem::path& scriptPath, const std::string& chunkName)
    {
        if (!std::filesystem::exists(scriptPath))
        {
            return false;
        }

        uint64_t fileHash = ComputeFileHash(scriptPath);
        std::string hashStr = ToHex(fileHash);
        std::filesystem::path cacheFile = m_cacheDir / (hashStr + ".luac");

        // Check if cached bytecode exists and is valid
        if (m_enabled && std::filesystem::exists(cacheFile))
        {
            std::ifstream cached(cacheFile, std::ios::binary | std::ios::ate);
            if (cached.is_open())
            {
                size_t size = static_cast<size_t>(cached.tellg());
                cached.seekg(0, std::ios::beg);
                std::vector<char> buffer(size);
                if (cached.read(buffer.data(), size))
                {
                    int loadStatus = luaL_loadbuffer(L, buffer.data(), size, chunkName.c_str());
                    if (loadStatus == 0)
                    {
                        // Successfully loaded from cache!
                        return true;
                    }
                    else
                    {
                        // Cache invalid or corrupt, pop error message and compile from source
                        lua_pop(L, 1);
                    }
                }
            }
        }

        // Cache miss or disabled: load from source
        std::ifstream sourceFile(scriptPath, std::ios::binary | std::ios::ate);
        if (!sourceFile.is_open())
        {
            std::cerr << "[ModCore] Could not open script: " << scriptPath.string() << std::endl;
            return false;
        }

        size_t sourceSize = static_cast<size_t>(sourceFile.tellg());
        sourceFile.seekg(0, std::ios::beg);
        std::vector<char> sourceBuffer(sourceSize);
        if (!sourceFile.read(sourceBuffer.data(), sourceSize))
        {
            std::cerr << "[ModCore] Failed reading script: " << scriptPath.string() << std::endl;
            return false;
        }

        int loadStatus = luaL_loadbuffer(L, sourceBuffer.data(), sourceSize, chunkName.c_str());
        if (loadStatus != 0)
        {
            const char* err = lua_tostring(L, -1);
            std::cerr << "[ModCore] Syntax error compiling " << scriptPath.string() << ": " << (err ? err : "") << std::endl;
            lua_pop(L, 1);
            return false;
        }

        // If caching is enabled, dump compiled bytecode to cacheFile
        if (m_enabled)
        {
            std::vector<uint8_t> dumpBuffer;
            if (lua_dump(L, DumpWriter, &dumpBuffer) == 0 && !dumpBuffer.empty())
            {
                std::ofstream out(cacheFile, std::ios::binary | std::ios::trunc);
                if (out.is_open())
                {
                    out.write(reinterpret_cast<const char*>(dumpBuffer.data()), dumpBuffer.size());
                }
            }
        }

        return true;
    }
}
