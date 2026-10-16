#ifndef WOW_SOURCE_WORLDCLIENT_LIQUIDWAVEMANAGER_H
#define WOW_SOURCE_WORLDCLIENT_LIQUIDWAVEMANAGER_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/LiquidSystem/Liquid.h"

namespace Liquid {
    struct SWave {
        /* 0000 */ float m_x;
        /* 0004 */ float m_y;
        /* 0008 */ float m_dirX;
        /* 000C */ float m_dirY;
        /* 0010 */ float m_speed;
        /* 0014 */ float m_length;
        /* 0018 */ float m_amplitude;
        /* 001C */ float unk_001C;
    };

    struct SWaveData {
        /* 0000 */ uint8_t unused_0000[72];
        /* 0048 */ SWave m_waves[3];
    };

    class CWaveManager : public IWaveManager {
        public:
        static float s_fadeOutTime;
        static float s_fadeInTime;

        static CWaveManager* Create();

        /* 0004 */ int32_t m_refCount;
        /* 0008 */ SWaveData m_data;
        /* 00B0 */ float m_time[3];
        /* 00BC */ float m_duration[3];
        /* 00C8 */ SWave m_waves[3];

        virtual void AddRef();
        virtual void Release();
        virtual void* GetData();
        virtual int32_t GetCount();

        CWaveManager();
        void PrepareRenderTransform(const C3Vector* camPos);
    };

    extern CDataAllocator s_waveManagerAllocator;

    void ClearWaveManagers();
}

#endif
