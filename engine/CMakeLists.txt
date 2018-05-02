cmake_minimum_required(VERSION 3.6)

project(engine)

include(opencv)
include(okao)
include(flatbuffers)
include(archive)
include(android_strip)
include(anki_build_cxx)

anki_build_cxx_library(cozmo_engine ${ANKI_SRCLIST_DIR})

if (NOT DEFINED USE_DAS)
  set(USE_DAS 0)
endif()

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
set(PLATFORM_FLAGS "")
if (VICOS)
  include(breakpad)
  set(PLATFORM_LIBS log)
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
    ${WEBOTS_LIBS}
  )
  set(PLATFORM_COMPILE_DEFS
    SIMULATOR
    THF_FUNCTIONALITY=0
    VC_AUDIOCAPTURE_FUNCTIONALITY=0
  )
endif()

if(ANKI_HAS_LIBARCHIVE)
  list(APPEND PLATFORM_COMPILE_DEFS
    ANKI_HAS_LIBARCHIVE=1
  )
else()
  list(APPEND PLATFORM_COMPILE_DEFS
    ANKI_HAS_LIBARCHIVE=0
  )
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
  cameraService
  canned_anim_lib_engine
  #victor_web_library
  # cti
  cti_vision
  cti_planning
  cubeBleClient
  osState
  # vendor
  ${FLATBUFFERS_LIBS}
  ${LIBARCHIVE_LIBS}
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
  CORETECH_ENGINE
  USE_DAS=${USE_DAS}
  "${PLATFORM_COMPILE_DEFS}"
)

target_include_directories(cozmo_engine 
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # allows reference to engine/
  #PRIVATE
  #$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/anki/cozmo/basestation/cozmoAPI>
)

if (VICOS)
  # TODO: support tools on mac
  add_subdirectory(tools)

  # Placeholder install task for cozmo_engine. We will eventually transition to using
  # install for all libs & binaries in order to assign appropriate permissions.
  install(TARGETS cozmo_engine
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/dist/lib
    ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/dist/lib
  )
endif()

