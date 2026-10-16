#ifndef ENGINE_SOURCE_FRAME_CSIMPLEANIMGROUP_H
#define ENGINE_SOURCE_FRAME_CSIMPLEANIMGROUP_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CSimpleAnim.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Storm/H/stpl.h"

class CScriptRegion;
class CStatus;
class XMLNode;

struct SIMPLEANIMNODE : public TSLinkedNode<SIMPLEANIMNODE> {
    /* 0008 */ CSimpleAnim* m_anim;
};

class CSimpleAnimGroup : public CScriptObject {
    public:
    static int32_t s_metatable;
    static int32_t s_objectType;
    static const char* s_objectTypeName;

    static int32_t GetObjectType();
    static void CreateScriptMetaTable();
    static void DestroyScriptMetaTable();
    static void RegisterScriptMethods(lua_State* L);

    /* 0020 */ TSLink<CSimpleAnimGroup> m_regionLink;
    /* 0028 */ TSLink<CSimpleAnimGroup> m_playingLink;
    /* 0030 */ CScriptRegion* m_region;
    /* 0034 */ TSExplicitList<CSimpleAnim, offsetof(CSimpleAnim, m_link)> m_animations;
    /* 0040 */ TSGrowableArray<TSList<SIMPLEANIMNODE, TSGetLink<SIMPLEANIMNODE> >*> m_animLists;
    /* 0050 */ FrameScript_Object::ScriptIx m_onLoad;
    /* 0058 */ FrameScript_Object::ScriptIx m_onPlay;
    /* 0060 */ FrameScript_Object::ScriptIx m_onPause;
    /* 0068 */ FrameScript_Object::ScriptIx m_onStop;
    /* 0070 */ FrameScript_Object::ScriptIx m_onFinished;
    /* 0078 */ FrameScript_Object::ScriptIx m_onUpdate;
    /* 0080 */ FrameScript_Object::ScriptIx m_onLoop;
    /* 0088 */ uint16_t m_flags;
    /* 008A */ uint8_t unused_008A[2];
    /* 008C */ int8_t m_loopType;
    /* 008D */ int8_t m_loopState;
    /* 008E */ uint8_t unused_008E[2];
    /* 0090 */ int32_t m_currentOrder;
    /* 0094 */ float m_elapsed;
    /* 0098 */ float m_duration;
    /* 009C */ float m_progress;
    /* 00A0 */ C2Vector m_initialOffset;

    virtual ~CSimpleAnimGroup();
    virtual int32_t GetScriptMetaTable();
    virtual FrameScript_Object::ScriptIx* GetScriptByName(const char* name, FrameScript_Object::ScriptData& data);
    virtual bool IsA(int32_t type);
    virtual CScriptObject* GetScriptObjectParent();
    virtual bool IsA(const char* typeName);
    virtual const char* GetObjectTypeName();

    CSimpleAnimGroup(CScriptRegion* owner);
    bool AddAnimListNode(TSList<SIMPLEANIMNODE, TSGetLink<SIMPLEANIMNODE> >* list, CSimpleAnim* anim);
    void AdvanceLoopState(float elapsedSec, float remaining);
    void ComputeDuration();
    void DoGroupUpdateWork(float elapsedSec);
    TSList<SIMPLEANIMNODE, TSGetLink<SIMPLEANIMNODE> >* FindAnimList(int32_t order);
    void Finish();
    int32_t GetMaxAnimOrder();
    bool HasAnimOfType(int32_t type);
    bool IsDone();
    void LoadXML(XMLNode* node, CStatus* status);
    void LoadXML_Anim(XMLNode* node, CStatus* status, CSimpleAnim* anim);
    void LoadXML_Scripts(XMLNode* node, CStatus* status);
    void NotifyAnimAdded(CSimpleAnim* anim);
    void NotifyAnimRemoved(CSimpleAnim* anim);
    bool NotifyAnimStopped(CSimpleAnim* anim);
    void NotifyOrderChanged(CSimpleAnim* anim, int32_t oldOrder);
    bool OnUpdate(float elapsedSec);
    void Pause();
    bool Play();
    void PostLoadXML(XMLNode* node, CStatus* status);
    void PreLoadXML(XMLNode* node, CStatus* status);
    TSList<SIMPLEANIMNODE, TSGetLink<SIMPLEANIMNODE> >* RegisterAnimList(int32_t order);
    bool RemoveAnimListNode(TSList<SIMPLEANIMNODE, TSGetLink<SIMPLEANIMNODE> >* list, CSimpleAnim* anim);
    void RunOnUpdateScript(float elapsedSec);
    void SetInitialOffset(const C2Vector& offset);
    void SortAnimations();
    void Stop(bool requested);
};

#endif
