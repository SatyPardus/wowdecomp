#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/MapDifficultyRec.h"
#include "WoW/Source/Object/ItemStats.h"
#include <map>

// STATIC: 0x00AD3028
const char* g_localeNames[NUM_DB_LOCALES] = { "deDE", "enGB", "enUS", "esES", "frFR", "koKR", "zhCN", "zhTW", "enCN", "enTW", "esMX", "ruRU" };

// STATIC: 0x00C5DEA0
int32_t g_clientDbPerformCompress;

// STATIC: 0x00AD2F90
static int32_t s_physicalDamageClassID = -1;

// STATIC: 0x00C5D660
static const ItemSubClassRec* s_unarmedWeapon;

// STATIC: 0x00C5D664
static const ItemClassRec* s_weaponItemClass;

// STATIC: 0x00C5D668
static TSFixedArray<const ResistancesRec*> s_resistances;

// STATIC: 0x00C5D674
static TSFixedArray<uint32_t> s_terrainTypeSoundIDs;

// STATIC: 0x00C5D680
static TSFixedArray<const ItemSubClassRec*> s_weaponSubclasses;

// STATIC: 0x00AD305C
WowClientDB<AchievementRec> g_achievementDB;

// STATIC: 0x00AD3080
WowClientDB<Achievement_CriteriaRec> g_achievement_CriteriaDB;

// STATIC: 0x00AD30A4
WowClientDB<Achievement_CategoryRec> g_achievement_CategoryDB;

// STATIC: 0x00AD30C8
WowClientDB<AnimationDataRec> g_animationDataDB;

// STATIC: 0x00AD30EC
WowClientDB<AreaGroupRec> g_areaGroupDB;

// STATIC: 0x00AD3110
WowClientDB<AreaPOIRec> g_areaPOIDB;

// STATIC: 0x00AD3134
WowClientDB<AreaTableRec> g_areaTableDB;

// STATIC: 0x00AD3158
WowClientDB<AreaTriggerRec> g_areaTriggerDB;

// STATIC: 0x00AD317C
WowClientDB<AttackAnimKitsRec> g_attackAnimKitsDB;

// STATIC: 0x00AD31A0
WowClientDB<AttackAnimTypesRec> g_attackAnimTypesDB;

// STATIC: 0x00AD31C4
WowClientDB<AuctionHouseRec> g_auctionHouseDB;

// STATIC: 0x00AD31E8
WowClientDB<BankBagSlotPricesRec> g_bankBagSlotPricesDB;

// STATIC: 0x00AD320C
WowClientDB<BannedAddOnsRec> g_bannedAddOnsDB;

// STATIC: 0x00AD3230
WowClientDB<BarberShopStyleRec> g_barberShopStyleDB;

// STATIC: 0x00AD3254
WowClientDB<BattlemasterListRec> g_battlemasterListDB;

// STATIC: 0x00AD3278
WowClientDB<CameraShakesRec> g_cameraShakesDB;

// STATIC: 0x00AD329C
WowClientDB<Cfg_CategoriesRec> g_cfg_CategoriesDB;

// STATIC: 0x00AD32C0
WowClientDB<Cfg_ConfigsRec> g_cfg_ConfigsDB;

// STATIC: 0x00AD32E4
WowClientDB<CharBaseInfoRec> g_charBaseInfoDB;

// STATIC: 0x00AD3308
WowClientDB<CharHairGeosetsRec> g_charHairGeosetsDB;

// STATIC: 0x00AD332C
WowClientDB<CharSectionsRec> g_charSectionsDB;

// STATIC: 0x00AD3350
WowClientDB<CharStartOutfitRec> g_charStartOutfitDB;

// STATIC: 0x00AD3374
WowClientDB<CharTitlesRec> g_charTitlesDB;

// STATIC: 0x00AD3398
WowClientDB<CharacterFacialHairStylesRec> g_characterFacialHairStylesDB;

// STATIC: 0x00AD33BC
WowClientDB<ChatChannelsRec> g_chatChannelsDB;

// STATIC: 0x00AD33E0
WowClientDB<ChatProfanityRec> g_chatProfanityDB;

// STATIC: 0x00AD3404
WowClientDB<ChrClassesRec> g_chrClassesDB;

// STATIC: 0x00AD3428
WowClientDB<ChrRacesRec> g_chrRacesDB;

// STATIC: 0x00AD344C
WowClientDB<CinematicCameraRec> g_cinematicCameraDB;

// STATIC: 0x00AD3470
WowClientDB<CinematicSequencesRec> g_cinematicSequencesDB;

// STATIC: 0x00AD3494
WowClientDB<CreatureDisplayInfoExtraRec> g_creatureDisplayInfoExtraDB;

// STATIC: 0x00AD34B8
WowClientDB<CreatureDisplayInfoRec> g_creatureDisplayInfoDB;

// STATIC: 0x00AD34DC
WowClientDB<CreatureFamilyRec> g_creatureFamilyDB;

// STATIC: 0x00AD3500
WowClientDB<CreatureModelDataRec> g_creatureModelDataDB;

// STATIC: 0x00AD3524
WowClientDB<CreatureMovementInfoRec> g_creatureMovementInfoDB;

// STATIC: 0x00AD3548
WowClientDB<CreatureSoundDataRec> g_creatureSoundDataDB;

// STATIC: 0x00AD356C
WowClientDB<CreatureSpellDataRec> g_creatureSpellDataDB;

// STATIC: 0x00AD3590
WowClientDB<CreatureTypeRec> g_creatureTypeDB;

// STATIC: 0x00AD35B4
WowClientDB<CurrencyTypesRec> g_currencyTypesDB;

// STATIC: 0x00AD35D8
WowClientDB<CurrencyCategoryRec> g_currencyCategoryDB;

// STATIC: 0x00AD35FC
WowClientDB<DanceMovesRec> g_danceMovesDB;

// STATIC: 0x00AD3620
WowClientDB<DeathThudLookupsRec> g_deathThudLookupsDB;

// STATIC: 0x00AD3644
WowClientDB<DeclinedWordRec> g_declinedWordDB;

// STATIC: 0x00AD3668
WowClientDB<DeclinedWordCasesRec> g_declinedWordCasesDB;

// STATIC: 0x00AD368C
WowClientDB<DestructibleModelDataRec> g_destructibleModelDataDB;

// STATIC: 0x00AD36B0
WowClientDB<DungeonEncounterRec> g_dungeonEncounterDB;

// STATIC: 0x00AD36D4
WowClientDB<DungeonMapRec> g_dungeonMapDB;

// STATIC: 0x00AD36F8
WowClientDB<DungeonMapChunkRec> g_dungeonMapChunkDB;

// STATIC: 0x00AD371C
WowClientDB<DurabilityCostsRec> g_durabilityCostsDB;

// STATIC: 0x00AD3740
WowClientDB<DurabilityQualityRec> g_durabilityQualityDB;

// STATIC: 0x00AD3764
WowClientDB<EmotesRec> g_emotesDB;

// STATIC: 0x00AD3788
WowClientDB<EmotesTextDataRec> g_emotesTextDataDB;

// STATIC: 0x00AD37AC
WowClientDB<EmotesTextSoundRec> g_emotesTextSoundDB;

// STATIC: 0x00AD37D0
WowClientDB<EmotesTextRec> g_emotesTextDB;

// STATIC: 0x00AD37F4
WowClientDB<EnvironmentalDamageRec> g_environmentalDamageDB;

// STATIC: 0x00AD3818
WowClientDB<ExhaustionRec> g_exhaustionDB;

// STATIC: 0x00AD383C
WowClientDB<FactionGroupRec> g_factionGroupDB;

// STATIC: 0x00AD3860
WowClientDB<FactionRec> g_factionDB;

// STATIC: 0x00AD3884
WowClientDB<FactionTemplateRec> g_factionTemplateDB;

// STATIC: 0x00AD38A8
WowClientDB<FileDataRec> g_fileDataDB;

// STATIC: 0x00AD38CC
WowClientDB<FootprintTexturesRec> g_footprintTexturesDB;

// STATIC: 0x00AD38F0
WowClientDB<FootstepTerrainLookupRec> g_footstepTerrainLookupDB;

// STATIC: 0x00AD3914
WowClientDB<GameObjectArtKitRec> g_gameObjectArtKitDB;

// STATIC: 0x00AD3938
WowClientDB<GameObjectDisplayInfoRec> g_gameObjectDisplayInfoDB;

// STATIC: 0x00AD395C
WowClientDB<GameTablesRec> g_gameTablesDB;

// STATIC: 0x00AD3980
WowClientDB<GameTipsRec> g_gameTipsDB;

// STATIC: 0x00AD39A4
WowClientDB<GemPropertiesRec> g_gemPropertiesDB;

// STATIC: 0x00AD39C8
WowClientDB<GlyphPropertiesRec> g_glyphPropertiesDB;

// STATIC: 0x00AD39EC
WowClientDB<GlyphSlotRec> g_glyphSlotDB;

// STATIC: 0x00AD3A10
WowClientDB<GMSurveyAnswersRec> g_gmSurveyAnswersDB;

// STATIC: 0x00AD3A34
WowClientDB<GMSurveyCurrentSurveyRec> g_gmSurveyCurrentSurveyDB;

// STATIC: 0x00AD3A58
WowClientDB<GMSurveyQuestionsRec> g_gmSurveyQuestionsDB;

// STATIC: 0x00AD3A7C
WowClientDB<GMSurveySurveysRec> g_gmSurveySurveysDB;

// STATIC: 0x00AD3AA0
WowClientDB<GMTicketCategoryRec> g_gmTicketCategoryDB;

// STATIC: 0x00AD3AC4
WowClientDB<GroundEffectDoodadRec> g_groundEffectDoodadDB;

// STATIC: 0x00AD3AE8
WowClientDB<GroundEffectTextureRec> g_groundEffectTextureDB;

// STATIC: 0x00AD3B0C
WowClientDB<gtBarberShopCostBaseRec> g_gtBarberShopCostBaseDB;

// STATIC: 0x00AD3B30
WowClientDB<gtCombatRatingsRec> g_gtCombatRatingsDB;

// STATIC: 0x00AD3B54
WowClientDB<gtChanceToMeleeCritRec> g_gtChanceToMeleeCritDB;

// STATIC: 0x00AD3B78
WowClientDB<gtChanceToMeleeCritBaseRec> g_gtChanceToMeleeCritBaseDB;

// STATIC: 0x00AD3B9C
WowClientDB<gtChanceToSpellCritRec> g_gtChanceToSpellCritDB;

// STATIC: 0x00AD3BC0
WowClientDB<gtChanceToSpellCritBaseRec> g_gtChanceToSpellCritBaseDB;

// STATIC: 0x00AD3BE4
WowClientDB<gtNPCManaCostScalerRec> g_gtNPCManaCostScalerDB;

// STATIC: 0x00AD3C08
WowClientDB<gtOCTClassCombatRatingScalarRec> g_gtOCTClassCombatRatingScalarDB;

// STATIC: 0x00AD3C2C
WowClientDB<gtOCTRegenHPRec> g_gtOCTRegenHPDB;

// STATIC: 0x00AD3C50
WowClientDB<gtOCTRegenMPRec> g_gtOCTRegenMPDB;

// STATIC: 0x00AD3C74
WowClientDB<gtRegenHPPerSptRec> g_gtRegenHPPerSptDB;

// STATIC: 0x00AD3C98
WowClientDB<gtRegenMPPerSptRec> g_gtRegenMPPerSptDB;

// STATIC: 0x00AD3CBC
WowClientDB<HelmetGeosetVisDataRec> g_helmetGeosetVisDataDB;

// STATIC: 0x00AD3CE0
WowClientDB<HolidayDescriptionsRec> g_holidayDescriptionsDB;

// STATIC: 0x00AD3D04
WowClientDB<HolidayNamesRec> g_holidayNamesDB;

// STATIC: 0x00AD3D28
WowClientDB<HolidaysRec> g_holidaysDB;

#ifdef DISABLE_ITEM_CACHE
static std::map<int32_t, ItemRec> s_itemDBOverlay;

static void ItemDBOverlayCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
}
#endif

template <>
ItemRec* WowClientDB<ItemRec>::GetRecord(int32_t id) const {
#ifdef DISABLE_ITEM_CACHE
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(id, none, ItemDBOverlayCallback, nullptr, true);

    if (!stats) {
        return nullptr;
    }

    ItemRec& rec = s_itemDBOverlay[id];
    rec.m_id = id;
    rec.m_classID = stats->m_class;
    rec.m_subclassID = stats->m_subClass;
    rec.m_soundOverrideSubclassID = stats->m_soundOverrideSubclass;
    rec.m_material = stats->m_material;
    rec.m_displayInfoID = stats->m_displayId;
    rec.m_inventoryType = stats->m_inventoryType;
    rec.m_sheatheType = stats->m_sheatheType;
    return &rec;
#else
    if (id < this->m_minID || id > this->m_maxID) {
        return nullptr;
    }

    return this->m_recordsById[id - this->m_minID];
#endif
}

void ClearItemDBOverlay() {
#ifdef DISABLE_ITEM_CACHE
    s_itemDBOverlay.clear();
#endif
}

// STATIC: 0x00AD3D4C
WowClientDB<ItemRec> g_itemDB;

// STATIC: 0x00AD3D70
WowClientDB<ItemBagFamilyRec> g_itemBagFamilyDB;

// STATIC: 0x00AD3D94
WowClientDB<ItemClassRec> g_itemClassDB;

// STATIC: 0x00AD3DB8
WowClientDB<ItemCondExtCostsRec> g_itemCondExtCostsDB;

// STATIC: 0x00AD3DDC
WowClientDB_RLE<ItemDisplayInfoRec> g_itemDisplayInfoDB;

// STATIC: 0x00AD3E00
WowClientDB<ItemExtendedCostRec> g_itemExtendedCostDB;

// STATIC: 0x00AD3E24
WowClientDB<ItemGroupSoundsRec> g_itemGroupSoundsDB;

// STATIC: 0x00AD3E48
WowClientDB<ItemLimitCategoryRec> g_itemLimitCategoryDB;

// STATIC: 0x00AD3E6C
WowClientDB<ItemPetFoodRec> g_itemPetFoodDB;

// STATIC: 0x00AD3E90
WowClientDB<ItemPurchaseGroupRec> g_itemPurchaseGroupDB;

// STATIC: 0x00AD3EB4
WowClientDB<ItemRandomPropertiesRec> g_itemRandomPropertiesDB;

// STATIC: 0x00AD3ED8
WowClientDB<ItemRandomSuffixRec> g_itemRandomSuffixDB;

// STATIC: 0x00AD3EFC
WowClientDB<ItemSetRec> g_itemSetDB;

// STATIC: 0x00AD3F20
WowClientDB<ItemSubClassMaskRec> g_itemSubClassMaskDB;

// STATIC: 0x00AD3F44
WowClientDB<ItemSubClassRec> g_itemSubClassDB;

// STATIC: 0x00AD3F68
WowClientDB<ItemVisualEffectsRec> g_itemVisualEffectsDB;

// STATIC: 0x00AD3F8C
WowClientDB<ItemVisualsRec> g_itemVisualsDB;

// STATIC: 0x00AD3FB0
WowClientDB<LanguageWordsRec> g_languageWordsDB;

// STATIC: 0x00AD3FD4
WowClientDB<LanguagesRec> g_languagesDB;

// STATIC: 0x00AD3FF8
WowClientDB<LfgDungeonExpansionRec> g_lfgDungeonExpansionDB;

// STATIC: 0x00AD401C
WowClientDB<LfgDungeonGroupRec> g_lfgDungeonGroupDB;

// STATIC: 0x00AD4040
WowClientDB<LfgDungeonsRec> g_lfgDungeonsDB;

// STATIC: 0x00AD4064
WowClientDB<LiquidTypeRec> g_liquidTypeDB;

// STATIC: 0x00AD4088
WowClientDB<LiquidMaterialRec> g_liquidMaterialDB;

// STATIC: 0x00AD40AC
WowClientDB<LoadingScreensRec> g_loadingScreensDB;

// STATIC: 0x00AD40D0
WowClientDB<LoadingScreenTaxiSplinesRec> g_loadingScreenTaxiSplinesDB;

// STATIC: 0x00AD40F4
WowClientDB<LockRec> g_lockDB;

// STATIC: 0x00AD4118
WowClientDB<LockTypeRec> g_lockTypeDB;

// STATIC: 0x00AD413C
WowClientDB<MailTemplateRec> g_mailTemplateDB;

// STATIC: 0x00AD4160
WowClientDB<MapRec> g_mapDB;

// STATIC: 0x00AD4184
WowClientDB<MapDifficultyRec> g_mapDifficultyDB;

// STATIC: 0x00AD41A8
WowClientDB<MaterialRec> g_materialDB;

// STATIC: 0x00AD41CC
WowClientDB<MovieRec> g_movieDB;

// STATIC: 0x00AD41F0
WowClientDB<MovieFileDataRec> g_movieFileDataDB;

// STATIC: 0x00AD4214
WowClientDB<MovieVariationRec> g_movieVariationDB;

// STATIC: 0x00AD4238
WowClientDB<NameGenRec> g_nameGenDB;

// STATIC: 0x00AD425C
WowClientDB<NPCSoundsRec> g_npcSoundsDB;

// STATIC: 0x00AD4280
WowClientDB<NamesProfanityRec> g_namesProfanityDB;

// STATIC: 0x00AD42A4
WowClientDB<NamesReservedRec> g_namesReservedDB;

// STATIC: 0x00AD42C8
WowClientDB<OverrideSpellDataRec> g_overrideSpellDataDB;

// STATIC: 0x00AD42EC
WowClientDB<PackageRec> g_packageDB;

// STATIC: 0x00AD4310
WowClientDB<PageTextMaterialRec> g_pageTextMaterialDB;

// STATIC: 0x00AD4334
WowClientDB<PaperDollItemFrameRec> g_paperDollItemFrameDB;

// STATIC: 0x00AD4358
WowClientDB<ParticleColorRec> g_particleColorDB;

// STATIC: 0x00AD437C
WowClientDB<PetPersonalityRec> g_petPersonalityDB;

// STATIC: 0x00AD43A0
WowClientDB<PowerDisplayRec> g_powerDisplayDB;

// STATIC: 0x00AD43C4
WowClientDB<PvpDifficultyRec> g_pvpDifficultyDB;

// STATIC: 0x00AD43E8
WowClientDB<QuestFactionRewardRec> g_questFactionRewardDB;

// STATIC: 0x00AD440C
WowClientDB<QuestInfoRec> g_questInfoDB;

// STATIC: 0x00AD4430
WowClientDB<QuestSortRec> g_questSortDB;

// STATIC: 0x00AD4454
WowClientDB<QuestXPRec> g_questXPDB;

// STATIC: 0x00AD4478
WowClientDB<ResistancesRec> g_resistancesDB;

// STATIC: 0x00AD449C
WowClientDB<RandPropPointsRec> g_randPropPointsDB;

// STATIC: 0x00AD44C0
WowClientDB<ScalingStatDistributionRec> g_scalingStatDistributionDB;

// STATIC: 0x00AD44E4
WowClientDB<ScalingStatValuesRec> g_scalingStatValuesDB;

// STATIC: 0x00AD4508
WowClientDB<ScreenEffectRec> g_screenEffectDB;

// STATIC: 0x00AD452C
WowClientDB<ServerMessagesRec> g_serverMessagesDB;

// STATIC: 0x00AD4550
WowClientDB<SheatheSoundLookupsRec> g_sheatheSoundLookupsDB;

// STATIC: 0x00AD4574
WowClientDB<SkillCostsDataRec> g_skillCostsDataDB;

// STATIC: 0x00AD4598
WowClientDB<SkillLineAbilityRec> g_skillLineAbilityDB;

// STATIC: 0x00AD45BC
WowClientDB<SkillLineCategoryRec> g_skillLineCategoryDB;

// STATIC: 0x00AD45E0
WowClientDB<SkillLineRec> g_skillLineDB;

// STATIC: 0x00AD4604
WowClientDB<SkillRaceClassInfoRec> g_skillRaceClassInfoDB;

// STATIC: 0x00AD4628
WowClientDB<SkillTiersRec> g_skillTiersDB;

// STATIC: 0x00AD464C
WowClientDB<SoundAmbienceRec> g_soundAmbienceDB;

// STATIC: 0x00AD4670
WowClientDB<SoundEntriesRec> g_soundEntriesDB;

// STATIC: 0x00AD4694
WowClientDB<SoundEmittersRec> g_soundEmittersDB;

// STATIC: 0x00AD46B8
WowClientDB<SoundProviderPreferencesRec> g_soundProviderPreferencesDB;

// STATIC: 0x00AD46DC
WowClientDB<SoundSamplePreferencesRec> g_soundSamplePreferencesDB;

// STATIC: 0x00AD4700
WowClientDB<SoundWaterTypeRec> g_soundWaterTypeDB;

// STATIC: 0x00AD4724
WowClientDB<SpamMessagesRec> g_spamMessagesDB;

// STATIC: 0x00AD4748
WowClientDB<SpellCastTimesRec> g_spellCastTimesDB;

// STATIC: 0x00AD476C
WowClientDB<SpellCategoryRec> g_spellCategoryDB;

// STATIC: 0x00AD4790
WowClientDB<SpellChainEffectsRec> g_spellChainEffectsDB;

// STATIC: 0x00AD47B4
WowClientDB<SpellDescriptionVariablesRec> g_spellDescriptionVariablesDB;

// STATIC: 0x00AD47D8
WowClientDB<SpellDifficultyRec> g_spellDifficultyDB;

// STATIC: 0x00AD47FC
WowClientDB<SpellDispelTypeRec> g_spellDispelTypeDB;

// STATIC: 0x00AD4820
WowClientDB<SpellDurationRec> g_spellDurationDB;

// STATIC: 0x00AD4844
WowClientDB<SpellEffectCameraShakesRec> g_spellEffectCameraShakesDB;

// STATIC: 0x00AD4868
WowClientDB<SpellFocusObjectRec> g_spellFocusObjectDB;

// STATIC: 0x00AD488C
WowClientDB<SpellIconRec> g_spellIconDB;

// STATIC: 0x00AD48B0
WowClientDB<SpellItemEnchantmentRec> g_spellItemEnchantmentDB;

// STATIC: 0x00AD48D4
WowClientDB<SpellItemEnchantmentConditionRec> g_spellItemEnchantmentConditionDB;

// STATIC: 0x00AD48F8
WowClientDB<SpellMechanicRec> g_spellMechanicDB;

// STATIC: 0x00AD491C
WowClientDB<SpellMissileRec> g_spellMissileDB;

// STATIC: 0x00AD4940
WowClientDB<SpellMissileMotionRec> g_spellMissileMotionDB;

// STATIC: 0x00AD4964
WowClientDB<SpellRadiusRec> g_spellRadiusDB;

// STATIC: 0x00AD4988
WowClientDB<SpellRangeRec> g_spellRangeDB;

// STATIC: 0x00AD49AC
WowClientDB<SpellRuneCostRec> g_spellRuneCostDB;

// STATIC: 0x00AD49D0
WowClientDB_RLE<SpellRec> g_spellDB;

// STATIC: 0x00AD49F4
WowClientDB<SpellShapeshiftFormRec> g_spellShapeshiftFormDB;

// STATIC: 0x00AD4A18
WowClientDB<SpellVisualEffectNameRec> g_spellVisualEffectNameDB;

// STATIC: 0x00AD4A3C
WowClientDB<SpellVisualKitRec> g_spellVisualKitDB;

// STATIC: 0x00AD4A60
WowClientDB<SpellVisualKitAreaModelRec> g_spellVisualKitAreaModelDB;

// STATIC: 0x00AD4A84
WowClientDB<SpellVisualKitModelAttachRec> g_spellVisualKitModelAttachDB;

// STATIC: 0x00AD4AA8
WowClientDB<SpellVisualRec> g_spellVisualDB;

// STATIC: 0x00AD4ACC
WowClientDB<StableSlotPricesRec> g_stableSlotPricesDB;

// STATIC: 0x00AD4AF0
WowClientDB<StationeryRec> g_stationeryDB;

// STATIC: 0x00AD4B14
WowClientDB<StringLookupsRec> g_stringLookupsDB;

// STATIC: 0x00AD4B38
WowClientDB<SummonPropertiesRec> g_summonPropertiesDB;

// STATIC: 0x00AD4B5C
WowClientDB<TalentRec> g_talentDB;

// STATIC: 0x00AD4B80
WowClientDB<TalentTabRec> g_talentTabDB;

// STATIC: 0x00AD4BA4
WowClientDB<TaxiNodesRec> g_taxiNodesDB;

// STATIC: 0x00AD4BC8
WowClientDB<TaxiPathNodeRec> g_taxiPathNodeDB;

// STATIC: 0x00AD4BEC
WowClientDB<TaxiPathRec> g_taxiPathDB;

// STATIC: 0x00AD4C10
WowClientDB<TeamContributionPointsRec> g_teamContributionPointsDB;

// STATIC: 0x00AD4C34
WowClientDB<TerrainTypeRec> g_terrainTypeDB;

// STATIC: 0x00AD4C58
WowClientDB<TerrainTypeSoundsRec> g_terrainTypeSoundsDB;

// STATIC: 0x00AD4C7C
WowClientDB<TotemCategoryRec> g_totemCategoryDB;

// STATIC: 0x00AD4CA0
WowClientDB<TransportAnimationRec> g_transportAnimationDB;

// STATIC: 0x00AD4CC4
WowClientDB<TransportPhysicsRec> g_transportPhysicsDB;

// STATIC: 0x00AD4CE8
WowClientDB<TransportRotationRec> g_transportRotationDB;

// STATIC: 0x00AD4D0C
WowClientDB<UISoundLookupsRec> g_uiSoundLookupsDB;

// STATIC: 0x00AD4D30
WowClientDB<UnitBloodLevelsRec> g_unitBloodLevelsDB;

// STATIC: 0x00AD4D54
WowClientDB<UnitBloodRec> g_unitBloodDB;

// STATIC: 0x00AD4D78
WowClientDB<VehicleRec> g_vehicleDB;

// STATIC: 0x00AD4D9C
WowClientDB<VehicleSeatRec> g_vehicleSeatDB;

// STATIC: 0x00AD4DC0
WowClientDB<VehicleUIIndicatorRec> g_vehicleUIIndicatorDB;

// STATIC: 0x00AD4DE4
WowClientDB<VehicleUIIndSeatRec> g_vehicleUIIndSeatDB;

// STATIC: 0x00AD4E08
WowClientDB<VocalUISoundsRec> g_vocalUISoundsDB;

// STATIC: 0x00AD4E2C
WowClientDB<WMOAreaTableRec> g_wmoAreaTableDB;

// STATIC: 0x00AD4E50
WowClientDB<WeaponImpactSoundsRec> g_weaponImpactSoundsDB;

// STATIC: 0x00AD4E74
WowClientDB<WeaponSwingSounds2Rec> g_weaponSwingSounds2DB;

// STATIC: 0x00AD4E98
WowClientDB<WeatherRec> g_weatherDB;

// STATIC: 0x00AD4EBC
WowClientDB<WorldMapAreaRec> g_worldMapAreaDB;

// STATIC: 0x00AD4EE0
WowClientDB<WorldMapContinentRec> g_worldMapContinentDB;

// STATIC: 0x00AD4F04
WowClientDB<WorldMapOverlayRec> g_worldMapOverlayDB;

// STATIC: 0x00AD4F28
WowClientDB<WorldMapTransformsRec> g_worldMapTransformsDB;

// STATIC: 0x00AD4F4C
WowClientDB<WorldSafeLocsRec> g_worldSafeLocsDB;

// STATIC: 0x00AD4F70
WowClientDB<WorldStateUIRec> g_worldStateUIDB;

// STATIC: 0x00AD4F94
WowClientDB<ZoneIntroMusicTableRec> g_zoneIntroMusicTableDB;

// STATIC: 0x00AD4FB8
WowClientDB<ZoneMusicRec> g_zoneMusicDB;

// STATIC: 0x00AD4FDC
WowClientDB<WorldStateZoneSoundsRec> g_worldStateZoneSoundsDB;

// STATIC: 0x00AD5000
WowClientDB<WorldChunkSoundsRec> g_worldChunkSoundsDB;

// STATIC: 0x00AD5024
WowClientDB<SoundEntriesAdvancedRec> g_soundEntriesAdvancedDB;

// STATIC: 0x00AD5048
WowClientDB<ObjectEffectRec> g_objectEffectDB;

// STATIC: 0x00AD506C
WowClientDB<ObjectEffectGroupRec> g_objectEffectGroupDB;

// STATIC: 0x00AD5090
WowClientDB<ObjectEffectModifierRec> g_objectEffectModifierDB;

// STATIC: 0x00AD50B4
WowClientDB<ObjectEffectPackageRec> g_objectEffectPackageDB;

// STATIC: 0x00AD50D8
WowClientDB<ObjectEffectPackageElemRec> g_objectEffectPackageElemDB;

// STATIC: 0x00AD50FC
WowClientDB<SoundFilterRec> g_soundFilterDB;

// STATIC: 0x00AD5120
WowClientDB<SoundFilterElemRec> g_soundFilterElemDB;

// STATIC: 0x00AF4998
WowClientDB<LightSkyboxRec> g_lightSkyboxDB;

// STATIC: 0x00AF49BC
WowClientDB_RLE<LightIntBandRec> g_lightIntBandDB;

// STATIC: 0x00AF49E0
WowClientDB_RLE<LightFloatBandRec> g_lightFloatBandDB;

// STATIC: 0x00AF4A04
WowClientDB<LightParamsRec> g_lightParamsDB;

// STATIC: 0x00AF4A28
WowClientDB<LightRec> g_lightDB;

// VERIFY: da36ac34 manual
// OFFSET: 0x006337D0
static void StaticDBLoadAll(void (WowClientDB_Base::*loadFunction)(const char*, int32_t)) {
    (g_achievementDB.*loadFunction)(__FILE__, __LINE__);
    (g_achievement_CriteriaDB.*loadFunction)(__FILE__, __LINE__);
    (g_achievement_CategoryDB.*loadFunction)(__FILE__, __LINE__);
    (g_animationDataDB.*loadFunction)(__FILE__, __LINE__);
    (g_areaGroupDB.*loadFunction)(__FILE__, __LINE__);
    (g_areaPOIDB.*loadFunction)(__FILE__, __LINE__);
    (g_areaTableDB.*loadFunction)(__FILE__, __LINE__);
    (g_areaTriggerDB.*loadFunction)(__FILE__, __LINE__);
    (g_attackAnimKitsDB.*loadFunction)(__FILE__, __LINE__);
    (g_attackAnimTypesDB.*loadFunction)(__FILE__, __LINE__);
    (g_auctionHouseDB.*loadFunction)(__FILE__, __LINE__);
    (g_bankBagSlotPricesDB.*loadFunction)(__FILE__, __LINE__);
    (g_bannedAddOnsDB.*loadFunction)(__FILE__, __LINE__);
    (g_barberShopStyleDB.*loadFunction)(__FILE__, __LINE__);
    (g_battlemasterListDB.*loadFunction)(__FILE__, __LINE__);
    (g_cameraShakesDB.*loadFunction)(__FILE__, __LINE__);
    (g_cfg_CategoriesDB.*loadFunction)(__FILE__, __LINE__);
    (g_cfg_ConfigsDB.*loadFunction)(__FILE__, __LINE__);
    (g_charBaseInfoDB.*loadFunction)(__FILE__, __LINE__);
    (g_charHairGeosetsDB.*loadFunction)(__FILE__, __LINE__);
    (g_charSectionsDB.*loadFunction)(__FILE__, __LINE__);
    (g_charStartOutfitDB.*loadFunction)(__FILE__, __LINE__);
    (g_charTitlesDB.*loadFunction)(__FILE__, __LINE__);
    (g_characterFacialHairStylesDB.*loadFunction)(__FILE__, __LINE__);
    (g_chatChannelsDB.*loadFunction)(__FILE__, __LINE__);
    (g_chatProfanityDB.*loadFunction)(__FILE__, __LINE__);
    (g_chrClassesDB.*loadFunction)(__FILE__, __LINE__);
    (g_chrRacesDB.*loadFunction)(__FILE__, __LINE__);
    (g_cinematicCameraDB.*loadFunction)(__FILE__, __LINE__);
    (g_cinematicSequencesDB.*loadFunction)(__FILE__, __LINE__);
    (g_creatureDisplayInfoDB.*loadFunction)(__FILE__, __LINE__);
    (g_creatureDisplayInfoExtraDB.*loadFunction)(__FILE__, __LINE__);
    (g_creatureFamilyDB.*loadFunction)(__FILE__, __LINE__);
    (g_creatureModelDataDB.*loadFunction)(__FILE__, __LINE__);
    (g_creatureMovementInfoDB.*loadFunction)(__FILE__, __LINE__);
    (g_creatureSoundDataDB.*loadFunction)(__FILE__, __LINE__);
    (g_creatureSpellDataDB.*loadFunction)(__FILE__, __LINE__);
    (g_creatureTypeDB.*loadFunction)(__FILE__, __LINE__);
    (g_currencyTypesDB.*loadFunction)(__FILE__, __LINE__);
    (g_currencyCategoryDB.*loadFunction)(__FILE__, __LINE__);
    (g_danceMovesDB.*loadFunction)(__FILE__, __LINE__);
    (g_deathThudLookupsDB.*loadFunction)(__FILE__, __LINE__);
    (g_destructibleModelDataDB.*loadFunction)(__FILE__, __LINE__);
    (g_dungeonEncounterDB.*loadFunction)(__FILE__, __LINE__);
    (g_dungeonMapDB.*loadFunction)(__FILE__, __LINE__);
    (g_dungeonMapChunkDB.*loadFunction)(__FILE__, __LINE__);
    (g_durabilityCostsDB.*loadFunction)(__FILE__, __LINE__);
    (g_durabilityQualityDB.*loadFunction)(__FILE__, __LINE__);
    (g_emotesDB.*loadFunction)(__FILE__, __LINE__);
    (g_emotesTextDB.*loadFunction)(__FILE__, __LINE__);
    (g_emotesTextDataDB.*loadFunction)(__FILE__, __LINE__);
    (g_emotesTextSoundDB.*loadFunction)(__FILE__, __LINE__);
    (g_environmentalDamageDB.*loadFunction)(__FILE__, __LINE__);
    (g_exhaustionDB.*loadFunction)(__FILE__, __LINE__);
    (g_factionDB.*loadFunction)(__FILE__, __LINE__);
    (g_factionGroupDB.*loadFunction)(__FILE__, __LINE__);
    (g_factionTemplateDB.*loadFunction)(__FILE__, __LINE__);
    (g_fileDataDB.*loadFunction)(__FILE__, __LINE__);
    (g_footprintTexturesDB.*loadFunction)(__FILE__, __LINE__);
    (g_footstepTerrainLookupDB.*loadFunction)(__FILE__, __LINE__);
    (g_gameObjectArtKitDB.*loadFunction)(__FILE__, __LINE__);
    (g_gameObjectDisplayInfoDB.*loadFunction)(__FILE__, __LINE__);
    (g_gameTablesDB.*loadFunction)(__FILE__, __LINE__);
    (g_gameTipsDB.*loadFunction)(__FILE__, __LINE__);
    (g_gemPropertiesDB.*loadFunction)(__FILE__, __LINE__);
    (g_glyphPropertiesDB.*loadFunction)(__FILE__, __LINE__);
    (g_glyphSlotDB.*loadFunction)(__FILE__, __LINE__);
    (g_gmSurveyAnswersDB.*loadFunction)(__FILE__, __LINE__);
    (g_gmSurveyCurrentSurveyDB.*loadFunction)(__FILE__, __LINE__);
    (g_gmSurveyQuestionsDB.*loadFunction)(__FILE__, __LINE__);
    (g_gmSurveySurveysDB.*loadFunction)(__FILE__, __LINE__);
    (g_gmTicketCategoryDB.*loadFunction)(__FILE__, __LINE__);
    (g_groundEffectDoodadDB.*loadFunction)(__FILE__, __LINE__);
    (g_groundEffectTextureDB.*loadFunction)(__FILE__, __LINE__);
    (g_gtBarberShopCostBaseDB.*loadFunction)(__FILE__, __LINE__);
    (g_gtCombatRatingsDB.*loadFunction)(__FILE__, __LINE__);
    (g_gtChanceToMeleeCritDB.*loadFunction)(__FILE__, __LINE__);
    (g_gtChanceToMeleeCritBaseDB.*loadFunction)(__FILE__, __LINE__);
    (g_gtChanceToSpellCritDB.*loadFunction)(__FILE__, __LINE__);
    (g_gtChanceToSpellCritBaseDB.*loadFunction)(__FILE__, __LINE__);
    (g_gtNPCManaCostScalerDB.*loadFunction)(__FILE__, __LINE__);
    (g_gtOCTClassCombatRatingScalarDB.*loadFunction)(__FILE__, __LINE__);
    (g_gtOCTRegenHPDB.*loadFunction)(__FILE__, __LINE__);
    (g_gtOCTRegenMPDB.*loadFunction)(__FILE__, __LINE__);
    (g_gtRegenHPPerSptDB.*loadFunction)(__FILE__, __LINE__);
    (g_gtRegenMPPerSptDB.*loadFunction)(__FILE__, __LINE__);
    (g_helmetGeosetVisDataDB.*loadFunction)(__FILE__, __LINE__);
    (g_holidayDescriptionsDB.*loadFunction)(__FILE__, __LINE__);
    (g_holidayNamesDB.*loadFunction)(__FILE__, __LINE__);
    (g_holidaysDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemBagFamilyDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemClassDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemCondExtCostsDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemDisplayInfoDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemExtendedCostDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemGroupSoundsDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemLimitCategoryDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemPetFoodDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemPurchaseGroupDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemRandomPropertiesDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemRandomSuffixDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemSetDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemSubClassDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemSubClassMaskDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemVisualEffectsDB.*loadFunction)(__FILE__, __LINE__);
    (g_itemVisualsDB.*loadFunction)(__FILE__, __LINE__);
    (g_languageWordsDB.*loadFunction)(__FILE__, __LINE__);
    (g_languagesDB.*loadFunction)(__FILE__, __LINE__);
    (g_lfgDungeonExpansionDB.*loadFunction)(__FILE__, __LINE__);
    (g_lfgDungeonGroupDB.*loadFunction)(__FILE__, __LINE__);
    (g_lfgDungeonsDB.*loadFunction)(__FILE__, __LINE__);
    (g_lightDB.*loadFunction)(__FILE__, __LINE__);
    (g_lightFloatBandDB.*loadFunction)(__FILE__, __LINE__);
    (g_lightIntBandDB.*loadFunction)(__FILE__, __LINE__);
    (g_lightParamsDB.*loadFunction)(__FILE__, __LINE__);
    (g_lightSkyboxDB.*loadFunction)(__FILE__, __LINE__);
    (g_liquidTypeDB.*loadFunction)(__FILE__, __LINE__);
    (g_liquidMaterialDB.*loadFunction)(__FILE__, __LINE__);
    (g_loadingScreensDB.*loadFunction)(__FILE__, __LINE__);
    (g_loadingScreenTaxiSplinesDB.*loadFunction)(__FILE__, __LINE__);
    (g_lockDB.*loadFunction)(__FILE__, __LINE__);
    (g_lockTypeDB.*loadFunction)(__FILE__, __LINE__);
    (g_mailTemplateDB.*loadFunction)(__FILE__, __LINE__);
    (g_mapDB.*loadFunction)(__FILE__, __LINE__);
    (g_mapDifficultyDB.*loadFunction)(__FILE__, __LINE__);
    (g_materialDB.*loadFunction)(__FILE__, __LINE__);
    (g_movieDB.*loadFunction)(__FILE__, __LINE__);
    (g_movieFileDataDB.*loadFunction)(__FILE__, __LINE__);
    (g_movieVariationDB.*loadFunction)(__FILE__, __LINE__);
    (g_nameGenDB.*loadFunction)(__FILE__, __LINE__);
    (g_npcSoundsDB.*loadFunction)(__FILE__, __LINE__);
    (g_namesProfanityDB.*loadFunction)(__FILE__, __LINE__);
    (g_namesReservedDB.*loadFunction)(__FILE__, __LINE__);
    (g_overrideSpellDataDB.*loadFunction)(__FILE__, __LINE__);
    (g_packageDB.*loadFunction)(__FILE__, __LINE__);
    (g_pageTextMaterialDB.*loadFunction)(__FILE__, __LINE__);
    (g_paperDollItemFrameDB.*loadFunction)(__FILE__, __LINE__);
    (g_particleColorDB.*loadFunction)(__FILE__, __LINE__);
    (g_petPersonalityDB.*loadFunction)(__FILE__, __LINE__);
    (g_powerDisplayDB.*loadFunction)(__FILE__, __LINE__);
    (g_pvpDifficultyDB.*loadFunction)(__FILE__, __LINE__);
    (g_questFactionRewardDB.*loadFunction)(__FILE__, __LINE__);
    (g_questInfoDB.*loadFunction)(__FILE__, __LINE__);
    (g_questSortDB.*loadFunction)(__FILE__, __LINE__);
    (g_questXPDB.*loadFunction)(__FILE__, __LINE__);
    (g_resistancesDB.*loadFunction)(__FILE__, __LINE__);
    (g_randPropPointsDB.*loadFunction)(__FILE__, __LINE__);
    (g_scalingStatDistributionDB.*loadFunction)(__FILE__, __LINE__);
    (g_scalingStatValuesDB.*loadFunction)(__FILE__, __LINE__);
    (g_screenEffectDB.*loadFunction)(__FILE__, __LINE__);
    (g_serverMessagesDB.*loadFunction)(__FILE__, __LINE__);
    (g_sheatheSoundLookupsDB.*loadFunction)(__FILE__, __LINE__);
    (g_skillCostsDataDB.*loadFunction)(__FILE__, __LINE__);
    (g_skillLineAbilityDB.*loadFunction)(__FILE__, __LINE__);
    (g_skillLineCategoryDB.*loadFunction)(__FILE__, __LINE__);
    (g_skillLineDB.*loadFunction)(__FILE__, __LINE__);
    (g_skillRaceClassInfoDB.*loadFunction)(__FILE__, __LINE__);
    (g_skillTiersDB.*loadFunction)(__FILE__, __LINE__);
    (g_soundAmbienceDB.*loadFunction)(__FILE__, __LINE__);
    (g_soundEmittersDB.*loadFunction)(__FILE__, __LINE__);
    (g_soundEntriesDB.*loadFunction)(__FILE__, __LINE__);
    (g_soundProviderPreferencesDB.*loadFunction)(__FILE__, __LINE__);
    (g_soundSamplePreferencesDB.*loadFunction)(__FILE__, __LINE__);
    (g_soundWaterTypeDB.*loadFunction)(__FILE__, __LINE__);
    (g_spamMessagesDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellCastTimesDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellCategoryDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellChainEffectsDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellDescriptionVariablesDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellDifficultyDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellDispelTypeDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellDurationDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellEffectCameraShakesDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellFocusObjectDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellIconDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellItemEnchantmentDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellItemEnchantmentConditionDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellMechanicDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellMissileDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellMissileMotionDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellRadiusDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellRangeDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellRuneCostDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellShapeshiftFormDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellVisualDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellVisualEffectNameDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellVisualKitDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellVisualKitAreaModelDB.*loadFunction)(__FILE__, __LINE__);
    (g_spellVisualKitModelAttachDB.*loadFunction)(__FILE__, __LINE__);
    (g_stableSlotPricesDB.*loadFunction)(__FILE__, __LINE__);
    (g_stationeryDB.*loadFunction)(__FILE__, __LINE__);
    (g_stringLookupsDB.*loadFunction)(__FILE__, __LINE__);
    (g_summonPropertiesDB.*loadFunction)(__FILE__, __LINE__);
    (g_talentDB.*loadFunction)(__FILE__, __LINE__);
    (g_talentTabDB.*loadFunction)(__FILE__, __LINE__);
    (g_taxiNodesDB.*loadFunction)(__FILE__, __LINE__);
    (g_taxiPathDB.*loadFunction)(__FILE__, __LINE__);
    (g_taxiPathNodeDB.*loadFunction)(__FILE__, __LINE__);
    (g_teamContributionPointsDB.*loadFunction)(__FILE__, __LINE__);
    (g_terrainTypeDB.*loadFunction)(__FILE__, __LINE__);
    (g_terrainTypeSoundsDB.*loadFunction)(__FILE__, __LINE__);
    (g_totemCategoryDB.*loadFunction)(__FILE__, __LINE__);
    (g_transportAnimationDB.*loadFunction)(__FILE__, __LINE__);
    (g_transportPhysicsDB.*loadFunction)(__FILE__, __LINE__);
    (g_transportRotationDB.*loadFunction)(__FILE__, __LINE__);
    (g_uiSoundLookupsDB.*loadFunction)(__FILE__, __LINE__);
    (g_unitBloodDB.*loadFunction)(__FILE__, __LINE__);
    (g_unitBloodLevelsDB.*loadFunction)(__FILE__, __LINE__);
    (g_vehicleDB.*loadFunction)(__FILE__, __LINE__);
    (g_vehicleSeatDB.*loadFunction)(__FILE__, __LINE__);
    (g_vehicleUIIndicatorDB.*loadFunction)(__FILE__, __LINE__);
    (g_vehicleUIIndSeatDB.*loadFunction)(__FILE__, __LINE__);
    (g_vocalUISoundsDB.*loadFunction)(__FILE__, __LINE__);
    (g_wmoAreaTableDB.*loadFunction)(__FILE__, __LINE__);
    (g_weaponImpactSoundsDB.*loadFunction)(__FILE__, __LINE__);
    (g_weaponSwingSounds2DB.*loadFunction)(__FILE__, __LINE__);
    (g_weatherDB.*loadFunction)(__FILE__, __LINE__);
    (g_worldMapAreaDB.*loadFunction)(__FILE__, __LINE__);
    (g_worldMapTransformsDB.*loadFunction)(__FILE__, __LINE__);
    (g_worldMapContinentDB.*loadFunction)(__FILE__, __LINE__);
    (g_worldMapOverlayDB.*loadFunction)(__FILE__, __LINE__);
    (g_worldSafeLocsDB.*loadFunction)(__FILE__, __LINE__);
    (g_worldStateUIDB.*loadFunction)(__FILE__, __LINE__);
    (g_zoneIntroMusicTableDB.*loadFunction)(__FILE__, __LINE__);
    (g_zoneMusicDB.*loadFunction)(__FILE__, __LINE__);
    (g_worldStateZoneSoundsDB.*loadFunction)(__FILE__, __LINE__);
    (g_worldChunkSoundsDB.*loadFunction)(__FILE__, __LINE__);
    (g_soundEntriesAdvancedDB.*loadFunction)(__FILE__, __LINE__);
    (g_objectEffectDB.*loadFunction)(__FILE__, __LINE__);
    (g_objectEffectGroupDB.*loadFunction)(__FILE__, __LINE__);
    (g_objectEffectModifierDB.*loadFunction)(__FILE__, __LINE__);
    (g_objectEffectPackageDB.*loadFunction)(__FILE__, __LINE__);
    (g_objectEffectPackageElemDB.*loadFunction)(__FILE__, __LINE__);
    (g_soundFilterDB.*loadFunction)(__FILE__, __LINE__);
    (g_soundFilterElemDB.*loadFunction)(__FILE__, __LINE__);
}

// VERIFY: 45af3496 auto
// OFFSET: 0x006337A0
int32_t GetPhysicalDamageClassID() {
    return s_physicalDamageClassID;
}

// VERIFY: 723e6c50 auto
// OFFSET: 0x006337B0
uint32_t GetPhysicalDamageClassMask() {
    return 1 << s_physicalDamageClassID;
}

// VERIFY: f122e16d auto
// OFFSET: 0x006337C0
int32_t ClientDBGetUnarmedWeapon() {
    return s_unarmedWeapon->m_subClassID;
}

// VERIFY: 88034703 manual
// OFFSET: 0x006348C0
static void LocateWeaponSubclass() {
    s_weaponItemClass = nullptr;

    for (int32_t i = g_itemClassDB.m_numRecords - 1; i >= 0; i--) {
        const ItemClassRec* rec = i >= 0 && i < g_itemClassDB.m_numRecords ? &g_itemClassDB.m_records[i] : nullptr;

        if (rec->m_flags & 1) {
            s_weaponItemClass = rec;
        }
    }
}

// VERIFY: d0f3ca01 auto
// OFFSET: 0x00634940
int32_t ClientDBGetNumWeaponSubClasses() {
    return s_weaponSubclasses.Count();
}

// VERIFY: 9c6cf3e8 auto
// OFFSET: 0x00634A80
int32_t GetMapResetTime(int32_t mapId, uint32_t difficulty) {
    MapDifficultyRec* entry = GetMapDifficultyEntryByMapIdAndDifficulty(mapId, difficulty, nullptr);

    if (entry) {
        return entry->m_raidDuration;
    }

    return 0;
}

// VERIFY: d0ca7548 manual
// OFFSET: 0x00634AE0
void InitResistances() {
    uint32_t count = g_resistancesDB.m_numRecords;
    s_physicalDamageClassID = -1;

    if (count != 7) {
        ConsolePrintf("Warning: The Resistances table has the wrong number of entries");

        if (count > 7) {
            count = 7;
        }
    }

    s_resistances.SetCount(count);

    for (int32_t i = 0; static_cast<uint32_t>(i) < count; i++) {
        const ResistancesRec* rec = i >= 0 && i < g_resistancesDB.m_numRecords ? &g_resistancesDB.m_records[i] : nullptr;

        if (rec->m_flags & 1) {
            s_physicalDamageClassID = i;
        }

        s_resistances[i] = rec;
    }
}

// VERIFY: 88c5149f manual
// OFFSET: 0x00634BA0
void InitTerrainTypeFlags() {
    uint32_t count = g_terrainTypeDB.m_maxID + 1;
    s_terrainTypeSoundIDs.SetCount(count);

    for (uint32_t i = 0; i < count; i++) {
        s_terrainTypeSoundIDs[i] = 0;

        const TerrainTypeRec* rec = g_terrainTypeDB.GetRecord(i);

        if (rec) {
            s_terrainTypeSoundIDs[i] = rec->m_soundID;
        }
    }
}

// VERIFY: ccd0205b manual
// OFFSET: 0x00634C60
void ClientDBShutdown() {
    s_resistances.Clear();
    s_terrainTypeSoundIDs.Clear();
}

// VERIFY: e620a1d6 manual
// OFFSET: 0x00634CC0
void InitWeaponSubclasses() {
    s_weaponSubclasses.SetCount(0);
    s_unarmedWeapon = nullptr;
    LocateWeaponSubclass();

    int32_t weaponClass = s_weaponItemClass->m_classID;
    int32_t count = 0;

    for (int32_t i = g_itemSubClassDB.m_numRecords - 1; i >= 0; i--) {
        const ItemSubClassRec* rec = i >= 0 && i < g_itemSubClassDB.m_numRecords ? &g_itemSubClassDB.m_records[i] : nullptr;

        if (rec->m_classID == weaponClass && count <= rec->m_subClassID + 1) {
            count = rec->m_subClassID + 1;
        }
    }

    s_weaponSubclasses.SetCount(count);

    for (int32_t i = g_itemSubClassDB.m_numRecords - 1; i >= 0; i--) {
        const ItemSubClassRec* rec = i >= 0 && i < g_itemSubClassDB.m_numRecords ? &g_itemSubClassDB.m_records[i] : nullptr;

        if (rec->m_classID == weaponClass) {
            s_weaponSubclasses[rec->m_subClassID] = rec;

            if (rec->m_flags & 4) {
                s_unarmedWeapon = rec;
            }
        }
    }
}

// VERIFY: f26ccc9b auto
// OFFSET: 0x00634E00
void ClientDBInitialize() {
    CVar* cvar = CVar::Lookup("dbCompress");

    if (cvar) {
        switch (cvar->m_intValue) {
            case -1:
            case 1:
                g_clientDbPerformCompress = 1;
                break;

            case 0:
                g_clientDbPerformCompress = 0;
                break;
        }
    }

    StaticDBLoadAll(&WowClientDB_Base::Load);
    GameTablesInitialize();
    InitResistances();
    InitTerrainTypeFlags();
    InitWeaponSubclasses();
    SDBItemSubclassInitialize();
    InitLights(1);
}

// VERIFY: 144a65d1 auto
// OFFSET: 0x00635D90
DB_LOCALE_INDEX LocaleNameToIndex(const char* locale) {
    for (int32_t i = 0; i < NUM_DB_LOCALES; i++) {
        if (!SStrCmpI(locale, g_localeNames[i], 4)) {
            return static_cast<DB_LOCALE_INDEX>(i);
        }
    }

    return DB_LOCALE_enUS;
}

// VERIFY: d0d3fe1b auto
// OFFSET: 0x00634910
const char* ClientDBStringLookup(STRINGLOOKUP lookup) {
    StringLookupsRec* rec = g_stringLookupsDB.GetRecord(lookup);
    return rec ? rec->m_string : nullptr;
}

// VERIFY: 3260ed78 manual
// OFFSET: 0x00634950
MapDifficultyRec* GetMapDifficultyEntryByMapIdAndDifficulty(int32_t mapId, uint32_t difficulty, int32_t* index) {
    for (int32_t i = 0; i < g_mapDifficultyDB.m_numRecords; i++) {
        MapDifficultyRec* rec = i < 0 || i >= g_mapDifficultyDB.m_numRecords ? nullptr : &g_mapDifficultyDB.m_records[i];

        if (rec->m_mapID != mapId) {
            continue;
        }

        for (int32_t j = i; j < g_mapDifficultyDB.m_numRecords; j++) {
            MapDifficultyRec* entry = j < 0 || j >= g_mapDifficultyDB.m_numRecords ? nullptr : &g_mapDifficultyDB.m_records[j];

            if (entry->m_mapID != mapId) {
                break;
            }

            if (entry->m_difficulty == static_cast<int32_t>(difficulty)) {
                if (index) {
                    *index = j;
                }

                return entry;
            }
        }
    }

    return nullptr;
}

// VERIFY: 126b0ac4 auto
// OFFSET: 0x00634AB0
int32_t GetMaxPlayersForMap(const MapRec* map, int32_t difficulty) {
    MapDifficultyRec* entry = GetMapDifficultyEntryByMapIdAndDifficulty(map->m_id, difficulty, nullptr);

    if (!entry || !entry->m_maxPlayers) {
        return map->m_maxPlayers;
    }

    return entry->m_maxPlayers;
}
