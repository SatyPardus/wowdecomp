#include "fmod_settings.h"

#ifdef FMOD_SUPPORT_WAVWRITER

#include "fmod_memory.h"
#include "fmod_output_wavwriter.h"
#include "fmod_string.h"
#include "fmod_systemi.h"

namespace FMOD
{



FMOD_OUTPUT_DESCRIPTION_EX wavwriteroutput;

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
        return OutputWavWriter::getDescriptionEx();
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
// VERIFY: 18d1b08d manual
// OFFSET: 0x00915F10
FMOD_OUTPUT_DESCRIPTION_EX *OutputWavWriter::getDescriptionEx()
{
    FMOD_memset(&wavwriteroutput, 0, sizeof(FMOD_OUTPUT_DESCRIPTION));
    
    wavwriteroutput.name          = "FMOD WavWriter Output";
    wavwriteroutput.version       = 0x00010100;
    wavwriteroutput.polling       = true;
    wavwriteroutput.getnumdrivers = &OutputWavWriter::getNumDriversCallback;
    wavwriteroutput.getdrivername = &OutputWavWriter::getDriverNameCallback;
    wavwriteroutput.getdrivercaps = &OutputWavWriter::getDriverCapsCallback;
    wavwriteroutput.init          = &OutputWavWriter::initCallback;
    wavwriteroutput.close         = &OutputWavWriter::closeCallback;
    wavwriteroutput.getposition   = &OutputWavWriter::getPositionCallback;
    wavwriteroutput.lock          = &OutputWavWriter::lockCallback;
    wavwriteroutput.unlock        = &OutputWavWriter::unlockCallback;
    wavwriteroutput.gethandle     = &OutputWavWriter::getHandleCallback;

    /*
        Private members
    */
    wavwriteroutput.mType         = FMOD_OUTPUTTYPE_WAVWRITER;
    wavwriteroutput.mSize         = sizeof(OutputWavWriter);

    return &wavwriteroutput;
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
FMOD_RESULT OutputWavWriter::getNumDrivers(int *numdrivers)
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
FMOD_RESULT OutputWavWriter::getDriverName(int driver, char *name, int namelen)
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
FMOD_RESULT OutputWavWriter::getDriverCaps(int id, FMOD_CAPS *caps)
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
// VERIFY: dcdba99e manual
// OFFSET: 0x00915BA0
FMOD_RESULT OutputWavWriter::init(int selecteddriver, FMOD_INITFLAGS flags, int *outputrate, int outputchannels, FMOD_SOUND_FORMAT *outputformat, int dspbufferlength, int dspnumbuffers, void *extradriverdata)
{
    FMOD_RESULT result;

    Plugin::init(); /* Set up gGlobal - for debug / file / memory access by this plugin. */

    result = mSystem->getSoftwareFormat(&mRate, &mFormat, &mChannels, 0, 0, &mBits);
    if (result != FMOD_OK)
    {
        return result;
    }
    
    result = SoundI::getBytesFromSamples(dspbufferlength, &mBufferLengthBytes, mChannels, mFormat);
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
// VERIFY: 643efd55 auto
// OFFSET: 0x00915E40
FMOD_RESULT OutputWavWriter::close()
{
    writeWavHeader();

    if (mFP)
    {
        fclose(mFP);
        mFP = 0;
    }

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
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
FMOD_RESULT OutputWavWriter::getPosition(unsigned int *pcm)
{
    FMOD_RESULT     result;
    unsigned int    pos = 0;
    int             outputrate;

    /*
        Get the values FMOD had set for rate, format, channels.
    */
    result = mSystem->getSoftwareFormat(&outputrate, 0, 0, 0, 0, 0);
    if (result != FMOD_OK)
    {
        return result;
    }

    FMOD_OS_Time_GetMs(&pos);
	
    pos *= outputrate;
    pos /= 1000;

	*pcm = pos;

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
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
// VERIFY: 73a35640 manual
// OFFSET: 0x009157B0
FMOD_RESULT OutputWavWriter::lock(unsigned int offset, unsigned int length, void **ptr1, void **ptr2, unsigned int *len1, unsigned int *len2)
{
	offset %= mBufferLengthBytes;

	if (offset + length > mBufferLengthBytes)
	{
		*ptr1 = (char *)mBuffer + offset;
		*ptr2 = (char *)mBuffer;
		*len1 = mBufferLengthBytes - offset;
		*len2 = length - (mBufferLengthBytes - offset);
	}
	else
	{
		*ptr1 = (char *)mBuffer + offset;
		*ptr2 = NULL;
		*len1 = length;
		*len2 = 0;
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
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
// VERIFY: 63eef41d auto
// OFFSET: 0x00915840
FMOD_RESULT OutputWavWriter::unlock(void *ptr1, void *ptr2, unsigned int len1, unsigned int len2)
{
    if (ptr1 && len1)
    {
        mLengthBytes += fwrite(ptr1, 1, len1, mFP);
    }

    if (ptr2 && len2)
    {
        mLengthBytes += fwrite(ptr2, 1, len2, mFP);
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
FMOD_RESULT OutputWavWriter::getHandle(void **handle)
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
// VERIFY: eb529d65 manual
// OFFSET: 0x009158A0
FMOD_RESULT OutputWavWriter::writeWavHeader()
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
FMOD_RESULT F_CALLBACK OutputWavWriter::getNumDriversCallback(FMOD_OUTPUT_STATE *output, int *numdrivers)
{
    OutputWavWriter *wavwriter = (OutputWavWriter *)output;

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
// VERIFY: 98a2cc99 auto
// OFFSET: 0x00915A80
FMOD_RESULT F_CALLBACK OutputWavWriter::getDriverNameCallback(FMOD_OUTPUT_STATE *output, int id, char *name, int namelen)
{
    OutputWavWriter *wavwriter = (OutputWavWriter *)output;

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
FMOD_RESULT OutputWavWriter::getDriverCapsCallback(FMOD_OUTPUT_STATE *output, int id, FMOD_CAPS *caps)
{
    OutputWavWriter *wavwriter = (OutputWavWriter *)output;

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
// VERIFY: ff10e8cf auto
// OFFSET: 0x00915EA0
FMOD_RESULT F_CALLBACK OutputWavWriter::initCallback(FMOD_OUTPUT_STATE *output, int selecteddriver, FMOD_INITFLAGS flags, int *outputrate, int outputchannels, FMOD_SOUND_FORMAT *outputformat, int dspbufferlength, int dspnumbuffers, void *extradriverdata)
{
    OutputWavWriter *wavwriter = (OutputWavWriter *)output;

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
// VERIFY: bab56508 manual
// OFFSET: 0x00915EE0
FMOD_RESULT F_CALLBACK OutputWavWriter::closeCallback(FMOD_OUTPUT_STATE *output)
{
    OutputWavWriter *wavwriter = (OutputWavWriter *)output;

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
FMOD_RESULT F_CALLBACK OutputWavWriter::getPositionCallback(FMOD_OUTPUT_STATE *output, unsigned int *pcm)
{
    OutputWavWriter *wavwriter = (OutputWavWriter *)output;

    return wavwriter->getPosition(pcm);
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
// VERIFY: b7575bb8 auto
// OFFSET: 0x00915AF0
FMOD_RESULT F_CALLBACK OutputWavWriter::lockCallback(FMOD_OUTPUT_STATE *output, unsigned int offset, unsigned int length, void **ptr1, void **ptr2, unsigned int *len1, unsigned int *len2)
{
    OutputWavWriter *wavwriter = (OutputWavWriter *)output;

    return wavwriter->lock(offset, length, ptr1, ptr2, len1, len2);
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
// VERIFY: 136c04e0 auto
// OFFSET: 0x00915B30
FMOD_RESULT F_CALLBACK OutputWavWriter::unlockCallback(FMOD_OUTPUT_STATE *output, void *ptr1, void *ptr2, unsigned int len1, unsigned int len2)
{
    OutputWavWriter *wavwriter = (OutputWavWriter *)output;

    return wavwriter->unlock(ptr1, ptr2, len1, len2);
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
// VERIFY: fcfadfe4 manual
// OFFSET: 0x00915B60
FMOD_RESULT F_CALLBACK OutputWavWriter::getHandleCallback(FMOD_OUTPUT_STATE *output, void **handle)
{
    OutputWavWriter *wavwriter = (OutputWavWriter *)output;

    return wavwriter->getHandle(handle);
}




}


#endif
