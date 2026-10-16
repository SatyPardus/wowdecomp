#include <cstdint>
#include "SOption.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00CAE950
static int32_t s_freecount;

// STATIC: 0x00CAE954
static int32_t s_alloccount;

// STATIC: 0x00CAE958
_STORMOPTIONS g_opt;

// VERIFY: e736f9e3 auto
// OFFSET: 0x00773A80
int32_t STORMAPI StormSetOption(int32_t optname, void* optval, uint32_t optlen) {
    SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);

    if (!optval) {
        return 0;
    }

    int32_t* value = static_cast<int32_t*>(optval);

    switch (optname) {
    case STORM_OPTION_SERR_LEAK_SILENT_WARNING:
        if (optlen != sizeof(int32_t)) {
            return 0;
        }

        g_opt.serrleaksilentwarning = *value;
        break;
    case STORM_OPTION_WAVE_CHUNK_SIZE:
        if (optlen != sizeof(int32_t) || !*value || (*value ^ (*value - 1)) != 2 * *value - 1) {
            return 0;
        }

        g_opt.wavechunksize = *value;
        break;
    case STORM_OPTION_SMEM_LEAK_SILENT_WARNING:
        if (optlen != sizeof(int32_t)) {
            return 0;
        }

        g_opt.smemleaksilentwarning = *value;
        break;
    case STORM_OPTION_ALIGN_STREAMING_WAVE_DATA:
        if (optlen != sizeof(int32_t)) {
            return 0;
        }

        g_opt.alignstreamingwavedata = *value;
        break;
    case STORM_OPTION_ECHO_TO_OUTPUT_DEBUG_STRING:
        if (optlen != sizeof(int32_t)) {
            return 0;
        }

        g_opt.echotooutputdebugstring = *value;
        break;
    case STORM_OPTION_ALLOC_COUNTS:
        if (optlen != 2 * sizeof(int32_t)) {
            return 0;
        }

        s_alloccount = value[0];
        s_freecount = value[1];
        break;
    case STORM_OPTION_SERR_SUPPRESS_LOGS:
        if (optlen != sizeof(int32_t)) {
            return 0;
        }

        g_opt.serrsuppresslogs = *value;
        break;
    case STORM_OPTION_CRC_ENABLED:
        if (optlen != sizeof(int32_t)) {
            return 0;
        }

        g_opt.crcenabled = *value != 0;
        break;
    case STORM_OPTION_ORDERED_PRINTF_ENABLED:
        if (optlen != sizeof(int32_t)) {
            return 0;
        }

        g_opt.orderedprintfenabled = *value != 0;
        break;
    case STORM_OPTION_UNICODE_ENABLED:
        if (optlen != sizeof(int32_t)) {
            return 0;
        }

        g_opt.unicodeenabled = *value != 0;
        break;
    case STORM_OPTION_UNK_0024:
        if (optlen != sizeof(int32_t)) {
            return 0;
        }

        g_opt.unused_0024 = *value != 0;
        break;
    case STORM_OPTION_UNK_0028:
        if (optlen != sizeof(int32_t)) {
            return 0;
        }

        g_opt.unk_0028 = *value != 0;
        break;
    default:
        return 0;
    }

    SErrSetLastError(STORM_ERROR_SUCCESS);
    return 1;
}
