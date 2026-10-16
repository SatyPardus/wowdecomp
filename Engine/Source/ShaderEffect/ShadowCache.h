#ifndef ENGINE_SOURCE_SHADEREFFECT_SHADOWCACHE_H
#define ENGINE_SOURCE_SHADEREFFECT_SHADOWCACHE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/ShaderEffect/Types.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CFrustum.h"

class CGxTex;
class CTexture;
struct CShadowCacheQueryData;

typedef void (*ShadowMatrixCallback)(C3Vector& center, float radius, C44Matrix& matrix, C3Vector& direction, int32_t band);
typedef void (*ShadowFrustumCallback)(CFrustum& frustum, C44Matrix& matrix, const C3Vector& direction, CShadowCacheQueryData& data, int32_t band);
typedef int32_t (*ShadowQueryCallback)(CShadowCacheQueryData& data, uint32_t frame);
typedef int32_t (*ShadowRenderCallback)(CShadowCacheQueryData& data, int32_t band, CGxTex* target, CGxTex* colorTarget, C3Vector& direction);

struct CShadowBand {
    /* 0000 */ CTexture* m_textures[2];
    /* 0008 */ float m_radius;
    /* 000C */ float m_updateDistance;
    /* 0010 */ C3Vector m_lastPos;
    /* 001C */ C3Vector m_pos;
    /* 0028 */ C3Vector m_dir;
    /* 0034 */ uint32_t m_updateCount;
    /* 0038 */ uint32_t m_current;

    CShadowBand();
};

struct CShadowCacheQueryData {
    /* 0000 */ int32_t m_queryFlags[3];
    /* 000C */ int32_t m_active[3];
    /* 0018 */ int32_t m_useCullFrustum[3];
    /* 0024 */ CAaBox m_extents[3];
    /* 006C */ CFrustum m_frustums[3];
    /* 0348 */ CFrustum m_cullFrustums[3];
    /* 0624 */ CFrustum m_bandFrustums[3];
    /* 0900 */ float m_cullRects[3][4];
    /* 0930 */ C3Vector m_pos[3];
    /* 0954 */ uint8_t m_textureIndex[3];
    /* 0958 */ float m_radius[3];
    /* 0964 */ float m_bounds[3][4];
    /* 0994 */ float m_texBounds[3][4];
    /* 09C4 */ C44Matrix m_matrices[3];
    /* 0A84 */ int32_t m_lastBand;
    /* 0A88 */ int32_t m_useFrustum;

    CShadowCacheQueryData();
};

class CShadowCache {
    public:
    static const float s_bandRadius[3];
    static const float s_bandUpdateDistance[3];
    static C4Vector s_cutoffPlane;
    static C4Vector s_cutoffPlaneDefault;
    static float s_debugAspect;
    static int32_t s_debugLevel;
    static CShadowBand s_extDynShadow;
    static float s_extDynShadowCullRect[4];
    static CShadowBand s_extShadowBands[3];
    static uint8_t s_frameCounter;
    static ShadowFrustumCallback s_frustumCallback;
    static CTexture* s_intDynShadow;
    static ShadowMatrixCallback s_matrixCallback;
    static int32_t s_needsRebuild;
    static C4Vector s_pcfOffsets[8];
    static ShadowQueryCallback s_queryCallback;
    static int32_t s_recreateTextures;
    static ShadowRenderCallback s_renderCallback;
    static EShadowMode s_shadowMode;
    static C4Vector s_shadowMatrices[12];
    static float s_shadowScale;
    static uint32_t s_shadowSize;
    static const char* s_shadowModeDescs[NUM_SHADOW_MODES + 1];
    static const uint32_t s_shadowValues[NUM_SHADOW_MODES + 1];
    static C3Vector s_sunDir;
    static C4Vector s_sunDirViewSpace;
    static float s_textureSize[2];

    static void ClearBandTextures();
    static void Destroy();
    static void GatherBands(CShadowCacheQueryData& data, const C3Vector& pos, CFrustum& frustum, int32_t useCullFrustum);
    static uint32_t GetMaxShadowMode();
    static EShadowMode GetShadowMode();
    static const char* GetShadowModeDesc(uint32_t mode);
    static uint32_t GetShadowValue();
    static void Init();
    static int32_t Initialize();
    static void InitPcfOffsets();
    static void __fastcall NotifyTexturesRecreate();
    static void PostUpdate();
    static void PreUpdate(const C3Vector& sunDir);
    static void RenderBands(CShadowCacheQueryData& data);
    static void SetDebugParams(float aspect, int32_t level);
    static void SetFrustumCallback(ShadowFrustumCallback callback);
    static void SetMatrixCallback(ShadowMatrixCallback callback);
    static void SetQueryCallback(ShadowQueryCallback callback);
    static void SetRenderCallback(ShadowRenderCallback callback);
    static void SetShadowMapDetailDoodad();
    static void SetShadowMapGenericGlobal();
    static void SetShadowMapGenericInterior(int32_t interior);
    static void SetShadowMapTerrain();
    static int32_t SetShadowMode(EShadowMode mode);
    static void SetShadowScale(float scale);
    static int32_t Update(const C3Vector& pos, int32_t useCullFrustum);
    static int32_t ValidateShadowMode(EShadowMode mode);
};

#endif
