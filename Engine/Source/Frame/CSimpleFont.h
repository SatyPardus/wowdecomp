#ifndef ENGINE_SOURCE_FRAME_CSIMPLEFONT_H
#define ENGINE_SOURCE_FRAME_CSIMPLEFONT_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Frame/CSimpleAnim.h"
#include "Engine/Source/Frame/CSimpleFontStringAttributes.h"
#include "Engine/Source/Frame/CSimpleFontable.h"
#include "Engine/Source/Storm/H/stpl.h"

class CSimpleFont;
class CStatus;
class XMLNode;

struct SIMPLEFONT : public TSHashObject<SIMPLEFONT, HASHKEY_STRI> {
    /* 0018 */ CSimpleFont* m_font;

    SIMPLEFONT();
    ~SIMPLEFONT();
};

class CSimpleFont : public FrameScript_Object, public CSimpleFontable {
    public:
    static int32_t s_metatable;
    static int32_t s_objectType;
    static const char* s_objectTypeName;

    static void CreateScriptMetaTable();
    static void Destroy();
    static void DestroyScriptMetaTable();
    static CSimpleFont* GetFont(const char* name, int32_t create);
    static int32_t GetObjectType();
    static void RegisterScriptMethods(lua_State* L);

    /* 0028 */ CSimpleFontStringAttributes m_attributes;
    /* 0058 */ const char* m_name;
    /* 005C */ uint32_t m_attributeMask;
    /* 0060 */ TSExplicitList<CSimpleFontable, offsetof(CSimpleFontable, m_fontableLink)> m_fontableList;

    virtual ~CSimpleFont();
    virtual const char* GetName();
    virtual int32_t GetScriptMetaTable();
    virtual void UpdateObjects();
    virtual bool IsA(int32_t type);
    virtual bool IsA(const char* typeName);
    virtual const char* GetObjectTypeName();
    virtual void FontObjectUpdated(CSimpleFontStringAttributes& attributes);

    CSimpleFont();
    CSimpleFont& operator=(const CSimpleFont& source);
    void LoadXML(XMLNode* node, CStatus* status);
    void RemoveFontable(CSimpleFontable* fontable);
};

int32_t CSimpleFont_GetFont(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_GetFontObject(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_GetIndentedWordWrap(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_GetJustifyH(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_GetJustifyV(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_GetShadowColor(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_GetShadowOffset(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_GetSpacing(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_GetTextColor(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_SetFont(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_SetFontObject(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_SetIndentedWordWrap(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_SetJustifyH(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_SetJustifyV(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_SetShadowColor(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_SetShadowOffset(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_SetSpacing(const char* name, CSimpleFont* font, lua_State* L);
int32_t CSimpleFont_SetTextColor(const char* name, CSimpleFont* font, lua_State* L);

#endif
