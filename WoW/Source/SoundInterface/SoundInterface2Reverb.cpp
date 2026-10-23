#include <cstddef>
#include <cstdint>
#include <cstring>
#include "Engine/Source/SoundEngine/SoundEngine.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"

// STATIC: 0x00B4AF2C
static const char* s_reverbDescription;

// VERIFY: b61935df manual
// OFFSET: 0x004D1EC0
static void GetReverbProperties(SoundProviderPreferencesRec* rec, FMOD_REVERB_PROPERTIES* properties) {
    if (!properties) {
        return;
    }

    FMOD_REVERB_PROPERTIES off = FMOD_PRESET_OFF;

    if (!rec) {
        memcpy(properties, &off, sizeof(FMOD_REVERB_PROPERTIES));
        s_reverbDescription = nullptr;
        return;
    }

    properties->Instance = 0;
    properties->Environment = rec->m_eaxenvironmentSelection;
    properties->EnvSize = rec->m_eax2EnvironmentSize;
    properties->EnvDiffusion = rec->m_eax2EnvironmentDiffusion;
    properties->Room = rec->m_eax2Room;
    properties->RoomHF = rec->m_eax2RoomHf;
    properties->RoomLF = rec->m_eax3RoomLf;
    properties->DecayTime = rec->m_eaxdecayTime;
    properties->DecayHFRatio = rec->m_eax2DecayHfratio;
    properties->DecayLFRatio = rec->m_eax3DecayLfratio;
    properties->Reflections = rec->m_eax2Reflections;
    properties->ReflectionsDelay = rec->m_eax2ReflectionsDelay;
    properties->Reverb = rec->m_eax2Reverb;
    properties->ReverbDelay = rec->m_eax2ReverbDelay;
    properties->EchoTime = rec->m_eax3EchoTime;
    properties->EchoDepth = rec->m_eax3EchoDepth;
    properties->ModulationTime = rec->m_eax3ModulationTime;
    properties->ModulationDepth = rec->m_eax3ModulationDepth;
    properties->AirAbsorptionHF = rec->m_eax2AirAbsorption;
    properties->HFReference = rec->m_eax3Hfreference;
    properties->LFReference = rec->m_eax3Lfreference;
    properties->Flags = FMOD_REVERB_FLAGS_DECAYTIMESCALE |
        FMOD_REVERB_FLAGS_REFLECTIONSSCALE |
        FMOD_REVERB_FLAGS_REFLECTIONSDELAYSCALE |
        FMOD_REVERB_FLAGS_REVERBSCALE |
        FMOD_REVERB_FLAGS_REVERBDELAYSCALE |
        FMOD_REVERB_FLAGS_DECAYHFLIMIT |
        FMOD_REVERB_FLAGS_ECHOTIMESCALE |
        FMOD_REVERB_FLAGS_MODULATIONTIMESCALE |
        FMOD_REVERB_FLAGS_HIGHQUALITYREVERB;
    properties->RoomRolloffFactor = rec->m_eax2RoomRolloff;
    properties->Diffusion = 100.0f;
    properties->Density = 100.0f;
    properties->ReflectionsPan[0] = 0.0f;
    properties->ReflectionsPan[1] = 0.0f;
    properties->ReflectionsPan[2] = 0.0f;
    properties->ReverbPan[0] = 0.0f;
    properties->ReverbPan[1] = 0.0f;
    properties->ReverbPan[2] = 0.0f;

    s_reverbDescription = rec->m_description;
}

// VERIFY: 881e694d manual
// OFFSET: 0x004D2050
void SI2::ApplyReverbSettings() {
    if (!SE2::IsReverbEnabled()) {
        return;
    }

    FMOD_REVERB_PROPERTIES properties;
    GetReverbProperties(g_soundProviderPreferencesDB.GetRecord(SI2::GetZoneReverb()), &properties);
    SE2::SetReverb(&properties);
}

// VERIFY: 8f6ff286 auto
// OFFSET: 0x004D20A0
void SI2::UpdateReverb() {
    SI2::ApplyReverbSettings();
}
