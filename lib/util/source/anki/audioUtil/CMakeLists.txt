project(libankiutil LANGUAGES CXX)

include(anki_build_cxx)
include(anki_build_strip)


set(AUDIO_UTIL_SRC
  audioRecognizerProcessor.cpp
  audioRecognizerProcessor.h
  speechRecognizer.cpp
  speechRecognizer.h
  waveFile.cpp
  waveFile.h
)

if (ANDROID)
  set(AUDIO_UTIL_SRC_PLATFORM
    audioCaptureSystem_android.cpp
    audioDataTypes_android.cpp
    audioFileReader_android.cpp
    audioPlayer_android.cpp
  )
elseif(APPLE)
  set(AUDIO_UTIL_SRC_PLATFORM
    audioCaptureSystem_iososx.mm
    audioDataTypes_iososx.mm
    audioFileReader_iososx.mm
    audioPlayer_iososx.mm
  )
else()
  set(AUDIO_UTIL_SRC_PLATFORM "")
endif()

add_library(util_audio
  ${AUDIO_UTIL_SRC}
  ${AUDIO_UTIL_SRC_PLATFORM}
)


set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
if (ANDROID)
  set(PLATFORM_LIBS android log OpenSLES)
elseif(MACOSX)
    find_library(AUDIO_TOOLBOX AudioToolbox)
    find_library(FOUNDATION Foundation)
    set(PLATFORM_LIBS ${AUDIO_TOOLBOX} ${FOUNDATION})
endif()

target_compile_options(util_audio
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_OPTIONS}
)

target_compile_definitions(util_audio
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_DEFINITIONS}
)

target_include_directories(util_audio PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
PRIVATE
  $<BUILD_INTERFACE:${PLATFORM_INCLUDES}> 
)

target_link_libraries(util_audio
  util
  ${PLATFORM_LIBS}
)

anki_build_strip(TARGET util_audio)

#export(TARGETS ankiutil FILE ankiutil-config.cmake)
