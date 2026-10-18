#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/Game/NameGen.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "ThirdParty/pcre/pcre.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <set>

namespace {
void Require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
size_t allocations;
void* Allocate(size_t size) { void* p = std::malloc(size); if (p) ++allocations; return p; }
void Free(void* p) { if (p) --allocations; std::free(p); }
template<class T> void Load(T& db) { if (!db.m_loaded) db.Load(__FILE__, __LINE__); Require(db.m_loaded, "Character name DBC failed to load"); }
}
void CheckCharacterNames() {
    Load(g_namesProfanityDB); Load(g_namesReservedDB); Load(g_chatProfanityDB); Load(g_spamMessagesDB); Load(g_nameGenDB);
    struct Cleanup { ~Cleanup() { ValidateNameDestroy(); CVar::Destroy(); } } cleanup;
    auto* savedAllocate = pcre_malloc; auto* savedFree = pcre_free;
    struct Allocator { void* (*allocate)(size_t); void (*free)(void*); ~Allocator() { pcre_malloc = allocate; pcre_free = free; } } allocator{savedAllocate, savedFree};
    pcre_malloc = Allocate; pcre_free = Free;
    for (int cycle = 0; cycle < 2; ++cycle) {
        ValidateNameInitialize((1 << 0) | (1 << 4) | (1 << 8), 0);
        Require(allocations > 0, "Original name filters did not compile");
        auto validate = [](uint32_t language, const char* name) { return ValidateCharacterNameConst(language, name, false, false, false, false, 0); };
        Require(validate(0, "Arlen") == 0 && validate(4, u8"晨风") == 0 && validate(8, u8"Борис") == 0, "Original character name alphabets changed");
        Require(validate(0, "") == 2 && validate(0, "A") == 3 && validate(0, "Abcdefghijklm") == 4 && validate(0, "A1") == 5 && validate(0, "Aaa") == 11, "Original character name rejection codes changed");
        Require(validate(4, u8"晨风A") == 5 && validate(0, "\xC0\xAF") == 5, "Original mixed alphabet/invalid character rejection changed");
        bool reservedChecked = false;
        for (int i = 0; i < g_namesReservedDB.m_numRecords && !reservedChecked; ++i) {
            const auto& record = g_namesReservedDB.m_records[i];
            if (record.m_language != 0 && record.m_language != -1) continue;
            const char* pattern = record.m_name;
            if (*pattern == '^') ++pattern;
            std::string candidate;
            while ((*pattern >= 'A' && *pattern <= 'Z') || (*pattern >= 'a' && *pattern <= 'z')) candidate += *pattern++;
            if (*pattern == '$') ++pattern;
            if (*pattern || candidate.size() < 2 || candidate.size() > 12 || validate(0, candidate.c_str())) continue;
            Require(ValidateCharacterNameConst(0, candidate.c_str(), false, false, true, false, 0) == 8, "Original reserved-name filter was bypassed");
            reservedChecked = true;
        }
        Require(reservedChecked, "No real literal reserved-name rule was exercised");
        char declensions[5][1024]{};
        const char* expected[5] = {u8"Бориса", u8"Борису", u8"Бориса", u8"Борисом", u8"Борисе"};
        Require(NameNeedsDeclensions(8, u8"Борис") && GetNumNameDeclensionSets(8, u8"Борис", 0) &&
            GetNameDeclensions(8, declensions[0], u8"Борис", 0, 0), "Original Russian name declension path failed");
        for (unsigned form = 0; form < 5; ++form)
            Require(std::strcmp(declensions[form], expected[form]) == 0 && ValidateCharacterNameDeclension(8, u8"Борис", declensions[form]) == 0,
                "Original Russian character name form changed");
        std::set<std::string> generated;
        CNameGen generator;
        generator.LoadNames(1, 0);
        Require(generator.m_names.Count() && generator.m_dictionaryCount, "Original human name dictionary is empty");
        for (unsigned i = 0; i < 64; ++i) {
            char name[64]; generator.GenerateName(name, sizeof(name));
            Require(name[0] && std::strlen(name) < sizeof(name) && ValidateStringIsUTF8(name), "Original name generator produced invalid text");
            generated.insert(name);
        }
        Require(generated.size() > 8, "Original name generator did not vary its names");
        const size_t count = allocations;
        ValidateNameInitialize((1 << 0) | (1 << 4) | (1 << 8), 0);
        Require(allocations == count, "Unchanged name policy rebuilt filters");
        ValidateNameInitialize(1 << 4, 1 << 4);
        Require(validate(4, u8"晨风") == 0 && validate(4, "Arlen") == 5 && !g_profanityTokens[0].Count(), "Original locale/charset policy replacement failed");
        ValidateNameDestroy();
        std::fprintf(stderr, "Original name policy cycle %d: %zu compiled allocations, %zu remain after destroy, %zu generated names\n", cycle, count, allocations, generated.size());
        Require(allocations == 0, "Original name policy retained compiled filters after destruction");
    }
}
