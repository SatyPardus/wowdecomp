#ifndef ENGINE_SOURCE_BASE_TYPES_H
#define ENGINE_SOURCE_BASE_TYPES_H

#include <cstddef>
#include <cstdint>

// Client.h extends this shared command ID space beyond the engine's entries.
// A fixed 32-bit representation permits those IDs without an invalid enum load.
enum CMDOPT : uint32_t {
    CMD_D3D = 0,
    CMD_D3D9EX = 1,
    CMD_DATA_DIR = 2,
    CMD_NO_LAG_FIX = 3,
    CMD_LOADFILE = 4,
    CMD_GAMETYPE = 5,
    CMD_OPENGL = 6,
    CMD_SW_TNL = 7,
    CMD_TIMEDEMO = 8,
    CMD_DEMOREZ = 9,
    CMD_DEMODEPTH = 10,
    CMD_DEMODETAIL = 11,
    CMD_DEMOSOUND = 12,
    CMD_FULL_SCREEN = 13,
    CMD_22050HZ = 14,
    CMD_NO_WARNINGS = 15,
    CMDOPTS = 16,
};

enum PROPERTY {
    PROP_EVENTCONTEXT = 0,
    PROP_HANDLETABLE = 1,
    PROP_EVENTSTATE = 2,
    PROP_TIMERS = 3,
    PROP_STRINGSTATE = 4,
    PROP_JASS = 5,
    PROP_JVM = 6,
    PROP_TEXTURES = 7,
    PROP_MODELS = 8,
    PROP_AGILE = 9,
    PROP_TEMPEST = 10,
    PROP_RAIN = 11,
    PROP_IPSE = 12,
    PROP_APPLICATION = 13,
    PROP_NET = 14,
    PROP_WORLD = 15,
    PROP_BATTLENET = 16,
    PROP_COLLISION = 17,
    PROP_AUTHENTICATION = 18,
    PROPERTIES = 19,
};

enum STATUS_TYPE {
    STATUS_INFO = 0,
    STATUS_WARNING = 1,
    STATUS_ERROR = 2,
    STATUS_FATAL = 3,
    STATUS_NUMTYPES = 4,
};

struct HOBJECT__ {
    /* 0000 */ int32_t m_unused;
};

typedef HOBJECT__* HOBJECT;

struct HPROPCONTEXT__ {
    /* 0000 */ int32_t m_unused;
};

typedef HPROPCONTEXT__* HPROPCONTEXT;

#endif
