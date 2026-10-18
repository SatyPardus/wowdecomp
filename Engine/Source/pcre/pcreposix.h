#ifndef ENGINE_SOURCE_PCRE_PCREPOSIX_H
#define ENGINE_SOURCE_PCRE_PCREPOSIX_H

#include <cstddef>
#include <cstdint>

struct pcre_extra;

enum {
    REG_ICASE = 0x0001,
    REG_NEWLINE = 0x0002,
    REG_NOTBOL = 0x0004,
    REG_NOTEOL = 0x0008
};

enum {
    REG_ASSERT = 1,
    REG_BADBR,
    REG_BADPAT,
    REG_BADRPT,
    REG_EBRACE,
    REG_EBRACK,
    REG_ECOLLATE,
    REG_ECTYPE,
    REG_EESCAPE,
    REG_EMPTY,
    REG_EPAREN,
    REG_ERANGE,
    REG_ESIZE,
    REG_ESPACE,
    REG_ESUBREG,
    REG_INVARG,
    REG_NOMATCH
};

typedef int32_t regoff_t;

struct regex_t {
    /* 0000 */ void* re_pcre;
    /* 0004 */ pcre_extra* re_extra;
    /* 0008 */ size_t re_nsub;
    /* 000C */ size_t re_erroffset;
};

struct regmatch_t {
    /* 0000 */ regoff_t rm_so;
    /* 0004 */ regoff_t rm_eo;
};

int32_t regcomp(regex_t* preg, const char* pattern, int32_t cflags);
void regfree(regex_t* preg);
int32_t regexec(const regex_t* preg, const char* string, size_t nmatch, regmatch_t pmatch[], int32_t eflags);

#endif
