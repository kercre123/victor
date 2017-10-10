cmake_minimum_required(VERSION 3.6)
project(engine)

include(opencv)
include(okao)
include(flatbuffers)
include(text2speech)
include(archive)
include(voice-recognition)
include(routing_http_server)
include(android_strip)

include(anki_build_cxx_library)
anki_build_cxx_library(cozmo_engine ${ANKI_SRCLIST_DIR})

if (NOT DEFINED USE_DAS)
  set(USE_DAS 0)
endif()

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
set(PLATFORM_FLAGS "")
if (ANDROID)
  include(breakpad)
  set(PLATFORM_LIBS android log GLESv2)
  set(PLATFORM_COMPILE_DEFS
    THF_FUNCTIONALITY=0
    VC_AUDIOCAPTURE_FUNCTIONALITY=0
  )
elseif (MACOSX)
  find_library(FOUNDATION Foundation)
  include(webots)
  set(PLATFORM_LIBS
    ${FOUNDATION}
    ${OPENCV_LIBS}
    ${ROUTING_HTTP_LIBS}
    ${WEBOTS_LIBS}
  )
  set(PLATFORM_COMPILE_DEFS
    SIMULATOR
    THF_FUNCTIONALITY=1
    VC_AUDIOCAPTURE_FUNCTIONALITY=1
  )
  set(PLATFORM_FLAGS -fobjc-arc)
endif()


target_link_libraries(cozmo_engine
  PUBLIC
  # anki libs
  clad
  util
  cti_common
  cti_messaging
  robot_interface
  audio_engine
  audio_multiplexer_engine
  engine_clad
  PRIVATE
  util_audio
  ble_cozmo
  DAS 
  androidHAL
  # cti
  cti_common
  cti_vision
  cti_planning
  # vendor
  ${FLATBUFFERS_LIBS}
  ${LIBARCHIVE_LIBS}
  ${TEXT2SPEECH_LIBS}
  ${VOICE_RECOGNITION_LIBS}
  ${BREAKPAD_LIBS}
  # system
  z
  # platform
  ${PLATFORM_LIBS}
)

target_compile_options(cozmo_engine PRIVATE
  ${SYMBOL_VISIBILITY_FLAGS}
  "${PLATFORM_FLAGS}"
)

android_strip(TARGET cozmo_engine)

target_compile_definitions(cozmo_engine
  PRIVATE
  COZMO_BASESTATION
  COZMO_V2
  CORETECH_BASESTATION
  USE_DAS=${USE_DAS}
  VOICE_RECOG_NONE=0
  VOICE_RECOG_THF=1
  VOICE_RECOG_PROVIDER=VOICE_RECOG_THF
  "${PLATFORM_COMPILE_DEFS}"
)

target_include_directories(cozmo_engine 
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # allows reference to engine/
  #PRIVATE
  #$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/anki/cozmo/basestation/cozmoAPI>
)

if (ANDROID)
  copy_opencv_android_libs()
  copy_tts_android_libs()
  add_dependencies(cozmo_engine
    copy_opencv_libs
    copy_tts_libs
  )
endif()

if (ANDROID)
  # TODO: support tools on mac
  add_subdirectory(tools)
endif(ANDROID)
