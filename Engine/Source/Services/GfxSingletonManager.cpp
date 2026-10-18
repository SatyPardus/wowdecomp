#include "Engine/Source/Base/CRTRandom.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include "Engine/Source/Services/GfxSingletonManager.h"
#include "Engine/Source/Services/ParticleSystem.h"
#include "Engine/Source/Services/ParticleSystem2.h"
#include "Engine/Source/Services/RibbonEmitter.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CRandom.h"

// STATIC: 0x00B2D678
static float s_scaler = 1.0f;

// STATIC: 0x00DCE8CC
ParticleSystemManager* ParticleSystemManager::s_instance;

// STATIC: 0x00DCE8D0
RibbonManager* RibbonManager::s_instance;

// VERIFY: 824e6114 auto
// OFFSET: 0x00980ED0
float ParticleSystemManager::GetScaler() {
    return s_scaler;
}

// VERIFY: 5b9d99cc auto
// OFFSET: 0x00980F70
void ParticleSystemManager::SetScaler(float scaler) {
    if (scaler < 0.0f) {
        s_scaler = 0.0f;
    } else if (scaler < 1.0f) {
        s_scaler = scaler;
    } else {
        s_scaler = 1.0f;
    }
}

// VERIFY: d3a1f51e manual
// OFFSET: 0x00981000
void ParticleSystemManager::Flush() {
    for (uint32_t i = this->m_emitters.Count(); i; i--) {
        this->m_emitters[i - 1]->DecRef();
    }

    this->m_emitters.m_count = 0;

    for (uint32_t i = this->m_emitters2.Count(); i; i--) {
        this->m_emitters2[i - 1]->DecRef();
    }

    this->m_emitters2.m_count = 0;

    for (uint32_t i = this->m_pendingEmitters.Count(); i; i--) {
        this->m_pendingEmitters[i - 1]->DecRef();
    }

    this->m_pendingEmitters.m_count = 0;

    for (uint32_t i = this->m_pendingEmitters2.Count(); i; i--) {
        this->m_pendingEmitters2[i - 1]->DecRef();
    }

    this->m_pendingEmitters2.m_count = 0;
}

// VERIFY: 2e8d51c6 auto
// OFFSET: 0x00981090
CParticleEmitter2* ParticleSystemManager::DuplicateEmitter(const CParticleEmitter2* emitter, int32_t copyParticles) {
    if (!emitter) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    CParticleEmitter2* duplicate = const_cast<CParticleEmitter2*>(emitter)->Clone(copyParticles);
    duplicate->AddRef();
    *this->m_emitters2.New() = duplicate;

    return duplicate;
}

// VERIFY: 9ed5c945 auto
// OFFSET: 0x009810E0
void RibbonManager::Flush() {
    for (uint32_t i = this->m_emitters.Count(); i; i--) {
        this->m_emitters[i - 1]->DecRef();
    }

    this->m_emitters.m_count = 0;

    for (uint32_t i = this->m_pendingEmitters.Count(); i; i--) {
        this->m_pendingEmitters[i - 1]->DecRef();
    }

    this->m_pendingEmitters.m_count = 0;
}

// VERIFY: 61e3c1a1 manual
// OFFSET: 0x00981130
ParticleSystemManager* ParticleSystemManager::GetInstance() {
    if (ParticleSystemManager::s_instance) {
        return ParticleSystemManager::s_instance;
    }

    void* mem = SMemAlloc(sizeof(ParticleSystemManager), __FILE__, __LINE__, 0);
    ParticleSystemManager::s_instance = mem ? new (mem) ParticleSystemManager : nullptr;

    CRndSeed seed((static_cast<uint32_t>(ClientRand()) << 16) | static_cast<uint16_t>(ClientRand()));

    for (uint32_t i = 0; i < 128; i++) {
        CParticleEmitter2::s_rndTable[i] = CRandom::ufloat_(seed);
    }

    CParticleEmitter2::Init();

    return ParticleSystemManager::s_instance;
}

// VERIFY: fb3884dd manual
// OFFSET: 0x00981200
ParticleSystemManager::~ParticleSystemManager() {
    this->Flush();
}

// VERIFY: eda66b90 manual
// OFFSET: 0x00981270
void ParticleSystemManager::Destroy() {
    if (ParticleSystemManager::s_instance) {
        CParticleEmitter2::Destroy();
        delete ParticleSystemManager::s_instance;
    }

    ParticleSystemManager::s_instance = nullptr;
}

// VERIFY: ef3840e9 manual
// OFFSET: 0x009812B0
void RibbonManager::Destroy() {
    delete RibbonManager::s_instance;
    RibbonManager::s_instance = nullptr;
}

// INLINED
RibbonManager::~RibbonManager() {
    this->Flush();
}
