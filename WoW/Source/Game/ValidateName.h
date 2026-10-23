#ifndef WOW_SOURCE_GAME_VALIDATENAME_H
#define WOW_SOURCE_GAME_VALIDATENAME_H

#include <cstdint>
#include "WoW/Source/Game/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/pcre/pcreposix.h"

extern TSFixedArray<regex_t> g_chatSpamTokens;
extern TSFixedArray<regex_t> g_profanityTokens[9];
extern TSFixedArray<regex_t> g_chatProfanityTokens[9];
extern TSFixedArray<regex_t> g_reservedNameTokens[9];

bool IsAlphaChinese(uint16_t c);
bool IsAlphaEnglish(uint16_t c);
bool IsAlphaInCharSet(uint16_t c, CHARSET charset);
bool IsAlphaKorean(uint16_t c);
bool IsAlphaLatin1(uint16_t c);
bool IsAlphaRussian(uint16_t c);
VALIDATE_NAME_RESULT ValidateCharacterNameConst(uint32_t language, const char* name, bool profanity, bool chatProfanity, bool reserved, bool english, int32_t extraLength);
VALIDATE_NAME_RESULT ValidateName(uint32_t language, const uint16_t* allowed, const char* name, uint32_t* length, CHARSET* charset, bool profanity, bool chatProfanity, bool reserved, bool relaxed, bool english, int32_t charsetMask);
int32_t NameNeedsDeclensions(int32_t locale, const char* name);
VALIDATE_NAME_RESULT ValidateGuildName(uint32_t language, const char* name);
VALIDATE_NAME_RESULT ValidateGuildBankTabName(uint32_t language, const char* name);
VALIDATE_NAME_RESULT ValidatePetName(uint32_t language, const char* name, bool profanity, bool chatProfanity, bool reserved, bool english, uint32_t extraLength);
int32_t RussianNameNeedsDeclensions(const char* name);
bool IsRussian(uint16_t c);
void ValidateNameDestroy();
void ValidateNameInitialize(int32_t localeMask, int32_t charsetMask);

bool ValidateStringIsUTF8(const char* string);
bool TruncateNewLines(char* text);

bool IsNumeric(uint16_t c);
bool IsUpper(uint16_t c);
uint16_t ToUpper(uint16_t c);
uint16_t ToLower(uint16_t c);
void CaseAccentInsensitiveParse(char* string, uint32_t size, int32_t locale);
bool StripHyperlinks(const char* text, char* buffer, uint32_t size);

bool ScrubString(const char* source, char* dest, int32_t maxChars, uint32_t destSize, int32_t flags);
bool FixCharacterNameCase(char* dest, const char* source, uint32_t size);

int32_t GetNumRussianNameDeclensionSets(const char* name, int32_t sex);
int32_t GetRussianNameDeclensions(char* declensions, const char* name, int32_t sex, uint32_t set);
uint32_t GetNumNameDeclensionSets(int32_t locale, const char* name, int32_t sex);
int32_t GetNameDeclensions(int32_t locale, char* declensions, const char* name, int32_t sex, uint32_t set);
VALIDATE_NAME_RESULT ValidateCharacterNameDeclension(int32_t locale, const char* name, const char* declension);
bool IsRussianDeclensionValid(const char* name, const char* declension);

#endif
