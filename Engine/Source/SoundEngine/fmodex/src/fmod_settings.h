#ifndef _FMOD_SETTINGS_H
#define _FMOD_SETTINGS_H

/*
    PLATFORM SPECIFICATION DEFINES
    Dont alter.  PLATFORM_WINDOWS has to come last because xbox and ce define WIN32 as well.
    PLATFORM_XENON has to come before xbox, because xenon defines _XBOX as well.
*/
#if defined(_XENON)

    #include <xdk.h>    /* For _XDK_VER */

    #define PLATFORM_XENON
    #define PLATFORM_ENDIAN_BIG
    #define PLATFORM_32BIT

#elif defined(_XBOX)

    #define PLATFORM_XBOX
    #define PLATFORM_ENDIAN_LITTLE
    #define PLATFORM_32BIT

#elif defined(_WIN64)

    #define PLATFORM_WINDOWS 
    #define PLATFORM_WINDOWS64
    #define PLATFORM_64BIT
    #define PLATFORM_ENDIAN_LITTLE

#elif defined(_WIN32) || defined(WIN32) || defined(_WINDOWS)

    #define PLATFORM_WINDOWS
    #define PLATFORM_ENDIAN_LITTLE
    #define PLATFORM_32BIT

    /*#define PLATFORM_WINDOWS_PS3MODE*/      /* Emulation of PS3 (uses alternate mixer) */
    /*#define FMOD_SUPPORT_HARDWAREXM*/       /* Emulation of PS2/PSP hardware xm support (uses dsound buffers) */

#elif defined(__linux__)

    #define PLATFORM_LINUX
    #define PLATFORM_ENDIAN_LITTLE

    #if defined(__amd64__)
        #define PLATFORM_LINUX64
        #define PLATFORM_64BIT
    #else
        #define PLATFORM_32BIT
    #endif

#elif defined(__APPLE__)

    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define PLATFORM_IPHONE
        #if TARGET_OS_SIMULATOR
            #define PLATFORM_IPHONE_SIMULATOR
        #else
            #define PLATFORM_IPHONE_DEVICE
        #endif
    #else
        #define PLATFORM_MAC
        #if defined(__i386__) || defined(__x86_64__)
            #define PLATFORM_MAC_INTEL
        #endif
    #endif
    #if defined(__arm64__) || defined(__aarch64__)
        #define PLATFORM_APPLE_ARM64
    #endif
    #if defined(__LP64__)
        #define PLATFORM_64BIT
    #else
        #define PLATFORM_32BIT
    #endif
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define PLATFORM_ENDIAN_LITTLE
    #else
        #define PLATFORM_ENDIAN_BIG
        #define PLATFORM_MAC_PPC
    #endif

#elif defined(__psp__)

    #define PLATFORM_PSP
    #define PLATFORM_ENDIAN_LITTLE
    #define PLATFORM_32BIT

#elif defined(R3000) || defined(R5900) || defined(SN_TARGET_PS2)

    #define PLATFORM_PS2
    #define PLATFORM_ENDIAN_LITTLE
    #define PLATFORM_32BIT

    #ifdef SN_TARGET_PS2
        #define R5900
    #endif
    
    #ifdef R3000
        #define PLATFORM_PS2_IOP
        #define FMOD_NO_FPU
        #define float int
    #endif
    #ifdef R5900
        #define PLATFORM_PS2_EE
    #endif
    
    #ifdef __MWERKS__
        #define PLATFORM_PS2_CODEWARRIOR
    #else
        #define PLATFORM_PS2_GCC
    #endif

#elif defined(SN_TARGET_PS3) || defined(SN_TARGET_PS3_SPU) || defined(_PS3)

    #define PLATFORM_PS3
    #define PLATFORM_ENDIAN_BIG
    #define PLATFORM_32BIT

    #ifdef SN_TARGET_PS3
        #define PLATFORM_PS3_PPU
    #endif

    #ifdef SN_TARGET_PS3_SPU
        #define PLATFORM_PS3_SPU

        #ifdef SPU_DECODE
            #define PLATFORM_PS3_SPU_STREAMDECODE
        #endif
    #endif

#elif defined(GEKKO)

	#if defined(_REVOLUTION)
		#define PLATFORM_WII
		#define PLATFORM_ENDIAN_BIG
		#define PLATFORM_32BIT
	#else
		#define PLATFORM_GC
		#define PLATFORM_ENDIAN_BIG
		#define PLATFORM_32BIT
	#endif

#elif defined(__sun)

    #define PLATFORM_SOLARIS
    #define PLATFORM_ENDIAN_LITTLE
    #define PLATFORM_32BIT

#else

    #error Please define a platform type.

#endif

#if defined(_DEBUG) && !defined(DEBUG)
    #define DEBUG 1
#endif


/*
    PLATFORM INDEPENDENT SETTINGS
    Comment out or undef features you dont wish to include in the FMOD build.
*/

/*
    File formats
*/
#define FMOD_SUPPORT_AIFF
#define FMOD_SUPPORT_ASF
#define FMOD_SUPPORT_CDDA
#define FMOD_SUPPORT_DLS
#define FMOD_SUPPORT_FLAC
#define FMOD_SUPPORT_FSB
#define FMOD_SUPPORT_IMAADPCM
#define FMOD_SUPPORT_IT
#define FMOD_SUPPORT_MIDI
#define FMOD_SUPPORT_MOD
#define FMOD_SUPPORT_MPEG
#define FMOD_SUPPORT_MPEG_LAYER2
#define FMOD_SUPPORT_MPEG_LAYER3
#define FMOD_SUPPORT_OGGVORBIS
/* #define FMOD_SUPPORT_TREMOR */
#define FMOD_SUPPORT_PLAYLIST
#define FMOD_SUPPORT_RAW
#define FMOD_SUPPORT_S3M
/* #define FMOD_SUPPORT_SF2 */
#define FMOD_SUPPORT_TAGS
#define FMOD_SUPPORT_USERCODEC
#define FMOD_SUPPORT_XM
#define FMOD_SUPPORT_WAV
#define FMOD_SUPPORT_VAG

/*
    Miscellaneous
*/
#define FMOD_SUPPORT_SOFTWARE
#define FMOD_SUPPORT_GEOMETRY
#define FMOD_SUPPORT_REVERB
#define FMOD_SUPPORT_RECORDING
#define FMOD_SUPPORT_3DSOUND
#define FMOD_SUPPORT_GETSPECTRUM
#define FMOD_SUPPORT_MEMORYMANAGEMENT       /* Advice is to leave this on unless you really know why you want to remove this */
#define FMOD_SUPPORT_NET
#define FMOD_SUPPORT_FILE
#define FMOD_SUPPORT_WAVWRITER
#define FMOD_SUPPORT_WAVWRITER_NRT
#define FMOD_SUPPORT_NOSOUND
#define FMOD_SUPPORT_NOSOUND_NRT
#define FMOD_SUPPORT_NONBLOCKING
#define FMOD_SUPPORT_STREAMING
#define FMOD_SUPPORT_SENTENCING
#define FMOD_SUPPORT_DLLS
#define FMOD_SUPPORT_PCM8
#define FMOD_SUPPORT_PCM16
#define FMOD_SUPPORT_PCM24
#define FMOD_SUPPORT_PCM32
#define FMOD_SUPPORT_PCMFLOAT
#define FMOD_SUPPORT_NONBLOCKSETPOS

//#define FMOD_SUPPORT_MYEARS

#if defined(DEBUG)
    #define FMOD_SUPPORT_RTTI
#endif

/* #define FMOD_SUPPORT_CMDLOG */

#define FMOD_SUPPORT_SPEAKERMODE_RAW
#define FMOD_SUPPORT_SPEAKERMODE_MONO
#define FMOD_SUPPORT_SPEAKERMODE_STEREO
#define FMOD_SUPPORT_SPEAKERMODE_QUAD
#define FMOD_SUPPORT_SPEAKERMODE_SURROUND
#define FMOD_SUPPORT_SPEAKERMODE_5POINT1
#define FMOD_SUPPORT_SPEAKERMODE_7POINT1
#define FMOD_SUPPORT_SPEAKERMODE_PROLOGIC


/*
    DSP effects and features
*/
#ifdef FMOD_SUPPORT_SOFTWARE
    #define FMOD_SUPPORT_OSCILLATOR
    #define FMOD_SUPPORT_LOWPASS
    #define FMOD_SUPPORT_LOWPASS2
    #define FMOD_SUPPORT_LOWPASS_SIMPLE
    #define FMOD_SUPPORT_HIGHPASS
    #define FMOD_SUPPORT_ECHO
    #define FMOD_SUPPORT_FLANGE
    #define FMOD_SUPPORT_DISTORTION
    #define FMOD_SUPPORT_NORMALIZE
    #define FMOD_SUPPORT_PARAMEQ
    #define FMOD_SUPPORT_PITCHSHIFT
    #define FMOD_SUPPORT_CHORUS
    #define FMOD_SUPPORT_ITECHO
    #define FMOD_SUPPORT_COMPRESSOR
    #define FMOD_SUPPORT_FREEVERB
    #define FMOD_SUPPORT_SFXREVERB   
    #define FMOD_SUPPORT_RESAMPLER_NOINTERP
    #define FMOD_SUPPORT_RESAMPLER_LINEAR
    #define FMOD_SUPPORT_RESAMPLER_CUBIC
    #define FMOD_SUPPORT_RESAMPLER_SPLINE
    #define FMOD_SUPPORT_DSPCODEC
    #define FMOD_SUPPORT_MULTIREVERB
#else
    #undef FMOD_SUPPORT_GETSPECTRUM
    #undef FMOD_SUPPORT_MOD
    #undef FMOD_SUPPORT_XM
    #undef FMOD_SUPPORT_IT
    #undef FMOD_SUPPORT_S3M
    #undef FMOD_SUPPORT_MIDI
#endif


/*
    Event system features
*/

//#define FMOD_EVENT_STRIPPED   // Define this to strip out _all_ extraneous features in the event system

#ifndef __FMOD_EVENT_TOOL
  #ifndef FMOD_EVENT_STRIPPED
    #define FMOD_EVENT_SUPPORT_SPAWN_INTENSITY
    #define FMOD_EVENT_SUPPORT_REVERB_LEVEL
    #define FMOD_EVENT_SUPPORT_FADE
    #define FMOD_EVENT_SUPPORT_TIME_OFFSET
    #define FMOD_EVENT_SUPPORT_STEALPRIORITY
    #define FMOD_EVENT_SUPPORT_POSITION_RANDOMISATION
    #define FMOD_EVENT_SUPPORT_CONES
    #define FMOD_EVENT_SUPPORT_DOPPLER_FACTOR
    #define FMOD_EVENT_SUPPORT_OCCLUSION
    #define FMOD_EVENT_SUPPORT_SPEAKER_SPREAD
    #define FMOD_EVENT_SUPPORT_PAN_LEVEL
    #define FMOD_EVENT_SUPPORT_PITCH_RANDOMISATION
    #define FMOD_EVENT_SUPPORT_VOLUME_RANDOMISATION
    #define FMOD_EVENT_SUPPORT_MUSICSYSTEM
  #endif
#else
  /*
      Don't touch these. FMOD Designer needs them set like this
  */
  #define FMOD_EVENT_SUPPORT_SPAWN_INTENSITY
  #define FMOD_EVENT_SUPPORT_REVERB_LEVEL
  #define FMOD_EVENT_SUPPORT_FADE
  #define FMOD_EVENT_SUPPORT_TIME_OFFSET
  #define FMOD_EVENT_SUPPORT_STEALPRIORITY
  #define FMOD_EVENT_SUPPORT_POSITION_RANDOMISATION
  #define FMOD_EVENT_SUPPORT_CONES
  #define FMOD_EVENT_SUPPORT_DOPPLER_FACTOR
  #define FMOD_EVENT_SUPPORT_OCCLUSION
  #define FMOD_EVENT_SUPPORT_SPEAKER_SPREAD
  #define FMOD_EVENT_SUPPORT_PAN_LEVEL
  #define FMOD_EVENT_SUPPORT_PITCH_RANDOMISATION
  #define FMOD_EVENT_SUPPORT_VOLUME_RANDOMISATION
  #define FMOD_EVENT_SUPPORT_MUSICSYSTEM
#endif


/*
    These are extra plugins we support but only as plugins.  They are not included in the main fmod4.dll.
*/
#ifdef PLUGIN_EXPORTS

    #ifndef PLUGIN_NEEDS_CDDA
        #undef FMOD_SUPPORT_CDDA
    #endif

#endif


/*
    DEBUG SETTINGS
*/

#ifdef DEBUG
    #define FMOD_DEBUG      /* Enabling this writes output to FMOD.log or client program.  See system_debug.c. */
#endif

/*
    PLATFORM DEPENDENT SETTINGS 
*/

#if defined(PLATFORM_WINDOWS64)

    #ifdef FMOD_SUPPORT_SOFTWARE        /* WASAPI support needs software to run. */
        #define FMOD_SUPPORT_WASAPI
    #endif
    #define FMOD_SUPPORT_DSOUND
    #define FMOD_SUPPORT_WINMM
    #ifdef FMOD_SUPPORT_SOFTWARE        /* OpenAL support needs software to run. */
        #define FMOD_SUPPORT_OPENAL
    #endif
    #define FMOD_SUPPORT_EAX
    #define FMOD_SUPPORT_PRAGMAPACK

    #undef FMOD_SUPPORT_CELT

#elif defined(PLATFORM_WINDOWS)

    #ifdef FMOD_SUPPORT_SOFTWARE        /* WASAPI support needs software to run. */
        #define FMOD_SUPPORT_WASAPI
    #endif
    #define FMOD_SUPPORT_DSOUND
    #define FMOD_SUPPORT_WINMM
    #ifdef FMOD_SUPPORT_SOFTWARE        /* OpenAL support needs software to run. */
        #define FMOD_SUPPORT_OPENAL
    #endif
    #define FMOD_SUPPORT_ASIO             /* Asio support */
    #define FMOD_SUPPORT_EAX
    /* #define FMOD_SUPPORT_I3DL2 */
    #define FMOD_SUPPORT_VSTPLUGIN
    #define FMOD_SUPPORT_WINAMPPLUGIN
    #define FMOD_SUPPORT_PRAGMAPACK
    #define FMOD_SUPPORT_SIMD

    #ifdef __MINGW32__
        #undef FMOD_SUPPORT_ASF
    #endif

    #ifdef PLATFORM_WINDOWS_PS3MODE
        #define FMOD_SUPPORT_RAWCODEC
        #define FMOD_SUPPORT_MIXER_NONRECURSIVE
        #undef FMOD_SUPPORT_OPENAL
        #undef FMOD_SUPPORT_PITCHSHIFT
        #undef FMOD_SUPPORT_FLAC
        #undef FMOD_SUPPORT_RECORDING
        #undef FMOD_SUPPORT_RTTI
        #undef FMOD_SUPPORT_DLS
        #undef FMOD_SUPPORT_IT
        #undef FMOD_SUPPORT_MIDI
        #undef FMOD_SUPPORT_MOD
        #undef FMOD_SUPPORT_S3M
        #undef FMOD_SUPPORT_XM
        #undef FMOD_SUPPORT_WASAPI
    #endif

#elif defined(PLATFORM_LINUX64)

    #define FMOD_SUPPORT_OSS
    #define FMOD_SUPPORT_ALSA
    #define FMOD_SUPPORT_ESD
    #define FMOD_SUPPORT_NEURAL

    #undef  FMOD_SUPPORT_ASF

    #undef FMOD_SUPPORT_MYEARS

#elif defined(PLATFORM_LINUX)

    #define FMOD_SUPPORT_OSS
    #define FMOD_SUPPORT_ALSA
    #define FMOD_SUPPORT_ESD
    #define FMOD_SUPPORT_SIMD
    #define FMOD_SUPPORT_NEURAL

    #undef  FMOD_SUPPORT_ASF

    #undef FMOD_SUPPORT_MYEARS

#elif defined(PLATFORM_XBOX)

    #define FMOD_SUPPORT_XWMA
    #define FMOD_SUPPORT_PRAGMAPACK
    #define FMOD_SUPPORT_SIMD

    #undef FMOD_SUPPORT_RECORDING    
    #undef FMOD_SUPPORT_ASF
    #undef FMOD_SUPPORT_FLAC
    #undef FMOD_SUPPORT_CDDA
    #undef FMOD_SUPPORT_WAVWRITER
    #undef FMOD_SUPPORT_WAVWRITER_NRT
	#undef FMOD_SUPPORT_PROFILE

    #undef FMOD_SUPPORT_MYEARS

    #undef FMOD_SUPPORT_CELT

#elif defined(PLATFORM_XENON)

    #define FMOD_SUPPORT_CMIXER           /* Use C Mixers */
    #define FMOD_SUPPORT_SIMD
    #define FMOD_SUPPORT_PRAGMAPACK
    #define FMOD_SUPPORT_I3DL2
    #define FMOD_SUPPORT_XMA
    #define FMOD_SUPPORT_XMA_NEWHAL
    #ifdef  FMOD_SUPPORT_SIMD
	    #define FMOD_SUPPORT_MPEG_SIMD        /* SIMD based decoding */
    #endif

    #undef FMOD_SUPPORT_ASF
    #undef FMOD_SUPPORT_NET
    #undef FMOD_SUPPORT_CDDA
    #undef FMOD_SUPPORT_WAVWRITER
    #undef FMOD_SUPPORT_WAVWRITER_NRT

    #undef FMOD_SUPPORT_MYEARS

    #define FMOD_SUPPORT_NEURAL

    #if _XDK_VER <= 7978
        #undef FMOD_SUPPORT_RECORDING
    #endif


#elif defined(PLATFORM_PS2)

    #define FMOD_SUPPORT_HARDWAREXM

    #undef FMOD_SUPPORT_AIFF
    #undef FMOD_SUPPORT_RECORDING
    #undef FMOD_SUPPORT_ASF
    #undef FMOD_SUPPORT_CDDA
    #undef FMOD_SUPPORT_NET
    #undef FMOD_SUPPORT_PROFILE
    #undef FMOD_SUPPORT_FLAC
    #undef FMOD_SUPPORT_DLLS
    #undef FMOD_SUPPORT_SF2
    #undef FMOD_SUPPORT_TAGS
    #undef FMOD_SUPPORT_WAVWRITER
    #undef FMOD_SUPPORT_WAVWRITER_NRT
    #undef FMOD_SUPPORT_NOSOUND
    #undef FMOD_SUPPORT_NOSOUND_NRT
    #undef FMOD_SUPPORT_FREEVERB
    #undef FMOD_SUPPORT_SFXREVERB
    #undef FMOD_SUPPORT_MULTIREVERB
    #undef FMOD_SUPPORT_CELT

    #undef FMOD_SUPPORT_MYEARS
    
    #undef FMOD_SUPPORT_RESAMPLER_CUBIC
    #undef FMOD_SUPPORT_RESAMPLER_SPLINE

    #ifdef PLATFORM_PS2_IOP
        #undef FMOD_SUPPORT_MEMORYTRACKER
    #endif

    #ifdef STRIPPED
        #undef FMOD_SUPPORT_SOFTWARE
        #undef FMOD_SUPPORT_DSPCODEC

        #undef FMOD_SUPPORT_MOD
        #undef FMOD_SUPPORT_S3M
        #undef FMOD_SUPPORT_XM
        #undef FMOD_SUPPORT_IT
        #undef FMOD_SUPPORT_MIDI
        #undef FMOD_SUPPORT_WAV
        #undef FMOD_SUPPORT_MPEG
        #undef FMOD_SUPPORT_OGGVORBIS
        #undef FMOD_SUPPORT_TREMOR
        #undef FMOD_SUPPORT_DLS
        #undef FMOD_SUPPORT_IMAADPCM
        #undef FMOD_SUPPORT_PLAYLIST
        #undef FMOD_SUPPORT_RAW
        #undef FMOD_SUPPORT_TAGS

        #undef FMOD_SUPPORT_OSCILLATOR
        #undef FMOD_SUPPORT_LOWPASS
        #undef FMOD_SUPPORT_LOWPASS2
        #undef FMOD_SUPPORT_LOWPASS_SIMPLE
        #undef FMOD_SUPPORT_HIGHPASS
        #undef FMOD_SUPPORT_ECHO
        #undef FMOD_SUPPORT_DELAY
        #undef FMOD_SUPPORT_FLANGE
        #undef FMOD_SUPPORT_TREMOLO
        #undef FMOD_SUPPORT_DISTORTION
        #undef FMOD_SUPPORT_NORMALIZE
        #undef FMOD_SUPPORT_PARAMEQ
        #undef FMOD_SUPPORT_PITCHSHIFT
        #undef FMOD_SUPPORT_CHORUS
        #undef FMOD_SUPPORT_FREEVERB
        #undef FMOD_SUPPORT_ITECHO
        #undef FMOD_SUPPORT_COMPRESSOR
        #undef FMOD_SUPPORT_SFXREVERB
        #undef FMOD_SUPPORT_GEOMETRY
        #undef FMOD_SUPPORT_GEOMETRY_THREADED
        #undef FMOD_SUPPORT_GETSPECTRUM
        #undef FMOD_SUPPORT_DOLBYHEADPHONES
    #endif

    #ifdef CODEWARRIOR
        #define FMOD_SUPPORT_PRAGMAPACK
    #endif

#elif defined(PLATFORM_PS3)

    #define FMOD_SUPPORT_SPURS            /* Support SPURS */
    #define FMOD_SUPPORT_SIMD
    #define FMOD_SUPPORT_RAWCODEC

    #undef FMOD_SUPPORT_FLAC
    #undef FMOD_SUPPORT_ASF
    #undef FMOD_SUPPORT_NET
    #undef FMOD_SUPPORT_CDDA
    #undef FMOD_SUPPORT_WAVWRITER
    #undef FMOD_SUPPORT_WAVWRITER_NRT
    #undef FMOD_SUPPORT_RTTI
    #undef FMOD_SUPPORT_DLLS

    #undef FMOD_SUPPORT_ASF
    #undef FMOD_SUPPORT_CDDA
    #undef FMOD_SUPPORT_FLAC
    #undef FMOD_SUPPORT_PLAYLIST

    #undef FMOD_SUPPORT_MYEARS

    #define FMOD_SUPPORT_NEURAL

    #define FMOD_SUPPORT_MPEG_SONYDECODER

    #ifdef FMOD_SUPPORT_MPEG_SONYDECODER
        #undef FMOD_SUPPORT_MPEG_LAYER2
    #endif

    /*
        DSP effects and features
    */
    #undef FMOD_SUPPORT_FREEVERB
    #undef FMOD_SUPPORT_RESAMPLER_NOINTERP
    #undef FMOD_SUPPORT_RESAMPLER_CUBIC
    #undef FMOD_SUPPORT_RESAMPLER_SPLINE

    #define FMOD_SUPPORT_MPEG_SPU    /* SPU based mpeg decoding */

    #ifdef PLATFORM_PS3_SPU

        #define FMOD_SUPPORT_MIXER_NONRECURSIVE

        #define FMOD_SUPPORT_SIMD
        #define FMOD_SUPPORT_MPEG_SIMD

        #undef FMOD_SUPPORT_PCM8
        #undef FMOD_SUPPORT_PCM24
        #undef FMOD_SUPPORT_PCM32

        #undef FMOD_SUPPORT_SPEAKERMODE_RAW
        #undef FMOD_SUPPORT_SPEAKERMODE_MONO
        #undef FMOD_SUPPORT_SPEAKERMODE_STEREO
        #undef FMOD_SUPPORT_SPEAKERMODE_QUAD
        #undef FMOD_SUPPORT_SPEAKERMODE_SURROUND
        #undef FMOD_SUPPORT_SPEAKERMODE_5POINT1
        #undef FMOD_SUPPORT_SPEAKERMODE_PROLOGIC

        #undef FMOD_SUPPORT_MPEG_SPU   /* It is supported, just we don't want to build ppu code on spu */

        /*
            Miscellaneous
        */
        #undef FMOD_SUPPORT_GEOMETRY
        #undef FMOD_SUPPORT_GEOMETRY_THREADED
        #undef FMOD_SUPPORT_RECORDING
        #undef FMOD_SUPPORT_3DSOUND
        #undef FMOD_SUPPORT_GETSPECTRUM
        #undef FMOD_SUPPORT_MEMORYMANAGEMENT
        #undef FMOD_SUPPORT_NET
        #undef FMOD_SUPPORT_PROFILE
        #undef FMOD_SUPPORT_FILE
        #undef FMOD_SUPPORT_WAVWRITER
        #undef FMOD_SUPPORT_WAVWRITER_NRT
        #undef FMOD_SUPPORT_NOSOUND
        #undef FMOD_SUPPORT_NOSOUND_NRT
        #undef FMOD_SUPPORT_NONBLOCKING
    #endif

    #undef FMOD_SUPPORT_CELT

#elif defined(PLATFORM_IPHONE)
 
    #define FMOD_SUPPORT_PRAGMAPACK
    #if defined(PLATFORM_IPHONE_DEVICE) && !defined(PLATFORM_APPLE_ARM64)
        #define FMOD_SUPPORT_SIMD
    #endif
    
    #undef FMOD_SUPPORT_ASF
	#undef FMOD_SUPPORT_VAG
    #undef FMOD_SUPPORT_CDDA
    #undef FMOD_SUPPORT_DLLS
    #if !defined(PLATFORM_APPLE_ARM64)
        #undef FMOD_SUPPORT_RESAMPLER_CUBIC
        #undef FMOD_SUPPORT_RESAMPLER_SPLINE
    #endif

    #ifdef STRIPPED
        /* Support FSB, ADPCM, WAV, User Codec and IT, remove everything else */
        #undef FMOD_SUPPORT_AIFF
        #undef FMOD_SUPPORT_DLS
        #undef FMOD_SUPPORT_FLAC
        #undef FMOD_SUPPORT_MIDI
        #undef FMOD_SUPPORT_MOD
        #undef FMOD_SUPPORT_MPEG
        #undef FMOD_SUPPORT_MPEG_LAYER2
        #undef FMOD_SUPPORT_MPEG_LAYER3
        #undef FMOD_SUPPORT_OGGVORBIS
        #undef FMOD_SUPPORT_PLAYLIST
        #undef FMOD_SUPPORT_RAW
        #undef FMOD_SUPPORT_S3M
        #undef FMOD_SUPPORT_TAGS
        #undef FMOD_SUPPORT_XM

        /* Remove all effects */
        #undef FMOD_SUPPORT_OSCILLATOR
        #undef FMOD_SUPPORT_LOWPASS
        #undef FMOD_SUPPORT_LOWPASS2
        #undef FMOD_SUPPORT_LOWPASS_SIMPLE
        #undef FMOD_SUPPORT_HIGHPASS
        #undef FMOD_SUPPORT_ECHO
        #undef FMOD_SUPPORT_DELAY
        #undef FMOD_SUPPORT_FLANGE
        #undef FMOD_SUPPORT_TREMOLO
        #undef FMOD_SUPPORT_DISTORTION
        #undef FMOD_SUPPORT_NORMALIZE
        #undef FMOD_SUPPORT_PARAMEQ
        #undef FMOD_SUPPORT_PITCHSHIFT
        #undef FMOD_SUPPORT_CHORUS
        #undef FMOD_SUPPORT_REVERB
        #undef FMOD_SUPPORT_ITECHO
        #undef FMOD_SUPPORT_COMPRESSOR
        #undef FMOD_SUPPORT_FREEVERB
        #undef FMOD_SUPPORT_SFXREVERB   
        #undef FMOD_SUPPORT_MULTIREVERB
    
        #undef FMOD_SUPPORT_GEOMETRY
        #undef FMOD_SUPPORT_GEOMETRY_THREADED
        #undef FMOD_SUPPORT_GETSPECTRUM
    #endif

    #undef FMOD_SUPPORT_CELT

#elif defined(PLATFORM_MAC)

    #define FMOD_SUPPORT_COREAUDIO
    // This port has no physical audio-CD device backend. Archive/file codecs remain enabled.
    #undef FMOD_SUPPORT_CDDA
    #define FMOD_SUPPORT_VSTPLUGIN   
    #define FMOD_SUPPORT_PRAGMAPACK
    #define FMOD_SUPPORT_NEURAL

    #undef FMOD_SUPPORT_ASF
	#undef FMOD_SUPPORT_VAG
      
#elif defined(PLATFORM_GC)

    #define FMOD_SUPPORT_GCADPCM          /* Gamecube ADPCM format */
    #define FMOD_SUPPORT_GCDVD            /* Gamecube hardware DVD streaming format */
    #define FMOD_SUPPORT_PRAGMAPACK

    #define FMOD_SUPPORT_GCADPCM_DSP      /* Gamecube ADPCM DSP file format */

    #undef  FMOD_SUPPORT_RECORDING
    #undef  FMOD_SUPPORT_NET
    #undef  FMOD_SUPPORT_PROFILE
    #undef  FMOD_SUPPORT_ASF
    #undef  FMOD_SUPPORT_CDDA
    #undef  FMOD_SUPPORT_FLAC
    #undef  FMOD_SUPPORT_VAG
    #undef  FMOD_SUPPORT_WAVWRITER
    #undef  FMOD_SUPPORT_WAVWRITER_NRT
    #undef  FMOD_SUPPORT_SFXREVERB
    #undef  FMOD_SUPPORT_MULTIREVERB

    #undef FMOD_SUPPORT_MYEARS

    #ifdef STRIPPED
        #undef FMOD_SUPPORT_SOFTWARE
        #undef FMOD_SUPPORT_DSPCODEC

        #undef FMOD_SUPPORT_MOD
        #undef FMOD_SUPPORT_S3M
        #undef FMOD_SUPPORT_XM
        #undef FMOD_SUPPORT_IT
        #undef FMOD_SUPPORT_MIDI
        #undef FMOD_SUPPORT_WAV
        #undef FMOD_SUPPORT_MPEG
        #undef FMOD_SUPPORT_OGGVORBIS
        #undef FMOD_SUPPORT_TREMOR
        #undef FMOD_SUPPORT_DLS
        #undef FMOD_SUPPORT_IMAADPCM
        #undef FMOD_SUPPORT_PLAYLIST
        #undef FMOD_SUPPORT_RAW
        #undef FMOD_SUPPORT_TAGS

        #undef FMOD_SUPPORT_OSCILLATOR
        #undef FMOD_SUPPORT_LOWPASS
        #undef FMOD_SUPPORT_LOWPASS2
        #undef FMOD_SUPPORT_LOWPASS_SIMPLE
        #undef FMOD_SUPPORT_HIGHPASS
        #undef FMOD_SUPPORT_ECHO
        #undef FMOD_SUPPORT_DELAY
        #undef FMOD_SUPPORT_FLANGE
        #undef FMOD_SUPPORT_TREMOLO
        #undef FMOD_SUPPORT_DISTORTION
        #undef FMOD_SUPPORT_NORMALIZE
        #undef FMOD_SUPPORT_PARAMEQ
        #undef FMOD_SUPPORT_PITCHSHIFT
        #undef FMOD_SUPPORT_CHORUS
        #undef FMOD_SUPPORT_REVERB
        #undef FMOD_SUPPORT_ITECHO
        #undef FMOD_SUPPORT_COMPRESSOR
        #undef FMOD_SUPPORT_GEOMETRY
        #undef FMOD_SUPPORT_GEOMETRY_THREADED
        #undef FMOD_SUPPORT_GETSPECTRUM
        #undef FMOD_SUPPORT_DOLBYHEADPHONES
    #endif

    #undef FMOD_SUPPORT_CELT

#elif defined (PLATFORM_WII)

    #pragma old_friend_lookup on 

    #define FMOD_SUPPORT_SIMD

    #define FMOD_SUPPORT_GCADPCM          /* Wii ADPCM format */
    #define FMOD_SUPPORT_PRAGMAPACK

	#define FMOD_SUPPORT_GCADPCM_DSP	  /* Wii ADPCM DSP file format */

    #undef  FMOD_SUPPORT_NET
    #undef  FMOD_SUPPORT_PROFILE
    #undef  FMOD_SUPPORT_ASF
    #undef  FMOD_SUPPORT_CDDA
    #undef  FMOD_SUPPORT_FLAC
    #undef  FMOD_SUPPORT_VAG
    #undef  FMOD_SUPPORT_WAVWRITER
    #undef  FMOD_SUPPORT_WAVWRITER_NRT

    #undef FMOD_SUPPORT_MYEARS

    #ifdef STRIPPED
        #undef FMOD_SUPPORT_SOFTWARE
        #undef FMOD_SUPPORT_DSPCODEC

        #undef FMOD_SUPPORT_MOD
        #undef FMOD_SUPPORT_S3M
        #undef FMOD_SUPPORT_XM
        #undef FMOD_SUPPORT_IT
        #undef FMOD_SUPPORT_MIDI
        #undef FMOD_SUPPORT_WAV
        #undef FMOD_SUPPORT_MPEG
        #undef FMOD_SUPPORT_OGGVORBIS
        #undef FMOD_SUPPORT_TREMOR
        #undef FMOD_SUPPORT_DLS
        #undef FMOD_SUPPORT_IMAADPCM
        #undef FMOD_SUPPORT_PLAYLIST
        #undef FMOD_SUPPORT_RAW
        #undef FMOD_SUPPORT_TAGS

        #undef FMOD_SUPPORT_OSCILLATOR
        #undef FMOD_SUPPORT_LOWPASS
        #undef FMOD_SUPPORT_LOWPASS2
        #undef FMOD_SUPPORT_LOWPASS_SIMPLE
        #undef FMOD_SUPPORT_HIGHPASS
        #undef FMOD_SUPPORT_ECHO
        #undef FMOD_SUPPORT_DELAY
        #undef FMOD_SUPPORT_FLANGE
        #undef FMOD_SUPPORT_TREMOLO
        #undef FMOD_SUPPORT_DISTORTION
        #undef FMOD_SUPPORT_NORMALIZE
        #undef FMOD_SUPPORT_PARAMEQ
        #undef FMOD_SUPPORT_PITCHSHIFT
        #undef FMOD_SUPPORT_CHORUS
        #undef FMOD_SUPPORT_ITECHO
        #undef FMOD_SUPPORT_COMPRESSOR
        #undef FMOD_SUPPORT_GEOMETRY
        #undef FMOD_SUPPORT_GEOMETRY_THREADED
        #undef FMOD_SUPPORT_GETSPECTRUM
        #undef FMOD_SUPPORT_DOLBYHEADPHONES
        #undef FMOD_SUPPORT_FREEVERB
        #undef FMOD_SUPPORT_SFXREVERB
        #undef FMOD_SUPPORT_MULTIREVERB
    #endif

    #undef FMOD_SUPPORT_CELT

#elif defined(PLATFORM_PSP)

    #define FMOD_SUPPORT_AT3
    #define FMOD_SUPPORT_MPEGPSP
    #define FMOD_SUPPORT_HARDWAREXM

    #define FMOD_SUPPORT_SIMD

    #undef FMOD_SUPPORT_DLLS
    #undef FMOD_SUPPORT_RTTI

    #undef FMOD_SUPPORT_AIFF
    #undef FMOD_SUPPORT_RECORDING
    #undef FMOD_SUPPORT_ASF
    #undef FMOD_SUPPORT_CDDA
    #undef FMOD_SUPPORT_NET
    #undef FMOD_SUPPORT_PROFILE
    #undef FMOD_SUPPORT_FLAC
    #undef FMOD_SUPPORT_SF2
    #undef FMOD_SUPPORT_TAGS
    /* #undef FMOD_SUPPORT_OGGVORBIS */
    /* #undef FMOD_SUPPORT_TREMOR */
    #undef FMOD_SUPPORT_MPEG
    #undef FMOD_SUPPORT_WAVWRITER
    #undef FMOD_SUPPORT_WAVWRITER_NRT
    #undef FMOD_SUPPORT_NOSOUND
    #undef FMOD_SUPPORT_NOSOUND_NRT
    
    #undef FMOD_SUPPORT_RESAMPLER_CUBIC
    #undef FMOD_SUPPORT_RESAMPLER_SPLINE
    #undef FMOD_SUPPORT_FREEVERB
    #undef FMOD_SUPPORT_SFXREVERB
    #undef FMOD_SUPPORT_MULTIREVERB

    #undef FMOD_SUPPORT_MYEARS

    #ifdef STRIPPED
        #undef FMOD_SUPPORT_SOFTWARE
        #undef FMOD_SUPPORT_DSPCODEC

        #undef FMOD_SUPPORT_MOD
        #undef FMOD_SUPPORT_S3M
        #undef FMOD_SUPPORT_XM
        #undef FMOD_SUPPORT_IT
        #undef FMOD_SUPPORT_MIDI
        #undef FMOD_SUPPORT_WAV
        #undef FMOD_SUPPORT_MPEG
        #undef FMOD_SUPPORT_OGGVORBIS
        #undef FMOD_SUPPORT_TREMOR
        #undef FMOD_SUPPORT_DLS
        #undef FMOD_SUPPORT_IMAADPCM
        #undef FMOD_SUPPORT_PLAYLIST
        #undef FMOD_SUPPORT_RAW
        #undef FMOD_SUPPORT_TAGS

        #undef FMOD_SUPPORT_OSCILLATOR
        #undef FMOD_SUPPORT_LOWPASS
        #undef FMOD_SUPPORT_LOWPASS2
        #undef FMOD_SUPPORT_LOWPASS_SIMPLE
        #undef FMOD_SUPPORT_HIGHPASS
        #undef FMOD_SUPPORT_ECHO
        #undef FMOD_SUPPORT_DELAY
        #undef FMOD_SUPPORT_FLANGE
        #undef FMOD_SUPPORT_TREMOLO
        #undef FMOD_SUPPORT_DISTORTION
        #undef FMOD_SUPPORT_NORMALIZE
        #undef FMOD_SUPPORT_PARAMEQ
        #undef FMOD_SUPPORT_PITCHSHIFT
        #undef FMOD_SUPPORT_CHORUS
        #undef FMOD_SUPPORT_FREEVERB
        #undef FMOD_SUPPORT_ITECHO
        #undef FMOD_SUPPORT_COMPRESSOR
        #undef FMOD_SUPPORT_FREEVERB
        #undef FMOD_SUPPORT_SFXREVERB
        #undef FMOD_SUPPORT_GEOMETRY
        #undef FMOD_SUPPORT_GEOMETRY_THREADED
        #undef FMOD_SUPPORT_GETSPECTRUM
        #undef FMOD_SUPPORT_DOLBYHEADPHONES
    #endif

    #ifdef FMOD_SUPPORT_MPEGPSP
        #define FMOD_SUPPORT_TAGS
    #endif

    #ifdef FMOD_SUPPORT_MPEGPSP
        #define FMOD_SUPPORT_TAGS
    #endif

    #ifdef CODEWARRIOR
        #define FMOD_SUPPORT_PRAGMAPACK
    #endif

    #undef FMOD_SUPPORT_CELT

#elif defined(PLATFORM_SOLARIS)

    #define FMOD_SUPPORT_OSS
    #define FMOD_SUPPORT_NEURAL

    #undef  FMOD_SUPPORT_ASF

    #undef FMOD_SUPPORT_MYEARS

    #undef FMOD_SUPPORT_CELT
#else
    
    #error Please set up a platform dependent settings field

#endif

#ifdef FMOD_STATICFORPLUGINS

    #undef FMOD_SUPPORT_DSOUND
    #undef FMOD_SUPPORT_WINMM
    #undef FMOD_SUPPORT_WASAPI
    #undef FMOD_SUPPORT_OPENAL
    #undef FMOD_SUPPORT_EAX
    #undef FMOD_SUPPORT_ASIO
    #undef FMOD_SUPPORT_OSS
    #undef FMOD_SUPPORT_ALSA
    #undef FMOD_SUPPORT_ESD
    #undef FMOD_SUPPORT_WAVWRITER
    #undef FMOD_SUPPORT_WAVWRITER_NRT
    #undef FMOD_SUPPORT_GEOMETRY
    #undef FMOD_SUPPORT_GEOMETRY_THREADED
    #undef FMOD_SUPPORT_VSTPLUGIN
    #undef FMOD_SUPPORT_WINAMPPLUGIN
    #undef FMOD_SUPPORT_DSPCODEC
    #undef FMOD_SUPPORT_REVERB
    #undef FMOD_SUPPORT_MULTIREVERB
	#undef FMOD_SUPPORT_PROFILE
    #undef FMOD_SUPPORT_NONBLOCKING
    #undef FMOD_SUPPORT_STREAMING
    #undef FMOD_SUPPORT_GETSPECTRUM

#endif

#ifdef PLUGIN_MPEG
    #undef FMOD_SUPPORT_IMAADPCM
    #undef FMOD_SUPPORT_XMA
    #undef FMOD_SUPPORT_RAW
    #undef FMOD_SUPPORT_CELT
#endif
#ifdef PLUGIN_WAV
    #undef FMOD_SUPPORT_XMA
    #undef FMOD_SUPPORT_RAW
    #undef FMOD_SUPPORT_MPEG
    #undef FMOD_SUPPORT_CELT
#endif
#ifdef PLUGIN_FSB
    #undef FMOD_SUPPORT_RAW
#endif

/*
    STACK SIZES FOR FMOD THREADS
*/

#if defined(FMOD_SUPPORT_MPEG) || defined(FMOD_SUPPORT_CELT)   /* Mpeg uses about 10k in local arrays in CodecMPEG::decodeLayer3 which is hard to remove. */
    #define STREAM_STACKSIZE       (32 * 1024)
    #ifdef FMOD_DEBUG
        #define ASYNC_STACKSIZE    (64 * 1024)
    #else
        #define ASYNC_STACKSIZE    (32 * 1024)
    #endif
    #define MIXER_STACKSIZE        (32 * 1024)
#else
    #define STREAM_STACKSIZE       (16 * 1024)
    #define ASYNC_STACKSIZE        (32 * 1024)
    #define MIXER_STACKSIZE        (16 * 1024)
#endif

#if defined(PLATFORM_GC) || defined(PLATFORM_WII)
    #define FILE_STACKSIZE     (16 * 1024)  /* Nintendo file libraries use a lot of stack */
#else
    #define FILE_STACKSIZE     (8  * 1024)
#endif

#define GEOMETRY_STACKSIZE (16 * 1024)

/*
    THREAD PRIORITIES FOR FMOD THREADS
*/

#define MIXER_THREADPRIORITY    Thread::PRIORITY_CRITICAL       /* Absolutely must get through. */
#define STREAM_THREADPRIORITY   Thread::PRIORITY_VERYHIGH       /* Pretty high, stuttering may result. */
#define FILE_THREADPRIORITY     Thread::PRIORITY_HIGH           /* Above normal, otherwise it may not trigger on cooperative systems. */
#define ASYNC_THREADPRIORITY    Thread::PRIORITY_HIGH           /* Above normal, otherwise it may not trigger on cooperative systems. */
#define GEOMETRY_THREADPRIORITY Thread::PRIORITY_LOW            /* Low priority background thread. */

#endif	/* _FMOD_SETTINGS_H */


