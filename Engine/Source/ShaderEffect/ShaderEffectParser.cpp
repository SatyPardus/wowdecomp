#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include "Engine/Source/ShaderEffect/ShaderEffectParser.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00A4D678
const EGxTexOp CShaderEffectParser::s_gxTexOpValues[GxTexOps_Last] = {
    GxTexOp_Mod,
    GxTexOp_Mod2x,
    GxTexOp_Add,
    GxTexOp_PassThru,
    GxTexOp_Decal,
    GxTexOp_Fade,
};

// STATIC: 0x00B1D5C8
const char* CShaderEffectParser::s_gxTexOpNames[GxTexOps_Last] = {
    "Mod",
    "Mod2x",
    "Add",
    "PassThru",
    "Decal",
    "Fade",
};

// VERIFY: 64b6e307 auto
// OFFSET: 0x00876F10
static int32_t FindScope(const char* data, uint32_t start, uint32_t end, uint32_t* open, uint32_t* close) {
    int32_t depth = 0;

    for (uint32_t i = start; i < end; i++) {
        if (data[i] == '{') {
            if (!depth) {
                *open = i;
            }

            depth++;
        } else if (data[i] == '}' && !--depth) {
            *close = i;
            return 1;
        }
    }

    return 0;
}

// VERIFY: 6bc79b07 auto
// OFFSET: 0x00876F70
static EGxTexOp LookupTexOp(const char* name) {
    for (uint32_t i = 0; i < GxTexOps_Last; i++) {
        if (!SStrCmpI(name, CShaderEffectParser::s_gxTexOpNames[i], STORM_MAX_STR)) {
            return CShaderEffectParser::s_gxTexOpValues[i];
        }
    }

    return GxTexOp_Mod;
}

// VERIFY: 361a6237 manual
// OFFSET: 0x00876E30
int32_t CShaderEffectParser::ExtractFuncKeyword(const char* text, uint32_t size, const char* keyword, char (*args)[64], uint32_t& argCount, uint32_t& pos) {
    const size_t length = strlen(keyword);
    if (size <= length || std::memcmp(text, keyword, length) || text[length] != '(') return 0;
    argCount = 0;
    unsigned column = 0;
    for (size_t i = length + 1; i < size; ++i) {
        const char c = text[i];
        if (c == ')') { pos += uint32_t(i + 1); return 1; }
        if (!argCount) { argCount = 1; args[0][0] = 0; }
        if (c == ',') {
            if (argCount == 16) throw std::runtime_error("Too many shader effect arguments");
            args[argCount++][0] = 0; column = 0;
        } else {
            if (!c || c == '{' || c == '}' || column == 63) throw std::runtime_error("Invalid shader effect argument");
            args[argCount - 1][column++] = c; args[argCount - 1][column] = 0;
        }
    }
    throw std::runtime_error("Unterminated shader effect argument list");
}

// VERIFY: 43a0acdb manual
// OFFSET: 0x00876FB0
void CShaderEffectParser::ParseFixedFunc(char* data, uint32_t size, uint32_t& pos, char (*args)[64], uint32_t argCount, ParsedEffect* effect) {
    uint32_t open;
    uint32_t close;

    if (!FindScope(data, pos, size, &open, &close)) throw std::runtime_error("Unterminated shader effect scope");

    pos = open + 1;

    while (pos < close) {
        if (CShaderEffectParser::ExtractFuncKeyword(data + pos, size - pos, "Pass", args, argCount, pos)) {
            if (argCount != 1 || effect->m_fixedFuncPassCount) throw std::runtime_error("Unsupported shader effect fixed pass declaration");
            ParsedEffect::FixedFuncPass* pass = &effect->m_fixedFuncPasses[effect->m_fixedFuncPassCount];

            char* end;
            pass->m_pass = strtol(args[0], &end, 10);
            if (*end || pass->m_pass < 1 || pass->m_pass > 2) throw std::runtime_error("Unsupported fixed-function texture stage count");

            uint32_t passOpen;
            uint32_t passClose;
            if (!FindScope(data, pos, close, &passOpen, &passClose)) throw std::runtime_error("Unterminated shader effect pass");

            pass->m_colorOps[0] = GxTexOp_Mod;
            pass->m_colorOps[1] = GxTexOp_Mod;
            pass->m_colorOps[2] = GxTexOp_Mod;
            pass->m_alphaOps[0] = GxTexOp_Mod;
            pass->m_alphaOps[1] = GxTexOp_Mod;
            pass->m_alphaOps[2] = GxTexOp_Mod;

            while (pos < passClose) {
                if (CShaderEffectParser::ExtractFuncKeyword(data + pos, size - pos, "ColorOp0", args, argCount, pos)) {
                    if (argCount != 1) throw std::runtime_error("Invalid shader texture operation");
                    pass->m_colorOps[0] = LookupTexOp(args[0]);
                } else if (CShaderEffectParser::ExtractFuncKeyword(data + pos, size - pos, "ColorOp1", args, argCount, pos)) {
                    if (argCount != 1) throw std::runtime_error("Invalid shader texture operation");
                    pass->m_colorOps[1] = LookupTexOp(args[0]);
                } else if (CShaderEffectParser::ExtractFuncKeyword(data + pos, size - pos, "AlphaOp0", args, argCount, pos)) {
                    if (argCount != 1) throw std::runtime_error("Invalid shader texture operation");
                    pass->m_alphaOps[0] = LookupTexOp(args[0]);
                } else if (CShaderEffectParser::ExtractFuncKeyword(data + pos, size - pos, "AlphaOp1", args, argCount, pos)) {
                    if (argCount != 1) throw std::runtime_error("Invalid shader texture operation");
                    pass->m_alphaOps[1] = LookupTexOp(args[0]);
                }

                pos++;
            }

            effect->m_fixedFuncPassCount++;
            pos = passClose + 1;
            continue;
        }

        pos++;
    }

    pos = close + 1;
}

// VERIFY: 6fc19477 manual
// OFFSET: 0x00877150
void CShaderEffectParser::ParseShader(char* data, uint32_t size, uint32_t& pos, char (*args)[64], uint32_t argCount, ParsedEffect* effect) {
    uint32_t open;
    uint32_t close;

    if (!FindScope(data, pos, size, &open, &close)) throw std::runtime_error("Unterminated shader effect scope");

    pos = open + 1;

    while (pos < close) {
        if (CShaderEffectParser::ExtractFuncKeyword(data + pos, size - pos, "Pass", args, argCount, pos)) {
            if (argCount || effect->m_shaderPassCount) throw std::runtime_error("Unsupported shader effect shader pass declaration");
            ParsedEffect::ShaderPass* pass = &effect->m_shaderPasses[effect->m_shaderPassCount];

            uint32_t passOpen;
            uint32_t passClose;
            if (!FindScope(data, pos, close, &passOpen, &passClose)) throw std::runtime_error("Unterminated shader effect pass");

            while (pos < passClose) {
                if (CShaderEffectParser::ExtractFuncKeyword(data + pos, size - pos, "VertexShader", args, argCount, pos)) {
                    if (argCount != 1) throw std::runtime_error("Invalid vertex shader declaration");
                    strcpy(pass->m_vertexShader, args[0]);
                } else if (CShaderEffectParser::ExtractFuncKeyword(data + pos, size - pos, "PixelShader", args, argCount, pos)) {
                    if (argCount != 1) throw std::runtime_error("Invalid pixel shader declaration");
                    strcpy(pass->m_pixelShader, args[0]);
                }

                pos++;
            }

            if (!pass->m_vertexShader[0] || !pass->m_pixelShader[0]) throw std::runtime_error("Incomplete shader effect pass");
            effect->m_shaderPassCount++;
            pos = passClose + 1;
            continue;
        }

        pos++;
    }

    pos = close + 1;
}

// VERIFY: 353721c3 auto
// OFFSET: 0x00877290
void CShaderEffectParser::ParseEffect(char* data, uint32_t size, uint32_t& pos, char (*args)[64], uint32_t argCount, ParsedEffect* effect) {
    if (argCount != 1 || !args[0][0]) throw std::runtime_error("Invalid shader effect name");
    *effect = {};
    SStrCopy(effect->m_name, args[0], sizeof(effect->m_name));
    effect->m_fixedFuncPassCount = 0;
    effect->m_shaderPassCount = 0;

    uint32_t open;
    uint32_t close;

    if (!FindScope(data, pos, size, &open, &close)) throw std::runtime_error("Unterminated shader effect scope");

    pos = open + 1;

    while (pos < close) {
        if (CShaderEffectParser::ExtractFuncKeyword(data + pos, size - pos, "FixedFunc", args, argCount, pos)) {
            CShaderEffectParser::ParseFixedFunc(data, size, pos, args, argCount, effect);
            continue;
        } else if (CShaderEffectParser::ExtractFuncKeyword(data + pos, size - pos, "Shader", args, argCount, pos)) {
            CShaderEffectParser::ParseShader(data, size, pos, args, argCount, effect);
            continue;
        }

        pos++;
    }
    pos = close + 1;
}

// VERIFY: e3f85c73 auto
// OFFSET: 0x00877360
void CShaderEffectParser::ParseEffectFile(char* data, uint32_t size, void (*callback)(const ParsedEffect&, void*), void* param) {
    char args[16][64];
    ParsedEffect effect;
    uint32_t argCount;

    uint32_t pos = 0;
    while (pos < size) {
        if (CShaderEffectParser::ExtractFuncKeyword(data + pos, size - pos, "Effect", args, argCount, pos)) {
            CShaderEffectParser::ParseEffect(data, size, pos, args, argCount, &effect);
            callback(effect, param);
            continue;
        }
        ++pos;
    }
}
