#ifndef WOW_SOURCE_UI_DRESSUPMODELFRAME_H
#define WOW_SOURCE_UI_DRESSUPMODELFRAME_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/CDataAllocator.h"
#include "WoW/Source/Ui/CharacterModelBase.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"

class CCharacterComponent;
class CSimpleFrame;
class CGItem_C;
class CM2Model;
struct lua_State;

class CGDressUpModelFrame : public CGCharacterModelBase {
    public:
    static CDataAllocator s_dressUpModelHeap;
    static int32_t s_metatable;
    static int32_t s_objectType;
    static const char* s_objectTypeName;

    static void operator delete(void* ptr);
    static CSimpleFrame* Create(CSimpleFrame* parent);
    static void CreateScriptMetaTable();
    static void DestroyScriptMetaTable();
    static int32_t GetObjectType();
    static void RegisterScriptMethods(lua_State* L);

    /* 0380 */ CCharacterComponent* m_component;
    /* 0384 */ CGUnitVirtualItem m_mainHand;
    /* 038C */ CGUnitVirtualItem m_offHand;
    /* 0394 */ uint8_t m_isNativeDisplay;
    /* 0395 */ uint8_t unused_0395 [3];

    ~CGDressUpModelFrame() override;
    int32_t GetScriptMetaTable() override;
    bool IsA(int32_t type) override;
    bool IsA(const char* typeName) override;
    const char* GetObjectTypeName() override;
    void UpdateModel() override;
    void SetModel(CM2Model* model) override;
    CM2Model* GetDuplicateUnitCharacterModel(CGUnit_C* unit) override;

    CGDressUpModelFrame(CSimpleFrame* parent);
    void Undress();
    void TryOn(int32_t itemId, const uint32_t* enchants, int32_t hand);
    void TryOnWithVisual(int32_t itemId, int32_t visual, int32_t hand);
    void TryOnItem(CGItem_C* item, int32_t hand);
};

#endif
