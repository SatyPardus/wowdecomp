#include "pcreposix.h"
#include <cstdlib>
#include <cstring>
#include "pcre.h"

static const int32_t POSIX_MALLOC_THRESHOLD = 10;

// STATIC: 0x00A46180
static const char* const estring[] = {
    "\\ at end of pattern",
    "\\c at end of pattern",
    "unrecognized character follows \\",
    "numbers out of order in {} quantifier",
    "number too big in {} quantifier",
    "missing terminating ] for character class",
    "invalid escape sequence in character class",
    "range out of order in character class",
    "nothing to repeat",
    "operand of unlimited repeat could match the empty string",
    "internal error: unexpected repeat",
    "unrecognized character after (?",
    "POSIX named classes are supported only within a class",
    "missing )",
    "reference to non-existent subpattern",
    "erroffset passed as NULL",
    "unknown option bit(s) set",
    "missing ) after comment",
    "parentheses nested too deeply",
    "regular expression too large",
    "failed to get memory",
    "unmatched parentheses",
    "internal error: code overflow",
    "unrecognized character after (?<",
    "lookbehind assertion is not fixed length",
    "malformed number after (?(",
    "conditional group contains more than two branches",
    "(?R or (?digits must be followed by )",
    "(?R or (?digits must be followed by )",
    "unknown POSIX class name",
    "POSIX collating elements are not supported",
    "this version of PCRE is not compiled with PCRE_UTF8 support",
    "spare error",
    "character value in \\x{...} sequence is too large",
    "invalid condition (?(0)",
    "\\C not allowed in lookbehind assertion",
    "PCRE does not support \\L, \\l, \\N, \\U, or \\u",
    "number after (?C is > 255",
    "closing ) for (?C expected",
    "recursive call could loop indefinitely",
    "unrecognized character after (?P",
    "syntax error after (?P",
    "two named groups have the same name",
    "invalid UTF-8 string",
    "support for \\P, \\p, and \\X has not been compiled",
    "malformed \\P or \\p sequence",
    "unknown property name after \\P or \\p"
};

// STATIC: 0x00A46240
static const int32_t eint[] = {
    REG_EESCAPE,
    REG_EESCAPE,
    REG_EESCAPE,
    REG_BADBR,
    REG_BADBR,
    REG_EBRACK,
    REG_ECTYPE,
    REG_ERANGE,
    REG_BADRPT,
    REG_BADRPT,
    REG_ASSERT,
    REG_BADPAT,
    REG_BADPAT,
    REG_EPAREN,
    REG_ESUBREG,
    REG_INVARG,
    REG_INVARG,
    REG_EPAREN,
    REG_ESIZE,
    REG_ESIZE,
    REG_ESPACE,
    REG_EPAREN,
    REG_ASSERT,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT,
    REG_ECTYPE,
    REG_BADPAT,
    REG_INVARG,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT,
    REG_EESCAPE,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT,
    REG_BADPAT
};

// VERIFY: 6b643012 manual
// OFFSET: 0x0083DEC0
static int32_t pcre_posix_error_code(const char* s) {
    for (size_t i = 0; i < sizeof(estring) / sizeof(estring[0]); i++) {
        if (strcmp(s, estring[i]) == 0) {
            return eint[i];
        }
    }

    return REG_ASSERT;
}

// VERIFY: 086e456f manual
// OFFSET: 0x0083DF20
int32_t regcomp(regex_t* preg, const char* pattern, int32_t cflags) {
    *preg = {};
    const char* errorptr;
    int erroffset;
    int options = PCRE_UTF8 | PCRE_NO_UTF8_CHECK;

    if ((cflags & REG_ICASE) != 0) {
        options |= PCRE_CASELESS;
    }

    if ((cflags & REG_NEWLINE) != 0) {
        options |= PCRE_MULTILINE;
    }

    preg->re_pcre = pcre_compile(pattern, options, &errorptr, &erroffset, nullptr);
    preg->re_erroffset = erroffset;

    if (preg->re_pcre == nullptr) {
        return pcre_posix_error_code(errorptr);
    }

    preg->re_extra = nullptr;
    preg->re_nsub = pcre_info(static_cast<const pcre*>(preg->re_pcre), nullptr, nullptr);

    return 0;
}

void regfree(regex_t* preg) {
    if (preg->re_extra) pcre_free(preg->re_extra);
    if (preg->re_pcre) pcre_free(preg->re_pcre);
    *preg = {};
}

// VERIFY: 50a3d188 manual
// OFFSET: 0x0083DFA0
int32_t regexec(const regex_t* preg, const char* string, size_t nmatch, regmatch_t pmatch[], int32_t eflags) {
    int rc;
    int options = PCRE_NO_UTF8_CHECK;
    int* ovector = nullptr;
    int small_ovector[POSIX_MALLOC_THRESHOLD * 3];
    bool allocated_ovector = false;

    if ((eflags & REG_NOTBOL) != 0) {
        options |= PCRE_NOTBOL;
    }

    if ((eflags & REG_NOTEOL) != 0) {
        options |= PCRE_NOTEOL;
    }

    const_cast<regex_t*>(preg)->re_erroffset = static_cast<size_t>(-1);

    if (nmatch > 0) {
        if (nmatch <= POSIX_MALLOC_THRESHOLD) {
            ovector = &small_ovector[0];
        } else {
            ovector = static_cast<int*>(malloc(sizeof(int) * nmatch * 3));

            if (ovector == nullptr) {
                return REG_ESPACE;
            }

            allocated_ovector = true;
        }
    }

    rc = pcre_exec(static_cast<const pcre*>(preg->re_pcre), preg->re_extra, string, static_cast<int>(strlen(string)), 0, options, ovector, static_cast<int>(nmatch * 3));

    if (rc == 0) {
        rc = static_cast<int>(nmatch);
    }

    if (rc >= 0) {
        size_t i;

        for (i = 0; i < static_cast<size_t>(rc); i++) {
            pmatch[i].rm_so = ovector[i * 2];
            pmatch[i].rm_eo = ovector[i * 2 + 1];
        }

        if (allocated_ovector) {
            free(ovector);
        }

        for (; i < nmatch; i++) {
            pmatch[i].rm_so = pmatch[i].rm_eo = -1;
        }

        return 0;
    }

    if (allocated_ovector) {
        free(ovector);
    }

    switch (rc) {
        case PCRE_ERROR_NOMATCH:
            return REG_NOMATCH;

        case PCRE_ERROR_NULL:
            return REG_INVARG;

        case PCRE_ERROR_BADOPTION:
            return REG_INVARG;

        case PCRE_ERROR_BADMAGIC:
            return REG_INVARG;

        case PCRE_ERROR_UNKNOWN_NODE:
            return REG_ASSERT;

        case PCRE_ERROR_NOMEMORY:
            return REG_ESPACE;

        case PCRE_ERROR_MATCHLIMIT:
            return REG_ESPACE;

        case PCRE_ERROR_BADUTF8:
            return REG_INVARG;

        case PCRE_ERROR_BADUTF8_OFFSET:
            return REG_INVARG;

        default:
            return REG_ASSERT;
    }
}
