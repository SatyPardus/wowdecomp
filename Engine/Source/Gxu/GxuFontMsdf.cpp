#include <cstddef>
#include <cstdint>
#include <cstring>
#include <windows.h>
#include <d3dcommon.h>

#include "Engine/Source/Gxu/GxuFontMsdf.h"
#include "Engine/Source/Gxu/GxuFontPlugin.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxDevice/CGxShader.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC
uint8_t g_msdfEnabled = 0;
static IGxuFontBackend* s_fontBackend = nullptr;
static HMODULE s_msdfPluginModule = nullptr;

typedef IGxuFontBackend* (*pfn_CreateFontBackend)();
typedef HRESULT (WINAPI *pfn_D3DCompile)(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    LPCSTR pSourceName,
    const void* pDefines,
    void* pInclude,
    LPCSTR pEntrypoint,
    LPCSTR pTarget,
    UINT Flags1,
    UINT Flags2,
    ID3DBlob** ppCode,
    ID3DBlob** ppErrorMsgs
);

static void* GxuFontAlloc(size_t size) {
    return SMemAlloc(size, __FILE__, __LINE__, 0);
}

static void GxuFontFree(void* ptr) {
    if (ptr) {
        SMemFree(ptr, __FILE__, __LINE__, 0);
    }
}

void Gxu_RegisterFontBackend(IGxuFontBackend* backend) {
    s_fontBackend = backend;
    g_msdfEnabled = (backend != nullptr) ? 1 : 0;
}

IGxuFontBackend* Gxu_GetFontBackend() {
    return s_fontBackend;
}

void MsdfInitialize() {
    // Attempt dynamic discovery of isolated MSDF plugin
    const char* pluginPaths[] = {
        "plugins\\msdf_renderer.dll",
        "msdf_renderer.dll",
        "bin\\Win\\plugins\\msdf_renderer.dll",
        "plugins\\msdf_renderer\\msdf_renderer.dll"
    };

    for (const char* path : pluginPaths) {
        s_msdfPluginModule = LoadLibraryA(path);
        if (s_msdfPluginModule) {
            auto createBackend = reinterpret_cast<pfn_CreateFontBackend>(
                GetProcAddress(s_msdfPluginModule, "CreateFontBackend")
            );
            if (createBackend) {
                IGxuFontBackend* backend = createBackend();
                if (backend && backend->Initialize(GxuFontAlloc, GxuFontFree)) {
                    Gxu_RegisterFontBackend(backend);
                    break;
                }
            }
            FreeLibrary(s_msdfPluginModule);
            s_msdfPluginModule = nullptr;
        }
    }
}

void MsdfShutdown() {
    if (s_fontBackend) {
        s_fontBackend->Shutdown();
        s_fontBackend = nullptr;
    }
    if (s_msdfPluginModule) {
        FreeLibrary(s_msdfPluginModule);
        s_msdfPluginModule = nullptr;
    }
    g_msdfEnabled = 0;
}

float MsdfComputeScreenPxRange(uint32_t renderedPixelSize) {
    if (s_fontBackend) {
        return s_fontBackend->ComputeScreenPxRange(renderedPixelSize);
    }
    float scale = static_cast<float>(renderedPixelSize) / static_cast<float>(MSDF_GLYPH_SIZE);
    return scale * static_cast<float>(MSDF_PIXEL_RANGE);
}

bool MsdfGenerateGlyph(FT_Face face, uint32_t pixelSize, uint32_t code, uint32_t baseline, GLYPHBITMAPDATA* dataPtr, uint32_t padding) {
    if (!g_fontMsdfPixelShader) {
        g_fontMsdfPixelShader = GxuFontCreateMsdfPixelShader();
    }
    if (s_fontBackend && g_msdfEnabled && g_fontMsdfPixelShader && g_fontMsdfPixelShader->Valid()) {
        return s_fontBackend->GenerateGlyph(face, pixelSize, code, baseline, dataPtr, padding);
    }
    return false;
}

CGxShader* GxuFontCreateMsdfPixelShader() {
    if (!g_theGxDevicePtr || !s_fontBackend || !g_msdfEnabled) {
        return nullptr;
    }

    CGxShader* existing = g_theGxDevicePtr->m_shaderList[GxSh_Pixel].Ptr("UI_MSDF");
    if (existing) {
        existing->m_refCount++;
        return existing;
    }

    CGxShader* shader = g_theGxDevicePtr->m_shaderList[GxSh_Pixel].New("UI_MSDF", 0, 0);
    if (!shader) {
        return nullptr;
    }

    shader->m_refCount = 1;
    shader->m_target = GxSh_Pixel;

    uint32_t bytecodeSize = 0;
    const void* bytecode = s_fontBackend->GetPixelShaderBytecode(&bytecodeSize);
    const char* hlslSource = s_fontBackend->GetPixelShaderHlsl();

    HMODULE hD3DCompiler = LoadLibraryA("d3dcompiler_47.dll");
    if (!hD3DCompiler) {
        hD3DCompiler = LoadLibraryA("d3dcompiler_43.dll");
    }

    if (hD3DCompiler && hlslSource) {
        pfn_D3DCompile d3dCompile = reinterpret_cast<pfn_D3DCompile>(GetProcAddress(hD3DCompiler, "D3DCompile"));
        if (d3dCompile) {
            ID3DBlob* codeBlob = nullptr;
            ID3DBlob* errorBlob = nullptr;
            HRESULT hr = d3dCompile(
                hlslSource,
                strlen(hlslSource),
                "UI_MSDF",
                nullptr,
                nullptr,
                "main",
                "ps_2_a",
                0,
                0,
                &codeBlob,
                &errorBlob
            );

            if (SUCCEEDED(hr) && codeBlob) {
                bytecode = codeBlob->GetBufferPointer();
                bytecodeSize = static_cast<uint32_t>(codeBlob->GetBufferSize());
            }

            if (errorBlob) {
                errorBlob->Release();
            }

            shader->m_code.SetCount(bytecodeSize);
            memcpy(shader->m_code.m_data, bytecode, bytecodeSize);

            if (codeBlob) {
                codeBlob->Release();
            }

            FreeLibrary(hD3DCompiler);
            hD3DCompiler = nullptr;
        } else {
            FreeLibrary(hD3DCompiler);
            hD3DCompiler = nullptr;
            shader->m_code.SetCount(bytecodeSize);
            memcpy(shader->m_code.m_data, bytecode, bytecodeSize);
        }
    } else {
        shader->m_code.SetCount(bytecodeSize);
        memcpy(shader->m_code.m_data, bytecode, bytecodeSize);
    }

    shader->m_loaded = 0;
    shader->m_valid = 0;

    if (!shader->Valid()) {
        g_msdfEnabled = 0;
        return nullptr;
    }

    return shader;
}
