#include "CDrawBowString.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/ObjectClient/GuildStats_C.h"

// VERIFY: fbd64b06 auto
// OFFSET: 0x007EADA0
bool GuildGetGuildTabard(WOWGUID guid, int32_t guildID, void (*callback)(int32_t guildID, const WOWGUID& guid, void* param, bool success), void* param, int32_t& emblemStyle, int32_t& emblemColor, int32_t& borderStyle, int32_t& borderColor, int32_t& background) {
    GuildStats_C* stats = g_guildInfoCache.GetRecord(guildID, guid, callback, param, false);

    emblemStyle = -1;
    emblemColor = -1;
    borderStyle = -1;
    borderColor = -1;
    background = -1;

    if (!stats || stats->m_emblemStyle == -1 || stats->m_emblemColor == -1 || stats->m_borderStyle == -1 || stats->m_borderColor == -1 || stats->m_background == -1) {
        return false;
    }

    emblemStyle = stats->m_emblemStyle;
    emblemColor = stats->m_emblemColor;
    borderStyle = stats->m_borderStyle;
    borderColor = stats->m_borderColor;
    background = stats->m_background;

    return true;
}
