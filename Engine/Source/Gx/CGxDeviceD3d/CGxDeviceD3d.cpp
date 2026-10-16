#include <cmath>
#include <cstdint>
#include <cstring>
#include <malloc.h>
#include "Engine/Source/Gx/CGxDeviceD3d/CGxDeviceD3d.h"
#include "Engine/Source/Gx/Blit.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Gx/CGxTex.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Os/Error.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "ThirdParty/nvapi/nvapi.h"

typedef IDirect3D9* (WINAPI* Direct3DCreate9Proc)(UINT sdkVersion);

// STATIC: 0x00A2E4A8
const D3DFORMAT CGxDeviceD3d::s_GxFormatToD3dFormat[CGxFormat::Formats_Last] = { D3DFMT_R5G6B5, D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_A2R10G10B10, D3DFMT_D16, D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D32 };

// STATIC: 0x00A2E4C8
const uint32_t CGxDeviceD3d::s_gxAttribToD3dAttribSize[7] = { 4, 4, 4, 8, 12, 4, 4 };

// STATIC: 0x00A2E4E4
static const D3DQUERYTYPE s_d3dQueryTypes[1] = { D3DQUERYTYPE_OCCLUSION };

// STATIC: 0x00A2F7E8
const EGxTexFormat CGxDeviceD3d::s_tolerableTexFmtMapping[GxTexFormats_Last] = { GxTex_Unknown, GxTex_Argb4444, GxTex_Argb4444, GxTex_Argb4444, GxTex_Argb4444, GxTex_Argb4444, GxTex_Dxt1, GxTex_Dxt3, GxTex_Dxt5, GxTex_Uv88, GxTex_Gr1616F, GxTex_R32F, GxTex_D24X8 };

// STATIC: 0x00A2F850
const D3DCUBEMAP_FACES CGxDeviceD3d::s_faceTypes[6] = { D3DCUBEMAP_FACE_POSITIVE_X, D3DCUBEMAP_FACE_NEGATIVE_X, D3DCUBEMAP_FACE_POSITIVE_Y, D3DCUBEMAP_FACE_NEGATIVE_Y, D3DCUBEMAP_FACE_POSITIVE_Z, D3DCUBEMAP_FACE_NEGATIVE_Z };

// STATIC: 0x00A2F81C
const D3DFORMAT CGxDeviceD3d::s_GxTexFmtToD3dFmt[GxTexFormats_Last] = { D3DFMT_UNKNOWN, D3DFMT_A8B8G8R8, D3DFMT_A8R8G8B8, D3DFMT_A4R4G4B4, D3DFMT_A1R5G5B5, D3DFMT_R5G6B5, D3DFMT_DXT1, D3DFMT_DXT3, D3DFMT_DXT5, D3DFMT_V8U8, D3DFMT_G16R16F, D3DFMT_R32F, D3DFMT_D24X8 };

// STATIC: 0x00A2F8D4
const D3DPRIMITIVETYPE CGxDeviceD3d::s_primitiveConversion[GxPrims_Last] = { D3DPT_POINTLIST, D3DPT_LINELIST, D3DPT_LINESTRIP, D3DPT_TRIANGLELIST, D3DPT_TRIANGLESTRIP, D3DPT_TRIANGLEFAN };

// STATIC: 0x00A2F8EC
const D3DDECLUSAGE CGxDeviceD3d::s_gxAttribToD3dAttribUsage[GxVAs_Last] = { D3DDECLUSAGE_POSITION, D3DDECLUSAGE_BLENDWEIGHT, D3DDECLUSAGE_BLENDINDICES, D3DDECLUSAGE_NORMAL, D3DDECLUSAGE_COLOR, D3DDECLUSAGE_COLOR, D3DDECLUSAGE_TEXCOORD, D3DDECLUSAGE_TEXCOORD, D3DDECLUSAGE_TEXCOORD, D3DDECLUSAGE_TEXCOORD, D3DDECLUSAGE_TEXCOORD, D3DDECLUSAGE_TEXCOORD, D3DDECLUSAGE_TEXCOORD, D3DDECLUSAGE_TEXCOORD };

// STATIC: 0x00A2F924
const uint32_t CGxDeviceD3d::s_gxAttribToD3dAttribUsageIndex[GxVAs_Last] = { 0, 0, 0, 0, 0, 1, 0, 1, 2, 3, 4, 5, 6, 7 };

// STATIC: 0x00A2F964
const D3DBLEND CGxDeviceD3d::s_srcBlend[12] = { D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA, D3DBLEND_DESTCOLOR, D3DBLEND_DESTCOLOR, D3DBLEND_DESTCOLOR, D3DBLEND_INVSRCALPHA, D3DBLEND_INVSRCALPHA, D3DBLEND_SRCALPHA, D3DBLEND_ONE, D3DBLEND_BLENDFACTOR };

// STATIC: 0x00A2F994
const D3DBLEND CGxDeviceD3d::s_dstBlend[12] = { D3DBLEND_ZERO, D3DBLEND_ZERO, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_ZERO, D3DBLEND_SRCCOLOR, D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_ZERO, D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_INVBLENDFACTOR };

// STATIC: 0x00A2F9C4
const D3DTEXTUREADDRESS CGxDeviceD3d::s_wrapModes[2] = { D3DTADDRESS_CLAMP, D3DTADDRESS_WRAP };

// STATIC: 0x00A2F9CC
static const D3DTEXTUREOP s_texOp[6] = { D3DTOP_MODULATE, D3DTOP_MODULATE2X, D3DTOP_ADD, D3DTOP_SELECTARG2, D3DTOP_BLENDCURRENTALPHA, D3DTOP_BLENDDIFFUSEALPHA };

// STATIC: 0x00A2F9E4
static const uint32_t s_texArgs[6][2] = { { D3DTA_TEXTURE, D3DTA_CURRENT }, { D3DTA_TEXTURE, D3DTA_CURRENT }, { D3DTA_TEXTURE, D3DTA_CURRENT }, { D3DTA_TEXTURE, D3DTA_CURRENT }, { D3DTA_CURRENT, D3DTA_TEXTURE }, { D3DTA_TEXTURE, D3DTA_CURRENT } };

// STATIC: 0x00A2FA14
const D3DCMPFUNC CGxDeviceD3d::s_cmpFunc[4] = { D3DCMP_LESSEQUAL, D3DCMP_EQUAL, D3DCMP_GREATEREQUAL, D3DCMP_LESS };

// STATIC: 0x00A2FA24
const D3DCULL CGxDeviceD3d::s_cullMode[3] = { D3DCULL_NONE, D3DCULL_CW, D3DCULL_CCW };

// STATIC: 0x00AD8EF0
EGxTexFormat CGxDeviceD3d::s_GxTexFormatToUse[GxTexFormats_Last] = { GxTex_Unknown, GxTex_Abgr8888, GxTex_Argb8888, GxTex_Argb4444, GxTex_Argb1555, GxTex_Rgb565, GxTex_Dxt1, GxTex_Dxt3, GxTex_Dxt5, GxTex_Uv88, GxTex_Gr1616F, GxTex_R32F, GxTex_D24X8 };

// STATIC: 0x00AD8F24
D3DDECLTYPE CGxDeviceD3d::s_gxAttribToD3dAttribType[7] = { D3DDECLTYPE_D3DCOLOR, D3DDECLTYPE_UBYTE4, D3DDECLTYPE_UBYTE4N, D3DDECLTYPE_FLOAT2, D3DDECLTYPE_FLOAT3, D3DDECLTYPE_SHORT2, D3DDECLTYPE_FLOAT1 };

// STATIC: 0x00AD8F40
D3DTEXTUREFILTERTYPE CGxDeviceD3d::s_filterModes[GxTexFilters_Last][3] = { { D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE }, { D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE }, { D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_POINT }, { D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_POINT }, { D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR }, { D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR } };

// STATIC: 0x00AD8F88
static uint32_t s_normalizeNormals = 5;

// STATIC: 0x00C6033C
static LPDIRECT3DSURFACE9 s_savedDepthSurface;

// STATIC: 0x00C606B0
static D3DMATERIAL9 s_d3dMaterial;

// STATIC: 0x00C606F8
static D3DLIGHT9 s_d3dLight;

// STATIC: 0x00C60760
static int32_t s_isIdent;

// VERIFY: 271a03c4 manual
// OFFSET: 0x0068FA60
void CGxDeviceD3d::IBufUnlock(CGxBuf* buf) {
    CGxPool* pool = buf->m_pool;
    EmergencyMem& emergencyMem = this->m_emergencyMem[pool->m_target];

    if (emergencyMem.m_lock) {
        emergencyMem.Unlock();
        buf->m_valid = 0;
        this->m_intF5C = 1;
        return;
    }

    HRESULT result = S_OK;

    switch (pool->m_target) {
        case GxPoolTarget_Vertex:
            result = static_cast<LPDIRECT3DVERTEXBUFFER9>(pool->m_apiSpecific)->Unlock();
            break;

        case GxPoolTarget_Index:
            result = static_cast<LPDIRECT3DINDEXBUFFER9>(pool->m_apiSpecific)->Unlock();
            break;

        default:
            break;
    }

    buf->m_valid = SUCCEEDED(result);
}

// VERIFY: 32fdd761 auto
// OFFSET: 0x0068FAE0
int32_t CGxDeviceD3d::BufUnlock(CGxBuf* buf, uint32_t size) {
    CGxDevice::BufUnlock(buf, size);
    this->IBufUnlock(buf);

    return 1;
}

// VERIFY: e175e4e3 manual
// OFFSET: 0x0068FB10
char* CGxDeviceD3d::IBufLock(CGxBuf* buf) {
    CGxPool* pool = buf->m_pool;

    if (!this->m_context) {
        return static_cast<char*>(this->m_emergencyMem[pool->m_target].Lock(buf->m_size));
    }

    DWORD flags = 0;

    if (pool->m_usage == GxPoolUsage_Stream) {
        uint32_t index = (buf->m_itemSize + pool->m_offset - 1) - (buf->m_itemSize + pool->m_offset - 1) % buf->m_itemSize;

        if (index + buf->m_size > static_cast<uint32_t>(pool->m_size)) {
            flags = D3DLOCK_DISCARD;
            pool->Discard();
            buf->m_index = 0;
            pool->m_offset = buf->m_size;
        } else {
            flags = D3DLOCK_NOOVERWRITE;
            buf->m_index = index;
            pool->m_offset = index + buf->m_size;
        }
    } else if (pool->m_usage == GxPoolUsage_Dynamic) {
        flags = D3DLOCK_NOOVERWRITE;
    }

    char* data = nullptr;

    if (!pool->m_apiSpecific) {
        this->CreatePoolAPI(pool);
    }

    if (!pool->m_apiSpecific) {
        return static_cast<char*>(this->m_emergencyMem[pool->m_target].Lock(buf->m_size));
    }

    HRESULT result = S_OK;

    switch (pool->m_target) {
        case GxPoolTarget_Vertex:
            result = static_cast<LPDIRECT3DVERTEXBUFFER9>(pool->m_apiSpecific)->Lock(buf->m_index, buf->m_size, reinterpret_cast<void**>(&data), flags);
            break;

        case GxPoolTarget_Index:
            result = static_cast<LPDIRECT3DINDEXBUFFER9>(pool->m_apiSpecific)->Lock(buf->m_index, buf->m_size, reinterpret_cast<void**>(&data), flags);
            break;

        default:
            break;
    }

    if (SUCCEEDED(result) && buf->m_size) {
        __try {
            volatile char* probe = data;

            if (pool->m_usage == GxPoolUsage_Stream) {
                *probe = 0;
            } else {
                *probe = *probe;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            result = E_FAIL;
        }
    }

    if (FAILED(result)) {
        this->IBufUnlock(buf);
        return static_cast<char*>(this->m_emergencyMem[pool->m_target].Lock(buf->m_size));
    }

    return data;
}

// VERIFY: 14c57719 auto
// OFFSET: 0x0068FCE0
char* CGxDeviceD3d::BufLock(CGxBuf* buf) {
    CGxDevice::BufLock(buf);

    return this->IBufLock(buf);
}

// VERIFY: aa2288d7 auto
// OFFSET: 0x0068FD00
void CGxDeviceD3d::BufData(CGxBuf* buf, const void* data, uint32_t size, uint32_t offset) {
    CGxDevice::BufData(buf, data, size, offset);

    char* dst = this->IBufLock(buf);
    memcpy(dst + offset, data, size);
    this->IBufUnlock(buf);
}

// VERIFY: c3d9f239 auto
// OFFSET: 0x0068FD50
CGxDeviceD3d::CGxDeviceD3d() : m_desktopDisplayMode() {
    this->m_d3dStereoConvergence = 1.0f;
    this->m_d3dStereoSeparation = 25.0f;
    this->m_api = GxApi_D3d9;
    this->m_hwnd = nullptr;
    this->m_d3dLib = nullptr;
    this->m_d3d = nullptr;
    this->m_d3dDevice = nullptr;
    this->m_d3dIsHwDevice = 0;
    this->m_d3dNVAPI = 0;
    this->m_d3dStereoEnabled = 0;
    this->m_d3dStereoRestore = 0;
    this->m_d3dStereoHandle = 0;
    this->m_d3dStereoDirty = 0;
    this->m_caps.m_colorFormat = GxCF_argb;
    this->m_caps.unk_0018 = 1;
    memset(this->m_deviceStates, 0xFF, sizeof(this->m_deviceStates));
    memset(this->m_texEnabled, 0, sizeof(this->m_texEnabled));
    memset(this->m_d3dVertexDecl, 0, sizeof(this->m_d3dVertexDecl));
    this->m_defDepthStencilSurface = nullptr;
    this->m_defColorSurface = nullptr;
    this->m_defDepthSurface = nullptr;
    this->m_surface3B44 = nullptr;
    this->m_eventQuery = nullptr;
    this->m_hwCursorDirty = 1;
    this->m_hwCursorTexture = nullptr;
    this->m_hwCursorBitmap = nullptr;
    this->m_texture3B58 = nullptr;
    this->DeviceCreatePools();
    this->DeviceCreateStreamBufs();
}

// VERIFY: 57303b58 auto
// OFFSET: 0x0068E140
void CGxDeviceD3d::IUnloadD3dLib(HINSTANCE& d3dLib, LPDIRECT3D9& d3d) {
    if (d3d) {
        d3d->Release();
        d3d = nullptr;
    }

    if (d3dLib) {
        FreeLibrary(d3dLib);
        d3dLib = nullptr;
    }
}

// VERIFY: 211a94b3 manual
// OFFSET: 0x0068E180
LPDIRECT3DINDEXBUFFER9 CGxDeviceD3d::ICreateD3dIB(EGxPoolUsage usage, uint32_t size) {
    D3DPOOL d3dPool = D3DPOOL_MANAGED;
    DWORD d3dUsage = D3DUSAGE_WRITEONLY | (this->m_d3dIsHwDevice ? 0 : D3DUSAGE_SOFTWAREPROCESSING);

    if (usage == GxPoolUsage_Stream || usage == GxPoolUsage_Dynamic) {
        d3dUsage |= D3DUSAGE_DYNAMIC;
        d3dPool = D3DPOOL_DEFAULT;
    }

    LPDIRECT3DINDEXBUFFER9 indexBuffer = nullptr;
    HRESULT hr = this->m_d3dDevice->CreateIndexBuffer(size, d3dUsage, D3DFMT_INDEX16, d3dPool, &indexBuffer, nullptr);

    return SUCCEEDED(hr) ? indexBuffer : nullptr;
}

// VERIFY: 72eca02b auto
// OFFSET: 0x0068E1F0
void CGxDeviceD3d::IReleaseD3dPool(CGxPool* pool) {
    if (pool->m_usage == GxPoolUsage_Stream) {
        pool->m_offset = 0;
    }

    pool->Invalidate();

    if (pool->m_apiSpecific) {
        switch (pool->m_target) {
            case GxPoolTarget_Vertex:
                static_cast<LPDIRECT3DVERTEXBUFFER9>(pool->m_apiSpecific)->Release();
                break;

            case GxPoolTarget_Index:
                static_cast<LPDIRECT3DINDEXBUFFER9>(pool->m_apiSpecific)->Release();
                break;

            default:
                break;
        }

        pool->m_apiSpecific = nullptr;
    }
}

// VERIFY: e9919487 manual
// OFFSET: 0x0068E450
void CGxDeviceD3d::DeviceEvictResources() {
    if (!this->m_context || !this->m_d3dDevice) {
        return;
    }

    HRESULT hr = this->m_d3dDevice->EvictManagedResources();

    if (FAILED(hr) && hr == D3DERR_DRIVERINTERNALERROR) {
        SErrDisplayError(STORM_ERROR_FATAL_CONDITION, __FILE__, __LINE__, "Driver internal error on EvictManagedResources().", 0, 1, 0x11111111);
    }
}

// VERIFY: a0f60ba9 auto
// OFFSET: 0x0068E4C0
void CGxDeviceD3d::DeviceSetGamma(float gamma) {
    CGxDevice::DeviceSetGamma(gamma);

    if ((this->m_d3dCaps.Caps2 & D3DCAPS2_FULLSCREENGAMMA) && !this->IDevIsWindowed()) {
        this->m_d3dDevice->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, reinterpret_cast<const D3DGAMMARAMP*>(&this->m_gammaRamp));
    }
}

// VERIFY: e62cb4c1 auto
// OFFSET: 0x0068E720
void CGxDeviceD3d::PoolDestroy(CGxPool* pool) {
    if (pool) {
        this->IReleaseD3dPool(pool);
        CGxDevice::PoolDestroy(pool);
    }
}

// VERIFY: b07d4055 auto
// OFFSET: 0x0068E750
void CGxDeviceD3d::CursorSetVisible(int32_t visible) {
    CGxDevice::CursorSetVisible(visible);

    if (this->m_hwCursor && this->m_context) {
        POINT point;
        GetCursorPos(&point);
        ScreenToClient(this->m_hwnd, &point);

        RECT rect;
        GetClientRect(this->m_hwnd, &rect);

        if (point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom) {
            this->m_d3dDevice->ShowCursor(this->m_cursorVisible);
        }
    }
}

// VERIFY: 9a0772fe auto
// OFFSET: 0x0068E7E0
void CGxDeviceD3d::CursorUnlock(uint32_t x, uint32_t y) {
    CGxDevice::CursorUnlock(x, y);
    this->m_hwCursorDirty = 1;
}

// VERIFY: ec65e113 auto
// OFFSET: 0x0068E810
void CGxDeviceD3d::ICursorDraw() {
    if (!this->m_hwCursor) {
        this->ISceneBegin();
    }

    CGxDevice::ICursorDraw();

    if (!this->m_hwCursor) {
        this->ISceneEnd();
    }

    if (!this->m_hwCursor || !this->m_hwCursorDirty || !this->m_hwCursorBitmap || !this->m_context) {
        return;
    }

    D3DLOCKED_RECT lockedRect;
    if (SUCCEEDED(this->m_hwCursorBitmap->LockRect(&lockedRect, nullptr, 0))) {
        for (uint32_t y = 0; y < 32; y++) {
            memcpy(static_cast<uint8_t*>(lockedRect.pBits) + y * lockedRect.Pitch, &this->m_cursor[y * 32], 32 * sizeof(uint32_t));
        }

        this->m_hwCursorBitmap->UnlockRect();
        this->m_d3dDevice->SetCursorProperties(this->m_cursorHotspotX, this->m_cursorHotspotY, this->m_hwCursorBitmap);
    }

    this->m_hwCursorDirty = 0;
}

// VERIFY: 79dfb7f5 auto
// OFFSET: 0x0068E900
void CGxDeviceD3d::ICursorCreate(const CGxFormat& format) {
    CGxDevice::ICursorCreate(format);

    if (this->m_hwCursor && !this->m_hwCursorTexture) {
        this->m_d3dDevice->CreateTexture(32, 32, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &this->m_hwCursorTexture, nullptr);

        if (this->m_hwCursorTexture) {
            this->m_hwCursorTexture->GetSurfaceLevel(0, &this->m_hwCursorBitmap);
        }

        this->m_hwCursorDirty = 1;
        this->ICursorDraw();
    }
}

// VERIFY: 8d560b80 auto
// OFFSET: 0x0068E980
void CGxDeviceD3d::StereoSetConvergence(float convergence) {
    this->m_d3dStereoDirty = 1;
    this->m_d3dStereoConvergence = convergence;
}

// VERIFY: ef5be528 auto
// OFFSET: 0x0068E9A0
void CGxDeviceD3d::StereoSetSeparation(float separation) {
    this->m_d3dStereoDirty = 1;
    this->m_d3dStereoSeparation = separation;
}

// VERIFY: 296eb823 auto
// OFFSET: 0x0068E9C0
void CGxDeviceD3d::QueryCreate(CGxQuery*& query, EGxQueryType type) {
    CGxDevice::QueryCreate(query, type);

    CGxQuery* gxQuery = query;
    LPDIRECT3DQUERY9 d3dQuery;

    if (!gxQuery->m_apiSpecific && this->m_d3dDevice->CreateQuery(s_d3dQueryTypes[gxQuery->m_type], &d3dQuery) == D3D_OK) {
        gxQuery->m_apiSpecific = d3dQuery;
    }
}

// VERIFY: 04d8fee3 auto
// OFFSET: 0x0068EA10
bool CGxDeviceD3d::QueryBegin(CGxQuery* query) {
    if (!this->m_context) {
        return false;
    }

    LPDIRECT3DQUERY9 d3dQuery;

    if (!query->m_apiSpecific) {
        if (this->m_d3dDevice->CreateQuery(s_d3dQueryTypes[query->m_type], &d3dQuery) == D3D_OK) {
            query->m_apiSpecific = d3dQuery;
        }

        if (!query->m_apiSpecific) {
            return false;
        }
    }

    HRESULT hr = D3D_OK;

    if (query->m_type == GxQueryType_Occlusion) {
        hr = static_cast<LPDIRECT3DQUERY9>(query->m_apiSpecific)->Issue(D3DISSUE_BEGIN);
    }

    return hr == D3D_OK;
}

// VERIFY: d92bc702 auto
// OFFSET: 0x0068EA90
bool CGxDeviceD3d::QueryGetParam(CGxQuery* query, EGxQueryParam param, uint32_t& data) {
    if (!this->m_context) {
        return false;
    }

    LPDIRECT3DQUERY9 d3dQuery;

    if (!query->m_apiSpecific) {
        if (this->m_d3dDevice->CreateQuery(s_d3dQueryTypes[query->m_type], &d3dQuery) == D3D_OK) {
            query->m_apiSpecific = d3dQuery;
        }

        if (!query->m_apiSpecific) {
            return false;
        }
    }

    switch (param) {
        case GxQueryParam_Unk0:
            data = static_cast<LPDIRECT3DQUERY9>(query->m_apiSpecific)->GetDataSize();
            return true;

        case GxQueryParam_Unk1:
            data = static_cast<LPDIRECT3DQUERY9>(query->m_apiSpecific)->GetData(nullptr, 0, 0) == D3D_OK;
            return false;

        default:
            return false;
    }
}

// VERIFY: 012b93fa manual
// OFFSET: 0x0068ED80
int32_t CGxDeviceD3d::ILoadD3dLib(HINSTANCE& d3dLib, LPDIRECT3D9& d3d) {
    d3dLib = nullptr;
    d3d = nullptr;
    d3dLib = LoadLibraryA("d3d9.dll");

    if (d3dLib) {
        Direct3DCreate9Proc d3dCreateProc = reinterpret_cast<Direct3DCreate9Proc>(GetProcAddress(d3dLib, "Direct3DCreate9"));

        if (d3dCreateProc) {
            d3d = d3dCreateProc(D3D_SDK_VERSION);

            if (d3d) {
                return 1;
            }

            CGxDevice::Log("CGxDeviceD3d::ILoadD3dLib(): unable to d3dCreateProc()");
        } else {
            CGxDevice::Log("CGxDeviceD3d::ILoadD3dLib(): unable to GetProcAddress()");
        }
    } else {
        CGxDevice::Log("CGxDeviceD3d::ILoadD3dLib(): unable to LoadLibrary()");
    }

    CGxDeviceD3d::IUnloadD3dLib(d3dLib, d3d);

    return 0;
}

// VERIFY: b78ec2a8 auto
// OFFSET: 0x0069FE10
void CGxDeviceD3d::ISetWindowFocus(int32_t focus) {
    this->m_windowFocus = focus;

    if (focus) {
        this->m_hwCursorDirty = 1;

        if (!this->m_format.m_window) {
            RECT rect;
            GetWindowRect(this->m_hwnd, &rect);
            ClipCursor(&rect);
        }
    }
}

// VERIFY: d0799ea2 manual
// OFFSET: 0x0069FB00
LPDIRECT3DVERTEXBUFFER9 CGxDeviceD3d::ICreateD3dVB(EGxPoolUsage usage, uint32_t size) {
    D3DPOOL d3dPool = D3DPOOL_MANAGED;
    DWORD d3dUsage = D3DUSAGE_WRITEONLY | (this->m_d3dIsHwDevice ? 0 : D3DUSAGE_SOFTWAREPROCESSING);

    if (usage == GxPoolUsage_Stream || usage == GxPoolUsage_Dynamic) {
        d3dUsage |= D3DUSAGE_DYNAMIC;
        d3dPool = D3DPOOL_DEFAULT;
    }

    LPDIRECT3DVERTEXBUFFER9 vertexBuffer = nullptr;
    HRESULT hr = this->m_d3dDevice->CreateVertexBuffer(size, d3dUsage, 0, d3dPool, &vertexBuffer, nullptr);

    return SUCCEEDED(hr) ? vertexBuffer : nullptr;
}

// VERIFY: 24701c91 auto
// OFFSET: 0x0069FB70
int32_t CGxDeviceD3d::CreatePoolAPI(CGxPool* pool) {
    switch (pool->m_target) {
        case GxPoolTarget_Vertex:
            pool->m_apiSpecific = this->ICreateD3dVB(pool->m_usage, pool->m_size);
            break;

        case GxPoolTarget_Index:
            pool->m_apiSpecific = this->ICreateD3dIB(pool->m_usage, pool->m_size);
            break;

        default:
            break;
    }

    return 1;
}

// VERIFY: 2ee2e8ff auto
// OFFSET: 0x0069FE80
void CGxDeviceD3d::DeviceSetGamma(const CGxGammaRamp& ramp) {
    CGxDevice::DeviceSetGamma(ramp);

    if ((this->m_d3dCaps.Caps2 & D3DCAPS2_FULLSCREENGAMMA) && !this->IDevIsWindowed()) {
        this->m_d3dDevice->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, reinterpret_cast<const D3DGAMMARAMP*>(&this->m_gammaRamp));
    }
}

// VERIFY: 0ec084bf auto
// OFFSET: 0x0069FED0
void* CGxDeviceD3d::DeviceWindow() {
    return this->m_hwnd;
}

// VERIFY: 493ad387 manual
// OFFSET: 0x0069FF40
void CGxDeviceD3d::DeviceOverride(EGxOverride override, uint32_t value) {
    CGxDevice::DeviceOverride(override, value);

    if (override == GxOverride_PixelShader) {
        this->m_caps.m_shaderTargets[GxSh_Pixel] = value;
    } else if (override == GxOverride_Unk7) {
        this->m_caps.m_texNonPow2 = value != 0;
    }
}

// VERIFY: 46e713d8 auto
// OFFSET: 0x0069FF80
void CGxDeviceD3d::PoolSizeSet(CGxPool* pool, uint32_t size) {
    this->IReleaseD3dPool(pool);
    pool->m_size = size;

    switch (pool->m_target) {
        case GxPoolTarget_Vertex:
            pool->m_apiSpecific = this->ICreateD3dVB(pool->m_usage, size);
            break;

        case GxPoolTarget_Index:
            pool->m_apiSpecific = this->ICreateD3dIB(pool->m_usage, size);
            break;

        default:
            break;
    }
}

// VERIFY: 70899344 auto
// OFFSET: 0x006A00C0
void CGxDeviceD3d::ICursorDestroy() {
    CGxDevice::ICursorDestroy();

    if (this->m_hwCursorBitmap) {
        this->m_hwCursorBitmap->Release();
        this->m_hwCursorBitmap = nullptr;
    }

    if (this->m_hwCursorTexture) {
        this->m_hwCursorTexture->Release();
        this->m_hwCursorTexture = nullptr;
    }
}

// VERIFY: ffea8e5a auto
// OFFSET: 0x006A0110
float CGxDeviceD3d::StereoGetConvergence() {
    return this->m_d3dStereoConvergence;
}

// VERIFY: 5cbe2012 auto
// OFFSET: 0x006A0120
float CGxDeviceD3d::StereoGetSeparation() {
    return this->m_d3dStereoSeparation;
}

// VERIFY: b7985cf0 auto
// OFFSET: 0x006A0130
bool CGxDeviceD3d::StereoEnabled() {
    return this->m_d3dStereoEnabled == 1;
}

// VERIFY: 54f891dc auto
// OFFSET: 0x006A0190
void CGxDeviceD3d::QueryDestroy(CGxQuery*& query) {
    CGxQuery* gxQuery = query;

    if (gxQuery->m_apiSpecific) {
        static_cast<LPDIRECT3DQUERY9>(gxQuery->m_apiSpecific)->Release();
    }

    gxQuery->m_apiSpecific = nullptr;
    CGxDevice::QueryDestroy(query);
}

// VERIFY: 57374da8 auto
// OFFSET: 0x006A0240
bool CGxDeviceD3d::QueryEnd(CGxQuery* query) {
    if (!this->m_context || !query->m_apiSpecific) {
        return false;
    }

    HRESULT hr = D3D_OK;

    if (query->m_type == GxQueryType_Occlusion) {
        hr = static_cast<LPDIRECT3DQUERY9>(query->m_apiSpecific)->Issue(D3DISSUE_END);
    }

    return hr == D3D_OK;
}

// VERIFY: 5708f36a manual
// OFFSET: 0x006A0310
bool CGxDeviceD3d::QueryGetData(CGxQuery* query, uint32_t& data) {
    if (!this->m_context || !query->m_apiSpecific) {
        return false;
    }

    LPDIRECT3DQUERY9 d3dQuery = static_cast<LPDIRECT3DQUERY9>(query->m_apiSpecific);
    HRESULT hr;

    do {
        hr = d3dQuery->GetData(&data, d3dQuery->GetDataSize(), D3DGETDATA_FLUSH);
    } while (hr == S_FALSE);

    return hr == D3D_OK;
}

// VERIFY: f57c2b90 auto
// OFFSET: 0x006A3420
void CGxDeviceD3d::ISceneEnd() {
    if (this->m_inScene) {
        this->m_d3dDevice->EndScene();
        this->m_inScene = 0;
    }
}

// VERIFY: d0242deb auto
// OFFSET: 0x006A5A00
void CGxDeviceD3d::CapsWindowSize(CRect& rect) {
    rect = this->DeviceCurWindow();
}

// VERIFY: c58c4596 manual
// OFFSET: 0x006A5E10
void CGxDeviceD3d::IShaderCreate(CGxShader* shader) {
    if (shader->m_target == GxSh_Vertex) {
        this->IShaderCreateVertex(shader);
    } else if (shader->m_target == GxSh_Pixel) {
        this->IShaderCreatePixel(shader);
    }
}

// VERIFY: f717b755 auto
// OFFSET: 0x006A9920
void CGxDeviceD3d::CapsWindowSizeInScreenCoords(CRect& rect) {
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
        rect = this->DeviceCurWindow();
    }
}

// VERIFY: 616797ae manual
// OFFSET: 0x006A9B40
void CGxDeviceD3d::XformSetProjection(const C44Matrix& matrix) {
    CGxDevice::XformSetProjection(matrix);

    C44Matrix projNative = matrix;

    if (!(fabs(projNative.c3 - 1.0f) < 0.00000023841858f) && !(fabs(projNative.c3) < 0.00000023841858f)) {
        projNative *= 1.0f / projNative.c3;
    }

    if (projNative.d3 == 0.0f) {
        float minZ = -(projNative.d2 / (projNative.c2 + 1.0f));
        float maxZ = -(projNative.d2 / (projNative.c2 - 1.0f));
        projNative.c2 = maxZ / (maxZ - minZ);
        projNative.d2 = maxZ * minZ / (minZ - maxZ);
    } else {
        float invC2 = 1.0f / projNative.c2;
        float minZ = (-1.0f - projNative.d2) * invC2;
        float maxZ = (1.0f - projNative.d2) * invC2;
        projNative.c2 = 1.0f / (maxZ - minZ);
        projNative.d2 = minZ / (minZ - maxZ);
    }

    if (!(this->m_appMasterEnables & (1 << GxMasterEnable_NormalProjection)) && projNative.d3 != 1.0f) {
        C44Matrix scale;
        scale.a0 = 0.2f;
        scale.b1 = 0.2f;
        scale.c2 = 0.2f;
        projNative = projNative * scale;
    }

    this->m_xforms[GxXform_Projection].m_dirty = 1;
    this->m_projNative = projNative;
}

// VERIFY: 146386db manual
// OFFSET: 0x006A9E00
void CGxDeviceD3d::XformSetView(const C44Matrix& matrix) {
    CGxDevice::XformSetView(matrix);
}

// VERIFY: 4be0f29d auto
// OFFSET: 0x006AA070
void CGxDeviceD3d::IShaderCreatePixel(CGxShader* shader) {
    shader->m_valid = 0;

    if (!this->m_context) {
        return;
    }

    shader->m_loaded = 1;

    LPDIRECT3DPIXELSHADER9 pixelShader;

    if (shader->m_code.Count() && this->m_d3dDevice->CreatePixelShader(reinterpret_cast<const DWORD*>(shader->m_code.m_data), &pixelShader) == D3D_OK) {
        shader->m_apiSpecific = pixelShader;
        shader->m_valid = 1;
    }
}

// VERIFY: 42b1d4b6 auto
// OFFSET: 0x006AA0D0
void CGxDeviceD3d::IShaderCreateVertex(CGxShader* shader) {
    shader->m_valid = 0;

    if (!this->m_context) {
        return;
    }

    shader->m_loaded = 1;

    LPDIRECT3DVERTEXSHADER9 vertexShader;

    if (shader->m_code.Count() && this->m_d3dDevice->CreateVertexShader(reinterpret_cast<const DWORD*>(shader->m_code.m_data), &vertexShader) == D3D_OK) {
        shader->m_apiSpecific = vertexShader;
        shader->m_valid = 1;
    }
}

// VERIFY: 38fa5626 manual
// OFFSET: 0x006AA130
void CGxDeviceD3d::ShaderCreate(CGxShader** shaders, EGxShTarget target, const char* path, const char* name, uint32_t permutations) {
    CGxDevice::ShaderCreate(shaders, target, path, name, permutations);

    if (permutations == 1 && !shaders[0]->m_loaded) {
        if (target == GxSh_Vertex) {
            this->IShaderCreateVertex(shaders[0]);
        } else if (target == GxSh_Pixel) {
            this->IShaderCreatePixel(shaders[0]);
        }
    }
}

// VERIFY: ade44033 manual
// OFFSET: 0x006AA190
void CGxDeviceD3d::ShaderDestroy(CGxShader*& shader) {
    if (shader->m_target == GxSh_Vertex) {
        if (shader->m_refCount == 1) {
            if (shader->m_apiSpecific) {
                static_cast<LPDIRECT3DVERTEXSHADER9>(shader->m_apiSpecific)->Release();
                shader->m_apiSpecific = nullptr;
            }

            shader->m_loaded = 0;
        }
    } else if (shader->m_target == GxSh_Pixel) {
        if (shader->m_refCount == 1) {
            if (shader->m_apiSpecific) {
                static_cast<LPDIRECT3DPIXELSHADER9>(shader->m_apiSpecific)->Release();
                shader->m_apiSpecific = nullptr;
            }

            shader->m_loaded = 0;
        }
    }

    CGxDevice::ShaderDestroy(shader);
}

void CGxDeviceD3d::DeviceUnk81(uint32_t a, uint32_t b) {
}

void CGxDeviceD3d::DeviceUnk82() {
}

void CGxDeviceD3d::DeviceUnk83(uint32_t a, uint32_t b) {
}

// VERIFY: 3b44d011 manual
// OFFSET: 0x006A3720
bool CGxDeviceD3d::GxVertexDecl::Compare(const D3DVERTEXELEMENT9* elements, uint32_t count) {
    if (count != this->m_count) {
        return false;
    }

    for (uint32_t i = 0; i < this->m_count; i++) {
        if (memcmp(&this->m_elements[i], &elements[i], sizeof(D3DVERTEXELEMENT9))) {
            return false;
        }
    }

    return true;
}

// VERIFY: dc0f168f auto
// OFFSET: 0x006A3810
void CGxDeviceD3d::IStateSyncEnables() {
    if (this->m_appMasterEnables == this->m_hwMasterEnables) {
        return;
    }

    int32_t enable;

    if (this->NeedsUpdate(this->m_appMasterEnables, this->m_hwMasterEnables, 0, 0, GxMasterEnable_PolygonFill, enable)) {
        this->m_d3dDevice->SetRenderState(D3DRS_FILLMODE, enable ? D3DFILL_SOLID : D3DFILL_WIREFRAME);
    }

    this->m_hwMasterEnables = this->m_appMasterEnables;
}

// VERIFY: 35d5837f auto
// OFFSET: 0x006A3870
void CGxDeviceD3d::IStateSyncClipPlanes() {
    if (!this->m_clipPlaneMask) {
        return;
    }

    for (uint32_t i = 0; i < 6; i++) {
        if (this->m_clipPlaneMask & (1 << i)) {
            this->m_d3dDevice->SetClipPlane(i, reinterpret_cast<const float*>(&this->m_clipPlane[i]));
        }
    }

    this->m_clipPlaneMask = 0;
}

// VERIFY: 1e83010d manual
// OFFSET: 0x006A38D0
void CGxDeviceD3d::IStateSyncScissorRect() {
    if (!this->m_scissorRectDirty) {
        return;
    }

    const CRect& window = this->DeviceCurWindow();
    RECT rect;

    rect.left = static_cast<LONG>(this->m_scissorRect.minX * window.maxX + 0.5f);
    rect.right = static_cast<LONG>(this->m_scissorRect.maxX * window.maxX + 1.0f);

    if (this->m_textureTarget[0].m_apiSpecific || this->m_textureTarget[1].m_apiSpecific) {
        rect.top = static_cast<LONG>(this->m_scissorRect.minY * window.maxY + 0.5f);
        rect.bottom = static_cast<LONG>(1.0f + this->m_scissorRect.maxY * window.maxY);
    } else {
        rect.top = static_cast<LONG>((1.0f - this->m_scissorRect.maxY) * window.maxY + 0.5f);
        rect.bottom = static_cast<LONG>(1.0f + (1.0f - this->m_scissorRect.minY) * window.maxY);
    }

    if (rect.left < 0) {
        rect.left = 0;
    }

    if (rect.top < 0) {
        rect.top = 0;
    }

    if (rect.right > window.maxX) {
        rect.right = static_cast<LONG>(window.maxX);
    }

    if (rect.bottom > window.maxY) {
        rect.bottom = static_cast<LONG>(window.maxY);
    }

    this->m_d3dDevice->SetScissorRect(&rect);
    this->m_scissorRectDirty = 0;
}

// VERIFY: 1602f0a7 auto
// OFFSET: 0x006A39E0
void CGxDeviceD3d::ISetVertexBuffer(uint32_t stream, LPDIRECT3DVERTEXBUFFER9 buffer, uint32_t offset, uint32_t stride) {
    if (!this->m_caps.m_streamOffset) {
        offset = 0;
    }

    if (this->m_d3dVertexStreamBuf[stream] != buffer || this->m_d3dVertexStreamOfs[stream] != offset || this->m_d3dVertexStreamStride[stream] != stride) {
        this->m_d3dDevice->SetStreamSource(stream, buffer, offset, stride);
        this->m_d3dVertexStreamBuf[stream] = buffer;
        this->m_d3dVertexStreamOfs[stream] = offset;
        this->m_d3dVertexStreamStride[stream] = stride;
    }
}

// VERIFY: 24c85496 manual
// OFFSET: 0x006A3C40
void CGxDeviceD3d::DsSet(EDeviceState state, uint32_t value) {
    if (this->m_deviceStates[state] == value) {
        return;
    }

    switch (state) {
        case Ds_SrcBlend:
            this->m_d3dDevice->SetRenderState(D3DRS_SRCBLEND, value);
            break;

        case Ds_DstBlend:
            this->m_d3dDevice->SetRenderState(D3DRS_DESTBLEND, value);
            break;

        case Ds_TssMagFilter0:
        case Ds_TssMagFilter1:
        case Ds_TssMagFilter2:
        case Ds_TssMagFilter3:
        case Ds_TssMagFilter4:
        case Ds_TssMagFilter5:
        case Ds_TssMagFilter6:
        case Ds_TssMagFilter7:
        case Ds_TssMagFilter8:
        case Ds_TssMagFilter9:
        case Ds_TssMagFilter10:
        case Ds_TssMagFilter11:
        case Ds_TssMagFilter12:
        case Ds_TssMagFilter13:
        case Ds_TssMagFilter14:
        case Ds_TssMagFilter15:
            this->m_d3dDevice->SetSamplerState(state - Ds_TssMagFilter0, D3DSAMP_MAGFILTER, value);
            break;

        case Ds_TssMinFilter0:
        case Ds_TssMinFilter1:
        case Ds_TssMinFilter2:
        case Ds_TssMinFilter3:
        case Ds_TssMinFilter4:
        case Ds_TssMinFilter5:
        case Ds_TssMinFilter6:
        case Ds_TssMinFilter7:
        case Ds_TssMinFilter8:
        case Ds_TssMinFilter9:
        case Ds_TssMinFilter10:
        case Ds_TssMinFilter11:
        case Ds_TssMinFilter12:
        case Ds_TssMinFilter13:
        case Ds_TssMinFilter14:
        case Ds_TssMinFilter15:
            this->m_d3dDevice->SetSamplerState(state - Ds_TssMinFilter0, D3DSAMP_MINFILTER, value);
            break;

        case Ds_TssMipFilter0:
        case Ds_TssMipFilter1:
        case Ds_TssMipFilter2:
        case Ds_TssMipFilter3:
        case Ds_TssMipFilter4:
        case Ds_TssMipFilter5:
        case Ds_TssMipFilter6:
        case Ds_TssMipFilter7:
        case Ds_TssMipFilter8:
        case Ds_TssMipFilter9:
        case Ds_TssMipFilter10:
        case Ds_TssMipFilter11:
        case Ds_TssMipFilter12:
        case Ds_TssMipFilter13:
        case Ds_TssMipFilter14:
        case Ds_TssMipFilter15:
            this->m_d3dDevice->SetSamplerState(state - Ds_TssMipFilter0, D3DSAMP_MIPFILTER, value);
            break;

        case Ds_TssWrapU0:
        case Ds_TssWrapU1:
        case Ds_TssWrapU2:
        case Ds_TssWrapU3:
        case Ds_TssWrapU4:
        case Ds_TssWrapU5:
        case Ds_TssWrapU6:
        case Ds_TssWrapU7:
        case Ds_TssWrapU8:
        case Ds_TssWrapU9:
        case Ds_TssWrapU10:
        case Ds_TssWrapU11:
        case Ds_TssWrapU12:
        case Ds_TssWrapU13:
        case Ds_TssWrapU14:
        case Ds_TssWrapU15:
            this->m_d3dDevice->SetSamplerState(state - Ds_TssWrapU0, D3DSAMP_ADDRESSU, value);
            break;

        case Ds_TssWrapV0:
        case Ds_TssWrapV1:
        case Ds_TssWrapV2:
        case Ds_TssWrapV3:
        case Ds_TssWrapV4:
        case Ds_TssWrapV5:
        case Ds_TssWrapV6:
        case Ds_TssWrapV7:
        case Ds_TssWrapV8:
        case Ds_TssWrapV9:
        case Ds_TssWrapV10:
        case Ds_TssWrapV11:
        case Ds_TssWrapV12:
        case Ds_TssWrapV13:
        case Ds_TssWrapV14:
        case Ds_TssWrapV15:
            this->m_d3dDevice->SetSamplerState(state - Ds_TssWrapV0, D3DSAMP_ADDRESSV, value);
            break;

        case Ds_TssTexTransformFlags0:
        case Ds_TssTexTransformFlags1:
        case Ds_TssTexTransformFlags2:
        case Ds_TssTexTransformFlags3:
        case Ds_TssTexTransformFlags4:
        case Ds_TssTexTransformFlags5:
        case Ds_TssTexTransformFlags6:
        case Ds_TssTexTransformFlags7:
            this->m_d3dDevice->SetTextureStageState(state - Ds_TssTexTransformFlags0, D3DTSS_TEXTURETRANSFORMFLAGS, value);
            break;

        case Ds_TssMaxAnisotropy0:
        case Ds_TssMaxAnisotropy1:
        case Ds_TssMaxAnisotropy2:
        case Ds_TssMaxAnisotropy3:
        case Ds_TssMaxAnisotropy4:
        case Ds_TssMaxAnisotropy5:
        case Ds_TssMaxAnisotropy6:
        case Ds_TssMaxAnisotropy7:
        case Ds_TssMaxAnisotropy8:
        case Ds_TssMaxAnisotropy9:
        case Ds_TssMaxAnisotropy10:
        case Ds_TssMaxAnisotropy11:
        case Ds_TssMaxAnisotropy12:
        case Ds_TssMaxAnisotropy13:
        case Ds_TssMaxAnisotropy14:
        case Ds_TssMaxAnisotropy15:
            this->m_d3dDevice->SetSamplerState(state - Ds_TssMaxAnisotropy0, D3DSAMP_MAXANISOTROPY, value);
            break;

        case Ds_TssTexCoordIndex0:
        case Ds_TssTexCoordIndex1:
        case Ds_TssTexCoordIndex2:
        case Ds_TssTexCoordIndex3:
        case Ds_TssTexCoordIndex4:
        case Ds_TssTexCoordIndex5:
        case Ds_TssTexCoordIndex6:
        case Ds_TssTexCoordIndex7:
            this->m_d3dDevice->SetTextureStageState(state - Ds_TssTexCoordIndex0, D3DTSS_TEXCOORDINDEX, value);
            break;

        case Ds_TssColorOp0:
        case Ds_TssColorOp1:
        case Ds_TssColorOp2:
        case Ds_TssColorOp3:
        case Ds_TssColorOp4:
        case Ds_TssColorOp5:
        case Ds_TssColorOp6:
        case Ds_TssColorOp7:
            this->m_d3dDevice->SetTextureStageState(state - Ds_TssColorOp0, D3DTSS_COLOROP, value);
            break;

        case Ds_TssAlphaOp0:
        case Ds_TssAlphaOp1:
        case Ds_TssAlphaOp2:
        case Ds_TssAlphaOp3:
        case Ds_TssAlphaOp4:
        case Ds_TssAlphaOp5:
        case Ds_TssAlphaOp6:
        case Ds_TssAlphaOp7:
            this->m_d3dDevice->SetTextureStageState(state - Ds_TssAlphaOp0, D3DTSS_ALPHAOP, value);
            break;

        case Ds_TssColorArg10:
        case Ds_TssColorArg11:
        case Ds_TssColorArg12:
        case Ds_TssColorArg13:
        case Ds_TssColorArg14:
        case Ds_TssColorArg15:
        case Ds_TssColorArg16:
        case Ds_TssColorArg17:
            this->m_d3dDevice->SetTextureStageState(state - Ds_TssColorArg10, D3DTSS_COLORARG1, value);
            break;

        case Ds_TssColorArg20:
        case Ds_TssColorArg21:
        case Ds_TssColorArg22:
        case Ds_TssColorArg23:
        case Ds_TssColorArg24:
        case Ds_TssColorArg25:
        case Ds_TssColorArg26:
        case Ds_TssColorArg27:
            this->m_d3dDevice->SetTextureStageState(state - Ds_TssColorArg20, D3DTSS_COLORARG2, value);
            break;

        case Ds_TssAlphaArg10:
        case Ds_TssAlphaArg11:
        case Ds_TssAlphaArg12:
        case Ds_TssAlphaArg13:
        case Ds_TssAlphaArg14:
        case Ds_TssAlphaArg15:
        case Ds_TssAlphaArg16:
        case Ds_TssAlphaArg17:
            this->m_d3dDevice->SetTextureStageState(state - Ds_TssAlphaArg10, D3DTSS_ALPHAARG1, value);
            break;

        case Ds_TssAlphaArg20:
        case Ds_TssAlphaArg21:
        case Ds_TssAlphaArg22:
        case Ds_TssAlphaArg23:
        case Ds_TssAlphaArg24:
        case Ds_TssAlphaArg25:
        case Ds_TssAlphaArg26:
        case Ds_TssAlphaArg27:
            this->m_d3dDevice->SetTextureStageState(state - Ds_TssAlphaArg20, D3DTSS_ALPHAARG2, value);
            break;

        case Ds_AmbientMaterialSource:
            this->m_d3dDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, value);
            break;

        case Ds_DiffuseMaterialSource:
            this->m_d3dDevice->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, value);
            break;

        case Ds_SpecularMaterialSource:
            this->m_d3dDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, value);
            break;

        case Ds_EmissiveMaterialSource:
            this->m_d3dDevice->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, value);
            break;

        case Ds_Ambient:
            this->m_d3dDevice->SetRenderState(D3DRS_AMBIENT, value);
            break;

        case Ds_AlphaBlendEnable:
            this->m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, value);
            break;

        case Ds_AlphaTestEnable:
            this->m_d3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, value);
            break;

        case Ds_AlphaRef:
            this->m_d3dDevice->SetRenderState(D3DRS_ALPHAREF, value);
            break;

        case Ds_FogEnable:
            this->m_d3dDevice->SetRenderState(D3DRS_FOGENABLE, value);
            break;

        case Ds_ZWriteEnable:
            this->m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, value);
            break;

        case Ds_ColorWriteEnable: {
            uint32_t d3dValue = (value & 0x1) != 0;

            if (value & 0x4) {
                d3dValue |= D3DCOLORWRITEENABLE_GREEN;
            }

            if (value & 0x2) {
                d3dValue |= D3DCOLORWRITEENABLE_BLUE;
            }

            if (value & 0x8) {
                d3dValue |= D3DCOLORWRITEENABLE_ALPHA;
            }

            this->m_d3dDevice->SetRenderState(D3DRS_COLORWRITEENABLE, d3dValue);
            break;
        }

        case Ds_Lighting:
            this->m_d3dDevice->SetRenderState(D3DRS_LIGHTING, value);
            break;

        case Ds_SpecularEnable:
            this->m_d3dDevice->SetRenderState(D3DRS_SPECULARENABLE, value);
            break;

        case Ds_CullMode:
            this->m_d3dDevice->SetRenderState(D3DRS_CULLMODE, value);
            break;

        case Ds_ClipPlaneEnable:
            this->m_d3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, value);
            break;

        case Ds_ZFunc:
            this->m_d3dDevice->SetRenderState(D3DRS_ZFUNC, value);
            break;

        case Ds_PointScaleA:
            this->m_d3dDevice->SetRenderState(D3DRS_POINTSCALE_A, value);
            break;

        case Ds_PointScaleB:
            this->m_d3dDevice->SetRenderState(D3DRS_POINTSCALE_B, value);
            break;

        case Ds_PointScaleC:
            this->m_d3dDevice->SetRenderState(D3DRS_POINTSCALE_C, value);
            break;

        case Ds_PointScaleEnable:
            this->m_d3dDevice->SetRenderState(D3DRS_POINTSCALEENABLE, value);
            break;

        default:
            break;
    }

    this->m_deviceStates[state] = value;
}

// VERIFY: daa41927 auto
// OFFSET: 0x006A4100
void CGxDeviceD3d::ISetTexCoordIndex(uint32_t tmu, int32_t texGen, uint32_t coord) {
    EDeviceState state = static_cast<EDeviceState>(Ds_TssTexCoordIndex0 + tmu);

    switch (texGen) {
        case GxTexGen_Disabled:
            this->DsSet(state, coord);
            break;

        case GxTexGen_ObjectLinear:
        case GxTexGen_EyeLinear:
        case GxTexGen_Unk3:
            this->DsSet(state, tmu | D3DTSS_TCI_CAMERASPACEPOSITION);
            break;

        case GxTexGen_ReflectionMap:
        case GxTexGen_SphereMap:
            this->DsSet(state, tmu | D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
            break;

        case GxTexGen_Normal:
            this->DsSet(state, tmu | D3DTSS_TCI_CAMERASPACENORMAL);
            break;

        default:
            break;
    }
}

// VERIFY: 40ebb823 auto
// OFFSET: 0x006A4190
void CGxDeviceD3d::ISetColorOp(uint32_t tmu, uint32_t op) {
    if (tmu < static_cast<uint32_t>(this->m_caps.m_numTmus)) {
        this->DsSet(static_cast<EDeviceState>(Ds_TssColorOp0 + tmu), s_texOp[op]);
        this->DsSet(static_cast<EDeviceState>(Ds_TssColorArg10 + tmu), s_texArgs[op][0]);
        this->DsSet(static_cast<EDeviceState>(Ds_TssColorArg20 + tmu), s_texArgs[op][1]);
    }
}

// VERIFY: c3e3636b auto
// OFFSET: 0x006A41F0
void CGxDeviceD3d::ISetAlphaOp(uint32_t tmu, uint32_t op) {
    if (tmu < static_cast<uint32_t>(this->m_caps.m_numTmus)) {
        this->DsSet(static_cast<EDeviceState>(Ds_TssAlphaOp0 + tmu), s_texOp[op]);
        this->DsSet(static_cast<EDeviceState>(Ds_TssAlphaArg10 + tmu), s_texArgs[op][0]);
        this->DsSet(static_cast<EDeviceState>(Ds_TssAlphaArg20 + tmu), s_texArgs[op][1]);
    }
}

// VERIFY: c7d0427b auto
// OFFSET: 0x006A4250
void CGxDeviceD3d::ISetMaterial(CImVector diffuse, CImVector emissive, CImVector specular, float specularExp) {
    s_d3dMaterial.Diffuse.r = diffuse.r * (1.0f / 255.0f);
    s_d3dMaterial.Diffuse.g = diffuse.g * (1.0f / 255.0f);
    s_d3dMaterial.Diffuse.b = diffuse.b * (1.0f / 255.0f);
    s_d3dMaterial.Diffuse.a = diffuse.a * (1.0f / 255.0f);
    s_d3dMaterial.Ambient = s_d3dMaterial.Diffuse;
    s_d3dMaterial.Emissive.r = emissive.r * (1.0f / 255.0f);
    s_d3dMaterial.Emissive.g = emissive.g * (1.0f / 255.0f);
    s_d3dMaterial.Emissive.b = emissive.b * (1.0f / 255.0f);
    s_d3dMaterial.Emissive.a = emissive.a * (1.0f / 255.0f);
    s_d3dMaterial.Specular.r = specular.r * (1.0f / 255.0f);
    s_d3dMaterial.Specular.g = specular.g * (1.0f / 255.0f);
    s_d3dMaterial.Specular.b = specular.b * (1.0f / 255.0f);
    s_d3dMaterial.Specular.a = specular.a * (1.0f / 255.0f);
    s_d3dMaterial.Power = specularExp;
    this->m_d3dDevice->SetMaterial(&s_d3dMaterial);
}

// VERIFY: d2ce53e9 manual
// OFFSET: 0x006A43D0
void CGxDeviceD3d::IStateSyncLights() {
    if (this->m_appRenderStates[GxRs_Lighting].m_value.m_data.i[0]) {
        if (this->m_ambientOnlyMode) {
            this->m_ambientOnlyMode = 0;
        }

        this->DsSet(Ds_Lighting, this->m_appMasterEnables & 0x1);
        this->DsSet(Ds_Ambient, 0);

        for (uint32_t i = 0; i < 4; i++) {
            CGxApiLight& light = this->m_lights[i];

            if (!light.m_flags) {
                continue;
            }

            if (light.m_flags & 0x1) {
                if (!light.m_enable) {
                    light.m_flags &= ~0x1;
                    this->m_d3dDevice->LightEnable(i, FALSE);
                    continue;
                }

                this->m_d3dDevice->LightEnable(i, TRUE);
            } else if (!light.m_enable) {
                continue;
            }

            if (light.m_dir.w == 1.0f) {
                s_d3dLight.Type = D3DLIGHT_POINT;
                s_d3dLight.Position.x = light.m_dir.x;
                s_d3dLight.Position.y = light.m_dir.y;
                s_d3dLight.Position.z = light.m_dir.z;
            } else {
                s_d3dLight.Type = D3DLIGHT_DIRECTIONAL;
                s_d3dLight.Direction.x = light.m_dir.x;
                s_d3dLight.Direction.y = light.m_dir.y;
                s_d3dLight.Direction.z = light.m_dir.z;
            }

            s_d3dLight.Diffuse.r = light.m_dirColor.x;
            s_d3dLight.Diffuse.g = light.m_dirColor.y;
            s_d3dLight.Diffuse.b = light.m_dirColor.z;
            s_d3dLight.Diffuse.a = 1.0f;
            s_d3dLight.Ambient.r = light.m_ambColor.x;
            s_d3dLight.Ambient.g = light.m_ambColor.y;
            s_d3dLight.Ambient.b = light.m_ambColor.z;
            s_d3dLight.Ambient.a = 0.0f;
            s_d3dLight.Specular.r = light.m_specColor.x;
            s_d3dLight.Specular.g = light.m_specColor.y;
            s_d3dLight.Specular.b = light.m_specColor.z;
            s_d3dLight.Specular.a = 0.0f;
            s_d3dLight.Range = 10000.0f;
            s_d3dLight.Falloff = 1.0f;
            s_d3dLight.Attenuation0 = light.m_constantAttenuation;
            s_d3dLight.Attenuation1 = light.m_linearAttenuation;
            s_d3dLight.Attenuation2 = light.m_quadraticAttenuation;
            this->m_d3dDevice->SetLight(i, &s_d3dLight);

            light.m_flags &= ~light.m_flags;
        }

        return;
    }

    if ((this->m_primVertexMask & (1 << GxVA_Color0)) || this->m_appRenderStates[GxRs_MatDiffuse].m_value.m_data.u[0] == 0xFFFFFFFF) {
        if (this->m_ambientOnlyMode) {
            this->m_ambientOnlyMode = 0;
        }

        this->DsSet(Ds_Lighting, 0);
        this->DsSet(Ds_Ambient, 0);
    } else {
        if (!this->m_ambientOnlyMode) {
            this->m_ambientOnlyMode = 1;

            for (uint32_t i = 0; i < 4; i++) {
                this->m_d3dDevice->LightEnable(i, FALSE);

                if (this->m_lights[i].m_enable) {
                    this->m_lights[i].m_flags |= 0x1;
                } else {
                    this->m_lights[i].m_flags &= ~0x1;
                }
            }
        }

        this->DsSet(Ds_Lighting, 1);
        this->DsSet(Ds_Ambient, 0xFFFFFFFF);
    }
}

// VERIFY: ccb732ea manual
// OFFSET: 0x006A4700
void CGxDeviceD3d::IStateSyncMaterial() {
    uint32_t emissiveSource;

    if (this->m_primVertexMask & (1 << GxVA_Color0)) {
        int32_t colorMaterial = this->m_appRenderStates[GxRs_ColorMaterial].m_value.m_data.i[0];
        this->DsSet(Ds_AmbientMaterialSource, colorMaterial == 0);
        this->DsSet(Ds_DiffuseMaterialSource, colorMaterial == 0);
        this->DsSet(Ds_SpecularMaterialSource, colorMaterial == 1);
        emissiveSource = colorMaterial == 2;
    } else {
        emissiveSource = 0;
        this->DsSet(Ds_AmbientMaterialSource, 0);
        this->DsSet(Ds_DiffuseMaterialSource, 0);
        this->DsSet(Ds_SpecularMaterialSource, 0);
    }

    this->DsSet(Ds_EmissiveMaterialSource, emissiveSource);
}

// VERIFY: 95b0bda6 auto
// OFFSET: 0x006A4850
void CGxDeviceD3d::IStateSyncXforms() {
    if (this->m_xforms[GxXform_Projection].m_dirty) {
        this->m_d3dDevice->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&this->m_projNative));
        this->m_xforms[GxXform_Projection].m_dirty = 0;
    }

    if (this->m_xforms[GxXform_View].m_dirty) {
        CGxMatrixStack& view = this->m_xforms[GxXform_View];
        this->m_d3dDevice->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view.m_mtx[view.m_level]));
        view.m_dirty = 0;
    }

    if (this->m_xforms[GxXform_World].m_dirty) {
        this->IXformSetWorld();
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(this->m_caps.m_numTmus); i++) {
        if (this->m_xforms[i].m_dirty || this->m_texGen[i].m_dirty) {
            this->IXformSetTex(i);
        }
    }
}

// VERIFY: 8e1dd0ae manual
// OFFSET: 0x006A4900
void CGxDeviceD3d::ISetTexture(uint32_t tmu, CGxTex* texId) {
    if (tmu >= 16) {
        return;
    }

    if (texId) {
        this->ITexMarkAsUpdated(texId);
        this->m_d3dDevice->SetTexture(tmu, static_cast<IDirect3DBaseTexture9*>(texId->m_apiSpecificData));

        const D3DTEXTUREFILTERTYPE* filter = s_filterModes[texId->m_flags.m_filter];
        this->DsSet(static_cast<EDeviceState>(Ds_TssMinFilter0 + tmu), filter[0]);
        this->DsSet(static_cast<EDeviceState>(Ds_TssMagFilter0 + tmu), filter[1]);
        this->DsSet(static_cast<EDeviceState>(Ds_TssMipFilter0 + tmu), filter[2]);
        this->DsSet(static_cast<EDeviceState>(Ds_TssWrapU0 + tmu), s_wrapModes[texId->m_flags.m_wrapU]);
        this->DsSet(static_cast<EDeviceState>(Ds_TssWrapV0 + tmu), s_wrapModes[texId->m_flags.m_wrapV]);
        this->DsSet(static_cast<EDeviceState>(Ds_TssMaxAnisotropy0 + tmu), texId->m_flags.m_maxAnisotropy);

        if (tmu < 8) {
            if (!this->m_appRenderStates[GxRs_PixelShader].m_value.m_data.i[0] && !this->m_texEnabled[tmu]) {
                this->ISetColorOp(tmu, this->m_appRenderStates[GxRs_ColorOp0 + tmu].m_value.m_data.i[0]);
                this->ISetAlphaOp(tmu, this->m_appRenderStates[GxRs_AlphaOp0 + tmu].m_value.m_data.i[0]);
                this->m_appRenderStates[GxRs_ColorOp0 + tmu].m_dirty = 0;
                this->m_appRenderStates[GxRs_AlphaOp0 + tmu].m_dirty = 0;
            }

            this->m_texEnabled[tmu] = true;
        }
    } else {
        this->m_d3dDevice->SetTexture(tmu, nullptr);

        if (tmu < 8) {
            if (!this->m_appRenderStates[GxRs_PixelShader].m_value.m_data.i[0] && this->m_texEnabled[tmu]) {
                this->DsSet(static_cast<EDeviceState>(Ds_TssColorOp0 + tmu), D3DTOP_DISABLE);
                this->DsSet(static_cast<EDeviceState>(Ds_TssAlphaOp0 + tmu), D3DTOP_DISABLE);
            }

            this->m_texEnabled[tmu] = false;
        }
    }
}

// VERIFY: 79183c1d manual
// OFFSET: 0x006A4AC0
void CGxDeviceD3d::ISetTexCoord(uint32_t tmu, uint32_t coord) {
    if (tmu < static_cast<uint32_t>(this->m_caps.m_numTmus)) {
        this->ISetTexCoordIndex(tmu, this->m_appRenderStates[GxRs_TexGen0 + tmu].m_value.m_data.i[0], coord);
    }
}

// VERIFY: 3b2376aa manual
// OFFSET: 0x006A4AF0
void CGxDeviceD3d::ISetTexGen(uint32_t tmu, int32_t texGen) {
    if (tmu >= static_cast<uint32_t>(this->m_caps.m_numTmus)) {
        return;
    }

    this->ISetTexCoordIndex(tmu, texGen, this->m_appRenderStates[GxRs_TextureCoord0 + tmu].m_value.m_data.i[0]);

    C44Matrix matrix;

    if (texGen == GxTexGen_ObjectLinear || texGen == GxTexGen_EyeLinear) {
        CGxMatrixStack& view = this->m_xforms[GxXform_View];
        matrix = view.m_mtx[view.m_level];
        matrix = matrix.AffineInverse();

        if (texGen == GxTexGen_ObjectLinear) {
            CGxMatrixStack& world = this->m_xforms[GxXform_World];
            matrix = matrix * world.m_mtx[world.m_level].Inverse();
        }
    } else if (texGen == GxTexGen_SphereMap) {
        matrix.a0 = 0.5f;
        matrix.b1 = 0.5f;
        matrix.d0 = 0.5f;
        matrix.d1 = 0.5f;
    } else {
        this->m_texGen[tmu].Identity();
        return;
    }

    CGxMatrixStack& stack = this->m_texGen[tmu];
    stack.m_dirty = 1;
    stack.m_flags[stack.m_level] &= ~CGxMatrixStack::F_Identity;
    stack.m_mtx[stack.m_level] = matrix;
}

// VERIFY: 1b818712 manual
// OFFSET: 0x006A4C30
void CGxDeviceD3d::IRsSendToHw(EGxRenderState which) {
    CGxStateBom& value = this->m_appRenderStates[which].m_value;

    switch (which) {
        case GxRs_PolygonOffset:
            if (this->m_caps.m_depthBias) {
                float bias = -value.m_data.f[0];
                this->m_d3dDevice->SetRenderState(D3DRS_DEPTHBIAS, *reinterpret_cast<DWORD*>(&bias));
            }

            break;

        case GxRs_MatDiffuse:
        case GxRs_MatEmissive:
        case GxRs_MatSpecular:
        case GxRs_MatSpecularExp: {
            CImVector diffuse;
            CImVector emissive;
            CImVector specular;
            *reinterpret_cast<uint32_t*>(&diffuse) = this->m_appRenderStates[GxRs_MatDiffuse].m_value.m_data.u[0];
            *reinterpret_cast<uint32_t*>(&emissive) = this->m_appRenderStates[GxRs_MatEmissive].m_value.m_data.u[0];
            *reinterpret_cast<uint32_t*>(&specular) = this->m_appRenderStates[GxRs_MatSpecular].m_value.m_data.u[0];
            this->ISetMaterial(diffuse, emissive, specular, this->m_appRenderStates[GxRs_MatSpecularExp].m_value.m_data.f[0]);

            this->m_appRenderStates[GxRs_MatDiffuse].m_dirty = 0;
            this->m_appRenderStates[GxRs_MatEmissive].m_dirty = 0;
            this->m_appRenderStates[GxRs_MatSpecular].m_dirty = 0;
            this->m_appRenderStates[GxRs_MatSpecularExp].m_dirty = 0;

            uint32_t specularEnable = 0;

            if (this->m_appRenderStates[GxRs_MatSpecularExp].m_value.m_data.f[0] > 0.0f && this->m_appRenderStates[GxRs_MatSpecular].m_value.m_data.i[0]) {
                specularEnable = 1;
            }

            this->DsSet(Ds_SpecularEnable, specularEnable);
            break;
        }

        case GxRs_NormalizeNormals:
            if (s_normalizeNormals != value.m_data.u[0]) {
                this->m_d3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, value.m_data.u[0]);
                s_normalizeNormals = value.m_data.u[0];
            }

            break;

        case GxRs_BlendingMode: {
            uint32_t mode = value.m_data.u[0];

            if (mode < 2) {
                this->DsSet(Ds_AlphaBlendEnable, 0);
            } else {
                this->DsSet(Ds_AlphaBlendEnable, 1);
                this->DsSet(Ds_SrcBlend, s_srcBlend[mode]);
                this->DsSet(Ds_DstBlend, s_dstBlend[mode]);
            }

            break;
        }

        case GxRs_AlphaRef: {
            int32_t alphaRef = value.m_data.i[0];

            if (alphaRef > 0) {
                this->DsSet(Ds_AlphaRef, alphaRef);
                this->DsSet(Ds_AlphaTestEnable, 1);
            } else {
                this->DsSet(Ds_AlphaTestEnable, 0);
            }

            break;
        }

        case GxRs_FogStart:
            this->m_d3dDevice->SetRenderState(D3DRS_FOGSTART, value.m_data.u[0]);
            break;

        case GxRs_FogEnd:
            this->m_d3dDevice->SetRenderState(D3DRS_FOGEND, value.m_data.u[0]);
            break;

        case GxRs_FogColor:
            this->m_d3dDevice->SetRenderState(D3DRS_FOGCOLOR, value.m_data.u[0]);
            break;

        case GxRs_Lighting: {
            uint32_t enable = 0;

            if (this->m_appMasterEnables & 0x1) {
                enable = value.m_data.i[0] != 0;
            }

            this->DsSet(Ds_Lighting, enable);
            break;
        }

        case GxRs_Fog: {
            uint32_t enable = 0;

            if (this->m_appMasterEnables & 0x2) {
                enable = value.m_data.i[0] != 0;
            }

            this->DsSet(Ds_FogEnable, enable);
            break;
        }

        case GxRs_DepthTest:
        case GxRs_DepthFunc: {
            uint32_t func;

            if ((this->m_appMasterEnables & 0x4) && this->m_appRenderStates[GxRs_DepthTest].m_value.m_data.i[0]) {
                func = s_cmpFunc[this->m_appRenderStates[GxRs_DepthFunc].m_value.m_data.i[0]];
            } else {
                func = D3DCMP_ALWAYS;
            }

            this->DsSet(Ds_ZFunc, func);
            this->m_appRenderStates[GxRs_DepthTest].m_dirty = 0;
            this->m_appRenderStates[GxRs_DepthFunc].m_dirty = 0;
            break;
        }

        case GxRs_DepthWrite: {
            uint32_t enable = 0;

            if (this->m_appMasterEnables & 0x8) {
                enable = value.m_data.i[0] != 0;
            }

            this->DsSet(Ds_ZWriteEnable, enable);
            break;
        }

        case GxRs_ColorWrite: {
            uint32_t mask = 0;

            if (this->m_appMasterEnables & 0x10) {
                mask = value.m_data.u[0];
            }

            this->DsSet(Ds_ColorWriteEnable, mask);
            break;
        }

        case GxRs_Culling: {
            uint32_t cull = 0;

            if (this->m_appMasterEnables & 0x20) {
                cull = value.m_data.i[0];

                if (value.m_data.i[0] > 2) {
                    cull = 2;
                }
            }

            this->DsSet(Ds_CullMode, s_cullMode[cull]);
            break;
        }

        case GxRs_ClipPlaneMask:
            this->DsSet(Ds_ClipPlaneEnable, value.m_data.u[0]);
            break;

        case GxRs_Multisample:
            this->m_d3dDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, value.m_data.i[0] != 0);
            break;

        case GxRs_ScissorTest:
            this->m_d3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, value.m_data.i[0] != 0);
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

        case GxRs_TextureCoord0:
        case GxRs_TextureCoord1:
        case GxRs_TextureCoord2:
        case GxRs_TextureCoord3:
        case GxRs_TextureCoord4:
        case GxRs_TextureCoord5:
        case GxRs_TextureCoord6:
        case GxRs_TextureCoord7:
            this->ISetTexCoord(which - GxRs_TextureCoord0, value.m_data.u[0]);
            break;

        case GxRs_VertexShader:
            this->IBindVertexShader(static_cast<CGxShader*>(value.m_data.p));
            break;

        case GxRs_PixelShader:
            this->IBindPixelShader(static_cast<CGxShader*>(value.m_data.p));
            break;

        case GxRs_PointScale: {
            float pointScale = value.m_data.f[0];
            const C3Vector& attenuation = *reinterpret_cast<const C3Vector*>(this->m_appRenderStates[GxRs_PointScaleAttenuation].m_value.m_data.f);

            if (attenuation != CGxDevice::s_pointScaleAttenuation) {
                const CRect& window = this->DeviceCurWindow();
                pointScale = pointScale / ((1.0f - this->m_viewport.m_y.m_l) * window.maxY - (1.0f - this->m_viewport.m_y.m_h) * window.maxY);
            }

            this->m_d3dDevice->SetRenderState(D3DRS_POINTSIZE, *reinterpret_cast<DWORD*>(&pointScale));
            break;
        }

        case GxRs_PointScaleAttenuation: {
            const C3Vector& attenuation = *reinterpret_cast<const C3Vector*>(value.m_data.f);
            uint32_t enable = attenuation != CGxDevice::s_pointScaleAttenuation;

            this->DsSet(Ds_PointScaleEnable, enable);

            if (enable) {
                this->DsSet(Ds_PointScaleA, value.m_data.u[0]);
                this->DsSet(Ds_PointScaleB, value.m_data.u[1]);
                this->DsSet(Ds_PointScaleC, value.m_data.u[2]);
            }

            break;
        }

        case GxRs_PointScaleMin:
            this->m_d3dDevice->SetRenderState(D3DRS_POINTSIZE_MIN, value.m_data.u[0]);
            break;

        case GxRs_PointScaleMax:
            this->m_d3dDevice->SetRenderState(D3DRS_POINTSIZE_MAX, value.m_data.u[0]);
            break;

        case GxRs_PointSprite:
            this->m_d3dDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, value.m_data.i[0] != 0);
            break;

        case Unk84: {
            uint32_t factor = static_cast<uint32_t>(value.m_data.f[0] * 255.0f);
            this->m_d3dDevice->SetRenderState(D3DRS_BLENDFACTOR, (((((factor << 8) | factor) << 8) | factor) << 8) | factor);
            break;
        }

        default:
            break;
    }
}

// VERIFY: f78906fa manual
// OFFSET: 0x006A5540
LPDIRECT3DVERTEXDECLARATION9 CGxDeviceD3d::ICreateD3dVertexDecl(const D3DVERTEXELEMENT9* elements, uint32_t count) {
    if (this->m_primVertexFormat < GxVertexBufferFormats_Last) {
        if (!this->m_d3dVertexDecl[this->m_primVertexFormat]) {
            this->m_d3dDevice->CreateVertexDeclaration(elements, &this->m_d3dVertexDecl[this->m_primVertexFormat]);
        }

        return this->m_d3dVertexDecl[this->m_primVertexFormat];
    }

    for (uint32_t i = 0; i < this->m_gxVertexDecl.Count(); i++) {
        if (this->m_gxVertexDecl[i].Compare(elements, count)) {
            return this->m_gxVertexDecl[i].m_d3dDecl;
        }
    }

    GxVertexDecl* decl = this->m_gxVertexDecl.New();
    decl->m_count = count;
    memcpy(decl->m_elements, elements, sizeof(D3DVERTEXELEMENT9) * count);
    this->m_d3dDevice->CreateVertexDeclaration(elements, &decl->m_d3dDecl);

    return decl->m_d3dDecl;
}

// VERIFY: d3a984ea manual
// OFFSET: 0x006A5700
void CGxDeviceD3d::IStateSyncVertexPtrs() {
    if (this->m_primVertexFormat < GxVertexBufferFormats_Last && this->m_d3dVertexDecl[this->m_primVertexFormat]) {
        LPDIRECT3DVERTEXDECLARATION9 decl = this->m_d3dVertexDecl[this->m_primVertexFormat];
        CGxBuf* buf = this->m_primVertexBuf;

        if (this->m_d3dCurrentVertexDecl != decl) {
            this->m_d3dDevice->SetVertexDeclaration(decl);
            this->m_d3dCurrentVertexDecl = decl;
        }

        this->ISetVertexBuffer(0, static_cast<LPDIRECT3DVERTEXBUFFER9>(buf->m_pool->m_apiSpecific), buf->m_index, this->m_primVertexSize);

        return;
    }

    CGxBuf** streamBufs = static_cast<CGxBuf**>(_alloca(sizeof(CGxBuf*) * GxVAs_Last));
    uint32_t* streamStrides = static_cast<uint32_t*>(_alloca(sizeof(uint32_t) * GxVAs_Last));
    uint32_t streamCount = 0;

    memset(streamBufs, 0, sizeof(CGxBuf*) * GxVAs_Last);
    memset(streamStrides, 0, sizeof(uint32_t) * GxVAs_Last);

    D3DVERTEXELEMENT9 elements[GxVAs_Last + 1];
    uint32_t elementCount = 0;

    for (uint32_t i = 0; i < GxVAs_Last; i++) {
        if (!(this->m_primVertexMask & (1 << i))) {
            continue;
        }

        uint32_t stream;

        for (stream = 0; stream < streamCount; stream++) {
            if (streamBufs[stream] == this->m_primVertexFormatBuf[i]) {
                break;
            }
        }

        if (stream == streamCount) {
            streamBufs[streamCount++] = this->m_primVertexFormatBuf[i];
        }

        const CGxVertexAttrib& attrib = this->m_primVertexFormatAttrib[i];
        streamStrides[stream] += s_gxAttribToD3dAttribSize[attrib.m_type];

        D3DVERTEXELEMENT9& element = elements[elementCount++];
        element.Stream = static_cast<WORD>(stream);
        element.Offset = static_cast<WORD>(attrib.m_offset);
        element.Type = static_cast<BYTE>(s_gxAttribToD3dAttribType[attrib.m_type]);
        element.Method = D3DDECLMETHOD_DEFAULT;
        element.Usage = static_cast<BYTE>(s_gxAttribToD3dAttribUsage[attrib.m_attrib]);
        element.UsageIndex = static_cast<BYTE>(s_gxAttribToD3dAttribUsageIndex[attrib.m_attrib]);
    }

    D3DVERTEXELEMENT9 end = D3DDECL_END();
    elements[elementCount] = end;

    LPDIRECT3DVERTEXDECLARATION9 decl = this->ICreateD3dVertexDecl(elements, elementCount + 1);

    if (this->m_d3dCurrentVertexDecl != decl) {
        this->m_d3dDevice->SetVertexDeclaration(decl);
        this->m_d3dCurrentVertexDecl = decl;
    }

    for (uint32_t stream = 0; stream < streamCount; stream++) {
        CGxBuf* buf = streamBufs[stream];
        this->ISetVertexBuffer(stream, static_cast<LPDIRECT3DVERTEXBUFFER9>(buf->m_pool->m_apiSpecific), buf->m_index, streamStrides[stream]);
    }
}

// VERIFY: 357545fe manual
// OFFSET: 0x006A5940
void CGxDeviceD3d::IStateSync() {
    int32_t hwVertexShader = this->m_hwRenderStates[GxRs_VertexShader].m_data.i[0];

    this->IShaderConstantsFlush();
    this->IRsSync(0);

    if (!hwVertexShader || !this->m_appRenderStates[GxRs_VertexShader].m_value.m_data.i[0]) {
        this->IStateSyncLights();
        this->IStateSyncMaterial();
        this->IStateSyncXforms();
    }

    this->IStateSyncEnables();
    this->IStateSyncClipPlanes();
    this->IStateSyncScissorRect();
    this->IStateSyncVertexPtrs();

    if (this->m_primIndexDirty) {
        this->m_primIndexDirty = 0;

        LPDIRECT3DINDEXBUFFER9 indexBuffer = static_cast<LPDIRECT3DINDEXBUFFER9>(this->m_primIndexBuf->m_pool->m_apiSpecific);

        if (this->m_d3dCurrentIndexBuf != indexBuffer) {
            this->m_d3dDevice->SetIndices(indexBuffer);
            this->m_d3dCurrentIndexBuf = indexBuffer;
        }
    }

    if (this->m_viewportDirty) {
        this->IXformSetViewport();
    }
}

// VERIFY: 23534957 auto
// OFFSET: 0x006A5A30
void CGxDeviceD3d::IXformSetWorld() {
    CGxMatrixStack& world = this->m_xforms[GxXform_World];

    if (!s_isIdent || !(world.m_flags[world.m_level] & CGxMatrixStack::F_Identity)) {
        this->m_d3dDevice->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world.m_mtx[world.m_level]));
    }

    int32_t isIdent = world.m_flags[world.m_level] & CGxMatrixStack::F_Identity;
    world.m_dirty = 0;
    s_isIdent = isIdent;
}

// VERIFY: c3372d44 manual
// OFFSET: 0x006A5AA0
void CGxDeviceD3d::IXformSetTex(uint32_t tmu) {
    uint32_t flags = D3DTTFF_DISABLE;
    D3DTRANSFORMSTATETYPE state = static_cast<D3DTRANSFORMSTATETYPE>(D3DTS_TEXTURE0 + tmu);
    CGxMatrixStack& texGen = this->m_texGen[tmu];
    CGxMatrixStack& xform = this->m_xforms[tmu];

    switch (g_theGxDevicePtr->m_appRenderStates[GxRs_TextureShader0 + tmu].m_value.m_data.i[0]) {
        case 0:
            if (texGen.m_flags[texGen.m_level] & CGxMatrixStack::F_Identity) {
                this->m_d3dDevice->SetTransform(state, reinterpret_cast<const D3DMATRIX*>(&texGen.m_mtx[texGen.m_level]));
            } else {
                this->m_d3dDevice->SetTransform(state, reinterpret_cast<const D3DMATRIX*>(&texGen.m_mtx[texGen.m_level]));
                flags = D3DTTFF_COUNT3;
            }

            break;

        case 1: {
            C44Matrix product = texGen.m_mtx[texGen.m_level] * xform.m_mtx[xform.m_level];
            C44Matrix matrix = product;

            if (this->m_appRenderStates[GxRs_TexGen0 + tmu].m_value.m_data.i[0]) {
                flags = D3DTTFF_COUNT3;
            } else {
                flags = D3DTTFF_COUNT2;
                matrix.c0 = product.d0;
                matrix.c1 = product.d1;
            }

            this->m_d3dDevice->SetTransform(state, reinterpret_cast<const D3DMATRIX*>(&matrix));
            break;
        }

        case 2: {
            C44Matrix matrix = texGen.m_mtx[texGen.m_level] * xform.m_mtx[xform.m_level];
            this->m_d3dDevice->SetTransform(state, reinterpret_cast<const D3DMATRIX*>(&matrix));
            flags = D3DTTFF_COUNT3 | D3DTTFF_PROJECTED;
            break;
        }

        default:
            break;
    }

    this->DsSet(static_cast<EDeviceState>(Ds_TssTexTransformFlags0 + tmu), flags);
    xform.m_dirty = 0;
    texGen.m_dirty = 0;
}

// VERIFY: e6b411da manual
// OFFSET: 0x006A5C70
void CGxDeviceD3d::IBindPixelShader(CGxShader* shader) {
    if (shader) {
        if (!shader->m_loaded) {
            this->IShaderCreatePixel(shader);
        }

        this->m_d3dDevice->SetPixelShader(static_cast<LPDIRECT3DPIXELSHADER9>(shader->m_apiSpecific));
        return;
    }

    this->m_d3dDevice->SetPixelShader(nullptr);

    for (uint32_t i = 0; i < 8; i++) {
        if (this->m_texEnabled[i]) {
            this->ISetColorOp(i, this->m_appRenderStates[GxRs_ColorOp0 + i].m_value.m_data.u[0]);
            this->ISetAlphaOp(i, this->m_appRenderStates[GxRs_AlphaOp0 + i].m_value.m_data.u[0]);
        } else {
            this->DsSet(static_cast<EDeviceState>(Ds_TssColorOp0 + i), D3DTOP_DISABLE);
            this->DsSet(static_cast<EDeviceState>(Ds_TssAlphaOp0 + i), D3DTOP_DISABLE);
        }

        this->m_appRenderStates[GxRs_ColorOp0 + i].m_dirty = 0;
        this->m_appRenderStates[GxRs_AlphaOp0 + i].m_dirty = 0;
    }
}

// VERIFY: 355e5938 manual
// OFFSET: 0x006A99E0
void CGxDeviceD3d::IXformSetViewport() {
    const CRect& window = this->DeviceCurWindow();
    D3DVIEWPORT9 viewport;

    viewport.X = static_cast<DWORD>(this->m_viewport.m_x.m_l * window.maxX + 0.5f);
    viewport.Y = static_cast<DWORD>((1.0f - this->m_viewport.m_y.m_h) * window.maxY + 0.5f);
    viewport.Width = static_cast<DWORD>(this->m_viewport.m_x.m_h * window.maxX - viewport.X + 0.5f);
    viewport.Height = static_cast<DWORD>((1.0f - this->m_viewport.m_y.m_l) * window.maxY - viewport.Y + 0.5f);
    viewport.MinZ = this->m_viewport.m_z.m_l;
    viewport.MaxZ = this->m_viewport.m_z.m_h;

    if (this->m_textureTarget[0].m_apiSpecific || this->m_textureTarget[1].m_apiSpecific) {
        viewport.Y = static_cast<DWORD>(window.maxY * this->m_viewport.m_y.m_l);
    }

    this->m_d3dDevice->SetViewport(&viewport);
    this->m_viewportDirty = 0;
}

// VERIFY: 2f91f397 manual
// OFFSET: 0x006A9FE0
void CGxDeviceD3d::IShaderConstantsFlush() {
    ShadowConstants& vs = CGxDevice::s_shadowConstants[1];

    if (vs.dirtyMin <= vs.dirtyMax) {
        this->m_d3dDevice->SetVertexShaderConstantF(vs.dirtyMin, &vs.constants[vs.dirtyMin * 4], vs.dirtyMax - vs.dirtyMin + 1);
    }

    vs.dirtyMin = 255;
    vs.dirtyMax = 0;

    ShadowConstants& ps = CGxDevice::s_shadowConstants[0];

    if (ps.dirtyMin <= ps.dirtyMax) {
        this->m_d3dDevice->SetPixelShaderConstantF(ps.dirtyMin, &ps.constants[ps.dirtyMin * 4], ps.dirtyMax - ps.dirtyMin + 1);
    }

    ps.dirtyMax = 0;
    ps.dirtyMin = 255;
}

// VERIFY: 8d590c92 manual
// OFFSET: 0x006AA2F0
void CGxDeviceD3d::IBindVertexShader(CGxShader* shader) {
    if (shader) {
        if (!shader->m_loaded) {
            this->IShaderCreateVertex(shader);
        }

        this->m_d3dDevice->SetVertexShader(static_cast<LPDIRECT3DVERTEXSHADER9>(shader->m_apiSpecific));
    } else {
        this->m_d3dDevice->SetVertexShader(nullptr);
    }
}

// VERIFY: 84e2843a manual
// OFFSET: 0x0068E250
void CGxDeviceD3d::ISetPresentParms(D3DPRESENT_PARAMETERS& d3dpp, const CGxFormat& format) {
    memset(&d3dpp, 0, sizeof(d3dpp));

    if (format.m_window) {
        D3DDISPLAYMODE displayMode;

        if (FAILED(this->m_d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &displayMode))) {
            displayMode.Format = this->m_desktopDisplayMode.Format;
        }

        const CRect& window = this->DeviceCurWindow();
        d3dpp.Windowed = TRUE;
        d3dpp.BackBufferWidth = static_cast<UINT>(window.maxX);
        d3dpp.BackBufferFormat = displayMode.Format;
        d3dpp.BackBufferHeight = static_cast<UINT>(window.maxY);
        d3dpp.BackBufferCount = format.m_vsync ? format.m_backBufferCount : 1;
        d3dpp.FullScreen_RefreshRateInHz = 0;
    } else {
        d3dpp.BackBufferWidth = format.m_size.x;
        d3dpp.BackBufferHeight = format.m_size.y;
        d3dpp.BackBufferFormat = s_GxFormatToD3dFormat[format.m_colorFormat];
        d3dpp.BackBufferCount = format.m_vsync ? format.m_backBufferCount : 1;
        d3dpp.FullScreen_RefreshRateInHz = format.m_refreshRate;
    }

    d3dpp.hDeviceWindow = this->m_hwnd;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = s_GxFormatToD3dFormat[format.m_depthFormat];

    switch (format.m_vsync) {
        case 1:
            d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
            break;

        case 2:
            d3dpp.PresentationInterval = format.m_window ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_TWO;
            break;

        case 3:
            d3dpp.PresentationInterval = format.m_window ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_THREE;
            break;

        case 4:
            d3dpp.PresentationInterval = format.m_window ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_FOUR;
            break;

        default:
            d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
            break;
    }

    if (format.m_multisampleCount <= 1) {
        d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
        return;
    }

    d3dpp.MultiSampleType = static_cast<D3DMULTISAMPLE_TYPE>(format.m_multisampleCount);

    DWORD qualityLevels;

    if (FAILED(this->m_d3d->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.BackBufferFormat, format.m_window, d3dpp.MultiSampleType, &qualityLevels)) || qualityLevels <= 1) {
        qualityLevels = 1;
    }

    d3dpp.MultiSampleQuality = static_cast<DWORD>((qualityLevels - 1) * format.m_multisampleQuality);
}

// VERIFY: 284cb5b4 manual
// OFFSET: 0x0068EB20
static ATOM WindowClassCreate() {
    HINSTANCE instance = GetModuleHandleA(nullptr);

    WNDCLASSEXA windowClass;
    memset(&windowClass, 0, sizeof(windowClass));
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_OWNDC;
    windowClass.lpfnWndProc = CGxDeviceD3d::WindowProcD3d;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "GxWindowClassD3d";
    windowClass.hIcon = static_cast<HICON>(LoadImageA(instance, "BlizzardIcon.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    windowClass.hCursor = LoadCursorA(instance, "BlizzardCursor.cur");

    if (!windowClass.hCursor) {
        windowClass.hCursor = LoadCursorA(instance, IDC_ARROW);
    }

    return RegisterClassExA(&windowClass);
}

// VERIFY: 9db942aa manual
// OFFSET: 0x0068EBB0
int32_t CGxDeviceD3d::ICreateWindow(CGxFormat& format) {
    HINSTANCE instance = GetModuleHandleA(nullptr);
    DWORD style;

    if (!format.m_window) {
        style = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU;
    } else if (format.m_maximize == 1) {
        style = WS_POPUP | WS_VISIBLE;
    } else if (format.m_maximize == 2) {
        style = WS_POPUP;
    } else {
        style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    }

    float aspectRatio = 0.0f;

    if (format.m_aspect) {
        int32_t width = format.m_size.x;

        if (width) {
            int32_t height = format.m_size.y;

            if (height) {
                aspectRatio = static_cast<float>(width) / height;
            }
        }
    }

    RECT clientRect;
    clientRect.right = format.m_size.x;
    clientRect.left = 0;
    clientRect.top = 0;
    clientRect.bottom = format.m_size.y;
    AdjustWindowRectEx(&clientRect, style, FALSE, 0);

    CGxDevice::s_clientAdjust.x = clientRect.right - format.m_size.x - clientRect.left;
    CGxDevice::s_clientAdjust.y = clientRect.bottom - format.m_size.y - clientRect.top;

    if (format.m_window) {
        if (format.m_maximize == 1) {
            format.m_pos.x = 0;
            format.m_pos.y = 0;
            format.m_size.x = GetSystemMetrics(SM_CXSCREEN);
            format.m_size.y = GetSystemMetrics(SM_CYSCREEN);
        } else {
            RECT windowRect;
            windowRect.left = 0;
            windowRect.top = 0;
            windowRect.right = format.m_size.x;
            windowRect.bottom = format.m_size.y;

            if (CGxDevice::AdjustWindowRectToWorkArea(aspectRatio, &CGxDevice::s_clientAdjust, &windowRect)) {
                format.m_pos.x = windowRect.left;
                format.m_pos.y = windowRect.top;
                format.m_size.x = windowRect.right - windowRect.left;
                format.m_size.y = windowRect.bottom - windowRect.top;
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

    this->m_hwnd = CreateWindowExA(WS_EX_APPWINDOW, "GxWindowClassD3d", "World of Warcraft", style, format.m_pos.x, format.m_pos.y, width, height, nullptr, nullptr, instance, this);

    if (this->m_hwnd && format.m_maximize != 2) {
        ShowWindow(this->m_hwnd, SW_SHOWNORMAL);
    }

    return this->m_hwnd != nullptr;
}

// VERIFY: d94c29eb manual
// OFFSET: 0x0068EE20
void CGxDeviceD3d::ISetCaps(const CGxFormat& format) {
    this->m_caps.m_numTmus = this->m_d3dCaps.MaxSimultaneousTextures >= 8 ? 8 : this->m_d3dCaps.MaxSimultaneousTextures;
    this->m_caps.m_pixelCenterOnEdge = 0;
    this->m_caps.m_texelCenterOnEdge = 1;

    uint32_t maxTextureSize = this->m_d3dCaps.MaxTextureWidth;
    this->m_caps.m_texMaxSize[GxTex_2d] = maxTextureSize >= 256 ? maxTextureSize : 256;
    this->m_caps.m_texMaxSize[GxTex_CubeMap] = maxTextureSize >= 256 ? maxTextureSize : 256;
    this->m_caps.m_texMaxSize[GxTex_Rectangle] = maxTextureSize >= 256 ? maxTextureSize : 256;
    this->m_caps.m_texMaxSize[GxTex_NonPow2] = maxTextureSize >= 256 ? maxTextureSize : 256;
    this->m_caps.m_maxIndex = this->m_d3dCaps.MaxVertexIndex;

    uint32_t filterCaps = this->m_d3dCaps.TextureFilterCaps;
    this->m_caps.m_texFilterTrilinear = (filterCaps & D3DPTFILTERCAPS_MIPFLINEAR) != 0;
    this->m_caps.m_texFilterAnisotropic = (filterCaps & (D3DPTFILTERCAPS_MINFANISOTROPIC | D3DPTFILTERCAPS_MAGFANISOTROPIC)) != 0;

    if (filterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) {
        s_filterModes[GxTex_Anisotropic][0] = D3DTEXF_ANISOTROPIC;
    }

    if (this->m_d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC) {
        s_filterModes[GxTex_Anisotropic][1] = D3DTEXF_ANISOTROPIC;
    }

    this->m_caps.m_maxTexAnisotropy = this->m_d3dCaps.MaxAnisotropy;

    if (this->m_caps.m_texFilterAnisotropic && this->m_d3dCaps.MaxAnisotropy < 2) {
        this->m_caps.m_texFilterAnisotropic = 0;
    }

    this->m_caps.m_depthBias = (this->m_d3dCaps.RasterCaps & D3DPRASTERCAPS_DEPTHBIAS) != 0;
    this->m_caps.m_numStreams = this->m_d3dCaps.MaxStreams;
    this->m_caps.m_streamOffset = this->m_d3dCaps.Caps2 & 0x1;

    for (uint32_t i = 0; i < GxShTargets_Last; i++) {
        this->m_caps.m_shaderTargets[i] = 0;
        this->m_caps.m_shaderConstants[i] = 0;
    }

    uint32_t pixelShaderVersion = this->m_d3dCaps.PixelShaderVersion;

    if (pixelShaderVersion >= D3DPS_VERSION(3, 0)) {
        this->m_caps.m_shaderTargets[GxSh_Pixel] = 4;
    } else if (pixelShaderVersion >= D3DPS_VERSION(2, 0)) {
        this->m_caps.m_shaderTargets[GxSh_Pixel] = 3;
    } else if (pixelShaderVersion >= D3DPS_VERSION(1, 4)) {
        this->m_caps.m_shaderTargets[GxSh_Pixel] = 2;
    } else if (pixelShaderVersion >= D3DPS_VERSION(1, 1)) {
        this->m_caps.m_shaderTargets[GxSh_Pixel] = 1;
    }

    if (this->m_caps.m_shaderTargets[GxSh_Pixel]) {
        uint32_t vertexShaderVersion = this->m_d3dCaps.VertexShaderVersion;

        if (vertexShaderVersion >= D3DVS_VERSION(3, 0)) {
            this->m_caps.m_shaderTargets[GxSh_Vertex] = 3;
        } else if (vertexShaderVersion >= D3DVS_VERSION(2, 0)) {
            this->m_caps.m_shaderTargets[GxSh_Vertex] = 2;
        } else if (vertexShaderVersion == D3DVS_VERSION(1, 1)) {
            this->m_caps.m_shaderTargets[GxSh_Vertex] = 1;
        }

        this->m_caps.m_shaderConstants[GxSh_Vertex] = this->m_d3dCaps.MaxVertexShaderConst;
    }

    if (format.m_fixedFunction[GxSh_Pixel] != -1 && format.m_fixedFunction[GxSh_Pixel] <= this->m_caps.m_shaderTargets[GxSh_Pixel]) {
        this->m_caps.m_shaderTargets[GxSh_Pixel] = format.m_fixedFunction[GxSh_Pixel];
    }

    if (format.m_fixedFunction[GxSh_Vertex] != -1 && format.m_fixedFunction[GxSh_Vertex] <= this->m_caps.m_shaderTargets[GxSh_Vertex]) {
        this->m_caps.m_shaderTargets[GxSh_Vertex] = format.m_fixedFunction[GxSh_Vertex];
    }

    if (this->m_caps.m_shaderTargets[GxSh_Pixel] == 4 && this->m_caps.m_shaderTargets[GxSh_Vertex] != 3) {
        this->m_caps.m_shaderTargets[GxSh_Pixel] = 3;
    }

    D3DDEVICE_CREATION_PARAMETERS creationParameters;
    this->m_d3dDevice->GetCreationParameters(&creationParameters);

    D3DADAPTER_IDENTIFIER9 identifier;
    this->m_d3d->GetAdapterIdentifier(creationParameters.AdapterOrdinal, 0, &identifier);

    if (identifier.VendorId == 0x10DE && identifier.DeviceId - 0x300 <= 0xFF && this->m_caps.m_shaderConstants[GxSh_Vertex] > 192) {
        this->m_caps.m_shaderConstants[GxSh_Vertex] = 192;
    }

    for (uint32_t i = 0; i < GxTexFormats_Last; i++) {
        if (i) {
            this->m_caps.m_texFmt[i] = this->m_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, this->m_devAdapterFormat, 0, D3DRTYPE_TEXTURE, s_GxTexFmtToD3dFmt[i]) == D3D_OK;
        } else {
            this->m_caps.m_texFmt[0] = 0;
        }
    }

    this->m_caps.m_generateMipMaps = (this->m_d3dCaps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP) != 0;

    if (this->m_caps.m_shaderTargets[GxSh_Vertex] <= 1) {
        s_gxAttribToD3dAttribType[1] = D3DDECLTYPE_D3DCOLOR;
        s_gxAttribToD3dAttribType[2] = D3DDECLTYPE_D3DCOLOR;
    }

    this->m_caps.m_rttFormat[GxTex_Argb8888] = this->m_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, this->m_devAdapterFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8) == D3D_OK;
    this->m_caps.m_rttFormat[GxTex_Rgb565] = this->m_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, this->m_devAdapterFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_R5G6B5) == D3D_OK;
    this->m_caps.m_rttFormat[GxTex_Gr1616F] = this->m_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, this->m_devAdapterFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_G16R16F) == D3D_OK;
    this->m_caps.m_rttFormat[GxTex_R32F] = this->m_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, this->m_devAdapterFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_R32F) == D3D_OK;
    this->m_caps.m_rttFormat[GxTex_D24X8] = this->m_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, this->m_devAdapterFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, D3DFMT_D24X8) == D3D_OK;

    this->m_caps.m_colorWrite = (this->m_d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_COLORWRITEENABLE) != 0;
    this->m_caps.m_hwCursor = this->m_d3dCaps.CursorCaps & D3DCURSORCAPS_COLOR;
    this->m_caps.m_maxClipPlanes = this->m_d3dCaps.MaxUserClipPlanes;

    uint32_t textureCaps = this->m_d3dCaps.TextureCaps;
    this->m_caps.m_texTarget[GxTex_2d] = 1;
    this->m_caps.m_texTarget[GxTex_CubeMap] = (textureCaps & D3DPTEXTURECAPS_CUBEMAP) != 0;
    this->m_caps.m_texTarget[GxTex_Rectangle] = 0;
    this->m_caps.m_texTarget[GxTex_NonPow2] = (textureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) || !(textureCaps & D3DPTEXTURECAPS_POW2);
    this->m_caps.m_texNonPow2 = 1;

    if (this->m_caps.m_texTarget[GxTex_NonPow2]) {
        this->m_caps.m_texNonPow2 = (textureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) != 0;

        if (identifier.VendorId == 0x10DE) {
            this->m_caps.m_texNonPow2 = 0;
        }
    }

    LPDIRECT3DQUERY9 query = nullptr;
    this->m_caps.m_occlusionQuery = this->m_d3dDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, &query) == D3D_OK;

    if (query) {
        query->Release();
    }

    this->m_caps.m_pointParameters = this->m_d3dCaps.MaxPointSize > 1.0f;
    this->m_caps.m_pointScaleMax = this->m_d3dCaps.MaxPointSize;
    this->m_caps.m_pointSprite = this->m_d3dCaps.MaxPointSize > 1.0f;
    this->m_caps.m_blendFactor = (this->m_d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_BLENDFACTOR) != 0;
    this->m_caps.m_shaderAlphaRef = 1;
    this->m_caps.unused_0114[1] = 0;
    this->m_caps.unused_0114[0] = 0;
    this->m_caps.unused_0114[2] = 0;
    this->m_caps.unused_0114[3] = 0;
    this->m_caps.unused_0114[4] = 0;
    this->m_caps.unk_0134 = this->m_caps.m_shaderTargets[GxSh_Pixel] != 4;
    this->m_caps.unk_0138 = this->m_caps.m_shaderTargets[GxSh_Pixel] != 4;
}

// VERIFY: fa4490e9 manual
// OFFSET: 0x0068F370
static void DefaultTexUserFunc(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    // STATIC: 0x00C60340
    static CImVector s_texels[64];

    switch (command) {
        case GxTex_Lock: {
            uint32_t color = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(userArg));

            for (uint32_t i = 0; i < 64; i++) {
                *reinterpret_cast<uint32_t*>(&s_texels[i]) = color;
            }

            break;
        }

        case GxTex_Latch:
            texelStrideInBytes = 4 * width;
            texels = s_texels;
            break;

        default:
            break;
    }
}

// VERIFY: ee744ef0 manual
// OFFSET: 0x0068F3D0
int32_t CGxDeviceD3d::ICreateD3dDevice(const CGxFormat& format) {
    if (NvAPI_Initialize() == NVAPI_OK) {
        this->m_d3dStereoEnabled = 0;
        this->m_d3dNVAPI = 1;

        NvU8 stereoEnabled;

        switch (NvAPI_Stereo_IsEnabled(&stereoEnabled)) {
            case NVAPI_NOT_SUPPORTED:
                CGxDevice::Log("No stereoscopic device present.");
                break;

            case NVAPI_NO_IMPLEMENTATION:
                CGxDevice::Log("No stereoscopic driver present.");
                break;

            case NVAPI_OK:
                this->m_d3dStereoRestore = this->m_d3dStereoEnabled;
                this->m_caps.m_stereoAvailable = 1;

                if (format.m_stereoEnabled) {
                    if (!stereoEnabled) {
                        NvAPI_Status status = NvAPI_Stereo_Enable();

                        if (status != NVAPI_OK) {
                            NvAPI_ShortString message;
                            NvAPI_GetErrorMessage(status, message);
                            CGxDevice::Log("Failed to enable stereoscopic mode: %s", message);
                        }
                    }
                } else if (stereoEnabled) {
                    NvAPI_Stereo_Disable();
                }

                break;

            default:
                break;
        }
    }

    bool hwTnL = format.m_hwTnL;

    if (hwTnL && !(this->m_d3dCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)) {
        hwTnL = false;
    }

    this->m_d3dIsHwDevice = hwTnL;

    D3DPRESENT_PARAMETERS d3dpp;
    this->ISetPresentParms(d3dpp, format);

    DWORD behaviorFlags = hwTnL ? D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE | D3DCREATE_FPU_PRESERVE : D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE;

    if (FAILED(this->m_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, this->m_hwnd, behaviorFlags, &d3dpp, &this->m_d3dDevice))) {
        this->m_d3dDevice = nullptr;
        return 0;
    }

    if (this->m_d3dNVAPI) {
        NvU8 stereoEnabled = 0;

        if (NvAPI_Stereo_IsEnabled(&stereoEnabled) == NVAPI_OK && stereoEnabled) {
            NvAPI_Stereo_CreateConfigurationProfileRegistryKey(NVAPI_STEREO_DEFAULT_REGISTRY_PROFILE);
            NvAPI_Stereo_CreateHandleFromIUnknown(this->m_d3dDevice, reinterpret_cast<StereoHandle*>(&this->m_d3dStereoHandle));

            if (this->m_d3dStereoHandle) {
                this->m_d3dStereoEnabled = 1;
                this->m_d3dStereoDirty = 1;
            }
        }
    }

    this->m_devAdapterFormat = d3dpp.BackBufferFormat;
    this->m_context = 1;
    this->m_devDepthFormat = d3dpp.AutoDepthStencilFormat;

    this->ISetCaps(format);
    this->Log(this->m_caps);
    CGxDevice::Log("\tNVAPI: %d", this->m_d3dNVAPI);
    CGxDevice::Log("\tstereoHandle: %x", this->m_d3dStereoHandle);

    this->IStateSetD3DDefaults();
    this->DeviceSetGamma(this->m_gammaRamp);
    this->ICursorCreate(format);

    // STATIC: 0x00C60444
    static uint32_t s_defaultColor = 0xFF00FF00;

    GxTexCreate(8, 8, GxTex_Argb8888, CGxTexFlags(GxTex_Linear), reinterpret_cast<void*>(static_cast<uintptr_t>(s_defaultColor)), DefaultTexUserFunc, this->m_texture3B58);

    if (this->m_texture3B58) {
        GxTexUpdate(this->m_texture3B58, 0, 0, 0, 0, 1);
    }

    g_theGxDevicePtr->ShaderCreate(CGxDevice::s_uiVertexShader, GxSh_Vertex, "Shaders\\Vertex", "UI", 2);
    g_theGxDevicePtr->ShaderCreate(&CGxDevice::s_uiPixelShader, GxSh_Pixel, "Shaders\\Pixel", "UI", 1);

    return 1;
}

// VERIFY: 7e7b165d manual
// OFFSET: 0x00690150
void CGxDeviceD3d::IReleaseD3dResources(int32_t freeTextures) {
    this->ITexForceRecreation(freeTextures);
    this->IReleaseD3dShaders(freeTextures);
    this->IReleaseD3dPools(freeTextures);
    memset(this->m_deviceStates, 0xFF, sizeof(this->m_deviceStates));

    if (this->m_defColorSurface) {
        this->m_defColorSurface->Release();
        this->m_defColorSurface = nullptr;
    }

    if (this->m_defDepthSurface) {
        this->m_defDepthSurface->Release();
        this->m_defDepthSurface = nullptr;
    }

    if (this->m_defDepthStencilSurface) {
        this->m_defDepthStencilSurface->Release();
        this->m_defDepthStencilSurface = nullptr;
    }

    if (this->m_surface3B44) {
        this->m_surface3B44->Release();
        this->m_surface3B44 = nullptr;
    }

    if (this->m_eventQuery) {
        this->m_eventQuery->Release();
        this->m_eventQuery = nullptr;
    }

    this->IReleaseD3dQueries();

    if (this->m_d3dDevice) {
        this->m_d3dDevice->ShowCursor(FALSE);
    }
}

// VERIFY: 2f85a8b3 manual
// OFFSET: 0x00690230
int32_t CGxDeviceD3d::DeviceWM(EGxWM wm, uintptr_t param1, uintptr_t param2) {
    switch (wm) {
        case GxWM_Size:
            if (param2 == 1 || param2 == 2) {
                this->m_windowVisible = 0;
                break;
            }

            this->m_windowVisible = 1;
            this->DeviceSetDefWindow(*reinterpret_cast<const CRect*>(param1));

            if (this->m_d3dDevice && this->m_context) {
                this->IReleaseD3dResources(0);

                D3DPRESENT_PARAMETERS d3dpp;
                this->ISetPresentParms(d3dpp, this->m_format);

                if (this->m_d3dDevice->Reset(&d3dpp) == D3D_OK) {
                    this->IStateSetD3DDefaults();
                    this->ISetWindowFocus(1);
                    this->m_context = 1;
                    this->m_intF5C = 0;
                    this->NotifyOnDeviceRestored();
                    this->m_viewportDirty = 1;
                    break;
                }

                this->m_context = 0;
            }

            this->m_viewportDirty = 1;
            break;

        case GxWM_DisplayChange:
            if (this->m_windowVisible) {
                this->DeviceSetDefWindow(*reinterpret_cast<const CRect*>(param1));
                this->m_viewportDirty = 1;
            }

            break;

        case GxWM_Destroy:
        case GxWM_KillFocus:
            this->m_windowFocus = 0;
            break;

        case GxWM_SetFocus:
            this->ISetWindowFocus(1);
            break;

        default:
            break;
    }

    return 0;
}

// VERIFY: 53945590 manual
// OFFSET: 0x006903B0
void CGxDeviceD3d::IDestroyD3d() {
    for (uint32_t i = 0; i < 2; i++) {
        if (CGxDevice::s_uiVertexShader[i]) {
            g_theGxDevicePtr->ShaderDestroy(CGxDevice::s_uiVertexShader[i]);
        }
    }

    if (CGxDevice::s_uiPixelShader) {
        g_theGxDevicePtr->ShaderDestroy(CGxDevice::s_uiPixelShader);
    }

    this->ICursorDestroy();
    this->IReleaseD3dResources(1);
    this->IReleaseD3dVertexDecl();

    if (this->m_eventQuery) {
        this->m_eventQuery->Release();
        this->m_eventQuery = nullptr;
    }

    if (this->m_texture3B58) {
        GxTexDestroy(this->m_texture3B58);
        this->m_texture3B58 = nullptr;
    }

    if (this->m_d3dStereoHandle) {
        NvAPI_Stereo_DestroyHandle(static_cast<StereoHandle>(this->m_d3dStereoHandle));
        this->m_d3dStereoHandle = nullptr;
    }

    if (this->m_d3dDevice) {
        this->m_d3dDevice->Release();
        this->m_d3dDevice = nullptr;
    }

    if (this->m_d3dNVAPI) {
        NvU8 stereoEnabled;

        if (NvAPI_Stereo_IsEnabled(&stereoEnabled) == NVAPI_OK) {
            if (this->m_d3dStereoRestore) {
                if (!stereoEnabled) {
                    NvAPI_Stereo_Enable();
                }
            } else if (stereoEnabled) {
                NvAPI_Stereo_Disable();
            }
        }
    }
}

// VERIFY: fc602985 manual
// OFFSET: 0x006904D0
int32_t CGxDeviceD3d::DeviceSetFormat(const CGxFormat& format) {
    CGxDevice::Log("CGxDeviceD3d::DeviceSetFormat():");
    this->Log(format);

    if (this->m_hwnd) {
        ShowWindow(this->m_hwnd, SW_HIDE);
    }

    this->IDestroyD3d();

    if (this->m_hwnd) {
        DestroyWindow(this->m_hwnd);
    }

    this->m_hwnd = nullptr;
    this->m_format = format;

    CGxFormat createFormat = format;

    if (this->ICreateWindow(createFormat) && this->ICreateD3dDevice(createFormat) && CGxDevice::DeviceSetFormat(createFormat)) {
        this->ISetWindowFocus(1);
        return 1;
    }

    CGxDevice::Log("CGxDeviceD3d::DeviceSetFormat(): unable to set format!");
    this->IDestroyD3d();

    if (this->m_hwnd) {
        DestroyWindow(this->m_hwnd);
    }

    this->m_hwnd = nullptr;

    return 0;
}

// VERIFY: 1b01f8e3 manual
// OFFSET: 0x006905F0
void CGxDeviceD3d::DeviceDestroy() {
    CGxDevice::DeviceDestroy();

    if (this->m_hwnd && this->m_ownhwnd) {
        DestroyWindow(this->m_hwnd);
        this->m_hwnd = nullptr;
    }

    if (this->m_hwndClass) {
        UnregisterClassA(reinterpret_cast<const char*>(static_cast<uintptr_t>(this->m_hwndClass)), GetModuleHandleA(nullptr));
        this->m_hwndClass = 0;
    }

    this->IDestroyD3d();

    if (this->m_d3d) {
        this->m_d3d->Release();
        this->m_d3d = nullptr;
    }

    if (this->m_d3dLib) {
        FreeLibrary(this->m_d3dLib);
        this->m_d3dLib = nullptr;
    }
}

// VERIFY: cbd2d003 manual
// OFFSET: 0x00690680
int32_t CGxDeviceD3d::ICreateD3d() {
    if (CGxDeviceD3d::ILoadD3dLib(this->m_d3dLib, this->m_d3d) && SUCCEEDED(this->m_d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &this->m_d3dCaps))) {
        if (this->m_desktopDisplayMode.Format) {
            return 1;
        }

        D3DDISPLAYMODE displayMode;

        if (SUCCEEDED(this->m_d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &displayMode))) {
            this->m_desktopDisplayMode = displayMode;
            return 1;
        }
    }

    this->IDestroyD3d();
    CGxDeviceD3d::IUnloadD3dLib(this->m_d3dLib, this->m_d3d);

    return 0;
}

// VERIFY: a236ad1a auto
// OFFSET: 0x00690750
int32_t CGxDeviceD3d::DeviceCreate(GxWindowProc windowProc, const CGxFormat& format) {
    this->m_ownhwnd = 1;

    HDC dc = GetDC(nullptr);

    if (GetDeviceGammaRamp(dc, &this->m_systemGammaRamp) && &this->m_gammaRamp != &this->m_systemGammaRamp) {
        this->m_gammaRamp = this->m_systemGammaRamp;
    }

    ReleaseDC(nullptr, dc);

    this->m_hwndClass = WindowClassCreate();

    if (!this->m_hwndClass) {
        char* error = OsGetLastErrorStr();
        CGxDevice::Log("CGxDeviceD3d::DeviceCreate(): WindowClassCreate() failed: %s", error);
        OsFreeLastErrorStr(error);
        this->DeviceDestroy();
        return 0;
    }

    if (this->ICreateD3d() && CGxDevice::DeviceCreate(windowProc, format)) {
        return 1;
    }

    this->DeviceDestroy();
    return 0;
}

// VERIFY: 9f23f35b auto
// OFFSET: 0x00690830
int32_t CGxDeviceD3d::DeviceCreate(void* window, const CGxFormat& format) {
    this->m_ownhwnd = 0;
    CGxDevice::DeviceCreate(window, format);
    this->m_hwnd = static_cast<HWND>(window);

    if (this->ICreateD3d() && this->ICreateD3dDevice(format) && CGxDevice::DeviceCreate(window, format)) {
        return 1;
    }

    this->DeviceDestroy();
    return 0;
}

// VERIFY: 64f9b1e2 manual
// OFFSET: 0x006A1690
void CGxDeviceD3d::IReleaseD3dQueries() {
    for (CGxQuery* query = this->m_queryList.Head(); query; query = this->m_queryList.Next(query)) {
        if (query->m_apiSpecific) {
            static_cast<LPDIRECT3DQUERY9>(query->m_apiSpecific)->Release();
        }

        query->m_apiSpecific = nullptr;
    }
}

// VERIFY: 3b5a78ad manual
// OFFSET: 0x006A1C60
void CGxDeviceD3d::IReleaseD3dPools(int32_t freePools) {
    for (CGxPool* pool = this->m_poolList.Head(); pool; pool = this->m_poolList.Next(pool)) {
        if (freePools || pool->m_usage == GxPoolUsage_Stream || pool->m_usage == GxPoolUsage_Dynamic) {
            this->IReleaseD3dPool(pool);
        }
    }
}

// VERIFY: edfb05c2 manual
// OFFSET: 0x006A2AA0
void CGxDeviceD3d::ITexForceRecreation(int32_t all) {
    for (CGxTex* texId = this->m_textures.Head(); texId; texId = this->m_textures.Next(texId)) {
        if (!texId->m_apiSpecificData || !(all || texId->m_flags.m_renderTarget)) {
            continue;
        }

        // STATIC: 0x00C60698
        static CiRect s_updateRect;

        if (!texId->m_needsCreation && (texId->m_apiSpecificData || texId->m_apiSpecificData2)) {
            static_cast<IDirect3DBaseTexture9*>(texId->m_apiSpecificData)->Release();
        }

        texId->m_apiSpecificData = this->m_texture3B58;
        texId->m_needsCreation = 1;
        this->TexMarkForUpdate(texId, s_updateRect, 0);

        uint32_t texelStrideInBytes;
        const void* texels;
        texId->m_userFunc(GxTex_3, texId->m_width, texId->m_height, 0, 0, texId->m_userArg, texelStrideInBytes, texels);
    }

    if (this->m_hwnd && this->m_d3dDevice) {
        this->NotifyOnTextureRecreation();
    }
}

// VERIFY: d8c0bcf5 auto
// OFFSET: 0x006A2BB0
void CGxDeviceD3d::TexDestroy(CGxTex* texId) {
    if (!texId->m_needsCreation && (texId->m_apiSpecificData || texId->m_apiSpecificData2)) {
        static_cast<IDirect3DBaseTexture9*>(texId->m_apiSpecificData)->Release();
        texId->m_apiSpecificData = nullptr;
    }

    CGxDevice::TexDestroy(texId);
}

// VERIFY: 571fccbe manual
// OFFSET: 0x006A2C00
void CGxDeviceD3d::ITexCreate(CGxTex* texId) {
    uint32_t width;
    uint32_t height;
    uint32_t baseMip;
    uint32_t mipCount;
    this->ITexWHDStartEnd(texId, width, height, baseMip, mipCount);

    texId->m_format = s_GxTexFormatToUse[texId->m_format];

    DWORD usage = 0;
    D3DPOOL pool = D3DPOOL_MANAGED;

    if (texId->m_flags.m_renderTarget) {
        usage = D3DUSAGE_RENDERTARGET;
        pool = D3DPOOL_DEFAULT;
    }

    if (texId->m_flags.m_generateMipMaps) {
        usage |= D3DUSAGE_AUTOGENMIPMAP;
    }

    if (texId->m_target == GxTex_CubeMap) {
        LPDIRECT3DCUBETEXTURE9 cubeTexture = nullptr;
        if (FAILED(this->m_d3dDevice->CreateCubeTexture(width, mipCount - baseMip, usage, s_GxTexFmtToD3dFmt[texId->m_format], pool, &cubeTexture, nullptr))) {
            return;
        }

        texId->m_apiSpecificData = cubeTexture;
    } else {
        LPDIRECT3DTEXTURE9 texture = nullptr;

        if (texId->m_format == GxTex_D24X8) {
            if (FAILED(this->m_d3dDevice->CreateTexture(width, height, 1, D3DUSAGE_DEPTHSTENCIL, D3DFMT_D24X8, pool, &texture, nullptr))) {
                return;
            }
        } else {
            uint32_t levels = mipCount - baseMip;

            if (FAILED(this->m_d3dDevice->CreateTexture(width, height, levels, usage, s_GxTexFmtToD3dFmt[texId->m_format], pool, &texture, nullptr))) {
                if (texId->m_flags.m_renderTarget) {
                    return;
                }

                s_GxTexFormatToUse[texId->m_format] = s_tolerableTexFmtMapping[texId->m_format];
                texId->m_format = s_GxTexFormatToUse[texId->m_format];

                if (FAILED(this->m_d3dDevice->CreateTexture(width, height, levels, usage, s_GxTexFmtToD3dFmt[texId->m_format], pool, &texture, nullptr))) {
                    return;
                }
            }
        }

        texId->m_apiSpecificData = texture;
    }

    texId->m_needsCreation = 0;
}

// VERIFY: 70dd0433 manual
// OFFSET: 0x006A2D80
void CGxDeviceD3d::ITexUpload(CGxTex* texId) {
    uint32_t texelStrideInBytes;
    const void* texels = nullptr;
    texId->m_userFunc(GxTex_Lock, texId->m_width, texId->m_height, 0, 0, texId->m_userArg, texelStrideInBytes, texels);

    uint32_t width;
    uint32_t height;
    uint32_t baseMip;
    uint32_t mipCount;
    this->ITexWHDStartEnd(texId, width, height, baseMip, mipCount);

    int32_t compressed = texId->m_dataFormat >= GxTex_Dxt1 && texId->m_dataFormat <= GxTex_Dxt5;
    uint32_t faceCount = texId->m_target == GxTex_CubeMap ? 6 : 1;

    for (uint32_t face = 0; face < faceCount; face++) {
        uint32_t mipWidth = width;
        uint32_t mipHeight = height;
        uint32_t level = 0;

        for (uint32_t mip = baseMip; mip != mipCount; mip++, level++) {
            texels = nullptr;
            texId->m_userFunc(GxTex_Latch, mipWidth, mipHeight, face, mip, texId->m_userArg, texelStrideInBytes, texels);

            if (texels && !texId->m_flags.m_renderTarget) {
                IDirect3DSurface9* surface = nullptr;
                HRESULT result;

                if (texId->m_target == GxTex_CubeMap) {
                    result = static_cast<IDirect3DCubeTexture9*>(texId->m_apiSpecificData)->GetCubeMapSurface(s_faceTypes[face], level, &surface);
                } else {
                    result = static_cast<IDirect3DTexture9*>(texId->m_apiSpecificData)->GetSurfaceLevel(level, &surface);
                }

                if (FAILED(result)) {
                    goto done;
                }

                RECT rect;
                rect.left = texId->m_updateRect.minX >> mip;
                rect.top = texId->m_updateRect.minY >> mip;
                rect.bottom = rect.top + 1 > texId->m_updateRect.maxY >> mip ? rect.top + 1 : texId->m_updateRect.maxY >> mip;
                rect.right = rect.left + 1 > texId->m_updateRect.maxX >> mip ? rect.left + 1 : texId->m_updateRect.maxX >> mip;

                if (static_cast<uint32_t>(texId->m_format - GxTex_Dxt1) <= 2) {
                    rect.left &= ~3;
                    rect.top &= ~3;
                    rect.right = (rect.right + 3) & ~3;
                    rect.bottom = (rect.bottom + 3) & ~3;

                    if (rect.bottom > static_cast<int32_t>(mipHeight)) {
                        rect.bottom = mipHeight;
                    }

                    if (rect.right > static_cast<int32_t>(mipWidth)) {
                        rect.right = mipWidth;
                    }
                }

                D3DLOCKED_RECT lockedRect;
                if (FAILED(surface->LockRect(&lockedRect, &rect, 0))) {
                    surface->Release();
                    goto done;
                }

                const uint8_t* src = static_cast<const uint8_t*>(texels);

                if (!texId->m_flags.m_bit15) {
                    EGxTexFormat dataFormat = texId->m_dataFormat;

                    if (compressed) {
                        src += s_texFormatBytesPerBlock[dataFormat] * (rect.left >> 2) + texelStrideInBytes * (rect.top >> 2);
                    } else {
                        src += ((rect.left * s_texFormatBitDepth[dataFormat]) >> 3) + texelStrideInBytes * rect.top;
                    }
                }

                C2iVector size;
                size.x = rect.right - rect.left;
                size.y = rect.bottom - rect.top;
                Blit(size, BlitAlpha_0, src, texelStrideInBytes, GxGetBlitFormat(texId->m_dataFormat), lockedRect.pBits, lockedRect.Pitch, GxGetBlitFormat(texId->m_format));

                surface->UnlockRect();
                surface->Release();
            }

            mipHeight >>= 1;
            mipWidth >>= 1;

            if (!mipWidth) {
                mipWidth = 1;
            }

            if (!mipHeight) {
                mipHeight = 1;
            }
        }
    }

done:
    texels = nullptr;
    texId->m_userFunc(GxTex_Unlock, texId->m_width, texId->m_height, 0, 0, texId->m_userArg, texelStrideInBytes, texels);

    if (!texId->m_flags.m_renderTarget) {
        static_cast<IDirect3DBaseTexture9*>(texId->m_apiSpecificData)->PreLoad();
    }
}

// VERIFY: e0a4f6e8 manual
// OFFSET: 0x006A3070
void CGxDeviceD3d::ITexMarkAsUpdated(CGxTex* texId) {
    if (!texId->m_needsUpdate || !this->m_context) {
        return;
    }

    if (texId->m_needsCreation || (!texId->m_apiSpecificData && !texId->m_apiSpecificData2)) {
        this->ITexCreate(texId);
    }

    if (!texId->m_needsCreation && (texId->m_apiSpecificData || texId->m_apiSpecificData2)) {
        if (texId->m_userFunc) {
            this->ITexUpload(texId);
        }

        CGxDevice::ITexMarkAsUpdated(texId);
    }
}

// VERIFY: f166562f auto
// OFFSET: 0x006A3350
void CGxDeviceD3d::ISceneBegin() {
    if (!this->m_context) {
        if (this->m_d3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
            this->IReleaseD3dResources(0);

            D3DPRESENT_PARAMETERS d3dpp;
            this->ISetPresentParms(d3dpp, this->m_format);

            if (this->m_d3dDevice->Reset(&d3dpp) == D3D_OK) {
                this->IStateSetD3DDefaults();
                this->ISetWindowFocus(1);
                this->m_context = 1;
                this->m_intF5C = 0;
                this->m_d3dStereoDirty = 1;
                this->NotifyOnDeviceRestored();
            }
        }

        if (!this->m_context) {
            return;
        }
    }

    this->ShaderConstantsClear();

    if (this->m_d3dDevice->BeginScene() == D3D_OK) {
        this->m_inScene = 1;
    }
}

// VERIFY: 677ac69d manual
// OFFSET: 0x006A3450
void CGxDeviceD3d::ScenePresent() {
    if (this->m_context) {
        int32_t scrShotClick = this->m_scrShotClick;

        CGxDevice::ScenePresent();
        this->ISceneEnd();
        this->ICursorDraw();

        if (this->m_format.m_fixLag) {
            if (this->m_eventQuery) {
                this->m_eventQuery->Issue(D3DISSUE_END);

                while (this->m_eventQuery->GetData(nullptr, 0, D3DGETDATA_FLUSH) == S_FALSE) {
                    OsSleep(0);
                }
            } else {
                LPDIRECT3DSURFACE9 backBuffer;

                if (SUCCEEDED(this->m_d3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer))) {
                    D3DSURFACE_DESC desc;
                    backBuffer->GetDesc(&desc);

                    D3DLOCKED_RECT lockedRect;

                    if (SUCCEEDED(backBuffer->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY))) {
                        backBuffer->UnlockRect();
                    }

                    backBuffer->Release();
                }
            }
        }

        if (scrShotClick) {
            this->DeviceScreenShot();
        }

        this->WaitForFPSCap();

        if (FAILED(this->m_d3dDevice->Present(nullptr, nullptr, nullptr, nullptr))) {
            this->m_context = 0;
        }

        if (this->m_d3dStereoHandle && this->m_d3dStereoDirty) {
            this->m_d3dStereoDirty = 0;

            NvAPI_Status status = NvAPI_Stereo_SetConvergence(static_cast<StereoHandle>(this->m_d3dStereoHandle), this->m_d3dStereoConvergence);
            CGxDevice::Log("NvAPI_Stereo_SetConvergence: status=%d convergence=%g", status, this->m_d3dStereoConvergence);

            status = NvAPI_Stereo_SetSeparation(static_cast<StereoHandle>(this->m_d3dStereoHandle), this->m_d3dStereoSeparation);
            CGxDevice::Log("NvAPI_Stereo_SetSeparation: status=%d separation=%g", status, this->m_d3dStereoSeparation);
        }
    }

    this->ISceneBegin();
}

// VERIFY: ada703ca manual
// OFFSET: 0x006A3620
void CGxDeviceD3d::Draw(CGxBatch* batch, int32_t count) {
    if (!this->m_context || this->m_intF5C) {
        return;
    }

    this->IStateSync();

    uint32_t baseVertexIndex = 0;

    if (!this->m_caps.m_streamOffset) {
        baseVertexIndex = this->m_primVertexFormatBuf[0]->m_index / this->m_primVertexFormatBuf[0]->m_itemSize;
    }

    if (count) {
        uint32_t primCount = this->PrimCalcCount(batch->m_primType, batch->m_count);
        this->m_d3dDevice->DrawIndexedPrimitive(s_primitiveConversion[batch->m_primType], baseVertexIndex, batch->m_minIndex, batch->m_maxIndex - batch->m_minIndex + 1, batch->m_start + (this->m_primIndexBuf->m_index >> 1), primCount);
    } else {
        uint32_t primCount = this->PrimCalcCount(batch->m_primType, batch->m_count);
        this->m_d3dDevice->DrawPrimitive(s_primitiveConversion[batch->m_primType], baseVertexIndex, primCount);
    }
}

// VERIFY: 3259f92c auto
// OFFSET: 0x006A3A60
void CGxDeviceD3d::IStateSetD3DDefaults() {
    this->m_d3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    this->m_d3dDevice->SetRenderState(D3DRS_LOCALVIEWER, TRUE);
    this->m_d3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    this->m_d3dDevice->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
    this->m_d3dDevice->SetRenderState(D3DRS_FOGDENSITY, 0);

    for (uint32_t i = 0; i < 16; i++) {
        this->m_d3dDevice->SetSamplerState(i, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
    }

    this->IRsForceUpdate();
    this->IRsSync(0);

    this->m_primVertexDirty = 0xFFFFFFFF;
    this->m_primIndexDirty = 0;
    this->m_d3dDevice->SetIndices(nullptr);
    this->m_d3dCurrentVertexDecl = nullptr;
    this->m_d3dCurrentIndexBuf = nullptr;

    for (uint32_t i = 0; i < 8; i++) {
        this->m_d3dVertexStreamBuf[i] = nullptr;
        this->m_d3dVertexStreamOfs[i] = 0xFFFFFFFF;
        this->m_d3dVertexStreamStride[i] = 0xFFFFFFFF;
    }

    this->m_d3dDevice->GetRenderTarget(0, &this->m_defColorSurface);
    this->m_d3dDevice->GetDepthStencilSurface(&this->m_defDepthSurface);
    this->InitLights();
    this->m_ambientOnlyMode = 0;

    if (!this->m_eventQuery) {
        this->m_d3dDevice->CreateQuery(D3DQUERYTYPE_EVENT, &this->m_eventQuery);
    }

    this->ISceneBegin();
}

// VERIFY: 079341d1 auto
// OFFSET: 0x006A5680
void CGxDeviceD3d::IReleaseD3dVertexDecl() {
    for (uint32_t i = 0; i < GxVertexBufferFormats_Last; i++) {
        if (this->m_d3dVertexDecl[i]) {
            this->m_d3dVertexDecl[i]->Release();
            this->m_d3dVertexDecl[i] = nullptr;
        }
    }

    for (uint32_t i = 0; i < this->m_gxVertexDecl.Count(); i++) {
        this->m_gxVertexDecl[i].m_d3dDecl->Release();
    }

    this->m_gxVertexDecl.m_count = 0;
}

// VERIFY: d5e85bed manual
// OFFSET: 0x006A5E40
void CGxDeviceD3d::IReleaseD3dShaders(int32_t freeShaders) {
    if (!freeShaders) {
        return;
    }

    TSHashTable<CGxShader, HASHKEY_STRI>& pixelShaders = this->m_shaderList[GxSh_Pixel];

    for (CGxShader* shader = pixelShaders.m_fulllist.Head(); shader; shader = pixelShaders.m_fulllist.Next(shader)) {
        if (shader->m_apiSpecific) {
            static_cast<LPDIRECT3DPIXELSHADER9>(shader->m_apiSpecific)->Release();
            shader->m_apiSpecific = nullptr;
            shader->m_loaded = 0;
        }
    }

    TSHashTable<CGxShader, HASHKEY_STRI>& vertexShaders = this->m_shaderList[GxSh_Vertex];

    for (CGxShader* shader = vertexShaders.m_fulllist.Head(); shader; shader = vertexShaders.m_fulllist.Next(shader)) {
        if (shader->m_apiSpecific) {
            static_cast<LPDIRECT3DVERTEXSHADER9>(shader->m_apiSpecific)->Release();
            shader->m_apiSpecific = nullptr;
            shader->m_loaded = 0;
        }
    }
}

// VERIFY: fca6a4dc manual
// OFFSET: 0x006A74B0
void CGxDeviceD3d::SceneClear(uint32_t mask, CImVector color) {
    if (!this->m_context) {
        return;
    }

    DWORD flags = (mask & 0x1) != 0;

    if (mask & 0x2) {
        flags |= D3DCLEAR_ZBUFFER;
    }

    if (this->m_viewportDirty) {
        this->IXformSetViewport();
    }

    this->m_d3dDevice->Clear(0, nullptr, flags, D3DCOLOR_ARGB(color.a, color.r, color.g, color.b), 1.0f, 0);
}

// VERIFY: 9a01b076 auto
// OFFSET: 0x0068E4A0
void CGxDeviceD3d::DeviceSetBaseMipLevel(uint32_t level) {
    this->ITexForceRecreation(1);
    CGxDevice::DeviceSetBaseMipLevel(level);
}

// VERIFY: bc1bdcba manual
// OFFSET: 0x006A0360
LRESULT CALLBACK CGxDeviceD3d::WindowProcD3d(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    CGxDeviceD3d* device = reinterpret_cast<CGxDeviceD3d*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

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

            uint32_t resizeType = 0;

            if (wparam == SIZE_MINIMIZED) {
                resizeType = 1;
            } else if (wparam == SIZE_MAXHIDE) {
                resizeType = 2;
            }

            device->DeviceWM(GxWM_Size, reinterpret_cast<uintptr_t>(&rect), resizeType);
            break;
        }

        case WM_ACTIVATE:
            if (LOWORD(wparam) == WA_INACTIVE) {
                if (!device->IDevIsWindowed()) {
                    CRect rect;
                    rect.minY = 0.0f;
                    rect.minX = 0.0f;
                    rect.maxY = 0.0f;
                    rect.maxX = 0.0f;
                    device->DeviceWM(GxWM_Size, reinterpret_cast<uintptr_t>(&rect), 1);
                }
            } else if (LOWORD(wparam) == WA_ACTIVE && !device->IDevIsWindowed()) {
                CRect rect;
                rect.minY = 0.0f;
                rect.minX = 0.0f;
                rect.maxY = 0.0f;
                rect.maxX = 0.0f;
                GxCapsWindowSizeInScreenCoords(rect);
                device->DeviceWM(GxWM_Size, reinterpret_cast<uintptr_t>(&rect), 3);
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

        case WM_SETCURSOR:
            if (!device) {
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }

            if (device->m_d3dDevice && LOWORD(lparam) == HTCLIENT) {
                SetCursor(nullptr);
                device->m_d3dDevice->ShowCursor(device->m_cursorVisible && device->m_hwCursor);
                return 1;
            }

            break;

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

// VERIFY: d4889697 auto
// OFFSET: 0x0068E510
void CGxDeviceD3d::DeviceSaveDepthSurface() {
    if (!this->m_context) {
        return;
    }

    this->m_d3dDevice->GetDepthStencilSurface(&s_savedDepthSurface);
    s_savedDepthSurface->Release();
}

// VERIFY: 50a84e81 auto
// OFFSET: 0x0068E540
void CGxDeviceD3d::DeviceRestoreDepthSurface() {
    if (!this->m_context) {
        return;
    }

    this->m_d3dDevice->SetDepthStencilSurface(s_savedDepthSurface);
}

// VERIFY: c0ba5e6c manual
// OFFSET: 0x0068E570
void CGxDeviceD3d::LogCrashInfo(char* buffer, uint32_t size) {
    D3DDEVICE_CREATION_PARAMETERS parameters;
    this->m_d3dDevice->GetCreationParameters(&parameters);

    uint32_t adapterCount = this->m_d3d->GetAdapterCount();

    char line[260];
    SStrPrintf(line, sizeof(line), "Adapter Count: %d\r\n", adapterCount);
    SStrPack(buffer, line, size);

    for (uint32_t i = 0; i < adapterCount; i++) {
        SStrPrintf(line, sizeof(line), "\r\nAdapter %d%s:\r\n", i, parameters.AdapterOrdinal == i ? " (primary)" : "");
        SStrPack(buffer, line, size);

        D3DADAPTER_IDENTIFIER9 identifier;
        this->m_d3d->GetAdapterIdentifier(i, 0, &identifier);

        SStrPrintf(line, sizeof(line), "  Driver: %s\r\n", identifier.Driver);
        SStrPack(buffer, line, size);
        SStrPrintf(line, sizeof(line), "  Version: %d.%d.%04d.%04d\r\n", HIWORD(identifier.DriverVersion.HighPart), LOWORD(identifier.DriverVersion.HighPart), HIWORD(identifier.DriverVersion.LowPart), LOWORD(identifier.DriverVersion.LowPart));
        SStrPack(buffer, line, size);
        SStrPrintf(line, sizeof(line), "  Description: %s\r\n", identifier.Description);
        SStrPack(buffer, line, size);
        SStrPrintf(line, sizeof(line), "  DeviceName: %s\r\n", identifier.DeviceName);
        SStrPack(buffer, line, size);
    }
}

// VERIFY: ba97dbff manual
// OFFSET: 0x0068F6A0
void CGxDeviceD3d::ICreateReadbackSurface() {
    if (this->m_surface3B44) {
        return;
    }

    const CRect& window = this->DeviceDefWindow();
    D3DFORMAT format;

    if (this->m_format.m_window) {
        D3DDISPLAYMODE displayMode;

        if (SUCCEEDED(this->m_d3d->GetAdapterDisplayMode(0, &displayMode))) {
            format = displayMode.Format;
        } else {
            format = this->m_desktopDisplayMode.Format;
        }
    } else {
        format = s_GxFormatToD3dFormat[this->m_format.m_colorFormat];
    }

    this->m_d3dDevice->CreateRenderTarget(static_cast<UINT>(window.maxX - window.minX), static_cast<UINT>(window.maxY - window.minY), format, D3DMULTISAMPLE_NONE, 0, TRUE, &this->m_surface3B44, nullptr);
}

// VERIFY: 7f0e0e27 manual
// OFFSET: 0x0068F770
void CGxDeviceD3d::DeviceSetRenderTarget(EGxBuffer buffer, CGxTex* texture, uint32_t plane) {
    if (!this->m_context) {
        return;
    }

    CGxDevice::TextureTarget& target = this->m_textureTarget[buffer];

    if (target.m_texture == texture && target.m_plane == plane) {
        return;
    }

    CGxDevice::DeviceSetRenderTarget(buffer, texture, plane);

    if (target.m_apiSpecific) {
        static_cast<LPDIRECT3DSURFACE9>(target.m_apiSpecific)->Release();
        target.m_apiSpecific = nullptr;
    }

    if (texture) {
        if (texture->m_needsCreation || (!texture->m_apiSpecificData && !texture->m_apiSpecificData2)) {
            this->ITexCreate(texture);
        }

        if (!texture->m_needsCreation && (texture->m_apiSpecificData || texture->m_apiSpecificData2)) {
            if (texture->m_target == GxTex_CubeMap) {
                static_cast<IDirect3DCubeTexture9*>(texture->m_apiSpecificData)->GetCubeMapSurface(s_faceTypes[plane], 0, reinterpret_cast<LPDIRECT3DSURFACE9*>(&target.m_apiSpecific));
            } else {
                static_cast<IDirect3DTexture9*>(texture->m_apiSpecificData)->GetSurfaceLevel(0, reinterpret_cast<LPDIRECT3DSURFACE9*>(&target.m_apiSpecific));
            }
        }
    }

    LPDIRECT3DSURFACE9 depthSurface = this->m_defDepthSurface;
    LPDIRECT3DSURFACE9 colorSurface = this->m_defColorSurface;

    if (this->m_textureTarget[GxBuffers_Color].m_apiSpecific) {
        colorSurface = static_cast<LPDIRECT3DSURFACE9>(this->m_textureTarget[GxBuffers_Color].m_apiSpecific);

        if (!this->m_textureTarget[GxBuffers_Depth].m_apiSpecific) {
            this->ICreateDepthStencilSurface(texture->m_width, texture->m_height);

            if (this->m_defDepthStencilSurface) {
                depthSurface = this->m_defDepthStencilSurface;
            }
        }
    }

    if (this->m_textureTarget[GxBuffers_Depth].m_apiSpecific) {
        depthSurface = static_cast<LPDIRECT3DSURFACE9>(this->m_textureTarget[GxBuffers_Depth].m_apiSpecific);
    }

    this->m_d3dDevice->SetRenderTarget(0, colorSurface);
    this->m_d3dDevice->SetDepthStencilSurface(depthSurface);

    GxXformSetViewport(this->m_viewport.m_x.m_l, this->m_viewport.m_x.m_h, this->m_viewport.m_y.m_l, this->m_viewport.m_y.m_h, this->m_viewport.m_z.m_l, this->m_viewport.m_z.m_h);

    this->m_viewportDirty = 1;
    this->m_scissorRectDirty = 1;
}

// VERIFY: 312f9bf5 auto
// OFFSET: 0x0068F900
void CGxDeviceD3d::DeviceResolveDepthBuffer(CGxTex* texture) {
    if (!this->m_context) {
        return;
    }

    if (texture && (texture->m_needsCreation || (!texture->m_apiSpecificData && !texture->m_apiSpecificData2))) {
        this->ITexCreate(texture);
    }

    this->m_d3dDevice->SetDepthStencilSurface(static_cast<LPDIRECT3DSURFACE9>(texture->m_apiSpecificData));
}

// VERIFY: 956c292e manual
// OFFSET: 0x0068F950
void CGxDeviceD3d::DeviceCopyTex(CGxTex* source, uint32_t sourcePlane, CGxTex* dest, uint32_t destPlane) {
    LPDIRECT3DSURFACE9 sourceSurface = nullptr;
    LPDIRECT3DSURFACE9 destSurface = nullptr;

    if (source) {
        if (source->m_needsCreation || (!source->m_apiSpecificData && !source->m_apiSpecificData2)) {
            this->ITexCreate(source);
        }

        if (!source->m_needsCreation && (source->m_apiSpecificData || source->m_apiSpecificData2)) {
            if (source->m_target == GxTex_CubeMap) {
                static_cast<IDirect3DCubeTexture9*>(source->m_apiSpecificData)->GetCubeMapSurface(s_faceTypes[sourcePlane], 0, &sourceSurface);
            } else {
                static_cast<IDirect3DTexture9*>(source->m_apiSpecificData)->GetSurfaceLevel(0, &sourceSurface);
            }
        }
    }

    if (dest) {
        if (dest->m_needsCreation || (!dest->m_apiSpecificData && !dest->m_apiSpecificData2)) {
            this->ITexCreate(dest);
        }

        if (!dest->m_needsCreation && (dest->m_apiSpecificData || dest->m_apiSpecificData2)) {
            if (dest->m_target == GxTex_CubeMap) {
                static_cast<IDirect3DCubeTexture9*>(dest->m_apiSpecificData)->GetCubeMapSurface(s_faceTypes[destPlane], 0, &destSurface);
            } else {
                static_cast<IDirect3DTexture9*>(dest->m_apiSpecificData)->GetSurfaceLevel(0, &destSurface);
            }
        }
    }

    if (sourceSurface) {
        if (destSurface) {
            this->m_d3dDevice->StretchRect(sourceSurface, nullptr, destSurface, nullptr, D3DTEXF_NONE);
        }

        if (sourceSurface) {
            sourceSurface->Release();
        }
    }

    if (destSurface) {
        destSurface->Release();
    }
}

// VERIFY: 6d2fe3bf manual
// OFFSET: 0x0068FE80
CGxDeviceD3d::~CGxDeviceD3d() {
}

// VERIFY: 36fbe9bf manual
// OFFSET: 0x0068FED0
void CGxDeviceD3d::DeviceReadPixels(CiRect& rect, TSGrowableArray<CImVector>& pixels) {
    this->ClampRectToWindow(rect);

    uint32_t height = rect.maxY - rect.minY;
    uint32_t width = rect.maxX - rect.minX;
    uint32_t count = width * height;
    pixels.SetCount(count);

    LPDIRECT3DSURFACE9 surface = nullptr;
    HRESULT result;

    if (this->m_format.m_multisampleCount < 2) {
        result = this->m_d3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
    } else {
        RECT sourceRect = { rect.minX, rect.minY, static_cast<LONG>(width), static_cast<LONG>(height) };

        this->ICreateReadbackSurface();

        if (!this->m_surface3B44) {
            return;
        }

        result = this->m_d3dDevice->StretchRect(this->m_defColorSurface, &sourceRect, this->m_surface3B44, &sourceRect, D3DTEXF_NONE);
        surface = this->m_surface3B44;
        surface->AddRef();
    }

    if (FAILED(result)) {
        return;
    }

    D3DLOCKED_RECT lockedRect;

    if (SUCCEEDED(surface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY))) {
        D3DSURFACE_DESC desc;
        surface->GetDesc(&desc);

        if (width <= desc.Width && height <= desc.Height) {
            const uint8_t* source = static_cast<const uint8_t*>(lockedRect.pBits);

            switch (desc.Format) {
                case D3DFMT_A8R8G8B8:
                case D3DFMT_X8R8G8B8: {
                    CImVector* dest = pixels.Ptr();

                    for (uint32_t y = height; y; y--) {
                        memcpy(dest, source, sizeof(CImVector) * width);
                        source += lockedRect.Pitch;
                        dest += width;
                    }

                    break;
                }

                case D3DFMT_R5G6B5: {
                    uint8_t* dest = reinterpret_cast<uint8_t*>(pixels.Ptr());

                    for (uint32_t y = height; y; y--) {
                        const uint8_t* row = source;

                        for (uint32_t x = width; x; x--) {
                            dest[3] = 0xFF;
                            dest[2] = row[1] & 0xF8;
                            dest[1] = (*reinterpret_cast<const uint16_t*>(row) >> 3) & 0xFC;
                            dest[0] = row[0] << 3;
                            dest += 4;
                            row += 2;
                        }

                        source += lockedRect.Pitch;
                    }

                    break;
                }

                case D3DFMT_X1R5G5B5:
                case D3DFMT_A1R5G5B5: {
                    uint8_t* dest = reinterpret_cast<uint8_t*>(pixels.Ptr());

                    for (uint32_t y = height; y; y--) {
                        const uint8_t* row = source;

                        for (uint32_t x = width; x; x--) {
                            dest[3] = 0xFF;
                            dest[2] = (*reinterpret_cast<const uint16_t*>(row) >> 7) & 0xF8;
                            dest[1] = (*reinterpret_cast<const uint16_t*>(row) >> 2) & 0xF8;
                            dest[0] = row[0] << 3;
                            dest += 4;
                            row += 2;
                        }

                        source += lockedRect.Pitch;
                    }

                    break;
                }

                default:
                    memset(pixels.Ptr(), 0xFF, sizeof(CImVector) * count);
                    break;
            }
        }

        surface->UnlockRect();
    }

    surface->Release();
}

// VERIFY: dd2f1dd0 manual
// OFFSET: 0x0069FAA0
int32_t CGxDeviceD3d::ICheckDepthStencilFormat(D3DFORMAT format) {
    return SUCCEEDED(this->m_d3d->CheckDeviceFormat(0, D3DDEVTYPE_HAL, this->m_devAdapterFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, format)) && SUCCEEDED(this->m_d3d->CheckDepthStencilMatch(0, D3DDEVTYPE_HAL, this->m_devAdapterFormat, this->m_devAdapterFormat, format));
}

// VERIFY: 18e824b7 manual
// OFFSET: 0x006A1950
void CGxDeviceD3d::DeviceReadDepths(CiRect& rect, TSGrowableArray<float>& depths) {
    this->ClampRectToWindow(rect);

    uint32_t count = (rect.maxY - rect.minY) * (rect.maxX - rect.minX);
    depths.SetCount(count);
    memset(depths.Ptr(), 0, sizeof(float) * count);
}

// VERIFY: de151551 manual
// OFFSET: 0x006A30D0
int32_t CGxDeviceD3d::TexCopy(CGxTex* dest, const C2iVector& destPos, const C2iVector& sourcePos, const C2iVector& size, uint32_t level, uint32_t plane) {
    if (!this->m_context) {
        return 0;
    }

    if (dest->m_needsCreation || (!dest->m_apiSpecificData && !dest->m_apiSpecificData2)) {
        this->ITexCreate(dest);
    }

    if (dest->m_needsCreation || (!dest->m_apiSpecificData && !dest->m_apiSpecificData2)) {
        return 0;
    }

    RECT sourceRect = { sourcePos.x, sourcePos.y, sourcePos.x + size.x, sourcePos.y + size.y };
    RECT destRect = { destPos.x, destPos.y, destPos.x + size.x, destPos.y + size.y };

    LPDIRECT3DSURFACE9 surface;
    HRESULT result;

    if (dest->m_target == GxTex_CubeMap) {
        result = static_cast<IDirect3DCubeTexture9*>(dest->m_apiSpecificData)->GetCubeMapSurface(s_faceTypes[plane], level, &surface);
    } else {
        result = static_cast<IDirect3DTexture9*>(dest->m_apiSpecificData)->GetSurfaceLevel(level, &surface);
    }

    if (SUCCEEDED(result)) {
        result = this->m_d3dDevice->StretchRect(this->m_defColorSurface, &sourceRect, surface, &destRect, D3DTEXF_NONE);
        surface->Release();
    }

    return SUCCEEDED(result);
}

// VERIFY: 3b8d3c9e manual
// OFFSET: 0x006A31E0
int32_t CGxDeviceD3d::TexStretch(CGxTex* dest, const CiRect* destRect, const CiRect* sourceRect, uint32_t level, uint32_t face) {
    if (!this->m_context) {
        return 0;
    }

    if (dest->m_needsCreation || (!dest->m_apiSpecificData && !dest->m_apiSpecificData2)) {
        this->ITexCreate(dest);
    }

    if (dest->m_needsCreation || (!dest->m_apiSpecificData && !dest->m_apiSpecificData2)) {
        return 0;
    }

    LPDIRECT3DSURFACE9 surface;
    HRESULT result;

    if (dest->m_target == GxTex_CubeMap) {
        result = static_cast<IDirect3DCubeTexture9*>(dest->m_apiSpecificData)->GetCubeMapSurface(s_faceTypes[face], level, &surface);
    } else {
        result = static_cast<IDirect3DTexture9*>(dest->m_apiSpecificData)->GetSurfaceLevel(level, &surface);
    }

    if (SUCCEEDED(result)) {
        RECT d3dDestRect = { 0, 0, 0, 0 };

        if (destRect) {
            d3dDestRect.left = destRect->minX;
            d3dDestRect.top = destRect->minY;
            d3dDestRect.right = destRect->maxX;
            d3dDestRect.bottom = destRect->maxY;
        }

        RECT d3dSourceRect = { 0, 0, 0, 0 };

        if (sourceRect) {
            d3dSourceRect.left = sourceRect->minX;
            d3dSourceRect.top = sourceRect->minY;
            d3dSourceRect.right = sourceRect->maxX;
            d3dSourceRect.bottom = sourceRect->maxY;
        }

        D3DTEXTUREFILTERTYPE filter;

        if (destRect && sourceRect && d3dDestRect.right - d3dDestRect.left == d3dSourceRect.right - d3dSourceRect.left && d3dDestRect.bottom - d3dDestRect.top == d3dSourceRect.bottom - d3dSourceRect.top) {
            filter = D3DTEXF_NONE;
        } else {
            filter = D3DTEXF_LINEAR;
        }

        result = this->m_d3dDevice->StretchRect(this->m_defColorSurface, sourceRect ? &d3dSourceRect : nullptr, surface, destRect ? &d3dDestRect : nullptr, filter);
        surface->Release();
    }

    return SUCCEEDED(result);
}

// VERIFY: 1aeb3daa auto
// OFFSET: 0x006A5D50
void CGxDeviceD3d::IShaderReload(CGxShader* shader, const char* path, const char* name) {
    switch (shader->m_target) {
        case GxSh_Vertex:
            if (shader->m_refCount == 1) {
                if (shader->m_apiSpecific) {
                    static_cast<LPDIRECT3DVERTEXSHADER9>(shader->m_apiSpecific)->Release();
                    shader->m_apiSpecific = nullptr;
                }

                shader->m_loaded = 0;
            }

            this->IShaderLoad(&shader, GxSh_Vertex, path, name, 1);
            this->IShaderCreateVertex(shader);
            break;

        case GxSh_Pixel:
            if (shader->m_refCount == 1) {
                if (shader->m_apiSpecific) {
                    static_cast<LPDIRECT3DPIXELSHADER9>(shader->m_apiSpecific)->Release();
                    shader->m_apiSpecific = nullptr;
                }

                shader->m_loaded = 0;
            }

            this->IShaderLoad(&shader, GxSh_Pixel, path, name, 1);
            this->IShaderCreatePixel(shader);
            break;

        default:
            break;
    }
}

// VERIFY: ddea5205 auto
// OFFSET: 0x006A7940
void CGxDeviceD3d::ICreateDepthStencilSurface(uint32_t width, uint32_t height) {
    if (this->m_defDepthStencilSurface) {
        D3DSURFACE_DESC desc;

        if (SUCCEEDED(this->m_defDepthStencilSurface->GetDesc(&desc)) && desc.Width >= width && desc.Height >= height) {
            return;
        }

        this->m_defDepthStencilSurface->Release();
    }

    D3DFORMAT formats[3] = { D3DFMT_D24S8, D3DFMT_D24X8, D3DFMT_D32 };

    for (uint32_t i = 0; i < 3; i++) {
        D3DFORMAT format = formats[i];

        if (this->ICheckDepthStencilFormat(format)) {
            this->m_d3dDevice->CreateDepthStencilSurface(width, height, format, D3DMULTISAMPLE_NONE, 0, FALSE, &this->m_defDepthStencilSurface, nullptr);
        }

        if (this->m_defDepthStencilSurface) {
            break;
        }
    }
}
