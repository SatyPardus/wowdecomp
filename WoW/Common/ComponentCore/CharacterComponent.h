#ifndef WOW_COMMON_COMPONENTCORE_CHARACTERCOMPONENT_H
#define WOW_COMMON_COMPONENTCORE_CHARACTERCOMPONENT_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/BLPFile/Types.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Services/Types.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2iVector.h"
#include "WoW/Common/ComponentCore/Types.h"
#include "WoW/Common/TextureCache.h"
#include "WoW/Source/Glue/ComponentData.h"

class CCharacterComponent;
class CComponentMipBits;
class CComponentRequest;
class CM2Model;
class ChrRacesRec;
class CreatureDisplayInfoRec;
class CreatureModelDataRec;
class CTexture;
struct CVar;
class ItemDisplayInfoRec;
struct st_race;

struct CharacterBaseVariation {
    /* 0000 */ HTEXTURECACHE m_texture [3];

    ~CharacterBaseVariation();
};

struct CharacterSection {
    /* 0000 */ HTEXTURECACHE m_layerTex [7];
    /* 001C */ uint32_t m_layerItemDisplayId [7];
    /* 0038 */ uint32_t m_layerMask;

    CharacterSection();
    ~CharacterSection();
};

struct CompSectionInfo {
    /* 0000 */ C2iVector m_pos;
    /* 0008 */ C2iVector m_size;
};

class CComponentMipBits {
    public:
    /* 0000 */ MipBits* m_mipBits;
    /* 0004 */ PIXEL_FORMAT m_format;
    /* 0008 */ TSLink<CComponentMipBits> m_link;
};

class CComponentRequest {
    public:
    /* 0000 */ uint32_t m_flags;
    /* 0004 */ EGxTexFormat m_format;
    /* 0008 */ CComponentMipBits* m_mipBits;
    /* 000C */ HTEXTURECACHE m_baseTextures [10];
    /* 0034 */ uint32_t m_layerCount;
    /* 0038 */ HTEXTURECACHE m_layerTextures [40];
    /* 00D8 */ uint8_t m_layerSections [40];
    /* 0100 */ TSLink<CComponentRequest> m_link;

    void AddLayer(uint8_t section, HTEXTURECACHE texture);
    void DestroyCachedTextures();
};

class CStatus;
class CCharacterComponent {
    public:
    static CVar* g_componentCompressVar;
    static CVar* g_componentTextureLevelVar;
    static CVar* g_componentThreadVar;
    static CCharacterComponent* m_activePlayerComponent;
    static void (CCharacterComponent::*m_itemFunc[NUM_COMPONENT_SECTIONS])(ITEM_SLOT, const ItemDisplayInfoRec*, bool);
    static void (CCharacterComponent::*m_prepFunc[NUM_COMPONENT_SECTIONS])();
    static int32_t s_bComponentCompression;
    static int32_t s_bComponentThread;
    static int32_t s_bInRenderPrep;
    static uint32_t* s_characterFacialHairStylesList;
    static st_race* s_chrVarArray;
    static uint32_t s_chrVarArrayLength;
    static EGxTexFormat s_gxFormat;
    static EGxTexFormat s_gxFormatHigh;
    static uint32_t s_mipLevels;
    static TSExplicitList<CCharacterComponent, 0> s_queue;
    static void (CCharacterComponent::*s_requestFunc[NUM_COMPONENT_SECTIONS])();
    static CompSectionInfo s_sectionInfoRaw[NUM_COMPONENT_SECTIONS];
    static MipBits* s_textureBuffer;

    static GEOCOMPONENTLINKS AddHandItem(CM2Model* model, const ItemDisplayInfoRec* displayRec, INVENTORY_SLOTS invSlot, SHEATHE_TYPE sheatheType, bool sheathed, bool shield, bool mainHand, int32_t itemVisual);
    static void AddLink(CM2Model* parent, GEOCOMPONENTLINKS link, const char* modelPath, const char* texturePath, int32_t itemVisual, const ItemDisplayInfoRec* displayRec);
    static CCharacterComponent* AllocComponent();
    static CComponentMipBits* AllocComponentMipBits(EGxTexFormat format);
    static CComponentRequest* AllocComponentRequest();
    static void ApplyMonsterGeosets(CM2Model* model, const CreatureDisplayInfoRec* displayRec);
    static void ComponentCloseFingers(CM2Model* model, COMP_HAND_SLOT hand);
    static void ComponentOpenFingers(CM2Model* model, COMP_HAND_SLOT hand);
    static void ComponentUtilAddItemVisual(CM2Model* model, int32_t itemVisual);
    static void CreateComponentFreeLists();
    static bool Destroy();
    static void DestroyComponentFreeLists();
    static void FreeComponent(CCharacterComponent* component);
    static INVENTORY_SLOTS GetInvSlotForItemSlot(ITEM_SLOT itemSlot);
    static ITEM_SLOT GetItemSlotForInvSlot(INVENTORY_SLOTS invSlot);
    static ITEM_SLOT GetItemSlotForInvType(INVENTORY_TYPES invType);
    static int32_t GetNthFacialFeatureIndex(int32_t raceID, int32_t sexID, int32_t classID, int32_t hairColorID, int32_t facialHairID, int32_t index, COMPONENT_SELECTION selection);
    static int32_t GetNumFacesForSkin(int32_t raceID, int32_t sexID, int32_t classID, int32_t skinID, COMPONENT_CONTEXT context);
    static int32_t GetNumFacialFeaturesForHairColor(int32_t raceID, int32_t sexID, int32_t classID, int32_t hairColorID, COMPONENT_CONTEXT context);
    static int32_t GetNumHairColorsForStyle(int32_t raceID, int32_t sexID, int32_t classID, int32_t hairStyleID, COMPONENT_CONTEXT context);
    static int32_t GetNumHairStylesForColor(int32_t raceID, int32_t sexID, int32_t classID, int32_t hairColorID, COMPONENT_CONTEXT context);
    static int32_t GetNumSkins(int32_t raceID, int32_t sexID, int32_t classID, COMPONENT_CONTEXT context);
    static GEOCOMPONENTLINKS GetSheatheLink(SHEATHE_TYPE sheatheType, bool mainHand);
    static char* GetTabardBackgroundFileName(COMPONENT_SECTIONS section, int32_t background, char* buffer, int32_t size);
    static char* GetTabardBorderFileName(COMPONENT_SECTIONS section, int32_t border, int32_t color, char* buffer, int32_t size);
    static char* GetTabardEmblemFileName(COMPONENT_SECTIONS section, int32_t emblem, int32_t color, char* buffer, int32_t size);
    static bool Initialize();
    static bool Initialize(EGxTexFormat format, uint32_t mipLevels, int32_t useThread, int32_t useCompression);
    static CM2Model* MakeAmmoModel(CM2Model* model, const ItemDisplayInfoRec* displayRec, uint32_t invType);
    static void Paste(HTEXTURECACHE texture, MipBits* dstMips, const C2iVector& dstPos, const C2iVector& srcPos, const C2iVector& srcSize, TCTEXTUREINFO& srcInfo, uint32_t srcMipLevel);
    static void PasteCrappyGreen(MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcSize, TCTEXTUREINFO& srcInfo, uint32_t srcMipLevel, int32_t dstMipOffset);
    static void PasteOpaque(HTEXTURECACHE texture, const BlpPalPixel* pal, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo, uint32_t srcMipLevel, int32_t dstMipOffset);
    static void PasteOpaqueScale(HTEXTURECACHE texture, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo);
    static void PasteFromSkin(COMPONENT_SECTIONS section, HTEXTURECACHE texture, MipBits* dstMips);
    static void PasteScale(HTEXTURECACHE texture, MipBits* dstMips, const C2iVector& dstPos, const C2iVector& srcPos, const C2iVector& srcSize, TCTEXTUREINFO& srcInfo);
    static void PasteToSection(COMPONENT_SECTIONS section, HTEXTURECACHE texture, MipBits* dstMips);
    static void PasteTransparent1Bit(HTEXTURECACHE texture, const BlpPalPixel* pal, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo, uint32_t srcMipLevel, int32_t dstMipOffset);
    static void PasteTransparent1BitScale(HTEXTURECACHE texture, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo);
    static void PasteTransparent4Bit(HTEXTURECACHE texture, const BlpPalPixel* pal, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo, uint32_t srcMipLevel, int32_t dstMipOffset);
    static void PasteTransparent4BitScale(HTEXTURECACHE texture, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo);
    static void PasteTransparent8Bit(HTEXTURECACHE texture, const BlpPalPixel* pal, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo, uint32_t srcMipLevel, int32_t dstMipOffset);
    static void PasteTransparent8BitScale(HTEXTURECACHE texture, MipBits* dstMips, C2iVector& dstPos, uint32_t dstStride, C2iVector& srcPos, C2iVector& srcSize, TCTEXTUREINFO& srcInfo);
    static void ProcessComponentFinishedList(int32_t all);
    static void ProcessComponentRequest(CComponentRequest* request);
    static void RemoveHandItem(CM2Model* model, INVENTORY_SLOTS invSlot, SHEATHE_TYPE sheatheType, bool shield);
    static void RemoveLinkpt(CM2Model* model, GEOCOMPONENTLINKS link);
    static void RemoveVisuals(CM2Model* model);
    static void ReplaceMonsterSkin(CM2Model* model, const CreatureDisplayInfoRec* displayRec, const CreatureModelDataRec* modelRec);
    static void ThreadDestroy();
    static int32_t ThreadInitialize();
    static uint32_t STORMAPI ThreadProc(void* param);
    static int32_t Update(const void* data, void* param);
    static void UpdateBaseTexture(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t face, uint32_t level, void* userArg, uint32_t& texelStrideInBytes, const void*& texels);
    static void ValidateComponentData(ComponentData* data, COMPONENT_CONTEXT context);

    /* 0000 */ TSLink<CCharacterComponent> m_link;
    /* 0008 */ uint32_t m_flags;
    /* 000C */ uint32_t m_dirtySections;
    /* 0010 */ uint32_t m_heapIndex;
    /* 0014 */ EGxTexFormat m_gxTexFormat;
    /* 0018 */ ComponentData m_data;
    /* 0190 */ CTexture* m_baseSkinTexture;
    /* 0194 */ CharacterBaseVariation m_baseVariation [CHARTEXTUREVARIATIONS_NUM];
    /* 01D0 */ CharacterSection m_section [10];
    /* 0428 */ uint32_t m_itemDisplayID [12];
    /* 0458 */ uint32_t m_handItemDisplayID [3];
    /* 0464 */ int32_t m_itemSlotForAttachSlot [50];
    /* 052C */ CComponentRequest* m_request;

    CCharacterComponent();
    ~CCharacterComponent();
    void AddCape();
    void AddHelm(int32_t itemVisual);
    void AddItem(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, int32_t itemVisual);
    void AddItem(ITEM_SLOT itemSlot, int32_t displayID, int32_t itemVisual);
    void AddItemBySlot(INVENTORY_SLOTS invSlot, int32_t displayID, int32_t itemVisual);
    void AddItemByType(INVENTORY_TYPES invType, int32_t displayID);
    void AddQuiver(int32_t itemVisual);
    void AddShoulders(int32_t itemVisual);
    void ApplyGuildColor(int32_t emblem, int32_t emblemColor, int32_t border, int32_t borderColor, int32_t background);
    int32_t AreShoulderModelsCorrect();
    int32_t BuildShoulderItemPaths(ItemDisplayInfoRec& rec, const ItemDisplayInfoRec*& displayRec, char* leftModel, char* rightModel, char* leftTexture, char* rightTexture);
    void ClearGuildTexture(int32_t count);
    int32_t ComposeHelmModelFilePath(const ChrRacesRec* raceRec, const ItemDisplayInfoRec* displayRec, char* buffer, uint32_t size);
    void ComputeLoadProgress(int64_t& local, int64_t& total);
    void CreateBaseTexture();
    void CreateComponentRequest();
    void CreateRequestAL();
    void CreateRequestAU();
    void CreateRequestFO();
    void CreateRequestHA();
    void CreateRequestHL();
    void CreateRequestHU();
    void CreateRequestLL();
    void CreateRequestLU();
    void CreateRequestTL();
    void CreateRequestTU();
    void DestroyRenderTextures();
    void FreeSectionTexture(COMPONENT_SECTIONS section, int32_t layer);
    void GeosRenderPrep();
    int32_t GetHelmModelFilePath(ItemDisplayInfoRec& rec, const ItemDisplayInfoRec*& displayRec);
    void GetPreferences(CharacterPreferences* preferences);
    int32_t GetQuiverModelFilePath(ItemDisplayInfoRec& rec, const ItemDisplayInfoRec*& displayRec);
    CharSectionsRec* GetSectionsRecord(CHARTEXTUREVARIATIONS variation, int32_t variationIndex, int32_t colorIndex, bool* found);
    bool Init(ComponentData* data, const char* debug);
    bool IsHelmModelCorrect();
    bool IsQuiverModelCorrect();
    bool ItemsLoaded(int32_t wait);
    void LoadBaseVariation(CHARTEXTUREVARIATIONS variation, int32_t textureIndex, int32_t variationIndex, int32_t colorIndex, COMPONENT_SECTIONS section, const char* debug);
    HTEXTURECACHE LoadItemComponentTexture(const ItemDisplayInfoRec* displayRec, COMPONENT_SECTIONS section);
    void Prep();
    void PrepSections();
    void ProcessFinishedRequest();
    void RemoveItem(ITEM_SLOT itemSlot);
    void RemoveItemBySlot(INVENTORY_SLOTS invSlot);
    void RemoveItemByType(INVENTORY_TYPES invType);
    bool RenderPrep(int32_t wait);
    void RenderPrepAL();
    void RenderPrepAll();
    void RenderPrepAU();
    void RenderPrepFO();
    void RenderPrepHA();
    void RenderPrepHL();
    void RenderPrepHU();
    void RenderPrepLL();
    void RenderPrepLU();
    void RenderPrepSections();
    void RenderPrepTL();
    void RenderPrepTU();
    void ReplaceExtraSkinTexture(const char* debug);
    void ReplaceHairTexture(int32_t hairStyleID, const char* debug);
    void SetBeardStyle(int32_t facialHairID, bool loadVariation, const char* debug);
    void SetDirtySections(uint32_t mask);
    void SetFace(int32_t faceID, bool loadHair, const char* debug);
    void SetHairColor(int32_t hairColorID, bool loadHair, const char* debug);
    void SetHairStyle(int32_t hairStyleID, const char* debug);
    void SetHandItemDisplay(int32_t displayID, uint32_t hand, INVENTORY_SLOTS invSlot, bool shield);
    void SetItemSlotUsingAttachSlot(ITEM_SLOT itemSlot, GEOCOMPONENTLINKS link);
    void SetMainHandItemDisplay(int32_t displayID);
    void SetOffHandItemDisplay(int32_t displayID);
    void SetShieldItemDisplay(int32_t displayID);
    bool SetNextFace(COMPONENT_CONTEXT context, int32_t colorOffset);
    bool SetNextFacialFeature(COMPONENT_CONTEXT context);
    bool SetNextHairColor(COMPONENT_CONTEXT context);
    bool SetNextHairStyle(COMPONENT_CONTEXT context);
    bool SetNextSkin(COMPONENT_CONTEXT context);
    bool SetPrevFace(COMPONENT_CONTEXT context, int32_t colorOffset);
    bool SetPrevFacialFeature(COMPONENT_CONTEXT context);
    bool SetPrevHairColor(COMPONENT_CONTEXT context);
    bool SetPrevHairStyle(COMPONENT_CONTEXT context);
    bool SetPrevSkin(COMPONENT_CONTEXT context);
    bool SetRandomFace(COMPONENT_CONTEXT context);
    bool SetRandomFacialFeature(COMPONENT_CONTEXT context);
    bool SetRandomHairColor(COMPONENT_CONTEXT context);
    bool SetRandomHairStyle(COMPONENT_CONTEXT context);
    bool SetRandomSkin(COMPONENT_CONTEXT context);
    void SetSkinColor(int32_t skinID, bool loadHair, bool renderAll, const char* debug);
    void SetTexturesDirty();
    bool SkinNpc(const char* debug);
    void TurnOffGuildTabardPurchase();
    void UpdateItem(ITEM_SLOT itemSlot, COMPONENT_SECTIONS section, const ItemDisplayInfoRec* displayRec, bool apply);
    void UpdateItemAL(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply);
    void UpdateItemAU(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply);
    void UpdateItemFO(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply);
    void UpdateItemHA(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply);
    void UpdateItemHL(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply);
    void UpdateItemHU(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply);
    void UpdateItemLL(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply);
    void UpdateItemLU(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply);
    void UpdateItemTL(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply);
    void UpdateItemTU(ITEM_SLOT itemSlot, const ItemDisplayInfoRec* displayRec, bool apply);
    void UpdateSections();
    int32_t UpdateTextureSlot(COMPONENT_SECTIONS section, const ItemDisplayInfoRec* displayRec, int32_t layer);
    bool VariationsLoaded(int32_t wait);
};

bool ComponentVarHandler(CVar* var, const char* oldValue, const char* value, void* arg);

CTexture* CreateTexture(const char* fileName, CStatus* status);

#endif
