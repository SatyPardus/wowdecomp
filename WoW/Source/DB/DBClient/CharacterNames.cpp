#include "WoW/Source/DB/WowClientDB.h"
#include "WoW/Source/DB/Rec/ChatProfanityRec.h"
#include "WoW/Source/DB/Rec/NameGenRec.h"
#include "WoW/Source/DB/Rec/NamesProfanityRec.h"
#include "WoW/Source/DB/Rec/NamesReservedRec.h"
#include "WoW/Source/DB/Rec/SpamMessagesRec.h"

// STATIC: 0x00AD33E0
WowClientDB<ChatProfanityRec> g_chatProfanityDB;

// STATIC: 0x00AD4238
WowClientDB<NameGenRec> g_nameGenDB;

// STATIC: 0x00AD4280
WowClientDB<NamesProfanityRec> g_namesProfanityDB;

// STATIC: 0x00AD42A4
WowClientDB<NamesReservedRec> g_namesReservedDB;

// STATIC: 0x00AD4724
WowClientDB<SpamMessagesRec> g_spamMessagesDB;
