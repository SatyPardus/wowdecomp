#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include "Engine/Source/Frame/CSimpleAnim.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleAnimGroup.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"

// STATIC: 0x00AC19D8
int32_t CSimpleAnim::s_metatable = LUA_NOREF;

// STATIC: 0x00B499F8
int32_t CSimpleAnim::s_objectType;

// STATIC: 0x00AC138C
const char* CSimpleAnim::s_objectTypeName = "Animation";

// INLINED
int32_t CSimpleAnim::GetObjectType() {
    if (!CSimpleAnim::s_objectType) {
        CSimpleAnim::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleAnim::s_objectType;
}

// INLINED
CSimpleAnimSCurveSmoother::CSimpleAnimSCurveSmoother() {
    this->m_in = 0.0f;
    this->m_out = 0.0f;
}

// VERIFY: 3f4699a8 auto
// OFFSET: 0x00497B40
void CSimpleAnimSCurveSmoother::SetWeights(float in, float out) {
    this->m_in = in < 0.0f ? 0.0f : (in < 1.0f ? in : 1.0f);
    this->m_out = out < 0.0f ? 0.0f : (out < 1.0f ? out : 1.0f);
}

// VERIFY: b5774e69 manual
// OFFSET: 0x00497BA0
float CSimpleAnimSCurveSmoother::GetSmoothProgress(float progress) {
    if (fabsf(this->m_in) >= 0.001f && fabsf(this->m_out) >= 0.001f) {
        return 0.5f - cosf(progress * 3.1415927f) * 0.5f;
    }

    if (fabsf(this->m_in) >= 0.001f) {
        return 1.0f - cosf(progress * 1.5707964f);
    }

    if (fabsf(this->m_out) >= 0.001f) {
        return cosf((progress + 1.0f) * 1.5707964f) * -1.0f;
    }

    return progress;
}

// VERIFY: 4fccd14d auto
// OFFSET: 0x00497500
FrameScript_Object::ScriptIx* CSimpleAnim::GetScriptByName(const char* name, FrameScript_Object::ScriptData& data) {
    FrameScript_Object::ScriptIx* script = this->FrameScript_Object::GetScriptByName(name, data);

    if (script) {
        return script;
    }

    if (!SStrCmpI(name, "OnLoad", STORM_MAX_STR)) {
        return &this->m_onLoad;
    }

    if (!SStrCmpI(name, "OnPlay", STORM_MAX_STR)) {
        return &this->m_onPlay;
    }

    if (!SStrCmpI(name, "OnPause", STORM_MAX_STR)) {
        return &this->m_onPause;
    }

    if (!SStrCmpI(name, "OnStop", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,requested) %s end";
        return &this->m_onStop;
    }

    if (!SStrCmpI(name, "OnFinished", STORM_MAX_STR)) {
        return &this->m_onFinished;
    }

    if (!SStrCmpI(name, "OnUpdate", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,elapsed) %s end";
        return &this->m_onUpdate;
    }

    return nullptr;
}

// VERIFY: c5c3a1e6 auto
// OFFSET: 0x004975F0
void CSimpleAnim::GetScriptTime(lua_State* L, double* time, uint32_t* calls) {
    this->FrameScript_Object::GetScriptTime(L, this->m_onLoad, time, calls);
    this->FrameScript_Object::GetScriptTime(L, this->m_onPlay, time, calls);
    this->FrameScript_Object::GetScriptTime(L, this->m_onPause, time, calls);
    this->FrameScript_Object::GetScriptTime(L, this->m_onStop, time, calls);
    this->FrameScript_Object::GetScriptTime(L, this->m_onFinished, time, calls);
    this->FrameScript_Object::GetScriptTime(L, this->m_onUpdate, time, calls);
}

// VERIFY: 0365b691 auto
// OFFSET: 0x00497670
void CSimpleAnim::SetSmoothProgress(float progress) {
    this->m_smoothProgress = progress;
}

// VERIFY: 64ce95af manual
// OFFSET: 0x00497680
void CSimpleAnim::SetMaxFramerate(float framerate) {
    if (framerate > 0.0001f) {
        this->m_maxFramerate = framerate;
        this->m_frameInterval = 1.0f / framerate;
    } else {
        this->m_maxFramerate = 0.0f;
        this->m_frameInterval = 0.0f;
    }
}

// VERIFY: 59850deb auto
// OFFSET: 0x004976C0
bool CSimpleAnim::GetSmoothing(float& in, float& out) {
    if (this->m_smoother) {
        in = this->m_smoother->m_in;
        out = this->m_smoother->m_out;
        return true;
    }

    in = 0.0f;
    out = 0.0f;
    return false;
}

// VERIFY: 78802c25 auto
// OFFSET: 0x00497AC0
void CSimpleAnim::RunOnUpdateScript(float elapsedSec) {
    if (this->m_onUpdate.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushnumber(L, elapsedSec);
        this->RunScript(this->m_onUpdate, 1, nullptr);
    }
}

// VERIFY: 661dd21d manual
// OFFSET: 0x00497C30
void CSimpleAnim::LoadXML_Scripts(XMLNode* node, CStatus* status) {
    lua_State* L = FrameScript_GetContext();

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        const char* body = child->m_body;
        const char* scriptName = child->m_name.m_str;

        FrameScript_Object::ScriptData data;
        data.m_wrapper = "return function(self) %s end";

        FrameScript_Object::ScriptIx* script = this->GetScriptByName(scriptName, data);

        if (!script) {
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "%s %s: Unknown script element %s", this->GetObjectTypeName(), name ? name : "<unnamed>", scriptName);
            continue;
        }

        if (script->m_luaRef) {
            luaL_unref(L, LUA_REGISTRYINDEX, script->m_luaRef);
        }

        if (!child->m_userData && body && *body) {
            char functionName[1024];
            SStrPrintf(functionName, sizeof(functionName), "*:%s", scriptName);

            void* mem = SMemAlloc(sizeof(FrameScriptFunctionRef), __FILE__, __LINE__, 0);
            child->m_userData = mem ? new (mem) FrameScriptFunctionRef(FrameScript_CompileFunction(functionName, data.m_wrapper, body, status)) : nullptr;
        }

        const char* function = child->GetAttributeByName("function");

        if (function && *function) {
            lua_pushstring(L, function);
            lua_rawget(L, LUA_GLOBALSINDEX);
            int32_t ref = luaL_ref(L, LUA_REGISTRYINDEX);

            if (ref == -1) {
                const char* name = this->GetName();
                status->Add(STATUS_WARNING, "%s %s: Unknown function %s in element %s", this->GetObjectTypeName(), name ? name : "<unnamed>", function, scriptName);
                script->m_luaRef = 0;
            } else {
                script->m_luaRef = ref;
            }
        } else if (child->m_userData) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, static_cast<FrameScriptFunctionRef*>(child->m_userData)->m_function);
            script->m_luaRef = luaL_ref(L, LUA_REGISTRYINDEX);
        } else {
            script->m_luaRef = 0;
        }

        script->m_tainted = nullptr;
    }
}

// VERIFY: eb02a39f manual
// OFFSET: 0x00497E40
void CSimpleAnim::PostLoadXML(XMLNode* node, CStatus* status) {
    this->m_flags ^= 1;

    if (this->m_onLoad.m_luaRef) {
        this->RunScript(this->m_onLoad, 0, nullptr);
    }
}

// VERIFY: 25606c0d auto
// OFFSET: 0x00497E60
void CSimpleAnim::Pause() {
    this->m_group->Pause();

    if (this->m_playState == 1) {
        this->m_playState = 2;

        if (this->m_onPause.m_luaRef) {
            this->RunScript(this->m_onPause, 0, nullptr);
        }
    }
}

// VERIFY: a55e865d manual
// OFFSET: 0x00497E90
bool CSimpleAnim::IsDelaying() {
    if (this->m_progressWithDelay <= 0.0f || this->m_progressWithDelay >= 1.0f) {
        return false;
    }

    float total = this->m_endDelay + this->m_duration + this->m_startDelay;

    if (fabsf(total) < 0.00000023841858f) {
        return false;
    }

    if (this->m_loopState == ANIM_LOOPSTATE_REVERSE) {
        if (this->m_endDelay > this->m_elapsed) {
            return true;
        }

        if (this->m_endDelay + this->m_duration > this->m_elapsed) {
            return false;
        }
    } else {
        if (this->m_startDelay > this->m_elapsed) {
            return true;
        }

        if (this->m_duration + this->m_startDelay > this->m_elapsed) {
            return false;
        }
    }

    return total > this->m_elapsed;
}

// VERIFY: 792cca8f manual
// OFFSET: 0x00497F30
void CSimpleAnim::SetSmoothing(float in, float out) {
    if ((in > 0.0f && in <= 1.0f) || (out > 0.0f && out <= 1.0f)) {
        if (!this->m_smoother) {
            void* mem = SMemAlloc(sizeof(CSimpleAnimSCurveSmoother), __FILE__, __LINE__, 0);
            this->m_smoother = mem ? new (mem) CSimpleAnimSCurveSmoother() : nullptr;
        }

        if (this->m_smoother) {
            this->m_smoother->SetWeights(in, out);
        }
    }
}

// VERIFY: 5bb9c1f7 auto
// OFFSET: 0x00497FE0
CScriptRegion* CSimpleAnim::GetRegionParent() {
    return this->m_group ? this->m_group->m_region : nullptr;
}

// VERIFY: 23c1384f manual
// OFFSET: 0x004985F0
float CSimpleAnim::UpdateProgress(float elapsedSec) {
    float prevElapsed = this->m_elapsed;
    this->m_elapsed += elapsedSec;

    float total = this->m_startDelay + this->m_endDelay + this->m_duration;
    float elapsed;

    if (total > this->m_elapsed) {
        elapsed = this->m_elapsed;
        this->m_progressWithDelay = elapsed / total;

        if (this->m_progressWithDelay > 1.0f) {
            this->m_progressWithDelay = 1.0f;
        }

        if (fabsf(this->m_duration) < 0.00000023841858f) {
            this->m_progress = 1.0f;
        } else {
            this->m_frameElapsed += elapsedSec;

            if (this->m_frameElapsed >= this->m_frameInterval) {
                float delay = this->m_loopState == ANIM_LOOPSTATE_REVERSE ? this->m_endDelay : this->m_startDelay;
                this->m_progress = (elapsed - delay) / this->m_duration;

                if (this->m_progress < 0.0f) {
                    this->m_progress = 0.0f;
                } else if (this->m_progress >= 1.0f) {
                    this->m_progress = 1.0f;
                }

                this->m_frameElapsed = 0.0f;
            }
        }
    } else {
        elapsed = total;
        this->m_progressWithDelay = 1.0f;
        this->m_progress = 1.0f;
    }

    float progress = this->m_loopState == ANIM_LOOPSTATE_REVERSE ? 1.0f - this->m_progress : this->m_progress;

    if (this->m_smoother) {
        progress = this->m_smoother->GetSmoothProgress(progress);
    }

    this->m_smoothProgress = progress;

    return elapsed - prevElapsed;
}

// VERIFY: 2d79de85 manual
// OFFSET: [0x00498C10, 0x00499780]
CSimpleAnim::~CSimpleAnim() {
    delete this->m_smoother;
}

// VERIFY: 0e40cb2a auto
// OFFSET: 0x00498CB0
bool CSimpleAnim::IsA(int32_t type) {
    return type == CSimpleAnim::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 0489e386 auto
// OFFSET: 0x00498CE0
bool CSimpleAnim::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleAnim::s_objectTypeName, STORM_MAX_STR) || !SStrCmpI(typeName, CScriptObject::s_objectTypeName, STORM_MAX_STR);
}

// VERIFY: 80ff8422 auto
// OFFSET: 0x00498D30
const char* CSimpleAnim::GetObjectTypeName() {
    return CSimpleAnim::s_objectTypeName;
}

// VERIFY: a447b9f3 auto
// OFFSET: 0x00498D40
int32_t CSimpleAnim::GetScriptMetaTable() {
    return CSimpleAnim::s_metatable;
}

// VERIFY: 22da5b2a auto
// OFFSET: 0x00498D50
bool CSimpleAnim::OnUpdate(float elapsedSec, float& consumed) {
    consumed = 0.0f;

    if (this->m_progressWithDelay >= 1.0f) {
        this->Apply(this->m_smoothProgress);
        return true;
    }

    if (!this->m_playState) {
        return true;
    }

    if (this->m_playState == 2) {
        this->Apply(this->m_smoothProgress);
        return false;
    }

    consumed = this->UpdateProgress(elapsedSec);
    this->RunOnUpdateScript(elapsedSec);
    this->Apply(this->m_smoothProgress);

    if (this->m_progressWithDelay < 1.0f) {
        return false;
    }

    if (this->m_onFinished.m_luaRef) {
        this->RunScript(this->m_onFinished, 0, nullptr);
    }

    return true;
}

// VERIFY: a66c18b2 manual
// OFFSET: 0x00499BB0
CSimpleAnim::CSimpleAnim(CSimpleAnimGroup* group) {
    this->m_group = nullptr;
    this->unused_002C = 0;
    this->m_order = -1;
    this->m_flags = 0;
    this->m_playState = 0;
    this->m_loopState = 0;
    this->m_startDelay = 0.0f;
    this->m_endDelay = 0.0f;
    this->m_duration = 0.0f;
    this->m_maxFramerate = 0.0f;
    this->m_frameInterval = 0.0f;
    this->m_frameElapsed = 0.0f;
    this->m_elapsed = 0.0f;
    this->m_progressWithDelay = 0.0f;
    this->m_progress = 0.0f;
    this->m_smoother = nullptr;

    if (group) {
        this->m_group = group;
        group->m_animations.LinkToTail(this);
    }
}

// VERIFY: 9f5ca4be auto
// OFFSET: 0x0049AD80
bool CSimpleAnim::Play() {
    if (!this->m_group->Play()) {
        return false;
    }

    if (this->m_playState != 1) {
        this->m_playState = 1;

        if (!this->m_loopState) {
            this->m_loopState = ANIM_LOOPSTATE_FORWARD;
        }

        if (this->m_onPlay.m_luaRef) {
            this->RunScript(this->m_onPlay, 0, nullptr);
        }
    }

    return true;
}

// VERIFY: f1643e49 manual
// OFFSET: 0x0049ADC0
void CSimpleAnim::Stop(bool requested) {
    if (!this->m_playState) {
        return;
    }

    this->m_elapsed = 0.0f;
    this->m_progressWithDelay = 0.0f;
    this->m_progress = 0.0f;
    this->m_loopState = ANIM_LOOPSTATE_NONE;
    this->m_playState = 0;

    if (this->m_onStop.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushboolean(L, requested);
        this->RunScript(this->m_onStop, 1, nullptr);
    }

    this->m_group->NotifyAnimStopped(this);
}

// VERIFY: 08dbcc72 auto
// OFFSET: 0x0049AE20
void CSimpleAnim::SetStartDelay(float delay, bool notify) {
    this->m_startDelay = delay;

    if (delay < 0.0f) {
        this->m_startDelay = 0.0f;
    }

    if (notify && this->m_group) {
        this->m_group->ComputeDuration();
    }
}

// VERIFY: 5ce1823c auto
// OFFSET: 0x0049AE60
void CSimpleAnim::SetEndDelay(float delay, bool notify) {
    this->m_endDelay = delay;

    if (delay < 0.0f) {
        this->m_endDelay = 0.0f;
    }

    if (notify && this->m_group) {
        this->m_group->ComputeDuration();
    }
}

// VERIFY: f06d0373 auto
// OFFSET: 0x0049AEA0
void CSimpleAnim::SetDuration(float duration, bool notify) {
    this->m_duration = duration;

    if (notify && this->m_group) {
        this->m_group->ComputeDuration();
    }
}

// VERIFY: ebd63269 auto
// OFFSET: 0x0049AEC0
void CSimpleAnim::SetOrder(int32_t order, bool notify, bool wrap) {
    int32_t oldOrder = this->m_order;

    if (wrap) {
        this->m_order = order < 0 || order > 99 ? 0 : order;
    } else {
        this->m_order = order < 0 ? 0 : (order > 99 ? 99 : order);
    }

    if (notify && this->m_group && oldOrder != order) {
        this->m_group->NotifyOrderChanged(this, oldOrder);
    }
}

// VERIFY: e53e8fe4 manual
// OFFSET: 0x0049B810
void CSimpleAnim::LoadXML(XMLNode* node, CStatus* status) {
    this->m_flags |= 1;

    const char* inherits = node->GetAttributeByName("inherits");

    if (inherits && *inherits) {
        const char* taint;
        bool locked;
        XMLNode* inheritNode = FrameXML_AcquireHashNode(inherits, taint, locked);

        if (inheritNode) {
            if (locked) {
                status->Add(STATUS_WARNING, "Recursively inherited node: %s", inherits);
            } else {
                this->LoadXML(inheritNode, status);
                FrameXML_ReleaseHashNode(inherits);
            }
        } else {
            status->Add(STATUS_WARNING, "Couldn't find inherited node: %s", inherits);
        }
    }

    const char* parentKey = node->GetAttributeByName("parentKey");

    if (parentKey && *parentKey) {
        lua_State* L = FrameScript_GetContext();
        CScriptObject* parent = this->GetScriptObjectParent();

        if (parent) {
            if (!parent->m_luaRegistered) {
                parent->RegisterScriptObject(nullptr);
            }

            lua_rawgeti(L, LUA_REGISTRYINDEX, parent->m_luaObjectRef);
            lua_pushstring(L, parentKey);

            if (!this->m_luaRegistered) {
                this->RegisterScriptObject(nullptr);
            }

            lua_rawgeti(L, LUA_REGISTRYINDEX, this->m_luaObjectRef);
            lua_settable(L, -3);
            lua_settop(L, -2);
        }
    }

    const char* startDelay = node->GetAttributeByName("startDelay");

    if (startDelay && *startDelay) {
        float delay = SStrToFloat(startDelay);
        this->m_startDelay = delay;

        if (delay < 0.0f) {
            this->m_startDelay = 0.0f;
        }
    }

    const char* endDelay = node->GetAttributeByName("endDelay");

    if (endDelay && *endDelay) {
        float delay = SStrToFloat(endDelay);
        this->m_endDelay = delay;

        if (delay < 0.0f) {
            this->m_endDelay = 0.0f;
        }
    }

    const char* duration = node->GetAttributeByName("duration");

    if (duration && *duration) {
        this->m_duration = SStrToFloat(duration);
    }

    const char* maxFramerate = node->GetAttributeByName("maxFramerate");

    if (maxFramerate && *maxFramerate) {
        this->SetMaxFramerate(SStrToFloat(maxFramerate));
    }

    const char* smoothing = node->GetAttributeByName("smoothing");

    if (smoothing && *smoothing) {
        float in;
        float out;

        if (StringToSmoothWeights(smoothing, in, out)) {
            this->SetSmoothing(in, out);
        } else {
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "%s: Invalid smoothing value: %s", name ? name : "<unnamed>", smoothing);
        }
    }

    const char* orderString = node->GetAttributeByName("order");

    if (orderString && *orderString) {
        int32_t order = SStrToInt(orderString) - 1;

        if (order < 0 || order > 99) {
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "%s: Invalid order value: %s. Order must be between %d and %d.", name ? name : "<unnamed>", orderString, 1, 100);
        }

        this->m_order = order < 0 ? 0 : (order > 99 ? 99 : order);
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "Scripts", STORM_MAX_STR)) {
            this->LoadXML_Scripts(child, status);
        }
    }
}

// VERIFY: 47ca2bcf auto
// OFFSET: 0x0049C6C0
bool CSimpleAnim::ChangeParent(CSimpleAnimGroup* group) {
    if (!group || this->m_group == group) {
        return false;
    }

    group->m_animations.LinkToTail(this);

    if (this->m_group) {
        this->m_group->NotifyAnimRemoved(this);
    }

    this->m_group = group;
    group->NotifyAnimAdded(this);

    return true;
}

// VERIFY: f0d68712 auto
// OFFSET: 0x00632050
void CSimpleAnim::Apply(float progress) {
}

// VERIFY: 257ae09c auto
// OFFSET: 0x00632050
void CSimpleAnim::ApplyInverse(float progress) {
}

// VERIFY: 3234216c auto
// OFFSET: 0x004383C0
CScriptObject* CSimpleAnim::GetScriptObjectParent() {
    return this->m_group;
}
