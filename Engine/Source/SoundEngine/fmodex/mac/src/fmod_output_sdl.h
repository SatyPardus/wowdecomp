#ifndef _FMOD_OUTPUT_SDL_H
#define _FMOD_OUTPUT_SDL_H

#include "fmod_settings.h"

#if defined(PLATFORM_MAC) && defined(FMOD_SUPPORT_COREAUDIO)

#include "fmod_outputi.h"
#include <SDL.h>

namespace FMOD
{
    class OutputSDL : public Output
    {
      private:
        SDL_AudioDeviceID mDevice;
        int               mOutputRate;
        int               mOutputChannels;
        FMOD_SOUND_FORMAT mOutputFormat;
        unsigned int      mPcmPosition;
        bool              mQuitAudio;

      public:
        static FMOD_OUTPUT_DESCRIPTION_EX *getDescriptionEx();

        FMOD_RESULT getNumDrivers(int *numdrivers);
        FMOD_RESULT getDriverName(int driver, char *name, int namelen);
        FMOD_RESULT getDriverCaps(int id, FMOD_CAPS *caps);
        FMOD_RESULT init(int selecteddriver, FMOD_INITFLAGS flags, int *outputrate, int outputchannels, FMOD_SOUND_FORMAT *outputformat, int dspbufferlength, int dspnumbuffers, void *extradriverdata);
        FMOD_RESULT close();
        FMOD_RESULT start();
        FMOD_RESULT stop();
        FMOD_RESULT getHandle(void **handle);
        FMOD_RESULT getPosition(unsigned int *pcm);

        static void SDLCALL audioCallback(void *userdata, Uint8 *stream, int len);

        static FMOD_RESULT F_CALLBACK getNumDriversCallback(FMOD_OUTPUT_STATE *output, int *numdrivers);
        static FMOD_RESULT F_CALLBACK getDriverNameCallback(FMOD_OUTPUT_STATE *output, int id, char *name, int namelen);
        static FMOD_RESULT F_CALLBACK getDriverCapsCallback(FMOD_OUTPUT_STATE *output, int id, FMOD_CAPS *caps);
        static FMOD_RESULT F_CALLBACK initCallback(FMOD_OUTPUT_STATE *output, int selecteddriver, FMOD_INITFLAGS flags, int *outputrate, int outputchannels, FMOD_SOUND_FORMAT *outputformat, int dspbufferlength, int dspnumbuffers, void *extradriverdata);
        static FMOD_RESULT F_CALLBACK closeCallback(FMOD_OUTPUT_STATE *output);
        static FMOD_RESULT F_CALLBACK startCallback(FMOD_OUTPUT_STATE *output);
        static FMOD_RESULT F_CALLBACK stopCallback(FMOD_OUTPUT_STATE *output);
        static FMOD_RESULT F_CALLBACK getHandleCallback(FMOD_OUTPUT_STATE *output, void **handle);
        static FMOD_RESULT F_CALLBACK getPositionCallback(FMOD_OUTPUT_STATE *output, unsigned int *pcm);
    };
}

#endif

#endif
