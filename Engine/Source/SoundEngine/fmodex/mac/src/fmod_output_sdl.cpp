#if defined(__APPLE__)

#include "fmod_settings.h"

#if defined(PLATFORM_MAC) && defined(FMOD_SUPPORT_COREAUDIO)

#include "fmod_output_sdl.h"
#include "fmod_memory.h"
#include "fmod_string.h"
#include "fmod_systemi.h"

#include <stdint.h>

namespace FMOD
{

FMOD_OUTPUT_DESCRIPTION_EX sdloutput;

static unsigned int OutputSDL_BytesPerSample(FMOD_SOUND_FORMAT format)
{
    switch (format)
    {
        case FMOD_SOUND_FORMAT_PCM16:    return 2;
        case FMOD_SOUND_FORMAT_PCMFLOAT: return 4;
        default:                         return 0;
    }
}

FMOD_OUTPUT_DESCRIPTION_EX *OutputSDL::getDescriptionEx()
{
    FMOD_memset(&sdloutput, 0, sizeof(FMOD_OUTPUT_DESCRIPTION_EX));

    sdloutput.name          = "FMOD SDL2 Output";
    sdloutput.version       = 0x00010100;
    sdloutput.polling       = false;
    sdloutput.getnumdrivers = &OutputSDL::getNumDriversCallback;
    sdloutput.getdrivername = &OutputSDL::getDriverNameCallback;
    sdloutput.getdrivercaps = &OutputSDL::getDriverCapsCallback;
    sdloutput.init          = &OutputSDL::initCallback;
    sdloutput.close         = &OutputSDL::closeCallback;
    sdloutput.start         = &OutputSDL::startCallback;
    sdloutput.stop          = &OutputSDL::stopCallback;
    sdloutput.gethandle     = &OutputSDL::getHandleCallback;
    sdloutput.getposition   = &OutputSDL::getPositionCallback;

    sdloutput.mType         = FMOD_OUTPUTTYPE_COREAUDIO;
    sdloutput.mSize         = sizeof(OutputSDL);

    return &sdloutput;
}

FMOD_RESULT OutputSDL::getNumDrivers(int *numdrivers)
{
    if (!numdrivers)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
    {
        *numdrivers = 0;
        return FMOD_OK;
    }

    int count = SDL_GetNumAudioDevices(0);
    *numdrivers = count < 0 ? 0 : count;

    return FMOD_OK;
}

FMOD_RESULT OutputSDL::getDriverName(int driver, char *name, int namelen)
{
    if (!name || namelen <= 0)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    const char *drivername = 0;
    if (SDL_WasInit(SDL_INIT_AUDIO) || SDL_InitSubSystem(SDL_INIT_AUDIO) == 0)
    {
        int count = SDL_GetNumAudioDevices(0);
        if (driver >= 0 && driver < count)
        {
            drivername = SDL_GetAudioDeviceName(driver, 0);
        }
    }

    FMOD_strncpy(name, drivername ? drivername : "Default SDL Audio Device", namelen);

    return FMOD_OK;
}

FMOD_RESULT OutputSDL::getDriverCaps(int id, FMOD_CAPS *caps)
{
    if (!caps)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    *caps = (FMOD_CAPS)(*caps | FMOD_CAPS_OUTPUT_MULTICHANNEL);
    *caps = (FMOD_CAPS)(*caps | FMOD_CAPS_OUTPUT_FORMAT_PCM16);
    *caps = (FMOD_CAPS)(*caps | FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT);

    return FMOD_OK;
}

FMOD_RESULT OutputSDL::init(int selecteddriver, FMOD_INITFLAGS flags, int *outputrate, int outputchannels, FMOD_SOUND_FORMAT *outputformat, int dspbufferlength, int dspnumbuffers, void *extradriverdata)
{
    (void)flags;
    (void)dspnumbuffers;
    (void)extradriverdata;

    if (!outputrate || !outputformat || outputchannels <= 0)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    Plugin::init();

    mDevice = 0;
    mOutputRate = *outputrate > 0 ? *outputrate : 48000;
    mOutputChannels = outputchannels;
    mOutputFormat = FMOD_SOUND_FORMAT_PCMFLOAT;
    mPcmPosition = 0;
    mQuitAudio = (SDL_WasInit(SDL_INIT_AUDIO) == 0);

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
    {
        return FMOD_ERR_OUTPUT_INIT;
    }

    const char *devicename = 0;
    int count = SDL_GetNumAudioDevices(0);
    if (selecteddriver >= 0 && selecteddriver < count)
    {
        devicename = SDL_GetAudioDeviceName(selecteddriver, 0);
    }

    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    SDL_zero(desired);
    SDL_zero(obtained);

    desired.freq = mOutputRate;
    desired.format = AUDIO_F32SYS;
    desired.channels = (Uint8)mOutputChannels;
    desired.samples = dspbufferlength > 0 ? (Uint16)dspbufferlength : 1024;
    desired.callback = &OutputSDL::audioCallback;
    desired.userdata = this;

    mDevice = SDL_OpenAudioDevice(devicename, 0, &desired, &obtained, 0);
    if (!mDevice)
    {
        desired.format = AUDIO_S16SYS;
        mDevice = SDL_OpenAudioDevice(devicename, 0, &desired, &obtained, 0);
        if (!mDevice)
        {
            if (mQuitAudio)
            {
                SDL_QuitSubSystem(SDL_INIT_AUDIO);
            }
            return FMOD_ERR_OUTPUT_INIT;
        }
        mOutputFormat = FMOD_SOUND_FORMAT_PCM16;
    }

    mOutputRate = obtained.freq;
    mOutputChannels = obtained.channels;
    *outputrate = obtained.freq;
    *outputformat = mOutputFormat;

    return FMOD_OK;
}

FMOD_RESULT OutputSDL::close()
{
    if (mDevice)
    {
        SDL_PauseAudioDevice(mDevice, 1);
        SDL_CloseAudioDevice(mDevice);
        mDevice = 0;
    }

    if (mQuitAudio)
    {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        mQuitAudio = false;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputSDL::start()
{
    if (!mDevice)
    {
        return FMOD_ERR_OUTPUT_INIT;
    }

    SDL_PauseAudioDevice(mDevice, 0);
    return FMOD_OK;
}

FMOD_RESULT OutputSDL::stop()
{
    if (mDevice)
    {
        SDL_PauseAudioDevice(mDevice, 1);
    }

    return FMOD_OK;
}

FMOD_RESULT OutputSDL::getHandle(void **handle)
{
    if (!handle)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    *handle = (void *)(uintptr_t)mDevice;
    return FMOD_OK;
}

FMOD_RESULT OutputSDL::getPosition(unsigned int *pcm)
{
    if (!pcm)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    *pcm = mPcmPosition;
    return FMOD_OK;
}

void SDLCALL OutputSDL::audioCallback(void *userdata, Uint8 *stream, int len)
{
    OutputSDL *output = (OutputSDL *)userdata;
    if (!output || !stream || len <= 0)
    {
        return;
    }

    unsigned int bytespersample = OutputSDL_BytesPerSample(output->mOutputFormat);
    if (!bytespersample || output->mOutputChannels <= 0)
    {
        SDL_memset(stream, 0, len);
        return;
    }

    unsigned int samples = (unsigned int)len / (bytespersample * (unsigned int)output->mOutputChannels);
    if (!samples || !output->readfrommixer || output->readfrommixer(output, stream, samples) != FMOD_OK)
    {
        SDL_memset(stream, 0, len);
        return;
    }

    output->mPcmPosition += samples;
}

FMOD_RESULT F_CALLBACK OutputSDL::getNumDriversCallback(FMOD_OUTPUT_STATE *output, int *numdrivers)
{
    return ((OutputSDL *)output)->getNumDrivers(numdrivers);
}

FMOD_RESULT F_CALLBACK OutputSDL::getDriverNameCallback(FMOD_OUTPUT_STATE *output, int id, char *name, int namelen)
{
    return ((OutputSDL *)output)->getDriverName(id, name, namelen);
}

FMOD_RESULT F_CALLBACK OutputSDL::getDriverCapsCallback(FMOD_OUTPUT_STATE *output, int id, FMOD_CAPS *caps)
{
    return ((OutputSDL *)output)->getDriverCaps(id, caps);
}

FMOD_RESULT F_CALLBACK OutputSDL::initCallback(FMOD_OUTPUT_STATE *output, int selecteddriver, FMOD_INITFLAGS flags, int *outputrate, int outputchannels, FMOD_SOUND_FORMAT *outputformat, int dspbufferlength, int dspnumbuffers, void *extradriverdata)
{
    return ((OutputSDL *)output)->init(selecteddriver, flags, outputrate, outputchannels, outputformat, dspbufferlength, dspnumbuffers, extradriverdata);
}

FMOD_RESULT F_CALLBACK OutputSDL::closeCallback(FMOD_OUTPUT_STATE *output)
{
    return ((OutputSDL *)output)->close();
}

FMOD_RESULT F_CALLBACK OutputSDL::startCallback(FMOD_OUTPUT_STATE *output)
{
    return ((OutputSDL *)output)->start();
}

FMOD_RESULT F_CALLBACK OutputSDL::stopCallback(FMOD_OUTPUT_STATE *output)
{
    return ((OutputSDL *)output)->stop();
}

FMOD_RESULT F_CALLBACK OutputSDL::getHandleCallback(FMOD_OUTPUT_STATE *output, void **handle)
{
    return ((OutputSDL *)output)->getHandle(handle);
}

FMOD_RESULT F_CALLBACK OutputSDL::getPositionCallback(FMOD_OUTPUT_STATE *output, unsigned int *pcm)
{
    return ((OutputSDL *)output)->getPosition(pcm);
}

}

#endif
#endif
