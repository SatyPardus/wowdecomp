#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_OBJECT_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_OBJECT_C_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/Object.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/Ui/DBCACHEHASH_NAME.h"

class CObjectEffectManager;
class CTexture;
struct CVar;
class C3Vector;
class C44Matrix;
class C4Quaternion;
class CClientObjCreate;
class CEffect;
class CGBag_C;
class CGObject_C;
class CGWorldFrame;
class CHashKeyGUID;
class CImVector;
class CM2Model;
class CMapEntity;
class PlaySpellVisuaKitData;
class CPassenger;
class PLAYERNAMEDESC;
class SpellRec;
class SpellVisualKitRec;
class SpellVisualEffectNameRec;
class SpellVisualKitModelAttachRec;
class SpellVisualRec;
enum OUT_OF_RANGE_TYPE : int32_t;
enum SPELL_VISUAL_KIT_TYPE : int32_t;

class CHashKeyGUID {
    public:
    /* 0000 */ WOWGUID m_guid;

    CHashKeyGUID();
    CHashKeyGUID(uint64_t guid);
    bool operator==(const CHashKeyGUID& key);
};

// INLINED
inline CHashKeyGUID::CHashKeyGUID() {
    this->m_guid.m_guid = 0;
}

// INLINED
inline CHashKeyGUID::CHashKeyGUID(uint64_t guid) {
    this->m_guid.m_guid = guid;
}

// INLINED
inline bool CHashKeyGUID::operator==(const CHashKeyGUID& key) {
    return this->m_guid == key.m_guid;
}

class CGObject_C : public CGObject, public TSHashObject<CGObject_C, CHashKeyGUID> {
    public:
    struct InteractIcon {
        /* 0000 */ CM2Model* m_model;
        /* 0004 */ float m_scale;
        /* 0008 */ const char* m_fileName;
    };

    static InteractIcon s_interactIcons[12];
    static const uint32_t s_questStatusIcons[11];
    static CTexture* s_selectionTexture;
    static CVar* s_cvObjectSelectionCircle;

    static void Initialize();
    static void ModelLoadedCallback(CM2Model* model, void* arg);
    static void Shutdown();
    static int32_t UpdateAllObjectEffectManagers();
    static CGObject_C* GetFocusObject();
    static void PrepareToFadeInt(CMapEntity* entity, float alpha, uint64_t transportGuid);
    static int32_t UpdateAllWorldObjects();
    static void LoopEffectHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param);
    static void EffectOnceHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param);
    static void EffectDecayHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param);
    static void AuraDecayHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param);
    static void AuraEffectHoldHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param);
    static void AuraEffectHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param);

    /* 0038 */ TSLink<void> m_link;
    /* 0040 */ uint32_t m_disableTime;
    /* 0044 */ TSList<void> m_objMirrorLists [6];
    /* 008C */ CM2Model* m_interactIcon;
    /* 0090 */ uint32_t m_questStatus;
    /* 0094 */ uint32_t m_interactIconType;
    /* 0098 */ float m_scale;
    /* 009C */ float m_scaleModifier;
    /* 00A0 */ uint32_t m_scaleChangeTime;
    /* 00A4 */ float m_scaleChangeStart;
    /* 00A8 */ CEffect* m_effectList;
    /* 00AC */ float m_height;
    /* 00B0 */ PLAYERNAMEDESC* m_nameDesc;
    /* 00B4 */ CM2Model* m_worldModel;
    /* 00B8 */ CMapEntity* m_worldObject;
    /* 00BC */ uint32_t m_modelFlags;
    /* 00C0 */ uint32_t m_fadeStartTime;
    /* 00C4 */ uint32_t m_fadeDuration;
    /* 00C8 */ uint8_t m_alpha;
    /* 00C9 */ uint8_t m_fadeStartAlpha;
    /* 00CA */ uint8_t m_fadeEndAlpha;
    /* 00CB */ uint8_t m_alphaScale;
    /* 00CC */ CObjectEffectManager* m_effectManager;

    virtual ~CGObject_C();
    virtual void Disable();
    virtual void Reenable();
    virtual void PostReenable();
    virtual void HandleOutOfRange(OUT_OF_RANGE_TYPE type);
    virtual void UpdateWorldObject(int32_t a2);
    virtual int32_t ShouldFadeout();
    virtual void UpdateDisplayInfo(int32_t force);
    virtual C3Vector GetNamePosition() const;
    virtual CGBag_C* GetBag();
    virtual const CGBag_C* GetBag() const;
    virtual C3Vector GetPosition() const;
    virtual C3Vector GetRawPosition() const;
    virtual float GetFacing() const;
    virtual float GetRawFacing() const;
    virtual float GetScale() const;
    virtual uint64_t GetTransportGUID() const;
    virtual C4Quaternion GetRotation() const;
    virtual void SetFrameOfReference(const C44Matrix* matrix);
    virtual int32_t IsQuestGiver() const;
    virtual void RefreshInteractIcon();
    virtual void UpdateInteractIcon();
    virtual void UpdateInteractIconAttach();
    virtual void UpdateInteractIconScale();
    virtual int32_t GetModelFileName(const char** name) const;
    virtual void ScaleChangeUpdate();
    virtual void ScaleChangeFinished();
    virtual void RenderTargetSelection() const;
    virtual void RenderPetTargetSelection() const;
    virtual void Render();
    virtual int32_t GetSelectionHighlightColor(CImVector* color) const;
    virtual float GetTrueScale() const;
    virtual void ModelLoaded(CM2Model* model);
    virtual void ApplyAlpha(uint32_t time);
    virtual void PreAnimate(CGWorldFrame* frame);
    virtual int32_t Animate(float elapsed);
    virtual void ShouldRender(uint32_t flags, int32_t& render, int32_t& hidden);
    virtual float GetRenderFacing() const;
    virtual void OnSpecialMountAnim();
    virtual int32_t IsSolidSelectable() const;
    virtual int32_t IsCollidable(int32_t flags) const;
    virtual int32_t CanHighlight() const;
    virtual int32_t CanBeTargetted() const;
    virtual int32_t FloatingTooltip() const;
    virtual void OnRightClick();
    virtual int32_t IsHighlightSuppressed() const;
    virtual void OnSpellEffectClear(CEffect* effect);
    virtual const SpellVisualRec* GetAppropriateSpellVisual(const SpellRec* spell, SpellVisualRec& visual, int32_t visualId, int32_t lowViolenceVisualId) const;
    virtual void ConnectToLightningThisFrame(uint32_t spellId, int32_t attachment);
    virtual C44Matrix GetMatrix() const;
    virtual void ObjectNameVisibilityChanged();
    virtual int32_t UpdateObjectNameString(int32_t flags, char* buffer, uint32_t size);
    virtual int32_t ShouldRenderObjectName(uint32_t flags) const;
    virtual CM2Model* GetObjectModel() const;
    virtual const char* GetObjectName() const;
    virtual int32_t GetPageTextID(void (*callback)(int32_t, const WOWGUID&, void*, bool)) const;
    virtual void CleanUpVehicleBoneAnimsBeforeObjectModelChange();
    virtual int32_t ShouldFadeIn() const;
    virtual float GetBaseAlpha() const;
    virtual bool IsTransport() const;
    virtual int32_t IsPointInside(const C3Vector& point) const;
    virtual int32_t AddPassenger(CPassenger* passenger, int32_t a2);
    virtual float GetSpeed() const;
    virtual void PlaySpellVisualKit_PlayAnims(const SpellRec* spell, const SpellVisualKitRec* kit, SPELL_VISUAL_KIT_TYPE type, const C3Vector* position, int32_t a5, int32_t a6, WOWGUID guid, uint32_t a8, const SpellVisualRec* visual, uint32_t a10, int32_t a11);
    virtual void PlaySpellVisualKit_HandleWeapons(const SpellRec* spell, const SpellVisualKitRec* kit, const C3Vector* position, WOWGUID guid, uint32_t a5, uint32_t flags, void (*callback)(CM2Model*, uint32_t, uint32_t, int32_t, uint32_t, uint64_t), int32_t* hasEffect);
    virtual int32_t PlaySpellVisualKit_DelayLightningEffects(const SpellRec* spell, const SpellVisualKitRec* kit, SPELL_VISUAL_KIT_TYPE type, const C3Vector* position, WOWGUID guid, int32_t a6, int32_t a7, int32_t a8, int32_t a9, uint32_t a10, const SpellVisualRec* visual);

    CGObject_C();
    CGObject_C(uint32_t time, const CClientObjCreate& create);
    void ClearEffectList(int32_t all);
    void ClearSpellEffect(uint32_t spellId, int32_t all);
    void DoFade(float alpha, uint32_t time);
    void PlayEffectName(int32_t attachType, uint32_t a3, int32_t spellId, const SpellVisualKitRec* kit, const SpellVisualEffectNameRec* effectName, uint32_t* flags, void (*callback)(CM2Model*, uint32_t, uint32_t, int32_t, uint32_t, uint64_t), const C3Vector* position, WOWGUID target, uint32_t a11, const SpellVisualKitModelAttachRec* attach);
    C44Matrix GetModelMatrix();
    C3Vector GetModelPosition();
    int32_t GetQuestGiverCursor(int32_t highlighted);
    void HandleScaleUpdate(float scale);
    void HideHighlightType(int32_t type);
    void AddWorldObject();
    int32_t IsObjectLocked();
    int32_t IsObjectModelLoaded();
    int32_t IsReadyToDraw();
    void ModelChanged();
    void PostInit(uint32_t time, const CClientObjCreate& create, bool a3);
    void ReleaseModel();
    void RemoveInteractIcon();
    void SetBlock(int32_t index, uint32_t value);
    void SetBoundsDirty();
    void SetDisablePending(int32_t pending);
    void SetInteractIconVisible(int32_t visible);
    CM2Model* SetModelStart(CM2Model* model);
    void SetObjectLocked(int32_t locked);
    void SetQuestStatus(uint32_t status);
    void SetStorage(uint32_t* storage, uint32_t* saved);
    void SetTypeID(OBJECT_TYPE_ID typeId);
    void ShowHighlightType(int32_t type);
    void UpdateEffectAttachments();
    void Fadeout();
    static void __stdcall BuildSelectionRotMatrix(C44Matrix* matrix, const C3Vector* position);
    static int32_t SetCircleRenderStates();
    void SetModelFinish(CM2Model* model);
    void SetModel(CM2Model* model);
    void PlaySpellVisualKit(const PlaySpellVisuaKitData& data);
    void ClearEffectKit(int32_t spellId, int32_t kitType, int32_t a4, int32_t a5, int32_t a6);
};

struct DBCACHEHASH_NAME : public TSHashObject<DBCACHEHASH_NAME, CHashKeyGUID> {
    /* 0020 */ uint8_t m_record [336];
    /* 0170 */ DBCACHETRAILER_GUID m_trailer;
};

namespace World {
    int32_t QueryObjectVisible(CMapEntity* entity);
    void TickObject(CMapEntity* entity);
}

#endif
