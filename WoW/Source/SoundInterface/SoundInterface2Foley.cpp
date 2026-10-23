#include <cstddef>
#include <cstdint>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/SoundEngine/SoundEngine.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ItemGroupSoundsRec.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"

// STATIC: 0x00B4AE54
static float s_taxiFidgetElapsed;

// STATIC: 0x00B4AE58
static float s_taxiFidgetDelay;

// VERIFY: 88e41fa1 auto
// OFFSET: 0x004CFB50
void SI2::SetPitch(SE2SoundKitObject* object, float pitch) {
    object->m_sound.SetPitch(pitch);
}

// VERIFY: f573d0ba auto
// OFFSET: 0x004CFB70
void SI2::Detach(SE2SoundKitObject* object) {
    object->m_sound.Detach();
}

// VERIFY: f4fa7734 manual
// OFFSET: 0x004CFC10
int32_t SI2::PlayFoleySound(int32_t materialId, const C3Vector* position, int32_t self) {
    MaterialRec* material = g_materialDB.GetRecord(materialId);

    if (!material || !material->m_foleySoundID) {
        return 0;
    }

    C3Vector soundPosition;
    soundPosition.x = position->x;
    soundPosition.y = position->y;
    soundPosition.z = position->z + 2.0f;

    if (!self) {
        return SI2::PlaySoundKit(material->m_foleySoundID, &soundPosition, nullptr, nullptr, 0, nullptr, 1, 0);
    }

    static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");

    int32_t listenerAtCharacter = s_listenerAtCharacter->m_intValue;

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();

    if (listenerAtCharacter) {
        properties.m_volumeScale = 0.65f;
    }

    properties.m_priority = 110;

    return SI2::PlaySoundKit(material->m_foleySoundID, listenerAtCharacter == 0 ? &soundPosition : nullptr, nullptr, &properties, 0, nullptr, 1, 0);
}

// VERIFY: 9dd45739 manual
// OFFSET: 0x004CFE00
int32_t SI2::IsTooCloseToExistingSoundKit(int32_t soundKitId, const C3Vector* position) {
    SEChannelInfo info[512];
    uint32_t hardwareCount;
    uint32_t softwareCount;
    uint32_t virtualCount;
    uint32_t loadingCount;
    uint32_t otherCount;

    int32_t count = SE2::GetChannelInfo(info, 512, 1, 1, 0, 0, &hardwareCount, &softwareCount, &virtualCount, &loadingCount, &otherCount);

    for (int32_t i = 0; i < count; i++) {
        SI2SoundUserData* userData = static_cast<SI2SoundUserData*>(info[i].m_userData);

        if (userData && userData->m_soundKitId == soundKitId) {
            float dx = position->x - info[i].m_position.x;
            float dy = position->y - info[i].m_position.y;
            float dz = position->z - info[i].m_position.z;

            if (dx * dx + dz * dz + dy * dy < 6.0f) {
                return 1;
            }
        }
    }

    return 0;
}

// VERIFY: 4d8f58a6 manual
// OFFSET: 0x004D0020
int32_t SndInterfacePlayItemSound(ITEMSOUNDTYPE type, CGItem_C* item) {
    if (item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) {
        if (type == ITEMSOUND_DROP) {
            return SI2::PlaySoundKit("(DONOTRENAME)WrappedItemDrop", nullptr, nullptr, nullptr);
        }

        return SI2::PlaySoundKit("(DONOTRENAME)WrappedItemPickup", nullptr, nullptr, nullptr);
    }

    const ItemGroupSoundsRec* sounds = item->m_groupSounds;

    if (sounds) {
        return SI2::PlaySoundKit(sounds->m_sound[type], nullptr, nullptr, nullptr, 0, nullptr, 1, 0);
    }

    return 0;
}

// VERIFY: fc148494 manual
// OFFSET: 0x004D0090
void SI2::PlayItemSound(ITEMSOUNDTYPE type, int32_t displayId) {
    ItemDisplayInfoRec display;

    if (!g_itemDisplayInfoDB.GetRecord(displayId, &display)) {
        return;
    }

    ItemGroupSoundsRec* sounds = g_itemGroupSoundsDB.GetRecord(display.m_groupSoundIndex);

    if (sounds) {
        SI2::PlaySoundKit(sounds->m_sound[type], nullptr, nullptr, nullptr, 0, nullptr, 1, 0);
    }
}

// VERIFY: fa14db2b manual
// OFFSET: 0x004D0110
void SI2::UpdateTaxiSounds(float elapsed) {
    // STATIC: 0x00B4AE5C
    static SE2SoundKitObject s_taxiSound;

    if (elapsed <= 0.0f) {
        return;
    }

    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !(player->m_unit->m_unitFieldFlags & 0x100000)) {
        return;
    }

    if (s_taxiFidgetDelay <= 0.0f) {
        float delayMin = CreatureSoundDataGetFidgetDelayMin(player->GetSoundData());
        float range = CreatureSoundDataGetFidgetDelayMax(player->GetSoundData());
        range = range - CreatureSoundDataGetFidgetDelayMin(player->GetSoundData());

        if (!CreatureSoundDataGetFidgetSound(player->GetSoundData(), 4)) {
            s_taxiFidgetDelay = 5.0f;
            s_taxiFidgetElapsed = 0.0f;
            return;
        }

        if (range <= 0.0f) {
            s_taxiFidgetDelay = 2.0f;
            return;
        }

        uint32_t roll = CRandom::dice_(static_cast<uint32_t>(static_cast<int64_t>(range * 100.0f)), g_rndSeed);
        s_taxiFidgetDelay = static_cast<float>(roll) * 0.0099999998f + delayMin;
    }

    if (!SI2::IsPlaying(&s_taxiSound)) {
        s_taxiFidgetElapsed = s_taxiFidgetElapsed + elapsed;
    }

    if (s_taxiFidgetDelay <= s_taxiFidgetElapsed) {
        int32_t soundKitId = CreatureSoundDataGetFidgetSound(player->GetSoundData(), 4);
        C3Vector position = player->GetPosition();
        SI2::PlaySoundKit(soundKitId, &position, &s_taxiSound, nullptr, 0, nullptr, 1, 0);
        SI2::AttachToObject(&s_taxiSound, player->m_obj->m_guid);
        s_taxiFidgetDelay = 0.0f;
        s_taxiFidgetElapsed = 0.0f;
    }
}
