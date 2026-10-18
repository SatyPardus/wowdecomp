#include "MoviePlayback.h"
#include "Engine/Source/Frame/CSimpleMovieFrame.h"
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/SoundEngine/SoundEngine.h"
#include "Engine/Source/SoundEngine/fmodex/apple/AppleOutput.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

MoviePlayback::MoviePlayback(const char* path) {
    if (SE2::IsInitialized()) throw std::runtime_error("Movie viewer requires its own sound engine");
    try {
        for (const char* name : {"Sound_EnableMode2", "Sound_EnableHardware", "Sound_DSPBufferSize", "Sound_EnableMixMode2"})
            if (!CVar::Lookup(name)) CVar::Register(name, "", 0, "0", nullptr, 7, false, nullptr, false);
        if (!CVar::Lookup("Sound_OutputQuality")) CVar::Register("Sound_OutputQuality", "", 0, "1", nullptr, 7, false, nullptr, false);
        int32_t channels = 32, driver = 0; m_audio = true;
        if (!SE2::Init(64, nullptr, 1, 0, &channels, &driver, "", nullptr, 0)) throw std::runtime_error("Movie sound initialization failed");
        CSimpleMovieFrame::CreateScriptMetaTable(); m_metadata = true;
        FrameXML_RegisterFactory("MovieFrame", Create_SimpleMovieFrame, false);
        const char* xml = R"(<Ui><MovieFrame name="AppleOriginalMovie" setAllPoints="true" frameStrata="FULLSCREEN" enableKeyboard="true">
            <Layers><Layer level="OVERLAY"><FontString name="AppleOriginalMovieSubtitle" inherits="GlueFontNormalHuge" hidden="true">
                <Size><AbsDimension x="700" y="100"/></Size>
                <Anchors><Anchor point="BOTTOM"><Offset><AbsDimension x="0" y="20"/></Offset></Anchor></Anchors>
            </FontString></Layer></Layers>
            <Scripts>
                <OnKeyUp>if key == "ESCAPE" or key == "SPACE" then self:StopMovie() end</OnKeyUp>
                <OnMovieFinished>self:Hide()</OnMovieFinished>
                <OnMovieShowSubtitle>AppleOriginalMovieSubtitle:SetText(text); AppleOriginalMovieSubtitle:Show()</OnMovieShowSubtitle>
                <OnMovieHideSubtitle>AppleOriginalMovieSubtitle:Hide()</OnMovieHideSubtitle>
            </Scripts>
        </MovieFrame></Ui>)";
        auto* tree = XMLTree_Load(xml, uint32_t(std::strlen(xml)));
        if (!tree) throw std::runtime_error("Cannot parse movie viewer XML");
        CStatus status;
        m_movie.reset(static_cast<CSimpleMovieFrame*>(FrameXML_CreateFrame(XMLTree_GetRoot(tree)->m_child, nullptr, &status)));
        XMLTree_Free(tree);
        if (!status.IsEmpty()) { char error[4096]; throw std::runtime_error(status.GetErrorStr(error, sizeof(error), STATUS_INFO, "\n")); }
        if (!m_movie || !m_movie->StartMovie(path, 64)) throw std::runtime_error("Cannot start original movie");
        m_movie->m_subtitlesEnabled = 1;
        if (!m_movie->m_sound.m_internal) throw std::runtime_error("Movie MP3 stream did not initialize");
        m_system = m_movie->m_sound.m_internal->m_system;
    } catch (...) { Close(); throw; }
}
MoviePlayback::~MoviePlayback() { Close(); }
void MoviePlayback::Close() {
    m_movie.reset(); m_system = nullptr;
    if (m_metadata) { CSimpleMovieFrame::DestroyScriptMetaTable(); m_metadata = false; }
    if (m_audio) { SE2::StopAllSounds(0); SE2::UnregisterHeartBeat(); SE2::ShutDown(0); m_audio = false; }
}
void MoviePlayback::SetActive(bool active) {
    if (active == m_active) return;
    if (FMODAppleSetActive(m_system, active) != FMOD_OK) throw std::runtime_error("Movie audio activity transition failed");
    if (active) m_movie->m_startTime += OsGetAsyncTimeMs() - m_pausedAt;
    else m_pausedAt = OsGetAsyncTimeMs();
    m_active = active;
}
void MoviePlayback::Report() const {
    FMODAppleOutputStats stats{};
    if (FMODAppleGetOutputStats(m_system, &stats) != FMOD_OK) throw std::runtime_error("Movie output failed");
    std::fprintf(stderr, "Original MovieFrame preview: decoded frame %d, movie time %u ms, %llu audio frames, %llu nonzero samples\n",
        m_movie->m_currentFrame, m_movie->m_elapsedMs, stats.frames, stats.nonzeroSamples);
}
