#include "CGGameObject_C_Type_QuestGiver.h"
#include "WoW/Source/Object/GameObjectDef.h"
#include "WoW/Source/Object/GameObjectStats.h"
#include "Engine/Source/Model2/M2Model.h"

// VERIFY: 369810f6 auto
// OFFSET: 0x007125C0
CGGameObject_C_Type_QuestGiver::CGGameObject_C_Type_QuestGiver(CGGameObject_C* owner) : CGGameObject_C_TypeAnimated(owner) {
    this->m_interactDistance = MAX_SHOP_DISTANCE;
}

// VERIFY: 8178ce5d auto
// OFFSET: 0x0070FDE0
int32_t CGGameObject_C_Type_QuestGiver::CanChangeCursor() const {
    int32_t cursor = this->m_owner->GetQuestGiverCursor(!this->CanUseNow(nullptr, nullptr, nullptr));

    if (!cursor) {
        return CGGameObject_C_TypeBase::CanChangeCursor();
    }

    return cursor;
}

// VERIFY: 7cce95da auto
// OFFSET: 0x0070FF20
void CGGameObject_C_Type_QuestGiver::CloseInteraction() {
    uint32_t keepOpenIndex = CGameObjectDef::GetPropNum(this->m_owner->m_gameObject->m_typeID, 22);
    GameObjectStats* keepOpenStats = this->m_owner->m_stats;
    uint32_t keepOpen = (keepOpenStats && keepOpenIndex < 24) ? keepOpenStats->m_propValue[keepOpenIndex] : 0;

    if (keepOpen) {
        return;
    }

    if (this->m_animState == 2 || this->m_animState == 3) {
        this->UpdateAnimState(4);
    }
}

// VERIFY: 86ad0112 manual
// OFFSET: 0x0070FE10
void CGGameObject_C_Type_QuestGiver::StartInteraction() {
    uint32_t index = CGameObjectDef::GetPropNum(this->m_owner->m_gameObject->m_typeID, 22);
    GameObjectStats* stats = this->m_owner->m_stats;

    if (stats && index < 24 && stats->m_propValue[index]) {
        uint32_t animIndex = CGameObjectDef::GetPropNum(this->m_owner->m_gameObject->m_typeID, 22);
        GameObjectStats* animStats = this->m_owner->m_stats;
        this->m_owner->PlayCustomAnim(animStats && animIndex < 24 ? animStats->m_propValue[animIndex] - 1 : 0xFFFFFFFF);
        return;
    }

    CM2Model* model = this->m_owner->m_worldModel;

    if (!model || !model->IsLoaded(0, 0)) {
        return;
    }

    uint32_t anim = CGGameObject_C_TypeAnimated::s_animSequenceIds[2];

    if (!model->HasSequence(anim)) {
        M2SequenceInfo info;
        info.Clear();
        model->GetSequenceInfo(anim, 0, info);
        anim = info.m_animId;
    }

    if (model->GetBoneSequenceId(0xFFFFFFFF) != anim) {
        this->UpdateAnimState(2);
    }
}
