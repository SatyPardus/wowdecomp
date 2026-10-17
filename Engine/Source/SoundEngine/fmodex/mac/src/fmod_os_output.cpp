#if defined(__APPLE__)

#include "fmod_settings.h"

#include "fmod.h"
#include "fmod_memory.h"
#include "fmod_output_nosound.h"
#include "fmod_output_nosound_nrt.h"
#include "fmod_output_sdl.h"
#include "fmod_output_wavwriter.h"
#include "fmod_output_wavwriter_nrt.h"
#include "fmod_pluginfactory.h"

FMOD_RESULT FMOD_OS_Output_Register(FMOD::PluginFactory *pluginfactory)
{
    if (!pluginfactory)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

#ifdef FMOD_SUPPORT_COREAUDIO
    pluginfactory->registerOutput(FMOD::OutputSDL::getDescriptionEx());
#endif
#ifdef FMOD_SUPPORT_NOSOUND
    pluginfactory->registerOutput(FMOD::OutputNoSound::getDescriptionEx());
#endif
#ifdef FMOD_SUPPORT_NOSOUND_NRT
    pluginfactory->registerOutput(FMOD::OutputNoSound_NRT::getDescriptionEx());
#endif
#ifdef FMOD_SUPPORT_WAVWRITER
    pluginfactory->registerOutput(FMOD::OutputWavWriter::getDescriptionEx());
#endif
#ifdef FMOD_SUPPORT_WAVWRITER_NRT
    pluginfactory->registerOutput(FMOD::OutputWavWriter_NRT::getDescriptionEx());
#endif

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Output_GetDefault(FMOD_OUTPUTTYPE *outputtype)
{
    if (!outputtype)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    *outputtype = FMOD_OUTPUTTYPE_COREAUDIO;
    return FMOD_OK;
}

#endif
