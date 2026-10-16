#ifndef WOW_SOURCE_UIUTIL_TOOLTIP_H
#define WOW_SOURCE_UIUTIL_TOOLTIP_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/FrameScript/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Common/WowTime.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/UIUtil/Types.h"

class CImVector;
class CSimpleFontString;
class CSimpleStatusBar;
class CSimpleTexture;
class CStatus;
class XMLNode;
struct lua_State;

class ItemLinkInfo {
    public:
    /* 0000 */ uint32_t m_itemId;
    /* 0004 */ uint32_t m_enchants [12];
    /* 0034 */ uint32_t m_enchantDurations [12];
    /* 0064 */ int32_t m_enchantCharges [12];
    /* 0094 */ uint32_t m_cooldown;
    /* 0098 */ int32_t m_proposedEnchant;
    /* 009C */ uint32_t unused_009C;
    /* 00A0 */ WOWGUID m_creator;
    /* 00A8 */ WOWGUID m_giftCreator;
    /* 00B0 */ uint32_t m_propertySeed;
    /* 00B4 */ int32_t m_randomPropertiesId;
    /* 00B8 */ int32_t m_spellCharges;
    /* 00BC */ uint32_t m_wrapped;
    /* 00C0 */ uint32_t m_lockId;
    /* 00C4 */ uint32_t m_maxDurability;
    /* 00C8 */ uint32_t m_durability;
    /* 00CC */ int32_t m_level;
    /* 00D0 */ uint32_t m_stackCount;
    /* 00D4 */ int32_t m_maxStackCount;

    void Clear();
    int32_t GetItemIdFromLink(const char* link);
};

extern const char* s_inventoryTypeTokens[29];

class CGItemStatsSummary;
class CGTalentGroup_C;
struct ScriptAuraInfo;
class CGTooltip;
class TalentRec;
class CAaRect;
class CGItem_C;
class CGUnit_C;
class CImVector;
class CSimpleFontString;

class CGPlayer_C;
class ItemNameCache_C;
class ItemSetRec;
class ItemStats_C;
class ScalingStatValuesRec;
class SpellItemEnchantmentRec;
class SpellRec;

struct FrameStackInfo {
    /* 0000 */ char m_name [1024];
    /* 0400 */ int32_t m_strata;
    /* 0404 */ int32_t m_level;
    /* 0408 */ bool m_mouseEnabled;
    /* 040C */ uint32_t m_visible;
};

struct ItemSetTooltipData {
    /* 0000 */ CGTooltip* m_tooltip;
    /* 0004 */ const ItemSetRec* m_set;
    /* 0008 */ ItemNameCache_C* m_items [17];
    /* 004C */ int32_t* m_equipped;
    /* 0050 */ uint8_t m_slotUsed [19];
};

void TooltipItemCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
int32_t GetScalingStatBudget(const ScalingStatValuesRec* values, uint32_t flags);
int32_t GetScalingStatArmor(const ScalingStatValuesRec* values, uint32_t flags);
void GetScalingStatDamage(const ScalingStatValuesRec* values, uint32_t flags, float* minDamage, float* maxDamage);
int32_t GetFeralAttackPower(const ItemStats_C* stats, float damage);
int32_t GetEnchantmentConditionText(CGPlayer_C* player, const SpellItemEnchantmentRec* enchant, char* buffer, uint32_t size, int32_t newline);
bool ResolveItemSetItem(ItemSetTooltipData* data, int32_t pass, int32_t itemId, int32_t slot);
void TooltipUnitNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void TooltipCorpseCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void TooltipSpellCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void TooltipObjectCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void TooltipQuestCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
void TooltipAchievementCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
int32_t FormatString(const char* token, int32_t count, FRAMESCRIPT_GENDER gender, char* buffer, uint32_t size);
int32_t TimeFractionBelow(float time, float fraction);

class CGTooltip : public CSimpleFrame {
    public:
    static CDataAllocator s_tooltipHeap;
    static int32_t s_metatable;
    static int32_t s_objectType;
    static const char* s_objectTypeName;

    static void operator delete(void* ptr);
    static CSimpleFrame* Create(CSimpleFrame* parent);
    static void CreateScriptMetaTable();
    static void DestroyScriptMetaTable();
    static const CImVector* GetItemQualityColor(uint32_t quality);
    static const char* GetItemQualityColorString(uint32_t quality);
    static const char* CreateItemLink(int32_t itemId, uint32_t quality, int32_t enchant, const int32_t* gems, int32_t randomPropertiesId, int32_t seed);
    static const char* GetItemHyperlinkString(CGItem_C* item);
    static const uint32_t* GetThreatStateColor(uint32_t status);
    static const char* GetItemHyperlinkString(int32_t itemId, uint32_t quality, int32_t enchant, int32_t randomPropertiesId, int32_t seed);
    static const char* GetQuestDifficultyColorString(int32_t level);
    static const char* GetQuestLink(int32_t questId, int32_t level);
    static const char* GetSpellLink(int32_t spellId);
    static const char* GetGlyphLink(int32_t slotId, int32_t glyphId);
    static const char* GetTalentHyperlinkString(const TalentRec* talent, const char* name, int32_t rank);
    static const char* GetAchievementHyperlink(int32_t achievementId, WOWGUID guid, int32_t a3, WowTime time, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
    static int32_t GetObjectType();
    static void RegisterScriptMethods(lua_State* L);

    /* 029C */ CSimpleFrame* m_owner;
    /* 02A0 */ int32_t m_anchorType;
    /* 02A4 */ uint32_t m_numLines;
    /* 02A8 */ uint32_t m_maxLines;
    /* 02AC */ TSFixedArray<CSimpleFontString*> m_leftStrings;
    /* 02B8 */ TSFixedArray<CSimpleFontString*> m_rightStrings;
    /* 02C4 */ TSFixedArray<int32_t> m_lineWrap;
    /* 02D0 */ CSimpleStatusBar* m_statusBar;
    /* 02D4 */ CSimpleTexture* m_textures [10];
    /* 02FC */ uint32_t m_textureLines [10];
    /* 0324 */ uint32_t m_numTextures;
    /* 0328 */ WOWGUID m_unitGuid;
    /* 0330 */ WOWGUID m_objectGuid;
    /* 0338 */ uint32_t unused_0338;
    /* 033C */ uint32_t unused_033C;
    /* 0340 */ WOWGUID m_itemGuid;
    /* 0348 */ WOWGUID m_corpseGuid;
    /* 0350 */ uint32_t m_corpseOwnerGuid;
    /* 0354 */ uint32_t unused_0354;
    /* 0358 */ WOWGUID m_healthGuid;
    /* 0360 */ int32_t m_itemId;
    /* 0364 */ int32_t m_spellId;
    /* 0368 */ int32_t m_questId;
    /* 036C */ int32_t m_achievementId;
    /* 0370 */ int32_t m_nextRankSpellId;
    /* 0374 */ uint32_t unused_0374;
    /* 0378 */ WOWGUID m_achievementGuid;
    /* 0380 */ bool m_achievementCompleted;
    /* 0384 */ WowTime m_achievementTime;
    /* 03A4 */ uint32_t m_achievementCriteria [4];
    /* 03B4 */ int32_t m_itemNameOnly;
    /* 03B8 */ int32_t m_itemSocketPreview;
    /* 03BC */ int32_t m_itemCompare;
    /* 03C0 */ WOWGUID m_itemOwner;
    /* 03C8 */ int32_t m_pendingQueries;
    /* 03CC */ uint32_t m_fading;
    /* 03D0 */ float m_fadeTime;
    /* 03D4 */ float m_padding;
    /* 03D8 */ float m_minimumWidth;
    /* 03DC */ uint32_t m_minimumWidthForce;
    /* 03E0 */ float m_offsetX;
    /* 03E4 */ float m_offsetY;
    /* 03E8 */ ItemLinkInfo m_itemLinkInfo;
    /* 04C0 */ int32_t m_itemUseLinkInfo;
    /* 04C4 */ int32_t m_itemHideCharges;
    /* 04C8 */ int32_t m_itemMinimumLevel;
    /* 04CC */ int32_t m_itemSkipSellPrice;
    /* 04D0 */ int32_t m_itemDisplayedLevel;
    /* 04D4 */ FrameScript_Object::ScriptIx m_onTooltipSetDefaultAnchor;
    /* 04DC */ FrameScript_Object::ScriptIx m_onTooltipCleared;
    /* 04E4 */ FrameScript_Object::ScriptIx m_onTooltipAddMoney;
    /* 04EC */ FrameScript_Object::ScriptIx m_onTooltipSetUnit;
    /* 04F4 */ FrameScript_Object::ScriptIx m_onTooltipSetItem;
    /* 04FC */ FrameScript_Object::ScriptIx m_onTooltipSetSpell;
    /* 0504 */ FrameScript_Object::ScriptIx m_onTooltipSetQuest;
    /* 050C */ FrameScript_Object::ScriptIx m_onTooltipSetAchievement;
    /* 0514 */ FrameScript_Object::ScriptIx m_onTooltipSetEquipmentSet;
    /* 051C */ FrameScript_Object::ScriptIx m_onTooltipSetFrameStack;
    /* 0524 */ uint32_t unused_0524;

    ~CGTooltip() override;
    int32_t GetScriptMetaTable() override;
    FrameScript_Object::ScriptIx* GetScriptByName(const char* name, FrameScript_Object::ScriptData& data) override;
    bool IsA(int32_t type) override;
    bool IsA(const char* typeName) override;
    const char* GetObjectTypeName() override;
    void GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse) override;
    void PostLoadXML(XMLNode* node, CStatus* status) override;
    void OnLayerUpdate(float elapsedSec) override;
    int32_t HideThis() override;
    int32_t ShowThis() override;

    CGTooltip(CSimpleFrame* parent);
    void CalculateSize();
    void AppendText(const char* text);
    void ClearTooltip();
    void AddDeltaHeader(int32_t* first);
    void AddColoredItemStat(int32_t value, const char* name, int32_t* first);
    int32_t SetItemDelta(const CGItemStatsSummary* summary, int32_t append);
    void SetBuff(int32_t spellId, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
    void SetFrameStack(bool showHidden);
    void FadeOut();
    void ResetPosition(int32_t force);
    void SetOwner(CSimpleFrame* owner, TOOLTIP_ANCHORPOINT anchorType, float offsetX, float offsetY);
    void SetDefaultAnchor();
    int32_t SetUnit(const WOWGUID& guid, int32_t showHealth, const WOWGUID& healthGuid, int32_t a4);
    void SetObject(const WOWGUID& guid);
    void SetCorpse(const WOWGUID& guid);
    void AddFontStrings(CSimpleFontString* left, CSimpleFontString* right);
    void AddLine(const char* left, const char* right, const CImVector& leftColor, const CImVector& rightColor, int32_t wrap);
    void AddLine(const char* left, const char* right, int32_t wrap);
    int32_t SetMacro(uint32_t macroId);
    int32_t SetEquipmentSet(uint32_t setId);
    void AddTexture(const char* texture, const CAaRect& coords, const CImVector& color);
    void AddEmpathyText(CGUnit_C* unit, char* buffer, uint32_t size);
    void AddSkinnableText(CGUnit_C* unit, char* buffer, uint32_t size);
    static void GetSummonedByString(CGUnit_C* unit, char* buffer, uint32_t size);
    static void GetDurationString(char* buffer, uint32_t size, uint64_t time, const char* token, const char* prefix, int32_t roundUp, int32_t seconds);
    static void GetItemDurationString(char* buffer, uint32_t size, uint64_t time, const char* token);
    static void __stdcall FormatStringForXML(const char* text, char* buffer);
    static int32_t GetDisplayedScalingItemLevel(const ItemStats_C* stats, const ItemLinkInfo* link, WOWGUID target, int32_t minimumLevel);
    void AddMoney(int32_t money, int32_t maxMoney);
    void AddSpellReagents(const SpellRec* spell, const char* token, void (*callback)(int32_t id, const WOWGUID& guid, void* param, bool success));
    int32_t SetItemNoStats();
    static void GetFloatDurationString(char* buffer, uint32_t size, float time, const char* token, const char* prefix);
    int32_t AddTalentPrereqs(const TalentRec* talent, CGTalentGroup_C* group, int32_t isInspect, int32_t isPet, int32_t usePreview);
    void AddTalentLearnText(const TalentRec* talent, CGTalentGroup_C* group, int32_t prereqsMet, int32_t rank, int32_t maxRank, int32_t a7, int32_t a8, int32_t isPet, int32_t usePreview);
    int32_t SetQuest(int32_t questId);
    void SetGlyph(int32_t slotId, int32_t glyphId, int32_t enabled, int32_t hideRemove, int32_t parseUnit);
    void SetTalent(const TalentRec* talent, CGTalentGroup_C* group, int32_t isInspect, int32_t useRank, int32_t rank, int32_t isPet, int32_t usePreview);
    int32_t SetAchievement(int32_t achievementId, WOWGUID guid, bool completed, WowTime time, uint32_t criteria1, uint32_t criteria2, uint32_t criteria3, uint32_t criteria4, int32_t a10);
    int32_t SetSpell(int32_t spellId, int32_t brief, uint32_t cooldown, int32_t isPet, int32_t showRank, int32_t isInspect, int32_t a8, const TalentRec* talent, CGTalentGroup_C* talentGroup, int32_t append, int32_t usePreview, int32_t a13, int32_t rank, int32_t maxRank, int32_t showLearn);
    int32_t SetItem(int32_t itemId, const WOWGUID* requester, const WOWGUID* itemGuid, int32_t nameOnly, int32_t compare, int32_t useLinkInfo, int32_t append, WOWGUID owner, int32_t socketPreview, FILE* stream, int32_t hideCharges, const ItemLinkInfo* link, int32_t minimumLevel, int32_t skipLock, int32_t skipSellPrice);
};

class CGItemStatsSummary {
    public:
    static int32_t GetItemComparison(CGTooltip* first, CGTooltip* second, CGItemStatsSummary* result);

    /* 0000 */ float m_dps;
    /* 0004 */ int32_t m_stats[73];
    /* 0128 */ uint32_t m_flags;

    CGItemStatsSummary();
    void Create();
    int32_t Create(CGItem_C* item, int32_t level);
    int32_t Create(const ItemStats_C* stats, const ItemLinkInfo* link, int32_t level);
    int32_t CreateFromTooltip(CGTooltip* tooltip);
    int32_t Fill(const char* link);
    CGItemStatsSummary operator-(const CGItemStatsSummary& other) const;
    void AccumulateStats(const ItemStats_C* stats);
    void AddStatMods(const ItemStats_C* stats);
    void AddResistances(const ItemStats_C* stats);
    void AddSpellEffects(const SpellRec* spell);
    void AddEnchantEffect(int32_t type, int32_t arg, int32_t amount);
    void AddEnchantment(const SpellItemEnchantmentRec* enchant);
    void AddEnchantment(const SpellItemEnchantmentRec* enchant, int32_t amount);
    void AccumulateDpsStat(const ItemStats_C* stats);
    void ScanEnchantSpells(const ItemStats_C* stats);
    void GetSocketFlags(const ItemStats_C* stats);
    void FillOutScalingItem(const ItemStats_C* stats, int32_t level);
    void FillOutRandomProperties(int32_t randomPropertiesId, uint32_t seed);
    void MakeStatNamesPretty(int32_t stat, const int32_t* group, uint32_t count, int32_t merge);
    void CleanStatNames();
};

class CSimpleUI {
    public:
    /* 0000 */ uint8_t unused_0000[720];
    /* 02D0 */ uint32_t unused_02D0;
    /* 02D4 */ uint8_t unused_02D4[84];
    /* 0328 */ uint32_t unused_0328;
    /* 032C */ uint32_t unused_032C;
    /* 0330 */ uint8_t unused_0330[40];
    /* 0358 */ uint32_t unused_0358;
    /* 035C */ uint32_t unused_035C;
    /* 0360 */ uint8_t unused_0360[396];
    /* 04EC */ uint32_t unused_04EC;
};

extern uint32_t s_tooltipTitleColor;
extern uint32_t s_tooltipTextColor;

int32_t SetCompareItem(CGTooltip* tooltip, uint32_t itemId, int32_t offset, int32_t shift, CGTooltip* compare);
int32_t SetTooltipUnitAura(CGTooltip* tooltip, ScriptAuraInfo* info);

extern const char* s_socketColorNames[5];
extern const char* s_itemModTokens[49];

#endif
