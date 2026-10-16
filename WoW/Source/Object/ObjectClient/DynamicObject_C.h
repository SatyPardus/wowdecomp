#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_DYNAMICOBJECT_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_DYNAMICOBJECT_C_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "WoW/Source/Object/DynamicObject.h"
#include "WoW/Source/Object/Passenger.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"

class BlizzardObject;
class CClientObjCreate;
class SpellRec;
class SpellVisualEffectNameRec;

class CGDynamicObject_C : public CGObject_C, public CGDynamicObject {
    public:
    static void Initialize();

    /* 00D8 */ CPassenger m_passenger;
    /* 0108 */ TSList<void> m_dynamicObjectMirrorLists [6];
    /* 0150 */ uint32_t m_stateFlags;
    /* 0154 */ float m_scale;
    /* 0158 */ BlizzardObject* m_blizzardVisual;
    /* 015C */ SE2SoundKitObject m_soundKit;

    ~CGDynamicObject_C() override;
    virtual C3Vector GetPosition() const;
    virtual C3Vector GetRawPosition() const;
    virtual float GetFacing() const;
    float GetRawFacing() const override;
    float GetScale() const override;
    uint64_t GetTransportGUID() const override;
    virtual C44Matrix GetMatrix() const;
    virtual void ShouldRender(uint32_t flags, int32_t& render, int32_t& hidden);
    void SetFrameOfReference(const C44Matrix* matrix) override;
    void Disable() override;
    void Reenable() override;
    void PostReenable() override;
    void HandleOutOfRange(OUT_OF_RANGE_TYPE type) override;
    void ModelLoaded(CM2Model* model) override;
    int32_t GetModelFileName(const char** name) const override;

    CGDynamicObject_C(uint32_t time, const CClientObjCreate& create);
    void HandleAnimEvent(uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time);
    void ObjectVisKitProc();
    void PostInit(uint32_t time, const CClientObjCreate& create, bool a3);
    void SetStorage(uint32_t* storage, uint32_t* saved);
    void UpdateDisplay(uint32_t time);
    SpellVisualEffectNameRec* GetVisualEffectNameRec(const SpellRec* spell) const;
};

#endif
