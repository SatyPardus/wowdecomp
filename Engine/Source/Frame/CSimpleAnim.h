#ifndef ENGINE_SOURCE_FRAME_CSIMPLEANIM_H
#define ENGINE_SOURCE_FRAME_CSIMPLEANIM_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/Types.h"
#include "Engine/Source/Storm/H/stpl.h"

class C2Vector;
class CScriptRegion;
class CSimpleAnimGroup;
class CStatus;
class XMLNode;

class CSimpleAnimSCurveSmoother {
    public:
    /* 0004 */ float m_in;
    /* 0008 */ float m_out;

    virtual void SetWeights(float in, float out);
    virtual float GetSmoothProgress(float progress);

    CSimpleAnimSCurveSmoother();
};

class CSimpleAnim : public CScriptObject {
    public:
    static int32_t s_metatable;
    static int32_t s_objectType;
    static const char* s_objectTypeName;

    static int32_t GetObjectType();
    static void CreateScriptMetaTable();
    static void DestroyScriptMetaTable();
    static void RegisterScriptMethods(lua_State* L);

    /* 0020 */ TSLink<CSimpleAnim> m_link;
    /* 0028 */ CSimpleAnimGroup* m_group;
    /* 002C */ uint8_t unused_002C;
    /* 002D */ uint8_t unused_002D[3];
    /* 0030 */ int8_t m_order;
    /* 0031 */ uint8_t m_flags;
    /* 0032 */ uint8_t unused_0032[2];
    /* 0034 */ uint8_t m_playState;
    /* 0035 */ int8_t m_loopState;
    /* 0036 */ uint8_t unused_0036[2];
    /* 0038 */ uint32_t unused_0038;
    /* 003C */ FrameScript_Object::ScriptIx m_onLoad;
    /* 0044 */ FrameScript_Object::ScriptIx m_onPlay;
    /* 004C */ FrameScript_Object::ScriptIx m_onPause;
    /* 0054 */ FrameScript_Object::ScriptIx m_onStop;
    /* 005C */ FrameScript_Object::ScriptIx m_onFinished;
    /* 0064 */ FrameScript_Object::ScriptIx m_onUpdate;
    /* 006C */ float m_startDelay;
    /* 0070 */ float m_endDelay;
    /* 0074 */ float m_duration;
    /* 0078 */ float m_maxFramerate;
    /* 007C */ float m_frameInterval;
    /* 0080 */ float m_frameElapsed;
    /* 0084 */ float m_elapsed;
    /* 0088 */ float m_progressWithDelay;
    /* 008C */ float m_progress;
    /* 0090 */ float m_smoothProgress;
    /* 0094 */ CSimpleAnimSCurveSmoother* m_smoother;

    virtual ~CSimpleAnim();
    virtual int32_t GetScriptMetaTable();
    virtual FrameScript_Object::ScriptIx* GetScriptByName(const char* name, FrameScript_Object::ScriptData& data);
    virtual bool IsA(int32_t type);
    virtual CScriptObject* GetScriptObjectParent();
    virtual bool IsA(const char* typeName);
    virtual const char* GetObjectTypeName();
    virtual void LoadXML(XMLNode* node, CStatus* status);
    virtual void PostLoadXML(XMLNode* node, CStatus* status);
    virtual void GetScriptTime(lua_State* L, double* time, uint32_t* calls);
    virtual void Apply(float progress);
    virtual void ApplyInverse(float progress);

    CSimpleAnim(CSimpleAnimGroup* group);
    bool ChangeParent(CSimpleAnimGroup* group);
    CScriptRegion* GetRegionParent();
    bool GetSmoothing(float& in, float& out);
    bool IsDelaying();
    void LoadXML_Scripts(XMLNode* node, CStatus* status);
    bool OnUpdate(float elapsedSec, float& consumed);
    void Pause();
    bool Play();
    void RunOnUpdateScript(float elapsedSec);
    void SetDuration(float duration, bool notify);
    void SetEndDelay(float delay, bool notify);
    void SetMaxFramerate(float framerate);
    void SetOrder(int32_t order, bool notify, bool wrap);
    void SetSmoothing(float in, float out);
    void SetSmoothProgress(float progress);
    void SetStartDelay(float delay, bool notify);
    void Stop(bool requested);
    float UpdateProgress(float elapsedSec);
};

void ReadOrigin(lua_State* L, int32_t index, FRAMEPOINT& point, C2Vector& offset);

#endif
