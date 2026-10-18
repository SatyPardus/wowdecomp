#include "ThirdParty/DivxDecoder/DivxDecoder.h"
#include <xvid.h>
#include <array>
#include <cstring>
#include <mutex>
#include <vector>

namespace {
struct Slot {
    void* handle = nullptr;
    int32_t width = 0, height = 0;
    std::vector<uint8_t> packet;
    ~Slot() { if (handle) xvid_decore(handle, XVID_DEC_DESTROY, nullptr, nullptr); }
};
std::array<Slot, 16> slots;
std::mutex mutex;
bool initialized = false;
bool Valid(int32_t decoder) { return decoder > 0 && size_t(decoder) < slots.size(); }
}

int32_t InitializeDivxDecoder(int32_t decoder, int32_t width, int32_t height) {
    if (!Valid(decoder) || width <= 0 || height <= 0 || width > 4096 || height > 4096) return 1;
    std::lock_guard<std::mutex> lock(mutex);
    if (!initialized) {
        xvid_gbl_init_t init{}; init.version = XVID_VERSION;
        if (xvid_global(nullptr, XVID_GBL_INIT, &init, nullptr) < 0) return 1;
        initialized = true;
    }
    auto& slot = slots[decoder];
    if (slot.handle) return 1; // A live decoder must never be replaced by another movie.
    xvid_dec_create_t create{};
    create.version = XVID_VERSION; create.width = width; create.height = height;
    // Nonzero dimensions enable Xvid's fixed-dimension validation of later VOLs.
    if (xvid_decore(nullptr, XVID_DEC_CREATE, &create, nullptr) < 0) return 1;
    slot.handle = create.handle; slot.width = width; slot.height = height;
    return 0;
}
int32_t SetOutputFormat(int32_t decoder, int32_t format, int32_t width, int32_t height) {
    std::lock_guard<std::mutex> lock(mutex);
    return !Valid(decoder) || !slots[decoder].handle || format != 1 ||
        width != slots[decoder].width || height != slots[decoder].height;
}
int32_t DivxDecode(int32_t decoder, DivxDecodeFrame* frame, int32_t flags) {
    if (!Valid(decoder) || !frame || flags || !frame->m_input ||
        frame->m_inputSize > 16 * 1024 * 1024 || (frame->m_render && !frame->m_output)) return 1;
    std::lock_guard<std::mutex> lock(mutex);
    auto& slot = slots[decoder];
    if (!slot.handle) return 1;
    // AVI empty packets repeat the preceding picture without resetting references.
    if (!frame->m_inputSize) return 0;
    try {
        // The codec uses aligned word lookahead. Own and zero-pad each packet;
        // never let it read the following AVI packet or the caller's allocation.
        slot.packet.resize(size_t(frame->m_inputSize) + 64);
        std::memcpy(slot.packet.data(), frame->m_input, frame->m_inputSize);
        std::memset(slot.packet.data() + frame->m_inputSize, 0, 64);
    } catch (...) { return 1; }
    size_t offset = 0;
    while (offset < frame->m_inputSize) {
        xvid_dec_frame_t decode{}; xvid_dec_stats_t stats{};
        decode.version = stats.version = XVID_VERSION;
        decode.general = XVID_LOWDELAY;
        decode.bitstream = slot.packet.data() + offset;
        decode.length = int(frame->m_inputSize - offset);
        decode.output.csp = frame->m_render ? XVID_CSP_BGRA | XVID_CSP_VFLIP : XVID_CSP_NULL;
        decode.output.plane[0] = frame->m_output; decode.output.stride[0] = slot.width * 4;
        const int consumed = xvid_decore(slot.handle, XVID_DEC_DECODE, &decode, &stats);
        if (consumed <= 0 || size_t(consumed) > frame->m_inputSize - offset) return 1;
        offset += size_t(consumed);
        // A VOL is metadata; continue to the image in this same AVI packet.
        if (stats.type != XVID_TYPE_VOL) return 0;
        if (stats.data.vol.width != slot.width || stats.data.vol.height != slot.height) return 1;
    }
    return 0;
}
int32_t UnInitializeDivxDecoder(int32_t decoder) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!Valid(decoder) || !slots[decoder].handle) return 1;
    auto& slot = slots[decoder];
    xvid_decore(slot.handle, XVID_DEC_DESTROY, nullptr, nullptr);
    slot.handle = nullptr; slot.width = slot.height = 0;
    std::vector<uint8_t>().swap(slot.packet);
    return 0;
}
