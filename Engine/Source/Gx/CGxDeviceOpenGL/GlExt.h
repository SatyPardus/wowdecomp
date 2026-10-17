#ifndef ENGINE_SOURCE_GX_CGXDEVICEOPENGL_GLEXT_H
#define ENGINE_SOURCE_GX_CGXDEVICEOPENGL_GLEXT_H

#include <cstddef>
#include <cstdint>
#if defined(_WIN32)
#include <windows.h>
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#define glProgramLocalParameters4fvEXT gx_glProgramLocalParameters4fvEXT
#define glProgramEnvParameters4fvEXT gx_glProgramEnvParameters4fvEXT
#define glBlendColorEXT gx_glBlendColorEXT
#define glFramebufferRenderbufferEXT gx_glFramebufferRenderbufferEXT
#define glFramebufferTexture3DEXT gx_glFramebufferTexture3DEXT
#define glFramebufferTexture2DEXT gx_glFramebufferTexture2DEXT
#define glFramebufferTexture1DEXT gx_glFramebufferTexture1DEXT
#define glCheckFramebufferStatusEXT gx_glCheckFramebufferStatusEXT
#define glGenFramebuffersEXT gx_glGenFramebuffersEXT
#define glDeleteFramebuffersEXT gx_glDeleteFramebuffersEXT
#define glBindFramebufferEXT gx_glBindFramebufferEXT
#define glRenderbufferStorageEXT gx_glRenderbufferStorageEXT
#define glGenRenderbuffersEXT gx_glGenRenderbuffersEXT
#define glDeleteRenderbuffersEXT gx_glDeleteRenderbuffersEXT
#define glBindRenderbufferEXT gx_glBindRenderbufferEXT
#define glPointParameterfvARB gx_glPointParameterfvARB
#define glPointParameterfARB gx_glPointParameterfARB
#define glGetQueryObjectuivARB gx_glGetQueryObjectuivARB
#define glGetQueryObjectivARB gx_glGetQueryObjectivARB
#define glGetQueryivARB gx_glGetQueryivARB
#define glEndQueryARB gx_glEndQueryARB
#define glBeginQueryARB gx_glBeginQueryARB
#define glDeleteQueriesARB gx_glDeleteQueriesARB
#define glGenQueriesARB gx_glGenQueriesARB
#define glUnmapBufferARB gx_glUnmapBufferARB
#define glMapBufferARB gx_glMapBufferARB
#define glBufferSubDataARB gx_glBufferSubDataARB
#define glBufferDataARB gx_glBufferDataARB
#define glGenBuffersARB gx_glGenBuffersARB
#define glDeleteBuffersARB gx_glDeleteBuffersARB
#define glBindBufferARB gx_glBindBufferARB
#define glGenProgramsNV gx_glGenProgramsNV
#define glDeleteProgramsNV gx_glDeleteProgramsNV
#define glProgramParameters4fvNV gx_glProgramParameters4fvNV
#define glBindProgramNV gx_glBindProgramNV
#define glLoadProgramNV gx_glLoadProgramNV
#define glDisableVertexAttribArrayARB gx_glDisableVertexAttribArrayARB
#define glEnableVertexAttribArrayARB gx_glEnableVertexAttribArrayARB
#define glVertexAttribPointerARB gx_glVertexAttribPointerARB
#define glGetProgramivARB gx_glGetProgramivARB
#define glIsProgramARB gx_glIsProgramARB
#define glProgramLocalParameter4fvARB gx_glProgramLocalParameter4fvARB
#define glProgramEnvParameter4fvARB gx_glProgramEnvParameter4fvARB
#define glGenProgramsARB gx_glGenProgramsARB
#define glDeleteProgramsARB gx_glDeleteProgramsARB
#define glBindProgramARB gx_glBindProgramARB
#define glProgramStringARB gx_glProgramStringARB
#define glCombinerParameteriNV gx_glCombinerParameteriNV
#define glCombinerParameterfvNV gx_glCombinerParameterfvNV
#define glFinalCombinerInputNV gx_glFinalCombinerInputNV
#define glCombinerStageParameterfvNV gx_glCombinerStageParameterfvNV
#define glCombinerOutputNV gx_glCombinerOutputNV
#define glCombinerInputNV gx_glCombinerInputNV
#define glDrawRangeElementsEXT gx_glDrawRangeElementsEXT
#define glCompressedTexSubImage2DARB gx_glCompressedTexSubImage2DARB
#define glCompressedTexImage2DARB gx_glCompressedTexImage2DARB
#define glClientActiveTextureARB gx_glClientActiveTextureARB
#define glActiveTextureARB gx_glActiveTextureARB
#define glUnlockArraysEXT gx_glUnlockArraysEXT
#define glLockArraysEXT gx_glLockArraysEXT

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef WINAPI
#define WINAPI
#endif

typedef int BOOL;
typedef unsigned int UINT;
typedef float FLOAT;
typedef void* HDC;
typedef void* HPBUFFERARB;
#endif

#define GL_TEXTURE0_ARB 0x84C0
#define GL_MAX_TEXTURE_UNITS_ARB 0x84E2
#define GL_MAX_TEXTURE_LOD_BIAS_EXT 0x84FD
#define GL_MAX_ELEMENTS_VERTICES_EXT 0x80E8
#define GL_MAX_ELEMENTS_INDICES_EXT 0x80E9
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB 0x8872
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB 0x851C
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB 0x84F8
#define GL_MAX_VERTEX_ATTRIBS_ARB 0x8869
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_POINT_SIZE_MAX_ARB 0x8127
#define GL_TEXTURE_CUBE_MAP_ARB 0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x8515
#define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE_MAX_LOD 0x813B
#define GL_TEXTURE_MAX_LEVEL 0x813D
#define GL_GENERATE_MIPMAP_SGIS 0x8191
#define GL_TEXTURE_COMPARE_MODE_ARB 0x884C
#define GL_TEXTURE_COMPARE_FUNC_ARB 0x884D
#define GL_DEPTH_TEXTURE_MODE_ARB 0x884B
#define GL_COMPARE_R_TO_TEXTURE_ARB 0x884E
#define GL_REFLECTION_MAP_ARB 0x8512
#define GL_NORMAL_MAP_ARB 0x8511
#define GL_COMBINE_ARB 0x8570
#define GL_COMBINE_RGB_ARB 0x8571
#define GL_COMBINE_ALPHA_ARB 0x8572
#define GL_RGB_SCALE_ARB 0x8573
#define GL_SOURCE0_RGB_ARB 0x8580
#define GL_SOURCE1_RGB_ARB 0x8581
#define GL_SOURCE2_RGB_ARB 0x8582
#define GL_SOURCE0_ALPHA_ARB 0x8588
#define GL_SOURCE1_ALPHA_ARB 0x8589
#define GL_SOURCE2_ALPHA_ARB 0x858A
#define GL_ARRAY_BUFFER_ARB 0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB 0x8893
#define GL_STREAM_DRAW_ARB 0x88E0
#define GL_STATIC_DRAW_ARB 0x88E4
#define GL_DYNAMIC_DRAW_ARB 0x88E8
#define GL_WRITE_ONLY_ARB 0x88B9
#define GL_SAMPLES_PASSED_ARB 0x8914
#define GL_QUERY_COUNTER_BITS_ARB 0x8864
#define GL_QUERY_RESULT_ARB 0x8866
#define GL_QUERY_RESULT_AVAILABLE_ARB 0x8867
#define GL_FRAMEBUFFER_EXT 0x8D40
#define GL_RENDERBUFFER_EXT 0x8D41
#define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#define GL_DEPTH_ATTACHMENT_EXT 0x8D00
#define GL_DEPTH_COMPONENT24_ARB 0x81A6
#define GL_REGISTER_COMBINERS_NV 0x8522
#define GL_PER_STAGE_CONSTANTS_NV 0x8535
#define GL_TEXTURE_SHADER_NV 0x86DE
#define GL_FRAGMENT_PROGRAM_ARB 0x8804
#define GL_VERTEX_PROGRAM_ARB 0x8620
#define GL_PROGRAM_FORMAT_ASCII_ARB 0x8875
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB 0x88B5
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB 0x88B4
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AB
#define GL_PROGRAM_ERROR_POSITION_ARB 0x864B
#define GL_MULTISAMPLE_FILTER_HINT_NV 0x8534
#define GL_PROGRAM_BINDING_ARB 0x8677
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB 0x88B6
#define GL_VARIABLE_A_NV 0x8523
#define GL_CONSTANT_COLOR0_NV 0x852A
#define GL_CONSTANT_COLOR1_NV 0x852B
#define GL_NUM_GENERAL_COMBINERS_NV 0x854E
#define GL_COLOR_SUM_CLAMP_NV 0x854F
#define GL_COMBINER0_NV 0x8550
#define GL_SHADER_OPERATION_NV 0x86DF
#define GL_CULL_MODES_NV 0x86E0
#define GL_RGBA_UNSIGNED_DOT_PRODUCT_MAPPING_NV 0x86D9
#define GL_PREVIOUS_TEXTURE_INPUT_NV 0x86E4
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_MULTISAMPLE_ARB 0x809D
#define GL_POINT_SPRITE_ARB 0x8861
#define GL_COORD_REPLACE_ARB 0x8862
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB 0x8642
#define GL_POINT_SIZE_MIN_ARB 0x8126
#define GL_POINT_DISTANCE_ATTENUATION_ARB 0x8129
#define GL_LIGHT_MODEL_COLOR_CONTROL 0x81F8
#define GL_SEPARATE_SPECULAR_COLOR 0x81FA
#define GL_CONSTANT_COLOR_EXT 0x8001
#define GL_CONSTANT_ALPHA_EXT 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA_EXT 0x8004

#define WGL_NUMBER_PIXEL_FORMATS_ARB 0x2000
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_RED_BITS_ARB 0x2015
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_PBUFFER_LOST_ARB 0x2036
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042

#if defined(_WIN32)
DECLARE_HANDLE(HPBUFFERARB);
#endif

typedef char GLcharARB;
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;

typedef void (APIENTRY* PFNGLLOCKARRAYSEXTPROC)(GLint first, GLsizei count);
typedef void (APIENTRY* PFNGLUNLOCKARRAYSEXTPROC)();
typedef void (APIENTRY* PFNGLACTIVETEXTUREARBPROC)(GLenum texture);
typedef void (APIENTRY* PFNGLCLIENTACTIVETEXTUREARBPROC)(GLenum texture);
typedef void (APIENTRY* PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY* PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY* PFNGLBINDBUFFERARBPROC)(GLenum target, GLuint buffer);
typedef void (APIENTRY* PFNGLDELETEBUFFERSARBPROC)(GLsizei n, const GLuint* buffers);
typedef void (APIENTRY* PFNGLGENBUFFERSARBPROC)(GLsizei n, GLuint* buffers);
typedef void (APIENTRY* PFNGLBUFFERDATAARBPROC)(GLenum target, GLsizeiptrARB size, const GLvoid* data, GLenum usage);
typedef void (APIENTRY* PFNGLBUFFERSUBDATAARBPROC)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid* data);
typedef GLvoid* (APIENTRY* PFNGLMAPBUFFERARBPROC)(GLenum target, GLenum access);
typedef GLboolean (APIENTRY* PFNGLUNMAPBUFFERARBPROC)(GLenum target);
typedef void (APIENTRY* PFNGLDRAWRANGEELEMENTSEXTPROC)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices);
typedef void (APIENTRY* PFNGLCOMBINERINPUTNVPROC)(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
typedef void (APIENTRY* PFNGLCOMBINEROUTPUTNVPROC)(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum);
typedef void (APIENTRY* PFNGLFINALCOMBINERINPUTNVPROC)(GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
typedef void (APIENTRY* PFNGLCOMBINERPARAMETERFVNVPROC)(GLenum pname, const GLfloat* params);
typedef void (APIENTRY* PFNGLCOMBINERPARAMETERINVPROC)(GLenum pname, GLint param);
typedef void (APIENTRY* PFNGLCOMBINERSTAGEPARAMETERFVNVPROC)(GLenum stage, GLenum pname, const GLfloat* params);
typedef void (APIENTRY* PFNGLPROGRAMSTRINGARBPROC)(GLenum target, GLenum format, GLsizei len, const GLvoid* string);
typedef void (APIENTRY* PFNGLBINDPROGRAMARBPROC)(GLenum target, GLuint program);
typedef void (APIENTRY* PFNGLPROGRAMENVPARAMETER4FVARBPROC)(GLenum target, GLuint index, const GLfloat* params);
typedef void (APIENTRY* PFNGLPROGRAMLOCALPARAMETER4FVARBPROC)(GLenum target, GLuint index, const GLfloat* params);
typedef void (APIENTRY* PFNGLPROGRAMENVPARAMETERS4FVEXTPROC)(GLenum target, GLuint index, GLsizei count, const GLfloat* params);
typedef void (APIENTRY* PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC)(GLenum target, GLuint index, GLsizei count, const GLfloat* params);
typedef void (APIENTRY* PFNGLDELETEPROGRAMSARBPROC)(GLsizei n, const GLuint* programs);
typedef void (APIENTRY* PFNGLGENPROGRAMSARBPROC)(GLsizei n, GLuint* programs);
typedef GLboolean (APIENTRY* PFNGLISPROGRAMARBPROC)(GLuint program);
typedef void (APIENTRY* PFNGLGETPROGRAMIVARBPROC)(GLenum target, GLenum pname, GLint* params);
typedef void (APIENTRY* PFNGLVERTEXATTRIBPOINTERARBPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
typedef void (APIENTRY* PFNGLENABLEVERTEXATTRIBARRAYARBPROC)(GLuint index);
typedef void (APIENTRY* PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)(GLuint index);
typedef void (APIENTRY* PFNGLGENQUERIESARBPROC)(GLsizei n, GLuint* ids);
typedef void (APIENTRY* PFNGLDELETEQUERIESARBPROC)(GLsizei n, const GLuint* ids);
typedef void (APIENTRY* PFNGLBEGINQUERYARBPROC)(GLenum target, GLuint id);
typedef void (APIENTRY* PFNGLENDQUERYARBPROC)(GLenum target);
typedef void (APIENTRY* PFNGLGETQUERYIVARBPROC)(GLenum target, GLenum pname, GLint* params);
typedef void (APIENTRY* PFNGLGETQUERYOBJECTIVARBPROC)(GLuint id, GLenum pname, GLint* params);
typedef void (APIENTRY* PFNGLGETQUERYOBJECTUIVARBPROC)(GLuint id, GLenum pname, GLuint* params);
typedef void (APIENTRY* PFNGLPOINTPARAMETERFARBPROC)(GLenum pname, GLfloat param);
typedef void (APIENTRY* PFNGLPOINTPARAMETERFVARBPROC)(GLenum pname, const GLfloat* params);
typedef void (APIENTRY* PFNGLBINDRENDERBUFFEREXTPROC)(GLenum target, GLuint renderbuffer);
typedef void (APIENTRY* PFNGLDELETERENDERBUFFERSEXTPROC)(GLsizei n, const GLuint* renderbuffers);
typedef void (APIENTRY* PFNGLGENRENDERBUFFERSEXTPROC)(GLsizei n, GLuint* renderbuffers);
typedef void (APIENTRY* PFNGLRENDERBUFFERSTORAGEEXTPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY* PFNGLBINDFRAMEBUFFEREXTPROC)(GLenum target, GLuint framebuffer);
typedef void (APIENTRY* PFNGLDELETEFRAMEBUFFERSEXTPROC)(GLsizei n, const GLuint* framebuffers);
typedef void (APIENTRY* PFNGLGENFRAMEBUFFERSEXTPROC)(GLsizei n, GLuint* framebuffers);
typedef GLenum (APIENTRY* PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)(GLenum target);
typedef void (APIENTRY* PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY* PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
typedef void (APIENTRY* PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRY* PFNGLBLENDCOLOREXTPROC)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);

typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC hdc);
typedef BOOL (WINAPI* PFNWGLGETPIXELFORMATATTRIBIVARBPROC)(HDC hdc, int pixelFormat, int layerPlane, UINT attributeCount, const int* attributes, int* values);
typedef BOOL (WINAPI* PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int* attribIList, const FLOAT* attribFList, UINT maxFormats, int* formats, UINT* numFormats);
typedef HPBUFFERARB (WINAPI* PFNWGLCREATEPBUFFERARBPROC)(HDC hdc, int pixelFormat, int width, int height, const int* attribList);
typedef HDC (WINAPI* PFNWGLGETPBUFFERDCARBPROC)(HPBUFFERARB pbuffer);
typedef int (WINAPI* PFNWGLRELEASEPBUFFERDCARBPROC)(HPBUFFERARB pbuffer, HDC hdc);
typedef BOOL (WINAPI* PFNWGLDESTROYPBUFFERARBPROC)(HPBUFFERARB pbuffer);
typedef BOOL (WINAPI* PFNWGLQUERYPBUFFERARBPROC)(HPBUFFERARB pbuffer, int attribute, int* value);
typedef BOOL (WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);
typedef void (APIENTRY* PFNGLGENPROGRAMSNVPROC)(GLsizei n, GLuint* programs);
typedef void (APIENTRY* PFNGLDELETEPROGRAMSNVPROC)(GLsizei n, const GLuint* programs);
typedef void (APIENTRY* PFNGLPROGRAMPARAMETERS4FVNVPROC)(GLenum target, GLuint index, GLsizei count, const GLfloat* params);
typedef void (APIENTRY* PFNGLBINDPROGRAMNVPROC)(GLenum target, GLuint id);
typedef void (APIENTRY* PFNGLLOADPROGRAMNVPROC)(GLenum target, GLuint id, GLsizei len, const GLubyte* program);

extern int32_t glExtGPUProgramParameters;
extern PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC glProgramLocalParameters4fvEXT;
extern PFNGLPROGRAMENVPARAMETERS4FVEXTPROC glProgramEnvParameters4fvEXT;
extern int32_t wglExtSwapControl;
extern int32_t wglArbPbuffer;
extern int32_t wglArbPixelFormat;
extern int32_t glATIEnvmapBumpmap;
extern int32_t glATIFragmentShader;
extern int32_t glNVMultisampleFilterHint;
extern int32_t glNVTextureShader3;
extern int32_t glNVTextureShader2;
extern int32_t glNVTextureShader;
extern int32_t glNVRegisterCombiners2;
extern int32_t glNVRegisterCombiners;
extern int32_t glUnkC60498;
extern int32_t glSGISGenerateMipmap;
extern int32_t glExtBlendColor;
extern int32_t glExtFramebufferMultisample;
extern int32_t glExtFramebufferObject;
extern int32_t glExtDrawRangeElements;
extern int32_t glExtTextureFilterAnisotropic;
extern int32_t glExtTextureCompressionS3tc;
extern int32_t glExtTextureCompression;
extern int32_t glExtTextureLodBias;
extern int32_t glExtClampToEdge;
extern int32_t glExtBgra;
extern int32_t glARBPointSprite;
extern int32_t glARBPointParameters;
extern int32_t glARBOcclusionQuery;
extern int32_t glARBMultisample;
extern int32_t glARBTextureCubeMap;
extern int32_t glARBTextureRectangle;
extern int32_t glARBTextureEnvCombine;
extern int32_t glARBVertexBufferObject;
extern int32_t glARBFragmentProgram;
extern int32_t glARBVertexProgram;
extern PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
extern PFNWGLQUERYPBUFFERARBPROC wglQueryPbufferARB;
extern PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB;
extern PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB;
extern PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB;
extern PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB;
extern PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
extern PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB;
extern PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
extern PFNGLBLENDCOLOREXTPROC glBlendColorEXT;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
extern PFNGLPOINTPARAMETERFVARBPROC glPointParameterfvARB;
extern PFNGLPOINTPARAMETERFARBPROC glPointParameterfARB;
extern PFNGLGETQUERYOBJECTUIVARBPROC glGetQueryObjectuivARB;
extern PFNGLGETQUERYOBJECTIVARBPROC glGetQueryObjectivARB;
extern PFNGLGETQUERYIVARBPROC glGetQueryivARB;
extern PFNGLENDQUERYARBPROC glEndQueryARB;
extern PFNGLBEGINQUERYARBPROC glBeginQueryARB;
extern PFNGLDELETEQUERIESARBPROC glDeleteQueriesARB;
extern PFNGLGENQUERIESARBPROC glGenQueriesARB;
extern PFNGLUNMAPBUFFERARBPROC glUnmapBufferARB;
extern PFNGLMAPBUFFERARBPROC glMapBufferARB;
extern PFNGLBUFFERSUBDATAARBPROC glBufferSubDataARB;
extern PFNGLBUFFERDATAARBPROC glBufferDataARB;
extern PFNGLGENBUFFERSARBPROC glGenBuffersARB;
extern PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB;
extern PFNGLBINDBUFFERARBPROC glBindBufferARB;
extern PFNGLGENPROGRAMSNVPROC glGenProgramsNV;
extern PFNGLDELETEPROGRAMSNVPROC glDeleteProgramsNV;
extern PFNGLPROGRAMPARAMETERS4FVNVPROC glProgramParameters4fvNV;
extern PFNGLBINDPROGRAMNVPROC glBindProgramNV;
extern PFNGLLOADPROGRAMNVPROC glLoadProgramNV;
extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB;
extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB;
extern PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB;
extern PFNGLGETPROGRAMIVARBPROC glGetProgramivARB;
extern PFNGLISPROGRAMARBPROC glIsProgramARB;
extern PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glProgramLocalParameter4fvARB;
extern PFNGLPROGRAMENVPARAMETER4FVARBPROC glProgramEnvParameter4fvARB;
extern PFNGLGENPROGRAMSARBPROC glGenProgramsARB;
extern PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB;
extern PFNGLBINDPROGRAMARBPROC glBindProgramARB;
extern PFNGLPROGRAMSTRINGARBPROC glProgramStringARB;
extern PFNGLCOMBINERPARAMETERINVPROC glCombinerParameteriNV;
extern PFNGLCOMBINERPARAMETERFVNVPROC glCombinerParameterfvNV;
extern PFNGLFINALCOMBINERINPUTNVPROC glFinalCombinerInputNV;
extern PFNGLCOMBINERSTAGEPARAMETERFVNVPROC glCombinerStageParameterfvNV;
extern PFNGLCOMBINEROUTPUTNVPROC glCombinerOutputNV;
extern PFNGLCOMBINERINPUTNVPROC glCombinerInputNV;
extern PFNGLDRAWRANGEELEMENTSEXTPROC glDrawRangeElementsEXT;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2DARB;
extern PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2DARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
extern PFNGLUNLOCKARRAYSEXTPROC glUnlockArraysEXT;
extern PFNGLLOCKARRAYSEXTPROC glLockArraysEXT;
extern GLint glMaxTextureImageUnitsARB;
extern GLint glMaxTextureUnits;
extern int32_t glExtCVA;
extern uint32_t glVersion;

void BindGlExtensions();
void BindWglExtensions();
bool FindGlExt(const char* name);
bool HasWGLExtension(const char* name);
void UnbindGlExtensions();

#endif
