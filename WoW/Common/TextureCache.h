#ifndef WOW_COMMON_TEXTURECACHE_H
#define WOW_COMMON_TEXTURECACHE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/BLPFile/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Common/ComponentCore/Types.h"

class CACHEENTRY;
typedef CACHEENTRY* HTEXTURECACHE;
class CAsyncObject;

class CACHEENTRY : public TSHashObject<CACHEENTRY, HASHKEY_NONE> {
    public:
    /* 0018 */ CAsyncObject* m_asyncObject;
    /* 001C */ TCTEXTUREINFO m_info;
    /* 0024 */ char m_fileName [128];
    /* 00A4 */ int32_t m_refCount;
    /* 00A8 */ uint32_t m_memHandle;
    /* 00AC */ BLPHeader* m_data;
    /* 00B0 */ uint32_t m_size : 20;
    /* 00B0 */ uint32_t m_loadFailed : 1;
    /* 00B0 */ uint32_t m_unused : 11;

    CACHEENTRY();
    ~CACHEENTRY();
    int32_t LoadTexture();
};

void TextureCacheComputeLoadProgress(HTEXTURECACHE handle, int64_t& local, int64_t& total);
HTEXTURECACHE TextureCacheCopyTexture(HTEXTURECACHE handle);
HTEXTURECACHE TextureCacheCreateTexture(const char* fileName);
void TextureCacheDestroy();
void TextureCacheDestroyTexture(HTEXTURECACHE handle);
int32_t TextureCacheGetInfo(HTEXTURECACHE handle, TCTEXTUREINFO& info, int32_t wait);
void* TextureCacheGetMip(HTEXTURECACHE handle, uint32_t mipLevel);
BlpPalPixel* TextureCacheGetPal(HTEXTURECACHE handle);
int32_t TextureCacheHasMips(HTEXTURECACHE handle);
void TextureCacheResetLoadCount();

#endif
