#include "Engine/Source/Frame/CSimpleMovieFrame.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxTex.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/SoundEngine/SoundEngine.h"
#include "Engine/Source/SoundEngine/fmodex/apple/AppleOutput.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
std::string MoviePath(const char* name) {
    return std::string("Interface\\Cinematics\\") + name;
}
std::unique_ptr<CSimpleMovieFrame> Create() {
    auto* memory = CSimpleMovieFrame::s_allocator.GetData(1, __FILE__, __LINE__);
    return std::unique_ptr<CSimpleMovieFrame>(new (memory) CSimpleMovieFrame(nullptr));
}
void Script(const char* code) {
    auto* lua = FrameScript_GetContext();
    Require(!luaL_loadstring(lua, code) && !lua_pcall(lua, 0, 0, 0), "Movie Lua callback failed");
}
}
void CheckMovieFrame(CGxDeviceMetal& device) {
    device.RsPush(); const auto projection = device.m_projNative;
    for (auto transform : {GxXform_View, GxXform_World}) device.XformPushAndSet(transform, C44Matrix(1));
    CSimpleMovieFrame::CreateScriptMetaTable();
    struct Cleanup {
        CGxDeviceMetal& device; C44Matrix projection;
        ~Cleanup() {
            if (device.FrameCommand()) device.ScenePresent(); device.Drain();
            device.RsPop(); GxXformPop(GxXform_View); GxXformPop(GxXform_World); device.m_projNative = projection;
            CSimpleMovieFrame::DestroyScriptMetaTable();
        }
    } cleanup{device, projection};
    device.RsInit();
    for (auto state : {GxRs_DepthTest, GxRs_DepthWrite, GxRs_Culling, GxRs_Lighting, GxRs_Fog, GxRs_AlphaRef}) GxRsSet(state, 0);
    GxRsSet(GxRs_ColorWrite, 15);
    auto desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:640 height:480 mipmapped:NO];
    desc.storageMode = MTLStorageModePrivate; desc.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:desc];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    desc.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:desc];
    pass.depthAttachment.loadAction = MTLLoadActionClear; pass.depthAttachment.clearDepth = 1;
    auto read = [&](int w, int h) {
        CiRect rect(0, 0, h, w); TSGrowableArray<CImVector> pixels; GxReadPixels(rect, pixels);
        return std::vector<CImVector>(pixels.Ptr(), pixels.Ptr() + pixels.Count());
    };
    unsigned layouts = 0, textures = 0; size_t changed = 0;
    for (const char* name : {"Logo_800", "Logo_1024", "WOW_FotLK_800", "WOW_FotLK_1024", "WOW_Intro_BC_800", "WOW_Intro_BC_1024", "WOW_Intro_1024"}) {
        const auto path = MoviePath(name);
        auto movie = Create(); Require(movie->ParseAVIFile(path.c_str()) && movie->OpenVideo(path.c_str()), "Original MovieFrame open failed");
        layouts |= 1u << movie->m_layout;
        device.BeginFrame(pass); movie->Render(); const auto before = read(640, 480); device.ScenePresent();
        for (unsigned frame = 0; frame < 96; ++frame) Require(movie->DecodeFrame(frame == 0 || frame == 95), "Original MovieFrame decode failed");
        device.BeginFrame(pass); movie->Render(); const auto after = read(640, 480);
        for (size_t i = 0; i < after.size(); ++i) changed += std::memcmp(&after[i], &before[i], 4) != 0;
        for (unsigned i = 0; i < 6 && movie->m_textures[i]; ++i) {
            const auto& info = *movie->m_textureData[i].m_info;
            CGxTexFlags flags(GxTex_Nearest); flags.m_renderTarget = 1;
            CGxTex* copy = nullptr;
            Require(device.TexCreate(GxTex_2d, info.m_width, info.m_height, 0, GxTex_Argb8888, GxTex_Unknown,
                flags, nullptr, nullptr, "Movie decoded texture check", copy), "Movie texture copy create failed");
            struct Texture { CGxDeviceMetal& device; CGxTex* value; ~Texture() { device.TexDestroy(value); } } texture{device, copy};
            device.DeviceCopyTex(movie->m_textures[i], 0, copy, 0); GxRenderTargetSet(GxBuffers_Color, copy, 0);
            const auto pixels = read(info.m_width, info.m_height);
            for (unsigned y = 0; y < info.m_height; ++y) {
                const auto* expected = movie->m_frameBuffer + info.m_offset + y * info.m_stride;
                const auto* actual = reinterpret_cast<const uint8_t*>(pixels.data() + y * info.m_width);
                // Wow.exe 0xA9ED80: layout 1's right half updates [0,488).
                // zhCN Intro is 466 high (Logo is 464), so two decoded bottom
                // rows are intentionally outside the original upload rectangle.
                const bool retainedZero = movie->m_layout == 1 && i == 1 && y >= 488;
                for (unsigned x = 0; x < info.m_width * 4; ++x) if (actual[x] != (retainedZero ? 0 : expected[x]))
                    throw std::runtime_error(std::string(name) + " texture " + std::to_string(i) + " byte " + std::to_string(x) +
                        " row " + std::to_string(y) + ": GPU " + std::to_string(actual[x]) + " decoded " + std::to_string(expected[x]));
            }
            GxRenderTargetSet(GxBuffers_Color, nullptr, 0); ++textures;
        }
        device.ScenePresent();
        device.BeginFrame(pass); movie->Render(); const auto repeat = read(640, 480); device.ScenePresent();
        Require(after.size() == repeat.size() && !std::memcmp(after.data(), repeat.data(), after.size() * 4), "Movie rendering changed without a decoded frame");
    }
    Require(layouts == 63 && changed > 1000, "Original movie layouts did not draw visible frames");

    // Exercise the original asynchronous MP3 sound, clock and Lua finish path.
    for (const char* name : {"Sound_EnableMode2", "Sound_EnableHardware", "Sound_DSPBufferSize", "Sound_EnableMixMode2"})
        if (!CVar::Lookup(name)) CVar::Register(name, "", 0, "0", nullptr, 7, false, nullptr, false);
    if (!CVar::Lookup("Sound_OutputQuality")) CVar::Register("Sound_OutputQuality", "", 0, "1", nullptr, 7, false, nullptr, false);
    int32_t channels = 32, driver = 0;
    Require(SE2::Init(64, nullptr, 1, 0, &channels, &driver, "", nullptr, 0), "Movie sound engine initialization failed");
    struct Audio { ~Audio() { SE2::StopAllSounds(0); SE2::UnregisterHeartBeat(); SE2::ShutDown(0); } } audio;
    auto movie = Create(); movie->SetName("AppleMovieCheck");
    Script("AppleMovieCheck:SetScript('OnMovieFinished', function(self) self.finished = (self.finished or 0) + 1 end)");
    const auto path = MoviePath("Logo_1024");
    Require(movie->StartMovie(path.c_str(), 16), "Original movie start with audio failed");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (movie->m_elapsedMs < 120 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        device.BeginFrame(pass); EventProcessPending();
        if (!device.FrameCommand()) device.BeginFrame(pass);
        movie->UpdateTiming(); movie->Render();
        if (device.FrameCommand()) device.ScenePresent();
    }
    Require(movie->m_sound.IsPlaying() && movie->m_currentFrame > 0 && movie->m_elapsedMs >= 120, "Movie audio/video clock did not advance");
    CSimpleMovieFrame::AudioDeviceChanged_ShutDown();
    Require(movie->m_audioRestart && !movie->m_sound.IsPlaying(), "Movie sound restart did not stop old channel");
    CSimpleMovieFrame::AudioDeviceChanged_Restart();
    Require(!movie->m_audioRestart && movie->m_sound.IsPlaying(), "Movie audio did not restart at elapsed position");
    movie->StopMovie(); movie->StopMovie();
    Script("assert(AppleMovieCheck.finished == 1)");
    Require(!movie->StartMovie("/missing-original-movie", 0) && !movie->m_decoder && !movie->m_videoData && !movie->m_audioData,
        "Failed movie start retained resources");
    Require(movie->StartMovie(path.c_str(), 0), "Movie start after failure did not retry"); movie->StopMovie();
    Script("assert(AppleMovieCheck.finished == 2)");
    Script("AppleMovieCheck:SetScript('OnMovieShowSubtitle', function(self, text) self.caption = text end); "
           "AppleMovieCheck:SetScript('OnMovieHideSubtitle', function(self) self.caption = nil; self.captionHidden = true end)");
    const auto intro = MoviePath("WOW_Intro_1024");
    Require(movie->StartMovie(intro.c_str(), 0) && movie->m_captions.Count() > 0, "Original movie subtitles did not load from MPQ");
    movie->m_subtitlesEnabled = 1;
    // Use the original silent playback clock to reach authored boundaries
    // without waiting through the opening narration during every GPU check.
    movie->m_sound.SetFinishedCallback2(nullptr, nullptr); movie->m_sound.Stop(1, 0); movie->m_finished = 0;
    const auto& caption = movie->m_captions[0];
    movie->m_startTime = OsGetAsyncTimeMs() - caption.m_startTime - 1;
    Require(movie->UpdateTiming() && movie->m_currentCaption == 0, "Movie subtitle did not start on its authored clock");
    lua_getglobal(FrameScript_GetContext(), "AppleMovieCheck"); lua_getfield(FrameScript_GetContext(), -1, "caption");
    Require(lua_isstring(FrameScript_GetContext(), -1) && !std::strcmp(lua_tostring(FrameScript_GetContext(), -1), caption.m_text), "Movie subtitle Lua callback changed original text");
    lua_pop(FrameScript_GetContext(), 2);
    movie->m_startTime = OsGetAsyncTimeMs() - caption.m_endTime - 1;
    Require(movie->UpdateTiming(), "Movie clock stopped at subtitle end");
    Script("assert(AppleMovieCheck.captionHidden)"); movie->StopMovie();
    Script("assert(AppleMovieCheck.finished == 3)");
    Require(!CSimpleMovieFrame::s_decoderCount && !CSimpleMovieFrame::s_currentMovieFrame, "Movie teardown retained decoder/audio owner");
    std::fprintf(stderr, "Original MovieFrame: six layouts, %u exact GPU textures, %zu changed pixels; MP3 clock/restart, real subtitle timing/text, Lua finish, retry and cleanup passed\n", textures, changed);
}
