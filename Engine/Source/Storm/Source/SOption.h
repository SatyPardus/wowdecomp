#ifndef ENGINE_SOURCE_STORM_SOURCE_SOPTION_H
#define ENGINE_SOURCE_STORM_SOURCE_SOPTION_H

#include <cstdint>

struct _STORMOPTIONS {
    /* 0000 */ int32_t smemleaksilentwarning;
    /* 0004 */ int32_t serrleaksilentwarning;
    /* 0008 */ uint32_t wavechunksize;
    /* 000C */ int32_t alignstreamingwavedata;
    /* 0010 */ int32_t echotooutputdebugstring;
    /* 0014 */ int32_t serrsuppresslogs;
    /* 0018 */ int32_t crcenabled;
    /* 001C */ int32_t orderedprintfenabled;
    /* 0020 */ int32_t unicodeenabled;
    /* 0024 */ int32_t unused_0024;
    /* 0028 */ int32_t unk_0028;
};

extern _STORMOPTIONS g_opt;

#endif
