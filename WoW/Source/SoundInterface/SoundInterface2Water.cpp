#include <cstddef>
#include <cstdint>
#include "WoW/Source/SoundInterface/SoundInterface2Water.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/LiquidTypeRec.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/WorldClient/World.h"

static const float FADEINTIME = 1.0f;
static const float FADEOUTTIME = 1.0f;

// STATIC: 0x00B4AF5C
static SI2WaterSound s_nearestWaterSounds[1];

// STATIC: 0x00B4AF8C
static SI2WaterSound s_waterSounds[1];

// VERIFY: c873f424 manual
// OFFSET: 0x004D20B0
static void _ShiftNearestWaterSounds(int32_t index) {
    for (int32_t i = sizeof(s_nearestWaterSounds) / sizeof(s_nearestWaterSounds[0]) - 2; i >= index; i--) {
        s_nearestWaterSounds[i + 1].m_distanceSq = s_nearestWaterSounds[i].m_distanceSq;
        s_nearestWaterSounds[i + 1].m_position = s_nearestWaterSounds[i].m_position;
        s_nearestWaterSounds[i + 1].m_soundKitId = s_nearestWaterSounds[i].m_soundKitId;
    }
}

// VERIFY: e3f0bac6 manual
// OFFSET: 0x004D21B0
static void _AddNearestWaterSound(float distanceSq, C3Vector position, const LiquidTypeRec* liquidType) {
    for (int32_t i = 0; i < sizeof(s_nearestWaterSounds) / sizeof(s_nearestWaterSounds[0]); i++) {
        SI2WaterSound& sound = s_nearestWaterSounds[i];

        if (sound.m_distanceSq >= 0.0f && sound.m_distanceSq <= distanceSq) {
            continue;
        }

        _ShiftNearestWaterSounds(i);
        sound.m_distanceSq = distanceSq;
        sound.m_position = position;
        sound.m_soundKitId = liquidType->m_soundID;
        return;
    }
}

// VERIFY: ee326ccd auto
// OFFSET: 0x004D2230
static int32_t _OkToQueryLiquidSounds() {
    // STATIC: 0x00B4AF4C
    static uint32_t s_lastTime = OsGetAsyncTimeMs();

    uint32_t now = OsGetAsyncTimeMs();
    uint32_t elapsed = now - s_lastTime;
    s_lastTime = now;

    // STATIC: 0x00AC3C74
    static uint32_t s_timeSinceQuery = 999999;

    s_timeSinceQuery += elapsed;

    if (s_timeSinceQuery < 500) {
        return 0;
    }

    s_timeSinceQuery = 0;

    // STATIC: 0x00B4AF48
    static CVar* s_enableAllSound = CVar::Lookup("Sound_EnableAllSound");
    // STATIC: 0x00B4AF44
    static CVar* s_enableAmbience = CVar::Lookup("Sound_EnableAmbience");

    if (!s_enableAllSound || !s_enableAllSound->m_intValue || !s_enableAmbience || !s_enableAmbience->m_intValue) {
        return 0;
    }

    if (CGGameUI::m_inMovie) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !player->m_worldObject) {
        return 0;
    }

    // STATIC: 0x00B4AF38
    static C3Vector s_lastPosition;

    C3Vector position = player->GetPosition();

    if (position == s_lastPosition) {
        return 0;
    }

    s_lastPosition = position;
    return 1;
}

// VERIFY: 3a676ac1 manual
// OFFSET: 0x004D2390
int32_t SI2::HandleWaterSounds(const void* data, void* param) {
    if (SI2::IsUnderwater()) {
        SI2::StopSound(&s_nearestWaterSounds[0].m_object, 0, -1.0f, 1);
        return 1;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !player->m_worldObject || !_OkToQueryLiquidSounds()) {
        return 1;
    }

    for (uint32_t i = 0; i < sizeof(s_nearestWaterSounds) / sizeof(s_nearestWaterSounds[0]); i++) {
        s_nearestWaterSounds[i].m_soundKitId = 0;
        s_nearestWaterSounds[i].m_distanceSq = -1.0f;
    }

    C3Vector position = player->GetPosition();
    CMapEntity* entity = player->m_worldObject;
    TSGrowableArray<World::LiquidQueryResult> results;

    World::QueryLiquidSounds(entity, 60.0f, results);

    for (uint32_t i = 0; i < results.Count(); i++) {
        LiquidTypeRec* liquidType = g_liquidTypeDB.GetRecord(results[i].m_liquidType);

        if (!(liquidType->m_flags & 0x80) || !entity || !World::QueryObjectInside(entity)) {
            C3Vector offset = results[i].m_offset;
            _AddNearestWaterSound(offset.x * offset.x + offset.y * offset.y, C3Vector(position.x + offset.x, position.y + offset.y, position.z + offset.z), liquidType);
        }
    }

    for (uint32_t i = 0; i < sizeof(s_waterSounds) / sizeof(s_waterSounds[0]); i++) {
        s_waterSounds[i].m_valid = 0;
    }

    for (uint32_t n = 0; n < sizeof(s_nearestWaterSounds) / sizeof(s_nearestWaterSounds[0]); n++) {
        SI2WaterSound& nearest = s_nearestWaterSounds[n];
        nearest.m_valid = 0;

        for (uint32_t i = 0; i < sizeof(s_waterSounds) / sizeof(s_waterSounds[0]); i++) {
            if (s_waterSounds[i].m_soundKitId == nearest.m_soundKitId && s_waterSounds[i].m_position == nearest.m_position) {
                nearest.m_valid = 1;
                s_waterSounds[i].m_valid = 1;
                break;
            }
        }

        for (uint32_t i = 0; i < sizeof(s_waterSounds) / sizeof(s_waterSounds[0]); i++) {
            if (!nearest.m_valid && s_waterSounds[i].m_soundKitId == nearest.m_soundKitId && (s_waterSounds[i].m_position - nearest.m_position).SquaredMag() < 400.0f) {
                s_waterSounds[i].m_position = nearest.m_position;
                nearest.m_valid = 1;
                s_waterSounds[i].m_valid = 1;
                s_waterSounds[i].m_moved = 1;
            }
        }
    }

    for (uint32_t i = 0; i < sizeof(s_waterSounds) / sizeof(s_waterSounds[0]); i++) {
        if (!s_waterSounds[i].m_valid) {
            SI2::StopSound(&s_waterSounds[i].m_object, 0, FADEOUTTIME, 1);
        }
    }

    for (uint32_t n = 0; n < sizeof(s_nearestWaterSounds) / sizeof(s_nearestWaterSounds[0]); n++) {
        SI2WaterSound& nearest = s_nearestWaterSounds[n];

        if (nearest.m_valid) {
            continue;
        }

        for (uint32_t i = 0; i < sizeof(s_waterSounds) / sizeof(s_waterSounds[0]); i++) {
            if (!s_waterSounds[i].m_valid) {
                s_waterSounds[i].m_position = nearest.m_position;
                s_waterSounds[i].m_soundKitId = nearest.m_soundKitId;
                break;
            }
        }
    }

    for (uint32_t i = 0; i < sizeof(s_waterSounds) / sizeof(s_waterSounds[0]); i++) {
        SI2WaterSound& sound = s_waterSounds[i];

        if (!SI2::IsPlaying(&sound.m_object)) {
            SE2SoundKitProperties properties;
            properties.ResetToDefaults();
            properties.m_fadeInTime = FADEINTIME;
            properties.m_group = 2;
            properties.m_loopMode = 1;
            SI2::PlaySoundKit(sound.m_soundKitId, &sound.m_position, &sound.m_object, &properties, 0, nullptr, 1, 0);
        } else if (sound.m_moved) {
            SI2::Set3DPosition(&sound.m_object, &sound.m_position);
        }
    }

    return 1;
}

// VERIFY: cf194119 auto
// OFFSET: 0x004D2750
void SI2::WaterShutdown() {
    SI2::StopSound(&s_nearestWaterSounds[0].m_object, 0, -1.0f, 1);
    EventUnregister(EVENT_ID_IDLE, SI2::HandleWaterSounds);
}
