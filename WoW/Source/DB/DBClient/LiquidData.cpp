#include "WoW/Source/DB/WowClientDB.h"
#include "WoW/Source/DB/Rec/LiquidTypeRec.h"
#include "WoW/Source/DB/Rec/LiquidMaterialRec.h"

// STATIC: 0x00AD4064
WowClientDB<LiquidTypeRec> g_liquidTypeDB;

// STATIC: 0x00AD4088
WowClientDB<LiquidMaterialRec> g_liquidMaterialDB;
