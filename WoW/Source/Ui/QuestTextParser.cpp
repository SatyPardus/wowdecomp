#include "QuestTextParser.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/SpellDescriptionVariablesRec.h"
#include "WoW/Source/DB/Rec/SpellItemEnchantmentRec.h"
#include "WoW/Source/DB/Rec/SpellRadiusRec.h"
#include "WoW/Source/DB/Rec/SpellRangeRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/VisibleItem_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/MathState.h"
#include "WoW/Source/Ui/PaperDollInfoFrame.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/DB/Rec/AchievementRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Game/GameTime.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Ui/CGWorldStateInfo.h"

// STATIC: 0x00BEB660
static int32_t s_lastNumber;

// STATIC: 0x00BEB668
TSGrowableArray<SpellVariable> s_spellVariables;

// STATIC: 0x00BEB678
static MathState s_mathState;

static int32_t SpellParserReplaceText(const SpellRec* spell, char* buffer, uint32_t size, int32_t parsePet, int32_t parseUnit, int32_t spellLevel, int32_t scale, int32_t noModifiers, const char** cursor);

// VERIFY: 1fdd2d49 manual
// OFFSET: 0x00576CD0
void GenderConditional(char* buffer, uint32_t size, int32_t gender, const CGUnit_C* unit, const NameCache* name, const char** cursor) {
    while (**cursor == ' ') {
        (*cursor)++;
    }

    if (!**cursor) {
        return;
    }

    const char* colon = SStrChr(*cursor, ':');

    if (!colon) {
        return;
    }

    const char* femaleText = colon + 1;
    const char* selector = SStrChr(colon + 1, ':');
    const char* semicolon = SStrChr(colon, ';');

    if (!semicolon) {
        return;
    }

    if (selector) {
        if (selector + 1 >= semicolon) {
            selector = nullptr;
        } else {
            switch (selector[1]) {
                case 'C':
                case 'c':
                    gender = CGUnit_C::GetDisplayClassGender(unit, name);
                    break;

                case 'R':
                case 'r':
                    gender = CGUnit_C::GetDisplayRaceGender(unit, name);
                    break;
            }
        }
    }

    const char* end = colon;

    if (gender) {
        *cursor = femaleText;

        while (**cursor == ' ') {
            (*cursor)++;
        }

        end = selector ? selector : semicolon;
    }

    int32_t length = end - *cursor;

    if (length) {
        uint32_t bufferLength = SStrLen(buffer);
        SStrPack(buffer, *cursor, size);

        if (bufferLength + length < size) {
            char* text = &buffer[bufferLength];
            int32_t i = length - 1;
            text[length] = '\0';

            if (text[i] == ' ') {
                do {
                    text[i] = '\0';

                    if (!i) {
                        break;
                    }

                    i--;
                } while (text[i] == ' ');
            }
        }
    }

    *cursor = semicolon;
}

// VERIFY: 5b10005b manual
// OFFSET: 0x00576EF0
static int32_t SpellParserPluralConditional(char* buffer, uint32_t size, int32_t number, const char** cursor) {
    while (**cursor == ' ') {
        (*cursor)++;
    }

    if (!**cursor) {
        return 1;
    }

    const char* semicolon = SStrChr(*cursor, ';');

    if (!semicolon) {
        return 1;
    }

    const char* text = *cursor;
    *cursor = semicolon + 1;
    uint32_t index = FrameScript_GetPluralIndex(number);

    for (uint32_t i = 0; i < index; i++) {
        while (*text != ':' && *text != ';') {
            text++;
        }

        if (*text == ';') {
            break;
        }

        text++;
    }

    while (*text == ' ') {
        text++;
    }

    const char* end = text;

    while (*end != ':' && *end != ';') {
        end++;
    }

    int32_t length = end - text;

    if (length) {
        uint32_t bufferLength = SStrLen(buffer);
        SStrPack(buffer, text, size);

        if (bufferLength + length < size) {
            char* out = &buffer[bufferLength];
            int32_t i = length - 1;
            out[length] = '\0';

            if (out[i] == ' ') {
                do {
                    out[i] = '\0';

                    if (!i) {
                        break;
                    }

                    i--;
                } while (out[i] == ' ');
            }
        }
    }

    return 1;
}

// VERIFY: 92c0a490 manual
// OFFSET: 0x00576FF0
static void SpellParserSkipParenthesis(const char** cursor, const char* end) {
    int32_t depth = 1;

    while (**cursor && *cursor < end) {
        if (**cursor == '(') {
            depth++;
        } else if (**cursor == ')' && !--depth) {
            (*cursor)++;
            return;
        }

        (*cursor)++;
    }
}

// VERIFY: 8169d1a5 manual
// OFFSET: 0x00577030
static int32_t SpellParserVerifyConditional(const char* text, const char** trueStart, const char** trueEnd, const char** elseText, const char** falseStart, const char** falseEnd) {
    *trueStart = SStrChr(text, '[');

    if (!*trueStart) {
        return 0;
    }

    *trueEnd = SStrChr(*trueStart, ']');

    if (!*trueEnd) {
        return 0;
    }

    if ((*trueEnd)[1] == '?') {
        *elseText = *trueEnd + 2;

        const char* nestedStart;
        const char* nestedEnd;
        const char* nestedElse;
        return SpellParserVerifyConditional(*elseText, &nestedStart, &nestedEnd, &nestedElse, falseStart, falseEnd);
    }

    *falseStart = SStrChr(*trueEnd, '[');

    if (!*falseStart) {
        return 0;
    }

    *falseEnd = SStrChr(*falseStart, ']');
    return *falseEnd != nullptr;
}

// VERIFY: a4cd88c6 manual
// OFFSET: 0x00577C40
static int32_t SpellParserVariable(char* buffer, uint32_t size, const char** cursor, int32_t* lastNumber, const char* end) {
    char name[16];
    int32_t length = end - *cursor;

    if (length > 15) {
        length = 15;
    }

    SStrCopy(name, *cursor + 1, length);

    for (uint32_t i = 0; i < s_spellVariables.m_count; i++) {
        if (!SStrCmp(s_spellVariables.m_data[i].m_name, name, STORM_MAX_STR)) {
            SStrPack(buffer, s_spellVariables.m_data[i].m_text, size);
            *lastNumber = s_spellVariables.m_data[i].m_intValue;
            *cursor = end + 1;
            return 1;
        }
    }

    return 0;
}

// VERIFY: 4b6e8371 manual
// OFFSET: 0x00577260
int32_t SpellItemEnchantmentParseText(const SpellItemEnchantmentRec* enchant, char* buffer, uint32_t size, int32_t value) {
    int32_t failed = 0;
    *buffer = '\0';
    const char* text = enchant->m_name;

    for (const char* dollar = SStrChr(text, '$'); dollar && *dollar; dollar = SStrChr(text, '$')) {
        int32_t length = dollar - text;

        if (dollar != text) {
            uint32_t bufferLength = SStrLen(buffer);
            SStrPack(buffer, text, size);

            if (bufferLength + length < size) {
                buffer[bufferLength + length] = '\0';
            }
        }

        const char* cursor = dollar + 1;

        if (*cursor == 'i') {
            char number[32];
            SStrPrintf(number, sizeof(number), "%d", value);
            SStrPack(buffer, number, size);

            if (*cursor) {
                cursor++;
            }
        } else {
            SStrPack(buffer, "$", size);
            failed = 1;
        }

        text = cursor;
    }

    SStrPack(buffer, text, size);
    return failed == 0;
}

// VERIFY: f4c9dae8 manual
// OFFSET: 0x005796C0
static bool SpellParserGetCondition(const char** cursor) {
    char type = **cursor;
    (*cursor)++;
    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    int32_t count = 0;

    while ((*cursor)[count] >= '0' && (*cursor)[count] <= '9') {
        count++;
    }

    SpellRec spell;

    if (!count) {
        return false;
    }

    char digits[11];
    SStrCopy(digits, *cursor, count + 1);
    *cursor += count;
    int32_t spellId = SStrToInt(digits);

    switch (type) {
        case 'A':
        case 'a':
            return player && player->HasAuraBySpellId(spellId);

        case 'S':
        case 's':
            return player && player->IsSpellKnown(spellId);
    }

    return false;
}

// VERIFY: 6f1a2060 manual
// OFFSET: 0x005797F0
static bool SpellParserParseConditional(const char** cursor, const char* end) {
    bool condition = false;

    if (!**cursor) {
        return false;
    }

    while (*cursor < end) {
        switch (**cursor) {
            case '!':
                (*cursor)++;

                while (**cursor && *cursor < end) {
                    if (SStrChr("sSaA", **cursor)) {
                        condition = !SpellParserGetCondition(cursor);
                        break;
                    }

                    if (**cursor == '(') {
                        condition = !SpellParserParseConditional(cursor, end);
                        SpellParserSkipParenthesis(cursor, end);
                        break;
                    }

                    (*cursor)++;
                }

                break;

            case '&':
                if (!condition) {
                    return false;
                }

                (*cursor)++;
                condition = SpellParserParseConditional(cursor, end);
                break;

            case '(':
                (*cursor)++;
                condition = SpellParserParseConditional(cursor, end);
                SpellParserSkipParenthesis(cursor, end);
                break;

            case ')':
                return condition;

            case 'A':
            case 'S':
            case 'a':
            case 's':
                condition = SpellParserGetCondition(cursor);
                break;

            case '|':
                if (condition) {
                    return true;
                }

                (*cursor)++;
                condition = SpellParserParseConditional(cursor, end);
                break;

            default:
                (*cursor)++;
                break;
        }

        if (!**cursor) {
            return condition;
        }
    }

    return condition;
}

// VERIFY: 14a2e3a8 auto
// OFFSET: 0x00579980
static int32_t SpellParserExpression(const SpellRec* spell, char* buffer, uint32_t size, int32_t parsePet, int32_t parseUnit, int32_t spellLevel, int32_t scale, int32_t noModifiers, int32_t* intValue, float* value, const char** cursor, const char* end) {
    char expression[256];
    char text[32];
    char format[5] = "%.0f";

    SStrCopy(expression, *cursor + 1, sizeof(expression));
    expression[end - *cursor - 1] = '\0';
    *cursor = end + 1;

    if (**cursor == '.') {
        char digit = (*cursor)[1];

        if (digit >= '1' && digit <= '9') {
            SStrPrintf(format, sizeof(format), "%%.%df", digit - '0');
            *cursor += 2;
        }
    }

    s_mathState.m_text = expression;
    s_mathState.m_numOps = 0;
    s_mathState.m_numConstants = 0;
    s_mathState.m_numIntArgs = 0;
    s_mathState.m_numVariables = 0;
    s_mathState.m_name = "TomT";
    s_mathState.m_status = 0;
    s_mathState.CompileSum(0);

    if (s_mathState.m_status == 1) {
        return 0;
    }

    s_mathState.m_status = 2;
    *value = s_mathState.Evaluate(spell, spellLevel, scale, parseUnit, parsePet, noModifiers);

    if (s_mathState.m_status != 2) {
        return 0;
    }

    *intValue = static_cast<int32_t>(*value);
    SStrPrintf(text, sizeof(text), format, *value);
    SStrPack(buffer, text, size);
    return 1;
}

// VERIFY: cdf229c9 auto
// OFFSET: 0x00579B30
static void SpellParserVariableExpression(const SpellRec* spell, const char* name, uint32_t nameLength, int32_t parsePet, int32_t parseUnit, int32_t spellLevel, int32_t scale, int32_t noModifiers, const char** cursor, const char* end) {
    SpellVariable* variable = s_spellVariables.New();

    if (!variable) {
        return;
    }

    memset(variable, 0, sizeof(SpellVariable));

    if (SpellParserExpression(spell, variable->m_text, sizeof(variable->m_text), parsePet, parseUnit, spellLevel, scale, noModifiers, &variable->m_intValue, &variable->m_value, cursor, end)) {
        SStrCopy(variable->m_name, name, nameLength);
    }
}

static int32_t SpellParserParseSubText(const SpellRec* spell, char* buffer, uint32_t size, int32_t parsePet, int32_t parseUnit, int32_t spellLevel, int32_t scale, int32_t noModifiers, const char* text, const char** cursor);

// VERIFY: 2490f157 manual
// OFFSET: 0x00579BA0
static int32_t SpellParserConditional(const SpellRec* spell, char* buffer, uint32_t size, int32_t parsePet, int32_t parseUnit, int32_t spellLevel, int32_t scale, int32_t noModifiers, const char** cursor, int32_t variables) {
    char text[1024];
    int32_t failed = 0;

    while (**cursor == ' ') {
        (*cursor)++;
    }

    if (!**cursor) {
        return 1;
    }

    const char* trueStart;
    const char* trueEnd;
    const char* elseText = nullptr;
    const char* falseStart;
    const char* falseEnd;

    if (!SpellParserVerifyConditional(*cursor, &trueStart, &trueEnd, &elseText, &falseStart, &falseEnd)) {
        return 1;
    }

    const char* start;
    const char* stop;

    if (SpellParserParseConditional(cursor, trueStart)) {
        start = trueStart + 1;
        stop = trueEnd;
    } else {
        if (elseText) {
            int32_t result = SpellParserConditional(spell, buffer, size, parsePet, parseUnit, spellLevel, scale, noModifiers, &elseText, variables);
            *cursor = elseText;
            return result;
        }

        stop = falseEnd;
        start = falseStart + 1;
    }

    int32_t length = stop - start + 1;
    SStrCopy(text, start, length);
    text[length] = '\0';
    const char* dollar = SStrChr(text, '$');

    if (variables) {
        if (dollar) {
            const char* expression = SStrChr(dollar, '{');

            if (expression) {
                const char* expressionEnd = SStrChr(expression, '}');

                if (expressionEnd) {
                    SpellParserVariableExpression(spell, buffer, size, parsePet, parseUnit, spellLevel, scale, noModifiers, &expression, expressionEnd);
                }
            }
        }
    } else {
        const char* subCursor = dollar;

        if (!SpellParserParseSubText(spell, buffer, size, parsePet, parseUnit, spellLevel, scale, noModifiers, text, &subCursor)) {
            SStrPack(buffer, "$", size);
            failed = 1;
        }
    }

    *cursor = falseEnd + 1;
    return failed == 0;
}

// VERIFY: ccc450a6 auto
// OFFSET: 0x00579D60
static void SpellParserVariables(const SpellRec* spell, int32_t parsePet, int32_t parseUnit, int32_t spellLevel, int32_t scale, int32_t noModifiers, const char** cursor) {
    while (*cursor) {
        if (!**cursor) {
            return;
        }

        const char* name = *cursor + 1;
        *cursor = name;
        const char* equals = SStrChr(name, '=');

        if (equals) {
            int32_t length = equals - name + 1;
            uint32_t nameLength = length <= 15 ? length : 15;
            *cursor = SStrChr(*cursor, '$');

            if (!*cursor) {
                continue;
            }

            (*cursor)++;

            if (**cursor == '?') {
                (*cursor)++;
                SpellParserConditional(spell, const_cast<char*>(name), nameLength, parsePet, parseUnit, spellLevel, scale, noModifiers, cursor, 1);
            } else if (**cursor == '{') {
                const char* end = SStrChr(*cursor, '}');

                if (end) {
                    SpellParserVariableExpression(spell, name, nameLength, parsePet, parseUnit, spellLevel, scale, noModifiers, cursor, end);
                }
            }
        }

        if (*cursor) {
            *cursor = SStrChr(*cursor, '$');
        }
    }
}

// VERIFY: efbcd492 manual
// OFFSET: 0x00579E50
static int32_t SpellParserReplaceText(const SpellRec* spell, char* buffer, uint32_t size, int32_t parsePet, int32_t parseUnit, int32_t spellLevel, int32_t scale, int32_t noModifiers, const char** cursor) {
    if (**cursor == '{') {
        const char* end = SStrChr(*cursor, '}');

        if (end) {
            float value;
            return SpellParserExpression(spell, buffer, size, parsePet, parseUnit, spellLevel, scale, noModifiers, &s_lastNumber, &value, cursor, end);
        }
    }

    if (**cursor == '<') {
        const char* end = SStrChr(*cursor, '>');

        if (end) {
            return SpellParserVariable(buffer, size, cursor, &s_lastNumber, end);
        }
    }

    float multiplier = 1.0f;

    if (**cursor == '/' || **cursor == '*') {
        const char* semicolon = SStrChr(*cursor, ';');

        if (semicolon) {
            char number[8];
            SStrCopy(number, *cursor + 1, sizeof(number));
            number[semicolon - *cursor - 1] = '\0';
            int32_t factor = SStrToInt(number);

            if (factor) {
                multiplier = static_cast<float>(factor);

                if (**cursor == '/') {
                    multiplier = 1.0f / multiplier;
                }
            }

            *cursor = semicolon + 1;
        }
    }

    int32_t count = 0;

    while ((*cursor)[count] >= '0' && (*cursor)[count] <= '9') {
        count++;
    }

    SpellRec loaded;

    if (count) {
        char digits[8];
        SStrCopy(digits, *cursor, count + 1);
        *cursor += count;

        if (!g_spellDB.GetRecord(QuestTextParser::Spell_C_SpellGetSpellIDForDifficulty(SStrToInt(digits)), &loaded)) {
            return 0;
        }

        spell = &loaded;

        if (loaded.m_attributesExB & 0x1000000) {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player) {
                spellLevel = player->m_unit->m_unitFieldLevel;
            }
        }

        if (loaded.m_maxLevel > 0 && spellLevel >= loaded.m_maxLevel) {
            spellLevel = loaded.m_maxLevel;
        }
    }

    const char* text = *cursor;
    uint32_t index = 0;
    int32_t hasIndex = 0;

    if (static_cast<uint8_t>(text[1] - '1') <= 8) {
        index = text[1] - '1';
        hasIndex = 1;

        if (index >= 3) {
            index = 0;
        }
    }

    char number[32];
    char duration[64];
    const char* output;

    switch (text[0]) {
        case '?':
            if (text[0]) {
                *cursor = text + 1;
            }

            return SpellParserConditional(spell, buffer, size, parsePet, parseUnit, spellLevel, scale, noModifiers, cursor, 0);

        case 'A':
        case 'a': {
            int32_t radius = 0;
            SpellRadiusRec* rec = g_spellRadiusDB.GetRecord(spell->m_effectRadiusIndex[index]);

            if (rec) {
                radius = static_cast<int32_t>(rec->m_radius);
                Spell_C_ModifySpellValue(spell, radius, 6);
            }

            SStrPrintf(number, sizeof(number), "%d", radius);
            output = number;
            break;
        }

        case 'B':
        case 'b':
            s_lastNumber = static_cast<int32_t>(spell->m_effectPointsPerCombo[index]);
            SStrPrintf(number, sizeof(number), "%d", static_cast<int32_t>(spell->m_effectPointsPerCombo[index]));
            output = number;
            break;

        case 'C':
        case 'c': {
            CGUnit_C* unit;

            if (parseUnit) {
                unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGCharacterInfo::m_inspectTarget, TYPE_UNIT, __FILE__, __LINE__));
            } else if (parsePet) {
                unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
            } else {
                unit = ClntObjMgrGetActivePlayerObj();
            }

            SStrPrintf(number, sizeof(number), "%d", Spell_C_GetPowerCost(spell, unit));
            output = number;
            break;
        }

        case 'D':
        case 'd': {
            int32_t time = Spell_C_GetDuration(spell, parsePet, parseUnit, noModifiers, 1);

            if (time > 0) {
                float seconds = static_cast<float>(time);

                if (TimeFractionBelow(seconds, 0.01f)) {
                    CGTooltip::GetDurationString(duration, sizeof(duration), time, "INT_SPELL_DURATION", nullptr, 0, 0);
                } else {
                    CGTooltip::GetFloatDurationString(duration, sizeof(duration), seconds, "SPELL_DURATION", nullptr);
                }
            } else {
                SStrCopy(duration, FrameScript_GetText("SPELL_DURATION_UNTIL_CANCELLED", -1, GENDER_NOT_APPLICABLE), sizeof(duration));
            }

            output = duration;
            break;
        }

        case 'E':
        case 'e': {
            float amplitude = spell->m_effectAmplitude[index];
            Spell_C_ModifySpellValue(spell, amplitude, 27);
            SStrPrintf(number, sizeof(number), "%.1f", amplitude * multiplier);
            output = number;
            break;
        }

        case 'F':
            SStrPrintf(number, sizeof(number), "%d", CMath::fint_n(spell->m_effectChainAmplitude[index] * multiplier));
            output = number;
            break;

        case 'G':
        case 'g': {
            if (text[0]) {
                *cursor = text + 1;
            }

            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (!player) {
                return 0;
            }

            int32_t gender = player->m_unit->m_unitFieldBytes0[2];

            if (gender == 2) {
                gender = player->m_player->m_playerBytes3[0];
            }

            GenderConditional(buffer, size, gender, player, nullptr, cursor);

            if (**cursor) {
                (*cursor)++;
            }

            return 1;
        }

        case 'H':
        case 'h': {
            int32_t chance = spell->m_procChance;
            Spell_C_ModifySpellValue(spell, chance, 18);
            s_lastNumber = chance;
            SStrPrintf(number, sizeof(number), "%d", chance);
            output = number;
            break;
        }

        case 'I':
        case 'i':
            s_lastNumber = spell->m_maxTargets;
            SStrPrintf(number, sizeof(number), "%d", spell->m_maxTargets);
            output = number;
            break;

        case 'L':
        case 'l':
            if (text[0]) {
                *cursor = text + 1;
            }

            return SpellParserPluralConditional(buffer, size, s_lastNumber, cursor);

        case 'M':
        case 'O':
        case 'S':
        case 'm':
        case 'o':
        case 's': {
            float minPoints;
            float maxPoints;
            Spell_C_GetMinMaxPoints(spell, index, &minPoints, &maxPoints, spellLevel, parsePet, parseUnit, noModifiers);
            minPoints *= scale;
            maxPoints *= scale;
            char type = **cursor;

            if (type == 'o' || type == 'O') {
                int32_t period = spell->m_effectAuraPeriod[index];

                if (!period) {
                    period = 5000;
                }

                int32_t time;

                if (period > 0 && (time = Spell_C_GetDuration(spell, parsePet, parseUnit, noModifiers, 0)) > 0) {
                    int32_t total = time + ((spell->m_attributesExE & 0x200) ? period : 0);
                    minPoints = minPoints * total / period;
                    maxPoints = total * maxPoints / period;
                } else {
                    minPoints = 0.0f;
                    maxPoints = 0.0f;
                }
            }

            float minValue = fabsf(minPoints) * multiplier;
            float maxValue = multiplier * fabsf(maxPoints);
            int32_t minInt = CMath::ftrunc(minValue);
            int32_t maxInt = CMath::ftrunc(maxValue);
            int32_t minIsInt;
            int32_t maxIsInt;

            if (minValue - floor(minValue) < 0.001f || ceilf(minValue) - minValue < 0.001f) {
                minIsInt = 1;

                if (minInt != static_cast<int32_t>(ceilf(minValue)) && ceilf(minValue) - minValue < 0.001f) {
                    minInt++;
                }
            } else {
                minIsInt = 0;
            }

            if (maxValue - floor(maxValue) < 0.001f || ceilf(maxValue) - maxValue < 0.001f) {
                maxIsInt = 1;

                if (maxInt != static_cast<int32_t>(ceilf(maxValue)) && ceilf(maxValue) - maxValue < 0.001f) {
                    maxInt++;
                }
            } else {
                maxIsInt = 0;
            }

            type = **cursor;

            if (type == 'm') {
                s_lastNumber = minIsInt ? minInt : 2;
            } else {
                s_lastNumber = maxIsInt ? maxInt : 2;
            }

            if (**cursor == 'm') {
                if (minIsInt) {
                    SStrPrintf(number, sizeof(number), "%d", minInt);
                } else {
                    SStrPrintf(number, sizeof(number), "%.1f", minValue);
                }
            } else if (**cursor == 'M') {
                if (maxIsInt) {
                    SStrPrintf(number, sizeof(number), "%d", maxInt);
                } else {
                    SStrPrintf(number, sizeof(number), "%.1f", maxValue);
                }
            } else if (maxPoints == minPoints) {
                if (minIsInt || type != 'S') {
                    SStrPrintf(number, sizeof(number), "%d", minInt);
                } else {
                    SStrPrintf(number, sizeof(number), "%.1f", minValue);
                }
            } else if (minIsInt && maxIsInt) {
                SStrPrintf(number, sizeof(number), FrameScript_GetText("INT_SPELL_POINTS_SPREAD_TEMPLATE", -1, GENDER_NOT_APPLICABLE), minInt, maxInt);
            } else if (type == 'S') {
                SStrPrintf(number, sizeof(number), FrameScript_GetText("SPELL_POINTS_SPREAD_TEMPLATE", -1, GENDER_NOT_APPLICABLE), minValue, maxValue);
            } else {
                int32_t upper = maxIsInt ? maxInt : static_cast<int32_t>(maxValue) + 1;
                SStrPrintf(number, sizeof(number), FrameScript_GetText("INT_SPELL_POINTS_SPREAD_TEMPLATE", -1, GENDER_NOT_APPLICABLE), static_cast<int32_t>(minValue), upper);
            }

            output = number;
            break;
        }

        case 'N':
        case 'n': {
            int32_t charges = spell->m_procCharges;
            Spell_C_ModifySpellValue(spell, charges, 4);
            s_lastNumber = charges;
            SStrPrintf(number, sizeof(number), "%d", charges);
            output = number;
            break;
        }

        case 'P':
        case 'p': {
            CGUnit_C* unit;

            if (parseUnit) {
                unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGCharacterInfo::m_inspectTarget, TYPE_UNIT, __FILE__, __LINE__));
            } else if (parsePet) {
                unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
            } else {
                unit = ClntObjMgrGetActivePlayerObj();
            }

            SStrPrintf(number, sizeof(number), "%d", Spell_C_GetPowerCostPerSecond(spell, unit));
            output = number;
            break;
        }

        case 'Q':
        case 'q':
            s_lastNumber = spell->m_effectMiscValue[index];
            SStrPrintf(number, sizeof(number), "%d", spell->m_effectMiscValue[index]);
            output = number;
            break;

        case 'R':
        case 'r': {
            float range = 0.0f;
            SpellRangeRec* rec = g_spellRangeDB.GetRecord(spell->m_rangeIndex <= 1 ? 1 : spell->m_rangeIndex);

            if (rec) {
                range = rec->m_rangeMax[0];
                Spell_C_ModifySpellValue(spell, range, 5);
            }

            s_lastNumber = static_cast<int32_t>(ceilf(range));
            SStrPrintf(number, sizeof(number), "%d", static_cast<int32_t>(range));
            output = number;
            break;
        }

        case 'T':
        case 't': {
            int32_t period;

            if (spell->m_procTypeMask & 1) {
                period = 5000;
            } else {
                period = spell->m_effectAuraPeriod[index];
                Spell_C_ModifySpellValue(spell, period, 19);

                for (uint32_t i = 0; i < 3; i++) {
                    if (spell->m_effectAuraPeriod[i]) {
                        if ((spell->m_attributesExE & 0x2000) && !(spell->m_attributesExC & 0x20000000)) {
                            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

                            if (player && player->m_unit->m_unitModCastSpeed >= 0.001f) {
                                period = CMath::fint_(period * player->m_unit->m_unitModCastSpeed);
                            }
                        }

                        break;
                    }
                }
            }

            if (TimeFractionBelow(static_cast<float>(period), 0.01f)) {
                SStrPrintf(number, sizeof(number), "%d", period / 1000);
            } else {
                SStrPrintf(number, sizeof(number), "%.2f", period * 0.001f);
            }

            output = number;
            break;
        }

        case 'U':
        case 'u':
            s_lastNumber = spell->m_cumulativeAura;
            SStrPrintf(number, sizeof(number), "%d", spell->m_cumulativeAura);
            output = number;
            break;

        case 'V':
        case 'v':
            s_lastNumber = spell->m_maxTargetLevel;
            SStrPrintf(number, sizeof(number), "%d", spell->m_maxTargetLevel);
            output = number;
            break;

        case 'X':
        case 'x': {
            int32_t targets = spell->m_effectChainTargets[index];
            Spell_C_ModifySpellValue(spell, targets, 17);
            s_lastNumber = targets;
            SStrPrintf(number, sizeof(number), "%d", targets);
            output = number;
            break;
        }

        case 'Z':
        case 'z': {
            AreaTableRec* area = g_areaTableDB.GetRecord(CGPlayer_C::GetBindAreaID());
            output = area ? area->m_areaName : FrameScript_GetText("HOME_INN", -1, GENDER_NOT_APPLICABLE);
            break;
        }

        case 'f':
            SStrPrintf(number, sizeof(number), "%.1f", spell->m_effectChainAmplitude[index] * multiplier);
            output = number;
            break;

        default:
            return 0;
    }

    SStrPack(buffer, output, size);

    if (**cursor) {
        (*cursor)++;

        if (hasIndex) {
            (*cursor)++;
        }
    }

    return 1;
}

// VERIFY: 93c08fa8 auto
// OFFSET: 0x0057AAF0
static int32_t SpellParserParseSubText(const SpellRec* spell, char* buffer, uint32_t size, int32_t parsePet, int32_t parseUnit, int32_t spellLevel, int32_t scale, int32_t noModifiers, const char* text, const char** cursor) {
    int32_t failed = 0;

    while (*cursor && **cursor) {
        int32_t length = *cursor - text;

        if (*cursor != text) {
            uint32_t bufferLength = SStrLen(buffer);
            SStrPack(buffer, text, size);

            if (bufferLength + length < size) {
                buffer[bufferLength + length] = '\0';
            }
        }

        (*cursor)++;

        if (!SpellParserReplaceText(spell, buffer, size, parsePet, parseUnit, spellLevel, scale, noModifiers, cursor)) {
            SStrPack(buffer, "$", size);
            failed = 1;
        }

        text = *cursor;
        *cursor = SStrChr(*cursor, '$');
    }

    SStrPack(buffer, text, size);
    return failed == 0;
}

// VERIFY: 17c38493 auto
// OFFSET: 0x0057ABC0
int32_t SpellParserParseText(const SpellRec* spell, char* buffer, uint32_t size, int32_t parsePet, int32_t parseUnit, int32_t spellLevel, int32_t useAuraDescription, int32_t scale, int32_t noModifiers) {
    *buffer = '\0';

    if (!spellLevel) {
        spellLevel = Spell_C_GetSpellLevel(spell, parsePet, parseUnit);
    }

    SpellDescriptionVariablesRec* variables = g_spellDescriptionVariablesDB.GetRecord(spell->m_descriptionVariablesID);

    if (variables) {
        const char* cursor = SStrChr(variables->m_variables, '$');
        SpellParserVariables(spell, parsePet, parseUnit, spellLevel, scale, noModifiers, &cursor);
    }

    const char* description;

    if (!useAuraDescription || !*(description = spell->m_auraDescription)) {
        description = spell->m_description;
    }

    const char* cursor = SStrChr(description, '$');
    int32_t result = SpellParserParseSubText(spell, buffer, size, parsePet, parseUnit, spellLevel, scale, noModifiers, description, &cursor);
    s_spellVariables.m_count = 0;
    return result;
}

// VERIFY: b9d5f6fa manual
// OFFSET: 0x00579590
int32_t QuestParserParseText(const char* text, char* buffer, uint32_t size, const WOWGUID& guid, const char* name, int32_t a6) {
    bool failed = false;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));
    NameCache* record;

    if (unit) {
        record = nullptr;
    } else {
        WOWGUID requester = { 0 };
        record = g_nameDBCache.GetRecord(guid.m_guid, requester, nullptr, nullptr, false);
    }

    *buffer = 0;
    const char* cursor = text;

    for (const char* token = SStrChr(text, '$'); token && *token; token = SStrChr(cursor, '$')) {
        uint32_t length = static_cast<uint32_t>(token - cursor);

        if (length) {
            uint32_t end = static_cast<uint32_t>(SStrLen(buffer));
            SStrPack(buffer, cursor, size);

            if (end + length < size) {
                buffer[end + length] = 0;
            }
        }

        cursor = token + 1;

        if (!QuestParserParseToken(buffer, size, record, name, a6, guid, unit, &cursor)) {
            SStrPack(buffer, "$", size);
            failed = true;
        }
    }

    SStrPack(buffer, cursor, size);
    return !failed;
}

// VERIFY: 692b064f manual
// OFFSET: 0x00576E50
void ParseCountDownTimerText(char* buffer, uint32_t size, int32_t time) {
    int32_t remaining = time - CGWorldStateInfo::GetUITime();

    if (remaining < 0) {
        remaining = 0;
    }

    int32_t hours = remaining / 3600;
    remaining %= 3600;
    int32_t minutes = remaining / 60;
    int32_t seconds = remaining % 60;
    char text[16];

    if (hours <= 0) {
        SStrPrintf(text, sizeof(text), "%02d:%02d", minutes, seconds);
    } else {
        SStrPrintf(text, sizeof(text), "%d:%02d:%02d", hours, minutes, seconds);
    }

    SStrPack(buffer, text, size);
}

// VERIFY: 5dfbac31 manual
// OFFSET: 0x00578F20
bool QuestParserParseToken(char* buffer, uint32_t size, const NameCache* record, const char* name, int32_t achievement, WOWGUID guid, const CGUnit_C* unit, const char** cursor) {
    char number[8];
    uint32_t digits = 0;

    if (**cursor) {
        do {
            char c = (*cursor)[digits];

            if (c < '0' || c > '9') {
                break;
            }

            digits++;
        } while ((*cursor)[digits]);

        if (digits) {
            SStrCopy(number, *cursor, digits + 1);
            *cursor += digits;
        }
    }

    char token = **cursor;
    char text[64];
    char value[16];
    char label[32];

    switch (token) {
        case 'A':
        case 'a':
            if (g_achievementDB.GetRecord(achievement)) {
                SStrPack(buffer, CGTooltip::GetAchievementHyperlink(achievement, guid, 1, g_clientGameTime, -1, -1, -1, -1), size);
            }

            break;

        case 'B':
        case 'b':
            SStrPack(buffer, "\n", size);
            break;

        case 'C':
        case 'c': {
            const char* className = CGUnit_C::GetDisplayClassName(unit, record);

            if (!className) {
                return false;
            }

            if (unit && !(unit->m_obj->m_type & TYPE_PLAYER)) {
                SStrPack(buffer, className, size);
                break;
            }

            SStrCopy(label, className, sizeof(label));

            if (**cursor == 'c') {
                SStrLower(label);
            }

            SStrPack(buffer, label, size);
            break;
        }

        case 'D':
        case 'd': {
            double minutes = GetWorldState(-SStrToInt(number));
            text[0] = 0;

            if (minutes >= 1440.0) {
                SStrPrintf(text, sizeof(text), FrameScript_GetText("INT_GENERAL_DURATION_DAYS", -1, GENDER_NOT_APPLICABLE), static_cast<int32_t>(static_cast<int64_t>((minutes - 1.0) * 0.0006944444444444445 + 1.0)));
            } else if (minutes >= 60.0) {
                bool roundUp = fmod(minutes, 60.0) >= 30.0;
                SStrPrintf(text, sizeof(text), FrameScript_GetText("INT_GENERAL_DURATION_HOURS", -1, GENDER_NOT_APPLICABLE), static_cast<int32_t>(static_cast<int64_t>(roundUp + minutes * 0.016666666666666666)));
            } else {
                SStrPrintf(text, sizeof(text), FrameScript_GetText("INT_GENERAL_DURATION_MIN", -1, GENDER_NOT_APPLICABLE), static_cast<int32_t>(static_cast<int64_t>(minutes)));
            }

            SStrPack(buffer, text, size);
            break;
        }

        case 'E':
        case 'e':
            SStrPrintf(value, sizeof(value), "%d", GetWorldState(-SStrToInt(number)));
            SStrPack(buffer, value, size);
            break;

        case 'G':
        case 'g': {
            if (!unit && !record) {
                return false;
            }

            if (token) {
                (*cursor)++;
            }

            int32_t gender = unit ? unit->m_unit->m_unitFieldBytes0[2] : record->m_sex;

            if (gender == 2 && unit && const_cast<CGUnit_C*>(unit)->IsActivePlayer()) {
                gender = static_cast<const CGPlayer_C*>(unit)->m_player->m_playerBytes3[0];
            }

            GenderConditional(buffer, size, gender, unit, record, cursor);
            break;
        }

        case 'K':
        case 'k':
            ParseCountDownTimerText(buffer, size, GetWorldState(SStrToInt(number)));
            break;

        case 'N':
        case 'n':
            if (!unit && !record) {
                if (!name) {
                    return false;
                }

                SStrPack(buffer, name, size);
            } else if (name) {
                SStrPack(buffer, name, size);
            } else if (unit) {
                SStrPack(buffer, const_cast<CGUnit_C*>(unit)->GetUnitName(nullptr, 1), size);
            } else {
                SStrPack(buffer, record->m_name, size);
            }

            break;

        case 'R':
        case 'r': {
            const char* raceName = CGUnit_C::GetDisplayRaceName(unit, record);

            if (!raceName) {
                return false;
            }

            if (unit && !(unit->m_obj->m_type & TYPE_PLAYER)) {
                SStrPack(buffer, raceName, size);
                break;
            }

            SStrCopy(label, raceName, sizeof(label));

            if (**cursor == 'r') {
                SStrLower(label);
            }

            SStrPack(buffer, label, size);
            break;
        }

        case 'T':
        case 't': {
            if (!unit && !record) {
                return false;
            }

            int32_t lower = token == 't';
            (*cursor)++;

            while (**cursor == ' ') {
                (*cursor)++;
            }

            if (!**cursor) {
                return true;
            }

            const char* colon = SStrChr(*cursor, ':');

            if (!colon) {
                return true;
            }

            const char* semicolon = SStrChr(colon, ';');

            if (!semicolon) {
                return true;
            }

            text[0] = 0;
            int32_t gender;

            if (unit) {
                if (unit->m_obj->m_type & TYPE_PLAYER) {
                    const CGPlayer_C* player = static_cast<const CGPlayer_C*>(unit);
                    uint8_t rank = player->m_player->m_playerFieldBytes[3];
                    SStrPrintf(label, sizeof(label), "PVP_RANK_%d_%d", rank, player->GetPVPFactionIndex());
                    const char* rankName = FrameScript_GetText(label, -1, GENDER_NOT_APPLICABLE);

                    if (rankName && *rankName) {
                        SStrCopy(text, rankName, sizeof(text));
                    }

                    if (text[0]) {
                        if (lower) {
                            SStrLower(text);
                        }

                        SStrPack(buffer, text, size);
                        *cursor = semicolon;
                        break;
                    }
                }

                gender = unit->m_unit->m_unitFieldBytes0[2];
            } else {
                gender = record->m_sex;
            }

            const char* end = colon;

            if (gender) {
                *cursor = colon + 1;

                while (**cursor == ' ') {
                    (*cursor)++;
                }

                end = semicolon;
            }

            uint32_t length = static_cast<uint32_t>(end - *cursor);

            if (length) {
                uint32_t start = static_cast<uint32_t>(SStrLen(buffer));
                SStrPack(buffer, *cursor, size);

                if (start + length < size) {
                    buffer[start + length] = 0;
                    char* segment = buffer + start;
                    uint32_t i = length - 1;

                    if (segment[i] == ' ') {
                        do {
                            segment[i] = 0;

                            if (!i) {
                                break;
                            }

                            i--;
                        } while (segment[i] == ' ');
                    }
                }
            }

            *cursor = semicolon;
            break;
        }

        case 'W':
        case 'w':
            SStrPrintf(value, sizeof(value), "%d", GetWorldState(SStrToInt(number)));
            SStrPack(buffer, value, size);
            break;

        default:
            return false;
    }

    if (**cursor) {
        (*cursor)++;
    }

    return true;
}
