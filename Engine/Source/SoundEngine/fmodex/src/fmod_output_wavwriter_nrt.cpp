#include "fmod_settings.h"

#ifdef FMOD_SUPPORT_WAVWRITER_NRT

#include "fmod_memory.h"
#include "fmod_output_wavwriter_nrt.h"
#include "fmod_string.h"
#include "fmod_systemi.h"

namespace FMOD
{


FMOD_OUTPUT_DESCRIPTION_EX wavwriteroutput_nrt;

#ifdef PLUGIN_EXPORTS

#ifdef __cplusplus
extern "C" {
#endif

    /*
        FMODGetOutputDescription is mandantory for every fmod plugin.  This is the symbol the registerplugin function searches for.
        Must be declared with F_API to make it export as stdcall.
    */
    F_DECLSPEC F_DLLEXPORT FMOD_OUTPUT_DESCRIPTION_EX * F_API FMODGetOutputDescriptionEx()
    {
        return OutputWavWriter_NRT::getDescriptionEx();
    }

#ifdef __cplusplus
}
#endif

#endif  /* PLUGIN_EXPORTS */

static const FMOD_GUID _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = { 0x00000003, 0x0000, 0x0010, { 0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71 } };
static const FMOD_GUID _KSDATAFORMAT_SUBTYPE_PCM        = { 0x00000001, 0x0000, 0x0010, { 0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71 } };

/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
// VERIFY: a31e7c17 manual
// OFFSET: 0x00915700
FMOD_OUTPUT_DESCRIPTION_EX *OutputWavWriter_NRT::getDescriptionEx()
{
    FMOD_memset(&wavwriteroutput_nrt, 0, sizeof(FMOD_OUTPUT_DESCRIPTION));
    
    wavwriteroutput_nrt.name          = "FMOD WavWriter Output - Non real-time";
    wavwriteroutput_nrt.version       = 0x00010100;
    wavwriteroutput_nrt.polling       = false;
    wavwriteroutput_nrt.getnumdrivers = &OutputWavWriter_NRT::getNumDriversCallback;
    wavwriteroutput_nrt.getdrivername = &OutputWavWriter_NRT::getDriverNameCallback;
    wavwriteroutput_nrt.getdrivercaps = &OutputWavWriter_NRT::getDriverCapsCallback;
    wavwriteroutput_nrt.init          = &OutputWavWriter_NRT::initCallback;
    wavwriteroutput_nrt.close         = &OutputWavWriter_NRT::closeCallback;
    wavwriteroutput_nrt.start         = &OutputWavWriter_NRT::startCallback;
    wavwriteroutput_nrt.stop          = &OutputWavWriter_NRT::stopCallback;
    wavwriteroutput_nrt.update        = &OutputWavWriter_NRT::updateCallback;
    wavwriteroutput_nrt.gethandle     = &OutputWavWriter_NRT::getHandleCallback;

    /*
        Private members
    */
    wavwriteroutput_nrt.mType         = FMOD_OUTPUTTYPE_WAVWRITER_NRT;
    wavwriteroutput_nrt.mSize         = sizeof(OutputWavWriter_NRT);

    return &wavwriteroutput_nrt;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
FMOD_RESULT OutputWavWriter_NRT::getNumDrivers(int *numdrivers)
{
    *numdrivers = 1;

    return FMOD_OK;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
FMOD_RESULT OutputWavWriter_NRT::getDriverName(int driver, char *name, int namelen)
{
    FMOD_strncpy(name, "fmodoutput.wav", namelen);

    return FMOD_OK;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
FMOD_RESULT OutputWavWriter_NRT::getDriverCaps(int id, FMOD_CAPS *caps)
{
    *caps = (FMOD_CAPS)(*caps | FMOD_CAPS_OUTPUT_MULTICHANNEL);
    *caps = (FMOD_CAPS)(*caps | FMOD_CAPS_OUTPUT_FORMAT_PCM8);
    *caps = (FMOD_CAPS)(*caps | FMOD_CAPS_OUTPUT_FORMAT_PCM16);
    *caps = (FMOD_CAPS)(*caps | FMOD_CAPS_OUTPUT_FORMAT_PCM24);
    *caps = (FMOD_CAPS)(*caps | FMOD_CAPS_OUTPUT_FORMAT_PCM32);
    *caps = (FMOD_CAPS)(*caps | FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT);

    return FMOD_OK;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
// VERIFY: f36c7972 manual
// OFFSET: 0x00915450
FMOD_RESULT OutputWavWriter_NRT::init(int selecteddriver, FMOD_INITFLAGS flags, int *outputrate, int outputchannels, FMOD_SOUND_FORMAT *outputformat, int dspbufferlength, int dspnumbuffers, void *extradriverdata)
{
    FMOD_RESULT result;

    Plugin::init(); /* Set up gGlobal - for debug / file / memory access by this plugin. */

    result = mSystem->getSoftwareFormat(&mRate, &mFormat, &mChannels, 0, 0, &mBits);
    if (result != FMOD_OK)
    {
        return result;
    }
    
    mBufferLength = dspbufferlength;

    result = SoundI::getBytesFromSamples(mBufferLength, &mBufferLengthBytes, mChannels, mFormat);
    if (result != FMOD_OK)
    {
        return result;
    }

    mBuffer = FMOD_Memory_Calloc(mBufferLengthBytes);
    if (!mBuffer)
    {
        return FMOD_ERR_MEMORY;
    }

    if (!extradriverdata)
    {
        FMOD_strncpy(mFileName, "fmodoutput.wav", FMOD_STRING_MAXPATHLEN);
    }
    else
    {
        FMOD_strncpy(mFileName, (char *)extradriverdata, FMOD_STRING_MAXPATHLEN);
    }

    return FMOD_OK;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
FMOD_RESULT OutputWavWriter_NRT::close()
{
    if (mBuffer)
    {
        FMOD_Memory_Free(mBuffer);
        mBuffer = 0;
    }

    return FMOD_OK;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
FMOD_RESULT OutputWavWriter_NRT::getHandle(void **handle)
{
    *handle = mFP;

    return FMOD_OK;
}

	  
/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
FMOD_RESULT OutputWavWriter_NRT::update()
{
#ifdef FMOD_SUPPORT_SOFTWARE
    FMOD_RESULT     result  = FMOD_OK;

    result = mix(mBuffer, mBufferLength);
    if (result != FMOD_OK)
    {
        return FMOD_OK;
    }

    fwrite(mBuffer, mBufferLengthBytes, 1, mFP);
    mLengthBytes += mBufferLengthBytes;
#endif

    return FMOD_OK;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
// VERIFY: beeb42a0 manual
// OFFSET: 0x009150D0
FMOD_RESULT OutputWavWriter_NRT::writeWavHeader()
{
    if (!mFP)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    fseek(mFP, 0, SEEK_SET);

    {
        bool extensible = false;

        WAVE_CHUNK FmtHdr =
        {
            { 'f','m','t',' '}, 
            sizeof(WAVE_FORMATEXTENSIBLE)
        };
    
        if (mFormat == FMOD_SOUND_FORMAT_PCMFLOAT && mChannels > 2)
        {
            extensible = true;
        }

        WAVE_FORMATEXTENSIBLE Fmt =
        {
            {
                static_cast<uint16_t>(extensible ? WAVE_FORMAT_EXTENSIBLE : mFormat == FMOD_SOUND_FORMAT_PCMFLOAT ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM),
                static_cast<uint16_t>(mChannels),
                static_cast<uint32_t>(mRate),
                static_cast<uint32_t>(mRate * mChannels * mBits / 8),
                static_cast<uint16_t>(mChannels * mBits / 8),
                static_cast<uint16_t>(mBits),
                0
            },
            0
        };

        if (Fmt.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            Fmt.Format.cbSize               = 22;
            Fmt.Samples.wValidBitsPerSample = mBits;
            Fmt.dwChannelMask               = 0;      /* FIXME - this should be set according to mulchaud.rtf */
            if (mFormat == FMOD_SOUND_FORMAT_PCMFLOAT)
            {
                FMOD_memcpy(&Fmt.SubFormat, &_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, sizeof(FMOD_GUID));
            }
            else
            {
                FMOD_memcpy(&Fmt.SubFormat, &_KSDATAFORMAT_SUBTYPE_PCM, sizeof(FMOD_GUID));
            }        
        }

        WAVE_CHUNK DataChunk = 
        { 
            {'d','a','t','a'}, mLengthBytes       /* size is unknown at the moment so just make it huge */
        };

        WAVE_CHUNK WavHeader = 
        { 
            {'R','I','F','F'}, 
            static_cast<uint32_t>(sizeof(FmtHdr) + sizeof(Fmt) + sizeof(DataChunk) + mLengthBytes),
        };

#ifdef PLATFORM_ENDIAN_BIG
        FmtHdr.size    = FMOD_SWAPENDIAN_DWORD(FmtHdr.size);
        DataChunk.size = FMOD_SWAPENDIAN_DWORD(DataChunk.size);
        WavHeader.size = FMOD_SWAPENDIAN_DWORD(WavHeader.size);

        Fmt.Format.wFormatTag       = FMOD_SWAPENDIAN_WORD(Fmt.Format.wFormatTag);
        Fmt.Format.nChannels        = FMOD_SWAPENDIAN_WORD(Fmt.Format.nChannels);
        Fmt.Format.nSamplesPerSec   = FMOD_SWAPENDIAN_DWORD(Fmt.Format.nSamplesPerSec);
        Fmt.Format.nAvgBytesPerSec  = FMOD_SWAPENDIAN_DWORD(Fmt.Format.nAvgBytesPerSec);
        Fmt.Format.nBlockAlign      = FMOD_SWAPENDIAN_WORD(Fmt.Format.nBlockAlign);
        Fmt.Format.wBitsPerSample   = FMOD_SWAPENDIAN_WORD(Fmt.Format.wBitsPerSample);
        Fmt.Format.cbSize           = FMOD_SWAPENDIAN_WORD(Fmt.Format.cbSize);

        Fmt.Samples.wValidBitsPerSample = FMOD_SWAPENDIAN_WORD(Fmt.Samples.wValidBitsPerSample);
        Fmt.dwChannelMask               = FMOD_SWAPENDIAN_DWORD(Fmt.dwChannelMask);

        Fmt.SubFormat.Data1 = FMOD_SWAPENDIAN_DWORD(Fmt.SubFormat.Data1);
        Fmt.SubFormat.Data2 = FMOD_SWAPENDIAN_WORD(Fmt.SubFormat.Data2);
        Fmt.SubFormat.Data3 = FMOD_SWAPENDIAN_WORD(Fmt.SubFormat.Data3);
#endif

        int written = 0;
        written = fwrite(&WavHeader, sizeof(WavHeader), 1, mFP);
        written = fwrite("WAVE", 4, 1, mFP);
        written = fwrite(&FmtHdr, sizeof(FmtHdr), 1, mFP);
        written = fwrite(&Fmt, sizeof(Fmt), 1, mFP);
        written = fwrite(&DataChunk, sizeof(DataChunk), 1, mFP);
    }

    return FMOD_OK;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
FMOD_RESULT OutputWavWriter_NRT::start()
{
    FMOD_RESULT result;

    mFP = fopen(mFileName, "wb");
    if (!mFP)
    {
        return FMOD_ERR_FILE_NOTFOUND;
    }

    result = writeWavHeader();
    if (result != FMOD_OK)
    {
        return result;
    }

    return FMOD_OK;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
FMOD_RESULT OutputWavWriter_NRT::stop()
{
    writeWavHeader();

    if (mFP)
    {
        fclose(mFP);
        mFP = 0;
    }

    return FMOD_OK;
}

/*
    ==============================================================================================================

    CALLBACK INTERFACE

    ==============================================================================================================
*/


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::getNumDriversCallback(FMOD_OUTPUT_STATE *output, int *numdrivers)
{
    OutputWavWriter_NRT *wavwriter = (OutputWavWriter_NRT *)output;

    return wavwriter->getNumDrivers(numdrivers);
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::getDriverNameCallback(FMOD_OUTPUT_STATE *output, int id, char *name, int namelen)
{
    OutputWavWriter_NRT *wavwriter = (OutputWavWriter_NRT *)output;

    return wavwriter->getDriverName(id, name, namelen);
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
FMOD_RESULT OutputWavWriter_NRT::getDriverCapsCallback(FMOD_OUTPUT_STATE *output, int id, FMOD_CAPS *caps)
{
    OutputWavWriter_NRT *wavwriter = (OutputWavWriter_NRT *)output;

    return wavwriter->getDriverCaps(id, caps);
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
// VERIFY: b4db65ce auto
// OFFSET: 0x009156C0
FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::initCallback(FMOD_OUTPUT_STATE *output, int selecteddriver, FMOD_INITFLAGS flags, int *outputrate, int outputchannels, FMOD_SOUND_FORMAT *outputformat, int dspbufferlength, int dspnumbuffers, void *extradriverdata)
{
    OutputWavWriter_NRT *wavwriter = (OutputWavWriter_NRT *)output;

    return wavwriter->init(selecteddriver, flags, outputrate, outputchannels, outputformat, dspbufferlength, dspnumbuffers, extradriverdata);
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
// VERIFY: 0837c617 manual
// OFFSET: 0x009152D0
FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::closeCallback(FMOD_OUTPUT_STATE *output)
{
    OutputWavWriter_NRT *wavwriter = (OutputWavWriter_NRT *)output;

    return wavwriter->close();
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
// VERIFY: 0b629a95 manual
// OFFSET: 0x00915320
FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::startCallback(FMOD_OUTPUT_STATE *output)
{
    OutputWavWriter_NRT *wavwriter = (OutputWavWriter_NRT *)output;

    return wavwriter->start();
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
// VERIFY: b60bb3b6 auto
// OFFSET: 0x00915370
FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::stopCallback(FMOD_OUTPUT_STATE *output)
{
    OutputWavWriter_NRT *wavwriter = (OutputWavWriter_NRT *)output;

    return wavwriter->stop();
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
// VERIFY: d32c0fd9 manual
// OFFSET: 0x009153B0
FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::updateCallback(FMOD_OUTPUT_STATE *output)
{
    OutputWavWriter_NRT *wavwriter = (OutputWavWriter_NRT *)output;

    return wavwriter->update();
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    
	[SEE_ALSO]
]
*/
// VERIFY: e06740c2 manual
// OFFSET: 0x00915410
FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::getHandleCallback(FMOD_OUTPUT_STATE *output, void **handle)
{
    OutputWavWriter_NRT *wavwriter = (OutputWavWriter_NRT *)output;

    return wavwriter->getHandle(handle);
}

}


#endif
