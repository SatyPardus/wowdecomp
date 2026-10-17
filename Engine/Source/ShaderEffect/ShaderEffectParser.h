#ifndef ENGINE_SOURCE_SHADEREFFECT_SHADEREFFECTPARSER_H
#define ENGINE_SOURCE_SHADEREFFECT_SHADEREFFECTPARSER_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/ShaderEffect/Types.h"

class CShaderEffectParser {
    public:
    struct ParsedEffect {
        struct FixedFuncPass {
            /* 0000 */ uint32_t m_pass;
            /* 0004 */ EGxTexOp m_colorOps[3];
            /* 0010 */ EGxTexOp m_alphaOps[3];
        };

        struct ShaderPass {
            /* 0000 */ char m_vertexShader[64];
            /* 0040 */ char m_pixelShader[64];
        };

        /* 0000 */ char m_name[64];
        /* 0040 */ uint32_t m_fixedFuncPassCount;
        /* 0044 */ FixedFuncPass m_fixedFuncPasses[1];
        /* 0060 */ uint32_t m_shaderPassCount;
        /* 0064 */ ShaderPass m_shaderPasses[1];
    };

    static const char* s_gxTexOpNames[GxTexOps_Last];
    static const EGxTexOp s_gxTexOpValues[GxTexOps_Last];

    static int32_t ExtractFuncKeyword(const char* text, uint32_t size, const char* keyword, char (*args)[64], uint32_t& argCount, uint32_t& pos);
    static void ParseEffect(char* data, uint32_t size, uint32_t& pos, char (*args)[64], uint32_t argCount, ParsedEffect* effect);
    static void ParseEffectFile(char* data, uint32_t size, void (*callback)(const ParsedEffect&, void*), void* param);
    static void ParseFixedFunc(char* data, uint32_t size, uint32_t& pos, char (*args)[64], uint32_t argCount, ParsedEffect* effect);
    static void ParseShader(char* data, uint32_t size, uint32_t& pos, char (*args)[64], uint32_t argCount, ParsedEffect* effect);
};

#endif
