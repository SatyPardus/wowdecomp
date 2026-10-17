#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>
#include "Engine/Source/ShaderEffect/ShaderEffectManager.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00D43408
TSHashTable<CShaderEffect, HASHKEY_STRI> CShaderEffectManager::m_ShaderList;

// VERIFY: 554f80ac manual
// OFFSET: 0x008764D0
void CShaderEffectManager::DestroyAllEffects() {
    CShaderEffect* effect = CShaderEffectManager::m_ShaderList.m_fulllist.Head();

    while (effect) {
        effect = CShaderEffectManager::m_ShaderList.DeleteNode(effect);
    }
}

// VERIFY: e6522edb auto
// OFFSET: 0x00876530
CShaderEffect* CShaderEffectManager::GetEffect(const char* name) {
    return CShaderEffectManager::m_ShaderList.Ptr(name);
}

// VERIFY: 82eb3c99 manual
// OFFSET: 0x00876BE0
CShaderEffect* CShaderEffectManager::CreateEffect(const char* name) {
    CShaderEffectManager::m_ShaderList.Ptr(name);

    return CShaderEffectManager::m_ShaderList.New(name, 0, 0);
}

// VERIFY: 156a1818 manual
// OFFSET: 0x00876CA0
void CShaderEffectManager::ParseEffectCallback(const CShaderEffectParser::ParsedEffect& effect, void* param) {
    CShaderEffect* shaderEffect = CShaderEffectManager::m_ShaderList.New(effect.m_name, 0, 0);

    shaderEffect->InitEffect(effect.m_shaderPassCount ? effect.m_shaderPasses[0].m_vertexShader : nullptr,
        effect.m_shaderPassCount ? effect.m_shaderPasses[0].m_pixelShader : nullptr);

    for (uint32_t i = 0; i < effect.m_fixedFuncPassCount; i++) {
        const CShaderEffectParser::ParsedEffect::FixedFuncPass& pass = effect.m_fixedFuncPasses[i];
        shaderEffect->InitFixedFuncPass(pass.m_colorOps, pass.m_alphaOps, pass.m_pass);
    }
}

// VERIFY: c6a3a3f5 manual
// OFFSET: 0x00876D90
void CShaderEffectManager::AddEffectFile(const char* fileName) {
    const std::string path = std::string("Shaders\\Effects\\") + fileName;
    SFileImpl* file = nullptr;
    if (!SFileOpenFile(path.c_str(), &file) || !file) return;
    struct Close { SFileImpl* file; ~Close() { SFileCloseFile(file); } } close{file};
    uint32_t high = 0;
    const uint32_t size = SFileGetFileSize(file, &high);
    if (high || !size || size > 1024 * 1024) throw std::runtime_error("Invalid shader effect file size: " + path);
    std::vector<char> data(size + 1, 0);
    uint32_t read = 0;
    if (!SFileReadFile(file, data.data(), size, &read, nullptr, 0) || read != size)
        throw std::runtime_error("Incomplete shader effect file: " + path);
    CShaderEffectParser::ParseEffectFile(data.data(), size, CShaderEffectManager::ParseEffectCallback, nullptr);
}
