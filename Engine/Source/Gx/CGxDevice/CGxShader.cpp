#include <cstdint>
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxDevice/CGxShader.h"

// INLINED
CGxShader::CGxShader() : m_refCount(0), m_apiSpecific(nullptr), m_target(0), unused_0028(0), m_valid(0), m_loaded(0), unused_0034(0), m_patched(0) {
}

// VERIFY: 089de908 manual
// OFFSET: 0x00689A50
int32_t CGxShader::Valid() {
    if (!this->m_loaded) {
        g_theGxDevicePtr->IShaderCreate(this);
    }

    return this->m_valid;
}

// VERIFY: 0031218a manual
// OFFSET: 0x00689A70
void CGxShader::Load(SFileImpl* file) {
    SFileReadFile(file, &this->unk_003C, 4, nullptr, nullptr, 0);
    SFileReadFile(file, &this->unk_0040, 4, nullptr, nullptr, 0);
    SFileReadFile(file, &this->unk_0044, 2, nullptr, nullptr, 0);
    SFileReadFile(file, &this->unk_0046, 2, nullptr, nullptr, 0);

    uint32_t codeSize;
    SFileReadFile(file, &codeSize, 4, nullptr, nullptr, 0);
    this->m_code.SetCount(codeSize);
    SFileReadFile(file, this->m_code.m_data, codeSize, nullptr, nullptr, 0);

    uint8_t padding[4];
    SFileReadFile(file, padding, ((codeSize + 3) & ~3u) - codeSize, nullptr, nullptr, 0);
}

// VERIFY: 73c2698b auto
// OFFSET: 0x00689B60
CGxShader::~CGxShader() {
}
