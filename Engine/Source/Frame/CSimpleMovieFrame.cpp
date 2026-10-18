#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#if defined(__APPLE__)
#include <alloca.h>
#else
#include <malloc.h>
#endif
#include "Engine/Source/Frame/CSimpleMovieFrame.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Frame/CRenderBatch.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxDevice/Types.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "ThirdParty/DivxDecoder/DivxDecoder.h"

// STATIC: 0x00B2CD48
int32_t CSimpleMovieFrame::s_metatable = LUA_NOREF;

// STATIC: 0x00DCE3D8
int32_t CSimpleMovieFrame::s_objectType;

// STATIC: 0x00B2CAEC
const char* CSimpleMovieFrame::s_objectTypeName = "MovieFrame";

// STATIC: 0x00DCE3DC
CSimpleMovieFrame* CSimpleMovieFrame::s_currentMovieFrame;

// STATIC: 0x00DCE3E0
int32_t CSimpleMovieFrame::s_decoderCount;

// STATIC: 0x00DCE3E4
CDataAllocator CSimpleMovieFrame::s_allocator(sizeof(CSimpleMovieFrame), 5);

// STATIC: 0x00A9E8B8
static uint32_t s_textureCount[6] = { 6, 2, 3, 4, 6, 2 };

// STATIC: 0x00A9E8D0
static uint32_t s_frameWidth[6] = { 800, 1024, 800, 1024, 800, 1024 };

// STATIC: 0x00A9E8E8
static uint32_t s_frameHeight[6] = { 384, 512, 512, 576, 384, 512 };

// STATIC: 0x00A9E900
static MOVIETEXTURERECT s_textureRects[6][6] = {
    {
        { 0.0f, 0.64f, 0.11f, -0.33f },
        { 0.64f, 0.96f, 0.11f, -0.33f },
        { 0.96f, 1.0f, 0.11f, -0.33f },
        { 0.0f, 0.64f, 0.33f, 0.11f },
        { 0.64f, 0.96f, 0.33f, 0.11f },
        { 0.96f, 1.0f, 0.33f, 0.11f },
    },
    {
        { 0.0f, 0.5f, 0.333f, -0.333f },
        { 0.5f, 1.0f, 0.333f, -0.333f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
    },
    {
        { 0.0f, 0.64f, 0.42f, -0.42f },
        { 0.64f, 0.96f, 0.42f, -0.42f },
        { 0.96f, 1.0f, 0.42f, -0.42f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
    },
    {
        { 0.0f, 0.5f, 0.3266f, -0.42f },
        { 0.5f, 1.0f, 0.3266f, -0.42f },
        { 0.0f, 0.5f, 0.42f, 0.3266f },
        { 0.5f, 1.0f, 0.42f, 0.3266f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
    },
    {
        { 0.0f, 0.64f, 0.11f, -0.33f },
        { 0.64f, 0.96f, 0.11f, -0.33f },
        { 0.96f, 1.0f, 0.11f, -0.33f },
        { 0.0f, 0.64f, 0.33f, 0.11f },
        { 0.64f, 0.96f, 0.33f, 0.11f },
        { 0.96f, 1.0f, 0.33f, 0.11f },
    },
    {
        { 0.0f, 0.5f, 0.333f, -0.333f },
        { 0.5f, 1.0f, 0.333f, -0.333f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
    },
};

// STATIC: 0x00A9EB40
static MOVIETEXTUREINFO s_textureInfo[6][6] = {
    {
        { 512, 256, 0, 3200 },
        { 256, 256, 2048, 3200 },
        { 32, 256, 3072, 3200 },
        { 512, 128, 819200, 3200 },
        { 256, 128, 821248, 3200 },
        { 32, 128, 822272, 3200 },
    },
    {
        { 512, 512, 0, 4096 },
        { 512, 512, 2048, 4096 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
    },
    {
        { 512, 512, 0, 3200 },
        { 256, 512, 2048, 3200 },
        { 32, 512, 3072, 3200 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
    },
    {
        { 512, 512, 0, 4096 },
        { 512, 512, 2048, 4096 },
        { 512, 64, 2097152, 4096 },
        { 512, 64, 2099200, 4096 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
    },
    {
        { 512, 256, 0, 3200 },
        { 256, 256, 2048, 3200 },
        { 32, 256, 3072, 3200 },
        { 512, 128, 819200, 3200 },
        { 256, 128, 821248, 3200 },
        { 32, 128, 822272, 3200 },
    },
    {
        { 512, 512, 0, 4096 },
        { 512, 512, 2048, 4096 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
    },
};

// STATIC: 0x00A9ED80
static MOVIEUPDATERECT s_updateRects[6][6] = {
    {
        { 0, 6, 512, 256 },
        { 0, 6, 256, 256 },
        { 0, 6, 32, 256 },
        { 0, 0, 512, 122 },
        { 0, 0, 256, 122 },
        { 0, 0, 32, 122 },
    },
    {
        { 0, 24, 512, 512 },
        { 0, 0, 512, 488 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
    },
    {
        { 0, 32, 512, 480 },
        { 0, 32, 256, 480 },
        { 0, 32, 32, 480 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
    },
    {
        { 0, 0, 512, 512 },
        { 0, 0, 512, 512 },
        { 0, 0, 512, 64 },
        { 0, 0, 512, 64 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
    },
    {
        { 0, 21, 512, 256 },
        { 0, 21, 256, 256 },
        { 0, 21, 32, 256 },
        { 0, 0, 512, 107 },
        { 0, 0, 256, 107 },
        { 0, 0, 32, 107 },
    },
    {
        { 0, 38, 512, 512 },
        { 0, 0, 512, 474 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
    },
};

// STATIC: 0x00A9EFC0
static uint32_t s_frameOffset[6] = { 0x4B00, 0x18000, 0x19000, 0, 0x10680, 0x26000 };

// STATIC: 0x00A24188
static uint16_t s_indices[4] = { 0, 1, 2, 3 };

// VERIFY: 6f6bd4ac auto
// OFFSET: 0x0095DB20
static void MovieSoundFinished(SESound* sound, void* param) {
    if (!sound->IsPlaying() && param) {
        static_cast<CSimpleMovieFrame*>(param)->m_finished = 1;
    }
}

// VERIFY: c631b86b auto
// OFFSET: 0x0095DB50
static void MovieTextureUpdate(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    if (command == GxTex_Latch) {
        MOVIETEXTUREDATA* data = static_cast<MOVIETEXTUREDATA*>(userArg);
        texels = data->m_buffer + data->m_info->m_offset;
        texelStrideInBytes = data->m_info->m_stride;
    }
}

// INLINED
void CSimpleMovieFrame::operator delete(void* ptr) {
    if (ptr) {
        CSimpleMovieFrame::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CSimpleMovieFrame::GetObjectType() {
    if (!CSimpleMovieFrame::s_objectType) {
        CSimpleMovieFrame::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleMovieFrame::s_objectType;
}

// VERIFY: 885d2767 auto
// OFFSET: 0x0095DAE0
void CSimpleMovieFrame::RunOnMovieShowSubtitleScript(const char* text) {
    if (this->m_onMovieShowSubtitle.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, text);
        this->RunScript(this->m_onMovieShowSubtitle, 1, nullptr);
    }
}

// VERIFY: e2201570 auto
// OFFSET: 0x0095DB80
FrameScript_Object::ScriptIx* CSimpleMovieFrame::GetScriptByName(const char* name, FrameScript_Object::ScriptData& data) {
    FrameScript_Object::ScriptIx* script = CSimpleFrame::GetScriptByName(name, data);

    if (script) {
        return script;
    }

    if (!SStrCmpI(name, "OnMovieFinished", STORM_MAX_STR)) {
        return &this->m_onMovieFinished;
    }

    if (!SStrCmpI(name, "OnMovieShowSubtitle", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,text) %s end";
        return &this->m_onMovieShowSubtitle;
    }

    if (!SStrCmpI(name, "OnMovieHideSubtitle", STORM_MAX_STR)) {
        return &this->m_onMovieHideSubtitle;
    }

    return nullptr;
}

// VERIFY: 177022f6 auto
// OFFSET: 0x0095DC10
void CSimpleMovieFrame::GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse) {
    FrameScript_Object::GetScriptTime(L, this->m_onMovieFinished, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onMovieShowSubtitle, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onMovieHideSubtitle, time, count);
    CSimpleFrame::GetScriptTime(L, time, count, recurse);
}

// VERIFY: be393b5d manual
// OFFSET: 0x0095DC80
int32_t CSimpleMovieFrame::ParseAVIFile(const char* fileName) {
    int32_t result = 0;
    SFileImpl* file = nullptr;
    char path[260];
    SStrPrintf(path, sizeof(path), "%s.avi", fileName);

    if (SFile::OpenEx(nullptr, path, 1, &file)) {
        char* header = nullptr;
        uint32_t headerSize = 0;
        char* index = nullptr;
        uint32_t indexSize = 0;
        uint32_t moviOffset = 0;
        char chunk[12];

        if (SFileReadFile(file, chunk, 12, nullptr, nullptr, 0) && !SStrCmpI(&chunk[0], "RIFF", 4) && !SStrCmpI(&chunk[8], "AVI ", 4)) {
            while (SFileReadFile(file, chunk, 8, nullptr, nullptr, 0)) {
                uint32_t size = *reinterpret_cast<uint32_t*>(&chunk[4]);

                if (!SStrCmpI(&chunk[0], "LIST", 4)) {
                    if (!SFileReadFile(file, chunk, 4, nullptr, nullptr, 0)) {
                        break;
                    }

                    size -= 4;

                    if (!SStrCmpI(&chunk[0], "hdrl", 4)) {
                        headerSize = size;
                        header = static_cast<char*>(alloca(size));

                        if (!SFileReadFile(file, header, size, nullptr, nullptr, 0)) {
                            break;
                        }
                    } else if (!SStrCmpI(&chunk[0], "movi", 4)) {
                        moviOffset = SFile::SetFilePointer(file, 0, nullptr, 1);
                        SFile::SetFilePointer(file, size, nullptr, 1);
                    } else {
                        SFile::SetFilePointer(file, size, nullptr, 1);
                    }
                } else if (!SStrCmpI(&chunk[0], "idx1", 4)) {
                    indexSize = size;
                    index = static_cast<char*>(alloca(size));

                    if (!SFileReadFile(file, index, size, nullptr, nullptr, 0)) {
                        break;
                    }
                } else {
                    SFile::SetFilePointer(file, size, nullptr, 1);
                }
            }

            int32_t streamIndex = 0;
            int32_t videoStream = -1;
            int32_t audioStream = -1;

            for (uint32_t offset = 0; offset < headerSize;) {
                if (!SStrCmpI(&header[offset], "LIST", 4)) {
                    offset += 12;
                    continue;
                }

                uint32_t chunkSize = *reinterpret_cast<uint32_t*>(&header[offset + 4]);

                if (!SStrCmpI(&header[offset], "strh", 4)) {
                    offset += 8;

                    if (!SStrCmpI(&header[offset], "vids", 4)) {
                        this->m_fps = static_cast<float>(*reinterpret_cast<uint32_t*>(&header[offset + 24])) / static_cast<float>(*reinterpret_cast<uint32_t*>(&header[offset + 20]));
                        videoStream = streamIndex;
                        this->m_frameCount = *reinterpret_cast<int32_t*>(&header[offset + 32]);
                    }

                    if (!SStrCmpI(&header[offset], "auds", 4)) {
                        audioStream = streamIndex;
                    }

                    streamIndex++;
                } else {
                    int32_t isFormat = !SStrCmpI(&header[offset], "strf", 4);
                    offset += 8;

                    if (isFormat && videoStream >= 0 && audioStream < 0) {
                        this->m_width = *reinterpret_cast<int32_t*>(&header[offset + 4]);
                        this->m_height = *reinterpret_cast<int32_t*>(&header[offset + 8]);
                    }
                }

                offset += chunkSize;
            }

            this->m_videoDataSize = 0;
            this->m_audioDataSize = 0;

            for (uint32_t i = 0; i < indexSize; i += 16) {
                if (index[i + 2] == 'd') {
                    this->m_videoDataSize += *reinterpret_cast<uint32_t*>(&index[i + 12]) + 4;
                }

                if (index[i + 2] == 'w') {
                    this->m_audioDataSize += *reinterpret_cast<uint32_t*>(&index[i + 12]);
                }
            }

            this->m_videoData = static_cast<uint8_t*>(SMemAlloc(this->m_videoDataSize, __FILE__, __LINE__, 0));
            this->m_audioData = static_cast<uint8_t*>(SMemAlloc(this->m_audioDataSize, __FILE__, __LINE__, 0));

            uint8_t* video = this->m_videoData;
            uint8_t* audio = this->m_audioData;

            for (uint32_t i = 0; i < indexSize; i += 16) {
                if (index[i + 2] == 'd') {
                    uint32_t size = *reinterpret_cast<uint32_t*>(&index[i + 12]);
                    *reinterpret_cast<uint32_t*>(video) = size;
                    video += 4;
                    SFile::SetFilePointer(file, *reinterpret_cast<uint32_t*>(&index[i + 8]) + moviOffset + 4, nullptr, 0);
                    SFileReadFile(file, video, size, nullptr, nullptr, 0);
                    video += size;
                }

                if (index[i + 2] == 'w') {
                    uint32_t size = *reinterpret_cast<uint32_t*>(&index[i + 12]);
                    SFile::SetFilePointer(file, *reinterpret_cast<uint32_t*>(&index[i + 8]) + moviOffset + 4, nullptr, 0);
                    SFileReadFile(file, audio, size, nullptr, nullptr, 0);
                    audio += size;
                }
            }

            this->m_videoPos = this->m_videoData;
            result = 1;
        }
    }

    if (file) {
        SFileCloseFile(file);
    }

    return result;
}

// VERIFY: db81495e manual
// OFFSET: 0x0095E090
void CSimpleMovieFrame::CloseVideo() {
    if (this->m_decoder) {
        UnInitializeDivxDecoder(this->m_decoder);
        CSimpleMovieFrame::s_decoderCount--;
        this->m_decoder = 0;
    }

    if (this->m_frameBuffer) {
        SMemFree(this->m_frameBuffer, __FILE__, __LINE__, 0);
        this->m_frameBuffer = nullptr;
    }

    if (this->m_videoData) {
        SMemFree(this->m_videoData, __FILE__, __LINE__, 0);
        this->m_videoData = nullptr;
    }

    for (uint32_t i = 0; i < 6; i++) {
        if (this->m_textures[i]) {
            GxTexDestroy(this->m_textures[i]);
            this->m_textures[i] = nullptr;
        }
    }
}

// VERIFY: 8fc9b180 auto
// OFFSET: 0x0095E130
void CSimpleMovieFrame::AudioDeviceChanged_ShutDown() {
    if (CSimpleMovieFrame::s_currentMovieFrame) {
        CSimpleMovieFrame::s_currentMovieFrame->m_audioRestart = 1;
        CSimpleMovieFrame::s_currentMovieFrame->m_sound.SetFinishedCallback2(nullptr, nullptr);
        CSimpleMovieFrame::s_currentMovieFrame->m_sound.Stop(1, -1.0f);
    }
}

// VERIFY: 7f50dcb6 manual
// OFFSET: 0x0095E170
void CSimpleMovieFrame::CloseAudio() {
    CSimpleMovieFrame::s_currentMovieFrame = nullptr;
    this->m_sound.Stop(1, -1.0f);

    if (this->m_audioData) {
        SMemFree(this->m_audioData, __FILE__, __LINE__, 0);
        this->m_audioData = nullptr;
    }
}

// VERIFY: 90a9a30a manual
// OFFSET: 0x0095E1C0
bool CSimpleMovieFrame::DecodeFrame(bool render) {
    uint32_t size = *reinterpret_cast<uint32_t*>(this->m_videoPos);
    this->m_videoPos += 4;

    DivxDecodeFrame frame = {};
    frame.m_output = this->m_frameBuffer + s_frameOffset[this->m_layout];
    frame.m_input = this->m_videoPos;
    this->m_videoPos += size;
    frame.m_inputSize = size;
    frame.m_render = render;

    if (DivxDecode(this->m_decoder, &frame, 0)) {
        return false;
    }

    if (render) {
        for (uint32_t i = 0; i < s_textureCount[this->m_layout]; i++) {
            const MOVIEUPDATERECT& rect = s_updateRects[this->m_layout][i];
            GxTexUpdate(this->m_textures[i], rect.m_minX, rect.m_minY, rect.m_maxX, rect.m_maxY, 1);
        }
    }

    return true;
}

// VERIFY: df56a904 manual
// OFFSET: 0x0095E350
int32_t CSimpleMovieFrame::OpenVideo(const char* fileName) {
    int32_t result = 0;

    this->m_startTime = OsGetAsyncTimeMs();
    this->m_elapsedMs = 0;
    this->m_decoder = ++CSimpleMovieFrame::s_decoderCount;

    if (!InitializeDivxDecoder(this->m_decoder, this->m_width, this->m_height) && !SetOutputFormat(this->m_decoder, 1, this->m_width, this->m_height)) {
        int32_t layoutFound = 1;

        if (this->m_width == 1024) {
            if (this->m_height == 576) {
                this->m_layout = 3;
            } else if (this->m_height == 436) {
                this->m_layout = 5;
            } else {
                this->m_layout = 1;
            }
        } else if (this->m_width == 800) {
            if (this->m_height == 448) {
                this->m_layout = 2;
            } else if (this->m_height == 342 || this->m_height == 340) {
                this->m_layout = 4;
            } else {
                this->m_layout = 0;
            }
        } else {
            layoutFound = 0;
        }

        if (layoutFound) {
            this->m_frameBuffer = static_cast<uint8_t*>(SMemAlloc(4 * s_frameWidth[this->m_layout] * s_frameHeight[this->m_layout], __FILE__, __LINE__, 0));

            if (this->m_frameBuffer) {
                memset(this->m_frameBuffer, 0, 4 * s_frameWidth[this->m_layout] * s_frameHeight[this->m_layout]);

                int32_t created = 1;
                CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);

                for (uint32_t i = 0; i < s_textureCount[this->m_layout]; i++) {
                    this->m_textureData[i].m_info = &s_textureInfo[this->m_layout][i];
                    this->m_textureData[i].m_buffer = this->m_frameBuffer;
                    created &= GxTexCreate(s_textureInfo[this->m_layout][i].m_width, s_textureInfo[this->m_layout][i].m_height, GxTex_Argb8888, flags, &this->m_textureData[i], MovieTextureUpdate, this->m_textures[i]);
                }

                if (created) {
                    for (uint32_t i = 0; i < s_textureCount[this->m_layout]; i++) {
                        GxTexUpdate(this->m_textures[i], 0, 0, 0, 0, 1);
                    }

                    result = 1;
                }
            }
        }
    }

    this->m_targetFrame = 0;
    this->m_currentFrame = -1;
    this->m_droppedFrames = 0;
    this->m_frameOffset = 0;

    if (!result) {
        this->CloseVideo();
    }

    return result;
}

// VERIFY: 47b77bfc auto
// OFFSET: 0x0095E600
int32_t CSimpleMovieFrame::OpenAudio(const char* fileName, int32_t volume, uint32_t position) {
    CSimpleMovieFrame::s_currentMovieFrame = this;

    this->m_sound.PlaySoundFromMemory(reinterpret_cast<const char*>(this->m_audioData), this->m_audioDataSize, 0);
    this->m_sound.SetChannelGroup("MOVIE", false);
    this->m_sound.SetFinishedCallback2(MovieSoundFinished, this);

    float soundVolume = volume * 0.003921569f;

    if (soundVolume < 0.0f) {
        soundVolume = 0.0f;
    } else if (soundVolume >= 1.0f) {
        soundVolume = 1.0f;
    }

    this->m_sound.SetVolume(soundVolume);

    if (position) {
        this->m_sound.SetPosition(position);
    }

    this->m_sound.Play();
    return 1;
}

// VERIFY: 6961ca44 auto
// OFFSET: 0x0095E6A0
void CSimpleMovieFrame::AudioDeviceChanged_Restart() {
    if (CSimpleMovieFrame::s_currentMovieFrame && CSimpleMovieFrame::s_currentMovieFrame->m_audioRestart) {
        CSimpleMovieFrame::s_currentMovieFrame->OpenAudio(CSimpleMovieFrame::s_currentMovieFrame->m_filename, CSimpleMovieFrame::s_currentMovieFrame->m_volume, CSimpleMovieFrame::s_currentMovieFrame->m_elapsedMs);
        CSimpleMovieFrame::s_currentMovieFrame->m_audioRestart = 0;
    }
}

// VERIFY: 74016685 auto
// OFFSET: 0x0095E6E0
CSimpleMovieFrame::CSimpleMovieFrame(CSimpleFrame* parent) : CSimpleFrame(parent), m_isPlaying(0), m_finished(0), m_subtitlesEnabled(0), m_audioRestart(0), m_volume(100), m_decoder(0), m_frameBuffer(nullptr), m_videoData(nullptr), m_audioData(nullptr) {
    this->m_fps = 0.0f;
    this->m_width = 0;
    this->m_height = 0;
    this->m_frameCount = 0;

    for (uint32_t i = 0; i < 6; i++) {
        this->m_textures[i] = nullptr;
    }
}

// VERIFY: b35f404b auto
// OFFSET: 0x0095E7D0
const char* CSimpleMovieFrame::GetObjectTypeName() {
    return CSimpleMovieFrame::s_objectTypeName;
}

// VERIFY: 8e847d4e auto
// OFFSET: 0x0095E7E0
int32_t CSimpleMovieFrame::GetScriptMetaTable() {
    return CSimpleMovieFrame::s_metatable;
}

// VERIFY: e8219013 auto
// OFFSET: 0x0095E800
bool CSimpleMovieFrame::IsA(int32_t type) {
    return type == CSimpleMovieFrame::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 68c5960a auto
// OFFSET: 0x0095E840
bool CSimpleMovieFrame::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleMovieFrame::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: 9b40a6ef manual
// OFFSET: 0x0095E880
int32_t CSimpleMovieFrame::OpenCaptions(const char* fileName) {
    char path[260];
    SStrPrintf(path, sizeof(path), "%s.sbt", fileName);

    void* data;

    if (SFile::LoadFile(path, &data, nullptr, 1, nullptr)) {
        const char* text = static_cast<const char*>(data);

        if (*reinterpret_cast<const uint8_t*>(text) == 0xEF) {
            text += 3;
        }

        char line[1024];

        do {
            SStrTokenize(&text, line, sizeof(line), "\r\n", nullptr);

            if (!line[0]) {
                break;
            }

            if (line[0] == '0') {
                MOVIECAPTION* caption = this->m_captions.New();

                uint32_t time = 60 * SStrToInt(&line[0]);
                time = 60 * (time + SStrToInt(&line[3]));
                time = 1000 * (time + SStrToInt(&line[6]));
                caption->m_startTime = time + 41 * SStrToInt(&line[9]);

                time = 60 * SStrToInt(&line[14]);
                time = 60 * (time + SStrToInt(&line[17]));
                time = 1000 * (time + SStrToInt(&line[20]));
                caption->m_endTime = time + 41 * SStrToInt(&line[23]);

                SStrTokenize(&text, line, sizeof(line), "\r\n", nullptr);
                caption->m_text = SStrDupA(line, __FILE__, __LINE__);
            }
        } while (line[0] && *text);

        SFile::Unload(data);
    }

    this->m_currentCaption = -1;
    return 1;
}

// VERIFY: 87eb9e47 manual
// OFFSET: 0x0095EA90
void CSimpleMovieFrame::CloseCaptions() {
    if (this->m_currentCaption >= 0) {
        if (this->m_onMovieHideSubtitle.m_luaRef) {
            this->RunScript(this->m_onMovieHideSubtitle, 0, nullptr);
        }

        this->m_currentCaption = -1;
    }

    for (uint32_t i = 0; i < this->m_captions.Count(); i++) {
        SMemFree(this->m_captions.m_data[i].m_text, __FILE__, __LINE__, 0);
    }

    this->m_captions.Clear();
}

// VERIFY: 26d17828 auto
// OFFSET: 0x0095EB30
int32_t CSimpleMovieFrame::StartMovie(const char* fileName, int32_t volume) {
    if (!this->ParseAVIFile(fileName) || !this->OpenVideo(fileName)) {
        return 0;
    }

    SStrCopy(this->m_filename, fileName, sizeof(this->m_filename));
    this->m_volume = volume;
    this->OpenAudio(fileName, volume, 0);
    this->OpenCaptions(fileName);
    this->m_isPlaying = 1;
    return 1;
}

// VERIFY: cd05ebab auto
// OFFSET: 0x0095EBA0
void CSimpleMovieFrame::StopMovie() {
    if (!this->m_isPlaying) {
        return;
    }

    this->CloseVideo();
    this->CloseAudio();
    this->CloseCaptions();
    this->m_finished = 0;
    this->m_isPlaying = 0;

    if (this->m_onMovieFinished.m_luaRef) {
        this->RunScript(this->m_onMovieFinished, 0, nullptr);
    }
}

// VERIFY: e507cb91 manual
// OFFSET: 0x0095EBF0
bool CSimpleMovieFrame::UpdateTiming() {
    int32_t playing = this->m_sound.IsPlaying();

    if (playing) {
        this->m_elapsedMs = this->m_sound.GetPosition();
        this->m_startTime = OsGetAsyncTimeMs() - this->m_elapsedMs;
    } else {
        this->m_elapsedMs = OsGetAsyncTimeMs() - this->m_startTime;
    }

    int32_t frame = static_cast<int32_t>(this->m_elapsedMs * this->m_fps * 0.001f + 0.5f);

    if (playing) {
        int32_t audioFrame = static_cast<int32_t>(this->m_sound.GetPosition() * this->m_fps * 0.001f + 0.5f);
        int32_t drift = audioFrame - this->m_frameOffset - frame;
        int32_t absDrift = abs(drift);

        if (absDrift > 2 || (absDrift > 1 && this->m_frameOffset + frame == this->m_currentFrame)) {
            if (drift > 0) {
                this->m_frameOffset++;
            } else {
                this->m_frameOffset--;
            }
        }
    }

    int32_t targetFrame = frame + this->m_frameOffset;

    if (targetFrame <= this->m_currentFrame) {
        targetFrame = this->m_currentFrame;
    }

    this->m_targetFrame = targetFrame;

    if (targetFrame >= this->m_frameCount) {
        this->m_finished = 1;
    }

    if (this->m_finished) {
        this->StopMovie();
        return false;
    }

    if (targetFrame == this->m_currentFrame) {
        return false;
    }

    if (targetFrame != this->m_currentFrame + 1) {
        this->m_droppedFrames++;
    }

    while (this->m_currentFrame < this->m_targetFrame - 1) {
        this->DecodeFrame(false);
        this->m_currentFrame++;
    }

    if (!this->DecodeFrame(true)) {
        this->m_finished = 1;
    }

    this->m_currentFrame = this->m_targetFrame;

    if (this->m_currentCaption >= 0 && this->m_captions.m_data[this->m_currentCaption].m_endTime <= this->m_elapsedMs) {
        if (this->m_onMovieHideSubtitle.m_luaRef) {
            this->RunScript(this->m_onMovieHideSubtitle, 0, nullptr);
        }

        this->m_currentCaption = -1;
    }

    if (this->m_currentCaption < 0 && (this->m_subtitlesEnabled || !this->m_sound.IsPlaying())) {
        for (uint32_t i = 0; i < this->m_captions.Count(); i++) {
            MOVIECAPTION& caption = this->m_captions.m_data[i];

            if (caption.m_startTime <= this->m_elapsedMs && caption.m_endTime > this->m_elapsedMs) {
                this->RunOnMovieShowSubtitleScript(this->m_captions.m_data[i].m_text);
                this->m_currentCaption = i;
                break;
            }
        }
    }

    return true;
}

// VERIFY: f304956c auto
// OFFSET: [0x0095E7F0, 0x0095EE40, 0x0095F310]
CSimpleMovieFrame::~CSimpleMovieFrame() {
    if (this->m_isPlaying) {
        this->CloseVideo();
        this->CloseAudio();
        this->CloseCaptions();
        this->m_finished = 0;
        this->m_isPlaying = 0;
    }
}

// VERIFY: 12d84d1a manual
// OFFSET: 0x0095EED0
void CSimpleMovieFrame::Render() {
    float aspect = CoordinateGetAspectCompensation();

    float minX;
    float maxX;
    float minY;
    float maxY;
    float minZ;
    float maxZ;
    GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);
    GxXformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    CImVector clearColor;
    clearColor.b = 0;
    clearColor.g = 0;
    clearColor.r = 0;
    clearColor.a = 0xFF;
    GxSceneClear(3, clearColor);

    C44Matrix view;
    C44Matrix projection;
    GxuXformCreateOrtho(0.0f, 1.0f, -0.5f, 0.5f, 0.0f, 500.0f, projection);
    GxXformSetView(view);
    GxXformSetProjection(projection);

    // STATIC: 0x00DCE418
    static C3Vector s_normal(0.0f, 0.0f, 1.0f);

    // STATIC: 0x00DCE3F8
    static C2Vector s_texCoords[4] = { C2Vector(0.0f, 0.0f), C2Vector(1.0f, 0.0f), C2Vector(0.0f, 1.0f), C2Vector(1.0f, 1.0f) };

    g_theGxDevicePtr->RsPush();

    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_BlendingMode, GxBlend_Opaque);
    GxUpdateAlphaRef(GxRs_AlphaRef);

    for (uint32_t i = 0; i < s_textureCount[this->m_layout]; i++) {
        const MOVIETEXTURERECT& rect = s_textureRects[this->m_layout][i];

        C3Vector positions[4] = {
            C3Vector(rect.m_left, rect.m_bottom * aspect, 0.0f),
            C3Vector(rect.m_right, rect.m_bottom * aspect, 0.0f),
            C3Vector(rect.m_left, aspect * rect.m_top, 0.0f),
            C3Vector(rect.m_right, aspect * rect.m_top, 0.0f)
        };

        GxPrimLockVertexPtrs(4, positions, sizeof(C3Vector), &s_normal, 0, nullptr, 0, nullptr, 0, s_texCoords, sizeof(C2Vector), nullptr, 0);
        GxRsSet(GxRs_Texture0, this->m_textures[i]);
        GxDrawLockedElements(GxPrim_TriangleStrip, 4, s_indices);
        GxPrimUnlockVertexPtrs();
    }

    g_theGxDevicePtr->RsPop();
    GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
}

// VERIFY: 953adff9 auto
// OFFSET: 0x0095F2E0
void CSimpleMovieFrame::RenderMovie(void* param) {
    CSimpleMovieFrame* movieFrame = static_cast<CSimpleMovieFrame*>(param);

    if (movieFrame->m_isPlaying) {
        movieFrame->UpdateTiming();
        movieFrame->Render();
    }
}

// VERIFY: fa8ad1a0 auto
// OFFSET: 0x0095F340
void CSimpleMovieFrame::OnFrameRender(CRenderBatch* batch, uint32_t layer) {
    CSimpleFrame::OnFrameRender(batch, layer);

    if (layer == 2) {
        batch->QueueCallback(&CSimpleMovieFrame::RenderMovie, this);
    }
}
