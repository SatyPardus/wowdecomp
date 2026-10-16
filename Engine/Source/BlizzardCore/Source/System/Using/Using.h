#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_USING_USING_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_USING_USING_H

#include <cstdint>

#define USING_MAX 32
#define DEPENDS_MAX 128
#define STRINGPOOL_SIZE 1024
#define HASH_SIZE 64

namespace System_Using {

struct Depends {
    /* 0000 */ int16_t id;
    /* 0002 */ uint8_t unused_0002[2];
    /* 0004 */ Depends* next;
};

struct UsingRec {
    /* 0000 */ char* name;
    /* 0004 */ int16_t id;
    /* 0006 */ int16_t refCount;
    /* 0008 */ void (*shutdownproc)();
    /* 000C */ int16_t dependCount;
    /* 000E */ uint8_t unused_000E[2];
    /* 0010 */ Depends* depends;
};

class UsingHashTable {
    public:
    struct Entry {
        /* 0000 */ uint32_t m_hash;
        /* 0004 */ uint16_t m_value;
        /* 0006 */ uint8_t unused_0006[2];
    };

    static uint32_t Hash(const char* name);

    /* 0000 */ Entry m_entries[HASH_SIZE];
    /* 0200 */ uint32_t m_used;

    bool Find(const char* name, uint32_t& slot);
    void Insert(const char* name, uint16_t value);
};

extern char s_UsingNames[STRINGPOOL_SIZE];
extern Depends s_Depends[DEPENDS_MAX];
extern UsingRec use[USING_MAX];
extern UsingHashTable s_Using;
extern char* s_UsingNamesTail;
extern int16_t s_NextUsingID;
extern int32_t s_NextDependsID;

UsingRec* Intern(const char* name);
void RegisterNull();
void ShutdownUnused();

}

namespace Blizzard {
namespace Using {

class GlobalUse {
    public:
    /* 0000 */ const char* m_name;

    GlobalUse();
    ~GlobalUse();
};

class Use {
    public:
    /* 0000 */ const char* m_name;

    Use(const char* name);
    ~Use();
};

void DependsOn(const char* me, const char* dependsOn);
void StartUsing(const char* name);
void StopUsing(const char* name);

}
}

namespace System_Using {

class UsingGlobalUse : public Blizzard::Using::Use {
    public:
    UsingGlobalUse();
};

}

#endif
