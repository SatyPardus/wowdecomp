#include "WoW/Source/DB/WowClientDB.h"
#include "WoW/Source/DB/Rec/CharHairGeosetsRec.h"
#include "WoW/Source/DB/Rec/CharSectionsRec.h"
#include "WoW/Source/DB/Rec/CharacterFacialHairStylesRec.h"
#include "WoW/Source/DB/Rec/ChrRacesRec.h"
#include "WoW/Source/DB/Rec/HelmetGeosetVisDataRec.h"
#include "WoW/Source/DB/Rec/ItemDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/ItemVisualEffectsRec.h"
#include "WoW/Source/DB/Rec/ItemVisualsRec.h"

// STATIC: 0x00AD3308
WowClientDB<CharHairGeosetsRec> g_charHairGeosetsDB;

// STATIC: 0x00AD332C
WowClientDB<CharSectionsRec> g_charSectionsDB;

// STATIC: 0x00AD3398
WowClientDB<CharacterFacialHairStylesRec> g_characterFacialHairStylesDB;

// STATIC: 0x00AD3428
WowClientDB<ChrRacesRec> g_chrRacesDB;

// STATIC: 0x00AD3CBC
WowClientDB<HelmetGeosetVisDataRec> g_helmetGeosetVisDataDB;

// STATIC: 0x00AD3DDC
WowClientDB_RLE<ItemDisplayInfoRec> g_itemDisplayInfoDB;

// STATIC: 0x00AD3F68
WowClientDB<ItemVisualEffectsRec> g_itemVisualEffectsDB;

// STATIC: 0x00AD3F8C
WowClientDB<ItemVisualsRec> g_itemVisualsDB;
