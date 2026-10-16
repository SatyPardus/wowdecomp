#ifndef WOW_SOURCE_WORLDCLIENT_DAYNIGHT_H
#define WOW_SOURCE_WORLDCLIENT_DAYNIGHT_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CMemBlock.h"
#include "Engine/Source/Storm/H/stpl.h"

class LightFloatBandRec;
class LightParamsRec;

class C2Vector;
class C3Vector;
class CImVector;
class CM2Model;
class CM2Scene;
class CTexture;
class CGxQuery;
class C44Matrix;
class CPortalView;
namespace DayNight { struct DNPlanet; }
class LightRec;

struct DNKeyframe {
    /* 0000 */ float m_time;
    /* 0004 */ float m_value;
};

namespace DayNight {
class CurrentLight {
    public:
    struct SkyboxBlend {
        /* 0000 */ uint32_t m_id;
        /* 0004 */ float m_weight;
    };

    /* 0000 */ CImVector m_ambient;
    /* 0004 */ CImVector m_diffuse;
    /* 0008 */ CImVector m_shadowOpacity;
    /* 000C */ CImVector m_skyColors [6];
    /* 0024 */ CImVector m_sunCloudColors [5];
    /* 0038 */ CImVector m_waterColors [4];
    /* 0048 */ float m_fogEnd;
    /* 004C */ float m_fogStartMul;
    /* 0050 */ float m_fogRate;
    /* 0054 */ float m_highlightSky;
    /* 0058 */ float m_glow;
    /* 005C */ float m_floatBand2;
    /* 0060 */ float m_cloudDensity;
    /* 0064 */ float m_floatBand4;
    /* 0068 */ float m_floatBand5;
    /* 006C */ float m_waterShallowAlpha;
    /* 0070 */ float m_waterDeepAlpha;
    /* 0074 */ float m_oceanShallowAlpha;
    /* 0078 */ float m_oceanDeepAlpha;
    /* 007C */ SkyboxBlend m_skyboxes [3];
    /* 0094 */ uint32_t m_cloudTypeId;
    /* 0098 */ float m_cloudWeight;

    CurrentLight();
    CurrentLight& operator=(const CurrentLight& source);
};
}

namespace DayNight {
struct DNOverrideSky : public TSHashObject<DNOverrideSky, HASHKEY_STRI> {
    /* 0018 */ CM2Model* m_model;
    /* 001C */ uint32_t m_animLengthMs;
    /* 0020 */ uint32_t m_lastTimeOfDay;
    /* 0024 */ uint32_t m_flags;
};
}

namespace DayNight {
struct DNClouds {
    /* 0000 */ float m_sharpness;
    /* 0004 */ float m_densityOverride;
    /* 0008 */ uint8_t m_density;
    /* 0009 */ uint8_t m_lod;
    /* 000A */ uint8_t m_fullUpdate;
    /* 000B */ uint8_t m_texIndex;
    /* 000C */ float m_animSpeed;
    /* 0010 */ uint32_t m_rowsPerUpdate;
    /* 0014 */ uint32_t m_row;
    /* 0018 */ EGxTexFormat m_texFormat;
    /* 001C */ uint32_t m_size;
    /* 0020 */ uint32_t m_shift;
    /* 0024 */ uint32_t m_mask;
    /* 0028 */ uint32_t m_octaves;
    /* 002C */ uint32_t m_enabled;
    /* 0030 */ TSFixedArray<CImVector> m_texels;
    /* 003C */ TSFixedArray<uint8_t> m_densities;
    /* 0048 */ TSFixedArray<float> m_prevRow;
    /* 0054 */ TSFixedArray<C3Vector> m_verts;
    /* 0060 */ TSFixedArray<C2Vector> m_texCoords;
    /* 006C */ TSFixedArray<CImVector> m_colors;
    /* 0078 */ TSFixedArray<uint16_t> m_indices;
    /* 0084 */ uint16_t m_nIndices;
    /* 0086 */ uint16_t m_nVerts;
    /* 0088 */ uint16_t m_animFrame;
    /* 008A */ uint16_t unused_008A;
    /* 008C */ float m_animTime;
    /* 0090 */ CTexture* m_textures [2];

    static void __stdcall Collide(const C3Vector* origin, const C3Vector* dir, C3Vector* out);
    static void Callback_GxTex(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels);
    static void Update();

    DNClouds();
    void GenSphere(float radius);
    void Render();
    void SetLOD(uint32_t lod, uint32_t rowsPerUpdate);
    void SetSharpness(float sharpness);
    void SetupBumpMap(C3Vector* shadeColor, C3Vector* sunColor, C3Vector* baseColor, C3Vector* sunPos, float* intensity);
    void UpdateTextureAnimation();
    void WorldToTexture(const C3Vector* world, C2Vector* uv);
    float GetDensity(const C3Vector* position, float scale);
};
}

namespace DayNight {
struct DNGlare {
    /* 0004 */ CGxQuery* m_occlusionQuery;
    /* 0008 */ uint32_t m_enable;
    /* 000C */ C3Vector m_position;
    /* 0018 */ CImVector m_color;
    /* 001C */ CTexture* m_texture;
    /* 0020 */ float m_baseScale;
    /* 0024 */ float m_scale;
    /* 0028 */ float m_fadeInRate;
    /* 002C */ float m_fadeOutRate;
    /* 0030 */ float m_fade;
    /* 0034 */ float m_fadeTarget;
    /* 0038 */ uint32_t m_skipOcclusionOnce;
    /* 003C */ float unused_003C[8];
    /* 005C */ uint32_t unused_005C[4];
    /* 006C */ uint32_t m_occlusionPixels;
    /* 0070 */ DNKeyframe m_alphaCurve [4];
    /* 0090 */ float m_scaleMin;
    /* 0094 */ float m_scaleMax;
    /* 0098 */ float m_minDot;
    /* 009C */ float m_alphaMin;
    /* 00A0 */ float m_alphaMax;
    /* 00A4 */ float m_screenGlow;
    /* 00A8 */ DayNight::DNPlanet* m_planet;

    virtual void Update(float deltaSec);
    virtual void Render();
    virtual float GetOcclusionFade();
    virtual float GetCloudDensityFade() = 0;
    virtual float GetLiquidDepthFade();

    DNGlare();
    void Destroy();
    void Initialize(const char* fileName);
    float GetQueryOcclusion();
    float GetPointOcclusion();
};

struct DNSunGlare : public DNGlare {
    float GetCloudDensityFade() override;
};

struct DNMoonGlare : public DNGlare {
    float GetCloudDensityFade() override;
};

void Billboard(C44Matrix& matrix, const C3Vector& direction);
}

namespace DayNight {
struct DNPlanet {
    /* 0000 */ C3Vector m_position;
    /* 000C */ CImVector m_color;
    /* 0010 */ CTexture* m_texture;
    /* 0014 */ float m_scale;
    /* 0018 */ float m_baseScale;
    /* 001C */ float m_period;

    void Destroy();
    void GenGeometry(C3Vector* verts, C2Vector* texCoords, CImVector* colors, uint16_t* indices, uint32_t* vertCount, uint32_t* indexCount);
    void ClipGeometry(C3Vector* verts, C2Vector* texCoords, CImVector* colors, uint16_t* indices, uint32_t* vertCount, uint32_t* indexCount);
    void Initialize(const char* fileName);
    void Render();
};
}

namespace DayNight {
struct DNPlanets {
    /* 0000 */ DayNight::DNPlanet m_sun;
    /* 0020 */ DayNight::DNPlanet m_moon;
    /* 0040 */ DayNight::DNPlanet m_moon2;
    /* 0060 */ uint32_t unused_0060;
    /* 0064 */ uint32_t unused_0064;
    /* 0068 */ uint32_t unused_0068;
    /* 006C */ uint32_t unused_006C;
};
}

namespace DayNight {
struct DNSky {
    /* 0000 */ TSFixedArray<C3Vector> m_geoVerts;
    /* 000C */ TSFixedArray<CImVector> m_clrVerts;
    /* 0018 */ TSFixedArray<uint16_t> m_indices;
    /* 0024 */ uint32_t m_sphThetaTess;
    /* 0028 */ uint16_t m_nVerts;
    /* 002A */ uint16_t m_nIndices;
    /* 002C */ float m_sphRadius;

    void Render();
    void GenSphere(float sphRadius);
    void SetColors();
};
}

namespace DayNight {
struct DNStars {
    /* 0000 */ CM2Scene* m_scene;
    /* 0004 */ CM2Model* m_model;
    /* 0008 */ CImVector m_color;
    /* 000C */ C3Vector m_position;
    /* 0018 */ uint32_t m_lastTimeMs;

    void Destroy();
    void Initialize();
    void Render();
    void Update();
};
}

namespace DayNight {
    float InterpFloatBand(const LightFloatBandRec* rec, int32_t now, int32_t bandIndex);
    float CalcIndividualColorFromDBRec(int32_t time, const LightParamsRec* params, int32_t band);
    void LoadLightingBands(int32_t time, CurrentLight* light, const LightParamsRec* params);
    float CalcFogRate(float fogStart, float fogEnd);
    void CalcColors(int32_t time, CurrentLight* light, const LightParamsRec* params);
    void BlendColors(CurrentLight* dst, const CurrentLight* src, float t);
    const LightParamsRec* GetLightParams(const LightRec* light, int32_t param);
    void SetLightColors();
    DNOverrideSky* GetOverrideSky(const char* path, uint32_t flags);
    void LoadMap(int32_t mapId);
    void LightLoad(TSGrowableArray<const LightRec*>& lights, int32_t mapId);
    void ColorLerpBytes(CImVector* out, const CImVector* a, const CImVector* b, float t);
    void BlendOutputs(CurrentLight* dst, const CurrentLight* src, float t);
    void BlendAreaLightColors(CurrentLight* dst, const LightRec* light, int32_t underwater);
    void BlendAreaLight(CurrentLight* dst, const LightRec* light, int32_t underwater, float maxFade);
    void BlendZoneLight(CurrentLight* dst, const LightRec* light, int32_t underwater, float maxFade, float distPct);
    void DoAreaLights(CurrentLight* light, int32_t underwater);
    void UpdateFog();
    void SetDirection();
    void UpdateLighting();
    void Update(int32_t reset, const C3Vector* cameraPos);
    void SetColors();
    void SetPlanets();
    CImVector DarkenColor(CImVector color, float scale);
    float InterpTable(const C2Vector* table, uint32_t count, float t);
    void SetOverrideFog(float startScalar, float end, CImVector color, int32_t showSky);
    void ClearOverrideFog();
    void SetOverrideLightParamsID(uint32_t id);
    void ClearOverrideLightParamsID();
    void SetFogMode(int32_t mode);
    void UnloadMap();
    void ClearLightFlag(int32_t bit);
    void ResetFogAccumulators(bool updateClouds);
    void InitSunGlare();
    void InitMoonGlare();
    void FindAndAddZoneLight(const C3Vector* position);
    void SyncSkyAnimWithTime(DNOverrideSky* sky, int32_t minutes);
    void DrawSky(DNOverrideSky* sky, float weight);
    int32_t GetFogOverride();
    void RenderGlares();
    void SetBlendSky(uint32_t index, const char* sky, int32_t a3, float blend);
    void UpdateStars();
    void AddLightOverride(int32_t lightId, int32_t overrideId, uint32_t duration);
    void RemoveLightOverride(int32_t lightId, uint32_t duration);
    void RenderSky(CPortalView* view);
}

struct SMOFog {
    struct Fog {
        /* 0000 */ float m_end;
        /* 0004 */ float m_startScalar;
        /* 0008 */ CImVector m_color;
    };

    /* 0000 */ uint32_t m_flags;
    /* 0004 */ C3Vector m_position;
    /* 0010 */ float m_smallerRadius;
    /* 0014 */ float m_largerRadius;
    /* 0018 */ Fog m_fog;
    /* 0024 */ Fog m_underwaterFog;
};

namespace DayNight {
    void ApplyFogSettings(const SMOFog::Fog* fog);
}

namespace DayNight {
struct AreaLightOverride {
    /* 0000 */ uint32_t m_id;
    /* 0004 */ LightRec* m_light;
    /* 0008 */ float m_fade;
    /* 000C */ int32_t m_state;
    /* 0010 */ uint32_t m_lastTime;
    /* 0014 */ uint32_t m_duration;
};

class LightQE {
    public:
    static int32_t s_localSort;

    static bool HasHigherPriority(const LightQE& a, const LightQE& b);

    /* 0000 */ float m_key;
    /* 0004 */ const LightRec** m_light;
};

class LightQueue : public CMemBlock {
    public:
    /* 0014 */ uint32_t m_chunk;
    /* 0018 */ uint32_t m_capacity;
    /* 001C */ uint32_t m_count;

    LightQueue(const uint32_t* params);
    void Erase(uint32_t index, uint32_t count);
    LightQE Pop();
    void Push(LightQE entry);
};

class GlareBase {
    public:
    static int32_t m_masterEnable;
};

extern int32_t s_mapId;
extern int32_t g_forceFullUpdate;
extern int32_t s_lightFlags;
extern float s_glowBlend;
extern DNStars g_stars;
extern DNSky g_sky;
extern DNClouds g_clouds;
extern DNPlanets g_planets;
extern DNSunGlare g_sunGlare;
extern DNMoonGlare g_moonGlare;
extern TSGrowableArray<const LightRec*> g_areaLights;
extern TSHashTable<DNOverrideSky, HASHKEY_STRI> g_overrideSky;
}

namespace DayNight {
// INLINED
inline CImVector InterpColor(const CImVector& a, const CImVector& b, float t) {
    CImVector color = a;
    uint8_t fraction = CMath::fint_(t * 255.0f);

    if (fraction) {
        color.InterpColor(fraction, &b);
    }

    return color;
}
}

#endif
