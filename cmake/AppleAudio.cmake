# Build the original software mixer/decoders independently of its OS boundary.
# Apple output, threads and files are implemented at the FMOD_OS_* boundary.
enable_language(OBJCXX)
set(FMODEX_DIR ${CMAKE_SOURCE_DIR}/Engine/Source/SoundEngine/fmodex)
set(FMODEX_FLAC_DIR ${FMODEX_DIR}/lib/flac-1.1.4)
file(GLOB FMODEX_APPLE_SOURCES CONFIGURE_DEPENDS
    "${FMODEX_DIR}/src/*.cpp"
    "${FMODEX_DIR}/lib/ogg_vorbis/ogg/src/*.c"
    "${FMODEX_DIR}/lib/ogg_vorbis/vorbis/lib/*.c"
    "${FMODEX_FLAC_DIR}/src/libFLAC/*.c"
    "${FMODEX_DIR}/lib/freeverb/*.cpp"
    "${FMODEX_DIR}/lib/sfx/foreverb/*.cpp"
)
add_library(fmodex STATIC ${FMODEX_APPLE_SOURCES}
    # These files contain portable scalar implementations alongside x86 branches.
    ${FMODEX_DIR}/win32/src/fmod_dsp_resampler_linear.cpp
    ${FMODEX_DIR}/win32/src/fmod_dsp_distortion.cpp
    ${FMODEX_DIR}/apple/OS.mm
    ${FMODEX_DIR}/apple/Network.cpp
    ${FMODEX_DIR}/apple/Output.mm
)
target_compile_options(fmodex PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
target_link_libraries(fmodex PUBLIC "-framework AudioToolbox" "-framework Foundation")
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_link_libraries(fmodex PUBLIC "-framework AVFAudio")
else ()
    target_link_libraries(fmodex PUBLIC "-framework CoreAudio")
endif ()
set_target_properties(fmodex PROPERTIES CXX_STANDARD 14)
target_include_directories(fmodex PUBLIC ${FMODEX_DIR}/src PRIVATE
    ${FMODEX_DIR}/lib/ogg_vorbis/ogg/include
    ${FMODEX_DIR}/lib/ogg_vorbis/vorbis/include
    ${FMODEX_DIR}/lib/ogg_vorbis/vorbis/lib
    ${FMODEX_FLAC_DIR}/include
    ${FMODEX_FLAC_DIR}/src/libFLAC/include
    ${FMODEX_DIR}/lib/sfx/foreverb
    ${FMODEX_DIR}/lib/freeverb
)
target_compile_definitions(fmodex PRIVATE FLAC__NO_DLL FLAC__HAS_OGG=1)
target_link_libraries(SoundEngine PUBLIC fmodex)

add_library(AppleAudioProbeCore STATIC Tools/AppleBootstrap/AudioProbe.cpp)
target_link_libraries(AppleAudioProbeCore PUBLIC fmodex AppleArchives)
if (NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_executable(AppleAudioProbe Tools/AppleBootstrap/audio_main.cpp)
    target_link_libraries(AppleAudioProbe PRIVATE AppleAudioProbeCore)
endif ()
