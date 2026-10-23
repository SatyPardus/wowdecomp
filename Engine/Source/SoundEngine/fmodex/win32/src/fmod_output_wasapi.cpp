#include "fmod_settings.h"

#ifdef FMOD_SUPPORT_WASAPI

#include "fmod_output_wasapi.h"
#include "fmod_codec_wav.h"
#include "fmod_soundi.h"
#include "fmod_string.h"

#include <stdio.h>

namespace FMOD
{

FMOD_OUTPUT_DESCRIPTION_EX wasapioutput;

#ifdef PLUGIN_EXPORTS

#ifdef __cplusplus
extern "C" {
#endif

    /*
        FMODGetOutputDescription is mandantory for every fmod plugin.
        This is the symbol the registerplugin function searches for.
        Must be declared with F_API to make it export as stdcall.
    */
    F_DECLSPEC F_DLLEXPORT FMOD_OUTPUT_DESCRIPTION_EX * F_API FMODGetOutputDescriptionEx()
    {
        return OutputWASAPI::getDescriptionEx();
    }

#ifdef __cplusplus
}
#endif

#endif  /* PLUGIN_EXPORTS */


// VERIFY: 7471290d auto
// OFFSET: 0x009394E0
FMOD_OUTPUT_DESCRIPTION_EX *OutputWASAPI::getDescriptionEx()
{
    FMOD_memset(&wasapioutput, 0, sizeof(FMOD_OUTPUT_DESCRIPTION_EX));

    wasapioutput.name                   = "FMOD WASAPI Output";
    wasapioutput.version                = 0x00010100;
    wasapioutput.mType                  = FMOD_OUTPUTTYPE_WASAPI;
    wasapioutput.mSize                  = sizeof(OutputWASAPI);
    wasapioutput.polling                = false;
    wasapioutput.getnumdrivers          = &OutputWASAPI::getNumDriversCallback;
    wasapioutput.getdrivername          = &OutputWASAPI::getDriverNameCallback;
    wasapioutput.getdrivercapsex        = &OutputWASAPI::getDriverCapsExCallback;
    wasapioutput.initex                 = &OutputWASAPI::initExCallback;
    wasapioutput.close                  = &OutputWASAPI::closeCallback;
    wasapioutput.getposition            = &OutputWASAPI::getPositionCallback;
    wasapioutput.lock                   = &OutputWASAPI::lockCallback;
    wasapioutput.unlock                 = &OutputWASAPI::unlockCallback;
    wasapioutput.start                  = &OutputWASAPI::startCallback;
    wasapioutput.stop                   = &OutputWASAPI::stopCallback;
    wasapioutput.gethandle              = &OutputWASAPI::getHandleCallback;

#ifdef FMOD_SUPPORT_RECORDING
    wasapioutput.record_getnumdrivers   = &OutputWASAPI::recordGetNumDriversCallback;
    wasapioutput.record_getdrivername   = &OutputWASAPI::recordGetDriverInfoCallback;
    wasapioutput.record_getdrivercaps   = &OutputWASAPI::recordGetDriverCapsCallback;
    wasapioutput.record_start           = &OutputWASAPI::recordStartCallback;
    wasapioutput.record_stop            = &OutputWASAPI::recordStopCallback;
    wasapioutput.record_getposition     = &OutputWASAPI::recordGetPositionCallback;
    wasapioutput.record_lock            = &OutputWASAPI::recordLockCallback;
#endif

    return &wasapioutput;
}


// VERIFY: c6e354fa manual
// OFFSET: 0x00937370
FMOD_RESULT OutputWASAPI::enumerateDefaultDevices(IMMDeviceEnumerator *pEnumerator)
{
    FMOD_RESULT      result      = FMOD_OK;
    HRESULT          hr;
    IMMDevice       *device      = NULL;
    IPropertyStore  *properties  = NULL;
    PROPVARIANT      description = {};
    PROPVARIANT      friendlyname = {};
    char             name[512] = {0};

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (hr >= 0)
    {
        if (device->OpenPropertyStore(STGM_READ, &properties) < 0)
        {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        FMOD_memset(&description, 0, sizeof(PROPVARIANT));
        FMOD_memset(&friendlyname, 0, sizeof(PROPVARIANT));

        if (properties->GetValue(PKEY_Device_DeviceDesc, &description) < 0)
        {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }
        if (properties->GetValue(PKEY_DeviceInterface_FriendlyName, &friendlyname) < 0)
        {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        sprintf(name, "%S (%S)", description.pwszVal, friendlyname.pwszVal);

        mRenderDrivers[0].name = (char *)FMOD_Memory_Calloc(FMOD_strlen(name) + 1);
        FMOD_strncpy(mRenderDrivers[0].name, name, FMOD_strlen(name));

        if (device->GetId(&mRenderDrivers[0].id) < 0)
        {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        mNumRenderDrivers++;

        PropVariantClear(&description);
        PropVariantClear(&friendlyname);

        if (properties)
        {
            properties->Release();
            properties = NULL;
        }
        if (device)
        {
            device->Release();
            device = NULL;
        }
    }
    else if (hr != E_NOTFOUND)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device);
    if (hr >= 0)
    {
        if (device->OpenPropertyStore(STGM_READ, &properties) < 0)
        {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        FMOD_memset(&description, 0, sizeof(PROPVARIANT));
        FMOD_memset(&friendlyname, 0, sizeof(PROPVARIANT));

        if (properties->GetValue(PKEY_Device_DeviceDesc, &description) < 0)
        {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }
        if (properties->GetValue(PKEY_DeviceInterface_FriendlyName, &friendlyname) < 0)
        {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        sprintf(name, "%S (%S)", description.pwszVal, friendlyname.pwszVal);

        mCaptureDrivers[0].name = (char *)FMOD_Memory_Calloc(FMOD_strlen(name) + 1);
        FMOD_strncpy(mCaptureDrivers[0].name, name, FMOD_strlen(name));

        if (device->GetId(&mCaptureDrivers[0].id) < 0)
        {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        mNumCaptureDrivers++;
    }
    else if (hr != E_NOTFOUND)
    {
        result = FMOD_ERR_OUTPUT_INIT;
    }

Exit:
    PropVariantClear(&description);
    PropVariantClear(&friendlyname);

    if (properties)
    {
        properties->Release();
        properties = NULL;
    }
    if (device)
    {
        device->Release();
    }

    return result;
}


// VERIFY: eccab240 manual
// OFFSET: 0x009376C0
FMOD_RESULT OutputWASAPI::enumerate()
{
    FMOD_RESULT           result      = FMOD_OK;
    IMMDeviceEnumerator  *enumerator  = NULL;
    IMMDeviceCollection  *collection  = NULL;
    IMMDevice            *device      = NULL;
    IPropertyStore       *properties  = NULL;
    PROPVARIANT           description = {};
    PROPVARIANT           friendlyname = {};
    UINT                  numdevices  = 0;
    UINT                  count;
    int                   i;

    if (!mCoInitialized)
    {
        Plugin::init();

        CoInitialize(NULL);
        mCoInitialized = true;
    }

    for (i = 0; i < mNumRenderDrivers; i++)
    {
        if (mRenderDrivers[i].name)
        {
            FMOD_Memory_Free(mRenderDrivers[i].name);
            mRenderDrivers[i].name = NULL;
        }
        CoTaskMemFree(mRenderDrivers[i].id);
    }
    for (i = 0; i < mNumCaptureDrivers; i++)
    {
        if (mCaptureDrivers[i].name)
        {
            FMOD_Memory_Free(mCaptureDrivers[i].name);
            mCaptureDrivers[i].name = NULL;
        }
        CoTaskMemFree(mCaptureDrivers[i].id);
    }
    mNumRenderDrivers  = 0;
    mNumCaptureDrivers = 0;

    if (CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER, IID_IMMDeviceEnumerator, (void **)&enumerator) < 0)
    {
        result = FMOD_ERR_COM;
        goto Exit;
    }

    result = enumerateDefaultDevices(enumerator);
    if (result != FMOD_OK)
    {
        goto Exit;
    }

    if (enumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &collection) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (collection->GetCount(&numdevices) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    for (count = 0; count < numdevices; count++)
    {
        char         name[512] = {0};
        IMMEndpoint *endpoint = NULL;
        EDataFlow    dataflow = eAll;

        if (collection->Item(count, &device) < 0)
        {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }
        if (device->OpenPropertyStore(STGM_READ, &properties) < 0)
        {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        FMOD_memset(&description, 0, sizeof(PROPVARIANT));
        FMOD_memset(&friendlyname, 0, sizeof(PROPVARIANT));

        if (properties->GetValue(PKEY_Device_DeviceDesc, &description) < 0 || properties->GetValue(PKEY_DeviceInterface_FriendlyName, &friendlyname) < 0)
        {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        sprintf(name, "%S (%S)", description.pwszVal, friendlyname.pwszVal);

        device->QueryInterface(IID_IMMEndpoint, (void **)&endpoint);
        endpoint->GetDataFlow(&dataflow);

        if ((dataflow == eRender || dataflow == eAll) && mNumRenderDrivers < FMOD_OUTPUT_MAXDRIVERS)
        {
            if (FMOD_strcmp(name, mRenderDrivers[0].name))
            {
                mRenderDrivers[mNumRenderDrivers].name = (char *)FMOD_Memory_Calloc(FMOD_strlen(name) + 1);
                FMOD_strncpy(mRenderDrivers[mNumRenderDrivers].name, name, FMOD_strlen(name));

                if (device->GetId(&mRenderDrivers[mNumRenderDrivers].id) < 0)
                {
                    result = FMOD_ERR_OUTPUT_INIT;
                    goto Exit;
                }

                mNumRenderDrivers++;
            }
        }

        if ((dataflow == eCapture || dataflow == eAll) && mNumCaptureDrivers < FMOD_OUTPUT_MAXDRIVERS && FMOD_strcmp(name, mCaptureDrivers[0].name))
        {
            mCaptureDrivers[mNumCaptureDrivers].name = (char *)FMOD_Memory_Calloc(FMOD_strlen(name) + 1);
            FMOD_strncpy(mCaptureDrivers[mNumCaptureDrivers].name, name, FMOD_strlen(name));

            if (device->GetId(&mCaptureDrivers[mNumCaptureDrivers].id) < 0)
            {
                result = FMOD_ERR_OUTPUT_INIT;
                goto Exit;
            }

            mNumCaptureDrivers++;
        }

        PropVariantClear(&description);
        PropVariantClear(&friendlyname);

        if (properties)
        {
            properties->Release();
            properties = NULL;
        }
        if (device)
        {
            device->Release();
            device = NULL;
        }
    }

    if (collection)
    {
        collection->Release();
        collection = NULL;
    }
    if (enumerator)
    {
        enumerator->Release();
    }

    return FMOD_OK;

Exit:
    PropVariantClear(&friendlyname);
    PropVariantClear(&description);

    if (properties)
    {
        properties->Release();
        properties = NULL;
    }
    if (device)
    {
        device->Release();
        device = NULL;
    }
    if (collection)
    {
        collection->Release();
        collection = NULL;
    }
    if (enumerator)
    {
        enumerator->Release();
    }

    return result;
}


FMOD_RESULT OutputWASAPI::getNumDrivers(int *numdrivers)
{
    FMOD_RESULT result;

    if (!numdrivers)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = enumerate();
    if (result != FMOD_OK)
    {
        return result;
    }

    *numdrivers = mNumRenderDrivers;

    return FMOD_OK;
}


// VERIFY: 4a80cb3e manual
// OFFSET: 0x00937B80
FMOD_RESULT OutputWASAPI::getDriverName(int id, char *name, int namelen)
{
    FMOD_RESULT result;

    if (!name || namelen < 1)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = enumerate();
    if (result != FMOD_OK)
    {
        return result;
    }

    if (id < 0 || id >= mNumRenderDrivers)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_strncpy(name, mRenderDrivers[id].name, namelen - 1);

    return FMOD_OK;
}


// VERIFY: 87cb9c61 manual
// OFFSET: 0x00937BE0
FMOD_RESULT OutputWASAPI::getDriverCapsEx(int id, FMOD_CAPS *caps, int *minfrequency, int *maxfrequency, FMOD_SPEAKERMODE *controlpanelspeakermode)
{
    FMOD_RESULT           result;
    IMMDeviceEnumerator  *enumerator  = NULL;
    IMMDevice            *device      = NULL;
    IAudioClient         *audioclient = NULL;
    WAVE_FORMATEX        *mixformat   = NULL;

    result = enumerate();
    if (result != FMOD_OK)
    {
        return result;
    }

    if (CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER, IID_IMMDeviceEnumerator, (void **)&enumerator) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
    }
    else if (enumerator->GetDevice(mRenderDrivers[id].id, &device) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
    }
    else if (device->Activate(IID_IAudioClient, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER, NULL, (void **)&audioclient) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
    }
    else if (audioclient->GetMixFormat(&mixformat) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
    }
    else
    {
        switch (mixformat->nChannels)
        {
            case 4:
            {
                *controlpanelspeakermode = FMOD_SPEAKERMODE_QUAD;
                break;
            }
            case 6:
            {
                *controlpanelspeakermode = FMOD_SPEAKERMODE_5POINT1;
                break;
            }
            case 8:
            {
                *controlpanelspeakermode = FMOD_SPEAKERMODE_7POINT1;
                break;
            }
            default:
            {
                *controlpanelspeakermode = FMOD_SPEAKERMODE_STEREO;
                break;
            }
        }

        *minfrequency = *maxfrequency = mixformat->nSamplesPerSec;

        *caps = (FMOD_CAPS)(FMOD_CAPS_OUTPUT_FORMAT_PCM8 | FMOD_CAPS_OUTPUT_FORMAT_PCM16 | FMOD_CAPS_OUTPUT_FORMAT_PCM24 | FMOD_CAPS_OUTPUT_FORMAT_PCM32 | FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT);
        if (mixformat->nChannels > 2)
        {
            *caps = (FMOD_CAPS)(*caps | FMOD_CAPS_OUTPUT_MULTICHANNEL);
        }
    }

    CoTaskMemFree(mixformat);

    if (audioclient)
    {
        audioclient->Release();
        audioclient = NULL;
    }
    if (device)
    {
        device->Release();
        device = NULL;
    }
    if (enumerator)
    {
        enumerator->Release();
    }

    return result;
}


// VERIFY: 21b96aa0 manual
// OFFSET: 0x00937D60
FMOD_RESULT OutputWASAPI::initEx(int selecteddriver, FMOD_INITFLAGS flags, int *outputrate, int outputchannels, FMOD_SOUND_FORMAT *outputformat, FMOD_SPEAKERMODE *speakermode, int dspbufferlength, int dspnumbuffers, int max2dchannels, int max3dchannels, void *extradriverdata)
{
    FMOD_RESULT             result;
    HRESULT                 hr;
    IMMDeviceEnumerator    *enumerator  = NULL;
    IMMDevice              *device      = NULL;
    WAVE_FORMATEX          *mixformat   = NULL;
    WAVE_FORMATEX          *closest     = NULL;
    WAVE_FORMATEXTENSIBLE   wfx;
    REFERENCE_TIME          period      = 0;
    int                     bits;

    result = enumerate();
    if (result != FMOD_OK)
    {
        return result;
    }

    if (!mNumRenderDrivers)
    {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (flags & FMOD_INIT_WASAPI_EXCLUSIVE)
    {
        mExclusiveMode = true;
    }

    if (CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER, IID_IMMDeviceEnumerator, (void **)&enumerator) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (enumerator->GetDevice(mRenderDrivers[selecteddriver].id, &device) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (device->Activate(IID_IAudioClient, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER, NULL, (void **)&mRenderAudioClient) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    FMOD_memset(&wfx, 0, sizeof(WAVE_FORMATEXTENSIBLE));

    if (mRenderAudioClient->GetMixFormat(&mixformat) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    switch (*outputformat)
    {
        case FMOD_SOUND_FORMAT_PCM8:     bits = 8;  break;
        case FMOD_SOUND_FORMAT_PCM16:    bits = 16; break;
        case FMOD_SOUND_FORMAT_PCM24:    bits = 24; break;
        case FMOD_SOUND_FORMAT_PCM32:
        case FMOD_SOUND_FORMAT_PCMFLOAT: bits = 32; break;
        case FMOD_SOUND_FORMAT_NONE:
        case FMOD_SOUND_FORMAT_GCADPCM:
        case FMOD_SOUND_FORMAT_IMAADPCM:
        case FMOD_SOUND_FORMAT_VAG:
        case FMOD_SOUND_FORMAT_XMA:
        case FMOD_SOUND_FORMAT_MPEG:     bits = 0;  break;
        default:
        {
            result = FMOD_ERR_FORMAT;
            goto Exit;
        }
    }

    switch (mixformat->nChannels)
    {
        case 1: *speakermode = FMOD_SPEAKERMODE_MONO;    break;
        case 2: *speakermode = FMOD_SPEAKERMODE_STEREO;  break;
        case 4: *speakermode = FMOD_SPEAKERMODE_QUAD;    break;
        case 6: *speakermode = FMOD_SPEAKERMODE_5POINT1; break;
        case 8: *speakermode = FMOD_SPEAKERMODE_7POINT1; break;
    }

    wfx.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
    wfx.Format.nChannels            = mixformat->nChannels;
    wfx.Format.nSamplesPerSec       = *outputrate;
    wfx.Format.nBlockAlign          = wfx.Format.nChannels * bits / 8;
    wfx.Format.wBitsPerSample       = bits;
    wfx.Format.nAvgBytesPerSec      = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
    wfx.Format.cbSize               = 22;
    wfx.Samples.wValidBitsPerSample = bits;
    FMOD_memcpy(&wfx.SubFormat, (*outputformat == FMOD_SOUND_FORMAT_PCMFLOAT ? &_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : &_KSDATAFORMAT_SUBTYPE_PCM), sizeof(GUID));

    hr = mRenderAudioClient->IsFormatSupported(mExclusiveMode ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED, &wfx.Format, &closest);
    if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == S_FALSE)
    {
        *outputrate = mixformat->nSamplesPerSec;

        wfx.Format.nSamplesPerSec  = mixformat->nSamplesPerSec;
        wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
    }

    if (mRenderAudioClient->GetDevicePeriod(mExclusiveMode ? NULL : &period, mExclusiveMode ? &period : NULL) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    hr = mRenderAudioClient->Initialize(mExclusiveMode ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED, 0, (REFERENCE_TIME)(dspbufferlength * dspnumbuffers) * 10000000 / wfx.Format.nSamplesPerSec, mExclusiveMode ? period : 0, &wfx.Format, NULL);
    if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT)
    {
        result = FMOD_ERR_OUTPUT_FORMAT;
        goto Exit;
    }
    if (hr < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    if (mRenderAudioClient->GetBufferSize(&mRenderBufferLength) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (mRenderAudioClient->GetService(IID_IAudioRenderClient, (void **)&mRenderClient) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (mRenderAudioClient->GetService(IID_IAudioClock, (void **)&mRenderAudioClock) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (mRenderClient->GetBuffer(mRenderBufferLength, &mRenderBuffer) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (mRenderClient->ReleaseBuffer(mRenderBufferLength, 0) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    result = FMOD_OS_CriticalSection_Create(&mRecordCrit);
    if (result == FMOD_OK)
    {
        mInitialised = true;
    }

Exit:
    CoTaskMemFree(closest);
    CoTaskMemFree(mixformat);

    if (device)
    {
        device->Release();
        device = NULL;
    }
    if (enumerator)
    {
        enumerator->Release();
    }

    return result;
}


// VERIFY: bdfc21e8 manual
// OFFSET: 0x009381B0
FMOD_RESULT OutputWASAPI::close()
{
    int i;

    if (mRenderAudioClock)
    {
        mRenderAudioClock->Release();
        mRenderAudioClock = NULL;
    }
    if (mRenderClient)
    {
        mRenderClient->Release();
        mRenderClient = NULL;
    }
    if (mRenderAudioClient)
    {
        mRenderAudioClient->Release();
        mRenderAudioClient = NULL;
    }
    if (mCaptureClient)
    {
        mCaptureClient->Release();
        mCaptureClient = NULL;
    }
    if (mCaptureAudioClient)
    {
        mCaptureAudioClient->Release();
        mCaptureAudioClient = NULL;
    }

    for (i = 0; i < mNumRenderDrivers; i++)
    {
        if (mRenderDrivers[i].name)
        {
            FMOD_Memory_Free(mRenderDrivers[i].name);
            mRenderDrivers[i].name = NULL;
        }
        CoTaskMemFree(mRenderDrivers[i].id);
    }
    for (i = 0; i < mNumCaptureDrivers; i++)
    {
        if (mCaptureDrivers[i].name)
        {
            FMOD_Memory_Free(mCaptureDrivers[i].name);
            mCaptureDrivers[i].name = NULL;
        }
        CoTaskMemFree(mCaptureDrivers[i].id);
    }
    mNumRenderDrivers  = 0;
    mNumCaptureDrivers = 0;

    if (mRecordBuffer)
    {
        FMOD_Memory_Free(mRecordBuffer);
        mRecordBuffer = NULL;
    }

    CoUninitialize();

    FMOD_OS_CriticalSection_Free(mRecordCrit);
    mRecordCrit = NULL;

    mInitialised   = false;
    mCoInitialized = false;

    return FMOD_OK;
}


FMOD_RESULT OutputWASAPI::getHandle(void **handle)
{
    *handle = mRenderClient;

    return FMOD_OK;
}


FMOD_RESULT OutputWASAPI::start()
{
    FMOD_RESULT result;

    result = OutputTimer::start();
    if (result != FMOD_OK)
    {
        return result;
    }

    if (mRenderAudioClient->Start() < 0)
    {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    return FMOD_OK;
}


FMOD_RESULT OutputWASAPI::stop()
{
    FMOD_RESULT result;

    if (!mInitialised)
    {
        return FMOD_OK;
    }

    result = OutputTimer::stop();
    if (result != FMOD_OK)
    {
        return result;
    }

    if (mRenderAudioClient->Stop() < 0)
    {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    return FMOD_OK;
}


// VERIFY: 6ea3f433 manual
// OFFSET: 0x00938340
FMOD_RESULT OutputWASAPI::getPosition(unsigned int *pcm)
{
    UINT64 position  = 0;
    UINT64 frequency = 0;
    UINT32 padding   = 0;

    if (mRenderAudioClock->GetPosition(&position, NULL) < 0 || mRenderAudioClock->GetFrequency(&frequency) < 0)
    {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    *pcm = (unsigned int)(mSystem->mOutputRate * position / frequency) % mRenderBufferLength;

    if (mRenderAudioClient->GetCurrentPadding(&padding) < 0)
    {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    if (!padding)
    {
        FMOD_RESULT  result;
        unsigned int bufferlength = 0;
        int          numbuffers   = 0;

        result = mSystem->getDSPBufferSize(&bufferlength, &numbuffers);
        if (result != FMOD_OK)
        {
            return result;
        }

        mFillBlock = ((int)(*pcm / bufferlength) % numbuffers + 1) % numbuffers;
    }

    return FMOD_OK;
}


// VERIFY: 338386c4 manual
// OFFSET: 0x00939120
FMOD_RESULT OutputWASAPI::lock(unsigned int offset, unsigned int length, void **ptr1, void **ptr2, unsigned int *len1, unsigned int *len2)
{
    FMOD_RESULT        result;
    UINT32             padding = 0;
    unsigned int       samples;
    FMOD_SOUND_FORMAT  format;
    int                channels;

    if (mRenderAudioClient->GetCurrentPadding(&padding) < 0)
    {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    format   = mSystem->mOutputFormat;
    channels = mSystem->mMaxOutputChannels;

    result = SoundI::getSamplesFromBytes(length, &samples, channels, format);
    if (result != FMOD_OK)
    {
        return result;
    }

    if (mRenderBufferLength - padding <= samples)
    {
        *ptr1 = NULL;
        *len1 = 0;
        *ptr2 = NULL;
        *len2 = 0;
        return FMOD_OK;
    }

    if (mRenderClient->GetBuffer(samples, (BYTE **)ptr1) < 0)
    {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    *len1 = length;
    *ptr2 = NULL;
    *len2 = 0;

    return FMOD_OK;
}


// VERIFY: 9c864a2f manual
// OFFSET: 0x00939310
FMOD_RESULT OutputWASAPI::unlock(void *ptr1, void *ptr2, unsigned int len1, unsigned int len2)
{
    FMOD_RESULT  result;
    unsigned int samples;

    result = SoundI::getSamplesFromBytes(len1, &samples, mSystem->mMaxOutputChannels, mSystem->mOutputFormat);
    if (result != FMOD_OK)
    {
        return result;
    }

    if (mRenderClient->ReleaseBuffer(samples, 0) < 0)
    {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    return FMOD_OK;
}


#ifdef FMOD_SUPPORT_RECORDING

// VERIFY: 637e006b manual
// OFFSET: 0x00938440
FMOD_RESULT OutputWASAPI::recordUpdate()
{
    UINT32  packetsize = 0;
    UINT32  numframes  = 0;
    DWORD   flags      = 0;
    BYTE   *src        = NULL;

    if (!mRecording)
    {
        return FMOD_OK;
    }

    if (mCaptureClient->GetNextPacketSize(&packetsize) < 0)
    {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    while (packetsize)
    {
        int   writeposition = mRecordWritePosition;
        BYTE *dest          = mRecordBuffer + writeposition * mRecordBlockAlign;

        if (mCaptureClient->GetBuffer(&src, &numframes, &flags, NULL, NULL) < 0)
        {
            return FMOD_ERR_OUTPUT_DRIVERCALL;
        }

        if (mRecordStereoToMono)
        {
            int blockalign = mRecordBlockAlign;

            for (int count = 0; count <= (int)numframes; count++)
            {
                FMOD_memcpy(dest, src, blockalign);

                src  += blockalign * 2;
                dest += blockalign;

                if (dest == mRecordBuffer + mRecordBufferLength * mRecordBlockAlign)
                {
                    dest = mRecordBuffer;
                }
            }
        }
        else if (writeposition + numframes > mRecordBufferLength)
        {
            unsigned int firstlength = mRecordBufferLength - writeposition;

            FMOD_memcpy(dest, src, firstlength * mRecordBlockAlign);
            src += firstlength * mRecordBlockAlign;
            FMOD_memcpy(mRecordBuffer, src, (numframes - firstlength) * mRecordBlockAlign);
        }
        else
        {
            FMOD_memcpy(dest, src, numframes * mRecordBlockAlign);
        }

        if (mCaptureClient->ReleaseBuffer(numframes) < 0)
        {
            return FMOD_ERR_OUTPUT_DRIVERCALL;
        }

        writeposition = (writeposition + numframes) % mRecordBufferLength;

        if (mCaptureClient->GetNextPacketSize(&packetsize) < 0)
        {
            return FMOD_ERR_OUTPUT_DRIVERCALL;
        }

        mRecordWritePosition = writeposition;
    }

    return FMOD_OK;
}


// VERIFY: d841336d auto
// OFFSET: 0x00938600
void CALLBACK OutputWASAPI::recordTimerCallback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)dwUser;

    FMOD_OS_CriticalSection_Enter(wasapi->mRecordCrit);
    wasapi->recordUpdate();
    FMOD_OS_CriticalSection_Leave(wasapi->mRecordCrit);
}


FMOD_RESULT OutputWASAPI::recordGetNumDrivers(int *numdrivers)
{
    FMOD_RESULT result;

    if (!numdrivers)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = enumerate();
    if (result != FMOD_OK)
    {
        return result;
    }

    *numdrivers = mNumCaptureDrivers;

    return FMOD_OK;
}


// VERIFY: 847f0fbf manual
// OFFSET: 0x00938640
FMOD_RESULT OutputWASAPI::recordGetDriverInfo(int id, char *name, int namelen)
{
    FMOD_RESULT result;

    if (!name || namelen < 1)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = enumerate();
    if (result != FMOD_OK)
    {
        return result;
    }

    if (id < 0 || id >= mNumCaptureDrivers)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_strncpy(name, mCaptureDrivers[id].name, namelen - 1);

    return FMOD_OK;
}


// VERIFY: 22a7e152 auto
// OFFSET: 0x009386A0
FMOD_RESULT OutputWASAPI::recordGetDriverCaps(int id, FMOD_CAPS *caps, int *minfrequency, int *maxfrequency)
{
    FMOD_RESULT           result;
    IMMDeviceEnumerator  *enumerator  = NULL;
    IMMDevice            *device      = NULL;
    IAudioClient         *audioclient = NULL;
    WAVE_FORMATEX        *mixformat   = NULL;

    result = enumerate();
    if (result != FMOD_OK)
    {
        return result;
    }

    if (CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER, IID_IMMDeviceEnumerator, (void **)&enumerator) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
    }
    else if (enumerator->GetDevice(mCaptureDrivers[id].id, &device) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
    }
    else if (device->Activate(IID_IAudioClient, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER, NULL, (void **)&audioclient) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
    }
    else if (audioclient->GetMixFormat(&mixformat) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
    }
    else
    {
        *minfrequency = *maxfrequency = mixformat->nSamplesPerSec;

        *caps = (FMOD_CAPS)(FMOD_CAPS_OUTPUT_FORMAT_PCM8 | FMOD_CAPS_OUTPUT_FORMAT_PCM16 | FMOD_CAPS_OUTPUT_FORMAT_PCM24 | FMOD_CAPS_OUTPUT_FORMAT_PCM32 | FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT);
        if (mixformat->nChannels > 2)
        {
            *caps = (FMOD_CAPS)(*caps | FMOD_CAPS_OUTPUT_MULTICHANNEL);
        }
    }

    CoTaskMemFree(mixformat);

    if (audioclient)
    {
        audioclient->Release();
        audioclient = NULL;
    }
    if (device)
    {
        device->Release();
        device = NULL;
    }
    if (enumerator)
    {
        enumerator->Release();
    }

    return result;
}


// VERIFY: 320c842d manual
// OFFSET: 0x009387C0
FMOD_RESULT OutputWASAPI::recordStart(int id, Sound *sound, bool loop)
{
    FMOD_RESULT             result;
    HRESULT                 hr;
    IMMDeviceEnumerator    *enumerator  = NULL;
    IMMDevice              *device      = NULL;
    WAVE_FORMATEX          *mixformat   = NULL;
    WAVE_FORMATEX          *closest     = NULL;
    WAVE_FORMATEXTENSIBLE   wfx;
    REFERENCE_TIME          period      = 0;
    SoundI                 *soundi      = (SoundI *)sound;
    int                     bits        = 0;

    if (!soundi || !soundi->mLength)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = enumerate();
    if (result != FMOD_OK)
    {
        return result;
    }

    if (!mNumCaptureDrivers)
    {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER, IID_IMMDeviceEnumerator, (void **)&enumerator) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (enumerator->GetDevice(mCaptureDrivers[id].id, &device) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (device->Activate(IID_IAudioClient, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER, NULL, (void **)&mCaptureAudioClient) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    FMOD_memset(&wfx, 0, sizeof(WAVE_FORMATEX));

    if (mCaptureAudioClient->GetMixFormat(&mixformat) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    switch (soundi->mFormat)
    {
        case FMOD_SOUND_FORMAT_PCM8:     bits = 8;  break;
        case FMOD_SOUND_FORMAT_PCM16:    bits = 16; break;
        case FMOD_SOUND_FORMAT_PCM24:    bits = 24; break;
        case FMOD_SOUND_FORMAT_PCM32:
        case FMOD_SOUND_FORMAT_PCMFLOAT: bits = 32; break;
        case FMOD_SOUND_FORMAT_NONE:
        case FMOD_SOUND_FORMAT_GCADPCM:
        case FMOD_SOUND_FORMAT_IMAADPCM:
        case FMOD_SOUND_FORMAT_VAG:
        case FMOD_SOUND_FORMAT_XMA:
        case FMOD_SOUND_FORMAT_MPEG:     bits = 0;  break;
        default:
        {
            result = FMOD_ERR_FORMAT;
            goto Exit;
        }
    }

    wfx.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
    wfx.Format.nChannels            = soundi->mChannels;
    wfx.Format.nSamplesPerSec       = (DWORD)soundi->mDefaultFrequency;
    wfx.Format.nBlockAlign          = wfx.Format.nChannels * bits / 8;
    wfx.Format.wBitsPerSample       = bits;
    wfx.Format.nAvgBytesPerSec      = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
    wfx.Format.cbSize               = 22;
    wfx.Samples.wValidBitsPerSample = bits;
    FMOD_memcpy(&wfx.SubFormat, (soundi->mFormat == FMOD_SOUND_FORMAT_PCMFLOAT ? &_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : &_KSDATAFORMAT_SUBTYPE_PCM), sizeof(GUID));

    if (mCaptureAudioClient->GetDevicePeriod(mExclusiveMode ? NULL : &period, mExclusiveMode ? &period : NULL) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    mRecordStereoToMono = false;

    hr = mCaptureAudioClient->IsFormatSupported(mExclusiveMode ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED, &wfx.Format, &closest);
    if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == S_FALSE)
    {
        if (soundi->mChannels == 1 && mixformat->nChannels == 2)
        {
            wfx.Format.nChannels       = 2;
            wfx.Format.nBlockAlign     = 2 * wfx.Format.wBitsPerSample / 8;
            wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
            mRecordStereoToMono        = true;
        }

        if (mixformat->nSamplesPerSec != soundi->mDefaultFrequency)
        {
            wfx.Format.nSamplesPerSec  = mixformat->nSamplesPerSec;
            wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
        }
    }

    hr = mCaptureAudioClient->Initialize(mExclusiveMode ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED, 0, 5000000, mExclusiveMode ? period : 0, &wfx.Format, NULL);
    if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT)
    {
        result = FMOD_ERR_FORMAT;
        goto Exit;
    }
    if (hr < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    mRecordRate       = wfx.Format.nSamplesPerSec;
    mRecordFormat     = soundi->mFormat;
    mRecordBlockAlign = soundi->mChannels * (bits / 8);

    if (mCaptureAudioClient->GetBufferSize(&mRecordBufferLength) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (mCaptureAudioClient->GetService(IID_IAudioCaptureClient, (void **)&mCaptureClient) < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    mRecordBuffer = (BYTE *)FMOD_Memory_Calloc(mRecordBlockAlign * mRecordBufferLength);
    if (!mRecordBuffer)
    {
        result = FMOD_ERR_MEMORY;
        goto Exit;
    }

    mRecordTimerPeriod   = period * 0.0001f;
    mRecordWritePosition = 0;

    mRecordTimerID = timeSetEvent((UINT)mRecordTimerPeriod, 0, recordTimerCallback, (DWORD_PTR)this, TIME_PERIODIC);
    if (!mRecordTimerID)
    {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (mCaptureAudioClient->Start() < 0)
    {
        result = FMOD_ERR_OUTPUT_INIT;
    }

Exit:
    CoTaskMemFree(closest);
    CoTaskMemFree(mixformat);

    if (device)
    {
        device->Release();
        device = NULL;
    }
    if (enumerator)
    {
        enumerator->Release();
    }

    return result;
}


// VERIFY: 8a1f828c auto
// OFFSET: 0x00938C60
FMOD_RESULT OutputWASAPI::recordStop()
{
    if (mCaptureAudioClient)
    {
        timeKillEvent(mRecordTimerID);
        mRecordTimerID = 0;

        FMOD_OS_CriticalSection_Enter(mRecordCrit);

        mCaptureAudioClient->Stop();

        if (mCaptureClient)
        {
            mCaptureClient->Release();
            mCaptureClient = NULL;
        }
        if (mCaptureAudioClient)
        {
            mCaptureAudioClient->Release();
            mCaptureAudioClient = NULL;
        }
        if (mRecordBuffer)
        {
            FMOD_Memory_Free(mRecordBuffer);
            mRecordBuffer = NULL;
        }

        FMOD_OS_CriticalSection_Leave(mRecordCrit);
    }

    return FMOD_OK;
}


FMOD_RESULT OutputWASAPI::recordGetPosition(unsigned int *pcm)
{
    *pcm = mRecordWritePosition;

    return FMOD_OK;
}


// VERIFY: d51a72d5 manual
// OFFSET: 0x00938D10
FMOD_RESULT OutputWASAPI::recordLock(unsigned int offset, unsigned int length, void **ptr1, void **ptr2, unsigned int *len1, unsigned int *len2)
{
	unsigned int lenbytes = mRecordBufferLength * mRecordBlockAlign;

	if (length > lenbytes)
    {
		length = lenbytes;
    }

	if (offset >= lenbytes)
	{
		*ptr1 = NULL;
		*ptr2 = NULL;
		*len1 = 0;
		*len2 = 0;
		return FMOD_ERR_INVALID_PARAM;
	}

	*ptr1 = mRecordBuffer + offset;

	if (offset + length > lenbytes)
	{
		*len1 = lenbytes - offset;
		*ptr2 = mRecordBuffer;
		*len2 = length + offset - lenbytes;
	}
	else
	{
		*len1 = length;
		*ptr2 = NULL;
		*len2 = 0;
	}

    return FMOD_OK;
}

#endif


// VERIFY: 295ab632 auto
// OFFSET: 0x00938DD0
FMOD_RESULT F_CALLBACK OutputWASAPI::getNumDriversCallback(FMOD_OUTPUT_STATE *output, int *numdrivers)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->getNumDrivers(numdrivers);
}


// VERIFY: e16cf880 auto
// OFFSET: 0x00938E10
FMOD_RESULT F_CALLBACK OutputWASAPI::getDriverNameCallback(FMOD_OUTPUT_STATE *output, int id, char *name, int namelen)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->getDriverName(id, name, namelen);
}


// VERIFY: 34c78eff auto
// OFFSET: 0x00938E40
FMOD_RESULT F_CALLBACK OutputWASAPI::getDriverCapsExCallback(FMOD_OUTPUT_STATE *output, int id, FMOD_CAPS *caps, int *minfrequency, int *maxfrequency, FMOD_SPEAKERMODE *controlpanelspeakermode)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->getDriverCapsEx(id, caps, minfrequency, maxfrequency, controlpanelspeakermode);
}


// VERIFY: 981f5a90 auto
// OFFSET: 0x00938E70
FMOD_RESULT F_CALLBACK OutputWASAPI::initExCallback(FMOD_OUTPUT_STATE *output, int selecteddriver, FMOD_INITFLAGS flags, int *outputrate, int outputchannels, FMOD_SOUND_FORMAT *outputformat, FMOD_SPEAKERMODE *speakermode, int dspbufferlength, int dspnumbuffers, int max2dchannels, int max3dchannels, void *extradriverdata)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->initEx(selecteddriver, flags, outputrate, outputchannels, outputformat, speakermode, dspbufferlength, dspnumbuffers, max2dchannels, max3dchannels, extradriverdata);
}


// VERIFY: 3e364739 manual
// OFFSET: 0x00938EC0
FMOD_RESULT F_CALLBACK OutputWASAPI::closeCallback(FMOD_OUTPUT_STATE *output)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->close();
}


// VERIFY: 63e236d6 manual
// OFFSET: 0x00938EF0
FMOD_RESULT F_CALLBACK OutputWASAPI::startCallback(FMOD_OUTPUT_STATE *output)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->start();
}


// VERIFY: 5497c800 manual
// OFFSET: 0x00938F30
FMOD_RESULT F_CALLBACK OutputWASAPI::stopCallback(FMOD_OUTPUT_STATE *output)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->stop();
}


FMOD_RESULT F_CALLBACK OutputWASAPI::getHandleCallback(FMOD_OUTPUT_STATE *output, void **handle)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->getHandle(handle);
}


// VERIFY: 60bee7a4 manual
// OFFSET: 0x00938F80
FMOD_RESULT F_CALLBACK OutputWASAPI::getPositionCallback(FMOD_OUTPUT_STATE *output, unsigned int *pcm)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->getPosition(pcm);
}


// VERIFY: 330e8834 auto
// OFFSET: 0x00939470
FMOD_RESULT F_CALLBACK OutputWASAPI::lockCallback(FMOD_OUTPUT_STATE *output, unsigned int offset, unsigned int length, void **ptr1, void **ptr2, unsigned int *len1, unsigned int *len2)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->lock(offset, length, ptr1, ptr2, len1, len2);
}


// VERIFY: 9816ebc2 auto
// OFFSET: 0x009394B0
FMOD_RESULT F_CALLBACK OutputWASAPI::unlockCallback(FMOD_OUTPUT_STATE *output, void *ptr1, void *ptr2, unsigned int len1, unsigned int len2)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->unlock(ptr1, ptr2, len1, len2);
}


#ifdef FMOD_SUPPORT_RECORDING

// VERIFY: 174774b6 auto
// OFFSET: 0x00938FE0
FMOD_RESULT F_CALLBACK OutputWASAPI::recordGetNumDriversCallback(FMOD_OUTPUT_STATE *output, int *numdrivers)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->recordGetNumDrivers(numdrivers);
}


// VERIFY: e888580f auto
// OFFSET: 0x00939020
FMOD_RESULT F_CALLBACK OutputWASAPI::recordGetDriverInfoCallback(FMOD_OUTPUT_STATE *output, int id, char *name, int namelen)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->recordGetDriverInfo(id, name, namelen);
}


// VERIFY: 1d9b6649 auto
// OFFSET: 0x00939050
FMOD_RESULT F_CALLBACK OutputWASAPI::recordGetDriverCapsCallback(FMOD_OUTPUT_STATE *output, int id, FMOD_CAPS *caps, int *minfrequency, int *maxfrequency)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->recordGetDriverCaps(id, caps, minfrequency, maxfrequency);
}


// VERIFY: a95fc3dd auto
// OFFSET: 0x00939080
FMOD_RESULT F_CALLBACK OutputWASAPI::recordStartCallback(FMOD_OUTPUT_STATE *output, int id, FMOD_SOUND *sound, int loop)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->recordStart(id, (Sound *)sound, loop ? true : false);
}


// VERIFY: e3bc6f8b manual
// OFFSET: 0x009390B0
FMOD_RESULT F_CALLBACK OutputWASAPI::recordStopCallback(FMOD_OUTPUT_STATE *output)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->recordStop();
}


FMOD_RESULT F_CALLBACK OutputWASAPI::recordGetPositionCallback(FMOD_OUTPUT_STATE *output, unsigned int *pcm)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->recordGetPosition(pcm);
}


// VERIFY: 6b4cd25f auto
// OFFSET: 0x009390E0
FMOD_RESULT F_CALLBACK OutputWASAPI::recordLockCallback(FMOD_OUTPUT_STATE *output, unsigned int offset, unsigned int length, void **ptr1, void **ptr2, unsigned int *len1, unsigned int *len2)
{
    OutputWASAPI *wasapi = (OutputWASAPI *)output;

    return wasapi->recordLock(offset, length, ptr1, ptr2, len1, len2);
}

#endif

}

#endif  /* FMOD_SUPPORT_WASAPI */
