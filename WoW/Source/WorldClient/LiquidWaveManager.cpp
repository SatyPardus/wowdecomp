#include "Engine/Source/Base/CRTRandom.h"
#include "LiquidWaveManager.h"
#include <cstring>
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include <cmath>
#include <cstdlib>
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/LiquidSystem/Liquid.h"

// STATIC: 0x00D31A30
CDataAllocator Liquid::s_waveManagerAllocator(sizeof(Liquid::CWaveManager), 0x10);

// STATIC: 0x00AF1690
float Liquid::CWaveManager::s_fadeOutTime = 5000.0f;

// STATIC: 0x00AF1694
float Liquid::CWaveManager::s_fadeInTime = 5000.0f;

// VERIFY: ba8e6430 manual
// OFFSET: 0x007D62A0
void Liquid::CWaveManager::PrepareRenderTransform(const C3Vector* camPos) {
    static int32_t s_lastFrame;

    if (s_lastFrame == CWorld::frameCnt) {
        return;
    }

    s_lastFrame = CWorld::frameCnt;

    static uint32_t s_lastTime = FrameTime::s_curTimeMs;
    uint32_t elapsed = FrameTime::s_curTimeMs - s_lastTime;
    s_lastTime = FrameTime::s_curTimeMs;

    for (uint32_t i = 0; i < 3; i++) {
        float time = elapsed + this->m_time[i];
        this->m_time[i] = time;

        float fadeIn = time / Liquid::CWaveManager::s_fadeInTime;

        if (fadeIn > 1.0f) {
            fadeIn = 1.0f;
        }

        float fadeOut;

        if (time >= this->m_duration[i]) {
            fadeOut = 0.0f;
        } else {
            fadeOut = (this->m_duration[i] - time) / Liquid::CWaveManager::s_fadeOutTime;

            if (fadeOut > 1.0f) {
                fadeOut = 1.0f;
            }
        }

        float fade = fadeOut <= fadeIn ? fadeOut : fadeIn;

        if (fade < 0.0f) {
            fade = 0.0f;
        }

        this->m_data.m_waves[i].m_amplitude = fade * this->m_waves[i].m_amplitude;

        if (this->m_time[i] >= this->m_duration[i]) {
            SWave& wave = this->m_waves[i];

            this->m_time[i] = 0.0f;
            this->m_duration[i] = static_cast<float>(ClientRand() % 20000 + 20000);

            C3Vector* target = CWorldScene::GetCamTarget();
            float toTargetX = target->x - camPos->x;
            float toTargetY = target->y - camPos->y;
            float toTargetZ = target->z - camPos->z;
            float lengthSq = toTargetZ * toTargetZ + toTargetX * toTargetX + toTargetY * toTargetY;

            if (lengthSq > 0.00000023841858f) {
                float inv = 1.0f / sqrtf(lengthSq);
                toTargetX = toTargetX * inv;
                toTargetY = inv * toTargetY;
            }

            float distX = (ClientRand() % 50 + 150.0f) * toTargetX;
            float x = (ClientRand() % 100 - 50) + camPos->x + distX;
            float distY = (ClientRand() % 50 + 150.0f) * toTargetY;
            float y = (ClientRand() % 100 - 50) + distY + camPos->y;
            wave.m_x = x;
            wave.m_y = y;

            float dirX = camPos->x - x;
            float dirY = camPos->y - y;
            float dirLengthSq = dirX * dirX + dirY * dirY;

            if (dirLengthSq > 0.00000023841858f) {
                float inv = 1.0f / sqrtf(dirLengthSq);
                dirX = dirX * inv;
                dirY = dirY * inv;
            }

            C44Matrix rotation;
            rotation.RotateAroundZ((ClientRand() % 2000 - 1000.0f) * 0.00078539818f);
            C3Vector dir = C3Vector(dirX, dirY, 0.0f) * rotation;
            wave.m_dirX = dir.x;
            wave.m_dirY = dir.y;

            wave.m_speed = (ClientRand() % 1000) * 0.039999999f + 10.0f;
            wave.m_length = (ClientRand() % 1000) * 0.2f + 200.0f;
            wave.m_amplitude = (ClientRand() % 1000) * 0.00050000002f + 0.5f;
            wave.unk_001C = (ClientRand() % 1000) * 0.000030000001f + 0.02f;

            this->m_data.m_waves[i] = wave;
            this->m_data.m_waves[i].m_amplitude = 0.0f;
        }
    }
}

// VERIFY: e6c1fe21 auto
// OFFSET: 0x007D6670
void Liquid::ClearWaveManagers() {
    Liquid::s_waveManagerAllocator.Clear(".?AVCWaveManager@Liquid@@", SLOG_OBJECT, 1);
}

// VERIFY: 845a53cd auto
// OFFSET: 0x00530B70
void Liquid::CWaveManager::AddRef() {
    this->m_refCount++;
}

// VERIFY: bd870ef6 auto
// OFFSET: 0x007D6200
int32_t Liquid::CWaveManager::GetCount() {
    return 42;
}

// VERIFY: fffbab63 manual
// OFFSET: 0x007D6210
void Liquid::CWaveManager::Release() {
    if (this->m_refCount-- == 1) {
        this->~CWaveManager();
        Liquid::s_waveManagerAllocator.PutData(this, nullptr, 0);
    }
}

// VERIFY: e6866435 manual
// OFFSET: 0x007D6240
Liquid::CWaveManager::CWaveManager() {
    this->m_refCount = 1;
    memset(&this->m_data, 0, sizeof(this->m_data));

    for (uint32_t i = 0; i < 3; i++) {
        this->m_time[i] = 0.0f;
        this->m_duration[i] = 0.0f;
    }

    memset(this->m_waves, 0, sizeof(this->m_waves));
}

// VERIFY: 8981a733 auto
// OFFSET: 0x007D6640
Liquid::CWaveManager* Liquid::CWaveManager::Create() {
    void* mem = Liquid::s_waveManagerAllocator.GetData(0, ".?AVCWaveManager@Liquid@@", SLOG_OBJECT);
    return mem ? new (mem) Liquid::CWaveManager() : nullptr;
}

// VERIFY: 79404128 auto
// OFFSET: 0x008C6C80
void* Liquid::CWaveManager::GetData() {
    return &this->m_data;
}
