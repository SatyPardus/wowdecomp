#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_GAMEOBJECT_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_GAMEOBJECT_C_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "WoW/Source/Game/ShipPath.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/GameObject.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/Object/ObjectClient/Types.h"
#include "WoW/Source/Object/Passenger.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/WorldClient/Map.h"

static const float MAX_SHOP_DISTANCE = 5.5555553f;
static const float MAX_BIND_DISTANCE = 10.0f;
static const float MAX_LOOT_DISTANCE = 5.0f;
static const float MAX_OBJ_INTEREST_RADIUS = 100.0f;
static const float MAX_SITCHAIRUSE_DISTANCE = 3.0f;
static const float MAX_SITCHAIR_DISTANCE_SQUARED = 0.16666667f;

class SpellRec;
class CMapBaseObj;
class CDataStore;
class CGGameObject_C;
class CGTooltip;
class CM2Model;
class CMapEntity;
class DestructibleModelDataRec;
class GameObjectStats;
class GameObjectStats_C;
class LockRec;
enum GAME_ERROR_TYPE : int32_t;

class CGGameObject_C_TypeBase {
    public:
    /* 0004 */ CGGameObject_C* m_owner;
    /* 0008 */ uint32_t m_useErrorType;
    /* 000C */ float m_interactDistance;

    virtual ~CGGameObject_C_TypeBase();
    virtual bool CanBeSolid() const;
    virtual bool ShouldFadeIn() const;
    virtual bool CanHighlight() const;
    virtual int32_t IsHighlightSuppressed() const;
    virtual int32_t CanChangeCursor() const;
    virtual bool CanUse() const;
    virtual bool CanUseNow(GAME_ERROR_TYPE* error, float* distance, const char** errorText) const;
    virtual int32_t CheckRange() const;
    virtual bool Use();
    virtual void OnUpdateFlags(uint32_t changed);
    virtual void OnUpdateAnimProgress();
    virtual void UpdateState(int32_t oldState, int32_t newState);
    virtual int32_t Animate(float elapsed);
    virtual void ShouldRender(uint32_t flags, int32_t& render, int32_t& hidden);
    virtual float GetBaseAlpha() const;
    virtual void HandleAnimEvent(uint32_t eventId, uint32_t data, const C3Vector& position, uint32_t time);
    virtual int32_t HandleAnimInterrupted();
    virtual void HandleAnimFinished();
    virtual void ActivateOneShotAnim(uint32_t anim);
    virtual const char* DebugStatus();
    virtual C3Vector GetPosition() const;
    virtual C3Vector GetRawPosition() const;
    virtual float GetFacing() const;
    virtual float GetRawFacing() const;
    virtual float GetSpeed() const;
    virtual void AddPassenger(CPassenger* passenger, int32_t a2);
    virtual int32_t IsPointInside(const C3Vector& point) const;
    virtual CMapBaseObj* GetMapObjectTransportID() const;
    virtual void PostInit(bool a1);
    virtual void PostInitWithStats();
    virtual void Reenable();
    virtual void Disable();
    virtual void PostReenable();
    virtual ShipPath* GetShipPath() const;
    virtual void UpdateMovement(uint32_t time, uint32_t elapsed);
    virtual void UpdateDisplay(uint32_t time);
    virtual void UpdateDisabled();
    virtual void StartInteraction();
    virtual void CloseInteraction();
    virtual void AddToolTipInfo(CGTooltip* tooltip);
    virtual uint32_t GetSequenceTimeFromRealTime(uint32_t time) const;
    virtual uint32_t GetTransportUpdateTime() const;
    virtual float GetInteractDistance() const;

    CGGameObject_C_TypeBase(CGGameObject_C* owner);
};

enum : int32_t {
    ANIMSTATE_SPAWN = 0,
    ANIMSTATE_CLOSED = 1,
    ANIMSTATE_OPENING = 2,
    ANIMSTATE_OPEN = 3,
    ANIMSTATE_CLOSING = 4,
    ANIMSTATE_DESTROYING = 5,
    ANIMSTATE_DESTROYED = 6,
    ANIMSTATE_REBUILDING = 7,
    ANIMSTATE_FIRSTCUSTOM = 8,
    ANIMSTATE_CUSTOM0 = 8,
    ANIMSTATE_CUSTOM1 = 9,
    ANIMSTATE_CUSTOM2 = 10,
    ANIMSTATE_CUSTOM3 = 11,
    ANIMSTATE_DESPAWN = 12,
    NUM_GAMEOBJ_ANIMSTATES = 13,
};

class CGGameObject_C_TypeAnimated : public CGGameObject_C_TypeBase {
    public:
    static uint32_t s_animSequenceIds[NUM_GAMEOBJ_ANIMSTATES];
    static const char* s_animStateNames[NUM_GAMEOBJ_ANIMSTATES];

    /* 0010 */ int32_t m_animState;
    /* 0014 */ int32_t m_pendingAnimState;
    /* 0018 */ int32_t m_nextAnimTime;
    /* 001C */ SE2SoundKitObject m_sound;

    virtual ~CGGameObject_C_TypeAnimated();
    virtual bool ShouldFadeIn() const;
    virtual void OnUpdateFlags(uint32_t changed);
    virtual void OnUpdateAnimProgress();
    virtual void UpdateState(int32_t oldState, int32_t newState);
    virtual int32_t Animate(float elapsed);
    virtual void HandleAnimEvent(uint32_t eventId, uint32_t data, const C3Vector& position, uint32_t time);
    virtual int32_t HandleAnimInterrupted();
    virtual void HandleAnimFinished();
    virtual void ActivateOneShotAnim(uint32_t anim);
    virtual const char* DebugStatus();
    virtual void PostInit(bool a1);
    virtual void Disable();
    virtual void PostReenable();
    virtual void UpdateDisplay(uint32_t time);
    virtual void UpdateDisabled();
    virtual void UpdateAnimState(uint32_t state);

    CGGameObject_C_TypeAnimated(CGGameObject_C* owner);
    void SetSequence(CM2Model* model);
    void ResetAnimState();
};

class CGGameObject_C_Type_Null : public CGGameObject_C_TypeBase {
    public:
    virtual bool CanUse() const;
    virtual const char* DebugStatus();
    virtual bool CanUseNow(GAME_ERROR_TYPE* error, float* distance) const;

    CGGameObject_C_Type_Null(CGGameObject_C* owner);
};

class CGGameObject_C_Type_WorldObj : public CGGameObject_C_TypeBase {
    public:
    static void SequenceFinishedHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param);

    /* 0010 */ CMapEntity* m_worldObject;

    virtual ~CGGameObject_C_Type_WorldObj();
    virtual bool CanBeSolid() const;
    virtual bool CanHighlight() const;
    virtual bool CanUse() const;
    virtual CMapBaseObj* GetMapObjectTransportID() const;
    virtual void PostInitWithStats();
    virtual void Disable();
    virtual void PostReenable();
    virtual void CreateWorldObj();
    virtual void DestroyWorldObj();

    CGGameObject_C_Type_WorldObj(CGGameObject_C* owner);
};

class CGGameObject_C_Type_TransportBase : public CGGameObject_C_Type_WorldObj {
    public:
    /* 0014 */ TSExplicitList<CPassenger, offsetof(CPassenger, m_link)> m_passengers;
    /* 0020 */ C3Vector m_position;
    /* 002C */ float m_speed;
    /* 0030 */ uint32_t m_pathTime;
    /* 0034 */ uint32_t m_worldObjectReady;
    /* 0038 */ int32_t m_currentState;
    /* 003C */ uint32_t m_transportUpdateTime;

    virtual ~CGGameObject_C_Type_TransportBase();
    virtual C3Vector GetPosition() const;
    virtual C3Vector GetRawPosition() const;
    virtual float GetSpeed() const;
    virtual void AddPassenger(CPassenger* passenger, int32_t a2);
    virtual int32_t IsPointInside(const C3Vector& point) const;
    virtual void PostInitWithStats();
    virtual void Reenable();
    virtual void Disable();
    virtual void PostReenable();
    virtual uint32_t GetTransportUpdateTime() const;

    CGGameObject_C_Type_TransportBase(CGGameObject_C* owner);
    void MovePassengers(int32_t* moved);
    float UpdateCurrentVelocity(uint32_t elapsed, const C3Vector& position, C3Vector* direction);
};

class CGGameObject_C_Type_MapObjTransport : public CGGameObject_C_Type_TransportBase {
    public:
    /* 0040 */ ShipPath m_shipPath;
    /* 0080 */ uint32_t m_currentLeg;

    virtual void UpdateState(int32_t oldState, int32_t newState);
    virtual void AddPassenger(CPassenger* passenger, int32_t a2);
    virtual void PostInitWithStats();
    virtual void PostReenable();
    virtual ShipPath* GetShipPath() const;
    virtual void UpdateMovement(uint32_t time, uint32_t elapsed);
    virtual uint32_t GetSequenceTimeFromRealTime(uint32_t time) const;

    CGGameObject_C_Type_MapObjTransport(CGGameObject_C* owner);
    void InitializeMovement();
    void UpdateObjectEffectState(uint32_t anim);
};

struct DestructibleBuildingState {
    /* 0000 */ const char* m_modelName;
    /* 0004 */ int32_t m_displayId;
    /* 0008 */ CMapEntity* m_object;
    /* 000C */ uint8_t m_loading;
    /* 000D */ uint8_t m_animate;
    /* 000E */ uint8_t unused_000E[2];
};

class CGGameObject_C_Type_DestructibleBuilding : public CGGameObject_C_Type_WorldObj {
    public:
    static void AmbientSequenceFinishedHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param);
    static void AnimEventCallback(CM2Model* model, uint32_t boneId, uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time, uint64_t param);
    static void DestructSequenceFinishedHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param);
    static void ImpactEffectSequenceFinishedHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param);
    static void MovedDoodadSequenceHandler(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param);

    /* 0014 */ DestructibleBuildingState m_states [4];
    /* 0054 */ int32_t m_previousState;
    /* 0058 */ float m_baseZ;
    /* 005C */ uint32_t m_animStartTime;
    /* 0060 */ uint32_t m_animDuration;
    /* 0064 */ CMapEntity* m_rebuildEffect;
    /* 0068 */ CMapEntity* m_baseCollision;
    /* 006C */ float m_sinkDepth;
    /* 0070 */ float m_previousSinkDepth;
    /* 0074 */ SE2SoundKitObject m_sound;
    /* 0088 */ CDestructibleProxy m_proxy;
    /* 00A8 */ DestructibleModelDataRec* m_modelData;
    /* 00AC */ uint8_t m_loadPending;
    /* 00AD */ uint8_t m_animating;

    virtual ~CGGameObject_C_Type_DestructibleBuilding();
    virtual bool CanHighlight() const;
    virtual int32_t IsHighlightSuppressed() const;
    virtual void OnUpdateFlags(uint32_t changed);
    virtual void HandleAnimEvent(uint32_t eventId, uint32_t data, const C3Vector& position, uint32_t time);
    virtual void PostInit(bool a1);
    virtual void PostInitWithStats();
    virtual void Disable();
    virtual void PostReenable();
    virtual void UpdateDisplay(uint32_t time);
    virtual void CreateWorldObj();
    virtual void DestroyWorldObj();
    C3Vector GetNamePosition() const;

    CGGameObject_C_Type_DestructibleBuilding(CGGameObject_C* owner);
    uint16_t GetImpactEffectDoodadSet(int32_t state) const;
    uint16_t GetDestructionDoodadSet(int32_t state) const;
    uint16_t GetAmbientDoodadSet(int32_t state) const;
    void UpdateDestructibleState();
};

class CGGameObject_C_Type_FishingNode : public CGGameObject_C_TypeAnimated {
    public:
    virtual bool CanUse() const;

    CGGameObject_C_Type_FishingNode(CGGameObject_C* owner);
};

class CGGameObject_C_Type_GuildBank : public CGGameObject_C_TypeAnimated {
    public:
    virtual int32_t CanChangeCursor() const;
    virtual bool Use();
    virtual void StartInteraction();

    CGGameObject_C_Type_GuildBank(CGGameObject_C* owner);
};

class CGGameObject_C_Type_MeetingStone : public CGGameObject_C_TypeAnimated {
    public:
    virtual bool Use();
    virtual void AddToolTipInfo(CGTooltip* tooltip);

    CGGameObject_C_Type_MeetingStone(CGGameObject_C* owner);
    int32_t CheckUseRange(uint32_t level) const;
};

class CGGameObject_C_Type_Ritual : public CGGameObject_C_TypeAnimated {
    public:
    virtual bool CanUseNow(GAME_ERROR_TYPE* error, float* distance, const char** errorText) const;

    CGGameObject_C_Type_Ritual(CGGameObject_C* owner);
};

class CGGameObject_C : public CGObject_C, public CGGameObject {
    public:
    static void Initialize();
    static void Shutdown();
    bool IsTracked() const;
    static void AnimEventCallback(CM2Model* model, uint32_t boneId, uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time, uint64_t param);
    static int32_t OnAnimProgressChanged(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param);
    static int32_t OnFlagsChanged(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param);
    static int32_t OnStateChanged(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param);
    static void SequenceCallback(CM2Model* model, uint32_t boneId, uint32_t sequenceId, int32_t interrupted, uint32_t time, uint64_t param);

    /* 00D8 */ CPassenger m_passenger;
    /* 0108 */ TSList<void> m_gameObjectMirrorLists [12];
    /* 0198 */ TSLink<CGGameObject_C> m_transportLink;
    /* 01A0 */ CGGameObject_C_TypeBase* m_typeData;
    /* 01A4 */ GameObjectStats* m_stats;
    /* 01A8 */ C44Matrix m_matrix;
    /* 01E8 */ CAaBox m_worldBounds;
    /* 0200 */ uint32_t m_transportTimeOffset;
    /* 0204 */ int32_t m_displayState;
    /* 0208 */ int32_t m_pendingOneShotAnim;
    /* 020C */ int32_t m_hasWorldBounds;
    /* 0210 */ CM2Model* m_lootEffect;
    /* 0214 */ uint32_t m_effectAnimState;

    virtual C3Vector GetPosition() const;
    virtual C3Vector GetRawPosition() const;
    void UpdateWorldObject(int32_t a2) override;
    float GetFacing() const override;
    float GetRawFacing() const override;
    C4Quaternion GetRotation() const override;
    void ModelLoaded(CM2Model* model) override;
    virtual uint64_t GetTransportGUID() const;
    virtual int32_t IsQuestGiver() const;
    virtual void UpdateInteractIconAttach();
    virtual int32_t Animate(float elapsed);
    virtual int32_t IsSolidSelectable() const;
    virtual int32_t IsCollidable(int32_t flags) const;
    virtual int32_t IsHighlightSuppressed() const;
    virtual int32_t GetModelFileName(const char** name) const;
    virtual void ShouldRender(uint32_t flags, int32_t& render, int32_t& hidden);
    virtual C44Matrix GetMatrix() const;
    virtual const char* GetObjectName() const;
    virtual int32_t ShouldFadeIn() const;
    virtual float GetBaseAlpha() const;
    virtual bool IsTransport() const;
    virtual int32_t IsPointInside(const C3Vector& point) const;
    virtual int32_t AddPassenger(CPassenger* passenger, int32_t a2);
    virtual float GetSpeed() const;
    virtual ShipPath* GetShipPath() const;
    C3Vector GetNamePosition() const override;
    void RefreshInteractIcon() override;
    void UpdateInteractIcon() override;
    int32_t CanHighlight() const override;
    int32_t FloatingTooltip() const override;
    void OnRightClick() override;
    int32_t GetPageTextID(void (*callback)(int32_t, const WOWGUID&, void*, bool)) const override;
    void PostReenable() override;

    CGGameObject_C(uint32_t time, const CClientObjCreate& create);
    ~CGGameObject_C() override;
    void Disable() override;
    void Reenable() override;
    void AddToolTipInfo(CGTooltip* tooltip);
    bool CanUse() const;
    bool IsCreatorOnlyUse() const;
    void CheckForPassiveHighlight();
    int32_t CanUseNow() const;
    int32_t CheckRange() const;
    void CloseInteraction();
    void UseObject();
    CMapEntity* CreateWorldObject(uint64_t guid, int32_t a4, uint32_t doodadSetCount, const int16_t* doodadSets);
    void AddWorldDamageText(int32_t amount);
    void AddWorldHealingText(int32_t amount, const SpellRec* spell);
    const char* GetCloseText() const;
    void GetCursorFile(char* buffer, uint32_t size) const;
    float GetInteractDistanceSquared() const;
    const char* GetModelFileNameInternal() const;
    const char* GetOpenText() const;
    uint32_t GetPropertyValue(uint32_t index) const;
    int32_t GetPageTextMaterial() const;
    int32_t GetPageTextLanguage() const;
    bool IsValidOpenAction(int32_t action) const;
    void LoadBaseObject(GameObjectStats_C* stats);
    void PlayCustomAnim(uint32_t anim);
    void PostInit(uint32_t time, const CClientObjCreate& create, bool reenable);
    void ResetDisplayState();
    void SetDisplayState(int32_t state);
    UNIT_REACTION ObjectReaction(const CGUnit_C* unit) const;
    LockRec* GetLockRec() const;
    bool IsValidTargetForSpell(const WOWGUID& item, int32_t spellId) const;
    int32_t CanChangeCursor() const;
    void SetPathProgress(float progress);
    void SetStorage(uint32_t* storage, uint32_t* saved);
    void ShowLootEffect();
    void StartInteraction();
    void UpdateObjectEffectAnimationStates(uint32_t anim);
    CAaBox GetWorldBounds() const;
    void UpdateDisplay(uint32_t time);
    uint32_t GetProperty(int32_t prop) const;
    bool IsLocked(int32_t* spellId, int32_t* skillValue, int32_t* requiredSkill, CGItem_C** key, uint32_t* entry) const;
    bool IsUsableInCombat() const;
    bool IsUsableMounted() const;
    bool IsUsableWhileImmune() const;
};

void ApplyGameObjectArtKit(CM2Model* model, int32_t artKit);
void GameObject_C_ProcessAnimEvent(uint32_t eventId, uint32_t displayId, uint32_t data, const C3Vector& position, SE2SoundKitObject* sound);
int32_t Packet_SMSG_GAMEOBJECT_CUSTOM_ANIM(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_GAMEOBJECT_DESPAWN_ANIM(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_GAMEOBJECT_PAGETEXT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

#endif
