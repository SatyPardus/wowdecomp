#ifndef WOW_SOURCE_GAME_GAMECLIENT_PLAYERNAME_H
#define WOW_SOURCE_GAME_GAMECLIENT_PLAYERNAME_H

#include <cstddef>
#include <cstdint>
#if defined(_WIN32)
#include <windows.h>
#endif
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/CImVector.h"

struct CVar;
class CGObject_C;
class CM2Lighting;
class CM2Model;
struct CGxString;
struct WORLDTEXTSTRING;

class PLAYERNAMEDESC {
    public:
    /* 0000 */ TSLink<PLAYERNAMEDESC> m_link;
    /* 0008 */ CGxString* m_string;
    /* 000C */ CImVector m_color;
    /* 0010 */ WOWGUID m_guid;
    /* 0018 */ uint32_t m_flags;
    /* 001C */ uint32_t m_renderFrame;
    /* 0020 */ WORLDTEXTSTRING* m_worldText [4];
    /* 0030 */ float m_zOffset;
    /* 0034 */ uint32_t unused_0034;

    static int32_t ShouldRender(CGObject_C* object, uint32_t raidTarget);

    void CreateWorldText(uint32_t type, const char* text, const CImVector* color, const int32_t* pvpRank);
    void DeleteWorldText(uint32_t type);
    void UpdateWorldText(uint32_t time);
    void UpdateWorldTextIcons();

    PLAYERNAMEDESC();
    ~PLAYERNAMEDESC();
    void Render();
    static void RenderIfVisible(CM2Model* model, const CM2Lighting* lighting, void* arg);
};

extern uint32_t s_nameMask;
extern TSExplicitList<PLAYERNAMEDESC, offsetof(PLAYERNAMEDESC, m_link)> s_playerNames;

PLAYERNAMEDESC* PlayerNameCreate(const WOWGUID& guid);
void PlayerNameDelete(PLAYERNAMEDESC* desc);
PLAYERNAMEDESC* ObjectNameCreate(const WOWGUID& guid);
void PlayerNameCreateText(PLAYERNAMEDESC* desc, uint32_t type, const char* text, const CImVector* color, const int32_t* pvpRank);
void PlayerNameTriggerColorUpdateAll();
void PlayerNameDeleteText(PLAYERNAMEDESC* desc, uint32_t type);

void PlayerNameTriggerColorUpdate(PLAYERNAMEDESC* desc);
void PlayerNameTriggerNameRegenerate(PLAYERNAMEDESC* desc);
void PlayerNameUpdateDrawCallback(PLAYERNAMEDESC* desc);
bool PlayerNameShowing(PLAYERNAMEDESC* desc);
float PlayerNameComputeScale(CGObject_C* object);

void PlayerNameInitialize();
void PlayerNameShutdown();
void PlayerNameUpdateEarly();
void PlayerNameUpdateLate();
void PlayerNameUpdateWorldText();

bool ObjectNameShowCallback(CVar* var, const char* oldValue, const char* value, void* arg);
void PlayerNameRegisterCVars();

#endif
