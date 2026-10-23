#ifndef WOW_SOURCE_GAME_GAMECLIENT_WORLDTEXT_H
#define WOW_SOURCE_GAME_GAMECLIENT_WORLDTEXT_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"

struct CGxString;
class CTexture;

enum WORLDTEXT_STYLE : uint32_t {
    WT_DAMAGE = 0,
    WT_ABSORB = 1,
    WT_CRIT = 2,
    WT_MISS = 3,
    WT_XPGAIN = 4,
    WT_HONORGAIN = 5,
    WT_HEAL = 6,
    WT_CRIT_HEAL = 7,
    WT_BUILDING_DAMAGE = 8,
    WT_BUILDING_HEAL = 9,
    WT_THREAT = 10,
    NUM_WORLDTEXTTYPES = 11,
    WT_ALL = 11, // Deletion selector; not an index into the style table.
};

struct WORLDTEXTTYPE {
    /* 0000 */ float m_ascendDistance;
    /* 0004 */ uint32_t m_fadeInTime;
    /* 0008 */ uint32_t m_fadeOutStart;
    /* 000C */ uint32_t m_duration;
    /* 0010 */ float m_startHeight;
    /* 0014 */ float m_endHeight;
    /* 0018 */ CImVector m_color;
};

struct SWorldTextIcon {
    /* 0000 */ C3Vector m_position;
    /* 000C */ float m_size;
    /* 0010 */ CImVector m_color;
    /* 0014 */ CTexture* m_texture;
};

struct WORLDTEXTSTRING : public CHandleObject {
    /* 0008 */ uint32_t m_type;
    /* 000C */ float m_halfWidth;
    /* 0010 */ float m_halfHeight;
    /* 0014 */ C3Vector m_position;
    /* 0020 */ CImVector m_color;
    /* 0024 */ uint32_t m_startTime;
    /* 0028 */ CGxString* m_string;
    /* 002C */ float m_fontHeight;
    /* 0030 */ C3Vector m_screenPosition;
    /* 003C */ char m_text[64];
    /* 007C */ int32_t m_pvpRank;

    ~WORLDTEXTSTRING() override;

    WORLDTEXTSTRING();
    void AddIcon();
    float CalculateAscendOffset(uint32_t elapsed);
    void CalculateNewColor(uint32_t elapsed);
    void CalculateTextHeight(uint32_t elapsed);
    void RecreateString();
    bool Update(uint32_t time);
};

void WorldTextAddIcon(WORLDTEXTSTRING* text);
WORLDTEXTSTRING* WorldTextCreate(uint32_t type, const C3Vector& position, const char* text, const CImVector* color, const int32_t* pvpRank);
void WorldTextDelete(WORLDTEXTSTRING* text);
float WorldTextGetAscendDistance(uint32_t type);
uint32_t WorldTextGetType(WORLDTEXTSTRING* text);
void WorldTextRenderBatch();
bool WorldTextUpdate(WORLDTEXTSTRING* text, uint32_t time);

#endif
