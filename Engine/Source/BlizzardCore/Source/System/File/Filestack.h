#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_FILE_FILESTACK_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_FILE_FILESTACK_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/BlizzardCore/Source/System/File/Types.h"

namespace System_File {
namespace Stacked {
struct FileParms {
    /* 0000 */ offset_in_Blizzard::File::Functions_to_System_File::Stacked::FileFunc m_verb;
    /* 0004 */ char* m_path;
    /* 0008 */ char* m_newpath;
    /* 000C */ Blizzard::File::Stream m_stream;
    /* 0010 */ Blizzard::File::FileInfo* m_info;
    /* 0014 */ uint32_t m_extra;
    /* 0018 */ Blizzard::File::FileInfo m_info2;
    /* 0050 */ Blizzard::File::FileInfoMask m_has;
    /* 0054 */ Blizzard::File::FileInfoMask m_want;
    /* 0058 */ int32_t m_mode;
    /* 005C */ void* m_buffer;
    /* 0060 */ int32_t m_amount;
    /* 0064 */ uint8_t unused_0064[4];
    /* 0068 */ int64_t m_pos;
    /* 0070 */ int32_t m_posMode;
    /* 0074 */ char* m_cwdbuf;
    /* 0078 */ int32_t m_cwdbuflen;
    /* 007C */ bool m_recursive;
    /* 007D */ bool m_makeCanonical;
    /* 007E */ uint8_t unused_007E[2];
    /* 0080 */ void* m_cookie;
    /* 0084 */ Blizzard::File::ProcessDirCallback m_dircallback;
    /* 0088 */ bool m_sorted;
    /* 0089 */ bool m_overwrite;
    /* 008A */ bool m_grantEveryone;
    /* 008B */ bool m_zeroFillAll;
    /* 008C */ bool m_supportsSparseFiles;
    /* 008D */ uint8_t unused_008D[3];
};
}
}

#endif
