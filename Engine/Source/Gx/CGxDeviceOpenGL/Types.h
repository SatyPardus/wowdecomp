#ifndef ENGINE_SOURCE_GX_CGXDEVICEOPENGL_TYPES_H
#define ENGINE_SOURCE_GX_CGXDEVICEOPENGL_TYPES_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/CGxDeviceOpenGL/GlExt.h"

class NvCombinerInput {
    public:
    /* 0000 */ GLenum m_input;
    /* 0004 */ GLenum m_mapping;
    /* 0008 */ GLenum m_componentUsage;
};

class NvCombinerStage {
    public:
    /* 0000 */ NvCombinerInput m_inputs [4];
    /* 0030 */ GLenum m_abOutput;
    /* 0034 */ GLenum m_cdOutput;
    /* 0038 */ GLenum m_sumOutput;
    /* 003C */ GLenum m_scale;
    /* 0040 */ GLenum m_bias;
    /* 0044 */ GLboolean m_abDotProduct;
    /* 0045 */ GLboolean m_cdDotProduct;
    /* 0046 */ GLboolean m_muxSum;
    /* 0047 */ uint8_t unused_0047;

    void Apply(GLenum stage, GLenum portion);
};

class NvGeneralCombiner {
    public:
    /* 0000 */ NvCombinerStage m_rgb;
    /* 0048 */ NvCombinerStage m_alpha;
    /* 0090 */ GLfloat m_constants [2][4];
};

class NvTextureShaderStage {
    public:
    /* 0000 */ GLenum m_shaderOperation;
    /* 0004 */ GLboolean m_cullModes [4];
    /* 0008 */ GLenum m_dotProductMapping;
    /* 000C */ GLenum m_previousInput;

    void Apply(uint32_t tmu);
};

class NvTextureShader {
    public:
    /* 0000 */ uint32_t unused_0000;
    /* 0004 */ NvTextureShaderStage m_stages [4];

    void Apply();
};

class NvPixelShader {
    public:
    /* 0000 */ uint32_t unused_0000;
    /* 0004 */ GLint m_numGeneralCombiners;
    /* 0008 */ GLboolean m_colorSumClamp;
    /* 0009 */ GLboolean m_perStageConstants;
    /* 000A */ uint16_t unused_000A;
    /* 000C */ NvGeneralCombiner m_generalCombiners [8];
    /* 058C */ NvCombinerInput m_finalInputs [7];
    /* 05E0 */ GLfloat m_constants [2][4];
    /* 0600 */ uint8_t m_constantMapCount;
    /* 0601 */ uint8_t m_constantMap [19];
    /* 0614 */ NvTextureShader m_textureShader;

    void SetupRegisterCombiners();
};

#endif
