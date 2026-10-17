#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <map>

struct lua_State;

namespace ModCore
{
    struct HookParam
    {
        std::string name;
        std::string type;
        std::string doc;
    };

    struct HookDefinition
    {
        std::string name;
        std::string category;
        std::string description;
        std::vector<HookParam> params;
        std::string returnType;
        std::string returnDoc;
    };

    struct MethodParam
    {
        std::string name;
        std::string type;
        std::string doc;
    };

    struct MethodDefinition
    {
        std::string name;
        std::string description;
        std::vector<MethodParam> params;
        std::string returnType;
    };

    struct ClassDefinition
    {
        std::string name;
        std::string parent;
        std::string description;
        std::vector<MethodParam> fields;
        std::vector<MethodDefinition> methods;
    };

    struct OpcodeDefinition
    {
        std::string name;
        uint32_t opcode;
        std::string direction; // "CLIENT_TO_SERVER" or "SERVER_TO_CLIENT"
        std::string description;
    };

    class AutoExport
    {
    public:
        static AutoExport& Get();

        void Initialize(const std::filesystem::path& apiDir);
        void RegisterHook(const HookDefinition& hook);
        void RegisterClass(const ClassDefinition& cls);
        void RegisterOpcode(const OpcodeDefinition& op);

        void ExportAll(lua_State* L);

        const std::vector<HookDefinition>& GetHooks() const { return m_hooks; }
        const std::vector<ClassDefinition>& GetClasses() const { return m_classes; }
        const std::vector<OpcodeDefinition>& GetOpcodes() const { return m_opcodes; }

    private:
        AutoExport();
        void PopulateDefaultMetadata();
        void ImportLuaMetadata(lua_State* L);

        void GenerateEmmyLua(const std::filesystem::path& outputPath);
        void GenerateJsonSchema(const std::filesystem::path& outputPath);
        void GenerateAiGuide(const std::filesystem::path& outputPath);

        std::filesystem::path m_apiDir;
        std::vector<HookDefinition> m_hooks;
        std::vector<ClassDefinition> m_classes;
        std::vector<OpcodeDefinition> m_opcodes;
    };
}
