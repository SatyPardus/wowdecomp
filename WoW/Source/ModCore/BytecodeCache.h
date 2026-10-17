#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

struct lua_State;

namespace ModCore
{
    class BytecodeCache
    {
    public:
        static BytecodeCache& Get();

        void Initialize(const std::filesystem::path& cacheDir, bool enabled);
        
        // Loads a script from disk: checks bytecode cache first, compiles & caches if needed.
        // Leaves the compiled chunk function on the Lua stack, returns true on success.
        bool LoadScript(lua_State* L, const std::filesystem::path& scriptPath, const std::string& chunkName);

        // Compute 64-bit FNV-1a hash of a file for fast cache validation
        static uint64_t ComputeFileHash(const std::filesystem::path& path);

    private:
        std::filesystem::path m_cacheDir;
        bool m_enabled = true;
    };
}
