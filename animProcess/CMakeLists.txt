cmake_minimum_required(VERSION 3.6)

project(animProcess)

# Build options
option(USE_DAS "Enable DAS reporting" OFF)

include(anki_build_cxx)
include(aubio)
include(flatbuffers)
include(opencv)
include(text2speech)
include(sensory)
include(robot_core)

anki_build_cxx_library(victor_anim ${ANKI_SRCLIST_DIR} STATIC)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES ${OPENCV_INCLUDE_PATHS})
set(PLATFORM_COMPILE_DEFS "")

if (ANDROID)
    include(breakpad)
    set(PLATFORM_LIBS
        android
        log
        robot_core)
elseif (MACOSX)
    include(webots)
    set(PLATFORM_LIBS
        ${OPENCV_LIBS}
        ${WEBOTS_LIBS}
    )
    set(PLATFORM_COMPILE_DEFS "-DSIMULATOR")
endif()

target_link_libraries(victor_anim
  PRIVATE
  util
  util_audio
  canned_anim_lib_anim
  # cti
  cti_common_robot
  cti_vision
  cti_messaging_robot
  robot_clad_cpplite
  robot_transport
  cloud_clad_cpp
  audio_engine
  audio_multiplexer_robot
  osState
  micdata
  DAS
  # vendor
  ${OPENCV_LIBS}
  ${FLATBUFFERS_LIBS}
  signal_essence
  ${TEXT2SPEECH_LIBS}
  ${SENSORY_LIBS}
  # platform
  ${PLATFORM_LIBS}
  # Aubio beat detection
  ${AUBIO_LIBS}
  PUBLIC
  victor_web_library
  robot_interface  # Needs to be public for cozmoConfig.h
)

android_strip(TARGET victor_anim)

target_compile_definitions(victor_anim
  PRIVATE
  USE_DAS=$<BOOL:${USE_DAS}>
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(victor_anim
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src> # allow "android/" prefix
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib/util/source/3rd>
  ${PLATFORM_INCLUDES}
)

# victor_animator binary only builds on Android now
# mac implementation uses (webotsCtrlAnim)
if (ANDROID)

  anki_build_cxx_executable(vic-anim ${ANKI_SRCLIST_DIR})

  target_link_libraries(vic-anim
    PRIVATE
    victor_anim
    victorCrashReports
    cti_common_robot
    util
    jsoncpp
    breakpad_client
    ${PLATFORM_LIBS}
  )

  android_strip(TARGET vic-anim)

endif()
