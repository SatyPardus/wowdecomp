#include "UnitCombat_C.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Source/DB/DBClient/CreatureStats_C.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellVisualKitRec.h"
#include "WoW/Source/Magic/MagicClient/SpellVisuals.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/ObjectClient/Effect_C.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/UnitCombatLog_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x00CA1600
static uint32_t s_nextVictimImpactTime;

// VERIFY: 64e4513d auto
// OFFSET: 0x00754EF0
const CGUnitVirtualItem* CGUnit_C::GetParryingItem(bool offhand) const {
    const CGUnitVirtualItem* item = this->GetVirtualItem(0, false);

    if (offhand || (item && item->m_classID != 2)) {
        item = this->GetVirtualItem(1, false);

        if (item && item->m_classID != 2 && item->m_classID != 4) {
            return nullptr;
        }
    }

    return item;
}

// VERIFY: 7a0f5b4d auto
// OFFSET: 0x00754F40
int32_t CGUnit_C::GetUnitSize() const {
    int32_t size = this->m_displayInfo->m_sizeClass;

    if (size == -1) {
        return this->m_modelData->m_sizeClass;
    }

    return size;
}

// VERIFY: b8716225 manual
// OFFSET: 0x00755270
void CGUnit_C::HandleEnvironmentDamage(uint32_t type, int32_t damage, int32_t absorbed, int32_t resisted) {
    if (damage || absorbed || resisted) {
        WOWGUID guid = this->m_obj->m_guid;
        UnitCombatLog(guid, type, damage, absorbed, resisted);

        if (this->IsActivePlayer()) {
            uint32_t flags = absorbed ? 0x20 : 0;

            if (resisted) {
                flags |= 0x80;
            }

            WOWGUID target = this->m_obj->m_guid;
            CGGameUI::ShowCombatFeedback(target, damage, 1 << s_environmentalDamageSchools[type], flags);
        }
    }

    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(g_environmentalDamageVisualKits[type]);

    if (kit) {
        PlaySpellVisuaKitData data(nullptr, kit, 0);
        this->PlaySpellVisualKit(data);
    }
}

// VERIFY: 94a622f4 auto
// OFFSET: 0x00755540
const CGUnitVirtualItem* CGUnit_C::GetAttackingWeapon(int32_t hand, bool requireLoadedModel) const {
    if (!this->m_sheatheState) {
        return nullptr;
    }

    const CGUnitVirtualItem* item = this->GetVirtualItem(hand != 0, false);

    if (!item || item->m_classID != 2) {
        return nullptr;
    }

    if (!requireLoadedModel) {
        return item;
    }

    CM2Model* model = this->m_worldModel;

    if (!model || !model->IsLoaded(0, 0)) {
        return nullptr;
    }

    CM2Model* attached = model->m_attachList;

    if (!attached) {
        return nullptr;
    }

    while (attached->m_attachmentId != (hand != 0) + 1) {
        attached = attached->m_attachNext;

        if (!attached) {
            return nullptr;
        }
    }

    if (attached->IsLoaded(0, 0)) {
        return item;
    }

    return nullptr;
}

// VERIFY: 2e85141a manual
// OFFSET: 0x007559E0
void CGUnit_C::StopAttack() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ATTACK_STOP));
    msg.Finalize();
    ClientServices::Send2(&msg);
    this->m_stopAttackSent = 1;
}

// VERIFY: f28002be manual
// OFFSET: 0x00755E40
void CGUnit_C::PlayVictimRound(const ATTACKROUNDINFO& info) {
    if (this->m_unit->m_unitDynamicFlags & 0x1) {
        return;
    }

    this->PlayVictimWorldFeedback(info);

    if (info.m_hitInfo & 0x2) {
        this->PlayWoundAnimKit(info.m_hitInfo & 0x200);

        if ((info.m_damage || (info.m_hitInfo & 0x1000000)) && (info.m_victimState == 1 || info.m_victimState == 4)) {
            WOWGUID attacker = info.m_attacker;
            this->ShowBloodSpurt(attacker, info.m_hitInfo & 0x8000);
        }
    }

    if (this->DoNotPlayWoundAnim()) {
        uint32_t now = FrameTime::s_curTimeMs;

        if (now <= s_nextVictimImpactTime) {
            return;
        }

        s_nextVictimImpactTime = static_cast<uint32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * 2000) >> 32) + now + 3000;
    }

    if (info.m_victimState == 3) {
        C3Vector position = this->GetPosition();
        this->SetImpactKitEffect(false, &info, &position);
    } else if (info.m_blocked) {
        C3Vector position = this->GetPosition();
        this->SetImpactKitEffect(true, &info, &position);
    } else if (info.m_victimState == 8) {
        C3Vector position = this->GetPosition();
        position.z += 2.0f;
        SI2::PlaySoundKit("(DONOTRENAME)ShieldWoodImpact", &position, nullptr, nullptr);
    }

    if ((info.m_hitInfo & 0xA0) || info.m_victimState == 7) {
        C3Vector position = this->GetPosition();
        position.z += 2.0f;
        SI2::PlaySoundKit("(DONOTRENAME)AbsorbGetHit", &position, nullptr, nullptr);
    } else if (info.m_hitInfo & 0x2) {
        if (info.m_hitInfo & 0x20000) {
            this->PlayUnitSound(9, 0);
        } else if (info.m_hitInfo & 0x200) {
            this->PlayUnitSound(3, 0);
        } else if (!(info.m_hitInfo & 0x10)) {
            this->PlayUnitSound(2, 0);
        }
    }
}

// VERIFY: e2e6478a manual
// OFFSET: 0x00756040
void CGUnit_C::PlayRangedItemPull() {
    CM2Model* ranged = this->m_rangedWeaponModel;

    if (!ranged || !ranged->IsLoaded(0, 0) || !ranged->HasSequence(160)) {
        return;
    }

    if (!this->m_worldModel || !this->m_worldModel->IsLoaded(0, 0)) {
        return;
    }

    M2BoneSequenceInfo info;
    info.m_sequenceId = 0xFFFFFFFF;

    if (this->m_torsoKeyBone == 4) {
        this->m_worldModel->GetBoneSequenceInfo(4, info);

        if (info.m_sequenceId == 0xFFFFFFFF) {
            this->m_worldModel->GetBoneSequenceInfo(0xFFFFFFFF, info);
        }
    } else {
        this->m_worldModel->GetBoneSequenceInfo(0xFFFFFFFF, info);
    }

    if (info.m_finished) {
        return;
    }

    int32_t elapsed = static_cast<int32_t>(static_cast<int64_t>(static_cast<int32_t>(info.m_time) / info.m_speed));
    int32_t remaining = info.m_endTime - elapsed - info.m_startTime;

    if (remaining <= 0) {
        return;
    }

    M2SequenceInfo sequence;
    sequence.Clear();
    this->m_rangedWeaponModel->GetSequenceInfo(160, 0, sequence);
    this->m_rangedWeaponModel->SetBoneSequence(0xFFFFFFFF, 160, 0, 0, static_cast<float>(static_cast<int32_t>(sequence.m_duration)) / remaining, 1, 1);
}

// VERIFY: ee364054 manual
// OFFSET: 0x007561E0
void CGUnit_C::OnDeathAnimate() {
    if (!this->DoNotLogDeath()) {
        UnitCombatLogUnitDead(this);
    }

    this->PlayDeathAnimation(0);

    if (!(this->m_obj->m_type & TYPEMASK_PLAYER) || !(static_cast<CGPlayer_C*>(this)->m_player->m_playerFlags & 0x4000)) {
        this->OnAiReaction(4);
    }

    this->KillCreatureLoopSound();

    if (this->IsActivePlayer()) {
        CGGameUI::UpdateActivePlayer();
    }
}

// VERIFY: 4078ec8f manual
// OFFSET: 0x00755380
void CGUnit_C::ShowBloodSpurt(const WOWGUID& attacker, int32_t a3) {
    UnitBloodRec* blood = this->GetBloodRecord();

    if (!blood) {
        return;
    }

    int32_t attachType = 15;
    CGUnit_C* attackerUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(attacker, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (attackerUnit) {
        C3Vector position = this->GetPosition();
        C3Vector attackerPosition = attackerUnit->GetPosition();
        float dx = attackerPosition.x - position.x;
        float dy = attackerPosition.y - position.y;
        float facing = this->CGUnit_C::GetRenderFacing();
        float cosFacing = cosf(facing);
        float sinFacing = sinf(facing);

        if (dy * sinFacing + dx * cosFacing < 0.0f) {
            attachType = 16;
        }
    }

    int32_t effectId = attachType == 15 ? blood->m_combatBloodSpurtFront[a3 != 0] : blood->m_combatBloodSpurtBack[a3 != 0];
    SpellVisualEffectNameRec* effectName = g_spellVisualEffectNameDB.GetRecord(effectId);

    if (!effectName) {
        return;
    }

    CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
    WOWGUID source = this->m_obj->m_guid;
    effect->AddEffect(attachType, 0, nullptr, effectName, &attacker, &source, 0x20, CGObject_C::EffectOnceHandler, nullptr, 0, nullptr);
    effect->Release();
}

// VERIFY: 5b00631f auto
// OFFSET: 0x007555E0
void CGUnit_C::ShakeCameraOnHit() {
    if (this->m_unit->m_unitFieldPetnumber) {
        return;
    }

    int32_t shakeId = this->m_modelData->m_deathThudShakeSize;

    if (shakeId) {
        C3Vector position = this->GetPosition();
        CGWorldFrame::GetActiveCamera()->AddShakeById(shakeId, &position);
    }
}

// VERIFY: 4da467b9 manual
// OFFSET: 0x00755630
void ATTACKROUNDINFO::Unpack(CDataStore& msg) {
    msg.Get(this->m_hitInfo);
    msg >> *reinterpret_cast<SmartGUID*>(&this->m_attacker);
    msg >> *reinterpret_cast<SmartGUID*>(&this->m_target);
    msg.Get(reinterpret_cast<uint32_t&>(this->m_damage));
    msg.Get(reinterpret_cast<uint32_t&>(this->m_overkill));
    uint8_t count;
    msg.Get(count);

    for (uint8_t i = 0; i < count; i++) {
        msg.Get(this->m_schoolMask[i]);
        msg.Get(this->m_subDamage[i]);
        msg.Get(this->m_subDamageInt[i]);
    }

    if (this->m_hitInfo & 0x60) {
        for (uint8_t i = 0; i < count; i++) {
            msg.Get(this->m_absorbed[i]);
        }
    }

    if (this->m_hitInfo & 0x180) {
        for (uint8_t i = 0; i < count; i++) {
            msg.Get(this->m_resisted[i]);
        }
    }

    uint8_t victimState;
    msg.Get(victimState);
    this->m_victimState = victimState;
    msg.Get(this->unk_0058);
    msg.Get(this->m_meleeSpellId);

    if (this->m_hitInfo & 0x2000) {
        msg.Get(this->m_blocked);
    }

    if (this->m_hitInfo & 0x800000) {
        msg.Get(this->m_rageGained);
    }

    if (this->m_hitInfo & 0x1) {
        msg.Get(this->unk_0068);

        for (uint32_t i = 0; i < 8; i++) {
            msg.Get(this->unk_006C[i]);
        }

        for (uint32_t i = 0; i < 2; i++) {
            msg.Get(this->unk_0024[i]);
            msg.Get(this->unk_002C[i]);
        }

        msg.Get(this->unk_008C[0]);
    }

    if (ClntObjMgrObjectPtr(this->m_target, TYPEMASK_UNIT, __FILE__, __LINE__) && !this->m_damage && this->m_blocked) {
        this->m_victimState = 5;
    }
}

// VERIFY: 49638f19 manual
// OFFSET: 0x00755850
void CGUnit_C::PlayCombatSpellMissEffects(uint32_t reason, const WOWGUID& source) {
    if ((this->m_unit->m_unitFieldFlags2 & 0x1) || this->GetClientStandState() == 7) {
        return;
    }

    CGUnit_C* attacker = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(source, TYPEMASK_UNIT, __FILE__, __LINE__));
    const CGUnitVirtualItem* weapon = nullptr;

    if (attacker && attacker->m_sheatheState) {
        const CGUnitVirtualItem* item = attacker->GetVirtualItem(0, false);
        weapon = item && item->m_classID == 2 ? item : nullptr;
    }

    WOWGUID mover = CGUnit_C::s_activeMover;

    switch (reason) {
        case 3:
            this->PlayBaseAnimation(ANIM_DODGE, 0);
            break;

        case 4: {
            this->PlayParryAnimation();
            C3Vector position = this->GetPosition();
            SI2::PlayParrySound(weapon, this->GetParryingItem(false), 0, &position, mover == source);
            break;
        }

        case 5: {
            this->PlayBaseAnimation(ANIM_SHIELD_BLOCK, 0);
            this->GetVirtualItem(0, false);
            const CGUnitVirtualItem* shield = this->GetVirtualItem(1, false);

            if (shield && shield->m_classID != 2 && shield->m_classID != 4) {
                shield = nullptr;
            }

            C3Vector position = this->GetPosition();
            SI2::PlayParrySound(nullptr, shield, 0, &position, mover == source);
            break;
        }

        default:
            break;
    }
}

// VERIFY: 8b89f8b0 manual
// OFFSET: 0x00755A60
void CGUnit_C::PlayVictimWorldFeedback(const ATTACKROUNDINFO& info) {
    WOWGUID activePlayer = ClntObjMgrGetActivePlayer();

    if (info.m_meleeSpellId && info.m_victimState != 1) {
        return;
    }

    uint32_t victimState = info.m_victimState;

    if (victimState == 8) {
        WOWGUID source = info.m_attacker;
        this->AddWorldText(source, 9, nullptr, false);
        CGGameUI::ShowCombatFeedbackAttackRoundInfo(&info);
        return;
    }

    if (victimState == 3) {
        WOWGUID source = info.m_attacker;
        this->AddWorldText(source, 4, nullptr, false);
        CGGameUI::ShowCombatFeedbackAttackRoundInfo(&info);
        return;
    }

    if (victimState == 6) {
        WOWGUID source = info.m_attacker;
        this->AddWorldText(source, 6, nullptr, false);
        CGGameUI::ShowCombatFeedbackAttackRoundInfo(&info);
        return;
    }

    if (victimState == 2) {
        if (info.m_attacker.m_guid) {
            CGUnit_C* attacker = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(info.m_attacker, TYPEMASK_UNIT, __FILE__, __LINE__));

            if (attacker) {
                bool oneHand = !CGUnit_C::IsTwoHanded(attacker->GetVirtualItem(0, false));
                int32_t self = activePlayer == info.m_attacker || activePlayer == info.m_target;
                C3Vector position = attacker->GetPosition();
                SI2::PlayMissedSound(oneHand, &position, self);
            }
        }

        WOWGUID source = info.m_attacker;
        this->AddWorldText(source, 3, nullptr, false);
        CGGameUI::ShowCombatFeedbackAttackRoundInfo(&info);
        return;
    }

    if (victimState == 5) {
        WOWGUID source = info.m_attacker;
        this->AddWorldText(source, 5, nullptr, false);
        CGGameUI::ShowCombatFeedbackAttackRoundInfo(&info);
        return;
    }

    if (victimState == 7) {
        WOWGUID source = info.m_attacker;
        this->AddWorldText(source, 7, nullptr, false);
        CGGameUI::ShowCombatFeedbackAttackRoundInfo(&info);
        return;
    }

    if (info.m_damage || (info.m_hitInfo & 0x1000000)) {
        CGUnit_C* victim = nullptr;
        CGUnit_C* attacker = nullptr;

        if (info.m_attacker.m_guid) {
            attacker = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(info.m_attacker, TYPEMASK_UNIT, __FILE__, __LINE__));
        }

        if (info.m_target.m_guid) {
            victim = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(info.m_target, TYPEMASK_UNIT, __FILE__, __LINE__));
        }

        if (attacker && victim) {
            int32_t self = activePlayer == info.m_attacker || activePlayer == info.m_target;
            C3Vector position = victim->GetPosition();
            SI2::PlayHitSound(attacker->GetAttackingWeapon((info.m_hitInfo >> 2) & 1, true), victim->GetImpactType(), info.m_hitInfo & 0x200, &position, self);
        }

        WOWGUID source = info.m_attacker;
        this->AddWorldDamageText(source, info.m_damage, 0, (info.m_hitInfo & 0x200) != 0, false, 0);
        CGGameUI::ShowCombatFeedbackAttackRoundInfo(&info);
        return;
    }

    if (activePlayer != info.m_target && !(info.m_hitInfo & 0x4000)) {
        int32_t reason;

        if (info.m_hitInfo & 0x20) {
            reason = 10;
        } else {
            reason = (info.m_hitInfo & 0x80) ? 2 : 1;
        }

        WOWGUID source = info.m_attacker;
        this->AddWorldText(source, reason, nullptr, false);
    }

    if (info.m_attacker.m_guid) {
        CGUnit_C* attacker = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(info.m_attacker, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (attacker) {
            bool oneHand = !CGUnit_C::IsTwoHanded(attacker->GetVirtualItem(0, false));
            int32_t self = activePlayer == info.m_attacker || activePlayer == info.m_target;
            C3Vector position = attacker->GetPosition();
            SI2::PlayMissedSound(oneHand, &position, self);
            CGGameUI::ShowCombatFeedbackAttackRoundInfo(&info);
            return;
        }
    }

    CGGameUI::ShowCombatFeedbackAttackRoundInfo(&info);
}

// VERIFY: 6ec92b3f manual
// OFFSET: 0x00756180
void CGUnit_C::ClearAttackRound() {
    if (!this->m_lastAttackRound.m_attacker.m_guid || !this->m_lastAttackRound.m_target.m_guid) {
        return;
    }

    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_lastAttackRound.m_target, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (target) {
        target->PlayVictimWorldFeedback(this->m_lastAttackRound);
    }

    memset(&this->m_lastAttackRound, 0, sizeof(ATTACKROUNDINFO));
}

// VERIFY: 7ce6616c manual
// OFFSET: 0x00756240
void CGUnit_C::HandleCombatAnimEvent(uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time) {
    CGUnit_C* attacker = nullptr;
    CGUnit_C* target = nullptr;

    if (this->m_lastAttackRound.m_attacker) {
        attacker = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_lastAttackRound.m_attacker, TYPEMASK_UNIT, __FILE__, __LINE__));
    }

    if (this->m_lastAttackRound.m_target) {
        target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_lastAttackRound.m_target, TYPEMASK_UNIT, __FILE__, __LINE__));
    }

    switch (eventId) {
        case 0x30484124:
        case 0x31484124:
        case 0x32484124:
        case 0x33484124:
            if (this->m_lastAttackRound.m_damage && attacker && attacker->m_soundData) {
                SE2SoundKitProperties properties;
                properties.ResetToDefaults();
                properties.m_group = 14;

                switch (eventId) {
                    case 0x30484124:
                        SI2::PlaySoundKit(attacker->m_soundData->m_customAttack[0], position, nullptr, &properties, 0, nullptr, 1, 0);
                        break;

                    case 0x31484124:
                        SI2::PlaySoundKit(attacker->m_soundData->m_customAttack[1], position, nullptr, &properties, 0, nullptr, 1, 0);
                        break;

                    case 0x32484124:
                        SI2::PlaySoundKit(attacker->m_soundData->m_customAttack[2], position, nullptr, &properties, 0, nullptr, 1, 0);
                        break;

                    case 0x33484124:
                        SI2::PlaySoundKit(attacker->m_soundData->m_customAttack[3], position, nullptr, &properties, 0, nullptr, 1, 0);
                        break;
                }

                this->m_lastAttackRound.m_hitInfo |= 0x200000;
            }

        case 0x48414324:
            if (attacker) {
                uint32_t victimState = attacker->m_lastAttackRound.m_victimState;

                if (!victimState || victimState == 2 || victimState == 6) {
                    attacker->SetKeyBoneSequenceSpeed(attacker->m_worldModel, 0xFFFFFFFF, 0.5f, 0);
                }
            } else if (!target) {
                if (this->m_obj->m_type & TYPEMASK_PLAYER) {
                    static_cast<CGPlayer_C*>(this)->HandleSpellEventSound();
                } else {
                    CreatureSoundDataRec* soundData = this->GetSoundData();

                    if (soundData) {
                        SI2::PlaySoundKit(soundData->m_soundExertionID, position, nullptr, nullptr, 0, nullptr, 0, 0);
                    }
                }
            }

            if (target) {
                target->PlayVictimRound(this->m_lastAttackRound);
            }

            this->m_lastAttackRound.m_attacker = WOWGUID();
            this->m_lastAttackRound.m_target = WOWGUID();
            return;

        case 0x53534324: {
            if (!attacker) {
                return;
            }

            uint32_t victimState = this->m_lastAttackRound.m_victimState;

            if (!victimState || victimState == 2 || victimState == 6) {
                return;
            }

            int32_t swingType = 0;

            if (this->GetWeaponSwingType(!(this->m_lastAttackRound.m_hitInfo & 4), &swingType) && this->m_lastAttackRound.m_victimState) {
                int32_t self = CGUnit_C::s_activeMover == this->m_lastAttackRound.m_attacker || CGUnit_C::s_activeMover == this->m_lastAttackRound.m_target;
                SI2::PlayWeaponSwooshSound(swingType, this->m_lastAttackRound.m_hitInfo & 0x200, position, this->m_lastAttackRound.m_hitInfo & 0x10, self);
            }

            return;
        }

        case 0x50574224: {
            if (this->IsActivePlayer()) {
                SpellRec spell;

                if (!g_spellDB.GetRecord(this->m_castSpellId, &spell) || !(spell.m_attributes & 2)) {
                    if (!g_spellDB.GetRecord(Spell_C_GetAutoRepeatingSpell(), &spell) || !(spell.m_attributes & 2)) {
                        return;
                    }
                }
            }

            if (this->m_movementData.m_flags & 0x2E0100F) {
                return;
            }

            this->PlayRangedItemPull();
            const CGUnitVirtualItem* item = this->GetVirtualItem(2, false);

            if (!item || item->m_classID != 2 || (item->m_subclassID != 3 && item->m_subclassID != 18)) {
                if (this->m_ammoModel && !this->m_ammoModel->m_attachParent) {
                    this->m_ammoModel->AttachToParent(this->m_worldModel, 35, nullptr, 0);
                }
            }

            this->m_animationState |= 0x4000;
            return;
        }

        case 0x48544424:
            this->PlayDeathThud();
            this->ShakeCameraOnHit();
            this->ShowLootEffect();
            return;

        case 0x50504324: {
            if (!target || (target->m_unit->m_unitFieldFlags2 & 1) || target->GetClientStandState() == 7) {
                return;
            }

            uint32_t victimState = this->m_lastAttackRound.m_victimState;

            if (victimState == 3) {
                if (!target->m_creatureCacheEntry || !(target->m_creatureCacheEntry->m_typeFlags & 0x200000)) {
                    target->PlayParryAnimation();
                }

                this->m_lastAttackRound.m_hitInfo &= ~2u;
            } else if (victimState == 2 || victimState == 8) {
                if (!target->m_creatureCacheEntry || !(target->m_creatureCacheEntry->m_typeFlags & 0x200000)) {
                    target->PlayBaseAnimation(ANIM_DODGE, 0);
                }

                this->m_lastAttackRound.m_hitInfo &= ~2u;
            } else if (this->m_lastAttackRound.m_blocked) {
                if (!target->m_creatureCacheEntry || !(target->m_creatureCacheEntry->m_typeFlags & 0x200000)) {
                    target->PlayBaseAnimation(ANIM_SHIELD_BLOCK, 0);
                }

                this->m_lastAttackRound.m_hitInfo &= ~2u;
            }

            return;
        }

        default:
            return;
    }
}

// VERIFY: a8a61e30 manual
// OFFSET: 0x00756770
void CGUnit_C::OnAttackStop(uint64_t target, int32_t faceTarget) {
    this->m_combat.m_attacking = WOWGUID{ 0 };
    this->m_combat.m_attackSent = 0;
    this->m_stopAttackSent = 0;
    this->ClearAttackRound();

    if (!target || !faceTarget) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ target }, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit) {
        this->m_clientFlags |= 0x1;
        C3Vector targetPosition = unit->GetPosition();
        C3Vector position = this->GetPosition();
        this->m_scriptedFacing = CalculateFacingTo(&position, &targetPosition);
    }
}

// VERIFY: 161e4292 manual
// OFFSET: 0x00756800
int32_t Packet_Group_29(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    switch (msgId) {
        case SMSG_ATTACK_START: {
            WOWGUID attacker;
            WOWGUID victim;
            msg->Get(attacker.m_guid);
            msg->Get(victim.m_guid);
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(attacker, TYPEMASK_UNIT, __FILE__, __LINE__));

            if (!unit) {
                return 0;
            }

            unit->m_combat.m_attackSent = 0;
            unit->SetAttacking(victim);

            if (!unit->IsActivePlayer()) {
                return 1;
            }

            FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_ENTER_COMBAT, nullptr);
            CGGameUI::UpdateCombatMode();
            return 1;
        }
        case SMSG_ATTACK_STOP:
        case SMSG_COMBAT_EVENT_FAILED: {
            SmartGUID attacker;
            SmartGUID victim;
            *msg >> attacker;
            *msg >> victim;
            uint32_t faceTarget;
            msg->Get(faceTarget);
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(attacker, TYPEMASK_UNIT, __FILE__, __LINE__));

            if (unit) {
                unit->OnAttackStop(victim.m_guid, faceTarget);

                if (unit->IsActivePlayer()) {
                    FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_LEAVE_COMBAT, nullptr);
                    CGGameUI::UpdateCombatMode();
                }
            }

            return 1;
        }
        case SMSG_ATTACKSWING_NOTINRANGE:
            SetAttackSwingError(1);
            return 0;
        case SMSG_ATTACKSWING_BADFACING:
            SetAttackSwingError(2);
            return 0;
        case SMSG_ATTACKSWING_DEADTARGET:
        case SMSG_ATTACKSWING_CANT_ATTACK: {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (!player) {
                return 0;
            }

            player->SetCombatMode();
            return 0;
        }
        case SMSG_ATTACKER_STATE_UPDATE: {
            ATTACKROUNDINFO info;
            info.Unpack(*msg);
            CGUnit_C* attacker = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(info.m_attacker, TYPEMASK_UNIT, __FILE__, __LINE__));

            if (attacker) {
                if (attacker->m_sheatheState != 1) {
                    attacker->SetSheatheState(1, 1, 0);
                }

                attacker->m_combat.m_attackSent = 0;
                attacker->ClearAttackRound();
                attacker->m_lastAttackRound = info;
                CGUnit_C* victim = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(info.m_target, TYPEMASK_UNIT, __FILE__, __LINE__));

                if (attacker->IsActivePlayer() && victim) {
                    static_cast<CGPlayer_C*>(attacker)->UpdateAttackPosition(victim);
                }

                attacker->PlayAttackerRound(info);

                if (attacker->m_lastAttackRound.m_target == ClntObjMgrGetActivePlayer()) {
                    WOWGUID guid = attacker->m_obj->m_guid;
                    TargetAttacker(guid);
                }

                if (attacker->IsAutoTracking() && !attacker->IsActiveMoverTracking(4)) {
                    attacker->ClearTrackingTarget(true, true);
                }
            } else {
                CGUnit_C* victim = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(info.m_target, TYPEMASK_UNIT, __FILE__, __LINE__));

                if (victim) {
                    victim->PlayVictimRound(info);
                }
            }

            UnitCombatLog(info);
            return 0;
        }
        case SMSG_ENVIRONMENTAL_DAMAGE_LOG: {
            uint64_t guid = 0;
            msg->Get(guid);
            uint8_t type = 0;
            msg->Get(type);
            uint32_t damage = 0;
            msg->Get(damage);
            uint32_t absorbed = 0;
            msg->Get(absorbed);
            uint32_t resisted = 0;
            msg->Get(resisted);
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ guid }, TYPEMASK_UNIT, __FILE__, __LINE__));

            if (unit) {
                unit->HandleEnvironmentDamage(type, damage, absorbed, resisted);
            }

            return 1;
        }
        case SMSG_CLEAR_TARGET: {
            uint64_t guid;
            msg->Get(guid);
            CGGameUI::ClearTarget(WOWGUID{ guid }, 1);
            return 0;
        }
        default:
            return 0;
    }
}
