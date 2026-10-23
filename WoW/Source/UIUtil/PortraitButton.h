#ifndef WOW_SOURCE_UIUTIL_PORTRAITBUTTON_H
#define WOW_SOURCE_UIUTIL_PORTRAITBUTTON_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "Engine/Source/Tempest/CImVector.h"

class CGPartyPetState;
class CGUnit_C;
class CSimpleTexture;
class CTexture;

struct PORTRAITCACHE {
    /* 0000 */ uint8_t m_dirty;
    /* 0001 */ uint8_t m_pad[3];
    /* 0004 */ CTexture* m_texture;
    /* 0008 */ TSGrowableArray<CImVector> m_texels;
    /* 0018 */ PORTRAITCACHE* m_prev;
    /* 001C */ PORTRAITCACHE* m_next;

    PORTRAITCACHE();
};

struct PLAYERPORTRAIT : public TSHashObject<PLAYERPORTRAIT, CHashKeyGUID> {
    /* 0020 */ PORTRAITCACHE m_cache;
};

struct ITEMPORTRAIT : public TSHashObject<ITEMPORTRAIT, HASHKEY_STRI> {
    /* 0018 */ CTexture* m_texture;
    /* 001C */ TSGrowableArray<CImVector> m_texels;
};

struct PORTRAITREQUEST : public TSLinkedNode<PORTRAITREQUEST> {
    /* 0008 */ WOWGUID m_guid;
    /* 0010 */ uint32_t m_flags;
};

struct UNITPORTRAIT : public TSHashObject<UNITPORTRAIT, HASHKEY_DWORD> {
    /* 0018 */ PORTRAITCACHE m_cache;
};

int32_t RTAlphaSupported();
void __fastcall UpdateAllPortraits();

void PortraitInitialize();
void PortraitShutdown();
void UpdatePendingPortraits();
void SetPortraitTexture(CSimpleTexture* texture, const char* fileName);
void SetPortraitTexture(CSimpleTexture* texture, uint32_t race, uint32_t sex, WOWGUID guid);
void SetPortraitTexture(CSimpleTexture* texture, uint32_t displayId, CGPartyPetState* petState);
void RenderPortraitTexture(CSimpleTexture* texture, CGUnit_C* unit);

int32_t ReadyModelForUI(CGUnit_C* unit, uint32_t flags);

extern const char* s_sexNames[3];

#endif
