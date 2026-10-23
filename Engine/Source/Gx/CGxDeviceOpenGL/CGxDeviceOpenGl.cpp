#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <malloc.h>
#include <new>
#include "Engine/Source/Gx/CGxDeviceOpenGL/CGxDeviceOpenGl.h"
#include "Engine/Source/Gx/CGxDeviceOpenGL/CGxVboBroker1.h"
#include "Engine/Source/Gx/CGxDeviceOpenGL/Types.h"
#include "Engine/Source/Gx/Blit.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Gx/CGxTex.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/ShaderEffect/Types.h"
#include "GameVersion.h"

static const uint32_t kNullTmu = 0xFFFFFFFF;

// STATIC: 0x00A2E000
static const GLenum s_glBufferTarget[GxPoolTargets_Last] = { GL_ARRAY_BUFFER_ARB, GL_ELEMENT_ARRAY_BUFFER_ARB };

// STATIC: 0x00A2E008
static const CGxDeviceOpenGl::EDeviceState s_bufferState[GxPoolTargets_Last] = { CGxDeviceOpenGl::Ds_ArrayBuffer, CGxDeviceOpenGl::Ds_ElementArrayBuffer };

// STATIC: 0x00A2E010
static const GLenum s_glBufferUsage[3] = { GL_STATIC_DRAW_ARB, GL_DYNAMIC_DRAW_ARB, GL_STREAM_DRAW_ARB };

// STATIC: 0x00A2E05C
static const GLenum s_glTexTarget[4] = { GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_2D };

// STATIC: 0x00A2E06C
static const GLenum s_glQueryTarget[1] = { GL_SAMPLES_PASSED_ARB };

// STATIC: 0x00A2F210
static const GLint s_glInternalFormat[GxTexFormats_Last] = { 0, GL_RGBA8, GL_RGBA8, GL_RGBA4, GL_RGB5_A1, GL_RGB5, 0x83F1, 0x83F2, 0x83F3, 0x8709, 0x881B, 0, GL_DEPTH_COMPONENT24_ARB };

// STATIC: 0x00A2F1E0
static const GLint s_glMinFilter[GxTexFilters_Last] = { GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR };

// STATIC: 0x00A2F1F8
static const GLint s_glMagFilter[GxTexFilters_Last] = { GL_NEAREST, GL_LINEAR, GL_LINEAR, GL_LINEAR, GL_LINEAR, GL_LINEAR };

// STATIC: 0x00A2F244
static const uint32_t s_glBytesPerPixel[GxTexFormats_Last] = { 0, 4, 4, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0 };

// STATIC: 0x00A2F2E0
static const GLenum s_glCubeFaces[GxTexCubeFaces_Last] = { 0x8515, 0x8516, 0x8517, 0x8518, 0x8519, 0x851A };

// STATIC: 0x00A2F278
static const GLenum s_glFormat[GxTexFormats_Last] = { 0, GL_RGBA, 0x80E1, 0x80E1, 0x80E1, GL_RGB, 0, 0, 0, 0x86F5, GL_RGB, GL_RGB, GL_DEPTH_COMPONENT };

// STATIC: 0x00A2F2AC
static const GLenum s_glType[GxTexFormats_Last] = { 0, 0x8367, 0x8367, 0x8365, 0x8366, 0x8363, 0, 0, 0, GL_BYTE, 0x140B, 0, GL_FLOAT };

struct GlTexEnvOp {
    GLenum m_mode;
    GLfloat m_scale;
    GLenum m_source0;
    GLenum m_source1;
    GLenum m_source2;
    GLenum m_combine;
};

// STATIC: 0x00AD8DF0
static GlTexEnvOp s_texEnvOps[GxTexOps_Last] = {
    { GL_REPLACE, 1.0f, 0x8578, GL_TEXTURE, 0x8576, GL_MODULATE },
    { GL_MODULATE, 2.0f, 0x8578, GL_TEXTURE, 0x8576, GL_MODULATE },
    { GL_ADD, 1.0f, 0x8578, GL_TEXTURE, 0x8576, GL_ADD },
    { GL_REPLACE, 1.0f, 0x8578, GL_TEXTURE, 0x8576, GL_REPLACE },
    { GL_DECAL, 1.0f, 0x8578, GL_TEXTURE, 0x8578, 0x8575 },
    { GL_REPLACE, 1.0f, GL_TEXTURE, 0x8578, 0x8578, 0x8575 },
};

// STATIC: 0x00AD8E9C
static GLfloat s_texEnvColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

// STATIC: 0x00A2E01C
static const GLint s_glAttribSize[7] = { 4, 4, 4, 2, 3, 2, 1 };

// STATIC: 0x00A2E038
static const GLenum s_glAttribType[7] = { GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_FLOAT, GL_FLOAT, GL_SHORT, GL_FLOAT };

// STATIC: 0x00A2E054
static const GLboolean s_glAttribNormalized[7] = { GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };

// STATIC: 0x00AD8D90
static GLenum s_glBlendSrc[12] = { GL_ONE, GL_ONE, GL_SRC_ALPHA, GL_SRC_ALPHA, GL_DST_COLOR, GL_DST_COLOR, GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE, GL_CONSTANT_ALPHA_EXT };

// STATIC: 0x00AD8DC0
static GLenum s_glBlendDst[12] = { GL_ZERO, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO, GL_SRC_COLOR, GL_ONE, GL_ONE, GL_ZERO, GL_ZERO, GL_ONE, GL_ONE_MINUS_CONSTANT_ALPHA_EXT };

// STATIC: 0x00AD8E80
static GLenum s_glDepthFunc[GxCmps_Last] = { GL_LEQUAL, GL_EQUAL, GL_GEQUAL, GL_LESS };

// STATIC: 0x00AD8E90
static GLenum s_glColorMaterial[3] = { GL_AMBIENT_AND_DIFFUSE, GL_SPECULAR, GL_EMISSION };

// STATIC: 0x00AD8EAC
static GLenum s_glPrim[GxPrims_Last] = { GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN };

// STATIC: 0x00C60334
static bool s_dummyContextCreated;

// STATIC: 0x00C60338
static int32_t s_deviceChanging;

// STATIC: 0x00C6030C
static EmergencyMem s_emergencyMem[GxPoolTargets_Last];

// VERIFY: 4da1571e manual
// OFFSET: 0x0068BF20
CGxDevice* CreateCGxDeviceOpenGl() {
    void* mem = SMemAlloc(sizeof(CGxDeviceOpenGl), __FILE__, __LINE__, 0);

    if (!mem) {
        return nullptr;
    }

    return new (mem) CGxDeviceOpenGl();
}

// VERIFY: 522c7eba manual
// OFFSET: 0x0068BBC0
CGxDeviceOpenGl::CGxDeviceOpenGl() {
    this->m_api = GxApi_OpenGl;
    this->m_hwnd = nullptr;
    this->m_ownhwnd = 0;
    this->m_hdc = nullptr;
    this->m_hglrc = nullptr;
    this->m_pbuffer = nullptr;
    this->m_pbufferDC = nullptr;
    this->m_pbufferRC = nullptr;
    this->m_fbo = 0;
    this->m_boundFbo = 0;
    this->unused_3B48 = 6;
    this->unused_3B4C = 0;
    this->unused_3B50 = 0;
    this->m_viewDirty = 0;
    this->m_caps.m_colorFormat = GxCF_rgba;
    this->m_caps.unk_0018 = 0;
    this->m_renderTargetMode = 0;
    this->m_ambientOnlyMode = 0;
    this->DsInit();
    this->unused_3AD0 = 0;
    this->m_materialDirty = 7;
    this->m_sphereMapMode = GL_SPHERE_MAP;

    for (uint32_t target = 0; target < GxPoolTargets_Last; target++) {
        for (uint32_t usage = 0; usage < 3; usage++) {
            this->m_useVbo[target][usage] = 1;
            this->m_useVboBroker[target][usage] = 0;
        }
    }

    this->m_boundTexture = 0xFFFFFFFF;
    this->DeviceCreatePools();
    this->DeviceCreateStreamBufs();
    this->m_glVendor = nullptr;
    this->m_glRenderer = nullptr;
    this->m_glVersion = nullptr;
}

// VERIFY: 5fa67711 auto
// OFFSET: [0x0068BD20, 0x0068BF50]
CGxDeviceOpenGl::~CGxDeviceOpenGl() {
    this->m_glVendor = nullptr;
    this->m_glRenderer = nullptr;
    this->m_glVersion = nullptr;
}

// VERIFY: 7c7555bf manual
// OFFSET: 0x006918C0
void CGxDeviceOpenGl::DsInit() {
    memset(this->m_deviceStates, 0, sizeof(this->m_deviceStates));
    this->m_deviceStates[Ds_DepthMask] = 1;
    this->m_deviceStates[Ds_ColorMask] = 1;
}

// VERIFY: e94dcc5a manual
// OFFSET: 0x006918F0
void CGxDeviceOpenGl::DsSet(EDeviceState state, uint32_t value, int32_t force) {
    uint32_t current = this->m_deviceStates[state];

    if (current == value && !force) {
        return;
    }

    switch (state) {
        case Ds_DepthMask:
            if (!value) {
                glDepthMask(GL_FALSE);
            } else {
                glDepthMask(GL_TRUE);
            }

            break;

        case Ds_ColorMask:
            glColorMask(value & 1, (value & 4) != 0, (value & 2) != 0, (value & 8) != 0);
            break;

        case Ds_ActiveTexture:
            if (static_cast<uint32_t>(this->m_caps.m_numTmus) > 1) {
                glActiveTextureARB(GL_TEXTURE0_ARB + value);
                glClientActiveTextureARB(GL_TEXTURE0_ARB + value);
            }

            break;

        case Ds_TexTarget0 + 0:
        case Ds_TexTarget0 + 1:
        case Ds_TexTarget0 + 2:
        case Ds_TexTarget0 + 3:
        case Ds_TexTarget0 + 4:
        case Ds_TexTarget0 + 5:
        case Ds_TexTarget0 + 6:
        case Ds_TexTarget0 + 7:
            if (static_cast<uint32_t>(state - Ds_TexTarget0) < static_cast<uint32_t>(glMaxTextureUnits)) {
                if (current) {
                    glDisable(current);
                }

                if (value) {
                    glEnable(value);
                }
            }

            break;

        case Ds_TexGenS0 + 0:
        case Ds_TexGenS0 + 1:
        case Ds_TexGenS0 + 2:
        case Ds_TexGenS0 + 3:
        case Ds_TexGenS0 + 4:
        case Ds_TexGenS0 + 5:
        case Ds_TexGenS0 + 6:
        case Ds_TexGenS0 + 7:
            if (!value) {
                glDisable(GL_TEXTURE_GEN_S);
            } else {
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, value);
            }

            if (!current) {
                glEnable(GL_TEXTURE_GEN_S);
            }

            break;

        case Ds_TexGenT0 + 0:
        case Ds_TexGenT0 + 1:
        case Ds_TexGenT0 + 2:
        case Ds_TexGenT0 + 3:
        case Ds_TexGenT0 + 4:
        case Ds_TexGenT0 + 5:
        case Ds_TexGenT0 + 6:
        case Ds_TexGenT0 + 7:
            if (!value) {
                glDisable(GL_TEXTURE_GEN_T);
            } else {
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, value);
            }

            if (!current) {
                glEnable(GL_TEXTURE_GEN_T);
            }

            break;

        case Ds_TexGenR0 + 0:
        case Ds_TexGenR0 + 1:
        case Ds_TexGenR0 + 2:
        case Ds_TexGenR0 + 3:
        case Ds_TexGenR0 + 4:
        case Ds_TexGenR0 + 5:
        case Ds_TexGenR0 + 6:
        case Ds_TexGenR0 + 7:
            if (!value) {
                glDisable(GL_TEXTURE_GEN_R);
            } else {
                glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, value);
            }

            if (!current) {
                glEnable(GL_TEXTURE_GEN_R);
            }

            break;

        case Ds_TexGenQ0 + 0:
        case Ds_TexGenQ0 + 1:
        case Ds_TexGenQ0 + 2:
        case Ds_TexGenQ0 + 3:
        case Ds_TexGenQ0 + 4:
        case Ds_TexGenQ0 + 5:
        case Ds_TexGenQ0 + 6:
        case Ds_TexGenQ0 + 7:
            if (!value) {
                glDisable(GL_TEXTURE_GEN_Q);
            } else {
                glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, value);
            }

            if (!current) {
                glEnable(GL_TEXTURE_GEN_Q);
            }

            break;

        case Ds_ColorOp0 + 0:
        case Ds_ColorOp0 + 1:
        case Ds_ColorOp0 + 2:
        case Ds_ColorOp0 + 3:
        case Ds_ColorOp0 + 4:
        case Ds_ColorOp0 + 5:
        case Ds_ColorOp0 + 6:
        case Ds_ColorOp0 + 7:
            if (static_cast<uint32_t>(state - Ds_ColorOp0) < static_cast<uint32_t>(glMaxTextureUnits)) {
                if (glARBTextureEnvCombine) {
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
                    glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, s_texEnvOps[value].m_scale);
                    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, s_texEnvOps[value].m_source0);
                    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, s_texEnvOps[value].m_source1);
                    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, s_texEnvOps[value].m_source2);
                    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, s_texEnvOps[value].m_combine);
                    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, s_texEnvColor);
                } else {
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, s_texEnvOps[value].m_mode);
                }
            }

            break;

        case Ds_AlphaOp0 + 0:
        case Ds_AlphaOp0 + 1:
        case Ds_AlphaOp0 + 2:
        case Ds_AlphaOp0 + 3:
        case Ds_AlphaOp0 + 4:
        case Ds_AlphaOp0 + 5:
        case Ds_AlphaOp0 + 6:
        case Ds_AlphaOp0 + 7:
            if (static_cast<uint32_t>(state - Ds_AlphaOp0) < static_cast<uint32_t>(glMaxTextureUnits) && glARBTextureEnvCombine) {
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
                glTexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, s_texEnvOps[value].m_scale);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, s_texEnvOps[value].m_source0);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, s_texEnvOps[value].m_source1);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB, s_texEnvOps[value].m_source2);
                glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, s_texEnvOps[value].m_combine);
            }

            break;

        case Ds_VertexArray0 + 0:
        case Ds_VertexArray0 + 1:
        case Ds_VertexArray0 + 2:
        case Ds_VertexArray0 + 3:
        case Ds_VertexArray0 + 4:
        case Ds_VertexArray0 + 5:
        case Ds_TexCoordArray0 + 0:
        case Ds_TexCoordArray0 + 1:
        case Ds_TexCoordArray0 + 2:
        case Ds_TexCoordArray0 + 3:
        case Ds_TexCoordArray0 + 4:
        case Ds_TexCoordArray0 + 5:
        case Ds_TexCoordArray0 + 6:
        case Ds_TexCoordArray0 + 7:
            if (this->m_deviceStates[Ds_VertexProgram]) {
                GLuint index = state - Ds_VertexArray0;

                if (!value) {
                    glDisableVertexAttribArrayARB(index);
                } else {
                    glEnableVertexAttribArrayARB(index);
                }
            } else {
                GLenum clientStates[14] = { GL_VERTEX_ARRAY, 0, 0, GL_NORMAL_ARRAY, GL_COLOR_ARRAY, GL_COLOR_ARRAY, GL_TEXTURE_COORD_ARRAY, GL_TEXTURE_COORD_ARRAY, GL_TEXTURE_COORD_ARRAY, GL_TEXTURE_COORD_ARRAY, GL_TEXTURE_COORD_ARRAY, GL_TEXTURE_COORD_ARRAY, GL_TEXTURE_COORD_ARRAY, GL_TEXTURE_COORD_ARRAY };
                GLenum clientState = clientStates[state - Ds_VertexArray0];

                if (!value) {
                    glDisableClientState(clientState);
                } else {
                    glEnableClientState(clientState);
                }
            }

            break;

        case Ds_PolygonOffsetFill:
            if (!value) {
                glDisable(GL_POLYGON_OFFSET_FILL);
            } else {
                glEnable(GL_POLYGON_OFFSET_FILL);
            }

            break;

        case Ds_PolygonOffset:
            glPolygonOffset(-4.0f, reinterpret_cast<float&>(value) * 65536.0f * -1.0f);
            break;

        case Ds_Blend:
            if (value) {
                glEnable(GL_BLEND);
            } else {
                glDisable(GL_BLEND);
            }

            break;

        case Ds_AlphaTest:
            if (value) {
                glEnable(GL_ALPHA_TEST);
            } else {
                glDisable(GL_ALPHA_TEST);
            }

            break;

        case Ds_RegisterCombiners:
            if (glNVRegisterCombiners) {
                if (value) {
                    glEnable(GL_REGISTER_COMBINERS_NV);
                } else {
                    glDisable(GL_REGISTER_COMBINERS_NV);
                }
            }

            break;

        case Ds_PerStageConstants:
            if (glNVRegisterCombiners2) {
                if (value) {
                    glEnable(GL_PER_STAGE_CONSTANTS_NV);
                } else {
                    glDisable(GL_PER_STAGE_CONSTANTS_NV);
                }
            }

            break;

        case Ds_TextureShader:
            if (glNVTextureShader) {
                if (value) {
                    glEnable(GL_TEXTURE_SHADER_NV);
                } else {
                    glDisable(GL_TEXTURE_SHADER_NV);
                }
            }

            break;

        case Ds_FragmentProgram:
            if (glARBFragmentProgram) {
                if (value) {
                    glEnable(GL_FRAGMENT_PROGRAM_ARB);
                } else {
                    glDisable(GL_FRAGMENT_PROGRAM_ARB);
                }
            }

            break;

        case Ds_VertexProgram:
            if (glARBVertexProgram) {
                for (uint32_t i = Ds_VertexArray0; i < Ds_TexCoordArray0; i++) {
                    this->DsSet(static_cast<EDeviceState>(i), 0, 0);
                }

                for (uint32_t tmu = 0; tmu < static_cast<uint32_t>(this->m_caps.m_numTmus); tmu++) {
                    this->DsSet(Ds_ActiveTexture, tmu, 0);
                    this->DsSet(static_cast<EDeviceState>(Ds_TexCoordArray0 + tmu), 0, 0);
                }

                this->m_primVertexDirty = 0xFFFFFFFF;

                if (!value) {
                    glDisable(GL_VERTEX_PROGRAM_ARB);
                } else {
                    glEnable(GL_VERTEX_PROGRAM_ARB);
                }
            }

            break;

        case Ds_MatrixMode:
            glMatrixMode(value);
            break;

        case Ds_BlendFunc:
            glBlendFunc(value >> 16, value & 0xFFFF);
            break;

        case Ds_ArrayBuffer:
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, value);
            break;

        case Ds_ElementArrayBuffer:
            glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, value);
            break;

        case Ds_CullFace:
            glCullFace(value);
            break;

        case Ds_ClipPlanes: {
            uint32_t changed = value ^ current;

            for (uint32_t plane = 0; plane < 6; plane++) {
                if (changed & (1 << plane)) {
                    if ((1 << plane) & value) {
                        glEnable(GL_CLIP_PLANE0 + plane);
                    } else {
                        glDisable(GL_CLIP_PLANE0 + plane);
                    }
                }
            }

            break;
        }

        default:
            break;
    }

    this->m_deviceStates[state] = value;
}

// VERIFY: 7ff52200 auto
// OFFSET: 0x0068A7F0
void CGxDeviceOpenGl::DeviceOverride(EGxOverride override, uint32_t value) {
    CGxDevice::DeviceOverride(override, value);

    switch (override) {
        case 0:
            this->m_caps.m_shaderTargets[GxSh_Pixel] = value;
            break;

        case 1:
            if (!value) {
                glARBVertexBufferObject = 0;
            }

            break;

        case 2:
            if (!value) {
                this->m_useVbo[0][2] = 0;
            }

            break;

        case 3:
            if (!value) {
                this->m_useVbo[1][2] = 0;
            }

            break;

        case 4:
            if (value) {
                this->m_useVboBroker[0][2] = 1;
            }

            break;

        case 5:
            if (value) {
                this->m_useVboBroker[1][2] = 1;
            }

            break;

        case 6:
            this->m_sphereMapMode = value != 0 ? GL_REFLECTION_MAP_ARB : GL_SPHERE_MAP;
            break;

        case 7:
            this->m_caps.m_texNonPow2 = value != 0;
            break;

        default:
            break;
    }
}

// VERIFY: c2d6b7a6 auto
// OFFSET: 0x0068A8E0
void CGxDeviceOpenGl::LogCrashInfo(char* buffer, uint32_t size) {
    char line[260];

    if (this->m_glVendor) {
        SStrPrintf(line, sizeof(line), "Vendor: %s\r\n", this->m_glVendor);
        SStrPack(buffer, line, size);
    }

    if (this->m_glRenderer) {
        SStrPrintf(line, sizeof(line), "Renderer: %s\r\n", this->m_glRenderer);
        SStrPack(buffer, line, size);
    }

    if (this->m_glVersion) {
        SStrPrintf(line, sizeof(line), "Version: %s\r\n", this->m_glVersion);
        SStrPack(buffer, line, size);
    }
}

// VERIFY: f95fc093 manual
// OFFSET: 0x0068A9A0
void CGxDeviceOpenGl::ISetCaps(const CGxFormat& format) {
    this->m_caps.m_numTmus = glMaxTextureUnits >= 8 ? 8 : glMaxTextureUnits;

    if (glARBFragmentProgram) {
        this->m_caps.m_numTmus = glMaxTextureImageUnitsARB >= 8 ? 8 : glMaxTextureImageUnitsARB;
    }

    this->m_caps.m_pixelCenterOnEdge = 1;
    this->m_caps.m_texelCenterOnEdge = 1;

    GLint maxSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    this->m_caps.m_texMaxSize[GxTex_2d] = maxSize < 256 ? 256 : maxSize;
    this->m_caps.m_texTarget[GxTex_2d] = 1;

    maxSize = 0;
    GLint cubeSize;

    if (!glARBTextureCubeMap || (glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &maxSize), cubeSize = maxSize, maxSize < 256)) {
        cubeSize = 256;
    }

    this->m_caps.m_texMaxSize[GxTex_CubeMap] = cubeSize;

    GLint rectangleSize = 0;
    this->m_caps.m_texTarget[GxTex_CubeMap] = glARBTextureCubeMap;
    maxSize = 0;

    GLint rectangleCap;

    if (glARBTextureRectangle && (glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB, &maxSize), rectangleSize = maxSize, maxSize >= 256)) {
        rectangleCap = maxSize;
    } else {
        rectangleCap = 256;
    }

    this->m_caps.m_texMaxSize[GxTex_Rectangle] = rectangleCap;
    this->m_caps.m_texTarget[GxTex_Rectangle] = glARBTextureRectangle;
    this->m_caps.m_texNonPow2 = 1;

    if (glARBFragmentProgram || glNVTextureShader || glNVTextureShader2 || glNVTextureShader3) {
        this->m_caps.m_texNonPow2 = 0;
    }

    if (rectangleSize < 256) {
        rectangleSize = 256;
    }

    this->m_caps.m_texMaxSize[GxTex_NonPow2] = rectangleSize;
    this->m_caps.m_texTarget[GxTex_NonPow2] = 0;
    this->m_caps.m_texFmt[GxTex_Dxt1] = glExtTextureCompressionS3tc;
    this->m_caps.m_texFmt[GxTex_Dxt3] = glExtTextureCompressionS3tc;
    this->m_caps.m_texFmt[GxTex_Dxt5] = glExtTextureCompressionS3tc;
    this->m_caps.m_texFmt[GxTex_Uv88] = glNVTextureShader;

    GLint numStreams = this->m_caps.m_numTmus + 3;

    if (glARBVertexProgram) {
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS_ARB, &numStreams);
    }

    this->m_caps.m_numStreams = numStreams;
    this->m_caps.m_streamOffset = 1;
    this->m_caps.m_generateMipMaps = glSGISGenerateMipmap;

    for (uint32_t i = 0; i < 6; i++) {
        this->m_caps.m_shaderTargets[i] = 0;
        this->m_caps.m_shaderConstants[i] = 0;
    }

    int32_t vertexTargets[12];
    memset(&vertexTargets[1], 0, sizeof(int32_t) * 11);
    vertexTargets[0] = 1;

    if (glARBVertexProgram) {
        vertexTargets[6] = 1;
    }

    DebugLog("Vertex Shader Support = ");

    for (uint32_t i = 0; i < 12; i++) {
        if (vertexTargets[i]) {
            DebugLog(i ? ", %s" : "%s", CGxDevice::s_vertexShaderTargetNames[i]);
        }
    }

    DebugLog("\n");

    int32_t pixelTargets[14];
    memset(&pixelTargets[1], 0, sizeof(int32_t) * 13);
    pixelTargets[0] = 1;

    if (glNVRegisterCombiners) {
        pixelTargets[7] = 1;
    }

    if (glNVRegisterCombiners2) {
        if (glNVTextureShader) {
            pixelTargets[8] = 1;
        }

        if (glNVTextureShader2) {
            pixelTargets[9] = 1;
        }

        if (glNVTextureShader3) {
            pixelTargets[10] = 1;
        }
    }

    if (glARBFragmentProgram) {
        pixelTargets[12] = 1;
    }

    DebugLog("Pixel Shader Support = ");

    for (uint32_t i = 0; i < 14; i++) {
        if (pixelTargets[i]) {
            DebugLog(i ? ", %s" : "%s", CGxDevice::s_pixelShaderTargetNames[i]);
        }
    }

    DebugLog("\n");

    int32_t pixelTarget = format.m_fixedFunction[GxSh_Pixel];

    if (pixelTarget == -1 || !pixelTargets[pixelTarget]) {
        if (pixelTargets[12]) {
            this->m_caps.m_shaderTargets[GxSh_Pixel] = 12;
        } else if (pixelTargets[10]) {
            this->m_caps.m_shaderTargets[GxSh_Pixel] = 10;
        } else if (pixelTargets[9]) {
            this->m_caps.m_shaderTargets[GxSh_Pixel] = 9;
        } else if (pixelTargets[8]) {
            this->m_caps.m_shaderTargets[GxSh_Pixel] = 8;
        } else if (pixelTargets[7]) {
            this->m_caps.m_shaderTargets[GxSh_Pixel] = 7;
        }
    } else {
        this->m_caps.m_shaderTargets[GxSh_Pixel] = pixelTarget;
    }

    int32_t vertexTarget = format.m_fixedFunction[GxSh_Vertex];

    if (vertexTarget == -1 || !vertexTargets[vertexTarget]) {
        if (this->m_caps.m_shaderTargets[GxSh_Pixel]) {
            if (vertexTargets[6]) {
                this->m_caps.m_shaderTargets[GxSh_Vertex] = 6;
            } else if (vertexTargets[7]) {
                this->m_caps.m_shaderTargets[GxSh_Vertex] = 7;
            } else if (vertexTargets[9]) {
                this->m_caps.m_shaderTargets[GxSh_Vertex] = 9;
            } else if (vertexTargets[8]) {
                this->m_caps.m_shaderTargets[GxSh_Vertex] = 8;
            }
        }
    } else {
        this->m_caps.m_shaderTargets[GxSh_Vertex] = vertexTarget;
    }

    switch (this->m_caps.m_shaderTargets[GxSh_Vertex]) {
        case 6:
        case 7: {
            GLint envParameters;
            GLint nativeParameters;
            glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &envParameters);
            glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &nativeParameters);
            this->m_caps.m_shaderConstants[GxSh_Vertex] = envParameters >= nativeParameters ? nativeParameters : envParameters;
            break;
        }

        case 8:
            this->m_caps.m_shaderConstants[GxSh_Vertex] = 96;
            break;

        case 9:
            this->m_caps.m_shaderConstants[GxSh_Vertex] = 256;
            break;

        default:
            break;
    }

    this->m_caps.m_texFilterTrilinear = 1;

    if (glExtTextureFilterAnisotropic) {
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, reinterpret_cast<GLint*>(&this->m_caps.m_maxTexAnisotropy));

        if (this->m_caps.m_maxTexAnisotropy > 1) {
            this->m_caps.m_texFilterAnisotropic = 1;
        }
    }

    this->m_caps.m_colorWrite = 1;
    glGetIntegerv(GL_MAX_CLIP_PLANES, reinterpret_cast<GLint*>(&this->m_caps.m_maxClipPlanes));

    if (this->m_caps.m_shaderTargets[GxSh_Vertex] == 6 || this->m_caps.m_shaderTargets[GxSh_Vertex] == 7) {
        this->m_caps.m_maxClipPlanes = 0;
    }

    this->m_caps.m_occlusionQuery = glARBOcclusionQuery;
    this->m_caps.m_hwCursor = 0;
    this->m_caps.m_pointScaleMax = 1.0f;
    this->m_caps.m_pointParameters = glARBPointParameters;

    if (glARBPointParameters) {
        GLfloat pointSizeMax;
        glGetFloatv(GL_POINT_SIZE_MAX_ARB, &pointSizeMax);
        this->m_caps.m_pointScaleMax = pointSizeMax;
    }

    this->m_caps.m_pointSprite = glARBPointSprite;

    if (!(this->m_caps.m_pointScaleMax > 1.0f)) {
        this->m_caps.m_pointParameters = 0;
        this->m_caps.m_pointSprite = 0;
    }

    this->m_caps.m_shaderAlphaRef = 1;
    this->m_caps.unk_0134 = 1;
    this->m_caps.unused_0114[1] = 0;
    this->m_caps.unused_0114[0] = 0;
    this->m_caps.unused_0114[2] = 0;
    this->m_caps.unused_0114[3] = 0;
    this->m_caps.unused_0114[4] = 0;
    this->m_caps.m_stereoAvailable = 0;
    this->m_caps.unk_0138 = this->m_caps.m_shaderTargets[GxSh_Pixel] != 12;
    this->Log(this->m_caps);
}

// VERIFY: 425ca66b manual
// OFFSET: 0x0068AF40
void CGxDeviceOpenGl::DestroyPoolAPI(CGxPool* pool) {
    if (!pool) {
        return;
    }

    pool->Invalidate();

    if (!pool->m_apiSpecific) {
        return;
    }

    if (glARBVertexBufferObject) {
        if (this->m_useVboBroker[pool->m_target][pool->m_usage]) {
            EDeviceState state = s_bufferState[pool->m_target];

            if (this->m_deviceStates[state] == static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pool->m_apiSpecific))) {
                this->DsSet(state, 0, 0);
            }

            if (pool->m_apiSpecific) {
                delete static_cast<CGxVboBroker*>(pool->m_apiSpecific);
            }

            pool->m_apiSpecific = nullptr;
            return;
        }

        if (this->m_useVbo[pool->m_target][pool->m_usage]) {
            EDeviceState state = s_bufferState[pool->m_target];

            if (this->m_deviceStates[state] == static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pool->m_apiSpecific))) {
                this->DsSet(state, 0, 0);
            }

            glDeleteBuffersARB(1, reinterpret_cast<GLuint*>(&pool->m_apiSpecific));
            pool->m_apiSpecific = nullptr;
            return;
        }
    }

    SMemFree(pool->m_apiSpecific, __FILE__, __LINE__, 0);
    pool->m_apiSpecific = nullptr;
}

// VERIFY: bc5e44a9 auto
// OFFSET: 0x0068B030
void CGxDeviceOpenGl::PoolDestroy(CGxPool* pool) {
    this->DestroyPoolAPI(pool);
    CGxDevice::PoolDestroy(pool);
}

// VERIFY: 63dd8882 auto
// OFFSET: 0x0068B050
void CGxDeviceOpenGl::QueryCreate(CGxQuery*& query, EGxQueryType type) {
    CGxDevice::QueryCreate(query, type);

    CGxQuery* created = query;

    if (!created->m_apiSpecific && glARBOcclusionQuery) {
        GLuint id;
        glGenQueriesARB(1, &id);
        created->m_apiSpecific = reinterpret_cast<void*>(static_cast<uintptr_t>(id));
    }
}

// VERIFY: 7ae0a306 auto
// OFFSET: 0x0068B090
void CGxDeviceOpenGl::QueryDestroy(CGxQuery*& query) {
    CGxQuery* destroyed = query;

    if (destroyed->m_apiSpecific) {
        GLuint id = static_cast<GLuint>(reinterpret_cast<uintptr_t>(destroyed->m_apiSpecific));
        glDeleteQueriesARB(1, &id);
        destroyed->m_apiSpecific = nullptr;
    }

    CGxDevice::QueryDestroy(query);
}

// VERIFY: 50f62f95 manual
// OFFSET: 0x0068B0D0
bool CGxDeviceOpenGl::QueryBegin(CGxQuery* query) {
    if (!this->m_context) {
        return false;
    }

    if (!query->m_apiSpecific) {
        if (glARBOcclusionQuery) {
            GLuint id;
            glGenQueriesARB(1, &id);
            query->m_apiSpecific = reinterpret_cast<void*>(static_cast<uintptr_t>(id));
        }

        if (!query->m_apiSpecific) {
            return false;
        }
    }

    glBeginQueryARB(s_glQueryTarget[query->m_type], static_cast<GLuint>(reinterpret_cast<uintptr_t>(query->m_apiSpecific)));
    return glGetError() == GL_NO_ERROR;
}

// VERIFY: ca7aaa9a manual
// OFFSET: 0x0068B140
bool CGxDeviceOpenGl::QueryEnd(CGxQuery* query) {
    if (!this->m_context || !query->m_apiSpecific) {
        return false;
    }

    glEndQueryARB(s_glQueryTarget[query->m_type]);
    return glGetError() == GL_NO_ERROR;
}

// VERIFY: 86f9ceda manual
// OFFSET: 0x0068B180
bool CGxDeviceOpenGl::QueryGetParam(CGxQuery* query, EGxQueryParam param, uint32_t& data) {
    if (!this->m_context || !query->m_apiSpecific) {
        return false;
    }

    GLenum error = GL_INVALID_OPERATION;

    if (param == 0) {
        GLint bits;
        glGetQueryivARB(s_glQueryTarget[query->m_type], GL_QUERY_COUNTER_BITS_ARB, &bits);
        error = glGetError();

        if (!error) {
            data = bits / 8;
        }
    } else if (param == 1) {
        GLuint available;
        glGetQueryObjectuivARB(static_cast<GLuint>(reinterpret_cast<uintptr_t>(query->m_apiSpecific)), GL_QUERY_RESULT_AVAILABLE_ARB, &available);
        error = glGetError();

        if (!error) {
            data = available;
            return true;
        }
    }

    return error == GL_NO_ERROR;
}

// VERIFY: c2035dcc manual
// OFFSET: 0x0068B230
bool CGxDeviceOpenGl::QueryGetData(CGxQuery* query, uint32_t& data) {
    if (!this->m_context || !query->m_apiSpecific) {
        return false;
    }

    glGetQueryObjectuivARB(static_cast<GLuint>(reinterpret_cast<uintptr_t>(query->m_apiSpecific)), GL_QUERY_RESULT_ARB, reinterpret_cast<GLuint*>(&data));
    GLenum error = glGetError();

    if (!error && !query->m_type) {
        data /= this->m_format.m_multisampleCount;
    }

    return error == GL_NO_ERROR;
}

// VERIFY: 0de7dfcb manual
// OFFSET: 0x0068B300
void CGxDeviceOpenGl::CreatePoolAPI(CGxPool* pool) {
    if (glARBVertexBufferObject) {
        if (this->m_useVboBroker[pool->m_target][pool->m_usage]) {
            uint32_t maxBits = 0;

            if (pool->m_target == GxPoolTarget_Vertex) {
                maxBits = 21;
            } else if (pool->m_target == GxPoolTarget_Index) {
                maxBits = 18;
            }

            void* mem = SMemAlloc(sizeof(CGxVboBroker1), __FILE__, __LINE__, 0);
            pool->m_apiSpecific = mem ? new (mem) CGxVboBroker1(s_glBufferTarget[pool->m_target], maxBits, this) : nullptr;
            return;
        }

        if (this->m_useVbo[pool->m_target][pool->m_usage]) {
            if (this->m_context) {
                glGenBuffersARB(1, reinterpret_cast<GLuint*>(&pool->m_apiSpecific));

                if (pool->m_apiSpecific) {
                    this->DsSet(s_bufferState[pool->m_target], static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pool->m_apiSpecific)), 0);
                    glBufferDataARB(s_glBufferTarget[pool->m_target], pool->m_size, nullptr, s_glBufferUsage[pool->m_usage]);
                }
            }

            return;
        }
    }

    pool->m_apiSpecific = SMemAlloc(pool->m_size, __FILE__, __LINE__, 0);
}

// VERIFY: 9ecd761f manual
// OFFSET: 0x0068B410
void CGxDeviceOpenGl::PoolSizeSet(CGxPool* pool, uint32_t size) {
    if (glARBVertexBufferObject) {
        if (this->m_useVboBroker[pool->m_target][pool->m_usage]) {
            pool->Invalidate();
            pool->m_size = size;
            return;
        }

        if (this->m_useVbo[pool->m_target][pool->m_usage]) {
            this->DsSet(s_bufferState[pool->m_target], static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pool->m_apiSpecific)), 0);
            glBufferDataARB(s_glBufferTarget[pool->m_target], size, nullptr, s_glBufferUsage[pool->m_usage]);
            pool->Invalidate();
            pool->m_size = size;
            return;
        }
    }

    if (pool->m_apiSpecific) {
        SMemFree(pool->m_apiSpecific, __FILE__, __LINE__, 0);
    }

    pool->m_apiSpecific = SMemAlloc(size, __FILE__, __LINE__, 0);
    pool->Invalidate();
    pool->m_size = size;
}

// VERIFY: dca42694 manual
// OFFSET: 0x0068B4D0
char* CGxDeviceOpenGl::BufLock(CGxBuf* buf) {
    CGxDevice::BufLock(buf);

    CGxPool* pool = buf->m_pool;

    if (!pool->m_apiSpecific) {
        this->CreatePoolAPI(pool);
    }

    char* mem = static_cast<char*>(pool->m_apiSpecific);

    if (!mem) {
        mem = static_cast<char*>(this->m_emergencyMem[pool->m_target].Lock(buf->m_size));
        pool->m_mem = mem;
        return mem;
    }

    if (glARBVertexBufferObject) {
        if (this->m_useVboBroker[pool->m_target][pool->m_usage]) {
            GLuint vbo;
            uint32_t offset;
            static_cast<CGxVboBroker*>(pool->m_apiSpecific)->SelectVbo(buf->m_size, vbo, offset);
            this->DsSet(s_bufferState[pool->m_target], vbo, 0);
            pool->m_mem = glMapBufferARB(s_glBufferTarget[pool->m_target], GL_WRITE_ONLY_ARB);
            buf->m_index = offset;
            return static_cast<char*>(pool->m_mem) + offset;
        }

        if (this->m_useVbo[pool->m_target][pool->m_usage]) {
            this->DsSet(s_bufferState[pool->m_target], static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mem)), 0);

            if (pool->m_usage == GxPoolUsage_Stream) {
                glBufferDataARB(s_glBufferTarget[pool->m_target], buf->m_size, nullptr, GL_STREAM_DRAW_ARB);
                mem = static_cast<char*>(s_emergencyMem[pool->m_target].Lock(buf->m_size));
                pool->m_mem = mem;
                buf->m_index = 0;
                return mem;
            }

            if (pool->m_usage <= GxPoolUsage_Dynamic) {
                mem = static_cast<char*>(s_emergencyMem[pool->m_target].Lock(buf->m_size));
                pool->m_mem = mem;
                return mem;
            }

            return nullptr;
        }
    }

    if (pool->m_usage == GxPoolUsage_Stream) {
        pool->m_mem = mem;
        buf->m_index = 0;
        return mem;
    }

    if (pool->m_usage <= GxPoolUsage_Dynamic) {
        pool->m_mem = mem;
        return mem + buf->m_index;
    }

    return nullptr;
}

// VERIFY: f0fb3ec5 manual
// OFFSET: 0x0068B670
void CGxDeviceOpenGl::BufData(CGxBuf* buf, const void* data, uint32_t size, uint32_t offset) {
    CGxDevice::BufData(buf, data, size, offset);

    CGxPool* pool = buf->m_pool;

    if (!pool->m_apiSpecific) {
        this->CreatePoolAPI(pool);
    }

    if (glARBVertexBufferObject) {
        if (this->m_useVboBroker[pool->m_target][pool->m_usage]) {
            GLuint vbo;
            uint32_t vboOffset;
            static_cast<CGxVboBroker*>(pool->m_apiSpecific)->SelectVbo(size, vbo, vboOffset);
            this->DsSet(s_bufferState[pool->m_target], vbo, 0);
            glBufferSubDataARB(s_glBufferTarget[pool->m_target], vboOffset, size, data);
            buf->m_index = vboOffset;
            return;
        }

        if (this->m_useVbo[pool->m_target][pool->m_usage]) {
            this->DsSet(s_bufferState[pool->m_target], static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pool->m_apiSpecific)), 0);

            if (pool->m_usage == GxPoolUsage_Stream) {
                glBufferDataARB(s_glBufferTarget[pool->m_target], size, data, GL_STREAM_DRAW_ARB);
                buf->m_index = 0;
            } else if (pool->m_usage <= GxPoolUsage_Dynamic) {
                if (size == static_cast<uint32_t>(pool->m_size)) {
                    glBufferDataARB(s_glBufferTarget[pool->m_target], size, data, s_glBufferUsage[pool->m_usage]);
                } else {
                    glBufferSubDataARB(s_glBufferTarget[pool->m_target], offset + buf->m_index, size, data);
                }
            }

            return;
        }
    }

    if (pool->m_usage == GxPoolUsage_Stream) {
        buf->m_index = 0;
        memcpy(static_cast<char*>(pool->m_apiSpecific) + offset, data, size);
    } else if (pool->m_usage <= GxPoolUsage_Dynamic) {
        memcpy(static_cast<char*>(pool->m_apiSpecific) + offset + buf->m_index, data, size);
    }
}

// VERIFY: 16e5c68a manual
// OFFSET: 0x0068B820
void CGxDeviceOpenGl::IQueryRelease() {
    for (CGxQuery* query = this->m_queryList.Head(); query; query = this->m_queryList.Next(query)) {
        if (query->m_apiSpecific) {
            GLuint id = static_cast<GLuint>(reinterpret_cast<uintptr_t>(query->m_apiSpecific));
            glDeleteQueriesARB(1, &id);
            query->m_apiSpecific = nullptr;
        }
    }
}

// VERIFY: e45295d9 manual
// OFFSET: 0x0068B880
void CGxDeviceOpenGl::IDeviceSetRenderTargetFBO(EGxBuffer buffer, CGxTex* texture, uint32_t plane) {
    CGxTex* current = this->m_textureTarget[buffer].m_texture;

    if (current && current->m_flags.m_bit8) {
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);
    }

    if (buffer == GxBuffers_Depth && !texture) {
        if (!this->m_boundFbo) {
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, this->m_fbo);
        }

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);

        if (!this->m_boundFbo) {
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        }
    }

    CGxDevice::DeviceSetRenderTarget(buffer, texture, plane);

    if (texture) {
        this->m_textureTarget[buffer].m_apiSpecific = texture;

        if (!texture->m_apiSpecificData) {
            glGenTextures(1, reinterpret_cast<GLuint*>(&texture->m_apiSpecificData));
        }

        if (texture->m_needsCreation) {
            this->BindTexture(texture, kNullTmu);

            GLenum target = s_glTexTarget[texture->m_target];
            glTexImage2D(target, 0, s_glInternalFormat[texture->m_format], texture->m_width, texture->m_height, 0, s_glFormat[texture->m_format], s_glType[texture->m_format], nullptr);

            if (s_glFormat[texture->m_format] == GL_DEPTH_COMPONENT) {
                glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
                glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
                glTexParameteri(target, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
            }

            texture->m_needsCreation = 0;
        }

        if (!this->m_fbo) {
            glGenFramebuffersEXT(1, &this->m_fbo);
        }

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, this->m_fbo);
        this->m_boundFbo = this->m_fbo;

        GLuint name = static_cast<GLuint>(reinterpret_cast<uintptr_t>(texture->m_apiSpecificData));
        GLenum target = s_glTexTarget[texture->m_target];

        if (buffer) {
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, target, name, 0);
        } else {
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, target, name, 0);

            if (texture->m_flags.m_bit8) {
                if (!texture->m_apiSpecificData2) {
                    GLuint renderbuffer = 0;
                    glGenRenderbuffersEXT(1, &renderbuffer);
                    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer);
                    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24_ARB, texture->m_width, texture->m_height);
                    texture->m_apiSpecificData2 = reinterpret_cast<void*>(static_cast<uintptr_t>(renderbuffer));
                }

                glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, static_cast<GLuint>(reinterpret_cast<uintptr_t>(texture->m_apiSpecificData2)));
            }
        }

        CRect rect;
        rect.minY = 0.0f;
        rect.minX = 0.0f;
        rect.maxY = static_cast<float>(texture->m_height);
        rect.maxX = static_cast<float>(texture->m_width);
        this->DeviceSetCurWindow(rect);
    } else {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        this->m_boundFbo = 0;
        this->DeviceSetCurWindow(this->DeviceDefWindow());
        this->m_textureTarget[buffer].m_apiSpecific = nullptr;
    }

    GxXformSetViewport(this->m_viewport.m_x.m_l, this->m_viewport.m_x.m_h, this->m_viewport.m_y.m_l, this->m_viewport.m_y.m_h, this->m_viewport.m_z.m_l, this->m_viewport.m_z.m_h);
    g_theGxDevicePtr->XformSetProjection(this->m_projection);
}

// VERIFY: 57fb30e2 manual
// OFFSET: 0x0068BB90
void CGxDeviceOpenGl::DeviceSetRenderTarget(EGxBuffer buffer, CGxTex* texture, uint32_t plane) {
    if (this->m_renderTargetMode == 0) {
        this->IDeviceSetRenderTargetCopy(buffer, texture, plane);
    } else if (this->m_renderTargetMode == 1) {
        this->IDeviceSetRenderTargetFBO(buffer, texture, plane);
    }
}

// VERIFY: 880640c2 manual
// OFFSET: 0x0068BDD0
int32_t CGxDeviceOpenGl::BufUnlock(CGxBuf* buf, uint32_t size) {
    CGxDevice::BufUnlock(buf, size);

    CGxPool* pool = buf->m_pool;
    int32_t result = 1;

    if (!pool->m_apiSpecific) {
        this->m_emergencyMem[pool->m_target].Unlock();
        buf->m_valid = 0;
        this->m_intF5C = 1;
        pool->m_mem = nullptr;
        return 0;
    }

    if (glARBVertexBufferObject) {
        if (this->m_useVboBroker[pool->m_target][pool->m_usage]) {
            static_cast<CGxVboBroker*>(pool->m_apiSpecific)->UnlockVbo(buf->m_index, size);
            result = glUnmapBufferARB(s_glBufferTarget[pool->m_target]);
            buf->m_valid = result;
            pool->m_mem = nullptr;
            return result;
        }

        if (this->m_useVbo[pool->m_target][pool->m_usage]) {
            EGxPoolTarget target = pool->m_target;

            if (this->m_emergencyMem[target].m_lock) {
                this->m_emergencyMem[target].Unlock();
                result = 0;
                this->m_intF5C = 1;
            } else if (s_emergencyMem[target].m_lock) {
                glBufferSubDataARB(s_glBufferTarget[target], buf->m_index, buf->m_size, s_emergencyMem[target].m_data.m_data);
                s_emergencyMem[pool->m_target].Unlock();
            }

            buf->m_valid = result;
        }
    }

    pool->m_mem = nullptr;
    return result;
}

// VERIFY: a7eda8bb manual
// OFFSET: 0x0068BF80
void CGxDeviceOpenGl::DeviceReadPixels(CiRect& rect, TSGrowableArray<CImVector>& pixels) {
    this->ClampRectToWindow(rect);

    int32_t height = rect.maxY - rect.minY;
    int32_t width = rect.maxX - rect.minX;
    pixels.SetCount(width * height);

    const CRect& window = this->DeviceCurWindow();
    glReadPixels(rect.minX, static_cast<GLint>(window.maxY - rect.maxY), width, height, 0x80E1, GL_UNSIGNED_BYTE, pixels.Ptr());

    uint32_t pitch = sizeof(CImVector) * width;
    CImVector* row = static_cast<CImVector*>(alloca(pitch));

    for (int32_t i = 0; i < width; i++) {
        new (&row[i]) CImVector();
    }

    CImVector* top = pixels.Ptr();
    CImVector* bottom = top + width * (height - 1);

    for (int32_t i = height / 2; i; i--) {
        memcpy(row, top, pitch);
        memcpy(top, bottom, pitch);
        memcpy(bottom, row, pitch);
        bottom -= width;
        top += width;
    }
}

// VERIFY: 547d88c5 manual
// OFFSET: 0x0068C070
void CGxDeviceOpenGl::DeviceReadDepths(CiRect& rect, TSGrowableArray<float>& depths) {
    this->ClampRectToWindow(rect);

    int32_t width = rect.maxX - rect.minX;
    int32_t height = rect.maxY - rect.minY;
    depths.SetCount(width * height);

    const CRect& window = this->DeviceCurWindow();
    glReadPixels(rect.minX, static_cast<GLint>(window.maxY - rect.maxY), width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depths.Ptr());

    float* top = depths.Ptr();
    float* bottom = top + width * (height - 1);

    for (int32_t i = height / 2; i; i--) {
        for (int32_t j = 0; j < width; j++) {
            float value = top[j];
            top[j] = bottom[j];
            bottom[j] = value;
        }

        bottom -= width;
        top += width;
    }
}

// VERIFY: 93898654 manual
// OFFSET: 0x0068C1F0
void CGxDeviceOpenGl::DeviceDestroyPoolAPI() {
    for (CGxPool* pool = this->m_poolList.Head(); !this->m_poolList.IsEnd(pool); pool = this->m_poolList.RawNext(pool)) {
        this->DestroyPoolAPI(pool);
    }
}

// VERIFY: 63ba8840 manual
// OFFSET: 0x0068C670
static void CreateDummyContext() {
    if (s_dummyContextCreated) {
        return;
    }

    s_dummyContextCreated = true;

    HINSTANCE instance = GetModuleHandleA(nullptr);

    WNDCLASSEXA windowClass;
    memset(&windowClass, 0, sizeof(windowClass));
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_OWNDC;
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "hwacc";

    ATOM atom = RegisterClassExA(&windowClass);

    if (!atom) {
        return;
    }

    HWND window = CreateWindowExA(0, reinterpret_cast<LPCSTR>(static_cast<uintptr_t>(atom)), "hwacc", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, nullptr, nullptr);

    if (window) {
        HDC dc = GetDC(window);

        if (dc) {
            int32_t count = DescribePixelFormat(dc, 0, 0, nullptr);

            PIXELFORMATDESCRIPTOR pfd;
            memset(&pfd.dwFlags, 0, sizeof(pfd) - offsetof(PIXELFORMATDESCRIPTOR, dwFlags));
            pfd.nSize = sizeof(pfd);
            pfd.nVersion = 1;

            for (int32_t i = 1; i <= count; i++) {
                DescribePixelFormat(dc, i, sizeof(pfd), &pfd);

                if ((pfd.dwFlags & (PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL)) && !(pfd.dwFlags & (PFD_GENERIC_FORMAT | PFD_SUPPORT_GDI))) {
                    SetPixelFormat(dc, i, &pfd);

                    HGLRC context = wglCreateContext(dc);

                    if (context) {
                        wglMakeCurrent(dc, context);
                        BindGlExtensions();
                        BindWglExtensions();
                        wglMakeCurrent(nullptr, nullptr);
                    }

                    wglDeleteContext(context);
                    break;
                }
            }

            ReleaseDC(window, dc);
        }

        DestroyWindow(window);
    }

    UnregisterClassA(reinterpret_cast<LPCSTR>(static_cast<uintptr_t>(atom)), instance);
}

// VERIFY: f09cd8b3 auto
// OFFSET: 0x0068C810
void CGxDeviceOpenGl::DeviceDestroyPbuffer() {
    if (this->m_pbuffer) {
        wglReleasePbufferDCARB(this->m_pbuffer, this->m_pbufferDC);
        wglDestroyPbufferARB(this->m_pbuffer);
    }

    this->m_pbuffer = nullptr;
    this->m_pbufferDC = nullptr;
    this->m_pbufferRC = nullptr;
}

// VERIFY: f0025cb1 manual
// OFFSET: 0x0068C860
void CGxDeviceOpenGl::IDevSetFocus(int32_t focus, const CGxFormat& format) {
    this->m_windowFocus = focus;

    if (!format.m_window || format.m_maximize == 2) {
        if (focus) {
            DISPLAY_DEVICEA device;
            memset(&device, 0, sizeof(device));
            device.cb = sizeof(device);

            DEVMODEA mode;
            memset(&mode, 0, sizeof(mode));
            mode.dmSize = sizeof(mode);

            EnumDisplayDevicesA(nullptr, 0, &device, 0);
            EnumDisplaySettingsA(device.DeviceName, format.m_apiSpecificModeID, &mode);
            ChangeDisplaySettingsExA(device.DeviceName, &mode, nullptr, CDS_FULLSCREEN, nullptr);
            SetWindowPos(this->m_hwnd, nullptr, 0, 0, format.m_size.x, format.m_size.y, SWP_NOREDRAW | SWP_NOCOPYBITS | SWP_DEFERERASE);
            ShowWindow(this->m_hwnd, SW_MAXIMIZE);

            RECT windowRect;
            GetWindowRect(this->m_hwnd, &windowRect);
            ClipCursor(&windowRect);
        } else {
            ChangeDisplaySettingsExA(nullptr, nullptr, nullptr, 0, nullptr);
            ShowWindow(this->m_hwnd, SW_MINIMIZE);
        }
    } else if (focus) {
        ShowWindow(this->m_hwnd, SW_SHOW);
    }
}

// VERIFY: dc5af953 auto
// OFFSET: 0x0068C9B0
int32_t CGxDeviceOpenGl::SetFormatMode(CGxFormat& format) {
    if (format.m_window) {
        format.m_apiSpecificModeID = 0;
        return 1;
    }

    DISPLAY_DEVICEA device;
    device.cb = sizeof(device);
    EnumDisplayDevicesA(nullptr, 0, &device, 0);

    uint32_t modeNum = 0;
    uint32_t bpp = (format.m_colorFormat != CGxFormat::Fmt_Rgb565 ? 16 : 0) + 16;

    DEVMODEA mode;
    mode.dmSize = sizeof(mode);

    if (!EnumDisplaySettingsA(device.DeviceName, 0, &mode)) {
        return 0;
    }

    while (mode.dmBitsPerPel != bpp || mode.dmDisplayFrequency != format.m_refreshRate || mode.dmPelsWidth != static_cast<DWORD>(format.m_size.x) || mode.dmPelsHeight != static_cast<DWORD>(format.m_size.y)) {
        if (!EnumDisplaySettingsA(device.DeviceName, ++modeNum, &mode)) {
            return 0;
        }
    }

    format.m_apiSpecificModeID = modeNum;
    return 1;
}

// VERIFY: 21483c6a auto
// OFFSET: 0x006A2660
static BOOL WglDestroyContext(HGLRC context) {
    wglMakeCurrent(nullptr, nullptr);
    return wglDeleteContext(context);
}

// VERIFY: a18dccdf manual
// OFFSET: 0x0068CA80
void CGxDeviceOpenGl::IDevRemoveGlContext() {
    this->DeviceDestroyPbuffer();

    if (this->m_hglrc) {
        this->ICursorDestroy();
        this->DeviceDestroyPoolAPI();
        this->ITexForceRecreation();
        this->IShaderForceRecreation();
        this->IQueryRelease();
        UnbindGlExtensions();
        WglDestroyContext(this->m_hglrc);
        this->m_hglrc = nullptr;
    }

    if (this->m_hdc) {
        SetDeviceGammaRamp(this->m_hdc, &this->m_systemGammaRamp);
        ReleaseDC(this->m_hwnd, this->m_hdc);
        this->m_hdc = nullptr;
    }

    this->m_context = 0;
}

// VERIFY: 72b6a45b manual
// OFFSET: 0x0068CB40
void CGxDeviceOpenGl::DeviceDestroy() {
    CGxDevice::DeviceDestroy();
    this->DeviceDestroyPbuffer();
    s_deviceChanging = 1;

    if (this->m_hwnd) {
        if (this->m_ownhwnd) {
            DestroyWindow(this->m_hwnd);
            this->m_hwnd = nullptr;
        }

        if (this->m_hwnd && !this->m_ownhwnd) {
            this->IDevRemoveGlContext();
        }
    }

    if (this->m_hwndClass) {
        UnregisterClassA(reinterpret_cast<LPCSTR>(static_cast<uintptr_t>(this->m_hwndClass)), GetModuleHandleA(nullptr));
        this->m_hwndClass = 0;
    }

    s_deviceChanging = 0;
}

// VERIFY: 2fa3d260 auto
// OFFSET: 0x0068CC00
void CGxDeviceOpenGl::DeviceSetBaseMipLevel(uint32_t level) {
    this->ITexForceRecreation();
    CGxDevice::DeviceSetBaseMipLevel(level);
}

// VERIFY: 01f4b10a auto
// OFFSET: 0x0068CC20
void CGxDeviceOpenGl::DeviceSetGamma(float gamma) {
    CGxDevice::DeviceSetGamma(gamma);

    if (!this->m_format.m_window) {
        SetDeviceGammaRamp(this->m_hdc, &this->m_gammaRamp);
    }
}

// VERIFY: 0bfbf04f auto
// OFFSET: 0x0068CC60
void CGxDeviceOpenGl::DeviceSetGamma(const CGxGammaRamp& ramp) {
    CGxDevice::DeviceSetGamma(ramp);

    if (!this->m_format.m_window) {
        SetDeviceGammaRamp(this->m_hdc, &this->m_gammaRamp);
    }
}

// VERIFY: f2dbfa69 auto
// OFFSET: 0x0068CCA0
void* CGxDeviceOpenGl::DeviceWindow() {
    return this->m_hwnd;
}

// VERIFY: 8a9d5950 manual
// OFFSET: 0x0068CCB0
static HWND CreateGameWindow(CGxFormat& format, void* param) {
    HINSTANCE instance = GetModuleHandleA(nullptr);

    DWORD style;

    if (!format.m_window) {
        style = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_MAXIMIZE;
    } else if (format.m_maximize == 1) {
        style = WS_POPUP | WS_VISIBLE;
    } else if (format.m_maximize == 2) {
        style = WS_POPUP;
    } else {
        style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW;
    }

    float aspectRatio = 0.0f;

    if (format.m_size.x && format.m_size.y) {
        aspectRatio = static_cast<float>(format.m_size.x) / static_cast<float>(format.m_size.y);
    }

    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = format.m_size.x;
    rect.bottom = format.m_size.y;
    AdjustWindowRectEx(&rect, style, FALSE, 0);

    CGxDevice::s_clientAdjust.x = rect.right - format.m_size.x - rect.left;
    CGxDevice::s_clientAdjust.y = rect.bottom - format.m_size.y - rect.top;

    if (format.m_window) {
        if (format.m_maximize == 1) {
            format.m_pos.x = 0;
            format.m_pos.y = 0;
            format.m_size.x = GetSystemMetrics(SM_CXSCREEN);
            format.m_size.y = GetSystemMetrics(SM_CYSCREEN);
        } else {
            RECT workRect;
            workRect.left = 0;
            workRect.top = 0;
            workRect.right = format.m_size.x;
            workRect.bottom = format.m_size.y;

            if (CGxDevice::AdjustWindowRectToWorkArea(aspectRatio, &CGxDevice::s_clientAdjust, &workRect)) {
                format.m_pos.x = workRect.left;
                format.m_pos.y = workRect.top;
                format.m_size.x = workRect.right - workRect.left;
                format.m_size.y = workRect.bottom - workRect.top;
            } else {
                format.m_pos.x = 0;
                format.m_pos.y = 0;
            }
        }
    }

    int32_t width = format.m_size.x;
    int32_t height = format.m_size.y;

    if (!width) {
        width = CW_USEDEFAULT;
    }

    if (!height) {
        height = CW_USEDEFAULT;
    }

    if (format.m_window && format.m_maximize != 1 && format.m_size.x && format.m_size.y) {
        width += CGxDevice::s_clientAdjust.x;
        height += CGxDevice::s_clientAdjust.y;
    }

    return CreateWindowExA(WS_EX_APPWINDOW, "GxWindowClassOpenGl", CLIENT_NAME, style, format.m_pos.x, format.m_pos.y, width, height, nullptr, nullptr, instance, param);
}

// VERIFY: 23cf22a4 auto
// OFFSET: 0x0068CE40
void CGxDeviceOpenGl::DeviceCreatePbuffer() {
    if (!wglArbPbuffer) {
        return;
    }

    int32_t pixelFormat = GetPixelFormat(this->m_hdc);
    const CRect& window = this->DeviceDefWindow();

    int32_t attributes[2];
    attributes[0] = 0;
    attributes[1] = 0;

    this->m_pbuffer = wglCreatePbufferARB(this->m_hdc, pixelFormat, static_cast<int32_t>(window.maxX - window.minX), static_cast<int32_t>(window.maxY - window.minY), attributes);
    this->m_pbufferDC = wglGetPbufferDCARB(this->m_pbuffer);

    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(this->m_pbufferDC, pixelFormat, sizeof(pfd), &pfd);

    if (pfd.cRedBits == 8) {
        if (pfd.cGreenBits == 8 && pfd.cBlueBits == 8) {
            this->m_caps.m_rttFormat[GxTex_Argb8888] = 1;
        }
    } else if (pfd.cRedBits == 5 && pfd.cGreenBits == 6 && pfd.cBlueBits == pfd.cRedBits) {
        this->m_caps.m_rttFormat[GxTex_Rgb565] = 1;
    }

    this->m_pbufferRC = wglGetCurrentContext();
}

// VERIFY: 57f4221f auto
// OFFSET: 0x0068CF20
void CGxDeviceOpenGl::DeviceQueryPbuffer() {
    if (!this->m_pbuffer) {
        return;
    }

    int32_t lost;
    wglQueryPbufferARB(this->m_pbuffer, WGL_PBUFFER_LOST_ARB, &lost);

    if (lost) {
        this->DeviceDestroyPbuffer();
        this->DeviceCreatePbuffer();
    }
}

// VERIFY: b30d7051 manual
// OFFSET: 0x006A2410
static int32_t SetupPixelFormat(HDC dc, const CGxFormat& format) {
    if (format.m_multisampleCount > 1) {
        return 0;
    }

    int32_t match = 0;
    uint8_t colorBits = 16;
    uint8_t blueBits = 5;
    uint8_t greenBits = 6;
    uint8_t redBits = 5;
    uint8_t alphaBits = 0;
    uint8_t depthBits = 16;
    uint8_t stencilBits = 0;

    switch (format.m_colorFormat) {
        case CGxFormat::Fmt_Rgb565:
            colorBits = 16;
            blueBits = 5;
            greenBits = 6;
            redBits = 5;
            alphaBits = 0;
            break;

        case CGxFormat::Fmt_ArgbX888:
            colorBits = 24;
            redBits = 8;
            greenBits = 8;
            blueBits = 8;
            alphaBits = 0;
            break;

        case CGxFormat::Fmt_Argb8888:
            colorBits = 24;
            redBits = 8;
            greenBits = 8;
            blueBits = 8;
            alphaBits = 8;
            break;

        case CGxFormat::Fmt_Argb2101010:
            colorBits = 30;
            redBits = 10;
            greenBits = 10;
            blueBits = 10;
            alphaBits = 2;
            break;

        default:
            break;
    }

    switch (format.m_depthFormat) {
        case CGxFormat::Fmt_Ds160:
            depthBits = 16;
            stencilBits = 0;
            break;

        case CGxFormat::Fmt_Ds24X:
            depthBits = 24;
            stencilBits = 0;
            break;

        case CGxFormat::Fmt_Ds248:
            depthBits = 24;
            stencilBits = 8;
            break;

        case CGxFormat::Fmt_Ds320:
            depthBits = 32;
            stencilBits = 0;
            break;

        default:
            break;
    }

    PIXELFORMATDESCRIPTOR pfd;
    pfd.cAlphaBits = alphaBits;
    pfd.cColorBits = colorBits;
    pfd.cDepthBits = depthBits;
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cRedBits = 0;
    pfd.cRedShift = 0;
    pfd.cGreenBits = 0;
    pfd.cGreenShift = 0;
    pfd.cBlueBits = 0;
    pfd.cBlueShift = 0;
    pfd.cAlphaShift = 0;
    pfd.cAccumBits = 0;
    pfd.cAccumRedBits = 0;
    pfd.cAccumGreenBits = 0;
    pfd.cAccumBlueBits = 0;
    pfd.cAccumAlphaBits = 0;
    pfd.cStencilBits = stencilBits;
    memset(&pfd.cAuxBuffers, 0, 15);

    int32_t pixelFormat = ChoosePixelFormat(dc, &pfd);

    if (!pixelFormat) {
        CGxDevice::Log("ChoosePixelFormat failed, color = %d, alpha = %d, depth = %d\n", colorBits, alphaBits, depthBits);
        return 0;
    }

    DescribePixelFormat(dc, pixelFormat, sizeof(pfd), &pfd);

    int32_t matches = 1;

    if (!format.m_window) {
        if (format.m_colorFormat != CGxFormat::Fmt_ArgbX888) {
            matches = pfd.cAlphaBits == alphaBits;
        }

        if (pfd.cRedBits == redBits && pfd.cBlueBits == blueBits && pfd.cGreenBits == greenBits) {
            match = 1;
        }

        matches &= match;
    }

    if (format.m_depthFormat != CGxFormat::Fmt_Ds24X) {
        matches &= pfd.cStencilBits == stencilBits;
    }

    if (!((pfd.cDepthBits == depthBits) & matches)) {
        CGxDevice::Log("SetupPixelFormat(): pfd does not match requested");
        return 0;
    }

    if (SetPixelFormat(dc, pixelFormat, &pfd)) {
        return 1;
    }

    CGxDevice::Log("SetPixelFormat failed, color = %d, alpha = %d, depth = %d, fmt = %d\n", colorBits, alphaBits, depthBits, pixelFormat);
    return 0;
}

// VERIFY: c2b2597a manual
// OFFSET: 0x006A2680
static int32_t SetupPixelFormatWGL(HDC dc, const CGxFormat& format) {
    int32_t* attributes = static_cast<int32_t*>(alloca(512));
    attributes[0] = WGL_DRAW_TO_WINDOW_ARB;
    attributes[1] = 1;
    attributes[2] = WGL_ACCELERATION_ARB;
    attributes[3] = WGL_FULL_ACCELERATION_ARB;
    attributes[4] = WGL_SUPPORT_OPENGL_ARB;
    attributes[5] = 1;
    attributes[6] = WGL_DOUBLE_BUFFER_ARB;
    attributes[7] = 1;

    int32_t alphaBits = 0;
    int32_t redBits = 0;
    int32_t greenBits = 0;
    int32_t blueBits = 0;

    switch (format.m_colorFormat) {
        case CGxFormat::Fmt_Rgb565:
            greenBits = 6;
            alphaBits = 0;
            blueBits = 5;
            redBits = 5;
            break;

        case CGxFormat::Fmt_ArgbX888:
            greenBits = 8;
            alphaBits = 0;
            blueBits = 8;
            redBits = 8;
            break;

        case CGxFormat::Fmt_Argb8888:
            alphaBits = 8;
            blueBits = 8;
            greenBits = 8;
            redBits = 8;
            break;

        default:
            break;
    }

    attributes[8] = WGL_PIXEL_TYPE_ARB;
    attributes[9] = WGL_TYPE_RGBA_ARB;
    attributes[10] = WGL_RED_BITS_ARB;
    attributes[11] = redBits;
    attributes[12] = WGL_GREEN_BITS_ARB;
    attributes[13] = greenBits;
    attributes[14] = WGL_BLUE_BITS_ARB;
    attributes[15] = blueBits;
    attributes[16] = WGL_ALPHA_BITS_ARB;
    attributes[17] = alphaBits;

    int32_t depthBits = 0;
    int32_t stencilBits = 0;

    switch (format.m_depthFormat) {
        case CGxFormat::Fmt_Ds160:
            depthBits = 16;
            stencilBits = 0;
            break;

        case CGxFormat::Fmt_Ds24X:
            depthBits = 24;
            stencilBits = 0;
            break;

        case CGxFormat::Fmt_Ds248:
            depthBits = 24;
            stencilBits = 8;
            break;

        case CGxFormat::Fmt_Ds320:
            depthBits = 32;
            stencilBits = 0;
            break;

        default:
            break;
    }

    attributes[18] = WGL_DEPTH_BITS_ARB;
    attributes[19] = depthBits;
    attributes[20] = WGL_STENCIL_BITS_ARB;
    attributes[21] = stencilBits;

    uint32_t count = 22;

    if (glARBMultisample && format.m_multisampleCount > 1) {
        attributes[22] = WGL_SAMPLE_BUFFERS_ARB;
        attributes[23] = 1;
        attributes[24] = WGL_SAMPLES_ARB;
        count = 26;
        attributes[25] = format.m_multisampleCount;

        if (glNVMultisampleFilterHint) {
            glHint(GL_MULTISAMPLE_FILTER_HINT_NV, format.m_multisampleQuality <= 0.5f ? GL_FASTEST : GL_NICEST);
        }
    }

    attributes[count] = 0;
    attributes[count + 1] = 0;

    int32_t pixelFormat;
    UINT numFormats;

    if (!wglChoosePixelFormatARB(dc, attributes, nullptr, 1, &pixelFormat, &numFormats)) {
        return 0;
    }

    int32_t matches = 1;

    if (!numFormats) {
        return 0;
    }

    int32_t attribute;
    int32_t value;

    if (!format.m_window) {
        if (format.m_colorFormat != CGxFormat::Fmt_ArgbX888) {
            attribute = WGL_ALPHA_BITS_ARB;
            matches = wglGetPixelFormatAttribivARB(dc, pixelFormat, 0, 1, &attribute, &value) && value == alphaBits;
        }

        attribute = WGL_RED_BITS_ARB;
        int32_t red = wglGetPixelFormatAttribivARB(dc, pixelFormat, 0, 1, &attribute, &value) && value == redBits;
        matches = red & matches;

        attribute = WGL_GREEN_BITS_ARB;
        int32_t green = wglGetPixelFormatAttribivARB(dc, pixelFormat, 0, 1, &attribute, &value) && value == greenBits;
        matches = green & matches;

        attribute = WGL_BLUE_BITS_ARB;
        int32_t blue = wglGetPixelFormatAttribivARB(dc, pixelFormat, 0, 1, &attribute, &value) && value == blueBits;
        matches = blue & matches;
    }

    if (format.m_depthFormat != CGxFormat::Fmt_Ds24X) {
        attribute = WGL_STENCIL_BITS_ARB;
        int32_t stencil = wglGetPixelFormatAttribivARB(dc, pixelFormat, 0, 1, &attribute, &value) && value == stencilBits;
        matches &= stencil;
    }

    attribute = WGL_DEPTH_BITS_ARB;
    int32_t depth = wglGetPixelFormatAttribivARB(dc, pixelFormat, 0, 1, &attribute, &value) && value == depthBits;
    matches = depth & matches;

    if (glARBMultisample && format.m_multisampleCount > 1) {
        attribute = WGL_SAMPLES_ARB;
        int32_t samples = wglGetPixelFormatAttribivARB(dc, pixelFormat, 0, 1, &attribute, &value) && value == static_cast<int32_t>(format.m_multisampleCount);
        matches &= samples;
    }

    if (!matches) {
        CGxDevice::Log("SetupPixelFormatWGL(): pixelFormat does not match requested");
        return 0;
    }

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd.nVersion, 0, sizeof(pfd) - sizeof(pfd.nSize));
    pfd.nSize = sizeof(pfd);

    return SetPixelFormat(dc, pixelFormat, &pfd);
}

// VERIFY: 10007263 auto
// OFFSET: 0x006A2A50
static HGLRC CreateGLContext(HDC dc, const CGxFormat& format) {
    int32_t result;

    if (wglArbPixelFormat) {
        result = SetupPixelFormatWGL(dc, format);
    } else {
        result = SetupPixelFormat(dc, format);
    }

    if (!result) {
        return nullptr;
    }

    return wglCreateContext(dc);
}

// VERIFY: b403578c manual
// OFFSET: 0x0069E140
void CGxDeviceOpenGl::IShaderConstantsClear() {
    this->ShaderConstantsClear();
}

// VERIFY: 80ad3a5a auto
// OFFSET: 0x0068CF60
int32_t CGxDeviceOpenGl::IDevAttachGlContext(const CGxFormat& format) {
    CreateDummyContext();

    this->m_hdc = GetDC(this->m_hwnd);

    if (this->m_hdc && (this->m_hglrc = CreateGLContext(this->m_hdc, format)) != nullptr) {
        wglMakeCurrent(this->m_hdc, this->m_hglrc);
        this->m_context = 1;

        if (wglExtSwapControl) {
            wglSwapIntervalEXT(format.m_vsync != 0);
        }

        this->ISetCaps(format);
        this->DeviceCreatePbuffer();
        this->ICursorCreate(format);
        this->IStateSetContextDefaults();
        this->IRsSync(1);
        this->IShaderConstantsClear();
        this->m_glVendor = glGetString(GL_VENDOR);
        this->m_glRenderer = glGetString(GL_RENDERER);
        this->m_glVersion = glGetString(GL_VERSION);

        return 1;
    }

    this->IDevRemoveGlContext();
    return 0;
}

// VERIFY: 11254ee2 manual
// OFFSET: 0x0068D050
LRESULT CALLBACK CGxDeviceOpenGl::WindowProcGl(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    CGxDeviceOpenGl* device = reinterpret_cast<CGxDeviceOpenGl*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    switch (message) {
        case WM_CREATE:
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTA*>(lparam)->lpCreateParams));
            return 0;

        case WM_DESTROY:
            device->DeviceWM(GxWM_Destroy, 0, 0);
            return 0;

        case WM_SIZE: {
            CRect rect;
            rect.minY = 0.0f;
            rect.minX = 0.0f;
            rect.maxY = HIWORD(lparam);
            rect.maxX = LOWORD(lparam);

            if (wparam == SIZE_MINIMIZED) {
                device->DeviceWM(GxWM_Size, reinterpret_cast<uintptr_t>(&rect), Gx_SizeMinimized);
            } else {
                uint32_t resizeType = 0;

                if (wparam == SIZE_MAXHIDE) {
                    resizeType = Gx_SizeMaxHide;
                }

                device->DeviceWM(GxWM_Size, reinterpret_cast<uintptr_t>(&rect), resizeType);
            }

            break;
        }

        case WM_ACTIVATE:
            if (wparam) {
                if (wparam == WA_ACTIVE && !device->IDevIsWindowed()) {
                    CRect rect;
                    rect.minY = 0.0f;
                    rect.minX = 0.0f;
                    rect.maxY = 0.0f;
                    rect.maxX = 0.0f;
                    GxCapsWindowSizeInScreenCoords(rect);
                    device->DeviceWM(GxWM_Size, reinterpret_cast<uintptr_t>(&rect), 3);
                }
            } else if (!device->IDevIsWindowed()) {
                CRect rect;
                rect.minY = 0.0f;
                rect.minX = 0.0f;
                rect.maxY = 0.0f;
                rect.maxX = 0.0f;
                device->DeviceWM(GxWM_Size, reinterpret_cast<uintptr_t>(&rect), Gx_SizeMinimized);
            }

            break;

        case WM_SETFOCUS:
            device->DeviceWM(GxWM_SetFocus, 0, 0);
            return 0;

        case WM_KILLFOCUS:
            device->DeviceWM(GxWM_KillFocus, 0, 0);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT paint;
            BeginPaint(hwnd, &paint);
            EndPaint(hwnd, &paint);
            return 0;
        }

        case WM_ERASEBKGND:
            return 0;

        case WM_DISPLAYCHANGE:
            if (!device->IDevIsWindowed()) {
                CRect rect;
                rect.minY = 0.0f;
                rect.minX = 0.0f;
                rect.maxY = HIWORD(lparam);
                rect.maxX = LOWORD(lparam);
                device->DeviceWM(GxWM_DisplayChange, reinterpret_cast<uintptr_t>(&rect), 0);
            }

            break;

        case WM_SYSCOMMAND:
            if (wparam == SC_SCREENSAVE || wparam == SC_MONITORPOWER) {
                return 0;
            }

            break;

        case WM_SIZING: {
            RECT* sizingRect = reinterpret_cast<RECT*>(lparam);

            CiRect rect;
            rect.minX = sizingRect->left;
            rect.minY = sizingRect->top;
            rect.maxX = sizingRect->right;
            rect.maxY = sizingRect->bottom;

            if (rect.maxX - rect.minX <= 0 || rect.maxY - rect.minY <= 0) {
                return 0;
            }

            switch (wparam) {
                case WMSZ_LEFT:
                    CGxDevice::ClampRectLeft(rect);
                    CGxDevice::ClampRectTopByAspect(rect);
                    break;

                case WMSZ_RIGHT:
                    CGxDevice::ClampRectRight(rect);
                    CGxDevice::ClampRectBottomByAspect(rect);
                    break;

                case WMSZ_TOP:
                    CGxDevice::ClampRectTop(rect);
                    CGxDevice::ClampRectLeftByAspect(rect);
                    break;

                case WMSZ_TOPLEFT:
                    CGxDevice::ClampRectTop(rect);
                    CGxDevice::ClampRectLeft(rect);
                    CGxDevice::ClampRectTopByAspect(rect);
                    CGxDevice::ClampRectLeftByAspect(rect);
                    break;

                case WMSZ_TOPRIGHT:
                    CGxDevice::ClampRectTop(rect);
                    CGxDevice::ClampRectRight(rect);
                    CGxDevice::ClampRectTopByAspect(rect);
                    CGxDevice::ClampRectRightByAspect(rect);
                    break;

                case WMSZ_BOTTOM:
                    CGxDevice::ClampRectBottom(rect);
                    CGxDevice::ClampRectRightByAspect(rect);
                    break;

                case WMSZ_BOTTOMLEFT:
                    CGxDevice::ClampRectBottom(rect);
                    CGxDevice::ClampRectLeft(rect);
                    CGxDevice::ClampRectLeftByAspect(rect);
                    CGxDevice::ClampRectBottomByAspect(rect);
                    break;

                case WMSZ_BOTTOMRIGHT:
                    CGxDevice::ClampRectBottom(rect);
                    CGxDevice::ClampRectRight(rect);
                    CGxDevice::ClampRectBottomByAspect(rect);
                    CGxDevice::ClampRectRightByAspect(rect);
                    break;

                default:
                    break;
            }

            sizingRect->left = rect.minX;
            sizingRect->bottom = rect.maxY;
            sizingRect->top = rect.minY;
            sizingRect->right = rect.maxX;

            return 1;
        }

        default:
            break;
    }

    if (device && device->m_windowProc) {
        return device->m_windowProc(hwnd, message, wparam, lparam);
    }

    return DefWindowProcA(hwnd, message, wparam, lparam);
}

// VERIFY: bb87ce15 manual
// OFFSET: 0x0068D460
int32_t CGxDeviceOpenGl::DeviceWM(EGxWM wm, uintptr_t param1, uintptr_t param2) {
    switch (wm) {
        case GxWM_Size:
            if (param2 == Gx_SizeMinimized || param2 == Gx_SizeMaxHide) {
                this->m_windowVisible = 0;
            } else {
                this->m_windowVisible = 1;

                const CRect& window = this->DeviceDefWindow();
                const CRect* rect = reinterpret_cast<const CRect*>(param1);
                bool grew = window.maxX - window.minX < rect->maxX - rect->minX || window.maxY - window.minY < rect->maxY - rect->minY;
                this->DeviceSetDefWindow(*rect);

                if (grew) {
                    this->DeviceDestroyPbuffer();
                    this->DeviceCreatePbuffer();
                } else {
                    this->DeviceQueryPbuffer();
                }

                this->m_viewportDirty = 1;
            }

            break;

        case GxWM_DisplayChange:
            if (this->m_windowVisible) {
                this->DeviceSetDefWindow(*reinterpret_cast<const CRect*>(param1));
                this->m_viewportDirty = 1;
            }

            break;

        case GxWM_Destroy:
            this->IDevRemoveGlContext();
            this->m_windowFocus = 0;

            if (!this->m_format.m_window || this->m_format.m_maximize == 2) {
                ChangeDisplaySettingsExA(nullptr, nullptr, nullptr, 0, nullptr);
                ShowWindow(this->m_hwnd, SW_MINIMIZE);
            }

            break;

        case GxWM_SetFocus:
            if (!s_deviceChanging) {
                this->IDevSetFocus(1, this->m_format);
            }

            break;

        case GxWM_KillFocus:
            if (!s_deviceChanging) {
                this->m_windowFocus = 0;

                if (!this->m_format.m_window || this->m_format.m_maximize == 2) {
                    ChangeDisplaySettingsExA(nullptr, nullptr, nullptr, 0, nullptr);
                    ShowWindow(this->m_hwnd, SW_MINIMIZE);
                }
            }

            break;

        default:
            break;
    }

    return 0;
}

// VERIFY: 075c29d3 auto
// OFFSET: 0x0068D610
int32_t CGxDeviceOpenGl::DeviceCreate(void* window, const CGxFormat& format) {
    s_deviceChanging = 1;
    this->m_ownhwnd = 0;

    HDC dc = GetDC(nullptr);

    if (GetDeviceGammaRamp(dc, &this->m_systemGammaRamp) && &this->m_gammaRamp != &this->m_systemGammaRamp) {
        this->m_gammaRamp = this->m_systemGammaRamp;
    }

    ReleaseDC(nullptr, dc);

    this->m_hwnd = static_cast<HWND>(window);

    if (this->IDevAttachGlContext(format)) {
        s_deviceChanging = 0;

        if (CGxDevice::DeviceCreate(window, format)) {
            return 1;
        }
    }

    this->DeviceDestroy();
    s_deviceChanging = 0;
    return 0;
}

// VERIFY: 9f9c690c auto
// OFFSET: 0x0068D6D0
int32_t CGxDeviceOpenGl::DeviceSetFormat(const CGxFormat& format) {
    CGxDevice::Log("CGxDeviceOpenGl::DeviceSetFormat():");
    this->Log(format);

    s_deviceChanging = 1;
    this->IDevRemoveGlContext();
    DestroyWindow(this->m_hwnd);
    this->m_hwnd = nullptr;

    CGxFormat newFormat = format;

    if (this->SetFormatMode(newFormat) && (this->m_hwnd = CreateGameWindow(newFormat, this)) != nullptr) {
        this->IDevSetFocus(1, newFormat);

        if (this->IDevAttachGlContext(newFormat)) {
            s_deviceChanging = 0;

            if (CGxDevice::DeviceSetFormat(newFormat)) {
                this->DeviceSetGamma(this->m_gammaRamp);
                return 1;
            }
        }
    }

    s_deviceChanging = 0;
    return 0;
}

// VERIFY: 12cc2e1f manual
// OFFSET: 0x0068D7B0
void CGxDeviceOpenGl::IDeviceSetRenderTargetCopy(EGxBuffer buffer, CGxTex* texture, uint32_t plane) {
    if (!this->m_context || !this->m_windowVisible) {
        return;
    }

    CGxDevice::TextureTarget& target = this->m_textureTarget[buffer];

    if (target.m_texture == texture && target.m_plane == plane) {
        return;
    }

    CGxDevice::DeviceSetRenderTarget(buffer, texture, plane);

    if (this->m_textureTarget[GxBuffers_Color].m_texture || this->m_textureTarget[GxBuffers_Depth].m_texture) {
        this->DeviceSetCurWindow(this->DeviceDefWindow());
    }

    CGxTex* previous = static_cast<CGxTex*>(target.m_apiSpecific);

    if (previous) {
        if (!previous->m_apiSpecificData) {
            glGenTextures(1, reinterpret_cast<GLuint*>(&previous->m_apiSpecificData));
        }

        this->BindTexture(previous, kNullTmu);

        if (previous->m_needsFlagUpdate) {
            this->ITexSetFlags(previous);
        }

        GLenum glTarget = s_glTexTarget[previous->m_target];

        if (previous->m_needsCreation) {
            glCopyTexImage2D(glTarget, 0, s_glInternalFormat[previous->m_format], 0, 0, previous->m_width, previous->m_height, 0);
        } else {
            glCopyTexSubImage2D(glTarget, 0, 0, 0, 0, 0, previous->m_width, previous->m_height);
        }

        previous->m_needsCreation = 0;
    }

    target.m_apiSpecific = texture;

    HDC dc = this->m_hdc;
    HGLRC context = this->m_hglrc;

    if (this->m_textureTarget[GxBuffers_Color].m_apiSpecific || this->m_textureTarget[GxBuffers_Depth].m_apiSpecific) {
        dc = this->m_pbufferDC;
        context = this->m_pbufferRC;
    }

    wglMakeCurrent(dc, context);

    GxXformSetViewport(this->m_viewport.m_x.m_l, this->m_viewport.m_x.m_h, this->m_viewport.m_y.m_l, this->m_viewport.m_y.m_h, this->m_viewport.m_z.m_l, this->m_viewport.m_z.m_h);
    g_theGxDevicePtr->XformSetProjection(this->m_projection);
}

// VERIFY: ae538796 manual
// OFFSET: 0x0068D950
void CGxDeviceOpenGl::CapsWindowSizeInScreenCoords(CRect& rect) {
    if (this->IDevIsWindowed()) {
        const CRect& window = this->DeviceCurWindow();

        POINT points[2];
        points[0].x = 0;
        points[0].y = 0;
        points[1].x = static_cast<LONG>(window.maxX);
        points[1].y = static_cast<LONG>(window.maxY);
        MapWindowPoints(this->m_hwnd, nullptr, points, 2);

        rect.minY = static_cast<float>(points[0].y);
        rect.minX = static_cast<float>(points[0].x);
        rect.maxY = static_cast<float>(points[1].y);
        rect.maxX = static_cast<float>(points[1].x);
    } else {
        const CRect& window = this->DeviceCurWindow();
        rect.minY = window.minY;
        rect.minX = window.minX;
        rect.maxY = window.maxY;
        rect.maxX = window.maxX;
    }
}

// VERIFY: a93b9ae1 manual
// OFFSET: 0x0068DA10
static ATOM WindowClassCreate() {
    HINSTANCE instance = GetModuleHandleA(nullptr);

    WNDCLASSEXA windowClass;
    memset(&windowClass, 0, sizeof(windowClass));
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_OWNDC;
    windowClass.lpfnWndProc = CGxDeviceOpenGl::WindowProcGl;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "GxWindowClassOpenGl";
    windowClass.hIcon = static_cast<HICON>(LoadImageA(instance, "BlizzardIcon.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    windowClass.hCursor = LoadCursorA(instance, "BlizzardCursor.cur");

    if (!windowClass.hCursor) {
        windowClass.hCursor = LoadCursorA(instance, IDC_ARROW);
    }

    return RegisterClassExA(&windowClass);
}

// VERIFY: 495b2371 auto
// OFFSET: 0x0068DAA0
int32_t CGxDeviceOpenGl::DeviceCreate(GxWindowProc windowProc, const CGxFormat& format) {
    this->m_ownhwnd = 1;

    HDC dc = GetDC(nullptr);

    if (GetDeviceGammaRamp(dc, &this->m_systemGammaRamp) && &this->m_gammaRamp != &this->m_systemGammaRamp) {
        this->m_gammaRamp = this->m_systemGammaRamp;
    }

    ReleaseDC(nullptr, dc);

    this->m_hwndClass = WindowClassCreate();

    if (this->m_hwndClass && CGxDevice::DeviceCreate(windowProc, format)) {
        return 1;
    }

    this->DeviceDestroy();
    return 0;
}

// VERIFY: a77c2696 manual
// OFFSET: 0x0068DB40
static void WglAdapterFormats(HDC dc, TSGrowableArray<CGxFormat>& formats) {
    wglGetPixelFormatAttribivARB = reinterpret_cast<PFNWGLGETPIXELFORMATATTRIBIVARBPROC>(wglGetProcAddress("wglGetPixelFormatAttribivARB"));

    if (!wglGetPixelFormatAttribivARB) {
        return;
    }

    int32_t numberAttrib = WGL_NUMBER_PIXEL_FORMATS_ARB;
    int32_t supportAttrib = WGL_SUPPORT_OPENGL_ARB;
    int32_t accelerationAttrib = WGL_ACCELERATION_ARB;
    int32_t pixelTypeAttrib = WGL_PIXEL_TYPE_ARB;
    int32_t colorBitsAttrib = WGL_COLOR_BITS_ARB;
    int32_t alphaBitsAttrib = WGL_ALPHA_BITS_ARB;
    int32_t depthBitsAttrib = WGL_DEPTH_BITS_ARB;
    int32_t stencilBitsAttrib = WGL_STENCIL_BITS_ARB;
    int32_t samplesAttrib = WGL_SAMPLES_ARB;

    int32_t count;

    if (!wglGetPixelFormatAttribivARB(dc, 0, 0, 1, &numberAttrib, &count)) {
        return;
    }

    for (int32_t i = 1; i < count; i++) {
        CGxFormat format;
        memset(&format, 0, sizeof(format));

        int32_t support;
        int32_t acceleration;
        int32_t pixelType;
        int32_t colorBits;
        int32_t alphaBits;
        int32_t depthBits;
        int32_t stencilBits;
        int32_t samples;

        if (!wglGetPixelFormatAttribivARB(dc, i, 0, 1, &supportAttrib, &support) || !support) {
            continue;
        }

        if (!wglGetPixelFormatAttribivARB(dc, i, 0, 1, &accelerationAttrib, &acceleration) || acceleration != WGL_FULL_ACCELERATION_ARB) {
            continue;
        }

        if (!wglGetPixelFormatAttribivARB(dc, i, 0, 1, &pixelTypeAttrib, &pixelType) || pixelType != WGL_TYPE_RGBA_ARB) {
            continue;
        }

        if (!wglGetPixelFormatAttribivARB(dc, i, 0, 1, &colorBitsAttrib, &colorBits)) {
            continue;
        }

        if (!wglGetPixelFormatAttribivARB(dc, i, 0, 1, &depthBitsAttrib, &depthBits)) {
            continue;
        }

        if (!wglGetPixelFormatAttribivARB(dc, i, 0, 1, &stencilBitsAttrib, &stencilBits)) {
            continue;
        }

        if (!wglGetPixelFormatAttribivARB(dc, i, 0, 1, &samplesAttrib, &samples) || !samples) {
            samples = 1;
        }

        if (colorBits == 16) {
            format.m_colorFormat = CGxFormat::Fmt_Rgb565;
        } else {
            if (colorBits != 32) {
                continue;
            }

            if (!wglGetPixelFormatAttribivARB(dc, i, 0, 1, &alphaBitsAttrib, &alphaBits)) {
                continue;
            }

            if (alphaBits) {
                if (alphaBits != 8) {
                    continue;
                }

                format.m_colorFormat = CGxFormat::Fmt_Argb8888;
            } else {
                format.m_colorFormat = CGxFormat::Fmt_ArgbX888;
            }
        }

        if (depthBits == 16) {
            if (stencilBits) {
                continue;
            }

            format.m_depthFormat = CGxFormat::Fmt_Ds160;
        } else if (depthBits == 24) {
            if (!stencilBits) {
                format.m_depthFormat = CGxFormat::Fmt_Ds24X;
            } else if (stencilBits == 8) {
                format.m_depthFormat = CGxFormat::Fmt_Ds248;
            } else {
                continue;
            }
        } else {
            if (depthBits != 32 || stencilBits) {
                continue;
            }

            format.m_depthFormat = CGxFormat::Fmt_Ds320;
        }

        format.m_multisampleCount = samples;

        if (!CGxDevice::FormatExists(formats, format)) {
            *formats.New() = format;
        }
    }
}

// VERIFY: 52975109 manual
// OFFSET: 0x0068DDD0
static void PfdAdapterFormats(HDC dc, TSGrowableArray<CGxFormat>& formats) {
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd.dwFlags, 0, sizeof(pfd) - offsetof(PIXELFORMATDESCRIPTOR, dwFlags));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;

    int32_t count = DescribePixelFormat(dc, 0, 0, nullptr);

    for (int32_t i = 1; i < count; i++) {
        DescribePixelFormat(dc, i, sizeof(pfd), &pfd);

        if (pfd.iPixelType != PFD_TYPE_RGBA) {
            continue;
        }

        if (!(pfd.dwFlags & (PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL)) || (pfd.dwFlags & (PFD_GENERIC_FORMAT | PFD_SUPPORT_GDI))) {
            continue;
        }

        CGxFormat format;
        memset(&format, 0, sizeof(format));

        if (pfd.cRedBits == 8) {
            if (pfd.cBlueBits != 8 || pfd.cGreenBits != pfd.cRedBits) {
                continue;
            }

            if (pfd.cAlphaBits == 8) {
                format.m_colorFormat = CGxFormat::Fmt_Argb8888;
            } else if (!pfd.cAlphaBits) {
                format.m_colorFormat = CGxFormat::Fmt_ArgbX888;
            }
        } else {
            if (pfd.cRedBits != 5) {
                continue;
            }

            if (pfd.cGreenBits != 5 && pfd.cGreenBits != 6) {
                continue;
            }

            if (pfd.cBlueBits != 5) {
                continue;
            }

            format.m_colorFormat = CGxFormat::Fmt_Rgb565;
        }

        if (pfd.cDepthBits == 16) {
            if (pfd.cStencilBits) {
                continue;
            }

            format.m_depthFormat = CGxFormat::Fmt_Ds160;
        } else if (pfd.cDepthBits == 24) {
            if (!pfd.cStencilBits) {
                format.m_depthFormat = CGxFormat::Fmt_Ds24X;
            } else if (pfd.cStencilBits == 8) {
                format.m_depthFormat = CGxFormat::Fmt_Ds248;
            } else {
                continue;
            }
        } else {
            if (pfd.cDepthBits != 32 || pfd.cStencilBits) {
                continue;
            }

            format.m_depthFormat = CGxFormat::Fmt_Ds320;
        }

        format.m_multisampleCount = 1;

        if (!CGxDevice::FormatExists(formats, format)) {
            *formats.New() = format;
        }
    }
}

// VERIFY: fafeb5f2 manual
// OFFSET: 0x0068DF60
int32_t CGxDevice::OpenGlAdapterFormats(TSGrowableArray<CGxFormat>& formats) {
    HINSTANCE instance = GetModuleHandleA(nullptr);

    WNDCLASSEXA windowClass;
    memset(&windowClass, 0, sizeof(windowClass));
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_OWNDC;
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "adfmt";

    ATOM atom = RegisterClassExA(&windowClass);

    if (!atom) {
        return formats.Count() != 0;
    }

    HWND window = CreateWindowExA(0, reinterpret_cast<LPCSTR>(static_cast<uintptr_t>(atom)), "adfmt", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, nullptr, nullptr);

    if (window) {
        HDC dc = GetDC(window);

        if (dc) {
            int32_t count = DescribePixelFormat(dc, 0, 0, nullptr);

            PIXELFORMATDESCRIPTOR pfd;
            memset(&pfd.dwFlags, 0, sizeof(pfd) - offsetof(PIXELFORMATDESCRIPTOR, dwFlags));
            pfd.nSize = sizeof(pfd);
            pfd.nVersion = 1;

            for (int32_t i = 1; i <= count; i++) {
                DescribePixelFormat(dc, i, sizeof(pfd), &pfd);

                if (!(pfd.dwFlags & (PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL)) || (pfd.dwFlags & (PFD_GENERIC_FORMAT | PFD_SUPPORT_GDI))) {
                    continue;
                }

                SetPixelFormat(dc, i, &pfd);

                HGLRC context = wglCreateContext(dc);

                if (!context) {
                    continue;
                }

                HGLRC currentContext = wglGetCurrentContext();
                HDC currentDC = wglGetCurrentDC();
                wglMakeCurrent(dc, context);

                wglGetExtensionsStringARB = reinterpret_cast<PFNWGLGETEXTENSIONSSTRINGARBPROC>(wglGetProcAddress("wglGetExtensionsStringARB"));

                if (HasWGLExtension("WGL_ARB_pixel_format")) {
                    WglAdapterFormats(dc, formats);
                } else {
                    PfdAdapterFormats(dc, formats);
                }

                wglMakeCurrent(currentDC, currentContext);
                wglDeleteContext(context);
                break;
            }

            ReleaseDC(window, dc);
        }

        DestroyWindow(window);
    }

    UnregisterClassA(reinterpret_cast<LPCSTR>(static_cast<uintptr_t>(atom)), instance);

    return formats.Count() != 0;
}

// VERIFY: f1a2e45f auto
// OFFSET: 0x006A5A00
void CGxDeviceOpenGl::CapsWindowSize(CRect& rect) {
    rect = this->DeviceCurWindow();
}

// VERIFY: 37050252 auto
// OFFSET: 0x00691870
static void ColorToFloats(GLfloat* out, const CImVector& color) {
    out[0] = color.r * oo255;
    out[1] = color.g * oo255;
    out[2] = color.b * oo255;
    out[3] = color.a * oo255;
}

// VERIFY: 8cbcfafc manual
// OFFSET: 0x00691FC0
void CGxDeviceOpenGl::IStateSyncIndexPtr() {
    if (!this->m_primIndexDirty) {
        return;
    }

    CGxBuf* buf = this->m_primIndexBuf;
    CGxPool* pool = buf->m_pool;

    if (glARBVertexBufferObject) {
        if (this->m_useVboBroker[pool->m_target][pool->m_usage]) {
            GLuint vbo;
            uint32_t offset;
            static_cast<CGxVboBroker*>(pool->m_apiSpecific)->GetLastSelectedVbo(vbo, offset);

            if (this->m_deviceStates[Ds_ElementArrayBuffer] != vbo) {
                glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, vbo);
                this->m_deviceStates[Ds_ElementArrayBuffer] = vbo;
            }

            this->m_indexPtr = this->m_primIndexBuf->m_index;
            this->m_primIndexDirty = 0;
            return;
        }

        if (this->m_useVbo[pool->m_target][pool->m_usage]) {
            GLuint name = static_cast<GLuint>(reinterpret_cast<uintptr_t>(pool->m_apiSpecific));

            if (this->m_deviceStates[Ds_ElementArrayBuffer] != name) {
                glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, name);
                this->m_deviceStates[Ds_ElementArrayBuffer] = name;
            }

            this->m_indexPtr = this->m_primIndexBuf->m_index;
            this->m_primIndexDirty = 0;
            return;
        }
    }

    if (buf->unk_001F) {
        this->m_indexPtr = buf->m_index;
        this->m_primIndexDirty = 0;
    } else {
        uintptr_t indexPtr = reinterpret_cast<uintptr_t>(pool->m_apiSpecific) + buf->m_index;
        this->m_primIndexDirty = 0;
        this->m_indexPtr = indexPtr;
    }
}

// VERIFY: ce270295 manual
// OFFSET: 0x006920E0
void CGxDeviceOpenGl::IStateSyncViewport() {
    const CRect& window = this->DeviceCurWindow();

    float width;
    float height;

    if (this->m_textureTarget[GxBuffers_Color].m_apiSpecific) {
        CGxTex* target = static_cast<CGxTex*>(this->m_textureTarget[GxBuffers_Color].m_apiSpecific);
        width = static_cast<float>(target->m_width);
        height = static_cast<float>(target->m_height);
    } else {
        width = window.maxX;
        height = window.maxY;
    }

    GLint x = static_cast<GLint>(this->m_viewport.m_x.m_l * width + 0.5f);
    GLint y = static_cast<GLint>(this->m_viewport.m_y.m_l * height + 0.5f);
    glViewport(x, y, static_cast<GLint>(width * this->m_viewport.m_x.m_h + 0.5f) - x, static_cast<GLint>(height * this->m_viewport.m_y.m_h + 0.5f) - y);
    glDepthRange(this->m_viewport.m_z.m_l, this->m_viewport.m_z.m_h);
    this->m_viewportDirty = 0;
}

// VERIFY: c4e80dae auto
// OFFSET: 0x006921D0
void CGxDeviceOpenGl::IStateSyncEnables() {
    if (this->m_appMasterEnables == this->m_hwMasterEnables) {
        return;
    }

    int32_t enable = 0;

    if (this->NeedsUpdate(this->m_appMasterEnables, this->m_hwMasterEnables, 0, 0, GxMasterEnable_PolygonFill, enable)) {
        if (enable) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
    }

    this->m_hwMasterEnables = this->m_appMasterEnables;
}

// VERIFY: 289ca54b auto
// OFFSET: 0x00692240
void CGxDeviceOpenGl::IStateSyncClipPlanes() {
    if (!this->m_clipPlaneMask) {
        return;
    }

    for (uint32_t i = 0; i < 6; i++) {
        if ((1 << i) & this->m_clipPlaneMask) {
            GLdouble equation[4];
            equation[0] = this->m_clipPlane[i].n.x;
            equation[1] = this->m_clipPlane[i].n.y;
            equation[2] = this->m_clipPlane[i].n.z;
            equation[3] = this->m_clipPlane[i].d;
            glClipPlane(GL_CLIP_PLANE0 + i, equation);
        }
    }

    this->m_clipPlaneMask = 0;
}

// VERIFY: abab714f manual
// OFFSET: 0x006922C0
void CGxDeviceOpenGl::IStateSyncScissorRect() {
    if (!this->m_scissorRectDirty) {
        return;
    }

    const CRect& window = this->DeviceCurWindow();

    float width;
    float height;

    if (this->m_textureTarget[GxBuffers_Color].m_apiSpecific) {
        CGxTex* target = static_cast<CGxTex*>(this->m_textureTarget[GxBuffers_Color].m_apiSpecific);
        width = static_cast<float>(target->m_width);
        height = static_cast<float>(target->m_height);
    } else {
        width = window.maxX;
        height = window.maxY;
    }

    GLint x = static_cast<GLint>(this->m_scissorRect.minX * width + 0.5f);
    GLint y = static_cast<GLint>(this->m_scissorRect.minY * height + 0.5f);
    glScissor(x, y, static_cast<GLint>(width * this->m_scissorRect.maxX + 0.5f) - x, static_cast<GLint>(height * this->m_scissorRect.maxY + 0.5f) - y);
    this->m_scissorRectDirty = 0;
}

// VERIFY: 189ffd89 manual
// OFFSET: 0x006923A0
void CGxDeviceOpenGl::ISetTexture(uint32_t tmu, CGxTex* texId) {
    if (tmu >= static_cast<uint32_t>(this->m_caps.m_numTmus)) {
        return;
    }

    this->DsSet(Ds_ActiveTexture, tmu, 0);

    if (texId) {
        this->DsSet(static_cast<EDeviceState>(Ds_TexTarget0 + tmu), s_glTexTarget[texId->m_target], 0);
        this->BindTexture(texId, tmu);
        this->ITexMarkAsUpdated(texId, tmu);
    } else {
        this->DsSet(static_cast<EDeviceState>(Ds_TexTarget0 + tmu), 0, 0);
    }
}

// VERIFY: e12b3ec3 manual
// OFFSET: 0x00693020
void CGxDeviceOpenGl::ISetColorOp(uint32_t tmu, uint32_t op) {
    if (tmu >= static_cast<uint32_t>(this->m_caps.m_numTmus)) {
        return;
    }

    this->DsSet(Ds_ActiveTexture, tmu, 0);
    this->DsSet(static_cast<EDeviceState>(Ds_ColorOp0 + tmu), op, 0);
}

// VERIFY: 1f506815 manual
// OFFSET: 0x00693730
void CGxDeviceOpenGl::ISetAlphaOp(uint32_t tmu, uint32_t op) {
    if (tmu >= static_cast<uint32_t>(this->m_caps.m_numTmus)) {
        return;
    }

    this->DsSet(Ds_ActiveTexture, tmu, 0);
    this->DsSet(static_cast<EDeviceState>(Ds_AlphaOp0 + tmu), op, 0);
}

// VERIFY: 5f976d96 manual
// OFFSET: 0x00693E40
void CGxDeviceOpenGl::IStateSyncVertexPtrs() {
    if (!this->m_primVertexDirty) {
        return;
    }

    uint32_t bit = 1;

    for (uint32_t attrib = 0; attrib < GxVAs_Last; attrib++, bit <<= 1) {
        if (!(bit & this->m_primVertexDirty)) {
            continue;
        }

        if (!(bit & this->m_primVertexMask)) {
            switch (attrib) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                    this->DsSet(static_cast<EDeviceState>(Ds_VertexArray0 + attrib), 0, 0);
                    break;

                default: {
                    uint32_t tmu = attrib - 6;

                    if (tmu < static_cast<uint32_t>(this->m_caps.m_numTmus)) {
                        this->DsSet(Ds_ActiveTexture, tmu, 0);
                        this->DsSet(static_cast<EDeviceState>(Ds_TexCoordArray0 + tmu), 0, 0);
                    }

                    break;
                }
            }

            continue;
        }

        CGxVertexAttrib& vertexAttrib = this->m_primVertexFormatAttrib[attrib];
        CGxBuf* buf = this->m_primVertexFormatBuf[attrib];
        CGxPool* pool = buf->m_pool;
        uintptr_t base;

        if (glARBVertexBufferObject && this->m_useVboBroker[pool->m_target][pool->m_usage]) {
            GLuint vbo;
            uint32_t offset;
            static_cast<CGxVboBroker*>(pool->m_apiSpecific)->GetLastSelectedVbo(vbo, offset);

            if (this->m_deviceStates[Ds_ArrayBuffer] != vbo) {
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
                this->m_deviceStates[Ds_ArrayBuffer] = vbo;
            }

            base = buf->m_index;
        } else if (glARBVertexBufferObject && this->m_useVbo[pool->m_target][pool->m_usage]) {
            GLuint name = static_cast<GLuint>(reinterpret_cast<uintptr_t>(pool->m_apiSpecific));

            if (this->m_deviceStates[Ds_ArrayBuffer] != name) {
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, name);
                this->m_deviceStates[Ds_ArrayBuffer] = name;
            }

            base = buf->m_index;
        } else {
            base = buf->m_index + reinterpret_cast<uintptr_t>(pool->m_apiSpecific);
        }

        const GLvoid* pointer = reinterpret_cast<const GLvoid*>(base + vertexAttrib.m_offset);

        if (!this->m_deviceStates[Ds_VertexProgram]) {
            switch (attrib) {
                case 0:
                    this->DsSet(Ds_VertexArray0, 1, 0);
                    glVertexPointer(s_glAttribSize[vertexAttrib.m_type], s_glAttribType[vertexAttrib.m_type], vertexAttrib.m_bufSize, pointer);
                    break;

                case 3:
                    glNormalPointer(s_glAttribType[vertexAttrib.m_type], vertexAttrib.m_bufSize, pointer);
                    this->DsSet(static_cast<EDeviceState>(Ds_VertexArray0 + 3), 1, 0);
                    break;

                case 4:
                    glColorPointer(s_glAttribSize[vertexAttrib.m_type], s_glAttribType[vertexAttrib.m_type], vertexAttrib.m_bufSize, pointer);
                    this->DsSet(static_cast<EDeviceState>(Ds_VertexArray0 + 4), 1, 0);
                    break;

                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                case 12:
                case 13: {
                    uint32_t tmu = 0;

                    while (tmu < 8 && this->m_appRenderStates[GxRs_TextureCoord0 + tmu].m_value.m_data.u[0] != attrib - 6) {
                        tmu++;
                    }

                    if (tmu < static_cast<uint32_t>(this->m_caps.m_numTmus)) {
                        this->DsSet(Ds_ActiveTexture, tmu, 0);
                        glTexCoordPointer(s_glAttribSize[vertexAttrib.m_type], s_glAttribType[vertexAttrib.m_type], vertexAttrib.m_bufSize, pointer);
                        this->DsSet(static_cast<EDeviceState>(Ds_TexCoordArray0 + tmu), 1, 0);
                    }

                    break;
                }

                default:
                    break;
            }
        } else {
            switch (attrib) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                    glVertexAttribPointerARB(attrib, s_glAttribSize[vertexAttrib.m_type], s_glAttribType[vertexAttrib.m_type], s_glAttribNormalized[vertexAttrib.m_type], vertexAttrib.m_bufSize, pointer);
                    this->DsSet(static_cast<EDeviceState>(Ds_VertexArray0 + attrib), 1, 0);
                    break;

                default: {
                    uint32_t tmu = 0;

                    while (tmu < 8 && this->m_appRenderStates[GxRs_TextureCoord0 + tmu].m_value.m_data.u[0] != attrib - 6) {
                        tmu++;
                    }

                    if (tmu < static_cast<uint32_t>(this->m_caps.m_numTmus)) {
                        this->DsSet(Ds_ActiveTexture, tmu, 0);
                        glVertexAttribPointerARB(attrib, s_glAttribSize[vertexAttrib.m_type], s_glAttribType[vertexAttrib.m_type], s_glAttribNormalized[vertexAttrib.m_type], vertexAttrib.m_bufSize, pointer);
                        this->DsSet(static_cast<EDeviceState>(Ds_VertexArray0 + attrib), 1, 0);
                    }

                    break;
                }
            }
        }
    }

    this->m_primVertexDirty = 0;
}

// VERIFY: 9806329f manual
// OFFSET: 0x00695640
void CGxDeviceOpenGl::IStateSyncLights() {
    if (this->m_appRenderStates[GxRs_Lighting].m_value.m_data.i[0]) {
        if (this->m_ambientOnlyMode) {
            this->m_ambientOnlyMode = 0;
        }

        if (this->m_appMasterEnables & 0x1) {
            glEnable(GL_LIGHTING);
        } else {
            glDisable(GL_LIGHTING);
        }

        GLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

        int32_t modelViewSet = 0;
        uint16_t mask = this->m_appRenderStates[GxRs_MatSpecularExp].m_value.m_data.f[0] == 0.0f ? 0xFFEF : 0xFFFF;
        uint16_t force = this->m_viewDirty ? 2 : 0;
        GLenum glLight = GL_LIGHT0;

        for (uint32_t i = 0; i < 4; i++, glLight++) {
            CGxApiLight& light = this->m_lights[i];
            uint16_t flags = force | (mask & light.m_flags & light.unk_0046);

            if (!flags) {
                continue;
            }

            if (flags & 0x1) {
                if (!light.m_enable) {
                    light.m_flags &= ~0x1;
                    glDisable(glLight);
                    continue;
                }

                glEnable(glLight);
            } else if (!light.m_enable) {
                continue;
            }

            if (flags & 0x2) {
                if (!modelViewSet) {
                    CGxMatrixStack& view = this->m_xforms[GxXform_View];
                    this->IXformSetModelView(view.m_mtx[view.m_level]);
                    modelViewSet = 1;
                }

                if (light.m_dir.w == 1.0f) {
                    glLightfv(glLight, GL_POSITION, &light.m_dir.x);
                } else {
                    GLfloat direction[4];
                    direction[0] = -light.m_dir.x;
                    direction[1] = -light.m_dir.y;
                    direction[2] = -light.m_dir.z;
                    direction[3] = -light.m_dir.w;
                    glLightfv(glLight, GL_POSITION, direction);
                }
            }

            if (flags & 0x20) {
                glLightf(glLight, GL_CONSTANT_ATTENUATION, light.m_constantAttenuation);
            }

            if (flags & 0x40) {
                glLightf(glLight, GL_LINEAR_ATTENUATION, light.m_linearAttenuation);
            }

            if (flags & 0x80) {
                glLightf(glLight, GL_QUADRATIC_ATTENUATION, light.m_quadraticAttenuation);
            }

            if (flags & 0x4) {
                GLfloat color[4];
                color[0] = light.m_ambColor.x;
                color[1] = light.m_ambColor.y;
                color[2] = light.m_ambColor.z;
                color[3] = 0.0f;
                glLightfv(glLight, GL_AMBIENT, color);
            }

            if (flags & 0x8) {
                GLfloat color[4];
                color[0] = light.m_dirColor.x;
                color[1] = light.m_dirColor.y;
                color[2] = light.m_dirColor.z;
                color[3] = 1.0f;
                glLightfv(glLight, GL_DIFFUSE, color);
            }

            if (flags & 0x10) {
                GLfloat color[4];
                color[0] = light.m_specColor.x;
                color[1] = light.m_specColor.y;
                color[2] = light.m_specColor.z;
                color[3] = 0.0f;
                glLightfv(glLight, GL_SPECULAR, color);
            }

            light.m_flags &= ~flags;
        }

        this->m_viewDirty |= modelViewSet;
        return;
    }

    if ((this->m_primVertexMask & (1 << GxVA_Color0)) || this->m_appRenderStates[GxRs_MatDiffuse].m_value.m_data.u[0] == 0xFFFFFFFF) {
        if (this->m_ambientOnlyMode) {
            this->m_ambientOnlyMode = 0;
        }

        glDisable(GL_LIGHTING);

        GLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    } else {
        if (!this->m_ambientOnlyMode) {
            this->m_ambientOnlyMode = 1;

            for (uint32_t i = 0; i < 4; i++) {
                glDisable(GL_LIGHT0 + i);

                if (this->m_lights[i].m_enable) {
                    this->m_lights[i].m_flags |= 0x1;
                } else {
                    this->m_lights[i].m_flags &= ~0x1;
                }
            }
        }

        glEnable(GL_LIGHTING);

        GLfloat ambient[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    }
}

// VERIFY: 1c3f3488 manual
// OFFSET: 0x00695960
void CGxDeviceOpenGl::IStateSyncMaterial() {
    int32_t colorMaterial = this->m_appRenderStates[GxRs_ColorMaterial].m_value.m_data.i[0];

    if (this->m_primVertexDirty & (1 << GxVA_Color0)) {
        this->m_materialDirty |= 1 << colorMaterial;
    }

    if (!this->m_materialDirty) {
        return;
    }

    GLfloat color[4];

    if (this->m_materialDirty & 0x1) {
        ColorToFloats(color, reinterpret_cast<const CImVector&>(this->m_appRenderStates[GxRs_MatDiffuse].m_value.m_data.u[0]));

        if (colorMaterial) {
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
        } else if (!(this->m_primVertexMask & (1 << GxVA_Color0))) {
            glColor4fv(color);
        }
    }

    if (this->m_materialDirty & 0x2) {
        ColorToFloats(color, reinterpret_cast<const CImVector&>(this->m_appRenderStates[GxRs_MatSpecular].m_value.m_data.u[0]));

        if (colorMaterial == 1) {
            if (!(this->m_primVertexMask & (1 << GxVA_Color0))) {
                glColor4fv(color);
            }
        } else {
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
        }
    }

    if (this->m_materialDirty & 0x4) {
        ColorToFloats(color, reinterpret_cast<const CImVector&>(this->m_appRenderStates[GxRs_MatEmissive].m_value.m_data.u[0]));

        if (colorMaterial == 2) {
            if (!(this->m_primVertexMask & (1 << GxVA_Color0))) {
                glColor4fv(color);
                this->m_materialDirty = 0;
                return;
            }
        } else {
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);
        }
    }

    this->m_materialDirty = 0;
}

// VERIFY: 3bd05b04 manual
// OFFSET: 0x00695AC0
void CGxDeviceOpenGl::IStateSyncXforms() {
    if (this->m_viewDirty) {
        CGxMatrixStack& world = this->m_xforms[GxXform_World];
        CGxMatrixStack& view = this->m_xforms[GxXform_View];
        this->IXformSetModelView(world.m_mtx[world.m_level] * view.m_mtx[view.m_level]);
        world.m_dirty = 0;
        view.m_dirty = 0;
    }

    for (uint32_t tmu = 0; tmu < static_cast<uint32_t>(this->m_caps.m_numTmus); tmu++) {
        CGxTex* texture = static_cast<CGxTex*>(this->m_appRenderStates[GxRs_Texture0 + tmu].m_value.m_data.p);
        CGxMatrixStack& xform = this->m_xforms[tmu];
        CGxMatrixStack& texGen = this->m_texGen[tmu];

        if (!texture || (!xform.m_dirty && !texGen.m_dirty && texture->m_target != GxTex_Rectangle)) {
            continue;
        }

        this->DsSet(Ds_ActiveTexture, tmu, 0);
        this->DsSet(Ds_MatrixMode, GL_TEXTURE, 0);

        // STATIC: 0x00C6062C
        static C44Matrix s_matrix;

        int32_t identity;

        if (static_cast<uint32_t>(this->m_appRenderStates[GxRs_TextureShader0 + tmu].m_value.m_data.i[0] - 1) > 1) {
            identity = 1;
            s_matrix.Identity();
        } else {
            identity = 0;
            s_matrix = texGen.m_mtx[texGen.m_level] * xform.m_mtx[xform.m_level];
        }

        xform.m_dirty = 0;
        texGen.m_dirty = 0;

        if (texture->m_target == GxTex_Rectangle) {
            C3Vector scale;
            scale.x = static_cast<float>(texture->m_width);
            scale.y = static_cast<float>(texture->m_height);
            scale.z = 1.0f;
            s_matrix.Scale(scale);
            texGen.m_dirty = 1;
        } else if (identity) {
            glLoadIdentity();
            continue;
        }

        glLoadMatrixf(&s_matrix.a0);
    }

    this->DsSet(Ds_MatrixMode, GL_MODELVIEW, 0);
}

// VERIFY: aa4d50f5 manual
// OFFSET: 0x00695DC0
void CGxDeviceOpenGl::IStateSetContextDefaults() {
    this->DsInit();

    GLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    glMatrixMode(GL_MODELVIEW);
    this->m_deviceStates[Ds_MatrixMode] = GL_MODELVIEW;
    glLoadIdentity();

    GLfloat planeR[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    GLfloat planeQ[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    for (uint32_t tmu = 0; tmu != static_cast<uint32_t>(this->m_caps.m_numTmus); tmu++) {
        if (static_cast<uint32_t>(this->m_caps.m_numTmus) > 1) {
            glActiveTextureARB(GL_TEXTURE0_ARB + tmu);
            glClientActiveTextureARB(GL_TEXTURE0_ARB + tmu);
        }

        this->m_deviceStates[Ds_ActiveTexture] = tmu;
        glTexGenfv(GL_R, GL_EYE_PLANE, planeR);
        glTexGenfv(GL_Q, GL_EYE_PLANE, planeQ);
        glTexGenfv(GL_R, GL_OBJECT_PLANE, planeR);
        glTexGenfv(GL_Q, GL_OBJECT_PLANE, planeQ);
    }

    if (glARBPointSprite) {
        for (uint32_t tmu = 0; tmu < static_cast<uint32_t>(this->m_caps.m_numTmus); tmu++) {
            glActiveTextureARB(GL_TEXTURE0_ARB + tmu);
            glClientActiveTextureARB(GL_TEXTURE0_ARB + tmu);
            glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
        }
    }

    this->InitLights();
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

    CGxMatrixStack& world = this->m_xforms[GxXform_World];
    CGxMatrixStack& view = this->m_xforms[GxXform_View];
    this->IXformSetModelView(world.m_mtx[world.m_level] * view.m_mtx[view.m_level]);
    this->m_viewDirty = 0;

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_SCISSOR_TEST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_DENSITY, 0.0f);
}

// VERIFY: 02fd48bc manual
// OFFSET: 0x00695FD0
void CGxDeviceOpenGl::ISetTexGen(uint32_t tmu, int32_t texGen) {
    if (tmu >= static_cast<uint32_t>(this->m_caps.m_numTmus)) {
        return;
    }

    this->DsSet(Ds_ActiveTexture, tmu, 0);

    switch (texGen) {
        case GxTexGen_Disabled:
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenS0 + tmu), 0, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenT0 + tmu), 0, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenR0 + tmu), 0, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenQ0 + tmu), 0, 0);
            break;

        case GxTexGen_ObjectLinear:
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenS0 + tmu), GL_OBJECT_LINEAR, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenT0 + tmu), GL_OBJECT_LINEAR, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenR0 + tmu), GL_OBJECT_LINEAR, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenQ0 + tmu), 0, 0);
            break;

        case GxTexGen_World:
        case GxTexGen_View:
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenS0 + tmu), GL_EYE_LINEAR, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenT0 + tmu), GL_EYE_LINEAR, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenR0 + tmu), GL_EYE_LINEAR, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenQ0 + tmu), 0, 0);
            break;

        case GxTexGen_ReflectionMap:
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenS0 + tmu), GL_REFLECTION_MAP_ARB, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenT0 + tmu), GL_REFLECTION_MAP_ARB, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenR0 + tmu), GL_REFLECTION_MAP_ARB, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenQ0 + tmu), 0, 0);
            break;

        case GxTexGen_Normal:
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenS0 + tmu), GL_NORMAL_MAP_ARB, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenT0 + tmu), GL_NORMAL_MAP_ARB, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenR0 + tmu), GL_NORMAL_MAP_ARB, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenQ0 + tmu), 0, 0);
            break;

        case GxTexGen_SphereMap:
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenS0 + tmu), this->m_sphereMapMode, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenT0 + tmu), this->m_sphereMapMode, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenR0 + tmu), 0, 0);
            this->DsSet(static_cast<EDeviceState>(Ds_TexGenQ0 + tmu), 0, 0);
            break;

        default:
            break;
    }

    if (texGen == GxTexGen_World) {
        CGxMatrixStack& stack = this->m_texGen[tmu];
        stack.m_dirty = 1;
        stack.m_flags[stack.m_level] &= ~CGxMatrixStack::F_Identity;

        C44Matrix& matrix = stack.m_mtx[stack.m_level];
        CGxMatrixStack& view = this->m_xforms[GxXform_View];
        this->IXformGLModelView(view.m_mtx[view.m_level], matrix);
        matrix = matrix.AffineInverse();
    } else {
        this->m_texGen[tmu].Identity();
    }
}

// VERIFY: aa2404af manual
// OFFSET: 0x0069CDC0
void CGxDeviceOpenGl::IRsSendToHw(EGxRenderState which) {
    const CGxStateBom& value = this->m_appRenderStates[which].m_value;

    switch (which) {
        case GxRs_PolygonOffset:
            if (value.m_data.f[0] == 0.0f) {
                this->DsSet(Ds_PolygonOffsetFill, 0, 0);
            } else {
                this->DsSet(Ds_PolygonOffset, value.m_data.u[0], 0);
                this->DsSet(Ds_PolygonOffsetFill, 1, 0);
            }

            break;

        case GxRs_MatDiffuse:
            this->m_materialDirty |= 0x1;
            break;

        case GxRs_MatEmissive:
            this->m_materialDirty |= 0x4;
            break;

        case GxRs_MatSpecular:
            this->m_materialDirty |= 0x2;
            break;

        case GxRs_MatSpecularExp:
            glMaterialf(GL_FRONT, GL_SHININESS, value.m_data.f[0]);
            break;

        case GxRs_NormalizeNormals:
            if (value.m_data.i[0]) {
                glEnable(GL_NORMALIZE);
            } else {
                glDisable(GL_NORMALIZE);
            }

            break;

        case GxRs_BlendingMode: {
            uint32_t mode = value.m_data.u[0];

            if (mode < 2) {
                this->DsSet(Ds_Blend, 0, 0);
            } else {
                this->DsSet(Ds_Blend, 1, 0);
                this->DsSet(Ds_BlendFunc, s_glBlendDst[mode] | (s_glBlendSrc[mode] << 16), 0);
            }

            break;
        }

        case GxRs_AlphaRef:
            if (value.m_data.i[0] <= 0) {
                this->DsSet(Ds_AlphaTest, 0, 0);
            } else {
                glAlphaFunc(GL_GEQUAL, value.m_data.i[0] * oo255);
                this->DsSet(Ds_AlphaTest, 1, 0);
            }

            break;

        case GxRs_FogStart:
            glFogf(GL_FOG_START, value.m_data.f[0]);
            break;

        case GxRs_FogEnd:
            glFogf(GL_FOG_END, value.m_data.f[0]);
            break;

        case GxRs_FogColor: {
            GLfloat color[4];
            ColorToFloats(color, reinterpret_cast<const CImVector&>(value.m_data.u[0]));
            glFogfv(GL_FOG_COLOR, color);
            break;
        }

        case GxRs_Lighting:
            if ((this->m_appMasterEnables & 0x1) && value.m_data.i[0]) {
                glEnable(GL_LIGHTING);
            } else {
                glDisable(GL_LIGHTING);
            }

            break;

        case GxRs_Fog:
            if ((this->m_appMasterEnables & 0x2) && value.m_data.i[0]) {
                glEnable(GL_FOG);
            } else {
                glDisable(GL_FOG);
            }

            break;

        case GxRs_DepthTest:
            if ((this->m_appMasterEnables & 0x4) && value.m_data.i[0]) {
                glEnable(GL_DEPTH_TEST);
            } else {
                glDisable(GL_DEPTH_TEST);
            }

            break;

        case GxRs_DepthFunc:
            glDepthFunc(s_glDepthFunc[value.m_data.i[0]]);
            break;

        case GxRs_DepthWrite:
            this->DsSet(Ds_DepthMask, (this->m_appMasterEnables & 0x8) ? value.m_data.u[0] : 0, 0);
            break;

        case GxRs_ColorWrite:
            this->DsSet(Ds_ColorMask, (this->m_appMasterEnables & 0x10) ? value.m_data.u[0] : 0, 0);
            break;

        case GxRs_Culling: {
            if (!(this->m_appMasterEnables & 0x20)) {
                glDisable(GL_CULL_FACE);
                break;
            }

            uint32_t culling = value.m_data.u[0];

            if (culling <= 2) {
                if (!culling) {
                    glDisable(GL_CULL_FACE);
                    break;
                }
            } else {
                culling = 2;
            }

            glEnable(GL_CULL_FACE);

            if (culling == 1) {
                glFrontFace(GL_CCW);
            } else {
                glFrontFace(GL_CW);
            }

            break;
        }

        case GxRs_ClipPlaneMask:
            this->DsSet(Ds_ClipPlanes, value.m_data.u[0], 0);
            break;

        case GxRs_Multisample:
            if (glARBMultisample) {
                if (value.m_data.i[0]) {
                    glEnable(GL_MULTISAMPLE_ARB);
                } else {
                    glDisable(GL_MULTISAMPLE_ARB);
                }
            }

            break;

        case GxRs_ScissorTest:
            if (value.m_data.i[0]) {
                glEnable(GL_SCISSOR_TEST);
            } else {
                glDisable(GL_SCISSOR_TEST);
            }

            break;

        case GxRs_Texture0:
        case GxRs_Texture1:
        case GxRs_Texture2:
        case GxRs_Texture3:
        case GxRs_Texture4:
        case GxRs_Texture5:
        case GxRs_Texture6:
        case GxRs_Texture7:
        case GxRs_Texture8:
        case GxRs_Texture9:
        case GxRs_Texture10:
        case GxRs_Texture11:
        case GxRs_Texture12:
        case GxRs_Texture13:
        case GxRs_Texture14:
        case GxRs_Texture15:
            this->ISetTexture(which - GxRs_Texture0, static_cast<CGxTex*>(value.m_data.p));
            break;

        case GxRs_ColorOp0:
        case GxRs_ColorOp1:
        case GxRs_ColorOp2:
        case GxRs_ColorOp3:
        case GxRs_ColorOp4:
        case GxRs_ColorOp5:
        case GxRs_ColorOp6:
        case GxRs_ColorOp7:
            this->ISetColorOp(which - GxRs_ColorOp0, value.m_data.u[0]);
            break;

        case GxRs_AlphaOp0:
        case GxRs_AlphaOp1:
        case GxRs_AlphaOp2:
        case GxRs_AlphaOp3:
        case GxRs_AlphaOp4:
        case GxRs_AlphaOp5:
        case GxRs_AlphaOp6:
        case GxRs_AlphaOp7:
            this->ISetAlphaOp(which - GxRs_AlphaOp0, value.m_data.u[0]);
            break;

        case GxRs_TexGen0:
        case GxRs_TexGen1:
        case GxRs_TexGen2:
        case GxRs_TexGen3:
        case GxRs_TexGen4:
        case GxRs_TexGen5:
        case GxRs_TexGen6:
        case GxRs_TexGen7:
            this->ISetTexGen(which - GxRs_TexGen0, value.m_data.i[0]);
            break;

        case GxRs_VertexShader:
            this->IBindVertexShader(static_cast<CGxShader*>(value.m_data.p));
            break;

        case GxRs_PixelShader:
            this->IBindPixelShader(static_cast<CGxShader*>(value.m_data.p));
            break;

        case GxRs_PointScale:
            glPointSize(value.m_data.f[0]);
            break;

        case GxRs_PointScaleAttenuation:
            if (glARBPointParameters) {
                glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION_ARB, value.m_data.f);
            }

            break;

        case GxRs_PointScaleMin:
            if (glARBPointParameters) {
                glPointParameterfARB(GL_POINT_SIZE_MIN_ARB, value.m_data.f[0]);
            }

            break;

        case GxRs_PointScaleMax:
            if (glARBPointParameters) {
                glPointParameterfARB(GL_POINT_SIZE_MAX_ARB, value.m_data.f[0]);
            }

            break;

        case GxRs_PointSprite:
            if (!glARBPointSprite) {
                break;
            }

            if (value.m_data.i[0]) {
                glEnable(GL_POINT_SPRITE_ARB);
            } else {
                glDisable(GL_POINT_SPRITE_ARB);
            }

            if (value.m_data.i[0]) {
                glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
            } else {
                glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
            }

            break;

        case GxRs_BlendFactor:
            if (glExtBlendColor) {
                glBlendColorEXT(value.m_data.f[0], value.m_data.f[0], value.m_data.f[0], value.m_data.f[0]);
            }

            break;

        case GxRs_ColorMaterial:
            glColorMaterial(GL_FRONT_AND_BACK, s_glColorMaterial[value.m_data.i[0]]);
            this->m_materialDirty = 7;
            break;

        default:
            break;
    }
}

// VERIFY: 7dd5571d auto
// OFFSET: 0x0069D550
void CGxDeviceOpenGl::IStateSync() {
    this->m_viewDirty |= this->m_xforms[GxXform_World].m_dirty | this->m_xforms[GxXform_View].m_dirty;
    this->IShaderConstantsFlush();
    this->IRsSync(0);
    this->IStateSyncLights();
    this->IStateSyncEnables();
    this->IStateSyncMaterial();
    this->IStateSyncXforms();
    this->m_viewDirty = 0;
    this->IStateSyncClipPlanes();
    this->IStateSyncScissorRect();
    this->IStateSyncViewport();
    this->IStateSyncVertexPtrs();
    this->IStateSyncIndexPtr();
}

// VERIFY: b88c4866 manual
// OFFSET: 0x0069E150
void CGxDeviceOpenGl::SceneClear(uint32_t mask, CImVector color) {
    glClearColor(color.r * oo255, color.g * oo255, color.b * oo255, color.a * oo255);

    uint32_t depthMask = this->m_deviceStates[Ds_DepthMask];
    this->DsSet(Ds_DepthMask, 1, 0);
    uint32_t colorMask = this->m_deviceStates[Ds_ColorMask];
    this->DsSet(Ds_ColorMask, 0xF, 0);

    GLbitfield bits = 0;

    if (mask & GxBufferBit_Color) {
        bits = GL_COLOR_BUFFER_BIT;
    }

    if (mask & GxBufferBit_Depth) {
        bits |= GL_DEPTH_BUFFER_BIT;
    }

    this->IStateSyncViewport();
    glClear(bits);
    this->DsSet(Ds_DepthMask, depthMask, 0);
    this->DsSet(Ds_ColorMask, colorMask, 0);
}

// VERIFY: a13dc98e manual
// OFFSET: 0x0069E220
void CGxDeviceOpenGl::ScenePresent() {
    int32_t screenShot = this->m_scrShotClick;
    CGxDevice::ScenePresent();

    if (screenShot) {
        this->DeviceScreenShot();
    }

    this->ICursorDraw();

    if (this->m_hwMasterEnables & 0x40) {
        wglSwapLayerBuffers(this->m_hdc, WGL_SWAP_MAIN_PLANE);
    } else {
        glFinish();
    }

    this->WaitForFPSCap();
    this->IShaderConstantsClear();
}

// VERIFY: 51ea4b76 manual
// OFFSET: 0x0069E280
void CGxDeviceOpenGl::IXformSetProjection(const C44Matrix& matrix) {
    C44Matrix projection(matrix.a0, matrix.a1, matrix.a2, matrix.a3, matrix.b0, matrix.b1, matrix.b2, matrix.b3, -matrix.c0, -matrix.c1, -matrix.c2, -matrix.c3, matrix.d0, matrix.d1, matrix.d2, matrix.d3);

    if (!(this->m_appMasterEnables & (1 << GxMasterEnable_NormalProjection)) && 1.0f != matrix.d3) {
        C44Matrix scale(0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        projection = projection * scale;
    }

    if (this->m_textureTarget[GxBuffers_Color].m_apiSpecific || this->m_textureTarget[GxBuffers_Depth].m_apiSpecific) {
        if (!g_theGxDevicePtr->Caps().unk_00B0) {
            projection.a1 = projection.a1 * -1.0f;
            projection.b1 = projection.b1 * -1.0f;
            projection.c1 = projection.c1 * -1.0f;
            projection.d1 = -1.0f * projection.d1;
            this->DsSet(Ds_CullFace, GL_FRONT, 0);
        }
    } else if (!g_theGxDevicePtr->Caps().unk_00B0) {
        this->DsSet(Ds_CullFace, GL_BACK, 0);
    }

    this->DsSet(Ds_MatrixMode, GL_PROJECTION, 0);
    glLoadMatrixf(&projection.a0);
    this->m_projNative = projection;
}

// VERIFY: 52a98c65 auto
// OFFSET: 0x0069E410
void CGxDeviceOpenGl::IXformGLModelView(const C44Matrix& view, C44Matrix& world) {
    world = C44Matrix(view.a0, view.a1, -view.a2, view.a3, view.b0, view.b1, -view.b2, view.b3, view.c0, view.c1, -view.c2, view.c3, view.d0, view.d1, -view.d2, view.d3);
}

// VERIFY: 9658e8ef auto
// OFFSET: 0x0069E4A0
void CGxDeviceOpenGl::IXformSetModelView(const C44Matrix& matrix) {
    C44Matrix modelView;
    this->IXformGLModelView(matrix, modelView);
    this->DsSet(Ds_MatrixMode, GL_MODELVIEW, 0);
    glLoadMatrixf(&modelView.a0);
}

// VERIFY: 0739bf05 auto
// OFFSET: 0x0069E510
void CGxDeviceOpenGl::XformSetProjection(const C44Matrix& matrix) {
    this->m_projection = matrix;
    this->IXformSetProjection(matrix);
}

// VERIFY: ffc7fd68 auto
// OFFSET: 0x0069E540
void CGxDeviceOpenGl::XformSetView(const C44Matrix& matrix) {
    CGxDevice::XformSetView(matrix);
    this->m_viewDirty = 1;
}

// VERIFY: 9e20c8ec auto
// OFFSET: 0x0069E560
void CGxDeviceOpenGl::Draw(CGxBatch* batch, int32_t count) {
    if (this->m_intF5C) {
        return;
    }

    this->IStateSync();

    if (count) {
        if (glExtDrawRangeElements) {
            glDrawRangeElementsEXT(s_glPrim[batch->m_primType], batch->m_minIndex, batch->m_maxIndex, batch->m_count, GL_UNSIGNED_SHORT, reinterpret_cast<const GLvoid*>(this->m_indexPtr + sizeof(uint16_t) * batch->m_start));
        } else {
            glDrawElements(s_glPrim[batch->m_primType], batch->m_count, GL_UNSIGNED_SHORT, reinterpret_cast<const GLvoid*>(this->m_indexPtr + sizeof(uint16_t) * batch->m_start));
        }
    } else {
        glDrawArrays(s_glPrim[batch->m_primType], batch->m_start, batch->m_count);
    }
}

// VERIFY: c8a42429 auto
// OFFSET: 0x0069E620
void CGxDeviceOpenGl::PrimPointSize(float size) {
    glPointSize(size);
}

// VERIFY: 7eee7edc auto
// OFFSET: 0x0069E640
void CGxDeviceOpenGl::PrimLineWidth(float width) {
    glLineWidth(width);
}

// VERIFY: 9ddcedf6 manual
// OFFSET: 0x0069D5D0
void CGxDeviceOpenGl::BindTexture(CGxTex* texId, uint32_t tmu) {
    if (tmu == kNullTmu) {
        this->DsSet(Ds_ActiveTexture, 0, 0);
        this->IRsForceUpdate(GxRs_Texture0);
    }

    GLuint name = static_cast<GLuint>(reinterpret_cast<uintptr_t>(texId->m_apiSpecificData));
    glBindTexture(s_glTexTarget[texId->m_target], name);
    this->m_boundTexture = name;
}

// VERIFY: 941b6b87 auto
// OFFSET: 0x0069D620
void CGxDeviceOpenGl::ITexSetFlags(CGxTex* texId) {
    GLenum target = s_glTexTarget[texId->m_target];
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, s_glMagFilter[texId->m_flags.m_filter]);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, s_glMinFilter[texId->m_flags.m_filter]);

    if (glExtClampToEdge) {
        glTexParameteri(target, GL_TEXTURE_WRAP_S, texId->m_flags.m_wrapU ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, texId->m_flags.m_wrapV ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    } else {
        glTexParameteri(target, GL_TEXTURE_WRAP_S, texId->m_flags.m_wrapU ? GL_REPEAT : GL_CLAMP);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, texId->m_flags.m_wrapV ? GL_REPEAT : GL_CLAMP);
    }

    if (glSGISGenerateMipmap && texId->m_target != GxTex_Rectangle) {
        glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, texId->m_flags.m_generateMipMaps);
    }

    if (glExtTextureFilterAnisotropic) {
        glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, texId->m_flags.m_maxAnisotropy);
    }

    texId->m_needsFlagUpdate = 0;
}

// VERIFY: 453d4d93 auto
// OFFSET: 0x0069D720
void CGxDeviceOpenGl::TexDestroy(CGxTex* texId) {
    if (texId->m_apiSpecificData) {
        glDeleteTextures(1, reinterpret_cast<GLuint*>(&texId->m_apiSpecificData));
    }

    CGxDevice::TexDestroy(texId);
}

// VERIFY: 7d611939 manual
// OFFSET: 0x0069D750
int32_t CGxDeviceOpenGl::TexCopy(CGxTex* dest, const C2iVector& destPos, const C2iVector& sourcePos, const C2iVector& size, uint32_t level, uint32_t plane) {
    if (!this->m_context) {
        return 0;
    }

    GLenum target = s_glTexTarget[dest->m_target];

    if (dest->m_target == GxTex_CubeMap) {
        target = s_glCubeFaces[plane];
    }

    if (!dest->m_apiSpecificData) {
        glGenTextures(1, reinterpret_cast<GLuint*>(&dest->m_apiSpecificData));
    }

    this->BindTexture(dest, kNullTmu);

    if (dest->m_needsFlagUpdate) {
        this->ITexSetFlags(dest);
    }

    if (dest->m_needsCreation) {
        glCopyTexImage2D(target, 0, s_glInternalFormat[dest->m_format], 0, 0, dest->m_width, dest->m_height, 0);
        dest->m_needsCreation = 0;
    }

    glCopyTexSubImage2D(target, level, destPos.x, destPos.y, sourcePos.x, sourcePos.y, size.x, size.y);

    return 1;
}

// VERIFY: 2f7659c3 manual
// OFFSET: 0x0069D820
void CGxDeviceOpenGl::ITexForceRecreation() {
    for (CGxTex* texId = this->m_textures.Head(); texId; texId = this->m_textures.Next(texId)) {
        if (!texId->m_apiSpecificData) {
            continue;
        }

        // STATIC: 0x00C6066C
        static CiRect s_updateRect;

        if (texId->m_flags.m_renderTarget && texId->m_apiSpecificData2) {
            GLuint renderbuffer = static_cast<GLuint>(reinterpret_cast<uintptr_t>(texId->m_apiSpecificData2));
            glDeleteRenderbuffersEXT(1, &renderbuffer);
        }

        if (s_glFormat[texId->m_format] == GL_DEPTH_COMPONENT) {
            glTexParameteri(s_glTexTarget[texId->m_target], GL_TEXTURE_COMPARE_MODE_ARB, 0);
        }

        glDeleteTextures(1, reinterpret_cast<GLuint*>(&texId->m_apiSpecificData));
        texId->m_apiSpecificData = nullptr;
        texId->m_apiSpecificData2 = nullptr;
        texId->m_needsCreation = 1;
        texId->m_needsFlagUpdate = 1;
        this->TexMarkForUpdate(texId, s_updateRect, 0);
    }
}

// VERIFY: 25c30aab manual
// OFFSET: 0x0069D920
void CGxDeviceOpenGl::ITexDownload(CGxTex* texId, uint32_t width, uint32_t height, uint32_t face, uint32_t mip, uint32_t baseMip, uint32_t stride, const void* texels) {
    // STATIC: 0x00C60680
    static TSGrowableArray<uint8_t> s_buffer;

    uint32_t minY = texId->m_updateRect.minY >> mip;
    uint32_t minX = texId->m_updateRect.minX >> mip;
    uint32_t maxY = minY + 1 > static_cast<uint32_t>(texId->m_updateRect.maxY >> mip) ? minY + 1 : texId->m_updateRect.maxY >> mip;
    uint32_t maxX = minX + 1 > static_cast<uint32_t>(texId->m_updateRect.maxX >> mip) ? minX + 1 : texId->m_updateRect.maxX >> mip;

    GLint internalFormat = s_glInternalFormat[texId->m_format];
    uint32_t updateWidth = maxX - minX;
    uint32_t updateHeight = maxY - minY;
    GLint level = mip - baseMip;

    GLenum target;

    if (texId->m_target == GxTex_CubeMap) {
        target = s_glCubeFaces[face];
    } else {
        target = s_glTexTarget[texId->m_target];
    }

    const uint8_t* bits = static_cast<const uint8_t*>(texels);

    switch (texId->m_format) {
        case GxTex_Argb8888:
        case GxTex_Argb4444:
        case GxTex_Argb1555:
        case GxTex_Rgb565:
        case GxTex_Uv88: {
            EGxTexFormat dataFormat = texId->m_dataFormat;

            if (dataFormat == GxTex_Dxt1 || dataFormat == GxTex_Dxt3 || dataFormat == GxTex_Dxt5) {
                dataFormat = GxTex_Rgb565;

                uint32_t bufferSize = (updateHeight * 16 * updateWidth) >> 3;

                if (s_buffer.Count() < bufferSize) {
                    s_buffer.SetCount(bufferSize);
                }

                const uint8_t* source = bits + ((minX * CGxDevice::s_texFormatBitDepth[texId->m_dataFormat]) >> 3) + stride * minY;
                BlitFormat destFormat = GxGetBlitFormat(GxTex_Rgb565);
                uint8_t* dest = s_buffer.Ptr();

                C2iVector size;
                size.x = updateWidth;
                size.y = updateHeight;

                uint32_t destPitch = CalcRowStride(destFormat, updateWidth, updateHeight);
                Blit(size, BlitAlpha_0, source, stride, GxGetBlitFormat(texId->m_dataFormat), dest, destPitch, destFormat);
                bits = s_buffer.Ptr();
            }

            GLenum format = s_glFormat[dataFormat];
            GLenum type = s_glType[dataFormat];

            if (texId->m_flags.m_bit15) {
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            } else {
                glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / s_glBytesPerPixel[dataFormat]);
            }

            if (texId->m_needsCreation) {
                if (!texId->m_flags.m_bit15 && updateWidth == width && updateHeight == height) {
                    glTexImage2D(target, level, internalFormat, width, height, 0, format, type, bits);
                } else {
                    glTexImage2D(target, level, internalFormat, width, height, 0, format, type, nullptr);
                    glTexSubImage2D(target, level, minX, minY, updateWidth, updateHeight, format, type, bits);
                }
            } else if (texId->m_flags.m_bit14 || (!minX && !minY && maxY == height && maxX == width)) {
                glTexSubImage2D(target, level, 0, 0, width, height, format, type, bits);
            } else {
                glTexSubImage2D(target, level, minX, minY, updateWidth, updateHeight, format, type, bits + minX * s_glBytesPerPixel[dataFormat] + stride * minY);
            }

            break;
        }

        case GxTex_Dxt1:
        case GxTex_Dxt3:
        case GxTex_Dxt5: {
            uint32_t blockX = minX & ~3;
            uint32_t blockY = minY & ~3;
            uint32_t blockWidth = ((maxX + 3) & ~3) - blockX;

            if (blockWidth <= 4) {
                blockWidth = 4;
            }

            uint32_t blockHeight = ((maxY + 3) & ~3) - blockY;

            if (blockHeight <= 4) {
                blockHeight = 4;
            }

            uint32_t bitDepth = CGxDevice::s_texFormatBitDepth[texId->m_format];
            uint32_t imageSize = (blockWidth * blockHeight * bitDepth) >> 3;
            uint32_t offset = 0;

            if (!texId->m_flags.m_bit15) {
                offset = stride * (blockY >> 2) + CGxDevice::s_texFormatBytesPerBlock[texId->m_dataFormat] * (blockX >> 2);
            }

            const uint8_t* source = bits + offset;

            if (blockWidth == 4 * (stride / CGxDevice::s_texFormatBytesPerBlock[texId->m_dataFormat]) && !texId->m_needsCreation) {
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

                if (width < 4 || height < 4 || (blockWidth == width && blockHeight == height)) {
                    glCompressedTexImage2DARB(target, level, internalFormat, width, height, 0, imageSize, source);
                } else {
                    glCompressedTexSubImage2DARB(target, level, blockX, blockY, blockWidth, blockHeight, internalFormat, imageSize, source);
                }
            } else {
                uint32_t fullSize;
                uint32_t bufferSize;

                if (texId->m_needsCreation) {
                    fullSize = (height * width * bitDepth) >> 3;
                    bufferSize = fullSize;
                } else {
                    fullSize = 0;
                    bufferSize = 0;
                }

                if (imageSize > bufferSize) {
                    bufferSize = (blockWidth * blockHeight * bitDepth) >> 3;
                }

                if (s_buffer.Count() < bufferSize) {
                    s_buffer.SetCount(bufferSize);
                }

                uint32_t rowSize = CGxDevice::s_texFormatBytesPerBlock[texId->m_dataFormat] * (blockWidth >> 2);
                uint8_t* buffer = s_buffer.Ptr();
                uint8_t* dest = buffer;

                for (uint32_t rows = blockHeight >> 2; rows; rows--) {
                    memcpy(dest, source, rowSize);
                    source += stride;
                    dest += rowSize;
                }

                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

                if (width < 4 || height < 4 || (blockWidth == width && blockHeight == height)) {
                    glCompressedTexImage2DARB(target, level, internalFormat, width, height, 0, imageSize, buffer);
                } else {
                    if (texId->m_needsCreation) {
                        glCompressedTexImage2DARB(target, level, internalFormat, width, height, 0, fullSize, buffer);
                    }

                    glCompressedTexSubImage2DARB(target, level, blockX, blockY, blockWidth, blockHeight, internalFormat, imageSize, buffer);
                }
            }

            break;
        }

        default:
            break;
    }
}

// VERIFY: 5a0e66a1 manual
// OFFSET: 0x0069DE60
void CGxDeviceOpenGl::ITexMarkAsUpdated(CGxTex* texId, uint32_t tmu) {
    if (!this->m_context) {
        return;
    }

    if (texId->m_needsUpdate && texId->m_userFunc) {
        uint32_t width = texId->m_width;
        uint32_t height = texId->m_height;

        if (!texId->m_apiSpecificData) {
            GLenum target = s_glTexTarget[texId->m_target];
            glGenTextures(1, reinterpret_cast<GLuint*>(&texId->m_apiSpecificData));
            this->BindTexture(texId, tmu);
            this->ITexSetFlags(texId);

            if (glUnkC60498 && (texId->m_format == GxTex_Dxt1 || texId->m_format == GxTex_Dxt3 || texId->m_format == GxTex_Dxt5)) {
                uint32_t size = width >= height ? height : width;
                uint32_t levels = 0;

                while (size > 4) {
                    size >>= 1;
                    levels++;
                }

                glTexParameteri(target, GL_TEXTURE_MAX_LOD, levels);
            }
        } else {
            this->BindTexture(texId, tmu);
        }

        uint32_t baseMip;
        uint32_t mipCount;

        if ((texId->m_flags.m_filter >= GxTex_NearestMipNearest && !texId->m_flags.m_generateMipMaps) || texId->m_flags.m_forceMipTracking) {
            uint32_t size = texId->m_height;

            if (texId->m_width > size) {
                size = texId->m_width;
            }

            mipCount = 1;

            while (size != 1) {
                size >>= 1;
                mipCount++;
            }

            baseMip = mipCount - 1;

            if (this->m_baseMipLevel < baseMip) {
                baseMip = this->m_baseMipLevel;
            }

            width >>= baseMip;
            height >>= baseMip;

            if (texId->m_flags.m_forceMipTracking) {
                mipCount = baseMip + 1;
            }
        } else {
            baseMip = 0;
            mipCount = 1;
        }

        glTexParameteri(s_glTexTarget[texId->m_target], GL_TEXTURE_MAX_LEVEL, mipCount - baseMip - 1);

        uint32_t texelStrideInBytes;
        const void* texels;
        texId->m_userFunc(GxTex_Lock, texId->m_width, texId->m_height, 0, 0, texId->m_userArg, texelStrideInBytes, texels);

        if (this->m_boundTexture != static_cast<GLuint>(reinterpret_cast<uintptr_t>(texId->m_apiSpecificData))) {
            this->BindTexture(texId, tmu);
        }

        uint32_t faceCount = texId->m_target == GxTex_CubeMap ? GxTexCubeFaces_Last : 1;

        for (uint32_t face = 0; face < faceCount; face++) {
            uint32_t mipWidth = width;
            uint32_t mipHeight = height;
            uint32_t levelCount = 0;

            for (uint32_t mip = baseMip; mip != mipCount; mip++, levelCount++) {
                texId->m_userFunc(GxTex_Latch, mipWidth, mipHeight, face, mip, texId->m_userArg, texelStrideInBytes, texels);

                if (!texId->m_flags.m_renderTarget) {
                    this->ITexDownload(texId, mipWidth, mipHeight, face, mip, baseMip, texelStrideInBytes, texels);
                }

                if (!levelCount && texId->m_flags.m_generateMipMaps) {
                    break;
                }

                mipWidth >>= 1;
                mipHeight >>= 1;

                if (!mipWidth) {
                    mipWidth = 1;
                }

                if (!mipHeight) {
                    mipHeight = 1;
                }
            }
        }

        texId->m_userFunc(GxTex_Unlock, texId->m_width, texId->m_height, 0, 0, texId->m_userArg, texelStrideInBytes, texels);

        if (!texId->m_flags.m_renderTarget) {
            texId->m_needsCreation = 0;
        }
    }

    if (texId->m_needsFlagUpdate) {
        this->ITexSetFlags(texId);
    }

    CGxDevice::ITexMarkAsUpdated(texId);
}

// VERIFY: 8215407b manual
// OFFSET: 0x0069E120
void CGxDeviceOpenGl::ITexMarkAsUpdated(CGxTex* texId) {
    this->ITexMarkAsUpdated(texId, kNullTmu);
}

// VERIFY: 468f21f9 auto
// OFFSET: 0x0069E660
void NvCombinerStage::Apply(GLenum stage, GLenum portion) {
    for (uint32_t i = 0; i <= 3; i++) {
        glCombinerInputNV(stage, portion, GL_VARIABLE_A_NV + i, this->m_inputs[i].m_input, this->m_inputs[i].m_mapping, this->m_inputs[i].m_componentUsage);
    }

    glCombinerOutputNV(stage, portion, this->m_abOutput, this->m_cdOutput, this->m_sumOutput, this->m_scale, this->m_bias, this->m_abDotProduct, this->m_cdDotProduct, this->m_muxSum);
}

// VERIFY: fcd12e49 auto
// OFFSET: 0x0069E6E0
void NvPixelShader::SetupRegisterCombiners() {
    glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, this->m_numGeneralCombiners);
    glCombinerParameteriNV(GL_COLOR_SUM_CLAMP_NV, this->m_colorSumClamp);
    glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, this->m_constants[0]);
    glCombinerParameterfvNV(GL_CONSTANT_COLOR1_NV, this->m_constants[1]);

    for (GLenum stage = GL_COMBINER0_NV; static_cast<GLint>(stage - GL_COMBINER0_NV) < this->m_numGeneralCombiners; stage++) {
        NvGeneralCombiner& combiner = this->m_generalCombiners[stage - GL_COMBINER0_NV];
        GLboolean perStageConstants = this->m_perStageConstants;
        combiner.m_rgb.Apply(stage, GL_RGB);
        combiner.m_alpha.Apply(stage, GL_ALPHA);

        if (perStageConstants) {
            glCombinerStageParameterfvNV(stage, GL_CONSTANT_COLOR0_NV, combiner.m_constants[0]);
            glCombinerStageParameterfvNV(stage, GL_CONSTANT_COLOR1_NV, combiner.m_constants[1]);
        }
    }

    for (uint32_t i = 0; i <= 6; i++) {
        glFinalCombinerInputNV(GL_VARIABLE_A_NV + i, this->m_finalInputs[i].m_input, this->m_finalInputs[i].m_mapping, this->m_finalInputs[i].m_componentUsage);
    }
}

// VERIFY: 5384e74c auto
// OFFSET: 0x0069E7D0
void NvTextureShaderStage::Apply(uint32_t tmu) {
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, this->m_shaderOperation);

    GLint cullModes[4];
    cullModes[0] = this->m_cullModes[0] ? GL_LESS : GL_GEQUAL;
    cullModes[1] = this->m_cullModes[1] ? GL_LESS : GL_GEQUAL;
    cullModes[2] = this->m_cullModes[2] ? GL_LESS : GL_GEQUAL;
    cullModes[3] = this->m_cullModes[3] ? GL_LESS : GL_GEQUAL;
    glTexEnviv(GL_TEXTURE_SHADER_NV, GL_CULL_MODES_NV, cullModes);

    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_RGBA_UNSIGNED_DOT_PRODUCT_MAPPING_NV, this->m_dotProductMapping);

    if (this->m_previousInput - GL_TEXTURE0_ARB < tmu) {
        glTexEnvi(GL_TEXTURE_SHADER_NV, GL_PREVIOUS_TEXTURE_INPUT_NV, this->m_previousInput);
    }
}

// VERIFY: 53324e6b auto
// OFFSET: 0x0069E890
void NvTextureShader::Apply() {
    for (uint32_t tmu = 0; tmu < 4; tmu++) {
        static_cast<CGxDeviceOpenGl*>(g_theGxDevicePtr)->DsSet(CGxDeviceOpenGl::Ds_ActiveTexture, tmu, 0);
        this->m_stages[tmu].Apply(tmu);
    }
}

// VERIFY: 87f0be32 auto
// OFFSET: 0x0069E8C0
void CGxDeviceOpenGl::IShaderDeleteVertex(CGxShader*& shader) {
    if (shader->m_refCount != 1 || !shader->m_apiSpecific) {
        return;
    }

    switch (this->m_caps.m_shaderTargets[GxSh_Vertex]) {
        case 6:
        case 7:
            glDeleteProgramsARB(1, reinterpret_cast<GLuint*>(&shader->m_apiSpecific));
            break;

        case 8:
        case 9:
            glDeleteProgramsNV(1, reinterpret_cast<GLuint*>(&shader->m_apiSpecific));
            break;

        default:
            break;
    }

    shader->m_apiSpecific = nullptr;
    shader->m_loaded = 0;
}

// VERIFY: 69da2014 auto
// OFFSET: 0x0069E930
void CGxDeviceOpenGl::IShaderDeletePixel(CGxShader*& shader) {
    if (shader->m_refCount != 1 || !shader->m_apiSpecific) {
        return;
    }

    if (this->m_caps.m_shaderTargets[GxSh_Pixel] == 12) {
        glDeleteProgramsARB(1, reinterpret_cast<GLuint*>(&shader->m_apiSpecific));
    }

    shader->m_apiSpecific = nullptr;
    shader->m_loaded = 0;
}

// VERIFY: 91d02dcb manual
// OFFSET: 0x0069E970
void CGxDeviceOpenGl::ShaderConstantsSet(EGxShTarget target, uint32_t index, const float* constants, uint32_t count) {
    if (target) {
        ShadowConstants& shadow = CGxDevice::s_shadowConstants[0];

        if (shadow.dirtyMin > index) {
            shadow.dirtyMin = index;
        }

        if (shadow.dirtyMax < index + count - 1) {
            shadow.dirtyMax = index + count - 1;
        }

        float* dest = &shadow.constants[index * 4];

        for (uint32_t i = 0; i < count; i++) {
            dest[0] = constants[0];
            dest[1] = constants[1];
            dest[2] = constants[2];
            dest[3] = constants[3];
            dest += 4;
            constants += 4;
        }

        return;
    }

    ShadowConstants& shadow = CGxDevice::s_shadowConstants[1];
    float* dest = &shadow.constants[index * 4];

    for (uint32_t i = index; i < index + count; i++) {
        int32_t changed = 0;

        if (constants[0] != dest[0]) {
            changed = 1;
            dest[0] = constants[0];
        }

        if (constants[1] != dest[1]) {
            changed = 1;
            dest[1] = constants[1];
        }

        if (constants[2] != dest[2]) {
            changed = 1;
            dest[2] = constants[2];
        }

        if (constants[3] != dest[3]) {
            dest[3] = constants[3];
            changed = 1;
        }

        if (changed) {
            if (shadow.dirtyMin > i) {
                shadow.dirtyMin = i;
            }

            if (shadow.dirtyMax < i) {
                shadow.dirtyMax = i;
            }
        }

        dest += 4;
        constants += 4;
    }
}

// VERIFY: de631ae6 auto
// OFFSET: 0x0069EB10
void CGxDeviceOpenGl::IShaderCreatePixel(CGxShader* shader) {
    uint32_t codeSize = shader->m_code.Count();
    shader->m_loaded = 1;
    shader->m_valid = 0;

    if (!codeSize) {
        return;
    }

    int32_t target = this->m_caps.m_shaderTargets[GxSh_Pixel];

    if (target < 7) {
        return;
    }

    if (target <= 10) {
        shader->m_apiSpecific = reinterpret_cast<void*>(1);
        shader->m_valid = 1;
    } else if (target == 12) {
        GLint current;
        glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_BINDING_ARB, &current);

        if (!shader->m_apiSpecific) {
            glGenProgramsARB(1, reinterpret_cast<GLuint*>(&shader->m_apiSpecific));
        }

        glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, static_cast<GLuint>(reinterpret_cast<uintptr_t>(shader->m_apiSpecific)));
        glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, shader->m_code.Count() - 1, shader->m_code.Ptr());

        GLint errorPosition;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPosition);

        GLint underNativeLimits;
        glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &underNativeLimits);

        shader->m_valid = errorPosition == -1 && underNativeLimits;
        glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, current);
    }
}

// VERIFY: ec0a4edc auto
// OFFSET: 0x0069EC10
void CGxDeviceOpenGl::IShaderCreateVertex(CGxShader* shader) {
    uint32_t codeSize = shader->m_code.Count();
    shader->m_loaded = 1;
    shader->m_valid = 0;

    if (!codeSize) {
        return;
    }

    switch (this->m_caps.m_shaderTargets[GxSh_Vertex]) {
        case 6:
        case 7: {
            GLint current;
            glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_BINDING_ARB, &current);

            if (!shader->m_apiSpecific) {
                glGenProgramsARB(1, reinterpret_cast<GLuint*>(&shader->m_apiSpecific));
            }

            glBindProgramARB(GL_VERTEX_PROGRAM_ARB, static_cast<GLuint>(reinterpret_cast<uintptr_t>(shader->m_apiSpecific)));
            glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, shader->m_code.Count() - 1, shader->m_code.Ptr());

            GLint errorPosition;
            glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPosition);

            GLint underNativeLimits;
            glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &underNativeLimits);

            shader->m_valid = errorPosition == -1 && underNativeLimits;
            glBindProgramARB(GL_VERTEX_PROGRAM_ARB, current);
            break;
        }

        case 8:
        case 9: {
            glGenProgramsNV(1, reinterpret_cast<GLuint*>(&shader->m_apiSpecific));
            glLoadProgramNV(GL_VERTEX_PROGRAM_ARB, static_cast<GLuint>(reinterpret_cast<uintptr_t>(shader->m_apiSpecific)), shader->m_code.Count() - 1, shader->m_code.Ptr());

            GLint errorPosition;
            glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPosition);
            shader->m_valid = errorPosition == -1;
            break;
        }

        default:
            break;
    }
}

// VERIFY: da2d444c manual
// OFFSET: 0x0069ED50
void CGxDeviceOpenGl::IVertexShaderPatch(CGxShader* shader) {
    if (shader->m_patched) {
        return;
    }

    shader->m_patched = 1;

    uint32_t numConstants = this->m_caps.m_shaderConstants[GxSh_Vertex];
    uint32_t size;

    if (numConstants < 223) {
        size = numConstants < 192 ? 94 : 192;
    } else {
        size = 223;
    }

    char* code = reinterpret_cast<char*>(shader->m_code.Ptr());
    char* end = code + shader->m_code.Count();

    switch (this->m_caps.m_shaderTargets[GxSh_Vertex]) {
        case 6: {
            uint32_t paramSize = size;

            for (char* c = code; c < end; c++) {
                if (!SStrCmp(c, ".local", 6)) {
                    int32_t bracket = c[6] == '[';
                    memcpy(c, ".env  ", 6);

                    int32_t first;
                    int32_t last;
                    int32_t consumed;

                    if (bracket && sscanf(c + 4, " [%d..%d]%n", &first, &last, &consumed) == 2 && last == 93) {
                        char buffer[64];
                        SStrPrintf(buffer, sizeof(buffer), "[%d..%d]          ", first, paramSize - 1);
                        memcpy(c + 4, buffer, consumed);
                    }
                }

                int32_t index;
                int32_t consumed;

                if (sscanf(c, "PARAM c[%d] = { %n", &index, &consumed) == 1 && index == 94) {
                    char buffer[64];
                    SStrPrintf(buffer, sizeof(buffer), "PARAM c[%d]={              ", paramSize);
                    memcpy(c, buffer, consumed);
                }
            }

            break;
        }

        case 7: {
            for (char* c = code; c < end; c++) {
                char* start = c;

                if (c + 6 >= end || memcmp(c, "PARAM c", 7)) {
                    continue;
                }

                c += 7;

                uint32_t reg = 0;
                uint32_t last = 0;

                if (c < end) {
                    while (isdigit(*c)) {
                        reg = *c++ + 10 * reg - '0';

                        if (c >= end) {
                            break;
                        }
                    }

                    if (c < end && *c == '[') {
                        c++;

                        uint32_t count = 0;

                        while (c < end && isdigit(*c)) {
                            count = *c++ + 10 * count - '0';
                        }

                        last = count + reg - 1;

                        if (c >= end || *c != ']') {
                            continue;
                        }

                        c++;
                    }
                }

                char localBlock[] = " = { program.local";
                char envBlock[] = " program.env  ";
                char local[] = " = program.local";
                char env[] = "rogram.env  ";

                if (c + 17 >= end) {
                    continue;
                }

                if (!memcmp(c, localBlock, 18)) {
                    memcpy(c, " = {", 4);
                    memcpy(c + 4, envBlock, 14);
                } else if (!memcmp(c, local, 16)) {
                    memcpy(c, " = p", 4);
                    memcpy(c + 4, env, 12);
                } else {
                    continue;
                }

                if (reg == 31 && last == 93) {
                    int32_t written = sprintf(start, "PARAM c%u[%u]={program.env[%u..%u]}", 31, size - 31, 31, size - 1);
                    char* tail = start + written;
                    *tail = ';';
                    tail++;

                    if (*tail != '\n') {
                        do {
                            *tail++ = ' ';
                        } while (*tail != '\n');
                    }
                }
            }

            break;
        }

        default:
            break;
    }
}

// VERIFY: 61ad2d9a auto
// OFFSET: 0x0069F270
void CGxDeviceOpenGl::IPixelShaderPatch(CGxShader* shader) {
    if (shader->m_patched) {
        return;
    }

    shader->m_patched = 1;

    if (this->m_caps.m_shaderTargets[GxSh_Pixel] != 12) {
        return;
    }

    char* code = reinterpret_cast<char*>(shader->m_code.Ptr());
    char* end = code + shader->m_code.Count();

    for (char* c = code; c < end; c++) {
        if (!SStrCmp(c, ".local", 6)) {
            memcpy(c, ".env  ", 6);
        }
    }
}

// VERIFY: 352a6b10 manual
// OFFSET: 0x0069F2D0
void CGxDeviceOpenGl::ShaderCreate(CGxShader** shaders, EGxShTarget target, const char* path, const char* name, uint32_t permutations) {
    CGxDevice::ShaderCreate(shaders, target, path, name, permutations);

    if (target == GxSh_Vertex) {
        for (uint32_t i = 0; i < permutations; i++) {
            this->IVertexShaderPatch(shaders[i]);
        }
    } else if (target == GxSh_Pixel) {
        for (uint32_t i = 0; i < permutations; i++) {
            this->IPixelShaderPatch(shaders[i]);
        }
    }

    if (permutations == 1 && !shaders[0]->m_loaded) {
        if (target == GxSh_Vertex) {
            this->IShaderCreateVertex(shaders[0]);
        } else if (target == GxSh_Pixel) {
            this->IShaderCreatePixel(shaders[0]);
        }
    }
}

// VERIFY: d3641275 auto
// OFFSET: 0x0069F370
void CGxDeviceOpenGl::ShaderDestroy(CGxShader*& shader) {
    switch (shader->m_target) {
        case GxSh_Vertex:
            this->IShaderDeleteVertex(shader);
            break;

        case GxSh_Pixel:
            this->IShaderDeletePixel(shader);
            break;

        default:
            break;
    }

    CGxDevice::ShaderDestroy(shader);
}

// VERIFY: 3ea93d9b manual
// OFFSET: 0x0069F3B0
void CGxDeviceOpenGl::IShaderConstantsSend(EGxShTarget target, uint32_t index, const float* constants, uint32_t count) {
    if (!this->m_caps.m_shaderTargets[target]) {
        return;
    }

    this->IRsSync(0);

    if (target == GxSh_Vertex) {
        switch (this->m_caps.m_shaderTargets[GxSh_Vertex]) {
            case 6:
            case 7:
                if (glExtGPUProgramParameters) {
                    glProgramEnvParameters4fvEXT(GL_VERTEX_PROGRAM_ARB, index, count, constants);
                } else {
                    for (uint32_t i = 0; i < count; i++) {
                        glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, i + index, constants);
                        constants += 4;
                    }
                }

                break;

            case 8:
            case 9:
                glProgramParameters4fvNV(GL_VERTEX_PROGRAM_ARB, index, count, constants);
                break;

            default:
                break;
        }

        return;
    }

    if (target != GxSh_Pixel) {
        return;
    }

    switch (this->m_caps.m_shaderTargets[GxSh_Pixel]) {
        case 7:
            for (uint32_t i = 0; i < count; i++) {
                glCombinerParameterfvNV(index + i + GL_CONSTANT_COLOR0_NV, constants);
                constants += 4;
            }

            break;

        case 8:
        case 9:
        case 10: {
            CGxShader* shader = static_cast<CGxShader*>(this->m_appRenderStates[GxRs_PixelShader].m_value.m_data.p);

            if (!shader) {
                break;
            }

            NvPixelShader* program = reinterpret_cast<NvPixelShader*>(shader->m_code.Ptr());

            if (!program) {
                break;
            }

            for (uint32_t i = 0; i < count; i++) {
                for (uint32_t j = 0; j < program->m_constantMapCount; j++) {
                    if (program->m_constantMap[j] >> 5 == index + i) {
                        uint32_t slot = program->m_constantMap[j] & 0x1F;

                        if (slot < 2) {
                            glCombinerParameterfvNV(slot + GL_CONSTANT_COLOR0_NV, constants);
                        } else {
                            glCombinerStageParameterfvNV((slot >> 1) - 1 + GL_COMBINER0_NV, (slot & 1) + GL_CONSTANT_COLOR0_NV, constants);
                        }
                    }
                }

                constants += 4;
            }

            break;
        }

        case 12:
            if (glExtGPUProgramParameters) {
                glProgramEnvParameters4fvEXT(GL_FRAGMENT_PROGRAM_ARB, index, count, constants);
            } else {
                for (uint32_t i = 0; i < count; i++) {
                    glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, i + index, constants);
                    constants += 4;
                }
            }

            break;

        default:
            break;
    }
}

// VERIFY: 2e3a6063 auto
// OFFSET: 0x0069F610
void CGxDeviceOpenGl::IShaderConstantsFlush() {
    ShadowConstants& vertex = CGxDevice::s_shadowConstants[1];

    if (vertex.dirtyMin <= vertex.dirtyMax) {
        this->IShaderConstantsSend(GxSh_Vertex, vertex.dirtyMin, &vertex.constants[vertex.dirtyMin * 4], vertex.dirtyMax - vertex.dirtyMin + 1);
    }

    vertex.dirtyMin = 255;
    vertex.dirtyMax = 0;

    ShadowConstants& pixel = CGxDevice::s_shadowConstants[0];

    if (pixel.dirtyMin <= pixel.dirtyMax) {
        this->IShaderConstantsSend(GxSh_Pixel, pixel.dirtyMin, &pixel.constants[pixel.dirtyMin * 4], pixel.dirtyMax - pixel.dirtyMin + 1);
    }

    pixel.dirtyMax = 0;
    pixel.dirtyMin = 255;
}

// VERIFY: 92b58809 auto
// OFFSET: 0x0069F6A0
void CGxDeviceOpenGl::IShaderReload(CGxShader* shader, const char* path, const char* name) {
    switch (shader->m_target) {
        case GxSh_Vertex:
            this->IShaderDeleteVertex(shader);
            this->IShaderLoad(&shader, GxSh_Vertex, path, name, 1);
            this->IShaderCreateVertex(shader);
            break;

        case GxSh_Pixel:
            this->IShaderDeletePixel(shader);
            this->IShaderLoad(&shader, GxSh_Pixel, path, name, 1);
            this->IShaderCreatePixel(shader);
            break;

        default:
            break;
    }
}

// VERIFY: 120ef147 manual
// OFFSET: 0x0069F730
void CGxDeviceOpenGl::IShaderCreate(CGxShader* shader) {
    switch (shader->m_target) {
        case GxSh_Vertex:
            this->IShaderCreateVertex(shader);
            break;

        case GxSh_Pixel:
            this->IShaderCreatePixel(shader);
            break;

        default:
            break;
    }
}

// VERIFY: 0e4b000b manual
// OFFSET: 0x0069F760
void CGxDeviceOpenGl::IBindPixelShader(CGxShader* shader) {
    if (shader) {
        if (!shader->m_loaded) {
            this->IShaderCreatePixel(shader);
        }

        switch (this->m_caps.m_shaderTargets[GxSh_Pixel]) {
            case 7: {
                NvPixelShader* program = reinterpret_cast<NvPixelShader*>(shader->m_code.Ptr());

                if (program) {
                    program->SetupRegisterCombiners();
                    this->DsSet(Ds_PerStageConstants, program->m_perStageConstants, 0);
                    this->DsSet(Ds_RegisterCombiners, 1, 0);
                }

                break;
            }

            case 8:
            case 9:
            case 10: {
                NvPixelShader* program = reinterpret_cast<NvPixelShader*>(shader->m_code.Ptr());

                if (program) {
                    program->SetupRegisterCombiners();
                    program->m_textureShader.Apply();
                    this->DsSet(Ds_PerStageConstants, program->m_perStageConstants, 0);
                    this->DsSet(Ds_RegisterCombiners, 1, 0);
                    this->DsSet(Ds_TextureShader, 1, 0);
                }

                break;
            }

            case 12:
                glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, static_cast<GLuint>(reinterpret_cast<uintptr_t>(shader->m_apiSpecific)));
                this->DsSet(Ds_FragmentProgram, 1, 0);
                break;

            default:
                break;
        }
    } else {
        switch (this->m_caps.m_shaderTargets[GxSh_Pixel]) {
            case 7:
                this->DsSet(Ds_RegisterCombiners, 0, 0);
                break;

            case 8:
            case 9:
            case 10:
                this->DsSet(Ds_RegisterCombiners, 0, 0);
                this->DsSet(Ds_TextureShader, 0, 0);
                break;

            case 12:
                this->DsSet(Ds_FragmentProgram, 0, 0);
                break;

            default:
                break;
        }
    }
}

// VERIFY: a4e6652f manual
// OFFSET: 0x0069F8D0
void CGxDeviceOpenGl::IBindVertexShader(CGxShader* shader) {
    if (shader) {
        if (!shader->m_loaded) {
            this->IShaderCreateVertex(shader);
        }

        switch (this->m_caps.m_shaderTargets[GxSh_Vertex]) {
            case 6:
            case 7:
                glBindProgramARB(GL_VERTEX_PROGRAM_ARB, static_cast<GLuint>(reinterpret_cast<uintptr_t>(shader->m_apiSpecific)));
                this->DsSet(Ds_VertexProgram, 1, 0);
                break;

            case 8:
            case 9:
                glBindProgramNV(GL_VERTEX_PROGRAM_ARB, static_cast<GLuint>(reinterpret_cast<uintptr_t>(shader->m_apiSpecific)));
                this->DsSet(Ds_VertexProgramNV, 1, 0);
                break;

            default:
                break;
        }
    } else {
        switch (this->m_caps.m_shaderTargets[GxSh_Vertex]) {
            case 6:
            case 7:
                this->DsSet(Ds_VertexProgram, 0, 0);
                break;

            case 8:
            case 9:
                this->DsSet(Ds_VertexProgramNV, 0, 0);
                break;

            default:
                break;
        }
    }
}

// VERIFY: 0e6bf6e0 manual
// OFFSET: 0x0069F9B0
void CGxDeviceOpenGl::IShaderForceRecreation() {
    TSExplicitList<CGxShader>& pixelShaders = this->m_shaderList[GxSh_Pixel].m_fulllist;

    for (CGxShader* shader = pixelShaders.Head(); !pixelShaders.IsEnd(shader); shader = pixelShaders.RawNext(shader)) {
        if (shader->m_apiSpecific && this->m_caps.m_shaderTargets[GxSh_Pixel] == 12) {
            glDeleteProgramsARB(1, reinterpret_cast<GLuint*>(&shader->m_apiSpecific));
        }

        shader->m_apiSpecific = nullptr;
        shader->m_loaded = 0;
    }

    TSExplicitList<CGxShader>& vertexShaders = this->m_shaderList[GxSh_Vertex].m_fulllist;

    for (CGxShader* shader = vertexShaders.Head(); !vertexShaders.IsEnd(shader); shader = vertexShaders.RawNext(shader)) {
        if (shader->m_apiSpecific) {
            switch (this->m_caps.m_shaderTargets[GxSh_Vertex]) {
                case 6:
                case 7:
                    glDeleteProgramsARB(1, reinterpret_cast<GLuint*>(&shader->m_apiSpecific));
                    break;

                case 8:
                case 9:
                    glDeleteProgramsNV(1, reinterpret_cast<GLuint*>(&shader->m_apiSpecific));
                    break;

                default:
                    break;
            }
        }

        shader->m_apiSpecific = nullptr;
        shader->m_loaded = 0;
    }
}

void CGxDeviceOpenGl::StereoSetConvergence(float convergence) {
}

float CGxDeviceOpenGl::StereoGetConvergence() {
    return 1.0f;
}

void CGxDeviceOpenGl::StereoSetSeparation(float separation) {
}

float CGxDeviceOpenGl::StereoGetSeparation() {
    return 0.0f;
}

bool CGxDeviceOpenGl::StereoEnabled() {
    return false;
}

int32_t CGxDeviceOpenGl::TexStretch(CGxTex* dest, const CiRect* destRect, const CiRect* sourceRect, uint32_t level, uint32_t face) {
    return 0;
}

void CGxDeviceOpenGl::DeviceUnk81(uint32_t a, uint32_t b) {
}

void CGxDeviceOpenGl::DeviceUnk82() {
}

void CGxDeviceOpenGl::DeviceUnk83(uint32_t a, uint32_t b) {
}
