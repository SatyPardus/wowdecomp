#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <typeinfo>
#include "Engine/Source/Services/ParticleSystem2.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxPool.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Data.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Services/GfxSingletonManager.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C33Matrix.h"
#include "Engine/Source/Tempest/C4Vector.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRange.h"

const float CParticleEmitter2::VEL_UPDATE_TIME = 0.033333335f;
const float CParticleEmitter2::MIN_ZSOURCE = 0.001f;

static const float s_maxTimeStep = 0.1f;

// STATIC: 0x00B2D530
static int32_t s_particleRenderEnable = 1;

// STATIC: 0x00B2D534
static float s_stepRound = 0.5f;

// STATIC: 0x00B2D540
static C3Vector s_particleNormal(0.0f, 0.0f, 1.0f);

// STATIC: 0x00B2D550
static C44Matrix s_particleXform(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

// STATIC: 0x00B2D590
static C33Matrix s_billboardXform(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

// STATIC: 0x00B2D5B4
static const float s_cornerOffsets[4][2] = { { -1.0f, 1.0f }, { -1.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f } };

// STATIC: 0x00B2D5D4
static const float s_cornerTexCoords[4][2] = { { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } };

// STATIC: 0x00DCE680
uint32_t CParticleEmitter2::s_initCount;

// STATIC: 0x00DCE684
CGxBuf* CParticleEmitter2::s_indexBuf;

// STATIC: 0x00DCE688
CGxPool* CParticleEmitter2::s_indexPool;

// STATIC: 0x00DCE68C
float CParticleEmitter2::s_viewDist;

// STATIC: 0x00DCE690
float CParticleEmitter2::s_rndTable[128];

// STATIC: 0x00DCE890
uint8_t CParticleEmitter2::s_noIndexBuffer;

static const uint32_t s_pqParams[2] = { 32, 32 };

// STATIC: 0x00DCE894
CSortableParticleQueue CParticleEmitter2::s_pq(s_pqParams);

// STATIC: 0x00DCE8C4
void* ParticleSystemManager::sm_projectParam;

// STATIC: 0x00DCE8C8
int32_t (*ParticleSystemManager::sm_projectCallback)(const C3Vector&, float&, void*);

// VERIFY: c5ae3283 manual
// OFFSET: 0x00979330
static uint32_t PartTrackSearch(const M2Array<fixed16>& times, float t) {
    uint32_t lo = 0;
    uint32_t hi = times.m_count - 1;

    if (times.m_count == 1) {
        return 0;
    }

    while (1) {
        uint32_t mid = (hi + lo) >> 1;

        if (times[mid].m_value * 0.000030518509f <= t) {
            lo = mid + 1;

            if (mid + 1 >= times.m_count - 1 || times[mid + 1].m_value * 0.000030518509f > t) {
                return mid;
            }
        } else {
            hi = mid - 1;
        }

        if (lo >= hi) {
            return lo;
        }
    }
}

// VERIFY: e650aa30 manual
// OFFSET: 0x009793B0
static float PartTrackKeys(const M2Array<fixed16>& times, float t, uint32_t& key0, uint32_t& key1) {
    if (times.m_count == 2) {
        key0 = 0;
        key1 = 1;
        return t;
    }

    if (times.m_count == 3) {
        float mid = times[1].m_value * 0.000030518509f;

        if (t >= mid) {
            key0 = 1;
            key1 = 2;
            return (t - mid) / (1.0f - mid);
        }

        key0 = 0;
        key1 = 1;
        return t / mid;
    }

    key0 = PartTrackSearch(times, t);
    key1 = key0 + 1;

    return (t - times[key0].m_value * 0.000030518509f) / (0.000030518509f * (times[key1].m_value - times[key0].m_value));
}

// VERIFY: 91ba216a auto
// OFFSET: 0x00979480
static C2Vector __stdcall PartTrackSample(const M2PartTrack<C2Vector>& track, float t) {
    C2Vector result;

    if (track.m_values.m_count == 1) {
        result = track.m_values[0];
        return result;
    }

    uint32_t key0;
    uint32_t key1;
    float blend = PartTrackKeys(track.m_times, t, key0, key1);

    result.x = (track.m_values[key1].x - track.m_values[key0].x) * blend + track.m_values[key0].x;
    result.y = blend * (track.m_values[key1].y - track.m_values[key0].y) + track.m_values[key0].y;

    return result;
}

// VERIFY: bae87e8d auto
// OFFSET: 0x009794F0
static float __stdcall PartTrackSample(const M2PartTrack<fixed16>& track, float t) {
    if (track.m_values.m_count == 1) {
        return track.m_values[0].m_value * 0.000030518509f;
    }

    uint32_t key0;
    uint32_t key1;
    float blend = PartTrackKeys(track.m_times, t, key0, key1);

    return blend * (0.000030518509f * track.m_values[key1].m_value - track.m_values[key0].m_value * 0.000030518509f) + track.m_values[key0].m_value * 0.000030518509f;
}

// VERIFY: 3e432a29 auto
// OFFSET: 0x00979560
static int32_t __stdcall PartTrackSample(const M2PartTrack<uint16_t>& track, float t) {
    if (track.m_values.m_count == 1) {
        return track.m_values[0];
    }

    uint32_t key0;
    uint32_t key1;
    float blend = PartTrackKeys(track.m_times, t, key0, key1);

    return static_cast<int32_t>(CMath::fint_(blend * (track.m_values[key1] - track.m_values[key0]) + track.m_values[key0]));
}

// VERIFY: 495193f9 manual
// OFFSET: 0x0097D7B0
static uint32_t PopIndex(TSGrowableArray<uint32_t>& array) {
    uint32_t value = *array.Top();
    array.SetCount(array.Count() - 1);

    return value;
}

// INLINED
static inline void AddVertex(CAaBox& bounds, CParticleEmitter2::ParticleVertex& vertex, const C3Vector& position, const CImVector& color, const C2Vector& texCoord) {
    *reinterpret_cast<C3Vector*>(vertex.m_position) = position;

    if (bounds.b.x > position.x) {
        bounds.b.x = position.x;
    }

    if (bounds.b.y > position.y) {
        bounds.b.y = position.y;
    }

    if (position.z < bounds.b.z) {
        bounds.b.z = position.z;
    }

    if (bounds.t.x < position.x) {
        bounds.t.x = position.x;
    }

    if (position.y > bounds.t.y) {
        bounds.t.y = position.y;
    }

    if (position.z > bounds.t.z) {
        bounds.t.z = position.z;
    }

    *reinterpret_cast<C3Vector*>(vertex.m_normal) = s_particleNormal;
    *reinterpret_cast<CImVector*>(vertex.m_color) = color;
    *reinterpret_cast<C2Vector*>(vertex.m_texCoord) = texCoord;

    vertex.m_position += vertex.m_positionStride;
    vertex.m_normal += vertex.m_normalStride;
    vertex.m_color += vertex.m_colorStride;
    vertex.m_texCoord += vertex.m_texCoordStride;
    vertex.m_count++;
}

// VERIFY: 645e971f auto
// OFFSET: 0x00978B30
void CParticleEmitter2::SetModel(CM2Scene* scene, const char* name) {
    if (scene && name && *name) {
        this->m_scene = scene;
        this->m_model = scene->CreateModel(name, 0);

        if (this->m_model) {
            this->m_params.m_hasModel = CParticleEmitter2::PT_MODEL;
        }
    }
}

// VERIFY: 0a4f2f59 auto
// OFFSET: 0x00978BE0
CAaBox& CParticleEmitter2::WorldBounds() {
    return this->m_worldBounds;
}

// VERIFY: 377f8251 auto
// OFFSET: 0x00978BF0
void CParticleEmitter2::SetMaterial(const CParticleMat& material, CTexture* texture) {
    if (this->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
    }

    this->m_texture = reinterpret_cast<CTexture*>(HandleDuplicate(reinterpret_cast<HOBJECT>(texture)));
    this->m_params.m_materialBlend = material.m_blendMode;
    this->m_params.m_materialFlags = material.m_flags;
}

// VERIFY: 9ef515a2 auto
// OFFSET: 0x00978C40
void CParticleEmitter2::SetTexture(CTexture* texture) {
    if (this->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
    }

    this->m_texture = reinterpret_cast<CTexture*>(HandleDuplicate(reinterpret_cast<HOBJECT>(texture)));
}

// VERIFY: a4b2b0e5 manual
// OFFSET: 0x00978C70
void CParticleEmitter2::SetTextureDimensions(uint32_t rows, uint32_t cols) {
    if ((rows & (rows - 1)) || (cols & (cols - 1)) || !rows || !cols) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    this->m_textureRows = rows;
    this->m_textureCols = cols;
    this->m_textureCellShift = -1;

    for (uint32_t i = cols; i; i >>= 1) {
        this->m_textureCellShift++;
    }

    this->m_textureCellWidth = 1.0f / this->m_textureCols;
    this->m_textureCellHeight = 1.0f / rows;
}

// VERIFY: 0d1ca425 manual
// OFFSET: 0x00978D00
void CParticleEmitter2::SetParticleStyle(int32_t head, int32_t tail, float tailLength, int32_t clampTail) {
    if (head) {
        this->m_flags |= 0x4;
    } else {
        this->m_flags &= ~0x4;
    }

    if (tail) {
        this->m_flags |= 0x8;
    } else {
        this->m_flags &= ~0x8;
    }

    if (clampTail) {
        this->m_flags |= 0x20000;
    } else {
        this->m_flags &= ~0x20000;
    }

    this->m_vertsPerParticle = 0;
    this->m_indicesPerParticle = 0;
    this->m_params.m_tailLength = tailLength;

    if (this->m_flags & 0x4) {
        this->m_vertsPerParticle = 4;
        this->m_indicesPerParticle = 6;
    }

    if (this->m_flags & 0x8) {
        this->m_vertsPerParticle += 4;
        this->m_indicesPerParticle += 6;
    }
}

// VERIFY: be707124 auto
// OFFSET: 0x00978DA0
void CParticleEmitter2::SetZsource(float zsource) {
    this->m_params.m_zsource = zsource;

    if (fabs(zsource) < MIN_ZSOURCE) {
        this->m_params.m_zsource = 0.0f;
    }
}

// VERIFY: bfe3df98 manual
// OFFSET: 0x00978DD0
void CParticleEmitter2::SetFollowParams(float speed1, float scale1, float speed2, float scale2) {
    float speedDelta = speed2 - speed1;

    if (fabs(speedDelta) < 0.00000023841858f) {
        this->m_followSlope = 0.0f;
        this->m_followIntercept = 0.0f;
        return;
    }

    float slope = (scale2 - scale1) / speedDelta;
    this->m_followSlope = slope;
    this->m_followIntercept = scale1 - speed1 * slope;
}

// VERIFY: 0f5748cd auto
// OFFSET: 0x00978E30
void CParticleEmitter2::SetChooseRandomTexture(int32_t choose) {
    if (this->m_textureRows * this->m_textureCols > 1 && choose) {
        this->m_flags |= 0x100000;
    } else {
        this->m_flags &= ~0x100000;
    }
}

// VERIFY: 46acd383 auto
// OFFSET: 0x00978E70
CParticleEmitter2* CParticleEmitter2::AddRef() {
    this->m_refCount++;

    return this;
}

// VERIFY: 4e4248e4 manual
// OFFSET: 0x00978E80
void CParticleEmitter2::DecRef() {
    if (this->m_refCount-- == 1) {
        delete this;
    }
}

// VERIFY: c8cd89a9 manual
// OFFSET: 0x00979170
void CParticleEmitter2::Init() {
    if (!CParticleEmitter2::s_initCount) {
        if (CParticleEmitter2::s_noIndexBuffer) {
            CParticleEmitter2::s_initCount++;
            CParticleEmitter2::s_indexPool = nullptr;
            CParticleEmitter2::s_indexBuf = nullptr;
            return;
        }

        CParticleEmitter2::s_indexPool = g_theGxDevicePtr->PoolCreate(GxPoolTarget_Index, GxPoolUsage_Static, sizeof(uint16_t) * 0x1FFF8, GxPoolHintBit_Unk2, "CParticleEmitter2_idx");
        CParticleEmitter2::s_indexBuf = g_theGxDevicePtr->BufCreate(CParticleEmitter2::s_indexPool, sizeof(uint16_t), 0x1FFF8, 0);
    }

    CParticleEmitter2::s_initCount++;
}

// VERIFY: 122129b6 auto
// OFFSET: 0x009791E0
void CParticleEmitter2::Destroy() {
    if (--CParticleEmitter2::s_initCount) {
        return;
    }

    if (CParticleEmitter2::s_indexBuf) {
        CGxBuf* buf = CParticleEmitter2::s_indexBuf;

        if (buf != g_theGxDevicePtr->BufStream(CParticleEmitter2::s_indexBuf->m_pool->m_target, 0, 0)) {
            g_theGxDevicePtr->BufDestroy(buf);
        }
    }

    if (CParticleEmitter2::s_indexPool) {
        g_theGxDevicePtr->PoolDestroy(CParticleEmitter2::s_indexPool);
    }
}

// VERIFY: 07fad2af manual
// OFFSET: 0x009792D0
float CParticleEmitter2::CalcVelocity() {
    return (CRandom::sfloat_(this->m_random) * this->m_params.m_variation + 1.0f) * this->m_params.m_speed;
}

// VERIFY: d4eba96e manual
// OFFSET: 0x009795D0
CImVector CParticleEmitter2::InterpolateColorTrack(float t) {
    const M2PartTrack<C3Vector>* track = this->m_params.m_colorTrack;
    CImVector color;

    if (track->m_values.m_count == 1) {
        color.b = static_cast<uint8_t>(CMath::fint_(track->m_values[0].z));
        color.g = static_cast<uint8_t>(CMath::fint_(track->m_values[0].y));
        color.r = static_cast<uint8_t>(CMath::fint_(track->m_values[0].x));
        color.a = 0xFF;
        return color;
    }

    uint32_t key0;
    uint32_t key1;
    float blend = PartTrackKeys(track->m_times, t, key0, key1);

    float r;
    float g;
    float b;

    if (this->m_flags & 0x10) {
        r = (this->m_params.m_replacementColors[key1].x - this->m_params.m_replacementColors[key0].x) * blend + this->m_params.m_replacementColors[key0].x;
        g = (this->m_params.m_replacementColors[key1].y - this->m_params.m_replacementColors[key0].y) * blend + this->m_params.m_replacementColors[key0].y;
        b = blend * (this->m_params.m_replacementColors[key1].z - this->m_params.m_replacementColors[key0].z) + this->m_params.m_replacementColors[key0].z;
    } else {
        const C3Vector& value0 = track->m_values[key0];
        const C3Vector& value1 = track->m_values[key1];
        r = (value1.x - value0.x) * blend + value0.x;
        g = (value1.y - value0.y) * blend + value0.y;
        b = blend * (value1.z - value0.z) + value0.z;
    }

    color.b = static_cast<uint8_t>(CMath::fint_(b));
    color.a = 0xFF;
    color.r = static_cast<uint8_t>(CMath::fint_(r));
    color.g = static_cast<uint8_t>(CMath::fint_(g));

    return color;
}

// VERIFY: 0e21dad3 auto
// OFFSET: 0x00979740
void CParticleEmitter2::ProjectParticle(CParticle2& particle) {
    float height;

    if (!ParticleSystemManager::sm_projectCallback(particle.m_position, height, ParticleSystemManager::sm_projectParam)) {
        return;
    }

    float life = particle.m_lifeSeed.m_value * this->m_params.m_lifeVariation * 0.000030518509f + this->m_params.m_life;

    if (life <= 0.001f) {
        life = 0.001f;
    }

    C2Vector scale = PartTrackSample(*this->m_params.m_scaleTrack, particle.m_age / life);

    if (scale.x <= scale.y) {
        particle.m_position.z = scale.y + height;
    } else {
        particle.m_position.z = scale.x + height;
    }
}

// VERIFY: 04d61ed6 manual
// OFFSET: 0x00979870
void CParticleEmitter2::CreateParticle(CParticle2& particle, float time, const C44Matrix& transform) {
    float age = CRandom::ufloat_(this->m_random) * time;
    float lifeSeed = CRandom::sfloat_(this->m_random);

    particle.m_lifeSeed = fixed16(lifeSeed);

    if (age < 0.0f) {
        age = 0.0f;
    }

    particle.m_age = age;
    particle.m_randomSeed = static_cast<uint16_t>(CRandom::uint32_(this->m_random));
    particle.m_position.x = 0.0f;
    particle.m_position.y = 0.0f;
    particle.m_position.z = 0.0f;

    if (!(this->m_flags & 0x200)) {
        particle.m_position *= transform;

        if (this->m_flags & 0x40000) {
            this->ProjectParticle(particle);
        }
    }

    float velocity = this->CalcVelocity();
    C3Vector direction = CRandom::C3Vector_(this->m_random);

    particle.m_velocity.x = direction.x * velocity + this->m_parentVelocity.x;
    particle.m_velocity.y = direction.y * velocity + this->m_parentVelocity.y;
    particle.m_velocity.z = velocity * direction.z + this->m_parentVelocity.z;

    particle.m_velocity.FlushDenormals();
}

// VERIFY: e014e81b manual
// OFFSET: 0x009799C0
void CParticleEmitter2::CreateParticle(CParticle2_Model& particle, float time, const C44Matrix& transform) {
    this->CreateParticle(static_cast<CParticle2&>(particle), time, transform);

    if (!(this->m_flags & 0x200)) {
        C33Matrix rotation(transform);
        C3Vector axis(rotation.a0, rotation.a1, rotation.a2);
        float lengthSq = axis.y * axis.y + axis.z * axis.z + axis.x * axis.x;

        if (lengthSq > 0.00000023841858f) {
            float scale = 1.0f / sqrt(lengthSq);
            rotation.a0 = axis.x * scale;
            rotation.a1 = axis.y * scale;
            rotation.a2 = scale * axis.z;
        }

        particle.m_orientation.FromRotationMatrix(rotation);
        reinterpret_cast<C4Vector&>(particle.m_orientation).NormalizeSafe();
    }

    float tumbleZ = CRandom::ufloat_(this->m_random);
    float tumbleY = CRandom::ufloat_(this->m_random);

    particle.m_angularVelocity.x = CRandom::ufloat_(this->m_random) * this->m_tumbleSpanX + this->m_tumbleBaseX;
    particle.m_angularVelocity.y = (tumbleY + 1.0f) * this->m_tumbleSpanY;
    particle.m_angularVelocity.z = (tumbleZ + 1.0f) * this->m_tumbleSpanZ;

    if (this->m_flags & 0x10000) {
        float signZ = (CRandom::uint32_(this->m_random) & 1) ? 1.0f : -1.0f;
        float signY = (CRandom::uint32_(this->m_random) & 1) ? 1.0f : -1.0f;
        float signX = (CRandom::uint32_(this->m_random) & 1) ? 1.0f : -1.0f;

        particle.m_angularVelocity.x = signX * particle.m_angularVelocity.x;
        particle.m_angularVelocity.y = particle.m_angularVelocity.y * signY;
        particle.m_angularVelocity.z = particle.m_angularVelocity.z * signZ;
    }

    if (!particle.m_model) {
        particle.m_model = this->m_scene->CreateModel(this->m_model->m_shared->m_fileName, 0);
    }
}

// VERIFY: 642edc1f manual
// OFFSET: 0x00979BB0
int32_t CParticleEmitter2::MoveParticle(CParticle2& particle, float deltaTime) {
    if (this->m_windTime > particle.m_age) {
        particle.m_velocity.x = this->m_windVectorX * deltaTime + particle.m_velocity.x;
        particle.m_velocity.y = this->m_windVectorY * deltaTime + particle.m_velocity.y;
        particle.m_velocity.z = deltaTime * this->m_windVectorZ + particle.m_velocity.z;

        particle.m_velocity.FlushDenormals();
    }

    if ((this->m_flags & 0x80000) && deltaTime + deltaTime < particle.m_age) {
        particle.m_position.x = this->m_burstOffset.x + particle.m_position.x;
        particle.m_position.y = this->m_burstOffset.y + particle.m_position.y;
        particle.m_position.z = this->m_burstOffset.z + particle.m_position.z;
    }

    float moveX = particle.m_velocity.x * deltaTime;
    float moveY = particle.m_velocity.y * deltaTime;
    float moveZ = particle.m_velocity.z * deltaTime;

    particle.m_position.x = moveX + particle.m_position.x;
    particle.m_position.y = moveY + particle.m_position.y;
    particle.m_position.z = moveZ + particle.m_position.z - this->m_params.m_gravity * deltaTime * deltaTime * 0.5f;
    particle.m_velocity.z = particle.m_velocity.z - this->m_params.m_gravity * deltaTime;

    if (0.0f != this->m_drag) {
        float drag = deltaTime * this->m_drag;

        if (drag > 1.0f) {
            drag = 1.0f;
        }

        particle.m_velocity.x = particle.m_velocity.x - drag * particle.m_velocity.x;
        particle.m_velocity.y = particle.m_velocity.y - particle.m_velocity.y * drag;
        particle.m_velocity.z = particle.m_velocity.z - drag * particle.m_velocity.z;
    }

    particle.m_velocity.FlushDenormals();

    if (!(this->m_flags & 0x1000)) {
        return 1;
    }

    float dot;
    float x;

    if (this->m_flags & 0x200) {
        dot = particle.m_position.z * moveZ + particle.m_position.y * moveY;
        x = particle.m_position.x;
    } else {
        x = particle.m_position.x - this->m_xform.d0;
        dot = (particle.m_position.z - this->m_xform.d2) * moveZ + (particle.m_position.y - this->m_xform.d1) * moveY;
    }

    if (dot + x * moveX > 0.0f) {
        return 0;
    }

    return 1;
}

// VERIFY: ff80f546 manual
// OFFSET: 0x00979D60
void CParticleEmitter2::InterpolateAllTracksSimple(const CParticle2& particle, CImVector* color, C2Vector* scale, int32_t* headCell, int32_t* tailCell) {
    float life = 0.001f;

    if (this->m_params.m_life > 0.001f) {
        life = this->m_params.m_life;
    }

    float t = particle.m_age / life;
    const SimpleKey* key = this->m_params.m_simpleKeys;

    if (t >= this->m_params.m_simpleMidTime) {
        key++;
        t = (t - this->m_params.m_simpleMidTime) / (1.0f - this->m_params.m_simpleMidTime);
    } else {
        t = t / this->m_params.m_simpleMidTime;
    }

    color->a = static_cast<uint8_t>(CMath::fint_((t * key->m_deltaA + key->m_color.a) * this->m_alphaScale));
    color->r = static_cast<uint8_t>(CMath::fint_(key->m_deltaR * t + key->m_color.r));
    color->g = static_cast<uint8_t>(CMath::fint_(key->m_deltaG * t + key->m_color.g));
    color->b = static_cast<uint8_t>(CMath::fint_(key->m_deltaB * t + key->m_color.b));

    float size = key->m_deltaScale * t + key->m_scale;
    scale->x = size;
    scale->y = size;

    *headCell = static_cast<int32_t>(CMath::fint_(t * key->m_deltaHeadCell + key->m_headCell));
    *tailCell = static_cast<int32_t>(CMath::fint_(key->m_deltaTailCell * t + key->m_tailCell));
}

// VERIFY: ab700f4c manual
// OFFSET: 0x00979E90
void CParticleEmitter2::InterpolateAllTracks(const CParticle2& particle, CImVector* color, C2Vector* scale, int32_t* headCell, int32_t* tailCell) {
    float life = particle.m_lifeSeed.m_value * this->m_params.m_lifeVariation * 0.000030518509f + this->m_params.m_life;

    if (life <= 0.001f) {
        life = 0.001f;
    }

    float t = particle.m_age / life;
    CRndSeed seed(particle.m_randomSeed);

    *color = this->InterpolateColorTrack(t);
    color->a = static_cast<uint8_t>(CMath::fint_(PartTrackSample(*this->m_params.m_alphaTrack, t) * this->m_alphaScale * 255.0f));
    *scale = PartTrackSample(*this->m_params.m_scaleTrack, t);

    *headCell = 0;
    *tailCell = 0;

    if (this->m_params.m_headCellTrack->m_times.m_count) {
        *headCell = PartTrackSample(*this->m_params.m_headCellTrack, t);
    } else if (this->m_flags & 0x100000) {
        *headCell = CRandom::dice_(this->m_textureRows * this->m_textureCols, seed);
    }

    if (this->m_params.m_tailCellTrack->m_times.m_count) {
        *tailCell = PartTrackSample(*this->m_params.m_tailCellTrack, t);
    }

    if (this->m_flags & 0x800000) {
        float variationY = CRandom::sfloat_(seed);
        float scaleX = CRandom::sfloat_(seed) * this->m_params.m_scaleVariationX + 1.0f;
        float scaleY = this->m_params.m_scaleVariationY * variationY + 1.0f;

        if (scaleY <= 0.000099999997f) {
            scaleY = 0.000099999997f;
        }

        if (scaleX <= 0.000099999997f) {
            scale->x = 0.000099999997f * scale->x;
        } else {
            scale->x = scaleX * scale->x;
        }

        scale->y = scaleY * scale->y;
    } else {
        float scaleXY = CRandom::sfloat_(seed) * this->m_params.m_scaleVariationX + 1.0f;

        if (scaleXY < 0.000099999997f) {
            scaleXY = 0.000099999997f;
        }

        scale->x = scale->x * scaleXY;
        scale->y = scaleXY * scale->y;
    }
}

// VERIFY: c107c23c manual
// OFFSET: 0x0097A130
void CParticleEmitter2::RandomizeSpin(const CParticle2& particle, float* spin, float* spinRate) {
    if (0.0f == this->m_params.m_initialSpinVariation && 0.0f == this->m_params.m_spinVariation) {
        *spin = this->m_params.m_initialSpin;
        *spinRate = this->m_params.m_spin;
        return;
    }

    CRndSeed seed(particle.m_randomSeed);

    if (0.0f == this->m_params.m_initialSpinVariation) {
        *spin = this->m_params.m_initialSpin;
    } else {
        *spin = CRandom::sfloat_(seed) * this->m_params.m_initialSpinVariation + this->m_params.m_initialSpin;
    }

    if (0.0f == this->m_params.m_spinVariation) {
        *spinRate = this->m_params.m_spin;
    } else {
        *spinRate = CRandom::sfloat_(seed) * this->m_params.m_spinVariation + this->m_params.m_spin;
    }
}

// VERIFY: 21e8a355 manual
// OFFSET: 0x0097A260
void __stdcall CParticleEmitter2::RenderIndices(CGxBuf* buf) {
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(buf));
    uint16_t* end = indices + 0x1FFF8;
    uint16_t vertex = 0;

    while (indices < end) {
        indices[0] = vertex;
        indices[1] = vertex + 1;
        indices[2] = vertex + 2;
        indices[3] = vertex + 3;
        indices[4] = vertex + 2;
        indices[5] = vertex + 1;
        indices += 6;
        vertex += 4;
    }

    g_theGxDevicePtr->BufUnlock(buf, 0);
    buf->m_hasData = 1;
}

// VERIFY: 7d50dd44 manual
// OFFSET: 0x0097A2E0
void CParticleEmitter2::FillOutParticleVertex(void* buffer, EGxVertexBufferFormat format, ParticleVertex* vertex) {
    uint32_t stride = GxVertexSize(format);

    vertex->m_position = static_cast<uint8_t*>(buffer) + GxVertexAttribOffset(format, GxVA_Position);
    vertex->m_positionStride = stride;

    if (this->m_params.m_materialFlags & 0x1) {
        vertex->m_normalStride = stride;
        vertex->m_normal = static_cast<uint8_t*>(buffer) + GxVertexAttribOffset(format, GxVA_Normal);
    } else {
        static C3Vector s_zeroNormal(0.0f, 0.0f, 0.0f);
        vertex->m_normal = reinterpret_cast<uint8_t*>(&s_zeroNormal);
        vertex->m_normalStride = 0;
    }

    vertex->m_color = static_cast<uint8_t*>(buffer) + GxVertexAttribOffset(format, GxVA_Color0);
    vertex->m_colorStride = stride;
    vertex->m_texCoordStride = stride;
    vertex->m_texCoord = static_cast<uint8_t*>(buffer) + GxVertexAttribOffset(format, GxVA_TexCoord0);
}

// VERIFY: 5ce9bae8 manual
// OFFSET: 0x0097A390
void CParticleEmitter2::RenderParticlesPrep(const C44Matrix* frameOfReference, const C44Matrix& view) {
    C44Matrix translate;
    translate.d0 = -this->m_viewPos.x;
    translate.d1 = -this->m_viewPos.y;
    translate.d2 = -this->m_viewPos.z;

    if (this->m_flags & 0x200) {
        s_particleXform = this->m_xform * translate * view;
    } else if (frameOfReference) {
        s_particleXform = *frameOfReference * translate * view;
    } else {
        s_particleXform = translate * view;
    }

    s_particleNormal.x = view.c0;
    s_particleNormal.y = view.c1;
    s_particleNormal.z = view.c2;

    if (!(this->m_flags & 0x4000)) {
        return;
    }

    s_billboardXform = C33Matrix(s_particleXform);

    if ((this->m_flags & 0x200) && fabs(this->m_xformScale) >= 0.00000023841858f) {
        s_billboardXform.Scale(1.0f / this->m_xformScale);
    }

    this->m_billboardAxis.x = s_billboardXform.c0;
    this->m_billboardAxis.y = s_billboardXform.c1;
    this->m_billboardAxis.z = s_billboardXform.c2;

    float lengthSq = this->m_billboardAxis.z * this->m_billboardAxis.z + this->m_billboardAxis.y * this->m_billboardAxis.y + this->m_billboardAxis.x * this->m_billboardAxis.x;

    if (lengthSq > 0.00000023841858f) {
        float scale = 1.0f / sqrt(lengthSq);
        this->m_billboardAxis.x = this->m_billboardAxis.x * scale;
        this->m_billboardAxis.y = this->m_billboardAxis.y * scale;
        this->m_billboardAxis.z = scale * this->m_billboardAxis.z;
    }
}

// VERIFY: d76443ea auto
// OFFSET: 0x0097A580
void __stdcall CParticleEmitter2::RenderParticleVertices(CGxBuf* buf, EGxVertexBufferFormat format, uint32_t vertexCount, uint32_t indexCount) {
    if (!s_particleRenderEnable) {
        return;
    }

    if (!CParticleEmitter2::s_indexBuf->m_hasData || !CParticleEmitter2::s_indexBuf->m_valid) {
        CParticleEmitter2::RenderIndices(CParticleEmitter2::s_indexBuf);
    }

    C44Matrix identity;
    g_theGxDevicePtr->XformSetView(identity);

    GxPrimVertexPtr(buf, format);
    g_theGxDevicePtr->PrimIndexPtr(CParticleEmitter2::s_indexBuf);

    CShaderEffect::SetTexMtx_Identity(0);
    CShaderEffect::SetDefaultShaders(0);
    CShaderEffect::UpdateWorldViewMatrix();

    CGxBatch batch;
    batch.m_primType = GxPrim_Triangles;
    batch.m_start = 0;
    batch.m_count = indexCount;
    batch.m_minIndex = 0;
    batch.m_maxIndex = vertexCount - 1;
    g_theGxDevicePtr->Draw(&batch, 1);
}

// VERIFY: a9b4f93f manual
// OFFSET: 0x0097A670
int32_t CParticleEmitter2::RenderParticle(CParticle2_Model& particle, const C44Matrix* frameOfReference) {
    uint32_t twinkle = 0;

    if (this->m_twinkleOnOff < 1.0f || 0.0f != this->m_twinkleScaleSpan) {
        twinkle = (static_cast<uint8_t>(CMath::fint_(this->m_twinkleFPS * particle.m_age)) + (reinterpret_cast<uintptr_t>(&particle) >> 5)) & 0x7F;
    }

    if (this->m_twinkleOnOff < 1.0f && this->m_twinkleOnOff < CParticleEmitter2::s_rndTable[twinkle]) {
        CM2Model* model = particle.m_model;

        if (model->m_attachParent) {
            model->m_fFlags &= ~0x80;
        } else {
            model->m_fFlags &= ~0x8;
        }

        if (model->m_attachParent) {
            model->m_fFlags &= ~0x20000;
        } else {
            model->m_fFlags &= ~0x10000;
        }

        return 0;
    }

    float life = particle.m_lifeSeed.m_value * this->m_params.m_lifeVariation * 0.000030518509f + this->m_params.m_life;

    if (life <= 0.001f) {
        life = 0.001f;
    }

    float t = particle.m_age / life;
    CImVector color = this->InterpolateColorTrack(t);
    float alpha = PartTrackSample(*this->m_params.m_alphaTrack, t) * this->m_alphaScale * 255.0f;
    uint8_t alphaByte = static_cast<uint8_t>(CMath::fint_(alpha));
    C2Vector scale = PartTrackSample(*this->m_params.m_scaleTrack, t);

    float twinkleScale = CParticleEmitter2::s_rndTable[twinkle] * this->m_twinkleScaleSpan + this->m_twinkleScaleBase;
    scale.x = scale.x * twinkleScale;
    scale.y = twinkleScale * scale.y;

    if (!(this->m_flags & 0x200) && (this->m_flags & 0x400)) {
        scale.x = scale.x * this->m_xformScale;
        scale.y = this->m_xformScale * scale.y;
    }

    C44Matrix transform(particle.m_orientation);
    transform.Scale(C3Vector(scale.x, scale.y, (scale.x + scale.y) * 0.5f));
    transform.d0 = particle.m_position.x;
    transform.d1 = particle.m_position.y;
    transform.d2 = particle.m_position.z;

    if (this->m_flags & 0x200) {
        transform *= this->m_xform;
    } else if (frameOfReference) {
        transform *= *frameOfReference;
    }

    CM2Model* model = particle.m_model;
    model->m_fFlags |= 0x8000;
    model->m_worldTransform = transform;

    model = particle.m_model;
    model->m_diffuseTintR = color.r * 0.0039215689f;
    model->m_diffuseTintG = color.g * 0.0039215689f;
    model->m_diffuseTintB = color.b * 0.0039215689f;
    particle.m_model->m_fadeAlpha = 0.0039215689f * alphaByte;

    model = particle.m_model;

    if (model->m_attachParent) {
        model->m_fFlags |= 0x80;
    } else {
        model->m_fFlags |= 0x8;
    }

    if (model->m_attachParent) {
        model->m_fFlags |= 0x20000;
    } else {
        model->m_fFlags |= 0x10000;
    }

    if (particle.m_model->IsLoaded(0, 0)) {
        CAaSphere sphere = particle.m_model->GetBoundingSphere();
        sphere.c *= transform;

        float radius = scale.y;

        if (scale.x > scale.y) {
            radius = scale.x;
        }

        sphere.r = radius * sphere.r;
        this->m_worldBounds |= sphere.Box();
    }

    return 1;
}

// VERIFY: 3b69063c manual
// OFFSET: 0x0097A990
void CParticleEmitter2::SetParticleColors(const CImVector& color0, const CImVector& color1, const CImVector& color2) {
    this->m_params.m_replacementColors[0].x = color0.r;
    this->m_params.m_replacementColors[0].y = color0.g;
    this->m_params.m_replacementColors[0].z = color0.b;
    this->m_params.m_replacementColors[1].x = color1.r;
    this->m_params.m_replacementColors[1].y = color1.g;
    this->m_params.m_replacementColors[1].z = color1.b;
    this->m_params.m_replacementColors[2].x = color2.r;
    this->m_params.m_replacementColors[2].y = color2.g;
    this->m_params.m_replacementColors[2].z = color2.b;

    this->m_flags |= 0x10;

    if (!(this->m_flags & 0x1000000)) {
        return;
    }

    this->m_params.m_simpleKeys[0].m_color.r = color0.r;
    this->m_params.m_simpleKeys[0].m_color.g = color0.g;
    this->m_params.m_simpleKeys[0].m_color.b = color0.b;
    this->m_params.m_simpleKeys[0].m_deltaR = color1.r - color0.r;
    this->m_params.m_simpleKeys[0].m_deltaG = color1.g - color0.g;
    this->m_params.m_simpleKeys[0].m_deltaB = color1.b - color0.b;

    this->m_params.m_simpleKeys[1].m_color.r = color1.r;
    this->m_params.m_simpleKeys[1].m_color.b = color1.b;
    this->m_params.m_simpleKeys[1].m_color.g = color1.g;
    this->m_params.m_simpleKeys[1].m_deltaR = color2.r - color1.r;
    this->m_params.m_simpleKeys[1].m_deltaG = color2.g - color1.g;
    this->m_params.m_simpleKeys[1].m_deltaB = color2.b - color1.b;
}

// VERIFY: 1ac89391 manual
// OFFSET: 0x0097AB10
void CParticleEmitter2::GetReplacementColors(CImVector& color0, CImVector& color1, CImVector& color2) {
    color0.r = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[0].x));
    color0.g = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[0].y));
    color0.b = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[0].z));
    color1.r = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[1].x));
    color1.g = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[1].y));
    color1.b = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[1].z));
    color2.g = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[2].y));
    color2.r = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[2].x));
    color2.b = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[2].z));
}

// VERIFY: fa4e24dc auto
// OFFSET: 0x0097AC00
void CParticleEmitter2::SetTwinkleScale(const CRange& scale) {
    this->m_twinkleScaleBase = scale.m_l;
    this->m_twinkleScaleSpan = scale.m_h - scale.m_l;
}

// VERIFY: 1fde211d auto
// OFFSET: 0x0097AC20
void CParticleEmitter2::UpdateXform(const C44Matrix& transform, const C3Vector& viewPos, const C44Matrix* frameOfReference) {
    this->m_viewPos = viewPos;

    if (!frameOfReference || (this->m_flags & 0x200)) {
        this->m_xform = transform;
    } else {
        this->m_xform = transform * frameOfReference->AffineInverse();
    }

    this->m_xformScale = sqrt(transform.a2 * transform.a2 + transform.a1 * transform.a1 + transform.a0 * transform.a0);
}

// VERIFY: f0d46ebb manual
// OFFSET: 0x0097ACB0
void CParticleEmitter2::InternalUpdate(float deltaTime, int32_t child) {
    float time = 0.0f;

    if (deltaTime < 0.0f || (time = deltaTime, deltaTime <= s_maxTimeStep)) {
        this->m_burstOffset = this->m_followOffset;
        this->StepUpdate(time, child);
        return;
    }

    float steps = floor(time * 10.0f);
    float remainder = deltaTime - steps * s_maxTimeStep;
    float maxSteps = floor(this->m_params.m_life * 10.0f);

    if (maxSteps < steps) {
        steps = maxSteps;
    }

    int32_t count = CMath::fint_(steps - s_stepRound);
    float scale = 1.0f / (count + 1);

    this->m_burstOffset = C3Vector(this->m_followOffset.x * scale, this->m_followOffset.y * scale, scale * this->m_followOffset.z);

    for (; count; count--) {
        this->StepUpdate(s_maxTimeStep, child);
    }

    this->StepUpdate(remainder, child);
}

// VERIFY: a66e4b1d auto
// OFFSET: 0x0097AE00
static void RecursiveEmitterModelLoaded(CM2Model* model, void* arg) {
    CParticleEmitter2* emitter = static_cast<CParticleEmitter2*>(arg);

    if (!model || !emitter) {
        return;
    }

    if (!(model->m_fFlags & 1)) {
        model->WaitForLoad(nullptr);
    }

    uint32_t count = 4;

    if (model->m_shared->m_data->m_particles.m_count < 4) {
        if (!(model->m_fFlags & 1)) {
            model->WaitForLoad(nullptr);
        }

        count = model->m_shared->m_data->m_particles.m_count;
    }

    for (uint32_t i = 0; i < count; i++) {
        if (!(model->m_fFlags & 1)) {
            model->WaitForLoad(nullptr);
        }

        CParticleEmitter2* child = model->m_particleEmitters[i];
        emitter->m_childEmitters[emitter->m_childEmitterCount++] = child;
        child->m_flags |= 0x1;
    }
}

// VERIFY: 8fe41730 auto
// OFFSET: 0x0097AEB0
void CParticleEmitter2::CreateChildEmittersFromModel(CM2Scene* scene, const char* name) {
    if (!scene) {
        return;
    }

    this->m_childModel = scene->CreateModel(name, 0);

    if (this->m_childModel) {
        this->m_childModel->SetLoadedCallback(RecursiveEmitterModelLoaded, this);
    }
}

// VERIFY: b2426889 manual
// OFFSET: 0x0097AEF0
void CParticleEmitter2::ChangeFrameOfReference(const C44Matrix* matrix) {
    if (!(this->m_flags & 0x200)) {
        uint32_t count = this->m_liveList.Count();

        for (uint32_t i = 0; i < count; i++) {
            uint32_t index = this->m_liveList[i];
            CParticle2* particle = this->m_params.m_hasModel ? static_cast<CParticle2*>(&this->m_particleModels[index]) : &this->m_particles[index];

            particle->m_position *= *matrix;

            C3Vector velocity;
            velocity.x = particle->m_velocity.y * matrix->b0 + matrix->c0 * particle->m_velocity.z + matrix->a0 * particle->m_velocity.x;
            velocity.y = matrix->a1 * particle->m_velocity.x + matrix->c1 * particle->m_velocity.z + particle->m_velocity.y * matrix->b1;
            velocity.z = matrix->c2 * particle->m_velocity.z + particle->m_velocity.y * matrix->b2 + particle->m_velocity.x * matrix->a2;
            particle->m_velocity = velocity;
        }
    }

    for (uint32_t i = 0; i < this->m_childEmitterCount; i++) {
        this->m_childEmitters[i]->ChangeFrameOfReference(matrix);
    }
}

// VERIFY: 0e336bc5 manual
// OFFSET: 0x0097AFF0
void CParticleEmitter2::SetSimpleColorKeys() {
    SimpleKey* keys = this->m_params.m_simpleKeys;

    if (this->m_flags & 0x10) {
        keys[0].m_color.r = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[0].x));
        keys[0].m_color.g = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[0].y));
        keys[0].m_color.b = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[0].z));
        keys = this->m_params.m_simpleKeys;
        keys[1].m_color.r = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[1].x));
        keys[1].m_color.b = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[1].z));
        keys[1].m_color.g = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[1].y));
        this->m_params.m_simpleKeys[0].m_deltaR = this->m_params.m_simpleKeys[1].m_color.r - this->m_params.m_simpleKeys[0].m_color.r;
        this->m_params.m_simpleKeys[0].m_deltaG = this->m_params.m_simpleKeys[1].m_color.g - this->m_params.m_simpleKeys[0].m_color.g;
        this->m_params.m_simpleKeys[0].m_deltaB = this->m_params.m_simpleKeys[1].m_color.b - this->m_params.m_simpleKeys[0].m_color.b;
        this->m_params.m_simpleKeys[1].m_deltaR = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[2].x)) - this->m_params.m_simpleKeys[1].m_color.r;
        this->m_params.m_simpleKeys[1].m_deltaG = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[2].y)) - this->m_params.m_simpleKeys[1].m_color.g;
        this->m_params.m_simpleKeys[1].m_deltaB = static_cast<uint8_t>(CMath::fint_(this->m_params.m_replacementColors[2].z)) - this->m_params.m_simpleKeys[1].m_color.b;
        return;
    }

    const M2PartTrack<C3Vector>* track = this->m_params.m_colorTrack;
    keys[0].m_color.r = static_cast<uint8_t>(CMath::fint_(track->m_values[0].x));
    keys[0].m_color.g = static_cast<uint8_t>(CMath::fint_(track->m_values[0].y));
    keys[0].m_color.b = static_cast<uint8_t>(CMath::fint_(track->m_values[0].z));

    switch (this->m_params.m_colorTrack->m_values.m_count) {
        case 1:
            this->m_params.m_simpleKeys[0].m_deltaR = 0;
            this->m_params.m_simpleKeys[0].m_deltaG = 0;
            this->m_params.m_simpleKeys[0].m_deltaB = 0;
            break;

        case 2:
            this->m_params.m_simpleKeys[0].m_deltaR = static_cast<int32_t>(CMath::fint_((this->m_params.m_colorTrack->m_values[1].x - this->m_params.m_colorTrack->m_values[0].x) * this->m_params.m_simpleMidTime));
            this->m_params.m_simpleKeys[0].m_deltaG = static_cast<int32_t>(CMath::fint_((this->m_params.m_colorTrack->m_values[1].y - this->m_params.m_colorTrack->m_values[0].y) * this->m_params.m_simpleMidTime));
            this->m_params.m_simpleKeys[0].m_deltaB = static_cast<int32_t>(CMath::fint_((this->m_params.m_colorTrack->m_values[1].z - this->m_params.m_colorTrack->m_values[0].z) * this->m_params.m_simpleMidTime));
            break;

        case 3:
            this->m_params.m_simpleKeys[0].m_deltaR = static_cast<int32_t>(CMath::fint_(this->m_params.m_colorTrack->m_values[1].x - this->m_params.m_colorTrack->m_values[0].x));
            this->m_params.m_simpleKeys[0].m_deltaG = static_cast<int32_t>(CMath::fint_(this->m_params.m_colorTrack->m_values[1].y - this->m_params.m_colorTrack->m_values[0].y));
            this->m_params.m_simpleKeys[0].m_deltaB = static_cast<int32_t>(CMath::fint_(this->m_params.m_colorTrack->m_values[1].z - this->m_params.m_colorTrack->m_values[0].z));
            break;
    }

    this->m_params.m_simpleKeys[1].m_color.r = this->m_params.m_simpleKeys[0].m_color.r + this->m_params.m_simpleKeys[0].m_deltaR;
    this->m_params.m_simpleKeys[1].m_color.g = this->m_params.m_simpleKeys[0].m_color.g + this->m_params.m_simpleKeys[0].m_deltaG;
    this->m_params.m_simpleKeys[1].m_color.b = this->m_params.m_simpleKeys[0].m_color.b + this->m_params.m_simpleKeys[0].m_deltaB;

    if (this->m_params.m_colorTrack->m_values.m_count == 1) {
        this->m_params.m_simpleKeys[1].m_deltaR = 0;
        this->m_params.m_simpleKeys[1].m_deltaG = 0;
        this->m_params.m_simpleKeys[1].m_deltaB = 0;
    } else if (this->m_params.m_colorTrack->m_values.m_count == 2) {
        this->m_params.m_simpleKeys[1].m_deltaR = static_cast<int32_t>(CMath::fint_((this->m_params.m_colorTrack->m_values[1].x - this->m_params.m_colorTrack->m_values[0].x) * (1.0f - this->m_params.m_simpleMidTime)));
        this->m_params.m_simpleKeys[1].m_deltaG = static_cast<int32_t>(CMath::fint_((this->m_params.m_colorTrack->m_values[1].y - this->m_params.m_colorTrack->m_values[0].y) * (1.0f - this->m_params.m_simpleMidTime)));
        this->m_params.m_simpleKeys[1].m_deltaB = static_cast<int32_t>(CMath::fint_((this->m_params.m_colorTrack->m_values[1].z - this->m_params.m_colorTrack->m_values[0].z) * (1.0f - this->m_params.m_simpleMidTime)));
    } else if (this->m_params.m_colorTrack->m_values.m_count == 3) {
        this->m_params.m_simpleKeys[1].m_deltaR = static_cast<int32_t>(CMath::fint_(this->m_params.m_colorTrack->m_values[2].x - this->m_params.m_colorTrack->m_values[1].x));
        this->m_params.m_simpleKeys[1].m_deltaG = static_cast<int32_t>(CMath::fint_(this->m_params.m_colorTrack->m_values[2].y - this->m_params.m_colorTrack->m_values[1].y));
        this->m_params.m_simpleKeys[1].m_deltaB = static_cast<int32_t>(CMath::fint_(this->m_params.m_colorTrack->m_values[2].z - this->m_params.m_colorTrack->m_values[1].z));
    }
}

// VERIFY: 0d763968 manual
// OFFSET: 0x0097B440
void CParticleEmitter2::SetSimpleAlphaKeys() {
    this->m_params.m_simpleKeys[0].m_color.a = static_cast<uint8_t>(CMath::fint_(this->m_params.m_alphaTrack->m_values[0].m_value * 0.000030518509f * 255.0f));

    const M2PartTrack<fixed16>* track = this->m_params.m_alphaTrack;

    switch (track->m_values.m_count) {
        case 1:
            this->m_params.m_simpleKeys[0].m_deltaA = 0;
            break;

        case 2:
            this->m_params.m_simpleKeys[0].m_deltaA = static_cast<int32_t>(CMath::fint_((track->m_values[1].m_value - track->m_values[0].m_value) * 0.000030518509f * this->m_params.m_simpleMidTime * 255.0f));
            break;

        case 3:
            this->m_params.m_simpleKeys[0].m_deltaA = static_cast<int32_t>(CMath::fint_((track->m_values[1].m_value - track->m_values[0].m_value) * 0.000030518509f * 255.0f));
            break;
    }

    this->m_params.m_simpleKeys[1].m_color.a = this->m_params.m_simpleKeys[0].m_color.a + this->m_params.m_simpleKeys[0].m_deltaA;

    if (this->m_params.m_alphaTrack->m_values.m_count == 1) {
        this->m_params.m_simpleKeys[1].m_deltaA = 0;
    } else if (this->m_params.m_alphaTrack->m_values.m_count == 2) {
        int32_t delta = this->m_params.m_alphaTrack->m_values[1].m_value - this->m_params.m_alphaTrack->m_values[0].m_value;
        this->m_params.m_simpleKeys[1].m_deltaA = static_cast<int32_t>(CMath::fint_((1.0f - this->m_params.m_simpleMidTime) * (delta * 0.000030518509f) * 255.0f));
    } else if (this->m_params.m_alphaTrack->m_values.m_count == 3) {
        int32_t delta = this->m_params.m_alphaTrack->m_values[2].m_value - this->m_params.m_alphaTrack->m_values[1].m_value;
        this->m_params.m_simpleKeys[1].m_deltaA = static_cast<int32_t>(CMath::fint_(delta * 0.000030518509f * 255.0f));
    }
}

// VERIFY: 898ba971 manual
// OFFSET: 0x0097B5F0
void CParticleEmitter2::SetSimpleScaleKeys() {
    this->m_params.m_simpleKeys[0].m_scale = this->m_params.m_scaleTrack->m_values[0].x;

    const M2PartTrack<C2Vector>* track = this->m_params.m_scaleTrack;
    float delta = 0.0f;

    switch (track->m_values.m_count) {
        case 1:
            this->m_params.m_simpleKeys[0].m_deltaScale = 0.0f;
            break;

        case 2:
            this->m_params.m_simpleKeys[0].m_deltaScale = (track->m_values[1].x - track->m_values[0].x) * this->m_params.m_simpleMidTime;
            break;

        case 3:
            this->m_params.m_simpleKeys[0].m_deltaScale = track->m_values[1].x - track->m_values[0].x;
            break;
    }

    this->m_params.m_simpleKeys[1].m_scale = this->m_params.m_simpleKeys[0].m_deltaScale + this->m_params.m_simpleKeys[0].m_scale;

    track = this->m_params.m_scaleTrack;

    if (track->m_values.m_count == 1) {
        this->m_params.m_simpleKeys[1].m_deltaScale = delta;
    } else if (track->m_values.m_count == 2) {
        this->m_params.m_simpleKeys[1].m_deltaScale = (1.0f - this->m_params.m_simpleMidTime) * (track->m_values[1].x - track->m_values[0].x);
    } else if (track->m_values.m_count == 3) {
        delta = track->m_values[2].x - track->m_values[1].x;
        this->m_params.m_simpleKeys[1].m_deltaScale = delta;
    }
}

// VERIFY: 317b8d7f manual
// OFFSET: 0x0097B6C0
void CParticleEmitter2::SetSimpleHeadCellKeys() {
    const M2PartTrack<uint16_t>* track = this->m_params.m_headCellTrack;

    if (!track->m_values.m_count) {
        this->m_params.m_simpleKeys[0].m_headCell = 0;
        this->m_params.m_simpleKeys[0].m_deltaHeadCell = 0;
        this->m_params.m_simpleKeys[1].m_headCell = 0;
        this->m_params.m_simpleKeys[1].m_deltaHeadCell = 0;
        return;
    }

    this->m_params.m_simpleKeys[0].m_headCell = track->m_values[0];

    switch (this->m_params.m_headCellTrack->m_values.m_count) {
        case 1:
            this->m_params.m_simpleKeys[0].m_deltaHeadCell = 0;
            break;

        case 2:
            this->m_params.m_simpleKeys[0].m_deltaHeadCell = static_cast<int32_t>(CMath::fint_((this->m_params.m_headCellTrack->m_values[1] - this->m_params.m_headCellTrack->m_values[0]) * this->m_params.m_simpleMidTime));
            break;

        case 3:
        case 4:
            this->m_params.m_simpleKeys[0].m_deltaHeadCell = this->m_params.m_headCellTrack->m_values[1] - this->m_params.m_headCellTrack->m_values[0];
            break;
    }

    this->m_params.m_simpleKeys[1].m_headCell = this->m_params.m_simpleKeys[0].m_headCell + this->m_params.m_simpleKeys[0].m_deltaHeadCell;

    switch (this->m_params.m_headCellTrack->m_values.m_count) {
        case 1:
            this->m_params.m_simpleKeys[1].m_deltaHeadCell = 0;
            break;

        case 2:
            this->m_params.m_simpleKeys[1].m_deltaHeadCell = static_cast<int32_t>(CMath::fint_((1.0f - this->m_params.m_simpleMidTime) * (this->m_params.m_headCellTrack->m_values[1] - this->m_params.m_headCellTrack->m_values[0])));
            break;

        case 3:
            this->m_params.m_simpleKeys[1].m_deltaHeadCell = this->m_params.m_headCellTrack->m_values[2] - this->m_params.m_headCellTrack->m_values[1];
            break;

        case 4:
            this->m_params.m_simpleKeys[1].m_deltaHeadCell = this->m_params.m_headCellTrack->m_values[3] - this->m_params.m_headCellTrack->m_values[2];
            break;
    }
}

// VERIFY: b81b92f7 manual
// OFFSET: 0x0097B840
void CParticleEmitter2::SetSimpleTailCellKeys() {
    const M2PartTrack<uint16_t>* track = this->m_params.m_tailCellTrack;

    if (!track->m_values.m_count) {
        this->m_params.m_simpleKeys[0].m_tailCell = 0;
        this->m_params.m_simpleKeys[0].m_deltaTailCell = 0;
        this->m_params.m_simpleKeys[1].m_tailCell = 0;
        this->m_params.m_simpleKeys[1].m_deltaTailCell = 0;
        return;
    }

    this->m_params.m_simpleKeys[0].m_tailCell = track->m_values[0];

    switch (this->m_params.m_tailCellTrack->m_values.m_count) {
        case 1:
            this->m_params.m_simpleKeys[0].m_deltaTailCell = 0;
            break;

        case 2:
            this->m_params.m_simpleKeys[0].m_deltaTailCell = static_cast<int32_t>(CMath::fint_((this->m_params.m_tailCellTrack->m_values[1] - this->m_params.m_tailCellTrack->m_values[0]) * this->m_params.m_simpleMidTime));
            break;

        case 3:
        case 4:
            this->m_params.m_simpleKeys[0].m_deltaTailCell = this->m_params.m_tailCellTrack->m_values[1] - this->m_params.m_tailCellTrack->m_values[0];
            break;
    }

    this->m_params.m_simpleKeys[1].m_tailCell = this->m_params.m_simpleKeys[0].m_tailCell + this->m_params.m_simpleKeys[0].m_deltaTailCell;

    switch (this->m_params.m_tailCellTrack->m_values.m_count) {
        case 1:
            this->m_params.m_simpleKeys[1].m_deltaTailCell = 0;
            break;

        case 2:
            this->m_params.m_simpleKeys[1].m_deltaTailCell = static_cast<int32_t>(CMath::fint_((1.0f - this->m_params.m_simpleMidTime) * (this->m_params.m_tailCellTrack->m_values[1] - this->m_params.m_tailCellTrack->m_values[0])));
            break;

        case 3:
            this->m_params.m_simpleKeys[1].m_deltaTailCell = this->m_params.m_tailCellTrack->m_values[2] - this->m_params.m_tailCellTrack->m_values[1];
            break;

        case 4:
            this->m_params.m_simpleKeys[1].m_deltaTailCell = this->m_params.m_tailCellTrack->m_values[3] - this->m_params.m_tailCellTrack->m_values[2];
            break;
    }
}

// VERIFY: c36b86ed auto
// OFFSET: 0x0097B9C0
CParticleEmitter2::SimpleKey::SimpleKey() {
    this->m_deltaR = 0;
    this->m_deltaG = 0;
    this->m_deltaB = 0;
    this->m_deltaA = 0;
}

// VERIFY: b35364c3 auto
// OFFSET: 0x0097B9E0
int32_t CParticleEmitter2::HasLiveParticles() {
    if (this->m_liveList.Count()) {
        return 1;
    }

    uint32_t count = this->m_childEmitterCount;

    for (uint32_t i = 0; i < count; i++) {
        if (this->m_childEmitters[i]->HasLiveParticles()) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 33d99656 manual
// OFFSET: 0x0097BA30
uint32_t CParticleEmitter2::GetNumParticleModels() {
    uint32_t count = 0;

    if (this->m_params.m_hasModel == CParticleEmitter2::PT_MODEL) {
        count = this->m_liveList.Count();
    }

    for (uint32_t i = 0; i < this->m_childEmitterCount; i++) {
        count += this->m_childEmitters[i]->GetNumParticleModels();
    }

    return count;
}

// VERIFY: 8d3bef82 auto
// OFFSET: 0x0097BA70
CM2Model* CParticleEmitter2::GetParticleModelInternal(uint32_t& index) {
    if (this->m_params.m_hasModel == CParticleEmitter2::PT_MODEL) {
        uint32_t count = this->m_liveList.Count();

        if (index < count) {
            return this->m_particleModels[this->m_liveList[index]].m_model;
        }

        index -= count;
    }

    uint32_t childCount = this->m_childEmitterCount;

    for (uint32_t i = 0; i < childCount; i++) {
        CM2Model* model = this->m_childEmitters[i]->GetParticleModelInternal(index);

        if (model) {
            return model;
        }
    }

    return nullptr;
}

// VERIFY: cc9ab909 auto
// OFFSET: 0x0097BAE0
uint32_t CParticleEmitter2::GetNumBatches() {
    uint32_t count = this->m_batchFlags & 0x1;

    if (count >= this->m_liveList.Count()) {
        count = this->m_liveList.Count();
    }

    for (uint32_t i = 0; i < this->m_childEmitterCount; i++) {
        count += this->m_childEmitters[i]->GetNumBatches();
    }

    return count;
}

// VERIFY: e8406df5 auto
// OFFSET: 0x0097BB30
uint32_t CParticleEmitter2::GetNumPrimitives() {
    uint32_t count = this->m_liveList.Count() * this->m_indicesPerParticle / 3;

    for (uint32_t i = 0; i < this->m_childEmitterCount; i++) {
        count += this->m_childEmitters[i]->GetNumPrimitives();
    }

    return count;
}

// VERIFY: 09c24957 manual
// OFFSET: [0x0097BCC0, 0x0097D890]
CParticleEmitter2::~CParticleEmitter2() {
    if (this->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
        this->m_texture = nullptr;
    }

    if (this->m_model) {
        this->m_model->Release();
        this->m_model = nullptr;
    }

    if (this->m_childModel) {
        this->m_childModel->Release();
        this->m_childModel = nullptr;
    }

    delete[] this->m_params.m_simpleKeys;
}

// VERIFY: 3003481a auto
// OFFSET: 0x0097BD80
void CParticleEmitter2::SetEmissionRate(float rate) {
    if (rate <= 0.0f) {
        this->m_params.m_emissionRate = 0.0f;
    } else {
        this->m_params.m_emissionRate = rate;
    }
}

// VERIFY: 05fb6744 manual
// OFFSET: 0x0097BDB0
int32_t CParticleEmitter2::MoveParticle(CParticle2_Model& particle, float deltaTime) {
    float speed = sqrt(particle.m_angularVelocity.z * particle.m_angularVelocity.z + particle.m_angularVelocity.y * particle.m_angularVelocity.y + particle.m_angularVelocity.x * particle.m_angularVelocity.x);

    if (speed > 0.000099999997f) {
        float halfAngle = speed * deltaTime * 0.5f;
        float scale = sin(halfAngle) * (1.0f / speed);

        C4Quaternion rotation;
        rotation.x = particle.m_angularVelocity.x * scale;
        rotation.y = particle.m_angularVelocity.y * scale;
        rotation.z = scale * particle.m_angularVelocity.z;
        rotation.w = cos(halfAngle);

        const C4Quaternion& q = particle.m_orientation;
        C4Quaternion result;
        result.x = q.w * rotation.x + rotation.z * q.y + rotation.w * q.x - q.z * rotation.y;
        result.y = rotation.w * q.y + q.w * rotation.y + q.z * rotation.x - rotation.z * q.x;
        result.z = rotation.z * q.w + rotation.w * q.z + q.x * rotation.y - rotation.x * q.y;
        result.w = rotation.w * q.w - q.x * rotation.x - q.y * rotation.y - rotation.z * q.z;
        particle.m_orientation = result;
    }

    return this->MoveParticle(static_cast<CParticle2&>(particle), deltaTime);
}

// VERIFY: 30a25656 manual
// OFFSET: 0x0097BE80
int32_t CParticleEmitter2::BuildVertex(const CParticle2& particle, ParticleVertex& vertex) {
    uint32_t twinkle = 0;

    if (this->m_twinkleOnOff < 1.0f || 0.0f != this->m_twinkleScaleSpan) {
        twinkle = (static_cast<uint8_t>(CMath::fint_(this->m_twinkleFPS * particle.m_age)) + (reinterpret_cast<uintptr_t>(&particle) >> 5)) & 0x7F;
    }

    if (this->m_twinkleOnOff < 1.0f && this->m_twinkleOnOff < CParticleEmitter2::s_rndTable[twinkle]) {
        return 0;
    }

    CImVector color;
    C2Vector scale;
    int32_t headCell;
    int32_t tailCell;

    if (this->m_flags & 0x1000000) {
        this->InterpolateAllTracksSimple(particle, &color, &scale, &headCell, &tailCell);
    } else {
        this->InterpolateAllTracks(particle, &color, &scale, &headCell, &tailCell);
    }

    float spin;
    float spinRate;
    this->RandomizeSpin(particle, &spin, &spinRate);

    if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
        uint8_t red = color.r;
        color.r = color.b;
        color.b = red;
    }

    float twinkleScale = CParticleEmitter2::s_rndTable[twinkle] * this->m_twinkleScaleSpan + this->m_twinkleScaleBase;
    scale.x = scale.x * twinkleScale;
    scale.y = twinkleScale * scale.y;

    if (this->m_flags & 0x400) {
        scale.x = scale.x * this->m_xformScale;
        scale.y = this->m_xformScale * scale.y;
    }

    C3Vector center = particle.m_position * s_particleXform;
    uint32_t flags = this->m_flags;

    if (flags & 0x4) {
        uint32_t column = headCell & (this->m_textureCols - 1);
        float u = column * this->m_textureCellWidth;
        int32_t row = headCell >> this->m_textureCellShift;
        float v = row * this->m_textureCellHeight;

        if ((flags & 0x200000) && particle.m_velocity.z * particle.m_velocity.z + particle.m_velocity.y * particle.m_velocity.y + particle.m_velocity.x * particle.m_velocity.x > 0.00000023841858f) {
            C3Vector reverse(-particle.m_velocity.x, -particle.m_velocity.y, -particle.m_velocity.z);
            C3Vector direction = TransformDirection(reverse, s_particleXform);
            C2Vector flat(direction);

            float flatSq = flat.y * flat.y + flat.x * flat.x;
            float directionSq = direction.y * direction.y + direction.z * direction.z + direction.x * direction.x;
            float inv = 0.0f;

            if (flatSq > 0.00000023841858f) {
                inv = 1.0f / sqrt(flatSq);
            }

            float axisX = flat.x * inv;
            float axisY = flat.y * inv;
            C2Vector size = scale;

            if (0.00000023841858f < inv) {
                size.x = 1.0f / sqrt(directionSq) / inv * scale.x;
            }

            for (uint32_t i = 0; i < 4; i++) {
                float offsetX = s_cornerOffsets[i][0] * size.x;
                float offsetY = s_cornerOffsets[i][1] * size.y;
                C3Vector position(offsetY * -axisY + offsetX * axisX + center.x, offsetX * axisY + offsetY * axisX + center.y, center.z);
                C2Vector texCoord;
                texCoord.x = s_cornerTexCoords[i][0] * this->m_textureCellWidth + u;
                texCoord.y = s_cornerTexCoords[i][1] * this->m_textureCellHeight + v;
                AddVertex(this->m_worldBounds, vertex, position, color, texCoord);
            }
        } else if (0.0f == this->m_params.m_spin && 0.0f == this->m_params.m_spinVariation) {
            if (flags & 0x4000) {
                for (uint32_t i = 0; i < 4; i++) {
                    float offsetX = s_cornerOffsets[i][0] * scale.x;
                    float offsetY = s_cornerOffsets[i][1] * scale.y;
                    C3Vector position(s_billboardXform.b0 * offsetY + offsetX * s_billboardXform.a0 + center.x, offsetY * s_billboardXform.b1 + offsetX * s_billboardXform.a1 + center.y, offsetY * s_billboardXform.b2 + offsetX * s_billboardXform.a2 + center.z);
                    C2Vector texCoord;
                    texCoord.x = s_cornerTexCoords[i][0] * this->m_textureCellWidth + u;
                    texCoord.y = s_cornerTexCoords[i][1] * this->m_textureCellHeight + v;
                    AddVertex(this->m_worldBounds, vertex, position, color, texCoord);
                }
            } else {
                for (uint32_t i = 0; i < 4; i++) {
                    C3Vector position(s_cornerOffsets[i][0] * scale.x + center.x, s_cornerOffsets[i][1] * scale.y + center.y, center.z);
                    C2Vector texCoord;
                    texCoord.x = s_cornerTexCoords[i][0] * this->m_textureCellWidth + u;
                    texCoord.y = s_cornerTexCoords[i][1] * this->m_textureCellHeight + v;
                    AddVertex(this->m_worldBounds, vertex, position, color, texCoord);
                }
            }
        } else {
            float angle = particle.m_age * spinRate + spin;

            if ((flags & 0x10000) && (reinterpret_cast<uintptr_t>(&particle) & 0x20)) {
                angle = -angle;
            }

            if (flags & 0x4000) {
                for (uint32_t i = 0; i < 4; i++) {
                    C33Matrix rotation = C33Matrix::Rotation(angle, this->m_billboardAxis, true);
                    float offsetX = s_cornerOffsets[i][0] * scale.x;
                    float offsetY = s_cornerOffsets[i][1] * scale.y;
                    float billboardX = s_billboardXform.b0 * offsetY + offsetX * s_billboardXform.a0;
                    float billboardY = offsetY * s_billboardXform.b1 + offsetX * s_billboardXform.a1;
                    float billboardZ = offsetY * s_billboardXform.b2 + offsetX * s_billboardXform.a2;
                    C3Vector position(rotation.a2 * billboardZ + rotation.a1 * billboardY + billboardX * rotation.a0 + center.x, rotation.b2 * billboardZ + rotation.b1 * billboardY + rotation.b0 * billboardX + center.y, billboardX * rotation.c0 + billboardY * rotation.c1 + billboardZ * rotation.c2 + center.z);
                    C2Vector texCoord;
                    texCoord.x = s_cornerTexCoords[i][0] * this->m_textureCellWidth + u;
                    texCoord.y = s_cornerTexCoords[i][1] * this->m_textureCellHeight + v;
                    AddVertex(this->m_worldBounds, vertex, position, color, texCoord);
                }
            } else {
                float sinAngle = sin(angle);
                float cosAngle = cos(angle);
                float cosX = cosAngle * scale.x;
                float cosY = cosAngle * scale.y;
                float sinX = scale.x * sinAngle;
                float sinY = scale.y * sinAngle;
                C2Vector texCoord;

                texCoord.x = u;
                texCoord.y = v;
                AddVertex(this->m_worldBounds, vertex, C3Vector(center.x - cosX - sinY, center.y - sinX + cosY, center.z), color, texCoord);

                texCoord.x = u;
                texCoord.y = v + this->m_textureCellHeight;
                AddVertex(this->m_worldBounds, vertex, C3Vector(center.x - cosX + sinY, center.y - sinX - cosY, center.z), color, texCoord);

                texCoord.x = this->m_textureCellWidth + u;
                texCoord.y = v;
                AddVertex(this->m_worldBounds, vertex, C3Vector(cosX + center.x - sinY, cosY + sinX + center.y, center.z), color, texCoord);

                texCoord.x = u + this->m_textureCellWidth;
                texCoord.y = v + this->m_textureCellHeight;
                AddVertex(this->m_worldBounds, vertex, C3Vector(cosX + sinY + center.x, sinX + center.y - cosY, center.z), color, texCoord);
            }
        }
    }

    if (!(this->m_flags & 0x8)) {
        return 1;
    }

    uint32_t column = tailCell & (this->m_textureCols - 1);
    float u = column * this->m_textureCellWidth;
    int32_t row = tailCell >> this->m_textureCellShift;
    float v = row * this->m_textureCellHeight;

    float tailLength = this->m_params.m_tailLength;

    if ((this->m_flags & 0x20000) && tailLength > particle.m_age) {
        tailLength = particle.m_age;
    }

    float reverseX = -particle.m_velocity.x;
    float reverseY = -particle.m_velocity.y;
    float reverseZ = -particle.m_velocity.z;

    C3Vector tail;
    tail.x = (reverseY * s_particleXform.b0 + reverseZ * s_particleXform.c0 + reverseX * s_particleXform.a0) * tailLength;
    tail.y = (reverseY * s_particleXform.b1 + reverseZ * s_particleXform.c1 + reverseX * s_particleXform.a1) * tailLength;
    tail.z = tailLength * (reverseY * s_particleXform.b2 + reverseZ * s_particleXform.c2 + reverseX * s_particleXform.a2);

    C2Vector flat(tail);
    float flatSq = flat.y * flat.y + flat.x * flat.x;

    if (flatSq < 0.00077160494f) {
        for (uint32_t i = 0; i < 4; i++) {
            C3Vector position(s_cornerOffsets[i][0] * scale.x + center.x, s_cornerOffsets[i][1] * scale.y + center.y, center.z);
            C2Vector texCoord;
            texCoord.x = s_cornerTexCoords[i][0] * this->m_textureCellWidth + u;
            texCoord.y = s_cornerTexCoords[i][1] * this->m_textureCellHeight + v;
            AddVertex(this->m_worldBounds, vertex, position, color, texCoord);
        }

        return 1;
    }

    float inv = 1.0f / sqrt(flatSq);
    float perpX = flat.x * (scale.x * inv);
    float perpY = flat.y * (inv * scale.y);
    C3Vector end(tail.x + center.x, tail.y + center.y, tail.z + center.z);
    C2Vector texCoord;

    texCoord.x = this->m_textureCellWidth * s_cornerTexCoords[0][0] + u;
    texCoord.y = this->m_textureCellHeight * s_cornerTexCoords[0][1] + v;
    AddVertex(this->m_worldBounds, vertex, C3Vector(center.x - perpY, center.y + perpX, center.z), color, texCoord);

    texCoord.x = this->m_textureCellWidth * s_cornerTexCoords[1][0] + u;
    texCoord.y = this->m_textureCellHeight * s_cornerTexCoords[1][1] + v;
    AddVertex(this->m_worldBounds, vertex, C3Vector(perpY + center.x, center.y - perpX, center.z), color, texCoord);

    texCoord.x = this->m_textureCellWidth * s_cornerTexCoords[2][0] + u;
    texCoord.y = this->m_textureCellHeight * s_cornerTexCoords[2][1] + v;
    AddVertex(this->m_worldBounds, vertex, C3Vector(end.x - perpY, perpX + end.y, end.z), color, texCoord);

    texCoord.x = u + s_cornerTexCoords[3][0] * this->m_textureCellWidth;
    texCoord.y = v + this->m_textureCellHeight * s_cornerTexCoords[3][1];
    AddVertex(this->m_worldBounds, vertex, C3Vector(end.x + perpY, end.y - perpX, end.z), color, texCoord);

    return 1;
}

// VERIFY: fe641edb manual
// OFFSET: 0x0097D370
void CParticleEmitter2::DetermineIfSimple() {
    uint32_t colorKeys = this->m_params.m_colorTrack->m_times.m_count;

    if (colorKeys > 3 || !colorKeys) {
        return;
    }

    uint32_t alphaKeys = this->m_params.m_alphaTrack->m_times.m_count;

    if (alphaKeys > 3 || !alphaKeys) {
        return;
    }

    const M2PartTrack<C2Vector>* scaleTrack = this->m_params.m_scaleTrack;
    uint32_t scaleKeys = scaleTrack->m_times.m_count;

    if (scaleKeys > 3 || !scaleKeys) {
        return;
    }

    uint32_t headKeys = this->m_params.m_headCellTrack->m_times.m_count;

    if (headKeys > 4 || (headKeys == 4 && this->m_params.m_headCellTrack->m_times[1].m_value != this->m_params.m_headCellTrack->m_times[2].m_value)) {
        return;
    }

    uint32_t tailKeys = this->m_params.m_tailCellTrack->m_times.m_count;

    if (tailKeys > 4 || (tailKeys == 4 && this->m_params.m_tailCellTrack->m_times[1].m_value != this->m_params.m_tailCellTrack->m_times[2].m_value)) {
        return;
    }

    if (0.0f != this->m_params.m_lifeVariation || (!headKeys && (this->m_flags & 0x100000))) {
        return;
    }

    if (this->m_params.m_scaleVariationX != 0.0f || this->m_params.m_scaleVariationY != 0.0f) {
        return;
    }

    for (uint32_t i = 0; i < scaleTrack->m_values.m_count; i++) {
        if (CMath::fnotequal(scaleTrack->m_values[i].x, scaleTrack->m_values[i].y)) {
            return;
        }
    }

    int16_t midTime = 0;

    if (colorKeys == 3) {
        midTime = this->m_params.m_colorTrack->m_times[1].m_value;
    } else if (alphaKeys == 3) {
        midTime = this->m_params.m_alphaTrack->m_times[1].m_value;
    } else if (scaleKeys == 3) {
        midTime = this->m_params.m_scaleTrack->m_times[1].m_value;
    } else if (headKeys >= 3) {
        midTime = this->m_params.m_headCellTrack->m_times[1].m_value;
    } else if (tailKeys >= 3) {
        midTime = this->m_params.m_tailCellTrack->m_times[1].m_value;
    }

    if (alphaKeys == 3 && this->m_params.m_alphaTrack->m_times[1].m_value != midTime) {
        return;
    }

    if (scaleKeys == 3 && this->m_params.m_scaleTrack->m_times[1].m_value != midTime) {
        return;
    }

    if (headKeys >= 3 && this->m_params.m_headCellTrack->m_times[1].m_value != midTime) {
        return;
    }

    if (tailKeys >= 3 && this->m_params.m_tailCellTrack->m_times[1].m_value != midTime) {
        return;
    }

    this->m_params.m_simpleKeys = new (__FILE__, __LINE__) SimpleKey[2];

    if (!this->m_params.m_simpleKeys) {
        return;
    }

    this->m_flags |= 0x1000000;
    this->m_params.m_simpleMidTime = midTime * 0.000030518509f;

    this->SetSimpleColorKeys();
    this->SetSimpleAlphaKeys();
    this->SetSimpleScaleKeys();
    this->SetSimpleHeadCellKeys();
    this->SetSimpleTailCellKeys();
}

// VERIFY: 4fbdf517 manual
// OFFSET: 0x0097D730
CSortableParticleQueue::CSortableParticleQueue(const uint32_t* params) : CMemBlock(sizeof(CSortableParticleRecord) * params[0], 0, typeid(CSortableParticleRecord).raw_name(), SLOG_OBJECT) {
    uint32_t count = params[0];
    uint32_t chunk = params[1];

    this->m_chunk = chunk;
    this->m_capacity = count;
    this->m_count = 0;

    if (count) {
        this->m_count++;
        return;
    }

    if (!chunk) {
        return;
    }

    uint32_t grow = chunk;

    if (grow <= 1 - count) {
        grow = 1 - count;
    }

    if (this->Resize(sizeof(CSortableParticleRecord) * (grow + count), true)) {
        this->m_capacity += grow;
        this->m_count++;
    }
}

// VERIFY: 8e4b7bac manual
// OFFSET: 0x0097D820
void CParticleEmitter2::EmitParticle(float time, const C44Matrix& transform) {
    uint32_t index = PopIndex(this->m_freeList);
    this->m_liveList.New(index);

    if (this->m_params.m_hasModel) {
        this->CreateParticle(this->m_particleModels[index], time, transform);
    } else {
        this->CreateParticle(this->m_particles[index], time, transform);
    }
}

// VERIFY: 04483e8b manual
// OFFSET: 0x0097D8C0
void CParticleEmitter2::EmitNewParticles(float deltaTime, const C44Matrix& transform) {
    float distanceScale = 1.0f;

    if (!(this->m_flags & 0x400000)) {
        distanceScale = 1.0f - (CParticleEmitter2::s_viewDist - 50.0f) * 0.02f;

        if (distanceScale < 0.25f) {
            distanceScale = 0.25f;
        } else if (distanceScale >= 1.0f) {
            distanceScale = 1.0f;
        }
    }

    float rate = CRandom::sfloat_(this->m_random) * this->m_params.m_emissionRateVariation + this->m_params.m_emissionRate;
    rate = ParticleSystemManager::GetScaler() * rate * distanceScale;

    if ((this->m_flags & 0x40) && (this->m_flags & 0x2)) {
        int32_t burst = static_cast<int32_t>(rate);

        while (this->m_freeList.Count() && burst--) {
            this->EmitParticle(0.0f, transform);
        }

        this->m_flags &= ~0x40;
    }

    if ((this->m_flags & 0x3) != 0x3) {
        return;
    }

    uint32_t emitted = 0;
    this->m_emitAccumulator = rate * deltaTime + this->m_emitAccumulator;

    if (this->m_flags & 0x2000) {
        C44Matrix& xform = const_cast<C44Matrix&>(transform);
        C3Vector position(xform.d0, xform.d1, xform.d2);
        C3Vector delta(position.x - this->m_parentPosition.x, position.y - this->m_parentPosition.y, position.z - this->m_parentPosition.z);
        int32_t count = static_cast<int32_t>(this->m_emitAccumulator + 0.5f);

        if (this->m_freeList.Count()) {
            while (count--) {
                float t = CRandom::ufloat_(this->m_random);
                xform.d0 = delta.x * t + this->m_parentPosition.x;
                xform.d1 = delta.y * t + this->m_parentPosition.y;
                xform.d2 = t * delta.z + this->m_parentPosition.z;

                this->EmitParticle(deltaTime, transform);
                emitted++;

                if (!this->m_freeList.Count()) {
                    break;
                }
            }
        }

        xform.d0 = position.x;
        xform.d1 = position.y;
        xform.d2 = position.z;
    } else {
        int32_t count = static_cast<int32_t>(this->m_emitAccumulator + 0.5f);

        if (this->m_freeList.Count()) {
            while (count--) {
                this->EmitParticle(deltaTime, transform);
                emitted++;

                if (!this->m_freeList.Count()) {
                    break;
                }
            }
        }
    }

    this->m_emitAccumulator = this->m_emitAccumulator - emitted;
}

// VERIFY: 2b7513ee manual
// OFFSET: 0x0097DB80
int32_t CParticleEmitter2::UpdateLiveParticle(float deltaTime, CParticle2* particle, uint32_t index) {
    C3Vector position = particle->m_position;
    int32_t alive;

    if (this->m_params.m_hasModel) {
        alive = this->MoveParticle(*static_cast<CParticle2_Model*>(particle), deltaTime);
    } else {
        alive = this->MoveParticle(*particle, deltaTime);
    }

    if (!alive) {
        this->DestroyParticle(*particle);
        this->m_freeList.New(this->m_liveList[index]);
        this->m_liveList[index] = *this->m_liveList.Top();
        PopIndex(this->m_liveList);
        return 0;
    }

    for (uint32_t i = 0; i < this->m_childEmitterCount; i++) {
        CParticleEmitter2* child = this->m_childEmitters[i];
        float d0 = this->m_xform.d0;
        float d1 = this->m_xform.d1;
        float d2 = this->m_xform.d2;

        this->m_xform.d0 = particle->m_position.x;
        this->m_xform.d1 = particle->m_position.y;
        this->m_xform.d2 = particle->m_position.z;

        if (child->m_flags & 0x800) {
            child->m_parentVelocity = particle->m_velocity;
        }

        if (child->m_flags & 0x2000) {
            child->m_parentPosition = position;
        }

        child->EmitNewParticles(deltaTime, this->m_xform);

        this->m_xform.d0 = d0;
        this->m_xform.d1 = d1;
        this->m_xform.d2 = d2;
    }

    return 1;
}

// VERIFY: 4ddbfa64 manual
// OFFSET: 0x0097DD20
void CParticleEmitter2::StepUpdate(float deltaTime, int32_t child) {
    if (deltaTime <= 0.0f) {
        return;
    }

    if ((this->m_flags & 0x3) == 0x3 || ((this->m_flags & 0x40) && (this->m_flags & 0x2))) {
        this->Sync();
    }

    if (!child) {
        this->EmitNewParticles(deltaTime, this->m_xform);
    }

    uint32_t i = 0;

    if (0.0f == this->m_params.m_lifeVariation) {
        float life = 0.001f;

        if (this->m_params.m_life > 0.001f) {
            life = this->m_params.m_life;
        }

        while (i < this->m_liveList.Count()) {
            uint32_t index = this->m_liveList[i];
            CParticle2* particle = this->m_params.m_hasModel ? static_cast<CParticle2*>(&this->m_particleModels[index]) : &this->m_particles[index];
            particle->m_age = particle->m_age + deltaTime;

            if (particle->m_age < life) {
                if (this->UpdateLiveParticle(deltaTime, particle, i)) {
                    i++;
                }
            } else {
                this->DestroyParticle(*particle);
                this->m_freeList.New(this->m_liveList[i]);
                this->m_liveList[i] = *this->m_liveList.Top();
                PopIndex(this->m_liveList);
            }
        }
    } else {
        while (i < this->m_liveList.Count()) {
            float life = 0.001f;
            uint32_t index = this->m_liveList[i];
            CParticle2* particle = this->m_params.m_hasModel ? static_cast<CParticle2*>(&this->m_particleModels[index]) : &this->m_particles[index];
            particle->m_age = particle->m_age + deltaTime;

            float particleLife = particle->m_lifeSeed.m_value * this->m_params.m_lifeVariation * 0.000030518509f + this->m_params.m_life;

            if (particleLife > 0.001f) {
                life = particleLife;
            }

            if (particle->m_age < life) {
                if (this->UpdateLiveParticle(deltaTime, particle, i)) {
                    i++;
                }
            } else {
                this->DestroyParticle(*particle);
                this->m_freeList.New(this->m_liveList[i]);
                this->m_liveList[i] = *this->m_liveList.Top();
                PopIndex(this->m_liveList);
            }
        }
    }

    for (uint32_t j = 0; j < this->m_childEmitterCount; j++) {
        this->m_childEmitters[j]->InternalUpdate(deltaTime, 1);
    }
}

// VERIFY: c7e311d7 manual
// OFFSET: 0x0097E080
CSortableParticleRecord CSortableParticleQueue::Pop() {
    CSortableParticleRecord* data = reinterpret_cast<CSortableParticleRecord*>(this->m_data);
    CSortableParticleRecord top = data[1];
    CSortableParticleRecord last = data[this->m_count - 1];

    if (this->m_count) {
        this->Erase(this->m_count - 1, 1);
    }

    if (this->m_count < 2) {
        return top;
    }

    uint32_t limit = this->m_count - 1;
    uint32_t half = limit >> 1;
    uint32_t index = 1;

    if (half) {
        while (1) {
            uint32_t child = 2 * index;
            data = reinterpret_cast<CSortableParticleRecord*>(this->m_data);

            if (child < limit && data[child].m_depth <= data[child + 1].m_depth) {
                child++;
            }

            if (last.m_depth >= data[child].m_depth) {
                break;
            }

            data[index] = data[child];
            index = child;

            if (child > half) {
                break;
            }
        }
    }

    reinterpret_cast<CSortableParticleRecord*>(this->m_data)[index] = last;

    return top;
}

// VERIFY: eeea2b31 manual
// OFFSET: 0x00979250
CParticleEmitter2::Params::Params() {
    this->m_hasModel = CParticleEmitter2::PT_QUAD;
    this->m_emissionRate = 0.0f;
    this->m_emissionRateVariation = 0.0f;
    this->m_life = 0.0f;
    this->m_lifeVariation = 0.0f;
    this->m_tailLength = 1.0f;
    this->m_speed = 0.0f;
    this->m_gravity = 0.0f;
    this->m_variation = 0.1f;
    this->m_zsource = 0.0f;
    this->m_initialSpin = 0.0f;
    this->m_initialSpinVariation = 0.0f;
    this->m_spin = 0.0f;
    this->m_spinVariation = 0.0f;
    this->m_materialBlend = 0;
    this->m_materialFlags |= 0x7;
    this->m_colorTrack = nullptr;
    this->m_alphaTrack = nullptr;
    this->m_scaleTrack = nullptr;
    this->m_scaleVariationX = 0.0f;
    this->m_scaleVariationY = 0.0f;
    this->m_headCellTrack = nullptr;
    this->m_tailCellTrack = nullptr;
    this->m_simpleMidTime = 0.0f;
    this->m_simpleKeys = nullptr;
}

// VERIFY: f6dd65ff manual
// OFFSET: 0x0097E150
CParticleEmitter2::CParticleEmitter2() : m_random(0) {
    this->m_emitAccumulator = 0.0f;
    this->m_refCount = 1;
    this->m_textureCellShift = 0;
    this->m_textureCellWidth = 1.0f;
    this->m_textureCellHeight = 1.0f;
    this->m_renderedParticles = 0;
    this->m_emitterType = PET_BASE_EMITTER;
    this->m_childEmitterCount = 4;
    this->m_model = nullptr;
    this->m_childModel = nullptr;
    this->m_vertsPerParticle = 0;
    this->m_indicesPerParticle = 0;
    this->m_alphaScale = 1.0f;
    this->m_texture = nullptr;
    this->unused_012C = 0;
    this->m_twinkleFPS = 10.0f;
    this->m_flags = 0x6;
    this->m_textureRows = 1;
    this->m_twinkleOnOff = 1.0f;
    this->m_textureCols = 1;
    this->m_twinkleScaleBase = 1.0f;
    this->m_twinkleScaleSpan = 0.0f;
    this->m_ivelScale = 1.0f;
    this->m_tumbleBaseX = 0.0f;
    this->m_tumbleSpanX = 0.0f;
    this->m_tumbleBaseY = 0.0f;
    this->m_tumbleSpanY = 0.0f;
    this->m_tumbleBaseZ = 0.0f;
    this->m_tumbleSpanZ = 0.0f;
    this->m_drag = 0.0f;
    this->m_windVectorX = 0.0f;
    this->m_windVectorY = 0.0f;
    this->m_windVectorZ = 0.0f;
    this->m_windTime = 0.0f;
    this->m_followIntercept = 0.0f;
    this->m_followSlope = 0.0f;
    this->m_ivelTimer = 0.0f;
    this->m_worldBounds.b.x = 3.4028235e38f;
    this->m_worldBounds.b.y = 3.4028235e38f;
    this->m_worldBounds.b.z = 3.4028235e38f;
    this->m_worldBounds.t.x = -3.4028235e38f;
    this->m_worldBounds.t.y = -3.4028235e38f;
    this->m_worldBounds.t.z = -3.4028235e38f;
    this->unused_0230 = 0;

    this->m_random.SetSeed((rand() << 16) | static_cast<uint16_t>(rand()));
    this->m_childEmitterCount = 0;
}

// VERIFY: 2e7b5ab6 manual
// OFFSET: 0x0097E3F0
void CParticleEmitter2::SyncReserve(uint32_t count, uint32_t used, uint32_t spare) {
    uint32_t target = count;

    if (count & (count - 1)) {
        target = 2 * count - 1;

        for (uint32_t bits = target & (2 * count - 2); bits; bits &= bits - 1) {
            target = bits;
        }
    }

    if (used + spare >= target) {
        return;
    }

    uint32_t shortfall = target - (used + spare);

    if (this->m_params.m_hasModel) {
        if (shortfall + this->m_particleModels.Count() > this->m_particleModels.m_alloc) {
            this->m_particleModels.ReallocData(shortfall + this->m_particleModels.Count());
        }
    } else if (shortfall + this->m_particles.Count() > this->m_particles.m_alloc) {
        this->m_particles.ReallocData(shortfall + this->m_particles.Count());
    }

    if (shortfall + this->m_liveList.Count() > this->m_liveList.m_alloc) {
        this->m_liveList.ReallocData(shortfall + this->m_liveList.Count());
    }

    if (shortfall + this->m_freeList.Count() > this->m_freeList.m_alloc) {
        this->m_freeList.ReallocData(shortfall + this->m_freeList.Count());
    }
}

// VERIFY: e29c25ff manual
// OFFSET: 0x0097E480
void CParticleEmitter2::SyncAllocation(uint32_t count) {
    if (this->m_params.m_hasModel) {
        uint32_t oldCount = this->m_particleModels.Count();

        if (oldCount >= count) {
            return;
        }

        this->SyncReserve(count, oldCount, this->m_particleModels.m_alloc - this->m_particleModels.Count());
        this->m_particleModels.SetCount(count);

        if (count + this->m_liveList.Count() > this->m_liveList.m_alloc) {
            this->m_liveList.ReallocData(count + this->m_liveList.Count());
        }

        if (count + this->m_freeList.Count() > this->m_freeList.m_alloc) {
            this->m_freeList.ReallocData(count + this->m_freeList.Count());
        }

        for (uint32_t i = oldCount; i != count; i++) {
            uint32_t index = i;
            this->m_freeList.New(index);
        }
    } else {
        uint32_t oldCount = this->m_particles.Count();

        if (oldCount >= count) {
            return;
        }

        this->SyncReserve(count, oldCount, this->m_particles.m_alloc - this->m_particles.Count());
        this->m_particles.SetCount(count);

        if (count + this->m_liveList.Count() > this->m_liveList.m_alloc) {
            this->m_liveList.ReallocData(count + this->m_liveList.Count());
        }

        if (count + this->m_freeList.Count() > this->m_freeList.m_alloc) {
            this->m_freeList.ReallocData(count + this->m_freeList.Count());
        }

        for (uint32_t i = oldCount; i != count; i++) {
            uint32_t index = i;
            this->m_freeList.New(index);
        }
    }
}

// VERIFY: f3003e82 manual
// OFFSET: 0x0097E580
void CParticleEmitter2::BuildParticleVertices(ParticleVertex& vertex, uint32_t count, const C44Matrix& transform) {
    uint32_t i = 0;

    if (this->m_flags & 0x20) {
        for (; i < this->m_liveList.Count(); i++) {
            uint32_t index = this->m_liveList[i];
            CParticle2* particle = this->m_params.m_hasModel ? static_cast<CParticle2*>(&this->m_particleModels[index]) : &this->m_particles[index];
            float depth = s_particleXform.c2 * particle->m_position.z + s_particleXform.b2 * particle->m_position.y + s_particleXform.a2 * particle->m_position.x + s_particleXform.d2;
            CParticleEmitter2::s_pq.Push(depth, particle);
        }

        for (uint32_t j = count; j; j--) {
            CSortableParticleRecord record = CParticleEmitter2::s_pq.Pop();
            this->BuildVertex(*record.m_particle, vertex);
        }
    } else {
        for (; i < count; i++) {
            uint32_t index = this->m_liveList[i];
            CParticle2* particle = this->m_params.m_hasModel ? static_cast<CParticle2*>(&this->m_particleModels[index]) : &this->m_particles[index];
            this->BuildVertex(*particle, vertex);
        }
    }

    if (count) {
        this->m_worldBounds = CAaBox::Transform(this->m_worldBounds, transform);
        this->m_worldBounds.b.x = this->m_worldBounds.b.x + this->m_viewPos.x;
        this->m_worldBounds.b.y = this->m_worldBounds.b.y + this->m_viewPos.y;
        this->m_worldBounds.b.z = this->m_viewPos.z + this->m_worldBounds.b.z;
        this->m_worldBounds.t.x = this->m_worldBounds.t.x + this->m_viewPos.x;
        this->m_worldBounds.t.y = this->m_worldBounds.t.y + this->m_viewPos.y;
        this->m_worldBounds.t.z = this->m_viewPos.z + this->m_worldBounds.t.z;
    }

    this->m_renderedParticles = vertex.m_count / this->m_vertsPerParticle;
}

// VERIFY: 722cfea2 manual
// OFFSET: 0x0097E730
void CParticleEmitter2::RenderParticles(const C44Matrix* frameOfReference, void* buffer) {
    CGxBuf* buf = nullptr;

    if (!s_particleRenderEnable) {
        this->m_batchFlags &= ~0x1;
        this->m_renderedParticles = 0;
        return;
    }

    C44Matrix view;
    CGxDevice* device = g_theGxDevicePtr;
    view = device->m_xforms[GxXform_View].m_mtx[device->m_xforms[GxXform_View].m_level];

    this->RenderParticlesPrep(frameOfReference, view);

    if (!TextureGetGxTex(this->m_texture, 0, nullptr)) {
        this->m_batchFlags &= ~0x1;
        this->m_renderedParticles = 0;
        return;
    }

    uint32_t count = 0x4000 / this->m_vertsPerParticle;

    if (count >= this->m_liveList.Count()) {
        count = this->m_liveList.Count();
    }

    EGxVertexBufferFormat format = (this->m_params.m_materialFlags & 0x1) ? GxVBF_PNCT : GxVBF_PCT;

    if (!buffer) {
        buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, GxVertexSize(format), count * this->m_vertsPerParticle);
        buffer = g_theGxDevicePtr->BufLock(buf);
    }

    ParticleVertex vertex;
    vertex.m_count = 0;
    this->FillOutParticleVertex(buffer, format, &vertex);
    this->BuildParticleVertices(vertex, count, view.AffineInverse());

    if (buf) {
        g_theGxDevicePtr->BufUnlock(buf, 0);
        buf->m_hasData = 1;

        if (this->m_renderedParticles) {
            CParticleEmitter2::RenderParticleVertices(buf, format, vertex.m_count, this->m_renderedParticles * this->m_indicesPerParticle);
            g_theGxDevicePtr->XformSetView(view);
        }
    }
}

// VERIFY: 3d799495 manual
// OFFSET: 0x0097E8D0
void CParticleEmitter2::PlaceParticleModels(const C44Matrix* frameOfReference) {
    if (!this->m_model) {
        return;
    }

    uint32_t flags = this->m_flags;

    if (!(flags & 0x20)) {
        for (uint32_t i = 0; i < this->m_liveList.Count(); i++) {
            this->RenderParticle(this->m_particleModels[this->m_liveList[i]], frameOfReference);
        }

        return;
    }

    C44Matrix view;
    CGxDevice* device = g_theGxDevicePtr;
    view = device->m_xforms[GxXform_View].m_mtx[device->m_xforms[GxXform_View].m_level];

    C44Matrix xform;
    xform.a2 = 0.0f;
    xform.b2 = 0.0f;
    xform.c2 = 1.0f;
    xform.d2 = 0.0f;

    if (flags & 0x200) {
        xform = this->m_xform * view;
    } else if (frameOfReference) {
        xform = *frameOfReference * view;
    } else {
        xform = view;
    }

    for (uint32_t i = 0; i < this->m_liveList.Count(); i++) {
        CParticle2* particle = &this->m_particles[this->m_liveList[i]];
        float depth = particle->m_position.z * xform.c2 + particle->m_position.y * xform.b2 + particle->m_position.x * xform.a2 + xform.d2;
        CParticleEmitter2::s_pq.Push(depth, particle);
    }

    for (uint32_t i = this->m_liveList.Count(); i; i--) {
        CSortableParticleRecord record = CParticleEmitter2::s_pq.Pop();
        this->RenderParticle(*static_cast<CParticle2_Model*>(record.m_particle), frameOfReference);
    }
}

// VERIFY: 75d4c0c2 manual
// OFFSET: 0x0097EA60
void CParticleEmitter2::Render(const C44Matrix* frameOfReference, void* buffer, uint32_t first) {
    this->m_worldBounds.b.x = 3.4028235e38f;
    this->m_worldBounds.b.y = 3.4028235e38f;
    this->m_worldBounds.t.x = -3.4028235e38f;
    this->m_worldBounds.b.z = 3.4028235e38f;
    this->m_worldBounds.t.y = -3.4028235e38f;
    this->m_worldBounds.t.z = -3.4028235e38f;

    if (!this->m_liveList.Count()) {
        this->m_batchFlags &= ~0x1;
        this->m_renderedParticles = 0;
        return;
    }

    this->m_batchFlags ^= (first ^ this->m_batchFlags) & 0x1;

    if (!this->m_params.m_hasModel) {
        this->RenderParticles(frameOfReference, buffer);
    }
}

// VERIFY: c2eb5985 manual
// OFFSET: 0x0097EB10
void CParticleEmitter2::Update(float deltaTime, const C44Matrix& transform, const C3Vector& viewPos, const C44Matrix* frameOfReference) {
    float dz = transform.d2 - viewPos.z;
    float dy = transform.d1 - viewPos.y;
    float dx = transform.d0 - viewPos.x;
    float distanceSq = dx * dx + (dy * dy + dz * dz);

    CParticleEmitter2::s_viewDist = distanceSq <= 0.0f ? 0.0f : sqrt(distanceSq);

    this->m_parentPosition = C3Vector(this->m_xform.d0, this->m_xform.d1, this->m_xform.d2);
    this->UpdateXform(transform, viewPos, frameOfReference);

    for (uint32_t i = 0; i < this->m_childEmitterCount; i++) {
        this->m_childEmitters[i]->UpdateXform(transform, viewPos, frameOfReference);
    }

    if (fabs(deltaTime) < 0.00000023841858f) {
        this->m_flags |= 0x100;
        return;
    }

    float zero;

    if (this->m_flags & 0x80000) {
        this->m_followOffset.x = this->m_xform.d0 - this->m_parentPosition.x;
        this->m_followOffset.y = this->m_xform.d1 - this->m_parentPosition.y;
        this->m_followOffset.z = this->m_xform.d2 - this->m_parentPosition.z;

        zero = 0.0f;
        float lengthSq = this->m_followOffset.z * this->m_followOffset.z + this->m_followOffset.y * this->m_followOffset.y + this->m_followOffset.x * this->m_followOffset.x;
        float speed = 0.0f;

        if (lengthSq > 0.0f) {
            speed = sqrt(lengthSq) / deltaTime;
        }

        float follow = speed * this->m_followSlope + this->m_followIntercept;

        if (follow < zero) {
            follow = zero;
        } else if (follow >= 1.0f) {
            follow = 1.0f;
        }

        this->m_followOffset.x = this->m_followOffset.x * follow;
        this->m_followOffset.y = follow * this->m_followOffset.y;
        this->m_followOffset.z = follow * this->m_followOffset.z;
    } else {
        zero = 0.0f;
    }

    if (this->m_flags & 0x800) {
        this->m_ivelTimer = this->m_ivelTimer + deltaTime;

        if (this->m_ivelTimer > VEL_UPDATE_TIME) {
            float rate = this->m_ivelTimer * 29.999998f;
            this->m_ivelTimer = zero;

            if (this->m_liveList.Count()) {
                this->m_parentVelocity = C3Vector(this->m_xform.d0 - this->m_parentPosition.x, this->m_xform.d1 - this->m_parentPosition.y, this->m_xform.d2 - this->m_parentPosition.z);

                float scale = 1.0f / rate * this->m_ivelScale;
                this->m_parentVelocity.x = this->m_parentVelocity.x * scale;
                this->m_parentVelocity.y = scale * this->m_parentVelocity.y;
                this->m_parentVelocity.z = scale * this->m_parentVelocity.z;
            } else {
                this->m_parentVelocity.x = zero;
                this->m_parentVelocity.y = zero;
                this->m_parentVelocity.z = zero;
            }
        }
    }

    this->InternalUpdate(deltaTime, 0);
    this->m_flags |= 0x80;

    if (this->m_params.m_hasModel == CParticleEmitter2::PT_MODEL) {
        this->PlaceParticleModels(frameOfReference);
    }
}

// VERIFY: 3d6eeb8b auto
// OFFSET: 0x0097EDF0
void CParticleEmitter2::Sync() {
    float count = (this->m_params.m_lifeVariation + this->m_params.m_life) * (this->m_params.m_emissionRateVariation + this->m_params.m_emissionRate) * 1.15f;
    this->SyncAllocation(static_cast<uint32_t>(count));

    for (uint32_t i = 0; i < this->m_childEmitterCount; i++) {
        CParticleEmitter2* child = this->m_childEmitters[i];
        uint32_t childCount = static_cast<uint32_t>(count * ((child->m_params.m_lifeVariation + child->m_params.m_life) * (child->m_params.m_emissionRateVariation + child->m_params.m_emissionRate) * 1.15f));

        if (childCount > 0x1000) {
            childCount = 0x1000;
        }

        child->SyncAllocation(childCount);
    }
}

// VERIFY: 770acaec manual
// OFFSET: 0x0097EEC0
CParticleEmitter2::CParticleEmitter2(const CParticleEmitter2& source, int32_t copyParticles) : m_random(0), m_params(source.m_params) {
    this->m_emitAccumulator = 0.0f;
    this->m_refCount = 1;
    this->m_textureCellShift = source.m_textureCellShift;
    this->m_textureCellWidth = source.m_textureCellWidth;
    this->m_textureCellHeight = source.m_textureCellHeight;
    this->m_priorityPlane = source.m_priorityPlane;
    this->m_emitterType = source.m_emitterType;
    this->m_childEmitterCount = 4;
    this->m_vertsPerParticle = source.m_vertsPerParticle;
    this->m_indicesPerParticle = source.m_indicesPerParticle;
    this->m_textureRows = source.m_textureRows;
    this->m_textureCols = source.m_textureCols;
    this->unused_012C = source.unused_012C;
    this->m_alphaScale = source.m_alphaScale;
    this->m_flags = source.m_flags;
    this->m_twinkleFPS = source.m_twinkleFPS;
    this->m_twinkleOnOff = source.m_twinkleOnOff;
    this->m_twinkleScaleBase = source.m_twinkleScaleBase;
    this->m_twinkleScaleSpan = source.m_twinkleScaleSpan;
    this->m_ivelScale = source.m_ivelScale;
    this->m_tumbleBaseX = source.m_tumbleBaseX;
    this->m_tumbleSpanX = source.m_tumbleSpanX;
    this->m_tumbleBaseY = source.m_tumbleBaseY;
    this->m_tumbleSpanY = source.m_tumbleSpanY;
    this->m_tumbleBaseZ = source.m_tumbleBaseZ;
    this->m_tumbleSpanZ = source.m_tumbleSpanZ;
    this->m_drag = source.m_drag;
    this->m_windVectorX = source.m_windVectorX;
    this->m_windVectorY = source.m_windVectorY;
    this->m_windVectorZ = source.m_windVectorZ;
    this->m_windTime = source.m_windTime;
    this->m_followIntercept = source.m_followIntercept;
    this->m_followSlope = source.m_followSlope;
    this->m_ivelTimer = source.m_ivelTimer;
    this->m_worldBounds.b.x = 0.0f;
    this->m_worldBounds.b.y = 0.0f;
    this->m_worldBounds.b.z = 0.0f;
    this->m_worldBounds.t.x = 0.0f;
    this->m_worldBounds.t.y = 0.0f;
    this->m_worldBounds.t.z = 0.0f;
    this->unused_0230 = 0;

    this->m_random.SetSeed((rand() << 16) | static_cast<uint16_t>(rand()));
    this->m_childEmitterCount = source.m_childEmitterCount;

    for (uint32_t i = 0; i < source.m_childEmitterCount; i++) {
        this->m_childEmitters[i] = ParticleSystemManager::GetInstance()->DuplicateEmitter(source.m_childEmitters[i], 0);
    }

    this->m_model = source.m_model;

    if (this->m_model) {
        this->m_model->m_refCount++;
    }

    this->m_childModel = source.m_childModel;

    if (this->m_childModel) {
        this->m_childModel->m_refCount++;
    }

    this->m_texture = reinterpret_cast<CTexture*>(HandleDuplicate(reinterpret_cast<HOBJECT>(source.m_texture)));
    this->m_flags &= ~0x80;

    if (!copyParticles || (!source.m_freeList.m_count && !source.m_liveList.m_count)) {
        return;
    }

    this->Sync();
    this->m_freeList.m_count = 0;
    this->m_liveList.m_count = 0;

    for (uint32_t i = 0; i < source.m_freeList.m_count; i++) {
        *this->m_freeList.New() = source.m_freeList.m_data[i];
    }

    uint32_t liveCount = source.m_liveList.m_count;

    if (this->m_params.m_hasModel) {
        for (uint32_t i = 0; i < liveCount; i++) {
            uint32_t index = source.m_liveList.m_data[i];
            *this->m_liveList.New() = index;

            CParticle2_Model& particle = this->m_particleModels.m_data[index];
            const CParticle2_Model& sourceParticle = source.m_particleModels.m_data[index];
            memcpy(&particle, &sourceParticle, offsetof(CParticle2_Model, m_model));
            particle.m_model = sourceParticle.m_model;

            if (particle.m_model) {
                particle.m_model->m_refCount++;
            }
        }

        return;
    }

    for (uint32_t i = 0; i < liveCount; i++) {
        uint32_t index = source.m_liveList.m_data[i];
        *this->m_liveList.New() = index;
        memcpy(&this->m_particles.m_data[index], &source.m_particles.m_data[index], sizeof(CParticle2));
    }
}

// VERIFY: 7bb8d416 auto
// OFFSET: 0x00632050
void CParticleEmitter2::DestroyParticle(CParticle2& particle) {
}

// VERIFY: acb8380a auto
// OFFSET: 0x009797F0
CParticle2_Model::CParticle2_Model(const CParticle2_Model& source) {
    this->m_age = source.m_age;
    this->m_position = source.m_position;
    this->m_velocity = source.m_velocity;
    this->m_lifeSeed = source.m_lifeSeed;
    this->m_randomSeed = source.m_randomSeed;
    this->m_orientation = source.m_orientation;
    this->m_angularVelocity = source.m_angularVelocity;
    this->m_model = source.m_model;

    if (this->m_model) {
        this->m_model->m_refCount++;
    }
}

// INLINED
CParticle2_Model::CParticle2_Model() {
    this->m_model = nullptr;
    this->m_orientation.x = 0.0f;
    this->m_orientation.y = 0.0f;
    this->m_orientation.z = 0.0f;
    this->m_orientation.w = 1.0f;
}

// INLINED
CParticle2_Model::~CParticle2_Model() {
    if (this->m_model) {
        this->m_model->Release();
        this->m_model = nullptr;
    }
}

// VERIFY: 378fed4b auto
// OFFSET: 0x007A0C70
void CSortableParticleQueue::Erase(uint32_t index, uint32_t count) {
    if (index >= this->m_count) {
        return;
    }

    if (index + count > this->m_count) {
        count = this->m_count - index;
    }

    uint32_t tail = this->m_count - index - count;

    if (tail) {
        memmove(this->m_data + sizeof(CSortableParticleRecord) * index, this->m_data + sizeof(CSortableParticleRecord) * (index + count), sizeof(CSortableParticleRecord) * tail);
    }

    this->m_count -= count;
}

// VERIFY: 4f99cb2d manual
// OFFSET: 0x007A0F50
void CSortableParticleQueue::Push(float depth, CParticle2* particle) {
    if (this->m_count + 1 <= this->m_capacity) {
        this->m_count++;
    } else if (this->m_chunk) {
        uint32_t grow = this->m_chunk;

        if (grow <= this->m_count - this->m_capacity + 1) {
            grow = this->m_count - this->m_capacity + 1;
        }

        if (this->Resize(sizeof(CSortableParticleRecord) * (grow + this->m_capacity), true)) {
            this->m_capacity += grow;
            this->m_count++;
        }
    }

    CSortableParticleRecord* data = reinterpret_cast<CSortableParticleRecord*>(this->m_data);
    uint32_t index = this->m_count - 1;

    if (this->m_count != 1 && index != 1) {
        uint32_t parent;

        do {
            data = reinterpret_cast<CSortableParticleRecord*>(this->m_data);
            parent = index >> 1;

            if (depth < data[parent].m_depth) {
                break;
            }

            data[index] = data[parent];
            index >>= 1;
        } while (parent > 1);
    }

    data = reinterpret_cast<CSortableParticleRecord*>(this->m_data);
    data[index].m_depth = depth;
    data[index].m_particle = particle;
}

// VERIFY: 15566d0e auto
// OFFSET: 0x00981310
CPlaneParticleEmitter::CPlaneParticleEmitter() {
    this->m_width = 0.0f;
    this->m_height = 0.0f;
    this->m_emitterType = PET_PLANE_EMITTER;
    this->m_latitude = 0.0f;
    this->m_longitude = 0.0f;
}

// VERIFY: 7803105c auto
// OFFSET: 0x00981350
CPlaneParticleEmitter::CPlaneParticleEmitter(const CPlaneParticleEmitter& source, int32_t copyParticles) : CParticleEmitter2(source, copyParticles) {
    this->m_width = source.m_width;
    this->m_height = source.m_height;
    this->m_latitude = source.m_latitude;
    this->m_longitude = source.m_longitude;
}

// VERIFY: f9a887d1 auto
// OFFSET: 0x009813B0
void CPlaneParticleEmitter::SetWidth(float width) {
    this->m_width = width;
}

// VERIFY: 1eb2e6bb auto
// OFFSET: 0x009813C0
void CPlaneParticleEmitter::SetHeight(float height) {
    this->m_height = height;
}

// VERIFY: bedfe79d auto
// OFFSET: 0x009813D0
void CPlaneParticleEmitter::SetLatitude(float latitude) {
    this->m_latitude = latitude;
}

// VERIFY: 7690086b auto
// OFFSET: 0x009813E0
void CPlaneParticleEmitter::SetLongitude(float longitude) {
    this->m_longitude = longitude;
}

// VERIFY: dfd2839f auto
// OFFSET: 0x009813F0
CSphereParticleEmitter::CSphereParticleEmitter() {
    this->m_minRadius = 0.0f;
    this->m_maxRadius = 0.0f;
    this->m_emitterType = PET_SPHERE_EMITTER;
    this->m_latitude = 0.0f;
    this->m_longitude = 0.0f;
}

// VERIFY: cc657858 auto
// OFFSET: 0x00981430
CSphereParticleEmitter::CSphereParticleEmitter(const CSphereParticleEmitter& source, int32_t copyParticles) : CParticleEmitter2(source, copyParticles) {
    this->m_minRadius = source.m_minRadius;
    this->m_maxRadius = source.m_maxRadius;
    this->m_latitude = source.m_latitude;
    this->m_longitude = source.m_longitude;
    this->m_radiusSpan = source.m_radiusSpan;
}

// VERIFY: a44879a5 auto
// OFFSET: 0x00981490
void CSphereParticleEmitter::SetWidth(float width) {
    this->m_minRadius = width;
    this->m_radiusSpan = this->m_maxRadius - width;
}

// VERIFY: 0f597e92 auto
// OFFSET: 0x009814B0
void CSphereParticleEmitter::SetHeight(float height) {
    this->m_maxRadius = height;
    this->m_radiusSpan = height - this->m_minRadius;
}

// VERIFY: be59e513 auto
// OFFSET: 0x009813E0
void CSphereParticleEmitter::SetLatitude(float latitude) {
    this->m_latitude = latitude;
}

// VERIFY: 23c64fef auto
// OFFSET: 0x009814D0
void CSphereParticleEmitter::SetLongitude(float longitude) {
    this->m_longitude = longitude;
}

// VERIFY: 32c67748 auto
// OFFSET: 0x009814E0
void CSplineParticleEmitter::SetEmissionRate(float rate) {
    this->m_ratePerUnit = rate;
    this->m_params.m_emissionRate = rate * this->m_end;
}

// VERIFY: 187f9f95 auto
// OFFSET: 0x00981500
void CSplineParticleEmitter::SetSpline(const C3Vector* points, uint32_t count) {
    this->m_spline.SetPoints(points, count);
}

// VERIFY: e4738c7d manual
// OFFSET: 0x00981510
CParticleEmitter2* CPlaneParticleEmitter::Clone(int32_t copyParticles) {
    void* mem = SMemAlloc(sizeof(CPlaneParticleEmitter), __FILE__, __LINE__, 0);

    return mem ? new (mem) CPlaneParticleEmitter(*this, copyParticles) : nullptr;
}

// VERIFY: 1e5e8bbd manual
// OFFSET: 0x00981550
CParticleEmitter2* CSphereParticleEmitter::Clone(int32_t copyParticles) {
    void* mem = SMemAlloc(sizeof(CSphereParticleEmitter), __FILE__, __LINE__, 0);

    return mem ? new (mem) CSphereParticleEmitter(*this, copyParticles) : nullptr;
}

// VERIFY: 0c4d4b72 manual
// OFFSET: 0x00981590
CPlaneParticleEmitter::~CPlaneParticleEmitter() {
}

// VERIFY: b135d8e8 manual
// OFFSET: 0x009815C0
void CPlaneParticleEmitter::CreateParticle(CParticle2& particle, float time, const C44Matrix& transform) {
    particle.m_age = CRandom::ufloat_(this->m_random) * time;
    particle.m_randomSeed = static_cast<uint16_t>(CRandom::uint32_(this->m_random));

    float heightScale = CRandom::sfloat_(this->m_random);
    float widthScale = CRandom::sfloat_(this->m_random);

    particle.m_position.x = widthScale * this->m_width * 0.5f;
    particle.m_position.y = 0.5f * (this->m_height * heightScale);
    particle.m_position.z = 0.0f;

    float speed = this->CalcVelocity();
    C3Vector velocity;

    if (0.0f == this->m_params.m_zsource) {
        float latitude = CRandom::sfloat_(this->m_random) * this->m_latitude;
        float longitude = CRandom::sfloat_(this->m_random) * this->m_longitude;
        float cosLatitude = cos(latitude);
        float sinLatitude = sin(latitude);
        float cosLongitude = cos(longitude);
        float sinLongitude = sin(longitude);

        velocity.x = cosLongitude * sinLatitude * speed;
        velocity.y = sinLatitude * sinLongitude * speed;
        velocity.z = speed * ((cosLongitude + 1.0f - cosLongitude) * cosLatitude);
    } else {
        C3Vector direction(particle.m_position.x, particle.m_position.y, particle.m_position.z - this->m_params.m_zsource);
        float scale = speed / sqrt(direction.y * direction.y + direction.x * direction.x + direction.z * direction.z);
        velocity.x = direction.x * scale;
        velocity.y = direction.y * scale;
        velocity.z = scale * direction.z;
    }

    if (this->m_flags & 0x200) {
        particle.m_velocity = velocity;
    } else {
        particle.m_velocity.x = transform.b0 * velocity.y + transform.c0 * velocity.z + transform.a0 * velocity.x;
        particle.m_velocity.y = transform.b1 * velocity.y + transform.a1 * velocity.x + transform.c1 * velocity.z;
        particle.m_velocity.z = velocity.z * transform.c2 + (velocity.y * transform.b2 + velocity.x * transform.a2);
        particle.m_position *= transform;

        if (this->m_flags & 0x40000) {
            this->ProjectParticle(particle);
        }
    }

    if (this->m_flags & 0x800) {
        float scale = CRandom::sfloat_(this->m_random) * this->m_params.m_variation + 1.0f;
        particle.m_velocity.x = this->m_parentVelocity.x * scale + particle.m_velocity.x;
        particle.m_velocity.y = this->m_parentVelocity.y * scale + particle.m_velocity.y;
        particle.m_velocity.z = scale * this->m_parentVelocity.z + particle.m_velocity.z;
    }
}

// VERIFY: 53c68299 manual
// OFFSET: 0x00981920
CSphereParticleEmitter::~CSphereParticleEmitter() {
}

// VERIFY: 681f68f1 manual
// OFFSET: 0x00981950
void CSphereParticleEmitter::CreateParticle(CParticle2& particle, float time, const C44Matrix& transform) {
    particle.m_age = CRandom::ufloat_(this->m_random) * time;
    particle.m_randomSeed = static_cast<uint16_t>(CRandom::uint32_(this->m_random));

    float radius = CRandom::ufloat_(this->m_random) * this->m_radiusSpan + this->m_minRadius;
    float latitude = CRandom::sfloat_(this->m_random) * this->m_latitude;
    float longitude = CRandom::sfloat_(this->m_random) * this->m_longitude;
    float cosLatitude = cos(latitude);
    float sinLatitude = sin(latitude);
    float cosLongitude = cos(longitude);
    float sinLongitude = sin(longitude);

    C3Vector direction(cosLongitude * cosLatitude, cosLatitude * sinLongitude, sinLatitude);

    particle.m_position.x = direction.x * radius;
    particle.m_position.y = direction.y * radius;
    particle.m_position.z = radius * sinLatitude;

    C3Vector velocity;

    if (0.0f != this->m_params.m_zsource) {
        direction = C3Vector(particle.m_position.x, particle.m_position.y, particle.m_position.z - this->m_params.m_zsource);
        float lengthSq = direction.y * direction.y + direction.x * direction.x + direction.z * direction.z;

        if (lengthSq > 0.00000023841858f) {
            float scale = 1.0f / sqrt(lengthSq);
            direction.x = direction.x * scale;
            direction.y = direction.y * scale;
            direction.z = direction.z * scale;
        }

        velocity = direction;
    } else if (this->m_flags & 0x8000) {
        velocity = C3Vector(0.0f, 0.0f, 1.0f);
    } else {
        velocity = direction;
    }

    float speed = this->CalcVelocity();
    velocity.x = velocity.x * speed;
    velocity.y = velocity.y * speed;
    velocity.z = speed * velocity.z;

    if (this->m_flags & 0x200) {
        particle.m_velocity = velocity;
    } else {
        particle.m_velocity.x = transform.c0 * velocity.z + transform.b0 * velocity.y + transform.a0 * velocity.x;
        particle.m_velocity.y = transform.c1 * velocity.z + transform.b1 * velocity.y + transform.a1 * velocity.x;
        particle.m_velocity.z = velocity.y * transform.b2 + velocity.z * transform.c2 + velocity.x * transform.a2;
        particle.m_position *= transform;

        if (this->m_flags & 0x40000) {
            this->ProjectParticle(particle);
        }
    }

    if (this->m_flags & 0x800) {
        float scale = CRandom::sfloat_(this->m_random) * this->m_params.m_variation + 1.0f;
        particle.m_velocity.x = this->m_parentVelocity.x * scale + particle.m_velocity.x;
        particle.m_velocity.y = this->m_parentVelocity.y * scale + particle.m_velocity.y;
        particle.m_velocity.z = scale * this->m_parentVelocity.z + particle.m_velocity.z;
    }
}

// VERIFY: ebe4b46d auto
// OFFSET: 0x00981C90
void CSplineParticleEmitter::SetWidth(float width) {
    if (width < 0.0f) {
        this->m_start = 0.0f;
    } else if (width < 1.0f) {
        this->m_start = width;
    } else {
        this->m_start = 1.0f;
    }
}

// VERIFY: 68ed2175 manual
// OFFSET: 0x00981CD0
void CSplineParticleEmitter::SetHeight(float height) {
    float end = 0.0f;

    if (height >= 0.0f) {
        end = height;

        if (height >= 1.0f) {
            end = 1.0f;
        }
    }

    if (fabs(end - this->m_end) >= 0.00000023841858f) {
        this->m_end = end;
        this->m_emitAtEnd = 1;
        this->m_params.m_emissionRate = end * this->m_ratePerUnit;
    }
}

// VERIFY: 4d7a6d2b auto
// OFFSET: 0x009813E0
void CSplineParticleEmitter::SetLatitude(float latitude) {
    this->m_latitude = latitude;
}

// VERIFY: 027aa93b auto
// OFFSET: 0x009814D0
void CSplineParticleEmitter::SetLongitude(float longitude) {
    this->m_longitude = longitude;
}

// VERIFY: 4df0155e manual
// OFFSET: 0x00981D40
void CSplineParticleEmitter::CreateParticle(CParticle2& particle, float time, const C44Matrix& transform) {
    particle.m_age = CRandom::ufloat_(this->m_random) * time;
    particle.m_randomSeed = static_cast<uint16_t>(CRandom::uint32_(this->m_random));

    float t;

    if (this->m_emitAtEnd) {
        t = this->m_end;
        this->m_emitAtEnd = 0;
    } else {
        t = (this->m_end - this->m_start) * CRandom::ufloat_(this->m_random) + this->m_start;
    }

    this->m_spline.Pos(t, particle.m_position, C3Spline::EVAL_ARCLENGTH);

    C3Vector velocity(0.0f, 0.0f, this->CalcVelocity());

    if (0.0f == this->m_params.m_zsource) {
        if (0.0f != this->m_latitude) {
            C3Vector up(0.0f, 0.0f, 1.0f);
            C3Vector tangent(0.0f, 0.0f, 0.0f);
            this->m_spline.Vel(t, tangent, C3Spline::EVAL_ARCLENGTH);

            float scale = 1.0f / sqrt(tangent.z * tangent.z + tangent.y * tangent.y + tangent.x * tangent.x);
            tangent.x = tangent.x * scale;
            tangent.y = tangent.y * scale;
            tangent.z = scale * tangent.z;

            if (fabs(tangent.z + (tangent.x + tangent.y) * 0.0f) > 0.89999998f) {
                up.z = 1.0f;
            }

            C33Matrix rotation = C33Matrix::Rotation(CRandom::sfloat_(this->m_random) * this->m_latitude, tangent, true);
            C3Vector direction = up * rotation;

            if (0.0f != this->m_longitude) {
                C3Vector offset(direction.x * this->m_longitude, direction.y * this->m_longitude, this->m_longitude * direction.z);
                float distance = CRandom::ufloat_(this->m_random);
                particle.m_position.x = offset.x * distance + particle.m_position.x;
                particle.m_position.y = offset.y * distance + particle.m_position.y;
                particle.m_position.z = distance * offset.z + particle.m_position.z;
            }

            velocity = C3Vector(direction.x * velocity.z, direction.y * velocity.z, velocity.z * direction.z);
        }
    } else {
        C3Vector direction(particle.m_position.x, particle.m_position.y, particle.m_position.z - this->m_params.m_zsource);
        float scale = velocity.z / sqrt(direction.y * direction.y + direction.x * direction.x + direction.z * direction.z);
        velocity = C3Vector(direction.x * scale, direction.y * scale, scale * direction.z);
    }

    if (this->m_flags & 0x200) {
        particle.m_velocity = velocity;
    } else {
        particle.m_velocity.x = transform.a0 * velocity.x + transform.b0 * velocity.y + transform.c0 * velocity.z;
        particle.m_velocity.y = transform.b1 * velocity.y + transform.a1 * velocity.x + transform.c1 * velocity.z;
        particle.m_velocity.z = velocity.z * transform.c2 + (velocity.x * transform.a2 + velocity.y * transform.b2);
        particle.m_position *= transform;

        if (this->m_flags & 0x40000) {
            this->ProjectParticle(particle);
        }
    }

    if (this->m_flags & 0x800) {
        float scale = CRandom::sfloat_(this->m_random) * this->m_params.m_variation + 1.0f;
        particle.m_velocity.x = this->m_parentVelocity.x * scale + particle.m_velocity.x;
        particle.m_velocity.y = this->m_parentVelocity.y * scale + particle.m_velocity.y;
        particle.m_velocity.z = scale * this->m_parentVelocity.z + particle.m_velocity.z;
    }
}

// VERIFY: 48680bae manual
// OFFSET: 0x009820F0
CSplineParticleEmitter::CSplineParticleEmitter() {
    this->m_start = 0.0f;
    this->m_end = 0.0f;
    this->m_latitude = 0.0f;
    this->m_emitterType = PET_SPLINE_EMITTER;
    this->m_longitude = 0.0f;
    this->m_emitAtEnd = 0;
    this->m_ratePerUnit = 0.0f;
}

// VERIFY: e9bf0ccf manual
// OFFSET: 0x00982150
CSplineParticleEmitter::~CSplineParticleEmitter() {
}

// VERIFY: fdc47bd6 manual
// OFFSET: 0x009821C0
CSplineParticleEmitter::CSplineParticleEmitter(const CSplineParticleEmitter& source, int32_t copyParticles) : CParticleEmitter2(source, copyParticles) {
    this->m_start = source.m_start;
    this->m_end = source.m_end;
    this->m_latitude = source.m_latitude;
    this->m_longitude = source.m_longitude;
    this->m_ratePerUnit = source.m_ratePerUnit;
    this->m_emitAtEnd = source.m_emitAtEnd;
    this->m_spline = source.m_spline;
}

// VERIFY: cb65ff4c manual
// OFFSET: 0x00982240
CParticleEmitter2* CSplineParticleEmitter::Clone(int32_t copyParticles) {
    void* mem = SMemAlloc(sizeof(CSplineParticleEmitter), __FILE__, __LINE__, 0);

    return mem ? new (mem) CSplineParticleEmitter(*this, copyParticles) : nullptr;
}
