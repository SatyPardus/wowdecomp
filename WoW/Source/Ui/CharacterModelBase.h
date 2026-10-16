#ifndef WOW_SOURCE_UI_CHARACTERMODELBASE_H
#define WOW_SOURCE_UI_CHARACTERMODELBASE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Frame/CSimpleModel.h"
#include "WoW/Source/Object/WGUID.h"

class CGUnit_C;
class CreatureStats_C;
class CM2Model;
class CSimpleFrame;
struct lua_State;

class CGCharacterModelBase : public CSimpleModel {
    public:
    static CDataAllocator s_characterModelHeap;
    static int32_t s_metatable;
    static int32_t s_objectType;
    static const char* s_objectTypeName;

    static void operator delete(void* ptr);
    static CSimpleFrame* Create(CSimpleFrame* parent);
    static void CreateScriptMetaTable();
    static void DestroyScriptMetaTable();
    static int32_t GetObjectType();
    static void RegisterScriptMethods(lua_State* L);

    /* 0368 */ WOWGUID m_unitGuid;
    /* 0370 */ uint32_t m_sequenceSet;
    /* 0374 */ uint32_t m_sequenceResetTime;
    /* 0378 */ CreatureStats_C* m_creatureStats;
    /* 037C */ uint32_t unused_037C;

    ~CGCharacterModelBase() override;
    int32_t GetScriptMetaTable() override;
    bool IsA(int32_t type) override;
    bool IsA(const char* typeName) override;
    const char* GetObjectTypeName() override;
    int32_t HideThis() override;
    int32_t ShowThis() override;
    int32_t IsDrawable() override;
    void UpdateModel() override;
    void OnModelLoaded(CM2Model* model) override;
    virtual CM2Model* GetDuplicateUnitCharacterModel(CGUnit_C* unit);

    CGCharacterModelBase(CSimpleFrame* parent);
    void ConfigureCamera();
    void RefreshUnit(WOWGUID guid);
    void SetUpModel(CGUnit_C* unit);
    void SetUpModel(CreatureStats_C* stats);
    void SetCreature(CreatureStats_C* stats);
    void RotateModel(float facing);
};

#endif
