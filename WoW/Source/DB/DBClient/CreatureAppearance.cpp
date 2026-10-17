#include "WoW/Source/DB/WowClientDB.h"
#include "WoW/Source/DB/Rec/CreatureDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/CreatureModelDataRec.h"
#include "WoW/Source/DB/Rec/ParticleColorRec.h"

// STATIC: 0x00AD34B8
WowClientDB<CreatureDisplayInfoRec> g_creatureDisplayInfoDB;

// STATIC: 0x00AD3500
WowClientDB<CreatureModelDataRec> g_creatureModelDataDB;

// STATIC: 0x00AD4358
WowClientDB<ParticleColorRec> g_particleColorDB;
