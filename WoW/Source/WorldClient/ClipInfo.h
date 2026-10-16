#ifndef WOW_SOURCE_WORLDCLIENT_CLIPINFO_H
#define WOW_SOURCE_WORLDCLIENT_CLIPINFO_H

#include <cstddef>
#include <cstdint>

class C3Vector;

class ClipInfo {
    public:
    /* 0000 */ uint32_t unused_0000;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint32_t unused_0008;
    /* 000C */ uint32_t unused_000C;
    /* 0010 */ uint32_t unused_0010;
    /* 0014 */ uint32_t unused_0014;
    /* 0018 */ uint32_t unused_0018;
};

namespace World {

    struct CLIPINFO {
        float d[6];
        uint32_t outcode;
        uint32_t pad;

        void Set(const C3Vector& ndc);
    };

    struct CLIPPOLY {
        /* 0000 */ C3Vector** verts;
        /* 0004 */ CLIPINFO** infos;
        /* 0008 */ uint32_t count;
    };

}


#endif
