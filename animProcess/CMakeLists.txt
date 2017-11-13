cmake_minimum_required(VERSION 3.6)

project(cozmo_anim)

include(anki_build_cxx)

anki_build_cxx_library(victor_anim ${ANKI_SRCLIST_DIR} STATIC)

include(flatbuffers)
include(opencv)
include(signalessence)
include(speexdsp)
include(text2speech)
include(sensory)

if (NOT DEFINED USE_DAS)
  set(USE_DAS 0)
endif()

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES ${OPENCV_INCLUDE_PATHS})
set(PLATFORM_COMPILE_DEFS "")

if (ANDROID)
    set(PLATFORM_LIBS
        android
        log)
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
  # cti
  cti_common_robot
  cti_vision
  cti_messaging_robot
  robot_clad_cpplite
  robot_transport
  audio_engine
  audio_multiplexer_robot
  # vendor
  ${OPENCV_LIBS}
  ${FLATBUFFERS_LIBS} 
  ${SIGNALESSENCE_LIBS}
  ${SPEEXDSP_LIBS}
  ${TEXT2SPEECH_LIBS}
  ${SENSORY_LIBS}
  # platform
  ${PLATFORM_LIBS}
  PUBLIC
  robot_interface  # Needs to be public for cozmoConfig.h
)

android_strip(TARGET victor_anim)

target_compile_definitions(victor_anim
  PRIVATE
  USE_DAS=${USE_DAS}
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(victor_anim 
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src> # allow "android/" prefix
  ${PLATFORM_INCLUDES}
)

# victor_animator binary only builds on Android now
# mac implementation uses (webotsCtrlAnim)
if (ANDROID)

  anki_build_cxx_executable(victor_animator ${ANKI_SRCLIST_DIR})

  target_link_libraries(victor_animator
    PRIVATE
    victor_anim
    cti_common_robot
    util
    jsoncpp
    ${PLATFORM_LIBS}
  )

  android_strip(TARGET victor_animator)

  copy_tts_android_libs()

  add_dependencies(victor_animator
    copy_tts_libs
  )

endif()

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/signalEssence/policy_actions.c PROPERTIES COMPILE_FLAGS -Wno-shorten-64-to-32)
